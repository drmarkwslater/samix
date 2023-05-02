#ifndef AUTOM_H
#define AUTOM_H

#ifdef AUTOM_C
#define AUTOM_VAR
#else
#define AUTOM_VAR extern
#endif

#define AUTOM_VAR_MAX 256
#define MAXVARNOM 32
#define MAXVALSTR 32

#define AUTOM_NOLINK 255

typedef enum {
	AUTOM_TRAME_DONNEES = 0,
	AUTOM_TRAME_CONFIG,
	AUTOM_TRAME_ETATS,
	AUTOM_TRAME_NOM,
	AUTOM_TRAME_CLIENTS,
	AUTOM_NBTYPES_TRAME
} AUTOM_TRAME;
AUTOM_VAR char *AutomTrameNom[]
#ifdef AUTOM_C
 = { "DONNEES","CONFIG","ETATS","NOM","CLIENTS","\0", }
#endif
;
#define AutomTrameTexte(type) ((type < AUTOM_NBTYPES_TRAME)? AutomTrameNom[type]: "indetermine")

typedef enum {
	AUTOM_DO = 0,
	AUTOM_AI,
	AUTOM_DO1,
	AUTOM_DO2,
	AUTOM_ONOFF,
	AUTOM_AO,
	AUTOM_CTE,
	AUTOM_PID,
	AUTOM_DIGITALOUTPUT,
	AUTOM_SECURITY,
	AUTOM_VARIABLE,
	AUTOM_TIME,
	AUTOM_STATETIME,
	AUTOM_STATE,
	AUTOM_FUNCTION,
	AUTOM_STATECOUNT,
	AUTOM_NBCONTENUS,
	AUTOM_BAD = 99
} AUTOM_CONTENU;
AUTOM_VAR char *AutomTypeNom[]
#ifdef AUTOM_C
 = { "DO","AI","DO1","DO2","ONOFF","AO","CTE","PID","DIGITALOUTPUT","SECURITY",
	"VARIABLE","TIME","STATETIME","STATE","FUNCTION","STATECOUNT","\0", }
#endif
;
#define AutomTypeTexte(type) ((type < AUTOM_NBCODES)? AutomTypeNom[type]: "(tbd)")

typedef enum {
	AUTOM_FREE = 0,
	AUTOM_READONLY,
	AUTOM_STATELOCKED,
	AUTOM_PIDLOCKED,
	AUTOM_SECULOCKED,
	AUTOM_NBETATS
} AUTOM_ETAT;
#define AUTOM_ETAT_CLES "free/read/statelock/pidlock/secured"
AUTOM_VAR char *AutomEtatNom[]
#ifdef AUTOM_C
 = { "free","readonly","state-locked","pid-locked","secu-locked","\0" }
#endif
;
#define AutomEtatTexte(etat) ((etat < AUTOM_NBETATS)? AutomEtatNom[etat]: "inconnu")

typedef enum {
	AUTOM_BOOL = 0,
	AUTOM_CLE,
	AUTOM_INT,
	AUTOM_FLOAT,
	AUTOM_STRING,
	AUTOM_NBCODES
} AUTOM_CODE;
#define AUTOM_CODE_CLES "bool/key/int/float/string"
AUTOM_VAR char *AutomCodeNom[]
#ifdef AUTOM_C
= { "bool","key","int","float","string","\0" }
#endif
;
#define AUTOM_CLES_LNGR 32

typedef enum {
	AUTOM_CALIB = 0,
	AUTOM_REGEN,
	AUTOM_NBROLES
} AUTOM_ROLE;
#define AUTOM_ROLE_CLES "calib/regen"
AUTOM_VAR char *AutomSrceRole[]
#ifdef AUTOM_C
 = { "calibration","regeneration","\0", }
#endif
;

typedef union {
	int i;                  /* valeur a utiliser si le type est un entier 32bits   */
	float r;                /* valeur a utiliser si le type est un flottant        */
	char b;                 /* valeur a utiliser sinon (booleen, No de cle, ..)    */
	char s[MAXVALSTR];
} TypeAutomVal;

// #define VAR_VECTOR
typedef struct {
    char nom[MAXVARNOM];
	int  usr;                   /* index de la variable utilisateur (-1 si inutilisee) */
	short size;                 /* nombre de valeurs                                   */
	short index;                /* index si element d'une variable vecteur             */
	short ref;                  /* reference de la variable vecteur                    */
	char utilisee;              /* si utilisee comme variable utilisateur              */
	char etat,contenu;
	char code;                  /* voir AUTOM_CODE_CLES                                */
	char cles[AUTOM_CLES_LNGR]; /* cles, si type AUTOM_CLES                            */
	TypeAutomVal val;
#ifdef VAR_VECTOR
	TypeAutomVal *tab;
#endif
} TypeAutomVar;

AUTOM_VAR int AutomLink;

AUTOM_VAR TypeAutomVar AutomVar[AUTOM_VAR_MAX];
AUTOM_VAR char *AutomListe[AUTOM_VAR_MAX + 1];
AUTOM_VAR int AutomVarNb;

#define AUTOM_ARCH_MAX 128
#define AUTOM_USER_LIBELLE 32
#define AUTOM_USER_UNITE 12
typedef struct {
	int num;                                 /* position dans la table fournie par l'automate */
	short index;                             /* index dans la variable si tableau             */
	char libelle[AUTOM_USER_LIBELLE];        /* commentaire                                   */
	char unite[AUTOM_USER_UNITE];            /* unite representee                             */
	char type;                               /* voir AUTOM_CODE_CLES                          */
	union { int i;  float r;  char b; } val; /* valeur a utiliser selon type                  */
	char cles[AUTOM_CLES_LNGR];              /* cles, si type AUTOM_CLES                      */
	char archive,affiche;
	union { int i; float r; char b; } min;   /* minimum affiche                               */
	union { int i; float r; char b; } max;   /* maximum affiche                               */
	Instrum instrum; Panel panel;            /* instruments affiches                          */
} TypeAutomUserVar;
AUTOM_VAR TypeAutomUserVar AutomUserVar[AUTOM_VAR_MAX];
AUTOM_VAR int AutomUserNb;

#define AUTOM_SRCE_MAX 8
typedef struct {
	int  num;                                /* index dans la table fournie par l'automate    */
	short index;                             /* index dans la variable si tableau             */
	char libelle[AUTOM_USER_LIBELLE];        /* commentaire                                   */
	char role;                               /* voir AUTOM_ROLE_CLES                          */
	char active;                             /* oui si en service                             */
} TypeAutomSrceVar;
AUTOM_VAR TypeAutomSrceVar AutomSrceVar[AUTOM_SRCE_MAX];
AUTOM_VAR int AutomSrceNb;
AUTOM_VAR char AutomSrceAccede; /* autorise a deplacer les sources */

typedef struct {
	char *nom;
	char type; /* voir AUTOM_CODE_CLES */
	char **cle;
	float **reel;
	char *libelle;
} TypeAutomInterne;
#define AUTOM_VAR_FLOAT_DEFAUT(nom,libelle) { chaine(nom), AUTOM_FLOAT, 0, &nom, libelle }

AUTOM_VAR float FlottantBidon;
AUTOM_VAR char  CharBidon;
AUTOM_VAR float *T_Bolo;
AUTOM_VAR char *PulseTubes;
// AUTOM_VAR float *T_Speer,*T_PT1,*P_regul;
#ifdef EDW_ORIGINE
	AUTOM_VAR char  *AutomSrceRegenAdrs[AUTOM_REGEN_MAX],*AutomSrceCalibAdrs[AUTOM_CALIB_MAX];
#endif /* EDW_ORIGINE */

AUTOM_VAR TypeAutomInterne AutomInterne[]
#ifdef AUTOM_C
= {
//	AUTOM_VAR_FLOAT_DEFAUT(T_Speer, "temperature speer"),
//	AUTOM_VAR_FLOAT_DEFAUT(T_PT1,   "temperature pulse-tube #1"),
//	AUTOM_VAR_FLOAT_DEFAUT(P_regul, "pression de regulation"),
//	AUTOM_VAR_FLOAT_DEFAUT(PulseTubes,   "activation des pulse-tubes"),
	{ "T_Bolo",         AUTOM_FLOAT, 0, &T_Bolo,            "temperature des detecteurs" },
	{ "PulseTubes",     AUTOM_BOOL, &PulseTubes, 0,         "activation des pulse-tubes" },
#ifdef EDW_ORIGINE
	{ AutomRegenNom[0], AUTOM_BOOL, &(AutomSrceRegenAdrs[0]), 0, "position source regeneration #1" },
	{ AutomRegenNom[1], AUTOM_BOOL, &(AutomSrceRegenAdrs[1]), 0, "position source regeneration #2" },
	{ AutomCalibNom[0], AUTOM_BOOL, &(AutomSrceCalibAdrs[0]), 0, "position source calibration #1" },
	{ AutomCalibNom[1], AUTOM_BOOL, &(AutomSrceCalibAdrs[1]), 0, "position source calibration #2" },
#endif /* EDW_ORIGINE */
	{ "\0",      0,      0,      0 }
}
#endif
;

int AutomRang(char *nom);
char AutomCode(int num);
int *AutomAdrsInt(int num);
char *AutomAdrsKey(int num);
float *AutomAdrsFloat(int num);
char *AutomNom(int num);
char AutomDataQuery(char *raison);
char AutomDataGet(char *raison);
char AutomEcritVar(int num, char *raison);
int AutomUserIndex(char *nom);
char AutomEcritUser(int i, char *raison);
int AutomCreeTrame(Menu menu, MenuItem item);
int AutomCommande(Menu menu, MenuItem item);
int AutomChoisi(Menu menu, MenuItem item);
char AutomSourceDeplace(char role, char descendre, char avec_source, char *prefixe);
void AutomVerifieConditions(char *prefixe, int k);
int AutomSauve();

#endif /* AUTOM_H */
