#include "environnement.h"
#include <ceb.h>

static char CebImprime;
static float CebPile[CEB_MAXPILE];        /* pile des resultats */
static CebInstr CebOper[CEB_MAXPILE];     /* pile des operandes/operateurs en cours */
static CebVar CebVars[CEB_MAXVARS];
static int CebVarNb;
static char CebErreur[256];
static char *CebInstrNom[] = {
	"nop", "load", "store", "monte", "exit",
	"add", "sub", "mul", "div", "mod", "eq", "ne", "ge", "gt", "le", "lt",
	"and", "or",
	"\0"
};

/* ================================================================ */
void CebInit(CebInstr *prgm) {
	CebVarNb = 0;
	CebImprime = 0;
	if(prgm) prgm->instr = CEB_EXIT;
}
/* ================================================================ */
void CebHelp(void) {
	printf("<instruction> := [ <variable> = ] <operation> | '#' < commentaire>\n");
	printf("<operation>   := <valeur> [ <operateur> <valeur> ]\n");
	printf("<valeur>      := <variable> | <constante> | '(' <operation> ')'\n");
	printf("<operateur>   := '+'  | '-'  | '*'  | '/' | '%%'  | '&' | '|' |\n");
	printf("                 '==' | '!=' | '>=' | '>' | '<=' | '<'\n");
}
/* ================================================================ */
char *CebErrorText(void) { return(CebErreur); }
/* ================================================================ */
static char CebVarExist(CebVar *v) { return(v->nom[0] != '\0'); }
/* ================================================================ */
CebVar *CebVarCreateLocale(char *nom, float val) {
/* Ajoute une variable dont la valeur est 'locale' (type IMME),
   et retourne une reference a cette variable (reference utilisable
   pour recuperer l'etat, l'adresse ou la valeur courante) */
	CebVar *w;

	if(CebVarNb >= CEB_MAXVARS) {
		sprintf(CebErreur,"Deja %d variables definies",CebVarNb);
		return(0);
	}
	w = &(CebVars[CebVarNb]);
	strncpy(w->nom,nom,CEB_MAXNOM);
	w->dim = 1;
	CebVarRefType(w) = CEB_IMME;
	CebVarRefEtat(w) = CEB_NONE;
	/* CebVarRefValue(w) */ w->val.imme = val;
	CebVarNb++;
	return(w);
}
/* ================================================================ */
CebVar *CebVarCreateWithAdrs(char *nom, int dim, float *adrs) {
/* Ajoute une variable dont l'adresse est geree par l'utilisateur (type ADRS),
   et retourne une reference a cette variable (reference utilisable
   pour recuperer l'etat [+ l'adresse ou la valeur courante]) */
	CebVar *w;

	if(CebVarNb >= CEB_MAXVARS) {
		sprintf(CebErreur,"Deja %d variables definies",CebVarNb);
		return(0);
	}
	w = &(CebVars[CebVarNb]);
	strncpy(w->nom,nom,CEB_MAXNOM);
	w->dim = dim;
	CebVarRefType(w) = CEB_ADRS;
	CebVarRefEtat(w) = CEB_NONE;
	/* CebVarRefAdrs(w) */ w->val.adrs = adrs;
	CebVarNb++;
	return(w);
}
/* ================================================================ */
CebVar *CebVarCreateCopy(CebVar *v) {
/* Ajoute une variable copie d'une variable declaree par l'utilisateur,
   et retourne une reference a cette variable (reference utilisable
   pour recuperer l'etat [+ l'adresse ou la valeur courante]) */
	CebVar *w;

	if(CebVarNb >= CEB_MAXVARS) {
		sprintf(CebErreur,"Deja %d variables definies",CebVarNb);
		return(0);
	}
	w = &(CebVars[CebVarNb]);
/*	memcpy(w,v,sizeof(CebVar)); pas si simple! */
	strncpy(w->nom,v->nom,CEB_MAXNOM);
	w->dim = v->dim;
	CebVarRefType(w) = CEB_ADRS;
	CebVarRefEtat(w) = CebVarRefEtat(v);
	/* CebVarRefAdrs(w) */ w->val.adrs = CebVarRefAdrs(v);
	CebVarNb++;
	return(w);
}
/* ================================================================ */
char CebVarInclude(CebVar *v) {
/* Ajoute comme variables tout un tableau declare par l'utilisateur */
	CebVar *w;

	if(!v) return(0);
	w = v;
	while(CebVarExist(w)) if(!CebVarCreateCopy(w++)) return(0);
	return(1);
}
/* ================================================================ */
void CebVarList(void) {
	int i;

	i = CebVarNb;
	while(i--) {
		printf("%2d: %-*s",CebVarNb - i,CEB_MAXNOM,CebVars[i].nom);
		if(CebVars[i].dim == 1) printf("      ");
		else printf("[%4d]",CebVars[i].dim);
		printf(" = %-10g (%s)\n",CebVarValue(CebVars[i]),
			((CebVarEtat(CebVars[i]) == CEB_DEF)? "initialisee" :
			(CebVarEtat(CebVars[i]) == CEB_REF)? "referencee" : "inutilisee"));
	}
}
/* ================================================================ */
void CebVarClear(void) { CebVarNb = 0; }
/* ================================================================ */
static char *CebSvt(char **curseur, char *liste, char *delim) {
/* Recherche du prochain element syntaxique */
	char *debut,*curs,*del; char sym_trouve;

	sym_trouve = 0;
	curs = *curseur;
	while(((*curs == ' ') || (*curs == 0x09)) && (*curs != '\0') && (*curs != '\n')) curs++;
	debut = curs; *delim  = ';';
	if(*curs == '\n') *curs = '\0';
	if(*curs == '\0') return(debut);
	do {
		if((*curs == ' ') || (*curs == 0x09)) {
			sym_trouve = 1;
			*curs = '\0';
			continue;
		}
		del = liste;
		while(*del != '\0') {
			if(*curs == *del) break;
			del++;
		}
		if(*del != '\0') { *delim = *del; break; }
		if(sym_trouve) { *delim = *curs; *curseur = 0; *curs = '\0'; return(debut); }
	} while(*(++curs));

	if(*curs == '\0') *curseur = curs; else *curseur = curs + 1;
	*curs = '\0';
	return(debut);
}
/* ================================================================ */
static char CebEnter(CebInstr *pc, CebInstr *oper) {
/* equivalent a <enter> de chez HP */
	if(oper->instr == CEB_INDEF) pc->instr = CEB_LOAD;
	else pc->instr = oper->instr;
	pc->var = oper->var;
	pc->mode = oper->mode;
	if(oper->mode == CEB_IMME) pc->val.imme = oper->val.imme;
	else if(oper->mode == CEB_ADRS) pc->val.adrs = oper->val.adrs;
	else if(oper->mode != CEB_PILE) {
		sprintf(CebErreur,"Expression vide");
		return(0);
	}
	return(1);
}
/* ================================================================ */
char CebAnalyse(FILE *f, CebInstr *prgm, CebInstr **fin) {
	char rc;

	CebImprime = 1;
	rc = CebCompile(f,prgm,fin);
	CebImprime = 0;
	return(rc);
}
#define EOL_IS_OA
/* ================================================================ */
char CebCompile(FILE *f, CebInstr *prgm, CebInstr **fin) {
/* Compile tout un fichier par appel a CebDecode sur chaque ligne */
	char ligne[256];
	CebInstr *pc;
	CebInstr *suite;
#ifdef EOL_IS_OA
	char *c; int n;
#endif

	pc = prgm;
	pc->instr = CEB_EXIT;
	pc->mode = CEB_NONE;
	suite = pc;   /* au cas ou la boucle serait vide (presence de breaks) */
#ifdef EOL_IS_OA
	c = ligne; n = 255;
#endif
	do {
#ifdef EOL_IS_OA
		if((*c = (char)getc(f)) == EOF)  *c = 0x0a;
		else if(*c == 0x0d) *c = 0x0a;
		if(*c == 0x0a) { c++; *c = '\0'; }
		else if(--n > 0) { c++; continue; }
#else
		if(!fgets(ligne,256,f)) break;
#endif
/*		printf("lu:\nc='");
		c = ligne; while(*c) { printf("  %c",isprint(*c)? *c: '.'); c++; }; printf(" '\nx='");
		c = ligne; while(*c) { printf(" %02x",*c); c++; }; printf(" '\n"); */
		if(!strncmp(ligne,"exit",4)) break;
		if(!CebDecode(ligne,pc,&suite)) {
			if(CebImprime) printf("    => %s\n",CebErreur); else return(0);
		}
		if(feof(f)) break;
		pc = suite;
#ifdef EOL_IS_OA
		c = ligne; n = 255;
#endif
	} while(1);
	*fin = suite;
	return(1);
}
/* ================================================================ */
char CebDecode(char *ligne, CebInstr *prgm, CebInstr **fin) {
/* Compile une ligne, et ajoute le resultat au code deja existant */
	CebInstr *oper;
	CebInstr *pc; char *curseur,*decl_tab;
	char *sym; char delim,c;
	int i,l,dim_tab,a_sauver,niveau;

	if(CebImprime) {
		printf("%2d/ %s",CebImprime++,ligne);
		if(ligne[strlen(ligne)-1] != '\n') printf("<\\n>\n");
	}
	if(*ligne == '#') return(1);
	curseur = ligne; pc = prgm; oper = CebOper;
	oper->instr = CEB_INDEF;
	oper->mode = CEB_INDEF;
	a_sauver = -1; niveau = 1;
	do {
		if(*curseur == '\0') break;
		sym = CebSvt(&curseur,"=+-*/%!><()&|\n;",&delim);
		if(curseur == 0) {
			sprintf(CebErreur,"Caractere '%c' inconnu (apres '%s')",delim,sym);
			return(0);
		}
		if(sym[0]) {
			if(isdigit(sym[0])) {
				sscanf(sym,"%g",&(oper->val.imme));
				oper->mode = CEB_IMME;
				oper->var = CEB_CONSTANTE;
			} else {
				decl_tab = sym;
				sym = CebSvt(&decl_tab,"[",&c);
				if(c == '[') {
					l = (int)strlen(decl_tab);
					if(decl_tab[l-1] == ']') decl_tab[l-1] = '\0';
					if(isdigit(decl_tab[l-1])) sscanf(decl_tab,"%d",&dim_tab);
					else {
						for(i=0; i<CebVarNb; i++) if(!strcmp(decl_tab,CebVars[i].nom)) break;
						if(i == CebVarNb) {
							sprintf(CebErreur,"Variable '%s' non definie",decl_tab);
							return(0);
						} else if(CebVars[i].dim > 1) {
							sprintf(CebErreur,"Variable '%s' non scalaire: [%d]",decl_tab,CebVars[i].dim);
							return(0);
						}
						dim_tab =  (int)CebVarValue(CebVars[i]);
					}
				} else dim_tab = 1;
				/* Recherche si <sym> existe deja (pourrait etre dichotomique sur liste...) */
				for(i=0; i<CebVarNb; i++) if(!strcmp(sym,CebVars[i].nom)) break;
				if(i == CebVarNb) {
					if(CebVarNb >= CEB_MAXVARS) {
						sprintf(CebErreur,"Deja %d variables definies",CebVarNb);
						return(0);
					}
					if((delim != '=') || (*curseur == '=')) {
						sprintf(CebErreur,"Variable '%s' non initialisee",sym);
						return(0);
					}
					strcpy(CebVars[CebVarNb].nom,sym);
					CebVars[CebVarNb].dim  = dim_tab;
					if(dim_tab == 1) {
						CebVarType(CebVars[CebVarNb]) = CEB_IMME; /* adresse pas definie en externe! */
						CebVarEtat(CebVars[CebVarNb]) = CEB_DEF;
					} else {
						CebVars[CebVarNb].val.adrs = (float *)malloc(dim_tab * sizeof(float));
						CebVarType(CebVars[CebVarNb]) = CEB_ADRS;
						CebVarEtat(CebVars[CebVarNb]) = CEB_DEF;
					}
					CebVarNb++;
				}
				oper->var = (unsigned short)i;
				oper->mode = CEB_ADRS;
				oper->val.adrs = CebVarAdrs(CebVars[i]);
				if(CebVarEtat(CebVars[i]) == CEB_NONE) {
					CebVarEtat(CebVars[i]) = (((delim != '=') || (*curseur == '='))? CEB_REF: CEB_DEF);
				}
			}
		}
		switch(delim) {
		  case '=':
			if(*curseur == '=') {
				curseur++;
				if(CebEnter(pc,oper)) pc++; else return(0); oper->instr = CEB_EQ;
			} else {
				if(oper->var == CEB_CONSTANTE) {
					sprintf(CebErreur,"'%s' ne peut figurer a gauche de '=' (n'est pas une variable)",
						sym);
					return(0);
				}
				if(a_sauver >= 0) {
					sprintf(CebErreur,"Une seule affectation est autorisee");
					return(0);
				}
				a_sauver = oper->var;
			}
			break;

		  case '(':
			if(sym[0] != '\0') {
				sprintf(CebErreur,"Operateur attendu entre '%s' et '('",sym);
				return(0);
			}
			if(niveau >= CEB_MAXPILE) {
				sprintf(CebErreur,"Deja %d niveaux de parentheses utilises",niveau - 1);
				return(0);
			}
			pc->instr = CEB_MONTE;
			pc->mode = CEB_NONE;
			pc++;
			oper++;
			oper->instr = CEB_INDEF;
			oper->mode = CEB_INDEF;
			niveau++;
			break;

		  case ')':
			if(niveau < 2) {
				sprintf(CebErreur,"Parenthese fermee en excedent");
				return(0);
			}
			if(CebEnter(pc,oper)) pc++; else return(0);
			--niveau; --oper;
			oper->mode = CEB_PILE;
			break;

		  case '+':
			if(CebEnter(pc,oper)) pc++; else return(0); oper->instr = CEB_ADD; break;

		  case '-':
			if(CebEnter(pc,oper)) pc++; else return(0); oper->instr = CEB_SUB; break;

		  case '*':
			if(CebEnter(pc,oper)) pc++; else return(0); oper->instr = CEB_MUL; break;

		  case '/':
			if(CebEnter(pc,oper)) pc++; else return(0); oper->instr = CEB_DIV; break;

		  case '%':
			if(CebEnter(pc,oper)) pc++; else return(0); oper->instr = CEB_MOD; break;

		  case '!':
			if(CebEnter(pc,oper)) pc++; else return(0);
			if(*curseur == '=') { curseur++; oper->instr = CEB_NE; }
			else {
				sprintf(CebErreur,"Operateur inconnu");
				return(0);
			}
			break;

		  case '>':
			if(CebEnter(pc,oper)) pc++; else return(0);
			if(*curseur == '=') { curseur++; oper->instr = CEB_GE; }
			else oper->instr = CEB_GT;
			break;

		  case '<':
			if(CebEnter(pc,oper)) pc++; else return(0);
			if(*curseur == '=') { curseur++; oper->instr = CEB_LE; }
			else oper->instr = CEB_LT;
			break;

		  case '&':
			if(CebEnter(pc,oper)) pc++; else return(0); oper->instr = CEB_AND; break;

		  case '|':
			if(CebEnter(pc,oper)) pc++; else return(0); oper->instr = CEB_OR; break;

		  case '\n': delim = ';'; break; /* pour le moment, fin de ligne == fin d'expression */

		}
	} while(delim != ';');

	if(CebEnter(pc,oper)) pc++; else return(0);

	while(niveau >= 2) {
		--niveau; --oper;
		oper->mode = CEB_PILE;
		if(CebEnter(pc,oper)) pc++; else return(0);
	}

	if((a_sauver >= 0) && (a_sauver < CebVarNb)) {
		pc->instr = CEB_STORE;
		pc->var = (unsigned short)a_sauver;
		pc->mode = CEB_ADRS;
		pc->val.adrs = CebVarAdrs(CebVars[a_sauver]);
		pc++;
	}

	pc->instr = CEB_EXIT;
	pc->mode = CEB_NONE;
	*fin = pc;

	return(1);
}
/* ================================================================ */
void CebPrgmList(CebInstr *prgm) { CebExec(prgm,-1); }
/* ================================================================ */
float CebExec(CebInstr *prgm, char debug) {
/* Deroule le code (listing et/ou execution).
   Si execution, renvoie le dernier resultat calcule present sur la pile */
	float *reg; CebInstr *pc;
	unsigned char instr; unsigned char mode;
	float *adrs;
	int num,i,nb_nop;

	reg = CebPile;
	pc = prgm;
	num = 0;
	nb_nop = 0;
	while((instr = pc->instr) != CEB_EXIT) {
		if(instr == CEB_NOP) {
			if(++nb_nop > CEB_MAXNOP) break;
		} else nb_nop = 0;
		mode = pc->mode;
		if(debug) {
			printf("%03d: %-6s",++num,CebInstrNom[instr]);
			switch(pc->mode) {
			  case CEB_NONE: printf("                   "); break;
			  case CEB_IMME:
				if(pc->var < CebVarNb) printf(" %-22s ",CebVars[pc->var].nom);
				else printf(" =%-16g      ",pc->val.imme);
				break;
			  case CEB_ADRS:
				if(pc->var < CebVarNb) printf(" %-22s ",CebVars[pc->var].nom);
				else {
					adrs = pc->val.adrs;
					for(i=0; i<CebVarNb; i++) if(adrs == CebVars[i].val.adrs) break;
					if(i == CebVarNb) {
						for(i=0; i<CebVarNb; i++) if(adrs == &(CebVars[i].val.imme)) break;
						if(i == CebVarNb) printf(" 0x%08lX              ",(unsigned long)adrs);
						else printf(" %-22s ",CebVars[i].nom);
					} else printf(" %-22s ",CebVars[i].nom);
				}
				break;
			  case CEB_PILE: printf(" Pile              "); break;
			}
			if(debug < 0) { printf("\n"); pc++; continue; }
		}
		switch(instr) {
		  case CEB_LOAD:
			switch(mode) {
			  case CEB_IMME: *reg = pc->val.imme; break;
			  case CEB_ADRS: *reg = *(pc->val.adrs); break;
			  case CEB_PILE: --reg; *reg = *(reg + 1); break;
			}
			break;

		  case CEB_STORE: *(pc->val.adrs) = *reg; break;

		  case CEB_MONTE: reg++; break;

		  case CEB_ADD:
			switch(mode) {
			  case CEB_IMME: *reg += pc->val.imme; break;
			  case CEB_ADRS: *reg += *(pc->val.adrs); break;
			  case CEB_PILE: --reg; *reg += *(reg + 1); break;
			}
			break;

		  case CEB_SUB:
			switch(mode) {
			  case CEB_IMME: *reg -= pc->val.imme; break;
			  case CEB_ADRS: *reg -= *(pc->val.adrs); break;
			  case CEB_PILE: --reg; *reg -= *(reg + 1); break;
			}
			break;

		  case CEB_MUL:
			switch(mode) {
			  case CEB_IMME: *reg *= pc->val.imme; break;
			  case CEB_ADRS: *reg *= *(pc->val.adrs); break;
			  case CEB_PILE: --reg; *reg *= *(reg + 1); break;
			}
			break;

		  case CEB_DIV:
			switch(mode) {
			  case CEB_IMME: *reg /= pc->val.imme; break;
			  case CEB_ADRS: *reg /= *(pc->val.adrs); break;
			  case CEB_PILE: --reg; *reg /= *(reg + 1); break;
			}
			break;

		  case CEB_MOD:
#ifdef MODULO
			switch(mode) {
			  case CEB_IMME: *reg = fmodf(*reg,pc->val.imme); break;
			  case CEB_ADRS: *reg = fmodf(*reg,*(pc->val.adrs)); break;
			  case CEB_PILE: --reg; *reg = fmodf(*reg,*(reg + 1)); break;
			}
#endif
			break;

		  case CEB_EQ:
			switch(mode) {
			  case CEB_IMME: *reg = (*reg == pc->val.imme)? 1.0: 0.0; break;
			  case CEB_ADRS: *reg = (*reg == *(pc->val.adrs))? 1.0: 0.0; break;
			  case CEB_PILE: --reg; *reg = (*reg == *(reg + 1))? 1.0: 0.0; break;
			}
			break;

		  case CEB_NE:
			switch(mode) {
			  case CEB_IMME: *reg = (*reg != pc->val.imme)? 1.0: 0.0; break;
			  case CEB_ADRS: *reg = (*reg != *(pc->val.adrs))? 1.0: 0.0; break;
			  case CEB_PILE: --reg; *reg = (*reg != *(reg + 1))? 1.0: 0.0; break;
			}
			break;

		  case CEB_GE:
			switch(mode) {
			  case CEB_IMME: *reg = (*reg >= pc->val.imme)? 1.0: 0.0; break;
			  case CEB_ADRS: *reg = (*reg >= *(pc->val.adrs))? 1.0: 0.0; break;
			  case CEB_PILE: --reg; *reg = (*reg >= *(reg + 1))? 1.0: 0.0; break;
			}
			break;


		  case CEB_GT:
			switch(mode) {
			  case CEB_IMME: *reg = (*reg > pc->val.imme)? 1.0: 0.0; break;
			  case CEB_ADRS: *reg = (*reg > *(pc->val.adrs))? 1.0: 0.0; break;
			  case CEB_PILE: --reg; *reg = (*reg > *(reg + 1))? 1.0: 0.0; break;
			}
			break;

		  case CEB_LE:
			switch(mode) {
			  case CEB_IMME: *reg = (*reg <= pc->val.imme)? 1.0: 0.0; break;
			  case CEB_ADRS: *reg = (*reg <= *(pc->val.adrs))? 1.0: 0.0; break;
			  case CEB_PILE: --reg; *reg = (*reg <= *(reg + 1))? 1.0: 0.0; break;
			}
			break;

		  case CEB_LT:
			switch(mode) {
			  case CEB_IMME: *reg = (*reg < pc->val.imme)? 1.0: 0.0; break;
			  case CEB_ADRS: *reg = (*reg < *(pc->val.adrs))? 1.0: 0.0; break;
			  case CEB_PILE: --reg; *reg = (*reg < *(reg + 1))? 1.0: 0.0; break;
			}
			break;

		  case CEB_AND:
			switch(mode) {
			  case CEB_IMME: *reg = ((*reg != 0.0) && (pc->val.imme != 0.0))? 1.0: 0.0; break;
			  case CEB_ADRS: *reg = ((*reg != 0.0) && (*(pc->val.adrs) != 0.0))? 1.0: 0.0; break;
			  case CEB_PILE: --reg; *reg = ((*reg != 0.0) && (*(reg + 1) != 0.0))? 1.0: 0.0; break;
			}
			break;

		  case CEB_OR:
			switch(mode) {
			  case CEB_IMME: *reg = ((*reg != 0.0) || (pc->val.imme != 0.0))? 1.0: 0.0; break;
			  case CEB_ADRS: *reg = ((*reg != 0.0) || (*(pc->val.adrs) != 0.0))? 1.0: 0.0; break;
			  case CEB_PILE: --reg; *reg = ((*reg != 0.0) || (*(reg + 1) != 0.0))? 1.0: 0.0; break;
			}
			break;
		}
		if(debug) {
			if(instr != CEB_MONTE) printf("= %g\n",*reg); else printf("\n");
		}

		pc++;
	}
	return(*reg);
}
/* ================================================================ */
void CebPrgmClear(CebInstr *prgm) { prgm->instr = CEB_EXIT; }

