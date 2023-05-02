#ifdef macintosh
#pragma message(__FILE__)
#endif
#define BRANCHE_AUTOM
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>   /* pour open() */

#include <environnement.h>
#include <claps.h>
#include <decode.h>
#include <dateheure.h>
#include <opium_demande.h>
#include <opium.h>
#include <samba.h>
#include <objets_samba.h>
#ifndef CODE_WARRIOR_VSN
#include <tcp/sktlib.h>
#endif

#define AUTOM_C
#include <autom.h>

#define MAXTRAME 8192

/* typedef enum {
	AUTOM_LNGR = 0,
	AUTOM_DEF,
	AUTOM_VAL
} AUTOM_ATTENDU; */

typedef enum {
	AUTOM_FICHIER = 1,
	AUTOM_SERVEUR,
	AUTOM_NBSRCES
} AUTOM_SRCE;
#define AUTOM_SRCE_CLES "neant/fichier/serveur"

typedef enum {
	AUTOM_EDW = 0,
	AUTOM_JSON,
	AUTOM_MIDAS,
	AUTOM_NBFMTS
} AUTOM_FORMAT;
#define AUTOM_FMT_CLES "edw/json/midas"

typedef enum {
	AUTOM_VOYANT = 1,
	AUTOM_COLONNE,
	AUTOM_CADRANT,
	AUTOM_NBAFFS
} AUTOM_AFFICHAGE;
#define AUTOM_AFF_CLES "non/voyant/colonne/cadrant"
#define AUTOM_MAX_FONDS 6
static OpiumColor AutomFonds[6] = { { GRF_RGB_RED }, { GRF_RGB_GREEN }, { GRF_RGB_YELLOW },
	{ GRF_RGB_ORANGE }, { WND_COLOR_DEMI, WND_COLOR_DEMI, WND_COLOR_MAX }, { GRF_RGB_TURQUOISE } };

#define AUTOM_CONFG "2;2"
#define AUTOM_WRITE "1;"
#define AUTOM_READ  "2;1"

extern ArgDesc *ArchAutomInfo;
static ArgDesc *AutomValeursDesc;
static char AutomSrcePath[MAXFILENAME];

/* .......... Parametrage general .......... */
static char AutomPrefsVsn;
static char AutomSrceType,AutomSrceFmt;
static char AutomSrceFic[MAXFILENAME],AutomSrceSvr[MAXFILENAME],AutomUserFic[MAXFILENAME],AutomUserVal[MAXFILENAME];
static int  AutomPort,AutomPermanent;
static ArgDesc AutomDesc[] = {
	{ "version",   DESC_CHAR                 &AutomPrefsVsn,  "version du format de ce fichier (garder cette variable en premier!)" },
	{ "source",    DESC_KEY AUTOM_SRCE_CLES, &AutomSrceType,  "origine des variables et de leur valeur ("AUTOM_SRCE_CLES")" },
	{ "fichier",   DESC_STR(MAXFILENAME)      AutomSrceFic,   "nom du fichier de transmission" },
	{ "serveur",   DESC_STR(MAXFILENAME)      AutomSrceSvr,   "adresse IP du serveur" },
	{ "port",      DESC_INT                  &AutomPort,      "port IP du serveur" },
	{ "format",    DESC_KEY AUTOM_FMT_CLES,  &AutomSrceFmt,   "format des donnees du serveur ("AUTOM_FMT_CLES")" },
	{ "permanent", DESC_BOOL                 &AutomPermanent, "garde la connexion en permanence" },
	{ "variables", DESC_STR(MAXFILENAME)      AutomUserFic,   "nom du fichier des variables a afficher et a archiver" },
	{ "valeurs",   DESC_STR(MAXFILENAME)      AutomUserVal,   "nom du fichier des valeurs (si source=fichier et format=json)" },
	DESC_END
};

/* .......... Variables delivrees par le support .......... */
static TypeAutomVar AutomVarLue;
static ArgDesc AutomVarDesc[] = {
	{ "name",   DESC_STR(MAXVARNOM)        AutomVarLue.nom,  0 },
	{ "code",   DESC_KEY AUTOM_CODE_CLES, &AutomVarLue.code, 0 },
	{ "size",   DESC_SHORT                &AutomVarLue.size, 0 },
	{ "keys",   DESC_STR(AUTOM_CLES_LNGR)  AutomVarLue.cles, 0 },
	{ "status", DESC_KEY AUTOM_ETAT_CLES, &AutomVarLue.etat, 0 },
	DESC_END
};
static ArgStruct AutomVarAS = { (void *)AutomVar, (void *)&AutomVarLue, sizeof(TypeAutomVar), AutomVarDesc };
static ArgDesc AutomVarsDesc[] = {
	{ "variables", DESC_STRUCT_SIZE(AutomVarNb,AUTOM_VAR_MAX) &AutomVarAS, 0 },
	DESC_END
};

static struct {
	char type,mode;
	short lngr;
	short nb;
	char validite[16];
} AutomMidasDef;
static ArgDesc AutomMidasKeyDesc[] = {
	{ "type",         DESC_CHAR    &AutomMidasDef.type, 0 },
	{ "access_mode",  DESC_CHAR    &AutomMidasDef.mode, 0 },
	{ "item_size",    DESC_SHORT   &AutomMidasDef.lngr, 0 },
	{ "num_values",   DESC_SHORT   &AutomMidasDef.nb,   0 },
	{ "last_written", DESC_STR(16) &AutomMidasDef.validite, 0 },
	DESC_END
};
/* .......... Variables utilisees dans Samba .......... */
static TypeAutomUserVar AutomUserVarLue;
static TypeAutomSrceVar AutomSrceVarLue;
static ArgDesc AutomUserDescBool[] = { DESC_END };
static ArgDesc AutomUserDescKey[] = {
	{ "cles",    DESC_STR(AUTOM_CLES_LNGR)    AutomUserVarLue.cles,    0 },
	DESC_END
};
static ArgDesc AutomUserDescInt[] = {
	{ "min", DESC_INT &AutomUserVarLue.min.i, 0 },
	{ "max", DESC_INT &AutomUserVarLue.max.i, 0 },
	DESC_END
};
static ArgDesc AutomUserDescFloat[] = {
	{ "min", DESC_FLOAT &AutomUserVarLue.min.r, 0 },
	{ "max", DESC_FLOAT &AutomUserVarLue.max.r, 0 },
	DESC_END
};
static ArgDesc *AutomUserAffDesc[] = { AutomUserDescBool, AutomUserDescKey, AutomUserDescInt, AutomUserDescFloat };
static ArgDesc AutomUserVarDesc[] = {
	{ 0,         DESC_LISTE AutomListe,      &AutomUserVarLue.num,     0 },
	{ 0,         DESC_STR(AUTOM_USER_LIBELLE) AutomUserVarLue.libelle, 0 },
	{ "index",   DESC_SHORT                  &AutomUserVarLue.index,   0 },
	{ "unite",   DESC_STR(AUTOM_USER_UNITE)   AutomUserVarLue.unite,   0 },
	{ "archive", DESC_BOOL                   &AutomUserVarLue.archive, 0 },
	{ "affiche", DESC_KEY AUTOM_AFF_CLES,    &AutomUserVarLue.affiche, 0 },
	{ "format",  DESC_KEY AUTOM_CODE_CLES,   &AutomUserVarLue.type,    0 },
	{ 0, DESC_VARIANTE(AutomUserVarLue.type)  AutomUserAffDesc,        0 },
	DESC_END
};
static ArgDesc AutomSrceVarDesc[] = {
	{ 0,         DESC_LISTE AutomListe,      &AutomSrceVarLue.num,     0 },
	{ 0,         DESC_STR(AUTOM_USER_LIBELLE) AutomSrceVarLue.libelle, 0 },
	{ "index",   DESC_SHORT                  &AutomSrceVarLue.index,   0 },
	{ "role",    DESC_KEY AUTOM_ROLE_CLES,   &AutomSrceVarLue.role,    0 },
	DESC_END
};
static ArgStruct AutomUserAS = { (void *)AutomUserVar, (void *)&AutomUserVarLue, sizeof(TypeAutomUserVar), AutomUserVarDesc };
static ArgStruct AutomSrceAS = { (void *)AutomSrceVar, (void *)&AutomSrceVarLue, sizeof(TypeAutomSrceVar), AutomSrceVarDesc };
static ArgDesc AutomUserDesc[] = {
	{ "utilise",    DESC_STRUCT_SIZE(AutomUserNb,AUTOM_ARCH_MAX) &AutomUserAS, 0 },
	{ "sources",    DESC_STRUCT_SIZE(AutomSrceNb,AUTOM_SRCE_MAX) &AutomSrceAS, 0 },
	{ "autorisees", DESC_BOOL  &AutomSrceAccede, "Autorisation de deplacer les sources" },
	DESC_END
};

static char AutomHome[MAXVARNOM];
static FILE *AutomTrameFile;
static int AutomNum;
static Panel pAutomUtilise;
static char AutomChoix; float AutomAffMin,AutomAffMax;
#ifndef CODE_WARRIOR_VSN
static TypeSocket AutomSkt;
#endif

static char AutomValeursLues;
static char AutomASauver;

static void AutomTraduitVar(int num, char *texte, int max);

/* ========================================================================== */
int AutomRang(char *nom) {
	int i;
	
    for(i=0; i<AutomVarNb; i++) if(!strcmp(nom,AutomVar[i].nom)) break;
    if(i < AutomVarNb) return(i); else return(-1);
}
/* ========================================================================== */
char AutomCode(int num) {
	if((num >= 0) && (num < AutomVarNb)) return(AutomVar[num].code);
	else return(0);
}
/* ========================================================================== */
int *AutomAdrsInt(int num) {
	if((num >= 0) && (num < AutomVarNb)) return(&(AutomVar[num].val.i));
    else return(0);
}
/* ========================================================================== */
char *AutomAdrsKey(int num) {
	if((num >= 0) && (num < AutomVarNb)) return(&(AutomVar[num].val.b));
	else return(0);
}
/* ========================================================================== */
float *AutomAdrsFloat(int num) {
	if((num >= 0) && (num < AutomVarNb)) return(&(AutomVar[num].val.r));
	else return(0);
}
/* ========================================================================== */
char *AutomNom(int num) {
	if((num >= 0) && (num < AutomVarNb)) return(AutomVar[num].nom);
	else return(AutomVar[AutomVarNb].nom);
}
/* ========================================================================== */
static void AutomVarRaz(TypeAutomVar *var, char etat, char contenu) {
	var->utilisee = 0;
	if(etat >= 0) var->etat = etat;
	var->contenu = contenu;
	switch(var->code) {
		case AUTOM_BOOL: var->cles[0] = '\0'; case AUTOM_CLE: var->val.b = 0; break;
		case AUTOM_INT: var->val.i = 0; var->cles[0] = '\0'; break;
		case AUTOM_FLOAT: var->val.r = 0.0; var->cles[0] = '\0'; break;
	}
}
/* ========================================================================== */
void AutomUserFromVar(int j) {
	TypeAutomUserVar *user; TypeAutomVar *var; TypeAutomVal *val; int i;

	user = &(AutomUserVar[j]); i = user->num;
	if((i < 0) || (i >= AutomVarNb)) return;
	var = &(AutomVar[i]);
#ifdef VAR_VECTOR
	if(var->size < 2) val = &(var->val);
	else {
		short k;
		k = user->index;
		if((k < 0) || (k >= var->size)) return;
		val = &(var->tab[k]);
	}
#else
	val = &(var->val);
#endif
	switch(user->type) {
		case AUTOM_BOOL:
			switch(var->code) {
				case AUTOM_BOOL:
				case AUTOM_CLE:   user->val.b = val->b; break;
				case AUTOM_INT:   user->val.b = (val->i > 0); break;
				case AUTOM_FLOAT: user->val.b = (val->r > 0.5); break;
			}
			break;
		case AUTOM_CLE:
			switch(var->code) {
				case AUTOM_BOOL:
				case AUTOM_CLE:   user->val.b = val->b; break;
				case AUTOM_INT:   user->val.b = (char)val->i; break;
				case AUTOM_FLOAT: user->val.b = (char)(val->r + 0.5); break;
			}
			break;
		case AUTOM_INT:
			switch(var->code) {
				case AUTOM_BOOL:
				case AUTOM_CLE:   user->val.i = (int)val->b; break;
				case AUTOM_INT:   user->val.i = val->i; break;
				case AUTOM_FLOAT: user->val.i = (int)val->r; break;
			}
			break;
		case AUTOM_FLOAT:
			switch(var->code) {
				case AUTOM_BOOL:
				case AUTOM_CLE:   user->val.r = (float)val->b; break;
				case AUTOM_INT:   user->val.r = (float)val->i; break;
				case AUTOM_FLOAT: user->val.r = val->r; break;
			}
			break;
	}
	/* char txt[80];
	AutomTraduitVar(i,txt,80); printf("(%s) supp %s = %s\n",__func__,var->nom,txt);
	switch(user->type) {
		case AUTOM_BOOL:
		case AUTOM_CLE:   printf("(%s) user %s = %d\n",__func__,var->nom,user->val.b); break;
		case AUTOM_INT:   printf("(%s) user %s = %d\n",__func__,var->nom,user->val.i); break;
		case AUTOM_FLOAT: printf("(%s) user %s = %g\n",__func__,var->nom,user->val.r); break;
	} */
}
/* ========================================================================== */
void AutomSrceFromVar(int j) {
	TypeAutomVar *var; TypeAutomSrceVar *srce; TypeAutomVal *val; int i;

	srce = &(AutomSrceVar[j]); i = srce->num;
	if((i < 0) || (i >= AutomVarNb)) return;
	var = &(AutomVar[i]);
#ifdef VAR_VECTOR
	if(var->size < 2) val = &(var->val);
	else {
		short k;
		k = user->index;
		if((k < 0) || (k >= var->size)) return;
		val = &(var->tab[k]);
	}
#else
	val = &(var->val);
#endif
	switch(var->code) {
		case AUTOM_BOOL:
		case AUTOM_CLE:   srce->active = val->b; break;
		case AUTOM_INT:   srce->active = (val->i > 0); break;
		case AUTOM_FLOAT: srce->active = (val->r > 0.5); break;
	}
}
/* ========================================================================== */
static void AutomExemple() {
	int i = 0; int source;
	strcpy(AutomVar[i].nom,"Pmain"); AutomVar[i].code = AUTOM_FLOAT; i++;
	strcpy(AutomVar[i].nom,"Pauxi"); AutomVar[i].code = AUTOM_FLOAT; i++;
	strcpy(AutomVar[i].nom,"HVnorth"); AutomVar[i].code = AUTOM_INT; i++;
	strcpy(AutomVar[i].nom,"HVsouth"); AutomVar[i].code = AUTOM_INT; i++;
	strcpy(AutomVar[i].nom,"SourceType"); AutomVar[i].code = AUTOM_CLE; strcpy(AutomVar[i].cles,"Fe/AmBe"); i++;
	strcpy(AutomVar[i].nom,"SourceInUse"); AutomVar[i].code = AUTOM_BOOL; source = i; i++;
	AutomVarNb = i;
	for(i=0; i<AutomVarNb; i++) AutomVarRaz(&(AutomVar[i]),AUTOM_READONLY,AUTOM_DO);

	AutomUserNb = AutomVarNb;
	for(i=0; i<AutomUserNb; i++) {
		AutomUserVar[i].num = i; AutomUserVar[i].index = 0;
		AutomUserVar[i].type = AutomVar[i].code;
		strcpy(AutomUserVar[i].libelle,AutomVar[i].nom);
		strcpy(AutomUserVar[i].cles,AutomVar[i].cles);
	}
	AutomSrceNb = 1;
	for(i=0; i<AutomSrceNb; i++) {
		AutomSrceVar[i].num = source; AutomSrceVar[i].index = 0;
		strcpy(AutomSrceVar[i].libelle,"Source in use");
		AutomSrceVar[i].role = AUTOM_CALIB;
	}
}
/* ========================================================================== */
static void AutomDecritValeurs() {
	ArgDesc *elt; int num; TypeAutomVar *var;

	if(AutomValeursDesc) { free(AutomValeursDesc); AutomValeursDesc = 0; }
	if(AutomVarNb) {
		AutomValeursDesc = ArgCreate(AutomVarNb);
		elt = AutomValeursDesc;
		for(num=0; num<AutomVarNb; num++) {
			var = &(AutomVar[num]);
			if(var->size > 1) continue;
			switch(var->code) {
			  case AUTOM_BOOL:   ArgAdd(elt++,var->nom,DESC_BOOL  &(var->val.b),0); break;
			  case AUTOM_INT:    ArgAdd(elt++,var->nom,DESC_INT   &(var->val.i),0); break;
			  case AUTOM_FLOAT:  ArgAdd(elt++,var->nom,DESC_FLOAT &(var->val.r),0); break;
			  case AUTOM_CLE:    ArgAdd(elt++,var->nom,DESC_KEY var->cles,&(var->val.b),0); break;
			  case AUTOM_STRING: ArgAdd(elt++,var->nom,DESC_STR(MAXVALSTR) var->val.s,0); break;
			}
		}
	}
}
/* ========================================================================== */
static void AutomDecritArch() {
	int i,num,nsav; ArgDesc *elt; TypeAutomUserVar *user; TypeAutomVar *var;

	if(CompteRendu.vars_manip) printf("  . Construction du descripteur des variables archivees\n");
	nsav = 0;
	if(ArchAutomInfo) { free(ArchAutomInfo); ArchAutomInfo = 0; }
#ifdef BIDOUILLE_EDW
	if(AutomUserNb == 0) {
		int i,j;
		printf("    * Pas de variable utilisateur declaree: utilisation des variables par defaut\n");
		i = 0;
		while(AutomInterne[i].nom[0]) {
			printf("    + (%d) '%s'\n",i+1,AutomInterne[i].nom);
			for(j=0; j<AutomVarNb; j++) if(!strcmp(AutomInterne[i].nom,AutomVar[j].nom)) break;
			if(j < AutomVarNb) {
				user = &(AutomUserVar[AutomUserNb]);
				user->num = j;
				user->type = AutomInterne[i].type;
				strncpy(user->libelle,AutomInterne[i].libelle,AUTOM_USER_LIBELLE);
				user->archive = user->affiche = 1;
				switch(user->type) {
				  case AUTOM_BOOL: case AUTOM_CLE: user->min.b = user->max.b = 0; break;
				  case AUTOM_INT: user->min.i = user->max.i = 0; break;
				  case AUTOM_FLOAT: user->min.r = user->max.r = 0; break;
				}
				user->instrum = 0; user->panel = 0 ;
				if(++AutomUserNb >= AUTOM_ARCH_MAX) break;
			}
			i++;
		}
	}
#endif /* BIDOUILLE_EDW */
	if(AutomUserNb) {
		ArchAutomInfo = ArgCreate(AutomUserNb);
		elt = ArchAutomInfo;
		for(i=0; i<AutomUserNb; i++) if(AutomUserVar[i].archive) {
			user = &(AutomUserVar[i]); var = &(AutomVar[user->num]);
			if(var->size > 1) continue;
			num = user->num;
			if((num < 0) || (num >= AutomVarNb)) continue;
			switch(user->type) {
			  case AUTOM_BOOL:  ArgAdd(elt++,var->nom,DESC_BOOL  &(user->val.b),user->libelle); nsav++; break;
			  case AUTOM_INT:   ArgAdd(elt++,var->nom,DESC_INT   &(user->val.b),user->libelle); nsav++; break;
			  case AUTOM_FLOAT: ArgAdd(elt++,var->nom,DESC_FLOAT &(user->val.r),user->libelle); nsav++; break;
			  case AUTOM_CLE:   ArgAdd(elt++,var->nom,DESC_KEY user->cles, &(user->val.b),user->libelle); nsav++; break;
			}
		}
		if(!nsav) { free(ArchAutomInfo); ArchAutomInfo = 0; }
	}
	if(CompteRendu.vars_manip) printf("    : %d variable%s archivee%s\n",Accord2s(nsav));
}
/* ========================================================================== */
static int AutomConnecte(char *raison) {
	if(AutomLink == AUTOM_NOLINK) return(1);
	if(AutomLink == -1) {
		// printf("(%s) Reconnexion\n",__func__);
		if(AutomSrceType == AUTOM_FICHIER) {
			AutomLink = open(AutomSrcePath,O_RDONLY,0);
			if(AutomLink == -1) {
				sprintf(raison,"Impossible d'ouvrir le fichier %s: %s\n",AutomSrcePath,strerror(errno));
				AutomLink = AUTOM_NOLINK;
				return(0);
			}
		} else {
			AutomLink = SocketClient(AutomSrceSvr,AutomPort,&AutomSkt);
			if(AutomLink == -1) {
				sprintf(raison,"Impossible de se connecter sur %s:%d (%d: %s)",
					AutomSrceSvr,AutomPort,errno,strerror(errno));
				return(0);
			}
		}
	} else {
		// printf("(%s) Deja connecte sur <%d>\n",__func__,AutomLink);
		if(AutomSrceType == AUTOM_FICHIER) lseek(AutomLink,0,SEEK_SET);
	}
	return(1);
}
/* ========================================================================== */
int Autom1Ecriture(char *commande, int num, char *raison) {
	int d,l,t,n; int rc; int attendu; char trame_ip[256]; TypeAutomVar *var;
	char valeur[32];
#ifdef DEBUG_SC
	int k;
#endif

	// printf("(%s) AutomLink = %d\n",__func__,AutomLink);
	attendu = AutomVarNb? AUTOM_TRAME_DONNEES: AUTOM_TRAME_CONFIG;
	sprintf(raison,"pas d'erreur detectee"); rc = 1;
	if(AutomLink == AUTOM_NOLINK) goto laferme;
	if(!commande) return(-1);
	if(!AutomConnecte(raison)) return(-1);
	rc = 0;
	if(!strcmp(commande,AUTOM_WRITE)) {
		if(num >= 0) {
			var = &(AutomVar[num]); valeur[31] = '\0';
			switch(var->code) {
			  case AUTOM_BOOL:
				if(AutomSrceFmt == AUTOM_EDW) strcpy(valeur,(var->val.b)? "1": "0");
				else if(AutomSrceFmt == AUTOM_JSON) strcpy(valeur,(var->val.b)? "on": "off");
				break;
			  case AUTOM_CLE:
				n = ArgKeyGetNb(var->cles);
				if(var->val.b < 0) var->val.b = 0; else if(var->val.b >= n) var->val.b = n - 1;
				ArgKeyGetText(var->cles,var->val.b,valeur,31);
				break;
			  case AUTOM_INT:   snprintf(valeur,31,"%d",var->val.i); break;
			  case AUTOM_FLOAT: snprintf(valeur,31,"%g",var->val.r); break;
			}
			if(AutomSrceFmt == AUTOM_EDW) sprintf(trame_ip,"%s%d;%s",commande,num,valeur);
			else if(AutomSrceFmt == AUTOM_JSON) sprintf(trame_ip,"write %s = %s",AutomVar[num].nom,valeur);
			d = strlen(trame_ip);
			sprintf(raison,"Commande '%s = %s' inoperante",AutomVar[num].nom,valeur); /* en reserve */
		} else { sprintf(raison,"Variable indefinie pour une commande WRITE"); goto laferme; }
	} else if(AutomSrceFmt == AUTOM_JSON) {
		if(!strcmp(commande,AUTOM_CONFG)) {
			sprintf(trame_ip,"read config");
			d = strlen(trame_ip);
		} else if(!strcmp(commande,AUTOM_READ)) {
			sprintf(trame_ip,"read data");
			d = strlen(trame_ip);
		} else { sprintf(raison,"Commande '%s' inoperante",commande); goto laferme; }
	} else if(AutomSrceFmt == AUTOM_MIDAS) {
		d = 0;
	} else {
		if((d = strlen(commande)) > 253) {
			sprintf(raison,"Trame a ecrire trop grande: %d/253 octets permis",d);
			goto laferme;
		}
		if(!strncmp(commande,"2;",2) && (d == 3)) {
			if(commande[2] == '0') attendu = AUTOM_TRAME_CLIENTS;
			else if(commande[2] == '1') { rc = 1; goto laferme; }
			else if(commande[2] == '2') attendu = AUTOM_TRAME_CONFIG;
			else if(commande[2] == '3') attendu = AUTOM_TRAME_NOM;
			else if(commande[2] == '6') attendu = AUTOM_TRAME_ETATS;
		}
		strcpy(trame_ip,commande);
	}
	if(d) {
		trame_ip[d] = 0x13; trame_ip[d + 1] = 0x10; trame_ip[d + 2] = 0;
		l = d + 2; // strlen(trame_ip)
		t = 0;
		if(AutomLink >= 0) while(t < l) {
			n = write(AutomLink,trame_ip+t,l-t);
		#ifdef DEBUG_SC
			printf("Ecrit %d/%d (au total %d/%d) caracteres dans le chemin #%d:\n",
				   n,l-t,t+n,l,AutomLink);
			for(k=t; k<l; k++) printf(" %02X",trame_ip[k]);
			trame_ip[d] = '\0';
			printf(" (%s)\n",trame_ip);
		#endif
			if(n < 0) {
				sprintf(raison,"Commande pas envoyee (%d: %s)",errno,strerror(errno));
				goto laferme;
			}
			t += n;
		}
	}
	rc = 1;
laferme:
	if(rc) return(attendu);
#ifndef CODE_WARRIOR_VSN
	else {
		if((AutomLink >= 0) && !AutomPermanent) {
			if(AutomSrceType == AUTOM_FICHIER) close(AutomLink);
			else SocketFermee(AutomLink);
			AutomLink = -1;
			// printf("(%s) Deconnexion\n",__func__);
		};
		return(-1);
	}
#endif

}
// #define DEBUG_SC
// #define DEBUG_SC2
/* ========================================================================== */
static int Autom1Lecture(AUTOM_TRAME attendu, int indx, char *raison) {
	char lue[MAXTRAME],*c,*d,*s,*e,sep; int j,k,t,rc,n,v; char def;
	char *nom,*valtxt;
	int tentative,lngr,fin,nb,nblues,nbmax,type,num,etat,contenu; float val,nouv;
	ARG_STYLES style_normal;
	TypeAutomVar *var;
#ifdef DEBUG_SC2
	#define DEBUG_SC
#endif
#ifdef DEBUG_SC
	char p;
#endif

	// printf("(%s) AutomLink = %d\n",__func__,AutomLink);
	sprintf(raison,"pas d'erreur detectee"); rc = 1;
	if(AutomLink == AUTOM_NOLINK) goto laferme;
	if((AutomSrceFmt == AUTOM_EDW) && (indx >= 0)) {
		var = &(AutomVar[indx]);
		switch(var->code) {
		  case AUTOM_BOOL:
		  case AUTOM_CLE:   nouv = (float)(var->val.b); break;
		  case AUTOM_INT:   nouv = (float)(var->val.i); break;
		  case AUTOM_FLOAT: nouv = var->val.r; break;
		}
	}
/*
 *	Reception de trames de support jusqu'au type attendu
 */
	nblues = 0; nbmax = 10;
	n = 0;
	tentative = 30;
	lngr = 0;
	do {
		if(AutomSrceFmt == AUTOM_EDW) {
			if(AutomLink >= 0) {
				t = read(AutomLink,lue,MAXTRAME);
				if(AutomTrameFile) { lue[t] = '\0'; fputs(lue,AutomTrameFile); }
			} else {
			#ifdef DEBUG_SC
				printf("  . lit maxi %d caracteres sur le chemin #%d (type attendu: %d)\n",MAXTRAME,-AutomLink,attendu);
				t = read(-AutomLink,lue,MAXTRAME);
				printf("  . obtenu %d caractere%s @%08X:\n",Accord1s(t),lue);
				p = lue[t]; lue[t] = '\0';
				printf("---------- [ %s ] ----------\n",lue);
				lue[t] = p;
			#else
				t = read(-AutomLink,lue,MAXTRAME); lue[t] = '\0';
			#endif
			}
			if(!strcmp(lue,"+")) continue;
			k = 0; c = lue;
			while(k < (t-1)) {
				if(*c == '$') {
					if(*(c+1) == '$') {
					#ifdef DEBUG_SC
						printf(". debut de trame trouve\n");
					#endif
						s = c; v = k;
						e = c + 4;
						if(*e != ';') {
							*(e + 16) = '\0';
							sprintf(raison,"Erreur de syntaxe dans l'entete de trame automate ('%s etc...')",c);
							rc = 0; goto laferme;
						}
						*e++ = '\0';
						// printf(". type transmis: '%s'\n",s+3);
						sscanf(s+3,"%hhd",(char *)&type);
					#ifdef DEBUG_SC
						printf(". type = %d/%d\n",type,attendu);
					#endif
						do { if(*c == '\n') break; c++; k++; } while(k < t);
						*c++ = '\0'; k++;
						sscanf(e,"%d",&lngr);
						// printf(". lngr=%d\n",lngr);
						n = k + lngr;
						if(n > MAXTRAME) {
							if(v > 0) {
								t = t - v;
								memmove(lue,lue+v,t);
								k -= v; c -= v; n -= v;
								s = lue; v = 0;
							}
						}
						fin = n;
						// printf(". fin=%d/%d\n",fin,t);
						if(fin > MAXTRAME) fin = MAXTRAME; /* La trame devra etre lue par morceaux */
						while(t < fin) {
							if(AutomLink >= 0) {
								nb = read(AutomLink,lue + t,MAXTRAME - t);
								if(AutomTrameFile) { lue[t+nb] = '\0'; fputs(lue+t,AutomTrameFile); }
							} else nb = read(-AutomLink,lue + t,MAXTRAME - t);
							if(nb <= 0) { fin = t; break; }
							t += nb;
						}
					#ifdef DEBUG_SC2
						printf("===== %-7s =======================================\n",AutomTrameTexte(type));
						// if(type == attendu) {
						printf("Trame @%08X[%d] type %d/%d:\n",c,fin,type,attendu);
						p = lue[fin]; lue[fin] = '\0';
						printf("%s\n",c);
						// WndStep("Impression terminee, c=%08X",c);
						lue[fin] = p;
						// }
						if(t <= fin) printf("===== fin de transmission\n");
					#endif
						attendu = AutomVarNb? AUTOM_TRAME_DONNEES: AUTOM_TRAME_CONFIG;
						if(type == attendu) {
							if(type == AUTOM_TRAME_CONFIG) { AutomVarNb = 0; AutomValeursLues = 0; }
							while(k < n) {
								d = c; /* prochaine "ligne": entre d et c */
								do {
									if(*c == '\n') break;
									c++; k++;
									if((k > fin) && (fin < n)) {
										/* En attendant la lecture d'un autre morceau de trame.. */
										sprintf(raison,"Trame inachevee (%d/%d)",fin,n);
										rc = 0; goto laferme;
									}
								} while(k < n);
								*c++ = '\0'; k++;
								switch(type) /* decodage d'une ligne */ {
								  case AUTOM_TRAME_DONNEES:
									sscanf(ItemSuivant(&d,';'),"%d",&num);
									if((num >= 0) && (num < AutomVarNb)) {
										sscanf(ItemSuivant(&d,';'),"%d",&etat);
										if(etat >= 0) {
											AutomVar[num].etat = etat;
											if((AutomVar[num].contenu == AUTOM_TIME) || (AutomVar[num].contenu == AUTOM_STATETIME)) {
												val = 0.0; /* une heure est disponible dans <d> */
											} else sscanf(d,"%g",&val);
											AutomVar[num].val.r = val;
											AutomValeursLues = 1;
										#ifdef DEBUG_SC
											printf("  | %3d) %s = %g\n",num,AutomVar[num].nom,AutomVar[num].val.r);
										#endif
										} else AutomVar[num].etat = AUTOM_READONLY;
									}
									if(indx >= 0) { if(num == indx) { if(val == nouv) break; } }
									break;
								  case AUTOM_TRAME_CONFIG:
									if(AutomVarNb < AUTOM_VAR_MAX) {
										strncpy(AutomVar[AutomVarNb].nom,ItemSuivant(&d,';'),MAXVARNOM);
										sscanf(d,"%d",&contenu);
										AutomVar[AutomVarNb].code = AUTOM_FLOAT;
										AutomVarRaz(&(AutomVar[AutomVarNb]),AUTOM_READONLY,(char)contenu);
									#ifdef DEBUG_SC
										printf("  | Variable #%d: %s (de type #%d)\n",AutomVarNb,AutomVar[AutomVarNb].nom,AutomVar[AutomVarNb].contenu);
									#endif
										AutomVarNb++; AutomListe[AutomVarNb][0] = '\0';
									} else {
										sprintf(raison,"Variable automate en excedent: %s",d);
										rc = 0; goto laferme;
									}
									rc = 1;
									break;
								  case AUTOM_TRAME_ETATS: break;
								  case AUTOM_TRAME_NOM: strncpy(AutomHome,d,MAXVARNOM); break;
								  case AUTOM_TRAME_CLIENTS: break;
								  default: break;
								}
							}
						}
						if(++nblues >= nbmax) break;
						else {
							// if(rc) break;
							if(rc && ((AutomLink >= 0) || (type == AUTOM_TRAME_DONNEES))) break;
							t = t - fin;
							if(t > 0) memmove(lue,lue+fin,t);
							k = 0; c = lue;
							lngr = 0;
							continue;
						}
					}
				}
				k++; c++;
			}
		} else if(AutomSrceFmt == AUTOM_JSON) {
			int chemin;
			if(AutomLink == -1) { if(!AutomConnecte(raison)) { rc = 0; goto laferme; } }
			chemin = (AutomLink >= 0)? AutomLink: -AutomLink;
			switch(attendu) {
			  case AUTOM_TRAME_CONFIG:
				ArgFromPath(chemin,AutomVarsDesc,"+");
				if(AutomTrameFile) {
					if(AutomSrceFmt == AUTOM_JSON) style_normal = ArgStyle(ARG_STYLE_JSON);
					ArgBegin(AutomTrameFile,0,AutomVarsDesc);
					if(AutomSrceFmt == AUTOM_JSON) ArgStyle(style_normal);
				}
				AutomDecritValeurs();
				attendu = AUTOM_NBTYPES_TRAME;
				break;
			  case AUTOM_TRAME_DONNEES:
				ArgFromPath(chemin,AutomValeursDesc,0);
				if(AutomTrameFile) {
					if(AutomSrceFmt == AUTOM_JSON) style_normal = ArgStyle(ARG_STYLE_JSON);
					ArgAppend(AutomTrameFile,0,AutomValeursDesc);
					if(AutomSrceFmt == AUTOM_JSON) ArgStyle(style_normal);
				}
				attendu = AUTOM_NBTYPES_TRAME;
				break;
			  default: break;
			}
		} else if(AutomSrceFmt == AUTOM_MIDAS) {
			TypeAutomVar *var; int chemin; int ligne,lngr;
			chemin = (AutomLink >= 0)? AutomLink: -AutomLink;
			attendu = AutomVarNb? AUTOM_TRAME_DONNEES: AUTOM_TRAME_CONFIG;
			if(attendu == AUTOM_TRAME_DONNEES) AutomValeursLues = 0;
			do {
				if((t = read(chemin,lue,MAXTRAME)) <= 0) { if(t < 0) rc = 0; break; }
				lue[t] = '\0';
				if(lue[t-1] == 0x0A) lue[--t] = '\0';
				if(lue[t-1] == 0x0D) lue[--t] = '\0';
				if(AutomTrameFile) fputs(lue,AutomTrameFile);
				// printf("(%s) Ligne lue: [%s]\n",__func__,lue);
				c = lue; ligne = 0;
				while(*c) {
					if((*c == '{') || (*c == '}') || (*c == 0x0D) || (*c == 0x0A)) { c++; continue; }
					nom = ItemJusquA(':',&c);
					d = c; while(*++c) {
						if((*c == '}') || (*c == 0x0D) || (*c == 0x0A)) break;
					}
					*c = '\0';
					// printf("(%s) Nom decode: [%s]\n",__func__,nom);
					n = strlen(nom) - 4;
					def = ((n >= 0) && !strcmp(nom+n,"/key"));
					var = &(AutomVar[AutomVarNb]);
					if(def) {
						if((attendu == AUTOM_TRAME_CONFIG) && (AutomVarNb < AUTOM_VAR_MAX)) {
							*(nom+n) = '\0';
							var->code = AUTOM_FLOAT;
							bzero(&AutomMidasDef,sizeof(AutomMidasDef)); AutomMidasDef.nb = 1;
							ItemJusquA('{',&d); ArgSplit(d,',',AutomMidasKeyDesc);
							strncpy(var->nom,nom,MAXVARNOM);
							var->etat = AUTOM_FREE;
							var->contenu = AUTOM_AO;
							var->cles[0] = '\0';
							switch(AutomMidasDef.type) {
							  case  7: var->code = AUTOM_INT; break;
							  case  8: var->code = AUTOM_BOOL; break;
							  case  9: var->code = AUTOM_FLOAT; break;
							  case 12: var->code = AUTOM_STRING; break;
							  default: break;
							}
						#ifdef VAR_VECTOR
							if(var->size != AutomMidasDef.nb) {
								if(var->tab) free(var->tab);
								if(AutomMidasDef.nb > 1) {
									var->tab = (TypeAutomVal *)malloc(AutomMidasDef.nb * sizeof(TypeAutomVal));
									if(!var->tab) AutomMidasDef.nb = 1;
								}
							}
						#endif
							var->size = AutomMidasDef.nb;
							var->index = 0;
							var->ref = AutomVarNb;
							AutomVarRaz(var,-1,AUTOM_DO);
							n = AutomVarNb;
							AutomVarNb++;
							if(AutomVar[n].size > 1) {
								for(k=0; k<AutomVar[n].size; k++) if(AutomVarNb < AUTOM_VAR_MAX) {
									var = &(AutomVar[AutomVarNb]);
									memcpy(var,&(AutomVar[n]),sizeof(TypeAutomVar));
									snprintf(var->nom,MAXVARNOM,"%s.%d",AutomVar[n].nom,k);
									var->size = 1;
									var->index = k;
									var->ref = n;
									AutomVarRaz(var,-1,AUTOM_DO);
									AutomVarNb++;
								}
							}
						}
						ligne++;
					} else if(nom[0] != '/') {
						n = ligne / 2;
						if(strcmp(nom,AutomVar[n].nom)) for(n=0; n<AutomVarNb; n++) {
							if(!strcmp(nom,AutomVar[n].nom)) break;
						}
						// printf("(%s) Valeur a lire, variable #%d/%d\n",__func__,n,AutomVarNb);
						if(n < AutomVarNb) {
							char verifiee; short ref;
							lngr = strlen(AutomVar[n].nom); t = AutomVar[n].size; ref = n;
							if(t > 1) { ItemJusquA('[',&d); n++; k = 0; }
							else verifiee = 1;
							while(*d && t) {
								valtxt = ItemAvant(",]",&sep,&d);
								var = &(AutomVar[n]);
								if(t > 1) verifiee = ((var->ref == ref) && (var->index == k));
								if(verifiee) switch(var->code) {
								  case AUTOM_INT: sscanf(valtxt,"%d",&(var->val.i)); break;
								  case AUTOM_BOOL: sscanf(valtxt,"%hhd",&(var->val.b)); break;
								  case AUTOM_FLOAT: sscanf(valtxt,"%g",&(var->val.r)); break;
								  case AUTOM_STRING: strncpy(var->val.s,valtxt,MAXVALSTR); break;
								}
								// char val[80]; AutomTraduitVar(n,val,80); printf("(%s) %s = %s\n",__func__,AutomVar[n].nom,val);
								--t; if(t > 0) { n++; k++; } else break;
								if(sep == ']') break;
							}
							AutomValeursLues++;
						}
						ligne++;
					}
					while(*++c) {
						if((*c == 0x0D) || (*c == 0x0A)) break;
					}
				}
			} while(1);
			/* for(n=0; n<AutomVarNb; n++) {
				char val[80];
				AutomTraduitVar(n,val,80); printf("(%s) variable de support %s = %s\n",__func__,AutomVar[n].nom,val);
			} */
			AutomDecritValeurs();
			// ArgPrint("*",AutomValeursDesc);
			break;
		}
	} while(--tentative && !lngr && (attendu != AUTOM_NBTYPES_TRAME));
	if(tentative <= 0) {
		if(AutomSrceFmt == AUTOM_EDW) {
			if(t > 16) t = 16;
			lue[t] = '\0';
			if(t) sprintf(raison,"Impossible de lire un debut de trame automate (lu: '%s etc...')",lue);
			else sprintf(raison,"Reponse automate absente ou vide");
		} else sprintf(raison,"Reponse automate absente ou vide");
		rc = 0;
	}

/*
 * Deconnexion
 */
laferme:
//	printf("(%s:%d) AutomLink = %d\n",__func__,__LINE__,AutomLink);
#ifndef CODE_WARRIOR_VSN
    if((AutomLink >= 0) && (AutomLink != AUTOM_NOLINK) && !AutomPermanent) {
		if(AutomSrceType == AUTOM_FICHIER) close(AutomLink);
		else SocketFermee(AutomLink);
		AutomLink = -1;
		// printf("(%s) Deconnexion\n",__func__);
	}
#endif
	if(attendu == AUTOM_TRAME_DONNEES) {
		for(j=0; j<AutomUserNb; j++) AutomUserFromVar(j);
		for(j=0; j<AutomSrceNb; j++) AutomSrceFromVar(j);
	}
//	printf("(%s:%d) AutomLink = %d\n",__func__,__LINE__,AutomLink);

	return(rc);
}
/* ========================================================================== */
static int Autom1Echange(char *commande, int indx, char *raison) {
	/* Autom1Echange(0,0,0.0,raison) partout remplace par AutomDataGet(raison) */
	int attendu;

	if((attendu = Autom1Ecriture(commande,indx,raison)) < 0) return(0);
	return(Autom1Lecture(attendu,indx,raison));
}
/* ========================================================================== */
char AutomDataQuery(char *raison) { return(Autom1Ecriture(AUTOM_READ,-1,raison)); }
/* ========================================================================== */
char AutomDataGet(char *raison) { return(Autom1Lecture(AUTOM_TRAME_DONNEES,-1,raison)); }
/* ========================================================================== */
static int AutomRefresh(Menu menu, MenuItem item) {
	char raison[256];

	if(AutomDataQuery(raison) >= 0) {
		if(AutomDataGet(raison)) { if(OpiumDisplayed(bAutomate)) OpiumRefresh(bAutomate); }
		else printf("%s/ Impossible de lire les informations du support (%s)\n",DateHeure(),raison);
	} else printf("%s/ Impossible de demander les informations du support (%s)\n",DateHeure(),raison);
	return(0);
}
/* ========================================================================== */
static void AutomTraduitVar(int num, char *texte, int max) {
	TypeAutomVar *var;

	strcpy(texte,"change");
	var = &(AutomVar[num]);
	switch(var->code) {
		case AUTOM_BOOL:  strncpy(texte,(var->val.b)? "oui": "non",max); break;
		case AUTOM_CLE:   ArgKeyGetText(var->cles,var->val.b,texte,max); break;
		case AUTOM_INT:   snprintf(texte,max,"%d",var->val.i); break;
		case AUTOM_FLOAT: snprintf(texte,max,"%g",var->val.r); break;
	}
}
/* ========================================================================== */
char AutomEcritVar(int num, char *raison) {
	if(Autom1Echange(AUTOM_WRITE,num,raison)) {
		char texte[80];
		AutomTraduitVar(num,texte,80);
		printf("%s/ Support: %s <- %s\n",DateHeure(),AutomVar[num].nom,texte);
		return(1);
	} else {
		printf("%s! Support: modification de %s impossible (%s)\n",DateHeure(),AutomVar[num].nom,raison);
		return(0);
	}
}
/* ========================================================================== */
int AutomUserIndex(char *nom) {
	int i;

	for(i=0; i<AutomUserNb; i++) if(!strcmp(nom,AutomVar[AutomUserVar[i].num].nom)) break;
	if(i < AutomUserNb) return(i); else return(-1);
}
/* ========================================================================== */
char AutomEcritUser(int j, char *raison) {
	TypeAutomUserVar *user; TypeAutomVar *var; TypeAutomVal *val;

	user = &(AutomUserVar[j]); var = &(AutomVar[user->num]);
#ifdef VAR_VECTOR
	if(var->size < 2) val = &(var->val);
	else {
		short k;
		k = user->index;
		if((k < 0) || (k >= var->size)) return;
		val = &(var->tab[k]);
	}
#else
	val = &(var->val);
#endif
	switch(user->type) {
	  case AUTOM_BOOL:
	  case AUTOM_CLE:
		switch(var->code) {
		  case AUTOM_BOOL:
		  case AUTOM_CLE:   val->b = user->val.b; break;
		  case AUTOM_INT:   val->i = (int)user->val.b; break;
		  case AUTOM_FLOAT: val->r = (float)user->val.b; break;
		}
		break;
	  case AUTOM_INT:
		switch(var->code) {
		  case AUTOM_BOOL:  val->b = (user->val.i > 0); break;
		  case AUTOM_CLE:   val->b = (char)(user->val.i & 0x7F); break;
		  case AUTOM_INT:   val->i = user->val.i; break;
		  case AUTOM_FLOAT: val->r = (float)user->val.i; break;
		}
		break;
	  case AUTOM_FLOAT:
		switch(var->code) {
		  case AUTOM_BOOL:  val->b = (user->val.r > 0.0); break;
		  case AUTOM_CLE:   val->b = (char)((int)(user->val.r) & 0x7F); break;
		  case AUTOM_INT:   val->i = (int)user->val.r; break;
		  case AUTOM_FLOAT: val->r = user->val.r; break;
		}
		break;
	}
	return(AutomEcritVar(user->num,raison));

}
/* ========================================================================== */
char AutomEcritSrce(int j, char *raison) {
	TypeAutomSrceVar *srce; TypeAutomVar *var; TypeAutomVal *val;

	srce = &(AutomSrceVar[j]); var = &(AutomVar[srce->num]);
#ifdef VAR_VECTOR
	if(var->size < 2) val = &(var->val);
	else {
		short k;
		k = user->index;
		if((k < 0) || (k >= var->size)) return;
		val = &(var->tab[k]);
	}
#else
	val = &(var->val);
#endif
	switch(var->code) {
	  case AUTOM_BOOL:
	  case AUTOM_CLE:   val->b = srce->active; break;
	  case AUTOM_INT:   val->i = (int)srce->active; break;
	  case AUTOM_FLOAT: val->r = (float)srce->active; break;
	}
	return(AutomEcritVar(srce->num,raison));

}
/* ========================================================================== */
int AutomCreeTrame(Menu menu, MenuItem item) {
	char raison[80];

	if(AutomLink >= 0) {
		if(OpiumReadFile("Sur quel fichier",AutomSrceFic,32) == PNL_CANCEL) return(0);
		RepertoireIdentique(FichierPrefAutom,AutomSrceFic,AutomSrcePath);
		if((AutomTrameFile = fopen(AutomSrcePath,"w"))) {
			Autom1Echange(AUTOM_CONFG,-1,raison);
			fprintf(AutomTrameFile,"+\n");
			AutomDataQuery(raison);
			AutomDataGet(raison);
			fclose(AutomTrameFile);
			AutomTrameFile = 0;
		} else OpiumFail("Fichier %s inutilisable (%s)",FichierPrefAutom,strerror(errno));
	} else OpiumWarn("Pas de liaison serveur a lire: creation impossible");
	return(0);
}
/* ========================================================================== */
int AutomCommande(Menu menu, MenuItem item) {
	char raison[80],texte[80]; char rc; int l;

	if(AccesRestreint()) return(0);
	if(OpiumReadList ("Variable a modifier",AutomListe,&AutomNum,MAXVARNOM) == PNL_CANCEL) return(0);
	if(AutomVar[AutomNum].etat != AUTOM_FREE) {
		OpiumFail("Cette variable est '%s', donc non modifiable",AutomEtatNom[AutomVar[AutomNum].etat]);
		return(0);
	}
	rc = PNL_CANCEL;
	/* if(AutomVar[AutomNum].unite[0]) sprintf(texte,"Nouvelle valeur (%s)",AutomVar[AutomNum].unite);
	else */ strcpy(texte,"Nouvelle valeur");
	switch(AutomVar[AutomNum].code) {
	  case AUTOM_BOOL:  rc = OpiumReadBool(texte,&(AutomVar[AutomNum].val.b)); break;
	  case AUTOM_CLE:   l = ArgKeyGetLngr(AutomVar[AutomNum].cles) + 1;
		                rc = OpiumReadKeyB(texte,AutomVar[AutomNum].cles,&(AutomVar[AutomNum].val.b),l);
		                break;
	  case AUTOM_INT:   rc = OpiumReadInt(texte,&(AutomVar[AutomNum].val.i)); break;
	  case AUTOM_FLOAT: rc = OpiumReadFloat(texte,&(AutomVar[AutomNum].val.r)); break;
	}
	if(rc == PNL_OK) {
		char texte[80];
		AutomTraduitVar(AutomNum,texte,80);
		AutomEcritVar(AutomNum,raison);
		if(OpiumDisplayed(bAutomate)) OpiumRefresh(bAutomate);
	}
	return(0);
}
/* ========================================================================== */
Menu mAutomRefresh;
TypeMenuItem iAutomRefresh[] = { { "Rafraichir", MNU_FONCTION AutomRefresh }, MNU_END };
/* ========================================================================== */
static void AutomConstruit() {
	int j,k,l,xc,yc,larg,larg_max,ymin,ylim;
	int xp,yp,yq,dx,col,naff;
	int xcolonne,ycolonne,xcadrant,ycadrant,xinstrum,yinstrum;
	TypeAutomUserVar *user; TypeAutomVar *var; char *nom;
	Panel panel; Instrum instrum; INS_FORME forme; void *appareil;

	if(CompteRendu.vars_manip) printf("  . Construction de la planche de bord pour le support\n");
	BoardTrash(bAutomate);
	BoardAddMenu(bAutomate,mAutomRefresh = MenuLocalise(iAutomRefresh),Dx,Dy,0);
	OpiumSupport(mAutomRefresh->cdr,WND_PLAQUETTE);
	xc = Dx; ymin = 3 * Dy;
	yp = 2 * Dy;
	xcolonne = Colonne3Quarts.largeur + Colonne3Quarts.graduation + (Colonne3Quarts.nbchars * Dx);
	ycolonne = Colonne3Quarts.longueur + Dy;
	xcadrant = (int)(1.5 * Cadrant2.rayon) + (3 * Dx);
	ycadrant = (int)(Cadrant2.rayon + Cadrant2.graduation) + Dy;
	yq = (ycolonne > ycadrant)? ycolonne: ycadrant;
	yq += (2 * Dy);
	larg_max = WndCurSvr->larg; ylim = ymin + yq;

	naff = 0;
	yc = ymin; col = 0;
	for(j=0; j<AutomUserNb; j++) if(AutomUserVar[j].affiche) {
		user = &(AutomUserVar[j]); var = &(AutomVar[user->num]);
		nom = user->libelle; // var->nom;
		larg = strlen(nom) + 3;
		panel = PanelCreate(1); k = -1;
		switch(user->type) {
		  case AUTOM_BOOL:
			k = PanelBool(panel,nom,&(user->val.b)); PanelItemColors(panel,k,AutomFonds,2); // larg += 3;
			break;
		  case AUTOM_INT:
			k = PanelInt(panel,nom,&(user->val.i)); // larg += 11;
			if(user->min.i != user->max.i) {
				PanelItemILimits(panel,k,user->min.i,user->max.i);
				l = user->max.i - user->min.i + 1; if(l > AUTOM_MAX_FONDS) l = AUTOM_MAX_FONDS;
				PanelItemColors(panel,k,AutomFonds,l);
			}
			break;
		  case AUTOM_FLOAT:
			k = PanelFloat(panel,nom,&(user->val.r)); // larg += 12;
			if(user->min.r < user->max.r) {
				PanelItemRLimits(panel,k,user->min.i,user->max.i);
				PanelItemColors(panel,k,AutomFonds,AUTOM_MAX_FONDS);
			}
			break;
		  case AUTOM_CLE:
			l = ArgKeyGetLngr(user->cles) + 1;
			k = PanelKeyB(panel,nom,user->cles,&(user->val.b),l); // larg += l;
			l = ArgKeyGetNb(user->cles); if(l > AUTOM_MAX_FONDS) l = AUTOM_MAX_FONDS;
			PanelItemColors(panel,k,AutomFonds,l);
			break;
		}
		if(k > 0) {
			PanelItemLngr(panel,k,6); larg += 6;
			if(var->etat != AUTOM_FREE) PanelItemSelect(panel,k,0);
			PanelItemSouligne(panel,k,0); PanelSupport(panel,WND_CREUX); PanelMode(panel,PNL_DIRECT);
		} else { PanelDelete(panel); panel = 0; }
		user->panel = panel; appareil = 0;
		switch(user->affiche) {
			case AUTOM_COLONNE: forme = INS_COLONNE; appareil = (void *)&Colonne3Quarts; xinstrum = xcolonne; yinstrum = ycolonne; break;
			case AUTOM_CADRANT: forme = INS_CADRAN; appareil = (void *)&Cadrant2; xinstrum = xcadrant; yinstrum = ycadrant; break;
			default: break;
		}
		xp = larg * Dx;
		if(appareil) { if(xp < xinstrum) xp = xinstrum; }
		if(user->affiche == AUTOM_VOYANT) {
			if(panel) {
				if((yc + yp) > ylim) {
					  xc += col; col = 0;
					  if(xc > larg_max) { xc = Dx; ymin += yp; if(ylim < (ymin + yp)) ylim = ymin + yp; }
					  yc = ymin;
				}
				//- dx = xp - (7 * Dx); if(dx < 0) dx = 0;
				dx = (col - (larg * Dx)) / 2; if(dx < 0) dx = 0;
				BoardAddPanel(bAutomate,panel,xc + dx,yc,0);
				naff++;
				yc += yp;
			}
		} else if(appareil) {
			if((yc + yq) > ylim) {
				xc += col;
				if(xc > larg_max) { xc = Dx; ymin += yq; if(ylim < (ymin + yq)) ylim = ymin + yq; }
				yc = ymin;
			}
			instrum = 0;
			switch(user->type) {
			  case AUTOM_BOOL: instrum = InstrumBool(forme,appareil,&(user->val.b)); break;
			  case AUTOM_INT: instrum = InstrumInt(forme,appareil,&(user->val.i),user->min.i,user->max.i); break;
			  case AUTOM_FLOAT: instrum = InstrumFloat(forme,appareil,&(user->val.r),user->min.r,user->max.r); break;
			  case AUTOM_CLE: instrum = InstrumKeyB(forme,appareil,&(user->val.b),user->cles); break;
			}
			if(instrum) {
				dx = ((larg * Dx) - xinstrum) / 2; if(dx < 0) dx = 0;
				InstrumSupport(instrum,WND_CREUX); BoardAddInstrum(bAutomate,instrum,xc + dx,yc,0);
			}
			user->instrum = instrum;
			if(panel) {
				yc += yinstrum;
				dx = (xinstrum - (larg * Dx)) / 2; if(dx < 0) dx = 0;
				BoardAddPanel(bAutomate,panel,xc + dx,yc,0);
				yc += (2 * Dy);
			}
			if(panel || instrum) naff++;
		}
		if(col < xp) col = xp;
	}
	if(CompteRendu.vars_manip) printf("    : %d variable%s affichee%s\n",Accord2s(naff));
}
/* ========================================================================== */
int AutomChoisi(Menu menu, MenuItem item) {
	int i,j,k,n; char modif,affiche; Panel p;

	if(OpiumExec(pAutomUtilise->cdr) == PNL_CANCEL) return(0);
	modif = 0;
	for(j=0; j<AutomUserNb; j++) {
		i = AutomUserVar[j].num;
		if(!(AutomVar[i].utilisee)) {
			AutomVar[i].usr = -1;
			AutomUserNb--;
			for(k=j; k<AutomUserNb; k++) memcpy(&(AutomUserVar[k]),&(AutomUserVar[k+1]),sizeof(TypeAutomUserVar));
			--j;
			for(i=0; i<AutomVarNb; i++)  if(AutomVar[i].usr > j) AutomVar[i].usr -= 1;
			modif = 1;
		}
	}
	for(i=0; i<AutomVarNb; i++) if(AutomVar[i].utilisee && (AutomVar[i].usr == -1)) {
		j = AutomUserNb;
		AutomVar[i].usr = j;
		AutomUserVar[j].num = i;
		strcpy(AutomUserVar[j].libelle,AutomVar[i].nom);
		AutomUserVar[j].type = AutomVar[i].code;
		switch(AutomUserVar[j].type) {
		  case AUTOM_BOOL:  AutomUserVar[j].val.b =  0; break;
		  case AUTOM_CLE:   AutomUserVar[j].val.b =  0; strcpy(AutomUserVar[j].cles,"non/oui"); break;
		  case AUTOM_INT:   AutomUserVar[j].val.i =  0; AutomUserVar[j].min.i = 0; AutomUserVar[j].max.i = 10; break;
		  case AUTOM_FLOAT: AutomUserVar[j].val.r =  0.0; AutomUserVar[j].min.r = 0.0; AutomUserVar[j].max.r = 1.0; break;
		}
		AutomUserVar[j].archive = AutomUserVar[j].affiche = NEANT;
		AutomUserVar[j].instrum = 0; AutomUserVar[j].panel = 0;
		AutomUserNb++;
		modif = 1;
	}

	n = 6;
	p = PanelCreate((1 + AutomUserNb) * n); PanelColumns(p,n); PanelMode(p,PNL_BYLINES);
	PanelItemSelect(p,PanelText(p,"Variable","User Name",AUTOM_USER_LIBELLE),0);
	PanelItemSelect(p,PanelText(p,0,"type",6),0);
	PanelItemSelect(p,PanelText(p,0,"affichee",9),0);
	PanelItemSelect(p,PanelText(p,0,"min",6),0);
	PanelItemSelect(p,PanelText(p,0,"max",6),0);
	PanelItemSelect(p,PanelText(p,0,"archivee",9),0);
	for(j=0; j<AutomUserNb; j++) {
		i = AutomUserVar[j].num;
		PanelText(p,AutomVar[i].nom,AutomUserVar[j].libelle,AUTOM_USER_LIBELLE);
		PanelKeyB(p,0,AUTOM_CODE_CLES,&(AutomUserVar[j].type),6);
		PanelKeyB(p,0,AUTOM_AFF_CLES,&(AutomUserVar[j].affiche),8);
		switch(AutomUserVar[j].type) {
		  case AUTOM_BOOL:  PanelBlank(p); PanelBlank(p); break;
		  case AUTOM_CLE:   PanelBlank(p); PanelText(p,0,AutomUserVar[j].cles,AUTOM_CLES_LNGR); break;
		  case AUTOM_INT:   PanelInt(p,0,&(AutomUserVar[j].min.i)); PanelInt(p,0,&(AutomUserVar[j].max.i)); break;
		  case AUTOM_FLOAT: PanelFloat(p,0,&(AutomUserVar[j].min.r)); PanelFloat(p,0,&(AutomUserVar[j].max.r)); break;
		}
		PanelBool(p,0,&(AutomUserVar[j].archive));
	}
	if(OpiumExec(p->cdr) == PNL_OK) modif = 1;
	if(modif) {
		for(j=0; j<AutomUserNb; j++) AutomUserFromVar(j);
		if((affiche = OpiumDisplayed(bAutomate))) OpiumClear(bAutomate);
		AutomConstruit();
		if(affiche) OpiumDisplay(bAutomate);
		AutomDecritArch();
	}
	PanelDelete(p);
	AutomASauver = 1;

	return(0);
}
#ifdef EDW_ORIGINE
/* ========================================================================== */
int AutomEdwDeplace(char descendre, char avec_source, char *prefixe) {
	int i,j; char active,etat; char raison[80];
	
	if(descendre) {
		if(!AutomDataQuery(raison) || !AutomDataGet(raison)) {
			if(prefixe) { if(*prefixe) printf("%s "); else printf("%s/ ",DateHeure()); } 
			printf("%s: %s\n",__func__,raison);
		}
		etat = 0;
		for(j=0; j<AUTOM_REGEN_MAX; j++) {
			i = AutomSrceRegenIndex[j];
			active = AutomVar[i].val.b;
			if(AutomSrceAccede && avec_source && (i >= 0) && !active) {
				AutomVar[i].val.b = 1;
				if(Autom1Echange(AUTOM_WRITE,i,raison)) {
					if(prefixe) { if(*prefixe) printf("%s "); else printf("%s/ ",DateHeure()); } 
					printf("%s mise en place\n",AutomNom(i));
					active = 1;
				} else if(prefixe) {
					if(*prefixe) printf("%s "); else printf("%s/ ",DateHeure());
					printf("Deplacement de %s impossible (%s)",AutomNom(i),raison);
				}
			} else {
				if(prefixe) { if(*prefixe) printf("%s "); else printf("%s/ ",DateHeure()); } 
				printf("Source %s inchangee (%s)\n",AutomRegenNom[j],active? "en place": "retiree");
			}
			etat |= active;
		}
	} else {
		// AutomDataQuery(raison); AutomDataGet();
		etat = 0;
		for(j=0; j<AUTOM_REGEN_MAX; j++) {
			i = AutomSrceRegenIndex[j];
			active = AutomVar[i].val.b;
			if(AutomSrceAccede && (i >= 0) && active) {
				AutomVar[i].val.b = 0;
				if(Autom1Echange(AUTOM_WRITE,i,raison)) {
					if(prefixe) { if(*prefixe) printf("%s "); else printf("%s/ ",DateHeure()); }
					printf("%s retiree (a suivre: attente de 3 secondes)\n",AutomNom(i));
					active = 0;
					SambaAttends(3000); /* le temps que la source remonte */
				} else if(prefixe) {
					if(*prefixe) printf("%s "); else printf("%s/ ",DateHeure());
					printf("Deplacement de %s impossible (%s)",AutomNom(i),raison);
				}
			} else {
				if(prefixe) { if(*prefixe) printf("%s "); else printf("%s/ ",DateHeure()); } 
				printf("Source %s inchangee (%s)\n",AutomRegenNom[j],active? "en place": "retiree");
			}
			etat |= active;
		}
	}
	return(etat);
}
#endif /* EDW_ORIGINE */
/* ========================================================================== */
char AutomSourceDeplace(char role, char descendre, char avec_source, char *prefixe) {
	int i,j; char active,etat; char raison[80];

	if(!AutomDataQuery(raison) || !AutomDataGet(raison)) {
		if(prefixe) { if(*prefixe) printf("%s "); else printf("%s/ ",DateHeure()); }
		printf("%s: %s\n",__func__,raison);
	}
	if(descendre) {
		etat = 0;
		for(j=0; j<AutomSrceNb; j++) if(AutomSrceVar[j].role == role) {
			i = AutomSrceVar[j].num;
			active = AutomSrceVar[j].active;
			if(prefixe) { if(*prefixe) printf("%s "); else printf("%s/ ",DateHeure()); }
			if(AutomSrceAccede && avec_source && (i >= 0) && !active) {
				AutomSrceVar[j].active = 1;
				if(AutomEcritSrce(j,raison)) {
					active = 1;
					printf("%s mise en place\n",AutomNom(i));
				} else printf("! Deplacement de %s impossible (%s)",AutomNom(i),raison);
			} else printf("! Source de %s %s inchangee (%s)\n",AutomSrceRole[role],AutomNom(i),active? "en place": "retiree");
			etat |= active;
		}
	} else {
		// AutomDataQuery(raison); AutomDataGet();
		etat = 0;
		for(j=0; j<AutomSrceNb; j++) if(AutomSrceVar[j].role == role) {
			i = AutomSrceVar[j].num;
			active = AutomSrceVar[j].active;
			if(prefixe) { if(*prefixe) printf("%s "); else printf("%s/ ",DateHeure()); }
			if(AutomSrceAccede && (i >= 0) && active) {
				AutomSrceVar[j].active = 0;
				if(AutomEcritSrce(j,raison)) {
					active = 0;
					printf("%s retiree (a suivre: attente de 3 secondes)\n",AutomNom(i));
					SambaAttends(3000); /* le temps que la source remonte */
				} else printf("! Deplacement de %s impossible (%s)",AutomNom(i),raison);
			} else printf("! Source %s %s inchangee (%s)\n",AutomSrceRole[role],AutomNom(i),active? "en place": "retiree");
			etat |= active;
		}
	}
	return(etat);
}
/* ========================================================================== */
static void AutomImprimeCondition(char *prefixe, char point, int k, char *texte) {
	int l;
	
	if(k > 0) l = printf("| "); else l = printf("%s%c ",prefixe,point);
	l += printf("%s",texte);
	if(k > 0) SambaCompleteLigne(k,l); else printf("\n");
}
/* ========================================================================== */
void AutomVerifieConditions(char *prefixe, int k) {
	char texte[256],raison[256];
	char en_place[AUTOM_NBROLES];
	
	if(AutomSrceType == AUTOM_FICHIER) {
		sprintf(texte,"L'information sur l'environnement est passee via un fichier");
		AutomImprimeCondition(prefixe,'*',k,texte);
	} else if(AutomLink == AUTOM_NOLINK) {
		sprintf(texte,"Pas d'information integree sur l'environnement");
		AutomImprimeCondition(prefixe,'*',k,texte);
	} else {
		AutomDataQuery(raison);
		AutomDataGet(raison);
		sprintf(texte,"Information d'environnement integree (lue a %s):",DateHeure());
		AutomImprimeCondition(prefixe,'*',k,texte);
		if(AutomSrceFmt == AUTOM_EDW) {
			int i,j;

			ReglTbolo = *T_Bolo; EtatTubes = *PulseTubes;
			sprintf(texte,". Temperature des detecteurs: %8.5fK",ReglTbolo);
			AutomImprimeCondition(prefixe,' ',k,texte);
			sprintf(texte,". Tubes pulses %s",EtatTubes? "actifs": "a l'arret");
			AutomImprimeCondition(prefixe,' ',k,texte);

			for(i=0; i<AUTOM_NBROLES; i++) en_place[i] = 0;
			for(j=0; j<AutomSrceNb; j++) {
				sprintf(texte,". Source de %s %s %s",AutomSrceRole[AutomSrceVar[j].role],AutomNom(AutomSrceVar[j].num),
					AutomSrceVar[j].active? "en place":"retiree");
				AutomImprimeCondition(prefixe,' ',k,texte);
				if(AutomSrceVar[j].active) en_place[AutomSrceVar[j].role] = 1;
			}
			for(i=0; i<AUTOM_NBROLES; i++) {
				sprintf(texte,"  = %s %s %sen cours\n",(en_place[i])? "Une": "Pas de",AutomSrceRole[i],(en_place[i])? "est ": "");
				AutomImprimeCondition(prefixe,' ',k,texte);
			}
#ifdef EDW_ORIGINE
			EtatRegen = 0; for(i=0; i<AUTOM_REGEN_MAX; i++) {
				if((EtatSrceRegen[i] = *(AutomSrceRegenAdrs[i]))) EtatRegen = 1;
				sprintf(texte,". Source de regeneration (60Co) %s %s",AutomRegenNom[i],EtatSrceRegen[i]? "en place":"retiree");
				AutomImprimeCondition(prefixe,' ',k,texte);
			}

			EtatCalib = 0; for(i=0; i<AUTOM_CALIB_MAX; i++) {
				if((EtatSrceCalib[i] = *(AutomSrceCalibAdrs[i]))) EtatCalib = 1;
				sprintf(texte,". Source de calibration (133Ba) %s %s",AutomCalibNom[i],EtatSrceCalib[i]? "en place":"retiree");
				AutomImprimeCondition(prefixe,' ',k,texte);
			}

			if(!EtatRegen && !EtatCalib) {
				sprintf(texte,". Pas de source en place");
				AutomImprimeCondition(prefixe,' ',k,texte);
			}
#endif /* EDW_ORIGINE */
		}
	}
}
/* ========================================================================== */
static TypeMenuItem iGestAutom[] = {
	{ "Envoyer commande",        MNU_FONCTION AutomCommande  },
	{ "Selection de variables",  MNU_FONCTION AutomChoisi    },
	{ "Affichage selection",     MNU_FORK   &bAutomate       },
	{ "Creation fichier trames", MNU_FONCTION AutomCreeTrame },
	{ "Sauver prefs",            MNU_FONCTION AutomSauve     },
	{ "Fermer",                  MNU_RETOUR                  },
	MNU_END
};

/* ========================================================================== */
int AutomInit() {
    FILE *fichier;
	char ligne[256],*r,*item; char erreur;
	char fmt_v1;
	char raison[256]; int i,j,rc;
	char encodage[16];
	char nom_complet[MAXFILENAME];
	char fichier_defs[MAXFILENAME],fichier_vals[MAXFILENAME];
#ifndef CODE_WARRIOR_VSN
	char hote[256];
#endif

	AutomASauver = 0;
	for(i=0; i<AUTOM_VAR_MAX; i++) AutomListe[i] = AutomVar[i].nom; AutomListe[0][0] = '\0';
	for(i=0; i<AUTOM_VAR_MAX; i++) {
		AutomVar[i].size = 1;
		AutomVar[i].utilisee = 0; AutomVar[i].usr = -1;
	#ifdef VAR_VECTOR
		AutomVar[i].tab = 0;
	#endif
	}
	AutomUserVarLue.index = AutomSrceVarLue.index = 0;
	//- initialise dans SettingsInit(): for(i=0; i<AUTOM_REGEN_MAX; i++) AutomRegenNom[i][0] = '\0';
	AutomTrameFile = 0;

	AutomPrefsVsn = 2;
	AutomSrceType = NEANT;
	AutomSrceFmt = AUTOM_EDW;
	strcpy(AutomSrceFic,"Support/defs");
	strcpy(AutomUserFic,"Support/vars");
	strcpy(AutomUserVal,"Support/vals");
	strcpy(AutomSrceSvr,"192.168.3.13"); // 134.158.176.112
	AutomPort = 6342;
	AutomPermanent = 1;
	AutomValeursDesc = 0;
	AutomVarNb = 0;
	AutomUserNb = 0;
	AutomSrceNb = 0;
	AutomSrceAccede = 0;
	for(j=0; j<AUTOM_ARCH_MAX; j++) {
		AutomUserVar[j].num = -1; AutomUserVar[j].index = 0;
		AutomUserVar[j].libelle[0] = '\0';
		AutomUserVar[j].type = AUTOM_FLOAT; AutomUserVar[j].val.r =  0.0;
		strcpy(AutomUserVar[j].cles,"non/oui");
		AutomUserVar[j].archive = AutomUserVar[j].affiche = NEANT;
		AutomUserVar[j].min.r = AutomUserVar[j].max.r = 0.0;
		AutomUserVar[j].instrum = 0; AutomUserVar[j].panel = 0;
	}
	AutomNum = 0; AutomChoix = 0;
	AutomValeursLues = 0;
	mAutom = MenuLocalise(iGestAutom);

	fmt_v1 = 0;
    AutomLink = -1;
	strcpy(AutomHome,"(pas identifie)");
	rc = 0;
	if((FichierPrefAutom[0] == '\0') || !strcmp(FichierPrefAutom,"/neant")) {
		printf("= Pas de support declare (%s)\n",FichierPrefAutom);
		AutomLink = AUTOM_NOLINK;
	} else if((fichier = fopen(FichierPrefAutom,"r"))) {
		SambaLogDef("= Lecture du support","dans",FichierPrefAutom);
		erreur = NEANT;
		while(LigneSuivante(ligne,256,fichier)) {
			r = ligne;
			if((*r == '#') || (*r == '\n')) continue;
			if(*r == '@') {
				printf("  . Support accessible via IP [description v1]\n");
				fmt_v1 = 1;
				r++;
				item = ItemSuivant(&r,':');
				strncpy(AutomSrceSvr,item,MAXFILENAME);
				item = ItemSuivant(&r,' ');
				if(*item) sscanf(item,"%d",&AutomPort);
				AutomLink = SocketClient(AutomSrceSvr,AutomPort,&AutomSkt);
				if(AutomLink == -1) {
					printf("! Impossible de se connecter en TCP/IP sur %s, port %d\n",AutomSrceSvr,AutomPort);
					erreur = AUTOM_SERVEUR;
					break;
				}
				printf("  . Le support est controle via %s sur le port %d\n",
					PeerName(AutomLink,hote)? hote: AutomSrceSvr,AutomPort);
				if(!AutomPermanent) { SocketFermee(AutomLink); AutomLink = -1; }
				if(Autom1Echange(AUTOM_CONFG,-1,raison))
					LogBook("  . %s: Support %s:%d trouve, %d variables\n",DateHeure(),AutomSrceSvr,AutomPort,AutomVarNb);
				else LogBook("  . %s: Impossible de se connecter sur %s:%d (%s)\n",DateHeure(),AutomSrceSvr,AutomPort,raison);
			} else if(*r == '/') {
				fmt_v1 = 1;
				r++;
				strcat(strcpy(fichier_defs,PrefPath),ItemSuivant(&r,' '));
				strcat(strcpy(fichier_vals,PrefPath),ItemSuivant(&r,' '));
				printf("  . Support simule dans '%s' [description v1]\n",fichier_defs); fflush(stdout);
				AutomLink = open(fichier_defs,O_RDONLY,0);
				if(AutomLink == -1) {
					printf("  ! Impossible d'ouvrir ce fichier (%s)\n",strerror(errno));
					AutomLink = AUTOM_NOLINK;
					erreur = AUTOM_FICHIER;
					break;
				}
				AutomVarNb = 0; AutomListe[AutomVarNb][0] = '\0';
				AutomLink = -AutomLink;
				if(Autom1Echange(AUTOM_CONFG,-1,raison))
					LogBook("  . Description trouvee avec %d variables\n",AutomVarNb);
				close(-AutomLink);
				printf("  . Valeurs lues dans '%s'\n",fichier_vals); fflush(stdout);
				AutomLink = open(fichier_vals,O_RDONLY,0);
				if(AutomLink == -1) {
					printf("  ! Impossible d'ouvrir ce fichier (%s)\n",strerror(errno));
					AutomLink = AUTOM_NOLINK;
					erreur = AUTOM_FICHIER;
					break;
				}
				AutomLink = -AutomLink;
				if(AutomDataQuery(raison) && AutomDataGet(raison)) LogBook("  . Valeurs trouvees\n");
				close(-AutomLink);
				AutomLink = AUTOM_NOLINK;
			} else if(!fmt_v1) {
				if(CompteRendu.vars_manip) printf("  . Fichier de configuration v2\n");
				ArgFromFile(fichier,AutomDesc,0);
				//- ArgPrint("*",AutomDesc);
				if(AutomSrceType == NEANT) { printf("  * Support desactive\n"); AutomLink = AUTOM_NOLINK; }
				ArgKeyGetText(AUTOM_FMT_CLES,AutomSrceFmt,encodage,16);
				if(CompteRendu.vars_manip) printf("  . Format des informations: %s\n",encodage);
				if(AutomSrceType == AUTOM_SERVEUR) {
					printf("  . Noms des variables a trouver chez le serveur %s, port %d\n",AutomSrceSvr,AutomPort);
					if(CompteRendu.vars_manip) printf("  . %s: Connexion demandee\n",DateHeure());
					AutomLink = SocketClient(AutomSrceSvr,AutomPort,&AutomSkt);
					if(AutomLink == -1) {
						printf("  ! %s: Impossible de se connecter en TCP sur %s:%d\n",DateHeure(),AutomSrceSvr,AutomPort);
						erreur = AUTOM_SERVEUR;
						if(AutomPermanent) AutomSrceType = AUTOM_FICHIER; // on passe sur fichier, mais si on veut insister par la suite:
						else continue;
					} else {
						if(CompteRendu.vars_manip) printf("  . %s: Connexion obtenue, controle %s via %s:%d\n",DateHeure(),
							AutomPermanent? "en permanence": "au cas par cas",PeerName(AutomLink,hote)? hote: AutomSrceSvr,AutomPort);
						if(!AutomPermanent) { SocketFermee(AutomLink); AutomLink = -1; }
						if(Autom1Echange(AUTOM_CONFG,-1,raison))
							LogBook("  . %s: Serveur %s:%d lu, %d variable%s definie%s\n",DateHeure(),AutomSrceSvr,AutomPort,Accord2s(AutomVarNb));
						else {
							LogBook("  ! %s: Impossible de lire les donnees de %s:%d (%s)\n",DateHeure(),AutomSrceSvr,AutomPort,raison);
							if(AutomPermanent) AutomSrceType = AUTOM_FICHIER;
						}
					}
				}
				if(AutomSrceType == AUTOM_FICHIER) /* fichier */ {
					RepertoireIdentique(FichierPrefAutom,AutomSrceFic,AutomSrcePath);
					SambaLogDef(". Noms des variables a trouver","dans",AutomSrcePath);
					AutomLink = open(AutomSrcePath,O_RDONLY,0);
					if(AutomLink == -1) {
						printf("  ! Impossible d'ouvrir le fichier %s: %s\n",AutomSrcePath,strerror(errno));
						AutomLink = AUTOM_NOLINK;
						erreur = AUTOM_FICHIER;
						continue;
					}
					AutomLink = -AutomLink;
					if(Autom1Lecture(AUTOM_TRAME_CONFIG,-1,raison)) { if(CompteRendu.vars_manip) printf("  . %d variable%s definies\n",Accord1s(AutomVarNb)); }
					else printf("  ! Variables introuvables (%s)\n",raison);
					if(AutomValeursLues || Autom1Lecture(AUTOM_TRAME_DONNEES,-1,raison)) { if(CompteRendu.vars_manip) printf("  . Valeurs lues\n"); }
					else printf("  ! Variables non initialisees (%s)\n",raison);
					close(-AutomLink); AutomLink = -1;
					AutomPermanent = 0;
					if(AutomSrceFmt == AUTOM_JSON) RepertoireIdentique(FichierPrefAutom,AutomUserVal,AutomSrcePath);
				}
				if(AutomSrceType != NEANT) {
					if(AutomUserFic[0]) {
						RepertoireIdentique(FichierPrefAutom,AutomUserFic,nom_complet);
						SambaLogDef(". Variables utilisees cherchees","dans",nom_complet);
						ArgScan(nom_complet,AutomUserDesc);
						//- ArgPrint("*",AutomUserDesc);
					} else printf("  ! Fichier des variables utilisee non defini. On n'utilisera AUCUNE variable du support...\n");
				}
				break;
			}
		}
		fclose(fichier);
		if(erreur == AUTOM_SERVEUR) printf("  ! Liaison support avec %s inexploitable\n",AutomSrceSvr);
		else if(erreur == AUTOM_FICHIER) printf("  ! Liaison support, meme simulee, impossible\n");
		else rc = 1;
	} else {
		// printf("! Fichier pour l'appareillage illisible: '%s' (%s)\n",FichierPrefAutom,strerror(errno));
		printf("\n! Support illisible dans                    '%s' (%s)\n",FichierPrefAutom,strerror(errno));
		AutomLink = AUTOM_NOLINK;
		AutomExemple();
		AutomDecritValeurs();
		AutomSrceFmt = AUTOM_JSON;
		RepertoireIdentique(FichierPrefAutom,AutomSrceFic,AutomSrcePath);
		if((AutomTrameFile = fopen(AutomSrcePath,"w"))) {
			ARG_STYLES style_normal;
			if(AutomSrceFmt == AUTOM_JSON) style_normal = ArgStyle(ARG_STYLE_JSON);
			ArgBegin(AutomTrameFile,0,AutomVarsDesc);
			ArgAppend(AutomTrameFile,0,AutomValeursDesc);
			fclose(AutomTrameFile);
			if(AutomSrceFmt == AUTOM_JSON) ArgStyle(style_normal);
			AutomTrameFile = 0;
		}
		AutomSauve();
	}
	/* pour l'exemple:
	if(AutomUserNb == 0) for(AutomUserNb=0; AutomUserNb<AutomVarNb; AutomUserNb++) {
		if(AutomUserNb >= AUTOM_ARCH_MAX) break;
		AutomUserVar[AutomUserNb].num = AutomUserNb;
		AutomUserVar[AutomUserNb].tor = 0;
		switch(AutomVar[AutomUserNb].contenu) {
			case AUTOM_DO: case AUTOM_DO1: case AUTOM_DO2: case AUTOM_ONOFF: case AUTOM_DIGITALOUTPUT:
				AutomUserVar[AutomUserNb].tor = 1; break;
		}
		strncpy(AutomUserVar[AutomUserNb].libelle,AutomVar[AutomUserNb].nom,AUTOM_USER_LIBELLE);
	} */
		
	FlottantBidon = 0.0; CharBidon = 0;
	if(rc) {
		Panel p; int ligs,cols; int j;

		// if(CompteRendu.vars_manip) printf("  . Connexion des %d variable%s utilisateur\n",Accord1s(AutomUserNb));
		for(i=0; i<AUTOM_VAR_MAX; i++) { AutomVar[i].utilisee = 0; AutomVar[i].usr = -1; }
		for(j=0; j<AutomUserNb; j++) {
			AutomVar[AutomUserVar[j].num].utilisee = 1;
			AutomVar[AutomUserVar[j].num].usr = j;
		}
		for(j=0; j<AutomUserNb; j++) AutomUserFromVar(j);
		for(j=0; j<AutomSrceNb; j++) AutomSrceFromVar(j);

		if(CompteRendu.vars_manip) {
			printf("  . %d variable%s utilisee%s%s\n",Accord2s(AutomUserNb),AutomUserNb? ":": ".");
			for(j=0; j<AutomUserNb; j++) {
				int num; char texte[80];
				num = AutomUserVar[j].num;
				AutomTraduitVar(num,texte,80);
				printf("    | %s = %s %s [%s]\n",AutomUserVar[j].libelle,texte,AutomUserVar[j].unite,AutomVar[num].nom);
			}
			printf("  . %d source%s utilisee%s%s\n",Accord2s(AutomSrceNb),AutomSrceNb? ":": ".");
			for(j=0; j<AutomSrceNb; j++) {
				int num;
				num = AutomSrceVar[j].num;
				printf("    | %s (%s): %s [%s]\n",AutomSrceVar[j].libelle,AutomSrceRole[AutomSrceVar[j].role],AutomSrceVar[j].active?"active":"retiree",AutomVar[num].nom);
			}
		}

		if(!ModeBatch) {
			// printf("  . Creation du panel de selection, pour %d variable%s\n",Accord1s(AutomVarNb));
			ligs = OpiumServerHeigth(0) / WndLigToPix(1); cols = ((AutomVarNb - 1) / ligs) + 1;
			pAutomUtilise = p = PanelCreate(AutomVarNb); PanelColumns(p,cols);
			for(i=0; i<AutomVarNb; i++) PanelItemColors(p,PanelBool(p,AutomVar[i].nom,&(AutomVar[i].utilisee)),SambaRougeVert,2);
			PanelTitle(p,"Variables utilisables");
			bAutomate = BoardCreate();
			AutomConstruit();
		}
		AutomDecritArch();

	#ifdef EDW_ORIGINE
		for(i=0; i<AUTOM_REGEN_MAX; i++) AutomSrceRegenIndex[i] = AutomRang(AutomRegenNom[i]);
		for(i=0; i<AUTOM_CALIB_MAX; i++) AutomSrceCalibIndex[i] = AutomRang(AutomCalibNom[i]);
		if(CompteRendu.vars_manip) {
			int num;
			printf("  * Variables de support a archiver:\n");
			for(i=0; i<AutomUserNb; i++) {
				num = AutomUserVar[i].num;
				if(AutomAdrsFloat(num) == 0)
					printf("    ! %s (%s) inaccessible\n",AutomVar[num].nom,AutomUserVar[i].libelle);
				else printf("    | %s (%s)\n",AutomVar[num].nom,AutomUserVar[i].libelle);
			}
			printf("    = %d variable%s\n",Accord1s(i));
			printf("  * Variables de support utilisees en interne [AutomInterne]:\n");
			i = 0;
			while(AutomInterne[i].nom[0]) {
				printf("    | %s (%s)\n",AutomInterne[i].nom,AutomInterne[i].libelle);
				i++;
			}
			printf("    = %d variable%s\n",Accord1s(i));
		}
	#endif /* EDW_ORIGINE */
		i = 0;
		while(AutomInterne[i].nom[0]) {
			switch(AutomInterne[i].type) {
			  case AUTOM_BOOL: case AUTOM_CLE:
				if(AutomInterne[i].cle) {
					*(AutomInterne[i].cle) = AutomAdrsKey(AutomRang(AutomInterne[i].nom));
					if(*(AutomInterne[i].cle) == 0) {
						printf("    ! %s (%s) inconnue\n",AutomInterne[i].nom,AutomInterne[i].libelle);
						*(AutomInterne[i].cle) = &CharBidon;
					}
				} else printf("    ! %s (%s) mal definie dans Samba\n",AutomInterne[i].nom,AutomInterne[i].libelle);
				break;
			  case AUTOM_FLOAT:
				if(AutomInterne[i].reel) {
					*(AutomInterne[i].reel) = AutomAdrsFloat(AutomRang(AutomInterne[i].nom));
					if(*(AutomInterne[i].reel) == 0) {
						printf("    ! %s (%s) inconnue\n",AutomInterne[i].nom,AutomInterne[i].libelle);
						*(AutomInterne[i].reel) = &FlottantBidon;
					}
				} else printf("    ! %s (%s) mal definie dans Samba\n",AutomInterne[i].nom,AutomInterne[i].libelle);
				break;
			}
			i++;
		}
		AutomVerifieConditions("  ",0);
	}
	printf("  * %s: AutomLink = %d\n",__func__,AutomLink);
    return(1);
}
/* ========================================================================== */
int AutomSetup() {
    return(1);
}
/* ========================================================================== */
int AutomSauve() {
	char nom_complet[MAXFILENAME];

	ArgPrint(FichierPrefAutom,AutomDesc); 
	RepertoireIdentique(FichierPrefAutom,AutomUserFic,nom_complet);
	ArgPrint(nom_complet,AutomUserDesc);
	return(0); 
}
/* ========================================================================== */
int AutomExit() {
#ifndef CODE_WARRIOR_VSN
    if(AutomLink >= 0) { SocketFermee(AutomLink); AutomLink = -1; }
#endif
	return(SambaSauve(&AutomASauver,"preferences de l'automate",1,1,AutomSauve));
    return(0);
}
