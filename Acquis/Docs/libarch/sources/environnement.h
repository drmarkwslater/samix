#ifndef ENVIRONNEMENT_H
#define ENVIRONNEMENT_H

/* 
(#define XCODE  : ajoute dans les flags de compilation)
(#define UNIX   : ajoute dans les flags de compilation)
*/
#undef OS9
#undef CODE_WARRIOR_VSN
#define SANS_PCI

#ifdef XCODE
	#define darwin
	#define CHAINE_VSN_EXTERNE
#else
	#define X11
#endif /* XCODE */

#ifdef GCC
	// #define INLINE inline
	#define INLINE
#else
	#define INLINE
#endif /* GCC */
//#ifdef GCC
//#pragma message "Avec GCC"
//#else
//#pragma message "Sans GCC"
//#endif

#ifdef darwin
	#define macintosh
	#define MACOSX
	#define DELAIS_UNIX
	#define LONG_IS_32BITS
	#define ACCES_DB
#endif /* darwin */

#ifndef X11
	#define CARBON
#endif /* !X11 */

#ifdef CARBON
	#include <EnvironnementApple.h>
#endif /* CARBON */

#endif /* ENVIRONNEMENT_H */
