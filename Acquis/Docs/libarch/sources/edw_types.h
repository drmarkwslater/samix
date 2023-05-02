#ifndef EDW_TYPES_H
#define EDW_TYPES_H

#ifdef macintosh
/* typedef long int32; garanti 32 bits, mais gcc hurle pour les formats (faut utiliser %ld) */
typedef int int32;
#endif

#ifdef UNIX
#ifndef macintosh
typedef int int32;
#endif
#endif

#ifdef __INTEL__
typedef int int32;
#endif
#define _NI_int32_DEFINED_
/* typedef ... int64; defini dans defineVAR.h */

#include <dimensions.h>
// #if (DETEC_MAX < 256)
// typedef unsigned char TypeCablePosition;
// #define CABLAGE_POS_MAX 256
// #else
typedef unsigned short TypeCablePosition;
// #define CABLAGE_POS_MAX 65536
#define CABLAGE_POS_MAX 4*256
#define CABLAGE_POS_UNDEF 0xFFFF
// #endif
#if (NUMER_MAX < 256)
typedef unsigned char TypeCableConnecteur;
#define CABLAGE_CONNECT_MAX 256
#else
typedef unsigned short TypeCableConnecteur;
#define CABLAGE_CONNECT_MAX 65536
#endif
#define CABLAGE_CONN_DIR (CABLAGE_CONNECT_MAX - 1)
#define CABLAGE_TXTNUL "vide"
#define CABLAGE_TXTDIR "intr"
#define CABLAGE_EXPLIC " (0="CABLAGE_TXTNUL", "chaine(CABLAGE_CONN_DIR)"="CABLAGE_TXTDIR")"

typedef enum {
	ARCH_INDIV = 0,
	ARCH_VOISINS,
	ARCH_TOUR,
	ARCH_COINC,
	ARCH_TOUS,
	ARCH_MANIP,
	ARCH_NBMODES
} ARCH_MODE;

typedef enum {
	TAMPON_INT16 = 0,
	TAMPON_FLOAT,
	TAMPON_NBTYPES
} TAMPON_TYPE;

typedef unsigned short TypeADU;

/* #define DONNEES_NON_SIGNEES abandonne le 15.01.04 */
#ifdef DONNEES_NON_SIGNEES
typedef unsigned short TypeDonnee;
#else
typedef short TypeDonnee;
#endif
typedef float TypeSignal; // initialement, short (pour le tampon pre-trigger) donc == TypeDonnee

typedef struct {
	union {
		TypeDonnee *sval;  /* tableau des valeurs codees comme des entiers sur 16 bits */
		int *ival;         /* tableau des valeurs codees comme des entiers sur 32 bits */
		TypeSignal *rval;  /* tableau des valeurs codees comme des reels sur 32 bits */
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
