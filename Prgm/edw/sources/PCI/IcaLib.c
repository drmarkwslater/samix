#include <environnement.h>

#include <stdlib.h>

#ifdef MACOSX
	#ifndef ATTENTE_AVEC_USLEEP
		#include <CoreServices/CoreServices.h> // pour eviter le warning: implicit declaration of function `TickCount'
	#endif
	#include <IOKit/IOKitLib.h>
#endif
#include "IcaDefine.h"
#include "IcaLib.h"

#ifndef macintosh
	#include <unistd.h>
#endif

#ifdef SANS_PCI
/* ========================================================================== */
// typedef hexa kern_return_t;
typedef hexa task_port_t;
// typedef unsigned long vm_address_t;
// typedef unsigned long vm_size_t;
// typedef hexa IOOptionBits;
// typedef hexa IOItemCount;

#ifdef BITS64
	kern_return_t IOConnectMapMemory(io_connect_t connect, uint32_t memoryType, task_port_t intoTask,
		mach_vm_address_t *atAddress, mach_vm_size_t *ofSize, IOOptionBits options);
#else  /* !BITS64 */
	kern_return_t IOConnectMapMemory(io_connect_t connect, hexa	memoryType,task_port_t intoTask,
		vm_address_t *atAddress, vm_size_t *ofSize,IOOptionBits options ) { return((kern_return_t)0); }
	kern_return_t IOConnectMethodScalarIScalarO(io_connect_t connect, hexa index,
		IOItemCount	scalarInputCount, IOItemCount scalarOutputCount,... ) { return((kern_return_t)0); }
#endif /* !BITS64 */
/* ========================================================================== */
IcaReturnCode IcaDerniereErreur() { return(0); }
/* ========================================================================== */
IcaReturnCode IcaConfigReadLong(io_connect_t *dataPort, int adrs, int *val) {
	return(0);
}
/* ========================================================================== */
IcaReturnCode IcaConfigWriteLong(io_connect_t *dataPort, int adrs, int val) {
	return(0);
}
/* ========================================================================== */
IcaReturnCode IcaConfigReadWord(io_connect_t *dataPort, int adrs, short *val) {
	return(0);
}
/* ========================================================================== */
IcaReturnCode IcaConfigWriteWord(io_connect_t *dataPort, int adrs, int val) {
	return(0);
}
/* ========================================================================== */
IcaReturnCode IcaConfigReadByte(io_connect_t *dataPort, int adrs, char *val) {
	return(0);
}
/* ========================================================================== */
IcaReturnCode IcaConfigWriteByte(io_connect_t *dataPort, int adrs, int val) {
	return(0);
}

/* ========================================================================== */
/* ACCES SPECIFIQUES A LA CARTE PCI D'ACQUISITION EDELWEISS (AB, MiG, ICA)    */
/* ========================================================================== */
void IcaInit(PciBoard board) { }
/* ========================================================================== */
void IcaRegisterUse(PciBoard board, int reg) { }
/* ========================================================================== */
IcaReturnCode IcaAdrsGet(PciBoard board, hexa type, Ulong **adrs, int *taille) {
	return(0);
}
/* ========================================================================== */
void IcaUtilWrite(PciBoard board, Ulong valeur) { }
/* ========================================================================== */
void IcaRegWrite(PciBoard board, int num, Ulong valeur) { }
/* ========================================================================== */
Ulong IcaRegRead(PciBoard board, int num) {
	return(0);
}
/* ========================================================================== */
IcaReturnCode IcaCommandSend(PciBoard board, int adrs, int action, int ss_adrs, int valeur) {
	return(0);
}
/* ========================================================================== */
IcaReturnCode IcaInstrWrite(PciBoard board, int adrs, int instr) {
	return(0);
}
/* ========================================================================== */
IcaReturnCode IcaFifoReset(PciBoard board) {
	return(0);
}
/* ========================================================================== */
int IcaFifoEmpty(PciBoard board, int nb_a_verifier) {
	return(0);
}
/* ========================================================================== */
hexa IcaFifoRead(PciBoard board) {
	return(0);
}
/* ========================================================================== */
IcaReturnCode IcaXferInit(PciBoard board, char avec_dma) {
	return(0);
}
/* ========================================================================== */
IcaReturnCode IcaXferSizeSet(PciBoard board, int taille) {
	return(0);
}
/* ========================================================================== */
IcaReturnCode IcaXferExec(PciBoard board) {
	return(0);
}
/* ========================================================================== */
int IcaXferDone(PciBoard board) {
	return(0);
}
/* ========================================================================== */
IcaReturnCode IcaXferStatusGet(PciBoard board, int *empty, int *half, int *full) {
	return(0);
}
/* ========================================================================== */
IcaReturnCode IcaXferStatusReset(PciBoard board) {
	return(0);
}
/* ========================================================================== */
char IcaDmaDispo(PciBoard board) {
	return(0);
}
#else /* !SANS_PCI */

static int DelaiMot=1,DelaiMsg=3000;  /* microsecondes */
/* .......................................................................... */
static void IcaAttends(int microsec) {
#ifdef ATTENTE_AVEC_USLEEP
	usleep(microsec);
#else
	int temps,delai,decompte;

	temps = TickCount(); /* 1 tick = 1/60 seconde */
	delai = microsec * 60; if(delai < 1000000) delai = 1; else delai /= 1000000;
	temps += delai; decompte = 1000000000;
	while((TickCount() < temps) && decompte) decompte--;
#endif
}
#ifdef MACOSX
/* ========================================================================== */
/* ACCES PCI STANDARD
   ..................

	Parametrage de IOConnectMethod<type-input>I<type-output>O :
		io_connect_t   dataPort : an io_connect_t returned from IOServiceOpen()
		hexa   index    : an index to the function in the Kernel
		<type>=Scalar:
			IOItemCount scalarCount : the number of scalar values (input si I, output si O)
		<type>=Structure:
			IOByteCount  structureSize : the size of the input structure si I
			IOByteCount *structureSize : the size of the output structure si O
		int             inputScalarValue si I
		int            *outputScalarPointer si O
		void *          structurePointer (I et O)

	Combinaisons permises:
		ScalarIScalarO
		ScalarIStructureO
		ScalarIStructureI
		StructureIStructureO
*/

static IcaReturnCode ErreurDriver = KERN_SUCCESS;

/* ========================================================================== */
IcaReturnCode IcaDerniereErreur() { return(ErreurDriver); }
/* ========================================================================== */
IcaReturnCode IcaConfigReadLong(io_connect_t *dataPort, int adrs, int *val) {
    ErreurDriver = IOConnectMethodScalarIScalarO(*dataPort,ICA_CONFIG_READ32,
		1,1,       /* nb of scalar input+output values */
		adrs,val); /* scalar input+output parameters   */
	return((ErreurDriver != kIOReturnSuccess));
}
/* ========================================================================== */
IcaReturnCode IcaConfigWriteLong(io_connect_t *dataPort, int adrs, int val) {
    ErreurDriver = IOConnectMethodScalarIScalarO(*dataPort,ICA_CONFIG_WRITE32,
		2,0,       /* nb of scalar input+output values */
		adrs,val); /* scalar input parameters          */
	return((ErreurDriver != kIOReturnSuccess));
}
/* ========================================================================== */
IcaReturnCode IcaConfigReadWord(io_connect_t *dataPort, int adrs, short *val) {
    ErreurDriver = IOConnectMethodScalarIScalarO(*dataPort,ICA_CONFIG_READ16,
		1,1,       /* nb of scalar input+output values */
		adrs,val); /* scalar input+output parameters   */
	return((ErreurDriver != kIOReturnSuccess));
}
/* ========================================================================== */
IcaReturnCode IcaConfigWriteWord(io_connect_t *dataPort, int adrs, int val) {
    ErreurDriver = IOConnectMethodScalarIScalarO(*dataPort,ICA_CONFIG_WRITE16,
		2,0,       /* nb of scalar input+output values */
		adrs,val); /* scalar input parameters          */
	return((ErreurDriver != kIOReturnSuccess));
}
/* ========================================================================== */
IcaReturnCode IcaConfigReadByte(io_connect_t *dataPort, int adrs, char *val) {
    ErreurDriver = IOConnectMethodScalarIScalarO(*dataPort,ICA_CONFIG_READ8,
		1,1,       /* nb of scalar input+output values */
		adrs,val); /* scalar input+output parameters   */
	return((ErreurDriver != kIOReturnSuccess));
}
/* ========================================================================== */
IcaReturnCode IcaConfigWriteByte(io_connect_t *dataPort, int adrs, int val) {
    ErreurDriver = IOConnectMethodScalarIScalarO(*dataPort,ICA_CONFIG_WRITE8,
		2,0,       /* nb of scalar input+output values */
		adrs,val); /* scalar input parameters          */
	return((ErreurDriver != kIOReturnSuccess));
}

/* ========================================================================== */
/* ACCES SPECIFIQUES A LA CARTE PCI D'ACQUISITION EDELWEISS (AB, MiG, ICA)    */
/* ========================================================================== */
void IcaInit(PciBoard board) {
    IOConnectMethodScalarIScalarO(*board,ICA_INIT,0,0);
}
/* ========================================================================== */
void IcaRegisterUse(PciBoard board, int reg) {
    IOConnectMethodScalarIScalarO(*board,ICA_REGISTER_USE,1,0,reg);
}
/* ========================================================================== */
IcaReturnCode IcaAdrsGet(PciBoard board, hexa type, Ulong **adrs, int *taille) {
    return(IOConnectMapMemory(*board,type,mach_task_self(),
		(vm_address_t *)adrs,(hexa *)taille,kIOMapAnywhere));
}
/* ========================================================================== */
void IcaUtilWrite(PciBoard board, Ulong valeur) {
    IOConnectMethodScalarIScalarO(*board,ICA_UTIL_WRITE,1,0,valeur);
}
/* ========================================================================== */
void IcaRegWrite(PciBoard board, int num, Ulong valeur) {
    IOConnectMethodScalarIScalarO(*board,ICA_REG_WRITE,2,0,num,valeur);
}
/* ========================================================================== */
Ulong IcaRegRead(PciBoard board, int num) {
	Ulong valeur;

    ErreurDriver = IOConnectMethodScalarIScalarO(*board,ICA_REG_READ,1,1,num,&valeur);
	if(ErreurDriver == KERN_SUCCESS) return(valeur); else return(0);
}
/* ========================================================================== */
IcaReturnCode IcaCommandSend(PciBoard board, int adrs, int action, int ss_adrs, int valeur) {
/*	a utiliser quand on saura mettre un delai dans le driver:
    	return(IOConnectMethodScalarIScalarO(*board,ICA_COMMAND_SEND,4,0,
		adrs,action,ss_adrs,valeur));
*/
	IOConnectMethodScalarIScalarO(*board,ICA_UTIL_WRITE,1,0,0x100 | adrs);                     IcaAttends(DelaiMot);
	IOConnectMethodScalarIScalarO(*board,ICA_UTIL_WRITE,1,0,((action << 6) & 0xC0) | ss_adrs); IcaAttends(DelaiMot);
	IOConnectMethodScalarIScalarO(*board,ICA_UTIL_WRITE,1,0,(valeur >> 8) & 0xFF);             IcaAttends(DelaiMot);
	IOConnectMethodScalarIScalarO(*board,ICA_UTIL_WRITE,1,0,valeur & 0xFF);
	IcaAttends(DelaiMsg);
	return(0);
}
/* ========================================================================== */
IcaReturnCode IcaInstrWrite(PciBoard board, int adrs, int instr) {
/*	a utiliser quand on saura mettre un delai dans le driver:
        return(IOConnectMethodScalarIScalarO(*board,ICA_INSTR_WRITE,2,0,
		adrs,instr));
*/
	int ss_adrs,data1,data0;

	data0 = instr & 0xFF; instr = instr >> 8;
	data1 = instr & 0xFF; instr = instr >> 8;
	ss_adrs = 0x40 | (instr & 0x3F);  /* 0x40: CLUZEL_DIFFERE */
	IOConnectMethodScalarIScalarO(*board,ICA_UTIL_WRITE,1,0,adrs);    IcaAttends(DelaiMot);
	IOConnectMethodScalarIScalarO(*board,ICA_UTIL_WRITE,1,0,ss_adrs); IcaAttends(DelaiMot);
	IOConnectMethodScalarIScalarO(*board,ICA_UTIL_WRITE,1,0,data1);   IcaAttends(DelaiMot);
	IOConnectMethodScalarIScalarO(*board,ICA_UTIL_WRITE,1,0,data0);
	IcaAttends(DelaiMsg);
	return(0);
}
/* ========================================================================== */
IcaReturnCode IcaFifoReset(PciBoard board) {
    return(IOConnectMethodScalarIScalarO(*board,ICA_FIFO_RESET,0,0));
}
/* ========================================================================== */
int IcaFifoEmpty(PciBoard board, int nb_a_verifier) {
	int num_non_nul;
    
    ErreurDriver = IOConnectMethodScalarIScalarO(*board,ICA_FIFO_EMPTY,1,1,
		nb_a_verifier,&num_non_nul);
	if(ErreurDriver == KERN_SUCCESS) return(num_non_nul); else return(-1);
}
/* ========================================================================== */
hexa IcaFifoRead(PciBoard board) {
 	hexa val;
    
    ErreurDriver = IOConnectMethodScalarIScalarO(*board,ICA_FIFO_READ,0,1,&val);
	if(ErreurDriver == KERN_SUCCESS) return(val); else return(0);
}
/* ========================================================================== */
IcaReturnCode IcaXferInit(PciBoard board, char avec_dma) {
    return(IOConnectMethodScalarIScalarO(*board,ICA_XFER_INIT,1,0,
		(int)avec_dma));
}
/* ========================================================================== */
IcaReturnCode IcaXferSizeSet(PciBoard board, int taille) {
    return(IOConnectMethodScalarIScalarO(*board,ICA_XFER_SIZE_SET,1,0,
		taille));
}
/* ========================================================================== */
IcaReturnCode IcaXferExec(PciBoard board) {
    return(IOConnectMethodScalarIScalarO(*board,ICA_XFER_EXEC,0,0));
}
/* ========================================================================== */
int IcaXferDone(PciBoard board) {
	int done;
    
    ErreurDriver = IOConnectMethodScalarIScalarO(*board,ICA_XFER_DONE,0,1,&done);
	// printf("(%s) retourne l'erreur %d pour %d donnees\n",__func__,ErreurDriver,done);
	if(ErreurDriver == KERN_SUCCESS) return(done); else return(-1);
}
/* ========================================================================== */
IcaReturnCode IcaXferStatusGet(PciBoard board, int *empty, int *half, int *full) {
    return(IOConnectMethodScalarIScalarO(*board,ICA_XFER_STATUS_GET,0,3,
		empty,half,full));
}
/* ========================================================================== */
IcaReturnCode IcaXferStatusReset(PciBoard board) {
    return(IOConnectMethodScalarIScalarO(*board,ICA_XFER_STATUS_RESET,0,0));
}
/* ========================================================================== */
char IcaDmaDispo(PciBoard board) {
	int avec_dma;
    
    ErreurDriver = IOConnectMethodScalarIScalarO(*board,ICA_DMA_DISPO,0,1,&avec_dma);
	if(ErreurDriver == KERN_SUCCESS) return((char)avec_dma); else return(0);
}

#else /* !MACOSX: MacOS 8.6 ou 9.x, voire linux */
#ifdef macintosh
	#include <Memory.h>
#endif /* macintosh */
#include <PciLib.h> /* pour PciDeviceAdrs */
#include <plx.h>

#define CLUZEL_NOT_FULLFIFO 0x00400000
#define CLUZEL_NOT_HALFFIFO 0x00200000
#define CLUZEL_NOT_EMPTY    0x00100000
#define CLUZEL_WASFULL      0x00080000
#define CLUZEL_FullFull(val) (((val) & CLUZEL_NOT_FULLFIFO) == 0)
#define CLUZEL_HalfFull(val) (((val) & CLUZEL_NOT_HALFFIFO) == 0)
/* #define CLUZEL_Empty(val) (((val) & CLUZEL_NOT_EMPTY) == 0) */
#define CLUZEL_Empty(val) (((val) & 0xFFFF) == 0)  /* Plus sur pour le moment?? */

typedef union {
	struct {
		byte b0,b1,b2,b3;
	} octet;
	hexa mot;
} IntSwappable;

static Ulong SimulPCI[4];
static IcaReturnCode ErreurDriver = 0;

/* .......................................................................... */
static void ByteSwappeInt(hexa *tab) {
	IntSwappable tempo;
	
	tempo.octet.b0 = ((IntSwappable *)tab)->octet.b3;
	tempo.octet.b1 = ((IntSwappable *)tab)->octet.b2;
	tempo.octet.b2 = ((IntSwappable *)tab)->octet.b1;
	tempo.octet.b3 = ((IntSwappable *)tab)->octet.b0;
	*tab = tempo.mot;
}
/* .......................................................................... */
static Ulong PLXread(PciBoard board, int reg) {
	Ulong val;
	val = board->PLXregs[reg]; ByteSwappeInt(&val);
	return(val);
}
/* .......................................................................... */
static void PLXwrite(PciBoard board, int reg, Ulong val) {
	Ulong ecr;
	ecr = val; ByteSwappeInt(&ecr); board->PLXregs[reg] = ecr;
}
/* ========================================================================== */
IcaReturnCode IcaDerniereErreur() { return(ErreurDriver); }
/* ========================================================================== */
void IcaInit(PciBoard board) {
	Ulong taille;

	board->PCIadrs = SimulPCI;
	board->ResetAdrs = (Ulong *)((int)(board->PCIadrs) + 8);
	board->PLXregs = 0;
	taille = CLUZEL_FIFO_INTERNE;
//	printf("  (IcaInit) Taille prevue: %08X\n",taille);
	do board->fifo = (Ulong *)malloc(taille); while(!board->fifo && (taille /= 2));
	board->taille = taille;
//	printf("  (IcaInit) FIFO interne a l'adresse %08X + %08X\n",board->fifo,board->taille);
}
/* ========================================================================== */
void IcaRegisterUse(PciBoard board, int reg) {
	board->PCIadrs = PciDeviceAdrs(board,reg);
//	printf("  (IcaRegisterUse) Acces PCI a l'adresse 0x%08X\n",board->PCIadrs);
	board->ResetAdrs = (Ulong *)((int)(board->PCIadrs) + 8);
}
/* ========================================================================== */
IcaReturnCode IcaAdrsGet(PciBoard board, hexa type, Ulong **adrs, int *taille) {
	switch(type) {
	  case ICA_MEM_INTERNE:
		*adrs = board->fifo;
		*taille = board->taille;
		break;
	  case ICA_ACCES_PCI:
		*adrs = board->PCIadrs;
		*taille = 0x20; /* en realite, 0x10000 */
		break;
	}
    return(0);
}
/* ========================================================================== */
void IcaRegWrite(PciBoard board, int num, Ulong valeur) { *(board->PCIadrs + num) = valeur; }
/* ========================================================================== */
Ulong IcaRegRead(PciBoard board, int num) { return(*(board->PCIadrs + num)); }
/* ========================================================================== */
void IcaUtilWrite(PciBoard board, Ulong valeur) { *(board->PCIadrs) = valeur; }
/* ========================================================================== */
IcaReturnCode IcaCommandSend(PciBoard board, byte adrs, byte action, byte ss_adrs, shex valeur) {
	*(board->PCIadrs) = 0x100 | adrs;                     IcaAttends(DelaiMot);
	*(board->PCIadrs) = ((action << 6) & 0xC0) | ss_adrs; IcaAttends(DelaiMot);
	*(board->PCIadrs) = (valeur >> 8) & 0xFF;             IcaAttends(DelaiMot);
	*(board->PCIadrs) = valeur & 0xFF;
	IcaAttends(DelaiMsg);
//	printf("(IcaCommandSend) envoye @%08X: %X.%02X.%04X\n",PCIadrs,0x100 | adrs,
//		((action << 6) & 0xC0) | ss_adrs,valeur & 0xFFFF);
	return(0);
}
/* ========================================================================== */
IcaReturnCode IcaInstrWrite(PciBoard board, int adrs, int instr) {
	int ss_adrs,data1,data0;

	data0 = instr & 0xFF; instr = instr >> 8;
	data1 = instr & 0xFF; instr = instr >> 8;
	ss_adrs = 0x40 | (instr & 0x3F);  /* 0x40: CLUZEL_DIFFERE */
	*(board->PCIadrs) = adrs;    IcaAttends(DelaiMot);
	*(board->PCIadrs) = ss_adrs; IcaAttends(DelaiMot);
	*(board->PCIadrs) = data1;   IcaAttends(DelaiMot);
	*(board->PCIadrs) = data0;
	IcaAttends(DelaiMsg);
//	printf("(IcaInstrWrite) envoye @%08X: %X.%02X.%02X%02X\n",
//		PCIadrs,0x100 | adrs,ss_adrs,data1,data0);
	return(0);
}
/* ========================================================================== */
IcaReturnCode IcaFifoReset(PciBoard board) { *(board->ResetAdrs) = 0; return(1); }
/* ========================================================================== */
int IcaFifoEmpty(PciBoard board, int nb_a_verifier) {
	int num_non_nul; shex val;

	num_non_nul = 0;
	while(nb_a_verifier--) {
		val = *(board->PCIadrs) & 0xFFFF; /* sinon, instruction en cache... */
		if(val) num_non_nul++;
	}
	return(num_non_nul);
}
/* ========================================================================== */
hexa IcaFifoRead(PciBoard board) { return(*(board->PCIadrs)); }
/* ========================================================================== */
IcaReturnCode IcaXferInit(PciBoard board, char avec_dma) {
#ifdef macintosh
#ifndef CARBON
	Ulong val;
	IcaReturnCode rc;
	LogicalToPhysicalTable adrs;
	int nb;
	Ulong DMAdest,DMAsize;
#endif
#endif

#ifndef SANS_PCI
	PciConfigWriteWord(board,PCI_CFG_COMMAND,(UInt16)0x0017);
#endif
	if(avec_dma) {
	#ifdef macintosh
	#ifndef CARBON
		rc = LockMemoryContiguous(board->fifo,board->taille);
		if(rc) printf("  La FIFO interne [%d octets] ne peut etre placee en RAM:\n%s\nDMA impossible\n",
			board->taille,strerror(rc));
		else {
			adrs.logical.address = board->fifo;
			adrs.logical.count = board->taille;
			nb = 8;
			rc = GetPhysical(&adrs,&nb);
			if(rc) printf("  L'adresse physique de la FIFO interne ne peut etre connue:\n%s\nDMA impossible\n",
				strerror(rc));
			else if(board->PLXregs = PciDeviceAdrs(board,0)) {
				DMAdest = (Ulong)adrs.physical[0].address;
				DMAsize = adrs.physical[0].count;
				board->max_lues = DMAsize / 4;
				printf("  FIFO interne physique (1/%d): @%08X + %X\n",nb,DMAdest,DMAsize);

			#ifdef RESET_ICA
				val = PLXread(board,PLX_CNTRL);
				val |= (1 << 30);  /* LRESET0# actif  */
				PLXwrite(board,PLX_CNTRL,val);
				val &= ~(1 << 30); /* LRESET0# efface */
				PLXwrite(board,PLX_CNTRL,val);
			#endif

				PLXwrite(board,PLX_DMA_PADR0,DMAdest);

				PLXwrite(board,PLX_DMA_SIZ0,DMAsize);

				val = PLXread(board,PLX_BIGEND);
				val |= 0xE4; /* BAddr 2+3 et DMA channels 0/1 en BigEndian */
				PLXwrite(board,PLX_BIGEND,val);
		
				val = PLXread(board,PLX_DMA_MODE0);
				val |= (1 << 11); /* LocalBus adrs constante */
				val |= (1 << 14); /* DMA EOT enable          */
				val |= (1 << 15); /* Fast terminate          */
				PLXwrite(board,PLX_DMA_MODE0,val);
		
				PLXwrite(board,PLX_DMA_LADR0,ICA_FIFO_ADRS);
		
				val = PLXread(board,PLX_DMA_DPR0);
				val |= (1 << 3); /* LocalBus to PciBus */
				PLXwrite(board,PLX_DMA_DPR0,val);
			} else printf("  Les registres locaux du PLX sont inaccessibles\n"); 
		}
	#else  /* CARBON */
		#pragma message("Pas de DMA avec CARBON!")
	#endif /* CARBON */
	#endif /* macintosh */
	}
	if(board->PLXregs == 0) board->max_lues = board->taille / 4;
	board->nb_lues = 0;
	board->Empty = board->Half = board->Full = 0;
	return(0);
}
/* ========================================================================== */
IcaReturnCode IcaXferSizeSet(PciBoard board, int taille) {
	if(taille == 0) taille = board->taille;
	else if(taille > board->taille) return(1);
	board->max_lues = taille / 4;
	if(board->PLXregs) PLXwrite(board,PLX_DMA_SIZ0,taille);
	return(0);
}
/* ========================================================================== */
IcaReturnCode IcaXferExec(PciBoard board) {
    IcaReturnCode rc;
	Ulong val;
	Ulong *user;

	rc = 0;
	if(board->PLXregs) {
		val = PLXread(board,PLX_DMA_CSR0);
		val |= 0x03;  /* DMA start */
		PLXwrite(board,PLX_DMA_CSR0,val);
	} else {
		board->nb_lues = 0; user = board->fifo;
		do {
			val = *(board->PCIadrs);
			if(CLUZEL_Empty(val)) { board->Empty++; break; }
//			if(!LectVideAdonf()) return; /* preventif */
			if(CLUZEL_HalfFull(val)) board->Half++;
			if(CLUZEL_FullFull(val)) board->Full++;
			*user++ = val;
		} while(++(board->nb_lues) < board->max_lues);
	}

	return(rc);
}
/* ========================================================================== */
int IcaXferDone(PciBoard board) {
	int val;

	if(board->PLXregs) {
		val = PLXread(board,PLX_DMA_CSR0);
		return((val & 0x10)? board->max_lues: 0);  /* PLX_DMA_CSR0<4>: DMA done */
	} else return(board->nb_lues);
}
/* ========================================================================== */
IcaReturnCode IcaXferStatusGet(PciBoard board, int *empty, int *half, int *full) {
	if(!board) return(0);
	*empty = board->Empty;
	*half = board->Half;
	*full = board->Full;
	return(1);
}
/* ========================================================================== */
IcaReturnCode IcaXferStatusReset(PciBoard board) {
	if(!board) return(0);
	board->Empty = board->Half = board->Full = 0;
	return(1);
}
/* ========================================================================== */
char IcaDmaDispo(PciBoard board) { return((board->PLXregs == 0)? 0: 1); }
#endif /* !MACOSX: MacOS 8.6 ou 9.x, voire linux */
#endif /* !SANS_PCI */
