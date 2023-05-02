#ifndef PCILIB_H
#define PCILIB_H

#ifdef CODE_WARRIOR_VSN
#include <PciDefine.h>
#else
#include <PCI/PciDefine.h>
#endif

hexa PciDeviceID(PciBoard board);
char PciDeviceFind(shex vid, shex did, int index, PciBoard board);
Ulong *PciDeviceAdrs(PciBoard board, int reg);

#endif /* PCILIB_H */
