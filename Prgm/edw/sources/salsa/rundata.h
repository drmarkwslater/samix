#ifndef RUNDATA_H
#define RUNDATA_H

#include <unistd.h>

#ifdef CODE_WARRIOR_VSN
	#include <stat.h>  /* off_t */
#else
	#include <sys/time.h>  /* gettimeofday */
#endif

// #define WndAction WndUserRequest

#include <defineVAR.h>

#ifdef ForGasol
	#define ForSamba
	#define ForOpiumGL
#endif
#ifdef ForSambaWXF
	#define ForSamba
	#define ForOpiumWXF
#endif
#ifdef ForSambaWXO
	#define ForSamba
	#define ForOpiumWXO
#endif
#ifdef ForSambaWXB
	#define ForSamba
	#define ForOpiumWXB
#endif
#ifdef ForSambaExec
	#define ForSamba
	#define ForOpiumTS
#endif

#ifdef CMDE_UNIX
	#define L_(texte) texte
	#define LL_(liste) liste
	#define WndColorLevel unsigned short
#else
	#define FROM_APPLI
	#include <opium.h>
#endif
#include <filtres.h>

#include <dimensions.h>
#include <edw_types.h>
#include <detecteurs.h>

#undef ARCHIVE_VAR
#ifdef ARCHIVE_C
#define ARCHIVE_VAR
#else
#define ARCHIVE_VAR extern
#endif

/* .............................. Definitions ............................... */

/* En cas de chgt de dimension, penser aux declarations de BB dans CmdeLitMontage */
#define MAXCATGVOIE 12
#define MAXECHELLES 3
#define MAXLISTENOMS 128
#define MAXMAITRE 4
#define MAXERREURLNGR 80

#define NOM_DISTANT "Portail"
#define NOM_SATELLITE "Acquisition"
#define NOM_SUPERVISEUR "Superviseur"

//-- #define MAXTRAME 8
//-- #define DETEC_ABSENT 0xFF

#define RL
#define TRMT_ADDITIONNELS
#ifdef TRMT_ADDITIONNELS
	// #define TRMT_DECONVOLUE
	// #define A_LA_MARINIERE
	// #define A_LA_GG
#endif

typedef enum {
	SAMBA_MONO = 0,
	SAMBA_SATELLITE,
	SAMBA_MAITRE,
	SAMBA_DISTANT,
	SAMBA_AUTOGERE,
	SAMBA_NBMODES
} SAMBA_MODES;
#define SAMBA_MODE_CLES "independant/satellite/maitre/distant/automatique"

typedef enum {
	SAMBA_PARTIT_STD = 0,
	SAMBA_PARTIT_SOLO,
	SAMBA_PARTIT_GLOBALE,
	SAMBA_PARTIT_NB
} SAMBA_PARTITION;
#define SAMBA_PARTIT_CLES "standard/solo/globale"

typedef enum {
	SAMBA_BITNIV_DETEC = 0,
	SAMBA_BITNIV_VOIE,
	SAMBA_BITNIV_NB
} SAMBA_BITNIV;
#define SAMBA_BITNIV_CLES "detector/channel"

typedef enum {
	SAMBA_BITNUM_LOCAL = 0,
	SAMBA_BITNUM_GLOBAL,
	SAMBA_BITNUM_NB
} SAMBA_BITNUM;
#define SAMBA_BITNUM_CLES "local/global"

typedef enum {
	SRC_SIMU = 0,
	SRC_REEL,
	SRC_EDW0,
//	SRC_EDW1,
//	SRC_EDW2,
	SRC_NB
} SRC_TYPE;

typedef enum {
	LECT_SYNC_D3 = 1,
	LECT_SYNC_SEC
} LECT_SYNCHRO_TYPES;
#define LECT_SYNCHRO_CLES "neant/D3/seconde"

typedef enum {
	TRMT_AU_VOL = 0,
	TRMT_PRETRG,
	TRMT_NBCLASSES
} TRMT_CLASSES;
#define TRMT_CLASSES_CLES "au_vol/pretrigger"

// #define TRMT_DECONVOLUE
typedef enum {
	TRMT_DEMODUL = 1,
#ifdef TRMT_ADDITIONNELS
	TRMT_LISSAGE,
	TRMT_MOYENNE,
	TRMT_MAXIMUM,
	#ifdef TRMT_DECONVOLUE
		TRMT_DECONV,
	#endif
	#ifdef A_LA_GG
		TRMT_INTEDIF,
	#endif /* A_LA_GG */
	#ifdef A_LA_MARINIERE
		TRMT_CSNSM,
	#endif /* A_LA_MARINIERE */
#endif /* TRMT_ADDITIONNELS */
	TRMT_FILTRAGE,
	TRMT_INVALID,
	TRMT_NBTYPES
} TRMT_TYPES;

#ifdef RL
typedef enum {
	TRMT_TEMPL_DB = 1,
	TRMT_TEMPL_ANA,
	TRMT_NBTEMPL
} TRMT_TEMPL_MODE;
#endif

typedef enum {
	TRGR_CABLE = 1,
	TRGR_SCRPT,
	TRGR_PERSO,
	TRGR_NBPRGM
} TRMT_TRIGS;

typedef enum {
	TRGR_SEUIL = 1,
	TRGR_FRONT,
	TRGR_DERIVEE,
	TRGR_ALEAT,
	TRGR_PORTE,
	TRGR_EXT,    // trigger sur aucune voie, mais via un signal (a definir!)
	TRGR_TEST,
	TRGR_NBCABLAGES
} TRMT_CABLAGES;

#define TRGR_SENS_CLES "negatif/indifferent/positif"

typedef enum {
	TRGR_INTERNE = 0,
	TRGR_DEPORTE,
	TRGR_FPGA,
	TRGR_NBORIGINES
} TRGR_ORIGINE;

typedef enum {
	TRGR_COUPE_SANS = 0,
	TRGR_COUPE_AVEC,
	TRGR_COUPE_TJRS,
	TRGR_COUPE_NB
} TRGR_COUPE;
#define TRGR_COUPURE_CLES "non/oui/toujours"

typedef enum {
	TRGR_COUPE_PASSE = 0,
	TRGR_COUPE_REJET
} TRGR_COUPE_ACTION;
#define TRGR_COUPURE_ACTION "passe/rejet"

typedef enum {
	TRGR_REPRISE_FIXE = 0,
	TRGR_REPRISE_SEUIL,
	TRGR_REPRISE_RENOUV,
	TRGR_REPRISE_NB
} TRGR_REPRISE;
#define TRGR_REPRISE_CLES "fixe/attends_seuil/reporte"

// "delai", deja pris en compte avec MONIT_DECAL
ARCHIVE_VAR char *VarName[MONIT_NBVARS+1]
#ifdef ARCHIVE_C
= {
	"amplitude", "amplibrute", "abs(ampl)", "maximum",  "tps.montee", "descente", "duree", "rc",
	"base", "bruit", "energie", "seuil",  "top", "dispers", "charge", "avance", "prelim",
	"decal", "\0"
}
#endif
;
ARCHIVE_VAR char *NtpName[MONIT_NBVARS+1]
#ifdef ARCHIVE_C
= {
	"ampl", "brute", "absampl", "max",   "montee", "descente", "duree", "rc",
	"base", "bruit", "energ", "seuil",   "top", "dispers", "charge", "avance", "prelim",
	"decal", "\0"
}
#endif
;
ARCHIVE_VAR char *VarUnit[MONIT_NBVARS+1]
#ifdef ARCHIVE_C
= {
	"ADU", "ADU", "ADU", "ADU",  "ms", "ms", "ms", "ms",
	"ADU", "ADU", "ADU", "ADU",  "ADU", "ms", "ADU", "ms", "ADU",
	"ms", "\0"
}
#endif
;

typedef enum {
	VARROLE_MIN = 0,
	VARROLE_MAX,
	VARROLE_ACT,
	VARROLE_NB
} TESTVAR_ROLE;
ARCHIVE_VAR char *VarRole[VARROLE_NB+1]
#ifdef ARCHIVE_C
= { "min", "max", "action", "\0" }
#endif
;

#define TESTVAR_ROLE_NOM 16
ARCHIVE_VAR char TestVarRole[MONIT_NBVARS][VARROLE_NB][TESTVAR_ROLE_NOM];

#define TESTVAR_MAX 4
typedef struct {
	int nbvars;
	int ntuple[MONIT_NBVARS]; // index dans VarName
} TypeVarJeu;
typedef struct {
	unsigned char ntuple;
	unsigned char inverse;
	float min,max;
} TypeVarTest;
typedef struct {    /* conditions pour selectionner des evenements */
	char catg;         /* numero d'etiquette associee (-1 si vraie coupure) (?) */
	int nbvars;
	TypeVarTest var[TESTVAR_MAX];  /* rempli selon la variable <Selections> */
} TypeCritereJeu;

ARCHIVE_VAR struct {
	TypeVarJeu coupures;
	TypeVarJeu classes;
	TypeVarJeu enreg;
} VarSelection;

typedef enum {
	TRMT_LDB = 1,
	TRMT_AMPLMAXI,
	TRMT_INTEGRAL,
	TRMT_EVTMOYEN,
	TRMT_NBEVTCALCULS
} TRMT_EVTCALCUL;

typedef enum {
	TRMT_PTN_AVANT = 1,
	TRMT_PTN_APRES,
	TRMT_NBPTNMODES
} TRMT_PTNMODE;
#define TRMT_PTN_TEXTES "non/avant/apres"

typedef enum {
	TRGR_DATE_MONTEE = 0,
	TRGR_DATE_MAXI,
	TRGR_NBDATES
} TRMT_EVTDATE;

typedef enum {
	TRMT_REGUL_TAUX = 1,
	TRMT_REGUL_INTERV
} TRMT_REGUL_MODE;

typedef enum {
	TRMT_REGUL_ADD = 1,
	TRMT_REGUL_MUL,
	TRMT_REGUL_DIV,
	TRMT_REGUL_SET
} TRMT_REGUL_ACTN;

typedef enum {
	CALAGE_PAR_PHYSIQUE = 1,
	CALAGE_UNIQUE,
} CALAGE_PORTEE;
#define CALAGES_PORTEE_CLES "neant/par physique/unique"
typedef enum {
	CALAGE_ENREG,
	CALAGE_AMPL,
} CALAGE_OBJET;
#define CALAGES_OBJET_CLES "enregistrement/temps du maxi"

typedef enum {
	CHARGE_PPALE = 1,
	CHARGE_TOTALE,
	CHARGE_NORMALISE,
	CHARGE_NB
} CHARGE_CALCUL;
#define CHARGE_MODE_CLES "neant/principale/totale/normalise"

typedef enum {
	STRM = 0,
	EVTS,
	ARCH_TYPEDATA
} ARCH_DONNEES;
#define ARCH_TYPE_CLES "stream/events"

/* .............................. Structures ................................ */
typedef struct {
	int nbetg;                    /* nombre d'etages                          */
	TypeDonneeFiltree *brute;     /* tampon circulaire des donnees brutes     */
	TypeDonneeFiltree *filtree[MAXFILTREETG];  /* tampon circulaire des donnees filtrees   */
	int max;                      /* taille d'iceux                           */
	int nb;                       /* nombre de donnees utilisables, ditto     */
	int prems;                    /* premiere donnee a utiiser, ditto         */
} TypeSuiviFiltre;

typedef struct {        /* parametres d'un traitement a la mariniere                */
	char nom[16];
	int A;                  /* decalage de la donnee sortie                         */
	int B;                  /* demi-ligne de base                                   */
	int C;                  /* base-max (??)                                        */
	int D;                  /* max-min (..)                                         */
} TypeTrmtCsnsm;

#define CONFIG_TOT 16
#define CONFIG_NOM 37
#define CONFIG_MAX 4
#define CONFIG_STD "standard"

typedef struct {
	float couplage,gain_ampli;
	float injection,mesure;
} TypeCalib; 
typedef struct {        /* traitements (au vol ou pre-trigger)                        */
	char std;               /* faux si traitement voie defini en propre               */
	char type;              /* voir TRMT_TYPES                                        */
	int  p1;                /* parm du traitmt (facteur lissage, marge demodul, ..)   */
	int  p2;                /* autre parm du traitmt (delta derivee..)                */
	float p3;               /* encore un autre parametre, flottant celui-la           */
	int  gain;              /* gain logiciel                                          */
	char texte[TRMT_TEXTE]; /* parm du traitmt si parm==texte                         */
#ifdef RL
	char template;          /* filtrage par template neant/database/analystique       */
#endif
} TypeTraitement;

typedef struct {        /* conditions pour garder/eliminer des evenements */
#ifdef SANS_SELECTION
	char catg;              /* numero d'etiquette associee (-1 si vraie coupure)         */
	char ampl_inv;          /* vrai si la coupure REJETTE les evts DANS l'intervalle     */
	char mont_inv;          /* vrai si la coupure REJETTE les evts DANS l'intervalle     */
	char bruit_inv;         /* vrai si la coupure REJETTE les evts DANS l'intervalle     */
	float underflow;        /* Amplitude (absolue) mini avant rejet (ADU)                */
	float overflow;         /* Amplitude (absolue) maxi avant rejet (ADU)                */
	float montmin;          /* temps de montee minimum (millisecs)                       */
	float montmax;          /* temps de montee maximum (millisecs)                       */
	float maxbruit;         /* bruit maximum sur la ligne de base                        */
	float porte;            /* largeur a mi-hauteur minimum (millisecs) ou taux (Hz)     */
#else
	float underflow;        /* Amplitude (absolue) mini avant rejet (ADU)                */
	float overflow;         /* Amplitude (absolue) maxi avant rejet (ADU)                */
	float montmin;          /* temps de montee minimum (millisecs)                       */
	float montmax;          /* temps de montee maximum (millisecs)                       */
#endif
} TypeEvtCriteres;

typedef struct {        /* recherche d'evenements si trigger == cable */
	char std;               /* faux si trigger voie defini en propre                     */
	char type;              /* type demande                                              */
	char sens;              /* sens de variation (+1)                                    */
	char origine;           /* lieu de calcul du trigger                                 */
	char regul_demandee;    /* vrai si la regul auto des seuils est autorisee            */
	char reprise;           /* tactique de temps mort (fixe/attends_seuil/reporte/alpha) */
	char coupures;          /* vrai si coupures actives                                  */
	char veto;              /* vrai si veto quand les coupures ne passent pas            */
//- #ifdef SANS_SELECTION
	TypeEvtCriteres condition;
//- #endif
	float minampl;          /* amplitude minimum                                         */
	float maxampl;          /* amplitude maximum si seuil negatif ou indifferent         */
	float montee;           /* temps de montee minimum (millisecs)                       */
	float porte;            /* largeur a mi-hauteur minimum (millisecs) ou taux (Hz)     */
	float fluct;            /* fluctuation de la periode si trgr aleatoire               */
	int   alphaampl;        /* seuil pour un temps mort special alpha                    */
	float alphaduree;       /* temps mort special alpha (ms)                             */
	struct {
		char  in,out;       /* autorisations de sortie / entree                          */
		float delai;        /* delai de sortie du signal trigger (-1 si pas emis)        */
		int   bolo,cap;     /* bolo et capteur branches sur sortie trigger autre voie    */
//		int   source;       /* voie branchee sur sortie trigger                          */
	} conn;                 /* connection de trigger entre voies                         */
} TypeTrigger;
typedef struct {
	char std;                  /* faux si regulation voie definie en propre       */
	char type;                 /* type de regulation du declenchement             */
	float mininf,maxinf;       /* planchers de regulation                         */
	float minsup,maxsup;       /* plafonds  de regulation                         */
} TypeRegulParms;
typedef struct {
	int duree;        /* duree de la mesure                  */
	char active;
	struct {
		int nb;          /* nombre d'evenements limite */
		char action;     /* action a entreprendre      */
		float parm;      /* parametre de cette action  */
	} min,max;        /* ceci pour un intervalle [min,max]   */
} TypeRegulTaux;
typedef struct {
	int   nb;
	float duree;
	float ajuste;
	float seuil;
	int   max;
	float incr;
} TypeRegulInterv;
typedef struct {
	struct {
		int pre,post;
		float rc;
		float seuil;
		char calcule;
	} deconv;
	float periodes;
	int mesureLdB;
	int traitees;
	int post_moyenne;
} TypeVoieDurees;
typedef struct {        /* classification des evenements */
	char nom[CATG_NOM];
	char num;             /* numerotation dans le tableau global (valide seulement sur duree du run) */
	short monit;          /* numero de MonitCatg associee */
#ifdef SANS_SELECTION
	TypeEvtCriteres condition;
#endif
	TypeCritereJeu  definition;
} TypeCateg;
typedef struct { char evt,trmt[TRMT_NBCLASSES],trgr,coup,rgul,catg; } TypeAcqInfo;
/* EVTS deja defini dans ARCH_DONNEES, utiliser STREAM/EVENTS a la place?
typedef enum {
	EVTS = DETEC_PARM_EVTS,
	TVOL = DETEC_PARM_TVOL,
	PREP = DETEC_PARM_PREP,
	TRGR = DETEC_PARM_TRGR,
	COUP = DETEC_PARM_COUP,
	RGUL = DETEC_PARM_RGUL,
	CATG = DETEC_PARM_CATG,
} ACQINFO;
*/
/* Texte: voir DetecteurParmsTitre dans detecteurs.c */

typedef struct {
	char nom[CONFIG_NOM];
	TypeVoieModele  evt;                  /* parametrage de l'evenement         */
	TypeTraitement  trmt[TRMT_NBCLASSES]; /* parametrage du traitement          */
	TypeTrigger     trgr;                 /* parametrage du trigger             */
	TypeCritereJeu  coup;                 /* suppression d'evenements           */
	TypeRegulParms  rgul;                 /* regulation du taux de trigger      */
	TypeCateg       catg[MAXCATGVOIE];    /* definition de classes d'evenements */
	char            coup_std;
	char            catg_std;
	int             nbcatg;
	TypeAcqInfo     defini;
	TypeAcqInfo     global;                /* index dans Config[CONFIG_TOT]     */
} TypeConfigDefinition;
typedef struct {
	TypeConfigDefinition def[CONFIG_MAX];
	TypeAcqInfo num[CONFIG_TOT]; /* index de Config[] vers TypeConfigDefinition */
	int nb;
} TypeConfigVoie;

typedef struct {
	TypeConfigDefinition def;
	TypeRegulTaux   echelle[MAXECHELLES]; /* parametres de regulation par taux           */
	TypeRegulInterv interv;               /* parametres de regulation par intervalles    */
	TypeVoieDurees  duree;
	TypeConfigVoie config;
	short vm;                       /* auto-index (pLectTraitements)                     */
	short numtrmt[TRMT_NBCLASSES];  /* numero de traitement (pLectBolosTrmt)             */
	char txttrgr[TRGR_NOM];         /* description du trigger (pLectBolosTrgr)           */
	char modulee;           /* vrai si electronique genere modulation sur cette voie     */
	char exec;
	int min,max;            /* limites pour l'affichage (en desuetude)                   */
} TypeVoieStandard;

typedef struct {
	char  nom[VOIE_NOM];
	char  prenom[MODL_NOM];
	char  type;             /* categorie                                                  */
	int   det;              /* detecteur represente                                       */
	char  cap;              /* capteur correspondant du detecteur                         */
	char  rep;              /* repartiteur a utiliser                                     */
	int   physique;         /* signal physique mesure par ce type de voie                 */
	short numero;           /* index ds tableau des voies (reference aussi ds chaque evt) */
	short num_hdr[ARCH_TYPEDATA]; /* index dans l'entete des sauvegardes selon type       */
	short gene;             /* index dans tableau des generateurs (-1 si pas defini)      */
//-	short copie;            /* -1 si cette voie ne doit pas etre dupliquee sur une autre  */
//-	short orig;             /* -1 si cette voie ne doit pas etre dupliquee d'une autre    */
	TypeComposantePseudo *pseudo;  /* composantes si pseudo-voie                          */
	byte adrs;     /* adresse de (ou des) ADC desservant cette voie              */
	char num_adc;           /* numero d'ADC utilise                                       */
	char type_adc;          /* type d'ADC utilise                                         */
	char modulee;           /* vrai si electronique genere modulation sur cette voie      */
	float gain_fixe,gain_variable;
	float Rmodul;           /* R0modul*C0modul / Cfroide (Mohms)                          */
	float Rbolo;            /* Resistance detecteur simulee (si modulee)                  */
	float sensibilite;      /* sensibilite en nV par keV                                  */
	float *compens;         /* adresse de la valeur de compensation si modulee            */
	char sauvee;                          /* faux si voie a ne jamais sauver              */
#ifdef SANS_SELECTION
	TypeEvtCriteres catg[MAXCATGVOIE];
#endif
	TypeCalib       calib;                /* parametres de calibration et bruit electr.   */
	TypeConfigDefinition def;
	TypeRegulTaux   echelle[MAXECHELLES]; /* parametres de regulation par taux            */
	TypeRegulInterv interv;               /* parametres de regulation par intervalles     */
	TypeVoieDurees  duree;
	TypeConfigVoie config;
	short numtrmt[TRMT_NBCLASSES];        /* numero de traitement (pLectBolosTrmt)        */
	char txttrgr[TRGR_NOM];               /* description du trigger (pLectBolosTrgr)      */
	char trgravant[TRGR_NOM];             /* description du trigger (pLectBolosTrgr)      */
	char exec;              /* vrai si demodulation au vol si possible pre-trigger        */
	int min,max;            /* limites pour l'affichage (en desuetude)                    */
	char affiche;           /* affichage demande (TANGO seulement)                        */
	char differe;           /* vrai si demodulation au vol a faire pre-trigger            */
	char  signe;            /* vrai si l'ADC est signe                                    */
	lhex  zero;             /* valeur du zero si l'ADC est signe (== bit de signe)        */
	lhex  extens;           /* extension du bit de signe pour avoir une donnee signee     */
	lhex  ADCmask;          /* type: voir LectEnCours.valeur                              */
	float nVenkeV;          /* facteur de calibration si lineaire (keV/nV)                */
	float ADUenmV;          /* Valeur d'1 ADU en mV a l'entree de l'ADC                   */
	float ADUenV;           /* Valeur d'1 ADU en V a l'entree de la compensation          */
	float ADUennV;          /* Valeur d'1 ADU en nV a la sortie du detecteur              */
	float nV_reels;         /* idem, en utilisant la valeur declaree pour la BB           */
	float ADUenkeV;         /* Valeur d'1 ADU en keV                                      */
	float ADUenE;           /* Valeur d'1 ADU en electrons dus au cablage (preampli)      */
	float RC;               /* Temps de descente de l'ampli associe                       */
} TypeVoieAlire;

typedef struct {
	int num;               /* numero de voie global                                       */
	short enregistree;     /* index dans la liste des voies du header                     */
	int64 date_locale;     /* date du maximum de l'evenement (point stocke)               */
	int64 debut;           /* point de debut d'evenement (point stocke)                   */
	int64 stamp_ref;       /* point de reference de l'evenement (point absolu)            */
	int maxigiga;          /* point de reference de l'evenement (point absolu)            */
	int maxistamp;         /* point de reference de l'evenement (point absolu)            */
	int adresse;           /* index du debut dans le tampon brut                          */
	int dim;               /* nombre de points sauvegardes                                */
	float horloge;         /* duree d'un point (millisecs)                                */
	int avant_evt;         /* nb points memorises avant le trigger                        */
	float trig_pos;        /* amplitude minimum                                           */
	float trig_neg;        /* amplitude maximum si seuil negatif ou indifferent           */
//	int up;                /* nombre de points de montee                                  */
/* 	int maxi;              valeur du maxi en ADU                                          */
	int sec,msec;          /* datation du debut en secondes et microsecondes              */
	int stamp,gigastamp;   /* datation du debut en nombre d echantillons                  */
	char stream;           /* vrai si trigger sur stream, faux si trigger deporte ou FPGA */
	char ref;              /* classe du tampon ayant servi au calcul                      */
	char mesuree;          /* vrai si dev, base, rms et date_locale mesurees              */
	char depasse_le_seuil;
	float val[MONIT_NBVARS];
	float amplimontee;     /* amplitude du la montee de 10 a 90 %                         */
	float total;		   /* integrale du signal total                                   */
	float integrale[DETEC_PHASES_MAX];	/* integrale du signal, par phase                 */
	TypeDonnee *filtrees;  /* tampon contenant les donnees traitees pre-trigger (monit)   */
	int max;               /* dimension du susdit                                         */
	int64 fin;             /* point de fin du meme                                        */
	int demdim;            /* dimension du sousdit tampon                                 */
	double *demarrage;     /* 1ers points filtres pour recalcul                           */
	int nbfiltres;         /* nombre ecrits de ceux-ci                                    */
	int recalcul;          /* index dans l'evt ou le recalcul doit commencer              */
} TypeVoieArchivee;

#define EVT_BITTRIGGER_MAX ((VOIES_MAX-1)/32)+1
typedef struct {
	int num;               /* numero absolu depuis le debut de la lecture                 */
	int64 stamp_ref;       /* point de reference de l'evenement (point absolu)            */
	int sec,msec;          /* datation de la reference en secondes et microsecondes       */
	int TMsec,TMmsec;      /* temps mort total au moment de cet evenement                 */
	char regen;            /* vrai si regeneration en cours                               */
	int gigastamp,stamp;   /* moment du maximum (nb echantillons 100kHz)                  */
	off_t pos;             /* position dans le fichier                                    */
	float delai;           /* delai depuis le precedent evt (secondes)                    */
	float trig;            /* delai depuis le precedent trigger (secondes)                */
	int mode;              /* Mode de sauvegarde si evts (seul/coinc/voisins/tous)        */
	char nom_catg[CATG_NOM]; /* categorie en toutes lettres                               */
	short categ;           /* numero de categorie (valide pendant un run)                 */
	struct {
		short num;           /* numero d'etiquette (voir GraphMarkName)                   */
		WndColorLevel r,g,b; /* couleur d'etiquette                                       */
	} tag;                 /* etiquette associee                                          */
	hexa liste[EVT_BITTRIGGER_MAX]; /* bolos/voies ayant depasse le seuil (1/bit)         */
	short bolo_hdr;        /* bolo a l'origine du trigger (index dans VoieEvent[])        */
	short voie_hdr;        /* voie a l'origine du trigger (index dans VoieEvent[])        */
	short voie_evt;        /* voie a l'origine du trigger (index dans Evt[].voie)         */
	int nbvoies;           /* nombre de voies sauvees                                     */
	TypeVoieArchivee voie[VOIES_MAX]; /* liste des voies sauvees                          */
	float *deconvolue;
	int deconv_max;
} TypeEvt;

typedef struct {
	float horloge;          /* duree d'un point du tampon (millisecs)                     */
	int lngr;               /* nb points stockes dans l'evenement                         */
	int avant_evt;          /* nb points stockes a utiliser avant le trigger              */
	int apres_evt;          /* nb points absolus a utiliser jusqu'a la fin de l'evenement */
	int base_dep;           /* nb points stockes pour debut de ligne de base effectif     */
	int base_lngr;          /* nb points stockes de ligne de base effective               */
	TypeTamponDonnees brutes; /* tampon pour les donnees brutes                           */
	TypeFiltre *filtre;     /* filtre finalement utilise pour cette voie                  */
	struct {
		int    dim;             /* dimension demandee            */
		int    max;             /* taille effective              */
		float  periodes;        /* nombre de periodes a moyenner */
		int    total;           /* nombre de points a surveiller */
		float *val;             /* tampon du pattern courant     */
		float *nb;              /* nombre de valeurs sommees     */
	} pattern;              /* valeur en cours                                      */
} TypeVoieDonnees;

#ifndef OPIUM_H
typedef void *Graph;
#endif

/* ............................... Variables ................................ */
ARCHIVE_VAR char SambaMoiMeme[80];
ARCHIVE_VAR char SambaRelease[12];
ARCHIVE_VAR char NomExterne[HOTE_NOM];
ARCHIVE_VAR char SambaCodeHote[CODE_MAX],SambaAdrsHote[HOTE_NOM];
ARCHIVE_VAR char ArchMoiMeme[80];
ARCHIVE_VAR float Version;
ARCHIVE_VAR char  SambaIntitule[256];
// ARCHIVE_VAR char  HoteCode;
// ARCHIVE_VAR char  NomEDW[HOTE_NOM];
ARCHIVE_VAR char  HomeDir[MAXFILENAME];
ARCHIVE_VAR char  Starter[MAXFILENAME];
ARCHIVE_VAR char  Gardiens[MAXFILENAME];
ARCHIVE_VAR char  SambaLogDemarrage;

ARCHIVE_VAR int   VsnManip;
//- ARCHIVE_VAR char  VoiesConnues[VOIE_LISTE_STR];

ARCHIVE_VAR char  BigEndianOrder,BigEndianSource,BitTriggerNiveau,BitTriggerNums;
ARCHIVE_VAR int   BitTriggerBloc,BitTriggerDim;
ARCHIVE_VAR char  BitTriggerNom[EVT_BITTRIGGER_MAX][16],BitTriggerCom[EVT_BITTRIGGER_MAX][40];
ARCHIVE_VAR char  ArchType;
ARCHIVE_VAR float VersionIdentification;
// ARCHIVE_VAR int   PrgmRepartiteur;
ARCHIVE_VAR float HorlogeSysteme;
ARCHIVE_VAR int   Diviseur0,Diviseur1,Diviseur2,Diviseur3;
ARCHIVE_VAR float Echantillonnage;  /* frequence ADC (kHz)          */
ARCHIVE_VAR float Frequence;        /* frequence stream (kHz)       */
ARCHIVE_VAR int   FIFOdim,FifoIP;
// ARCHIVE_VAR int   ModeleVoieNb;       /* Nb voies par bolo (3 si BB v1, depend en theorie du type de BN) */
ARCHIVE_VAR int   DetecteursMax;    /* Nb de bolos maxi autorise    */
ARCHIVE_VAR int   BoloGeres;        /* Nb max de bolos geres (112)  */
ARCHIVE_VAR int   VoiesGerees;      /* Nb max de voies gerees (336) */
ARCHIVE_VAR int   BoloNb;           /* Nb effectivement connus      */
ARCHIVE_VAR int   BolosUtilises;    /* Nb effectivement utilises    */
ARCHIVE_VAR int   BolosAssocies;    /* Nb en association            */
ARCHIVE_VAR int   BolosAdemarrer;   /* Nb a demarrer avec le run    */
ARCHIVE_VAR int   BolosAentretenir; /* Nb a regenerer regulierement */
ARCHIVE_VAR short BolosPresents;    /* Nb dans l'entete d'archivage */
ARCHIVE_VAR int   BoloStandard;
ARCHIVE_VAR char  ModulationExiste;
ARCHIVE_VAR int   Voie8bits;
ARCHIVE_VAR float MasseTotale;
ARCHIVE_VAR char  RunCategCles[80]; // RunEnvir[256];

ARCHIVE_VAR TypeVoieStandard VoieStandard[VOIES_TYPES],CaptParam;
ARCHIVE_VAR TypeVoieAlire VoieManip[VOIES_MAX];
ARCHIVE_VAR TypeVoieDonnees *VoieEvent;
ARCHIVE_VAR int   VoiesNb;            /* Nb voies connues                 */
ARCHIVE_VAR int   VoiesLues;          /* Nb voies transmises par bloc PCI */
ARCHIVE_VAR short VoieLue[VOIES_MAX]; /* No de VoieManip dans bloc PCI    */
                                      /* ne contient une voie que si elle est 'lue' */
ARCHIVE_VAR TypeCateg CategLue;
ARCHIVE_VAR TypeTrmtCsnsm TrmtCsnsm[TRMT_CSNSM_MAX];
ARCHIVE_VAR int TrmtCsnsmNb;

ARCHIVE_VAR char TrmtDeconvolue;

/* ........................................................ */

ARCHIVE_VAR int   DureeTampons;           /* dimension tampons (toutes voies) en ms */
ARCHIVE_VAR int   LngrTampons;            /* longueur d'un tampon sans reduction    */
ARCHIVE_VAR int   UsageTampons;           /* limite raisonnable avant debordement   */
ARCHIVE_VAR char *SrceTitre[SRC_NB+1]
#ifdef ARCHIVE_C
= { "simule", "reel", "edw0", "\0" } // , "edw1", "edw2"
#endif
;
ARCHIVE_VAR char *SrceListe[SRC_NB+1]
#ifdef ARCHIVE_C
= { "gene", "reel", "edw0", "\0" } // , "edw1", "edw2"
#endif
;
ARCHIVE_VAR char **SrceNoms;
ARCHIVE_VAR char  SrceCles[MAXLISTENOMS];
ARCHIVE_VAR int   SrceType;
ARCHIVE_VAR char  SrceName[80];

ARCHIVE_VAR char  RunCategName[TYPERUN_NOM]; /* texte correspondant a RunCategNum */
ARCHIVE_VAR int   RunTypeVar;
ARCHIVE_VAR char  RunTypeName[TYPERUN_NOM];
ARCHIVE_VAR char  SettingsModeDonnees; // SettingsRunEnvr;
ARCHIVE_VAR char  SettingsMultitasks;
ARCHIVE_VAR int   SettingsDepileMax;
ARCHIVE_VAR int   SettingsReadoutPeriod;
ARCHIVE_VAR char  SettingsAutoSync;       /* resynchro auto sur donnees perdues     */
ARCHIVE_VAR char  SettingsChargeReglages,SettingsChargeBolos;
ARCHIVE_VAR char  SettingsIdentCheck,SettingsStatusCheck;
ARCHIVE_VAR int   SettingsSynchroMax;
ARCHIVE_VAR float CompensationAttente;
ARCHIVE_VAR int   SettingsDLUdetec;
//- ARCHIVE_VAR float SettingsInhibe;
ARCHIVE_VAR float SettingsPreTrigger;
ARCHIVE_VAR char  SettingsCalculEvt,SettingsTempsEvt,SettingsRegulEvt;
ARCHIVE_VAR char  SettingsMontee,SettingsIntegrale;
ARCHIVE_VAR char  SettingsCoupures,SettingsConformite,SettingsTrigInOut;
ARCHIVE_VAR char  TrmtRegulActive;
ARCHIVE_VAR int   TrmtRegulModifs;
ARCHIVE_VAR char  SettingsAltivec,SettingsSoustra,SettingsSauteEvt,SettingsCalcPretrig,LectTrigOnline;
ARCHIVE_VAR char  SettingsSansTrmt,SettingsStartactive,SettingsDLUactive;
ARCHIVE_VAR char  SettingsCalageMode,SettingsCalageObj,SettingsCalageRef,SettingsCalageModl[MODL_NOM];
ARCHIVE_VAR char  SettingsCentrageType,SettingsCentrageRef,SettingsCentrageNom[MODL_NOM];
ARCHIVE_VAR char  LectSynchroType;
ARCHIVE_VAR int64 LectDureeD3;
ARCHIVE_VAR char  TrgrGardeSigne;   /* vrai si trigger accorde/ si signe amplitude conserve celui du trigger */
ARCHIVE_VAR int   TrgrRegulClock,TrgrRegulDelai;

ARCHIVE_VAR int   NumeroLect,NumeroBanc,NumeroManip;
ARCHIVE_VAR char  ArchNom[16],ArchDate[16],ArchHeure[16];
ARCHIVE_VAR int   ArchT0sec,ArchT0msec;
ARCHIVE_VAR int   TimeStamp0,GigaStamp0;
ARCHIVE_VAR int   TimeStampN,GigaStampN;
ARCHIVE_VAR int   ArchReduc,ArchRegenTaux;
ARCHIVE_VAR char  RegenManuelle;
ARCHIVE_VAR struct timeval LectDateRun,CalcDateSpectre;
ARCHIVE_VAR char  LectStatsValides;
ARCHIVE_VAR int   LectSyncMesure;
ARCHIVE_VAR short ArchDeclare[ARCH_TYPEDATA],ArchSauve[ARCH_TYPEDATA];

#define MAXCOMMENTNB 8
#define MAXCOMMENTLNGR 64
ARCHIVE_VAR char LectComment[MAXCOMMENTNB][MAXCOMMENTLNGR];

ARCHIVE_VAR char *ModlVoieListe[VOIES_TYPES+1];

ARCHIVE_VAR char **TrmtClasseTrad;
ARCHIVE_VAR char *TrmtClasseTitre[TRMT_NBCLASSES+1];
ARCHIVE_VAR char *TrmtClasseNom[TRMT_NBCLASSES+1];

ARCHIVE_VAR char **TrmtTypeNoms;
ARCHIVE_VAR char *TrmtTypeListe[TRMT_NBTYPES+1];
ARCHIVE_VAR char TrmtTypeCles[MAXLISTENOMS];

ARCHIVE_VAR char **TrmtCalculNoms;
ARCHIVE_VAR char *TrmtCalculListe[TRMT_NBEVTCALCULS+2]; /* nom des calculs utilisables par user    */
ARCHIVE_VAR char TrmtCalculCles[MAXLISTENOMS];

ARCHIVE_VAR char **TrmtLibelleNoms;
ARCHIVE_VAR char *TrmtLibelleListe[TRMT_NBEVTCALCULS+1];

ARCHIVE_VAR char **TrmtTemplateNoms;
ARCHIVE_VAR char *TrmtTemplateListe[TRMT_NBTEMPL+1];
ARCHIVE_VAR char TrmtTemplateCles[MAXLISTENOMS];

ARCHIVE_VAR char **TrgrTypeNoms;;
ARCHIVE_VAR char *TrgrTypeListe[TRGR_NBCABLAGES+2];  /* nom des cablages utilisables par user   */
ARCHIVE_VAR char TrgrTypeCles[MAXLISTENOMS];

ARCHIVE_VAR char **TrgrDefNoms;
ARCHIVE_VAR char *TrgrDefListe[TRGR_NBPRGM+2];         /* nom des types utilisables par user      */
ARCHIVE_VAR char TrgrDefCles[MAXLISTENOMS];

ARCHIVE_VAR char **TrgrOrigineNoms;
ARCHIVE_VAR char *TrgrOrigineListe[TRGR_NBORIGINES+1];
ARCHIVE_VAR char TrgrOrigineCles[MAXLISTENOMS];

ARCHIVE_VAR char **TrgrDateNoms;
ARCHIVE_VAR char *TrgrDateListe[TRGR_NBDATES+2];        /* nom des datations  utilisables par user */
ARCHIVE_VAR char TrgrDateCles[MAXLISTENOMS];

ARCHIVE_VAR char TrmtRegulCles[MAXLISTENOMS],TrmtRegulTrad[MAXLISTENOMS];

ARCHIVE_VAR struct {
	char nom_scrpt[TRGR_SCRPT_MAX];        /* nom du fichier si trigger de type scrpt   */
	int64 analyse;                         /* temps deja analyse                        */
	char type;                             /* numero de type choisi                     */
	char demande;                          /* demande (pas active si reglage chaleur)   */
	char enservice;                        /* effectif au moment de la prise de donnees */
	char actif;                            /* peut etre suspendu dynamiquement          */
} Trigger;
ARCHIVE_VAR struct {
	char demandee;                         /* demandee (pas active si reglage chaleur)  */
	char enservice;                        /* effectif au moment de la prise de donnees */
} Archive;

ARCHIVE_VAR char *ArchFmtNom[ARCH_TYPEDATA+1]
#ifdef ARCHIVE_C
= { "Donnees brutes", "Evenements", "\0" }
#endif
;
ARCHIVE_VAR char ArchClesStream[MAXLISTENOMS];
ARCHIVE_VAR char ArchModeStream,ArchFormat;
ARCHIVE_VAR int  ArchDetecMode;
ARCHIVE_VAR char ArchAssocMode;

ARCHIVE_VAR char ArchiveOuverte[ARCH_TYPEDATA];
ARCHIVE_VAR char ArchiveChemin[ARCH_TYPEDATA][MAXFILENAME],ArchiveCheminCourant[MAXFILENAME];
ARCHIVE_VAR char ArchiveFichier[ARCH_TYPEDATA][MAXFILENAME];
ARCHIVE_VAR char ArchivePartit[MAXFILENAME];
ARCHIVE_VAR char SeuilVariationId[256];

ARCHIVE_VAR float ReglTbolo;
ARCHIVE_VAR char  ArchInfoStatus,EtatTubes;
#ifdef EDW_ORIGINE
	ARCHIVE_VAR char  EtatSrceRegen[AUTOM_REGEN_MAX],EtatSrceCalib[AUTOM_CALIB_MAX];
	ARCHIVE_VAR char  EtatRegen,EtatCalib;
#endif /* EDW_ORIGINE */
ARCHIVE_VAR float LectTauxSeuil;      /* Seuil d'alarme de taux d'evenement (Hz) */
ARCHIVE_VAR float LectDelaiMini;       /* Seuil de delai pour enregistrement (ms) */

/* ---------- Parametres d'appel de TANGO pour le lancer depuis SAMBA ---------- */

/* communs a SAMBA et TANGO */
ARCHIVE_VAR char TopDir[MAXFILENAME],ExecDir[MAXFILENAME];
ARCHIVE_VAR char PrefsTango[MAXFILENAME];
ARCHIVE_VAR char SauvPrefs[MAXFILENAME],ResuPrefs[MAXFILENAME],FigsPrefs[MAXFILENAME],DbazPrefs[MAXFILENAME];
ARCHIVE_VAR char InfoPrefs[MAXFILENAME],SubDirAide[MAXFILENAME],SubDirTrad[MAXFILENAME];
ARCHIVE_VAR char ServeurX11[MAXFILENAME];

ARCHIVE_VAR char FullTopDir[MAXFILENAME],TangoDir[MAXFILENAME];
ARCHIVE_VAR char SauvVol[MAXFILENAME],ResuPath[MAXFILENAME],FigsPath[MAXFILENAME],DbazPath[MAXFILENAME];
ARCHIVE_VAR char DicoSrce[MAXFILENAME],Dictionnaire[MAXFILENAME];
ARCHIVE_VAR char HelpPath[MAXFILENAME];
ARCHIVE_VAR char AnaPath[MAXFILENAME],CoupePath[MAXFILENAME];

ARCHIVE_VAR char SambaFontName[MAXFILENAME];
ARCHIVE_VAR int  SambaFontSize;
ARCHIVE_VAR char SambaKbdType;
ARCHIVE_VAR int  SambaVdoNum;
ARCHIVE_VAR byte SambaTraits;
ARCHIVE_VAR char LangueDemandee[MAXFILENAME];
ARCHIVE_VAR char LangueEtendDico;
ARCHIVE_VAR char ModeScript,Bavard;
ARCHIVE_VAR char CreationArg,SousUnix,FondNoir,FondPlots,Parlote;
ARCHIVE_VAR char ModeBatch;
//- ARCHIVE_VAR char LangDir[MAXFILENAME];

/* propres a TANGO */
ARCHIVE_VAR char TangoInfoDir[MAXFILENAME];
ARCHIVE_VAR char AnaDir[MAXFILENAME],GrafDir[MAXFILENAME],CoupeDir[MAXFILENAME],SubsetDir[MAXFILENAME],CalibDir[MAXFILENAME];
ARCHIVE_VAR char UniteEventName[MAXFILENAME],UniteTempsName[MAXFILENAME];

ARCHIVE_VAR char Rapide,Tolerant,Separateur;
ARCHIVE_VAR char AfficheHdrEvt,AfficheVoies,AfficheNtuple;
ARCHIVE_VAR char AfficheMemeLdB,AfficheMemeMaxi,AfficheLoupe;
ARCHIVE_VAR char AfficheEvtCalib,AfficheEvtDeconv;
ARCHIVE_VAR char EventName[32];
ARCHIVE_VAR char EvtsDir[MAXFILENAME],EvtsPath[MAXFILENAME],FichierEvents[MAXFILENAME];
ARCHIVE_VAR char NomFiltres[MAXFILENAME];
ARCHIVE_VAR char OptnSpec[80],RunsName[80],CommandeDirecte[256];
ARCHIVE_VAR int  MaxEvts;
ARCHIVE_VAR float SambaRCdefaut;
ARCHIVE_VAR char XcodeTangoDebugTxt[8];

#define ENVIR_MAX 32
#define ENVIR_NOM 32
#define ENVIR_CLES 80
#define ENVIR_EXPLICS 80
typedef struct {
	char nom[ENVIR_NOM];
	char type;
	int64 var;
	char mot_cles[ENVIR_CLES];
	char explics[ENVIR_EXPLICS];
} TypeEnvirVar;
ARCHIVE_VAR TypeEnvirVar EnvirVar[ENVIR_MAX],EnvirVarLue;
ARCHIVE_VAR int  EnvirVarNb;

ARCHIVE_VAR ArgDesc *LectEnvirDesc;
void SambaEnvirCreeDesc(void *arg);

ARCHIVE_VAR char ArchClesTypes[256];
ARCHIVE_VAR ArgDesc EnvirVarComplNul[]
#ifdef ARCHIVE_C
 = {
	{ "description", DESC_STR(ENVIR_EXPLICS)         EnvirVarLue.explics,  0 },
	DESC_END
}
#endif
;
ARCHIVE_VAR ArgDesc EnvirVarComplCle[]
#ifdef ARCHIVE_C
 = {
	{ "liste", DESC_STR(ENVIR_CLES) EnvirVarLue.mot_cles, 0 },
	{ "description", DESC_STR(ENVIR_EXPLICS)         EnvirVarLue.explics,  0 },
	DESC_END
}
#endif
;
ARCHIVE_VAR ArgDesc *EnvirVarComplDesc[ARG_TYPE_MAX];
ARCHIVE_VAR ArgDesc EnvirVarDesc[ENVIR_MAX]
#ifdef ARCHIVE_C
 = {
	{ 0,             DESC_STR(ENVIR_NOM)             EnvirVarLue.nom,      0 },
	{ 0,             DESC_KEY ArchClesTypes,       &(EnvirVarLue.type),    0 },
	{ 0,             DESC_VARIANTE(EnvirVarLue.type) EnvirVarComplDesc,    0 },
	DESC_END
 }
#endif
;
ARCHIVE_VAR ArgStruct EnvirVarAS
#ifdef ARCHIVE_C
 = { (void *)EnvirVar, (void *)&EnvirVarLue, sizeof(TypeEnvirVar), EnvirVarDesc }
#endif
;
ARCHIVE_VAR ArgDesc EnvirModlDesc[]
#ifdef ARCHIVE_C
 = {
	{ "Variables", DESC_STRUCT_SIZE(EnvirVarNb,ENVIR_MAX) &EnvirVarAS, 0 },
	{              DESC_EXEC SambaEnvirCreeDesc, 0 },
	DESC_END
}
#endif
;

#define TANGO_SEP_CLES "root/csv/tab"
#define TANGO_EVT_LDB "reelles/alignees"
#define TANGO_EVT_MAXI "reels/alignes"
#define TANGO_EVT_TRACE "masque/trace"

ARCHIVE_VAR ArgDesc TangoDesc[]
#ifdef ARCHIVE_C
 = {
	{ DESC_COMMENT "Traitement et Analyse Nouvelle GeneratiOn" },
	{ 0,             DESC_STR(32)               EventName,        "Fichier a analyser" },
	{ "m",           DESC_INT                  &MaxEvts,          "Nombre d'evenements maximum (0 si evenements de longueur fixe)" },
	{ "o",           DESC_STR(80)               OptnSpec,         "Options specifiques a la manip" },
	{ DESC_COMMENT 0 },
	{ "P",           DESC_STR(MAXFILENAME)      PrefsTango,       "Repertoire des preferences de TANGO (relatif au repertoire courant)" },
	{ "T",           DESC_DEVIENT("A") },
	{ "A",           DESC_STR(MAXFILENAME)      AnaDir,           "Repertoire des produits d'analyse (relatif aux preferences)" },

	/* Organisation commune avec SAMBA, si appele dans SAMBA */
	{ "E",           DESC_STR(MAXFILENAME)      SauvPrefs,        "Espace de sauvegarde des donnees (relatif a la racine)" },
	{ "R",           DESC_STR(MAXFILENAME)      ResuPrefs,        "Repertoire des resultats  (relatif a la racine)" },
	{ "G",           DESC_STR(MAXFILENAME)      FigsPrefs,        "Repertoire des figures recopiees (relatif a la racine)" },
	{ "D",           DESC_STR(MAXFILENAME)      DbazPrefs,        "Racine de la base de donnees des detecteurs (relatif a la racine)" },
	{ "evts",        DESC_STR(MAXFILENAME)      EvtsDir,          "Repertoire des evenements sauvegardes (relatif a l'espace de sauvegarde)" },
	{ "unix",        DESC_BOOL                 &SousUnix,         "Execution sous UNIX (SambaArgs optionnel)" },
	{ DESC_COMMENT "Si pas batch:" },
	{ "display",     DESC_STR(MAXFILENAME)      ServeurX11,       "Serveur X11 a utiliser pour l'affichage" },
	{ "info",        DESC_STR(MAXFILENAME)      InfoPrefs,        "Repertoire des informations complementaires (relatif a la racine)" },
	{ "langages",    DESC_NONE }, // DESC_STR(MAXFILENAME)  LangDir, "Repertoire des dictionnaires (relatif a la racine)" },
	{ "langue",      DESC_STR(MAXFILENAME)      LangueDemandee,   "Langage a utiliser dans les dialogues" },
	{ "dico",        DESC_KEY "net/fixe/maj",  &LangueEtendDico,  "maj: complete avec les derniers ajouts/ net: retire les termes non traduits (net/fixe/maj)" },
	{ "noir",        DESC_BOOL                 &FondNoir,         "Fond noir, texte blanc pour les fenetres de dialogue" },
	{ "fontname",    DESC_STR(MAXFILENAME)      SambaFontName,    "Type de caracteres" },
	{ "fontsize",    DESC_INT                  &SambaFontSize,    "Taille des caracteres" },
	{ "keyboard",    DESC_KEY "US/FR-mac/FR-dell", &SambaKbdType, "Type de clavier (US/FR-mac/FR-dell)" },
	{ "videonum",    DESC_INT                  &SambaVdoNum,      "Numero d'ecran avec OpenGL" },
	{ "qualite",     DESC_KEY "papier/ecran",  &FondPlots,        "Qualite des graphiques (papier/ecran)" },
	{ "linesize",    DESC_OCTET                &SambaTraits,      "Largeur des traits des graphiques en qualite papier" },
	{ "aide",        DESC_STR(MAXFILENAME)      SubDirAide,       "Sous-repertoire de l'aide (relatif au repertoire 'info')" },
	{ "trad",        DESC_STR(MAXFILENAME)      SubDirTrad,       "Sous-repertoire des dictionnaires de langue (relatif au repertoire 'info')" },

	/* Forcement particulier a TANGO */
	{ DESC_COMMENT 0 },
	{ "RC_us",       DESC_DEVIENT("RC_ms") },
	{ "RC_ms",       DESC_FLOAT                &SambaRCdefaut,    "Temps de descente par defaut de l'ampli (corrige par le fichier 'ZonesTemps')" },
	{ "graphiques",  DESC_STR(MAXFILENAME)      GrafDir,          "Repertoire des graphiques (relatif aux produits d'analyse)" },
	{ "coupures",    DESC_STR(MAXFILENAME)      CoupeDir,         "Repertoire des coupures (relatif aux produits d'analyse)" },
	{ "subsets",     DESC_STR(MAXFILENAME)      SubsetDir,        "Repertoire des sous-ensemble d'evenements (relatif aux produits d'analyse)" },
	{ "filtres",     DESC_TEXT                  NomFiltres,       "Fichier de filtres pre-definis (relatif aux preferences)" },
	{ "calibration", DESC_STR(MAXFILENAME)      CalibDir,         "Repertoire des donnees de calibration (relatif aux produits d'analyse)" },
	{ "calib",       DESC_DEVIENT("unite.event") },
	{ "unite.event", DESC_STR(MAXFILENAME)      UniteEventName,   "Fichier de l'evenement unite (relatif aux donnees de calibration)" },
	{ "unite.temps", DESC_STR(MAXFILENAME)      UniteTempsName,   "Fichier des temps caracteristiques de l'evenement unite (relatif aux donnees de calibration)" },
	{ "sep",         DESC_KEY TANGO_SEP_CLES,  &Separateur,       "Separateur des colonnes des ntuples ecrits ("TANGO_SEP_CLES")" },
	{ "evt.hdr",     DESC_BOOL                 &AfficheHdrEvt,    "Affiche les entetes d'evenements" },
	{ "evt.voies",   DESC_BOOL                 &AfficheVoies,     "Affiche les infos des voies ayant participe a l'evenement" },
	{ "evt.ntuple",  DESC_BOOL                 &AfficheNtuple,    "Affiche le ntuple calcule sur l'evenement" },
	{ "evt.ldb",     DESC_KEY TANGO_EVT_LDB,   &AfficheMemeLdB,   "Lignes de base pour un evenement ("TANGO_EVT_LDB")" },
	{ "evt.maxi",    DESC_KEY TANGO_EVT_MAXI,  &AfficheMemeMaxi,  "Temps des maxima pour un evenement ("TANGO_EVT_MAXI")" },
	{ "evt.loupe",   DESC_BOOL                 &AfficheLoupe,     "Agrandissement de l'affichage du detecteur touche" },
	{ "evt.unite",   DESC_KEY TANGO_EVT_TRACE, &AfficheEvtCalib,  "Affichage de l'evenement unite ("TANGO_EVT_TRACE")" },
	{ "evt.deconv",  DESC_KEY TANGO_EVT_TRACE, &AfficheEvtDeconv, "Affichage de l'evenement deconvolue ("TANGO_EVT_TRACE")" },
	{ DESC_COMMENT 0 },
	{ "batch",       DESC_NONE }, // DESC_BOOL &ModeScript,       "Execution en mode batch" },
	{ "exec",        DESC_STR(256)              CommandeDirecte,  "Commande a executer immediatement (syntaxe avec -exec=help)" },
	{ "liste",       DESC_STR(80)               RunsName,         "Demande une liste de runs" },
	{ "fast",        DESC_BOOL                 &Rapide,           "Ouverture seule (ou lecture prealable) des archives" }, // EvtAuPif
	{ "aleat",       DESC_BOOL                 &Tolerant,         "Ne verifie PAS la continuite de la numerotation des evenements" }, // Discontinu
	{ "verbeux",     DESC_BOOL                 &Bavard,           "Mode bavard" },

	{ "NSDocumentRevisionsDebugMode", DESC_STR(8) XcodeTangoDebugTxt,  "Execute via Xcode" },
	DESC_END
}
#endif
;

void AnnexesDefaults();
char SambaTopDirAt(char *top_dir, char *exe_dir, char *args);
void SambaAjouteTopDir(char *nom_local, char *nom_complet);
void SambaCreePath(char *base, char rep, char *dir, char *path);
void SambaLangueDicos(char *dico_compilo, char *dico_user);
void SambaLangueConclusion();

void SambaDumpBolos();
int  SambaDumpVoies(char toutes, char composition);
int  SambaDumpVoiesToutes();

void SambaTrgrEncode(TypeTrigger *trgr, char *texte, int max);
float *SambaAssure(float *buffer, int demandee, int *effective);
void SambaEventCopy(float *trace, TypeDonnee *adrs, int dim, int pt0, int lngr);
float SambaDeconvolue(float *deconvolue, TypeDonnee *adrs, int dim, int pt0, int lngr,
	float ldb, int pre, int post, float perte, float seuil, char calcule);
float SambaCalculeCharges(float *deconvolue, int lngr, char positif, float seuil, float *max, float *prelim, int *duree, int *avance);

#endif
