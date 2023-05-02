#ifndef EDW_TYPES_H
#define EDW_TYPES_H

#include <defineVAR.h>
#include <dimensions.h>

#ifdef FBI
	include <detecteurs.h>
	#if (DETEC_MAX < 256)
		typedef byte TypeCablePosition;
		#define CABLAGE_POS_MAX 256
	#else
		typedef shex TypeCablePosition;
		#define CABLAGE_POS_MAX 65536
	#endif
#endif /* FBI */

typedef shex TypeCablePosition;
#if (NUMER_MAX < 256)
	typedef byte TypeCableConnecteur;
	#define CABLAGE_CONNECT_MAX 256
#else
	typedef shex TypeCableConnecteur;
	#define CABLAGE_CONNECT_MAX 65536
#endif

typedef enum {
	ARCH_INDIV = 0,
	ARCH_VOISINS,
	ARCH_POSY,
	ARCH_COINC,
	ARCH_TOUS,
	ARCH_MANIP,
	ARCH_NBMODES
} ARCH_MODE;

typedef enum {
	TAMPON_INT16 = 0,
	TAMPON_INT32,
	TAMPON_FLOAT,
	TAMPON_NBTYPES
} TAMPON_TYPE;

typedef enum {
	MONIT_AMPL = 0,
	MONIT_BRUTE,
	MONIT_ABS,
	MONIT_MAX,      // identique a MONIT_AMPL, sauf [peut-etre!] si utilisation de l'evt moyen

	MONIT_MONTEE,
	MONIT_DESCENTE, // entre maxi et mi-hauteur descente
	MONIT_DUREE,    // entre mi-hauteurs (descente - montee)
	MONIT_RC,       // temps de descente e(-t/RC)

	MONIT_BASE,
	MONIT_BRUIT,
	MONIT_ENERGIE,  // == 0, sauf si utilisation de l'evt moyen (car = MONIT_AMPL calibre en energie)
	MONIT_SEUIL,    // identique a MONIT_RMS, sauf si utilisation de l'evt moyen (car calibre en energie)

	MONIT_TOP,      // si deconvolution: valeur (ADU) du pic maxi
	MONIT_DISPER,   // si deconvolution: temps entre emergence pic maxi et dernier temps > seuil
	MONIT_CHARGE,   // si deconvolution: integrale au dessus du seuil pendant dispersion
	MONIT_AVANCE,   // si deconvolution: temps entre premiere emergence et emergence pic maxi
	MONIT_PRELIM,   // si deconvolution: integrale au dessus du seuil pendant avance

	MONIT_DECAL,
	MONIT_NBVARS
} MONIT_VARIABLE;

typedef shex TypeADU;

/* #define DONNEES_NON_SIGNEES abandonne le 15.01.04 */
#ifdef DONNEES_NON_SIGNEES
	typedef shex TypeDonnee;
#else
	typedef short TypeDonnee;
#endif
typedef int32 TypeLarge;
typedef float TypeSignal; // initialement, short (pour le tampon pre-trigger) donc == TypeDonnee

typedef struct {
	union {
		void       *data;  /* tableau des valeurs a utiliser de facon generique        */
		TypeDonnee *sval;  /* tableau des valeurs codees comme des entiers sur 16 bits */
		TypeLarge  *ival;  /* tableau des valeurs codees comme des entiers sur 32 bits */
		TypeSignal *rval;  /* tableau des valeurs codees comme des reels sur 32 bits   */
	} t;
	int dim;          /* dimension demandee  */
	int max;          /* dimension effective */
	int nb;           /* quantite utilisable */
	int prem;         /* pointeur de debut   */
	char type;        /* cf TAMPON_TYPE      */
} TypeTamponDonnees;
typedef struct {
	int *val;         /* tableau des valeurs */
	int dim;          /* dimension demandee  */
	int max;          /* dimension effective */
	int nb;           /* quantite utilisable */
	int prem;         /* pointeur de debut   */
} TypeTamponInt;
typedef struct {
	float *val;       /* tableau des valeurs */
	int dim;          /* dimension demandee  */
	int max;          /* dimension effective */
	int nb;           /* quantite utilisable */
	int prem;         /* pointeur de debut   */
} TypeTamponFloat;

#endif
