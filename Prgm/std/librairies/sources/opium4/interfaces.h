#ifndef INTERFACES_H
#define INTERFACES_H

#ifdef OS9
	#define X11
#endif /* OS9 */

#if defined(Foropium4)
	#define QUICKDRAW
	#define CARBON

#elif defined(ForOpiumX)
	#define X11

#elif defined(ForOpiumGL)
	#define OPENGL

#elif defined(ForOpiumWXF) || defined(ForOpiumWXO) || defined(ForOpiumWXB)
	#define WXWIDGETS

#elif defined(ForOpiumTS)
	#define MBOX

#endif

#if !defined(QUICKDRAW) && !defined(X11) && !defined(OPENGL) && !defined(WXWIDGETS) && !defined(MBOX)
	#define INDEFINI
#endif



#ifdef BATCH
	#ifdef X11
		#undef X11
	#endif /* X11 */
	#ifdef OPENGL
		#undef OPENGL
	#endif /* OPENGL */
	#ifdef WXWIDGETS
		#undef WXWIDGETS
	#endif /* WXWIDGETS */
	#ifdef QUICKDRAW
		#undef QUICKDRAW
	#endif /* QUICKDRAW */
#endif /* BATCH */

#ifdef QUICKDRAW
	#define MENU_BARRE
	#define MENU_BARRE_MAC
	#define QUICKDRAW_OR_OPENGL
	#define QUICKDRAW_OR_OPENGL_OR_WXWIDGETS
	#define QUICKDRAW_OR_OPENGL_OR_WXWIDGETS_OR_MBOX
#endif /* QUICKDRAW */

#ifdef OPENGL
	#define EVTS_PILE_INTERNE
	#define QUICKDRAW_OR_OPENGL
	#define OPENGL_OR_WXWIDGETS
	#define QUICKDRAW_OR_OPENGL_OR_WXWIDGETS
	#define QUICKDRAW_OR_OPENGL_OR_WXWIDGETS_OR_MBOX
#endif /* OPENGL */

#ifdef WXWIDGETS
/* WXF: boucle standard (fermee) -> gere les evts via WndEventFromWx, qui appelle WndEventBuild et OpiumManage
   WXO: boucle ouverte -> gere les evts via WndEventFromWx, qui empile les evts (pile geree par Opium)
   WXB: passe par une appli serveur, avec dialogue via mailbox
*/
	#if defined(ForOpiumWXO)
		#define EVTS_PILE_INTERNE
	#elif defined(ForOpiumWXB)
		#define EVTS_PILE_MBOX
	#endif
	#define OPENGL_OR_WXWIDGETS
	#define QUICKDRAW_OR_OPENGL_OR_WXWIDGETS
	#define QUICKDRAW_OR_OPENGL_OR_WXWIDGETS_OR_MBOX
#endif /* ForOpiumWX */

#ifdef MBOX
	#define EVTS_PILE_MBOX
	#define QUICKDRAW_OR_OPENGL_OR_WXWIDGETS_OR_MBOX
#endif

/* WXF et WXO necessitent de compiler wxi_interface avec l'option WXI_DIRECT
   WXB necessite de compiler wxi_interface avec l'option WXI_MBX_APPLI (qui utilise MBOX),
       PLUS d'executer le serveur (SambaGT) compile avec wxi_interface option WXI_MBX_SERVEUR
   MBOX necessite d'executer le serveur (si WxWidgets, compile avec wxi_interface option WXI_MBX_SERVEUR)
*/

#if defined(WXWIDGETS) || defined(MBOX)
	#define WXWIDGETS_OR_MBOX
#endif

#ifdef WIN32
	int WndPrint(const char *szfmt, ...);
	#define BYTESWAP
	#define WIN32_BACKGROUND
	#define DEBUG_CONSOLE_SIZE 500
#else
	#define WndPrint printf
#endif

#endif /* INTERFACES_H */
