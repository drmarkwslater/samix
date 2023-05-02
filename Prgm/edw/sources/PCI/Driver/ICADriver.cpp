#include <IOKit/IOLib.h>
#include <ICADriver.h>
#include <plx.h>
// Code de retour de type IOReturn dans:
//         /System/Library/Frameworks/IOKit.framework/Versions/A/Headers/IOReturn.h

// #include <Timer.h>
// extern void Microseconds(UInt64 *microTickCount);
// #include <unistd.h>
// void usleep(hexa microsec);

#define PPC

typedef union {
	struct {
		byte b0,b1,b2,b3;
	} octet;
	Ulong mot;
} IntSwappable;

#define CLUZEL_NOT_FULLFIFO 0x00400000
#define CLUZEL_NOT_HALFFIFO 0x00200000
#define CLUZEL_NOT_EMPTY    0x00100000
#define CLUZEL_WASFULL      0x00080000
#define CLUZEL_FullFull(val) (((val) & CLUZEL_NOT_FULLFIFO) == 0)
#define CLUZEL_HalfFull(val) (((val) & CLUZEL_NOT_HALFFIFO) == 0)
/* #define CLUZEL_Empty(val) (((val) & CLUZEL_NOT_EMPTY) == 0) */
#define CLUZEL_Empty(val) (((val) & 0xFFFF) == 0)  /* Plus sur pour le moment?? */

// #define DEBUG

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define super IOService 
OSDefineMetaClassAndStructors(ICADriver, IOService)
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* ========================================================================== */
/* ACCES PCI STANDARD                                                         */
/* ========================================================================== */
IOReturn ICADriver::ConfigRead32(int adrs, int *val) {
    // If the user client's 'open' method was never called, return kIOReturnNotOpen.
    if(!isOpen()) return(kIOReturnNotOpen);
    *val = carte->configRead32((UInt8)adrs);
#ifdef DEBUG
    IOLog("ICADriver::ConfigRead(adrs=%d,&val) -> %08X\n",adrs,*val);
#endif
    return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::ConfigWrite32(int adrs, int val) {
    if(!isOpen()) return(kIOReturnNotOpen);
    carte->configWrite32((UInt8)adrs,(Ulong)val);
    return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::ConfigRead16(int adrs, shex *val) {
    if(!isOpen()) return(kIOReturnNotOpen);
    *val = (shex)carte->configRead16((UInt8)adrs);
    return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::ConfigWrite16(int adrs, int val) {
    if(!isOpen()) return(kIOReturnNotOpen);
    carte->configWrite16((UInt8)adrs,(UInt16)val);
    return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::ConfigRead8(int adrs, byte *val) {
	if(!isOpen()) return(kIOReturnNotOpen);
	*val = (byte)carte->configRead8((UInt8)adrs);
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::ConfigWrite8(int adrs, int val) {
	if(!isOpen()) return(kIOReturnNotOpen);
	carte->configWrite8((UInt8)adrs,(UInt8)val);
	return(kIOReturnSuccess);
}

/* ========================================================================== */
/* Impression des registres de configuration PCI                              */
/* ========================================================================== */
#include <PciOffsets.h>

#define KB 1024
#define MB (KB * KB)

#ifndef PCI_MAX_BUS
#  define PCI_MAX_BUS	256
#endif

#ifndef PCI_MAX_DEV
#  define PCI_MAX_DEV	21
#endif

#ifndef PCI_MAX_FUNC
#  define PCI_MAX_FUNC	8
#endif

enum {
	PCI_CLASS_OLD = 0,
	PCI_CLASS_DISK,
	PCI_CLASS_NET,
	PCI_CLASS_DPLY,
	PCI_CLASS_MMED,
	PCI_CLASS_MEM,
	PCI_CLASS_BRDG,
	PCI_CLASS_COMM,
	PCI_CLASS_SYST,
	PCI_CLASS_INPT,
	PCI_CLASS_DOCK,
	PCI_CLASS_PROC,
	PCI_CLASS_SRAL,
	PCI_CLASS_OTHR,
	PCI_NB_CLASSES
};
char *Classe[PCI_NB_CLASSES] = {
	"old",
	"disk",
	"net",
	"dply",
	"mmed",
	"mem",
	"brdg",
	"comm",
	"syst",
	"inpt",
	"dock",
	"proc",
	"sral",
	"othr"
};
char *TypeDevice[PCI_NB_CLASSES] = {
	"Ancien device",
	"Disque",
	"Lien reseau",
	"Display",
	"Multimedia",
	"Memoire",
	"Bridge",
	"Communication",
	"Systeme",
	"Input",
	"Docking",
	"Processeur",
	"E/S serie",
	"Divers"
};
#define PCI_NB_DISKS 6
char *Disque[PCI_NB_DISKS] = {
	"SCSI",
	"IDE",
	"floppy",
	"IPI",
	"RAID",
	"divers"
};
#define PCI_NB_NETS 5
char *Net[PCI_NB_NETS] = {
	"Ethernet",
	"Token Ring",
	"FDDI",
	"ATM",
	"divers"
};
#define PCI_NB_BRIDGES 9
char *Bridge[PCI_NB_BRIDGES] = {
	"Host/PCI",
	"PCI/ISA",
	"PCI/EISA",
	"PCI/MicroChannel",
	"PCI/PCI",
	"PCI/PCMCIA",
	"PCI/NuBus",
	"PCI/CardBus",
	"divers"
};
#define PCI_NB_INPUTS 4
char *InputDev[PCI_NB_INPUTS] = {
	"keyboard",
	"digitizer (pen)",
	"mouse",
	"divers"
};
#define PCI_NB_SERIALS 5
char *Serial[PCI_NB_SERIALS] = {
	"FireWire",
	"ACCESSbus",
	"SSA",
	"USB",
	"FibreChannel"
};

/* ========================================================================== */
	IOReturn ICADriver::PciRegDump(char memoire) {
	Ulong val,did,mem,cmd,msk;
	char vendeur[32];
	int id,classe;
	int i,k;
	
	IOLog("ICADriver::PciRegDump --- Registres de configuration PCI:\n");
	did = carte->configRead32(PCI_CFG_VENDOR_ID);
	if(did == 0xFFFFFFFF) {
		IOLog("ICADriver::PciRegDump: carte non identifiable\n");
		return(kIOReturnError);
	}

	id = did & 0xFFFF;
	if(id == 0x1057) strcpy(vendeur,"Motorola");
	else if(id == 0x1000) strcpy(vendeur,"Symbios");
	else if(id == 0x1011) strcpy(vendeur,"DEC");
	else if(id == 0x10AD) strcpy(vendeur,"Windbond");
	else if(id == 0x10B5) strcpy(vendeur,"PLX");
	else if(id == 0x10DC) strcpy(vendeur,"CERN");
	else if(id == 0x11B0) strcpy(vendeur,"RAMiX");
	else sprintf(vendeur,"%04X",id);
	
	val = carte->configRead32(PCI_CFG_REVISION);
	classe = (val >> 8) & 0xFFFFFF;
	i = (classe >> 16) & 0xFF;
	k = (classe >> 8) & 0xFF;
	if((i < 0) || (i >= PCI_NB_CLASSES)) i = PCI_NB_CLASSES - 1;
	IOLog("%s",TypeDevice[i]);
	if(i == PCI_CLASS_OLD) {
		if(k == 0) IOLog(" non-VGA"); else if(k == 1) IOLog(" VGA");
	} else if(i == PCI_CLASS_DISK) {
		if(k >= PCI_NB_DISKS) k = PCI_NB_DISKS - 1;
		IOLog(" %s",Disque[k]);
	} else if(i == PCI_CLASS_NET) {
		if(k >= PCI_NB_NETS) k = PCI_NB_NETS - 1;
		IOLog(" %s",Net[k]);
	} else if(i == PCI_CLASS_DPLY) {
		if(k == 0) {
			i = classe & 0xFF;
			if(i == 0) IOLog(" compatible VGA"); else if(i == 1) IOLog(" compatible 8514");
		} else if(k == 1) IOLog(" XGA");
	} else if(i == PCI_CLASS_MMED) {
		if(k == 0) IOLog(" video"); else if(k == 1) IOLog(" audio");
	} else if(i == PCI_CLASS_MEM) {
		if(k == 0) IOLog(" RAM"); else if(k == 1) IOLog(" Flash");
	} else if(i == PCI_CLASS_BRDG) {
		if(k >= PCI_NB_BRIDGES) k = PCI_NB_BRIDGES - 1;
		IOLog(" %s",Bridge[k]);
	} else if(i == PCI_CLASS_INPT) {
		if(k >= PCI_NB_INPUTS) k = PCI_NB_INPUTS - 1;
		IOLog(" %s",InputDev[k]);
	} else if(i == PCI_CLASS_PROC) {
		if(k == 0x00) IOLog(" 386"); 
		else if(k == 0x01) IOLog(" 486"); 
		else if(k == 0x02) IOLog(" Pentium"); 
		else if(k == 0x10) IOLog(" Alpha"); 
		else if(k == 0x20) IOLog(" PowerPC"); 
		else if(k == 0x40) IOLog(" (co-pro)"); 
	} else if(i == PCI_CLASS_SRAL) {
		if(k >= PCI_NB_SERIALS) k = PCI_NB_SERIALS - 1;
		IOLog(" %s",Serial[k]);
	}
	IOLog(" %s-%04X",vendeur,(did >> 16) & 0xFFFF);
	IOLog(" revision %d\n\n",val & 0xFF);
	
	IOLog("Device: %04X                                Vendor: %04X\n",
		  (did >> 16) & 0xFFFF,did & 0xFFFF);
	
	cmd = carte->configRead32(PCI_CFG_COMMAND);
	IOLog("Status: %04X                               Command: %04X ",
		  (cmd >> 16) & 0xFFFF,cmd & 0xFFFF);
	if(cmd & 7) {
		if(cmd & PCI_CMD_MASTER_ENABLE) IOLog("-Master");
		if(cmd & PCI_CMD_MEM_ENABLE) IOLog("-Mem");
		if(cmd & PCI_CMD_IO_ENABLE) IOLog("-I/O");
		IOLog(" enabled\n");
	} else IOLog("-no access\n");
	
	IOLog("Classe: %06X                            Revision:   %02X\n",
		  classe,val & 0xFF);
	
	val = carte->configRead32(PCI_CFG_CACHE_LINE_SIZE);
	IOLog("BIST: %02X     HdrType:  %02X     Latcy:  %02X     Cache:   %02X\n",
		  (val >> 24) & 0xFF,(val >> 16) & 0xFF,(val >> 8) & 0xFF,val & 0xFF);
	
	for(i=0; i<6; i++) {
		mem = carte->configRead32(PCI_CFG_BASE_ADDRESS_0 + (i * 4));
		if(memoire) {
			val = 0xFFFFFFFF;
			carte->configWrite32(PCI_CFG_BASE_ADDRESS_0 + (i * 4),val);
			val = carte->configRead32(PCI_CFG_BASE_ADDRESS_0 + (i * 4));
			if(val == 0) break;
			carte->configWrite32(PCI_CFG_BASE_ADDRESS_0 + (i * 4),mem);
		} else val = mem;
		IOLog("Base address  #%d: %08X  ",i,mem);
		if(val & PCI_BASE_IO) IOLog("(I/O space"); else IOLog("(Mem space");
		if((val & 0x6) == PCI_BASE_BELOW_1M) IOLog(", base<1Mb [0x00100000]");
		if((val & 0x6) == PCI_BASE_IN_64BITS) IOLog(", 64 bits");
		if(memoire) {
			IOLog(", =%08X: ",val);
			msk = (val & PCI_BASE_IO)?  PCI_IOBASE_MASK: PCI_MEMBASE_MASK;
			k = 0xFFFFFFFF - (val & msk); k++;
			if(k < KB) IOLog("%d bytes",k);
			else if(k < MB) IOLog("0x%04X bytes [%d Kb]",k,k/KB);
			else IOLog("0x%08X bytes [%d Mb]",k,k/MB);
		}
		IOLog(")\n");
	}
	
	val = carte->configRead32(PCI_CFG_CIS);
	IOLog("CardBus CIS pntr: %08X\n",val);
	
	val = carte->configRead32(PCI_CFG_SUB_VENDOR_ID);
	id = val & 0xFFFF;
	if(id == 0x1057) strcpy(vendeur,"Motorola");
	else if(id == 0x1000) strcpy(vendeur,"Symbios");
	else if(id == 0x1011) strcpy(vendeur,"DEC");
	else if(id == 0x10AD) strcpy(vendeur,"Windbond");
	else if(id == 0x10B5) strcpy(vendeur,"PLX");
	else if(id == 0x10DC) strcpy(vendeur,"CERN");
	else if(id == 0x11B0) strcpy(vendeur,"RAMiX");
	else sprintf(vendeur,"%04X",id);
	IOLog("    Sous-systeme: %s-%04X\n",vendeur,(val >> 16) & 0xFFFF);
	
	mem = carte->configRead32(PCI_CFG_EXPANSION_ROM);
	if(memoire) {
		val = 0xFFFFFFFE;
		carte->configWrite32(PCI_CFG_EXPANSION_ROM,val);
		val = carte->configRead32(PCI_CFG_EXPANSION_ROM);
		if(val != 0) carte->configWrite32(PCI_CFG_EXPANSION_ROM,mem);
	} else val = mem;
	IOLog("ROM base address: %08X, %s",mem,(mem & 1)?"active":"inactive");
	if(memoire) {
		IOLog(" (=%08X: ",val);
		k = 0xFFFFFFFF - (val & 0xFFFFF800); k++;
		if(k < KB) IOLog("%d bytes",k);
		else if(k < MB) IOLog("0x%04X bytes [%d Kb]",k,k/KB);
		else IOLog("0x%08X bytes [%d Mb]",k,k/MB);
		IOLog(")");
	}
	IOLog("\n");
	
	val = carte->configRead32(PCI_CFG_DEV_INT_LINE);
	IOLog("MaxLat: %02X    MinGnt:  %02X     ItPin:  %02X    ItLine:   %02X\n",
		  (val >> 24) & 0xFF,(val >> 16) & 0xFF,(val >> 8) & 0xFF,val & 0xFF);
	
	IOLog("\n");
	
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::PlxRegDump() {
	int reg; Ulong val;

	IOLog("ICADriver::PlxRegDump --- Registres de configuration PLX (@%08X):",PLXregs);
	if(PLXregs) for(reg=0; reg<64; reg++) {
		if(!(reg % 4)) IOLog("\n%02X:",reg * 4);
		val = PLXread(reg); /* PLXregs[reg]; */
		IOLog(" %08X",(hexa)val);
	} else IOLog(" localisation inconnue.");
	IOLog("\n");
	return(0);
}

/* ========================================================================== */
/* ACCES SPECIFIQUES A LA CARTE PCI D'ACQUISITION EDELWEISS (AB, MiG, ICA)    */
/* ========================================================================== */
static void Attente(hexa microsec) {
#ifdef TROP_BEAU
	UInt64 limite,temps,decompte;

	Microseconds(/* (UnsignedWide *) */(&temps));
	limite = temps + (UInt64)microsec; decompte = 1000000000;
	while((temps < limite) && --decompte) {
		Microseconds(/* (UnsignedWide *) */(&temps));
	}
#endif
}
/* .......................................................................... */
static INLINE void ByteSwappeInt(Ulong *val) {
	IntSwappable tempo;
	
	tempo.octet.b0 = ((IntSwappable *)val)->octet.b3;
	tempo.octet.b1 = ((IntSwappable *)val)->octet.b2;
	tempo.octet.b2 = ((IntSwappable *)val)->octet.b1;
	tempo.octet.b3 = ((IntSwappable *)val)->octet.b0;
	*val = tempo.mot;
}
/* .......................................................................... */
INLINE Ulong BigEndianInt(Ulong val) {
#ifdef PPC
	return(val);
#else
	Ulong tmp;

	tmp = val;
	ByteSwappeInt(&tmp);
	return(tmp);
#endif
}
/* ========================================================================== */
INLINE Ulong ICADriver::PLXread(int reg) {
#ifdef PPC
	Ulong val;

	val = PLXregs[reg];
	ByteSwappeInt(&val);
//	IOLog("ICADriver::PLXread(%04X) rend %08X\n",reg,val);
	return(val);
#else
	return(PLXregs[reg]);
#endif
}
/* ========================================================================== */
INLINE void ICADriver::PLXwrite(int reg, Ulong val) {
#ifdef PPC
	Ulong ecr;

	ecr = val;
	ByteSwappeInt(&ecr);
	PLXregs[reg] = ecr;
//	IOLog("ICADriver::PLXwrite(%04X,%08X)\n",reg,ecr);
#else
	PLXregs[reg] = val;
#endif
	OSSynchronizeIO();
}
/* ========================================================================== */
IOReturn ICADriver::Setup() { /* OBSOLETE */

	if(!isOpen()) return(kIOReturnNotOpen);

	IOLog("ICADriver::Setup() (appel OBSOLETE, par IcaInit dans l'application)\n");
/*	PCIadrs = SimulPCI;
	ResetAdrs = (Ulong *)((int)PCIadrs + 8);
	mapping_pci = mapping_plx = 0;
	PLXregs = 0; */
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::UtilWrite(int valeur) {
	if(!isOpen()) return(kIOReturnNotOpen);
//	IOLog("ICADriver::UtilWrite(%08X) a l'adresse %08X\n",valeur,(hexa)PCIadrs);
	*PCIadrs = valeur;
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::RegWrite(int num, int valeur) {
	if(!isOpen()) return(kIOReturnNotOpen);
	*(PCIadrs + num) = valeur;
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::RegRead(int num, int *valeur) {
	if(!isOpen()) return(kIOReturnNotOpen);
	*valeur = *(PCIadrs + num);
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::CommandSend(int adrs, int action, int ss_adrs, int valeur) {
	hexa delai_mot=1,delai_msg=3000;  /* microsecondes */

	if(!isOpen()) return(kIOReturnNotOpen);
	*PCIadrs = 0x100 | adrs;                     Attente(delai_mot);
	*PCIadrs = ((action << 6) & 0xC0) | ss_adrs; Attente(delai_mot);
	*PCIadrs = (valeur >> 8) & 0xFF;             Attente(delai_mot);
	*PCIadrs = valeur & 0xFF;
	Attente(delai_msg);
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::InstrWrite(int adrs, int instr) {
	int ss_adrs,data1,data0;
	hexa delai_mot=1,delai_msg=3000;  /* microsecondes */

	if(!isOpen()) return(kIOReturnNotOpen);
	data0 = instr & 0xFF; instr = instr >> 8;
	data1 = instr & 0xFF; instr = instr >> 8;
	ss_adrs = 0x40 | (instr & 0x3F);  /* action: differee */
	*PCIadrs = adrs;    Attente(delai_mot);
	*PCIadrs = ss_adrs; Attente(delai_mot);
	*PCIadrs = data1;   Attente(delai_mot);
	*PCIadrs = data0;
	Attente(delai_msg);
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::FifoReset() {
//	IOLog("ICADriver::FifoReset %08X <- 0\n",(hexa)ResetAdrs);
	*ResetAdrs = 0; return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::FifoEmpty(int nb_a_verifier, int *num_non_nul) {
	shex val;

    if(!isOpen()) return(kIOReturnNotOpen);
	*num_non_nul = 0;
	while(nb_a_verifier--) {
		val = *PCIadrs & 0xFFFF; /* sinon, instruction en cache... */
		if(val) *num_non_nul += 1;
	}
	return(kIOReturnSuccess);
}
/* ========================================================================== */
// IOReturn ICADriver::FifoRead(Ulong *val) { *val = *PCIadrs; return(kIOReturnSuccess); }
IOReturn ICADriver::FifoRead(Ulong *val) {
	*val = *PCIadrs;
//	IOLog("ICADriver::FifoRead lu: %08X\n",*val);
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::RegisterUse(int reg) { /* OBSOLETE */
#ifdef A_VIRER
    if(!isOpen()) return(kIOReturnNotOpen);
	acces_pci = kIOPCIConfigBaseAddress0 + (4 * reg);
	mapping_pci = carte->mapDeviceMemoryWithRegister(acces_pci);
    if(mapping_pci) {
		PCIadrs = (Ulong *)mapping_pci->getVirtualAddress();
//		PCIadrs = (Ulong *)mapping_pci->getPhysicalAddress();
		ResetAdrs = (Ulong *)((int)PCIadrs + 8);
		IOLog("ICADriver::RegisterUse(0x%02X) PCIadrs=%08X, ResetAdrs=%08X\n",reg,
			(hexa)PCIadrs,(hexa)ResetAdrs);
		return(kIOReturnSuccess);
    } else IOLog("ICADriver::RegisterUse(0x%02X) map non disponible\n",reg);
#endif
	return(kIOReturnError);
}
/* ========================================================================== */
IOReturn ICADriver::XferInit(int avec_dma, Ulong taille, Ulong *fifo) {
	IOReturn rc;
	Ulong DMAdest,DMAsize,of7,dim,val;
	
	IOLog("ICADriver::XferInit(%s DMA, FIFO @%08X + %08X)\n",
		avec_dma?"avec":"sans",(hexa)fifo,(hexa)taille);
#ifdef PPC
	IOLog("                   compile avec l'option PPC (donc en big endian)\n");
#else
	IOLog("                   compile sans l'option PPC (donc en little endian)\n");
#endif
    if(!isOpen()) {
		IOLog("ICADriver::XferInit() appele sur un driver pas ouvert\n");
		return(kIOReturnNotOpen);
	}

	if(PLXregs) {
		val = PLXread(PLX_BIGEND);
#ifdef PPC
		val |= 0xE4; /* BAddr 2+3 et DMA channels 0/1 en BigEndian */
#else
		val &= 0xFFFFFF00; /* BAddr 2+3 et DMA channels 0/1 en LittleEndian */
#endif
		PLXwrite(PLX_BIGEND,val);
	}
	mem_fifo = IOMemoryDescriptor::withAddress((void *)fifo,taille,kIODirectionOutIn);
	IOLog("ICADriver::XferInit,  mem_fifo @%08X\n",(hexa)mem_fifo);
	if(!mem_fifo) return(kIOReturnNoMemory);
	rc = mem_fifo->prepare(kIODirectionOutIn);     // rc = LockMemoryContiguous(fifo,taille);
	mem_fifo->retain();
	if(rc != kIOReturnSuccess) return(rc);
	if(avec_dma && PLXregs) {
		dma_desc = IOMemoryDescriptor::withAddress((void *)dma,DMA_MAXBLOCS * sizeof(TypeDmaBloc),
			kIODirectionOutIn);
		if(!dma_desc) return(kIOReturnNoMemory);
		rc = dma_desc->prepare(kIODirectionOutIn);     // rc = LockMemoryContiguous(dma,taille);
		dma_desc->retain();
		if(rc != kIOReturnSuccess) return(rc);
		max_lues = 0; of7 = 0; nb_blocs = 0;
		do {
			DMAdest = (Ulong)mem_fifo->getPhysicalSegment(of7,&DMAsize); // mem_fifo->getPhysicalAddress();
			if(DMAdest == 0) break;
			max_lues += (DMAsize / 4);
//			IOLog("ICADriver::XferInit,  Lecture accordee de %d mots\n",max_lues);
			if(of7 == 0) {
				PLXwrite(PLX_DMA_PADR0,DMAdest);
				PLXwrite(PLX_DMA_SIZ0,DMAsize);
				if(DMAsize == taille) {
					IOLog("ICADriver::XferInit,  Bloc DMA unique: ->%08X + %08X\n",
						  (hexa)DMAdest,(hexa)DMAsize);
					break;
				}
			}
			if(nb_blocs == 0) {
				premier_bloc = (Ulong)dma_desc->getPhysicalSegment(0,&dim);
				IOLog("ICADriver::XferInit,  Bloc DMA #%-2d @%08X\n",0,premier_bloc);
				premier_bloc &= 0xFFFFFFF0; premier_bloc |= 0x9; /* LocalBus to PciBus, PCI adrs space */
			}
			dma[nb_blocs].adrs_pci = DMAdest;       // BigEndianInt(DMAdest);
			dma[nb_blocs].adrs_loc = ICA_FIFO_ADRS; // BigEndianInt(ICA_FIFO_ADRS);
			dma[nb_blocs].taille   = DMAsize;       // BigEndianInt(DMAsize);
			val = (Ulong)dma_desc->getPhysicalSegment((nb_blocs + 1) * sizeof(TypeDmaBloc),&dim);
			IOLog("ICADriver::XferInit,  Bloc DMA #%-2d: ->%08X + %08X, suivant @%08X\n",
				  nb_blocs,(hexa)DMAdest,(hexa)DMAsize,val);
			val &= 0xFFFFFFF0; val |= 0x9; /* LocalBus to PciBus, PCI adrs space */
			dma[nb_blocs].suivant  = val;           // BigEndianInt(val);
			if(++nb_blocs >= DMA_MAXBLOCS) break;
			of7 += DMAsize;
		} while(1);
		if(max_lues == 0) return(kIOReturnNoMemory);
		if(nb_blocs) {
			val = dma[nb_blocs - 1].suivant; // BigEndianInt(dma[nb_blocs - 1].suivant);
			val |= 0xA;  /* LocalBus to PciBus, fin de chaine */
			IOLog("ICADriver::XferInit,  Bloc DMA #%-2d: suivant @%08X\n",nb_blocs-1,val);
			dma[nb_blocs - 1].suivant = val; // BigEndianInt(val);
			
			PLXwrite(PLX_DMA_DPR0,premier_bloc);
			
			val = PLXread(PLX_DMA_MODE0);
			val |= (1 << 9); /* Scatter/gather mode */
			PLXwrite(PLX_DMA_MODE0,val);
		}
		dma_ok = 1;
		PlxRegDump();

	} else {
		max_lues = taille / 4;
		mapping_fifo = mem_fifo->map();
		if(!mapping_fifo) return(kIOReturnNoMemory);
		FifoUserAdrs = (Ulong *)mapping_fifo->getVirtualAddress();
//		IOLog("ICADriver::XferInit,  fifo=%08X, FifoUserAdrs=%08X\n",
//			(hexa)fifo,(hexa)FifoUserAdrs);
	}

	nb_lues = 0;
	Empty = Half = Full = 0;
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::XferExec(Ulong taille, Ulong *fifo) {
	Ulong val;

//	XferInit(1,taille,fifo);
	if(dma_ok) {
		if(nb_blocs) {
//-			IOLog("ICADriver::XferExec,  ecrit %08X dans PLX[%02X]\n",
//-				premier_bloc,PLX_DMA_DPR0*4);
			PLXwrite(PLX_DMA_DPR0,premier_bloc);
		} //- else IOLog("ICADriver::XferExec,  PLX[%02X] inchange\n",PLX_DMA_DPR0*4);
		val = PLXread(PLX_DMA_CSR0);
		val |= 0x03;  /* DMA start */
		PLXwrite(PLX_DMA_CSR0,val);
//-		IOLog("ICADriver::XferExec,  Demande de transfert DMA\n");
//-		PlxRegDump();
	} else {
		nb_lues = 0;
		do {
			val = *PCIadrs;
//			IOLog("ICADriver::XferExec lu: %08X\n",val);
			if(CLUZEL_Empty(val)) { Empty++; break; }
//			if(!LectVideAdonf()) return; /* preventif */
			if(CLUZEL_HalfFull(val)) Half++;
			if(CLUZEL_FullFull(val)) Full++;
			*FifoUserAdrs++ = val;
		} while(++nb_lues < max_lues);
	}
//	if(mem_fifo) {
//		mem_fifo->complete(kIODirectionOutIn);
//		mem_fifo = 0;
//	}
//	if(mapping_fifo) {
//		mapping_fifo->release();
//		mapping_fifo = 0;
//	}
	
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::XferDone(int *done) {
	Ulong val;

    if(!isOpen()) return(kIOReturnNotOpen);
	if(dma_ok) {
		val = PLXread(PLX_DMA_CSR0);
		*done = (val & 0x10)? max_lues: 0;  /* PLX_DMA_CSR0<4>: DMA done */
		IOLog("ICADriver::XferDone,  PLX_DMA_CSR0=%08X (%s, a lire=%d)\n",
			val,(val & 0x10)?"fini":"en cours",max_lues);
	} else *done = nb_lues;
//	IOLog("(ICADriver::XferDone) Lecture declaree de %d mots\n",*done);
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::XferStatusGet(int *empty, int *half, int *full) {
    if(!isOpen()) return(kIOReturnNotOpen);
	*empty = Empty;
	*half = Half;
	*full = Full;
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::XferStatusReset() {
    if(!isOpen()) return(kIOReturnNotOpen);
	Empty = Half = Full = 0;
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::DmaDispo(int *dispo) {
    if(!isOpen()) return(kIOReturnNotOpen);
	*dispo = dma_ok? 1: 0;
	return(kIOReturnSuccess);
}

/* ========================================================================== */
IOMemoryDescriptor *ICADriver::PCIGetDesc(void) {
	IOMemoryDescriptor *desc;

	desc = carte->getDeviceMemoryWithRegister(acces_pci);
	if(desc) desc->retain();
	return(desc);
}

/* ========================================================================== */
/* METHODES GENERALES POUR DRIVERS PCI                                        */
/* ========================================================================== */
bool ICADriver::init(OSDictionary *dictionary) {
// #ifdef DEBUG
    IOLog("\nICADriver::init()\n");
// #endif
    if (!super::init(dictionary)) return(false);
    return(true);
}
/* ========================================================================== */
IOService *ICADriver::probe(IOService *provider, SInt32 *score) {
// #ifdef DEBUG
    IOLog("ICADriver::probe()\n");
// #endif
    IOService *res = super::probe(provider, score);
    return(res);
}
/* ========================================================================== */
bool ICADriver::start(IOService *provider) {
#ifdef DEBUG
    IOMemoryDescriptor *mem;
    IOMemoryMap *map;
#endif
	Ulong did;

    IOLog("==========================================================================\n");
    IOLog("ICADriver::start() demande\n");
    if(!super::start(provider)) return(false);
    carte = (IOPCIDevice *)provider;
//  carte->setMemoryEnable(true);
//  carte->setBusMasterEnable(true);
    carte->configWrite16((UInt8)kIOPCIConfigCommand,(UInt16)0x0017);
	PciRegDump(1);
	{
		int reg;
		for(reg=0; reg<6; reg++) {
			mapping_pci = carte->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0 + (4 * reg));
			if(mapping_pci) {
				IOLog("ICADriver::start,  BADR%d physique: %08X, virtuelle: %08X\n",reg,
					  (Ulong *)mapping_pci->getPhysicalAddress(),
					  (Ulong *)mapping_pci->getVirtualAddress());
				mapping_pci->release();
			} else IOLog("ICADriver::start,  BADR%d non mappee\n",reg);
		}
	}
	dma_ok = 0; PLXregs = 0;
	did = carte->configRead32(PCI_CFG_VENDOR_ID);
	if(did == 0x540610B5) {
		Ulong val;
		mapping_plx = carte->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
		if(mapping_plx) {
			PLXregs = (Ulong *)mapping_plx->getVirtualAddress();
			if(PLXregs) {
				val = PLXread(PLX_CNTRL);
				val |= (1 << 30); /* LRESET0# actif */
				PLXwrite(PLX_CNTRL,val);
				val &= ~(1 << 30); /* LRESET0# efface */
				PLXwrite(PLX_CNTRL,val);
				
				val = PLXread(PLX_BIGEND);
#ifdef PPC
				val |= 0xE4; /* BAddr 2+3 et DMA channels 0/1 en BigEndian */
#else
				val &= 0xFFFFFF00; /* BAddr 2+3 et DMA channels 0/1 en LittleEndian */
#endif
				PLXwrite(PLX_BIGEND,val);
				
				val = PLXread(PLX_DMA_MODE0);
				val |= (1 << 11); /* LocalBus adrs constante */
				val |= (1 << 14); /* DMA EOT# Enable         */
				val |= (1 << 15); /* Fast terminate          */
				PLXwrite(PLX_DMA_MODE0,val);
				
				PLXwrite(PLX_DMA_LADR0,ICA_FIFO_ADRS);
				
				val = PLXread(PLX_DMA_DPR0);
				val |= (1 << 3); /* LocalBus to PciBus */
				PLXwrite(PLX_DMA_DPR0,val);

//				PlxRegDump();
				val = PLXread(PLX_BIGEND);
				IOLog("ICADriver::start,  BIGEND(%02X): %08X\n",PLX_BIGEND * 4,(hexa)val);
				
				IOLog("ICADriver::start,  Rechargement de l'EEPROM\n");
				val = PLXread(PLX_CNTRL);
				val &= ~(1 << 29); /* reload efface */
				PLXwrite(PLX_CNTRL,val);
				val |= (1 << 29); /* reload Local Config Registers from EEPROM */
				PLXwrite(PLX_CNTRL,val);
//				val &= ~(1 << 29); /* reload efface */
//				PLXwrite(PLX_CNTRL,val);

				val = PLXread(PLX_BIGEND);
#ifdef PPC
				val |= 0xE4; /* BAddr 2+3 et DMA channels 0/1 en BigEndian */
#else
				val &= 0xFFFFFF00; /* BAddr 2+3 et DMA channels 0/1 en LittleEndian */
#endif
				PLXwrite(PLX_BIGEND,val);
				
				PlxRegDump();
			}
		}		
	}

	if(did == 0x434E5253) acces_pci = kIOPCIConfigBaseAddress0;
	else acces_pci = kIOPCIConfigBaseAddress2;
	mapping_pci = carte->mapDeviceMemoryWithRegister(acces_pci);
    if(mapping_pci) {
		PCIadrs = (Ulong *)mapping_pci->getVirtualAddress();
		ResetAdrs = (Ulong *)((int)PCIadrs + 8);
		IOLog("ICADriver::start,  Acces PCI @%02X (PCIadrs=%08X, ResetAdrs=%08X)\n",
			  acces_pci,(hexa)PCIadrs,(hexa)ResetAdrs);
    } else IOLog("ICADriver::start,  mapping PCI non disponible...\n");
	mem_fifo = 0;
	mapping_fifo = 0;
	nb_blocs = 0;
	
#ifdef DEBUG
    /* print all the device's memory ranges */
    for( Ulong index = 0;
         index < carte->getDeviceMemoryCount();
         index++ ) {
        mem = carte->getDeviceMemoryWithIndex(index);
        assert(mem);
        IOLog("Range[%ld] %08lx:%08lx\n", index,
              mem->getPhysicalAddress(), mem->getLength());
    }

    /* look up a range based on its config space base address register */
    mem = carte->getDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
    if(mem) IOLog("Range@0x%x %08lx:%08lx\n", kIOPCIConfigBaseAddress0,
		mem->getPhysicalAddress(), mem->getLength());

    /* map a range based on its config space base address register */
    map = carte->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
    if(map) {
        IOLog("Range@0x%x (%08lx) mapped to kernel virtual address %08x\n",
                kIOPCIConfigBaseAddress0,
                map->getPhysicalAddress(),
                map->getVirtualAddress());
        /* release the map object, and the mapping itself */
        map->release();
    }

    /* read a config space register */
    IOLog("Config register@0x%x = %08lx\n", kIOPCIConfigCommand,
          carte->configRead32(kIOPCIConfigCommand) );
#endif

    /* publish ourselves so clients can find us */
    registerService();
    IOLog("ICADriver::start() - termine\n");
    return(true);
}
/* ========================================================================== */
void ICADriver::stop(IOService *provider) {
// #ifdef DEBUG
    IOLog("ICADriver::stop()\n");
// #endif
	if(mapping_plx) {
		mapping_plx->release();
		mapping_plx = 0;
	}
	if(mapping_pci) {
		mapping_pci->release();
		mapping_pci = 0;
	}
	if(mapping_fifo) {
		mapping_fifo->release();
		mapping_fifo = 0;
	}
	if(mem_fifo) {
		mem_fifo->complete(kIODirectionOutIn);
		mem_fifo = 0;
	}
    super::stop(provider);
}
/* ========================================================================== */
void ICADriver::free(void) {
// #ifdef DEBUG
    IOLog("ICADriver::free()\n");
// #endif
    super::free();
}
