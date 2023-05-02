/* pciConfigLib.h - PCI bus constants header file */

/* Copyright 1984-1998 Wind River Systems, Inc. */

/*
modification history
--------------------
01q,06apr98,dat  added back pciDevConfig, which was omitted. removed non-ansi
		 declarations.
01p,11mar98,tm   renamed from pciIomapLib.h to pciConfigLib.h
                 scope changed: LOCAL int pciPack() --> int pciConfigBDFPack()
01o,08mar98,tm   moved PCI_IN_*, PCI_OUT_* macros here from pciIomapLib.c
01n,04mar98,tm   moved pciIomapShow.c prototypes to pciIomapShow.h
01m,04mar98,tm   updated argument types on all I/O functions 
01l,04mar98,dat  added PCI_MECHANISM_0 for user defined config mechanism
01k,04mar98,tm   moved config space header definitions to pciHeaderDefs.h
01j,24feb98,tm   moved interrupt handling functions to pciIntLib.{c,h}
                 added PCI class definitions
01i,06dec97,rlp  added SNOOZE_MODE, SLEEP_MODE_DIS and PCI_CFG_MODE.
01h,25jun97,jpd  added PCI_MECHANISM_3, increased PCI_IRQ_LINES to 32.
01g,12jan97,hdn  changed member/variable name "vender" to "vendor".
01f,12jan97,hdn  changed member/variable name "class" to "classCode".
01e,03dec96,hdn  added header-type bits definitions.
01d,13nov96,hdn  added command-bit and address-mask definitions.
01c,06aug96,hdn  added a structure PCI_INT_NODE.
01b,28mar96,hdn  removed BIOS dependent stuff.
01a,23nov94,bcs  written.
*/

#ifndef PCIOFFSETS_H
#define PCIOFFSETS_H

/* PCI Configuration mechanisms */

#define PCI_MECHANISM_0         0       /* non-std user-supplied interface */
#define	PCI_MECHANISM_1		1	/* current PC-AT hardware mechanism */
#define	PCI_MECHANISM_2		2	/* deprecated */

/* Configuration I/O addresses for mechanism 1 */

#define	PCI_CONFIG_ADDR		0x0cf8	/* write 32 bits to set address */
#define	PCI_CONFIG_DATA		0x0cfc	/* 8, 16, or 32 bit accesses */

/* Configuration I/O addresses for mechanism 2 */

#define	PCI_CONFIG_CSE		0x0cf8	/* CSE register */
#define	PCI_CONFIG_FORWARD	0x0cfa	/* forward register */
#define	PCI_CONFIG_BASE		0xc000	/* base register */

/* PCI command bits */

#define PCI_CMD_IO_ENABLE	0x0001	/* IO access enable */
#define PCI_CMD_MEM_ENABLE	0x0002	/* memory access enable */
#define PCI_CMD_MASTER_ENABLE	0x0004	/* bus master enable */
#define PCI_CMD_MON_ENABLE	0x0008	/* monitor special cycles enable */
#define PCI_CMD_WI_ENABLE	0x0010	/* write and invalidate enable */
#define PCI_CMD_SNOOP_ENABLE	0x0020	/* palette snoop enable */
#define PCI_CMD_PERR_ENABLE	0x0040	/* parity error enable */
#define PCI_CMD_WC_ENABLE	0x0080	/* wait cycle enable */
#define PCI_CMD_SERR_ENABLE	0x0100	/* system error enable */
#define PCI_CMD_FBTB_ENABLE	0x0200	/* fast back to back enable */

/* PCI base address mask bits */

#define PCI_MEMBASE_MASK	~0xf	/* mask for memory base address */
#define PCI_IOBASE_MASK		~0x3	/* mask for IO base address */
#define PCI_BASE_IO		0x1	/* IO space indicator */
#define PCI_BASE_BELOW_1M	0x2	/* memory locate below 1MB */
#define PCI_BASE_IN_64BITS	0x4	/* memory locate anywhere in 64 bits */
#define PCI_BASE_PREFETCH	0x8	/* memory prefetchable */

/* PCI header type bits */

#define PCI_HEADER_TYPE_MASK	0x7f	/* mask for header type */
#define PCI_HEADER_PCI_PCI	0x01	/* PCI to PCI bridge */
#define PCI_HEADER_MULTI_FUNC	0x80	/* multi function device */

/* PCI configuration device and driver */
 
#define SNOOZE_MODE             0x40    /* snooze mode */
#define SLEEP_MODE_DIS          0x00    /* sleep mode disable */

/* Standard device configuration register offsets */
/* Note that only modulo-4 addresses are written to the address register */

#define	PCI_CFG_VENDOR_ID		0x00
#define	PCI_CFG_DEVICE_ID		0x02
#define	PCI_CFG_COMMAND			0x04
#define	PCI_CFG_STATUS			0x06
#define	PCI_CFG_REVISION		0x08
#define	PCI_CFG_PROGRAMMING_IF	0x09
#define	PCI_CFG_SUBCLASS		0x0a
#define	PCI_CFG_CLASS			0x0b
#define	PCI_CFG_CACHE_LINE_SIZE	0x0c
#define	PCI_CFG_LATENCY_TIMER	0x0d
#define	PCI_CFG_HEADER_TYPE		0x0e
#define	PCI_CFG_BIST			0x0f
#define	PCI_CFG_BASE_ADDRESS_0	0x10
#define	PCI_CFG_BASE_ADDRESS_1	0x14
#define	PCI_CFG_BASE_ADDRESS_2	0x18
#define	PCI_CFG_BASE_ADDRESS_3	0x1c
#define	PCI_CFG_BASE_ADDRESS_4	0x20
#define	PCI_CFG_BASE_ADDRESS_5	0x24
#define	PCI_CFG_CIS				0x28
#define	PCI_CFG_SUB_VENDOR_ID	0x2c
#define	PCI_CFG_SUB_SYSTEM_ID	0x2e
#define	PCI_CFG_EXPANSION_ROM	0x30
#define	PCI_CFG_RESERVED_0		0x34
#define	PCI_CFG_RESERVED_1		0x38
#define	PCI_CFG_DEV_INT_LINE	0x3c
#define	PCI_CFG_DEV_INT_PIN		0x3d
#define	PCI_CFG_MIN_GRANT		0x3e
#define	PCI_CFG_MAX_LATENCY		0x3f
#define PCI_CFG_SPECIAL_USE     0x41
#define PCI_CFG_MODE            0x43


/* PCI-to-PCI bridge configuration register offsets */
/* Note that only modulo-4 addresses are written to the address register */

#define	PCI_CFG_PRIMARY_BUS	0x18
#define	PCI_CFG_SECONDARY_BUS	0x19
#define	PCI_CFG_SUBORDINATE_BUS	0x1a
#define	PCI_CFG_SEC_LATENCY	0x1b
#define	PCI_CFG_IO_BASE		0x1c
#define	PCI_CFG_IO_LIMIT	0x1d
#define	PCI_CFG_SEC_STATUS	0x1e
#define	PCI_CFG_MEM_BASE	0x20
#define	PCI_CFG_MEM_LIMIT	0x22
#define	PCI_CFG_PRE_MEM_BASE	0x24
#define	PCI_CFG_PRE_MEM_LIMIT	0x26
#define	PCI_CFG_PRE_MEM_BASE_U	0x28
#define	PCI_CFG_PRE_MEM_LIMIT_U	0x2c
#define	PCI_CFG_IO_BASE_U	0x30
#define	PCI_CFG_IO_LIMIT_U	0x32
#define	PCI_CFG_ROM_BASE	0x38
#define	PCI_CFG_BRG_INT_LINE	0x3c
#define	PCI_CFG_BRG_INT_PIN	0x3d
#define	PCI_CFG_BRIDGE_CONTROL	0x3e

/* PCI Class definitions for find by class function */

#define PCI_CLASS_PRE_PCI20	0x00
#define PCI_CLASS_MASS_STORAGE	0x01
#define PCI_CLASS_NETWORK_CTLR	0x02
#define PCI_CLASS_DISPLAY_CTLR	0x03
#define PCI_CLASS_MMEDIA_DEVICE	0x04
#define PCI_CLASS_MEM_CTLR	0x05
#define PCI_CLASS_BRIDGE_CTLR	0x06
#define PCI_CLASS_COMM_CTLR	0x07
#define PCI_CLASS_BASE_PERIPH	0x08
#define PCI_CLASS_INPUT_DEVICE	0x09
#define PCI_CLASS_DOCK_DEVICE	0x0A
#define PCI_CLASS_PROCESSOR	0x0B
#define PCI_CLASS_SERIAL_BUS	0x0C
#define PCI_CLASS_UNDEFINED	0xFF

/* Conditional defines for new configuration definitions */

/* Cache Line Size - 32 32-bit value = 128 bytes */
#ifndef PCI_CACHE_LINE_SIZE
#define PCI_CACHE_LINE_SIZE            0x20
#endif /* PCI_CACHE_LINE_SIZE */

/* Latency Timer value - 255 PCI clocks */
#ifndef PCI_LATENCY_TIMER
#define PCI_LATENCY_TIMER           0xff
#endif /* PCI_LATENCY_TIMER */

#endif
