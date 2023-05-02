#ifndef WND_H
#define WND_H

#include <stdio.h>

#include "environnement.h"
#ifdef macintosh
	#include "EnvironnementApple.h"
#endif /* macintosh */
#include <utils/defineVAR.h>

#ifdef WND_C
	#define WND_VAR
#else
	#define WND_VAR extern
#endif

#ifdef TIMER_NIVEAU_WND
	#include "timeruser.h"
#endif /* TIMER_NIVEAU_WND */
// #define TIMER_PAR_EVT
#ifdef TIMER_PAR_EVT
	#define TIMER_UNIQUE
#endif /* TIMER_PAR_EVT */

#define WND_RELIEF
#define WND_HTML  (char *)(-3)
#define WND_VT100 (char *)(-2)
#define WND_NONE  (char *)(-1)
#define WND_REFRESH 0x7F

#define WND_A4 (WndServer *)-1
#define WND_PIXEL_CM (72.0 / 2.54)
/* 21 cm fois 72 dpi */
#define WND_A4_LARG 595
/* 29.7 cm fois 72 dpi */
#define WND_A4_HAUT 841

typedef unsigned short WndColorLevel;
#define WND_COLORMAX ((1 << (sizeof(WndColorLevel) * 8)) - 1)
#define WND_COLOR100 ((float)WND_COLORMAX)
#define WND_COLORFLOAT (100.0 / WND_COLOR100)
#define WND_COLOR8(val) (unsigned char)((val >> 8) & 0xFF)
#define WND_MAXCOULEUR 15

#define WND_COLOR_MIN     0x0000
#define WND_COLOR_MAX     0xFFFF
#define WND_COLOR_DEMI    0x7FFF
#define WND_COLOR_QUART   0x3FFF
#define WND_COLOR_3QUARTS 0xBFFF

typedef void (*TypeWndTimeSlice)();

typedef enum {
	WND_PS_DERNIERE = 0,
	WND_PS_NOUVELLE,
	WND_PS_PREMIERE
} WND_PS_SAUTPAGE;

// #define WND_GC(f,index) f->gc[f->qualite].coul[index]

#define PRINT_GC(gc) \
	printf("(%s:%d) GC %s @%08llX ->fgnd @%08llX, ->bgnd @%08llX\n",__func__,__LINE__,\
		chaine_preprocesseur(gc),(IntAdrs)gc,\
		gc? (IntAdrs)(gc->foreground): 0xfeedc0de,gc? (IntAdrs)(gc->background): 0xfeedc0de);
#define PRINT_FGC(f,index) \
	printf("(%s:%d) %08llX.gc[%d] @%08llX ->fgnd @%08llX, ->bgnd @%08llX\n",__func__,__LINE__,\
		(IntAdrs)f,index,(IntAdrs)f->gc[WND_Q_ECRAN].coul[index],\
		f->gc[WND_Q_ECRAN].coul[index]? (IntAdrs)(f->gc[WND_Q_ECRAN].coul[index]->foreground): 0xfeedc0de,\
		f->gc[WND_Q_ECRAN].coul[index]? (IntAdrs)(f->gc[WND_Q_ECRAN].coul[index]->background): 0xfeedc0de);
#define PRINT_COLOR(couleur) \
	printf("(%s:%d) COUL %s @%08llX -> { %04X %04X %04X }\n",__func__,__LINE__,\
		chaine_preprocesseur(couleur),(IntAdrs)couleur,\
		couleur? couleur->red: 0xcaca,couleur? couleur->green: 0xcaca,couleur? couleur->blue: 0xcaca)

typedef enum {
	WND_KBD_US  = 0,
	WND_KBD_FR_MAC,
	WND_KBD_FR_DELL,
	WND_KBD_NB
} WND_KBD;

typedef enum {
	WND_CHECK = 0,
	WND_WAIT = 1
} WND_EVENT_MODE;

typedef enum {
	WND_FEN_STD = 0,
	WND_FEN_SUB,
	WND_FEN_BRD
} WND_TYPE;

typedef enum {
	WND_GC_STD = 0,
	WND_GC_CURSOR,
	WND_GC_ASCR,
	WND_GC_TEXT,
	WND_GC_CLEAR,
	WND_GC_LITE,
	WND_GC_DARK,
	WND_GC_SELECTED,

	WND_GC_HILITE,
	WND_GC_MODS,
	WND_GC_REVERSE,
	WND_GC_LUMIN,
	WND_GC_BRDNOIR,
	WND_GC_BRDBLANC,
	WND_GC_GREY,
	WND_GC_BUTN,
	WND_GC_SDRK,
	WND_GC_NOIR,
	WND_GC_VERT,
	WND_GC_ROUGE,
	WND_GC_ORANGE,
	WND_GC_AREANAME,
	WND_GC_NB
} WND_GC_COUL;

#define WND_GC_BASE WND_GC_SELECTED+1

typedef enum {
	WND_Q_PAPIER = 0,
	WND_Q_ECRAN,
	WND_NBQUAL
} WND_QUALITE;
WND_VAR char *WndSupportNom[WND_NBQUAL+1]
#ifdef WND_C
 = { "papier", "ecran", "\0" }
#endif
;
WND_VAR char WndSupportCles[16];

#define WND_GC(f,index) f->gc[f->qualite].coul[index]
// #define WndContext WND_GC

#define WND_STD      WND_GC(f,WND_GC_STD)
#define WND_CURSOR   WND_GC(f,WND_GC_CURSOR)
#define WND_ASCR     WND_GC(f,WND_GC_ASCR)
#define WND_TEXT     WND_GC(f,WND_GC_TEXT)
#define WND_CLEAR    WND_GC(f,WND_GC_CLEAR)
#define WND_LITE     WND_GC(f,WND_GC_LITE)
#define WND_DARK     WND_GC(f,WND_GC_DARK)
#define WND_SELECTED WND_GC(f,WND_GC_SELECTED)
/*
#define WND_HILITE   WND_GC(f,WND_GC_HILITE)
#define WND_MODS     WND_GC(f,WND_GC_MODS)
#define WND_REVERSE  WND_GC(f,WND_GC_REVERSE)
#define WND_LUMIN    WND_GC(f,WND_GC_LUMIN)
#define WND_GREY     WND_GC(f,WND_GC_GREY)
#define WND_BUTN     WND_GC(f,WND_GC_BUTN)
#define WND_SDRK     WND_GC(f,WND_GC_SDRK)
#define WND_VERT     WND_GC(f,WND_GC_VERT)
#define WND_ROUGE    WND_GC(f,WND_GC_ROUGE)
#define WND_ORANGE   WND_GC(f,WND_GC_ORANGE)
 */

#ifdef INITIALEMENT
	#define WND_XY 0x01
	#define WND_E  0x02
	#define WND_T  0x0C
	#define WND_HORIZONTAL 0x00
	#define WND_VERTICAL   0x01
	#define WND_INTERIEUR  0x00
	#define WND_EXTERIEUR  0x02
	#define WND_RAINURE    0x00
	#define WND_REGLETTE   0x04
	#define WND_ECLAIRE    0x08
	#define WND_OMBRE      0x0C
#else
	#define WND_DEF 0x00
	#define WND_T   0x03
	#define WND_XY  0x04
	#define WND_E   0x08
	#define WND_TOT 0x10

	#define WND_RAINURE    0x00
	#define WND_REGLETTE   0x01
	#define WND_ECLAIRE    0x02
	#define WND_OMBRE      0x03
	#define WND_HORIZONTAL WND_DEF
	#define WND_VERTICAL   WND_XY
	#define WND_INTERIEUR  WND_DEF
	#define WND_EXTERIEUR  WND_E
#endif

#define WND_BORD_HAUT  (WND_HORIZONTAL | WND_ECLAIRE | WND_EXTERIEUR)
#define WND_BORD_DRTE  (WND_VERTICAL   | WND_OMBRE   | WND_EXTERIEUR)
#define WND_BORD_BAS   (WND_HORIZONTAL | WND_OMBRE   | WND_EXTERIEUR)
#define WND_BORD_GCHE  (WND_VERTICAL   | WND_ECLAIRE | WND_EXTERIEUR)
#define WND_FOND_HAUT  (WND_HORIZONTAL | WND_OMBRE   | WND_INTERIEUR)
#define WND_FOND_DRTE  (WND_VERTICAL   | WND_ECLAIRE | WND_INTERIEUR)
#define WND_FOND_GCHE  (WND_VERTICAL   | WND_OMBRE   | WND_INTERIEUR)
#define WND_FOND_BAS   (WND_HORIZONTAL | WND_ECLAIRE | WND_INTERIEUR)

typedef enum {
	WND_RIEN = 0,
	WND_LIGNES,
	WND_PLAQUETTE,
	WND_CREUX,
	WND_RAINURES,
	WND_REGLETTES,
	WND_NBSUPPORTS
} WND_SUPPORTS;

#ifdef OS9
	typedef enum { false = 0, true = 1 } BOOLEAN;
#endif /* OS9 */
#ifdef UNIX
	#ifndef cplusplus
		typedef enum { false = 0, true = 1 } BOOLEAN;
	#endif /* cplusplus */
#endif /* UNIX */
typedef enum { ERREUR = -1, NUL = 0, BON = 1 } RETURN_CODE;
typedef char Bool;

#include "interfaces.h"
#include "implement.h"
#include "actioncodes.h"

typedef struct {
	WndFontId id;
	short width,ascent,descent,leading;
} WndFont;

typedef struct {
	WndScreen d;
	int larg,haut;
	WndFont fonte;
	struct { WndContextVal coul[WND_GC_NB]; } gc[WND_NBQUAL];
	int lig,col,decal;
} WndServer;

typedef struct {
	struct {
		unsigned short ident;
		unsigned int taille;
		unsigned int reserve;
		unsigned int offset;
	} fichier;
	struct {
		unsigned int taille;
		unsigned int larg,haut;
		unsigned short plans;
		unsigned short bits_par_couleur; // 1 (2 coul), 4 (16 coul), 8 (256 coul), 16, 32
		unsigned int codage;             // 1=8bits/pixel, 2=4bits/pixel, 3=reference de palette
		unsigned int total;
		unsigned int h_resol,v_resol;
		unsigned int dim_palette;
		unsigned int nb_ppales;
	} image;
} TypeWndBmpHeader;
typedef struct {
	unsigned char b,g,r,x;
} TypeWndBmpPalette,*WndBmpPalette;
typedef struct {
	unsigned char b,g,r;
} TypeWndBmpPixel24,*WndBmpPixel24;
//typedef struct {
//	unsigned short b,g,r;
//} TypeWndBmpPixel16,*WndBmpPixel16;
typedef struct {
	unsigned int x,b,g,r;
} TypeWndBmpPixel32,*WndBmpPixel32;

typedef enum { ICONE_H = 0, ICONE_FIC, ICONE_BMP } ICONE_TYPE;
typedef struct {
	char m;
	unsigned char r,g,b;
} TypeWndIcnPxl,*WndIcnPxl;
typedef struct {
	short larg,haut;
	WndIcnPxl pixel;
} TypeWndIcone,*WndIcone;

typedef struct {
	unsigned char  r;
	unsigned char  g;
	unsigned char  b;
} WndCol8bits;

typedef struct {
	int x0,y0,dx,dy;
	WndColor *lut; int nbcolors;
	WndColorLevel *R,*G,*B,*A;
	WndSurface surf;
} WndImage;

typedef enum {
	WND_ACTN_PAINT = 0,
	WND_ACTN_WRITE,
	WND_ACTN_STRING,
	WND_ACTN_LINE,
	WND_ACTN_NB
} WND_ACTION;
typedef struct {
	WndContextPtr gc;
	int i,j,k,l;
	void *info;
	char action;
} WndTypeExtraActn;

typedef struct {
	WndContextPtr gc;
	int x0,y0;
	char en_cours,mode;
	WndLineParm parm;
} TypeWndLineInfo,*WndLineInfo;
#define WndAddLine(f,x,y,l,h) WndSetLine(f,x,y,(x)+(l),(y)+(h))

typedef struct WndFrameStruct {
	WndServer *s;
	WndIdent w;
	void *cdr;
	short nb_gc;
	struct { WndContextPtr *coul; } gc[WND_NBQUAL];
	int x,y,h,v;               /* taille                                             */
	int x0,y0,xm,ym;           /* limites temporaires (sous-fenetre)                 */
	short lig0,col0,ligm,colm; /* idem, en termes de lignes/colonnes                 */
	short lig,col;             /* position du curseur (-1 si pas de curseur affiche) */
	TypeWndLineInfo info;      /* infos de tracage de ligne                          */
	int  derniere_actn;
	char en_cours;
	char passive;
	char qualite;
	char double_buffer;
	WndImage image;
	WndTypeExtraActn *extras;
	short extra_nb,extra_max;
#ifdef TIMER_NIVEAU_WND
	#ifdef TIMER_PAR_EVT
		#ifdef TIMER_UNIQUE
			unsigned char delai,restant;
		#else
			Timer timer;
		#endif
	#endif
#endif
	struct WndFrameStruct *dessous;
} *WndFrame;
#define WND_NOT_IN_STACK (WndFrame)-1
#define WND_AT_END (WndFrame)0

#define WND_ACTION_LNGR 80
typedef struct WndActionStruct {
	short  type;
	WndIdent w;
	int   x,y,h,v;
	unsigned long code;
	char  texte[WND_ACTION_LNGR];
	struct WndActionStruct *suivante;
} WndAction;
#define WND_INACTION (WndAction *)0

WND_VAR WndAction WndEventPrecedent;

#define WND_MAXUSERKEYS 32
typedef struct {
	int   code;
	char  texte[WND_ACTION_LNGR];
	int (*fctn)(void); /* un retour int peut etre demande par un menu opium */
} WndUserKeyType;

WND_VAR int WndLogX
#ifdef WND_C
 = 500
#endif
;
WND_VAR int WndLogY
#ifdef WND_C
 = 45
#endif
;
WND_VAR int WndLogLines
#ifdef WND_C
 = 50
#endif
;
WND_VAR int WndLogCols
#ifdef WND_C
 = 80
#endif
;
WND_VAR char WndLogSave
#ifdef WND_C
 = 1
#endif
;
#define WND_LOGLNGR 80
WND_VAR char WndLogName[WND_LOGLNGR]
#ifdef WND_C
 = "\07Journal"
#endif
;
WND_VAR char WndCodeHtml;
WND_VAR char WndQual;
WND_VAR char WndModeNone;
WND_VAR char WndModeVt100;
WND_VAR char WndInitVt100
#ifdef WND_C
 = 1
#endif
;

#ifdef EVTS_PILE_MBOX
	WND_VAR Wmbox WndMailBox
#else
	WND_VAR void *WndMailBox
#endif
#ifdef WND_C
	= 0
#endif
;

WND_VAR WndContextVal WndPeintureGrise;
WND_VAR char WndQueryExit;
WND_VAR WndIdent  WndRefreshed;
#ifdef TIMER_PAR_EVT
	#ifndef TIMER_UNIQUE
		WND_VAR Timer WndTimerUsed;
	#endif
#endif
WND_VAR WndServer WndDefSvr;
WND_VAR WndServer *WndCurSvr;
WND_VAR char WndDisplayInUse[MAXFILENAME];
WND_VAR WndFrame WndTopFrame,WndLastWriteOnly;
WND_VAR WndUserKeyType WndUserKey[WND_MAXUSERKEYS];

WND_VAR WndColorLevel WndLevelBgnd[WND_NBQUAL],WndLevelText[WND_NBQUAL];

WND_VAR WndColor *WndColorBlack,*WndColorWhite;
WND_VAR WndColor *WndColorBgnd[WND_NBQUAL],*WndColorText[WND_NBQUAL];
WND_VAR WndColor *WndColorMods[WND_NBQUAL],*WndColorHilite[WND_NBQUAL];
WND_VAR WndColor *WndColorAscr[WND_NBQUAL],*WndColorActif[WND_NBQUAL],*WndColorSub[WND_NBQUAL],*WndColorBoard[WND_NBQUAL];
WND_VAR WndColor *WndColorButn[WND_NBQUAL],*WndColorLumin[WND_NBQUAL],*WndColorPale[WND_NBQUAL];
WND_VAR WndColor *WndColorLite[WND_NBQUAL],*WndColorDark[WND_NBQUAL],*WndColorSuperdark[WND_NBQUAL],*WndColorGrey[WND_NBQUAL];
WND_VAR WndColor *WndColorNoir[WND_NBQUAL],*WndColorRouge[WND_NBQUAL],*WndColorOrange[WND_NBQUAL],*WndColorBleu[WND_NBQUAL];
WND_VAR WndColor *WndColorNote[WND_NBQUAL],*WndColorWarn[WND_NBQUAL],*WndColorErrr[WND_NBQUAL];
WND_VAR int WndColorNb;

WND_VAR int WndUserKeyNb;
WND_VAR char WndMenuAffiche;

WND_VAR WndPoint *WndLigneVal;
WND_VAR int WndLigneDim;

void WndStep(char *fmt, ...);
void WndLog(int x, int y, int ligs, int cols, char *nom);
void WndSetFontName(char *chaine);
void WndSetFontSize(int taille);
void WndSetKeyBoard(char type);
void WndSetVideo(int num);
void WndSetBgndBlack(char noir);
char WndSetQualite(char qual);
void WndMbxCreate(char *appli, int id, char *dir);
Bool WndInit(char *display);
Bool WndOpen(WndServer *s, char *display);
void WndFenColorNb(WndFrame f, short nb);
void WndFenColorSet(WndFrame f, short num, short couleur);
void WndFenColorDump(WndFrame f, int qual);
void WndExit(void);
void WndJournalTitle(char *texte);
int WndTotalWidth(WndServer *svr);
int WndTotalHeigth(WndServer *svr);
void WndAPropos(char *appli, void (*fctn)());
void WndContextSetColors(WndFrame f, WndContextPtr gc, WndColor *fond, WndColor *text);
#ifdef MENU_BARRE
	void WndMenuDebug(char dbg);
	int  WndMenuCreate(char *texte);
	void WndMenuItem(char *texte, char suite, char sel);
	void WndMenuSepare(void);
	void WndMenuAnnonce(char *texte);
	void WndMenuInsert(void);
	void WndMenuClear(char *titre);
	void WndMenuDisplay(void);
#endif
int WndLigToPix(int lig);
int WndColToPix(int col);
int WndPixToLig(int pix);
int WndPixToCol(int pix);
char WndAppliTime(TypeWndTimeSlice fctn);
void WndNewRoot(char *title, int sizx, int sizy);
WndFrame WndDisplay(char qual, int posx, int posy, int sizx, int sizy);
WndFrame WndAttach(char qual, int posx, int posy, int sizx, int sizy);
WndFrame WndCreate(int type, char qual, int posx, int posy, int sizx, int sizy);
void WndAttachTo(WndFrame f, void *cdr);
void WndMinimale(WndFrame f);
void WndAffiche(WndFrame f, int x, int y);
void WndRepere(WndFrame f, int x0, int y0, int xm, int ym);
void WndMask(WndFrame f, int x0, int y0, int xm, int ym);
void WndTitle(WndFrame f, char *titre);
void WndStackPrePrint(char *routine, WndFrame f);
char WndBasicRGB(WndFrame f, WndColorLevel fr, WndColorLevel fg, WndColorLevel fb, WndColorLevel tr, WndColorLevel tg, WndColorLevel tb);
char WndRefreshBegin(WndFrame f);
void WndRefreshEnd(WndFrame f);
void WndControls(WndFrame f);
void WndBorders(WndFrame f);
WndCursor WndCreateStdCursor(int num);
WndCursor WndCreateUserCursor(int larg, int haut, unsigned char *map);
void WndAssignCursor(WndFrame f, WndCursor curseur);
void WndExtraInit(WndFrame f, short max);
void WndExtraAdd(WndFrame f, char action, WndContextPtr gc, int i, int j, int k, int l, void *info);
short WndExtraNb(WndFrame f);
void WndExtraDisplay(WndFrame f);
void WndStackPrePrint(char *routine, WndFrame f);
void WndResize(WndFrame f);
void WndModale(WndFrame f);
void WndRaise(WndFrame f);
void WndRefreshRect(WndFrame f, int x, int y, int l, int h);
void WndStackPrint(const char *routine, WndFrame f);
void WndPutAtTop(WndFrame f);
void WndShowTheTop(void);
void WndSendToTop(WndFrame f);
#ifdef OPENGL_OR_WXWIDGETS
	void WndFocusTo(WndIdent w);
#endif
#ifdef X11
	#define WndUpdateBegin(w)
	#define WndUpdateEnd(w)
#else
	void WndUpdateBegin(WndIdent w);
	void WndUpdateEnd(WndIdent w);
	char WndRefreshActif(WndIdent w);
	void WndRefreshAll(void);
#endif
char WndEnPeinture(WndIdent w);
char WndRefreshActif(WndIdent w);
void WndWriteOnlySet(WndFrame f);
#ifdef TIMER_NIVEAU_WND
	#ifdef TIMER_PAR_EVT
		#ifdef TIMER_UNIQUE
			void WndWriteOnlyDump(void);
			void WndWriteOnlyRefresh(WndFrame f, unsigned char delai) ;
		#else
			void WndRefreshAuto(WndFrame f, unsigned char delai);
			void WndRefreshStop(WndFrame f);
		#endif
	#endif
#endif
void WndMove(WndFrame f, int x, int y);
void WndAlign(WndFrame f, WndFrame to, int x, int y);
void WndShow(WndFrame f);
void WndBlank(WndFrame f);
WndFrame WndSupprime(WndFrame f);
#define WndClear(f) f = WndSupprime(f)
int WndUserKeyAdd(int code, char *texte, int (*fctn)());
int WndEventLoopStarted(WndIdent w_initiale, int *code_a_rendre);
void WndEventLoopStop(WndIdent w);
void WndEventLoopKill(void);
#ifdef WXWIDGETS
	int WndEventFromWx(WxiRef wx, long stamp, unsigned short type, unsigned long info, int x, int y, unsigned short mods);
#endif
void WndEventQueue(char type, WndIdent w);
int WndEventNew(WndAction *u, WND_EVENT_MODE mode);
#ifdef QUICKDRAW
	char WndUpdateFlushed(WndAction *u);
#else
	#define WndUpdateFlushed(u) 1
#endif
int WndEventReady(WndAction *u);
//int WndReady(int evt, WndAction *u);
int WndEventWait(int evt, WndAction *u);
void WndEventSend(WndFrame f, int type, int x, int y, int h, int v);
WndColor *WndColorGetFromName(char *nom);
WndColor *WndColorGetFromRGB(WndColorLevel r, WndColorLevel g, WndColorLevel b);
char WndColorSetByName(WndColor *c, char *nom);
char WndColorSetByRGB(WndColor *c, int r, int g, int b);
void WndColorFree(WndColor *c);
WndContextPtr WndContextCreate(WndFrame f);
WndContextPtr WndContextSupportCreate(WndFrame f, int qual);
WndContextPtr WndContextCreateFromVal(WndFrame f, WndContextVal *gcval);
void WndContextCopy(WndFrame f, WndContextPtr gc_src, WndContextPtr gc_dest);
void WndContextFree(WndFrame f, WndContextPtr gc);
char WndContextFgndName(WndFrame f, WndContextPtr gc, char *nom);
char WndContextFgndRGB(WndFrame f, WndContextPtr gc, WndColorLevel r, WndColorLevel g, WndColorLevel b);
char WndContextBgndRGB(WndFrame f, WndContextPtr gc, WndColorLevel r, WndColorLevel g, WndColorLevel b);
char WndContextFgndColor(WndFrame f, WndContextPtr gc, WndColor *c);
char WndContextBgndColor(WndFrame f, WndContextPtr gc, WndColor *c);
void WndContextLine(WndFrame f, WndContextPtr gc, unsigned int type, unsigned int width);
void WndImageInit(WndFrame f, int larg, int haut, WndColor *lut, int dim);
void WndImageOffset(WndFrame f, int dx, int dy);
void WndImagePixel(WndFrame f, int x, int y, int val);
void WndImageShow(WndFrame f);
WndIcone WndIconeFromBmpFile(char *nom_complet, char *explics);
WndIcone WndIconeRead(char *nom_complet, char *explics);
char WndIconeSave(WndIcone icone, char type, char *nom, char *chemin, char *explics);
WndIcone WndIconeCompactee(WndIcone srce, int facteur);
void WndIconeDelete(WndIcone icone);
void WndIconeInit(WndFrame f, WndIcone icone);
void WndIconePixel(WndFrame f, int x, int y, WndIcnPxl pixel);

char WndBeginLine(WndFrame f, WndContextPtr gc, unsigned int mode);
void WndGCLine(WndFrame f, WndContextPtr gc);
void WndAbsLine(WndFrame f);
void WndRelLine(WndFrame f);
void WndSetLine(WndFrame f, int x0, int y0, int x1, int y1);
void WndEndLine(WndFrame f);
// void WndAddRelief(WndLineInfo info, int epaisseur, int type, int x, int y, int l);

WndPoint *WndLigneAlloc(int dim);
void WndLine(WndFrame f, WndContextPtr gc, int x, int y, int l, int h);
void WndPoly(WndFrame f, WndContextPtr gc, WndPoint *p, int nb, int type);
void WndRectangle(WndFrame f, WndContextPtr c, int x, int y, int l, int h);
void WndRelief(WndFrame f, int epaisseur, int type, int x, int y, int l);
void WndPaint(WndFrame f, int x, int y, int l, int h, WndColor *c);
void WndFillFgnd(WndFrame f, WndContextPtr gc, int x, int y, int l, int h);
void WndFillBgnd(WndFrame f, WndContextPtr gc, int x, int y, int l, int h);
void WndOvale(WndFrame f, WndContextPtr c, int x, int y, int l, int h);
void WndSecteur(WndFrame f, WndContextPtr gc, int x, int y, int l, int h, int a0, int da);
void WndArc(WndFrame f, WndContextPtr gc, int x, int y, int l, int h, int a0, int da);
void WndEntoure(WndFrame f, char support, int x, int y, int h, int v);
void WndErase(WndFrame f, int x, int y, int l, int h);
void WndCursorSet(WndFrame f, short lig, short col);
void WndHBar(WndFrame f, WndContextPtr gc, int lig, int col, int lngr);
void WndVBar(WndFrame f, WndContextPtr gc, int lig, int col, int haut);
void WndSouligne(WndFrame f, WndContextPtr gc, int lig, int col, int lngr);
void WndBarreDroite(WndFrame f, WndContextPtr gc, int lig, int col, int lngr);
void WndBarreGauche(WndFrame f, WndContextPtr gc, int lig, int col, int lngr);
void WndButton(WndFrame f, WndContextPtr gc, int lig, int col, int dim, char *texte, char dans_planche);

void WndDrawString(WndFrame f, WndContextPtr gc, int x, int y, char *texte, size_t l);
void WndString(WndFrame f, WndContextPtr gc, int x, int y, char *texte);
void WndTextToPix(WndFrame f, int lig, int col, int *x, int *y);
void WndPixToText(WndFrame f, int x, int y, int *lig, int *col);
void WndWrite(WndFrame f, WndContextPtr gc, int lig, int col, char *texte);
void WndClearText(WndFrame f, WndContextPtr gc, int lig, int col, int lngr);
void WndClearString(WndFrame f, WndContextPtr gc, int x, int y, int l);

void WndBell(WndFrame f, int force);
int WndLaunch(char *commande);
FILE *WndPSopen(char *nom, char *suite);
char WndPSopened(void);
void WndPSstring(int x, int y, char *texte);
void WndPSposition(int posx, int posy);
void WndPSfintrace(void);
void WndPSpage(WND_PS_SAUTPAGE nouvelle);
int WndPSpageNb(void);
void WndPSclose(void);

#endif
