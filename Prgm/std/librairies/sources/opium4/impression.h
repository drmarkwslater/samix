#ifndef IMPRESSION_H
#define IMPRESSION_H

#ifndef CGI
#include <opium4/opium.h>
#endif

#ifdef IMPRIME_C
#define VAR_IMPRIME
#else
#define VAR_IMPRIME extern
#endif

/*
    support |    terminal ==
            |      apercu       |      fichier      |      papier
 format     |     / safari      |                   |
 -----------|-------------------|-------------------|-----------------
    texte   |    TermPrint      |   fprintf(f:txt)  |   fprintf
            |                   |                   | + enscript
 -----------|-------------------|-------------------|-----------------
    PS      |   fprintf(f:PS)   |   fprintf(f:PS)   |   fprintf(f:PS)
            | + gs -dBATCH <f>  |                   | + lpr
 -----------|-------------------|-------------------|-----------------
    PDF     |   fprintf(f:PS)   |   fprintf(f:PS)   |   fprintf(f:PS)
            | + gs ... <g> <f>  | + gs ... <g> <f>  | + lpr
            | + Apercu <g>      |                   |
 -----------|-------------------|-------------------|-----------------
	html    |  fprintf(stdout)  |   fprintf(f:html) |   fprintf
 -----------|-------------------|-------------------|-----------------
 
 */

typedef enum {
	IMPRESSION_TERMINAL = 0,
	IMPRESSION_FICHIER,
	IMPRESSION_PAPIER,
	IMPRESSION_EFFACE,
	IMPRESSION_ANNULE,
	IMPRESSION_SUPPORT_NB
} IMPRESSION_SUPPORT;
#define IMPRESSION_SUPPORT_LNGR 12
VAR_IMPRIME char *ImpressionSupports[]
#ifdef IMPRIME_C
 = { "apercu", "fichier", "imprimante", "\0" }
#endif
;

typedef enum {
	IMPRESSION_TEXTE = 0,
	IMPRESSION_PS,
	IMPRESSION_PDF,
	IMPRESSION_TEX,
	IMPRESSION_HTML,
	IMPRESSION_HTTP,
	IMPRESSION_FORMAT_NB
} IMPRESSION_FORMAT;
#define IMPRESSION_FORMAT_LNGR 12
VAR_IMPRIME char *ImpressionFormats[IMPRESSION_FORMAT_NB+1]
#ifdef IMPRIME_C
 = { "texte", "postscript", "PDF", "TeX", "html", "http", "\0" }
#endif
;

typedef enum {
	IMPRESSION_REGULIER = 0,
	IMPRESSION_ITALIQUE,
	IMPRESSION_GRAS,
	IMPRESSION_STYLE_NB
} IMPRESSION_STYLE;
#define IMPRESSION_FORMAT_LNGR 12
VAR_IMPRIME char *ImpressionStyles[IMPRESSION_STYLE_NB+1]
#ifdef IMPRIME_C
 = { "regulier", "italique", "gras", "\0" }
#endif
;
VAR_IMPRIME char ImpressionStylesTag[IMPRESSION_STYLE_NB]
#ifdef IMPRIME_C
= { '\0', 'i', 'b' }
#endif
;

typedef enum {
	IMPRESSION_AUTRE = 0,
	IMPRESSION_LPR,
	IMPRESSION_ENSCRIPT,
	IMPRESSION_CMDE_NB
} IMPRESSION_CMDE;
VAR_IMPRIME char *ImpressionClients[IMPRESSION_CMDE_NB+1]
#ifdef IMPRIME_C
 = { "nul", "lpr", "enscript", "\0" }
#endif
;
#define MAX_TRAITSVERT 64
typedef enum {
	IMPRESSION_LEFT = 0,
	IMPRESSION_CENTER,
	IMPRESSION_RIGHT
} IMPRESSION_ALIGNEMENT;
typedef enum {
	IMPRESSION_INCHANGEE = 0,
	IMPRESSION_LIBRE,
	IMPRESSION_NBCARS,
	IMPRESSION_POURCENT,
} IMPRESSION_TYPELARGEUR;
// VAR_IMPRIME int ImpressionCol[MAX_TRAITSVERT+1];
// VAR_IMPRIME char ImpressionAlign[MAX_TRAITSVERT];
// VAR_IMPRIME char ImpressionLargr[MAX_TRAITSVERT];
VAR_IMPRIME struct {
	short col; char largr; char aligne;
} ImprTraitVert[MAX_TRAITSVERT];
#define ImprimeTableauPos(trait) ImprTraitVert[trait].col

void ImpressionInit(void);
void ImpressionHauteurPage(int nb);
void ImpressionFormatte(IMPRESSION_FORMAT fmt);
void ImprimeTailleDefaut(int t);
void ImprimeEpaisseurDefaut(int e);
void ImpressionSurSupport(IMPRESSION_SUPPORT support, void *adrs);
int ImprimeTraitVertNb(void);
void ImprimeTraitVertPlace(int i, int col);
void ImprimeTraitVertEcarte(int i, int larg);
void ImprimeTraitVertDernier(int i);
void ImpressionServeur(char *dest, char *titre, int tirage);
int  ImpressionLargeurCourante(void);
int  ImpressionHauteurCourante(void);
int  ImpressionLigneCourante(void);
void ImpressionHtmlDebut(char *titre, int couleur);
void ImpressionHtmlFin(void);
void ImpressionStockePoint(char *nom);
void ImprimeWebTableDesign(int epaisseur, int largeur, int pad, int espace);
void ImprimeCadreFormatte(IMPRESSION_FORMAT fmt);
void ImprimeCadreTexte(char *texte);
void ImprimeCadreEcrit(char *fmt, ...);
void ImprimeCadreOuvre(char *style, char *fmt, ...);
void ImprimeCadreLigneDebut(int n);
void ImprimeCadreLigne(char *texte);
void ImprimeCadreLigneFin(void);
void ImprimeCadreFerme(void);
int  ImpressionPrete(char *orientation);
void ImprimeFond(int col, int nb, unsigned short r, unsigned short g, unsigned short b);
void ImprimeNFonds(int col, int nb, int lig, unsigned short r, unsigned short g, unsigned short b);
void ImprimeCouleur(unsigned short r, unsigned short g, unsigned short b);
void ImprimeStyle(IMPRESSION_STYLE style);
int  ImprimeToHtml(char *dest, char *srce, int max);
void ImprimeColonneFormat(int k, IMPRESSION_ALIGNEMENT aligne, IMPRESSION_TYPELARGEUR mode, int val);
void ImpressionPlaceColonne(int *trait, int largeur, int *col, int *fin);
void ImprimeLimite(int col, int nb, char basse);
void ImprimeLimiteCouleur(int col, int nb, char basse, unsigned short r, unsigned short g, unsigned short b);
void ImprimeTrait(int col, int nb, int epais);
void ImprimeColonnes(int nb, ...);
void ImprimeTableauDebut(int nb, ...);
void ImprimeTableauLargeurs(int col0, int nb, ...);
void ImprimeTableauChapeau(char *titre);
int  ImprimeTableauAjouteColonne(int col);
int  ImprimeTableauAjouteLargeur(int larg);
void ImprimeTableauPret(void);
void ImprimeTableauCol(int col, char *fmt, ...);
// INLINE int ImprimeTableauPos(int col);
void ImprimeTableauTrait(int col0, int colN, int epais);
void ImprimeLesTraits(int trait);
void ImprimeTableauLimite(int col0, int colN);
void ImprimeTableauFerme(void);
void ImprimeTableauFin(void);
char ImprimeGroupeLignes(int nb);
char ImpressionPageCommencee(void);
void ImprimeFinLigne(void);
void ImprimeSautPage(void);
void ImprimeNouvelleLigne(int nb);
void ImprimeNouvellePage(void);
int  ImprimeEnCol(int col, char *fmt, ...);
void ImprimeCouleur(unsigned short r, unsigned short g, unsigned short b);
void ImprimeDirect(char *fmt, ...);
INLINE void ImprimeDirectN(char *motif, int nb);
void ImprimeDirectTrait(char *nom0, char *nom1, int epais);
void ImprimeDirectEntoure(char *nom0, char *nom1, int epais);
void ImprimeDirectSouligne(char *texte, int epais);
void ImprimeDirectSurligne(char *texte, unsigned short r, unsigned short g, unsigned short b);
void ImpressionTerminePage(void);
int  ImpressionPStoPDF(char *nom_complet);
void ImpressionFerme(void);
int  ImpressionTeXtoPDF(char *nom_complet);
void ImpressionFin(char avec_opium);
char *ImpressionFichierCree(void);
char *ImpressionLog(void);

#endif
