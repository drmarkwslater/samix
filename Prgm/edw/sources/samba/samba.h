#ifndef SAMBA_H
#define SAMBA_H

#ifdef SAMBA_C
#define SAMBA_VAR
#else
#define SAMBA_VAR extern
#endif

/*!
    @header samba
    @abstract   Definitions generales pour SAMBA, mais pas pour TANGO
    @discussion Symboles, structures et variables
*/
// #define MODULES_RESEAU
// #define MSGS_RESEAU
// #define ETAT_RESEAU
// #define PARMS_RESEAU
// #define SETUP_RESEAU

#if defined(MSGS_RESEAU) || defined(ETAT_RESEAU) || defined(PARMS_RESEAU) || defined(SETUP_RESEAU)
	#ifndef MODULES_RESEAU
		#define MODULES_RESEAU
	#endif
#endif


#include <defineVAR.h>
#include <decode.h>
#include <dico.h>
#include <calendrier.h>
#include <ceb.h>
#ifdef CMDE_UNIX
	#define L_(texte) texte
	#define LL_(liste) liste
#else
	#include <opium_demande.h>
	#include <opium.h>
	// #define WndAction WndUserRequest
#endif
#include <nanopaw.h>
#ifndef __INTEL__
	#include <histos.h>
#endif

/*
#ifdef OPENGL
#pragma message("Option dans "__FILE__": OPENGL")
#else
#pragma message("Option dans "__FILE__": PAS OPENGL")
#endif
#ifdef QUICKDRAW
#pragma message("Option dans "__FILE__": QUICKDRAW")
#else
#pragma message("Option dans "__FILE__": PAS QUICKDRAW")
#endif
#ifdef X11
#pragma message("Option dans "__FILE__": X11")
#else
#pragma message("Option dans "__FILE__": PAS X11")
#endif
*/

#include <rfftw.h>
#include <filtres.h>
#include <IcaLib.h>
#include <sktlib.h>
#include <timeruser.h>

#ifdef MSGS_RESEAU
	#include <carla/msgs.h>
#endif

#include <dimensions.h>

#define MAXEVTS 1024
#define MAX_TRIG_INSTR 16 /* 256 */
#define MAXMONIT 50
#define MAXTRACES 32
// #define DIM_LUT 64 mais... mais... de quoi??? pas OPIUM_LUT_DIM?
#define RESERVE_EVT 0
#define MEDIA_MAX 32

//#define COMPTEUR_0 2134000000
#define COMPTEUR_0 0

//#define COMP_COEF 0.25
#define COMP_COEF (float)0.5
// #define COMP_COEF 1.0

#include <monit.h>
#include <rundata.h>
#include <monit.h>
#include <scripts.h>
#include <salsa.h>
// #include <structure.h>
#include <sncf.h>
#include <edwdb.h>
#include <media.h>
//.. #include <autom.h>

/* .............................. Definitions ............................... */
#ifndef CODE_WARRIOR_VSN
	#define AVEC_IP
	#define AVEC_OPERA
	//#define NIDAQ
#endif

#ifdef macintosh
	// #define PAROLE
	// #define VIA_QD_SPEECH
#endif

#ifdef XCODE
	#define COMMANDE_PING "ping -c 1 -t 1 -noQq %d.%d.%d.%d >/dev/null"
#else
	#define COMMANDE_PING "ping -c 1 -nq %d.%d.%d.%d >/dev/null"
#endif

#define PRINT_OPIUM_ADRS(objet) printf("(%s) " #objet " @%08X ->cdr @%08X ->adrs @%08X\n",__func__,(hexa)objet,(hexa)(objet->cdr),(hexa)((objet->cdr)->adrs));


#ifndef SANS_PCI
	#define AVEC_PCI
#endif
/* define PCI_DIRECTIF du temps ou on imposait l'adresse PCI, voire le bus */
//- #ifndef CLUZEL_FIFO_INTERNE
//- #define CLUZEL_FIFO_INTERNE 131072
//- #endif
#define ADRS_PCI_DISPO
// #define ALTIVEC
/* L'acces PCI direct peut s'executer sans FIFO interne */
/* #define ACCES_PCI_DIRECT */

#define printf LogBook

#define MONIT_EVT_TRAITE
//+ #define CARRE_A_VERIFIER
#define TRAITEMENT_DEVELOPPE
#define TRIGGER_DEVELOPPE
#define NTUPLES_ONLINE

#define DUREE_ILLIMITEE 999999

typedef enum {
	CARTE_AB = 0,
	CARTE_MIG,
	CARTE_ICA,
	CARTE_UNKN
} CARTE_TYPE;

typedef enum {
	LECT_DONNEES = 0,
	LECT_IDENT,
	LECT_COMP,
	LECT_NBMODES
} LECT_MODE;

typedef enum {
	VOIE_NOM_STD = 0,
	VOIE_NOM_COURT,
	VOIE_NOM_NB
} VOIE_NOM_STYLE;
#define VOIE_NOM_CLES "standard/court"

typedef enum {
	LECT_CONS_ETAT = 0,
	LECT_CONS_CMDE,
	LECT_CONS_NB
} LECT_CONS_TYPE;

typedef enum {
	COMPENS_MINI = 1,
	COMPENS_MAXI,
	COMPENS_PIED,
	COMPENS_ERREUR,
	COMPENS_OK,
	COMPENS_HS
} COMPENS_ETAPE;

typedef enum {
	CALC_SPTV_MANU = 0,  // le nouveau est en jaune et propose au cas par cas
	CALC_SPTV_SURV,      // le nouveau est en jaune et ajoute automatiquement
	CALC_SPTV_AUTO,      // le nouveau n'est pas affiche et ajoute automatiquement
	CALC_SPTV_GLIS,      // le nouveau n'est pas affiche et ajoute avec un poids de epsilon
	CALC_SPTV_MODESNB
} CALC_SPTV_MODES;

/*!
    @enum 
    @abstract   Types de mesure de spectres
    @discussion Applicables aux spectres automatiques
    @constant   CALC_SPTA_SOMME sommation simple
 	@constant   CALC_SPTA_TRIGGE declenche (par quoi?)
 	@constant   CALC_SPTA_GLISSANT moyenne ponderee glissante
*/
typedef enum {
	CALC_SPTA_SOMME = 0,
	CALC_SPTA_TRIGGE,
	CALC_SPTA_GLISSANT,
	CALC_SPTA_MODESNB
} CALC_SPTA_MODES;

// la chaine ci-dessous sert uniquement a la planche de lecture compacte (d'ou l'ordre..)
typedef enum {
	LECT_SVG_EVENTS = 0,
	LECT_SVG_NEANT,
	LECT_SVG_STREAM,
	LECT_SVG_NBMODES
} LECT_SVG_MODES;
#define LECT_SVG_CLES "evts/non/stream"

typedef enum {
	RUN_FAMILLE_BANC = 0,
	RUN_FAMILLE_MANIP,
	RUN_FAMILLE_NB
} RUN_FAMILLE;

typedef enum {
	CAHIER_TEXTE = 1,
	CAHIER_ELOG,
	CAHIER_NB
} CAHIER_MANIP;

typedef enum {
	NTP_ASCII = 1,
	NTP_PAW,
	NTP_NBMODES
} NTP_MODE;
#define NTP_MODE_CLES "sans/ASCII/PAW"

typedef enum {
	TEST_VOIE_UNIQUE = 1,
	TEST_EVENEMENT,
	TEST_NBMODES
} TEST_MODE;

/* Erreurs pendant la lecture */
/* Hardware version 0 */
#define LECT_PBHW    0x2000  /* Collision                    */
#define LECT_DESYNC  0x1000  /* Desynchronisation            */
/* Hardware version 1.0 */
#define LECT_COLL    0x0001  /* Collision                    */
#define LECT_ALRM_0  0x0002  /* Alarme sur bolo 0            */
#define LECT_ALRM_1  0x0004  /* Alarme sur bolo 1            */
#define LECT_ALRM_2  0x0008  /* Alarme sur bolo 2            */
#define LECT_ALRM_3  0x0010  /* Alarme sur bolo 3            */
#define LECT_ALRM_4  0x0020  /* Alarme sur bolo 4            */
#define LECT_ALRM_5  0x0040  /* Alarme sur bolo 5            */
#define LECT_ALRM_6  0x0080  /* Alarme sur bolo 6            */
#define LECT_ALRM_7  0x0100  /* Alarme sur bolo 7            */
#define LECT_ALRM_8  0x0200  /* Alarme sur bolo 8            */
#define LECT_ALRM_9  0x0400  /* Alarme sur bolo 9            */
//+ #define LECT_SYNCHRO_D3 0x1000  /* Synchro sur D3            */
#define LECT_SYNCHRO_D3 LectBitD2D3  /* Synchro sur D3            */
/* Software */
#define LECT_FULL    0x1001  /* Buffers de donnees pleins    */
#define LECT_NFND    0x1002  /* Modulation pas trouvee       */
#define LECT_EOF     0x1003  /* Fin de fichier donnees       */
#define LECT_UNKN    0x1004  /* Types de donnee inconnu (0)  */
#define LECT_SYNC    0x1005  /* Resynchronisation impossible */
#define LECT_EMPTY   0x1006  /* FIFO deseperement vide       */
#define LECT_TOOMUCH 0x1007  /* FIFO deseperement pleine     */
#define LECT_INVALID 0x1008  /* Donnee incorrecte            */
#define LECT_INCOHER 0x1009  /* Incoherence FillNb/NonNul    */
#define LECT_UNEXPEC 0x100A  /* Donnee de type inattendu     */
#define LECT_NODATA  0x100B  /* Pas de donnee enregistree    */
#define LECT_OVERFLO 0x100C  /* Le CPU est deborde           */
#define LECT_EVTS    0x100D  /* Taux d'evenements en exces   */
#define LECT_ARCH    0x100E  /* Archivage en defaut          */
#define LECT_ARRETE  0x100F  /* Repartiteur arrete           */
#define LECT_PREVU   0x1010  /* Arret programme d'avance     */

SAMBA_VAR TypeADU LectCodeConnu[]
#ifdef SAMBA_C
 = { 
	LECT_PBHW    , LECT_DESYNC  , LECT_COLL    , 
	LECT_ALRM_0  , LECT_ALRM_1  , LECT_ALRM_2  , LECT_ALRM_3  , LECT_ALRM_4  , 
	LECT_ALRM_5  , LECT_ALRM_6  , LECT_ALRM_7  , LECT_ALRM_8  , LECT_ALRM_9  , 
	LECT_FULL    , LECT_NFND    , LECT_EOF     , LECT_UNKN    , LECT_SYNC    , 
	LECT_EMPTY   , LECT_TOOMUCH , LECT_INVALID , LECT_INCOHER , LECT_UNEXPEC ,
	LECT_NODATA  , LECT_OVERFLO , LECT_EVTS    , LECT_ARCH    , LECT_ARRETE  ,
	LECT_PREVU   , 0
}
#endif
;
#define LECT_MAXCODES 32
// SAMBA_VAR char **LectListeExplics;
SAMBA_VAR char *LectListeExplics[LECT_MAXCODES+1];

/* .............................. Structures ................................ */

// SAMBA_VAR char  AcquisCode[MAXSAT][HOTE_NOM];
// SAMBA_VAR char  AcquisNom[MAXSAT][HOTE_NOM];
// #ifdef MSGS_RESEAU
//     SAMBA_VAR TypeMailAdrs BalAcquis[MAXSAT];
// #endif
// #ifndef CODE_WARRIOR_VSN
//     SAMBA_VAR TypeSocket SambaSynchroSkt[MAXSAT];
// #endif
// SAMBA_VAR int   SambaSynchroPath[MAXSAT];

#define MAX_FOLDERS 50
SAMBA_VAR struct {
	char *nom;
	char titre[80];
} FolderRef[MAX_FOLDERS];
SAMBA_VAR int FolderNb,FolderNum;
SAMBA_VAR char FolderActn;
SAMBA_VAR char *FolderListe[MAX_FOLDERS+1];

#define ACQUIS_MAXREP 32
typedef enum {
	SATELLITE_IGNORE = 0,  /* cet hote n'est pas sur le reseau                 */
	SATELLITE_ABSENT,      /* hote present mais le serveur Carla ne repond pas */
	SATELLITE_ARRETE,      /* Serveur present mais Samba pas demaree           */
	SATELLITE_ATTENTE,     /* Samba demaree, prete a acquerir                  */
	SATELLITE_ACTIF,       /* Acquisition en cours                             */
	SATELLITE_NBETATS
} SATELLITE_ETAT;
SAMBA_VAR char *AcquisTexte[]
#ifdef SAMBA_C
 = { "ignoree", "injoignable", "arretee", "en attente", "active" "\0" }
#endif
;

typedef enum {
	SAT_ETAT = 0,
	SAT_EVT,
	SAT_NBTAGS
} SAT_TAG;

typedef struct {
	shex catg;
	shex sat;
	union {
		struct {
			char  nom_run[RUN_NOM];
			char  duree_lue[20];
			int   evt_trouves,evt_ecrits;
		} etat;
		struct {
			int64 timestamp;
		} evt;
	} m;
} TypeAcquisMsg;

#ifdef ETAT_RESEAU
	SAMBA_VAR int IdLocal;
#endif
#ifdef MSGS_RESEAU
	SAMBA_VAR TypeMailAdrs BalSuperviseur;
#endif

typedef struct {
	char  status;
	char  active;
	char  nom_run[RUN_NOM];
	char  duree_lue[20];
	int   evt_recus,evt_trouves,evt_ecrits;
	float KgJ;
} TypeAcquisEtat;

typedef struct {
	char code[CODE_MAX];
	char adrs[HOTE_NOM];
	char runs[2];
	int  nbrep;
	int  rep[ACQUIS_MAXREP];
	TypeAcquisEtat etat;
	struct { TypeSocket skt; int path; } synchro;
	struct { TypeSocket skt; int path; } ecoute;
#ifdef ETAT_RESEAU
	int id;
#endif
#ifdef MSGS_RESEAU
	TypeMailAdrs bal;
#endif
} TypeAcquis;
SAMBA_VAR TypeAcquis Acquis[MAXSAT],AcquisLue;
SAMBA_VAR int    AcquisNb,AcquisLocale;
SAMBA_VAR char  *AcquisListe[MAXSAT+1];
SAMBA_VAR Panel pAcquisEtat;

typedef struct {
	char mtr[HOTE_NOM];
	int  nbsat;
	int  sat[MAXSAT];
	int  exter[MAXSAT];
} TypePartit;
SAMBA_VAR TypePartit Partit[MAXMAITRE],PartitLue;
SAMBA_VAR int PartitNb,PartitLocale;
SAMBA_VAR char SambaMaitre,SambaSat;
SAMBA_VAR int  LectureMaitre,EcritureMaitre;
SAMBA_VAR char AcquisMsgEnvoye[80];

// #define GALETTES_NB 4
// #define TOURS_NB 12
#define GALETTES_NB ChassisDetec[0].nb
#define TOURS_NB ChassisDetec[1].nb
// #define QUADRANT_NB 4
// #define QUADRANT_LNGR 28
// #define QUADRANT_OF7 28
#define QUADRANT_NB ChassisNumer[0].nb
#define QUADRANT_LNGR ChassisNumer[1].nb
// #define QUADRANT_OF7 ChassisNumer[0].dim
#define QUADRANT_OF7 ChassisNumer[1].nb

#define CHASSIS_DIMMAX 3
#define CHASSIS_OF7 0
// avec CHASSIS_OF7==1;
// - z,Z,0: le decodage peut prendre la valeur 0 (soit 0 pour 'a', 'A' et '1')
// - a,A,1: le decodage ne prendra pas la valeur 0 (donc sera decale de +CHASSIS_OF7, soit CHASSIS_OF7 pour 'a', 'A' et '1')
// - Attention, avec 1: on obtiendra 1 pour '1' et quand meme 0 pour '0'
// - tandis qu'avec 0: on obtiendra 0 pour '1' et meme -1 pour '0' si CHASSIS_OF7==1

// !!! mais !!! CABLAGE_POSITION & al. fonctionnent avec CHASSIS_OF7==0, donc a==z, A==Z, 1==0
typedef enum {
	CHASSIS_CODE_a = 0,
	CHASSIS_CODE_A,
	CHASSIS_CODE_z,
	CHASSIS_CODE_Z,
	CHASSIS_CODE_0,
	CHASSIS_CODE_1,
	CHASSIS_CODE_NOMS,
	CHASSIS_CODE_RANG,
	CHASSIS_NBCODES
} CHASSIS_CODES;
#define CHASSIS_CODE_CLES "a/A/z/Z/0/1/noms/rang"
typedef struct {
	short nb;
	short max;
	char codage;
	char *cles;
} TypeChassisAxe;
SAMBA_VAR TypeChassisAxe ChassisDetec[CHASSIS_DIMMAX],ChassisNumer[CHASSIS_DIMMAX],ChassisLu;
SAMBA_VAR int ChassisDetecDim,ChassisNumerDim;
SAMBA_VAR char ChassisDetecUnique,ChassisNumerUnique;

#ifdef PCI_DIRECTIF
	SAMBA_VAR char  PCIbus[MAXFILENAME];
#endif
#ifdef ADRS_PCI_DISPO
	SAMBA_VAR unsigned long *PCIadrs[PCI_MAX];
#endif

typedef struct {
	PciBoard port;
	int      type;
	char     dma;
	int32    octets;
	unsigned long  *fifo;
	int32    top,bottom;
} TypeCartePci;

#ifdef NIDAQ
	#include <NIDAQmxBase.h>
	SAMBA_VAR char LectNIactive[NI_TACHES_MAX];
	SAMBA_VAR TaskHandle LectNITache[NI_TACHES_MAX];
	int SambaTesteDAQmx(char *nom, int k, char log);
#endif

typedef enum {
	IDENT_ABSENT = 0,
	IDENT_HS,
	IDENT_DESYNCHRO,
	IDENT_IMPREVU,
	IDENT_NORMAL,
	IDENT_NBETATS
} IDENT_RESULTAT;

typedef struct {
	int num;
	void *ref;
} TypeClassement;

#define SAMBA_NTPVOIE_NB MONIT_NBVARS
#define SAMBA_NTP_NIVEAU "non/modele/voie"
typedef enum {
	NTP_MODELE = 1,
	NTP_VOIE,
	NTP_NBNIVEAUX
} NTP_NIVEAU;

SAMBA_VAR TypePlotVar *MonitNtp;
SAMBA_VAR int NtDim;              /* profondeur maxi de ntuple allouee */
SAMBA_VAR int NtUsed;             /* profondeur reellement utilisee    */
SAMBA_VAR float *NtDet;
SAMBA_VAR float *NtVoie[MONIT_NBVARS][VOIES_MAX];
SAMBA_VAR float *NtDelai;
SAMBA_VAR float *NtDate;

typedef struct {
	int rep;                /* repartiteur implique                                         */
	TypeADU *donnees;       /* tampons pour les donnees brutes                              */
	TypeADU *prochain;      /* adresse de la prochaine donnee                               */
	int dim;                /* nombre de points dans les tampons                            */
	short lus;              /* nb total de points memorises                                 */
    short analyse;          /* premier point a analyser                                     */
	TypeADU attendu;        /* valeur attendue                                              */
	TypeADU lu;             /* valeur lue en premier                                        */
	short essais;           /* nombre de valeurs lues                                       */
	short erreurs;          /* nombre de differences avec la valeur lue en premier          */
	short resultat;         /* resultat de l'analyse                                        */
} TypeVoieIdentifiee;

typedef struct {
	int num;                /* numero dans le tableau de toutes les voies (VoieManip p.exe) */
	int rep;                /* numero de repartiteur                                        */
} TypeVoieTransmise;

typedef struct StructHistoDeVoie {
#ifndef __INTEL__
	HistoData hd;
#endif
	char catg;
	char D;          /* dimension (1/2)                     */
	char Xvar;       /* 0: amplitude / 1: montee / etc...   */
	char Yvar;       /* 0: amplitude / 1: montee / etc...   */
	short Yvoie;
	struct StructHistoDeVoie *suivant;
} TypeHistoDeVoie,*HistoDeVoie;

typedef struct {
	char pente;            /* pente en cours                                            */
	float precedente;      /* valeur precedent la valeur courante...                    */
	float mihauteur;       /* niveau a attteindre pour estimer la largeur de la porte   */
	int64 dernierpic;      /* date du precedent extremum (ou debut evt si NIVEAU)       */
	int64 date_evt;        /* date du debut evt si NIVEAU ou DEV					    */
#ifdef RL
	int64 posmax;
#endif
} TypeVoieSuivi;

typedef struct {
	char lue;                    /* vrai si effectivement lue                                   */
	/*	char invalide;             presente dans la trame mais inutilisable                     */
	char post;                   /* vrai si au moins un traitement pre-trigger est demande      */
	char cree_traitee;           /* vrai si le tampon 'traitee' est rempli [=( post | trigger )] */
	char regul_active;           /* mode de regulation auto des seuils de declenchement          */
	char regul_standard;         /* vrai si regulation identique au standard                     */
	char circulaire;             /* vrai si buffer plein (sert au graphique)                     */
	char sauvee;                 /* faux si voie a ne jamais sauver                              */
	int decal;                   /* decalage (pts absolus) debut evenement p.r. chaleur          */
	int interv;                  /* fenetre de recherche (pts absolus)                           */
	// int rapide;               /* temps de descente du signal rapide                           */
	struct {
		int t0;                  /* debut (stockes depuis le maxi) de phase du signal produit    */
		int dt;                  /* duree (stockes depuis le debut) de phase du signal produit   */
	} phase[DETEC_PHASES_MAX];
	int rang_sample;             /* rang dans l'echantillon SambaEchantillon                     */
	TypeTamponDonnees *brutes;   /* tampon pour les donnees brutes                               */
	TypeDonnee *prochain_s16;    /* adresse de la prochaine donnee brute                         */
	TypeDonnee derniere_s16;     /* derniere valeur entree dans le tampon                        */
	TypeDonnee courante_s16;     /* derniere valeur decodee                                      */
	TypeLarge  *prochain_i32;    /* adresse de la prochaine donnee brute	                     */
	TypeLarge  derniere_i32;     /* derniere valeur entree dans le tampon                        */
	TypeLarge  courante_i32;     /* derniere valeur decodee                                      */
	int64 lus;                   /* nb total de points memorises                                 */
	int64 sauve;                 /* dernier point sauve par ArchiveBloc                          */
	int64 jetees;                /* nb ecrasees avant trmt pre-trigger                           */
//	TypeTamponDonnees lissage;   /* tampon de lissage pour le calcul de la ligne de base         */
	TypeTamponDonnees *traitees; /* tampon pour les donnees traitees pre-trigger                 */
	TypeTamponFloat   reelles;   /* liste des taux d'evenements en fonction du temps             */
	TypeTamponFloat   seuilmin;  /* liste des seuils min (positifs) en fonction du temps         */
	TypeTamponFloat   seuilmax;  /* liste des seuils max (negatifs) en fonction du temps         */
	TypeTamponFloat   tabtaux;   /* liste des taux d'evenements en fonction du temps             */
	CebVar *var_montee,*var_amplitude,*var_niveau,*var_duree;     /* variables pour le trigger   */
	float *adrs_montee,*adrs_amplitude,*adrs_niveau,*adrs_duree;  /* valeurs pour le trigger     */
	struct {                /* traitements (au vol ou pre-trigger)                               */
		int  compactage;         /* facteur de reduction (selon type)                            */
		int  nbdata;             /* nombre de donnees prises en compte, idem                     */
		TypeFiltre *filtre;      /* filtre finalement utilise pour cette voie                    */
		int  compte;             /* compteur, utilise par l'algorithme de traitememt             */
		char phase;              /* phase du traitement, idem                                    */
		char utilise;            /* traitement reellement utilise                                */
		int32  valeur;           /* valeur temporaire calculee, idem                             */
		double reelle;           /* valeur temporaire calculee, idem                             */
		TypeSuiviFiltre suivi;   /* suivi du filtrage                                            */
		TypeTamponDonnees donnees; /* tampon pour le resultat du traitement courant              */
	#ifdef TRMT_ADDITIONNELS
		TypeTamponFloat X;       /* suivis du traitement courant                                  */
		TypeTamponInt   L;       /* suivis du traitement courant                                  */
	#if defined(A_LA_GG) || defined(A_LA_MARINIERE)
		TypeTamponFloat Y,Z;     /* suivis de (X,) Y et Z, dans le traitement 'a la mariniere' ou 'intedif' */
		//? TypeTamponInt S;     /* meme objectif                                                 */
		float V,W;               /* meme objectif                                                 */
	#endif
	#endif
		char affichee;         /* si oui, calcul des infos suivantes                        */
		float ab6[2];          /* abcisse pour les traces (contient l'horloge)              */
		int64 plus_ancien;     /* plus ancien point memorise                                */
		int64 point0;          /* "valeur" en temps de tampon[0] (pour affichage)           */
		int64 point_affiche;   /* premier point deja affiche                                */
		int64 point_vu;        /* prochain premier point deja affiche                       */
	} trmt[TRMT_NBCLASSES];
	int64 delai_template;   /* delai (absolu) introduit par un filtrage par template        */
	TypeVoieSuivi suivi;
	int dim_deconv;
	float *deconvoluee;
    struct {
        TypeTrigger *trgr;     /* parametrage effectif du trigger                           */
        char special;          /* vrai si le trigger est different du trigger standard      */
        char calcule;
        char cherche;
		char coupe;            /* vrai doit on doit vraiment regarder les coupures          */
		char conformite;       /* vrai doit on doit vraiment regarder la conformite         */
//		char ampl_inv;         /* vrai si la coupure REJETTE les evts DANS l'intervalle     */
//		char mont_inv;         /* vrai si la coupure REJETTE les evts DANS l'intervalle     */
//		char bruit_inv;        /* vrai si la coupure REJETTE les evts DANS l'intervalle     */
		float minampl;         /* amplitude minimum                                         */
        char signe;            /* signe de l'evenement attendu (0: les deux sens)           */
        short mincount;        /* nb coups horloge min pour front (d'apres 'montee')        */
        short mindelai;        /* nb coups horloge min pour porte (d'apres 'porte')         */
        int  inhibe;           /* points stockes */
		int  sautlong;         /* points stockes */
   } trig;                 /* variables auxilaires pour le du trigger                      */
	struct {                /* dernier signal (evt..) trouve sur cette voie                 */
		int64 analyse;         /* premier point absolu a analyser                           */
		short climbs;          /* nombre de points de meme derivee (~= temps de montee)     */
		char mesure;           /* vrai si somme et carre recalcules                         */
		char genere_evt;       /* vrai si evenement a simuler                               */
		float base;            /* base du pic (== sommet du pic precedent)                  */
//		TypeDonnee courante;   /* valeur courante (utilisee si trmt pre-trigger)            */
		int max;               /* longueur de l'intervalle de lissage                       */
		int nb;                /* nombre de points exploitables dans icelui                 */
		double somme;          /* somme du signal toujours dans le susdit                   */
		double carre;          /* somme du carre du encore le meme					        */
//		float moyenne;         /* moyenne dans ces conditions                               */
		float bruit;           /* bruit correspondant                                       */
		float periode;         /* duree moyenne entre evts si TRGR_ALEAT en nb.pts			*/
		int64 futur_evt;       /* date du debut du futur evt si TRGR_ALEAT			        */
	} signal;
	struct {                /* gestion de l'evenement moyen                                 */
		char calcule;          /* il est a calculer           */
		int nb;                /* nombre d'evenements cumules */
		float decal;           /* decalage relatif en ms      */
	} moyen;
	TypeEvtTemplate somme;  /* valeur en cours                                              */
	TypeEvtTemplate unite;  /* valeur de reference                                          */
	float norme_evt;        /* normalisation du fit (1/denominateur dans l'ajustement)      */
	struct {
		int    dim;             /* dimension demandee            */
		int    max;             /* taille effective              */
		int    total;           /* nombre de points a surveiller */
		float *val;             /* tampon du pattern courant     */
		float *nb;              /* nombre de valeurs sommees     */
	} pattern;              /* valeur en cours                                              */
	TypePattern pattref;    /* valeur de reference                                          */
	float taux;             /* pour affichage                                               */
	int64 date_dernier;     /* idem                                                         */
	int64 date_ref_taux;    /* idem                                                         */
	int nbevts;             /* idem                                                         */
	int dernier_evt;        /* idem                                                         */
	int evt_affiche;        /* idem                                                         */
	struct {
		int64 futur_evt;         /* date du debut du futur evt si TRGR_ALEAT			      */
		float max;               /* maximum attendu                                           */
		int etape;               /* stamp en cours (peut etre a cheval sur plusieurs trames)  */
		TypeADU modul,compens;   /* cas ou la voie est modulee                                */
		short demiperiode,phase; /* cas ou la voie est modulee                                */
		float facteurC;          /* cas ou la voie est modulee                                */
		char manu;               /* si 1 evt demande                                          */
	} gene;
#define MAXREGULINTERV 16
	struct {
//		TypeRegulParms parm;
		struct {
//			TypeRegulTaux echelle[MAXECHELLES];
			long prochaine[MAXECHELLES];
			int nbevts[MAXECHELLES];
		} taux;
		struct {
//			TypeRegulInterv parm;
			float min[MAXREGULINTERV];  /* valeur du max du signal sur chaque intervalle     */
			float max[MAXREGULINTERV];  /* valeur du min du signal sur chaque intervalle     */
			int32 limp,limm;            /* limite pour depassement positif,negatif           */
			char evtp[MAXREGULINTERV];  /* vrai si depassement positif                       */
			char evtm[MAXREGULINTERV];  /* vrai si depassement negatif                       */
			float moyp,moym;            /* somme des max,min                                 */
			float nbp,nbm;              /* nb intervalles ayant contribue                    */
			float derp,derm;            /* derniere valeur du max,min (a sortir de la somme) */
			int ovrp,ovrm;              /* nb d'intervalles consecutifs avec depassement     */
			int lngr,used;              /* longueur intervalle (pts stockes), dernier intervalle utilise */
		} interv;
	} rgul;
	float *ntp[SAMBA_NTPVOIE_NB];
	int min,max;            /* mini et maxi du signal recu (ADU)                            */
} TypeVoieTampons;

#define MAX_EMPILES 32
typedef struct {
	int   trig;   /* voie qui a declenche */
	float niveau;
	int64 date;   /* pts absolus */
	int64 debut;  /* pts absolus */
	int64 fin;    /* pts absolus */
	int   delai;  /* pts absolus */
	short  origine;
	short nbvoies;  /* nombre de voies dans l'evenement a suivre */
	struct {
		short indx; /* index dans VoieManip                      */
		short lngr; /* longueur de l'evenement                   */
		short lus;  /* nb. donnees deja lues                     */
		int   adrs; /* adresse du premier point dans VoieTampon  */
	} voie[REPART_VOIES_MAX]; /* liste des voies */
} TypeEvtDetecte;
SAMBA_VAR TypeEvtDetecte EvtStocke[MAX_EMPILES];

#define MAXPICS 5
typedef struct {
	TypeDonnee min,max; /* intervalle de recherche du pic */
	float *t,*position,*sigma;
	int prems,nb,dim;
} TypePic;

SAMBA_VAR TypePic Pic[MAXPICS];
SAMBA_VAR int PicsNb;
SAMBA_VAR char PicsActif;
SAMBA_VAR int PicSuiviDim;
SAMBA_VAR float *PicX,*PicY,*PicT;
SAMBA_VAR char *PicV;
SAMBA_VAR int PicsDim; /* nombre de points de suivi (= dimension des 3 tableaux precedents) */
SAMBA_VAR Graph gPicsPositions,gPicsSigmas;

/* ............................... Variables ................................ */
extern char *SambaVersion,*SambaCompile;

typedef struct {
	int nb_depile;
	struct {
		TypeDonnee *prochain;   /* adresse de la prochaine donnee brute */
		int nb;                 /* quantite utilisable */
		int prem;               /* pointeur de debut   */
	} voie[VOIES_MAX];
} TypeSambaPartagee;
SAMBA_VAR TypeSambaPartagee *SambaPartage;
SAMBA_VAR int SambaPartageId;
SAMBA_VAR int SambaPartageDim;

SAMBA_VAR char XcodeSambaDebug;
SAMBA_VAR char SambaJournal[80];
SAMBA_VAR char RunJournal[MAXFILENAME];
SAMBA_VAR char StartFromScratch,CreationFichiers,EdbActive;
SAMBA_VAR char EdbServeur[HOTE_NOM];
SAMBA_VAR char NomOrdiLocal[HOTE_NOM];
SAMBA_VAR size_t SambaDefMax,SambaDefInitial;
#undef FILESEP
#define FILESEP RepertoireDelim
SAMBA_VAR char  SambaInfoDir[MAXFILENAME];
SAMBA_VAR char  PrefPath[MAXFILENAME],MaitreDir[MAXFILENAME];
SAMBA_VAR char  ArchDir[ARCH_TYPEDATA][MAXFILENAME];
SAMBA_VAR char  ArchRun[MAXFILENAME];
SAMBA_VAR int   ArchPath;
SAMBA_VAR char  ArchPartition;
SAMBA_VAR char  NomSequences[MAXFILENAME];
SAMBA_VAR char  BBstsPath[MAXFILENAME];
SAMBA_VAR char *BBstsDir
#ifdef SAMBA_C
 = "monitoring/StatusBB/"
#endif
;
SAMBA_VAR char  HelpPath[MAXFILENAME];
SAMBA_VAR char  SambaMode;
SAMBA_VAR char  SambaChassisOptimal,SambaExploreHW,SambaIdentSauvable,SettingsSauveChoix,SettingsTramesMaxi;
SAMBA_VAR char  RunDateDebut[32];
SAMBA_VAR int64 LectTimeStamp0,LectTimeStampN,LectStampMini;
SAMBA_VAR int   LectRepartAttendus;
SAMBA_VAR char  LectArretExterieur;
SAMBA_VAR int   SambaDonneeMin,SambaDonneeMax;

SAMBA_VAR int   CpuActifs;
SAMBA_VAR char  PCIconnecte;     /* vrai si une carte PCI est accessible (anciennement PCIadrs[PCInum] != -1)               */
SAMBA_VAR char  PCIdemande;      /* vrai si on veut utiliser l'acces PCI (== PCIadrs[PCInum] represente bien une carte PCI) */
SAMBA_VAR char  IPdemande;       /* vrai si au moins un numeriseur a utilise une liaison internet                           */
SAMBA_VAR char  IPactifs;        /* vrai si au moins un numeriseur utilise encore une liaison internet                      */
SAMBA_VAR char  PasMaitre,Demarreur;
SAMBA_VAR int   PortLectRep,PortLectSat;
SAMBA_VAR char  LoggerType;
SAMBA_VAR char  LoggerAdrs[MAXFILENAME],LoggerUser[1024];
SAMBA_VAR char  LoggerCmde[1024];
#ifdef MODULES_RESEAU
	SAMBA_VAR int   PortSvr;
#endif

typedef enum {
	SAMBA_DICO_REGLAGES,
	SAMBA_DICO_VOIES,
	SAMBA_DICO_NB
} SAMBA_DICO_SUJET;

#define MAXSAMBATERME 16
typedef struct {
	char accepte[MAXSAMBATERME];
	char officiel[MAXSAMBATERME];
} TypeSambaTerme;

#define MAXSAMBALEXIQUE 64
typedef struct {
	char nom[MAXDICONOM];
	int nbtermes;
	TypeSambaTerme terme[MAXSAMBALEXIQUE];
} TypeSambaDico;

SAMBA_VAR TypeSambaTerme SambaTermeTempo;
SAMBA_VAR TypeSambaDico SambaDicoTempo;
SAMBA_VAR char *SambaDicoNom[SAMBA_DICO_NB+1]
#ifdef SAMBA_C
 = { "reglages", "voies", "\0" }
#endif
;
// SAMBA_VAR TypeDico SambaDicoReglages,SambaDicoVoies;
SAMBA_VAR TypeDico SambaDicoDefs[SAMBA_DICO_NB];
SAMBA_VAR char *SambaSelectionReponses[]
#ifdef SAMBA_C
 = { "Annuler", "Utiliser", "\0" }
#endif
;
SAMBA_VAR char **SambaSelectionText;

SAMBA_VAR char *LectViaNom[CARTE_UNKN+2]
#ifdef SAMBA_C
 = { "AB", "MiG", "ICA", "inconnue", "\0" }
#endif
;

SAMBA_VAR char *SambaBooleen[]
#ifdef SAMBA_C
 = { "Non", "Oui", "\0" }
#endif
;
SAMBA_VAR char **SambaNonOui;
SAMBA_VAR char *SambaNonSi[]
#ifdef SAMBA_C
= { "Non, c'est sur", "Oups! on arrete..", "\0" }
#endif
;

typedef enum {
	EDITLIST_EFFACE = 0,
	EDITLIST_INSERE,
	EDITLIST_GARDE,
	EDITLIST_NBACTIONS
} EDITLIST_ACTION;
#define EDITLIST_ACTION_CLES "retirer/inserer/garder"

SAMBA_VAR struct {
	struct { char hw,sw,elec; } config;
	char initialisation,modeles;
	struct {
		char fichier,assoc,compte,maint,classmt,perimetre;
	} detectr;
	struct { char capa,select; } repart;
	struct { char sauveg,classmt; } numer;
	struct { char repart,numer,detectr; } cablage;
	struct { char locales,composition; } voies;
	char reseau;
	char vars_manip;
	struct { char liste; } media;
} CompteRendu;
SAMBA_VAR Panel pSettingsCompteRendu;
SAMBA_VAR ArgDesc SambaCrDesc[]
#ifdef SAMBA_C
 = {
	{ "ordi.hardware",          DESC_BOOL &CompteRendu.config.hw,         "Configuration materielle de la machine d'acquisition" },
	{ "ordi.software",          DESC_BOOL &CompteRendu.config.sw,         "Configuration logicielle de la machine d'acquisition" },
	{ "electronique",           DESC_BOOL &CompteRendu.config.elec,       "Configuration de l'electronique d'acquisition" },
	{ "initialisation",         DESC_BOOL &CompteRendu.initialisation,    "Phases de l'initialisation" },
	{ "modeles",                DESC_BOOL &CompteRendu.modeles,           "Examen des modeles" },
	{ "reseau",                 DESC_BOOL &CompteRendu.reseau,            "Aquisitions connues" },
	{ "repartiteurs.capacite",  DESC_BOOL &CompteRendu.repart.capa,       "Capacite de transfert des repartiteurs" },
	{ "repartiteurs.selecteur", DESC_BOOL &CompteRendu.repart.select,     "Mode de selection de voies des repartiteurs" },
	{ "detecteurs.fichier",     DESC_BOOL &CompteRendu.detectr.fichier,   "Fichier de definition pour chaque detecteur" },
	{ "detecteurs.associes",    DESC_BOOL &CompteRendu.detectr.assoc,     "Detecteurs associes pour chaque detecteur" },
	{ "detecteurs.compte",      DESC_BOOL &CompteRendu.detectr.compte,    "Comptage des detecteurs" },
	{ "detecteurs.maintenance", DESC_BOOL &CompteRendu.detectr.maint,     "Script de maintenance pour chaque detecteur" },
	{ "detecteurs.classement",  DESC_BOOL &CompteRendu.detectr.classmt,   "Classement initial des detecteurs" },
	{ "detecteurs.perimetre",   DESC_BOOL &CompteRendu.detectr.perimetre, "Liste des detecteurs avec mention local/exterieur" },
	{ "numeriseurs.sauvegarde", DESC_BOOL &CompteRendu.numer.sauveg,      "Ressources des numeriseurs a sauver" },
	{ "numeriseurs.classement", DESC_BOOL &CompteRendu.numer.classmt,     "Classement initial des numeriseurs" },
	{ "cablage.repartiteurs",   DESC_BOOL &CompteRendu.cablage.repart,    "Cablage des repartiteurs" },
	{ "cablage.numeriseurs",    DESC_BOOL &CompteRendu.cablage.numer,     "Cablage des numeriseurs" },
	{ "cablage.detecteurs",     DESC_BOOL &CompteRendu.cablage.detectr,   "Cablage des detecteurs" },
	{ "voies.locales",          DESC_BOOL &CompteRendu.voies.locales,     "Liste des voies locales" },
	{ "voies.composition",      DESC_BOOL &CompteRendu.voies.composition, "Composition des pseudo voies (si voies.locales uniquement)" },
	{ "vars_manip",             DESC_BOOL &CompteRendu.vars_manip,        "Liste des variables du montage experimental a utiliser" },
	{ "media.liste",            DESC_BOOL &CompteRendu.media.liste,       "Liste des media lus ou crees" },
	DESC_END
}
#endif
;

typedef enum {
	CLASSE_NUMER = 0,
	CLASSE_DETEC,
	CLASSE_NBOBJETS
} CLASSE_OBJET;
SAMBA_VAR char *ClasseObjet[CLASSE_NBOBJETS+1]
#ifdef SAMBA_C
 = { "numeriseurs", "detecteurs", "\0" }
#endif
;
typedef enum {
	CLASSE_NOM = 0,
	CLASSE_ENTREE,
	CLASSE_POSITION,
	CLASSE_POSY,
	CLASSE_FISCHER,
	CLASSE_FIBRE,
	CLASSE_NEANT,
	CLASSE_INFONB
} CLASSE_INFO;
SAMBA_VAR char *ClasseInfo[CLASSE_INFONB+1]
#ifdef SAMBA_C
 = { "nom", "entree", "position", "tour", "fischer", "fibre", "actuelle", "\0" }
#endif
;
typedef enum {
	CLASSE_TEXTE = 0,
	CLASSE_ENTIER,
	CLASSE_INDEFINIE
} CLASSE_TYPE;
SAMBA_VAR int  ClassementDemande[CLASSE_NBOBJETS],ClassementCourant[CLASSE_NBOBJETS];
SAMBA_VAR char ClasseObjetATrier,ClasseCritereDeTri;

SAMBA_VAR char  SambaSrceUtilisee;
SAMBA_VAR char  LectSurFichier;
SAMBA_VAR int   LectSession;
SAMBA_VAR long  LectRunTime,LectRunStop;
SAMBA_VAR char  CtlgPath[MAXFILENAME],FichierCatalogue[MAXFILENAME];
SAMBA_VAR FILE *LectArchFile; SAMBA_VAR int LectArchPath; SAMBA_VAR char LectSurFichierDejaVu;
SAMBA_VAR int   LectBitD2D3;
#ifdef CODE_WARRIOR_VSN
	SAMBA_VAR int64 LectArchData;       /* position dans le fichier du debut des donnees */
#else
	SAMBA_VAR off_t LectArchData;       /* position dans le fichier du debut des donnees */
#endif
SAMBA_VAR TypePciBoard PCIacces[PCI_MAX];
SAMBA_VAR TypeCartePci PCIedw[PCI_MAX];
SAMBA_VAR int PCInb;
SAMBA_VAR int PCInum;
SAMBA_VAR int NumDefaut,IPserial,BNserial;
SAMBA_VAR unsigned long SimulPCI[4];
SAMBA_VAR int RepartFile;
SAMBA_VAR unsigned long *FifoPrecedente;
SAMBA_VAR int32 FifoTopPrec;
SAMBA_VAR int DebugRestant;

SAMBA_VAR TypeMediaListe SambaMedia[MEDIA_MAX];

#ifdef BRANCHE_TESTS
	/* Ensemble destine a simuler des evenements par poussoir */
	/* Depasse par le Generateur                              */
	int SambaCalculeTemplates();
	void mc_init(float energie, float resolution);       /* voir gener.c */
	#define MAXTEMPLATE 256
	SAMBA_VAR int   VoieTouchee,BoloTouche;
	SAMBA_VAR float Template[VOIES_TYPES][MAXTEMPLATE];  /* evenement normalise a 1.0            */
	SAMBA_VAR int   TemplDim[VOIES_TYPES];               /* dimension dudit evenement            */
	SAMBA_VAR int   TemplMontee[VOIES_TYPES];            /* nombre de points de la montee        */
	SAMBA_VAR int   TemplDescente[VOIES_TYPES];          /* nombre de points de la descente      */
	SAMBA_VAR int   TemplTemps;                          /* temps local a l'evenement            */
	SAMBA_VAR float AmplDeposee[VOIES_TYPES];            /* max evt en ADU (sera * par Template) */
	SAMBA_VAR int   EvtGenere;                           /* numero d'evenement genere            */
	SAMBA_VAR float TestEnergie,TestResolution;          /* mc_init(TestEnergie,TestResolution); */
	SAMBA_VAR Cadre bTests;
	#ifdef AJOUTE_COSINUS
		static float AmplitudeCosinus,PeriodeCosinus; /* ADU, ms */
		static TypeDonnee LectTrmtInitValTraitee,LectTrmtInitValBrute;
	#endif
#endif /* BRANCHE_TESTS */

SAMBA_VAR char  PetitEcran,ExpertConnecte,ImprimeConfigs,RegenEnCours,RelaisFermes,MiniStream;
SAMBA_VAR int   ExecAn,ExecMois,ExecJour,ExecHeure,ExecMin,ExecSec;
SAMBA_VAR char  RunDateBanc[8],RunTypeLu[8];
SAMBA_VAR char  FichierRunPrecedent[MAXFILENAME];
SAMBA_VAR char  DateCourante[8];
SAMBA_VAR float gusecToKgJ;      /* Conversion de (grammes x microsecondes) en (Kg.jour) */
SAMBA_VAR float ValeurADU;       /* Valeur courante d'1 ADU (volts, keV, ..)             */
SAMBA_VAR float EnSecondes;      /* Temps d'un echantillon en secondes (pour les evaluations de temps) */
SAMBA_VAR int64 DernierPointTraite;
SAMBA_VAR int64 LectDerniereD3;  /* dernier numero de bloc portant la synchro D3          */
SAMBA_VAR char  TrmtApanique,LectAffichePanique;
SAMBA_VAR int   ArchEcrites[ARCH_TYPEDATA];     /* evenements ou donnees sauves dans la derniere tranche */
SAMBA_VAR int   VoieEnReglage;
#define MAXUSERNUM 256
SAMBA_VAR char  SambaUserNum[MAXUSERNUM];

SAMBA_VAR Panel pLectConditions,pLectConfigRun,pRunParms;

SAMBA_VAR char *TrmtListe[TRMT_NBTYPES+FLTR_MAX+1];
SAMBA_VAR int   EvtSupprimes;
SAMBA_VAR int   PointsEvt;

SAMBA_VAR HistoDeVoie *VoieHisto;        /* POINTEUR sur les histos a remplir */
SAMBA_VAR TypeVoieTampons *VoieTampon;
SAMBA_VAR int VoieIdentMax;
SAMBA_VAR TypeVoieIdentifiee VoieIdent[VOIES_MAX];

SAMBA_VAR char NomParms[MAXFILENAME];
SAMBA_VAR char ConfigGeneral[MAXFILENAME];
SAMBA_VAR char FichierPrefModeles[MAXFILENAME],FichierPrefMedia[MAXFILENAME],FichierPrefGene[MAXFILENAME];
SAMBA_VAR char FichierCrParms[MAXFILENAME],FichierPrefParms[MAXFILENAME],FichierPrefDico[MAXFILENAME];
SAMBA_VAR char FichierPrefDetecteurs[MAXFILENAME],FichierPrefRepartiteurs[MAXFILENAME];
// SAMBA_VAR char FichierListeDet[MAXFILENAME];
SAMBA_VAR char FichierPrefProcedures[MAXFILENAME],FichierPrefFiltres[MAXFILENAME],FichierPrefSequences[MAXFILENAME],FichierPrefRegulEvt[MAXFILENAME];
SAMBA_VAR char FichierPrefCablage[MAXFILENAME],FichierPrefNumeriseurs[MAXFILENAME];
SAMBA_VAR char FichierPrefVi[MAXFILENAME],FichierPrefEvtUnite[MAXFILENAME];
SAMBA_VAR char FichierEtatElec[MAXFILENAME],FichierRunEnvir[MAXFILENAME],FichierRunCaract[MAXFILENAME],FichierRunComment[MAXFILENAME];
SAMBA_VAR char FichierCaldrRef[MAXFILENAME];
SAMBA_VAR char FichierPrefLecture[MAXFILENAME],FichierPrefMonit[MAXFILENAME],FichierPrefTagDefs[MAXFILENAME],FichierPrefMonitCatg[MAXFILENAME];
SAMBA_VAR char FichierPrefAutom[MAXFILENAME],FichierPrefVerif[MAXFILENAME],FichierPrefReseau[MAXFILENAME],FichierPrefSatellites[MAXFILENAME];
SAMBA_VAR char FichierPrefCalcul[MAXFILENAME],FichierPrefFinesses[MAXFILENAME],FichierPrefExports[MAXFILENAME],FichierPrefSelection[MAXFILENAME];
SAMBA_VAR char FichierPrefBasic[MAXFILENAME],FichierPrefBanc[MAXFILENAME];
SAMBA_VAR char MonitFenFichier[MAXFILENAME];
SAMBA_VAR char FichierScriptStart[MAXFILENAME],FichierScriptStop[MAXFILENAME];
SAMBA_VAR char FichierEntretienStart[MAXFILENAME],FichierEntretienStop[MAXFILENAME];
SAMBA_VAR char FichierRegenStart[MAXFILENAME],FichierRegenStop[MAXFILENAME];
SAMBA_VAR char NomScriptStart[MAXFILENAME],NomScriptStop[MAXFILENAME];
SAMBA_VAR char NomEntretienStart[MAXFILENAME],NomEntretienStop[MAXFILENAME];
SAMBA_VAR char NomRegenStart[MAXFILENAME],NomRegenStop[MAXFILENAME];
SAMBA_VAR char CalendrierNom[MODL_NOM],CalendrierRun[MODL_NOM];
#define MAXRUNS 256 
SAMBA_VAR TypeCaldrEvt SambaRunPrevu[MAXRUNS]; SAMBA_VAR int SambaRunsNb;
SAMBA_VAR int  SambaRunCourant;

SAMBA_VAR char ConfigNom[CONFIG_TOT+1][CONFIG_NOM],*ConfigListe[CONFIG_TOT+2],*ConfigCle[CONFIG_TOT+2];
SAMBA_VAR char ConfigNouv[CONFIG_NOM];
SAMBA_VAR int  ConfigNb,ConfigNum,ConfigAvant;

SAMBA_VAR char FichierModlDetecteurs[MAXFILENAME];
SAMBA_VAR char FichierModlNumeriseurs[MAXFILENAME];
SAMBA_VAR char FichierModlRepartiteurs[MAXFILENAME];
SAMBA_VAR char FichierModlCablage[MAXFILENAME];
SAMBA_VAR char FichierModlChassis[MAXFILENAME];
SAMBA_VAR char FichierModlEnvir[MAXFILENAME];

SAMBA_VAR char *SambaProcedure[SAMBA_PROC_MAX];
SAMBA_VAR int  SambaProcsNb,SambaProcsNum;

SAMBA_VAR TypeClassement OrdreNumer[NUMER_MAX],OrdreDetec[DETEC_MAX];

SAMBA_VAR int   SettingsRetard,SettingsFibreLngr;
SAMBA_VAR int   SettingsDate;

SAMBA_VAR int   SettingsPartition;
SAMBA_VAR int   SettingsTauxPeriode;  /* Periode de calcul du taux d'evenement (s), peut etre == 0 si instantanne */
SAMBA_VAR int   SettingsRegenDelai;   /* Duree mini pour reprise apres regen (s)          */
SAMBA_VAR int   SettingsRazNtdDuree;  /* Duree du RAZ des FET NTD (s)                     */
SAMBA_VAR int   SettingsRazNbSiDuree; /* Duree du RAZ des FET NbSi apres polarisation (s) */
SAMBA_VAR int   SettingsIntervTrmt,SettingsIntervSeq,SettingsIntervUser;
SAMBA_VAR int   SettingsMaxSuivi;
SAMBA_VAR char  SettingsPauseEvt,CalcNtpDemande,CalcNtpAffiche,SettingsStockeFiltrees; // SettingsAttendsSeuil
SAMBA_VAR char  SettingsLectureGlobale;
SAMBA_VAR char  SettingsSecuPolar;
SAMBA_VAR char  ArchSauveTrace;
SAMBA_VAR int   SettingsNtupleMax;
SAMBA_VAR int   SettingsRefCalage;
SAMBA_VAR char  ArchSauveNtp;
SAMBA_VAR char  SettingsHdrGlobal;
SAMBA_VAR int   SettingsHistoVoie;
SAMBA_VAR float SettingsAmplMin,SettingsAmplMax,SettingsMonteeMin,SettingsMonteeMax;
SAMBA_VAR int   SettingsAmplBins,SettingsMonteeBins;
SAMBA_VAR int   SettingsFIFOwait;       /* limite d'acces a la FIFO si vide    */ /* =150 */
SAMBA_VAR int   SettingsIPEwait,SettingsD3wait;
SAMBA_VAR float SettingsDelaiIncertitude;
SAMBA_VAR Menu  mSettingsTags;
SAMBA_VAR Panel pSettingsModes,pSettingsTrigger,pLectRegen,pLectEvtNum,pLectEvtQui,pSettingsDate;

#define SETTINGS_MAXNOM 32
typedef enum {
	SETTINGS_TRMT = 0,
	SETTINGS_TRGR,
//	SETTINGS_COUP,
	SETTINGS_RGUL,
	SETTINGS_CATG,
	SETTINGS_NBVOLETS
} SETTINGS_VOLET;
//- static char *SettingsTypes[SETTINGS_NBVOLETS+1] = { "trmt", "trgr", "coup", "rgul", "catg", "\0" };
// static char *SettingsTypes[SETTINGS_NBVOLETS+1] = { "trmt", "trgr", "rgul", "catg", "\0" };
SAMBA_VAR char *SettingsVoletTitre[SETTINGS_NBVOLETS+1]
#ifdef SAMBA_C
 = { "traitement", "trigger", "regulation", "categorie", "\0" }
#endif
;

SAMBA_VAR struct {
	int bolo,cap;
	char standard_a_jour;
	Cadre planche;
	MenuItem general_std,general_det;
	Menu general; Panel config;
	Menu menu_voie[DETEC_CAPT_MAX];
	int (*creation)(Panel appelant, void *v2);
	int (*standardise)(Panel panel, void *v2);
	int (*standard2)(Panel panel, void *v2);
} SettingsVolet[SETTINGS_NBVOLETS];


SAMBA_VAR char  RunCategNum,SettingsRunFamille,SettingsVoiesNom,SettingsImprBolosVsn;
SAMBA_VAR char  SettingsInlineProcess,SettingsGenePossible,SettingsDeconvPossible;
SAMBA_VAR Panel pEtatBolos;
SAMBA_VAR char  NumeriseurListeExiste,DetecteurListeExiste;

SAMBA_VAR char  RegenDebutJour[10],RegenDebutHeure[10];
SAMBA_VAR char  RelaisDebutJour[10],RelaisDebutHeure[10];

SAMBA_VAR char  VerifEtatFinal,VerifRepertIP,VerifIdentBB,VerifIdentRepart,VerifCompens,VerifDisplay,VerifpTauxEvts;
SAMBA_VAR char  VerifMonitEvts,VerifCalculTauxEvts,VerifBittrigger,VerifIntegrales,VerifEvtMoyen,VerifOctetsStream;
SAMBA_VAR char  VerifCalculEvts,VerifSuiviEtatEvts,VerifSuiviTrmtEvts,VerifRunDebug,VerifCpuCalcul,VerifCalculDetail;
SAMBA_VAR char  VerifTempsPasse,VerifConsoCpu;
SAMBA_VAR char  GestionConfigs;
SAMBA_VAR char  SettingsParmsASauver;

SAMBA_VAR char  LectIdentFaite;     /* identification des numeriseurs deja faite */
SAMBA_VAR char  LectSourceActive;   /* etat de la source de regeneration         */
SAMBA_VAR char  SettingsRegen;
#ifdef EDW_ORIGINE
	SAMBA_VAR char  AutomRegenNom[AUTOM_REGEN_MAX][16],AutomCalibNom[AUTOM_CALIB_MAX][16]; /* en fait, MAXVARNOM */
#endif /* EDW_ORIGINE */

typedef enum {
	LECT_FROM_ELEM = 1,
	LECT_FROM_STD,
	LECT_FROM_KILL,
	LECT_FROM_NBMODES
} LECT_FROM_MODE;
SAMBA_VAR char LectAcqLanceur;

SAMBA_VAR Timer LectTaskReadout;
SAMBA_VAR int   SambaLecturePid;
SAMBA_VAR char  LectAcqCollective;
SAMBA_VAR char  LectChangeParms;    /* affiche le panel qui permet de modifier l'environnement */
SAMBA_VAR char  LectDecodeStatus;   /* se donne la peine de decoder le status V2 (inutile depuis que OPERA le fait) */
SAMBA_VAR char  LectModeStatus;     /* ne decode que le status                         */
SAMBA_VAR char  LectLitStatus;      /* recopie les status des BO                       */
SAMBA_VAR char  LectSauveStatus;    /* sauve les status des BB                         */
SAMBA_VAR char  LectStatusDispo;    /* un status de BB reste a sauver                  */
SAMBA_VAR char  LectRetardOption;   /* vrai pour afficher le retardateur               */
SAMBA_VAR char  LectStoppeRepart;   /* vrai pour arreter les repartiteurs sur stop run */
SAMBA_VAR int64 LectNextSeq;

SAMBA_VAR float CompensationApprox;
SAMBA_VAR PlotEspace EvtEspace;
SAMBA_VAR Panel pEtatDetecteurs;
SAMBA_VAR Panel pReglagesConsignes[LECT_CONS_NB];
SAMBA_VAR Graph gDonnee[VOIES_MAX];
#ifdef REGUL_UNIQUE
	SAMBA_VAR int64 LectDerniereRegul[MAXECHELLES];
#endif
SAMBA_VAR Panel pTauxDetaille; SAMBA_VAR Graph gTauxDetaille;
SAMBA_VAR Graph gSeuils;
SAMBA_VAR Graph gEvtMoyen,gPattern;
//- SAMBA_VAR int64 TrmtLastEvt;

SAMBA_VAR int   LectTrancheRelue;   /* numero de tranche en relecture            */
SAMBA_VAR int64 LectIntervTrmt,LectIntervExport[EXPORT_MAXPACKS],LectIntervData;
SAMBA_VAR int64 LectNextTrmt,LectNextExport[EXPORT_MAXPACKS];
SAMBA_VAR int64 SynchroD2traitees,SynchroD2lues,LectNextDisp; // utilise dans traimt.c pour diagnostics
SAMBA_VAR char  LectTrmtSousIt;

SAMBA_VAR HistoData hdBruitAmpl,hdEvtAmpl,hdBruitMontee,hdEvtMontee;
SAMBA_VAR H2D h2D;
SAMBA_VAR Graph gEvtSolo,gEvtRunCtrl,gBruitAmpl,gBruitMontee,g2D;
SAMBA_VAR float MomentEvt[5];          /* abscisse marquage evt  */
SAMBA_VAR short AmplitudeEvt[5];       /* ordonnees marquage evt */

SAMBA_VAR int64 SynchroD2_us;          /* duree d'une synchronisation D2 en us              */
SAMBA_VAR float SynchroD2_kHz;         /* frequence d'une synchronisation en kHz            */
SAMBA_VAR int64 BlocsParD3;            /* nombre de blocs par synchronisation BlocsParD3    */
SAMBA_VAR int64 LectStampsLus;         /* nombre total de blocs (ou trames) lus sur FIFO    */
SAMBA_VAR int64 LectStampPrecedents;   /* idem, sommes depuis demarrage utilisateur         */
SAMBA_VAR int64 LectStampPerdus;       /* nombre de blocs perdus lors de la transmission    */
SAMBA_VAR int64 LectStampJetes;        /* nombre de blocs jetes pour non-traitement         */
SAMBA_VAR int64 LectStampSautes;       /* nombre d'echantillons sautes apres trigger        */
SAMBA_VAR int64 LectValZappees;        /* nombre d'acces FIFO jetes pour non-sauvegarde     */
SAMBA_VAR int64 LectSyncRead;          /* desynchronisation a la lecture de la FIFO         */
SAMBA_VAR int64 ArchSyncWrite;         /* desynchronisation a l'ecriture sur fichier        */
SAMBA_VAR int64 LectArretProgramme;    /* timestamp limite pour arret sur erreur            */
// SAMBA_VAR TypeADU LectErreurCode;      /* Erreur rencontree (lecture en parallele)          */
SAMBA_VAR int   LectAttenteErreur;     /* temps d'attente en cas de relance sur erreur      */
SAMBA_VAR int64 LectDebutEvts;         /* date du premier evenement a sauver                */
SAMBA_VAR int64 LectT0Run;             /* datation debut de lecture                         */
SAMBA_VAR int64 LectTexec;             /* cumul temps de lecture                            */
SAMBA_VAR struct timeval synchro1,synchroN;
SAMBA_VAR int   LectSignalAmpl,LectSignalZero;
SAMBA_VAR float LectSignalTemps;
SAMBA_VAR int64 TempsTotalLect,TrmtNbLus,ArchNbLus;
SAMBA_VAR int64 TrmtTempsEvt,TrmtPasActif,TrmtTempsArchive,TrmtTempsFiltrage;
SAMBA_VAR int64 TempsTotalTrmt,TempsTotalArch;
SAMBA_VAR int64 TempsTempoTrmt,TempsTempoArch;
SAMBA_VAR clock_t CpuTempoTrmt,CpuTempoArch;
SAMBA_VAR clock_t CpuTotalTrmt,CpuTotalArch;
SAMBA_VAR int64 CpuMaxTrigger,CpuMaxEvt;
SAMBA_VAR int64 CpuStampTrigger,CpuFinTrigger,CpuStampEvt,CpuFinEvt;
SAMBA_VAR char  TrmtAttendComplet;
SAMBA_VAR int   PileUp,TrmtEvtJetes,EvtNbCoupes,EvtNbIncoherents;
SAMBA_VAR int   EvtsEmpiles;
SAMBA_VAR char  BoloTrigge[DETEC_NOM],VoieTriggee[MODL_NOM];
SAMBA_VAR char  TempsTrigger[16];
//- SAMBA_VAR float BnAuxiDuree,BnAuxiPretrig;
SAMBA_VAR float TrmtRegulAttendu;
//+ #ifdef REGUL_UNIQUE
SAMBA_VAR TypeRegulTaux TrmtRegulTaux[MAXECHELLES];
SAMBA_VAR TypeRegulInterv TrmtRegulInterv;
//+ endif
SAMBA_VAR struct {
	lhex  valeur;                 /* valeur deja recuperee                              */
	lhex  precedente;             /* precedente <valeur>                                */
	short echantillon;            /* rang de la donnee dans l'echantillon               */
	short rep;                    /* numero de repartiteur ayant delivre la valeur      */
	char  dispo;                  /* vrai si <valeur> est utilisable                    */
	char  is_status;              /* vrai si <valeur> est le status                     */
} LectEnCours;
#define ERREUR_FCTN 32
#define ERREUR_FILE 80
SAMBA_VAR struct {
	char nom[ERREUR_FCTN];
	char fic[ERREUR_FILE];
	int ligne;
	int num;
	TypeADU code;
} LectErreur;
SAMBA_VAR char  LectureActive;         /* vrai si lecture en cours                          */
SAMBA_VAR char  LecturePourRepart;     /* vrai si lecture pour fenetre repartiteur          */
SAMBA_VAR char  LectModeSauvegarde;
SAMBA_VAR char  LectCompacteUtilisee;
SAMBA_VAR char  LectureLog;            /* vrai si compte-rendus de demarrage/arret a faire  */
SAMBA_VAR char  LectReglagesOuverts;
SAMBA_VAR char  LectModeSpectresAuto;
SAMBA_VAR int   LectModeSpectresBanc;
//SAMBA_VAR char  TriggerDemande;        /* vrai si trigger demande                           */
SAMBA_VAR int   LectDureeMaxi,LectDureeTranche;
SAMBA_VAR int   LectTrancheNum[ARCH_TYPEDATA],LectRegenNum,LectStreamNum;
SAMBA_VAR char  LectAutoriseSequences; /* pour controler globalement le sequencement        */
SAMBA_VAR int   LectDepileWait;        /* temps d'attente de LectDepileBoost() si it  (usec) */
SAMBA_VAR char  LectDureeActive;       /* vrai si limitation de la lecture                  */
SAMBA_VAR char  LectDepileInhibe;      /* suspend la lecture sur elle est sous interruption */
SAMBA_VAR char  LectDepileEnCours;
SAMBA_VAR int   LectDepileNb;
SAMBA_VAR int   LectDepileTestees;
SAMBA_VAR int64 LectDepileAttente;     /* duree entre les deux derniers appels a LectDepile */
SAMBA_VAR int64 LectDepileDuree;       /* duree du dernier appel a LectDepile               */
SAMBA_VAR int64 LectDepileTfin;        /* date de fin du dernier appel a LectDepile         */
SAMBA_VAR char  LectLogComp;           /* Traitmt ecrit un C.R. pdt la compensation         */
SAMBA_VAR char  LectStampSpecifique,LectDemarreBolo,LectEntretien,LectEntretienEnCours;
SAMBA_VAR char  LectEpisodeEnCours;
SAMBA_VAR int   LectEpisodeAgite;
SAMBA_VAR char  VerifIpLog;            /* imprime les infos de transmission IP              */
SAMBA_VAR char  VerifErreurLog;        /* imprime les infos durant un episode d'erreurs     */
SAMBA_VAR char  VerifErreurStoppe;     /* Arrete le repartiteur si erreur de transmission   */
// SAMBA_VAR char  LectMode;           /* voir enum {} LECT_MODE              */
// SAMBA_VAR char  LectRunNouveau;     /* pour numerotation run               */
SAMBA_VAR Panel pLectEtat;
SAMBA_VAR int   VoiesLocalesNb;        /* nombre de voies lues localement	                */
//- SAMBA_VAR int   VoiesSurPCI;       /* dont nombre de voies lues sur PCI                 */
SAMBA_VAR char  LectAutreSequence;
SAMBA_VAR int   LectDureeAcq,LectDureeRegen,LectDureeStream;
SAMBA_VAR int   LectGardeReset;         /* duree minimale depuis reset des detecteurs (s)    */
SAMBA_VAR int   LectDureeReset;         /* duree minimale entre reset des detecteurs (s)     */
SAMBA_VAR int   LectAttenteReset;       /* attente avant mesure des spectres (s)             */
SAMBA_VAR char  ArchAttendsD3;          /* vrai si doit archiver sur synchro D3              */
SAMBA_VAR int   LectPlancheSuiviMargeX,LectPlancheSuiviMargeY;

/*
	PanelShort(p,"Init. brutes",&LectTrmtInitValBrute);
	PanelShort(p,"Init. traitees",&LectTrmtInitValTraitee);
	PanelBool (p,"Ajout cosinus",&TestSimul);
	PanelFloat(p,"Amplitude (ADU)",&AmplitudeCosinus);
	PanelFloat(p,"Periode (ms)",&PeriodeCosinus);
*/

SAMBA_VAR char  TestSimul;
SAMBA_VAR int   TestSimulDelai;

SAMBA_VAR char TrmtDemande,TrmtRegulType,TrmtRegulDemandee;
SAMBA_VAR char TrmtIdentPasFaite;
SAMBA_VAR char TrmtHampl,TrmtHmontee,TrmtH2D;
SAMBA_VAR CebInstr TrmtPrgm[MAX_TRIG_INSTR];
SAMBA_VAR char *TrmtDatationListe[PHYS_TYPES+1];
SAMBA_VAR char *TrmtClassesListe[TRMT_NBCLASSES+1]
#ifdef SAMBA_C
 = { "brute", "traitee", "\0" }
#endif
;
SAMBA_VAR char *LectModeTexte[]
#ifdef SAMBA_C
 = { "en serie", "en parallele", "boostee", "optimisee", "version 4", "\0" }
#endif
;

SAMBA_VAR struct {
	short rep;
	short entree;
	short voie;
	TypeADU precedente;
} SambaEchantillon[SAMBA_ECHANT_MAX]; /* description d'un echantillon de donnees EN MODE STREAM */
SAMBA_VAR int SambaEchantillonReel,SambaEchantillonLngr; /* SambaEchantillonLngr = ECHANTILLON TOTAL Y COMPRIS LES PSEUDOS!! */
#define SAMBA_ECHANT_STATUS -2

SAMBA_VAR struct {
	short rep;
	short entree;
	short voie;
	TypeADU precedente;
} SambaEnregEvt[SAMBA_ECHANT_MAX]; /* description d'un echantillon de donnees EN MODE STREAM */
SAMBA_VAR int SambaEnregEvtLngr;

/* defini le trigger. Attention: il peut y avoir 'post-traitement' sans trigger */
typedef enum {
	TRGR_EXTREMA = 1,
	TRGR_NIVEAU
} TRGR_RECHERCHE;

typedef enum {
	TRGR_EXTERNE = 0,
	TRGR_ALGO_RANDOM,
	TRGR_ALGO_SCRIPT,
	TRGR_ALGO_FRONT,
	TRGR_ALGO_PORTE,
	TRGR_ALGO_NIVEAU,
	TRGR_ALGO_CONN,
	TRGR_NBALGOS
} TRGR_ALGOS;
SAMBA_VAR char *TrgrOrigineTexte[TRGR_NBALGOS + 1]
#ifdef SAMBA_C
 = { "externe", "aleatoire", "script", "front", "porte", "niveau", "connecte", "\0" }
#endif
;
SAMBA_VAR char *CalcDonneesName[]
#ifdef SAMBA_C
 = { "brute", "traitee", "evt.moyen", "\0" }
#endif
;

SAMBA_VAR int    TrmtPanicMax;
SAMBA_VAR float *TrgrResultat; /* Ref. variable et valeur trigger */
// SAMBA_VAR char TriggerActif;

// SAMBA_VAR char  CalculeSuppl;
SAMBA_VAR char *TrmtCsnsmNom[TRMT_CSNSM_MAX + 1];
#ifdef VISU_FONCTION_TRANSFERT
SAMBA_VAR char   TrmtHaffiche;
#endif

typedef struct {
	int        nbpts;
	rfftw_plan plan;
	fftw_real *temps,*freq;
} TypeCalcFftBase,*CalcFftBase;

SAMBA_VAR int    MonitFftDonnees;
SAMBA_VAR int    MonitFftVoie;
SAMBA_VAR int    MonitFftDim;
SAMBA_VAR char   MonitFftMode;
SAMBA_VAR int    MonitFftNb;
SAMBA_VAR float  MonitFftEpsilon;
SAMBA_VAR GraphAxeParm MonitFftParm[2];
SAMBA_VAR TypeCalcFftBase  MonitFftBase;
SAMBA_VAR char   MonitFftDemande,MonitFftActif;
SAMBA_VAR int    MonitFftNbFreq;
SAMBA_VAR float  MonitFftNorme;
SAMBA_VAR float  MonitFftNbSpectres,MonitFftEpsilon;
SAMBA_VAR float *MonitFftCarreSomme;
SAMBA_VAR float *MonitFftCarreMoyenne;
SAMBA_VAR float *MonitFftNouveau;
SAMBA_VAR float *MonitFftMoyenne;
SAMBA_VAR float  MonitFftAb6[2];
SAMBA_VAR Graph  gMonitFftAuVol;

#define MAXINTERVALLES 100
SAMBA_VAR double CalcTi[MAXINTERVALLES];
SAMBA_VAR int64  CalcPi[MAXINTERVALLES];
SAMBA_VAR TypeCalcFftBase CalcPlanStd;
SAMBA_VAR Panel  pCalcSelPtNb;
SAMBA_VAR int    CalcDim,CalcMax;
SAMBA_VAR float  CalcAvance; // CalcAfaire,CalcFaits,
SAMBA_VAR Instrum iCalcAvancement;
#ifdef BCP_EVTS
	SAMBA_VAR int    CalcNb;
#endif

SAMBA_VAR int    CalcDonnees;
SAMBA_VAR int    CalcSpectresMax;

SAMBA_VAR char   CalcModeAuto;
SAMBA_VAR GraphAxeParm CalcAuVolParms[2];
SAMBA_VAR char   CalcSpectreAutoAuVol,CalcSpectreAutoPseudo,CalcSpectreAutoInval;
SAMBA_VAR float *CalcAutoCarreSomme[VOIES_MAX];
SAMBA_VAR float *CalcAutoSpectreMoyenne[VOIES_MAX];
SAMBA_VAR int    CalcAutoSpectreDim[VOIES_MAX];
SAMBA_VAR char   CalcAutoBolo[DETEC_MAX]; //,CalcAutoAffiche[DETEC_MAX];
SAMBA_VAR char   CalcAutoPhys[PHYS_TYPES];
SAMBA_VAR struct {
	char  nom_phys[MODL_NOM]; int lu;
	int   dim,nbfreq;
	float ab6[2];
	GraphAxeParm parms[2];
} CalcSpectreAuto[PHYS_TYPES],CalcSpectreAutoPrefs[PHYS_TYPES],CalcSpectreAutoLu;
SAMBA_VAR int    CalcSpectreAutoDimX,CalcSpectreAutoDimY;
SAMBA_VAR int    CalcSpectreAutoDuree;
SAMBA_VAR float  CalcGlissantEpsilon;
SAMBA_VAR int    CalcPhysNb;
SAMBA_VAR float  CalcSpectreAutoNb[VOIES_MAX];
SAMBA_VAR Graph gCalcSpectreBolo[DETEC_MAX][PHYS_TYPES];
SAMBA_VAR int   gCalcSpectreY[DETEC_MAX][PHYS_TYPES];
SAMBA_VAR Graph gCalcSpectreAffiche[DETEC_MAX * PHYS_TYPES];
SAMBA_VAR Graph gCalcSpectreAutoDefaut;
SAMBA_VAR int    CalcSpectreFenNb;
SAMBA_VAR Menu  mCalcSpectreControle;
#ifdef SPECTRES_SEQUENCES
SAMBA_VAR Menu  mCalcSpectreAutoEnCours,mCalcSpectreAutoFinis;
SAMBA_VAR Panel pCalcSpectreAuto,pCalcSpectreBolo,pCalcSpectreAffiche;
#endif /* SPECTRES_SEQUENCES */

SAMBA_VAR off_t ArchPosEvt;
SAMBA_VAR char ArchiveSurRepert,ArchHorsDelai;
SAMBA_VAR int64 ArchTrancheTaille,ArchTrancheReste[ARCH_TYPEDATA];
SAMBA_VAR int64 ArchStampsSauves,ArchStreamSauve;
SAMBA_VAR int  ArchEvtsVides,ArchEvtsPerdus;
SAMBA_VAR int  ArchLngrEvt,ArchLngrVoie;
SAMBA_VAR TypeDonnee *ArchMoyenne;
SAMBA_VAR int  ArchMoyenneMax;

SAMBA_VAR int AutomSrceCalibIndex[AUTOM_CALIB_MAX],AutomSrceRegenIndex[AUTOM_REGEN_MAX];

SAMBA_VAR char *ArchModeTexte[ARCH_NBMODES + 1]
#ifdef SAMBA_C
= { "individuelle", "avec leurs voisins", "par tour", "en coincidence", "complete", "\0" }
#endif
;

typedef enum {
	ARCH_DEFAUT = 1,
	ARCH_STREAM,
	ARCH_EVTS,
	ARCH_TOUT,
	ARCH_NBBUFF
} ARCH_BUFF;
#define ARCH_BUFF_CLES "jetee/defaut/stream/evts/tout"

#define MAXDEBUG 20
SAMBA_VAR int ArchEvt2emeLot;

// SAMBA_VAR float EvtTaux;
SAMBA_VAR int EvtAffichable,EvtASauver,EvtNbMax; // SAMBA_VAR int EvtTot;
// SAMBA_VAR TypeEvt Evt[MAXEVTS];
SAMBA_VAR TypeEvt *Evt;
SAMBA_VAR TypeEvt EvtPrecedent;

SAMBA_VAR int Dx,Dy;
SAMBA_VAR Menu mMonitBarre,mSambaProcs;

SAMBA_VAR Menu mSettings;
SAMBA_VAR Menu mGest,mImprim;
SAMBA_VAR Menu mCmdes;
SAMBA_VAR Menu mLect;
SAMBA_VAR Menu mLectRuns;
#ifdef ANCIEN_MENU_AFFICHAGES
	SAMBA_VAR Menu mMonit;
#endif
SAMBA_VAR Menu mCalc;
SAMBA_VAR Menu mGeneGere;

//-SAMBA_VAR Menu mBasic;
SAMBA_VAR Menu mBasicDefinitions;
SAMBA_VAR Panel pBasicDesc;
int BasicEcritures(),BasicDirect(),BasicLectures(),BasicResetFIFO();
int BasicIpEcrit(),BasicIpRaz(),BasicIpStart(),BasicIpLit();
int BasicCaliRaz(),BasicCaliEcrit(),BasicCaliLit();
int BasicSmbRaz(),BasicSmbEcrit(),BasicSmbLit();
SAMBA_VAR TypeMenuItem iBasicPCI[]
#ifdef SAMBA_C
 = {
	{ "Definitions",  MNU_MENU    &mBasicDefinitions },
	{ "Ecriture",     MNU_FONCTION  BasicEcritures },
	{ "Acces direct", MNU_FONCTION  BasicDirect },
	{ "Lecture",      MNU_FONCTION  BasicLectures },
	{ "Reset FIFO",   MNU_FONCTION  BasicResetFIFO },
	{ "Preferences",  MNU_PANEL   &pBasicDesc },
//	{ "Fermer",       MNU_RETOUR },
	MNU_END
}
#endif
;
SAMBA_VAR TypeMenuItem iBasicIp[]
#ifdef SAMBA_C
 = {
	{ "Ecriture",     MNU_FONCTION  BasicIpEcrit },
	//	{ "Mise a jour",  MNU_FONCTION  BasicIpMaj }, // remplace par procedures init et reset
	//-	{ "MAJ globale",  MNU_FONCTION  BasicIpTousMaj },
	//-	{ "Parametrage",  MNU_FONCTION  BasicIpParms },
	{ "Vidage",       MNU_FONCTION  BasicIpRaz },
	{ "Demarrage",    MNU_FONCTION  BasicIpStart },
	{ "Lecture",      MNU_FONCTION  BasicIpLit },
	{ "Fermer",       MNU_RETOUR },
	MNU_END
}
#endif
;
SAMBA_VAR TypeMenuItem iBasicCali[]
#ifdef SAMBA_C
 = {
	{ "Preferences",  MNU_PANEL   &pBasicDesc },
	{ "Demarrage",    MNU_FONCTION  BasicCaliRaz },
	{ "Ecriture",     MNU_FONCTION  BasicCaliEcrit },
	{ "Lecture",      MNU_FONCTION  BasicCaliLit },
	{ "Fermer",       MNU_RETOUR },
	MNU_END
}
#endif
;
SAMBA_VAR TypeMenuItem iBasicSmb[]
#ifdef SAMBA_C
 = {
	{ "Demarrage",    MNU_FONCTION  BasicSmbRaz },
	{ "Ecriture",     MNU_FONCTION  BasicSmbEcrit },
	{ "Lecture",      MNU_FONCTION  BasicSmbLit },
	{ "Fermer",       MNU_RETOUR },
	MNU_END
}
#endif
;

SAMBA_VAR Menu mSwDump,mAutom,mDiag;
SAMBA_VAR Menu mCalcFft;
SAMBA_VAR Panel pCalculDesc; int CalculeSauve();

// SAMBA_VAR Cadre bBoloReglageBB1; (voir LectReglages)

SAMBA_VAR OpiumColor SambaJauneVert[]
#ifdef SAMBA_C
 = { { GRF_RGB_YELLOW }, { GRF_RGB_GREEN } }
#endif
;
SAMBA_VAR OpiumColor SambaRougeVert[]
#ifdef SAMBA_C
 = { { GRF_RGB_RED }, { GRF_RGB_GREEN } }
#endif
;
SAMBA_VAR OpiumColor SambaVertRouge[]
#ifdef SAMBA_C
= { { GRF_RGB_GREEN }, { GRF_RGB_RED } }
#endif
;
SAMBA_VAR OpiumColor SambaRougeVertJauneOrange[]
#ifdef SAMBA_C
 = { { GRF_RGB_RED }, { GRF_RGB_GREEN }, { GRF_RGB_YELLOW }, { GRF_RGB_ORANGE } }
#endif
;
SAMBA_VAR OpiumColor SambaRougeVertJauneViolet[]
#ifdef SAMBA_C
= { { GRF_RGB_RED }, { GRF_RGB_GREEN }, { GRF_RGB_YELLOW }, { WND_COLOR_DEMI, WND_COLOR_QUART, WND_COLOR_MAX } }
#endif
;
SAMBA_VAR OpiumColor SambaOrangeVert[]
#ifdef SAMBA_C
 = { { GRF_RGB_ORANGE }, { GRF_RGB_GREEN } }
#endif
;
SAMBA_VAR OpiumColor SambaOrangeVertJauneBleuViolet[]
#ifdef SAMBA_C
= { { GRF_RGB_ORANGE }, { GRF_RGB_GREEN }, { GRF_RGB_YELLOW },{ WND_COLOR_DEMI, WND_COLOR_DEMI, WND_COLOR_MAX }, { GRF_RGB_TURQUOISE } }
#endif
;
SAMBA_VAR OpiumColor SambaJauneVertOrangeBleu[]
#ifdef SAMBA_C
 = { { GRF_RGB_YELLOW }, { GRF_RGB_GREEN }, { GRF_RGB_ORANGE }, { WND_COLOR_DEMI, WND_COLOR_DEMI, WND_COLOR_MAX } }
#endif
;
SAMBA_VAR OpiumColor SambaVertJauneOrangeRouge[]
#ifdef SAMBA_C
 = { { GRF_RGB_GREEN }, { GRF_RGB_YELLOW }, { GRF_RGB_ORANGE }, { GRF_RGB_RED } }
#endif
;
SAMBA_VAR OpiumColor SambaBleuRougeOrangeJauneVert[]
#ifdef SAMBA_C
 = { { WND_COLOR_DEMI, WND_COLOR_DEMI, WND_COLOR_MAX }, { GRF_RGB_RED }, { GRF_RGB_ORANGE }, { GRF_RGB_YELLOW }, { GRF_RGB_GREEN } }
#endif
;
SAMBA_VAR OpiumColor SambaRougeOrangeJauneVert[]
#ifdef SAMBA_C
 = { { GRF_RGB_RED }, { GRF_RGB_ORANGE }, { GRF_RGB_YELLOW }, { GRF_RGB_GREEN } }
#endif
;
SAMBA_VAR OpiumColor SambaBleuOrangeVertRougeJaune[]
#ifdef SAMBA_C
 = { { WND_COLOR_DEMI, WND_COLOR_DEMI, WND_COLOR_MAX }, { GRF_RGB_ORANGE }, { GRF_RGB_GREEN }, { GRF_RGB_RED }, { GRF_RGB_YELLOW } }
#endif
;
SAMBA_VAR OpiumColor SambaBlancNoir[]
#ifdef SAMBA_C
= { { GRF_RGB_WHITE }, { GRF_RGB_BLACK } }
#endif
;
SAMBA_VAR OpiumColor SambaNoirOrange[]
#ifdef SAMBA_C
= { { GRF_RGB_BLACK }, { GRF_RGB_ORANGE } }
#endif
;

#ifdef COULEURS_A_LA_MAIN
	SAMBA_VAR OpiumColor SambaBOETJVRJ[]
	#ifdef SAMBA_C
	 = { { WND_COLOR_DEMI, WND_COLOR_DEMI, WND_COLOR_MAX }, { GRF_RGB_ORANGE }, { 0, WND_COLOR_DEMI, WND_COLOR_MAX }, { GRF_RGB_TURQUOISE },
		{ WND_COLOR_QUART*3, WND_COLOR_MAX, 0 }, { GRF_RGB_GREEN }, { GRF_RGB_RED }, { GRF_RGB_YELLOW } }
	#endif
	;
	SAMBA_VAR OpiumColor SambaBOETVRJ[]
	#ifdef SAMBA_C
	 = { { WND_COLOR_DEMI, WND_COLOR_DEMI, WND_COLOR_MAX }, { GRF_RGB_ORANGE }, { 0, WND_COLOR_DEMI, WND_COLOR_MAX }, { GRF_RGB_TURQUOISE },
		{ GRF_RGB_GREEN }, { GRF_RGB_RED }, { GRF_RGB_YELLOW } }
	#endif
	;
	SAMBA_VAR OpiumColor SambaBOETMVRJ[]
	#ifdef SAMBA_C
	 = { { WND_COLOR_DEMI, WND_COLOR_DEMI, WND_COLOR_MAX }, { GRF_RGB_ORANGE }, { 0, WND_COLOR_DEMI, WND_COLOR_MAX }, { GRF_RGB_TURQUOISE },
		{ GRF_RGB_MAGENTA }, { GRF_RGB_GREEN }, { GRF_RGB_RED }, { GRF_RGB_YELLOW } }
	#endif
	;
	SAMBA_VAR OpiumColor SambaBOETMVVRJ[]
	#ifdef SAMBA_C
	 = { { WND_COLOR_DEMI, WND_COLOR_DEMI, WND_COLOR_MAX }, { GRF_RGB_ORANGE }, { WND_COLOR_DEMI, WND_COLOR_MAX, 0 }, { GRF_RGB_TURQUOISE }, { GRF_RGB_MAGENTA },  
		{ WND_COLOR_QUART, WND_COLOR_QUART, WND_COLOR_QUART }, { GRF_RGB_GREEN }, { GRF_RGB_RED }, { GRF_RGB_YELLOW } }
	#endif
	;
#else  /* COULEURS_A_LA_MAIN */
	SAMBA_VAR OpiumColor *SambaTrmtCouleurs;
#endif /* COULEURS_A_LA_MAIN */

SAMBA_VAR OpiumColor SambaVioletVert[]
#ifdef SAMBA_C
= { { WND_COLOR_DEMI, WND_COLOR_QUART, WND_COLOR_MAX }, { GRF_RGB_GREEN } }
#endif
;
SAMBA_VAR OpiumColor SambaArcEnCiel[256];

SAMBA_VAR TypeInstrumCadrant Cadrant1
#ifdef SAMBA_C
 = { -(PI / 4.0), (PI / 4.0), 100.0, 6.0 }
#endif
 ;
SAMBA_VAR TypeInstrumCadrant Cadrant2
#ifdef SAMBA_C
 = { -(PI / 4.0), (PI / 4.0), 80.0, 5.0 }
#endif
 ;
SAMBA_VAR TypeInstrumPotar Selecteur
#ifdef SAMBA_C
 = { -(PI / 4.0), (PI / 4.0), 25.0, 15.0 }
#endif
 ;
SAMBA_VAR TypeInstrumPotar Select180
#ifdef SAMBA_C
 = { -(1.8 * PI / 4.0), (1.8 * PI / 4.0), 15.0, 10.0 }
#endif
 ;
SAMBA_VAR TypeInstrumPotar Select270
#ifdef SAMBA_C
 = { -(3.0 * PI / 4.0), (3.0 * PI / 4.0), 25.0, 15.0 }
#endif
 ;
SAMBA_VAR TypeInstrumPotar Vernier
#ifdef SAMBA_C
// = { -(3.0 * PI / 4.0), (3.0 * PI / 4.0), 15.0, 10.0 } ou 20.0, 7.0
 = { -(0.6 * PI), (0.6 * PI), 15.0, 10.0 }
#endif
 ;
SAMBA_VAR TypeInstrumColonne Colonne
#ifdef SAMBA_C
 = {120, 6, 0, 0, 3, 4}
#endif
 ;
SAMBA_VAR TypeInstrumColonne Colonne3Quarts
#ifdef SAMBA_C
= {90, 8, 0, 0, 6, 5}
#endif
;
SAMBA_VAR TypeInstrumColonne ColonneMoitie
#ifdef SAMBA_C
= {60, 6, 0, 0, 4, 4}
#endif
;
SAMBA_VAR TypeInstrumGlissiere Glissiere
#ifdef SAMBA_C
 = {120, 15, 0, 1, 6, 3}
#endif
 ;
/*
SAMBA_VAR TypeInstrumGlissiere GlissTexte4
#ifdef SAMBA_C
 = {64, 30, 0, 1, 12, 8}
#endif
 ;
SAMBA_VAR TypeInstrumGlissiere GlissTexte3
#ifdef SAMBA_C
= {48, 30, 0, 1, 10, 8}
#endif
;
SAMBA_VAR TypeInstrumGlissiere GlissTexte2
#ifdef SAMBA_C
 = {32, 30, 0, 1, 10, 8}
#endif
 ;
SAMBA_VAR TypeInstrumGlissiere GlissTexteMini
#ifdef SAMBA_C
= {25, 30, 0, 1, 10, 8}
#endif
;
SAMBA_VAR TypeInstrumBascule BasculeHoriz
#ifdef SAMBA_C
= {15, 15, 1}
#endif
;
 */
SAMBA_VAR TypeInstrumGlissiere Gliss2lignes
#ifdef SAMBA_C
= {15, 15, 0, 1, 8, 3}
#endif
;
SAMBA_VAR TypeInstrumGlissiere Gliss3lignes
#ifdef SAMBA_C
= {30, 15, 0, 1, 8, 3}
#endif
;
SAMBA_VAR TypeInstrumGlissiere Gliss4lignes
#ifdef SAMBA_C
= {45, 15, 0, 1, 8, 3}
#endif
;
SAMBA_VAR TypeInstrumGlissiere Gliss5lignes
#ifdef SAMBA_C
= {60, 15, 0, 1, 8, 3}
#endif
;
SAMBA_VAR TypeInstrumGlissiere Gliss6lignes
#ifdef SAMBA_C
= {75, 15, 0, 1, 6, 3}
#endif
;
SAMBA_VAR TypeInstrumGlissiere Gliss12lignes
#ifdef SAMBA_C
= {150, 15, 0, 1, 6, 3}
#endif
;
SAMBA_VAR TypeInstrumGlissiere GlissHoriz
#ifdef SAMBA_C
= {75, 15, 1, 1, 8, 3}
#endif
;
SAMBA_VAR TypeInstrumLevier Levier
#ifdef SAMBA_C
= {25, 10 }
#endif
;

SAMBA_VAR WndContextPtr SambaGC;
SAMBA_VAR Cadre bLectSpectres,bEtatDetecteurs;
SAMBA_VAR Cadre bTests,bTableauGeneral,bPrmCourants,bFinesses,bLecture,bSuiviEvts,bPrmDetailles;
SAMBA_VAR Cadre bSauvegardes,bTraitements,bDefEvts,bCablageTrigger,bRegulation;
SAMBA_VAR Cadre bAutomate;
SAMBA_VAR Cadre bLectBases;
SAMBA_VAR Graph gLectBaseNiveaux,gLectBaseBruits;
SAMBA_VAR Panel pLectBaseValeurs; // pLectHachoir;
SAMBA_VAR float Tzero[2];
SAMBA_VAR float LectBaseNiveau[VOIES_TYPES][DETEC_MAX];
SAMBA_VAR float LectBaseBruit[VOIES_TYPES][DETEC_MAX];
SAMBA_VAR Panel pLectMode,pLectTraitements,pLectConfigChoix,pDetecConfigDup,pDetecConfigMov,pMonitEcran;
SAMBA_VAR Cadre bMonitEcran;
SAMBA_VAR Instrum cArchive;
SAMBA_VAR Info iEcran;

SAMBA_VAR Panel pDefEvts;
SAMBA_VAR Panel pPeriodes,pNtuples;
SAMBA_VAR int EvtSignales,TrmtNbNouveaux,TrmtNbEchecs,TrmtNbPremat,TrmtNbRelances;

// #define HISTO_TIMING_TRMT
#ifdef HISTO_TIMING_TRMT
	#define MAXHISTOTIMING 100
	SAMBA_VAR int HistoTiming[MAXHISTOTIMING];
#endif
// #define HISTOS_DELAIS_EVT
#ifdef HISTOS_DELAIS_EVT
	#define MAXHISTOTRIG 100
	SAMBA_VAR int HistoTrig[MAXHISTOTRIG],HistoEvts[MAXHISTOTRIG];
#endif

/* .......................................................................... */

int AccesRestreint();

void TangoDefaults();

void NumeriseurStructInit(); void NumeriseurZero();
char NumeriseurBuilder(Menu menu, TypeMenuItem *item);

void DetecteursStructInit(); void DetecteurZero();
char DetecteurBuilder(Menu menu, TypeMenuItem *item);

void CablageStructInit();

int LectDemarre();
void OrgaSetup();
void SambaConstruitMateriel();
void BancSptrRafraichi(int avance, int etape);
#ifdef AVEC_PCI
	IcaReturnCode IcaDerniereErreur();
#else
	#define IcaDerniereErreur() 999
#endif

void SambaAttends(int ms);
void SambaLogDef(char *definition, char *mode, char *nom);
void SambaCompleteLigne(int tot, int cur);
void SambaLigneTable(char c, int cur, int tot);
void SambaMicrosec(int microsec);
int SambaUpdateInstrumFromItem(Panel p, int num, void *arg);
int SambaUpdatePanel(Instrum i);
void SambaRefreshPanel(Instrum i);
int SambaSauve(char *a_sauver, char *objet, char genre, char nombre, TypeFctn fctn);

int SambaAfficheVersion(Menu menu, TypeMenuItem *item);
int SambaNote(char *fmt, ...);
int SambaNoteCont(char *fmt, ...);
int SambaNoteSup(char *fmt, ...);
void SambaRunDate();
void SambaRunNomNouveau();
void SambaRunNomEnregistre();
void SambaAjouteRacine(char *complet, char *dir, char *declare);
void SambaAjoutePrefPath(char *complet, char *declare);

void EnvirVarPrintVal(TypeEnvirVar *envir, char *texte, int max);

int SambaScriptLance(Menu menu, TypeMenuItem *item);
int SambaDisque();
int SambaLangueDemande(Menu menu, TypeMenuItem *item);
void SambaLitCalendrier(Menu menu, TypeMenuItem *item);
int SambaChargeFpga(Menu menu, TypeMenuItem *item);
int SambaSauveArgs();
void SambaModifiePerimetre();
void SambaConfigCopieCatg(TypeConfigDefinition *dest, TypeConfigDefinition *srce);
int SambaConfigChange(Panel panel, int item, void *arg);
int SambaBufferiseShort(byte *buffer, shex valeur);
int SambaBufferiseInt(byte *buffer, int valeur);
int SambaBufferiseInt64(byte *buffer, int64 valeur);
int SambaBufferiseTexte(byte *buffer, char *texte, int m);
int SambaEcritSat1Court(int sat, byte cmde, hexa c1, hexa c2, shex valeur);
int SambaEcritSatCourt(int sat, byte cmde, byte cle, hexa c1, hexa c2, int nb, ...);
int SambaEcritSatLong(int sat, byte cmde, int64 valeur);
char SambaSauveReseau();
void SambaTrmtEncode(int voie, int classe);

int DiagComplet();

long SambaSecondes();
#ifndef CODE_WARRIOR_VSN
INLINE 
#endif
void SambaTempsEchantillon(int64 echant, int s0, int us0, int *sec, int *usec);
void SambaDefaults();
char SambaAlloc();
void RazVoiesManip();
void SettingsDefaults();
void SambaTrmtEncodeTous();
void TrmtNomme();
int ArchEdw2Hdr(int f, char imprime_actions, char imprime_entetes, char *explic);
FILE *ArchiveAccede(int fmt);
void ArchiveSeuils(int secs);

// int SambaRepartCharge(Menu menu);
// int SambaRepartRestart(Menu menu);

int TemplatesDecode(int voie, char embetant, char log);
#ifdef RL
int TemplatesCalcule(int voie, char log);
#endif
void TemplateLogFin(char log);
void TemplatesFiltre(int voie, char log);
float TemplatesNorme(int voie);
void ReglageRafraichiTout(int bb, int num, TypeReglage *regl);
void SambaPositionDecode(TypeChassisAxe *direction, int dim, char *code, TypeCablePosition *position);
void SambaPositionEncode(TypeChassisAxe *direction, int dim, short *position, char *code);
void SambaCoordEncode(TypeChassisAxe *direction, int dim, short *position, char *code, char of7);
void SambaAnalysePosition(short code, TypeChassisAxe *direction, int dim, short *position);

char MonitTagsSonorisee(TypeTagDisplay *tag);
void MonitTagsGlobal(int v, TypeConfigDefinition *utilisee);

void SelectionInit(TypeCritereJeu *critere, TypeVarJeu *jeu);
int  SettingsDetecCree(char volet, Panel appelant, void *v2);
int  SettingsDetecEvtsCree(Panel appelant, void *v2);
void SettingsDetecAffiche(char volet);
void DetecteursEvtCateg();
int  MonitHistosDef(),MonitEcranPanel(),MonitEcranPlanche(),MonitNtupleChange();
void MonitNtupleDump();

int LectJournalTrmt();
int TrmtImprimeTrgr();
int LectIdentImprime();
int LectDumpFifo();
int LectBuffDump();
int LectBuffStatus();
int LectTrmtEnter(Cadre cdr, void * ptr);
int LectTrmtExit(Panel panel, void * ptr);
char LectFixePostTrmt();
void LectDeclencheErreur(const char *nom, char *fic, int ligne, int num, TypeADU code);
int LectIdentBB();
int LectAcqStd(),LectStop();
void LectRegenAffiche(char termine, char avec_source);
void LectRegenTermine(char regen);
void LectRegenLance(char avec_source);
void LectRegenStoppe(char avec_dialogue);
int LectRegenChange();
int LectCompenseTout();
void LectDisplay();
void LectStocke1Seuil();
void LectTraiteStocke();
void LectActionUtilisateur();
int LectSpectres();
int LectSpectresAutoMesure(Menu menu, TypeMenuItem *item);
int LectSpectreTampons();
void LectConsignesConstruit();
char LectTrmtUtilise(int voie, int classe);
int LectRenovePlanche();
void LectPlancheSuivi(char affiche);
// #include <monit.h>

void TrgrPerso();
int TrmtRAZ(char log);
void TrmtInit(),TrmtStart(),TrmtSurBuffer();
int TrmtDonnee(int classe, char trmt, int voie, TypeDonnee donnee, TypeDonnee *adrs);
int TrmtCandidatSignale(char async, int voietrig, int64 moment, float niveau, short origine);
void TrmtConstruitEvt();
INLINE char SambaTraiteDonnee(int classe, char trmt, int voie, int32 donnee, char d3, int64 index, int32 *adrs, double *reelle);
INLINE char SambaDeclenche(int voie, float reelle);

typedef enum {
	FFT_SUR_MOYENNE = 0,
	FFT_MOYENNEE,
	FFT_SOMMEE,
	FFT_PUISSANCE,
	FFT_NBMODES
} FFT_MODE;
char CalculeFFTplan(CalcFftBase base, int nbpts, char *raison);
void CalculePuissance(fftw_real *fft, float *spectre, int nbpts, float dt, float unite);
char CalculeFFTTable(TypeDonnee *entree, int nbpts, char *raison);
char CalculeFFTVoie(char mode, int trmt, int voie, int64 *pt, int nbinterv, int nbpts, float *spectre, char *raison);
int CalculeFiltre(float omega1, float omega2, char modele, char type, short degre, TypeFiltre *coef);
int CalcSpectreAutoParms();
int CalcSpectreAutoSauve();

#ifdef MSGS_RESEAU
int LectReglage(),SettingsTriggerCtrl(),LectRazHistos(),LectDemarre(),LectStop();
SAMBA_VAR TypeMailFunction SambaCommandes[]
	#ifdef SAMBA_C
	= {
		{ "Reglages",   LectReglage,   0 },
		{ "Suspendre",  SettingsTriggerCtrl,   0 },
		{ "RAZ histos", LectRazHistos, 0 },
		{ "Demarrer",   LectDemarre,   0 },
		{ "Stopper",    LectStop,   0 },
	//	{ "Precedent",  MonitEvtPrecedent, 0 }, attention les parametres!
	//	{ "Suivant",    MonitEvtSuivant,   0 }, attention les parametres!
		CARLA_ENDFCTN
	}
	#endif
;
#endif

#endif
