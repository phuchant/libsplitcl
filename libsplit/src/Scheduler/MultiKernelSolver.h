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

    void set_keri_from_kerj_Dk2H_coef(int i, int j, int k, double coef);
    void set_keri_from_kerj_H2Dk_coef(int i, int j, int k, double coef);
    double get_keri_from_kerj_Dk2H_coef(int i, int j, int k);
    double get_keri_from_kerj_H2Dk_coef(int i, int j, int k);


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
    double *kerikerjDk2HCoefs;
    double *kerikerjH2DkCoefs;

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
    int get_left_keri_from_kerj_Dk2H_rowIdx(int i, int j, int k) const;
    int get_right_keri_from_kerj_Dk2H_rowIdx(int i, int j, int k) const;
    int get_comm_keri_from_kerj_Dk2H_rowIdx(int i, int j, int k) const;
    int get_left_keri_from_kerj_H2Dk_rowIdx(int i, int j, int k) const;
    int get_right_keri_from_kerj_H2Dk_rowIdx(int i, int j, int k) const;
    int get_comm_keri_from_kerj_H2Dk_rowIdx(int i, int j, int k) const;
    int get_coef_keri_from_kerj_Dk2H_rowIdx(int i, int j, int k) const;
    int get_coef_keri_from_kerj_H2Dk_rowIdx(int i, int j, int k) const;


    int get_T_D2H_keri_colIdx(int i) const;
    int get_T_H2D_keri_colIdx(int i) const;
    int get_T_keri_colIdx(int i) const;
    int get_x_kidj_colIdx(int i, int j) const;
    int get_gr_kidj_colIdx(int i, int j) const;
    int get_left_kikjdk_D2H_colIdx(int i, int j, int k) const;
    int get_right_kikjdk_D2H_colIdx(int i, int j, int k) const;
    int get_comm_kikjdk_D2H_colIdx(int i, int j, int k) const;
    int get_left_kikjdk_H2D_colIdx(int i, int j, int k) const;
    int get_right_kikjdk_H2D_colIdx(int i, int j, int k) const;
    int get_comm_kikjdk_H2D_colIdx(int i, int j, int k) const;
    int get_coef_kikjdk_D2H_colIdx(int i, int j, int k) const;
    int get_coef_kikjdk_H2D_colIdx(int i, int j, int k) const;

    void createGlpProb();
    void updateGlpMatrix();

  };

};
#endif /* MULTIKERNELSOLVER_H */
