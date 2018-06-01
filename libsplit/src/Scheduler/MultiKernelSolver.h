#ifndef MULTIKERNELSOLVER_H
#define MULTIKERNELSOLVER_H

extern "C" {
#include <glpk.h>
}

namespace libsplit {

  class MultiKernelSolver {
  public:
    MultiKernelSolver(int nbDevices, int nbKernels);
    ~MultiKernelSolver();

    void setKernelGr(int kerId, const double *gr);
    void setKernelPerf(int kerId, const double *perf);
    void setD2HConstraint(int kerId, int devId, const double *coefs);
    void setH2DConstraint(int kerId, int devId, const double *coefs);
    void setKernelsD2HConstraint(int kerId, int src, int devId,
				 const double *coefs);
    void setKernelsH2DConstraint(int kerId, int src, int devId,
				 const double *coefs);

    // Compute the D2H constraint for device dev.
    // Returns true if the point Di2H_time / granu_diff should be taken into
    // account for the linear regression and false otherwise.
    bool getD2HConstraint(int dev,
			 const double *prev_gr, const double *cur_gr,
			 const double *prev_prefix_gr,
			 const double *cur_prefix_gr, int **constraint);

    // Compute the H2D constraint for device dev.
    // Returns true if the point H2Di_time / granu_diff should be taken into
    // account for the linear regression and false otherwise.
    bool getH2DConstraint(int dev,
			  const double *prev_gr, const double *cur_gr,
			  const double *prev_prefix_gr,
			  const double *cur_prefix_gr, int **constraint);


    // Return the new partition for the cycle or NULL if the partition remains
    // unchanged.
    double *getGranularities();

    void dumpProb();

  private:
    int nbDevices;
    int nbKernels;
    double **kernelGr;
    double **kernelPerf;
    double ****kernelsD2HConstraints;
    double ****kernelsH2DConstraints;

    // GLPK
    glp_prob *lp;
    int *ia, *ja;
    double *ar;
    int nbRows;
    int nbCols;

    double *lastPartition;

    int get_keri_devj_rowIdx(int i, int j) const;
    int get_gr_keri_devj_rowIdx(int i, int j) const;
    int get_gr_keri_rowIdx(int i) const;

    int get_keri_from_kerj_Dk2H_rowIdx(int i, int j, int k) const;
    int get_keri_from_kerj_H2Dk_rowIdx(int i, int j, int k) const;

    int get_T_D2H_keri_colIdx(int i) const;
    int get_T_H2D_keri_colIdx(int i) const;
    int get_T_keri_colIdx(int i) const;
    int get_x_kidj_colIdx(int i, int j) const;
    int get_gr_kidj_colIdx(int i, int j) const;


    void createGlpProb();
    void updateGlpMatrix();

  };

};
#endif /* MULTIKERNELSOLVER_H */
