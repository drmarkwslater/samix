#ifndef ENVIRONNEMENT_H
#define ENVIRONNEMENT_H

// defines du compilateur cpp standard: "touch foo.h; cpp -dM foo.h"
// defines du compilateur Xcode C++: touch foo.hh ; xpp -dM -E foo.hh
// (avec "alias xpp /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++")

/*
(#define XCODE  : ajoute dans les flags de compilation)
(#define UNIX   : ajoute dans les flags de compilation)
*/
#undef OS9
#undef CODE_WARRIOR_VSN
#define SANS_PCI
#define ATTENTE_AVEC_USLEEP

#ifdef XCODE
	#define Darwin
	#define CHAINE_VSN_EXTERNE
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

#ifdef Darwin
//#define macintosh
	#define MACOSX
	#define ACCES_DB
#endif /* Darwin */

#endif /* ENVIRONNEMENT_H */
