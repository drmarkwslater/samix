#ifndef REPARTITEURS_H
#define REPARTITEURS_H

#ifdef REPARTITEURS_C
#define REPARTITEUR_VAR
#else
#define REPARTITEUR_VAR extern
#endif

#include <environnement.h>
#include <edw_types.h>
#include <media.h>

#include <opium4/opium.h>
#include <tcp/sktlib.h>
#include <PCI/PciDefine.h>

#define OPERA_SUPPORT
#define CALI_SUPPORT

/* .......................................................................... */
/*                                M O D E L E S                               */

#define REPART_MAX 128
#define REPART_LOGIN_NOM 16

#define REPART_NOM 32
#define REPART_ENTREES_MAX 128
#define REPART_MODL_NOM 32
#define REPART_CODE_NOM 8
#define REPART_TYPES 12
#define REPART_MAXMSG 256

#define REPART_IP_CNTL (shex)0x8000

/* ... Listes de codes ... */
#define CODES_FORMAT "edw0/edw1/edw2"
#define CODES_ESCLAVE "interne/externe"
#define CODES_SYNCHRO "20160/25000/100000/100000"

typedef enum {
	FAMILLE_CLUZEL = 0,
	FAMILLE_OPERA,
	FAMILLE_CALI,
	FAMILLE_IPE,
	FAMILLE_ETHER,
	FAMILLE_MEDIA,
	FAMILLE_ARCHIVE,
	FAMILLE_SAMBA,
	FAMILLE_NI,
	FAMILLE_NB
} REPART_FAMILLES;
REPARTITEUR_VAR char *RepartFamilleListe[FAMILLE_NB+1]
#ifdef REPARTITEURS_C
 = { "Cluzel", "OPERA", "CALI", "IPE", "Ethernet", "Media", "Archive", "SAMBA", "NI", "\0" }
#endif
;
REPARTITEUR_VAR struct {
	size_t lngr;
} RepartTrameDef[FAMILLE_NB];
// REPARTITEUR_VAR size_t RepartTrameLngr[FAMILLE_NB];

// #define MAXREPFAM 8

#define REP_SELECT_UNDEF -1
typedef enum {
	REP_SELECT_NEANT = 0,
	REP_SELECT_NUMER,
	REP_SELECT_CANAL,
	REP_SELECT_NB
} REP_SELECT_NIVEAU;
REPARTITEUR_VAR char *RepartNomSelecteur[REP_SELECT_NB+1]
#ifdef REPARTITEURS_C
 = { "aucun", "numeriseurs", "canaux", "\0" }
#endif
;

typedef enum {
	REP_STATUT_HS = 0,
	REP_STATUT_VALIDE,
	REP_STATUT_SIMULE,
	REP_STATUT_NB
} REP_STATUTS;
#define REP_STATUTS_CLES "HS/valide/simule"

typedef enum {
	INTERF_PCI = 0,
	INTERF_IP,
	INTERF_NAT, // National Intruments
	INTERF_USB,
	INTERF_FILE,
	INTERF_NBTYPES
} INTERF_TYPES;

REPARTITEUR_VAR char *InterfaceListe[INTERF_NBTYPES+1]
#ifdef REPARTITEURS_C
 = { "PCI", "IP", "NI", "USB", "Fichier", "\0" }
#endif
;

typedef struct {
	short adrs;
	short ssadrs;
	short valeur;
} TypePciReg;

typedef struct {
	hexa adrs;
	hexa valeur;
} Type32bitsReg;

/* #define IP_DECOMPOSEE(ip) \
	BigEndianOrder? (ip>>24&0xFF):  ip&0xFF,\
	BigEndianOrder? (ip>>16)&0xFF: (ip>>8)&0xFF,\
	BigEndianOrder? (ip>>8)&0xFF : (ip>>16)&0xFF,\
	BigEndianOrder?  ip&0xFF     : (ip>>24&0xFF) */

typedef enum {
	IPACTN_FTP,
	IPACTN_TELNET,
	IPACTN_SSH,
	IPACTN_SCP,
	IPACTN_SHELL,
	IPACTN_NB
} IP_ACTION;
#define REPART_ACTIONS_CLES "ftp/telnet/ssh/scp/shell"

typedef struct {
	char duree; /* secondes */
	char type; /* ftp/telnet/shell */
	char directe;
	char fichier[MAXFILENAME];
	char dest[MAXFILENAME];
} TypeIpActn;

#define REPART_IPACTN_MAX 12
#define REPART_PCIREG_MAX 8

typedef struct {
	union {
		struct { int nbactn; TypeIpActn  actn[REPART_IPACTN_MAX]; } ip;
		struct { int nbregs; TypePciReg  reg[REPART_PCIREG_MAX];  } pci;
	};
} TypeRepartProc;

typedef struct {
	char  nom[REPART_MODL_NOM];  /* Nom precis pour les impressions            */
	char  code[REPART_CODE_NOM]; /* code reference dans les preferences        */
	int   famille;               /* famille a laquelle il est rattache         */
	int   interface;             /* type d'interface                           */
	short taille_donnees;
	short taille_entete;
	short code_status;
	struct {
		int type;                /* type de selection des donnees              */
		short max;               /* nb registres de selection                  */
	} selecteur;
	char  revision;              /* niveau de realisation dans cette famille   */
	char  status;                /* existence d'un mot de status               */
	char  regroupe_numer;        /* vrai si voies regroupees par numeriseur    */
	char  bits32;                /* 0: 16bits/ 1: 32bits                       */
	int   delai_msg;             /* attente entre ordres (us)                  */
	float version;               /* valeur purement utilisateur                */
	float horloge;               /* horloge fondamentale du systeme (MHz)      */
	int   diviseur0;             /* diviseur de l'horloge pour un bloc         */
	int   diviseur1;             /* diviseur pour l'echantillonnage            */
	lhex  masque;                /* bits de donnee utiles                      */
//	short voies_par_numer;       /* nombre de voies par numeriseur             */
	short max_numer;             /* nombre de numeriseurs transmissibles       */
	TypeRepartProc i;            /* initialisation du repartiteur              */
	TypeRepartProc d;            /* demarrage du repartiteur                   */
} TypeRepModele;

REPARTITEUR_VAR TypePciReg RepartRegLu;
REPARTITEUR_VAR TypeIpActn RepartActnLue;

REPARTITEUR_VAR TypeRepModele ModeleRep[REPART_TYPES],ModeleRepLu;
REPARTITEUR_VAR char *ModeleRepListe[REPART_TYPES+1];
REPARTITEUR_VAR int ModeleRepNb;

#ifdef A_TERMINER
/* ... Variantes selon la famille ... 
static ArgDesc RepartCluzelDesc[] = {
	{ "d2", DESC_SHORT &(ModeleRepLu.r.cluzel.d2), 0 },
	{ "d3", DESC_SHORT &(ModeleRepLu.r.cluzel.d3), 0 },
	DESC_END
};
static ArgDesc RepartOperaDesc[] = {
	{ "d0",      DESC_SHORT                    &(ModeleRepLu.r.opera.d0), 0 },
	{ "d2d3",    DESC_KEY CODES_SYNCHRO,       &(ModeleRepLu.r.opera.d2d3), 0 },
	{ "retard",  DESC_BYTE                     &(ModeleRepLu.r.opera.retard), 0 },
	{ "mode",    DESC_LISTE OperaCodesAcquis,  &(ModeleRepLu.r.opera.mode), 0 },
	{ "horloge", DESC_KEY CODES_ESCLAVE,       &(ModeleRepLu.r.opera.clck), 0 },
	{ "synchro", DESC_KEY CODES_ESCLAVE,       &(ModeleRepLu.r.opera.sync), 0 },
	DESC_END
};
static ArgDesc RepartCaliDesc[] = {
	{ "horloge", DESC_KEY CODES_ESCLAVE,       &(RepartLu.r.cali.clck), 0 },
	DESC_END
};
static ArgDesc RepartIpeDesc[] = {
	{ "horloge", DESC_KEY CODES_ESCLAVE,       &(RepartLu.r.cali.clck), 0 },
	DESC_END
};
static ArgDesc RepartSambaDesc[] = {
	{ "horloge", DESC_KEY CODES_ESCLAVE,       &(RepartLu.r.cali.clck), 0 },
	DESC_END
};
static ArgDesc RepartNiDesc[] = {
	{ "nb.voies", DESC_INT &(ModeleRepLu.voies_par_numer), 0 },
	DESC_END
};
static ArgDesc RepartArchiveDesc[] = { DESC_END };
ArgDesc *ModeleFamDesc[] = { RepartCluzelDesc, RepartOperaDesc, RepartCaliDesc, RepartIpeDesc, RepartSambaDesc, RepartNiDesc, RepartArchiveDesc, 0 }; 

>> ... Variantes selon l'interface ... <<
static ArgDesc RepartRegDesc[] = {
	{ 0, DESC_SHEX &(RepartRegLu.adrs), 0 },
	{ 0, DESC_SHEX &(RepartRegLu.ssadrs), 0 },
	{ 0, DESC_SHEX &(RepartRegLu.valeur), 0 },
	DESC_END
};
static ArgStruct RepartRegsInitAS = { (void *)ModeleRepLu.i.pci.reg, (void *)&RepartRegLu, sizeof(TypePciReg), RepartRegDesc };
static ArgStruct RepartRegsRazAS  = { (void *)ModeleRepLu.d.pci.reg, (void *)&RepartRegLu, sizeof(TypePciReg), RepartRegDesc };
static ArgDesc ModeleIfPciDesc[] = {
	{ "init",    DESC_STRUCT_SIZE(ModeleRepLu.i.pci.nbregs,REPART_PCIREG_MAX) &RepartRegsInitAS, 0 },
	{ "start",   DESC_STRUCT_SIZE(ModeleRepLu.d.pci.nbregs,REPART_PCIREG_MAX) &RepartRegsRazAS, 0 },
	DESC_END
};
static ArgDesc RepartActnXferDesc[] = {
	{ 0,       DESC_STR(MAXFILENAME)  RepartActnLue.fichier, 0 },
	{ "dest",  DESC_STR(MAXFILENAME)  RepartActnLue.dest, 0 },
	DESC_END
};
static ArgDesc RepartActnScriptDesc[] = {
	{ 0,       DESC_STR(MAXFILENAME)  RepartActnLue.fichier, 0 },
	{ "duree", DESC_BYTE             &RepartActnLue.duree, 0 },
	DESC_END
};
static ArgDesc *RepartActnParmDesc[] = { RepartActnXferDesc, RepartActnScriptDesc, RepartActnScriptDesc, RepartActnScriptDesc, 0 };
static ArgDesc RepartActnDesc[] = {
	{ 0, DESC_KEY REPART_ACTIONS_CLES,    &RepartActnLue.type, 0 },
	{ 0, DESC_VARIANTE(RepartActnLue.type) RepartActnParmDesc, 0 },
	DESC_END
};
static ArgStruct ModeleRepActnInitAS = { (void *)ModeleRepLu.i.ip.actn, (void *)&RepartActnLue, sizeof(TypeIpActn), RepartActnDesc };
static ArgStruct ModeleRepActnRazAS  = { (void *)ModeleRepLu.d.ip.actn, (void *)&RepartActnLue, sizeof(TypeIpActn), RepartActnDesc };
static ArgDesc ModeleIfIpDesc[] = {
	{ "port",    DESC_INT            &ModeleRepLu.ecr.port, 0 },
	{ "user",    DESC_STR(REPART_LOGIN_NOM) ModeleRepLu.p.ip.user, 0 },
	{ "pswd",    DESC_STR(REPART_LOGIN_NOM) ModeleRepLu.p.ip.pswd, 0 },
	{ "init",    DESC_STRUCT_SIZE(ModeleRepLu.i.ip.nbactn,REPART_IPACTN_MAX) &ModeleRepActnInitAS, 0 },
	{ "start",   DESC_STRUCT_SIZE(ModeleRepLu.d.ip.nbactn,REPART_IPACTN_MAX) &ModeleRepActnRazAS, 0 },
	DESC_END
};
static ArgDesc ModeleIfUsbDesc[] = { DESC_END };
static ArgDesc ModeleIfFicDesc[] = { DESC_END };
static ArgDesc *ModeleIfDesc[]  = { ModeleIfPciDesc, ModeleIfIpDesc, ModeleIfUsbDesc, ModeleIfFicDesc, 0 }; */
#endif /* A_TERMINER */

REPARTITEUR_VAR ArgDesc ModeleRepDesc[]
#ifdef REPARTITEURS_C
 = {
	{ 0,                DESC_STR(REPART_CODE_NOM)       ModeleRepLu.code,           0 },
	{ "designation",    DESC_STR(REPART_MODL_NOM)       ModeleRepLu.nom,            0 },
  	{ "famille",        DESC_LISTE RepartFamilleListe, &ModeleRepLu.famille,        0 }, // "famille de repartiteur (Cluzel/OPERA/NI)" },
	{ "interface",      DESC_LISTE InterfaceListe,     &ModeleRepLu.interface,      0 }, // "type d'interface (pci/ip/usb/file)" },
	{ "ip.max_donnees", DESC_SHORT                     &ModeleRepLu.taille_donnees, 0 },
	{ "ip.dim_entete",  DESC_SHORT                     &ModeleRepLu.taille_entete,  0 },
	{ "ip.code_status", DESC_SHORT                     &ModeleRepLu.code_status,    0 },
  	{ "type_selecteur", DESC_LISTE RepartNomSelecteur, &ModeleRepLu.selecteur.type, 0 }, // "type de selection des donnees (aucun/numeriseurs/canaux)" },
	{ "dim_selecteur",  DESC_SHORT                     &ModeleRepLu.selecteur.max,  0 }, // "nb registres 32bits de selection" },
	{ "type",           DESC_DEVIENT("niveau") },
	{ "niveau",         DESC_BYTE                      &ModeleRepLu.revision,       0 }, // "niveau de realisation dans cette famille" },
	{ "revision",       DESC_DEVIENT("niveau") },
	{ "version",        DESC_FLOAT                     &ModeleRepLu.version,        0 },
	{ "delai_msg",      DESC_INT                       &ModeleRepLu.delai_msg,      0 }, // "attente entre ordres (us)" },
	{ "horloge_MHz",    DESC_FLOAT                     &ModeleRepLu.horloge,        0 }, // "horloge fondamentale du systeme (MHz)" },
	{ "diviseur0",      DESC_INT                       &ModeleRepLu.diviseur0,      0 }, // "diviseur de l'horloge pour un bloc" },
	{ "diviseur1",      DESC_INT                       &ModeleRepLu.diviseur1,      0 }, // "diviseur pour l'echantillonnage" },
	{ "masque",         DESC_HEXA                      &ModeleRepLu.masque,         0 }, // "bits de donnee utiles" },
	{ "donnees",        DESC_KEY "16bits/32bits",      &ModeleRepLu.bits32,         0 }, // "dimension des mots de donnee (0: 16bits/ 1: 32bits)" },
	{ "max_numer",      DESC_SHORT                     &ModeleRepLu.max_numer,      0 }, // "maxi numeriseurs transmissibles" },
	{ "bloc_numer",     DESC_BOOL                      &ModeleRepLu.regroupe_numer, 0 }, // "vrai si voies regroupees par numeriseur" },
	{ "status",         DESC_BOOL                      &ModeleRepLu.status,         0 }, // "production d'un mot de status en fin de bloc" },
#ifdef A_TERMINER
	{ 0,         DESC_VARIANTE(ModeleRepLu.famille) ModeleFamDesc,     0 },
	{ 0,         DESC_VARIANTE(ModeleRepLu.interf)  ModeleIfDesc,      0 },
#endif
	DESC_END
}
#endif
;
REPARTITEUR_VAR ArgStruct ModeleRepAS
#ifdef REPARTITEURS_C
 = { (void *)ModeleRep, (void *)&ModeleRepLu, sizeof(TypeRepModele), ModeleRepDesc }
#endif
;

REPARTITEUR_VAR ArgDesc RepertModlDesc[]
#ifdef REPARTITEURS_C
 = {
	{ "Repartiteurs", DESC_STRUCT_SIZE(ModeleRepNb,REPART_TYPES) &ModeleRepAS, 0 },
	DESC_END
}
#endif
;

/* .......................................................................... */
/*                        M A T E R I E L   U T I L I S E                     */

typedef enum {
	REPART_CHARGE = 0,
	REPART_DEMARRE,
	REPART_PARMS,
	REPART_CONNECTE,
	REPART_NBETAPES,
	REPART_STOPPE
} REPART_ETAPES;
REPARTITEUR_VAR char *RepartItem[REPART_NBETAPES+2]
#ifdef REPARTITEURS_C
= { "charger", "demarrer", "parametrer", "connecter", "\0", "stopper" }
#endif
;
REPARTITEUR_VAR char *RepartEtape[REPART_NBETAPES+2]
#ifdef REPARTITEURS_C
= { "Chargement", "Redemarrage", "Parametrage", "Connexion", "\0", "Arret" }
#endif
;

typedef enum {
	REPART_ENVOI_DETEC = 0,
	REPART_ENVOI_STATUS,
	REPART_ENVOI_NB
} REPART_ENVOI_NATURE;

typedef enum {
	DONNEES_STD = 0,
	DONNEES_BB,
	DONNEES_STAMP,
	DONNEES_FICHIER,
	DONNEES_NBCATEG
} REPART_DONNEES;
REPARTITEUR_VAR char *RepartDonneesListe[DONNEES_NBCATEG+1]
#ifdef REPARTITEURS_C
= { "standard", "BB", "stamp", "fichier", "\0" }
#endif
;
#define REPART_DONNEES_CLES "standard/BB/stamp/fichier"

typedef enum {
	REPART_IP_ABANDON = 0,
	REPART_IP_INSISTE,
	REPART_IP_GENERE,
	REPART_IP_CORRECNB
} REPART_IP_CORRECTION;
#define REPART_CORRECTION_CLES "abandonne/insiste/genere"

typedef enum {
	NUMER_MODE_IDEM = 0, NUMER_MODE_COMPTEUR, NUMER_MODE_ACQUIS, NUMER_MODE_IDENT, NUMER_MODE_NB
} NUMER_MODE;
REPARTITEUR_VAR char *NumerModeTexte[NUMER_MODE_NB+1]
#ifdef REPARTITEURS_C
 = { "inchange", "timestamp", "acquisition", "identification", "\0" }
#endif
;
REPARTITEUR_VAR char NumerModeAcquis,LectAcqModeNumer;

typedef enum {
	REPART_MODE_INDEX = 0,
	REPART_MODE_RAMPE,
	REPART_MODE_IPE,
	REPART_MODE_NB
} REPART_MODES;

REPARTITEUR_VAR struct {
	int32 masque_fixe;
	int32 masque_rampe;
	int32 offset; /* increment de la rampe, exemple 0x40 si masque rampe = 0xFC0 */
} RepartMode[REPART_MODE_NB]
#ifdef REPARTITEURS_C
= { { 0x0000FFFF, 0x00000000, 0x00000000},
	{ 0x00000000, 0x0000FFFF, 0x00000001},
	{ 0x0000FFF0, 0x0000000F, 0x00000001}
}
#endif
;

#define RESERVOIRS

/* ................................. Cluzel ................................. */
#define CLUZEL_ADRS        0x7F /* adresse repartiteur    */
#define CLUZEL_SSAD_ACQUIS 0x08
#define CLUZEL_SSAD_IDENT  0x09

#define CLUZEL_DELAI_MOT 1000
#define CLUZEL_DELAI_MSG   3
/* #define CLUZEL_CODE_ACQUIS 5    ancienne valeur de CMDE_ACQUIS, et index dans la table CmdeDef, pour obtenir la sous-adresse */
/* #define CLUZEL_CODE_IDENT  6    ancienne valeur de CMDE_IDENT,  et index dans la table CmdeDef, pour obtenir la sous-adresse */
#define CLUZEL_START 0
#define CLUZEL_STOP  1

typedef enum {
	CLUZEL_IMMEDIAT = 0,
	CLUZEL_DIFFERE,
	CLUZEL_MODULEE,
	CLUZEL_D3
} CLUZEL_MOMENT_ACTION;

/* Types de donnee */
#define CLUZEL_MARQUE_MODULATION (TypeADU)0x4000  /* Debut de synchro D2          */
#define CLUZEL_MARQUE_ECHANT     (TypeADU)0xC000  /* Debut de bloc hors modul     */
#define CLUZEL_MARQUE_NEANT      (TypeADU)0x8000  /* Ni l'un ni l'autre           */

/* Masques */
#define CLUZEL_MARQUE_SELECT     (TypeADU)0xC000  /* Chercher les blocs           */
// #define CLUZEL_VALEUR_SELECT   (TypeADU)0x3FFF  /* 0x0FFF (EDW1) puis 0x3FFF (EDW2) */ 
// #define CLUZEL_ALARMES_SELECT  (TypeADU)0x000F  /* Recuperer les alarmes HW/2   */
/* Hardware version 1: voie vehiculant des infos d'etat */
#define CLUZEL_ERREUR_SELECT  (TypeADU)0x3000  /* Recuperer les erreurs        */
#define CLUZEL_STATUS_SELECT  (TypeADU)0x0F00  /* Recuperer le status          */
// #define CLUZEL_VOIE8b_SELECT  0x00FF  /* Recuperer la voie a status   */

#define DATATYPE(val)   (TypeADU)(val & CLUZEL_MARQUE_SELECT)
#define DATAERREUR(val) (TypeADU)(val & CLUZEL_ERREUR_SELECT)
#define DATASTATUS(val) (TypeADU)(val & CLUZEL_STATUS_SELECT)
// #define DATAVALEUR(val) (val & CLUZEL_VALEUR_SELECT)
#define DATAVALEUR(voie,val) (val & VoieManip[voie].ADCmask)
// #define DATAVOIE8b(val) (val & CLUZEL_VOIE8b_SELECT)

#define CLUZEL_NOT_FULLFIFO 0x00400000
#define CLUZEL_NOT_HALFFIFO 0x00200000
#define CLUZEL_NOT_EMPTY    0x00100000
#define CLUZEL_WASFULL      0x00080000
#define CLUZEL_FullFull(val) (((val) & CLUZEL_NOT_FULLFIFO) == 0)
#define CLUZEL_HalfFull(val) (((val) & CLUZEL_NOT_HALFFIFO) == 0)
/* #define CLUZEL_Empty(val) (((val) & CLUZEL_NOT_EMPTY) == 0) */
#define CLUZEL_Empty(val) (((val) & 0xFFFF) == 0)  /* Plus sur pour le moment?? */

REPARTITEUR_VAR int CluzelLue;

#ifdef OPERA_SUPPORT
/* ................................. Opera .................................. */

#ifndef _version_OPERA
#include <structure.h>
#endif

REPARTITEUR_VAR char *OperaCodesMaj[]
#ifdef REPARTITEURS_C
#ifdef BB2_SEULE
 = {
	"cew", "Altera", "BBv2 complet", "BBv2 interne", "tout", "sauve config", 
	"(6=neant)", "(7=neant)", "bavard", "discret", "raz erreurs", "\0"
}
#else /* BB2_SEULE */
 = {
	"cew", "Altera", "BBv2", "BBv2-3", "BBv3", "tout", "sauve config", 
	"(6=neant)", "data", "bavard", "discret", "raz erreurs", "\0"
}
#endif /* BB2_SEULE */
#endif
;
REPARTITEUR_VAR byte OperaValMaj[]
#ifdef REPARTITEURS_C
#ifdef BB2_SEULE
 = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }
#else /* BB2_SEULE */
 = { 0, 1, 2, 23, 3, 4, 5, 6, 7, 8, 9, 10 }
#endif /* BB2_SEULE */
#endif
;

// EDW_BB21 remplace EDW_FBB le 19.12.08
// code_acqui_repartiteur_veto: anciennement 4, code "cadenceur"
// Opera11: x=16, retard=20, masque_BB=8, code_acqui=4, code_synchro=6
#define OPERA_NBMODES 16
REPARTITEUR_VAR char *OperaCodesAcquis[OPERA_NBMODES+1]
#ifdef REPARTITEURS_C
 = { "test",      "EDW_BB1",    "EDW_BB1_moy4",  "EDW_BB21",
	 "synchro",   "ADC_moy2",   "ADC_moy2_comp", "ADC_comp", 
	 "EDW_BB2",   "synchro_SC", "veto",          "test_xmit", 
	 "test_recv", "BBv2_comp",  "mode_14",       "mode_15",
	 "\0" }
#endif
;

#define OPERA_V2

#define OPERA_TRAME_STS      0xFFFF

#ifdef OPERA_V1
#define OPERA_CDE_BB 'C'
#endif
#ifdef OPERA_V2
#define OPERA_CDE_BB 'W'
#endif

#ifdef MEME_PAS_UTILE
union {
	short hexa[OPERA_STATUS_LNGR];
	struct {
		short nserie;            /* [0] */
		unsigned alim:1;         /* [1:0] */
		unsigned err_sync:1;     /* [1:1] */
		unsigned tempe:12;       /* [1:2-13] */
		unsigned reserve1:2;     /* [1:14-15] */
		short div2;              /* [2] */
		short div3;              /* [3] */
		short regul_t_gain;      /* [4] */
		short regul[6];          /* [5-10] */
		short filtre[6];         /* [11-16] */
		short adc[6];            /* [17-22] */
		short dac[12];           /* [23-34] */
		unsigned relai1:1;       /* [35:0] */
		unsigned relai2:1;       /* [35:1] */
		unsigned relai3:1;       /* [35:2] */
		unsigned reserve2:13;    /* [35:3-15] */
		short ref;               /* [36] */
		short triangle[6];       /* [37-42] */
		short carre[6];          /* [43-48] */
	} bb2;
} val_status_BB2;
#endif

typedef Structure_trame_udp TypeOperaTrame,*OperaTrame;

#define OPERA_STATUS_LNGR _nb_mots_status_bbv2
// #define OPERA_TAILLE_DONNEES (_nb_data_trame_udp * sizeof(uint4))
// #define OPERA_TAILLE_ENTETE (sizeof(TypeOperaTrame) - OPERA_TAILLE_DONNEES)
#define VOIES_PAR_4OCTETS (sizeof(uint4) / sizeof(TypeDonnee))
#define OPERA_NB_DONNEES (_nb_data_trame_udp * VOIES_PAR_4OCTETS)

#ifdef OPERA_STATUS_DANS_ENTETE
	#define OPERA_TRAME_TIMESTAMP(trame)     ((((int64)(((OperaTrame)trame)->code) & 0xFFFF) << 32) + ((int64)(((OperaTrame)trame)->temps) & 0xFFFFFFFF))
	#define OPERA_TRAME_NUMERO(trame)         (((OperaTrame)trame)->identifiant & 0xffff)
	#define OPERA_TRAME_D0(trame)            ((((OperaTrame)trame)->identifiant >> 16) & 0x1ff)
	#define OPERA_TRAME_RETARD(trame)        ((((OperaTrame)trame)->identifiant >> 26) & 0x3f)
	#define OPERA_TRAME_TYPE_ACQUIS(trame)   ((((OperaTrame)trame)->code >> 26) & 0x1f)
	#define OPERA_TRAME_MASQUE_BB(trame)     ((((OperaTrame)trame)->code >> 20) & 0x3f)
	#define OPERA_TRAME_CODE_SYNCHRO(trame)  ((((OperaTrame)trame)->code >> 16) & 0x0f)
	#define OPERA_TRAME_SYNCHRO_NUM(trame)   (int)((OPERA_TRAME_TIMESTAMP(trame) / 100000) & 0x3FFF)
#else

	#define OPERA_TRAME_TIMESTAMP(trame)     ((((int64)(((Structure_trame_status *)trame)->status_opera.temps_pd_fort) & 0xFFFF) << 30) + ((int64)(((Structure_trame_status *)trame)->status_opera.temps_pd_faible) & 0x3FFFFFFF))
	#define OPERA_TRAME_NUMERO(trame)       (((Structure_trame_status *)trame)->identifiant & 0xffff)
	#define OPERA_TRAME_D0(trame)            ((Structure_trame_status *)trame)->status_opera.registre_x
	#define OPERA_TRAME_RETARD(trame)        ((Structure_trame_status *)trame)->status_opera.registre_retard
	#define OPERA_TRAME_TYPE_ACQUIS(trame)  (((Structure_trame_status *)trame)->status_opera.code_acqui & 0x0f)
	#define OPERA_TRAME_MASQUE_BB(trame)     ((Structure_trame_status *)trame)->status_opera.masque_BB
	#define OPERA_TRAME_CODE_SYNCHRO(trame) (((Structure_trame_status *)trame)->status_opera.code_synchro & 0x0f)
	#define OPERA_TRAME_SYNCHRO_NUM(trame) ((((OperaTrame)trame)->identifiant >> 16) & 0x3fff)

#endif

#endif /* OPERA_SUPPORT */

#ifdef CALI_SUPPORT
/* .................................. CALI .................................. */

/*
Niveau 1: Boite CALI
Niveau 2: RedPitaya en mode CALI seul
Niveau 3: RedPitaya avec linux, transfert BRAM en 64 bits
Niveau 4: RedPitaya avec linux, transfert BRAM en 32 bits
*/

typedef enum {
	CALI_REG_SEL = 0,
	CALI_REG_RUN,
	CALI_REG_TIMEOUT,
	CALI_REG_TAILLE,
	CALI_REG_D0,
	CALI_REG_ADC,
	CALI_REG_MOYENNE,
	CALI_REG_SORTIE,
	CALI_REG_MODE,
	CALI_REG_STATUS
} CALI_REG;

#define CALI_RESET   (hexa)0x00000020

#define CALI_START   (hexa)0x00000001
#define CALI_STOP    (hexa)0x00000002

#define CALI_EXTERNE (hexa)0x00000000
#define CALI_INDEX   (hexa)0x00010000
#define CALI_COUNTER (hexa)0x00020000

typedef enum {
	CALI_DATA_BB = 0,
	CALI_DATA_INDEX,
	CALI_DATA_STAMP,
	CALI_NBDATAMODES
} CALI_DATAMODE;
REPARTITEUR_VAR char *CaliCodesDataMode[CALI_NBDATAMODES+1]
#ifdef REPARTITEURS_C
= { "numeriseur", "index", "timestamp", "\0" }
#endif
;
REPARTITEUR_VAR hexa CaliModeCode[CALI_NBDATAMODES]
#ifdef REPARTITEURS_C
 = { CALI_EXTERNE, CALI_INDEX, CALI_COUNTER }
#endif
;
REPARTITEUR_VAR char CaliRepDataMode[CALI_NBDATAMODES]
#ifdef REPARTITEURS_C
= { 0, REPART_MODE_INDEX, REPART_MODE_RAMPE }
#endif
;
REPARTITEUR_VAR char *CaliStatusBitTexte[9]
#ifdef REPARTITEURS_C
= { "Erreur lecture FIFO", "Erreur ecriture FIFO", "FIFO carrement pleine", "FIFO carrement vide",
	"FIFO presque pleine", "FIFO presque vide",    "Saturation ADC",        "Canal actif",         "\0" }
#endif
;


#define CALI_STATUS_READ_ERROR  (hexa)0x01
#define CALI_STATUS_WRITE_ERROR (hexa)0x02
#define CALI_STATUS_FULL        (hexa)0x04
#define CALI_STATUS_EMPTY       (hexa)0x08
#define CALI_STATUS_HALF_FULL   (hexa)0x10
#define CALI_STATUS_HALF_EMPTY  (hexa)0x20
#define CALI_STATUS_OVERFLOW    (hexa)0x40
#define CALI_STATUS_ENABLED     (hexa)0x80

typedef struct {
	int64 stamp;
//	byte nb_d2;
//	shex id;
//	byte version;
	hexa id;
	byte status[4];
} TypeCaliEntete;

#define CALI_TAILLE_ENTETE sizeof(TypeCaliEntete)
#define CALI_DONNEES_NB 720
#define CALI_TAILLE_DONNEES (CALI_DONNEES_NB * sizeof(TypeDonnee))
// sizeof(TypeCaliTrame) == CALI_TAILLE_ENTETE + CALI_TAILLE_DONNEES

typedef struct {
	TypeCaliEntete hdr;
	TypeDonnee valeur[CALI_DONNEES_NB];
} TypeCaliTrame,*CaliTrame;

#define CALI_TRAME_NUMERO(trame) ((((CaliTrame)trame)->hdr.id >> 8) & 0xFFFFFF)
#define CALI_TRAME_TIMESTAMP(trame) (((CaliTrame)trame)->hdr.stamp)

REPARTITEUR_VAR TypeCaliEntete CaliEnteteLue;
REPARTITEUR_VAR hexa CaliTrameLue;
REPARTITEUR_VAR byte CaliVersionLue;
REPARTITEUR_VAR byte CaliSelectMsg[32];

#endif /* CALI_SUPPORT */

/* .................................. IPE ................................... */

/* Version 1 (IPE_MESSAGES_V1):
	message SLT: 'S' <var> <valeur>
	message FLT: 'F' <flt#> 1 <nb fibres> { <selection> }
	             'F' <flt#> 2 <nb fibres> { <BB_mode> }
	             'F' <flt#> 3 <SW_FIFO_ref>
	message BB : 'D' <flt#> <adrs> <ssadrs> <valeur>
    -------------------------------------------------------- 
   Version 2 (IPE_MESSAGES_V2):
	message SLT: 'S' <var> <valeur>
	message FLT: 'F' 'S' <nb_fibres> { <mode> <selection> }
				 'F' 'U' <nb_flt> { <UDPfifo#> }
	message BB : 'D' <flt#> <adrs> <ssadrs> <valeur>
     -------------------------------------------------------- 
   Version 3 (IPE_MESSAGES_V3): voir doc IPE du 14.05.12
 
 */

/* modes, version detaillee:
typedef enum { IPE_BB1 = 1, IPE_BB2_1, IPE_BB2, IPE_BB3, IPE_VETO, IPE_NBADCMODES } IPE_ADCMODE;
REPARTITEUR_VAR char *IpeCodesAdcMode[IPE_NBADCMODES+1]
#ifdef REPARTITEURS_C
 = { "nul", "BB1", "BB2-1", "BB2", "BB3", "veto", "\0" }
#endif
;
REPARTITEUR_VAR short IpeVpn[IPE_NBADCMODES]
#ifdef REPARTITEURS_C
 = { 0, 3, 4, 6, 5, 2 }
#endif
;
*/

#define IPE_MESSAGES_V3

#ifdef IPE_MESSAGES_V2
	#define IPE_TYPE_SEL 'S'
	#define IPE_TYPE_UDP 'U'
	#define IPE_DEST_SLT 'S'
	#define IPE_DEST_FLT 'F'
	#define IPE_DEST_BB  'D'
#endif

#define IPE_CHASSIS_MAX 4 // ajoute le 04.12.20
#define IPE_FLT_SIZE  6
#define IPE_FLT_MAX  24
#define IPE_FIBRES_PARMOT 4

#define IPE_FIBRE_REGS (((IPE_FLT_SIZE - 1) / IPE_FIBRES_PARMOT) + 1)
#define IPE_ENTREE_MAX (IPE_FLT_SIZE * IPE_FLT_MAX)

#define IPE_PORT(repart,entree) (entree + repart->r.ipe.of7)
#define IPE_FLT_ABS(repart,flt) (flt + repart->r.ipe.decale)
#define IPE_FLT_REL(repart,flt) (flt - repart->r.ipe.decale)
#define IPE_FLT_NUM(entree) (entree / IPE_FLT_SIZE)
#define IPE_FLT_FBR(entree,num) (entree - (num * IPE_FLT_SIZE))
#define IPE_FLT_NBUSED(repart) (((repart->nbentrees - 1) / IPE_FLT_SIZE) + 1)
// #define IPE_FBR_NUM(flt,fbr) ((flt * IPE_FLT_SIZE) + fbr)
#define IPE_ENTREE(flt,fbr) ((flt * IPE_FLT_SIZE) + fbr)
#define IPE_CODE_ENTREE(repart,entree) 'A'+(entree / IPE_FLT_SIZE)+repart->r.ipe.decale,(entree - ((entree / IPE_FLT_SIZE) * IPE_FLT_SIZE))+1
#define IPE_CODE_FIBRE(flt,fbr) 'A'+flt,fbr+1

#define IPE_SLT_CONTROL 0xA80000
#define IPE_SLT_PIXBUS  0xA80028
#define IPE_SLT_FIFOCSR 0xE00010

#define IPE_FLT_CONTROL  0x04
#define IPE_FLT_ACCES_BB 0x18
#define IPE_FLT_DELAIS   0x24
#define IPE_FLT_SELECT   0x2C

#define IPE_REG_WRITE "KWA"
#define IPE_FLT_WRITE "KWF"
#define IPE_SLT_WRITE "KWC"
#define IPE_SLT_CONFG "KWL"

#define IPE_BBWRITE_AUTO IPE_REG_WRITE"_sendBBCmd_0x%02X_0x%02X_0x%02X_0x%02X_FLT_%d_FIBER_%d"
#define IPE_FIFO_START "startFIFO"
#define IPE_FIFO_STOP  "stopFIFO"
#define IPE_BBSTS_ENABLE "FLTsendBBstatusMask"

typedef enum {
	IPE_SLT_RESET = 0,
	IPE_SLT_FPGA,
	IPE_SLT_START,
	IPE_SLT_STOP,
	IPE_SLT_EXEC,
	IPE_SLT_CLOSE,
	IPE_SLT_RELOAD,
	IPE_SLT_IP_RESTART,
	IPE_SLT_NBCMDES
} IPE_SLT_CMDE;
REPARTITEUR_VAR char *IpeSltCmde[IPE_SLT_NBCMDES+1]
#ifdef REPARTITEURS_C
= { "init", "chargeBBFile", "startStreamLoop", "stopStreamLoop", "coldStart",
	"reset", "reloadConfigFile", "restartKCommandSockets", "\0" }
#endif
;

#define IPE_BOURRE_QUANTUM 6
typedef enum {
	IPE_BOURRE_FLT = 1,
	IPE_BOURRE_FIBRE,
	IPE_BOURRE_NBMODES
} IPE_BOURRE_MODE;
#define IPE_BOURRE_CLES "neant/flt/fibre"

#define IPE_PORTS_BB "chassis/samba"

#define IPE_TRAME_STS      (shex)0xFFFF
#define IPE_TRAME_BB       (shex)0xFFFE
#define IPE_TRAME_CRATE    (shex)0xFFD0

typedef union {
	byte fibre[IPE_FIBRES_PARMOT];
	hexa total;
} TypeIpeFibreReg;

typedef struct {
	union {
		struct { byte modeFLT,modeBB,enable,speciaux; } partiel;
		hexa complet;
	} controle;
	hexa fibres_fermees;
	byte branchee,active,enabled;
	void *repart; int l0; // pour le cliquodrome
} TypeIpeFlt;

REPARTITEUR_VAR char IpeFltNom[IPE_FLT_MAX][32];
REPARTITEUR_VAR Panel pIpeStatus;

typedef enum {
	IPE_BB1 = 1,
	IPE_BB2,
	IPE_NBADCMODES
} IPE_ADCMODE;
REPARTITEUR_VAR char *IpeCodesAdcMode[IPE_NBADCMODES+1]
#ifdef REPARTITEURS_C
 = { "nul", "BB1", "BB2", "\0" }
#endif
;
REPARTITEUR_VAR short IpeVpn[IPE_NBADCMODES]
#ifdef REPARTITEURS_C
 = { 1, 3, 6 }
#endif
;
REPARTITEUR_VAR byte IpeDelais[IPE_NBADCMODES]
#ifdef REPARTITEURS_C
 = { 0x00, 0x73, 0x75 }
#endif
;

typedef enum {
	IPE_DATA_BB = 0,
	IPE_DATA_INDEX,
	IPE_DATA_STAMP,
	IPE_DATA_MIXTE,
	IPE_NBDATAMODES
} IPE_DATAMODE;
REPARTITEUR_VAR char *IpeCodesDataMode[IPE_NBDATAMODES+1]
#ifdef REPARTITEURS_C
= { "numeriseurs", "index", "timestamp", "mixte", "\0" }
#endif
;
REPARTITEUR_VAR char IpeRepDataMode[IPE_NBDATAMODES]
#ifdef REPARTITEURS_C
 = { 0, REPART_MODE_INDEX, REPART_MODE_RAMPE, REPART_MODE_IPE }
#endif
;

/* projet IPE initial:
#define IPE_FIFO_MAX 20
typedef struct {
	char  flt;
	char  fibre;
	short reserve;
	shex val[140];
} TypeIpeBBstsPrevu;
typedef struct {
	char selection[IPE_FLT_MAX][IPE_FLT_SIZE];
	char udp_path[IPE_FLT_MAX];
	hexa ip[IPE_FIFO_MAX];
} TypeIpeSltStsPrevu;
typedef struct {
	int64 stamp;
	short nbdetec; // so-called frames
	short nbstamps;
	hexa liste_flt;
	short reserve;
} TypeIpeEntete;
#define IPE_TAILLE_ENTETE 4 + sizeof(TypeIpeEntete)
#define IPE_DONNEES_NB 717
typedef struct {
	TypeIpeEntete hdr;
	TypeDonnee val[IPE_DONNEES_NB];
} TypeIpeDonnees;
*/

typedef struct {
	hexa stamp_haut;   // en int64, la valeur est "int-swappee" (donc pas de 'int64 stamp;')
	hexa stamp_bas;
	hexa temps_seconde;
	hexa dim;          // nombre d'octets = sizeof(TypeIpeCrateSts)
	// SLT registers:
	uint32_t SLTTimeLow;       // the according SLT register
	uint32_t SLTTimeHigh;      // the according SLT register
	uint32_t d0;               // contains d0, previously in cew: registre_x, =20  
	uint32_t pixbus_enable;
	// software (ipe4reader) status:
	uint32_t micro_bbv2;       // previously in cew: micro_bbv2 (BBv2/2.3/3 programming), means 'prog_status'
	uint32_t error_counter;
	uint32_t internal_error_info;
	uint32_t ipe4reader_status;
	uint32_t version;          // _may_ be useful in some particular cases (version of C code/firmware/hardware?) 
	uint32_t numFIFOnumADCs;   // numfifo (& 0xffff0000)  and total number of ADCs in the UDP data packets (&0xffff)
	uint32_t maxUDPSize;       // variable UDP packet size: this is the size dedicated to this FIFO
} TypeIpeCrateSts;

#define IPE_BBSTS_LNGR (((_nb_mots_status_bbv2 - 1) / 2) + 1)
typedef struct {
	hexa    dim;
	union {
		struct {
			byte   type,crate;
			byte   flt,fbr;
		} b;
		hexa complete;
	} source;
	hexa    inutile;
	shex  val[IPE_BBSTS_LNGR];
} TypeIpeBBsts;
#define IPE_BBSTS_HDR (sizeof(TypeIpeBBsts) - (IPE_BBSTS_LNGR * sizeof(shex)))

#define IPE_DONNEES_NB 720
#define IPE_TAILLE_DONNEES (IPE_DONNEES_NB * sizeof(TypeDonnee))
typedef struct { TypeDonnee val[IPE_DONNEES_NB]; } TypeIpeDonnees;

#define IPE_TAILLE_ENTETE 4
typedef struct {
	shex type;
	shex nb_d3;
	union {
		TypeIpeDonnees  donnees;
		TypeIpeCrateSts sts_crate;
		TypeIpeBBsts    sts_bb;
	} p;
} TypeIpeTrame,*IpeTrame;

#define IPE_TRAME_NUMERO(trame)       ((IpeTrame)trame)->type
#define IPE_TRAME_SYNCHRO_NUM(trame)  ((IpeTrame)trame)->nb_d3
// malheureusement, en int64 la valeur est "int-swappee": #define IPE_TRAME_TIMESTAMP(trame) (((IpeTrame)trame)->p.sts_crate.stamp)
// bretelle provisoire: #define IPE_TRAME_TIMESTAMP(trame) ((int64)(((IpeTrame)trame)->p.sts_crate.temps_seconde) * 100000)
//+ #define IPE_TRAME_TIMESTAMP(trame)    (((int64)(((IpeTrame)trame)->p.sts_crate.stamp_haut) << 32) | (int64)(((IpeTrame)trame)->p.sts_crate.stamp_bas) & 0xFFFFFFFF)
#define IPE_TRAME_TIMESTAMP(trame)    (((int64)(((IpeTrame)trame)->p.sts_crate.stamp_haut) << 30) | ((int64)(((IpeTrame)trame)->p.sts_crate.stamp_bas) & 0x3FFFFFFF))

/* ................................ ARCHIVE ................................. */

typedef enum {
	ARCH_FMT_EDW1 = 0,
	ARCH_FMT_EDW2,
	ARCH_FMT_EDW0,
	ARCHIVE_FMTNB
} ARCH_FORMAT;

/* ................................. SAMBA .................................. */

#define SMB_C 'c' /* connection retour vers le maitre */
#define SMB_N 'n' /* commande numeriseur              */
#define SMB_M 'm' /* marche (demarre l'acquisition)   */
#define SMB_A 'a' /* arret (stoppe l'acquisition)     */
#define SMB_E 'e' /* trigger declenche                */

#define SMB_CONNECT SMB_C, 0, 0     // valeur = port
#define SMB_NUMER   SMB_N           // mode,adrs,ssadrs,nb,[valeur]
#define SMB_MARCHE  SMB_M, 0, 0     // valeur = { validite (secondes) | stampmini }
#define SMB_ARRET   SMB_A, 0, 0, 0

typedef enum {
	SMB_NUMER_AUTO_RESS = 0,
	SMB_NUMER_AUTO_ADRS,
	SMB_NUMER_MANU_RESS,
	SMB_NUMER_MANU_ADRS,
	SMB_NUMER_NBMODES
} SMB_NUMER_MODE;

/* ............................... Structures ............................... */

typedef struct {
	union {
		struct { short d2,d3; } cluzel;
		struct { int mode; short d0; byte retard,d2d3; char clck,sync; int lngr_echant; } opera;
		struct { char clck; byte sel; } cali;
		struct { short maxentrees,voies_par_numer; char clck; } eth;
		struct {
			shex chassis; short fifo; hexa masque;
			short d0; char clck,ports_BB; byte bourrage; short fltnb,decale;
			byte modeBB[REPART_ENTREES_MAX];
			char fpga[80]; short of7; // of7 en nb d'entrees
			TypeIpeFlt *flt; hexa ctl_defaut[IPE_FLT_MAX]; byte retard[REPART_ENTREES_MAX];
		} ipe;
		struct { char fmt; } f;
		struct { float horloge; /* int lngr_echant; */ } samba;
	};
} TypeRepartRegl;

typedef struct {
	char fibre[FIBRE_NOM]; /* nom de la liaison avec le numeriseur */
	TypeAdrs adrs;
	char utilisee; /* positionne par RepartiteursSelectionneVoiesLues       */
	char voies;    /* == vpn, fixe par exemple dans RepartiteurBrancheIdent */
} TypeRepartEntree;

typedef struct {
	int port; int max_udp;
	char user[REPART_LOGIN_NOM],pswd[REPART_LOGIN_NOM];
} TypeRepartAccesIp;

typedef struct {
	union { int path; FILE *file; } o;
	union { off_t of7; long off; } d;
	short partition;
	char started;
} TypeRepartAccesArch;

#define REPART_RESERVOIR_MAX 8
typedef struct {
	char nom[REPART_NOM];
	char code_rep[5];
	int  type;                     /* index dans ModeleRep[]                             */
	int  rep;                      /* index dans le tableau                              */
	char famille;                  /* voir RepartFamilleListe[FAMILLE_NB+1]              */
	char revision;                 /* niveau de realisation dans cette famille           */
	char interf;                   /* voir InterfaceListe[INTERF_NBTYPES+1]              */
	char valide;                   /* faux si acces impossible ou deconseille            */
	char present;                  /* vrai si acces verifie                              */
	char local;                    /* vrai si utilise pour acces a un bolo local         */
	char utile;                    /* vrai si doit etre utilise pour la lecture          */
	char actif;                    /* vrai si participe effectivement a la lecture       */
	char candidat;                 /* vrai si valide, actif et a des voies a lire        */
	char avec_gene;                /* vrai si au moins une voie est generee              */
	char charge_fpga;              /* vrai si doit charger le FPGA de ses numeriseurs    */
	char status_fpga;              /* etat du chargement du FPGA de ses numeriseurs      */
	char bb;                       /* doit gerer le mode des numeriseurs (acquis/ident)  */
	char en_route;                 /* vrai si on lui demande des donnees                 */
	char arrete;                   /* vrai si n'envoie plus de donnee                    */
	char fifo_pleine;              /* vrai si FIFO pas videe                             */
	char recopie;                  /* vrai si deja lu par autre Acquis[]                 */
#ifdef REPART_SELECTION
	char select;                   /* vrai si selectionne pour init/reset                */
#endif
	char D3trouvee;                /* vrai si au moins une synchro D3 a ete trouvee      */
//	char event;                    /* vrai ne transmet que des evts (sinon, du stream)   */
	char categ;                    /* type des numeriseurs (standard,BB,stamp,..)        */
//	char sans_donnee;              /* n'envoie que des status, et pas de donnee          */
	char mode;                     /* type des donnees (selon mode famille-dependant)    */
	char check;                    /* verification des donnees (selon mode ci-dessus)    */
	int sat;                       /* acquisition a utiliser                             */
	TypeIpAdrs adrs;               /* adresse dans le media: #carte PCI, adresse IP, ... */
	lhex masque;                   /* bits de donnees utiles                             */
	lhex zero,extens;
	char parm[MAXFILENAME];        /* parametre lu (decode en <numero> si PCI)           */
	char fichier[MAXFILENAME];     /* fichier de definition ("\0" si liste 1er niveau)   */
	char maintenance[MAXFILENAME]; /* fichier de maintenance interface-dependant         */
	TypeRepartRegl r;              /* reglages du repartiteur                            */
	TypeRepartProc i;              /* initialisation du repartiteur                      */
	TypeRepartProc d;              /* demarrage du repartiteur                           */
	union {                        /* moyens d'acces au repartiteur                      */
		// struct { int ecr,lec; TypeSocket skt_sortie,skt_entree; TypeRepartAccesIp acces; } ip;
		struct { int max_udp; char user[REPART_LOGIN_NOM],pswd[REPART_LOGIN_NOM]; } ip;
		struct { PciBoard port; int page_prgm; Ulong bitD2D3; } pci;
		struct { char bin; TypeRepartAccesArch *a; } f;
	} p;
	struct {
		short valeur;      /* taille d'une valeur transmise                   */
		short entete;      /* taille de l'entete                              */
		short totale;      /* taille de la structure d'accueil                */
		short trame;       /* taille d'une trame non terminale                */
	} dim;
	TypeMedia ecr,lec;
	int32 octets,maxdonnees;
	lhex  *fifo32;
	lhex  *data32;
	TypeDonnee  *data16;
	int32 top,bottom;
	int32 depilable,depile;
	struct {
		Ulong *reservoir;
		int32 reserve,fond,capacite;
	} depot[REPART_RESERVOIR_MAX];
	short depot_ancien,depot_recent,depot_nb;
	int64 acces_demandes,acces_remplis,acces_vides;
	int   serie_vide,passages;
	int64 dernier_vide,dernier_rempli;
	int64 donnees_recues;
//	short vi0;                        /* index de la premiere voie identifiee (VoieIdent)       */
	short voies_par_numer;            /* nombre de voies par numeriseur                         */
	short premiere_donnee;            /* index de la premiere voie transmise (SambaEchantillon) */
	short nbdonnees;                  /* nombre total de donnees transmises par echantillon     */
//	short donnee[REPART_VOIES_MAX];   /* liste des voies lues                                   */
	int   nbident;                    /* nombre de voies identifiees (meme ordre!)              */
	TypeADU ident[REPART_VOIES_MAX];  /* liste des identifieurs, des voies si direct            */
	short maxentrees;                 /* nombre effectif de numeriseurs transmissibles          */
	int   nbentrees;                  /* dimension de la liste ci-dessous (0 si inactif)        */ // <int> car utilise via DESC_STR_SIZE
	TypeRepartEntree *entree;         /* liste des numeriseurs connectes                        */
	union {
		struct {
			short taille_derniere;    /* taille d'une trame terminale                    */
			shex  num_sts;            /* numero de trame si status                       */
			short duree_trame;        /* nombre d'ech./voie dans une trame               */
			int64 stampD3;            /* increment stamp entre D3                        */
			int64 inc_stamp;          /* increment stamp si stamp change                 */
			int   tramesD3;           /* numero max [Opera]/nb [Cali] trame entre D3     */
			int   dim_D3;             /* nb total de donnees attendues entre D3          */
			int   lu_entre_D3;        /* nb total de donnees lues entre D3               */
			int   relance;            /* nb trames a redemander (cde P) entre 2 relances */
			int64 stamp0,stamp1,stampEnCours,stampDernier,stampPerdus,recu;
			int   num0,num1,numAVenir;
			int32 saut;
			char  en_defaut,synchro_vue;
			int64 stamp_redemande; int trame_redemandee;
			int64 appel_perdant;
			int   manquant,depile;
			int64 duree_vide,duree_inutile; int nb_vide,nb_inutile;
			int   err_cew,err_synchro,err_stamp;
		} ip;
		struct { char en_cours; } pci;
		struct { char mode; int lu; char terminee; } f;
	} s;                      /* informations d'etat                              */
	char simule;
	struct {
		int64 intervalle,prochaine;
		int64 timestamp,sample;
		int num_trame,max;
		char started;
	} gene;
	// float temps_precedent;
	char date_status[12]; char status_demande,status_relu;
} TypeRepartiteur;

typedef enum {
	REPART_IGNORE = 0,
	REPART_ABSENT,
	REPART_PRESENT,
	REPART_ACCES,
	REPART_DISPO,
	REPART_NBETATS
} REPART_ETATS;

/* ................................ Variables................................ */

#define PERIMETRE_AVEC_PLANCHE

#ifndef PERIMETRE_AVEC_PLANCHE
REPARTITEUR_VAR Panel pRepartLocal;
#endif
#ifdef REPART_SELECTION
REPARTITEUR_VAR Panel pRepartSelecte;
#endif

REPARTITEUR_VAR char *RepartEtatListe[REPART_NBETATS+1]
#ifdef REPARTITEURS_C
 = { "ignore","absent", "existe", "pret", "operationnel", "\0" }
#endif
;
REPARTITEUR_VAR char RepartEtat[REPART_MAX];

REPARTITEUR_VAR char RepartiteurFichierLu[MAXFILENAME];
REPARTITEUR_VAR TypeRepartiteur Repart[REPART_MAX],RepartLu;
REPARTITEUR_VAR char *RepartListe[REPART_MAX+1];
REPARTITEUR_VAR int   RepartNb;
REPARTITEUR_VAR int   RepartNbActifs;
REPARTITEUR_VAR TypeRepartiteur *RepartSeul;
REPARTITEUR_VAR TypeRepartiteur *RepartConnecte;
REPARTITEUR_VAR int   RepartIpeNb;
REPARTITEUR_VAR char  RepartiteurValeurEnvoyee[REPART_MAXMSG];
REPARTITEUR_VAR struct {
	short valeur;
	TypeTamponDonnees tampon;
} RepartConnexionEnCours[REPART_VOIES_MAX];
REPARTITEUR_VAR Panel pRepartDonnees,pRepartChoix;
REPARTITEUR_VAR int RepartDemande,RepartEntreeDemandee;
REPARTITEUR_VAR char RepartDataMode; /* ATTENTION: doit valoir 0 si donnees = numeriseur */
REPARTITEUR_VAR char RepartDataErreurs;
REPARTITEUR_VAR char RepartAvecInit;

REPARTITEUR_VAR Cadre cRepartAcq;
REPARTITEUR_VAR Instrum iRepartRafraichi;
REPARTITEUR_VAR char RepartiteursASauver;

REPARTITEUR_VAR int   RepartD3Attendues;       /* nombre de synchros sur D3 a attendre   */
REPARTITEUR_VAR int   RepartTramesDebug;       /* nombre de synchros sur D3 a attendre   */

#define UDP_BLOC_LNGR 2000
REPARTITEUR_VAR float RepartUdpRelance;        /* intervalle de relance UDP (sec)        */
REPARTITEUR_VAR int   RepartNumLibre[FAMILLE_NB];

REPARTITEUR_VAR struct {
	int64 date,copie,delai;
	int64 vide;           /* nombre d'acces avec FIFO vide          */
	int   depilees;
	int   halffull,fullfull;
} RepartPci;

REPARTITEUR_VAR struct {
	int64 acces;           /* nombre d'acces IP                      */
	int64 traitees;        /* nombre d'acces FIFO utiles             */
	int64 perdues;         /* nombre d'acces FIFO perdus (au moins)  */
	int64 vide;            /* nombre d'acces IP sans donnee          */
	int   reduc;           /* total manque taille trames             */
	int   depilees;
} RepartIp;
REPARTITEUR_VAR int   RepartIpEssais,RepartIpRecues,RepartIpErreurs;
REPARTITEUR_VAR int   RepartIpNiveauPerte;
REPARTITEUR_VAR int   RepartIpEchappees;
REPARTITEUR_VAR char  RepartIpCorrection;      /* redemande aux BO les trames manquantes */
REPARTITEUR_VAR int64 RepartIpOctetsLus;

/* ... Variantes selon la famille ... */
REPARTITEUR_VAR ArgDesc RepartCluzelDesc[]
#ifdef REPARTITEURS_C
 = {
	{ "d2", DESC_SHORT &(RepartLu.r.cluzel.d2), 0 },
	{ "d3", DESC_SHORT &(RepartLu.r.cluzel.d3), 0 },
	DESC_END
}
#endif
;
REPARTITEUR_VAR ArgDesc RepartOperaDesc[]
#ifdef REPARTITEURS_C
 = {
	{ "d0",      DESC_SHORT                    &(RepartLu.r.opera.d0), 0 },
	{ "d2d3",    DESC_KEY CODES_SYNCHRO,       &(RepartLu.r.opera.d2d3), 0 },
	{ "retard",  DESC_BYTE                     &(RepartLu.r.opera.retard), 0 },
	{ "mode",    DESC_LISTE OperaCodesAcquis,  &(RepartLu.r.opera.mode), 0 },
	{ "horloge", DESC_KEY CODES_ESCLAVE,       &(RepartLu.r.opera.clck), 0 },
	{ "synchro", DESC_KEY CODES_ESCLAVE,       &(RepartLu.r.opera.sync), 0 },
	DESC_END
}
#endif
;
REPARTITEUR_VAR ArgDesc RepartCaliDesc[]
#ifdef REPARTITEURS_C
 = { { "horloge", DESC_KEY CODES_ESCLAVE,       &(RepartLu.r.cali.clck), 0 }, DESC_END }
#endif
;
REPARTITEUR_VAR ArgDesc RepartEthDesc[]
#ifdef REPARTITEURS_C
= {
	{ "horloge",         DESC_KEY CODES_ESCLAVE, &(RepartLu.r.eth.clck),            0 },
	{ "max_numer",       DESC_SHORT              &(RepartLu.r.eth.maxentrees),      0 },
	{ "voies_par_numer", DESC_SHORT              &(RepartLu.r.eth.voies_par_numer), 0 },
	DESC_END
}
#endif
;
REPARTITEUR_VAR int RepartIpeCtlLus,RepartIpeRetLus;
REPARTITEUR_VAR ArgDesc RepartIpeDesc[]
#ifdef REPARTITEURS_C
 = {
	{ "d0",         DESC_SHORT                    &(RepartLu.r.ipe.d0), 0 },
	{ "horloge",    DESC_KEY CODES_ESCLAVE,       &(RepartLu.r.ipe.clck), 0 },
	{ "bourrage",   DESC_KEY IPE_BOURRE_CLES,     &(RepartLu.r.ipe.bourrage), 0 },
	{ "fpga",       DESC_STR(80)                    RepartLu.r.ipe.fpga, 0 },
	{ "ports_BB",   DESC_KEY IPE_PORTS_BB,        &(RepartLu.r.ipe.ports_BB), 0 },
	{ "fifo",       DESC_SHORT                    &(RepartLu.r.ipe.fifo), 0 },
	{ "decalage",   DESC_SHORT                    &(RepartLu.r.ipe.decale), 0 },
	{ "flt",        DESC_SHORT                    &(RepartLu.r.ipe.fltnb), 0 },
	{ "masque_flt", DESC_HEXA                     &(RepartLu.r.ipe.masque), 0 },
	{ "controle",   DESC_HEXA_SIZE(RepartIpeCtlLus,IPE_FLT_MAX)        RepartLu.r.ipe.ctl_defaut, 0 },
	{ "retards",    DESC_BYTE_SIZE(RepartIpeRetLus,REPART_ENTREES_MAX) RepartLu.r.ipe.retard, 0 },
	DESC_END
}
#endif
;
REPARTITEUR_VAR ArgDesc RepartMediaDesc[]
#ifdef REPARTITEURS_C
 = {
	{ "nom",       DESC_STR(MEDIA_MAXNOM)      RepartLu.ecr.nom,  0 },
// trop tard:	{ "interface", DESC_LISTE InterfaceListe, &RepartLu.interf,      0 },
	DESC_END
}
#endif
;
REPARTITEUR_VAR ArgDesc RepartArchiveDesc[]
#ifdef REPARTITEURS_C
 = { { "format",  DESC_KEY CODES_FORMAT,         &RepartLu.r.f.fmt, 0 }, DESC_END }
#endif
;
REPARTITEUR_VAR ArgDesc RepartSambaDesc[]
#ifdef REPARTITEURS_C
 = { { "horloge", DESC_FLOAT                     &(RepartLu.r.samba.horloge), 0 }, DESC_END }
#endif
;
REPARTITEUR_VAR ArgDesc RepartNiDesc[]
#ifdef REPARTITEURS_C
 = { { "nb_voies", DESC_INT &(RepartLu.voies_par_numer), 0 }, DESC_END }
#endif
;

REPARTITEUR_VAR ArgDesc *RepartOptnFamDesc[]
#ifdef REPARTITEURS_C
	= { RepartCluzelDesc, RepartOperaDesc, RepartCaliDesc, RepartIpeDesc, RepartEthDesc, RepartMediaDesc, RepartArchiveDesc, RepartSambaDesc, RepartNiDesc, 0 }
#endif
; 

#ifdef AVEC_PCI
#define DEBUG_RAW
#endif
#ifdef DEBUG_RAW
#define MAXRAWDATA 4000
// #define MAXRAWDATA 100
REPARTITEUR_VAR int32 LectRawNext;
REPARTITEUR_VAR int64 LectNonNul;
REPARTITEUR_VAR int32 LectRaw[MAXRAWDATA];
#endif

char RepartiteurEcritNumeriseur(TypeRepartiteur *repart, short entree, byte *cmde, int lngr);
void IpeAutoriseFibre(TypeRepartiteur *repart, int entree, char autorise);
int NiSelecte1voie(TypeRepartiteur *repart, int vbb, int largeur_table);


void RepartiteursStructInit();
char RepartiteurModlLit(char *fichier);
int  RepartiteursLit(char *fichier);
void RepartiteurInit(int modl, TypeRepartiteur *repart, char interface);
int  RepartiteursInitTous(REPART_ETAPES reset, float echantillonage, Menu menu);
void RepartiteurVerifieParmsEtFIFO(TypeRepartiteur *repart, char *message);
void RepartiteurCodeEntree(TypeRepartiteur *repart, int l, char *code);
void RepartiteurDebranche(TypeRepartiteur *repart, int l);
int  RepartiteurAvecFibre(char *fibre, int *l);
int  RepartiteursSauve();
int  RepartiteursEcrit(char *fichier);
int  RepartDiffuse(Menu menu, MenuItem item);
void RepartiteursLocalise();
int  RepartiteurDemarre(Menu menu, MenuItem item);
int  RepartChargeFpga(char *explics);
void RepartiteurMajListes();
void RepartCompteDonnees(TypeRepartiteur *repart);
void RepartiteursSelectionneVoiesLues(char toutes, char log);
void RepartImprimeEchantillon(char std);
void RepartiteurAccepteEntrees(TypeRepartiteur *repart, int nb_bn);
void RepartiteursImprimeNumeriseurs();
char RepartiteurAdopte(TypeRepartiteur *repart, char verifie, char *explics);
int  RepartiteursVerifie(char toutes, char verifie, char log);
int  RepartiteursElection(char log);
int  RepartiteurRafraichi();
char RepartAjusteHorloge(float *frequence, float *relance);
INLINE char RepartIpVerifiePresence(TypeRepartiteur *repart, char *marge);
INLINE char RepartIpEcriturePrete(TypeRepartiteur *repart, char *marge);
INLINE void RepartIpEcritureFerme(TypeRepartiteur *repart);
void RepartIpRazCompteurs(TypeRepartiteur *repart);
void RepartiteurDemarreNumeriseurs(TypeRepartiteur *repart, NUMER_MODE mode, byte adrs, char log);
INLINE char RepartiteurEcritRessourceBB(TypeRepartiteur *repart, short entree, shex adrs, hexa ss_adrs, TypeADU valeur);
char RepartiteurDemarreTransmissions(TypeRepartiteur *repart, char log);
char RepartiteurArretTransmissions(TypeRepartiteur *repart, char avec_vidage, char log);
char RepartiteurSuspendTransmissions(TypeRepartiteur *repart, char log);
char RepartiteurStoppeTransmissions(TypeRepartiteur *repart, char log);
void RepartCompteCalagesD3();
char RepartiteurRepereD2(TypeRepartiteur *repart, lhex *val1, char log);
char RepartiteurRepereD3(TypeRepartiteur *repart, lhex *val1, char log);
INLINE char RepartVidePile(TypeRepartiteur *repart, char *pas_tous_secs, short *pleins, char log);

INLINE ssize_t RepartIpEcrit(TypeRepartiteur *repart, const void *buffer, size_t lngr);
INLINE ssize_t RepartIpLitTrame(TypeRepartiteur *repart, void *trame, size_t lngr);
INLINE void RepartIpTransmission(TypeRepartiteur *repart, char nature, int duree, char log);
INLINE void RepartIpLectureFerme(TypeRepartiteur *repart);
void RepartIpRazCompteurs(TypeRepartiteur *repart);
INLINE char RepartIpDepile1Donnee(TypeRepartiteur *repart, int32 *donnee);

char CluzelNouvelle(TypeRepartiteur *repart, int num, char categ);
void CluzelIcaFlashWrite(PciBoard board, int adrs, int val);
int  CluzelEnvoieBasique(PciBoard board, byte action, byte adrs, byte cmde, TypeADU valeur);
/* il aurait fallu: (.., byte action, byte adrs, byte ss_adrs, int valeur) */
INLINE int CluzelEnvoieCommande(PciBoard board, byte adrs, byte ss_adrs, TypeADU valeur, byte action);
char CluzelDepile1Donnee(TypeRepartiteur *repart, lhex *donnee, int *a_decompter, char log);

char OperaNouvelle(TypeRepartiteur *repart, char *adrs, char categ, int mode, byte retard, char horloge);
void OperaRecopieStatus(Structure_trame_status *trame, TypeRepartiteur *repart);
void OperaDimensions(int mode, shex *d1, int *voies_par_mot, short *vpn);
int  OperaSelecteur(TypeRepartiteur *repart, int selection, float echantillonage, byte *buffer, int *l, char *explics);
void RepartImprimeCompteurs(TypeRepartiteur *repart, char *prefixe, int lngr);
void OperaDump(byte *buffer, int nb, char hexa);

char CaliNouvelle(TypeRepartiteur *repart, char *adrs, char categ);
INLINE void CaliSwappeTrame(TypeRepartiteur *repart, CaliTrame trame, ssize_t nb);
void CaliDump(TypeRepartiteur *repart, byte *buffer, ssize_t nb, char hexa);
void CaliRecopieStatus(CaliTrame trame, TypeRepartiteur *repart);
int  CaliSelectCanaux(TypeRepartiteur *repart, byte selection, float echantillonage, byte *buffer, int *t);
int  CaliSelecteur(TypeRepartiteur *repart, int selection, float echantillonage, byte *buffer, int *l);

INLINE int SmbEcrit(TypeRepartiteur *repart, byte cmde, byte c1, byte c2, shex valeur);

char NiDepile1Donnee(TypeRepartiteur *repart, lhex *donnee);

// int  IpeSelecteur(TypeRepartiteur *repart, int selection, float echantillonage, byte *buffer, int *l);
char IpeEcritFlt(TypeRepartiteur *repart, int flt, int reg, hexa val, char log);
int  IpeSelectCanaux(TypeRepartiteur *repart, int nbregs, byte *selection, float echantillonage, byte *buffer, int *t);
void IpeRecopieStatus(IpeTrame trame, TypeRepartiteur *repart, char avec_pixbus);
void IpeDump(byte *buffer, int nb, char hexa);

int32 RepartGenereEvt(TypeRepartiteur *repart, int voie, int gene, float ldb);
int RepartiteurChercheIP();
void RepartiteurAppliquePerimetre();
int RepartiteurRedefiniPerimetre(Menu menu, MenuItem item);
int RepartiteursStatus();
int RepartiteurIpBasique(TypeRepartiteur *repart);
int RepartiteursClique(Figure fig, WndAction *e);
int RepartiteursAffiche(TypeRepartiteur *repart);
int RepartiteursCommande();

/* ................. variables statiques polluant l'editeur ................. */
#ifdef REPARTITEURS_C
static short RepartIpeFlt,RepartIpeFibre;
static hexa IpePixBus[IPE_CHASSIS_MAX];
static hexa IpeTempsLu,IpeErreursNb,IpeInternalInfo,IpeSWstatus,IpeTrameDim;
static hexa IpeStampMsb,IpeStampLsb;
static short IpeFifoNb,IpeAdcNb;
static struct {
	byte delai12;
	byte delai120;
	byte selection;
} IpeStatus[IPE_FLT_MAX][IPE_FLT_SIZE];
static char IpeRegEntete[3 * IPE_FLT_SIZE][16];

#ifdef A_REVOIR_PEUT_ETRE
static struct {
	char titre_pts[MODL_NOM+32];  /* (sert au panel etat)          */
	struct {
		int  a_lire;     /* Nombre de points a lire sur fichier      */
		int  lus;        /* Nombre de points deja lus sur fichier    */
		int  step;       /* Nombre de boucles a faire avant stockage */
		int  loop;       /* Nombre de boucles faites avant stockage  */
	} arch;
	char au_vol,sur_tampon;
} ArchVoieStatus[VOIES_MAX];    /* maxi dans un bloc */
#endif /* A_REVOIR_PEUT_ETRE */

#endif /* REPARTITEURS_C */

#endif /* REPARTITEURS_H */
