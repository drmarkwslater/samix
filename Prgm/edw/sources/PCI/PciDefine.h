#ifndef PCIDEFINE_H
#define PCIDEFINE_H

#ifdef CODE_WARRIOR_VSN
#include <defineVAR.h>
#else
#include <utils/defineVAR.h>
#endif

#ifdef UNIX
	#ifndef macintosh
		#ifndef SANS_PCI
			#define SANS_PCI
		#endif
	#endif
#endif

#ifdef SANS_PCI
	#include <PciOffsets.h>
	typedef hexa io_connect_t;
	typedef hexa RegEntryID;
	typedef io_connect_t TypePciBoard,*PciBoard;

#else /* !SANS_PCI */
	#ifdef VXWORKS
		#include <globals.h>
		#include <ioLib.h>     /* pour ioctl */
		#include <sysLib.h>
		#include <dllLib.h>
		/* #include <drv/pci/pciConfigLib.h> */
		/* #include <pciMapLib.h> double-emploi avec le precedent */
		#include <vxWorks.h>   /* pour PciInit, ainsi que les suivants */
		#include <dllLib.h>
		#include "pciMapLib.h"
		#include "config.h" /* voir commentaire suivant, extrait de pciIomapLib.c */
		/*
		 * The following defines specify, by default, the maximum number of busses,
		 * devices and functions allowed by the PCI 2.1 Specification.
		 *
		 * Any or all may be overriden by defining them in config.h.
		 */
		typedef struct {
			byte b;  /* Bus      */
			byte d;  /* Device   */
			byte f;  /* Function */
		} TypePciBoard,*PciBoard;
	#endif /* VXWORKS */

	#ifdef macintosh
		#ifdef XCODE
			#include <IOKit/IOKitLib.h>
			typedef io_connect_t TypePciBoard,*PciBoard; // au final, hexa
		#else /* MACOS classique */
			// #include <PCI.h>
			// typedef Ulong UINT32;
			#include <NameRegistry.h>
			typedef struct {
				RegEntryID id;
				Ulong *PCIadrs,*ResetAdrs;
				Ulong *adrs_lues,max_lues,nb_lues;
				Ulong  Empty,Half,Full;
				Ulong *PLXregs;
				Ulong  taille;
				Ulong *fifo;
			} TypePciBoard,*PciBoard;
			char *PciName[]
			#ifdef MAINFILE
			 = {
				"Devices:device-tree:pci",
				"Devices:device-tree:pci:pci-bridge",
				"Devices:device-tree:bandit",
			/*	"Root:PowerMac3,1:Core99PE:pci:AppleMacRiscPCI:pci-bridge:IOPCI2PCIBridge", */
				"\0"
			}
			#endif
			;
		#endif /* MACOS classique */
	#endif /* macintosh */

	#ifdef CODE_WARRIOR_VSN
		#include <PciOffsets.h>
	#else
		#include <PCI/PciOffsets.h>
	#endif
	#define ERROR -1
#endif /* !SANS_PCI */

// PciConfigReadByte(PCIboard b, void *adrs, byte *val)
#ifdef VXWORKS
	#define PciConfigReadByte(b,adrs,val) pciConfigInByte(b->b,b->d,b->f,adrs,val)
	#define PciConfigReadWord(b,adrs,val) pciConfigInWord(b->b,b->d,b->f,adrs,val)
	#define PciConfigReadLong(b,adrs,val) pciConfigInLong(b->b,b->d,b->f,adrs,val)
	#define PciConfigWriteByte(b,adrs,val) pciConfigOutByte(b->b,b->d,b->f,adrs,val)
	#define PciConfigWriteWord(b,adrs,val) pciConfigOutWord(b->b,b->d,b->f,adrs,val)
	#define PciConfigWriteLong(b,adrs,val) pciConfigOutLong(b->b,b->d,b->f,adrs,val)
#endif
#ifdef macintosh
	#ifdef XCODE
		#define PciConfigReadByte(b,adrs,val) IcaConfigReadByte(b,(int)(adrs),(char *)val)
		#define PciConfigReadWord(b,adrs,val) IcaConfigReadWord(b,(int)(adrs),(short *)val)
		#define PciConfigReadLong(b,adrs,val) IcaConfigReadLong(b,(int)adrs,(int *)val)
		#define PciConfigWriteByte(b,adrs,val) IcaConfigWriteByte(b,(int)(adrs),(int)val)
		#define PciConfigWriteWord(b,adrs,val) IcaConfigWriteWord(b,(int)(adrs),(int)val)
		#define PciConfigWriteLong(b,adrs,val) IcaConfigWriteLong(b,(int)adrs,(int)val)
	#else
		#define PciConfigReadByte(b,adrs,val) ExpMgrConfigReadByte(&(b->id),(void *)(adrs),(UInt8 *)val)
		#define PciConfigReadWord(b,adrs,val) ExpMgrConfigReadWord(&(b->id),(void *)(adrs),(UInt16 *)val)
		#define PciConfigReadLong(b,adrs,val) ExpMgrConfigReadLong(&(b->id),(void *)(adrs),(Ulong *)val)
		#define PciConfigWriteByte(b,adrs,val) ExpMgrConfigWriteByte(&(b->id),(void *)(adrs),(UInt8)val)
		#define PciConfigWriteWord(b,adrs,val) ExpMgrConfigWriteWord(&(b->id),(void *)(adrs),(UInt16)val)
		#define PciConfigWriteLong(b,adrs,val) ExpMgrConfigWriteLong(&(b->id),(void *)(adrs),(Ulong)val)
	#endif
#endif

#endif /* PCIDEFINE_H */
