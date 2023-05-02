/* ellf.c
 *
 * Read ellf.doc before attempting to compile this program.
 */


#include <stdio.h>
#include <stdlib.h>
/* size of arrays: */
#define ARRSIZ 50


/* System configurations */
#define FLTR_C
#include "mconf.h"


//extern double PI, PIO2, MACHEP, MAXNUM;
extern double PI,PIO2, MACHEP, MAXNUM;

static double aa[ARRSIZ];
static double pp[ARRSIZ];
static double y[ARRSIZ];
static double zs[ARRSIZ];
cmplx z[ARRSIZ];
static double wr = 0.0;
static double cbp = 0.0;
static double wc = 0.0;
static double rn = 8.0;
static double c = 0.0;
static double cgam = 0.0;
static double scale = 0.0;
static double fs = 1.0e4;
static double dbr = 0.5;
static double dbd = -40.0;
static double f1 = 1.5e3;
static double f2 = 2.0e3;
static double f3 = 2.4e3;
double dbfac = 0.0;
static double a = 0.0;
static double b = 0.0;
static double q = 0.0;
static double r = 0.0;
static double u = 0.0;
static double k = 0.0;
static double m = 0.0;
static double Kk = 0.0;
static double Kk1 = 0.0;
static double Kpk = 0.0;
static double Kpk1 = 0.0;
static double eps = 0.0;
static double rho = 0.0;
static double phi = 0.0;
static double sn = 0.0;
static double cn = 0.0;
static double dn = 0.0;
static double sn1 = 0.0;
static double cn1 = 0.0;
static double dn1 = 0.0;
static double phi1 = 0.0;
static double m1 = 0.0;
static double m1p = 0.0;
static double cang = 0.0;
static double sang = 0.0;
static double bw = 0.0;
static double ang = 0.0;
double fnyq = 0.0;
static double ai = 0.0;
static double pn = 0.0;
static double an = 0.0;
static double gam = 0.0;
static double cng = 0.0;
double gain = 0.0;
static int lr = 0;
static int nt = 0;
static int i = 0;
static int j = 0;
static int jt = 0;
static int nc = 0;
static int ii = 0;
static int ir = 0;
int zord = 0;
static int icnt = 0;
static int mh = 0;
static int jj = 0;
static int jh = 0;
static int jl = 0;
static int n = 8;
static int np = 0;
static int nz = 0;
static int type = 1;
static int kind = 1;

//static char wkind[] =
//{"Filter kind:\n1 Butterworth\n2 Chebyshev\n3 Elliptic\n"};

//static char salut[] =
//{"Filter shape:\n1 low pass\n2 band pass\n3 high pass\n4 band stop\n"};

#define cabs cabs_mig
double cabs_mig();

#ifdef ANSIPROT
extern double exp ( double );
extern double log ( double );
extern double cos ( double );
extern double sin ( double );
extern double sqrt ( double );
extern double fabs ( double );
extern double asin ( double );
extern double atan ( double );
extern double atan2 ( double, double );
extern double pow ( double, double );
// extern double cabs ( cmplx *z );
extern void cadd ( cmplx *a, cmplx *b, cmplx *c );
extern void cdiv ( cmplx *a, cmplx *b, cmplx *c );
// extern void cmov ( void *a, void *b );
extern void cmov ( cmplx *a, cmplx *b );
extern void cmul ( cmplx *a, cmplx *b, cmplx *c );
extern void cneg ( cmplx *a );
extern void cracine ( cmplx *z, cmplx *w );
extern void csub ( cmplx *a, cmplx *b, cmplx *c );
extern double ellie ( double phi, double m );
extern double ellik ( double phi, double m );
extern double ellpe ( double x );
extern int ellpj ( double, double, double *, double *, double *, double * );
extern double ellpk ( double x );
int getnum ( char *line, double *val );
double cay ( double q );
int lampln ( void );
int spln ( void );
int xfun ( void );
int zplna ( void );
int zplnb ( void );
int zplnc ();
int quadf ( double, double, int , double *, double *);
// int ellf(int , int , int , float , float , float , float , float , TypeFiltre*)
double response ( double, double );
#else
double exp(), log(), cos(), sin(), sqrt();
double ellpk(), ellik(), asin(), atan(), atan2(), pow();
double cay();
double response();
int lampln(), spln(), xfun(), zplna(), zplnb(), zplnc(), quadf();
#define fabs(x) ( (x) < 0 ? -(x) : (x) )
#endif

// int main()
int ellfcalc(char modele, char typ0, short degre, float dbr0, int maxi,
			 float omega1, float omega2, float omega3, int *ncoef, double *dir, double *inv)
{
	// char str[80];

	dbfac = 10.0/log(10.0);

	dbg = 0;

	//top:

	kind=(int)modele+1;
	type=(int)typ0+1;
	rn=(double)degre;
	dbr=(double)dbr0;
	fs=1.0;
	f2=(double)omega1;
	f1=(double)omega2;
	dbd=(double)omega3;

	// printf("kind=%d,type=%d,f2=%g / fs=%lg\n",kind,type,f2,fs);

	// Kind of filter (1: Butterworth, 2: Chebyshev, 3: Elliptic,  0: exit to monitor)

	if( (kind <= 0) || (kind > 3) )
		exit(0);

	// Shape of filter (1: low pass, 2: band pass, 3: high pass,  4: band reject, 0: exit to monitor)

	if( (type <= 0) || (type > 4) )
		exit(0);

	// Order of filter (an integer)
	n = (int)rn;
	if( n <= 0 )
	{
		//specerr:
		printf( " Specification error in filtre (routine ellfcalc) order = %d  <=0 \n 	 ",n);
		return(0);
		//	goto top;
	}
	rn = n;	/* ensure it is an integer */
	if( kind > 1 ) /* not Butterworth */
	{
		// Passband ripple (peak to peak decibels)
		if( dbr <= 0.0 )
		{printf( " Specification error in filtre (routine ellfcalc) f3=%f < 0 \n ", (float)dbr);
			return(0);}
		if( kind == 2 )
		{
			/* For Chebyshev filter, ripples go from 1.0 to 1/sqrt(1+eps^2) */
			phi = exp( 0.5*dbr/dbfac );

			if( (n & 1) == 0 )
				scale = phi;
			else
				scale = 1.0;
		}
		else
		{ /* elliptic */
			eps = exp( dbr/dbfac );
			scale = 1.0;
			if( (n & 1) == 0 )
				scale = sqrt( eps );
			eps = sqrt( eps - 1.0 );
		}
	}
	// Sampling frequency (Hz)

	// printf("avant   verif de fs: fs=%lg\n",fs);
	// printf("pendant verif de fs: fs=%lg\n",fs); fs passe a 0 si compil avec option "use int64" !!!
	if( fs <= 0.0 )

	{ printf( " Specification error in filtre (routine ellfcalc)  sampling frequency = %g < 0  \n 	 ",fs);
		return(0);
	}

	fnyq = 0.5 * fs;

	// Passband edge frequency (Hz)

	if( (f2 <= 0.0) || (f2 >= fnyq) )
	{
		printf( "  Specification error in filtre (routine ellfcalc) frequency 2 = %f <=0 or >= fnyq = %f \n  ",(float)f2,(float)fnyq);
		return(0);
	}

	if((type == 2 ) || (type == 4 ))
	{
		// Second passband edge frequency (for band pass or reject filters)

		if( (f1 <= 0.0) || (f1 >= fnyq) )
		{
			printf( "  Specification error in filtre (routine ellfcalc) frequency 1 = %f <=0 or >= fnyq = %f \n  ",(float)f1,(float)fnyq);
			return(0);
		}
	}
	else
	{
		f1 = 0.0;
	}

	if( f2 < f1 )
	{
		a = f2;
		f2 = f1;
		f1 = a;
	}
	if( type == 3 )	/* high pass */
	{
		bw = f2;
		a = fnyq;
	}
	else
	{
		bw = f2 - f1;
		a = f2;
	}
	/* Frequency correspondence for bilinear transformation
	 *
	 *  Wanalog = tan( 2 pi Fdigital T / 2 )
	 *
	 * where T = 1/fs
	 */
	ang = bw * PI / fs;
	// printf("bw=%lg, PI=%g et fs=%lg, soit ang=(bw * PI / fs)=%lg\n",bw,PI,fs,ang);
	cang = cos( ang );
	c = sin(ang) / cang; /* Wanalog */
	if( kind != 3 )
	{
		wc = c;
		//	printf( "cos( 1/2 (Whigh-Wlow) T ) = %.5e, wc = %.5e\n", cang, wc );
	}


	if( kind == 3 )
	{ /* elliptic */
		cgam = cos( (a+f1) * PI / fs ) / cang;
		//	Stop band edge frequency (Hz)  or stop band attenuation (entered as -decibels)

		if( dbd > 0.0 )
			f3 = dbd;
		else
		{ /* calculate band edge from db down */
			a = exp( -dbd/dbfac );
			m1 = eps/sqrt( a - 1.0 );
			m1 *= m1;
			m1p = 1.0 - m1;
			Kk1 = ellpk( m1p );
			Kpk1 = ellpk( m1 );
			q = exp( -PI * Kpk1 / (rn * Kk1) );
			k = cay(q);
			if( type >= 3 )
				wr = k;
			else
				wr = 1.0/k;
			if( type & 1 )
			{
				f3 = atan( c * wr ) * fs / PI;
			}
			else
			{
				a = c * wr;
				a *= a;
				b = a * (1.0 - cgam * cgam) + a * a;
				b = (cgam + sqrt(b))/(1.0 + a);
				f3 = (PI/2.0 - asin(b)) * fs / (2.0*PI);
			}
		}
		switch( type )
		{
			case 1:
				if( f3 <= f2 )
				{
					printf( "  Specification error in filtre (routine ellfcalc) frequency 3 = %f <=  frequency 2 = %f \n  ",(float)f3,(float)f2);
					return(0);
				}
				break;

			case 2:
				if( (f3 > f2) || (f3 < f1) )

				{
					printf( "  Specification error in filtre (routine ellfcalc) frequency 3 = %f >  frequency 2 = %f or frequency 3 = %f <  frequency 1 = %f \n  ",(float)f3,(float)f2,(float)f3,(float)f1);
					return(0);
				}
				break;

			case 3:
				if( f3 >= f2 )
				{
					printf( "  Specification error in filtre (routine ellfcalc) frequency 3 = %f >=  frequency 2 = %f \n  ",(float)f3,(float)f2);
					return(0);
				}

				break;

			case 4:
				if( (f3 <= f1) || (f3 >= f2) )
				{
					printf( "  Specification error in filtre (routine ellfcalc) frequency 3 = %f <=  frequency 1 = %f or frequency 3 = %f >=  frequency 2 = %f \n  ",(float)f3,(float)f1,(float)f3,(float)f2);
					return(0);
				}

				break;
		}
		ang = f3 * PI / fs;
		cang = cos(ang);
		sang = sin(ang);

		if( type & 1 )
		{
			wr = sang/(cang*c);
		}
		else
		{
			q = cang * cang  -  sang * sang;
			sang = 2.0 * cang * sang;
			cang = q;
			wr = (cgam - cang)/(sang * c);
		}

		if( type >= 3 )
			wr = 1.0/wr;
		if( wr < 0.0 )
			wr = -wr;
		y[0] = 1.0;
		y[1] = wr;
		cbp = wr;

		if( type >= 3 )
			y[1] = 1.0/y[1];

		if( type & 1 )
		{
			for( i=1; i<=2; i++ )
			{
				aa[i] = atan( c * y[i-1] ) * fs / PI ;
			}
			//	printf( "pass band %.9E\n", aa[1] );
			//	printf( "stop band %.9E\n", aa[2] );
		}
		else
		{
			for( i=1; i<=2; i++ )
			{
				a = c * y[i-1];
				b = atan(a);
				q = sqrt( 1.0 + a * a  -  cgam * cgam );
#ifdef ANSIC
				q = atan2( q, cgam );
#else
				q = atan2( cgam, q );
#endif
				aa[i] = (q + b) * fnyq / PI;
				pp[i] = (q - b) * fnyq / PI;
			}
			//	printf( "pass band %.9E %.9E\n", pp[1], aa[1] );
			//	printf( "stop band %.9E %.9E\n", pp[2], aa[2] );
		}
		lampln();	/* find locations in lambda plane */
		if( (2*n+2) > ARRSIZ ) {
			*ncoef=2*n+2;
			goto toosml;
		}
	}

	/* Transformation from low-pass to band-pass critical frequencies
	 *
	 * Center frequency
	 *                     cos( 1/2 (Whigh+Wlow) T )
	 *  cos( Wcenter T ) = ----------------------
	 *                     cos( 1/2 (Whigh-Wlow) T )
	 *
	 *
	 * Band edges
	 *            cos( Wcenter T) - cos( Wdigital T )
	 *  Wanalog = -----------------------------------
	 *                        sin( Wdigital T )
	 */

	if( kind == 2 )
	{ /* Chebyshev */
		a = PI * (a+f1) / fs ;
		cgam = cos(a) / cang;
		a = 2.0 * PI * f2 / fs;
		cbp = (cgam - cos(a))/sin(a);
	}
	if( kind == 1 )
	{ /* Butterworth */
		a = PI * (a+f1) / fs ;
		cgam = cos(a) / cang;
		a = 2.0 * PI * f2 / fs;
		cbp = (cgam - cos(a))/sin(a);
		scale = 1.0;
	}

	spln();		/* find s plane poles and zeros */

	if( ((type & 1) == 0) && ((4*n+2) > ARRSIZ) ) {
		*ncoef=4*n+2;
		goto toosml;
	}

	//	printf("  . Conversion plan s vers z\n");
	if(!zplna()) return(0);	/* convert s plane to z plane */
	//	printf("  ... terminee\n");
	zplnb();
	zplnc();
	*ncoef=zord;
	if((zord + 1) > maxi) goto toosml;
	for( j=0; j<=zord; j++ )
	{dir[j]=pp[zord-j];}
	//	{dir[j]=pp[j];}
	for( j=0; j<=zord-1; j++ )
	{inv[j]=aa[zord-j];}
	//	{inv[j]=aa[j+1];}

	//	printf("  . Fonction de transfert\n");
	xfun(); /* tabulate transfer function */
	//	printf("  ... terminee\n");
	//goto top;
	return(1);

toosml:
	// printf( "Cannot continue, storage arrays too small\n" );
	//goto top;
	return(0);
}


int lampln()
{

	wc = 1.0;
	k = wc/wr;
	m = k * k;
	Kk = ellpk( 1.0 - m );
	Kpk = ellpk( m );
	q = exp( -PI * rn * Kpk / Kk );	/* the nome of k1 */
	m1 = cay(q); /* see below */
	/* Note m1 = eps / sqrt( A*A - 1.0 ) */
	a = eps/m1;
	a =  a * a + 1;
	a = 10.0 * log(a) / log(10.0);
	// printf( "dbdown %.9E\n", a );
	a = 180.0 * asin( k ) / PI;
	b = 1.0/(1.0 + eps*eps);
	b = sqrt( 1.0 - b );
	// printf( "theta %.9E, rho %.9E\n", a, b );
	m1 *= m1;
	m1p = 1.0 - m1;
	Kk1 = ellpk( m1p );
	Kpk1 = ellpk( m1 );
	r = Kpk1 * Kk / (Kk1 * Kpk);
	// printf( "consistency check: n= %.14E\n", r );
	/*   -1
	 * sn   j/eps\m  =  j ellik( atan(1/eps), m )
	 */
	b = 1.0/eps;
	phi = atan( b );
	u = ellik( phi, m1p );
	// printf( "phi %.7e m %.7e u %.7e\n", phi, m1p, u );
	/* consistency check on inverse sn */
	ellpj( u, m1p, &sn, &cn, &dn, &phi );
	a = sn/cn;
	// printf( "consistency check: sn/cn = %.9E = %.9E = 1/eps\n", a, b );
	u = u * Kk / (rn * Kk1);	/* or, u = u * Kpk / Kpk1 */
	return 0;
}




/* calculate s plane poles and zeros, normalized to wc = 1 */
int spln()
{
	for( i=0; i<ARRSIZ; i++ )
		zs[i] = 0.0;
	np = (n+1)/2;
	nz = 0;
	if( kind == 1 )
	{
		/* Butterworth poles equally spaced around the unit circle
		 */
		if( n & 1 )
			m = 0.0;
		else
			m = PI / (2.0*n);
		for( i=0; i<np; i++ )
		{	/* poles */
			lr = i + i;
			zs[lr] = -cos(m);
			zs[lr+1] = sin(m);
			m += PI / n;
		}
		/* high pass or band reject
		 */
		if( type >= 3 )
		{
			/* map s => 1/s
			 */
			for( j=0; j<np; j++ )
			{
				ir = j + j;
				ii = ir + 1;
				b = zs[ir]*zs[ir] + zs[ii]*zs[ii];
				zs[ir] = zs[ir] / b;
				zs[ii] = zs[ii] / b;
			}
			/* The zeros at infinity map to the origin.
			 */
			nz = np;
			if( type == 4 )
			{
				nz += n/2;
			}
			for( j=0; j<nz; j++ )
			{
				ir = ii + 1;
				ii = ir + 1;
				zs[ir] = 0.0;
				zs[ii] = 0.0;
			}
		}
	}
	if( kind == 2 )
	{
		/* For Chebyshev, find radii of two Butterworth circles
		 * See Gold & Rader, page 60
		 */
		rho = (phi - 1.0)*(phi+1);  /* rho = eps^2 = {sqrt(1+eps^2)}^2 - 1 */
		eps = sqrt(rho);
		/* sqrt( 1 + 1/eps^2 ) + 1/eps  = {sqrt(1 + eps^2)  +  1} / eps
		 */
		phi = (phi + 1.0) / eps;
		phi = pow( phi, 1.0/rn );  /* raise to the 1/n power */
		b = 0.5 * (phi + 1.0/phi); /* y coordinates are on this circle */
		a = 0.5 * (phi - 1.0/phi); /* x coordinates are on this circle */
		if( n & 1 )
			m = 0.0;
		else
			m = PI / (2.0*n);
		for( i=0; i<np; i++ )
		{	/* poles */
			lr = i + i;
			zs[lr] = -a * cos(m);
			zs[lr+1] = b * sin(m);
			m += PI / n;
		}
		/* high pass or band reject
		 */
		if( type >= 3 )
		{
			/* map s => 1/s
			 */
			for( j=0; j<np; j++ )
			{
				ir = j + j;
				ii = ir + 1;
				b = zs[ir]*zs[ir] + zs[ii]*zs[ii];
				zs[ir] = zs[ir] / b;
				zs[ii] = zs[ii] / b;
			}
			/* The zeros at infinity map to the origin.
			 */
			nz = np;
			if( type == 4 )
			{
				nz += n/2;
			}
			for( j=0; j<nz; j++ )
			{
				ir = ii + 1;
				ii = ir + 1;
				zs[ir] = 0.0;
				zs[ii] = 0.0;
			}
		}
	}
	if( kind == 3 )
	{
		nz = n/2;
		ellpj( u, 1.0-m, &sn1, &cn1, &dn1, &phi1 );
		for( i=0; i<ARRSIZ; i++ )
			zs[i] = 0.0;
		for( i=0; i<nz; i++ )
		{	/* zeros */
			a = n - 1 - i - i;
			b = (Kk * a) / rn;
			ellpj( b, m, &sn, &cn, &dn, &phi );
			lr = 2*np + 2*i;
			zs[ lr ] = 0.0;
			a = wc/(k*sn);	/* k = sqrt(m) */
			zs[ lr + 1 ] = a;
		}
		for( i=0; i<np; i++ )
		{	/* poles */
			a = n - 1 - i - i;
			b = a * Kk / rn;
			ellpj( b, m, &sn, &cn, &dn, &phi );
			r = k * sn * sn1;
			b = cn1*cn1 + r*r;
			a = -wc*cn*dn*sn1*cn1/b;
			lr = i + i;
			zs[lr] = a;
			b = wc*sn*dn1/b;
			zs[lr+1] = b;
		}
		if( type >= 3 )
		{
			nt = np + nz;
			for( j=0; j<nt; j++ )
			{
				ir = j + j;
				ii = ir + 1;
				b = zs[ir]*zs[ir] + zs[ii]*zs[ii];
				zs[ir] = zs[ir] / b;
				zs[ii] = zs[ii] / b;
			}
			while( np > nz )
			{
				ir = ii + 1;
				ii = ir + 1;
				nz += 1;
				zs[ir] = 0.0;
				zs[ii] = 0.0;
			}
		}
	}
	// printf( "s plane poles:\n" );
	//j = 0;
	//for( i=0; i<np+nz; i++ )
	//	{
	//	a = zs[j];
	//	++j;
	//	b = zs[j];
	//	++j;
	//	printf( "%.9E %.9E\n", a, b );
	//	if( i == np-1 )
	//		printf( "s plane zeros:\n" );
	//	}
	return 0;
}





/*		cay()
 *
 * Find parameter corresponding to given nome by expansion
 * in theta functions:
 * AMS55 #16.38.5, 16.38.7
 *
 *       1/2
 * ( 2K )                   4     9
 * ( -- )     =  1 + 2q + 2q  + 2q  + ...  =  Theta (0,q)
 * ( pi )                                          3
 *
 *
 *       1/2
 * ( 2K )     1/4       1/4        2    6    12    20
 * ( -- )    m     =  2q    ( 1 + q  + q  + q   + q   + ...) = Theta (0,q)
 * ( pi )                                                           2
 *
 * The nome q(m) = exp( - pi K(1-m)/K(m) ).
 *
 *                                1/2
 * Given q, this program returns m   .
 */
double cay(local_q)
double local_q;
{
	double local_a, local_b, p, local_r;
	double t1, t2;

	local_a = 1.0;
	local_b = 1.0;
	local_r = 1.0;
	p = local_q;

	do
	{
		local_r *= p;
		local_a += 2.0 * local_r;
		t1 = fabs( local_r/local_a );

		local_r *= p;
		local_b += local_r;
		p *= local_q;
		t2 = fabs( local_r/local_b );
		if( t2 > t1 )
			t1 = t2;
	}
	while( t1 > MACHEP );

	local_a = local_b/local_a;
	local_a = 4.0 * sqrt(local_q) * local_a * local_a;	/* see above formulas, solved for m */
	return(a);
}




/*		zpln.c
 * Program to convert s plane poles and zeros to the z plane.
 */

extern cmplx cone;

int zplna()
{
	cmplx local_r, cnum, cden, cwc, ca, cb, b4ac;
	double C;
	char faux;

	faux = 0;
	if( kind == 3 )
		C = c;
	else
		C = wc;

	for( i=0; i<ARRSIZ; i++ )
	{
		z[i].r = 0.0;
		z[i].i = 0.0;
	}

	nc = np;
	jt = -1;
	ii = -1;

	for( icnt=0; icnt<2; icnt++ )
	{
		/* The maps from s plane to z plane */
		do
		{
			ir = ii + 1;
			ii = ir + 1;
			local_r.r = zs[ir];
			local_r.i = zs[ii];

			switch( type )
			{
				case 1:
				case 3:
					/* Substitute  s - r  =  s/wc - r = (1/wc)(z-1)/(z+1) - r
					 *
					 *     1  1 - r wc (       1 + r wc )
					 * =  --- -------- ( z  -  -------- )
					 *    z+1    wc    (       1 - r wc )
					 *
					 * giving the root in the z plane.
					 */
					cnum.r = 1 + C * local_r.r;
					cnum.i = C * local_r.i;
					cden.r = 1 - C * local_r.r;
					cden.i = -C * local_r.i;
					jt += 1;
					if(dbg) printf("(%s) cdiv1(%g + i x %g)\n",__func__,cden.r,cden.i);
					cdiv( &cden, &cnum, &z[jt] );
					if(dbg) printf("(%s.1) z[%d] = (%g + i x %g)\n",__func__,jt,z[jt].r,z[jt].i);
					if( local_r.i != 0.0 )
					{
						/* fill in complex conjugate root */
						jt += 1;
						z[jt].r = z[jt-1 ].r;
						z[jt].i = -z[jt-1 ].i;
						if(dbg) printf("(%s.2) z[%d] = (%g + i x %g)\n",__func__,jt,z[jt].r,z[jt].i);
					}
					break;

				case 2:
				case 4:
					/* Substitute  s - r  =>  s/wc - r
					 *
					 *     z^2 - 2 z cgam + 1
					 * =>  ------------------  -  r
					 *         (z^2 + 1) wc
					 *
					 *         1
					 * =  ------------  [ (1 - r wc) z^2  - 2 cgam z  +  1 + r wc ]
					 *    (z^2 + 1) wc
					 *
					 * and solve for the roots in the z plane.
					 */
					if( kind == 2 )
						cwc.r = cbp;
					else
						cwc.r = c;
					cwc.i = 0.0;
					cmul( &local_r, &cwc, &cnum );     /* r wc */
					csub( &cnum, &cone, &ca );   /* a = 1 - r wc */
					cmul( &cnum, &cnum, &b4ac ); /* 1 - (r wc)^2 */
					csub( &b4ac, &cone, &b4ac );
					b4ac.r *= 4.0;               /* 4ac */
					b4ac.i *= 4.0;
					if((b4ac.r > 1.0e10) || (b4ac.i > 1.0e10)) { printf("(%s!!) b4ac = (%g + i x %g)\n",__func__,b4ac.r,b4ac.i); faux = 1; dbg = 1; }
					cb.r = -2.0 * cgam;          /* b */
					cb.i = 0.0;
					if((cb.r > 1.0e10) || (cb.i > 1.0e10)) { printf("(%s!!) b4ac = (%g + i x %g)\n",__func__,cb.r,cb.i); faux = 1; dbg = 1; }
					cmul( &cb, &cb, &cnum );     /* b^2 */
					csub( &b4ac, &cnum, &b4ac ); /* b^2 - 4 ac */
					//				printf("  . Extraction racine(%g + i %g)\n",b4ac.r,b4ac.i);
					cracine( &b4ac, &b4ac );
					//				printf("  ... terminee\n");
					cb.r = -cb.r;  /* -b */
					cb.i = -cb.i;
					ca.r *= 2.0; /* 2a */
					ca.i *= 2.0;
					if((ca.r > 1.0e10) || (ca.i > 1.0e10)) { printf("(%s!!) b4ac = (%g + i x %g)\n",__func__,ca.r,ca.i); faux = 1; dbg = 1; }
					cadd( &b4ac, &cb, &cnum );   /* -b + sqrt( b^2 - 4ac) */
					if(dbg) {
						printf("(%s) b4ac = (%g + i x %g)\n",__func__,b4ac.r,b4ac.i);
						printf("(%s) + cb = (%g + i x %g)\n",__func__,cb.r,cb.i);
						printf("(%s) = cnum (%g + i x %g)\n",__func__,cnum.r,cnum.i);
						printf("(%s) / ca = (%g + i x %g)\n",__func__,ca.r,ca.i);
					}
					cdiv( &ca, &cnum, &cnum );   /* ... /2a */
					if(dbg) printf("(%s) = cnum (%g + i x %g)\n",__func__,cnum.r,cnum.i);
					jt += 1;
					cmov( &cnum, &z[jt] );
					if(dbg) printf("(%s.3) z[%d] = (%g + i x %g)\n",__func__,jt,z[jt].r,z[jt].i);
					if((z[jt].r > 1.0e10) || (z[jt].i > 1.0e10)) { printf("(%s!!) b4ac = (%g + i x %g)\n",__func__,z[jt].r,z[jt].i); faux = 1; dbg = 1; }
					if( cnum.i != 0.0 )
					{
						jt += 1;
						z[jt].r = cnum.r;
						z[jt].i = -cnum.i;
						if(dbg) printf("(%s.4) z[%d] = (%g + i x %g)\n",__func__,jt,z[jt].r,z[jt].i);
					}
					if( (local_r.i != 0.0) || (cnum.i == 0) )
					{
						csub( &b4ac, &cb, &cnum );  /* -b - sqrt( b^2 - 4ac) */
						if(dbg) printf("(%s) cdiv3(%g + i x %g)\n",__func__,ca.r,ca.i);
						cdiv( &ca, &cnum, &cnum );  /* ... /2a */
						if(dbg) printf("(%s) termine\n",__func__);
						jt += 1;
						cmov( &cnum, &z[jt] );
						if(dbg) printf("(%s.5) z[%d] = (%g + i x %g)\n",__func__,jt,z[jt].r,z[jt].i);
						if( cnum.i != 0.0 )
						{
							jt += 1;
							z[jt].r = cnum.r;
							z[jt].i = -cnum.i;
							if(dbg) printf("(%s.6) z[%d] = (%g + i x %g)\n",__func__,jt,z[jt].r,z[jt].i);
						}
					}
			} /* end switch */
		}
		while( --nc > 0 );

		if( icnt == 0 )
		{
			zord = jt+1;
			if( nz <= 0 )
			{
				if( kind != 3 ) {
					if(dbg) printf("(%s) zord=%d, nz=%d\n",__func__,zord,nz);
					return(1-faux);
				} else
					break;
			}
		}
		nc = nz;
	} /* end for() loop */
	if(dbg) printf("(%s) zord=%d\n",__func__,zord);
	return(1-faux);
}




int zplnb()
{
	cmplx lin[2];

	lin[1].r = 1.0;
	lin[1].i = 0.0;

	if( kind != 3 )
	{ /* Butterworth or Chebyshev */
		/* generate the remaining zeros */
		while( 2*zord - 1 > jt )
		{
			if( type != 3 )
			{
				//	printf( "adding zero at Nyquist frequency\n" );
				jt += 1;
				z[jt].r = -1.0; /* zero at Nyquist frequency */
				z[jt].i = 0.0;
			}
			if( (type == 2) || (type == 3) )
			{
				//	printf( "adding zero at 0 Hz\n" );
				jt += 1;
				z[jt].r = 1.0; /* zero at 0 Hz */
				z[jt].i = 0.0;
			}
		}
	}
	else
	{ /* elliptic */
		while( 2*zord - 1 > jt )
		{
			jt += 1;
			z[jt].r = -1.0; /* zero at Nyquist frequency */
			z[jt].i = 0.0;
			if( (type == 2) || (type == 4) )
			{
				jt += 1;
				z[jt].r = 1.0; /* zero at 0 Hz */
				z[jt].i = 0.0;
			}
		}
	}
	// printf( "order = %d\n", zord );

	/* Expand the poles and zeros into numerator and
	 * denominator polynomials
	 */
	for( icnt=0; icnt<2; icnt++ )
	{
		for( j=0; j<ARRSIZ; j++ )
		{
			pp[j] = 0.0;
			y[j] = 0.0;
		}
		pp[0] = 1.0;
		for( j=0; j<zord; j++ )
		{
			jj = j;
			if( icnt )
				jj += zord;
			a = z[jj].r;
			b = z[jj].i;
			for( i=0; i<=j; i++ )
			{
				jh = j - i;
				pp[jh+1] = pp[jh+1] - a * pp[jh] + b * y[jh];
				y[jh+1] =  y[jh+1]  - b * pp[jh] - a * y[jh];
			}
		}
		if( icnt == 0 )
		{
			for( j=0; j<=zord; j++ )
				aa[j] = pp[j];
		}
	}
	/* Scale factors of the pole and zero polynomials */
	a = 1.0;
	switch( type )
	{
		case 3:
			a = -1.0;

		case 1:
		case 4:

			pn = 1.0;
			an = 1.0;
			for( j=1; j<=zord; j++ )
			{
				pn = a * pn + pp[j];
				an = a * an + aa[j];
			}
			break;

		case 2:
			gam = PI/2.0 - asin( cgam );  /* = acos( cgam ) */
			mh = zord/2;
			pn = pp[mh];
			an = aa[mh];
			ai = 0.0;
			if( mh > ((zord/4)*2) )
			{
				ai = 1.0;
				pn = 0.0;
				an = 0.0;
			}
			for( j=1; j<=mh; j++ )
			{
				a = gam * j - ai * PI / 2.0;
				cng = cos(a);
				jh = mh + j;
				jl = mh - j;
				pn = pn + cng * (pp[jh] + (1.0 - 2.0 * ai) * pp[jl]);
				an = an + cng * (aa[jh] + (1.0 - 2.0 * ai) * aa[jl]);
			}
	}
	return 0;
}




int zplnc()
{
	double pa, pb, za, zb;
	//int ninv,ndir;

	gain = an/(pn*scale);
	if( (kind != 3) && (pn == 0) )
		gain = 1.0;
	// printf( "constant gain factor %23.13E\n", gain );
	for( j=0; j<=zord; j++ )
		pp[j] = gain * pp[j];

	// printf( "z plane Denominator      Numerator\n" );
	//for( j=0; j<=zord; j++ )
	//	{
	//	printf( "%2d %17.9E %17.9E\n", j, aa[j], pp[j] );
	//  }


	//    printf( "poles and zeros with corresponding quadratic factors\n" );

	for( j=0; j<zord; j++ )
	{
		a = z[j].r;
		b = z[j].i;
		if( b >= 0.0 )
		{
			//		printf( "%d pole  %23.13E %23.13E\n",j, a, b );
			quadf( a, b, 1 , &pb, &pa);
		}
		jj = j + zord;
		a = z[jj].r;
		b = z[jj].i;
		if( b >= 0.0 )
		{
			//		printf( "%d zero  %23.13E %23.13E\n",j, a, b );
 		quadf( a, b, 0 , &zb , &za);
		}
	}
	return 0;
}




/* display quadratic factors
 */
int quadf( x, local_y, pzflg , bb , local_aa )
double x, local_y,*local_aa,*bb;
int pzflg;	/* 1 if poles, 0 if zeros */
{
	double  local_a,local_b,local_r, f, g, g0;

	if( local_y > 1.0e-16 )
	{
		local_a = -2.0 * x;
		local_b = x*x + local_y*local_y;
	}
	else
	{
		local_a = -x;
		local_b = 0.0;
	}
	//printf( "q. f.\nz**2 %23.13E\nz**1 %23.13E\n", local_b, local_a );
	*local_aa=local_a;
	*bb=local_b;
	if( local_b != 0.0 )
	{
		/* resonant frequency */
		local_r = sqrt(local_b);
		f = PI/2.0 - asin( -local_a/(2.0*local_r) );
		f = f * fs / (2.0 * PI );
		/* gain at resonance */
		g = 1.0 + local_r;
		g = g*g - (local_a*local_a/local_r);
		g = (1.0 - local_r) * sqrt(g);
		g0 = 1.0 + local_a + local_b;	/* gain at d.c. */
	}
	else
	{
		/* It is really a first-order network.
		 * Give the gain at fnyq and D.C.
		 */
		f = fnyq;
		g = 1.0 - local_a;
		g0 = 1.0 + local_a;
	}

	if( pzflg )
	{
		if( g != 0.0 )
			g = 1.0/g;
		else
			g = MAXNUM;
		if( g0 != 0.0 )
			g0 = 1.0/g0;
		else
			g = MAXNUM;
	}
	//printf( "f0 %16.8E  gain %12.4E  DC gain %12.4E\n\n", f, g, g0 );
	return 0;
}



/* Print table of filter frequency response
 */
int xfun()
{
	double f, local_r;
	int local_i;

	f = 0.0;

	for( local_i=0; local_i<=20; local_i++ )
	{
		local_r = response( f, gain );
		if( local_r <= 0.0 )
			local_r = -999.99;
		else
			local_r = 2.0 * dbfac * log( local_r );
		//	printf( "%10.1f  %10.2f\n", f, local_r );
		f = f + 0.05 * fnyq;
	}
	return 0;
}


/* Calculate frequency response at f Hz
 * mulitplied by amp
 */
double response( f, amp )
double f, amp;
{
	cmplx x, num, den, w;
	double local_u;
	int local_j;

	/* exp( j omega T ) */
	local_u = 2.0 * PI * f /fs;
	x.r = cos(local_u);
	x.i = sin(local_u);
	//	if(dbg) printf("(%s) x = (%g + i x %g) [zord=%d]\n",__func__,x.r,x.i,zord);
	if(dbg) {
		for( local_j=0; local_j<zord; local_j++ ) printf("(%s) z[%d] = (%g + i x %g)\n",__func__,local_j,z[local_j].r,z[local_j].i);
		for( local_j=0; local_j<zord; local_j++ ) printf("(%s) z[%d] = (%g + i x %g)\n",__func__,local_j,z[local_j+zord].r,z[local_j].i);
	}
	
	num.r = 1.0;
	num.i = 0.0;
	den.r = 1.0;
	den.i = 0.0;
	for( local_j=0; local_j<zord; local_j++ )
	{
		csub( &z[local_j], &x, &w );
		//		if(dbg) printf("(%s) (%g + i x %g) - (%g + i x %g) -> w = (%g + i x %g)\n",__func__,z[local_j].r,z[local_j].i,x.r,x.i,w.r,w.i);
		//		if(dbg) printf("(%s) (%g + i x %g) x (%g + i x %g) ",__func__,w.r,w.i,den.r,den.i);
		cmul( &w, &den, &den );
		//		if(dbg) printf("-> den = (%g + i x %g)\n",den.r,den.i);
		csub( &z[local_j+zord], &x, &w );
		//		if(dbg) printf("(%s) (%g + i x %g) - (%g + i x %g) -> w = (%g + i x %g)\n",__func__,z[local_j+zord].r,z[local_j+zord].i,x.r,x.i,w.r,w.i);
		cmul( &w, &num, &num );
	}
	if(dbg) printf("(%s) cdiv(%g + i x %g)\n",__func__,den.r,den.i);
		cdiv( &den, &num, &w );
		if(dbg) printf("(%s) termine\n",__func__);
			w.r *= amp;
			w.i *= amp;
			local_u = cabs( &w );
			return(local_u);
}




/* Get a number from keyboard.
 * Display previous value and keep it if user just hits <CR>.
 */
int getnum( line, val )
char *line;
double *val;
{
	char s[40];
	
	printf( "%s = %.9E ? ", line, *val );
//	gets( s );
	fgets( s, 40, stdin );
	if( s[0] != '\0' )
	{
		sscanf( s, "%lf", val );
		printf( "%.9E\n", *val );
	}
	return 0;
}

