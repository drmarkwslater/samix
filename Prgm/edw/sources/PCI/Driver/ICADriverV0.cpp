#include <IOKit/IOLib.h>
#define AVEC_MICROSECONDS
#ifdef AVEC_TICKCOUNT
Ulong TickCount(void);
#endif
#ifdef AVEC_MICROSECONDS
void Microseconds(UInt64 *temps);
#endif
#include <ICADriver.h>
#include <plx.h>
#define VAR
#define MAINFILE
#include <cmdelist.h>

typedef union {
	struct {
		byte b0,b1,b2,b3;
	} octet;
	Ulong mot;
} IntSwappable;

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
/* ACCES SPECIFIQUES A LA CARTE PCI D'ACQUISITION EDELWEISS (AB, MiG, ICA)    */
/* ========================================================================== */
static void IcaAttends(int microsec) {
#ifdef AVEC_TICKCOUNT
	Ulong temps,delai,decompte;

	temps = TickCount(); /* 1 tick = 1/60 seconde */
	delai = microsec * 60; if(delai < 1000000) delai = 1; else delai /= 1000000;
	temps += delai; decompte = 1000000000;
	while((TickCount() < temps) && decompte) decompte--;
#endif
#ifdef AVEC_MICROSECONDS
	UInt64 limite,temps,decompte;

	Microseconds(/* (UnsignedWide *) */(&temps));
	limite = temps + (UInt64)microsec; decompte = 1000000000;
	while((temps < limite) && --decompte) {
		Microseconds(/* (UnsignedWide *) */(&temps));
	}
#endif
}
/* .......................................................................... */
static void ByteSwappeInt(Ulong *val) {
	IntSwappable tempo;
	
	tempo.octet.b0 = ((IntSwappable *)val)->octet.b3;
	tempo.octet.b1 = ((IntSwappable *)val)->octet.b2;
	tempo.octet.b2 = ((IntSwappable *)val)->octet.b1;
	tempo.octet.b3 = ((IntSwappable *)val)->octet.b0;
	*val = tempo.mot;
}
/* ========================================================================== */
IOReturn ICADriver::Setup() {
    if(!isOpen()) return(kIOReturnNotOpen);
	PCIadrs = SimulPCI;
	ResetAdrs = (Ulong *)((int)PCIadrs + 8);
	PLXregs = 0;
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::AdrsUse(int reg) {
	IOMemoryMap *map;

    if(!isOpen()) return(kIOReturnNotOpen);
	map = carte->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0 + (4 * reg));
    if(map) {
		IOLog("ICADriver::AdrsUse(%d) -> adresse physique=%08X\n",reg,(Ulong)map->getPhysicalAddress());
//		PCIadrs = (Ulong *)map->getVirtualAddress();
		PCIadrs = (Ulong *)map->getPhysicalAddress();
		ResetAdrs = (Ulong *)((int)PCIadrs + 8);
		map->release();
		return(kIOReturnSuccess);
    } else IOLog("ICADriver::AdrsUse(0x%X) -> map non disponible\n",reg);
	return(kIOReturnSuccess);  /* trouver plutot kIOReturnFailed... */
}
/* ========================================================================== */
IOReturn ICADriver::CommandSend(int num, int adrs, int action, int valeur) {
	int DelaiMot=1,DelaiMsg=3000;

	if(!isOpen()) return(kIOReturnNotOpen);
	*PCIadrs = 0x100 | adrs;                                  IcaAttends(DelaiMot);
	*PCIadrs = ((action << 6) & 0xC0) | CmdeDef[num].ss_adrs; IcaAttends(DelaiMot);
	*PCIadrs = (valeur >> 8) & 0xFF;                          IcaAttends(DelaiMot);
	*PCIadrs = valeur & 0xFF;
	IcaAttends(DelaiMsg);
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::InstrWrite(int adrs, int instr) {
	int ss_adrs,data1,data0;
	int DelaiMot=1,DelaiMsg=3000;

	if(!isOpen()) return(kIOReturnNotOpen);
	data0 = instr & 0xFF; instr = instr >> 8;
	data1 = instr & 0xFF; instr = instr >> 8;
	ss_adrs = ((CLUZEL_DIFFERE << 6) & 0xC0) | (instr & 0x3F);
	*PCIadrs = adrs;    IcaAttends(DelaiMot);
	*PCIadrs = ss_adrs; IcaAttends(DelaiMot);
	*PCIadrs = data1;   IcaAttends(DelaiMot);
	*PCIadrs = data0;
	IcaAttends(DelaiMsg);
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::FifoReset() { *ResetAdrs = 0; return(kIOReturnSuccess); }
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
IOReturn ICADriver::FifoRead(Ulong *val) { *val = *PCIadrs; return(kIOReturnSuccess); }
/* ========================================================================== */
IOReturn ICADriver::XferInit(int fifo, int taille, int avec_dma) {
	int rc; Ulong DMAdest,DMAsize;

    if(!isOpen()) return(kIOReturnNotOpen);
	adrs_lues = (Ulong *)fifo;
    carte->configWrite16((UInt8)kIOPCIConfigCommand,(UInt16)0x0017);
	if(avec_dma) {
//		rc = LockMemoryContiguous(fifo,taille);
		IOMemoryDescriptor *mem;
		IOMemoryMap *map;
		Ulong val;
//		LogicalToPhysicalTable adrs; unsigned long nb;
	
//		adrs.logical.address = fifo;
//		adrs.logical.count = taille;
//		nb = 8;
//		rc = GetPhysical(&adrs,&nb);
//			DMAdest = (Ulong)adrs.physical[0].address;
//			DMAsize = adrs.physical[0].count;
//#define A_REGLER
#ifdef A_REGLER
		mem->initWithAddress((void *)adrs_lues,taille,kIODirectionOutIn);	
		if(mem) {
			mem->prepare(kIODirectionOutIn);
			DMAdest = (Ulong)mem->getPhysicalAddress();
#else
			DMAdest = (Ulong)PCIadrs;
#endif
			DMAsize = (Ulong)taille;
			max_lues = DMAsize;
			map = carte->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
			if(map) {
//				PLXregs = PciDeviceAdrs(board,0);
//				PLXregs = (Ulong *)map->getVirtualAddress();
				PLXregs = (Ulong *)map->getPhysicalAddress();
//				IOLog("ICADriver::adresses  PLX=%08X\n",(Ulong)PLXregs);
				map->release();

				val = PLXregs[PLX_BIGEND];
				ByteSwappeInt(&val);
				val |= 0xE4; /* BAddr 2+3 et DMA channels 0/1 en BigEndian */
				ByteSwappeInt(&val);
				PLXregs[PLX_BIGEND] = val;

				val = PLXregs[PLX_DMA_MODE0];
				ByteSwappeInt(&val);
				val |= (1 << 11); /* LocalBus adrs constante */
				ByteSwappeInt(&val);
				PLXregs[PLX_DMA_MODE0] = val;
	
				val = DMAdest;
				ByteSwappeInt(&val);
				PLXregs[PLX_DMA_PADR0] = val;
	
				val = 0x50000000;
				ByteSwappeInt(&val);
				PLXregs[PLX_DMA_LADR0] = val;
	
				val = DMAsize;
				ByteSwappeInt(&val);
				PLXregs[PLX_DMA_SIZ0] = val;
	
				val = PLXregs[PLX_DMA_DPR0];
				ByteSwappeInt(&val);
				val |= (1 << 3); /* LocalBus to PciBus */
				ByteSwappeInt(&val);
				PLXregs[PLX_DMA_DPR0] = val;

				PLXregs = 0;

			}
#ifdef A_REGLER
		}
#endif
	}
	if(PLXregs == 0) max_lues = taille;
	nb_lues = 0;
	Empty = Half = Full = 0;
	return(rc);
}
/* ========================================================================== */
IOReturn ICADriver::DmaDispo(int *dispo) {
    if(!isOpen()) return(kIOReturnNotOpen);
	*dispo = (PLXregs == 0)? 0: 1;
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::XferExec() {
	Ulong val;

    if(!isOpen()) return(kIOReturnNotOpen);
	if(PLXregs) {
		val = PLXregs[PLX_DMA_CSR0];
		ByteSwappeInt(&val);
		val |= 0x03;  /* DMA start */
		ByteSwappeInt(&val);
		PLXregs[PLX_DMA_CSR0] = val;
	} else do {
		val = *PCIadrs;
		if(CLUZEL_Empty(val)) { Empty++; break; }
//		if(!LectVideAdonf()) return; /* preventif */
		if(CLUZEL_HalfFull(val)) Half++;
		if(CLUZEL_FullFull(val)) Full++;
		adrs_lues[nb_lues] = val;
		nb_lues += 1;
	} while(nb_lues < max_lues);
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICADriver::XferDone(int *done) {
	Ulong val;

    if(!isOpen()) return(kIOReturnNotOpen);
	if(PLXregs) {
		val = PLXregs[PLX_DMA_CSR0];
		ByteSwappeInt(&val);
		*done = (val & 0x10)? max_lues: 0;  /* PLX_DMA_CSR0<4>: DMA done */
	} else *done = nb_lues;
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
/* METHODES GENERALES POUR DRIVERS PCI                                        */
/* ========================================================================== */
bool ICADriver::start(IOService *provider) {
#ifdef DEBUG
    IOMemoryDescriptor *mem;
    IOMemoryMap *map;
#endif

    IOLog("ICADriver::start()\n");
    if(!super::start(provider)) return(false);
    carte = (IOPCIDevice *)provider;
    carte->setMemoryEnable(true);
    carte->setBusMasterEnable(true);

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
    return(true);
}
/* ========================================================================== */
void ICADriver::stop(IOService *provider) {
// #ifdef DEBUG
    IOLog("ICADriver::stop()\n");
// #endif
    super::stop(provider);
}
/* ========================================================================== */
bool ICADriver::init(OSDictionary *dictionary) {
#ifdef DEBUG
    IOLog("\nICADriver::init()\n");
#endif
    if (!super::init(dictionary)) return(false);
    return(true);
}
/* ========================================================================== */
void ICADriver::free(void) {
#ifdef DEBUG
    IOLog("ICADriver::free()\n");
#endif
    super::free();
}
/* ========================================================================== */
IOService *ICADriver::probe(IOService *provider, SInt32 *score) {
#ifdef DEBUG
    IOLog("ICADriver::probe()\n");
#endif
    IOService *res = super::probe(provider, score);
    return(res);
}
