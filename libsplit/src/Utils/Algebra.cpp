#include <Utils/Algebra.h>

#include <iostream>
#include <cmath>

namespace libsplit {

  double dot(const double *x, const double*y, int N)
  {
    double res = 0;
    for (int i=0; i<N; ++i)
      res += x[i] * y[i];

    return res;
  }

  double norm(const double *x, int N)
  {
    double res = 0;
    for (int i=0; i<N; i++)
      res += x[i] * x[i];
    return sqrt(res);
  }

  // y = alpha * x + y
  void axpy(const double alpha, const double *x, double *y, int N)
  {
    for (int i=0; i<N; i++)
      y[i] += alpha * x[i];
  }

  // c = alpha * x + y
  void caxpy(const double alpha, const double *x, const double *y,
	     double *c, int N)
  {
    for (int i=0; i<N; i++)
      c[i] = alpha * x[i] + y[i];
  }

  // X = alpha * X
  void scal(const double alpha, double *x, int N)
  {
    for (int i=0; i<N;i++)

      x[i] *= alpha;
  }

  // Cmp = Amn * Bnp
  void gemm(int m, int n, int p, const double *A, const double *B,
	    double *C)
  {
    for (int i=0; i<m; ++i) {
      for (int j=0; j<p; j++) {
	C[i*p + j] = 0;

	for (int k=0; k<n; k++) {
	  C[i*p + j] += A[i*n + k] * B[k*p + j];
	}
      }
    }
  }

  double *getIdMatrix(int n)
  {
    double *res = new double[n*n*sizeof(double)];
    for (int i=0; i<n; ++i) {
      for (int j=0; j<n; ++j) {
	if (i == j)
	  res[i*n+j] = -1;
	else
	  res[i*n+j] = 0;
      }
    }

    return res;
  }

  void printMatrix(const double *A, int m, int n)
  {
    for (int i=0; i<m; i++) {
      for (int j=0; j<n; j++) {
	std::cerr << A[i*n + j] << " ";
      }
      std::cerr << "\n";
    }
  }

};
