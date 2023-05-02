#ifndef ACTIONCODES_H
#define ACTIONCODES_H

typedef enum {
	WND_EXPOSE = 0,
	WND_MOVED,
	WND_RESIZE,
	WND_CONFIG,
	WND_FOCUS,
	WND_POUBELLE,
	WND_DELETE,
	WND_KEY,
	WND_NOKEY,
	WND_PRESS,
	WND_RELEASE,
	WND_DOUBLE,
#ifdef WND_SCROLLX
	WND_SCROLLX,
#endif
#ifdef WND_SCROLLX
	WND_SCROLLY,
#endif
#ifdef MENU_BARRE
	WND_BARRE,
#endif
	WND_PAINTNOW,
	WND_EXIT,
	WND_ACTIONTYPENB
} WND_ACTIONTYPE;

#ifdef WND_ACTIONNAMES
	#define WND_ACTIONVAR
#else
	#define WND_ACTIONVAR extern
#endif

WND_ACTIONVAR const char *WndActionName[WND_ACTIONTYPENB]
#ifdef WND_ACTIONNAMES
 = {
	"Expose",
	"Moved",
	"Resize",
	"Config",
	"FocusIn",
	"Poubelle",
	"Delete",
	"KeyPress",
	"KeyRelease",
	"MousePress",
	"MouseRelease",
	"Double",
	// autres types: definis ou pas dans <actioncodes.h>
#ifdef WND_SCROLLX
	"ScrollX",
#endif
#ifdef WND_SCROLLY
	"ScrollY",
#endif
#ifdef MENU_BARRE
	"Barre",
#endif
	"PaintNow",
	"Exit"
}
#endif
;

#define WND_ACTIONNAME(type) (type >= 0)?((type < WND_ACTIONTYPENB)? WndActionName[type]: "inconnu"): "inexistant"

#endif /* ACTIONCODES_H */
