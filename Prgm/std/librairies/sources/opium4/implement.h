#ifndef IMPLEMENT_H
#define IMPLEMENT_H

#ifndef __cplusplus
	#define EXTERNC
	typedef void               *WxiRef;
	// typedef struct wxImage   *WndSurface,WndTypeSurface;
	typedef void               *WxiCurseur;
#endif

/* ------------------------------------------------------------------------ */
/* ------------------------- IMPLEMENTATION WXWIDGETS --------------------- */
/* ------------------------------------------------------------------------ */
#ifdef WXWIDGETS
// #pragma message("Implementation graphique: WXWIDGETS")
	#ifndef ForOpiumWXB
		// wxi_interface.h defini WXI_DIRECT comme defined(ForOpiumWXF) || defined(ForOpiumWXO)
		#include "WXI/wxi_interface.h"

		typedef int WndLineParm; /* bidon */
		typedef WxiPoint   WndPoint;
		typedef WxiScreen  WndScreen;
		typedef WxiRef     WndIdent;
		typedef WxiCurseur WndCursor;
		#define WmboxEventCode unsigned short
	#endif

#endif /* WXWIDGETS */

/* ------------------------------------------------------------------------ */
/* -------------------------- IMPLEMENTATION MAILBOX ---------------------- */
/* ------------------------------------------------------------------------ */
#ifdef EVTS_PILE_MBOX
	#include "wmbox.h"
	typedef int        WndLineParm; /* bidon */
	typedef WmboxPoint WndPoint;
	typedef void      *WndIdent;
	typedef void      *WndScreen;
	// typedef struct wxImage   *WndSurface,WndTypeSurface;
	typedef void      *WndCursor;
#endif /* EVTS_PILE_MBOX */

#ifdef WXWIDGETS_OR_MBOX
	// recopie de <wx/defs.h>:
	#include "wxi_defs.h"

	#define WND_EVTMAXNUM WND_ACTIONTYPENB
	#define WND_ASCINT_WIDZ 15

	typedef enum {
		WND_MSELEFT   = 0,
		WND_MSEMIDDLE,
		WND_MSERIGHT,
		WND_MSEMONTE,
		WND_MSEDESC,
		WND_NBBUTTONS
	} WND_MSEBUTTONS;
	typedef enum {
		WND_ABS = 0,
		WND_REL = 1
	} WND_COORDINATES;
	typedef enum {
		XK_ASCII = 0,
		XK_Alt_L,
		XK_Home,
		XK_KP_F4,
		XK_Left,
		XK_Right,
		XK_Up,
		XK_Down,
		XK_NBKEYS
	} WND_KEYS;
	typedef enum {
		WND_CURS_STD = 0,
		WND_CURS_FLECHE_DR,
		WND_CURS_FLECHE_GA,
		WND_CURS_CROIX,
		WND_CURS_VISEUR,
		WND_CURS_MAIN
	} WND_CURSORS;

	typedef void *WndIdent;
	typedef int   WndFontId;
	typedef int   WndFontInfo;
	typedef struct {
		WndColorLevel  red;
		WndColorLevel  green;
		WndColorLevel  blue;
	} WndColor;
	typedef struct {
		int width,height;
		unsigned char *pixels;
	} *WndSurface,WndTypeSurface;
	typedef struct {
		WndFontId font;
		WndColor *foreground,*background;
		short line_width,line_style;
	} WndContextVal,*WndContextPtr;
	typedef struct  {
		WndIdent       w;
		UInt16         what;
		unsigned long  message;
		Ulong          when;
		struct { short v; short h; } where;
		UInt16         modifiers;
	} WndEvent;
#endif

/* ------------------------------------------------------------------------ */
/* ------------------------- IMPLEMENTATION OpenGL ------------------------ */
/* ------------------------------------------------------------------------ */
#ifdef OPENGL
// #pragma message("Implementation graphique: OPENGL")
	#ifdef Darwin
		#include <GLUT/glut.h>
	#endif
	#ifdef linux
		#include <GLUT/GL/glut.h>
	#endif
	// contient glutBitmapHeight mais necessite libglut.dylib et ligGL.dylib juste avant libX11.dylib
	//- #include <GL/freeglut.h>
	// #include <GLFW/glfw3.h> ne fait pas partie du folder std/librairies/OpenGL
	#include <glfw3.h>

	#define WND_ASCINT_WIDZ 15
	#define WndXToDouble(posx,sizx) (((double)(2 * (posx)) / (double)(sizx)) - 1.0)
	#define WndYToDouble(posy,sizy) (1.0 - ((double)(2 * (posy)) / (double)(sizy)))

	typedef GLFWmonitor *WndScreen;
	typedef GLFWwindow  *WndIdent;
	typedef GLFWcursor  *WndCursor;
	typedef GLFWimage   *WndSurface,WndTypeSurface;
	/* idem GLFWimage: typedef struct {
		int width,height;
		unsigned char *pixels;
	} *WndSurface,WndTypeSurface; */

	typedef int          WndFontId;
	typedef int    		 WndFontInfo;

	typedef struct {
		int sizx,sizy; char doit_terminer;
	} WndLineParm;

	typedef struct {
		int x,y;
	} WndPoint;

	typedef struct {
		WndColorLevel  red;
		WndColorLevel  green;
		WndColorLevel  blue;
	} WndColor;

	typedef struct {
		WndFontId font;
		WndColor *foreground,*background;
		short line_width,line_style;
	} WndContextVal,*WndContextPtr;

	typedef struct  {
		WndIdent       w;
		UInt16         what;
		unsigned long  message;
		Ulong          when;
		struct { short v; short h; } where;
		UInt16         modifiers;
	} WndEvent;

	#define WND_EVTMAXNUM WND_ACTIONTYPENB

	// GLFW_ARROW_CURSOR, GLFW_IBEAM_CURSOR, GLFW_CROSSHAIR_CURSOR,
	// GLFW_HAND_CURSOR, GLFW_HRESIZE_CURSOR et GLFW_VRESIZE_CURSOR
	typedef enum {
		WND_CURS_STD = GLFW_HAND_CURSOR,
		WND_CURS_FLECHE_DR = GLFW_ARROW_CURSOR,
		WND_CURS_FLECHE_GA = GLFW_ARROW_CURSOR,
		WND_CURS_CROIX = GLFW_CROSSHAIR_CURSOR,
		WND_CURS_VISEUR = GLFW_IBEAM_CURSOR,
		WND_CURS_MAIN = GLFW_HAND_CURSOR
	} WND_CURSORS;

	typedef enum {
		WND_ABS = 0,
		WND_REL = 1
	} WND_COORDINATES;

	typedef enum {
		WND_DEV_KBD = 0x1000,
		WND_DEV_TXT = 0x2000,
		WND_DEV_MSE = 0x3000,
		WND_DEV_MGR = 0x4000
	} WND_DEVICE;
	#define WND_DEV_MASK 0xF000
	#define WND_DEV_ACTN 0x0FFF

	typedef enum {
		WND_MSELEFT   = GLFW_MOUSE_BUTTON_LEFT,
		WND_MSEMIDDLE = GLFW_MOUSE_BUTTON_MIDDLE,
		WND_MSERIGHT  = GLFW_MOUSE_BUTTON_RIGHT,
		WND_MSEMONTE  = GLFW_MOUSE_BUTTON_4,
		WND_MSEDESC   = GLFW_MOUSE_BUTTON_5,
		WND_NBBUTTONS
	} WND_MSEBUTTONS;
	typedef enum {
		XK_ASCII = 0,
		XK_Alt_L = GLFW_MOD_SUPER, // GLFW_MOD_ALT,
		XK_Home,
		XK_KP_F4,
		XK_Left,
		XK_Right,
		XK_Up,
		XK_Down,
		XK_NBKEYS // = XK_Down
	} WND_KEYS;

/* fin implementation OPENGL */
#endif /* OPENGL */

#ifndef OPENGL
	typedef enum {
		GL_POINTS = 0,
		GL_LINES,
		GL_LINE_LOOP,
		GL_LINE_STRIP,
		GL_TRIANGLES,
		GL_TRIANGLE_STRIP,
		GL_TRIANGLE_FAN,
		GL_QUADS,
		GL_QUAD_STRIP,
		GL_POLYGON
	} WND_GL_VERTEXMODE;
#endif

/* ------------------------------------------------------------------------ */
/* -------------------------- IMPLEMENTATION X11 -------------------------- */
/* ------------------------------------------------------------------------ */
#ifdef X11
// #pragma message("Implementation graphique: X11")
	#include <X11/Xlibint.h>
	#include <X11/Xlib.h>
	#include <X11/Xutil.h>
	#include <X11/keysym.h>
	#include <X11/cursorfont.h>
	#define WND_INTERLIGNE 2
	#define WND_XMIN 7
	#define WND_YMIN 27
	#define WND_ASCINT_WIDZ 10

	// GC gere les informations via un element "GContext gid". Voir GraphDialogDonnee.

	typedef Display      *WndScreen;
	typedef XFontStruct  *WndFontInfo;
	typedef Font          WndFontId;
	typedef XColor        WndColor;
	typedef Window        WndIdent;
	typedef XGCValues     WndContextVal;
	typedef GC            WndContextPtr;

	#ifdef XLIB_ILLEGAL_ACCESS
		typedef struct {
			XExtData *ext_data;	/* hook for extension to hang data */
			GContext gid;	/* protocol ID for graphics context */
			Bool rects;		/* boolean: TRUE if clipmask is list of rectangles */
			Bool dashes;	/* boolean: TRUE if dash-list is really a list */
			unsigned long dirty;/* cache dirty bits */
			XGCValues values;	/* shadow structure of values */
		} *WndContextPtr;
	#endif

	typedef struct {
		WndScreen d;
	} WndLineParm;

	typedef XEvent        WndEvent;
	typedef XPoint        WndPoint; /* on veut simplement struct { short x,y } */
	typedef XImage       *WndSurface,WndTypeSurface;
	typedef Cursor        WndCursor;

	#define WND_EVTMAXNUM LASTEvent

	typedef enum {
		WND_MSELEFT   = Button1,
		WND_MSEMIDDLE = Button2,
		WND_MSERIGHT  = Button3,
		WND_MSEMONTE  = Button4,
		WND_MSEDESC   = Button5,
		WND_NBBUTTONS = 6
	} WND_MSEBUTTONS;
	typedef enum {
		WND_ABS = CoordModeOrigin,
		WND_REL = CoordModePrevious
	} WND_COORDINATES;
	typedef enum {
		WND_CURS_STD = XC_X_cursor,
		WND_CURS_FLECHE_DR = XC_arrow,
		WND_CURS_FLECHE_GA = XC_left_ptr,
		WND_CURS_CROIX = XC_crosshair,
		WND_CURS_VISEUR = XC_cross,
		WND_CURS_MAIN = XC_hand2
	} WND_CURSORS;

/* fin implementation X11 */
#endif /* X11 */

/* ------------------------------------------------------------------------ */
/* -------------------------- IMPLEMENTATION Windows ---------------------- */
/* ------------------------------------------------------------------------ */
#ifdef WIN32
// #pragma message("Implementation graphique: WIN32")

	#include <afxwin.h>
	#include <afx.h>

	#define WND_XMIN 7 /* ... */
	#define WND_YMIN 27 /* ... */
	#define WND_ASCINT_WIDZ 10 /* ... */
	#define WND_INTERLIGNE 2

	WND_VAR short WndTitleBar;
	WND_VAR short WndBorderSize;

	#define PI 3.14159265358979

	#define WND_EVTMAXNUM 515

	typedef enum {
		XK_ASCII = 0,
		XK_Alt_L = VK_MENU,
		XK_Home = VK_HOME,
		XK_KP_F4 = VK_F4,
		XK_Left = VK_LEFT,
		XK_Right = VK_RIGHT,
		XK_Up = VK_UP,
		XK_Down = VK_DOWN,
		XK_NBKEYS = 8
	} WND_KEYS;

	typedef enum {
		WND_MSELEFT = MK_LBUTTON,
		WND_MSEMIDDLE = MK_MBUTTON,
		WND_MSERIGHT = MK_RBUTTON,
		WND_MSEMONTE  = 4,
		WND_MSEDESC   = 5,
		WND_NBBUTTONS = 4
	} WND_MSEBUTTONS;

	typedef enum {
		WND_ABS = 0,
		WND_REL = 1
	} WND_COORDINATES;

	typedef HWND		WndIdent;
	typedef POINT		WndPoint;
	typedef COLORREF	WndColor;
	typedef HFONT		WndFontId;
	typedef HCURSOR		WndCursor;
	typedef MSG		    WndEvent;
	typedef INT		    WndScreen; /* ... */
	typedef HFONT		WndFontInfo; /* ... */

	typedef struct {
		PAINTSTRUCT paintst; HDC hDC; HPEN hPen, hPenOld;
	} WndLineParm;

 	typedef struct {
		WndFontId	font;
		WndColor	foreground;
		WndColor	background;
		int		line_width;
		int		line_style;
	} WndContextVal, *WndContextPtr;

	typedef struct {
		HDC hDC;
		PAINTSTRUCT paintStruct;
		int width, height;
		unsigned char *pixels;
	} *WndSurface,WndTypeSurface;

	#define WND_CURS_MAIN IDC_HAND
	#define WND_CURS_STD IDC_ARROW
	#define WND_CURS_FLECHE_DR IDC_ARROW
	#define WND_CURS_FLECHE_GA IDC_ARROW
	#define WND_CURS_CROIX IDC_CROSS
	#define WND_CURS_VISEUR IDC_CROSS

	#ifdef WIN32_DEBUG
		void OpiumWin32Log(char *fmt);
	#endif

/* fin implementation Windows */
#endif /* WIN32 */

/* ------------------------------------------------------------------------ */
/* ----------------------- IMPLEMENTATION QUICKDRAW ----------------------- */
/* ------------------------------------------------------------------------ */
#ifdef QUICKDRAW

// #pragma message("Implementation graphique: QUICKDRAW")
#ifdef CARBON
	// #pragma message("Implementation graphique: CARBON")
	#ifdef PROJECT_BUILDER
		#include <Carbon/Carbon.h>
	#else  /* !PROJECT_BUILDER */
		#ifdef XCODE
			//?? #define __CARBONSOUND__
			/*
			#undef  __OSX_AVAILABLE_STARTING
			#define __OSX_AVAILABLE_STARTING(i,j)
			#undef  __OSX_AVAILABLE_BUT_DEPRECATED_MSG
			#define __OSX_AVAILABLE_BUT_DEPRECATED_MSG(i,j,k,l,m)
			*/
			#include <Carbon/Carbon.h>
			// Ou est-il??? #include <CoreServices/CarbonCore/CarbonCore.h>
			#include <CoreServices/CoreServices.h>
			#include <CoreGraphics/CoreGraphics.h>
//			#include "/System/Library/Frameworks/ApplicationServices.framework/Frameworks/QD.framework/Headers/QD.h"
		#else  /* !XCODE */
			#include <Carbon.h>
		#endif /* !XCODE */
	#endif /* !PROJECT_BUILDER */
#else /* CW5 */
	#include <Quickdraw.h>
	/* Ajout pour G4MiG et Samba/G3 (mais pas Tango/G3...): */
	#include <Events.h>
#endif /* CARBON */

//#ifdef CW5
typedef short QDpos;
//#else
//typedef int QDpos;
//#endif

#define WND_ASCINT_WIDZ 15
#define WND_INFRONT (WindowPtr)-1
#define WND_BEHIND  NIL

/* Version 1: typedef QDGlobals    *WndScreen; */
typedef BitMap        WndScreen;
typedef FontInfo      WndFontInfo;
typedef short         WndFontId;
typedef RGBColor      WndColor;
typedef EventRecord   WndEvent;
typedef int WndCursor;
#ifdef CARBON
	typedef WindowRef     WndIdent;
#else
	typedef WindowPtr     WndIdent;
#endif

typedef int WndLineParm; /* bidon */
typedef struct {
	int x,y;
} WndPoint;

typedef struct {
	int width,height;
	unsigned char *pixels;
} *WndSurface,WndTypeSurface;

typedef struct {
	WndFontId font;
	WndColor *foreground,*background;
	short line_width,line_style;
} WndContextVal,*WndContextPtr;

#define WND_EVTMAXNUM kHighLevelEvent+1

typedef enum {
	WND_MSELEFT = 1,
	WND_MSEMIDDLE,
	WND_MSERIGHT,
	WND_MSEMONTE,
	WND_MSEDESC,
	WND_NBBUTTONS = 4
} WND_MSEBUTTONS;
typedef enum {
	XK_ASCII = 0,
	XK_Alt_L,
	XK_Home,
	XK_KP_F4,
	XK_Left,
	XK_Right,
	XK_Up,
	XK_Down,
	XK_NBKEYS // = XK_Down
} WND_KEYS;
typedef enum {
	WND_CURS_STD = 0,
	WND_CURS_FLECHE_DR,
	WND_CURS_FLECHE_GA,
	WND_CURS_CROIX,
	WND_CURS_VISEUR,
	WND_CURS_MAIN
} WND_CURSORS;
typedef enum {
	WND_ABS = 0,
	WND_REL = 1
} WND_COORDINATES;

/* Gestion du MenuBar */
#define MENU_POMME   32000
#define   ITEM_APROPOS   1
#define MENU_FICHIER 32001
#define   ITEM_NOUVEAU   1
#define   ITEM_OUVRIR    2
#define   ITEM_FERMER    3
#define   ITEM_SAUVER    4
#define   ITEM_PAGE      6
#define   ITEM_IMPRIMER  7
#define   ITEM_QUITTER   9
#define MENU_EDITER  32002
#define   ITEM_ANNULER   1
#define   ITEM_COUPER    3
#define   ITEM_COPIER    4
#define   ITEM_COLLER    5
#define   ITEM_EFFACER   6
#define   ITEM_SELECTALL 8

/* fin implementation MAC */
#endif /* QUICKDRAW */

/* ------------------------------------------------------------------------ */
/* ---------------------- IMPLEMENTATION INDETERMINEE --------------------- */
/* ------------------------------------------------------------------------ */
#ifdef INDEFINI

	#ifdef OPIUM_VERIFIE_IMPLEMENTATION
		#pragma message("Compilation INDEFINIE")
	#endif
	#define WND_ASCINT_WIDZ 15
	#define WND_INFRONT (WindowPtr)-1
	#define WND_BEHIND  NIL
	#define WND_EVTMAXNUM WND_ACTIONTYPENB

	typedef void      *WndIdent;
	typedef void      *WndScreen;
	typedef void      *WndCursor;
	typedef int        WndFontId;
	typedef int        WndFontInfo;
	typedef struct {
		WndColorLevel  red;
		WndColorLevel  green;
		WndColorLevel  blue;
	} WndColor;
	typedef struct {
		WndFontId font;
		WndColor *foreground,*background;
		short line_width,line_style;
	} WndContextVal,*WndContextPtr;
	typedef struct  {
		WndIdent       w;
		UInt16         what;
		unsigned long  message;
		Ulong          when;
		struct { short v; short h; } where;
		UInt16         modifiers;
	} WndEvent;

	typedef int WndLineParm; /* bidon */
	typedef struct {
		int x,y;
	} WndPoint;

	typedef struct {
		int width,height;
		unsigned char *pixels;
	} *WndSurface,WndTypeSurface;

	typedef enum {
		WND_MSELEFT = 1,
		WND_MSEMIDDLE,
		WND_MSERIGHT,
		WND_MSEMONTE,
		WND_MSEDESC,
		WND_NBBUTTONS = 4
	} WND_MSEBUTTONS;
	typedef enum {
		XK_ASCII = 0,
		XK_Alt_L,
		XK_Home,
		XK_KP_F4,
		XK_Left,
		XK_Right,
		XK_Up,
		XK_Down,
		XK_NBKEYS // = XK_Down
	} WND_KEYS;

/* fin implementation indefinie */
#endif /* INDEFINI */

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

#ifdef WIN32
	int WndPrint(const char *szfmt, ...);
	#define BYTESWAP
	#define WIN32_BACKGROUND
	#define DEBUG_CONSOLE_SIZE 500
#else
	#define WndPrint printf
#endif

#endif /* IMPLEMENT_H */
