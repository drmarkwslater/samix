#ifndef FITLIB_H
#define FITLIB_H

#ifdef FITLIB_C
	#define FITLIB_VAR
#else
	#define FITLIB_VAR extern
#endif

/* ===================================================================== */
typedef enum {
	FIT_ERR_SIGMA0 = 1,
	FIT_ERR_NBPTS,
	FIT_ERR_DETERN,
	FIT_ERR_DETER0,
	FIT_ERR_DETERP,
	FIT_ERRORNB
} FIT_ERROR;

FITLIB_VAR char *FitErrorList[FIT_ERRORNB+1]
#ifdef FITLIB_C
 = { "Pas d'erreur", "Sigma nul", "Nb points < 3",
     "Determinant negatif", "Determinant nul", "Determinant positif", "\0" }
#endif
;

FITLIB_VAR char *FitError
#ifdef FITLIB_C
 = "Pas d'erreur"
#endif
;

/* ===================================================================== */

char FitLineaire(float *a, float *erra, float *b, float *errb, float *chisq, float *x, float *y, float *sigma, char *valid, int npt);
char FitSecondDegre(float *x, float *y, float *sigma, char *valid, int longueur,
	float *a, float *b, float *c);
char FitParabole(float *x, float *y, float *sigma, char *valid, int longueur,
	float *C, float *k, float *x0);
char FitGaussienne(float *x, float *y, char *valid, int longueur,  float fond,
	float *amp, float *sigma, float *x0);

#ifdef ALEXEI
double Interpolate(int jleft, int jright, double *y, int lowborder, int upperborder, int precision);
double *spline_cubic_set ( int n, double *t, double *y, int ibcbeg, double ybcbeg, 
  int ibcend, double ybcend, int precision );
double spline_cubic_val ( int n, double *t, double tval, double *y, double *ypp,
  double *ypval, double *yppval );
double *d3_np_fs( int n, double *a, double *b, int precision);
void dvec_bracket3 ( int n, double t[], double tval, int *left );
#endif

#endif

