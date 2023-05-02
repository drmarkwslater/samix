#ifndef DIMENSIONS_H
#define DIMENSIONS_H

#define NEANT 0
#define KB 1024
#define MB (KB * KB)

#define MODL_NOM 16 /* un peu galvaude. Preciser un peu.. */
#define CODE_MAX 8

#define MAXSAT 8

#define DETEC_NOM 16
#define DETEC_REGL_NOM 16
#define DETEC_REGL_MAX 64
#define DETEC_CAPT_MAX 16  // MAXVOIESBOLO // DETEC_CAPT_MAX // 6 dans le futur
#define DETEC_SOFT_MAX 16

#define VOIE_NOM (DETEC_NOM+MODL_NOM+1)

#define NUMER_MAX 112
#define NUMER_NOM 16
#define NUMER_RESS_NOM 16
#define NUMER_RESS_VALTXT 12
#define NUMER_FPGA_TYPES 8
#define NUMER_ACCES_TYPES 8

#define REPART_VOIES_MAX 256

#define FIBRE_NOM 8 // doit etre <= (NUMER_NOM - 2)
// HOST_NAME_MAX defini dans <limits.h>??
#define HOTE_NOM 80
#define PCI_MAX 4
#define NI_TACHES_MAX 2
#define TYPERUN_NOM 12
#define RUN_NOM 16
#define UNITE_NOM 16
#define TRGR_NOM 28
#define FLTR_MAX 64
#define SAMBA_PROC_MAX 64
#define SAMBA_ECHANT_MAX 512

#define TRMT_NOM 32
#define TRMT_CSNSM_MAX 8
#define TRGR_SCRPT_MAX 40

#define AUTOM_REGEN_MAX 2
#define AUTOM_CALIB_MAX 2

#define EXPORT_MAXPACKS 8

#endif


