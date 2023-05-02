#ifndef FILTRES_H
#define FILTRES_H

#include "defineVAR.h"

#ifdef FILTRES_C
	#define FILTRE_VAR
#else
	#define FILTRE_VAR extern
#endif

#define FILTRE_DOUBLE
#define MAXFILTRENOM 20
#define MAXFILTRECOEF 64
#define MAXFILTREETG 8

/*
 * .......... Constantes .......................................................
 */
/* l'ordre des deux typedef ci-dessous est impose par ellfcalc() */
typedef enum {
	FILTRE_BUT = 0,
	FILTRE_CHE,
	FILTRE_ELL,
	FILTRE_MIG,
	FILTRE_DIR,
	FILTRE_NBMODELES
} FILTRE_MODELE;
typedef enum {
	FILTRE_PBAS = 0,
	FILTRE_PASSEB,
	FILTRE_PHAUT,
    FILTRE_COUPEB,
	FILTRE_NBTYPES
} FILTRE_TYPE;
typedef enum {
	FILTRE_REL = 0,
	FILTRE_HZ,
	FILTRE_KHZ,
	FILTRE_MHZ,
	FILTRE_GHZ,
	FILTRE_NBUNITES
} FILTRE_UNITE;

/*
 * .......... Types de variables ...............................................
 */
#ifdef FILTRE_DOUBLE
	typedef double TypeDonneeFiltree;
#else
	typedef float TypeDonneeFiltree;
#endif

typedef struct {
	char nbdir,nbinv;
	TypeDonneeFiltree direct[MAXFILTRECOEF],inverse[MAXFILTRECOEF];
} TypeCelluleFiltrante;

typedef struct {
	char calcule;
	char nbetg; char nbmax;
	TypeCelluleFiltrante etage[MAXFILTREETG];
} TypeFiltre;

typedef struct {
	char nom[MAXFILTRENOM];
	char commentaire[80];
	char modele;
	char type;     /* Passe-bas, passe-haut, passe-bande, ...       */
	short degre;
	char  unite1;  /* si 0: relatif freq. echantillonage            */
	char  unite2;  /* si 0: relatif freq. echantillonage            */
	float omega1;  /* freq. coupure basse [/ freq. echantillonage]  */
	float omega2;  /* freq. coupure haute [/ freq. echantillonage]  */
	TypeFiltre coef;
} TypeFiltreDef;

/*
 * .......... Variables ........................................................
 */
FILTRE_VAR char FiltreErreur[256];

FILTRE_VAR char FiltreFichier[MAXFILENAME];
FILTRE_VAR TypeFiltreDef *FiltreGeneral;
FILTRE_VAR char **FiltreListe;
FILTRE_VAR int FiltreMax,FiltreNb,FiltreNum;

/*
 * .......... Fonctions ........................................................
 */
int FiltreCalcule(float omega1, float omega2, char modele, char type, short degre, TypeFiltre *coef);
int FiltreCalculeStd(float omega1, float omega2, char modele, char type, short degre, TypeFiltre *coef);
void FiltreStandardiseNom(TypeFiltreDef *filtrage);
int FiltresLit(char *nom, char log);
int FiltresAjout(void);
int FiltresModif(void);
int FiltresPerso(void);
int FiltresDump(void);
int FiltresImprime(void);
int FiltresRetrait(void);
int FiltresEcrit(char *nom);

int FiltresRestore(void);
int FiltresSauve(void);
int FiltresInit(int max);
void FiltresDialogueCreate(void);
void FiltresExit(void);

/*
 * .......... Interface Utilisateur ............................................
 */
FILTRE_VAR char *FiltreModeles[FILTRE_NBMODELES + 1]
#ifdef FILTRES_C
 = { "Butterworth", "Chebyshev", "Elliptic", "\0", "MiG", "Direct" }
#endif
;
FILTRE_VAR char *FiltreTypes[FILTRE_NBTYPES + 1]
#ifdef FILTRES_C
 = { "b (passe-bas)", "p (passe-bande)", "h (passe-haut)", "c (coupe-bande)", "\0" }
#endif
;
FILTRE_VAR char *FiltreUnites[FILTRE_NBUNITES + 1]
#ifdef FILTRES_C
= { "ech.", "Hz", "kHz", "MHz", "GHz", "\0" }
#endif
;

#endif
