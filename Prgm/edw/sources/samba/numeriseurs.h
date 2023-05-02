#ifndef NUMERISEURS_H
#define NUMERISEURS_H

#ifdef NUMERISEURS_C
#define NUMERISEUR_VAR
#else
#define NUMERISEUR_VAR extern
#endif

#include <environnement.h>
#include <edw_types.h>

#include <opium4/opium.h>
#include <utils/claps.h>

#include <repartiteurs.h>

/* Versions des numeriseurs:
BB v0   : premiere version P.Cluzel, utilisee une fois a Saclay pendant la prehistoire
BB v1.0 : deuxieme version P.Cluzel, fabriquee a 4 exemplaires. Adressage par piano, 14 bits pour 2,048V.
BB v1.1 : version B. Paul: idem v1.0 sauf 8 DAC, ADC a 2,5V et mecanique compacte.
BB v2.0 : version A. Benoit + B. Paul
BB v2.3 : variante anticipant la version 3 pour la voie chaleur

    5.0 : ADC d'essai de NOSTOS
    5.1 : bouchon pour boitier OPERA
*/


/* .......................................................................... */
/*                                M O D E L E S                               */

typedef enum {
	RESS_INT = 0,
	RESS_FLOAT,
	RESS_CLES,
	RESS_PUIS2,
	RESS_NBTYPES
} RESS_TYPES;
#define RESS_TYPETEXTE "int/float/key/puis2"

typedef enum {
	RESS_NUL = 0,     /* a ne pas gerer vraiment                */
	RESS_DIVERS,      /* pas de role particulier                */
	RESS_POLAR,       /* polarisation                           */
	RESS_GAIN,        /* gain variable                          */
	RESS_MODUL,       /* amplitude de modulation chaleur        */
	RESS_COMP,        /* compensation carree interne            */
	RESS_CORR,        /* correction supplementaire anti-glitch  */
	RESS_D2,          /* periode de modulation                  */
	RESS_D3,          /* synchronisation lente                  */
	RESS_FET_MASSE,   /* RAZ FET par bit de mise a la masse     */
	RESS_FET_CC,      /* RAZ FET par mise a 0V                  */
	RESS_ALIM,        /* etat de fonctionnement                 */
	RESS_AUXI,        /* auxiliaire (a remettre a 0 sur RAZ)    */
	RESS_ADC,         /* ADC tracable                           */
	RESS_IDENT,       /* porte l'identification du boitier      */
	RESS_NBCATEG      /* voir aussi description ModeleBnRessDesc */
} RESS_CATEG;
#define RESS_CATEGTEXTE "nul/divers/polar/gain/modul/comp/corr/d2/d3/fet_masse/fet_cc/alim/auxi/adc/ident"

NUMERISEUR_VAR char *RessCategListe[RESS_NBCATEG+1]
#ifdef NUMERISEURS_C
= { "nul", "divers", "polar", "gain", "modulation", "compensation", "correction", 
	"d2", "d3", "fet_masse", "fet_cc", "alim", "auxiliaire", "ADC", "ident", "\0" }
#endif
;

#define NUMER_TYPES 16
#define NUMER_ADC_TYPES 16

#define NUMER_FPGA_CODE 8

#define NUMER_PROC_MAX 8
#define NUMER_RESS_MAX 128
#define NUMER_CLES_MAX 16
#define NUMER_ADC_MAX 8

#define NUMER_STATUS_DIM 128
#define NUMER_DIGITS_DAC 7

typedef enum {
	NUMER_RAPPORT_IDENT = 1, NUMER_RAPPORT_TEMPER, NUMER_RAPPORT_D2, NUMER_RAPPORT_D3, NUMER_RAPPORT_ALIM,
	NUMER_RAPPORT_REF, NUMER_RAPPORT_ADCON1, NUMER_RAPPORT_ADCON2, NUMER_RAPPORT_ADCON3, NUMER_RAPPORT_ADCON4, NUMER_RAPPORT_R1, NUMER_RAPPORT_R2,
	NUMER_RAPPORT_ADCVAL1, NUMER_RAPPORT_ADCVAL2, NUMER_RAPPORT_ADCVAL3, NUMER_RAPPORT_ADCVAL4, NUMER_RAPPORT_ADCVAL5, NUMER_RAPPORT_ADCVAL6,
	NUMER_RAPPORT_RT, NUMER_RAPPORT_RG1, NUMER_RAPPORT_RG2, NUMER_RAPPORT_RG3, NUMER_RAPPORT_RG4,
	NUMER_RAPPORT_DACVAL1, NUMER_RAPPORT_DACVAL2, NUMER_RAPPORT_DACVAL3, NUMER_RAPPORT_DACVAL4,  NUMER_RAPPORT_DACVAL5,  NUMER_RAPPORT_DACVAL6,
	NUMER_RAPPORT_DACVAL7, NUMER_RAPPORT_DACVAL8, NUMER_RAPPORT_DACVAL9, NUMER_RAPPORT_DACVAL10, NUMER_RAPPORT_DACVAL11, NUMER_RAPPORT_DACVAL12,
	NUMER_RAPPORT_SLOW1,NUMER_RAPPORT_SLOW2,NUMER_RAPPORT_SLOW3,NUMER_RAPPORT_SLOW4,NUMER_RAPPORT_SLOW5,NUMER_RAPPORT_SLOW6,
	NUMER_RAPPORT_NB
} NUMER_RAPPORT_ITEM;
NUMERISEUR_VAR char *NumeriseurRapportItem[]
#ifdef NUMERISEURS_C
 = {
	"neant", "ident", "temper", "d2", "d3", "alim",
	"ref", "AdcOn1", "AdcOn2", "AdcOn3", "AdcOn4", "r1", "r2",
	"AdcVal1", "AdcVal2", "AdcVal3", "AdcVal4", "AdcVal5", "AdcVal6",
	"Rt", "Rg1", "Rg2", "Rg3", "Rg4",
	"DacVal1", "DacVal2", "DacVal3", "DacVal4",  "DacVal5",  "DacVal6",
	"DacVal7", "DacVal8", "DacVal9", "DacVal10", "DacVal11", "DacVal12",
	"SlwVal1", "SlwVal2", "SlwVal3", "SlwVal4",  "SlwVal5",  "SlwVal6",
	"\0"
}
#endif
;
NUMERISEUR_VAR char *NumeriseurRapportRelais
#ifdef NUMERISEURS_C
 = "mflx"
#endif
;
#define NUMER_RAPPORT_ADC(i) ((i < -25000)? '<': ((i < -10000)? '-': ((i < 10000)? '.' : ((i < 25000)? '+': '>'))))
#define NUMER_RAPPORT_DAC_REEL(v) ((v <= -1.0)? '<': ((v < -0.03)? '0'+(char)(-v/0.1): ((v < 0.03)? '.' : ((v < 1.0)? 'a'+(char)(v/0.1): '>'))))
#define NUMER_RAPPORT_DAC_HEXA(h) ((h <= (0x8000-3000))? '<': ((h < (0x8000-100))? '0'+(char)((0x8000-h)/300): ((h < (0x8000+100))? '.' : ((h < (0x8000+3000))? 'a'+(char)((h-0x8000)/300): '>'))))
// ( val < -3000 ? '<' : val > 3000 ? '>' : val<-100 ? '0' - (val/300) : val>100 ? 'a'+ (val/300) : '.' ) 
#ifdef NUMER_ETAT_CONDENSE
#define NUMER_RAPPORT_HEXA(h) ((h <= 0x400)? '<': ((h <= (0x8000-0x6000))? '-': ((h < (0x8000-(0x6000/32)))? '0'+(char)((0x8000-h)/(0x6000/10)): ((h < (0x8000+(0x6000/32)))? '.' : ((h < (0x8000+0x6000))? 'a'+(char)((h-0x8000)/(0x6000/10)): ((h < 0x7BFF)? '+': '>'))))))
/* soit:
 representation signee:                                          |  en pourcentage:
         |adu| <   768 (0x0300) : '.'                            |  0,0 | .. |   2,3 |
   768 < |adu| < 24576 (0x6000) : {'0' .. '9'} | {'a' .. 'j'}    |  2,3 | .. |  75,0 | (par pas de 7,5%)
 24576 < |adu| < 31744 (0x7C00) : '-' | '+'                      | 75,0 | .. |  96,9 |
 31744 < |adu|                  : '<' | '>'                      | 96,6 | .. | 100.0 |
 
 representation non signee:                   |  en pourcentage signé:
         adu <  1024 (0x0400) : '<'           | -100,0 | .. | -96.6 |
  1024 < adu <  8192 (0x2000) : '-'           |  -96,9 | .. | -75,0 |
  8192 < adu < 32000 (0x7D00) : '9' .. '0'    |  -75,0 | .. |  -2,3 | (par pas de 7,5%)
 32000 < adu < 33536 (0x8300) : '.'           |   -2,3 | .. |   2,3 |
 33536 < adu < 57344 (0xE000) : 'a' .. 'j'    |    2,3 | .. |  75,0 | (par pas de 7,5%)
 57344 < adu < 64512 (0xFC00) : '+'           |   75,0 | .. |  96,9 |
 64512 < adu                  : '>'           |   96,6 | .. | 100.0 |
 */
#else  /* NUMER_ETAT_CONDENSE */
#define NUMER_RAPPORT_HEXA1(h) ((h <= 0x66E)? '<': (((h < 0x799A)? '-': ((h < 0x8666)? ' ' : ((h < 0xF992)? '+': '>')))))
#define NUMER_RAPPORT_HEXA2(h) ((h <= 0x66E)? '<': (((h < 0x799A)? '1'+(char)((0x799A-h)/0x0CCC): (((h < 0xF992)? '0'+(char)((h-0x799A)/0x0CCC): '>')))))

#endif /* NUMER_ETAT_CONDENSE */

// NUMER_RESS_NOM defini dans edw_types.h pour cablage.h
#define NUMER_PROC_NOM 16
#define NUMER_ADC_NOM 16

typedef struct {
	char  nom[NUMER_ADC_NOM];
	int   bits;             /* nombre de bits de l'ADC                                      */
	char  signe;            /* vrai si l'ADC est signe                                      */
	float polar;            /* polarisation de l'ADC (Volts)                                */
//	char  ident;            /* vrai si contient l'adresse du numeriseur (en identification) */
//?	TypeDonnee zero;        /* valeur du zero si l'ADC est signe (== bit de signe)          */
//?	lhex extens;            /* extension du bit de signe pour avoir une donnee signee       */
//?	int min,max;            /* Signal recu minimum/maximum (ADU) [pour l'affichage]         */
} TypeADCModele;

typedef struct {
	char fichier[MAXFILENAME];
	char code[NUMER_FPGA_CODE];
	char opera;
} TypeFpgaModl;

#define NUMER_ACCES_MAXFMT 40
#define NUMER_ACCES_MAXVAR 8
#define NUMER_ACCES_VARFMT 16
#define NUMER_ACCES_VARSTR 32
typedef enum {
	NUMER_ACCES_BB = 0,
	NUMER_ACCES_TXT,
	NUMER_ACCES_BIN,
	NUMER_ACCES_NB
} NUMER_ACCES_TYPE;

typedef enum {
	NUMER_VAR_IDENT = 0,
	NUMER_VAR_ADRS,
	NUMER_VAR_NOM,
	NUMER_VAR_VAL,
	NUMER_VAR_HEXA,
	NUMER_VAR_NB
} NUMER_VARS;

#define ACCES_FORMATS "bb/texte/binaire"
#define ACCES_VARIABLES "ident/adrs/nom/valeur/hexa"

typedef struct {
	char cle;
	char fmt[NUMER_ACCES_VARFMT];
} TypeAccesVar;

typedef struct {
	char nom[MODL_NOM];
	char mode; // char media;
	char fmt[NUMER_ACCES_MAXFMT]; /* cas NUMER_ACCES_TXT seulement */
	int  nb;
	TypeAccesVar var[NUMER_ACCES_MAXVAR];
} TypeAccesModl;

#define NUMER_RESS_KEYSTR 80
typedef struct {
	char nom[NUMER_RESS_NOM];
	char categ;                 /* categorie de commande                                    */
	char type;                  /* type de conversion utilisateur <-> hexa                  */
	char unite[NUMER_RESS_NOM]; /* unite representee pour l'utilisateur                     */
	hexa ssadrs;                /* adresse interne du BN                                    */
	hexa masque;                /* masque indiquant le nombre de bits utilises              */
	char  bit0;                 /* si != -1, decalage pour insertion dans hexa a charger    */
	char  init;                 /* si ressource a charger en initialisation                 */
//	char  enreg;                /* si ressource a enregistrer                               */
	shex defaut;                /* valeur par defaut si ressource initialisable (cf <init>) */
	int rapport;                /* type d'information utilisable dans l'etat numeriseurs    */
	short status;               /* place (mot de 16 bits) dans le status relu               */
	short col;                  /* colonne d'affichage dans la fenetre de commande          */
#ifdef FORME_UNION
	union {
		struct { float min,max; } r;   /* flottants */
		struct { int min,max; } i;     /* entiers   */
		struct { char cles[NUMER_RESS_KEYSTR]; shex hval[NUMER_CLES_MAX]; int nb; } k; /* mots-cles */
	} lim;                    /* parametres de conversion                                */
#else
	struct { float min,max; } lim_r;    /* flottants */
	struct { int min,max; } lim_i;      /* entiers   */
	struct { char cles[NUMER_RESS_KEYSTR]; shex hval[NUMER_CLES_MAX]; int nb; } lim_k; /* mots-cles */
#endif
	//? shex (*usertohexa)(void *ptr); /* fonction speciale de conversion user->hexa */
	//? void (*hexatouser)(shex hval, void *ptr); /* idem, hexa->user */
} TypeBNModlRessource;

typedef struct {
	char nom[NUMER_PROC_NOM];
	char fichier[MAXFILENAME];
} TypeModeleBnProc;

typedef struct {
	char  nom[MODL_NOM];
	int   num;                       /* index dans ModeleBN[]                                        */
	int   suivant;
	float version;                   /* version du modele                                            */
	char  a_debloquer;               /* procedure speciale necessaire pour activer les tensions      */
	char  avec_status;               /* capable de delivrer un status de ses ressources              */
	short ident;                     /* version de l'identification (0 si absente)                   */
	int   fpga;                      /* variante de fpga (pour le chargement)                        */
	TypeAccesModl acces;
	int   nbress;                    /* nombre de ressources accessibles                             */
	TypeBNModlRessource ress[NUMER_RESS_MAX];
	int   nbprocs;                   /* nombre de procedures ci-dessous definies                     */
	TypeModeleBnProc    proc[NUMER_PROC_MAX];
	int   nbadc;                     /* nombre d'ADC disponibles                                     */
	int   adc[NUMER_ADC_MAX];        /* liste des ADC disponibles sur ce numeriseur                  */
	float gain[NUMER_ADC_MAX];       /* gains de l'ampli (fixe)                                      */
	int   nbgains;                   /* nombre de gains ci-dessus declares                           */
	char  numress[NUMER_STATUS_DIM]; /* numero de ressource etant donnee une position dans le status */
	char  rapport[NUMER_RAPPORT_NB]; /* numero de ressource etant donnee une position dans le rapport */
	int   nbenregs;                  /* nombre de ressources a enregistrer, ci-dessous definies      */
	char  enreg_nom[NUMER_RESS_MAX][NUMER_RESS_NOM]; /* noms lus dans la definition du modele (* si tous) */
	int   enreg_sts[NUMER_RESS_MAX]; /* position dans le status                                      */
	int   enreg_dim;                 /* nombre de valeurs effectives dans le bloc a enregistrer      */
} TypeBNModele;

NUMERISEUR_VAR TypeADCModele ModeleADC[NUMER_ADC_TYPES],ModeleADCLu;
NUMERISEUR_VAR char *ModeleADCListe[NUMER_ADC_TYPES+1];
NUMERISEUR_VAR int ModeleADCNb;
NUMERISEUR_VAR TypeFpgaModl FpgaModl[NUMER_FPGA_TYPES],FpgaModlLu;
NUMERISEUR_VAR char *FpgaCode[NUMER_FPGA_TYPES+2];
NUMERISEUR_VAR int FpgaNb;
NUMERISEUR_VAR TypeAccesModl AccesModl[NUMER_ACCES_TYPES],AccesModlLu;
NUMERISEUR_VAR int AccesNb;
NUMERISEUR_VAR TypeBNModele ModeleBN[NUMER_TYPES],ModeleBNLu;
NUMERISEUR_VAR char *ModeleNumerListe[NUMER_TYPES+1];
NUMERISEUR_VAR int ModeleBNNb;
NUMERISEUR_VAR int ModlNumerChoisi;

/* .......................... Descriptions .......................... */
NUMERISEUR_VAR ArgDesc ModeleADCDesc[]
#ifdef NUMERISEURS_C
 = {
	{ 0,                DESC_STR(MODL_NOM) ModeleADCLu.nom,   0 },
	{ "bits",           DESC_INT             &ModeleADCLu.bits,  0 },
	{ 0, DESC_KEY "non_signes/signes",       &ModeleADCLu.signe, 0 },
	{ "polar_Volts",     DESC_FLOAT          &ModeleADCLu.polar, 0 }, // "tension de reference" },
	DESC_END
}
#endif
;
NUMERISEUR_VAR ArgStruct ModeleADCAS
#ifdef NUMERISEURS_C
 = { (void *)ModeleADC, (void *)&ModeleADCLu, sizeof(TypeADCModele), ModeleADCDesc }
#endif
;

NUMERISEUR_VAR ArgDesc FpgaDesc[]
#ifdef NUMERISEURS_C
 = {
	{ 0,         DESC_STR(NUMER_FPGA_CODE)   FpgaModlLu.code,    0 },
	{ "fichier", DESC_STR(MAXFILENAME)       FpgaModlLu.fichier, 0 },
	{ "opera",   DESC_BYTE                  &FpgaModlLu.opera,   0 },
	DESC_END
}
#endif
;
NUMERISEUR_VAR ArgStruct FpgaAS
#ifdef NUMERISEURS_C
 = { (void *)FpgaModl, (void *)&FpgaModlLu, sizeof(TypeFpgaModl), FpgaDesc }
#endif
;

void NumeriseurModlAccesInit(void *bloc);

NUMERISEUR_VAR TypeAccesVar ModeleBnVarLue;
NUMERISEUR_VAR ArgDesc ModeleBnVarDesc[]
#ifdef NUMERISEURS_C
= {
	{ "var",    DESC_KEY  ACCES_VARIABLES,  &ModeleBnVarLue.cle, 0 },
	{ "format", DESC_STR(NUMER_ACCES_VARFMT) ModeleBnVarLue.fmt, 0 },
	DESC_END
}
#endif
;
NUMERISEUR_VAR ArgStruct ModeleBnVarAS
#ifdef NUMERISEURS_C
 = { (void *)ModeleBNLu.acces.var, (void *)&ModeleBnVarLue, sizeof(TypeAccesVar), ModeleBnVarDesc }
#endif
;
/* NUMERISEUR_VAR ArgDesc AccesDesc[]
#ifdef NUMERISEURS_C
 = {
	{ 0,              DESC_STR(MODL_NOM)                AccesModlLu.nom,  0 },
	{ "mode",         DESC_KEY ACCES_FORMATS,          &AccesModlLu.mode, 0 },
	{ "format",       DESC_STR(NUMER_ACCES_MAXFMT)      AccesModlLu.fmt,  0 },
	{ "variables",    DESC_KEY_SIZE(AccesModlLu.nb,NUMER_ACCES_MAXVAR) ACCES_VARIABLES, AccesModlLu.var, 0 },
	{ DESC_END_DEFAUT (void *)NumeriseurModlAccesInit, (char *)&AccesModlLu },
}
#endif
;
NUMERISEUR_VAR ArgStruct AccesAS
#ifdef NUMERISEURS_C
 = { (void *)AccesModl, (void *)&AccesModlLu, sizeof(TypeAccesModl), AccesDesc }
#endif
; */

NUMERISEUR_VAR ArgDesc ModeleBnAccesDesc[]
#ifdef NUMERISEURS_C
 = {
	{ "mode",    DESC_KEY ACCES_FORMATS,                                  &ModeleBNLu.acces.mode, 0 },
	{ "format",  DESC_STR(NUMER_ACCES_MAXFMT)                              ModeleBNLu.acces.fmt,  0 },
	{ "donnees", DESC_STRUCT_SIZE(ModeleBNLu.acces.nb,NUMER_ACCES_MAXVAR) &ModeleBnVarAS, 0 },
	{ DESC_END_DEFAUT (void *)NumeriseurModlAccesInit, (char *)&(ModeleBNLu.acces) },
}
#endif
;

NUMERISEUR_VAR TypeBNModlRessource ModeleBnRessLu;
/* le dernier gadget a la mode: */
NUMERISEUR_VAR ArgDesc ModeleBnRessIntDesc[]
#ifdef NUMERISEURS_C
 = {
	{ "int.min",   DESC_INT           &ModeleBnRessLu.lim_i.min, 0 },
	{ "int.max",   DESC_INT           &ModeleBnRessLu.lim_i.max, 0 },
	DESC_END
}
#endif
;
NUMERISEUR_VAR ArgDesc ModeleBnRessFloatDesc[]
#ifdef NUMERISEURS_C
 = {
	{ "float.min", DESC_FLOAT         &ModeleBnRessLu.lim_r.min, 0 },
	{ "float.max", DESC_FLOAT         &ModeleBnRessLu.lim_r.max, 0 },
	DESC_END
}
#endif
;
NUMERISEUR_VAR ArgDesc ModeleBnRessKeyDesc[]
#ifdef NUMERISEURS_C
 = {
	{ "key.textes",  DESC_STR(NUMER_RESS_KEYSTR)                           ModeleBnRessLu.lim_k.cles, 0 },
	{ "key.valeurs", DESC_SHEX_SIZE(ModeleBnRessLu.lim_k.nb,NUMER_CLES_MAX) ModeleBnRessLu.lim_k.hval, 0 },
	DESC_END
}
#endif
;
NUMERISEUR_VAR ArgDesc ModeleBnRessPuisDesc[]
#ifdef NUMERISEURS_C
 = {
	{ "bit.min",   DESC_INT           &ModeleBnRessLu.lim_i.min, 0 },
	{ "val.min",   DESC_INT           &ModeleBnRessLu.lim_i.max, 0 },
	DESC_END
}
#endif
;
NUMERISEUR_VAR ArgDesc *ModeleBnRessOptnDesc[]
#ifdef NUMERISEURS_C
 = { ModeleBnRessIntDesc, ModeleBnRessFloatDesc, ModeleBnRessKeyDesc, ModeleBnRessPuisDesc, 0 }
#endif
;
NUMERISEUR_VAR ArgDesc ModeleBnRessDesc[]
#ifdef NUMERISEURS_C
 = {
	{ 0,           DESC_STR(NUMER_RESS_NOM)           ModeleBnRessLu.nom,     0 },
//	{ 0,           DESC_KEY RESS_CATEGTEXTE,         &ModeleBnRessLu.categ,   0 },
	{ "categorie", DESC_KEY RESS_CATEGTEXTE,         &ModeleBnRessLu.categ,   0 },
	{ "adresse",   DESC_HEXA                         &ModeleBnRessLu.ssadrs,  0 },
	{ "masque",    DESC_HEXA                         &ModeleBnRessLu.masque,  0 },
	{ "bit0",      DESC_BYTE                         &ModeleBnRessLu.bit0,    0 },
	{ "status",    DESC_SHORT                        &ModeleBnRessLu.status,  0 },
	{ "init",      DESC_BOOL                         &ModeleBnRessLu.init,    0 },
	{ "defaut",    DESC_SHEX                         &ModeleBnRessLu.defaut,  0 },
	{ "rapport",   DESC_LISTE NumeriseurRapportItem, &ModeleBnRessLu.rapport, 0 },
	{ "colonne",   DESC_SHORT                        &ModeleBnRessLu.col,     0 },
	{ "unite",     DESC_STR(NUMER_RESS_NOM)          &ModeleBnRessLu.unite,   0 },
	{ "codage",    DESC_KEY RESS_TYPETEXTE,          &ModeleBnRessLu.type,    0 },
	{ 0,           DESC_VARIANTE(ModeleBnRessLu.type) ModeleBnRessOptnDesc,   0 },
	DESC_END
}
#endif
;
/* Ca au moins, ca marche:
NUMERISEUR_VAR ArgDesc ModeleBnRessDesc[] = {
	{ 0,     DESC_STR(NUMER_RESS_NOM) ModeleBnRessLu.nom,     0 },
	{ "categorie", DESC_KEY "divers/polar/gain/modul/comp/corr/d2/d3/fet", &ModeleBnRessLu.categ, 0 },
	{ "unite",     DESC_STR(NUMER_RESS_NOM) &ModeleBnRessLu.unite,     0 },
	{ "voie",      DESC_SHORT         &ModeleBnRessLu.voie,      0 },
	{ "adresse",   DESC_SHEX          &ModeleBnRessLu.ssadrs,    0 },
	{ "masque",    DESC_HEXA          &ModeleBnRessLu.masque,    0 },
	{ "decalage",  DESC_BYTE          &ModeleBnRessLu.bit0,      0 },
	{ "codage",    DESC_KEY "int/float/key/puis2",  &ModeleBnRessLu.type, 0 },
	{ "float.min", DESC_FLOAT         &ModeleBnRessLu.lim_r.min, 0 },
	{ "float.max", DESC_FLOAT         &ModeleBnRessLu.lim_r.max, 0 },
	{ "int.min",   DESC_INT           &ModeleBnRessLu.lim_i.min, 0 },
	{ "int.max",   DESC_INT           &ModeleBnRessLu.lim_i.max, 0 },
	{ "key.textes",  DESC_STR(NUMER_RESS_KEYSTR)  ModeleBnRessLu.lim_k.cles, 0 },
	{ "key.valeurs", DESC_SHEX_SIZE(ModeleBnRessLu.lim_k.nb,NUMER_CLES_MAX) ModeleBnRessLu.lim_k.hval, 0 },
	{ "colonne",   DESC_SHORT         &ModeleBnRessLu.col,       0 },
	DESC_END
}; */
NUMERISEUR_VAR ArgStruct ModeleBnRessAS
#ifdef NUMERISEURS_C
 = { (void *)ModeleBNLu.ress, (void *)&ModeleBnRessLu, sizeof(TypeBNModlRessource), ModeleBnRessDesc }
#endif
;

NUMERISEUR_VAR TypeModeleBnProc ModeleBnProcLue;
NUMERISEUR_VAR ArgDesc ModeleBnProcDesc[]
#ifdef NUMERISEURS_C
 = {
	{ 0, DESC_STR(NUMER_PROC_NOM) ModeleBnProcLue.nom,     0 },
	{ 0, DESC_STR(MAXFILENAME)    ModeleBnProcLue.fichier, 0 },
	DESC_END
 }
#endif
;
NUMERISEUR_VAR ArgStruct ModeleBnProcAS
#ifdef NUMERISEURS_C
 = { (void *)ModeleBNLu.proc, (void *)&ModeleBnProcLue, sizeof(TypeModeleBnProc), ModeleBnProcDesc }
#endif
;

#ifdef NUMERISEURS_C
	NUMERISEUR_VAR ArgStruct ModeleBnAccesAS = { 0, 0, 0, ModeleBnAccesDesc };
#endif
void NumeriseurModlInit(void *bloc);
NUMERISEUR_VAR ArgDesc ModeleBNDesc[]
#ifdef NUMERISEURS_C
 = {
	{ 0,            DESC_STR(MODL_NOM)           ModeleBNLu.nom,         0 },
	{ "version",    DESC_FLOAT                  &ModeleBNLu.version,     0 },
	{ "inactive",   DESC_BOOL                   &ModeleBNLu.a_debloquer, 0 },
	{ "ident",      DESC_SHORT                  &ModeleBNLu.ident,       0 }, /* vsn ident, attention si 0, pas de bb a demarrer! */
	{ "fpga",       DESC_LISTE FpgaCode,        &ModeleBNLu.fpga,        0 }, /* atttention: decalage de 1 pour accepter 'neant' (cf NumeriseursModlEpilogue) */
//+	{ "acces",      DESC_STRUCT                 &ModeleBnAccesDesc,      0 },
	{ "acces",      DESC_STRUCT_DIM(1)          &ModeleBnAccesAS,        0 },
	{ "ressources", DESC_STRUCT_SIZE(ModeleBNLu.nbress,NUMER_RESS_MAX)              &ModeleBnRessAS,       0 },
	{ "adc",        DESC_LISTE_SIZE(ModeleBNLu.nbadc,NUMER_ADC_MAX) ModeleADCListe,  ModeleBNLu.adc,       0 },
	{ "gain",       DESC_FLOAT_SIZE(ModeleBNLu.nbgains,NUMER_ADC_MAX)                ModeleBNLu.gain,      0 },
	{ "procedures", DESC_STRUCT_SIZE(ModeleBNLu.nbprocs,NUMER_PROC_MAX)             &ModeleBnProcAS,       0 },
	{ "enregistre", DESC_STR_SIZE(NUMER_RESS_NOM,ModeleBNLu.nbenregs,NUMER_RESS_MAX) ModeleBNLu.enreg_nom, 0 },
	{ DESC_END_DEFAUT (void *)NumeriseurModlInit, (char *)&ModeleBNLu },
}
#endif
;
NUMERISEUR_VAR ArgStruct ModeleBNAS
#ifdef NUMERISEURS_C
 = { (void *)ModeleBN, (void *)&ModeleBNLu, sizeof(TypeBNModele), ModeleBNDesc }
#endif
;

NUMERISEUR_VAR ArgDesc NumerModlDesc[]
#ifdef NUMERISEURS_C
 = {
	{ "ADC",         DESC_STRUCT_SIZE(ModeleADCNb,NUMER_ADC_TYPES) &ModeleADCAS, 0 },
	{ "FPGA",        DESC_STRUCT_SIZE(FpgaNb,NUMER_FPGA_TYPES)     &FpgaAS,      0 },
//	{ "Acces",       DESC_STRUCT_SIZE(AccesNb,NUMER_ACCES_TYPES)   &AccesAS,     0 },
	{ "Numeriseurs", DESC_STRUCT_SIZE(ModeleBNNb,NUMER_TYPES)      &ModeleBNAS,  0 },
	DESC_END
}
#endif
;

int NumerModlRessIndex(TypeBNModele *modele_bn, char *nom);

/* .......................................................................... */
/*                        M A T E R I E L   U T I L I S E                     */

#define NUMER_DIFFUSION 0xFF    /* adresse pour diffusion */

#define NUMER_ABSENT -1
#define NUMER_TOUS -1

typedef enum {
	NUMER_ENTREE = 0,
	NUMER_ECHANGE,
	NUMER_SORTIE,
	NUMER_OPER_NB
} NUMER_OPERATION;
NUMERISEUR_VAR char *NumeriseurOper[NUMER_OPER_NB+1]
#ifdef NUMERISEURS_C
= { "entree", "echange", "sortie", "\0" }
#endif
;

typedef enum {
	NUMER_CONNECTION = 0,
	NUMER_RESSOURCES,
	NUMER_TOUT,
	NUMER_CONTENU_NB
} NUMER_CONTENU;

typedef enum {
	NUMER_TRACE_DAC = 0,
	NUMER_TRACE_ADC,
	NUMER_TRACE_NB
} NUMER_TRACE_TYPE;

typedef struct {
	int           type;      /* type de numeriseur                                  */
	byte serial;    /* numero de serie du numeriseur                       */
	byte adrs;      /* adresse (donnee par le piano)                       */
	float         gain[NUMER_ADC_MAX]; /* gains de l'ampli (fixe)                    */
	int           nbgains;   /* nombre de gains ci-dessus declares                  */
	char          fibre[FIBRE_NOM]; /* nom de la liaison avec le repartiteur        */
} TypeNumeriseurDef;

typedef struct {
	char           init;     /* participe a l'initialisation du numeriseur          */
	shex hval;     /* valeur a envoyer dans la ressource                  */
	union {
		float r;             /* valeur a utiliser si la ressource est un flottant   */
		int i;               /* valeur a utiliser sinon (entiers, No de cle, ..)    */
	} val;
	float hex_to_rval;       /* facteur de conversion hexa vers user                */
	float rval_to_hex;       /* facteur de conversion user vers hexa                */
	float correctif;         /* rectification de la dynamique pour les flottants    */
	char cval[NUMER_RESS_VALTXT]; /* valeur a afficher/lire pour l'utilisateur      */
	TypeReglage *regl;       /* reglage de detecteur vise                           */
	struct {
		TypeTamponDonnees tampon;
		Graph g; int y;
	} historique;
	char a_charger;          /* en cours de modification par l'utilisateur          */
} TypeNumeriseurRess;

typedef struct {
	int niv;
	char codees,brutes,hexa;
} TypeNumeriseurPlancheOptn;

typedef struct {
	Cadre planche;
	char a_rafraichir; char verrouillee;
	short prems,nb;
	TypeNumeriseurPlancheOptn optn;
	Menu pave,bouton,blocage;
	// Menu charge;
} TypeNumeriseurCmde;

typedef struct {
	void *numeriseur;
	NUMER_TRACE_TYPE type; /* necessaire dans NumeriseurTracage */
	Graph g;
	int nb /* ,prem */;
} TypeNumeriseurTrace;

typedef struct {
	char             nom[NUMER_NOM];
	int              bb;          /* index dans le tableau                               */
	TypeNumeriseurDef def;
	char fichier[MAXFILENAME];    /* si non vide, fichier ou ecrire ce numeriseur        */
	TypeCableConnecteur fischer;  /* numero de connecteur sortant du cryostat            */
	float            version;     /* version                                             */
	char             avec_status; /* capable de delivrer un status de ses ressources     */
	char             vsnid;       /* version d'identification                            */
	char             local;       /* vrai s'il est raccorde a un detecteur local         */
	char             a_lire;      /* vrai si doit transmettre une voie lue               */
	char             sel;         /* vrai si selectionne                                 */
//	char             fmt;         /* type de donnees transmises                          */
	shex   ident;       /* identificateur du numeriseur                        */
	shex   adc_id[NUMER_ADC_MAX]; /* identificateurs des ADC                   */
	int              nbident;     /* nombre d'identificateurs des ADC (== nb ADC)        */
	char             change_id;   /* vrai si les identificateurs ont ete corriges        */
	int              nbadc;       /* nombre d'ADC et partant, nombre de voies delivrees  */
	short           *voie;        /* liste desdites                                      */
	ArgDesc         *desc;        /* descriptif des registres                            */
	char             code_rep[8];
	short            entree;      /* numero d'entree dans le repartiteur (entree[l])     */
	TypeRepartiteur *repart;      /* repartiteur a utiliser                              */
	TypeNumeriseurRess  ress[NUMER_RESS_MAX];
	TypeNumeriseurCmde  interface; /* structure pour manipuler les ressources            */
	TypeNumeriseurTrace trace[NUMER_TRACE_NB];
	TypeModeleBnProc    proc[NUMER_PROC_MAX];
	int              nbprocs;     /* nombre de procedures ci-dessus definies             */
	int              numproc;     /* numero de la procedure a appliquer                  */
	char            *nomproc[NUMER_PROC_MAX+1];   /* pointeur sur les noms de procedure (= liste desdites) */
	TypeHwExecContext exec;       /* contexte d'execution de script                      */
	char             bloque;      /* vrai si polars bloquees a 0 (BB1 SAC)               */
	char             verrouille;  /* vrai si changements verrouilles (BB2)               */
	int debut;                    /* pour programme d'initialisation particulier         */
	int taille;                   /*             ---     ditto      ---                  */
	struct { char demande; char a_copier; char termine; short nb; char lu[12]; int stamp; } status; /* etat de la lecture du status */
	shex status_val[NUMER_RESS_MAX]; /* table desdites valeurs                  */
	shex enreg_val[NUMER_RESS_MAX];  /* table de ces valeurs a enregistrer      */
	char enreg_txt[MAXFILENAME];  /* fichier individuel temporaire (monitoring web)       */
	int64 enreg_stamp;            /* l'enregistrement est pret si /= 0                    */
} TypeNumeriseur;

#define NUMER_INSTRM_MAX (NUMER_RESS_MAX * NUMER_MAX)
typedef struct {
	char type;                  /* type d'interface associee    */
	void *instrum;              /* adresse de l'interface       */
	TypeReglage *reglage;       /* reglage de detecteur associe */
	TypeNumeriseur *numeriseur; /* adresse du numeriseur        */
	short num;                  /* numero de reglage numeriseur */
} TypeNumeriseurInstrum;

NUMERISEUR_VAR TypeNumeriseur Numeriseur[NUMER_MAX+1];
NUMERISEUR_VAR char *NumeriseurListe[NUMER_MAX+1];
NUMERISEUR_VAR int NumeriseurNb;
NUMERISEUR_VAR TypeNumeriseurInstrum NumeriseurInstr[NUMER_INSTRM_MAX];
NUMERISEUR_VAR int NumeriseurInstrNb;
NUMERISEUR_VAR char NumeriseurPeutStopper,NumeriseurEtatPeutStopper;
NUMERISEUR_VAR char NumeriseurAcharger,NumeriseurAdemarrer,NumeriseurIdentifiable,NumeriseurAvecStatus;

NUMERISEUR_VAR char NumeriseurFichierLu[MAXFILENAME];
NUMERISEUR_VAR char NumeriseurRacine[MAXFILENAME];
NUMERISEUR_VAR char NumeriseurStock[MAXFILENAME];

// #define NUMER_ETAT_CONDENSE
#ifdef NUMER_ETAT_CONDENSE
#define NUMER_ETAT_LNGR 96
#else
#define NUMER_ETAT_LNGR 150
#endif
NUMERISEUR_VAR struct {
	char nom[16];
	char texte[NUMER_ETAT_LNGR+1];
	char *detec;
} NumeriseurEtat[NUMER_MAX];
NUMERISEUR_VAR char NumeriseurEtatEntete[NUMER_ETAT_LNGR+1];
NUMERISEUR_VAR Cadre bNumeriseurEtat;
NUMERISEUR_VAR Instrum iNumeriseurEtatMaj;
NUMERISEUR_VAR char NumeriseurEtatDemande;
NUMERISEUR_VAR int  NumeriseurEtatStamp;
NUMERISEUR_VAR char NumeriseurEtatDate[12];

#define NUMER_ANONM_MAX 3
NUMERISEUR_VAR TypeNumeriseur NumeriseurAnonyme[NUMER_ANONM_MAX];
// NUMERISEUR_VAR int NumeriseurNbAnonm;
#define NUMER_BB_VSNMAX 4
NUMERISEUR_VAR int NumeriseurTypeBB[NUMER_BB_VSNMAX+1],NumeriseurTypeCALI;

NUMERISEUR_VAR struct {
	int bb;
	char oper;
	TypeNumeriseur *adrs;
	TypeNumeriseurPlancheOptn optn;
} NumeriseurChoix;

//typedef enum {
//	NUMER_CODE_UNDEF = 0, NUMER_CODE_COMPTEUR, NUMER_CODE_ACQUIS = CLUZEL_CODE_ACQUIS, NUMER_CODE_IDENT = CLUZEL_CODE_IDENT       
//} NUMER_CODE_DONNEES;
//NUMERISEUR_VAR NUMER_CODE_DONNEES NumerModeCode[NUMER_MODE_NB]
//#ifdef NUMERISEURS_C
// = { NUMER_CODE_UNDEF, NUMER_CODE_COMPTEUR, NUMER_CODE_ACQUIS, NUMER_CODE_IDENT }
//#endif
//;

int NumeriseurRessDown(Menu menu);
int NumeriseurRessUp(Menu menu);
NUMERISEUR_VAR TypeMenuItem iNumeriseurRessUpDown[]
#ifdef NUMERISEURS_C
 = {
	{ "+", MNU_FONCTION NumeriseurRessUp   },
	{ "-", MNU_FONCTION NumeriseurRessDown },
	MNU_END
}
#endif
;

char NumeriseursModlLit(char *fichier);
void NumeriseursModlProcedures(char *chemin_modeles);
void NumeriseursModlAssure();
void NumeriseursModlEpilogue();
void NumeriseursModlImprime();

void NumeriseurNeuf(TypeNumeriseur *numeriseur, int type, shex ident, TypeRepartiteur *repart);
FILE *NumeriseurDefLit(char *item, TypeNumeriseurDef *def, char *nom_local, char *fichier_bb, char *fin);
void NumeriseurComplete(int bb, TypeBNModele *modele_bn, TypeCableConnecteur connecteur, 
						TypeNumeriseurDef *def, FILE *h, char *nom_local, char fin);
int  NumeriseursLit(char *fichier);
void NumeriseursEcrit(char *fichier, char quoi, int qui);
void NumeriseurTermineAjout();
int  NumeriseursDemarre();
short NumeriseurMaxVoies(TypeNumeriseur *numeriseur);
void NumeriseursBB1Debloque(TypeNumeriseur *numeriseur, char toujours, char *prefixe);
int  NumeriseursSelectionneLarge();
int  NumeriseursDebloqueTous();
int  NumeriseursInitTous();
int  NumeriseursRebloqueTous();
int NumeriseurAvecFibre(char *fibre);
char NumeriseurPlace(FILE *f, char *nom_local, TypeCableConnecteur connecteur);
int NumeriseurSelection(char exclusive);
int NumeriseurEtatEnCours(Instrum instrum, void *adrs);
void NumeriseurRapportEtabli(TypeNumeriseur *numeriseur);
shex NumeriseurRessCurrentVal(TypeNumeriseur *numeriseur, int num, int *i, float *r);
shex NumeriseurRessUserDecode(TypeNumeriseur *numeriseur, int num, char *cval, int *i, float *r);
void NumeriseurRessFloatDecode(TypeNumeriseur *numeriseur, short num, float rval, shex *hval);
shex NumeriseurRessValEncode(TypeNumeriseur *numeriseur, int num, char *cval, int ival, float rval);
shex NumeriseurRessHexaEncode(TypeNumeriseur *numeriseur, int num, shex hval, char *cval, int *i, float *r);
void NumeriseurRessUserChange(TypeNumeriseur *numeriseur, int num);
void NumeriseurRessHexaChange(TypeNumeriseur *numeriseur, int num);
void NumeriseurRessValChangee(TypeNumeriseur *numeriseur, int num);
INLINE char NumeriseurChargeValeurBB(TypeNumeriseur *numeriseur, hexa ss_adrs, TypeADU valeur);
INLINE void NumeriseurAutoriseBB2(TypeNumeriseur *numeriseur, char autorise, char log);
INLINE char NumeriseurAutoriseAcces(TypeNumeriseur *numeriseur, char autorise, char log);
shex NumeriseurRessourceChargeAuto(TypeNumeriseur *numeriseur, int k, char *prefixe, char *marge);
shex NumeriseurAdrsChargeAuto(TypeNumeriseur *numeriseur, hexa ssadrs, shex hval, char *suite, char *marge);
void NumeriseurChargeFin(char *prefixe);
shex NumeriseurRessourceCharge(TypeNumeriseur *numeriseur, int k, char *prefixe);
int  NumeriseurCharge(TypeNumeriseur *numeriseur, char tout, char log);
INLINE void NumeriseurRecopieStatus(TypeNumeriseur *numeriseur, short index, hexa lu);
char NumeriseurProcExec(TypeNumeriseur *numeriseur, int p);
int  NumeriseurRapport(Menu menu, MenuItem item);
int  NumeriseurRessFromVal(void *instrum, void *arg);
int  NumeriseurRessFromADU(Panel panel, int num, void *arg);
int  NumeriseurRessFromUser(Panel panel, int num, void *arg);
int  NumeriseurRessFromKey(Panel panel, int num, void *arg);
char NumeriseurRessEnCours(void *adrs, char *texte, void *arg);
int  NumeriseurAfficheChoix(TypeNumeriseur *numeriseur);
int  NumeriseurActionSouris(TypeNumeriseur *numeriseur, WndAction *e);
int  NumeriseurLectClique(Figure fig, WndAction *e);
int  NumeriseurOperationSouris(TypeNumeriseur *numeriseur, WndAction *e);
int  NumeriseurOrgaClique(Figure fig, WndAction *e);
int  NumeriseurExec();
char NumeriseurRafraichi(TypeNumeriseur *numeriseur);
void NumeriseurArreteTraces(TypeNumeriseur *numeriseur);

#endif

