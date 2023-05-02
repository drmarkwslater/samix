#ifndef PLX_H
#define PLX_H

#define PLX_LAS0_RR    0x00/4
#define PLX_LAS0_BA    0x04/4

#define PLX_MARBR      0x08/4
#define PLX_BIGEND     0x0C/4

#define PLX_EROM_RR    0x10/4
#define PLX_EROM_BA    0x14/4

#define PLX_LBRD0      0x18/4

#define PLX_DM_RR      0x1C/4
#define PLX_DM_LBAM    0x20/4
#define PLX_DM_LBAI    0x24/4
#define PLX_DM_PBAM    0x28/4
#define PLX_DM_CFGA    0x2C/4

#define PLX_MBOX0      0x40/4
#define PLX_MBOX1      0x44/4
#define PLX_MBOX2      0x48/4
#define PLX_MBOX3      0x4C/4
#define PLX_MBOX4      0x50/4
#define PLX_MBOX5      0x54/4
#define PLX_MBOX6      0x58/4
#define PLX_MBOX7      0x5C/4

#define PLX_P2L_DBELL  0x60/4
#define PLX_L2P_DBELL  0x64/4

#define PLX_INTCSR     0x68/4
#define PLX_CNTRL      0x6C/4
#define PLX_PCI_HIDR   0x70/4
#define PLX_PCI_HREV   0x74/4

#define PLX_DMA_MODE0  0x80/4
#define PLX_DMA_PADR0  0x84/4
#define PLX_DMA_LADR0  0x88/4
#define PLX_DMA_SIZ0   0x8C/4
#define PLX_DMA_DPR0   0x90/4

#define PLX_DMA_MODE1  0x94/4
#define PLX_DMA_PADR1  0x98/4
#define PLX_DMA_LADR1  0x9C/4
#define PLX_DMA_SIZ1   0xA0/4
#define PLX_DMA_DPR1   0xA4/4

#define PLX_DMA_CSR0   0xA8/4
//#define PLX_DMA_CSR1   0xA9/4

#define PLX_DMA_ARB    0xAC/4
#define PLX_DMA_THR    0xB0/4
#define PLX_DMA_DAC0   0xB4/4
#define PLX_DMA_DAC1   0xB8/4

#define PLX_LAS1_RR    0xF0/4
#define PLX_LAS1_BA    0xF4/4
#define PLX_LBRD1      0xF8/4

#define PLX_DM_DAC     0xFC/4

#endif
