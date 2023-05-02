#include "environnement.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
extern char *__progname;

#include <utils/defineVAR.h>

#define IMPRIME_C
#include <impression.h>
#include "html.h"
#define Accord1s(var) var,(var>1)?"s":""
#define Accord2s(var) var,(var>1)?"s":"",(var>1)?"s":""

/*  8 pouces fois 72 dpi: #define A4_LARG 576 */
/* 11 pouces fois 72 dpi: #define A4_HAUT 792 */
/* PDF:
 /Width  1653
 /Heigth 2338
 */

/* 21 cm fois 72 dpi */
#define A4_LARG 595
/* 29.7 cm fois 72 dpi */
// #define A4_HAUT 841 mais visiblement, ce n'est pas la bonne valeur!!
#define A4_HAUT 792

#define A4_TRANSL A4_HAUT
#define GRIS_CLAIR 0xDFFF,0xDFFF,0xDFFF

#define MAX_LIGNE 1024

static char ImpressionOptions[80];
static char ImpressionNomComplet[256];
static char ImpressionCR[4096];
// static int  ImpressionLargeur;
static char ImpressionPortrait;
static IMPRESSION_SUPPORT ImpressionSupport;
static IMPRESSION_FORMAT ImpressionFormat;
static IMPRESSION_CMDE ImpressionCde;
static int  ImpressionHautFeuille,ImpressionInterligne,ImpressionNumLigne,ImpressionMaxLignes;
static int  ImpressionTaille;
static char ImpressionStyleCourant;
static unsigned int ImpressionFontColor;
static char ImpressionEnCours;
static char ImpressionLigne[MAX_LIGNE];
#define MAX_TITRE 80
static char ImpressionTitre[MAX_TITRE];
static IMPRESSION_FORMAT ImprCadresFmt;
static char ImprCadresHtml;
static char ImprCadresOuverts;
static FILE *ImprCadreDest;
static struct {
	int epaisseur;
	char active;
} ImprPs;
static struct {
	char active;
} ImprTex;
static struct {
	int epaisseur;
	int largeur,pad,espace;
	unsigned int bgnd;
	char active; char tables_ouvertes;
} ImprWeb;

#ifdef OS9
static int Pipe,Clavier;
#endif

static FILE *PrintPath;
static char Imprimante[32];
static int ImpressionTirage;
#ifdef OPIUM_H
static Term tImpression;
#else
#define OpiumError printf
#endif /* OPIUM_H */

static void ImprimeColonnesHaut(int i);
static void ImprimeVideLigne();
static inline void ImprimeWeb(char *fmt, ...);

#ifdef OS9
/* ========================================================================== */
void BrancheImprimanteOS9(void) {
	int id; char commande[256];
	char *parametres,*index();

	if((ImpressionCde == IMPRESSION_LPR) || (ImpressionCde == IMPRESSION_ENSCRIPT))
		sprintf(commande,"%s %s -P %s -#%d %s",ImpressionClients[ImpressionCde],ImpressionOptions,Imprimante,ImpressionTirage,ImpressionNomComplet);
	else {
		strcpy(commande,ImpressionClients[ImpressionCde]);
		/* strcat(strcat(commande," "),ImpressionOptions); */
		strcat(strcat(commande," "),ImpressionNomComplet);
	}
	parametres = index(commande,' '); *parametres++ = '\0';
	Pipe = open("/pipe",S_IREAD | S_IWRITE);
	if(Pipe == -1) {
		OpiumError("open(\"pipe\") impossible, erreur #%d (%s)",errno,strerror(errno));
		return(0);
	}
	Clavier = dup(0);
	close(0);
	dup(Pipe);
	id = os9exec(os9forkc,commande,parametres,environ,0,0,3);
	if(id == -1) {
		OpiumError("lancement de l'impression impossible, erreur #%d (%s)",errno,strerror(errno));
		return(0);
	}
	close(0);
	dup(Clavier);
}
#endif
/* ========================================================================== */
void ImpressionInit(void) {
	ImpressionMaxLignes = 50;
	ImpressionFormat = IMPRESSION_PDF;
	ImpressionSupport = IMPRESSION_PAPIER;
	ImpressionCde = IMPRESSION_ENSCRIPT;
	ImprWeb.bgnd = 0xFFFFFF; /* blanc */
	ImpressionFontColor = 0x000000; /* noir */
	ImpressionTaille = 11;
	ImpressionTitre[0] = '\0';
	ImpressionTirage = 1;
	ImpressionEnCours = 0;
	ImprPs.epaisseur = 1;
	ImprPs.active = 0;
	ImprTex.active = 0;
	ImprWeb.epaisseur = 0;
	ImprWeb.largeur = 800;
	ImprWeb.pad = 0;
	ImprWeb.espace = 10;
	ImprWeb.active = 0;
	ImprWeb.tables_ouvertes = 0;
	ImprCadresFmt = IMPRESSION_TEXTE;
	ImprCadresHtml = 0;
	ImprCadresOuverts = 0;
	ImprCadreDest = stdout;
	strcpy(Imprimante,"printer");
	PrintPath = stdout;
	// umask(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
}
/* ========================================================================== */
void ImpressionHauteurPage(int nb) { ImpressionMaxLignes = nb; }
/* ========================================================================== */
void ImpressionFormatte(IMPRESSION_FORMAT fmt) { ImpressionFormat = fmt; }
/* ========================================================================== */
void ImprimeTailleDefaut(int t) { ImpressionTaille = t; }
/* ========================================================================== */
void ImprimeEpaisseurDefaut(int epaisseur) {
	ImprPs.epaisseur = epaisseur;
	ImprWeb.epaisseur = epaisseur;
}
/* ========================================================================== */
void ImpressionServeur(char *dest, char *titre, int tirage) {
	strncpy(Imprimante,dest,32);
	strncpy(ImpressionTitre,titre,MAX_TITRE);
	ImpressionTirage = tirage;
}
/* ========================================================================== */
// int ImpressionLargeurCourante(void) { return(ImpressionLargeur); }
/* ========================================================================== */
int ImpressionHauteurCourante(void) { return(ImpressionMaxLignes); }
/* ========================================================================== */
int ImpressionLigneCourante(void) { return(ImpressionNumLigne); }
/* ========================================================================== */
void ImpressionSurSupport(IMPRESSION_SUPPORT support, void *adrs) {
	/* adrs doit etre un nom de fichier sauf:
	 - si support=terminal et format=texte, alors adrs est une adresse de Term;
	 - si support=terminal et format=html ou http, alors ecriture directe ou via HttpEcrit() */
	ImpressionSupport = support;

#ifdef IMPR_V1
#ifdef OPIUM_H
	if((ImpressionSupport == IMPRESSION_TERMINAL) && (ImpressionFormat == IMPRESSION_TEXTE)) {
		tImpression = (Term)adrs;
		strcpy(ImpressionNomComplet,"*");
	} else
#endif /* OPIUM_H */
	{
		if(((ImpressionFormat == IMPRESSION_HTML) || (ImpressionFormat == IMPRESSION_HTTP)) && (ImpressionSupport == IMPRESSION_TERMINAL))
			strcpy(ImpressionNomComplet,"*");
		else strncpy(ImpressionNomComplet,(char *)adrs,256);
	}
#else /* IMPR_V1 */

	if(ImpressionSupport == IMPRESSION_TERMINAL) {
		if((ImpressionFormat != IMPRESSION_PS) && (ImpressionFormat != IMPRESSION_PDF) && (ImpressionFormat != IMPRESSION_TEX)) {
			strcpy(ImpressionNomComplet,"*");
		#ifdef OPIUM_H
			if(adrs && (ImpressionFormat == IMPRESSION_TEXTE)) tImpression = (Term)adrs;
		#endif
			return;
		}
	}
	if(adrs) strncpy(ImpressionNomComplet,(char *)adrs,256); else strcpy(ImpressionNomComplet,"Listing");

#endif /* IMPR_V1 */
}
/* ========================================================================== */
int ImprimeTraitVertNb(void) {
	int i;
	i = 0; while(ImprTraitVert[i].col >= 0) i++;
	return(i);
}
/* ========================================================================== */
void ImprimeTraitVertPlace(int i, int col) {
	int pos;
	if(i < 0) return;
	pos = (col > 0)? col: -col;
	if(i && (pos <= ImprTraitVert[i-1].col)) return;
	ImprTraitVert[i].col = (short)pos;
	if(i) {
		ImprTraitVert[i-1].largr = (char)(ImprTraitVert[i].col - ImprTraitVert[i-1].col);
		ImprTraitVert[i-1].aligne = (col > 0)? IMPRESSION_LEFT: IMPRESSION_RIGHT;
	}
}
/* ========================================================================== */
void ImprimeTraitVertEcarte(int i, int larg) {
	short espace;

	if(i < 0) return;
	espace = (short)((larg > 0)? larg: -larg);
	if(espace <= 0) return;
	if(i) {
		ImprTraitVert[i].col = ImprTraitVert[i - 1].col + espace;
		ImprTraitVert[i-1].largr = (char)espace;
		ImprTraitVert[i-1].aligne = (larg > 0)? IMPRESSION_LEFT: IMPRESSION_RIGHT;
	}
}
/* ========================================================================== */
void ImprimeTraitVertDernier(int i) {
	ImprTraitVert[i].col = ImprTraitVert[i].largr = -1;
	ImprTraitVert[i].aligne = IMPRESSION_LEFT;
}
/* ========================================================================== */
int ImpressionPrete(char *orientation) {

/*
   format |          TEXTE           |            PS            |           PDF            |          HTML            |          HTTP            |
 support  | contenu destin  fonction | contenu destin  fonction | contenu destin  fonction | contenu destin  fonction | contenu destin  fonction |
 ---------|--------------------------|--------------------------|--------------------------|--------------------------|--------------------------|
 TERMINAL | direct  *       --       | ps      fichier open     | ps      fichier gs+open  | html    *       --       | html    socket  --       |
 FICHIER  | direct  fichier --       | ps      fichier --       | ps      fichier gs       | html    fichier open     | html    fichier download |
 PAPIER   | direct  fichier lpr      | ps      fichier lpr      | ps      fichier lpr      | html    fichier open     | html    fichier open     |

 destination: nom dans ImpressionNomComplet [defini par ImpressionSurSupport()], chemin (printf/fprintf) dans PrintPath
 HTML: SInPATIS, HTTP: serveur
 ecriture socket: via HttpEcrit()
*/
	/* il y a peut-etre mieux? */
	if(!strcmp(orientation,"portrait")) {
		ImpressionPortrait = 1; // ImpressionLargeur = 80;
	} else if(!strcmp(orientation,"landscape")) {
		ImpressionPortrait = 0; // ImpressionLargeur = 132;
	} else {
		ImpressionPortrait = 0; // sscanf(orientation,"%d",&ImpressionLargeur);
		// if((ImpressionLargeur < 0) || (ImpressionLargeur > 132)) ImpressionLargeur = 132;
	}
	ImprimeTraitVertDernier(0);
	ImprimeVideLigne();
	ImprWeb.bgnd = 0xFFFFFF; ImpressionFontColor = 0x000000;
	ImpressionStyleCourant = '\0';
	ImprPs.active = ((ImpressionFormat == IMPRESSION_PS) || (ImpressionFormat == IMPRESSION_PDF));
	ImprTex.active = (ImpressionFormat == IMPRESSION_TEX);
	ImprWeb.active = ((ImpressionFormat == IMPRESSION_HTML) || (ImpressionFormat == IMPRESSION_HTTP));

#ifdef OPIUM_H
	if((ImpressionSupport == IMPRESSION_TERMINAL) && (ImpressionFormat == IMPRESSION_TEXTE)) {
		if(!(OpiumDisplayed(tImpression->cdr))) OpiumDisplay(tImpression->cdr);
		// TermBlank(tImpression);
		TermEmpty(tImpression); /* met hold a 0 */
		TermHold(tImpression);
		ImpressionEnCours = 1;
		return(1);
	} else if(ImpressionSupport == IMPRESSION_EFFACE) {
		/* if(OpiumDisplayed(tImpression->cdr)) */ OpiumClear(tImpression->cdr);
		return(0);
	}
#endif /* OPIUM_H */
	if((ImpressionSupport == IMPRESSION_PAPIER) && !ImprWeb.active) {
		if(ImpressionFormat == IMPRESSION_TEXTE) ImpressionCde = IMPRESSION_ENSCRIPT;
		else ImpressionCde = IMPRESSION_LPR;
		sprintf(ImpressionOptions,"-J %s ",ImpressionTitre);
		if(ImpressionCde == IMPRESSION_LPR) {
			if(ImpressionPortrait) strcat(ImpressionOptions,"-oportrait -oraw"); /* fait bien FF + fonte, mais pas landscape */
			else strcat(ImpressionOptions,"-olanscape -oraw");                   /* fait bien landscape mais pas FF, + pb fonte */
		} else if(ImpressionCde == IMPRESSION_ENSCRIPT) {
			strcat(ImpressionOptions,"-f Courier9 -B -M A4 ");
			if(!ImpressionPortrait) strcat(ImpressionOptions,"-r");
		}
	}
#ifdef OS9
	if(ImpressionSupport == IMPRESSION_PAPIER) BrancheImprimanteOS9();
	else if(!(PrintPath = fopen(ImpressionNomComplet,"w"))) {
		// OpiumError("Impossible d'ouvrir le listing '%s' en ecriture (%s)",ImpressionNomComplet,strerror(errno));
		return(0);
	}
#else
	// umask(0x1B6); // umask(0666); // umask(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	// umask(S_IWOTH);
	umask(0);
	if(!strcmp(ImpressionNomComplet,"*")) PrintPath = stdout;
	else if(!(PrintPath = fopen(ImpressionNomComplet,"w"))) {
		// char cwd[256];
		// getcwd(cwd,80);
		// OpiumError("Impossible d'ouvrir le listing '%s' en ecriture (%s) depuis '%s'",ImpressionNomComplet,strerror(errno),cwd);
		//-- printf("(%s) Impossible d'ouvrir le listing '%s' en ecriture (%s)\n",__func__,ImpressionNomComplet,strerror(errno));
		return(0);
	}
	//- printf("(%s) Impressions dans %08lX\n",__func__,(Ulong)PrintPath);
#endif
	ImpressionEnCours = 1;
	ImpressionNumLigne = 0;
	if(ImprPs.active) {
		fprintf(PrintPath,"%%!PS-Adobe-1.0\n");
		/* Pas obligatoire mais ca fait plus joli... */
		fprintf(PrintPath,"%%%%Creator: GROS Michel\n");
		//			fprintf(PrintPath,"%%%%based on OPIUM4 version %s\n",OPIUM_VSN);
		if(ImpressionPortrait) {
			ImpressionHautFeuille = A4_HAUT;
			fprintf(PrintPath,"%%%%BoundingBox: 0 0 %d %d\n",A4_LARG,A4_HAUT);
			fprintf(PrintPath,"%%%%Orientation: Portrait\n");
		} else {
			ImpressionHautFeuille = A4_LARG;
			fprintf(PrintPath,"%%%%BoundingBox: 0 0 %d %d\n",A4_HAUT,A4_LARG);
			fprintf(PrintPath,"%%%%Orientation: Landscape\n");
		}
		fprintf(PrintPath,"%%%%DocumentPaperSizes: A4\n");
		ImpressionInterligne = 11;
		ImpressionMaxLignes = (ImpressionHautFeuille / ImpressionInterligne) - 1;
		fprintf(PrintPath,"%%%%DocumentFonts: Courier\n");
		fprintf(PrintPath,"%%%%EndComments\n\n");
		/* Ca non plus [...aussi] */
		fprintf(PrintPath,"%%%%EndProlog\n\n");
		//	fprintf(PrintPath,"%%%%Page: %d %d\n",WndPSnum,WndPSnum);

		fprintf(PrintPath,"/accents [\n");
		fprintf(PrintPath,"  16#80 /euro\n");
		fprintf(PrintPath,"  16#82 /eacute\n");
		fprintf(PrintPath,"  16#85 /agrave\n");
		fprintf(PrintPath,"  16#88 /agrave\n");
		fprintf(PrintPath,"  16#8a /egrave\n");
		fprintf(PrintPath,"  16#8d /ccedilla\n");
		fprintf(PrintPath,"  16#8e /eacute\n");
		fprintf(PrintPath,"  16#8f /egrave\n");
		fprintf(PrintPath,"  16#90 /ecircumflex\n");
		fprintf(PrintPath,"  16#94 /icircumflex\n");
		fprintf(PrintPath,"  16#99 /ocircumflex\n");
		fprintf(PrintPath,"  16#b0 /ordm\n");
		fprintf(PrintPath,"  16#e0 /agrave\n");
		fprintf(PrintPath,"  16#e2 /acircumflex\n");
		fprintf(PrintPath,"  16#e7 /ccedilla\n");
		fprintf(PrintPath,"  16#e8 /egrave\n");
		fprintf(PrintPath,"  16#e9 /eacute\n");
		fprintf(PrintPath,"  16#ea /ecircumflex\n");
		fprintf(PrintPath,"  16#eb /edieresis\n");
		fprintf(PrintPath,"  16#ee /icircumflex\n");
		fprintf(PrintPath,"  16#ef /idieresis\n");
		fprintf(PrintPath,"  16#f4 /ocircumflex\n");
		fprintf(PrintPath,"  16#f9 /ugrave\n");
		fprintf(PrintPath,"  16#fb /ucircumflex\n");
		fprintf(PrintPath,"] def\n");

		// fprintf(PrintPath,"/accentue 20 dict def begin\n");
		fprintf(PrintPath,"  /police_base /Courier findfont def\n");
		fprintf(PrintPath,"  /nouveau_dico police_base maxlength dict def\n");
		fprintf(PrintPath,"  police_base {\n");
		fprintf(PrintPath,"    exch dup /FID ne\n");
		fprintf(PrintPath,"      { dup /Encoding eq\n");
		fprintf(PrintPath,"        { exch dup length array copy\n");
		fprintf(PrintPath,"          nouveau_dico 3 1 roll put }\n");
		fprintf(PrintPath,"        { exch nouveau_dico 3 1 roll put }\n");
		fprintf(PrintPath,"        ifelse\n");
		fprintf(PrintPath,"      } { pop pop } ifelse\n");
		fprintf(PrintPath,"  } forall\n");
		fprintf(PrintPath,"  nouveau_dico /FontName /CourierAccentue put\n");
		fprintf(PrintPath,"  accents aload\n");
		fprintf(PrintPath,"  length 2 idiv { nouveau_dico /Encoding get 3 1 roll put } repeat\n");
		fprintf(PrintPath,"  /CourierAccentue nouveau_dico definefont pop\n");
		// fprintf(PrintPath,"end\n");
		fprintf(PrintPath,"\n");

		fprintf(PrintPath,"  /police_base2 /Courier-Oblique findfont def\n");
		fprintf(PrintPath,"  /nouveau_dico2 police_base2 maxlength dict def\n");
		fprintf(PrintPath,"  police_base2 {\n");
		fprintf(PrintPath,"    exch dup /FID ne\n");
		fprintf(PrintPath,"      { dup /Encoding eq\n");
		fprintf(PrintPath,"        { exch dup length array copy\n");
		fprintf(PrintPath,"          nouveau_dico2 3 1 roll put }\n");
		fprintf(PrintPath,"        { exch nouveau_dico2 3 1 roll put }\n");
		fprintf(PrintPath,"        ifelse\n");
		fprintf(PrintPath,"      } { pop pop } ifelse\n");
		fprintf(PrintPath,"  } forall\n");
		fprintf(PrintPath,"  nouveau_dico2 /FontName /CourierItalique put\n");
		fprintf(PrintPath,"  accents aload\n");
		fprintf(PrintPath,"  length 2 idiv { nouveau_dico2 /Encoding get 3 1 roll put } repeat\n");
		fprintf(PrintPath,"  /CourierItalique nouveau_dico2 definefont pop\n");
		fprintf(PrintPath,"\n");

		fprintf(PrintPath,"  /police_base3 /Courier-Bold findfont def\n");
		fprintf(PrintPath,"  /nouveau_dico3 police_base3 maxlength dict def\n");
		fprintf(PrintPath,"  police_base3 {\n");
		fprintf(PrintPath,"    exch dup /FID ne\n");
		fprintf(PrintPath,"      { dup /Encoding eq\n");
		fprintf(PrintPath,"        { exch dup length array copy\n");
		fprintf(PrintPath,"          nouveau_dico3 3 1 roll put }\n");
		fprintf(PrintPath,"        { exch nouveau_dico3 3 1 roll put }\n");
		fprintf(PrintPath,"        ifelse\n");
		fprintf(PrintPath,"      } { pop pop } ifelse\n");
		fprintf(PrintPath,"  } forall\n");
		fprintf(PrintPath,"  nouveau_dico3 /FontName /CourierGras put\n");
		fprintf(PrintPath,"  accents aload\n");
		fprintf(PrintPath,"  length 2 idiv { nouveau_dico3 /Encoding get 3 1 roll put } repeat\n");
		fprintf(PrintPath,"  /CourierGras nouveau_dico3 definefont pop\n");
		fprintf(PrintPath,"\n");

		fprintf(PrintPath,"/CourierAccentue findfont %d scalefont setfont\n",ImpressionTaille);
		if(!ImpressionPortrait) fprintf(PrintPath,"[0 -1 1 0 0 %d] concat\n",A4_TRANSL);
		fprintf(PrintPath,"/epaisseur currentlinewidth store\n");
		fprintf(PrintPath,"/marge_x 5 store\n");
		fprintf(PrintPath,"%%/hauteur { (fg) false charpath flattenpath pathbbox exch pop exch sub exch pop } store\n");
		fprintf(PrintPath,"/hauteur %d store\n",ImpressionInterligne);
		fprintf(PrintPath,"/largeur (a) stringwidth pop store\n\n");
		// fprintf(PrintPath,"/imprime { 32 string cvs show } def\n\n");
		fprintf(PrintPath,"/surligne {\n");
		fprintf(PrintPath,"	store\n");
		fprintf(PrintPath,"	/dx nb largeur mul store\n");
		//fprintf(PrintPath,"	/dy hauteur lig mul 1 sub store\n");
		fprintf(PrintPath,"	/dy hauteur lig mul store\n");
		fprintf(PrintPath,"	/xc currentpoint pop store\n");
		fprintf(PrintPath,"	/yc currentpoint exch pop store\n");
		fprintf(PrintPath,"	newpath\n");
		fprintf(PrintPath,"	xc yc dy add 2 sub moveto\n");
		fprintf(PrintPath,"	xc dx add yc dy add 2 sub lineto\n");
		fprintf(PrintPath,"	xc dx add yc 2 sub lineto\n");
		fprintf(PrintPath,"	xc yc 2 sub lineto\n");
		fprintf(PrintPath,"	fill\n");
		fprintf(PrintPath,"	closepath\n");
		fprintf(PrintPath,"	0 0 0 setrgbcolor\n");
		fprintf(PrintPath,"	xc yc moveto\n");
		fprintf(PrintPath,"} def\n\n");

		fprintf(PrintPath,"marge_x %d hauteur %d mul sub moveto\n",ImpressionHautFeuille,++ImpressionNumLigne);
		fprintf(PrintPath,"/haut_col currentpoint exch pop hauteur add store\n");
		//+ WndPSstroked = 0;
	} else if(ImprTex.active) {
		// A convertir via "pdflatex <nom>.tex" (rend <nom>.pdf)
		fprintf(PrintPath,"\\documentclass[12pt, a4paper, twoside]{article}\n");
		// if(!ImpressionPortrait) fprintf(PrintPath,"\\usepackage {lscape}\n");
		if(!ImpressionPortrait) fprintf(PrintPath,"\\usepackage[landscape]{geometry}\n");
		fprintf(PrintPath,"\\usepackage[utf8]{inputenc}\n");
		// fprintf(PrintPath,"\\usepackage {xcolor}\n");
		fprintf(PrintPath,"\\usepackage[table]{xcolor}\n");
		fprintf(PrintPath,"\\definecolor {blanc}     {RGB}{255, 255, 255}\n");
		fprintf(PrintPath,"\\definecolor {noir}      {RGB}{0, 0, 0}\n");
		fprintf(PrintPath,"\\definecolor {bleufonce} {RGB}{63, 63, 255}\n");
		fprintf(PrintPath,"\\definecolor {bleuclair} {RGB}{227, 227, 255}\n");
		fprintf(PrintPath,"\n");
	}
	return(1);
}
/* tentatives infructueuses:

 %/pt0 currentpoint store
 %/pt1 currentpoint store

 %(/pt1=\() show
 %pt1 pop texte cvs show
 %(\)) show

 %newpath
 %pt1 moveto
 %pt1 pt2 pop exch lineto
 %pt2 lineto
 %pt1 pt2 exch pop exch pop lineto
 %closepath

 */
/* ========================================================================== */
void ImprimeCadreFormatte(IMPRESSION_FORMAT fmt) {
	ImprCadresFmt = fmt;
	ImprCadresHtml = ((fmt == IMPRESSION_HTML) || (fmt == IMPRESSION_HTTP));
}
/* ========================================================================== */
void ImprimeCadreTexte(char *texte) {
	char traduit[8192]; char *ptr;
	if(ImprCadresHtml) {
		ImprimeToHtml(traduit,texte,8192); ptr = traduit;
	} else ptr = texte;
	if(ImprCadresFmt == IMPRESSION_HTTP) HttpEcrit(ptr);
	else fputs(ptr,ImprCadreDest);
}
/* ========================================================================== */
void ImprimeCadreEcrit(char *fmt, ...) {
	char texte[MAX_LIGNE]; va_list argptr;

	va_start(argptr, fmt);
	vsnprintf(texte, MAX_LIGNE, fmt, argptr);
	va_end(argptr);
	ImprimeCadreTexte(texte);
}
/* ========================================================================== */
void ImprimeCadreOuvre(char *style, char *fmt, ...) {
	char texte_brut[MAX_LIGNE],texte_html[MAX_LIGNE],*legende; int l,max;
	va_list argptr; int nb;

	if(ImprCadresOuverts < 0) ImprCadresOuverts = 0;
//	if(ImprCadresFmt == IMPRESSION_TEXTE) ImprCadreDest = stdout; else ImprCadreDest = PrintPath;
	if(fmt) {
		if(ImprCadresHtml) {
			if(style && style[0]) l = sprintf(texte_brut,"<fieldset style=\"%s\"><legend>",style);
			else strcpy(texte_brut,"<fieldset><legend>"); l = (int)strlen(texte_brut);
			legende = texte_html;
			max = MAX_LIGNE - l - 10;
		} else {
			legende = texte_brut;
			max = MAX_LIGNE;
			l = 0;
		}
		va_start(argptr, fmt);
		nb = vsnprintf(legende, max, fmt, argptr);
		va_end(argptr);
		if(ImprCadresHtml) {
			ImprimeToHtml(texte_brut+l,legende,MAX_LIGNE-l);
			strcat(texte_brut,"</legend>\n");
			// printf("(%s) Ouvre le cadre #%d dans %08lX\n",__func__,ImprCadresOuverts+1,(Ulong)ImprCadreDest);
			ImprimeCadreTexte(texte_brut);
		/* ne rien changer, surtout pas ecrire sur le postscript!
		} else if(ImprPs.active) {
			ImprCadreDest = stdout; */
		} else {
			// printf("(%s) Ouvre le cadre #%d dans %08lX\n",__func__,ImprCadresOuverts+1,(Ulong)ImprCadreDest);
			ImprimeCadreLigneDebut(1);
			fprintf(ImprCadreDest," _ %s __________\n",texte_brut);
		}
	} else if(ImprCadresHtml) {
		if(style && style[0]) {
			strcat(strcat(strcpy(texte_brut,"<fieldset style=\""),style),"\">\n");
			ImprimeCadreTexte(texte_brut);
		} else ImprimeCadreTexte("<fieldset>\n");
	} else {
		ImprimeCadreLigneDebut(1);
		fputs("\n",ImprCadreDest);
		for(l=0; l<ImprCadresOuverts; l++) fputs("| ",ImprCadreDest);
		fprintf(ImprCadreDest," ________________\n");
	}
	ImprCadresOuverts++;
}
/* ========================================================================== */
void ImprimeCadreLigneDebut(int n) {
	int i,l;

	// printf("(%s) Demarre %d nouvelle%s ligne%s dans %08lX\n",__func__,Accord2s(n),(Ulong)ImprCadreDest);
	for(i=0; i<n; i++) {
		if(ImprCadresHtml) ImprimeCadreTexte("<br>");
		else {
			for(l=0; l<ImprCadresOuverts; l++) ImprimeCadreTexte("| ");
			if(i < (n - 1)) ImprimeCadreTexte("\n");
		}
	}
}
/* ========================================================================== */
void ImprimeCadreLigne(char *texte) {
	ImprimeCadreLigneDebut(1);
	ImprimeCadreTexte(texte);
	ImprimeCadreTexte("\n"); // == ImprimeCadreLigneFin()
}
/* ========================================================================== */
void ImprimeCadreLigneFin(void) { ImprimeCadreTexte("\n"); }
/* ========================================================================== */
void ImprimeCadreFerme(void) {
	// printf("(%s) Ferme le cadre #%d dans %08lX\n",__func__,ImprCadresOuverts,(Ulong)ImprCadreDest);
	if(ImprCadresOuverts) {
		--ImprCadresOuverts;
		if(ImprCadresHtml) ImprimeCadreTexte("</fieldset>\n");
		else {
			int l; for(l=0; l<ImprCadresOuverts; l++) fputs("| ",ImprCadreDest);
			fprintf(ImprCadreDest,"|______________________________\n");
		}
	}
}
/* ========================================================================== */
void ImprimeWebTableDesign(int epaisseur, int largeur, int pad, int espace) {
	ImprWeb.epaisseur = epaisseur;
	ImprWeb.largeur = largeur;
	ImprWeb.pad = pad;
	ImprWeb.espace = espace;
}
/* ========================================================================== */
void ImpressionHtmlDebut(char *titre, int couleur) {
	// printf("Content-type: text/html\n\n");
	printf("<html>\n");
	printf("<head><title>%s</title></head>\n",titre);
	printf("<body bgcolor='#%06X'>\n",couleur);
}
/* ========================================================================== */
void ImpressionHtmlFin(void) {
	printf("</body>\n");
	printf("</html>\n");
}
/* ========================================================================== */
static inline void ImprimeWeb(char *fmt, ...) {
	char texte[MAX_LIGNE];
	va_list argptr; int nb;

	va_start(argptr, fmt);
	nb = vsnprintf(texte, MAX_LIGNE, fmt, argptr);
	va_end(argptr);

	char *ptr;
	if(ImprCadresHtml) {
		char traduit[8192];
		ImprimeToHtml(traduit,texte,8192); ptr = traduit;
	} else ptr = texte;
	if(ImpressionFormat == IMPRESSION_HTTP) HttpEcrit(ptr);
	else fputs(ptr,PrintPath);
}
/* ========================================================================== */
static void ImprimeWebTableDebut(void) {
	ImprimeWeb("<table style=\"width=%d; text-align: left\" border=%d cellpadding=%d cellspacing=%d>\n",
		ImprWeb.largeur,ImprWeb.epaisseur,ImprWeb.pad,ImprWeb.espace);
	ImprWeb.tables_ouvertes++;
}
/* ========================================================================== */
static void ImprimeWebTableFin(void) {
	if(ImprWeb.tables_ouvertes) { ImprimeWeb("</table>\n"); --ImprWeb.tables_ouvertes; }
}
/* ========================================================================== */
static void ImprimeVideLigne(void) {
	int l,i;

	if(ImprWeb.active) {
		if(ImprWeb.tables_ouvertes && (ImprTraitVert[0].col >= 0)) {
			char texte[80];
			// if(ImprWeb.active) fprintf(PrintPath,"<!-- ImprimeVideLigne -->");
			fprintf(PrintPath,"<!-- ImprimeVideLigne: ligne de tableau dans %s avec fond %06X -->\n",__func__,ImprWeb.bgnd);
			if(ImprWeb.bgnd == 0xFFFFFF) strcpy(ImpressionLigne,"<tr>");
			else sprintf(ImpressionLigne,"<tr bgcolor='#%06X'>",ImprWeb.bgnd);
			i = 0; while(ImprTraitVert[i+1].col >= 0) {
				strcat(ImpressionLigne,"<td");
				if(ImprTraitVert[i].aligne == IMPRESSION_CENTER) strcat(ImpressionLigne," align=center");
				else if(ImprTraitVert[i].aligne == IMPRESSION_RIGHT) strcat(ImpressionLigne," align=right");
				if(ImprTraitVert[i].largr > 0) /* ImprTraitVert[i].largr = ImprTraitVert[i].col - ImprTraitVert[i-1].col */ {
					sprintf(texte,"%d",ImprTraitVert[i].largr * 7);
					strcat(strcat(ImpressionLigne," width="),texte);
				} else if(ImprTraitVert[i].largr < 0) {
					sprintf(texte,"%d %%%%",-ImprTraitVert[i].largr);
					strcat(strcat(ImpressionLigne," width="),texte);
				}
				strcat(ImpressionLigne,">");
				strcat(ImpressionLigne,"</td>");
				i++;
			}
			strcat(ImpressionLigne,"</tr>\n");
		} else {
			//?? strcpy(ImpressionLigne,"<br>");
			l = sizeof(ImpressionLigne); while(l--) ImpressionLigne[l] = '\0';
		}
	} else {
		l = sizeof(ImpressionLigne); while(l--) ImpressionLigne[l] = ' ';
		if(!ImprPs.active) {
			i = 0; while(ImprTraitVert[i].col >= 0) ImpressionLigne[ImprTraitVert[i++].col] = '|';
		}
	}
}
/* ========================================================================== */
void ImpressionStockePoint(char *nom) {
	if(!ImprPs.active) return;
	fprintf(PrintPath,"/%sx currentpoint pop store\n",nom);
	fprintf(PrintPath,"/%sy currentpoint exch pop store\n",nom);
}
/* ========================================================================== */
static void ImpressionTextePS(char *texte) {
	/* en fait, inutilise si pas PS */
	unsigned char *c;

	if(*texte == '\0') return;
	if(ImprPs.active)  {
		fprintf(PrintPath,"(");
		c = (unsigned char *)texte;
		while(*c) {
			if((*c == '(') || (*c == ')')) fprintf(PrintPath,"\\%c",*c);
		#ifdef ACCENTS_PS_SUPPRIMES
			else if(*c <= 0x7E) fprintf(PrintPath,"%c",*c);
			else {
				switch(*c) {
					case 0x8d: fprintf(PrintPath,"c"); break; /* 'ç' */
					case 0x8e: fprintf(PrintPath,"e"); break; /* 'é' */
					case 0x8f: fprintf(PrintPath,"e"); break; /* 'è' */
					case 0xb0: fprintf(PrintPath,"o"); break; /* 'º' */
					case 0xe0: fprintf(PrintPath,"a"); break; /* 'à' */
					case 0xe2: fprintf(PrintPath,"a"); break; /* 'â' */
					case 0xe7: fprintf(PrintPath,"c"); break; /* 'ç' */
					case 0xe8: fprintf(PrintPath,"e"); break; /* 'è' */
					case 0xe9: fprintf(PrintPath,"e"); break; /* 'é' */
					case 0xea: fprintf(PrintPath,"e"); break; /* 'ê' */
					case 0xeb: fprintf(PrintPath,"e"); break; /* 'ë' */
					case 0xee: fprintf(PrintPath,"i"); break; /* 'î' */
					case 0xef: fprintf(PrintPath,"i"); break; /* 'ï' */
					case 0xf4: fprintf(PrintPath,"o"); break; /* 'ô' */
					case 0xf9: fprintf(PrintPath,"u"); break; /* 'ù' */
					case 0xfb: fprintf(PrintPath,"u"); break; /* 'û' */
					default: fprintf(PrintPath," ");
				}
				// printf("'%c' = 0x%02X\n",*c,*c);
			}
		#else  /* ACCENTS_PS_SUPPRIMES */
			else fprintf(PrintPath,"%c",*c);
		#endif /* ACCENTS_PS_SUPPRIMES */
			c++;
		}
		fprintf(PrintPath,") show\n");
	} else if(ImprWeb.active) {
		ImprimeWeb("<br>%s",texte); // <p>
	} else {
#ifdef OPIUM_H
		if(ImpressionSupport == IMPRESSION_TERMINAL) TermPrint(tImpression,texte);
		else
#endif /* OPIUM_H */
			fputs(texte,PrintPath);
	}
}
/* ========================================================================== */
char ImprimeGroupeLignes(int nb) {
	if(((ImpressionNumLigne + nb) >= ImpressionMaxLignes) && !ImprWeb.active) {
		ImprimeSautPage();
		return(0);
	}
	return(1);
}
/* ========================================================================== */
char ImpressionPageCommencee(void) {
	int l,i,k;
	if(ImpressionNumLigne == 1) {
		l = sizeof(ImpressionLigne);
		i = 0;
		for(k=0; k<l; k++) {
			if(ImpressionLigne[k] == '\0') break;
			if(!ImprPs.active && (k == ImprTraitVert[i].col)) {
				if(ImpressionLigne[k] != '|') break;
				i++;
			} else if(ImpressionLigne[l] != ' ') break;
		}
		if(k >= l) return(0);
		else return((ImpressionLigne[k] != '\0'));
	} else return(1);
}
/* ========================================================================== */
void ImprimeFinLigne(void) {
	int l;

	if(ImprWeb.active) fprintf(PrintPath,"<!-- ImprimeFinLigne -->");
//	if(!ImprWeb.active) {
		l = sizeof(ImpressionLigne);
		//-	if(ImprWeb.active) fprintf(PrintPath,"<!-- dernier caractere: #%d -->",l);
		while(l--) if((ImpressionLigne[l] != '\0') && (ImpressionLigne[l] != ' ')) break;
		//- if(ImprWeb.active) fprintf(PrintPath,"<!-- premier caractere: #%d -->",l);
		ImpressionLigne[l+1] = '\0';
//	}
	if(ImprPs.active) {
		ImpressionTextePS(ImpressionLigne); // fprintf(PrintPath,"(%s) show\n",ImpressionLigne);
		ImprimeVideLigne();
		if(ImpressionNumLigne >= ImpressionMaxLignes) ImprimeSautPage();
		else fprintf(PrintPath,"marge_x %d hauteur %d mul sub moveto\n",ImpressionHautFeuille,++ImpressionNumLigne);
	} else {
		if(ImpressionFormat == IMPRESSION_HTTP) HttpEcrit("%s\n",ImpressionLigne);
	#ifdef OPIUM_H
		else if(ImpressionSupport == IMPRESSION_TERMINAL) {
			TermPrint(tImpression,"%s\n",ImpressionLigne);
			ImprimeVideLigne();
			return;
		}
	#endif /* OPIUM_H */
		else fprintf(PrintPath,"%s\n",ImpressionLigne);
		ImprimeVideLigne();
		if(ImpressionNumLigne >= ImpressionMaxLignes) ImprimeSautPage();
		else ImpressionNumLigne++;
	}
	return;
}
/* ========================================================================== */
void ImprimeFond(int col, int nb, unsigned short r, unsigned short g, unsigned short b) {
	if(ImprPs.active) {
		fprintf(PrintPath,"marge_x %d largeur mul add %d hauteur %d mul sub moveto\n",col,ImpressionHautFeuille,ImpressionNumLigne);
		fprintf(PrintPath,"%g %g %g setrgbcolor /lig 1 store /nb %d surligne\n",
			(float)r / 65535.0,(float)g / 65535.0,(float)b / 65535.0, nb);
		fprintf(PrintPath,"marge_x %d hauteur %d mul sub moveto\n",ImpressionHautFeuille,ImpressionNumLigne);
	} else if(ImprWeb.active) {
		ImprWeb.bgnd = (((unsigned int)r & 0xFF00) << 8) | ((unsigned int)g & 0xFF00) | ((unsigned int)b >> 8);
		// fprintf(PrintPath,"<!-- ImprimeFond demande: (%04X %04X %04X), soit RGB=%06X -->\n",r,g,b,ImprWeb.bgnd);
		if(!strncmp(ImpressionLigne,"<tr",3)) {
			int i,j,k; char tempo[MAX_LIGNE];
			// char ligne[MAX_LIGNE];
			// strcpy(ligne,ImpressionLigne);
			k = -1; i = 3; j = 0;
			while(ImpressionLigne[i]) {
				if((k < 0) && ImpressionLigne[i] == '>') j = k = i;
				else if((k < 0) && !strncmp(ImpressionLigne+i," bgcolor='#",11)) {
					k = i;
					j = k + 18;
				}
				i++;
			}
			if(!j) { j = k = i; }
			strcpy(tempo,ImpressionLigne+j); // tempo[i - j] = '\0';
			k += sprintf(ImpressionLigne+k," bgcolor='#%06X'",ImprWeb.bgnd);
			strcpy(ImpressionLigne+k,tempo);
		}
	}
}
/* ========================================================================== */
void ImprimeNFonds(int col, int nb, int lig, unsigned short r, unsigned short g, unsigned short b) {
	if(!ImprPs.active) return;
	fprintf(PrintPath,"marge_x %d largeur mul add %d hauteur %d mul sub moveto\n",col,ImpressionHautFeuille,lig * ImpressionNumLigne);
	fprintf(PrintPath,"%g %g %g setrgbcolor /lig %d store /nb %d surligne\n",
			(float)r / 65535.0,(float)g / 65535.0,(float)b / 65535.0, lig,nb);
	fprintf(PrintPath,"marge_x %d hauteur %d mul sub moveto\n",ImpressionHautFeuille,ImpressionNumLigne);
}
/* ========================================================================== */
void ImprimeCouleur(unsigned short r, unsigned short g, unsigned short b) {
	if(ImprPs.active) fprintf(PrintPath,"%g %g %g setrgbcolor\n",
		(float)r / 65535.0,(float)g / 65535.0,(float)b / 65535.0);
	else if(ImprWeb.active)
		ImpressionFontColor = (((unsigned int)r & 0xFF00) << 8) | ((unsigned int)g & 0xFF00) | ((unsigned int)b >> 8);
	// if(ImprWeb.active) fprintf(PrintPath,"<!-- ImprimeCouleur demande: (%04x %04X %04X), soit RGB=%06X -->\n",r,g,b,ImpressionFontColor);
}
/* ========================================================================== */
void ImprimeStyle(IMPRESSION_STYLE style) {
	int l;

	if(style >= IMPRESSION_STYLE_NB) style = IMPRESSION_REGULIER;

	if(ImprPs.active) {
		l = sizeof(ImpressionLigne); while(l--) if(ImpressionLigne[l] != ' ') break;
		if(l >= 0) {
			ImpressionLigne[l+1] = '\0';
			ImpressionTextePS(ImpressionLigne);
			ImprimeVideLigne();
			fprintf(PrintPath,"marge_x %d hauteur %d mul sub moveto\n",ImpressionHautFeuille,ImpressionNumLigne);
		}
		switch(style) {
			case IMPRESSION_ITALIQUE:
				fprintf(PrintPath,"/CourierItalique findfont %d scalefont setfont\n",ImpressionTaille);
				break;
			case IMPRESSION_GRAS:
				fprintf(PrintPath,"/CourierGras findfont %d scalefont setfont\n",ImpressionTaille);
				break;
			default:
				fprintf(PrintPath,"/CourierAccentue findfont %d scalefont setfont\n",ImpressionTaille);
		}
	} else if(ImprWeb.active) {
		ImpressionStyleCourant = ImpressionStylesTag[style];
	}
}
/* ========================================================================== */
void ImprimeNouvelleLigne(int nb) {
	int reste = nb; while(reste--) {
		ImprimeFinLigne();
		if(ImprWeb.active && (ImprTraitVert[0].col < 0)) {
			if(ImpressionFormat == IMPRESSION_HTTP) HttpEcrit("<br>\n");
			else fputs("<br>\n",PrintPath);
		}
	}
}
/* ========================================================================== */
void ImprimeSautPage(void) {
	int i; char a_tracer;

	// fprintf(PrintPath,"(<%d>) show\n",ImpressionNumLigne);
	a_tracer = (ImpressionNumLigne > 1);
	ImpressionNumLigne = 1;
	if(ImprPs.active) {
		if((ImprTraitVert[0].col >= 0) && a_tracer) {
			fprintf(PrintPath,"/xc currentpoint pop store\n");
			fprintf(PrintPath,"/bas_col currentpoint exch pop store\n");
			i = 0;
			while(ImprTraitVert[i].col >= 0) {
				fprintf(PrintPath,"newpath ");
				fprintf(PrintPath,"marge_x %d largeur mul add haut_col moveto ",ImprTraitVert[i].col);
				fprintf(PrintPath,"marge_x %d largeur mul add bas_col lineto ",ImprTraitVert[i].col);
				fprintf(PrintPath,"stroke closepath\n");
				fprintf(PrintPath,"xc bas_col moveto\n");
				i++;
			}
		}
		fprintf(PrintPath,"stroke\n");
		fprintf(PrintPath,"showpage\n");
		if(!ImpressionPortrait) fprintf(PrintPath,"[0 -1 1 0 0 %d] concat\n",A4_TRANSL);
		fprintf(PrintPath,"marge_x %d hauteur %d mul sub moveto\n",ImpressionHautFeuille,ImpressionNumLigne);
		if(ImprTraitVert[0].col >= 0) fprintf(PrintPath,"/haut_col currentpoint exch pop hauteur add store\n");
//?	} else if(ImprWeb.active) {
//?		ImprimeWeb("<hr size=4>\n");
	} else if(!ImprWeb.active) {
#ifdef OPIUM_H
		if(ImpressionSupport == IMPRESSION_TERMINAL)
			TermPrint(tImpression,"\n--------------------------------- Saut de page ---------------------------------\n\n");
		else
#endif /* OPIUM_H */
			/* if(ImpressionSupport == IMPRESSION_PAPIER) */ fprintf(PrintPath,"\f");
	}

	// ImprimeNouvelleLigne(1);
}
/* ========================================================================== */
void ImprimeNouvellePage(void) { ImprimeFinLigne(); ImprimeSautPage(); }
/* ========================================================================== */
void ImpressionPlaceColonne(int *trait, int largeur, int *col, int *fin) {
	int marge;

	marge = (ImprPs.active)? 1: ((ImprWeb.active)? 0: 2);
	*trait = *fin;
	*col = *trait + marge;
	*fin = *col + largeur + 1;
}
/* ========================================================================== */
void ImprimeColonnes(int nb, ...) {
	va_list argptr; int i,m; int col;

	if(ImprTraitVert[0].col >= 0) {
		if(ImprPs.active) {
			fprintf(PrintPath,"/xc currentpoint pop store\n");
			fprintf(PrintPath,"/bas_col currentpoint exch pop store\n");
			fprintf(PrintPath,"%d setlinewidth\n",ImprPs.epaisseur);
			i = 0;
			while(ImprTraitVert[i].col >= 0) {
				fprintf(PrintPath,"newpath ");
				fprintf(PrintPath,"marge_x %d largeur mul add haut_col moveto ",ImprTraitVert[i].col);
				fprintf(PrintPath,"marge_x %d largeur mul add bas_col lineto ",ImprTraitVert[i].col);
				fprintf(PrintPath,"stroke closepath\n");
				fprintf(PrintPath,"xc bas_col moveto\n");
				i++;
			}
			fprintf(PrintPath,"epaisseur setlinewidth\n");
		} else if(ImprWeb.active) {
			printf("<!-- Fin de table demande dans %s -->\n",__func__);
			ImprimeWebTableFin();
		}
	}
	if(nb == 0) {
		int l;
		ImprTraitVert[0].col = -1;
		if(ImprWeb.active) { l = sizeof(ImpressionLigne); while(l--) ImpressionLigne[l] = ' '; }
		return;
	}
	m = nb; if(m > MAX_TRAITSVERT) m = MAX_TRAITSVERT;
	va_start(argptr, nb);
	i = 0; while(i < m) {
		col = va_arg(argptr,int);
		ImprimeTraitVertPlace(i,col);
		i++;
	}
	ImprimeTraitVertDernier(i);
	va_end(argptr);
	ImprimeColonnesHaut(i);
}
/* ========================================================================== */
void ImprimeColonneFormat(int k, IMPRESSION_ALIGNEMENT aligne, IMPRESSION_TYPELARGEUR mode, int val) {
	int i;

	i = ImprimeTraitVertNb();
	if((k >= 0) && (k < i)) {
		ImprTraitVert[k].aligne = (char)aligne;
		switch(mode) {
			case IMPRESSION_INCHANGEE: break;
			case IMPRESSION_LIBRE:     ImprTraitVert[k].largr = 0;    break;
			case IMPRESSION_NBCARS:    ImprTraitVert[k].largr = (char)val;  break;
			case IMPRESSION_POURCENT:  ImprTraitVert[k].largr = -(char)val; break;
		}
	}
}
/* ========================================================================== */
static void ImprimeColonnesHaut(int i) {
	if(ImprPs.active) {
		ImprimeLimite(ImprTraitVert[0].col,ImprTraitVert[i-1].col-ImprTraitVert[0].col,0);
		fprintf(PrintPath,"/haut_col currentpoint exch pop hauteur add store\n");
	} else if(ImprWeb.active) {
		ImprimeLesTraits(1);
		ImprimeVideLigne();
	} else {
		ImprimeLimite(ImprTraitVert[0].col,ImprTraitVert[i-1].col-ImprTraitVert[0].col,0);
		i = 0; while(ImprTraitVert[i].col >= 0) ImpressionLigne[ImprTraitVert[i++].col] = '|';
	}
}
/* ========================================================================== */
int ImprimeToHtml(char *dest, char *srce, int max) {
	unsigned char *c,*d; char *t; int i,n;

//-	if(strcpy(dest,srce)) return((int)strlen(dest));
	d = c = (unsigned char *)srce;
	t = dest;
	i = 0;
	while(*c) {
		if(*c > 0x7E) {
			if(c > d) { strncpy(t+i,(char *)d,c-d); i += (c - d); }
			n = max - i - 1;
			switch(*c) {
				case 0x80: strncpy(t+i,"&euro;",n); i += 7; break;
				case 0x82: strncpy(t+i,"&eacute;",n); i += 8; break;
				case 0x85: strncpy(t+i,"&agrave;",n); i += 8; break;
				case 0x88: strncpy(t+i,"&agrave;",n); i += 8; break;
				case 0x8a: strncpy(t+i,"&egrave;",n); i += 8; break;
				case 0x8d: strncpy(t+i,"&ccedil;",n); i += 8; break;
				case 0x8e: strncpy(t+i,"&eacute;",n); i += 8; break;
				case 0x8f: strncpy(t+i,"&egrave;",n); i += 8; break;
				case 0x90: strncpy(t+i,"&ecirc;",n);  i += 7; break;
				case 0x94: strncpy(t+i,"&icirc;",n);  i += 7; break;
				case 0x99: strncpy(t+i,"&ocirc;",n);  i += 7; break;
				case 0xb0: strncpy(t+i,"&ordm;",n);   i += 6; break;
				case 0xe0: strncpy(t+i,"&agrave;",n); i += 8; break;
				case 0xe2: strncpy(t+i,"&acirc;",n);  i += 7; break;
				case 0xe7: strncpy(t+i,"&ccedil;",n); i += 8; break;
				case 0xe8: strncpy(t+i,"&egrave;",n); i += 8; break;
				case 0xe9: strncpy(t+i,"&eacute;",n); i += 8; break;
				case 0xea: strncpy(t+i,"&ecirc;",n);  i += 7; break;
				case 0xeb: strncpy(t+i,"&euml;",n);   i += 6; break;
				case 0xee: strncpy(t+i,"&icirc;",n);  i += 7; break;
				case 0xef: strncpy(t+i,"&iuml;",n);   i += 6; break;
				case 0xf4: strncpy(t+i,"&ocirc;",n);  i += 7; break;
				case 0xf9: strncpy(t+i,"&ugrave;",n); i += 8; break;
				case 0xfb: strncpy(t+i,"&ucirc;",n);  i += 7; break;
				default:   strncpy(t+i,"_",n);        i += 1; break;
			}
			d = c + 1;
		}
		c++;
		if(i >= max) { i = max - 1; break; }
	}
	n = (int)(c - d); if(n > (max - i - 1)) n = max - i - 1;
	if(i < (max - 1)) { strncpy(t+i,(char *)d,n); i += n; }
	*(t+i) = '\0';

	return(i);
}
/* ========================================================================== */
static void ImprimeInsere(int col, char *texte) {
	int l;

	if(col < 0) col = 0; else if(col > (MAX_LIGNE - 2)) col = MAX_LIGNE - 2;
	l = (int)strlen(texte);
	if(ImprWeb.active) {
		int i,k; int zone,bloc; char marque,quote; char tempo[MAX_LIGNE];
	#ifdef DEBUG_INSERE
		printf("<!-- '%s'@%d/ ",texte,col);
		zone = 0; while(ImprTraitVert[zone].col >= 0) {
			printf("[%d]:%d ",zone,ImprTraitVert[zone].col);
			zone++;
		}
		printf(" -->\n");
	#endif
		zone = 0;
		if(ImprTraitVert[0].col >= 0) {
			while(ImprTraitVert[zone].col >= 0) { if((col +  1) < ImprTraitVert[zone].col) break; zone++; }
		}
	#ifdef DEBUG_INSERE
		printf("<!-- zone visee: %d -->\n",zone);
	#endif
		i = 0; bloc = 0; k = 0; marque = quote = 0;
		while(ImpressionLigne[i] && (marque || (k < (col + 1)))) {
	#ifdef DEBUG_INSERE
			printf("<!-- caractere #%d: %c -->\n",i,ImpressionLigne[i]);
	#endif
			if(quote) {
				k++;
				if(ImpressionLigne[i] == '\"') quote = 0;
			} else if(marque) {
				if(ImpressionLigne[i] == '>') {
					marque = 0;
				#ifdef DEBUG_INSERE
					printf("<!-- fin de marque: '...%s' -->\n",ImpressionLigne+i);
				#endif
				}
			} else if(ImpressionLigne[i] == '<') {
				if(!strncmp(ImpressionLigne+i,"<td>",4)) {
					k = ImprTraitVert[bloc++].col; i += 3;
				#ifdef DEBUG_INSERE
					printf("<!-- entree en zone %d (soit col. %d): '...%s' -->\n",bloc,k,ImpressionLigne+i-3);
				#endif
				} else if(!strncmp(ImpressionLigne+i,"</td>",5)) {
				#ifdef DEBUG_INSERE
					printf("<!-- sortie de zone %d (%s): '...%s' -->\n",bloc,(bloc == zone)?"termine":"a suivre",ImpressionLigne+i);
				#endif
					if(bloc == zone) break; else i += 4;
				} else if(!strncmp(ImpressionLigne+i,"<td ",4)) {
					k = ImprTraitVert[bloc++].col; i += 3; marque = 1;
				#ifdef DEBUG_INSERE
					printf("<!-- entree en zone %d (soit col. %d): '...%s' (fin de marque attendue) -->\n",bloc,k,ImpressionLigne+i-3);
				#endif
				} else marque = 1;
			} else if(ImpressionLigne[i] == '&') {
				k++;
				if(!strncmp(ImpressionLigne+i,"&ccedil;",8)) i += 7;
				else if(!strncmp(ImpressionLigne+i,"&agrave;",8)) i += 7;
				else if(!strncmp(ImpressionLigne+i,"&acirc;",7)) i += 6;
				else if(!strncmp(ImpressionLigne+i,"&egrave;",8)) i += 7;
				else if(!strncmp(ImpressionLigne+i,"&eacute;",8)) i += 7;
				else if(!strncmp(ImpressionLigne+i,"&ecirc;",7)) i += 6;
				else if(!strncmp(ImpressionLigne+i,"&ordm;",6)) i += 5;
				else if(!strncmp(ImpressionLigne+i,"&euml;",6)) i += 5;
				else if(!strncmp(ImpressionLigne+i,"&icirc;",7)) i += 6;
				else if(!strncmp(ImpressionLigne+i,"&iuml;",6)) i += 5;
				else if(!strncmp(ImpressionLigne+i,"&ocirc;",7)) i += 6;
				else if(!strncmp(ImpressionLigne+i,"&ugrave;",8)) i += 7;
				else if(!strncmp(ImpressionLigne+i,"&ucirc;",7)) i += 6;
				else if(!strncmp(ImpressionLigne+i,"&euro;",6)) i += 5;
			} else {
				k++;
				if(ImpressionLigne[i] == '\"') quote = 1;
			}
			i++;
		}
		if(ImpressionLigne[i]) {
		#define ACCENTS_HTML_SUPPRIMES
		#ifdef ACCENTS_HTML_V1
			unsigned char *c,*d;
		#endif
			strcpy(tempo,ImpressionLigne+i);
			while(k++ < col) ImpressionLigne[i++] = ' ';
			if(ImpressionStyleCourant) i += sprintf(ImpressionLigne+i,"<%c>",ImpressionStyleCourant);
			if(ImpressionFontColor) i += sprintf(ImpressionLigne+i,"<font color='#%06X'>",ImpressionFontColor);
		#ifdef ACCENTS_HTML_SUPPRIMES
			#ifdef ACCENTS_HTML_V1
			d = c = (unsigned char *)texte;
			while(*c) {
				if(*c > 0x7E) {
					strncpy(ImpressionLigne+i,(char *)d,c-d); i += (c - d);
					switch(*c) {
						case 0x80: strcpy(ImpressionLigne+i,"&euro;"); i += 7; break;
						case 0x82: strcpy(ImpressionLigne+i,"&eacute;"); i += 8; break;
						case 0x85: strcpy(ImpressionLigne+i,"&agrave;"); i += 8; break;
						case 0x88: strcpy(ImpressionLigne+i,"&agrave;"); i += 8; break;
						case 0x8a: strcpy(ImpressionLigne+i,"&egrave;"); i += 8; break;
						case 0x8d: strcpy(ImpressionLigne+i,"&ccedil;"); i += 8; break;
						case 0x8e: strcpy(ImpressionLigne+i,"&eacute;"); i += 8; break;
						case 0x8f: strcpy(ImpressionLigne+i,"&egrave;"); i += 8; break;
						case 0x90: strcpy(ImpressionLigne+i,"&ecirc;");  i += 7; break;
						case 0x94: strcpy(ImpressionLigne+i,"&icirc;");  i += 7; break;
						case 0x99: strcpy(ImpressionLigne+i,"&ocirc;");  i += 7; break;
						case 0xb0: strcpy(ImpressionLigne+i,"&ordm;");   i += 6; break;
						case 0xe0: strcpy(ImpressionLigne+i,"&agrave;"); i += 8; break;
						case 0xe2: strcpy(ImpressionLigne+i,"&acirc;");  i += 7; break;
						case 0xe7: strcpy(ImpressionLigne+i,"&ccedil;"); i += 8; break;
						case 0xe8: strcpy(ImpressionLigne+i,"&egrave;"); i += 8; break;
						case 0xe9: strcpy(ImpressionLigne+i,"&eacute;"); i += 8; break;
						case 0xea: strcpy(ImpressionLigne+i,"&ecirc;");  i += 7; break;
						case 0xeb: strcpy(ImpressionLigne+i,"&euml;");   i += 6; break;
						case 0xee: strcpy(ImpressionLigne+i,"&icirc;");  i += 7; break;
						case 0xef: strcpy(ImpressionLigne+i,"&iuml;");   i += 6; break;
						case 0xf4: strcpy(ImpressionLigne+i,"&ocirc;");  i += 7; break;
						case 0xf9: strcpy(ImpressionLigne+i,"&ugrave;"); i += 8; break;
						case 0xfb: strcpy(ImpressionLigne+i,"&ucirc;");  i += 7; break;
						default: strcpy(ImpressionLigne+i,"_"); i += 1; break;
					}
					d = c + 1;
				}
				c++;
			}
			strncpy(ImpressionLigne+i,(char *)d,c-d); i += (c - d);
			#else /* !ACCENTS_HTML_V1 */
			i += ImprimeToHtml(ImpressionLigne+i,texte,MAX_LIGNE-i);
			#endif /* !ACCENTS_HTML_V1 */
		#else /* !ACCENTS_HTML_SUPPRIMES */
			strcpy(ImpressionLigne+i,texte); i += l;
		#endif /* ACCENTS_HTML_SUPPRIMES */
			if(ImpressionFontColor) i += sprintf(ImpressionLigne+i,"</font>");
			if(ImpressionStyleCourant) i += sprintf(ImpressionLigne+i,"</%c>",ImpressionStyleCourant);
			strcpy(ImpressionLigne+i,tempo);
		} else {
			while(k++ < col) ImpressionLigne[i++] = ' ';
			strcpy(ImpressionLigne+i,texte);
		}
	} else {
		if(l >= (MAX_LIGNE - col)) { l = MAX_LIGNE - col - 1; ImpressionLigne[col + l] = '\0'; }
		memcpy(ImpressionLigne+col,texte,l);
	}
}
/* ========================================================================== */
int ImprimeEnCol(int col, char *fmt, ...) {
	va_list argptr; int cnt; char texte[256];

	va_start(argptr, fmt);
	cnt = vsprintf(texte, fmt, argptr);
	va_end(argptr);

	ImprimeInsere(col,texte);
	return((int)strlen(texte));
}
/* ========================================================================== */
void ImprimeLesTraits(int trait) {
	if(ImprWeb.active) {
		int epaisseur = ImprWeb.epaisseur;
		printf("<!-- Fin de table demande dans %s -->\n",__func__);
		ImprimeWebTableFin();
		ImprWeb.epaisseur = trait;
		printf("<!-- Debut de table demande dans %s -->\n",__func__);
		ImprimeWebTableDebut();
		ImprWeb.epaisseur = epaisseur;
	}
	//++ ImprWeb.epaisseur = !trait; if(ImprWeb.active) ImprimeWebTableDebut();
}
/* ========================================================================== */
void ImprimeTrait(int col, int nb, int epais) {
	char trait; int i,k,m;

	// ImprimeNouvelleLigne(1);

	if(ImprPs.active) {
		fprintf(PrintPath,"/xc currentpoint pop store\n");
		fprintf(PrintPath,"/yc currentpoint exch pop store\n");
		fprintf(PrintPath,"/y %d hauteur %d.3 mul sub store\n",ImpressionHautFeuille,ImpressionNumLigne-1);
		m = col + nb;
		// printf("(%s) %d .. %d\n",__func__,col,m);
		fprintf(PrintPath,"newpath %d setlinewidth marge_x largeur %d mul add y moveto marge_x largeur %d mul add y lineto stroke closepath\n",epais,col,m);
		fprintf(PrintPath,"epaisseur setlinewidth xc yc moveto\n");
	} else if(ImprWeb.active) {
		if(ImprTraitVert[0].col >= 0) ImprimeLesTraits(epais);
		else ImprimeWeb("<hr noshade>\n");
	} else {
		switch(epais) {
			case 0: trait = ' '; break;
			case 1: trait = '.'; break;
			case 2: trait = '-'; break;
			default: trait = '='; break;
		}
		for(i=0; i<nb; i++) ImpressionLigne[col+i] = trait;
		i = 0;
		while((k = ImprTraitVert[i++].col) >= 0) {
			if((k >= col) && (k < col+nb)) ImpressionLigne[k] = '|';
		}
	}
}
/* ========================================================================== */
void ImprimeLimite(int col, int nb, char basse) {
	int i,k,m;

	if(basse) ImprimeNouvelleLigne(1);

	if(ImprPs.active) {
		fprintf(PrintPath,"/xc currentpoint pop store\n");
		fprintf(PrintPath,"/yc currentpoint exch pop store\n");
		if(basse) fprintf(PrintPath,"/y %d hauteur %d mul sub store\n",ImpressionHautFeuille,ImpressionNumLigne);
		else fprintf(PrintPath,"/y %d hauteur %d mul sub store\n",ImpressionHautFeuille,ImpressionNumLigne-1);
		m = col + nb;
		fprintf(PrintPath,"newpath %d setlinewidth marge_x largeur %d mul add y moveto marge_x largeur %d mul add y lineto stroke closepath\n",ImprPs.epaisseur,col,m);
		fprintf(PrintPath,"epaisseur setlinewidth xc yc moveto\n");
	} else if(ImprWeb.active) {
		if(ImprTraitVert[0].col >= 0) {
			// if(basse) ImprimeWebTableFin(); else ImprimeWebTableDebut();
			if(basse) {
				printf("<!-- Fin de table demande dans %s -->\n",__func__);
				ImprimeWebTableFin();
			} else {
				printf("<!-- Debut de table demande dans %s -->\n",__func__);
				ImprimeWebTableDebut();
			}
		}
	} else {
		for(i=0; i<nb; i++) ImpressionLigne[col+i] = '_';
		if(basse) {
			i = 0;
			while((k = ImprTraitVert[i++].col) >= 0) {
				if((k >= col) && (k < col+nb)) ImpressionLigne[k] = '|';
			}
		} else {
			ImpressionLigne[col] = ImpressionLigne[col+nb] = ' ';
			ImprimeNouvelleLigne(1);
		}
	}
}
/* ========================================================================== */
void ImprimeLimiteCouleur(int col, int nb, char basse, unsigned short r, unsigned short g, unsigned short b) {
	int i,k,m;

	if(basse) ImprimeNouvelleLigne(1);

	if(ImprPs.active) {
		fprintf(PrintPath,"/xc currentpoint pop store\n");
		fprintf(PrintPath,"/yc currentpoint exch pop store\n");
		if(basse) fprintf(PrintPath,"/y %d hauteur %d mul sub store\n",ImpressionHautFeuille,ImpressionNumLigne);
		else fprintf(PrintPath,"/y %d hauteur %d mul sub store\n",ImpressionHautFeuille,ImpressionNumLigne-1);
		m = col + nb;
		fprintf(PrintPath,"newpath %d setlinewidth marge_x largeur %d mul add y moveto marge_x largeur %d mul add y lineto %g %g %g setrgbcolor stroke closepath\n",
			ImprPs.epaisseur,col,m,(float)r / 65535.0,(float)g / 65535.0,(float)b / 65535.0);
		fprintf(PrintPath,"epaisseur setlinewidth xc yc moveto\n");
	} else if(ImprWeb.active) {
		if(ImprTraitVert[0].col >= 0) {
			// if(basse) ImprimeWebTableFin();
			// else ImprimeWebTableDebut();
			if(basse) {
				printf("<!-- Fin de table demande dans %s -->\n",__func__);
				ImprimeWebTableFin();
			} else {
				printf("<!-- Debut de table demande dans %s -->\n",__func__);
				ImprimeWebTableDebut();
			}
		}
	} else {
		for(i=0; i<nb; i++) ImpressionLigne[col+i] = '_';
		if(basse) {
			i = 0;
			while((k = ImprTraitVert[i++].col) >= 0) {
				if((k >= col) && (k < col+nb)) ImpressionLigne[k] = '|';
			}
		} else {
			ImpressionLigne[col] = ImpressionLigne[col+nb] = ' ';
			ImprimeNouvelleLigne(1);
		}
	}
}
/* ========================================================================== */
void ImprimeTableauDebut(int nb, ...) {
	/* defini des valeurs absolues */
	va_list argptr; int i,m,n,col;

	if(ImprTraitVert[0].col >= 0) {
		if(ImprPs.active) {
			n = ImprimeTraitVertNb();
			ImprimeLimite(ImprTraitVert[0].col,ImprTraitVert[n-1].col-ImprTraitVert[0].col,1);
			fprintf(PrintPath,"/xc currentpoint pop store\n");
			fprintf(PrintPath,"/bas_col currentpoint exch pop store\n");
			fprintf(PrintPath,"%d setlinewidth\n",ImprPs.epaisseur);
			i = 0;
			while(ImprTraitVert[i].col >= 0) {
				fprintf(PrintPath,"newpath ");
				fprintf(PrintPath,"marge_x %d largeur mul add haut_col moveto ",ImprTraitVert[i].col);
				fprintf(PrintPath,"marge_x %d largeur mul add bas_col lineto ",ImprTraitVert[i].col);
				fprintf(PrintPath,"stroke closepath\n");
				fprintf(PrintPath,"xc bas_col moveto\n");
				i++;
			}
			fprintf(PrintPath,"epaisseur setlinewidth\n");
		} else if(ImprWeb.active) {
			printf("<!-- Fin de table demande dans %s -->\n",__func__);
			ImprimeWebTableFin();
		}
	}
	if(nb == 0) { ImprTraitVert[0].col = -1; return; }
	m = nb; if(m > MAX_TRAITSVERT) m = MAX_TRAITSVERT;
	va_start(argptr, nb);
	i = 0; while(i < m) {
		//? ImprTraitVert[i].aligne = IMPRESSION_LEFT;
		col = va_arg(argptr,int);
		ImprimeTraitVertPlace(i,col);
		i++;
	}
	va_end(argptr);
	ImprimeTraitVertDernier(i);
	ImprimeColonnesHaut(i);
}
/* ========================================================================== */
void ImprimeTableauLargeurs(int col0, int nb, ...) {
	/* defini les largeurs */
	va_list argptr; int i,m,n; int larg;

	if(ImprTraitVert[0].col >= 0) {
		if(ImprPs.active) {
			n = ImprimeTraitVertNb();
			ImprimeLimite(ImprTraitVert[0].col,ImprTraitVert[n-1].col-ImprTraitVert[0].col,1);
			fprintf(PrintPath,"/xc currentpoint pop store\n");
			fprintf(PrintPath,"/bas_col currentpoint exch pop store\n");
			fprintf(PrintPath,"%d setlinewidth\n",ImprPs.epaisseur);
			i = 0;
			while(ImprTraitVert[i].col >= 0) {
				fprintf(PrintPath,"newpath ");
				fprintf(PrintPath,"marge_x %d largeur mul add haut_col moveto ",ImprTraitVert[i].col);
				fprintf(PrintPath,"marge_x %d largeur mul add bas_col lineto ",ImprTraitVert[i].col);
				fprintf(PrintPath,"stroke closepath\n");
				fprintf(PrintPath,"xc bas_col moveto\n");
				i++;
			}
			fprintf(PrintPath,"epaisseur setlinewidth\n");
		} else if(ImprWeb.active) {
			printf("<!-- Fin de table demande dans %s -->\n",__func__);
			ImprimeWebTableFin();
		}
	}
	if(nb == 0) { ImprTraitVert[0].col = -1; return; }
	m = nb + 1; if(m > MAX_TRAITSVERT) m = MAX_TRAITSVERT;
	ImprTraitVert[0].col = (short)col0;
	va_start(argptr, nb);
	i = 1; while(i < m) {
		larg = va_arg(argptr,int);
		ImprimeTraitVertEcarte(i,larg);
		i++;
	}
	va_end(argptr);
	ImprimeTraitVertDernier(i);
	ImprimeColonnesHaut(i);
}
/* ========================================================================== */
void ImprimeTableauChapeau(char *titre) {
	int i,j,c; int premiere,derniere;
	short trait[MAX_TRAITSVERT+1]; char largr[MAX_TRAITSVERT+1]; char aligne[MAX_TRAITSVERT+1];

	premiere = ImprTraitVert[0].col;
	i = 0; while(ImprTraitVert[i].col >= 0) {
//		if((i > 0) && (ImprTraitVert[i-1].aligne == IMPRESSION_RIGHT)) trait[i] = -ImprTraitVert[i].col;
//		else trait[i] = ImprTraitVert[i].col;
		trait[i] = ImprTraitVert[i].col;
		largr[i] = ImprTraitVert[i].largr;
		aligne[i] = ImprTraitVert[i].aligne;
		i++;
	}
//	trait[i] = -1;
	ImprTraitVert[0].col = -1;
	derniere = ImprTraitVert[i - 1].col;
	c = ((derniere - premiere - (int)strlen(titre)) / 2) + premiere + 1;
	if(c < (premiere + 1)) c = premiere + 1;
	ImprimeLimite(premiere,derniere,0); ImprimeColonnes(2,premiere,derniere);
	ImprimeEnCol(c,titre); ImprimeFond(premiere,derniere-premiere,GRIS_CLAIR);
	ImprimeLimite(premiere,derniere,1);
	ImprimeFond(premiere,derniere-premiere,GRIS_CLAIR); ImprimeColonnes(0);
	ImprimeNouvelleLigne(1);
	for(j=0; j<i; j++) {
//		if(trait[j] >= 0) {
//			ImprTraitVert[j].col = trait[j];
//			if(j > 0) ImprTraitVert[j-1].aligne = IMPRESSION_LEFT;
//		} else {
//			ImprTraitVert[j].col = -trait[j];
//			if(j > 0) ImprTraitVert[j-1].aligne = IMPRESSION_RIGHT;
//		}
		ImprTraitVert[j].col = trait[j];
		ImprTraitVert[j].largr = largr[j];
		ImprTraitVert[j].aligne = aligne[j];
	}
	ImprimeTraitVertDernier(j);
	ImprimeLimite(premiere,derniere-premiere,0);
	if(ImprPs.active) fprintf(PrintPath,"/haut_col currentpoint exch pop hauteur add store\n");
	else if(ImprWeb.active) {
		ImprimeWeb("\n<tr><td align=center>%s</td></tr>\n",titre);
	} else {
		i = 0;
		while(ImprTraitVert[i].col >= 0) ImpressionLigne[ImprTraitVert[i++].col] = '|';
	}
}
/* ========================================================================== */
int ImprimeTableauAjouteColonne(int col) {
	int i;

	i = ImprimeTraitVertNb();
	ImprimeTraitVertPlace(i,col);
	ImprimeTraitVertDernier(i + 1);
	return(i);
}
/* ========================================================================== */
int ImprimeTableauAjouteLargeur(int larg) {
	int i;

	i = ImprimeTraitVertNb();
	ImprimeTraitVertEcarte(i,larg);
	ImprimeTraitVertDernier(i + 1);
	return(i);
 }
/* ========================================================================== */
void ImprimeTableauPret(void) { ImprimeColonnesHaut(ImprimeTraitVertNb()); }
/* ========================================================================== */
void ImprimeTableauCol(int col, char *fmt, ...) {
	va_list argptr; int cnt; char texte[256]; int i,k;

	va_start(argptr, fmt);
	cnt = vsprintf(texte, fmt, argptr);
	va_end(argptr);

	i = ImprimeTraitVertNb();
	if((col < 0) || (col >= i)) return;
	if(ImprTraitVert[col].aligne == IMPRESSION_RIGHT) {
		k = ImprTraitVert[col].largr - 1 - (int)strlen(texte);
		if(k < 0) k = 1;
	} else k = 1;
	ImprimeInsere(ImprTraitVert[col].col+k,texte);
}
/* ========================================================================== */
void ImprimeTableauTrait(int col0, int colN, int epais) {
	ImprimeTrait(ImprTraitVert[col0].col,ImprTraitVert[colN+1].col-ImprTraitVert[col0].col,epais);
}
/* ========================================================================== */
void ImprimeTableauLimite(int col0, int colN) {
	ImprimeLimite(ImprTraitVert[col0].col,ImprTraitVert[colN+1].col-ImprTraitVert[col0].col,1);
	ImprimeNouvelleLigne(1);
}
/* ========================================================================== */
void ImprimeTableauFerme(void) {
	int i;

	if(ImpressionNumLigne < ImpressionMaxLignes) {
		i = ImprimeTraitVertNb();
		ImprimeLimite(ImprTraitVert[0].col,ImprTraitVert[i-1].col-ImprTraitVert[0].col,1);
	} else {
		ImprimeFinLigne();
		if(ImprWeb.active) printf("<!-- Fin de table demande dans %s -->\n",__func__);
		if(ImprWeb.active) ImprimeWebTableFin();
	}
	if(ImprPs.active) {
		if(ImpressionNumLigne > 1) {
			fprintf(PrintPath,"/xc currentpoint pop store\n");
			fprintf(PrintPath,"/bas_col currentpoint exch pop store\n");
			fprintf(PrintPath,"%d setlinewidth\n",ImprPs.epaisseur);
			i = 0;
			while(ImprTraitVert[i].col >= 0) {
				fprintf(PrintPath,"newpath ");
				fprintf(PrintPath,"marge_x %d largeur mul add haut_col moveto ",ImprTraitVert[i].col);
				fprintf(PrintPath,"marge_x %d largeur mul add bas_col lineto ",ImprTraitVert[i].col);
				fprintf(PrintPath,"stroke closepath\n");
				fprintf(PrintPath,"xc bas_col moveto\n");
				i++;
			}
			fprintf(PrintPath,"epaisseur setlinewidth\n");
		}
	}
}
/* ========================================================================== */
void ImprimeTableauFin(void) {
	ImprimeTableauFerme();
	ImprTraitVert[0].col = -1;
	// if(ImprWeb.active) printf("<br>\n"); else
	ImprimeNouvelleLigne(1);
}
/* ========================================================================== */
void ImprimeDirect(char *fmt, ...) {
	va_list argptr; int cnt;
	char ligne[256];
	int l,k; char lf,page;

	va_start(argptr, fmt);
	cnt = vsprintf(ligne, fmt, argptr);
	va_end(argptr);

	if(!ImprPs.active) {
		if(ImpressionFormat == IMPRESSION_HTTP) HttpEcrit(ligne);
	#ifdef OPIUM_H
		else if(ImpressionSupport == IMPRESSION_TERMINAL) TermPrint(tImpression,ligne);
	#endif /* OPIUM_H */
		else fputs(ligne,PrintPath);
			//#ifdef OS9: l = strlen(ligne); write(Pipe,ligne,l);
	} else {
		l = 0; k = l;
		while(ligne[k]) {
			lf = page = 0;
			// if(ligne[k] == 'é') ligne[k] = 'e'; compile comme du multicaractere!
			// else {
				lf = (ligne[k] == '\n'); if(!lf) page = (ligne[k] == '\f');
			// }
			if(lf || page) {
				ligne[k] = '\0';
				ImpressionTextePS(ligne+l); // fprintf(PrintPath,"(%s) show\n",);
				if(lf) {
					ImpressionNumLigne++;
					fprintf(PrintPath,"marge_x %d hauteur %d mul sub moveto\n",ImpressionHautFeuille,ImpressionNumLigne);
				} else /* if(page) */ {
					fprintf(PrintPath,"stroke\n");
					fprintf(PrintPath,"showpage\n");
					if(!ImpressionPortrait) fprintf(PrintPath,"[0 -1 1 0 0 %d] concat\n",A4_TRANSL);
					ImpressionNumLigne = 1;
					fprintf(PrintPath,"marge_x %d hauteur %d mul sub moveto\n",ImpressionHautFeuille,ImpressionNumLigne);
				}
				l = k + 1;
			}
			k++;
		}
		if(k > l) ImpressionTextePS(ligne+l); // fprintf(PrintPath,"(%s) show\n",);
	}
}
/* ========================================================================== */
INLINE void ImprimeDirectN(char *motif, int nb) {
	int n; for(n=0; n<nb; n++) ImprimeDirect(motif);
}
/* ========================================================================== */
void ImprimeDirectTrait(char *nom0, char *nom1, int epais) {
	if(!ImprPs.active) return;
	fprintf(PrintPath,"newpath\n");
	fprintf(PrintPath,"%d setlinewidth\n",epais);
	fprintf(PrintPath,"%sx %sy moveto\n",nom0,nom0);
	fprintf(PrintPath,"%sx %sy lineto\n",nom1,nom1);
	fprintf(PrintPath,"stroke\n");
	fprintf(PrintPath,"closepath\n");
	fprintf(PrintPath,"epaisseur setlinewidth\n");
}
/* ========================================================================== */
void ImprimeDirectEntoure(char *nom0, char *nom1, int epais) {
	if(!ImprPs.active) return;
	fprintf(PrintPath,"newpath\n");
	fprintf(PrintPath,"%d setlinewidth\n",epais);
	fprintf(PrintPath,"%sx 2 sub %sy hauteur add 4 add moveto\n",nom0,nom0);
	fprintf(PrintPath,"%sx 2 add %sy hauteur add 4 add lineto\n",nom1,nom0);
	fprintf(PrintPath,"%sx 2 add %sx 4 sub lineto\n",nom1,nom1);
	fprintf(PrintPath,"%sx 2 sub %sy 4 sub lineto\n",nom0,nom1);
	fprintf(PrintPath,"closepath\n");
	fprintf(PrintPath,"epaisseur setlinewidth\n");
}
/* ========================================================================== */
void ImprimeDirectSouligne(char *texte, int epais) {
	if(!ImprPs.active) return;
	fprintf(PrintPath,"/x0 currentpoint pop store\n");
	fprintf(PrintPath,"/y0 currentpoint exch pop 2 sub %d sub store\n",epais);
	ImpressionTextePS(texte); // fprintf(PrintPath,"(%s) show\n",texte);
	fprintf(PrintPath,"/x1 currentpoint pop store\n");
	fprintf(PrintPath,"/y1 currentpoint exch pop store\n");
	fprintf(PrintPath,"newpath\n");
	fprintf(PrintPath,"%d setlinewidth\n",epais);
	fprintf(PrintPath,"x0 y0 moveto x1 y0 lineto stroke\n");
	fprintf(PrintPath,"closepath\n");
	fprintf(PrintPath,"x1 y1 moveto\n");
	fprintf(PrintPath,"epaisseur setlinewidth\n");
}
/* ========================================================================== */
void ImprimeDirectSurligne(char *texte, unsigned short r, unsigned short g, unsigned short b) {
	size_t l;

	if(!ImprPs.active) return;
	l = strlen(texte); while(l && (texte[l-1] == '\n')) l--;
	if(l) fprintf(PrintPath,"%g %g %g setrgbcolor /lig 1 store /nb %ld surligne\n",
				  (float)r / 65535.0,(float)g / 65535.0,(float)b / 65535.0, l);
}
/* ========================================================================== */
void ImpressionTerminePage(void) {
	if(ImprPs.active) fprintf(PrintPath,"stroke\nshowpage\n");
}
/* ========================================================================== */
void ImpressionFerme(void) { fclose(PrintPath); }
/* ========================================================================== */
int ImpressionPStoPDF(char *nom_complet) {
	char commande[512],nom_pdf[MAXFILENAME],*nom_initial;
	size_t l; int rc;

	nom_initial = nom_complet;
	if(nom_complet[0] == '-') {
		strcpy(nom_pdf,"-"); nom_initial++;
	} else {
		l = strlen(nom_complet) - 3;
		if(!strcmp(nom_complet+l,".ps")) { strncpy(nom_pdf,nom_complet,l); strcpy(nom_pdf+l,".pdf"); }
		else strcat(strcpy(nom_pdf,nom_complet),".pdf");
	}
	/* char message[256];
	sprintf(commande,"umask 0");
	sprintf(message,"Commande: %s\n",commande); strcat(ImpressionCR,message);
	errno = 0;
	rc = system(commande);
	sprintf(message,"Code de retour: %d (errno=%d)\n",rc,errno); strcat(ImpressionCR,message);

	sprintf(commande,"/usr/local/bin/gs -q -dNOPAUSE -dBATCH -sDEVICE=pdfwrite -sOUTPUTFILE=%s -f %s ",
		nom_pdf,nom_initial);
	sprintf(message,"Commande: %s\n",commande); strcat(ImpressionCR,message);
	errno = 0;
	rc = system(commande);
	sprintf(message,"Code de retour: %d (errno=%d)\n",rc,errno); strcat(ImpressionCR,message); */

	sprintf(commande,"umask 0; /usr/local/bin/gs -q -dNOPAUSE -dBATCH -sDEVICE=pdfwrite -sOUTPUTFILE=%s -f %s ",nom_pdf,nom_initial);
	errno = 0;
	rc = system(commande);

	if(!rc) { rc = remove(nom_initial); strcpy(nom_complet,nom_pdf); }
	return(rc);
}
/* ========================================================================== */
int ImpressionTeXtoPDF(char *nom_complet) {
	char commande[512],nom_pdf[MAXFILENAME],*nom_initial;
	size_t l; int rc;

	nom_initial = nom_complet;
	l = strlen(nom_complet) - 4;
	if(!strcmp(nom_complet+l,".tex")) { strncpy(nom_pdf,nom_complet,l); strcpy(nom_pdf+l,".pdf"); }
	else strcat(strcpy(nom_pdf,nom_complet),".pdf");
	sprintf(commande,"umask 0; pdflatex %s",nom_initial);
	errno = 0;
	rc = system(commande);

	if(!rc) { rc = remove(nom_initial); strcpy(nom_complet,nom_pdf); }
	return(rc);
}
/* ========================================================================== */
void ImpressionFin(char avec_opium) {
	char commande[512],nom_initial[MAXFILENAME];
	char message[256]; int rc;

	if(!ImpressionEnCours) return;
	ImpressionEnCours = 0;
	if(ImprPs.active) {
		fprintf(PrintPath,"stroke\n");
		fprintf(PrintPath,"showpage\n");
	} else if(ImprTex.active) {
		fprintf(PrintPath,"\\end{document}\n");
	} else {
	#ifdef OPIUM_H
		if((ImpressionSupport == IMPRESSION_TERMINAL) && (ImpressionFormat != IMPRESSION_HTTP)) {
			TermRelease(tImpression);
			OpiumDisplay(tImpression->cdr);
			return;
		}
	#endif /* OPIUM_H */
		if(!strcmp(ImpressionNomComplet,"*")) {
			if(!ImprWeb.active) printf("---------------- Impression terminee\n");
			return;
		}
		//#ifdef OS9: close(Pipe); close(clavier);
	}
	fclose(PrintPath);
	sprintf(ImpressionCR,"Fichier initialement cree: %s\n",ImpressionNomComplet);
	if((ImpressionSupport == IMPRESSION_PAPIER) && !ImprWeb.active) {
		if((ImpressionCde == IMPRESSION_LPR) || (ImpressionCde == IMPRESSION_ENSCRIPT))
			sprintf(commande,"/usr/bin/%s %s -P %s -#%d %s",
				ImpressionClients[ImpressionCde],ImpressionOptions,Imprimante,ImpressionTirage,ImpressionNomComplet);
		else {
			strcpy(commande,ImpressionClients[ImpressionCde]); // strcat(strcat(commande," "),ImpressionOptions);
			strcat(strcat(commande," "),ImpressionNomComplet);
		}
		// system(commande);
		sprintf(message,"Impression par \"%s\"\n",commande); strcat(ImpressionCR,message);
		sprintf(message,"Code de retour: %d\n",system(commande)); strcat(ImpressionCR,message);
	} else if((ImprPs.active && (ImpressionFormat == IMPRESSION_PDF)) || ImprTex.active) {
		strcpy(nom_initial,ImpressionNomComplet);
		if(ImprPs.active) rc = ImpressionPStoPDF(ImpressionNomComplet);
		else rc = ImpressionTeXtoPDF(ImpressionNomComplet);
		if(rc == 0) {
			sprintf(message,"Fichier supprime: %s\n",nom_initial); strcat(ImpressionCR,message);
		} else if(strcmp(nom_initial,ImpressionNomComplet)) {
			sprintf(message,"Fichier a effacer: %s [%s]\n",nom_initial,strerror(errno)); strcat(ImpressionCR,message);
		} else {
			sprintf(message,"Probleme au passage en PDF, code %d: %s\n",rc,strerror(errno)); strcat(ImpressionCR,message);
		}
		sprintf(message,"Fichier a utiliser: %s\n",ImpressionNomComplet); strcat(ImpressionCR,message);
	} else if(ImprTex.active) {

	}

	if((ImprPs.active && (ImpressionSupport == IMPRESSION_TERMINAL))
	   || (ImprWeb.active && (ImpressionSupport == IMPRESSION_PAPIER))) {
		sprintf(commande,"open %s &",ImpressionNomComplet);
		system(commande);
	#ifdef OPIUM_H
		if(avec_opium) OpiumWarn("Retour a l'application %s",__progname);
	#endif /* OPIUM_H */
	}
}
/* ========================================================================== */
char *ImpressionFichierCree(void) { return(ImpressionNomComplet); }
/* ========================================================================== */
char *ImpressionLog(void) { return(ImpressionCR); }
