#include <stdio.h>
#include <math.h>

#define FITLIB_C
#include "fitlib.h"

#define DET(a00,a01,a02,a10,a11,a12,a20,a21,a22) \
 ( a00*a11*a22 \
 + a01*a12*a20 \
 + a02*a10*a21 \
 - a00*a12*a21 \
 - a01*a10*a22 \
 - a02*a11*a20 )

/*###########################################################################*/
/*                                                                           */
/*                  Fit d'une droite y=a+bx:                                 */
/*                                                                           */
/*                       Retour: 1: O.K                                      */
/*                               2: erreurs = 0                              */
/*                               3: determinant de la matrice = 0            */
/*                               4: carre erreur negatif                     */
/*###########################################################################*/

/* ===================================================================== */
char FitLineaire(float *a, float *erra, float *b, float *errb, float *chisq, float *x, float *y, float *sigma, char *valid, int npt) {
	double	s1, sx, sxx, sy, sxy, d, sigma2, temp;
	float  *ptx, *pty, *ptsig;
	float  ftemp;
	int i;
	
	s1 = sx = sxx = sy = sxy = d = 0.;	
	*a = *erra = *b = *errb = *chisq = 0.;
	ptx = x;
	pty = y;
	ptsig = sigma;
	for( i = 0 ; i < npt ; i++, x++, y++, sigma++ ) { 
		if(!valid[i]) continue;
		sigma2 = (double) *sigma;
		if( sigma2  == 0 ) { FitError = FitErrorList[FIT_ERR_SIGMA0]; return(0); } // return ( 2 );
		sigma2 = 1. / (sigma2*sigma2);
		s1  += sigma2;
		sx  += ((double) *x) * sigma2;
		sxx += ((double) *x) * ((double) *x) * sigma2;
		sy  += ((double) *y) * sigma2;
		sxy += ((double) *x) * ((double) *y) * sigma2;
	}	
	d = s1 * sxx - sx * sx;	  
	if( d  == 0.0 ) { FitError = FitErrorList[FIT_ERR_DETER0]; return(0); } // return ( 3 );
	*a = (float)( (sy * sxx - sx * sxy) /d );
	*b = (float)( (s1 * sxy - sx * sy ) /d );	
	temp = sxx / d;
	if( temp  <= 0 ) { FitError = FitErrorList[FIT_ERR_DETERN]; return(0); } // return ( 4 );
	*erra = (float)sqrt( temp );
	temp = s1 / d;
	if( temp  <= 0 ) { FitError = FitErrorList[FIT_ERR_DETERN]; return(0); } // return ( 4 );
	*errb = (float)sqrt( temp );
	x=ptx;
	y=pty;
	sigma=ptsig;
	for( i = 0 ; i < npt ; i++,x++,y++,sigma++ )  if(valid[i] && (*sigma != 0.0)) {
		ftemp = ( *y - *a - *b * *x ) / *sigma;
		*chisq += ftemp*ftemp; 
	}
	FitError = FitErrorList[0]; return(1);
}
/* ========================================================================= */
char FitSecondDegre(float *x, float *y, float *sigma, char *valid ,int longueur, float *a, float *b, float *c) {
/*
Codes de retour:
 1: tout va bien
-1: Longueur de tableau insuffisante (<3)
-2: Determinant 1ere matrice trop petit
*/
	int i,j;
	double sx4,sx3,sx2,sx1,sx0,syx2,syx,sy;
	double zz,det;

	*a = 10.0;
	*b = 0.0;
	*c = 1.0;
	if (longueur < 3) { FitError = FitErrorList[FIT_ERR_NBPTS]; return(0); } // return(-1);
	
/* Somme ponderee sur i des 1,x,x2,x3,x4,y,yx et yx2 */
	sx4 = sx3 = sx2 = sx1 = sx0 = syx2 = syx = sy = 0.0;
	for (i=0,j=0; i<longueur; i++) if(valid[i]) {
		zz  = 1.0 / (sigma[i] * sigma[i]); sx0 += zz; sy  += zz*y[i];
		zz *= x[i]; sx1 += zz; syx  += zz*y[i];
		zz *= x[i]; sx2 += zz; syx2 += zz*y[i];
		zz *= x[i]; sx3 += zz;
		zz *= x[i]; sx4 += zz;
		j++;
	}
	if(j < 3) { FitError = FitErrorList[FIT_ERR_NBPTS]; return(0); } // return(-1);
	det = DET(sx4,sx3,sx2,sx3,sx2,sx1,sx2,sx1,sx0);
	/* if ( fabs(det)<0.00001 ) { FitError = FitErrorList[FIT_ERR_DETERN]; return(0); } // return (-2); */
  	if (det == 0.0) { FitError = FitErrorList[FIT_ERR_DETER0]; return(0); } // return(-2);

/* y = ax2 + bx + c */
	*a   = (float)(DET(syx2,syx,sy,sx3,sx2,sx1,sx2,sx1,sx0)/det);
	*b   = (float)(DET(sx4,sx3,sx2,syx2,syx,sy,sx2,sx1,sx0)/det);
	*c   = (float)(DET(sx4,sx3,sx2,sx3,sx2,sx1,syx2,syx,sy)/det);
	FitError = FitErrorList[0]; return(1); // return(1);
}
/* ========================================================================= */
char FitParabole(float *x, float *y, float *sigma, char *valid, int longueur, float *max, float *k, float *x0)
{
/*
Codes de retour:
 1: tout va bien
-1: Longueur de tableau insuffisante (<3)
-2: Determinant 1ere matrice trop petit
*/
	int i,j;
	double sx4,sx3,sx2,sx1,sx0,syx2,syx,sy;
	double zz,a,b,c,det;

	*k = 10.0;
	*x0 = 0.0;
	*max = 1.0;
	if (longueur < 3) { FitError = FitErrorList[FIT_ERR_NBPTS]; return(0); } // return(-1);
	
/* Somme ponderee sur i des 1,x,x2,x3,x4,y,yx et yx2 */
	sx4 = sx3 = sx2 = sx1 = sx0 = syx2 = syx = sy = 0.0;
	for (i=0,j=0; i<longueur; i++) if(valid[i]) {
		zz  = 1.0 / (sigma[i] * sigma[i]); sx0 += zz; sy  += zz*y[i];
		zz *= x[i]; sx1 += zz; syx  += zz*y[i];
		zz *= x[i]; sx2 += zz; syx2 += zz*y[i];
		zz *= x[i]; sx3 += zz;
		zz *= x[i]; sx4 += zz;
		j++;
	}
	if(j < 3) { FitError = FitErrorList[FIT_ERR_NBPTS]; return(0); } // return(-1);
	det = DET(sx4,sx3,sx2,sx3,sx2,sx1,sx2,sx1,sx0);
	/* if ( fabs(det)<0.00001 ) { FitError = FitErrorList[FIT_ERR_DETERN]; return(0); } // return (-2); */
  	if (det == 0.0) { FitError = FitErrorList[FIT_ERR_DETER0]; return(0); } // return (-2);

/* y = ax2 + bx + c */
	a   = DET(syx2,syx,sy,sx3,sx2,sx1,sx2,sx1,sx0)/det;
	b   = DET(sx4,sx3,sx2,syx2,syx,sy,sx2,sx1,sx0)/det;
	c   = DET(sx4,sx3,sx2,sx3,sx2,sx1,syx2,syx,sy)/det;
	if(a == 0.0) { FitError = FitErrorList[FIT_ERR_DETER0]; return(0); } // return(-3);

/* soit y = k(x-x0)**2 + max */
	*k = (float)a;                               /* "amplitude"          */
	*x0 = (float)(-b / (2.0 * a));               /* position x du sommet */
	*max = (float)(c - ((b * b) / (4.0 * a)));   /* position y du sommet */
	FitError = FitErrorList[0]; return(1); // return(1);
}
/* ========================================================================= */
char FitGaussienne(float *x, float *y, char *valid, int longueur,  float fond, 
				  float *amp, float *sigma, float *x0) {
/*
Codes de retour:
 1: tout va bien
-1: Longueur de tableau insuffisante (<3)
-2: Determinant 1ere matrice trop petit
-3: Determinant 2eme matrice (-sigma2) trop petit ou positif

Par rapport a la version EROS:
int FitGaussienne(y, poids, longueur, poidsfond, fond, amp, sigma, x0)
	double *y, *poids;
	int longueur, poidsfond;  double *fond, *amp, *sigma, *x0;
	Return Code: 0 = OK , 2 = Erreur malloc , 1 = Echec du fit

on impose ici poidsfond=0, fond pas modifie, poids=1 sauf si !valide,
ni x[] ni ylog[] ne sont alloues, on passe des float et non des double
*/
	int i,j;
	double sx4,sx3,sx2,sx1,sx0,syx2,syx,sy;
	double zz,ylog,a,b,c,det;

/* A tout hasard */
	*sigma = 10.0;
	*x0 = 0.0;
	*amp = 1.0;
	if (longueur < 3) { FitError = FitErrorList[FIT_ERR_NBPTS]; return(0); } // return(-1);

/* Somme ponderee sur i des 1,x,x2,x3,x4,y,yx et yx2 */
	sx4 = sx3 = sx2 = sx1 = sx0 = syx2 = syx = sy = 0.0;
	for (i=0,j=0; i<longueur; i++) if(valid[i]) {
		if (y[i] <= fond) continue;
		ylog= log((double)(y[i] - fond));
		zz = 1.0;   sx0 += zz; sy   += zz*ylog;
		zz *= x[i]; sx1 += zz; syx  += zz*ylog;
		zz *= x[i]; sx2 += zz; syx2 += zz*ylog;
		zz *= x[i]; sx3 += zz; 
		zz *= x[i]; sx4 += zz;
		j++;
	}
//	printf("(%s) %d valeurs au dessus de %g\n",__func__,j,fond);
	if(j < 3) { FitError = FitErrorList[FIT_ERR_NBPTS]; return(0); } // return(-1);
	det = DET(sx4,sx3,sx2,sx3,sx2,sx1,sx2,sx1,sx0);
//	printf("(%s) Determinant = %g\n",__func__,det);
	/* if ( fabs(det)<0.00001 ) { FitError = FitErrorList[FIT_ERR_DETERN]; return(0); } // return (-2); */
	if(det == 0.0) { FitError = FitErrorList[FIT_ERR_DETER0]; return(0); } // return (-2);

/* Log(y-fond) = ax2 + bx + c */
	a   = DET(syx2,syx,sy,sx3,sx2,sx1,sx2,sx1,sx0)/det;
	b   = DET(sx4,sx3,sx2,syx2,syx,sy,sx2,sx1,sx0)/det;
	c   = DET(sx4,sx3,sx2,sx3,sx2,sx1,syx2,syx,sy)/det;
	/* if(a > -0.001) { FitError = FitErrorList[FIT_ERR_DETERN]; return(0); } // return(-3); */
//	printf("(%s) Coefficient 2nd degre = %g\n",__func__,a);
	if(a >= 0.0) { FitError = FitErrorList[FIT_ERR_DETERP]; return(0); } // return(-3);

//	printf("(%s) Polynome: %g X2   %+g X   %+g\n",__func__,a,b,c);
/* soit y = amp * exp(-(x-x0)**2 / 2*sigma**2) + fond */
	*sigma = (float)(1.0 / sqrt(-2.0 * a));
	*x0 = (float)(-b / (2.0 * a));
	*amp = (float)exp(c - ((b*b) / (4.0 * a)));
	FitError = FitErrorList[0]; return(1); // return(1);
}
#ifdef ALEXEI
/* ===================================================================== */
// Function counting extremum of massive yvalue using cubic spline interpolations
double Interpolate(int jleft, int jright, double *y, int lowborder, int upperborder, int precision) {
	// jleft, jright - values og intervals of interpolations, 
	// y value to interpolate, lowborder, upperborder -intervals of
	// finding maximum value from central point. Central point corespond to pre-maximum  
	// 
	int n = jleft+jright; // number of point to interpolate
	double *x; // creating massive x for the general perposis
	double x2[precision*n]; // same
	double xvalue; 
	double yval = 0.0; // Output spline values
	int i;
	int npoint = n*precision;
	int ilow = ((n-jright)-lowborder)*precision;
	int iupper = ((n-jright)+upperborder)*precision;
	
	x = x2;
	for (i =0; i<n; i++) // Then x value is not nessesary to know: uniformy *y
		*(x+i) = i; 
	
	for (i =ilow; i<iupper; i++) { 
		xvalue = ((float)i)/(float)precision; // i/precision
		
		// Initialisation and initial settings 
		int ibcbeg,ibcend;
		double ybcbeg,ybcend;
		
		ibcbeg = 0;
		ybcbeg = 0.0;
		ibcend = 0;
		ybcend = 0.0;
		// Set up and computes second derivatives
		double *ypp;
		
		
		ypp = spline_cubic_set (n, x, y, ibcbeg, ybcbeg, ibcend, ybcend, precision);
		
		
		double yppval; // Output derivatives
		double ypval; 
		double tyval;
		
		tyval = spline_cubic_val (n, x, xvalue, y, ypp, &ypval, &yppval);
		
		
		yval = (fabs(yval) > fabs(tyval))? yval: tyval; // Find extremum
		
	}
	//printf("å¤å¤ Spline value( %f) = %f \n",xvalue,yval); 
	return yval;
} 
//**********************************************************************

double *spline_cubic_set ( int n, double *t, double *y, int ibcbeg, double ybcbeg, 
  int ibcend, double ybcend , int precision)
{
//**********************************************************************
//
//  Purpose:
//
//    SPLINE_CUBIC_SET computes the second derivatives of a piecewise cubic spline.
//
//  Discussion:
//
//    For data interpolation, the user must call SPLINE_SET to determine
//    the second derivative data, passing in the data to be interpolated,
//    and the desired boundary conditions.
//
//    The data to be interpolated, plus the SPLINE_SET output, defines
//    the spline.  The user may then call SPLINE_VAL to evaluate the
//    spline at any point.
//
//    The cubic spline is a piecewise cubic polynomial.  The intervals
//    are determined by the "knots" or abscissas of the data to be
//    interpolated.  The cubic spline has continous first and second
//    derivatives over the entire interval of interpolation.
//
//    For any point T in the interval T(IVAL), T(IVAL+1), the form of
//    the spline is
//
//      SPL(T) = A(IVAL)
//             + B(IVAL) * ( T - T(IVAL) )
//             + C(IVAL) * ( T - T(IVAL) )**2
//             + D(IVAL) * ( T - T(IVAL) )**3
//
//    If we assume that we know the values Y(*) and YPP(*), which represent
//    the values and second derivatives of the spline at each knot, then
//    the coefficients can be computed as:
//
//      A(IVAL) = Y(IVAL)
//      B(IVAL) = ( Y(IVAL+1) - Y(IVAL) ) / ( T(IVAL+1) - T(IVAL) )
//        - ( YPP(IVAL+1) + 2 * YPP(IVAL) ) * ( T(IVAL+1) - T(IVAL) ) / 6
//      C(IVAL) = YPP(IVAL) / 2
//      D(IVAL) = ( YPP(IVAL+1) - YPP(IVAL) ) / ( 6 * ( T(IVAL+1) - T(IVAL) ) )
//
//    Since the first derivative of the spline is
//
//      SPL'(T) =     B(IVAL)
//              + 2 * C(IVAL) * ( T - T(IVAL) )
//              + 3 * D(IVAL) * ( T - T(IVAL) )**2,
//
//    the requirement that the first derivative be continuous at interior
//    knot I results in a total of N-2 equations, of the form:
//
//      B(IVAL-1) + 2 C(IVAL-1) * (T(IVAL)-T(IVAL-1))
//      + 3 * D(IVAL-1) * (T(IVAL) - T(IVAL-1))**2 = B(IVAL)
//
//    or, setting H(IVAL) = T(IVAL+1) - T(IVAL)
//
//      ( Y(IVAL) - Y(IVAL-1) ) / H(IVAL-1)
//      - ( YPP(IVAL) + 2 * YPP(IVAL-1) ) * H(IVAL-1) / 6
//      + YPP(IVAL-1) * H(IVAL-1)
//      + ( YPP(IVAL) - YPP(IVAL-1) ) * H(IVAL-1) / 2
//      =
//      ( Y(IVAL+1) - Y(IVAL) ) / H(IVAL)
//      - ( YPP(IVAL+1) + 2 * YPP(IVAL) ) * H(IVAL) / 6
//
//    or
//
//      YPP(IVAL-1) * H(IVAL-1) + 2 * YPP(IVAL) * ( H(IVAL-1) + H(IVAL) )
//      + YPP(IVAL) * H(IVAL)
//      =
//      6 * ( Y(IVAL+1) - Y(IVAL) ) / H(IVAL)
//      - 6 * ( Y(IVAL) - Y(IVAL-1) ) / H(IVAL-1)
//
//    Boundary conditions must be applied at the first and last knots.  
//    The resulting tridiagonal system can be solved for the YPP values.
//
//  Modified:
//
//    06 February 2004
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, int N, the number of data points.  N must be at least 2.
//    In the special case where N = 2 and IBCBEG = IBCEND = 0, the
//    spline will actually be linear.
//
//    Input, double T[N], the knot values, that is, the points were data is
//    specified.  The knot values should be distinct, and increasing.
//
//    Input, double Y[N], the data values to be interpolated.
//
//    Input, int IBCBEG, left boundary condition flag:
//      0: the cubic spline should be a quadratic over the first interval;
//      1: the first derivative at the left endpoint should be YBCBEG;
//      2: the second derivative at the left endpoint should be YBCBEG.
//
//    Input, double YBCBEG, the values to be used in the boundary
//    conditions if IBCBEG is equal to 1 or 2.
//
//    Input, int IBCEND, right boundary condition flag:
//      0: the cubic spline should be a quadratic over the last interval;
//      1: the first derivative at the right endpoint should be YBCEND;
//      2: the second derivative at the right endpoint should be YBCEND.
//
//    Input, double YBCEND, the values to be used in the boundary
//    conditions if IBCEND is equal to 1 or 2.
//
//    Output, double SPLINE_CUBIC_SET[N], the second derivatives of the cubic spline.
//
//  double *a;
//  double *b;
	double tval;
	int i;
	double *ypp;
	//
	//  Check.
	//
	
	if ( n <= 1 )
		{
		printf("\n");
		printf("SPLINE_CUBIC_SET - Fatal error!\n");
		printf("  The number of data points N must be at least 2.\n");
		printf("  The input value is  %d .\n",n);
		return NULL;
		}
	
	for ( i = 0; i < n - 1; i++ )
		{
		if ( *(t+i+1) <= *(t+i) )
			{
			printf("\n");
			printf("SPLINE_CUBIC_SET - Fatal error!\n");
			printf("  The knots must be strictly increasing, but\n");
			printf("  T( %d ) = %f  \n",i,*(t+i));
			printf("  T( %d ) = %f  \n",i+1,*(t+i+1));
			return NULL;
			}
		}
	
	
	double a[3*precision*n];
	double b[precision*n];
	
	
	//
	//  Set up the first equation.
	//
	if ( ibcbeg == 0 )
		{
		b[0] = 0.0;
		*(a+1+0*3) = 1.0;
		*(a+0+1*3) = -1.0;
		}
	else if ( ibcbeg == 1 )
		{
		b[0] = ( *(y+1) - *y ) / ( *(t+1) - *t ) - ybcbeg;
		a[1+0*3] = ( *(t+1) - *t) / 3.0;
		a[0+1*3] = ( *(t+1) - *t) / 6.0;
		}
	else if ( ibcbeg == 2 )
		{
		b[0] = ybcbeg;
		a[1+0*3] = 1.0;
		a[0+1*3] = 0.0;
		}
	else
		{
		printf("\n");
		printf("SPLINE_CUBIC_SET - Fatal error!\n");
		printf("  IBCBEG must be 0, 1 or 2.\n");
		printf("  The input value is %d .\n",ibcbeg);
		//    delete [] a;
		//    delete [] b;
		free(a);
		free(b);
		return NULL;
		}
	//
	//  Set up the intermediate equations.
	//
	
	for ( i = 1; i < n-1; i++ )
		{
		b[i] = (*(y+i+1) - *(y+i) ) / ( *(t+i+1) - *(t+i) )
		- (*(y+i) - *(y+i-1) ) / ( *(t+i) - *(t+i-1) );
		a[2+(i-1)*3] = (*(t+i) - *(t+i-1) ) / 6.0;
		a[1+ i   *3] = (*(t+i+1) - *(t+i-1) ) / 3.0;
		a[0+(i+1)*3] = (*(t+i+1) - *(t+i) ) / 6.0;
		}
	//
	//  Set up the last equation.
	//
	
	if ( ibcend == 0 )
		{
		b[n-1] = 0.0;
		a[2+(n-2)*3] = -1.0;
		a[1+(n-1)*3] = 1.0;
		}
	else if ( ibcend == 1 )
		{
		b[n-1] = ybcend - (*(y+n-1) - *(y+n-2)) / (*(t+n-1) - *(t+n-2));
		a[2+(n-2)*3] = (*(t+n-1) - *(t+n-2)) / 6.0;
		a[1+(n-1)*3] = (*(t+n-1) - *(t+n-2)) / 3.0;
		}
	else if ( ibcend == 2 )
		{
		b[n-1] = ybcend;
		a[2+(n-2)*3] = 0.0;
		a[1+(n-1)*3] = 1.0;
		}
	else
		{
		printf("\n");
		printf("SPLINE_CUBIC_SET - Fatal error!\n");
		printf("  IBCEND must be 0, 1 or 2.\n");
		printf("  The input value is %d.\n",ibcend);
		free(a);
		free(b);
		//    delete [] a;
		//    delete [] b;
		return NULL;
		}
	//
	//  Solve the linear system.
	//
	
	if ( n == 2 && ibcbeg == 0 && ibcend == 0 )
		{
		//    ypp = new double[2];
		
		ypp[0] = 0.0;
		ypp[1] = 0.0;
		}
	else
		{
		
		ypp = d3_np_fs( n, a, b, precision);
		if ( !ypp )
			{
			printf("\n");
			printf("SPLINE_CUBIC_SET - Fatal error!\n");
			printf("  The linear system could not be solved.\n");
			//      delete [] a;
			//      delete [] b;
			free(a);
			free(b);
			return NULL;
			}
		
		}
	
	//  delete [] a;
	//  delete [] b;
	
    return ypp;
}
//**********************************************************************

double spline_cubic_val ( int n, double *t, double tval, double *y, 
  double *ypp, double *ypval, double *yppval )
{
//**********************************************************************
//
//  Purpose:
//
//    SPLINE_CUBIC_VAL evaluates a piecewise cubic spline at a point.
//
//  Discussion:
//
//    SPLINE_CUBIC_SET must have already been called to define the values of YPP.
//
//    For any point T in the interval T(IVAL), T(IVAL+1), the form of
//    the spline is
//
//      SPL(T) = A
//             + B * ( T - T(IVAL) )
//             + C * ( T - T(IVAL) )**2
//             + D * ( T - T(IVAL) )**3
//
//    Here:
//      A = Y(IVAL)
//      B = ( Y(IVAL+1) - Y(IVAL) ) / ( T(IVAL+1) - T(IVAL) )
//        - ( YPP(IVAL+1) + 2 * YPP(IVAL) ) * ( T(IVAL+1) - T(IVAL) ) / 6
//      C = YPP(IVAL) / 2
//      D = ( YPP(IVAL+1) - YPP(IVAL) ) / ( 6 * ( T(IVAL+1) - T(IVAL) ) )
//
//  Modified:
//
//    04 February 1999
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, int N, the number of knots.
//
//    Input, double Y[N], the data values at the knots.
//
//    Input, double T[N], the knot values.
//
//    Input, double TVAL, a point, typically between T[0] and T[N-1], at
//    which the spline is to be evalulated.  If TVAL lies outside
//    this range, extrapolation is used.
//
//    Input, double Y[N], the data values at the knots.
//
//    Input, double YPP[N], the second derivatives of the spline at
//    the knots.
//
//    Output, double *YPVAL, the derivative of the spline at TVAL.
//
//    Output, double *YPPVAL, the second derivative of the spline at TVAL.
//
//    Output, double SPLINE_VAL, the value of the spline at TVAL.
//

  double dt;
  double h;
  int i;
  int ival;
  double yval;
//
//  Determine the interval [ T(I), T(I+1) ] that contains TVAL.
//  Values below T[0] or above T[N-1] use extrapolation.
//
  ival = n - 2;

  for ( i = 0; i < n-1; i++ )
  {
    if ( tval < t[i+1] )
    {
      ival = i;
      break;
    }
  }
//
//  In the interval I, the polynomial is in terms of a normalized
//  coordinate between 0 and 1.
//
  dt = tval - t[ival];
  h = t[ival+1] - t[ival];

  yval = y[ival]
    + dt * ( ( y[ival+1] - y[ival] ) / h
           - ( ypp[ival+1] / 6.0 + ypp[ival] / 3.0 ) * h
    + dt * ( 0.5 * ypp[ival]
    + dt * ( ( ypp[ival+1] - ypp[ival] ) / ( 6.0 * h ) ) ) );

  *ypval = ( y[ival+1] - y[ival] ) / h
    - ( ypp[ival+1] / 6.0 + ypp[ival] / 3.0 ) * h
    + dt * ( ypp[ival]
    + dt * ( 0.5 * ( ypp[ival+1] - ypp[ival] ) / h ) );

  *yppval = ypp[ival] + dt * ( ypp[ival+1] - ypp[ival] ) / h;

  return yval;
}
//**********************************************************************

double *d3_np_fs( int n, double *a, double *b, int precision)
{
//**********************************************************************
//
//  Purpose:
//
//    D3_NP_FS factors and solves a D3 system.
//
//  Discussion:
//
//    The D3 storage format is used for a tridiagonal matrix.
//    The superdiagonal is stored in entries (1,2:N), the diagonal in
//    entries (2,1:N), and the subdiagonal in (3,1:N-1).  Thus, the
//    original matrix is "collapsed" vertically into the array.
//
//    This algorithm requires that each diagonal entry be nonzero.
//    It does not use pivoting, and so can fail on systems that
//    are actually nonsingular.
//
//  Example:
//
//    Here is how a D3 matrix of order 5 would be stored:
//
//       *  A12 A23 A34 A45
//      A11 A22 A33 A44 A55
//      A21 A32 A43 A54  *
//
//  Modified:
//
//    15 November 2003
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, int N, the order of the linear system.
//
//    Input/output, double A[3*N].
//    On input, the nonzero diagonals of the linear system.
//    On output, the data in these vectors has been overwritten
//    by factorization information.
//
//    Input, double B[N], the right hand side.
//
//    Output, double D3_NP_FS[N], the solution of the linear system.
//    This is NULL if there was an error because one of the diagonal
//    entries was zero.
//
  int i;
  double *xval;
  double xmult;
//
//  Check.
//
  for ( i = 0; i < n; i++ )
  {
    if ( a[1+i*3] == 0.0 )
    {
      return NULL;
    }
  }
  double x[precision*n];


  for ( i = 0; i < n; i++ )
  {
    x[i] = b[i];
  }

  for ( i = 1; i < n; i++ )
  {
    xmult = a[2+(i-1)*3] / a[1+(i-1)*3];
    a[1+i*3] = a[1+i*3] - xmult * a[0+i*3];
    x[i] = x[i] - xmult * x[i-1];
  }

  x[n-1] = x[n-1] / a[1+(n-1)*3];
  for ( i = n-2; 0 <= i; i-- )
  {
    x[i] = ( x[i] - a[0+(i+1)*3] * x[i+1] ) / a[1+i*3];
  }
  xval = x;
  return xval;
}
//******************************************************************************
//********************************************************************

void dvec_bracket3 ( int n, double t[], double tval, int *left )
{
//********************************************************************
//
//  Purpose:
//
//    DVEC_BRACKET3 finds the interval containing or nearest a given value.
//
//  Discussion:
//
//    The routine always returns the index LEFT of the sorted array
//    T with the property that either
//    *  T is contained in the interval [ T[LEFT], T[LEFT+1] ], or
//    *  T < T[LEFT] = T[0], or
//    *  T > T[LEFT+1] = T[N-1].
//
//    The routine is useful for interpolation problems, where
//    the abscissa must be located within an interval of data
//    abscissas for interpolation, or the "nearest" interval
//    to the (extreme) abscissa must be found so that extrapolation
//    can be carried out.
//
//    For consistency with other versions of this routine, the
//    value of LEFT is assumed to be a 1-based index.  This is
//    contrary to the typical C and C++ convention of 0-based indices.
//
//  Modified:
//
//    24 February 2004
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, int N, length of the input array.
//
//    Input, double T[N], an array that has been sorted into ascending order.
//
//    Input, double TVAL, a value to be bracketed by entries of T.
//
//    Input/output, int *LEFT.
//
//    On input, if 1 <= LEFT <= N-1, LEFT is taken as a suggestion for the
//    interval [ T[LEFT-1] T[LEFT] ] in which TVAL lies.  This interval
//    is searched first, followed by the appropriate interval to the left
//    or right.  After that, a binary search is used.
//
//    On output, LEFT is set so that the interval [ T[LEFT-1], T[LEFT] ]
//    is the closest to TVAL; it either contains TVAL, or else TVAL
//    lies outside the interval [ T[0], T[N-1] ].
//

  int high;
  int low;
  int mid;
//  
//  Check the input data.
//
  if ( n < 2 ) 
  {
    printf("\n");
    printf("DVEC_BRACKET3 - Fatal error!\n");
    printf("  N must be at least 2.\n");
    exit ( 1 );
  }
//
//  If *LEFT is not between 1 and N-1, set it to the middle value.
//
  if ( *left < 1 || n - 1 < *left ) 
  {
    *left = ( n + 1 ) / 2;
  }

//
//  CASE 1: TVAL < T[*LEFT]:
//  Search for TVAL in (T[I],T[I+1]), for I = 1 to *LEFT-1.
//
  if ( tval < t[*left] ) 
  {

    if ( *left == 1 ) 
    {
      return;
    }
    else if ( *left == 2 ) 
    {
      *left = 1;
      return;
    }
    else if ( t[*left-2] <= tval )
    {
      *left = *left - 1;
      return;
    }
    else if ( tval <= t[1] ) 
    {
      *left = 1;
      return;
    }
// 
//  ...Binary search for TVAL in (T[I-1],T[I]), for I = 2 to *LEFT-2.
//
    low = 2;
    high = *left - 2;

    for (;;)
    {

      if ( low == high )
      {
        *left = low;
        return;
      }

      mid = ( low + high + 1 ) / 2;

      if ( t[mid-1] <= tval ) 
      {
        low = mid;
      }
      else 
      {
        high = mid - 1;
      }

    }
  }
// 
//  CASE 2: T[*LEFT] < TVAL:
//  Search for TVAL in (T[I-1],T[I]) for intervals I = *LEFT+1 to N-1.
//
  else if ( t[*left] < tval ) 
  {

    if ( *left == n - 1 ) 
    {
      return;
    }
    else if ( *left == n - 2 ) 
    {
      *left = *left + 1;
      return;
    }
    else if ( tval <= t[*left+1] )
    {
      *left = *left + 1;
      return;
    }
    else if ( t[n-2] <= tval ) 
    {
      *left = n - 1;
      return;
    }
// 
//  ...Binary search for TVAL in (T[I-1],T[I]) for intervals I = *LEFT+2 to N-2.
//
    low = *left + 2;
    high = n - 2;

    for ( ; ; ) 
    {

      if ( low == high ) 
      {
        *left = low;
        return;
      }

      mid = ( low + high + 1 ) / 2;

      if ( t[mid-1] <= tval ) 
      {
        low = mid;
      }
      else 
      {
        high = mid - 1;
      }
    }
  }
//
//  CASE 3: T[*LEFT-1] <= TVAL <= T[*LEFT]:
//  T is just where the user said it might be.
//
  else 
  {
  }

  return;
}
#endif
