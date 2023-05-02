#ifndef WMBOX_H
#define WMBOX_H

#ifdef WMBOX_C
	#define WMBOX_VAR
#else
	#define WMBOX_VAR extern
#endif

#ifdef __cplusplus
	#define EXTERNC extern "C"
#else
	#define EXTERNC
#endif

#include <semaphore.h>

#define WMB_MEM_CLE 2022

/*
LIBRE 
-> Appli: demande de pointeur -> RESERVEE
-> Appli: remplissage -> REDIGEE
-> Server: cycle d'examen, reponse -> REPONDUE
-> Appli: extraction valeurs -> LIBRE
*/

typedef enum {
	WMB_STS_LIBRE = 0,
	WMB_STS_RESERVEE,
	WMB_STS_REDIGEE,
	WMB_STS_REPONDUE,
	WXU_STS_NB
} WXI_STATUS;
WMBOX_VAR const char *WmboxStsListe[]
#ifdef WMBOX_C
 = { "libre", "reservee", "redigee", "repondue", "\0" }
#endif
;

#define WMB_STS_RESERVE WMB_STS_RESERVEE
#define WMB_STS_COMPLET WMB_STS_REDIGEE
#define WMB_STS_UTILISE WMB_STS_REPONDUE

typedef enum {
	WMB_CDE_DISPLAY_START = 1,
	WMB_CDE_DISPLAY_SIZE,
	WMB_CDE_DISPLAY_EXIT,
	WMB_CDE_ACCEPT_EVENTS,
	WMB_CDE_CHECK_EVENTS,
	WMB_CDE_STOP_EVENTS,
	WMB_CDE_KILL_EVENTS,
	WMB_CDE_WINDOW_CREATE,
	WMB_CDE_WINDOW_TITLE,
	WMB_CDE_WINDOW_TIMER,
	WMB_CDE_WINDOW_GETSIZE,
	WMB_CDE_WINDOW_RESIZE,
	WMB_CDE_WINDOW_MOVE,
	WMB_CDE_WINDOW_RAISE,
	WMB_CDE_WINDOW_RECT,
	WMB_CDE_WINDOW_SHOW,
	WMB_CDE_WINDOW_REFRESH,
	WMB_CDE_WINDOW_DESTROY,
	WMB_CDE_FONT_SIZE,
	WMB_CDE_FONT_INFO,
	WMB_CDE_CURSOR_STD,
	WMB_CDE_RGB_FGND,
	WMB_CDE_RGB_BGND,
	WMB_CDE_RGB_TEXT,
	WMB_CDE_PAINT_BEGIN,
	WMB_CDE_PAINT_END,
	WMB_CDE_RECT_ERAZ,
	WMB_CDE_RECT_FILL,
	WMB_CDE_RECT_OVAL,
	WMB_CDE_RECT_DRAW,
	WMB_CDE_TEXT_DRAW,
	WMB_CDE_LINE_DRAW,
	WMB_CDE_ARC_DRAW,
	WMB_CDE_POLY_DRAW,
	WMB_CDE_POLY_SIZE,
	WMB_CDE_POLY_FILL,
	WMB_CDE_NB
} WMB_CDES;

WMBOX_VAR const char *WmboxCmdeListe[]
#ifdef WMBOX_C
= {	"None", "DisplayStart", "DisplaySize", "DisplayExit",
	"AcceptEvents", "CheckEvents", "StopEvents", "KillEvents",
	"WindowCreate", "WindowTitle", "WindowTimer", "WindowGetSize", "WindowResize",
	"WindowMove", "WindowRaise", "WindowRect", "WindowShow", "WindowRefresh", "WindowDestroy",
	"FontSize", "FontInfo", "CursorStd",
	"RgbFgnd", "RgbBgnd", "RgbText", "PaintBegin", "PaintEnd",
	"RectEraz", "RectFill", "OvalDraw", "RectDraw", "TextDraw", "LineDraw",
	"ArcDraw", "PolyDraw", "PolySize", "PolyFill",
	"\0"
}
#endif
;
#define WMB_CDE_NAME(cmde) ((cmde >= 0) && (cmde < WMB_CDE_NB))? WmboxCmdeListe[cmde]: "inconnue"

#define WMB_CDE_MAX 8192
#define WMB_EVT_MAX 8192
#define WMB_PTS_MAX 8192

#define WmboxEventCode unsigned short
#ifdef WXI_INTERFACE_H
	typedef WxiPoint WmboxPoint;
#else
	typedef struct { int x,y; } WmboxPoint;
#endif

#define WMB_NOM_MAX  64
#define WMB_TPRM_MAX 64
#define WMB_SPRM_MAX 4
#define WMB_UPRM_MAX 8
#define WMB_IPRM_MAX 2
typedef struct {
	int num;
	char log;
	unsigned char status;
	unsigned char cmde;
	char reponse_attendue;
	void *w; // struct WxiIdent *
	int x,y,l,h; char rc;
	char tprm[WMB_TPRM_MAX];
	short sprm[WMB_SPRM_MAX];
	unsigned short uprm[WMB_UPRM_MAX];
	int iprm[WMB_IPRM_MAX];
//	WmboxPoint *points;
} TypeWmboxCommande,*WmboxCommande;

typedef struct WxiEvent {
	struct WxiIdent *w;
	WmboxEventCode type;
	char log;
	unsigned long info;
	unsigned long stamp;
	int x,y;
	unsigned short mods;
	int num;
	unsigned char status;
} TypeWmboxEvent,*WmboxEvent;

typedef struct {
	int id; char nom[WMB_NOM_MAX];
	sem_t *verrou; char cdelog,evtlog;
	short cde_a_ecrire,cde_a_lire;
	short evt_a_ecrire,evt_a_lire;
	TypeWmboxCommande commande[WMB_CDE_MAX];
	TypeWmboxEvent event[WMB_EVT_MAX];
	WmboxPoint points[WMB_PTS_MAX];
	struct {
		int nb;
		int dim; WmboxPoint *val;
	} poly;
} TypeWmbox,*Wmbox;

//- WMBOX_VAR int WmboxId;
//- WMBOX_VAR short WmboxRequestNum,WmboxReplyNum,WmboxEventNum;

EXTERNC Wmbox WmboxCreate();
EXTERNC void WmboxShare(Wmbox mbx, char *nom);
EXTERNC void WmboxRaz(Wmbox mbx);
EXTERNC void WmboxRequestLog(Wmbox mbx, char log);
EXTERNC void WmboxEventLog(Wmbox mbx, char log);
EXTERNC void WmboxRequestStatus(Wmbox mbx, WmboxCommande commande, const char *fctn);
EXTERNC void WmboxEventStatus(Wmbox mbx, WmboxEvent event, const char *fctn);
EXTERNC void WmboxRequestError(Wmbox mbx, WmboxCommande commande, const char *cause, const char *fctn);

EXTERNC WmboxCommande WmboxRequestPtr(Wmbox mbx, unsigned char cmde);
EXTERNC char WmboxRequestSend(WmboxCommande commande);
EXTERNC char WmboxRequestReply(Wmbox mbx, WmboxCommande commande);
EXTERNC void WmboxRequestReuse(WmboxCommande commande, unsigned char cmde);
EXTERNC void WmboxRequestClose(Wmbox mbx, WmboxCommande commande);

EXTERNC WmboxEvent WmboxEventPtr(Wmbox mbx, WmboxEventCode type);
EXTERNC void WmboxEventSend(WmboxEvent event);
EXTERNC WmboxEvent WmboxEventNext(Wmbox mbx);
EXTERNC void WmboxEventFree(WmboxEvent event);

EXTERNC void WmboxFree(Wmbox mbx);

#endif /* WMBOX_H */
