#include <Options.h>
#include "SchedulerMKGR.h"

#include <iostream>
#include <map>

#include <cassert>
#include <gsl/gsl_fit.h>

extern "C" {
#include <mkgr.h>
}

// -----------------------------------------------------------------------------
// Data shared by all kernels in the cycle
static mkgr_t pl = NULL;
static double *global_granu_dscr = NULL;
static int global_size_gr;

// For each kernel in the cycle, communication time to each device
static double **cycle_comm = NULL; // [cyclelength][nbdevices]

// For each kernel in the cycle, granularity on each device
static double **cycle_granu = NULL; // [cyclelength][nbdevices]

// For each kernel k in the cycle, and for each device d,
// there is a map used to store the points of the function computing
// the communication time from the difference of granularities.
// The map is of the form:
// <gr_ker_k_dev_d - gr_ker_kprev_dev_d, comm_time_ker_k_dev_d>
static std::map<double, double> **dcommMap = NULL;

// Store the previous size of the map, this is used to know when to recompute
// the linear regression.
static unsigned **dcommMapSizePrev = NULL;

// Communication coefficients for each kernel and for each devices.
// These coefficients are computed by computing a linear regression
// of dcommMap.
static double **cycle_dcomm_auto = NULL; // [cyclelength][nbdevices]

// Array of comm perfs sent to mkgr
static double **comm_perf_from_to = NULL; // [cyclelength][nbdevice *  2 * 3]
// nb constraints = 2 * cyclelength * (nbdevices)
//
// example with 2 devices and 2 kernels
// kernel0:
// c1: comm k0 dev 0 to dev 1              (used only if k1gr1 < k0gr1 in mkgr)
// c2: comm k0 dev 0 to dev 1 (negative)   (used only if k1gr1 < k0gr1 in mkgr)
// c3: comm k0 dev1 to dev 0               (used only if k1gr0 < k0gr0 in mkgr)
// c4: comm k0 dev1 to dev 0  (negative)   (used only if k1gr0 < k0gr0 in mkgr)
// kernel1:
// c5: comm k1 dev 0 to dev 1              (used only if k0gr1 < k1gr1 in mkgr)
// c6: comm k1 dev 0 to dev 1 (negative)   (used only if k0gr1 < k1gr1 in mkgr)
// c7: comm k1 dev1 to dev 0               (used only if k0gr0 < k1gr0 in mkgr)
// c8: comm k1 dev1 to dev 0  (negative)   (used only if k0gr0 < k1gr0 in mkgr)
//
// mapping from cycle_dcomm:
// c1 = cycle_dcomm[0][1]
// c2 = -1 * cycle_dcomm[0][0]
// c3 = cycle_dcomm[0][0]
// c4 = -1 * cycle_dcomm[0][1]
// c5 = cycle_dcomm[1][1]
// c6 = -1 * cycle_dcomm[1][0]
// c7 = cycle_dcomm[1][0]
// c8 = -1 * cycle_dcomm[1][1]
//
// ex cycle 0 (k0 to k1): fromdev0,todev1,c5,fromdev0,todev1,c6,
//                        fromdev1,todev0,c7,fromdev1,todev0,c8


// -------------------------------------------------------------------------

SchedulerMKGR::SchedulerMKGR(unsigned nbDevices, DeviceInfo *devInfo,
			     unsigned numArgs, unsigned nbGlobals,
			     KernelAnalysis *analysis)
  : SchedulerAutoAbs(nbDevices, devInfo, numArgs, nbGlobals, analysis) {

  if (nbDevices > 2) {
    std::cerr << "Error: MKGR doesn't work with more than 2 devices.\n";
    exit(EXIT_FAILURE);
  }

  if (id > optCycleLength-1) {
    std::cerr << "Error kernel id is " << id << " and cycle length is "
	      << optCycleLength << "\n";
    exit(EXIT_FAILURE);
  }


  // Allocate data for this kernel
  kernel_perfs = new double[nbDevices * 3];

  // Allocate data shared by all kernel in the cycle
  if (!pl)
    pl = mkgr_init(nbDevices);

  if (!cycle_comm) {
    cycle_comm = new double *[optCycleLength];
    cycle_granu = new double *[optCycleLength];
    cycle_dcomm_auto = new double *[optCycleLength];
    dcommMap = new std::map<double, double> *[optCycleLength];
    dcommMapSizePrev = new unsigned *[optCycleLength];

    comm_perf_from_to = new double *[optCycleLength];

    for (unsigned i=0; i<optCycleLength; ++i) {
      cycle_comm[i] = new double[nbDevices];
      cycle_granu[i] = new double[nbDevices];
      cycle_dcomm_auto[i] = new double[nbDevices];
      dcommMap[i] = new std::map<double, double>[nbDevices];
      dcommMapSizePrev[i] = new unsigned[nbDevices];

      comm_perf_from_to[i] = new double[nbDevices * 6];

      for (unsigned j=0; j<nbDevices;j++) {
	cycle_comm[i][j] = 0;
	cycle_granu[i][j] = 0;
	cycle_dcomm_auto[i][j] = 0;
	dcommMapSizePrev[i][j] = 0;
      }

      // Works only with 2 devices
      for (unsigned j=0; j<nbDevices; j++) {
	comm_perf_from_to[i][6*j + 0] = 1 - j; // device from
	comm_perf_from_to[i][6*j + 1] = j; // device to
	comm_perf_from_to[i][6*j + 2] = 0; // dcomm
	comm_perf_from_to[i][6*j + 3] = 1 - j; // device from
	comm_perf_from_to[i][6*j + 4] = j; // device to
	comm_perf_from_to[i][6*j + 5] = 0; // negative dcomm
      }
    }
  }
}

SchedulerMKGR::~SchedulerMKGR() {
  if (pl) {
    free(pl);
    pl = NULL;
  }
}

void
SchedulerMKGR::initScheduler(const size_t *global_work_size,
			     const size_t *local_work_size, unsigned dim) {
  (void) local_work_size;
  (void) global_work_size;
  (void) dim;
}

void
SchedulerMKGR::endIter() {
  SchedulerAutoAbs::endIter();

  // Do not measure first iteration because there can me some communication
  // made only the first time the kernel is launched
  // then, do not measure communication just after changing the granularity
  // for the same reason
  // iter1, iter3, iter5, iter7, ...
  if (iterno % 2 != 1)
    return;

  int nbSplits = size_gr / 3;

  // Update kernel_perfs
  for (int i=0; i<nbSplits; ++i) {
    kernel_perfs[i*3] = granu_dscr[i*3]; // dev id
    kernel_perfs[i*3+1] = granu_dscr[i*3+2]; // granu
    kernel_perfs[i*3+2] = kernel_time[(int) granu_dscr[i*3]]; // time
  }

  assert(id < optCycleLength);
  assert(nbSplits == (int)nbDevices);

  // Update cycle_comm and cycle granu
  std::cerr << "comm time kernel " << id << " : ";
  for (unsigned i=0; i<nbDevices; i++) {
    cycle_comm[id][i] = comm_time[i];
    cycle_granu[id][i] = 0;
    std::cerr << cycle_comm[id][i] << " ";
  }
  std::cerr << "\n";

  for (int i=0; i<nbSplits; i++) {
    cycle_granu[id][(int) granu_dscr[i*3]] = granu_dscr[i*3+2];
  }

  // set kernel_perfs to MKGR
  mkgr_set_kernel_performance(pl, id, kernel_perfs, nbSplits * 3);
  std::cerr << "set kernel " << id << " perf : ";
  for (int i =0; i<nbSplits*3; ++i)
    std::cerr << kernel_perfs[i] << " ";
  std::cerr << "\n";

  // Only last kernel update dcomm and set comm_perfs to mkgr
  if (id != optCycleLength -1)
    return;

  // Update dcomm
  for (unsigned ki=0; ki<optCycleLength; ki++) {
    int kprev  = (ki + optCycleLength - 1) % optCycleLength;
    for (unsigned d=0; d<nbDevices; d++) {

      // Insert a new point in the map for communication
      if (cycle_granu[ki][d] >= cycle_granu[kprev][d]) {
	double granu_diff = cycle_granu[ki][d] - cycle_granu[kprev][d];
	double time_measured = cycle_comm[ki][d];
	dcommMap[ki][d].insert(std::pair<double, double>(granu_diff,
							 time_measured));
      }

      // If the size of the map for communication has changed and there is at
      // least 2 measures, recompute the linear regression.
      unsigned dcommMapSize = dcommMap[ki][d].size();
      if (dcommMapSize >= 2 && dcommMapSize > dcommMapSizePrev[ki][d]) {
	dcommMapSizePrev[ki][d] = dcommMapSize;

	double Xvector[dcommMapSize], Yvector[dcommMapSize];
	unsigned vecId = 0;
	for (auto I = dcommMap[ki][d].begin(), E = dcommMap[ki][d].end();
	     I != E; ++I) {
	  Xvector[vecId] = I->first;
	  Yvector[vecId] = I->second;
	  vecId++;
	}
	double c0, c1, cov00, cov01, cov11, sumsq;
	gsl_fit_linear(Xvector, 1, Yvector, 1, dcommMapSize,
		       &c0, &c1, &cov00, &cov01, &cov11, &sumsq);
	// std::cerr << "comm constraint, kernel " << ki << " dev " << d
	// 	  << " : c0=" << c0 << " c1=" << c1 << "\n";
	cycle_dcomm_auto[ki][d] = c1;
      }
    }
    // std::cerr << "\n";
  }

  // Update comm_perf_from_to
  for (unsigned ki=0; ki<optCycleLength; ki++) {
    for (unsigned d=0;d<nbDevices; d++) {
      // positive constraint
      comm_perf_from_to[ki][6*d+2] = cycle_dcomm_auto[ki][d];
      // negative constraint (only works with 2 devices)
      comm_perf_from_to[ki][6*d+5] = -1 * cycle_dcomm_auto[ki][1-d];
    }

    // Set comm_perfs for this kernel
    int kprev = (ki + optCycleLength -1) % optCycleLength;
    mkgr_set_comm_performance(pl,
    			      kprev, /* id from */
    			      ki /* id to */,
    			      comm_perf_from_to[ki], nbDevices * 6);
    std::cerr << "Set comm perf from k"<< kprev << " to " << ki << ": ";
    for (unsigned i=0; i<6*nbDevices; i++)
      std::cerr << comm_perf_from_to[ki][i] << " ";
    std::cerr << "\n";

    // for (unsigned d=0; d<nbDevices;d++) {
    //   std::cerr << "map kernel " << ki << " dev " << d << " : ";
    //   for (auto I = dcommMap[ki][d].begin(), E = dcommMap[ki][d].end();
    // 	   I != E; ++I) {
    // 	std::cerr << "<" << I->first << "," << I->second << "> ";
    //   }
    //   std::cerr << "\n";
    // }
  }
}

void
SchedulerMKGR::getGranu() {
  iterno++;

  // Set perfs and return
  if (iterno % 2 != 0)
    return;

  if (id == 0) {
    mkgr_get_granularities(pl, &global_granu_dscr, &global_size_gr,
			   &objective_time);
    std::cerr << "get granularities : ";
    for (int i=0; i<global_size_gr; ++i)
      std::cerr << global_granu_dscr[i] << " ";
    std::cerr << "\n";
  }

  double *my_granu = NULL;
  for (int i=0; i<global_size_gr/5; ++i) {
    if (global_granu_dscr[i*5] != id)
      continue;
    my_granu = &global_granu_dscr[i*5+1];
  }
  assert(my_granu);

  int nbGranu = 0;
  for (int i=0; i<2; ++i) {
    if (my_granu[i*2+1] > 0)
      nbGranu++;
  }

  size_gr = nbGranu * 3;
  int curIdx = 0;
  for (int i=0; i<2; ++i) {
    if (my_granu[i*2+1] == 0)
      continue;

    granu_dscr[curIdx++] = my_granu[i*2]; //dev id
    granu_dscr[curIdx++] = 1; // nbkernels
    granu_dscr[curIdx++] = my_granu[i*2+1]; //granu
  }

  std::cerr << "ker " << id << " granu_dscr: ";
  for (int i=0; i<size_gr; ++i)
    std::cerr << granu_dscr[i] << " ";
  std::cerr << "\n";

  if (id == 2)
    free(global_granu_dscr);
}
