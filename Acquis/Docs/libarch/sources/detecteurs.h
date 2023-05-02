#ifndef DETECTEURS_H
#define DETECTEURS_H

#ifdef DETECTEURS_C
#define DETECTEUR_VAR
#else
#define DETECTEUR_VAR extern
#endif

#include <environnement.h>

#include <rundata.h>
// #include <archive.h> pas possible: rebouclage sur detecteurs.h !
#include <edw_types.h>

#include <edwdb.h> // pour TypeCourbeVi

#ifdef Samba
#include <opium.h>
#include <histos.h>
#include <organigramme.h>
#include <scripts.h>
#endif /* Samba */

#include <claps.h>

// #define NOM_REP  "Repartiteur", associe a 'TypeDetecteur Cluzel;' et 'int BoloRep; (Bolo[BoloRep] == Cluzel)'
#define DETEC_STANDARD "Standard"

#define PHYS_TYPES 8

#define VOIES_TYPES 32
// VOIES_MAX = 200 jusqu'au 05.09.13
#define VOIES_MAX 512
#define VOIE_LISTE_STR VOIES_TYPES*(MODL_NOM+2)

#define DETEC_TYPES 32
#define DETEC_MAX 64
#define DETEC_AUXI_MAX 8
#define DETEC_ASSOC_MAX 4
#define DETEC_VOISINS_MAX 3
#define DETEC_PHASES_MAX 2
#define DETEC_COMPO_MAX 256

/* Il faut choisir l'un des 3 styles ... */
// #define VOIES_STYLE_NTD
// #define VOIES_STYLE_EDW2
#define VOIES_STYLE_INSTALLE
// #define VOIES_STYLE_UNIQUE

#define RL

typedef enum {
	DETEC_PARM_EVTS = 0,
	DETEC_PARM_TVOL,
	DETEC_PARM_PREP,
	DETEC_PARM_TRGR,
	DETEC_PARM_RGUL,
	DETEC_PARM_SOFT,
	DETEC_PARM_GENL,
	DETEC_PARM_ENVR,
	DETEC_PARM_NBNATURES
} DETEC_PARMS_NATURE;

/* .......................................................................... */
/*                                M O D E L E S                               */

typedef enum {
#ifdef VOIES_STYLE_NTD
	DETEC_NTD = 0,
	DETEC_NbSiA,
	DETEC_NbSiB,
	DETEC_IASC,
	DETEC_IASL,
	DETEC_ID_AB,
	DETEC_ID_CD,
	DETEC_ID_GH,
	DETEC_FID_AB,
	DETEC_FID_CD,
	DETEC_1VOIE,
#endif
#ifdef VOIES_STYLE_EDW2
	DETEC_NTD = 0,
	DETEC_NbSiA,
	DETEC_NbSiB,
	DETEC_IASC,
	DETEC_IASL,
	DETEC_ID_AB,
	DETEC_ID_CD,
	DETEC_ID_GH,
	DETEC_FID_AB,
	DETEC_FID_CD,
	DETEC_NbSi,
	DETEC_IAS,
	DETEC_ID,
	DETEC_FID,
	DETEC_1VOIE,
#endif
#ifdef VOIES_STYLE_UNIQUE
	DETEC_1VOIE = 0,
#endif
	DETEC_NBTYPES
} DETEC_TYPE;

typedef enum {
	CMDE_D0 = 0,
	CMDE_D1,
	CMDE_D2,
	CMDE_D3,
	CMDE_DUREE_DC,        /* Numeriseurs 1 seulement */
	CMDE_ACQUIS,
	CMDE_IDENT,
	CMDE_DIVIS3,          /* Numeriseur 0 seulement */
	CMDE_STATUT,          /* Numeriseur 0 seulement */
	CMDE_AMPL_MODUL,
	CMDE_COMP_MODUL,
	CMDE_CORR_TRNGL,
	CMDE_CORR_PIED,
	CMDE_POLAR_IONIS,     /* Numeriseurs 1 seulement */
	CMDE_POLAR_VOIE3,     /* Numeriseurs 1 seulement */
	CMDE_CHAUFFE_FET,     /* Numeriseurs 1.1 seulement */
	CMDE_REGUL_BOLO,      /* Numeriseurs 1.1 seulement */
	CMDE_GAIN_VOIE1,
	CMDE_GAIN_VOIE2,
	CMDE_GAIN_VOIE3,
	CMDE_CNTL_VOIE1,      /* Numeriseurs 1 seulement */
	CMDE_STOP_MODUL,      /* Numeriseurs 1 seulement */
	//	CMDE_DAC1,
	CMDE_DAC2,
	//	CMDE_DAC3,
	CMDE_DAC4,
	CMDE_DAC5,
	//	CMDE_DAC6,
	//	CMDE_DAC7,
	//	CMDE_DAC8,
	//	CMDE_DAC9,
	//	CMDE_DAC10,
	//	CMDE_DAC11,
	//	CMDE_DAC12,
	//	CMDE_COMP_TRI_V5,
	//	CMDE_COMP_TRI_V6,
	//	CMDE_AMPL_TRI_D9,
	//	CMDE_AMPL_TRI_D10,
	CMDE_AMPL_TRI_D11,
	CMDE_AMPL_TRI_D12,
	CMDE_COMP_CAR_V5,
	CMDE_COMP_CAR_V6,
	//	CMDE_AMPL_CAR_D9,
	//	CMDE_AMPL_CAR_D10,
	CMDE_AMPL_CAR_D11,
	CMDE_AMPL_CAR_D12,
	CMDE_GAIN_V1,
	CMDE_GAIN_V2,
	CMDE_GAIN_V3,
	//	CMDE_GAIN_Vx,
	//	CMDE_GAIN_Vx,
	CMDE_GAIN_V4,
	//	CMDE_REGUL_V1,
	//	CMDE_REGUL_V2,
	//	CMDE_REGUL_V3,
	//	CMDE_REGUL_V4,
	//	CMDE_REGUL_V5,
	//	CMDE_REGUL_V6,
	//	CMDE_RELAIS,
	//	CMDE_REFERENCE,
	//	CMDE_REGUL_PARM,
	CMDE_NBTYPES
} CMDE_TYPE;

typedef enum {
	REGL_D2,
	REGL_D3,
	REGL_MODUL,
	REGL_COMP,
	REGL_CORR,
	REGL_POLAR,
	REGL_GAIN,
	REGL_FET_MASSE,
	REGL_FET_CC,
	REGL_RAZFET,
	REGL_RESET,
	REGL_RELAIS,
	REGL_DIVERS,
	REGL_NBTYPES
} REGL_TYPE;
#define REGL_TYPETEXTE "d2/d3/modulation/compensation/correction/polar/gain/fet_masse/fet_cc/razfet/reset/relais/divers"

DETECTEUR_VAR char *ReglCmdeListe[REGL_NBTYPES+1]
#ifdef DETECTEURS_C
= { "d2", "d3", "modulation", "compensation", "correction", "polar", "gain", "fet_masse", "fet_cc", "razfet","reset","relais", "divers", "\0" }
#endif
;

typedef struct {
	char  nom[MODL_NOM];
	float duree;            /* duree evenement (millisec.)                                  */
	float delai;            /* delai evenement (millisec.)                                  */
	float tempsmort;        /* temps mort avant nouvelle recherche (milllisec.) (=post-trigger si <0) */
	float interv;           /* fenetre de recherche (millisec.)                             */
#ifdef RL
	int dimtemplate;
    float montee;
	float descente1;
	float descente2;
	float facteur2;   
#endif
} TypeSignalPhysique;

typedef struct {
	char  nom[MODL_NOM];
	// char  modulee;          /* vrai si l'electronique genere une modulation sur cette voie  */
	char  utilisee;         /* vrai si utilisee par au moins 1 detecteur reel               */
	char  std;              /* faux si evenement voie defini en propre                      */
	int   physique;         /* signal physique mesure par ce type de voie                   */
	float duree;            /* duree evenement (millisec.)                                  */
	float delai;            /* delai evenement (millisec.)                                  */
	float tempsmort;        /* temps mort avant nouvelle recherche (milllisec.) (=post-trigger si <0) */
	float interv;           /* fenetre de recherche (millisec.)                             */
	float coinc;            /* duree possible de coincidence (millisec.)                    */
	float pretrigger;       /* proportion de l'evenement avant le trigger (%)               */
	float base_dep;         /* position du debut de ligne de base dans le pre-trigger (%)   */
	float base_arr;         /* position de fin de ligne de base dans le pre-trigger (%)     */
	float ampl_10;          /* debut du calcul du temps de montee (% amplitude)             */
	float ampl_90;          /* fin   du calcul du temps de montee (% amplitude)             */
	struct {
		float t0;              /* debut (msecs depuis le maxi) de phase du signal produit   */
		float dt;              /* duree (msecs depuis le debut) de phase du signal produit  */
	} phase[DETEC_PHASES_MAX];
	//	float rapide;           /* duree de la premiere phase du signal produit                 */
	//	int a_lisser;           /* dimension du lissage pour calcul ligne de base               */
#ifdef RL
    int dimtemplate;
    float montee;
	float descente1;
	float descente2;
	float facteur2;
#endif
	int min_moyen;          /* amplitude min pour calcul evt moyen							*/
	int max_moyen;          /* amplitude max pour calcul evt moyen							*/
} TypeVoieModele;

#define DETEC_NOM_LNGR 16
#define DETEC_NOM_COURT 6
#define DETECT_SOFT_LNGR 256

typedef struct ComposantePseudo {
	float coef;
	short srce;
	struct ComposantePseudo *svt;
} TypeComposantePseudo;

typedef struct {
	char nom[MODL_NOM];           /* nom du pseudo capteur                               */
	char def[DETECT_SOFT_LNGR];   /* definition en fonction des capteurs precedents      */
	int  ref;                     /* capteur de reference                                */
	TypeComposantePseudo *pseudo; /* structure a reproduire au niveau des detecteurs     */
} TypeDetModlPseudo;

typedef struct {
	char nom[DETEC_NOM_LNGR];
	char surnom[DETEC_NOM_COURT];
	int  capteur;            /* voie locale concernee                                   */
	int  cmde;               /* type de commande                                        */
} TypeDetModlReglage;

typedef struct {
	char  nom[MODL_NOM];
	int   num;                  /* index dans ModeleDet[]                              */
	float masse;                /* masse de la cible (g)                               */
	char  avec_regen;           /* laisse la possibilite d'un reset de l'electronique  */
	int   suivant;              /* prochain numero libre dans la serie                 */
	int   nbcapt;               /* nombre de capteurs a lire                           */
	int   type[DETEC_CAPT_MAX]; /* liste des categories de voie a lire                 */
	int   nbsoft;               /* nombre de voies logiciellles a lire                 */
	TypeDetModlPseudo soft[DETEC_SOFT_MAX];
	char  nom_assoc[DETEC_ASSOC_MAX][MODL_NOM];
	int   nbassoc;              /* nombre de modeles associes                          */
	int   assoc[DETEC_ASSOC_MAX];    /* liste des modeles associes                     */
	int   nbregl;               /* nombre de reglages a faire sur ce type de detecteur */
	TypeDetModlReglage regl[DETEC_REGL_MAX];  /* liste des reglages                    */
	int   duree_raz;            /* duree du RAZ FET (secondes)                         */
	char  legende[DETEC_REGL_MAX][8]; /* entetes du panel des consignes                */
} TypeDetModele;

DETECTEUR_VAR TypeSignalPhysique ModelePhys[PHYS_TYPES],ModelePhysLu;
DETECTEUR_VAR char *ModelePhysListe[PHYS_TYPES+1];
DETECTEUR_VAR int ModelePhysNb;
DETECTEUR_VAR char ModelePhysNoms[PHYS_TYPES][MODL_NOM];
DETECTEUR_VAR TypeVoieModele ModeleVoie[VOIES_TYPES];
DETECTEUR_VAR char *ModeleVoieListe[VOIES_TYPES+1];
DETECTEUR_VAR TypeVoieModele ModeleVoieLu;
DETECTEUR_VAR int ModeleVoieNb;
DETECTEUR_VAR TypeDetModele ModeleDet[DETEC_TYPES],ModeleDetLu;
DETECTEUR_VAR char *ModeleDetListe[DETEC_TYPES+1];
DETECTEUR_VAR int ModeleDetNb;
DETECTEUR_VAR int ModlDetecChoisi;

/* DETECTEUR_VAR ArgDesc ModelePhysDesc[]
#ifdef DETECTEURS_C
 = {
	{ 0,				DESC_STR(MODL_NOM) ModelePhysLu.nom, 0 },
	{ "duree",			DESC_FLOAT &ModelePhysLu.duree,      "duree evenement (millisec.)" },
	{ "temps-mort",		DESC_FLOAT &ModelePhysLu.tempsmort,  "temps mort apres maxi evenement (millisec.)" },
	{ "fenetre",		DESC_FLOAT &ModelePhysLu.interv,     "fenetre de recherche (millisec.)" },
	{ "delai",			DESC_FLOAT &ModelePhysLu.delai,      "delai evenement (millisec.)" },
	#ifdef RL
	{ "dimtemplate",	DESC_INT   &ModelePhysLu.dimtemplate,	"dim. template (points)" },
	{ "montee",			DESC_FLOAT &ModelePhysLu.montee,		"montee (millisec.)" },
	{ "descente1",		DESC_FLOAT &ModelePhysLu.descente1,		"descente1 (millisec.)" },
	{ "descente2",		DESC_FLOAT &ModelePhysLu.descente2,		"descente2 (millisec.)" },
	{ "facteur2",		DESC_FLOAT &ModelePhysLu.facteur2,		"facteur2 " },
	#endif
	DESC_END
}
#endif
; */
DETECTEUR_VAR ArgDesc ModelePhysDesc[]
#ifdef DETECTEURS_C
= {
	{ 0,				DESC_STR(MODL_NOM) ModelePhysLu.nom, 0 },
	{ "duree-ms",		DESC_FLOAT &ModelePhysLu.duree,         0 },
	{ "temps-mort-ms",	DESC_FLOAT &ModelePhysLu.tempsmort,     0 },
	{ "fenetre-ms",		DESC_FLOAT &ModelePhysLu.interv,        0 },
	{ "delai-ms",		DESC_FLOAT &ModelePhysLu.delai,         0 },
	#ifdef RL
	{ "dimtemplate",	DESC_INT   &ModelePhysLu.dimtemplate,   0 },
	{ "montee-ms",		DESC_FLOAT &ModelePhysLu.montee,	    0 },
	{ "descente1-ms",	DESC_FLOAT &ModelePhysLu.descente1,	    0 },
	{ "descente2-ms",	DESC_FLOAT &ModelePhysLu.descente2,	    0 },
	{ "facteur2",		DESC_FLOAT &ModelePhysLu.facteur2,	    0 },
	#endif
	DESC_END
}
#endif
;
DETECTEUR_VAR ArgStruct ModelePhysAS
#ifdef DETECTEURS_C
= { (void *)ModelePhys, (void *)&ModelePhysLu, sizeof(TypeSignalPhysique), ModelePhysDesc }
 #endif
;

/* DETECTEUR_VAR ArgDesc ModeleVoieDesc[]
#ifdef DETECTEURS_C
 = {
	{ 0,             DESC_STR(MODL_NOM) ModeleVoieLu.nom, 0 },
	{ "physique",    DESC_LISTE ModelePhysListe, &ModeleVoieLu.physique, "signal physique mesure par ce type de voie" },
	//	{ "modulee",     DESC_BOOL  &ModeleVoieLu.modulee,    "vrai si l'electronique genere une modulation sur cette voie" },
	{ "temps-mort",  DESC_FLOAT &ModeleVoieLu.tempsmort,  "temps mort apres maxi evt (millisec.)" },
	{ "pretrigger",  DESC_FLOAT &ModeleVoieLu.pretrigger, "proportion de l'evenement avant le trigger (%)" },
	{ "base.debut",  DESC_FLOAT &ModeleVoieLu.base_dep,   "position du debut de ligne de base dans le pre-trigger (%)" },
	{ "base.fin",    DESC_FLOAT &ModeleVoieLu.base_arr,   "position de fin de ligne de base dans le pre-trigger (%)" },
	{ "coincidence", DESC_FLOAT &ModeleVoieLu.coinc,      "duree possible de coincidence (millisec.)" },
	DESC_END
}
#endif
; */
DETECTEUR_VAR ArgDesc ModeleVoieDesc[]
#ifdef DETECTEURS_C
= {
	{ 0,                DESC_STR(MODL_NOM) ModeleVoieLu.nom,    0 },
	{ "physique",       DESC_LISTE ModelePhysListe, &ModeleVoieLu.physique, 0 },
	{ "temps-mort-ms",  DESC_FLOAT &ModeleVoieLu.tempsmort,  0 },
	{ "pretrigger_%",   DESC_FLOAT &ModeleVoieLu.pretrigger, 0 },
	{ "base.debut_%",   DESC_FLOAT &ModeleVoieLu.base_dep,   0 },
	{ "base.fin_%",     DESC_FLOAT &ModeleVoieLu.base_arr,   0 },
	{ "montee.debut_%", DESC_FLOAT &ModeleVoieLu.ampl_10,   0 },
	{ "montee.fin_%",   DESC_FLOAT &ModeleVoieLu.ampl_90,   0 },
	{ "coincidence-ms", DESC_FLOAT &ModeleVoieLu.coinc,      0 },
	DESC_END
}
#endif
;
DETECTEUR_VAR ArgStruct ModeleVoieAS
#ifdef DETECTEURS_C
 = { (void *)ModeleVoie, (void *)&ModeleVoieLu, sizeof(TypeVoieModele), ModeleVoieDesc }
#endif
;

DETECTEUR_VAR char ModeleDetVoies[DETEC_CAPT_MAX][MODL_NOM];
DETECTEUR_VAR char ModeleDetAssoc[DETEC_ASSOC_MAX][MODL_NOM];
DETECTEUR_VAR TypeDetModlReglage ModeleDetReglLu;
DETECTEUR_VAR TypeDetModlPseudo  ModeleDetSoftLu;
DETECTEUR_VAR ArgDesc ModeleDetReglDesc[]
#ifdef DETECTEURS_C
 = {
	{ 0,           DESC_STR(DETEC_REGL_NOM)     ModeleDetReglLu.nom,     0 },
	{ "categorie", DESC_LISTE ReglCmdeListe,   &ModeleDetReglLu.cmde,    0 },
	{ "capteur",   DESC_LISTE ModeleVoieListe, &ModeleDetReglLu.capteur, 0 },
	DESC_END
}
#endif
;
DETECTEUR_VAR ArgStruct ModeleDetReglAS
#ifdef DETECTEURS_C
 = { (void *)ModeleDetLu.regl, (void *)&ModeleDetReglLu, sizeof(TypeDetModlReglage), ModeleDetReglDesc }
#endif
;

DETECTEUR_VAR ArgDesc ModeleDetSoftDesc[]
#ifdef DETECTEURS_C
 = {
	{ 0,           DESC_STR(MODL_NOM)           ModeleDetSoftLu.nom,     0 },
	{ 0,           DESC_STR(DETECT_SOFT_LNGR)   ModeleDetSoftLu.def,     0 },
	DESC_END
}
#endif
;
DETECTEUR_VAR ArgStruct ModeleDetSoftAS
#ifdef DETECTEURS_C
 = { (void *)ModeleDetLu.soft, (void *)&ModeleDetSoftLu, sizeof(TypeDetModlPseudo), ModeleDetSoftDesc }
#endif
;

DETECTEUR_VAR ArgDesc ModeleDetDesc[]
#ifdef DETECTEURS_C
 = {
	//	{ "capteurs",   DESC_STR_SIZE(MODL_NOM,ModeleDetLu.nbcapt,DETEC_CAPT_MAX)  ModeleDetVoies, 0 },
	{ 0,              DESC_STR(MODL_NOM)                                                  ModeleDetLu.nom,  0 },
	{ "capteurs",     DESC_LISTE_SIZE(ModeleDetLu.nbcapt,DETEC_CAPT_MAX) ModeleVoieListe, ModeleDetLu.type, 0 },
	{ "pseudos",      DESC_STRUCT_SIZE(ModeleDetLu.nbsoft,DETEC_SOFT_MAX)                &ModeleDetSoftAS,  0 },
	{ "reglages",     DESC_STRUCT_SIZE(ModeleDetLu.nbregl,DETEC_REGL_MAX)                &ModeleDetReglAS,  0 },
	{ "associes",     DESC_STR_SIZE(MODL_NOM,ModeleDetLu.nbassoc,DETEC_ASSOC_MAX)         ModeleDetLu.nom_assoc,   0 },
	{ "regeneration", DESC_BOOL                                                          &ModeleDetLu.avec_regen, 0 },
	{ "duree_razfet", DESC_INT                                                           &ModeleDetLu.duree_raz,   0 },
	DESC_END
}
#endif
;
DETECTEUR_VAR ArgStruct ModeleDetAS
#ifdef DETECTEURS_C
 = { (void *)ModeleDet, (void *)&ModeleDetLu, sizeof(TypeDetModele), ModeleDetDesc }
#endif
;

DETECTEUR_VAR ArgDesc DetectionDesc[]
#ifdef DETECTEURS_C
 = {
	{ "Physique",     DESC_STRUCT_SIZE(ModelePhysNb,PHYS_TYPES)    &ModelePhysAS, 0 },
	{ "Voies",        DESC_STRUCT_SIZE(ModeleVoieNb,VOIES_TYPES)   &ModeleVoieAS, 0 },
	{ "Detecteurs",   DESC_STRUCT_SIZE(ModeleDetNb,DETEC_TYPES)    &ModeleDetAS,  0 },
	DESC_END
}
#endif
;


/* .......................................................................... */
/*                        M A T E R I E L   U T I L I S E                     */

typedef enum {
	DETEC_HS = 0,
	DETEC_OK,
} DETEC_STATUT;
#define DETEC_STATUTS "HS/ok"

typedef enum {
	DETEC_LISTE = 0,
	DETEC_REGLAGES,
	DETEC_TOUT,
	DETEC_CONTENU_NB
} DETEC_CONTENU;

typedef struct {
	short bolo;                    /* numero de bolo hote                             */
	short capt;                    /* numero de capteur                               */
	short voie;                    /* numero de voie affectee                         */
	short modele;                  /* modele de voie utilise                          */
	char nom[MODL_NOM];            /* nom de la nouvelle voie (hors nom detecteur)    */
//	TypeComposantePseudo *pseudo;  /* composantes si pseudo-voie                      */
	short numgain;                 /* numero de gain variable dans le numeriseur      */
	float gain_reel;               /* valeurs de gain variables dans le numeriseur    */
	struct {
		short          num;        /* numero de numeriseur                            */
		unsigned char  serial;     /* numero de serie du numeriseur                   */
		unsigned char  adc;        /* numero d'ADC a lire                             */
		unsigned char  adrs;       /* adresse (donnee par le piano)                   */
	} bb;
} TypeCapteur;

typedef struct {
	char nom[DETEC_NOM_LNGR];     /* nom (repris du modele, utilise pour DB)              */
	short num;                    /* numero de reglage (meme que dans ModeleDet)          */
	short bolo;                   /* numero de bolo hote                                  */
	short capt;                   /* numero de voie locale concernee                      */
	short cmde;                   /* numero de DAC a utiliser                             */
	short bb;                     /* numero de BN a utiliser                              */
//	unsigned char adrs;           /* adresse BN a utiliser                                */
	unsigned char ss_adrs;        /* adresse du registre BN a utiliser                    */
	unsigned char negatif;        /* adresse du registre BN a utiliser en negatif         */
	unsigned char identik;        /* adresse du registre BN a utiliser de meme valeur     */
	short ress;                   /* numero de la ressource BN                            */
	short ressneg;                /* numero de la ressource BN  a utiliser en negatif     */
	short resspos;                /* numero de la ressource BN  a utiliser de meme valeur */
	char std[NUMER_RESS_VALTXT];  /* valeurs standard en texte                            */
	char user[NUMER_RESS_VALTXT]; /* consignes courantes utilisees et affichees           */
	union {                       /* valeur numerique de la consigne ..                   */
		float r;                  /* .. dans le cas "flottant" ..                         */
		int i;                    /* .. et dans le cas "entier" ou "cle" ..               */
	} val;                        /* (utilise seulement dans le panel consignes)          */
	char modifie;                 /* (utilise seulement dans le panel consignes)          */
	char *valeurs;                /* valeurs a utiliser, tirees du cablage  (cf razfet)   */
	/* float mval;                   valeur memorisee                                     */
	/* float rval;                   valeur affichee et ecrite dans le detecteur          */
	/* TypeADU hval;                 idem                                                 */
} TypeReglage;

#ifdef Samba
typedef struct {
	char  script[MAXFILENAME];       /* fichier contenant le script a executer            */
	short bloc;                      /* numero du programme (bloc de script) a executer   */
} TypeDetecteurPrgm;
#endif

typedef struct {
	char nom[DETEC_NOM];             /* nom (p.exe GGA1, voir XFN)                           */
	short numero;                    /* index dans le tableau des detecteurs                 */
	short ref;                       /* numero eventuel d'un detecteur de reference          */
	short num_hdr;                   /* index dans l'entete des sauvegardes                  */
	TypeCablePosition pos;           /* position numerique dans le cryostat                  */
	char  adresse[CODE_MAX];         /* position alphabetique(<lettre-galette><numero-trou>) */
	char  fichier[MAXFILENAME];      /* si non vide, fichier ou ecrire ce bolo               */
	char  a_sauver;                  /* vrai si doit etre sauve dans FichierDetecteurs       */
	char  branche;                   /* vrai si au moins un numeriseur est connecte          */
	char  lu;                        /* statut au sein de la manip                           */
	char  local;                     /* vrai si gere par cet ordi                            */
	char  a_lire;                    /* vrai si lu et local                                  */
	char  avec_regen;                /* laisse la possibilite d'un reset de l'electronique   */
	char  razfet_en_cours;           /* etat du RAZ FET (bit 2 dans CNTL_VOIE1)              */
//	char  modifie;                   /* vrai si des reglages ont ete modifies                */
	char  seuils_changes;            /* vrai si les seuils ont ete changes en cours de run   */
	char  sel;                       /* vrai si selectionne pour la modif des parms          */
	int   mode_arch;                 /* mode de sauvegarde                                   */
	int   type;                      /* type (index dans DetModele)                          */
	float masse;                     /* masse du detecteur (g)                               */
	char  assoc_imposes;             /* vrai si liste des associes a sauver imposee          */
	int   nbassoc;                   /* nombre de detecteurs associes                        */
	short assoc[DETEC_ASSOC_MAX];    /* liste des detecteurs associes (plusieurs BB)         */
	char  nom_assoc[DETEC_ASSOC_MAX][DETEC_NOM];
	char  hote[HOTE_NOM];            /* nom du systeme charge de le lire                     */
	int   debut;                     /* pour programme d'initialisation particulier          */
	int   taille;                    /*             ---     ditto      ---                   */
	char  voisins_imposes;           /* vrai si liste des voisins a sauver imposee           */
	int   nbvoisins;                 /* nombre de voisins                                    */
	short voisin[DETEC_VOISINS_MAX]; /* liste des numeros de voisins                         */
	char  nomvoisins[DETEC_VOISINS_MAX][DETEC_NOM];
	short d2,d3;                     /* diviseurs D2 et D3                                   */
	int reset;                       /* intervalle entre deux resets (secondes)              */
	TypeReglage *regl_cc_fet;        /* reglage commandant le court-circuit des FETs         */
	//? TypeReglage *regl_trig;       /* reglage commandant l'envoi de la voie rapide (sur trigger) */
	int   nbreglages;                /* dimension du tableau suivant                         */
	TypeReglage reglage[DETEC_REGL_MAX]; /* liste des reglages                               */
	int   nbcapt;		             /* nombre de capteurs a lire                            */
	TypeCapteur captr[DETEC_CAPT_MAX];
#ifdef Samba
	TypeDetecteurPrgm start;         /* mise en service en debut de run                      */
	TypeDetecteurPrgm regul;         /* remise en etat periodique                            */
	TypeHwExecContext exec;          /* contexte d'execution de script                       */
	struct { Cadre planche; TypeOrgaLiaisons cablage; } schema;
#endif
} TypeDetecteur;

DETECTEUR_VAR char *BoloNom[DETEC_MAX+2];
DETECTEUR_VAR char *VoieNom[VOIES_MAX+1];

DETECTEUR_VAR TypeDetecteur Bolo[DETEC_MAX+1],DetecteurLu;
DETECTEUR_VAR TypeDetecteur DetecteurStandard[MAXSAT],*DetecteurStandardLocal;
DETECTEUR_VAR int   DetecStandardNb;
DETECTEUR_VAR char  DetecteurStandardListe;
DETECTEUR_VAR TypeReglage **DetecReglagesLocaux;
DETECTEUR_VAR int   DetecReglagesLocauxNb;
DETECTEUR_VAR char  VoieStatus[MODL_NOM];
DETECTEUR_VAR char DetecTexteComposition[DETEC_COMPO_MAX];
#ifdef Samba
DETECTEUR_VAR struct { char nomcomplet[MAXFILENAME]; } DetecScriptNoms[HW_MAXBLOC];
DETECTEUR_VAR TypeHwScript DetecScriptsLibrairie;
#endif /* Samba */

/* Ca pose probleme de devoir inclure archive.h, car icelui appelle detecteur.h .. */
DETECTEUR_VAR char *ArchModeNoms[ARCH_NBMODES + 1]
#ifdef DETECTEURS_C
 = { "seul", "voisins", "tour", "coinc", "tous", "manip", "\0" }
#endif
;
DETECTEUR_VAR char *ArchModeDetec[ARCH_NBMODES + 2];
/* .. et on a besoin ceans de ArchModeDetec et donc de ARCH_NBMODES !! */

DETECTEUR_VAR ArgDesc DetecParmsStdDesc[]
#ifdef DETECTEURS_C
 = {
	{ "masse",     DESC_FLOAT                &DetecteurLu.masse,        "masse du detecteur (g)" },
	{ "mode",      DESC_LISTE ArchModeDetec, &DetecteurLu.mode_arch,    "mode de sauvegarde" },
#ifdef Samba
	{ "demarrage", DESC_STR(MAXFILENAME)      DetecteurLu.start.script, "nom du fichier de demarrage de run" },
	{ "entretien", DESC_STR(MAXFILENAME)      DetecteurLu.regul.script, "nom du fichier d'entretien periodique" },
#endif /* Samba */
	DESC_END
}
#endif
;
/* DETECTEUR_VAR ArgDesc DetecteurParmsVsnDesc[]
#ifdef DETECTEURS_C
 = {
	{ "voisins",   DESC_LISTE_SIZE(DetecteurLu.nbvoisins,DETEC_VOISINS_MAX) BoloNom, DetecteurLu.voisin, 0 }, necessite de passer voisin en int
	DESC_END
}
#endif
; */
DETECTEUR_VAR ArgDesc DetecParmsVsnDesc[]
#ifdef DETECTEURS_C
 = {
	{ "voisins",   DESC_STR_SIZE(DETEC_NOM,DetecteurLu.nbvoisins,DETEC_VOISINS_MAX) DetecteurLu.nomvoisins, "detecteurs a sauver aussi en mode 'voisins'" },
	DESC_END
}
#endif
;
DETECTEUR_VAR ArgDesc DetecParmsAscDesc[]
#ifdef DETECTEURS_C
 = {
	{ "associes",  DESC_STR_SIZE(DETEC_NOM,DetecteurLu.nbassoc,DETEC_ASSOC_MAX)     DetecteurLu.nom_assoc,  "detecteurs a sauver aussi en mode 'individuel'" },
	DESC_END
}
#endif
;
DETECTEUR_VAR ArgDesc *DetecParmsStdPtr,*DetecParmsVsnPtr,*DetecParmsAscPtr;
DETECTEUR_VAR ArgDesc DetecParmsDesc[]
#ifdef DETECTEURS_C
 = {
	{ DESC_COMMENT 0 },
	DESC_INCLUDE(DetecParmsStdPtr),
	DESC_INCLUDE(DetecParmsVsnPtr),
	DESC_INCLUDE(DetecParmsAscPtr),
	{ "lecteur",   DESC_NONE }, // DESC_STR(HOTE_NOM) DetecteurLu.hote, "nom du systeme charge de le lire" },
	{ "ip",        DESC_NONE },
	DESC_END
}
#endif
;
DETECTEUR_VAR ArgDesc DetecParmsCompletDesc[]
#ifdef DETECTEURS_C
= {
	{ "nom",       DESC_STR(DETEC_NOM)         DetecteurLu.nom,  "nom" },
	{ "modele",    DESC_LISTE ModeleDetListe, &DetecteurLu.type, "modele" },
	{ "statut",    DESC_KEY DETEC_STATUTS,    &DetecteurLu.lu,   "statut ("DETEC_STATUTS")" },
	DESC_INCLUDE(DetecParmsStdPtr),
	DESC_INCLUDE(DetecParmsVsnPtr),
	DESC_INCLUDE(DetecParmsAscPtr),
	{ "lecteur",   DESC_NONE }, // DESC_STR(HOTE_NOM) DetecteurLu.hote, "nom du systeme charge de le lire" },
	{ "ip",        DESC_NONE },
	DESC_END
}
#endif
;
DETECTEUR_VAR ArgStruct DetecParmsCompletAS
#ifdef DETECTEURS_C
 = { 0, (void *)&DetecteurLu, sizeof(TypeDetecteur), DetecParmsCompletDesc }
#endif
;

typedef struct {
	char nom[MODL_NOM];
	char composition[DETEC_COMPO_MAX];
} TypeDetectPseudo;
DETECTEUR_VAR TypeDetectPseudo Pseudo[DETEC_CAPT_MAX],PseudoLu;
DETECTEUR_VAR int PseudoNb;
DETECTEUR_VAR ArgDesc Detec1PseudoDesc[]
#ifdef DETECTEURS_C
 = {
	{ "nom",         DESC_STR(MODL_NOM)        PseudoLu.nom, 0 },
	{ "composition", DESC_STR(DETEC_COMPO_MAX) PseudoLu.composition, 0 },
	DESC_END
}
#endif
;
DETECTEUR_VAR ArgStruct DetecPseudoAS
#ifdef DETECTEURS_C
 = { (void *)Pseudo, (void *)&PseudoLu, sizeof(TypeDetectPseudo), Detec1PseudoDesc }
#endif
;
DETECTEUR_VAR ArgDesc DetectionPseudosDesc[]
#ifdef DETECTEURS_C
 = {
	{ "Voies logicielles", DESC_STRUCT_SIZE(PseudoNb,DETEC_CAPT_MAX) &DetecPseudoAS, 0 },
	DESC_END
}
#endif
;

DETECTEUR_VAR char DetecteurRacine[MAXFILENAME];
DETECTEUR_VAR char DetecteurStock[MAXFILENAME];

DETECTEUR_VAR int  BoloNum,CapteurNum,VoieNum,ModlNum,NbBolosLocaux;
DETECTEUR_VAR char *CapteurListe[DETEC_CAPT_MAX];
DETECTEUR_VAR char *NbVoiesListe[DETEC_TYPES+1];
DETECTEUR_VAR char NbVoiesTexte[DETEC_TYPES][8];
DETECTEUR_VAR int  NbVoiesNb;
DETECTEUR_VAR int  NbVoiesVal[DETEC_TYPES];
DETECTEUR_VAR char ReglageDetecTousIndiv;
DETECTEUR_VAR char ReglageLog,ReglageComplet;
DETECTEUR_VAR int  BoloLocalNb;
//DETECTEUR_VAR char BoloPosition[DETEC_MAX][8];
//DETECTEUR_VAR char BoloConnecteur[DETEC_MAX][8];
DETECTEUR_VAR char **ReglGains;
DETECTEUR_VAR char ReglagesEtatDemande;
DETECTEUR_VAR char DetecAvecReglages,DetecAvecPolar,DetecAvecRaz,DetecAvecModulation;

#ifdef Samba
//DETECTEUR_VAR Cadre ReglagePlanche[VOIES_MAX];
//DETECTEUR_VAR short ReglagePlanchePrems[VOIES_MAX];
//DETECTEUR_VAR short ReglagePlancheNb[VOIES_MAX];
//DETECTEUR_VAR Graph gReglageSignal[VOIES_MAX];
//DETECTEUR_VAR char ReglageAvecSignal[VOIES_MAX];
#endif

#define MAXOSCILLO 8
#define MAXOSCVOIES 8
#define MAXREPET 9
DETECTEUR_VAR int OscilloNb;
typedef struct {
	short num;
	short voie[MAXOSCVOIES];
	short bolo[MAXOSCVOIES];
	short nbvoies;
	char  mode,grille;
	int   traces;
	float *moyenne;
	int   ampl,zero;
	int   dim,lngr;
#ifdef Samba
	Instrum iNb,iAcq,iDemod,iAff;
	Instrum iTemps,iAmpl,iZero,iTrig,iActif;
	Panel pTemps,pAmpl,pZero,pTrig;
	Graph ecran;
    struct {
        Histo histo; HistoData hd;
        float min,max;
		Panel pAmpl; Instrum iMin,iMax; int vMin,vMax;
        Graph graph;
		Cadre cRaz,cSauve;
    } h;
#endif
	float ab6[MAXREPET+1][2];
	float horloge,temps;
	char  acq,defile,avec_trigger,demodule,trmt_cur,bZero;
	float marqueX[2]; TypeDonnee marqueY[2]; int marque;
	float niveauX[2],niveauY[2]; int niveau,evt;
} TypeOscillo,*Oscillo;
DETECTEUR_VAR Oscillo OscilloReglage[MAXOSCILLO];
DETECTEUR_VAR int  OscilloNum;
DETECTEUR_VAR char OscilloOuvert;
#ifdef Samba
DETECTEUR_VAR Menu OscilloAcqAllume;
#endif

//- DETECTEUR_VAR float ReglMoyenne,ReglBruit,ReglCompens;
DETECTEUR_VAR float ReglVbolo,ReglIbolo,ReglRbolo;

#define MAXVI 32
DETECTEUR_VAR float ReglTabI[MAXVI],ReglTabV[MAXVI],ReglTabR[MAXVI],ReglTabT[MAXVI];
DETECTEUR_VAR char  ReglTabNom[DETEC_NOM];
DETECTEUR_VAR int   ReglTabNb;
DETECTEUR_VAR TypeCourbeVi *CourbeVi;
#ifdef Samba
DETECTEUR_VAR Menu  mGestVI;
DETECTEUR_VAR Graph gGestVi;
DETECTEUR_VAR Panel pDetecteurVoie,pReglViAuto;
DETECTEUR_VAR struct {
	float dep,arr,pas,cur;
	int   attente;
	int   nb_mesures,nb_voies;
	struct {
		short num;
		float I[MAXVI]; float V[MAXVI]; float R[MAXVI];
	} voie[VOIES_MAX];
	float T[MAXVI];
	Graph g[DETEC_MAX];
	char  oper,arrete,actif;
} ReglVi;
#endif /* Samba */

DETECTEUR_VAR char OscilloDemodule;

typedef struct {
#ifdef Samba
	Cadre planche;
	Menu  menu_voie,menu_suite,menu_modul,menu_debloque,menu_raz;
	Panel stats,taux_evts,vi,raz,status,electrons;
#endif
	float trigger;
	char  trig_actif;
	short prems,nb;
	char  complete;
	char  modifiable;
	char  validite;
	char  mode;
	Oscillo oscillo;
	float pic2pic,moyenne,bruit,mesure_enV,bruit_mV,bruit_e,gain_ampli;
	float compens,V,I,R;
} TypeVoieInfo;
DETECTEUR_VAR TypeVoieInfo VoieInfo[VOIES_MAX];

#define MAXCDES 2048
DETECTEUR_VAR int32 Commandes[MAXCDES];
DETECTEUR_VAR int   CommandesNb;
#ifdef Samba
DETECTEUR_VAR Term tCmdes;
#endif

/*
typedef enum {
	CMDE_COMPTEUR = 0,
	CMDE_ONOFF = 1,
	CMDE_AMPL = 3,
	CMDE_GAIN = 4,
	CMDE_CHALEUR = 5,
	CMDE_MODUL = 6
} CMDE_ORGANE1;

DETECTEUR_VAR int   CmdeNum,CmdeNb; DETECTEUR_VAR unsigned short CmdeAdrs; DETECTEUR_VAR short CmdeValeur;
DETECTEUR_VAR int   CmdeAction;
DETECTEUR_VAR char  CmdesASauver;
DETECTEUR_VAR char  ReglUserUnite[CMDE_NBTYPES][16];
DETECTEUR_VAR float CmdeUserMin[CMDE_NBTYPES],CmdeUserMax[CMDE_NBTYPES],CmdeUserRange[CMDE_NBTYPES];
DETECTEUR_VAR char  CmdeTitre[CMDE_NBTYPES][5+MODL_NOM];

float   CmdeFloat(CMDE_TYPE cmde, TypeADU donnee);
TypeADU CmdeADU(CMDE_TYPE cmde, float valeur);
void    CmdeChargeFloat(int bolo, int capteur, CMDE_TYPE cmde, float valeur);
 */

void   DetecteursModlLit(char *fichier, FILE *g);
void   DetecteursModlEpilogue();
void   DetecteursModlVerifie();
void   DetecteurAjouteStd();
char  *DetecteurCanalStandardise(DETEC_PARMS_NATURE nature, int bolo, int cap, char actn);
void   DetecteurSauveConfig(int config_ggale);
int    DetecteursLit(char *nom_principal, char stream_deja_lu);
int    DetecteursLitFichier(FILE *fichier, char *nom_partiel, char *hote_global, TypeCablePosition pos_defaut, char stream_deja_lu, char log);
void   DetecteurEcrit(FILE *global, TypeDetecteur *detectr, int bolo, char avec_reglages, char quoi);
int    DetecteurEcritTous(char quoi, int qui);
void   DetecteurSauveListe();
void   DetecteurSauveSelectReglages(char avec_standard);
int    DetecteurSauveReglages(int bolo);
int    DetecteurCree(TypeDetecteur *detectr, char *nom, int dm, char *ref, char *hote, char *nom_partiel);
// void ReglageVerifieStatut();
void   DetecteursDecompte();
void   DetecteursVoisinage(char log);
char   DetecteurSelectionne();
void   DetecteurPlace(TypeDetecteur *detectr, TypeCablePosition pos_hexa);
TypeCablePosition DetecteurRetire(TypeDetecteur *detectr);
#ifdef Samba
char   DetecteurBrancheCapteur(TypeDetecteur *detectr, short cap, int bb, short vbb, unsigned char adrs, char *explic);
char   DetecteurVoieBrancheNumer(TypeDetecteur *detectr, short cap, int bb, short vbb, unsigned char adrs, char *explic);
#endif
void   DetecteurDebrancheCapteur(TypeDetecteur *detectr, short cap);
char   DetecteurBrancheNumeriseurs(TypeDetecteur *detectr);
void   DetecteurDebrancheNumeriseurs(TypeDetecteur *detectr);
char   DetecteurConnecteVoies(TypeDetecteur *detectr, int *nb_erreurs);
void   DetecteurConnecteReglages(TypeDetecteur *detectr, int *nb_erreurs);
void   DetecteurTermine(TypeDetecteur *detectr, char log);
void   ReglageDumpVoies();
char  *DetecteurCompositionEncode(TypeComposantePseudo *pseudo);
void   ReglageChargeFlottant(TypeDetecteur *detectr, TypeReglage *regl, int *bb_autorise, char *prefixe, float valeur);
void   ReglageChargeStandard(TypeDetecteur *detectr, TypeReglage *regl, int *bb_autorise, char *prefixe);
INLINE void ReglageChargeTerminee(int bb_autorise);
void  ReglageRafraichiTout(int bb, int num, TypeReglage *regl);
int   DetecteurMiseEnService();
int   DetecteurFlip(),DetecteurRazFetEDW();
INLINE int DetecteurChargeStandard(int bolo, char vrai_bolo, char *prefixe);
int   DetecteurChargeTous(char *prefixe);
void  DetecteurViMesuree(int voie);
INLINE void DetecteurAdcCalib(int voie, char *prefixe);
#ifdef Samba
void  ReglageFinalise(TypeReglage *reglage);
int   DetecteurCompenseVoie(Menu menu, MenuItem *item);
int   DetecteurReglages(Menu menu, MenuItem *item);
#endif
INLINE void  ReglageEffetDeBord(TypeReglage *regl, char *prefixe);
void  ReglageChangeFloat(TypeReglage *regl, float fval, char *prefixe);
int   ReglageNum(int bolo, int voie, int cmde);
TypeReglage *ReglagePtr(int voie, int cmde);
void  DetecteurRazFet(TypeDetecteur *detectr, TypeReglage *regl, int *bb_autorise, char *prefixe, char raz);
int   DetecteurActive(void *adrs);
int   DetecteursImprime(char tous);
#ifdef Samba
int   DetecteursDump(Menu menu, MenuItem *item);
void  ReglageRazBolo(TypeDetecteur *detectr, char *nom);
int   DetecteurParmsEntree(Menu menu, MenuItem *item);
void  DetecteurParmsConstruitVoie(Cadre planche, int voie, int *x0, int *y0, int cap);
int   DetecteurParmsSave(Menu menu, MenuItem *item);
int   DetecteurEcritTout(Menu menu, MenuItem *item);
#endif
int   DetecteurSauveTousReglages();
// int   ReglageEnregistrerEdw();
#ifdef Samba
int   DetecteurOscillo(Menu menu, MenuItem *item);
int   DetecteurGegene(Menu menu, MenuItem *item);
void  OscilloInit(Oscillo oscillo, int voie);
void  OscilloChangeTemps(Oscillo oscillo);
void  OscilloChangeMinMax(Oscillo oscillo);
int64 OscilloCourbesDebut(Oscillo oscillo, int *nb);
void  OscilloCourbesTrace(Oscillo oscillo, int64 debut, int nbpts);
int OscilloChangeiAmpl(Instrum instrum, Oscillo oscillo),  OscilloChangepAmpl(Panel panel, int item, void *arg);
int OscilloChangeiZero(Instrum instrum, Oscillo oscillo),  OscilloChangepZero(Panel panel, int item, void *arg);
int OscilloChangeiTemps(Instrum instrum, Oscillo oscillo), OscilloChangepTemps(Panel panel, int item, void *arg);
int OscilloRefresh(Instrum instrum, Oscillo oscillo);
INLINE char OscilloAffiche(Oscillo oscillo);
#endif

int   DetecteurVoieModelesActifs();
#ifdef Samba
int   DetecteurListeCapteurs(Panel panel, int num, void *arg);
#endif

#endif
