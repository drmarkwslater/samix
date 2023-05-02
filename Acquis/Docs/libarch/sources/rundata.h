#ifndef RUNDATA_H
#define RUNDATA_H

#include <unistd.h>

#ifdef CODE_WARRIOR_VSN
#include <stat.h>  /* off_t */
#else
#include <sys/time.h>  /* gettimeofday */
#endif

#include <defineVAR.h>
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
// #define A_LA_MARINIERE
// #define A_LA_GG

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
	SRC_EDW1,
	SRC_EDW2,
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

typedef enum {
	TRMT_DEMODUL = 1,
#ifdef TRMT_ADDITIONNELS
	TRMT_LISSAGE,
	TRMT_MOYENNE,
	TRMT_MAXIMUM,
	TRMT_DECONV,
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
	TRGR_REPRISE_FIXE = 0,
	TRGR_REPRISE_SEUIL,
	TRGR_REPRISE_RENOUV,
	TRGR_REPRISE_NB
} TRGR_REPRISE;
#define TRGR_REPRISE_CLES "fixe/attends_seuil/reporte"

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
	CALAGE_UNIQUE
} CALAGE_PORTEE;
#define CALAGES_TEXTES "neant/par_physique/unique"

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
	int  parm;              /* parm du traitmt (facteur lissage, marge demodul, ..)   */
	int  parm2;             /* autre parm du traitmt (delta derivee..)                */
	int  gain;              /* gain logiciel                                          */
	char texte[TRMT_NOM];   /* parm du traitmt si parm==texte                         */
#ifdef RL
	char template;          /* filtrage par template neant/database/analystique       */
#endif
} TypeTraitement;
typedef struct {        /* recherche d'evenements si trigger == cable */
	char std;               /* faux si trigger voie defini en propre                  */
	char type;              /* type demande                                           */
	char sens;              /* sens de variation (+1)                                 */
	char origine;           /* lieu de calcul du trigger                              */
	char regul_demandee;    /* vrai si la regul auto des seuils est autorisee         */
	char reprise;           /* tactique de temps mort (fixe/attends_seuil/reporte)    */
	char coupures;          /* vrai si coupures actives                               */
	char veto;              /* vrai si veto quand les coupures ne passent pas         */
	float minampl;          /* amplitude minimum                                      */
	float maxampl;          /* amplitude maximum si seuil negatif ou indifferent      */
	float montee;           /* temps de montee minimum (millisecs)                    */
	float porte;            /* largeur a mi-hauteur minimum (millisecs) ou taux (Hz)  */
	float fluct;            /* fluctuation de la periode si trgr aleatoire            */
	float underflow;        /* Amplitude (absolue) mini avant rejet (ADU)             */
	float overflow;         /* Amplitude (absolue) maxi avant rejet (ADU)             */
	float montmin;          /* temps de montee minimum (millisecs)                    */
	float montmax;          /* temps de montee maximum (millisecs)                    */
	float maxbruit;         /* bruit maximum sur la ligne de base                     */
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
	float periodes;
	int mesureLdB;
	int traitees;
	int post_moyenne;
} TypeVoieDurees;
typedef struct {
	char nom[CONFIG_NOM];
	TypeVoieModele  evt;                  /* parametrage par config de l'evenement    */
	TypeTraitement  trmt[TRMT_NBCLASSES]; /* parametres par config du traitement      */
	TypeTrigger     trgr;                 /* parametrage par config du trigger        */
	TypeRegulParms  rgul;
	struct { char evt,trmt[TRMT_NBCLASSES],trgr,rgul; } defini;
	struct { char evt,trmt[TRMT_NBCLASSES],trgr,rgul; } global; /* index dans Config[CONFIG_TOT] */
} TypeConfigDefinition;
typedef struct {
	TypeConfigDefinition def[CONFIG_MAX];
	struct {
		short evt,trmt[TRMT_NBCLASSES],trgr,rgul;
	} num[CONFIG_TOT]; /* index de Config[] vers TypeConfigDefinition */
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
	int   physique;         /* signal physique mesure par ce type de voie                 */
	short numero;           /* index ds tableau des voies (reference aussi ds chaque evt) */
	short num_hdr[ARCH_TYPEDATA]; /* index dans l'entete des sauvegardes selon type       */
	short simu;             /* index ds tableau des simulations (-1 si pas defini)        */
//-	short copie;            /* -1 si cette voie ne doit pas etre dupliquee sur une autre  */
//-	short orig;             /* -1 si cette voie ne doit pas etre dupliquee d'une autre    */
	TypeComposantePseudo *pseudo;  /* composantes si pseudo-voie                          */
	unsigned char adrs;     /* adresse de (ou des) ADC desservant cette voie              */
	char num_adc;           /* numero d'ADC utilise                                       */
	char type_adc;          /* type d'ADC utilise                                         */
	char modulee;           /* vrai si electronique genere modulation sur cette voie      */
	char gegene;            /* vrai si generation drecurente 'evenements autorisee        */
	float gain_fixe,gain_variable;
	float Rmodul;           /* R0modul*C0modul / Cfroide (Mohms)                          */
	float sensibilite;      /* sensibilite en nV par keV                                  */
	float *compens;         /* adresse de la valeur de compensation si modulee            */
	char sauvee;                          /* faux si voie a ne jamais sauver              */
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
	TypeADU zero;           /* valeur du zero si l'ADC est signe (== bit de signe)        */
	unsigned short extens;  /* extension du bit de signe pour avoir une donnee signee     */
	int32 ADCmask;          /* type: voir LectEnCours.valeur                              */
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
	int64 debut;           /* point de debut d'evenement (point stocke)                   */
	int64 ptmax;           /* point du maximum de l'evenement (point absolu)              */
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
	char depasse_le_seuil;
	float base;            /* ligne de base en ADU                                        */
	float bruit;           /* bruit sur la permiere moitie de la ligne de base            */
	float amplitude;       /* amplitude donnees trigger en ADU                            */
	float amplibrute;      /* amplitude donnees brutes en ADU                             */
	float maxADU;          /* valeur du maximum en ADU                                    */
	float montee;		   /* temps de montee en millisecs                                */
	float descente;		   /* temps de descente en millisecs                              */
	float integrale[DETEC_PHASES_MAX];	/* integrale du signal, par phase                 */
	float total;		   /* integrale du signal total                                   */
	float decal;		   /* decalage avec la voie ayant trigge (millisecs)              */
	float seuil;           /* meilleur fit evt unite sur ligne de base                    */
	float energie;         /* fit evt unite                                               */
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
	int sec,msec;          /* datation du max en secondes et microsecondes                */
	int TMsec,TMmsec;      /* temps mort total au moment de cet evenement                 */
	char regen;            /* vrai si regeneration en cours                               */
	int gigastamp,stamp;   /* moment du maximum (nb echantillons 100kHz)                  */
	off_t pos;             /* position dans le fichier                                    */
	float delai;           /* delai depuis le precedent evt (secondes)                    */
	float trig;            /* delai depuis le precedent trigger (secondes)                */
	int mode;              /* Mode de sauvegarde si evts (seul/coinc/voisins/tous)        */
	unsigned int liste[EVT_BITTRIGGER_MAX]; /* bolos/voies ayant depasse le seuil (1/bit) */
	short bolo_hdr;        /* bolo a l'origine du trigger (index dans VoieEvent[])        */
	short voie_hdr;        /* voie a l'origine du trigger (index dans VoieEvent[])        */
	short voie_evt;        /* voie a l'origine du trigger (index dans Evt[].voie)         */
	int nbvoies;           /* nombre de voies sauvees                                     */
	TypeVoieArchivee voie[VOIES_MAX]; /* liste des voies sauvees                          */
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

ARCHIVE_VAR int   VsnManip;
//- ARCHIVE_VAR char  VoiesConnues[VOIE_LISTE_STR];

ARCHIVE_VAR char  BigEndianOrder,BigEndianSource,BitTriggerNiveau,BitTriggerNums;
ARCHIVE_VAR int   BitTriggerBloc,BitTriggerDim;
ARCHIVE_VAR char  BitTriggerNom[EVT_BITTRIGGER_MAX][16],BitTriggerCom[EVT_BITTRIGGER_MAX][40];
ARCHIVE_VAR char  ArchType;
ARCHIVE_VAR float VersionIdentification;
ARCHIVE_VAR int   PrgmRepartiteur;
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
ARCHIVE_VAR int   BolosAdemarrer;   /* Nb a demarrer avec le run    */
ARCHIVE_VAR int   BolosAentretenir; /* Nb a regenerer regulierement */
ARCHIVE_VAR short BolosPresents;    /* Nb dans l'entete d'archivage */
ARCHIVE_VAR int   BoloStandard;
ARCHIVE_VAR int   Voie8bits;
ARCHIVE_VAR float MasseTotale;
ARCHIVE_VAR char  RunCategCles[80]; // RunEnvir[256];

ARCHIVE_VAR TypeVoieStandard VoieStandard[VOIES_TYPES],CaptParam;
ARCHIVE_VAR TypeVoieAlire VoieManip[VOIES_MAX];
ARCHIVE_VAR TypeVoieDonnees *VoieEvent;
ARCHIVE_VAR int VoiesNb;              /* Nb voies connues                 */
ARCHIVE_VAR int   VoiesLues;          /* Nb voies transmises par bloc PCI */
ARCHIVE_VAR short VoieLue[VOIES_MAX]; /* No de VoieManip dans bloc PCI    */
                                      /* ne contient une voie que si elle est 'lue' */
ARCHIVE_VAR TypeTrmtCsnsm TrmtCsnsm[TRMT_CSNSM_MAX];
ARCHIVE_VAR int TrmtCsnsmNb;

ARCHIVE_VAR char TrmtDeconvolue;

/* ........................................................ */

ARCHIVE_VAR int   DureeTampons;           /* dimension tampons (toutes voies) en ms */
ARCHIVE_VAR int   LngrTampons;            /* longueur d'un tampon sans reduction    */
ARCHIVE_VAR int   UsageTampons;           /* limite raisonnable avant debordement   */
ARCHIVE_VAR char **SrceNoms;
ARCHIVE_VAR char *SrceListe[TRMT_NBEVTCALCULS+2]; /* nom des calculs utilisables par user    */
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
ARCHIVE_VAR char  SettingsCoupures,SettingsCoherenceEvt;
ARCHIVE_VAR char  TrmtRegulActive;
ARCHIVE_VAR int   TrmtRegulModifs;
ARCHIVE_VAR char  SettingsAltivec,SettingsSoustra,SettingsSauteEvt,SettingsCalcPretrig,LectTrigOnline;
ARCHIVE_VAR char  SettingsSansTrmt,SettingsStartactive,SettingsDLUactive;
ARCHIVE_VAR char  SettingsCalageType,SettingsCalageRef,SettingsCentrageType,SettingsCentrageRef;
ARCHIVE_VAR char  SettingsCalageNom[MODL_NOM],SettingsCentrageNom[MODL_NOM];
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

// ARCHIVE_VAR char **TrmtClasseNoms;
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

ARCHIVE_VAR char TrmtRegulAction[MAXLISTENOMS];

ARCHIVE_VAR char *TrmtRegulTextes
#ifdef ARCHIVE_C
 = "continuer/increment_de/multipl.par/diviser_par/attribuer"
#endif
;

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
ARCHIVE_VAR char SauvVol[MAXFILENAME];
ARCHIVE_VAR char EventName[MAXFILENAME],FichierEvents[MAXFILENAME];
ARCHIVE_VAR char PrefsTango[MAXFILENAME],ResuPath[MAXFILENAME],FigPath[MAXFILENAME],DbazPath[MAXFILENAME];
ARCHIVE_VAR char SambaInfoDir[MAXFILENAME],TangoInfoDir[MAXFILENAME],HelpPath[MAXFILENAME];
ARCHIVE_VAR char AnaDir[MAXFILENAME],GrafDir[MAXFILENAME],CalibName[MAXFILENAME];
ARCHIVE_VAR char ServeurX11[MAXFILENAME];
ARCHIVE_VAR char SambaFontName[MAXFILENAME];
ARCHIVE_VAR int  SambaFontSize;
ARCHIVE_VAR unsigned char SambaTraits;
ARCHIVE_VAR char LangDir[MAXFILENAME],LangMag[MAXFILENAME],LangueDemandee[MAXFILENAME];
ARCHIVE_VAR char LangueEtendDico;
ARCHIVE_VAR char ModeScript,Bavard;
ARCHIVE_VAR char CreationArg,SousUnix,FondNoir,FondPlots;

/* propres a TANGO */
ARCHIVE_VAR char Rapide,Tolerant;
ARCHIVE_VAR char EvtsDir[MAXFILENAME];
ARCHIVE_VAR char NomFiltres[MAXFILENAME];
ARCHIVE_VAR char OptnSpec[80],RunsName[80],CommandeDirecte[256];
ARCHIVE_VAR int  MaxEvts;
ARCHIVE_VAR float TangoRCdefaut;
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

ARCHIVE_VAR ArgDesc TangoDesc[]
#ifdef ARCHIVE_C
 = {
	{ DESC_COMMENT "Traitement et Analyse Nouvelle GeneratiOn" },
	{ "E",        DESC_TEXT                SauvVol,          "Espace de sauvegarde des donnees" },
	{ "evts",     DESC_TEXT                EvtsDir,          "Repertoire des evenements sauvegardes (relatif a l'espace de sauvegarde)" },
	{ "o",        DESC_STR(80)             OptnSpec,         "Options specifiques a la manip" },
	{ 0,          DESC_TEXT                EventName,        "Nom du fichier a analyser" },
	{ "m",        DESC_INT                &MaxEvts,          "Nombre d'evenements maximum (0 si evenements de longueur fixe)" },
	{ DESC_COMMENT 0 },
	{ "P",        DESC_TEXT                PrefsTango,       "Repertoire des preferences SAMBA/TANGO (relatif au repertoire courant)" },
	{ "R",        DESC_TEXT                ResuPath,         "Repertoire des resultats (idem)" },
	{ "G",        DESC_TEXT                FigPath,          "Repertoire des figures sauvees (idem)" },
	{ "D",        DESC_TEXT                DbazPath,         "Racine de la base de donnees des detecteurs (idem)" },
	{ DESC_COMMENT 0 },
	{ "batch",    DESC_NONE }, // DESC_BOOL                &ModeScript,      "Execution en mode batch" },
	{ "exec",     DESC_STR(256)            CommandeDirecte,  "Commande a executer immediatement (syntaxe avec -exec=help)" },
	{ "liste",    DESC_STR(80)             RunsName,         "Demande une liste de runs" },
	{ "fast",     DESC_BOOL               &Rapide,           "Ouverture seule (ou lecture prealable) des archives" }, // EvtAuPif
	{ "aleat",    DESC_BOOL               &Tolerant,         "Ne verifie PAS la continuite de la numerotation des evenements" }, // Discontinu
	{ "info",     DESC_STR(MAXFILENAME)    TangoInfoDir,     "Repertoire contenant l'aide et le dictionnaire de langue" },
	{ "langages", DESC_STR(MAXFILENAME)    LangDir,          "Repertoire des dictionnaires (relatif au repertoire des preferences)" },
	{ "langue",   DESC_STR(MAXFILENAME)    LangueDemandee,   "Langage a utiliser dans les dialogues" },
	{ "dico",     DESC_KEY "net/fixe/maj", &LangueEtendDico, "maj: complete avec les derniers ajouts/ net: retire les termes non traduits (net/fixe/maj)" },
	{ "verbeux",  DESC_BOOL                &Bavard,          "Mode bavard" },
	{ "unix",     DESC_BOOL                &SousUnix,        "Execution sous UNIX (TangoArgs optionnel)" },
	{ DESC_COMMENT "Si pas batch:" },
	{ "display",  DESC_STR(MAXFILENAME)     ServeurX11,      "Serveur X11 a utiliser pour l'affichage" },
	{ "noir",     DESC_BOOL                &FondNoir,        "Fond noir, texte blanc pour les fenetres de dialogue" },
	{ "fontname", DESC_STR(MAXFILENAME)     SambaFontName,   "Type de caracteres" },
	{ "fontsize", DESC_INT                 &SambaFontSize,   "Taille des caracteres" },
	{ "qualite",  DESC_KEY "papier/ecran", &FondPlots,       "Qualite des graphiques (papier/ecran)" },
	{ "linesize", DESC_OCTET               &SambaTraits,     "Largeur des traits des graphiques en qualite papier" },
	{ "linecolr", DESC_NONE }, // DESC_STR(16)           TangoColrFig,     "Couleur des traits pour nanopaw" },
	{ "fitcolr",  DESC_NONE }, // DESC_STR(16)           TangoColrFit,     "Couleur des fits pour nanopaw" },
	{ DESC_COMMENT 0 },
	{ "RC_us",    DESC_FLOAT               &TangoRCdefaut,   "Temps de descente par defaut de l'ampli" },
	{ "filtres",  DESC_TEXT                 NomFiltres,      "Nom du fichier de filtres pre-definis (relatif aux preferences)" },
	{ "T",        DESC_TEXT                 AnaDir,          "Repertoire des preferences TANGO (relatif aux preferences)" },
	{ "calib",    DESC_TEXT                 CalibName,       "Nom du fichier de l'evenement unite (relatif aux preferences TANGO)" },
	{ "A",        DESC_TEXT                 HelpPath,        "Repertoire contenant l'aide" },
	{ "NSDocumentRevisionsDebugMode", DESC_STR(8) XcodeTangoDebugTxt,  "Execute via Xcode" },
	DESC_END
}
#endif
;

void SambaDumpBolos();
int SambaDumpVoies(char toutes, char composition);
int SambaDumpVoiesToutes();

void SambaTrgrEncode(TypeTrigger *trgr, char *texte, int max);
float SambaDeconvolue(float *deconvolue, TypeDonnee *adrs, int dim, float ldb, int pre, int post, float perte, char calcule);

#endif
