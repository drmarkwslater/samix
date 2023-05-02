#include <environnement.h>

#ifdef XCODE
	#include <stdio.h>
#endif

typedef hexa Ulong;

#ifdef CODE_WARRIOR_VSN
	#include <PciLib.h>
#else
	#include <PCI/PciLib.h>
#endif

#ifdef macintosh
#ifndef XCODE
char *PciName[] = {
	"Devices:device-tree:pci",
	"Devices:device-tree:pci:pci-bridge",
	"Devices:device-tree:bandit",
/*	"Root:PowerMac3,1:Core99PE:pci:AppleMacRiscPCI:pci-bridge:IOPCI2PCIBridge", */
	"\0"
};
#endif
#endif

/* ========================================================================== */
hexa PciDeviceID(PciBoard board) {
#ifdef SANS_PCI
	return(0);
#else /* !SANS_PCI */
	hexa id_lu;

	if((int)board == 0) return(0); else {
		// PciConfigReadLong(board,PCI_CFG_VENDOR_ID,&id_lu);
		PciConfigReadLong(board,PCI_CFG_SUB_VENDOR_ID,&id_lu);
		// printf("  CSR#0 lu: %08X\n",id_lu);
		return(id_lu);
	}
#endif /* !SANS_PCI */
}
/* ========================================================================== */
char PciDeviceFind(shex vid, shex did, int index, PciBoard board) {
#ifdef SANS_PCI
	return(0);
#else /* !SANS_PCI */
	#ifdef VXWORKS
		int rc;
		int b,d,f;

		rc = pciFindDevice(vid,did,index,&b,&d,&f);
		board->b = b; board->d = d; board->f = f;
		return((rc == ERROR)? 0: 1);
	#endif /* ! */
	#ifdef macintosh
		#ifdef XCODE
			hexa id_lu,id_cherche;

			if((int)*board == 0) return(0); else {
				id_cherche = ((did << 16) & 0xFFFF0000) | (vid & 0xFFFF);
		//		PciConfigReadLong(board,PCI_CFG_VENDOR_ID,&id_lu);
		//		printf("  CSR#0 envisage: %08X (lu: %08X)\n",id_cherche,id_lu);
				PciConfigReadLong(board,PCI_CFG_SUB_VENDOR_ID,&id_lu);
				printf("  CSR#0B envisage: %08X (lu: %08X)\n",id_cherche,id_lu);
				return(id_cherche == id_lu);
			}
		#else /* !XCODE */
			char nom[256];
			int type;

			printf("  CSR#0B envisage: %04X-%04X\n",vid,did);
			type = 0;
			while(PciName[type][0]) {
				sprintf(nom,"%s:pci%04x,%04x",PciName[type],vid,did);
				if(RegistryCStrEntryLookup(NULL,nom,&(board->id)) == noErr) return(1);
				type++;
			}
			return(0);
		#endif /* !XCODE */
	#endif /* !macintosh */
#endif /* !SANS_PCI */
}
/* ========================================================================== */
Ulong *PciDeviceAdrs(PciBoard board, int reg) {
#ifdef SANS_PCI
 	return(0);
#else
	Ulong *adrs;

 	PciConfigReadLong(board,PCI_CFG_BASE_ADDRESS_0 + (reg * 4),&adrs);
    return(adrs);
 #endif
}
