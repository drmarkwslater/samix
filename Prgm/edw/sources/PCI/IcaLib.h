#ifndef ICALIB_H
#define ICALIB_H

#include <environnement.h>

#ifdef CODE_WARRIOR_VSN
#include <PciDefine.h>
#else
#include <PCI/PciDefine.h>
#endif

typedef enum {
	ICA_REG_UTIL = 0,
	ICA_REG_04,
	ICA_REG_RESET,
	ICA_REG_0C,
	ICA_REG_SELECT0,       /* 0x10 */
	ICA_REG_SELECT1,
	ICA_REG_SELECT2,
	ICA_REG_SELECT3,
	ICA_REG_STATUS,        /* 0x20 */
	ICA_REG_FPROM_ADRS,
	ICA_REG_FPROM_DATA,
	ICA_REG_FPROM_CNTL,
	ICA_REG_FILL,          /* 0x30 */
	ICA_REG_LECTCC,
	ICA_REG_FPROM_READ ,
	ICA_REG_FIFO_CNTL,
	ICA_REG_SIGNATURE,     /* 0x40 */
	ICA_REG_NB
} ICA_REGISTRE;

#ifdef SANS_PCI
	typedef char IcaReturnCode;
	IcaReturnCode IcaDerniereErreur();
	IcaReturnCode IcaConfigReadLong(io_connect_t *dataPort, int adrs, int *val);
	IcaReturnCode IcaConfigWriteLong(io_connect_t *dataPort, int adrs, int val);
	IcaReturnCode IcaConfigReadWord(io_connect_t *dataPort, int adrs, short *val);
	IcaReturnCode IcaConfigWriteWord(io_connect_t *dataPort, int adrs, int val);
	IcaReturnCode IcaConfigReadByte(io_connect_t *dataPort, int adrs, char *val);
	IcaReturnCode IcaConfigWriteByte(io_connect_t *dataPort, int adrs, int val);
#else  /* !SANS_PCI */
	#ifdef XCODE
		typedef kern_return_t IcaReturnCode;
		IcaReturnCode IcaDerniereErreur();
		IcaReturnCode IcaConfigReadLong(io_connect_t *dataPort, int adrs, int *val);
		IcaReturnCode IcaConfigWriteLong(io_connect_t *dataPort, int adrs, int val);
		IcaReturnCode IcaConfigReadWord(io_connect_t *dataPort, int adrs, short *val);
		IcaReturnCode IcaConfigWriteWord(io_connect_t *dataPort, int adrs, int val);
		IcaReturnCode IcaConfigReadByte(io_connect_t *dataPort, int adrs, char *val);
		IcaReturnCode IcaConfigWriteByte(io_connect_t *dataPort, int adrs, int val);
	#else  /* MACOS classique */
		typedef char IcaReturnCode;
	#endif /*  MACOS classique */
	/*
	typedef struct {
		int util; en ecriture, sinon en lecture c'est la FIFO 
		int reg04;
		int reset;
		int reg0C;
		int select0,select1,select2,select3;
		int status;
		int frpom_adrs,fprom_data,fprom_cntl;
		int fifo_cntl;
	} TypeIca;
	*/
#endif /* !SANS_PCI */

void IcaInit(PciBoard board);
void IcaRegisterUse(PciBoard board, int reg);
/*
IcaReturnCode IcaAdrsGet(PciBoard board, hexa type, Ulong **adrs, int *taille);
void IcaRegWrite(PciBoard board, int num, Ulong valeur);
Ulong IcaRegRead(PciBoard board, int num);
void IcaUtilWrite(PciBoard board, Ulong valeur); */
IcaReturnCode IcaAdrsGet(PciBoard board, hexa type, void **adrs, int *taille);
void IcaRegWrite(PciBoard board, int num, unsigned long valeur);
unsigned long IcaRegRead(PciBoard board, int num);
void IcaUtilWrite(PciBoard board, unsigned long valeur);
// IcaReturnCode IcaCommandSend(PciBoard board, byte adrs, byte action, byte ss_adrs, shex valeur);
IcaReturnCode IcaCommandSend(PciBoard board, int adrs, int action, int ss_adrs, int valeur);
IcaReturnCode IcaInstrWrite(PciBoard board, int adrs, int instr);
IcaReturnCode IcaFifoReset(PciBoard board);
int IcaFifoEmpty(PciBoard board, int nb_a_verifier);
hexa IcaFifoRead(PciBoard board);
IcaReturnCode IcaXferInit(PciBoard board, char avec_dma);
IcaReturnCode IcaXferSizeSet(PciBoard board, int taille);
IcaReturnCode IcaXferExec(PciBoard board);
int IcaXferDone(PciBoard board);
IcaReturnCode IcaXferStatusGet(PciBoard board, int *empty, int *half, int *full);
IcaReturnCode IcaXferStatusReset(PciBoard board);
char IcaDmaDispo(PciBoard board);

#endif
