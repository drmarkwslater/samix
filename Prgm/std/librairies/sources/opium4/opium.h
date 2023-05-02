#ifndef OPIUM_H
#define OPIUM_H

#define DISPLAY_CREUX

#define DEBUG_OPIUM4
#define TIMER_NIVEAU_OPIUM

#include "environnement.h"

#include <utils/defineVAR.h>
#include <utils/claps.h>
#include <utils/dico.h>
#include <opium4/wnd.h>

#ifdef OPIUM_C
	#define OPIUM_VAR
#else
	#define OPIUM_VAR extern
#endif

#ifdef TIMER_NIVEAU_OPIUM
	#ifdef TIMER_PAR_EVT
		#ifndef TIMER_UNIQUE
			#include "timeruser.h"
		#endif
	#endif
#endif

#pragma mark Types et constantes
/* .............................. opium.h ................................... */
#define OPIUM_MAXCDR 4096
#define OPIUM_MAXTITRE 256
#define OPIUM_MAXNOM 64
#define OPIUM_MAXFILE 256

typedef WndServer *OpiumServer;

#define OPIUM_AUTOPOS -1
#define OPIUM_ADJACENT -2
#define OPIUM_AVANT -3
#define OPIUM_APRES -4
#define OPIUM_ALIGNE -5
#define OPIUM_A_DROITE_DE   OPIUM_ADJACENT,0,
#define OPIUM_A_GAUCHE_DE   OPIUM_ADJACENT,-1,
#define OPIUM_EN_DESSOUS_DE 0,OPIUM_ADJACENT,
#define OPIUM_AU_DESSUS_DE  -1,OPIUM_ADJACENT,
#define OPIUM_PRECEDENT 0

typedef enum {
	OPIUM_MENU = 0,
	OPIUM_PANEL,
	OPIUM_TERM,
	OPIUM_GRAPH,
	OPIUM_INFO,
	OPIUM_PROMPT,
	OPIUM_BOARD,
	OPIUM_INSTRUM,
	OPIUM_FIGURE,
	OPIUM_ONGLET,
	OPIUM_NBTYPES
} OPIUM_TYPE;
typedef enum {
	OPIUM_DISPLAY = 0,
	OPIUM_SUBWND,
	OPIUM_EXEC,
	OPIUM_FORK,
	OPIUM_MODENB
} OPIUM_MODEXEC;
OPIUM_VAR char *OpiumExecName[OPIUM_MODENB+1]
#ifdef OPIUM_C
= { "DISPLAY", "SUBWND", "EXEC", "FORK", "\0" }
#endif
;

#define L_(texte) DicoLocale(&OpiumDico,texte)
#define LL_(liste) DicoLocaleListeNouvelle(&OpiumDico,liste)
#define LC_(liste,ancienne) DicoLocaleListeChange(&OpiumDico,liste,ancienne)

typedef enum {
	OPIUM_TOUJOURS = 0,
	OPIUM_NECESSAIRE = 1
} OPIUM_REFRESH_CONDITION;
typedef enum {
	OPIUM_PS_ECRAN = 0,
	OPIUM_PS_A4
} OPIUM_PS_FORMAT;
#define OPIUM_PSMARGE_X 2
#define OPIUM_PSMARGE_Y 60

typedef struct {
	WndColorLevel r,g,b;
} OpiumColor;
#define OPIUM_LUT_DIM 256

#define OPIUM_COLORS_AUTO OPIUM_LUT_DIM
typedef struct {
	int max; /* nombre de couleurs (2,4,8,16,..) */
	int num; /* numero de couleur (1,3,5,..,max-1) */
} OpiumColorState;

#define OPIUM_MAXKEYS 16
#define OPIUM_ALT (XK_Alt_L << 16)
//typedef struct {
//	int code;
//	int (*fctn)(void *, int);
//} OpiumKeyDef;

char OpiumTimeGiven(TypeWndTimeSlice fctn);

#pragma mark Cadres
/* .............................. cadres ................................... */

typedef struct StructCadre {
	WndFrame f;                   /* 0 si pas affiche a l'ecran               */
	struct StructCadre *planche;  /* 0 si ne fait pas partie d'une planche    */
	struct StructCadre *suivant;  /* 0 si dernier cadre                       */
	struct StructCadre *actif;    /* cadre actif si planche                   */
	char **bouton;                /* bouton en bas de planche (si planche)    */
	short nb_boutons;             /* nombre des ci-dessus dits                */
	char type; short num;         /* type de fenetre et numero dans le type   */
	char support;                 /* rien / lignes / plaquette / creux        */
	char modexec; char log;
	char volant;                  /* vrai si propre fenetre meme si dans planche */
	char displayed;               /* doit passer a vrai a la fin du 1er expose */
	char debranche;               /* vrai si clavier ou souris inactifs       */
	char var_modifiees;           /* vrai si a modifie une variable           */
	char gele;                    /* vrai si a ne pas rafraichir              */
	void *adrs;                   /* adresse boitier ou premier cadre         */
	int  defaulth,defaultv;
	int  defaultx,defaulty;
	int  pos_std_x,pos_std_y;
	struct StructCadre *ref_pos;  /* reference pour le positionnement relatif */
	int  x,y;                     /* position locale (planche/fenetre)        */
	int  x0,y0;                   /* position dans la fenetre                 */
	int  haut,larg;               /* dimensions normales necessaires          */
	int  dx,dy,dh,dv;             /* fenetre affichee en coord. locales       */
	int  ligmin,colmin,lignb,colnb; /* zone affichee en nb de caracteres      */
	char nom[OPIUM_MAXNOM];       /* pour l'aide en ligne                     */
	char titre[OPIUM_MAXTITRE];
//	TypeOpiumFctnArg enter,exit;
	int (*enter)(struct StructCadre *, void *);
	int (*exit)(struct StructCadre *, void *);
	int (*clear)(struct StructCadre *, void *);
	int (*a_vide)(struct StructCadre *, WndAction *, void *);
	void *enter_arg,*exit_arg,*clear_arg,*a_vide_arg;
	int  code_rendu;
#ifdef TIMER_NIVEAU_OPIUM
#ifdef _XtIntrinsic_h
	unsigned int64 timer;      /* intervalle en millisecondes */
#endif
#endif
	int delai,restant;
	short fond;
	struct {
		int code;
		int (*fctn)(struct StructCadre *, int);
	} key[OPIUM_MAXKEYS];
} TypeCadre,*Cadre;

typedef int (*TypeOpiumFctn)(Cadre);
typedef int (*TypeOpiumFctnInt)(Cadre, int);
typedef int (*TypeOpiumFctnArg)(Cadre, void *);

#define OpiumCadre(objet) (objet? objet->cdr: 0)
#define OpiumDisplayed(cdr) (cdr? cdr->displayed: 0)
#define OpiumAlEcran(objet) (objet? ((objet->cdr)? (objet->cdr)->displayed: 0): 0)

int PasImplementee(void);
int EnConstruction(void);
int OpiumDebug(int opt, int level);
void OpiumKeyBoard(char type);
void OpiumLink(int id, char *dir);
int OpiumInit(char *display);
OpiumServer OpiumOpen(char *display);
void OpiumClose(OpiumServer svr);
void OpiumAppCode(char *nom);
void OpiumPort(int port);
void OpiumServerCurrent(OpiumServer svr);
void OpiumServerDefault(OpiumServer svr);
int OpiumServerWidth(OpiumServer svr);
int OpiumServerHeigth(OpiumServer svr);
void OpiumExit(void);
char OpiumDicoUtilise(char *fichier, DICO_MODE mode);
char OpiumDicoSauve(char *fichier);
void OpiumSetMouse(char flag);
Cadre OpiumCreate(int type, void *adrs);
int OpiumCopy(Cadre cdr);
int OpiumDelete(Cadre cdr);
void OpiumName(Cadre cdr, char *nom);
void OpiumTitle(Cadre cdr, char *titre);
void OpiumGetTitle(Cadre cdr, char *titre);
void OpiumOptions(int type, TypeOpiumFctn fctn);
void OpiumPeriodicFctn(TypeFctn fctn);
void OpiumEnterFctn(Cadre cdr, TypeOpiumFctnArg fctn, void *arg);
void OpiumExitFctn(Cadre cdr, TypeOpiumFctnArg fctn, void *arg);
void OpiumClearFctn(Cadre cdr, TypeOpiumFctnArg fctn, void *arg);
void *OpiumEnterArg(Cadre cdr);
void *OpiumExitArg(Cadre cdr);
void *OpiumClearArg(Cadre cdr);
void OpiumMinimum(Cadre cdr);
void OpiumBranche(Cadre cdr, char branche);
int OpiumKeyEnable(Cadre cdr, int code, TypeOpiumFctnInt fctn);
void OpiumPosition(Cadre cdr, int x, int y);
void OpiumSize(Cadre cdr, int h, int v);
void OpiumGetPosition(Cadre cdr, int *x, int *y);
void OpiumGetSize(Cadre cdr, int *h, int *v);
void OpiumPlace(Cadre cdr, int x, int y);
void OpiumAligne(Cadre cdr, Cadre to, int x, int y);
void OpiumBloque(Cadre cdr, char gele);
void OpiumSizeChanged(Cadre cdr);
void OpiumDrawingColor(Cadre cdr, WndContextPtr *gc, char *nom);
void OpiumDrawingRGB(Cadre cdr, WndContextPtr *gc,
	WndColorLevel r, WndColorLevel g, WndColorLevel b);
void OpiumDrawingBgndRGB(Cadre cdr, WndContextPtr *gc,
	WndColorLevel r, WndColorLevel g, WndColorLevel b);
void OpiumColorInit(OpiumColorState *s);
int  OpiumColorNext(OpiumColorState *s);
void OpiumColorAssign(OpiumColor *couleur, WndColorLevel r, WndColorLevel g, WndColorLevel b);
void OpiumColorArcEnCiel(OpiumColor *lut, int dim);
void OpiumColorCopy(OpiumColor *d, OpiumColor *s);
// void OpiumAfficheBoutons(Cadre cdr, WndFrame f, int nb, int premier);
void OpiumBouton(Cadre cdr, WndFrame f, char *texte);
void OpiumBoutonsScrolles(Cadre cdr, WndFrame f, int premier, int nb, char *texte[]);
void OpiumBoutonsTjrs(Cadre cdr, WndFrame f, int premier, int nb, char *texte[]);
void OpiumSupport(Cadre cdr, char support);
void OpiumMode(Cadre cdr, char modexec);
void OpiumLog(Cadre cdr, char log);
void OpiumEntryBefore(int (*fctn)());
void OpiumEntryAfter(int (*fctn)(int));
double OpiumGradsLineaires(double echelle);
int  OpiumGradOscillo(float v, char sup, int *d);
void OpiumSetOptions(Cadre cdr);
char OpiumEstContenuDans(Cadre cdr, Cadre planche);
void OpiumPlaceLocale(Cadre cdr);
char OpiumCoordFenetre(Cadre cdr, WndFrame *f);
int  OpiumUse(Cadre cdr, int modexec);
int  OpiumDisplay(Cadre cdr);
char OpiumRefreshBegin(Cadre cdr);
void OpiumRefreshEnd(Cadre cdr);
int  OpiumRefreshAgain(Cadre cdr);
int  OpiumRefresh(Cadre cdr);
void OpiumWndResize(Cadre cdr);
void OpiumHtmlBandeauDefini(TypeFctn fctn);
void OpiumActive(Cadre cdr);
int  OpiumTraiteRequete(char *url, char *demandeur, char *parms);
int  OpiumFork(Cadre cdr);
int  OpiumExec(Cadre cdr_initial);
int  OpiumSub(Cadre cdr_initial);
int  OpiumRun(Cadre cdr_initial, int mode);
int  OpiumWindow(Cadre cdr_initial, int mode, int *code_a_rendre);
// int  OpiumLoop(Cadre cdr_initial, int mode);
void OpiumPhotoDir(char *repert);
void OpiumPhotoSauve(void);
int  OpiumManage(WndIdent w_initiale, WndAction *u, int *cdr_ouverts);
void OpiumDernierClic(int *x, int *y);
void OpiumUpdateFlush(void);
int  OpiumUserAction(void);
void OpiumScrollX(Cadre cdr, int dx);
void OpiumScrollY(Cadre cdr, int dy);
void OpiumScrollXset(Cadre cdr, int dx);
void OpiumScrollYset(Cadre cdr, int dy);
void OpiumScrollXmin(Cadre cdr);
void OpiumScrollYmin(Cadre cdr);
void OpiumScrollXmax(Cadre cdr);
void OpiumScrollYmax(Cadre cdr);
#ifdef TIMER_NIVEAU_OPIUM
#ifdef _XtIntrinsic_h
static void OpiumTimerRestart(Cadre cdr);
int OpiumTimer(Cadre cdr, unsigned long timer);
#else  /* _XtIntrinsic_h */
#ifdef TIMER_PAR_EVT
#ifdef TIMER_UNIQUE
int OpiumTimer(Cadre cdr, int timer);
#else  /* TIMER_UNIQUE */
int OpiumTimer(Cadre cdr, int delai);
#endif  /* TIMER_UNIQUE */
#else   /* TIMER_PAR_EVT (donc ici, timer gere au niveau OPIUM et non WND) */
int OpiumTimer(Cadre cdr, int delai);
#endif  /* TIMER_PAR_EVT */
#endif  /* _XtIntrinsic_h */
#endif  /* TIMER_NIVEAU_OPIUM */
int OpiumClear(Cadre cdr);
void OpiumMacroDelim(char *chaine);
int OpiumMacroStore(char *nomlu);
void OpiumMacroClose(void);
int OpiumMacroRecall(char *nom);
int OpiumMacroBefore(void);
int OpiumMacroAfter(int key);
int OpiumMacroBase(void);

int OpiumEditCopier(Cadre cdr, int cle);
int OpiumEditColler(Cadre cdr, int cle);
int OpiumEditEffacer(Cadre cdr, int cle);

void OpiumTxtHome(char *path);
int OpiumTxtCreer(void);
void OpiumPSformat(OPIUM_PS_FORMAT choix);
void OpiumPSlargeur(float cm);
void OpiumPSentete(char *texte);
int OpiumPSInit(char *nom, int page);
void OpiumPSPosition(int x, int y);
void OpiumPSNouvellePage(void);
void OpiumPSRecopie(Cadre cdr);
void OpiumPSPause(void);
void OpiumPSReprend(void);
void OpiumPSClose(void);
int OpiumPSprint(Cadre cdr, int cle);
void OpiumPShome(char *path);
int OpiumPSComplet(void);
int OpiumPSDemarre(void);
int OpiumPSAjoute(void);
int OpiumPSPage(void);
int OpiumPSTermine(void);

void OpiumDicoEdite(Dico dico);

void OpiumEcritReel(char *chaine, char *fmt, float reel);
void OpiumEcritDble(char *chaine, char *fmt, double dble);

#pragma mark Aide
/* ................................ aide.h .................................. */

void HelpSource(char *nom);
void HelpMode(char mode);
int HelpMenu(Cadre cdr, int cle);

#pragma mark Prompts
/* .............................. prompts.h ................................. */
typedef enum {
	OPIUM_CHECK = 0,
	OPIUM_NOTE,
	OPIUM_WARN,
	OPIUM_ERROR,
	OPIUM_MSGNB
} OPIUM_MSG;
#define OPIUM_WARNING OPIUM_WARN

void OpiumPromptVO(char brut);
int  OpiumError(char *fmt, ...);
int  OpiumMessage(char niveau, char *fmt, ...);
int  OpiumNote(char *fmt, ...);
int  OpiumWarn(char *fmt, ...);
int  OpiumFail(char *fmt, ...);
int  OpiumCheck(char *fmt, ...);
int  OpiumChoix(int nb, char *boutons[], char *fmt, ...);
void OpiumProgresCreate(void);
void OpiumProgresTitle(char *titre);
void OpiumProgresInit(int total);
char OpiumProgresRefresh(int actuel);
void OpiumProgresClose(void);
int  OpiumGetChar(void);
int  OpiumCheckChar(void);
int  OpiumSizePrompt(Cadre cdr, char from_wm);
int  OpiumRefreshPrompt(Cadre cdr);
int  OpiumRunPrompt(Cadre cdr, WndAction *e);

#pragma mark LogBook
/* .............................. logbook.h ................................. */
void LogInit(void);
void LogOnStd(char b);
FILE *LogOnFile(char *nom);
FILE *LogAddFile(char *nom);
void LogOffFile(void);
lhex LogOnTerm(char *nom, int ligs, int cols, int taille);
void LogOffTerm(void);
void LogBlocBegin(void);
void LogBlocEnd(void);
void LogDump(FILE *f);
void LogDate(void);
void LogName(void);
void LogEltFloat(char *chaine, int i, float f);
void LogFloat(char *chaine, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9);
void LogString(char * chaine);
int  LogBook(char *fmt, ...);
void LogClose(void);

#pragma mark Instruments
/* .............................. instrum.h ................................. */

#define INS_MAXFMT 12

typedef enum {
	INS_CADRAN = 0,   /* pour affichage */
	INS_COLONNE,      /* pour affichage */
	INS_POTAR,        /* pour commande (idem suivants) */
	INS_GLISSIERE,
	INS_LEVIER,       /* 2 positions seulement */
	INS_POUSSOIR,     /* booleen */
	INS_BASCULE,      /* booleen */
	INS_RADIO,        /* plusieurs mutuellement exclusifs (boutons 'radio' [cf listes et keys], etc...) */
	INS_VOYANT        /* pour affichage */
} INS_FORME;

typedef enum {
	INS_AXE_CENTRE,
	INS_AXE_BAS_GAUCHE,
	INS_AXE_BAS_MILIEU,
	INS_AXE_BAS_DROIT,
	INS_AXE_MILIEU_DROIT,
	INS_AXE_HAUT_DROIT,
	INS_AXE_HAUT_MILIEU,
	INS_AXE_HAUT_GAUCHE,
	INS_AXE_MILIEU_GAUCHE
} INS_AXE_GEOM;

//typedef enum {
//	instrum->relief = WND_GC_BASE,
//	INS_COUL_CADRANT,
//	INS_COUL_BOUTON,
//	INS_COUL_INDIC,
//	INS_COUL_NB
//} INS_COULEURS;

typedef struct {
	Cadre cdr;
	char  type;                /* int/long/short/float/double             */
	char  deja_trace;
	char support;              /* support pour la zone 'variable'         */
	char gradauto,log;
	char  format[INS_MAXFMT];  /* format d'affichage                      */
	INS_FORME forme;
	void *parms;               /* parametres d'affichage (selon forme)    */
	void *adrs;                /* adresse utilisateur (selon type)        */
	TypeFctn on_change;
	void *change_arg;
	union {
		struct {
			int min,max;
		} i;
		struct {
			short min,max;
		} s;
		struct {
			float min,max;
		} f;
		struct {
			double min,max;
		} d;
		struct {
			void *liste;       /* mot-cles utilisateur (LISTx ou KEYx)     */
		} l;
	} lim;                     /* limites a appliquer                      */
	union {
		struct {
			double x,y;        /* dimensions instrum, a multiplier par le rayon */
			double x0,y0;      /* position axe, a multiplier par le rayon       */
		} cercle;
		struct {
			int decal;         /* decalage du trace p.r. origine de la fenetre  */
		} rect;
		struct {
			int htext;         /* hauteur reservee aux legendes                 */
		} levier;
		struct {
			int lngr;          /* largeur du texte inclus                   */
			WndContextPtr *gc; /* table des GC correspondants               */
			int   nbfonds;     /* nombre d'elements d'icelles               */
			int   numfond;     /* index du fond a utiliser                  */
			union {
				struct { int min,total; } i;
				struct { float min,total; } r;
			} echelle;         /* intervalle de colorisation                */
		} voyant;
	} geom;                    /* geometrie adoptee                         */
	struct {
		int bgnd;              /* type de GC pour le fond                   */
		int fgnd;              /* type de GC pour le texte                  */
		int interr;            /* type de GC pour l'interieur               */
	} gc;
	double min;                /* minimum affiche == lim.*.min              */
	double echelle;            /* echelle de deplacement                    */
	double dev;                /* deviation courante                        */
	double grad;               /* valeur d'une graduation                   */
	struct {
		int x,y;
		int l,h;
	} trace;                   /* indicateur si deja trace                  */
	short relief,cadran,bouton,interr;
} TypeInstrum,*Instrum;

/* Parametres a fournir a InstrumAdd */
typedef struct {
	double angle_min,angle_max;      /* degres p.r. verticale */
	double rayon; double graduation; /* rayon aiguille/bouton, longueur des graduations */
} TypeInstrumCirc,TypeInstrumCadrant,TypeInstrumPotar;
typedef struct {
	int longueur,largeur;      /* longueur,largeur voyant/bouton                */
	char horiz; char position; /* 1: graduations dessous/a droite, 0: les deux  */
	int nbchars;               /* a prevoir pour les graduations                */
	int graduation;            /* en plus de la largeur                         */
} TypeInstrumRect,TypeInstrumColonne,TypeInstrumGlissiere;
typedef struct {
	int nombre;
	int cote;     /* forme carree plutot que ronde */
} TypeInstrumRadio;
typedef struct {
	int hauteur,largeur;
} TypeInstrumLevier;
typedef struct {
	int longueur,largeur; /* largeur dans le sens de l'axe de bascule  */
	char horiz;           /* sens de la bascule (perp. axe de bascule) */
} TypeInstrumBascule;
typedef struct {
	OpiumColor *fond;          /* table de couleurs de fond               */
} TypeInstrumVoyant;

Instrum InstrumInt(INS_FORME forme, void *parms, int *adrs, int min, int max);
Instrum InstrumShort(INS_FORME forme, void *parms, short *adrs, short min, short max);
Instrum InstrumFloat(INS_FORME forme, void *parms, float *adrs, float min, float max);
Instrum InstrumDouble(INS_FORME forme, void *parms, double *adrs, double min, double max);
Instrum InstrumList(INS_FORME forme, void *parms, int *adrs, char **liste);
Instrum InstrumListL(INS_FORME forme, void *parms, int64 *adrs, char **liste);
Instrum InstrumListS(INS_FORME forme, void *parms, short *adrs, char **liste);
Instrum InstrumListB(INS_FORME forme, void *parms, char *adrs, char **liste);
Instrum InstrumKey(INS_FORME forme, void *parms, int *adrs, char *modeles);
Instrum InstrumKeyL(INS_FORME forme, void *parms, int64 *adrs, char *modeles);
Instrum InstrumKeyS(INS_FORME forme, void *parms, short *adrs, char *modeles);
Instrum InstrumKeyB(INS_FORME forme, void *parms, char *adrs, char *modeles);
Instrum InstrumBool(INS_FORME forme, void *parms, char *adrs);
void InstrumCircDim(TypeInstrumCirc *cercle, int rayon);
void InstrumRectDim(TypeInstrumRect *rect, int longueur, int largeur);
void InstrumLevierDim(TypeInstrumLevier *levier, int hauteur, int largeur);
void InstrumBascDim(TypeInstrumBascule *bascule, int longueur, int largeur);
void InstrumCircGrad(TypeInstrumCirc *cercle, int graduation);
void InstrumRectGrad(TypeInstrumRect *rect, int graduation);
void InstrumRectHoriz(TypeInstrumRect *rect, char horiz);
void InstrumBascHoriz(TypeInstrumBascule *bascule, char horiz);
int InstrumCircRayon(TypeInstrumCirc *cercle);
int InstrumRectLngr(TypeInstrumRect *rect);
int InstrumRectLarg(TypeInstrumRect *rect);
int InstrumLevierHaut(TypeInstrumLevier *levier);
int InstrumBascLngr(TypeInstrumBascule *bascule);
void InstrumTitle(Instrum i, char *texte);
void InstrumILimits(Instrum i, int min, int max);
void InstrumRLimits(Instrum i, float min, float max);
void InstrumGradSet(Instrum i, float grad);
void InstrumGradAuto(Instrum i);
void InstrumGradLog(Instrum i, char log);
void InstrumSupport(Instrum i, char support);
void InstrumFormat(Instrum i, char *format);
void InstrumCouleur(Instrum i, int indic);
void InstrumOnChange(Instrum i, TypeFctn fctn, void *arg);
void *InstrumChangeArg(Instrum i);
void OpiumInstrumColorNb(Cadre cdr, Cadre fait, short *nc);
void OpiumInstrumColorSet(Cadre cdr, WndFrame f);
int OpiumSizeInstrum(Cadre cdr, char from_wm);
int OpiumRefreshInstrum(Cadre cdr);
int InstrumRefreshVar(Instrum instrum);
int OpiumRunInstrum(Cadre cdr, WndAction *e);
void InstrumDelete(Instrum instrum);

#pragma mark Infos
/* ............................... infos.h .................................. */

typedef struct {
	Cadre cdr;
	int   haut,larg;
	char *texte;
} TypeInfo,*Info;

Info InfoCreate(int lignes, int colonnes);
void InfoDelete(Info info);
void InfoErase(Info info);
void InfoTitle(Info i, char *texte);

int  InfoWrite(Info info, int ligne, char *fmt, ...);
int  InfoInsert(Info info, int ligne, int col, char *fmt, ...);
Info InfoNotifie(Info info, char *fmt, ...);
// int  InfoFloat(Info info, int ligne, char *fmt,
// 	float p0,float p1,float p2,float p3,float p4,float p5,float p6,float p7);
// int  InfoFloat(Info info, int ligne, char *fmt, ...);
// int  InfoDouble(Info info, int ligne, char *fmt,
// 	double p0,double p1,double p2,double p3,double p4,double p5,double p6,double p7);
// int  InfoDouble(Info info, int ligne, char *fmt, ...);
int  OpiumSizeInfo(Cadre cdr, char from_wm);
int  OpiumRefreshInfo(Cadre cdr);
int  OpiumRunInfo(Cadre cdr, WndAction *e);

#pragma mark Figures
/* .............................. figures.h ................................. */

#define FIG_MAXTEXTE 32

typedef enum {
	FIG_DROITE = 0,
	FIG_LIEN,
	FIG_ZONE,
	FIG_ICONE,
	FIG_CADRE,
	FIG_NBTYPES
} FIG_TYPE;
OPIUM_VAR char *FigTypeNom[FIG_NBTYPES+1]
#ifdef OPIUM_C
= { "droite", "lien", "zone", "icone", "cadre", "\0" }
#endif
;

typedef enum {
	FIG_CENTRE = 0,
	FIG_A_GAUCHE,
	FIG_A_DROITE,
	FIG_EN_DESSOUS,
	FIG_AU_DESSUS,
	FIG_HAUT,
	FIG_BAS,
	FIG_NBCOTES
} FIG_COTE;

//typedef enum {
//	FIG_COUL_DESSIN = WND_GC_BASE, // WND_GC_NOIR commun avec BRD
//	FIG_COUL_ICONE,
//	FIG_COUL_NB
//} FIG_COULEURS;

typedef struct {
	short dx,dy;
	unsigned char relief;
	WndColorLevel r,g,b;
} TypeFigDroite;
typedef struct {
	short dx,dy,coude;
	unsigned char horiz;
	unsigned char relief;
	WndColorLevel r,g,b;
} TypeFigLien;
typedef struct {
	short larg,haut;
	unsigned char support;
	WndColorLevel r,g,b;
} TypeFigZone;
typedef TypeWndIcone TypeFigIcone;

typedef struct StructFigure {
	Cadre cdr;
	FIG_COTE cote_texte;
	char ecrit; // WND_GC_COUL
	char type;
	char forme_tempo;
	union {
		TypeFigDroite *droite;
		TypeFigLien   *lien;
		TypeFigZone   *zone;
		TypeFigIcone  *icone;
		void          *adrs;
	} forme;
	char texte[FIG_MAXTEXTE];
	union {
		WndContextPtr gc;
		WndColor *c;
	} p;
	void *adrs;
	short dessin,icone; /* colorisation */
	int (*fctn)(struct StructFigure *fig, WndAction *e);
} TypeFigure,*Figure;

typedef int (*TypeFigFctn)(Figure fig, WndAction *e);

Figure FigureCreate(char type, void *forme, void *adrs, TypeFigFctn fctn);
void FigureChangeForme(Figure fig, char type, void *forme);
void FigureFormeTempo(Figure fig, char tempo);
void FigureDelete(Figure fig);
void FigureErase(Figure fig);
void FigureTitle(Figure fig, char *texte);
void FigureTexte(Figure fig, char *texte);
void FigureLegende(Figure fig, FIG_COTE cote, char *texte);
void FigureEcrit(Figure fig, WND_GC_COUL ecrit);
void FigureColor(Figure fig, WndColorLevel r, WndColorLevel g, WndColorLevel b);
char FigureRedessine(Figure fig, char plus_fond);

void OpiumFigureColorNb(Cadre cdr, Cadre fait, short *nc);
void OpiumFigureColorSet(Cadre cdr, WndFrame f);
int  OpiumSizeFigure(Cadre cdr, char from_wm);
int  OpiumRefreshFigure(Cadre cdr);
int  OpiumRunFigure(Cadre cdr, WndAction *e);

#pragma mark Graphes
/* ............................... graphs.h ................................. */

/*
**    Definition des tableaux (GraphData->parm):
**
**     7      6      5      4      3      2      1      0
**  --------------------------------------------------------
**  |    GRF_     |     GRF_    |    GRF_     |    GRF_    |
**  |    AXIS     |     ADRS    |    TYPE     |    TRAN    |
**  --------------------------------------------------------
**
**    GRF_AXIS = 00 tableau de X, = 10 tableau de Y, = 01 tableau de Z, = 11 image directe.
**    GRF_ADRS = 0 tableau, = 1 fonction, =2 tableau de (type,couleur), =3 index
**                 si index: valeur de depart dans fctn, valeur du pas dans fctn+1
**    GRF_TYPE = 0 entiers 32 bits, = 1 entiers 16 bits, = 2 flottants [ = 3 double? ]
**    GRF_TRAN = 0 pas de transformation, = 1 log decimal, = 2 gamma, = 3 pas de trace
**                 attention: inutilisable en externe a la librairie (l'utilisateur
**                 voit GRF_MODE a la place)
**
**
**    Types de traces (GraphData->trace):
**
**     7      6      5      4      3      2      1      0
**  --------------------------------------------------------
**  |    GRF_    |        GRF_        |        GRF_        |
**  |    MODE    |        LINE        |        WIDZ        |
**  --------------------------------------------------------
**
**    GRF_MODE = 0 courbe, = 1 histogramme, = 2 erreur, = 3 points   si axe Y
**    GRF_LINE = 0 a 7 type de la trace ou type de point
**    GRF_WIDZ = 0 a 7 epaisseur de la trace.
**
*/
#define GRF_MAX_DATA 64  /* soit 32 courbes maxi par graphe */
#define GRF_MAX_TITRE 32 /* maxi titre de tableau ajoute */

/* ..... Definition du tableau (data->parm) ..... */

#define GRF_AXIS  0xC0      /* 0 ou parm>0: x ou z / 1 ou parm<0: y           */
#define GRF_ADRS  0x30      /* 0: tableau / 1: fonction / 2: poly / 3: index  */
#define GRF_TYPE  0x0C      /* 0: int / 1: short / 2: float / 3: double       */
#define GRF_TRAN  0x03      /* 0: aucune / 1: log10 / 2: gamma / 3: pas trace */

#define GRF_NONE  0x00
#define GRF_LOGD  0x01
#define GRF_GAMMA 0x02
#define GRF_NODSP 0x03

#define GRF_INT   0x00
#define GRF_SHORT 0x04
#define GRF_FLOAT 0x08
#define GRF_DBLE  0x0C

#define GRF_ARRAY 0x00
#define GRF_FCTN  0x10
#define GRF_INDIV 0x20
#define GRF_INDEX 0x30

#define GRF_XAXIS 0x00
#define GRF_YAXIS 0x80
#define GRF_ZAXIS 0x40
#define GRF_IMAGE 0xC0

#define GRF_ICONE 0x100

/* ..... Trace de la courbe (data->trace) ..... */

OPIUM_VAR unsigned char GraphWidzDefaut[WND_NBQUAL];

#define GRF_MODE 0xC0      /* 0: courbe / 1: histo / 2: erreurs / 3: points  */
#define GRF_LINE 0x38      /* type de trace (0-7)                            */
#define GRF_WIDZ 0x07      /* largeur de trace (0-7)                         */

#define GRF_SHIFT_MODE 6   /* decalages en bits de l'info mode dans <trace>  */
#define GRF_SHIFT_LINE 3   /* decalages en bits de l'info line dans <trace>  */
#define GRF_SHIFT_WIDZ 0   /* decalages en bits de l'info widz dans <trace>  */

typedef enum {
	GRF_LIN = 0,
	GRF_HST,
	GRF_ERR,
	GRF_PNT,
	GRF_NBMODES
} GRF_MODES;
OPIUM_VAR char *GraphModeName[GRF_NBMODES+1]
#ifdef OPIUM_C
 = { "courbe", "histo", "erreurs", "points", "\0" }
#endif
;
#define GRF_CODE_LIN (GRF_LIN << GRF_SHIFT_MODE)
#define GRF_CODE_HST (GRF_HST << GRF_SHIFT_MODE)
#define GRF_CODE_ERR (GRF_ERR << GRF_SHIFT_MODE)
#define GRF_CODE_PNT (GRF_PNT << GRF_SHIFT_MODE)

/* implique: pas de trace de type points avec largeur de trait == 7 */
#define GRF_NODISPLAY (GRF_CODE_PNT | GRF_WIDZ)
#define GRF_HIDDEN(trace) ((trace & GRF_NODISPLAY) == GRF_NODISPLAY)
#define GRF_SHOWN(trace) ((trace & GRF_NODISPLAY) != GRF_NODISPLAY)
#define GRF_HIDE(trace) { trace = (trace & GRF_LINE) | GRF_NODISPLAY; }
#define GRF_SHOW(trace,mode) { trace = (mode << GRF_SHIFT_MODE) | (trace & GRF_LINE) | 1; }

/* Si data->trace indique un mode 'points' (#3=GRF_PNT),
   le type des points est dans le type de trace (data->trace{GRF_LINE})
 ATTENTION: les valeurs 6 et 7 sont reservees! */
typedef enum {
	GRF_POINT = 0,
	GRF_CROIX,
	GRF_TRGLE,
	GRF_CARRE,
	GRF_STAR,
	GRF_ROND,
	GRF_NBMARKS,
	GRF_TOUTE = 7
} GRF_MARKS;
OPIUM_VAR char *GraphMarkName[GRF_NBMARKS+1]
#ifdef OPIUM_C
 = { "point", "croix", "triangle", "carre", "etoile", "cercle", "\0" }
#endif
;
#define GRF_ABSENT GRF_NBMARKS

#define GRF_CODE_POINT (GRF_POINT << GRF_SHIFT_LINE)
#define GRF_CODE_CROIX (GRF_CROIX << GRF_SHIFT_LINE)
#define GRF_CODE_STAR  (GRF_STAR  << GRF_SHIFT_LINE)
#define GRF_CODE_TRGLE (GRF_TRGLE << GRF_SHIFT_LINE)
#define GRF_CODE_CARRE (GRF_CARRE << GRF_SHIFT_LINE)
#define GRF_CODE_ROND  (GRF_ROND  << GRF_SHIFT_LINE)

#define GRF_CODE_TOUTE (GRF_TOUTE << GRF_SHIFT_LINE)

#define GRF_DIM_POINT 1
#define GRF_DIM_CROSS 6
#define GRF_DIM_STAR 6
#define GRF_DIM_TRGLE 4
#define GRF_DIM_CARRE 4
#define GRF_DIM_ROND 4

/* ..... Style de la courbe (data->style) ..... */

#define GRAPH_HISTO_FLOTTANT 0x01

typedef enum {
	GRF_DETAIL_LISSE = 0,
	GRF_DETAIL_MINMAX,
	GRF_DETAIL_TOUT,
	GRF_DETAIL_NB
} GRF_DETAILS;
#define GRF_SHIFT_DETAIL 1 /* decalages en bits de l'info detail dans <style>  */
#define GRAPH_DETAIL  (3 << GRF_SHIFT_DETAIL)

#define GRF_CODE_LISSE  (GRF_DETAIL_LISSE  << GRF_SHIFT_DETAIL)
#define GRF_CODE_MINMAX (GRF_DETAIL_MINMAX << GRF_SHIFT_DETAIL)
#define GRF_CODE_TOUT   (GRF_DETAIL_TOUT   << GRF_SHIFT_DETAIL)

/* ..... Trace des axes (graph->mode) ..... */

#define GRF_NBAXES  0x0003 /* masque pour le nb. d'axes */

#define GRF_0AXES   0x0000
#define GRF_2AXES   0x0001
#define GRF_4AXES   0x0002

#define GRF_GRID    0x0004
#define GRF_NOGRAD  0x0008
#define GRF_LEGEND  0x0010
#define GRF_QUALITE 0x0020

typedef enum {
	GRF_GRADNONE = 0,
	GRF_GRADINF,
	GRF_GRADSUP,
	GRF_GRADBOTH
} GRF_GRADTYPE;
typedef enum {
	GRF_ZONE_XMIN = 0,
	GRF_ZONE_YMIN,
	GRF_ZONE_XMAX,
	GRF_ZONE_YMAX,
	GRF_ZONE_XGRAD,
	GRF_ZONE_YGRAD,
	GRF_MAX_ZONES  /* nombre de zones du graphe associees a une action */
} GRF_CODE_ZONES;

/* ..... Divers ..... */
#define GRF_UNDEFINED -1      /* Echelle encore non definie     */
#define GRF_DIRECT    0xFFF0  /* Echelle directe (1 = 1 pixel)  */

//typedef enum {
//	GRF_COUL_POMME = WND_GC_BASE,
//	GRF_COUL_GRILLE,
//	GRF_COUL_AJOUT,
//	GRF_COUL_NB
//} GRF_COULEURS;

#define GRF_NORMAL WND_GC(f,WND_GC_TEXT)
//#define GRF_POMME  WND_GC(f,GRF_COUL_POMME)
//#define GRF_GRILLE WND_GC(f,GRF_COUL_GRILLE)
//#define GRF_AJOUT  WND_GC(f,GRF_COUL_AJOUT)
#define GRF_POMME  WND_GC(f,graph->pomme)
#define GRF_GRILLE WND_GC(f,graph->grille)
#define GRF_AJOUT  WND_GC(f,graph->ajout)

typedef enum {
	GRF_ICN = 0,
	GRF_BMP,
	GRF_NOM,
	GRF_RGB,
	GRF_LUT
} GRF_COULEUR_FORMATS;

typedef enum {
	GRF_LUT_ALL = 0,
	GRF_LUT_RED,
	GRF_LUT_GREEN,
	GRF_LUT_BLUE,
	GRF_LUT_NBTYPES
} GRF_LUT_TYPES;

#define GRF_RGB_RED       WND_COLOR_MAX,               0,               0
#define GRF_RGB_ORANGE    WND_COLOR_MAX,  WND_COLOR_DEMI,               0
#define GRF_RGB_YELLOW    WND_COLOR_MAX,   WND_COLOR_MAX,               0
#define GRF_RGB_GREEN                 0,   WND_COLOR_MAX,               0
#define GRF_RGB_TURQUOISE             0,   WND_COLOR_MAX,   WND_COLOR_MAX
#define GRF_RGB_BLUE                  0,               0,   WND_COLOR_MAX
#define GRF_RGB_VIOLET   WND_COLOR_DEMI,               0,   WND_COLOR_MAX
#define GRF_RGB_MAGENTA   WND_COLOR_MAX,               0,   WND_COLOR_MAX
#define GRF_RGB_WHITE     WND_COLOR_MAX,   WND_COLOR_MAX,   WND_COLOR_MAX
#define GRF_RGB_BLACK                 0,               0,               0
#define GRF_RGB_GREY             0xD000,          0xD000,          0xD000

/* ..... Structures ..... */
typedef struct {
	unsigned char trace;
	unsigned char r,g,b;
} TypeGraphIndiv;

/*
**  GraphScale : Un axe de graphe
**
**  num   : numero du tableau (num = -2: x ou y == position en pixels)
**  cols  : nombre de colonnes pour le texte des graduations en Y
**  typegrad : facon de tracer les graduations (type GRF_GRADTYPE)
**  reel  : vrai si type float (type int sinon)
**  pret  : vrai si valeurs a jour
**  minauto, maxauto, gradauto: vrais si min, max et grad sont calcules automatiquement
**  logd  : vrai si axe en log
**  logmin: valeur du min utilisateur si axe en log
**    -- valeurs en unites utilisateur (int ou float)
**  min   : minimum dans le tableau.
**  max   : maximum dans le tableau.
**  ech   : echelle.
**  axe   : position de l'autre axe
**  grad  : valeur d'une graduation
**  g1    : valeur de la premiere graduation a tracer
**  chgt_unite : fonction eventuelle de changement d'unite des graduations
**    -- valeurs en unites ecran (nombres de pixels ecran)
**  total : dimension du graphe.
**  pixel : taille pixel ecran en unite utilisateur (== total/ech)
**  marge : marge du graphe.
**  gmin  : numero de graduation minimum compte tenu de l'ascenseur
**  format: format d'affichage
*/
typedef struct {
	char autom,log;
	union {
		struct { int   min,max; } i;
		struct { float min,max; } r;
	} lim;
} GraphAxeParm;

typedef struct {
	short num; short cols; unsigned char typegrad;
	char reel; char pret; char minauto,maxauto,gradauto;
	char logd;
	double logmin;
	union {
		struct { int   min,max,axe; } i;
		struct { float min,max,axe; } r;
	} u;
	float grad,g1,ech,pixel;
	float (*chgt_unite)(void *, int, float);
	int  marge_min,marge_max,marge_totale,utile,gmin;
	unsigned int lngr; int  zoom;
	char format[12];
	char titre[80];
} GraphScale;

typedef struct {
	char type;  /* GRF_NOM (nom de couleur), GRF_RGB ({R,G,B}) ou GRF_LUT */
	union {
		char nom[WND_NBQUAL][WND_MAXCOULEUR];
		OpiumColor rgb[WND_NBQUAL];
		struct {
			int dim;
			WndColor *val;
		} lut;
	} def;
} GraphColor;

/*
**  GraphData : Un jeu de valeurs a utiliser dans un graphe
**
**  parm : 8 bits pour definir le tableau (voir ci-dessus)
**  trace: 8 bits pour definir le trace (voir ci-dessus)
**  gc   : contexte graphique (couleur,trait).
**  dim  : nombre de points dans le tableau
**  min  : premier point a tracer
**  max  : dernier point a tracer (si max < min, buffer circulaire)
**  gamma: table de transformation, appliquee si GRF_TRAN == GRF_GAMMA
**         (transformation lineaire entre xmin et xmax)
**  fctn : adresse du tableau de valeurs ou de la fonction.
*/
typedef struct {
	unsigned char parm,style,trace[WND_NBQUAL];
	char titre[GRF_MAX_TITRE];
	GraphColor couleur;
	WndContextPtr gc[WND_NBQUAL];
	int dim,min,max;
	union {
		struct { float xmin,xmax,vmin,vmax; } r;
		struct { int   xmin,xmax,vmin,vmax; } i;
	} gamma;
	union { struct { float v; } r; struct { int   v; } i; } of7;
	void *fctn;
} TypeGraphData,*GraphData;

/*
**  Graph         : Un graphe au complet (inclue 2 GraphScale et <n> GraphData)
**
**  cdr  : pointeur sur l'objet OPIUM.
**  mode : options de trace des axes
**  haut : hauteur du graphe en pixel.
**  larg : largeur du graphe en pixel.
**  zoom : rapport largeur demandee / largeur affichee
**  max  : nombre maximum de tableau de valeurs.
**  dim  : nombre actuel de tableau de valeurs.
**  grad : longueur d'une graduation (sera double si axe au milieu)
**  of7  : distance (texte des graduations - trait)
**  axe  : tableau contenant les caracteristiques des deux axes du graphe.
**  data : tableau des tableaux de valeurs
*/
typedef struct StructGraph {
	Cadre  cdr;
//	unsigned int haut,larg;
	unsigned short mode;
	short  max,dim;
	short  grad,of7;
	char   pret;       /* =1 des que les GC sont definis */
	int mouse[2];
	GraphScale axe[2];
	unsigned short mode_sauve;   /* utilise par GraphDialogExec */
	GraphScale axe_sauve[2];     /* utilise par GraphDialogExec */
	GraphData data_ptr[GRF_MAX_DATA];
	GraphData data;
	struct {
		unsigned int xmin,ymin;  /* coin gauche-haut */
		unsigned int xmax,ymax;  /* coin droit-bas   */
	} zone[GRF_MAX_ZONES];
	char usermouse[32];
	short pomme,grille,ajout;
	int (*ajouts)(struct StructGraph *graph, WndAction *e);
	int (*onclic)(struct StructGraph *graph, WndAction *e);
} TypeGraph,*Graph;

typedef int (*TypeGraphFctn)(Graph graph, WndAction *e);
/* si retour == 0: est suivie par reticule et zoom comme normal;
                1: pas d'autre action sur le graphge;
               -1: idem mais ferme le graphe (retour PNL_OK) */
#define GraphDim(graph) (graph? graph->dim: 0)

void GraphLargeurTraits(int qual, unsigned char width);
void GraphQualiteDefaut(char qual);
Graph GraphCreate(int larg, int haut, short max);
void GraphDelete(Graph graph);
void GraphTitle(Graph graph, char *texte);
void GraphMode(Graph graph, unsigned short mode);
void GraphErase(Graph graph);
int GraphConnect(Graph graph, int type, void *adrs, int dim);
int GraphAdd(Graph graph, int type, void *adrs, int dim);
int GraphInclude(Graph graph, int type, void *adrs, int dim);
void GraphArrayWindowSize(int larg, int haut);
Graph GraphArrayDisplay(char *nom, char type_x, void *dx, char type_table, void *table, int lngr);
void GraphArrayHold(Graph graph, char hold);
void GraphArrayAdd(Graph g, char type_table, void *table, int lngr);
void GraphArrayName(Graph graph, int num, char *nom);
void GraphArrayRGB(Graph graph, int num, WndColorLevel r, WndColorLevel g, WndColorLevel b);
void GraphArrayDelete(Graph graph);
Graph GraphCreateIcone(WndIcone icone, unsigned int type);
void GraphAjoute(Graph graph, TypeGraphFctn fctn);
void GraphOnClic(Graph graph, TypeGraphFctn fctn);
int GraphDimGet(Graph graph);
void *GraphDataGet(Graph graph, int num, int *dim);
WndColor *GraphLUTCreate(int dim, int type);
int GraphDataReplace(Graph graph, int num, int type, void *adrs, int dim);
char GraphDataDisconnect(Graph graph, int num);
char GraphDataRemove(Graph graph, int num);
int GraphDataTranslate(Graph graph, int num, char transfo);
int GraphDataIntGamma(Graph graph, int num, int xmin, int xmax, int vmin, int vmax);
int GraphDataFloatGamma(Graph graph, int num, float xmin, float xmax, float vmin, float vmax);
int GraphDataIntOffset(Graph graph, int num, int ival);
int GraphDataFloatOffset(Graph graph, int num, float rval);
int GraphDataName(Graph graph, int num, char *nom);
int GraphDataBmp(Graph graph, int num, WndColor *lut, int dim);
int GraphDataColor(Graph graph, int num, char *nom);
int GraphDataRGB(Graph graph, int num, WndColorLevel r, WndColorLevel g, WndColorLevel b);
int GraphDataSupportColor(Graph graph, int num, int qual, char *nom);
int GraphDataSupportRGB(Graph graph, int num, int qual, WndColorLevel r, WndColorLevel g, WndColorLevel b);
int GraphDataLUT(Graph graph, int num, WndColor *lut, int dim);
int GraphDataTrace(Graph graph, int num, int mode, int type);
int GraphDataSupportTrace(Graph graph, int num, int qual, int mode, int type, int width);
char GraphDataStyle(Graph graph, int num, char style);
void GraphDataStart(Graph graph, int num, int min);
void GraphDataEnd(Graph graph, int num, int max);
void GraphDataUse(Graph graph, int num, int nb);
void GraphDataScroll(Graph graph, int num, int nb);
void GraphDataRescroll(Graph graph, int num, int nb);
void GraphStart(Graph graph, int min);
void GraphEnd(Graph graph, int max);
void GraphReset(Graph graph);
void GraphUse(Graph graph, int nb);
void GraphScroll(Graph graph, int nb);
void GraphRescroll(Graph graph, int nb);
void GraphResize(Graph graph, int larg, int haut);
void GraphZoom(Graph graph, unsigned int zoomh, unsigned int zoomv);
void GraphDump(Graph graph);
int GraphAxisDefine(Graph graph, int num);
int GraphAxisTitle(Graph graph, unsigned char axe, char *titre);
int GraphAxisSet(Graph graph, unsigned char axe);
int GraphAxisReset(Graph graph, unsigned char axe);
int GraphAxisFormat(Graph graph, unsigned char axe, char *format);
int GraphAxisIntRange(Graph graph, unsigned char axe, int imin, int imax);
int GraphAxisFloatRange(Graph graph, unsigned char axe, float rmin, float rmax);
int GraphGetIntRange(Graph graph, unsigned char axe, int *imin, int *imax);
int GraphGetFloatRange(Graph graph, unsigned char axe, float *rmin, float *rmax);
int GraphAxisAutoRange(Graph graph, unsigned char axe);
int GraphAxisIntMin(Graph graph, unsigned char axe, int imin);
int GraphAxisFloatMin(Graph graph, unsigned char axe, float rmin);
int GraphAxisAutoMin(Graph graph, unsigned char axe);
int GraphAxisIntMax(Graph graph, unsigned char axe, int imax);
int GraphAxisFloatMax(Graph graph, unsigned char axe, float rmax);
int GraphAxisAutoMax(Graph graph, unsigned char axe);
int GraphAxisIntGrad(Graph graph, unsigned char axe, int igrad);
int GraphAxisFloatGrad(Graph graph, unsigned char axe, float rgrad);
int GraphAxisAutoGrad(Graph graph, unsigned char axe);
int GraphAxisChgtUnite(Graph graph, unsigned char axe, float (*fctn)(Graph, int, float));
int GraphAxisLog(Graph graph, unsigned char axe, char logd);
char GraphAxisIsLog(Graph graph, unsigned char axe);
int GraphAxisIntGet(Graph graph, unsigned char axe, int *imin, int *imax, int *igrad);
int GraphAxisFloatGet(Graph graph, unsigned char axe, float *rmin, float *rmax, float *rgrad);
int GraphAxisMarge(Graph graph, unsigned char axe);
int GraphUserScrollX(Graph graph, int ix, float rx);
int GraphUserScrollY(Graph graph, int iy, float ry);
int GraphScreenXFloat(Graph graph, float r);
int GraphScreenXInt(Graph graph, int r);
int GraphScreenYFloat(Graph graph, float r);
int GraphScreenYInt(Graph graph, int r);
void GraphAjusteParms(Graph graph, unsigned char axe, GraphAxeParm *parms);
int GraphBlank(Graph graph);
void OpiumGraphColorNb(Cadre cdr, Cadre fait, short *nc);
void OpiumGraphColorSet(Cadre cdr, WndFrame f);
int OpiumSizeGraph(Cadre cdr, char from_wm);
int OpiumRefreshGraph(Cadre cdr);
int OpiumRunGraph(Cadre cdr, WndAction *e);
int GraphTrace(Cadre cdr, int ajoutes);
void GraphPrint(Graph graph, int x, int y, char *texte);
void GraphPosUser(GraphScale *scale, int x, int *ival, float *rval);
void GraphEventUserInt(Graph graph, WndAction *e, int *ix, int *iy);
void GraphEventUserFloat(Graph graph, WndAction *e, float *rx, float *ry);
char GraphGetLast(int *ix, float *rx, int *iy, float *ry);
int GraphGetPoint(Graph graph, int *ix, float *rx, int *iy, float *ry);
int GraphGetRect(Graph graph, int *ix0, float *rx0, int *iy0, float *ry0,
	int *ix1, float *rx1, int *iy1, float *ry1);
void GraphZoneAcces(Graph g, int num);
int GraphParmsSave(Graph graph);
void GraphDialogCreate(void);
int GraphDialogExec(Cadre cdr /*, WndAction *u */);

#pragma mark Terminaux
/* ............................... terms.h .................................. */
typedef struct {
	Cadre cdr;
	int haut,larg;           /* nombre de lignes & colonnes maxi affichees                    */

	int maxlignes,nblignes;  /* nombre de lignes gerees, nombre de lignes disponibles         */
	int *ligne;              /* tampon des positions des lignes (1er caractere dans <buffer>) */
	int premiere,derniere;   /* premiere et derniere lignes dispo (premiere=0 ou derniere+1)  */
	int courante;            /* premiere ligne affichee (inutilise, avec ascenseurs)          */
	int col1;                /* colonne de debut de la premiere ligne dispo                   */

	char *buffer;            /* tampon des caracteres                                         */
	int taille,debut,nbcars; /* taille du tampon des caracteres, debut logique, nb caracteres dispo */
	char hold,printed;
	short normal;            /* couleur d'ecriture normale                                    */
} TypeTerm,*Term;

Term TermCreate(int ligs, int cols, int taille);
void TermEmpty(Term term);
void TermDelete(Term term);
void TermTitle(Term t, char *texte);
void TermColumns(Term term, int cols);
void TermLines(Term term, int ligs);
void TermHold(Term term);
void TermRelease(Term term);
void TermBlank(Term term);
void TermAdd(Term term, char *ligne);

int  TermPrint(Term term, char *fmt,  ...);
//void TermFloat(Term term, char *format,
//	float p0, float p1, float p2, float p3, float p4,
//	float p5, float p6, float p7, float p8, float p9);
// void TermFloat(Term term, char *format, ...);
//void TermDouble(Term term, char *format,
//	double p0, double p1, double p2, double p3, double p4,
//	double p5, double p6, double p7, double p8, double p9);
// void TermDouble(Term term, char *format, ...);

int TermScroll(Term term, int depl);
void TermDump(Term term, FILE *f);
void OpiumTermColorNb(Cadre cdr, Cadre fait, short *nc);
void OpiumTermColorSet(Cadre cdr, WndFrame f);
int OpiumSizeTerm(Cadre cdr, char from_wm);
int OpiumRefreshTerm(Cadre cdr);
int OpiumRunTerm(Cadre cdr, WndAction *e);

#pragma mark Panels
/* .............................. panels.h .................................. */
// typedef int (*TypeItemFctn)(char *, int *, void *);
// typedef int (*TypeOpiumIntFctn)(int);

typedef enum {
	PNL_REMARK = 0, PNL_SEPARE, PNL_PSWD, PNL_FILE, PNL_TEXTE,
	PNL_LONG, PNL_INT, PNL_SHORT, PNL_CHAR, PNL_OCTET, PNL_LETTRE,
	PNL_LHEX, PNL_HEX, PNL_SHEX, PNL_BYTE, PNL_BOOL,
	PNL_FLOAT, PNL_DBLE, PNL_ADU,
	PNL_LISTE, PNL_LISTL, PNL_LISTD, PNL_LISTS, PNL_LISTB,
	PNL_KEY, PNL_KEYL, PNL_KEYS, PNL_KEYB,
	PNL_USER, PNL_BOUTON,
	PNL_NBTYPES
} PNL_TYPE;

typedef enum {
	PNL_NOMODIF = 0,
	PNL_RESET, PNL_APPLY, PNL_CANCEL, PNL_OK,
	PNL_NBREPLIES
} PNL_REPLIES;

typedef enum {
	PNL_WRITE = -1,
	PNL_VERIFY,
	PNL_READ
} PNL_STEPS;

typedef enum {
	PNL_PROTEGE = 0,
	PNL_MODIFIABLE,
	PNL_MODIFIE,
	PNL_NBETATS
} PNL_ETATS;

//typedef enum {
//	PNL_COUL_TRACE = WND_GC_BASE,
//	PNL_COUL_MODS,              // WND_GC_MODS commun avec BRD
//	PNL_COUL_HILITE,            // WND_GC_HILITE commun avec BRD
//	PNL_COUL_NB
//} PNL_COULEURS;

// #define PNL_NORMAL WND_GC(f,WND_GC_TEXT)
//#define PNL_HILITE WND_GC(f,PNL_COUL_HILITE)
//#define PNL_MODS   WND_GC(f,PNL_COUL_MODS)
//#define PNL_TRACE  WND_GC(f,PNL_COUL_TRACE)
#define PNL_TRACE  WND_GC(f,panel->trace)
// #define PNL_HILITE WND_GC(f,panel->hilite)
// #define PNL_MODS   WND_GC(f,panel->mods)

#define PNL_SECURE  0x00
#define PNL_DIRECT  0x01
#define PNL_INBOARD 0x02
#define PNL_RDONLY  0x04
#define PNL_BYLINES 0x08
#define PNL_EPARS   0x10
#define PNL_SELECTR 0x20
#define PNL_EXCLUSF 0x40

#define PNL_MAXCOLS 32
#define PNL_MAXVAL 16  /* dimension maxi du texte des valeurs  */
#define PNL_MAXFMT 12  /* dimension maxi du format des valeurs */

typedef struct {
	char *texte;
	char *sel;
} TypeOpiumSelecteur,*OpiumSelecteur;
#define OPIUM_SELECT_FIN {0,0}

typedef char (*TypePanelFctnMod)(void *, char *, void *);
typedef int (*TypePanelFctnRef)(void *, int, void *);
// rend "TypePanelFctnRef rtn;" equivalent a "int (*rtn)(void *, int, void *);"
// et une 'TypePanelFctnRef' doit rendre un 'int'

typedef struct {
	char *texte;               /* texte de presentation                   */
	char *traduit;             /* le meme, localise                       */
	int   num;                 /* index dans le panel                     */
	char  sel;                 /* vrai si item a priori modifiable        */
	char  mod;                 /* vrai si item reellement modifiable      */
	char  ecrit;               /* vrai si ptrval ecrit donc utilisable    */
	char  type;                /* int/short/ ...                          */
	char  souligne;            /* petit trait discret pour separation     */
	char  barreDR;             /* petit trait discret pour separation     */
	char  barreMI;             /* petit trait discret pour separation     */
	char  barreGA;             /* petit trait discret pour separation     */
	void *adrs;                /* adresse utilisateur (selon type)        */
	void *fctn;                /* fonction utilisateur (USER ou LISTx)    */
	char  limites;             /* vrai si limites a appliquer             */
	union {
		struct { int min,max; } i;
		struct { float min,max; } r;
	} lim;                     /* limites a appliquer                     */
	OpiumColor *coultexte;     /* couleur du texte                        */
	WndContextPtr gctexte;     /* GC correspondant                        */
	OpiumColor *fond;          /* table de couleurs de fond               */
	WndContextPtr *gc;         /* table des GC correspondants             */
	short nbfonds;             /* nombre d'elements d'icelles             */
	short numfond;             /* index du fond a utiliser                */
	union {
		struct { int min,total; } i;
		struct { float min,total; } r;
	} echelle;                 /* intervalle de colorisation              */
	int64  tempo;              /* valeur rendue par LISTx                 */
	short maxuser;             /* maxi octets stockes (TEXT/USER/LIST)    */
	short maxdisp;             /* maxi caracteres a afficher              */
	short first;               /* premier caractere affiche               */
	char  etat;                /* voir PNL_ETATS                          */
	short couleur;             /* couleur a utiliser (selon modif ou pas) */
	char  valeur[PNL_MAXVAL];  /* valeur modifiee en ASCII                */
	char *ptrval;              /* idem, si plus de PNL_MAXVAL caracteres  */
	short ptrdim;              /* nombre de caracteres dans ptrval        */
	char  format[PNL_MAXFMT];  /* format d'affichage                      */
	TypePanelFctnMod modif;    /* fctn(adrs_var,texte,arg) a executer si modif     */
	void *modif_arg;           /* ... et son(s) argument(s)  [..!..]               */
	TypePanelFctnRef rtn;      /* fctn(panel,item,arg) a executer sur <Return>     */
	void *rtn_arg;             /* ... et son(s) argument(s)  [..!..]               */
	TypePanelFctnRef exit;     /* fctn(panel,item,arg) a executer en sortie d'item */
	void *exit_arg;            /* ... et son(s) argument(s)  [..!..]               */
} TypePanelItem,*PanelItem;

typedef struct StructPanel {
	Cadre cdr;
	PanelItem items;
	short max,cur;              /* maximum et nombre courant d'items                */
	short nbcols,*col,*var;     /* nb, emplacement, larg. des colonnes              */
	short col1,var1;            /* larg. des colonnes si nbcols==1                  */
	short maxligs;              /* nombre de lignes maxi (max/nbcols)               */
	short nbligs;               /* nombre effectif de lignes non inclus les boutons */
	struct { short col; short lig; } prochaine; /* prochain emplacement d'item si >0   */
	char  overstrike, /* ecrit, */ not_to_refresh,entete_verrouillee;
	char  support;              /* support pour la zone 'variables'        */
	unsigned short mode;
	short xcurseur,ycurseur;    /* emplacement du curseur fenetre affichee */
	short icurseur,ccurseur;    /* idem, avant fenetre affichee            */
	short precedent;            /* item utilise avant evt actuel           */
	int (*on_apply)(struct StructPanel *, void *);
	int (*on_ok)(struct StructPanel *, void *);
	int (*on_reset)(struct StructPanel *, void *);
	void *reset_arg,*apply_arg,*ok_arg;   /* ... et leur(s) argument(s)              */
	char *bouton[PNL_NBREPLIES];
	short mods,hilite,trace;
} TypePanel,*Panel;

typedef int (*TypePanelFctnInt)(Panel, int);
typedef int (*TypePanelFctnArg)(Panel, void *);
typedef int (*TypePanelFctnItem)(Panel, int, void *);
typedef char *(*TypePanelFctnButn)(void *);

Panel PanelCreate(int maxitems);
void PanelDelete(Panel panel);
void PanelErase(Panel panel);
Panel PanelDuplique(Panel p);
void PanelTitle(Panel panel, char *texte);
void PanelMode(Panel panel, unsigned short mode);
void PanelDansPlanche(Panel panel);
void PanelVerrouEntete(Panel panel, unsigned char verrou);
void PanelSupport(Panel panel, char support);
void PanelColumns(Panel panel, int cols);
void PanelBoutonText(Panel panel, int n, char *texte);
void PanelOnReset(Panel panel, TypePanelFctnArg fctn, void *arg);
void PanelOnApply(Panel panel, TypePanelFctnArg fctn, void *arg);
void PanelOnOK(Panel panel, TypePanelFctnArg fctn, void *arg);
void *PanelApplyArg(Panel panel);
void *PanelOKArg(Panel panel);
void PanelMaxChamp(short max);
void PanelPrepare(Panel panel, int col, int lig);
int PanelBlank(Panel panel);
int PanelRemark(Panel panel, char *texte);
int PanelSepare(Panel panel, char *texte);
int PanelPswd(Panel panel, char *texte, char *adrs, int lngr);
int PanelFile(Panel panel, char *texte, char *adrs, int lngr);
int PanelText(Panel panel, char *texte, char *adrs, int lngr);
int PanelLong(Panel panel, char *texte, int64 *adrs);
int PanelLHex(Panel panel, char *texte, int64 *adrs);
int PanelInt(Panel panel, char *texte, int *adrs);
int PanelHex(Panel panel, char *texte, int *adrs);
int PanelShort(Panel panel, char *texte, short *adrs);
int PanelSHex(Panel panel, char *texte, short *adrs);
int PanelAdu(Panel panel, char *texte, unsigned short *adrs);
int PanelChar(Panel panel, char *texte, char *adrs);
int PanelOctet(Panel panel, char *texte, unsigned char *adrs);
int PanelLettre(Panel panel, char *texte, char *adrs);
int PanelByte(Panel panel, char *texte, unsigned char *adrs);
int PanelBool(Panel panel, char *texte, char *adrs);
int PanelFloat(Panel panel, char *texte, float *adrs);
int PanelDouble(Panel panel, char *texte, double *adrs);
int PanelList(Panel panel, char *texte, char **liste, int *adrs, int lngr);
int PanelListL(Panel panel, char *texte, char **liste, long *adrs, int lngr);
int PanelListD(Panel panel, char *texte, char **liste, int64 *adrs, int lngr);
int PanelListS(Panel panel, char *texte, char **liste, short *adrs, int lngr);
int PanelListB(Panel panel, char *texte, char **liste, char *adrs, int lngr);
int PanelKey(Panel panel, char *texte, char *modeles, int *adrs, int lngr);
int PanelKeyL(Panel panel, char *texte, char *modeles, int64 *adrs, int lngr);
int PanelKeyS(Panel panel, char *texte, char *modeles, short *adrs, int lngr);
int PanelKeyB(Panel panel, char *texte, char *modeles, char *adrs, int lngr);
int PanelUser(Panel panel, char *texte, TypeArgUserFctn fctn, void *adrs, int lngr);
int PanelBouton(Panel panel, char *texte, TypePanelFctnButn fctn, void *adrs, int lngr);
Panel PanelSelecteur(OpiumSelecteur select, unsigned short exclusif);
// Panel PanelArgs(ArgDesc *desc);
#define PanelArgs(desc) PanelDesc(desc,0)
void PanelAddDesc(Panel p, ArgDesc *desc, int larg);
Panel PanelDesc(ArgDesc *desc, char avec);
void OpiumReadTitle(char *titre);
int OpiumReadPswd(char *texte, char *adrs, int lngr);
int OpiumReadFile(char *texte, char *adrs, int lngr);
int OpiumReadText(char *texte, char *adrs, int lngr);
int OpiumReadLong(char *texte, int64 *adrs);
int OpiumReadLHex(char *texte, int64 *adrs);
int OpiumReadInt(char *texte, int *adrs);
int OpiumReadHex(char *texte, int *adrs);
int OpiumReadShort(char *texte, short *adrs);
int OpiumReadSHex(char *texte, short *adrs);
int OpiumReadAdu(char *texte, unsigned short *adrs);
int OpiumReadChar(char *texte, char *adrs);
int OpiumReadOctet(char *texte, unsigned char *adrs);
int OpiumReadLettre(char *texte, char *adrs);
int OpiumReadByte(char *texte, char *adrs);
int OpiumReadBool(char *texte, char *adrs);
int OpiumReadFloat(char *texte, float *adrs);
int OpiumReadDouble(char *texte, double *adrs);
int OpiumReadList(char *texte, char **liste, int *adrs, int lngr);
int OpiumReadListL(char *texte, char **liste, long *adrs, int lngr);
int OpiumReadListD(char *texte, char **liste, int64 *adrs, int lngr);
int OpiumReadListS(char *texte, char **liste, short *adrs, int lngr);
int OpiumReadListB(char *texte, char **liste, char *adrs, int lngr);
int OpiumReadKey(char *texte, char *modeles, int *adrs, int lngr);
int OpiumReadKeyL(char *texte, char *modeles, int64 *adrs, int lngr);
int OpiumReadKeyS(char *texte, char *modeles, short *adrs, int lngr);
int OpiumReadKeyB(char *texte, char *modeles, char *adrs, int lngr);
int OpiumReadUser(char *texte, TypeArgUserFctn fctn, void *adrs, int lngr);
int OpiumReadBouton(char *texte, TypePanelFctnButn fctn, void *adr, int lngr);
void OpiumReadFormat(char *format);
void OpiumReadLngr(int lngr);
int PanelFormat(Panel panel, int num, char *format);
int PanelLngr(Panel panel, int num, int lngr);
int PanelItemMax(Panel panel);
int PanelItemNb(Panel panel);
char PanelItemIsSelected(Panel panel, int num);
char PanelItemEstModifie(Panel panel, int num);
void PanelItemChangeTexte(Panel panel, int num, char *texte);
void PanelItemRelit(Panel panel, int num);
int PanelItemFormat(Panel panel, int num, char *format);
int PanelItemLngr(Panel panel, int num, int lngr);
int PanelItemSelect(Panel panel, int num, char sel);
int PanelItemSouligne(Panel panel, int num, char souligne);
int PanelItemBarreDroite(Panel panel, int num, char trace);
int PanelItemBarreMilieu(Panel panel, int num, char trace);
int PanelItemBarreGauche(Panel panel, int num, char trace);
int PanelItemIScale(Panel panel, int num, int min, int total);
int PanelItemRScale(Panel panel, int num, float min, float total);
int PanelItemTextColor(Panel panel, int num, OpiumColor *coul);
int PanelItemColors(Panel panel, int num, OpiumColor *fond, int nb);
int PanelItemModif(Panel panel, int num, TypePanelFctnMod fctn, void *arg);
int PanelItemReturn(Panel panel, int num, TypePanelFctnItem fctn, void *arg);
int PanelItemExit(Panel panel, int num, TypePanelFctnItem fctn, void *arg);
void *PanelItemArg(Panel panel, int num);
int PanelItemILimits(Panel panel, int num, int min, int max);
int PanelItemRLimits(Panel panel, int num, float min, float max);
int PanelItemNoLimits(Panel panel, int num);
char PanelCode(PanelItem item, char etape, char *texte, int *col);
void OpiumPanelColorNb(Cadre cdr, Cadre fait, short *nc);
void OpiumPanelColorSet(Cadre cdr, WndFrame f);
int OpiumSizePanel(Cadre cdr, char from_wm);
void PanelItemUpdate(Panel panel, int num, int maj);
void PanelUpdate(Panel panel, int maj);
void PanelRefreshVars(Panel panel);
void PanelCursorOnVar(Panel panel, int item, int curs);
void PanelCursorMove(Panel panel, int item, int curs);
int OpiumRefreshPanel(Cadre cdr);
void PanelApply(Panel panel, int maj);
int OpiumRunPanel(Cadre cdr, WndAction *e);

#pragma mark Menus
/* .............................. menus.h ................................... */
//- #define MNU_MAXCOLS 8

typedef enum {
	MNU_CODE_RETOUR = 0,
	MNU_CODE_CONSTANTE,
	MNU_CODE_FONCTION,
	MNU_CODE_MENU,
	MNU_CODE_PANEL,
	MNU_CODE_EXEC,
	MNU_CODE_FORK,
	MNU_CODE_DISPLAY,
	MNU_CODE_COMMANDE,
	MNU_CODE_BOOLEEN,
	MNU_CODE_SEPARE,
	MNU_CODE_TOPWND,
	MNU_CODE_EXIT,
	MNU_NBCODES
} MNU_CODE;

// typedef enum {
//  MNU_COUL_HILITE = WND_GC_BASE,
// 	MNU_COUL_REVERSE,
// 	MNU_COUL_ALLUME,
// 	MNU_COUL_NB
// } MNU_COULEURS;

// #define MNU_NORMAL  WND_TEXT
// #define MNU_HILITE  WND_GC(f,MNU_COUL_HILITE)
// #define MNU_REVERSE WND_GC(f,MNU_COUL_REVERSE)
// #define MNU_ALLUME  WND_GC(f,MNU_COUL_ALLUME)
#define MNU_NORMAL  WND_GC(f,menu->normal)
#define MNU_HILITE  WND_GC(f,menu->hilite)
#define MNU_REVERSE WND_GC(f,menu->reverse)

typedef enum {
	MNU_ITEM_CACHE = -1,
	MNU_ITEM_HS = 0,
	MNU_ITEM_OK = 1
} MNU_ITEM_STATUS;

#define MNU_ITEM_HDR   0,0,0,0,0,1,-1,1
#define MNU_RETOUR     MNU_ITEM_HDR, MNU_CODE_RETOUR,    0
#define MNU_CONSTANTE  MNU_ITEM_HDR, MNU_CODE_CONSTANTE, (void *)
#define MNU_FONCTION   MNU_ITEM_HDR, MNU_CODE_FONCTION,  (void *)
#define MNU_MENU       MNU_ITEM_HDR, MNU_CODE_MENU,      (void *)
#define MNU_PANEL      MNU_ITEM_HDR, MNU_CODE_PANEL,     (void *)
#define MNU_EXEC       MNU_ITEM_HDR, MNU_CODE_EXEC,      (void *)
#define MNU_FORK       MNU_ITEM_HDR, MNU_CODE_FORK,      (void *)
#define MNU_DISPLAY    MNU_ITEM_HDR, MNU_CODE_DISPLAY,   (void *)
#define MNU_COMMANDE   MNU_ITEM_HDR, MNU_CODE_COMMANDE,  (void *)
#define MNU_BOOLEEN    MNU_ITEM_HDR, MNU_CODE_BOOLEEN,   (void *)
#define MNU_SEPARATION 0,0,0,0,0,0,-1,0, MNU_CODE_SEPARE,    0
#define MNU_TOPWND     MNU_ITEM_HDR, MNU_CODE_TOPWND,    (void *)
#define MNU_EXIT       MNU_ITEM_HDR, MNU_CODE_EXIT,      0

#define MNU_END        { 0,MNU_RETOUR }

/* ***** structures de donnees ***** */
typedef struct {
	char *texte;
	char *traduit;
	short posx,posy,lngr; /* position affichage local (en pixels)    */
	WndContextPtr gc;  /* 0 si standard                              */
	char  sel;      /* item selectionnable                           */
	char  aff;      /* numero d'affichage                            */
	char  key;      /* item selectionne par clavier                  */
	char  type;     /* nil    / constante / fonction / menu / panel  */
	void *adrs;     /* unused / valeur    /        ... adrs ...      */
} TypeMenuItem,*MenuItem;

typedef struct {
	Cadre cdr;
	MenuItem items;
	int nbitems,maxitems;
	int nbligs;      /* nombre de lignes dans une colonne     */
	int nbcols,*loc,*larg;  /* nb, emplacement, larg. des colonnes */
	int maxcols;
	int defaut;      /* item par defaut                       */
	int item_pointe; /* item pointe par la souris (WND_PRESS) */
	int curs;        /* curseur (choix par clavier)           */
	short normal,hilite,reverse;
	int taille_calculee;
} TypeMenu,*Menu;

typedef int (*TypeMenuFctn)(Menu, MenuItem );

OPIUM_VAR Menu mMenuTop,mMenuFenetres;

Menu MenuCreate(MenuItem items);
Menu MenuLocalise(MenuItem items);
void MenuSupport(Menu menu, char support);
Menu MenuBouton(char *texte, char *traduit, short posx, short posy, short lngr,
	WndContextPtr gc, char sel, char aff, char key, char type, void *adrs);
void MenuRelocalise(Menu m);
void MenuTitle(Menu m, char *texte);
Menu MenuIntitule(MenuItem items, char *texte);
Menu MenuTitled(char *texte, MenuItem items);
void MenuColumns(Menu m, int nbcols);
int MenuItemNum(Menu m, MenuItem item_cherche);
int MenuItemNb(Menu m);
int MenuLargeurPix(Menu m);
int MenuItemGetIndex(Menu m, IntAdrs num);
char *MenuItemGetText(Menu m, IntAdrs num);
void MenuItemSetText(Menu m, IntAdrs num, char *nouveau);
char *MenuItemGetLocalise(Menu m, IntAdrs num);
void MenuItemSetLocalise(Menu m, IntAdrs num, char *nouveau);
WndContextPtr MenuItemGetContext(Menu m, IntAdrs num);
void MenuItemSetContext(Menu m, IntAdrs num, WndContextPtr gc);
void MenuItemDefault(Menu m, IntAdrs num);
void MenuItemEnable(Menu m, IntAdrs num);
void MenuItemDisable(Menu m, IntAdrs num);
void MenuItemHide(Menu m, IntAdrs num);
void MenuItemSelect(Menu m, IntAdrs num, char *nouveau, char sel);
void MenuItemAllume(Menu menu, IntAdrs num, char *nouveau,
	WndColorLevel r, WndColorLevel g, WndColorLevel b);
void MenuItemEteint(Menu menu, IntAdrs num, char *nouveau);
char MenuItemArray(Menu menu, int max);
int MenuItemAdd(Menu menu, char *texte, char *traduit, short posx, short posy, short lngr,
	WndContextPtr gc, char sel, char aff, char key, char type, void *adrs);
int MenuItemReplace(Menu menu, char *texte_origine, char *texte_nouveau,
	short posx, short posy, short lngr,
	WndContextPtr gc, char sel, char key, char type, void *adrs);
int MenuItemCopy(Menu menu, MenuItem included);
void MenuItemDelete(Menu menu);
void MenuDelete(Menu m);
void OpiumMenuColorNb(Cadre cdr, Cadre fait, short *nc);
void OpiumMenuColorSet(Cadre cdr, WndFrame f);
int OpiumSizeMenu(Cadre cdr, char from_wm);
int OpiumRefreshMenu(Cadre cdr);
int OpiumRunMenu(Cadre cdr, WndAction *e, int *cdr_ouverts);
#ifdef MENU_BARRE
	int MenuBarreManage(int menu_num, unsigned long item_num); // , char *texte
	void MenuBarreCreate(Menu menu);
	void MenuBarreFenetreOuverte(Cadre cdr);
	void MenuBarreExec(void);
#endif
void MenuTopWndItem(Menu menu);
void MenuTopWndAdd(Cadre cdr);
void MenuTopWndDel(Cadre cdr);

#ifdef CODE_WARRIOR_VSN
#pragma mark Onglets
#endif
/* ............................. onglets.h .................................. */
typedef struct {
	Cadre cdr;
	char **liste;
	int numero;
	short nblig,nbcol;
	short selec;
	void (*construit)(int);
} TypeOnglet,*Onglet;

Onglet OngletDefini(char **liste, void (*construit)(int));
int OngletNb(Onglet onglet);
void OngletMaxCol(Onglet onglet, short nbcol);
void OngletNbLigs(Onglet onglet, short nblig);
int OngletRangees(Onglet onglet);
void OngletDemande(Onglet onglet, int numero);
int OngletCourant(Onglet onglet);
void OpiumOngletColorNb(Cadre cdr, Cadre fait, short *nc);
void OpiumOngletColorSet(Cadre cdr, WndFrame f);
int OpiumSizeOnglet(Cadre cdr, char from_wm);
int OpiumRefreshOnglet(Cadre cdr);
int OpiumRunOnglet(Cadre cdr, WndAction *e);
void OngletDelete(Onglet onglet);

#pragma mark Editeur
/* .............................. editeur.h ................................. */
typedef struct OpiumTableDeclaration {
	char *nom;
	char type;
	Menu menu;
	ArgDesc *restreinte;
	int *nb;
	void *var;
	struct OpiumTableDeclaration *suivante;
	char fichier[MAXFILENAME];
	ArgDesc *desc;
} TypeOpiumTableDeclaration,*OpiumTableDeclaration;

void OpiumTableCreate(OpiumTableDeclaration *liste);
OpiumTableDeclaration OpiumTableAdd(OpiumTableDeclaration *liste,
	char *nom, char type, void *var, int *nb, ArgDesc *restreinte);
OpiumTableDeclaration OpiumTablePtr(OpiumTableDeclaration liste, char *nom);
void OpiumTableFichier(OpiumTableDeclaration decl, char *fichier, ArgDesc *desc);
char OpiumTableExec(char *nom, ARG_TYPES type, void *var, int *nb, int nbrepet, ArgDesc *restreinte);
// OpiumTableDeclaration OpiumTableDeclare(char *nom, char type, void *var, int *nb, ArgDesc *restreinte);
Menu OpiumTableMenuCreate(OpiumTableDeclaration liste);


#pragma mark Planches
/* .............................. planches.h ................................ */
//typedef enum {
//	BRD_COUL_ZONE = WND_GC_BASE, // WND_GC_NOIR commun avec FIG
//	BRD_COUL_SELEC,              // WND_GC_MODS commun avec PNL
//	BRD_COUL_HILITE,             // WND_GC_HILITE commun avec PNL
//	BRD_COUL_FOND,
//	BRD_COUL_NB
//} BRD_COULEURS;

#define BRD_NORMAL WND_GC(f,WND_GC_TEXT)
//#define BRD_ZONE   WND_GC(f,BRD_COUL_ZONE)
//#define BRD_SELEC  WND_GC(f,BRD_COUL_SELEC)
//#define BRD_HILITE WND_GC(f,BRD_COUL_HILITE)
//#define BRD_FOND   WND_GC(f,BRD_COUL_FOND)

// #define BRD_SELEC  WND_GC(f,board->selec)
#define BRD_FOND   WND_GC(f,board->fond)

Cadre BoardCreate(void);
void BoardOnClic(Cadre board, int (*reponse)(Cadre, WndAction *, void *), void *arg);
Cadre BoardAdd(Cadre board, Cadre boitier, int x, int y, Cadre ref);
Cadre BoardJoin(Cadre board, Cadre boitier, int x, int y, Cadre ref);
Cadre BoardAddMenu(Cadre board, Menu menu, int x, int y, Cadre ref);
Cadre BoardAddBouton(Cadre board, char *texte, void *fctn, int x, int y, Cadre ref);
Cadre BoardAddPoussoir(Cadre board, char *texte, char *etat, int x, int y, Cadre ref);
Cadre BoardAddPanel(Cadre board, Panel panel, int x, int y, Cadre ref);
Cadre BoardAddGraph(Cadre board, Graph graph, int x, int y, Cadre ref);
Cadre BoardAddInstrum(Cadre board, Instrum instrum, int x, int y, Cadre ref);
Cadre BoardAddTerm(Cadre board, Term term, int x, int y, Cadre ref);
Cadre BoardAddInfo(Cadre board, Info info, int x, int y, Cadre ref);
Cadre BoardAddFigure(Cadre board, Figure fig, int x, int y, Cadre ref);
Cadre BoardAddOnglet(Cadre board, Onglet onglet, int y);
char BoardAreaOpen(char *titre, WND_SUPPORTS support);
void BoardAreaMargesHori(int dx_avant, int dx_apres);
void BoardAreaMargesVert(int dy_dessus, int dy_dessous);
void BoardAreaDeplaceBas(TypeFigZone *zone, int dx, int dy);
void BoardAreaCancel(Cadre board, int *x, int *y);
void BoardAreaCurrentPos(Cadre board, int *xmin, int *ymin, int *xmax, int *ymax);
void BoardAreaClose(Cadre board, TypeFigZone *zone, int *x, int *y);
void BoardAreaEnd(Cadre board, int *x, int *y);
void BoardBoutonText(Cadre board, char **liste);
void BoardReplace(Cadre board, Cadre avant, Cadre apres);
void BoardRemove(Cadre board, Cadre boitier);
void BoardDismount(Cadre board);
void BoardTrash(Cadre board);
Cadre BoardRepertoireOuvre(char *repert, char qui);
char *BoardRepertoireChoisi(char *repert, char qui);
int BoardRefreshVars(Cadre cdr);
void OpiumBoardColorNb(Cadre cdr, Cadre fait, short *nc);
void OpiumBoardColorSet(Cadre cdr, WndFrame f);
int OpiumSizeBoard(Cadre cdr, char from_wm);
int OpiumRefreshBoard(Cadre cdr);
int OpiumRunBoard(Cadre cdr, WndAction *e);
Cadre BoardComposant(Cadre board, int x, int y);
void BoardDelete(Cadre board);

#pragma mark Variables globales
/* .............................. variables ................................. */
OPIUM_VAR Cadre OpiumCdr[OPIUM_MAXCDR],OpiumCdrMinimal;
OPIUM_VAR int   OpiumNbCdr;

OPIUM_VAR char  OpiumCodeHtml;
#define OPIUM_APP_NOM 16
OPIUM_VAR char  OpiumHttpTitre[OPIUM_APP_NOM];
OPIUM_VAR int   OpiumHttpPort;
OPIUM_VAR ArgDesc *OpiumHttpDesc;
#define OPIUM_ITEM_NOM 32
OPIUM_VAR char  OpiumHttpAction[OPIUM_ITEM_NOM];
OPIUM_VAR int   OpiumHttpVars;
OPIUM_VAR char  OpiumHttpRequetteArrivee;

OPIUM_VAR TypeDico OpiumDico;
OPIUM_VAR char  OpiumDicoAllonge;
OPIUM_VAR char  OpiumJpegEspace[MAXFILENAME];
OPIUM_VAR char  OpiumDernierMessage[256];
OPIUM_VAR Cadre OpiumToRefresh[OPIUM_MAXCDR];
OPIUM_VAR int   OpiumNbRefresh;
OPIUM_VAR OpiumTableDeclaration OpiumTableChaineGlobale;
OPIUM_VAR Cadre OpiumCdrPrompt,OpiumPrioritaire,OpiumLastRefreshed;
OPIUM_VAR char  GraphUserSaisi;
OPIUM_VAR Menu GraphMdial;
OPIUM_VAR Panel OpiumPanelQuick;
OPIUM_VAR int   OpiumMargeY,OpiumMargeX;
// OPIUM_VAR int cdr->ligmin,cdr->lignb,cdr->colmin,cdr->colnb;
OPIUM_VAR int   OpiumZoom0[2],OpiumZoom1[2];
OPIUM_VAR char  OpiumZoomEnCours;
// OPIUM_VAR TypeFctn OpiumEntryPre;              /* pre-traitement entree     */
// OPIUM_VAR TypeOpiumIntFctn OpiumEntryPost;     /* post-traitement entree    */
OPIUM_VAR int (*OpiumEntryPre)(void);                 /* pre-traitement entree    */
OPIUM_VAR int (*OpiumEntryPost)(int);             /* post-traitement entree    */
OPIUM_VAR TypeFctn OpiumUserPeriodic;
OPIUM_VAR TypeFctn OpiumHtmlPageBandeau;
OPIUM_VAR char  OpiumUseMouse;
OPIUM_VAR char *OpiumNextTitle;
OPIUM_VAR WndPoint  OpiumMark[GRF_NBMARKS][9];
OPIUM_VAR WndPoint  OpiumPointPapier[5];  /* special trace sur papier (a la place de OpiumMark[GRF_POINT]) */
OPIUM_VAR WndCursor OpiumCurseurMain,OpiumCurseurViseur;
OPIUM_VAR WndColor *OpiumRouge,*OpiumOrange,*OpiumJaune,*OpiumVert;
OPIUM_VAR char      OpiumZoomAutorise;
// OPIUM_VAR WndColorLevel OpiumR[OPIUM_COLORS_AUTO],OpiumG[OPIUM_COLORS_AUTO],OpiumB[OPIUM_COLORS_AUTO];
OPIUM_VAR OpiumColor OpiumColorVal[OPIUM_COLORS_AUTO];
#define OpiumColorRGB(k) OpiumColorVal[k].r,OpiumColorVal[k].g,OpiumColorVal[k].b

OPIUM_VAR char  OpiumPanelResizable[PNL_NBTYPES];
OPIUM_VAR char *OpiumPanelReply[PNL_NBREPLIES+1]
#ifdef OPIUM_C
 = {"NoModif", "Reset", "Appliquer", "Annuler", "OK", "\0" }
#endif
;
OPIUM_VAR char *OpiumPromptNom[OPIUM_MSGNB+1]
#ifdef OPIUM_C
 = { "DECISION?", "INFORMATION..", "ATTENTION!", "ERREUR!!", "\0" }
#endif
;
OPIUM_VAR char *OpiumOui,*OpiumNon,*OpiumChoixBool,*OpiumListeVide,*OpiumListeInvl;
OPIUM_VAR char **OpiumReplyStandard,**OpiumPromptTitre;
OPIUM_VAR char OpiumPromptFmtVO;
// OPIUM_VAR char **OpiumReplyTraduit,**OpiumReplyChoix;
OPIUM_VAR OpiumColor OpiumOrangeVert[]
#ifdef OPIUM_C
 = { { GRF_RGB_ORANGE }, { GRF_RGB_GREEN } }
#endif
;
OPIUM_VAR Menu MenuEnBarre;

#define OPIUM_MAX_RESERVE 256
OPIUM_VAR char OpiumEditSurligne[OPIUM_MAX_RESERVE];
OPIUM_VAR char OpiumEditReserve[OPIUM_MAX_RESERVE];
OPIUM_VAR char OpiumGlissante;
OPIUM_VAR WndFrame OpiumEditFen;
OPIUM_VAR PanelItem OpiumEditItem;
OPIUM_VAR int  OpiumEditType,OpiumEditCar0,OpiumEditLig,OpiumEditCol;
OPIUM_VAR WndAction *OpiumEditDernierEvt;

OPIUM_VAR int OpiumEditCurs;

OPIUM_VAR FILE *OpiumTxtfile;
OPIUM_VAR Cadre OpiumPScadre;
OPIUM_VAR char OpiumPSzoome;
OPIUM_VAR char OpiumPSenCours;
OPIUM_VAR char GraphErrorText[256];
OPIUM_VAR char GraphQualite; /* 0: papier / 1: ecran */
OPIUM_VAR int GraphArrayLarg,GraphArrayHaut;
OPIUM_VAR char GraphArrayEnCours;

OPIUM_VAR int BoardZoneNb;

#ifdef CODE_WARRIOR_VSN
	#pragma mark Debug
#endif
/* ............................ opiumdebug.h ................................ */
#ifdef DEBUG_OPIUM4
	OPIUM_VAR char OpiumMseButton[WND_NBBUTTONS][12];
	OPIUM_VAR char *OpiumCdrType[OPIUM_NBTYPES+1]
	#ifdef OPIUM_C
	= { "menu", "panel", "term", "graphe", "info", "prompt", "planche", "instrum", "figure", "onglet", "\0" }
	#endif
	;
	OPIUM_VAR char OpiumNf[OPIUM_NBTYPES]
	#ifdef OPIUM_C
	 = { 0, 0, 0, 0, 2, 0, 1, 2, 1, 2 }
	#endif
	;
	#define OpiumLeLa(cdr) (((cdr->type >= 0) && (cdr->type <OPIUM_NBTYPES))? (OpiumNf[cdr->type]?((OpiumNf[cdr->type] == 2)?"l'":"la "):"le "): "la ")
	#define OpiumAuAla(cdr) (((cdr->type >= 0) && (cdr->type <OPIUM_NBTYPES))? (OpiumNf[cdr->type]?((OpiumNf[cdr->type] == 2)?"a l'":"a la "):"au "): "a la ")
	#define OpiumDuDela(cdr) (((cdr->type >= 0) && (cdr->type <OPIUM_NBTYPES))? (OpiumNf[cdr->type]?((OpiumNf[cdr->type] == 2)?"de l'":"de la "):"du "): "de la ")
	#define OPIUMCDRTYPE(type) (((type >= 0) && (type <OPIUM_NBTYPES))? OpiumCdrType[type]: "bizarrete")

	OPIUM_VAR char *OpiumMenuType[MNU_NBCODES+1]
	#ifdef OPIUM_C
	= {
		"retour", "constante", "fonction", "menu", "panel",
		"exec", "fork", "display",
		"commande", "booleen", "separation", "exit", "\0"
	}
	#endif
	;
	OPIUM_VAR char *OpiumPanelType[PNL_NBTYPES+1]
	#ifdef OPIUM_C
	= {
		"remarque", "separation", "password", "fichier", "texte",
		"long", "int", "short", "char", "octet", "lettre",
		"lhex", "hex", "shex", "byte", "bool",
		"float", "double", "adu",
		"liste", "listL", "listD", "listS", "listB", "key", "keyL", "keyS", "keyB",
		"user", "bouton",
		"\0"
	}
	#endif
	;

	typedef enum {
		OPIUM_DEBUG_WND = 0,
		OPIUM_DEBUG_OPIUM,
		OPIUM_DEBUG_MENU,
		OPIUM_DEBUG_PANEL,
		OPIUM_DEBUG_TERM,
		OPIUM_DEBUG_GRAPH,
		OPIUM_DEBUG_INFO,
		OPIUM_DEBUG_PROMPT,
		OPIUM_DEBUG_BOARD,
		OPIUM_DEBUG_INSTRUM,
		OPIUM_DEBUG_FIGURE,
		OPIUM_DEBUG_ONGLET,
		OPIUM_DEBUG_NBOPTS
	} OPIUM_DEBUG_OPTS;

	OPIUM_VAR int OpiumDebugLevel[OPIUM_DEBUG_NBOPTS];

	#define DEBUG_WND(level)     (OpiumDebugLevel[OPIUM_DEBUG_WND]     >= level)
	#define DEBUG_OPIUM(level)   (OpiumDebugLevel[OPIUM_DEBUG_OPIUM]   >= level)
	#define DEBUG_MENU(level)    (OpiumDebugLevel[OPIUM_DEBUG_MENU]    >= level)
	#define DEBUG_PANEL(level)   (OpiumDebugLevel[OPIUM_DEBUG_PANEL]   >= level)
	#define DEBUG_TERM(level)    (OpiumDebugLevel[OPIUM_DEBUG_TERM]    >= level)
	#define DEBUG_GRAPH(level)   (OpiumDebugLevel[OPIUM_DEBUG_GRAPH]   >= level)
	#define DEBUG_INFO(level)    (OpiumDebugLevel[OPIUM_DEBUG_INFO]    >= level)
	#define DEBUG_PROMPT(level)  (OpiumDebugLevel[OPIUM_DEBUG_PROMPT]  >= level)
	#define DEBUG_BOARD(level)   (OpiumDebugLevel[OPIUM_DEBUG_BOARD]   >= level)
	#define DEBUG_INSTRUM(level) (OpiumDebugLevel[OPIUM_DEBUG_INSTRUM] >= level)
	#define DEBUG_FIGURE(level)  (OpiumDebugLevel[OPIUM_DEBUG_FIGURE]  >= level)
	#define DEBUG_ONGLET(level)  (OpiumDebugLevel[OPIUM_DEBUG_ONGLET]  >= level)

	OPIUM_VAR char *OpiumDebugNom[OPIUM_DEBUG_NBOPTS+1]
	#ifdef OPIUM_C
	= {
		"Window", "Opium", "Menus", "Panels", "Terminals", "Graphiques",
		"Infos", "Prompt", "Planches", "Instrument", "Figures", "Onglets", "\0"
	}
	#endif
	;

#else /* DEBUG_OPIUM4 */
	#define OPIUM_DEBUG_WND      0
	#define OPIUM_DEBUG_OPIUM    0
	#define OPIUM_DEBUG_MENU     0
	#define OPIUM_DEBUG_PANEL    0
	#define OPIUM_DEBUG_TERM     0
	#define OPIUM_DEBUG_GRAPH    0
	#define OPIUM_DEBUG_INFO     0
	#define OPIUM_DEBUG_PROMPT   0
	#define OPIUM_DEBUG_BOARD    0
	#define OPIUM_DEBUG_INSTRUM  0
	#define OPIUM_DEBUG_FIGURE   0
	#define OPIUM_DEBUG_ONGLET   0
	#define DEBUG_WND(level)     0
	#define DEBUG_OPIUM(level)   0
	#define DEBUG_MENU(level)    0
	#define DEBUG_PANEL(level)   0
	#define DEBUG_TERM(level)    0
	#define DEBUG_GRAPH(level)   0
	#define DEBUG_INFO(level)    0
	#define DEBUG_PROMPT(level)  0
	#define DEBUG_BOARD(level)   0
	#define DEBUG_INSTRUM(level) 0
	#define DEBUG_FIGURE(level)  0
	#define DEBUG_ONGLET(level)  0
#endif /* DEBUG_OPIUM4 */

void OpiumDumpConnexions(const char *fctn, Cadre contenant);

#endif

