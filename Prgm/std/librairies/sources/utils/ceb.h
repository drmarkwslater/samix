#ifndef CEB_H
#define CEB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MODULO
#ifdef MODULO
#include <math.h>    /* pour fmodf */
#ifdef PROJECT_BUILDER
#define fmodf(x,y) (float)fmod((double)x, (double)y);
#endif
#endif
/* #define _USE_CTYPE_INLINE_ */
#include <ctype.h>

/* ................................................................ */
/*                                ceb.h                             */
#define CEB_MAXPILE 32
#define CEB_MAXNOM 32
#define CEB_MAXVARS 128
#define CEB_MAXNOP 2

#define CEB_INDEF 255

typedef enum {
	CEB_NOP = 0,
	CEB_LOAD,
	CEB_STORE,
	CEB_MONTE,
	CEB_EXIT,
	CEB_ADD,
	CEB_SUB,
	CEB_MUL,
	CEB_DIV,
	CEB_MOD,
	CEB_EQ,
	CEB_NE,
	CEB_GE,
	CEB_GT,
	CEB_LE,
	CEB_LT,
	CEB_AND,
	CEB_OR
} CEB_INSTR;

#define CEB_NONE 0
#define CEB_CONSTANTE 65535
/* #define CEB_MASKMODE 0x03
   #define CEB_MASKETAT 0x0C */

typedef enum {
	CEB_IMME = 1,
	CEB_ADRS,
	CEB_PILE
} CEB_MODES;

typedef enum {
	CEB_DEF = 1,
	CEB_REF,
	CEB_FONCTION
} CEB_TYPEVAR;

typedef struct {
	char nom[CEB_MAXNOM];
	char type;
	char etat;
	int  dim;
	union {
		float imme;
		float *adrs;
	} val;
} CebVar,*CebVarRef;

#define CEB_ENDVARS { "\0", CEB_NONE, CEB_NONE, 0.0 }

#define CebVarType(v) (v.type)
#define CebVarRefType(w) (w->type)
#define CebVarEtat(v) (v.etat)
#define CebVarRefEtat(w) (w->etat)

#define CebVarAdrs(v) ((v.type == CEB_IMME)? &(v.val.imme): v.val.adrs)
#define CebVarRefAdrs(w) ((w->type == CEB_IMME)? &(w->val.imme): w->val.adrs)
#define CebVarValue(v) ((v.type == CEB_IMME)? v.val.imme: *(v.val.adrs))
#define CebVarRefValue(w) ((w->type == CEB_IMME)? w->val.imme: *(w->val.adrs))

typedef struct {
	unsigned char instr;
	unsigned short var;
	unsigned char mode;
	union {
		float imme;
		float *adrs;
	} val;
} CebInstr;

void CebInit(CebInstr *prgm);
char *CebErrorText(void);
void CebHelp(void);
CebVar *CebVarCreateLocale(char *nom, float val);
CebVar *CebVarCreateWithAdrs(char *nom, int dim, float *adrs);
CebVar *CebVarCreateCopy(CebVar *v);
char CebVarInclude(CebVar *v);
void CebVarList(void);
void CebVarClear(void);
char CebAnalyse(FILE *f, CebInstr *prgm, CebInstr **fin);
char CebCompile(FILE *f, CebInstr *prgm, CebInstr **fin);
char CebDecode(char *ligne, CebInstr *prgm, CebInstr **fin);
void CebPrgmList(CebInstr *prgm);
float CebExec(CebInstr *prgm, char debug);
void CebPrgmClear(CebInstr *prgm);

#endif
