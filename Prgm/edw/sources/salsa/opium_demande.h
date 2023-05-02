#ifndef OPIUM_DEMANDE_H
#define OPIUM_DEMANDE_H

//- #pragma message("Compilation pour "chaine(APPLI_NAME))
#if defined(ForSamba)
	#define ForOpiumQD

#elif defined(ForSamix)
	#define ForSamba
	#define ForOpiumX

#elif defined(ForGasol)
	#define ForSamba
	#define ForOpiumGL

#elif defined(ForSambaWXF)
	#define ForSamba
	#define ForOpiumWXF

#elif defined(ForSambaWXO)
	#define ForSamba
	#define ForOpiumWXO

#elif defined(ForSambaWXB)
	#define ForSamba
	#define ForOpiumWXB

#elif defined(ForSambaExec)
	#define ForSamba
	#define ForOpiumTS

#elif defined(ForTango)
	#define ForOpiumQD

#elif defined(ForIpaix)
	#define ForTango
	#define ForOpiumX

#elif defined(ForGigas)
	#define ForTango
	#define ForOpiumGL

#elif defined(ForTangoWXF)
	#define ForTango
	#define ForOpiumWXF

#elif defined(ForTangoWXO)
	#define ForTango
	#define ForOpiumWXO

#elif defined(ForTangoWXB)
	#define ForTango
	#define ForOpiumWXB

#endif

#ifdef OPIUM_VERIFIE_IMPLEMENTATION
	#if defined(ForOpiumQD)
		#pragma message("Application avec QUICKDRAW")

	#elif defined(ForOpiumX)
		#pragma message("Application avec X11")

	#elif defined(ForOpiumGL)
		#pragma message("Application avec OPENGL")

	#elif defined(ForOpiumWXF)
		#pragma message("Application avec WXWIDGETS en boucle fermee")

	#elif defined(ForOpiumWXO)
		#pragma message("Application avec WXWIDGETS en boucle ouverte")

	#elif defined(ForOpiumWXB)
		#pragma message("Application avec WXWIDGETS via mailbox")

	#elif defined(ForOpiumTS)
		#pragma message("Application Executive avec MBOX")

	#else
		#pragma message("WINDOW MANAGER INDEFINI!")

	#endif
#endif /* OPIUM_VERIFIE_IMPLEMENTATION */

#endif /* OPIUM_DEMANDE_H */
