#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h> /* pour juste toupper */
/* tout ca pour read, write et close */
//#ifdef __MWERKS__
//#include <Types.h>
//#include <string.h>
//#else
#include <sys/types.h>
#include <sys/uio.h>
#ifndef __INTEL__
	#ifndef WIN32
		#include <strings.h>  /* pour strlen qui rend size_t si intrinseque */
	#else  /* WIN32 */
		#include <string.h>
	#endif /* WIN32 */
#endif /* __INTEL__ */
//#endif
#include <stdarg.h>
// #include <varargs.h>

#define CLAPS_C

#include <decode.h>
#include <claps.h>

#define DESC_NOT_LAST(d) (d && (d->type >= 0))
#define DESC_LAST(d) (!d || (d->type < 0))

#ifndef ERROR
#define ERROR -1
#endif /* ERROR */
#define ICI __func__,__LINE__

#define MAXLIGNE 1024

#ifdef OS9
	#define DESC_APPLI _prgname()
	#define TAB '\x09'
#else /* !OS9 */
	extern char *__progname;
	#define DESC_APPLI __progname
	#define TAB 0x09
	#ifdef aix
		#undef DESC_NOT_LAST
		#undef DESC_LAST
		#define DESC_NOT_LAST(d) d->type != 0xFF
		#define DESC_LAST(d) d->type == 0xFF
	#endif /* aix */
#endif /* !OS9 */

typedef enum {
	DESC_MODE_SUIVANT = 0,
	DESC_MODE_SAUTE_BLOC,
} DESC_MODES;

/* conversion xml <-> claps:

<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist SYSTEM "file://localhost/System/Library/DTDs/PropertyList.dtd">
<plist version="0.9">
<dict>
	<key>Description</key>
	<string>Timbuktu Host</string>
	<key>Messages</key>
	<dict>
		<key>start</key>
		<string>Starting Timbuktu Host</string>
		<key>stop</key>
		<string>Stopping Timbuktu Host</string>
	</dict>
	<key>OrderPreference</key>
	<string>Late</string>
	<key>Provides</key>
	<array>
		<string>TimbuktuHost</string>
	</array>
	<key>Requires</key>
	<array>
		<string>Resolver</string>
		<string>Disks</string>
	</array>
</dict>
</plist>

<key> introduit un nom de variable, soit <key>:     [rien], </key>:     ' = '
<dict> introduit une structure,     soit <dict>:    '{',    </dict>:    '}'
<array> introduit un tableau,       soit <array>:   '(',    </array>:   ')'  [Attention a la virgule separative]
<string> introduit un texte,        soit <string>:  '"',    </string>:  '"'  [Attention au " integre]
<integer> introduit un entier,      soit <integer>: [rien], </integer>: [rien]
<real> introduit un reel,           soit <real>:    [rien], </real>:    [rien]
<true> introduit la valeur 'vrai',  soit <true>:    'oui'
<false> introduit la valeur 'faux', soit <false>:   'non'
<data> introduit un codage Base64
<date> introduit une date

 soit:
 
 {
	Description = "Timbuktu Host"
	Messages = {
 		start = "Starting Timbuktu Host"
		stop = "Stopping Timbuktu Host"
	}
	OrderPreference = "Late"
	Provides = ( "TimbuktuHost" )
	Requires = ( "Resolver", "Disks" )
 }

 */

typedef struct StructTypeNiveau {
	ArgDesc   *desc_en_cours;
	ArgDesc   *var_en_cours;
	ArgStruct *strt;
	int        posit_a_venir,index;
	char       valeur_a_lire,valeur_lue,table_en_cours;
	struct StructTypeNiveau *precedent;
} ArgTypeNiveau;

static char ArgDelimiteurs[] = "  #,;=:(){}[]";

static int ArgDbg = 0;
static char ArgReport = 1;
static int ArgCompat = 0;
static int ArgNiveau = 0;
static char ArgWeb = 0;
static char ArgStyleCourant = ARG_STYLE_MIG;
static char ArgAjouteCR = 0;
// static char ArgModeIf = DESC_MODE_SUIVANT;
static char *ArgNomDeclare = 0;
static char ArgErreurTexte[256];
static char ArgAplatDemande = 0;
static ItemVar ArgSymbolTable = 0;
static ArgDesc *ArgPrevItem;

static int ArgAjoute(FILE *f, int p, char aide, ArgDesc *desc, char tous, char origine, char separe);
static int ArgGet(FILE *f, int p, ArgDesc *desc_initiale, char *delim, char affecte);

/* ========================================================================== */
void ArgSuitAdresse(char *nom, void *adrs) {
	strncpy(ArgAdrsNom,nom,ARG_MAXSUIVI); ArgAdrsNom[ARG_MAXSUIVI - 1] = '\0';
	ArgAdrsSuivie = adrs;
}
/* ========================================================================== */
void ArgReportError(char report) { ArgReport = report; }
/* ========================================================================== */
void ArgNomEnCours(char *nom)  { ArgNomDeclare = nom; }
/* ==========================================================================
 static void ArgPointeErreur(char *ligne, char *position) {
	int col,k;

	fprintf(stderr,"     ");
	col = position - ligne; k = 0;
	for(k=0; k<col; k++) fprintf(stderr,"%c",(ligne[k] == TAB)? TAB: ' ');
	fprintf(stderr,"^----- ici\n");
 }
 ============================================================================ */
static void ArgImprimeErreur(const char *fctn, int ligne, int num_err, int num_lig, char *lue, int col, char *fmt, ...) {
	va_list argptr; int k,nb; char texte[1024];

	if(!ArgReport) return;
	if(fmt[0] == '\0') return;
	va_start(argptr,fmt); nb = vsnprintf(texte,1024,fmt,argptr); va_end(argptr);

	fprintf(stderr,"(%s:%04d) %2d: %s en ligne %d",fctn,ligne,num_err,texte,num_lig);
	if(ArgNomDeclare) fprintf(stderr," (%s)",ArgNomDeclare);
	fprintf(stderr,"\n");
	if(lue) {
		fprintf(stderr,"    [%s]\n",lue);
		if(col >= 0) {
			fprintf(stderr,"     ");
			for(k=0; k<col; k++) fprintf(stderr,"%c",(lue[k] == TAB)? TAB: ' ');
			fprintf(stderr,"^----- ici\n");
		}
	}
}
/* ========================================================================== */
char ArgBilan(char *nom, int r) {
	if(r > 0) {
		fprintf(stderr,"! ArgScan: %d erreur%s dans le fichier %s\n",Accord1s(r),nom);
		return(-1);
	} else if(r < 0) {
		fprintf(stderr,"! ArgScan: fin inattendue du fichier %s\n",nom);
		return(-1);
	} else return(1);
}
/* ========================================================================== */
void ArgKeyFromList(char *modeles, char **liste) {
	int i;

	modeles[0] = '\0'; i = 0;
	while(*liste[i]) { if(i) strcat(strcat(modeles,"/"),liste[i]); else strcpy(modeles,liste[i]); i++; }
}
/* ========================================================================== */
int ArgKeyGetNb(char *modeles) {
	/* renvoie le nombre de textes dans <modeles> */
	int i; char *c;
	
	c = modeles; i = 1;
	do {
		while(*c) { if(*(++c) == '/') break; }
		if(*c == '\0') break;
		i++;
	} while(1);
	return(i);
}
/* ========================================================================== */
int ArgKeyGetLngr(char *modeles) {
	/* renvoie la longueur maximum des textes dans <modeles> */
	int l,m; char *c,*d;

	d = modeles; m = 0;
	do {
		c = d;
		while(*c) { if(*(++c) == '/') break; }
		l = c - d;
		if(l > m) m = l;
		if(*c == '\0') break;
		d = c + 1;
	} while(1);
	return(m);
}
/* ========================================================================== */
char ArgKeyGetText(char *modeles, char index, char *nom, int max) {
	/* renvoie le nom (tire de <modeles>) correspondant a la variable <index> */
	int i,l; char *c,*d,*s;

	s = modeles; i = 0; d = c = s;
	while(i++ <= index) {
		d = s;
		while(*c) if(*(++c) == '/') { s = c + 1; break; };
		if(*c == '\0') break;
	};
	if(i > index) {
		l = c - d; if(l > (max - 1)) l = max - 1;
		strncpy(nom,d,l); nom[l] = '\0';
		return(1);
	} else {
		nom[0] = '\0';
		return(0);
	}
}
/* ========================================================================== */
int ArgKeyGetIndex(char *modeles, char *texte) {
	/* renvoie l'index dans <modeles> du texte, s'il y est trouve */
	int i,l,n; char *c,*d;

	n = strlen(texte);
	d = modeles; i = 0;
	do {
		c = d;
		while(*c) { if(*(++c) == '/') break; }
		l = c - d;
		if((l == n) && !strncmp(d,texte,n)) break;
		if(*c == '\0') { i = -1; break; }
		d = c + 1; i++;
	} while(1);
	return(i);
}
/* ========================================================================= */
void ArgHelp(ArgDesc *desc) {
/* Imprime l'aide associee a une description de parametres d'appel.
   Utilisee soit avec le parametres -h, soit en cas d'erreur 
   de syntaxe des parametres.
   Appeler cette fonction fait quitter le programme. */
	register int k; register ArgDesc *a,*d;
	// register char *n;
	char nom[80],eol[3];

	if(ArgAjouteCR) strcpy(eol,"\n\r"); else strcpy(eol,"\n");
	strncpy(nom,DESC_APPLI,80);
//	n = nom; while(*n) { *n = toupper(*n); n++; }
	fprintf(stderr,"%s: ",nom);
	d = desc;
	while(d->type >= 0) {
		if(!d->type) {
			if(d->explic) fprintf(stderr,"%s",d->explic);
			fprintf(stderr,"%s",eol);
		} else break;
		d++;
	}
	if(d == desc) fprintf(stderr,"%s",eol);
	fprintf(stderr,"Syntaxe: %s <opts>",nom);

	a = d; k = 0;
	while(DESC_NOT_LAST(a)) {
		if(a->adrs && !a->texte) {
			switch(a->type) {
			  case ARG_TYPE_TEXTE:  case ARG_TYPE_INT: case ARG_TYPE_HEXA:
			  case ARG_TYPE_SHORT: case ARG_TYPE_SHEX: case ARG_TYPE_FLOAT:
			  case ARG_TYPE_OCTET: case ARG_TYPE_BYTE: 
			  case ARG_TYPE_LISTE: case ARG_TYPE_USER: fprintf(stderr," <%s%d>",ArgNomType[(int)a->type],++k); break;
			  case ARG_TYPE_BOOL:  fprintf(stderr," oui/non"); ++k; break;
			  case ARG_TYPE_KEY: fprintf(stderr," %s",a->modele); k++; break;
			}
		}
		a++;
	}
	if(k) fprintf(stderr," <opts>"); fprintf(stderr,"%s",eol);

	if(k) /* parametres positionnels */ {
		a = d; k = 0;
		while(DESC_NOT_LAST(a)) {
			if(a->adrs && !a->texte) {
				switch(a->type) {
				  case ARG_TYPE_TEXTE:
					fprintf(stderr,"  <%s%d>: ",ArgNomType[(int)a->type],++k);
					if(a->explic) fprintf(stderr,"%s%s",a->explic,eol);
					else fprintf(stderr,"chaine de caracteres...\n");
					break;
				  case ARG_TYPE_INT:
					fprintf(stderr,"  <%s%d>: ",ArgNomType[(int)a->type],++k);
						if(a->explic) fprintf(stderr,"%s%s",a->explic,eol);
					else fprintf(stderr,"valeur entiere...\n");
					break;
				  case ARG_TYPE_HEXA:
					fprintf(stderr,"  <%s%d>: ",ArgNomType[(int)a->type],++k);
						if(a->explic) fprintf(stderr,"%s%s",a->explic,eol);
					else fprintf(stderr,"valeur hexadecimale...\n");
					break;
				  case ARG_TYPE_SHORT:
					fprintf(stderr,"  <%s%d>: ",ArgNomType[(int)a->type],++k);
						if(a->explic) fprintf(stderr,"%s%s",a->explic,eol);
					else fprintf(stderr,"valeur sur 16 bits...\n");
					break;
				  case ARG_TYPE_SHEX:
					fprintf(stderr,"  <%s%d>: ",ArgNomType[(int)a->type],++k);
						if(a->explic) fprintf(stderr,"%s%s",a->explic,eol);
					else fprintf(stderr,"valeur hexadecimale sur 16 bits...\n");
					break;
				  case ARG_TYPE_FLOAT:
					fprintf(stderr,"  <%s%d>: ",ArgNomType[(int)a->type],++k);
						if(a->explic) fprintf(stderr,"%s%s",a->explic,eol);
					else fprintf(stderr,"valeur flottante...\n");
					break;
				  case ARG_TYPE_OCTET:
					fprintf(stderr,"  <%s%d>: ",ArgNomType[(int)a->type],++k);
						if(a->explic) fprintf(stderr,"%s%s",a->explic,eol);
					else fprintf(stderr,"valeur sur 8 bits...\n");
					break;
				  case ARG_TYPE_BYTE:
					fprintf(stderr,"  <%s%d>: ",ArgNomType[(int)a->type],++k);
						if(a->explic) fprintf(stderr,"%s%s",a->explic,eol);
					else fprintf(stderr,"valeur non signee hexadecimale sur 8 bits...\n");
					break;
				  case ARG_TYPE_BOOL:
				  case ARG_TYPE_KEY:
				  case ARG_TYPE_LISTE:
				  case ARG_TYPE_USER:
					if(a->explic) {
						fprintf(stderr,"<%s%d>: ",ArgNomType[(int)a->type],++k);
						fprintf(stderr,"%s%s",a->explic,eol);
					}
					break;
				}
			}
			a++;
		}
	}

	/* Mot-cles */
	a = d;
	fprintf(stderr,"  <opts>:%s    -help: ce que vous lisez... (egalement --help, --h, -?)%s",eol,eol);
	fprintf(stderr,"    -*: ecriture des parametres%s",eol);
	while(DESC_NOT_LAST(a)) {
		if(a->adrs && a->texte) {
			fprintf(stderr,"    -%s",a->texte);
			switch(a->type) {
			  case ARG_TYPE_TEXTE: case ARG_TYPE_INT:  case ARG_TYPE_HEXA:
			  case ARG_TYPE_SHORT: case ARG_TYPE_SHEX: case ARG_TYPE_FLOAT:
			  case ARG_TYPE_OCTET: case ARG_TYPE_BYTE:
			  case ARG_TYPE_LISTE: case ARG_TYPE_USER: fprintf(stderr,"=<%s>",ArgNomType[(int)a->type]); break;
			  case ARG_TYPE_KEY: fprintf(stderr,"=%s",a->modele); break;
			}
			if(a->explic) fprintf(stderr,": %s%s",a->explic,eol); else fprintf(stderr,"%s",eol);
		} else if(!a->adrs && a->explic) fprintf(stderr,"%s%s",a->explic,eol);
		a++;
	}
	fprintf(stderr,"Valeurs effectivement utilisees:%s",eol);
	ArgPrint("?",desc);
	exit(0);
}
/* ========================================================================= */
void ArgDebug(int niveau) { ArgDbg = niveau; }
/* ========================================================================= */
void ArgCompatible(char compat) { ArgCompat = compat; }
/* ========================================================================= */
void ArgStdoutBrut(char avec_cr) { ArgAjouteCR = avec_cr; }
/* ========================================================================= */
void ArgSymbolAdd(char *nom, char *valeur) {
	ArgSymbolTable = ItemVarAdd(ArgSymbolTable,nom,valeur);
}
/* ========================================================================= */
int ArgDecodeVal(char *vallue, ArgDesc *a, int index, char fic) {
/* Attribue une valeur a une variable (decrite par a).
   La valeur (chaine contenue dans vallue) peut optionnellement 
   etre precedee du signe = 
   Le parametre fic assure le fonctionnement correct pour les booleens lus
   dans un fichier.
   Cette fonction retourne la longueur decodee (y compris le signe =) */
	register int k,l,r; int col;
	register char *sep,*item;
	char *adrs; int dim,nb; 
	int tempo; char *pchar; unsigned char *octet;
	char **liste;
	char minus[8]; char cle_permise[DESC_CLE_LNGR]; int l0;
	TypeArgUserFctn fctn;

	if(ArgDbg >= 1) {
		if((a->dim == 1) && (index == 0)) printf("   *Affectation: %s %s = '%s'\n",
			ArgNomType[(int)a->type],a->texte? a->texte: "positionnel",vallue);
		else printf("   *Affectation: %s %s[%d] = '%s'\n",
			ArgNomType[(int)a->type],a->texte? a->texte: "positionnel",index,vallue);
	}
	ArgErreurTexte[0] = '\0';
	a->lu = 1;
	dim = a->dim; if((dim == 0) && a->taille) dim = *(a->taille) + 1; /* admet un element en plus de la taille courante */
	adrs = (char *)a->adrs;
	if(adrs && a->handle) /* dans ce cas, *(a->taille) doit etre prealablement initialise */ {
		adrs = (char *)(*(long *)adrs);
		if(!adrs) {
			switch(a->type) {
			  case ARG_TYPE_TEXTE: 
				if((a->lngr == 0) && (dim == 1)) { nb = strlen(vallue); if(*vallue != '=') nb++; }
				else nb = a->lngr + 1; break;
			  case ARG_TYPE_INT:
			  case ARG_TYPE_HEXA:  nb = sizeof(int); break;
			  case ARG_TYPE_SHORT:
			  case ARG_TYPE_SHEX:  nb = sizeof(short); break;
			  case ARG_TYPE_FLOAT: nb = sizeof(float); break;
			  case ARG_TYPE_OCTET: case ARG_TYPE_BYTE: case ARG_TYPE_BOOL: case ARG_TYPE_KEY:
				nb = sizeof(char); break;
			  case ARG_TYPE_LISTE: nb = sizeof(int); break;
			  case ARG_TYPE_USER: nb = sizeof(int); break;
			  default: nb = 0;
			}
			if(nb && dim) adrs = (char *)malloc(dim * nb);
			*((long *)a->adrs) = (long)adrs;
			if(adrs) a->dim = dim;
		}
	}
	if(!adrs) dim = 0;
	r = 0;
	if(*vallue == '=') { vallue++; r = 1; }
	if(index < dim) switch(a->type) {
	  case ARG_TYPE_TEXTE:
		if(a->lngr) {
			char *var;
			var = adrs + (index * a->lngr);
			strncpy(var,vallue,a->lngr);
			// ItemVarUsePrefix(ArgSymbolTable,vallue,var,a->lngr);
			*(var + a->lngr - 1) = '\0';
		} else strcpy(adrs,vallue);
		break;
	  case ARG_TYPE_INT:   
		if(!strncmp(vallue,"0x",2)) sscanf(vallue,"%x",(int *)adrs + index);
		else sscanf(vallue,"%d",(int *)adrs + index);
		break;
	  case ARG_TYPE_HEXA:  sscanf(vallue,"%x",(unsigned int *)adrs + index); break;
	  case ARG_TYPE_SHORT: 
		if(!strncmp(vallue,"0x",2)) sscanf(vallue,"%hx",(unsigned short *)adrs + index);
		else sscanf(vallue,"%hd",(unsigned short *)adrs + index);
		break;
	  case ARG_TYPE_SHEX:  sscanf(vallue,"%hx",(unsigned short *)adrs + index); break;
	  case ARG_TYPE_FLOAT: sscanf(vallue,"%f",(float *)adrs + index); break;
	  case ARG_TYPE_CHAR:
		if(!strncmp(vallue,"0x",2)) sscanf(vallue,"%x",&tempo);
		else sscanf(vallue,"%i",&tempo);
		pchar = (char *)adrs + index;
		*pchar = (tempo & 0xFF);
		break;
	  case ARG_TYPE_OCTET:
	  case ARG_TYPE_BYTE:
		if(!strncmp(vallue,"0x",2)) sscanf(vallue,"%x",&tempo);
		else sscanf(vallue,"%i",&tempo);
		octet = (unsigned char *)adrs + index;
		*octet = (tempo & 0xFF);
		break;
	  case ARG_TYPE_LETTRE:
		pchar = (char *)adrs + index;
		*pchar = vallue[0];
		break;
	  case ARG_TYPE_BOOL:
		if((r > 0) || fic) {
			strncpy(minus,vallue,8); minus[7] = '\0'; ItemMinuscules(minus);
			if(!strcmp(minus,"off") || !strcmp(minus,"false")  || !strcmp(minus,"faux") || (minus[0] == 'n'))
				*(adrs + index) = 0;
			else *(adrs + index) = 1;
			break;
		} else { *(adrs + index) = 1; return(0); }
		break;
	  case ARG_TYPE_KEY:
		sep = a->modele - 1;
		k = 0;
		do {
			sep = item = sep + 1;
			while(*sep) { if(*sep == '/') break; sep++; }
			l = (int)(sep - item);
			// ambiguite et erreur possible si 2 cles ont le meme debut: if(!strncmp(item,vallue,l)) { *(adrs + index) = k & 255; return(l + r); }
			strncpy(cle_permise,item,l); cle_permise[l] = '\0';
			if(!strcmp(cle_permise,vallue)) { *(adrs + index) = k & 255; return(l + r); }
			if(k == 0) l0 = l;
			k++;
		} while(*sep);
		strncpy(cle_permise,a->modele,l); cle_permise[l] = '\0';
		sprintf(ArgErreurTexte,"Valeur '%s' inattendue. Attendu: %s. Remplacee par %s",vallue,a->modele,cle_permise);
	    *(adrs + index) = 0;
		break;
	  case ARG_TYPE_LISTE:
		liste = (char **)(a->modele);
		k = 0;
		while(*liste[0] != '\0') {
			// if(ArgDbg >= 2) printf("    . liste[%d]=%s = %s?\n",k,*liste,vallue);
			if(!strcmp(*liste++,vallue)) { *((int *)adrs + index) = k; /* if(ArgDbg >= 2) printf("    @%08X=%d\n",(hexa)((int *)adrs + index),k); */ return(strlen(vallue) + r); }
			k++;
		}
		// if(ArgDbg >= 2) printf("    ! mot-cle %s introuvable\n",vallue);
		sprintf(ArgErreurTexte,"Valeur '%s' inattendue. Remplacee par %s",vallue,*(char **)(a->modele));
		*((int *)adrs + index) = 0;
		break;
	  case ARG_TYPE_USER:
		fctn = (TypeArgUserFctn)(a->modele);
		fctn(vallue,(int *)adrs + index,&col);
		break;
	  case ARG_TYPE_STRUCT:
	  case ARG_TYPE_RIEN:
		break;
	  default:
		sprintf(ArgErreurTexte,"Affectation '%s[%d] = %s' de type invalide: #%d (%s)",
			a->texte? a->texte: "positionnel",index,vallue,a->type,
			((a->type >= 0) && (a->type < ARG_TYPE_EXEC))? ArgNomType[(int)a->type]: "indefini");
	} else if((a->type == ARG_TYPE_BOOL) && (r == 0) && !fic) return(0);
	return(strlen(vallue) + r);
}
/* ========================================================================= */
char *ArgItemGetName(ArgDesc *desc, int k) {
	if(k >= 0) return(desc[k].texte); else return("");
}
/* ========================================================================= */
int ArgItemGetIndex(ArgDesc *desc, char *texte) {
	int k; ArgDesc *a;

	a = desc; k = 0;
	while(DESC_NOT_LAST(a)) {
		if(a->texte) if(!strcmp(a->texte,texte)) break;
		a++; k++;
	}
	if(DESC_NOT_LAST(a)) return(k); else return(-1);
}
/* ========================================================================= */
char ArgItemLu(ArgDesc *desc, int index) {
	if(index < 0) return(0);
	return((desc+index)->lu);
}
/* ========================================================================= */
char ArgVarLue(ArgDesc *desc, char *texte) {
	int index;
	index = ArgItemGetIndex(desc,texte);
	if(index < 0) return(0); else return((desc+index)->lu);
}
/* ========================================================================= */
int ArgGetVarLue(ArgDesc *desc) {
	int k; ArgDesc *a;

	a = desc; k = 0;
	while(DESC_NOT_LAST(a)) {
		if(a->lu) break;
		a++; k++;
	}
	if(DESC_NOT_LAST(a)) return(k); else return(-1);
}
/* ========================================================================= */
void ArgVarUtilise(ArgDesc *desc, char *texte, char utile) {
	int index;
	index = ArgItemGetIndex(desc,texte);
	if(index >= 0) (desc+index)->lu = utile;
}
/* ========================================================================= */
static char ArgIdent(char *argv, ArgDesc *desc, char arg, int *k, int km, ArgDesc **p) {
/* Identifie la categorie (positionnel ou predefini) d'un parametre argv,
   et le fait decoder et assigner.
   Le parametre est predefini si:
   - en mode arg, il commence par '-';
   - sinon, il contient un '='.
   Les nombres actuel et maximum de parametres positionnels sont a fournir
   dans *k et km, et la prochaine description positionnelle commencera a 
   partir de *p
   Retourne vrai si tout va bien, faux en cas d'erreur. */
	register int i,l,n; int m,d;
	register char c; char predefini,trouve;
	ArgDesc *a; char ok;

	ok = 1;
	if(arg) {
		if((a = ArgPrevItem)) {
			ArgDecodeVal(argv,a,0,0);
			ArgPrevItem = 0;
			return(1);
		}
		predefini = (argv[0] == '-'); d = 1;
	} else { predefini = (index(argv,'=') != NULL); d = 0; }
	if(predefini) {
		if(!strcmp(argv+1,"help") || !strcmp(argv+1,"-help") || !strcmp(argv+1,"-h") || !strcmp(argv+1,"?")) return(0);
		else if(!strcmp(argv+1,"!")) return(1);
		l = strlen(argv);
		for(i=d; i<l; i++) {
			c = argv[i];
			if(c == '?') ok= 0;
		    /*  else if(c == '!') ArgPrint("*",desc); pas le bon endroit */
			else {
				a = desc;
				while(DESC_NOT_LAST(a)) {
					if(a->texte) {
						m = strlen(a->texte);
						/* pb si un nom est contenu dans le nom d'un parm suivant */
//						printf("pour '%s', teste '%s' (%d cars.)\n",argv+i,a->texte,m);
						if(m == 1) trouve = (c == *a->texte);
						else {
							trouve = !strncmp(argv+i,a->texte,m) && ((argv[i+m] == '\0') || (argv[i+m] == '='));
							if(trouve) i += (m - 1);
						}
						if(trouve) {
							if((a->type != ARG_TYPE_BOOL) && (argv[i+1] == '\0')) {
								ArgPrevItem = a;
								return(1);
							}
							n = ArgDecodeVal(argv+i+1,a,0,0);
							i += n;
							break;
						}
					}
					a++;
				}
				if(DESC_LAST(a)) {
					fprintf(stderr,"Parametre '%s' incompris!\n",argv+i);
					ok = 0;
				}
			}
		}
	} else {
		*k += 1;
		if(*k > km) {
			fprintf(stderr,"Deja %d parametres positionnels!\n",*k);
			ok = 0;
			return(ok);
		}
		while((*p)->type >= 0) { if((*p)->adrs && !(*p)->texte && ((*p)->type < ARG_TYPE_EXEC)) break; *p += 1; }
		if((*p)->type > 0) {
			ArgDecodeVal(argv,*p,0,0);
			*p += 1;
		} else {
			fprintf(stderr,"Parametre positionnel #%d mal defini\n",*k);
			ok = 0;
		}
	}
	return(ok);
}
/* ========================================================================= */
char ArgDecodeItem(char *item, ArgDesc *desc) {
/* Decode et attribue la valeur de la chaine de caracteres item.
   Retourne vrai si tout va bien, faux en cas d'erreur. */
	ArgDesc *p;
	int k,km;

	/* Tout ca pour determiner le nombre maximum de parametres positionnels */
	p = desc;
	km = 0; while(DESC_NOT_LAST(p)) { if(p->adrs && !p->texte && (p->type < ARG_TYPE_EXEC)) km++; p++; }

	k = 0; p = desc;
	return(ArgIdent(item,desc,0,&k,km,&p));
}
/* ========================================================================= */
char ArgSplit(char *argv, char delim, ArgDesc *desc) {
/* Decoupe une chaine unique (argv) en parametres separes par le 
   delimiteur delim, et assure l'assignation aux variables */
	char *r,*item;
	ArgDesc *p;
	int k,km;
	char ok;

	ok = 1;
	/* Determine le nombre maximum de parametres positionnels
	   et pre-assigne la valeur faux aux booleens */
	p = desc;
	km = 0; while(DESC_NOT_LAST(p)) {
		if(p->adrs && !p->texte && (p->type < ARG_TYPE_EXEC)) km++;
		if(p->type == ARG_TYPE_BOOL) *(char *)p->adrs = 0;
		p++;
	}

	/* Analyse parametre par parametre */
	ArgPrevItem = 0; k = 0; p = desc;
	r = argv;
	do {
		item = ItemJusquA(delim,&r);
		if(item[0] == '\0') break;
		if(!ArgIdent(item,desc,0,&k,km,&p)) ok = 0;
	} while(1);

	return(ok);
}
/* ========================================================================== */
char ArgFromWeb(ArgDesc *desc) {
	int lngr; char requete[256]; char *c;
	
	requete[0] = '\0';
	c = getenv("REQUEST_METHOD"); // printf("<br>%s = %s\n","REQUEST_METHOD",c?c:"indetermine");
	if(c && !strcmp(c,"POST")) {
		c = getenv("CONTENT_LENGTH");
		if(c) sscanf(c,"%d",&lngr); else lngr = 256;
		fgets(requete,lngr+1,stdin);
	} else {
		c = getenv("QUERY_STRING");
		if(c) strncpy(requete,c,256);
	}
	// printf("<br>Requete: '%s'\n",requete);
	return(ArgSplit(requete,'&',desc));
}
/* ========================================================================= */
char ArgList(int argc, char *argv[], ArgDesc *desc) {
/* Balaie un tableau de chaines (argv[]), et assure l'assignation aux variables */
	int j;
	ArgDesc *p;
	int k,km;
	char ok;

	ok = 1;
	/* Determine le nombre maximum de parametres positionnels
	   et pre-assigne la valeur faux aux booleens */
	km = 0; p = desc;
	while(DESC_NOT_LAST(p)) {
		if(p->adrs && !p->texte && (p->type < ARG_TYPE_EXEC)) km++;
		if(p->type == ARG_TYPE_BOOL) *(char *)p->adrs = 0;
		p++;
	}
	if(p) {
		if(p->adrs) {
			ArgFctn f; void *bloc_a_lire;
			f = (ArgFctn)(p->adrs); bloc_a_lire = (void *)(p->explic);
			f(bloc_a_lire);
		}
	}
	
	/* Analyse chaine par chaine */
	ArgPrevItem = 0; j = 0; k = 0; p = desc;
	while(++j < argc) if(!ArgIdent(argv[j],desc,1,&k,km,&p)) ok = 0;
	if(!ok) ArgHelp(desc);
	if(!strcmp(argv[argc-1],"-!")) { ArgPrint("*",desc); ok = 0; }
	
	return(ok);
}
/* ========================================================================= */
char ArgListChange(int argc, char *argv[], ArgDesc *desc) {
/* Balaie un tableau de chaines (argv[]), et assure l'assignation aux variables */
	int j;
	ArgDesc *p;
	int k,km;
	char ok;
	
	ok = 1;
	/* Determine le nombre maximum de parametres positionnels
		et NE pre-assigne PAS de valeur a quelque variable que ce soit */
	km = 0; p = desc;
	while(DESC_NOT_LAST(p)) {
		if(p->adrs && !p->texte && (p->type < ARG_TYPE_EXEC)) km++;
		p++;
	}
	
	/* Analyse chaine par chaine */
	ArgPrevItem = 0; j = 0; k = 0; p = desc;
	while(++j < argc) if(!ArgIdent(argv[j],desc,1,&k,km,&p)) ok = 0;
	if(!ok) ArgHelp(desc);
	if(!strcmp(argv[argc-1],"-!")) { ArgPrint("*",desc); ok = 0; }
	
	return(ok);
}
/* ========================================================================= */
ArgDesc *ArgCreate(int n) { return((ArgDesc *)malloc((n+1) * sizeof(ArgDesc))); }	
/* ========================================================================= */
void ArgAdd(ArgDesc *c, char *texte, char lu, int type, char hndl, short lngr, int dim, 
	int *taille, char *modele, void *adrs, char *explic) {
	ArgDesc *d;

	c->texte = texte; c->type = type; c->handle = hndl;
	c->lngr = lngr; c->dim = dim; c->taille = taille;
	c->modele = modele;
	c->adrs = adrs; c->explic = explic;
	c->lu = 0;
	d = c+1;
	d->texte = (char *)0; d->type = -1; d->handle = 0;
	d->lngr = 0; d->dim = 0; d->taille = (int *)0;
	d->modele = (char *)0;
	d->lu = 0;
	d->adrs = (void *)0; d->explic = (char *)0;
}
/* ========================================================================= */
void ArgDefaut(ArgDesc *c, ArgFctn f, void *bloc_a_lire) {
/* remplace un DESC_END par un DESC_END_DEFAUT (pour les desc remplies par ArgAdd notamment) */
	ArgDesc *a;

	if((a = c)) {
		while(DESC_NOT_LAST(a)) a++;
		a->adrs = (void *)f; a->explic = (char *)bloc_a_lire;
	}
}
/* ========================================================================= */
static void ArgExec(ArgDesc *c) {
	if(c) {
		// printf("(%s) Execution d'une fonction @%08X\n",__func__,(hexa)c->adrs);
		if(c->adrs) {
			ArgFctn f; void *bloc_a_lire;
			f = (ArgFctn)(c->adrs); bloc_a_lire = (void *)(c->explic);
			f(bloc_a_lire);
		}
	}
}
/* ========================================================================= */
void ArgLngr(ArgDesc *c, char *texte, short lngr, char *fmt) {
	ArgDesc *p;

	p = c;
	while(DESC_NOT_LAST(p)) {
		if(!strcmp(p->texte,texte)) {
			if(lngr > 0) p->lngr = lngr;
			if(fmt && (p->type != ARG_TYPE_KEY) && (p->type != ARG_TYPE_LISTE) && (p->type != ARG_TYPE_USER)) p->modele = fmt;
			return;
		}
		p++;
	}
}
/* ========================================================================= */
int ArgFromFile(FILE *f, ArgDesc *desc, char *delim) {
	return(ArgGet(f,-1,desc,delim,1));
}
/* ========================================================================= */
int ArgSkipOnFile(FILE *f, ArgDesc *desc, char *delim) {
	return(ArgGet(f,-1,desc,delim,0));
}
/* ========================================================================= */
int ArgFromPath(int p, ArgDesc *desc, char *delim) {
	return(ArgGet(0,p,desc,delim,1));
}
/* ========================================================================= */
int ArgSkipOnPath(int p, ArgDesc *desc, char *delim) {
	return(ArgGet(0,p,desc,delim,0));
}
/* ========================================================================= */
static int ArgGet(FILE *f, int p, ArgDesc *desc_initiale, char *delim, char affecte)  {
/* Balaie un fichier (ouvert par f=fopen()), et assure l'assignation aux variables,
   jusqu'a apparition de la chaine delim.
   Une ligne commencant par # est un commentaire.
   On peut inclure la lecture d'un sous-fichier avec une ligne @<nom-de-fichier>.
   Retourne le nombre d'erreurs, ou (si 0 erreur) -1 si fin de fichier atteinte avant delimiteur */
	register int i,numlig,erreur;
	int posit_a_venir,index,l,n; // col,dim;
	char ligne[MAXLIGNE],lue[MAXLIGNE],*c;
	char *suite,*sep,*item_lu;
	char trouve,abandon_ligne,non_blanc_avant,fin_affectation,fin_lecture;
	char valeur_a_lire,valeur_lue,table_en_cours;
	ArgDesc *desc,*var_en_cours,**variante,*autre; ArgStruct *strt;
	ArgTypeNiveau *niveau,*niv_prec;

	if(ArgDbg >= 1) {
		var_en_cours = desc_initiale;
		printf("==  %s appele avec ArgDbg=%d et la definition: {\n",__func__,ArgDbg);
		while(DESC_NOT_LAST(var_en_cours)) {
			printf("        %s %s[%d] @%08X\n",ArgNomType[var_en_cours->type],var_en_cours->texte,var_en_cours->dim,(hexa)var_en_cours->adrs);
			if(var_en_cours->type == ARG_TYPE_VARIANTE) {
				if(!var_en_cours->modele && (autre = *(ArgDesc **)(var_en_cours->adrs))) /* simple inclusion */ {
					printf("        Inclusion de {\n");
					while(DESC_NOT_LAST(autre)) {
						printf("            %s %s[%d] @%08X\n",ArgNomType[autre->type],autre->texte,autre->dim,(hexa)autre->adrs);
						autre++;
					}
					printf("        }\n");
				}
			}
			var_en_cours++;
		}
		printf("    }\n");
	}

	niveau = 0;
	desc = desc_initiale;
	var_en_cours = desc;
	if(affecte && var_en_cours) while(DESC_NOT_LAST(var_en_cours)) {
		if(var_en_cours->taille) {
			if((var_en_cours->dim == 0) && (*(var_en_cours->taille) != 0)) {
				var_en_cours->dim = *(var_en_cours->taille);
				if(ArgDbg >= 1) printf("    taille dynamique initiale: %s %s[%d]\n",ArgNomType[var_en_cours->type],var_en_cours->texte?var_en_cours->texte:"anonyme",var_en_cours->dim);
			}
			*(var_en_cours->taille) = 0;
		}
		var_en_cours++;
	}
	ArgExec(var_en_cours);
	var_en_cours = 0;
	posit_a_venir = 0;
	index = 0;
	strt = 0;
	valeur_a_lire = 0;
	valeur_lue = 0;
	table_en_cours = 0;
	erreur = 0;
	fin_lecture = 0;
	numlig = 0;
	ArgDelimiteurs[0] = TAB;
	
	/* Lecture d'une ligne du fichier tant que la fin d'icelui n'a pas ete atteinte */
	do {
		if(f && (p < 0)) c = LigneSuivante(ligne,MAXLIGNE,f);
		else /* (!f && (p >= 0)) */ c = EnregSuivant(ligne,MAXLIGNE,p);
		if(!c) { 
			if(delim) {
				if(!erreur) erreur = -2;
				ArgImprimeErreur(ICI,++erreur,numlig,0,-1,"fin du fichier, delimiteur '%s' introuvable\n",delim);
			}
			break;
		}
		numlig++;
		strncpy(lue,ligne,MAXLIGNE - 1); c = &(lue[strlen(lue) - 1]); if(*c == '\n') *c = '\0';
		if(ArgDbg >= 2) printf("    ------------------------------------------------------\n");
		if(ArgDbg >= 1) printf("%02d) Lue: \" %s \"\n",numlig,lue);
		if(!lue[0]) continue;  /* <cr><lf> engendre une ligne supplementaire vide */
		if(lue[0] == '#') continue;
		if(!strncmp(lue,"//",2)) continue;
/*
	ligne :== affectation { [ {','|';'} affectation ] }
	affectation :== [blancs] <nom> { blancs | '=' | ':' | TABs } ['('] <valeur> [ {, <valeur>} ')' ]
	valeur :== { scalaire | { { affectations | '{' } '}' } }
 */
		/* debut de ligne: on saute les blancs en tete. Le delim de fin de desc peut etre rencontre */
		suite = ligne;
		abandon_ligne = 0;
		non_blanc_avant = 0;
		/* recherche d'une affectation tant que la ligne n'est pas finie, sauf commentaire */
		do {
			while(*suite) {
				if((*suite != ' ') && (*suite != TAB)) break;
				suite++;
			}
			if(delim && !niveau) { i = suite - ligne; if(!strcmp(delim,lue+i)) { fin_lecture = 1; break; } }
			/* cas special, pas de suite sur la meme ligne */
			if(*suite == '@') {
				sep = suite + 1;
				while(*sep) {
					if((*sep == '\0') || (*sep == '\n')) break;
					i++; sep++;
				}
				*sep = '\0';
				if(ArgScan(suite+1,desc) <= 0) ArgImprimeErreur(ICI,++erreur,numlig,lue,-1,"lecture de %s en erreur",suite+1);
				break;
			}
			item_lu = ItemTrouve(&suite,ArgDelimiteurs,&trouve); /* n'evite plus TAB */
			if(!non_blanc_avant && *item_lu) non_blanc_avant = 1;
			if((trouve == ' ') || (trouve == TAB)) {
				while(*suite) {
					if((*suite != ' ') && (*suite != TAB)) break;
					suite++;
				}
				trouve = *suite;
				if(*suite) {
					char *d;
					d = ArgDelimiteurs + 2; /* evite <blanc> et TAB */
					while(*d) { if(*suite == *d) { suite++; break; }; d++; }
					if(!(*d)) trouve = '=';
				}
			}
			if(trouve == '[') trouve = '('; else if(trouve == ']') trouve = ')';
			else if(trouve == ':') { if(*suite == '=') { trouve = '@'; suite++; } else trouve = '='; } // ":=" precedement traduit en ","
			//- else if((trouve == '=') && ((suite-ligne) >= 2) && (*(suite-2) == ':')) { trouve = ' '; *(suite-2) = '\0'; } 
 			else if(trouve == '\n') trouve = '\0';
			if(*suite == '\n') *suite = '\0';
			fin_affectation = (trouve == ',') || (trouve == ';') || (trouve == ')') || (trouve == '}');
			abandon_ligne = ((trouve == '#') || (trouve == '\0') || (*suite == '\0'));
			if(ArgDbg >= 1) printf("    Item lu: '%s', trouve '%c', suite='%c' (variable %s, valeur %s et %s non-blanc), affectation %s\n",
					item_lu,trouve,*suite,var_en_cours? (var_en_cours->texte? var_en_cours->texte: "positionnelle"):"a definir",
					valeur_a_lire?"a lire":"pas attendue",non_blanc_avant?"suite de":"premier",fin_affectation?"trouvee":"a trouver");
			if(valeur_a_lire) {
				if(*item_lu || (trouve == '#') || (trouve == ',') || (trouve == ';') || (((trouve == ')') || (trouve == '}')) && non_blanc_avant) || (trouve == '\0')) valeur_lue = 1;
			} else if(*item_lu == '\0') {
				if(abandon_ligne && !fin_affectation) break;
			} else {
				if(/* (*item_lu != '\0') on n'est PAS de suite tombes sur un debut de valeur ('(', '{', etc..)
				&& */ !fin_affectation && !abandon_ligne && desc) /* ni, ensuite, sur une fin d'affectation */ {
					/* variable nommee? */
					ArgDesc *alias_vu;
					alias_vu = 0;
					var_en_cours = (trouve == '@')? 0: desc;
					while(DESC_NOT_LAST(var_en_cours)) {
						if(var_en_cours->type == ARG_TYPE_VARIANTE) {
							if(var_en_cours->modele) {
								variante = (ArgDesc **)var_en_cours->adrs;
								l = *(var_en_cours->modele);
								if(ArgDbg == 1) printf("    (%s.%d) variante #%d\n",ICI,l);
								if(l < 0) l = 0;
								/* contestable: pas de marque de fin de tableau de variante[]: else {
									n = 0; while(variante[n] && (n <= l)) n++; if(n > 0) --n;
									if(l > n) l = n;
								} */
								autre = variante[l];
								while(DESC_NOT_LAST(autre)) {
									if(autre->texte) { if(!strcmp(autre->texte,item_lu)) break; }
									if(ArgDbg == 1) printf("    (%s.%d) '%s' pas pareil que '%s'\n",ICI,autre->texte,item_lu);
									autre++;
								}
								if(ArgDbg == 1) printf("    (%s.%d) ... on termine avec autre=%08X\n",ICI,(hexa)autre);
								if(autre && autre->texte) { var_en_cours = autre; break; } /* .. oui! */
							} else if((autre = *(ArgDesc **)(var_en_cours->adrs))) /* simple inclusion */ {
								if(ArgDbg == 1) printf("    (%s.%d) autre=%08X ->type=%08X ->texte=%08X:'%s'\n",ICI,(hexa)autre,(hexa)(autre->type),(hexa)(autre->texte),autre->texte);
								while(DESC_NOT_LAST(autre)) {
									if(ArgDbg == 1) printf("    (%s.%d) autre=%08X ->type=%08X ->texte=%08X:'%s'/'%s'\n",ICI,(hexa)autre,(hexa)(autre->type),(hexa)(autre->texte),autre->texte,item_lu);
									if(autre->texte) { if(!strcmp(autre->texte,item_lu)) break; }
									if(ArgDbg == 1) printf("    (%s.%d) '%s' pas pareil que '%s'\n",ICI,autre->texte,item_lu);
									autre++;
									if(ArgDbg == 1) printf("    (%s.%d) on change pour autre=%08X\n",ICI,(hexa)autre);
								}
								if(ArgDbg == 1) printf("    (%s.%d) ... on termine avec autre=%08X\n",ICI,(hexa)autre);
								if(autre && autre->texte) { var_en_cours = autre; break; }
							}
						}
						if(var_en_cours->texte) {
							if(!strcmp(var_en_cours->texte,item_lu)) {
								if((var_en_cours->type == ARG_TYPE_RIEN) && var_en_cours->modele) {
									if(!alias_vu) alias_vu = var_en_cours;
									else if(alias_vu == var_en_cours) {
										ArgImprimeErreur(ICI,++erreur,numlig,lue,item_lu-ligne,"le renvoi de l'alias '%s' revient sur lui-meme",var_en_cours->texte);
										var_en_cours = 0; break;
									}
									item_lu = var_en_cours->modele;
									var_en_cours = desc;
									continue;
								}
								break;
							}
						}
						var_en_cours++;
					}
					if(var_en_cours) {
						if(var_en_cours->texte) { valeur_a_lire = 1; valeur_lue = 0; } /* .. oui! */
						else var_en_cours = 0;
					}
				}
				if(!var_en_cours && desc) /* ou !valeur_a_lire? */ {
					if(trouve == '=') {
						if(!ArgCompat) {
							ArgImprimeErreur(ICI,++erreur,numlig,lue,item_lu-ligne,"mot-cle '%s' inconnu",item_lu);
							// ArgPointeErreur(ligne,item_lu);
						}
						valeur_a_lire = 1;
					} else {
						/* parametre positionnel? */
						i = 0; var_en_cours = desc;
						while(DESC_NOT_LAST(var_en_cours)) {
							if((var_en_cours->type != ARG_TYPE_COMMENT) && (var_en_cours->type < ARG_TYPE_EXEC)) {
								if(var_en_cours->type == ARG_TYPE_VARIANTE) {
									if(var_en_cours->modele) {
										l = *(var_en_cours->modele);
										variante = (ArgDesc **)var_en_cours->adrs;
										if(l < 0) l = 0; else {
											n = 0; while(variante[n] && (n <= l)) n++; --n;
											if(l > n) l = n;
										}
										autre = variante[l];
										while(DESC_NOT_LAST(autre)) {
											if(autre->texte == 0) { if(i == posit_a_venir) { posit_a_venir++; break; } i++; }
											autre++;
										}
										if(autre->type > 0) { var_en_cours = autre; break; } /* .. oui! */
									} else if((autre = *(ArgDesc **)(var_en_cours->adrs))) /* simple inclusion */ {
										// printf("(%s.3) autre=%08X ->type=%08X ->texte=%08X\n",__func__,(hexa)autre,(hexa)(autre->type),(hexa)(autre->texte));
										// printf("(%s.3) ='%s'\n",__func__,autre->texte);
										while(DESC_NOT_LAST(autre)) {
											   // printf("(%s.4) autre=%08X ->type=%08X ->texte=%08X\n",__func__,(hexa)autre,(hexa)(autre->type),(hexa)(autre->texte));
											   // printf("(%s.4) ='%s'\n",__func__,autre->texte);
											if(autre->texte) { if(!strcmp(autre->texte,item_lu)) break; }
											autre++;
										}
										if(DESC_NOT_LAST(autre)) { var_en_cours = autre; break; }									
									}
								}
								if(var_en_cours->texte == 0) { if(i == posit_a_venir) { posit_a_venir++; break; } i++; }
							}
							var_en_cours++;
						}
						if(var_en_cours->type > 0) { valeur_a_lire = 0; valeur_lue = 1; } /* .. oui! */
						else {
							if(!ArgCompat) {
								ArgImprimeErreur(ICI,++erreur,numlig,lue,suite-1-ligne,"parametre positionnel #%d en excedent, ou mot-cle '%s' inconnu",
									posit_a_venir,item_lu);
								// ArgPointeErreur(ligne,suite - 1);
							}
							var_en_cours = 0;
						}
					}
				}
				if(var_en_cours) var_en_cours->lu = 1; // on pourrait ici mettre *taille a 0
				index = 0;
			}
			if(ArgDbg >= 1) printf("    Var en cours: '%s[%d]' [si positionnel: #%d], valeur lue: '%s'\n",
				var_en_cours? (var_en_cours->texte? var_en_cours->texte: "(positionnel)"):"(a definir)",index,posit_a_venir,
				valeur_lue? item_lu: "(pas encore)");
			/* if(var_en_cours) {
				if(var_en_cours->type != ARG_TYPE_STRUCT) {
					if(trouve == '{') trouve = '('; else if(trouve == '}') trouve = ')';
				}
			} */
			if(trouve == '{') {
				niv_prec = niveau;
				niveau = (ArgTypeNiveau *)malloc(sizeof(ArgTypeNiveau));
				if(niveau) {
					niveau->desc_en_cours = desc;
					niveau->var_en_cours = var_en_cours;
					niveau->strt = strt;
					niveau->posit_a_venir = posit_a_venir;
					niveau->index = index;
					niveau->valeur_a_lire = valeur_a_lire;
					niveau->valeur_lue = valeur_lue;
					niveau->table_en_cours = table_en_cours;
					niveau->precedent = niv_prec;
				}
				if(!var_en_cours) continue;
				if(var_en_cours->type == ARG_TYPE_RIEN) {
					strt = 0;
					desc = 0;
				} else if(var_en_cours->dim >= 0) {
					strt = (ArgStruct *)(var_en_cours->adrs);
					desc = strt->desc;
				} else {
					strt = 0;
					desc = (ArgDesc *)(var_en_cours->adrs);
				}
				var_en_cours = desc;  // ?
				if(affecte && var_en_cours) while(DESC_NOT_LAST(var_en_cours)) {
					if((long)(var_en_cours->taille)) {
						if((var_en_cours->dim == 0) && (*(var_en_cours->taille) != 0)) {
							var_en_cours->dim = *(var_en_cours->taille);
							if(ArgDbg >= 3) printf("    taille dynamique corrigee: %s %s[%d]\n",ArgNomType[var_en_cours->type],var_en_cours->texte?var_en_cours->texte:"anonyme",var_en_cours->dim);
						}
						*(var_en_cours->taille) = 0;
					}
					var_en_cours++;
				}
				//? var_en_cours = 0;
				posit_a_venir = 0;
				index = 0;
				valeur_a_lire = 0;
				valeur_lue = 0;
				table_en_cours = 0;
				var_en_cours = desc;
				if(ArgDbg >= 2) printf("    Structure ouverte: {\n");
				if(var_en_cours) while(DESC_NOT_LAST(var_en_cours)) {
					if(ArgDbg >= 2) printf("        %s %s[%d] @%08X\n",ArgNomType[var_en_cours->type],var_en_cours->texte,var_en_cours->dim,(hexa)var_en_cours->adrs);
					var_en_cours++;
				}
				if(ArgDbg >= 2) printf("    }\n");
				ArgExec(var_en_cours);
				var_en_cours = desc;
				continue;
			} else if(trouve == '(') { table_en_cours++; if(ArgDbg >= 2) printf("    Table ouverte\n"); continue; }
			if(var_en_cours && valeur_lue && (var_en_cours->type != ARG_TYPE_RIEN)) {
				if((var_en_cours->dim == 0) || (index < var_en_cours->dim)) {
					if(affecte) {
						if((trouve != ')') || !table_en_cours || *item_lu) /* pour traiter la syntaxe '()' */ {
							ArgDecodeVal(item_lu,var_en_cours,index,1);
							if(ArgErreurTexte[0]) ArgImprimeErreur(ICI,++erreur,numlig,lue,-1,ArgErreurTexte);
							index++;
						}
						if(var_en_cours->taille) *(var_en_cours->taille) = index;
						if(ArgDbg >= 3) printf("    %s.taille=%d\n",var_en_cours->texte?var_en_cours->texte: "anonyme",(var_en_cours->taille)?*(var_en_cours->taille):-1);
						if(index >= var_en_cours->dim) var_en_cours = 0;
					}
				} else {
					if(!ArgCompat) {
						ArgImprimeErreur(ICI,++erreur,numlig,lue,suite-1-ligne,"pas plus de %d element(s) pour %s",
							var_en_cours->dim,var_en_cours->texte? var_en_cours->texte: "ce positionnel");
						// ArgPointeErreur(ligne,suite - 1);
					}
					var_en_cours = 0;
				}
				valeur_lue = 0;
			}
			while((trouve == '}') || (trouve == ')')) {
				if(trouve == '}') {
					if(niveau) {
						desc = niveau->desc_en_cours;
						var_en_cours = niveau->var_en_cours;
						index = niveau->index;
						if(var_en_cours && (var_en_cours->type != ARG_TYPE_RIEN)) {
							if(((var_en_cours->dim == 0) || (index < var_en_cours->dim))) {
								if(strt) {
									if(strt->table && strt->tempo && strt->lngr) {
										memcpy((void *)((long)(strt->table) + (index * strt->lngr)),strt->tempo,(size_t)strt->lngr);
										if(ArgDbg >= 2) {
											int t;
											printf("   *Affectation: %s %s[%d] = {\n",ArgNomType[(int)var_en_cours->type],
												var_en_cours->texte? var_en_cours->texte: "anonyme",index);
											t = ArgNiveau; ArgNiveau = 2; ArgAjoute(stdout,0,0,strt->desc,1,-1,0); ArgNiveau = t;
											printf("    }\n");
										}
										index++;
									}
								}
								if(var_en_cours->taille && affecte) *(var_en_cours->taille) = index;
								if(ArgDbg >= 3) printf("    %s.taille=%d\n",var_en_cours->texte?var_en_cours->texte: "anonyme",(var_en_cours->taille)?*(var_en_cours->taille):-1);
							} else if(strt && !ArgCompat) {
								ArgImprimeErreur(ICI,++erreur,numlig,lue,suite-1-ligne,"pas plus de %d element(s) pour %s",
									var_en_cours->dim,var_en_cours->texte? var_en_cours->texte: "ce positionnel");
								// ArgPointeErreur(ligne,suite - 1);
							}
						}
						strt = niveau->strt;
						posit_a_venir = niveau->posit_a_venir;
						valeur_a_lire = niveau->valeur_a_lire;
						valeur_lue = niveau->valeur_lue;
						table_en_cours = niveau->table_en_cours;
						niv_prec = niveau->precedent;
						free(niveau);
						niveau = niv_prec;
					} else strt = 0;
					if(ArgDbg >= 2) printf("    Structure fermee, suite='%c'\n",*suite);
				} else if(trouve == ')') {
					if(table_en_cours) --table_en_cours;
					var_en_cours = 0; 
					valeur_a_lire = 0;
					if(ArgDbg >= 2) printf("    Table fermee\n");
				}
				valeur_lue = 0;
				while(*suite) {
					if((*suite != ' ') && (*suite != TAB)) break;
					suite++;
					if(*suite == '\n') *suite = '\0';
				}
				trouve = *suite; if(*suite) { suite++; if(*suite == '\n') *suite = '\0'; }
				abandon_ligne = ((trouve == '#') || (trouve == '\0') || (*suite == '\0'));
				if(ArgDbg >= 2) printf("    Marques suivantes: trouve='%c', suite='%c'\n",trouve,*suite);
			};
			if(ArgDbg >= 2) printf("    trouve='%c', abandon_ligne %s, table_en_cours %s\n",
								   trouve,abandon_ligne? "vrai":"faux",table_en_cours? "vrai":"faux");
			if((trouve == ';')
			|| (((trouve == ',') || abandon_ligne) && !table_en_cours)
			|| (((trouve != ',') && !abandon_ligne) && table_en_cours)) { var_en_cours = 0; valeur_a_lire = 0; }
		} while(!abandon_ligne && *suite);
		if(fin_lecture || (erreur > 20)) break;
	} while(1);

	var_en_cours = desc;
	if(ArgDbg >= 1) printf("(%s) Execution de fonctions? {\n",__func__);
	while(DESC_NOT_LAST(var_en_cours)) {
		if(ArgDbg >= 1) printf("        %s %s[%d] @%08lX\n",ArgNomType[var_en_cours->type],var_en_cours->texte,var_en_cours->dim,(lhex)var_en_cours->adrs);
		if(var_en_cours->type == ARG_TYPE_VARIANTE) {
			if(!var_en_cours->modele && (autre = *(ArgDesc **)(var_en_cours->adrs))) /* simple inclusion */ {
				if(ArgDbg >= 1) printf("        Inclusion de {\n");
				while(DESC_NOT_LAST(autre)) {
					if(ArgDbg >= 1) printf("            %s %s[%d] @%08lX\n",ArgNomType[autre->type],autre->texte,autre->dim,(lhex)autre->adrs);
					if(autre->type == ARG_TYPE_EXEC) {
						if(ArgDbg >= 1) printf("(%s) Execution d'une fonction @%08lX en ligne %d\n",__func__,(lhex)autre->adrs,(int)(autre-*(ArgDesc **)(var_en_cours->adrs)));
						ArgExec(autre);
					}
					autre++;
				}
				if(ArgDbg >= 1) printf("        }\n");
			}
		} else if(var_en_cours->type == ARG_TYPE_EXEC) {
			if(ArgDbg >= 1) printf("(%s) Execution d'une fonction @%08lX en ligne %d\n",__func__,(lhex)var_en_cours->adrs,(int)(var_en_cours-desc));
			ArgExec(var_en_cours);
		}
		var_en_cours++;
	}
	if(ArgDbg >= 1) printf("}\n");

	return(erreur);
}
/* ========================================================================= */
char ArgScan(char *nom, ArgDesc *desc)  {
/* Decode le fichier nom, qui sera ouvert ici en bufferise (fopen()).
   Retourne 0 si le fichier est illisible, -1 en cas d'erreur(s) de decodage, 
   1 si tout va bien */
	int r;
	FILE *f;

	if(nom) { if(!strcmp(nom,"*")) f = stdin; else f = fopen(nom,"r"); } else return(0);
	if(!f) return(0);
	ArgNomEnCours(nom);
	r = ArgFromFile(f,desc,0);
	fclose(f);
	ArgNomEnCours(0);
	return(ArgBilan(nom,r));
}
/* ========================================================================= */
ARG_STYLES ArgStyle(ARG_STYLES style) {
	ARG_STYLES ancien;

	ancien = ArgStyleCourant;
	/* if(style >= 0) */ ArgStyleCourant = style;
	return(ancien);
}
/* ========================================================================= */
void ArgIndent(int n) { ArgNiveau = n; }
/* ========================================================================= */
static char ArgPseudoAffect(ArgDesc *desc, int *nb) {
/* Tout ca pour determiner s'il y a un parametre positionnel en tete et pas d'explication */
	int k,n,i,p,e;
	ArgDesc *a;
/* i: numero de variable
   k: nombre de positionnels
   e: nombre d'explications
   p: premier numero de positionnel
   n: premier numero de variable nommee */

	k = 0; n = -1; i = 0; p = -1; e = 0;
	a = desc;
	while(DESC_NOT_LAST(a)) {
		if(a->adrs && (a->type < ARG_TYPE_EXEC)) {
			if(!a->texte && (a->type != ARG_TYPE_VARIANTE)) { k++; if(p < 0) p = i; }
			else { if(n < 0) n = i; }
			if(a->explic) e++;
			i++;
		}
		a++;
	}
	if(n < 0) n = p + 1;
	*nb = i;
//	return((k == 1) && (p < n) && (e == 0));
	return((k > 0) && (p < n) && (e == 0)); /* autres positionnels admis */
}
/* ========================================================================= */
static INLINE void ArgAjouteVirgule(FILE *f, char aide, int index, char a_la_ligne) {
	int n;

	if(a_la_ligne) {
		fprintf(f,",\n"); if(!aide) for(n=0; n<ArgNiveau; n++) fprintf(f,"%c",TAB);
	} else if(index % 10) fprintf(f,", ");
	else { fprintf(f,",\n"); if(!aide) for(n=0; n<=ArgNiveau; n++) fprintf(f,"%c",TAB);  }
}
/* ========================================================================= */
static char ArgContientDelim(char *symbole, char *delim) {
	char *s,*d;

	if((ArgStyleCourant == ARG_STYLE_LINUX) || (ArgStyleCourant == ARG_STYLE_JSON)) return(1);
	s = symbole;
	while(*s) {
		d = delim;
		while(*d) { if(*s == *d) return(1); d++; }
		s++;
	}
	return(0);
}
/* ========================================================================= */
static int ArgAjoute(FILE *f, int p, char aide, ArgDesc *desc, char tous, char origine, char separe) {
/* Ecrit le contenu complet d'une description (desc) sur un fichier deja
   ouvert f (par fopen(), qui peut de ce fait etre stdio ou stderr).
   La mise en page est legerement differente s'il s'agit d'une aide */
	register int k,l,v;
	register ArgDesc *a,*d,**u;
	register char *m;
	int i,n,nb,col; char pseudo_affect,a_plat,a_la_ligne;
	char debut_liste,fin_liste,comment[4],fin_ligne[2];
	char txt[256],*item;
	int index,dim;
	void *adrs;
	int *pint; short *pshort; unsigned char *octet; char *pchar; float *pfloat; char **pliste;
	ArgStruct *strt;
	TypeArgUserFctn fctn;
	char eol[3];

	if(!desc) return(1);
	if(!f && (p >= 0)) { f = fdopen(p,"w"); p = 0; }
	//= printf("(%s) Ecriture de la description @%08X\n",__func__,(hexa)desc);
	if(ArgAjouteCR) strcpy(eol,"\n\r"); else strcpy(eol,"\n");
	a = desc; nb = 0;
	if(ArgStyleCourant == ARG_STYLE_JSON) {
		debut_liste = '['; fin_liste = ']'; strcpy(comment,"//"); strcpy(fin_ligne,",");
		pseudo_affect = 0;
		if(origine < 0) { if(ArgWeb) fprintf(f,"<br>"); fprintf(f,"{%s",eol); ArgNiveau++; }
	} else {
		debut_liste = '('; fin_liste = ')'; strcpy(comment,"#"); fin_ligne[0] = '\0';
		pseudo_affect = ArgPseudoAffect(desc,&nb);
		while(a->type >= 0) {
			if(a->type == ARG_TYPE_COMMENT) {
				if(a->explic && !aide) {
					if(ArgWeb) fprintf(f,"<br>"); fputs(comment,f);
					if(ArgNiveau) for(n=0; n<ArgNiveau; n++) fprintf(f,"%c",TAB);
					else fprintf(f," ");
					fprintf(f,"%s%s",a->explic,eol);
				}
			} else break;
			a++;
		}
	}
//	if(ArgStyleCourant == ARG_STYLE_JSON) pseudo_affect = 0;
//	else pseudo_affect = ArgPseudoAffect(desc,&nb);
	a_plat = ArgAplatDemande;
	if(separe && !aide && (ArgNiveau == 0) && (ArgStyleCourant != ARG_STYLE_LINUX) && (ArgStyleCourant != ARG_STYLE_JSON) && !pseudo_affect && !ArgAplatDemande) fprintf(f,"%s%s",comment,eol);

	ArgDelimiteurs[0] = TAB;
//	if(ArgStyleCourant == ARG_STYLE_JSON) { debut_liste = '['; fin_liste = ']'; }
//	else { debut_liste = '('; fin_liste = ')'; }

	a_la_ligne = 0; strt = 0;
	i = 0; k = 0;
	while(DESC_NOT_LAST(a)) {
		if((!tous && !(a->lu) && (a->type != ARG_TYPE_VARIANTE)) || (a->type >= ARG_TYPE_EXEC)) { a++; continue; }
		if(!aide && (a->type != ARG_TYPE_RIEN) && (a->type != ARG_TYPE_VARIANTE)) {
			if(!pseudo_affect && !a_plat) { if(ArgWeb) fprintf(f,"<br>"); for(n=0; n<ArgNiveau; n++) fprintf(f,"%c",TAB); }
		}
		adrs = a->adrs;
		if(a->handle && adrs) adrs = (void *)*(long *)a->adrs;
		if(adrs) {
			if(a->texte) {
				if(aide) {
					if(a->type == ARG_TYPE_BOOL) fprintf(f,"  -%s ",a->texte);
					else fprintf(f,"  -%s=",a->texte);
				} else if(a->type != ARG_TYPE_VARIANTE) {
					if(ArgStyleCourant == ARG_STYLE_LINUX) fprintf(f,"%s=",a->texte);
					else if(ArgStyleCourant == ARG_STYLE_JSON) fprintf(f,"\"%s\": ",a->texte);
					else fprintf(f,"%s = ",a->texte); /* fprintf(f,"%s%c",a->texte,TAB);*/
				} else if(ArgStyleCourant != ARG_STYLE_JSON) {
					fputs(comment,f);
					for(n=0; n<ArgNiveau; n++) fprintf(f,"%c",TAB);
					fprintf(f,"%s",a->texte);
					adrs = a->modele;
					d = desc;
					while(d->type >= 0) { if(d->adrs == adrs) break; d++; }
					if(d->type < 0) fprintf(f," selon option");
					else {
						if(d->texte) fprintf(f," pour %s '",d->texte);
						else fprintf(f," pour '");
						switch(d->type) {
							case ARG_TYPE_INT:    fprintf(f,"%d",*(int *)adrs); break;
							case ARG_TYPE_HEXA:   fprintf(f,"%#x",*(int *)adrs); break;
							case ARG_TYPE_SHORT:  fprintf(f,"%hd",*(short *)adrs); break;
							case ARG_TYPE_SHEX:   fprintf(f,"%#hx",*(short *)adrs); break;
							case ARG_TYPE_CHAR:   fprintf(f,"%hhd",*(char *)adrs); break;
							case ARG_TYPE_OCTET:  fprintf(f,"%hhd",*(unsigned char *)adrs); break;
							case ARG_TYPE_BYTE:   fprintf(f,"%#hhx",*(unsigned char *)adrs); break;
							case ARG_TYPE_LETTRE: fprintf(f,"%c",*(char *)adrs); break;
							case ARG_TYPE_BOOL:   fprintf(f,"%s",(*(char *)adrs)?"oui":"non"); break;
							case ARG_TYPE_KEY: 
								m = item = d->modele;
								v = (int)*(char *)adrs;
								while(*m) {
									if(*m == '/') { if(!(v--)) break; else item = m + 1; }
									m++;
								}
								l = (int)(m - item);
								strncpy(txt,item,l); txt[l] = '\0';
								/* if(ArgContientDelim(txt,ArgDelimiteurs)) fprintf(f,"\"%s\"",txt);
								else */ fputs(txt,f);
								break;
							case ARG_TYPE_LISTE: 
								pliste = (char **)(d->modele);
								pint = (int *)adrs;
								m = pliste[*pint++];
								/* if(ArgContientDelim(m,ArgDelimiteurs)) fprintf(f,"\"%s\"",m);
								else */ fputs(m,f);
								break;
							case ARG_TYPE_USER: 
								fctn = (TypeArgUserFctn)(d->modele);
								fprintf(f,"%s",(char *)fctn(0,(int *)adrs,&col));
								break;
							default: fprintf(f,"(valeur indescriptible)");
						}
						fprintf(f,"'");
					}
					fprintf(f,"%s",eol);
				}
			} else if(aide) {
				fprintf(f,"  <%s%d>: ",ArgNomType[a->type],++k);
			}
//			dim = a->dim; if((dim == 0) && a->taille) dim = *(a->taille);
			if(a->taille) dim = *(a->taille); else dim = a->dim;
			/* pas de <CR> ici, car on enchaine sur l'impression de la valeur */
			if(ArgDbg >= 3) printf("(%s %s[%s%d]]) ",ArgNomType[a->type],a->texte?a->texte:"anonyme",a->taille?"*":"",dim);
			if((a->type == ARG_TYPE_TEXTE) && !a->lngr) dim = 1;
			if(dim == 0) fprintf(f,"%c%c",debut_liste,fin_liste);
			else if(dim > 1) {
				if(a->type == ARG_TYPE_STRUCT) {
					fprintf(f,"%c%s",debut_liste,eol);
					ArgNiveau++;
					if(!aide) { if(ArgWeb) fprintf(f,"<br>"); for(n=0; n<ArgNiveau; n++) fprintf(f,"%c",TAB); }
				} else fprintf(f,"%c ",debut_liste);
			}
			switch(a->type) {
			  case ARG_TYPE_TEXTE: 
				for(index=0; index<dim; index++) {
					if(index) ArgAjouteVirgule(f,aide,index,a_la_ligne);
					item = (char *)adrs + (index * a->lngr);
					if(ArgContientDelim(item,ArgDelimiteurs)) fprintf(f,"\"%s\"",item); else fputs(item,f);
				}
				break;
			  case ARG_TYPE_INT:   // fprintf(f,"%d",*(int *)adrs); break;
				pint = (int *)adrs;
				for(index=0; index<dim; index++) {
					if(index) ArgAjouteVirgule(f,aide,index,a_la_ligne);
					fprintf(f,"%d",*pint++);
				}
				break;
			  case ARG_TYPE_HEXA:  // fprintf(f,"%x",*(int *)adrs); break;
				pint = (int *)adrs;
				for(index=0; index<dim; index++) {
					if(index) ArgAjouteVirgule(f,aide,index,a_la_ligne);
					fprintf(f,"%#x",*pint++);
				}
				break;
			  case ARG_TYPE_SHORT: // s = (short *)adrs; fprintf(f,"%d",*s); break;
				pshort = (short *)adrs;
				for(index=0; index<dim; index++) {
					if(index) ArgAjouteVirgule(f,aide,index,a_la_ligne);
					fprintf(f,"%hd",*pshort++);
				}
				break;
			  case ARG_TYPE_SHEX:  // s = (short *)adrs; fprintf(f,"%x",*s); break;
				pshort = (short *)adrs;
				for(index=0; index<dim; index++) {
					if(index) ArgAjouteVirgule(f,aide,index,a_la_ligne);
					fprintf(f,"%#hx",*pshort++);
				}
				break;
			  case ARG_TYPE_FLOAT: // fprintf(f,"%g",*(float *)adrs); break;
				pfloat = (float *)adrs;
				for(index=0; index<dim; index++) {
					if(index) ArgAjouteVirgule(f,aide,index,a_la_ligne);
					fprintf(f,"%g",*pfloat++);
				}
				break;
			  case ARG_TYPE_CHAR:  // c = (char *)adrs; fprintf(f,"%d",*c); break;
				pchar = (char *)adrs;
				for(index=0; index<dim; index++) {
					if(index) ArgAjouteVirgule(f,aide,index,a_la_ligne);
					fprintf(f,"%d",*pchar++);
				}
				break;
			  case ARG_TYPE_OCTET:  // c = (unsigned char *)adrs; fprintf(f,"%d",*c); break;
				octet = (unsigned char *)adrs;
				for(index=0; index<dim; index++) {
					if(index) ArgAjouteVirgule(f,aide,index,a_la_ligne);
					fprintf(f,"%d",*octet++);
				}
				break;
			  case ARG_TYPE_BYTE:  // c = (unsigned char *)adrs; fprintf(f,"%#x",*c); break;
				octet = (unsigned char *)adrs;
				for(index=0; index<dim; index++) {
					if(index) ArgAjouteVirgule(f,aide,index,a_la_ligne);
					fprintf(f,"%#x",*octet++);
				}
				break;
			  case ARG_TYPE_LETTRE:  // c = (char *)adrs; fprintf(f,"%c",*c); break;
				pchar = (char *)adrs;
				for(index=0; index<dim; index++) {
					if(index) ArgAjouteVirgule(f,aide,index,a_la_ligne);
					fprintf(f,"%c",*pchar++);
				}
				break;
			  case ARG_TYPE_BOOL:  // if(!a->texte || !aide) fputs(*((char *)adrs)?"on":"off",f); break;
				pchar = (char *)adrs;
				for(index=0; index<dim; index++) {
					if(index) ArgAjouteVirgule(f,aide,index,a_la_ligne);
					if(aide) fputs(*(pchar++)? "present":"absent",f);
					else if(ArgStyleCourant == ARG_STYLE_JSON) fputs(*(pchar++)? "true":"false",f);
					else fputs(*(pchar++)? "oui":"non",f);
				}
				break;
			  case ARG_TYPE_KEY:
				for(index=0; index<dim; index++) {
					if(index) ArgAjouteVirgule(f,aide,index,a_la_ligne);
					m = item = a->modele;
					v = (int)*((char *)adrs + index);
					while(*m) {
						if(*m == '/') { if(!(v--)) break; else item = m + 1; }
						m++;
					}
					l = (int)(m - item);
					strncpy(txt,item,l); txt[l] = '\0';
					if(ArgContientDelim(txt,ArgDelimiteurs)) fprintf(f,"\"%s\"",txt); else fputs(txt,f);
				}
				break;
			  case ARG_TYPE_LISTE:
				pliste = (char **)a->modele;
				pint = (int *)adrs;
					// printf("(%s) modele[0]=%s, var[0]=%d\n",__func__,pliste[0],*pint);
				for(index=0; index<dim; index++) {
					if(index) ArgAjouteVirgule(f,aide,index,a_la_ligne);
					// printf("(%s) m=pliste[%d]=%08X\n",__func__,*pint,(hexa)pliste[*pint]);
					m = pliste[*pint++];
					if(!m) continue;
					if(ArgContientDelim(m,ArgDelimiteurs)) fprintf(f,"\"%s\"",m); else fputs(m,f);
				}
				break;
			  case ARG_TYPE_USER: 
				fctn = (TypeArgUserFctn)a->modele;
				pint = (int *)adrs;
				for(index=0; index<dim; index++) {
					if(index) ArgAjouteVirgule(f,aide,index,a_la_ligne);
					strncpy(txt,(char *)fctn(0,pint++,&col),255); txt[255] = '\0';
					if(ArgContientDelim(txt,ArgDelimiteurs)) fprintf(f,"\"%s\"",txt);
				else fprintf(f,"%s",txt);
				}
				break;
			  case ARG_TYPE_STRUCT:
				if(dim >= 0) {
					strt = (ArgStruct *)adrs;
					if(ArgStyleCourant == ARG_STYLE_JSON) ArgAplatDemande = 0;
					else ArgAplatDemande = ArgPseudoAffect(strt->desc,&n) || (n < 6);
				} else { dim = 1; strt = 0; ArgPseudoAffect(adrs,&n); ArgAplatDemande = (n < 6); }
				for(index=0; index<dim; index++) {
					if(index) ArgAjouteVirgule(f,aide,index,(ArgStyleCourant == ARG_STYLE_JSON)? 1: a_la_ligne);
					fprintf(f,"{");
					if(!ArgAplatDemande) { fprintf(f,"%s",eol); ArgNiveau++; } else fprintf(f," ");
					if(strt) {
						if(strt->tempo && strt->table && strt->lngr) memcpy(strt->tempo,strt->table + (index * strt->lngr),(size_t)(strt->lngr));
						ArgAjoute(f,p,aide,strt->desc,1,a->type,0); /* si on est la, c'est que cette variable doit etre ajoutee, donc dans son entier */
					} else ArgAjoute(f,p,aide,(ArgDesc *)adrs,1,a->type,0);
					if(!ArgAplatDemande) { --ArgNiveau; if(!aide) { if(ArgWeb) fprintf(f,"<br>"); for(n=0; n<ArgNiveau; n++) fprintf(f,"%c",TAB); } }
					else { fprintf(f," "); a_la_ligne = 1; }
					fprintf(f,"}");
				}
				ArgAplatDemande = a_plat;
				break;
			  case ARG_TYPE_VARIANTE:
				if(a->modele) {
					l = *(a->modele);
					u = (ArgDesc **)adrs;
					if(l < 0) l = 0; else {
						n = 0; while(u[n] && (n <= l)) n++; --n; /* trouve n = dimension du tableau */
						if(l > n) l = n;
					}
					ArgAjoute(f,p,aide,u[l],tous,a->type,1);
				} else if(adrs) /* DESC_INCLUDE */ {
					if(*(ArgDesc **)adrs) ArgAjoute(f,p,aide,*(ArgDesc **)adrs,tous,a->type,1);
					else if(ArgStyleCourant == ARG_STYLE_JSON) { a++; continue; }
				}
				break;
			}
			if(dim > 1) {
				if(a->type == ARG_TYPE_STRUCT) {
					fprintf(f,"%s",eol);
					--ArgNiveau;
					if(!aide) { if(ArgWeb) fprintf(f,"<br>"); for(n=0; n<ArgNiveau; n++) fprintf(f,"%c",TAB); }
					fprintf(f,"%c",fin_liste);
				} else fprintf(f," %c",fin_liste);
			}
			a_la_ligne = 0;
			if(a_plat) {
				if(pseudo_affect) {
					if(a->texte) { if(i < (nb - 1)) fprintf(f,", "); }
					else {
						//2 if(!i) { if(origine != ARG_TYPE_VARIANTE) fprintf(f," := "); else fprintf(f,", "); }
						//2 else if((a+1)->adrs) fprintf(f,", "); 
						//2 else fprintf(f," ");
						if((a+1)->adrs) {
							if(!i && (origine != ARG_TYPE_VARIANTE)) fprintf(f," := "); else fprintf(f,", ");
						} else fprintf(f," ");
						//1 else {
						//1	    if((a+1)->texte && (a+1)->adrs) fprintf(f,", ");
						//1	    else fprintf(f," ");
						//1 }
					}
				} else if(aide) {
					if(!a->texte || (a->type != ARG_TYPE_BOOL)) fprintf(f,"%s%s",fin_ligne,eol); // avant json: putc('\n',f);
				} else if(a->explic && (ArgStyleCourant != ARG_STYLE_JSON)) fprintf(f,"%s %c%s %s%s",fin_ligne,TAB,comment,a->explic,eol); // a->explic peut pointer sur n'importe quoi (si struct)
				else if(i < (nb - 1)) fprintf(f,", "); // putc('\n',f);
			} else if(ArgStyleCourant != ARG_STYLE_JSON) {
				if(a->type != ARG_TYPE_VARIANTE) { if(a->explic) fprintf(f,"%s %c%s %s%s",fin_ligne,TAB,comment,a->explic,eol); else fprintf(f,"%s",eol); }
			} else if(DESC_NOT_LAST((a+1))) fprintf(f,"%s%s",fin_ligne,eol); else if(origine != ARG_TYPE_VARIANTE) fprintf(f,"%s",eol);
			i++;
		} else if(a->explic && !aide && (ArgStyleCourant != ARG_STYLE_JSON)) {
	    	fputs(comment,f); if(ArgWeb) fprintf(f,"<br>");
	    	if(ArgNiveau) for(n=0; n<ArgNiveau; n++) fprintf(f,"%c",TAB);
	    	else fprintf(f," ");
	    	fprintf(f,"%s%s",a->explic,eol);
		} else if(!aide && (a->type != ARG_TYPE_RIEN) && (ArgStyleCourant != ARG_STYLE_JSON)) {
			if(ArgStyleCourant == ARG_STYLE_LINUX) fprintf(f,"%s",eol); else fprintf(f,"%s%s",comment,eol);
		}
		a++;
	}
	if((ArgStyleCourant == ARG_STYLE_JSON) && (origine < 0)) { --ArgNiveau; fprintf(f,"}%s",eol); }
	return(1);
}
#ifdef A_REVOIR
			} else if(a->type != ARG_TYPE_VARIANTE) {
				if(ArgStyleCourant != ARG_STYLE_JSON) {
					if(a->explic) fprintf(f,"%s %c%s %s%s",fin_ligne,TAB,comment,a->explic,eol); else fprintf(f,"%s",eol);
				} else {
					if((origine != ARG_TYPE_STRUCT) || DESC_NOT_LAST((a+1))) fprintf(f,"%s%s",fin_ligne,eol); else fprintf(f,"%s",eol);
				}
			}
#endif
/* ========================================================================= */
static int ArgEcritFichier(char *nom, ArgDesc *desc, char tous) {
	/* Ecrit le contenu complet d'une description (desc) sur un fichier
	denomme nom (qui peut etre "*" pour stdio ou "**" pour stderr - dans ce 
				 dernier cas, on imprime plutot une aide).
	Retourne 0 si nom est a 0, -1 si le fichier ne peut etre ouvert */
	char fichier,aide;
	FILE *f;

	aide = 0;
	if(nom) {
		fichier = 0;
		if(!strcmp(nom,"*")) f = stdout;
		else if(!strcmp(nom,"**")) f = stderr;
		else if(!strcmp(nom,"?")) { f = stderr; aide = 1; }
		else if(!strcmp(nom,"@")) { f = stdout; ArgWeb = 1; }
		else { fichier = 1; RepertoireCreeRacine(nom); f = fopen(nom,"w"); }
		//= printf("(%s) Ecrit (via %08X) desc @%08X sur le %s '%s'\n",__func__,(hexa)f,desc,fichier?"fichier":"terminal",nom);
	} else { errno = EIDRM; return(0); }
	if(!f) {
		fprintf(stderr,"%s: %s\n",nom,strerror(errno)); if(ArgAjouteCR) fprintf(stderr,"\r");
		return(-1);
	}
//	if(fichier) {
//		char prg[80];
//		strncpy(prg,DESC_APPLI,80);
//		// n = prg; while(*n) { *n = toupper(*n); n++; }
//		fprintf(f,"# %s - configuration\n",prg); if(ArgAjouteCR) fprintf(stderr,"\r");
//	}
	fprintf(f,"%s %s%s",(ArgStyleCourant == ARG_STYLE_JSON)? "//": "#",DESC_SIGNE,ArgAjouteCR? "\n\r": "\n");
	ArgAjoute(f,0,aide,desc,tous,-1,0);
	if(fichier) fclose(f); /* else putc(0x0c,f); */
	ArgWeb = 0;
	return(1);
}
/* ========================================================================= */
int ArgBegin(FILE *f, char aide, ArgDesc *desc) {
	fprintf(f,"%s %s%s",(ArgStyleCourant == ARG_STYLE_JSON)? "//": "#",DESC_SIGNE,ArgAjouteCR? "\n\r": "\n");
	return(ArgAjoute(f,0,aide,desc,1,-1,0));
}
/* ========================================================================= */
int ArgAppend(FILE *f, char aide, ArgDesc *desc) {
	return(ArgAjoute(f,0,aide,desc,1,-1,1));
}
/* ========================================================================= */
int ArgAppendModif(FILE *f, char aide, ArgDesc *desc) {
	return(ArgAjoute(f,0,aide,desc,0,-1,1));
}
/* ========================================================================= */
int ArgToPath(int p, char aide, ArgDesc *desc) {
	return(ArgAjoute(0,p,aide,desc,1,-1,1));
}
/* ========================================================================= */
int ArgModifToPath(int p, char aide, ArgDesc *desc) {
	return(ArgAjoute(0,p,aide,desc,0,-1,1));
}
/* ========================================================================= */
int ArgPrint(char *nom, ArgDesc *desc) { return(ArgEcritFichier(nom,desc,1)); }
/* ========================================================================= */
int ArgPrintModif(char *nom, ArgDesc *desc) { return(ArgEcritFichier(nom,desc,0)); }
/* ========================================================================= */
int ArgPrintStyle(char *nom, ArgDesc *desc, ARG_STYLES style_demande) {
	int rc; ARG_STYLES style_std;
	style_std = ArgStyleCourant;
	/* if(style_demande >= 0) */ ArgStyleCourant = style_demande;
	rc = ArgPrint(nom,desc);
	ArgStyleCourant = style_std;
	return(rc);
}
/* ========================================================================= */
static void ArgAssign(int ival, short hval, char bval, float rval, char *cval,
	char type, char *modele, char *adrs) {
	/* Attribue une valeur a une variable (a l'adresse adrs) dont le type est dans.. type!!
	En cas de mots-cles, les modeles sont dans.. modele (re !!) */
	register int k,l,r; int col;
	register char *sep,*item;
	unsigned char *pchar;
	char **liste;
	TypeArgUserFctn fctn;
	
	r = 0;
	switch(type) {
	  case ARG_TYPE_TEXTE:
		strcpy(adrs,cval);
		break;
	  case ARG_TYPE_INT:
	  case ARG_TYPE_HEXA:
		*(int *)adrs = ival;
		break;
	  case ARG_TYPE_SHORT:
	  case ARG_TYPE_SHEX:
		*(short *)adrs = hval;
		break;
	  case ARG_TYPE_FLOAT:
		*(float *)adrs = rval;
		break;
	  case ARG_TYPE_OCTET:
		*adrs = bval;
		break;
	  case ARG_TYPE_BYTE:
		pchar = (unsigned char *)adrs;
		*pchar = (unsigned char)bval;
		break;
	  case ARG_TYPE_BOOL:
		*adrs = bval? 1: 0;
		break;
	  case ARG_TYPE_KEY:
		sep = modele - 1;
		k = 0;
		do {
			sep = item = sep + 1;
			while(*sep) { if(*sep == '/') break; sep++; }
			l = (int)(sep - item);
			if(!strncmp(item,cval,l)) { *adrs = k & 255; return; }
			k++;
		} while(*sep);
		break;
	  case ARG_TYPE_LISTE:
		liste = (char **)modele;
		k = 0;
		while(*liste[0] != '\0') {
			if(!strcmp(*liste++,cval)) { *(int *)adrs = k; return; }
		}
		break;
	  case ARG_TYPE_USER:
		fctn = (TypeArgUserFctn)modele;
		fctn(cval,(int *)adrs,&col);
		break;
	  case ARG_TYPE_RIEN:
		  break;
	  default:
		fprintf(stderr,"Le type d'un (au moins) parametre est inconnu [=%d]\n",type);
	}
	return;
}
/* ========================================================================= */
int ArgInput(int p, ArgDesc *desc, int maxi, char *delim)  {
	/* Balaie un fichier (ouvert par p=open()), et assure l'assignation aux variables,
	jusqu'a apparition de la chaine delim.
	Retourne le nombre d'erreurs, ou (si 0 erreur) -1 si fin de fichier atteinte avant delimiteur */
	register int i,n,r;
	register ArgDesc *c;
	register char *sep;
	int k;
	char ligne[MAXLIGNE],*nomvar;
	char ok; char type;
	void *adrs;
	int ival; short hval; char bval; float rval; char cval[256];
	
	k = 0;
	r = 0;
	n = 0;
	do {
		sep = ligne; ok = 1;
		do {
			if(!read(p,sep,sizeof(char))) { ok = 0; break; } /* 0 sur fin de fichier */
		} while(*sep++ != '\0');
		if(!ok) { 
			if(delim) fprintf(stderr,"%2d/ Ligne %d: fin du fichier, et delimiteur introuvable [%s]\n",
				++r,n,delim);
			break;
		}
		if(delim) { if(!strcmp(delim,ligne)) break; }
		read(p,&type,sizeof(char));
		switch(type) {
			case ARG_TYPE_TEXTE:
			case ARG_TYPE_KEY:
			case ARG_TYPE_LISTE:
				sep = cval;
				do {
					if(!read(p,sep,sizeof(char))) { ok = 0; break; } /* 0 sur fin de fichier */
				} while(*sep++ != '\0');
				if(!ok) *sep = '\0';
				break;
			case ARG_TYPE_USER: break;
			case ARG_TYPE_INT:   
			case ARG_TYPE_HEXA:  read(p,&ival,sizeof(int)); break;
			case ARG_TYPE_SHORT: 
			case ARG_TYPE_SHEX:  read(p,&hval,sizeof(short)); break;
			case ARG_TYPE_FLOAT: read(p,&rval,sizeof(float)); break;
			case ARG_TYPE_OCTET: case ARG_TYPE_BYTE: case ARG_TYPE_BOOL:
				read(p,&bval,sizeof(char)); break;
		}
		n++;
		nomvar = ligne; ok = 0;
		if(*nomvar) {
			i = 0; c = desc;
			while(c->type >= 0) {
				if(c->texte) { if(!strcmp(c->texte,nomvar)) break; }
				i++; c++;
			}
			if(c->texte) ok = 1;
		}
		if(ok) {
			adrs = c->adrs;
			if(c->handle && adrs) adrs = (void *)*(long *)c->adrs;
			ArgAssign(ival,hval,bval,rval,cval,c->type,c->modele,(char *)adrs);
		} else {
			k++;
			i = 0; c = desc;
			while(c->type >= 0) {
				if(c->type != ARG_TYPE_COMMENT) {
					if(c->texte == 0) { if(++i == k) break; }
				}
				c++;
			}
			if(c->type > 0) {
				adrs = c->adrs;
				if(c->handle && adrs) adrs = (void *)*(long *)c->adrs;
				ArgAssign(ival,hval,bval,rval,cval,c->type,c->modele,(char *)adrs);
			} else {
				fprintf(stderr,"%2d/ Ligne %d: parametre positionnel #%d en excedent\n    [%s]\n",
						++r,n,k,nomvar);
			}
			
		}
		if(n == maxi) break;
		if(r > 20) break;
	} while(1);
	
	return(r);
}
/* ========================================================================= */
char ArgRead(char *nom, ArgDesc *desc)  {
	/* Decode le fichier nom, qui sera ouvert ici avec open().
	Retourne 0 si le fichier est illisible, -1 en cas d'erreur(s) de decodage, 
	1 si tout va bien */
	int r; int p;
	
	if(nom) p = open(nom,O_RDONLY,0); else return(0);
	if(p < 0) {
		fprintf(stderr,"%s: %s\n",nom,strerror(errno)); if(ArgAjouteCR) fprintf(stderr,"\r");
		return(0);
	}
	r = ArgInput(p,desc,0,0);
	close(p);
	return(ArgBilan(nom,r));
}
/* ========================================================================= */
int ArgOutput(int p, ArgDesc *desc) {
	/* Ecrit le contenu complet d'une description (desc) sur un fichier deja
	ouvert p (par open()). */
	register int k,l,v;
	register ArgDesc *a;
	register char *n;
	void *adrs;
	char txt[256],*item; char **liste;
	TypeArgUserFctn fctn;
	char zero;

	zero = '\0'; l = 0;
	a = desc; k = 0; item = txt; /* pour gcc */
	while(DESC_NOT_LAST(a)) {
		adrs = a->adrs;
		if(a->handle && adrs) adrs = (void *)*(long *)a->adrs;
		if(adrs) {
			if(a->texte) write(p,a->texte,strlen(a->texte)+1);
			else write(p,&zero,1);
			write(p,&(a->type),sizeof(char));
			switch(a->type) {
				case ARG_TYPE_TEXTE: write(p,adrs,strlen(item)+1); break;
				case ARG_TYPE_INT:   
				case ARG_TYPE_HEXA:  write(p,adrs,sizeof(int)); break;
				case ARG_TYPE_SHORT: 
				case ARG_TYPE_SHEX:  write(p,adrs,sizeof(short)); break;
				case ARG_TYPE_FLOAT: write(p,adrs,sizeof(float)); break;
				case ARG_TYPE_OCTET: case ARG_TYPE_BYTE: case ARG_TYPE_BOOL:
					write(p,adrs,sizeof(char)); break;
				case ARG_TYPE_KEY:
					n = item = a->modele;
					v = (int)*((char *)adrs);
					while(*n) {
						if(*n == '/') { if(!(v--)) break; else item = n + 1; }
						n++;
					}
					l = (int)(n - item); if(l > 255) l = 255;
					strncpy(txt,item,l); txt[l] = '\0';
					write(p,txt,l+1);
					break;
				case ARG_TYPE_LISTE:
					liste = (char **)(a->modele);
					v = *(int *)adrs;
					write(p,liste[v],l+1);
				case ARG_TYPE_USER:
					fctn = (TypeArgUserFctn)(a->modele);
					break;
			}
			/* ici, on pourrait ajouter une separation entre items */
		}
		a++;
	}
	return(1);
}
/* ========================================================================= */
int ArgWrite(char *nom, ArgDesc *desc) {
	/* Ecrit en binaire le contenu complet d'une description (desc) 
	sur un fichier denomme nom.
	Retourne 0 si nom est a 0, -1 si le fichier ne peut etre ouvert */
	int p;
	
	p = open(nom,O_WRONLY,0);
	if(p < 0) {
		fprintf(stderr,"%s: %s\n",nom,strerror(errno)); if(ArgAjouteCR) fprintf(stderr,"\r");
		return(-1);
	}
	ArgOutput(p,desc);
	close(p);
	return(1);
}
