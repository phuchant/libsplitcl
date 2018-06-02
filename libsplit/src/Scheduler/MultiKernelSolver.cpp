#include <Options.h>
#include <Scheduler/MultiKernelSolver.h>
#include <Utils/Debug.h>

#include <iostream>

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define EPSILON 1.0e-10
#define SPEED_TRESHOLD 1.10
#define MIN_ITER 10

namespace libsplit {
  MultiKernelSolver::MultiKernelSolver(int nbDevices, int nbKernels)
    : nbDevices(nbDevices), nbKernels(nbKernels) {
    kernelGr = new double *[nbKernels]();
    kernelPerf = new double *[nbKernels]();
    for (int i=0; i<nbKernels; i++) {
      kernelGr[i] = new double[nbDevices]();
      kernelPerf[i] = new double[nbDevices]();
    }

    kernelsD2HConstraints = new double ***[nbKernels]();
    kernelsH2DConstraints = new double ***[nbKernels]();
    for (int i=0; i<nbKernels; i++) {
      kernelsH2DConstraints[i] = new double **[nbKernels]();
      kernelsD2HConstraints[i] = new double **[nbKernels]();
      for (int j=0; j<nbKernels; j++) {
	kernelsD2HConstraints[i][j] = new double *[nbDevices]();
	kernelsH2DConstraints[i][j] = new double *[nbDevices]();
	for (int k=0; k<nbDevices; k++) {
	  kernelsD2HConstraints[i][j][k] = new double[2*nbDevices]();
	  kernelsH2DConstraints[i][j][k] = new double[2*nbDevices]();
	}
      }
    }

    lastPartition = NULL;

    createGlpProb();
  }

  MultiKernelSolver::~MultiKernelSolver() {
    glp_delete_prob(lp);
    delete[] ia;
    delete[] ja;
    delete[] ar;

    for (int i=0; i<nbKernels; i++) {
      delete[] kernelGr[i];
      delete[] kernelPerf[i];
    }
    delete[] kernelGr;
    delete[] kernelPerf;

    for (int i=0; i<nbKernels; i++) {
      for (int j=0; j<nbKernels; j++) {
	for (int k=0; k<nbDevices; k++) {
	  delete[] kernelsD2HConstraints[i][j][k];
	  delete[] kernelsH2DConstraints[i][j][k];
	}
	delete[] kernelsD2HConstraints[i][j];
	delete[] kernelsH2DConstraints[i][j];
      }
      delete[] kernelsD2HConstraints[i];
      delete[] kernelsH2DConstraints[i];
    }

    delete[] kernelsD2HConstraints;
    delete[] kernelsH2DConstraints;
  }

  void
  MultiKernelSolver::setKernelGr(int kerId, const double *gr) {
    memcpy(kernelGr[kerId], gr, nbDevices*sizeof(double));
  }

  void
  MultiKernelSolver::setKernelPerf(int kerId, const double *perf) {
    memcpy(kernelPerf[kerId], perf, nbDevices*sizeof(double));
  }

  void
  MultiKernelSolver::setKernelsD2HConstraint(int kerId, int src, int devId,
					     const double *coefs)
  {
    assert(kerId != src);
    if (optMKGRNoComm)
      return;
    memcpy(kernelsD2HConstraints[kerId][src][devId], coefs, 2*nbDevices*sizeof(double));
  }

  void
  MultiKernelSolver::setKernelsH2DConstraint(int kerId, int src, int devId,
					     const double *coefs)
  {
    assert(kerId != src);
    if (optMKGRNoComm)
      return;
    memcpy(kernelsH2DConstraints[kerId][src][devId], coefs, 2*nbDevices*sizeof(double));
  }

  static bool deviceOverlapBetweenKernels(int nbDevices, int dev,
					  const double *prev_gr,
					  const double *cur_gr,
					  const double *prev_prefix_gr,
					  const double *cur_prefix_gr)
  {
    if (prev_gr[dev] <= EPSILON || cur_gr[dev] <= EPSILON)
      return false;

    if (dev == 0 || dev == nbDevices-1)
      return true;


    // ----        | (A)
    //       ----- v
    //
    //       ----- | (B)
    // -----       v
    return !(prev_prefix_gr[dev+1] + EPSILON < cur_prefix_gr[dev] || // (A)
	     cur_prefix_gr[dev+1]  + EPSILON < prev_prefix_gr[dev]);  // (B)
  }



  bool
  MultiKernelSolver::getD2HConstraint(int dev,
				      const double *prev_gr,
				      const double *cur_gr,
				      const double *prev_prefix_gr,
				      const double *cur_prefix_gr,
				      int **constraint) {
    *constraint = (int *) calloc(2 * nbDevices, sizeof(double));

    // Null granularity -> co comm observed
    if (prev_gr[dev] <= EPSILON)
      return false;

    // First or last device
    // ----       or       ----  | (negative constraint, no comm observed))
    // --------        --------  v

    // --------        --------  v
    // ----       or       ----  |
    if (dev == 0 || dev == nbDevices-1) {
      (*constraint)[dev] = 1;
      (*constraint)[nbDevices + dev] = -1;

      if (prev_gr[dev]  < cur_gr[dev])
	return false;

      return true;
    }

    // Included (negative constraint, no comm observed)
    //         ----      |
    //      ----------   v
    if ( dev > 0 && dev < nbDevices-1 &&
	 cur_prefix_gr[dev]  < prev_prefix_gr[dev] + EPSILON &&
	 prev_prefix_gr[dev+1]  < cur_prefix_gr[dev+1] + EPSILON) {
      for (int i=0; i<=dev-1; i++) {
	(*constraint)[nbDevices + i] += 1;
	(*constraint)[i] -= 1;
      }
      for (int i=0; i<=dev; i++) {
	(*constraint)[i] += 1;
	(*constraint)[nbDevices+ i] -= 1;
      }

      return false;
    }


    // No overlap : k_prev_d
    if (!deviceOverlapBetweenKernels(nbDevices, dev, prev_gr, cur_gr,
				     prev_prefix_gr, cur_prefix_gr)) {
      (*constraint)[dev] = 1;
      return true;
    }

    // Overlap both sides
    // ----------
    // ###----###
    if ( dev > 0 && dev < nbDevices-1 &&
	 prev_prefix_gr[dev]  < cur_prefix_gr[dev] &&
	 cur_prefix_gr[dev+1]  < prev_prefix_gr[dev+1]) {
      for (int i=0; i<=dev-1; i++) {
	(*constraint)[nbDevices+i] += 1;
	(*constraint)[i] -= 1;
      }
      for (int i=0; i<=dev; i++) {
	(*constraint)[i] += 1;
	(*constraint)[nbDevices+i] -= 1;
      }
      return true;
    }

    // Overlap one side
    // |-------         |
    // ###|--------     v
    if (prev_prefix_gr[dev]  < cur_prefix_gr[dev]) {
      for (int i=0; i<=dev-1; i++) {
	(*constraint)[nbDevices+i] = 1;
	(*constraint)[i] = -1;
      }

      return true;
    }

    //    |--------  |
    // |------#####  v
    for (int i=0; i<=dev; i++) {
      (*constraint)[i] = 1;
      (*constraint)[nbDevices+i] = -1;
    }
    return true;
  }

  bool
  MultiKernelSolver::getH2DConstraint(int dev,
				      const double *prev_gr,
				      const double *cur_gr,
				      const double *prev_prefix_gr,
				      const double *cur_prefix_gr,
				      int **constraint) {
    *constraint = (int *) calloc(2 * nbDevices, sizeof(double));

    // Null granularity -> no comm observed
    if (cur_gr[dev] <= EPSILON)
      return false;

    // First or last device
    // --------        -------- | (negative constraint, no comm observed)
    // ----       or       ---- v

    // ----                ---- |
    // --------   or   -------- v
    if (dev == 0 || dev == nbDevices-1) {
      (*constraint)[nbDevices+dev] = 1;
      (*constraint)[dev] = -1;

      if (cur_gr[dev]  < prev_gr[dev])
	return false;

      return true;
    }


    // Included
    // -------- |  (negative constraint, no com observed)
    //   ----   v
    if ( dev > 0 && dev < nbDevices-1 &&
	 prev_prefix_gr[dev]  < cur_prefix_gr[dev] + EPSILON &&
	 cur_prefix_gr[dev+1]  < prev_prefix_gr[dev+1] + EPSILON) {
      for (int i=0; i<=dev-1; i++) {
	(*constraint)[i] += 1;
	(*constraint)[nbDevices + i] -= 1;
      }
      for (int i=0; i<=dev; i++) {
	(*constraint)[nbDevices + i] += 1;
	(*constraint)[i] -= 1;
      }

      return false;
    }

    // No overlap : k_d
    if (!deviceOverlapBetweenKernels(nbDevices, dev, prev_gr, cur_gr,
				     prev_prefix_gr, cur_prefix_gr)) {
      (*constraint)[nbDevices + dev] = 1;
      return true;
    }

    // Overlap both sides
    // ###----###
    // ----------
    if ( dev > 0 && dev < nbDevices-1 &&
	 prev_prefix_gr[dev] > cur_prefix_gr[dev]   &&
	 cur_prefix_gr[dev+1] > prev_prefix_gr[dev+1] ) {
      for (int i=0; i<=dev-1; i++) {
	(*constraint)[i] += 1;
	(*constraint)[nbDevices+i] -= 1;
      }
      for (int i=0; i<=dev; i++) {
	(*constraint)[nbDevices+i] += 1;
	(*constraint)[i] -= 1;
      }
      return true;
    }

    // Overlap one side
    // |-------####    |
    //    |--------    v
    if (prev_prefix_gr[dev+1] < cur_prefix_gr[dev+1] ) {
      for (int i=0; i<=dev; i++) {
	(*constraint)[nbDevices+i] = 1;
	(*constraint)[i] = -1;
      }

      return true;
    }

    // ###|--------  |
    // |-------      v
      for (int i=0; i<=dev-1; i++) {
	(*constraint)[i] = 1;
	(*constraint)[nbDevices+i] = -1;
      }

      return true;
  }

  int
  MultiKernelSolver::get_keri_devj_rowIdx(int i, int j) const {
    return 1 + i * (2 * nbDevices + 1) + j;
  }

  int
  MultiKernelSolver::get_gr_keri_devj_rowIdx(int i, int j) const {
    return 1 + i * (2 * nbDevices + 1) + nbDevices + j;
  }

  int
  MultiKernelSolver::get_gr_keri_rowIdx(int i) const {
    return 1 + i * (2 * nbDevices + 1) + 2 * nbDevices;
  }

  int
  MultiKernelSolver::get_keri_from_kerj_Dk2H_rowIdx(int i, int j, int k) const {
    int rowId = 1 +
      nbKernels * (2 * nbDevices + 1) + // kernels constraints offset
      i * (nbKernels * nbDevices) +
      j * nbDevices + k;

    return rowId;
  }

  int
  MultiKernelSolver::get_keri_from_kerj_H2Dk_rowIdx(int i, int j, int k) const {
    int rowId = 1 +
      nbKernels * (2 * nbDevices + 1) + // kernels constraints offset
      nbKernels * nbKernels * nbDevices  + // D2H constraints offset
      i * (nbKernels * nbDevices) +
      j * nbDevices + k;

    return rowId;
  }

  int
  MultiKernelSolver::get_T_D2H_keri_colIdx(int i) const {
    return 1 + nbKernels * (2 * nbDevices) + i * 2;
  }

  int
  MultiKernelSolver::get_T_H2D_keri_colIdx(int i) const {
    return 1 + nbKernels * (2 * nbDevices) + i * 2 + 1;
  }

  int
  MultiKernelSolver::get_T_keri_colIdx(int i) const {
    return 1 + nbKernels * (2 * nbDevices) + nbKernels * 2 + i;
  }

  int
  MultiKernelSolver::get_x_kidj_colIdx(int i, int j) const {
    return 1 + i * (2 * nbDevices) + j;
  }

  int
  MultiKernelSolver::get_gr_kidj_colIdx(int i, int j) const {
    return 1 + i * (2 * nbDevices) + nbDevices + j;
  }



  void
  MultiKernelSolver::createGlpProb() {
    // 1) Create GLPK Problem

    const int nbKernelTimeConstraints = nbKernels * nbDevices;
    const int nbGranuConstraints = nbKernels * nbDevices;
    const int nbGranuSumConstraints = nbKernels;
    const int nbKernelsD2HConstraints = nbKernels * nbKernels * nbDevices;
    const int nbKernelsH2DConstraints = nbKernels * nbKernels * nbDevices;

    nbRows = nbKernelTimeConstraints + nbGranuConstraints +
      nbGranuSumConstraints + nbKernelsD2HConstraints +
      nbKernelsH2DConstraints;
    nbCols = nbKernels*3 + nbKernels*2*nbDevices;

    ia = new int[1+nbRows*nbCols]();
    ja = new int[1+nbRows*nbCols]();
    ar = new double[(nbRows+1)*(nbCols+1)]();
    lp = glp_create_prob();
    glp_term_out(GLP_OFF);

    glp_set_prob_name(lp, "kernel granularity");
    glp_set_obj_dir(lp, GLP_MIN);

    int rowIdx = 1;
    int colIdx = 1;
    char *name = NULL;

    // Rows
    glp_add_rows(lp, nbRows);

    // Kernel constraints
    for (int i=0; i<nbKernels; i++) {
      // ker%d_dev%d
      for (int j=0; j<nbDevices; j++) {
	asprintf(&name, "ker%d_dev%d", i, j);
	glp_set_row_name(lp, rowIdx, name);
	free(name);
	glp_set_row_bnds(lp, rowIdx, GLP_UP, 0.0, 0.0);
	rowIdx++;
      }

      // gr_ker%d_dev%d
      for (int j=0; j<nbDevices; j++) {
	asprintf(&name, "gr_ker%d_dev%d", i, j);
	glp_set_row_name(lp, rowIdx, name);
	free(name);
	glp_set_row_bnds(lp, rowIdx, GLP_FX, 0.0, 0.0);
	rowIdx++;
      }

      // gr_ker%d
      asprintf(&name, "gr_ker%d", i);
      glp_set_row_name(lp, rowIdx, name);
      free(name);
      glp_set_row_bnds(lp, rowIdx, GLP_FX, 1.0, 0.0);
      rowIdx++;
    }

    // D2H Comm constraints
    for (int i=0; i<nbKernels; i++) {

      // ker%d_ker%d_D%dtoH
      for (int j=0; j<nbKernels; j++) {
	for (int k=0; k<nbDevices; k++) {
	  asprintf(&name, "ker%d_from_k%d_D%dtoH", i, j, k);
	  glp_set_row_name(lp, rowIdx, name);
	  free(name);
	  glp_set_row_bnds(lp, rowIdx, GLP_UP, 0.0, 0.0);
	  rowIdx++;
	}
      }
    }

    // H2D Comm constraints
    for (int i=0; i<nbKernels; i++) {
      // ker%d_ker%d_HtoD%d
      for (int j=0; j<nbKernels; j++) {
	for (int k=0; k<nbDevices; k++) {
	  asprintf(&name, "ker%d_from_k%d_HtoD%d", i, j, k);
	  glp_set_row_name(lp, rowIdx, name);
	  free(name);
	  glp_set_row_bnds(lp, rowIdx, GLP_UP, 0.0, 0.0);
	  rowIdx++;
	}
      }
    }

    // Cols
    glp_add_cols(lp, nbCols);

    for (int i=0; i<nbKernels; i++) {
      // x_k%dd%d
      for (int j=0; j<nbDevices; j++) {
	asprintf(&name, "x_k%dd%d", i, j);
	glp_set_col_name(lp, colIdx, name);
	glp_set_obj_coef(lp, colIdx, 0);
	glp_set_col_bnds(lp, colIdx, GLP_LO, 0.0, 0.0);
	free(name);
	colIdx++;
      }

      // gr_k%dd%d
      for (int j=0; j<nbDevices; j++) {
	asprintf(&name, "gr_k%dd%d", i, j);
	glp_set_col_name(lp, colIdx, name);
	glp_set_obj_coef(lp, colIdx, 0);
	glp_set_col_bnds(lp, colIdx, GLP_LO, 0.0, 0.0);
	free(name);
	colIdx++;
      }

    }


    for (int i=0; i<nbKernels; i++) {
      // T_D2H_ker%d
      asprintf(&name, "T_D2H_ker%d", i);
      glp_set_col_name(lp, colIdx, name);
      free(name);
      glp_set_col_bnds(lp, colIdx, GLP_LO, 0.0, 0.0);
      glp_set_obj_coef(lp, colIdx, 1.0);
      colIdx++;
      // T_H2D_ker%d
      asprintf(&name, "T_H2D_ker%d", i);
      glp_set_col_name(lp, colIdx, name);
      free(name);
      glp_set_col_bnds(lp, colIdx, GLP_LO, 0.0, 0.0);
      glp_set_obj_coef(lp, colIdx, 1.0);
      colIdx++;
    }

    for (int i=0; i<nbKernels; i++) {
      // T_ker%d
      asprintf(&name, "T_ker%d", i);
      glp_set_col_name(lp, colIdx, name);
      free(name);
      glp_set_obj_coef(lp, colIdx, 1.0);
      glp_set_col_bnds(lp, colIdx, GLP_LO, 0.0, 0.0);
      colIdx++;
    }
  }

  void
  MultiKernelSolver::updateGlpMatrix()
  {
    // Reset matrix
    memset(ia, 0, (1+nbRows*nbCols) * sizeof(int));
    memset(ja, 0, (1+nbRows*nbCols) * sizeof(int));
    memset(ar, 0, (1+nbRows*nbCols) * sizeof(double));

    // Fill matrix

    int idx = 1;

    // Kernels Constraints
    for (int i=0; i<nbKernels; i++) {
      for (int j=0; j<nbDevices; j++) {
	// kernelperf
	// ex:  ker0_dev0: - T_k0 + 32.64 x_k0_d0 <= 0

	ia[idx] = get_keri_devj_rowIdx(i, j);
	ja[idx] = get_T_keri_colIdx(i);
	ar[idx] = -1.0;
	idx++;
	ia[idx] = ia[idx-1];
	ja[idx] = get_x_kidj_colIdx(i, j);
	ar[idx] = kernelPerf[i][j];
	idx++;

	// kernelgr
	// ex:  gr_ker0_dev0: - gr_k0_d0 + 0.4 x_k0_d0 = 0
	ia[idx] = get_gr_keri_devj_rowIdx(i, j);
	ja[idx] = get_gr_kidj_colIdx(i, j);
	ar[idx] = -1.0;
	idx++;
	ia[idx] = ia[idx-1];
	ja[idx] = get_x_kidj_colIdx(i, j);
	ar[idx] = kernelGr[i][j];
	idx++;
      }

      // gr_ker0: + gr_k0_d2 + gr_k0_d1 + gr_k0_d0 = 1
      for (int j=0; j<nbDevices; j++) {
	ia[idx] = get_gr_keri_rowIdx(i);
	ja[idx] = get_gr_kidj_colIdx(i, j);
	ar[idx] = 1;
	idx++;
      }
    }

    for (int i=1; i<idx; i++) {
      assert(ia[i] < get_keri_from_kerj_Dk2H_rowIdx(0, 0, 0));
    }

    // Comm constraints
    for (int kcur=0; kcur<nbKernels; kcur++) {
      for (int kprev=0; kprev<nbKernels; kprev++) {
	// if (kcur == kprev)
	//   continue;

	for (int dev=0; dev<nbDevices; dev++) {

	  // D2HConstraints

	  bool isConstraint = false;

	  // kprev coefs
	  for (int c=0; c<nbDevices; c++) {
	    if (kernelsD2HConstraints[kcur][kprev][dev][c] == 0)
	      continue;

	    isConstraint = true;
	    ia[idx] = get_keri_from_kerj_Dk2H_rowIdx(kcur, kprev, dev);
	    ja[idx] = get_gr_kidj_colIdx(kprev, c);
	    ar[idx] = kernelsD2HConstraints[kcur][kprev][dev][c];
	    idx++;
	  }

	  // kcur coefs
	  for (int c=0; c<nbDevices; c++) {
	    if (kernelsD2HConstraints[kcur][kprev][dev][nbDevices+c] == 0)
	      continue;

	    isConstraint = true;
	    ia[idx] = get_keri_from_kerj_Dk2H_rowIdx(kcur, kprev, dev);

	    ja[idx] = get_gr_kidj_colIdx(kcur, c);
	    ar[idx] = kernelsD2HConstraints[kcur][kprev][dev][nbDevices+c];
	    idx++;
	  }

	  // T_D2H_keri
	  if (isConstraint) {
	    ia[idx] = get_keri_from_kerj_Dk2H_rowIdx(kcur, kprev, dev);
	    ja[idx] = get_T_D2H_keri_colIdx(kcur);
	    ar[idx] = -1.0;
	    idx++;
	  }

	  // H2DConstraints
	  isConstraint = false;

	  // kprev coefs
	  for (int c=0; c<nbDevices; c++) {
	    if (kernelsH2DConstraints[kcur][kprev][dev][c] == 0)
	      continue;

	    isConstraint = true;
	    ia[idx] = get_keri_from_kerj_H2Dk_rowIdx(kcur, kprev, dev);
	    ja[idx] = get_gr_kidj_colIdx(kprev, c);
	    ar[idx] = kernelsH2DConstraints[kcur][kprev][dev][c];
	    idx++;
	  }

	  // kcur coefs
	  for (int c=0; c<nbDevices; c++) {
	    if (kernelsH2DConstraints[kcur][kprev][dev][nbDevices+c] == 0)
	      continue;

	    isConstraint = true;
	    ia[idx] = get_keri_from_kerj_H2Dk_rowIdx(kcur, kprev, dev);
	    ja[idx] = get_gr_kidj_colIdx(kcur, c);
	    ar[idx] = kernelsH2DConstraints[kcur][kprev][dev][nbDevices+c];
	    idx++;
	  }

	  // T_D2H_keri
	  if (isConstraint) {
	    ia[idx] = get_keri_from_kerj_H2Dk_rowIdx(kcur, kprev, dev);
	    ja[idx] = get_T_H2D_keri_colIdx(kcur);
	    ar[idx] = -1.0;
	    idx++;
	  }
	}
      }
    }

    int nnz = idx-1;
    assert(nnz <= 1 + nbRows*nbCols);

    for (int i=1; i<=nnz; i++) {
      assert(ia[i] <= nbRows);
      assert(ja[i] <= nbCols);
    }

    glp_load_matrix(lp, nnz, ia, ja, ar);
  }

  void MultiKernelSolver::dumpProb() {
    static unsigned iter=0;
    iter++;
    char fileprob[128];
    char filesol[128];
    sprintf(fileprob, "prob%d.txt", iter);
    sprintf(filesol, "sol%d.txt", iter);
    glp_write_lp(lp, NULL, fileprob);
    glp_print_sol(lp, filesol);
  }

  double *
  MultiKernelSolver::getGranularities() {
    // Update problem
    updateGlpMatrix();

    glp_scale_prob(lp, GLP_SF_AUTO);
    glp_std_basis(lp);

    // Resolve
    glp_simplex(lp, NULL);

    // debug
    DEBUG("mkgr_prob", dumpProb());

    // Get solution
    double *ret = new double[nbKernels * nbDevices];

    for (int i=0; i<nbKernels; i++) {
      for (int j=0; j<nbDevices; j++) {
	int colIdx = 1 + i * 2 * nbDevices + nbDevices + j; // gr_ki_dj
	ret[i*nbDevices + j] = glp_get_col_prim(lp, colIdx);
      }
    }

    // Reset kernels comm constraints
    for (int i=0; i<nbKernels; i++) {
      for (int j=0; j<nbKernels; j++) {
	for (int k=0; k<nbDevices; k++) {
	  memset(kernelsD2HConstraints[i][j][k], 0, 2*nbDevices*sizeof(double));
	  memset(kernelsH2DConstraints[i][j][k], 0, 2*nbDevices*sizeof(double));
	}
      }
    }

    // Get objective time with solution
    double obj = glp_get_obj_val(lp);

    // Get objective time with previous partition
    for (int i=0; i<nbKernels; i++) {
      for (int j=0; j<nbDevices;j++) {
	int colIdx = 1 + i * 2 * nbDevices + nbDevices + j; // gr_ki_dj
	glp_set_col_bnds(lp, colIdx, GLP_FX, kernelGr[i][j], kernelGr[i][j]);
      }
    }
    glp_simplex(lp,NULL);
    double objcur = glp_get_obj_val(lp);

    // Restore problem
    for (int i=0; i<nbKernels; i++) {
      for (int j=0; j<nbDevices;j++) {
	int colIdx = 1 + i * 2 * nbDevices + nbDevices + j; // gr_ki_dj
	glp_set_col_bnds(lp, colIdx, GLP_LO, 0.0, 0.0);
      }
    }

    double potential_speedup = objcur / obj;

    static int curiter = 0;
    curiter++;

    // Do not return a new partition if the potential speedup is lower
    // than SPEED_TRESHOLD
    if (curiter > MIN_ITER && potential_speedup < SPEED_TRESHOLD)
      return NULL;

    return ret;
  }

};
