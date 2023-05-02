#ifndef CLAPS_H
#define CLAPS_H

#include <stdio.h>

#ifdef CLAPS_C
#define CLAPS_VAR
#else
#define CLAPS_VAR extern
#endif

#define DESC_SIGNE "[claps]"

#define DESC_CLE_LNGR 256

typedef enum {
	ARG_STYLE_MIG = 0,
	ARG_STYLE_LINUX,
	ARG_STYLE_JSON,
	ARG_STYLE_NB
} ARG_STYLES;

typedef struct {
	char *texte;   /* vaut 0 si parametre positionnel         */
	char  lu;      /* vrai si texte rencontre en lecture      */
	char  type;    /* voir ARG_TYPES                          */
	char  handle;  /* vrai si adresse dynamique               */
	short lngr;    /* longueur si type texte, ou longueur item pour panels */
	int   dim;     /* dimension tableau si limite statique    */
	int  *taille;  /* dimension tableau si limite dynamique   */
	char *modele;  /* valeurs permises si type ARG_TYPE_KEY ou ARG_TYPE_LISTE, 
	                  ou adresse de la fonction si ARG_TYPE_USER,
	                  ou format pour panels, ou nouveau texte si ARG_ALIAS  */
	void *adrs;    /* adresse de la variable, ou ArgStruct* si ARG_TYPE_STRUCT, ou *ArgDesc[] si ARG_TYPE_VARIANTE */
	char *explic;
} ArgDesc;

typedef void (*ArgFctn)(void *bloc);
typedef struct {
	void *table;   /* adresse de la variable in fine                          */
	void *tempo;   /* adresse de la structure elementaire tampon de meme type */
	int lngr;      /* longueur de la structure de meme type                   */
	ArgDesc *desc; /* description de la structure elementaire                 */
} ArgStruct;

typedef enum {
	ARG_TYPE_COMMENT = 0,
	ARG_TYPE_TEXTE,
	ARG_TYPE_INT,
	ARG_TYPE_HEXA,
	ARG_TYPE_SHORT,
	ARG_TYPE_SHEX,
	ARG_TYPE_FLOAT,
	ARG_TYPE_CHAR,
	ARG_TYPE_OCTET,
	ARG_TYPE_BYTE,
	ARG_TYPE_LETTRE,
	ARG_TYPE_BOOL,
	ARG_TYPE_KEY,
	ARG_TYPE_LISTE,
	ARG_TYPE_USER,
	ARG_TYPE_STRUCT,
	ARG_TYPE_DESC,
	ARG_TYPE_RIEN,
	ARG_TYPE_VARIANTE,
	ARG_TYPE_EXEC,
	ARG_TYPE_IF,
	ARG_TYPE_MAX
} ARG_TYPES;

CLAPS_VAR char *ArgNomType[ARG_TYPE_MAX+1]
#ifdef CLAPS_C
 = {
	"commentaire", "texte", "int", "hexa", "short", "shorthexa", 
	"float", "char", "octet", "octethexa", "lettre", "booleen", "mot-cle", "liste", "user",
	"struct", "desc", "obsolete", "variante", "fonction", "condition", "\0"
}
#endif
;

#define DESC_COMMENT      0,0,ARG_TYPE_COMMENT,  0,0,0,0,0,0,
#define DESC_TEXT           0,ARG_TYPE_TEXTE,    0,0,1,0,0, (void *)
#define DESC_STR(l)         0,ARG_TYPE_TEXTE,    0,l,1,0,0, (void *)
#define DESC_INT            0,ARG_TYPE_INT,      0,0,1,0,0, (void *)
#define DESC_HEXA           0,ARG_TYPE_HEXA,     0,0,1,0,0, (void *)
#define DESC_SHORT          0,ARG_TYPE_SHORT,    0,0,1,0,0, (void *)
#define DESC_SHEX           0,ARG_TYPE_SHEX,     0,0,1,0,0, (void *)
#define DESC_FLOAT          0,ARG_TYPE_FLOAT,    0,0,1,0,0, (void *)
#define DESC_CHAR           0,ARG_TYPE_OCTET,    0,0,1,0,0, (void *)
#define DESC_OCTET          0,ARG_TYPE_OCTET,    0,0,1,0,0, (void *)
#define DESC_BYTE           0,ARG_TYPE_BYTE,     0,0,1,0,0, (void *)
#define DESC_LETTRE         0,ARG_TYPE_LETTRE,   0,0,1,0,0, (void *)
#define DESC_BOOL           0,ARG_TYPE_BOOL,     0,0,1,0,0, (void *)
#define DESC_KEY            0,ARG_TYPE_KEY,      0,0,1,0,(char *)
#define DESC_LISTE          0,ARG_TYPE_LISTE,    0,0,1,0,(char *)
#define DESC_USER           0,ARG_TYPE_USER,     0,0,1,0,(char *)
// #define DESC_STRUCT         0,ARG_TYPE_STRUCT,   0,0,1,0,0, (void *)
#define DESC_STRUCT         0,ARG_TYPE_STRUCT,   0,0,-1,0,0, (void *)
#define DESC_DESC           0,ARG_TYPE_DESC,     0,0,1,0,0, (void *)
#define DESC_NONE           0,ARG_TYPE_RIEN,     0,0,0,0,0,0,0
#define DESC_ALIAS(v)       0,ARG_TYPE_RIEN,     0,0,0,0,v,0,0
#define DESC_VARIANTE(t)    0,ARG_TYPE_VARIANTE, 0,0,1,0,&t, (void *)
#define DESC_INCLUDE(v) { 0,0,ARG_TYPE_VARIANTE, 0,0,1,0, 0, (void *)(&v), 0 }

#define DESC_DEVIENT(v)     0,ARG_TYPE_RIEN,     0,0,0,0,v,0,0

/* adresse statique et limite de tableau statique <n> */
#define DESC_STR_DIM(l,n)   0,ARG_TYPE_TEXTE,   0,l,n,0,0, (void *)
#define DESC_INT_DIM(n)     0,ARG_TYPE_INT,     0,0,n,0,0, (void *)
#define DESC_HEXA_DIM(n)    0,ARG_TYPE_HEXA,    0,0,n,0,0, (void *)
#define DESC_SHORT_DIM(n)   0,ARG_TYPE_SHORT,   0,0,n,0,0, (void *)
#define DESC_SHEX_DIM(n)    0,ARG_TYPE_SHEX,    0,0,n,0,0, (void *)
#define DESC_FLOAT_DIM(n)   0,ARG_TYPE_FLOAT,   0,0,n,0,0, (void *)
#define DESC_CHAR_DIM(n)    0,ARG_TYPE_CHAR,    0,0,n,0,0, (void *)
#define DESC_OCTET_DIM(n)   0,ARG_TYPE_OCTET,   0,0,n,0,0, (void *)
#define DESC_BYTE_DIM(n)    0,ARG_TYPE_BYTE,    0,0,n,0,0, (void *)
#define DESC_LETTRE_DIM(n)  0,ARG_TYPE_LETTRE,  0,0,n,0,0, (void *)
#define DESC_BOOL_DIM(n)    0,ARG_TYPE_BOOL,    0,0,n,0,0, (void *)
#define DESC_KEY_DIM(n)     0,ARG_TYPE_KEY,     0,0,n,0,(char *)
#define DESC_LISTE_DIM(n)   0,ARG_TYPE_LISTE,   0,0,n,0,(char *)
#define DESC_USER_DIM(n)    0,ARG_TYPE_USER,    0,0,n,0,(char *)
#define DESC_STRUCT_DIM(n)  0,ARG_TYPE_STRUCT,  0,0,n,0,0, (void *)
// #define DESC_DESC_DIM(n)    0,ARG_TYPE_DESC,    0,0,n,0,0, (void *)

/* adresse statique et limite de tableau dynamique <&a> avec maxi a <n> */
#define DESC_STR_SIZE(l,a,n)  0,ARG_TYPE_TEXTE,  0,l,n,&a,0, (void *)
#define DESC_INT_SIZE(a,n)    0,ARG_TYPE_INT,    0,0,n,&a,0, (void *)
#define DESC_HEXA_SIZE(a,n)   0,ARG_TYPE_HEXA,   0,0,n,&a,0, (void *)
#define DESC_SHORT_SIZE(a,n)  0,ARG_TYPE_SHORT,  0,0,n,&a,0, (void *)
#define DESC_SHEX_SIZE(a,n)   0,ARG_TYPE_SHEX,   0,0,n,&a,0, (void *)
#define DESC_FLOAT_SIZE(a,n)  0,ARG_TYPE_FLOAT,  0,0,n,&a,0, (void *)
#define DESC_CHAR_SIZE(a,n)   0,ARG_TYPE_CHAR,   0,0,n,&a,0, (void *)
#define DESC_OCTET_SIZE(a,n)  0,ARG_TYPE_OCTET,  0,0,n,&a,0, (void *)
#define DESC_BYTE_SIZE(a,n)   0,ARG_TYPE_BYTE,   0,0,n,&a,0, (void *)
#define DESC_LETTRE_SIZE(a,n) 0,ARG_TYPE_LETTRE, 0,0,n,&a,0, (void *)
#define DESC_BOOL_SIZE(a,n)   0,ARG_TYPE_BOOL,   0,0,n,&a,0, (void *)
#define DESC_KEY_SIZE(a,n)    0,ARG_TYPE_KEY,    0,0,n,&a,(char *)
#define DESC_LISTE_SIZE(a,n)  0,ARG_TYPE_LISTE,  0,0,n,&a,(char *)
#define DESC_USER_SIZE(a,n)   0,ARG_TYPE_USER,   0,0,n,&a,(char *)
#define DESC_STRUCT_SIZE(a,n) 0,ARG_TYPE_STRUCT, 0,0,n,&a,0, (void *)
// #define DESC_DESC_SIZE(a,n)   0,ARG_TYPE_DESC,   0,0,n,&a,0, (void *)

/* adresse dynamique et limite de tableau dynamique <&a> */
#define DESC_STR_PTR        0,ARG_TYPE_TEXTE,  1,0,1,0,0, (void *)

#define DESC_STR_ADRS(l,a)  0,ARG_TYPE_TEXTE,  1,l,0,&a,0, (void *)
#define DESC_INT_ADRS(a)    0,ARG_TYPE_INT,    1,0,0,&a,0, (void *)
#define DESC_HEXA_ADRS(a)   0,ARG_TYPE_HEXA,   1,0,0,&a,0, (void *)
#define DESC_SHORT_ADRS(a)  0,ARG_TYPE_SHORT,  1,0,0,&a,0, (void *)
#define DESC_SHEX_ADRS(a)   0,ARG_TYPE_SHEX,   1,0,0,&a,0, (void *)
#define DESC_FLOAT_ADRS(a)  0,ARG_TYPE_FLOAT,  1,0,0,&a,0, (void *)
#define DESC_CHAR_ADRS(a)   0,ARG_TYPE_CHAR,   1,0,0,&a,0, (void *)
#define DESC_OCTET_ADRS(a)  0,ARG_TYPE_OCTET,  1,0,0,&a,0, (void *)
#define DESC_BYTE_ADRS(a)   0,ARG_TYPE_BYTE,   1,0,0,&a,0, (void *)
#define DESC_LETTRE_ADRS(a) 0,ARG_TYPE_LETTRE, 1,0,0,&a,0, (void *)
#define DESC_BOOL_ADRS(a)   0,ARG_TYPE_BOOL,   1,0,0,&a,0, (void *)
#define DESC_KEY_ADRS(a)    0,ARG_TYPE_KEY,    1,0,0,&a,(char *)
#define DESC_LISTE_ADRS(a)  0,ARG_TYPE_LISTE,  1,0,0,&a,(char *)
#define DESC_USER_ADRS(a)   0,ARG_TYPE_USER,   1,0,0,&a,(char *)
//#define DESC_STRUCT_ADRS(a)  ARG_TYPE_STRUCT,1,0,0,&a,0, (void *)
// #define DESC_DESC_ADRS(a)   0,ARG_TYPE_DESC,   1,0,0,&a,0, (void *)

#define DESC_EXEC         0,0,ARG_TYPE_EXEC,   0,0,0,0,0, (void *)
/* utilisation: "{ DESC_EXEC fonction, pointeur_sur_donnees }"
   fonction: void fctn(void *ptr) */

#define DESC_END        { 0,0,-1,              0,0,0,0,0, 0,0 }
#define DESC_END_DEFAUT   0,0,-1,              0,0,0,0,0, (void *)
/* la desc se termine par "{ DESC_END_DEFAUT fonction, pointeur_sur_donnees }" au lieu de "DESC_END"
   fonction: void fctn(void *ptr) */

/* si a->type == ARG_TYPE_IF
     si a->dim == 0, variable (de type char) en a->adrs
                     condition (de type texte) en a->modele ("=", ">", ...)
                     valeur de test = a->lngr
					 . si condition vraie, noter "sauter APRES else sauf si endif 1er"
                     . sinon, noter "sauter JUSQU'A 1er de else ou endif"
               == 1, c'est un else
					 . sauter selon note precedente
			   == 2, c'est un endif
                     . annuler la note
					 
   donc 
	 ArgModeIf = DESC_MODE_SUIVANT par defaut
	 si ArgModeIf < DESC_MODE_SAUTE_BLOC, execution
     si ArgModeIf = DESC_MODE_SAUTE_BLOC, pas execution
     si a->type == ARG_TYPE_IF
       si a->dim == 0, si condition vraie, ArgModeIf = DESC_MODE_SUIVANT
					   sinon               ArgModeIf = DESC_MODE_SAUTE_BLOC
	   si a->dim == 1, si ArgModeIf == DESC_MODE_SUIVANT, ArgModeIf = DESC_MODE_SAUTE_BLOC
                       si ArgModeIf == DESC_MODE_SAUTE_BLOC, ArgModeIf = DESC_MODE_SUIVANT
       si a->dim == 2, ArgModeIf = DESC_MODE_SUIVANT
*/
#define DESC_IF(v1,cond,val) { 0,0,ARG_TYPE_IF, 0,val,0, 0,cond,(void *)v1, 0 }
#define DESC_ELSE            { 0,0,ARG_TYPE_IF, 0, 0, 1, 0, 0, 0, 0 }
#define DESC_ENDIF           { 0,0,ARG_TYPE_IF, 0, 0, 2, 0, 0, 0, 0 }

#define ARG_MAXSUIVI 32
CLAPS_VAR void *ArgAdrsSuivie
#ifdef CLAPS_C
 = 0
#endif
;
CLAPS_VAR char ArgAdrsNom[ARG_MAXSUIVI];

typedef char *(*TypeArgUserFctn)(char *, void *, int *);

char *ArgCommandLine(int argc, char *argv[]);
void ArgSuitAdresse(char *nom, void *adrs);
int  ArgLignesLues(void);
void ArgReportError(char report);
void ArgKeyFromList(char *modeles, char **liste);
int  ArgKeyGetNb(char *modeles);
int  ArgKeyGetLngr(char *modeles);
char ArgKeyGetText(char *modeles, char index, char *nom, int max);
int  ArgKeyGetIndex(char *modeles, char *texte);
void ArgDump(ArgDesc *desc);
void ArgHelp(ArgDesc *desc);
void ArgDebug(int niveau);
void ArgCompatible(char compat);
void ArgDefiniDelim();
void ArgStdoutBrut(char avec_cr);
void ArgSymbolAdd(char *nom, char *valeur);
// int ArgDecodeVal(char *vallue, char type, char *modele, char *adrs, char fic);
int  ArgDecodeVal(char *vallue, ArgDesc *a, int index, char fic);
void ArgEncodeVal(ArgDesc *a, char *texte, int max);
char *ArgItemGetName(ArgDesc *desc, int k);
int  ArgItemGetIndex(ArgDesc *desc, char *texte);
char ArgItemLu(ArgDesc *desc, int index);
char ArgVarLue(ArgDesc *desc, char *texte);
int  ArgGetVarLue(ArgDesc *desc);
void ArgVarUtilise(ArgDesc *desc, char *texte, char utile);
char ArgDecodeItem(char *item, ArgDesc *desc);
char ArgSplit(char *argv, char delim, ArgDesc *desc);
char ArgFromWeb(ArgDesc *desc);
char ArgList(int argc, char *argv[], ArgDesc *desc);
char ArgListChange(int argc, char *argv[], ArgDesc *desc);
ArgDesc *ArgCreate(int n);
void ArgAdd(ArgDesc *c, char *texte, char lu, int type, char hndl, short lngr, int dim, 
			int *taille, char *modele, void *adrs, char *explic);
void ArgDefaut(ArgDesc *c, ArgFctn f, void *bloc_a_lire);
void ArgLngr(ArgDesc *c, char *texte, short lngr, char *fmt);
int  ArgSkipOnFile(FILE *f, ArgDesc *desc, char *delim);
int  ArgFromFile(FILE *f, ArgDesc *desc, char *delim);
int  ArgFromPath(int f, ArgDesc *desc, char *delim);
int  ArgSkipOnPath(int p, ArgDesc *desc, char *delim);
char ArgBilan(char *nom, int r);
void ArgNomEnCours(char *nom);
char ArgScan(char *nom, ArgDesc *desc);
void ArgIndent(int n);
//- int  ArgAjoute(FILE *f, char aide, ArgDesc *desc, char tous);
//- int  ArgEcritFichier(char *nom, ArgDesc *desc, char tous);
int  ArgBegin(FILE *f, char aide, ArgDesc *desc);
int  ArgAppend(FILE *f, char aide, ArgDesc *desc);
int  ArgAppendModif(FILE *f, char aide, ArgDesc *desc);
int  ArgToPath(int p, char aide, ArgDesc *desc);
int  ArgModifToPath(int p, char aide, ArgDesc *desc);
ARG_STYLES ArgStyle(ARG_STYLES style);
int  ArgPrint(char *nom, ArgDesc *desc);
int  ArgPrintModif(char *nom, ArgDesc *desc);
int  ArgPrintStyle(char *nom, ArgDesc *desc, ARG_STYLES style_demande);
int  ArgInput(int p, ArgDesc *desc, int maxi, char *delim);
char ArgRead(char *nom, ArgDesc *desc);
int  ArgOutput(int p, ArgDesc *desc);
int  ArgWrite(char *nom, ArgDesc *desc);

#endif

