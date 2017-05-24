#include <Scheduler/MultiKernelSolver.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define EPSILON 1.0e-10

namespace libsplit {
  MultiKernelSolver::MultiKernelSolver(int nbDevices, int nbKernels)
    : nbDevices(nbDevices), nbKernels(nbKernels) {
    kernelGr = new double *[nbKernels]();
    kernelPerf = new double *[nbKernels]();
    for (int i=0; i<nbKernels; i++) {
      kernelGr[i] = new double[nbDevices]();
      kernelPerf[i] = new double[nbDevices]();
    }

    D2HConstraints = new double **[nbKernels]();
    H2DConstraints = new double **[nbKernels]();
    for (int i=0; i<nbKernels; i++) {
      D2HConstraints[i] = new double *[nbDevices]();
      H2DConstraints[i] = new double *[nbDevices]();
      for (int j=0; j<nbDevices; j++) {
	D2HConstraints[i][j] = new double[2*nbDevices]();
	H2DConstraints[i][j] = new double[2*nbDevices]();
      }
    }

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
      for (int j=0; j<nbDevices; j++) {
	delete[] D2HConstraints[i][j];
	delete[] H2DConstraints[i][j];
      }

      delete[] D2HConstraints[i];
      delete[] H2DConstraints[i];
    }

    delete[] D2HConstraints;
    delete[] H2DConstraints;
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
  MultiKernelSolver::setD2HConstraint(int kerId, int devId, const double *coefs)
  {
    memcpy(D2HConstraints[kerId][devId], coefs, 2*nbDevices*sizeof(double));
  }

  void
  MultiKernelSolver::setH2DConstraint(int kerId, int devId, const double *coefs)
  {
    memcpy(H2DConstraints[kerId][devId], coefs, 2*nbDevices*sizeof(double));
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

      if (prev_gr[dev] + EPSILON < cur_gr[dev])
	return false;

      return true;
    }

    // Included (negative constraint, no comm observed)
    //         ----      |
    //      ----------   v
    if ( dev > 0 && dev < nbDevices-1 &&
	 cur_prefix_gr[dev] + EPSILON < prev_prefix_gr[dev] &&
	 prev_prefix_gr[dev+1] + EPSILON < cur_prefix_gr[dev+1]) {
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

    // Overlap one side
    // |-------         |
    // ###|--------     v
    if (prev_prefix_gr[dev] + EPSILON < cur_prefix_gr[dev]) {
      for (int i=0; i<=dev-1; i++) {
	(*constraint)[nbDevices+i] = 1;
	(*constraint)[i] = -1;
      }

      return true;
    }
    //    |--------  |
    // |------#####  v
    if (cur_prefix_gr[dev] + EPSILON < prev_prefix_gr[dev]) {
      for (int i=0; i<=dev; i++) {
	(*constraint)[i] = 1;
	(*constraint)[nbDevices+i] = -1;
      }

      return true;
    }


    // Overlap both sides
    // ----------
    // ###----###
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

      if (cur_gr[dev] + EPSILON < prev_gr[dev])
	return false;

      return true;
    }


    // Included
    // -------- |  (negative constraint, no com observed)
    //   ----   v
    if ( dev > 0 && dev < nbDevices-1 &&
	 prev_prefix_gr[dev] + EPSILON < cur_prefix_gr[dev] &&
	 cur_prefix_gr[dev+1] + EPSILON < prev_prefix_gr[dev+1]) {
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

    // Overlap one side
    // |-------####    |
    //    |--------    v
    if (prev_prefix_gr[dev] + EPSILON < cur_prefix_gr[dev]) {
      for (int i=0; i<=dev; i++) {
	(*constraint)[nbDevices+i] = 1;
	(*constraint)[i] = -1;
      }

      return true;
    }
    // ###|--------  |
    // |-------      v
    if (cur_prefix_gr[dev] + EPSILON < prev_prefix_gr[dev]) {
      for (int i=0; i<=dev-1; i++) {
	(*constraint)[i] = 1;
	(*constraint)[nbDevices+i] = -1;
      }

      return true;
    }


    // Overlap both sides
    // ###----###
    // ----------
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
  MultiKernelSolver::get_keri_Dj2H_rowIdx(int i, int j) const {
    return 1 + nbKernels * (2 * nbDevices + 1) + i * (2 * nbDevices) + j;
  }

  int
  MultiKernelSolver::get_keri_H2Dj_rowIdx(int i, int j) const {
    return 1 + nbKernels * (2 * nbDevices + 1) + i * (2 * nbDevices) +
      nbDevices + j;
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

    ia = new int[1+1000]();
    ja = new int[1+1000]();
    ar = new double[1+1000]();
    lp = glp_create_prob();
    glp_term_out(GLP_OFF);

    glp_set_prob_name(lp, "kernel granularity");
    glp_set_obj_dir(lp, GLP_MIN);

    nbRows = nbKernels * (2*nbDevices+1) + nbKernels * (2*nbDevices);
    nbCols = nbKernels*3 + nbKernels*2*nbDevices;
    int rowIdx = 1;
    int colIdx = 1;
    char *name = NULL;

    // Rows
    glp_add_rows(lp, nbRows);

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

    for (int i=0; i<nbKernels; i++) {
      // ker%d_D%dtoH
      for (int j=0; j<nbDevices; j++) {
	asprintf(&name, "ker%d_D%dtoH", i, j);
	glp_set_row_name(lp, rowIdx, name);
	free(name);
	glp_set_row_bnds(lp, rowIdx, GLP_UP, 0.0, 0.0);
	rowIdx++;
      }

      // ker%d_Hto%d
      for (int j=0; j<nbDevices; j++) {
	asprintf(&name, "ker%d_HtoD%d", i, j);
	glp_set_row_name(lp, rowIdx, name);
	free(name);
	glp_set_row_bnds(lp, rowIdx, GLP_UP, 0.0, 0.0);
	rowIdx++;
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
    memset(ia, 0, 1+1000 * sizeof(int));
    memset(ja, 0, 1+1000 * sizeof(int));
    memset(ar, 0, 1+1000 * sizeof(double));

    // Fill matrix

    int idx = 1;
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

      for (int j=0; j<nbDevices; j++) {
	// D2HConstraints
	// ex ker0_D0toH: coef x_k0d0 + coef x_k0_d1 + .... - T_D2H_keri <= 0

	// kprev coefs
	for (int k = 0; k<nbDevices; k++) {
	  if (D2HConstraints[i][j][k] == 0)
	    continue;

	  int kprev = (i + nbKernels - 1) % nbKernels;
	  ia[idx] = get_keri_Dj2H_rowIdx(i, j) ; //1 + nbKernels * (2 * nbDevices + 1) + i * 2 * nbDevices + j;
	  ja[idx] = get_gr_kidj_colIdx(kprev, k); //1 + kprev * 2 * nbDevices + nbDevices + k; // gr_kprev_dk
	  ar[idx] = D2HConstraints[i][j][k];
	  idx++;
	}
	// kpcur coefs
	for (int k = 0; k<nbDevices; k++) {
	  if (D2HConstraints[i][j][nbDevices+k] == 0)
	    continue;

	  ia[idx] = get_keri_Dj2H_rowIdx(i, j); //1 + nbKernels * (2 * nbDevices + 1) + i * 2 * nbDevices + j;
	  ja[idx] = get_gr_kidj_colIdx(i, k); //1 + i * 2 * nbDevices + nbDevices + k; // gr_ki_dk
	  ar[idx] = D2HConstraints[i][j][nbDevices+k];
	  idx++;
	}

	// T_D2H_keri
	ia[idx] = get_keri_Dj2H_rowIdx(i, j); //1 + nbKernels * (2 * nbDevices + 1) + i * 2 * nbDevices + j;
	ja[idx] = get_T_D2H_keri_colIdx(i); //1 + nbKernels * (2 * nbDevices) + i * 2;
	ar[idx] = -1.0;
	idx++;

	// H2DConstraints
	// ex ker0_HtoD0: coef x_k0d0 + coef x_k0_d1 + .... - TH2D_keri <= 0

	// kprev coefs
	for (int k = 0; k<nbDevices; k++) {
	  if (H2DConstraints[i][j][k] == 0)
	    continue;

	  int kprev = (i + nbKernels - 1) % nbKernels;
	  ia[idx] = get_keri_H2Dj_rowIdx(i, j); //1 + nbKernels * (2 * nbDevices + 1) + i * 2 * nbDevices + nbDevices + j;
	  ja[idx] = get_gr_kidj_colIdx(kprev, k);//1 + kprev * 2 * nbDevices + nbDevices + k; // gr_kprev_dk
	  ar[idx] = H2DConstraints[i][j][k];
	  idx++;
	}
	// kcur coefs
	for (int k = 0; k<nbDevices; k++) {
	  if (H2DConstraints[i][j][nbDevices+k] == 0)
	    continue;

	  ia[idx] = get_keri_H2Dj_rowIdx(i, j); //1 + nbKernels * (2 * nbDevices + 1) + i * 2 * nbDevices + nbDevices + j;
	  ja[idx] = get_gr_kidj_colIdx(i, k); //1 + i * 2 * nbDevices + nbDevices + k; // gr_ki_dk
	  ar[idx] = H2DConstraints[i][j][nbDevices+k];
	  idx++;
	}

	// T_H2D_keri
	ia[idx] = get_keri_H2Dj_rowIdx(i, j); //1 + nbKernels * (2 * nbDevices + 1) + i * 2 * nbDevices + nbDevices + j;
	ja[idx] = get_T_H2D_keri_colIdx(i); //1 + nbKernels * (2 * nbDevices) + i * 2 + 1;
	ar[idx] = -1.0;
	idx++;
      }
    }

    int nnz = idx-1;

    for (int i=1; i<=nnz; i++) {
      assert(ia[i] <= nbRows);
      assert(ja[i] <= nbCols);
    }

    glp_load_matrix(lp, nnz, ia, ja, ar);
  }

  double *
  MultiKernelSolver::getGranularities() {
    // Update problem
    updateGlpMatrix();

    // Resolve
    glp_simplex(lp, NULL);


    // Get solution
    double *ret = new double[nbKernels * nbDevices];

    for (int i=0; i<nbKernels; i++) {
      for (int j=0; j<nbDevices; j++) {
	int colIdx = 1 + i * 2 * nbDevices + nbDevices + j; // gr_ki_dj
	ret[i*nbDevices + j] = glp_get_col_prim(lp, colIdx);
      }
    }

    // Reset comm constraints
    for (int i=0; i<nbKernels; i++) {
      for (int j=0; j<nbDevices; j++) {
	memset(D2HConstraints[i][j], 0, 2*nbDevices*sizeof(double));
	memset(H2DConstraints[i][j], 0, 2*nbDevices*sizeof(double));
      }
    }

    return ret;
  }

};
