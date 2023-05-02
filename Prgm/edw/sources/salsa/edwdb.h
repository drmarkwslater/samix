#ifndef EDWDB_H
#define EDWDB_H

#include <defineVAR.h>
#include <claps.h>
#include <db.h>
#include <dimensions.h>


typedef struct {
	char *type;
	ArgDesc *desc;
} TypeEdwDbElt,*EdwDbElt;

typedef enum {
	EDWDB_EVTMOYEN = 0,
	EDWDB_PATTERN,
	EDWDB_VI,
	EDWDB_BRUIT,
	EDWDB_NBTYPES
} EDWDB_TYPE;

#define EDWDB_NOM_EVTMOYEN "evtmoyen"
#define EDWDB_NOM_PATTERN  "pattern"
#define EDWDB_NOM_VI       "Vi"
#define EDWDB_NOM_BRUIT    "bruit"

#ifdef EDWDB_C
#define EDWDB_VAR
#else
#define EDWDB_VAR extern
#endif

EDWDB_VAR char *EdwTypeNom[EDWDB_NBTYPES+1]
#ifdef EDWDB_C
 = { EDWDB_NOM_EVTMOYEN, EDWDB_NOM_PATTERN, EDWDB_NOM_VI, EDWDB_NOM_BRUIT, "\0" }
#endif
;
/* 
 * ..... element de base de donnees de type "evtmoyen" .........................
 */
typedef struct {
	char  nom[VOIE_NOM];    /* nom complet de la voie                    */
	float horloge;          /* duree d'un point (millisecs)              */
	float valeur;           /* valeur representee                        */
	char  unite[UNITE_NOM];  /* type de valeur (ADU, nV, keV, .. )        */
	float amplitude;        /* nombre d'ADU correspondant                */
	int   taille;           /* dimension du tableau                      */
	int   pre_trigger;      /* nombre de points jusqu'au signal physique */
	int   depart,arrivee;   /* intervalle de validite pour le fit        */
	float *val;
} TypeEvtTemplate;

EDWDB_VAR TypeEvtTemplate EdwEvtInfo;
EDWDB_VAR ArgDesc EdwEvtDesc[]
#ifdef EDWDB_C
= {
	{ "voie",        DESC_TEXT   EdwEvtInfo.nom, 0 },
	{ "horloge",     DESC_FLOAT &EdwEvtInfo.horloge, "ms" },
	{ "valeur",      DESC_FLOAT &EdwEvtInfo.valeur, "en unite ci-dessous" },
	{ "unite",       DESC_TEXT   EdwEvtInfo.unite, 0 },
	{ "amplitude",   DESC_FLOAT &EdwEvtInfo.amplitude, "ADU" },
	{ "taille",      DESC_INT   &EdwEvtInfo.taille, 0 },
	{ "pre-trigger", DESC_INT   &EdwEvtInfo.pre_trigger, 0 },
	{ "debut",       DESC_INT   &EdwEvtInfo.depart, 0 },
	{ "fin",         DESC_INT   &EdwEvtInfo.arrivee, 0 },
	{ "profil",      DESC_FLOAT_ADRS(EdwEvtInfo.taille) &EdwEvtInfo.val, 0 },
	DESC_END
}
#endif
;
EDWDB_VAR TypeEdwDbElt EdwEvt
#ifdef EDWDB_C
= { EDWDB_NOM_EVTMOYEN, EdwEvtDesc }
#endif
;

/* 
 * ..... element de base de donnees de type "pattern" ..........................
 */
typedef struct {
	char  nom[VOIE_NOM]; /* nom complet de la voie        */
	float horloge;          /* duree d'un point (millisecs)  */
	int   taille;           /* dimension du tableau          */
	float *val;             /* profil                        */
} TypePattern;

EDWDB_VAR TypePattern EdwPatternInfo;
EDWDB_VAR ArgDesc EdwPatternDesc[]
#ifdef EDWDB_C
= {
	{ "voie",        DESC_TEXT   EdwPatternInfo.nom, 0 },
	{ "horloge",     DESC_FLOAT &EdwPatternInfo.horloge, "ms" },
	{ "taille",      DESC_INT   &EdwPatternInfo.taille, 0 },
	{ "profil",      DESC_FLOAT_ADRS(EdwPatternInfo.taille) &EdwPatternInfo.val, 0 },
	DESC_END
}
#endif
;
EDWDB_VAR TypeEdwDbElt EdwPattern
#ifdef EDWDB_C
= { EDWDB_NOM_PATTERN, EdwPatternDesc }
#endif
;

/*
 * ...... element de base de donnees de type "V(I)" ............................
 */
typedef struct StructCourbeVi {
	char nom[DETEC_NOM];
	float tempe; /* mK */
	int date,heure;
	int nb;
	float *I,*V;
	struct StructCourbeVi *suivante;
} TypeCourbeVi;

EDWDB_VAR TypeCourbeVi EdwViInfo;
EDWDB_VAR ArgDesc EdwViDesc[]
#ifdef EDWDB_C
= {
	{ "voie",        DESC_TEXT   EdwViInfo.nom, 0 },
	{ "tempe",       DESC_FLOAT &EdwViInfo.tempe, "mK" },
	{ "date",        DESC_INT   &EdwViInfo.date, 0 },
	{ "heure",       DESC_INT   &EdwViInfo.heure, 0 },
	{ "taille",      DESC_INT   &EdwViInfo.nb, 0 },
	{ "I",           DESC_FLOAT_ADRS(EdwViInfo.nb) &EdwViInfo.I, 0 },
	{ "V",           DESC_FLOAT_ADRS(EdwViInfo.nb) &EdwViInfo.V, 0 },
	DESC_END
}
#endif
;
EDWDB_VAR TypeEdwDbElt EdwVi
#ifdef EDWDB_C
= { EDWDB_NOM_VI, EdwViDesc }
#endif
;
/* .......................................................................... */

/*
 * ...... element de base de donnees de type "bruit" ...........................
 */
#define MAXTRMTNOM 16
#define MAXCONDNOM 32
typedef struct {
	char  nom[VOIE_NOM];
	char  type;                   /* tampon utilise                */
	char  au_vol[MAXTRMTNOM],sur_tampon[MAXTRMTNOM];
	char  type_bn[MODL_NOM];   /* type de numeriseur            */
	byte serial;          /* numero de serie du numeriseur */
	long  date,heure;
	char  cond[MAXCONDNOM]; char srce[TYPERUN_NOM];
	float tempe_bolos,tempe_chateau,radon;
	char  tubes,lique,ventil;
	float horloge;                /* duree d'un point (millisecs)  */
	int   d2;
	int   nbtemps,nbfreq;
	short *temps;
	float *freq;
} TypeSpBruit;

EDWDB_VAR TypeSpBruit EdwSpBruitInfo;
EDWDB_VAR ArgDesc EdwSpBruitDesc[]
#ifdef EDWDB_C
= {
	{ "voie",            DESC_TEXT                    EdwSpBruitInfo.nom,           0 },
	{ "type",            DESC_KEY "brute/traitee",  &EdwSpBruitInfo.type,          0 },
/* En principe, rundata.h ne devrait pas etre declare obligatoirement avec db.h
	{ "trmt-au-vol",     DESC_KEY TrmtTypeCles,     &EdwSpBruitInfo.au_vol,        "Traitement au vol" },
	{ "trmt-sur-tampon", DESC_KEY TrmtTypeCles,     &EdwSpBruitInfo.sur_tampon,    "Traitement pre-trigger" }, */
	{ "trmt-au-vol",     DESC_TEXT                    EdwSpBruitInfo.au_vol,        "Traitement au vol" },
	{ "trmt-sur-tampon", DESC_TEXT                    EdwSpBruitInfo.sur_tampon,    "Traitement pre-trigger" },
	{ "numeriseur",      DESC_TEXT                    EdwSpBruitInfo.type_bn,       0},
	{ "numero",          DESC_OCTET                  &EdwSpBruitInfo.serial,        "du numeriseur" },
	{ "date",            DESC_INT                    &EdwSpBruitInfo.date,          0 },
	{ "heure",           DESC_INT                    &EdwSpBruitInfo.heure,         0 },
	{ "secondes",        DESC_INT                    &EdwSpBruitInfo.date,          0 },
	{ "microsecs",       DESC_INT                    &EdwSpBruitInfo.heure,         0 },
/* Meme remarque que pour TrmtTypeCles..
	{ "conditions",      DESC_KEY RunEnvir,         &EdwSpBruitInfo.cond,          "Conditions du run" },
	{ "source",          DESC_KEY RunCategCles,         &EdwSpBruitInfo.srce,          0 }, */
	{ "conditions",      DESC_TEXT                    EdwSpBruitInfo.cond,          "Conditions du run" },
	{ "source",          DESC_TEXT                    EdwSpBruitInfo.srce,          0 },
	{ "tempe.bolos",     DESC_FLOAT                  &EdwSpBruitInfo.tempe_bolos,   "Temperature de la platine bolo (K)" },
	{ "tempe.chateau",   DESC_FLOAT                  &EdwSpBruitInfo.tempe_chateau, "Temperature du chateau (K)" },
	{ "radon",           DESC_FLOAT                  &EdwSpBruitInfo.radon,         "Flux (m3/h)" },
	{ "tubes-pulses",    DESC_KEY "arretes/actifs", &EdwSpBruitInfo.tubes,         "Etat des tubes pulses" },
	{ "liquefacteur",    DESC_KEY "arrete/actif",   &EdwSpBruitInfo.lique,         "Etat du liquefacteur" },
	{ "ventilation",     DESC_KEY "arretee/active", &EdwSpBruitInfo.ventil,        "Etat de la ventilation" },
	{ "horloge",         DESC_FLOAT                  &EdwSpBruitInfo.horloge,       "ms" },
	{ "modulation",      DESC_INT                    &EdwSpBruitInfo.d2,    0 },
	{ "temps.nb",        DESC_INT                    &EdwSpBruitInfo.nbtemps,       0 },
	{ "freq.nb",         DESC_INT                    &EdwSpBruitInfo.nbfreq,        0 },
	{ "signal",          DESC_SHORT_ADRS(EdwSpBruitInfo.nbtemps) &EdwSpBruitInfo.temps, 0 },
	{ "spectre",         DESC_FLOAT_ADRS(EdwSpBruitInfo.nbfreq)  &EdwSpBruitInfo.freq,  0 },
	DESC_END
}
#endif
;
EDWDB_VAR TypeEdwDbElt EdwSpBruit
#ifdef EDWDB_C
= { EDWDB_NOM_BRUIT, EdwSpBruitDesc }
#endif
;
/* .......................................................................... */

int EdwDbPrint(char *dbaz, int bolo, int vm, EdwDbElt elt);
int EdwDbScan(char *dbaz, int date, int bolo, int vm, EdwDbElt elt, char log);

#endif
