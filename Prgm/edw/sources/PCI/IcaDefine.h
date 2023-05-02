#ifndef ICADEFINE_H
#define ICADEFINE_H

#ifndef CLUZEL_FIFO_INTERNE
// #define CLUZEL_FIFO_INTERNE 131072
// #define CLUZEL_FIFO_INTERNE 4096
#define CLUZEL_FIFO_INTERNE 128*1024*4
#endif
#define ICA_FIFO_ADRS 0x50000000

enum {
	ICA_MEM_INTERNE = 100,  /* pourquoi pas 0? cf AppleSamplePCI, on ne sait jamais... */
	ICA_ACCES_PCI
};

enum {
	ICA_OPEN,
	ICA_CLOSE,
	ICA_CONFIG_READ32,
	ICA_CONFIG_WRITE32,
	ICA_CONFIG_READ16,
	ICA_CONFIG_WRITE16,
	ICA_CONFIG_READ8,
	ICA_CONFIG_WRITE8,

	ICA_INIT,
	ICA_REGISTER_USE,
	ICA_UTIL_WRITE,
	ICA_REG_WRITE,
	ICA_REG_READ,
	ICA_COMMAND_SEND,
	ICA_INSTR_WRITE,
	ICA_FIFO_RESET,
	ICA_FIFO_EMPTY,
	ICA_FIFO_READ,
	ICA_XFER_INIT,
	ICA_XFER_SIZE_SET,
	ICA_XFER_EXEC,
	ICA_XFER_DONE,
	ICA_XFER_STATUS_GET,
	ICA_XFER_STATUS_RESET,
	ICA_DMA_DISPO,
	ICA_METHODES_NB
};

#endif