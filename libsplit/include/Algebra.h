#ifndef ALGEBRA_H
#define ALGEBRA_H


double dot(const double *x, const double*y, int N);
double norm(const double *x, int N);

// y = alpha * x + y
void axpy(const double alpha, const double *x, double *y, int N);

// c = alpha * x + y
void caxpy(const double alpha, const double *x, const double *y,
	   double *c, int N);

// X = alpha * X
void scal(const double alpha, double *x, int N);

// Cmp = Amn * Bnp
void gemm(int m, int n, int p, const double *A, const double *B,
	  double *C);

double *getIdMatrix(int n);

void printMatrix(const double *A, int m, int n);

#endif /* ALGEBRA_H */
