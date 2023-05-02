#define REPARTITEURS_C

#include <environnement.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <samba.h>
// #include <cmdes.h>
#include <repartiteurs.h>
#include <evenement.h>
#include <numeriseurs.h>
#include <gene.h>
#include <archive.h>
#include <objets_samba.h> /* pour Acquis->etat */
#include <banc.h>
#include <utils/claps.h>
#include <utils/decode.h>
#include <utils/dateheure.h>
#include <opium4/opium.h>

#ifdef AVEC_PCI
	//#include <PCI/plx.h>
	#include <PCI/IcaLib.h>
#endif
#define TRAME_STD 1440

static char RepartiteurFichierModeles[MAXFILENAME];

/* ... Listes de codes ... */
#ifdef NATURE_PAR_ENTREE
	typedef enum {
		REP_DATA_STREAM = 0,
	//	REP_DATA_MIXTE,
	//	REP_DATA_EVENTS,
		REP_DATA_PASVUE,
		REP_DATA_NBFMT
	} REP_DATA_FMT;
	static char *RepDataFmtNoms[REP_DATA_NBFMT+1] = { "stream", /* "mixte", */ "events", /* "inconnue", */ "\0" };
	#define REPART_FMT_CLES "stream/events"
#else
	typedef enum {
		REP_DATA_PASVUE = 0,
		REP_DATA_UTILISEE,
		REP_DATA_NBFMT
	} REP_DATA_FMT;
//-	static char *RepDataFmtNoms[REP_DATA_NBFMT+1] = { "inutile", "utilisee", "\0" };
#endif

static char *OperaEntreesBN[] = { "aucune", "entree 1", "entree 2", "2 entrees", "\0" };

/* ... Variantes selon l'interface ... */
	/* PCI */
static ArgDesc RepartRegDesc[] = {
	{ 0, DESC_SHEX &(RepartRegLu.adrs), 0 },
	{ 0, DESC_SHEX &(RepartRegLu.ssadrs), 0 },
	{ 0, DESC_SHEX &(RepartRegLu.valeur), 0 },
	DESC_END
};
static ArgStruct RepartRegsInitAS = { (void *)RepartLu.i.pci.reg, (void *)&RepartRegLu, sizeof(TypePciReg), RepartRegDesc };
static ArgStruct RepartRegsRazAS  = { (void *)RepartLu.d.pci.reg, (void *)&RepartRegLu, sizeof(TypePciReg), RepartRegDesc };
static ArgDesc RepartPciDesc[] = {
	{ "init",    DESC_STRUCT_SIZE(RepartLu.i.pci.nbregs,REPART_PCIREG_MAX) &RepartRegsInitAS, 0 },
	{ "start",   DESC_STRUCT_SIZE(RepartLu.d.pci.nbregs,REPART_PCIREG_MAX) &RepartRegsRazAS, 0 },
	DESC_END
};
	/* IP */
static ArgDesc RepartActnScriptDesc[] = {
	{ 0,         DESC_STR(MAXFILENAME)  RepartActnLue.fichier, 0 },
	{ "duree",   DESC_BYTE             &RepartActnLue.duree, 0 },
	DESC_END
};
static ArgDesc RepartActnSshDesc[] = {
	{ 0,         DESC_STR(MAXFILENAME)  RepartActnLue.fichier, 0 },
	{ "direct",  DESC_BOOL             &RepartActnLue.directe, 0 },
	DESC_END
};
static ArgDesc RepartActnXferDesc[] = {
	{ 0,         DESC_STR(MAXFILENAME)  RepartActnLue.fichier, 0 },
	{ "dest",    DESC_STR(MAXFILENAME)  RepartActnLue.dest, 0 },
	DESC_END
};
static ArgDesc *RepartActnParmDesc[] = { RepartActnXferDesc, RepartActnScriptDesc, RepartActnSshDesc, RepartActnScriptDesc, RepartActnScriptDesc, 0 };
static ArgDesc RepartActnDesc[] = {
	{ 0, DESC_KEY REPART_ACTIONS_CLES,    &RepartActnLue.type, 0 },
	{ 0, DESC_VARIANTE(RepartActnLue.type) RepartActnParmDesc, 0 },
	DESC_END
};
static ArgStruct RepartActnInitAS = { (void *)RepartLu.i.ip.actn, (void *)&RepartActnLue, sizeof(TypeIpActn), RepartActnDesc };
static ArgStruct RepartActnRazAS  = { (void *)RepartLu.d.ip.actn, (void *)&RepartActnLue, sizeof(TypeIpActn), RepartActnDesc };
static ArgDesc RepartIpDesc[] = {
	{ "type",    DESC_KEY MediaClesType,   &RepartLu.ecr.type, 0 },
	{ "port",    DESC_INT                  &RepartLu.ecr.port, 0 },
	{ "send",    DESC_INT                  &RepartLu.p.ip.max_udp, 0 },
	{ "user",    DESC_STR(REPART_LOGIN_NOM) RepartLu.p.ip.user, 0 },
	{ "pswd",    DESC_STR(REPART_LOGIN_NOM) RepartLu.p.ip.pswd, 0 },
	{ "init",    DESC_STRUCT_SIZE(RepartLu.i.ip.nbactn,REPART_IPACTN_MAX) &RepartActnInitAS, 0 },
	{ "start",   DESC_STRUCT_SIZE(RepartLu.d.ip.nbactn,REPART_IPACTN_MAX) &RepartActnRazAS, 0 },
	DESC_END
};
	/* Autres.. */
static ArgDesc RepartNatDesc[] = { DESC_END };
static ArgDesc RepartUsbDesc[] = { DESC_END };
static ArgDesc RepartFicDesc[] = { DESC_END };

static ArgDesc *RepartOptnIfDesc[]  = { RepartPciDesc, RepartIpDesc, RepartNatDesc, RepartUsbDesc, RepartFicDesc, 0 };

/* ... Description generale ... */
//       int RepartNumerLu[REPART_VOIES_MAX];
#define REPART_ENTREE_NOM NUMER_NOM + 1 + 7
char RepartEntreeLue[REPART_ENTREES_MAX][REPART_ENTREE_NOM];
//      ArgDesc RepartSrceNoms[]  = { { "numeriseurs", DESC_LISTE_SIZE(RepartLu.nbentrees,REPART_VOIES_MAX) NumeriseurListe, RepartNumerLu, 0 }, DESC_END };
		ArgDesc RepartSrceNoms[] = {
//			{ "numeriseurs", DESC_LISTE_SIZE(RepartLu.nbentrees,REPART_VOIES_MAX) NumeriseurListe, RepartNumerLu, 0 },
			{ "numeriseurs", DESC_STR_SIZE(REPART_ENTREE_NOM,RepartLu.nbentrees,REPART_ENTREES_MAX) RepartEntreeLue, 0 },
			{ "ident",       DESC_NONE }, // DESC_SHEX_SIZE(RepartLu.nbident,REPART_VOIES_MAX)  RepartLu.ident, 0 },
			DESC_END
		};
		ArgDesc RepartSrceIdent[] = { { "ident", DESC_SHEX_SIZE(RepartLu.nbident,REPART_VOIES_MAX) RepartLu.ident, 0 }, DESC_END };
		ArgDesc RepartSrceStamp[] = { DESC_END };
		ArgDesc RepartSrceFile[]  = { { "run",   DESC_STR_SIZE(REPART_ENTREE_NOM,RepartLu.nbentrees,REPART_ENTREES_MAX) RepartEntreeLue, 0 }, DESC_END };
		ArgDesc *RepartOptnSrceDesc[] = { RepartSrceNoms, RepartSrceIdent, RepartSrceStamp, RepartSrceFile, 0 };

static ArgDesc RepartDesc[] = {
	{ "adresse",      DESC_STR(MAXFILENAME)            RepartLu.parm,        "Adresse IP ou No de carte PCI ou nom de run" },
	{ "etat",         DESC_KEY REP_STATUTS_CLES,      &RepartLu.valide,      REP_STATUTS_CLES },
	{ "maintenance",  DESC_STR(MAXFILENAME)            RepartLu.maintenance, "Fichier definition les actions de chargement et demarrrage (init/start)" },
	{ "donnees",      DESC_NONE }, // DESC_LISTE RepartDonneesListe, &RepartLu.categ, 0 }, avec categ: int plutot que char
//	{ "ident",        DESC_SHEX_SIZE(RepartLu.nbident,REPART_VOIES_MAX) RepartLu.ident, 0 },
	{ "numerisation", DESC_KEY REPART_DONNEES_CLES,   &RepartLu.categ,       REPART_DONNEES_CLES },
	{ "envoi",        DESC_NONE }, // DESC_KEY REPART_FMT_CLES,       &RepartLu.event,       "type de donnees envoyees ("REPART_FMT_CLES")" },
	{ 0,              DESC_VARIANTE(RepartLu.categ)    RepartOptnSrceDesc,   0 },
	{ 0,              DESC_VARIANTE(RepartLu.famille)  RepartOptnFamDesc,    0 },
	{ 0,              DESC_VARIANTE(RepartLu.interf)   RepartOptnIfDesc,     0 },
	DESC_END
};
static ArgDesc RepartDescStandard[] = /* parms interface dans fichier 'maintenance' */ {
	{ "adresse",      DESC_STR(MAXFILENAME)            RepartLu.parm,        0 },
	{ "etat",         DESC_KEY REP_STATUTS_CLES,      &RepartLu.valide,      0 },
	{ "maintenance",  DESC_STR(MAXFILENAME)            RepartLu.maintenance, 0 },
	{ "numerisation", DESC_KEY REPART_DONNEES_CLES,   &RepartLu.categ,       0 },
	{ "envoi",        DESC_NONE }, // DESC_KEY REPART_FMT_CLES,       &RepartLu.event,       0 },
	{ 0,              DESC_VARIANTE(RepartLu.categ)    RepartOptnSrceDesc,   0 },
	{ 0,              DESC_VARIANTE(RepartLu.famille)  RepartOptnFamDesc,    0 },
	DESC_END
};

static short RepartLocal[REPART_MAX];
static int  RepartLocaux;
static char RepartiteurParmModifie;
// static int  RepartMem;
static int  RepartTraceHaut,RepartTraceLarg,RepartTraceVisu;
static char RepartDonneeNom[REPART_VOIES_MAX][16];
static char RepartDonneesAffiche,RepartCourbesAffiche;
static char RepartDonneesConstruit;
static char RepartNumerNom[REPART_VOIES_MAX][16];
static Graph RepartGraphEnCours;
static short RepartGraphVoie[REPART_VOIES_MAX];
static int  RepartGraphVoieNb;
static float RepartStatusHorloge[2];
static char RepartFpgaInter;
static int RepartEvtEmpile;

static int CluzelModele;

static shex OperaMasque;
static shex  OperaCodeEntrees;
static int   RepartVersionLogiciel;
#define REPART_INFOFPGA_DIM 32
static char  RepartInfoFpgaBB[REPART_INFOFPGA_DIM];
static int   OperaNbD3,OperaErreursCew,OperaErreursTimeStamp,OperaErreursSynchro;
static char  OperaParmQ;
static int   OperaModele;
static char OperaCde;
static char OperaTampon[16];

static int CaliModele;

static char *RepartHeure;
static Panel pRepartEtat,pRepartHeure;
static Menu  mRepartEtat;
static Cadre bRepartEtat;
static Panel pRepartStatus,pRepartIpeChoixEntree;
static Panel pRepartChercheIP;
static char RepartChercheIPnet[16]; static short RepartChercheIPdep,RepartChercheIParr;
static TypeFigDroite RepartLigneVert1  = { 0, 0, WND_RAINURES,  0x0000, 0x0000, 0x0000 };
static TypeFigDroite RepartLigneVert2  = { 0, 0, WND_REGLETTES, 0x0000, 0x0000, 0x0000 };
static TypeFigZone RepertIconeCntrl    = { 0, 0, WND_RAINURES,  0x7FFF, 0x7FFF, 0x7FFF };
static TypeFigZone RepertIconeRegistre = { 0, 0, WND_RAINURES,  0x7FFF, 0x7FFF, 0x7FFF };

typedef struct {
	short canaux;       /* nombre de voies transmises                            */
	short canal;        /* numero de voie en cours (index dans EvtStocke[RepartEvtEmpile].voie) */
	short tranche;      /* numero de tranche                                     */
	short tot;          /* nombre de valeurs deja lues                           */
} TypeTrigCourant;
static TypeTrigCourant TrigEnCours;

static char RepartiteurBrancheIdent(TypeRepartiteur *repart);
static int RepartEtatChange(Menu menu, MenuItem item);
static void RepartIpEnvoie(TypeRepartiteur *repart, byte *buffer, int l, char termine, char attente, char log);

static INLINE char CluzelRAZFifo(PciBoard pci, int fifo_dim, char secu, char log);
static int CluzelReset(TypeRepartiteur *repart, char avec_opium, char avec_reboot);
static INLINE char CluzelVidePile(TypeRepartiteur *repart, char *pas_tous_secs, short *pleins, char log);

static int OperaEntreesLimite(TypeRepartiteur *repart, shex *code_entrees);
static void OperaReset(TypeRepartiteur *repart, float echantillonage, char avec_opium);
static shex OperaDecodeSelecteur(TypeRepartiteur *repart, shex selecteur);
static int OperaCmdeFpga(TypeRepartiteur *repart, int fpga);
static int OperaCmdeQ(TypeRepartiteur *repart);
static void OperaCompteursTrame(TypeRepartiteur *repart);
static void OperaPrintReglages(TypeRepartiteur *repart, char *prefixe, int lngr);
static INLINE char OperaVidePile(TypeRepartiteur *repart, char *pas_tous_secs, short *pleins, char log);

static shex CaliLissage(shex diviseur0, char revision);
static void CaliCompteursTrame(TypeRepartiteur *repart);
void CaliDumpTrameStrm(TypeRepartiteur *repart, CaliTrame trame_cali, size_t taille_trame, int64 num, char *prefixe);
/* static INLINE */ char CaliVidePile(TypeRepartiteur *repart, char *pas_tous_secs, short *pleins, char log);

static void IpeSetupFibres(TypeRepartiteur *repart);
static void IpeBourreFlt(TypeRepartiteur *repart, int flt, char log, int lngr);
INLINE static void IpeEcritRessourceBB(TypeRepartiteur *repart, short entree, shex adrs, hexa ss_adrs, TypeADU valeur);
static void IpeCmdeFpga(TypeRepartiteur *repart, int fpga);
static void IpeTraduitProgBB(IpeTrame trame);
static void IpeCompteursTrame(TypeRepartiteur *repart);
/* static INLINE */ char IpeVidePile(TypeRepartiteur *repart, char *pas_tous_secs, short *pleins, char log);

static void NiTransmissionRaz(int largeur_table);
static void NiAjusteHorloge(TypeRepartiteur *repart, int bloc, int nbvoies, int largeur_table);
static INLINE char NiVidePile(TypeRepartiteur *repart, char *pas_tous_secs, short *pleins, char log);

static int ArchInit(TypeRepartiteur *repart, int l);
static int ArchOuvre(TypeRepartiteur *repart, int l);
static int ArchDemarre(TypeRepartiteur *repart, int l);
static void ArchFerme(TypeRepartiteur *repart, int l);
static void ArchRecommence(TypeRepartiteur *repart);
static INLINE char ArchLitBloc(TypeRepartiteur *repart, char *pas_tous_secs, short *pleins, char log);

static char RepartSchemaChange;

static int RepartEntreeChangee,RepartBBbranche;
static int RepartiteurUsage(Menu menu, MenuItem item);
static int RepartiteurChangeEntree(Menu menu, MenuItem item);
static Menu mRepartiteurClique;
static TypeMenuItem iRepartiteurClique[] = {
	{ "Usage repartiteur",  MNU_FONCTION RepartiteurUsage },
	{ "Modifier entree",    MNU_FONCTION RepartiteurChangeEntree },
	{ "Sauver Liste",       MNU_FONCTION RepartiteursSauve },
//+	{ "Ajouter numeriseur", MNU_FONCTION NumeriseurAjoute },
	{ "Fermer",             MNU_RETOUR },
	MNU_END
};

int LectAcqElementaire(NUMER_MODE mode);

#pragma mark ---------- modeles ----------
static void RepartModlAjouteMedia();
/* .......................................................................... */
/*                                M O D E L E S                               */
/* ========================================================================== */
void RepartModlStandard() {
	int i;
	/* Versions des repartiteurs PCI:
	 <0  : connexion directe (NOSTOS)
	 0   : version totalement disparue
	 1   : version P.Cluzel, 3 bolos x 3 voies statiques + status opionnel
	 1.1 : Carte SuperCluzel, par HD (plus ou moins 2.x-compatible) sans status
	 1.2 : idem, avec status et synchros esclaves
	 2.0 : chassis HD (cartes CC+IBB) via ICA ou MiG
	 */

	i = 0;

	/* strcpy(ModeleRep[i].nom,"proto Cluzel");
	strcpy(ModeleRep[i].code,"proto");
	ModeleRep[i].famille = FAMILLE_CLUZEL;
	ModeleRep[i].interface = INTERF_PCI;
	ModeleRep[i].revision = 0;
	ModeleRep[i].selecteur.type = REP_SELECT_NEANT;
	ModeleRep[i].selecteur.max = 0;
	ModeleRep[i].version = 0.1;
	ModeleRep[i].delai_msg = 3000;
	ModeleRep[i].horloge = 60.0;
	ModeleRep[i].diviseur0 = 10;
	ModeleRep[i].diviseur1 = 60;
	ModeleRep[i].masque = 0x3FFF;
	ModeleRep[i].max_numer = 3;
	ModeleRep[i].regroupe_numer = 0;
	ModeleRep[i].status = 0;
	i++; */
	strcpy(ModeleRep[i].nom,"Cluzel 3BB");
	strcpy(ModeleRep[i].code,"Cluzel");
	ModeleRep[i].famille = FAMILLE_CLUZEL;
	ModeleRep[i].interface = INTERF_PCI;
	ModeleRep[i].revision = 1;
	ModeleRep[i].selecteur.type = REP_SELECT_NEANT;
	ModeleRep[i].selecteur.max = 0;
	ModeleRep[i].version = 1.0;
	ModeleRep[i].delai_msg = 3000;
	ModeleRep[i].horloge = 60.0;
	ModeleRep[i].diviseur0 = 10;
	ModeleRep[i].diviseur1 = 60;
	ModeleRep[i].masque = 0x3FFF;
	ModeleRep[i].bits32 = 0;
	ModeleRep[i].max_numer = 3;
	ModeleRep[i].regroupe_numer = 0;
	ModeleRep[i].status = 1;
	i++;
	/* strcpy(ModeleRep[i].nom,"SuperCluzel sans status");
	strcpy(ModeleRep[i].code,"SCv1");
	ModeleRep[i].famille = FAMILLE_CLUZEL;
	ModeleRep[i].interface = INTERF_PCI;
	ModeleRep[i].revision = 2;
	ModeleRep[i].selecteur.type = REP_SELECT_NUMER;
	ModeleRep[i].selecteur.max = 1;
	ModeleRep[i].version = 1.1;
	ModeleRep[i].delai_msg = 3000;
	ModeleRep[i].horloge = 125.0;
	ModeleRep[i].diviseur0 = 21;
	ModeleRep[i].diviseur1 = 60;
	ModeleRep[i].masque = 0x3FFF;
	ModeleRep[i].bits32 = 0;
	ModeleRep[i].max_numer = 10;
	ModeleRep[i].regroupe_numer = 0;
	ModeleRep[i].status = 0;
	i++; */
	strcpy(ModeleRep[i].nom,"SuperCluzel");
	strcpy(ModeleRep[i].code,"SCv2");
	ModeleRep[i].famille = FAMILLE_CLUZEL;
	ModeleRep[i].interface = INTERF_PCI;
	ModeleRep[i].revision = 3;
	ModeleRep[i].selecteur.type = REP_SELECT_NUMER;
	ModeleRep[i].selecteur.max = 1;
	ModeleRep[i].version = 1.2;
	ModeleRep[i].delai_msg = 3000;
	ModeleRep[i].horloge = 125.0;
	ModeleRep[i].diviseur0 = 21;
	ModeleRep[i].diviseur1 = 60;
	ModeleRep[i].masque = 0x3FFF;
	ModeleRep[i].bits32 = 0;
	ModeleRep[i].max_numer = 10;
	ModeleRep[i].regroupe_numer = 0;
	ModeleRep[i].status = 1;
	i++;
	CluzelModele = i;
	/* strcpy(ModeleRep[i].nom,"Chassis");
	strcpy(ModeleRep[i].code,"CC");
	ModeleRep[i].famille = FAMILLE_CLUZEL;
	ModeleRep[i].interface = INTERF_PCI;
	ModeleRep[i].revision = 4;
	ModeleRep[i].selecteur.type = REP_SELECT_NUMER;
	ModeleRep[i].selecteur.max = 4;
	ModeleRep[i].version = 2.0;
	ModeleRep[i].delai_msg = 3000;
	ModeleRep[i].horloge = 60.0;
	ModeleRep[i].diviseur0 = 10;
	ModeleRep[i].diviseur1 = 60;
	ModeleRep[i].masque = 0x3FFF;
	ModeleRep[i].bits32 = 0;
	ModeleRep[i].max_numer = 12;
	ModeleRep[i].regroupe_numer = 0;
	ModeleRep[i].status = 1;
	i++;
	strcpy(ModeleRep[i].nom,"Carte NOSTOS");
	strcpy(ModeleRep[i].code,"NOSTOS");
	ModeleRep[i].famille = FAMILLE_CLUZEL;
	ModeleRep[i].interface = INTERF_PCI;
	ModeleRep[i].revision = -1;
	ModeleRep[i].selecteur.type = REP_SELECT_NEANT;
	ModeleRep[i].selecteur.max = 0;
	ModeleRep[i].delai_msg = 3000;
	ModeleRep[i].version = 0.2;
	ModeleRep[i].horloge = 2.5;
	ModeleRep[i].diviseur0 = 2;
	ModeleRep[i].diviseur1 = 1;
	ModeleRep[i].masque = 0x3FFF;
	ModeleRep[i].bits32 = 0;
	ModeleRep[i].max_numer = 1;
	ModeleRep[i].regroupe_numer = 0;
	ModeleRep[i].status = 0;
	i++;
	strcpy(ModeleRep[i].nom,"Boite OPERA maitre");
	strcpy(ModeleRep[i].code,"OPERA-m");
	ModeleRep[i].famille = FAMILLE_OPERA;
	ModeleRep[i].interface = INTERF_IP;
	ModeleRep[i].revision = 0; // soft avec entete sur 3 int32, p.exe repariteur veto
	ModeleRep[i].selecteur.type = REP_SELECT_NEANT;
	ModeleRep[i].selecteur.max = 0;
	ModeleRep[i].delai_msg = 10000;
	ModeleRep[i].version = 2.4;
	ModeleRep[i].horloge = 125.0;
	ModeleRep[i].diviseur0 = 21;
	ModeleRep[i].diviseur1 = 60;
	ModeleRep[i].masque = 0xFFFF;
	ModeleRep[i].bits32 = 0;
	ModeleRep[i].max_numer = 2;
	ModeleRep[i].regroupe_numer = 1;
	ModeleRep[i].status = 0;
	 i++; */
	strcpy(ModeleRep[i].nom,"Boite OPERA");
	strcpy(ModeleRep[i].code,"Opera");
	ModeleRep[i].famille = FAMILLE_OPERA;
	ModeleRep[i].interface = INTERF_IP;
	ModeleRep[i].revision = 1;
	ModeleRep[i].selecteur.type = REP_SELECT_NUMER;
	ModeleRep[i].selecteur.max = 1;
	ModeleRep[i].version = 2.8;
	ModeleRep[i].delai_msg = 10000;
	ModeleRep[i].horloge = 125.0;
	ModeleRep[i].diviseur0 = 5;
	ModeleRep[i].diviseur1 = 18;
	ModeleRep[i].masque = 0xFFFF;
	ModeleRep[i].bits32 = 0;
	ModeleRep[i].max_numer = 1;
	ModeleRep[i].regroupe_numer = 1;
	ModeleRep[i].status = 0;
	i++;
	OperaModele = i;
	strcpy(ModeleRep[i].nom,"Horloge OPERA");
	strcpy(ModeleRep[i].code,"Horloge");
	ModeleRep[i].famille = FAMILLE_OPERA;
	ModeleRep[i].interface = INTERF_IP;
	ModeleRep[i].revision = 0;
	ModeleRep[i].selecteur.type = REP_SELECT_NEANT;
	ModeleRep[i].selecteur.max = 0;
	ModeleRep[i].version = 2.0;
	ModeleRep[i].delai_msg = 10000;
	ModeleRep[i].horloge = 125.0;
	ModeleRep[i].diviseur0 = 21;
	ModeleRep[i].diviseur1 = 60;
	ModeleRep[i].masque = 0xFFFF;
	ModeleRep[i].bits32 = 0;
	ModeleRep[i].max_numer = 1;
	ModeleRep[i].regroupe_numer = 1;
	ModeleRep[i].status = 0;
	i++;
	strcpy(ModeleRep[i].nom,"Carte CALI");
	strcpy(ModeleRep[i].code,"CALI");
	ModeleRep[i].famille = FAMILLE_CALI;
	ModeleRep[i].interface = INTERF_IP;
	ModeleRep[i].revision = 1;
	ModeleRep[i].selecteur.type = REP_SELECT_CANAL;
	ModeleRep[i].selecteur.max = 1;
	ModeleRep[i].version = 1.0;
	ModeleRep[i].delai_msg = 10000;
	ModeleRep[i].horloge = 100.0;
	ModeleRep[i].diviseur0 = 1;
	ModeleRep[i].diviseur1 = 100;
	ModeleRep[i].masque = 0xFFFF;
	ModeleRep[i].bits32 = 0;
	ModeleRep[i].max_numer = 1;
	ModeleRep[i].regroupe_numer = 1;
	ModeleRep[i].status = 0;
	i++;
	CaliModele = i;
	strcpy(ModeleRep[i].nom,"Carte Ethernet");
	strcpy(ModeleRep[i].code,"IP");
	ModeleRep[i].famille = FAMILLE_ETHER;
	ModeleRep[i].interface = INTERF_IP;
	ModeleRep[i].revision = 1;
	ModeleRep[i].selecteur.type = REP_SELECT_NEANT;
	ModeleRep[i].selecteur.max = 0;
	ModeleRep[i].version = 1.0;
	ModeleRep[i].delai_msg = 10000;
	ModeleRep[i].horloge = 100.0;
	ModeleRep[i].diviseur0 = 1;
	ModeleRep[i].diviseur1 = 100;
	ModeleRep[i].masque = 0xFFFFFFFF;
	ModeleRep[i].bits32 = 1;
	ModeleRep[i].max_numer = 1;
	ModeleRep[i].regroupe_numer = 1;
	ModeleRep[i].status = 0;
	i++;
	strcpy(ModeleRep[i].nom,"NI-6251");
	strcpy(ModeleRep[i].code,"6251");
	ModeleRep[i].famille = FAMILLE_NI;
	ModeleRep[i].interface = INTERF_NAT;
	ModeleRep[i].revision = 1;
	ModeleRep[i].selecteur.type = REP_SELECT_CANAL;
	ModeleRep[i].selecteur.max = 1;
	ModeleRep[i].version = 6251.0;
	ModeleRep[i].delai_msg = 0;
	ModeleRep[i].horloge = 1.0;
	ModeleRep[i].diviseur0 = 1;
	ModeleRep[i].diviseur1 = 1;
	ModeleRep[i].masque = 0xFFFF;
	ModeleRep[i].bits32 = 0;
	ModeleRep[i].max_numer = 1;
	ModeleRep[i].regroupe_numer = 1;
	ModeleRep[i].status = 0;
	i++;
	strcpy(ModeleRep[i].nom,"Archive SAMBA");
	strcpy(ModeleRep[i].code,"Archive");
	ModeleRep[i].famille = FAMILLE_ARCHIVE;
	ModeleRep[i].interface = INTERF_FILE;
	ModeleRep[i].selecteur.type = REP_SELECT_CANAL;
	ModeleRep[i].selecteur.max = 1;
	ModeleRep[i].revision = 0;
	ModeleRep[i].version = 1;
	ModeleRep[i].delai_msg = 0;
	ModeleRep[i].max_numer = 4;
	ModeleRep[i].regroupe_numer = 0;
	ModeleRep[i].bits32 = 0;
	ModeleRep[i].status = 0;
	i++;

	ModeleRepNb = i;
	RepartModlAjouteMedia();

}
/* ========================================================================== */
static void RepartModlAjouteMedia() {
	int i;

	i = ModeleRepNb;
	strcpy(ModeleRep[i].nom,"Media divers");
	strcpy(ModeleRep[i].code,"Media");
	ModeleRep[i].famille = FAMILLE_MEDIA;
	ModeleRep[i].interface = INTERF_USB;
	ModeleRep[i].revision = 1;
	ModeleRep[i].selecteur.type = REP_SELECT_CANAL;
	ModeleRep[i].selecteur.max = 1;
	ModeleRep[i].version = 1.0;
	ModeleRep[i].delai_msg = 10000;
	ModeleRep[i].horloge = 1.0;
	ModeleRep[i].diviseur0 = 1;
	ModeleRep[i].diviseur1 = 1;
	ModeleRep[i].masque = 0xFFFF;
	ModeleRep[i].bits32 = 0;
	ModeleRep[i].max_numer = 1;
	ModeleRep[i].regroupe_numer = 1;
	ModeleRep[i].status = 0;
	i++;
	ModeleRepNb = i;
}
/* ========================================================================== */
char RepartiteurModlLit(char *fichier) {
	FILE *f; int modl; char nom_complet[MAXFILENAME];
	char media_defini;

	if(fichier) {
		strcpy(RepartiteurFichierModeles,fichier);
		RepertoireIdentique(RepartiteurFichierLu,RepartiteurFichierModeles,nom_complet);
	} else {
		RepartiteurFichierModeles[0] = '\0';
		strcpy(nom_complet,FichierModlRepartiteurs);
	}
	if((f = fopen(nom_complet,"r"))) {
		if(SambaLogDemarrage) SambaLogDef(". Lecture des modeles de repartiteurs","dans",nom_complet);
		ArgFromFile(f,RepertModlDesc,0);
		fclose(f);
	} else {
		printf("  ! Fichier %s inutilisable (%s). Modeles standard utilises\n",nom_complet,strerror(errno));
		RepartModlStandard();
		if(fichier) {
			strcpy(RepartiteurFichierModeles,"modeles");
			RepertoireIdentique(RepartiteurFichierLu,RepartiteurFichierModeles,nom_complet);
		} else {
			RepartiteurFichierModeles[0] = '\0';
			strcpy(nom_complet,FichierModlRepartiteurs);
		}
		ArgPrint(nom_complet,RepertModlDesc);
	}
	CluzelModele = OperaModele = CaliModele = 0;
	media_defini = 0;
	for(modl=0; modl<ModeleRepNb; modl++) {
		if(ModeleRep[modl].famille == FAMILLE_CLUZEL) CluzelModele = modl + 1;
		else if(ModeleRep[modl].famille == FAMILLE_OPERA) OperaModele = modl + 1;
		else if(ModeleRep[modl].famille == FAMILLE_CALI) CaliModele = modl + 1;
		else if(ModeleRep[modl].famille == FAMILLE_MEDIA) media_defini = modl + 1;
	}
	if(CompteRendu.modeles) {
		printf("  . Examen des modeles de repartiteurs\n");
		if(CluzelModele) printf("    . Modele de %s par defaut: %s\n",RepartFamilleListe[FAMILLE_CLUZEL],ModeleRep[CluzelModele-1].nom);
		else printf("    . Modele de %s inconnu\n",RepartFamilleListe[FAMILLE_CLUZEL]);
		if(OperaModele) printf("    . Modele de %s par defaut: %s\n",RepartFamilleListe[FAMILLE_OPERA],ModeleRep[OperaModele-1].nom);
		else printf("    . Modele de %s inconnu\n",RepartFamilleListe[FAMILLE_OPERA]);
		if(CaliModele) printf("    . Modele de %s par defaut: %s\n",RepartFamilleListe[FAMILLE_CALI],ModeleRep[CaliModele-1].nom);
		else printf("    . Modele de %s inconnu\n",RepartFamilleListe[FAMILLE_CALI]);
	}
	if(media_defini) { if(CompteRendu.modeles) printf("    . Au moins un media defini ('%s')\n",ModeleRep[media_defini-1].code); }
	else {
		RepartModlAjouteMedia();
		if(CompteRendu.modeles) printf("    . Media ajoute: '%s'\n",ModeleRep[ModeleRepNb-1].code);
	}
	for(modl=0; modl<ModeleRepNb; modl++) {
		if(((ModeleRep[modl].famille == FAMILLE_ARCHIVE) && (ModeleRep[modl].interface != INTERF_FILE))
		   || ((ModeleRep[modl].famille != FAMILLE_ARCHIVE) && (ModeleRep[modl].interface == INTERF_FILE))) break;
	}
	if(modl < ModeleRepNb) {
		printf("    ! Incoherence entre famille %s et interface %s\n",RepartFamilleListe[ModeleRep[modl].famille],InterfaceListe[ModeleRep[modl].interface]);
		return(0);
	}
	return(1);
}

#pragma mark --- materiel utilise ---
/* .......................................................................... */
/*                        M A T E R I E L   U T I L I S E                     */
/* ========================================================================== */
int RepartiteursEcrit(char *fichier) {
	FILE *f,*g; char autre[MAXFILENAME],nom_complet[MAXFILENAME];
	int rep,l; char branche[8+NUMER_NOM];

	RepertoireCreeRacine(fichier);
	if((f = fopen(fichier,"w"))) {
		if(RepartiteurFichierModeles[0]) fprintf(f,"modeles @%s\n",RepartiteurFichierModeles);
		autre[0] = '\0'; g = 0;
		for(rep=0; rep<RepartNb; rep++) {
			char standard; TypeNumeriseur *numeriseur;
			memcpy(&RepartLu,Repart+rep,sizeof(TypeRepartiteur));
			RepartLu.nbentrees = 0;
			for(l=0; l<Repart[rep].maxentrees; l++) {
				RepartEntreeLue[l][0] = '\0'; branche[0] = '\0';
				// printf("(%s) Entree %d: fibre '%s', bb @%08X\n",__func__,l+1,Repart[rep].entree[l].fibre,(hexa)(Repart[rep].entree[l].adrs));
				if(Repart[rep].categ == DONNEES_FICHIER) strcpy(branche,(char *)Repart[rep].entree[l].adrs);
				else if(Repart[rep].entree[l].fibre[0]) snprintf(branche,NUMER_NOM,"[%s]",Repart[rep].entree[l].fibre);
				else if((numeriseur = (TypeNumeriseur *)Repart[rep].entree[l].adrs)) {
					if(numeriseur->def.fibre[0]) snprintf(branche,NUMER_NOM,"[%s]",numeriseur->def.fibre);
					else strcpy(branche,NumeriseurListe[numeriseur->bb]);
				}
				// printf("(%s) soit texte = '%s', branche = '%s'\n",__func__,&(RepartEntreeLue[l][0]),branche);
				if(branche[0]) {
					if(Repart[rep].famille == FAMILLE_IPE)
						snprintf(&(RepartEntreeLue[l][0]),REPART_ENTREE_NOM,"%s/%s",branche,IpeCodesAdcMode[RepartLu.r.ipe.modeBB[l]]);
					else strcpy(&(RepartEntreeLue[l][0]),branche); // RepartNumerLu[l] = numeriseur->bb;
					RepartLu.nbentrees = l + 1;
				}
			}
			for(l=0; l<RepartLu.nbentrees; l++) if(RepartEntreeLue[l][0] == '\0') {
				strcpy(&(RepartEntreeLue[l][0]),NumeriseurListe[NumeriseurNb]); // RepartNumerLu[l] = NumeriseurNb;
			}
			if(Repart[rep].famille == FAMILLE_IPE) { RepartIpeCtlLus = RepartLu.r.ipe.fltnb; RepartIpeRetLus = IPE_FLT_SIZE * RepartLu.r.ipe.fltnb; }
			standard = 0;
			if(RepartLu.maintenance[0] && strcmp(RepartLu.maintenance,".")) {
				FILE *h; if((h = fopen(RepartLu.maintenance,"r"))) { standard = 1; fclose(h); }
			}
			if(!standard) strcpy(RepartLu.maintenance,".");
			if(strcmp(RepartLu.fichier,autre)) {
				if(g) { fclose(f); f = g; g = 0; }
				if(RepartLu.fichier[0]) {
					RepertoireIdentique(RepartiteurFichierLu,RepartLu.fichier,nom_complet);
					g = f;
					f = fopen(nom_complet,"r");
					if(f) strcpy(autre,RepartLu.fichier);
					else { f = g; g = 0; }
				}
			}
			if(RepartLu.interf == ModeleRep[RepartLu.type].interface)
				fprintf(f,"#\n=%s {\n",ModeleRep[RepartLu.type].code);
			else fprintf(f,"#\n=%s:%s {\n",ModeleRep[RepartLu.type].code,InterfaceListe[RepartLu.interf]);
			ArgIndent(1); ArgAppend(f,0,standard? RepartDescStandard: RepartDesc); ArgIndent(0);
			fprintf(f,"}\n");
		}
		fclose(f); if(g) fclose(g);
		printf("* Liste des repartiteurs sauvee sur %s\n",fichier);
		RepartiteursASauver = 0;
		return(1);
	} else /* Fichier des repartiteurs inaccessible */ {
		printf("! Impossible de sauver les repartiteurs sur:\n  %s\n  (%s)\n",fichier,strerror(errno));
		return(0);
	}
}
/* ========================================================================== */
int RepartiteursSauve() { return(RepartiteursEcrit(FichierPrefRepartiteurs)); }
/* ========================================================================== */
void RepartiteurInit(int modl, TypeRepartiteur *repart, char interface) {
	int i; TypeRepModele *modele_rep;

	repart->type = modl;
	repart->interf = interface;
	modele_rep = &(ModeleRep[modl]);
	repart->famille = modele_rep->famille;
	repart->masque = modele_rep->masque;
	repart->maxentrees = modele_rep->max_numer; /* sera ajuste dans RepartiteurVerifieParmsEtFIFO */
	repart->extens = 0xFFFFFFFF - repart->masque; repart->zero = (repart->masque + 1) / 2;
	switch(repart->famille) {
		case FAMILLE_OPERA: case FAMILLE_SAMBA: case FAMILLE_IPE: repart->ecr.type = MEDIA_UDP_WR; break;
		case FAMILLE_CALI: case FAMILLE_ETHER: repart->ecr.type = MEDIA_TCP_WR; break;
		default: repart->ecr.type = (interface == INTERF_IP)? MEDIA_TCP_WR: MEDIA_USB; break;
	}
	repart->categ = DONNEES_STD;
//	repart->event = REP_DATA_STREAM;
	repart->mode = 0;
	repart->valide = 1;
	repart->maintenance[0] = '\0';
	repart->check = 0;
	repart->candidat = 0;
	repart->avec_gene = 0;
	//	repart->sans_donnee = 0;
	repart->mode = 0;
	repart->D3trouvee = 0;
	repart->utile = 0;
	repart->fifo_pleine = 0;
	repart->local = 0;
	repart->en_route = 0;
	repart->recopie = 0;
	repart->status_demande = repart->status_relu = 0; strcpy(repart->date_status,"neant");
	repart->octets = 0; repart->fifo32 = 0; repart->data16 = 0;
	repart->depilable = 0;
	repart->entree = 0;
	repart->nbdonnees = 0;
	repart->nbident = 0;
	if(repart->famille == FAMILLE_CLUZEL) {
		repart->r.cluzel.d2 = 100; repart->r.cluzel.d3 = 1000;
	} else if(repart->famille == FAMILLE_OPERA) {
		repart->r.opera.d0 = 21;
		repart->r.opera.d2d3 = 2;
		repart->r.opera.retard = 36;
		repart->r.opera.mode = code_acqui_EDW_BB2;
		repart->r.opera.clck = 0;
		repart->r.opera.sync = 0;
//		repart->ecr.type = MEDIA_UDP_WR;
		repart->ecr.port = _port_serveur_UDP;
		repart->p.ip.max_udp = 245;
		strcpy(repart->p.ip.user,"root");
		repart->p.ip.pswd[0] = '\0';
	} else if(repart->famille == FAMILLE_CALI) {
		repart->r.cali.sel = 0;
		repart->r.cali.clck = 0;
//		repart->ecr.type = MEDIA_TCP_WR;
		repart->ecr.port = 7;
		repart->p.ip.max_udp = 0x7FFFFF;
		repart->p.ip.user[0] = '\0';
		repart->p.ip.pswd[0] = '\0';
	} else if(repart->famille == FAMILLE_ETHER) {
		repart->r.eth.clck = 0;
		repart->r.eth.maxentrees = 1;
		repart->r.eth.voies_par_numer = 4;
//		repart->ecr.type = MEDIA_TCP_WR;
		repart->ecr.port = 23;
		repart->p.ip.max_udp = 0x7FFFFF;
		repart->p.ip.user[0] = '\0';
		repart->p.ip.pswd[0] = '\0';
	} else if(repart->famille == FAMILLE_MEDIA) {
		repart->ecr.port = (repart->ecr.type == MEDIA_USB)? 0: 7;
	} else if(repart->famille == FAMILLE_SAMBA) {
		repart->r.samba.horloge = 100.0;
//		repart->ecr.type = MEDIA_TCP_WR;
		repart->ecr.port = 7;
		repart->p.ip.max_udp = 0x7FFFFF;
		repart->p.ip.user[0] = '\0';
		repart->p.ip.pswd[0] = '\0';
	} else if(repart->famille == FAMILLE_IPE) {
		repart->r.ipe.d0 = 20;
		repart->r.ipe.clck = 0;
		repart->r.ipe.ports_BB = 1;
		repart->r.ipe.bourrage = IPE_BOURRE_FLT;
		strcpy(repart->r.ipe.fpga,"/root/edelweiss/fpga/");
		for(i=0; i<REPART_ENTREES_MAX; i++) {
			repart->r.ipe.modeBB[i] = IPE_BB2;
			repart->r.ipe.retard[i] = IpeDelais[IPE_BB2];
		}
		for(i=0; i<IPE_FLT_MAX; i++) repart->r.ipe.ctl_defaut[i] = 0x00000002;
		repart->r.ipe.chassis = 0;
		repart->r.ipe.fifo = -1;
		repart->r.ipe.fltnb = 1;
		repart->r.ipe.decale = 0;
		repart->r.ipe.masque = 0;
//		repart->ecr.type = MEDIA_UDP_WR;
		repart->ecr.port = 9940;
		repart->p.ip.max_udp = 245;
		strcpy(repart->p.ip.user,"root");
		strcpy(repart->p.ip.pswd,"edelweiss2011");
	}
}
/* ========================================================================== */
void RepartiteurVerifieParmsEtFIFO(TypeRepartiteur *repart, char *message) {
	int rep,ip,num,lim,dep; int k,l,flt,num_utilise;
	char buffer[80],adrs[80],nom[REPART_NOM];
	int voies_par_mot; shex d1;
	TypeRepModele *modele_rep;

	message[0] = '\0';
	// printf("(%s.1) Repartiteur @%s\n",__func__,repart->parm);

	modele_rep = &(ModeleRep[repart->type]);

	repart->revision = modele_rep->revision;
#ifdef REPART_SELECTION
	repart->select = repart->valide;
#endif
	repart->simule = (repart->valide == REP_STATUT_SIMULE);
	if(repart->famille != FAMILLE_CLUZEL) {
		if(repart->fifo32) { free(repart->fifo32); repart->fifo32 = 0; }
	}
	switch(repart->interf) {
		case INTERF_PCI: if(repart->i.pci.nbregs || repart->d.pci.nbregs) RepartAvecInit = 1; break;
		case INTERF_IP: if(repart->i.ip.nbactn || repart->d.ip.nbactn) RepartAvecInit = 1; break;
		case INTERF_USB: RepartAvecInit = 0; /* pas sur */ break;
	}

	switch(repart->famille) {

		case FAMILLE_OPERA: case FAMILLE_CALI: case FAMILLE_ETHER: case FAMILLE_SAMBA: case FAMILLE_IPE:
			if(repart->famille == FAMILLE_OPERA) {
			#ifdef OPERA_SUPPORT
				OperaDimensions(repart->r.opera.mode,&d1,&voies_par_mot,&(repart->voies_par_numer));
				OperaCodeEntrees = OperaDecodeSelecteur(repart,OperaMasque);
				OperaEntreesLimite(repart,&OperaCodeEntrees);
			#endif
			} else if(repart->famille == FAMILLE_CALI) {
				repart->maxentrees = 1;
				repart->voies_par_numer = 4;
			} else if(repart->famille == FAMILLE_ETHER) {
				repart->maxentrees =  repart->r.eth.maxentrees;
				repart->voies_par_numer = repart->r.eth.voies_par_numer;
			} else if(repart->famille == FAMILLE_IPE) {
				RepartIpeNb++;
				k = -1;
				for(rep=0; rep<repart->rep; rep++) if(Repart[rep].famille == FAMILLE_IPE) {
					if(k < Repart[rep].r.ipe.chassis) k = Repart[rep].r.ipe.chassis;
					if(!strcmp(Repart[rep].parm,repart->parm)) break;
				}
				if(rep < repart->rep) repart->r.ipe.chassis = Repart[rep].r.ipe.chassis;
				else {
					if(++k < IPE_CHASSIS_MAX) repart->r.ipe.chassis = k;
					else {
						sprintf(message,"Chassis IPE @%s excedentaire (maxi: %d)",repart->parm,IPE_CHASSIS_MAX);
						break;
					}
				}
				repart->voies_par_numer = 6;
				if(repart->r.ipe.masque) {
					hexa masque;
					masque = repart->r.ipe.masque;
					IpePixBus[repart->r.ipe.chassis] |= masque;
					repart->r.ipe.decale = -1; repart->r.ipe.fltnb = 0;
					for(k=0; k<31; k++) {
						if((masque & 1)) {
							if(repart->r.ipe.decale < 0) repart->r.ipe.decale = k;
							repart->r.ipe.fltnb += 1;
						} else if(repart->r.ipe.decale >= 0) break; // pas de trou dans le gruyere
						masque = (masque >> 1) & 0x7FFFFFFF;
					}
				}
				if(repart->r.ipe.decale < 0) repart->r.ipe.decale = 0;
				if(!repart->r.ipe.fltnb) repart->r.ipe.fltnb = 1;
				repart->r.ipe.of7 = repart->r.ipe.decale * IPE_FLT_SIZE;
				repart->maxentrees = repart->r.ipe.fltnb * IPE_FLT_SIZE;
				repart->r.ipe.flt = (TypeIpeFlt *)malloc(repart->r.ipe.fltnb * sizeof(TypeIpeFlt));
				if(!(repart->r.ipe.flt)) { sprintf(message,"Pas de memoire pour la gestion des flt"); break; }
				l = 0;
				for(flt=0; flt<repart->r.ipe.fltnb; flt++) {
					hexa complet;
					complet = repart->r.ipe.ctl_defaut[flt]; if(BigEndianOrder) ByteSwappeInt(&complet);
					repart->r.ipe.flt[flt].branchee = repart->r.ipe.flt[flt].active = repart->r.ipe.flt[flt].enabled = 0;
					repart->r.ipe.flt[flt].controle.complet = complet;
					/* printf("(%s) Control[%2d] = 0x%08X = %02X %02X %02X %02X\n",__func__,flt+1,complet,
						repart->r.ipe.flt[flt].controle.partiel.speciaux,repart->r.ipe.flt[flt].controle.partiel.enable,
						repart->r.ipe.flt[flt].controle.partiel.modeBB,repart->r.ipe.flt[flt].controle.partiel.modeFLT); */
					repart->r.ipe.flt[flt].repart = (void *)repart; repart->r.ipe.flt[flt].l0 = l; l += IPE_FLT_SIZE;
				}
			}
//			repart->ecr.ip.path = -1; repart->lec.ip.path = -1;
			ip = HostInt(repart->parm); /* inet_addr(nom) == ip */
			// printf("  | adresse IP = %08X\n",ip);
			repart->adrs.val = ip;
			if((repart->adrs.champ[3] == 0) && (IPserial > 0)) repart->adrs.champ[3] = IPserial++;
			num_utilise =
			   ((repart->famille == FAMILLE_IPE) && (repart->adrs.champ[3] > 90))? repart->adrs.champ[3] - 90:
			   (((repart->famille == FAMILLE_SAMBA) && (repart->adrs.champ[3] > 20))? repart->adrs.champ[3] - 20:
				repart->adrs.champ[3]);
			if(RepartNumLibre[repart->famille] <= num_utilise) RepartNumLibre[repart->famille] = num_utilise + 1;
			if((repart->famille == FAMILLE_IPE) && (repart->r.ipe.fifo >= 0))
				sprintf(nom,"%s%02d.%d",modele_rep->code,num_utilise,repart->r.ipe.fifo+1);
			else sprintf(nom,"%s%02d",modele_rep->code,num_utilise);
			if(repart->ecr.port > 0) sprintf(adrs,"%s:%d",repart->parm,repart->ecr.port);
			else strcpy(adrs,repart->parm);
			MediaDefini(&(repart->ecr),nom,adrs,repart->ecr.type); MediaListeAjouteRef(SambaMedia,&(repart->ecr));

			for(rep=0; rep<repart->rep; rep++) if((Repart[rep].interf == repart->interf)
				&& (Repart[rep].adrs.val == repart->adrs.val) && (Repart[rep].ecr.port == repart->ecr.port)) break;
			if(rep < repart->rep) {
				sprintf(message,"Les repartiteurs #%d et %d ont la meme adresse, %d.%d.%d.%d:%d",rep+1,repart->rep+1,IP_CHAMPS(repart->adrs),repart->ecr.port);
				memcpy(&(repart->ecr.ip.skt),&(Repart[rep].ecr.ip.skt),sizeof(TypeSocket));
				repart->valide = 0;
			} else if(SocketInitNum(repart->adrs.val,repart->ecr.port,&(repart->ecr.ip.skt)) <= 0) {
				sprintf(message,"L'adresse %d.%d.%d.%d:%d est incorrecte",IP_CHAMPS(repart->adrs),repart->ecr.port);
			};
			repart->present = 0;
			if(repart->valide && VerifRepertIP) {
				repart->present = 1;
				if(!repart->simule) {
					sprintf(buffer,COMMANDE_PING,IP_CHAMPS(repart->adrs));
					if(system(buffer) != 0) {
						sprintf(message,"L'adresse %d.%d.%d.%d:%d ne repond pas sur Internet",IP_CHAMPS(repart->adrs),repart->ecr.port);
						repart->present = 0;
					} else printf("  * L'adresse %d.%d.%d.%d:%d repond sur Internet\n",IP_CHAMPS(repart->adrs),repart->ecr.port);
				} else printf("  * Les reponses a l'adresse %d.%d.%d.%d:%d sont simulees\n",IP_CHAMPS(repart->adrs),repart->ecr.port);
			} /* else sprintf(message,"  * La connexion sur %d.%d.%d.%d n'a meme pas ete tentee (repartiteur %s)",
			   IP_CHAMPS(repart->adrs),repart->valide? "valide": "invalide"); */
			if(modele_rep->bits32) repart->octets = FIFOdim * sizeof(lhex);
			else repart->octets = FIFOdim * sizeof(shex);
			repart->fifo32 = (lhex *)malloc(repart->octets);
			if(repart->famille == FAMILLE_OPERA) {
				OperaTrame trame_opera;
				trame_opera = (OperaTrame)(repart->fifo32);
				repart->data16 = (TypeDonnee *)(trame_opera->data);
				sprintf(repart->code_rep,"o%02d",num_utilise);
				repart->s.ip.relance = 245;
			} else if(repart->famille == FAMILLE_CALI) {
				CaliTrame trame_cali;
				trame_cali = (CaliTrame)(repart->fifo32);
				repart->data16 = trame_cali->valeur;
				sprintf(repart->code_rep,"k%02d",num_utilise);
				repart->s.ip.relance = 0x7FFFFF;
			} else if(repart->famille == FAMILLE_ETHER) {
				char *trame_eth;
				trame_eth = (char *)(repart->fifo32) + repart->dim.entete;
				repart->data16 = (TypeDonnee *)trame_eth;
				sprintf(repart->code_rep,"e%02d",num_utilise);
			} else if(repart->famille == FAMILLE_SAMBA) {
				repart->data16 = (TypeDonnee *)(repart->fifo32);
				sprintf(repart->code_rep,"s%d",num_utilise);
				repart->s.ip.relance = 0x7FFF;
			} else if(repart->famille == FAMILLE_IPE) {
				IpeTrame trame_ipe;
				trame_ipe = (IpeTrame)(repart->fifo32);
				repart->data16 = trame_ipe->p.donnees.val;
				if(repart->r.ipe.chassis == 0) {
					if(repart->r.ipe.fifo < 0) sprintf(repart->code_rep,"i%02d",num_utilise);
					else sprintf(repart->code_rep,"i.%d",repart->r.ipe.fifo+1);
				} else sprintf(repart->code_rep,"i%d%d",repart->r.ipe.chassis+1,repart->r.ipe.fifo+1);
				repart->s.ip.relance = 245;
			}
			repart->data32 = (lhex *)repart->data16;
			repart->depilable = FIFOdim;
			break;

		case FAMILLE_CLUZEL: case FAMILLE_NI:
			if(repart->parm[0]) sscanf(repart->parm,"%d",&num); else {
				num = 1;
				for(rep=0; rep<repart->rep; rep++) if(Repart[rep].interf == repart->interf) num++;
			}
			repart->adrs.val = num;
			num -= 1; /* num == index dans table des cartes trouvees (PCI) / des taches (NI) */
			if(repart->famille == FAMILLE_CLUZEL) lim = PCInb;
			else if(repart->famille == FAMILLE_NI) lim = NI_TACHES_MAX;
            else lim = 0;
			if(num > lim) {
				sprintf(message,"Repartiteur #%d sur PCI #%d: Seulement %d interface%s PCI permise%s",
					repart->rep+1,repart->adrs.val,lim,(lim>1)?"s":"",(lim>1)?"s":"");
				break;
			};
			if(repart->famille == FAMILLE_CLUZEL) {
				repart->voies_par_numer = 3;
				if((num >= 0) && (num < PCInb)) repart->p.pci.port = PCIedw[num].port; else repart->p.pci.port = 0;
				repart->p.pci.page_prgm = 0x1E; // different dans les versions precedentes, ex-PrgmRepartiteur (= 0x1E)
				repart->p.pci.bitD2D3 = (modele_rep->version < 1.1)? 0x2000: 0x1000;
				repart->octets = PCIedw[num].octets;
				repart->fifo32 = PCIedw[num].fifo;
				repart->depilable = PCIedw[num].octets / sizeof(Ulong);
				if(PCInb < 2) sprintf(nom,"%s",modele_rep->code);
				else sprintf(nom,"%s@PCI%d",modele_rep->code,repart->adrs.val);
				sprintf(repart->code_rep,"SC%d",repart->adrs.val); // LectViaNom[PCIedw[num].type] rend "MiG"
				Diviseur2 = repart->r.cluzel.d2;
				CluzelLue = repart->rep;
			} else if(repart->famille == FAMILLE_NI) {
				repart->voies_par_numer = 1;
				/* repart->voies_par_numer reel demande via RepartDesc */
				repart->p.pci.port = 0;
				repart->p.pci.page_prgm = 0x00;
				repart->p.pci.bitD2D3 = 0;
				if(repart->valide) {
					repart->octets = FIFOdim * sizeof(TypeDonnee);
					repart->fifo32 = (Ulong *)malloc(repart->octets);
					repart->depilable = FIFOdim;
					printf("  . FIFO allouee @%08X +%08X (soit %d valeurs)\n",(hexa)repart->fifo32,repart->octets,FIFOdim);
				}
				sprintf(nom,"NI:Dev%d",repart->adrs.val);
				sprintf(repart->code_rep,"NI");
			};
			repart->data32 = repart->fifo32;
			repart->data16 = (TypeDonnee *)repart->fifo32;
			break;

		case FAMILLE_MEDIA:
			strncpy(nom,repart->ecr.nom,REPART_NOM);
			// printf("(%s.2) Repartiteur %s @%s via %s\n",__func__,nom,repart->parm,InterfaceListe[repart->interf]);
			if(repart->interf == INTERF_USB) MediaDefini(&(repart->ecr),nom,repart->parm,MEDIA_USB);
			else {
				if(repart->ecr.port > 0) sprintf(adrs,"%s:%d",repart->parm,repart->ecr.port);
				else strcpy(adrs,repart->parm);
				MediaDefini(&(repart->ecr),nom,adrs,repart->ecr.type);
			}
			MediaListeAjouteRef(SambaMedia,&(repart->ecr));
			break;

		case FAMILLE_ARCHIVE:
			sprintf(nom,"%s%02d",modele_rep->code,repart->rep+1);
			repart->adrs.val = 1;
			sprintf(repart->code_rep,"a%02d",repart->adrs.val);
			repart->voies_par_numer = 1;
			/* repart->voies_par_numer reel demande via RepartDesc */
			repart->octets = FIFOdim * sizeof(TypeDonnee);
			repart->fifo32 = (Ulong *)malloc(repart->octets);
			repart->data16 = (TypeDonnee *)repart->fifo32;
			repart->depilable = FIFOdim;
			break;

		default: repart->adrs.val = 0; strcpy(nom,"tbd"); break;
	}
	if(repart->maxentrees) {
		if((repart->entree = (TypeRepartEntree *)malloc(repart->maxentrees * sizeof(TypeRepartEntree)))) {
			for(l=0; l<repart->maxentrees; l++) bzero((void *)&(repart->entree[l]),sizeof(TypeRepartEntree));
		} else {
			printf("  ! Pas assez de memoire pour gerer %d entree%s de %s\n",Accord1s(repart->maxentrees),nom);
			repart->maxentrees = 0;
		}
	}
//-	repart->nbentrees = repart->maxentrees;
	if(repart->fifo32) memset(repart->fifo32,10,repart->octets); /* rempli avec 0x0A0A = 2570 */
	repart->maxdonnees = ((int)(repart->fifo32) + repart->octets - (int)(repart->data16)) / sizeof(TypeDonnee);
	if(CompteRendu.repart.capa) printf("  . Reserve de donnees pour %s: %d valeurs\n",nom,repart->maxdonnees);
	for(dep=0; dep<REPART_RESERVOIR_MAX; dep++) {
		repart->depot[dep].reservoir = 0;
		repart->depot[dep].capacite = 0;
		repart->depot[dep].reserve = repart->depot[dep].fond = 0;
	}
	repart->depot_ancien = repart->depot_recent = 0; repart->depot_nb = 0;
	strncpy(repart->nom,nom,REPART_NOM);
	if(message[0]) repart->valide = 0;
}
/* ========================================================================== */
void RepartiteurCodeEntree(TypeRepartiteur *repart, int l, char *code) {
	if(repart->famille == FAMILLE_IPE) {
		if(RepartIpeNb > 1) {
			if(l >= 0) sprintf(code,"%d%c%d",repart->r.ipe.chassis+1,IPE_CODE_ENTREE(repart,l));
			else sprintf(code,"%d--",repart->r.ipe.chassis+1);
		} else if(l >= 0) sprintf(code,"%c%d",IPE_CODE_ENTREE(repart,l));
		else strcpy(code,"--");
	} else {
		if(RepartNb > 1) strcpy(code,repart->code_rep);
		else if(l >= 0) sprintf(code,"%d",l+1); else sprintf(code,"-");
	}
}
/* ========================================================================== */
void RepartiteurBranche(TypeRepartiteur *repart, int l, TypeNumeriseur *numeriseur) {
	repart->entree[l].adrs = (TypeAdrs)numeriseur;
	if(l >= repart->nbentrees) repart->nbentrees = l + 1;
	if(numeriseur) {
		numeriseur->repart = repart;
		numeriseur->entree = l;
		RepartiteurCodeEntree(repart,l,numeriseur->code_rep);
	}
	if(repart->famille == FAMILLE_IPE) {
		if(numeriseur) repart->r.ipe.modeBB[l] = (numeriseur->vsnid < 2)? IPE_BB1: IPE_BB2;
		else repart->r.ipe.modeBB[l] = IPE_BB2;
	}
}
/* ========================================================================== */
void RepartiteurDebranche(TypeRepartiteur *repart, int l) {
	TypeNumeriseur *numeriseur;

	numeriseur = (TypeNumeriseur *)(repart->entree[l].adrs);
	repart->entree[l].adrs = 0;
	if(!numeriseur) return;
	numeriseur->repart = 0;
	numeriseur->entree = 0;
}
/* ==========================================================================
static void RepartiteurImprime(TypeRepartiteur *repart) {
	memcpy(&RepartLu,repart,sizeof(TypeRepartiteur));
	fprintf(stdout,"#\n=%s {\n",ModeleRep[RepartLu.type].code);
	ArgIndent(1); ArgAppend(stdout,0,RepartDesc); ArgIndent(0);
	fprintf(stdout,"}\n");
}
 ========================================================================== */
int RepartiteurAvecFibre(char *fibre, int *l) {
/* Rend le repartiteur (et l'entree l) connecte a la fibre 'fibre' */
	int rep,j;

	*l = 0;
	if(fibre[0] == '\0') return(-1);
	for(rep=0; rep<RepartNb; rep++)  {
		for(j=0; j<Repart[rep].maxentrees; j++) if(!strcmp(Repart[rep].entree[j].fibre,fibre)) break;
		if(j < Repart[rep].maxentrees) break;
	}
	if(rep < RepartNb) { *l = j; return(rep); } else return(-1);
}
/* ========================================================================== */
void RepartiteursStructInit() {
	int i,modl,vl,rep,k;

	RepartiteurParmModifie = 0;
	RepartiteursASauver = 0;
	for(i=0; i<FAMILLE_NB; i++) RepartNumLibre[i] = 2;
	strcpy(RepartChercheIPnet,"192.168.0");
	RepartChercheIPdep = 2; RepartChercheIParr = 127;
	RepartDonneesAffiche = RepartCourbesAffiche = 0;
	RepartDataMode = 0;
	RepartAvecInit = 0;
	cRepartAcq = 0;
	pRepartStatus = 0;
	pIpeStatus = 0;
	pRepartIpeChoixEntree = 0;
	pRepartDonnees = 0;
	iRepartRafraichi = 0;
	OperaMasque = 1;
	RepartVersionLogiciel = 0;
	RepartInfoFpgaBB[0] = '\0';
	//	sprintf(RepartInfoFpgaBB,"%02d%% prog/#%d",100,4);
	for(modl=0; modl<REPART_TYPES; modl++) {
		ModeleRep[modl].selecteur.type = REP_SELECT_UNDEF;
		ModeleRep[modl].selecteur.max = 0;
	}
	RepartTrameDef[FAMILLE_OPERA].lngr = sizeof(TypeOperaTrame);
	RepartTrameDef[FAMILLE_CALI].lngr = sizeof(TypeCaliTrame);
	RepartTrameDef[FAMILLE_IPE].lngr = sizeof(TypeIpeTrame);

	for(vl=0; vl<REPART_VOIES_MAX; vl++) {
		RepartConnexionEnCours[vl].valeur = 0;
		bzero(&(RepartConnexionEnCours[vl].tampon),sizeof(TypeTamponDonnees));
		sprintf(&(RepartDonneeNom[vl][0]),"Donnee %d",vl+1);
	}
	for(i=0; i<IPE_FLT_MAX; i++) sprintf(&(IpeFltNom[i][0]),"FLT #%d",i+1);
	NumerModeAcquis = NUMER_MODE_ACQUIS;
	RepartGraphEnCours = 0;
	RepartTraceLarg = 512000;
	RepartTraceHaut = 300;
	RepartTraceVisu = 512;
	// RepartMem = 0;
	RepartActnLue.duree = 0;
	CluzelLue = -1;
	CluzelModele = OperaModele = CaliModele = 0;

	OperaCde = 0; /* h */
	strcpy(OperaTampon,"001105020102");
	OperaParmQ = 3;

	RepartNb = 0;
	ModeleRepNb = 0;
	for(i=0; i<REPART_TYPES; i++) ModeleRepListe[i] = ModeleRep[i].nom; ModeleRepListe[i] = "\0";
	RepartiteurFichierModeles[0] = '\0';
	for(rep=0; rep<REPART_MAX; rep++) {
		Repart[rep].nom[0] = '\0';
		Repart[rep].fichier[0] = '\0';
		RepartListe[rep] = Repart[rep].nom;
		RepartEtat[rep] = REPART_IGNORE;
	}
	RepartListe[rep] = "\0";

	RepartIpeNb = 0;
	RepartIpeFlt = RepartIpeFibre = 1;
	for(k=0; k<IPE_CHASSIS_MAX; k++) IpePixBus[k] = 0;
}
/* ========================================================================== */
int RepartiteursLit(char *fichier) {
	int nb_erreurs;
	FILE *f,*g; char ligne[256],*r,*s,*item;
	int i; size_t l; int rep,modl; char interf;
	int vl,vt;
	TypeADU lu;
	TypeRepartiteur *repart; TypeNumeriseur *numeriseur;
	char log; char au_moins_un_rep_lu;
	char message[256],nom_complet[MAXFILENAME];

	/* lecture du fichier */
	log = 0;
	RepartConnecte = &(Repart[0]);
	repart = RepartConnecte; // GCC
	strcpy(RepartiteurFichierLu,fichier);
	// printf(". %d entetes de voies dans la premiere trame d'evenement\n",(int)REPART_EVENT_VOIEHDR_NB_DEF);
	f = fopen(fichier,"r");
	if(!f && StartFromScratch) {
		printf("> Installe repartiteurs    dans '%s'\n",fichier);
		RepartModlStandard();
		strcpy(RepartiteurFichierModeles,"modeles");
		RepertoireIdentique(RepartiteurFichierLu,RepartiteurFichierModeles,nom_complet);
		ArgPrint(nom_complet,RepertModlDesc);
		//-	vt = 0;
		rep = 0;

		repart = &(Repart[rep]);
		OperaNouvelle(repart,"192.168.0.2",DONNEES_STD,code_acqui_EDW_BB2,36,1); /* externe */
		repart->nbentrees = 1;
		repart->entree = (TypeRepartEntree *)malloc(ModeleRep[repart->type].max_numer * sizeof(TypeRepartEntree));
		repart->entree[0].adrs = (TypeAdrs)&(Numeriseur[NumeriseurNb-1]);
		for(l=1; l<ModeleRep[repart->type].max_numer; l++) repart->entree[l].adrs = 0;
		rep++;

		repart = &(Repart[rep]);
		CaliNouvelle(repart,"192.168.2.2",DONNEES_STD);
		repart->nbentrees = 1;
		repart->entree = (TypeRepartEntree *)malloc(ModeleRep[repart->type].max_numer * sizeof(TypeRepartEntree));
		repart->entree[0].adrs = (TypeAdrs)&(Numeriseur[NumeriseurNb-1]);
		for(l=1; l<ModeleRep[repart->type].max_numer; l++) repart->entree[l].adrs = 0;
		rep++;

		RepartNb = rep;
		RepartiteursEcrit(fichier);
		f = fopen(fichier,"r");
	}

	nb_erreurs = 0;
	au_moins_un_rep_lu = 0;
	vl = 0; // GCC
	//-	BoloMaxTot = 0;
	vt = 0; g = 0;
	if(f) {
		SambaLogDef("= Lecture des repartiteurs","dans",fichier);
		printf("  . La verification des adresses IP %s\n",VerifRepertIP? "est prevue": "NE SERA PAS effectuee");
		do {
			while(LigneSuivante(ligne,256,f)) {
				if((ligne[0] == '#') || (ligne[0] == '\n')) continue;
				if(RepartNb >= REPART_MAX) {
					printf("  ! Deja %d repartiteurs lus! Repartiteurs suivants abandonnes\n",RepartNb);
					++nb_erreurs;
					break;
				}
				/* '=' {Cluzel | SCv1 | SCv2 | CC | OPERA} [':HS'] ['<'BB|direct|stamp[+<retard>][/<mode>]:<detec>'>'] '@' {<carte_PCI> | <adrs_ip>}
					suivi eventuellement de '{' / { <ressource> '=' <valeur> / } '}'
					suivi eventuellement de la reponse des BN a l'identification.
					Acces direct: le detecteur peut etre deja declare dans le fichier Detecteurs,
								  mais on compte alors un seul numeriseur pour ce detecteur.
					Actuellement, dans ce cas on a aussi une seule voie... mais la boite OPERA
					peut en transmettre deux selon le code d'acquisition!!!
				 */
				vl = 0;
				r = ligne;
				if(*r == '@') {
					s = r + 1;
					ItemJusquA(' ',&s);
					l = strlen(r); if(*(r+l-1) == '\n') *(r+l-1) = '\0';
					if(g) {
						printf("  ! Deja une indirection! fichier %s abandonne\n",r+1);
						continue;
					}
					strncpy(Repart[rep].fichier,r+1,MAXFILENAME);
					g = f;
					RepertoireIdentique(RepartiteurFichierLu,r+1,nom_complet);
					SambaLogDef(". Lecture de repartiteurs","dans",nom_complet);
					f = fopen(nom_complet,"r");
					if(!f) {
						printf("  ! Impossible de lire des repartiteurs d'apres:\n  %s\n  (%s)\n",
						   nom_complet,strerror(errno));
						++nb_erreurs;
						f = g; g = 0; continue;
					}
				} else if(*r == '=') {
					if(!ModeleRepNb) {
						RepartModlStandard();
						strcpy(RepartiteurFichierModeles,"ModelesRepartiteurs");
						RepertoireIdentique(RepartiteurFichierLu,RepartiteurFichierModeles,nom_complet);
						ArgPrint(nom_complet,RepertModlDesc);
						printf("  * Liste standard des modeles de repartiteurs ecrite dans:\n  %s\n", nom_complet);
					}
					r++;
					item = ItemJusquA('{',&r);
					s = item; ItemJusquA(':',&s);
					//- for(modl=0; modl<ModeleRepNb; modl++) printf("  * Repartiteur modele #%d: '%s'\n",modl,ModeleRep[modl].code);
					for(modl=0; modl<ModeleRepNb; modl++) if(!strcmp(item,ModeleRep[modl].code)) break;
					if(modl >=  ModeleRepNb) { printf("  ! Type de repartiteur inconnu: '%s'\n",item); ++nb_erreurs; continue; }
					interf = ModeleRep[modl].interface;
					if(*s) {
						i = 0; while(InterfaceListe[i][0]) { if(!strcmp(InterfaceListe[i],s)) break; i++; }
						if(InterfaceListe[interf][0]) interf = i;
					}
					if(log > 1) printf("  + Repartiteur #%d de type %s via %s\n",RepartNb+1,ModeleRep[modl].code,InterfaceListe[interf]);
					RepartiteurInit(modl,&RepartLu,interf);

					ArgFromFile(f,RepartDesc,"}");
					if(RepartLu.maintenance[0] && strcmp(RepartLu.maintenance,".")) {
						char maintenance[MAXFILENAME]; FILE *g;
						RepertoireIdentique(fichier,RepartLu.maintenance,maintenance);
						if((g = fopen(maintenance,"r"))) {
							ArgFromFile(g,RepartOptnIfDesc[RepartLu.interf],0);
							fclose(g);
						}
					}
					if(((RepartLu.famille == FAMILLE_ARCHIVE) && (RepartLu.categ != DONNEES_FICHIER))
					   || ((RepartLu.famille != FAMILLE_ARCHIVE) && (RepartLu.categ == DONNEES_FICHIER))) {
						printf("  ! Incoherence entre famille %s et donnee %s\n",RepartFamilleListe[RepartLu.famille],RepartDonneesListe[RepartLu.categ]); ++nb_erreurs;
					}
					// printf("  * Repartiteur[%d] = {\n",RepartNb); ArgIndent(1); ArgAppend(stdout,0,RepartDesc); printf("  }\n"); ArgIndent(1);
					rep = RepartNb++; repart = &(Repart[rep]);
					memcpy(repart,&RepartLu,sizeof(TypeRepartiteur));
					repart->rep = rep; au_moins_un_rep_lu = 1;
					RepartiteurVerifieParmsEtFIFO(repart,message);

					switch(repart->categ) {
					  case DONNEES_STD:
						if(repart->nbentrees > repart->maxentrees) repart->nbentrees = repart->maxentrees;
						if(CompteRendu.repart.capa) printf("  . %s: %d entree%s\n",repart->nom,Accord1s(repart->nbentrees));
						for(l=0; l<repart->maxentrees; l++) {
							if(l < repart->nbentrees) {
								char *r,*nom_bb,*nom_mode; char mode_bb,mode_avant,fibre; int bb;
								repart->entree[l].fibre[0] ='\0';
								if(log > 1) printf("    > %s, entree #%d: %s\n",repart->nom,l+1,&(RepartEntreeLue[l][0]));
								//- bb = RepartNumerLu[l];
								r = &(RepartEntreeLue[l][0]);
								nom_bb = ItemJusquA('/',&r); fibre = 0;
								if(log > 2) printf("      > numeriseur: '%s'\n",nom_bb);
								if(!strcmp(nom_bb,NumeriseurListe[NumeriseurNb])) /* on a entre "neant" */ {
									repart->entree[l].adrs = 0;
									if(repart->famille == FAMILLE_IPE) repart->r.ipe.modeBB[l] = 0;
								} else {
									if((nom_bb[0] == '[') || (nom_bb[0] == '>')) {
										int lngr;
										fibre = 1; nom_bb++;
										lngr = strlen(nom_bb);
										if(nom_bb[lngr-1] == ']') nom_bb[lngr-1] = '\0';
										strncpy(repart->entree[l].fibre,nom_bb,FIBRE_NOM); repart->entree[l].fibre[FIBRE_NOM-1] = '\0';
										for(bb=0; bb<NumeriseurNb; bb++) if(!strcmp(nom_bb,Numeriseur[bb].def.fibre)) break;
									} else {
										for(bb=0; bb<NumeriseurNb; bb++) if(!strcmp(nom_bb,NumeriseurListe[bb])) break;
									}
									if((bb >= 0) && (bb < NumeriseurNb)) /* (Numeriseur[RepartNumerLu[l]].bb < NumeriseurNb) */{
										numeriseur = &(Numeriseur[bb]);
										RepartiteurBranche(repart,l,numeriseur);
										if(repart->entree[l].fibre[0] =='\0') strcpy(repart->entree[l].fibre,numeriseur->def.fibre);
										if(log > 2) printf("      > %s.%d <- %s (0x%0X)\n",repart->nom,l+1,numeriseur->nom,(hexa)numeriseur);
										if(repart->famille == FAMILLE_IPE) {
											nom_mode = ItemJusquA(' ',&r);
											mode_avant = repart->r.ipe.modeBB[l];
											mode_bb = IPE_NBADCMODES;
											if(nom_mode[0]) {
												for(mode_bb=0; mode_bb<IPE_NBADCMODES; mode_bb++) if(!strcmp(nom_mode,IpeCodesAdcMode[mode_bb])) break;
											}
											if(mode_bb >= IPE_NBADCMODES) {
												for(mode_bb=0; mode_bb<IPE_NBADCMODES; mode_bb++) if(IpeVpn[mode_bb] == numeriseur->nbadc) break;
												if(mode_bb >= IPE_NBADCMODES) {
													for(mode_bb=0; mode_bb<IPE_NBADCMODES; mode_bb++) if(numeriseur->nbadc < IpeVpn[mode_bb]) break;
													if(mode_bb >= IPE_NBADCMODES) mode_bb = mode_avant;
												}
												printf("  ! %s, entree %d (%s): mode %s inconnu (remplace par %s)\n",repart->nom,l+1,nom_bb,nom_mode,IpeCodesAdcMode[mode_bb]);
												++nb_erreurs;
											}
											repart->r.ipe.modeBB[l] = mode_bb;
											repart->r.ipe.flt[IPE_FLT_NUM(l)].branchee = repart->r.ipe.flt[IPE_FLT_NUM(l)].enabled = 1;
										}
									} else {
										repart->entree[l].adrs = 0;
										if(repart->famille == FAMILLE_IPE) repart->r.ipe.modeBB[l] = 0;
										printf("  ! %s, entree %d: nom de %s inconnu (%s)\n",repart->nom,l+1,fibre? "fibre":"numeriseur",nom_bb);
										++nb_erreurs;
									}
								}
							} else {
								repart->entree[l].adrs = 0;
								if(repart->famille == FAMILLE_IPE) repart->r.ipe.modeBB[l] = 0;
							}
							// if(repart->entree[l].adrs) printf("    = %s\n",((TypeNumeriseur *)(repart->entree[l].adrs))->nom); else printf("    = pas de numeriseur connecte\n");
						}
						break;
					  case DONNEES_BB:
						RepartiteurBrancheIdent(repart);
						break;
					  case DONNEES_STAMP:
						if(repart->nbident || repart->nbentrees) {
							printf("  ! %s (timestamp) a %d entree%s et %d ident%s\n",repart->nbentrees,repart->nbident);
							++nb_erreurs;
							repart->nbident = 0; repart->nbentrees = 0;
						}
						break;
					  case DONNEES_FICHIER:
						repart->p.f.a = (TypeRepartAccesArch *)malloc(repart->maxentrees * sizeof(TypeRepartAccesArch));
						if(!(repart->p.f.a)) { printf("  ! Pas assez de memoire pour les entrees de %s\n",repart->nom); break; }
						repart->p.f.bin = (repart->r.f.fmt == ARCH_FMT_EDW2);
						if(repart->nbentrees > repart->maxentrees) repart->nbentrees = repart->maxentrees;
						if(CompteRendu.repart.capa) printf("  . %s: %d entree%s\n",repart->nom,Accord1s(repart->nbentrees));
						repart->nbdonnees = 0;
						for(l=0; l<repart->maxentrees; l++) {
							if(l < repart->nbentrees) {
								if(log > 1) printf("    > %s, entree #%d: %s\n",repart->nom,l+1,&(RepartEntreeLue[l][0]));
								repart->entree[l].adrs = (TypeAdrs)malloc(strlen(&(RepartEntreeLue[l][0]))+1);
								if(!(repart->entree)) { printf("  ! Pas assez de memoire pour l'entree #%d de %s\n",l+1,repart->nom); break; }
								strcpy((char *)repart->entree[l].adrs,&(RepartEntreeLue[l][0]));
								if(repart->p.f.bin) repart->p.f.a[l].o.path = -1; else repart->p.f.a[l].o.file = 0;
								ArchInit(repart,l);
								ArchOuvre(repart,l);
								ArchDemarre(repart,l);
							} else repart->entree[l].adrs = 0;
						}
						repart->nbident = 0;
						break;
					}
					if(repart->categ != DONNEES_FICHIER) RepartCompteDonnees(repart);
					if(repart->categ == DONNEES_BB) repart->categ = DONNEES_STD;
					if(repart->famille == FAMILLE_OPERA) OperaCompteursTrame(repart);
					else if(repart->famille == FAMILLE_CALI) CaliCompteursTrame(repart);
					else if(repart->famille == FAMILLE_IPE) IpeCompteursTrame(repart);
					vl = 0;
					vt += repart->nbdonnees;
					if(log) {
						printf("  . %-9s: lecture de donnees %s sur %s,",repart->nom,RepartDonneesListe[repart->categ],ModeleRep[repart->type].nom);
						if(repart->famille == FAMILLE_OPERA) printf("en mode %s,",OperaCodesAcquis[repart->r.opera.mode]);
						printf("\n               via %s",InterfaceListe[repart->interf]);
						if(repart->interf == INTERF_IP) printf(" a l'adresse %d.%d.%d.%d:%d",IP_CHAMPS(repart->adrs),repart->ecr.port);
						else if(repart->interf == INTERF_FILE) printf(": %s",repart->parm);
						else {
							printf(" a l'emplacement #%d",repart->adrs.val);
							if(repart->famille == FAMILLE_CLUZEL) {
								if(repart->adrs.val <= PCInb) printf(" (carte %s)",LectViaNom[PCIedw[repart->adrs.val-1].type]);
								else printf(" (CARTE ABSENTE)");
							}
						}
						printf("\n");
						if(!repart->valide) printf("    ! LECTURE INVALIDEE\n");
						printf("               %d numeriseur%s maxi de %d voies, soit %d voies maxi\n",
							Accord1s(repart->maxentrees),repart->voies_par_numer,repart->nbdonnees);
					}
					if(message[0]) { printf("  ! %s. Repartiteur %s invalide.\n",message,repart->nom); nb_erreurs++; }
				} else {
					if(!RepartNb) {
						item = ItemJusquA('@',&r);
						if(!strncmp(item,"modele",6)) {
							if(!RepartiteurModlLit(ItemJusquA(' ',&r))) ++nb_erreurs;
							// printf("  * Modeles = {\n"); ArgIndent(1); ArgAppend(stdout,0,RepertModlDesc); printf("  }\n");
						}
					} else if(au_moins_un_rep_lu) {
						while(vl < repart->nbdonnees) {
							item = ItemJusquA(' ',&r);
							if(item[0]) { sscanf(item,"%hx",&lu); repart->ident[vl++] = lu; }
							else break;
						};
						repart->nbident = vl;
					}
				}
			}
			fclose(f); f = 0;
			if(g) { f = g; g = 0; } else f = 0;
		} while(f);
	} else /* Fichier des repartiteurs inaccessible */ {
		printf("\n! Impossible de lire les repartiteurs d'apres:\n  %s\n  (%s)\n",
			fichier,strerror(errno));
		nb_erreurs++;
	}
	if(CompteRendu.repart.capa) {
		printf("  . %d repartiteur%s connu%s\n",Accord2s(RepartNb));
		printf("  . Capacite totale: %d voie%s\n",Accord1s(vt));
	}

	if(!ModeleRepNb) {
		RepartModlStandard();
		strcpy(RepartiteurFichierModeles,"ModelesRepartiteurs");
		RepertoireIdentique(RepartiteurFichierLu,RepartiteurFichierModeles,nom_complet);
		ArgPrint(nom_complet,RepertModlDesc);
	}
	i = 0;
	if(CompteRendu.repart.select) printf("  * Modification des selecteurs:");
	for(modl=0; modl<ModeleRepNb; modl++) if(ModeleRep[modl].selecteur.type == REP_SELECT_NEANT) {
		// ModeleRep[modl].selecteur.type = REP_SELECT_NEANT;
		ModeleRep[modl].selecteur.max = 0;
		switch(ModeleRep[modl].famille) {
		  case FAMILLE_CLUZEL:
			if(ModeleRep[modl].version > 1.05) {
				ModeleRep[modl].selecteur.type = REP_SELECT_NUMER;
				ModeleRep[modl].selecteur.max = 1; /* sauf si ICA (=4), ou Chassis avec MiG (=2) */
			};
			break;
		  case FAMILLE_OPERA:
			ModeleRep[modl].selecteur.type = REP_SELECT_NUMER; ModeleRep[modl].selecteur.max = 1; break;
		  case FAMILLE_CALI: case FAMILLE_NI:
			ModeleRep[modl].selecteur.type = REP_SELECT_CANAL; ModeleRep[modl].selecteur.max = 1; break;
		  case FAMILLE_IPE:
			ModeleRep[modl].selecteur.type = REP_SELECT_CANAL; ModeleRep[modl].selecteur.max = 64; break;
		}
		if(CompteRendu.repart.select && (ModeleRep[modl].selecteur.type != REP_SELECT_NEANT)) {
		   if(!i) printf("\n"); i++;
			printf("    . %s [famille %s]: selection au niveau '%s'\n",
			ModeleRep[modl].nom,RepartFamilleListe[ModeleRep[modl].famille],RepartNomSelecteur[ModeleRep[modl].selecteur.type]);
		}
	}
	if(CompteRendu.repart.select && !i) printf(" aucune\n");

	/* refait ici car RepartNb==1 pour le premier!! */
	if(RepartNb > 1) {
		repart = &(Repart[0]);
		if(repart->categ == DONNEES_STD) {
			for(l=0; l<repart->nbentrees; l++) if(repart->entree[l].adrs)
				RepartiteurCodeEntree(repart,l,((TypeNumeriseur *)(repart->entree[l].adrs))->code_rep);
		}
	}
	for(rep=0; rep<RepartNb; rep++) if((Repart[rep].famille != FAMILLE_MEDIA) && (Repart[rep].interf == INTERF_IP)) {
		char adrs[80];
		repart = &(Repart[rep]);
		RepartIpEcriturePrete(repart,"  ");
		sprintf(adrs,"%d",PortLectRep+rep);
		MediaDefini(&(repart->lec),repart->nom,adrs,MEDIA_UDP_RD);
		MediaListeAjouteRef(SambaMedia,&(repart->lec));
	}
	// strcat(strcpy(nom_complet,HomeDir),"Desktop/Repartiteurs");
	// RepartiteursEcrit(nom_complet);
	// MediaListeDump(SambaMedia,"* Liste des media de repartiteurs");

	mRepartiteurClique = MenuLocalise(iRepartiteurClique);

	pRepartChercheIP = PanelCreate(3);
	PanelText(pRepartChercheIP,"Reseau (a.b.c)",RepartChercheIPnet,16);
	PanelShort(pRepartChercheIP,"Premiere adresse",&RepartChercheIPdep);
	PanelShort(pRepartChercheIP,"Derniere adresse",&RepartChercheIParr);
	pRepartEtat = PanelCreate(REPART_MAX);

	PanelSupport(pRepartEtat,WND_CREUX); PanelMode(pRepartEtat,PNL_RDONLY|PNL_DIRECT);
	mRepartEtat = MenuLocalise(0);
	MenuColumns(mRepartEtat,REPART_NBETAPES); OpiumSupport(mRepartEtat->cdr,WND_PLAQUETTE);
	RepartHeure = DateHeure();  /* recopie l'adresse interne, donc pas besoin de re-affectation */
	pRepartHeure = PanelCreate(1);
	i = PanelText(pRepartHeure,"Verifie a",RepartHeure,10);
	// PanelItemSelect(pRepartHeure,i,0);
	PanelSupport(pRepartHeure,WND_CREUX);	PanelMode(pRepartHeure,PNL_RDONLY|PNL_DIRECT);
	bRepartEtat = BoardCreate();

#ifndef PERIMETRE_AVEC_PLANCHE
	pRepartLocal = PanelCreate(REPART_MAX);
#endif
#ifdef REPART_SELECTION
	pRepartSelecte = PanelCreate(REPART_MAX);
#endif
	RepartiteurMajListes();

	pRepartChoix = PanelCreate(2);
	PanelList(pRepartChoix,"Repartiteur",RepartListe,&RepartDemande,REPART_NOM);
	PanelInt(pRepartChoix,"Entree",&RepartEntreeDemandee);

	pRepartIpeChoixEntree = PanelCreate(2);
	PanelShort(pRepartIpeChoixEntree,"FLT #",&RepartIpeFlt);
	PanelShort(pRepartIpeChoixEntree,"fibre",&RepartIpeFibre);

	// printf("* Definition des repartiteurs:\n"); for(rep=0; rep<RepartNb; rep++) RepartiteurImprime(Repart+rep);
//	printf("(%s) %s.%d <- %s (0x%0X)\n",__func__,Repart[6].nom,1,((TypeNumeriseur *)(Repart[6].entree[0].adrs))->nom,(hexa)(Repart[6].entree[0].adrs));
	return(nb_erreurs);
}
/* ========================================================================== */
void RepartiteurMajListes() {
#ifndef PERIMETRE_AVEC_PLANCHE
	int rep,i;
	PanelErase(pRepartLocal);
	PanelColumns(pRepartLocal,((RepartNb-1)/50)+1);
	for(rep=0; rep<RepartNb; rep++) {
		i = PanelBool(pRepartLocal,Repart[rep].nom,&(Repart[rep].local));
		PanelItemColors(pRepartLocal,i,SambaRougeVert,2);
	}
#endif
#ifdef REPART_SELECTION
	#ifdef PERIMETRE_AVEC_PLANCHE
		int rep,i;
	#endif
	PanelErase(pRepartSelecte);
	for(rep=0; rep<RepartNb; rep++) {
		i = PanelBool(pRepartSelecte,Repart[rep].nom,&(Repart[rep].select));
		PanelItemColors(pRepartSelecte,i,SambaJauneVertOrangeBleu,2); // PanelItemSouligne(p,i,0);
	}
#endif
}
/* ========================================================================== */
static char RepartiteurBrancheIdent(TypeRepartiteur *repart) {
	int k,l; int vl,bb,vsnid,type,modl;  char change;
	shex ident; short vpn;
	shex d1; int voies_par_mot;
	TypeNumeriseur *numeriseur;

	// printf("  . Utilise %d identifieur%s connecte%s a %s\n",Accord2s(repart->nbident),repart->nom);
	if(repart->famille == FAMILLE_OPERA) OperaDimensions(repart->r.opera.mode,&d1,&voies_par_mot,&vpn);
	else vpn = repart->voies_par_numer;
	modl = repart->type;
	change = 0;
	k = 0; l = 0; vl = 0;
	while(k < repart->nbident) {
		if(l >= repart->maxentrees) break;
		if(repart->famille == FAMILLE_IPE) vpn = IpeVpn[repart->r.ipe.modeBB[l]];
		ident = repart->ident[k] & 0xFFF;
		// printf("    . Ident #%d: %03x, entree #%d\n",k,ident,l);
		if(ident == 0xFFF) {
			printf("  ! L'identifieur %04X (pour l'entree %s[%d]) rend la suite inutilisable\n",ident,repart->nom,l+1);
			break;
		} else if(ident) {
			numeriseur = 0;
			for(bb=0; bb<NumeriseurNb; bb++) if(ident == Numeriseur[bb].ident) break;
			if(bb < NumeriseurNb) {
				numeriseur = &(Numeriseur[bb]);
				numeriseur->entree = l;
				if(numeriseur != (TypeNumeriseur *)repart->entree[l].adrs) {
					numeriseur->repart = repart;
					RepartiteurCodeEntree(repart,l,numeriseur->code_rep);
					change = 1;
				}
			} else {
				printf("  ! Le numeriseur connecte a l'entree %s[%d] est inconnu (identifieur: %04X)\n",repart->nom,l+1,ident);
				if(NumeriseurNb < NUMER_MAX) {
					vsnid = (ident >> 8) & 0xF;
					if(vsnid <= NUMER_BB_VSNMAX) type = NumeriseurTypeBB[vsnid]; else type = -1;
					if(type >= 0) {
						numeriseur = &(Numeriseur[NumeriseurNb]);
						NumeriseurNeuf(numeriseur,type,ident,repart);
						numeriseur->bb = NumeriseurNb;
						numeriseur->entree = l;
						printf("  . Ce numeriseur est connecte a l'entree %s[%d]\n",repart->nom,l+1);
						change = 1;
						RepartiteurCodeEntree(repart,l,numeriseur->code_rep);
						NumeriseurNb++;
						NumeriseurTermineAjout();
					} else printf("  ! L'identifieur indique une version inconnue: %d. Numeriseur ignore\n",vsnid);
				} else printf("  ! Pas plus de %d numeriseur%s connu%s. Numeriseur ignore\n",Accord2s(NUMER_MAX));
			}
			if(change) {
				repart->entree[l].adrs = (TypeAdrs)numeriseur;
				repart->entree[l].voies = vpn;
				if((repart->famille == FAMILLE_IPE) && numeriseur) {
					repart->r.ipe.modeBB[l] = (numeriseur->vsnid > 1)? IPE_BB2: IPE_BB1;
					repart->r.ipe.flt[IPE_FLT_NUM(l)].branchee = repart->r.ipe.flt[IPE_FLT_NUM(l)].enabled = 1;
				}
			}
		} else if(repart->entree[l].adrs) {
			repart->entree[l].adrs = 0;
			repart->entree[l].voies = 0;
			change = 1;
		}
		// if(numeriseur) printf("    = %s\n",numeriseur->nom); else printf("    = pas de numeriseur connecte\n");
		//- repart->entree[l].nbident = vpn;
		if(ModeleRep[modl].regroupe_numer) k += vpn; else k++;
		l++;
	};
	if(repart->nbentrees != l) { repart->nbentrees = l; change = 1; }
	for(l=repart->nbentrees; l<repart->maxentrees; l++) repart->entree[l].adrs = 0;
	// printf("    . Recherche terminee pour %s: %d entree%s\n",repart->nom,Accord1s(l));
	return(change);
}
/* ========================================================================== */
char RepartAjusteIdent() {
	int vi,bb,rep,l,vsnid,type;
	shex ident; short vpn;
	shex d1; int voies_par_mot;
	TypeRepartiteur *repart; TypeNumeriseur *numeriseur;
	char change;

	change = 0;
	for(vi=0; vi<VoieIdentMax; vi++) if(SambaEchantillon[vi].voie == 0) {
		ident = VoieIdent[vi].lu & 0xFFF;
		if(VoieIdent[vi].resultat == IDENT_ABSENT) numeriseur = 0;
		else if(VoieIdent[vi].resultat == IDENT_IMPREVU) {
			for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].ident == ident) break;
			if(bb < NumeriseurNb) numeriseur = &(Numeriseur[bb]); else numeriseur = 0;
		} else continue;
		rep = SambaEchantillon[vi].rep; l = SambaEchantillon[vi].entree;
		repart = &(Repart[rep]);
		if((VoieIdent[vi].resultat == IDENT_IMPREVU) && !numeriseur) {
			printf("  ! Le numeriseur connecte a l'entree %s[%d] est inconnu (identifieur: %04X)\n",repart->nom,l+1,ident);
			if(NumeriseurNb < NUMER_MAX) {
				vsnid = (ident >> 8) & 0xF;
				if(vsnid <= NUMER_BB_VSNMAX) type = NumeriseurTypeBB[vsnid]; else type = -1;
				if(type >= 0) {
					numeriseur = &(Numeriseur[NumeriseurNb]);
					NumeriseurNeuf(numeriseur,type,ident,repart);
					numeriseur->bb = NumeriseurNb;
					printf("  . Ce numeriseur sera connecte a l'entree %s[%d]\n",repart->nom,l+1);
					change = 1;
					NumeriseurNb++;
					NumeriseurTermineAjout();
				} else printf("  ! L'identifieur indique une version inconnue: %d. Numeriseur ignore\n",vsnid);
			} else printf("  ! Pas plus de %d numeriseur%s connu%s. Numeriseur ignore\n",Accord2s(NUMER_MAX));
		}
		if(numeriseur != (TypeNumeriseur *)repart->entree[l].adrs) {
			RepartiteurBranche(repart,l,numeriseur);
			if(numeriseur) {
				if(repart->famille == FAMILLE_OPERA) OperaDimensions(repart->r.opera.mode,&d1,&voies_par_mot,&vpn);
				else if(repart->famille == FAMILLE_IPE) {
					repart->r.ipe.modeBB[l] = (numeriseur->vsnid > 1)? IPE_BB2: IPE_BB1;
					repart->r.ipe.flt[IPE_FLT_NUM(l)].branchee = repart->r.ipe.flt[IPE_FLT_NUM(l)].enabled = 1;
					vpn = IpeVpn[repart->r.ipe.modeBB[l]];
				} else vpn = repart->voies_par_numer;
				repart->entree[l].voies = vpn;
			} else repart->entree[l].voies = 0;
			change = 1;
		}
	}
	return(change);
}
/* ========================================================================== */
static void RepartiteursAjoutePseudos(int lngr) {
	int bolo,cap,voie; TypeDetecteur *detectr;
	int k; char trouvee;

	trouvee = 0; k = 0;
	for(bolo=0; bolo<BoloNb; bolo++) {
		detectr = &(Bolo[bolo]);
		for(cap=0; cap<detectr->nbcapt; cap++) if((voie = detectr->captr[cap].voie) >= 0) {
			if(VoieManip[voie].pseudo && VoieTampon[voie].lue) {
				TypeComposantePseudo *composante;
				trouvee = 1;
				if(lngr) k = printf("|        | %16s | %-33s "," ",VoieManip[voie].nom);
				composante = VoieManip[voie].pseudo;
				while(composante) {
					if(!(VoieTampon[composante->srce].lue)) break;
					composante = composante->svt;
				}
				if(composante) {
					if(lngr) k += printf("| %-10s ","incomplete");
				} else {
					VoieTampon[voie].rang_sample = SambaEchantillonLngr;
					SambaEchantillon[SambaEchantillonLngr].rep = -1;
					SambaEchantillon[SambaEchantillonLngr].entree = -1;
					SambaEchantillon[SambaEchantillonLngr].voie = voie;
					SambaEchantillonLngr++;
					if(lngr) k += printf("| %-10s ","composee");
				}
				if(lngr) SambaLigneTable(' ',k,lngr);
			}
		}
	}
	if(trouvee && lngr) printf("|________|__________________|___________________________________|____________|\n");
}
/* ========================================================================== */
void RepartCompteDonnees(TypeRepartiteur *repart) {
	int l; short vpn_pour_tous;
	shex d1; int voies_par_mot;

	if(repart->famille == FAMILLE_OPERA) OperaDimensions(repart->r.opera.mode,&d1,&voies_par_mot,&vpn_pour_tous);
	else vpn_pour_tous = repart->voies_par_numer;

	repart->nbdonnees = 0;
	for(l=0; l<repart->nbentrees; l++) {
		if(repart->famille == FAMILLE_IPE) repart->entree[l].voies = IpeVpn[repart->r.ipe.modeBB[l]];
		else repart->entree[l].voies = vpn_pour_tous;
		repart->nbdonnees += repart->entree[l].voies;
	}
}
/* ========================================================================== */
char RepartiteurAdopte(TypeRepartiteur *repart, char verifie, char *explics) {
	char ok; int i,j,sat;

	if(!repart) { sprintf(explics,"repartiteur indefini"); return(0); }
	else {
		ok = 1;
		repart->utile = 1;
		if((repart->interf == INTERF_IP) && verifie) {
			if(repart->valide) {
				if(SambaMaitre) {
					for(j=0; j<Partit[PartitLocale].nbsat; j++) {
						sat = Partit[PartitLocale].sat[j];
						for(i=0; i<Acquis[sat].nbrep; i++) if(Acquis[sat].rep[i] == repart->rep) break;
						if(i < Acquis[sat].nbrep) { Partit[PartitLocale].exter[j] = 1; break; }
					}
					if(j >= Partit[PartitLocale].nbsat) {
						sprintf(explics,"pas lu par la partition locale");
						ok = 0; repart->valide = 0;
					}
				} else if(!RepartIpEcriturePrete(repart,"|    ")) {
					sprintf(explics,"connexion pour ecriture sur %d.%d.%d.%d:%d impossible",IP_CHAMPS(repart->adrs),repart->ecr.port);
					ok = 0; repart->valide = 0;
				}
			} else { sprintf(explics,"hors service"); ok = 0; }
		}
	}
	return(ok);
}
/* ========================================================================== */
int RepartiteursVerifie(char toutes, char verifie, char log) {
	int voie,bolo,cap,bb,rep,j,n,l; char explics[256]; TypeRepartiteur *repart;

	n = 0;
	if(log) {
		printf("|                           Liste des repartiteurs a lire                            |\n");
		printf("|____________________________________________________________________________________|\n");
	}
	for(rep=0; rep<RepartNb; rep++) Repart[rep].utile = 0;
	if(SambaMaitre) for(j=0; j<Partit[PartitLocale].nbsat; j++) Partit[PartitLocale].exter[j] = 0;
	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue && !VoieManip[voie].pseudo) {
//		l = printf("|  . Pour la voie %s (detecteur #%d):",VoieManip[voie].nom,VoieManip[voie].det); SambaCompleteLigne(86,l);
		if(!toutes && (VoieManip[voie].def.trmt[TRMT_AU_VOL].type == TRMT_INVALID)) continue;
		if((bolo = VoieManip[voie].det) < 0) continue; if(!Bolo[bolo].a_lire) continue;
		cap = VoieManip[voie].cap;
//		l = printf("|  > Capteur #%d de %s",cap,Bolo[bolo].nom); SambaCompleteLigne(86,l);
		if((bb = Bolo[bolo].captr[cap].bb.num) < 0) {
			if(log) { l = printf("|  ! Pas de numeriseur pour transmettre la voie %s",VoieManip[voie].nom); SambaCompleteLigne(86,l); }
			n++; continue;
		}
//		l = printf("|  > Numeriseur: %s",Numeriseur[bb].nom); SambaCompleteLigne(86,l);
		if(!(repart = Numeriseur[bb].repart)) {
			if(log) { l = printf("|  ! Pas de repartiteur pour transmettre le numeriseur %s",Numeriseur[bb].nom); SambaCompleteLigne(86,l); }
			n++; continue;
		}
//		l = printf("|  > Repartiteur: %s",repart->nom); SambaCompleteLigne(86,l);
		if(!(repart->utile)) {
			l = 0;
			if(!RepartiteurAdopte(repart,verifie,explics)) {
				if(repart->valide) n++;
				if(log) l = printf("|  ! %s (validite: %d): %s",repart->nom,repart->valide,explics);
			} else if(log) l = printf("|  + %s",repart->nom);
			if(l) SambaCompleteLigne(86,l);
		}
	}
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].status_demande) {
		repart = &(Repart[rep]);
		l = 0;
		if(!RepartiteurAdopte(repart,verifie,explics)) {
			if(log) l = printf("|  ! %s (connexion demandee): %s\n",repart->nom,explics);
			n++;
		} else if(log) l = printf("|  + %s (connexion demandee)",repart->nom);
		if(l) SambaCompleteLigne(86,l);
		break;
	}
	if(n && log) { l = printf("|  !!! %d repartiteur%s en defaut",Accord1s(n)); SambaCompleteLigne(86,l); }
	if(log && SambaMaitre) {
		printf("|____________________________________________________________________________________|\n");
		printf("|                           Liste des acquisitions a lire                            |\n");
		printf("|____________________________________________________________________________________|\n");
		for(j=0; j<Partit[PartitLocale].nbsat; j++) if(Partit[PartitLocale].exter[j]) {
			l = printf("|  + %s",Acquis[Partit[PartitLocale].sat[j]].code); SambaCompleteLigne(86,l);
		}

	}
	if(log) printf("|____________________________________________________________________________________|\n");

	return(n);
}
/* ========================================================================== */
void RepartiteursLocalise() {
	int rep,sat,i,j;

	for(rep=0; rep<RepartNb; rep++) Repart[rep].local = 0;
	if(SambaMode == SAMBA_MAITRE) {
		//- printf("(%s) Partition locale: %d/%d\n",__func__,PartitLocale,PartitNb);
		if(PartitLocale >= 0) {
			for(j=0; j<Partit[PartitLocale].nbsat; j++) {
				sat = Partit[PartitLocale].sat[j];
				for(i=0; i<Acquis[sat].nbrep; i++) Repart[Acquis[sat].rep[i]].local = 1;
			}
		} else for(rep=0; rep<RepartNb; rep++) Repart[rep].local = 1;
	} else for(i=0; i<Acquis[AcquisLocale].nbrep; i++) Repart[Acquis[AcquisLocale].rep[i]].local = 1;
	RepartLocaux = 0; for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) RepartLocal[RepartLocaux++] = rep;
	// for(rep=0; rep<RepartNb; rep++) printf("  | %s: repartiteur %s\n",Repart[rep].nom,Repart[rep].local?"local":"exterieur");

#ifdef REPART_SELECTION
	for(rep=0; rep<RepartNb; rep++) Repart[rep].select = Repart[rep].local;
#endif

}
/* ========================================================================== */
void RepartiteursImprimeNumeriseurs() {
	/* Implique que NumeriseurNb soit deja initialise */
	int rep,l,vbb;
	TypeRepartiteur *repart; TypeNumeriseur *numeriseur;

	for(rep=0; rep<RepartNb; rep++) {
		repart = &(Repart[rep]);
		if(repart->nbentrees) for(l=0; l<repart->nbentrees; l++) {
			if((numeriseur = (TypeNumeriseur *)(repart->entree[l].adrs))) {
				// printf("  . %s lit @%08x [%s] (%d voie%s)",repart->nom,(hexa)numeriseur,numeriseur->nom,Accord1s(numeriseur->nbadc));
				printf("  . %s lit %s (%d voie%s)",repart->nom,numeriseur->nom,Accord1s(numeriseur->nbadc));
				if(repart->nbentrees > 1) printf(" sur son entree %d",l+1);
				if(numeriseur->def.fibre[0]) printf(" via la fibre %s",numeriseur->def.fibre);
				printf("\n");
				if(numeriseur->nbadc) {
					printf("            a savoir: ");
					for(vbb=0; vbb<numeriseur->nbadc; vbb++) {
						if(vbb) printf(",");
						if(numeriseur->voie[vbb] >= 0) printf(" %s",VoieManip[numeriseur->voie[vbb]].nom);
						else printf(" (non cablee)");
					}
					printf("\n");
				}
				numeriseur->nbident = numeriseur->nbadc;
			} else if((repart->famille != FAMILLE_IPE) || repart->r.ipe.flt[IPE_FLT_NUM(l)].branchee) {
				printf("  . %s n'a pas de numeriseur connecte",repart->nom,l+1);
				if(repart->nbentrees > 1) printf(" sur son entree %d\n",l+1); else printf("\n");
			}
		} else printf("  . %s ne lit aucun numeriseur\n",repart->nom);
		if(repart->categ == DONNEES_STAMP) printf("  . %s est sense ne gerer que le timestamp\n",repart->nom);
		if(AcquisNb > 1) printf("  . %s retransmet ses donnees a %s\n",repart->nom,Acquis[repart->sat].code);
	}
}
/* ========================================================================== */
static Cadre bRepartLocaux;
static char RepartPerimetreReponse;
/* .......................................................................... */
static int RepartPerimetreAucun(Menu menu, MenuItem item) {
	int rep;
	for(rep=0; rep<RepartNb; rep++) Repart[rep].local = 0;
	BoardRefreshVars(bRepartLocaux);
	return(0);
}
/* .......................................................................... */
static int RepartPerimetreTous(Menu menu, MenuItem item) {
	int rep;
	for(rep=0; rep<RepartNb; rep++) Repart[rep].local = 1;
	BoardRefreshVars(bRepartLocaux);
	return(0);
}
/* .......................................................................... */
static int RepartPerimetreFerme(Cadre cdr, void *arg) {
//	if(!RepartPerimetreReponse) RepartPerimetreReponse = -1; return(1);
	RepartPerimetreReponse = (bRepartLocaux->code_rendu == 1)? -1: 1;
	return(1);
}
/* ..........................................................................
static int RepartPerimetreAnnule(Menu menu, MenuItem item) {
	RepartPerimetreReponse = -1; return(1);
}
   ..........................................................................
static int RepartPerimetreContinue(Menu menu, MenuItem item) {
	RepartPerimetreReponse = 1; return(1);
}
   ..........................................................................
static TypeMenuItem iRepartPerimetreStopStart[] = {
	{ "Annuler",  MNU_FONCTION RepartPerimetreAnnule },
	{ "Utiliser", MNU_FONCTION RepartPerimetreContinue },
	MNU_END
};
   .......................................................................... */
 static TypeMenuItem iRepartLocaux[] = {
	{ "Aucun", MNU_FONCTION RepartPerimetreAucun },
	{ "Tous",  MNU_FONCTION RepartPerimetreTous },
	MNU_END
 };
/* ========================================================================== */
void RepartiteurAppliquePerimetre() {
	int bb,rep,l,vbb,voie,bolo; char log;
	TypeNumeriseur *numeriseur;

	log = CompteRendu.detectr.perimetre;
	for(bb=0; bb<NumeriseurNb; bb++) Numeriseur[bb].local = Numeriseur[bb].sel = 0;
	for(bolo=0; bolo<BoloNb; bolo++) Bolo[bolo].local = 0;
	for(rep=0; rep<RepartNb; rep++) if((Repart[rep].categ == DONNEES_STD) || (Repart[rep].categ == DONNEES_BB)) {
		printf("  . Repartiteur %s: %s (%d entree%s)\n",Repart[rep].nom,Repart[rep].local? "local":"exterieur",
			Accord1s(Repart[rep].nbentrees));
		for(l=0; l<Repart[rep].nbentrees; l++) if(Repart[rep].local) {
			if((numeriseur = (TypeNumeriseur *)(Repart[rep].entree[l].adrs))) {
				// printf("(%s) Repart#%d (%s), entree %d: lit %s\n",__func__,rep,Repart[rep].nom,l,numeriseur->nom);
				numeriseur->local = numeriseur->sel = Repart[rep].local;
				if(log) printf("    %d> Numeriseur %s: %s (%d ADC%s)\n",l+1,numeriseur->nom,numeriseur->local? "local":"exterieur",Accord1s(numeriseur->nbadc));
				for(vbb=0; vbb<numeriseur->nbadc; vbb++) {
					// printf("(%s) %s[%d] = voie #%d\n",__func__,numeriseur->nom,vbb,numeriseur->voie[vbb]);
					if((voie = numeriseur->voie[vbb]) >= 0) {
						if(log) printf("       %2d| Voie '%s'",vbb+1,VoieManip[numeriseur->voie[vbb]].nom);
					} else { if(log) printf("       %2d| Voie non raccordee\n",vbb+1); continue; }
					// printf("(%s) %s.det = bolo #%d/%d\n",__func__,VoieManip[voie].nom,VoieManip[voie].det,BoloNb);
					if((bolo = VoieManip[voie].det) >= 0) {
						Bolo[bolo].local = Repart[rep].local;
						if(log) printf(", detecteur#%d ('%s') %s\n",bolo+1,Bolo[bolo].nom,Bolo[bolo].local? "local":"exterieur");
						// strcpy(Bolo[bolo].hote,Acquis[sat].code);
					} else if(log) printf(", sans detecteur\n");
				}
			} else if(log) printf("    !> Pas de numeriseur\n");
		}
	}
	if(CompteRendu.detectr.perimetre) {
		printf("  . Resume detecteurs:\n");
		for(bolo=0; bolo<BoloNb; bolo++) printf("    | %s: %s\n",Bolo[bolo].nom,Bolo[bolo].local? "local":"exterieur");
		printf("  . Fin d'evaluation du perimetre\n");
	}
	DetecteursVoisinage(0);
	LectRenovePlanche();
}
/* ========================================================================== */
int RepartiteurRedefiniPerimetre(Menu menu, MenuItem item) {
	char ok;

	ok = (menu == 0);
	if((RepartNb > 1) || !Repart[0].local) {
#ifdef PERIMETRE_AVEC_PLANCHE
		int rep,i; Menu m; Panel p; char local[REPART_MAX];

		if(bRepartLocaux) {
			if(OpiumDisplayed(bRepartLocaux)) OpiumClear(bRepartLocaux);
			BoardTrash(bRepartLocaux);
		} else bRepartLocaux = BoardCreate();
		if(!bRepartLocaux) return(0);
		RepartPerimetreReponse = 0; for(rep=0; rep<RepartNb; rep++) local[rep] = Repart[rep].local;
		MenuColumns((m = MenuLocalise(iRepartLocaux)),2); OpiumSupport(m->cdr,WND_PLAQUETTE);
		BoardAddMenu(bRepartLocaux,m,Dx,Dy,0);
		PanelColumns((p = PanelCreate(RepartNb)),((RepartNb-1)/50)+1);
		PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
		for(rep=0; rep<RepartNb; rep++) {
			i = PanelBool(p,Repart[rep].nom,&(Repart[rep].local));
			PanelItemColors(p,i,SambaRougeVert,2);
		}
		BoardAddPanel(bRepartLocaux,p,Dx,3 * Dy,0);
		// MenuColumns((m = MenuLocalise(iRepartPerimetreStopStart)),2); OpiumSupport(m->cdr,WND_PLAQUETTE);
		// BoardAddMenu(bRepartLocaux,m,OPIUM_EN_DESSOUS_DE p->cdr);
		BoardBoutonText(bRepartLocaux,SambaSelectionText);
		OpiumExitFctn(bRepartLocaux,RepartPerimetreFerme,0);
		OpiumExec(bRepartLocaux);
		if(RepartPerimetreReponse < 1) {
			for(rep=0; rep<RepartNb; rep++) Repart[rep].local = local[rep];
			return(0);
		}
#else
		if(OpiumExec(pRepartLocal->cdr) == PNL_CANCEL) return(0);
#endif
		// for(rep=0; rep<RepartNb; rep++) printf(". %s %s\n",Repart[rep].nom,Repart[rep].local? "local": "exterieur");
		if(Acquis[AcquisLocale].etat.active) {
			Acquis[AcquisLocale].etat.active = 0; NumeriseurEtatDemande = 0;
		} else if(OpiumDisplayed(bNumeriseurEtat)) NumeriseurRapport(0,0);
		RepartiteurAppliquePerimetre();
		SambaModifiePerimetre();
		LectConsignesConstruit();
		// tend a disparaitre: if(OpiumDisplayed(bSchemaManip)) OrgaTraceTout(0,0);
		if(OpiumDisplayed(bOrgaCryostat)) OrgaMagasinRefait();
	} else if(menu) OpiumError("Un seul repartiteur: operation inutile");
	return(ok);
}
/* ========================================================================== */
int RepartiteursElection(char log) {
	int rep,n,l,m;

	if(log) {
	m = printf(" _______________________________________________________________\n");
		printf("|              Liste des repartiteurs utilisables               |\n");
		printf("|_______________________________________________________________|\n");
	}
	n = 0;
	for(rep=0; rep<RepartNb; rep++) {
		Repart[rep].candidat = (Repart[rep].valide && Repart[rep].local); // Repart[rep].nbdonnees == 0 si trigger deporte
		if(Repart[rep].candidat) {
			n++;
			if(log) { l = printf("| %16s | %s",Repart[rep].nom,Repart[rep].actif? "active": "hors service"); SambaCompleteLigne(m,l); }
		}
	}
	if(log) printf("|__________________|____________________________________________|\n");
	return(n);
}
/* ========================================================================== */
void RepartiteursSelectionneVoiesLues(char toutes, char log) {
	int i,k,l,adc,rep,gene,vl,voie,modl,nbregs,reg,pflt,flt;
	int lngr;
	short vpn; // ,saut
	char autre,par_numeriseur,selecteur,trgr,reelle,voie_lue,bourrage,vraiment_absente;
	int n,bloc;
	TypeRepartiteur *repart; TypeNumeriseur *numeriseur;
#define MAX_SEL 1024
#define SEL_NUMER_PAR_OCTET
	byte selcanal[4*MAX_SEL];  hexa selection[MAX_SEL],autorise;
	byte selcanevt[4*MAX_SEL]; hexa selectevt[MAX_SEL];
	byte buffer[65536];
	shex d1; int voies_par_mot;

	if(log) {
		LogBlocBegin();
		lngr =
		printf(" ____________________________________________________________________________\n");
		printf("|                           Selection des voies                              |\n");
		printf("|____________________________________________________________________________|\n");
	l = printf("| * Selection de %s voies",toutes?"toutes les":"certaines"); SambaCompleteLigne(lngr,l);
	for(rep=0; rep<RepartNb; rep++) {
	l = printf("|   . %s: %s",Repart[rep].nom,Repart[rep].utile?"a utiliser":"pas concerne"); SambaCompleteLigne(lngr,l); }
		printf("|____________________________________________________________________________|\n");
		printf("| entree |    numeriseur    | voie                              | statut     |\n");
		printf("|________|__________________|___________________________________|____________|\n");
	} else lngr = 0;

	reg = 0; autorise = 1; flt = 0; k = nbregs = 0;// GCC
	autre = 0;
	SambaEchantillonReel = SambaEchantillonLngr = 0; RepartNbActifs = 0; RepartSeul = 0;
	for(rep=0; rep<RepartNb; rep++) if((Repart[rep].utile && toutes) || !toutes) {
		repart = &(Repart[rep]);
		repart->actif = 0;
		repart->bb = 0;
		if(!toutes) {
			for(l=0; l<repart->nbentrees; l++) {
				if((numeriseur = (TypeNumeriseur *)(repart->entree[l].adrs))) {
					for(adc=0; adc<numeriseur->nbadc; adc++) if((voie = numeriseur->voie[adc]) >= 0) {
						if(VoieTampon[voie].lue || toutes) break;
					}
					if(adc < numeriseur->nbadc) break;
				}
			}
			if((l >= repart->nbentrees) && (repart->categ != DONNEES_STAMP)) continue;
		} else if(repart->famille == FAMILLE_CLUZEL) repart->nbentrees = repart->maxentrees;
		if(log && autre) SambaLigneTable('=',0,lngr);
		autre = 1;
		modl = repart->type;
		if(log) {
			k = printf("| %s (%s, %s=%s)",repart->nom,ModeleRep[modl].nom,InterfaceListe[ModeleRep[modl].interface],repart->parm);
			if(!repart->valide) k += printf(": declare invalide");
			else if(!SambaMaitre) {
				if((repart->interf == INTERF_IP) && !RepartIpEcriturePrete(repart," ")) {
					k += printf(": invalide (pas de reponse)"); repart->valide = 0;
				} else if(repart->categ == DONNEES_STAMP) k += printf(": donnee %s",RepartDonneesListe[repart->categ]);
			}
			SambaLigneTable(' ',k,lngr);
			if(repart->valide && (repart->categ != DONNEES_STAMP)) SambaLigneTable('_',0,lngr);
		}
		repart->premiere_donnee = SambaEchantillonLngr;
		vl = 0;
		if(repart->valide) {
			if(repart->categ == DONNEES_STAMP) repart->actif = 1;
			else {
				if(repart->famille == FAMILLE_NI) { NiTransmissionRaz(log?lngr:0); n = 0; bloc = 0; }
				selecteur = ModeleRep[modl].selecteur.type;
				if(selecteur) {
					nbregs = ModeleRep[modl].selecteur.max;
					if(repart->famille == FAMILLE_CLUZEL) {
						if(PCIedw[repart->adrs.val - 1].type == CARTE_ICA) nbregs = 4;
						else /* (PCIedw[repart->adrs.val - 1].type == CARTE_MIG) */ {
							if(ModeleRep[modl].version < 2.0) nbregs = 1; else nbregs = 2;
						}
					}
					if(nbregs > MAX_SEL) nbregs = MAX_SEL; /* mais ca serait etonnant! */
				#ifdef SEL_NUMER_PAR_OCTET
					if(selecteur == REP_SELECT_CANAL) {
						nbregs *= 4;
						if(repart->famille == FAMILLE_IPE) nbregs = (((nbregs - 1) / IPE_FLT_SIZE) + 1) * IPE_FLT_SIZE;
						for(reg=0; reg<nbregs; reg++) selcanal[reg] = selcanevt[reg] = 0;
					} else
				#endif
					for(reg=0; reg<nbregs; reg++) selection[reg] = selectevt[reg] = 0;
					reg = 0; autorise = 1;
					// printf(": %s, %d registre%s de selection\n",repart->nom,Accord1s(nbregs));
				}
				par_numeriseur = ModeleRep[modl].regroupe_numer;
				for(l=0; l<repart->nbentrees; l++) repart->entree[l].utilisee = REP_DATA_PASVUE;
				if(repart->famille == FAMILLE_OPERA) OperaDimensions(repart->r.opera.mode,&d1,&voies_par_mot,&vpn);
				else vpn = repart->voies_par_numer;
				l = 0; adc = 0; if(repart->famille == FAMILLE_IPE) pflt = -1; else { flt = pflt = 0; }
				forever {
					char numer_txt[16];
					if(repart->famille == FAMILLE_IPE) {
						flt = IPE_FLT_NUM(l);
						if(l == (flt * IPE_FLT_SIZE)) {
							if(toutes) repart->r.ipe.flt[flt].active = (repart->r.ipe.flt[flt].branchee && repart->r.ipe.flt[flt].enabled);
							else {
								repart->r.ipe.flt[flt].active = 0;
								if(repart->r.ipe.flt[flt].enabled) for(i=0; i<IPE_FLT_SIZE; i++) if((numeriseur = (TypeNumeriseur *)(repart->entree[l+i].adrs))) {
									int j; for(j=0; j<numeriseur->nbadc; j++) if((voie = numeriseur->voie[j]) >= 0) {
										if(VoieTampon[voie].lue) break;
									}
									if(j < numeriseur->nbadc) { repart->r.ipe.flt[flt].active = 1; break; }
								}
							}
						}
						if(pflt != flt) {
							if(pflt >= 0) IpeBourreFlt(repart,pflt,log,lngr);
							if(log) {
								if(!(repart->r.ipe.flt[flt].active)) {
									if(pflt >= 0) {
										if(repart->r.ipe.flt[pflt].active) printf("|........|..................|...................................|............|\n");
									}
									printf("| %2d..%2d | FLT#%02d %-7s   ",(flt*IPE_FLT_SIZE)+1,(flt+1)*IPE_FLT_SIZE,flt+1,(repart->r.ipe.flt[flt].enabled)?"standby":"HS");
									printf("|                                   |            |\n");
								} else if(pflt >= 0) printf("|........|..................|...................................|............|\n");
							}
							pflt = flt;
						}
					}
					if(log) {
						k = 0;
						if(toutes) {
							if((repart->famille == FAMILLE_IPE) && !adc) {
								if(repart->r.ipe.modeBB[l]) sprintf(numer_txt,"mode %s",IpeCodesAdcMode[repart->r.ipe.modeBB[l]]);
								else sprintf(numer_txt,"inconnu");
							} else strcpy(numer_txt," ");
						}
					}
					numeriseur = (TypeNumeriseur *)(repart->entree[l].adrs);
					if(numeriseur && (repart->famille == FAMILLE_IPE)) {
						if(!repart->r.ipe.flt[flt].active) numeriseur = 0;
						else if(repart->r.ipe.bourrage == IPE_BOURRE_FIBRE) {
							int j; for(j=0; j<numeriseur->nbadc; j++) if((voie = numeriseur->voie[j]) >= 0) {
								if(VoieTampon[voie].lue) break;
							}
							if(j >= numeriseur->nbadc) numeriseur = 0;
						}
					}
					if(!adc) {
						if((repart->famille == FAMILLE_IPE) && (repart->r.ipe.bourrage != IPE_BOURRE_FIBRE)) vpn = IpeVpn[repart->r.ipe.modeBB[l]];
						repart->entree[l].voies = vpn;
					}
					vraiment_absente = ((selecteur == REP_SELECT_CANAL) || (repart->entree[l].utilisee == REP_DATA_PASVUE));
					voie = -1; voie_lue = 0; bourrage = 0;
					if(toutes) {
						if(repart->famille == FAMILLE_NI) { bloc += NiSelecte1voie(repart,vl,log?lngr:0); n++; }
						if((repart->famille != FAMILLE_IPE) || repart->r.ipe.flt[flt].active) {
							voie = adc; /* implique 'toutes' <=> 'mode == NUMER_MODE_IDENT' */
							voie_lue = 1;
							repart->actif = 1; repart->entree[l].utilisee = REP_DATA_UTILISEE;
							repart->bb = 0; // pour l'instant: sera mis a 1 dans LectIdentBB()
							if(log) {
								if(!k) k = printf("|");
								if(par_numeriseur && adc) k += printf("        | %-16s | voie %2d%-26s | %-10s ",numer_txt,adc+1," ","testee");
								else k += printf(" %4d   | %-16s | voie %2d%-26s | %-10s ",l+1,numer_txt,adc+1," ","testee");
							}
						}
					} else if(numeriseur) {
						if(numeriseur->ident) repart->bb = 1;
					#ifndef SEL_NUMER_PAR_OCTET
						if(par_numeriseur) { if(selecteur == REP_SELECT_CANAL) saut = ((((vpn - 1) / 8) + 1) * 8) - vpn; }
					#endif
						reelle = 1;
						if(adc < numeriseur->nbadc) {
							if((voie = numeriseur->voie[adc]) >= 0) { if(VoieManip[voie].pseudo) reelle = 0; }
						}
						if(reelle) {
							if(log) {
								if(!k) k = printf("|");
								if(par_numeriseur && adc) k += printf("        | %16s "," ");
								else k += printf(" %4d   |%c%-16s ",l+1,(numeriseur->ident)?'.':' ',numeriseur->nom);
							}
							if(adc < numeriseur->nbadc) {
								if((voie = numeriseur->voie[adc]) >= 0) {
									if(log) k += printf("|+%-33s ",VoieManip[voie].nom);
									if(VoieTampon[voie].lue) {
										voie_lue = 1;
										if(repart->famille == FAMILLE_NI) { bloc += NiSelecte1voie(repart,vl,log?lngr:0); n++; }
										repart->actif = 1;
									#ifdef NATURE_PAR_ENTREE
										/* nature ('utilisee') determinee d'apres la premiere voie transmise
										if(repart->entree[l].utilisee == REP_DATA_PASVUE)
											repart->entree[l].utilisee = (VoieTampon[voie].trig.trgr->type == TRGR_EXT)? REP_DATA_EVENTS: REP_DATA_STREAM;
										else if(repart->entree[l].utilisee != REP_DATA_MIXTE) {
											if(((repart->entree[l].utilisee == REP_DATA_STREAM) && (VoieTampon[voie].trig.trgr->type == TRGR_EXT))
											   || ((repart->entree[l].utilisee == REP_DATA_EVENTS) && (VoieTampon[voie].trig.trgr->type != TRGR_EXT)))
												repart->entree[l].utilisee = REP_DATA_MIXTE;
										} */
									#endif
										repart->entree[l].utilisee = REP_DATA_UTILISEE;
										if(log) k += printf("| %-10s ","lue");
									} else if(log) k += printf("| %-10s ",selecteur?"ignoree":"inutile");
								} else if(log) k += printf("| %-33s | %-10s ","(indefinie)",vraiment_absente?" ":"jetee");
							}
							if((repart->famille == FAMILLE_IPE) && (repart->r.ipe.bourrage == IPE_BOURRE_FIBRE) && !voie_lue) bourrage = 1;
							if((adc >= numeriseur->nbadc) && log) {
								if(bourrage) k += printf("| %-33s | %-10s ","(0000)","calage");
								else k += printf("| %-33s | %-10s ","(neant)",(vraiment_absente?" ":"jetee"));
							}
						}
					} else {
						if(log) {
							if((repart->famille != FAMILLE_IPE) || repart->r.ipe.flt[flt].active) {
								if(!par_numeriseur || !adc) {
									if(!k) k = printf("|");
									k += printf(" %4d   | %-16s | %-33s | %-10s ",l+1,"neant"," ",vraiment_absente?" ":"jetee");
								}
							}
						}
					}
					if(log && k) SambaLigneTable(' ',k,lngr);
					//- printf("0] %s[%d] %s\n",repart->nom,l+1,RepDataFmtNoms[repart->entree[l].utilisee]);
					/* !! Les voies dont le trigger est externe ne font pas partie de l'echantillon stream */
					//- printf("(%s) VoieTampon[%d].trig.trgr = @%08X\n",__func__,voie,VoieTampon[voie].trig.trgr);
					trgr = TRGR_INTERNE;
					if(voie >= 0) { if(VoieTampon[voie].trig.trgr) trgr = VoieTampon[voie].trig.trgr->origine; }
					//- printf("(%s) | trgr# = %d\n",__func__,trgr);
					if((toutes || !selecteur || ((selecteur == REP_SELECT_CANAL) && ((voie_lue  && (trgr == TRGR_INTERNE)) || bourrage)))) {
						if((repart->famille != FAMILLE_IPE) || repart->r.ipe.flt[flt].active) {
							if(SambaEchantillonLngr < SAMBA_ECHANT_MAX) {
								VoieTampon[voie].rang_sample = SambaEchantillonLngr;
								SambaEchantillon[SambaEchantillonLngr].rep = rep;
								SambaEchantillon[SambaEchantillonLngr].entree = l;
								SambaEchantillon[SambaEchantillonLngr].voie = voie;
								SambaEchantillonLngr++;
								if(((gene = VoieManip[voie].gene) >= 0) && Gene[gene].mode_actif) repart->avec_gene = 1;
							}
						}
					}
					if(selecteur == REP_SELECT_CANAL) {
					#ifdef SEL_NUMER_PAR_OCTET
						if(voie_lue || toutes) selcanal[reg] |= autorise;
						// printf(": entree %d -> selcanal[%d] = %02X\n",l,reg,selcanal[reg]);
						if((adc + 1) >= vpn) { if(reg < nbregs) { autorise = 1; reg++; } }
						else autorise = (autorise << 1);
					#else
						if(voie_lue || toutes) selection[reg] |= autorise;
						if(autorise == 0x80000000) { if(reg < nbregs) { autorise = 1; reg++; } }
						else autorise = (autorise << 1);
						if(par_numeriseur && saut && (adc == (vpn - 1))) /* infaisable si pas par numeriseur */ {
							autorise = (autorise << saut);
							if(autorise == 0x00000000) { if(reg < nbregs) { autorise = 1; reg++; } }
						}
					#endif
					}
					if(par_numeriseur) { if(++adc >= vpn) { adc = 0; if(++l >= repart->nbentrees) break; } }
					else { if(++l >= repart->nbentrees) { l = 0; if(++adc >= vpn) break; } }
				}
				if(repart->famille == FAMILLE_IPE) IpeBourreFlt(repart,flt,log,lngr);
				if(log) printf("|________|__________________|___________________________________|____________|\n");

				if(selecteur) {
					if(selecteur == REP_SELECT_NUMER) {
					#ifdef NATURE_PAR_ENTREE
						if(!toutes) {
							for(l=0; l<repart->nbentrees; l++) if((numeriseur = (TypeNumeriseur *)(repart->entree[l].adrs))) numeriseur->fmt = REP_DATA_PASVUE;
							//- for(l=0; l<repart->nbentrees; l++) printf("1] %s[%d] %s\n",repart->nom,l+1,RepDataFmtNoms[repart->entree[l].utilisee]);
							for(l=0; l<repart->nbentrees; l++) if(repart->entree[l].utilisee != REP_DATA_PASVUE) {
								if((numeriseur = (TypeNumeriseur *)(repart->entree[l].adrs))) {
									if(numeriseur->fmt == REP_DATA_PASVUE) numeriseur->fmt = repart->entree[l].utilisee;
									else if(numeriseur->fmt != REP_DATA_MIXTE) {
										if(numeriseur->fmt != repart->entree[l].utilisee) {
											numeriseur->fmt = REP_DATA_MIXTE;
											repart->entree[l].utilisee = REP_DATA_MIXTE;
										}
									}
								}
							}
							// pas utilise: for(l=0; l<repart->nbentrees; l++) if(repart->entree[l].utilisee == REP_DATA_MIXTE) repart->entree[l].utilisee = REP_DATA_PASVUE;
							//- for(l=0; l<repart->nbentrees; l++) printf("2] %s[%d] %s\n",repart->nom,l+1,RepDataFmtNoms[repart->entree[l].utilisee]);
						}
					#endif
						for(l=0; l<repart->nbentrees; l++) {
							if((repart->entree[l].utilisee == REP_DATA_UTILISEE) || toutes) selection[reg] |= autorise;
							//- if((repart->entree[l].utilisee == REP_DATA_EVENTS) || toutes) selectevt[reg] |= autorise;
							if(autorise == 0x80000000) { if(reg < nbregs) { autorise = 1; reg++; } }
							else autorise = (autorise << 1);
						}
						if(!toutes) {
							vl = 0;
							l = 0; adc = 0;
							forever {
								if(par_numeriseur) vpn = repart->entree[l].voies;
								//- for(l=0; l<repart->nbentrees; l++) printf("3] %s[%d] %s\n",repart->nom,l+1,RepDataFmtNoms[repart->entree[l].utilisee]);
								if(repart->entree[l].utilisee == REP_DATA_UTILISEE) {
									voie = -1; reelle = 1;
									if((numeriseur = (TypeNumeriseur *)(repart->entree[l].adrs))) {
										if(adc < numeriseur->nbadc) {
											if((voie = numeriseur->voie[adc]) >= 0) { if(VoieManip[voie].pseudo) reelle = 0; }
										}
									}
									trgr = (VoieTampon[voie].trig.trgr)? VoieTampon[voie].trig.trgr->origine: TRGR_INTERNE;
									if(reelle && ((voie < 0) || (trgr == TRGR_INTERNE))) {
										if(SambaEchantillonLngr < SAMBA_ECHANT_MAX) {
											VoieTampon[voie].rang_sample = SambaEchantillonLngr;
											SambaEchantillon[SambaEchantillonLngr].rep = rep;
											SambaEchantillon[SambaEchantillonLngr].entree = l;
											SambaEchantillon[SambaEchantillonLngr].voie = voie;
											SambaEchantillonLngr++;
											if(((gene = VoieManip[voie].gene) >= 0) && Gene[gene].mode_actif) repart->avec_gene = 1;
										}
									}
								} //- else for(l=0; l<repart->nbentrees; l++) printf("4] %s[%d] %s\n",repart->nom,l+1,RepDataFmtNoms[repart->entree[l].utilisee]);
								if(par_numeriseur) { if(++adc >= vpn) { adc = 0; if(++l >= repart->nbentrees) break; } }
								else { if(++l >= repart->nbentrees) { l = 0; if(++adc >= vpn) break; } }
							}
						}
					}
					repart->nbdonnees = SambaEchantillonLngr - repart->premiere_donnee;
					k = printf("| %d voie%s lue%s en stream",Accord2s(repart->nbdonnees)); SambaLigneTable(' ',k,lngr);
					if(!SambaMaitre) {
						// if(log) { k = printf("| Registre de selection: %08X", selection[0]); SambaLigneTable(' ',k,lngr); }
						if(log) k = printf("| Ecrit:");
						if(repart->actif) {
							int j,m; char explics[256];
							switch(repart->famille) {
								case FAMILLE_CLUZEL:
									if(PCIedw[repart->adrs.val - 1].type == CARTE_ICA) {
										for(reg=0; reg<nbregs; reg++) {
										#ifdef AVEC_PCI
											IcaRegWrite(repart->p.pci.port,ICA_REG_SELECT0+reg,selection[reg]);
										#endif
											if(log) k += printf(" %04X%04X",ICA_REG_SELECT0+reg,selection[reg]);
										}
									} else /* (PCIedw[repart->adrs.val - 1].type == CARTE_MIG) */ {
										for(reg=0; reg<nbregs; reg++) {
											CluzelEnvoieBasique(repart->p.pci.port,0,0xBB,reg,selection[reg]);
											if(log) k += printf(" %02X%02X%04X",0xBB,reg,selection[reg]);
										}
									}
									break;
								case FAMILLE_OPERA:
									n = OperaSelecteur(repart,selection[0],Echantillonnage,buffer,&j,explics);
									if(log) {
										if(j > 0) {
											k += printf(" %c",buffer[0]);
											for(i=1; i<j; i++) k += printf(".%02X",buffer[i]);
										} else k += printf(" ..rien! (pas de chemin d'acces a ce repartiteur)");
										if(explics[0]) { SambaLigneTable(' ',k,lngr); k = printf("! %s",explics); }
									}
									break;
								case FAMILLE_CALI:
								#ifdef SEL_NUMER_PAR_OCTET
									n = CaliSelectCanaux(repart,selcanal[0],Echantillonnage,buffer,&j);
								#else
									n = CaliSelecteur(repart,selection[0],Echantillonnage,buffer,&j);
								#endif
									strncpy((char *)CaliSelectMsg,(char *)buffer,32);
									if(log) {
										if(j) k += printf(" '%s'",buffer);
										else k += printf(" ..rien! (pas de chemin d'acces a ce repartiteur)");
									}
									break;
								case FAMILLE_NI: NiAjusteHorloge(repart,bloc,n,log?lngr:0); break;
								case FAMILLE_IPE:
								#ifdef SEL_NUMER_PAR_OCTET
									n = IpeSelectCanaux(repart,reg,selcanal,Echantillonnage,buffer,&m);
								#else
									n = IpeSelecteur(repart,reg+1,selection,Echantillonnage,buffer,&m);
								#endif
									if(log) {
										if(m > 0) {
										#ifdef IPE_MESSAGES_V2
											k += printf(" %c.%c.%d",buffer[0],buffer[1],buffer[2]);
											for(i=3; i<j; ) {
												if((k+5) > lngr) { SambaLigneTable(' ',k,lngr); k = printf("|           "); }
												k += printf("/%d",buffer[i++]);
												k += printf(":%02X",buffer[i++]);
											}
										#endif
										#ifdef IPE_MESSAGES_V3
											i = 0;
											while(i < m) {
												j = i; while((buffer[j] != '/') && (buffer[j] != '\0')) j++;
												buffer[j] = '\0';
												if(!i) k += printf(" %s",buffer);
												else { SambaLigneTable(' ',k,lngr); k = printf("|        %s",buffer+i); }
												i = j + 1;
											}
										#endif
										} else k += printf(" ..rien! (pas de chemin d'acces a ce repartiteur)");
									}
									break;
							}
						} else if(log) k += printf(" ..rien! (pas de voie a lire)");
						if(log) SambaLigneTable(' ',k,lngr);
					}
				} else {
					repart->nbdonnees = SambaEchantillonLngr - repart->premiere_donnee;
					if(log && (repart->nbdonnees > 1)) {
						k = printf("| Les %d voies de ce repartiteur seront toutes lues en stream",repart->nbdonnees);
						SambaLigneTable(' ',k,lngr);
					}
				}
				if(repart->actif) {
					if(ModeleRep[modl].status && !LectSurFichier) {
						SambaEchantillon[SambaEchantillonLngr].rep = rep;
						SambaEchantillon[SambaEchantillonLngr].entree = -1;
						SambaEchantillon[SambaEchantillonLngr].voie = SAMBA_ECHANT_STATUS;
						SambaEchantillonLngr++;
					}
					if(log) {
						k = printf("| Gestion de BB %s",(repart->bb)?"requise":"inutile");
						if(repart->bb) k += printf(" (. = numeriseur de type BB)");
						SambaLigneTable(' ',k,lngr);
					}
				} else {
					k = printf("! %s ne transmet pas de voie: desactive",repart->nom);
					if(log) SambaLigneTable(' ',k,lngr); else printf("\n");
				}
			}
		}
		/* if(!repart->actif) {
					k = printf("! Repartiteur %s inactif alors qu'il est necessaire",repart->nom);
					if(log) SambaLigneTable(' ',k,lngr); else printf("\n");
		} */
		if(repart->actif) {
			if(RepartNbActifs) RepartSeul = 0;
			else if((repart->famille == FAMILLE_CALI) || (repart->famille == FAMILLE_IPE)) RepartSeul = repart;
			RepartNbActifs++;
		}
	} else Repart[rep].actif = 0;
	if(log && (SambaEchantillonLngr > 0)) SambaLigneTable('_',0,lngr);
	SambaEchantillonReel = SambaEchantillonLngr;
	RepartiteursAjoutePseudos(lngr);
	VoiesLues = SambaEchantillonLngr; // a supprimer

	if(SambaEchantillonReel == 0) {
		k = printf("! Aucune voie n'a ete selectionnee en stream");
		if(log) { SambaLigneTable(' ',k,lngr); SambaLigneTable('_',0,lngr); } else printf("\n");
	}
	/* {
	 int rep;
	 printf("| Etat des repartiteurs a la fin de %s\n",__func__);
	 for(rep=0; rep<RepartNb; rep++) printf("| %16s: %s\n",Repart[rep].nom,Repart[rep].actif? "actif": "arrete");
	 } */
}
/* ========================================================================== */
void RepartImprimeEchantillon(char std) {
	int lngr,k;
	int rep,rep_vu,vt,voie; // int prec,vl;

	if(SambaEchantillonLngr > 0) {

		lngr =
		printf(" _______________________________________________________________\n");
		printf("|         Structure du bloc pour 1 echantillon de temps         |\n");
		printf("|_______________________________________________________________|\n");
		if(std)
			k = printf("| rang repartiteur      voie");
		else
			k = printf("| rang repartiteur      transmise"); SambaCompleteLigne(lngr,k);
		printf("|_______________________________________________________________|\n");
		rep_vu = -1; // vl = 0; prec = -1;
		for(vt=0; vt<SambaEchantillonLngr; vt++) {
			k = printf("| %3d: ",vt+1);
			rep = SambaEchantillon[vt].rep;
			if(rep != rep_vu) {
				if(rep >= 0) k += printf("%-16s ",Repart[rep].nom);
				else k += printf("%-16s ","(interne)");
				rep_vu = rep; // prec = -1;
			} else k += printf("|                ");
			voie = SambaEchantillon[vt].voie;
			if(voie == SAMBA_ECHANT_STATUS) k += printf("(status numeriseurs)");
			else if(voie < 0) k += printf("(donnee ignoree)");
			else if(VoieManip[voie].pseudo) k += printf("%s (composee)",VoieManip[voie].nom);
			else {
				if(std) k += printf("%s",VoieManip[voie].nom);
				else {
					// if(voie != prec) { vl = 0; prec = voie; }
					if(Repart[rep].maxentrees > 1)
						k += printf("entree %d, voie %d",SambaEchantillon[vt].entree+1,voie+1);
					else k += printf("voie %d",voie+1);
				}
			}
			// if(!std) vl++;
			SambaCompleteLigne(lngr,k);
		}
		printf("|_______________________________________________________________|\n");
	} else {
		printf(" _______________________________________________________________\n");
		printf("! Bloc pour 1 echantillon de temps: vide                        |\n");
		printf("|_______________________________________________________________|\n");
	}
	rep_vu = -1;
	/* A revoir
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].utile && Repart[rep].valide) {
		TypeRepartiteur *repart; int l;
		repart = &(Repart[rep]);
		for(l=0; l<repart->nbentrees; l++) if(repart->entree[l].utilisee == REP_DATA_EVENTS) {
			if(rep_vu < 0) {
				printf("|           Voies transmises sous forme d'evenements            |\n");
				printf("|_______________________________________________________________|\n");
				printf("| Repartiteur      | entree                                     |\n");
				printf("|__________________|____|_______________________________________|\n");
			}
			if(rep != rep_vu) {
				k = printf("| %-16s | ",repart->nom);
				rep_vu = rep;
			} else k = printf("| .                | ");
			if(!repart->event) {
				k += printf("!! REPARTITEUR INADAPTE"); SambaCompleteLigne(lngr,k);
				k = printf("| .                | ");
			}
			k += printf("%3d | ",l+1);
			if((voie = SambaEchantillon[vt].voie) >= 0) k += printf("%s",VoieManip[voie].nom);
			else k += printf("!! Aucune voie rattachee a cette entree");
			SambaCompleteLigne(lngr,k);
		}
	} */
	if(rep_vu >= 0) printf("|_______________________________________________________________|\n");

}
/* ========================================================================== */
void RepartImprimeCompteurs(TypeRepartiteur *repart, char *prefixe, int lngr) {
	int k; float temps;

	switch(repart->interf) {
	  case INTERF_IP:
		k = printf("%s Echantillon : %d voie%s;",prefixe,Accord1s(repart->nbdonnees));
		if(lngr) SambaLigneTable(' ',k,lngr); else printf("\n");
		temps = (float)(repart->s.ip.duree_trame) / Echantillonnage;
		if((repart->famille == FAMILLE_OPERA) && (repart->nbdonnees < VOIES_PAR_4OCTETS)) {
			int nb_stamps;
			nb_stamps = _nb_data_trame_udp;
			k = printf("%s Trame       : %d echantillon%s [%g ms], %d stamp%s, %d octet%s;",prefixe,
				Accord1s(repart->s.ip.duree_trame),temps,Accord1s(nb_stamps),Accord1s(repart->dim.trame));
				if(lngr) SambaLigneTable(' ',k,lngr); else printf("\n");
		} else {
			k = printf("%s Trame       : %d echantillon%s [%g ms], %d octet%s;",prefixe,
				Accord1s(repart->s.ip.duree_trame),temps,Accord1s(repart->dim.trame));
				if(lngr) SambaLigneTable(' ',k,lngr); else printf("\n");
		}
		temps = (float)(repart->s.ip.inc_stamp) * EnSecondes;
		k = printf("%s SynchroD3   : %d echantillon%s [%g s],",prefixe,Accord1s((int)repart->s.ip.inc_stamp),temps);
		if(repart->s.ip.taille_derniere)
			k += printf(" derniere trame: #%d avec %d echantillon%s.",repart->s.ip.tramesD3,Accord1s(repart->s.ip.taille_derniere));
		else k += printf(" derniere trame: #%d.",repart->s.ip.tramesD3-1);
		if(lngr) SambaLigneTable(' ',k,lngr); else printf("\n");
		break;
	}
}
/* ========================================================================== */
void RepartiteurAccepteEntrees(TypeRepartiteur *repart, int nb_bn) {
	int l;

	if(nb_bn > repart->maxentrees) nb_bn = repart->maxentrees;
	repart->nbentrees = nb_bn;
	if(!repart->entree) {
		repart->entree = (TypeRepartEntree *)malloc(repart->maxentrees * sizeof(TypeRepartEntree));
		for(l=0; l<repart->maxentrees; l++) repart->entree[l].adrs = 0;
	}
	/* printf("* %s: %d detecteur%s de %d voie%s, soit %d voie%s a lire\n",repart->nom,
		   repart->nbentrees,(repart->nbentrees > 1)?"s":"",
		   repart->voies_par_numer,(repart->voies_par_numer > 1)?"s":"",
		   Accord1s(repart->nbdonnees)); */
}
#ifdef OBSOLETE
/* ========================================================================== */
void RepartiteursBrancheNumeriseurs() /* INUTILISE !?? */ {
	/* Implique que NumeriseurNb soit deja initialise */
	int bb,rep,vi,l,vbb,nb_bn,vsnid,type; char par_numeriseur;
	shex ident;
	TypeRepartiteur *repart; TypeNumeriseur *numeriseur;

	for(rep=0; rep<RepartNb; rep++) {
		repart = &(Repart[rep]);
		switch(repart->categ) {
		  case DONNEES_BB:
			par_numeriseur = ModeleRep[repart->type].regroupe_numer;
			nb_bn = ((repart->nbident - 1) / repart->voies_par_numer) + 1;
			RepartiteurAccepteEntrees(repart,nb_bn);
			repart->nbdonnees = repart->nbident;
			ident = 0;
			for(vi=0; vi<repart->nbident; vi++) {
				if(par_numeriseur) {
					l = vi / repart->voies_par_numer; vbb = vi - (l * repart->voies_par_numer);
				} else {
					vbb = vi / nb_bn; l = vi - (vbb * nb_bn);
				}
				if(l >= repart->maxentrees) {
					printf("  ! %s: %d identifieur%s, mais %d numeriseur%s permis\n",repart->nom,Accord1s(repart->nbident),Accord1s(repart->maxentrees));
					continue;
				}
				if(!vbb) {
					ident = repart->ident[vi] & 0xFFF;
					if(!ident || (ident == 0xFFF)) continue;
					for(bb=0; bb<NumeriseurNb; bb++) if(ident == Numeriseur[bb].ident) break;
					if(bb < NumeriseurNb) {
						numeriseur = &(Numeriseur[bb]);
						if(repart->entree[l].adrs != (TypeAdrs)numeriseur) {
							if(repart->entree[l].adrs) {
								printf("  ! Changement de branchement pour %s[%d]=%04X: %s est remplace par %s\n",
									   repart->nom,vi,ident,((TypeNumeriseur *)repart->entree[l].adrs)->nom,numeriseur->nom);
							}
							printf("  . %s lit %s (%d voie%s) sur son entree %d\n",repart->nom,numeriseur->nom,Accord1s(numeriseur->nbadc),l+1);
							if(numeriseur->nbadc) {
								printf("            a savoir:");
								for(vbb=0; vbb<numeriseur->nbadc; vbb++) {
									if(vbb) printf(",");
									if(numeriseur->voie[vbb] >= 0) printf(" %s",VoieManip[numeriseur->voie[vbb]].nom);
									else printf(" (non cablee)");
								}
								printf("\n");
							}
							RepartiteurBranche(repart,l,numeriseur);
						}
					} else {
						printf("  ! Pas de numeriseur connu a raccorder a %s[%d] (identifieur: %04X)\n",repart->nom,vi,ident);
						if(NumeriseurNb < NUMER_MAX) {
							numeriseur = &(Numeriseur[NumeriseurNb]);
							vsnid = (ident >> 8) & 0xF;
							if(vsnid <= NUMER_BB_VSNMAX) type = NumeriseurTypeBB[vsnid]; else type = -1;
							if(type >= 0) {
								NumeriseurNeuf(numeriseur,type,ident,repart);
								numeriseur->bb = NumeriseurNb;
								RepartiteurBranche(repart,l,numeriseur);
								NumeriseurNb++;
								NumeriseurTermineAjout();
								/*++ numeriseur->fischer = 0;
							#if (CABLAGE_CONNECT_MAX <= 256)
								OpiumReadByte("Connecteur (0=neant)",&(numeriseur->fischer));
							#else
								OpiumReadShort("Connecteur (0=neant)",&(numeriseur->fischer));
							#endif
								 if(numeriseur->def.serial == 0xFF)
								 snprintf(numeriseur->fichier,MAXFILENAME,"%s",ModeleBN[type].nom);
								else snprintf(numeriseur->fichier,MAXFILENAME,"%s_%02d",ModeleBN[type].nom,numeriseur->def.serial);
								NumeriseurLitSeulement(numeriseur); */
							} else printf("  ! L'identifieur indique une version inconnue: %d. Numeriseur ignore\n",vsnid);
						} else printf("  ! Pas plus de %d numeriseur%s connu%s. Numeriseur ignore\n",Accord2s(NUMER_MAX));
					}
				}
				// if((numeriseur = (TypeNumeriseur *)repart->entree[l].adrs)) repart->donnee[vi] = numeriseur->voie[vbb];
			}
			break;
		  case DONNEES_STD:
			if(repart->nbentrees) for(l=0; l<repart->nbentrees; l++) {
				if((numeriseur = (TypeNumeriseur *)(repart->entree[l].adrs))) {
					printf("  . %s lit %s (%d voie%s) sur son entree %d\n",repart->nom,numeriseur->nom,Accord1s(numeriseur->nbadc),l+1);
					if(numeriseur->nbadc) {
						printf("            a savoir: ");
						for(vbb=0; vbb<numeriseur->nbadc; vbb++) {
							if(vbb) printf(",");
							if(numeriseur->voie[vbb] >= 0) printf(" %s",VoieManip[numeriseur->voie[vbb]].nom);
							else printf(" (non cablee)");
						}
						printf("\n");
					}
				} else printf("  . %s n'a pas de numerisation definie sur son entree %d\n",repart->nom,l+1);
			} else printf("  . %s ne lit aucun numeriseur\n",repart->nom);
			break;
		  case DONNEES_STAMP:
			printf("  . %s ne transmet que le timestamp\n",repart->nom);
			break;
		  default:
			printf("  . %s ne sert a rien\n",repart->nom);
			break;
		}
	}
}
#endif
/* ========================================================================== */
static char RepartiteursProcedure(TypeRepartiteur *repart, TypeRepartProc *proc) {
	char ok;
	int i,l;
	PciBoard pci; TypePciReg *reg;
	TypeIpActn *actn;
	char ligne[256],template[256],*script,commande[256],*pswd,*nom_simple; int rc; FILE *f,*g;
	char nom_complet[MAXFILENAME];

	ok = 1;
	printf("            . Execution de %d instruction%s\n",Accord1s(proc->ip.nbactn));
	if(repart->interf == INTERF_PCI) {
		pci = repart->p.pci.port;
		for(i=0; i<proc->pci.nbregs; i++) {
			reg = &(proc->pci.reg[i]);
			printf("            . ecrit: %02X.%02X.%04X\n",reg->adrs,reg->ssadrs,reg->valeur);
			CluzelEnvoieBasique(pci,0,reg->adrs,reg->ssadrs,reg->valeur);
		}
	} else if(repart->interf == INTERF_IP) {
		if(repart->valide) {
			for(i=0; i<proc->ip.nbactn; i++) {
				actn = &(proc->ip.actn[i]);
				SambaAjoutePrefPath(nom_complet,actn->fichier);
				switch(actn->type) {
					case IPACTN_FTP:
						if(repart->p.ip.pswd[0]) pswd = repart->p.ip.pswd; else pswd = "x";
						printf("            . ftp %s, dest=%s\n",actn->fichier,actn->dest);
						if(RepertoireExiste(nom_complet)) {
							sprintf(commande,"cd %s; ftp -u ftp://%s:%s@%d.%d.%d.%d%s *",
								nom_complet,repart->p.ip.user,pswd,IP_CHAMPS(repart->adrs),actn->dest);
						} else {
							nom_simple = rindex(nom_complet,SLASH);
							if(nom_simple) nom_simple++; else nom_simple = nom_complet;
							l = strlen(actn->dest); if(actn->dest[l-1] == SLASH) actn->dest[l-1] = '\0';
							sprintf(commande,"ftp -u ftp://%s:%s@%d.%d.%d.%d%s/%s %s",
								repart->p.ip.user,pswd,IP_CHAMPS(repart->adrs),actn->dest,nom_simple,nom_complet);
						}
						// printf("              = %s <CR>\n",commande);
						printf("............. compte-rendu d'execution .........................................\n");
						rc = system(commande);
						printf("............. execution terminee, status = %-4d ................................\n",rc);
						break;
					case IPACTN_TELNET:
						printf("            . telnet %s\n",actn->fichier);
						if((f = fopen(nom_complet,"r"))) {
							strcpy(template,"IpShellScript_XXXX");
							script = mktemp(template);
							if(script) {
								struct tm heure_eclatee;
								RepertoireCreeRacine(script);
								if((g = fopen(script,"w"))) {
									time_t heure_compactee;
									if(repart->p.ip.user[0]) {
										fprintf(g,"sleep 1\n");
										fprintf(g,"echo %s\n",repart->p.ip.user);
										if(repart->p.ip.pswd[0]) {
											fprintf(g,"sleep 1\n");
											fprintf(g,"echo %s\n",repart->p.ip.pswd);
										}
									}
									while(LigneSuivante(ligne,256,f)) {
										if(!strncmp(ligne,"sleep",5)) fputs(ligne,g);
										else {
											l = strlen(ligne); if(ligne[l-1] == '\n') ligne[l-1] = '\0';
											fprintf(g,"sleep 1\n");
											fprintf(g,"echo \"%s\"\n",ligne);
										}
									}
									fprintf(g,"sleep 1\n");
									fclose(g);
									sprintf(commande,"chmod a+x %s; ./%s | telnet %d.%d.%d.%d",script,script,IP_CHAMPS(repart->adrs));
									// printf("              : %s <CR>\n",commande);
									if(actn->duree > 0) {
										time(&heure_compactee);
										heure_compactee += actn->duree;
										memcpy(&heure_eclatee,localtime(&heure_compactee),sizeof(struct tm));
									}
									printf("............. compte-rendu d'execution .........................................\n");
									rc = system(commande);
									printf("............. execution terminee, status = %-4d ................................\n",rc);
									if(actn->duree > 0) {
										time_t heure_courante;
										printf("              - attente jusqu'a %02d:%02d:%02d\n",heure_eclatee.tm_hour,heure_eclatee.tm_min,heure_eclatee.tm_sec);
										OpiumError("Fin de la procedure a %02d:%02d:%02d\n",heure_eclatee.tm_hour,heure_eclatee.tm_min,heure_eclatee.tm_sec);
										do { sleep(1); } while(time(&heure_courante) < heure_compactee);
										printf("              - attente terminee\n");
									}
								} else printf("              ! fichier %s inutilisable (%s). Action abandonnee\n",script,strerror(errno));
								remove(script);
							} else printf("              ! fichier %s inutilisable (%s). Action abandonnee\n",template,strerror(errno));
							fclose(f);
						} else printf("              ! fichier %s inutilisable (%s). Action abandonnee\n",nom_complet,strerror(errno));
						break;
					case IPACTN_SSH:
						if(actn->directe) {
							printf("            . ssh %s (direct)\n",actn->fichier);
							sprintf(commande,"ssh -f %s@%d.%d.%d.%d \"%s\" &",repart->p.ip.user,IP_CHAMPS(repart->adrs),actn->fichier);
							printf("............. compte-rendu d'execution .........................................\n");
							printf("            > %s\n",commande);
							rc = system(commande);
							printf("............. execution terminee, status = %-4d ................................\n",rc);
						} else {
							printf("            . ssh %s (batch)\n",actn->fichier);
							printf("............. compte-rendu d'execution .........................................\n");
							if((f = fopen(nom_complet,"r"))) {
								while(LigneSuivante(ligne,256,f)) {
									if(!strncmp(ligne,"sleep",5)) {
										int secs;
										sscanf(ligne+6,"%d",&secs);
										if(secs && (secs <= 120)) sleep(secs);
									} else {
										l = strlen(ligne); if(ligne[l-1] == '\n') ligne[l-1] = '\0';
										sprintf(commande,"ssh %s@%d.%d.%d.%d \"%s\"",repart->p.ip.user,IP_CHAMPS(repart->adrs),ligne);
										rc = system(commande);
									}
								}
								fclose(f);
							} else printf("              ! fichier %s inutilisable (%s). Action abandonnee\n",nom_complet,strerror(errno));
							printf("............. execution terminee ...............................................\n");
						}
						break;
					case IPACTN_SCP:
						printf("            . scp %s %s\n",actn->fichier,actn->dest);
						if(RepertoireExiste(nom_complet)) {
							sprintf(commande,"cd %s; scp * %s@%d.%d.%d.%d:%s",nom_complet,repart->p.ip.user,IP_CHAMPS(repart->adrs),actn->dest);
						} else {
							nom_simple = rindex(nom_complet,SLASH);
							if(nom_simple) nom_simple++; else nom_simple = nom_complet;
							l = strlen(actn->dest); if(actn->dest[l-1] == SLASH) actn->dest[l-1] = '\0';
							sprintf(commande,"scp %s %s@%d.%d.%d.%d:%s/%s",nom_complet,repart->p.ip.user,IP_CHAMPS(repart->adrs),actn->dest,nom_simple);
						}
						printf("............. compte-rendu d'execution .........................................\n");
						rc = system(commande);
						printf("............. execution terminee, status = %-4d ................................\n",rc);
						break;
					case IPACTN_SHELL:
						printf("            . shell %s\n",actn->fichier);
						sprintf(commande,"bash %s",actn->fichier);
						printf("............. compte-rendu d'execution .........................................\n");
						rc = system(commande);
						printf("............. execution terminee, status = %-4d ................................\n",rc);
						break;
				}
			}
		} else printf("            ! repartiteur invalide\n");
	} else {
		printf("           ! INTERFACE NON GEREE\n");
		ok = 0;
	}
	return(ok);
}
/* ========================================================================== */
static char Repartiteur1Init(TypeRepartiteur *repart, REPART_ETAPES etape, float echantillonage, char local, char avec_opium) {
	int modl; float version;
	char ok; int num;

	if(!repart) { printf("! Repartiteur indefini\n"); return(-1); }
	if(local && !(repart->local)) /* anciennement select */ return(-1);
	modl = repart->type; version = ModeleRep[modl].version;
	if(etape != REPART_STOPPE) printf("          > %s (%s v%.1f)",repart->nom,ModeleRep[modl].nom,version);
	if(repart->interf == INTERF_PCI) {
		num = repart->adrs.val - 1;
		if(num >= PCInb) {
			if(etape != REPART_STOPPE) printf(" (acces PCI #%d, CARTE ABSENTE)\n",num + 1);
			printf("            !!! Operation impossible\n");
			return(0);
		}
		if(etape != REPART_STOPPE) printf(" @PCI%d via carte %s\n",num + 1,LectViaNom[PCIedw[num].type]);
	} else if(repart->interf == INTERF_IP) {
		if(etape != REPART_STOPPE) printf(" @IP %d.%d.%d.%d:%d\n",IP_CHAMPS(repart->adrs),repart->ecr.port);
		if(!RepartIpEcriturePrete(repart,"            ")) {
			printf("            !!! Connexion pour ecriture sur %d.%d.%d.%d:%d impossible\n",IP_CHAMPS(repart->adrs),repart->ecr.port);
			return(0);
		}
	} else if(repart->interf == INTERF_FILE) { if(etape != REPART_STOPPE) printf(" dans %s\n",repart->parm); }
	else if(etape != REPART_STOPPE) printf("\n");

	ok = 1;
	if(etape == REPART_CHARGE) {
		if(repart->famille == FAMILLE_CALI) {
			int l;
			l = sprintf(RepartiteurValeurEnvoyee,"p %d %x",PortLectRep+repart->rep,repart->s.ip.relance);
			RepartIpEcrit(repart,RepartiteurValeurEnvoyee,l+1); SambaMicrosec(ModeleRep[modl].delai_msg);
			printf("            . Ecrit: '%s'\n",RepartiteurValeurEnvoyee);
		}
		if(!RepartiteursProcedure(repart,&(repart->i))) ok = 0;
	} else if(etape == REPART_DEMARRE) {
		if(repart->famille == FAMILLE_CLUZEL) {
			char avec_reboot; float version;
			avec_reboot = 0;
			version = ModeleRep[repart->type].version;
			if((version > 1.05) && (version < 2.0) && avec_opium) {
				char texte[80];
				sprintf(texte,"avec Reset de %s",repart->nom);
				if(OpiumReadBool(texte,&avec_reboot) == PNL_CANCEL) goto abandon;
				if(avec_reboot) {
					texte[0] = '\0';
					if(OpiumReadPswd("Code d'acces",texte,16) == PNL_CANCEL) goto abandon;
					if(strcmp(texte,"darkmatr")) goto abandon;
				}
			}
			CluzelReset(repart,avec_opium,avec_reboot);
		} else if(repart->famille == FAMILLE_ARCHIVE) ArchRecommence(repart);
		if(!RepartiteursProcedure(repart,&(repart->d))) ok = 0;
	} else if(etape == REPART_PARMS) {
		if(repart->famille == FAMILLE_OPERA) OperaReset(repart,echantillonage,avec_opium);
		else if(repart->famille == FAMILLE_IPE) IpeSetupFibres(repart);
	} else if(etape == REPART_STOPPE) {
		if(!RepartiteurSuspendTransmissions(repart,1)) ok = 0;
	}
	return(ok);

abandon:
	OpiumError("La commande en cours est abandonnee");
	return(0);
}
/* ========================================================================== */
static int RepartEtatChange(Menu menu, MenuItem item) {
	int k,local,rep; REPART_ETAPES etape;

	k = MenuItemNum(menu,item) - 1;
	etape = k / RepartLocaux;
	local = k - (etape * RepartLocaux);
	rep = RepartLocal[local];
	printf("%s/ %s du repartiteur %s:\n",DateHeure(),RepartEtape[etape],Repart[rep].nom);
	if(etape < REPART_CONNECTE) Repartiteur1Init(&(Repart[rep]),etape,Echantillonnage,1,1);
	else {
		OpiumClear(bRepartEtat);
		RepartiteursAffiche(&(Repart[rep]));
	}
	return(0);
}
/* ========================================================================== */
int RepartiteursInitTous(REPART_ETAPES etape, float echantillonage, Menu menu) {
	int rep; char ok,avec_opium;

	if(menu) { if(AccesRestreint()) return(0); avec_opium = 1; } else avec_opium = 0;
	printf("%s/ %s des repartiteurs locaux:\n",DateHeure(),RepartEtape[etape]);
	ok = 1;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) {
		if(!Repartiteur1Init(&(Repart[rep]),etape,echantillonage,1,avec_opium)) ok = 0;
	}
	printf("          ... %s %s\n",RepartEtape[etape],ok?"termine.": "EN ERREUR !");

	return(0);
}
/* ========================================================================== */
int RepartiteurContacte(Menu menu, MenuItem item) {
	int rep;

	if(!RepartiteurRedefiniPerimetre(0,0)) return(0);
	printf("%s/ %s des repartiteurs locaux sur IP:\n",DateHeure(),"Reconnexion");
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local && (Repart[rep].interf == INTERF_IP)) {
		printf("%s/ . %s:\n",DateHeure(),Repart[rep].nom);
		printf("%s/   * Fermeture de la liaison\n",DateHeure());
		RepartIpEcritureFerme(&(Repart[rep]));
		RepartIpEcriturePrete(&(Repart[rep]),"            ");
	}

	return(0);
}
/* ========================================================================== */
int RepartiteurDemarre(Menu menu, MenuItem item) {
	if(!RepartiteurRedefiniPerimetre(0,0)) return(0);
	RepartiteursInitTous(REPART_CHARGE,Echantillonnage,menu);
	RepartiteursInitTous(REPART_DEMARRE,Echantillonnage,0);
	RepartiteursInitTous(REPART_PARMS,Echantillonnage,0);
	return(0);
}
/* ========================================================================== */
int RepartiteurStoppe(Menu menu, MenuItem item) {
	int rep;
	//	rep = 0;
	//	if(RepartNb > 1) {
	//		if(OpiumReadList("Lequel",RepartListe,&rep,REPART_NOM) == PNL_CANCEL) return(0);
	//	}
	//	Repartiteur1Init(&(Repart[rep]),REPART_STOPPE,0.0,0,0);

	Panel p; char pas_touche[REPART_MAX]; int i;

	PanelColumns((p = PanelCreate(RepartNb)),((RepartNb-1)/50)+1);
	for(rep=0; rep<RepartNb; rep++) {
		pas_touche[rep] = 1;
		i = PanelKeyB(p,Repart[rep].nom,"arreter/laisser",&(pas_touche[rep]),8);
		PanelItemColors(p,i,SambaRougeVert,2);
	}
	if(OpiumExec(p->cdr) == PNL_OK) {
		for(rep=0; rep<RepartNb; rep++) if(!pas_touche[rep]) {
			Repartiteur1Init(&(Repart[rep]),REPART_STOPPE,0.0,0,0);
		}
	}
	PanelDelete(p);

	return(0);
}
/* ========================================================================== */
int RepartDiffuse(Menu menu, MenuItem item) {
	int rep; char type;

	type = -1;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) /* anciennement select */ {
		if(type < 0) {
			type = Repart[rep].type;
			memcpy(&RepartLu,&(Repart[rep]),sizeof(TypeRepartiteur));
		} else if(Repart[rep].type != type) break;
	}
	if(rep < RepartNb) {
		OpiumError("Les types %s et %s sont melanges",ModeleRep[type],ModeleRep[Repart[rep].type]);
		return(0);
	}
	RepartLu.rep = -1;
	RepartiteursAffiche(&RepartLu);

	return(0);
}
/* ========================================================================== */
static int RepartStoppeFpga(Menu menu, MenuItem item) {
	RepartFpgaInter = 1; return(0);
}
/* ========================================================================== */
int RepartChargeFpga(char *explics) {
	int rep,k,nb,fpga,bb; char bof,tilt,pas_fini,en_test; int *stade; char *msg,*vue; int dy;
	int nbsleeps;  /* nombre maxi d'attentes avant abandon */
	int timesleep; /* attente en microsecs si pile vide (pas de donnees) */
	int xmit; /* nombre d'erreurs de transmissions permises avant abandon (IPE) */
	TypeRepartiteur *repart;
	Cadre planche; Instrum *niveau; Panel p;
	TypeInstrumGlissiere fenetre = { 225, 8, 1, 0, 3, 1 };

	fpga = ModeleBN[ModlNumerChoisi].fpga;
	if((fpga < 0) || (fpga >= NUMER_FPGA_TYPES)) return(0);
	repart = 0; // GCC

	printf("%s/ Chargement des FPGA code %s (%s):\n",DateHeure(),FpgaModl[fpga].code,FpgaModl[fpga].fichier);
	RepartFpgaInter = 0;
	stade = 0; vue = 0; msg = 0; niveau = 0;
	tilt = 0;
	bof = 1;
	nb = 0;
	for(rep=0; rep<RepartNb; rep++) Repart[rep].charge_fpga = Repart[rep].status_fpga = 0;
	for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].sel && (repart = Numeriseur[bb].repart)) {
		if(!repart->charge_fpga) {
			if(repart->local) {
				if((repart->famille == FAMILLE_OPERA) || (repart->famille ==  FAMILLE_IPE)) {
					printf("%s/ . Utilisation de %s\n",DateHeure(),repart->nom);
					repart->charge_fpga = 1;
					nb++;
				} else printf("%s/ ! Repartiteur %s inadapte\n",DateHeure(),repart->nom);
			} else printf("%s/ ! Repartiteur %s hors champ\n",DateHeure(),repart->nom);
		}
	}
	if(!nb) goto hev;
	vue = Malloc(nb,char);
	stade = Malloc(nb,int);
	if(BancEnTest) msg = 0; else msg = (char *)malloc(nb * REPART_INFOFPGA_DIM);
	niveau = Malloc(nb,Instrum);
	if(!niveau) {
		printf("%s ! Memoire indisponible pour %d octets\n",DateHeure(),nb*(sizeof(int)+sizeof(char)+REPART_INFOFPGA_DIM+sizeof(Instrum))); goto hev;
	}
	bof = 0;

	if(BancEnTest) { p = 0; planche = 0; dy = 0; /* GCC */ } else {
		planche = BoardCreate();
		dy = Dy;
		p = PanelCreate(2 * nb);
	}
	k = 0;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].charge_fpga) {
		repart = &(Repart[rep]);
		if(repart->interf == INTERF_IP) {
			if(repart->famille == FAMILLE_IPE) {
				byte texte[80]; int l;
				l = sprintf((char *)texte,"%s_0x%08X_0x%08X",IPE_REG_WRITE,IPE_SLT_PIXBUS,IpePixBus[repart->r.ipe.chassis]); texte[l] = '\0';
				if(repart->ecr.ip.path >= 0) { RepartIpEcrit(repart,texte,l); SambaMicrosec(ModeleRep[repart->type].delai_msg); }
			}
			RepartIpTransmission(repart,REPART_ENVOI_DETEC,0,1); /* duree -> -1? */
			RepartIpTransmission(repart,REPART_ENVOI_STATUS,repart->s.ip.relance,1);
			repart->status_demande = 1;
			switch(repart->famille) {
				case FAMILLE_OPERA: OperaCmdeFpga(repart,fpga); break;
				case FAMILLE_IPE:   IpeCmdeFpga(repart,fpga);   break;
			}
		} else { repart->status_demande = 0; break; } // si pas IP, pas pris en compte!!
		vue[k] = 0; stade[k] = 0;
		if(BancEnTest) niveau[k] = BancProgression;
		else {
			strcpy(msg+(k*REPART_INFOFPGA_DIM),"inconnu");
			PanelItemSelect(p,PanelText(p,"Status",msg+(k*REPART_INFOFPGA_DIM),REPART_INFOFPGA_DIM),0);
			PanelItemSelect(p,PanelInt(p,"Programme (%)",&(stade[k])),0);
			niveau[k] = InstrumInt(INS_COLONNE,&fenetre,&(stade[k]),0,100); InstrumSupport(niveau[k],WND_CREUX);
			BoardAddInstrum(planche,niveau[k],WndColToPix(50),dy,0); dy += WndLigToPix(2);
		}
		k++;
	}

	if(!BancEnTest) {
		PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX); // PanelMode(p,PNL_DIRECT|PNL_RDONLY);
		BoardAddPanel(planche,p,Dx,Dy,0);
		BoardAddBouton(planche,"Stopper",RepartStoppeFpga,OPIUM_EN_DESSOUS_DE 0);
		OpiumPosition(planche,300,200);
		strcpy(RepartInfoFpgaBB,"inconnu");
		OpiumTitle(planche,"Chargement FPGA");
		OpiumFork(planche);
		OpiumRefresh(planche);
		OpiumUserAction();
	}

	en_test = 0; xmit = 10;
	do {
		pas_fini = 0;
		k = 0;
		for(rep=0; rep<RepartNb; rep++) if(Repart[rep].charge_fpga) {
			if(Repart[rep].simule) {
				sleep(2);
				stade[k] = en_test++ * 5;
				snprintf(RepartInfoFpgaBB,REPART_INFOFPGA_DIM,"%3d/100",stade[k]);
				if(stade[k] < 101) pas_fini = 1;
				if(!BancEnTest) { strcpy(msg+(k*REPART_INFOFPGA_DIM),RepartInfoFpgaBB); OpiumRefresh(niveau[k]->cdr); }
				printf("%s/ . Etat %s simule: %s\n",DateHeure(),repart->nom,RepartInfoFpgaBB);
				k++; continue;
			}
			repart = &(Repart[rep]);
			// printf("          Disponibilite de %s %s..\n",repart->nom,repart->status_demande? "recherchee":"abandonnee");
			if(!repart->status_demande) continue;
			nb = 0;
			timesleep = 500000; /* microsecs */
			nbsleeps = 10; /* fois timesleep */
//			timesleep = 1000000; /* microsecs */
//			nbsleeps = 8; /* fois timesleep */
			if(repart->famille == FAMILLE_OPERA) {
				Structure_trame_status *trame; hexa lngr; int limite;
				gettimeofday(&LectDateRun,0); limite = LectDateRun.tv_sec + (timesleep * nbsleeps / 1000000);
				trame = (Structure_trame_status *)repart->fifo32;
				lngr = sizeof(TypeSocket);
				do {
					nb = RepartIpLitTrame(repart,trame,sizeof(Structure_trame_status));
					// printf("%s.%06d/ Lecture trame de %s: %d octet%s (erreur %d/%d)\n",DateHeure(),LectDateRun.tv_usec,repart->nom,Accord1s(nb),errno,EAGAIN);
					if(nb == -1) { if(errno == EAGAIN) SambaMicrosec(timesleep); else break; }
					else if(nb > 0) {
						if(BigEndianOrder) ByteSwappeInt((hexa *)trame);
						// printf("               > trame #%x/%x\n",OPERA_TRAME_NUMERO(trame),OPERA_TRAME_STS);
						if(OPERA_TRAME_NUMERO(trame) == OPERA_TRAME_STS) {
							vue[k] = 1;
							if(BigEndianOrder) ByteSwappeIntArray((hexa *)trame+1,(nb - 1) / sizeof(Ulong));
							OperaRecopieStatus(trame,repart);
							printf("%s/ . Etat %s: %04X (%s)\n",DateHeure(),(hexa)repart->nom,trame->status_opera.micro_bbv2,RepartInfoFpgaBB);
							stade[k] = trame->status_opera.micro_bbv2 >> 8;
							if(stade[k] < 101) pas_fini = 1; else repart->status_demande = 0;
							if(BancEnTest) { if(stade[k] < 101) *BancPrgs = stade[k]; }
							else strcpy(msg+(k*REPART_INFOFPGA_DIM),RepartInfoFpgaBB);
							InstrumRefreshVar(niveau[k]);
							break;
						}
					}
					gettimeofday(&LectDateRun,0);
				} while(LectDateRun.tv_sec < limite);
			} else if(repart->famille == FAMILLE_IPE) {
				IpeTrame trame; hexa lngr; int limite;
				gettimeofday(&LectDateRun,0); limite = LectDateRun.tv_sec + (timesleep * nbsleeps / 1000000);
				trame = (IpeTrame)repart->fifo32;
				lngr = sizeof(TypeSocket);
				do {
					nb = RepartIpLitTrame(repart,trame,repart->dim.totale);
					if(nb == -1) { if(errno != EAGAIN) {
						if(xmit) { printf("%s/ ! Erreur de transmission %d pour %s: %s [%d]\n",DateHeure(),errno,repart->nom,strerror(errno),xmit); xmit--; }
						else break;
					}; SambaMicrosec(timesleep); }
					else if(nb > 0) {
						if(BigEndianOrder) ByteSwappeIntArray((hexa *)trame,nb / sizeof(Ulong));
						if(IPE_TRAME_NUMERO(trame) == IPE_TRAME_STS) {
							vue[k] = 1;
							if(BigEndianOrder) ByteSwappeIntArray((hexa *)trame+1,(nb - 1) / sizeof(Ulong));
							// IpeTraduitProgBB(trame);
							IpeRecopieStatus(trame,repart,0); // si reponse en erreur, pourrait remettre PixBus a 0, ce qui fait planter ipe4read ensuite...
							printf("%s/ . Etat %s: %04X (%s)\n",DateHeure(),repart->nom,(hexa)trame->p.sts_crate.micro_bbv2,RepartInfoFpgaBB);
							stade[k] = trame->p.sts_crate.micro_bbv2 >> 8;
							if(stade[k] < 101) pas_fini = 1; else repart->status_demande = 0;
							if(BancEnTest) { if(stade[k] < 101) *BancPrgs = stade[k]; }
							else strcpy(msg+(k*REPART_INFOFPGA_DIM),RepartInfoFpgaBB);
							InstrumRefreshVar(niveau[k]);
							break;
						}
					}
					gettimeofday(&LectDateRun,0);
				} while(LectDateRun.tv_sec < limite);
			}
			if(nb == -1) { printf("%s/ ! Erreur de transmission %d pour %s: %s\n",DateHeure(),errno,repart->nom,strerror(errno)); tilt = 1; }
			k++;
		};
		if(!BancEnTest) PanelRefreshVars(p);
		OpiumUserAction();
		if(BancEnTest && BancUrgence) RepartFpgaInter = 1;
		if(RepartFpgaInter) break;
	} while(pas_fini);
	k = 0;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].charge_fpga) {
		if(!vue[k]) { printf("%s/ ! Pas d'information d'etat pour %s\n",DateHeure(),Repart[rep].nom); tilt = 1; }
		k++;
	}
	for(rep=0; rep<RepartNb; rep++) Repart[rep].status_demande = 0;
	if(!BancEnTest) { OpiumClear(planche); BoardTrash(planche); BoardDelete(planche); }

	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].charge_fpga && (Repart[rep].famille == FAMILLE_IPE)) {
		TypeRepartiteur *repart; int flt;
		repart = &(Repart[rep]);
		RepartIpTransmission(repart,REPART_ENVOI_STATUS,0,1);
		IpeEcritRessourceBB(repart,-1,0xFF,ss_adrs_alim,_bloque_commandes);
		printf("%s/   . Ecrit: %s / alim = bloque\n",DateHeure(),RepartiteurValeurEnvoyee);
		for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].sel && (repart == Numeriseur[bb].repart)) Numeriseur[bb].verrouille = 1;
		printf("%s/ + %s:\n",DateHeure(),repart->nom);
		for(flt = 0; flt<repart->r.ipe.fltnb; flt++) if(repart->r.ipe.flt[flt].fibres_fermees != 0x3F) {
			repart->r.ipe.flt[flt].fibres_fermees = 0x3F;
			IpeEcritFlt(repart,flt,IPE_FLT_ACCES_BB,repart->r.ipe.flt[flt].fibres_fermees,1);
		}
	}

hev:
	if(stade) free(stade); if(vue) free(vue); if(msg) free(msg); if(niveau) free(niveau);
	sprintf(explics,"Chargement des FPGA %s",bof? "abandonne": (tilt? "en erreur": (RepartFpgaInter? "interrompu": "termine")));
	printf("%s/ %s\n",DateHeure(),explics);
//	if(tilt) OpiumError(explics);

	return(!bof && !tilt && !RepartFpgaInter);
}
/* ========================================================================== */
char RepartAjusteHorloge(float *frequence, float *relance) /* kHz, sec */ {
	int rep,modl; shex diviseur0,d0,d1,lissage; float horloge,echantillonnage;
	int voies_par_mot;
	char clck,ok;

	d0 = d1 = 0; clck = 0; ok = 1;
	echantillonnage = *frequence;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].candidat) {
		modl = Repart[rep].type;
		if(Repart[rep].famille == FAMILLE_CLUZEL) {
			d0 = ModeleRep[modl].diviseur0;
			d1 = ModeleRep[modl].diviseur1;
		} else {
			if(Repart[rep].famille == FAMILLE_OPERA) {
				if(Repart[rep].r.opera.clck) continue;
				OperaDimensions(Repart[rep].r.opera.mode,&d1,&voies_par_mot,0);
			} else if(Repart[rep].famille == FAMILLE_CALI) {
				if(Repart[rep].r.cali.clck) continue;
				d1 = 1;
			} else if(Repart[rep].famille == FAMILLE_IPE) d1 = 60;
			else continue;
			diviseur0 = (short)((ModeleRep[modl].horloge * 1000.0 / (*frequence * (float)d1)) + 0.5);
			if(!d0) switch(Repart[rep].famille) {
			  case FAMILLE_CALI:
				lissage = CaliLissage(diviseur0,Repart[rep].revision);
			  	if(Repart[rep].revision == 1) d0 = (diviseur0 / lissage) * lissage;
				else d0 = lissage;
				break;
			  default: d0 = diviseur0; break;
			} // tant pis si incompatibilite.. else if(d0 != diviseur0) ok = 0;
		}
		horloge = ModeleRep[modl].horloge; clck = 1;
		break; // tant pis si incompatibilite..
	}
	if(!clck) horloge = HorlogeSysteme; if(!d0) d0 = Diviseur0; if(!d1) d1 = Diviseur1;
	if(d0 && d1) {
		echantillonnage = (horloge * 1000.0) / (float)(d0 * d1);
		// printf("(%s) Horloge %g MHz, d0=%d, d1=%d soit echantillonnage: %g kHz\n",__func__,horloge,d0,d1,echantillonnage);
		*frequence = echantillonnage;
	} else echantillonnage = *frequence;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].candidat && (Repart[rep].interf == INTERF_IP)) {
		float duree_rep,duree_compteur; /* seconde */
		if(Repart[rep].famille == FAMILLE_OPERA) {
			duree_compteur = (float)(Repart[rep].s.ip.inc_stamp) / (echantillonnage * 1000.0);
			Repart[rep].s.ip.relance = 245;
		} else if(Repart[rep].famille == FAMILLE_CALI) {
			float duree_trame,duree_D3;
			duree_D3 = 1.0; /* seconde */
			duree_trame = (float)(Repart[rep].s.ip.duree_trame) / (echantillonnage * 1000.0);
			/* en fait, stampD3 == 1000000 ou repart->s.ip.duree_trame, donc compteurs ci-dessous independants de l'echantillonage
			Repart[rep].s.ip.tramesD3 = (int)((duree_D3 / (echantillonnage * 1000.0)) / duree_trame);
			Repart[rep].s.ip.dim_D3 = Repart[rep].s.ip.tramesD3 * Repart[rep].s.ip.duree_trame * Repart[rep].nbdonnees;
			Repart[rep].s.ip.taille_derniere = Repart[rep].s.ip.stampD3 - (Repart[rep].s.ip.duree_trame * Repart[rep].s.ip.tramesD3); */
			duree_compteur = duree_trame;
			Repart[rep].s.ip.relance = (int)(((*relance + SettingsDelaiIncertitude) / duree_compteur) + 0.9);
		} else if(Repart[rep].famille == FAMILLE_IPE) {
			duree_compteur = (float)(Repart[rep].s.ip.inc_stamp) / (echantillonnage * 1000.0);
			Repart[rep].s.ip.relance = 245;
        } else duree_compteur = 0;
		if(Repart[rep].s.ip.relance > Repart[rep].p.ip.max_udp) Repart[rep].s.ip.relance = Repart[rep].p.ip.max_udp;
		/* on relance au moins <SettingsDelaiIncertitude> (5 par defaut) secondes avant la fin de la transmission */
		duree_rep = ((float)Repart[rep].s.ip.relance * duree_compteur) - SettingsDelaiIncertitude;
		if(duree_rep < *relance) *relance = duree_rep;
	}
	return(ok);
}
/* ========================================================================== */
void RepartCompteCalagesD3() {
	int rep;
	char ip_inclus;

	RepartD3Attendues = 0;
	if(LectModeStatus) return;
	ip_inclus = 0;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif && (Repart[rep].famille != FAMILLE_CALI) && !Repart[rep].D3trouvee) {
		RepartD3Attendues++;
		if(Repart[rep].interf == INTERF_IP) ip_inclus++;
	}
	if((RepartD3Attendues <= 1) && (LectSynchroType != LECT_SYNC_D3) && (ip_inclus < 2)) RepartD3Attendues = 0;
}
/* ========================================================================== */
int RepartiteurRafraichi() {
	TypeRepartiteur *repart; TypeNumeriseur *numeriseur;
	int i,k,l,vl,vt,nb,max,prem,actives,vpn; hexa zoom; char fin_flt,trait_a_droite;
	TypeTamponDonnees *tampon; TypeDonnee *adrs; Graph g; char texte[80];
	int c; OpiumColorState s; /* suivi des couleurs   */
	int x,y,h,v;

	repart = RepartConnecte;

	repart->status_demande = Acquis[AcquisLocale].etat.active;
	if(iRepartRafraichi && OpiumDisplayed(iRepartRafraichi->cdr)) InstrumRefreshVar(iRepartRafraichi);

	if(RepartDonneesAffiche) /* le panel des valeurs instantannees */ {
		// for(l=0; l<repart->nbentrees; l++) printf("| %s.%d: %d voie%s\n",repart->nom,l+1,Accord1s(repart->entree[l].voies));
		vpn = 0; actives = 0;
		for(l=0; l<repart->nbentrees; l++) if((repart->famille != FAMILLE_IPE) || repart->r.ipe.flt[IPE_FLT_NUM(l)].active) {
			if(repart->entree[l].voies > vpn) vpn = repart->entree[l].voies;
			actives++;
		}
		nb = vpn * actives;
		if(actives > 1) nb += actives;
		// printf("| Panel donnees: %d x %d + %d = %d item%s\n",vpn,actives,(actives > 1)?actives:0,Accord1s(nb));
		if(pRepartDonnees && (PanelItemMax(pRepartDonnees) != nb)) {
			if(OpiumDisplayed(pRepartDonnees->cdr)) OpiumClear(pRepartDonnees->cdr);
			PanelDelete(pRepartDonnees);
			pRepartDonnees = 0;
			RepartDonneesConstruit = 0;
		}
		if(pRepartDonnees && RepartDonneesConstruit) {
			if(OpiumDisplayed(pRepartDonnees->cdr)) PanelRefreshVars(pRepartDonnees);
			else { OpiumDisplay(pRepartDonnees->cdr); OpiumUserAction(); }
		} else if(repart->status_relu) {
			if(!pRepartDonnees) pRepartDonnees = PanelCreate(nb);
			PanelErase(pRepartDonnees);
			PanelColumns(pRepartDonnees,actives);
			PanelMode(pRepartDonnees,PNL_DIRECT|PNL_RDONLY);
			vt = 0;
			for(l=0; l<repart->nbentrees; l++) if((repart->famille != FAMILLE_IPE) || repart->r.ipe.flt[IPE_FLT_NUM(l)].active) {
				if(actives > 1) {
					if((numeriseur = repart->entree[l].adrs)) PanelRemark(pRepartDonnees,numeriseur->nom);
					else {
						sprintf(&(RepartNumerNom[l][0]),"Numer#%d",l+1);
						PanelRemark(pRepartDonnees,&(RepartNumerNom[l][0]));
					}
				}
				fin_flt = (repart->famille == FAMILLE_IPE) && !((l + 1) % IPE_FLT_SIZE);
				trait_a_droite = fin_flt && ((l + 1) < repart->nbentrees);
				for(vl=0; vl<vpn; vl++) {
					if(vl < repart->entree[l].voies) {
						if(!ModeleRep[repart->type].regroupe_numer) vt = (vl * repart->nbentrees) + l;
						k = PanelSHex(pRepartDonnees,l?0:&(RepartDonneeNom[vl][0]),&(RepartConnexionEnCours[vt].valeur)); vt++;
					} else k = PanelText(pRepartDonnees,l?0:&(RepartDonneeNom[vl][0]),0,5);
					if(trait_a_droite) PanelItemBarreDroite(pRepartDonnees,k,1);
				}
				if(fin_flt) vt = (((vt - 1) / IPE_BOURRE_QUANTUM) + 1) * IPE_BOURRE_QUANTUM;
			}

			PanelTitle(pRepartDonnees,repart->nom);
			if(OpiumDisplayed(cRepartAcq)) {
				OpiumGetPosition(cRepartAcq,&x,&y); OpiumGetSize(cRepartAcq,&h,&v);
				OpiumPosition(pRepartDonnees->cdr,x+(h/2),y+v+50);
			} else OpiumPosition(pRepartDonnees->cdr,10,500);
			OpiumDisplay(pRepartDonnees->cdr);
			OpiumUserAction(); /* sinon pas displayed a la prochaine, et tout plante */
			RepartDonneesConstruit = 1;
		}
	} else if(pRepartDonnees && OpiumDisplayed(pRepartDonnees->cdr)) {
		OpiumClear(pRepartDonnees->cdr);
		RepartDonneesConstruit = 0;
	}

	if((g = RepartGraphEnCours)) /* le trace des valeurs lues */ {
		if(RepartCourbesAffiche) {
			if(!OpiumDisplayed(g->cdr)) {
				if(repart->status_relu) {
					Panel p; char a_tracer[REPART_VOIES_MAX]; char nom[REPART_VOIES_MAX][16]; int rc;
					p = PanelCreate(repart->nbdonnees);
					for(vt=0; vt<repart->nbdonnees; vt++) {
						sprintf(&(nom[vt][0]),"Donnee %d",vt+1);
						a_tracer[vt] = 1;
						/* horrible bretelle speciale BB2 */
						if(repart->nbdonnees == 6) a_tracer[2] = a_tracer[3] = 0;
						PanelBool(p,&(nom[vt][0]),&(a_tracer[vt]));
					}
					rc = OpiumExec(p->cdr);
					PanelDelete(p);
					if(rc == PNL_CANCEL) {
						RepartCourbesAffiche = 0;
						goto rafraichi_etat;
					}
					GraphErase(g);
					x = GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,RepartStatusHorloge,RepartTraceLarg);
					GraphDataName(g,x,"t(s)");
					OpiumColorInit(&s);
					RepartGraphVoieNb = 0;
					for(vt=0; vt<repart->nbdonnees; vt++) if(a_tracer[vt]) {
						tampon = &(RepartConnexionEnCours[vt].tampon);
						tampon->dim = RepartTraceLarg;
						if(tampon->t.sval && (tampon->dim != tampon->max)) {
							adrs = tampon->t.sval; tampon->t.sval = 0; tampon->max = 0;
							free(adrs);
						}
						if(!tampon->t.sval) {
							tampon->t.sval = (TypeDonnee *)malloc(tampon->dim * sizeof(TypeDonnee));
							if(tampon->t.sval) tampon->max = tampon->dim; else tampon->max = 0;
							tampon->type = TAMPON_INT16;
						}
						tampon->nb = 0; tampon->prem = 0;
						if(tampon->t.sval) {
							y = GraphAdd(g,GRF_SHORT|GRF_YAXIS,tampon->t.sval,tampon->max);
							sprintf(texte,"voie %d",vt+1);
							GraphDataName(g,y,texte);
							c = OpiumColorNext(&s);
							GraphDataRGB(g,y,OpiumColorRGB(c));
							RepartGraphVoie[RepartGraphVoieNb++] = vt;
						}
					}
					GraphAxisAutoRange(g,GRF_YAXIS);
					GraphMode(g,GRF_2AXES | GRF_LEGEND);
					if(OpiumDisplayed(cRepartAcq)) {
						OpiumGetPosition(cRepartAcq,&x,&y); OpiumGetSize(cRepartAcq,&h,&v);
						OpiumPosition(g->cdr,x+h+15,y);
					}
					GraphParmsSave(g);
					GraphUse(g,0);
					OpiumDisplay(g->cdr);
				}
			} else {
				max = 0; prem = 0;
				for(i=0; i<RepartGraphVoieNb; i++) {
					vt = RepartGraphVoie[i];
					GraphDataUse(g,i+1,RepartConnexionEnCours[vt].tampon.nb);
					GraphDataRescroll(g,i+1,RepartConnexionEnCours[vt].tampon.prem);
					if(max < RepartConnexionEnCours[vt].tampon.nb) max = RepartConnexionEnCours[vt].tampon.nb;
					if(prem < RepartConnexionEnCours[vt].tampon.prem) prem = RepartConnexionEnCours[vt].tampon.prem;
				}
				GraphDataUse(g,0,max); GraphDataRescroll(g,0,prem);
				if(max > RepartTraceVisu) { zoom = ((max - 1) / RepartTraceVisu) + 1; GraphZoom(g,zoom,1); }
				OpiumScrollXmax(g->cdr);
				OpiumRefresh(g->cdr);
			}
		} else if(OpiumDisplayed(g->cdr)) OpiumClear(g->cdr);
	}

rafraichi_etat:
	if(repart->status_relu && !RepartiteurParmModifie && pRepartStatus && OpiumDisplayed(pRepartStatus->cdr)) {
		PanelRefreshVars(pRepartStatus);
		repart->status_relu = 0;
	}

	return(1);
}
#pragma mark ----------- Cluzel -----------
/* ========================================================================== */
char CluzelNouvelle(TypeRepartiteur *repart, int num, char categ) {
	//- char message[256];

	if(!repart) { OpiumFail("! Repartiteur indefini"); return(0); }
	else if(!CluzelModele) { OpiumFail("! Type de repartiteur indefini: '%s'",RepartFamilleListe[FAMILLE_CLUZEL]); return(0); }
	repart->type = CluzelModele - 1;
	repart->adrs.val = num;
	sprintf(repart->parm,"%d",num);
	repart->valide = 1;
	strcpy(repart->maintenance,".");
	repart->categ = categ;
	repart->nbident = 0;
	repart->nbentrees = 0; repart->entree = 0;
	repart->nbdonnees = 0;
	repart->famille = ModeleRep[repart->type].famille;
	repart->interf = ModeleRep[repart->type].interface;
	repart->r.cluzel.d2 = 100; repart->r.cluzel.d3 = 1000;
	//- RepartiteurVerifieParmsEtFIFO(repart,message);
	return(1);
}
/* ========================================================================== */
void CluzelIcaFlashWrite(PciBoard board, int adrs, int val) {
	if(!board) return;
#ifdef AVEC_PCI
	IcaRegWrite(board,ICA_REG_FPROM_ADRS,adrs);
	IcaRegWrite(board,ICA_REG_FPROM_DATA,val);
	IcaRegWrite(board,ICA_REG_FPROM_CNTL,0x02);  /* WE + CS */
	IcaRegWrite(board,ICA_REG_FPROM_CNTL,0x06);  /* remonte WE */
	IcaRegWrite(board,ICA_REG_FPROM_CNTL,0x07);  /* remonte enfin CS */
#endif
}
/* ========================================================================== */
static INLINE char CluzelRAZFifo(PciBoard pci, int fifo_dim, char secu, char log) {
	int nbval,nb_non_nul,max_fois;

	if(!pci) return(0);
	if(secu) {
		//inutile depuis le debut: CluzelEnvoieCommande(pci,CLUZEL_ADRS,CLUZEL_SSAD_IDENT,CLUZEL_STOP,CLUZEL_IMMEDIAT);
		CluzelEnvoieCommande(pci,CLUZEL_ADRS,CLUZEL_SSAD_ACQUIS,CLUZEL_STOP,CLUZEL_IMMEDIAT);
		if(log) printf("%s/   . Arret repartiteur prealable\n",DateHeure());
		SambaAttends(100);
		nbval = fifo_dim / sizeof(Ulong);

#ifdef AVEC_PCI
	#define VIDAGE_FIFO
#endif

	#ifdef VIDAGE_FIFO
		nb_non_nul = IcaFifoEmpty(pci,2 * nbval);
		if(nb_non_nul < 0) printf("%s/   . Vidage FIFO: le driver ne repond pas\n",DateHeure());
		else if(log) printf("%s/   . FIFO videe (%d/%d valeurs non-nulles dans la FIFO)\n",
					   DateHeure(),nb_non_nul,nbval);
	#else
        nb_non_nul = 0;
	#endif
		max_fois = 2;
		do {
		#ifdef AVEC_PCI
			IcaFifoReset(pci);
			if(log) printf("%s/   . FIFO resettee\n",DateHeure());
		#ifdef VIDAGE_FIFO
			nb_non_nul = IcaFifoEmpty(pci,nbval);
			if(nb_non_nul < 0) printf("%s/   . Vidage FIFO: le driver ne repond pas\n",DateHeure());
			else if(log) printf("%s/   . FIFO videe (%s: %d/%d valeurs non-nulles dans la FIFO)\n",
						   DateHeure(),nb_non_nul? "probleme": "OK",nb_non_nul,nbval);
		#endif
		#endif
		} while(nb_non_nul && (--max_fois > 0));
		/*		return(nb_non_nul == 0); */
		return((nb_non_nul >= 0)  && (nb_non_nul < nbval));

		/* Mode cavalier, ou l'on se contente de reseter la FIFO en peine activite */
	} else {
	#ifdef AVEC_PCI
		IcaFifoReset(pci);
	#endif
		if(log) printf("%s/   . FIFO resettee\n",DateHeure());
		return(1);
	}
}
/* ========================================================================== */
static int CluzelReset(TypeRepartiteur *repart, char avec_opium, char avec_reboot) {
	int modl; float version;
	PciBoard pci; //- TypePciReg *reg;
#ifdef AVEC_PCI
	int mot,num,nb; hexa val,relu;
#endif

	if(!repart) { printf("! Repartiteur indefini\n"); return(0); }
	pci = repart->p.pci.port;
	if(!pci) return(0);
	modl = repart->type; version = ModeleRep[modl].version;
#ifdef AVEC_PCI
	if(ModeleRep[modl].revision < 0) /* Carte NOSTOS */ {
		Ulong val;
		// if(Echantillonnage == 2500.0) val = 0x80; else val = 0x00;
		if(Echantillonnage == 2500.0) val = 0x00; else val = 0x80;
		IcaUtilWrite(pci,val);
		printf("            . ecrit %02X sur le bus UTIL\n",val);
	} else if(version < 1.1) /* vraie Cluzel */ {
	#define RAZ_EN_IDENT
	#ifdef RAZ_EN_IDENT
		if(repart->bb) {
			printf("            . mise generale en identification\n");
			CluzelEnvoieCommande(pci,NUMER_DIFFUSION,CLUZEL_SSAD_ACQUIS,CLUZEL_STOP,CLUZEL_MODULEE);
			CluzelEnvoieCommande(pci,NUMER_DIFFUSION,CLUZEL_SSAD_IDENT,CLUZEL_START,CLUZEL_MODULEE);
		}
	#else
		if(repart->bb) {
			printf("            . mise generale en acquisition\n");
			CluzelEnvoieCommande(pci,NUMER_DIFFUSION,CLUZEL_SSAD_IDENT,CLUZEL_STOP,CLUZEL_MODULEE);
			CluzelEnvoieCommande(pci,NUMER_DIFFUSION,CLUZEL_SSAD_ACQUIS,CLUZEL_START,CLUZEL_MODULEE);
		}
	#endif
		// printf("  . Demarrage du repartiteur\n");
		// CluzelEnvoieCommande(pci,CLUZEL_ADRS,CLUZEL_SSAD_ACQUIS,CLUZEL_START,CLUZEL_MODULEE);
		printf("            . arret du repartiteur\n");
		CluzelEnvoieCommande(pci,CLUZEL_ADRS,CLUZEL_SSAD_ACQUIS,CLUZEL_STOP,CLUZEL_MODULEE); /* des fois qu'il ne soit pas concerne par les diffusions */
		//inutile depuis le debut: CluzelEnvoieCommande(pci,CLUZEL_ADRS,CLUZEL_SSAD_IDENT,CLUZEL_STOP,CLUZEL_MODULEE);
	} else if(version < 2.0) /* super Cluzel avec ou sans status */ {
		if(repart->bb) {
			printf("            . mise generale en identification\n");
			CluzelEnvoieCommande(pci,NUMER_DIFFUSION,CLUZEL_SSAD_ACQUIS,CLUZEL_STOP,CLUZEL_MODULEE);
			CluzelEnvoieCommande(pci,NUMER_DIFFUSION,CLUZEL_SSAD_IDENT,CLUZEL_START,CLUZEL_MODULEE);
		}
		printf("            . arret du repartiteur\n");
		CluzelEnvoieCommande(pci,CLUZEL_ADRS,CLUZEL_SSAD_ACQUIS,CLUZEL_STOP,CLUZEL_MODULEE); /* des fois qu'il ne soit pas concerne par les diffusions */
		//inutile depuis le debut: CluzelEnvoieCommande(pci,CLUZEL_ADRS,CLUZEL_SSAD_IDENT,CLUZEL_STOP,CLUZEL_MODULEE);
		if(avec_reboot) {
			nb = 4 * (int)((float)(repart->r.cluzel.d2 * repart->r.cluzel.d3) / Echantillonnage);
			printf("            . redemarrage du repartiteur et synchronisation des numeriseurs (%g secondes)\n",(float)nb / 1000.0);
			printf("            . *SI* CETTE SUPERCLUZEL EST MAITRE, ELLE A REDEMARRE TOUTES LES AUTRES\n");
			if(avec_opium) OpiumError("*SI* CETTE SUPERCLUZEL EST MAITRE, ELLE A REDEMARRE TOUTES LES AUTRES");
			CluzelEnvoieBasique(pci,0,0xCD,0x00,0x0001);  /* reset de TOUTES les SC si ecrit dans une SC maitre */
			SambaAttends(nb);                             /* attend la fin du reset des numeriseurs             */
		} else printf("            . PAS de redemarrage du repartiteur\n");
		nb = repart->nbentrees; mot = 0;
		while(nb) {
			if(nb >= 16) {
				val = 0xFFFF;
				CluzelEnvoieBasique(pci,0,0xBB,mot,val);
				printf("            . selecteur %d=%04X (ecrit: %02X%02X%04X)\n",mot,val,0xBB,mot,val);
				mot++; nb -= 16;
			} else {
				val = (1 << nb) - 1;
				CluzelEnvoieBasique(pci,0,0xBB,mot,val);
				printf("            . selecteur %d=%04X (ecrit: %02X%02X%04X)\n",mot,val,0xBB,mot,val);
				mot++; nb = 0;
			}
		}
		if(ModeleRep[modl].status) {
			printf("            . activation de la production du status et configuration du compteur veto\n");
			CluzelEnvoieBasique(pci,0,0xCE,0x00,0x0003); /* bit 0=1 si reset timestamp sur start acquis */
		} else printf("            . production du status non activee et compteur veto indefini\n");
	} else if(version < 3.0) /* chassis HD */ {
		num = repart->adrs.val - 1;
		if(PCIedw[num].type == CARTE_ICA) {
			/* 1. reset de la carte ICA */
			IcaRegWrite(pci,ICA_REG_RESET,0x02); /* reset carte ICA */
			IcaRegWrite(pci,ICA_REG_RESET,0x00); /* il FAUT faire retomber le reset a la main */
			IcaRegWrite(pci,ICA_REG_FIFO_CNTL,0x00);
			/* 2. chargement des registres d'icelle */
			nb = repart->nbentrees; mot = 0;
			while(nb) {
				if(nb >= 32) {
					IcaRegWrite(pci,ICA_REG_SELECT0+mot,0xFFFFFFFF);
					printf("            . selecteur %d=0xFFFFFFFF\n",mot);
					mot++; nb -= 32;
				} else {
					val = (1 << nb) - 1;
					IcaRegWrite(pci,ICA_REG_SELECT0+mot,val);
					printf("            . selecteur %d=0x%08X\n",mot,val);
					mot++; nb = 0;
				}
			}
			for( ; mot<4; mot++) {
				IcaRegWrite(pci,ICA_REG_SELECT0+mot,0);
				printf("            . selecteur %d=0x00000000\n",mot);
			}
			/* 3. reset de la carte CC par arret de la transmission optique */
			CluzelIcaFlashWrite(pci,0x001FFFFF,0x01); SambaAttends(10);
			CluzelIcaFlashWrite(pci,0x001FFFFF,0x00); SambaAttends(10);
			CluzelEnvoieBasique(pci,0,0xA6,0x00,0x0000);  /* so-called "Reset Backplane" (efface le bit 11) */
			printf("            . fin du reset CC (2 secondes) a %s\n",DateHeure());
			SambaAttends(2000);             /* attend la fin du reset des numeriseurs         */
			/* 4. mise en condition de l'ICA */
			IcaRegWrite(pci,ICA_REG_FIFO_CNTL,0x00);
			IcaFifoReset(pci);
			/* 5. enumeration */
			printf("            . reset IBB a %s\n",DateHeure());
			CluzelEnvoieBasique(pci,0,0xA8,0x00,0x0001);  /* debut reset IBB */
			CluzelEnvoieBasique(pci,0,0xA8,0x00,0x0000);  /* fin de reset IBB */
			CluzelEnvoieBasique(pci,0,0xFF,0x88,0x0001);  /* equivalent a CluzelEnvoieCommande(pci,NUMER_DIFFUSION,CLUZEL_SSAD_ACQUIS,CLUZEL_STOP,CLUZEL_MODULEE) */
			CluzelEnvoieBasique(pci,0,0xFF,0x89,0x0000);  /* equivalent a CluzelEnvoieCommande(pci,NUMER_DIFFUSION,CLUZEL_SSAD_IDENT,CLUZEL_START,CLUZEL_MODULEE) */
			CluzelEnvoieBasique(pci,0,0xE0,0x00,0x0000);  /* c'est parti! */
			SambaAttends(50);
			/* 6. attente de la fin de la mise en route */
			CluzelEnvoieBasique(pci,0,0xAA,0x00,0x0001);  /* ce sont les donnees qui vont vers la sortie de CC */
			printf("            . envoi vers FIFO a %s\n",DateHeure());
			CluzelEnvoieBasique(pci,0,0x7F,0x48,0x0000);  /* equivalent a CluzelEnvoieCommande(pci,CLUZEL_ADRS,CLUZEL_SSAD_ACQUIS,CLUZEL_START,CLUZEL_MODULEE) */
			relu = IcaRegRead(pci,ICA_REG_STATUS);
			if(!((relu & 0x3FFF) & 0x0200)) printf("            . en erreur (status CC=%08X)\n",relu);
			else printf("            . terminee\n");
		} else { /* (PCIedw[num].type == CARTE_MIG ou AB) */
			/* 1. chargement des registres d'icelle */
			nb = repart->nbentrees; mot = 0;
			while(nb) {
				if(nb >= 16) {
					val = 0xFFFF;
					CluzelEnvoieBasique(pci,0,0xBB,mot,val);
					printf("            . Selecteur %d=%04X (ecrit: %02X%02X%04X)\n",mot,val,0xBB,mot,val);
					mot++; nb -= 16;
				} else {
					val = (1 << nb) - 1;
					CluzelEnvoieBasique(pci,0,0xBB,mot,val);
					printf("            . Selecteur %d=%04X (ecrit: %02X%02X%04X)\n",mot,val,0xBB,mot,val);
					mot++; nb = 0;
				}
			}
			//				printf("            . Mise generale en identification\n");
			//				CluzelEnvoieCommande(pci,NUMER_DIFFUSION,CLUZEL_SSAD_ACQUIS,CLUZEL_STOP,CLUZEL_MODULEE);
			//				CluzelEnvoieCommande(pci,NUMER_DIFFUSION,CLUZEL_SSAD_IDENT,CLUZEL_START,CLUZEL_MODULEE);
			printf("            . Arret du repartiteur\n");
			CluzelEnvoieCommande(pci,CLUZEL_ADRS,CLUZEL_SSAD_ACQUIS,CLUZEL_STOP,CLUZEL_MODULEE); /* des fois qu'il ne soit pas concerne par les diffusions */
			//inutile depuis le debut: CluzelEnvoieCommande(pci,CLUZEL_ADRS,CLUZEL_SSAD_IDENT,CLUZEL_STOP,CLUZEL_MODULEE);
			/* 2. reset backplane */
			CluzelEnvoieBasique(pci,0,0xA6,0x00,0x0000);  /* so-called "Reset Backplane" (efface le bit 11) */
			printf("            . fin du reset CC a %s\n",DateHeure());
			/* 3. programmation des IBB */
			printf("            . programmation IBB et reset BB (9 secondes) a %s\n",DateHeure());
			CluzelEnvoieBasique(pci,0,0xA6,0x00,0x0001); SambaAttends(1000);
			CluzelEnvoieBasique(pci,0,0xA8,0x00,0x0001); SambaAttends(1000);
			CluzelEnvoieBasique(pci,0,0xA8,0x00,0x0000); SambaAttends(1000);
			CluzelEnvoieBasique(pci,0,0xA2,repart->p.pci.page_prgm,0x0000);  /* positionnement pointeur, ex-PrgmRepartiteur */
			CluzelEnvoieBasique(pci,0,0xA7,0x00,0x0001);  /* lancement programmation IBB             */
			SambaAttends(3000);                           /* attend la fin du reset des numeriseurs? */
			CluzelEnvoieBasique(pci,0,0xA6,0x00,0x0000);
			SambaAttends(3000);                           /* synchronisation des communications      */
			/* 4. enumeration */
			printf("            . identification BB (2 secondes) a %s\n",DateHeure());
			CluzelEnvoieBasique(pci,0,0xFF,0x88,0x0001);  /* equivalent a CluzelEnvoieCommande(pci,NUMER_DIFFUSION,CLUZEL_SSAD_ACQUIS,CLUZEL_STOP,CLUZEL_MODULEE) */
			CluzelEnvoieBasique(pci,0,0xFF,0x89,0x0000);  /* equivalent a CluzelEnvoieCommande(pci,NUMER_DIFFUSION,CLUZEL_SSAD_IDENT,CLUZEL_START,CLUZEL_MODULEE) */
			SambaAttends(2000);          /* attend la fin du reset des numeriseurs  */
			printf("            . reset et enumeration IBB (2 secondes) a %s\n",DateHeure());
			CluzelEnvoieBasique(pci,0,0xA8,0x00,0x0001);  /* debut reset IBB                         */
			CluzelEnvoieBasique(pci,0,0xA8,0x00,0x0000);  /* fin de reset IBB                        */
			CluzelEnvoieBasique(pci,0,0xE0,0x00,0x0000);  /* c'est parti!                            */
			SambaAttends(50);
			nb = 4 * (int)((float)(repart->r.cluzel.d2 * repart->r.cluzel.d3) / Echantillonnage);
			printf("            . resynchronisation des numeriseurs (%g secondes)\n",(float)nb / 1000.0);
			//					CluzelEnvoieBasique(pci,0,0xCD,0x00,0x0001);  /* reset de TOUTES les SC si ecrit dans une SC maitre */
			SambaAttends(nb);            /* attend la fin du reset des numeriseurs  */
			/* 5. activation de la production du status */
			printf("            . activation de la production du status\n");
			CluzelEnvoieBasique(pci,0,0xCE,0x00,0x0002); /* bit 0=1 si reset timestamp sur start acquis */
			/* 6. attente de la fin de la mise en route */
			printf("            . envoi vers FIFO a %s\n",DateHeure());
			CluzelEnvoieBasique(pci,0,0xAA,0x00,0x0001);  /* ce sont les donnees qui vont vers la sortie de CC */
			CluzelEnvoieBasique(pci,0,0x7F,0x48,0x0000);  /* equivalent a CluzelEnvoieCommande(pci,CLUZEL_ADRS,CLUZEL_SSAD_ACQUIS,CLUZEL_START,CLUZEL_MODULEE) */
		}
	}
#else
	printf("            !!! Acces PCI non compile dans cette version de SAMBA\n");
#endif /* AVEC_PCI */
	return(0);
}
/* pas de delai dans l'ICA? SambaMicrosec(CLUZEL_DELAI_MOT / 1000); */
/* ========================================================================== */
int CluzelEnvoieBasique(PciBoard board, byte action, byte adrs, byte cmde, TypeADU valeur) {
	if(!board) return(0);
#ifdef AVEC_PCI
#ifdef PCI_SEND_INUTILISE
	IcaUtilWrite(board,0x100 | adrs);          SambaMicrosec(1);
	IcaUtilWrite(board,cmde & 0xFF);           SambaMicrosec(1); // impose CLUZEL_IMMEDIAT car action == 0
	IcaUtilWrite(board,(valeur >> 8)  & 0xFF); SambaMicrosec(1);
	IcaUtilWrite(board,valeur & 0xFF);         SambaMicrosec(1);
	SambaAttends(10);
	return(1);
#else
	return((int)IcaCommandSend(board,adrs,action,cmde,valeur));
#endif
#else
	return(1);
#endif
}
/* ========================================================================== */
INLINE int CluzelEnvoieCommande(PciBoard board, byte adrs, byte ss_adrs, TypeADU valeur, byte action) {
	if(!board) return(0);
#ifdef AVEC_PCI
#ifdef PCI_SEND_INUTILISE
	IcaUtilWrite(board,0x100 | adrs);                     SambaMicrosec(CLUZEL_DELAI_MOT / 1000);
	IcaUtilWrite(board,((action << 6) & 0xC0) | ss_adrs); SambaMicrosec(CLUZEL_DELAI_MOT / 1000);
	IcaUtilWrite(board,(valeur >> 8) & 0xFF);             SambaMicrosec(CLUZEL_DELAI_MOT / 1000);
	IcaUtilWrite(board,valeur & 0xFF);                    SambaMicrosec(CLUZEL_DELAI_MSG * 1000);
	return(1);
#else  /* PCI_SEND_INUTILISE */
	return((int)IcaCommandSend(board,adrs,action,ss_adrs,valeur));
#endif /* PCI_SEND_INUTILISE */
#else  /* AVEC_PCI */
	return(1);
#endif /* AVEC_PCI */
}
/* ========================================================================== */
INLINE static void CluzelDemarreNumeriseurs(TypeRepartiteur *repart, NUMER_MODE mode, byte adrs, char *prefixe) {
	/* if(mode == NUMER_MODE_ACQUIS) {
	 IcaCommandSend(repart->p.pci.port,adrs,CLUZEL_MODULEE,CLUZEL_SSAD_IDENT,CLUZEL_STOP);
	 if(log) printf("            . Ecrit: 1%02X.%02X.%04X\n",adrs,((CLUZEL_MODULEE << 6) & 0xC0) | CLUZEL_SSAD_IDENT,CLUZEL_STOP);
	 IcaCommandSend(repart->p.pci.port,adrs,CLUZEL_MODULEE,CLUZEL_SSAD_ACQUIS,CLUZEL_START);
	 if(log) printf("            . Ecrit: 1%02X.%02X.%04X\n",adrs,((CLUZEL_MODULEE << 6) & 0xC0) | CLUZEL_SSAD_ACQUIS,CLUZEL_START);
	} else {
	 IcaCommandSend(repart->p.pci.port,adrs,CLUZEL_MODULEE,CLUZEL_SSAD_ACQUIS,CLUZEL_STOP);
	 if(log) printf("            . Ecrit: 1%02X.%02X.%04X\n",adrs,((CLUZEL_MODULEE << 6) & 0xC0) | CLUZEL_SSAD_ACQUIS,CLUZEL_STOP);
	 IcaCommandSend(repart->p.pci.port,adrs,CLUZEL_MODULEE,CLUZEL_SSAD_IDENT,CLUZEL_START);
	 if(log) printf("            . Ecrit: 1%02X.%02X.%04X\n",adrs,((CLUZEL_MODULEE << 6) & 0xC0) | CLUZEL_SSAD_IDENT,CLUZEL_START);
	} */
	if(!repart) { printf("! Repartiteur indefini\n"); return; }
	if(!repart->p.pci.port) { printf("! %s: pas d'acces PCI\n",repart->nom); return; }
	if(mode == NUMER_MODE_COMPTEUR) {
		if(prefixe) printf("%s! Mode '%s' pas implemente\n",prefixe,NumerModeTexte[mode]);
	} else if(mode == NUMER_MODE_ACQUIS) {
		CluzelEnvoieCommande(repart->p.pci.port,adrs,CLUZEL_SSAD_IDENT,CLUZEL_STOP,CLUZEL_MODULEE);
		if(prefixe) printf("%s. Ecrit: 1%02X.%02X.%04X\n",prefixe,adrs,((CLUZEL_MODULEE << 6) & 0xC0) | CLUZEL_SSAD_IDENT,CLUZEL_STOP);
		CluzelEnvoieCommande(repart->p.pci.port,adrs,CLUZEL_SSAD_ACQUIS,CLUZEL_START,CLUZEL_MODULEE);
		if(prefixe) printf("%s. Ecrit: 1%02X.%02X.%04X\n",prefixe,adrs,((CLUZEL_MODULEE << 6) & 0xC0) | CLUZEL_SSAD_ACQUIS,CLUZEL_START);
	} else {
		CluzelEnvoieCommande(repart->p.pci.port,adrs,CLUZEL_SSAD_ACQUIS,CLUZEL_STOP,CLUZEL_MODULEE);
		if(prefixe) printf("%s. Ecrit: 1%02X.%02X.%04X\n",prefixe,adrs,((CLUZEL_MODULEE << 6) & 0xC0) | CLUZEL_SSAD_ACQUIS,CLUZEL_STOP);
		CluzelEnvoieCommande(repart->p.pci.port,adrs,CLUZEL_SSAD_IDENT,CLUZEL_START,CLUZEL_MODULEE);
		if(prefixe) printf("%s. Ecrit: 1%02X.%02X.%04X\n",prefixe,adrs,((CLUZEL_MODULEE << 6) & 0xC0) | CLUZEL_SSAD_IDENT,CLUZEL_START);
	}
}
/* ========================================================================== */
INLINE static void CluzelEcritRessourceBB(TypeRepartiteur *repart, shex adrs, char ss_adrs, TypeADU valeur) {
	if(!repart) { printf("! Repartiteur indefini\n"); return; }
#ifdef AVEC_PCI
	if(repart->p.pci.port) {
		if(repart->adrs.val <= PCInb) IcaCommandSend(repart->p.pci.port,adrs,CLUZEL_IMMEDIAT,(int)ss_adrs,valeur);
		sprintf(RepartiteurValeurEnvoyee,"%02X.%02X.%04X",adrs,ss_adrs,valeur);
	} else
#endif
		sprintf(RepartiteurValeurEnvoyee,"(pas d'acces PCI)");
}
/* ========================================================================== */
static INLINE char CluzelVidePile(TypeRepartiteur *repart, char *pas_tous_secs, short *pleins, char log) {
#ifdef AVEC_PCI
	int num; int pci_vide;
#endif

	if(!repart) { printf("! Repartiteur indefini\n"); return(0); }
	if(!repart->p.pci.port) return(0);
#ifdef AVEC_PCI
#ifdef ACCES_PCI_DIRECT
	if(!LectEnCours.dispo) {
		LectEnCours.valeur = *(PCIadrs[Repart[rep].adrs.val-1]); // IcaFifoRead(repart->p.pci.port);
		repart->top++; LectEnCours.dispo = 1;
		*pas_tous_secs = 1;
	}
#else
	if(repart->s.pci.en_cours && (repart->top <= 0)) {
		int64 t1;
		t1 = (VerifTempsPasse? DateMicroSecs(): 0);
		if(RepartPci.copie) {
			if((t1 - RepartPci.copie) > RepartPci.delai) printf("%s/ %s: delai entre copies PCI = %lld us\n",DateHeure(),repart->nom,t1 - RepartPci.copie);
		}
		RepartPci.copie = t1;
		repart->top = IcaXferDone(repart->p.pci.port);
		num = repart->adrs.val - 1;
		if(log) printf("          %s: FIFO testee, %d donnee%s\n",repart->nom,Accord1s(repart->top));
		if(repart->top <= 0) {
			RepartPci.vide++;
			if(!PCIedw[num].dma || (repart->top < 0)) LectDeclencheErreur(_ORIGINE_,41,LECT_EMPTY);
			else {
				*pas_tous_secs = 1; /* on insiste lourdement, car comme c'est "en_cours", ca va bientot arriver.. */
				if(log) printf("          %s <%06lld>: pile PCI vide\n",repart->nom,t1); // plus facile a lire avec t1-t0
			}
			return(0);
		}
		repart->s.pci.en_cours = 0;
		repart->acces_remplis++;
		if(!PCIedw[num].dma) {
			IcaXferStatusGet(repart->p.pci.port,&pci_vide,&RepartPci.halffull,&RepartPci.fullfull);
			if(RepartPci.fullfull) *pleins += 1;
			if(!(RepartPci.vide = pci_vide)) *pas_tous_secs = 1;
		} // else *pas_tous_secs = 1; on relira pas, de toutes facons, tant que la pile SW n'a pas ete videe
	}
#endif
#endif
	return(1);
}
/* ========================================================================== */
char CluzelDepile1Donnee(TypeRepartiteur *repart, lhex *donnee, int *a_decompter, char log) {
	int premier,num,dep;
	char lue;

	if(!repart) { printf("! Repartiteur indefini\n"); return(0); }
	if(!repart->p.pci.port) return(0);

	num = repart->adrs.val - 1;
	lue = 0; *a_decompter = 0;

#ifdef ACCES_PCI_DIRECT
#ifdef CLUZEL_FIFO_INTERNE
	if((repart->bottom < repart->top) && (repart->bottom < (repart->octets / sizeof(Ulong)))) {
		repart->fifo32[repart->bottom++] = *donnee;
		if(repart->bottom >= repart->top) repart->top = repart->bottom = 0;
		lue = 1;
		repart->depile += 1;
	} else { repart->top = repart->bottom = 0; return(0); }
#endif
	if(CLUZEL_Empty(*donnee)) { RepartPci.vide++; return(0); }
	repart->donnees_recues++;
	if(CLUZEL_HalfFull(*donnee)) RepartPci.halffull++;
	if(CLUZEL_FullFull(*donnee)) RepartPci.fullfull++;
#else /* ACCES_PCI_DIRECT */

#ifdef RESERVOIRS
	premier = repart->depot_ancien;
	if(log > 1) printf("          %s: %d reservoir%s, plus ancien=%d\n",repart->nom,Accord1s(repart->depot_nb),premier);
	if(repart->depot_nb) until(lue) {
		dep = repart->depot_ancien;
		if(log > 1) printf("          . Depot #%d: [%d %d]\n",dep,repart->depot[dep].fond,repart->depot[dep].reserve);
		if(repart->depot[dep].fond < repart->depot[dep].reserve) {
			*donnee = repart->depot[dep].reservoir[(repart->depot[dep].fond)++];
			*a_decompter += 1;
			repart->depile += 1;
			if(PCIedw[num].dma && CLUZEL_Empty(*donnee)) {
				if(FifoPrecedente) {
					FifoTopPrec = repart->depot[dep].fond + 1;
					if((FifoTopPrec  * sizeof(Ulong)) > PCIedw[num].octets) FifoTopPrec = PCIedw[num].octets / sizeof(Ulong);
					memcpy(FifoPrecedente,repart->depot[dep].reservoir,FifoTopPrec * sizeof(Ulong));
				}
				RepartPci.vide++;
				if(log) printf("          %s: on a vide le reservoir #%d, il y avait %d donnee%s\n",repart->nom,dep,Accord1s(repart->depot[dep].fond));
				repart->depot[dep].reserve = repart->depot[dep].fond = 0;
				dep++; repart->depot_ancien = (dep < REPART_RESERVOIR_MAX)? dep: 0;
				if(repart->depot_ancien == premier) break;
			} else {
				lue = 1;
				if(repart->depot[dep].fond >= repart->depot[dep].reserve) {
					repart->depot[dep].fond = repart->depot[dep].reserve = 0;
					if(log) printf("          . On arrive au sommet du reservoir #%d\n",dep);
					dep++; repart->depot_ancien = (dep < REPART_RESERVOIR_MAX)? dep: 0;
				}
			}
		} else {
			dep++; repart->depot_ancien = (dep < REPART_RESERVOIR_MAX)? dep: 0;
			if(repart->depot_ancien == premier) break;
		}
	}
	if(!lue) {
		if(log > 1) printf("          . Pile reguliere: [%d %d]\n",repart->bottom,repart->top);
		if(repart->bottom < repart->top) {
			*donnee = repart->fifo32[repart->bottom++];
			*a_decompter += 1; repart->depile += 1;
			if(PCIedw[num].dma && CLUZEL_Empty(*donnee)) {
				if(FifoPrecedente) {
					FifoTopPrec = repart->bottom + 1;
					if((FifoTopPrec  * sizeof(Ulong)) > PCIedw[num].octets) FifoTopPrec = PCIedw[num].octets / sizeof(Ulong);
					memcpy(FifoPrecedente,repart->fifo32,FifoTopPrec * sizeof(Ulong));
				}
				if(log) printf("          On a vide la pile, il y avait %d donnee%s\n",Accord1s(repart->bottom));
				repart->top = repart->bottom = 0;
				RepartPci.vide++;
			} else {
				lue = 1;
				if(repart->bottom >= repart->top) {
					if(log) printf("          On arrive au sommet de la pile ([%d %d])\n",repart->nom,repart->bottom,repart->top);
					repart->top = repart->bottom = 0;
				}
			}
		} else {
			if(log) printf("          On etait au sommet de la pile ([%d %d])\n",repart->nom,repart->bottom,repart->top);
			repart->top = repart->bottom = 0;
		}
	}
	if(!lue) return(0);
	if(PCIedw[num].dma) {
		if(CLUZEL_HalfFull(*donnee)) RepartPci.halffull++;
		if(ModeleRep[repart->type].revision >= 0) { if(CLUZEL_FullFull(*donnee)) RepartPci.fullfull++; }
	}
#else /* RESERVOIRS */
	if((repart->bottom < repart->top) && (repart->bottom < (PCIedw[PCInum].octets / sizeof(Ulong)))) {
		*donnee = repart->fifo32[repart->bottom++]; lue = 1;
		*a_decompter += 1; repart->depile += 1;
		if(repart->bottom >= repart->top) {
			if(log) printf("          %s: on arrive au sommet de la pile ([%d %d])\n",repart->nom,repart->bottom,repart->top);
			if(log) printf("          . Valeur de type %X (%s), voie %d [bloc %lld], %s le status\n",
			 LectTypePrevu,verifie_type?"a verifier":"ignore",SambaEchantillon[LectEnCours.echantillon].voie,LectStampsLus+1,LectEnCours.is_status?"c'est":"ce n'est pas");
			repart->top = repart->bottom = 0;
		}
	} else {
		//	if(a_decompter != (a_depiler[LectEnCours.rep] - (repart->top - repart->bottom))) {
		//		LectErreur.code = LECT_INCOHER; LectErreur.num = 48; goto retour;
		//	}
		if(log) printf("          %s: on etait au sommet de la pile ([%d %d])\n",repart->nom,repart->bottom,repart->top);
		if(log) printf("          . Valeur de type %X (%s), voie %d [bloc %lld], %s le status\n",
					   LectTypePrevu,verifie_type?"a verifier":"ignore",SambaEchantillon[LectEnCours.echantillon].voie,LectStampsLus+1,LectEnCours.is_status?"c'est":"ce n'est pas");
		repart->top = repart->bottom = 0; lue = 0;
		return(lue);
	}
	if(PCIedw[num].dma) {
		if(CLUZEL_Empty(*donnee)) {
			RepartPci.vide++;
			if(log) printf("          %s: on a vide la pile, il y avait %d donnee%s\n",repart->nom,Accord1s(repart->bottom));
			if(log) printf("          . Valeur de type %X (%s), voie %d [bloc %lld], %s le status\n",
						   LectTypePrevu,verifie_type?"a verifier":"ignore",SambaEchantillon[LectEnCours.echantillon].voie,LectStampsLus+1,LectEnCours.is_status?"c'est":"ce n'est pas");
			if(FifoPrecedente) {
				FifoTopPrec = repart->bottom + 2;
				memcpy(FifoPrecedente,repart->fifo32,FifoTopPrec * sizeof(Ulong));
			}
			repart->top = repart->bottom = 0; lue = 0;
			return(lue);
		}
		if(CLUZEL_HalfFull(*donnee)) RepartPci.halffull++;
		if(CLUZEL_FullFull(*donnee)) RepartPci.fullfull++;
	}
#endif /* RESERVOIRS */
	repart->donnees_recues++;

#endif /* ACCES_PCI_DIRECT */

	if(RepartPci.fullfull) { LectDeclencheErreur(_ORIGINE_,54,LECT_TOOMUCH); return(0); }

	return(lue);
}
#ifdef OPERA_SUPPORT
#pragma mark ----------- Opera -----------
/* ========================================================================== */
char OperaNouvelle(TypeRepartiteur *repart, char *adrs, char categ,
	int mode, byte retard, char horloge) {
	//- char message[256];

	if(!repart) { OpiumFail("! Repartiteur indefini"); return(0); }
	else if(!OperaModele) { OpiumFail("! Type de repartiteur indefini: '%s'",RepartFamilleListe[FAMILLE_OPERA]); return(0); }
	repart->type = OperaModele - 1;
	strcpy(repart->parm,adrs);
	repart->valide = 1;
	strcpy(repart->maintenance,"../../Opera/maintenance_standard");
	repart->categ = categ;
	repart->nbident = 0;
	repart->nbentrees = 0; repart->entree = 0;
	repart->nbdonnees = 0;
	repart->famille = ModeleRep[repart->type].famille;
	repart->interf = ModeleRep[repart->type].interface;
	repart->r.opera.mode = mode;
	repart->r.opera.d0 = 21;
	repart->r.opera.d2d3 = code_synchro_100000;
	repart->r.opera.retard = retard;
	repart->r.opera.clck = horloge; /* interne/externe */
	repart->r.opera.sync = horloge; /* interne/externe */
	repart->ecr.port = _port_serveur_UDP;
	strcpy(repart->p.ip.user,"root");
	repart->p.ip.pswd[0] = '\0';
	repart->i.ip.nbactn = 0;
	repart->d.ip.nbactn = 0;
	//- RepartiteurVerifieParmsEtFIFO(repart,message);
	return(1);
}
/* ========================================================================== */
void OperaRecopieStatus(Structure_trame_status *trame, TypeRepartiteur *repart) {
	OperaNbD3 = trame->status_opera.temps_seconde;
	if(trame->status_opera.micro_bbv2 == 1) {
		if(repart->status_fpga == 0) strncpy(RepartInfoFpgaBB,"Startup charge /var/*.rbf",REPART_INFOFPGA_DIM);
		else strcpy(RepartInfoFpgaBB,"Startup termine");
		repart->arrete = 1;
	} else if(trame->status_opera.micro_bbv2 == 2) {
		if(repart->status_fpga == 1) strncpy(RepartInfoFpgaBB,"Ouverture de /var/*.rbf",REPART_INFOFPGA_DIM);
		else strncpy(RepartInfoFpgaBB,"/var/*.rbf ouvert",REPART_INFOFPGA_DIM);
	} else if(trame->status_opera.micro_bbv2 > 0) {
		int stade;
		stade = trame->status_opera.micro_bbv2 >> 8;
		if(stade <= 100) snprintf(RepartInfoFpgaBB,REPART_INFOFPGA_DIM,"%3d/100 prgm BB#%02d",
			stade,trame->status_opera.micro_bbv2 & 0xff);
		else {
			int port; byte buffer[8];
			strcpy(RepartInfoFpgaBB,"Redemarrage");
			port = PortLectRep + repart->rep;
			buffer[0] = 'S'; buffer[1] = 0;
			buffer[2] = (byte)(port & 0xFF); buffer[3] = (byte)((port >> 8) & 0xFF);
			RepartIpEcrit(repart,buffer,4); SambaMicrosec(ModeleRep[repart->type].delai_msg);
			printf("%s/   . Ecrit: %c.%02X.%02X.%02X\n",DateHeure(),buffer[0],buffer[1],buffer[2],buffer[3]);
			buffer[0] = 'P'; buffer[1] = 0xF0;
			RepartIpEcrit(repart,buffer,4); SambaMicrosec(ModeleRep[repart->type].delai_msg);
			printf("%s/   . Ecrit: %c.%02X.%02X.%02X\n",DateHeure(),buffer[0],buffer[1],buffer[2],buffer[3]);
			repart->arrete = 0;
		}
	} else {
		if(repart->status_fpga == 2) strcpy(RepartInfoFpgaBB,"*.rbf ou BB illisible");
		else if(repart->status_fpga > 2) strcpy(RepartInfoFpgaBB,"Verification en erreur");
		else strcpy(RepartInfoFpgaBB,"inchange");
	}
	RepartInfoFpgaBB[REPART_INFOFPGA_DIM-1] = '\0';
	OperaErreursCew = trame->status_opera.nb_erreurs_synchro_cew;
	OperaErreursTimeStamp = trame->status_opera.nb_erreurs_timestamp;
	OperaErreursSynchro = trame->status_opera.erreurs_synchro_opera;
	RepartVersionLogiciel = trame->status_opera.version_cew;
	repart->r.opera.d0 = OPERA_TRAME_D0(trame);
	repart->r.opera.retard = OPERA_TRAME_RETARD(trame);
	repart->r.opera.mode = OPERA_TRAME_TYPE_ACQUIS(trame);
	OperaMasque = OPERA_TRAME_MASQUE_BB(trame);
	repart->r.opera.d2d3 = OPERA_TRAME_CODE_SYNCHRO(trame);
	repart->r.opera.sync  = (repart->r.opera.d2d3 >> 2) & 0x01;
	repart->r.opera.clck = (repart->r.opera.d2d3 >> 3) & 0x01;
	repart->r.opera.d2d3  &= 0x03;
	strcpy(repart->date_status,DateHeure());
	repart->status_relu = 1;
	repart->status_fpga = trame->status_opera.micro_bbv2;
	NumeriseurEtatStamp = OperaNbD3; strcpy(NumeriseurEtatDate,repart->date_status);
}
/* ==========================================================================
static int OperaEntreesNb(shex code_entrees) {
	int i,n;

	n = 0;
	for(i=0; i<16; i++) if(code_entrees & (1 << i)) n++;
	return(n);
}
   ========================================================================== */
static int OperaEntreesLimite(TypeRepartiteur *repart, shex *code_entrees) {
	int i,n,max; shex code_final;

	max = repart->maxentrees;
	n = 0; code_final = 0;
	for(i=0; i<16; i++) if(*code_entrees & (1 << i)) {
		code_final |= (1 << i); n++;
		if(n >= max) break;
	};
	*code_entrees = code_final;
	return(n);
}
/* ========================================================================== */
static shex OperaEncodeSelecteur(TypeRepartiteur *repart, shex code_entrees, int voies_par_mot) {
	shex entrees,sel,mots;
	int i;

	if(!code_entrees) return(1);
	entrees = code_entrees;
	OperaEntreesLimite(repart,&entrees);
	mots = repart->voies_par_numer / voies_par_mot; // voies_par_numer deja fourni par un OperaDimensions prealable
	sel = 0;
	for(i=0; i<5; i++) { if(entrees & (1 << i)) sel |= mots; mots = (mots << 3); }
	return(sel);
}
/* ========================================================================== */
static shex OperaDecodeSelecteur(TypeRepartiteur *repart, shex selecteur) {
	shex sel; int i;
	shex code_entrees;

	sel = selecteur; code_entrees = 0; i = 0;
	while(sel) { if(sel & 0x7) code_entrees |= (1 << i); sel = sel >> 3; i++; }
	OperaEntreesLimite(repart,&code_entrees);
	return(code_entrees);
}
/* ========================================================================== */
static void OperaReset(TypeRepartiteur *repart, float echantillonage, char avec_opium) {
	int i,l,n,selection; //,nb
	byte buffer[16]; char explics[256];

	if(repart->categ != DONNEES_STAMP) {
		selection = 0;
		repart->nbdonnees = 0;
		if(repart->entree) for(i=0; i<repart->nbentrees; i++) {
			if(repart->entree[i].adrs > 0)  {
				selection = (selection | (1 << i));
				repart->nbdonnees += repart->voies_par_numer; // vpn constant, dans le cas Opera
			}
		} else { selection = 1; repart->nbdonnees = 1; }
		// for(i=0; i<repart->nbdonnees; i++) repart->donnee[i] = -1;
		printf("            . Ajustement de l'horloge, retard sur fibre et codes d'acquisition\n");
		n = OperaSelecteur(repart,selection,echantillonage,buffer,&l,explics);
		OperaPrintReglages(repart,"            .",0);
		if(l) {
			printf("              . ecrit: %c",buffer[0]);
			for(i=1; i<l; i++) printf(".%02X",buffer[i]);
			printf("\n");
			if(explics[0]) printf("            ! %s\n",explics);
		} else printf("            ! pas de chemin d'acces a ce repartiteur!\n");
#ifdef RAZ_EN_IDENT
		if(repart->bb) {
			printf("            . Mise generale en identification\n");
			RepartiteurDemarreNumeriseurs(repart,NUMER_MODE_IDENT,NUMER_DIFFUSION,0);
		}
#endif
	} else printf("            . Boitier de categorie %s: non modifie\n",RepartDonneesListe[repart->categ]);
}
/* ========================================================================== */
void OperaDimensions(int mode, shex *d1, int *voies_par_mot, short *vpn) {
	int bbvoies;
	switch(mode) {
		case code_acqui_EDW_BB1:
		case code_acqui_EDW_BB21:
			//	case code_acqui_EDW_FBB:
			*d1 = 60; bbvoies = 3; *voies_par_mot = VOIES_PAR_4OCTETS; break;
		case code_acqui_EDW_BB2:
			*d1 = 60; bbvoies = 6; *voies_par_mot = VOIES_PAR_4OCTETS; break;
		/* case code_acqui_repartiteur_veto: anciennement 4
			*d1 = 60; bbvoies = 2; *voies_par_mot = VOIES_PAR_4OCTETS; break; */
		case code_acqui_veto:
			*d1 = 60; bbvoies = 2; *voies_par_mot = VOIES_PAR_4OCTETS; break;
		case code_acqui_EDW_BB1_moy4:
			*d1 = 240; bbvoies = 3; *voies_par_mot = VOIES_PAR_4OCTETS; break;
		case code_acqui_synchro:
		case code_acqui_synchro_cluzel:
			*d1 = 60; bbvoies = 1; *voies_par_mot = 1; break;
		case code_acqui_ADC_moy2_comp:
			*d1 = 9; bbvoies = 1; *voies_par_mot = 1; break;
		default: *d1 = 18; bbvoies = 1; *voies_par_mot = 1;
	}
	if(vpn) {
		if(bbvoies < 2) *vpn = 1;
		else *vpn = (((bbvoies - 1) / VOIES_PAR_4OCTETS) + 1) * VOIES_PAR_4OCTETS;
	}
}
/* ========================================================================== */
static void OperaCompteursTrame(TypeRepartiteur *repart) {
	int nb_bn,nbdonnees,lngr_synchro,nb_stamps;
	shex d1; int voies_par_mot; short vpn;

	OperaDimensions(repart->r.opera.mode,&d1,&voies_par_mot,&vpn);
	repart->dim.totale = sizeof(TypeOperaTrame);
	repart->dim.entete = OPERA_TAILLE_ENTETE;
	repart->s.ip.num_sts = OPERA_TRAME_STS - 1; // test si > pour IPE, si = FFFF si OPERA
	if(repart->nbdonnees) nbdonnees = repart->nbdonnees;
	else {
		OperaCodeEntrees = OperaDecodeSelecteur(repart,OperaMasque);
		nb_bn = OperaEntreesLimite(repart,&OperaCodeEntrees);
		nbdonnees = nb_bn * vpn;
		if(!nbdonnees) nbdonnees = 1;
	}
	// repart->s.ip.duree_trame = ((OPERA_TAILLE_DONNEES - 1) / (nbdonnees * sizeof(TypeDonnee))) + 1;
	repart->s.ip.duree_trame = OPERA_TAILLE_DONNEES / (nbdonnees * sizeof(TypeDonnee)); // ((OPERA_TAILLE_DONNEES - 1) / (nbdonnees * sizeof(TypeDonnee))) + 1;
	repart->dim.trame = (repart->s.ip.duree_trame * nbdonnees * sizeof(TypeDonnee)) + OPERA_TAILLE_ENTETE;
	_calcul_valeur_synchro(lngr_synchro,repart->r.opera.d2d3);
	repart->s.ip.stampD3 = (int64)lngr_synchro;
	repart->s.ip.inc_stamp = repart->s.ip.stampD3;
	nb_stamps = (nbdonnees >= VOIES_PAR_4OCTETS)? repart->s.ip.duree_trame: _nb_data_trame_udp;
	repart->s.ip.tramesD3 = (lngr_synchro - 1) / nb_stamps;
	repart->s.ip.dim_D3 = (int)repart->s.ip.inc_stamp * nbdonnees;
	repart->s.ip.taille_derniere = (int)(repart->s.ip.stampD3 - (repart->s.ip.duree_trame * repart->s.ip.tramesD3));
	repart->gene.intervalle = (int64)(1000.0 * (float)(repart->s.ip.duree_trame) / Echantillonnage);
}
/* ========================================================================== */
static void OperaConfigure(TypeRepartiteur *repart) {
	int l,nb_bn,voies_par_mot,type,vsnid; shex d1;

	OperaDimensions(repart->r.opera.mode,&d1,&voies_par_mot,&(repart->voies_par_numer));
	OperaCodeEntrees = OperaDecodeSelecteur(repart,OperaMasque);
	nb_bn = OperaEntreesLimite(repart,&OperaCodeEntrees);
	vsnid = 0;
	if(repart->r.opera.mode == code_acqui_EDW_BB1) {
		type = NumeriseurTypeBB[1];
	} else if((repart->r.opera.mode == code_acqui_EDW_BB21) || (repart->r.opera.mode == code_acqui_EDW_BB2)) {
		type = NumeriseurTypeBB[2]; vsnid = 2; // == ModeleBN[type].ident !..
	} else type = -1;
	// printf("* Branchement de %d numeriseur(s) de type #%d (%s)\n",nb_bn,type,(type < 0)?"inconnu":ModeleBN[type].nom);
	RepartiteurAccepteEntrees(repart,nb_bn);
	for(l=0; l<nb_bn; l++) if(!repart->entree[l].adrs && (l < NUMER_ANONM_MAX)) {
		repart->entree[l].adrs = (TypeAdrs)(&(NumeriseurAnonyme[0]));
		NumeriseurNeuf(&(NumeriseurAnonyme[l]),type,0,repart);
		NumeriseurAnonyme[l].entree = l;
		NumeriseurAnonyme[l].avec_status = (vsnid > 1);
		NumeriseurAnonyme[l].vsnid = vsnid;
		NumeriseurAnonyme[l].repart = repart;
		NumeriseurAnonyme[l].bb = -1;
		break;
	}
	OperaCompteursTrame(repart);
}
/* ========================================================================== */
static void OperaPrintReglages(TypeRepartiteur *repart, char *prefixe, int lngr) {
	int k;

	k = printf("%s D0=%d, D2xD3=%d, retard=%d, mode %s %d voie%s, horloge %s, synchro %s",prefixe,
		repart->r.opera.d0,repart->r.opera.d2d3,repart->r.opera.retard,
		OperaCodesAcquis[repart->r.opera.mode],Accord1s(repart->r.opera.lngr_echant),
		repart->r.opera.clck? "externe":"interne",repart->r.opera.sync? "externe":"interne");
	if(lngr) SambaLigneTable(' ',k,lngr); else printf("\n");
	RepartImprimeCompteurs(repart,prefixe,lngr);
}
/* ========================================================================== */
int OperaSelecteur(TypeRepartiteur *repart, int selection, float echantillonage, byte *buffer, int *l, char *explics) {
	int modl,voies_par_mot; int n; shex diviseur1;

	explics[0] = '\0';
	if(!RepartIpEcriturePrete(repart,"  ")) { *l = 0; return(0); }
	modl = repart->type;
	OperaDimensions(repart->r.opera.mode,&diviseur1,&voies_par_mot,&(repart->voies_par_numer));
	repart->r.opera.d0 = (shex)((ModeleRep[modl].horloge * 1000.0 / (echantillonage * (float)diviseur1)) + 0.5);
	buffer[0] = 'h';
	buffer[1] = _FPGA_code_horloge;
	buffer[2] = (byte)(repart->r.opera.d0 & 0xFF);
	buffer[3] = (byte)((repart->r.opera.d0 >> 8) & 0xFF);
	/* retard du aux fibres, 4 ns/coup (au lieu de 5 avant, l'horloge etant passee de 200 a 250 MHz, soit un coup d'horloge en 4ns au lieu de 5) */
	if(!repart->r.opera.retard) repart->r.opera.retard = SettingsRetard + (SettingsFibreLngr * 6) / 4;
	buffer[4] = repart->r.opera.retard;
	/* masque BB (nb.mots 32 bits par 3 bits, soit b'010010 si 2 BB de 3 voies */
	buffer[5] = (byte)OperaEncodeSelecteur(repart,selection,voies_par_mot);
	buffer[6] = repart->r.opera.mode;
	buffer[7] = repart->r.opera.d2d3;
	if(repart->r.opera.clck) buffer[7] += code_synchro_esclave_temps;
	if(repart->r.opera.sync) buffer[7] += code_synchro_esclave_synchro;
	repart->r.opera.lngr_echant = (((buffer[5] >> 3) & 0x7) + (buffer[5] & 0x7)) * voies_par_mot; /* selection en mots de 32 bits */
	if(repart->r.opera.lngr_echant != repart->nbdonnees) {
		sprintf(explics,"%d voie%s finalement delivree%s, au lieu de %d",Accord2s(repart->r.opera.lngr_echant),repart->nbdonnees);
		// if(repart->r.opera.lngr_echant > repart->nbdonnees) for(i=repart->nbdonnees; i<repart->r.opera.lngr_echant; i++) repart->donnee[i] = -1;
		// repart->nbdonnees = repart->r.opera.lngr_echant;
	}
	OperaCompteursTrame(repart);
	*l = 8;
	n = RepartIpEcrit(repart,buffer,*l); SambaMicrosec(ModeleRep[modl].delai_msg);
	return(n);
}
/* ========================================================================== */
static int OperaCmdeFpga(TypeRepartiteur *repart, int fpga) {
	byte buffer[4]; int port;

	if(!RepartIpEcriturePrete(repart,"            ")) return(0);
	if(Acquis[AcquisLocale].etat.active) {
		port = PortLectRep + repart->rep;
		buffer[0] = 'P'; buffer[1] = 0;
		buffer[2] = (byte)(port & 0xFF); buffer[3] = (byte)((port >> 8) & 0xFF);
		RepartIpEcrit(repart,buffer,4); SambaMicrosec(ModeleRep[repart->type].delai_msg);
		printf("%s/   . Ecrit: %c.%02X.%02X.%02X\n",DateHeure(),buffer[0],buffer[1],buffer[2],buffer[3]);
		buffer[0] = 'S'; buffer[1] = 0xF0;
		RepartIpEcrit(repart,buffer,4); SambaMicrosec(ModeleRep[repart->type].delai_msg);
		printf("%s/   . Ecrit: %c.%02X.%02X.%02X\n",DateHeure(),buffer[0],buffer[1],buffer[2],buffer[3]);
	}
	buffer[0] = 'Q'; buffer[1] = (byte)FpgaModl[fpga].opera;
	RepartIpEcrit(repart,buffer,2); SambaMicrosec(ModeleRep[repart->type].delai_msg);
	printf("%s/   . Ecrit: %c.%02X (mise a jour %s)\n",DateHeure(),buffer[0],buffer[1],OperaCodesMaj[OperaParmQ]);
	return(0);
}
/* ========================================================================== */
static int OperaCmdeQ(TypeRepartiteur *repart) {
	byte buffer[4],code; int port; int i;

	if(!RepartIpEcriturePrete(repart,"            ")) return(0);
	code = OperaValMaj[OperaParmQ];
	if(Acquis[AcquisLocale].etat.active) {
		// ((OperaParmQ == 2) || (OperaParmQ == 3) || (OperaParmQ == 4))
		for(i=0; i<NUMER_FPGA_TYPES; i++) if(code == FpgaModl[i].opera) break;
		if(i < NUMER_FPGA_TYPES) {
			port = PortLectRep + repart->rep;
			buffer[0] = 'P'; buffer[1] = 0;
			buffer[2] = (byte)(port & 0xFF); buffer[3] = (byte)((port >> 8) & 0xFF);
			RepartIpEcrit(repart,buffer,4); SambaMicrosec(ModeleRep[repart->type].delai_msg);
			printf("%s/   . Ecrit: %c.%02X.%02X.%02X\n",DateHeure(),buffer[0],buffer[1],buffer[2],buffer[3]);
			buffer[0] = 'S'; buffer[1] = 0xF0;
			RepartIpEcrit(repart,buffer,4); SambaMicrosec(ModeleRep[repart->type].delai_msg);
			printf("%s/   . Ecrit: %c.%02X.%02X.%02X\n",DateHeure(),buffer[0],buffer[1],buffer[2],buffer[3]);
		}
	}
	buffer[0] = 'Q'; buffer[1] = code;
	RepartIpEcrit(repart,buffer,2); SambaMicrosec(ModeleRep[repart->type].delai_msg);
	printf("%s/   . Ecrit: %c.%02X (mise a jour %s)\n",DateHeure(),buffer[0],buffer[1],OperaCodesMaj[OperaParmQ]);
	return(0);
}
/* ========================================================================== */
static int OperaCmdeH(TypeRepartiteur *repart) {
	byte buffer[8]; int i,l;
	int voies_par_mot,nb_bn; shex d1;

	OperaDimensions(repart->r.opera.mode,&d1,&voies_par_mot,&(repart->voies_par_numer));
	nb_bn = OperaEntreesLimite(repart,&OperaCodeEntrees);
	RepartiteurAccepteEntrees(repart,nb_bn);
	repart->nbdonnees = nb_bn * repart->voies_par_numer;
	// for(l=0; l<repart->nbdonnees; l++) repart->donnee[l] = -1;
	OperaMasque = OperaEncodeSelecteur(repart,OperaCodeEntrees,voies_par_mot);
	OperaCompteursTrame(repart);

	if(pRepartStatus && OpiumDisplayed(pRepartStatus->cdr)) PanelRefreshVars(pRepartStatus);

	if(RepartIpEcriturePrete(repart,"            ")) {
		buffer[0] = 'h';
		buffer[1] = _FPGA_code_horloge;
		buffer[2] = (byte)(repart->r.opera.d0 & 0xFF);  /* soit frequence a 100 MHz/5/18 = 1,1 MHz */
		buffer[3] = (byte)((repart->r.opera.d0 >> 8) & 0xFF);
		buffer[4] = (byte)repart->r.opera.retard;       /* retard du aux fibres */
		buffer[5] = (byte)OperaMasque;                  /* masque BB (nb.mots 32 bits par 3 bits, soit b'010010 si 2 BB de 3 voies */
		buffer[6] = repart->r.opera.mode;               /* code acquisition */
		buffer[7] = repart->r.opera.d2d3
			+ (repart->r.opera.sync? code_synchro_esclave_synchro: 0)
			+ (repart->r.opera.clck? code_synchro_esclave_temps: 0); /* code synchro */
		l = 8;
		RepartIpEcrit(repart,buffer,l); SambaMicrosec(ModeleRep[repart->type].delai_msg);
		printf("            . Ecrit: %c",buffer[0]);
		for(i=1; i<l; i++) printf(".%02X",buffer[i]);
		printf(" (D0=%d, D2xD3=%d, retard=%d, mode %s %d voie%s, horloge %s, synchro %s)\n",
			repart->r.opera.d0,repart->r.opera.d2d3,repart->r.opera.retard,
			OperaCodesAcquis[repart->r.opera.mode],Accord1s(repart->nbdonnees),
			repart->r.opera.clck? "externe":"interne",repart->r.opera.sync? "externe":"interne");
	} else printf("          ! Parametrage de %s impossible: pas de chemin d'ecriture\n",repart->nom);
	RepartiteurParmModifie = 0;
	return(0);
}
/* ========================================================================== */
INLINE void OperaSwappeTrame(void *buffer, int nb) {
	ByteSwappeIntArray((hexa *)buffer,nb / sizeof(hexa));
}
/* ========================================================================== */
void OperaDump(byte *buffer, int nb, char hexa) {
	int i,j,num,nb_d3; short *court; int d2d3; short acquis,horloges,nb1,nb2; char sync,clck;
	char deja_vu;

	if(nb == -1) {
		if(errno == EAGAIN) printf("===== Lecture: tampon vide\n");
		else printf("===== Lecture en erreur (%s)\n",strerror(errno));
	} else {
		printf("===== Lecture de %d octets, ",nb);
		court = 0; // GCC
		nb_d3 = OPERA_TRAME_SYNCHRO_NUM(buffer);
		num = OPERA_TRAME_NUMERO(buffer);
		printf("trame No %d.%05d",nb_d3,num);
		if(num == OPERA_TRAME_STS) {
			printf(" (status)\n");
			acquis = OPERA_TRAME_TYPE_ACQUIS(buffer);
			horloges = OPERA_TRAME_CODE_SYNCHRO(buffer);
			_calcul_valeur_synchro(d2d3,horloges);
			sync = (OPERA_TRAME_CODE_SYNCHRO(buffer) >> 2) & 0x1;
			clck = (OPERA_TRAME_CODE_SYNCHRO(buffer) >> 3) & 0x1;
			nb1 = (OPERA_TRAME_MASQUE_BB(buffer) >> 0) & 0x7;
			nb2 = (OPERA_TRAME_MASQUE_BB(buffer) >> 3) & 0x7;
			printf("Diviseur         : %04X (%d)\n",OPERA_TRAME_D0(buffer),OPERA_TRAME_D0(buffer));
			printf("Retard           : %04X (%d)\n",OPERA_TRAME_RETARD(buffer),OPERA_TRAME_RETARD(buffer));
			printf("Code acquis      : %04X (%s)\n",acquis,((acquis >= 0) && (acquis < OPERA_NBMODES))? OperaCodesAcquis[acquis]: "illegal");
			printf("Code synchro     : %04X (D2D3 = %d)\n",horloges,d2d3);
			printf("Esclave sync     : %04X (%s: synchro %s)\n",sync,sync? "oui": "non",sync? "externe": "interne");
			printf("Esclave clock    : %04X (%s: horloge %s)\n",clck,clck? "oui": "non",clck? "externe": "interne");
			printf("Nb mots entree 1 : %04X (%d donnee%s)\n",nb1,Accord1s(nb1));
			printf("Nb mots entree 2 : %04X (%d donnee%s)\n",nb2,Accord1s(nb2));
			printf("Timestamp        : %08llX (%lld)\n",OPERA_TRAME_TIMESTAMP(buffer),OPERA_TRAME_TIMESTAMP(buffer));
		} else printf(" (donnees):");
		if(hexa) {
			if(!BigEndianOrder) OperaSwappeTrame((void *)buffer,nb);
		} else court = (short *)buffer;
		deja_vu = 0 ;
		for(i=0; i<nb; i++) {
			if(!(i % 16)) {
				if(!deja_vu) printf("\n%4d:",i);
				if(i) {
					if(!memcmp(buffer+i,buffer+i-16,16)) {
						if(!deja_vu) printf(" ...");
						deja_vu = 1;
						i += 15;
					} else {
						if(deja_vu) printf("\n%4d:",i);
						deja_vu = 0;
					}
				}
			} else if(!(i % 4)) printf("  ");
			if(!deja_vu) {
				if(hexa) printf(" %02X",buffer[i]); else {
					j = i / 2;
					if(!BigEndianOrder) { if((j % 2)) printf(" %6d",court[j-1]); else printf(" %6d",court[j+1]); }
					else if(court) printf(" %6d",court[j]);
					i++;
				}
			}
		}
		printf("\n");
		if(hexa && !BigEndianOrder) OperaSwappeTrame((void *)buffer,nb);
	}
}
/* ========================================================================== */
static int OperaChangeParms() {
	int rep; TypeRepartiteur *repart;

	if(RepartConnecte->rep >= 0) OperaCmdeH(RepartConnecte);
	else for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) /* anciennement select */ {
		repart = &(Repart[rep]);
		repart->r.opera.d0 = RepartConnecte->r.opera.d0;
		repart->r.opera.retard = RepartConnecte->r.opera.retard;
		repart->r.opera.d2d3 = RepartConnecte->r.opera.d2d3;
		repart->r.opera.clck = RepartConnecte->r.opera.clck;
		repart->r.opera.sync = RepartConnecte->r.opera.sync;
		repart->r.opera.mode = RepartConnecte->r.opera.mode;
		printf("          * %s:\n",repart->nom);
		OperaCmdeH(repart);
	}
	return(0);
}
/* ========================================================================== */
static int OperaMaj() {
	int rep;

	if(RepartConnecte->rep >= 0) OperaCmdeQ(RepartConnecte);
	else for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local && (Repart[rep].famille == FAMILLE_OPERA))  /* anciennement select */{
		printf("          * %s:\n",Repart[rep].nom);
		OperaCmdeQ(&(Repart[rep]));
	}
	return(0);
}
/* ========================================================================== */
static int OperaDelayM() {
	if(RepartConnecte->r.opera.retard) RepartConnecte->r.opera.retard -= 1;
	OperaChangeParms();
	return(0);
}
/* ========================================================================== */
static int OperaDelayP() {
	RepartConnecte->r.opera.retard += 1;
	OperaChangeParms();
	return(0);
}
/* ========================================================================== */
static int OperaClockM() {
	if(RepartConnecte->r.opera.d0) RepartConnecte->r.opera.d0 -= 1;
	OperaChangeParms();
	return(0);
}
/* ========================================================================== */
static int OperaClockP() {
	RepartConnecte->r.opera.d0 += 1;
	OperaChangeParms();
	return(0);
}
/* ========================================================================== */
INLINE static void OperaDemarreNumeriseurs(TypeRepartiteur *repart, NUMER_MODE mode, byte adrs, char *prefixe) {
	byte buffer[8]; shex cde_cluzel;

	if(mode == NUMER_MODE_COMPTEUR) {
		if(prefixe) printf("%s! Mode '%s' pas implemente\n",prefixe,NumerModeTexte[mode]);
	} else {
		buffer[0] = OPERA_CDE_BB; buffer[1] = _FPGA_code_EDW; buffer[2] = adrs; buffer[3] = CLUZEL_SSAD_ACQUIS;
		cde_cluzel = (mode == NUMER_MODE_ACQUIS)? CLUZEL_START: CLUZEL_STOP;
		buffer[4] = (byte)((cde_cluzel >> 8) & 0xFF); buffer[5] = (byte)(cde_cluzel & 0xFF); buffer[6] = '\0';
		RepartIpEcrit(repart,buffer,6); SambaMicrosec(ModeleRep[repart->type].delai_msg);
		if(prefixe) printf("%s. Ecrit: %c.%02X.%02X.%02X.%02X%02X\n",prefixe,buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]);
	}
}
/* ========================================================================== */
INLINE static void OperaEcritRessourceBB(TypeRepartiteur *repart, shex adrs, hexa ss_adrs, TypeADU valeur) {
	byte buffer[8]; int n;

	buffer[0] = OPERA_CDE_BB;
	buffer[1] = _FPGA_code_EDW;
	buffer[2] = adrs;
	buffer[3] = (byte)(ss_adrs & 0xFF);
	buffer[4] = (byte)((valeur >> 8) & 0xFF);
	buffer[5] = (byte)(valeur & 0xFF);
	n = RepartIpEcrit(repart,buffer,6); SambaMicrosec(ModeleRep[repart->type].delai_msg);
	sprintf(RepartiteurValeurEnvoyee,"%c.%02X.%02X.%02X.%02X%02X",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]);
}
/* ==========================================================================
INLINE static void OperaEcritNumeriseur(TypeRepartiteur *repart, byte *cmde, int lngr) {
	byte buffer[8]; int i,j,n;

	i = 0; buffer[i++] = OPERA_CDE_BB; buffer[i++] = _FPGA_code_EDW;
	for(j=0; j<lngr; j++) buffer[i++] = cmde[j];
	n = RepartIpEcrit(repart,buffer,i); SambaMicrosec(ModeleRep[repart->type].delai_msg);
	sprintf(RepartiteurValeurEnvoyee,"%c.%02X.%02X.%02X.%02X%02X",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]);
}
   ========================================================================== */
static INLINE char OperaVidePile(TypeRepartiteur *repart, char *pas_tous_secs, short *pleins, char log) {
#ifdef AVEC_OPERA
	int octets_lus,donnees_lues,num_trame,haut_pile,sautees,nb_stamps,stamp_absents,lues,nb;
	hexa lngr;
	// char episode_en_cours;
	byte *pile,fin_trame[OPERA_TAILLE_ENTETE];
	OperaTrame trame;
	char trame_inattendue;
	int64 timestamp,stamp_attendu;
	int64 t2,t3;

	if(repart->top < 0) { LectDeclencheErreur(_ORIGINE_,42,LECT_INCOHER); return(0); }
	if(repart->s.ip.manquant) return(0); // generer les donnees manquantes avant d'en rajouter: test sur (top - bottom) prioritaire
	haut_pile = repart->top * sizeof(TypeDonnee);
	if((haut_pile + sizeof(TypeOperaTrame)) > repart->octets) { *pleins += 1; return(0); }
	pile = (byte *)repart->fifo32; /* data16 est decalee de OPERA_TAILLE_ENTETE octets */
	if(repart->top) memcpy(fin_trame,pile+haut_pile,OPERA_TAILLE_ENTETE);
	trame = (OperaTrame)(pile + haut_pile);
	lngr = sizeof(TypeSocket);
	RepartIpEssais++;
	RepartIp.acces++;
	repart->acces_demandes++;
	lues = 0;
	// if(repart->s.ip.stamp_redemande >= 0)
	t2 = VerifTempsPasse? (DateMicroSecs() - LectT0Run): 0; t3 = t2;
	octets_lus = RepartIpLitTrame(repart,trame,sizeof(TypeOperaTrame));
	if(repart->s.ip.stamp_redemande >= 0) t3 = VerifTempsPasse? (DateMicroSecs() - LectT0Run): 0;
	if(octets_lus == -1) {
		if(errno == EAGAIN) {
			if(log) printf("          %s: vide\n",repart->nom);
			RepartIp.vide++;
			repart->acces_vides++;
			repart->serie_vide++;
			repart->dernier_vide = t2;
			if(repart->s.ip.stamp_redemande >= 0) { repart->s.ip.duree_vide += (t3 - t2); repart->s.ip.nb_vide++; }
			return(0);
		} else { LectDeclencheErreur(_ORIGINE_,43,LECT_EMPTY); return(0); }
	} else if((octets_lus <= 0) || (octets_lus > sizeof(TypeOperaTrame))) {
		LectDeclencheErreur(_ORIGINE_,44,LECT_UNKN); return(0);
	} else if(((TypeSocketIP *)(&(repart->lec.ip.skt)))->sin_addr.s_addr != repart->adrs.val) {
		LectDeclencheErreur(_ORIGINE_,45,LECT_UNEXPEC); return(0);
	}
	if(log) {
		donnees_lues = (octets_lus - OPERA_TAILLE_ENTETE) / sizeof(TypeDonnee); /* 2 octets par voie */
		printf("          %s: %d valeur%s\n",repart->nom,Accord1s(donnees_lues));
	}
	repart->acces_remplis++;
	repart->serie_vide = 0;
	repart->dernier_rempli = t2;
	*pas_tous_secs = 1;
	RepartIpRecues++;
	//a gettimeofday(&LectDateRun,0);
	//a dernier_temps = (float)(LectDateRun.tv_sec % 60) + ((float)LectDateRun.tv_usec / 1000000.0);
	if(BigEndianOrder) ByteSwappeIntArray((hexa *)trame,octets_lus / sizeof(Ulong));
	num_trame = OPERA_TRAME_NUMERO(trame);
	if(num_trame == OPERA_TRAME_STS) {
		char chgt; int err; int64 nbD3;
		repart->s.ip.stampDernier = OPERA_TRAME_TIMESTAMP(trame);
		nbD3 = repart->s.ip.stampDernier / repart->s.ip.inc_stamp;
		if(repart->s.ip.stamp0 == 0) {
		#ifdef OPERA_STATUS_DANS_ENTETE
			trame = (OperaTrame)pile;
			repart->s.ip.stamp0 = OPERA_TRAME_TIMESTAMP(trame);
		#else
			repart->s.ip.stamp0 = repart->s.ip.stampDernier;
		#endif
			repart->s.ip.num0 = 0;
			if((LectTimeStamp0 == 0) && (!LectStampSpecifique || (repart->categ == DONNEES_STAMP))) {
				LectTimeStamp0 = repart->s.ip.stamp0;
				if(repart->s.ip.stampD3 > repart->s.ip.duree_trame) LectTimeStamp0 += (int64)(repart->s.ip.num0 * repart->s.ip.duree_trame);
				LectTimeStampN = LectTimeStamp0;
				printf("            . TimeStamp initial du run: %lld (par repartiteur %s)\n",LectTimeStamp0,RepartDonneesListe[repart->categ]);
				if(repart->categ == DONNEES_STAMP) { repart->actif = 0; return(0); }
			}
		}
		if((repart->s.ip.err_cew == -1) && !ReglVi.actif) {
			printf("%s/ %s, etat initial: resynchro=%d / desynchro=%d / perdu=%d\n",DateHeure(),repart->nom,
				   ((Structure_trame_status *)trame)->status_opera.nb_erreurs_synchro_cew,
				   ((Structure_trame_status *)trame)->status_opera.nb_erreurs_timestamp,
				   ((Structure_trame_status *)trame)->status_opera.erreurs_synchro_opera);
		}
		chgt = 0;
		err = ((Structure_trame_status *)trame)->status_opera.nb_erreurs_synchro_cew; // if(!BigEndianOrder) TwoShortsSwappeInt(&err);
		if(err != repart->s.ip.err_cew) { chgt = chgt || (repart->s.ip.err_cew >= 0); repart->s.ip.err_cew = err; }
		err = ((Structure_trame_status *)trame)->status_opera.nb_erreurs_timestamp; // if(!BigEndianOrder) TwoShortsSwappeInt(&err);
		if(err != repart->s.ip.err_stamp) { chgt = chgt || (repart->s.ip.err_stamp >= 0); repart->s.ip.err_stamp = err; }
		err = ((Structure_trame_status *)trame)->status_opera.erreurs_synchro_opera; // if(!BigEndianOrder) TwoShortsSwappeInt(&err);
		if(err != repart->s.ip.err_synchro) { chgt = chgt || (repart->s.ip.err_synchro >= 0); repart->s.ip.err_synchro = err; }
		if(chgt) {
			printf("%s/ %s: erreur autodetectee, resynchro=%d / desynchro=%d / perdu=%d a la lecture #%d\n",
				   DateHeure(),repart->nom,repart->s.ip.err_cew,repart->s.ip.err_stamp,repart->s.ip.err_synchro,repart->acces_remplis);
			RepartDataErreurs++;
		}
		if(repart->status_demande && (repart == RepartConnecte)) OperaRecopieStatus((Structure_trame_status *)trame,repart);
		else if(LectLitStatus && (NumeriseurEtatDemande || ReglagesEtatDemande)) {
			NumeriseurEtatStamp = ((Structure_trame_status *)trame)->status_opera.temps_seconde;
			strcpy(NumeriseurEtatDate,DateHeure());
		}
		if(Archive.enservice && LectSauveStatus) {
			TypeNumeriseur *numeriseur; int l;
			TypeBNModele *modele_bn;
			for(l=0; l<repart->nbentrees; l++) {
				if((numeriseur = (TypeNumeriseur *)repart->entree[l].adrs)) {
					modele_bn = &(ModeleBN[numeriseur->def.type]);
					if(modele_bn->enreg_dim && !numeriseur->enreg_stamp) {
						hexa *status_lu; int nb;
						if(numeriseur->avec_status) {
							status_lu = (hexa *)(((Structure_trame_status *)trame)->status_bbv2_1);
							nb = _nb_mots_status_bbv2;
						} else status_lu = 0;
						if(status_lu) {
							int n;
							if(nb > NUMER_RESS_MAX) nb = NUMER_RESS_MAX;
							for(n=0; n<modele_bn->enreg_dim; n++) {
								numeriseur->enreg_val[n] = status_lu[modele_bn->enreg_sts[n]];
							}
							numeriseur->enreg_stamp = repart->s.ip.stampDernier;
							LectStatusDispo = 1;
						}
					}
				}
			}
		}
		if(LectLitStatus) {
			TypeNumeriseur *numeriseur; int l;
			for(l=0; l<repart->nbentrees; l++) {
				numeriseur = (TypeNumeriseur *)repart->entree[l].adrs; /* une seule trame par numeriseur */
				if(numeriseur && numeriseur->status.a_copier) {
					hexa *status_lu; int i,nb;
					if(numeriseur->avec_status) {
						status_lu = (hexa *)(((Structure_trame_status *)trame)->status_bbv2_1);
						nb = _nb_mots_status_bbv2;
					} else status_lu = 0;
					if(status_lu) {
						// for(i=0; i<nb; i++) NumeriseurRecopieStatus(numeriseur,i,*status_lu++); /* Alain nous met des short dans des int */
						for(i=0; i<nb; i++) numeriseur->status_val[i] = *status_lu++;
						numeriseur->status.nb = nb;
						strcpy(numeriseur->status.lu,DateHeure());
						numeriseur->status.stamp = NumeriseurEtatStamp;
					}
					numeriseur->status.termine = 1;
				}
			}
		}
		RepartIp.traitees++; lues++;
		*pas_tous_secs = 1;
		if(LectDureeReset) {
			int depuis_reset,avant_reset;
			depuis_reset = Modulo(nbD3,LectDureeReset);
			avant_reset = LectDureeReset - depuis_reset;
			if((depuis_reset == LectGardeReset) || ((avant_reset * repart->s.ip.inc_stamp) > CalcSpectreAutoDuree)) {
				LectDureeReset = 0;
				OpiumProgresClose();
			} else if(!OpiumProgresRefresh(LectAttenteReset-avant_reset)) { LectDureeReset = 0; OpiumProgresClose(); }
			else return(0);
		}
		if(repart->top) memcpy(pile+haut_pile,fin_trame,OPERA_TAILLE_ENTETE);
		return(0);
	} else if(LectModeStatus) return(0);
	if(repart->arrete) printf("%s! %s envoie a nouveau des donnees\n",DateHeure(),repart->nom);
	repart->arrete = 0;
#ifdef OPERA_STATUS_DANS_ENTETE
	timestamp = OPERA_TRAME_TIMESTAMP(trame);
#else
	timestamp = repart->s.ip.stampDernier;
#endif
	if(num_trame == 0) {
		if(!repart->D3trouvee) { repart->D3trouvee = 1; RepartCompteCalagesD3(); }
		if((LectTimeStamp0 == 0) && (!LectStampSpecifique || (repart->categ == DONNEES_STAMP))) {
			LectTimeStamp0 = timestamp;
			if(repart->s.ip.stampD3 > repart->s.ip.duree_trame) LectTimeStamp0 += (int64)(num_trame * repart->s.ip.duree_trame);
			LectTimeStampN = LectTimeStamp0;
			printf("            . TimeStamp initial du run: %lld (par repartiteur %s)\n",LectTimeStamp0,RepartDonneesListe[repart->categ]);
			if(repart->categ == DONNEES_STAMP) { repart->actif = 0; return(0); }
			LectDerniereD3 = 0;
		} else LectDerniereD3 = (timestamp + (int64)(num_trame * repart->s.ip.duree_trame)) - LectTimeStamp0;
		// printf("%s/ Trame %lld/%d, soit derniere D3=%lld\n",DateHeure(),timestamp,num_trame,LectDerniereD3);
	}
	trame_inattendue = 0; sautees = stamp_absents = 0;
	repart->s.ip.stamp1 = timestamp;
	repart->s.ip.num1 = num_trame;
	if(repart->s.ip.numAVenir >= 0) {
		if(repart->s.ip.stamp_redemande >= 0) {
			// episode_en_cours = 1; // si on y revient...
			if((timestamp == repart->s.ip.stamp_redemande) && (num_trame == repart->s.ip.trame_redemandee)) {
				nb = (int)(repart->acces_remplis - repart->s.ip.appel_perdant);
				printf("%s/ %s: %d trame%s recuperee%s dans pile[%d] dont %d bof%s (%.3f ms) + %d vide%s (%.3f ms)\n",DateHeure(),
					   repart->nom,Accord2s(nb),RepartIpNiveauPerte,Accord1s(repart->s.ip.nb_inutile),(float)repart->s.ip.duree_inutile/1000.0,
					   Accord1s(repart->s.ip.nb_vide),(float)repart->s.ip.duree_vide/1000.0);
				RepartIp.perdues += (repart->acces_remplis - repart->s.ip.appel_perdant);
				repart->s.ip.stampEnCours = timestamp;
				repart->s.ip.numAVenir = num_trame;
				repart->s.ip.stamp_redemande = -1; repart->s.ip.trame_redemandee = -1;
				repart->s.ip.saut = 0;
				repart->s.ip.en_defaut = 0;
				repart->s.ip.appel_perdant = 0;
			} else {
				repart->s.ip.duree_inutile += (t3 - t2); repart->s.ip.nb_inutile++;
				sautees = (int)(((timestamp - repart->s.ip.stamp_redemande) * (repart->s.ip.tramesD3 + 1) / repart->s.ip.inc_stamp) + (num_trame - repart->s.ip.trame_redemandee));
				if(sautees > repart->s.ip.tramesD3) {
					printf("%s/ %s: perdu %12lld/%3d, recue %12lld/%3d (#%lld dans pile[%d])\n",DateHeure(),
						   repart->nom,repart->s.ip.stamp_redemande,repart->s.ip.trame_redemandee,timestamp,num_trame,
						   repart->acces_remplis,RepartIpRecues);
					//a printf(" a t=%.3f/%.3f",dernier_temps,repart->temps_precedent);
					LectDeclencheErreur(_ORIGINE_,46,LECT_SYNC); return(0);
				}
			}
		} else repart->s.ip.en_defaut = 0;
		stamp_attendu = repart->s.ip.stampEnCours;
		if(num_trame != repart->s.ip.numAVenir) trame_inattendue = 1;
		else if(timestamp != stamp_attendu) { if(RepartIpCorrection == REPART_IP_INSISTE) trame_inattendue = 1; }
		if(trame_inattendue) {
			if(!LectEpisodeEnCours) {
				++LectEpisodeAgite;
				if(VerifErreurLog) printf("%s! Debut episode #%d apres precedente lecture de %lld puis attente de %lld us (lecture #%d)\n",
					DateHeure(),LectEpisodeAgite,LectDepileDuree,LectDepileAttente,LectDepileNb);
				else printf("%s! Debut episode #%d\n",DateHeure(),LectEpisodeAgite);
				LectEpisodeEnCours = 1;
			}
			repart->s.ip.en_defaut = 1;
			// episode_en_cours = 1; // manquee = 1;
			RepartIpNiveauPerte = RepartIpRecues;
			sautees = (int)(((timestamp - stamp_attendu) * (repart->s.ip.tramesD3 + 1) / repart->s.ip.inc_stamp) + (num_trame - repart->s.ip.numAVenir));
			if(VerifErreurLog) printf("%s/ %s, trame %12lld/%3d: recue %12lld/%3d (#%lld dans pile[%d]",DateHeure(),
				   repart->nom,stamp_attendu,repart->s.ip.numAVenir,timestamp,num_trame,
				   repart->acces_remplis,RepartIpNiveauPerte);
			//a printf(" a t=%.3f/%.3f",dernier_temps,repart->temps_precedent);
			//b if(repart->s.ip.stamp_redemande >= 0) printf(" + %d bof%s (%.3f ms) + %d vide%s (%.3f ms)",
			//b		Accord1s(repart->s.ip.nb_inutile),(float)repart->s.ip.duree_inutile/1000.0,Accord1s(repart->s.ip.nb_vide),(float)repart->s.ip.duree_vide/1000.0);
			if(!repart->s.ip.appel_perdant) repart->s.ip.appel_perdant = repart->acces_remplis;
			if(sautees < 0) {
				if(RepartIpCorrection == REPART_IP_INSISTE) /* loupee une deuxieme fois! */ {
					if(VerifErreurLog) printf("), perdu %12lld/%3d pour la deuxieme fois\n",repart->s.ip.stamp_redemande,repart->s.ip.trame_redemandee);
					repart->s.ip.stamp_redemande = -1; /* ouais... mais c'est quand meme foutu, on est archi noye!! */
				} else if(VerifErreurLog) printf("), %d en trop\n",-sautees);
				LectDeclencheErreur(_ORIGINE_,47,LECT_TOOMUCH); return(0);
			} else {
				if(VerifErreurLog) printf("), %d echappee%s\n",Accord1s(sautees));
				RepartIpEchappees += sautees;
				if(repart->s.ip.stamp_redemande < 0) {
					if((RepartIpCorrection == REPART_IP_INSISTE) && (sautees < 15)) {
						byte buffer[4];
						repart->s.ip.stamp_redemande = stamp_attendu;
						repart->s.ip.trame_redemandee = repart->s.ip.numAVenir;
						/* envoi de la fameuse commande 'R' avec repart->s.ip.numAVenir comme argument */
						buffer[0] = 'R'; buffer[1] = 0;
						buffer[2] = (byte)(repart->s.ip.trame_redemandee & 0xFF);
						buffer[3] = (byte)((repart->s.ip.trame_redemandee >> 8) & 0xFF);
						RepartIpEcrit(repart,buffer,4); // SambaMicrosec(ModeleRep[repart->type].delai_msg);
						// #ifdef TRACE_TRAME_NIV1
						printf("%s/ %s, trame redemandee:  %3d (%c.%02X.%02X.%02X)\n",DateHeure(),
							   repart->nom,repart->s.ip.trame_redemandee,buffer[0],buffer[1],buffer[2],buffer[3]);
						// #endif
						repart->s.ip.duree_vide = repart->s.ip.duree_inutile = 0;
						repart->s.ip.nb_vide = repart->s.ip.nb_inutile = 0;
					} else {
						int donnees_absentes;
						nb_stamps = (repart->nbdonnees >= VOIES_PAR_4OCTETS)? repart->s.ip.duree_trame: _nb_data_trame_udp;
						stamp_absents = (int)((timestamp + (num_trame * nb_stamps)) - (stamp_attendu + (repart->s.ip.numAVenir * nb_stamps)));
						LectStampPerdus += stamp_absents;
						repart->s.ip.stampPerdus += (int64)stamp_absents;
						donnees_absentes = stamp_absents * repart->nbdonnees;
						repart->s.ip.lu_entre_D3 += donnees_absentes;
						if(RepartIpCorrection == REPART_IP_GENERE) {
							// on generera ainsi des valeurs constantes au depilement
							repart->s.ip.manquant = donnees_absentes;
							repart->s.ip.saut = repart->top; /* point ou il faut commencer a generer, en esperant qu'il n'y en ait qu'un dans la pile */
							if(VerifErreurLog) printf("%s/ %s, generation de %d valeur%s constante%s x %d voie%s\n",DateHeure(),repart->nom,Accord2s(stamp_absents),Accord1s(repart->nbdonnees));
						}
						num_trame = repart->s.ip.numAVenir + sautees;
					}
				} // else printf("%s/ %s, trame NON redemandee\n",DateHeure(),repart->nom);
			}
		}
		repart->s.ip.numAVenir = num_trame + 1;
	} else {
		repart->s.ip.numAVenir = num_trame + 1; repart->s.ip.stampEnCours = timestamp;
	}
	if(repart->s.ip.numAVenir > repart->s.ip.tramesD3) {
		repart->s.ip.stampEnCours += repart->s.ip.inc_stamp;
		repart->s.ip.numAVenir -= (repart->s.ip.tramesD3 + 1);
	}
	//a repart->temps_precedent = dernier_temps;
	if(repart->s.ip.stamp_redemande >= 0) return(0);
	/* Impression brute de la derniere trame avant synchro (et la premiere apres):
	 if((repart->adrs.champ[3] == 61) && ((repart->s.ip.numAVenir == 0) || (repart->s.ip.numAVenir == 1))) {
	 printf("%s/ %s: trame %12lld/%3d (+%lld D3) @%d, %d octets maxi/trame\n",DateHeure(),
	 repart->nom,timestamp,num_trame,(timestamp-LectTimeStamp0)/repart->s.ip.inc_stamp,
	 repart->top * sizeof(TypeDonnee),sizeof(TypeOperaTrame));
	 OperaDump(trame,octets_lus,1);
	 } */
	if(repart->top) memcpy(trame,fin_trame,OPERA_TAILLE_ENTETE);
	RepartIpOctetsLus += octets_lus;
	RepartIp.traitees++; lues++;
	donnees_lues = (octets_lus - OPERA_TAILLE_ENTETE) / sizeof(TypeDonnee); /* 2 octets par voie */
	repart->donnees_recues += donnees_lues;
	repart->top += donnees_lues;
	repart->s.ip.lu_entre_D3 += donnees_lues;
	if(repart->s.ip.numAVenir == 0) /* 0 <=> chgmt timestamp <=> synchro D3.. */ {
		if(repart->nbdonnees < VOIES_PAR_4OCTETS) repart->s.ip.lu_entre_D3 = (repart->s.ip.lu_entre_D3 * repart->nbdonnees / VOIES_PAR_4OCTETS);
		if(repart->s.ip.lu_entre_D3 != repart->s.ip.dim_D3) {
			printf("%s! %s: trame %12lld/%3d, %d octets recus soit %d donnees recues entre D3 (au lieu de %d)\n",DateHeure(),
				   repart->nom,timestamp,num_trame,octets_lus,repart->s.ip.lu_entre_D3,repart->s.ip.dim_D3);
			RepartIpErreurs++;
		}
		repart->s.ip.lu_entre_D3 = 0;
	} else if(octets_lus != sizeof(TypeOperaTrame)) {
		printf("%s! %s: trame %12lld/%3d, %d octets recus (au lieu de %d)\n",DateHeure(),
			   repart->nom,timestamp,num_trame,octets_lus,sizeof(TypeOperaTrame));
		RepartIpErreurs++;
	}
	if(RepartIpRecues > RepartIp.depilees) RepartIp.depilees = RepartIpRecues;
	// if(LectEpisodeEnCours && !manquee) printf("%s/ %s  OK, %d trames lues\n",DateHeure(),repart->nom,lues);
#endif /* AVEC_OPERA */
	return(1);
}
#endif /* OPERA_SUPPORT */
#ifdef CALI_SUPPORT
#pragma mark ----------- Cali -----------
/* ========================================================================== */
char CaliNouvelle(TypeRepartiteur *repart, char *adrs, char categ) {
	//- char message[256];

	if(!repart) { OpiumFail("! Repartiteur indefini"); return(0); }
	else if(!CaliModele) { OpiumFail("! Type de repartiteur indefini: '%s'",RepartFamilleListe[FAMILLE_CALI]); return(0); }
	repart->type = CaliModele - 1;
	strcpy(repart->parm,adrs);
	repart->valide = 1;
	strcpy(repart->maintenance,".");
	repart->categ = categ;
	repart->nbident = 0;
	repart->nbentrees = 0; repart->entree = 0;
	repart->nbdonnees = 0;
	repart->famille = ModeleRep[repart->type].famille;
	repart->interf = ModeleRep[repart->type].interface;
	repart->ecr.port = 7;
	repart->p.ip.user[0] = '\0';
	repart->p.ip.pswd[0] = '\0';
	repart->i.ip.nbactn = 0;
	repart->d.ip.nbactn = 0;
	//- RepartiteurVerifieParmsEtFIFO(repart,message);
	return(1);
}
/* ========================================================================== */
static void CaliCompteursTrame(TypeRepartiteur *repart) {
	int n;
	repart->dim.totale = sizeof(TypeCaliTrame);
	repart->dim.entete = CALI_TAILLE_ENTETE;
	repart->s.ip.num_sts = 0xFFFF;
	n = repart->nbdonnees; if(n <= 0) n = 1;
	repart->s.ip.duree_trame = CALI_DONNEES_NB / n; // ((CALI_DONNEES_NB - 1) / n) + 1;
	repart->dim.trame = (repart->s.ip.duree_trame * n * sizeof(TypeDonnee)) + sizeof(TypeCaliEntete);
	/* ci-dessous, valeurs temporaires.. */
	repart->s.ip.stampD3 = repart->s.ip.duree_trame; // (int64)1000000;
	repart->s.ip.inc_stamp = repart->s.ip.duree_trame;
	repart->s.ip.tramesD3 = (int)(repart->s.ip.stampD3 / repart->s.ip.duree_trame);
	repart->s.ip.dim_D3 = repart->s.ip.tramesD3 * repart->s.ip.duree_trame * n;
	repart->s.ip.taille_derniere = (int)(repart->s.ip.stampD3 - (repart->s.ip.duree_trame * repart->s.ip.tramesD3));
	repart->gene.intervalle = (int64)(1000.0 * (float)(repart->s.ip.duree_trame) / Echantillonnage);
//	printf("(%s) Intervalle entre deux trames: %lld us\n",__func__,repart->gene.intervalle);
}
#define CALI_LISSAGE
/* ========================================================================== */
static shex CaliLissage(shex diviseur0, char revision) {
	shex lissage;
#ifdef CALI_LISSAGE
	int i,max; float div,dist,min;
#endif

	/* v1: lissage = 16; do {
		d0 = diviseur0 / lissage;
		if((diviseur0 - (d0 * lissage)) == 0) break;
		lissage /= 2;
	} while(lissage > 0); */

	lissage = 1;
#ifdef CALI_LISSAGE
	min = 65536.0;
	max = 128;
	if(revision == 1) div = (float)diviseur0 / 10.0; /* on vise 10MHz, soit 1/10 frequence fondamentale */
	else {
		div = (float)diviseur0;
		if(revision >= 3) max = 32768;
	}
	// printf("(%s) diviseur0=%d, lissage vise=%g\n",__func__,diviseur0,div);
	for(i=1; i<=max; i*=2) {
		dist = (float)i - div; if(dist < 0) dist = -dist;
		// printf("(%s) distance au lissage=%d: %g\n",__func__,i,dist);
		if(dist < min) { min = dist; lissage = i; }
	}
#endif
	// printf("(%s) lissage retenu: %d\n",__func__,lissage);

	return(lissage);
}
/* ========================================================================== */
int CaliSelectCanaux(TypeRepartiteur *repart, byte selection, float echantillonage, byte *buffer, int *t) {
	int modl; int l,n;
	shex diviseur0,diviseur1,d0,lissage; hexa cntrl;

	if(!RepartIpEcriturePrete(repart,"  ")) { *t = 0; return(0); }
	modl = repart->type;
	diviseur1 = 1; /* curieusement... */
	diviseur0 = (shex)((ModeleRep[modl].horloge * 1000.0 / (echantillonage * (float)diviseur1)) + 0.5);
	lissage = CaliLissage(diviseur0,repart->revision); if(repart->revision == 1) d0 = diviseur0 / lissage; else d0 = 1;
	if(lissage < 2) lissage = 0;
	CaliCompteursTrame(repart);
	repart->r.cali.sel = selection & 0xF;
	cntrl = (hexa)repart->r.cali.sel; if(repart->r.cali.clck) cntrl = (cntrl | 0x10);
	// l = sprintf(buffer,"w %x %x/w %x %x/w %x %x",CALI_REG_D0,diviseur0,CALI_REG_MOYENNE,lissage,CALI_REG_SEL,cntrl);
	// 0x40 = FrameId reset to 1 / 0x20 = firmware reset
	if(repart->revision == 1) {
		l = sprintf((char *)buffer,"w %x %x/w %x %x/w %x %x/w %x %x/w %x %x",
			CALI_REG_SEL,0x20,CALI_REG_D0,d0,CALI_REG_MOYENNE,lissage,CALI_REG_SEL,cntrl,CALI_REG_SEL,0xC0);
		n = RepartIpEcrit(repart,buffer,l+1); SambaMicrosec(ModeleRep[modl].delai_msg);
	} else {
	#ifdef RP_RESET_MANUEL
		int k; k = 0;
		cntrl = (cntrl | 0x20); // selection, master/slave, soft reset
		l = sprintf((char *)buffer+k,"w %x %x/w %x %x/w %x %x",
			CALI_REG_SEL,cntrl,CALI_REG_D0,d0,CALI_REG_MOYENNE,lissage);
		RepartIpEnvoie(repart,buffer+k,l,1,1,-1); k += l; buffer[k++] = '/'; buffer[k++] = '/';

		cntrl = (cntrl & 0x1F); // selection, master/slave, fin soft reset
		l = sprintf((char *)buffer+k,"w %x %x",CALI_REG_SEL,cntrl);
		RepartIpEnvoie(repart,buffer+k,l,1,1,-1); k += l; buffer[k++] = '/'; buffer[k++] = '/';

		cntrl = (cntrl | 0xC0); // selection, master/slave, frameId reset, TS reset
		l = sprintf((char *)buffer+k,"w %x %x",CALI_REG_SEL,cntrl);
		RepartIpEnvoie(repart,buffer+k,l,1,1,-1); k += l; buffer[k++] = '/'; buffer[k++] = '/';
		cntrl = (cntrl & 0x1F); // selection, master/slave, fin frameId reset et TS reset
		l = sprintf((char *)buffer+k,"w %x %x",CALI_REG_SEL,cntrl);
		RepartIpEnvoie(repart,buffer+k,l,1,0,-1);
		n = k + l;
	#else /* !RP_RESET_MANUEL */
		struct timeval heure_depart; struct timezone tz;
		gettimeofday(&heure_depart,0); tz.tz_minuteswest = 0; tz.tz_dsttime = 0; // tz pas pris en compte
		heure_depart.tv_sec += 3600; // special RedPitaya
		l = sprintf((char *)buffer,"k %ld %d %d %d/w %x %x/w %x %x/w %x %x/w %x %x/w %x %x",
			heure_depart.tv_sec,heure_depart.tv_usec,tz.tz_minuteswest,tz.tz_dsttime,
			CALI_REG_SEL,0x20,CALI_REG_D0,d0,CALI_REG_MOYENNE,lissage,CALI_REG_SEL,cntrl,CALI_REG_SEL,0xC0);
		n = RepartIpEcrit(repart,buffer,l+1); SambaMicrosec(ModeleRep[modl].delai_msg);
	#endif /* !RP_RESET_MANUEL */
	}
	*t = strlen((char *)buffer);
	return(n);
}
/* ========================================================================== */
int CaliSelecteur(TypeRepartiteur *repart, int selection, float echantillonage, byte *buffer, int *t) {
	int modl; int l,n;
	shex diviseur0,diviseur1,d0,lissage; hexa cntrl;

	if(!RepartIpEcriturePrete(repart,"  ")) { *t = 0; return(0); }
	modl = repart->type;
	diviseur1 = 1; /* curieusement... */
	diviseur0 = (shex)((ModeleRep[modl].horloge * 1000.0 / (echantillonage * (float)diviseur1)) + 0.5);
	lissage = CaliLissage(diviseur0,repart->revision); if(repart->revision == 1) d0 = diviseur0 / lissage; else d0 = 1;
	if(lissage < 2) lissage = 0;
	CaliCompteursTrame(repart);
	repart->r.cali.sel = (byte)(selection & 0xF);
	cntrl = (hexa)repart->r.cali.sel; if(repart->r.cali.clck) cntrl = (cntrl | 0x10);
	// l = sprintf(buffer,"w %x %x/w %x %x/w %x %x",CALI_REG_D0,diviseur0,CALI_REG_MOYENNE,lissage,CALI_REG_SEL,cntrl);
	// 0x40 = FrameId reset to 1 / 0x20 = firmware reset
	l = sprintf((char *)buffer,"w %x %x/w %x %x/w %x %x/w %x %x",
		CALI_REG_SEL,0xE0,CALI_REG_D0,d0,CALI_REG_MOYENNE,lissage,CALI_REG_SEL,cntrl);
	n = RepartIpEcrit(repart,buffer,l+1); SambaMicrosec(ModeleRep[modl].delai_msg);
	*t = strlen((char *)buffer);
	return(n);
}
/* ========================================================================== */
void CaliRecopieStatus(CaliTrame trame, TypeRepartiteur *repart) {
	memcpy(&CaliEnteteLue,&(trame->hdr),CALI_TAILLE_ENTETE);
	CaliTrameLue = CALI_TRAME_NUMERO(trame);
	CaliVersionLue = trame->hdr.id & 0xFF;
	strcpy(repart->date_status,DateHeure());
	RepartiteurAccepteEntrees(repart,1);
	if(!repart->entree[0].adrs) {
		repart->entree[0].adrs = (TypeAdrs)(&(NumeriseurAnonyme[0]));
		NumeriseurNeuf(&(NumeriseurAnonyme[0]),NumeriseurTypeCALI,0,repart);
		NumeriseurAnonyme[0].entree = 0;
		NumeriseurAnonyme[0].avec_status = 0;
		NumeriseurAnonyme[0].vsnid = 1;
		NumeriseurAnonyme[0].repart = repart;
		NumeriseurAnonyme[0].bb = -1;
	}
	CaliCompteursTrame(repart);
	repart->status_relu = 1;
}
/* ========================================================================== */
INLINE static void CaliChangeMode(TypeRepartiteur *repart, char mode, char *prefixe) {
	char texte[80]; int l;

	l = sprintf(texte,"w %x %x",CALI_REG_MODE,CaliModeCode[mode]);
	RepartIpEcrit(repart,texte,l+1); SambaMicrosec(ModeleRep[repart->type].delai_msg);
	if(prefixe) printf("%s. Ecrit: '%s'\n",prefixe,texte);
}
/* ========================================================================== */
INLINE static void CaliEcritRessourceBB(TypeRepartiteur *repart, shex adrs, hexa ss_adrs, TypeADU valeur) {
	int l;

	l = sprintf(RepartiteurValeurEnvoyee,"w %x %x",CALI_REG_SORTIE,valeur);
	RepartIpEcrit(repart,RepartiteurValeurEnvoyee,l+1); SambaMicrosec(ModeleRep[repart->type].delai_msg);
}
/* ========================================================================== */
void CaliSwappeTrameV1(TypeRepartiteur *repart, void *buffer, int nb) {
	if(repart->revision == 1) {
//		printf("Niveau 1: inversion!\n");
		ByteSwappeLong((int64 *)buffer);
		ByteSwappeInt((hexa *)(buffer + 8));
		ByteSwappeIntArray((hexa *)(buffer + 16),(nb - 16) / sizeof(hexa));
	} else {
		hexa id;
//		printf("Niveau 2: inversion!\n");
		id = *(hexa *)(buffer + 8); ByteSwappeInt(&id);
		*(hexa *)(buffer + 8) = *(hexa *)(buffer + 12);
		*(hexa *)(buffer + 12) = id;
		//? FourShortsSwappeLongArray((int64 *)(buffer + 16),(nb - 16) / sizeof(int64));
		// TwoShortsSwappeIntArray((hexa *)(buffer + 16),(nb - 16) / sizeof(hexa));
		{	int i; int64 *adrs; FourShortsSwappable tempo;
			adrs = (int64 *)(buffer + 16);
			for(i=0; i<nb; i++) {
				tempo.court.s2 = ((FourShortsSwappable *)adrs)->court.s0;
				tempo.court.s3 = ((FourShortsSwappable *)adrs)->court.s1;
				tempo.court.s0 = ((FourShortsSwappable *)adrs)->court.s2;
				tempo.court.s1 = ((FourShortsSwappable *)adrs)->court.s3;
				*adrs++ = tempo.mot64;
			}
		}
	}
}
/* ========================================================================== */
void CaliSwappeTrame(TypeRepartiteur *repart, CaliTrame trame, ssize_t nb) {
	int i;

	if(repart->revision == 1) {
		ByteSwappeLong(&(trame->hdr.stamp));
		ByteSwappeInt((hexa *)&(trame->hdr.id));
		ByteSwappeIntArray((hexa *)&(trame->valeur),(nb - CALI_TAILLE_ENTETE) / sizeof(hexa));
	} else if(repart->revision == 4) {
		ByteSwappeInt(&(trame->hdr.id));
		ByteSwappeInt((unsigned int *)&(trame->hdr.status));
	} else {
		hexa id; int *adrs32;
		adrs32 = (int *)(trame->hdr.status);    // adrs32 pointe en vrai sur l'Id
		id = trame->hdr.id; ByteSwappeInt(&id); // Id pointe en vrai sur le status, qui est byteswappe
		trame->hdr.id = *adrs32;                // hdr.id recupere l'Id, qui n'est pas byteswappe ici
		*adrs32 = id;                           // hdr.status recupere le status byteswappe
		{	int64 *adrs64; int n64; FourShortsSwappable tempo;
			adrs64 = (int64 *)&(trame->valeur);
			n64 = nb / sizeof(int64);
			for(i=0; i<n64; i++) {
				tempo.court.s2 = ((FourShortsSwappable *)adrs64)->court.s0;
				tempo.court.s3 = ((FourShortsSwappable *)adrs64)->court.s1;
				tempo.court.s0 = ((FourShortsSwappable *)adrs64)->court.s2;
				tempo.court.s1 = ((FourShortsSwappable *)adrs64)->court.s3;
				*adrs64++ = tempo.mot64;
			}
		}
	}
}
/* ========================================================================== */
void CaliSwappePrint(TypeRepartiteur *repart, byte *buffer, ssize_t nb) {
	if(repart->revision == 1) CaliSwappeTrame(repart,(CaliTrame)buffer,nb);
	else if(repart->revision == 4) {
		CaliSwappeTrame(repart,(CaliTrame)buffer,nb);
		ByteSwappeLong((int64 *)buffer);
		ByteSwappeInt((hexa *)(buffer + 12));
		ByteSwappeShortArray((shex *)(buffer + 16),(nb - 16) / sizeof(shex));
		{
			int i,nb16; TypeDonnee val,*adrs,*next;
			nb16 = nb / sizeof(TypeDonnee);
			adrs = ((CaliTrame)buffer)->valeur;
			for(i=0; i<nb16; i++) {
				val = *adrs; next = adrs + 1;
				*adrs = *next; *next = val;
				adrs = next + 1; i++;
			}
		}
	} else {
		ByteSwappeLong((int64 *)buffer);
		ByteSwappeInt((hexa *)(buffer + 8));
		ByteSwappeIntArray((hexa *)(buffer + 16),(nb - 16) / sizeof(hexa));
	}
}
/* ========================================================================== */
void CaliDump(TypeRepartiteur *repart, byte *buffer, ssize_t nb, char en_hexa) {
	hexa num; int64 stamp;
	int i,j,k; short num_bloc; TypeADU *court; char deja_vu;

	if(nb == -1) {
		if(errno == EAGAIN) printf("===== Lecture: tampon vide\n");
		else printf("===== Lecture en erreur (%s)\n",strerror(errno));
	} else {
		num = CALI_TRAME_NUMERO(buffer); // ByteSwappeInt(&num);
		stamp = CALI_TRAME_TIMESTAMP(buffer);
		printf("===== Lecture de %d octets, trame No %d [id: %08X] (timestamp %lld [%08llX]):\n",nb,num,((CaliTrame)buffer)->hdr.id,stamp,stamp);
		// for(i=0; i<4; i++) printf(" [%d]=%02X",i,((CaliTrame)buffer)->hdr.status[i]); printf("\n");
		printf("\nStatus "); for(i=0; i<4; i++) printf(" Canal %d: %02X%13s",i+1,((CaliTrame)buffer)->hdr.status[i]," "); printf("\n");
		for(j=8; j>0; ) {
			--j;
			for(i=0; i<4; i++) { k = 1 << j; if((((CaliTrame)buffer)->hdr.status[i] & k)) break; }
			if(i < 4) {
				printf("bit %d: ",j);
				for(i=0; i<4; i++) {
					k = 1 << j; printf(" %-24s",((((CaliTrame)buffer)->hdr.status[i] & k))? CaliStatusBitTexte[j]: "--");
				}
				printf("\n");
			}
		}
		printf("\n"); CaliDumpTrameStrm(repart,(CaliTrame)buffer,nb,repart->acces_remplis,"");
		if(en_hexa) {
			if(!BigEndianOrder) CaliSwappePrint(repart,buffer,nb);
			court = 0; // GCC
		} else {
			if(!BigEndianOrder) {
				FourShortsSwappeLong((int64 *)buffer);
				TwoShortsSwappeInt((unsigned int *)(buffer + 8));
				/* if(repart->revision == 1) */ TwoShortsSwappeIntArray((unsigned int *)(buffer + 16),(nb - 16) / sizeof(unsigned int));
			}
			court = (TypeADU *)buffer;
		}
		num_bloc = 0;
		deja_vu = 0;
		for(i=0; i<nb; i++) {
			if(!(i % 16)) {
				if(!deja_vu) printf("\n%4d: ",i);
				if(i) {
					if(!memcmp(buffer+i,buffer+i-16,16)) {
						if(!deja_vu) printf(" ...");
						deja_vu = 1;
						i += 15;
					} else {
						if(deja_vu) printf("\n%4d: ",i);
						deja_vu = 0;
					}
				}
				num_bloc++;
			} else { if(!(i % 2)) printf(" "); if(!(i % 4)) printf(" "); }
			if(!deja_vu) {
				if(en_hexa) printf("%02X",buffer[i]); else {
					j = i / 2;
					// if(BigEndianOrder) { if((j % 2)) printf(" %6d",court[j-1]); else printf(" %6d",court[j+1]); } else if(court)
					if(num_bloc == 1) printf(" %04X",court[j]);
					else printf(" %6d",(TypeDonnee)((int)court[j] - (int)0x8000));
					i++;
				}
			}
		}
		printf("\n");
		if(en_hexa) {
			if(!BigEndianOrder) CaliSwappePrint(repart,buffer,nb);
		} else {
			if(!BigEndianOrder) {
				FourShortsSwappeLong((int64 *)buffer);
				TwoShortsSwappeInt((unsigned int *)(buffer + 8));
				if(repart->revision == 1) TwoShortsSwappeIntArray((unsigned int *)(buffer + 16),(nb - 16) / sizeof(unsigned int));
			}
		}
	}
}
// #define MODE_CALI_STATUS
/* ========================================================================== */
void CaliDumpTrameStrm(TypeRepartiteur *repart, CaliTrame trame_cali, size_t taille_trame, int64 num, char *prefixe) {
	int nb,imprime; // hexa *ts_haut,*ts_bas;

	nb = (int)taille_trame;
//	ts_bas = (hexa *)trame_cali; ts_haut = (hexa *)((char *)trame_cali + 4);
	printf("%sTrame[%X] (%d octets) #%lld recue par ethernet:\n",prefixe,nb,nb,num);
	printf("%s| Stamp 64   : %016llX (%lld)\n",prefixe,trame_cali->hdr.stamp,trame_cali->hdr.stamp);
//	printf("%s| Stamp 2x32 : %08X%08X (%lld)\n",prefixe,*ts_haut,*ts_bas,((hex64)*ts_haut << 32) | (hex64)*ts_bas);
	printf("%s| Id         : %08X (trame #%ld)\n",prefixe,trame_cali->hdr.id,(trame_cali->hdr.id >> 8) & 0xFFFFFF);
	printf("%s| Status     : %02X %02X %02X %02X\n",prefixe,
		trame_cali->hdr.status[0],trame_cali->hdr.status[1],trame_cali->hdr.status[2],trame_cali->hdr.status[3]); // 3..0 si little endian
#ifdef MODE_CALI_STATUS
	{
		int i,j,k;
		printf("%s| . Status  ",prefixe); for(i=0; i<4; i++) printf("   ADC %d: %02X%13s",i+1,status[3-i]," "); printf("\n");
		for(j=8; j>0; ) {
			--j;
			for(i=0; i<4; i++) { k = 1 << j; if((status[3-i] & k)) break; }
			if(i < 4) {
				printf("%s| .  bit %d: ",prefixe,j);
				for(i=0; i<4; i++) {
					k = 1 << j; printf(" %-24s",((status[3-i] & k))? CaliStatusBitTexte[j]: "--");
				}
				printf("\n");
			}
		}
	}
#endif /* MODE_CALI */
#define PAR_OCTETS
	imprime = 16; // nb;
#if defined(PAR_OCTETS)
	#define hex hex8
	#define fmt "%02X"
#elif defined(PAR_DATA)
	#define hex hex16
	#define fmt "%04hX"
#elif defined(PAR_HEX32)
	#define hex hexa
	#define fmt "%08X"
#else // PAR_HEX64
	#define hex hex64
	#define fmt "%016llX"
#endif
	hex *c = (hex *)trame_cali; int k,l,m,n;
	printf("%s| Vue %2d bits:",prefixe,sizeof(hex) * 8); if(imprime > 16) printf("\n%s|  000:",prefixe);
	m = 16 / sizeof(hex); n = imprime; k = 0;
	while(n) {
		l = m; while(l) { if(!(l % 2)) { printf(" "); if(!(l % 4)) printf(" "); } if(sizeof(hex) > 1) printf(" "); printf(fmt,*c++); --l; }
		n -= 16; if(n) printf("\n%s|  %03X:",prefixe,imprime - n); else printf("\n");
	}
#undef hex
#undef fmt
}
/* ========================================================================== */
void CaliDumpTrameEvt(CaliTrame trame_cali, size_t taille_trame) {
	TypeEventFrame *trame_evt; TypeEventDefinition *info; TypeEventChannel *canal; TypeEventDataRef *ref;
	int voie,n,e;

	n = (int)taille_trame;
	printf(". Trame recue (%d octets):\n",n);
	printf("  | Stamp      : %016llX (%lld)\n",trame_cali->hdr.stamp,trame_cali->hdr.stamp);
	printf("  | Id         : %08X (%d)\n",trame_cali->hdr.id,trame_cali->hdr.id);
	printf("  | Status     : %02X %02X %02X %02X\n",
		   trame_cali->hdr.status[0],trame_cali->hdr.status[1],
		   trame_cali->hdr.status[2],trame_cali->hdr.status[3]);

	trame_evt = (TypeEventFrame *)(&(trame_cali->valeur));
	e = ((trame_evt->type.etiquette >= 0) && (trame_evt->type.etiquette < 4))? trame_evt->type.etiquette: 0;
	printf("  | Etiquette  : %04X (%s)\n",trame_evt->type.etiquette,Etiquette[e]);
	printf("  | +bourrage  :      %04X %04X %04X\n",trame_evt->type.nul[0],trame_evt->type.nul[1],trame_evt->type.nul[2]);

	if(trame_evt->type.etiquette == REPART_EVENT_DEF) {
		info = &(trame_evt->is.hdr.def);
		printf("  | Stamp      : %016llX (%lld)\n",info->stamp,info->stamp);
		printf("  | Num evt    : %08X (%d)\n",info->num,info->num);
		printf("  | Entree     : %02X (numeriseur #%hd)\n",info->bb,info->bb);
		printf("  | ADC        : %02X (trigger sur voie #%hd)\n",info->trig,info->trig);
		printf("  | Niv.       : %04X (%hd ADU)\n",info->niveau,info->niveau);
		printf("  | Dim        : %04X (%hd voie[s])\n",info->dim,info->dim);
		printf("  | Vsn        : %02X (%d)\n",info->version,info->version);
		printf("  | Bourrage   : %02X\n",info->vide);
		printf("  | Temps mort : %08X (%d)\n",info->temps_mort,info->temps_mort);
		canal = trame_evt->is.hdr.trace.canal;
		for(voie=0; voie<info->dim; voie++) {
			printf("  | Canal %d    :\n",voie+1);
			printf("  | . stamp    : %016llX (%lld)\n",canal->stamp,canal->stamp);
			printf("  | . dim      : %04X (%hd)\n",canal->dim,canal->dim);
			printf("  | . entree   : %04X (%hd)\n",canal->entree,canal->entree);
			printf("  | . ADC      : %04X (%hd)\n",canal->adc,canal->adc);
			printf("  | . bourrage : %04X (%hd)\n",canal->nul,canal->nul);
			canal++;
		}
	} if(trame_evt->type.etiquette == REPART_EVENT_DATA) {
		ref = &(trame_evt->is.donnees.ref);
		printf("  | Num evt    : %08X (%d)\n",ref->evt,ref->evt);
		printf("  | Tranche    : %04X (tranche #%hd)\n",ref->tranche,ref->tranche);
		printf("  | Dim        : %04X (%hd valeurs)\n",ref->nb,ref->nb);
	}
	{
		byte *c; shex *s; int k; size_t hdr_lngr;
		hdr_lngr = CALI_TAILLE_ENTETE + sizeof(TypeEventTypeTrame) + sizeof(TypeEventDataRef);
		if(trame_evt->type.etiquette == REPART_EVENT_DATA) { n = hdr_lngr; printf("  | Entete    :\n  |  000: "); }
		else printf("  | Contenu   :\n  |  000: ");
		c = (byte *)trame_cali; k = 0;
		while(n--) {
			if(k < hdr_lngr) printf("%02X",*c++); k++;
			if(!(k%2)) printf(" "); if(!(k%4)) printf(" "); if(!(k%16)) { if(n) printf("\n  |  %03X: ",k); else printf("\n"); }
		}
		if((k%16)) printf("\n");
		if(trame_evt->type.etiquette == REPART_EVENT_DATA) {
			n = ((int)taille_trame - hdr_lngr) / sizeof(TypeADU);
			s = (shex *)((char *)trame_cali + hdr_lngr); k = 0;
			printf("  | Donnees   :\n  |  000: ");
			while(n--) {
				printf("%7d",*s++); k++;
				if(!(k%2)) printf(" "); if(!(k%4)) printf(" "); if(!(k%8)) { if(n) printf("\n  |  %03d: ",k); else printf("\n"); }
			}
			if((k%8)) printf("\n");
		}
	}
}
/* ========================================================================== */
INLINE char CaliVidePile(TypeRepartiteur *repart, char *pas_tous_secs, short *pleins, char log) {
	hexa lngr;
	ssize_t octets_lus;
	int donnees_lues,num_trame,haut_pile,sautees,stamp_absents,lues,nb,nbD3;
	byte *pile,fin_trame[CALI_TAILLE_ENTETE];
	CaliTrame trame; void *adrs;
	char synchroD3; // trame_inattendue;
	int64 timestamp; // stamp_attendu;
	int64 t2;

	if(repart->top < 0) { LectDeclencheErreur(_ORIGINE_,42,LECT_INCOHER); return(0); }
	if(repart->s.ip.manquant) return(0); // generer les donnees manquantes avant d'en rajouter: test sur (top - bottom) prioritaire
	haut_pile = repart->top * sizeof(TypeDonnee);
	if((haut_pile + sizeof(TypeCaliTrame)) > repart->octets) { *pleins += 1; return(0); }
	pile = (byte *)repart->fifo32; /* data16 est decalee de CALI_TAILLE_ENTETE octets, donc partir de fifo32 descend de CALI_TAILLE_ENTETE octets */
	if(repart->top) memcpy(fin_trame,pile+haut_pile,CALI_TAILLE_ENTETE);
	trame = (CaliTrame)(pile + haut_pile);
	lngr = sizeof(TypeSocket);
	RepartIpEssais++;
	RepartIp.acces++;
	repart->acces_demandes++;
	lues = 0;
	t2 = VerifTempsPasse? (DateMicroSecs() - LectT0Run): 0;
	octets_lus = RepartIpLitTrame(repart,trame,sizeof(TypeCaliTrame));
#ifdef CALI_REDEMANDE
	if(repart->s.ip.stamp_redemande >= 0) t3 = VerifTempsPasse? (DateMicroSecs() - LectT0Run): 0;
#endif
	if(octets_lus == -1) {
		if(errno == EAGAIN) {
			if(log) printf("          %s: vide\n",repart->nom);
			RepartIp.vide++;
			repart->acces_vides++;
			repart->serie_vide++;
			repart->dernier_vide = t2;
		#ifdef CALI_REDEMANDE
			if(repart->s.ip.stamp_redemande >= 0) { repart->s.ip.duree_vide += (t3 - t2); repart->s.ip.nb_vide++; }
		#endif
			return(0);
		} else { LectDeclencheErreur(_ORIGINE_,43,LECT_EMPTY); return(0); }
	} else if((octets_lus <= 0) || (octets_lus > sizeof(TypeCaliTrame))) {
		LectDeclencheErreur(_ORIGINE_,44,LECT_UNKN); return(0);
	} else if(((TypeSocketIP *)(&(repart->lec.ip.skt)))->sin_addr.s_addr != repart->adrs.val) {
		LectDeclencheErreur(_ORIGINE_,45,LECT_UNEXPEC); return(0);
	}
	if(log) {
		donnees_lues = (octets_lus - CALI_TAILLE_ENTETE) / sizeof(shex); /* 2 octets par voie */
		printf("          %s: %d valeur%s\n",repart->nom,Accord1s(donnees_lues));
	}
	repart->acces_remplis++;
	if(repart->arrete) printf("%s! %s envoie a nouveau des donnees\n",DateHeure(),repart->nom);
	repart->arrete = 0;
	repart->serie_vide = 0;
	repart->dernier_rempli = t2;
	*pas_tous_secs = 1;
	RepartIpRecues++;
	//a gettimeofday(&LectDateRun,0);
	//a dernier_temps = (float)(LectDateRun.tv_sec % 60) + ((float)LectDateRun.tv_usec / 1000000.0);
	adrs = (void *)trame; num_trame = -1;
	/* CaliDump(repart,(byte *)trame,octets_lus,1); */
	// CaliDumpTrameEvt(trame,octets_lus);
	if(!BigEndianOrder) {
		if(repart->revision > 1) {
			hexa id;
			if(repart->revision < 4) id = *(hexa *)(trame->hdr.status);
			else { id = trame->hdr.id; ByteSwappeInt(&id); }
			num_trame = (id >> 8) & 0xFFFFFF;
			if(num_trame != REPART_EVENT) CaliSwappeTrame(repart,trame,octets_lus);
		} else CaliSwappeTrame(repart,trame,octets_lus);
	}
//	if(!BigEndianOrder && (repart->acces_remplis < 5)) printf("(%s:%d) %08X %08X %08X %08X\n",__func__,__LINE__,
//													   *(int *)(adrs),*(int *)(adrs+4),*(int *)(adrs+8),*(int *)(adrs+12));
	if(num_trame != REPART_EVENT) num_trame = CALI_TRAME_NUMERO(trame);
	if(num_trame == REPART_EVENT) {
		TypeEventFrame *trame_evt; TypeEventDefinition *info; TypeEventChannel *canal; TypeNumeriseur *numeriseur;
		int voie,c;

		RepartIpOctetsLus += octets_lus;
		RepartIp.traitees++;
		donnees_lues = (octets_lus - CALI_TAILLE_ENTETE) / sizeof(TypeDonnee); /* 2 octets par voie */
		repart->donnees_recues += donnees_lues;
//-		repart->top += donnees_lues;
		trame_evt = (TypeEventFrame *)(&(trame->valeur));
		if(trame_evt->type.etiquette == REPART_EVENT_DEF) {
			info = &(trame_evt->is.hdr.def);
			if(Acquis[AcquisLocale].etat.evt_recus == 0) Acquis[AcquisLocale].etat.evt_recus = (info->num - 1);
			if(info->num != (Acquis[AcquisLocale].etat.evt_recus + 1)) {
				printf("%s/ ! Evenement #%d attendu, recu: %d\n",DateHeure(),Acquis[AcquisLocale].etat.evt_recus,info->num);
			} else if(VerifCalculEvts) printf("%s/ Evenement recu: #%d\n",DateHeure(),info->num);
			voie = -1;
			if((info->bb >= 0) && (info->bb < repart->nbentrees))
				numeriseur = (TypeNumeriseur *)(repart->entree[info->bb].adrs);
			else numeriseur = 0;
			if(numeriseur) {
				if((info->trig >= 0) && (info->trig < numeriseur->nbadc)) voie = numeriseur->voie[info->trig];
			}
			RepartEvtEmpile = TrmtCandidatSignale(1,voie,info->stamp,info->niveau,TRGR_EXTERNE);
			if(RepartEvtEmpile < 0) printf("%s/ Evenement #%d degage\n",DateHeure(),info->num);
			TrigEnCours.canaux  = info->dim;
			TrigEnCours.canal   = 0;
			TrigEnCours.tranche = 0;
			TrigEnCours.tot     = 0; /* nombre de valeurs deja lues */
			for(c=0; c<info->dim; c++) {
				canal = &(trame_evt->is.hdr.trace.canal[c]);
				voie = -1;
				if((canal->entree >= 0) && (canal->entree < repart->nbentrees))
					numeriseur = (TypeNumeriseur *)(repart->entree[canal->entree].adrs);
				else numeriseur = 0;
				if(numeriseur) {
					if((canal->adc >= 0) && (canal->adc < numeriseur->nbadc)) voie = numeriseur->voie[canal->adc];
				}
				if((nb = EvtStocke[RepartEvtEmpile].nbvoies) < REPART_VOIES_MAX) {
					EvtStocke[RepartEvtEmpile].voie[nb].indx = voie;
					EvtStocke[RepartEvtEmpile].voie[nb].lngr = canal->dim;
					EvtStocke[RepartEvtEmpile].voie[nb].lus  = 0;
					if(voie < 0) EvtStocke[RepartEvtEmpile].voie[nb].adrs = 0;
					else if(VoieTampon[voie].brutes->type == TAMPON_INT16)
						EvtStocke[RepartEvtEmpile].voie[nb].adrs = VoieTampon[voie].prochain_s16 - VoieTampon[voie].brutes->t.sval;
					else if(VoieTampon[voie].brutes->type == TAMPON_INT32)
						EvtStocke[RepartEvtEmpile].voie[nb].adrs = VoieTampon[voie].prochain_i32 - VoieTampon[voie].brutes->t.ival;
					EvtStocke[RepartEvtEmpile].nbvoies++;
					if(VerifCalculEvts) printf("          L'evt #%d sur la voie %s commence a %d\n",info->num,VoieManip[voie].nom,EvtStocke[RepartEvtEmpile].voie[nb].adrs);
				}
				canal++;
			}
		} else if(trame_evt->type.etiquette == REPART_EVENT_DATA) {
			TypeEventData *donnees;
			TypeDonnee *sval,*prochain_s16,d16;
			TypeLarge  *ival,*prochain_i32,d32;
			// TypeSignal *rval,*prochain_r32,donnee_r32;
			int nb; short voie;
			donnees = (TypeEventData *)(&(trame_evt->is.donnees));
			if(donnees->ref.tranche == TrigEnCours.tranche) {
				if(VerifCalculEvts) printf("%s/ Tranche #%d recue: %d valeur%s\n",DateHeure(),donnees->ref.tranche,Accord1s(donnees->ref.nb));
				TrigEnCours.tranche += 1;
			} else {
				if(VerifCalculEvts) printf("%s/ Tranche #%d attendue, recue: %d\n",DateHeure(),TrigEnCours.tranche,donnees->ref.tranche);
				TrigEnCours.tranche = donnees->ref.tranche + 1;
			}
			if(TrigEnCours.canal >= TrigEnCours.canaux) {
				printf("%s/ Donnees en excedent (canal #%d/%d non prevu)\n",DateHeure(),TrigEnCours.canal,TrigEnCours.canaux);
				nb = donnees->ref.nb;
			} else {
				voie = EvtStocke[RepartEvtEmpile].voie[TrigEnCours.canal].indx;
				if(VoieTampon[voie].brutes->type == TAMPON_INT16) sval = donnees->val.s;
				else if(VoieTampon[voie].brutes->type == TAMPON_INT32) ival = donnees->val.i;
				nb = 0;
			}
			while(nb < donnees->ref.nb) {
//				if(BigEndianOrder) val_lue = val[nb++]; else if(nb & 1) val_lue = val[nb++ - 1]; else val_lue = val[++nb];
				if((voie >= 0) && VoieTampon[voie].brutes && VoieTampon[voie].brutes->max) {
				#ifndef CORRECTION_VALEUR
					if(VoieTampon[voie].brutes->type == TAMPON_INT16) d16 = sval[nb++];
					else if(VoieTampon[voie].brutes->type == TAMPON_INT32) d32 = ival[nb++];
				#else
					int32 val_lue;
					if(VoieTampon[voie].brutes->type == TAMPON_INT16) {
						val_lue = sval[nb++];
						if(repart->famille == FAMILLE_CALI) {
							d16 = (TypeDonnee)((val_lue & VoieManip[voie].ADCmask) - VoieManip[voie].zero);
						} else if((repart->famille == FAMILLE_OPERA) && (repart->r.opera.mode == code_acqui_EDW_BB2)) {
							d16 = (LectEnCours.valeur & 0x8000)? LectEnCours.valeur & 0x7FFF: LectEnCours.valeur | 0x8000;
						} else {
							if(VoieManip[voie].ADCmask == 0xFFFF) d16 = (TypeDonnee)val_lue;
							else {
								d16 = (val_lue & VoieManip[voie].ADCmask); // DATAVALEUR(voie,val_lue)
								if(VoieManip[voie].signe) {
								#ifdef DONNEES_NON_SIGNEES
									if(d16 < VoieManip[voie].zero) donnee += VoieManip[voie].zero;
									else d16 -= VoieManip[voie].zero;
								#else
								#ifdef SIGNE_ARITHMETIQUE
									if(d16 >= VoieManip[voie].zero) donnee -= (2 * VoieManip[voie].zero);
								#else  /* SIGNE_ARITHMETIQUE */
									if(d16 & VoieManip[voie].zero) donnee |= VoieManip[voie].extens;
								#endif /* SIGNE_ARITHMETIQUE */
								#endif
								}
							}
						}
					} else if(VoieTampon[voie].brutes->type == TAMPON_INT32) {
						val_lue = ival[nb++];
						if(repart->famille == FAMILLE_CALI) {
							d32 = (val_lue & VoieManip[voie].ADCmask) - VoieManip[voie].zero;
						} else if((repart->famille == FAMILLE_OPERA) && (repart->r.opera.mode == code_acqui_EDW_BB2)) {
							d32 = (LectEnCours.valeur & VoieManip[voie].zero)? LectEnCours.valeur & 0x7FFF: LectEnCours.valeur | 0x8000;
						} else {
							if(VoieManip[voie].ADCmask == 0xFFFFFFF) d32 = val_lue;
							else {
								d32 = (val_lue & VoieManip[voie].ADCmask); // DATAVALEUR(voie,val_lue)
								if(VoieManip[voie].signe) {
								#ifdef DONNEES_NON_SIGNEES
									if(d32 < VoieManip[voie].zero) donnee += VoieManip[voie].zero;
									else d32 -= VoieManip[voie].zero;
								#else
								#ifdef SIGNE_ARITHMETIQUE
									if(d32 >= VoieManip[voie].zero) donnee -= (2 * VoieManip[voie].zero);
								#else  /* SIGNE_ARITHMETIQUE */
									if(d32 & VoieManip[voie].zero) donnee |= VoieManip[voie].extens;
								#endif /* SIGNE_ARITHMETIQUE */
								#endif
								}
							}
						}
					}
				#endif /* CORRECTION_VALEUR */
					if(VoieTampon[voie].brutes->type == TAMPON_INT16) {
						VoieTampon[voie].derniere_s16 = VoieTampon[voie].courante_s16 = d16; /* courante_s16 pour la reutilisation de voies (pseudos) */
						*(VoieTampon[voie].prochain_s16) = d16;
						prochain_s16 = VoieTampon[voie].prochain_s16;
						if(((VoieTampon[voie].brutes->prem + 1) == VoieTampon[voie].brutes->max) || ((VoieTampon[voie].brutes->nb + 1) == VoieTampon[voie].brutes->max))
							prochain_s16 = VoieTampon[voie].brutes->t.sval;
						else prochain_s16++;
					} else if(VoieTampon[voie].brutes->type == TAMPON_INT32) {
						VoieTampon[voie].derniere_i32 = VoieTampon[voie].courante_i32 = d32; /* courante_i32 pour la reutilisation de voies (pseudos) */
						*(VoieTampon[voie].prochain_i32) = d32;
						prochain_i32 = VoieTampon[voie].prochain_i32;
						if(((VoieTampon[voie].brutes->prem + 1) == VoieTampon[voie].brutes->max) || ((VoieTampon[voie].brutes->nb + 1) == VoieTampon[voie].brutes->max))
							prochain_i32 = VoieTampon[voie].brutes->t.ival;
						else prochain_i32++;
					}
					LectStampsLus++;
					VoieTampon[voie].lus++;
					if(VoieTampon[voie].brutes->nb < VoieTampon[voie].brutes->max) {
						VoieTampon[voie].brutes->nb++;
						if(VoieTampon[voie].brutes->nb >= VoieTampon[voie].brutes->max) {
							VoieTampon[voie].circulaire = 1;
							if(VoieTampon[voie].brutes->type == TAMPON_INT16)
								VoieTampon[voie].prochain_s16 = VoieTampon[voie].brutes->t.sval + VoieTampon[voie].brutes->prem;
							else if(VoieTampon[voie].brutes->type == TAMPON_INT32)
								VoieTampon[voie].prochain_i32 = VoieTampon[voie].brutes->t.ival + VoieTampon[voie].brutes->prem;
						} else {
							if(VoieTampon[voie].brutes->type == TAMPON_INT16) VoieTampon[voie].prochain_s16 += 1;
							else if(VoieTampon[voie].brutes->type == TAMPON_INT32) VoieTampon[voie].prochain_i32 += 1;
						}
					} else {
						/* if(SettingsModeDonnees == 0) { LectDeclencheErreur(_ORIGINE_,51,LECT_FULL); goto retour; } */
						VoieTampon[voie].circulaire = 1;
						VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien++;
						VoieTampon[voie].brutes->prem++;
						if(VoieTampon[voie].brutes->prem >= VoieTampon[voie].brutes->max) {
							VoieTampon[voie].brutes->prem = 0;
							VoieTampon[voie].trmt[TRMT_AU_VOL].point0 = VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien;
						}
						if(VoieTampon[voie].brutes->type == TAMPON_INT16)
							VoieTampon[voie].prochain_s16 = VoieTampon[voie].brutes->t.sval + VoieTampon[voie].brutes->prem;
						if(VoieTampon[voie].brutes->type == TAMPON_INT32)
							VoieTampon[voie].prochain_i32 = VoieTampon[voie].brutes->t.ival + VoieTampon[voie].brutes->prem;
					}
					if((VoieTampon[voie].brutes->type == TAMPON_INT16) && (prochain_s16 != VoieTampon[voie].prochain_s16)) {
						printf("%s/ !! Ecriture dans le buffer de %s a la mauvaise place [%lld]\n",VoieManip[voie].nom,++LectSyncRead);
					} else if((VoieTampon[voie].brutes->type == TAMPON_INT32) && (prochain_i32 != VoieTampon[voie].prochain_i32)) {
						printf("%s/ !! Ecriture dans le buffer de %s a la mauvaise place [%lld]\n",VoieManip[voie].nom,++LectSyncRead);
					}
				}
				if(++(EvtStocke[RepartEvtEmpile].voie[TrigEnCours.canal].lus) >= EvtStocke[RepartEvtEmpile].voie[TrigEnCours.canal].lngr) {
					TrigEnCours.canal++;
					voie = EvtStocke[RepartEvtEmpile].voie[TrigEnCours.canal].indx;
				}
			}
		} else if(trame_evt->type.etiquette == REPART_EVENT_CLOSED) {
			if(VerifCalculEvts) printf("%s/ Evenement a traiter: #%d\n",DateHeure(),EvtASauver);
			TrmtConstruitEvt();

		}

		/* mode stream */
	} else {
		//+ if(repart->revision == 1) repart->s.ip.stampDernier = CALI_TRAME_TIMESTAMP(trame); else repart->s.ip.stampDernier = repart->s.ip.stampEnCours;
		repart->s.ip.stampDernier = CALI_TRAME_TIMESTAMP(trame);
		nbD3 = (int)(repart->s.ip.stampDernier / repart->s.ip.inc_stamp);
		synchroD3 = 0;
		//a revoir: synchroD3 = (Modulo(repart->s.ip.stampDernier,repart->s.ip.tramesD3) == 0);
		if(repart->s.ip.stamp0 == 0) {
			repart->s.ip.stamp0 = repart->s.ip.stampDernier;
			repart->s.ip.num0 = 0;
			if((LectTimeStamp0 == 0) && (!LectStampSpecifique || (repart->categ == DONNEES_STAMP))) {
				LectTimeStamp0 = repart->s.ip.stamp0;
				if(repart->s.ip.stampD3 > repart->s.ip.duree_trame) LectTimeStamp0 += (int64)(repart->s.ip.num0 * repart->s.ip.duree_trame);
				LectTimeStampN = LectTimeStamp0;
				printf("          . TimeStamp initial du run: %lld (par repartiteur %s)\n",LectTimeStamp0,RepartDonneesListe[repart->categ]);
				if(repart->categ == DONNEES_STAMP) { repart->actif = 0; return(0); }
			}
		}
		if(synchroD3) {
			int64 stamp1;
			if(!repart->D3trouvee) { repart->D3trouvee = 1; RepartCompteCalagesD3(); }
			stamp1 = repart->s.ip.stamp1; if(repart->s.ip.stampD3 > repart->s.ip.duree_trame) stamp1 += (int64)(repart->s.ip.num1 * repart->s.ip.duree_trame);
			LectDerniereD3 = stamp1 - LectTimeStamp0;
			// printf("Synchro D3 apres %lld donnees de %s lues\n",VoieTampon[OscilloReglage[OscilloNum]->voie[0]].lus,VoieManip[OscilloReglage[OscilloNum]->voie[0]].nom);
		}
		if(repart->status_demande && (repart == RepartConnecte)) CaliRecopieStatus(trame,repart);
		timestamp = repart->s.ip.stampDernier;
		repart->s.ip.stamp1 = timestamp;
		repart->s.ip.num1 = num_trame;
		if(repart->s.ip.numAVenir >= 0) {
			/* ----- if(repart->s.ip.stamp_redemande >= 0) voir le code pour OPERA correspondant */
			if(/* (timestamp != repart->s.ip.stampEnCours) || */ (num_trame != repart->s.ip.numAVenir)) {
				int prevu,charge_utile,donnees_absentes;
				RepartIpNiveauPerte = RepartIpRecues;
				prevu = repart->s.ip.duree_trame * repart->nbdonnees * sizeof(TypeDonnee); // repart->s.ip.taille_derniere si num == tramesD3
				charge_utile = octets_lus - repart->dim.entete;
				sautees = num_trame - repart->s.ip.numAVenir;
				if(sautees < 0) {
					printf("%s/ ! %s, trame %12lld/%d[%d]: recue %12lld/%d[%d] (#%lld dans pile[%d]), erreur de transmission ignoree\n",
						DateHeure(),repart->nom,repart->s.ip.stampEnCours,repart->s.ip.numAVenir,prevu,timestamp,num_trame,charge_utile,
						repart->acces_remplis,RepartIpNiveauPerte);
				}
				else /* if(VerifErreurLog) */ printf("%s/ ! %s, trame %12lld/%d[%d]: recue %12lld/%d[%d] (#%lld dans pile[%d]), %d echappee%s\n",
					DateHeure(),repart->nom,repart->s.ip.stampEnCours,repart->s.ip.numAVenir,prevu,timestamp,num_trame,charge_utile,
					repart->acces_remplis,RepartIpNiveauPerte,Accord1s(sautees));
				CaliDumpTrameStrm(repart,trame,octets_lus,repart->acces_remplis,"          ! ");
				if(!repart->s.ip.synchro_vue) {
					gettimeofday(&LectDateRun,0);
					printf("                  > [%06d] attente d'une transmission sans erreur\n",LectDateRun.tv_usec);
					RepartIp.reduc += (prevu - charge_utile);
					repart->s.ip.stampEnCours = timestamp + repart->s.ip.inc_stamp;
					repart->s.ip.numAVenir = num_trame + 1;
					return(0);
				}
				if(sautees > 0) {
					RepartIpEchappees += sautees;
					stamp_absents = (int)(sautees * repart->s.ip.inc_stamp);
					LectStampPerdus += stamp_absents;
					if(!repart->s.ip.appel_perdant) repart->s.ip.appel_perdant = repart->acces_remplis;
					repart->s.ip.stampPerdus += (int64)stamp_absents;
				#ifdef ACCEPTABLE_MAIS_PAS_UTILISE
					repart->s.ip.manquant = stamp_absents * repart->nbdonnees;
					//a revoir: repart->s.ip.lu_entre_D3 += repart->s.ip.manquant;
					// la trame est deja dans la pile, donc il faudrait la recopier n fois
					// en attendant, on genere des 0 au depilement
					if(stamp_absents) printf("%s/ %s, generation de %d valeur%s constante%s par voie\n",DateHeure(),repart->nom,Accord2s(stamp_absents));
				#else
					donnees_absentes = stamp_absents * repart->nbdonnees;
					if(donnees_absentes < 0) donnees_absentes = 0;
					repart->s.ip.lu_entre_D3 += donnees_absentes;
					if(RepartIpCorrection == REPART_IP_GENERE) {
						// on generera ainsi des valeurs constantes au depilement
						repart->s.ip.manquant = donnees_absentes;
						repart->s.ip.saut = repart->top; /* point ou il faut commencer a generer, en esperant qu'il n'y en ait qu'un dans la pile */
						printf("%s/ %s, generation de %d valeur%s constante%s x %d voie%s\n",DateHeure(),
							repart->nom,Accord2s(stamp_absents),Accord1s(repart->nbdonnees));
					}
					//? num_trame = repart->s.ip.numAVenir + sautees;
				#endif
				} else if(!RepartIpCorrection) { LectDeclencheErreur(_ORIGINE_,42,LECT_UNEXPEC); return(0); }
				repart->s.ip.stampEnCours = timestamp;
			} else if(repart->acces_remplis < 4) {
				printf("%s/ %s, trame recue: %12lld/%d[%d] (#%lld dans pile[%d])\n",DateHeure(),repart->nom,timestamp,num_trame,
					octets_lus - repart->dim.entete,repart->acces_remplis,RepartIpNiveauPerte);
				CaliDumpTrameStrm(repart,trame,octets_lus,repart->acces_remplis,"          ");

			}
			if(!(repart->s.ip.synchro_vue) && (timestamp > repart->s.ip.stamp0)) {
				gettimeofday(&LectDateRun,0);
				printf("%s/ %s,   [%06d] demarrage de la recuperation des donnees\n",DateHeure(),repart->nom,LectDateRun.tv_usec);
			}
			repart->s.ip.synchro_vue = 1;
			repart->s.ip.numAVenir = num_trame + 1;
		} else {
			repart->s.ip.numAVenir = num_trame + 1; repart->s.ip.stampEnCours = timestamp;
			printf("%s/ %s, 1ere trame recue %12lld/%d[%d]\n",DateHeure(),repart->nom,repart->s.ip.stampEnCours,repart->s.ip.numAVenir);
		}
		if(repart->s.ip.numAVenir == 0x1000000) repart->s.ip.numAVenir = 0;
		repart->s.ip.stampEnCours += repart->s.ip.inc_stamp;
		// if(repart->s.ip.numAVenir > repart->s.ip.tramesD3) repart->s.ip.numAVenir = 0;
		//a repart->temps_precedent = dernier_temps;
	#ifdef CALI_REDEMANDE
		if(repart->s.ip.stamp_redemande >= 0) return(0);
	#endif
		if(LectDureeReset) {
			int depuis_reset,avant_reset;
			depuis_reset = Modulo(nbD3,LectDureeReset);
			avant_reset = LectDureeReset - depuis_reset;
			if((depuis_reset == LectGardeReset) || ((avant_reset * repart->s.ip.inc_stamp) > CalcSpectreAutoDuree)) {
				LectDureeReset = 0;
				OpiumProgresClose();
			} else if(!OpiumProgresRefresh(LectAttenteReset-avant_reset)) { LectDureeReset = 0; OpiumProgresClose(); }
			else return(0);
		}
		if(repart->top) memcpy(trame,fin_trame,CALI_TAILLE_ENTETE);
		RepartIpOctetsLus += octets_lus;
		RepartIp.traitees++; lues++;
		donnees_lues = (octets_lus - CALI_TAILLE_ENTETE) / sizeof(TypeDonnee); /* 2 octets par voie */
		repart->donnees_recues += donnees_lues;
		repart->top += donnees_lues;
		//a revoir: repart->s.ip.lu_entre_D3 += donnees_lues;
		if(synchroD3) {
			if(repart->s.ip.lu_entre_D3 != repart->s.ip.dim_D3) {
				printf("%s! %s: trame %12lld/%3d, %d octets recus soit %d donnees recues entre D3 (au lieu de %d)\n",DateHeure(),
					   repart->nom,timestamp,num_trame,octets_lus,repart->s.ip.lu_entre_D3,repart->s.ip.dim_D3);
				RepartIpErreurs++;
			}
			repart->s.ip.lu_entre_D3 = 0;
		} else if(octets_lus != sizeof(TypeCaliTrame)) {
			printf("%s! %s: trame %12lld/%3d, %d octets recus (au lieu de %d)\n",DateHeure(),
				   repart->nom,timestamp,num_trame,octets_lus,sizeof(TypeCaliTrame));
			RepartIpErreurs++;
		}
	}

	if(RepartIpRecues > RepartIp.depilees) RepartIp.depilees = RepartIpRecues;
	return(1);
}
#endif /* CALI_SUPPORT */
// #define ETH_SUPPORT
#ifdef ETH_SUPPORT
#pragma mark ----------- Ethernet -----------
/* ========================================================================== */
static void EtherCompteursTrame(TypeRepartiteur *repart) {
	short dim_donnees,dim_echant;
	TypeRepModele *modele_rep;

	modele_rep = &(ModeleRep[repart->type]);
	repart->dim.valeur = (modele_rep->bits32)? sizeof(TypeLarge): sizeof(TypeDonnee);
	dim_donnees = modele_rep->taille_donnees;                  // CALI_DONNEES_NB
	repart->dim.entete = modele_rep->taille_entete;            // CALI_TAILLE_ENTETE;
	repart->dim.totale = repart->dim.entete + (dim_donnees * repart->dim.valeur); // sizeof(TypeCaliTrame);
	dim_echant = (short)(repart->nbdonnees); if(dim_echant <= 0) dim_echant = 1;
	repart->s.ip.duree_trame = dim_donnees / dim_echant;       // ((CALI_DONNEES_NB - 1) / n) + 1;
	repart->dim.trame = (repart->s.ip.duree_trame * dim_echant * repart->dim.valeur) + repart->dim.entete;
	repart->s.ip.num_sts = modele_rep->code_status;            // 0xFFFF;
	/* ci-dessous, valeurs temporaires.. */
	repart->s.ip.stampD3 = repart->s.ip.duree_trame; // (int64)1000000;
	repart->s.ip.inc_stamp = repart->s.ip.duree_trame;
	repart->s.ip.tramesD3 = (int)(repart->s.ip.stampD3 / repart->s.ip.duree_trame);
	repart->s.ip.dim_D3 = repart->s.ip.tramesD3 * repart->s.ip.duree_trame * dim_echant;
	repart->s.ip.taille_derniere = (int)(repart->s.ip.stampD3 - (repart->s.ip.duree_trame * repart->s.ip.tramesD3));
	repart->gene.intervalle = (int64)(1000.0 * (float)(repart->s.ip.duree_trame) / Echantillonnage);
	//	printf("(%s) Intervalle entre deux trames: %lld us\n",__func__,repart->gene.intervalle);
}
/* ========================================================================== */
INLINE char EtherVidePile(TypeRepartiteur *repart, char *pas_tous_secs, short *pleins, char log) {
	hexa lngr;
	ssize_t octets_lus;
	int donnees_lues,num_trame,haut_pile,sautees,stamp_absents,lues,nb,nbD3;
	byte *pile,fin_trame[CALI_TAILLE_ENTETE];
	CaliTrame trame; void *adrs;
	char synchroD3; // trame_inattendue;
	int64 timestamp; // stamp_attendu;
	int64 t2;

	if(repart->top < 0) { LectDeclencheErreur(_ORIGINE_,42,LECT_INCOHER); return(0); }
	if(repart->s.ip.manquant) return(0); // generer les donnees manquantes avant d'en rajouter: test sur (top - bottom) prioritaire
	haut_pile = repart->top * repart->dim.valeur;
	if((haut_pile + repart->dim.totale) > repart->octets) { *pleins += 1; return(0); }
	pile = (byte *)repart->fifo32; /* data16 est decalee de repart->dim.entete octets, donc partir de fifo32 descend de repart->dim.entete octets */
	if(repart->top) memcpy(fin_trame,pile+haut_pile,repart->dim.entete);
	trame = (CaliTrame)(pile + haut_pile);
	lngr = sizeof(TypeSocket);
	RepartIpEssais++;
	RepartIp.acces++;
	repart->acces_demandes++;
	lues = 0;
	t2 = VerifTempsPasse? (DateMicroSecs() - LectT0Run): 0;
	octets_lus = RepartIpLitTrame(repart,trame,repart->dim.totale);
#ifdef CALI_REDEMANDE
	if(repart->s.ip.stamp_redemande >= 0) t3 = VerifTempsPasse? (DateMicroSecs() - LectT0Run): 0;
#endif
	if(octets_lus == -1) {
		if(errno == EAGAIN) {
			if(log) printf("          %s: vide\n",repart->nom);
			RepartIp.vide++;
			repart->acces_vides++;
			repart->serie_vide++;
			repart->dernier_vide = t2;
#ifdef CALI_REDEMANDE
			if(repart->s.ip.stamp_redemande >= 0) { repart->s.ip.duree_vide += (t3 - t2); repart->s.ip.nb_vide++; }
#endif
			return(0);
		} else { LectDeclencheErreur(_ORIGINE_,43,LECT_EMPTY); return(0); }
	} else if((octets_lus <= 0) || (octets_lus > repart->dim.totale)) {
		LectDeclencheErreur(_ORIGINE_,44,LECT_UNKN); return(0);
	} else if(((TypeSocketIP *)(&(repart->lec.ip.skt)))->sin_addr.s_addr != repart->adrs.val) {
		LectDeclencheErreur(_ORIGINE_,45,LECT_UNEXPEC); return(0);
	}
	if(log) {
		donnees_lues = (octets_lus - repart->dim.entete) / sizeof(shex); /* 2 octets par voie */
		printf("          %s: %d valeur%s\n",repart->nom,Accord1s(donnees_lues));
	}
	repart->acces_remplis++;
	if(repart->arrete) printf("%s! %s envoie a nouveau des donnees\n",DateHeure(),repart->nom);
	repart->arrete = 0;
	repart->serie_vide = 0;
	repart->dernier_rempli = t2;
	*pas_tous_secs = 1;
	RepartIpRecues++;
	//a gettimeofday(&LectDateRun,0);
	//a dernier_temps = (float)(LectDateRun.tv_sec % 60) + ((float)LectDateRun.tv_usec / 1000000.0);
	adrs = (void *)trame; num_trame = -1;
	/* CaliDump(repart,(byte *)trame,octets_lus,1); */
	// CaliDumpTrameEvt(trame,octets_lus);
	if(!BigEndianOrder) {
		if(repart->revision > 1) {
			num_trame = trame->hdr.id >> 8;
			if(num_trame != REPART_EVENT) CaliSwappeTrame(repart,trame,octets_lus);
		} else CaliSwappeTrame(repart,trame,octets_lus);
	}
	//	if(!BigEndianOrder && (repart->acces_remplis < 5)) printf("(%s:%d) %08X %08X %08X %08X\n",__func__,__LINE__,
	//													   *(int *)(adrs),*(int *)(adrs+4),*(int *)(adrs+8),*(int *)(adrs+12));
	if(num_trame != REPART_EVENT) num_trame = CALI_TRAME_NUMERO(trame);
	if(num_trame == REPART_EVENT) {
		TypeEventFrame *trame_evt; TypeEventDefinition *info; TypeEventChannel *canal; TypeNumeriseur *numeriseur;
		int voie,c;

		RepartIpOctetsLus += octets_lus;
		RepartIp.traitees++;
		donnees_lues = (octets_lus - repart->dim.entete) / repart->dim.valeur; /* 2 octets par voie */
		repart->donnees_recues += donnees_lues;
		//-		repart->top += donnees_lues;
		trame_evt = (TypeEventFrame *)(&(trame->valeur));
		if(trame_evt->type.etiquette == REPART_EVENT_DEF) {
			info = &(trame_evt->is.hdr.def);
			if(Acquis[AcquisLocale].etat.evt_recus == 0) Acquis[AcquisLocale].etat.evt_recus = (info->num - 1);
			if(info->num != (Acquis[AcquisLocale].etat.evt_recus + 1)) {
				printf("%s/ ! Evenement #%d attendu, recu: %d\n",DateHeure(),Acquis[AcquisLocale].etat.evt_recus,info->num);
			} else if(VerifCalculEvts) printf("%s/ Evenement recu: #%d\n",DateHeure(),info->num);
			voie = -1;
			if((info->bb >= 0) && (info->bb < repart->nbentrees))
				numeriseur = (TypeNumeriseur *)(repart->entree[info->bb].adrs);
			else numeriseur = 0;
			if(numeriseur) {
				if((info->trig >= 0) && (info->trig < numeriseur->nbadc)) voie = numeriseur->voie[info->trig];
			}
			RepartEvtEmpile = TrmtCandidatSignale(1,voie,info->stamp,info->niveau,TRGR_EXTERNE);
			if(RepartEvtEmpile < 0) printf("%s/ Evenement #%d degage\n",DateHeure(),info->num);
			TrigEnCours.canaux  = info->dim;
			TrigEnCours.canal   = 0;
			TrigEnCours.tranche = 0;
			TrigEnCours.tot     = 0; /* nombre de valeurs deja lues */
			for(c=0; c<info->dim; c++) {
				canal = &(trame_evt->is.hdr.trace.canal[c]);
				voie = -1;
				if((canal->entree >= 0) && (canal->entree < repart->nbentrees))
					numeriseur = (TypeNumeriseur *)(repart->entree[canal->entree].adrs);
				else numeriseur = 0;
				if(numeriseur) {
					if((canal->adc >= 0) && (canal->adc < numeriseur->nbadc)) voie = numeriseur->voie[canal->adc];
				}
				if((nb = EvtStocke[RepartEvtEmpile].nbvoies) < REPART_VOIES_MAX) {
					EvtStocke[RepartEvtEmpile].voie[nb].indx = voie;
					EvtStocke[RepartEvtEmpile].voie[nb].lngr = canal->dim;
					EvtStocke[RepartEvtEmpile].voie[nb].lus  = 0;
					if(voie < 0) EvtStocke[RepartEvtEmpile].voie[nb].adrs = 0;
					else if(VoieTampon[voie].brutes->type == TAMPON_INT16)
						EvtStocke[RepartEvtEmpile].voie[nb].adrs = VoieTampon[voie].prochain_s16 - VoieTampon[voie].brutes->t.sval;
					else if(VoieTampon[voie].brutes->type == TAMPON_INT32)
						EvtStocke[RepartEvtEmpile].voie[nb].adrs = VoieTampon[voie].prochain_i32 - VoieTampon[voie].brutes->t.ival;
					EvtStocke[RepartEvtEmpile].nbvoies++;
					if(VerifCalculEvts) printf("          L'evt #%d sur la voie %s commence a %d\n",info->num,VoieManip[voie].nom,EvtStocke[RepartEvtEmpile].voie[nb].adrs);
				}
				canal++;
			}
		} else if(trame_evt->type.etiquette == REPART_EVENT_DATA) {
			TypeEventData *donnees;
			TypeDonnee *sval,*prochain_s16,d16;
			TypeLarge  *ival,*prochain_i32,d32;
			// TypeSignal *rval,*prochain_r32,donnee_r32;
			int nb; short voie;
			donnees = (TypeEventData *)(&(trame_evt->is.donnees));
			if(donnees->ref.tranche == TrigEnCours.tranche) {
				if(VerifCalculEvts) printf("%s/ Tranche #%d recue: %d valeur%s\n",DateHeure(),donnees->ref.tranche,Accord1s(donnees->ref.nb));
				TrigEnCours.tranche += 1;
			} else {
				if(VerifCalculEvts) printf("%s/ Tranche #%d attendue, recue: %d\n",DateHeure(),TrigEnCours.tranche,donnees->ref.tranche);
				TrigEnCours.tranche = donnees->ref.tranche + 1;
			}
			if(TrigEnCours.canal >= TrigEnCours.canaux) {
				printf("%s/ Donnees en excedent (canal #%d/%d non prevu)\n",DateHeure(),TrigEnCours.canal,TrigEnCours.canaux);
				nb = donnees->ref.nb;
			} else {
				voie = EvtStocke[RepartEvtEmpile].voie[TrigEnCours.canal].indx;
				if(VoieTampon[voie].brutes->type == TAMPON_INT16) sval = donnees->val.s;
				else if(VoieTampon[voie].brutes->type == TAMPON_INT32) ival = donnees->val.i;
				nb = 0;
			}
			while(nb < donnees->ref.nb) {
				//				if(BigEndianOrder) val_lue = val[nb++]; else if(nb & 1) val_lue = val[nb++ - 1]; else val_lue = val[++nb];
				if((voie >= 0) && VoieTampon[voie].brutes && VoieTampon[voie].brutes->max) {
#ifndef CORRECTION_VALEUR
					if(VoieTampon[voie].brutes->type == TAMPON_INT16) d16 = sval[nb++];
					else if(VoieTampon[voie].brutes->type == TAMPON_INT32) d32 = ival[nb++];
#else
					int32 val_lue;
					if(VoieTampon[voie].brutes->type == TAMPON_INT16) {
						val_lue = sval[nb++];
						if(repart->famille == FAMILLE_CALI) {
							d16 = (TypeDonnee)((val_lue & VoieManip[voie].ADCmask) - VoieManip[voie].zero);
						} else if((repart->famille == FAMILLE_OPERA) && (repart->r.opera.mode == code_acqui_EDW_BB2)) {
							d16 = (LectEnCours.valeur & 0x8000)? LectEnCours.valeur & 0x7FFF: LectEnCours.valeur | 0x8000;
						} else {
							if(VoieManip[voie].ADCmask == 0xFFFF) d16 = (TypeDonnee)val_lue;
							else {
								d16 = (val_lue & VoieManip[voie].ADCmask); // DATAVALEUR(voie,val_lue)
								if(VoieManip[voie].signe) {
#ifdef DONNEES_NON_SIGNEES
									if(d16 < VoieManip[voie].zero) donnee += VoieManip[voie].zero;
									else d16 -= VoieManip[voie].zero;
#else
#ifdef SIGNE_ARITHMETIQUE
									if(d16 >= VoieManip[voie].zero) donnee -= (2 * VoieManip[voie].zero);
#else  /* SIGNE_ARITHMETIQUE */
									if(d16 & VoieManip[voie].zero) donnee |= VoieManip[voie].extens;
#endif /* SIGNE_ARITHMETIQUE */
#endif
								}
							}
						}
					} else if(VoieTampon[voie].brutes->type == TAMPON_INT32) {
						val_lue = ival[nb++];
						if(repart->famille == FAMILLE_CALI) {
							d32 = (val_lue & VoieManip[voie].ADCmask) - VoieManip[voie].zero;
						} else if((repart->famille == FAMILLE_OPERA) && (repart->r.opera.mode == code_acqui_EDW_BB2)) {
							d32 = (LectEnCours.valeur & VoieManip[voie].zero)? LectEnCours.valeur & 0x7FFF: LectEnCours.valeur | 0x8000;
						} else {
							if(VoieManip[voie].ADCmask == 0xFFFFFFF) d32 = val_lue;
							else {
								d32 = (val_lue & VoieManip[voie].ADCmask); // DATAVALEUR(voie,val_lue)
								if(VoieManip[voie].signe) {
#ifdef DONNEES_NON_SIGNEES
									if(d32 < VoieManip[voie].zero) donnee += VoieManip[voie].zero;
									else d32 -= VoieManip[voie].zero;
#else
#ifdef SIGNE_ARITHMETIQUE
									if(d32 >= VoieManip[voie].zero) donnee -= (2 * VoieManip[voie].zero);
#else  /* SIGNE_ARITHMETIQUE */
									if(d32 & VoieManip[voie].zero) donnee |= VoieManip[voie].extens;
#endif /* SIGNE_ARITHMETIQUE */
#endif
								}
							}
						}
					}
#endif /* CORRECTION_VALEUR */
					if(VoieTampon[voie].brutes->type == TAMPON_INT16) {
						VoieTampon[voie].derniere_s16 = VoieTampon[voie].courante_s16 = d16; /* courante_s16 pour la reutilisation de voies (pseudos) */
						*(VoieTampon[voie].prochain_s16) = d16;
						prochain_s16 = VoieTampon[voie].prochain_s16;
						if(((VoieTampon[voie].brutes->prem + 1) == VoieTampon[voie].brutes->max) || ((VoieTampon[voie].brutes->nb + 1) == VoieTampon[voie].brutes->max))
							prochain_s16 = VoieTampon[voie].brutes->t.sval;
						else prochain_s16++;
					} else if(VoieTampon[voie].brutes->type == TAMPON_INT32) {
						VoieTampon[voie].derniere_i32 = VoieTampon[voie].courante_i32 = d32; /* courante_i32 pour la reutilisation de voies (pseudos) */
						*(VoieTampon[voie].prochain_i32) = d32;
						prochain_i32 = VoieTampon[voie].prochain_i32;
						if(((VoieTampon[voie].brutes->prem + 1) == VoieTampon[voie].brutes->max) || ((VoieTampon[voie].brutes->nb + 1) == VoieTampon[voie].brutes->max))
							prochain_i32 = VoieTampon[voie].brutes->t.ival;
						else prochain_i32++;
					}
					LectStampsLus++;
					VoieTampon[voie].lus++;
					if(VoieTampon[voie].brutes->nb < VoieTampon[voie].brutes->max) {
						VoieTampon[voie].brutes->nb++;
						if(VoieTampon[voie].brutes->nb >= VoieTampon[voie].brutes->max) {
							VoieTampon[voie].circulaire = 1;
							if(VoieTampon[voie].brutes->type == TAMPON_INT16)
								VoieTampon[voie].prochain_s16 = VoieTampon[voie].brutes->t.sval + VoieTampon[voie].brutes->prem;
							else if(VoieTampon[voie].brutes->type == TAMPON_INT32)
								VoieTampon[voie].prochain_i32 = VoieTampon[voie].brutes->t.ival + VoieTampon[voie].brutes->prem;
						} else {
							if(VoieTampon[voie].brutes->type == TAMPON_INT16) VoieTampon[voie].prochain_s16 += 1;
							else if(VoieTampon[voie].brutes->type == TAMPON_INT32) VoieTampon[voie].prochain_i32 += 1;
						}
					} else {
						/* if(SettingsModeDonnees == 0) { LectDeclencheErreur(_ORIGINE_,51,LECT_FULL); goto retour; } */
						VoieTampon[voie].circulaire = 1;
						VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien++;
						VoieTampon[voie].brutes->prem++;
						if(VoieTampon[voie].brutes->prem >= VoieTampon[voie].brutes->max) {
							VoieTampon[voie].brutes->prem = 0;
							VoieTampon[voie].trmt[TRMT_AU_VOL].point0 = VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien;
						}
						if(VoieTampon[voie].brutes->type == TAMPON_INT16)
							VoieTampon[voie].prochain_s16 = VoieTampon[voie].brutes->t.sval + VoieTampon[voie].brutes->prem;
						if(VoieTampon[voie].brutes->type == TAMPON_INT32)
							VoieTampon[voie].prochain_i32 = VoieTampon[voie].brutes->t.ival + VoieTampon[voie].brutes->prem;
					}
					if((VoieTampon[voie].brutes->type == TAMPON_INT16) && (prochain_s16 != VoieTampon[voie].prochain_s16)) {
						printf("%s/ !! Ecriture dans le buffer de %s a la mauvaise place [%lld]\n",VoieManip[voie].nom,++LectSyncRead);
					} else if((VoieTampon[voie].brutes->type == TAMPON_INT32) && (prochain_i32 != VoieTampon[voie].prochain_i32)) {
						printf("%s/ !! Ecriture dans le buffer de %s a la mauvaise place [%lld]\n",VoieManip[voie].nom,++LectSyncRead);
					}
				}
				if(++(EvtStocke[RepartEvtEmpile].voie[TrigEnCours.canal].lus) >= EvtStocke[RepartEvtEmpile].voie[TrigEnCours.canal].lngr) {
					TrigEnCours.canal++;
					voie = EvtStocke[RepartEvtEmpile].voie[TrigEnCours.canal].indx;
				}
			}
		} else if(trame_evt->type.etiquette == REPART_EVENT_CLOSED) {
			if(VerifCalculEvts) printf("%s/ Evenement a traiter: #%d\n",DateHeure(),EvtASauver);
			TrmtConstruitEvt();

		}

		/* mode stream */
	} else {
		if(repart->revision == 1) repart->s.ip.stampDernier = CALI_TRAME_TIMESTAMP(trame);
		else repart->s.ip.stampDernier = repart->s.ip.stampEnCours;
		nbD3 = (int)(repart->s.ip.stampDernier / repart->s.ip.inc_stamp);
		synchroD3 = 0;
		//a revoir: synchroD3 = (Modulo(repart->s.ip.stampDernier,repart->s.ip.tramesD3) == 0);
		if(repart->s.ip.stamp0 == 0) {
			repart->s.ip.stamp0 = repart->s.ip.stampDernier;
			repart->s.ip.num0 = 0;
			if((LectTimeStamp0 == 0) && (!LectStampSpecifique || (repart->categ == DONNEES_STAMP))) {
				LectTimeStamp0 = repart->s.ip.stamp0;
				if(repart->s.ip.stampD3 > repart->s.ip.duree_trame) LectTimeStamp0 += (int64)(repart->s.ip.num0 * repart->s.ip.duree_trame);
				LectTimeStampN = LectTimeStamp0;
				printf("          . TimeStamp initial du run: %lld (par repartiteur %s)\n",LectTimeStamp0,RepartDonneesListe[repart->categ]);
				if(repart->categ == DONNEES_STAMP) { repart->actif = 0; return(0); }
			}
		}
		if(synchroD3) {
			int64 stamp1;
			if(!repart->D3trouvee) { repart->D3trouvee = 1; RepartCompteCalagesD3(); }
			stamp1 = repart->s.ip.stamp1; if(repart->s.ip.stampD3 > repart->s.ip.duree_trame) stamp1 += (int64)(repart->s.ip.num1 * repart->s.ip.duree_trame);
			LectDerniereD3 = stamp1 - LectTimeStamp0;
			// printf("Synchro D3 apres %lld donnees de %s lues\n",VoieTampon[OscilloReglage[OscilloNum]->voie[0]].lus,VoieManip[OscilloReglage[OscilloNum]->voie[0]].nom);
		}
		if(repart->status_demande && (repart == RepartConnecte)) CaliRecopieStatus(trame,repart);
		timestamp = repart->s.ip.stampDernier;
		repart->s.ip.stamp1 = timestamp;
		repart->s.ip.num1 = num_trame;
		if(repart->s.ip.numAVenir >= 0) {
			/* ----- if(repart->s.ip.stamp_redemande >= 0) voir le code pour OPERA correspondant */
			if((timestamp != repart->s.ip.stampEnCours) || (num_trame != repart->s.ip.numAVenir)) {
				int donnees_absentes;
				RepartIpNiveauPerte = RepartIpRecues;
				sautees = num_trame - repart->s.ip.numAVenir;
				if(VerifErreurLog) {
					int prevu,charge_utile;
					prevu = repart->s.ip.duree_trame * repart->nbdonnees * repart->dim.valeur; // repart->s.ip.taille_derniere si num == tramesD3
					charge_utile = octets_lus - repart->dim.entete;
					printf("%s/ %s, trame %12lld/%d[%d]: recue %12lld/%d[%d] (#%lld dans pile[%d]), %d echappee%s\n",
						   DateHeure(),repart->nom,repart->s.ip.stampEnCours,repart->s.ip.numAVenir,prevu,
						   timestamp,num_trame,charge_utile,repart->acces_remplis,RepartIpNiveauPerte,Accord1s(sautees));
					if(!repart->s.ip.synchro_vue) {
						gettimeofday(&LectDateRun,0);
						printf("                  > [%06d] attente d'une transmission sans erreur\n",LectDateRun.tv_usec);
						RepartIp.reduc += (prevu - charge_utile);
						repart->s.ip.stampEnCours = timestamp + repart->s.ip.inc_stamp;
						repart->s.ip.numAVenir = num_trame + 1;
						return(0);
					}
				}
				RepartIpEchappees += sautees;
				stamp_absents = (int)(sautees * repart->s.ip.inc_stamp);
				LectStampPerdus += stamp_absents;
				if(!repart->s.ip.appel_perdant) repart->s.ip.appel_perdant = repart->acces_remplis;
				repart->s.ip.stampPerdus += (int64)stamp_absents;
			#ifdef ACCEPTABLE_MAIS_PAS_UTILISE
				repart->s.ip.manquant = stamp_absents * repart->nbdonnees;
				//a revoir: repart->s.ip.lu_entre_D3 += repart->s.ip.manquant;
				// la trame est deja dans la pile, donc il faudrait la recopier n fois
				// en attendant, on genere des 0 au depilement
				if(stamp_absents) printf("%s/ %s, generation de %d valeur%s constante%s par voie\n",DateHeure(),repart->nom,Accord2s(stamp_absents));
			#else
				donnees_absentes = stamp_absents * repart->nbdonnees;
				if(donnees_absentes < 0) donnees_absentes = 0;
				repart->s.ip.lu_entre_D3 += donnees_absentes;
				if(RepartIpCorrection == REPART_IP_GENERE) {
					// on generera ainsi des valeurs constantes au depilement
					repart->s.ip.manquant = donnees_absentes;
					repart->s.ip.saut = repart->top; /* point ou il faut commencer a generer, en esperant qu'il n'y en ait qu'un dans la pile */
					printf("%s/ %s, generation de %d valeur%s constante%s x %d voie%s\n",DateHeure(),repart->nom,Accord2s(stamp_absents),Accord1s(repart->nbdonnees));
				}
				num_trame = repart->s.ip.numAVenir + sautees;
			#endif
				repart->s.ip.stampEnCours = timestamp;
			}
			if(!(repart->s.ip.synchro_vue) && (timestamp > repart->s.ip.stamp0)) {
				gettimeofday(&LectDateRun,0);
				printf("%s/ %s,   [%06d] demarrage de la recuperation des donnees\n",DateHeure(),repart->nom,LectDateRun.tv_usec);
			}
			repart->s.ip.synchro_vue = 1;
			repart->s.ip.numAVenir = num_trame + 1;
		} else {
			repart->s.ip.numAVenir = num_trame + 1; repart->s.ip.stampEnCours = timestamp;
			printf("%s/ %s, 1ere trame recue %12lld/%d[%d]\n",DateHeure(),repart->nom,repart->s.ip.stampEnCours,repart->s.ip.numAVenir);
		}
		if(repart->s.ip.numAVenir == 0x1000000) repart->s.ip.numAVenir = 0;
		repart->s.ip.stampEnCours += repart->s.ip.inc_stamp;
		// if(repart->s.ip.numAVenir > repart->s.ip.tramesD3) repart->s.ip.numAVenir = 0;
		//a repart->temps_precedent = dernier_temps;
#ifdef CALI_REDEMANDE
		if(repart->s.ip.stamp_redemande >= 0) return(0);
#endif
		if(LectDureeReset) {
			int depuis_reset,avant_reset;
			depuis_reset = Modulo(nbD3,LectDureeReset);
			avant_reset = LectDureeReset - depuis_reset;
			if((depuis_reset == LectGardeReset) || ((avant_reset * repart->s.ip.inc_stamp) > CalcSpectreAutoDuree)) {
				LectDureeReset = 0;
				OpiumProgresClose();
			} else if(!OpiumProgresRefresh(LectAttenteReset-avant_reset)) { LectDureeReset = 0; OpiumProgresClose(); }
			else return(0);
		}
		if(repart->top) memcpy(trame,fin_trame,repart->dim.entete);
		RepartIpOctetsLus += octets_lus;
		RepartIp.traitees++; lues++;
		donnees_lues = (octets_lus - repart->dim.entete) / repart->dim.valeur; /* 2 octets par voie */
		repart->donnees_recues += donnees_lues;
		repart->top += donnees_lues;
		//a revoir: repart->s.ip.lu_entre_D3 += donnees_lues;
		if(synchroD3) {
			if(repart->s.ip.lu_entre_D3 != repart->s.ip.dim_D3) {
				printf("%s! %s: trame %12lld/%3d, %d octets recus soit %d donnees recues entre D3 (au lieu de %d)\n",DateHeure(),
					   repart->nom,timestamp,num_trame,octets_lus,repart->s.ip.lu_entre_D3,repart->s.ip.dim_D3);
				RepartIpErreurs++;
			}
			repart->s.ip.lu_entre_D3 = 0;
		} else if(octets_lus != repart->dim.totale) {
			printf("%s! %s: trame %12lld/%3d, %d octets recus (au lieu de %d)\n",DateHeure(),
				   repart->nom,timestamp,num_trame,octets_lus,sizeof(TypeCaliTrame));
			RepartIpErreurs++;
		}
	}

	if(RepartIpRecues > RepartIp.depilees) RepartIp.depilees = RepartIpRecues;
	return(1);
}
#endif
#pragma mark ----------- SAMBA -----------
/* ========================================================================== */
INLINE int SmbEcrit(TypeRepartiteur *repart, byte cmde, byte c1, byte c2, shex valeur) {
	byte buffer[8]; int n;

	buffer[0] = cmde;
	buffer[1] = 0;
	buffer[2] = c1;
	buffer[3] = c2;
	buffer[5] = (byte)((valeur >> 8) & 0xFF);
	buffer[4] = (byte)(valeur & 0xFF);
	n = RepartIpEcrit(repart,buffer,6); SambaMicrosec(ModeleRep[repart->type].delai_msg);
	sprintf(RepartiteurValeurEnvoyee,"%c.%02X.%02X.%02X.%02X%02X",buffer[0],buffer[1],buffer[2],buffer[3],buffer[5],buffer[4]);
	return(n);
}
/* ========================================================================== */
INLINE static void SmbDemarreNumeriseurs(TypeRepartiteur *repart, NUMER_MODE mode, byte adrs, char *prefixe) {
	shex cde_cluzel;

	if(mode == NUMER_MODE_COMPTEUR) {
		if(prefixe) printf("%s! Mode '%s' pas implemente\n",prefixe,NumerModeTexte[mode]);
	} else {
		cde_cluzel = (mode == NUMER_MODE_ACQUIS)? CLUZEL_START: CLUZEL_STOP;
		SmbEcrit(repart,SMB_NUMER,adrs,CLUZEL_SSAD_ACQUIS,cde_cluzel);
		if(prefixe) printf("%s. Ecrit: %s\n",prefixe,RepartiteurValeurEnvoyee);
	}
}
#pragma mark ----------- Fichier -----------
/* ========================================================================== */
char ArchOpenRun() {
	char nom_fichier[MAXFILENAME];

	strcat(strcat(strcpy(ArchRun,RelecPath),ArchDir[STRM]),SrceName);
	if(RepertoireExiste(ArchRun)) {
		strcat(strcat(strcat(strcpy(nom_fichier,ArchRun),FILESEP),SrceName),"_000");
		ArchPartition = 0;
	} else {
		strcpy(nom_fichier,ArchRun);
		ArchPartition = -1;
	}
	if((ArchPath = open(nom_fichier,O_RDONLY,0)) >= 0) return(1);
	else return(0);
}
/* ========================================================================== */
char ArchOpenPartition() {
	char nom_fichier[MAXFILENAME]; char numero[8];

	sprintf(numero,"_%03d",++ArchPartition);
	strcat(strcat(strcat(strcpy(nom_fichier,ArchRun),FILESEP),SrceName),numero);
	if((ArchPath = open(nom_fichier,O_RDONLY,0)) >= 0) return(1);
	else return(0);
}
/* ========================================================================== */
static int ArchInit(TypeRepartiteur *repart, int l) {
	char nomdemande[MAXFILENAME],*runname; char texte[80];

	if(!strcmp(repart->parm,".") || !strcmp(repart->parm,"-")) {
		strcat(strcpy(repart->parm,RelecPath),ArchDir[STRM]);
	} else if(!strcmp(repart->parm,"?")) {
		strcat(strcpy(nomdemande,RelecPath),ArchDir[STRM]);
		sprintf(texte,"Nom du volume a utiliser pour l'archivage %s",repart->nom);
		if(OpiumReadFile(texte,nomdemande,80) == PNL_CANCEL) return(0);
		strcpy(repart->parm,nomdemande);
	}
	runname = (char *)repart->entree[l].adrs;
	nomdemande[0] = '\0';
	if(SrceName[0] == '\0') strcpy(SrceName,"amjjxnnn"); // StreamALire
	if(!strcmp(runname,".") || !strcmp(runname,"-")) {
		if(SambaSrceUtilisee) runname[0] = '?';
		else { strcpy(nomdemande,SrceName); SambaSrceUtilisee = 1; }
	}
	if(!strcmp(runname,"?")) {
		strcpy(nomdemande,SrceName);
		sprintf(texte,"Nom du run a lire via l'entree #%d",l+1);
		if(OpiumReadFile(texte,nomdemande,80) == PNL_CANCEL) return(0);
	}
	if(nomdemande[0]) {
		free(repart->entree[l].adrs);
		repart->entree[l].adrs = (TypeAdrs)malloc(strlen(nomdemande)+1);
		if(!(repart->entree)) { printf("  ! Pas assez de memoire pour l'entree #%d de %s\n",l+1,repart->nom); return(0); }
		strcpy((char *)repart->entree[l].adrs,nomdemande);
	} else strncpy(SrceName,runname,80);
	repart->p.f.a[l].partition = 0;
	repart->p.f.a[l].started = 0;
	return(1);
}
/* ========================================================================== */
static int ArchOuvre(TypeRepartiteur *repart, int l) {
	char nomcomplet[MAXFILENAME],*runname; char ouvert;
	int lngr;
	char nomrun[RUN_NOM],numero[12];

	runname = (char *)repart->entree[l].adrs;
	if(runname[0] == FILESEP[0]) {
		strcpy(nomcomplet,runname);
		lngr = strlen(runname) - 1;
		if(lngr < 0) { OpiumError("Nom de fichier vide"); return(0); }
		if(!strcmp(runname+lngr,FILESEP)) runname[lngr] = '\0';
		while(--lngr >= 0) { if(runname[lngr] == FILESEP[0]) break; }
		strncpy(nomrun,runname+lngr+1,RUN_NOM);
	} else {
		strncpy(nomrun,runname,RUN_NOM);
		strcat(strcat(strcpy(nomcomplet,repart->parm),FILESEP),runname);
	}
	if(RepertoireExiste(nomcomplet)) {
		sprintf(numero,"_%03d",repart->p.f.a[l].partition);
		strcat(strcat(strcat(nomcomplet,FILESEP),nomrun),numero);
	} else if(repart->p.f.a[l].partition) {
		LectDeclencheErreur(_ORIGINE_,91,LECT_EOF);
		return(0);
	}
	ArchFerme(repart,l);
	if(repart->p.f.a[l].partition) printf("%s/ ",DateHeure()); else printf("  * ");
	printf("Ouverture du fichier %s\n",nomcomplet);
	ouvert = 0;
	if(repart->p.f.bin) {
		if((repart->p.f.a[l].o.path = open(nomcomplet,O_RDONLY,0)) >= 0) {
			ouvert = 1; if(LectTrancheRelue == 0) repart->p.f.a[l].d.of7 = 0; /* temporaire */
		}
	} else {
		if((repart->p.f.a[l].o.file = fopen(nomcomplet,"r"))) {
			ouvert = 1; if(LectTrancheRelue == 0) repart->p.f.a[l].d.off = ftell(repart->p.f.a[l].o.file);
		}
	}
	if(!ouvert) {
		if(repart->p.f.a[l].partition) {
			printf("%s/ ! Il n'y a pas d'autre partition dans ce run\n",DateHeure());
			LectDeclencheErreur(_ORIGINE_,92,LECT_EOF);
		} else OpiumFail("Fichier '%s' inaccessible: %s, execution impossible",nomcomplet,strerror(errno));
		return(0);
	}
	repart->p.f.a[l].started = 0;
	//- if(!LectTrancheRelue) LectSurFichierDejaVu = 0;
	return(1);
}
/* ========================================================================== */
static void ArchFerme(TypeRepartiteur *repart, int l) {
	if(repart->p.f.bin) {
		if(repart->p.f.a[l].o.path >= 0) { close(repart->p.f.a[l].o.path); repart->p.f.a[l].o.path = -1; }
	} else {
		if(repart->p.f.a[l].o.file) { fclose(repart->p.f.a[l].o.file); repart->p.f.a[l].o.file = 0; }
	}
}
/* ========================================================================== */
static int ArchEntete(TypeRepartiteur *repart, int l, char *explic) {
	int p; char ligne[1024],*c,*item,*nom_bolo; char nom[256];
	float version_lue;
	char imprime_entetes,imprime_actions;
	int i,lngr;
	char byteswappe;
	int voie,bolo,cap,vm; int nb,etage;
	TypeFiltre filtre;

	imprime_entetes = imprime_actions = 1;
	p = repart->p.f.a[l].o.path;
	if(imprime_actions) printf("- Lecture de la signature\n");
	if(!EnregSuivant(ligne,1024,p)) {
		sprintf(explic,"Premiere ligne illisible (%s)",strerror(errno));
		return(0);
	}
	lngr = strlen(ligne); if(lngr) { if(ligne[lngr - 1] == '\n') ligne[lngr - 1] = '\0'; }
	if(strcmp(ligne,SIGNATURE)) {
		sprintf(explic,"Fichier etranger (%s)",ligne);
		return(0);
	}
	if(imprime_entetes) printf(". Type de fichier: '%s'\n",ligne);
	if(!EnregSuivant(ligne,1024,p)) {
		sprintf(explic,"Deuxieme ligne illisible (%s)",strerror(errno));
		return(0);
	}
	lngr = strlen(ligne); if(lngr) { if(ligne[lngr - 1] == '\n') ligne[lngr - 1] = '\0'; }
	lngr = strlen(LIGNE_VERSION);
	if(!strncmp(ligne,LIGNE_VERSION,lngr)) {
		sscanf(ligne+lngr,"%g",&version_lue);
		if(imprime_entetes) printf(". Version d'origine de cette archive: %g\n",version_lue);
	} else {
		version_lue = 6.3;
		printf(". Version d'origine, irrecuperable. Version par defaut: %g\n",version_lue);
	}

	if(imprime_actions) printf("- Lecture de l'entete generale\n");
	BigEndianOrder = 1; /* par defaut */
	ArchHdrNbInc = ArchHdrNbDesc[STRM];
	ArgFromPath(p,ArchHdrGeneral,LIMITE_ENTETE);
	BigEndianSource = BigEndianOrder;
	BigEndianOrder = EndianIsBig(); /* pour restaurer la bonne valeur */
	byteswappe = (BigEndianOrder != BigEndianSource);
	if(imprime_entetes) ArgPrint("*",ArchHdrGeneral);

	DetecteursModlVerifie();
	voie = 0; /* Si un filtre est declare avant toute voie, il est affecte a la voie 0 par defaut */
	do {
		if(!EnregSuivant(ligne,1024,p)) break;

		if(!strncmp(ligne,"* Det",5) || !strncmp(ligne,"* Bolo",6)) {
			c = ligne + 12;
			item = ItemSuivant(&c,':');
			if(!strcmp(item,"indetermine")) {
				if(imprime_actions) printf("- Le detecteur #%d suivant est %s et ne sera pas lu\n",BoloNb,item);
			} else {
				if(imprime_actions) {
                    if(imprime_entetes) printf("\n");
                    printf("- Lecture des caracteristiques du detecteur #%d, '%s'\n",BoloNb,item);
                }
				/* detecteur deja declare? */
				for(bolo=0; bolo<BoloNb; bolo++) if(!strcmp(item,Bolo[bolo].nom)) break;
				if(bolo < BoloNb) {
					ArgSkipOnPath(p,ArchHdrBoloDef,LIMITE_ENTETE); // ArgSkipOnPath nouvellement ajoute dans CLAPS!
					continue;
				}
                ReglageRazBolo(&BoloEcrit,item);
				ArgFromPath(p,ArchHdrBoloDef,LIMITE_ENTETE);
				if(!strncmp(BoloEcrit.hote,"EDWACQ-",7)) {
					strcpy(nom,BoloEcrit.hote+7);
					strcpy(BoloEcrit.hote,nom);
				}
				if(imprime_entetes) ArgPrint("*",ArchHdrBoloDef);
				if(BoloNb < BoloGeres) {
					memcpy(Bolo+BoloNb,&BoloEcrit,sizeof(TypeDetecteur)); /* !!! reglages pas recopies */
					Bolo[BoloNb].local = 1;
					Bolo[BoloNb].a_lire = 1;
                    if(version_lue < 7.0) {
						/* A cette epoque, un seul bolo etait de toutes facons sauve.
						 On eut pu, s'il y en eu plusieurs, partir de VoiesConnues qui donne ModeleVoie
						 et generer toutes les VoieManip */
						for(cap=0; cap<DETEC_CAPT_MAX; cap++) Bolo[BoloNb].captr[cap].bb.adrs = 0x33;
						for(voie=0; voie<VoiesNb; voie++) {
							strcat(strcat(VoieManip[voie].nom," "),Bolo[BoloNb].nom);
                            if((cap = Bolo[BoloNb].nbcapt) < DETEC_CAPT_MAX) {
                                Bolo[BoloNb].captr[cap].voie = voie;
                                Bolo[BoloNb].nbcapt += 1;
                            }
                        }
                    } else {
                        for(voie=0; voie<VoiesNb; voie++) {
                            strcpy(nom,VoieManip[voie].nom);
                            c = nom;
                            ItemSuivant(&c,' ');
                            nom_bolo = ItemSuivant(&c,' ');
                            if(!strcmp(Bolo[BoloNb].nom,nom_bolo)) {
								VoieManip[voie].det = BoloNb;
                                if((cap = Bolo[BoloNb].nbcapt) < DETEC_CAPT_MAX) {
                                    Bolo[BoloNb].captr[cap].voie = voie;
									printf("  . Bolo[%d].voie[%d] = #%d\n",BoloNb,cap,voie);
                                    Bolo[BoloNb].nbcapt += 1;
									if(VoieManip[voie].type < VOIES_TYPES) ModeleVoie[VoieManip[voie].type].utilisee = 1;
                                }
                            }
                        }
                    }
					if(imprime_actions) {
                        if(imprime_entetes) printf("=> ");
						if(Bolo[BoloNb].lu == DETEC_OK) {
							printf(". Bolo #%02d %3gg: %16s, place en %03x, lu par %s via BB#%d @%02X\n",
								BoloNb,Bolo[BoloNb].masse,Bolo[BoloNb].nom,Bolo[BoloNb].pos,
								Bolo[BoloNb].hote,Bolo[BoloNb].captr[0].bb.serial,Bolo[BoloNb].captr[0].bb.adrs);
						} else {
							printf(". Bolo #%02d non lu: '%s'\n",BoloNb,Bolo[BoloNb].nom);
						}
					}
                    BoloNb++;
				} else {
					if(imprime_actions) printf(". Ce detecteur est excedentaire (maxi: %d) et ne sera pas memorise\n",BoloGeres);
				}
			}

		} else if(!strncmp(ligne,"* Physique",10)) {
			ArgFromPath(p,ArchHdrPhysDef,LIMITE_ENTETE);
			if(imprime_entetes) ArgPrint("*",ArchHdrPhysDef);

		} else if(!strncmp(ligne,"* Voie",6)) {
			c = ligne + 7;
			item = ItemSuivant(&c,':');
			voie = VoiesNb;
			memcpy(&VoieEcrite,VoieManip+voie,sizeof(TypeVoieAlire));
			if(imprime_actions) {
                if(imprime_entetes) printf("\n");
                printf("- Lecture de la definition de la voie '%s'\n",item);
            }
			VoieEcrite.gain_variable = 1.0;  /* oubli dans V7.2 */
			//- VoieEcrite.copie = -1; /* ajoute depuis V7.19 */
			VoieEcrite.pseudo = 0;
			VoieEcrite.def.evt.base_dep = 25.0; /* oublie jusqu'au moins V9.1.8 */
			VoieEcrite.def.evt.base_arr = 75.0; /* oublie jusqu'au moins V9.1.8 */
			VoieEcrite.def.evt.ampl_10 = 10.0; /* ajoute a la version 9.17 */
			VoieEcrite.def.evt.ampl_90 = 90.0; /* ajoute a la version 9.17 */
			ArgFromPath(p,ArchHdrVoieDef,LIMITE_ENTETE);
			if(VoiesNb < VoiesGerees) {
				if(version_lue < 7.0) {
					/* on ne fait pas dans la dentelle, mais de toutes facons, cela ne concerne qu'UN SEUL fichier! [ !!: + NOSTOS] */
					VoieEcrite.det = 0;
					VoieEcrite.type = voie;
					VoieEcrite.def.evt.pretrigger = 25.0;
					VoieEcrite.def.evt.tempsmort = 25.0;
					VoieEcrite.ADCmask = 0x3FFF;
					VoieEcrite.ADUenmV = 250.0;
					VoieEcrite.gain_fixe = 1.0;
					VoieEcrite.Rmodul = 2200.0 / 8.2;
				}
				if(imprime_entetes) ArgPrint("*",ArchHdrVoieDef);
				memcpy(VoieManip+voie,&VoieEcrite,sizeof(TypeVoieAlire));
				/* malheureusement, les trois donnees suivantes sont obsoletes a ce niveau... donc VoieEvent indefini!!! */
				VoieEvent[voie].horloge = ArchVoie.horloge;
				VoieEvent[voie].lngr = ArchVoie.dim;
				VoieEvent[voie].avant_evt = ArchVoie.avant_evt;
				VoieEvent[voie].filtre = 0;
				strncpy(VoieManip[voie].nom,item,VOIE_NOM);
				//			if(VoieEvent[voie].avant_evt <= 0) VoieEvent[voie].avant_evt = (VoieEvent[voie].lngr * 3) / 4;
				strcpy(nom,VoieManip[voie].nom);
				c = nom;
				ItemJusquA(' ',&c);
				for(vm=0; vm<ModeleVoieNb; vm++) if(!strcmp(nom,ModeleVoie[vm].nom)) break;
				if((vm >= ModeleVoieNb) && (ModeleVoieNb < VOIES_TYPES)) {
					int p,ph;
					if(!strncmp(nom,"Chal",4) || !strncmp(nom,"chal",4) || !strncmp(nom,"lum",3)) p = ModeleVoie[vm].physique = 0;
					else p = ModeleVoie[vm].physique = 1;
					strncpy(ModeleVoie[vm].nom,nom,MODL_NOM);
					ModeleVoie[vm].duree = ModelePhys[p].duree;
					ModeleVoie[vm].delai = ModelePhys[p].delai;
					ModeleVoie[vm].tempsmort = ModelePhys[p].tempsmort;
					ModeleVoie[vm].interv = ModelePhys[p].interv;
					ModeleVoie[vm].coinc = 20.0;
					ModeleVoie[vm].pretrigger = 25.0;
					ModeleVoie[vm].base_dep = 25.0;
					ModeleVoie[vm].base_arr = 75.0;
					for(ph=0; ph<DETEC_PHASES_MAX; ph++) {
						ModeleVoie[vm].phase[ph].t0 = 0.0;
						ModeleVoie[vm].phase[ph].dt = 0.0;
					}
					ModeleVoie[vm].utilisee = 0;
#ifdef RL
					ModeleVoie[vm].dimtemplate = ModelePhys[p].dimtemplate;
					ModeleVoie[vm].montee = ModelePhys[p].montee;
					ModeleVoie[vm].descente1 = ModelePhys[p].descente1;
					ModeleVoie[vm].descente2 = ModelePhys[p].descente2;
					ModeleVoie[vm].facteur2 = ModelePhys[p].facteur2;
#endif
					ModeleVoie[vm].min_moyen = 1000.0;
					ModeleVoie[vm].max_moyen = 8000.0;
					vm = ModeleVoieNb++;
					ModeleVoie[vm+1].nom[0] = '\0';
				} else vm = 0;
				ModeleVoie[vm].utilisee = 1;
				//				if(imprime_entetes) printf(": Utilisation d'une voie de type %s\n",ModeleVoie[vm].nom);
				VoieManip[voie].type = vm;
				VoieManip[voie].extens = 0xFFFF - (shex)VoieManip[voie].ADCmask;
				VoieManip[voie].zero = (VoieManip[voie].ADCmask + 1) / 2;
				if(version_lue >= 7.0) {
					if(VoieManip[voie].gain_fixe == 0.0) VoieManip[voie].gain_fixe = 1.0; /* V7.3 et avant */
					if(VoieManip[voie].nVenkeV <= 0.0) VoieManip[voie].nVenkeV = 1.0;
					nom_bolo = ItemSuivant(&c,' ');
					for(bolo=0; bolo<BoloNb; bolo++) {
						if(!strcmp(Bolo[bolo].nom,nom_bolo)) {
							VoieManip[voie].det = bolo;
							if((cap = Bolo[bolo].nbcapt) < DETEC_CAPT_MAX) {
								Bolo[bolo].captr[cap].voie = voie;
								// printf("  . Bolo[%d].voie[%d] = #%d\n",bolo,cap,voie);
								Bolo[bolo].nbcapt += 1;
							}
							break;
						}
					}
				}
				VoiesNb++;
				VoiesLues = VoiesNb;
			} else {
				if(imprime_actions) printf(". Cette voie est excedentaire (maxi: %d) et ne sera pas memorisee\n",VoiesGerees);
			}

		} else if(!strncmp(ligne,"* Filtre",8)) { /* <voie> est la derniere voie lue */
			read(p,&nb,sizeof(int));
#ifndef CODE_WARRIOR_VSN
			if(byteswappe) ByteSwappeInt((hexa *)&nb);
#endif
			filtre.nbetg = nb;
			for(etage=0; etage<filtre.nbetg; etage++) {
				read(p,&nb,sizeof(int));
#ifndef CODE_WARRIOR_VSN
				if(byteswappe) ByteSwappeInt((hexa *)&nb);
#endif
				//				if(voie < 2) printf("Filtre %s: %d etages\n",VoieManip[voie].nom,nb);
				filtre.etage[etage].nbdir = nb;
				for(i=0; i<nb; i++)
					read(p,&(filtre.etage[etage].direct[i]),sizeof(double));
				if(byteswappe) ByteSwappeDoubleArray(filtre.etage[etage].direct,nb);
				read(p,&nb,sizeof(int));
#ifndef CODE_WARRIOR_VSN
				if(byteswappe) ByteSwappeInt((hexa *)&nb);
#endif
				filtre.etage[etage].nbinv = nb;
				for(i=0; i<nb; i++)
					read(p,&(filtre.etage[etage].inverse[i]),sizeof(double));
				if(byteswappe) ByteSwappeDoubleArray(filtre.etage[etage].inverse,nb);
			}
			if(!VoieEvent[voie].filtre) VoieEvent[voie].filtre = (TypeFiltre *)malloc(sizeof(TypeFiltre));
			if(VoieEvent[voie].filtre) memcpy(VoieEvent[voie].filtre,&filtre,sizeof(TypeFiltre));


		} else if(!strncmp(ligne,"* Run",5)) {
			if(imprime_actions) {
                if(imprime_entetes) printf("\n");
                printf("- Lecture de l'entete de run\n");
            }
			strncpy(nom,SrceName,80);
			ArgFromPath(p,ArchHdrRun,LIMITE_ENTETE);
			strncpy(SrceName,nom,80);
			/* if(!strncmp(Acquis[AcquisLocale].code,"EDWACQ-",7)) {
				strcpy(nom,Acquis[AcquisLocale].code+7);
				strncpy(Acquis[AcquisLocale].code,nom,CODE_MAX);
			}
			if(!strncmp(Starter,"EDWACQ-",7)) {
				strcpy(nom,Starter+7);
				strcpy(Starter,nom);
			} */
			Acquis[AcquisLocale].runs[0] = 's';
			if(imprime_entetes) ArgPrint("*",ArchHdrRun);

		} else if(!strncmp(ligne,"* Donnees",9)) break;

		else {
			lngr = strlen(ligne); if(ligne[lngr - 1] == '\n') ligne[lngr - 1] = '\0';
			printf(". Ligne abandonnee (%s)\n",ligne);
		}

	} forever;

	/* si on a declare un bloc global, il doit etre mis en fin de liste, meme si DetecteurStandardLocal.taille == 0 */
	ReglageRazBolo(Bolo+BoloNb,"Standard"); DetecteurAjouteStd();

	if(imprime_entetes) SambaDumpBolos(); else if(imprime_actions) printf("\n");
	return(1);
}
/* ========================================================================== */
static int ArchDemarre(TypeRepartiteur *repart, int l) {
	int bolo,voie;
	long pos;
	char type; char imprime;
	char stream,boostee;
	char raison[256];

	pos = 0; // GCC
	if(repart->r.f.fmt == ARCH_FMT_EDW0) {
	#ifdef A_REVOIR_PEUT_ETRE
		int voie,vt,maxbuffer,parm;
		VoiesLues = 2;
		BoloGeres = 1;
		ArchVoieStatus[0].arch.a_lire = 8192;   /* Longueur de bloc dans la voie chaleur    */
		ArchVoieStatus[1].arch.a_lire = 2048;   /* Longueur de bloc dans la voie ionisation */
		repart->s.f.lu = 0;                  /* Nombre de blocs lus (= nombre d'evenements) [LectArchLus] */
		maxbuffer = 0;
		for(vt=0; vt<VoiesLues; vt++) {
			voie = SambaEchantillon[vt].voie = vt;
			if(maxbuffer < ArchVoieStatus[vt].arch.a_lire) maxbuffer = ArchVoieStatus[vt].arch.a_lire;
		}
		if(maxbuffer == 0) {
			OpiumError("Tampon de longueur 0!");
			if(repart->p.f.a[l].o.file) { fclose(repart->p.f.a[l].o.file); repart->p.f.a[l].o.file = 0; }
			if(repart->p.f.a[l].o.path >= 0) { close(repart->p.f.a[l].o.path); repart->p.f.a[l].o.path = -1; }
			return(0);
		}

		for(vt=0; vt<VoiesLues; vt++) {
			ArchVoieStatus[vt].arch.lus = 0;
			parm = maxbuffer / ArchVoieStatus[vt].arch.a_lire;
			ArchVoieStatus[vt].arch.loop = ArchVoieStatus[vt].arch.step = parm;
			voie = vt;
			VoieManip[voie].def.trmt[TRMT_AU_VOL].p1 = parm;
			// if(parm > 1) VoieManip[voie].def.trmt[TRMT_AU_VOL].type = TRMT_LISSAGE; else
			VoieManip[voie].def.trmt[TRMT_AU_VOL].type = NEANT;
		}
		repart->s.f.mode = 1;
	#endif /* A_REVOIR_PEUT_ETRE */

	} else if(repart->r.f.fmt == ARCH_FMT_EDW2) {
		imprime = (repart->p.f.a[l].partition == 0);
		if(imprime) printf("\n= Relecture d'une archive Samba\n");
		type = repart->r.f.fmt;
		stream = ArchModeStream; boostee = SettingsMultitasks;
		ArchModeStream = ARCH_BLOC; /* valeur par defaut, car absent de l'entete avant V6.6 */
		if(!ArchEntete(repart,l,raison)) {
			printf("! %s: lecture abandonnee\n",raison);
			OpiumFail("Relecture impossible (%s)",raison);
			return(0);
		}
        repart->p.f.a[l].d.of7 = lseek(repart->p.f.a[l].o.path,0,SEEK_CUR);
		repart->p.f.a[l].started = 1;
		pos = (long)repart->p.f.a[l].d.of7;
		for(voie=0; voie<VoiesNb; voie++) {
			printf(". Voie lue #%d: '%s'\n",voie+1,VoieManip[voie].nom);
			VoieTampon[voie].lue = 1;
		}
		VoieIdentMax = VoiesNb;
		LectSurFichierDejaVu = 1;
		//- if(Version >= 6.3) CmdeDef[RESS_D2].valeur = Diviseur2;
#ifdef OBSOLETE
		for(vt=0; vt<SambaEchantillonLngr; vt++) if((voie = SambaEchantillon[vt].voie) >= 0) {
			ArchVoieStatus[vt].arch.loop = ArchVoieStatus[vt].arch.step = VoieTampon[voie].trmt[TRMT_AU_VOL].compactage;
			ArchVoieStatus[vt].arch.a_lire = 0; ArchVoieStatus[vt].arch.lus = 0;
			// printf("(LectAcqStd) loop[%d] = step = %d\n",voie,ArchVoieStatus[vt].arch.step);
		}
#endif
		for(bolo=0; bolo<BoloNb; bolo++) Bolo[bolo].a_lire = 1;
		repart->s.f.mode = (ArchModeStream == ARCH_BLOC)? 1: 0;
		ArchModeStream = stream; SettingsMultitasks = boostee;
	}

	if(repart->p.f.bin) {
		if(repart->p.f.a[l].o.path >= 0) printf("  . Fichier de type 'path' positionne initialement a l'octet %04lx\n",(lhex)pos);
		else printf("  * Fichier PAS ENCORE OUVERT\n");
	} else {
		if(repart->p.f.a[l].o.file) printf("  . Fichier de type 'FILE' positionne initialement a l'octet %04lx\n",ftell(repart->p.f.a[l].o.file));
		else printf("  * Fichier PAS ENCORE OUVERT\n");
	}
	return(1);
}
/* ========================================================================== */
static char ArchRewind(TypeRepartiteur *repart, int l, char en_cours) {
	int pos; long off; off_t of7; char ouvert;

	ouvert = 0;
	if(repart->p.f.bin) {
		if(repart->p.f.a[l].o.path >= 0) {
			if(en_cours) lseek(repart->p.f.a[l].o.path,repart->p.f.a[l].d.of7,SEEK_SET);
			else lseek(repart->p.f.a[l].o.path,0,SEEK_SET);
			of7 = lseek(repart->p.f.a[l].o.path,0,SEEK_CUR); pos = (int)of7;
			ouvert = 1;
		}
	} else {
		if(repart->p.f.a[l].o.file) {
			if(en_cours) fseek(repart->p.f.a[l].o.file,repart->p.f.a[l].d.off,SEEK_SET);
			else rewind(repart->p.f.a[l].o.file); /* efface l'indicateur d'erreurs */
			off = ftell(repart->p.f.a[l].o.file); pos = (int)off;
			ouvert = 1;
		}
	}
	if(ouvert) printf("%s/ Fichier %s positionne a l'octet %04x\n",DateHeure(),(char *)repart->entree[l].adrs,pos);
	else printf("%s/ ! Le fichier %s n'est pas ouvert\n",DateHeure(),(char *)repart->entree[l].adrs);
	return(ouvert);
}
/* ========================================================================== */
static void ArchRecommence(TypeRepartiteur *repart) {
	int l;

	for(l=0; l<repart->nbentrees; l++) {
		if(repart->p.f.a[l].partition == 0) ArchRewind(repart,l,1);
		else {
			repart->p.f.a[l].partition = 0;
			ArchOuvre(repart,l);
			ArchDemarre(repart,l);
		}
	}
}
/* ========================================================================== */
static INLINE char ArchLitBloc(TypeRepartiteur *repart, char *pas_tous_secs, short *pleins, char log) {
	int l,nb,attendu,lu;

	errno = 0;
	for(l=0; l<repart->nbentrees; l++) {
		nb = FIFOdim - repart->top;
		if(nb <= 0) break;
		if(repart->nbentrees > 1) nb = 1;
		attendu = nb * sizeof(TypeDonnee);
		lu = 0; //? lu = attendu;
		if(repart->p.f.bin) {
			if(repart->p.f.a[l].o.path >= 0) lu = read(repart->p.f.a[l].o.path,repart->data16 + repart->top,attendu); // LectArchPath
		} else {
			if(repart->p.f.a[l].o.file) lu = fread(repart->data16 + repart->top,sizeof(TypeDonnee),nb,repart->p.f.a[l].o.file) * sizeof(TypeDonnee); // LectArchFile
		}
		if(lu != attendu) {
			if(repart->p.f.bin) {
				if(repart->p.f.a[l].o.path >= 0) printf("%s/  read(p=%x,nb=%d): ",DateHeure(),repart->p.f.a[l].o.path,nb);
				else printf("%s/ ?read(p=x,nb=%d): ",DateHeure(),nb);
			} else {
				if(repart->p.f.a[l].o.file)  printf("%s/ fread(f=%x,nb=%d): ",DateHeure(),(hexa)repart->p.f.a[l].o.file,nb);
				else printf("%s/ ?read(f=x,nb=%d): ",DateHeure(),nb);
			}
			if(errno) printf("erreur %d (%s)\n",errno,strerror(errno));
			else {
				printf("fin de fichier trouvee\n");
				repart->p.f.a[l].partition += 1;
				if(ArchOuvre(repart,l)) ArchDemarre(repart,l);
				else return(0);
			}
			repart->s.f.terminee = 1;
		}
		if(lu >= 0) nb = lu / sizeof(TypeDonnee); else nb = 0;
		if(!BigEndianOrder) ByteSwappeWordArray((shex *)(repart->data16 + repart->top),nb);
		repart->top += nb;
		repart->donnees_recues += nb;
	}

	return(1);
}
#pragma mark ----------- NI -----------
/* ========================================================================== */
static void NiTransmissionRaz(int largeur_table) {
#ifdef NIDAQ
	char nom_tache[80]; int nbcols,num;

	nbcols = largeur_table? 5: 0;
	num = repart->adrs.val - 1;
	if(LectNIactive[num]) SambaTesteDAQmx("Arret du DMA",DAQmxBaseStopTask(LectNITache[num]),nbcols);
	LectNIactive[num] = 0;
	if(LectNITache[num]) SambaTesteDAQmx("Effacement du DMA",DAQmxBaseClearTask(LectNITache[num]),nbcols);
	LectNITache[num] = 0;
	sprintf(nom_tache,"samba_%02d",num+1);
	//	printf("            . Creation de la tache de lecture '%s'\n",nom_tache);
	if(largeur_table) { l = printf("| Creation du DMA '%s'",nom_tache); SambaCompleteLigne(largeur_table,l); }
	SambaTesteDAQmx(0,DAQmxBaseCreateTask(nom_tache,&(LectNITache[num])),nbcols);
#endif
}
/* ========================================================================== */
int NiSelecte1voie(TypeRepartiteur *repart, int vbb, int largeur_table) {
	int nbdata = 0;
#ifdef NIDAQ
	char nom_canal[12],nom_source[16],nom_polar[16]; char mode; int k;
	int32 source; float64 polar; /* volts */

	sprintf(nom_canal,"%s/ai%d",repart->nom+3,vbb);
	polar = ModeleADC[ModeleBN[type_bn].adc[vbb]].polar;
	sprintf(nom_polar,"polar-v%d",vbb+1);
	if((k = NumerModlRessIndex(&(ModeleBN[type_bn]),nom_polar)) >= 0) {
		if(ArgItemLu(numeriseur->desc,k)) polar = numeriseur->ress[k].val.r;
	}
	mode = 0;
	sprintf(nom_source,"source-v%d",vbb+1);
	if((k = NumerModlRessIndex(&(ModeleBN[type_bn]),nom_source)) >= 0) {
		if(ArgItemLu(numeriseur->desc,k)) mode = numeriseur->ress[k].val.i;
	}
	if(largeur_table) {
		l = printf("| Connexion du canal de lecture %s a +/- %g Volts",
				   nom_canal,polar);
		SambaCompleteLigne(largeur_table,l);
		l = printf("|   en mode %s",mode?"floating source [FS]":"ground-ref source (ref. single-ended) [GS]");
		SambaCompleteLigne(largeur_table,l);
	}
	if(mode) source = DAQmx_Val_Diff; else source = DAQmx_Val_RSE;
	SambaTesteDAQmx(0,DAQmxBaseCreateAIVoltageChan(LectNITache[num],nom_canal,NULL,source,-polar,polar,DAQmx_Val_Volts,NULL),nbcols);
	nbdata = (int)Echantillonnage;
	if((k = NumerModlRessIndex(&(ModeleBN[type_bn]),"bloc")) >= 0) {
		if(ArgItemLu(numeriseur->desc,k)) nbdata = numeriseur->ress[k].val.i;
	}
#endif
	return(nbdata);
}
/* ========================================================================== */
static void NiAjusteHorloge(TypeRepartiteur *repart, int bloc, int nbvoies, int largeur_table) {
#ifdef NIDAQ
	uInt32 dim_dma;
	if(largeur_table) { l = printf("| Echantillonnage a %g kHz continu par blocs de %d",Echantillonnage,bloc); SambaCompleteLigne(largeur_table,l); }
	SambaTesteDAQmx(0,DAQmxBaseCfgSampClkTiming(LectNITache[num],"OnboardClock",(float64)Echantillonnage*1000.0*(float64)nbvoies,DAQmx_Val_Rising,DAQmx_Val_ContSamps,bloc),nbcols);
	dim_dma = (uInt32)(3.0 * (float64)Echantillonnage * 1000.0 * (float64)n); /* timeout=3 secondes, voir DAQmxBaseReadBinaryI16 */
	if(largeur_table) { l = printf("| %d echantillons dans le tampon de transfert DMA",dim_dma); SambaCompleteLigne(largeur_table,l); }
	SambaTesteDAQmx(0,DAQmxBaseCfgInputBuffer(LectNITache[num],dim_dma),nbcols);
#endif
}
/* ========================================================================== */
static INLINE char NiVidePile(TypeRepartiteur *repart, char *pas_tous_secs, short *pleins, char log) {
#ifdef NIDAQ
	int nb;

	if(repart->top == 0) {
		int max;
		max = FIFOdim - repart->top; nb = repart->octets;
		repart->acces_demandes++;
		/* tache de recuperation des donnees (le mode DAQmx_Val_GroupByChannel eviterait la redistribution par voie) */
		DAQmxBaseReadBinaryI16(LectNITache[repart->adrs.val-1],DAQmx_Val_Auto,2.0,DAQmx_Val_GroupByScanNumber,
							   (int16 *)(repart->data16 + repart->top),max,&nb,NULL);
		if(nb) {
			repart->acces_remplis++;
			repart->top += nb;
			repart->donnees_recues += nb;
			*pas_tous_secs = 1;
		}
		// printf("t=%12lld: lecture #%04lld, %6d/%-6d donnees recues  (total=%lld)\n",t1,repart->acces_demandes,nb,max,repart->donnees_recues);
	} // else printf("t=%12lld: top=%d\n",t1,repart->top);
#endif
	return(1);
}
/* ========================================================================== */
char NiDepile1Donnee(TypeRepartiteur *repart, lhex *donnee) {
	if((repart->bottom < repart->top) && (repart->bottom < (repart->octets / sizeof(Ulong)))) {
		*donnee = (lhex)repart->data16[repart->bottom++];
		repart->depile += 1;
		if(repart->bottom >= repart->top) repart->top = repart->bottom = 0;
	} else {
		repart->top = repart->bottom = 0;
		return(0);
	}
	return(1);
}
#pragma mark ----------- IPE -----------
/* ========================================================================== */
static void IpeCompteursTrame(TypeRepartiteur *repart) {
	int donnees_trame;

	if(!repart) return;
	repart->dim.totale = sizeof(TypeIpeTrame);
	repart->dim.entete = IPE_TAILLE_ENTETE;
	repart->s.ip.num_sts = REPART_IP_CNTL;
	if(repart->nbdonnees == 0) repart->nbdonnees = 1;
	repart->s.ip.duree_trame = IPE_DONNEES_NB / repart->nbdonnees; // nombre entier d'echantillons dans une trame
	if(SettingsTramesMaxi) donnees_trame = IPE_DONNEES_NB; // si trames maximales, echantillons a cheval
	else donnees_trame = repart->s.ip.duree_trame * repart->nbdonnees;
	repart->dim.trame = (donnees_trame * sizeof(TypeDonnee)) + IPE_TAILLE_ENTETE;
	repart->s.ip.stampD3 = (int64)100000;
	repart->s.ip.inc_stamp = repart->s.ip.stampD3;
	if(SettingsTramesMaxi)
		repart->s.ip.tramesD3 = (((int)repart->s.ip.inc_stamp - 1) * repart->nbdonnees) / IPE_DONNEES_NB; // si trames maximales donc echantillons a cheval
	else repart->s.ip.tramesD3 = ((int)repart->s.ip.inc_stamp - 1) / repart->s.ip.duree_trame;
	repart->s.ip.taille_derniere = (int)(repart->s.ip.stampD3 - (donnees_trame * repart->s.ip.tramesD3 / repart->nbdonnees));
	repart->s.ip.dim_D3 = (int)repart->s.ip.inc_stamp * repart->nbdonnees;
	repart->gene.intervalle = (int64)(1000.0 * (float)(repart->s.ip.duree_trame) / Echantillonnage);
}
#ifdef IPE_MESSAGES_V2
/* ========================================================================== */
int IpeSelectCanauxV2(TypeRepartiteur *repart, int nbregs, byte *selection, float echantillonage, byte *buffer, int *t) {
	int i,j,n;

	if(!RepartIpEcriturePrete(repart,"  ")) { *t = 0; return(0); }
	j = 0;
	buffer[j++] = IPE_DEST_FLT;
	buffer[j++] = IPE_TYPE_SEL;
	buffer[j++] = (byte)nbregs;
	for(i=0; i<nbregs; i++) {
		buffer[j++] = repart->r.ipe.modeBB[i];
		buffer[j++] = selection[i];
	}
	*t = j;
	n = RepartIpEcrit(repart,buffer,*t); SambaMicrosec(ModeleRep[repart->type].delai_msg);
	return(n);
}
/* ========================================================================== */
static void IpeCmdeSltV2(TypeRepartiteur *repart) {
	byte buffer[8]; int i,l;

	if(RepartIpEcriturePrete(repart,"            ")) {
		l = 0;
		buffer[l++] = // IPE_DEST_SLT: IPE_MESSAGES_V2
		buffer[l++] = repart->r.ipe.retard;
		/* ... */
		RepartIpEcrit(repart,buffer,l); SambaMicrosec(ModeleRep[repart->type].delai_msg);
		printf("            . Ecrit: %c",buffer[0]); for(i=1; i<l; i++) printf(".%02X",buffer[i]);
		printf(" (retard=%d)\n",repart->r.ipe.retard);
	} else if(repart) printf("          ! Parametrage de %s impossible: pas de chemin d'ecriture\n",repart->nom);
	else printf("          ! Repartiteur indefini\n");
	RepartiteurParmModifie = 0;
}
#endif /* IPE_MESSAGES_V2 */
#ifdef IPE_MESSAGES_V3
/* ==========================================================================
static void IpeCmdeSlt(TypeRepartiteur *repart, hexa reg, hexa val) {
	byte texte[80]; int k,l;

	if(RepartIpEcriturePrete(repart,"            ")) {
		l = sprintf((char *)texte,"%s_0x%08X_0x%08X",IPE_REG_WRITE,reg,val); texte[l] = '\0';
		k = RepartIpEcrit(repart,texte,l); SambaMicrosec(ModeleRep[repart->type].delai_msg);
		printf("            . Ecrit: %s (transmis: %d/%d)\n",texte,k,l);
	} else if(repart) printf("          ! Parametrage de %s impossible: pas de chemin d'ecriture\n",repart->nom);
	else printf("          ! Repartiteur indefini\n");
	RepartiteurParmModifie = 0;
}
   ==========================================================================
 static char IpeEcritConfig(TypeRepartiteur *repart, int flt, char *reg, hexa val, char log) {
 char texte[80]; int k,l;

 if(!repart) return(0);
 l = sprintf(RepartiteurValeurEnvoyee,"%s_%s(%d): 0x%08X",IPE_SLT_CONFG,reg,IPE_FLT_ABS(repart,flt),val); texte[l] = '\0';
 if(repart->ecr.ip.path >= 0) {
 k = RepartIpEcrit(repart,RepartiteurValeurEnvoyee,l); SambaMicrosec(ModeleRep[repart->type].delai_msg);
 } else k = 0;
 if(log) printf("%s/   . Ecrit: %s <- %s [%d/%d]\n",DateHeure(),repart->nom,RepartiteurValeurEnvoyee,k,l);
 return(k == l);
 }
   ==========================================================================
static int IpeHorloges(TypeRepartiteur *repart, float echantillonage, byte *buffer, int *l, char *explics) {
	int modl; int i,n; shex d0,d1;

	explics[0] = '\0';
	if(!repart) return(0);
	if(!RepartIpEcriturePrete(repart,"            ")) { *l = 0; return(0); }
	modl = repart->type;
	d1 = 60;
	d0 = (shex)((ModeleRep[modl].horloge * 1000.0 / (echantillonage * (float)d1)) + 0.5);
	buffer[0] = 'h';
	buffer[1] = _FPGA_code_horloge;
	buffer[2] = (byte)(d0 & 0xFF);
	buffer[3] = (byte)((d0 >> 8) & 0xFF);
	buffer[4] = 0;
	buffer[5] = 0x03;
	buffer[6] = code_acqui_EDW_BB2;
	buffer[7] = code_synchro_100000;
	if(repart->r.ipe.clck) buffer[7] += (code_synchro_esclave_temps + code_synchro_esclave_synchro);
	*l = 8;
	n = RepartIpEcrit(repart,buffer,*l); SambaMicrosec(ModeleRep[modl].delai_msg);
	printf("            . Ecrit: %c",buffer[0]);
	for(i=1; i<*l; i++) printf(".%02X",buffer[i]);
	return(n);
}
   ========================================================================== */
static int IpeCmdeH(TypeRepartiteur *repart) {
	byte buffer[8]; int i,l;

	if(pRepartStatus && OpiumDisplayed(pRepartStatus->cdr)) PanelRefreshVars(pRepartStatus);

	if(RepartIpEcriturePrete(repart,"            ")) {
		buffer[0] = 'h';
		buffer[1] = _FPGA_code_horloge;
		buffer[2] = (byte)(repart->r.ipe.d0 & 0xFF);          /* soit frequence a 100 MHz/5/18 = 1,1 MHz */
		buffer[3] = (byte)((repart->r.ipe.d0 >> 8) & 0xFF);
		buffer[4] = 0;                 /* retard du aux fibres */
		buffer[5] = 0x03;                 /* masque BB (nb.mots 32 bits par 3 bits, soit b'010010 si 2 BB de 3 voies */
		buffer[6] = code_acqui_EDW_BB2;                       /* code acquisition */
		buffer[7] = code_synchro_100000; /* code synchro */
		if(repart->r.ipe.clck) buffer[7] += (code_synchro_esclave_temps + code_synchro_esclave_synchro);
		l = 8;
		RepartIpEcrit(repart,buffer,l); SambaMicrosec(ModeleRep[repart->type].delai_msg);
		printf("            . Ecrit: %c",buffer[0]);
		for(i=1; i<l; i++) printf(".%02X",buffer[i]);
		printf(" (d0=%d, retard=%d, mode %s, horloge %s)\n",
			   repart->r.ipe.d0,0,OperaCodesAcquis[code_acqui_EDW_BB2],
			   repart->r.ipe.clck? "externe":"interne");
	} else if(repart) printf("          ! Parametrage de %s impossible: pas de chemin d'ecriture\n",repart->nom);
	else printf("          ! Repartiteur indefini\n");
	RepartiteurParmModifie = 0;
	return(0);
}
/* ========================================================================== */
static char IpeEcritCommande(TypeRepartiteur *repart, byte *texte, byte *buffer, int *t, int *n) {
	int j,k,l; char ok;

	if(!repart) return(0);
	l = strlen((char *)texte); j = *t;
	k = RepartIpEcrit(repart,texte,l); SambaMicrosec(ModeleRep[repart->type].delai_msg);
	ok = (k == l);
	strcat((char *)buffer,"/"); j++; buffer[j] = '\0';
	if(k > 0) *n += k;
	if(!ok) { strcat((char *)buffer,"(!) "); j += 4; buffer[j] = '\0'; k = l; }
	strncat((char *)buffer,(char *)texte,k); j += k; buffer[j] = '\0';
	*t = j;
	return(ok);
}
/* ========================================================================== */
char IpeEcritFlt(TypeRepartiteur *repart, int flt, int reg, hexa val, char log) {
	char texte[80]; int adrs,k,l;

	if(!repart) return(0);
	adrs = reg; if(BigEndianOrder) ByteSwappeInt((hexa *)&adrs);
	l = sprintf(RepartiteurValeurEnvoyee,"%s_0x%02X_0x%08X_0x%08X",IPE_FLT_WRITE,IPE_FLT_ABS(repart,flt)+1,adrs,val); texte[l] = '\0';
	if(repart->ecr.ip.path >= 0) {
		k = RepartIpEcrit(repart,RepartiteurValeurEnvoyee,l); SambaMicrosec(ModeleRep[repart->type].delai_msg);
	} else k = 0;
	if(log > 0) printf("%s/   . Ecrit: %s <- %s [%d/%d]\n",DateHeure(),repart->nom,RepartiteurValeurEnvoyee,k,l);
	else if(log < 0) printf("              . Ecrit: %s <- %s [%d/%d]\n",repart->nom,RepartiteurValeurEnvoyee,k,l);
	return(k == l);
}
/* ========================================================================== */
INLINE void IpeAutoriseFibre(TypeRepartiteur *repart, int entree, char autorise) {
	int flt,fbr;

	if(!repart) return;
	flt = IPE_FLT_NUM(entree); fbr = IPE_FLT_FBR(entree,flt);
// #define TEMPORAIRE
#ifdef TEMPORAIRE
	if(autorise) repart->r.ipe.flt[flt].fibres_fermees = 0x00;
	else repart->r.ipe.flt[flt].fibres_fermees = 0x3F;
#else
	if(autorise) repart->r.ipe.flt[flt].fibres_fermees &= ~(1 << fbr);
	else repart->r.ipe.flt[flt].fibres_fermees |= (1 << fbr);
#endif
	IpeEcritFlt(repart,flt,IPE_FLT_ACCES_BB,repart->r.ipe.flt[flt].fibres_fermees,1);
	 // SambaMicrosec(ModeleRep[repart->type].delai_msg);
}
/* ========================================================================== */
static void IpeSetupFibres(TypeRepartiteur *repart) {
	int flt,fbr;

	for(flt=0; flt<repart->r.ipe.fltnb; flt++) if(repart->r.ipe.flt[flt].enabled) {
		repart->r.ipe.flt[flt].controle.partiel.enable &= 0xC0;
		repart->r.ipe.flt[flt].controle.partiel.enable |= 0x3F;
		repart->r.ipe.flt[flt].controle.partiel.modeBB &= 0xC0;
		for(fbr=0; fbr<IPE_FLT_SIZE; fbr++) if(repart->r.ipe.modeBB[IPE_ENTREE(flt,fbr)] == IPE_BB1) {
			repart->r.ipe.flt[flt].controle.partiel.modeBB = (repart->r.ipe.flt[flt].controle.partiel.modeBB | (1 << fbr));
		}
		IpeEcritFlt(repart,flt,IPE_FLT_CONTROL,repart->r.ipe.flt[flt].controle.complet,-1);
		repart->r.ipe.flt[flt].fibres_fermees = 0x3F;
		IpeEcritFlt(repart,flt,IPE_FLT_ACCES_BB,repart->r.ipe.flt[flt].fibres_fermees,-1);
	}
}
/* ========================================================================== */
int IpeSelectCanaux(TypeRepartiteur *repart, int nbregs, byte *selection, float echantillonage, byte *buffer, int *t) {
	int i,j,k,l,m,n,o,flt,fbr,fltabs; shex d0,d1; byte texte[80]; char en_finale,louppe;
	hexa pixbus,masque,complet,bit; TypeNumeriseur *numeriseur;
	TypeIpeFibreReg delais[IPE_FIBRE_REGS],selecteur[IPE_FIBRE_REGS];

	if(!repart) return(0);
	if(!RepartIpEcriturePrete(repart,"            ")) { *t = 0; return(0); }
	n = 0; j = 0; louppe = 0;

	d1 = 60;
	d0 = (shex)((ModeleRep[repart->type].horloge * 1000.0 / (echantillonage * (float)d1)) + 0.5);
	texte[0] = 'h'; texte[1] = _FPGA_code_horloge; texte[2] = (byte)(d0 & 0xFF); texte[3] = (byte)((d0 >> 8) & 0xFF);
	texte[4] = 0; texte[5] = 0x03; texte[6] = code_acqui_EDW_BB2; texte[7] = code_synchro_100000;
	if(repart->r.ipe.clck) buffer[7] += (code_synchro_esclave_temps + code_synchro_esclave_synchro);
	l = 8; texte[l] = '\0';
	k = RepartIpEcrit(repart,texte,l); SambaMicrosec(ModeleRep[repart->type].delai_msg);
	louppe = (k != l);
	if(k > 0) n += k;
	k = sprintf((char *)buffer,"%s%c",louppe?"(!) ":"",texte[0]); j += k;
	for(i=1; i<l; i++) { k = sprintf((char *)buffer+j,".%02X",texte[i]); j += k; }; buffer[j] = '\0';
	*t = j;

	if(repart->r.ipe.fifo < 0) {
		pixbus = 0; bit = 1;
		for(flt=0; flt<repart->r.ipe.fltnb; flt++) {
			if(repart->r.ipe.flt[flt].active) pixbus |= bit;
			bit = (bit << 1);
		}
		l = sprintf((char *)texte,"%s_0x%08X_0x%08X",IPE_REG_WRITE,IPE_SLT_PIXBUS,pixbus); texte[l] = '\0';
		if(!IpeEcritCommande(repart,texte,buffer,t,&n)) louppe = 1;
	}

	for(flt=0; flt<repart->r.ipe.fltnb; flt++) if(!(repart->r.ipe.flt[flt].active) && repart->r.ipe.flt[flt].enabled) {
		repart->r.ipe.flt[flt].controle.partiel.modeBB &= 0xC0;
		repart->r.ipe.flt[flt].controle.partiel.enable &= 0xC0;
		complet = repart->r.ipe.flt[flt].controle.complet; if(BigEndianOrder) ByteSwappeInt(&complet);
		l = sprintf((char *)texte,"%s_0x%02X_0x%08X_0x%08X",IPE_FLT_WRITE,IPE_FLT_ABS(repart,flt)+1,IPE_FLT_CONTROL,complet); texte[l] = '\0';
		if(!IpeEcritCommande(repart,texte,buffer,t,&n)) louppe = 1;
	}
	for(flt=0; flt<repart->r.ipe.fltnb; flt++) if(repart->r.ipe.flt[flt].active) {
		masque = 0; bit = 1;
		if(LectLitStatus || (Archive.enservice && LectSauveStatus)) for(fbr=0; fbr<IPE_FLT_SIZE; fbr++) {
			if((numeriseur = (TypeNumeriseur *)(repart->entree[IPE_ENTREE(flt,fbr)].adrs))) {
				if(numeriseur->avec_status) masque |= bit;
			}
			bit = (bit << 1);
		}
		l = sprintf((char *)texte,"%s_%s(%d): 0x%08X",IPE_SLT_CONFG,IPE_BBSTS_ENABLE,IPE_FLT_ABS(repart,flt),masque); texte[l] = '\0';
		if(!IpeEcritCommande(repart,texte,buffer,t,&n)) louppe = 1;
	}

	fbr = 0;
	for(i=0; i<nbregs; i++) {
		flt = IPE_FLT_NUM(i); fltabs = IPE_FLT_ABS(repart,flt);
		en_finale = (repart->r.ipe.flt[flt].active)? selection[i]: 0;
		if(repart == RepartConnecte) {
			IpeStatus[flt][fbr].delai12 = repart->r.ipe.retard[i] & 0xF;
			IpeStatus[flt][fbr].delai120 = (repart->r.ipe.retard[i] >> 4) & 0xF;
			IpeStatus[flt][fbr].selection = en_finale;
		}
		if(fbr == 0) {
			repart->r.ipe.flt[flt].controle.partiel.modeBB &= 0xC0;
			repart->r.ipe.flt[flt].controle.partiel.enable &= 0xC0;
			for(m=0; m<IPE_FIBRE_REGS; m++) { delais[m].total = selecteur[m].total = 0; }
		}
		m = fbr / IPE_FIBRES_PARMOT; o = fbr - (m * IPE_FIBRES_PARMOT); if(BigEndianOrder) o = IPE_FIBRES_PARMOT - 1 - o;
		// delais[m].fibre[o] = IpeDelais[repart->r.ipe.modeBB[i]];
		delais[m].fibre[o] = repart->r.ipe.retard[i];
		selecteur[m].fibre[o] = en_finale;

		if(en_finale) repart->r.ipe.flt[flt].controle.partiel.enable = (repart->r.ipe.flt[flt].controle.partiel.enable | (1 << fbr));
		if(repart->r.ipe.modeBB[i] == IPE_BB1) repart->r.ipe.flt[flt].controle.partiel.modeBB = (repart->r.ipe.flt[flt].controle.partiel.modeBB | (1 << fbr));
		if(++fbr == IPE_FLT_SIZE) {
			complet = repart->r.ipe.flt[flt].controle.complet; if(BigEndianOrder) ByteSwappeInt(&complet);
			l = sprintf((char *)texte,"%s_0x%02X_0x%08X_0x%08X",IPE_FLT_WRITE,fltabs+1,IPE_FLT_CONTROL,complet); texte[l] = '\0';
			if(!IpeEcritCommande(repart,texte,buffer,t,&n)) louppe = 1;
			for(m=0; m<IPE_FIBRE_REGS; m++) {
				l = sprintf((char *)texte,"%s_0x%02X_0x%08X_0x%08X",IPE_FLT_WRITE,fltabs+1,IPE_FLT_DELAIS+(4*m),delais[m].total); texte[l] = '\0';
				if(!IpeEcritCommande(repart,texte,buffer,t,&n)) louppe = 1;
			}
			for(m=0; m<IPE_FIBRE_REGS; m++) {
				l = sprintf((char *)texte,"%s_0x%02X_0x%08X_0x%08X",IPE_FLT_WRITE,fltabs+1,IPE_FLT_SELECT+(4*m),selecteur[m].total); texte[l] = '\0';
				if(!IpeEcritCommande(repart,texte,buffer,t,&n)) louppe = 1;
			}
			fbr = 0;
		}
	}
	if(louppe) { strcat((char *)buffer,"/! = Ecriture en erreur"); *t += 23; buffer[*t] = '\0'; }
	IpeCompteursTrame(repart);
	if(pIpeStatus && OpiumDisplayed(pIpeStatus->cdr)) PanelRefreshVars(pIpeStatus);
	return(n);
}
#endif /* IPE_MESSAGES_V3 */
/* ========================================================================== */
static void IpeBourreFlt(TypeRepartiteur *repart, int flt, char log, int lngr) {
	if(!repart) return;
	if((repart->r.ipe.bourrage == IPE_BOURRE_FLT) && repart->r.ipe.flt[flt].active) {
		int frontiere; int k,vl; char prems;
		frontiere = (((SambaEchantillonLngr - 1) / IPE_BOURRE_QUANTUM) + 1) * IPE_BOURRE_QUANTUM;
		prems = 1;
		vl = SambaEchantillonLngr - repart->premiere_donnee;
		if(frontiere < SAMBA_ECHANT_MAX) while(SambaEchantillonLngr < frontiere) {
			if(log) {
				if(prems) { k = printf("|   --   | %-16s ","neant"); prems = 0; }
				else k = printf("|        | %-16s ","neant");
				k += printf("| %-33s | %-10s ","(0000)","calage");
				SambaLigneTable(' ',k,lngr);
			}
			SambaEchantillon[SambaEchantillonLngr].rep = repart->rep;
			SambaEchantillon[SambaEchantillonLngr].entree = -1;
			SambaEchantillon[SambaEchantillonLngr].voie = -1;
			SambaEchantillonLngr++;
		}
	}
}
/* ========================================================================== */
INLINE static void IpeSwappeTrame(void *buffer, int nb) {
/* projet IPE initial:
	ByteSwappeShortArray((shex *)buffer,2);
	ByteSwappeLong((int64 *)&(((IpeTrame)buffer)->p.donnees.hdr.stamp));
//	ByteSwappeShortArray((shex *)((char *)(((IpeTrame)buffer)->p.donnees.hdr)+sizeof(int64)),sizeof(TypeIpeEntete)-sizeof(int64));
	ByteSwappeShortArray((shex *)((char *)(&(((IpeTrame)buffer)->p.donnees.hdr.nbdetec))),sizeof(TypeIpeEntete)-sizeof(int64));
	ByteSwappeIntArray((hexa *)((IpeTrame)buffer)->p.donnees.val,(nb - sizeof(TypeIpeEntete) - (2 * sizeof(short))) / sizeof(hexa));
*/
	ByteSwappeIntArray((hexa *)buffer,nb / sizeof(hexa));
}
/* ========================================================================== */
static void IpeTraduitProgBB(IpeTrame trame) {
	if(trame->p.sts_crate.micro_bbv2 == 1) strcpy(RepartInfoFpgaBB,"Chargement");
	else if(trame->p.sts_crate.micro_bbv2 == 2) strcpy(RepartInfoFpgaBB,"Ouverture *.rbf");
	else if(trame->p.sts_crate.micro_bbv2 > 0) {
		int stade;
		stade = trame->p.sts_crate.micro_bbv2 >> 8;
		// trame->p.sts_crate.micro_bbv2 & 0xff rend toujours 3
		if(stade <= 100) snprintf(RepartInfoFpgaBB,REPART_INFOFPGA_DIM,"%3d/100 progr. BB#%02d",
			stade,trame->p.sts_crate.micro_bbv2 & 0xff);
		else strcpy(RepartInfoFpgaBB,"Redemarrage");
	} else strcpy(RepartInfoFpgaBB,"inchange");
}
/* ========================================================================== */
void IpeDump(byte *buffer, int nb, char hexa) {
	int i,j,k; long n; shex num,nb_d3; short *court; char deja_vu;
	int64 stamp_slt;
	TypeIpeBBsts *sts_bb;

	if(nb == -1) {
		if(errno == EAGAIN) printf("===== Lecture: tampon vide\n");
		else printf("===== Lecture en erreur (%s)\n",strerror(errno));
	} else {
		printf("===== Lecture de %d octets, ",nb);
		court = 0; // GCC
		if(BigEndianOrder) TwoShortsSwappeInt((unsigned int *)buffer);
		nb_d3 = IPE_TRAME_SYNCHRO_NUM(buffer);
		num = IPE_TRAME_NUMERO(buffer);
		printf("trame No %d.%05d",nb_d3,num);
		sts_bb = 0;
		if(num == IPE_TRAME_STS) {
			printf(" (%04X: crate status):\n",num);
			IpeTraduitProgBB((IpeTrame)buffer);
			stamp_slt = ((int64)(((IpeTrame)buffer)->p.sts_crate.SLTTimeHigh) << 32) | ((int64)(((IpeTrame)buffer)->p.sts_crate.SLTTimeLow) & 0xFFFFFFFF);
			printf("Timestamp            : %08llX (%lld)\n",IPE_TRAME_TIMESTAMP(buffer),IPE_TRAME_TIMESTAMP(buffer));
			printf("Temps (s)            : %08X (%d)\n",((IpeTrame)buffer)->p.sts_crate.temps_seconde,((IpeTrame)buffer)->p.sts_crate.temps_seconde);
			printf("SLTTimeLow           : %08X\n",((IpeTrame)buffer)->p.sts_crate.SLTTimeLow);
			printf("SLTTimeHigh          : %08X (%lld)\n",((IpeTrame)buffer)->p.sts_crate.SLTTimeHigh,stamp_slt);
			printf("Diviseur D0          : %08X (%d)\n",((IpeTrame)buffer)->p.sts_crate.d0,((IpeTrame)buffer)->p.sts_crate.d0 & 0xFFFF);
			printf("PixBus enable        : %08X\n",((IpeTrame)buffer)->p.sts_crate.pixbus_enable);
			printf("Prog.BB              : %08X (%s)\n",((IpeTrame)buffer)->p.sts_crate.micro_bbv2,RepartInfoFpgaBB);
			printf("Error counter        : %08X (%d)\n",((IpeTrame)buffer)->p.sts_crate.error_counter,((IpeTrame)buffer)->p.sts_crate.error_counter);
			printf("Internal error info  : %08X\n",((IpeTrame)buffer)->p.sts_crate.internal_error_info);
			printf("Software status      : %08X\n",((IpeTrame)buffer)->p.sts_crate.ipe4reader_status);
			printf("Software version     : %08X (%d)\n",((IpeTrame)buffer)->p.sts_crate.version,((IpeTrame)buffer)->p.sts_crate.version);
			printf("FIFO & ADC           : %08X (%d FIFO, %d ADC)\n",((IpeTrame)buffer)->p.sts_crate.numFIFOnumADCs,
				(((IpeTrame)buffer)->p.sts_crate.numFIFOnumADCs >> 16) & 0xFFFF,
				((IpeTrame)buffer)->p.sts_crate.numFIFOnumADCs & 0xFFFF);
			printf("Max UDP packet size  : %08X (%d)\n",((IpeTrame)buffer)->p.sts_crate.maxUDPSize,((IpeTrame)buffer)->p.sts_crate.maxUDPSize);
			sts_bb = (TypeIpeBBsts *)((long)&(((IpeTrame)buffer)->p.sts_crate) + (long)(((IpeTrame)buffer)->p.sts_crate.dim));
			n = (long)sts_bb - (long)buffer;
			if((n + 4) > nb) { printf("BB status            : neant\n"); sts_bb = 0; }
			else printf("BB status\n");
		} else if(num == IPE_TRAME_BB) {
			sts_bb = (TypeIpeBBsts *)&(((IpeTrame)buffer)->p.sts_bb);
			printf(" (%04X: BB status)\n",num);
		}
		if(sts_bb) {
			n = 0;
			while(sts_bb->dim) {
				if(BigEndianOrder) {
					ByteSwappeInt(&(sts_bb->source.complete));
					printf("|  Fibre %02d.%d [%d]: Id=%04X, alim=%04X, tempe=%04X, d2=%04X, d3=%04X\n",
						sts_bb->source.b.flt,sts_bb->source.b.fbr,sts_bb->dim,
						sts_bb->val[1],sts_bb->val[0],sts_bb->val[3],sts_bb->val[2],sts_bb->val[5]);
				} else printf("|  Fibre %02d.%d [%d]: Id=%04X, alim=%04X, tempe=%04X, d2=%04X, d3=%04X\n",
					sts_bb->source.b.flt,sts_bb->source.b.fbr,sts_bb->dim,
					sts_bb->val[0],sts_bb->val[1],sts_bb->val[2],sts_bb->val[3],sts_bb->val[4]);
				if(sts_bb->dim) sts_bb->dim = 128;
				sts_bb = (TypeIpeBBsts *)((long)sts_bb + (long)(sts_bb->dim));
				n = (long)sts_bb - (long)buffer;
				if((n + 4) > nb) break;
			}
			if((n + 4) > nb) printf("|_ Fin du tampon __\n"); else printf("|_ Status vide ____\n");
		} else if(num != IPE_TRAME_STS) printf(" (donnees):");
		if(hexa) {
			// if(!BigEndianOrder) IpeSwappeTrame((void *)buffer,nb);
		} else court = (short *)buffer;
		deja_vu = 0 ;
		for(i=0; i<nb; i++) {
			if(!(i % 16)) {
				if(!deja_vu) printf("\n%4d:",i);
				if(i) {
					if(!memcmp(buffer+i,buffer+i-16,16)) {
						if(!deja_vu) printf(" ...");
						deja_vu = 1;
						i += 15;
					} else {
						if(deja_vu) printf("\n%4d:",i);
						deja_vu = 0;
					}
				}
			} else if(!(i % 4)) printf("  ");
			if(!deja_vu) {
				if(BigEndianOrder) k = i; else { k = ((i & 1))? i - 1: i + 1; }
				if(hexa) printf(" %02X",buffer[k]); else {
					j = i / 2;
					/* if(!BigEndianOrder) { if((j % 2)) printf(" %6d",court[j-1]); else printf(" %6d",court[j+1]); }
					else */ if(court) printf(" %6d",court[j]);
					i++;
				}
			}
		}
		printf("\n");
		// if(hexa && !BigEndianOrder) IpeSwappeTrame((void *)buffer,nb);
	}
}
/* ========================================================================== */
void IpeRecopieStatus(IpeTrame trame, TypeRepartiteur *repart, char avec_pixbus) {

	if(!repart || !trame) return;
	repart->r.ipe.d0 = trame->p.sts_crate.d0;
	IpeTempsLu = trame->p.sts_crate.temps_seconde; // IPE_TRAME_TIMESTAMP(trame) / 100000;
	IpeStampMsb = trame->p.sts_crate.SLTTimeHigh;
	IpeStampLsb = trame->p.sts_crate.SLTTimeLow;
	if(avec_pixbus) IpePixBus[repart->r.ipe.chassis] = trame->p.sts_crate.pixbus_enable;
	IpeErreursNb = trame->p.sts_crate.error_counter;
	IpeInternalInfo = trame->p.sts_crate.internal_error_info;
	IpeSWstatus = trame->p.sts_crate.ipe4reader_status;
	RepartVersionLogiciel = trame->p.sts_crate.version;
	IpeFifoNb = (trame->p.sts_crate.numFIFOnumADCs >> 16) & 0xFFFF;
	IpeAdcNb = trame->p.sts_crate.numFIFOnumADCs & 0xFFFF;
	IpeTrameDim = trame->p.sts_crate.maxUDPSize;

	if(trame->p.sts_crate.micro_bbv2 == 1) {
		if(repart->status_fpga == 0) strncpy(RepartInfoFpgaBB,"Startup charge /var/*.rbf",REPART_INFOFPGA_DIM);
		else strcpy(RepartInfoFpgaBB,"Startup termine");
	} else if(trame->p.sts_crate.micro_bbv2 == 2) {
		if(repart->status_fpga == 1) strncpy(RepartInfoFpgaBB,"Ouverture de *.rbf",REPART_INFOFPGA_DIM);
		else strncpy(RepartInfoFpgaBB,"*.rbf ouvert",REPART_INFOFPGA_DIM);
	} else if(trame->p.sts_crate.micro_bbv2 > 0) {
		int stade;
		stade = trame->p.sts_crate.micro_bbv2 >> 8;
		// trame->p.sts_crate.micro_bbv2 & 0xff rend toujours 3
		if(stade <= 100) snprintf(RepartInfoFpgaBB,REPART_INFOFPGA_DIM,"%3d/100 prgm BB#%02d",
								  stade,trame->p.sts_crate.micro_bbv2 & 0xff);
		else strcpy(RepartInfoFpgaBB,"Redemarrage");
	} else {
		if(repart->status_fpga == 2) strcpy(RepartInfoFpgaBB,"*.rbf ou BB illisible");
		else if(repart->status_fpga > 2) strcpy(RepartInfoFpgaBB,"Verification en erreur");
		else strcpy(RepartInfoFpgaBB,"inchange");
	}
	RepartInfoFpgaBB[REPART_INFOFPGA_DIM-1] = '\0';
	repart->status_fpga = trame->p.sts_crate.micro_bbv2;

	strcpy(repart->date_status,DateHeure());
	NumeriseurEtatStamp = IpeTempsLu; strcpy(NumeriseurEtatDate,repart->date_status);
#ifdef A_AJUSTER
	if(trame->p.sts_crate.micro_bbv2 == 1) repart->arrete = 1;
	else if((trame->p.sts_crate.micro_bbv2 >> 8) > 100) {
		int port,flt; byte buffer[8];
		port = PortLectRep + repart->rep;
		buffer[0] = 'S'; buffer[1] = 0;
		buffer[2] = (byte)(port & 0xFF); buffer[3] = (byte)((port >> 8) & 0xFF);
		RepartIpEcrit(repart,buffer,4); SambaMicrosec(ModeleRep[repart->type].delai_msg);
		printf("%s/   . Ecrit: %c.%02X.%02X.%02X\n",DateHeure(),buffer[0],buffer[1],buffer[2],buffer[3]);
		buffer[0] = 'P'; buffer[1] = 0xF0;
		RepartIpEcrit(repart,buffer,4); SambaMicrosec(ModeleRep[repart->type].delai_msg);
		printf("%s/   . Ecrit: %c.%02X.%02X.%02X\n",DateHeure(),buffer[0],buffer[1],buffer[2],buffer[3]);
		repart->arrete = 0;
		for(flt = 0; flt<repart->r.ipe.fltnb; flt++) if(repart->r.ipe.flt[flt].fibres_fermees != 0x3F) {
			repart->r.ipe.flt[flt].fibres_fermees = 0x3F;
			IpeEcritFlt(repart,flt,IPE_FLT_ACCES_BB,repart->r.ipe.flt[flt].fibres_fermees,1);
		}
	}
#endif
	repart->status_relu = 1;
#ifdef A_AJOUTER
	int flt,fbr,m,o;
	TypeIpeFibreReg delais[IPE_FIBRE_REGS],selecteur[IPE_FIBRE_REGS];

	for(flt=0; flt<IPE_FLT_MAX; flt++) {
		/* a remplacer d'apres IPE_FLT_CONTROL repart->r.ipe.flt[flt].controle.complet = 0; */
		/* a remplacer d'apres IPE_FLT_DELAIS  */ for(m=0; m<IPE_FIBRE_REGS; m++) delais[m].total = 0;
		/* a remplacer d'apres IPE_FLT_SELECT  */ for(m=0; m<IPE_FIBRE_REGS; m++) selecteur[m].total = 0;
		for(fbr=0; fbr<IPE_FLT_SIZE; fbr++) {
			m = fbr / IPE_FIBRES_PARMOT; o = fbr - (m * IPE_FIBRES_PARMOT); if(BigEndianOrder) o = 3 - o;
			IpeStatus[flt][fbr].delai12 = delais[m].fibre[o] & 0xF;
			IpeStatus[flt][fbr].delai120 = (delais[m].fibre[o] >> 4) & 0xF;
			IpeStatus[flt][fbr].selection = selecteur[m].fibre[o];
		}
	}
#endif
}
/* ========================================================================== */
static int IpeChangeParmsGlob() {
	int rep; TypeRepartiteur *repart;

	if(RepartConnecte->rep >= 0) IpeCmdeH(RepartConnecte);
	else for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) /* anciennement select */ {
		repart = &(Repart[rep]);
		repart->r.ipe.d0 = RepartConnecte->r.ipe.d0;
		repart->r.ipe.clck = RepartConnecte->r.ipe.clck;
		printf("          * %s:\n",repart->nom);
		IpeCmdeH(repart);
	}
	return(0);
}
/* ========================================================================== */
int IpeEcritPixbus(TypeRepartiteur *repart, byte *texte, int *l) {
	int k;

	*l = sprintf((char *)texte,"%s_0x%08X_0x%08X",IPE_REG_WRITE,IPE_SLT_PIXBUS,IpePixBus[repart->r.ipe.chassis]); texte[*l] = '\0';
	if(repart->ecr.ip.path >= 0) {
		k = RepartIpEcrit(repart,texte,*l); SambaMicrosec(ModeleRep[repart->type].delai_msg);
	} else k = 0;
	return(k);
}
/* ========================================================================== */
static int IpeChangePixbus() {
	byte texte[80]; int k,l;

	k = IpeEcritPixbus(RepartConnecte,texte,&l);
	if(k == l) printf("  . Ecrit: %s\n",texte); else printf("  . Ecrit: %s, transmis: %d/%d (%s)\n",texte,k,l,strerror(errno));
	return(0);
}
/* ========================================================================== */
static int IpeClockM() {
	if(RepartConnecte->r.ipe.d0) RepartConnecte->r.ipe.d0 -= 1;
	IpeChangeParmsGlob();
	return(0);
}
/* ========================================================================== */
static int IpeClockP() {
	RepartConnecte->r.ipe.d0 += 1;
	IpeChangeParmsGlob();
	return(0);
}
/* ========================================================================== */
INLINE static int IpeChangeParmsFlt(Panel p, void *adrs) {
	TypeRepartiteur *repart; int flt,fibre,fltabs,l,k,m,o; byte texte[80];
	int pixbus,bit; hexa complet;
	TypeIpeFibreReg delais[IPE_FIBRE_REGS],selecteur[IPE_FIBRE_REGS];

	repart = RepartConnecte;
	if(repart->r.ipe.fifo < 0) {
		pixbus = 0; bit = 1;
		for(flt=0; flt<repart->r.ipe.fltnb; flt++) {
			if(repart->r.ipe.flt[flt].enabled) pixbus |= bit;
			bit = (bit << 1);
		}
		l = sprintf((char *)texte,"%s_0x%08X_0x%08X",IPE_REG_WRITE,IPE_SLT_PIXBUS,pixbus); texte[l] = '\0';
		k = RepartIpEcrit(repart,texte,l); SambaMicrosec(ModeleRep[repart->type].delai_msg);
		if(k == l) printf("  . Ecrit: %s\n",texte); else printf("  . Ecrit: %s, transmis: %d/%d (%s)\n",texte,k,l,strerror(errno));
	}
	for(flt=0; flt<repart->r.ipe.fltnb; flt++) if(repart->r.ipe.flt[flt].branchee) {
		fltabs = IPE_FLT_ABS(repart,flt);
		complet = repart->r.ipe.flt[flt].controle.complet; if(BigEndianOrder) ByteSwappeInt(&complet);
		l = sprintf((char *)texte,"%s_0x%02X_0x%08X_0x%08X",IPE_FLT_WRITE,fltabs+1,IPE_FLT_CONTROL,complet); texte[l] = '\0';
		k = RepartIpEcrit(repart,texte,l); SambaMicrosec(ModeleRep[repart->type].delai_msg);
		if(k == l) printf("  . Ecrit: %s\n",texte); else printf("  . Ecrit: %s, transmis: %d/%d (%s)\n",texte,k,l,strerror(errno));
		for(m=0; m<IPE_FIBRE_REGS; m++) delais[m].total = selecteur[m].total = 0;
		for(fibre=0; fibre<IPE_FLT_SIZE; fibre++) {
			m = fibre / IPE_FIBRES_PARMOT; o = fibre - (m * IPE_FIBRES_PARMOT); if(BigEndianOrder) o = IPE_FIBRES_PARMOT - 1 - o;
			delais[m].fibre[o] = ((IpeStatus[flt][fibre].delai120 << 4) & 0xF0) + (IpeStatus[flt][fibre].delai12 & 0xF);
			selecteur[m].fibre[o] = IpeStatus[flt][fibre].selection;
			repart->r.ipe.retard[IPE_ENTREE(flt,fibre)] = delais[m].fibre[o];
		}
		for(m=0; m<IPE_FIBRE_REGS; m++) {
			l = sprintf((char *)texte,"%s_0x%02X_0x%08X_0x%08X",IPE_FLT_WRITE,fltabs+1,IPE_FLT_DELAIS+(4*m),delais[m].total); texte[l] = '\0';
			k = RepartIpEcrit(repart,texte,l); SambaMicrosec(ModeleRep[repart->type].delai_msg);
			if(k == l) printf("  . Ecrit: %s\n",texte); else printf("  . Ecrit: %s, transmis: %d/%d (%s)\n",texte,k,l,strerror(errno));
		}
		for(m=0; m<IPE_FIBRE_REGS; m++) {
			l = sprintf((char *)texte,"%s_0x%02X_0x%08X_0x%08X",IPE_FLT_WRITE,fltabs+1,IPE_FLT_SELECT+(4*m),selecteur[m].total); texte[l] = '\0';
			k = RepartIpEcrit(repart,texte,l); SambaMicrosec(ModeleRep[repart->type].delai_msg);
			if(k == l) printf("  . Ecrit: %s\n",texte); else printf("  . Ecrit: %s, transmis: %d/%d (%s)\n",texte,k,l,strerror(errno));
		}
	}
	return(0);
}
/* ==========================================================================
static TypePanelFctnMod IpeRessEnCours(void *adrs, char *texte, void *arg) {
	int i;

	i = (int)arg;
	// IpeRess[i].a_charger = 1;
	// printf("Modif en cours du reglage %s.%d\n",numeriseur->nom,k);
	return(1);
}
   ==========================================================================
static TypePanelFctnItem IpeRessChangee(Panel p, int item, void *arg) {
	int i;

	i = (int)arg;
	// IpeRess[i].a_charger = 0;
	// printf("Modif terminee pour le reglage %s.%d\n",numeriseur->nom,k);
	return(1);
}
   ========================================================================== */
static void IpeCmdeFpga(TypeRepartiteur *repart, int fpga) {
	int k,l,flt,fbr,bb; TypeNumeriseur *numeriseur;
	char texte[80];

	for(flt = 0; flt<repart->r.ipe.fltnb; flt++) repart->r.ipe.flt[flt].fibres_fermees = 0x3F;
	for(l=0; l<repart->nbentrees; l++) if((numeriseur = (TypeNumeriseur *)repart->entree[l].adrs)) {
		if(numeriseur->sel) {
			flt = IPE_FLT_NUM(l); fbr = IPE_FLT_FBR(l,flt);
			repart->r.ipe.flt[flt].fibres_fermees &= ~(1 << fbr);
		}
	}
	for(flt = 0; flt<repart->r.ipe.fltnb; flt++) if(repart->r.ipe.flt[flt].fibres_fermees != 0x3F) {
		IpeEcritFlt(repart,flt,IPE_FLT_ACCES_BB,repart->r.ipe.flt[flt].fibres_fermees,1);
	}
	l = sprintf(RepartiteurValeurEnvoyee,"%s_%s_%s%s",IPE_SLT_WRITE,IpeSltCmde[IPE_SLT_FPGA],repart->r.ipe.fpga,FpgaModl[fpga].fichier); texte[l] = '\0';
	k = RepartIpEcrit(repart,RepartiteurValeurEnvoyee,l); SambaMicrosec(ModeleRep[repart->type].delai_msg);
	printf("%s/   . Ecrit: %s <- %s [%d/%d]\n",DateHeure(),repart->nom,RepartiteurValeurEnvoyee,k,l);

	IpeEcritRessourceBB(repart,-1,0xFF,ss_adrs_alim,_autorise_commandes);
	printf("%s/   . Ecrit: %s / alim = libere\n",DateHeure(),RepartiteurValeurEnvoyee);
	for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].sel && (repart == Numeriseur[bb].repart)) Numeriseur[bb].verrouille = 0;
}
/* ========================================================================== */
static int IpeChargeFpga() {
	int rep,fpga;

	if(!NumeriseurSelection(1)) return(0);
	if((ModlNumerChoisi < 0) || (ModlNumerChoisi >= ModeleBNNb)) return(0);
	fpga = ModeleBN[ModlNumerChoisi].fpga;
	if((fpga < 0) || (fpga >= NUMER_FPGA_TYPES)) return(0);
	printf("%s/ Chargement des FPGA code %s (%s):\n",DateHeure(),FpgaModl[fpga].code,FpgaModl[fpga].fichier);
	if(RepartConnecte->rep >= 0) {
		if(RepartConnecte->local) IpeCmdeFpga(RepartConnecte,fpga);
		else OpiumError("Ce repartiteur n'est pas dans le perimetre de l'acquisition");
	} else for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local && (Repart[rep].famille == FAMILLE_IPE)) /* anciennement select */ {
		printf("%s/ + %s:\n",DateHeure(),Repart[rep].nom);
		IpeCmdeFpga(&(Repart[rep]),fpga);
	}
	return(0);
}
/* ========================================================================== */
INLINE static void IpeChangeMode(TypeRepartiteur *repart, char mode, char *prefixe) {
	byte buffer[80]; hexa complet; int l,flt;

	if(!repart) return;
	for(flt = 0; flt<repart->r.ipe.fltnb; flt++) if(repart->r.ipe.flt[flt].active) {
		repart->r.ipe.flt[flt].controle.partiel.modeFLT &= 0xCF;
		repart->r.ipe.flt[flt].controle.partiel.modeFLT |= ((mode << 4) & 0x30);
		complet = repart->r.ipe.flt[flt].controle.complet; if(BigEndianOrder) ByteSwappeInt(&complet);
		l = sprintf((char *)buffer,"%s_0x%02X_0x%08X_0x%08X",IPE_FLT_WRITE,IPE_FLT_ABS(repart,flt)+1,IPE_FLT_CONTROL,complet); buffer[l] = '\0';
		RepartIpEcrit(repart,buffer,l); SambaMicrosec(ModeleRep[repart->type].delai_msg);
		if(prefixe) printf("%s. Ecrit: %s\n",prefixe,buffer);
	}
	if(pIpeStatus && OpiumDisplayed(pIpeStatus->cdr)) PanelRefreshVars(pIpeStatus);
}
/* ========================================================================== */
INLINE static void IpeDemarreNumeriseurs(TypeRepartiteur *repart, NUMER_MODE mode, byte adrs, char *prefixe) {
	byte buffer[80]; shex cde_cluzel; int flt;

	if(!repart) return;
	if(mode == NUMER_MODE_COMPTEUR) {
		if(prefixe) printf("%s! Mode '%s' pas implemente\n",prefixe,NumerModeTexte[mode]);
	} else {
		for(flt=0; flt<repart->r.ipe.fltnb; flt++) if(repart->r.ipe.flt[flt].enabled) IpeEcritFlt(repart,flt,IPE_FLT_ACCES_BB,0x0,1);
		buffer[0] = OPERA_CDE_BB; buffer[1] = _FPGA_code_EDW; buffer[2] = adrs; buffer[3] = CLUZEL_SSAD_ACQUIS;
		cde_cluzel = (mode == NUMER_MODE_ACQUIS)? CLUZEL_START: CLUZEL_STOP;
		buffer[4] = (byte)((cde_cluzel >> 8) & 0xFF); buffer[5] = (byte)(cde_cluzel & 0xFF); buffer[6] = '\0';
		RepartIpEcrit(repart,buffer,6); SambaMicrosec(ModeleRep[repart->type].delai_msg);
		if(prefixe) printf("%s. Ecrit: %c.%02X.%02X.%02X.%02X%02X\n",prefixe,buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]);
		for(flt=0; flt<repart->r.ipe.fltnb; flt++) if(repart->r.ipe.flt[flt].enabled) {
			repart->r.ipe.flt[flt].fibres_fermees = 0x3F;
			IpeEcritFlt(repart,flt,IPE_FLT_ACCES_BB,repart->r.ipe.flt[flt].fibres_fermees,1);
		}
	}
}
/* ========================================================================== */
INLINE static void IpeEcritRessourceBB(TypeRepartiteur *repart, short entree, shex adrs, hexa ss_adrs, TypeADU valeur) {
	if(!repart) return;
	if(repart->r.ipe.ports_BB || (entree < 0)) {
		byte buffer[8]; int n;
		buffer[0] = OPERA_CDE_BB;
		buffer[1] = _FPGA_code_EDW;
		buffer[2] = adrs;
		buffer[3] = (byte)(ss_adrs & 0xFF);
		buffer[4] = (byte)((valeur >> 8) & 0xFF);
		buffer[5] = (byte)(valeur & 0xFF);
		n = RepartIpEcrit(repart,buffer,6); SambaMicrosec(ModeleRep[repart->type].delai_msg);
		sprintf(RepartiteurValeurEnvoyee,"%c.%02X.%02X.%02X.%02X%02X",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]);
	} else {
		int l,n; int flt,fbr;
		flt = IPE_FLT_NUM(entree); fbr = IPE_FLT_FBR(entree,flt);
		l = sprintf(RepartiteurValeurEnvoyee,IPE_BBWRITE_AUTO,adrs,ss_adrs,(byte)((valeur >> 8) & 0xFF),(byte)(valeur & 0xFF),IPE_FLT_ABS(repart,flt)+1,fbr);
		RepartiteurValeurEnvoyee[l] = '\0';
		n = RepartIpEcrit(repart,RepartiteurValeurEnvoyee,l); SambaMicrosec(ModeleRep[repart->type].delai_msg);
	}
}
/* ========================================================================== */
/* static */ INLINE char IpeVidePile(TypeRepartiteur *repart, char *pas_tous_secs, short *pleins, char log) {
	int octets_lus,donnees_lues,donnees_absentes,haut_pile,sautees,dechet,nb_stamps,stamp_absents,nb; // ajout
	hexa lngr; long n; shex num_trame;
	int64 stamp_recu,timestamp,stamp_attendu; int64 t2,t3;
	// char episode_en_cours;
	byte *pile,fin_trame[IPE_TAILLE_ENTETE];
	TypeDonnee val;
	IpeTrame trame;
	char debug;
	char trame_inattendue,controle_strict;

// #define TIMESTAMP_FIXE
// #define TIMESTAMP_NON_CONTROLE
	if(!repart) return(0);
	if(repart->top < 0) { LectDeclencheErreur(_ORIGINE_,42,LECT_INCOHER); return(0); }
	if(repart->s.ip.manquant) return(0); // generer les donnees manquantes avant d'en rajouter: test sur (top - bottom) prioritaire
	haut_pile = repart->top * sizeof(TypeDonnee);
	if((haut_pile + sizeof(TypeIpeTrame)) > repart->octets) { *pleins += 1; return(0); }
	pile = (byte *)repart->fifo32; /* data16 est decalee de IPE_TAILLE_ENTETE octets */
	if(repart->top) memcpy(fin_trame,pile+haut_pile,IPE_TAILLE_ENTETE);
	trame = (IpeTrame)(pile + haut_pile);
	lngr = sizeof(TypeSocket);
	RepartIpEssais++;
	RepartIp.acces++;
	repart->acces_demandes++;
	debug = VerifIpLog || log;
	// if(repart->s.ip.stamp_redemande >= 0)
	if(VerifTempsPasse) t2 = VerifTempsPasse? (DateMicroSecs() - LectT0Run): 0; else t2 = 0; t3 = t2;
	octets_lus = RepartIpLitTrame(repart,trame,sizeof(TypeIpeTrame));
	if(repart->s.ip.stamp_redemande >= 0) t3 = VerifTempsPasse? (DateMicroSecs() - LectT0Run): 0;
	if(octets_lus == -1) {
		if(errno == EAGAIN) {
			if(log) printf("          %s: vide\n",repart->nom);
			RepartIp.vide++;
			repart->acces_vides++;
			repart->serie_vide++;
			repart->dernier_vide = t2;
			if(repart->s.ip.stamp_redemande >= 0) { repart->s.ip.duree_vide += (t3 - t2); repart->s.ip.nb_vide++; }
			return(0);
		} else { LectDeclencheErreur(_ORIGINE_,43,LECT_EMPTY); return(0); }
	} else if((octets_lus <= 0) || (octets_lus > sizeof(TypeIpeTrame))) {
		LectDeclencheErreur(_ORIGINE_,44,LECT_UNKN); return(0);
	} else if(((TypeSocketIP *)(&(repart->lec.ip.skt)))->sin_addr.s_addr != repart->adrs.val) {
		LectDeclencheErreur(_ORIGINE_,45,LECT_UNEXPEC); return(0);
	}
	if(log) {
		donnees_lues = (octets_lus - IPE_TAILLE_ENTETE) / sizeof(TypeDonnee); /* 2 octets par voie */
		printf("          %s: %d valeur%s\n",repart->nom,Accord1s(donnees_lues));
	}
	repart->acces_remplis++;
	if(repart->arrete) printf("%s! %s envoie a nouveau des donnees apres %lld us\n",DateHeure(),repart->nom,t2-repart->dernier_rempli);
	repart->arrete = 0;
	repart->serie_vide = 0;
	repart->dernier_rempli = t2;
	controle_strict = Trigger.actif && !LectEntretienEnCours;
	*pas_tous_secs = 1;
	RepartIpRecues++;
	//a gettimeofday(&LectDateRun,0);
	//a dernier_temps = (float)(LectDateRun.tv_sec % 60) + ((float)LectDateRun.tv_usec / 1000000.0);
	if(BigEndianOrder) {
		ByteSwappeIntArray((hexa *)trame,octets_lus / sizeof(Ulong));
		TwoShortsSwappeInt((hexa *)trame);
	}
	num_trame = IPE_TRAME_NUMERO(trame);
	if(num_trame > REPART_IP_CNTL) {
		TypeIpeBBsts *sts_bb; int64 nbD3;
		nbD3 = 0; // GCC
		if(num_trame == IPE_TRAME_STS) {
			stamp_recu = IPE_TRAME_TIMESTAMP(trame);
			nbD3 = stamp_recu / repart->s.ip.inc_stamp;
		#ifndef TIMESTAMP_NON_CONTROLE
			repart->s.ip.stampEnCours += repart->s.ip.inc_stamp;
			repart->s.ip.numAVenir = 0; // -= (repart->s.ip.tramesD3 + 1);
		#endif
			if(repart->s.ip.synchro_vue) {
				if(debug) printf("%s/ %s{%lld}, trame %12lld/%3d: recue %12lld/%3d (#%lld dans pile[%d]) ",DateHeure(),
					   repart->nom,repart->acces_remplis,repart->s.ip.stampEnCours,repart->s.ip.numAVenir,stamp_recu,num_trame,
					   repart->acces_remplis,RepartIpNiveauPerte);
				if((nbD3 * repart->s.ip.inc_stamp) != stamp_recu) /* pas un multiple de inc_stamp=100000 */ {
					if(debug) printf("[ERREUR! timestamp remis a %lld]\n",repart->s.ip.stampEnCours);
					else printf("%s/ %s{%lld}, trame %12lld/%3d: recue %12lld/%3d (#%lld dans pile[%d]), timestamp remis a %lld\n",DateHeure(),
						repart->nom,repart->acces_remplis,repart->s.ip.stampEnCours,repart->s.ip.numAVenir,stamp_recu,num_trame,
						repart->acces_remplis,RepartIpNiveauPerte,repart->s.ip.stampEnCours);
					if(VerifErreurLog) IpeDump((byte *)trame,octets_lus,1);
					if(VerifErreurStoppe) { RepartiteurStoppeTransmissions(repart,1); LectDeclencheErreur(_ORIGINE_,46,LECT_DESYNC); return(0); }
					repart->s.ip.stampDernier = repart->s.ip.stampEnCours;
					num_trame = repart->s.ip.numAVenir;
					goto rattrapee;
				} else if(stamp_recu != repart->s.ip.stampEnCours) {
					if(debug) printf("[%s]\n",(stamp_recu == repart->s.ip.stampEnCours)? "ok": "BAD VALUE!");
					else printf("%s/ %s{%lld}, trame %12lld/%3d: recue %12lld/%3d (#%lld dans pile[%d]), timestamp remis a %lld\n",DateHeure(),
						repart->nom,repart->acces_remplis,repart->s.ip.stampEnCours,repart->s.ip.numAVenir,stamp_recu,num_trame,
						repart->acces_remplis,RepartIpNiveauPerte,repart->s.ip.stampEnCours);
					if(VerifErreurLog) IpeDump((byte *)trame,octets_lus,1);
					if(VerifErreurStoppe) { RepartiteurStoppeTransmissions(repart,1); LectDeclencheErreur(_ORIGINE_,55,LECT_SYNC); return(0); }
					repart->s.ip.stampDernier = repart->s.ip.stampEnCours;
					num_trame = repart->s.ip.numAVenir;
					goto rattrapee;
				}
			} else {
				repart->s.ip.stampEnCours = stamp_recu;
				repart->s.ip.numAVenir = num_trame;
				repart->s.ip.synchro_vue = 1;
			}
			repart->s.ip.stampDernier = stamp_recu;
			if((LectArretProgramme > 0) && (repart->s.ip.stampDernier >= LectArretProgramme)) {
				RepartiteurStoppeTransmissions(repart,1); LectDeclencheErreur(_ORIGINE_,53,LECT_DESYNC); return(0);
			}
		#ifdef TIMESTAMP_NON_CONTROLE
		#ifndef TIMESTAMP_FIXE
			repart->s.ip.numAVenir = -1; // implique, d'ailleurs: pas de verification sur la trame courante
		#endif
		#endif
			if(repart->s.ip.stamp0 == 0) {
				repart->s.ip.stamp0 = repart->s.ip.stampDernier;
				repart->s.ip.num0 = 0;
				if((LectTimeStamp0 == 0) && (!LectStampSpecifique || (repart->categ == DONNEES_STAMP))) {
					LectTimeStamp0 = repart->s.ip.stamp0;
					LectTimeStampN = LectTimeStamp0;
					if(repart->s.ip.stampD3 > repart->s.ip.duree_trame) LectTimeStamp0 += (int64)(repart->s.ip.num0 * repart->s.ip.duree_trame);
					printf("            . TimeStamp initial du run: %lld (par repartiteur %s)\n",LectTimeStamp0,RepartDonneesListe[repart->categ]);
					if(repart->categ == DONNEES_STAMP) { repart->actif = 0; return(0); }
				}
			}
			if(repart->status_demande && (repart == RepartConnecte)) IpeRecopieStatus(trame,repart,1);
			if(LectLitStatus || (Archive.enservice && LectSauveStatus) /* && (NumeriseurEtatDemande || ReglagesEtatDemande) */) {
				NumeriseurEtatStamp = trame->p.sts_crate.temps_seconde; // IPE_TRAME_TIMESTAMP(trame) / 100000;
				strcpy(NumeriseurEtatDate,DateHeure());
			}
			sts_bb = (TypeIpeBBsts *)((long)&(trame->p.sts_crate) + (long)(trame->p.sts_crate.dim));
		} else sts_bb = &(trame->p.sts_bb);
		if(LectLitStatus || (Archive.enservice && LectSauveStatus)) {
			while(sts_bb->dim) {
				TypeNumeriseur *numeriseur; TypeBNModele *modele_bn; char copie,archive; int i,j,l,m,nb;
				// if(sts_bb->dim) sts_bb->dim = 128;
				if(BigEndianOrder) ByteSwappeInt(&(sts_bb->source.complete));
				n = (long)sts_bb - (long)trame; if((n + 4) > octets_lus) break;
				l = IPE_ENTREE(IPE_FLT_REL(repart,sts_bb->source.b.flt),sts_bb->source.b.fbr);
				if((l >= 0) && (l < repart->nbentrees) && (numeriseur = (TypeNumeriseur *)repart->entree[l].adrs) && (numeriseur->avec_status)) {
					//* printf("(%s) %s: status %s\n",__func__,numeriseur->nom,numeriseur->status.a_copier?"demande":"ignore");
					modele_bn = &(ModeleBN[numeriseur->def.type]);
					copie = (LectLitStatus && numeriseur->status.a_copier);
					archive = (Archive.enservice && LectSauveStatus && modele_bn->enreg_dim && !numeriseur->enreg_stamp);
					if(copie || archive) {
						nb = (sts_bb->dim - IPE_BBSTS_HDR) / sizeof(shex);
						if(archive) {
							for(m=0; m<modele_bn->enreg_dim; m++) {
								if((j = modele_bn->enreg_sts[m]) < nb) {
									i = (BigEndianOrder)? ((j & 1)? j - 1: j + 1): j;
									numeriseur->enreg_val[m] = sts_bb->val[i];
								}
							}
							numeriseur->enreg_stamp = repart->s.ip.stampDernier;
							LectStatusDispo = 1;
						}
						if(copie) {
							shex *status_lu;
							int64 t1,t2;
							t1 = (VerifTempsPasse? DateMicroSecs(): 0);
							status_lu = sts_bb->val;
							// faire plus malin? (fiabilite de _nb_mots_status_bbv2 ...)
							for(j=0; j<nb; j++) {
								i = (BigEndianOrder)? ((j & 1)? j - 1: j + 1): j;
								numeriseur->status_val[i] = *status_lu++;
							}
							numeriseur->status.nb = nb;
							strcpy(numeriseur->status.lu,NumeriseurEtatDate);
							numeriseur->status.stamp = NumeriseurEtatStamp;
							numeriseur->status.termine = 1;
							t2 = (VerifTempsPasse? DateMicroSecs(): 0) - t1;
						}
					}
				}
				sts_bb = (TypeIpeBBsts *)((long)sts_bb + (long)(sts_bb->dim));
			}
		}
		RepartIp.traitees++;
		if(LectDureeReset) {
			int depuis_reset,avant_reset;
			depuis_reset = Modulo(nbD3,LectDureeReset);
			avant_reset = LectDureeReset - depuis_reset;
			if((depuis_reset == LectGardeReset) || ((avant_reset * repart->s.ip.inc_stamp) > CalcSpectreAutoDuree)) {
				LectDureeReset = 0;
				OpiumProgresClose();
			} else if(!OpiumProgresRefresh(LectAttenteReset-avant_reset)) { LectDureeReset = 0; OpiumProgresClose(); }
			else return(0);
		}
		if(repart->top) memcpy(pile+haut_pile,fin_trame,IPE_TAILLE_ENTETE);
		return(0);
	} else if(LectModeStatus || LectDureeReset) return(0);

rattrapee:
	timestamp = repart->s.ip.stampDernier;
	if(num_trame == 0) {
		if(!repart->D3trouvee) { repart->D3trouvee = 1; RepartCompteCalagesD3(); }
		if((LectTimeStamp0 == 0) && (!LectStampSpecifique || (repart->categ == DONNEES_STAMP))) {
			LectTimeStamp0 = timestamp;
			if(repart->s.ip.stampD3 > repart->s.ip.duree_trame) LectTimeStamp0 += (int64)(num_trame * repart->s.ip.duree_trame);
			LectTimeStampN = LectTimeStamp0;
			printf("            . TimeStamp initial du run: %lld (par repartiteur %s)\n",LectTimeStamp0,RepartDonneesListe[repart->categ]);
			if(repart->categ == DONNEES_STAMP) { repart->actif = 0; return(0); }
			LectDerniereD3 = 0;
		} else LectDerniereD3 = timestamp - LectTimeStamp0;
		if(repart->s.ip.lu_entre_D3 && (repart->s.ip.lu_entre_D3 != repart->s.ip.dim_D3)) {
			printf("%s! %s{%lld}: trame %12lld/%3d, %d octets recus (%d/%d donnees prealables), ",DateHeure(),
				repart->nom,repart->acces_remplis,timestamp,num_trame,octets_lus,repart->s.ip.lu_entre_D3,repart->s.ip.dim_D3);
			RepartIpErreurs++;
			if(controle_strict && VerifErreurStoppe) {
				printf("situation redhibitoire!\n");
				RepartiteurStoppeTransmissions(repart,1); LectDeclencheErreur(_ORIGINE_,49,LECT_DESYNC); return(0);
			}
			if(repart->s.ip.lu_entre_D3 > repart->s.ip.dim_D3) {
				dechet = repart->s.ip.lu_entre_D3 - repart->s.ip.dim_D3;
				repart->top -= dechet;
				repart->s.ip.lu_entre_D3 = 0;
				printf("%d donnee%s eliminee%s\n",Accord2s(dechet));
			} else {
				RepartIpEchappees += (repart->s.ip.tramesD3 - repart->s.ip.num1);
				donnees_absentes = repart->s.ip.dim_D3 - repart->s.ip.lu_entre_D3;
				dechet = 0;
				val = (repart->top > 0)? *(TypeDonnee *)(repart->data16 + (repart->top - 1)): 0;
				while(repart->s.ip.lu_entre_D3 < repart->s.ip.dim_D3) {
					*(TypeDonnee *)(repart->data16 + repart->top++) = val;
					repart->s.ip.lu_entre_D3++; dechet++;
					if(repart->top >= repart->maxdonnees) break;
				}
				printf("%d valeur%s =%d ajoutee%s [%d/%d]\n",Accord1s(dechet),val,AccordAvec(dechet),repart->top,repart->maxdonnees);
				stamp_absents = donnees_absentes / repart->nbdonnees;
				LectStampPerdus += stamp_absents;
				repart->s.ip.stampPerdus += (int64)stamp_absents;
				donnees_absentes -= dechet;
				if((RepartIpCorrection == REPART_IP_GENERE) && donnees_absentes) {
					// on generera ainsi des valeurs constantes au depilement
					repart->s.ip.manquant = donnees_absentes;
					repart->s.ip.saut = repart->top; /* point ou il faut commencer a generer, en esperant qu'il n'y en ait qu'un dans la pile */
					/* if(VerifErreurLog) printf("%s/ %s{%lld}: + generation de %d valeur%s constante%s x %d voie%s\n",DateHeure(),
						repart->nom,repart->acces_remplis,Accord2s(stamp_absents),Accord1s(repart->nbdonnees)); */
					printf("%s/ %s{%lld}: + generation de %d valeur%s constante%s\n",DateHeure(),
						   repart->nom,repart->acces_remplis,Accord2s(donnees_absentes));
				}
			}
			//- LectArretProgramme = repart->s.ip.stampDernier + (5 * repart->s.ip.inc_stamp);
		}
		if(debug) printf("%s! %s{%lld}: trame %12lld/%3d, %d octets recus soit %d donnees recues entre D3, remise a zero\n",DateHeure(),
			repart->nom,repart->acces_remplis,timestamp,num_trame,octets_lus,repart->s.ip.lu_entre_D3);
		if(repart->s.ip.lu_entre_D3 == repart->s.ip.dim_D3) repart->s.ip.lu_entre_D3 = 0;
	}
//	controle_strict = 0;
	sautees = stamp_absents = 0;
	repart->s.ip.stamp1 = timestamp;
	repart->s.ip.num1 = num_trame;
	if(repart->s.ip.numAVenir >= 0) {
		if(repart->s.ip.stamp_redemande >= 0) {
			// episode_en_cours = 1; // si on y revient...
			if((timestamp == repart->s.ip.stamp_redemande) && (num_trame == repart->s.ip.trame_redemandee)) {
				nb = (int)(repart->acces_remplis - repart->s.ip.appel_perdant);
				printf("%s/ %s{%lld}: %d trame%s recuperee%s dans pile[%d] dont %d bof%s (%.3f ms) + %d vide%s (%.3f ms)\n",DateHeure(),
					   repart->nom,repart->acces_remplis,Accord2s(nb),RepartIpNiveauPerte,Accord1s(repart->s.ip.nb_inutile),(float)repart->s.ip.duree_inutile/1000.0,
					   Accord1s(repart->s.ip.nb_vide),(float)repart->s.ip.duree_vide/1000.0);
				RepartIp.perdues += (repart->acces_remplis - repart->s.ip.appel_perdant);
				repart->s.ip.stampEnCours = timestamp;
				repart->s.ip.numAVenir = num_trame;
				repart->s.ip.stamp_redemande = -1; repart->s.ip.trame_redemandee = -1;
				repart->s.ip.saut = 0;
				repart->s.ip.en_defaut = 0;
				repart->s.ip.appel_perdant = 0;
			} else {
				repart->s.ip.duree_inutile += (t3 - t2); repart->s.ip.nb_inutile++;
				sautees = (int)(((timestamp - repart->s.ip.stamp_redemande) * (repart->s.ip.tramesD3 + 1) / repart->s.ip.inc_stamp) + (num_trame - repart->s.ip.trame_redemandee));
				if(sautees > repart->s.ip.tramesD3) {
					printf("%s/ %s{%lld}: perdu %12lld/%3d, recue %12lld/%3d (#%lld dans pile[%d])\n",DateHeure(),
						   repart->nom,repart->acces_remplis,repart->s.ip.stamp_redemande,repart->s.ip.trame_redemandee,timestamp,num_trame,
						   repart->acces_remplis,RepartIpRecues);
					//a printf(" a t=%.3f/%.3f",dernier_temps,repart->temps_precedent);
					LectDeclencheErreur(_ORIGINE_,47,LECT_SYNC); return(0);
				}
			}
		} else repart->s.ip.en_defaut = 0;
		stamp_attendu = repart->s.ip.stampEnCours;
		if(num_trame != repart->s.ip.numAVenir) trame_inattendue = 1;
		else if(timestamp != stamp_attendu) trame_inattendue = 1;
		else trame_inattendue = 0;
		if(trame_inattendue) {
			if(!LectEpisodeEnCours) {
				++LectEpisodeAgite;
				if(VerifErreurLog) printf("%s! Debut episode #%d apres precedente lecture de %lld puis attente de %lld us (lecture #%d)\n",
					DateHeure(),LectEpisodeAgite,LectDepileDuree,LectDepileAttente,LectDepileNb);
				else printf("%s! Debut episode #%d\n",DateHeure(),LectEpisodeAgite);
				LectEpisodeEnCours = 1;
			}
			repart->s.ip.en_defaut = 1;
			// episode_en_cours = 1; // manquee = 1;
			RepartIpNiveauPerte = RepartIpRecues;
			sautees = (int)((timestamp - stamp_attendu) * (repart->s.ip.tramesD3 + 1) / repart->s.ip.inc_stamp) + (num_trame - repart->s.ip.numAVenir);
			if(VerifErreurLog) printf("%s/ %s{%lld}, trame %12lld/%3d: recue %12lld/%3d (#%lld dans pile[%d]",DateHeure(),
				   repart->nom,repart->acces_remplis,stamp_attendu,repart->s.ip.numAVenir,timestamp,num_trame,
				   repart->acces_remplis,RepartIpNiveauPerte);
			//a printf(" a t=%.3f/%.3f",dernier_temps,repart->temps_precedent);
			//b if(repart->s.ip.stamp_redemande >= 0) printf(" + %d bof%s (%.3f ms) + %d vide%s (%.3f ms)",
			//b		Accord1s(repart->s.ip.nb_inutile),(float)repart->s.ip.duree_inutile/1000.0,Accord1s(repart->s.ip.nb_vide),(float)repart->s.ip.duree_vide/1000.0);
			if(!repart->s.ip.appel_perdant) repart->s.ip.appel_perdant = repart->acces_remplis;
			if((sautees < 0) && (RepartIpCorrection != REPART_IP_ABANDON)) {
				if(RepartIpCorrection == REPART_IP_INSISTE) /* loupee une deuxieme fois! */ {
					if(VerifErreurLog) printf("), perdu %12lld/%3d pour la deuxieme fois\n",repart->s.ip.stamp_redemande,repart->s.ip.trame_redemandee);
					repart->s.ip.stamp_redemande = -1; /* ouais... mais c'est quand meme foutu, on est archi noye!! */
				} else if(VerifErreurLog) {
					printf("), %d en trop\n",-sautees);
					IpeDump((byte *)trame,octets_lus,1);
					if(num_trame > repart->s.ip.tramesD3) {
						// timestamp TEMPORAIREMENT fixe..
						timestamp += repart->s.ip.inc_stamp;
						num_trame -= (repart->s.ip.tramesD3 + 1);
					}
				}
				if(controle_strict) {
					LectDeclencheErreur(_ORIGINE_,48,LECT_TOOMUCH);
					return(0); /* on laisse carrement tomber le dernier arrivage, qui a l'air d'etre inutilisable */
				}
			} else if(sautees) {
				if(VerifErreurLog) printf("), %d echappee%s\n",Accord1s(sautees));
				RepartIpEchappees += sautees;
				if(repart->s.ip.stamp_redemande < 0) {
					if((RepartIpCorrection == REPART_IP_INSISTE) && (sautees < 15) && controle_strict) {
						byte buffer[4];
						repart->s.ip.stamp_redemande = stamp_attendu;
						repart->s.ip.trame_redemandee = repart->s.ip.numAVenir;
						/* envoi de la fameuse commande 'R' avec repart->s.ip.numAVenir comme argument */
						buffer[0] = 'R'; buffer[1] = 0;
						buffer[2] = (byte)(repart->s.ip.trame_redemandee & 0xFF);
						buffer[3] = (byte)((repart->s.ip.trame_redemandee >> 8) & 0xFF);
						RepartIpEcrit(repart,buffer,4); // SambaMicrosec(ModeleRep[repart->type].delai_msg);
						// #ifdef TRACE_TRAME_NIV1
						printf("%s/ %s{%lld}, trame redemandee:  %3d (%c.%02X.%02X.%02X)\n",DateHeure(),
							   repart->nom,repart->acces_remplis,repart->s.ip.trame_redemandee,buffer[0],buffer[1],buffer[2],buffer[3]);
						// #endif
						repart->s.ip.duree_vide = repart->s.ip.duree_inutile = 0;
						repart->s.ip.nb_vide = repart->s.ip.nb_inutile = 0;
					} else {
						nb_stamps = repart->s.ip.duree_trame;
						stamp_absents = (int)(timestamp + (num_trame * nb_stamps) - (stamp_attendu + (repart->s.ip.numAVenir * nb_stamps)));
						LectStampPerdus += stamp_absents;
						repart->s.ip.stampPerdus += (int64)stamp_absents;
						donnees_absentes = stamp_absents * repart->nbdonnees;
						// repart->s.ip.lu_entre_D3 += donnees_absentes; fait au moment dans LectDepileDonnees()
						if(RepartIpCorrection == REPART_IP_GENERE) {
							// on generera ainsi des valeurs constantes au depilement
							repart->s.ip.manquant = donnees_absentes;
							repart->s.ip.saut = repart->top; /* point ou il faut commencer a generer, en esperant qu'il n'y en ait qu'un dans la pile */
							/* if(VerifErreurLog) */ printf("%s/ %s{%lld}, generation de %d valeur%s constante%s x %d voie%s\n",DateHeure(),
								repart->nom,repart->acces_remplis,Accord2s(stamp_absents),Accord1s(repart->nbdonnees));
						}
						num_trame = repart->s.ip.numAVenir + sautees;
					}
				} // else printf("%s/ %s, trame NON redemandee\n",DateHeure(),repart->nom);
			} else if(VerifErreurLog) printf("), erreur de numerotation\n");
		}
		repart->s.ip.numAVenir = num_trame + 1;
	} else {
		repart->s.ip.numAVenir = num_trame + 1; repart->s.ip.stampEnCours = timestamp;
	}
#ifdef TIMESTAMP_FIXE
	if(repart->s.ip.numAVenir > repart->s.ip.tramesD3) {
		repart->s.ip.stampEnCours += repart->s.ip.inc_stamp;
		repart->s.ip.numAVenir -= (repart->s.ip.tramesD3 + 1);
	}
#endif
	//a repart->temps_precedent = dernier_temps;
	if(repart->s.ip.stamp_redemande >= 0) return(0);
	if(repart->top) memcpy(trame,fin_trame,IPE_TAILLE_ENTETE);
	RepartIpOctetsLus += octets_lus;
	RepartIp.traitees++;
	donnees_lues = (octets_lus - IPE_TAILLE_ENTETE) / sizeof(TypeDonnee); /* 2 octets par voie */
	repart->donnees_recues += donnees_lues;
	repart->top += donnees_lues;
	repart->s.ip.lu_entre_D3 += donnees_lues;
	if(repart->s.ip.numAVenir > repart->s.ip.tramesD3) /* (repart->s.ip.numAVenir == 0) 0 <=> chgmt timestamp <=> synchro D3 <=> trop tard .. */ {
		if(repart->s.ip.lu_entre_D3 != repart->s.ip.dim_D3) {
			printf("%s! %s{%lld}: trame %12lld/%3d, %d octets recus soit %d donnees recues entre D3 (au lieu de %d), ",DateHeure(),
				repart->nom,repart->acces_remplis,timestamp,num_trame,octets_lus,repart->s.ip.lu_entre_D3,repart->s.ip.dim_D3);
			RepartIpErreurs++;
			if(controle_strict && VerifErreurStoppe) {
				printf("situation redhibitoire!\n");
				RepartiteurStoppeTransmissions(repart,1); LectDeclencheErreur(_ORIGINE_,51,LECT_DESYNC); return(0);
			}
			if(repart->s.ip.lu_entre_D3 > repart->s.ip.dim_D3) {
				dechet = repart->s.ip.lu_entre_D3 - repart->s.ip.dim_D3;
				repart->top -= dechet;
				repart->s.ip.lu_entre_D3 = 0;
				printf("%d donnee%s eliminee%s\n",Accord2s(dechet));
			} else {
				donnees_absentes = repart->s.ip.dim_D3 - repart->s.ip.lu_entre_D3;
				dechet = 0;
				val = (repart->top > 0)? *(TypeDonnee *)(repart->data16 + (repart->top - 1)): 0;
				while(repart->s.ip.lu_entre_D3 < repart->s.ip.dim_D3) {
					*(TypeDonnee *)(repart->data16 + repart->top++) = val;
					repart->s.ip.lu_entre_D3++; dechet++;
					if(repart->top >= repart->maxdonnees) break;
				}
				printf("%d valeur%s =%d ajoutee%s [%d/%d]\n",Accord1s(dechet),val,AccordAvec(dechet),repart->top,repart->maxdonnees);
				stamp_absents = donnees_absentes / repart->nbdonnees;
				LectStampPerdus += stamp_absents;
				repart->s.ip.stampPerdus += (int64)stamp_absents;
				donnees_absentes -= dechet;
				if((RepartIpCorrection == REPART_IP_GENERE) && donnees_absentes) {
					// on generera ainsi des valeurs constantes au depilement
					repart->s.ip.manquant = donnees_absentes;
					repart->s.ip.saut = repart->top; /* point ou il faut commencer a generer, en esperant qu'il n'y en ait qu'un dans la pile */
					printf("%s/ %s{%lld}: + generation de %d valeur%s constante%s\n",DateHeure(),
						   repart->nom,repart->acces_remplis,Accord2s(donnees_absentes));
				}
			}
		}
		if(debug) printf("%s! %s{%lld}: trame %12lld/%3d, %d octets recus soit %d donnees recues entre D3, remise a zero\n",DateHeure(),
			   repart->nom,repart->acces_remplis,timestamp,num_trame,octets_lus,repart->s.ip.lu_entre_D3);
		if(repart->s.ip.lu_entre_D3 == repart->s.ip.dim_D3) repart->s.ip.lu_entre_D3 = 0;
	} else if(octets_lus != repart->dim.trame) {
		printf("%s! %s{%lld}: trame %12lld/%3d, %d octets recus au lieu de %d (%d donnees recues depuis D3)\n",DateHeure(),
			repart->nom,repart->acces_remplis,timestamp,num_trame,octets_lus,repart->dim.trame,repart->s.ip.lu_entre_D3);
		RepartIpErreurs++;
		// RepartiteurStoppeTransmissions(repart,1); LectDeclencheErreur(_ORIGINE_,52,LECT_DESYNC); return(0);
	} else if(repart->s.ip.lu_entre_D3 > repart->s.ip.dim_D3) {
		printf("%s! %s{%lld}: trame %12lld/%3d, %d octets recus soit deja %d donnees depuis D3 (maximum attendu: %d), ",DateHeure(),
			repart->nom,repart->acces_remplis,timestamp,num_trame,octets_lus,repart->s.ip.lu_entre_D3,repart->s.ip.dim_D3);
		RepartIpErreurs++;
		if(controle_strict && VerifErreurStoppe) {
			printf("situation redhibitoire!\n");
			RepartiteurStoppeTransmissions(repart,1); LectDeclencheErreur(_ORIGINE_,50,LECT_DESYNC); return(0);
		}
		dechet = repart->s.ip.lu_entre_D3 - repart->s.ip.dim_D3;
		repart->top -= dechet;
		repart->s.ip.lu_entre_D3 = 0;
		printf("%d donnee%s eliminee%s\n",Accord2s(dechet));
	}
	if(RepartIpRecues > RepartIp.depilees) RepartIp.depilees = RepartIpRecues;
	return(1);
}
#pragma mark ----------- Pre-implementation -----------
/* ========================================================================== */
INLINE char RepartIpEcriturePrete(TypeRepartiteur *repart, char *marge) {
	char buffer[256],qui[80];

	if(!repart) { printf("%s! Repartiteur indefini\n",marge); return(0); }
	else if(repart->ecr.ip.path < 0) {
		sprintf(qui,"%s* Acces en ecriture sur %d.%d.%d.%d:%d",marge,IP_CHAMPS(repart->adrs),repart->ecr.port);
		if(repart->valide == REP_STATUT_HS) {
			printf("%s inutile\n",qui); repart->valide = REP_STATUT_HS;
		} else if(repart->simule) {
			repart->ecr.ip.path = repart->rep;
			printf("%s simule sur <%d>\n",qui,repart->ecr.ip.path); repart->valide = REP_STATUT_VALIDE;
		} else if((repart->famille == FAMILLE_IPE) || (repart->famille == FAMILLE_OPERA) || (repart->famille == FAMILLE_SAMBA)) {
			if(!repart->present) {
				sprintf(buffer,COMMANDE_PING,IP_CHAMPS(repart->adrs));
				if(system(buffer) != 0) { printf("%s impossible (ne repond pas)\n",qui); return(0); } else repart->present = 1;
			}
			if((repart->ecr.ip.path = SocketConnectUDP(&(repart->ecr.ip.skt))) < 0) { printf("%s impossible (%s)\n",qui,SocketErreur()); return(0); }
			printf("%s ouvert en <%d>\n",qui,repart->ecr.ip.path); repart->valide = REP_STATUT_VALIDE;
			if(repart->famille == FAMILLE_IPE) IpeSetupFibres(repart);
		} else if(repart->famille == FAMILLE_CALI) {
			if(!repart->present) {
				sprintf(buffer,COMMANDE_PING,IP_CHAMPS(repart->adrs));
				if(system(buffer) != 0) { printf("%s impossible (ne repond pas)\n",qui); return(0); } else repart->present = 1;
			}
			if((repart->ecr.ip.path = SocketConnect(&(repart->ecr.ip.skt))) < 0) { printf("%s impossible (%s)\n",qui,SocketErreur()); return(0); }
			printf("%s ouvert en <%d>\n",qui,repart->ecr.ip.path); repart->valide = REP_STATUT_VALIDE;
		} else { printf("%s non supporte\n",qui); return(0); }
	}
	return(1);
}
/* ========================================================================== */
INLINE ssize_t RepartIpEcrit(TypeRepartiteur *repart, const void *buffer, size_t lngr) {

	if(repart->simule) return((ssize_t)lngr);
	else {
		ssize_t rc;
		if(repart->ecr.ip.path >= 0) rc = write(repart->ecr.ip.path,buffer,lngr); else rc = -1;
		return(rc);
	}
}
/* ========================================================================== */
void RepartIpRazCompteurs(TypeRepartiteur *repart) {
	if(!repart) return;
	repart->s.ip.stamp0 = 0; repart->s.ip.num0 = 0;
	repart->s.ip.stamp1 = 0; repart->s.ip.num1 = 0;
	repart->s.ip.numAVenir = -1;
	repart->s.ip.stampPerdus = 0;
	repart->s.ip.recu = 0;
	repart->s.ip.appel_perdant = 0;
	repart->s.ip.stamp_redemande = -1;
	repart->s.ip.trame_redemandee = -1;
	repart->s.ip.manquant = 0;
	repart->s.ip.saut = 0;
	repart->s.ip.en_defaut = 0;
	repart->s.ip.synchro_vue = 0;
	repart->s.ip.depile = 0;
	repart->s.ip.lu_entre_D3 = 0;
	repart->s.ip.err_cew = repart->s.ip.err_stamp = repart->s.ip.err_synchro = -1;
}
/* ========================================================================== */
static void RepartIpEnvoie(TypeRepartiteur *repart, byte *buffer, int l, char termine, char attente, char log) {
	int rc;

	buffer[l] = '\0';
	if(termine) l++;
	rc = RepartIpEcrit(repart,buffer,l);
	if(rc < 0) {
		if(!RepartIpEcriturePrete(repart,"            ")) return;
		rc = RepartIpEcrit(repart,buffer,l);
		if(rc < 0) return;
	}
	if(attente) SambaMicrosec(ModeleRep[repart->type].delai_msg);
	if(log >= 0) {
		printf("%s/   . Ecrit '%s'",DateHeure(),buffer);
		if(rc < 0) printf(": %s\n",strerror(errno)); else printf("\n");
	}
}
/* ========================================================================== */
static void RepartIpToOpera(TypeRepartiteur *repart, byte *buffer, char log) {
	int rc;

	rc = RepartIpEcrit(repart,buffer,4); SambaMicrosec(ModeleRep[repart->type].delai_msg);
	if(log >= 0) {
		printf("%s/   . Ecrit %c.%02X.%02X.%02X",DateHeure(),buffer[0],buffer[1],buffer[2],buffer[3]);
		if(rc < 0) printf(": %s",strerror(errno)); printf("\n");
	}

}
/* ========================================================================== */
INLINE void RepartIpTransmission(TypeRepartiteur *repart, char nature, int duree, char log) {
	/* demande l'envoi des donnees pendant <duree> synchros D3. Ne ferme pas la socket si duree < 0 */
	int k,l,i,adc,voie,num; char trgr; int d3_a_lire; shex val; byte buffer[256];
	ssize_t octets_lus; byte trame[TRAME_STD];
	TypeNumeriseur *numeriseur;

	if(!repart) return;
	if(!RepartIpEcriturePrete(repart,"            ")) return;
	d3_a_lire = (duree > 0)? duree: 0;
	if(d3_a_lire) {
		if(repart->lec.ip.path < 0) {
			if(log) printf("%s/   . Affectation du port %d pour la lecture\n",DateHeure(),repart->lec.port);
			if(repart->simule) {
				repart->lec.ip.path = repart->rep;
				if(log) printf("%s/   ! Socket pour lecture sur %d.%d.%d.%d:%d simulee.\n",DateHeure(),IP_CHAMPS(repart->adrs),repart->lec.port);
			} else {
				if((repart->lec.ip.path = SocketUDP(repart->lec.port)) >= 0) {
					if(fcntl(repart->lec.ip.path,F_SETFL,O_NONBLOCK) == -1) {
						if(log) printf("%s/   ! Socket pour lecture sur %d.%d.%d.%d:%d bloquante: %s\n",DateHeure(),IP_CHAMPS(repart->adrs),repart->lec.port,strerror(errno));
					}
				} else if(log) printf("%s/   !!! Connexion pour lecture sur %d.%d.%d.%d:%d impossible: %s\n",DateHeure(),IP_CHAMPS(repart->adrs),repart->lec.port,strerror(errno));
			}
			if(repart->famille == FAMILLE_SAMBA) {
				SmbEcrit(repart,SMB_CONNECT,(shex)repart->lec.port);
				if(log >= 0) printf("%s/   . Ecrit '%s'\n",DateHeure(),RepartiteurValeurEnvoyee);
			}
		}
		if(repart->lec.ip.path >= 0) {
			if(log) printf("%s/   . Acces en lecture sur %d.%d.%d.%d:%d ouvert en <%d>\n",DateHeure(),IP_CHAMPS(repart->adrs),repart->lec.port,repart->lec.ip.path);
			if(repart->fifo_pleine) {
				if(log) printf("%s/   . Vidage de la FIFO\n",DateHeure());
				if(!repart->simule) do octets_lus = RepartIpLitTrame(repart,trame,TRAME_STD); while(octets_lus >= 0);
				repart->fifo_pleine = 0;
			}
			if(repart->famille == FAMILLE_OPERA) {
				buffer[0] = (nature == REPART_ENVOI_DETEC)? 'P':'S'; buffer[1] = (byte)(d3_a_lire & 0xFF);
				buffer[2] = (byte)(repart->lec.port & 0xFF);
				buffer[3] = (byte)((repart->lec.port >> 8) & 0xFF);
				RepartIpToOpera(repart,buffer,log);
			} else if(repart->famille == FAMILLE_CALI) {
				if(repart->revision > 2) {
				#ifdef RP_RESET_MANUEL
					hexa cntrl;
					if(!repart->r.cali.clck) {
						cntrl = (hexa)repart->r.cali.sel; if(repart->r.cali.clck) cntrl = (cntrl | 0x10); // selection et master/slave
						cntrl = (cntrl | 0xC0); // plus frameId reset et TS reset
						l = sprintf((char *)buffer,"w %x %x",CALI_REG_SEL,cntrl);
						RepartIpEnvoie(repart,buffer,l,1,1,0); // avec petit delai avant prochain envoi
					}
				#endif /* RP_RESET_MANUEL */
					num = 0; k = 0;
					for(i=0; i<repart->nbentrees; i++) if((numeriseur = (TypeNumeriseur *)(repart->entree[i].adrs))) {
						for(adc=0; adc<numeriseur->nbadc; adc++) if((voie = numeriseur->voie[adc]) >= 0) {
							num++;
							trgr = (VoieTampon[voie].trig.trgr)? VoieTampon[voie].trig.trgr->origine: TRGR_INTERNE;
							if(!Trigger.enservice || !VoieTampon[voie].lue || (trgr == TRGR_INTERNE)) {
								if(k) buffer[k++] = '/'; l = sprintf((char *)buffer+k,"t%d off",num); k += l;
							} else {
								char sens[4]; float seuil; int lissage,derivee;
								l = k; RepartIpEnvoie(repart,buffer,l,1,0,log); k = 0; // purge les commandes precedentes
								if(VoieTampon[voie].trig.signe > 0) { strcpy(sens,"on"); seuil = VoieTampon[voie].trig.trgr->minampl; }
								else { strcpy(sens,"neg"); seuil = VoieTampon[voie].trig.trgr->maxampl; }
								lissage = (VoieManip[voie].def.trmt[TRMT_PRETRG].type == TRMT_LISSAGE)? VoieManip[voie].def.trmt[TRMT_PRETRG].p1: 0;
								derivee = (VoieTampon[voie].trig.trgr->type == TRGR_DERIVEE)? VoieTampon[voie].trig.mincount: 0.0;
								l = sprintf((char *)buffer,"t%d %s glob %g %d %d %d %d",num,sens,seuil,lissage,derivee,VoieEvent[voie].lngr,VoieEvent[voie].avant_evt);
								RepartIpEnvoie(repart,buffer,l,1,0,log);
							}
						}
					}
				#ifdef RP_RESET_MANUEL
					if(!repart->r.cali.clck) {
						if(k) buffer[k++] = '/';
						cntrl = (cntrl & 0x1F); // selection, master/slave, fin frameId reset et TS reset
						l = sprintf((char *)buffer+k,"w %x %x",CALI_REG_SEL,cntrl); k += l;
					}
				#endif /* RP_RESET_MANUEL */
					if(k) { l = k; RepartIpEnvoie(repart,buffer,l,1,0,log); } // purge les commandes precedentes
				}
				l = sprintf((char *)buffer,"p %d %x/w %x %x",repart->lec.port,d3_a_lire,CALI_REG_RUN,CALI_START); RepartIpEnvoie(repart,buffer,l,1,0,log);
			} else if(repart->famille == FAMILLE_SAMBA) {
				val = (shex)(d3_a_lire & 0xFFFF);
				SmbEcrit(repart,SMB_MARCHE,val);
				if(log) printf("%s/   . Ecrit '%s'\n",DateHeure(),RepartiteurValeurEnvoyee);
			} else if(repart->famille == FAMILLE_IPE) {
				if(repart->r.ipe.fifo < 0) l = sprintf((char *)buffer,"%s_%s",IPE_SLT_WRITE,IpeSltCmde[IPE_SLT_START]);
				else l = sprintf((char *)buffer,"%s_%s_%d",IPE_SLT_WRITE,IPE_FIFO_START,repart->r.ipe.fifo);
				RepartIpEnvoie(repart,buffer,l,0,1,log);
				buffer[0] = /* (nature == REPART_ENVOI_DETEC)? 'P':'S' */ 'P'; buffer[1] = (byte)(d3_a_lire & 0xFF);
				buffer[2] = (byte)(repart->lec.port & 0xFF);
				buffer[3] = (byte)((repart->lec.port >> 8) & 0xFF);
				RepartIpToOpera(repart,buffer,log);
			}
			if(repart->simule) ((TypeSocketIP *)(&(repart->lec.ip.skt)))->sin_addr.s_addr = repart->adrs.val;
			repart->s.ip.stampEnCours = repart->s.ip.inc_stamp; repart->s.ip.numAVenir = -1;
			if(repart->famille == FAMILLE_CALI) repart->gene.num_trame = 0;
			else repart->gene.num_trame = REPART_IP_CNTL;
			repart->gene.prochaine = 0;
			repart->gene.sample = 0;
			repart->gene.timestamp = repart->s.ip.stampEnCours;
			repart->gene.max = d3_a_lire;
			repart->gene.started = 1;
			// printf("%s/   . (%s) Timestamp initial: %lld, increment: %d\n",DateHeure(),__func__,repart->gene.timestamp,repart->s.ip.duree_trame);
		}
	} else /* pas de trames a lire, on arrete */ {
		if(repart->famille == FAMILLE_CALI) {
			l = sprintf((char *)buffer,"w %x %x",CALI_REG_RUN,CALI_STOP);
			RepartIpEnvoie(repart,buffer,l,1,1,log);
		} else if(repart->famille == FAMILLE_IPE) {
			buffer[0] = /* (nature == REPART_ENVOI_DETEC)? 'P':'S' */ 'P'; buffer[1] = (byte)(d3_a_lire & 0xFF);
			buffer[2] = (byte)(repart->lec.port & 0xFF);
			buffer[3] = (byte)((repart->lec.port >> 8) & 0xFF);
			RepartIpToOpera(repart,buffer,log);
			if(repart->r.ipe.fifo < 0) l = sprintf((char *)buffer,"%s_%s",IPE_SLT_WRITE,IpeSltCmde[IPE_SLT_STOP]);
			else l = sprintf((char *)buffer,"%s_%s_%d",IPE_SLT_WRITE,IPE_FIFO_STOP,repart->r.ipe.fifo);
			RepartIpEnvoie(repart,buffer,l,0,1,log);
		} else if(repart->famille == FAMILLE_SAMBA) {
			SmbEcrit(repart,SMB_ARRET);
			if(log >= 0) printf("%s/   . Ecrit '%s'\n",DateHeure(),RepartiteurValeurEnvoyee);
		}
		if(repart->simule) repart->gene.started = 0;
		gettimeofday(&LectDateRun,0);
		if(log >= 0) printf("%s/   . [%06d] Transmission arretee\n",DateHeure(),LectDateRun.tv_usec);
		if(!duree) {
			if((repart->lec.ip.path >= 0) && repart->fifo_pleine) {
				usleep(100000); /* 100 ms */
				if(log) printf("%s/   . Vidage de la FIFO\n",DateHeure());
				if(!repart->simule) do octets_lus = RepartIpLitTrame(repart,trame,TRAME_STD); while(octets_lus >= 0);
				repart->fifo_pleine = 0;
			}
			RepartIpLectureFerme(repart);
			if(log) printf("%s/   . Fermeture de la liaison\n",DateHeure());
		} else if(log) printf("%s/   . FIFO laissee pleine\n",DateHeure());
	#ifdef PAS_SUR
		if((duree <= 0) && (repart->famille == FAMILLE_CALI)) {
			//? empeche le vidage de la FIFO ("1 lecture") a la main pour debug: if(duree < 0) RepartIpLectureFerme(repart);
			SocketFermee(repart->ecr.ip.path); repart->ecr.ip.path = -1;
			printf("%s/   . Acces en ecriture sur %d.%d.%d.%d:%d ferme\n",DateHeure(),IP_CHAMPS(repart->adrs),repart->ecr.port);
		}
	#endif
	}
}
/* ========================================================================== */
void RepartIpContinue(TypeRepartiteur *repart, int duree, char log) {
	/* demande l'envoi des donnees pendant <duree> synchros D3. Ne ferme pas la socket si duree < 0 */
	int l; shex val; byte buffer[80];

	if(!repart) return;
	if(!RepartIpEcriturePrete(repart,"            ")) return;

	if((repart->famille == FAMILLE_OPERA) || (repart->famille == FAMILLE_IPE)) {
		buffer[0] = 'P'; buffer[1] = (byte)(duree & 0xFF);
		buffer[2] = (byte)(repart->lec.port & 0xFF);
		buffer[3] = (byte)((repart->lec.port >> 8) & 0xFF);
		RepartIpToOpera(repart,buffer,log);
	} else if(repart->famille == FAMILLE_CALI) {
//		if(!repart->r.cali.clck) {
			l = sprintf((char *)buffer,"w %x %x",CALI_REG_RUN,CALI_START);
			RepartIpEnvoie(repart,buffer,l,1,0,log);
//		}
	} else if(repart->famille == FAMILLE_SAMBA) {
		val = (shex)(duree & 0xFFFF);
		SmbEcrit(repart,SMB_MARCHE,val);
		if(log) printf("%s/   . Ecrit: '%s'\n",DateHeure(),RepartiteurValeurEnvoyee);
	}
	if(repart->simule) repart->gene.max = duree;
}
/* ==========================================================================
INLINE char RepartIpDepile1Donnee(TypeRepartiteur *repart, int32 *donnee) {
	char lue;

	if(!repart) return(0);
	lue = 0;
	if(repart->s.ip.manquant && (repart->bottom == repart->s.ip.saut)) {
		repart->s.ip.lu_entre_D3++;
		if(--repart->s.ip.manquant == 0) {
			repart->s.ip.en_defaut = 0;
			if(repart->s.ip.lu_entre_D3 >= repart->s.ip.dim_D3) repart->s.ip.lu_entre_D3 -= repart->s.ip.dim_D3;
		}
		lue = 1;
	} else if(repart->bottom < repart->top) {
		if(BigEndianOrder) *donnee = repart->data16[repart->bottom++];
		else {
			if(repart->bottom & 1) *donnee = repart->data16[repart->bottom++ - 1];
			else *donnee = repart->data16[++repart->bottom];
		}
		lue = 1;
		if(repart->bottom >= repart->top) repart->top = repart->bottom = repart->s.ip.saut = 0;
		repart->depile += 1;
	} else { repart->top = repart->bottom = repart->s.ip.saut = 0; }

	return(lue);
}
   ========================================================================== */
INLINE void RepartIpEcritureFerme(TypeRepartiteur *repart) {
	if(repart->famille == FAMILLE_CALI) {
		byte cmde[4]; strcpy((char *)cmde,"q");
		RepartIpEnvoie(repart,cmde,1,1,1,1);
	}
	if(!repart->simule) SocketFermee(repart->ecr.ip.path);
	repart->ecr.ip.path = -1;
}
/* ========================================================================== */
int32 RepartGenereEvt(TypeRepartiteur *repart, int voie, int gene, float ldb) {
	float periode,pic,reso; float fluct,evt; char demarre_evt,mode_auto,interv_pois;
	int32 val;

	evt = 0;
	if((gene = VoieManip[voie].gene) >= 0) {
		periode = Gene[gene].periode; pic = Gene[gene].pic; reso = Gene[gene].reso / 6.4; // 6.4 magique
		demarre_evt = (Gene[gene].mode_auto && (repart->gene.sample >= VoieTampon[voie].gene.futur_evt))
		           || (Gene[gene].mode_manu && (VoieTampon[voie].gene.etape < 0));
		mode_auto = Gene[gene].mode_auto; interv_pois = (Gene[gene].interv == GENE_INTERV_POIS);
	} else {
		periode = 1.4 / EnSecondes; pic = 1000.0; reso = 5.0;
		demarre_evt = (VoieTampon[voie].gene.etape < 0);
		mode_auto = interv_pois = 1;
	}

	if(demarre_evt) {
		VoieTampon[voie].gene.etape = 0;
		HasardTirage(reso,12,fluct);
		// VoieTampon[voie].gene.max = pic * (1.0 + (0.06 * fluct)) * (1.0 - 0.03 * (reso - 5.0));
		VoieTampon[voie].gene.max = pic * (1.0 + ((fluct - (0.5 * reso)) * 0.8));
		// printf("%s/ Impulsion gene#%d sur %s: %g/%g ADU\n",DateHeure(),gene,VoieManip[voie].nom,VoieTampon[voie].gene.max,Gene[gene].pic);
		if(gene >= 0) {
		//	if((gene >= 0) && Gene[gene].mode_manu) {
		//		printf("%s/ Impulsion sur %s demandee par l'utilisateur\n",DateHeure(),VoieManip[voie].nom);
		//	}
			Gene[gene].evts++;
			if(OpiumAlEcran(Gene[gene].panneau.evts)) PanelRefreshVars(Gene[gene].panneau.evts);
			if(Gene[gene].mode_manu) {
				Gene[gene].mode_manu = 0;
				if(OpiumDisplayed(Gene[gene].panneau.bouton.manu)) OpiumRefreshAgain(Gene[gene].panneau.bouton.manu);
			}
			if((VoieTampon[voie].unite.taille == 0) && (Gene[gene].panneau.message[0] == '\0')) {
				strcpy(Gene[gene].panneau.message,L_("Pas d'evenement genere (longueur template=0)"));
				PanelRefreshVars(Gene[gene].panneau.texte.msg);
			} else if((VoieTampon[voie].unite.taille > 0) && (Gene[gene].panneau.message[0] != '\0')) {
				Gene[gene].panneau.message[0] = '\0'; PanelRefreshVars(Gene[gene].panneau.texte.msg);
			}
		}
		if(mode_auto) {
			if(interv_pois) {
				do HasardTirage(1.0,12,fluct) while(fluct <= 0.0);
				VoieTampon[voie].gene.futur_evt = repart->gene.sample - (int64)(logf(fluct) * periode);
			} else VoieTampon[voie].gene.futur_evt = repart->gene.sample + (0.7 * periode);
		}
	}
	if(VoieManip[voie].modulee) {
		int diff,k; float evtJ,evtK,dev;
		diff = VoieTampon[voie].gene.compens - VoieTampon[voie].gene.modul;
		if((VoieTampon[voie].gene.etape >= 0) && (VoieTampon[voie].unite.val)) {
			k = VoieTampon[voie].gene.etape;
			if(k == 0) evtJ = 0.0;
			else evtJ = (VoieTampon[voie].gene.max * VoieTampon[voie].unite.val[k-1]);
			evtK = (VoieTampon[voie].gene.max * VoieTampon[voie].unite.val[k]);
			evt = evtJ + ((evtK - evtJ) * ((0.5 * VoieTampon[voie].gene.phase / VoieTampon[voie].gene.demiperiode) + 0.5));
			dev = (float)diff + (0.5 * evt);
			if((VoieTampon[voie].gene.phase)++ < 0) val = (int)(ldb + dev); else {
				val = (int)(ldb - dev);
				if(VoieTampon[voie].gene.phase >= VoieTampon[voie].gene.demiperiode) {
					VoieTampon[voie].gene.phase = -VoieTampon[voie].gene.demiperiode;
					if(++VoieTampon[voie].gene.etape >= VoieTampon[voie].unite.taille) VoieTampon[voie].gene.etape = -1;
				}
			}
		} else {
			if((VoieTampon[voie].gene.phase)++ < 0) val = (int)ldb + diff; else {
				val = (int)ldb - diff;
				if(VoieTampon[voie].gene.phase >= VoieTampon[voie].gene.demiperiode) {
					VoieTampon[voie].gene.phase = -VoieTampon[voie].gene.demiperiode;
				}
			}
		}
	} else {
		if(VoieTampon[voie].gene.etape >= 0) {
			if(VoieTampon[voie].unite.val) {
				evt = (VoieTampon[voie].gene.max * VoieTampon[voie].unite.val[VoieTampon[voie].gene.etape]);
			}
			if(++VoieTampon[voie].gene.etape >= VoieTampon[voie].unite.taille) VoieTampon[voie].gene.etape = -1;
		}
		val = (int)(ldb + evt);
	}
	return(val);
}
/* ========================================================================== */
static int RepartGenereTrame(TypeRepartiteur *repart, int nb_stamps, TypeDonnee *donnees, int max) {
	int nb_donnees; int t,vt,vt0,voie,gene; char genere;
	float dc,bruit; float fluct,ldb; int32 val;

	nb_donnees = 0;
	vt0 = repart->premiere_donnee;
	for(t=0; t<nb_stamps; t++) {
		for(vt=vt0; vt<SambaEchantillonLngr; vt++) {
			if(SambaEchantillon[vt].rep != repart->rep) break;
			genere = 0;
			if((voie = SambaEchantillon[vt].voie) >= 0) {
				genere = (((gene = VoieManip[voie].gene) >= 0) && Gene[gene].mode_actif);
				if(repart->simule) {
					if(genere) { dc = Gene[gene].ldb; bruit = Gene[gene].bruit * 12.0; }
					else { dc = 5.0; bruit = 50.0; }
					HasardTirage(bruit,12,fluct);
					ldb = dc + fluct - (bruit / 2.0);
				} else {
					if(BigEndianOrder) ldb = (float)donnees[nb_donnees];
					else if(nb_donnees & 1) ldb = (float)donnees[nb_donnees - 1];
					else ldb = (float)donnees[nb_donnees + 1];
				}
			} // else val = 0xCACA;
			if(genere) {
				val = RepartGenereEvt(repart,voie,gene,ldb);
				if(repart->famille == FAMILLE_CALI) { val += 0x8000; if(repart->revision > 1) val = -val; }
				if(BigEndianOrder) donnees[nb_donnees++] = (TypeDonnee)val;
				else if(nb_donnees & 1) donnees[nb_donnees++ - 1] = (TypeDonnee)val;
				else donnees[++nb_donnees] = (TypeDonnee)val;
			} else nb_donnees++;
			if(nb_donnees >= max) break;
		}
		if(nb_donnees >= max) break;
	}
	repart->gene.sample += t;
	return(nb_donnees);
}
#define DEBUG_IP
/* ========================================================================== */
INLINE ssize_t RepartIpLitTrame(TypeRepartiteur *repart, void *trame, size_t lngr) {
	int i,max,nb_stamps,nb_donnees; int64 date_us; ssize_t taille_sts,octets_lus; socklen_t taille;
	CaliTrame trame_cali; IpeTrame trame_ipe; OperaTrame trame_opera; TypeDonnee *donnees;

	taille = sizeof(TypeSocket);
#ifdef DEBUG_IP
	if(!SettingsGenePossible) {
		octets_lus = recvfrom(repart->lec.ip.path,trame,lngr,MSG_WAITALL,&(repart->lec.ip.skt),&taille);
		if((octets_lus > 0) && (repart->acces_remplis < RepartTramesDebug)) {
			int64 timestamp; int num_trame,prevu,charge_utile;
			CaliDumpTrameStrm(repart,(CaliTrame)trame,lngr,repart->acces_remplis,"          . ");
			if(!BigEndianOrder) CaliSwappeTrame(repart,(CaliTrame)trame,octets_lus);
			prevu = repart->s.ip.duree_trame * repart->nbdonnees * sizeof(TypeDonnee); // repart->s.ip.taille_derniere si num == tramesD3
			// if(repart->revision == 1) timestamp = CALI_TRAME_TIMESTAMP(trame);
			// else timestamp = repart->s.ip.stampEnCours;
			timestamp = CALI_TRAME_TIMESTAMP(trame);
			num_trame = CALI_TRAME_NUMERO(trame);
			charge_utile = octets_lus - repart->dim.entete;
			printf("          = %s, trame %12lld/%d[%d]: recue %12lld/%d[%d] (#%lld dans pile[%d])\n",
				repart->nom,repart->s.ip.stampEnCours,repart->s.ip.numAVenir,prevu,timestamp,num_trame,charge_utile,
				repart->acces_remplis,RepartIpNiveauPerte);
			// CaliDump(repart,(byte *)trame,octets_lus,1);
			if(!BigEndianOrder) CaliSwappeTrame(repart,(CaliTrame)trame,octets_lus);
		}
		return(octets_lus);
	}
#else
	if(!SettingsGenePossible) return(recvfrom(repart->lec.ip.path,trame,lngr,0,&(repart->lec.ip.skt),&taille));
#endif
	else {
		if(!repart->simule) {
			octets_lus = recvfrom(repart->lec.ip.path,trame,lngr,MSG_WAITALL,&(repart->lec.ip.skt),&taille);
			if(repart->avec_gene && (octets_lus > 0)) {
				if(repart->famille == FAMILLE_CALI) {
					trame_cali = (CaliTrame)trame;
					// printf("(%s) Lu %ld octets a l'adresse %08llX\n",__func__,octets_lus,(unsigned long long)trame);
					if(!BigEndianOrder) CaliSwappeTrame(repart,trame_cali,octets_lus);
					nb_stamps = repart->s.ip.duree_trame;
					donnees = (TypeDonnee *)(trame_cali->valeur);
					RepartGenereTrame(repart,nb_stamps,donnees,CALI_DONNEES_NB);
					if(!BigEndianOrder) CaliSwappeTrame(repart,trame_cali,octets_lus);
				}
				repart->gene.prochaine += repart->gene.intervalle; // pas d'incrementation si trame de controle
			}
			return(octets_lus);
		} else {
			octets_lus = 0;
			errno = EAGAIN;
			if(!repart->gene.started) return(-1);
			date_us = DateMicroSecs();
			if(date_us < repart->gene.prochaine) return(-1);
			max = (repart->dim.totale - repart->dim.entete) / sizeof(TypeDonnee);
			if(repart->famille == FAMILLE_CALI) {
				if(repart->gene.num_trame > repart->gene.max) { errno = EBUSY; return(-1); }
				trame_cali = (CaliTrame)trame;
				trame_cali->hdr.stamp = repart->gene.timestamp;
				trame_cali->hdr.id = (repart->gene.num_trame << 8) | 0x0A;
				int nb_dacs; nb_dacs = (repart->revision == 1)? 4: 2;
				for(i=0; i<nb_dacs; i++)
					trame_cali->hdr.status[i] = ((repart->r.cali.sel & (1 << i)))? 0xA8: 0x00;
				for( ; i<4; i++) trame_cali->hdr.status[i] = 0x00;
				nb_stamps = repart->s.ip.duree_trame;
				donnees = (TypeDonnee *)(trame_cali->valeur);
				nb_donnees = RepartGenereTrame(repart,nb_stamps,donnees,CALI_DONNEES_NB);
				octets_lus = repart->dim.entete + (nb_donnees * sizeof(TypeDonnee));
				if(!BigEndianOrder) {
					if((repart->revision == 2) || (repart->revision == 3)) {
						ByteSwappeInt(&(trame_cali->hdr.id));
						ByteSwappeInt((unsigned int *)&(trame_cali->hdr.status));
					}
					CaliSwappeTrame(repart,trame_cali,octets_lus);
				}
				repart->gene.timestamp += nb_stamps;
				repart->gene.num_trame++;
				repart->gene.prochaine += repart->gene.intervalle; // pas d'incrementation si trame de controle
			} else {
				if(repart->gene.num_trame >= REPART_IP_CNTL) {
					switch(repart->famille) {
					  case FAMILLE_OPERA:
						trame_opera = (OperaTrame)trame;
						trame_opera->identifiant = OPERA_TRAME_STS;
						((Structure_trame_status *)trame)->status_opera.temps_pd_fort = (int)(repart->gene.timestamp >> 30);
						((Structure_trame_status *)trame)->status_opera.temps_pd_faible = (int)(repart->gene.timestamp & 0x3FFFFFFF);
						taille_sts = sizeof(Structure_trame_status);
						break;
					  case FAMILLE_IPE:
						trame_ipe = (IpeTrame)trame;
						trame_ipe->type = IPE_TRAME_STS;
						trame_ipe->nb_d3 = repart->gene.timestamp / repart->s.ip.inc_stamp;
						trame_ipe->p.sts_crate.stamp_haut = (int)(repart->gene.timestamp >> 30);
						trame_ipe->p.sts_crate.stamp_bas = (int)(repart->gene.timestamp & 0x3FFFFFFF);
						trame_ipe->p.sts_crate.temps_seconde = trame_ipe->nb_d3;
						trame_ipe->p.sts_crate.dim = sizeof(TypeIpeCrateSts);
						trame_ipe->p.sts_crate.SLTTimeLow = (uint32_t)(repart->gene.timestamp & 0xFFFFFFFF);
						trame_ipe->p.sts_crate.SLTTimeHigh = (uint32_t)(repart->gene.timestamp >> 32);
						trame_ipe->p.sts_crate.d0 = (uint32_t)(ModeleRep[repart->type].diviseur0);
						trame_ipe->p.sts_crate.pixbus_enable = (uint32_t)IpePixBus[repart->r.ipe.chassis];
						trame_ipe->p.sts_crate.micro_bbv2 = (uint32_t)(100 << 8);
						trame_ipe->p.sts_crate.error_counter = (uint32_t)0;
						trame_ipe->p.sts_crate.internal_error_info = (uint32_t)0;
						trame_ipe->p.sts_crate.ipe4reader_status = (uint32_t)0;
						trame_ipe->p.sts_crate.version = (uint32_t)0;
						trame_ipe->p.sts_crate.numFIFOnumADCs = (uint32_t)((6 << 16) | 120); // 6 FIFOs sur 4 x 5 FLT de 6 voies chacune
						trame_ipe->p.sts_crate.maxUDPSize = (uint32_t)(repart->dim.totale);
						taille_sts = sizeof(TypeIpeCrateSts);
						break;
					}
					octets_lus = repart->dim.entete + taille_sts;
					// if(repart->acces_remplis < 10) printf("%s/ %s[%lld], trame[%d] de status generee %12lld/%-3d\n",DateHeure(),
					// 	repart->nom,repart->acces_remplis,octets_lus,repart->gene.timestamp,IPE_TRAME_STS);
					repart->gene.num_trame = 0;
				} else {
					switch(repart->famille) {
					  case FAMILLE_OPERA:
						trame_opera = (OperaTrame)trame;
						trame_opera->identifiant = (((repart->gene.timestamp / repart->s.ip.inc_stamp) << 16) & 0x3FFF0000) & (repart->gene.num_trame & 0xFFFF);
						donnees = (TypeDonnee *)(trame_opera->data);
						break;
					  case FAMILLE_IPE:
						trame_ipe = (IpeTrame)trame;
						trame_ipe->type = repart->gene.num_trame;
						trame_ipe->nb_d3 = repart->gene.timestamp / repart->s.ip.inc_stamp;
						//- TwoShortsSwappeInt((hexa *)trame);
						donnees = (TypeDonnee *)(trame_ipe->p.donnees.val);
						break;
					}
					if(repart->gene.num_trame == repart->s.ip.tramesD3) nb_stamps = repart->s.ip.taille_derniere;
					else nb_stamps = repart->s.ip.duree_trame;
					nb_donnees = RepartGenereTrame(repart,nb_stamps,donnees,max);
					octets_lus = repart->dim.entete + (nb_donnees * sizeof(TypeDonnee));
					// if(repart->acces_remplis < 10) printf("%s/ %s[%lld], trame[%d] de donnees generee %12lld/%-3d\n",DateHeure(),
					// 	repart->nom,repart->acces_remplis,octets_lus,repart->gene.timestamp,repart->gene.num_trame);
					if(repart->gene.num_trame < repart->s.ip.tramesD3) {
						repart->gene.num_trame++;
					} else {
						repart->gene.timestamp += repart->s.ip.inc_stamp;
						repart->gene.num_trame = REPART_IP_CNTL;
					}
					repart->gene.prochaine += repart->gene.intervalle; // pas d'incrementation si trame de controle
				}
				// if(repart->acces_remplis < 10) IpeDump((byte *)trame,octets_lus,1);
				if(BigEndianOrder) switch(repart->famille) {
					case FAMILLE_OPERA: OperaSwappeTrame(trame,octets_lus); break;
					case FAMILLE_IPE: IpeSwappeTrame(trame,octets_lus); break;
				}
			}
			errno = 0;
			return(octets_lus);
		}
	}
}
/* ========================================================================== */
INLINE void RepartIpLectureFerme(TypeRepartiteur *repart) {
	if(!repart->simule) SocketFermee(repart->lec.ip.path);
	printf("%s/   . Acces en lecture depuis %d.%d.%d.%d:%d ferme\n",DateHeure(),IP_CHAMPS(repart->adrs),repart->lec.port);
	repart->lec.ip.path = -1;
}
/* ========================================================================== */
INLINE char RepartiteurEcritRessourceBB(TypeRepartiteur *repart, short entree, shex adrs, hexa ss_adrs, TypeADU valeur) {
	if(!repart) { sprintf(RepartiteurValeurEnvoyee,"! Repartiteur indetermine"); return(0); }
	else if(repart->famille == FAMILLE_CLUZEL) CluzelEcritRessourceBB(repart,adrs,ss_adrs,valeur);
	else switch(repart->interf) {
	  case INTERF_IP:
		if(!RepartIpEcriturePrete(repart,"            ")) {
			printf("            ! Connexion pour ecriture sur %s[%d.%d.%d.%d:%d] impossible\n",repart->nom,IP_CHAMPS(repart->adrs),repart->ecr.port);
			sprintf(RepartiteurValeurEnvoyee,"! Connexion impossible"); return(0);
		}
		switch(repart->famille) {
		  case FAMILLE_OPERA: OperaEcritRessourceBB(repart,adrs,ss_adrs,valeur); break;
		  case FAMILLE_CALI:  CaliEcritRessourceBB(repart,adrs,ss_adrs,valeur);  break;
		  case FAMILLE_SAMBA: SmbEcrit(repart,SMB_NUMER,(byte)adrs,(byte)ss_adrs,valeur);  break;
		  case FAMILLE_IPE:   IpeEcritRessourceBB(repart,entree,adrs,ss_adrs,valeur); break;
		}
		break;
	  case INTERF_NAT:   break;
	  case INTERF_USB:  break;
	  case INTERF_FILE: break;
	}
	return(1);
}
/* ========================================================================== */
INLINE char RepartiteurEcritNumeriseur(TypeRepartiteur *repart, short entree, byte *cmde, int lngr) {
	byte buffer[256]; int i,j,k,n; char ascii,ok;

	i = 0; ascii = 0; ok = 1;
	if(!repart) { sprintf(RepartiteurValeurEnvoyee,"! Repartiteur indetermine"); return(0); }
	else if(repart->famille == FAMILLE_CLUZEL) {
		// a refaire: CluzelEcritRessourceBB(repart,adrs,ss_adrs,valeur);
		// sinon: sprintf(RepartiteurValeurEnvoyee,"! Repartiteur non supporte"); return(0);
		ok = 0;
	#ifdef AVEC_PCI
		if(repart->p.pci.port) {
			if(repart->adrs.val <= PCInb) {
				byte adrs,ss_adrs; TypeADU valeur;
				adrs = cmde[0]; ss_adrs = cmde[1]; valeur = (((TypeADU)cmde[2] << 8) & 0xFF00) | ((TypeADU)cmde[3] & 0xFF);
				IcaCommandSend(repart->p.pci.port,adrs,CLUZEL_IMMEDIAT,ss_adrs,valeur);
				sprintf(RepartiteurValeurEnvoyee,"%02X.%02X.%04X",adrs,ss_adrs,valeur);
				ok = 1;
			} else sprintf(RepartiteurValeurEnvoyee,"(Acces PCI invalide)");
		} else
	#endif
			sprintf(RepartiteurValeurEnvoyee,"(pas d'acces PCI)");
	} else switch(repart->interf) {
		case INTERF_IP:
			if(!RepartIpEcriturePrete(repart,"            ")) {
				printf("            ! Connexion pour ecriture sur %s[%d.%d.%d.%d:%d] impossible\n",repart->nom,IP_CHAMPS(repart->adrs),repart->ecr.port);
				sprintf(RepartiteurValeurEnvoyee,"! Connexion impossible"); return(0);
			}
			switch(repart->famille) {
				case FAMILLE_OPERA: buffer[i++] = OPERA_CDE_BB; buffer[i++] = _FPGA_code_EDW; for(j=0; j<lngr; j++) buffer[i++] = cmde[j]; break;
				case FAMILLE_CALI:  for(j=0; j<lngr; j++) buffer[i++] = cmde[j]; ascii = 1; break;
				case FAMILLE_SAMBA: buffer[i++] = SMB_NUMER; buffer[i++] = 0; for(j=0; j<lngr; j++) buffer[i++] = cmde[j]; break;
				case FAMILLE_IPE:
					if(repart->r.ipe.ports_BB || (entree < 0)) {
						buffer[i++] = OPERA_CDE_BB; buffer[i++] = _FPGA_code_EDW; for(j=0; j<lngr; j++) buffer[i++] = cmde[j];
					} else {
						int flt,fbr;
						flt = IPE_FLT_NUM(entree); fbr = IPE_FLT_FBR(entree,flt);
						i = sprintf((char *)buffer,"%s_sendBBCmd_",IPE_REG_WRITE);
						for(j=0; j<lngr; j++) i += sprintf((char *)buffer+i,"_0x%02X",cmde[j]);
						i += sprintf((char *)buffer+i,"_FLT_%d_FIBER_%d",IPE_FLT_ABS(repart,flt)+1,fbr);
						ascii = 1;
					}
					break;
			}
			if(ascii) buffer[i++] = '\0';
			n = RepartIpEcrit(repart,buffer,i); SambaMicrosec(ModeleRep[repart->type].delai_msg);
			if(ascii) strncpy((char *)RepartiteurValeurEnvoyee,(char *)buffer,REPART_MAXMSG);
			else {
				j = sprintf(RepartiteurValeurEnvoyee,"%c",buffer[0]);
				for(k=1; k<i; k++) j += sprintf(RepartiteurValeurEnvoyee+j,".%02X",buffer[k]);
			}
			break;
		case INTERF_USB:
			UsbWrite(repart->ecr.ip.path,(char *)cmde,RepartiteurValeurEnvoyee);
			break;
		case INTERF_NAT:  break;
		case INTERF_FILE: break;
	}
	// printf("(%s) Transmis via %s: %s\n",__func__,repart->nom,RepartiteurValeurEnvoyee);
	return(ok);
}
/* ========================================================================== */
void RepartiteurChangeMode(TypeRepartiteur *repart, char mode, char log) {
	char *prefixe;

	if(log) prefixe = "            "; else prefixe = 0;
	if(log) switch(repart->famille) {
	  case FAMILLE_CALI: printf("%s* %s: passage en mode %s\n",prefixe,repart->nom,CaliCodesDataMode[mode]); break;
	  case FAMILLE_IPE:  printf("%s* %s: passage en mode %s\n",prefixe,repart->nom,IpeCodesDataMode[mode]); break;
	}

	switch(repart->famille) {
	  case FAMILLE_CALI: CaliChangeMode(repart,mode,prefixe); repart->mode = CaliRepDataMode[mode]; break;
	  case FAMILLE_IPE:  IpeChangeMode(repart,mode,prefixe);  repart->mode = IpeRepDataMode[mode]; break;
	}
}
/* ========================================================================== */
void RepartiteurDemarreNumeriseurs(TypeRepartiteur *repart, NUMER_MODE mode, byte adrs, char log) {
	char *prefixe;

	if(log) prefixe = "            "; else prefixe = 0;
	if(log) {
		if((repart->nbentrees > 1) && (adrs == NUMER_DIFFUSION))
			printf("%s* %s: passage des %d numeriseurs lus en mode %s\n",prefixe,repart->nom,repart->nbentrees,NumerModeTexte[mode]);
		else if(adrs == NUMER_DIFFUSION) printf("%s* %s: passage du numeriseur en mode %s\n",prefixe,repart->nom,NumerModeTexte[mode]);
		else printf("%s* %s: passage du numeriseur @%02X (%d) en mode %s\n",prefixe,repart->nom,adrs,NumerModeTexte[mode]);
	}

	if(repart->famille == FAMILLE_CLUZEL) CluzelDemarreNumeriseurs(repart,mode,adrs,prefixe);
	else switch(repart->interf) {
	  case INTERF_IP:
		if(!RepartIpEcriturePrete(repart,"            ")) {
			printf("            ! Connexion pour ecriture sur %s[%d.%d.%d.%d:%d] impossible\n",repart->nom,IP_CHAMPS(repart->adrs),repart->ecr.port);
			return;
		}
		switch(repart->famille) {
		  case FAMILLE_OPERA: OperaDemarreNumeriseurs(repart,mode,adrs,prefixe); break;
		  case FAMILLE_SAMBA: SmbDemarreNumeriseurs(repart,mode,adrs,prefixe); break;
		  case FAMILLE_IPE:   IpeDemarreNumeriseurs(repart,mode,adrs,prefixe);   break;
		}
		break;
	  case INTERF_NAT:   break;
	  case INTERF_USB:  break;
	  case INTERF_FILE: break;
	}
}
/* ========================================================================== */
char RepartiteurDemarreTransmissions(TypeRepartiteur *repart, char log) {

	if(log) printf("%s/ . Demarrage %s %s",DateHeure(),ModeleRep[repart->type].nom,repart->nom);

	/* ............................. */
	if(repart->interf == INTERF_PCI) {
		if(log) printf(" @PCI #%d\n",repart->adrs.val);
		if(repart->famille == FAMILLE_CLUZEL) {
			char securite; int fifo_dim;
			/* Au fait... on peut vraiment lire des donnees? */
			if(!PCIconnecte) {
				printf("%s/   !!! Pas de carte PCI disponible, execution abandonnee\n",DateHeure());
				OpiumError("Pas de carte PCI accessible pour lire des donnees");
				return(0);
			}
		#ifdef AVEC_PCI
			IcaXferStatusReset(repart->p.pci.port);
		#endif /* AVEC_PCI */
			securite = (ModeleRep[repart->type].version < 2.0);
			if(!CluzelRAZFifo(repart->p.pci.port,repart->octets,securite,log)) return(0);
			fifo_dim = repart->octets / sizeof(Ulong);
		#ifdef CLUZEL_FIFO_INTERNE
			if(repart->fifo32) { int i; for(i=0; i<fifo_dim ; i++) repart->fifo32[i] = 0;  /* FIFO pas mise a jour si FIFO carte MiG deja vide */ }
		#endif
			CluzelEnvoieCommande(repart->p.pci.port,CLUZEL_ADRS,CLUZEL_SSAD_ACQUIS,CLUZEL_START,CLUZEL_MODULEE);
			printf("%s/   . Ecrit: %X.%02X.%02X%02X\n",DateHeure(),
				   0x100 | CLUZEL_ADRS, ((CLUZEL_MODULEE << 6) & 0xC0) | 0x08, /* 0x08 == CLUZEL_SSAD_ACQUIS */
				   (CLUZEL_START >> 8) & 0xFF,CLUZEL_START & 0xFF);
		}

		/* ............................. */
	} else if(repart->interf == INTERF_IP) {
		if(log) printf(" @IP %d.%d.%d.%d [%d]\n",IP_CHAMPS(repart->adrs),repart->s.ip.relance);
		RepartIpTransmission(repart,REPART_ENVOI_DETEC,repart->s.ip.relance,log);

		/* ............................. */
#ifdef NIDAQ_A_VOIR
	} else if(repart->interf == INTERF_NAT) {
		int num;
		num = repart->adrs.val - 1;
		// if(LectNIactive[num]) { if(!SambaTesteDAQmx("Arret du DMA",DAQmxBaseStopTask(LectNITache[num]),log)) LectNIactive[num] = 0; }
		if(!LectNIactive[num]) {
			if(!SambaTesteDAQmx("Demarrage du DMA",DAQmxBaseStartTask(LectNITache[num]),log)) LectNIactive[num] = 1;
		}
#endif

		/* ............................. */
	} else if(repart->interf == INTERF_FILE) {

		/* ............................. */
	} else if(log) printf(": non supporte (%d numeriseurs associes)\n",repart->nbentrees);

	repart->en_route = 1;
#ifndef CODE_WARRIOR_VSN
	gettimeofday(&LectDateRun,0);
	if(log) printf("%s/   . [%06d] Transmission demarree\n",DateHeure(),LectDateRun.tv_usec);
#endif

	return(1);
}
/* ========================================================================== */
char RepartiteurArretTransmissions(TypeRepartiteur *repart, char avec_vidage, char log) {
	int duree;

	if(log) printf("%s/ . Arret %s %s",DateHeure(),ModeleRep[repart->type].nom,repart->nom);
	if(repart->interf == INTERF_PCI) {
		if(log) printf(" @PCI #%d\n",repart->adrs.val);
		if(repart->famille == FAMILLE_CLUZEL) {
			if(!PCIconnecte) return(0);
			CluzelEnvoieCommande(repart->p.pci.port,CLUZEL_ADRS,CLUZEL_SSAD_ACQUIS,CLUZEL_STOP,CLUZEL_MODULEE);
		}
	} else if(repart->interf == INTERF_IP) {
		if(log) printf(" @IP %d.%d.%d.%d\n",IP_CHAMPS(repart->adrs));
		duree = avec_vidage? 0: -1;
		RepartIpTransmission(repart,REPART_ENVOI_DETEC,duree,log);
#ifdef NIDAQ
	} else if(repart->interf == INTERF_NAT) {
		int num;
		num = repart->adrs.val - 1;
		if(LectNIactive[num]) { if(!SambaTesteDAQmx("Arret du DMA",DAQmxBaseStopTask(LectNITache[num]),log)) LectNIactive[num] = 0; }
#endif
	} else if(log) printf(": non supporte (%d numeriseur%s associe%s)\n",Accord2s(repart->nbentrees));
	repart->en_route = 0;

	return(1);
}
/* ========================================================================== */
char RepartiteurSuspendTransmissions(TypeRepartiteur *repart, char log) {
	return(RepartiteurArretTransmissions(repart,0,log));
}
/* ========================================================================== */
char RepartiteurStoppeTransmissions(TypeRepartiteur *repart, char log) {
	return(RepartiteurArretTransmissions(repart,1,log));
}
/* ========================================================================== */
char RepartiteurRepereD2(TypeRepartiteur *repart, lhex *val1, char log) {
	/* Doit rendre repart->bottom et repart->top cales sur une synchro D2 */
	int nbsleeps;  /* nombre maxi d'attentes avant abandon */
	int timesleep; /* attente en microsecs si pile vide (pas de donnees) */
	float timeout; /* delai maxi en secondes avant abandon */
	char trouvee;
	TypeADU marque_attendue;
	lhex mot_lu;
	int num_trame;


	*val1 = 0;
	trouvee = 0;
    mot_lu = 0;
	num_trame = -1; // GCC

	if(repart->famille == FAMILLE_CLUZEL) {
#ifdef ACCES_PCI_DIRECT
		int examuser,numpt;
#endif
		repart->top = repart->bottom = FifoTopPrec = 0;
		if(ModeleRep[repart->type].revision >= 0) marque_attendue = CLUZEL_MARQUE_MODULATION;
		else marque_attendue = CLUZEL_MARQUE_ECHANT;
#ifdef ACCES_PCI_DIRECT
		nbsleeps = SettingsFIFOwait * LectBlockSize * Diviseur2;
		nbsleeps = 2000;
		if(log) printf("%s/   . Attente d'une synchro D2 (%04X), au plus %d lectures a %08X\n",DateHeure(),marque_attendue,nbsleeps,PCIadrs[repart->rep]);
		examuser = SettingsIntervUser * Echantillonnage; numpt = 0;
		do {
			if(++numpt == examuser) { OpiumUserAction(); numpt = 0; }
			if(LectureSuspendue) sleep(1);
			else mot_lu = *(PCIadrs[repart->rep]); // IcaFifoRead(repart->p.pci.port);
			// printf("%5d: %08X\n",2001-nbsleeps,mot_lu);
		} while((DATATYPE(mot_lu) != marque_attendue) && (nbsleeps-- > 0));
#else
		timesleep = 1000; /* microsecs */
		nbsleeps = 2000; /* fois timesleep */
		timeout = (float)(nbsleeps*timesleep)/1000000.0;
		if(log) printf("%s/   . Attente d'une synchro D2 (%04X), au plus %g secondes\n",DateHeure(),marque_attendue,timeout);
		do {
		#ifdef AVEC_PCI
			IcaXferExec(repart->p.pci.port);
		#endif
			printf("%s/     : Transfert demande\n",DateHeure());
			repart->s.pci.en_cours = 1;
			repart->bottom = 0;
			//+ do SambaMicrosec(timesleep); while(((repart->top = IcaXferDone(repart->p.pci.port)) == 0) && (--nbsleeps > 0));
			do {
				SambaMicrosec(timesleep);
			#ifdef AVEC_PCI
				repart->top = IcaXferDone(repart->p.pci.port);
			#else
				repart->top = 0;
			#endif
				printf("%s/     : Transfert termine avec %d donnee%s, reste %d tentative%s\n",
					   DateHeure(),Accord1s(repart->top),Accord1s(nbsleeps));
			} while((repart->top <= 0) && (--nbsleeps > 0));
			if(repart->top < 0) {
				printf("%s/   ! Transfert avorte sur erreur #%d\n",DateHeure(),IcaDerniereErreur());
				repart->top = 0;
				break;
			}
			repart->s.pci.en_cours = 0;
			while(repart->bottom < repart->top) {
				mot_lu = repart->fifo32[repart->bottom];
				if(DATATYPE(mot_lu) == marque_attendue) break;
				if(CLUZEL_Empty(mot_lu)) break;
				repart->bottom++;
			};
		} while((DATATYPE(mot_lu) != marque_attendue) && (--nbsleeps > 0));
#endif /* ACCES_PCI_DIRECT */
		if(DATATYPE(mot_lu) == marque_attendue) {
			trouvee = 1;
			repart->acces_demandes++;
#ifdef ACCES_PCI_DIRECT
#ifdef CLUZEL_FIFO_INTERNE
			repart->fifo32[repart->bottom] = mot_lu;
			repart->top = repart->bottom + 1;
#endif
#endif /* ACCES_PCI_DIRECT */
#ifdef DEBUG_ACCES
			LectPciAdrs = 1;
#endif
#ifdef DEBUG_RAW
			LectRawNext = 0;
			LectRaw[LectRawNext++] = mot_lu;
			LectNonNul = 1;
#endif
			/*			printf("%s/ . Modulation trouvee, lecture active\n",DateHeure()); vire cause perte synchro (?) */
		} else {
			trouvee = 0;
			printf("%s/   !!! Pas de modulation dans la pile de %d valeurs\n",DateHeure(),repart->top);
			repart->bottom = 0; trouvee = 0;
			{
				int n,m,i,apres;
				int blocksize;
				blocksize = repart->nbdonnees; if(ModeleRep[repart->type].status) blocksize++;
				printf("Fifo interne [%d:%d]:",repart->bottom,repart->top);
				n = 0; apres = 2000;
				m = (repart->top < repart->bottom+apres)? repart->top: repart->bottom+apres;
				for(i=n; i<m; i++) {
					if(!(i % blocksize)) printf("\n%5d:",i);
					if(!((i % blocksize) % repart->nbentrees)) printf(" ");
					printf(" %08X",repart->fifo32[i]);
				}
				if(repart->top <= 0) printf(" vide"); printf("\n");
			}
		}

#ifdef NIDAQ
	} else if(repart->famille == FAMILLE_NI) {
		int limite,nb;
		int num;
		repart->top = repart->bottom = FifoTopPrec = 0;
		timeout = 10.0;
		if(log) printf("%s/   . Attente d'une donnee, au plus %g secondes\n",DateHeure(),timeout);
		num = repart->adrs.val - 1;
		if(!LectNIactive[num]) {
			if(!SambaTesteDAQmx("Demarrage du DMA",DAQmxBaseStartTask(LectNITache[num]),log)) LectNIactive[num] = 1;
		}
		gettimeofday(&LectDateRun,0); limite = LectDateRun.tv_sec + (int)timeout;
		do {
			repart->acces_demandes++;
			SambaTesteDAQmx("Lecture de donnees",DAQmxBaseReadBinaryI16(LectNITache[repart->adrs.val-1],DAQmx_Val_Auto,3.0,DAQmx_Val_GroupByScanNumber,(int16 *)(repart->data16 + repart->top),FIFOdim - repart->top,&nb,NULL),log);
			if(nb) {
				repart->acces_remplis++;
				repart->top += nb;
				repart->donnees_recues += nb;
				mot_lu = (lhex)(repart->data16[repart->bottom]);
				trouvee = 1; break;
			} else SambaMicrosec(1000);
			gettimeofday(&LectDateRun,0);
		} while(LectDateRun.tv_sec < limite);
#endif

#ifdef AVEC_OPERA
	} else if(repart->famille == FAMILLE_OPERA) {
		int limite;
		hexa lngr; int octets_lus,donnees_lues; OperaTrame trame;
		int num_bloc,blocs_a_sauter;
		timeout = 5.0;
		if(log) printf("%s/   . Attente d'une synchro D2, au plus %g secondes\n",DateHeure(),timeout);
		gettimeofday(&LectDateRun,0); limite = LectDateRun.tv_sec + (int)timeout;
		trame = (OperaTrame)repart->fifo32;
		lngr = sizeof(TypeSocket);
		do {
			octets_lus = RepartIpLitTrame(repart,trame,sizeof(TypeOperaTrame));
			if(octets_lus == -1) {
				if(errno == EAGAIN) SambaMicrosec(1000);
				else { printf("%s/     ! Repartiteur illisible (%s)\n",DateHeure(),strerror(errno)); return(0); }
			} else if(octets_lus > 0) {
				if(BigEndianOrder) ByteSwappeIntArray((hexa *)trame,octets_lus / sizeof(Ulong));
				// #ifdef DEBUG_STATUS_BBV2
				//	if(LectLitStatus && (OPERA_TRAME_NUMERO(trame) < MAXTRAMES)) memcpy(&(BOtrame0[OPERA_TRAME_NUMERO(trame)][0]),trame,sizeof(TypeOperaTrame));
				// #endif
				num_trame = OPERA_TRAME_NUMERO(trame);
				if(num_trame == OPERA_TRAME_STS) {
					repart->s.ip.stampDernier = OPERA_TRAME_TIMESTAMP(trame);
					gettimeofday(&LectDateRun,0);
					continue;
				}
				num_bloc = (num_trame *  repart->s.ip.duree_trame) % Diviseur2;
				if(num_bloc == 0) blocs_a_sauter = 0;
				else blocs_a_sauter = Diviseur2 - num_bloc;
				RepartIpOctetsLus = octets_lus;
				donnees_lues = (octets_lus - OPERA_TAILLE_ENTETE) / sizeof(TypeDonnee); /* 2 octets par voie */
				repart->acces_remplis++;
				repart->top = donnees_lues;
				repart->donnees_recues = donnees_lues;
				repart->bottom = blocs_a_sauter * repart->nbdonnees;
				if(repart->bottom < repart->top) {
				#ifdef OPERA_STATUS_DANS_ENTETE
					repart->s.ip.stamp0 = OPERA_TRAME_TIMESTAMP(trame);
				#else
					repart->s.ip.stamp0 = repart->s.ip.stampDernier;
				#endif
					repart->s.ip.num0 = OPERA_TRAME_NUMERO(trame); /* insuffisant, manque le decalage <blocs_a_sauter> */
					printf("%s/   . Synchro D2 fixee au mot %d de la trame #%d\n",DateHeure(),repart->bottom,repart->s.ip.num0);
					if(BigEndianOrder) mot_lu = (lhex)(repart->data16[repart->bottom]) & 0xFFFF;
					else if(repart->bottom & 1) mot_lu = (lhex)(repart->data16[repart->bottom - 1]) & 0xFFFF;
					else mot_lu = (lhex)(repart->data16[repart->bottom + 1]) & 0xFFFF;
					trouvee = 1; break;
				}
			}
			gettimeofday(&LectDateRun,0);
		} while(LectDateRun.tv_sec < limite);
#endif

	} else if(repart->famille == FAMILLE_IPE) {
		int limite;
		hexa lngr; int octets_lus,donnees_lues; IpeTrame *trame;
		// shex num_trame;
		int num_bloc,blocs_a_sauter;
		timeout = 5.0;
		if(log) printf("%s/   . Attente d'une synchro D2, au plus %g secondes\n",DateHeure(),timeout);
		gettimeofday(&LectDateRun,0); limite = LectDateRun.tv_sec + (int)timeout;
		trame = (IpeTrame *)repart->fifo32;
		lngr = sizeof(TypeSocket);
		do {
			octets_lus = RepartIpLitTrame(repart,trame,sizeof(IpeTrame));
			if(octets_lus == -1) {
				if(errno == EAGAIN) SambaMicrosec(1000);
				else { printf("%s/     ! Repartiteur illisible (%s)\n",DateHeure(),strerror(errno)); return(0); }
			} else if(octets_lus > 0) {
				if(BigEndianOrder) ByteSwappeIntArray((hexa *)trame,octets_lus / sizeof(Ulong));
				// #ifdef DEBUG_STATUS_BBV2
				//	if(LectLitStatus && (OPERA_TRAME_NUMERO(trame) < MAXTRAMES)) memcpy(&(BOtrame0[OPERA_TRAME_NUMERO(trame)][0]),trame,sizeof(TypeOperaTrame));
				// #endif
				num_trame = IPE_TRAME_NUMERO(trame);
				if(num_trame == IPE_TRAME_STS) {
					repart->s.ip.stampDernier = IPE_TRAME_TIMESTAMP(trame);
					gettimeofday(&LectDateRun,0);
					continue;
				} else if(num_trame > REPART_IP_CNTL) continue;
				num_bloc = (num_trame *  repart->s.ip.duree_trame) % Diviseur2;
				if(num_bloc == 0) blocs_a_sauter = 0;
				else blocs_a_sauter = Diviseur2 - num_bloc;
				RepartIpOctetsLus = octets_lus;
				donnees_lues = (octets_lus - IPE_TAILLE_ENTETE) / sizeof(TypeDonnee); /* 2 octets par voie */
				repart->acces_remplis++;
				repart->top = donnees_lues;
				repart->donnees_recues = donnees_lues;
				repart->bottom = blocs_a_sauter * repart->nbdonnees;
				/* NE TROUVE PAS!! */
				if(repart->bottom < repart->top) {
					repart->s.ip.stamp0 = repart->s.ip.stampDernier;
					repart->s.ip.num0 = num_trame; /* insuffisant, manque le decalage <blocs_a_sauter> */
					printf("%s/   . Synchro D2 fixee au mot %d de la trame #%d\n",DateHeure(),repart->bottom,repart->s.ip.num0);
					if(BigEndianOrder) mot_lu = (lhex)(repart->data16[repart->bottom]) & 0xFFFF;
					else if(repart->bottom & 1) mot_lu = (lhex)(repart->data16[repart->bottom - 1]) & 0xFFFF;
					else mot_lu = (lhex)(repart->data16[repart->bottom + 1]) & 0xFFFF;
					trouvee = 1; break;
				}
			}
			gettimeofday(&LectDateRun,0);
		} while(LectDateRun.tv_sec < limite);

	} else if(repart->famille == FAMILLE_CALI) {
		int limite;
		hexa lngr; int octets_lus,donnees_lues; CaliTrame trame;
		timeout = 5.0;
		if(log) printf("%s/   . Attente d'une synchro D2, au plus %g secondes\n",DateHeure(),timeout);
		gettimeofday(&LectDateRun,0); limite = LectDateRun.tv_sec + (int)timeout;
		trame = (CaliTrame)repart->fifo32;
		lngr = sizeof(TypeSocket);
		do {
			octets_lus = RepartIpLitTrame(repart,trame,sizeof(TypeCaliTrame));
			if(octets_lus == -1) {
				if(errno == EAGAIN) SambaMicrosec(1000);
				else { printf("%s/     ! Repartiteur illisible (%s)\n",DateHeure(),strerror(errno)); return(0); }
			} else if(octets_lus > 0) {
				if(!BigEndianOrder) CaliSwappeTrame(repart,trame,octets_lus);
				num_trame = CALI_TRAME_NUMERO(trame);
				repart->s.ip.stampDernier = CALI_TRAME_TIMESTAMP(trame);
				repart->s.ip.stampEnCours = repart->s.ip.stampDernier;
				repart->s.ip.numAVenir = -1;
				printf("%s/   . Trame recue: 0x%012llx/%d[%d]\n",DateHeure(),repart->nom,repart->s.ip.stampEnCours,num_trame,octets_lus);
				gettimeofday(&LectDateRun,0);
				RepartIpOctetsLus = octets_lus;
				donnees_lues = (octets_lus - CALI_TAILLE_ENTETE) / sizeof(TypeDonnee); /* 2 octets par voie */
				repart->acces_remplis++;
				repart->top = donnees_lues;
				repart->donnees_recues = donnees_lues;
				repart->bottom = 0;
				if(repart->bottom < repart->top) {
					repart->s.ip.stamp0 = repart->s.ip.stampDernier;
					repart->s.ip.num0 = num_trame; /* insuffisant, manque le decalage <blocs_a_sauter> */
					printf("%s/   . Synchro D2 fixee au mot %d de la trame #%d\n",DateHeure(),repart->bottom,repart->s.ip.num0);
					if(!BigEndianOrder) mot_lu = (lhex)(repart->data16[repart->bottom]) & 0xFFFF;
					else if(repart->bottom & 1) mot_lu = (lhex)(repart->data16[repart->bottom - 1]) & 0xFFFF;
					else mot_lu = (lhex)(repart->data16[repart->bottom + 1]) & 0xFFFF;
					trouvee = 1; break;
				}
			}
			gettimeofday(&LectDateRun,0);
		} while(LectDateRun.tv_sec < limite);

	}
	*val1 = mot_lu;
	if(trouvee) {
		repart->s.ip.stampEnCours = repart->s.ip.stampDernier;
		repart->s.ip.numAVenir = num_trame + 1;
	}
	return(trouvee);
}
/* ========================================================================== */
char RepartiteurRepereD3(TypeRepartiteur *repart, lhex *val1, char log) {
	/* Doit rendre repart->bottom et repart->top cales sur une synchro D3 */
	int nb_lues;
	int timeout; /* delai maxi en secondes avant abandon */
	int timesleep; /* attente en microsecs si pile vide (pas de donnees) */
	int limite;
	char trouvee;
	lhex mot_lu;
#ifdef AVEC_PCI
	TypeADU marque_attendue; TypeDonnee val;
	int i;
#endif

	*val1 = 0;
#ifdef CODE_WARRIOR_VSN
	return(1);
#else
    trouvee = 0; nb_lues = 0; mot_lu = 0;
	timeout = 5; /* secs */
	timesleep = 1000; /* microsecs */
	gettimeofday(&LectDateRun,0); limite = LectDateRun.tv_sec + timeout;
	if(log) printf("%s/   . [%06d] Attente d'une synchro D3, au plus %d secondes\n",DateHeure(),LectDateRun.tv_usec,timeout);
	if(repart->famille == FAMILLE_CLUZEL) {
	#ifdef AVEC_PCI
		if(!ModeleRep[repart->type].status) return(0);
		marque_attendue = CLUZEL_MARQUE_MODULATION;
		repart->s.pci.en_cours = 0;
		do {
			if(!(repart->s.pci.en_cours)) {
				repart->top = repart->bottom = 0;
				IcaXferExec(repart->p.pci.port); repart->s.pci.en_cours = 1;
			}
			repart->top = IcaXferDone(repart->p.pci.port);
			if(repart->top < 0) {
				printf("%s/   !          Transfert avorte sur erreur #%d\n",DateHeure(),IcaDerniereErreur());
				repart->top = 0;
				break;
			} else if(repart->top == 0) SambaMicrosec(timesleep);
			else {
				repart->s.pci.en_cours = 0;
				while(repart->bottom < repart->top) {
					mot_lu = repart->fifo32[repart->bottom];
					if(CLUZEL_Empty(mot_lu)) break;
					nb_lues++;
					/* se caler sur les modulations puis fin de bloc */
					if(DATATYPE(mot_lu) == marque_attendue) {
						i = 1;
						while(i < repart->nbdonnees) {
							val = repart->fifo32[repart->bottom+i];
							if(CLUZEL_Empty(val)) break;
							i++; nb_lues++;
						}
						if(i < repart->nbdonnees) {
							/* on est monstre embetes, dans ce cas... */
							repart->bottom += (i - 1);
							break;
						} else {
							val = repart->fifo32[repart->bottom+i];
							if(CLUZEL_Empty(val)) {
								/* c'est pas mieux */
								repart->bottom += (i - 1);
								break;
							}
							if(val & repart->p.pci.bitD2D3) { repart->D3trouvee = 1; trouvee = 1; break; }
							repart->bottom += i;
						}
					}
					repart->bottom++;
				}
				if(trouvee) break;
			}
			gettimeofday(&LectDateRun,0);
		} while(LectDateRun.tv_sec < limite);
	#else
		repart->D3trouvee = 1;
		trouvee = 1;
	#endif /* AVEC_PCI */
	#ifdef NIDAQ
	} else if(repart->famille == FAMILLE_NI) {
		int num,limite,nb;
		repart->top = repart->bottom = FifoTopPrec = 0;
		num = repart->adrs.val - 1;
		if(!LectNIactive[num]) {
			if(!SambaTesteDAQmx("Demarrage du DMA",DAQmxBaseStartTask(LectNITache[num]),log)) LectNIactive[num] = 1;
		}
		gettimeofday(&LectDateRun,0); limite = LectDateRun.tv_sec + 3;
		do {
			repart->acces_demandes++;
			SambaTesteDAQmx("Lecture de donnees NI",DAQmxBaseReadBinaryI16(LectNITache[num],DAQmx_Val_Auto,2.0,DAQmx_Val_GroupByScanNumber,
				(int16 *)(repart->data16 + repart->top),FIFOdim - repart->top,&nb,NULL),log);
			if(nb) {
				repart->D3trouvee = 1;
				repart->acces_remplis++;
				repart->top += nb;
				repart->donnees_recues += nb;
				mot_lu = (lhex)repart->data16[repart->bottom];
				nb_lues += nb;
				trouvee = 1; break;
			} else SambaMicrosec(1000);
			gettimeofday(&LectDateRun,0);
		} while(LectDateRun.tv_sec < limite);
	#endif
	#ifdef AVEC_OPERA
	} else if(repart->famille == FAMILLE_OPERA) {
		hexa lngr; int octets_lus,donnees_lues; OperaTrame trame;
		shex num_trame,precedent;
		int nb;
		precedent = REPART_IP_CNTL; nb = 0;
		gettimeofday(&LectDateRun,0);
		trame = (OperaTrame)repart->fifo32;
		lngr = sizeof(TypeSocket);
		do {
			octets_lus = RepartIpLitTrame(repart,trame,sizeof(TypeOperaTrame));
			if(octets_lus == -1) {
				if(errno == EAGAIN) SambaMicrosec(timesleep);
				else { printf("%s/     ! Repartiteur illisible (%s)\n",DateHeure(),strerror(errno)); return(0); }
			} else if(octets_lus > 0) {
				repart->acces_remplis++;
				if(BigEndianOrder) ByteSwappeIntArray((hexa *)trame,octets_lus / sizeof(Ulong));
				nb++;
				donnees_lues = (octets_lus - OPERA_TAILLE_ENTETE) / sizeof(TypeDonnee); /* 2 octets par voie */
				nb_lues += donnees_lues;
				num_trame = OPERA_TRAME_NUMERO(trame);
				if(num_trame == OPERA_TRAME_STS) /* status */ {
					repart->s.ip.stampDernier = OPERA_TRAME_TIMESTAMP(trame);
					gettimeofday(&LectDateRun,0);
					continue;
				} else if(num_trame == 0) {
					repart->s.ip.lu_entre_D3 = donnees_lues;
					repart->D3trouvee = 1; trouvee = 1; break;
				} else {
					if(precedent < REPART_IP_CNTL) {
						if(num_trame != (precedent + 1)) {
							printf("%s/     ! [%06d] C'est la trame #%d qui suit la #%d\n",DateHeure(),LectDateRun.tv_usec,num_trame,precedent);
							if(num_trame < (precedent + 1)) {
								repart->D3trouvee = 0; trouvee = 1; break;
							}
						}
					} else if(log) printf("%s/     ! [%06d] Premiere trame lue: #%d\n",DateHeure(),LectDateRun.tv_usec,num_trame);
					precedent = num_trame;
				}
			}
			gettimeofday(&LectDateRun,0);
		} while(LectDateRun.tv_sec < limite);
		if(trouvee) {
			RepartIpOctetsLus = octets_lus;
			repart->top = donnees_lues;
			repart->donnees_recues = donnees_lues;
			repart->bottom = 0;
		#ifdef OPERA_STATUS_DANS_ENTETE
			if(ModeleRep[repart->type].revision) repart->s.ip.stamp0 = OPERA_TRAME_TIMESTAMP(trame);
		#else
			if(ModeleRep[repart->type].revision) repart->s.ip.stamp0 = repart->s.ip.stampDernier;
			else repart->s.ip.stamp0 = ((((int64)(trame->data[0]) & 0xFFFF) << 32) + ((int64)(trame->data[1]) & 0xFFFFFFFF));
		#endif
			repart->s.ip.num0 = num_trame;
			repart->s.ip.stampEnCours = repart->s.ip.stampDernier;
			repart->s.ip.numAVenir = num_trame + 1;
			repart->s.ip.synchro_vue = 1;
			LectDerniereD3 = 0;
			if(BigEndianOrder) mot_lu = (lhex)(repart->data16[0]) & 0xFFFF; /* en principe, repart->bottom */
			else mot_lu = (lhex)(repart->data16[1]) & 0xFFFF; /* en principe, repart->bottom+1 */
			if(log) {
				printf("%s/     .          Synchro D3 %s a la %d%s lecture (trame precedente: #%d)\n",
					   DateHeure(),repart->D3trouvee?"trouvee":"MANQUEE",nb,(nb>1)?"eme":"ere",precedent);
				printf("              .          Trame de depart: %lld/%d\n",repart->s.ip.stamp0,repart->s.ip.num0);
			}
			if(ArchAttendsD3 && repart->D3trouvee) { ArchStampsSauves = LectDerniereD3; ArchAttendsD3 = 0; }
		}
	#endif /* AVEC_OPERA */

	} else if(repart->famille == FAMILLE_IPE) {
		hexa lngr; int octets_lus,donnees_lues; TypeIpeTrame *trame;
		shex num_trame,precedent;
		int nb; int essais;
		precedent = REPART_IP_CNTL; nb = 0; essais = 2;
		repart->s.ip.stampDernier = 0;
		gettimeofday(&LectDateRun,0);
		trame = (TypeIpeTrame *)repart->fifo32;
		lngr = sizeof(TypeSocket);
		do {
			octets_lus = RepartIpLitTrame(repart,trame,sizeof(TypeIpeTrame));
			if(octets_lus == -1) {
				if(errno == EAGAIN) SambaMicrosec(timesleep);
				else { printf("%s/     ! Repartiteur illisible (%s)\n",DateHeure(),strerror(errno)); return(0); }
			} else if(octets_lus > 0) {
				repart->acces_remplis++;
				if(BigEndianOrder) ByteSwappeIntArray((hexa *)trame,octets_lus / sizeof(Ulong));
				nb++;
				donnees_lues = (octets_lus - IPE_TAILLE_ENTETE) / sizeof(TypeDonnee); /* 2 octets par voie */
				nb_lues += donnees_lues;
				num_trame = IPE_TRAME_NUMERO(trame);
				// printf("              * Trame lue: #%d\n",num_trame);
				if(num_trame == IPE_TRAME_STS) {
					repart->s.ip.stampDernier = IPE_TRAME_TIMESTAMP(trame);
					gettimeofday(&LectDateRun,0);
					continue;
				} else if(num_trame > REPART_IP_CNTL) continue;
				else if(num_trame == 0) {
					repart->s.ip.lu_entre_D3 = donnees_lues;
					if(!repart->s.ip.stampDernier) repart->s.ip.stampDernier = IPE_TRAME_SYNCHRO_NUM(trame) * 100000;
					if(--essais == 0) { repart->D3trouvee = 1; trouvee = 1; break; }
				} else {
					if(precedent < REPART_IP_CNTL) {
						if(num_trame != (precedent + 1)) {
							printf("%s/     ! [%06d] C'est la trame #%d qui suit la #%d\n",DateHeure(),LectDateRun.tv_usec,num_trame,precedent);
							if(num_trame < (precedent + 1)) { repart->D3trouvee = 0; trouvee = 1; break; }
						}
					} else if(log) printf("%s/     ! [%06d] Premiere trame lue: #%d\n",DateHeure(),LectDateRun.tv_usec,num_trame);
					precedent = num_trame;
				}
			}
			gettimeofday(&LectDateRun,0);
		} while(LectDateRun.tv_sec < limite);
		if(trouvee) {
			RepartIpOctetsLus = octets_lus;
			repart->top = donnees_lues;
			repart->donnees_recues = donnees_lues;
			repart->bottom = 0;
			repart->s.ip.stamp0 = repart->s.ip.stampDernier;
			repart->s.ip.num0 = num_trame;
			repart->s.ip.stampEnCours = repart->s.ip.stampDernier;
			repart->s.ip.numAVenir = num_trame + 1;
			repart->s.ip.synchro_vue = 1;
			LectDerniereD3 = 0;
			if(BigEndianOrder) mot_lu = (lhex)(repart->data16[0]) & 0xFFFF; /* en principe, repart->bottom */
			else mot_lu = (lhex)(repart->data16[1]) & 0xFFFF; /* en principe, repart->bottom+1 */
			if(log) {
				printf("%s/     .          Synchro D3 %s a la %d%s lecture (trame precedente: #%d)\n",
					   DateHeure(),repart->D3trouvee?"trouvee":"MANQUEE",nb,(nb>1)?"eme":"ere",precedent);
				printf("              .          Trame de depart: %lld/%d\n",repart->s.ip.stamp0,repart->s.ip.num0);
			}
			if(ArchAttendsD3 && repart->D3trouvee) { ArchStampsSauves = LectDerniereD3; ArchAttendsD3 = 0; }
		}

	} else if(repart->famille == FAMILLE_CALI) {
		hexa lngr; int octets_lus,donnees_lues; CaliTrame trame;
		int num_trame,precedent,nb;
		precedent = -1; nb = 0;
		gettimeofday(&LectDateRun,0);
		trame = (CaliTrame)repart->fifo32;
		lngr = sizeof(TypeSocket);
		do {
			octets_lus = RepartIpLitTrame(repart,trame,sizeof(TypeCaliTrame));
			if(octets_lus == -1) {
				if(errno == EAGAIN) SambaMicrosec(timesleep);
				else { printf("%s/     ! Repartiteur illisible (%s)\n",DateHeure(),strerror(errno)); return(0); }
			} else if(octets_lus > 0) {
				repart->acces_remplis++;
				//- if(BigEndianOrder) ByteSwappeIntArray((hexa *)trame,octets_lus / sizeof(Ulong));
				if(!BigEndianOrder) CaliSwappeTrame(repart,trame,octets_lus);
				nb++;
				donnees_lues = (octets_lus - CALI_TAILLE_ENTETE) / sizeof(TypeDonnee); /* 2 octets par voie */
				nb_lues += donnees_lues;
				num_trame = CALI_TRAME_NUMERO(trame);
				repart->s.ip.stampDernier = CALI_TRAME_TIMESTAMP(trame);
				gettimeofday(&LectDateRun,0);
			#ifdef NE_MARCHE_PAS_TOUJOURS
				if(Modulo(repart->s.ip.stampDernier,repart->s.ip.tramesD3) == 0) {
					repart->s.ip.lu_entre_D3 = donnees_lues;
					repart->D3trouvee = 1; trouvee = 1; break;
				} else {
					if(precedent >= 0) {
						if(num_trame != (precedent + 1)) {
							printf("%s/     ! [%06d] C'est la trame #%d qui suit la #%d\n",DateHeure(),LectDateRun.tv_usec,num_trame,precedent);
							if(num_trame < (precedent + 1)) {
								repart->D3trouvee = 0; trouvee = 1; break;
							}
						}
					} else if(log) printf("%s/     ! [%06d] Premiere trame lue: #%d\n",DateHeure(),LectDateRun.tv_usec,num_trame);
					precedent = num_trame;
				}
			#else
				repart->s.ip.lu_entre_D3 = Modulo(repart->s.ip.stampDernier,repart->s.ip.tramesD3) + donnees_lues;
				repart->D3trouvee = 1; trouvee = 1; break;
			#endif
			}
			gettimeofday(&LectDateRun,0);
		} while(LectDateRun.tv_sec < limite);
		if(trouvee) {
			RepartIpOctetsLus = octets_lus;
			repart->top = donnees_lues;
			repart->donnees_recues = donnees_lues;
			repart->bottom = 0;
			repart->s.ip.stamp0 = repart->s.ip.stampDernier;
			repart->s.ip.num0 = num_trame;
			repart->s.ip.stampEnCours = repart->s.ip.stampDernier + repart->s.ip.inc_stamp;
			repart->s.ip.numAVenir = num_trame + 1;
			if(!BigEndianOrder) mot_lu = (lhex)(repart->data16[0]) & 0xFFFF; /* en principe, repart->bottom */
			else mot_lu = (lhex)(repart->data16[1]) & 0xFFFF; /* en principe, repart->bottom+1 */
			if(log) printf("%s/     .          Synchro D3 %s a la %d%s lecture (trame precedente: #%d)\n",
						   DateHeure(),repart->D3trouvee?"trouvee":"MANQUEE",nb,(nb>1)?"eme":"ere",precedent);
			if(ArchAttendsD3 && repart->D3trouvee) { ArchStampsSauves = LectDerniereD3; ArchAttendsD3 = 0; }
		}

	}
	gettimeofday(&LectDateRun,0);
	if(log) {
		if(!nb_lues) {
			printf("%s/   ! Ce repartiteur ne delivre pas de donnee\n",DateHeure());
			OpiumWarn("Acquisition impossible avec %s#%02d: carte muette",ModeleRep[repart->type].nom,repart->adrs.champ[3]);
		}
		printf("%s/   . [%06d] Fin d'attente de synchro D3\n",DateHeure(),LectDateRun.tv_usec);
	}
	*val1 = mot_lu;
	return(trouvee);
#endif /* CODE_WARRIOR_VSN */
}
/* ========================================================================== */
char RepartiteurRepereD3commune(char log) {
	/* Doit rendre repart->bottom et repart->top cales sur une synchro D3 */
	TypeRepartiteur *repart; void *trame; //- size_t taille_trame; int taille_entete;
	int rep;
	hexa lngr; ssize_t octets_lus;
	// shex num_trame,num_status,precedent;
	int num,num_trame,num_status,precedent,nb_trames,donnees_lues;
	int timeout;   /* delai maxi en secondes avant abandon */
	int timesleep; /* attente en microsecs si pile vide (pas de donnees) */
	int limite;
	char trouvee,pas_fini;
	lhex mot_lu;
#ifdef AVEC_PCI
	TypeDonnee val;
	int i;
#endif

	LectEnCours.valeur = 0;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) {
		repart = &(Repart[rep]);
		repart->donnees_recues = 0;
		repart->D3trouvee = 0;
		repart->top = repart->bottom = FifoTopPrec = 0;
		if(repart->famille == FAMILLE_CLUZEL) repart->s.pci.en_cours = 0;
		else if(repart->famille == FAMILLE_NI) {
			num = repart->adrs.val - 1;
		#ifdef NIDAQ
			if(!LectNIactive[num]) {
				if(!SambaTesteDAQmx("Demarrage du DMA",DAQmxBaseStartTask(LectNITache[num]),log)) LectNIactive[num] = 1;
			}
		#endif
		} else if(repart->interf == INTERF_IP) {
			precedent = REPART_IP_CNTL;
			nb_trames = 0; lngr = sizeof(TypeSocket);
		}
	}
	mot_lu = 0;
	timeout = 5; /* secs */
	timesleep = 1000; /* microsecs */
	gettimeofday(&LectDateRun,0); limite = LectDateRun.tv_sec + timeout;
	if(log) printf("%s/ . [%06d] Attente d'une synchro D3, au plus %d secondes\n",DateHeure(),LectDateRun.tv_usec,timeout);
	do {
		pas_fini = 0;
		for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif && !Repart[rep].D3trouvee) {
			repart = &(Repart[rep]);
			trouvee = 0;
			if(repart->famille == FAMILLE_CLUZEL) {
			#ifdef AVEC_PCI
				if(!ModeleRep[repart->type].status) return(0);
				if(!(repart->s.pci.en_cours)) {
					repart->top = repart->bottom = 0;
					IcaXferExec(repart->p.pci.port); repart->s.pci.en_cours = 1;
				}
				repart->top = IcaXferDone(repart->p.pci.port);
				if(repart->top < 0) {
					printf("%s/   !          Transfert avorte sur erreur #%d\n",DateHeure(),IcaDerniereErreur());
					repart->top = 0;
					return(0);
				} else if(repart->top > 0) {
					repart->s.pci.en_cours = 0;
					nb_trames++;
					while(repart->bottom < repart->top) /* se caler sur les modulations puis fin de bloc */ {
						mot_lu = repart->fifo32[repart->bottom];
						if(CLUZEL_Empty(mot_lu)) break;
						repart->donnees_recues += 1;
						if(DATATYPE(mot_lu) == CLUZEL_MARQUE_MODULATION) {
							i = 1;
							while(i < repart->nbdonnees) {
								val = repart->fifo32[repart->bottom+i];
								if(CLUZEL_Empty(val)) break;
								repart->donnees_recues += 1;
								i++;
							}
							if(i < repart->nbdonnees) /* on est monstre embetes, dans ce cas... */ {
								repart->bottom += (i - 1); break;
							} else {
								mot_lu = repart->fifo32[repart->bottom+i];
								if(CLUZEL_Empty(mot_lu)) /* c'est pas mieux */ {
									repart->bottom += (i - 1); break;
								}
								if(val & repart->p.pci.bitD2D3) { donnees_lues = 0; repart->D3trouvee = 1; trouvee = 1; break; }
								repart->bottom += i;
							}
						}
						repart->bottom++;
					}
				}
			#else
				repart->D3trouvee = 1; trouvee = 1;
			#endif /* AVEC_PCI */
			#ifdef NIDAQ
			} else if(repart->famille == FAMILLE_NI) {
				int nb;
				repart->acces_demandes++;
				num = repart->adrs.val - 1;
				SambaTesteDAQmx("Lecture de donnees NI",DAQmxBaseReadBinaryI16(LectNITache[num],DAQmx_Val_Auto,2.0,DAQmx_Val_GroupByScanNumber,(int16 *)(repart->data16 + repart->top),FIFOdim - repart->top,&nb,NULL),log);
				if(nb) {
					repart->acces_remplis++;
					repart->top += nb;
					repart->donnees_recues += nb;
					donnees_lues = nb;
					mot_lu = (lhex)(repart->data16[repart->bottom]);
					repart->D3trouvee = 1; trouvee = 1;
				}
			#endif
			} else if(repart->interf == INTERF_IP) {
				trame = (void *)(repart->fifo32);
				octets_lus = RepartIpLitTrame(repart,trame,repart->dim.totale);
				if(octets_lus == -1) {
					if(errno != EAGAIN) {
						repart->actif = 0;
						printf("%s/     ! Repartiteur %s illisible (%s)\n",DateHeure(),repart->nom,strerror(errno));
						printf("%s/   !!! Synchro D3 absente, repartiteur desactive...\n",DateHeure());
						return(0);
					}
				} else if(octets_lus > 0) {
					if(BigEndianOrder) {
						if(repart->famille == FAMILLE_OPERA) OperaSwappeTrame(trame,octets_lus);
						else if(repart->famille == FAMILLE_IPE) IpeSwappeTrame(trame,octets_lus);
					} else {
						if(repart->famille == FAMILLE_CALI) CaliSwappeTrame(repart,(CaliTrame)trame,octets_lus);
					}
					nb_trames++;
					donnees_lues = (octets_lus - repart->dim.entete) / sizeof(TypeDonnee); /* 2 octets par voie */
					if(donnees_lues > 0) repart->donnees_recues += donnees_lues;
					repart->acces_remplis++;
					if(repart->famille == FAMILLE_CALI) {
						num_trame = CALI_TRAME_NUMERO(trame);
						repart->s.ip.stampDernier = CALI_TRAME_TIMESTAMP(trame);
						repart->s.ip.lu_entre_D3 = Modulo(repart->s.ip.stampDernier,repart->s.ip.tramesD3) + donnees_lues;
						repart->D3trouvee = 1; trouvee = 1;
					} else {
						if(repart->famille == FAMILLE_OPERA) {
							num_trame = OPERA_TRAME_NUMERO(trame);
							num_status = OPERA_TRAME_STS;
						} else if(repart->famille == FAMILLE_IPE) {
							num_trame = IPE_TRAME_NUMERO(trame);
							num_status = IPE_TRAME_STS;
						}
						if(num_trame == num_status) /* status */ {
							if(repart->famille == FAMILLE_OPERA) repart->s.ip.stampDernier = OPERA_TRAME_TIMESTAMP(trame);
							else if(repart->famille == FAMILLE_IPE) repart->s.ip.stampDernier = IPE_TRAME_TIMESTAMP(trame);
						} else if(num_trame == 0) {
							RepartIpOctetsLus = octets_lus;
							repart->s.ip.lu_entre_D3 = donnees_lues;
							repart->D3trouvee = 1; trouvee = 1;
						} else {
							if(precedent < REPART_IP_CNTL) {
								if(num_trame != (precedent + 1)) {
									printf("%s/   ! [%06d] %s: c'est la trame #%d qui suit la #%d\n",DateHeure(),LectDateRun.tv_usec,repart->nom,num_trame,precedent);
									if(num_trame < (precedent + 1)) trouvee = 1;
								}
							} else if(log) printf("%s/   . [%06d] %s, premiere trame lue: #%d\n",DateHeure(),LectDateRun.tv_usec,repart->nom,num_trame);
							precedent = num_trame;
						}
					}
				}
				if(trouvee) {
					repart->top = donnees_lues;
					repart->donnees_recues = donnees_lues;
					repart->bottom = 0;
					if((repart->famille != FAMILLE_OPERA) || ModeleRep[repart->type].revision)
						repart->s.ip.stamp0 = repart->s.ip.stampDernier;
					else /* ((repart->famille == FAMILLE_OPERA) && ModeleRep[repart->type].revision) */ {
						repart->s.ip.stamp0 = (((int64)(((OperaTrame)trame)->data[0]) & 0xFFFF) << 32) + ((int64)(((OperaTrame)trame)->data[1]) & 0xFFFFFFFF);
					}
					repart->s.ip.num0 = num_trame;
					repart->s.ip.stampEnCours = repart->s.ip.stampDernier;
					if(repart->famille == FAMILLE_CALI) repart->s.ip.stampEnCours += repart->s.ip.inc_stamp;
					repart->s.ip.numAVenir = num_trame + 1;
					repart->s.ip.synchro_vue = 1;
					LectDerniereD3 = 0;
					if(repart->famille == FAMILLE_CALI) {
						if(!BigEndianOrder) mot_lu = (lhex)(repart->data16[0] )& 0xFFFF; /* en principe, repart->bottom */
						else mot_lu = (lhex)(repart->data16[1]) & 0xFFFF; /* en principe, repart->bottom+1 */
					} else {
						if(BigEndianOrder) mot_lu = (lhex)(repart->data16[0]) & 0xFFFF; /* en principe, repart->bottom */
						else mot_lu = (lhex)(repart->data16[1]) & 0xFFFF; /* en principe, repart->bottom+1 */
					}
				}
			}
			if(trouvee) {
				gettimeofday(&LectDateRun,0);
				if(synchro1.tv_sec == 0) memcpy(&synchro1,&LectDateRun,sizeof(struct timeval));
				memcpy(&synchroN,&LectDateRun,sizeof(struct timeval));
				if(log) {
					printf("%s/   . [%06d] %s: synchro D3 %s a la %d%s lecture",DateHeure(),
						LectDateRun.tv_usec,repart->nom,repart->D3trouvee?"trouvee":"MANQUEE",nb_trames,(nb_trames>1)?"eme":"ere");
					if(repart->interf == INTERF_IP) printf(" (trame precedente: #%d)",precedent);
					printf(", donnee #%d\n",repart->bottom);
					printf("            .          %s: ",repart->nom);
					if(repart->interf == INTERF_IP) printf("trame de depart = %lld/%d, ",repart->s.ip.stamp0,repart->s.ip.num0);
					printf("valeur lue = %08lX\n",mot_lu);
				}
				if(ArchAttendsD3 && repart->D3trouvee) { ArchStampsSauves = LectDerniereD3; ArchAttendsD3 = 0; }
			} else pas_fini = 1;
		}
		if(pas_fini) SambaMicrosec(timesleep);
		gettimeofday(&LectDateRun,0);
	} while(pas_fini && (LectDateRun.tv_sec < limite));
	if(log) printf("%s/ . [%06d] Fin d'attente de synchro D3\n",DateHeure(),LectDateRun.tv_usec);
	LectEnCours.valeur = mot_lu;
	LectEnCours.rep = rep;
	return(!pas_fini);
}
/* ========================================================================== */
char RepartiteursAttendStamp(char log) {
	TypeRepartiteur *repart;
	int rep; char termine[REPART_MAX];
	int nb,num0; hexa lngr; int limite;
	char d3_trouvee;
	void *trame;

	d3_trouvee = 0; // GCC
	LectRepartAttendus = 0;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) { termine[rep] = 0; LectRepartAttendus++; }
	if(log) printf("%s/ . Synchro D3 minimale attendue: %lld pour %d repartiteur%s\n",DateHeure(),LectStampMini,Accord1s(LectRepartAttendus));

	synchro1.tv_sec = 0;
	do {
		for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif && !termine[rep]) {
			repart = &(Repart[rep]);
			repart->acces_demandes++;
			trame = (void *)repart->fifo32;
			gettimeofday(&LectDateRun,0); limite = LectDateRun.tv_sec + 1;
			do {
				if(repart->interf == INTERF_IP) {
					lngr = sizeof(TypeSocket);
					nb = RepartIpLitTrame(repart,trame,RepartTrameDef[repart->famille].lngr);
					// printf("> recu via <%d>: %d octet%s\n",repart->lec.ip.path,Accord1s(nb));
					if(nb == -1) { if(errno != EAGAIN) printf("%9s   ! %s %s hors service (%s)\n"," ",ModeleRep[repart->type].nom,repart->nom,strerror(errno)); LectRepartAttendus--; break; }
					else if(nb > 0) {
						if(BigEndianOrder) {
							ByteSwappeInt((hexa *)trame);
							if(repart->famille == FAMILLE_IPE) TwoShortsSwappeInt((hexa *)trame);
						} else if(repart->famille == FAMILLE_CALI) ByteSwappeLong((int64 *)trame);
						num0 = 0;
						switch(repart->famille) {
							case FAMILLE_OPERA:
								d3_trouvee = (OPERA_TRAME_NUMERO(trame) == OPERA_TRAME_STS);
								if(d3_trouvee) repart->s.ip.stampDernier = OPERA_TRAME_TIMESTAMP(trame);
								// if(d3_trouvee) printf("  + Trame %04X, TS=%lld\n",OPERA_TRAME_NUMERO(trame),repart->s.ip.stampDernier);
								// else printf("  + Trame %04X (pas D3)\n",OPERA_TRAME_NUMERO(trame));
								// OperaDump((byte *)trame,nb,1);
								break;
							case FAMILLE_IPE:
								d3_trouvee = (IPE_TRAME_NUMERO(trame) == IPE_TRAME_STS);
								if(d3_trouvee) repart->s.ip.stampDernier = IPE_TRAME_TIMESTAMP(trame);
								break;
							case FAMILLE_CALI:
								repart->s.ip.stampDernier = CALI_TRAME_TIMESTAMP(trame); num0 = CALI_TRAME_NUMERO(trame);
								d3_trouvee = (Modulo(repart->s.ip.stampDernier,repart->s.ip.tramesD3) == 0);
								break;
						}
						if(d3_trouvee) {
							if(repart->s.ip.stampDernier >= LectStampMini) {
								if(log) printf("%s/   . %s en service a compter du timestamp %lld\n",DateHeure(),repart->nom,repart->s.ip.stampDernier);
								if(synchro1.tv_sec == 0) memcpy(&synchro1,&LectDateRun,sizeof(struct timeval));
								memcpy(&synchroN,&LectDateRun,sizeof(struct timeval));
								// if(repart->s.ip.stamp0 == 0) {
									repart->s.ip.stamp0 = repart->s.ip.stampDernier;
									repart->s.ip.num0 = num0;
									if((LectTimeStamp0 == 0) && (!LectStampSpecifique || (repart->categ == DONNEES_STAMP))) {
										LectTimeStamp0 = repart->s.ip.stamp0;
										if(repart->s.ip.stampD3 > repart->s.ip.duree_trame) LectTimeStamp0 += (int64)(repart->s.ip.num0 * repart->s.ip.duree_trame);
										LectTimeStampN = LectTimeStamp0;
										if(log) printf("%9s   . TimeStamp initial du run: %lld (par repartiteur %s)\n"," ",LectTimeStamp0,RepartDonneesListe[repart->categ]);
										if(repart->categ == DONNEES_STAMP) {
											if(log) printf("%9s   . Repartiteur desactive a partir de maintenant\n"," ");
											repart->actif = 0;
										}
									}
								// }
								repart->top = repart->donnees_recues = repart->s.ip.lu_entre_D3 = 0; // (nb - CALI_TAILLE_ENTETE) / sizeof(TypeDonnee);
								repart->bottom = 0;
								repart->D3trouvee = 1;
								RepartIpOctetsLus += nb;
								if(repart->categ != DONNEES_STAMP) {
									if(BigEndianOrder) ByteSwappeIntArray((hexa *)trame+1,(nb - sizeof(int)) / sizeof(Ulong));
									else if(repart->famille == FAMILLE_CALI)
										ByteSwappeIntArray((hexa *)((char *)trame + sizeof(int64)),(nb - sizeof(int64)) / sizeof(Ulong));
									switch(repart->famille) {
										case FAMILLE_OPERA: OperaRecopieStatus((Structure_trame_status *)trame,repart); break;
										case FAMILLE_IPE:   IpeRecopieStatus((IpeTrame)trame,repart,1); break;
										case FAMILLE_CALI:  CaliRecopieStatus((CaliTrame)trame,repart); break;
									}
								}
								termine[rep] = 1;
								LectRepartAttendus--;
								break;
							}
						}
					}
				} else {
					/* attendre un synchroD3 quand meme */
					termine[rep] = 1;
					LectRepartAttendus--;
					break;
				}
				// SambaMicrosec(200000);
				gettimeofday(&LectDateRun,0);
			} while(LectDateRun.tv_sec < limite);
		};
		OpiumUserAction();
	} while(LectRepartAttendus);

	LectRepartAttendus = 0;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif && !termine[rep]) { LectRepartAttendus++; Repart[rep].actif = 0; }
	return((LectRepartAttendus != 0));
}
/* ========================================================================== */
INLINE char RepartVidePile(TypeRepartiteur *repart, char *pas_tous_secs, short *pleins, char log) {
	switch(repart->famille) {
		case FAMILLE_CLUZEL:  return(CluzelVidePile(repart,pas_tous_secs,pleins,log)); break;
		case FAMILLE_OPERA:   return(OperaVidePile(repart,pas_tous_secs,pleins,log));  break;
		case FAMILLE_CALI:    return(CaliVidePile(repart,pas_tous_secs,pleins,log));   break;
		case FAMILLE_IPE:     return(IpeVidePile(repart,pas_tous_secs,pleins,log));    break;
		case FAMILLE_ARCHIVE: return(ArchLitBloc(repart,pas_tous_secs,pleins,log));    break;
		case FAMILLE_SAMBA:   break;
		case FAMILLE_NI:      return(NiVidePile(repart,pas_tous_secs,pleins,log));     break;
	}
	return(0);
}
#pragma mark ----------- Interface -----------
/* ========================================================================== */
static int RepartiteurDeconnecteBN() {
	RepartiteurAccepteEntrees(RepartConnecte,0);
	return(0);
}
/* ==========================================================================
static void RepartiteursRenomme() {
	int rep; TypeRepartiteur *repart; TypeRepModele *modele_rep;

	for(rep=0; rep<RepartNb; rep++) {
		repart = &(Repart[rep]); modele_rep = &(ModeleRep[repart->type]);
		switch(repart->famille) {
			case FAMILLE_OPERA: case FAMILLE_CALI: case FAMILLE_IPE:
				sprintf(repart->nom,"%s%02d",modele_rep->code,repart->adrs.champ[3]);
				break;
			case FAMILLE_CLUZEL:
				if(PCInb < 2) sprintf(nom,"%s",modele_rep->code);
				else sprintf(nom,"%s@PCI%d",modele_rep->code,repart->adrs.val);
				break;
			case FAMILLE_NI:
				sprintf(repart->nom,"NI:Dev%d",repart->adrs.val);
				break;
		}
	}
}
   ========================================================================== */
static char RepartiteurParmEnCours() {
	RepartiteurParmModifie = 1; RepartiteursASauver = 1;
	return(1);
}
/* ========================================================================== */
static int RepartiteurModeDemande() {
	int rep;

	if(RepartConnecte->rep >= 0) {
		RepartiteurChangeMode(RepartConnecte,RepartDataMode,1);
	} else for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) /* anciennement select */ {
		RepartiteurChangeMode(&(Repart[rep]),RepartDataMode,1);
	}
	return(0);
}
/* ========================================================================== */
static int RepartiteurChangeNumerMode() {
	int rep;

	if(RepartConnecte->rep >= 0) {
		printf("          * %s: passage des numeriseurs lus en mode %s\n",
			   RepartConnecte->nom,NumerModeTexte[NumerModeAcquis]);
		RepartiteurDemarreNumeriseurs(RepartConnecte,NumerModeAcquis,NUMER_DIFFUSION,0);
	} else for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) /* anciennement select */ {
		printf("          * %s: passage des numeriseurs lus en mode %s\n",
			   Repart[rep].nom,NumerModeTexte[NumerModeAcquis]);
		RepartiteurDemarreNumeriseurs(&(Repart[rep]),NumerModeAcquis,NUMER_DIFFUSION,0);
	}
	return(0);
}
/* ========================================================================== */
static int RepartiteurAcquiert() {
	TypeRepartiteur *repart; int rep;

	repart = RepartConnecte;
	if(repart->status_demande) {
		if(pRepartDonnees && OpiumDisplayed(pRepartDonnees->cdr)) OpiumClear(pRepartDonnees->cdr);
		if(!RepartGraphEnCours) {
			RepartGraphEnCours = GraphCreate(RepartTraceVisu,RepartTraceHaut,REPART_VOIES_MAX+1);
		} else if(OpiumDisplayed(RepartGraphEnCours->cdr)) OpiumClear(RepartGraphEnCours->cdr);
		if(RepartGraphEnCours) {
			char texte[80];
			sprintf(texte,"Donnees %s",repart->nom); OpiumTitle(RepartGraphEnCours->cdr,texte);
		}
		RepartStatusHorloge[0] = 0.0; RepartStatusHorloge[1] = 1.0 / Echantillonnage;

		if((SettingsMultitasks != 4) && (repart->famille == FAMILLE_CALI)) /* ?? */ {
			int l,n;
			if(!RepartIpEcriturePrete(repart,"")) return(0);
			l = sprintf(RepartiteurValeurEnvoyee,"w %x F",CALI_REG_SEL);
			n = RepartIpEcrit(repart,RepartiteurValeurEnvoyee,l+1); SambaMicrosec(ModeleRep[repart->type].delai_msg);
			printf("* Ecrit: '%s'\n",RepartiteurValeurEnvoyee);
		}

		/* lancement de l'acquisition */
		//? if(repart->famille == FAMILLE_OPERA) repart->status_relu = 0; else repart->status_relu = 1;
		RepartDonneesConstruit = 0;
		LectLitStatus = 1;
		repart->status_relu = 0;
		if(!Acquis[AcquisLocale].etat.active) {
			for(rep=0; rep<RepartNb; rep++) Repart[rep].utile = 0;
			repart->local = repart->utile = 1; RepartiteursSelectionneVoiesLues(1,1); printf("\n"); RepartImprimeEchantillon(0);
			RepartImprimeCompteurs(repart,"* ",0);
			LecturePourRepart = 1;
			LectAcqElementaire(NumerModeAcquis);
		}
	} else {
		Acquis[AcquisLocale].etat.active = 0;
	}
	LecturePourRepart = 0;
	return(0);
}
/* ========================================================================== */
int RepartiteurChercheIP() {
	int i,k,l,m; int ip; char adresseIP[24]; char trouve;
	TypeRepartiteur repart_tempo;

	if(OpiumExec(pRepartChercheIP->cdr) == PNL_CANCEL) return(0);

	printf("%s/ Recherche de repartiteurs Opera entre %s.%d et %s.%d\n",DateHeure(),
		RepartChercheIPnet,RepartChercheIPdep,RepartChercheIPnet,RepartChercheIParr);
	repart_tempo.type = OperaModele - 1;
	repart_tempo.rep = RepartNb;
	repart_tempo.simule = 0;
	repart_tempo.ecr.port = _port_serveur_UDP;
	repart_tempo.lec.port = PortLectRep + repart_tempo.rep;
	repart_tempo.present = 0;
	repart_tempo.famille = FAMILLE_OPERA;
	repart_tempo.maxentrees = ModeleRep[repart_tempo.type].max_numer;
	repart_tempo.s.ip.relance = 240;
	repart_tempo.entree = (TypeRepartEntree *)malloc(repart_tempo.maxentrees * sizeof(TypeRepartEntree));
	for(l=0; l<repart_tempo.maxentrees; l++) repart_tempo.entree[l].adrs = 0;
	for(i=RepartChercheIPdep; i<=RepartChercheIParr; i++) {
		sprintf(adresseIP,"%s.%d",RepartChercheIPnet,i);
		printf("          | %s:\n",adresseIP);
		ip = HostInt(adresseIP);
		if(SocketInitNum(ip,_port_serveur_UDP,&(repart_tempo.ecr.ip.skt)) <= 0) {
			printf("          ! L'adresse '%s:%d' est incorrecte",adresseIP,_port_serveur_UDP);
			return(0);
		};
		repart_tempo.ecr.ip.path = -1;
		repart_tempo.lec.ip.path = -1;
		repart_tempo.status_demande = 1;
		repart_tempo.adrs.val = HostAdrsToInt(adresseIP);
		if(RepartIpEcriturePrete(&repart_tempo,"          | ")) {
			TypeOperaTrame tampon; void *trame; hexa lngr; int limite,nb;
			int timesleep,nbsleeps;
			trouve = 0;
			RepartIpTransmission(&repart_tempo,REPART_ENVOI_STATUS,repart_tempo.s.ip.relance,-1);
			timesleep = 10000; /* microsecs */
			nbsleeps = 120; /* fois timesleep */
			gettimeofday(&LectDateRun,0); limite = LectDateRun.tv_sec + (timesleep * nbsleeps / 1000000);
			trame = (void *)&tampon;
			lngr = sizeof(TypeSocket);
			k = m = 0;
			do {
				nb = RepartIpLitTrame(&repart_tempo,trame,sizeof(TypeOperaTrame));
				k++;
				// printf("            recvfrom(%d) rend %d: %s\n",repart_tempo.lec.ip.path,nb,(nb == -1)?strerror(errno):"OK");
				if(nb == -1) { if(errno == EAGAIN) SambaMicrosec(timesleep); else break; }
				else if(((TypeSocketIP *)(&(repart_tempo.lec.ip.skt)))->sin_addr.s_addr != ip) continue;
				else if(nb > 0) {
					m++;
					// if(BigEndianOrder) ByteSwappeInt((hexa *)trame);
					if(BigEndianOrder) ByteSwappeIntArray((hexa *)trame,nb / sizeof(Ulong));
					if(OPERA_TRAME_NUMERO(trame) == OPERA_TRAME_STS) {
						// if(BigEndianOrder) ByteSwappeIntArray((hexa *)trame+1,nb / sizeof(Ulong));
						OperaRecopieStatus((Structure_trame_status *)trame,&repart_tempo);
						OperaConfigure(&repart_tempo);
						printf("          | [t=%d]: cew v%d.%d en mode %s avec %d numeriseur%s",OperaNbD3,
							RepartVersionLogiciel/1000,RepartVersionLogiciel%1000,OperaCodesAcquis[repart_tempo.r.opera.mode],
							Accord1s(repart_tempo.nbentrees));
						if(RepartInfoFpgaBB[0]) printf(" (%s)\n",RepartInfoFpgaBB); else printf("\n");
						trouve = 1;
						break;
					}
				}
				gettimeofday(&LectDateRun,0);
			} while(LectDateRun.tv_sec < limite);
			RepartIpTransmission(&repart_tempo,REPART_ENVOI_STATUS,0,0);
			// if(!trouve) printf("inutilisable (%d trame%s recue%s dont %d correcte%s)\n",Accord2s(k),Accord1s(m));
			if(!trouve) printf("          | ! inutilisable\n");
			RepartIpEcritureFerme(&repart_tempo);
		} else printf("          | ! hors circuit\n");
		repart_tempo.status_demande = 0;
	}
	printf("          |__________________________ termine\n");
	return(0);
}
/* ========================================================================== */
int RepartiteursStatus() {
	int rep,nb,i;
	int timesleep,nbsleeps;
	int32 valeur_lue;
	TypeRepartiteur *repart;

	if(RepartNb <= 0) {
		OpiumError("Pas de repartiteur declare");
		return(0);
	}
	if(Acquis[AcquisLocale].etat.active) {
		OpiumError("Incompatibilite avec l'acquisition en cours");
		return(0);
	}
	printf("%s/ Affichage de l'etat des repartiteurs: demarre\n",DateHeure());
	strcpy(RepartHeure,"l'aveugle"); // strcpy(RepartHeure,"point d'heure");
	RepartLocaux = 0; for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) RepartLocal[RepartLocaux++] = rep;
	// if(RepartLocaux != RepartMem) {
		//? RepartiteursRenomme();
		PanelErase(pRepartEtat);
		for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) {
			i = PanelListB(pRepartEtat,Repart[rep].nom,RepartEtatListe,&(RepartEtat[rep]),13);
			// PanelItemSelect(pRepartEtat,i,0);
			PanelItemColors(pRepartEtat,i,SambaBleuRougeOrangeJauneVert,REPART_NBETATS);
		}
		MenuItemDelete(mRepartEtat);
		MenuItemArray(mRepartEtat,REPART_NBETAPES*RepartNb);
		for(i=0; i<REPART_NBETAPES; i++) {
			for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) MenuItemAdd(mRepartEtat,RepartItem[i],MNU_FONCTION RepartEtatChange);
		}
		BoardDismount(bRepartEtat);
		BoardAddPanel(bRepartEtat,pRepartEtat,Dx,WndLigToPix(2),0);
		BoardAddMenu (bRepartEtat,mRepartEtat,OPIUM_A_DROITE_DE pRepartEtat->cdr);
		// Helas, ne marche pas (se cale sur le dernier): BoardAddPanel(bRepartEtat,pRepartHeure,OPIUM_EN_DESSOUS_DE pRepartEtat->cdr);
		BoardAddPanel(bRepartEtat,pRepartHeure,Dx,WndLigToPix(3+RepartNb),0);
	//	RepartMem = RepartLocaux;
	// }
	OpiumUse(bRepartEtat,OPIUM_FORK);
	PanelRefreshVars(pRepartHeure);
	OpiumUserAction();
	until(OpiumDisplayed(bRepartEtat)) { SambaMicrosec(10000); OpiumUserAction(); }
	do {
		for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local && Repart[rep].valide) {
			RepartEtat[rep] = REPART_ABSENT;
			repart = &(Repart[rep]);
			// printf("Existence de %s ..\n",repart->nom);
			if(repart->famille == FAMILLE_CLUZEL) {
				int num;
				num = repart->adrs.val - 1;
				if((num >= 0) && (num < PCInb)) {
					char securite;
					RepartEtat[rep] = REPART_ACCES; /* implique REPART_PRESENT */
				#ifdef AVEC_PCI
					IcaXferStatusReset(repart->p.pci.port);
				#endif
					securite = (ModeleRep[repart->type].version < 2.0);
					if(!CluzelRAZFifo(repart->p.pci.port,repart->octets,securite,0)) continue;
					CluzelEnvoieCommande(repart->p.pci.port,CLUZEL_ADRS,CLUZEL_SSAD_ACQUIS,CLUZEL_START,CLUZEL_MODULEE);
				#ifdef AVEC_PCI
					IcaXferExec(repart->p.pci.port);
				#endif
					repart->s.pci.en_cours = 1;
				}
			} else if(repart->interf == INTERF_IP) {
				/* Pas tres rapide, a cause du time-out de ping
				char buffer[80];
				sprintf(buffer,COMMANDE_PING,repart->parm);
				if(system(buffer) == 0) {
					RepartEtat[rep] = REPART_PRESENT;
				    RepartIpEcriturePrete(repart,"            ");
					if(repart->ecr.ip.path >= 0) {
						RepartEtat[rep] = REPART_ACCES;
						RepartIpTransmission(repart,REPART_ENVOI_STATUS,repart->s.ip.relance,0);
					}
				} */
				if(RepartIpEcriturePrete(repart,"            ")) {
					RepartEtat[rep] = REPART_ACCES; /* implique REPART_PRESENT */
					RepartIpTransmission(repart,REPART_ENVOI_STATUS,repart->s.ip.relance,0);
					if(repart->famille == FAMILLE_CALI) SambaMicrosec(ModeleRep[repart->type].delai_msg);
				}
			} else if(repart->famille == FAMILLE_NI) {
			}
			// printf(".. %s\n",RepartEtatListe[RepartEtat[rep]]);
			OpiumUserAction();
		}
		for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local && Repart[rep].valide && (RepartEtat[rep] == REPART_ACCES)) {
			repart = &(Repart[rep]);
			// printf("Disponibilite de %s ..\n",repart->nom);
			timesleep = 10000; /* microsecs */
			nbsleeps = 20; /* fois timesleep */
			if(repart->famille == FAMILLE_CLUZEL) {
			#ifdef AVEC_PCI
				do SambaMicrosec(timesleep); while(((repart->top = IcaXferDone(repart->p.pci.port)) <= 0) && (--nbsleeps > 0));
			#endif
				repart->s.pci.en_cours = 0;
				repart->bottom = 0;
				while(repart->bottom < repart->top) {
					valeur_lue = repart->fifo32[repart->bottom];
					// if(CLUZEL_Empty(valeur_lue)) break;
					if((valeur_lue & 0xFFFF) == 0x0000) break;
					repart->bottom++;
				};
				if(repart->bottom > 0) RepartEtat[rep] = REPART_DISPO;
			} else if(repart->famille == FAMILLE_OPERA) {
				OperaTrame trame; hexa lngr; int limite;
				gettimeofday(&LectDateRun,0); limite = LectDateRun.tv_sec + (timesleep * nbsleeps / 1000000);
				trame = (OperaTrame)repart->fifo32;
				lngr = sizeof(TypeSocket);
				do {
					nb = RepartIpLitTrame(repart,trame,sizeof(TypeOperaTrame));
					if(nb == -1) { if(errno == EAGAIN) SambaMicrosec(timesleep); else break; }
					else if(nb > 0) {
						RepartEtat[rep] = REPART_DISPO;
						if(repart->status_demande) {
							if(BigEndianOrder) ByteSwappeInt((hexa *)trame);
							/* if(OPERA_TRAME_NUMERO(trame) == OPERA_TRAME_STS) {
								if(BigEndianOrder) ByteSwappeIntArray((hexa *)trame+1,(nb - 1) / sizeof(Ulong));
								OperaRecopieStatus((Structure_trame_status *)trame,repart);
								OperaConfigure(repart);
							} */
						}
						break;
					}
					gettimeofday(&LectDateRun,0);
				} while(LectDateRun.tv_sec < limite);
			} else if(repart->famille == FAMILLE_IPE) {
				IpeTrame trame; hexa lngr; int limite;
				gettimeofday(&LectDateRun,0); limite = LectDateRun.tv_sec + (timesleep * nbsleeps / 1000000);
				trame = (IpeTrame)repart->fifo32;
				lngr = sizeof(TypeSocket);
				do {
					nb = RepartIpLitTrame(repart,trame,sizeof(TypeIpeTrame));
					if(nb == -1) { if(errno == EAGAIN) SambaMicrosec(timesleep); else break; }
					else if(nb > 0) { RepartEtat[rep] = REPART_DISPO; break; }
					gettimeofday(&LectDateRun,0);
				} while(LectDateRun.tv_sec < limite);
			} else if(repart->famille == FAMILLE_CALI) {
				CaliTrame trame; hexa lngr; int limite;
				gettimeofday(&LectDateRun,0); limite = LectDateRun.tv_sec + (timesleep * nbsleeps / 1000000);
				trame = (CaliTrame)repart->fifo32;
				lngr = sizeof(TypeSocket);
				do {
					nb = RepartIpLitTrame(repart,trame,sizeof(TypeCaliTrame));
					// printf("%s/ recu %d octet%s\n",DateHeure(),Accord1s(nb));
					if(nb == -1) { if(errno == EAGAIN) SambaMicrosec(timesleep); else break; }
					else if(nb > 0) {
						RepartEtat[rep] = REPART_DISPO;
						if(BigEndianOrder) ByteSwappeIntArray((hexa *)trame,nb / sizeof(Ulong));
						CaliRecopieStatus(trame,repart);
						break;
					}
					gettimeofday(&LectDateRun,0);
				} while(LectDateRun.tv_sec < limite);
			} else if(repart->famille == FAMILLE_NI) {
			}
			// printf(".. %s\n",RepartEtatListe[RepartEtat[rep]]);
			OpiumUserAction();
		};
		/* if(RepartNb > 1) {
			printf("%s/ Etat des %d repartiteurs:\n",DateHeure(),RepartNb);
			for(rep=0; rep<RepartNb; rep++) printf("          | %s: %s\n",Repart[rep].nom,RepartEtatListe[RepartEtat[rep]]);
		} else printf("%s/ Etat du repartiteur: %s\n",DateHeure(),RepartEtatListe[RepartEtat[0]]); */
		PanelRefreshVars(pRepartEtat);
		DateHeure(); PanelRefreshVars(pRepartHeure);
		OpiumUserAction();
	} while(OpiumDisplayed(bRepartEtat));
	printf("%s/ Affichage de l'etat des repartiteurs termine\n",DateHeure());

	return(0);
}
/* ========================================================================== */
int RepartiteurIpBasique(TypeRepartiteur *repart) {
	char cmde;
	byte buffer[12];
	char entree[32];
	int i,k,lngr; int val;

	if(!RepartIpEcriturePrete(repart,"")) {
		OpiumFail(L_("Connexion pour ecriture sur %s (%d.%d.%d.%d:%d) impossible"),repart->nom,IP_CHAMPS(repart->adrs),repart->ecr.port);
		return(0);
	}
	k = lngr = 0;
	cmde = OperaCde;
	if(OpiumReadKeyB("Code commande","h/w/p/q/x",&cmde,4) == PNL_CANCEL) return(0);
	OperaCde = cmde;
	if(cmde != OperaCde) switch(OperaCde) {
		case 0 /* H */: strcpy(OperaTampon,"110005020102"); break;
		case 1 /* W */: strcpy(OperaTampon,"010a0001"); break;
		case 2 /* P */: strcpy(OperaTampon,"800010"); break;
		case 3 /* Q */: strcpy(OperaTampon,"03"); break;
	}
	if((OperaCde == 0) && (repart->famille == FAMILLE_IPE)) strcpy(OperaTampon,"140000030800");
	switch(OperaCde) {
	  case 0 /* H */:
		if(OpiumReadText("horloge,0,retard,sel,acq,sync (02X)",OperaTampon,16) == PNL_CANCEL) return(0);
		buffer[0] = 'h'; buffer[1] = _FPGA_code_horloge; k = 2; lngr = 6;
		break;
	  case 1 /* W */:
		if(OpiumReadText("adrs,ssadrs,data0,data1 (02X)",OperaTampon,16) == PNL_CANCEL) return(0);
		buffer[0] = 'W'; buffer[1] = _FPGA_code_EDW; k = 2; lngr = 4;
		break;
	  case 2 /* P */:
		if(OpiumReadText("paquets,port1,port0 (02X)",OperaTampon,16) == PNL_CANCEL) return(0);
		buffer[0] = 'P'; k = 1; lngr = 3;
		break;
	  case 3 /* Q */:
		if(OpiumReadText("code (02X)",OperaTampon,16) == PNL_CANCEL) return(0);
		buffer[0] = 'Q'; k = 1; lngr = 1;
		break;
	  case 4 /* X */:
		buffer[0] = 'X'; k = 1; lngr = 0;
		break;
	}
	strncpy(entree,OperaTampon,32);
	i = lngr;
	while(i) {
		entree[2 * i] = '\0';
		--i;
		sscanf(entree+(2*i),"%x",&val);
		buffer[k + i] = (byte)(val & 0xFF);
	}
	lngr += k;
	RepartIpEcrit(repart,buffer,lngr); SambaMicrosec(ModeleRep[repart->type].delai_msg);
	printf("===== Ecriture de %c",buffer[0]);
	for(i=1; i<lngr; i++) printf(".%02X",buffer[i]);
	printf("\n");

	return(1);
}
/* ========================================================================== */
static int RepartiteurBasique() {
	if(RepartConnecte->interf == INTERF_IP) RepartiteurIpBasique(RepartConnecte);
	else OpiumError(L_("Commandes basiques non programmees pour les repartiteurs avec interface %s"),
		InterfaceListe[RepartConnecte->interf]);
	return(0);
}
/* ========================================================================== */
static int RepartiteurAfficheEntree1() {
	TypeNumeriseur *numeriseur;

	// NumeriseurAfficheChoix(&(NumeriseurAnonyme[0]));
	//- printf("(%s) Affiche numeriseur @%08X\n",__func__,RepartConnecte->entree[0].adrs);
	numeriseur = (TypeNumeriseur *)(RepartConnecte->entree[0].adrs);
	numeriseur->status.a_copier = RepartConnecte->status_demande;
	NumeriseurAfficheChoix(numeriseur);
	return(0);
}
/* ========================================================================== */
static int RepartiteurAfficheEntree2() {
	TypeNumeriseur *numeriseur;

	numeriseur = (TypeNumeriseur *)(RepartConnecte->entree[1].adrs);
	numeriseur->status.a_copier = RepartConnecte->status_demande;
	NumeriseurAfficheChoix(numeriseur);
	return(0);
}
/* ========================================================================== */
static int RepartiteurResetFibres() {
	IpeSetupFibres(RepartConnecte);
	return(0);
}
/* ========================================================================== */
static int RepartiteurAfficheEntreeIpe() {
	int l; TypeNumeriseur *numeriseur;

	if(OpiumExec(pRepartIpeChoixEntree->cdr) == PNL_CANCEL) return(0);
	l = ((RepartIpeFlt - 1) * IPE_FLT_SIZE) + (RepartIpeFibre - 1);
	if((l >= 0) && (l < RepartConnecte->maxentrees)) {
		if((numeriseur = (TypeNumeriseur *)(RepartConnecte->entree[l].adrs))) {
			numeriseur->status.a_copier = RepartConnecte->status_demande;
			NumeriseurAfficheChoix(numeriseur);
		} else OpiumError(L_("Pas de numeriseur lu via l'entree locale #%d"),l);
	} else OpiumError(L_("Illegal: le numero d'entree locale vaut %d, dans ce cas"),l);
	return(0);
}
/* ========================================================================== */
static Menu mOperaPlusMoins;
static TypeMenuItem iOperaPlusMoins[] = {
	{ "-", MNU_FONCTION OperaDelayM },
	{ "-", MNU_FONCTION OperaClockM },
	{ "+", MNU_FONCTION OperaDelayP },
	{ "+", MNU_FONCTION OperaClockP },
	MNU_END
};
Menu mOperaConnections;
static TypeMenuItem iOperaConnections[] = {
	{ "Commandes basiques",  MNU_FONCTION RepartiteurBasique },
	{ "Afficher entree 1",   MNU_FONCTION RepartiteurAfficheEntree1 },
	{ "Afficher entree 2",   MNU_FONCTION RepartiteurAfficheEntree2 },
	{ "Deconnecter entrees", MNU_FONCTION RepartiteurDeconnecteBN },
	MNU_END
};

static Menu mIpePlusMoins;
static TypeMenuItem iIpePlusMoins[] = {
	{ "-", MNU_FONCTION IpeClockM },
	{ "+", MNU_FONCTION IpeClockP },
	MNU_END
};
Menu mIpeConnections;
static TypeMenuItem iIpeConnections[] = {
	{ "Reset fibres",        MNU_FONCTION RepartiteurResetFibres },
	{ "Chargement FPGA",     MNU_FONCTION IpeChargeFpga },
	{ "Commandes basiques",  MNU_FONCTION RepartiteurBasique },
	{ "Afficher une entree", MNU_FONCTION RepartiteurAfficheEntreeIpe },
	MNU_END
};
/* ========================================================================== */
int RepartiteursAffiche(TypeRepartiteur *repart) {
	char a_construire; Menu dernier;

	a_construire = 1; dernier = 0;
	if(!cRepartAcq) cRepartAcq = BoardCreate();
	else {
		if(OpiumDisplayed(cRepartAcq)) OpiumClear(cRepartAcq);
		if(repart == RepartConnecte) a_construire = 0;
		else BoardTrash(cRepartAcq);
	}
	if(a_construire) {
		int x,y,ymin,cols;
		Panel p; int i,nb;
		Instrum instrum,i_donnees,i_traces;

		x = 0; y = Dy;

		if(repart->rep >= 0) {
			p = PanelCreate(1);
			PanelItemSelect(p,PanelList(p,L_("Repartiteur"),RepartListe,&(repart->rep),REPART_NOM),0);
			PanelMode(p,PNL_RDONLY|PNL_DIRECT);
			PanelSupport(p,WND_CREUX);
			BoardAddPanel(cRepartAcq,p,x,y,0); y += ((PanelItemNb(p) + 1) * Dy);
			if(RepartNb > 1) BoardAddMenu(cRepartAcq,MenuBouton(L_("Autre repartiteur"),MNU_FONCTION RepartiteursCommande),x,y,0); y += (2 * Dy);
		}
		if(repart->famille == FAMILLE_IPE) {
			p = PanelCreate(1); // PanelMode(p,PNL_DIRECT);
			PanelHex(p,"PixBus",(int *)&IpePixBus[repart->r.ipe.chassis]);
			PanelSupport(p,WND_CREUX);
			PanelOnApply(p,IpeChangePixbus,0);
			BoardAddPanel(cRepartAcq,p,x + (30 * Dx),Dy,0);
		}
		if((repart->famille == FAMILLE_CALI) || (repart->famille == FAMILLE_IPE)) {
			p = PanelCreate(1); PanelSupport(p,WND_CREUX);
			if(repart->famille == FAMILLE_CALI)
				PanelListB(p,L_("Mode repartiteur"),CaliCodesDataMode,&RepartDataMode,14);
			else PanelListB(p,L_("Mode repartiteur"),IpeCodesDataMode,&RepartDataMode,14);
			PanelOnApply(p,RepartiteurModeDemande,0);
			BoardAddPanel(cRepartAcq,p,x,y,0); y += ((PanelItemNb(p) + 2) * Dy);
			instrum = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&repart->check,L_("ignore/test"));
			OpiumSupport(instrum->cdr,WND_RAINURES);
			BoardAddInstrum(cRepartAcq,instrum,OPIUM_A_DROITE_DE 0);
		}

		if((repart->famille == FAMILLE_CLUZEL) || (repart->famille == FAMILLE_OPERA)
		|| (repart->famille == FAMILLE_IPE) || (repart->famille == FAMILLE_SAMBA)) {
			p = PanelCreate(1); PanelSupport(p,WND_CREUX);
			PanelListB(p,L_("Mode numeriseurs"),NumerModeTexte,&NumerModeAcquis,14);
			PanelOnApply(p,RepartiteurChangeNumerMode,0);
			BoardAddPanel(cRepartAcq,p,x,y,0); y += ((PanelItemNb(p) + 2) * Dy);
		}
		ymin = y;

		if(repart->famille == FAMILLE_IPE) {
			int nbinfos,k,flt,fibre,fltabs; Figure fig; int x0,xc,haut,larg,dep,x;
			int h; char *texte;
			char style_chassis;

			k = 0; // GCC
			nbinfos = 7 + (3 * IPE_FLT_SIZE);
			style_chassis = 1;
			if(style_chassis) {
				nbinfos += 1;
				p = pIpeStatus = PanelCreate(repart->r.ipe.fltnb * nbinfos);
				PanelColumns(p,repart->r.ipe.fltnb); /* PanelMode(p,PNL_DIRECT);  */
			} else {
				p = pIpeStatus = PanelCreate((1 + repart->r.ipe.fltnb) * nbinfos);
				PanelColumns(p,nbinfos); PanelMode(p,PNL_BYLINES); /* |PNL_DIRECT */
				PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"sel",3),0),1);
				PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"lat",3),0),1);
				PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"ena",3),0),1);
				PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"BB",3),0),1);
				PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"mod",3),0),1);
			#ifdef PAR_FIBRE
				for(fibre=0; fibre<IPE_FLT_SIZE; fibre++) {
					PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"d12",3),0),1);
					PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"120",3),0),1);
					PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"SM",3),0),1);
				}
				larg = 3 * 5 * Dx;
			#else
				for(fibre=0; fibre<IPE_FLT_SIZE; fibre++) PanelItemSouligne(p,PanelItemSelect(p,PanelItemLngr(p,PanelItemFormat(p,PanelChar(p,0,&(SambaUserNum[fibre])),"%2d"),3),0),1);
				for(fibre=0; fibre<IPE_FLT_SIZE; fibre++) PanelItemSouligne(p,PanelItemSelect(p,PanelItemLngr(p,PanelItemFormat(p,PanelChar(p,0,&(SambaUserNum[fibre])),"%2d"),3),0),1);
				for(fibre=0; fibre<IPE_FLT_SIZE; fibre++) PanelItemSouligne(p,PanelItemSelect(p,PanelItemLngr(p,PanelItemFormat(p,PanelChar(p,0,&(SambaUserNum[fibre])),"%2d"),3),0),1);
				larg = IPE_FLT_SIZE * 5 * Dx;
			#endif
			}
			for(flt=0; flt<repart->r.ipe.fltnb; flt++) {
				fltabs = IPE_FLT_ABS(repart,flt);
				if(style_chassis) {
					if(!flt) texte = "FLT"; else texte = 0;
					PanelItemSouligne(p,PanelItemSelect(p,PanelItemLngr(p,PanelItemFormat(p,PanelChar(p,texte,&(SambaUserNum[fltabs])),"%2d"),3),0),1);
					if(!flt) texte = L_("branchee"); else texte = 0;
				} else texte = &(IpeFltNom[fltabs][0]);
			k = PanelKeyB(p,texte," -/ +",(char *)&(repart->r.ipe.flt[flt].branchee),3);
				PanelItemColors(p,k,SambaVioletVert,2);
				if(style_chassis & !flt) texte = "enabled"; else texte = 0;
			k = PanelKeyB(p,texte," -/ +",(char *)&(repart->r.ipe.flt[flt].enabled),3);
				PanelItemColors(p,k,SambaRougeVert,2);
				if(style_chassis & !flt) texte = "active"; else texte = 0;
			k = PanelKeyB(p,texte," -/ +",(char *)&(repart->r.ipe.flt[flt].active),3);
				PanelItemColors(p,k,SambaJauneVertOrangeBleu,2); PanelItemSelect(p,k,0);
				if(style_chassis & !flt) texte = "Special"; else texte = 0;
				PanelItemLngr(p,PanelItemFormat(p,PanelByte(p,texte,&(repart->r.ipe.flt[flt].controle.partiel.speciaux)),"%02X"),3);
				if(style_chassis & !flt) texte = "InEnable"; else texte = 0;
				PanelItemLngr(p,PanelItemFormat(p,PanelByte(p,texte,&(repart->r.ipe.flt[flt].controle.partiel.enable)),"%02X"),3);
				if(style_chassis & !flt) texte = "BB1 clock"; else texte = 0;
				PanelItemLngr(p,PanelItemFormat(p,PanelByte(p,texte,&(repart->r.ipe.flt[flt].controle.partiel.modeBB)),"%02X"),3);
				if(style_chassis & !flt) texte = "FLT mode"; else texte = 0;
			k = PanelItemLngr(p,PanelItemFormat(p,PanelByte(p,texte,&(repart->r.ipe.flt[flt].controle.partiel.modeFLT)),"%02X"),3);
				if(style_chassis) PanelItemSouligne(p,k,1);
			#ifdef PAR_FIBRE
				for(fibre=0; fibre<IPE_FLT_SIZE; fibre++) {
					PanelItemLngr(p,PanelItemFormat(p,PanelByte(p,0,&(IpeStatus[flt][fibre].delai12)),"%02X"),3);
					PanelItemLngr(p,PanelItemFormat(p,PanelByte(p,0,&(IpeStatus[flt][fibre].delai120)),"%02X"),3);
					PanelItemLngr(p,PanelItemFormat(p,PanelByte(p,0,&(IpeStatus[flt][fibre].selection)),"%02X"),3);
				}
			#else
				h = 0;
				for(fibre=0; fibre<IPE_FLT_SIZE; fibre++) {
					IpeStatus[flt][fibre].delai12 = repart->r.ipe.retard[IPE_ENTREE(flt,fibre)] & 0xF;
					IpeStatus[flt][fibre].delai120 = (repart->r.ipe.retard[IPE_ENTREE(flt,fibre)] >> 4) & 0xF;
				}
				for(fibre=0; fibre<IPE_FLT_SIZE; fibre++) {
					if(style_chassis & !flt) { sprintf(&(IpeRegEntete[h][0]),"D12.%d",fibre+1); texte = &(IpeRegEntete[h][0]); h++; } else texte = 0;
					k = PanelItemFormat(p,PanelByte(p,texte,&(IpeStatus[flt][fibre].delai12)),"%2X");
				}
				if(style_chassis) PanelItemSouligne(p,k,1);
				for(fibre=0; fibre<IPE_FLT_SIZE; fibre++) {
					if(style_chassis & !flt) { sprintf(&(IpeRegEntete[h][0]),"D120.%d",fibre+1); texte = &(IpeRegEntete[h][0]); h++; } else texte = 0;
					k = PanelItemFormat(p,PanelByte(p,texte,&(IpeStatus[flt][fibre].delai120)),"%2X");
				}
				if(style_chassis) PanelItemSouligne(p,k,1);
				for(fibre=0; fibre<IPE_FLT_SIZE; fibre++) {
					if(style_chassis & !flt) { sprintf(&(IpeRegEntete[h][0]),"ADCsel.%d",fibre+1); texte = &(IpeRegEntete[h][0]); h++; } else texte = 0;
					PanelItemFormat(p,PanelByte(p,texte,&(IpeStatus[flt][fibre].selection)),"%2X");
				}
			#endif
			}
			PanelSupport(p,WND_CREUX);
			PanelOnApply(p,IpeChangeParmsFlt,0);
			/* nb = PanelItemNb(p);
			for(i=0; i<nb; i++) if(PanelItemIsSelected(p,i) {
				PanelItemModif(p,i,IpeRessEnCours,(void *)i);
				PanelItemExit(p,i,IpeRessChangee,(void *)i);
			} */

			if(style_chassis) {
				PanelItemSouligne(p,k,1);
				BoardAddPanel(cRepartAcq,p,60 * Dx,Dy,0);
			} else {
				x0 = 50 * Dx; xc = 4 * 5 * Dx;
				dep = x0 + (((repart->r.ipe.fltnb < 10)? 13: 14) * Dx); haut = 3 * Dy;
				RepartLigneVert1.dy = 22 * Dy;
				RepartLigneVert2.dy = (1 + repart->r.ipe.fltnb) * Dy;
				RepertIconeCntrl.larg = xc - 4; RepertIconeCntrl.haut = Dy + 4;
				RepertIconeRegistre.larg = larg - 4; RepertIconeRegistre.haut = Dy + 4;

				BoardAddPanel(cRepartAcq,p,x0+Dx,haut,0);
				BoardAddFigure(cRepartAcq,FigureCreate(FIG_DROITE,(void *)&RepartLigneVert1,0,0),x0,Dy,0);
				BoardAddFigure(cRepartAcq,FigureCreate(FIG_DROITE,(void *)&RepartLigneVert2,0,0),dep,haut-2,0);
				fig = FigureCreate(FIG_ZONE,(void *)&RepertIconeCntrl,0,0); FigureTexte(fig,L_("Controle"));
				BoardAddFigure(cRepartAcq,fig,dep+2,Dy,0);
			#ifdef PAR_FIBRE
				for(fibre=0; fibre<IPE_FLT_SIZE; fibre++) {
					char titre[16];
					sprintf(titre,"Fibre #%d",fibre+1);
					fig = FigureCreate(FIG_ZONE,(void *)&RepertIconeRegistre,0,0); FigureTexte(fig,titre);
					BoardAddFigure(cRepartAcq,fig,dep + xc + (larg * fibre),Dy,0);
					BoardAddFigure(cRepartAcq,FigureCreate(FIG_DROITE,(void *)&RepartLigneVert2,0,0),(dep + 20 + (15 * fibre))*Dx,haut-2,0);
				}
			#else
				x = dep + xc;
				BoardAddFigure(cRepartAcq,FigureCreate(FIG_DROITE,(void *)&RepartLigneVert2,0,0),x,haut-2,0);
				fig = FigureCreate(FIG_ZONE,(void *)&RepertIconeRegistre,0,0); FigureTexte(fig,L_("Delais 20"));
				BoardAddFigure(cRepartAcq,fig,x+2,Dy,0);
				x += larg;
				BoardAddFigure(cRepartAcq,FigureCreate(FIG_DROITE,(void *)&RepartLigneVert2,0,0),x,haut-2,0);
				fig = FigureCreate(FIG_ZONE,(void *)&RepertIconeRegistre,0,0); FigureTexte(fig,L_("Delais 120"));
				BoardAddFigure(cRepartAcq,fig,x+2,Dy,0);
				x += larg;
				BoardAddFigure(cRepartAcq,FigureCreate(FIG_DROITE,(void *)&RepartLigneVert2,0,0),x,haut-2,0);
				fig = FigureCreate(FIG_ZONE,(void *)&RepertIconeRegistre,0,0); FigureTexte(fig,"Selection ADC");
				BoardAddFigure(cRepartAcq,fig,x+2,Dy,0);
				x += larg;
				BoardAddFigure(cRepartAcq,FigureCreate(FIG_DROITE,(void *)&RepartLigneVert2,0,0),x,haut-2,0);
			#endif
			}
		}

		cols = 7;
		if(repart->famille == FAMILLE_CLUZEL) {
			pRepartStatus = PanelCreate(2); // pas de date_status
			PanelShort(pRepartStatus,"d2",&repart->r.cluzel.d2);
			PanelShort(pRepartStatus,"d3",&repart->r.cluzel.d3);
			cols += 2; /* lngr max texte items */
		} else if(repart->famille == FAMILLE_NI) {
			pRepartStatus = PanelCreate(1); // pas de date_status
			PanelShort(pRepartStatus,"nb.voies",&repart->voies_par_numer);
			cols += 8; /* lngr max texte items */
		} else if(repart->famille == FAMILLE_OPERA) {
			pRepartStatus = PanelCreate(15+1); // attention a date_status (plus loin)
			PanelOctet (pRepartStatus,"Retard",&repart->r.opera.retard);
			PanelShort(pRepartStatus,"D0",&repart->r.opera.d0);
			PanelKeyB (pRepartStatus,"D2D3",CODES_SYNCHRO,(char *)&repart->r.opera.d2d3,8); /* d0 et retard en face de leurs touches + et - */
			PanelKeyB (pRepartStatus,"Horloge",CODES_ESCLAVE,&repart->r.opera.clck,8);
			PanelKeyB (pRepartStatus,"Synchro",CODES_ESCLAVE,&repart->r.opera.sync,8);
			PanelList (pRepartStatus,"Mode",OperaCodesAcquis,&repart->r.opera.mode,10);
			if(repart->rep >= 0) {
				PanelItemSelect(pRepartStatus,PanelSHex (pRepartStatus,"Masque",(short *)&OperaMasque),0);
				PanelItemSelect(pRepartStatus,PanelShort(pRepartStatus,"Voies/BN",&repart->voies_par_numer),0);
			}
			PanelListS(pRepartStatus,"Entree BN",OperaEntreesBN,(short *)&OperaCodeEntrees,10);
			if(repart->rep >= 0) {
				PanelItemSelect(pRepartStatus,PanelText (pRepartStatus,"FPGA BN",RepartInfoFpgaBB,16),0);
				PanelItemSelect(pRepartStatus,PanelInt  (pRepartStatus,"Logiciel",&RepartVersionLogiciel),0);
				PanelItemSelect(pRepartStatus,PanelInt  (pRepartStatus,"Nb D3",&OperaNbD3),0);
				PanelItemSelect(pRepartStatus,PanelInt  (pRepartStatus,"Resynchro",&OperaErreursCew),0);
				PanelItemSelect(pRepartStatus,PanelInt  (pRepartStatus,"Desynchro",&OperaErreursTimeStamp),0);
				PanelItemSelect(pRepartStatus,PanelInt  (pRepartStatus,"Perdu",&OperaErreursSynchro),0);
			}
			PanelOnApply(pRepartStatus,OperaChangeParms,0);
			cols += 17;
		} else if(repart->famille == FAMILLE_CALI) {
			pRepartStatus = PanelCreate(7+1); // attention a date_status (plus loin)
			PanelKeyB(pRepartStatus,L_("Horloge"),CODES_ESCLAVE,&repart->r.cali.clck,8);
			PanelItemSelect(pRepartStatus,PanelInt   (pRepartStatus,L_("No Trame"),(int *)&(CaliTrameLue)),0);
			PanelItemSelect(pRepartStatus,PanelOctet  (pRepartStatus,"Version",&(CaliVersionLue)),0);
			PanelItemSelect(pRepartStatus,PanelItemFormat(pRepartStatus,PanelByte(pRepartStatus,"Status v1",&(CaliEnteteLue.status[0])),"%02X"),0);
			PanelItemSelect(pRepartStatus,PanelItemFormat(pRepartStatus,PanelByte(pRepartStatus,"Status v2",&(CaliEnteteLue.status[1])),"%02X"),0);
			PanelItemSelect(pRepartStatus,PanelItemFormat(pRepartStatus,PanelByte(pRepartStatus,"Status v3",&(CaliEnteteLue.status[2])),"%02X"),0);
			PanelItemSelect(pRepartStatus,PanelItemFormat(pRepartStatus,PanelByte(pRepartStatus,"Status v4",&(CaliEnteteLue.status[3])),"%02X"),0);
			cols += 9 + 5;
		} else if(repart->famille == FAMILLE_IPE) {
			pRepartStatus = PanelCreate(14+1); // attention a date_status (plus loin)
			PanelShort(pRepartStatus,"D0",&repart->r.ipe.d0);
			PanelKeyB (pRepartStatus,L_("Horloge"),CODES_ESCLAVE,&repart->r.ipe.clck,8);
			PanelItemSelect(pRepartStatus,PanelInt  (pRepartStatus,L_("Nb synchros D3"),(int *)&IpeTempsLu),0);
			PanelItemSelect(pRepartStatus,PanelHex  (pRepartStatus,"PixBus",(int *)&IpePixBus[repart->r.ipe.chassis]),0);
			PanelItemSelect(pRepartStatus,PanelText (pRepartStatus,"FPGA BB",RepartInfoFpgaBB,16),0);
			PanelItemSelect(pRepartStatus,PanelInt  (pRepartStatus,L_("Nb.erreurs"),(int *)&IpeErreursNb),0);
			PanelItemSelect(pRepartStatus,PanelHex  (pRepartStatus,L_("Erreur interne"),(int *)&IpeInternalInfo),0);
			PanelItemSelect(pRepartStatus,PanelHex  (pRepartStatus,L_("Etat Logiciel"),(int *)&IpeSWstatus),0);
			PanelItemSelect(pRepartStatus,PanelInt  (pRepartStatus,L_("Version Logiciel"),&RepartVersionLogiciel),0);
			PanelItemSelect(pRepartStatus,PanelShort(pRepartStatus,"Nb.FIFO",&IpeFifoNb),0);
			PanelItemSelect(pRepartStatus,PanelShort(pRepartStatus,"Nb.ADC",&IpeAdcNb),0);
			PanelItemSelect(pRepartStatus,PanelInt  (pRepartStatus,L_("Trame maxi"),(int *)&IpeTrameDim),0);
			PanelItemSelect(pRepartStatus,PanelHex  (pRepartStatus,"TimeStamp MSB",(int *)&IpeStampMsb),0);
			PanelItemSelect(pRepartStatus,PanelHex  (pRepartStatus,"TimeStamp LSB",(int *)&IpeStampLsb),0);
			PanelOnApply(pRepartStatus,IpeChangeParmsGlob,0);
			cols += 16 + 9;
		} else pRepartStatus = 0;
		if(pRepartStatus) {
			nb = PanelItemNb(pRepartStatus);
			for(i=0; i<nb; i++) PanelItemModif(pRepartStatus,i+1,RepartiteurParmEnCours,0);
			PanelSupport(pRepartStatus,WND_CREUX);
			BoardAddPanel(cRepartAcq,pRepartStatus,x,y,0); y += ((nb + 2) * Dy);
		} else nb = 0;

		if(repart->famille == FAMILLE_OPERA) {
			mOperaPlusMoins = MenuLocalise(iOperaPlusMoins);
			MenuColumns(mOperaPlusMoins,2); OpiumSupport(mOperaPlusMoins->cdr,WND_PLAQUETTE);
			BoardAddMenu(cRepartAcq,mOperaPlusMoins,OPIUM_A_DROITE_DE pRepartStatus->cdr);
			dernier = mOperaPlusMoins;
			if(repart->rep >= 0) {
				mOperaConnections = MenuLocalise(iOperaConnections);
				OpiumSupport(mOperaConnections->cdr,WND_PLAQUETTE);
				BoardAddMenu(cRepartAcq,mOperaConnections,OPIUM_EN_DESSOUS_DE mOperaPlusMoins->cdr);
				dernier = mOperaConnections;
			}
		} else if(repart->famille == FAMILLE_IPE) {
			mIpePlusMoins = MenuLocalise(iIpePlusMoins);
			MenuColumns(mIpePlusMoins,2); OpiumSupport(mIpePlusMoins->cdr,WND_PLAQUETTE);
			BoardAddMenu(cRepartAcq,mIpePlusMoins,OPIUM_A_DROITE_DE pRepartStatus->cdr);
			dernier = mIpePlusMoins;
		}

		if(repart->rep >= 0) {
			int calage_gauche,en_bas,etage1,etage2; char status_long;

			if((repart->famille == FAMILLE_OPERA) || (repart->famille == FAMILLE_CALI) || (repart->famille == FAMILLE_IPE)) {
				PanelItemSelect(pRepartStatus,PanelText(pRepartStatus,L_("Etat a"),repart->date_status,10),0); y += Dy; nb++;
				iRepartRafraichi = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&repart->status_demande,L_("fige/a jour"));
				InstrumOnChange(iRepartRafraichi,RepartiteurAcquiert,0);
				OpiumSupport(iRepartRafraichi->cdr,WND_RAINURES);
			} else iRepartRafraichi = 0;
			//? if(repart->lec.ip.path > 0) {
				if(cols < 14) cols = 14 * Dx; calage_gauche = x + ((cols + 4) * Dx);
				if((y - (12 * Dy)) >= ymin) { en_bas = y - (4 * Dy); etage1 = en_bas - (4 * Dy); etage2 = etage1 - (4 * Dy); }
				else { etage2 = ymin; etage1 = etage2 + (4 * Dy); en_bas = etage1 + (4 * Dy); }
				status_long = (nb >= ((y - etage2) / Dy));

				i_donnees = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&RepartDonneesAffiche,L_("sans/avec"));
				InstrumTitle(i_donnees,L_("Donnees"));
				OpiumSupport(i_donnees->cdr,WND_RAINURES);
				i_traces = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&RepartCourbesAffiche,L_("sans/avec"));
				InstrumTitle(i_traces,L_("Traces"));
				OpiumSupport(i_traces->cdr,WND_RAINURES);

				if(repart->famille == FAMILLE_OPERA) {
					if(dernier) BoardAddInstrum(cRepartAcq,i_donnees,OPIUM_EN_DESSOUS_DE dernier->cdr);
					else BoardAddInstrum(cRepartAcq,i_donnees,OPIUM_EN_DESSOUS_DE 0);
					BoardAddInstrum(cRepartAcq,i_traces,OPIUM_EN_DESSOUS_DE 0);
					BoardAddInstrum(cRepartAcq,iRepartRafraichi,OPIUM_EN_DESSOUS_DE 0);
				} else if(repart->famille == FAMILLE_IPE) {
					BoardAddInstrum(cRepartAcq,i_donnees,calage_gauche,13 * Dy,0);
					BoardAddInstrum(cRepartAcq,i_traces,calage_gauche,17 * Dy,0);
					BoardAddInstrum(cRepartAcq,iRepartRafraichi,calage_gauche,en_bas,0);
				} else if(status_long) {
					BoardAddInstrum(cRepartAcq,i_donnees,calage_gauche,etage2,0);
					BoardAddInstrum(cRepartAcq,i_traces,calage_gauche,etage1,0);
					BoardAddInstrum(cRepartAcq,iRepartRafraichi,OPIUM_EN_DESSOUS_DE 0);
				} else {
					BoardAddInstrum(cRepartAcq,i_donnees,calage_gauche,etage1,0);
					BoardAddInstrum(cRepartAcq,i_traces,OPIUM_A_DROITE_DE 0);
					BoardAddInstrum(cRepartAcq,iRepartRafraichi,calage_gauche,en_bas,0);
				}
			//? }
		}

		if(repart->famille == FAMILLE_OPERA) {
			p = PanelCreate(1); PanelSupport(p,WND_CREUX);
			PanelListB(p,"Mise a jour",OperaCodesMaj,&OperaParmQ,14);
			PanelOnApply(p,OperaMaj,0);
			BoardAddPanel(cRepartAcq,p,x,y,0); y += ((PanelItemNb(p) + 2) * Dy);
		} else if(repart->famille == FAMILLE_IPE) {
			mIpeConnections = MenuLocalise(iIpeConnections);
			OpiumSupport(mIpeConnections->cdr,WND_PLAQUETTE);
			BoardAddMenu(cRepartAcq,mIpeConnections,x+(15*Dx),y+Dy,0);
			dernier = mIpeConnections;
		}
	}

	RepartConnecte = repart;
	if(repart->rep >= 0) {
		printf("%s/ ----- Utilisation du repartiteur %s -----\n",DateHeure(),repart->nom);
		OpiumTitle(cRepartAcq,repart->nom);
	} else {
		printf("%s/ ----- Utilisation d'une liste de repartiteur -----\n",DateHeure());
		OpiumTitle(cRepartAcq,"Diffusion");
	}
	OpiumFork(cRepartAcq);
	return(0);
}
/* ========================================================================== */
static int RepartiteurUsage(Menu menu, MenuItem item) {
	if((RepartSchemaChange = (OpiumReadBool("Repartiteur a utiliser",&(RepartConnecte->valide)) == PNL_OK))) {
		OrgaTraceTout(0,0);
		RepartiteursASauver = 1;
	}
	return(0);
}
/* ========================================================================== */
static int RepartiteurProposeNumeriseur(Panel panel, int item, void *arg) {
	TypeNumeriseur *numeriseur;

	if(panel) {
		PanelApply(panel,1);
		numeriseur = (TypeNumeriseur *)(RepartConnecte->entree[RepartEntreeChangee].adrs);
		if(numeriseur) RepartBBbranche = numeriseur->bb;
		if(OpiumDisplayed(panel->cdr)) OpiumRefresh(panel->cdr);
	}

	return(0);
}
/* ========================================================================== */
static int RepartiteurChangeEntree(Menu menu, MenuItem item) {
	TypeRepartiteur *repart; TypeNumeriseur *numeriseur;
	char **liste; int l; Panel p; int k,rc;

	repart = RepartConnecte;
	liste = (char **)malloc((repart->maxentrees + 1) * sizeof(char *));
	if(!liste) { OpiumError("Memoire saturee"); return(0); }
	for(l=0; l<repart->maxentrees; l++) {
		numeriseur = (TypeNumeriseur *)(repart->entree[l].adrs);
		if(numeriseur) liste[l] = numeriseur->nom;
		else {
			liste[l] = (char *)malloc(NUMER_NOM + 1);
			if(!liste[l]) { OpiumError("Memoire saturee"); return(0); }
			snprintf(liste[l],NUMER_NOM,"#%d (vide)",l+1);
		}
	}
	liste[l] = "\0";
	p = PanelCreate(2);
	k = PanelList(p,"Entree a modifier",liste,&RepartEntreeChangee,NUMER_NOM);
	PanelList(p,"Numeriseur a brancher",NumeriseurListe,&RepartBBbranche,NUMER_NOM);
	PanelItemExit(p,k,RepartiteurProposeNumeriseur,0);
	RepartEntreeChangee = 0; RepartBBbranche = 0;
	rc = OpiumExec(p->cdr);
	PanelDelete(p);

	NumeriseurTermineAjout();

	if(rc == PNL_CANCEL) return(0);
	numeriseur = (TypeNumeriseur *)(RepartConnecte->entree[RepartEntreeChangee].adrs);
	if(RepartBBbranche < NumeriseurNb) {
		if(numeriseur) /* c'est l'ancien */ {
			if(RepartBBbranche == numeriseur->bb) return(0);
			numeriseur->repart = 0;
			numeriseur->entree = 0;
		}
		numeriseur = &(Numeriseur[RepartBBbranche]);
		if((repart = numeriseur->repart) && (numeriseur->entree >= 0) && (numeriseur->entree < repart->maxentrees)) {
			repart->entree[numeriseur->entree].adrs = 0; /* c'est l'ancien (peut-etre) */
		}
		repart = RepartConnecte; l = RepartEntreeChangee;
		RepartiteurBranche(repart,l,numeriseur);
		//- repart->entree[l].nbident = vpn;
		printf("* %s branchee sur %s.%d\n",numeriseur->nom,repart->nom,l+1);
		OrgaTraceTout(0,0);
	} else {
		if(!numeriseur) return(0);
		numeriseur->repart = 0;
		numeriseur->entree = 0;
		RepartConnecte->entree[RepartEntreeChangee].adrs = 0;
		//- repart->entree[l].nbident = 0;
		printf("* Entree %s.%d debranchee\n",repart->nom,RepartEntreeChangee+1);
		OrgaTraceTout(0,0);
	}
	RepartCompteDonnees(repart);
	RepartiteursASauver = 1;

	return(0);
}
/* ========================================================================== */
int RepartiteursClique(Figure fig, WndAction *e) {
	TypeRepartiteur *repart;

	repart = (TypeRepartiteur *)(fig->adrs);
	RepartConnecte = repart;
	if(e->code == WND_MSERIGHT) {
		RepartSchemaChange = 0;
		// OpiumReadBool("Valide",&(repart->valide));
		OpiumExec(mRepartiteurClique->cdr);
	} else RepartiteursAffiche(repart);
	return(0);
}
/* ========================================================================== */
int RepartiteursCommande() {
	int rep;

	if(!RepartConnecte) RepartConnecte = &(Repart[0]);
	//? RepartiteursRenomme();
	rep = RepartConnecte->rep; if(rep < 0) rep = 0;
	if(RepartNb > 1) {
		if(OpiumReadList("Repartiteur",RepartListe,&rep,REPART_NOM) == PNL_CANCEL) return(0);
	} else rep = 0;
	if(cRepartAcq && OpiumDisplayed(cRepartAcq) && (rep == RepartConnecte->rep)) return(0);
	RepartConnecte->status_demande = 0;
	if(Acquis[AcquisLocale].etat.active) {
		int l;
		RepartiteurAcquiert(); /* pour arreter proprement l'acquisition */
		for(l=0; l<RepartConnecte->nbentrees; l++) {
			TypeNumeriseur *numeriseur; TypeNumeriseurCmde *interface;
			numeriseur = RepartConnecte->entree[l].adrs;
			if(numeriseur) {
				NumeriseurArreteTraces(numeriseur);
				interface = &(numeriseur->interface);
				if(interface) {
					if(OpiumDisplayed(interface->planche)) OpiumClear(interface->planche);
				}
			}
		}
	}
	RepartiteursAffiche(&(Repart[rep]));
	return(0);
}
