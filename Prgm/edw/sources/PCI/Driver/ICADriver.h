#ifndef ICADRIVER_H
#define ICADRIVER_H

#include <IOKit/IOService.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/pci/IOPCIDevice.h>
#include "IcaDefine.h"

#define DMA_MAXBLOCS 64

typedef struct {
	Ulong adrs_pci;
	Ulong adrs_loc;
	Ulong taille;
	Ulong suivant;
} TypeDmaBloc;

class ICADriver : public IOService {
	OSDeclareDefaultStructors(ICADriver)

	private:
		IOPCIDevice *carte;
		IOMemoryMap *mapping_pci,*mapping_plx,*mapping_fifo;
		IOMemoryDescriptor *mem_fifo;
		Ulong  acces_pci;
		Ulong *PCIadrs,*ResetAdrs,*FifoUserAdrs;
//		Ulong *adrs_lues,
		Ulong  max_lues,nb_lues;
		Ulong  Empty,Half,Full;
		Ulong *PLXregs;
		Ulong  SimulPCI[4];
		TypeDmaBloc dma[DMA_MAXBLOCS] __attribute__ ((aligned (16)));
		IOMemoryDescriptor *dma_desc;
		Ulong  premier_bloc;
		int nb_blocs;
		char dma_ok;

	public:
		IOReturn ConfigRead32(int adrs, int *val);
		IOReturn ConfigWrite32(int adrs, int val);
		IOReturn ConfigRead16(int adrs, shex *val);
		IOReturn ConfigWrite16(int adrs, int val);
		IOReturn ConfigRead8(int adrs, byte *val);
		IOReturn ConfigWrite8(int adrs, int val);

		IOReturn PciRegDump(char memoire);
		IOReturn PlxRegDump();
		Ulong PLXread(int reg);
		void PLXwrite(int reg, Ulong val);
		IOReturn Setup();
		IOReturn RegisterUse(int reg);
		IOReturn UtilWrite(int valeur);
		IOReturn RegWrite(int num, int valeur);
		IOReturn RegRead(int num, int *valeur);
		IOReturn CommandSend(int adrs, int action, int ss_adrs, int valeur);
		IOReturn InstrWrite(int adrs, int instr);
		IOReturn FifoReset();
		IOReturn FifoEmpty(int nb_a_verifier, int *num_non_nul);
		IOReturn FifoRead(Ulong *val);
		IOReturn XferInit(int avec_dma, Ulong taille, Ulong *fifo);
		IOReturn XferExec(Ulong taille, Ulong *fifo);
		IOReturn XferDone(int *done);
		IOReturn XferStatusGet(int *empty, int *half, int *full);
		IOReturn XferStatusReset();
		IOReturn DmaDispo(int *dispo);
		virtual IOMemoryDescriptor *PCIGetDesc(void);

		virtual bool init(OSDictionary *dictionary = 0);
		virtual void free(void);
		virtual IOService *probe(IOService *provider, SInt32 *score);
		/* IOService overrides */
		virtual bool start(IOService *provider);
		virtual void stop(IOService *provider);
};

#endif
