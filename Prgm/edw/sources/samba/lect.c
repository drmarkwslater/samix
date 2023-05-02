/*		Branche 'lecture': lit soit la carte PCI, soit un fichier de donnees brutes,
		place les donnees brutes dans un tampon et appelle le traitement pre-trigger
		a intervalles reguliers (<SettingsIntervTrmt> fois une synchronisation de 1 ms)
*/

#include <environnement.h>
// #define MODULES_RESEAU defini (ou non) dans le projet (et non dans environnement.h)

//#ifdef macintosh
//#pragma message("Compilation de "__FILE__)
//#endif
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <strings.h>
#include <sys/time.h>    /* pour gettimeofday */
#include <sys/types.h>
#ifndef CODE_WARRIOR_VSN
	#include <sysexits.h>
	#include <sys/resource.h>
#endif
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>

#ifdef AVEC_IP
	#include <sys/socket.h>
#endif

// #include <sys/types.h>
#ifdef CODE_WARRIOR_VSN
	#ifdef CW5
		#define __func__ "xxx"
	#endif
	#include <stat.h>  /* pour avoir off_t */
#else
	#include <sys/stat.h>  /* pour stat() */
#endif
#include <math.h>

#ifdef PROJECT_BUILDER
	#define sqrtf(x) (float)sqrt((double)x)
	#define expf(x) (float)exp((double)x)
#endif

#ifdef CODE_WARRIOR_VSN
	#include <claps.h>
	#include <decode.h>
	#include <dateheure.h>
	#include <timeruser.h>
	#include <fitlib.h>
	#include <opium_demande.h>
	#include <opium.h>
	//#include <plx.h>
	#include <IcaLib.h>

#else
	#include <utils/claps.h>
	#include <utils/decode.h>
	#include <utils/dateheure.h>
	#include <opium_demande.h>
	#include <opium4/opium.h>
	#include <utils/timeruser.h>
	#include <utils/fitlib.h>
	#include <PCI/IcaDefine.h>
	#ifdef MODULES_RESEAU
		//-- /* UNIX deactive dans wnd.h pour utiliser CARBON... */
		//-- #ifndef UNIX
		//-- #define UNIX
		//-- #endif
		#include <carla/data.h>
		#include <carla/msgs.h>
	#endif
#endif

#include <samba.h>
#include <repartiteurs.h>
#include <numeriseurs.h>
#include <cablage.h>
#include <detecteurs.h>
#include <gene.h>
#include <organigramme.h>
#include <export.h>
#include <archive.h>
#include <banc.h>
#include <diags.h>
#include <objets_samba.h>
#include <sequences.h>
#include <monit.h>
#include <autom.h>

#ifdef PAROLE
	#ifdef VIA_QD_SPEECH
		#include <speech.h>
	#else
		#define SpeakString AudioSpeakP
	#endif
#endif

#ifdef CODE_WARRIOR_VSN
	#ifdef AVEC_PCI
		//#include <PCI/plx.h>
		#include <PCI/IcaLib.h>
	#endif
#endif

#pragma mark -------- Constantes --------
/* ................... definitions propres a la lecture ............... */

#define _DECLENCHE_ 0,0,0,LectErreur.num,LectErreur.code

/* #define STATUS_AVEC_TIMER */
#define MONTRE_VARIABLES_INTERNES
#define SIMUL_SIN

// #define VOIES_FANTOMES
// #define VERIFIE_ALLOCS
// #define VERIFIE_TYPE
#define PAS_D_ERREUR_PERMISE
// #define DEBUG_PCI
// #define DEBUG_ACCES
#define DEBUG_DEPILE
#ifdef DEBUG_DEPILE
#ifdef DEBUG_RAW
// #define DEBUG_FILL
#endif
#endif
// #define DEBUG_T0

// #define VERIF_ETAT_REPART_FINAL
// #define VERIF_TIMESTAMP_FINAL
// #define VERIF_NBEVTS_FINAL
// #define VERIF_STATSCPU_FINALES

typedef enum {
	LECT_RETARD_PREVU = 1,
	LECT_RETARD_CALDR,
	LECT_RETARD_AUTO
} LECT_RETARD_MODE;

typedef enum {
	LECT_COMPACTE_BRUIT = 0,
	LECT_COMPACTE_PARMS,
	LECT_COMPACTE_DONNEES,
	LECT_COMPACTE_COM,
	LECT_COMPACTE_ONGLETSNB
} LECT_COMPACTE_ONGLET;

static char *LectCompacteOngletNoms[LECT_COMPACTE_ONGLETSNB+1] = {
	"Bruit electronique", "Parametres", "Donnees", "Communications" "\0"
};

static int LectCompacteNumero;
static Onglet LectCompacteOnglets;

#pragma mark ------------ Variables -----------
/* ................... structures propres a la lecture ................ */
static char LectASauver;
// static char LectErreurNom[32];
// int  LectErreurNum;

/* Lieu de detection de l'erreur
	serie 00: LectExec + LectIdentBB
	      10: 
	      20: 
	      30: LectVideAdonf+LectActionUtilisateur
	      40: LectDepileOptimise
	      50: LectDepileOptimise
          80: LectSynchro
          90: archive (donc LectErreur.num reference dans archive.c)
*/
static int LectDetectRtneNum[100];
static char *LectDetectNom[] = {
	"LectExec",
	"neant",
	"neant",
	"LectVideAdonf",
	"LectDepileOptimise",
	"LectDepileOptimise",
	"LectActionUtilisateur",
	"LectDisplay",
	"LectSynchro",
	"ArchiveAccede",
	"\0"
};

// #define SOFT_AB
#ifdef SOFT_AB
	#include "util_cew.h"
	static Status_opera etat_opera; Status_bbv2 etat_bbv2; long double temps;
#endif

// #define DEBUG_STATUS_BBV2
#ifdef DEBUG_STATUS_BBV2
	#define MAXSTATUSCODE 1024
	#define MAXSTATUSDECODE 64
		static shex StatusCode[MAXSTATUSCODE],StatusDecode[MAXSTATUSDECODE];
		static short BBlu[MAXSTATUSCODE];
		static int IndexStatusCode,IndexStatusDecode;
	#define MAXTRAMES 4
		static shex BOtrame0[MAXTRAMES][sizeof(TypeOperaTrame)/sizeof(shex)];
	#ifdef SOFT_AB
		static shex ABtrame0[MAXTRAMES][sizeof(TypeOperaTrame)/sizeof(shex)];
	#endif
#endif

typedef enum {
	LECT_PRGM_STD = 1,
	LECT_PRGM_ELEM,
} LECT_PRGM;

static char LectSigne,LectParBloc;
static char LectBoucleExterne,LectDepileSousIt,LectTrmtEnCours;
static char LectAcqEnCours;
static char LectRunNormal,LectStrictStd,LectMaintenance,ReglagesEtatPeutStopper;
static char LectAttendIdent;        /* vrai si attend une identification dans les donnees */
static char LectSynchroReperee;     /* vrai si donnees calees sur une synchro             */
static char LectDemarrageDemande,LectureSuspendue,Lect1ModulALire;
static char LectRelancePrevue,LectArretePoliment;
static char LectBBaffiches,LectRepAffiches;

//- static char  LectVal1Lue;            /* vrai si 1ere valeur d'un bloc deja lue             */
//- static int32 zzLectVal1;             /* 1ere valeur en question...                         */
//- static int32 zzLectLastVal1;         /* precedente valeur de zzLectVal1                    */
//- static char  LectVal1IsStatus;       /* vrai si ladite valeur est le status                */
static char  LectStatusPresent[REPART_MAX];  /* vrai si status present dans le bloc        */
static int32 LectMasqueStatus[REPART_MAX];   /* masque effectif de controle du status      */
static int   LectRefStamp;           /* repartiteur de reference pour calculer D2 d'apres timestamp   */
static char  LectSynchroD3;          /* vrai si synchro D3 dans le bloc courant            */
static int64 LectRecaleD3;           /* nombre total de recalages sur D3                   */
//- static char  LectAffEvt;             /* pour ralentir l'affichage des evts (autorisation d'affichage) */
static int   LectArchLus;            /* nombre de blocs lus sur fichier edw1               */
static int   LectErreurFichier;      /* derniere erreur sur lecture fichier (0 si eof)     */
static int   LectAttenteRebootOpera; /* temps d'attente du reboot Opera                    */
//+ static Timer LectTaskReadout;
static pid_t LectPid;
// static char  LectRetardStartMode,LectRetardStopMode;
// static char  LectRetardStartHeure[12],LectRetardStopHeure[12];
static struct {
	struct {
		char jour[12],heure[12];
		int64 date;
		char mode;
	} start,stop;
} LectRetard;

static int   FifoTotale;
static int64 LectNextUser,LectNextSat;
static int64 LectLastEvt;
static int64 TempsTotalDispo;                    /* cumul temps entre appels de LectDepile      */
static int64 LectUsageInterface[INTERF_NBTYPES]; /* cumul temps d'acces a chaque interface      */
static int64 LectUsageInterfacesToutes;          /* cumul temps d'acces a toutes les interfaces */
static int64 LectDernierStocke,LectHeureStocke;  /* heures d'appel a TraiteStocke               */
static int64 LectDepileIntervMax;
static int64 LectEntreDepile;
static int64 LectDerniereNonVide;
static int   LectDelaisTropCourt,LectDelaisTropLong;
static int64 LectUdpRecues,LectUdpErr,LectUdpOvr;
static int   LectSerieVide;

static char  LectCommentComplet[MAXCOMMENTNB * MAXCOMMENTLNGR];
static int64 LectIntervSeq,LectIntervUser,LectIntervSat;
static int   LectDansTraiteStocke,LectDansDisplay,LectDansActionUtilisateur;
static int   LectBoloDemande;
static short LectVoiesIntrouvables;
static char  LectReglageEnCours;   /* reglage bolo lance depuis bLecture       */
static char  LectPlancheSuiviEnCours;

static Panel pLectRtAuto;
static int   LectRtPas;            /* temps d'attente entre 2 mesures de R(T)  */
static int   LectRtNb;             /* nombre de mesures de R(T)                */

static int  LectMaxAbsent;
static int  LectVoieRep;
// static int  LectRepEnCours;
// static short LectDonneeEnCours;
// static char LectVoieTrmt;      /* traitement de la voie en cours (VoieALire)         */
static int  LectAvantSynchroD2;   /* numero de point (en temps) avant une synchro D2    */
static int  LectBlockSize;        /* mots/bloc                                          */
static TypeADU LectTypePrevu;     /* type de donnee attendu                             */
static float LectBase1Niveau[VOIES_TYPES],LectBase1Bruit[VOIES_TYPES];
static char LectBaseTexteNiv[12],LectBaseTexteBruit[12];
static float LectTauxAb6[2]; static int LectTauxMax,LectTauxNb,LectTauxPrem;
static float LectSeuilsAb6[2]; static int LectSeuilsMax,LectSeuilsNb,LectSeuilsPrem;
static OpiumColorState LectTauxCouleur,LectSeuilsCouleur;

#define LECT_INFONB 2
#define LECT_INFOLNGR 50
// static char LectTextRegen[LECT_INFOLNGR],LectTextReconv[LECT_INFOLNGR];
static char LectInfo[LECT_INFONB][LECT_INFOLNGR],LectAffichee[LECT_INFOLNGR];

static int64 LectArchDuree;
static char LectSrceTexte[40];

static TypeDonnee *LectTamponInit,*LectTamponPtr; static int LectTamponSize;
static int LectErreursNb;
#ifndef PAS_D_ERREUR_PERMISE
static TypeDonnee LectErreur1;
static int64 LectMarque1;
#endif
static char LectRtEnCours;

// static WndContextPtr LectAllume;

static Instrum iReglagesEtatRun;

static int   LectBolosTrmtCateg,LectBolosTrgrCateg;
static Panel pFichierLectLect,pFichierLectEcri;
static Info iLectPause,iLectIdent;

static int   LectBuffVal0,LectBuffValN;
static char  LectBuffOrigType,LectBuffDestType,LectBuffHexa;
static int   LectCompacteMaxFifo;
static char  LectBuffFichier[256];
static Panel pLectBuffPrint;
// static Panel pIntervAff;
// static char  LectEcoule[20];
// static float KgJ;
static Panel pLectEcoule,pLectPileMax;

static int64 LectBlocsMaxi;
static Panel pRunMode,pDureeSynchro,pReprises,pEtatTrigger,pLectRetardeur;
static TypeInstrumGlissiere LectGlissTempo = {150, 15, 1, 1, 8, 3};

// static Instrum cTaux,cAffEvt;
static Instrum cIntervAff;
static Panel pTauxGlobal;
static float LectTauxMin;

static char LectPaniqueAffichee;
static int64 LectTempoRun;
static int64 TempsTempoLect;
static clock_t CpuTempoRun,CpuTempoLect,CpuTotalLect;
static int CpuTauxOccupe;
static int LectNbDepilees,LectMaxDepilees; // int LectTauxPile;
static Instrum cCpuTauxOccupe,cLectTauxPile;

static float PerteIP,TempsMort,PerteEvts;
static Instrum cLectPerteIP,cTempsMort,cPerteEvts;

#ifdef DEBUG_ACCES
static int64 LectPciAdrs;
#endif

#ifdef DEBUG_DEPILE
#define MAXDEPILE 1024
static int LectDepileStockes;
static int64 LectSerie[MAXDEPILE + 1];
#endif
#ifdef DEBUG_FILL
static int64 LectFillMax,LectFillMax2;
static int64 LectFillNb;
#endif

// #define DEBUG_STATUS_MIG
#ifdef DEBUG_STATUS_MIG
#define STATUSLIST_LNGR 4096
int LectStatusList[STATUSLIST_LNGR];
int LectStatusNb;
#endif

#ifdef TESTE_MEMOIRE
static int LectPrevMem = 0;
static int LectCurrMem = 0;
#endif

static Menu mLectDemarrage,mLectArret,mLectSpectre,mLectSupplements;
static Menu mLectSuiviStart,mLectSuiviStop,mLectSuiviFocus,mLectSonorise;
static Menu mLectRegen;
static Panel pRepartXmit;
static Cadre bLectSpectre;
// static TypeFigDroite LectLigneVert1 = { 0, 0, WND_RAINURES, 0x0000, 0x0000, 0x0000 };
// static TypeFigDroite LectLigneVert2 = { 0, 0, WND_RAINURES, 0x0000, 0x0000, 0x0000 };
// static TypeFigDroite LectLigneHori1 = { 0, 0, WND_RAINURES, 0x0000, 0x0000, 0x0000 };
// static TypeFigDroite LectLigneHori2 = { 0, 0, WND_RAINURES, 0x0000, 0x0000, 0x0000 };
// static TypeFigDroite LectLigneHori3 = { 0, 0, WND_RAINURES, 0x0000, 0x0000, 0x0000 };
static TypeInstrumGlissiere LectGlissTemps = { 200, 15, 1, 1, 8, 3 };
// static TypeInstrumGlissiere LectGlissSeuil = { 0, 15, 0, 1, 6, 3};
static TypeInstrumGlissiere LectGlissSeuil = { 0, 15, 0, 1, 0, 0};
//- static TypeFigZone LectMonitCadreCntl,LectMonitCadreAnalyse,LectMonitCadreCapacite; // LectMonitCadreSignal
static TypeOscillo LectOscilloSignal;
static TypeMonitFenetre LectFenSignal,LectFenEvt,LectFenSpectre,LectFenBruit;
static float LectMonitAmplMin,LectMonitAmplMax,LectMonitRmsMin,LectMonitRmsMax;
static float LectSeuilRelatif;
static float LectSeuilReel;
static Instrum LectInstrumSeuil;
static Panel LectPanelSeuil;

static int LectCategRangFocus[CATG_MAX+2],LectCategRangSon[CATG_MAX+2];

// static char Lect1Val();
#ifdef ADRS_PCI_DISPO
// static void LectPadBloc(int voie, int lngr);
#endif
// static void LectPlancheAllume(Menu menu, char *commande, 
//	WndColorLevel r, WndColorLevel g, WndColorLevel b);

#ifdef OBSOLETE
static int LectArchLit();
static void LectArchFerme();
#endif

static void LectRazTampons(char log);
// static void LectDepileBoost();
#ifdef LECT_MODE_ABANDONNE
static int LectIdent(Menu menu);
#endif

float DetecteurConsigneCourante(int voie, int cmde);
void ReglageChargeUser(TypeDetecteur *detectr, TypeReglage *regl, int *bb_autorise, char *prefixe, char *cval);
int TemplatesAbsent(int voie, char log);
char RepartiteurRepereD3commune(char log);
char RepartiteursAttendStamp(char log);
void RepartIpContinue(TypeRepartiteur *repart, int duree, char log);
char RepartAjusteIdent();
int IpeEcritPixbus(TypeRepartiteur *repart, byte *texte, int *l);
void MonitTraceAjusteY(TypeMonitTrace *t, TypeTamponDonnees *donnees, Graph g);
int CalcSpectreAutoConstruit();
void CalcSpectreAutoPlanche(Cadre planche, int *xa, int *ya);
int CalcSpectreAutoAffiche();
int CalcSpectreAutoEfface();

void TrmtStop();

static long LectProchainTaux;
static long LectDateRegulBolo,LectDateRelance,LectIntervRelance;
static char AttenteFinRegen;

static ArgDesc LectCaractDesc[] = {
	{ "Trigger.demande",         DESC_BOOL        &Trigger.demande,     "Detection d'evenement a activer (sinon, c'est un stream)" },
	{ "Detec.demarrage.actif",   DESC_BOOL        &SettingsStartactive, "Procedures de demarrage des detecteurs, activees" },
	{ "Detec.entretien.actif",   DESC_BOOL        &SettingsDLUactive,   "Procedures d'entretien des detecteurs, activees" },
	{ "Sauvegarde.evt", DESC_LISTE ArchModeDetec, &ArchDetecMode,       "Mode de sauvegarde si evts (seul/coinc/voisins/tous/manip)" },
	{ "Sauvegarde.assoc",        DESC_BOOL        &ArchAssocMode,       "Sauvegarde egalement des detecteurs associes" },
	{ "Trmt.regul.evt",          DESC_BOOL        &TrmtRegulDemandee,   "Type de regulation du taux d'evenements (non/taux/intervalles)" },
	{ "Trmt.archivage",          DESC_BOOL        &Archive.demandee,     "Archivage des evenements ou des donnees brutes" },
	DESC_END
};

static ArgDesc LectEtatPolarDesc[] = {
	{ 0, DESC_KEY "normale/regeneration", &RegenEnCours, 0 },
	{ "jour",         DESC_STR(10)  RegenDebutJour, 0 },
	{ "heure",        DESC_STR(10)  RegenDebutHeure, 0 },
	{ "source",       DESC_KEY "absente/active", &LectSourceActive, 0 },
	DESC_END
};
static ArgDesc LectEtatRelaisDesc[] = {
	{ 0, DESC_KEY "ouverts/fermes", &RelaisFermes, 0 },
	{ "jour",   DESC_STR(10)  RelaisDebutJour, 0 },
	{ "heure",  DESC_STR(10)  RelaisDebutHeure, 0 },
	DESC_END
};
// static ArgStruct LectCondRunAS = { 0, 0, 0, LectEnvirDesc };
static ArgStruct LectEtatPolarAS = { 0, 0, 0, LectEtatPolarDesc };
static ArgStruct LectEtatRelaisAS = { 0, 0, 0, LectEtatRelaisDesc };
static ArgDesc LectEtatDesc[] = {
	{ "run",          DESC_NONE }, // DESC_STRUCT_DIM(1) &LectCondRunAS, 0 },
	{ "polarisation", DESC_STRUCT_DIM(1) &LectEtatPolarAS, 0 },
	{ "relais",       DESC_STRUCT_DIM(1) &LectEtatRelaisAS, 0 },
	DESC_END
};

#define MAXVARIATIONS VOIES_MAX
typedef struct {
	int voie,nbevts,duree;
	char descend;
	float min,max;
} TypeLectSeuilVariation;
static TypeLectSeuilVariation SeuilVariation[MAXVARIATIONS],SeuilVarEcrite;
static int SeuilVariationNb;
static int SeuilVariationDate;
static ArgDesc SeuilVariationDesc[] = {
	{ "voie",          DESC_LISTE VoieNom, &SeuilVarEcrite.voie,    0 },
	{ "event_found",   DESC_INT            &SeuilVarEcrite.nbevts,  0 },
	{ "timescale(mn)", DESC_INT            &SeuilVarEcrite.duree,   0 },
	{ "direction",     DESC_KEY "up/down", &SeuilVarEcrite.descend, 0 },
	{ "min_if_pos",    DESC_FLOAT          &SeuilVarEcrite.min,     0 },
	{ "max_if_neg",    DESC_FLOAT          &SeuilVarEcrite.max,     0 },
	DESC_END
};
static ArgStruct SeuilVariationAS  = { (void *)SeuilVariation, (void *)&SeuilVarEcrite, sizeof(TypeLectSeuilVariation), SeuilVariationDesc };
static ArgDesc SeuilVarListeDesc[] = {
	{ "time(s)", DESC_INT                                         &SeuilVariationDate, 0 },
	{ "changes", DESC_STRUCT_SIZE(SeuilVariationNb,MAXVARIATIONS) &SeuilVariationAS,   0 },
	DESC_END
};

// #include <sys/resource.h>
#include <time.h>
static struct rusage StatsSystemeDebut,StatsSystemeFin;
static struct timeval LectDateRunDebut,LectDateRunFin;
static clock_t LectCpuRunDebut,LectCpuRunFin;

char CaliVidePile(TypeRepartiteur *repart, char *pas_tous_secs, short *pleins, char log);
char IpeVidePile(TypeRepartiteur *repart, char *pas_tous_secs, short *pleins, char log);

void OscilloConnecte(Oscillo oscillo);

static void LectDecompteVoies(char log);

void LectRegenLance(char avec_source);
void LectRegenStoppe(char avec_dialogue);
static int LectRunProgramme(char log);
static void LectRetardRefresh();
static char LectDelaiEcoule();
static void LectItRecue();
static int LectRepartStoppe(Menu menu);

void LectExecTermine();
void LectAcqElemTermine();
void LectAcqStdTermine();

void ArchiveSeuils(int secs);
// void MonitFenNettoie(void *tampon);
static void LectCompacteAffiche(int numero);
static int LectCompacteCliquePlanche(Cadre planche, WndAction *e, void *arg);

static int LectConsignesDecode(Cadre cdr, void *arg);
static int LectConsigneAjoute(LECT_CONS_TYPE type, int bolo, TypeReglage *regl);
static void LectConsigneCharge(TypeReglage *regl, char autorise);
static int LectConsigneTouche(Panel panel, int item, void *arg);
static int LectConsigneModifie(Panel panel, int item, void *arg);
static int LectConsignesBoutonLoad(Panel p, void *ptr);
static int LectConsignesBoutonSave(Panel p, void *ptr);

#pragma mark ----------- Spectres ----------
/* ========================================================================== */
int LectSpectreTampons() {
	int bolo,cap,phys,voie; int nbfreq,dim,n;

	// printf("%s/ Preparation du calcul des spectres\n",DateHeure());
	for(bolo=0; bolo<BoloNb; bolo++) if(CalcAutoBolo[bolo]) {
		for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
			voie = Bolo[bolo].captr[cap].voie;
		#ifdef SPECTRES_AUTO_ALLOC_DYN
			if(!VoieTampon[voie].lue) continue; // si active, consequences dans LectSpectresAutoMesure et LectAcqStd()
		#endif
			CalcSpectreAutoNb[voie] = 0.0;
			phys = ModeleVoie[VoieManip[voie].type].physique;
			if(!(dim = CalcSpectreAuto[phys].dim)) continue;
			nbfreq = CalcSpectreAuto[phys].nbfreq = (CalcSpectreAuto[phys].dim / 2) + 1;
			if(CalcAutoSpectreDim[voie] != nbfreq) {
				if(CalcAutoCarreSomme[voie]) { free(CalcAutoCarreSomme[voie]); CalcAutoCarreSomme[voie] = 0; }
				if(CalcAutoSpectreMoyenne[voie]) { free(CalcAutoSpectreMoyenne[voie]); CalcAutoSpectreMoyenne[voie] = 0 ; }
				CalcAutoSpectreDim[voie] = 0;
			}
			n = nbfreq * sizeof(float);
			if(!CalcAutoCarreSomme[voie]) {
				CalcAutoCarreSomme[voie] = (float *)malloc(n);
				if(!CalcAutoCarreSomme[voie]) {
					OpiumFail("Manque de place memoire pour le spectre**2 somme (%d octets)",n);
					Acquis[AcquisLocale].etat.active = 0;
					return(0);
				}
			}
			if(!CalcAutoSpectreMoyenne[voie]) {
				CalcAutoSpectreMoyenne[voie] = (float *)malloc(n);
				if(!CalcAutoSpectreMoyenne[voie]) {
					OpiumFail("Manque de place memoire pour le spectre moyenne (%d octets)",n);
					Acquis[AcquisLocale].etat.active = 0;
					return(0);
				}
				for(n=0; n<nbfreq; n++) CalcAutoSpectreMoyenne[voie][n]= 1.0;
				CalcAutoSpectreDim[voie] = nbfreq;
			}
			if(CalcAutoCarreSomme[voie]) for(n=0; n<nbfreq; n++) CalcAutoCarreSomme[voie][n]= 0.0;
			// printf("%s/ . Voie %s prete\n",DateHeure(),VoieManip[voie].nom);
		}
	};
	CalcGlissantEpsilon = 1.0 / (float)CalcSpectresMax;
	printf("%s/ Spectres automatiques prets\n",DateHeure());
	
	return(1);
}

#pragma mark ----------- Utilitaires ----------
/* ========================================================================== */
void LectDeclencheErreur(const char *nom, char *fic, int ligne, int num, TypeADU code) {
	if(nom) { strncpy(LectErreur.nom,nom,ERREUR_FCTN); LectErreur.nom[ERREUR_FCTN-1] = '\0'; }
	else strcpy(LectErreur.nom,"une routine anonyme");
	if(fic) { strncpy(LectErreur.fic,fic,ERREUR_FILE); LectErreur.fic[ERREUR_FILE-1] = '\0'; }
	else strcpy(LectErreur.fic,"un fichier inconnu");
	LectErreur.ligne = ligne;
	LectErreur.num = num;
	LectErreur.code = code;
	if(LectErreur.code) Acquis[AcquisLocale].etat.active = 0;
}
/* ========================================================================== */
static void LectExplique(TypeADU rc) {
	switch(rc) {
		case 0:            strcpy(ArchExplics,L_("Fin normale (stop utilisateur)")); break;
		case LECT_COLL:    strcpy(ArchExplics,L_("Collision")); break;
		case LECT_ALRM_0:  strcpy(ArchExplics,L_("Alarme sur le numeriseur de rang 0")); break;
		case LECT_ALRM_1:  strcpy(ArchExplics,L_("Alarme sur le numeriseur de rang 1")); break;
		case LECT_ALRM_2:  strcpy(ArchExplics,L_("Alarme sur le numeriseur de rang 2")); break;
		case LECT_ALRM_3:  strcpy(ArchExplics,L_("Alarme sur le numeriseur de rang 3")); break;
		case LECT_ALRM_4:  strcpy(ArchExplics,L_("Alarme sur le numeriseur de rang 4")); break;
		case LECT_ALRM_5:  strcpy(ArchExplics,L_("Alarme sur le numeriseur de rang 5")); break;
		case LECT_ALRM_6:  strcpy(ArchExplics,L_("Alarme sur le numeriseur de rang 6")); break;
		case LECT_ALRM_7:  strcpy(ArchExplics,L_("Alarme sur le numeriseur de rang 7")); break;
		case LECT_ALRM_8:  strcpy(ArchExplics,L_("Alarme sur le numeriseur de rang 8")); break;
		case LECT_ALRM_9:  strcpy(ArchExplics,L_("Alarme sur le numeriseur de rang 9")); break;
		case CLUZEL_MARQUE_NEANT:     strcpy(ArchExplics,L_("Debut de bloc perdu")); break;
		case CLUZEL_MARQUE_ECHANT:    strcpy(ArchExplics,L_("Debut de bloc inattendu")); break;
		case CLUZEL_MARQUE_MODULATION:  strcpy(ArchExplics,L_("Debut de synchro D2 inattendue")); break;
		case LECT_FULL:    strcpy(ArchExplics,L_("Buffers de donnees pleins")); break;
		case LECT_NFND:    strcpy(ArchExplics,L_("1ere synchronisation pas trouvee")); break;
		case LECT_EOF:
			if(LectErreurFichier) sprintf(ArchExplics,L_("Erreur de relecture de type %d (%s)"),LectErreurFichier,strerror(LectErreurFichier));
			else strcpy(ArchExplics,L_("Fin du fichier des donnees"));
			break;
		case LECT_UNKN:    strcpy(ArchExplics,L_("Type de donnee indetermine (0)")); break;
		case LECT_SYNC:    strcpy(ArchExplics,L_("Synchro introuvable dans la FIFO")); break;
		case LECT_EMPTY:   strcpy(ArchExplics,L_("FIFO deseperement vide")); break;
		case LECT_TOOMUCH: strcpy(ArchExplics,L_("FIFO deseperement pleine")); break;
		case LECT_INVALID: strcpy(ArchExplics,L_("Donnee incorrecte")); break;
		case LECT_INCOHER: strcpy(ArchExplics,L_("Incoherence FillNb/NonNul, ou No trame IP incorrect")); break;
		case LECT_UNEXPEC: strcpy(ArchExplics,L_("Donnee de type inattendu")); break;
		case LECT_NODATA:  strcpy(ArchExplics,L_("Pas de donnee enregistree")); break;
		case LECT_OVERFLO: strcpy(ArchExplics,L_("CPU surcharge")); break;
		case LECT_EVTS:    strcpy(ArchExplics,L_("Taux d'evenements en exces")); break;
		case LECT_ARCH:    strcpy(ArchExplics,L_("Archivage impossible")); break;
		case LECT_ARRETE:  strcpy(ArchExplics,L_("Repartiteur arrete")); break;
		case LECT_PREVU:   strcpy(ArchExplics,L_("Arret de la lecture programme d'avance")); break;
		default:
			if(DATAERREUR(rc)) {
				unless((rc & LECT_DESYNC)) strcpy(ArchExplics,L_("Collision h/w"));
				else unless((rc & LECT_PBHW)) strcpy(ArchExplics,L_("Desynchronisation h/w"));
				else strcpy(ArchExplics,L_("Collision + desynchro h/w"));
			} else sprintf(ArchExplics,L_("Cause inconnue codee %04X"),rc);
	}
	// { char code[8]; sprintf(code," [%04X]",rc); strcat(ArchExplics,code); }
}
/* ========================================================================== */
static void LectRecopieExplics(char *texte, int i) {
	int l;
	
	l = strlen(texte) + 1;
	LectListeExplics[i] = Malloc(l,char);
	if(LectListeExplics[i]) strcpy(LectListeExplics[i],texte);
	// printf("Erreur[%2d][%2d]: '%s'",i,l,LectListeExplics[i]);
	// printf(" (Erreur[%2d][%2d]: '%s' @%08X)\n",0,strlen(LectListeExplics[0]),LectListeExplics[0],(hexa)(LectListeExplics[0]));
}
/* ========================================================================== */
static void LectListeErreursConnues() {
	int i,nb;

	nb = 0; while(LectCodeConnu[nb++])
		;
	i = 0;
	// LectListeExplics = 0;
	if(nb) {
		// LectListeExplics = Malloc(nb+1,char*);
		if(nb > LECT_MAXCODES) nb = LECT_MAXCODES;
		// printf("%d message%s d'erreur genere%s\n",Accord2s(nb+1));
		pour_tout(i,nb-1) {
			LectExplique(LectCodeConnu[i]);
			LectRecopieExplics(ArchExplics,i);
		}
		LectRecopieExplics(L_("(autre code)"),i++);
	}
	LectRecopieExplics("\0",i++);
	// pour_tout(i,nb+1) printf("Erreur[%2d]: '%s'\n",i,LectListeExplics[i]);
}
/* ========================================================================== */
static TypeADU LectMessageArret(const char *orig, char *fic, int ligne, int num, TypeADU rc) {
	LectExplique(rc);
	if(num) {
		printf("%s/ ! Declenchement du detecteur d'erreur #%d dans ",DateHeure(),num);
		if(orig) {
			unless(strcmp(orig,"xxx")) printf("%s",LectDetectNom[LectDetectRtneNum[num]]);
			else {
				printf("%s\n            appele dans %s:%d",orig,fic,ligne);
				if(strcmp(orig,LectDetectNom[LectDetectRtneNum[num]])) printf(" (declare dans %s)",LectDetectNom[LectDetectRtneNum[num]]);
			}
		} else printf("%s\n            appele dans %s:%d",LectErreur.nom,LectErreur.fic,LectErreur.ligne);
		printf("\n");
		if(LectRelancePrevue) 
			printf("%s!!! Erreur %04X (%s), relance de %s #%d\n",DateHeure(),LectErreur.code,ArchExplics,LectSrceTexte,LectSession);
		else printf("%s!!! Erreur %04X (%s), %s est stoppee\n",DateHeure(),LectErreur.code,ArchExplics,LectSrceTexte);
	} else if(LectCntl.LectMode == LECT_COMP) return(rc); // printf("%s/ Fin de la procedure\n",DateHeure());
	else printf("%s/ Fin de %s demandee par l'utilisateur\n",DateHeure(),LectSrceTexte);
	return(rc);
}
/* ========================================================================== */
static int LectBranche(char en_service) {
//	OpiumBranche(cModeLecture->cdr,en_service && ExpertConnecte);
//	OpiumBranche(cTrigger->cdr,en_service && ExpertConnecte);
//	OpiumBranche(cArchive->cdr,en_service);
//	OpiumBranche(pRunMode->cdr,en_service);
	if(pLectMode) OpiumBranche(pLectMode->cdr,en_service);
	return(0);
}
/* ========================================================================== */
static int LectAutorise(Cadre cdr, void *arg) {
	return(LectBranche(1));
}
/* ========================================================================== */
static void LectDumpFifoFromRep(int rep, int max) {
	int j,k,v,val; int lim,nb;
	TypeRepModele *modele_rep;

	printf("FIFO[%d - %d]:",Repart[rep].bottom,Repart[rep].top);
	modele_rep = &(ModeleRep[Repart[rep].type]);
	lim = (Repart[rep].top < max)? Repart[rep].top: max;
	nb = Repart[rep].nbdonnees; if(nb <= 0) nb = 8;
	for(j=Repart[rep].bottom,v=0; j<lim; j++,v++) {
		unless((v % nb)) {
			if(modele_rep->status && v) {
				if(Repart[rep].famille == FAMILLE_CLUZEL) {
					val = Repart[rep].fifo32[j]; printf(" %08X",val);
				} else {
					if((j & 1)) k = j - 1; else k = j + 1;
					if(modele_rep->bits32) {
						val = Repart[rep].data32[k]; printf(" %08X",val);
					} else {
						val = Repart[rep].data16[k]; printf(" %04X",val & 0xFFFF);
					}
				}
				if(CLUZEL_Empty(val)) break;
				j++;
			}
			printf("\n%4d:",j);
		}
		if(Repart[rep].famille == FAMILLE_CLUZEL) {
			val = Repart[rep].fifo32[j]; printf(" %08X",val);
		} else {
			if((j & 1)) k = j - 1; else k = j + 1;
			val = Repart[rep].data16[k]; printf(" %04X",val & 0xFFFF);
		}
		if((Repart[rep].famille == FAMILLE_CLUZEL) && CLUZEL_Empty(val)) break;
	}
	printf("\n");
}
/* ========================================================================== */
int LectDumpFifo() {
	int max,rep;

	max = 1024;
	if(OpiumReadInt("Sur quelle profondeur",&max) == PNL_CANCEL) return(0);
	rep = 0;
	if(RepartNb > 1) {
		if(OpiumReadList("Du repartiteur",RepartListe,&rep,8) == PNL_CANCEL) return(0);
	}
	LectDumpFifoFromRep(rep,max);
	return(0);
}
/* ========================================================================== */
void LectUdpStatus(int64 *recues, int64 *err, int64 *ovr) {
	char template[MAXFILENAME],*listing;
	char commande[MAXFILENAME]; char *hdr;
	char ligne[256],*r; int l; int64 val;
	FILE *f;

	*recues = *err = *ovr = 0;
	strcpy(template,"Listing_XXXX");
	listing = mktemp(template);
	if(listing) {
		if(listing[0] == '\0') return;
		RepertoireCreeRacine(listing);
	#ifdef Darwin
		sprintf(commande,"netstat -sp udp >%s",listing); hdr = "udp:";
	#endif
	#ifdef Linux
		sprintf(commande,"netstat -sp >%s",listing); hdr = "Udp:";
	#endif
		system(commande);
		if((f = fopen(listing,"r"))) {
			while(LigneSuivante(ligne,256,f)) {
				if(!strncmp(ligne,hdr,4)) continue;
				else {
					r = ligne; l = strlen(ligne) - 1; if(ligne[l] == '\n') ligne[l] = '\0';
					sscanf(ItemJusquA(' ',&r),"%lld",&val);
				#ifdef Darwin
					if(!strcmp(r,"datagrams received")) *recues = val;
					else if(!strcmp(r,"with incomplete header")) *err += val;
					else if(!strcmp(r,"with bad data length field")) *err += val;
					else if(!strcmp(r,"with bad checksum")) *err += val;
					else if(!strcmp(r,"dropped due to full socket buffers")) *ovr = val;
				#endif
				#ifdef Linux
					if(!strcmp(r,"packets received")) *recues = val;
					else if(!strcmp(r,"packet receive errors")) *err += val;
					else if(!strcmp(r,"receive buffer errors")) *ovr = val;
					else if(!strcmp(r,"packets to unknown port received")) *err += val;
				#endif
				}
			}
			fclose(f);
		}
		// printf("(%s) Resultat de netstat dans '%s'\n",__func__,listing);
		remove(listing);
	}
}

#pragma mark ------ Processus de demarrage ------
/* ========================================================================== */
static int LectConfigChoix(Menu menu, MenuItem item) {
	if(OpiumExec(pLectConfigChoix->cdr) == PNL_OK) SambaConfigChange(0,0,0);
	return(0);
}
/* ========================================================================== */
char LectTrmtUtilise(int voie, int classe) {
    /*	if(LectModeSpectresAuto) return(CalcSpectreAutoAuVol? VoieManip[voie].def.trmt[TRMT_AU_VOL].type: NEANT);
     else if(OscilloAffiche(VoieInfo[voie].oscillo))
     return((VoieInfo[voie].oscillo)->demodule);
     else return(VoieManip[voie].def.trmt[TRMT_AU_VOL].type); */
    
    if(classe == TRMT_AU_VOL) {
        if(LectModeSpectresAuto) return(CalcSpectreAutoAuVol? VoieManip[voie].def.trmt[TRMT_AU_VOL].type: NEANT);
        else if(OscilloAffiche(VoieInfo[voie].oscillo)) return((VoieInfo[voie].oscillo)->demodule);
        else return(VoieManip[voie].def.trmt[TRMT_AU_VOL].type);
    } else if(VoieTampon[voie].trig.special) return(NEANT);
    else return(VoieManip[voie].def.trmt[classe].type);
}
/* ========================================================================== */
char LectFixePostTrmt() {
	char mode_evts,mode_normal;

	mode_normal = (LectRunNormal && !LectModeStatus);
	mode_evts = (Trigger.demande && !MiniStream && !LectModeSpectresAuto);
	Trigger.enservice = (mode_normal && mode_evts);
	/* pas faux, mais a voir plus tard: on peut avoir les deux modes en meme temps
	ArchSauve[EVTS] = (mode_normal && mode_evts && Archive.demandee);
	ArchSauve[STRM] = (mode_normal && !mode_evts && Archive.demandee);
	Archive.enservice = (ArchSauve[EVTS] || ArchSauve[STRM]); */
	Archive.enservice = (mode_normal && Archive.demandee);  // i.e. (!LectModeStatus && (mode == LECT_DONNEES) && !LectMaintenance && Archive.demandee)
	return(mode_evts);
}
/* ========================================================================== */
static char LectFixeMode(LECT_MODE mode, char init) {
	int k,l,rep,voie,bolo,bb,sat; char mode_evts; TypeRepartiteur *repart;
	
	if(Acquis[AcquisLocale].etat.active) {
		OpiumWarn("%s est deja lancee, en %s",LectSrceTexte,LectureSuspendue? "pause": "cours");
		return(0);
	}
	k = 0;
	switch(mode) {
		case LECT_DONNEES: strcpy(LectSrceTexte,"la lecture"); break;
		case LECT_IDENT:   strcpy(LectSrceTexte,"l'identification"); break;
		case LECT_COMP:    strcpy(LectSrceTexte,"la compensation"); break;
		default: break;
	}
	if(LectureLog) {
		//#ifdef ADRS_PCI_DISPO
		//	printf("==================== Demarrage de %s (PCI @%08X) ====================\n",LectSrceTexte,(hexa)PCIadrs[PCInum]);
		//#endif
		if(LectSurFichier) {
			printf("============= Mise en route de la lecture du fichier %s: %s =============\n",SrceTitre[SrceType],SrceName);
		} else {
			k = 86;
			printf("\n\n");
			printf(" ====================================================================================\n");
			printf("|                  Mise en route de %-32s                 |\n",LectSrceTexte);
			printf("|____________________________________________________________________________________|\n");
		}
	}

	LectBranche(0); /* remettre a 1 si return(0) */
	if(init) {
		/* Etat des lieux */
		if(LectureLog > 1) {
			l = printf("| Evaluation des modes de lecture selon fenetres de commandes ouvertes"); SambaCompleteLigne(k,l);
		}
		LectMaintenance = 0; TrmtRegulActive = 0;
		for(voie=0; voie<VoiesNb; voie++) { bolo = VoieManip[voie].det; Bolo[bolo].seuils_changes = 0; }
		for(rep=0; rep<RepartNb; rep++) Repart[rep].actif = 0;

		/* spectres de bruit? */
		if(LectModeSpectresAuto) {
			for(voie=0; voie<VoiesNb; voie++) if(OpiumDisplayed(VoieInfo[voie].planche)) OpiumClear(VoieInfo[voie].planche);
			// for(bb=0; bb<NumeriseurNb; bb++) if(OpiumDisplayed(Numeriseur[bb].interface.planche)) OpiumClear(Numeriseur[bb].interface.planche);
			// for(bb=0; bb<NUMER_ANONM_MAX; bb++) if(OpiumDisplayed(NumeriseurAnonyme[bb].interface.planche)) OpiumClear(NumeriseurAnonyme[bb].interface.planche);
			if(OpiumDisplayed(cRepartAcq)) OpiumClear(cRepartAcq);
			ReglagesEtatDemande = NumeriseurEtatDemande = 0;
			ReglVi.actif = 0;
			Archive.demandee = 0;
		}

		/* maintenance detecteurs? */
		LectReglagesOuverts = 0;
		for(voie=0; voie<VoiesNb; voie++) if(OpiumDisplayed(VoieInfo[voie].planche)) { LectReglagesOuverts = 1; break; }
		
		/* maintenance numeriseurs? */
		LectBBaffiches = 0;
		if(!LectModeSpectresAuto) {
			for(bb=0; bb<NumeriseurNb; bb++) if(OpiumDisplayed(Numeriseur[bb].interface.planche)) {
				if((repart = Numeriseur[bb].repart)) repart->actif = 1;
				LectBBaffiches = 1;
			}
			unless(LectBBaffiches) {
				for(bb=0; bb<NUMER_ANONM_MAX; bb++) if(OpiumDisplayed(NumeriseurAnonyme[bb].interface.planche)) {
					if((repart = NumeriseurAnonyme[bb].repart)) repart->actif = 1;
					LectBBaffiches = 1;
				}
			}
		}
		
		/* maintenance repartiteurs? */
		LectRepAffiches = 0; if(!LectModeSpectresAuto && OpiumDisplayed(cRepartAcq)) { LectRepAffiches = 1; if(RepartConnecte) RepartConnecte->actif = 1; }
		
		/* etat numeriseurs ou detecteurs? */
		if(ReglagesEtatDemande || NumeriseurEtatDemande) {
			for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) Repart[rep].actif = 1; // .actif normalement determine dans RepartiteursSelectionneVoiesLues
		}
		
		/* indicateurs de modes */
		if(LectReglagesOuverts || LectBBaffiches || LectRepAffiches || ReglVi.actif) { LectMaintenance = 1; MonitFftActif = 0; }
		else MonitFftActif = MonitFftDemande;
		if(!MonitFftActif && OpiumAlEcran(gMonitFftAuVol)) OpiumClear(gMonitFftAuVol->cdr);
		LectAttendIdent = (mode == LECT_IDENT);
		LectRunNormal = ((mode == LECT_DONNEES) && !LectMaintenance);
		LectStrictStd = (LectRunNormal && !LectModeSpectresAuto && !ReglagesEtatDemande && !NumeriseurEtatDemande);
		LectAcqCollective = (LectRunNormal && (LectAcqLanceur == LECT_FROM_STD) && (PartitLocale >= 0) && (SettingsPartition == SAMBA_PARTIT_STD));
		LectStampSpecifique = 0;
		
		LectDemarreBolo = LectEntretien = 0;
		if(LectRunNormal) {
			// int bolo;
			for(rep=0; rep<RepartNb; rep++) if((Repart[rep].categ == DONNEES_STAMP) && Repart[rep].valide) { Repart[rep].actif = 1; LectStampSpecifique = 1; }
			// if(SettingsStartactive) { for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire && (Bolo[bolo].start.bloc >= 0)) { LectDemarreBolo = 1; break; } }
			// if(SettingsDLUactive) { for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire && (Bolo[bolo].regul.bloc >= 0)) { LectEntretien = 1; break; } }
			LectDemarreBolo = (BolosAdemarrer && SettingsStartactive);
			LectEntretien = (BolosAentretenir && SettingsDLUactive && SettingsDLUdetec);
			/* if(LectEntretien && EtatRegen) {
			 OpiumWarn("Source de regeneration active: MAINTENANCE DE DETECTEUR INHIBEE");
			 } */
		}
		LectEntretienEnCours = 0;
		LectCntl.LectMode = mode;
		for(sat=0; sat<AcquisNb; sat++) strcpy(Acquis[AcquisLocale].etat.nom_run,"neant");
		Tzero[1] = 1.0 / Echantillonnage;
	}
	mode_evts = LectFixePostTrmt();
	if(LectureLog && !LectSurFichier) {
		l = printf("| Session              | #%d",LectSession);                      SambaCompleteLigne(k,l);
		l = printf("| Run normal           | %s",LectRunNormal? "oui": "non");       SambaCompleteLigne(k,l);
		l = printf("| Trigger demande      | %s",Trigger.demande? "oui": "non");     SambaCompleteLigne(k,l);
		l = printf("| Suivi des evenements | %s",LectStrictStd? "oui": "non");       SambaCompleteLigne(k,l);
		l = printf("| Mini stream          | %s",MiniStream? "oui": "non");          SambaCompleteLigne(k,l);
		l = printf("| Mode evenements      | %s",mode_evts? "oui": "non");           SambaCompleteLigne(k,l);
		l = printf("| Trigger en service   | %s",Trigger.enservice? "oui": "non");   SambaCompleteLigne(k,l);
		l = printf("| Sauve donnees        | %s",Archive.enservice? "oui": "non");   SambaCompleteLigne(k,l);
		l = printf("| Demarrage detecteurs | %s",SettingsStartactive? "oui": "non"); SambaCompleteLigne(k,l);
		l = printf("| Entretien detecteurs | %s",SettingsDLUactive? "oui": "non");   SambaCompleteLigne(k,l);
			printf("|______________________|_____________________________________________________________|\n");
	}
	AutomVerifieConditions("",k);
	if(LectureLog && !LectSurFichier) 
		// printf("|====================================================================================|\n\n");
		printf("|____________________________________________________________________________________|\n\n");
	// sprintf(Acquis[AcquisLocale].etat.nom_run,"%s%c%03d",DateCourante,Acquis[AcquisLocale].runs[0],NumeroBanc++);
	// printf("* Nouveau run: '%s'\n",Acquis[AcquisLocale].etat.nom_run);

	return(1);
}
/* ========================================================================== */
static int LectChangeDureePartitions(Panel p, int k) {
	int64 nbsecs;
	
	nbsecs =  (int64)LectDureeMaxi;
	LectBlocsMaxi = (int64)((float)(nbsecs * 60000) * Echantillonnage);
	nbsecs =  (int64)LectDureeTranche;	
	ArchTrancheTaille = (int64)((float)(nbsecs * 60000) * Echantillonnage);
	
	return(0);
}
/* ========================================================================== */
static void LectSelecteVoies() {
	/* Liste des voies a lire et activation des repartiteurs */
	int bolo,cap,voie,v,phys,bb,fmt; Oscillo oscillo; // short num_hdr[ARCH_TYPEDATA];
	int i,j,l;
	TypeMonitFenetre *f; TypeMonitTrace *t;
	char log;

	log = 1; // (LectureLog > 1);
	if(log) {
		// printf(". Liste des voies a traiter:\n");
		printf(" ____________________________________________________________________________________\n");
		printf("|                             Liste des voies a traiter                              |\n");
		printf("|____________________________________________________________________________________|\n");
	}

	for(bb=0; bb<NumeriseurNb; bb++) { Numeriseur[bb].a_lire = 0; Numeriseur[bb].enreg_txt[0] = '\0'; }
	/* version 1: on ne sauve le status que des numeriseurs dont on lit une voie de detecteur
	 for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
		int cap;
		bolo = VoieManip[voie].det; cap = VoieManip[voie].cap;
		if((bolo >= 0) && (cap > 0)) bb = Bolo[bolo].captr[cap].bb.num;
		if(bb >= 0) Numeriseur[bb].a_lire = 1;
	 }
	 for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].a_lire && !Numeriseur[bb].local)
		printf("! Le numeriseur %s doit lire au moins une voie, mais son detecteur n'est pas accessible\n",Numeriseur[bb].nom);
	 } */
	/* version 2: on sauve le status de toutes les entrees de tous les repartiteurs (voir dans ArchiveDefini) */
	
	if(LectSurFichier) {
		for(voie=0; voie<VoiesNb; voie++) {
			for(fmt=0; fmt<ARCH_TYPEDATA; fmt++) VoieManip[voie].num_hdr[fmt] = -1;
			if(log && VoieTampon[voie].lue) {
				l = printf("|  + %s",VoieManip[voie].nom); SambaCompleteLigne(86,l);
			}
		}
		LectDecompteVoies(log);
		if(log) printf("|____________________________________________________________________________________|\n");
		return;
	}

	// ReglageVerifieStatut();
	if(LectMaintenance) DetecteursDecompte(); /* pour Bolo[].a_lire donc VoieTampon[].lue */
	else DetecteursVoisinage(0);              /* et les voisins aussi, du reste ..        */
	for(voie=0; voie<VoiesNb; voie++) {
		VoieTampon[voie].lue = 0; VoieTampon[voie].sauvee = NEANT; VoieTampon[voie].rang_sample = -1;
        if(VoieTampon[voie].trig.special) free(VoieTampon[voie].trig.trgr);
        VoieTampon[voie].trig.special = 0; VoieTampon[voie].trig.trgr = 0;
		for(fmt=0; fmt<ARCH_TYPEDATA; fmt++) VoieManip[voie].num_hdr[fmt] = -1;
	}

	if(LectReglagesOuverts) {
		for(voie=0; voie<VoiesNb; voie++) if(/* VoieInfo[voie].planche &&  */OpiumDisplayed(VoieInfo[voie].planche)) {
			VoieTampon[voie].lue = 1;
            if(VoieInfo[voie].complete == 2) {
                if((VoieTampon[voie].trig.trgr = (TypeTrigger *)malloc(sizeof(TypeTrigger)))) {
                    Oscillo oscillo;

					Trigger.enservice = 1;
					VoieTampon[voie].trig.trgr->origine = TRGR_INTERNE;
					VoieTampon[voie].trig.trgr->type = TRGR_SEUIL;
                    VoieTampon[voie].trig.trgr->minampl = VoieTampon[voie].trig.trgr->maxampl = VoieInfo[voie].trigger;
                    VoieTampon[voie].trig.trgr->sens = (VoieInfo[voie].trigger >= VoieInfo[voie].moyenne)? 2: 0;
					VoieTampon[voie].trig.special = 1;
                    oscillo = VoieInfo[voie].oscillo;
                    if(OpiumAlEcran((oscillo->iActif))) InstrumRefreshVar(oscillo->iActif);
                }
            }
			if(log) { l = printf("|  + %s (reglage detecteur)",VoieManip[voie].nom); SambaCompleteLigne(86,l); }
		#ifdef REGLAGE_TOUT_LE_BOLO
			if((bolo = VoieManip[voie].det) >= 0) {
				TypeRepartiteur *repart; int cap;
				if((bb = Bolo[bolo].captr[VoieManip[voie].cap].bb.num) >= 0) {
					repart = Numeriseur[bb].repart;
					for(cap=0; cap<Bolo[bolo].nbcapt; cap++) if(cap != VoieManip[voie].cap) {
						if((bb = Bolo[bolo].captr[cap].bb.num) >= 0) {
							if(repart = Numeriseur[bb].repart) {
								if((v = Bolo[bolo].captr[cap].voie) >= 0) {
									if(!VoieManip[v].pseudo) VoieTampon[v].lue = 1;
									if(log) { l = printf("|  + %s (reglage detecteur)",VoieManip[v].nom); SambaCompleteLigne(86,l); }
								}
							}
						}
					}
				}
			}
		#endif
			oscillo = VoieInfo[voie].oscillo;
			if(OscilloAffiche(oscillo)) {
				int v; for(v=0; v<oscillo->nbvoies; v++) if(oscillo->voie[v] != voie) {
					VoieTampon[oscillo->voie[v]].lue = 1;
					if(log) { l = printf("|  + %s (reglage detecteur)",VoieManip[oscillo->voie[v]].nom); SambaCompleteLigne(86,l); }
				}
			}
		}
	}
	if(ReglVi.actif) for(v=0; v<ReglVi.nb_voies; v++) {
		VoieTampon[ReglVi.voie[v].num].lue = 1;
		if(log) { l = printf("|  + %s (calcul des courbes V(I))",VoieManip[ReglVi.voie[v].num].nom); SambaCompleteLigne(86,l); }
	}
	if(LectModeSpectresAuto) {
		for(bolo=0; bolo<BoloNb; bolo++) if(CalcAutoBolo[bolo]) {
			for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
				voie = Bolo[bolo].captr[cap].voie;
				VoieTampon[voie].lue = (VoieManip[voie].def.trmt[TRMT_AU_VOL].type != TRMT_INVALID);
				if(!VoieTampon[voie].lue) continue;
				phys = ModeleVoie[VoieManip[voie].type].physique;
				if(CalcAutoPhys[phys] && (CalcSpectreAuto[phys].dim > 0)) {
					if((VoieTampon[voie].trig.trgr = (TypeTrigger *)malloc(sizeof(TypeTrigger)))) {
						Trigger.enservice = 0;
						VoieTampon[voie].trig.trgr->origine = TRGR_INTERNE;
						VoieTampon[voie].trig.trgr->type = NEANT;
						VoieTampon[voie].trig.special = 1;
					}
				} else VoieTampon[voie].lue = 0;
			}
		}
	}
	if(LectMaintenance || LectModeSpectresAuto) {
		for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if(f->affiche == MONIT_ALWS) {
			for(j=0, t=f->trace; j<f->nb; j++,t++) {
				voie = t->voie; if(VoieTampon[voie].lue) continue;
				bolo = VoieManip[voie].det; if(bolo < 0) continue;
				if(Bolo[bolo].a_lire && (VoieManip[voie].def.trmt[TRMT_AU_VOL].type != TRMT_INVALID)) {
					VoieTampon[voie].lue = 1;
					if(log) { l = printf("|  + %s (toujours affichee)",VoieManip[voie].nom); SambaCompleteLigne(86,l); }
				}
			}
		}
	}
	LectDureeReset = 0;
	if(LectModeSpectresBanc > 0) BancSptrSelecteVoies(log);
	else if(LectRunNormal) {
		Trigger.actif = Trigger.enservice;
		for(voie=0; voie<VoiesNb; voie++) {
			bolo = VoieManip[voie].det;
			if(bolo < 0) {
				if(ReglagesEtatDemande) VoieTampon[voie].lue = 1;
				else if(!LectModeSpectresAuto) VoieTampon[voie].lue = (VoieManip[voie].def.trmt[TRMT_AU_VOL].type != TRMT_INVALID);
			} else if(ReglagesEtatDemande) VoieTampon[voie].lue = Bolo[bolo].a_lire;
			else if(LectModeSpectresAuto) {
				phys = ModeleVoie[VoieManip[voie].type].physique;
				VoieTampon[voie].lue = CalcAutoBolo[bolo] && (CalcSpectreAuto[phys].dim > 0);
				if(VoieManip[voie].pseudo) VoieTampon[voie].lue &= CalcSpectreAutoPseudo;
				else VoieTampon[voie].lue &= ((VoieManip[voie].def.trmt[TRMT_AU_VOL].type != TRMT_INVALID) || CalcSpectreAutoInval);
				if(VoieTampon[voie].lue) {
					if(Bolo[bolo].reset) {
						if(!LectDureeReset) LectDureeReset = Bolo[bolo].reset;
						else if(Bolo[bolo].reset < LectDureeReset) LectDureeReset = Bolo[bolo].reset;
					}
				}
			} else VoieTampon[voie].lue = Bolo[bolo].a_lire && (VoieManip[voie].def.trmt[TRMT_AU_VOL].type != TRMT_INVALID);
			/* printf("(%s) %s: %s %s, et traitement au vol #%d/%d => %s\n",__func__,
				VoieManip[voie].nom,Bolo[bolo].nom,Bolo[bolo].a_lire?"a lire":"ignore",
				VoieManip[voie].def.trmt[TRMT_AU_VOL].type,TRMT_INVALID,
				VoieTampon[voie].lue?"lue":"ignoree"); */
			//- if(log && ((bolo >= 0)? Bolo[bolo].a_lire: 1) && (VoieManip[voie].def.trmt[TRMT_AU_VOL].type != TRMT_INVALID))
			if(log && VoieTampon[voie].lue) {
				l = printf("|  + %s (detecteur %s)",VoieManip[voie].nom,(bolo >= 0)? (Bolo[bolo].a_lire? "en service": "HS"): "indefini"); SambaCompleteLigne(86,l);
			}
			// if(log && !VoieTampon[voie].lue)
			//	printf("  - %s (detecteur %s)\n",VoieManip[voie].nom,(bolo >= 0)? (Bolo[bolo].a_lire? "en service": "HS"): "indefini");
		}
	}
	LectDecompteVoies(log);

	if(log) printf("|____________________________________________________________________________________|\n");
	RepartiteursVerifie(0,1,log);

}
/* ========================================================================== */
static void LectDecompteVoies(char log) {
	int fmt,voie,l;

	VoiesLocalesNb = 0; 
	// num_hdr[STRM] = num_hdr[EVTS] = 0;
	for(fmt=0; fmt<ARCH_TYPEDATA; fmt++) ArchDeclare[fmt] = ArchSauve[fmt] = 0;
	for(voie=0; voie<VoiesNb; voie++) {
		if(VoieTampon[voie].lue) {
            if(VoieTampon[voie].trig.trgr == 0) VoieTampon[voie].trig.trgr = &(VoieManip[voie].def.trgr);
			else {
				VoieTampon[voie].trig.trgr->conn.in = VoieTampon[voie].trig.trgr->conn.out = 0;
			}
			VoieTampon[voie].sauvee = VoieManip[voie].sauvee;
			VoiesLocalesNb++;
		}
		VoieManip[voie].num_hdr[STRM] = VoieManip[voie].num_hdr[EVTS] = -1;
		if((VoieTampon[voie].lue || SettingsHdrGlobal) && Archive.enservice) {
			if((VoieManip[voie].sauvee == ARCH_STREAM) || (VoieManip[voie].sauvee == ARCH_TOUT) || ((VoieManip[voie].sauvee == ARCH_DEFAUT) && !Trigger.enservice)) {
				VoieManip[voie].num_hdr[STRM] = ArchDeclare[STRM]++;
				if(VoieTampon[voie].lue) ArchSauve[STRM]++;
			}
			if(((VoieManip[voie].sauvee == ARCH_EVTS)  || (VoieManip[voie].sauvee == ARCH_TOUT) || (VoieManip[voie].sauvee == ARCH_DEFAUT)) &&  Trigger.enservice ) {
				VoieManip[voie].num_hdr[EVTS] = ArchDeclare[EVTS]++;
				if(VoieTampon[voie].lue) ArchSauve[EVTS]++;
			}
		}
	}
	if(log) {
		l = printf("|  = %2d voie%s lue%s",Accord2s(VoiesLocalesNb)); SambaCompleteLigne(86,l);
		l = printf("|  . %2d voie%s sauvee%s en stream",Accord2s(ArchSauve[STRM])); SambaCompleteLigne(86,l);
		l = printf("|  . %2d voie%s sauvee%s par evenements",Accord2s(ArchSauve[EVTS])); SambaCompleteLigne(86,l);
		l = printf("|  . %2d voie%s declaree%s en entete du stream",Accord2s(ArchDeclare[STRM])); SambaCompleteLigne(86,l);
		l = printf("|  . %2d voie%s declaree%s en entete par evenement",Accord2s(ArchDeclare[EVTS])); SambaCompleteLigne(86,l);
	}
}
/* ========================================================================== */
static char LectRegulRaz(int voie) {
	int j; char regulee;

	regulee = 0;
	if(VoieManip[voie].def.rgul.type == TRMT_REGUL_TAUX) {
		for(j=0; j<MAXECHELLES; j++) {
			VoieTampon[voie].rgul.taux.nbevts[j] = 0;
			VoieTampon[voie].rgul.taux.prochaine[j] = 0;
			if((VoieManip[voie].echelle[j].min.parm == 0.0) && (VoieManip[voie].echelle[j].max.parm == 0.0))
				VoieManip[voie].echelle[j].active = 0;
			if(VoieManip[voie].echelle[j].active) regulee = 1;
		}
	}
	return(regulee);
}
/* ==========================================================================
static INLINE void LectSetupNIchannel(int voie, int bolo, int rep, char log)  {
	int num,adc,bb,type_bn,k,l;
	char entree[12],nom[16];
	float64 polar;
	
	num = Repart[rep].adrs.val - 1;
	adc = VoieManip[voie].num_adc;
	sprintf(entree,"%s/ai%d",Repart[rep].nom+3,adc);
	bb = Bolo[bolo].captr[VoieManip[voie].cap].bb.num;
	type_bn = Numeriseur[bb].def.type;
	polar = ModeleADC[ModeleBN[type_bn].adc[adc]].polar;
	sprintf(nom,"polar-v%d",adc+1);
	if((k = NumerModlRessIndex(&(ModeleBN[type_bn]),nom)) >= 0) {
		if(ArgItemLu(Numeriseur[bb].desc,k)) polar = Numeriseur[bb].ress[k].val.r;
	}
//	printf("            . Connexion du canal de lecture %s a +/- %g Volts\n",entree,polar);
	if(log) { l = printf("| Connexion du canal de lecture %s a +/- %g Volts",entree,polar); SambaCompleteLigne(largeur_table,l); }
	SambaTesteDAQmx(0,DAQmxBaseCreateAIVoltageChan(LectNITache[num],entree,NULL,DAQmx_Val_RSE,-polar,polar,DAQmx_Val_Volts,NULL),log);
}
   ========================================================================== */
static char LectConstruitTampons(char log) {
/* Seulement hors mode=LECT_IDENT */
	int voie,bolo,cap,classe,modele,ph,catg,i,j,k;
	char trmt,nb_templates;
	int compactage;
	short d2;
	char meme_horloge,tampon_traitees_ajuste,voies_regulees;
	int nbpts,nboctets,memoire,manque;
	int fin_evt,fin_tous_evts,dim;
	TypeFiltreDef *filtrage; TypeFiltre *filtre; float facteur,unite,omega1,omega2;
	TypeMonitFenetre *f; Graph g;
#ifdef RL
    int template_dim_max;
#endif

// #ifdef DEBUG_T0
	log = 1;
// #endif

	if(OpiumAlEcran(pMonitEcran)) OpiumClear(pMonitEcran->cdr);
	if(SettingsIntervSeq) {
		LectSeuilsAb6[0] = 0.0;
		LectSeuilsAb6[1] = (float)SettingsIntervSeq / 3600000.0; /* entre 2 sequences, en heures (1 mn par defaut) */
		LectSeuilsMax = (int)(12.0 / LectSeuilsAb6[1]);       /* 12 heures d'affichage des seuils, soit 720 points */
	} else LectSeuilsMax = 0;
	if(SettingsTauxPeriode) {
		LectTauxAb6[0] = 0.0;
		LectTauxAb6[1] = (float)SettingsTauxPeriode / 60.0; /* entre 2 affichages, en minutes (1 sec. par defaut) */
		LectTauxMax = (int)(12.0 * 60.0 / LectSeuilsAb6[1]); /* 12 heures d'affichage des taux, soit 43200 points */
	} else LectTauxMax = 0;
	for(k=0; k<FiltreNb; k++) {
		filtrage = &(FiltreGeneral[k]);
		if((filtrage->modele >= 0) && ((filtrage->unite1 != FILTRE_REL) || (filtrage->unite2 != FILTRE_REL))) filtrage->coef.calcule = 0;
	}
	
/*
 * Durees en nombre de points lus ou stockes (d'apres les durees en ms dans VoieManip)
 */
	LngrTampons = DureeTampons * Echantillonnage; /* en points absolus */
	UsageTampons = 3 * LngrTampons / 4;           /* en points absolus */
//	printf("* Dimension de tampon standard: %d mots\n",LngrTampons);
	TrmtDemande = 0;
	voies_regulees = 0;
	fin_tous_evts = 0; meme_horloge = 1; compactage = 0; template_dim_max = 0;
	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
		if(log > 1) printf(". Voie %s: configuration des tampons\n",VoieManip[voie].nom);
		VoieTampon[voie].lue = 1; /* peut avoir ete mise a -1 en cas d'arret du repartiteur */
		VoieManip[voie].differe = 0;
		if((bolo = VoieManip[voie].det) >= 0) d2 = Bolo[bolo].d2; else d2 = 100;
		for(classe = 0; classe < TRMT_NBCLASSES; classe++) {
            trmt = LectTrmtUtilise(voie,classe);
			VoieTampon[voie].trmt[classe].utilise = trmt;
			VoieTampon[voie].trmt[classe].filtre = 0;
            // printf("(%s) Traitement #%d %s: #%d\n",__func__,classe+1,VoieManip[voie].nom,trmt);
			switch(trmt) {
			  case TRMT_DEMODUL:
				if((classe == TRMT_AU_VOL) && VoieManip[voie].exec && !LectReglagesOuverts) {
					VoieTampon[voie].trmt[classe].compactage = 1;
					VoieManip[voie].differe = 1;
				} else {
					VoieTampon[voie].trmt[classe].compactage = d2;
					VoieManip[voie].differe = 0;
				}
				if(VoieManip[voie].def.trmt[classe].p1 >= (d2 / 2)) VoieManip[voie].def.trmt[classe].p1 = (d2 / 2) - 1;
				VoieTampon[voie].trmt[classe].nbdata = (d2 / 2) - VoieManip[voie].def.trmt[classe].p1;
				VoieTampon[voie].trmt[classe].filtre = 0;
				break;
			#ifdef TRMT_ADDITIONNELS
			  case TRMT_LISSAGE:
				if(VoieManip[voie].def.trmt[classe].p1 <= 0) VoieManip[voie].def.trmt[classe].p1 = 1;
				VoieTampon[voie].trmt[classe].compactage = 1;
				VoieTampon[voie].trmt[classe].nbdata = 0;
				VoieTampon[voie].trmt[classe].filtre = 0;
				VoieTampon[voie].trmt[classe].X.dim = VoieManip[voie].def.trmt[classe].p1 + 1;
				if(VoieTampon[voie].trmt[classe].X.max != VoieTampon[voie].trmt[classe].X.dim) {
					if(VoieTampon[voie].trmt[classe].X.val) {
						free(VoieTampon[voie].trmt[classe].X.val);
						VoieTampon[voie].trmt[classe].X.val = 0;
					}
					VoieTampon[voie].trmt[classe].X.val = (float *)malloc(VoieTampon[voie].trmt[classe].X.dim * sizeof(float));
					if(!VoieTampon[voie].trmt[classe].X.val) VoieTampon[voie].trmt[classe].X.max = 0;
					else VoieTampon[voie].trmt[classe].X.max = VoieTampon[voie].trmt[classe].X.dim;
				}
				VoieTampon[voie].trmt[classe].X.nb = VoieTampon[voie].trmt[classe].X.prem = 0;
				break;
			  case TRMT_MOYENNE:
			  case TRMT_MAXIMUM:
				if(VoieManip[voie].def.trmt[classe].p1 <= 0) VoieManip[voie].def.trmt[classe].p1 = 1;
				VoieTampon[voie].trmt[classe].compactage = VoieManip[voie].def.trmt[classe].p1;
				VoieTampon[voie].trmt[classe].nbdata = 0;
				VoieTampon[voie].trmt[classe].filtre = 0;
				break;
			#ifdef TRMT_DECONVOLUE
			  case TRMT_DECONV:
				if(VoieManip[voie].def.trmt[classe].p1 <= 0) VoieManip[voie].def.trmt[classe].p1 = 3;
				if(VoieManip[voie].def.trmt[classe].p2 <= 0) VoieManip[voie].def.trmt[classe].p2 = 5;
				if(VoieManip[voie].def.trmt[classe].p3 <= 0.0) VoieManip[voie].def.trmt[classe].p3 = 0.140;
				VoieTampon[voie].trmt[classe].compactage = 1;
				VoieTampon[voie].trmt[classe].nbdata = 0;
				VoieTampon[voie].trmt[classe].filtre = 0;
				VoieTampon[voie].trmt[classe].L.dim = VoieManip[voie].def.trmt[classe].p1 + 1;
				if(VoieTampon[voie].trmt[classe].L.max != VoieTampon[voie].trmt[classe].L.dim) {
					if(VoieTampon[voie].trmt[classe].L.val) {
						free(VoieTampon[voie].trmt[classe].L.val);
						VoieTampon[voie].trmt[classe].L.val = 0;
					}
					VoieTampon[voie].trmt[classe].L.val = (int *)malloc(VoieTampon[voie].trmt[classe].L.dim * sizeof(int));
					if(!VoieTampon[voie].trmt[classe].L.val) VoieTampon[voie].trmt[classe].L.max = 0;
					else VoieTampon[voie].trmt[classe].L.max = VoieTampon[voie].trmt[classe].L.dim;
				}
				break;
			#endif
			#ifdef A_LA_GG
			  case TRMT_INTEDIF:
				if(VoieManip[voie].def.trmt[classe].p1 <= 0) VoieManip[voie].def.trmt[classe].p1 = 7;
				if(VoieManip[voie].def.trmt[classe].p3 <= 0) VoieManip[voie].def.trmt[classe].p3 = 0.2;
				VoieManip[voie].def.trmt[classe].p2 = (VoieManip[voie].def.trmt[classe].p3 / VoieEvent[voie].horloge); /* .p3 est en ms */
				VoieTampon[voie].trmt[classe].compactage = 1;
				VoieTampon[voie].trmt[classe].nbdata = 0;
				VoieTampon[voie].trmt[classe].filtre = 0;
				VoieTampon[voie].trmt[classe].X.dim = VoieManip[voie].def.trmt[classe].p1 + 1;
				if(VoieTampon[voie].trmt[classe].X.max != VoieTampon[voie].trmt[classe].X.dim) {
					if(VoieTampon[voie].trmt[classe].X.val) {
						free(VoieTampon[voie].trmt[classe].X.val);
						VoieTampon[voie].trmt[classe].X.val = 0;
					}
					VoieTampon[voie].trmt[classe].X.val = (float *)malloc(VoieTampon[voie].trmt[classe].X.dim * sizeof(float));
					if(!VoieTampon[voie].trmt[classe].X.val) VoieTampon[voie].trmt[classe].X.max = 0;
					else VoieTampon[voie].trmt[classe].X.max = VoieTampon[voie].trmt[classe].X.dim;
				}
				VoieTampon[voie].trmt[classe].X.nb = VoieTampon[voie].trmt[classe].X.prem = 0;
				VoieTampon[voie].trmt[classe].Y.dim = VoieManip[voie].def.trmt[classe].p2 + 1;
				if(VoieTampon[voie].trmt[classe].Y.max != VoieTampon[voie].trmt[classe].Y.dim) {
					if(VoieTampon[voie].trmt[classe].Y.val) {
						free(VoieTampon[voie].trmt[classe].Y.val);
						VoieTampon[voie].trmt[classe].Y.val = 0;
					}
					VoieTampon[voie].trmt[classe].Y.val = (float *)malloc(VoieTampon[voie].trmt[classe].Y.dim * sizeof(float));
					if(!VoieTampon[voie].trmt[classe].Y.val) VoieTampon[voie].trmt[classe].Y.max = 0;
					else VoieTampon[voie].trmt[classe].Y.max = VoieTampon[voie].trmt[classe].Y.dim;
				}
				VoieTampon[voie].trmt[classe].Y.nb = VoieTampon[voie].trmt[classe].Y.prem = 0;
				break;
			#endif /* A_LA_GG */
			#ifdef A_LA_MARINIERE
			  case TRMT_CSNSM:
				if((VoieManip[voie].def.trmt[classe].p1 < 0) || (VoieManip[voie].def.trmt[classe].p1 >= TrmtCsnsmNb)) VoieManip[voie].def.trmt[classe].p1 = 0;
				i = VoieManip[voie].def.trmt[classe].p1;
				VoieTampon[voie].trmt[classe].compactage = 1;
				VoieTampon[voie].trmt[classe].nbdata = 0;
				VoieTampon[voie].trmt[classe].filtre = 0;
				VoieTampon[voie].trmt[classe].V = VoieTampon[voie].trmt[classe].W = 0.0;
				VoieTampon[voie].trmt[classe].X.dim = TrmtCsnsm[i].A + 1;
				if(VoieTampon[voie].trmt[classe].X.max != VoieTampon[voie].trmt[classe].X.dim) {
					if(VoieTampon[voie].trmt[classe].X.val) {
						free(VoieTampon[voie].trmt[classe].X.val);
						VoieTampon[voie].trmt[classe].X.val = 0;
					}
					VoieTampon[voie].trmt[classe].X.val = (float *)malloc(VoieTampon[voie].trmt[classe].X.dim * sizeof(float));
					if(!VoieTampon[voie].trmt[classe].X.val) VoieTampon[voie].trmt[classe].X.max = 0;
					else VoieTampon[voie].trmt[classe].X.max = VoieTampon[voie].trmt[classe].X.dim;
				}
				VoieTampon[voie].trmt[classe].X.nb = VoieTampon[voie].trmt[classe].X.prem = 0;
				VoieTampon[voie].trmt[classe].Y.dim = (2 * TrmtCsnsm[i].B) + TrmtCsnsm[i].C + 1;
				if(VoieTampon[voie].trmt[classe].Y.max != VoieTampon[voie].trmt[classe].Y.dim) {
					if(VoieTampon[voie].trmt[classe].Y.val) {
						free(VoieTampon[voie].trmt[classe].Y.val);
						VoieTampon[voie].trmt[classe].Y.val = 0;
					}
					VoieTampon[voie].trmt[classe].Y.val = (float *)malloc(VoieTampon[voie].trmt[classe].Y.dim * sizeof(float));
					if(!VoieTampon[voie].trmt[classe].Y.val) VoieTampon[voie].trmt[classe].Y.max = 0;
					else VoieTampon[voie].trmt[classe].Y.max = VoieTampon[voie].trmt[classe].Y.dim;
				}
				VoieTampon[voie].trmt[classe].Y.nb = VoieTampon[voie].trmt[classe].Y.prem = 0;
				VoieTampon[voie].trmt[classe].Z.dim = TrmtCsnsm[i].D + 1;
				if(VoieTampon[voie].trmt[classe].Z.max != VoieTampon[voie].trmt[classe].Z.dim) {
					if(VoieTampon[voie].trmt[classe].Z.val) {
						free(VoieTampon[voie].trmt[classe].Z.val);
						VoieTampon[voie].trmt[classe].Z.val = 0;
					}
					VoieTampon[voie].trmt[classe].Z.val = (float *)malloc(VoieTampon[voie].trmt[classe].Z.dim * sizeof(float));
					if(!VoieTampon[voie].trmt[classe].Z.val) VoieTampon[voie].trmt[classe].Z.max = 0;
					else VoieTampon[voie].trmt[classe].Z.max = VoieTampon[voie].trmt[classe].Z.dim;
				}
				VoieTampon[voie].trmt[classe].Z.nb = VoieTampon[voie].trmt[classe].Z.prem = 0;
				if(!VoieTampon[voie].trmt[TRMT_AU_VOL].X.val || !VoieTampon[voie].trmt[TRMT_AU_VOL].Y.val || !VoieTampon[voie].trmt[TRMT_AU_VOL].Z.val) {
					printf("!!! Filtrage CSNSM sur %s: manque de donnees, traitement annule\n",VoieManip[voie].nom);
					VoieManip[voie].def.trmt[classe].type = NEANT;
				}
				break;
			#endif /* A_LA_MARINIERE */
			#endif /* TRMT_ADDITIONNELS */
			  case TRMT_FILTRAGE:
				VoieTampon[voie].trmt[classe].compactage = 1;
				VoieTampon[voie].trmt[classe].nbdata = 0;
				modele = VoieManip[voie].type;
				if(VoieManip[voie].def.trmt[classe].texte[0]) {
					for(k=0; k<FiltreNb; k++) if(!strcmp(VoieManip[voie].def.trmt[classe].texte,FiltreGeneral[k].nom)) break;
					if(k < FiltreNb) VoieManip[voie].def.trmt[classe].p1 = k;
					else printf("!!! Filtre de la voie %s inconnu (%s)\n",VoieManip[voie].nom,VoieManip[voie].def.trmt[classe].texte);
				} else k = VoieManip[voie].def.trmt[classe].p1;
				if((k >= 0) && (k < FiltreNb)) {
					filtrage = &(FiltreGeneral[k]);
					filtre = &(filtrage->coef);
					if((filtrage->modele >= 0) && ((filtrage->unite1 != FILTRE_REL) || (filtrage->unite2 != FILTRE_REL)) && !filtre->calcule) {
						if(classe == TRMT_AU_VOL) facteur = 1.0 / Echantillonnage;
						else if(classe == TRMT_PRETRG)
							facteur = ((float)VoieTampon[voie].trmt[TRMT_AU_VOL].compactage / Echantillonnage);
						if(filtrage->unite1 != FILTRE_REL) {
							switch(filtrage->unite1) {
							  case FILTRE_HZ:  unite = 1.0e-3; break;
							  case FILTRE_KHZ: unite = 1.0; break;
							  case FILTRE_MHZ: unite = 1.0e3; break;
							  case FILTRE_GHZ: unite = 1.0e6; break;
							  default: unite = 1.0; break;
							}
							omega1 = filtrage->omega1 * unite * facteur;
							// printf("(%s) O1 = %g x %g x %g = %g\n",__func__,filtrage->omega1,unite,facteur,omega1);
						} else {
							omega1 = filtrage->omega1;
							// printf("(%s) O1[%d] = %g = %g\n",__func__,k,filtrage->omega1,omega1);
						}
						if(filtrage->unite2 != FILTRE_REL) {
							switch(filtrage->unite2) {
							  case FILTRE_HZ:  unite = 1.0e-3; break;
							  case FILTRE_KHZ: unite = 1.0; break;
							  case FILTRE_MHZ: unite = 1.0e3; break;
							  case FILTRE_GHZ: unite = 1.0e6; break;
							  default: unite = 1.0; break;
							}
							omega2 = filtrage->omega2 * unite * facteur;
						} else omega2 = filtrage->omega2;
						printf(". Calcul des coefficients du filtre #%d: %s\n",k+1,filtrage->nom);
						if(!FiltreCalculeStd(omega1,omega2,filtrage->modele,filtrage->type,filtrage->degre,filtre)) {
							OpiumError("Filtre %s inutilisable: %s",filtrage->nom,FiltreErreur);
							printf("!!! Filtre inutilisable: %s\n",FiltreErreur); return(0);
						}
					}
					if(VoieManip[voie].def.trmt[classe].texte[0] == '\0') strncpy(VoieManip[voie].def.trmt[classe].texte,FiltreGeneral[k].nom,TRMT_TEXTE);
				} else filtre = 0;
				VoieTampon[voie].trmt[classe].filtre = filtre;
				break;
			  case TRMT_INVALID:
				VoieTampon[voie].trmt[classe].compactage = compactage; /* pour ne pas influencer sur les autres */
				VoieTampon[voie].trmt[classe].nbdata = 0;
				VoieTampon[voie].trmt[classe].filtre = 0;
				break;
			  default:
				VoieTampon[voie].trmt[classe].compactage = 1;
				VoieTampon[voie].trmt[classe].nbdata = 0;
				VoieTampon[voie].trmt[classe].filtre = 0;
			}
			if(VoieManip[voie].def.trmt[classe].gain == 0) VoieManip[voie].def.trmt[classe].gain = 1;
			SambaTrmtEncode(voie,classe);
		}
		if(VoieTampon[voie].trig.trgr->type == TRGR_EXT) {
			VoieTampon[voie].decal = 0; VoieTampon[voie].interv = 0;
		} else {
			VoieTampon[voie].decal = (int)(VoieManip[voie].def.evt.delai * Echantillonnage); /* decalage points absolus (~ p.r. voie 0) */
			VoieTampon[voie].interv = (int)(VoieManip[voie].def.evt.interv * Echantillonnage); /* nb points absolus */
		}
		// printf("(%s) Recherche du maxi pour %16s dans un intervalle de %d points absolus decales de %d\n",__func__,
		// 	VoieManip[voie].nom,VoieTampon[voie].interv,VoieTampon[voie].decal);
		if(ArchSauve[STRM] && (VoieManip[voie].num_hdr[STRM] >= 0) && meme_horloge) {
			if(compactage) {
				if(VoieTampon[voie].trmt[TRMT_AU_VOL].compactage != compactage) {
					OpiumFail("Execution impossible: toutes les voies sauvees en stream n'ont pas la meme horloge");
					LectJournalTrmt();
					LectBranche(1);
					return(0);
				}
			} else compactage = VoieTampon[voie].trmt[TRMT_AU_VOL].compactage;
		} else if(VoieManip[voie].differe) {
			VoieTampon[voie].trmt[TRMT_PRETRG].compactage *= d2;
		}
		for(classe = 0; classe < TRMT_NBCLASSES; classe++) if(VoieTampon[voie].trmt[classe].compactage <= 0) VoieTampon[voie].trmt[classe].compactage = 1;
		VoieTampon[voie].brutes->dim = LngrTampons / VoieTampon[voie].trmt[TRMT_AU_VOL].compactage; // soit DureeTampons / VoieEvent[voie].horloge;
		
		VoieEvent[voie].horloge = (float)VoieTampon[voie].trmt[TRMT_AU_VOL].compactage / Echantillonnage;
		if(VoieTampon[voie].trig.trgr->type == TRGR_TEST) VoieEvent[voie].lngr = CalcDim;
		else VoieEvent[voie].lngr = VoieManip[voie].def.evt.duree / VoieEvent[voie].horloge; /* nb points stockes par evt */
		VoieEvent[voie].base_dep = (int)((float)VoieEvent[voie].lngr * VoieManip[voie].def.evt.base_dep * VoieManip[voie].def.evt.pretrigger / 10000.0); /* pts stockes */
		VoieEvent[voie].base_lngr = (int)((float)VoieEvent[voie].lngr * (VoieManip[voie].def.evt.base_arr - VoieManip[voie].def.evt.base_dep) * VoieManip[voie].def.evt.pretrigger / 10000.0); /* pts stockes */
		VoieEvent[voie].avant_evt = (int)((float)VoieEvent[voie].lngr * VoieManip[voie].def.evt.pretrigger / 100.0); /* pts stockes */
		VoieEvent[voie].apres_evt = (VoieEvent[voie].lngr - VoieEvent[voie].avant_evt) * VoieTampon[voie].trmt[TRMT_AU_VOL].compactage; /* pts absolus */
	#ifdef DEBUG_T0
		printf("(%s) #%d=%s: compactage=%d, horloge=%g ms, dim.tampon=%d\n",__func__,voie,VoieManip[voie].nom,
			   VoieTampon[voie].trmt[TRMT_AU_VOL].compactage,VoieEvent[voie].horloge,VoieTampon[voie].brutes->dim);
	#endif

		for(ph=0; ph<DETEC_PHASES_MAX; ph++) {
			VoieTampon[voie].phase[ph].t0 = (int)(VoieManip[voie].def.evt.phase[ph].t0 / VoieEvent[voie].horloge); /* points stockes dans descente rapide */
			VoieTampon[voie].phase[ph].dt = (int)(VoieManip[voie].def.evt.phase[ph].dt / VoieEvent[voie].horloge); /* points stockes dans descente rapide */
		}
		fin_evt = VoieTampon[voie].decal + (VoieTampon[voie].interv / 2) + VoieEvent[voie].apres_evt;
		if(fin_evt > fin_tous_evts) fin_tous_evts = fin_evt;
		// printf("(%s) Post-trigger de %d points, soit fin evt apres %d points (globalement %d points)\n",__func__,
		// 	VoieEvent[voie].apres_evt,fin_evt,fin_tous_evts);
	#ifdef RL
        if(VoieManip[voie].def.evt.dimtemplate * VoieTampon[voie].trmt[TRMT_AU_VOL].compactage > template_dim_max)
			template_dim_max = VoieManip[voie].def.evt.dimtemplate * VoieTampon[voie].trmt[TRMT_AU_VOL].compactage;
	#endif
		if(VoieManip[voie].modulee) VoieTampon[voie].pattern.dim = 0;
		else {
			i = (int)(VoieManip[voie].duree.periodes + 0.4); VoieManip[voie].duree.periodes = (float)i; /* pour tronquer */
			VoieTampon[voie].pattern.dim = (SettingsSoustra && (VoieManip[voie].duree.periodes > 0.0))? d2: 0;
		}
		VoieTampon[voie].regul_active = (Trigger.enservice && (VoieTampon[voie].trig.trgr->type != NEANT) && VoieTampon[voie].trig.trgr->regul_demandee)? VoieManip[voie].def.rgul.type: NEANT;
		if(LectRegulRaz(voie)) voies_regulees = 1;
		VoieTampon[voie].trmt[TRMT_AU_VOL].ab6[0] = 0.0; /* (float)VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien * VoieEvent[voie].horloge; */
		VoieTampon[voie].trmt[TRMT_AU_VOL].ab6[1] = VoieEvent[voie].horloge;
		VoieTampon[voie].trmt[TRMT_PRETRG].ab6[0] = 0.0;
		VoieTampon[voie].trmt[TRMT_PRETRG].ab6[1] = VoieEvent[voie].horloge * (float)VoieTampon[voie].trmt[TRMT_PRETRG].compactage;
#ifdef DEBUG_T0
		printf("(%s) Ab6[%d] au vol = %g + n x %g\n",__func__,voie,VoieTampon[voie].trmt[TRMT_AU_VOL].ab6[0],VoieTampon[voie].trmt[TRMT_AU_VOL].ab6[1]);
		printf("(%s) Ab6[%d] tampon = %g + n x %g\n",__func__,voie,VoieTampon[voie].trmt[TRMT_PRETRG].ab6[0],VoieTampon[voie].trmt[TRMT_PRETRG].ab6[1]);
#endif
		MonitEvtAb6[voie][0] = 0.0;
		MonitEvtAb6[voie][1] = VoieTampon[voie].trmt[TRMT_AU_VOL].ab6[1];
		MonitTrmtAb6[voie][0] = 0.0;
		MonitTrmtAb6[voie][1] = VoieTampon[voie].trmt[TRMT_PRETRG].ab6[1];
		VoieTampon[voie].post = (VoieTampon[voie].pattern.dim || (VoieManip[voie].def.trmt[TRMT_PRETRG].type != NEANT) || LectModeSpectresAuto);
		VoieTampon[voie].post = (VoieTampon[voie].post && (VoieManip[voie].def.trmt[TRMT_PRETRG].type != TRMT_INVALID));
		if(VoieTampon[voie].post) TrmtDemande = 1;
	} else {
		VoieTampon[voie].brutes->dim = 0;
		VoieTampon[voie].pattern.dim = 0;
		VoieTampon[voie].regul_active = NEANT;
		for(classe = 0; classe < TRMT_NBCLASSES; classe++) SambaTrmtEncode(voie,classe); // a tout hasard
	}
	if(compactage <= 0) compactage = 1; Frequence = Echantillonnage / (float)compactage;
	TrmtRegulActive = voies_regulees? TrmtRegulDemandee: 0;

	MasseTotale = 0.0;
	for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire) {
		for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
			voie = Bolo[bolo].captr[cap].voie;
			if(VoieTampon[voie].lue) { MasseTotale += Bolo[bolo].masse; break; }
		}
	}

/* Situation des voies au demarrage */
	if(log > 2) for(voie=0; voie<VoiesNb; voie++) {
		printf("Voie%02d[%d]: %s",voie,VoieTampon[voie].brutes->max,VoieManip[voie].nom);
		printf(" sur alim %s,",VoieManip[voie].modulee?"modulee":"DC");;
		printf(" evt %g ms",VoieManip[voie].def.evt.duree/1000.0);
		printf(" echantillonnee par %g ms:",VoieEvent[voie].horloge);
		if(VoieTampon[voie].lue) {
			printf("\n          @%08X,",VoieTampon[voie].brutes->t.sval);
			printf(" filtrage @%08x[%d]\n",VoieTampon[voie].traitees->t.rval,VoieTampon[voie].traitees->max);
		} else printf(" pas lue\n");
	}
#ifdef TESTE_MEMOIRE
	i = FreeMem();
	if(LectCurrMem) printf("Memoire libre avant    : %11d octets (perte: %d pendant le run)\n",
		i,LectCurrMem - i);
	else printf("Memoire libre avant    : %11d octets\n",i);
    LectPrevMem = i;
    j = i;
#endif

/*
 * Liberation des tampons inutiles ou de taille incorrecte
 */
	tampon_traitees_ajuste = 0;
    for(voie=0; voie<VoiesNb; voie++) {
		if(log > 1) printf(". Voie %s: liberation des tampons inutiles ou de taille incorrecte\n",VoieManip[voie].nom);
		/* donnees brutes */
        nbpts = VoieTampon[voie].brutes->dim;
        if(nbpts != VoieTampon[voie].brutes->max) {
            if(VoieTampon[voie].brutes->t.data) {
				if(log > 1) printf(". Libere VoieTampon[%d].brutes->t.sval @%08X pour %d donnees\n",voie,VoieTampon[voie].brutes->t.sval,VoieTampon[voie].brutes->max);
                free(VoieTampon[voie].brutes->t.data);
//				MonitFenNettoie(voie,TRMT_AU_VOL,MONIT_SIGNAL);
//				MonitFenNettoie(voie,TRMT_AU_VOL,MONIT_EVENT);
                VoieTampon[voie].brutes->t.data = 0;
			}
            VoieTampon[voie].brutes->max = 0;
		}
		/* evenement moyen */
		if(VoieTampon[voie].lue && VoieTampon[voie].moyen.calcule) nbpts = VoieEvent[voie].lngr;
		else nbpts = 0;
        if(nbpts != VoieTampon[voie].somme.taille) {
            if(VoieTampon[voie].somme.val) {
				if(log > 1) printf("Libere VoieTampon[%d].somme.val @%08X pour %d donnees\n",voie,VoieTampon[voie].somme.val,VoieTampon[voie].somme.taille);
                free(VoieTampon[voie].somme.val);
//				MonitFenNettoie(voie,TRMT_AU_VOL,MONIT_MOYEN);
                VoieTampon[voie].somme.val = 0;
            }
            VoieTampon[voie].somme.taille = 0;
        }
		/* suivi du filtrage (le pb des longueurs sera traite a l'affectation) */
		for(classe = 0; classe < TRMT_NBCLASSES; classe++) {
			if(!VoieTampon[voie].lue || (VoieManip[voie].def.trmt[classe].type != TRMT_FILTRAGE)) {
				if(log > 1 && VoieTampon[voie].trmt[classe].suivi.brute) 
					printf("Libere VoieTampon[%d].trmt[%d].suivi.brute @%08X pour %d donnees\n",voie,classe,VoieTampon[voie].trmt[classe].suivi.brute,VoieTampon[voie].trmt[classe].suivi.max);
				if(VoieTampon[voie].trmt[classe].suivi.brute) free(VoieTampon[voie].trmt[classe].suivi.brute);
				VoieTampon[voie].trmt[classe].suivi.brute = 0;
				for(j=0; j<MAXFILTREETG; j++) {
					if(VoieTampon[voie].trmt[classe].suivi.filtree[j]) {
						free(VoieTampon[voie].trmt[classe].suivi.filtree[j]);
						VoieTampon[voie].trmt[classe].suivi.filtree[j] = 0;
					}
				}
				VoieTampon[voie].trmt[classe].suivi.max = 0;
				VoieTampon[voie].trmt[classe].suivi.nbetg = 0;
			}
		}
		/* resultat du traitement pre-trigger */
		if(VoieTampon[voie].lue && !VoieTampon[voie].trig.special /* && ((VoieManip[voie].def.trmt[TRMT_PRETRG].type != NEANT) || (SettingsSoustra && (VoieManip[voie].duree.periodes > 0.0))) */) {
			// printf("(%s) Taille prevue pour le tampon apres traitement de %s: %d\n",__func__,VoieManip[voie].nom,VoieManip[voie].duree.traitees);
			VoieTampon[voie].traitees->dim = VoieManip[voie].duree.traitees;
			dim = fin_tous_evts - VoieTampon[voie].decal + (VoieTampon[voie].interv / 2) + (VoieEvent[voie].avant_evt * VoieTampon[voie].trmt[TRMT_AU_VOL].compactage);
			// printf("(%s) Pretrigger de %d points stockes, soit dimension mini=%d absolus",__func__,VoieEvent[voie].avant_evt,dim);
			dim /= (VoieTampon[voie].trmt[TRMT_AU_VOL].compactage * VoieTampon[voie].trmt[TRMT_PRETRG].compactage);
			// printf(" (%d traites)\n",dim);
//+				dim += (SettingsIntervTrmt / VoieEvent[voie].horloge);
			dim += 1;
			if(VoieTampon[voie].traitees->dim < dim) VoieTampon[voie].traitees->dim = dim;
			if(VoieTampon[voie].traitees->dim < VoieEvent[voie].lngr) VoieTampon[voie].traitees->dim = VoieEvent[voie].lngr;
#ifdef RL
//			VoieTampon[voie].traitees->dim += (int) (2. * template_dim_max / VoieTampon[voie].trmt[TRMT_AU_VOL].compactage) + 1;
			dim = (2 * template_dim_max / VoieTampon[voie].trmt[TRMT_AU_VOL].compactage) + 1;
			// printf("(%s) Taille du template de %s: %d\n",__func__,VoieManip[voie].nom,dim);
			if(VoieTampon[voie].traitees->dim < dim) VoieTampon[voie].traitees->dim = dim;
#endif
			// printf(" soit %d attribues\n",VoieTampon[voie].traitees->dim);
			// reactiver quand le debug sera fini: if(VoieManip[voie].duree.traitees != VoieTampon[voie].traitees->dim) {
				if(!tampon_traitees_ajuste) {
					printf("\n");
					printf(" _______________________________________________________________ \n");
					printf("|           Ajustement du tampon de donnees traitees            |\n");
					printf("|_______________________________________________________________|\n");
					printf("| Voie                              |    demande  |   attribue  |\n");
					printf("|___________________________________|_____________|_____________|\n");
					tampon_traitees_ajuste = 1;
				}
				printf("| %-33s | %11d | %11d |\n",VoieManip[voie].nom,VoieManip[voie].duree.traitees,VoieTampon[voie].traitees->dim);
			// reactiver ... }
		} else VoieTampon[voie].traitees->dim = 0;
		if(VoieTampon[voie].traitees->dim != VoieTampon[voie].traitees->max) {
			if(VoieTampon[voie].traitees->t.rval) {
				if(log > 1) printf("Libere VoieTampon[%d].traitees->t.rval @%08X pour %d donnees\n",voie,VoieTampon[voie].traitees->t.rval,VoieTampon[voie].traitees->max);
				free(VoieTampon[voie].traitees->t.rval);
//				MonitFenNettoie(voie,TRMT_PRETRG,MONIT_SIGNAL);
//				MonitFenNettoie(voie,TRMT_PRETRG,MONIT_EVENT);
				VoieTampon[voie].traitees->t.rval = 0;
			}
			VoieTampon[voie].traitees->max = 0;
		}
		/* pattern moyen */
		nbpts = VoieTampon[voie].pattern.dim;
		if(nbpts != VoieTampon[voie].pattern.max) {
			if(VoieTampon[voie].pattern.val) {
				if(log > 1) printf("Libere VoieTampon[%d].pattern.val @%08X pour %d donnees\n",voie,VoieTampon[voie].pattern.val,VoieTampon[voie].pattern.max);
				free(VoieTampon[voie].pattern.val);
//				MonitFenNettoie(voie,TRMT_AU_VOL,MONIT_PATTERN);
				VoieTampon[voie].pattern.val = 0;
			}
			if(VoieTampon[voie].pattern.nb) {
				if(log > 1) printf("Libere VoieTampon[%d].pattern.nb  @%08X pour %d donnees\n",voie,VoieTampon[voie].pattern.nb,VoieTampon[voie].pattern.max);
				free(VoieTampon[voie].pattern.nb);
				VoieTampon[voie].pattern.nb = 0;
			}
			VoieTampon[voie].pattern.max = 0;	
		}
		if(VoieTampon[voie].pattern.max != VoieTampon[voie].pattref.taille) {
			if(VoieTampon[voie].pattref.val) {
				if(log > 1) printf("Libere VoieTampon[%d].pattref.val @%08X pour %d donnees\n",voie,VoieTampon[voie].pattref.val,VoieTampon[voie].pattern.max);
				free(VoieTampon[voie].pattref.val);
				VoieTampon[voie].pattref.val = 0;
			}
			VoieTampon[voie].pattref.taille = 0;
		}
		if(!VoieTampon[voie].lue) {
			/* graphiques utilisateurs */
            for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if((g = f->g)) {
                for(j=0; j<f->nb; j++) if(voie == f->trace[j].voie) {
                    if(OpiumDisplayed(g->cdr)) OpiumClear(g->cdr);
					if(log > 1) printf("Efface un graphique @%08X\n",g);
                    GraphDelete(g);
                    f->g = 0;
                    break;
                }
            }
			/* fenetres elementaires des donnees brutes */
			if(gDonnee[voie]) {
				if(OpiumDisplayed(gDonnee[voie]->cdr)) OpiumClear(gDonnee[voie]->cdr);
				if(log > 1) printf("Efface un graphique @%08X\n",gDonnee[voie]);
				GraphDelete(gDonnee[voie]);
				gDonnee[voie] = 0;
			}
		}
		/* suivi des seuils */
		if(VoieTampon[voie].lue && (VoieTampon[voie].trig.trgr->type != NEANT) && (VoieTampon[voie].trig.trgr->sens > 0))
			VoieTampon[voie].seuilmin.dim = LectSeuilsMax;
		else VoieTampon[voie].seuilmin.dim = 0;
		if(VoieTampon[voie].seuilmin.dim != VoieTampon[voie].seuilmin.max) {
			if(VoieTampon[voie].seuilmin.val) {
				if(log > 1) printf("Libere VoieTampon[%d].seuilmin.val @%08X pour %d donnees\n",voie,VoieTampon[voie].seuilmin.val,VoieTampon[voie].seuilmin.max);
				free(VoieTampon[voie].seuilmin.val);
				VoieTampon[voie].seuilmin.val = 0;
			}
			VoieTampon[voie].seuilmin.max = 0;
		}
		if(VoieTampon[voie].lue && (VoieTampon[voie].trig.trgr->type != NEANT) && (VoieTampon[voie].trig.trgr->sens < 2)) 
			VoieTampon[voie].seuilmax.dim = LectSeuilsMax;
		else VoieTampon[voie].seuilmax.dim = 0;
		if(VoieTampon[voie].seuilmax.dim != VoieTampon[voie].seuilmax.max) {
			if(VoieTampon[voie].seuilmax.val) {
				if(log > 1) printf("Libere VoieTampon[%d].seuilmax.val @%08X pour %d donnees\n",voie,VoieTampon[voie].seuilmax.val,VoieTampon[voie].seuilmax.max);
				free(VoieTampon[voie].seuilmax.val);
				VoieTampon[voie].seuilmax.val = 0;
			}
			VoieTampon[voie].seuilmax.max = 0;
		}			
		/* suivi des taux */
		if(VoieTampon[voie].lue && (VoieTampon[voie].trig.trgr->type != NEANT))
			VoieTampon[voie].tabtaux.dim = LectTauxMax;
		else VoieTampon[voie].tabtaux.dim = 0;
		if(VoieTampon[voie].tabtaux.dim != VoieTampon[voie].tabtaux.max) {
			if(VoieTampon[voie].tabtaux.val) {
				if(log > 1) printf("Libere VoieTampon[%d].tabtaux.val @%08X pour %d donnees\n",voie,VoieTampon[voie].tabtaux.val,VoieTampon[voie].tabtaux.max);
				free(VoieTampon[voie].tabtaux.val);
				VoieTampon[voie].tabtaux.val = 0;
			}
			VoieTampon[voie].tabtaux.max = 0;
		}
	}
	if(tampon_traitees_ajuste) printf("|___________________________________|_____________|_____________|\n");
			
	for(catg=0; catg<MonitCatgNb+1; catg++) if(MonitCatgPtr[catg]->utilisee) MonitCatgPtr[catg]->activite.dim = LectTauxMax;

	if(!PicsNb || (PicsDim != SettingsAmplBins)) {
		if(PicsDim) { free(PicX); free(PicV); free(PicV); PicsDim = 0; }
	}
			
/* Ainsi, en cas de chgmt de taille, on a D'ABORD libere tous les buffers condamnes.
   Ca n'empeche que meme si on se contente de ne changer que le nb de bolos lus, 
   il faut, quoiqu'il en soit, faire quelques alloc/deallocations */

/*
 * Affectation des seuls tampons necessaires et mise a jour des informations annexes
 */
	manque = memoire = 0;
	nb_templates = 0;
	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
		if(log > 1) printf(". Voie %s: affectation des seuls tampons necessaires\n",VoieManip[voie].nom);
	/* donnees brutes */
		nbpts = VoieTampon[voie].brutes->dim;
		if(VoieTampon[voie].brutes->type == TAMPON_INT16) nboctets = nbpts * sizeof(TypeDonnee);
		else if(VoieTampon[voie].brutes->type == TAMPON_INT32) nboctets = nbpts * sizeof(TypeLarge);
		else nboctets = nbpts * sizeof(TypeSignal);
		if(log > 1) printf("Demande VoieTampon[%d].brutes->t.sval   pour %d donnees\n",voie,VoieTampon[voie].brutes->dim);
		memoire += nbpts;
		if(!VoieTampon[voie].brutes->t.data) {
			VoieTampon[voie].brutes->t.data = (void *)malloc(nboctets);
		#ifdef TESTE_MEMOIRE
			j -= nboctets;
		#endif
			if(!VoieTampon[voie].brutes->t.data) {
				printf("Donnees voie #%d: %d octets introuvables\n",voie,nboctets);
				manque += nbpts;
				VoieTampon[voie].brutes->max = 0;
				continue;
			}
			memset((void *)VoieTampon[voie].brutes->t.data,0xB,nboctets);
		}
		VoieTampon[voie].brutes->max = nbpts;
		VoieTampon[voie].prochain_s16 = VoieTampon[voie].brutes->t.sval;
		VoieTampon[voie].prochain_i32 = VoieTampon[voie].brutes->t.ival;
		if(log > 1) printf("Alloue  VoieTampon[%d].tampon @%08X pour %d donnees\n",voie,VoieTampon[voie].brutes->t.sval,VoieTampon[voie].brutes->max);
	#ifdef VERIFIE_ALLOCS
		printf("Alloc V%d[%d] (%s) @%08X->%08X\n",voie,nbpts,VoieManip[voie].nom,VoieTampon[voie].brutes->t.sval,VoieTampon[voie].prochain);
	#endif
	/* espace pour base/bruit ligne de base */
		if(VoieManip[voie].duree.mesureLdB <= 0) VoieManip[voie].duree.mesureLdB = 50;
		else if(VoieManip[voie].duree.mesureLdB > VoieTampon[voie].brutes->max) VoieManip[voie].duree.mesureLdB = VoieTampon[voie].brutes->max;
	/* utilisation de la bonne configuration
		if(ConfigNb > 1) {
			TypeConfigDefinition *config,*usuelle;
			usuelle = &(VoieManip[voie].config.usuelle);
			config = 0;
			if(ConfigNum >= 0) {
				int num_config;
				for(num_config=0; num_config<VoieManip[voie].config.nb; num_config++) {
					if(VoieManip[voie].config.def[num_config].num == ConfigNum) {
						config = &(VoieManip[voie].config.def[num_config]);
						break;
					}
				}
			}
			if(config && config->defini.evt) memcpy(&(VoieManip[voie].def.evt),&(config->evt),sizeof(TypeVoieModele));
			else memcpy(&(VoieManip[voie].def.evt),&(usuelle->evt),sizeof(TypeVoieModele));
			for(classe=0; classe<TRMT_NBCLASSES; classe++)
				if(config && config->defini.trmt[classe]) memcpy(&(VoieManip[voie].def.trmt[classe]),&(config->trmt[classe]),sizeof(TypeTraitement));
				else memcpy(&(VoieManip[voie].def.trmt[classe]),&(usuelle->trmt[classe]),sizeof(TypeTraitement));
			if(config && config->defini.trgr) memcpy(&(VoieManip[voie].def.trgr),&(config->trgr),sizeof(TypeTrigger));
			else memcpy(&(VoieManip[voie].def.trgr),&(usuelle->trgr),sizeof(TypeTrigger));
			if(config && config->defini.rgul) memcpy(&(VoieManip[voie].def.rgul),&(config->rgul),sizeof(TypeRegulParms));
			else memcpy(&(VoieManip[voie].def.rgul),&(usuelle->rgul),sizeof(TypeRegulParms));
			!!! manque les catg...
		} */
	/* resultat du traitement pre-trigger */
		VoieTampon[voie].traitees->nb = 0;
		VoieTampon[voie].traitees->prem = 0;
		if(VoieTampon[voie].traitees->dim) {
			if(VoieTampon[voie].traitees->max != VoieTampon[voie].traitees->dim) {
				if((log > 1) && VoieTampon[voie].traitees->t.rval) 
					printf("Libere VoieTampon[%d].traitees->t.rval @%08X pour %d donnees\n",voie,VoieTampon[voie].traitees->t.rval,VoieTampon[voie].traitees->max);
				if(VoieTampon[voie].traitees->t.rval) free(VoieTampon[voie].traitees->t.rval);
				VoieTampon[voie].traitees->t.rval = (TypeSignal *)malloc(VoieTampon[voie].traitees->dim * sizeof(TypeSignal));
				if(VoieTampon[voie].traitees->t.rval) VoieTampon[voie].traitees->max = VoieTampon[voie].traitees->dim;
				else {
					VoieTampon[voie].traitees->max = 0;
					VoieManip[voie].def.trmt[TRMT_PRETRG].type = NEANT;
				}
			}
			// printf("Creation %s traitee @%08X[%d]\n",VoieManip[voie].nom,(hexa)VoieTampon[voie].traitees->t.rval,VoieTampon[voie].traitees->max);
		}
		if(log > 1) printf("Alloue VoieTampon[%d].traitees->t.rval @%08X pour %d donnees\n",voie,VoieTampon[voie].traitees->t.rval,VoieTampon[voie].traitees->max);
        if(EvtASauver < 0) {
            if(VoieTampon[voie].brutes->type == TAMPON_INT16)
                printf("(%s) %s, tampon Sbrut cree (%08llx)\n",__func__,VoieManip[voie].nom,(IntAdrs)VoieTampon[voie].brutes->t.sval);
            else printf("(%s) %s, tampon Rbrut cree (%08llx)\n",__func__,VoieManip[voie].nom,(IntAdrs)VoieTampon[voie].brutes->t.rval);
            if(VoieTampon[voie].traitees->type == TAMPON_INT16)
                printf("(%s) %s, tampon Strig cree (%08llx)\n",__func__,VoieManip[voie].nom,(IntAdrs)VoieTampon[voie].traitees->t.sval);
            else printf("(%s) %s, tampon Rtrig cree (%08llx)\n",__func__,VoieManip[voie].nom,(IntAdrs)VoieTampon[voie].traitees->t.rval);
        }
	/* suivi du filtrage ou de la derivee */
		for(classe = 0; classe < TRMT_NBCLASSES; classe++) {
			if((VoieManip[voie].def.trmt[classe].type == TRMT_FILTRAGE) && VoieTampon[voie].trmt[classe].filtre) {
				if((VoieTampon[voie].trmt[classe].suivi.max != SettingsMaxSuivi) || (VoieTampon[voie].trmt[classe].suivi.nbetg != VoieTampon[voie].trmt[classe].filtre->nbetg)) {
					if((log > 1) && VoieTampon[voie].trmt[classe].suivi.brute) 
						printf("Libere VoieTampon[%d].trmt[%d].suivi.brute @%08X pour %d donnees\n",voie,classe,VoieTampon[voie].trmt[classe].suivi.brute,VoieTampon[voie].trmt[classe].suivi.max);
					if(VoieTampon[voie].trmt[classe].suivi.brute)
						free(VoieTampon[voie].trmt[classe].suivi.brute);
					VoieTampon[voie].trmt[classe].suivi.brute = (TypeDonneeFiltree *)malloc(SettingsMaxSuivi * sizeof(TypeDonneeFiltree));
					for(j=0; j<VoieTampon[voie].trmt[classe].filtre->nbetg; j++) {
						if((log > 1) && VoieTampon[voie].trmt[classe].suivi.filtree[j]) 
							printf("Libere VoieTampon[%d].trmt[%d].suivi.filtree[%d] @%08X pour %d donnees\n",voie,classe,j,VoieTampon[voie].trmt[classe].suivi.filtree[j],SettingsMaxSuivi);
						if(VoieTampon[voie].trmt[classe].suivi.filtree[j]) free(VoieTampon[voie].trmt[classe].suivi.filtree[j]);
						VoieTampon[voie].trmt[classe].suivi.filtree[j] = (TypeDonneeFiltree *)malloc(SettingsMaxSuivi * sizeof(TypeDonneeFiltree));
					}
					if(VoieTampon[voie].trmt[classe].suivi.filtree[--j]) {
						VoieTampon[voie].trmt[classe].suivi.max = SettingsMaxSuivi;
						VoieTampon[voie].trmt[classe].suivi.nbetg = VoieTampon[voie].trmt[classe].filtre->nbetg;
					} else {
						VoieTampon[voie].trmt[classe].suivi.max = 0;
						VoieTampon[voie].trmt[classe].suivi.nbetg = 0;
						VoieManip[voie].def.trmt[classe].type = NEANT;
					}
				}
			}
		}
	/* conditions du trigger dans les unites adaptees */
		if(VoieTampon[voie].trig.trgr->type != NEANT) {
			if(VoieTampon[voie].trig.trgr->type != TRGR_ALEAT) {
				/* Tous les nombres de points ci-dessous sont des nombres de points stockes */
				if(VoieManip[voie].def.evt.tempsmort > 0.0)
					VoieTampon[voie].trig.inhibe = VoieManip[voie].def.evt.tempsmort / VoieEvent[voie].horloge;
				else VoieTampon[voie].trig.inhibe = VoieEvent[voie].lngr - VoieEvent[voie].avant_evt;
				if(VoieTampon[voie].trig.inhibe < 1) VoieTampon[voie].trig.inhibe = 1;
				VoieTampon[voie].trig.sautlong = VoieManip[voie].def.trgr.alphaduree / VoieEvent[voie].horloge;
				VoieTampon[voie].trig.signe = VoieTampon[voie].trig.trgr->sens - 1;
				VoieTampon[voie].trig.mincount = (VoieTampon[voie].trig.trgr->montee / VoieEvent[voie].horloge); /* .montee est en ms */
				VoieTampon[voie].trig.mindelai = (VoieTampon[voie].trig.trgr->porte  / VoieEvent[voie].horloge); /* .porte  est en ms */
				VoieTampon[voie].trig.calcule = SettingsCalculEvt;
				if((VoieTampon[voie].trig.trgr->type == TRGR_DERIVEE) && (VoieTampon[voie].trig.calcule == TRMT_LDB))
					VoieTampon[voie].trig.calcule = TRMT_AMPLMAXI;
			} else VoieTampon[voie].trig.calcule = TRMT_LDB;
		}
	/* evenement moyen */
		if(VoieTampon[voie].moyen.calcule && !VoieTampon[voie].somme.val) {
			VoieTampon[voie].somme.val = (float *)malloc(VoieEvent[voie].lngr * sizeof(float));
			if(VoieTampon[voie].somme.val) VoieTampon[voie].somme.taille = VoieEvent[voie].lngr;
            else VoieTampon[voie].somme.taille = 0;
        }
	/* pattern moyen */
        if(VoieTampon[voie].pattern.dim) {
			if(VoieTampon[voie].pattern.max != VoieTampon[voie].pattern.dim) {
				if((log > 1) && VoieTampon[voie].pattern.val)
					printf("Libere VoieTampon[%d].pattern.val @%08X pour %d donnees\n",voie,VoieTampon[voie].pattern.val,VoieTampon[voie].pattern.max);
				if(VoieTampon[voie].pattern.val) free(VoieTampon[voie].pattern.val);
				if((log > 1) && VoieTampon[voie].pattern.nb)
					printf("Libere VoieTampon[%d].pattern.nb @%08X pour %d donnees\n",voie,VoieTampon[voie].pattern.nb,VoieTampon[voie].pattern.max);
				if(VoieTampon[voie].pattern.nb) free(VoieTampon[voie].pattern.nb);
				VoieTampon[voie].pattern.val = (float *)malloc(VoieTampon[voie].pattern.dim * sizeof(float));
				VoieTampon[voie].pattern.nb  = (float *)malloc(VoieTampon[voie].pattern.dim * sizeof(float));
				if(VoieTampon[voie].pattern.nb) VoieTampon[voie].pattern.max = VoieTampon[voie].pattern.dim;				
				else {
					if(VoieTampon[voie].pattern.val) {
						free(VoieTampon[voie].pattern.val);
						VoieTampon[voie].pattern.val = 0;
					}
					VoieTampon[voie].pattern.max = 0;
				}
				if(log > 1) printf("Alloue VoieTampon[%d].pattern.val @%08X pour %d donnees\n",voie,VoieTampon[voie].pattern.val,VoieTampon[voie].pattern.max);
			}
        }
	/* suivi des seuils */
		if(VoieTampon[voie].seuilmin.dim && (VoieTampon[voie].seuilmin.max == 0)) {
			VoieTampon[voie].seuilmin.val = (float *)malloc(VoieTampon[voie].seuilmin.dim * sizeof(float));
			if(VoieTampon[voie].seuilmin.val) VoieTampon[voie].seuilmin.max = VoieTampon[voie].seuilmin.dim;
		}
		if(VoieTampon[voie].seuilmax.dim && (VoieTampon[voie].seuilmax.max == 0)) {
			VoieTampon[voie].seuilmax.val = (float *)malloc(VoieTampon[voie].seuilmax.dim * sizeof(float));
			if(VoieTampon[voie].seuilmax.val) VoieTampon[voie].seuilmax.max = VoieTampon[voie].seuilmax.dim;
		}
	/* suivi des taux */
		if(VoieTampon[voie].tabtaux.dim && (VoieTampon[voie].tabtaux.max == 0)) {
			VoieTampon[voie].tabtaux.val = (float *)malloc(VoieTampon[voie].tabtaux.dim * sizeof(float));
			if(VoieTampon[voie].tabtaux.val) VoieTampon[voie].tabtaux.max = VoieTampon[voie].tabtaux.dim;
		}
	}

	/* suivi des taux par categorie */
	for(catg=0; catg<MonitCatgNb+1; catg++) {
		TypeMonitCatg *categ = MonitCatgPtr[catg];
		if(categ->utilisee && categ->activite.dim && (categ->activite.max == 0)) {
			if(categ->activite.val) { free(categ->activite.val); categ->activite.val = 0; }
			categ->activite.val = (float *)malloc(categ->activite.dim * sizeof(float));
			if(categ->activite.val) categ->activite.max = categ->activite.dim;
		}
	}

	/* Calcul des templates */
	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
		if(VoieManip[voie].def.trmt[TRMT_PRETRG].template == TRMT_TEMPL_DB) {
			if(log) nb_templates++;
			if(!TemplatesDecode(voie,0,nb_templates)) TemplatesCalcule(voie,nb_templates);
		} else if(VoieManip[voie].def.trmt[TRMT_PRETRG].template == TRMT_TEMPL_ANA) {
			if(log) nb_templates++;
			TemplatesCalcule(voie,nb_templates);
		} else if(VoieTampon[voie].trig.calcule == TRMT_EVTMOYEN) {
			if(log) nb_templates++;
			if(!TemplatesDecode(voie,0,nb_templates)) TemplatesCalcule(voie,nb_templates);
		} else {
			if(VoieManip[voie].gene >= 0) { TemplatesCalcule(voie,0); VoieTampon[voie].delai_template = 0; }
			if(log && (VoiesNb > 1)) nb_templates++;
			TemplatesAbsent(voie,nb_templates);
		}
		if((VoieTampon[voie].trig.calcule == TRMT_EVTMOYEN) && !VoieTampon[voie].unite.val) VoieTampon[voie].trig.calcule = TRMT_AMPLMAXI;
    }
	if(nb_templates) TemplateLogFin(nb_templates); else printf("  . Pas de filtrage par template\n");
		
#ifdef TESTE_MEMOIRE
    printf("Memoire libre prevue   : %11d octets\n",j);
    i = FreeMem();
    printf("Memoire libre apres    : %11d octets (perte: %d au cours de la verification)\n",i,j - i);
#endif
    if(manque) {
        printf("Il manque %ld/%ld octets => reduire les tampons a %g millisecondes\n",
            manque * sizeof(TypeDonnee),memoire * sizeof(TypeDonnee),
            DureeTampons * (float)(memoire - manque) / (float)memoire);
        OpiumFail("Trop de memoire demandee (voir journal)");
		LectBranche(1);
        return(0);
    }

#ifdef VISU_FONCTION_TRANSFERT
	TrmtHaffiche = 0;
#endif
	LectRazTampons(log);

	
	/*
	 * Graphiques utilisateur
	 */
	printf("\n");
	if(LectCntl.LectMode == LECT_DONNEES) {
		char affiche; int num;
		if(LectCompacteUtilisee && !LectMaintenance && !LectModeSpectresAuto) {
			int j,numx,numy,voie; TypeMonitFenetre *f; TypeMonitTrace *t; Graph g;
			TypeTamponDonnees *donnees;
			f = &LectFenSignal;
			f->axeY.i.min = LectOscilloSignal.marqueY[0]; f->axeY.i.max = LectOscilloSignal.marqueY[1];
			f->axeX.r.max = LectOscilloSignal.temps;
			if((g = MonitFenCreeGraphe(f,0))) {
				for(j=0, t=f->trace; j<f->nb; j++,t++) {
					voie = t->voie; numx = 2 * j; numy = numx + 1;
					donnees = &(VoieTampon[voie].trmt[t->var].donnees);
					GraphDataUse(g,numx,donnees->nb);
					GraphDataUse(g,numy,donnees->nb);
					GraphDataRescroll(g,numx,donnees->prem);
					GraphDataRescroll(g,numy,donnees->prem);
				}
			}
			OscilloChangeMinMax(&LectOscilloSignal);
			OscilloChangeTemps(&LectOscilloSignal);
			MonitFenCreeGraphe(&LectFenEvt,0);
			MonitFenCreeGraphe(&LectFenSpectre,0);
			MonitFenCreeGraphe(&LectFenBruit,0);
		}
		for(num=0; num<OscilloNb; num++) OscilloConnecte(OscilloReglage[num]);
		for(voie=0; voie<VoiesNb; voie++) {
			for(trmt=0; trmt<TRMT_NBCLASSES; trmt++) VoieTampon[voie].trmt[trmt].affichee = 0;
		}
		for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) {
			if(log > 1) printf(". Fenetre '%s': preparation\n",f->titre);
			MonitFenMemo(f);
			affiche = (LectMaintenance || LectModeSpectresAuto)? (f->affiche == MONIT_ALWS):
				((f->affiche == MONIT_SHOW) || (f->affiche == MONIT_ALWS));
			if(affiche) {
				if(MonitFenAffiche(f,1)) {
					if((f->type == MONIT_SIGNAL) || (f->type == MONIT_EVENT)) {
						TypeMonitTrace *t;
						for(j=0, t=f->trace; j<f->nb; j++,t++) {
							voie = t->voie; if(!VoieTampon[voie].lue) continue;
							trmt = t->var;
							VoieTampon[voie].trmt[trmt].affichee = 1;
						}
					}
				} else {
					if(f->nb) {
						TypeMonitTrace *t;
						printf("! Fenetre '%s' pas affichee:\n",f->titre);
						for(j=0, t=f->trace; j<f->nb; j++,t++) {
							voie = t->voie;
							if(!VoieTampon[voie].lue) printf("! . La voie %s n'est pas lue\n",VoieManip[voie].nom);
							else if(!VoieTampon[voie].trmt[t->var].donnees.t.data) printf("! . Pas de tampon de classe %d pour la voie %s\n",t->var,VoieManip[voie].nom);
//							switch(t->var) {
//							  case TRMT_AU_VOL:
//								if(!VoieTampon[voie].brutes->t.sval) printf("! . Pas de tampon brut pour la voie %s\n",VoieManip[voie].nom);
//								break;
//							  case TRMT_PRETRG:
//								if(!VoieTampon[voie].traitees->t.rval) printf("! . Pas de tampon traite pour la voie %s\n",VoieManip[voie].nom);
//								break;
//							}
						}
						OpiumWarn("Fenetre '%s' pas affichee (aucun tampon cree)",f->titre);
					} else OpiumWarn("Fenetre '%s' pas affichee (pas de trace demandee)",f->titre);
				}
			} else { MonitFenClear(f); MonitFenDetach(f); }
		}
	#ifdef NTUPLES_ONLINE
		if(CalcNtpDemande && (SettingsNtupleMax != NtDim)) {
			if(NtDim) { PlotNtupleSupprime(); NtDim = 0; }
			if(PlotNtupleCree(SettingsNtupleMax,0.0)) NtDim = SettingsNtupleMax;
		}
	#endif
		if(TrmtRegulActive && MonitAffgSeuils && gSeuils) /* suivi des seuils */ {
			int x,y,k;
			GraphErase(gSeuils);
			x = GraphAdd(gSeuils,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,LectSeuilsAb6,LectSeuilsMax);  /* 12 heures d'affichage des seuils, soit 720 points */
			GraphDataName(gSeuils,x,"t(h)");
			OpiumColorInit(&LectSeuilsCouleur);
			for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue && (VoieTampon[voie].trig.trgr->type != NEANT)) {
				k = OpiumColorNext(&LectSeuilsCouleur);
				if(VoieTampon[voie].seuilmin.val) {
					y = GraphAdd(gSeuils,GRF_FLOAT|GRF_YAXIS,VoieTampon[voie].seuilmin.val,LectSeuilsMax);
					GraphDataRGB(gSeuils,y,OpiumColorRGB(k));
					GraphDataName(gSeuils,y,VoieManip[voie].nom);
				}
				if(VoieTampon[voie].seuilmax.val) {
					y = GraphAdd(gSeuils,GRF_FLOAT|GRF_YAXIS,VoieTampon[voie].seuilmax.val,LectSeuilsMax);
					GraphDataRGB(gSeuils,y,OpiumColorRGB(k));
					if(!VoieTampon[voie].seuilmin.val) GraphDataName(gSeuils,y,VoieManip[voie].nom);
				}
			}
			GraphMode(gSeuils,GRF_2AXES | GRF_LEGEND);
			GraphUse(gSeuils,0);
			LectStocke1Seuil();
		}
		if(MonitAffgTaux && gTauxDetaille) /* suivi des taux */ {
			int x,y,k;
			GraphErase(gTauxDetaille);
			x = GraphAdd(gTauxDetaille,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,LectTauxAb6,LectTauxMax); /* 12 heures d'affichage des taux, soit 43200 points */
			GraphDataName(gTauxDetaille,x,"t(mn)"); GraphAxisTitle(gTauxDetaille,GRF_XAXIS,"t(mn)");
			OpiumColorInit(&LectTauxCouleur);
			for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue && (VoieTampon[voie].trig.trgr->type != NEANT)) {
				k = OpiumColorNext(&LectTauxCouleur);
				y = GraphAdd(gTauxDetaille,GRF_FLOAT|GRF_YAXIS,VoieTampon[voie].tabtaux.val,LectTauxMax);
				GraphDataRGB(gTauxDetaille,y,OpiumColorRGB(k));
				GraphDataName(gTauxDetaille,y,VoieManip[voie].nom);
				GraphDataTrace(gTauxDetaille,y,GRF_HST,0);
			}
			GraphAxisFloatMin(gTauxDetaille,GRF_YAXIS,0.0);
			// GraphAxisAutoMax(gTauxDetaille,GRF_YAXIS);
			GraphMode(gTauxDetaille,GRF_2AXES | GRF_LEGEND);
			GraphUse(gTauxDetaille,0);
		}

		if(MonitEvtClasses && !LectReglagesOuverts) LectPlancheSuivi(1);

		if(PicsNb && PicsActif) {
			int y,k; char a_refaire;
			a_refaire = (GraphDimGet(gPicsPositions) != (2 * PicsNb));
			for(k=0; k<PicsNb; k++) {
				if(Pic[k].dim != PicSuiviDim) {
					if(Pic[k].dim) {
						free(Pic[k].t); free(Pic[k].position); free(Pic[k].sigma);
						Pic[k].dim = 0;
						a_refaire = 1;
					}
					Pic[k].t = (float *)malloc(PicSuiviDim * sizeof(float));
					if(!Pic[k].t) continue;
					Pic[k].position = (float *)malloc(PicSuiviDim * sizeof(float));
					if(!Pic[k].position) { free(Pic[k].t); continue; }
					Pic[k].sigma = (float *)malloc(PicSuiviDim * sizeof(float));
					if(!Pic[k].sigma) { free(Pic[k].t); free(Pic[k].position); }
					else {
						Pic[k].dim = PicSuiviDim;
						a_refaire = 1;
					}
				}
			}
			if(a_refaire) {
				GraphErase(gPicsPositions);
				GraphErase(gPicsSigmas);
				for(k=0; k<PicsNb; k++) if(Pic[k].dim) {
					GraphAdd(gPicsPositions,GRF_XAXIS|GRF_FLOAT,Pic[k].t,Pic[k].dim);
					y = GraphAdd(gPicsPositions,GRF_YAXIS|GRF_FLOAT,Pic[k].position,Pic[k].dim);
					switch(k) {
						case 0: GraphDataRGB(gPicsPositions,y,GRF_RGB_GREEN); break;
						case 1: GraphDataRGB(gPicsPositions,y,GRF_RGB_YELLOW); break;
						case 2: GraphDataRGB(gPicsPositions,y,GRF_RGB_BLUE); break;
						case 3: GraphDataRGB(gPicsPositions,y,GRF_RGB_MAGENTA); break;
						default: GraphDataRGB(gPicsPositions,y,GRF_RGB_RED); break;
					}
					GraphAdd(gPicsSigmas,GRF_XAXIS|GRF_FLOAT,Pic[k].t,Pic[k].dim);
					y = GraphAdd(gPicsSigmas,GRF_YAXIS|GRF_FLOAT,Pic[k].sigma,Pic[k].dim);
					switch(k) {
						case 0: GraphDataRGB(gPicsSigmas,y,GRF_RGB_GREEN); break;
						case 1: GraphDataRGB(gPicsSigmas,y,GRF_RGB_YELLOW); break;
						case 2: GraphDataRGB(gPicsSigmas,y,GRF_RGB_BLUE); break;
						case 3: GraphDataRGB(gPicsSigmas,y,GRF_RGB_MAGENTA); break;
						default: GraphDataRGB(gPicsSigmas,y,GRF_RGB_RED); break;
					}
				}
			}
			if(PicsDim != SettingsAmplBins) {
				PicX = (float *)malloc(SettingsAmplBins * sizeof(float));
				if(PicX) {
					PicY = (float *)malloc(SettingsAmplBins * sizeof(float));
					if(!PicY) free(PicX); 
					else {
						PicV = (char *)malloc(SettingsAmplBins * sizeof(char));
						if(PicV) PicsDim = SettingsAmplBins;
						else { free(PicX); free(PicY); }
					}
				}
			}
		}	
	}
#ifdef TESTE_MEMOIRE
	i = FreeMem();
	printf("Memoire libre finale   : %11d octets\n",i);
	LectCurrMem = i;
#endif

	return(1);
}
/* ========================================================================== */
static void LectRazTampons(char log) {
	int voie,classe,gene,catg,i,j,k,nb;
	int max; int rep;
    
	SettingsDate = 0; /* pour les recherches dans la database */
	LectBlockSize = 0;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) {
		LectBlockSize += Repart[rep].nbdonnees; if(LectStatusPresent[rep]) LectBlockSize++;
		// printf("* Bloc #%-2d %s status, taille totale (apres selection): %d mots\n",rep,LectStatusPresent[rep]? "avec": "sans",LectBlockSize);
		LectMasqueStatus[rep] = 0;  /* a corriger apres identification, et si utile */
	}
	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
		if(log > 1) printf(". Voie %s: RAZ des tampons\n",VoieManip[voie].nom);
		DetecteurAdcCalib(voie,log?"":0);
		MonitTagsGlobal(voie,&(VoieManip[voie].def));
#ifdef AVANCE_COMPTEURS
		VoieTampon[voie].lus = COMPTEUR_0;                                   /* nombre total de points memorises    */
		if(VoieTampon[voie].lus > VoieTampon[voie].brutes->max) {
			VoieTampon[voie].brutes->nb = VoieTampon[voie].brutes->max;
			VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien = VoieTampon[voie].lus - VoieTampon[voie].brutes->max;  /* plus ancien point memorise */
			VoieTampon[voie].circulaire = 1;
		} else {
			VoieTampon[voie].brutes->nb = VoieTampon[voie].lus;               /* nombre de points utilisables */
			VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien = 0;                                      /* plus ancien point memorise   */
			VoieTampon[voie].circulaire = 0;
		}
		for(j=0; j<VoieTampon[voie].brutes->nb; j++) VoieTampon[voie].brutes->t.sval[j] = 0;
		VoieTampon[voie].brutes->prem = Modulo(VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien,VoieTampon[voie].brutes->max);  /* prochain point a memoriser */
		VoieTampon[voie].trmt[TRMT_AU_VOL].point0 = (VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien / (int64)VoieTampon[voie].brutes->max) * (int64)VoieTampon[voie].brutes->max;
		VoieTampon[voie].prochain = VoieTampon[voie].brutes->t.sval + VoieTampon[voie].brutes->prem;
		// VoieTampon[voie].lissage.prem = 0;
		VoieTampon[voie].trmt[TRMT_PRETRG].plus_ancien = 0;
		VoieTampon[voie].traitees->nb = COMPTEUR_0;
		if(VoieTampon[voie].traitees->nb > VoieTampon[voie].traitees->max) VoieTampon[voie].traitees->nb = VoieTampon[voie].traitees->max;
		if(VoieTampon[voie].traitees->max) VoieTampon[voie].traitees->prem = COMPTEUR_0 % VoieTampon[voie].traitees->max;
		else VoieTampon[voie].traitees->prem = 0;
		VoieTampon[voie].trmt[TRMT_PRETRG].point0 = VoieTampon[voie].trmt[TRMT_PRETRG].plus_ancien - VoieTampon[voie].traitees->prem;
		for(j=0; j<VoieTampon[voie].traitees->nb; j++) VoieTampon[voie].traitees->t.rval[j] = 0;
#else
		VoieTampon[voie].lus = 0;                   /* nombre total de points memorises    */
		VoieTampon[voie].brutes->nb = 0;             /* nombre de points utilisables        */
		VoieTampon[voie].circulaire = 0;
		VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien = 0;                 /* premier point memorise              */
		VoieTampon[voie].brutes->prem = 0;           /* prochain point a memoriser          */
		VoieTampon[voie].trmt[TRMT_AU_VOL].point0 = 0;
		VoieTampon[voie].prochain_s16 = VoieTampon[voie].brutes->t.sval + VoieTampon[voie].brutes->prem;
		VoieTampon[voie].prochain_i32 = VoieTampon[voie].brutes->t.ival + VoieTampon[voie].brutes->prem;
		// VoieTampon[voie].lissage.prem = 0;
		VoieTampon[voie].traitees->nb = 0;
		VoieTampon[voie].traitees->prem = 0;
		VoieTampon[voie].trmt[TRMT_PRETRG].plus_ancien = 0;
		VoieTampon[voie].traitees->prem = 0;
		VoieTampon[voie].trmt[TRMT_PRETRG].point0 = VoieTampon[voie].trmt[TRMT_PRETRG].plus_ancien - VoieTampon[voie].traitees->prem;
#endif
		for(j=0; j<VoieTampon[voie].brutes->max; j++) {
			if(VoieTampon[voie].brutes->type == TAMPON_INT16) VoieTampon[voie].brutes->t.sval[j] = 0;
			else VoieTampon[voie].brutes->t.ival[j] = 0;
		}
		for(j=0; j<VoieTampon[voie].traitees->max; j++) VoieTampon[voie].traitees->t.rval[j] = 0;
		VoieTampon[voie].cree_traitee = 1; /* a priori (pour LectBuffStatus), refait dans TrmtRAZ() */
		VoieTampon[voie].signal.nb = 0;
		VoieTampon[voie].signal.mesure = 0;
		VoieTampon[voie].signal.genere_evt = 0;
		VoieTampon[voie].moyen.nb = 0;
		VoieTampon[voie].moyen.decal = 0.0;
		if(VoieTampon[voie].moyen.calcule && VoieTampon[voie].somme.val) {
			nb = VoieTampon[voie].somme.taille;
			for(i=0; i<nb; i++) VoieTampon[voie].somme.val[i] = 0.0;
		}
		VoieTampon[voie].pattern.total = (int)VoieManip[voie].duree.periodes * VoieTampon[voie].pattern.max;
		if(VoieTampon[voie].pattern.val) {
			for(i=0; i<VoieTampon[voie].pattern.max; i++) {
				VoieTampon[voie].pattern.val[i] = 0.0;
				VoieTampon[voie].pattern.nb[i] = 0.0;
			}
		}

		VoieTampon[voie].seuilmin.nb = VoieTampon[voie].seuilmin.prem = 0;
		VoieTampon[voie].seuilmax.nb = VoieTampon[voie].seuilmax.prem = 0;
		VoieTampon[voie].tabtaux.nb = VoieTampon[voie].tabtaux.prem = 0;
		VoieTampon[voie].sauve = 0;         /* dernier point traite                */
		VoieTampon[voie].jetees = 0;
		VoieTampon[voie].date_dernier = 0;
		VoieTampon[voie].date_ref_taux = 0;
		VoieTampon[voie].dernier_evt = VoieTampon[voie].evt_affiche = -1;
        for(classe = 0; classe < TRMT_NBCLASSES; classe++) {
            VoieTampon[voie].trmt[classe].compte = 0;
            VoieTampon[voie].trmt[classe].reelle = 0.0;
            VoieTampon[voie].trmt[classe].valeur = 0;
            VoieTampon[voie].trmt[classe].phase = 0;
            if(VoieManip[voie].def.trmt[classe].type == TRMT_FILTRAGE) {
                VoieTampon[voie].trmt[classe].suivi.nb = 0;
                VoieTampon[voie].trmt[classe].suivi.prems = 0;
            }
            VoieTampon[voie].trmt[classe].point_affiche = -1;
			if(LectureLog > 1) printf(". Voie %s: tampon des donnees %ss: de type %d\n",VoieManip[voie].nom,TrmtClassesListe[classe],VoieTampon[voie].trmt[classe].donnees.type);
        }

#ifdef VERIFIE_ALLOCS
        printf("Prochain V%d @%08X\n",voie,VoieTampon[voie].prochain);
#endif
	}
	if(Acquis[AcquisLocale].etat.active && (LectCntl.LectMode != LECT_COMP)) printf("%s/ Les tampons ont ete vides\n",DateHeure());

	LectBoloDemande = BoloNum;
	for(i=0; i<ModeleVoieNb; i++) LectBase1Niveau[i] = LectBase1Bruit[i] = 0.0;
	for(i=0; i<ModeleVoieNb; i++) {
		for(j=0; j<BoloNb; j++) LectBaseNiveau[i][j] = LectBaseBruit[i][j] = 0.0;
	}
	max = 0;
	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
		j = VoieManip[voie].det;
		if(max < j) max = j;
	}
	GraphAxisIntRange(gLectBaseNiveaux,GRF_XAXIS,0,max); /* (pourquoi c'est pas automatique?) */
	GraphAxisIntRange(gLectBaseBruits,GRF_XAXIS,0,max); /* (pourquoi c'est pas automatique?) */
	max++;
	GraphUse(gLectBaseNiveaux,max);
	GraphUse(gLectBaseBruits,max);

	for(catg=0; catg<MonitCatgNb+1; catg++) if(MonitCatgPtr[catg]->utilisee) MonitCatgPtr[catg]->activite.nb = 0;
	for(gene=0; gene<GeneNb; gene++) {
		Gene[gene].evts = 0;
		if(OpiumAlEcran(Gene[gene].panneau.evts)) PanelRefreshVars(Gene[gene].panneau.evts);
	}
	for(k=0; k<PicsNb; k++) { Pic[k].prems = Pic[k].nb = 0; }
	HistoDataRaz(hdEvtAmpl);

	if(SambaInfos) {
		int plot; for(plot=0; plot<SAMBA_SHR_PLOTNB; plot++) {
			SambaInfos->courbe[plot].prem = 0;
			SambaInfos->courbe[plot].nb = 0;
			SambaInfos->courbe[plot].max = 0.0;
		}
	}
}
/* ========================================================================== */
static void LectFixeIntervalles() {
	int k,gene;

/* Reperes internes independants des voies, sauf que l'echantillonnage devrait dependre des voies vraiment lues */
	if(!SambaMaitre) RepartAjusteHorloge(&Echantillonnage,&RepartUdpRelance);
	// Opera: LectIntervRelance = 12000.0 / Echantillonnage; /* synchro=code_synchro_100000, soit 100000 echantillons par relance UDP */
	LectIntervRelance = (long)RepartUdpRelance;
	LectChangeDureePartitions(0,0);
	if(VoiesLocalesNb) {
		Diviseur2 = (Diviseur2 / 2) * 2;  /* Il faut une valeur paire, sinon ca deconne de partout */
		if(Diviseur2 < 1) Diviseur2 = 1;  /* ... sauf pour NOSTOS !! */
		BlocsParD3 = Diviseur2 * Diviseur3;
	}
	// printf("(%s) Echantillonnage retenu: %g kHz\n",__func__,Echantillonnage);
	EnSecondes = 0.001 / Echantillonnage;
	for(gene=0; gene<GeneNb; gene++) {
		Gene[gene].periode = 1.4 * Echantillonnage / (Gene[gene].freq / 1000.0);
	}
	SynchroD2_us = (int64)((float)(Diviseur2 * 1000) / Echantillonnage);     /* duree d'une synchronisation D2 en us     */
	SynchroD2_kHz = Echantillonnage / (float)Diviseur2;                      /* frequence d'une synchro D2 en kHz        */
	LectIntervTrmt   = (int64)((float)SettingsIntervTrmt * SynchroD2_kHz);  /* Intervalle entre traitement des donnees  */
	for(k=0; k<ExportPackNb; k++) if(!ExportPack[k].quand) {
		if(ExportPack[k].periode) LectIntervExport[k] = (int64)((float)ExportPack[k].periode * SynchroD2_kHz); /* Intervalle entre exportations            */
		else ExportPack[k].periode = EXPORT_RUN_DEBUT;
	}
	LectIntervSeq    = (int64)((float)SettingsIntervSeq * SynchroD2_kHz);    /* Intervalle entre sequencements           */
	LectIntervUser   = (int64)((float)SettingsIntervUser * SynchroD2_kHz);   /* Intervalle entre actions user            */
	LectIntervData   = (int64)((float)MonitIntervAff * SynchroD2_kHz);       /* Intervalle entre affichages              */
	LectIntervSat    = (int64)(5000 * SynchroD2_kHz);                        /* Intervalle entre requete etat satellites */
	LectMaxAbsent = 200;
}
/* ========================================================================== */
static void LectRazPartiel() {
	int voie,vt,fmt; int j,k;

	LectFixeIntervalles();
    /*	Remise a zero des differents compteurs generaux */
	if(LectureLog > 1) printf(". Remise a zero des differents compteurs generaux\n");
	LectPid = 0;
	LectureSuspendue = 0;
	LectDepileNb = 0;
	LectDepileTestees = 0;
	LectMaxDepilees = 0;
	LectSerieVide = 0;
	TrmtRegulModifs = 0;
	TrmtTempsFiltrage = TrmtTempsArchive = 0;
#ifdef DEBUG_ACCES
	LectPciAdrs = 0;
#endif
#ifdef DEBUG_DEPILE
	LectDepileStockes = 0;
#endif
#ifdef DEBUG_STATUS_MIG
	LectStatusNb = 0;
#endif
#ifdef DEBUG_FILL
	LectFillNb = LectFillMax = LectFillMax2 = 0;
#endif
#ifdef DEBUG_RAW
	LectRawNext = 0;
	LectNonNul = 0;
#endif
	MonitEvtAlEcran = -1;

	LectStampsLus = COMPTEUR_0;   /* nombre total de blocs lus  sur FIFO    */
	SynchroD2lues = COMPTEUR_0 / Diviseur2;    /* nombre de synchros lues sur FIFO       */
	TempsTrigger[0] = '\0';
	ArchStampsSauves = COMPTEUR_0;  /* blocs deja sauves en mode stream       */
	ArchStreamSauve = 0;
	bzero(&LectEnCours,sizeof(LectEnCours));
	for(fmt=0; fmt<ARCH_TYPEDATA; fmt++) ArchTrancheReste[fmt] = ArchTrancheTaille;
	LectNextTrmt   = LectAttendIdent? 1: SynchroD2lues + LectIntervTrmt;
	for(k=0; k<ExportPackNb; k++) if(!ExportPack[k].quand) LectNextExport[k] = SynchroD2lues + LectIntervExport[k];
	LectNextSeq    = SynchroD2lues + LectIntervSeq;
	LectNextUser   = SynchroD2lues + LectIntervUser;
	LectNextDisp   = SynchroD2lues; // + LectIntervData;
	LectNextSat    = SynchroD2lues + LectIntervSat;
	LectRecaleD3 = 0;
	LectDerniereD3 = 0;
#ifdef DEBUG_STATUS_BBV2
	IndexStatusCode = IndexStatusDecode = 0;
#endif
	TousLesEvts.utilises = 0; TousLesEvts.stamp_utilise = 0;
	gettimeofday(&CalcDateSpectre,0);
	gettimeofday(&LectDateRun,0);
	if(SettingsTauxPeriode) LectProchainTaux = LectDateRun.tv_sec + (long)SettingsTauxPeriode;
	else LectProchainTaux = 0;
	for(voie=0; voie<VoiesNb; voie++) {
		VoieTampon[voie].nbevts = 0;
		if(VoieManip[voie].def.rgul.type == TRMT_REGUL_TAUX) {
			for(j=0; j<MAXECHELLES; j++) VoieTampon[voie].rgul.taux.prochaine[j] = LectDateRun.tv_sec + (long)(VoieManip[voie].echelle[j].duree * 60);
		}
	}
	for(vt=0; vt<SambaEchantillonLngr; vt++) SambaEchantillon[vt].precedente = 0;

	if(LectureLog > 1) printf(". Lecture de donnees initialisee\n");
		
    return;
}
/* ========================================================================== */
static void LectRazCompteurs() {
	int j,sat,fmt;
	
	LectRazPartiel();
	CpuTempoLect = CpuTempoTrmt = CpuTempoArch = 0;
	CpuTotalLect = CpuTotalTrmt = CpuTotalArch = 0;
	CpuTauxOccupe = 0;
//	if(VerifTempsPasse) InstrumILimits(cCpuTauxOccupe,0,100);
//	else if(VerifConsoCpu) InstrumILimits(cCpuTauxOccupe,0,400);
	if(cCpuTauxOccupe) OpiumRefreshInstrum(cCpuTauxOccupe->cdr);
	
	TempsTempoLect = TempsTempoTrmt = TempsTempoArch = 0;
	TempsTotalLect = 0; TempsTotalDispo = 0;
	CpuMaxTrigger = CpuMaxEvt = 0;
	CpuStampTrigger = CpuFinTrigger = CpuStampEvt = CpuFinEvt = 0;
	for(j=0; j<INTERF_NBTYPES; j++) LectUsageInterface[j] = 0;
	LectUsageInterfacesToutes = 0;
	RepartPci.depilees = RepartIp.depilees = 0;
	RepartIp.traitees = RepartIp.perdues = 0;
	LectStampSautes = LectStampPerdus = LectStampJetes = LectValZappees = 0;
	LectSyncRead = ArchSyncWrite = 0;
	PerteIP = TempsMort = PerteEvts = 0.0;
	RepartIpEchappees = 0;
	RepartIp.reduc = 0;
	ArchEvtsVides = ArchEvtsPerdus = 0;
	LectCntl.MonitEvtNum = 0;
	LectPaniqueAffichee = 1;
	LectAffichee[0] = '\0';
	MonitP0 = 0; MonitT0 = 0.0;
	LectTimeStamp0 = LectTimeStampN = 0;
	for(fmt=0; fmt<ARCH_TYPEDATA; fmt++) ArchEcrites[fmt] = 0;      /* evenements deja sauves dans la tranche */
	for(sat=0; sat<AcquisNb; sat++) {
		Acquis[sat].etat.status = SATELLITE_ATTENTE;
		Acquis[sat].etat.active = 0;
		Acquis[sat].etat.duree_lue[0] = '\0';
		Acquis[sat].etat.evt_trouves = 0;
		Acquis[sat].etat.evt_ecrits = 0;
		Acquis[sat].etat.KgJ = 0.0;
	}
}
/* ========================================================================== */
static char LectAccrocheD3(char starter_declare) {
	int rep; TypeRepartiteur *repart;
	char eligible[REPART_MAX]; char elu;
	
	for(rep=0; rep<RepartNb; rep++) eligible[rep] = Repart[rep].actif; // && Repart[rep].nbentrees
	elu = 0;
	until(elu) {
		/* Choix d'un repartiteur chez qui je vais detecter une synchro D3 */
		for(rep=0; rep<RepartNb; rep++) if((Repart[rep].categ == DONNEES_STAMP) && eligible[rep]) break;
		if(rep >= RepartNb) for(rep=0; rep<RepartNb; rep++) if(eligible[rep]) {
			if(Repart[rep].interf == INTERF_IP) break;
			else if(Repart[rep].interf == INTERF_PCI) {
				if(LectStatusPresent[rep]) break;
			}
		}
		if(rep >= RepartNb) { OpiumError("Pas de repartiteur capable de donner une synchro D3"); break; }
		else {
			repart = &(Repart[rep]);
			RepartiteurStoppeTransmissions(repart,LectureLog);
			if((repart->famille == FAMILLE_IPE) && SettingsIPEwait) {
				printf("%s/ Attente du vidage pepere des FIFOs IPE: %d seconde%s...\n",DateHeure(),Accord1s(SettingsIPEwait));
				sleep(SettingsIPEwait);
			}
			unless(RepartiteurDemarreTransmissions(repart,LectureLog)) {
				printf("%s/ !!! Demarrage impossible. Acquisition stoppee\n",DateHeure());
				return(0);
			}
			unless(RepartiteurRepereD3(repart,&LectEnCours.valeur,LectureLog)) {
				printf("%s/ !!! Synchro D3 absente chez ce repartiteur, du coup il est declasse\n",DateHeure());
				unless(OpiumChoix(2,SambaNonOui,"Synchro D3 absente dans la %s %s => autre repartiteur utilise?",ModeleRep[repart->type].nom,repart->nom)) {
					printf("%s/ !!! Demarrage prevu impossible. Acquisition stoppee\n",DateHeure());
					return(0);
				}
				eligible[rep] = 0;
			} else {
				elu = 1;
				if(RepartNbActifs > 1) {
					if(LectureLog) printf("%s/ . Synchro D3 initiale trouvee: %lld/%d\n",DateHeure(),repart->s.ip.stamp0,repart->s.ip.num0);
					RepartiteurStoppeTransmissions(repart,LectureLog);
					if((repart->famille == FAMILLE_IPE) && SettingsIPEwait) {
						printf("%s/ Attente du vidage pepere des FIFOs IPE: %d seconde%s...\n",DateHeure(),Accord1s(SettingsIPEwait));
						sleep(SettingsIPEwait);
					}
				} else {
					if(LectureLog) printf("%s/ . Demarrage sur la valeur %08X au timestamp %lld/%d\n",DateHeure(),LectEnCours.valeur,repart->s.ip.stampDernier,repart->s.ip.numAVenir-1);
					LectSynchroReperee = 1;
				}
				/* Suite de la procedure selon etendue de la synchro D3 */
				if(LectAcqCollective && (LectSession < 2)) {
					int j;
					LectStampMini = repart->s.ip.stamp0 + (2 * repart->s.ip.stampD3);
					if(LectureLog) printf("%s/ . Synchro D3 minimale demandee: %lld\n",DateHeure(),LectStampMini);
					for(j=0; j<Partit[PartitLocale].nbsat; j++) if(Partit[PartitLocale].exter[j]) {
						SambaEcritSatLong(Partit[PartitLocale].sat[j],SMB_M,LectStampMini);
					}
					RepartD3Attendues = 1;
				} else if(RepartNbActifs > 1) SambaAttends(SettingsD3wait); /* ms */
				if(starter_declare) /* un starter a ete defini: synchro globale niveau manip */ {
					/* je suis starter manip, sinon je ne serais pas la... */
					/* demander donc aux satellites de demarrer a ce moment meme (cf DetecteurListeSat) */
					int sat; byte msg_ip[4];
					msg_ip[0] = 'D'; msg_ip[1] = '\0';
					for(sat=0; sat<AcquisNb; sat++) if(sat != AcquisNb) {
						if(LectureLog) printf("%s/ . Demarrage demande a %s\n",DateHeure(),Acquis[sat].code);
						send(Acquis[sat].synchro.path,msg_ip,2,0);
					}
				}
			}
		}
	}
	if(OpiumAlEcran(pRepartXmit)) PanelRefreshVars(pRepartXmit);
	return(1);
}
#define UDP_SYNC_LNGR 4
/* ========================================================================== */
static char LectSynchro(NUMER_MODE mode) {
/* Vide la pile (PCI ou IP) jusqu'a l'apparition du debut de la synchro (D2 ou D3) */
	char starter_declare,attente_ipe;
	byte texte[80];
	int i,k,l,rep,dep,dim,nb_actifs,nb_fifos,nb_val_ip,depuis_reset;
	int64 d3_initial;
	float v;
	char rc,rep_muet;
	TypeRepartiteur *repart,*ze_repart; TypeNumeriseur *numeriseur;
	byte msg_ip[UDP_SYNC_LNGR]; hexa lngr; int lu;
#ifdef ACCES_PCI_DIRECT
	int examuser,numpt;
#endif

	/* {
		int rep;
		printf("| Etat des repartiteurs au debut de %s\n",__func__);
		for(rep=0; rep<RepartNb; rep++) printf("| %16s: %s\n",Repart[rep].nom,Repart[rep].actif? "actif": "arrete");
	} */

	/* Mise a jour des reperes pour la routine sous interuption */
	if(RepartNbActifs == 0) {
		OpiumFail("Pas de repartiteur active. La lecture n'a pas ete lancee");
		return(0);
	}
	unless(RepartNbActifs) printf("        ! Bizarre... aucun repartiteur n'a ete active!!\n");
	ze_repart = &(Repart[0]);

	if(SequenceRegenAsuivre && !RegenEnCours) { LectRegenLance(1); SequenceRegenAsuivre = 0; }

	HasardInit;

	/* TimerStart(LectTaskReadout,SettingsReadoutPeriod*1000); pour tenir compte d'un eventuel changement de frequence */

	bzero(&LectEnCours,sizeof(LectEnCours));
	ArchHorsDelai = 0;
	ArchExplics[0] = '\0';
	LectAvantSynchroD2 = Diviseur2;
	LectTypePrevu = CLUZEL_MARQUE_NEANT;
	LectSynchroD3 = 0;
	// LectErrNb = 0;
	ArchAttendsD3 = 1;
	RepartPci.vide = RepartPci.halffull = RepartPci.fullfull = 0;
	RepartIp.acces = RepartIp.vide = 0; RepartIpOctetsLus = 0;
	LectEpisodeEnCours = 0;
	RepartPci.date = RepartPci.copie = 0;
	RepartPci.delai = 1000000;
	ArchT0sec = 0;
	ArchT0msec = 0;
	LectStatusDispo = 0;
	repart = ze_repart; v = 0.0; // GCC
	// if(!VoiesLocalesNb) return(1);
	
	rc = 1;
#ifdef OBSOLETE
	/* Synchro pour simulation ou relecture fichier */
	if(LectSurFichier) {
		if(LectParBloc) { if(LectArchLit() <= 0) return(0); /* transformer en LECT_EOF si 0 */ }
		Repart[RepartFile].bottom = 0;
		/* rc = Lect1Val();
		Repart[RepartFile].data16[Repart[RepartFile].bottom] = (LectEnCours.valeur & 0xFFFF);
		Repart[RepartFile].top = Repart[RepartFile].bottom + 1; */
		Repart[RepartFile].top = 0;
		#ifndef CODE_WARRIOR_VSN
		gettimeofday(&LectDateRun,0);
		#endif
		if(LectureLog) printf("%s/ Debut relecture\n",DateHeure());
		LectArchDuree = 0;

	/* Lecture reelle */
	} else 
#endif
		if(LectSurFichier) {
			for(rep=0; rep<RepartNb; rep++) Repart[rep].simule = 1;
		}
		if(LectDemarreBolo) {
			int bolo,b; short derniere; char a_suivre;
			printf("%s/ ============================== Demarrage des detecteurs ==============================\n",DateHeure());
			for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire && ((b = Bolo[bolo].start.bloc) >= 0)) {
				Bolo[bolo].exec.inst = DetecScriptsLibrairie.bloc[b].premiere;
				ScriptBoucleVide(&(Bolo[bolo].exec.boucle)); Bolo[bolo].exec.date = -1;
				printf("%s/ Detecteur %s: activation du script de demarrage '%s' @%d-%d\n",DateHeure(),Bolo[bolo].nom,Bolo[bolo].start.script,
				DetecScriptsLibrairie.bloc[b].premiere,DetecScriptsLibrairie.bloc[b].derniere);
			}
			ScriptKill = 0;
			repeter {
				gettimeofday(&LectDateRun,0); a_suivre = 0;
				for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire && ((b = Bolo[bolo].start.bloc) >= 0)) {
					derniere = DetecScriptsLibrairie.bloc[b].derniere;
					if(Bolo[bolo].exec.inst < derniere) {
						if(Bolo[bolo].exec.date <= LectDateRun.tv_sec) {
							short secs;
							// printf("%s/ Detecteur %s: execution des instructions [%d, %d[\n",DateHeure(),Bolo[bolo].nom,Bolo[bolo].exec.inst,derniere);
							while(Bolo[bolo].exec.inst < derniere) {
								ScriptExecBatch(DetecScriptsLibrairie.action,HW_DETEC,(void *)&(Bolo[bolo]),&(Bolo[bolo].exec),derniere,&secs,"");
								if(secs > 0) {
									Bolo[bolo].exec.date = LectDateRun.tv_sec + secs;
									// printf("%s/ Detecteur %s: prochaine execution a %ld s\n",DateHeure(),Bolo[bolo].nom,Bolo[bolo].exec.date);
									break;
								}
							}
							NumeriseurChargeFin(ScriptMarge());
							if(Bolo[bolo].exec.inst >= derniere) printf("%s/ Detecteur %s: script d'entretien @%d/%d termine\n",DateHeure(),
								Bolo[bolo].nom,Bolo[bolo].exec.inst,derniere);
						}
						if(Bolo[bolo].exec.inst < derniere) a_suivre = 1;
					}
				}
				if(a_suivre) sleep(1); else break;
			} indefiniment;
			ScriptKill = 0;
			printf("%s/ ============================== Fin du demarrage des detecteurs ==============================\n",DateHeure());
		} else printf("%s/ Pas de demarrage de detecteur\n",DateHeure());

	if(SambaMaitre) {
		if(!LectAccrocheD3(0)) return(0);
		for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) { ze_repart = &(Repart[rep]); break; }
	} else {
		/* Arret de tous les repartiteurs, quoiqu'il en soit du futur */
		if(LectureLog) printf("%s/ Arret de tous les repartiteurs\n",DateHeure());
		attente_ipe = 0;
		for(rep=0; rep<RepartNb; rep++) {
			int l;
			repart = &(Repart[rep]);
			if(!repart->recopie) {
				if(repart->actif) {
					RepartiteurStoppeTransmissions(repart,LectureLog);
					if(repart->famille == FAMILLE_IPE) attente_ipe = SettingsIPEwait;
				} else if(Repart[rep].local && LectureLog) {
					printf("          . %s",repart->nom);
					if(repart->interf == INTERF_PCI) printf(" @PCI #%d",repart->adrs.val);
					else if(repart->interf == INTERF_IP) printf(" @IP %d.%d.%d.%d",IP_CHAMPS(repart->adrs));
					else if(repart->interf == INTERF_NAT) printf("#%d",repart->adrs.val);
					else printf(" [non supporte!!]");
					printf(": hors service (%d numeriseur%s associe%s)\n",Accord2s(repart->nbentrees));
				}
			}
			for(dep=0; dep<REPART_RESERVOIR_MAX; dep++) {
				repart->depot[dep].reserve = repart->depot[dep].fond = 0;
				repart->depot_ancien = repart->depot_recent = 0;
			}
			repart->top = repart->bottom = 0;
			repart->depile = 0;
			repart->D3trouvee = 0;
			repart->acces_demandes = repart->donnees_recues = repart->acces_remplis = repart->acces_vides = repart->serie_vide = 0;
			repart->dernier_vide = repart->dernier_rempli = 0;
			repart->arrete = 0;
			for(l=0; l<repart->nbentrees; l++) {
				if((numeriseur = (TypeNumeriseur *)repart->entree[l].adrs)) numeriseur->enreg_stamp = 0;
			}
			if(repart->interf == INTERF_IP) RepartIpRazCompteurs(repart);
		}
		if(OpiumAlEcran(pRepartXmit)) PanelRefreshVars(pRepartXmit);
		if(attente_ipe) {
			printf("%s/ Attente du vidage pepere des FIFOs IPE: %d seconde%s...\n",DateHeure(),Accord1s(attente_ipe));
			sleep(attente_ipe);
		}
//		if(IPdemande) {
//			printf("%s/ Attente de la fin des transmissions via IP\n",DateHeure());
//			SambaAttends(1000); /* ms */ /* le temps de la boite OPERA se rende compte (i.e. a chaque D3) */
//		}
		if(LectureLog) {
			printf("%s/ FIFO interne disponible:\n",DateHeure());
			for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) {
				repart = &(Repart[rep]);
				//+ v = (float)(repart->maxdonnees / repart->nbdonnees) / Echantillonnage;
				//+ printf("          . %s: %8d valeur%s (validite: %.3f ms, lecture: %.1f/100 FIFO)\n",repart->nom,repart->maxdonnees,v,100.0*(float)SettingsReadoutPeriod/v);
				printf("          . pour %s: %d valeur%s\n",repart->nom,Accord1s(repart->maxdonnees));
			}
		}
 
		/* Decompte de la FIFO disponible */
		if(LectureLog) printf("%s/ FIFO externe disponible:\n",DateHeure());
		FifoTotale = 0; PCIdemande = 0; IPdemande = 0; IPactifs = 0; nb_fifos = 0; nb_val_ip = 0;
		for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) {
			repart = &(Repart[rep]);
			LectStatusPresent[rep] = (ModeleRep[repart->type].status && !LectSurFichier);
			if(repart->interf == INTERF_PCI) {
				PCIdemande = 1;
				if(repart->nbdonnees) {
					v = (float)(repart->depilable / repart->nbdonnees) / Echantillonnage;
					if(LectureLog) printf("          . FIFO PCI , %8d valeur%s (validite: %.3f ms, lecture: %5.1f/100 FIFO)   [%s @PCI #%d]\n",
						Accord1s(repart->depilable),v,100.0*(float)SettingsReadoutPeriod/v,ModeleRep[repart->type].nom,repart->adrs.val);
				} else if(LectureLog) printf("          . FIFO PCI , %8d valeur%s [%s @PCI #%d]\n",
					Accord1s(repart->depilable),ModeleRep[repart->type].nom,repart->adrs.val);
				FifoTotale += repart->depilable;
				RepartPci.delai = (int64)v * 500; /* validite_ms * 1000 * (2 / 4) */
				nb_fifos++;
			} else if(repart->interf == INTERF_IP) { nb_val_ip += repart->nbdonnees; IPdemande = 1; IPactifs++; nb_fifos++; }
		}
	#ifdef macintosh
		dim = FifoIP / sizeof(TypeADU);
		if(nb_val_ip) {
			v = (float)(dim / nb_val_ip) / Echantillonnage;
			if(LectureLog) printf("           %s FIFO IP%s, %8d valeur%s (validite: %.3f ms, lecture: %.1f/100 FIFO)\n",PCIdemande? " .":"",(nb_fifos > 1)?"  ":"",
				Accord1s(dim),v,100.0*(float)SettingsReadoutPeriod/v);
			FifoTotale += dim;
		} else if(LectureLog) printf("           %s FIFO IP%s, %8d valeur%s\n",PCIdemande? " .":"",(nb_fifos > 1)?"  ":"",Accord1s(dim));
	#endif
		if((nb_fifos > 1) && LectureLog) printf("          FIFO totale: %8d valeurs\n",FifoTotale);
		if(IPdemande) {
			LectUdpStatus(&LectUdpRecues,&LectUdpErr,&LectUdpOvr);
			printf("          Trames UDP jusque la recues: %lld, en erreur: %lld, jetees: %lld\n",LectUdpRecues,LectUdpErr,LectUdpOvr);
		}
		
		/* Demarrage ceans de tous les numeriseurs geres par cet ordi */
		if(mode == NUMER_MODE_IDEM) {
			if(LectureLog) printf("        * L'etat des numeriseurs est laisse inchange\n",DateHeure());
		} else {
			if(LectureLog) printf("%s/ Demarrage de tous les numeriseurs en mode %s\n",DateHeure(),NumerModeTexte[mode]);
			for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif && !Repart[rep].recopie) {
				repart = &(Repart[rep]);
				if(repart->bb || (mode == NUMER_MODE_IDENT)) {
					if(repart->famille == FAMILLE_CLUZEL) {
						if(ModeleRep[repart->type].version < 2.0) {
							if(LectureLog) printf("%s/ . Demarrage de tous les numeriseurs de la %s @PCI #%d\n",
								DateHeure(),ModeleRep[repart->type].nom,repart->adrs.val);
							RepartiteurDemarreNumeriseurs(repart,mode,NUMER_DIFFUSION,LectureLog);
						} else {
							int l;
							if(LectureLog) printf("%s/ . Demarrage des numeriseurs locaux de la %s @PCI #%d\n",
								DateHeure(),ModeleRep[repart->type].nom,repart->adrs.val);
							for(l=0; l<repart->nbentrees; l++) if((numeriseur = (TypeNumeriseur *)(repart->entree[l].adrs))) {
								RepartiteurDemarreNumeriseurs(repart,mode,numeriseur->def.adrs,LectureLog);
							}
						}
					} else {
						if(LectureLog) printf("%s/ . Demarrage de tous les numeriseurs lus par %s\n",DateHeure(),repart->nom);
						RepartiteurDemarreNumeriseurs(repart,mode,NUMER_DIFFUSION,LectureLog);
					}
				} else if(LectureLog) printf("%s/ . Le repartiteur %s n'a pas de 'Boite Bolo' a demarrer\n",DateHeure(),repart->nom);
			}
		}
			
		for(rep=0; rep<RepartNb; rep++) if((Repart[rep].famille == FAMILLE_IPE)  && !Repart[rep].recopie) {
			k = IpeEcritPixbus(&(Repart[rep]),texte,&l);
			if(LectureLog) {
				if(k == l) printf("%s/ . Selection des FLTs: ecrit %s\n",DateHeure(),texte);
				else printf("%s/ . Selection des FLTs: ecrit %s, transmis: %d/%d (%s)\n",DateHeure(),texte,k,l,strerror(errno));
			}
			break; // un seul chassis IPE??
		}

		/* Synchronisation sur l'horloge D3 */
		LectSynchroReperee = 0;
		LectDureeD3 = 0;
		for(rep=0; rep<RepartNb; rep++) {
			if(!LectDureeD3) LectDureeD3 = Repart[rep].s.ip.stampD3;
			else if(LectDureeD3 != Repart[rep].s.ip.stampD3)
				printf("%s/ ! Le repartiteur %s a une synchro D3 = %d au lieu de %lld\n",DateHeure(),Repart[rep].nom,Repart[rep].s.ip.stampD3,LectDureeD3);
		}
		RepartCompteCalagesD3();
		if(((RepartD3Attendues && (RepartNbActifs > 1)) || LectAcqCollective || LectDureeReset) && !LectStampMini) {
			if(LectureLog) printf("%s/ Calage prealable sur une synchro D3\n",DateHeure());
			starter_declare = strcmp(Starter,"neant");
			if(!starter_declare || Demarreur) /* pas de starter (donc synchro locale), ou alors c'est moi */ {
				if(!LectAccrocheD3(starter_declare)) return(0);
			} else /* je ne suis pas starter, alors qu'il y en a un */ {
				/* Attente message du maitre */
				if(LectureLog) printf("%s/ Attente du message de demarrage de la part de %s\n",DateHeure(),Starter);
			#ifndef CODE_WARRIOR_VSN
				lngr = sizeof(TypeSocket);
				i = 0;
				do {
					lu = recvfrom(Acquis[AcquisLocale].synchro.path,msg_ip,UDP_SYNC_LNGR,0,&(Acquis[AcquisLocale].synchro.skt),&lngr);
					// if(!i) printf("%s/ . recvfrom(%d,lngr=%d) rend %d soit errno=%d/%d\n",DateHeure(),Acquis[AcquisLocale].synchro.path,UDP_SYNC_LNGR,lu,errno,EAGAIN);
					if(lu == -1) {
						if(errno != EAGAIN) {
							LectDeclencheErreur(_ORIGINE_,80,LECT_EMPTY);
							return(0);
						}
					}
					if(msg_ip[0] == 'D') break;
					if(i < 9) i++; else i = 0;
					SambaAttends(100);
					OpiumUserAction();
				} while(LectDemarrageDemande);
				if(LectDemarrageDemande && LectureLog) printf("%s/ Message recu: '%c'\n",DateHeure(),msg_ip[0]);
				else {
					if(LectureLog) printf("%s/ Attente interrompue par l'utilisateur\n",DateHeure());
					return(0);
				}
			 #endif
			}
		} else {
			ArchAttendsD3 = 0;
			if(LectureLog) printf("%s/ Pas de calage prealable sur une synchro D3\n",DateHeure());
		}

		if(!LectSynchroReperee) {
			/* Demarrage, enfin, de tous les repartiteurs geres par cet ordi */
			if(LectureLog) printf("%s/ Demarrage de %d repartiteur%s\n",DateHeure(),Accord1s(RepartNbActifs));
			for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif && !Repart[rep].recopie) {
				unless(RepartiteurDemarreTransmissions(&(Repart[rep]),LectureLog)) {
					printf("%s/   !!! Demarrage impossible. Acquisition stoppee\n",DateHeure());
					return(0);
				}
				if(Repart[rep].interf == INTERF_FILE) sprintf(LectInfo[1],"Relecture du run %s, partition %03d",Repart[rep].parm,LectTrancheRelue);
			}
			if(OpiumAlEcran(pRepartXmit)) PanelRefreshVars(pRepartXmit);
		}
		if(LectModeStatus) {
			if(LectureLog) printf("%s/ PAS de recherche de synchro\n",DateHeure());
		} else if(LectAcqCollective) RepartiteursAttendStamp(LectureLog);
		else if(!LectSynchroReperee) {
			// verif_stamp0: printf("          > LectTimeStamp0 = %lld (fourniture %s)\n",LectTimeStamp0,LectStampSpecifique?"specifique":"quelconque");
			if(LectureLog) printf("%s/ Recherche d'une synchro %s%s\n",DateHeure(),RepartD3Attendues?"D3":"D2",(RepartNbActifs > 1)?" commune":"");
			synchro1.tv_sec = 0;
//#define START_D3_SERIE
		#ifdef START_D3_SERIE
			for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) {
				repart = &(Repart[rep]);
				if(LectureLog) {
					printf("%s/ . Synchronisation du %s %s",DateHeure(),ModeleRep[repart->type].nom,repart->nom);
					if(repart->interf == INTERF_PCI) printf(" @PCI #%d\n",repart->adrs.val);
					else if(repart->interf == INTERF_IP) printf(" @IP %d.%d.%d.%d:%d\n",IP_CHAMPS(repart->adrs),repart->ecr.port);
					else printf("\n");
				}
				if(RepartD3Attendues) {
					unless(RepartiteurRepereD3(repart,&LectEnCours.valeur,LectureLog)) {
						repart->actif = 0;
						printf("%s/   !!! Demarrage prevu impossible. Acquisition stoppee\n",DateHeure());
						return(0);
					} else {
						LectSynchroReperee = 1;
						if(synchro1.tv_sec == 0) memcpy(&synchro1,&LectDateRun,sizeof(struct timeval));
						memcpy(&synchroN,&LectDateRun,sizeof(struct timeval));
					}
				} else unless(RepartiteurRepereD2(repart,&LectEnCours.valeur,LectureLog)) {
					printf("%s/   !!! Synchro D2 absente, execution abandonnee\n",DateHeure());
					OpiumFail("Pas de synchro D2 pour demarrer");
					return(0);
				}
				if(LectureLog) {
					printf("%s/   . Synchro %s avec la donnee #%d %08X",DateHeure(),RepartD3Attendues?"D3":"D2",repart->bottom,LectEnCours.valeur);
					if(repart->interf == INTERF_IP) printf(", au timestamp %lld/%d",repart->s.ip.stamp0,repart->s.ip.num0);
					printf("\n");
				}
				if((LectTimeStamp0 == 0) && (repart->interf == INTERF_IP) && (repart->s.ip.stamp0 > 0) && (!LectStampSpecifique || (repart->categ == DONNEES_STAMP))) {
					LectTimeStamp0 = repart->s.ip.stamp0;
					if(repart->s.ip.stampD3 > repart->s.ip.duree_trame) LectTimeStamp0 += (int64)(repart->s.ip.num0 * repart->s.ip.duree_trame);
					LectTimeStamp0 += (int64)(repart->bottom / repart->nbdonnees);
					LectTimeStampN = LectTimeStamp0;
					if(LectureLog) printf("            . TimeStamp initial du run: %lld (par repartiteur %s)\n",LectTimeStamp0,RepartDonneesListe[repart->categ]);
					if(repart->categ == DONNEES_STAMP) {
						repart->actif = 0;
						if(LectureLog) printf("            . Repartiteur desactive a partir de maintenant\n");
					}
				}
				// repart->temps_precedent = LectDateRun.tv_sec;
			}
			#ifdef ACCES_PCI_DIRECT
				#ifdef CLUZEL_FIFO_INTERNE
					if(repart->top > repart->bottom) {
						LectEnCours.dispo = 1;
						LectEnCours.valeur = repart->fifo32[repart->bottom];
					}
				#endif
				/* sinon, il faut retrouver cette derniere valeur ainsi lue (<mot_lu> dans RepartiteurRepereDx)
				 note: LectEnCours.dispo == 1 car RepartiteurRepereDx retournent <trouvee> */
			#endif
		#else  /* !START_D3_SERIE donc START_D3_PARALLELE */
			if(LectureLog) {
				printf("%s/ . Recherche en parallele pour %d repartiteur%s:\n",DateHeure(),Accord1s(RepartNbActifs));
				for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) {
					repart = &(Repart[rep]);
					printf("%s/   . %s %s",DateHeure(),ModeleRep[repart->type].nom,repart->nom);
					if(repart->interf == INTERF_PCI) printf(" @PCI #%d\n",repart->adrs.val);
					else if(repart->interf == INTERF_IP) printf(" @IP %d.%d.%d.%d:%d\n",IP_CHAMPS(repart->adrs),repart->ecr.port);
					else printf("\n");
				}
			}
			if(RepartD3Attendues) {
				LectSynchroReperee = RepartiteurRepereD3commune(LectureLog);
			} else {
				for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) {
					if(RepartiteurRepereD2(&(Repart[rep]),&LectEnCours.valeur,LectureLog)) {
						if(LectureLog) {
							/* printf("%s/     .          Synchro D2 a la %d%s lecture",DateHeure(),
								nb_trames,(nb_trames>1)?"eme":"ere");
							if(repart->interf == INTERF_IP) printf(" (trame precedente: #%d)",precedent);
							printf(", donnee #%d\n",repart->bottom);
							printf("              .          ");
							if(repart->interf == INTERF_IP) printf("Trame de depart: %lld/%d, valeur = %08X\n",repart->s.ip.stamp0,repart->s.ip.num0,mot_lu);
							else printf("Valeur = %08X\n",mot_lu); */
							printf("%s/     .          Synchro D2 trouvee, donnee #%d\n",DateHeure(),repart->bottom);
						}
						gettimeofday(&LectDateRun,0);
						if(synchro1.tv_sec == 0) memcpy(&synchro1,&LectDateRun,sizeof(struct timeval));
						memcpy(&synchroN,&LectDateRun,sizeof(struct timeval));
					} else {
						printf("%s/ !!! Synchro D2 absente, execution abandonnee\n",DateHeure());
						OpiumFail("Pas de synchro D2 pour demarrer");
						return(0);
					}
				}
				LectSynchroReperee = 1;
			}
			if(LectSynchroReperee) {
				if(LectTimeStamp0 == 0) for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) {
					repart = &(Repart[rep]);
					if((repart->interf == INTERF_IP) && (repart->s.ip.stamp0 > 0) && (!LectStampSpecifique || (repart->categ == DONNEES_STAMP))) {
						LectTimeStamp0 = repart->s.ip.stamp0;
						if(repart->s.ip.stampD3 > repart->s.ip.duree_trame) LectTimeStamp0 += (int64)(repart->s.ip.num0 * repart->s.ip.duree_trame);
						if(repart->nbdonnees) LectTimeStamp0 += (int64)(repart->bottom / repart->nbdonnees); else LectTimeStamp0 += (int64)(repart->bottom);
						LectTimeStampN = LectTimeStamp0;
						if(LectureLog) printf("            . TimeStamp initial du run: %lld (par repartiteur %s)\n",LectTimeStamp0,RepartDonneesListe[repart->categ]);
						if(repart->categ == DONNEES_STAMP) {
							repart->actif = 0;
							if(LectureLog) printf("            . Repartiteur desactive a partir de maintenant\n");
						}
						break;
					}
				}
			} else {
				rep_muet = 0;
				for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif && !repart->D3trouvee && !repart->donnees_recues) {
					printf("%s/ ! Le repartiteur %s ne delivre pas de donnee\n",DateHeure(),repart->nom); rep_muet++;
				}
				if(rep_muet) OpiumWarn("Acquisition impossible: %d carte%s muette%s",Accord2s(rep_muet));
				printf("%s/ !!! Demarrage prevu impossible. Acquisition avortee\n",DateHeure());
				return(0);
			}
		#endif /* !START_D3_SERIE */
		} else /* un seul repartiteur */ {
			LectTimeStamp0 = repart->s.ip.stamp0;
			if(repart->s.ip.stampD3 > repart->s.ip.duree_trame) LectTimeStamp0 += (int64)(repart->s.ip.num0 * repart->s.ip.duree_trame);
			LectTimeStamp0 += (int64)(repart->bottom / repart->nbdonnees);
			LectTimeStampN = LectTimeStamp0;
			if(LectureLog) printf("            . TimeStamp initial du run: %lld (par repartiteur %s)\n",LectTimeStamp0,RepartDonneesListe[repart->categ]);
			if(synchro1.tv_sec == 0) memcpy(&synchro1,&LectDateRun,sizeof(struct timeval));
			memcpy(&synchroN,&LectDateRun,sizeof(struct timeval));
		}
		nb_actifs = 0; LectRefStamp = -1;
		for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) {
			if((LectRefStamp < 0) && (Repart[rep].interf == INTERF_IP)) LectRefStamp = rep;
			nb_actifs++;
			if(nb_actifs == 1) ze_repart = &(Repart[rep]); 
			repart->gene.prochaine = DateMicroSecs();
		}
		if(LectModeStatus) printf("          . Reference du timestamp: %s\n",(LectRefStamp >= 0)? Repart[LectRefStamp].nom: "aucune");
		if(nb_actifs) {
			if(nb_actifs == RepartNbActifs) printf("          . %d repartiteur%s en service\n",Accord1s(nb_actifs));
			else {
				printf("          ! Seulement %d repartiteur%s utilise%s sur %d prevu%s\n",Accord2s(nb_actifs),Accord1s(RepartNbActifs));
				RepartNbActifs = nb_actifs;
			}
		} else {
			printf("          !!! Pas de repartiteur actif, lecture abandonnee\n");
			OpiumFail("Repartiteur(s) demande(s) inactif(s)");
			return(0);
		}
		if(RepartD3Attendues) {
			LectSyncMesure = ((synchroN.tv_sec - synchro1.tv_sec) * 1000000) + (synchroN.tv_usec - synchro1.tv_usec);
			if(LectureLog) printf("          . Duree de synchronisation: %d us\n",LectSyncMesure);
		} else LectSyncMesure = 0;
		if(pDureeSynchro) { if(OpiumDisplayed(pDureeSynchro->cdr)) PanelRefreshVars(pDureeSynchro); }
		RepartCompteCalagesD3();
	}
	if(LectDureeReset) {
		LectGardeReset = 1; // parametrable?
		while(LectGardeReset >= 0) if(((LectDureeReset - LectGardeReset) * ze_repart->s.ip.inc_stamp) <= CalcSpectreAutoDuree) --LectGardeReset; else break;
		d3_initial = ze_repart->s.ip.stampDernier / ze_repart->s.ip.inc_stamp;
		depuis_reset = Modulo(d3_initial,LectDureeReset);
		LectAttenteReset = LectDureeReset - depuis_reset;
		if((depuis_reset == LectGardeReset) || ((LectAttenteReset * ze_repart->s.ip.inc_stamp) > CalcSpectreAutoDuree)) LectDureeReset = 0;
		OpiumProgresTitle("Attente du reset");
		OpiumProgresInit(LectAttenteReset);
	}
	
	CpuTempoRun = clock();
	gettimeofday(&LectDateRun,0);
	LectTempoRun = LectT0Run = ((int64)LectDateRun.tv_sec * 1000000) + (int64)LectDateRun.tv_usec;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif && Repart[rep].simule) Repart[rep].gene.prochaine = LectT0Run;

	if(LectCntl.LectMode == LECT_DONNEES) {
		if(LectSynchroType == LECT_SYNC_SEC) {
			do {
				ArchT0msec = LectDateRun.tv_usec;
				gettimeofday(&LectDateRun,0);
			} while(LectDateRun.tv_usec >= ArchT0msec);
		}
		ArchT0sec = LectDateRun.tv_sec;
		ArchT0msec = LectDateRun.tv_usec;
		if(LectureLog) printf("%s/ Date absolue de debut de run  : %ld.%06d secondes\n",DateHeure(),LectDateRun.tv_sec,(int)LectDateRun.tv_usec);
		// SequenceDateChange = LectDateRun.tv_sec + (long)Sequence[SequenceCourante].duree * 60;
		if(LectEntretien) {
			/* if(EtatRegen) {
				printf("%s/ Source de regeneration active: MAINTENANCE DE DETECTEUR INHIBEE\n",DateHeure());
				LectEntretien = 0;
			} else */ {
				int bolo,b; int64 delai;
				if(LectSession < 2) LectDateRegulBolo = 0; // on lance tout de suite sans attendre la DLC.. LectDateRun.tv_sec + SettingsDLUdetec; (LectDateRegulBolo =  LectDateRun.tv_sec: possible)
				if(LectureLog) {
					delai = LectDateRegulBolo - LectDateRun.tv_sec;
					if(delai > 0) printf("%s/ Premiere maintenance detecteur: dans %ld secondes\n",DateHeure(),delai);
					else printf("%s/ Premiere maintenance detecteur: maintenant\n",DateHeure());
				}
				for(bolo=0; bolo<BoloNb; bolo++) if((b = Bolo[bolo].regul.bloc) >= 0) {
					Bolo[bolo].exec.date = LectDateRegulBolo;
					Bolo[bolo].exec.inst = DetecScriptsLibrairie.bloc[b].premiere; /* on commence par le debut .. */
				}
			}
		} else printf("%s/ Pas de maintenance de detecteur\n",DateHeure());
		LectDateRelance = LectDateRun.tv_sec + LectIntervRelance;
		{	struct tm prochain;
		memcpy(&prochain,localtime(((time_t *)(&LectDateRelance))),sizeof(struct tm));
		printf("%s/ %d repartiteur%s IP lance%s jusqu'a %02d:%02d:%02d\n",DateHeure(),
			   Accord2s(IPactifs),prochain.tm_hour,prochain.tm_min,prochain.tm_sec); }
	}
	if(LectureLog) {
		if(LectStampMini)
			printf("%s/ Prise en compte des donnees a la synchro D3 %lld\n",DateHeure(),LectStampMini);
		else printf("%s/ Prise en compte des donnees %s\n",DateHeure(),RepartD3Attendues? "a la prochaine synchro D3": "immediate");
	}
	LectStampMini = 0;
	memcpy(&LectDateRunDebut,&LectDateRun,sizeof(struct timeval));
	LectCpuRunDebut = clock();
	if(getrusage(RUSAGE_SELF,&StatsSystemeDebut) < 0) {
		printf("%s/ Statistiques systeme indisponibles (%s)\n",DateHeure(),strerror(errno));
		LectStatsValides = 0;
	} else LectStatsValides = 1;
	//-	printf("%s/ Debut d'acquisition, CPU utilise: %ld ticks\n",DateHeure(),LectCpuRunDebut);
	
	return(rc);
}

#pragma mark ------ Processus periodiques ------

/* ========================================================================== */
void LectStocke1Seuil() {
	int voie,i;
	
	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue && (VoieTampon[voie].trig.trgr->type != NEANT)) {
		if(VoieTampon[voie].seuilmin.val) {
			if(VoieTampon[voie].seuilmin.nb < VoieTampon[voie].seuilmin.max) i = (VoieTampon[voie].seuilmin.nb)++;
			else {
				i = (VoieTampon[voie].seuilmin.prem)++;
				if(VoieTampon[voie].seuilmin.prem >= VoieTampon[voie].seuilmin.max) VoieTampon[voie].seuilmin.prem = 0;
			}
			if(VoieTampon[voie].trig.trgr->sens > 0) VoieTampon[voie].seuilmin.val[i] = VoieTampon[voie].trig.trgr->minampl;
			else VoieTampon[voie].seuilmin.val[i] = 0.0;
			LectSeuilsNb = VoieTampon[voie].seuilmin.nb;
			LectSeuilsPrem = VoieTampon[voie].seuilmin.prem;
		}
		if(VoieTampon[voie].seuilmax.val) {
			if(VoieTampon[voie].seuilmax.nb < VoieTampon[voie].seuilmax.max) i = (VoieTampon[voie].seuilmax.nb)++;
			else {
				i = (VoieTampon[voie].seuilmax.prem)++;
				if(VoieTampon[voie].seuilmax.prem >= VoieTampon[voie].seuilmax.max) VoieTampon[voie].seuilmax.prem = 0;
			}
			if(VoieTampon[voie].trig.trgr->sens < 2) VoieTampon[voie].seuilmax.val[i] = VoieTampon[voie].trig.trgr->maxampl;
			else VoieTampon[voie].seuilmax.val[i] = 0.0;
		}
	}
	return;
}
/* ========================================================================== */
static void LectStockeTauxVoies() {
	int voie,i;
	
	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue && (VoieTampon[voie].trig.trgr->type != NEANT)) {
		if(VoieTampon[voie].tabtaux.val) {
			if(VoieTampon[voie].tabtaux.nb < VoieTampon[voie].tabtaux.max) i = (VoieTampon[voie].tabtaux.nb)++;
			else {
				i = (VoieTampon[voie].tabtaux.prem)++;
				if(VoieTampon[voie].tabtaux.prem >= VoieTampon[voie].tabtaux.max) VoieTampon[voie].tabtaux.prem = 0;
			}
			VoieTampon[voie].tabtaux.val[i] = VoieTampon[voie].taux;
			LectTauxNb = VoieTampon[voie].tabtaux.nb;
			LectTauxPrem = VoieTampon[voie].tabtaux.prem;
		}
	}
	return;
}
/* ========================================================================== */
static char LectTermineStocke(int rc, char vidage) {
	int voie;
	char restait;

	restait = 0;
	if(ArchSauve[EVTS]) restait = ArchiveEvt(0,vidage,__func__);
	if(ArchSauve[STRM]) {
		if(ArchModeStream == ARCH_BRUTES) restait = ArchiveBrutes(1);
		else if(ArchModeStream == ARCH_BLOC) {
			for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) { ArchiveBloc(voie); restait = 1; }
		}
	}
	return(restait);
}
/* ========================================================================== */
void LectTraiteStocke() {
	int bolo,voie; int a_sauver; int i,j,k; char log;
	float prec;
	char raison[256];
	int64 avant,delai;
	// TypeDonnee *prochain[VOIES_MAX];

	LectDernierStocke = LectHeureStocke;
	LectHeureStocke = (VerifTempsPasse? DateMicroSecs(): 0);
	log = VerifRunDebug;
	if(log) printf("%s/ %lld SynchroD2 lues, prochain traitement a %lld, %d evt%s a sauver\n",
		DateHeure(),SynchroD2lues,LectNextTrmt,Accord1s(EvtASauver));
	LectDansTraiteStocke = 1;
	// for(voie=0; voie<VoiesNb; voie++) prochain[voie] = VoieTampon[voie].prochain;
	/* Traitement pre-trigger et sauvegarde eventuelle */
	for(k=0; k<ExportPackNb; k++) if(!ExportPack[k].quand && ExportPack[k].info_desc && (SynchroD2lues >= LectNextExport[k])) {
		LectNextExport[k] += LectIntervExport[k];
		ExporteInfos(0,k);
	}
	if(SynchroD2lues >= LectNextTrmt) {
		LectNextTrmt += LectIntervTrmt;
		avant = LectStampsLus;
		/* cas des evenements */
		if(EvtASauver > RESERVE_EVT) {
			if(LectDureeActive && (ArchTrancheReste[EVTS] <= 0)) {
				ArchiveVide(ArchSauve[EVTS],"Partition");
				if(ArchSauve[EVTS]) ArchiveNouvellePartition(EVTS); else ArchTrancheReste[EVTS] = ArchTrancheTaille;
			} else {
//				if(LectDebutEvts < 0) printf("%s/ Debut des evenements perdu (%d) a l'evenement %d\n",DateHeure(),(int)LectDebutEvts,EvtASauver);
				a_sauver = (int)(LectStampsLus - LectDebutEvts);
				if(a_sauver > UsageTampons) {
//					printf("%s/ %d evts, 1er a %d avec tampon utile a %d + %d = %d\n",
//						   DateHeure(),EvtASauver,(int)LectDebutEvts,(int)(LectStampsLus-UsageTampons),(int)UsageTampons,(int)LectStampsLus);
					ArchiveVide(ArchSauve[EVTS],"Lect 3/4");
					if(ArchHorsDelai) {
						int64 debut_evts,debut_tampons;
						debut_evts = LectDebutEvts;
						debut_tampons = LectStampsLus - LngrTampons;
						printf("%s/ %6d,%05d points a sauver pour une limite de %d,%05d / %d,%05d\n",DateHeure(),
							   (int)(a_sauver/100000),Modulo(a_sauver,100000),
							   (int)(UsageTampons/100000),Modulo(UsageTampons,100000),
							   (int)(LngrTampons/100000),Modulo(LngrTampons,100000));
						delai = LectHeureStocke - LectDernierStocke;
						printf("          %6d,%06d s depuis le precedent appel\n",
							   (int)(delai/1000000),Modulo(delai,1000000));
						printf("          Blocs lus:     %6d,%05d          synchro D2 lues: %12lld\n",
							   (int)(LectStampsLus/100000),Modulo(LectStampsLus,100000),SynchroD2lues);
						printf("          Debut tampons: %6d,%05d      prochain traitement: %12lld +%d D2\n",
							   (int)(debut_tampons/100000),Modulo(debut_tampons,100000),LectNextTrmt,1000000,(int)LectIntervTrmt);
						printf("          Debut evts:    %6d,%05d\n",
							   (int)(debut_evts/100000),Modulo(debut_evts,100000));
						ArchHorsDelai = 0;
					}
				}
			}
		}
		/* cas des streams (priorite absolue) */
		if(ArchSauve[STRM]) {
			if(LectDureeActive && (ArchTrancheReste[STRM] <= 0)) {
				ArchiveBrutes(1);
				ArchiveNouvellePartition(STRM);
			} else {
				if(ArchModeStream == ARCH_BLOC) {
					for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
						if((VoieTampon[voie].lus -  VoieTampon[voie].sauve) > (3 * VoieTampon[voie].brutes->max / 4)) ArchiveBloc(voie);
					}
				} else if(ArchModeStream == ARCH_BRUTES) {
					if(ArchAttendsD3) { if(LectDerniereD3) { ArchStampsSauves = LectDerniereD3; ArchAttendsD3 = 0; } }
					unless(ArchAttendsD3) ArchiveBrutes(0);
				}
			}
		}
		/* traitement sur tampons si on a le temps */
		if(Trigger.enservice || (LectStampsLus == avant) || LectModeSpectresAuto) TrmtSurBuffer();
		if(!Acquis[AcquisLocale].etat.active) return;
		if(Archive.enservice && LectSauveStatus && LectStatusDispo) {
			FILE *f,*g; int bb,n; TypeBNModele *modele_bn;
			if(FichierStatusBB[0]) {
				f = fopen(FichierStatusBB,"a");
				if(f) {
					// for(bb=0; bb<NumeriseurNb; bb++) printf("(%s) Le status de la %s date de %lld\n",__func__,Numeriseur[bb].nom,Numeriseur[bb].enreg_stamp);
					for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].enreg_stamp) {
						fprintf(f,"%16s",Numeriseur[bb].nom);
						fprintf(f,"%12s",ModeleBN[Numeriseur[bb].def.type].nom);
						fwrite(&(Numeriseur[bb].enreg_stamp),sizeof(int64),1,f);
						//? fwrite(&(ModeleBN[Numeriseur[bb].def.type].enreg_dim),sizeof(int),1,f);
						fwrite(Numeriseur[bb].enreg_val,sizeof(shex),ModeleBN[Numeriseur[bb].def.type].enreg_dim,f);
					}
					fclose(f);
					LectStatusDispo = 0;
				} else {
					printf("%s/ ! Fichier '%s' inutilisable (%s)\n",DateHeure(),FichierStatusBB,strerror(errno));
					printf("          ! Sauvegarde du status definitif des numeriseurs supprimee\n");
					FichierStatusBB[0] = '\0';
				}
			}
			for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].enreg_stamp) {
				if(Numeriseur[bb].enreg_txt[0]) {
					g = fopen(Numeriseur[bb].enreg_txt,"a");
					if(g) {
						fprintf(g,"%16lld",Numeriseur[bb].enreg_stamp);
						modele_bn = &(ModeleBN[Numeriseur[bb].def.type]);
						for(n=0; n<modele_bn->enreg_dim; n++) {
						#ifdef CODAGE_UTILISATEUR
							char texte[80];
							NumeriseurRessHexaEncode(&(Numeriseur[bb]),modele_bn->numress[modele_bn->enreg_sts[n]],Numeriseur[bb].enreg_val[n],texte,0,0);
							fprintf(g," %s",texte);
						#else
							int val;
							val = (int)Numeriseur[bb].enreg_val[n];
							fprintf(g," %d",val - 32768);
						#endif
						}
						fprintf(g,"\n");
						fclose(g);
						// printf("%s/ Status sauvegarde dans %s\n",DateHeure(),Numeriseur[bb].enreg_txt);
					} else {
						printf("%s/ ! Fichier '%s' inutilisable (%s)\n",DateHeure(),Numeriseur[bb].enreg_txt,strerror(errno));
						printf("          ! Sauvegarde du status temporaire du numeriseur %s supprimee\n",Numeriseur[bb].nom);
						Numeriseur[bb].enreg_txt[0] = '\0';
					}
				}
				Numeriseur[bb].enreg_stamp = 0;
			}
		}
	}
			
	/* Pause ou arret eventuel */
	gettimeofday(&LectDateRun,0);
	if((ScriptDateLimite > 0) && (LectDateRun.tv_sec > ScriptDateLimite)) {
		Acquis[AcquisLocale].etat.active = 0;
		printf("%s/ la duree de lecture demandee a ete faite\n",DateHeure());
	} else if((LectDureeMaxi != DUREE_ILLIMITEE) && LectDureeActive && (LectStampsLus > LectBlocsMaxi) && ArchSauve[EVTS]) {
		Acquis[AcquisLocale].etat.active = 0;
		printf("%s/ la duree maximum de lecture a ete atteinte\n",DateHeure());
	} else if(Lect1ModulALire) {
		printf("%s/ Une synchro D2 a ete lue\n",DateHeure());
		Lect1ModulALire = 0;
		unless(LectureSuspendue) OpiumDisplay(iLectPause->cdr);
		LectureSuspendue = 1;
	} else if(SettingsSynchroMax && (SynchroD2lues >= SettingsSynchroMax)) {
		Acquis[AcquisLocale].etat.active = 0;
		if(SettingsSynchroMax == 1)
			printf("%s/ la synchro D2 unique demandee a ete lue\n",DateHeure());
		else printf("%s/ les %d synchro D2 maxi demandees ont ete lues\n",DateHeure(),SettingsSynchroMax);
		if(LectCntl.LectMode != LECT_IDENT) {
			/*  prevenir l'utilisateur...! */
			unless(OpiumDisplayed(iLectPause->cdr)) OpiumDisplay(iLectPause->cdr);
			LectureSuspendue = 1;
		}
	}

	/* Actions activees toutes les minutes */
	if((SynchroD2lues >= LectNextSeq) && !LectModeSpectresAuto) {
		// AutomDataGet(); trop lent... rafraichir a la main!
		AutomDataQuery(raison);
	#ifndef CODE_WARRIOR_VSN
		if(LectAutreSequence) {
			// printf("%s/ On en est a %d s, prochain changement a %d s\n",DateHeure(),
			//	Modulo(LectDateRun.tv_sec,1000000),Modulo(SequenceDateChange,1000000));
			if(LectDateRun.tv_sec >= (SequenceDateChange - 1)) /* petit pb d'arrondi possible */ {
				LectTermineStocke(0,1);
				if(SequenceSuivante()) {
					SequenceParms(1);
					switch(SequenceCodeCourant()) {
						case SEQUENCE_ACQ: strcpy(LectInfo[0],L_("Acquisition normale en cours")); break;
						case SEQUENCE_STREAM: strcpy(LectInfo[0],L_("Sequence de stream en cours")); break;
						case SEQUENCE_SPECTRES: strcpy(LectInfo[0],L_("Sequence de spectres en cours")); break;
					}
					if(OpiumDisplayed(pLectRegen->cdr)) PanelRefreshVars(pLectRegen);
				}
			}
		}
		if(TrmtRegulActive) {
			/* PLUSIEURS ECHELLES DE TEMPS POSSIBLES SELON VOIE !!! */
			SeuilVariationNb = 0;
			for(voie=0; voie<VoiesNb; voie++) if((VoieTampon[voie].lue > 0) && (VoieTampon[voie].regul_active == TRMT_REGUL_TAUX) && Trigger.actif) {
				TypeRegulTaux *echelle;
				//- arrondi, du temps ou les min/max de trigger etaient de type TypeDonnee: float dmin,dmax; (ajoutes a min/maz avant conversion)
				//- dmin = (VoieTampon[voie].trig.trgr->minampl >= 0.0)? 0.5: -0.5;
				//- dmax = (VoieTampon[voie].trig.trgr->maxampl >= 0.0)? 0.5: -0.5;
				/* printf("(%s) Regulation prevue pour %s ",__func__,VoieManip[voie].nom);
				for(j=0; j<MAXECHELLES; j++) printf("%s a %d/%d",j?",":":",VoieTampon[voie].rgul.taux.prochaine[j],LectDateRun.tv_sec);
				printf("\n"); */
				for(j=0; j<MAXECHELLES; j++) if(LectDateRun.tv_sec >= (VoieTampon[voie].rgul.taux.prochaine[j] - 1)) {
					char modifs_avant;
					modifs_avant = TrmtRegulModifs;
					// if(VoieTampon[voie].regul_standard) echelle = &(TrmtRegulTaux[j]); else echelle = &(VoieManip[voie].echelle[j]);
					echelle = &(VoieManip[voie].echelle[j]);
					if(echelle->active) {
						/* printf("%s/ On en est a %d s, prochain changement %d mn a %d s\n",DateHeure(),
							Modulo(LectDateRun.tv_sec,1000000),echelle->duree,Modulo(LectProchaineRegul[j],1000000)); */
						/* printf("%s/ %d evts/%d mn sur %s, variation de %g\n",DateHeure(),
							VoieTampon[voie].rgul.taux.nbevts[j],echelle->duree,VoieManip[voie].nom,echelle->max.parm); */
						if(echelle->min.action && (VoieTampon[voie].rgul.taux.nbevts[j] < echelle->min.nb)) {
							printf("%s/ %12s, %2d evts/%d mn: descend seuil",DateHeure(),
								VoieManip[voie].nom,VoieTampon[voie].rgul.taux.nbevts[j],echelle->duree);
							SeuilVariation[SeuilVariationNb].voie = voie;
							SeuilVariation[SeuilVariationNb].nbevts = VoieTampon[voie].rgul.taux.nbevts[j];
							SeuilVariation[SeuilVariationNb].duree = echelle->duree;
							SeuilVariation[SeuilVariationNb].descend = 1;
							if(VoieTampon[voie].trig.trgr->sens > 0) {
								prec = VoieTampon[voie].trig.trgr->minampl;
								if(echelle->min.action == TRMT_REGUL_ADD) { 
									VoieTampon[voie].trig.trgr->minampl += echelle->min.parm; TrmtRegulModifs++;
								} else if(echelle->min.action == TRMT_REGUL_MUL) {
									VoieTampon[voie].trig.trgr->minampl = VoieTampon[voie].trig.trgr->minampl * echelle->min.parm; TrmtRegulModifs++;
								} else if(echelle->min.action == TRMT_REGUL_DIV) {
									VoieTampon[voie].trig.trgr->minampl = VoieTampon[voie].trig.trgr->minampl / echelle->min.parm; TrmtRegulModifs++;
								} else if(echelle->min.action == TRMT_REGUL_SET) {
									VoieTampon[voie].trig.trgr->minampl = echelle->min.parm; TrmtRegulModifs++;
								}
								if((VoieTampon[voie].trig.trgr->sens == 1) && (VoieTampon[voie].trig.trgr->maxampl >= VoieTampon[voie].trig.trgr->minampl)) 
									VoieTampon[voie].trig.trgr->minampl = VoieTampon[voie].trig.trgr->maxampl + 1.0;
								if(VoieTampon[voie].trig.trgr->minampl < VoieManip[voie].def.rgul.mininf) {
									VoieTampon[voie].trig.trgr->minampl = VoieManip[voie].def.rgul.mininf;
									printf(" a son plancher de %g",VoieTampon[voie].trig.trgr->minampl);
								} else if(VoieTampon[voie].trig.trgr->minampl > VoieManip[voie].def.rgul.minsup) {
									VoieTampon[voie].trig.trgr->minampl = VoieManip[voie].def.rgul.minsup;
									printf(" a son plafond de %g",VoieTampon[voie].trig.trgr->minampl);
								} else printf(" min de %g a %g",prec,VoieTampon[voie].trig.trgr->minampl);
							}
							if(VoieTampon[voie].trig.trgr->sens < 2) {
								prec = VoieTampon[voie].trig.trgr->maxampl;
								if(echelle->min.action == TRMT_REGUL_ADD) {
									VoieTampon[voie].trig.trgr->maxampl -= echelle->min.parm; TrmtRegulModifs++;
								} else if(echelle->min.action == TRMT_REGUL_MUL) {
									VoieTampon[voie].trig.trgr->maxampl = VoieTampon[voie].trig.trgr->maxampl * echelle->min.parm; TrmtRegulModifs++;
								} else if(echelle->min.action == TRMT_REGUL_DIV) {
									//- VoieTampon[voie].trig.trgr->maxampl /= echelle->min.parm; TrmtRegulModifs++;
									VoieTampon[voie].trig.trgr->maxampl = VoieTampon[voie].trig.trgr->maxampl / echelle->min.parm; TrmtRegulModifs++;
								} else if(echelle->min.action == TRMT_REGUL_SET) {
									VoieTampon[voie].trig.trgr->maxampl = echelle->min.parm; TrmtRegulModifs++;
								}
								if((VoieTampon[voie].trig.trgr->sens == 1) && (VoieTampon[voie].trig.trgr->maxampl >= VoieTampon[voie].trig.trgr->minampl)) 
									VoieTampon[voie].trig.trgr->maxampl = VoieTampon[voie].trig.trgr->minampl - 1.0;
								if(VoieTampon[voie].trig.trgr->sens > 0) printf(",");
								if(VoieTampon[voie].trig.trgr->maxampl > VoieManip[voie].def.rgul.maxinf) {
									VoieTampon[voie].trig.trgr->maxampl = VoieManip[voie].def.rgul.maxinf;
									printf(" a son plancher de %g",VoieTampon[voie].trig.trgr->maxampl);
								} else if(VoieTampon[voie].trig.trgr->maxampl < VoieManip[voie].def.rgul.maxsup) {
									VoieTampon[voie].trig.trgr->maxampl = VoieManip[voie].def.rgul.maxsup;
									printf(" a son plafond de %g",VoieTampon[voie].trig.trgr->maxampl);
								} else printf(" max de %g a %g",prec,VoieTampon[voie].trig.trgr->maxampl);
							}
							printf("\n");
							VoieTampon[voie].trig.trgr->std = 0;
							bolo = VoieManip[voie].det; Bolo[bolo].seuils_changes = 1;
							if(OpiumDisplayed(mLectSupplements->cdr)) MenuItemAllume(mLectSupplements,3,0,GRF_RGB_YELLOW);
						} else if(echelle->max.action && (VoieTampon[voie].rgul.taux.nbevts[j] > echelle->max.nb)) {
							printf("%s/ %12s, %2d evts/%d mn: monte seuil",DateHeure(),
								   VoieManip[voie].nom,VoieTampon[voie].rgul.taux.nbevts[j],echelle->duree);
							SeuilVariation[SeuilVariationNb].voie = voie;
							SeuilVariation[SeuilVariationNb].nbevts = VoieTampon[voie].rgul.taux.nbevts[j];
							SeuilVariation[SeuilVariationNb].duree = echelle->duree;
							SeuilVariation[SeuilVariationNb].descend = 0;
							if(VoieTampon[voie].trig.trgr->sens > 0) {
								prec = VoieTampon[voie].trig.trgr->minampl;
								if(echelle->max.action == TRMT_REGUL_ADD) {
									VoieTampon[voie].trig.trgr->minampl += echelle->max.parm; TrmtRegulModifs++;
								} else if(echelle->max.action == TRMT_REGUL_MUL) {
									VoieTampon[voie].trig.trgr->minampl = VoieTampon[voie].trig.trgr->minampl * echelle->max.parm; TrmtRegulModifs++;
								} else if(echelle->max.action == TRMT_REGUL_DIV) {
									VoieTampon[voie].trig.trgr->minampl = VoieTampon[voie].trig.trgr->minampl / echelle->max.parm; TrmtRegulModifs++;
								} else if(echelle->min.action == TRMT_REGUL_SET) {
									VoieTampon[voie].trig.trgr->minampl = echelle->max.parm; TrmtRegulModifs++;
								}
								if((VoieTampon[voie].trig.trgr->sens == 1) && (VoieTampon[voie].trig.trgr->maxampl >= VoieTampon[voie].trig.trgr->minampl)) 
									VoieTampon[voie].trig.trgr->minampl = VoieTampon[voie].trig.trgr->maxampl + 1.0;
								if(VoieTampon[voie].trig.trgr->minampl < VoieManip[voie].def.rgul.mininf) {
									VoieTampon[voie].trig.trgr->minampl = VoieManip[voie].def.rgul.mininf;
									printf(" a son plancher de %g",VoieTampon[voie].trig.trgr->minampl);
								} else if(VoieTampon[voie].trig.trgr->minampl > VoieManip[voie].def.rgul.minsup) {
									VoieTampon[voie].trig.trgr->minampl = VoieManip[voie].def.rgul.minsup;
									printf(" a son plafond de %g",VoieTampon[voie].trig.trgr->minampl);
								} else printf(" min de %g a %g",prec,VoieTampon[voie].trig.trgr->minampl);
							}
							if(VoieTampon[voie].trig.trgr->sens < 2) {
								prec = VoieTampon[voie].trig.trgr->maxampl;
								if(echelle->max.action == TRMT_REGUL_ADD) {
									VoieTampon[voie].trig.trgr->maxampl -= echelle->max.parm; TrmtRegulModifs++;
								} else if(echelle->max.action == TRMT_REGUL_MUL) {
									VoieTampon[voie].trig.trgr->maxampl = VoieTampon[voie].trig.trgr->maxampl * echelle->max.parm; TrmtRegulModifs++;
								} else if(echelle->max.action == TRMT_REGUL_DIV) {
									VoieTampon[voie].trig.trgr->maxampl = VoieTampon[voie].trig.trgr->maxampl / echelle->max.parm; TrmtRegulModifs++;
								} else if(echelle->min.action == TRMT_REGUL_SET) {
									VoieTampon[voie].trig.trgr->maxampl = echelle->max.parm; TrmtRegulModifs++;
								}
								if((VoieTampon[voie].trig.trgr->sens == 1) && (VoieTampon[voie].trig.trgr->maxampl >= VoieTampon[voie].trig.trgr->minampl)) 
									VoieTampon[voie].trig.trgr->maxampl = VoieTampon[voie].trig.trgr->minampl - 1.0;
								if(VoieTampon[voie].trig.trgr->sens > 0) printf(",");
								if(VoieTampon[voie].trig.trgr->maxampl > VoieManip[voie].def.rgul.maxinf) {
									VoieTampon[voie].trig.trgr->maxampl = VoieManip[voie].def.rgul.maxinf;
									printf(" a son plancher de %g",VoieTampon[voie].trig.trgr->maxampl);
								} else if(VoieTampon[voie].trig.trgr->maxampl < VoieManip[voie].def.rgul.maxsup) {
									VoieTampon[voie].trig.trgr->maxampl = VoieManip[voie].def.rgul.maxsup;
									printf(" a son plafond de %g",VoieTampon[voie].trig.trgr->maxampl);
								} else printf(" max de %g a %g",prec,VoieTampon[voie].trig.trgr->maxampl);
							}
							printf("\n");
							// printf("          Variation #%d type %d de %g\n",TrmtRegulModifs,echelle->max.action,echelle->max.parm);
							VoieTampon[voie].trig.trgr->std = 0;
							bolo = VoieManip[voie].det; Bolo[bolo].seuils_changes = 1;
							if(OpiumDisplayed(mLectSupplements->cdr)) MenuItemAllume(mLectSupplements,3,0,GRF_RGB_YELLOW);
						}
						VoieTampon[voie].rgul.taux.nbevts[j] = 0;
						if((TrmtRegulModifs > modifs_avant) && (SeuilVariationNb < MAXVARIATIONS)) {
							SeuilVariation[SeuilVariationNb].min = VoieTampon[voie].trig.trgr->minampl;
							SeuilVariation[SeuilVariationNb].max = VoieTampon[voie].trig.trgr->maxampl;
							SeuilVariationNb++;
						}
					}
					VoieTampon[voie].rgul.taux.prochaine[j] = LectDateRun.tv_sec + (long)(echelle->duree * 60);
				}
			}
			if(TrmtRegulModifs) {
				printf("%s/ Ajustement de %d seuil%s\n",DateHeure(),Accord1s(TrmtRegulModifs));
				if(ArchSauve[EVTS]) {
					int usecs;
					SambaTempsEchantillon(DernierPointTraite,ArchT0sec,ArchT0msec,&SeuilVariationDate,&usecs);					
					ArchiveSeuils(SeuilVariationDate);
					if(EdbActive) {
						// ARG_STYLES ancien;
						// ancien = ArgStyle(ARG_STYLE_JSON); ArgPrint("SeuilVariation",SeuilVarListeDesc); ArgStyle(ancien);
						sprintf(SeuilVariationId,"thr_%s_%d",Acquis[AcquisLocale].etat.nom_run,SeuilVariationDate);
						DbSendDesc(EdbServeur,SeuilVariationId,SeuilVarListeDesc);
						if(DbStatus) printf("%s/ Base de Donnees renseignee (Id: %s, rev: %s)\n",DateHeure(),DbId,DbRev);
						else {
							printf("%s/ ! Renseignement de la Base de Donnees par %s en erreur\n",DateHeure(),__func__);
							printf("%9s   . Nature: %s, raison: %s\n"," ",DbErreur,DbRaison);
							printf("%9s   . Id: %s, rev: %s\n"," ",SeuilVariationId,"nouvelle entree");
						}
					}
				}
				TrmtRegulModifs = 0;
			}
			LectStocke1Seuil();
		}
	#endif /* CODE_WARRIOR_VSN */
		if(AutomDataGet(raison)) {
			ReglTbolo = *T_Bolo; EtatTubes = *PulseTubes;
			/* on ne change plus la finalite du run
			for(i=0; i<AUTOM_CALIB_MAX; i++) EtatSrceCalib[i] = *(AutomSrceCalibAdrs[i]);
			for(i=0; i<AUTOM_REGEN_MAX; i++) EtatSrceRegen[i] = *(AutomSrceRegenAdrs[i]);  */
		} else {
			printf("%s/ %s\n",DateHeure(),raison);
			printf("%s/ Connexion automate DESACTIVEE\n",DateHeure());
			AutomLink = -2;
			ReglTbolo = 300.0;
			for(i=0; i<AutomUserNb; i++) switch(AutomUserVar[i].type) {
				case AUTOM_BOOL: case AUTOM_CLE: AutomUserVar[i].val.b = 0; break;
				case AUTOM_INT: AutomUserVar[i].val.i = 0; break;
				case AUTOM_FLOAT: AutomUserVar[i].val.r = 0.0; break;
			}
		}
		LectNextSeq = SynchroD2lues + LectIntervSeq;
	}

	LectDansTraiteStocke = 0;
	LectDepileInhibe = 0;
	/* for(voie=0; voie<VoiesNb; voie++) if(prochain[voie] != VoieTampon[voie].prochain) {
		printf("(%s) %16s @%08llX -> %08llX [%.1f]\n",__func__,VoieManip[voie].nom,
			(IntAdrs)(prochain[voie]),(IntAdrs)(VoieTampon[voie].prochain),
			   (float)VoieTampon[voie].lus * VoieEvent[voie].horloge);
	} */
	return;
}
/* ========================================================================== */
void LectActionUtilisateur() {
#ifdef MSGS_RESEAU
	char msg[256];
#endif

	LectDansActionUtilisateur = 1;
	/* { struct timeval heure; gettimeofday(&heure,0);
		printf("(%s) [%s:%06d] Appel apres %lld synchroD2 lues\n",__func__,DateHeure(),heure.tv_usec,SynchroD2lues); } */

	/* Action utilisateur possible */
	if(SynchroD2lues >= LectNextUser) {
		OpiumUserAction();
		if(WndQueryExit) Acquis[AcquisLocale].etat.active = 0;
#ifdef MSGS_RESEAU
		if(CarlaMsgGetCmde(msg,CARLA_TEXTMAX)) CarlaMsgExec(SambaCommandes,msg);
#endif
//1		LectNextUser += LectIntervUser;
//2		nb = SynchroD2lues / LectIntervUser; LectNextUser = (nb + 1) * LectIntervUser;
		LectNextUser = SynchroD2lues + LectIntervUser;
		if(LectureSuspendue) printf("%s/ Pause demandee par l'utilisateur\n",DateHeure());
		/* else unless(Acquis[AcquisLocale].etat.active) LectMessageArret(_ORIGINE_,0,0); fait au return (cf lecture en parallele) */
	}

	/* Mise a jour de l'affichage */
	// printf("%s/ %lld synchro%s lue%s, prochain affichage a %lld\n",DateHeure(),Accord2s(SynchroD2lues),LectNextDisp);
	if(LectureSuspendue) {
		LectBuffStatus(); LectDisplay();
		while(LectureSuspendue) OpiumUserAction();
		if(Acquis[AcquisLocale].etat.active) printf("%s/ Reprise demandee par l'utilisateur\n",DateHeure());
		else LectMessageArret(_ORIGINE_,0,0);
	} else if(SynchroD2lues >= LectNextDisp) {
		// LectAffEvt = 1;
		LectDisplay();
//1		LectNextDisp += LectIntervData;
		LectNextDisp = SynchroD2lues + LectIntervData;
		// printf("(%s) Display apres %lld synchroD2 lues, prochain display a %lld\n",__func__,SynchroD2lues,LectNextDisp);
	}

	LectDansActionUtilisateur = 0;
}
#ifdef MODULES_RESEAU
/* ========================================================================== */
static void LectAfficheEvt() {
#ifdef A_REVOIR
	Graph g; int rc; int voietrig;
	/* Voir MonitAfficheEvt avec 'Evt[evtnum].' remplace par 'EvtPartage->' */

	voietrig = EvtPartage->voie;
	g = gEvtRunCtrl;
	rc = GraphDataReplace(g,1,GRF_SHORT|GRF_YAXIS,EvtPartage->val,EvtPartage->lngr);
	if(rc < 0) printf("(LectAfficheEvt) le pointage sur l'evenement a echoue\n");
	GraphUse(g,EvtPartage->lngr);
	GraphAxisReset(g,GRF_XAXIS);
	GraphAxisReset(g,GRF_YAXIS);
	if(EvtPartage->filtrees) {
		rc = GraphDataReplace(g,5,GRF_SHORT|GRF_YAXIS,EvtPartage->filtrees+EvtPartage->max-nbfiltrees,nbfiltrees);
		if(rc < 0) printf("(LectAfficheEvt[evt solo]) le pointage sur le resultat du filtrage a echoue\n");
	}
	OpiumRefresh(gEvtRunCtrl->cdr);
	LectCntl.MonitEvtNum = EvtPartage->num;
	strcpy(BoloTrigge,Bolo[VoieManip[voietrig].det].nom);
	strcpy(VoieTriggee,VoieManip[voietrig].prenom);
	MonitEvtAmpl = EvtPartage->val[MONIT_AMPL];
	MonitEvtMontee = EvtPartage->val[MONIT_MONTEE];
	PanelRefreshVars(pLectEvtNum);
	PanelRefreshVars(pLectEvtQui);
#endif
}
#endif
/* ========================================================================== */
static void LectCalculeTaux(TypeMonitCatg *categ) {
	if(!(categ->utilisee)) return;
	if(categ->stamp_utilise != categ->stamp_trouve) {
		categ->freq = (float)(categ->trouves - categ->utilises)
		/ ((float)(categ->stamp_trouve - categ->stamp_utilise) * EnSecondes);
		categ->utilises = categ->trouves; categ->stamp_utilise = categ->stamp_trouve;
	} else if(categ->trouves == categ->utilises) {
		if(DernierPointTraite != categ->stamp_trouve) {
			categ->freq = 1.0 / ((float)(DernierPointTraite - categ->stamp_trouve) * EnSecondes);
		} else if(categ->delai > 0.0) categ->freq = 1.0 / categ->delai; /* Hz */
		else categ->freq = 0.0;
	}
}
/* ========================================================================== */
static void LectFenRefresh(TypeMonitFenetre *f) {
	Graph g; Cadre cdr; TypeMonitTrace *t; char peut_afficher,doit_terminer;

	char trmt;
	TypeTamponDonnees *tampon;
	int j,numx,numy,voie,position;
	int64 synchros_a_passer,synchros_passees;
	int64 t0;
	char synchro_modulation;
	char ok;
	char raison[256];
#ifdef A_RE_EXAMINER
	int64 periode;
#endif

	if((f->affiche == MONIT_HIDE)|| !(g = f->g)) return;
	if(LectMaintenance || LectModeSpectresAuto) { if(f->affiche != MONIT_ALWS) return; }
	cdr = g->cdr; peut_afficher = 1;
	doit_terminer = OpiumRefreshBegin(cdr);
	// printf("(%s) %s rafraichi: '%s'\n",__func__,MonitFenTypeName[f->type],f->titre);
	/* ..... */
	if(f->type == MONIT_SIGNAL) {
		synchro_modulation = 0;
		synchros_a_passer = 0;
		synchros_passees = 0;
		if(MonitSynchro) for(j=0, t=f->trace; j<f->nb; j++,t++) {
			voie = t->voie; if(VoieTampon[voie].lue <= 0) continue;
			if(VoieManip[voie].modulee && (VoieTampon[voie].trmt[TRMT_AU_VOL].utilise == NEANT)) {
				synchro_modulation = 1;
				break;
			}
		}
		for(j=0, t=f->trace; j<f->nb; j++,t++) {
			TypeTamponDonnees *donnees;
			voie = t->voie; if(VoieTampon[voie].lue <= 0) continue;
			trmt = t->var; numx = t->numx; numy = numx + 1;
			donnees = &(VoieTampon[voie].trmt[trmt].donnees);
		#ifdef ALAMBIQUE
			if(trmt == TRMT_AU_VOL) {
				GraphDataUse(g,numx,VoieTampon[voie].brutes->nb);
				GraphDataUse(g,numy,VoieTampon[voie].brutes->nb);
				if(VoieTampon[voie].circulaire) {
					// position = Modulo(VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien,VoieTampon[voie].brutes->max); /* position == brutes.prem? */
					position = VoieTampon[voie].brutes->prem;
					GraphDataRescroll(g,numx,position);
					GraphDataRescroll(g,numy,position);
				} else position = 0;
				// printf("(%s) Voie '%s'.brutes->nb=%d/%d, .first=%lld (plein %s, position=%d/%d)\n",__func__,VoieManip[voie].nom,
				//	VoieTampon[voie].brutes->nb,VoieTampon[voie].brutes->max,VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien,
				//  VoieTampon[voie].circulaire?"complet":"partiel",position,VoieTampon[voie].brutes->prem);
			#ifdef A_RE_EXAMINER
				if(synchro_modulation) /* calage affichage sur synchro repartiteur */ {
					if(VoieManip[voie].modulee) {
						debut = VoieTampon[voie].lus - (int64)(f->axeX.r.max / VoieEvent[voie].horloge) - LectDerniereD3;
						periode = (int64)(Diviseur2 / VoieTampon[voie].trmt[TRMT_AU_VOL].compactage);
						synchros_a_passer = ((debut / periode) * periode) + LectDerniereD3;
						synchro_modulation = 0; /* plus la peine de deplacer l'ascenseur */
					}
				} else {
					synchros_passees = VoieTampon[voie].lus - (int64)(f->axeX.r.max / VoieEvent[voie].horloge);
					if(!synchros_a_passer) synchros_a_passer = synchros_passees;
					else if(synchros_passees < synchros_a_passer) synchros_a_passer = synchros_passees;
					/* ca aussi... en fait c'est GraphUserScrollX qui est mal utilise
					 synchros_passees = VoieTampon[voie].lus - (int64)(f->axeX.r.max / VoieEvent[voie].horloge);
					 if(synchros_a_passer < 0) synchros_a_passer = 0; */
				}
				if(synchros_a_passer) /* calage par deplacement de l'ascenseur */ {
					temps = (VoieTampon[voie].trmt[trmt].ab6[0] + (VoieTampon[voie].trmt[trmt].ab6[1] * 1 * synchros_a_passer));
					GraphUserScrollX(g,0,temps);
				}
			#endif /* A_RE_EXAMINER */
			} else /* (trmt == TRMT_PRETRG) */ {
				if(VoieTampon[voie].trmt[trmt].point_affiche != DernierPointTraite) {
					VoieTampon[voie].trmt[trmt].point_vu = DernierPointTraite;
					GraphDataUse(g,numx,VoieTampon[voie].traitees->nb);
					GraphDataUse(g,numy,VoieTampon[voie].traitees->nb);
					GraphDataRescroll(g,numx,VoieTampon[voie].traitees->prem);
					GraphDataRescroll(g,numy,VoieTampon[voie].traitees->prem);
				#ifdef A_RE_EXAMINER
					temps = ((float)DernierPointTraite / Echantillonnage) - f->axeX.r.max;
					GraphUserScrollX(g,0,temps);
				#endif /* A_RE_EXAMINER */
				}
				VoieTampon[voie].trmt[trmt].point_affiche = VoieTampon[voie].trmt[trmt].point_vu;
			}
		#else  /* !ALAMBIQUE */
			GraphDataUse(g,numx,donnees->nb);
			GraphDataUse(g,numy,donnees->nb);
			GraphDataRescroll(g,numx,donnees->prem);
			GraphDataRescroll(g,numy,donnees->prem);
		#endif /* !ALAMBIQUE */
			if(t->ajuste) MonitTraceAjusteY(t,donnees,g);
			
		}
		unless(synchros_passees) { OpiumScrollXmax(cdr); /* au pire... */ }
		
		/* ..... */
	} else if(f->type == MONIT_EVENT) {
		int va,prem; int trmt;
		int evtnum,evtdim;
		int64 evtstart;
		int64 trmtstart,trmtend;
		TypeVoieArchivee *archivee;
		hexa zoom; float total;
		
		unless(EvtASauver) { if(doit_terminer) OpiumRefreshEnd(cdr); return; }; /* Pas d'evenement en memoire */
		evtnum = EvtASauver - 1; prem = -1; total = 0.0;
		LectCntl.MonitEvtNum = Evt[evtnum].num;
		for(va=0; va<Evt[evtnum].nbvoies; va++) {
			archivee = &(Evt[evtnum].voie[va]);
			for(j=0, t=f->trace; j<f->nb; j++,t++) {
				voie = t->voie;
				if(voie != archivee->num) continue;
				if(prem < 0) prem = voie;
				evtstart = archivee->debut; evtdim = archivee->dim;
				trmtstart = evtstart; trmtend = evtstart + evtdim;
				numx = t->numx; numy = numx + 1;
				trmt = (int)t->var;
				if(trmt == TRMT_AU_VOL) {
					if(evtstart < VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien) continue; /* Evenement plus en memoire */
					if((evtstart + evtdim) > VoieTampon[voie].lus) {
						evtdim = (int)(VoieTampon[voie].lus - evtstart);
					}
					if(evtdim <= 0) continue;
					// t0 = (evtstart / (int64)VoieTampon[voie].brutes->max) * (int64)VoieTampon[voie].brutes->max;
					// position = (int)(evtstart - t0);
					position = archivee->adresse;
					t0 = evtstart - (int64)position;
					MonitEvtAb6[voie][0] = (float)(((double)t0 * (double)VoieTampon[voie].trmt[TRMT_AU_VOL].ab6[1]) - (double)(MonitT0 * 1000.0));
					GraphDataReplace(g,numy,GRF_SHORT|GRF_YAXIS,VoieTampon[voie].brutes->t.sval,VoieTampon[voie].brutes->max);
					GraphDataUse(g,numx,evtdim);
					GraphDataUse(g,numy,evtdim);
					GraphDataRescroll(g,numx,position);
					GraphDataRescroll(g,numy,position);
					total = (float)evtdim / Echantillonnage;
					
				} else {  /* (trmt == TRMT_PRETRG) */
					int64 debut_traitees,fin_traitees;
					int trmtdim,trmtdecal; int64 reduction;
					
					reduction = (int64)VoieTampon[voie].trmt[TRMT_PRETRG].compactage;
					fin_traitees = DernierPointTraite / (int64)VoieTampon[voie].trmt[TRMT_AU_VOL].compactage;
					debut_traitees = fin_traitees - (VoieTampon[voie].traitees->nb * reduction);
					if(trmtstart < debut_traitees) trmtstart = debut_traitees;
					if(trmtend > fin_traitees) trmtend = fin_traitees;
					trmtdecal = (int)(trmtstart - evtstart);
					trmtstart /= reduction; trmtend /= reduction;
					trmtdim = (int)(trmtend - trmtstart);
					if(trmtdim <= 0) continue;
					t0 = (trmtstart / (int64)VoieTampon[voie].traitees->max) * (int64)VoieTampon[voie].traitees->max;
					position = (int)(evtstart - t0);
					MonitTrmtAb6[voie][0] = (float)(((double)t0 * (double)VoieTampon[voie].trmt[TRMT_PRETRG].ab6[1]) - (double)(MonitT0 * 1000.0));
					GraphDataReplace(g,numy,GRF_SHORT|GRF_YAXIS,VoieTampon[voie].traitees->t.rval,VoieTampon[voie].traitees->max);
					GraphDataUse(g,numx,trmtdim);
					GraphDataUse(g,numy,trmtdim);
					GraphDataRescroll(g,numx,position);
					GraphDataRescroll(g,numy,position);
					total = (float)(trmtdim * VoieTampon[voie].trmt[TRMT_AU_VOL].compactage) / Echantillonnage;
				
				}
			}
		}
		if(f->axeX.r.max > total) f->axeX.r.max = total;
		zoom = total / f->axeX.r.max;
		// printf("Zoom pour %s: %g / %g = %d\n",f->titre,total,f->axeX.r.max,zoom);
		if((zoom > 1) && (prem >= 0) && VoieEvent[prem].lngr) {
			int dx; // avec <dx> fonction du pretrigger (pour avoir l'evt au milieu)
			GraphZoom(g,zoom,1);
			dx = ((VoieEvent[prem].avant_evt * cdr->larg) / VoieEvent[prem].lngr) - (cdr->dh / 2);
			OpiumScrollXset(cdr,dx);
		}
		
		/* ..... */
	} else if(f->type == MONIT_HISTO) {
		if(Acquis[AcquisLocale].etat.evt_trouves < 2) { if(doit_terminer) OpiumRefreshEnd(cdr); return; };
		if(!f->axeY.i.max) GraphAxisReset(g,GRF_YAXIS);
		
		/* ..... */
		/* ..... */
	} else if(f->type == MONIT_2DHISTO) {
		int min,max;
		if(Acquis[AcquisLocale].etat.evt_trouves < 2) { if(doit_terminer) OpiumRefreshEnd(cdr); return; };
		H2DGetExtremaInt(f->p.s.histo,&min,&max);
		GraphDataIntGamma(g,2,min,max,0,OPIUM_LUT_DIM - 1);
		OpiumBloque(cdr,1);
		peut_afficher = 0;

	} else if(f->type == MONIT_BIPLOT) {
		if(NtUsed <= f->p.b.nb) GraphUse(g,NtUsed);
		else { GraphStart(g,NtUsed - f->p.b.nb); GraphEnd(g,NtUsed); }

		/* ..... */
	} else if(f->type == MONIT_PATTERN) {
		for(j=0, t=f->trace; j<f->nb; j++,t++) {
			voie = t->voie; if(VoieTampon[voie].lue <= 0) { if(doit_terminer) OpiumRefreshEnd(cdr); return; };
			numx = t->numx; numy = numx + 1;
			GraphDataUse(g,numx,VoieTampon[voie].pattern.max);
			GraphDataUse(g,numy,VoieTampon[voie].pattern.max);
		}
		
		/* ..... */
	} else if(f->type == MONIT_MOYEN) {
		float tdeb,tfin,tmin,tmax; char prems; int trmt;
		prems = 1; tmin = tmax = 0.0;
		for(j=0, t=f->trace; j<f->nb; j++,t++) {
			voie = t->voie;
			if(VoieTampon[voie].moyen.nb <= 0) continue;
			numx = t->numx; numy = numx + 1;
			// if(VoieTampon[voie].traitees->t.rval) trmt = TRMT_PRETRG; else 
			trmt = TRMT_AU_VOL;
			tdeb = VoieTampon[voie].moyen.decal / (float)VoieTampon[voie].moyen.nb;
			tfin = tdeb + (VoieTampon[voie].somme.taille * VoieTampon[voie].trmt[trmt].ab6[1]);
			if(prems) { tmin = tdeb; tmax = tfin; prems = 0; }
			else { if(tmin > tdeb) tmin = tdeb; if(tmax < tfin) tmax = tfin; }
			MonitEvtAb6[voie][0] = tdeb;
			GraphDataUse(g,numx,VoieTampon[voie].somme.taille);
			GraphDataUse(g,numy,VoieTampon[voie].somme.taille);
		}
		GraphAxisFloatRange(g,GRF_XAXIS,tmin,tmax);
		
		/* ..... */
	} else if(f->type == MONIT_FFT) {
		for(j=0, t=f->trace; j<f->nb; j++,t++) {
			int voie,evt; int k; int64 debut_brut; int trmt,of7,p1,p0,va,compactage;
			voie = t->voie; if(VoieTampon[voie].lue <= 0) continue;
			//				printf("Examen de la voie %s a t1=%d\n",VoieManip[voie].nom,(int)VoieTampon[voie].lus);
		#define VERSION_MIXTE
		#ifdef VERSION_MIXTE
			trmt = t->var;
			tampon = &(VoieTampon[voie].trmt[trmt].donnees);
			compactage = (trmt == TRMT_AU_VOL)? 1: VoieTampon[voie].trmt[TRMT_PRETRG].compactage;
			p1 = tampon->nb;
			of7 = tampon->prem;
			k = f->p.f.intervalles; if(k == 0) k = 1;
			evt = EvtASauver - 1;
			do {
				p0 = p1 - f->axeX.pts;
				if(p0 < 0) break;
				ok = 1;
				if(f->p.f.excl) {
					debut_brut = (p0 + of7) * compactage;
					while(evt >= 0) {
						for(va=0; va<Evt[evt].nbvoies; va++) if(Evt[evt].voie[va].num == voie) break;
						if(va < Evt[evt].nbvoies) {
							if((Evt[evt].voie[va].debut + (int64)Evt[evt].voie[va].dim) <= debut_brut) break;
							p1 = Evt[evt].voie[va].adresse - of7;
							if(p1 < 0) {
								if(Evt[evt].voie[va].debut < VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien) evt = -1;
								else p1 += VoieTampon[voie].brutes->max;
							}
							ok = 0;
						}
						// printf("- Pb avec evt #%d: %ld + %d > %ld\n",evt,Evt[evt].debut,Evt[evt].dim,debut);
						if(--evt < 0) break;
					}
				}
				if(ok) {
					CalcPi[--k] = p0 + of7;
					// printf("- L'intervalle %d demarre a %g\n",k,CalcTi[k]);
					p1 = p0;
				}
			} while(k > 0);
			//				printf("Intervalles demandes: %d, non calcules: %d\n",f->p.f.intervalles,k);
			if(k < f->p.f.intervalles)
				ok = CalculeFFTVoie(FFT_PUISSANCE,trmt,voie,CalcPi+k,f->p.f.intervalles-k,f->axeX.pts,f->p.f.spectre[j],raison);
			else {
				if(p0 < 0) continue;
				sprintf(raison,"Pas d'intervalle > %d points entre les %d evenements",f->axeX.pts,EvtASauver);
				ok = 0;
			}
		#else /* !VERSION_MIXTE */
		#ifndef BCP_EVTS
			k = f->p.f.intervalles; if(k == 0) k = 1;
			evt = EvtASauver - 1;
			t1 = VoieTampon[voie].lus;
			do {
				debut = t1 - (int64)f->axeX.pts;
				//					printf("Debut echantillon a td=%d (premier en memoire=%d)\n",(int)debut,(int)VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien);
				if(debut <= VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien) break;
				ok = 1;
				if(f->p.f.excl) {
					while(evt >= 0) {
						trig = Evt[evt].voie_evt;
						if((Evt[evt].voie[trig].debut + (int64)Evt[evt].voie[trig].dim) <= debut) break;
						t1 = Evt[evt].voie[trig].debut;
						ok = 0;
						// printf("- Pb avec evt #%d: %ld + %d > %ld\n",evt,Evt[evt].debut,Evt[evt].dim,debut);
						if(--evt < 0) break;
					}
				}
				if(ok) {
					CalcPi[--k] = debut;
					// printf("- L'intervalle %d demarre a %g\n",k,CalcTi[k]);
					t1 = debut;
				}
			} while(k > 0);
			//				printf("Intervalles demandes: %d, non calcules: %d\n",f->p.f.intervalles,k);
			if(k < f->p.f.intervalles)
				ok = CalculeFFTVoie(FFT_PUISSANCE,trmt,voie,CalcPi+k,f->p.f.intervalles-k,f->axeX.pts,f->p.f.spectre[j],raison);
			else {
				if(debut <= VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien) continue;
				sprintf(raison,"Pas d'intervalle > %d points entre les %d evenements",f->axeX.pts,EvtASauver);
				ok = 0;
			}
		#else /* !BCP_EVTS */
			if(f->p.f.excl) {
				debut = VoieTampon[voie].lus - (int64)f->axeX.pts;
				ok = CalculeFFTVoie(FFT_PUISSANCE,trmt,voie,&debut,1,f->axeX.pts,f->p.f.spectre[j],raison);
			} else {
				ok = CalculeFFTVoie(FFT_PUISSANCE,trmt,voie,CalcPi,CalcNb,f->axeX.pts,f->p.f.spectre[j],raison);
			}
		#endif /* !BCP_EVTS */
		#endif /* !VERSION_MIXTE */
			unless(ok) {
				OpiumWarn("Fenetre '%s': %s",f->titre,raison);
				f->affiche = MONIT_HIDE;
			}
		}
		GraphAxisReset(g,GRF_YAXIS);
	} else peut_afficher = 0; /* f->type inconnu */

	if(peut_afficher) { OpiumDisplay(cdr); f->dernier_evt = Acquis[AcquisLocale].etat.evt_trouves; }
	if(doit_terminer) OpiumRefreshEnd(cdr);
}
/* ========================================================================== */
void LectDisplay() {
	int i,j,k,n,bolo,voie,vt,bb,v,catg;
	char trmt;
	TypeMonitFenetre *f; TypeMonitTrace *t; Graph g; // Cadre cdr;
	TypeMonitCatg *categ;
	TypeNumeriseur *numeriseur; Cadre cdr; Oscillo oscillo;
	float temps,expo;
	int sec_simule,usec_simule; 
	int64 t1,temps_total;
	int64 temps_utilise;
	clock_t cpu_utilise; // cpu_total;
	int64 evtmax,debut;
	int64 point0; float t0;
	char status_montre,affiche_evt,doit_terminer,taux_voie_change;
	Histo histo; float hmin,hbin; int nbins; int *adrs;
	float fond,amp,sigma,x0,val; float tevt;
	TypeVoieArchivee *triggee;
	int secs,usecs;
#ifdef A_RE_EXAMINER
	int64 periode;
#endif

	LectDansDisplay = 1;
	temps = 0.0; cpu_utilise = 0; temps_utilise = 0; taux_voie_change = 0;// GCC
	//- printf("(%s) Appel apres %lld synchroD2 lues\n",__func__,SynchroD2lues); // ImprimeStack;
/*
==== Planches de surveillances diverses
*/
	/* Ve 15.02.13: les planches deviennent !Displayed (??) apres LectStd: OpiumDisplayed(bNumeriseurEtat) && */
	for(bb=0; bb<NumeriseurNb; bb++) {
		numeriseur = &(Numeriseur[bb]);
		//* printf("(%s) %s: status %s\n",__func__,numeriseur->nom,numeriseur->status.a_copier?"demande":"ignore");
		if(numeriseur->status.a_copier) {
			for(i=0; i<numeriseur->status.nb; i++) NumeriseurRecopieStatus(numeriseur,i,numeriseur->status_val[i]);
			NumeriseurRafraichi(numeriseur);
			if(numeriseur->status.demande) {
				if(NumeriseurEtatDemande) NumeriseurRapportEtabli(numeriseur);
			}
		}
	}
	for(bb=0; bb<NUMER_ANONM_MAX; bb++) NumeriseurRafraichi(&(NumeriseurAnonyme[bb]));
	if(NumeriseurEtatDemande) {
		BoardRefreshVars(bNumeriseurEtat);
//		printf("(%s) Etat numeriseur rafraichi\n",__func__);
	}
	if(ReglagesEtatDemande) {
		PanelRefreshVars(pReglagesConsignes[LECT_CONS_ETAT]);
		PanelRefreshVars(pEtatDetecteurs);
	}

	if(LectModeSpectresAuto && (LectModeSpectresBanc <= 0)) CalcSpectreAutoAffiche();
	LectRepAffiches = 0; if(cRepartAcq && OpiumDisplayed(cRepartAcq)) LectRepAffiches = 1; if(LectRepAffiches) RepartiteurRafraichi();

/*
==== Planche "reglage detecteur"
*/

	if(ReglVi.actif) for(voie=0; voie<VoiesNb; voie++) VoieInfo[voie].compensable = 0;
	LectReglagesOuverts = 0;
	for(voie=0; voie<VoiesNb; voie++) if(VoieInfo[voie].planche && OpiumDisplayed(VoieInfo[voie].planche)) {
		LectReglagesOuverts = 1; status_montre = 0; // !(numeriseur->avec_status); assez vicieux, faut bien le dire
		doit_terminer = OpiumRefreshBegin(VoieInfo[voie].planche);
		if(LectLitStatus || LectDecodeStatus) for(j=VoieInfo[voie].prems; j<(VoieInfo[voie].prems+VoieInfo[voie].nb); j++) {
			TypeNumeriseur *numeriseur; int k;
			numeriseur = NumeriseurInstr[j].numeriseur; k = NumeriseurInstr[j].num;
			unless(numeriseur->ress[k].a_charger) {
				TypeReglage *regl;
				if(NumeriseurInstr[j].type == OPIUM_PANEL) {
					NumeriseurRessHexaChange(numeriseur,k);
					PanelRefreshVars((Panel)NumeriseurInstr[j].instrum);
				} else if(NumeriseurInstr[j].type == OPIUM_INSTRUM) {
					NumeriseurRessHexaChange(numeriseur,k);
					InstrumRefreshVar((Instrum)NumeriseurInstr[j].instrum);
				}
				if((regl = numeriseur->ress[k].regl)) {
					if((regl->cmde == REGL_D2) || (regl->cmde == REGL_D3) || (regl->cmde == REGL_GAIN)) ReglageEffetDeBord(regl,0);
				}
				if(numeriseur->avec_status && !status_montre) { VoieInfo[voie].validite = 1; status_montre = 1; }
			}
			if(status_montre) PanelRefreshVars(VoieInfo[voie].status);
		}
		if(VoieTampon[voie].lue <= 0) continue;
		if((oscillo = VoieInfo[voie].oscillo)) {
			int recul; int position; TypeDonnee *adrs;
			if(VoieManip[voie].def.trmt[TRMT_AU_VOL].gain == 0) VoieManip[voie].def.trmt[TRMT_AU_VOL].gain = 1;
			debut = OscilloCourbesDebut(oscillo,&recul);
			position = Modulo(debut,VoieTampon[voie].brutes->max);
			adrs = VoieTampon[voie].brutes->t.sval + position;
			if(!VoieManip[voie].modulee || oscillo->demodule) {
				double val,moyenne,bruit,min,max;
				moyenne = bruit = 0.0;
				min = max = 0.0; // pour gcc
				for(i=0, j=position; i<oscillo->dim; i++) {
					val = (double)*adrs++ / (double)VoieManip[voie].def.trmt[TRMT_AU_VOL].gain; j++;
					if(i == 0) min = max = val;
					else {
						if(val < min) min = val;
						if(val > max) max = val;
					}
					if(j == VoieTampon[voie].brutes->max) { j = 0; adrs = VoieTampon[voie].brutes->t.sval; }
					moyenne += val;
					bruit += (val * val);
				}
				if(oscillo->dim) {
					moyenne = moyenne / (double)oscillo->dim;
					VoieInfo[voie].moyenne = (float)moyenne;
					VoieInfo[voie].bruit = (float)(sqrt((bruit / (double)oscillo->dim) - (moyenne * moyenne)));
					// printf("(%s) S2=%g sur %d pts, m=%g soit bruit=%g\n",__func__,bruit,oscillo->dim,moyenne,VoieInfo[voie].bruit);
				} else VoieInfo[voie].moyenne = VoieInfo[voie].bruit = 0.0;
				VoieInfo[voie].deltaADU = (float)(VoieInfo[voie].trigger - moyenne);
				if(OpiumAlEcran(oscillo->pDelta)) PanelRefreshVars(oscillo->pDelta);
				VoieInfo[voie].pic2pic = (float)(max - min);
				VoieInfo[voie].mesure_enV = VoieInfo[voie].moyenne * VoieManip[voie].ADUenV;
				VoieInfo[voie].bruit_mV = (float)(1000.0 * VoieInfo[voie].bruit * VoieManip[voie].ADUenV);
				VoieInfo[voie].bruit_e = VoieInfo[voie].bruit * VoieManip[voie].ADUenE;
				if(OpiumAlEcran(VoieInfo[voie].electrons)) PanelRefreshVars(VoieInfo[voie].electrons);
			} else {
				int lngr,phase,parm,compte,modul,donnee;
				int valsup,valinf; float nbsup,nbinf;
				bolo = VoieManip[voie].det;
				modul = Bolo[bolo].d2; if(modul <= 2) modul = 200;
				lngr = (oscillo->dim / modul) * modul;
				if(lngr) parm = VoieManip[voie].def.trmt[TRMT_AU_VOL].p1;
				else { lngr = oscillo->dim; parm = 1; } // si modul est > oscillo->dim
				valsup = 0; nbsup = 0.0;
				valinf = 0; nbinf = 0.0;
				compte = 0; phase = 0;
				for(i=0, j=position; i<lngr; i++) {
					donnee = (int)*adrs++; j++;
					if(j == VoieTampon[voie].brutes->max) { j = 0; adrs = VoieTampon[voie].brutes->t.sval; }
					compte = compte + 1;
					switch(phase) {
						case 0:   /* phase 0: 1ere marge                    */
							if(compte > parm) { valsup += donnee; nbsup += 1.0; phase++; };
							break;
						case 1:   /* phase 1: 1ere demi-periode hors marges */
							if(compte <= ((modul / 2) /* pas de marge en fin de demi-periode */)) {
								valsup += donnee; nbsup += 1.0;
							} else phase++;
							break;
						case 2:  /* phase 2: marges inter-periodes         */
							if(compte > ((modul / 2) + parm)) { valinf += donnee; nbinf += 1.0; phase++; }
							break;
						case 3:  /* phase 3: 2eme demi-periode hors marges */
							valinf += donnee; nbinf += 1.0;
							if(compte == modul) { compte = 0; phase = 0; }
							break;
					}
				}
				if((nbsup > 0.0) && (nbinf > 0.0)) {
					VoieInfo[voie].moyenne = (float)0.5 * (((float)valsup / nbsup) + ((float)valinf / nbinf));
					VoieInfo[voie].bruit = (((float)valsup / nbsup) - ((float)valinf / nbinf));
				} else VoieInfo[voie].moyenne = VoieInfo[voie].bruit = 0.0;
				VoieInfo[voie].mesure_enV = VoieInfo[voie].bruit * VoieManip[voie].ADUenV;
				/* printf("(%s) demi mesure_enV = 1/2 * (bruit=%g) * (ADUenV=%g) = %g\n",__func__,VoieInfo[voie].bruit,VoieManip[voie].ADUenV,COMP_COEF * VoieInfo[voie].mesure_enV);
					printf("(%s) + (consigne=%g) = (compensation=%g)\n",__func__,DetecteurConsigneCourante(voie,REGL_COMP),(COMP_COEF * VoieInfo[voie].mesure_enV) + DetecteurConsigneCourante(voie,REGL_COMP)); */
			}
			VoieInfo[voie].compensable = 1;
			/* if(v == 0) {
			 ReglMoyenne = VoieInfo[voie].moyenne; if(!oscillo->demodule) ReglMoyenne *= 2;
			 ReglBruit = VoieInfo[voie].bruit;
			 ReglCompens = (COMP_COEF * VoieInfo[voie].mesure_enV) + DetecteurConsigneCourante(voie,REGL_COMP);
			 } */
			if(OpiumAlEcran(VoieInfo[voie].stats)) {
				/* les excitations (a en deduire) seront a donner en amplitude et non crete-crete, donc COMP_COEF=1/2 */
				if(VoieManip[voie].modulee) VoieInfo[voie].compens = (COMP_COEF * VoieInfo[voie].mesure_enV) + DetecteurConsigneCourante(voie,REGL_COMP);
				PanelRefreshVars(VoieInfo[voie].stats);
			}
			if(OpiumAlEcran(VoieInfo[voie].taux_evts)) PanelRefreshVars(VoieInfo[voie].taux_evts);
			DetecteurViMesuree(voie);
			if(OpiumAlEcran(oscillo->ecran)) {
				if(oscillo->bZero) {
					val = VoieInfo[voie].moyenne; //- if(!oscillo->demodule) val /= 2.0;
					oscillo->zero = (int)(val + 0.5);
					PanelRefreshVars(oscillo->pZero);
					InstrumRefreshVar(oscillo->iZero);
					OscilloChangeMinMax(oscillo);
					OscilloRefresh(0,oscillo);
					oscillo->bZero = 0;
				}
				if(oscillo->defile) OscilloCourbesTrace(oscillo,debut,recul);
				cdr = (oscillo->ecran)->cdr;
				if(cdr->volant) {
					// OpiumAligne(cdr,VoieInfo[voie].planche,cdr->defaultx,cdr->defaulty);
					OpiumActive(cdr);
				}
			}
			if(Trigger.actif && oscillo->h.actif && OpiumAlEcran(oscillo->h.graph)) {
				GraphAxisReset(oscillo->h.graph,GRF_YAXIS);
				cdr = (oscillo->h.graph)->cdr;
				OpiumRefresh(cdr);
				if(cdr->volant) {
					// OpiumAligne(cdr,VoieInfo[voie].planche,cdr->defaultx,cdr->defaulty);
					OpiumActive(cdr);
				}
			}
		}
		// OpiumRefresh(VoieInfo[voie].planche);
		if(doit_terminer) OpiumRefreshEnd(VoieInfo[voie].planche);
		/* break; */
	}
#ifdef GESTION_OSCILLO_SEPAREE
	if(LectReglagesOuverts) {
		int num;
		/* (OscilloReglage[num]->ecran && OpiumDisplayed((OscilloReglage[num]->ecran)->cdr)) simplifie.. */
		for(num=0; num<OscilloNb; num++) {
			oscillo = OscilloReglage[num];
			if(OpiumAlEcran(oscillo->ecran)) {
				voie = oscillo->voie[0];
				doit_terminer = OpiumRefreshBegin(VoieInfo[voie].planche);
				if(oscillo->bZero) {
					if((voie >= 0) && (voie < VoiesNb)) {
						val = VoieInfo[voie].moyenne;
						//- if(!oscillo->demodule) val /= 2.0;
						oscillo->zero = (int)(val + 0.5);
						PanelRefreshVars(oscillo->pZero);
						InstrumRefreshVar(oscillo->iZero);
						OscilloChangeMinMax(oscillo);
						OscilloRefresh(0,oscillo);
						oscillo->bZero = 0;
					}
				}
			} else doit_terminer = 0;
			debut = OscilloCourbesDebut(oscillo,&recul);
			if(OpiumAlEcran(oscillo->ecran) && oscillo->defile) OscilloCourbesTrace(oscillo,debut,recul);
			if(Trigger.actif && oscillo->h.actif && OpiumAlEcran(oscillo->h.graph)) {
				GraphAxisReset(oscillo->h.graph,GRF_YAXIS);
				OpiumRefresh((oscillo->h.graph)->cdr);
			}
			for(v=0; v<oscillo->nbvoies; v++) {
				int position; TypeDonnee *adrs;

				voie = oscillo->voie[v]; bolo = oscillo->bolo[v];
				if((voie < 0) || (voie >= VoiesNb)) { OpiumWarn("Oscilloscope connecte a la voie #%d",voie); continue; }
				if((bolo < 0) || (bolo >= BoloNb)) { OpiumWarn("Oscilloscope connecte au detecteur #%d",bolo); continue; }
				if(VoieTampon[voie].lue <= 0) continue;
				if(VoieManip[voie].def.trmt[TRMT_AU_VOL].gain == 0) VoieManip[voie].def.trmt[TRMT_AU_VOL].gain = 1;
				position = Modulo(debut,VoieTampon[voie].brutes->max);
				adrs = VoieTampon[voie].brutes->t.sval + position;
				// printf("(%s) P1=%d/%d soit adrs @%08X (%08X)\n",__func__,position,VoieTampon[voie].brutes->max,(hexa)adrs,(hexa)VoieTampon[voie].brutes->t.sval);
				// printf("(%s) %s %s modulee et oscillo en mode %s\n",__func__,VoieManip[voie].nom,VoieManip[voie].modulee?"est":"non",oscillo->demodule?"demodulation":"brut");
				if(!VoieManip[voie].modulee || oscillo->demodule) {
					double val,moyenne,bruit,min,max;
					// printf("(%s) evaluation signal DC\n",__func__);
					moyenne = bruit = 0.0;
					min = max = 0.0; // pour gcc
					for(i=0, j=position; i<oscillo->dim; i++) {
						val = (double)*adrs++ / (double)VoieManip[voie].def.trmt[TRMT_AU_VOL].gain; j++;
						if(i == 0) min = max = val;
						else {
							if(val < min) min = val;
							if(val > max) max = val;
						}
						if(j == VoieTampon[voie].brutes->max) { j = 0; adrs = VoieTampon[voie].brutes->t.sval; }
						moyenne += val;
						bruit += (val * val);
					}
					if(oscillo->dim) {
						moyenne = moyenne / (double)oscillo->dim;
						VoieInfo[voie].moyenne = (float)moyenne;
						VoieInfo[voie].bruit = sqrt((bruit / (double)oscillo->dim) - (moyenne * moyenne));
						// printf("(%s) S2=%g sur %d pts, m=%g soit bruit=%g\n",__func__,bruit,oscillo->dim,moyenne,VoieInfo[voie].bruit);
					} else VoieInfo[voie].moyenne = VoieInfo[voie].bruit = 0.0;
					VoieInfo[voie].deltaADU = VoieInfo[voie].trigger - moyenne;
					if(OpiumAlEcran(oscillo->pDelta)) PanelRefreshVars(oscillo->pDelta);
					VoieInfo[voie].pic2pic = max - min;
					VoieInfo[voie].mesure_enV = VoieInfo[voie].moyenne * VoieManip[voie].ADUenV;
					VoieInfo[voie].bruit_mV = 1000.0 * VoieInfo[voie].bruit * VoieManip[voie].ADUenV;
					VoieInfo[voie].bruit_e = VoieInfo[voie].bruit * VoieManip[voie].ADUenE;
					if(OpiumAlEcran(VoieInfo[voie].electrons)) PanelRefreshVars(VoieInfo[voie].electrons);
					/*
					 if(resultat->sigma) gain = resultat->moyenne / resultat->sigma; else gain = HUGE;
					 bruit = signoir * gain;
					 sprintf(ligne,"   %-3.1f    %-4.1f",gain,bruit);
					 #ifdef GAINS_LOG
					 d = gain; if(d > 0.0) d = log10(gain); else d = -HUGE;
					 sprintf(ligne,"   %-3.1f    %-4.1f    %-5.3f",gain,bruit,d);
					 #endif
					 LogFloat("   gain: %g e/ADU, bruit: %g e\n",gain,bruit);
					 if(Listing[iCcd]) fprintf(Listing[iCcd],ligne); // fputs(ligne,Listing[iCcd])
					 */
				} else {
					int lngr,phase,parm,compte,modul,donnee;
					// int valeur; float nbdata;
					int valsup,valinf; float nbsup,nbinf;
					modul = Bolo[bolo].d2; if(modul <= 2) modul = 200;
					lngr = (oscillo->dim / modul) * modul;
					if(lngr) parm = VoieManip[voie].def.trmt[TRMT_AU_VOL].p1;
					else { lngr = oscillo->dim; parm = 1; } // si modul est > oscillo->dim
					valsup = 0; nbsup = 0.0;
					valinf = 0; nbinf = 0.0;
					compte = 0; phase = 0;
					// printf("(%s) evaluation modulation, marge=%d/%d/%d\n",__func__,parm,lngr,oscillo->dim);
					for(i=0, j=position; i<lngr; i++) {
						donnee = (int)*adrs++; j++;
						if(j == VoieTampon[voie].brutes->max) { j = 0; adrs = VoieTampon[voie].brutes->t.sval; }
						compte = compte + 1;
						switch(phase) {
						  case 0:   /* phase 0: 1ere marge                    */
							if(compte > parm) { valsup += donnee; nbsup += 1.0; phase++; }
							break;
						  case 1:   /* phase 1: 1ere demi-periode hors marges */
							if(compte <= ((modul / 2) /* pas de marge en fin de demi-periode */)) {
								valsup += donnee; nbsup += 1.0;
							} else phase++;
							break;
						  case 2:  /* phase 2: marges inter-periodes         */
							if(compte > ((modul / 2) + parm)) { valinf += donnee; nbinf += 1.0; phase++; }
							break;
						  case 3:  /* phase 3: 2eme demi-periode hors marges */
							valinf += donnee; nbinf += 1.0;
							if(compte == modul) { compte = 0; phase = 0; }
							break;
						}
					}
					if((nbsup > 0.0) && (nbinf > 0.0)) {
						VoieInfo[voie].moyenne = 0.5 * (((float)valsup / nbsup) + ((float)valinf / nbinf));
						VoieInfo[voie].bruit = (((float)valsup / nbsup) - ((float)valinf / nbinf));
					} else VoieInfo[voie].moyenne = VoieInfo[voie].bruit = 0.0;
					VoieInfo[voie].mesure_enV = VoieInfo[voie].bruit * VoieManip[voie].ADUenV;
				}
				VoieInfo[voie].compensable = 1;
				if(OpiumAlEcran(VoieInfo[voie].stats)) {
					/* les excitations (a en deduire) seront a donner en amplitude et non crete-crete, donc COMP_COEF=1/2 */
					if(VoieManip[voie].modulee) VoieInfo[voie].compens = (COMP_COEF * VoieInfo[voie].mesure_enV) + DetecteurConsigneCourante(voie,REGL_COMP);
					PanelRefreshVars(VoieInfo[voie].stats);
				}
				if(OpiumAlEcran(VoieInfo[voie].taux_evts)) PanelRefreshVars(VoieInfo[voie].taux_evts);
				DetecteurViMesuree(voie);
			}
			if(doit_terminer) OpiumRefreshEnd(VoieInfo[voie].planche);
		}
	}
#endif /* GESTION_OSCILLO_SEPAREE */

	if(ReglVi.actif) {
		char termine; int m;
		termine = 1;
		for(v=0; v<ReglVi.nb_voies; v++) {
			voie = ReglVi.voie[v].num;
			if(VoieInfo[voie].compensable) DetecteurCompenseVoie(0,(MenuItem)(long)voie);
			else if(VoieTampon[voie].signal.mesure) {
				if(VoieTampon[voie].signal.nb) VoieInfo[voie].moyenne = VoieTampon[voie].signal.somme / (float)VoieTampon[voie].signal.nb;
				else VoieInfo[voie].moyenne = 0.0;
				VoieInfo[voie].mesure_enV = VoieInfo[voie].moyenne * VoieManip[voie].ADUenV;
				/* les excitations (a en deduire) seront a donner en amplitude et non crete-crete, donc COMP_COEF=1/2 */
				DetecteurViMesuree(voie);
				DetecteurCompenseVoie(0,(MenuItem)(long)voie); // remet VoieTampon[voie].signal.mesure a 0
			} else termine = 0;
			// DetecteurViInsere();
		}
		if(termine) {
			if(ReglVi.nb_mesures == 0) {
				printf("\n");
				for(v=0; v<ReglVi.nb_voies; v++) printf(" _______________________"); printf("\n");
				printf("| V(I) a T =%5.1f mK    ",*T_Bolo * 1000.0); for(v=0; v<ReglVi.nb_voies-1; v++) printf("                        "); printf("|\n");
				for(v=0; v<ReglVi.nb_voies; v++) printf("|_______________________"); printf("|\n");
				for(v=0; v<ReglVi.nb_voies; v++) printf("| %-22s",VoieManip[ReglVi.voie[v].num].nom); printf("|\n");
				for(v=0; v<ReglVi.nb_voies; v++) printf("| I(nA)   V(mV)   R(MO) "); printf("|\n");
				for(v=0; v<ReglVi.nb_voies; v++) printf("|_______|_______|_______"); printf("|\n");
			}
			m = ReglVi.nb_mesures;
//-			for(v=0; v<ReglVi.nb_voies; v++) printf(" _______________________"); printf("\n"); // temporaire
			for(v=0; v<ReglVi.nb_voies; v++) {
				voie = ReglVi.voie[v].num;
				printf("|%6.3f |%6.3f |%6.3f ",VoieInfo[voie].I,VoieInfo[voie].V,VoieInfo[voie].R);
				if(m < MAXVI) {
					ReglVi.voie[voie].I[m] = VoieInfo[voie].I;
					ReglVi.voie[voie].V[m] = VoieInfo[voie].V;
					ReglVi.voie[voie].R[m] = VoieInfo[voie].R;
				}
			}
			printf("|\n");
//-			for(v=0; v<ReglVi.nb_voies; v++) printf("|_______|_______|_______"); printf("|\n"); // temporaire
			ReglVi.nb_mesures++;
			for(bolo=0; bolo<BoloNb; bolo++) if(ReglVi.g[bolo]) {
				GraphUse(ReglVi.g[bolo],ReglVi.nb_mesures);
				GraphAxisReset(ReglVi.g[bolo],GRF_XAXIS);
				GraphAxisReset(ReglVi.g[bolo],GRF_YAXIS);
				OpiumDisplay(ReglVi.g[bolo]->cdr);
			}
			if(ReglVi.oper) ReglVi.cur *= ReglVi.pas; else ReglVi.cur += ReglVi.pas;
			if((ReglVi.cur < (ReglVi.arr + 0.1)) /* pb d'arrondi */ && Acquis[AcquisLocale].etat.active) {
				TypeReglage *rampl;
				for(v=0; v<ReglVi.nb_voies; v++) {
					voie = ReglVi.voie[v].num;
					if((rampl = ReglagePtr(voie,REGL_MODUL))) ReglageChangeFloat(rampl,ReglVi.cur,0);
				}
				SambaAttends(ReglVi.attente*1000);
			} else {
				for(v=0; v<ReglVi.nb_voies; v++) printf("|_______|_______|_______"); printf("|\n");
				ReglVi.actif = 0;
				if(ReglVi.arrete) Acquis[AcquisLocale].etat.active = 0;
			}
		}
	}

    SambaTempsEchantillon((LectStampPrecedents + LectStampsLus),0,0,&sec_simule,&usec_simule);
    S_usPrint(Acquis[AcquisLocale].etat.duree_lue,sec_simule,usec_simule);

	/*
	 ==== Taux d'evenements et trace de la derniere voie ayant trigge
	 */
	taux_voie_change = 0;
	if(Trigger.enservice) {
		gettimeofday(&LectDateRun,0);
		if(LectDateRun.tv_sec > LectProchainTaux) {
			if(SettingsTauxPeriode) LectProchainTaux += (long)SettingsTauxPeriode;
			else LectProchainTaux = LectDateRun.tv_sec;
			if(sec_simule) LectCntl.TauxGlobal = (float)Acquis[AcquisLocale].etat.evt_trouves / (float)sec_simule;
			/* Valeur utilisee s'il n'y a pas au moins un evenement nouveau */
			/* Normalement, TousLesEvts.trouves == Acquis[AcquisLocale].etat.evt_trouves */
			LectCalculeTaux(&TousLesEvts);
			if(LectStrictStd && !LectReglagesOuverts && OpiumDisplayed(bSuiviEvts)) {
				if(OpiumAlEcran(gEvtSolo)) OpiumClear(gEvtSolo->cdr);
				for(catg=0; catg<MonitCatgNb+1; catg++) {
					categ = MonitCatgPtr[catg];
					LectCalculeTaux(categ);
					//- printf("(%s) %s @ %g Hz\n",__func__,categ->nom,categ->freq);
					if(categ->evol && categ->activite.val) {
						Graph g;
						if(categ->activite.nb < categ->activite.max) i = (categ->activite.nb)++;
						else {
							i = (categ->activite.prem)++;
							if(categ->activite.prem >= categ->activite.max) categ->activite.prem = 0;
						}
						categ->activite.val[i] = categ->freq;
						if((g = categ->gSuivi)) {
							GraphUse(g,categ->activite.nb);
							if((categ->activite.nb == categ->activite.max) && (categ->activite.max > 2000)) {
								GraphZoom(g,categ->activite.max/1000,1);
								GraphRescroll(g,categ->activite.prem);
							}
							if(MonitEvtClasses && !LectReglagesOuverts && OpiumDisplayed(g->cdr)) GraphAxisFloatMin(g,GRF_YAXIS,0.0);
							OpiumRefresh(g->cdr);
						}
					}
					PanelRefreshVars(categ->pTaux);
					PanelRefreshVars(categ->pMinMax); PanelRefreshVars(categ->pEvtval);
				}
			}

			for(voie=0; voie<VoiesNb; voie++) {
				int64 delai;
				delai = VoieTampon[voie].date_dernier - VoieTampon[voie].date_ref_taux;
				if(delai) VoieTampon[voie].taux = (float)VoieTampon[voie].nbevts / ((float)delai * EnSecondes);
				else VoieTampon[voie].taux = 1.0 / ((float)(DernierPointTraite - VoieTampon[voie].date_dernier) * EnSecondes);
				if(VerifCalculTauxEvts) printf("Evt[%d]=%d sur %s, total %d a %lld depuis %lld donc sur %lld ms (soit %.3f/s)\n",
					EvtAffichable,Evt[EvtAffichable].num,VoieManip[voie].nom,VoieTampon[voie].nbevts,
					VoieTampon[voie].date_dernier,VoieTampon[voie].date_ref_taux,delai,VoieTampon[voie].taux);
				VoieTampon[voie].nbevts = 0; VoieTampon[voie].date_ref_taux = VoieTampon[voie].date_dernier;
			}
			//?? if((MonitAffpTaux && pTauxDetaille) || (MonitAffgTaux && gTauxDetaille)) {
				if(MonitAffgTaux && gTauxDetaille) { LectStockeTauxVoies(); taux_voie_change = 1; }
			// }
			if(SambaInfos) {
				TypeSambaCourbe *courbe;
				courbe = &(SambaInfos->courbe[SAMBA_SHR_SUIVI]);
				if(courbe->catg >= 0) {
					categ = MonitCatgPtr[courbe->catg];
					if(courbe->nb < SAMBA_SHR_MAXPTS) i = (courbe->nb)++;
					else { i = (courbe->prem)++; if(courbe->prem >= SAMBA_SHR_MAXPTS) courbe->prem = 0; }
					courbe->val[i] = categ->freq;
					if(courbe->max < categ->freq) courbe->max = categ->freq;
				}
			}
			if(TousLesEvts.freq > LectTauxSeuil) {
				// OpiumFail("Taux d'evenements EXCESSIF !! (%g / %g Hz)",TousLesEvts.freq,LectTauxSeuil);
				printf("%s/ Taux d'evenements en exces, a %g Hz (contre %g Hz permis)\n",DateHeure(),TousLesEvts.freq,LectTauxSeuil);
				LectDeclencheErreur(_ORIGINE_,70,LECT_EVTS);
			}
		}
		/* if(OpiumDisplayed(bLecture)) {
			double mag,nbchiffres; float min,max;
			if(TousLesEvts.freq > 0.0) {
				mag = log10((double)TousLesEvts.freq);
				modf(mag,&nbchiffres);
				min = (float)pow(10.0,nbchiffres);
				if(min < 0.01) min = 0.01;
			} else min = 0.01;
			if(min != LectTauxMin) {
				max = 9.99 * min;
				InstrumRLimits(cTaux,min,max);
				if((min > 0.01) && (min < 100.0)) InstrumGradSet(cTaux,min);
				else InstrumGradSet(cTaux,2.0*min);
				OpiumRefresh(cTaux->cdr);
				LectTauxMin = min;
			} else InstrumRefreshVar(cTaux);
		} */
		if(OpiumAlEcran(pTauxGlobal)) PanelRefreshVars(pTauxGlobal);
		affiche_evt = (MonitAffEvts && !LectReglagesOuverts);
		// affiche_evt &&= LectAffEvt; pour ralentir l'affichage, procede abandonne
		// affiche_evt &&= (LectCntl.MonitEvtNum != MonitEvtAlEcran); fait dans MonitEvtAffiche
		// affiche_evt = affiche_evt && OpiumDisplayed((gEvtRunCtrl->cdr)->planche) && MonitEvtAvecRun; fait dans MonitEvtAffiche
	#ifdef PARMS_RESEAU
		affiche_evt = affiche_evt && (EvtPartage->num > LectLastEvt);
	#endif
		// printf("(%s) ================> Affichage des evenements %s\n",__func__,affiche_evt?"demande":"ignore");
		if(affiche_evt) {
		#ifdef PARMS_RESEAU
			if(SambaMode == SAMBA_DISTANT) {
				CarlaDataRequest(AcqCible,IdEvtPartage);
				LectLastEvt = EvtPartage->num;

			} else {
		#endif
				MonitEvtAffiche(0,/* EvtAffichable */0,0,0); /* risque de confusion avec juste l'index (EvtAffichable) */
				LectLastEvt = Acquis[AcquisLocale].etat.evt_trouves; // LectAffEvt = 0;
		#ifdef PARMS_RESEAU
				if(SambaMode == SAMBA_SATELLITE) CarlaDataUpdate(IdEvtPartage);
				else if(SambaMode == SAMBA_MAITRE) LectAfficheEvt();
			}
		#endif
		} //? else PanelRefreshVars(pLectEvtNum);
		if(MonitEvtClasses && !LectReglagesOuverts) for(catg=0; catg<MonitCatgNb+1; catg++) {
			categ = MonitCatgPtr[catg];
			if(categ->utilisee
			&& ((categ->trace && (MonitCategFocus < 0)) || ((catg == MonitCategFocus) && !MonitFigeFocus))
			&& (categ->affichable > 0) && (categ->affichable != categ->affiche)) {
				GraphAxisLog(categ->gEvt,GRF_YAXIS,MonitLogFocus);
				MonitEvtAffiche(categ,0,0,0);
			}
		}
		// printf("(Samba:%s) Courbe#%d sur catg %d: %d points\n",__func__,SAMBA_SHR_FOCUS,catg,SambaInfos->courbe[SAMBA_SHR_FOCUS].nb);
		if(SambaInfos) {
			TypeEvt *evt; TypeVoieArchivee *archivee; float *trace; int evtindex,dim,vt,voie;
			evtindex = LectCntl.MonitEvtNum - Evt[0].num;
			if((evtindex >= 0) && (evtindex < EvtASauver)) {
				evt = &(Evt[evtindex]);
				if(evt->categ == SambaShrCatg[SAMBA_SHR_FOCUS]) {
					vt = evt->voie_evt; archivee = &(evt->voie[vt]); voie = archivee->num;
					dim = VoieEvent[voie].lngr; if(dim > SAMBA_SHR_MAXPTS) dim = SAMBA_SHR_MAXPTS;
					if(evt->deconvolue) trace = evt->deconvolue;
					else {
						VoieTampon[voie].deconvoluee = SambaAssure(VoieTampon[voie].deconvoluee,VoieEvent[voie].lngr,&(VoieTampon[voie].dim_deconv));
						if(VoieTampon[voie].deconvoluee) trace = VoieTampon[voie].deconvoluee;
						else trace = 0;
					}
					if(trace) {
					SambaInfos->courbe[SAMBA_SHR_FOCUS].min = SambaInfos->courbe[SAMBA_SHR_FOCUS].max = trace[0];
					int i; for(i=0; i<dim; i++) {
						SambaInfos->courbe[SAMBA_SHR_FOCUS].val[i] = trace[i];
						if(SambaInfos->courbe[SAMBA_SHR_FOCUS].min > trace[i])
							SambaInfos->courbe[SAMBA_SHR_FOCUS].min = trace[i];
						if(SambaInfos->courbe[SAMBA_SHR_FOCUS].max < trace[i])
							SambaInfos->courbe[SAMBA_SHR_FOCUS].max = trace[i];
					}
					SambaInfos->courbe[SAMBA_SHR_FOCUS].nb = dim;
					}
				}
			}
		}

	} //? else PanelRefreshVars(pLectEvtNum);
	if(TrmtApanique || LectAffichePanique) {
		if(!LectAffichee[0]) strcpy(LectAffichee,&(LectInfo[0][0]));
		if(LectPaniqueAffichee) strcpy(LectInfo[0],L_("Debordement des donnees, temps mort en cours")); // "Data overflow, dead time on air"
		else LectInfo[0][0] = '\0';
		PanelItemIScale(pLectRegen,1,0,1);
		PanelItemColors(pLectRegen,1,SambaNoirOrange,2);
		if(OpiumAlEcran(pLectRegen)) OpiumRefreshPanel(pLectRegen->cdr); // PanelRefreshVars(pLectRegen);
		LectPaniqueAffichee = 1 - LectPaniqueAffichee;
	} else if(LectAffichee[0]) {
		strcpy(&(LectInfo[0][0]),LectAffichee); LectAffichee[0] = '\0';
		PanelItemColors(pLectRegen,1,0,0);
		if(OpiumAlEcran(pLectRegen)) OpiumRefreshPanel(pLectRegen->cdr); // PanelRefreshVars(pLectRegen);
	}

/*
 ==== Informations en memoire partagee (a l'usage du serveur)
 */
	if(SambaInfos) {
		strncpy(SambaInfos->origine,__func__,SAMBA_SHR_FCTN);
		sprintf(SambaInfos->validite,"%s %s",DateCivile(),DateHeure());
		SambaInfos->trigger_demande  = Trigger.demande;
		SambaInfos->archive_demandee = Archive.demandee;
		SambaInfos->en_cours    = Acquis[AcquisLocale].etat.active;
		SambaInfos->evts_vus    = Acquis[AcquisLocale].etat.evt_trouves;
		SambaInfos->evts_sauves = Acquis[AcquisLocale].etat.evt_ecrits;
		SambaInfos->taux_actuel = TousLesEvts.freq;
		SambaInfos->taux_global = LectCntl.TauxGlobal;
		strncpy(SambaInfos->run,Acquis[AcquisLocale].etat.nom_run,12);
		strncpy(SambaInfos->duree,Acquis[AcquisLocale].etat.duree_lue,12);
		SambaInfos->sessions    = LectSession;
		SambaInfos->nb_categs   = MonitCatgNb+1;
		int catg; for(catg=0; catg<MonitCatgNb+1; catg++) {
			strcpy(SambaInfos->categ[catg].nom,MonitCatgPtr[catg]->nom);
			SambaInfos->categ[catg].freq = MonitCatgPtr[catg]->freq;
		}
	}

	if(LectMaintenance || LectModeSpectresAuto) goto monite;
	if(LectReglageEnCours) { LectReglageEnCours = 0; OpiumFork(bLecture); }

#ifdef MODULES_RESEAU
/*
==== Machines distantes: demander l'etat de la lecture
*/
	if(SambaMode == SAMBA_DISTANT) {
	#ifdef PARMS_RESEAU
		CarlaDataRequest(AcqCible,IdCntlLecture);
	#endif
//		printf("Dernier evenement pris: %d, affiche: %d\n",Acquis[AcquisLocale].etat.evt_trouves,LectLastEvt);
		if(Acquis[AcquisLocale].etat.active != LectureActive) {
			LectureActive = Acquis[AcquisLocale].etat.active;
			if(Acquis[AcquisLocale].etat.active) {
				if(OpiumAlEcran(mLectDemarrage)) MenuItemAllume(mLectDemarrage,1,L_("En cours"),GRF_RGB_GREEN);
				if(OpiumAlEcran(mLectArret)) MenuItemAllume(mLectArret,1,L_("Stopper "),GRF_RGB_YELLOW);
				if(OpiumAlEcran(mLectSuiviStart)) MenuItemAllume(mLectSuiviStart,1,L_("En cours"),GRF_RGB_GREEN);
				if(OpiumAlEcran(mLectSuiviStop)) MenuItemAllume(mLectSuiviStart,1,L_("Stopper "),GRF_RGB_YELLOW);
			} else {
				if(OpiumAlEcran(mLectDemarrage)) MenuItemAllume(mLectDemarrage,1,L_("Demarrer"),GRF_RGB_YELLOW);
				if(OpiumAlEcran(mLectArret)) MenuItemEteint(mLectArret,1,L_("Arrete  "));
				if(OpiumAlEcran(mLectSuiviStart)) MenuItemAllume(mLectSuiviStart,1,L_("Demarrer"),GRF_RGB_YELLOW);
				if(OpiumAlEcran(mLectSuiviStop)) MenuItemEteint(mLectSuiviStart,1,L_("Arrete  "));
			}
		}
/*		Si poussoir 'suspendre/activer trigger' dans planche bLecture
		if(Trigger.actif != TriggerActif) {
			TriggerActif = Trigger.actif;
			if(TriggerActif) MenuItemAllume(mSettingsTriggerCtrl,1,"Suspendre",GRF_RGB_GREY);
			else MenuItemAllume(mSettingsTriggerCtrl,1," Activer ",GRF_RGB_YELLOW);
			printf("%s/ Recherche d'evenements %s\n",DateHeure(),TriggerActif? "reactivee": "suspendue");
		} */
	} else
#endif
/*
==== sinon (machines en ligne): calcul d'un t0 pour l'affichage des signaux
*/
	if(VerifDisplay)  temps = ((float)LectStampsLus / (Echantillonnage * 1000.0)) - MonitT0; /* decalage: "actuel" - "debut buffers" (secondes) */
#ifdef T0_SI_AFFICHE
	t0 = -1.0;
	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue > 0) {
		for(trmt=0; trmt<TRMT_NBCLASSES; trmt++) if(VoieTampon[voie].trmt[trmt].affichee) {
			float tvoie;
			tvoie = (float)VoieTampon[voie].trmt[trmt].point0 * VoieTampon[voie].trmt[trmt].ab6[1];
			VoieTampon[voie].trmt[trmt].ab6[0] = tvoie;
			if((t0 < 0.0) || (t0 > tvoie)) t0 = tvoie;
			if(VerifDisplay) printf("(%s)  Point0[%s %s]=%lld, soit X0=%g (inf{X0}=%g)\n",__func__,
				VoieManip[voie].nom,TrmtClasseTitre[trmt],
				VoieTampon[voie].trmt[trmt].point0,VoieTampon[voie].trmt[trmt].ab6[0],t0);
		}
	}
	if(t0 >= 0.0) {
		point0 = (int64)((t0 + 500.0) / 1000.0); /* arrondi a un nombre entier de secondes */
		MonitT0 = (float)point0; /* attention, ceci est en SECONDES (pour un affichage plus sympa) */
	}
	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue > 0) {
		for(trmt=0; trmt<TRMT_NBCLASSES; trmt++) if(VoieTampon[voie].trmt[trmt].affichee) VoieTampon[voie].trmt[trmt].ab6[0] -= (MonitT0 * 1000.0);
		if(VerifDisplay) {
			for(trmt=0; trmt<TRMT_NBCLASSES; trmt++) if(VoieTampon[voie].trmt[trmt].affichee)
				printf("              D'ou X0[%s %s]=%g\n",VoieManip[voie].nom,TrmtClasseTitre[trmt],VoieTampon[voie].trmt[trmt].ab6[0]);
		}
	}
#else
	if((LectStampsLus - MonitP0) >= (2 * (int64)LngrTampons)) {
		MonitP0 += (int64)LngrTampons;
		t0 = (float)MonitP0 / Echantillonnage;       /* ms */
		point0 = (int64)((t0 + 500.0) / 1000.0);     /* arrondi a un nombre entier de secondes */
		MonitP0 = point0 * 1000.0 * Echantillonnage; /* absolus */
		MonitT0 = (float)point0;                     /* attention, ceci est en SECONDES (pour un affichage plus sympa) */
	}
	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue > 0) {
		for(trmt=0; trmt<TRMT_NBCLASSES; trmt++) if(VoieTampon[voie].trmt[trmt].affichee)
			VoieTampon[voie].trmt[trmt].ab6[0] = (float)(VoieTampon[voie].trmt[trmt].point0 - (MonitP0 / VoieTampon[voie].trmt[TRMT_AU_VOL].compactage)) * VoieTampon[voie].trmt[trmt].ab6[1];
		if(VerifDisplay) for(trmt=0; trmt<TRMT_NBCLASSES; trmt++) if(VoieTampon[voie].trmt[trmt].affichee)
			printf("              D'ou X0[%s %s]=%g\n",VoieManip[voie].nom,TrmtClasseTitre[trmt],VoieTampon[voie].trmt[trmt].ab6[0]);
	}
#endif
	if(VerifDisplay && (temps > 100.0)) VerifDisplay = 0;
	/*
	 ==== et mise a jour des differentes variables scalaires
	 */
	expo = (float)(LectStampPrecedents + LectStampsLus - TrmtPasActif) / (Echantillonnage / 1000.0); /* microsecondes */
	Acquis[AcquisLocale].etat.KgJ = MasseTotale * expo * gusecToKgJ;
	if(LectEntretien) {
		if(LectEntretienEnCours) {
			int attente,derniere;
			derniere = 0;
			for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire && ((attente = (Bolo[bolo].exec.date - LectDateRun.tv_sec)) > derniere)) derniere = attente;
			sprintf(LectInfo[1],L_("Pause entretien: %d s"),derniere);
		} else if(LectDateRegulBolo > LectDateRun.tv_sec) sprintf(LectInfo[1],L_("Entretien detecteurs dans %ld s"),LectDateRegulBolo - LectDateRun.tv_sec);
		else strcpy(LectInfo[1],L_("Entretien detecteurs: maintenant"));
	}
	if(FifoTotale) LectCntl.LectTauxPile = (LectMaxDepilees * 100) / FifoTotale;
	LectMaxDepilees = 0;
	
    temps_total = 0;
	if(VerifConsoCpu) {
		cpu_utilise = CpuTempoLect + CpuTempoTrmt + CpuTempoArch;
	//	c0 = clock(); cpu_total = c0 - CpuTempoRun; CpuTempoRun = c0;
		t1 = DateMicroSecs(); temps_total = t1 - LectTempoRun; LectTempoRun = t1;
		CpuTempoLect = CpuTempoTrmt = CpuTempoArch = 0;
	} else if(VerifTempsPasse) {
		temps_utilise = TempsTempoLect + TempsTempoTrmt + TempsTempoArch;
		t1 = DateMicroSecs(); temps_total = t1 - LectTempoRun; LectTempoRun = t1;
		TempsTempoLect = TempsTempoTrmt = TempsTempoArch = 0;
	} else CpuTauxOccupe = 0;
	if(temps_total) {
		if(VerifTempsPasse) CpuTauxOccupe = (int)((temps_utilise  * 100) / temps_total);
		else if(VerifConsoCpu) CpuTauxOccupe = (int)((cpu_utilise  * 100) / ((clock_t)temps_total * CpuActifs));
	}
	
	if(LectStampsLus) {
		PerteIP = ((float)LectStampPerdus * 100.0) / (float)LectStampsLus;
		TempsMort = ((float)LectStampJetes * 100.0) / (float)LectStampsLus;
	}
	n = EvtNbCoupes + EvtNbIncoherents + Acquis[AcquisLocale].etat.evt_trouves;
	if(n > 0)
		PerteEvts = (100.0 * (float)(PileUp + TrmtEvtJetes + TrmtNbEchecs + ArchEvtsPerdus)) / (float)n;
	else PerteEvts = 0.0;
	if(OpiumDisplayed(bLecture)) {
		doit_terminer = OpiumRefreshBegin(bLecture);
		PanelRefreshVars(pLectEvtNum); // pour MonitT0 mais melange avec infos evt affiche - ou pas
		PanelRefreshVars(pEtatTrigger);
		PanelRefreshVars(pLectEcoule);
		PanelRefreshVars(pLectRegen);
		InstrumRefreshVar(cLectTauxPile);
		InstrumRefreshVar(cLectPerteIP);
		InstrumRefreshVar(cCpuTauxOccupe);
		InstrumRefreshVar(cTempsMort);
		InstrumRefreshVar(cPerteEvts);
		PanelRefreshVars(pLectPileMax);
		if(doit_terminer) OpiumRefreshEnd(bLecture);
	}
	if(SambaSat && (EcritureMaitre >= 0)) {
		byte buffer[80]; int i,n;
		i = 0;
		i += SambaBufferiseShort(buffer+i,SAT_ETAT);
		i += SambaBufferiseShort(buffer+i,(short)AcquisLocale);
		i += SambaBufferiseTexte(buffer+i,Acquis[AcquisLocale].etat.nom_run,RUN_NOM);
		i += SambaBufferiseTexte(buffer+i,Acquis[AcquisLocale].etat.duree_lue,20);
		i += SambaBufferiseInt(buffer+i,Acquis[AcquisLocale].etat.evt_trouves);
		n = write(EcritureMaitre,buffer,i);
		/* {	int j;
			printf("%s/ Ecrit %d/%d: [",DateHeure(),n,i);
			for(j=0; j<i; j++) printf("%c%02X",j?'.':' ',buffer[j]);
			printf(" ]\n");
		} */
	}
#ifndef STATUS_AVEC_TIMER
	if(pLectEtat) { if(OpiumDisplayed(pLectEtat->cdr)) PanelRefreshVars(pLectEtat); }
#endif

/*
==== Etat des acquisitions satellites

#ifdef MODULES_RESEAU
	if((SambaMode == SAMBA_MAITRE) && (SynchroD2lues >= LectNextSat) && OpiumDisplayed(pAcquisEtat->cdr)) {
		int sat;
		OpiumRefresh(pAcquisEtat->cdr);
		LectNextSat = SynchroD2lues + LectIntervSat;
		for(sat=1; sat<AcquisNb; sat++) if(Acquis[sat].etat.status > SATELLITE_ABSENT) {
			Acquis[sat].etat.status = SATELLITE_ABSENT;
		#ifdef ETAT_RESEAU
			CarlaDataRequest(Acquis[sat].adrs,Acquis[sat].id);
		#endif
		}
	}
#endif
*/

/*
==== Graphiques utilisateur
*/
monite:
	if((LectCntl.LectMode == LECT_IDENT) 
	|| (LectCntl.LectMode == LECT_COMP)) goto conclusion;
	if(LectCompacteUtilisee && (LectCompacteNumero == LECT_COMPACTE_DONNEES)) {
		if(OpiumAlEcran(LectOscilloSignal.ecran) && LectOscilloSignal.bZero) {
			int recul,position; TypeDonnee *adrs; double val,moyenne;
			voie = LectOscilloSignal.voie[0];
			debut = OscilloCourbesDebut(&LectOscilloSignal,&recul);
			position = Modulo(debut,VoieTampon[voie].brutes->max);
			adrs = VoieTampon[voie].brutes->t.sval + position;
			moyenne = 0.0;
			for(i=0, j=position; i<LectOscilloSignal.dim; i++) {
				val = (double)*adrs++ / (double)VoieManip[voie].def.trmt[TRMT_AU_VOL].gain; j++;
				if(j == VoieTampon[voie].brutes->max) { j = 0; adrs = VoieTampon[voie].brutes->t.sval; }
				moyenne += val;
			}
			if(LectOscilloSignal.dim) moyenne = moyenne / (double)LectOscilloSignal.dim;
			//- if(!LectOscilloSignal.demodule) val /= 2.0;
			LectOscilloSignal.zero = (int)(moyenne + 0.5);
			PanelRefreshVars(LectOscilloSignal.pZero);
			InstrumRefreshVar(LectOscilloSignal.iZero);
			OscilloChangeMinMax(&LectOscilloSignal);
			OscilloRefresh(0,&LectOscilloSignal);
			LectOscilloSignal.bZero = 0;
		}
		LectFenRefresh(&LectFenSignal);
		// LectFenRefresh(&LectFenEvt);
		LectFenRefresh(&LectFenSpectre);
		LectFenRefresh(&LectFenBruit);
	}
	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) LectFenRefresh(f);
	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if((f->affiche != MONIT_HIDE) && (g = f->g)) {
		if(LectMaintenance || LectModeSpectresAuto) { if(f->affiche != MONIT_ALWS) continue; }
		if(f->type == MONIT_SIGNAL) for(j=0, t=f->trace; j<f->nb; j++,t++) {
			voie = t->voie; if(VoieTampon[voie].lue <= 0) continue;
			trmt = (int)t->var;
			if(trmt == TRMT_PRETRG) VoieTampon[voie].trmt[trmt].point_affiche = VoieTampon[voie].trmt[trmt].point_vu;
		}
	}
			
/*
==== Spectre de bruit au vol
*/
	if(MonitFftActif && Acquis[AcquisLocale].etat.active) {
		int i,j,action;
		int64 pt0,lus,premier;
		char nom[80];
		fftw_real *ptr_temps; float unite;
		TypeDonnee *ptr_int16; TypeSignal *ptr_float; TypeTamponDonnees *tampon;

		tampon = &(VoieTampon[MonitFftVoie].trmt[MonitFftDonnees].donnees);
		if(!tampon->max) goto suite1;
        lus = 0; premier = 0;
		if(MonitFftDonnees == TRMT_AU_VOL) {
			// tampon = &(VoieTampon[MonitFftVoie].brutes);
			lus = VoieTampon[MonitFftVoie].lus;
			premier = VoieTampon[MonitFftVoie].trmt[TRMT_AU_VOL].plus_ancien;
			if((lus < (int64)MonitFftDim) && (premier <= 0)) goto suite1;
		} else if(MonitFftDonnees == TRMT_PRETRG) {
			// tampon = &(VoieTampon[MonitFftVoie].traitees);
			lus = tampon->prem + tampon->nb;
			premier = tampon->prem;
		}
		pt0 = lus - (int64)MonitFftDim;
		/* if(pt0 < premier) {
			pt0 = premier;
			if((pt0 + (int64)MonitFftDim) > lus) MonitFftDim = (int)(lus - pt0);
		} */
		if(pt0 < premier)  goto suite1;
		ptr_temps = MonitFftBase.temps;
		j = Modulo(pt0,tampon->max);
		if(tampon->type == TAMPON_INT16) {
			ptr_int16 = tampon->t.sval + j;
			for(i=0; i<MonitFftDim; i++) {
				*ptr_temps++ = (fftw_real)*ptr_int16++;
				if(++j >= tampon->max) { j = 0; ptr_int16 = tampon->t.sval; }
			}
		} else {
			ptr_float = tampon->t.rval + j;
			for(i=0; i<MonitFftDim; i++) {
				*ptr_temps++ = (fftw_real)*ptr_float++;
				if(++j >= tampon->max) { j = 0; ptr_float = tampon->t.rval; }
			}
		}
		rfftw_one(MonitFftBase.plan,MonitFftBase.temps,MonitFftBase.freq);
		action = 2;
		if((MonitFftMode == CALC_SPTV_MANU) || (MonitFftMode == CALC_SPTV_SURV)) {
	/*		{
				printf("Nouvelle FFT @%08X -> %08X:",MonitFftBase.freq,MonitFftNouveau);
				for(i=0; i<MonitFftDim; i++) {
					if(!(i % 10)) printf("\n%4d|",i);
					printf(" %10.0f",MonitFftBase.freq[i]);
				}
				printf("\n");
			}*/
			CalculePuissance(MonitFftBase.freq,MonitFftNouveau,MonitFftDim,MonitFftNorme,VoieManip[MonitFftVoie].ADUennV);
			if(MonitFftMode == CALC_SPTV_MANU) {
				if(MonitFftNbSpectres < 0.5) {
					GraphDataReplace(gMonitFftAuVol,1,GRF_FLOAT|GRF_YAXIS,MonitFftNouveau,MonitFftNbFreq);
					GraphDataRGB(gMonitFftAuVol,1,GRF_RGB_YELLOW);
					GraphDataName(gMonitFftAuVol,1,"A ajouter");
					GraphDataDisconnect(gMonitFftAuVol,2);
				} else {
					if(MonitFftNbSpectres < 1.5) {
						GraphDataReplace(gMonitFftAuVol,1,GRF_FLOAT|GRF_YAXIS,MonitFftMoyenne,MonitFftNbFreq);
						GraphDataRGB(gMonitFftAuVol,1,GRF_RGB_GREEN);
						GraphReset(gMonitFftAuVol);
					}
					GraphDataReplace(gMonitFftAuVol,2,GRF_FLOAT|GRF_YAXIS,MonitFftNouveau,MonitFftNbFreq);
				}
				OpiumDisplay(gMonitFftAuVol->cdr);
		/*		if(OpiumChoix(2,SambaNonOui,"Incorporation du jaune")) */
		//		OpiumPrioritaire = mLectSpectre->cdr;
				action = OpiumExec(mLectSpectre->cdr); /* 1: non / 2: oui */
		//		OpiumPrioritaire = 0;
			} else {  /* (MonitFftMode == CALC_SPTV_SURV) */
				if(MonitFftNbSpectres < 1.5)
					GraphDataReplace(gMonitFftAuVol,2,GRF_FLOAT|GRF_YAXIS,MonitFftNouveau,MonitFftNbFreq);
			}
		}
		if(action == 2) {
			if(MonitFftMode == CALC_SPTV_GLIS) {
				MonitFftNbSpectres = 1.0;
				MonitFftCarreSomme[0] = (MonitFftCarreSomme[0] * (1.0 - MonitFftEpsilon))
					 + (MonitFftEpsilon * MonitFftBase.freq[0] * MonitFftBase.freq[0] * MonitFftNorme);
				for(i=1; i<(MonitFftDim + 1) / 2; i++) {
					MonitFftCarreSomme[i] = (MonitFftCarreSomme[i] * (1.0 - MonitFftEpsilon))
						+ (MonitFftEpsilon * ((MonitFftBase.freq[i] * MonitFftBase.freq[i]) + (MonitFftBase.freq[MonitFftDim - i] * MonitFftBase.freq[MonitFftDim - i])) * MonitFftNorme);
				}
				if(!(MonitFftDim % 2)) MonitFftCarreSomme[MonitFftDim / 2] = (MonitFftCarreSomme[MonitFftDim / 2] * (1.0 - MonitFftEpsilon))
					+ (MonitFftEpsilon * MonitFftBase.freq[MonitFftDim / 2] * MonitFftBase.freq[MonitFftDim / 2] * MonitFftNorme);
				sprintf(nom,"Moyenne/%d spectres",MonitFftNb);
			} else /* if(MonitFftMode == CALC_SPTV_AUTO, _MANU et _SURV) */ {
				MonitFftNbSpectres += 1.0;
				MonitFftCarreSomme[0] = MonitFftCarreSomme[0] + (MonitFftBase.freq[0] * MonitFftBase.freq[0] * MonitFftNorme);
				for(i=1; i<(MonitFftDim + 1) / 2; i++) {
					MonitFftCarreSomme[i] = MonitFftCarreSomme[i]
						+ (((MonitFftBase.freq[i] * MonitFftBase.freq[i]) + (MonitFftBase.freq[MonitFftDim - i] * MonitFftBase.freq[MonitFftDim - i])) * MonitFftNorme);
				}
				if(!(MonitFftDim % 2)) MonitFftCarreSomme[MonitFftDim / 2] = MonitFftCarreSomme[MonitFftDim / 2]
					+ (MonitFftBase.freq[MonitFftDim / 2] * MonitFftBase.freq[MonitFftDim / 2] * MonitFftNorme);
				sprintf(nom,"Moyenne/%g spectres",MonitFftNbSpectres);
			}
/*			{
				printf("FFT moyenne @%08X -> %08X sur %g:",MonitFftCarreMoyenne,MonitFftCarreSomme,MonitFftNbSpectres);
				for(i=0; i<MonitFftDim; i++) {
					if(!(i % 10)) printf("\n%4d|",i);
					printf(" %10.0f",MonitFftCarreMoyenne[i]);
				}
				printf("\n");
			}*/
			unite = VoieManip[MonitFftVoie].nV_reels; if(unite < 0.0) unite = -unite; // classiquement ADUennV au lieu de nV_reels
			for(i=0; i<MonitFftNbFreq; i++) {
				MonitFftMoyenne[i] = unite * sqrtf(MonitFftCarreSomme[i] / MonitFftNbSpectres);
			}
			GraphDataName(gMonitFftAuVol,1,nom);
		} // else if(action == 3) MonitFftActif = 0;
		if(MonitFftMode != CALC_SPTV_SURV) GraphDataDisconnect(gMonitFftAuVol,2);
		OpiumDisplay(gMonitFftAuVol->cdr);
	}
suite1:
#ifdef VISU_FONCTION_TRANSFERT
	TrmtHaffiche = 1;
#endif
	if(LectMaintenance || LectModeSpectresAuto) goto conclusion;
/*
==== Niveau et bruit RMS des lignes de base
*/
	if(bLectBases) {
		if(MonitAffBases) {
			float sigma2;
			for(voie=0; voie<VoiesNb; voie++) {
				i = VoieManip[voie].type;
				j = VoieManip[voie].det;
				if(VoieTampon[voie].signal.nb) {
					LectBaseNiveau[i][j] = VoieTampon[voie].signal.somme / (float)VoieTampon[voie].signal.nb;
					sigma2 = VoieTampon[voie].signal.carre / (float)VoieTampon[voie].signal.nb;
					sigma2 -= (LectBaseNiveau[i][j] * LectBaseNiveau[i][j]);
					LectBaseBruit[i][j] = sqrtf(sigma2);
				} else LectBaseNiveau[i][j] = LectBaseBruit[i][j] = 0;
				if(j == BoloNum) {
					LectBase1Niveau[i] = LectBaseNiveau[i][j];
					LectBase1Bruit[i] = LectBaseBruit[i][j];
				}
			}
			if(!OpiumDisplayed(bLectBases)) OpiumDisplay(bLectBases);
			else {
				if(pLectBaseValeurs) PanelRefreshVars(pLectBaseValeurs);
				if(gLectBaseNiveaux) OpiumRefresh(gLectBaseNiveaux->cdr);
				if(gLectBaseBruits) OpiumRefresh(gLectBaseBruits->cdr);
			}
		} else if(OpiumDisplayed(bLectBases)) OpiumClear(bLectBases);
	}
/*
==== Graphiques et panels particuliers geres par Affichages > Ecran
*/
	if(PicsNb && PicsActif) {
		int nbevts;
		/* suivi de calibration */
		histo = hdEvtAmpl->histo;
		if(!HistoGetBinFloat(histo,&hmin,&hbin,&nbins)) {
			printf("%s/ Histogramme des amplitudes de la voie '%s' absent. Suivi du gain abandonne\n",DateHeure(),VoieManip[SettingsHistoVoie].nom);
			PicsNb = 0; PicsActif = 0; TrmtHampl = 0;
			goto fin_pics;
		}
		nbevts = 0;
		nbins = histo->x.nb;
		for(k=0; k<PicsNb; k++) {
			j = 0; adrs = HistoGetDataInt(hdEvtAmpl);
			for(i=0; i<nbins; i++, adrs++) {
				PicX[j] = hmin + ((float)i * hbin);
				if(PicX[j] < Pic[k].min) continue;
				if(PicX[j] > Pic[k].max) break;
				PicY[j] = (float)(*adrs);
				PicV[j] = 1;
				nbevts += *adrs;
				j++;
			}
			// printf("%s/ Pic#%d: %d evt%s entre %g et %g\n",DateHeure(),k+1,Accord1s(nbevts),hmin,hmin+(hbin*(float)nbins));
			if(nbevts < 3) continue;
			fond = 0.0;
			if(FitGaussienne(PicX,PicY,PicV,j,fond,&amp,&sigma,&x0)) {
				Pic[k].min = x0 - (2 * sigma);
				Pic[k].max = x0 + (2 * sigma);
				triggee = 0;
				n = EvtASauver;
				while(n-- > 0) {
					/* prendre plutot le dernier evt ou c'est la voie regardee qui a trigge */
					vt = Evt[n].voie_evt; triggee = &(Evt[n].voie[vt]); voie = triggee->num;
					if(voie == SettingsHistoVoie) break;
				};
				if((voie == SettingsHistoVoie) && triggee) {
					evtmax = (triggee->debut + VoieEvent[voie].avant_evt) * VoieTampon[voie].trmt[TRMT_AU_VOL].compactage;
					SambaTempsEchantillon(evtmax,0,0,&secs,&usecs);
					tevt = (float)(Pic[k].nb); // (float)secs;
					n = Pic[k].nb;
					if(n < Pic[k].dim) {
						Pic[k].t[n] = tevt;
						Pic[k].position[n] = x0;
						Pic[k].sigma[n] = sigma;
						Pic[k].nb += 1;
						// printf("* %d) ajoute ampl=%g/%g au temps %g/%d\n",n+1,Pic[k].position[n],x0,Pic[k].t[n],secs);
						GraphDataUse(gPicsPositions,2*k,Pic[k].nb);
						GraphDataUse(gPicsPositions,2*k+1,Pic[k].nb);
						GraphDataUse(gPicsSigmas,2*k,Pic[k].nb);
						GraphDataUse(gPicsSigmas,2*k+1,Pic[k].nb);
					} else {
						n = Pic[k].prems;
						Pic[k].t[n] = tevt;
						Pic[k].position[n] = x0;
						Pic[k].sigma[n] = sigma;
						Pic[k].prems++;
						if(Pic[k].prems >= Pic[k].dim) Pic[k].prems = 0;
						GraphDataRescroll(gPicsPositions,2*k,Pic[k].prems);
						GraphDataRescroll(gPicsPositions,2*k+1,Pic[k].prems);
						GraphDataRescroll(gPicsSigmas,2*k,Pic[k].prems);
						GraphDataRescroll(gPicsSigmas,2*k+1,Pic[k].prems);
					}
					GraphAxisReset(gPicsPositions,GRF_XAXIS);
					GraphAxisReset(gPicsPositions,GRF_YAXIS);
					GraphAxisReset(gPicsSigmas,GRF_XAXIS);
					GraphAxisReset(gPicsSigmas,GRF_YAXIS);
				}
			} // else printf("%s/ Fit des amplitudes de la voie '%s' impossible\n",DateHeure(),VoieManip[SettingsHistoVoie].nom);
		}
fin_pics:
		OpiumDisplay(gPicsPositions->cdr);
		OpiumDisplay(gPicsSigmas->cdr);
	}

	if(MonitHampl) {
		if(!OpiumDisplayed(gBruitAmpl->cdr)) OpiumDisplay(gBruitAmpl->cdr);
		else { GraphAxisReset(gBruitAmpl,GRF_YAXIS); OpiumRefresh(gBruitAmpl->cdr); }
	}
	if(MonitHmontee) {
		if(!OpiumDisplayed(gBruitMontee->cdr)) OpiumDisplay(gBruitMontee->cdr);
		else { GraphAxisReset(gBruitMontee,GRF_YAXIS); OpiumRefresh(gBruitMontee->cdr); }
	}
	if(MonitH2D) {
		if(MonitH2DY[0] == MonitH2DY[1]) {
			int min,max;
			H2DGetExtremaInt(h2D,&min,&max);
			GraphDataIntGamma(g2D,2,min,max,0,OPIUM_LUT_DIM - 1);
		}
		if(!OpiumDisplayed(g2D->cdr)) OpiumDisplay(g2D->cdr);
		else OpiumRefresh(g2D->cdr);
	}
	if(MonitAffpTaux && pTauxDetaille) {
		if(OpiumDisplayed(pTauxDetaille->cdr)) PanelRefreshVars(pTauxDetaille);
		else OpiumDisplay(pTauxDetaille->cdr);
	}
	if(taux_voie_change /* && MonitAffgTaux && gTauxDetaille */) {
		GraphUse(gTauxDetaille,LectTauxNb);
		if((LectTauxNb == LectTauxMax) && (LectTauxMax > 2000)) {
			GraphZoom(gTauxDetaille,LectTauxMax/1000,1);
			GraphRescroll(gTauxDetaille,LectTauxPrem);
		}
		if(OpiumDisplayed(gTauxDetaille->cdr)) {
			GraphAxisFloatMin(gTauxDetaille,GRF_YAXIS,0.0);
			OpiumRefresh(gTauxDetaille->cdr);
		} else OpiumDisplay(gTauxDetaille->cdr);
	}
	if(MonitAffgSeuils && gSeuils) {
		GraphUse(gSeuils,LectSeuilsNb);
		GraphRescroll(gSeuils,LectSeuilsPrem);
		if(OpiumDisplayed(gSeuils->cdr)) OpiumRefresh(gSeuils->cdr);
		else OpiumDisplay(gSeuils->cdr);
	}
	if(MonitAffModes && pSettingsModes) {
		if(OpiumDisplayed(pSettingsModes->cdr)) PanelRefreshVars(pSettingsModes);
		else OpiumDisplay(pSettingsModes->cdr);
	}
	if(MonitAffTrig && pSettingsTrigger) {
		if(OpiumDisplayed(pSettingsTrigger->cdr)) PanelRefreshVars(pSettingsTrigger);
		else if(PanelItemNb(pSettingsTrigger)) OpiumDisplay(pSettingsTrigger->cdr);
	}
	if(MonitAffEtat && pLectEtat) {
		if(OpiumDisplayed(pLectEtat->cdr)) PanelRefreshVars(pLectEtat);
		else OpiumDisplay(pLectEtat->cdr);
	}

//	printf("%11d octets apres LectDisplay\n",FreeMem());
conclusion:
	LectDansDisplay = 0;
	LectDepileInhibe = 0;
	return;
}
#ifdef JAMAIS_UTILISE
/* ========================================================================== */
static int LectSauveSpectre() {
	int i,j,k,total; int voie,bolo,cap,type_bn;
	int trmt,lngr,taille,maxi;
	TypeDonnee *tampon_origine,*ptr_data,*tampon_sauve,*ptr_sauve;
	TypeMonitFenetre *f; TypeMonitTrace *t; Graph g;
	Panel p[TRMT_NBCLASSES]; int nb[TRMT_NBCLASSES]; 
	char sauve[VOIES_MAX][TRMT_NBCLASSES];
	int dim[VOIES_MAX][TRMT_NBCLASSES];
	TypeMonitFenetre *fen[VOIES_MAX][TRMT_NBCLASSES];
	int numtrace[VOIES_MAX][TRMT_NBCLASSES];

	for(trmt=0; trmt<TRMT_NBCLASSES; trmt++) {
		p[trmt] = PanelCreate(2*VOIES_MAX); PanelColumns(p[trmt],2); PanelMode(p[trmt],PNL_BYLINES);
		nb[trmt] = 0;
		for(voie=0; voie<VoiesNb; voie++) sauve[voie][trmt] = 0;
	}
	total = 0;
	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if((f->affiche != MONIT_HIDE) && (f->type == MONIT_FFT) && (g = f->g)) {
		for(j=0, t=f->trace; j<f->nb; j++,t++) {
			voie = t->voie;
			trmt = (int)t->var;
			if(sauve[voie][trmt]) {
				OpiumError("Plusieurs traces pour la voie %s %s",VoieManip[voie].nom,TrmtClassesListe[trmt]);
			} else {
				numtrace[voie][trmt] = j;
				fen[voie][trmt] = f;
				sauve[voie][trmt] = 1;
				dim[voie][trmt] = f->axeX.pts;
				PanelBool(p[trmt],VoieManip[voie].nom,&(sauve[voie][trmt]));
				PanelInt(p[trmt]," + echantillons sauves",&(dim[voie][trmt]));
				nb[trmt] += 1;
				total++;
			}
		}
	}
	if (total <= 0) {
		OpiumError("Il n'y a pas de spectre en memoire");
		goto hev;
	}
	maxi = 0; tampon_sauve = 0;
	for(trmt=0; trmt<TRMT_NBCLASSES; trmt++) if(nb[trmt] > 0) {
		if(nb[trmt] == 1) PanelItemSelect(p[trmt],1,0);
		if(OpiumExec(p[trmt]->cdr) == PNL_CANCEL) {
//			for(voie=0; voie<VoiesNb; voie++) sauve[voie][trmt] = 0;
//			nb[trmt] = 0;
			continue;
		}
		for(voie=0; voie<VoiesNb; voie++) if(sauve[voie][trmt]) {
			taille = dim[voie][trmt];
			if(taille) {
				if(trmt == TRMT_AU_VOL) {
					tampon_origine = VoieTampon[voie].brutes->t.sval; lngr = VoieTampon[voie].brutes->max;
				} else /* (trmt == TRMT_PRETRG) */ {
					tampon_origine = VoieTampon[voie].traitees->t.rval; lngr = VoieTampon[voie].traitees->max;
				}
				if(taille > lngr) taille = lngr;
				if(taille > maxi) {
					if(tampon_sauve) free(tampon_sauve);
					tampon_sauve = (TypeDonnee *)malloc(taille * sizeof(TypeDonnee));
					if(tampon_sauve) maxi = taille; else maxi = 0;
				}
				if(tampon_sauve) {
					if(trmt == TRMT_AU_VOL) k = Modulo(VoieTampon[voie].lus - taille,lngr);
					else /* (trmt == TRMT_PRETRG) */ k = VoieTampon[voie].traitees->prem;
					ptr_data = tampon_origine + k; ptr_sauve = tampon_sauve;
					for(i=0; i<taille; i++) {
						*ptr_sauve++ = *ptr_data++;
						if(++k >= lngr) { k = 0; ptr_data = tampon_origine; }
					}
				} else {
					OpiumError("Manque %d octets pour sauver les donnees %ss de %s\n",
						taille * sizeof(TypeDonnee),TrmtClassesListe[trmt],VoieManip[voie].nom);
					taille = 0;
				}
			} else tampon_sauve = 0;
			strcpy(EdwSpBruitInfo.nom,VoieManip[voie].nom);
			EdwSpBruitInfo.type = trmt;
//			EdwSpBruitInfo.au_vol = VoieManip[voie].def.trmt[TRMT_AU_VOL].type;
//			EdwSpBruitInfo.sur_tampon = VoieManip[voie].def.trmt[TRMT_PRETRG].type;
			ArgKeyGetText(TrmtTypeCles,VoieManip[voie].def.trmt[TRMT_AU_VOL].type,EdwSpBruitInfo.au_vol,MAXTRMTNOM);
			ArgKeyGetText(TrmtTypeCles,VoieManip[voie].def.trmt[TRMT_PRETRG].type,EdwSpBruitInfo.sur_tampon,MAXTRMTNOM);
			bolo = VoieManip[voie].det;
			for(cap=0; cap<Bolo[bolo].nbcapt; cap++) if(Bolo[bolo].captr[cap].voie == voie) break;
			if(cap >= Bolo[bolo].nbcapt) cap = 0;
			type_bn = Numeriseur[Bolo[bolo].captr[cap].bb.num].def.type;
			strcpy(EdwSpBruitInfo.type_bn,ModeleBN[type_bn].nom);
			EdwSpBruitInfo.serial = Bolo[bolo].captr[cap].bb.serial;
		#ifdef CODE_WARRIOR_VSN
			EdwSpBruitInfo.date = 0;
			EdwSpBruitInfo.heure = 0;
		#else
			EdwSpBruitInfo.date = CalcDateSpectre.tv_sec;
			EdwSpBruitInfo.heure = CalcDateSpectre.tv_usec;
		#endif
//			ArgKeyGetText(RunEnvir,SettingsRunEnvr,EdwSpBruitInfo.cond,MAXCONDNOM);
			strcpy(EdwSpBruitInfo.srce,RunCategName);
			EdwSpBruitInfo.tempe_bolos = ReglTbolo = *T_Bolo;
			EdwSpBruitInfo.tempe_chateau = 300.0;
			EdwSpBruitInfo.radon = 0.0;
			EdwSpBruitInfo.tubes = EtatTubes = *PulseTubes;
			EdwSpBruitInfo.lique = 1;
			EdwSpBruitInfo.ventil = 0;
			EdwSpBruitInfo.horloge = VoieEvent[voie].horloge;
			EdwSpBruitInfo.d2 = Bolo[bolo].d2;
			EdwSpBruitInfo.nbtemps = taille;
			EdwSpBruitInfo.temps = tampon_sauve;
			f = fen[voie][trmt]; j = numtrace[voie][trmt];
			EdwSpBruitInfo.nbfreq = (f->axeX.pts / 2) + 1;
			EdwSpBruitInfo.freq = f->p.f.spectre[j];
			EdwDbPrint(DbazPath,VoieManip[voie].det,VoieManip[voie].type,&EdwSpBruit);
			nb[trmt] -= 1;
			if(nb[trmt] <= 0) break;
		}
	}
				
	if(tampon_sauve) free(tampon_sauve);
hev:
	for(trmt=0; trmt<TRMT_NBCLASSES; trmt++) PanelDelete(p[trmt]);
	return(0);
}
#endif /* JAMAIS_UTILISE */

#pragma mark ------- Comptes-rendus --------

/* ========================================================================== */
static void LectJournalLeDebut() {
	/* Debut de l'impression de l'entete de log du run */
#ifdef MODULES_RESEAU
	int sat;
#endif
	
	LogBlocBegin();
	printf(" ____________________________________________________________________________________\n");
	printf("| Configuration | %-66s |\n",SambaIntitule);
	printf("|      lue dans | %-66s |\n",PrefPath);
	printf("|_______________|____________________________________________________________________|\n");
#ifdef MODULES_RESEAU
	for(sat=0; sat<AcquisNb; sat++) {
		int l;
		if(sat == 0) l = printf("| Reseau        | "); else l = printf("|               | ");
		l += printf("%6s: %-32s (%s)",Acquis[sat].code,Acquis[sat].adrs,AcquisTexte[(int)Acquis[sat].etat.status]);
		SambaCompleteLigne(86,l);
	}
	printf("|_______________|____________________________________________________________________|\n");
#endif
	printf("\n");
	
	DetecteursImprime(0);
//		SambaDumpVoies(0,1);
	LogBlocEnd();
}
/* ========================================================================== */
static void LectJournalTrmtTitre(char *titre, int nbdonnees) {
	int classe; int k;

	if(nbdonnees) {
		printf("| %-16s |   gains    ADU(nV)|",titre);
		for(classe=0; classe<TRMT_NBCLASSES; classe++) printf(" %-18s   |",TrmtClasseTitre[classe]);
		printf("\n");
	} else { k = printf("| %-16s | Pas de donnee",titre); SambaCompleteLigne(86,k); }

	return;
}
/* ========================================================================== */
static void LectJournalTrmtVoie(int voie) {
	int classe,parm,gain; char trmt;

	if(voie >= 0) printf("| %-16s |%5.0f x%-4.1f %7.1f|",
		VoieManip[voie].nom,VoieManip[voie].gain_fixe,VoieManip[voie].gain_variable,VoieManip[voie].ADUennV);
	else printf("| %-16s |                   |","(ignoree)");
	for(classe=0; classe<TRMT_NBCLASSES; classe++) {
		if(voie >= 0) {
			trmt = VoieTampon[voie].trmt[classe].utilise;
			parm = VoieManip[voie].def.trmt[classe].p1;
			gain = VoieManip[voie].def.trmt[classe].gain;
			switch(trmt) {
			  case NEANT:         printf("%-18s    |","neant"); break;
			  case TRMT_DEMODUL:  printf("%-14sx%2d +%-3d|",TrmtTypeListe[trmt],gain,parm); break;
			  case TRMT_FILTRAGE: printf("%-18sx%2d |",FiltreGeneral[parm].nom,gain); break;
		#ifdef TRMT_ADDITIONNELS
			  case TRMT_LISSAGE:  printf("%-14sx%2d /%-3d|",TrmtTypeListe[trmt],gain,parm); break;
			  case TRMT_MOYENNE:
			  case TRMT_MAXIMUM:  printf("%-14sx%2d /%-3d|",TrmtTypeListe[trmt],gain,parm); break;
			#ifdef TRMT_DECONVOLUE
			  case TRMT_DECONV:   printf("%-14s:%5.7f|",TrmtTypeListe[trmt],gain,VoieManip[voie].def.trmt[classe].rc); break;
			#endif
			#ifdef A_LA_GG
			  case TRMT_INTEDIF:  printf("%-14sx%d/%d,%-4.2f|",TrmtTypeListe[trmt],gain,parm,VoieManip[voie].def.trmt[classe].p3); break;
			#endif /* A_LA_GG */
			#ifdef A_LA_MARINIERE
			  case TRMT_CSNSM:    printf("%-18sx%2d |",TrmtCsnsm[parm].nom,gain); break;
			#endif /* A_LA_MARINIERE */
		#endif /* TRMT_ADDITIONNELS */
			  case TRMT_INVALID:  printf("%-18s    |","invalidee"); break;
			  default:            printf("%-14sx%2d /%-3d|",gain,TrmtTypeListe[trmt],parm); break;
			}
		} else printf("                      |");
	}
	printf("\n");
//1	int l;
//1	l = printf("|                  | ");
//1	l += printf("%7.1f ADU/keV, %6d points/evt, %3g kHz %7",
//1				VoieManip[voie].ADUenkeV,VoieEvent[voie].lngr,
//1				1.0/VoieEvent[voie].horloge,VoieManip[voie].modulee? "modulee": " ");
//1	SambaCompleteLigne(86,l);

//2	printf("|                  |        en keV: %7.2f|",VoieManip[voie].ADUenkeV);
//2	for(classe=0; classe<TRMT_NBCLASSES; classe++) printf("                    |");
//2	printf("\n");
	
	return;
}
/* ========================================================================== */
static void LectJournalTrmtSepare(char trait, char avec_colonnes) {
	int k,classe;
	
	// printf("|__________________|___________________");
	printf("|"); k = 18; while(k--) printf("%c",trait); fflush(stdout);
	printf("|"); k = 19; while(k--) printf("%c",trait); fflush(stdout);
	for(classe=0; classe<TRMT_NBCLASSES; classe++) {
		// printf("%c______________________",avec_colonnes? '|': '_');
		printf("%c",avec_colonnes? '|': trait); k = 22; while(k--) printf("%c",trait);  fflush(stdout);
	}
	printf("|\n");
	return;
}
/* ========================================================================== */
int LectJournalTrmt() {
	int voie,vt,bolo; int l,n,nb_copiees,rep,vl;
	char titre[80];

	if(!VoiesLocalesNb) return(0);
	LogBlocBegin();
	printf(" ____________________________________________________________________________________\n");
	printf("|                                     Traitements                                    |\n");
	printf("|____________________________________________________________________________________|\n");

	vt = 0; n = 0;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) {
		if(Repart[rep].interf == INTERF_PCI) sprintf(titre,"%s @PCI%d",ModeleRep[Repart[rep].type].code,Repart[rep].adrs.val);
		else sprintf(titre,"%s @IP %d.%d",ModeleRep[Repart[rep].type].code,Repart[rep].adrs.champ[2],Repart[rep].adrs.champ[3]);
		LectJournalTrmtTitre(titre,Repart[rep].nbdonnees);
		if(Repart[rep].nbdonnees) {
			LectJournalTrmtSepare('.',1);
			for(vl=0; vl<Repart[rep].nbdonnees; vl++) LectJournalTrmtVoie(SambaEchantillon[Repart[rep].premiere_donnee + vl].voie);
		} // else { l = printf("| Pas de donnee    |"); SambaCompleteLigne(86,l); }
	#ifdef MENTIONNE_STATUS_BB_DANS_DATA
		if(LectStatusPresent[rep]) { l = printf("| + status         |"); n++; }
		else l = printf("| (sans status)    |");
		SambaCompleteLigne(86,l);
	#endif
		LectJournalTrmtSepare('_',1);
		vt += Repart[rep].nbdonnees;
	}
	l = printf("| Lu               | %d donnee%s",Accord1s(SambaEchantillonLngr)); SambaCompleteLigne(86,l);
	l = printf("| Transmis         | %d voie%s",Accord1s(vt)); if(n) l += printf(" + %d status",n); SambaCompleteLigne(86,l);
	LectJournalTrmtSepare('_',0);

	nb_copiees = 0;
	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue && VoieManip[voie].pseudo) {
//		TypeComposantePseudo *composante;
		if(!nb_copiees) {
			LectJournalTrmtTitre("Pseudo voies",1);
			LectJournalTrmtSepare('_',1);
		}
		LectJournalTrmtVoie(voie);
//		l = printf("|                  | = ");
//		composante = VoieManip[voie].pseudo;
//		while(composante) {
//			if((composante->coef > 0) && (composante != VoieManip[voie].pseudo)) l += printf("+");
//			l += printf("%g x '%s' ",composante->coef,VoieManip[composante->srce].nom);
//			composante = composante->svt;
//		}
		l = printf("|                  | = %s",DetecteurCompositionEncode(VoieManip[voie].pseudo));
		SambaCompleteLigne(86,l);
		LectJournalTrmtSepare('.',1);
		nb_copiees++;
	}
	if(nb_copiees) LectJournalTrmtSepare('_',1);

	l = printf("| Soustraction du pattern %s",SettingsSoustra? 
		((SettingsSoustra == TRMT_PTN_AVANT)? "avant filtrage": "apres filtrage"): "OMISE");
	SambaCompleteLigne(86,l);
	if(SettingsSoustra) {
		l = printf("| %s pendant les evenements",SettingsSauteEvt? "sauf": "y compris");
		SambaCompleteLigne(86,l);
		for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
			if(!VoieTampon[voie].pattern.val) {
				l = printf("| SAUF pour la voie %s",VoieManip[voie].nom);
				bolo = VoieManip[voie].det;
				if(VoieManip[voie].modulee) l += printf(" (car elle est modulee)");
				else if(VoieTampon[voie].pattern.dim) l += printf(" (par manque de memoire)");
				else if(VoieManip[voie].duree.periodes > 0.0)
					l += printf(" (raison peu claire: periode=%g, modulation=%d)",VoieManip[voie].duree.periodes,Bolo[VoieManip[voie].det].d2);
				else l += printf(" (nombre de periodes de calcul = 0)");
				SambaCompleteLigne(86,l);
			}
		}
	}
	printf("|____________________________________________________________________________________|\n");
	printf("\n");

	/*	{
		TypeFiltre *filtre;
		int etg,nbdir,nbinv,i;

		for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
			if(filtre = VoieTampon[voie].trmt[TRMT_PRETRG].filtre) {
				printf("%s: %d etage%s\n",VoieManip[voie].nom,Accord1s(filtre->nbetg));
				for(etg=0; etg<filtre->nbetg; etg++) {
					nbdir = filtre->etage[etg].nbdir;
					printf("%2d/  ",etg+1);
					for(i=0; i<nbdir; i++) printf(" %15g",filtre->etage[etg].direct[i]);
					printf("\n");
					nbinv = filtre->etage[etg].nbinv;
					printf("    +");
					for(i=0; i<nbinv; i++) printf(" %15g",filtre->etage[etg].inverse[i]);
					printf("\n");
				}
			}
		}
	} */
	
	LogBlocEnd();
	return(0);
}
/* ========================================================================== */
static void LectJournalLaSuite(char log_tout) {
	int k,n,rep; char type_sync[16]; // int vt,voie;

	if(log_tout) {
		LogBlocBegin();
		printf("Reglages fins:\n");
		printf("  Echantillonnage          : %g us (%g kHz)\n",1000.0/Echantillonnage,Echantillonnage);
		printf("  Synchro D2               : %d echantillon%s (une par %g ms, soit %g kHz)\n",Accord1s(Diviseur2),(float)SynchroD2_us/1000.0,SynchroD2_kHz);
		printf("  Lecture                  : "); if(SettingsMultitasks) printf("toutes les %d ms, ",SettingsReadoutPeriod); printf("%s\n",LectModeTexte[SettingsMultitasks]);
		printf("  Attente de rebouclage    : %d us\n",LectDepileWait);
		printf("  Duree donnees brutes     : %2g secondes         (%d echantillons)\n",DureeTampons / 1000.0,LngrTampons);
		printf("  Traitement               : toutes les %5d   ms (%5d synchros D2)\n",SettingsIntervTrmt,(int)LectIntervTrmt);
		n = 0;
		for(k=0; k<ExportPackNb; k++) if(!ExportPack[k].quand) n++;
		if(n) {
			printf("  Exportation%s periodique%s :\n",(n>1)?"s":" ",(n>1)?"s":" ");
			for(k=0; k<ExportPackNb; k++) if(!ExportPack[k].quand) {
				printf("  | %-22s : toutes les %5d   ms (%5d synchros D2)\n",
					ExportPack[k].nom,ExportPack[k].periode,(int)LectIntervExport[k]);
			}
		}
		printf("  Sequencement             : toutes les %8.2f s (%5d synchros D2)\n",(float)SettingsIntervSeq/1000.0,(int)LectIntervSeq);
		printf("  Action utilisateur       : toutes les %8.2f s (%5d synchros D2)\n",(float)SettingsIntervUser/1000.0,(int)LectIntervUser);
		if(LectCntl.LectMode == LECT_DONNEES)
			printf("  Affichage donnees        : toutes les %8.2f s (%5d synchros D2)\n",MonitIntervAff/1000.0,(int)LectIntervData);
		else if(LectCntl.LectMode == LECT_COMP)
			printf("  Signal evalue          sur %8.2f s (%5d synchros D2)\n",MonitIntervAff/1000.0,(int)LectIntervData);
		printf("  Profondeur n-tuples      : %d evenements\n",SettingsNtupleMax);
		for(rep=0; rep<RepartNb; rep++) if((Repart[rep].interf == INTERF_IP) && Repart[rep].actif) break;
		printf("  Relance UDP              : toutes les %5d s (%s)\n",LectIntervRelance,(rep < RepartNb)? "activee":"inactivee");
		printf("  Trames UDP               : %s\n",SettingsTramesMaxi?"echantillons partiels pour une taille maximum": "taille dynamique pour des echantillons entiers");
		if(RepartNb > 1) {
			printf("  Utilisation des repartiteurs:\n");
			for(rep=0; rep<RepartNb; rep++) {
				if(Repart[rep].actif) {
					printf("  . Repartiteur %s pris en compte",Repart[rep].nom);
				#ifdef MENTIONNE_STATUS_BB_DANS_DATA
					printf(", status");
					if(LectStatusPresent[rep]) {
						if(SettingsStatusCheck) printf(" avec masque = %04X",LectMasqueStatus[rep]); else printf(" pas verifie");
					} else {
						if(Voie8bits >= VOIES_TYPES) printf(": neant"); else printf(" porte par la voie %16s",ModeleVoie[Voie8bits].nom);
					}
				#endif
					printf("\n");
					RepartImprimeCompteurs(&(Repart[rep]),"            | ",0);
				} //+ else printf("  . Repartiteur %s inutilise\n",Repart[rep].nom);
			}
		} else RepartImprimeCompteurs(&(Repart[0]),"            | ",0);
		printf("  Demarrage detecteurs         : %s\n",LectDemarreBolo? "active": "desactive"); // indique "Maintenance" en date du 04.03.16
		if(LectEntretien) printf("  Maintenance detecteurs       : toutes les %d s\n",SettingsDLUdetec);
		else printf("  Maintenance detecteurs       : desactivee\n");
		if(ExportPackNb > n) {
			int m; m = ExportPackNb - n;
			printf("  Exportation%s synchrone%s      :\n",(m>1)?"s":" ",(m>1)?"s":" ");
			for(k=0; k<ExportPackNb; k++) if(ExportPack[k].quand) {
				char *c;
				ArgKeyGetText(EXPORT_QUAND_CLES,ExportPack[k].quand,type_sync,16);
				c = type_sync; ItemSuivant(&c,'_');
				printf("  | %-26s : %s de %s\n",ExportPack[k].nom,type_sync,c);
			}
		}
		if(!ExportPackNb) printf("  Exportations                 : neant\n");
		ArgKeyGetText(LECT_SYNCHRO_CLES,LectSynchroType,type_sync,16);
		printf("  Synchronisation de demarrage : %s\n",type_sync);
		if(LectRunTime) printf("  Arret automatique apres      : %ld secondes\n",LectRunTime);
		else if(SettingsSynchroMax) printf("  Arret impose apres %d synchros D2 (soit %g s)\n",
			SettingsSynchroMax,(float)SettingsSynchroMax/SynchroD2_kHz/1000.0);
		else printf("  Pas d'arret impose initialement\n");
		printf("  %s des evenements\n",MonitAffEvts? "Affichage":"Pas d'affichage");
		/* printf("  Contenu de chaque echantillon:\n");
		 for(vt=0; vt<SambaEchantillonLngr; vt++) printf("  | %2d: %s\n",vt+1,((voie = SambaEchantillon[vt].voie) >= 0)? VoieManip[voie].nom:"rien"); */
		LogBlocEnd();
	}
#ifdef PAROLE
	SpeakString("\pAcquisition started");
#endif
}
/* ========================================================================== */
static void LectJournalDemarrage() {
	printf("* Prise en compte des status %s\n",LectModeStatus? "uniquement": "et des donnees");
	printf("* Trigger: actuellement %s\n",(Trigger.actif)? "actif": "suspendu");
	printf("\nDeroulement:\n-----------\n");
}
#if defined(ETAPES_EVTS) || defined(VERIF_NBEVTS_FINAL)
/* ========================================================================== */
static void LectJournalCompteEvts() {
	int n,m,k;
	
	k = EvtsEmpiles? EvtsEmpiles - 1: 0;
	printf("\n");
	printf("Traitement des evenements:\n");
	printf(". Evenements signales                   : %6d\n",EvtSignales);
	printf("  - Debordements de la pile             : %6d\n",PileUp);
	printf("  - Doublons retires de la pile         : %6d\n",EvtSupprimes);
	printf("  - Pertes pour retard du traitement    : %6d\n",TrmtEvtJetes);
	printf("  - Evenements encore dans la pile      : %6d\n",k);
	printf("  ------------------------------------- : ------\n");
	n = EvtSignales - (PileUp + EvtSupprimes + TrmtEvtJetes + k);
	printf("  = Evenements accessibles              : %6d\n",n);
	printf(". Datations a revoir                    : %6d\n",TrmtNbRelances);
	printf("\n");
	m = n + TrmtNbRelances;
	printf(". Demandes de traitement                : %6d (comptees: %d)\n",m,TrmtNbNouveaux);
	printf("  - Triggers prematures                 : %6d\n",TrmtNbPremat);
	printf("  - Datations revues                    : %6d\n",TrmtNbRelances);
	printf("  - Datations hors delai                : %6d\n",TrmtNbEchecs);
	printf("  - Evenement en cours                  : %6d\n",EvtsEmpiles? 1: 0);
	printf("  ------------------------------------- : ------\n");
	n = TrmtNbPremat + TrmtNbRelances + TrmtNbEchecs + (EvtsEmpiles? 1: 0);
	printf("  = Evenements traites                  : %6d\n",m - n);
	printf("  - Evenements coupes a posteriori      : %6d\n",EvtNbCoupes);
	printf("  - Evenements non conformes            : %6d\n",EvtNbIncoherents);
	printf("  ------------------------------------- : ------\n");
	printf("  = Evenements retenus                  : %6d\n",m - n - EvtNbCoupes - EvtNbIncoherents);
	printf("  - Sauvegardes %-23s : %6d\n",ArchSauve[EVTS]? "hors delai": "desactivees",ArchEvtsPerdus);
	printf("  = Evenements sauves                   : %6d\n",Acquis[AcquisLocale].etat.evt_ecrits);
	printf("\n");
}
#endif
/* ========================================================================== */
static void LectJournalPMD() {

#if defined(DEBUG_DEPILE) || defined(HISTO_TIMING_TRMT) || defined(HISTOS_DELAIS_EVT) || defined(ETAPES_EVTS) || defined(DEBUG_STATUS_BBV2) || defined(DEBUG_STATUS_MIG)
	printf("\nInformations internes:\n---------------------\n");
#endif

#ifdef HISTO_TIMING_TRMT
	{
		int k,total;
		printf("Repartition des temps d'attente du traitement (s):");
		printf("\n      "); for(k=0; k<10; k++) printf("      ,%d",k);
		printf("\n      "); for(k=0; k<10; k++) printf("________",k);
		total = 0;
		for(k=0; k<MAXHISTOTIMING; k++) {
			if(!(k%10)) printf("\n%3d | ",k/10);
			printf(" %7d",HistoTiming[k]);
			total += HistoTiming[k];
		}
		printf("\n");
		printf("    %d entree%s dans cette table\n\n",total,(total>1)?"s":"");
	}
#endif /* HISTO_TIMING_TRMT */
	if(Trigger.enservice) {
	#ifdef HISTOS_DELAIS_EVT
		int k,total;
		printf("Repartition des delais entre triggers (x 100 ech.):");
		printf("\n      "); for(k=0; k<10; k++) printf("      %d0",k);
		printf("\n      "); for(k=0; k<10; k++) printf("________",k);
		total = 0;
		for(k=0; k<MAXHISTOTRIG; k++) {
			if(!(k%10)) printf("\n%3d | ",k*10);
			printf(" %7d",HistoTrig[k]);
			total += HistoTrig[k];
		}
		printf("\n");
		printf("    %d entree%s dans cette table\n\n",total,(total>1)?"s":"");
	#endif
	#ifdef ETAPES_EVTS
		LectJournalCompteEvts()
	#endif
	#ifdef HISTOS_DELAIS_EVT
		printf("Repartition des delais entre evenements memorises (x 100 ech.):");
		printf("\n      "); for(k=0; k<10; k++) printf("      %d0",k);
		printf("\n      "); for(k=0; k<10; k++) printf("________",k);
		total = 0;
		for(k=0; k<MAXHISTOTRIG; k++) {
			if(!(k%10)) printf("\n%3d | ",k*10);
			printf(" %7d",HistoEvts[k]);
			total += HistoEvts[k];
		}
		printf("\n");
		printf("    %d entree%s dans cette table\n\n",total,(total>1)?"s":"");
	#endif
	}
	
#ifdef DEBUG_STATUS_BBV2
	if(LectDecodeStatus) {
		int i,j,k,bb;
		for(k=0; k<MAXTRAMES; k++) {
			printf("Trame lue #%d:",k);
			for(i=0; i<sizeof(TypeOperaTrame)/sizeof(shex); i++) {
				if(!((i - 18) % 96)) printf("\n[%d]",(i - 18)/96);
				if(!(i % 6)) printf("\n%4d:",i);
				if(!((i - 2) % 6) || !((i - 4) % 6)) printf(" ");
				printf(" %04X",BOtrame0[k][i]);
				if(!((i - 5) % 6)) {
					for(j=0; j<16; j++) {
						if(!(j % 4)) printf(" ");
						printf(" %s",(BOtrame0[k][i] & (1 << (15 - j)))? "1": "0");
					}
				}
			}
			printf("\n");
		#ifdef SOFT_AB
			printf("Trame AB #%d:",k);
			for(i=0; i<sizeof(TypeOperaTrame)/sizeof(shex); i++) {
				if(!((i - 18) % 96)) printf("\n[%d]",(i - 18)/96);
				if(!(i % 6)) printf("\n%4d:",i);
				if(!((i - 2) % 6) || !((i - 4) % 6)) printf(" ");
				printf(" %04X",ABtrame0[k][i]);
				if(!((i - 5) % 6)) {
					for(j=0; j<16; j++) {
						if(!(j % 4)) printf(" ");
						printf(" %s",(ABtrame0[k][i - 5] & (1 << (15 - j)))? "1": "0");
					}
				}
			}
			printf("\n");
		#endif /* SOFT_AB */
		}
	#ifdef SOFT_AB
		printf("Status decode par AB:");
		printf("\n     "); for(i=0; i<16; i++) printf("  %2d ",i);
		for(i=0; i<OPERA_STATUS_LNGR; i++) {
			if(!(i % 16)) printf("\n%4d:",i);
			printf(" %04X",etat_bbv2.stbbv2[i]);
		}
		printf("\n");
	#endif
		printf("Status decode par MiG:");
		printf("\n     "); for(i=0; i<16; i++) printf("  %2d ",i);
		for(i=0; i<IndexStatusDecode; i++) {
			if(!(i % 16)) printf("\n%4d:",i);
			printf(" %04X",StatusDecode[i]);
		}
		printf("\n");
	/* printf("Mots de donnees codant le status (avec le BB concerne):");
		printf("\n     "); for(i=0; i<16; i++) printf("  %2d    ",i);
		for(i=0; i<IndexStatusCode; i++) {
			if(!(i % 16)) printf("\n%4d:",i);
			printf(" %04X(%d)",StatusCode[i],BBlu[i]);
		}
		printf("\n");
		bb = 0;
		printf("Ressources numeriseur #%d:",bb);
		printf("\n     "); for(i=0; i<16; i++) printf("  %2d ",i);
		for(i=0; i<ModeleBN[Numeriseur[bb].def.type].nbress; i++) {
			if(!(i % 16)) printf("\n%4d:",i);
			printf(" %04X",Numeriseur[bb].hval[i]);
		}
		printf("\n"); */
	}
#endif /* DEBUG_STATUS_BBV2 */

#ifdef DEBUG_STATUS_MIG
	printf("Liste des %d premiers status de bloc:",LectStatusNb);
	for(i=0; i<LectStatusNb; i++) {
		if(!(i%10)) printf("\n%5d:",i);
		printf(" %08X",LectStatusList[i]);
	}
	printf("\n");
#endif

	if(VerifEtatFinal) {
		printf("\nEtat final (variables internes):\n-------------------------------\n");
		LectBuffStatus();
		printf("\n");
		if(SettingsMultitasks) {
	/*
	 *      PMD special config a un seul bolo
	 */
			if(1 /* rc == LECT_INVALID */) {
			#ifdef DEBUG_RAW
				{
					int dep,i,k,n,m;
					int avant=200,apres=200;
					int rep,blocksize;

				#define EXISTE_FIFO
				#ifdef PCI
					#ifndef CLUZEL_FIFO_INTERNE
						#undef EXISTE_FIFO
					#endif
				#endif
					
					rep = LectEnCours.rep;
				#ifdef EXISTE_FIFO
					if(rep >= 0) { blocksize = Repart[rep].nbdonnees; if(LectStatusPresent[rep]) blocksize++; }
					if(FifoPrecedente) {
						printf("Fifo precedente [%d:%d]:",0,FifoTopPrec);
						m = (FifoTopPrec > apres)? apres: FifoTopPrec;
						m = (((m - 1) / blocksize) + 1) * blocksize;
						n = (FifoTopPrec > avant)? ((FifoTopPrec - avant) / blocksize) * blocksize: 0;
						if(n < m) m = FifoTopPrec;
						for(i=0; i<m; i++) {
							if(!(i % blocksize)) printf("\n%5d:",i);
							if(!((i % blocksize) % Repart[rep].nbentrees)) printf(" ");
							k = FifoPrecedente[i];
							printf("%c%08X",((k&0xC000)==0xC000)?'/':(((k&0xC000)==0x4000)?'*':' '),k);
						}
						printf("\n");
						if(m < FifoTopPrec) {
							printf("       . . .");
							for(i=n; i<FifoTopPrec; i++) {
								if(!(i % blocksize)) printf("\n%5d:",i);
								if(!((i % blocksize) % Repart[rep].nbentrees)) printf(" ");
								k = FifoPrecedente[i];
								printf("%c%08X",((k&0xC000)==0xC000)?'/':(((k&0xC000)==0x4000)?'*':' '),k);
							}
							printf("\n");
						}
					} else printf("Fifo precedente: pas affichee, car pas allouee dans SambaLinkPCI\n");
					if(rep >= 0) {
						dep = Repart[rep].depot_ancien;
						if(Repart[rep].depot[dep].fond < Repart[rep].depot[dep].reserve) {
							printf("Reservoir #%d [%d:%d]:",dep,Repart[rep].depot[dep].fond,Repart[rep].depot[dep].reserve);
							n = (((Repart[rep].depot[dep].fond > avant)? Repart[rep].depot[dep].fond: 0) / blocksize) * blocksize;
							m = (Repart[rep].depot[dep].reserve < Repart[rep].depot[dep].fond+apres)? Repart[rep].depot[dep].reserve: Repart[rep].depot[dep].fond+apres;
							for(i=n; i<m; i++) {
								if(!(i % blocksize)) printf("\n%5d:",i);
								if(!((i % blocksize) % Repart[rep].nbentrees)) printf(" ");
								k = Repart[rep].depot[dep].reservoir[i];
								printf("%c%08X",(i == Repart[rep].depot[dep].fond)?'+':(((k&0xC000)==0xC000)?'/':(((k&0xC000)==0x4000)?'*':' ')),k);
							}
							printf("\n");
						} else {
							printf("Fifo interne [%d:%d]:",Repart[rep].bottom,Repart[rep].top);
							n = (((Repart[rep].bottom > avant)? Repart[rep].bottom-avant: 0) / blocksize) * blocksize;
							m = (Repart[rep].top < Repart[rep].bottom+apres)? Repart[rep].top: Repart[rep].bottom+apres;
							for(i=n; i<m; i++) {
								if(!(i % blocksize)) printf("\n%5d:",i);
								if(!((i % blocksize) % Repart[rep].nbentrees)) printf(" ");
								if(LectSurFichier) {
									shex h;
									h = Repart[RepartFile].data16[i];
									printf("%c%04X",(i == Repart[rep].bottom)?'+':(((h&0xC000)==0xC000)?'/':(((h&0xC000)==0x4000)?'*':' ')),h);
								} else {
									k = Repart[rep].fifo32[i];
									printf("%c%08X",(i == Repart[rep].bottom)?'+':(((k&0xC000)==0xC000)?'/':(((k&0xC000)==0x4000)?'*':' ')),k);
								}
							}
							printf("\n");
						}
					}
					printf("Valeurs depilees: %d la derniere fois, %d au maximum\n",LectNbDepilees,LectMaxDepilees);
				#ifdef DEBUG_STATUS_MIG
					printf("Liste des %d premiers status de bloc:",LectStatusNb);
					for(i=0; i<LectStatusNb; i++) {
						if(!(i%10)) printf("\n%5d:",i);
						printf(" %08X",LectStatusList[i]);
					}
					printf("\n");
				#endif
				#else /* EXISTE_FIFO */
					int nb;
					/* donnees deja lues */
					nb = 100; /* MAXRAWDATA */; if(nb > LectNonNul) nb = LectNonNul;
					i = Modulo((LectNonNul - (int64)nb),MAXRAWDATA);
					while(nb--) {
						printf(" %08X",LectRaw[i++]);
						if(!(nb % blocksize)) printf("\n");
						if(i >= MAXRAWDATA) i = 0;
					}
					/* donnees encore sur la pile */
					if(nb % blocksize) printf("\n");
					printf("+%08X",*(PCIadrs[Repart[rep].adrs.val-1])); // IcaFifoRead(Repart[rep].p.pci.port));
					i = blocksize - 1; while(i--) printf(" %08X",*(PCIadrs[Repart[rep].adrs.val-1]); /* IcaFifoRead(Repart[rep].p.pci.port) */); printf("\n");
					{
						int j; hexa val;
						j = 2; /* 100?... */
						while(j--) {
							i = blocksize; while(i--) {
								val = *(PCIadrs[Repart[rep].adrs.val-1]); // IcaFifoRead(Repart[rep].p.pci.port);
			//					if(val & 0xFFFF0000) printf(" %08X",val); else ++i;
								printf(" %08X",val);
							}
							printf("\n");
						}
					}
				#endif /* CLUZEL_FIFO_INTERNE -> en fait, EXISTE_FIFO */
				}
			#endif /* DEBUG_RAW */
			// #define RECONSTITUE
			#ifdef RECONSTITUE
				{ int pt,i,voie,vt,bolo,nb;

					nb = 10; /* < STATUSLIST_LNGR si possible */
				#ifdef DEBUG_DEPILE
					if(nb > STATUSLIST_LNGR) nb = STATUSLIST_LNGR;
				#endif
					if(nb > LectStampsLus) nb = LectStampsLus;
					pt = LectStampsLus - nb;
					i = pt % VoieTampon[0].brutes->max; 
					while(nb-- > 0) {
						printf("%9d:",pt);
						for(vt=0; vt<SambaEchantillonLngr; vt++) if((voie = SambaEchantillon[vt].voie) >= 0) {
							if(pt >= VoieTampon[voie].lus) break;
							printf(" %04X",*(VoieTampon[voie].brutes->t.sval + i));
						}
						printf("  ");
						if(vt >= SambaEchantillonLngr) {
				#ifdef DEBUG_DEPILE
					#ifdef DEBUG_STATUS_MIG
							if(LectStatusPresent[rep]) printf(" %04X",LectStatusList[pt % STATUSLIST_LNGR]);
					#endif
				#endif
							printf("\n");
						}
						i++; if(i > VoieTampon[0].brutes->max) i -= VoieTampon[0].brutes->max;
						pt++;
					}
					if(LectEnCours.dispo) printf("          +%04X\n",
				#ifdef DONNEES_NON_SIGNEES
						LectSurFichier? LectEnCours.valeur:
							VoieManip[LectVoieALire].signe? 
								((DATAVALEUR(LectVoieALire,LectEnCours.valeur) < VoieTampon[LectVoieALire].zero)?
									DATAVALEUR(voie,LectEnCours.valeur) + VoieTampon[LectVoieALire].zero
									: DATAVALEUR(LectVoieALire,LectEnCours.valeur) - VoieTampon[LectVoieALire].zero)
								: DATAVALEUR(LectVoieALire,LectEnCours.valeur)
				#else
					#ifdef SIGNE_ARITHMETIQUE
						LectSurFichier? LectEnCours.valeur:
							VoieManip[LectVoieALire].signe?
								((DATAVALEUR(LectVoieALire,LectEnCours.valeur) < VoieTampon[LectVoieALire].zero)?
									DATAVALEUR(LectVoieALire,LectEnCours.valeur): DATAVALEUR(LectVoieALire,LectEnCours.valeur) - (2 * VoieTampon[LectVoieALire].zero))
								: DATAVALEUR(LectVoieALire,LectEnCours.valeur)
					#else
						LectSurFichier? LectEnCours.valeur:
							VoieManip[LectVoieALire].signe?
								((DATAVALEUR(LectVoieALire,LectEnCours.valeur) & VoieTampon[LectVoieALire].zero)?
									(DATAVALEUR(LectVoieALire,LectEnCours.valeur) | VoieTampon[LectVoieALire].extens): DATAVALEUR(LectVoieALire,LectEnCours.valeur))
								: DATAVALEUR(LectVoieALire,LectEnCours.valeur)
					#endif /* SIGNE_ARITHMETIQUE */
				#endif /* DONNEES_NON_SIGNEES */
						);
					else if(vt < SambaEchantillonLngr) printf("\n");
				}
			#endif /* RECONSTITUE */
			#ifdef DEBUG_DEPILE
				if(LectDepileStockes >= MAXDEPILE) printf("LectDepile appele au moins %d fois\n",LectDepileStockes);
				else printf("LectDepile appele %d fois\n",LectDepileStockes);
			#ifdef DEBUG_FILL
				printf("LectFill appele %d%06d fois",
					(int)(LectFillNb/1000000),Modulo(LectFillNb,1000000));
				if(LectDepileStockes > 1) {
					printf(" (pour maxi %d%06d fois, sinon %d%06d fois)\n",
						(int)(LectFillMax/1000000),Modulo(LectFillMax,1000000),
						(int)(LectFillMax2/1000000),Modulo(LectFillMax2,1000000));
				} else printf("\n");
			#endif /* DEBUG_FILL */
			#define DEBUG_IMPR_SERIES
			#ifdef DEBUG_IMPR_SERIES
				{	int n=10, j, vides=0, pleines=0; int64 uses=0;
					n = LectDepileStockes; /* on veut tout, au 23.03.04 */
					j = LectDepileStockes - n; j = ((j / 10) * 10);
					printf("Valeurs depilees pour chacun des %d derniers appels:",LectDepileStockes - j);
					while(j < LectDepileStockes) {
						if(LectSerie[j]) pleines++; else vides++;
						uses += LectSerie[j];
						if(!(j % 10)) printf("\n%4d:",j);
						printf(" %2d%06d",(int)(LectSerie[j]/1000000),Modulo(LectSerie[j],1000000));
						j++;
					}
					printf("\nSoit %d appels, dont %d avec pile interne vide et %d avec des valeurs a recuperer\n",
						pleines+vides,vides,pleines);
					printf("  et un total de %d%06d valeurs utilisees\n",(int)(uses/1000000),Modulo(uses,1000000));
				}
			#else
				printf("LectFill[%d] = %d%06d\n",LectDepileStockes-1,
					(int)(LectSerie[LectDepileStockes-1]/1000000),Modulo(LectSerie[LectDepileStockes-1],1000000));
			#endif /* DEBUG_IMPR_SERIES */
			#endif /* DEBUG_DEPILE */
/* -------------- fin PMD 1 bolo -------------- */
			} /* rc */

		#ifdef DEBUG_ACCES
			printf("%lld acces PCI, dont",LectPciAdrs);
			#ifdef DEBUG_RAW
			printf(" %lld acces non nuls et",LectNonNul);
			#endif /* DEBUG_RAW */
			printf(" %lld acces nuls\n",RepartPci.vide);
		#endif /* DEBUG_ACCES */
			printf("Voie a lire: #%d du repartiteur #%d, temps en cours: %d\n",
				LectVoieRep,LectEnCours.rep,Diviseur2 - LectAvantSynchroD2);
			printf("Derniere valeur lue: %08X (%s); type attendu: %04X\n",
				LectEnCours.valeur,LectEnCours.dispo?"a memoriser":"perimee",LectTypePrevu);
		} /* fin particularites multitasks */
	}
}
/* ========================================================================== */
void LectCourbeDump(int plot) {
	TypeSambaCourbe *courbe; int i;
	courbe = &(SambaInfos->courbe[plot]);
	printf("(%s) Suivi des %s:\n",__func__,MonitCatgPtr[courbe->catg]->nom);
	printf("| nb=%d/%d, prem=%d",courbe->nb,SAMBA_SHR_MAXPTS,courbe->prem);
	for(i=0; i<courbe->nb; i++) {
		if(!(i % 10)) printf("\n| %4d :",i);
		printf(" %.3f",courbe->val[i]);
	}
	printf("\n");
}
/* ========================================================================== */
static void LectJournalCPU(char *tache, int64 duree, char *supp, int64 inclus, int64 total) {
	if(tache) {
		printf("  %-7s:  %7d,%06d s",tache,(int)(duree/1000000),Modulo(duree,1000000));
		if(total) printf("   (%4.1f%%)",((double)duree / (double)total) * 100.0);
	} else printf("                                      ");
	if(supp) {
		if(inclus >= 0) {
			printf("     %-17s: %7d,%06d s",supp,(int)(inclus/1000000),Modulo(inclus,1000000));
			if(total) printf("   (%4.1f%%)",((double)inclus / (double)total) * 100.0);
		} else printf("     %-17s",supp);
	}
	printf("\n");
}
/* ========================================================================== */
static void LectJournalCPUpartiel(char *tache, int64 duree, int64 total) {
	printf("                                      dont %-17s: %7d,%06d s",tache,(int)(duree/1000000),Modulo(duree,1000000));
	if(total) printf("   (%4.1f%%)",((double)duree / (double)total) * 100.0);
	printf("\n");
}
/* ========================================================================== */
static void LectJournalLaFin(TypeADU rc, char avec_physique, char avec_perfos) {
	int sec_reel,usec_reel,sec_simule,usec_simule; char ok,vu;
#ifdef MODULES_RESEAU
	int sat; float KgJ;
#endif
	int voie,vl,rep; int i,m,n; int64 t; float reel,cpu;
	int64 recues,err,ovr;
	int64 temps_reel,temps_simule,temps_libre,attente;
#ifdef VERIF_ETAT_REPART_FINAL
	int64 remplies,totales,valeurs;
#endif
	float lus_reellement;
	char *nom_run,*tache,supp[80];

	LogBlocBegin();
	LectJournalPMD();
	printf("\nBilan:\n-----");

	if(LectCntl.LectMode == LECT_COMP) {
		if(!LectEnCours.dispo) LectEnCours.valeur = LectEnCours.precedente;
		printf("Etat de la FIFO PCI a la derniere lecture:\n  ");
		printf("%s pleine, ",(LectEnCours.valeur & CLUZEL_NOT_FULLFIFO)?"pas":"");
		printf("%s demi-pleine, ",(LectEnCours.valeur & CLUZEL_NOT_HALFFIFO)?"pas":"");
		printf("%s vide, ",(LectEnCours.valeur & CLUZEL_NOT_EMPTY)?"pas":"");
		printf("%s ete pleine",(LectEnCours.valeur & CLUZEL_WASFULL)?"a":"n'a jamais");
		printf(" (valeur %s: %08X)\n",LectEnCours.dispo? "courante":"precedente",LectEnCours.valeur);
	} else {

		nom_run = Acquis[AcquisLocale].etat.nom_run; if(!strcmp(nom_run,"neant")) nom_run = L_("non sauvegarde");
		printf("\nAcces aux donnees:\n");
		printf("  Run %s:\n",nom_run);
		printf("  . %-10s le %s\n","demarre",RunDateDebut);
		printf("  . %-10s le %s a %s\n",LectRelancePrevue? "interrompu": "termine",DateCivile(),DateHeure());
		printf("  . duree : %s (en %d fois)\n",Acquis[AcquisLocale].etat.duree_lue,LectSession);
		if(LectStampSautes && VoiesNb) {
			LectStampSautes /= VoiesNb;
			printf("  . %lld echantillons ignores apres trigger  (%.3f s, temps mort volontaire: %.1f%%)\n",
			LectStampSautes,(float)LectStampSautes * EnSecondes,LectStampsLus? 100.0 * (double)LectStampSautes / (double)LectStampsLus: 100.0);
		}
		printf("  Detection d'erreurs:\n");
		printf("  . desynchronisations en lecture   : %lld\n",LectSyncRead);
		printf("  . desynchronisations en ecriture  : %lld\n",ArchSyncWrite);
		if(IPdemande) {
			printf("  . erreurs internes repartiteurs   : %d\n",RepartDataErreurs);
			printf("  . erreurs de transmission trames  : %d\n",RepartIpErreurs);
			printf("  . periodes de perturbation reseau : %d\n",LectEpisodeAgite);
		}
		/* if(SettingsAutoSync) {
			if(RepartIp.perdues) printf("  %9lld valeur%s perdue%s (derniere valeur valide reutilisee)\n",
				Accord2s(RepartIp.perdues));
			else printf("  Aucune valeur n'a ete perdue\n");
		} */
		for(rep=0; rep<RepartNb; rep++) if((Repart[rep].famille == FAMILLE_OPERA) && Repart[rep].s.ip.stampPerdus) {
			int64 theo; int nb_stamps;
			nb_stamps = (Repart[rep].nbdonnees >= VOIES_PAR_4OCTETS)? Repart[rep].s.ip.duree_trame: _nb_data_trame_udp;
			Repart[rep].s.ip.stamp0 += (int64)(Repart[rep].s.ip.num0 * nb_stamps);
			Repart[rep].s.ip.stamp1 += (int64)(Repart[rep].s.ip.num1 * nb_stamps);
			printf("  . %s: %lld valeurs echappees par voie",Repart[rep].nom,Repart[rep].s.ip.stampPerdus);
			if((theo = (Repart[rep].s.ip.stamp1 - Repart[rep].s.ip.stamp0))) 
				printf(" (%.1f%%)\n",(float)(Repart[rep].s.ip.stampPerdus * 10000 / theo) / 100.0);
			else printf("\n");
		}
		ok = 1;
		for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue > 0) {
			if(VoieTampon[voie].jetees) {
				printf("  . voie %s: %lld valeurs non traitees",VoieManip[voie].nom,VoieTampon[voie].jetees);
				if(VoieTampon[voie].lus) {
					printf(" (temps mort: %.1f%%)\n",100.0 * (double)VoieTampon[voie].jetees / (double)VoieTampon[voie].lus);
				} else printf("\n");
				ok = 0;
			}
		}
		if(LectStampPerdus) printf("  ! toutes les voies: %lld valeurs non recues  (%.3f s, TEMPS MORT ACCIDENTEL: %.1f%%)\n",
			LectStampPerdus,(float)LectStampPerdus * EnSecondes,LectStampsLus? 100.0 * (double)LectStampPerdus / (double)LectStampsLus: 100.0);
		if(LectStampJetes) printf("  ! toutes les voies: %lld valeurs non traitees (%.3f s, TEMPS MORT ACCIDENTEL: %.1f%%)\n",
			LectStampJetes,(float)LectStampJetes * EnSecondes,LectStampsLus? 100.0 * (double)LectStampJetes / (double)LectStampsLus: 100.0);
		else if(LectValZappees) printf("  ! toutes les voies: %lld valeurs non sauvees (%.3f s, TEMPS MORT ACCIDENTEL: %.1f%%)\n",
			LectValZappees,(float)LectValZappees * EnSecondes,LectStampsLus? 100.0 * (double)LectValZappees / (double)LectStampsLus: 100.0);
		else if(ok) printf("  . aucune valeur n'a ete perdue\n");
		if(Trigger.enservice) {
			n = PileUp + TrmtEvtJetes + TrmtNbEchecs + ArchEvtsPerdus;
			if(PileUp) printf("  ! %d evenement%s n'%s pas pu etre empile%s\n",
				Accord1s(PileUp),(PileUp>1)?"ont":"a",(PileUp>1)?"s":"");
			if(TrmtEvtJetes) printf("  ! %d evenement%s n'%s pas ete traite%s a temps\n",
				Accord1s(TrmtEvtJetes),(TrmtEvtJetes>1)?"ont":"a",(TrmtEvtJetes>1)?"s":"");
			if(TrmtNbEchecs) printf("  ! %d evenement%s n'%s pas ete date%s a temps\n",
				Accord1s(TrmtNbEchecs),(TrmtNbEchecs>1)?"ont":"a",(TrmtNbEchecs>1)?"s":"");
			if(ArchEvtsPerdus) printf("  ! %d evenement%s n'%s pas ete sauve%s a temps\n",
				Accord1s(ArchEvtsPerdus),(ArchEvtsPerdus>1)?"ont":"a",(ArchEvtsPerdus>1)?"s":"");
			if(n) printf("  > au total, %d evenement%s %s ete perdu%s\n",
				Accord1s(n),(n>1)?"ont":"a",(n>1)?"s":"");
			else printf("  . aucun evenement n'a ete perdu\n");
		}
		
		temps_reel = LectTexec;
		sec_reel = (int)(temps_reel / 1000000);
		usec_reel = (int)(temps_reel - (sec_reel * 1000000));
		
		if((LectCntl.LectMode == LECT_DONNEES) && avec_physique) {
			printf("\nPhysique:\n");
		#ifdef MODULES_RESEAU
			KgJ = 0.0;
			printf("  Exposition (Kg.jour):\n");
			for(sat=0; sat<AcquisNb; sat++) {
				printf("    %-24s: %12.3g\n",Acquis[sat].code,Acquis[sat].etat.KgJ);
				KgJ += Acquis[sat].etat.KgJ;
			}
			printf("    %-24s: %12.3g\n","Total",KgJ);
		#else
			printf("  Exposition : %.3g Kg.jour;\n",Acquis[AcquisLocale].etat.KgJ);
		#endif
			if(Trigger.enservice) {
				
				printf("  %d evenement%s coupe%s a posteriori\n",Accord2s(EvtNbCoupes));
				printf("  %d evenement%s non conforme%s (mesure/trigger)\n",Accord2s(EvtNbIncoherents));
				if(SrceType == SRC_EDW0)
					printf("  %d/%d evenements detectes\n",Acquis[AcquisLocale].etat.evt_trouves,LectArchLus);
				else printf("  %d evenement%s traite%s",Accord2s(Acquis[AcquisLocale].etat.evt_trouves));
				if(EvtSupprimes) printf(" [dont %d multiple%s]",Accord1s(EvtSupprimes));
				if(Acquis[AcquisLocale].etat.KgJ) printf(", soit  %g evts/Kg/j",(float)Acquis[AcquisLocale].etat.evt_trouves / Acquis[AcquisLocale].etat.KgJ);
				printf(";\n");
				printf("  %d evenement%s sauve%s",Accord2s(Acquis[AcquisLocale].etat.evt_ecrits));
				if(ArchEvtsVides) printf(", dont %d vide%s (delai trop court);\n",Accord1s(ArchEvtsVides)); else printf(";\n");
				if(LectStampsLus) printf("  Taux global: %.3f Hz\n",(float)(Acquis[AcquisLocale].etat.evt_trouves + EvtSupprimes) / ((float)(LectStampPrecedents + LectStampsLus) * EnSecondes));
				lus_reellement = LectStampPrecedents + LectStampsLus - TrmtPasActif;
				if(lus_reellement) printf("  Taux retenu: %.3f Hz\n",(float)Acquis[AcquisLocale].etat.evt_trouves / ((float)lus_reellement * EnSecondes));
			} else printf("  Pas d'evenement detecte, leur recherche n'etant pas activee\n");
		}
	
		printf("\nCompteurs:\n");
		if(LectSurFichier) {
			if(LectArchFile) printf("  Fichier positionne finalement a l'octet %04lx",ftell(LectArchFile));
			else if(LectArchPath >= 0) 
				printf("Fichier positionne finalement a l'octet %04lx",(lhex)lseek(LectArchPath,0,SEEK_CUR));
			else printf("Fichier DEJA FERME");
			if(SrceType == SRC_EDW0) printf(" (%d blocs lus)\n",LectArchLus);
			else printf("\n");
		}
		SambaTempsEchantillon(LectStampsLus,0,0,&sec_simule,&usec_simule);
		printf("  Demarrage du run au stamp      : %16lld\n",LectTimeStamp0);
		printf("  Echantillons lus               : %16lld",  LectStampsLus); if(LectStampPrecedents) printf(" [ajoutes]"); printf("\n");
		printf("  Derniere synchro D3 recue a    : %16lld\n",LectTimeStamp0+LectDerniereD3);
		printf("  Synchros D2 enregistrees       : %14lld\n",Accord2s(SynchroD2lues));
		printf("  %lld recalage%s sur D3\n",Accord1s(LectRecaleD3));
		if(LectStampPrecedents) {
			printf("  Temps lu dans cette session    : %d,%06d s\n",sec_simule,usec_simule);
			SambaTempsEchantillon((LectStampPrecedents + LectStampsLus - TrmtPasActif),0,0,&sec_simule,&usec_simule);
			printf("  Temps cumule                   : %d,%06d s\n",sec_simule,usec_simule);
		} else
			printf("  Temps lu                       : %d,%06d s\n",sec_simule,usec_simule);
		if(LectParBloc) {
			sec_reel = (int)(LectArchDuree / 1000000); usec_reel = (int)(LectArchDuree - (sec_reel * 1000000));
			printf("  Temps de relecture             : %d,%06d s\n",sec_reel,usec_reel);
		}
		printf("  Temps utilise                  : %d,%06d s",sec_reel,usec_reel);
		temps_simule = (int64)((float)LectStampsLus / (Echantillonnage / 1000.0));
		if(temps_simule) printf(" (glissement: %.3f%%)\n",((double)(temps_reel - temps_simule) / (double)temps_simule) * 100.0);
		else printf("\n");
		sec_reel = (int)(LectTexec / 1000000); usec_reel = (int)(LectTexec - (sec_reel * 1000000));
		printf("  Octets lus via IP              : %lld en %d,%06d s\n",RepartIpOctetsLus,sec_reel,usec_reel);
		/* SambaEchantillonLngr = ECHANTILLON TOTAL Y COMPRIS LES PSEUDOS!! */
		printf("  Taux de transfert attendu      : %.1f Mbits/s\n",(float)(SambaEchantillonReel * sizeof(TypeDonnee) * 8) * Echantillonnage / 1000.0);
		printf("  Taux de transfert recu         : %.1f Mbits/s\n",8.0 * ((float)RepartIpOctetsLus / (float)LectTexec));
		
		if(PCIdemande) {
			if(!LectEnCours.dispo) LectEnCours.valeur = LectEnCours.precedente;
			printf("  Etat de la FIFO PCI a la derniere lecture:\n  . ");
			printf("%spleine, ",(LectEnCours.valeur & CLUZEL_NOT_FULLFIFO)?"pas ":"");
			printf("%sdemi-pleine, ",(LectEnCours.valeur & CLUZEL_NOT_HALFFIFO)?"pas ":"");
			printf("%svide, ",(LectEnCours.valeur & CLUZEL_NOT_EMPTY)?"pas ":"");
			printf("%s ete pleine",(LectEnCours.valeur & CLUZEL_WASFULL)?"a":"n'a jamais");
			printf(" (valeur %s: %08X)\n",LectEnCours.dispo? "courante":"precedente",LectEnCours.valeur);
			/* pour verification:
			 printf("Pile %s demi-pleine\n",CLUZEL_HalfFull(LectEnCours.valeur)?"":"pas");
			 printf("Pile %s pleine\n",CLUZEL_FullFull(LectEnCours.valeur)?"":"pas"); */
			printf("  . %lld acces a la FIFO PCI alors qu'elle etait vide",RepartPci.vide);
			if(RepartPci.vide) {
				printf(" (1/%d donnees)\n",(int)(double)(LectStampsLus * (int64)LectBlockSize / RepartPci.vide));
				attente = TempsTotalDispo / RepartPci.vide;
				printf("  . Temps moyen entre lectures   : %lld us\n",attente);
			} else printf("\n");
			for(rep=0; rep<RepartNb; rep++) if((Repart[rep].famille == FAMILLE_CLUZEL) && Repart[rep].actif) {
				printf("  . %-10s: ",Repart[rep].nom);
				if(Repart[rep].depot_nb) {
					short dep;
					for(dep=0; dep<REPART_RESERVOIR_MAX; dep++) {
						if(dep) printf("                ");
						if(Repart[rep].depot[dep].reservoir) printf("reservoir #%d utilise, donnees dans [%d %d]",dep,Repart[rep].depot[dep].fond,Repart[rep].depot[dep].reserve);
						else printf("reservoir #%d inutilise",dep);
						if(dep == Repart[rep].depot_ancien) printf(", le plus ancien rempli");
						if(dep == Repart[rep].depot_recent) printf(", le prochain a remplir");
						printf("\n");
					}
				} else printf("depot inutilise\n");
			}
		}
		if(IPdemande) {
			printf("  Delai maximum entre lectures   : %.3f ms (nominal: %d)\n",(float)LectDepileIntervMax / 1000.0,SettingsReadoutPeriod);
			printf("  Intervalles ridicules          : %d\n",LectDelaisTropCourt);
			printf("  Intervalles excessifs          : %d\n",LectDelaisTropLong);
			printf("  Nombre de lectures             : %12lld\n",(int64)LectDepileNb);
			if(SambaPartage) printf("  Nombre de lancements           : %12d\n",(int64)SambaPartage->nb_depile);
			printf("  Acces IP total                 : %12lld\n",RepartIp.acces);
			printf("  Acces IP sans donnee           : %12lld",RepartIp.vide);
			if(RepartIp.acces) printf(" (%.1f%%%% des acces)",100.0 * (double)RepartIp.vide / (double)RepartIp.acces); printf("\n");
			printf("  Trames utiles                  : %12lld",RepartIp.traitees);
			if(RepartIpEchappees) printf(" (dont %d generee%s)",Accord1s(RepartIpEchappees)); printf("\n");
			printf("  Trames renvoyees               : %12lld\n",RepartIp.perdues);
			printf("  Total defauts de longueur      : %12lld\n",RepartIp.reduc);
			recues = 0;
			for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) recues += (int64)Repart[rep].acces_remplis;
			printf("  Trames UDP vues par Samba      : %12lld",recues);
			if(RepartIpEchappees) printf(" (et %d echappee%s)",Accord1s(RepartIpEchappees)); printf("\n");
			LectUdpStatus(&recues,&err,&ovr);
			printf("  Trames UDP vues par le Systeme : %12lld (echappees: %lld, en erreur: %lld)\n",recues-LectUdpRecues,ovr-LectUdpOvr,err-LectUdpErr);
		}
		
		vu = 0;
		m = 0; n = -1; for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif || Repart[rep].arrete) { n = rep; m++; }
		ok = 1;
		if(((LectErreur.code == LECT_EMPTY) && (LectErreur.num == 55)) || ((LectErreur.code == LECT_ARRETE) && (LectErreur.num == 56))) {
			for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif && (Repart[rep].bottom == 0) && (Repart[rep].top <= 0)) {
				int delai;
				delai = (int)(Repart[rep].dernier_vide - Repart[rep].dernier_rempli)/1000;
				if(!vu) { if(m > 1) printf("\nEtat des repartiteurs:\n"); else if(n >= 0) printf("\nEtat %s:\n",ModeleRep[Repart[n].type].nom); vu = 1; }
				if(delai) {
					printf("  %-10s: PAS DE DONNEE depuis %d ms, malgre %d demande%s\n",Repart[rep].nom,
						   delai,Accord1s(Repart[rep].serie_vide));
					ok = 0;
					OpiumWarn("Le repartiteur %s semble arrete",Repart[rep].nom);
				} else {
					printf("  %-10s: pas de donnee utilisable, malgre %d demande%s\n",Repart[rep].nom,
						   Accord1s(Repart[rep].serie_vide));
					ok = 0;
					OpiumWarn("Le repartiteur %s semble deborde",Repart[rep].nom);
				}
			}
		}
		for(rep=0; rep<RepartNb; rep++) if(Repart[rep].arrete) {
			if(!vu) { if(m > 1) printf("\nEtat des repartiteurs:\n"); else if(n >= 0) printf("\nEtat %s:\n",ModeleRep[Repart[n].type].nom); vu = 1; }
			printf("  %-10s retire de l'acquisition. %d voie%s abandonnee%s",Repart[rep].nom,
				   Accord2s(Repart[rep].nbdonnees));
			if(Repart[rep].nbdonnees < 3) printf(": "); else if(Repart[rep].nbdonnees > 0) printf(":\n             . ");
			n = 0;
			for(vl=0; vl<Repart[rep].nbdonnees; vl++) {
				voie = SambaEchantillon[Repart[rep].premiere_donnee + vl].voie; // Repart[rep].donnee[l];
				if(voie >= 0) { if(n++) printf(", "); printf("%s",VoieManip[voie].nom); }
			}
			printf("\n");
			ok = 0;
		}
//?		if(!ok) printf("\n");
		
#ifdef VERIF_ETAT_REPART_FINAL
		for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif && (rep != LectEnCours.rep)) break;
		if(rep >= RepartNb) LectEnCours.rep = -1;
		remplies = totales = valeurs = 0;
		for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) {
			printf("  %3s %-8s @%-16s : Pile=[%-6d, %6d]\n",(rep == LectEnCours.rep)? "(*)": " . ",
				   ModeleRep[Repart[rep].type].code,Repart[rep].parm,Repart[rep].bottom,Repart[rep].top);
			if((Repart[rep].bottom < 0) || (Repart[rep].top < 0)) 
				printf("                                   Pile=[%08X, %08X]\n",Repart[rep].bottom,Repart[rep].top); 
			printf("             %lld/%lld acces non-vides, %12lld valeurs recues",Repart[rep].acces_remplis,Repart[rep].acces_demandes,Repart[rep].donnees_recues);
			remplies += Repart[rep].acces_remplis; totales += Repart[rep].acces_demandes; valeurs += Repart[rep].donnees_recues;
			if(Repart[rep].acces_remplis) printf(" (%lld valeurs/appel)\n",Repart[rep].donnees_recues/Repart[rep].acces_remplis); else printf("\n");
		}
		if(LectEnCours.rep >= 0) printf("  (*) = Repartiteur en cours\n");
		if(m > 1) printf("   . Totaux: %lld/%lld acces non-vides, %12lld valeurs recues",remplies,totales,valeurs);
#endif
		
#ifdef VERIF_TIMESTAMP_FINAL
		if(IPdemande) {
			int nb_stamps;
			ok = 0;
			for(rep=0; rep<RepartNb; rep++) if((Repart[rep].famille == FAMILLE_OPERA) && Repart[rep].actif) {
				if(!ok) { printf("\nVerification de l'echantillonage via IP:\n"); ok = 1; }
				printf("    %-8s @%-16s (%d voies/echantillon, soit %d echantillons/trame)\n",
					   ModeleRep[Repart[rep].type].code,Repart[rep].parm,Repart[rep].nbdonnees,Repart[rep].s.ip.duree_trame);
				nb_stamps = (Repart[rep].nbdonnees >= VOIES_PAR_4OCTETS)? Repart[rep].s.ip.duree_trame: _nb_data_trame_udp;
				if(nb_stamps != Repart[rep].s.ip.duree_trame) printf("       Attention: %d echantillons par timestamp\n",Repart[rep].s.ip.duree_trame / nb_stamps);
				printf("       ");
				printf("Premiere trame lue: numero %5d",Repart[rep].s.ip.num0); printf("      ");
				printf("Derniere trame lue: numero %5d",Repart[rep].s.ip.num1); printf("\n");
				Repart[rep].s.ip.num1 += 1;  /* la trame num1 a ete effectivement lue, soit 0 a num1 trames lues ... */
				printf("       ");
				printf("Timestamp         : %12lld",Repart[rep].s.ip.stamp0); printf("      ");
				printf("Timestamp         : %12lld",Repart[rep].s.ip.stamp1); printf("\n");
				printf("       ");
				printf("Supplement        : %12d",Repart[rep].s.ip.num0 * nb_stamps); printf("      ");
				printf("Supplement        : %12d",Repart[rep].s.ip.num1 * nb_stamps); printf("\n");
				printf("       ");
				Repart[rep].s.ip.stamp0 += (int64)(Repart[rep].s.ip.num0 * nb_stamps);
				Repart[rep].s.ip.stamp1 += (int64)(Repart[rep].s.ip.num1 * nb_stamps);
				printf("Timestamp initial : %12lld",Repart[rep].s.ip.stamp0); printf("      ");
				printf("Timestamp final   : %12lld",Repart[rep].s.ip.stamp1); printf("\n");
				printf("       ");
				printf("Delta(timestamp)  : %12lld",Repart[rep].s.ip.stamp1 - Repart[rep].s.ip.stamp0); printf("      ");
				printf("Perdu             : %12lld\n",Repart[rep].s.ip.stampPerdus);
				if(nb_stamps != Repart[rep].s.ip.duree_trame) 
					printf("       Echantillons lus  : %12lld\n",(Repart[rep].s.ip.stamp1 - Repart[rep].s.ip.stamp0) * (int64)(Repart[rep].s.ip.duree_trame / nb_stamps));
			}
		}
#endif
		
#ifdef VERIF_NBEVTS_FINAL
		LectJournalCompteEvts();
#endif
			
		if(avec_perfos) {
			printf("\nTemps utilise en secondes");
			if(temps_reel) printf(" (et en %% [du temps reel]):\n"); else printf(":\n");
			
			tache = "Lecture";
			for(i=0; i<INTERF_NBTYPES; i++) if(LectUsageInterface[i]) {
				strcat(strcpy(supp,"dont acces "),InterfaceListe[i]);
				LectJournalCPU(tache,TempsTotalLect,supp,LectUsageInterface[i],temps_reel);
				tache = 0;
			}
			if(tache) LectJournalCPU(tache,TempsTotalLect,0,0,temps_reel);
			else if(RepartNbActifs > 1) LectJournalCPU(tache,TempsTotalLect,"soit total ",LectUsageInterfacesToutes,temps_reel);
			LectJournalCPU("dispo",TempsTotalDispo,"pour...",-1,temps_reel);
			temps_libre = TempsTotalDispo;
			LectJournalCPU("Traitmt",TempsTotalTrmt,"plus lecture",TrmtNbLus,temps_reel); temps_libre -= TempsTotalTrmt;
			LectJournalCPUpartiel("Processing",TrmtTempsFiltrage,temps_reel);
			LectJournalCPUpartiel("Reconstruction",TrmtTempsEvt,temps_reel);
			if(TrmtTempsArchive) {
				LectJournalCPUpartiel("Sauvegarde",TrmtTempsArchive,temps_reel);
			}
			if(ArchSauve[EVTS] || ArchSauve[STRM]) {
				LectJournalCPU("Archive",TempsTotalArch,"plus lecture",ArchNbLus,temps_reel); temps_libre -= TempsTotalArch;
			}
			if(temps_libre >= 0) LectJournalCPU("Attente",temps_libre,0,0,temps_reel);
			else LectJournalCPU("CPU#2",-temps_libre,0,0,temps_reel);
			
			// printf("\nTemps maxi  Reconstruction : %8lld us pour t =  %lld\n",CpuMaxEvt,CpuStampEvt);
			// printf("Temps maxi  Processing     : %8lld us pour t = [%lld .. %lld[ (%d donnee%s)\n",CpuMaxTrigger,CpuStampTrigger,CpuFinTrigger,Accord1s((int)(CpuFinTrigger-CpuStampTrigger)));
			// printf("Temps libre Processing     : %8lld us\n",(int64)((float)LectIntervTrmt * 1000.0 / SynchroD2_kHz));
			
			printf("\n");
			t = (((int64)(LectDateRunFin.tv_sec - LectDateRunDebut.tv_sec)) * 1000000) + (int64)(LectDateRunFin.tv_usec - LectDateRunDebut.tv_usec);
			reel = (float)((double)t / 1000000.0);
			cpu = (float)(LectCpuRunFin - LectCpuRunDebut)/(float)CLOCKS_PER_SEC;
			printf("Temps reel utilise : %13.3f s\n",reel);
			printf("Temps CPU  utilise : %13.3f s (%5.1f %%)\n",cpu,100.0 * cpu / reel);
			if(LectStatsValides) {
				t = (((int64)(StatsSystemeFin.ru_stime.tv_sec - StatsSystemeDebut.ru_stime.tv_sec)) * 1000000) + (int64)(StatsSystemeFin.ru_stime.tv_usec - StatsSystemeDebut.ru_stime.tv_usec);
				reel = (float)((double)t / 1000000.0);
				printf("| Mode systeme     : %13.3f s\n",reel);
				t = (((int64)(StatsSystemeFin.ru_utime.tv_sec - StatsSystemeDebut.ru_utime.tv_sec)) * 1000000) + (int64)(StatsSystemeFin.ru_utime.tv_usec - StatsSystemeDebut.ru_utime.tv_usec);
				reel = (float)((double)t / 1000000.0);
				printf("| Mode utilisateur : %13.3f s\n",reel);
			}
			printf("| . Lecture        : %13.3f s\n",(float)CpuTotalLect/(float)CLOCKS_PER_SEC);
			printf("| . Traitement     : %13.3f s\n",(float)CpuTotalTrmt/(float)CLOCKS_PER_SEC);
			printf("| . Archivage      : %13.3f s\n",(float)CpuTotalArch/(float)CLOCKS_PER_SEC);
#ifdef VERIF_STATSCPU_FINALES
			if(LectStatsValides) {
				printf("Mem. residente max : %13.3f GB\n",(double)StatsSystemeFin.ru_maxrss / 1048576.0);
				printf("Pages reclamees    : %9ld\n",StatsSystemeFin.ru_minflt-StatsSystemeDebut.ru_minflt);
				printf("Pages relues       : %9ld\n",StatsSystemeFin.ru_majflt-StatsSystemeDebut.ru_majflt);
				printf("Pages sorties      : %9ld\n",StatsSystemeFin.ru_nswap-StatsSystemeDebut.ru_nswap);
				printf("Lectures           : %9ld\n",StatsSystemeFin.ru_inblock-StatsSystemeDebut.ru_inblock);
				printf("Ecritures          : %9ld\n",StatsSystemeFin.ru_oublock-StatsSystemeDebut.ru_oublock);
				printf("Pauses volontaires : %9ld\n",StatsSystemeFin.ru_nvcsw-StatsSystemeDebut.ru_nvcsw);
				printf("Pauses forcees     : %9ld\n",StatsSystemeFin.ru_nivcsw-StatsSystemeDebut.ru_nivcsw);
			} else printf("Statistiques systeme indisponibles\n");
#endif
		}
	}

	if(rc) {
		printf("\nConditions de l'arret:\n");
	#ifdef PAROLE
		SpeakString("\pAcquisition in error");
	#endif
		printf("  Code de retour: %04X\n",rc);
		printf("  Donnee attendue: %04X\n",LectTypePrevu);
		LectExplique(rc);
		/* echo <ArchExplics> | mail -s "Samba alert" -c juillard@csnsm.in2p3.fr michel.gros@cea.fr */
		if(Gardiens[0]) {
			char texte[256];
			sprintf(texte,"echo \"Le %s a %s: %s sur %s, %s est stoppee.\" | mail -s \"Alerte Samba\" %s",
				DateCivile(),DateHeure(),ArchExplics,Acquis[AcquisLocale].code,LectSrceTexte,Gardiens);
			system(texte);
			printf("  Message envoye a %s\n",Gardiens);
		}
		printf("\n          ====== %s, %s est stoppee ======\n",ArchExplics,LectSrceTexte);
	} else {
	#ifdef PAROLE
		SpeakString("\pAcquisition stopped");
	#endif
        printf("\n============================== Fin normale de %s ==============================\n",LectSrceTexte);
	#ifdef JULES_VEUT_PAS
		if(!OpiumDisplayed(pLectEtat->cdr)) OpiumMessage(OPIUM_INFO,"Arret normal de %s",LectSrceTexte);
	#endif
	}
	LogBlocEnd();

	// LectCourbeDump(SAMBA_SHR_SUIVI);
}

#pragma mark - Acces temps-reel (en parallele)
/* ========================================================================== */
INLINE char SambaTraiteDonnee(int classe, char trmt, int voie, int32 donnee, char d3, int64 index, int32 *adrs, double *reelle) {
	int parm,nbdata,compte,modul; // int kx,ky;
	char dispo;

	/* Remplissage dans le tampon immediat si pas de traitement */
	if(trmt == NEANT) { if(!reelle) *adrs = donnee; return(1); }

	switch(trmt) {
		// case NEANT: if(!reelle) *adrs = donnee; dispo = 1; break;
		case TRMT_DEMODUL:
			dispo = 0;
			if(!(nbdata = VoieTampon[voie].trmt[classe].nbdata)) break;
			if(d3) {
				if(VoieTampon[voie].trmt[classe].compte > 1) {
					if(reelle) *reelle = VoieTampon[voie].trmt[classe].reelle * (double)VoieManip[voie].def.trmt[classe].gain / (double)nbdata;
					else *adrs = (int32)((VoieTampon[voie].trmt[classe].valeur * VoieManip[voie].def.trmt[classe].gain) / nbdata);
					dispo = 1;
					LectRecaleD3++;
				}
				VoieTampon[voie].trmt[classe].phase = 0;
				compte = 2;
			} else compte = VoieTampon[voie].trmt[classe].compte + 1;
			parm = VoieManip[voie].def.trmt[classe].p1;
			modul = Bolo[VoieManip[voie].det].d2;
			VoieTampon[voie].trmt[classe].compte = compte;
			switch(VoieTampon[voie].trmt[classe].phase) {
			  case 0:   /* phase 0: 1ere marge                    */
				if(compte > parm) {
					if(reelle) VoieTampon[voie].trmt[classe].reelle = *reelle;
					else VoieTampon[voie].trmt[classe].valeur = donnee;
					VoieTampon[voie].trmt[classe].phase++;
				}
				break;
			  case 1:   /* phase 1: 1ere demi-periode hors marges */
				if(compte <= ((modul / 2) /* pas de marge en fin de demi-periode */)) {
					if(reelle) VoieTampon[voie].trmt[classe].reelle += *reelle;
					else VoieTampon[voie].trmt[classe].valeur += donnee;
				} else VoieTampon[voie].trmt[classe].phase++;
				break;
			  case 2:  /* phase 2: marges inter-periodes         */
				if(compte > ((modul / 2) + parm)) {
					if(reelle) VoieTampon[voie].trmt[classe].reelle -= *reelle;
					else VoieTampon[voie].trmt[classe].valeur -= donnee;
					VoieTampon[voie].trmt[classe].phase++;
				}
				break;
			  case 3:  /* phase 3: 2eme demi-periode hors marges */
				if(reelle) VoieTampon[voie].trmt[classe].reelle -= *reelle;
				else VoieTampon[voie].trmt[classe].valeur -= donnee;
				if(compte == modul) {
					if(reelle) *reelle = VoieTampon[voie].trmt[classe].reelle * (double)VoieManip[voie].def.trmt[classe].gain / (double)nbdata;
					else *adrs = (int32)((VoieTampon[voie].trmt[classe].valeur * VoieManip[voie].def.trmt[classe].gain) / nbdata);
					VoieTampon[voie].trmt[classe].compte = 0;
					VoieTampon[voie].trmt[classe].phase = 0;
					dispo = 1;
				};
				break;
			};
			break;
	#ifdef TRMT_ADDITIONNELS
		case TRMT_LISSAGE:
			if(reelle) {
				/* X: valeur d'origine (pour pouvoir lisser) */
				if(VoieTampon[voie].trmt[classe].X.nb < VoieTampon[voie].trmt[classe].X.max) {
					VoieTampon[voie].trmt[classe].X.val[VoieTampon[voie].trmt[classe].X.nb] = *reelle;
					VoieTampon[voie].trmt[classe].X.nb += 1;
				} else {
					VoieTampon[voie].trmt[classe].X.val[VoieTampon[voie].trmt[classe].X.prem] = *reelle;
					VoieTampon[voie].trmt[classe].X.prem += 1;
					if(VoieTampon[voie].trmt[classe].X.prem >= VoieTampon[voie].trmt[classe].X.max) 
						VoieTampon[voie].trmt[classe].X.prem = 0;
				}
				/* lissage sur le tableau X */
				if(VoieTampon[voie].trmt[classe].compte < VoieManip[voie].def.trmt[classe].p1) {
					VoieTampon[voie].trmt[classe].reelle += *reelle;
					VoieTampon[voie].trmt[classe].compte += 1;
				} else {
					int k1;
					k1 = Modulo(index - VoieManip[voie].def.trmt[classe].p1,VoieTampon[voie].trmt[classe].X.max);
					VoieTampon[voie].trmt[classe].reelle += (*reelle - VoieTampon[voie].trmt[classe].X.val[k1]);
				}
				*reelle = VoieTampon[voie].trmt[classe].reelle * (double)VoieManip[voie].def.trmt[classe].gain / (double)VoieTampon[voie].trmt[classe].compte;
			} else {
				if(VoieTampon[voie].trmt[classe].compte < VoieManip[voie].def.trmt[classe].p1) {
					VoieTampon[voie].trmt[classe].valeur += donnee;
					VoieTampon[voie].trmt[classe].compte += 1;
				} else {
					int k1;
					k1 = Modulo(index - VoieManip[voie].def.trmt[classe].p1,VoieTampon[voie].brutes->max);				     
					VoieTampon[voie].trmt[classe].valeur += (donnee - VoieTampon[voie].brutes->t.sval[k1]);
				}
				*adrs = (int32)((VoieTampon[voie].trmt[classe].valeur * VoieManip[voie].def.trmt[classe].gain) / VoieTampon[voie].trmt[classe].compte);
			}
			dispo = 1;
			break;
		case TRMT_MOYENNE:
			if(reelle) VoieTampon[voie].trmt[classe].reelle += *reelle;
			else VoieTampon[voie].trmt[classe].valeur += donnee;
			VoieTampon[voie].trmt[classe].compte += 1;
			if(VoieTampon[voie].trmt[classe].compte < VoieManip[voie].def.trmt[classe].p1) dispo = 0;
			else {
				if(reelle) *reelle = VoieTampon[voie].trmt[classe].reelle * (double)VoieManip[voie].def.trmt[classe].gain / (double)VoieTampon[voie].trmt[classe].compte;
				else *adrs = (int32)((VoieTampon[voie].trmt[classe].valeur * VoieManip[voie].def.trmt[classe].gain) / VoieTampon[voie].trmt[classe].compte);
				VoieTampon[voie].trmt[classe].reelle = 0.0;
				VoieTampon[voie].trmt[classe].valeur = 0;
				VoieTampon[voie].trmt[classe].compte = 0;
				dispo = 1;
			}
			break;
		case TRMT_MAXIMUM:
			if(reelle) {
				if(VoieTampon[voie].trmt[classe].compte == 0) VoieTampon[voie].trmt[classe].reelle = *reelle;
				else if(*reelle > VoieTampon[voie].trmt[classe].reelle) VoieTampon[voie].trmt[classe].reelle = *reelle;
			} else {
				if(VoieTampon[voie].trmt[classe].compte == 0) VoieTampon[voie].trmt[classe].valeur = donnee;
				else if(donnee > VoieTampon[voie].trmt[classe].valeur) VoieTampon[voie].trmt[classe].valeur = donnee;
			}
			VoieTampon[voie].trmt[classe].compte += 1;
			if(VoieTampon[voie].trmt[classe].compte < VoieManip[voie].def.trmt[classe].p1) dispo = 0;
			else {
				if(reelle) *reelle = VoieTampon[voie].trmt[classe].reelle * (float)VoieManip[voie].def.trmt[classe].gain;
				else *adrs = (int32)((VoieTampon[voie].trmt[classe].valeur * VoieManip[voie].def.trmt[classe].gain));
				VoieTampon[voie].trmt[classe].reelle = 0.0;
				VoieTampon[voie].trmt[classe].valeur = 0;
				VoieTampon[voie].trmt[classe].compte = 0;
				dispo = 1;
			}
			break;
		#ifdef TRMT_DECONVOLUE
		case TRMT_DECONV:
			int pre,post; float perte;
			// pre = VoieManip[voie].def.trmt[classe].p1;
			valeur = (reelle)? *reelle: (float)donnee;
			/* X: valeur d'origine (pour pouvoir lisser) */
			if(VoieTampon[voie].trmt[classe].X.nb < VoieTampon[voie].trmt[classe].X.max) {
				VoieTampon[voie].trmt[classe].X.val[VoieTampon[voie].trmt[classe].X.nb] = valeur;
				VoieTampon[voie].trmt[classe].X.nb += 1;
			} else {
				VoieTampon[voie].trmt[classe].X.val[VoieTampon[voie].trmt[classe].X.prem] = valeur;
				VoieTampon[voie].trmt[classe].X.prem += 1;
				if(VoieTampon[voie].trmt[classe].X.prem >= VoieTampon[voie].trmt[classe].X.max)
					VoieTampon[voie].trmt[classe].X.prem = 0;
			}
			/* lissage sur le tableau X */
			if(VoieTampon[voie].trmt[classe].compte < VoieManip[voie].def.trmt[classe].p1) {
				if(reelle) VoieTampon[voie].trmt[classe].reelle += *reelle;
				else VoieTampon[voie].trmt[classe].valeur += donnee;
				VoieTampon[voie].trmt[classe].compte += 1;
			} else {
				int k1;
				k1 = Modulo(index - VoieManip[voie].def.trmt[classe].p1,VoieTampon[voie].trmt[classe].X.max);
				if(reelle) VoieTampon[voie].trmt[classe].reelle += (*reelle - VoieTampon[voie].trmt[classe].X.val[k1]);
				else VoieTampon[voie].trmt[classe].valeur += (donnee - VoieTampon[voie].trmt[classe].X.val[k1]);
			}
			// post = VoieManip[voie].def.trmt[classe].p2;
			// perte = expf(-VoieEvent[voie].horloge / VoieManip[voie].def.trmt[classe].rc);
			// SambaDeconvolue(VoieTampon[voie].deconvoluee,tampon,dim,pt0,duree_evt,base,pre,post,perte,0.0,CHARGE_NORMALISE);
			dispo = 1;
			break;
		#endif
		#ifdef A_LA_GG
		case TRMT_INTEDIF:
			valeur = (reelle)? *reelle: (float)donnee;
			/* X: valeur d'origine (pour pouvoir lisser) */
			if(VoieTampon[voie].trmt[classe].X.nb < VoieTampon[voie].trmt[classe].X.max) {
				VoieTampon[voie].trmt[classe].X.val[VoieTampon[voie].trmt[classe].X.nb] = valeur;
				VoieTampon[voie].trmt[classe].X.nb += 1;
			} else {
				VoieTampon[voie].trmt[classe].X.val[VoieTampon[voie].trmt[classe].X.prem] = valeur;
				VoieTampon[voie].trmt[classe].X.prem += 1;
				if(VoieTampon[voie].trmt[classe].X.prem >= VoieTampon[voie].trmt[classe].X.max) 
					VoieTampon[voie].trmt[classe].X.prem = 0;
			}
			/* lissage sur le tableau X */
			if(VoieTampon[voie].trmt[classe].compte < VoieManip[voie].def.trmt[classe].p1) {
				if(reelle) VoieTampon[voie].trmt[classe].reelle += *reelle;
				else VoieTampon[voie].trmt[classe].valeur += donnee;
				VoieTampon[voie].trmt[classe].compte += 1;
			} else {
				int k1;
				k1 = Modulo(index - VoieManip[voie].def.trmt[classe].p1,VoieTampon[voie].trmt[classe].X.max);
				if(reelle) VoieTampon[voie].trmt[classe].reelle += (*reelle - VoieTampon[voie].trmt[classe].X.val[k1]);
				else VoieTampon[voie].trmt[classe].valeur += (donnee - VoieTampon[voie].trmt[classe].X.val[k1]);
			}
			/* Y: valeurs lissees (pour pouvoir deriver) */
			valeur = (reelle)? VoieTampon[voie].trmt[classe].reelle: (double)VoieTampon[voie].trmt[classe].valeur;
			if(VoieTampon[voie].trmt[classe].Y.nb < VoieTampon[voie].trmt[classe].Y.max) {
				kx = VoieTampon[voie].trmt[classe].Y.nb;
				VoieTampon[voie].trmt[classe].Y.val[VoieTampon[voie].trmt[classe].Y.nb] = valeur;
				VoieTampon[voie].trmt[classe].Y.nb += 1;
			} else {
				kx = VoieTampon[voie].trmt[classe].Y.prem;
				VoieTampon[voie].trmt[classe].Y.val[VoieTampon[voie].trmt[classe].Y.prem] = valeur;
				VoieTampon[voie].trmt[classe].Y.prem += 1;
				if(VoieTampon[voie].trmt[classe].Y.prem >= VoieTampon[voie].trmt[classe].Y.max) 
					VoieTampon[voie].trmt[classe].Y.prem = 0;
			}
			/* derivation */
			if(VoieTampon[voie].trmt[classe].Y.nb < VoieManip[voie].def.trmt[classe].p2) { if(reelle) *reelle = 0.0; else *adrs = 0; }
			else {
				ky = kx - VoieManip[voie].def.trmt[classe].p2; if(ky < 0) ky += VoieTampon[voie].trmt[classe].Y.max;
				if(reelle) *reelle = (VoieTampon[voie].trmt[classe].Y.val[kx] - VoieTampon[voie].trmt[classe].Y.val[ky]) * (double)VoieManip[voie].def.trmt[classe].gain / (float)VoieTampon[voie].trmt[classe].compte);
				else *adrs = (int32)(((VoieTampon[voie].trmt[classe].Y.val[kx] - VoieTampon[voie].trmt[classe].Y.val[ky]) * VoieManip[voie].def.trmt[classe].gain) / VoieTampon[voie].trmt[classe].compte);
				// if(index < 50) printf("(%s    ) Z = Y[%d]=%d - Y[%d]=%d = %6d\n",__func__,kx,VoieTampon[voie].trmt[classe].Y.val[kx],ky,VoieTampon[voie].trmt[classe].Y.val[ky],*adrs);
			}
			dispo = 1;
			break;
		#endif /* A_LA_GG */
		#ifdef A_LA_MARINIERE
		case TRMT_CSNSM:
		{ int ix,iAx,iy,i1y,iCy,iBCy,i2BCy,iz,iDz; int64 d;
			float B,Yi,Z;
		  parm = VoieManip[voie].def.trmt[classe].p1;
		  if(VoieTampon[voie].trmt[classe].X.nb < VoieTampon[voie].trmt[classe].X.max) {
			  ix = VoieTampon[voie].trmt[classe].X.nb;
			  VoieTampon[voie].trmt[classe].X.nb += 1;
		  } else {
			  ix = VoieTampon[voie].trmt[classe].X.prem;
			  VoieTampon[voie].trmt[classe].X.prem += 1;
			  if(VoieTampon[voie].trmt[classe].X.prem >= VoieTampon[voie].trmt[classe].X.max)
				  VoieTampon[voie].trmt[classe].X.prem = 0;
		  }
		  VoieTampon[voie].trmt[classe].X.val[ix] = (reelle)? *reelle: (double)donnee;
		  if(VoieTampon[voie].trmt[classe].Y.nb < VoieTampon[voie].trmt[classe].Y.max) {
			  iy = VoieTampon[voie].trmt[classe].Y.nb;
			  VoieTampon[voie].trmt[classe].Y.nb += 1;
		  } else {
			  iy = VoieTampon[voie].trmt[classe].Y.prem;
			  VoieTampon[voie].trmt[classe].Y.prem += 1;
			  if(VoieTampon[voie].trmt[classe].Y.prem >= VoieTampon[voie].trmt[classe].Y.max)
				  VoieTampon[voie].trmt[classe].Y.prem = 0;
		  }
		  Yi = (reelle)? *reelle: (double)donnee;
		  d = (int64)TrmtCsnsm[parm].A;
		  if(LectStampsLus >= d) {
			  iAx = Modulo(LectStampsLus - d,VoieTampon[voie].trmt[classe].X.max);
			  Yi -= VoieTampon[voie].trmt[classe].X.val[iAx];
		  }
		  if(LectStampsLus >= 1) {
			  i1y = Modulo(LectStampsLus - 1,VoieTampon[voie].trmt[classe].Y.max);
			  Yi += VoieTampon[voie].trmt[classe].Y.val[i1y];
		  }
		  VoieTampon[voie].trmt[classe].Y.val[iy] = Yi;
		  d = (int64)TrmtCsnsm[parm].C;
		  if(LectStampsLus >= d) {
			  iCy = Modulo(LectStampsLus - d,VoieTampon[voie].trmt[classe].Y.max);
			  VoieTampon[voie].trmt[classe].V += VoieTampon[voie].trmt[classe].Y.val[iCy];
			  VoieTampon[voie].trmt[classe].W += VoieTampon[voie].trmt[classe].Y.val[iCy];
			  d = (int64)(TrmtCsnsm[parm].B + TrmtCsnsm[parm].C);
			  if(LectStampsLus >= d) {
				  iBCy = Modulo(LectStampsLus - d,VoieTampon[voie].trmt[classe].Y.max);
				  VoieTampon[voie].trmt[classe].V -= (2 * VoieTampon[voie].trmt[classe].Y.val[iBCy]);
				  d = (int64)((2 * TrmtCsnsm[parm].B) + TrmtCsnsm[parm].C);
				  if(LectStampsLus >= d) {
					  i2BCy = Modulo(LectStampsLus - d,VoieTampon[voie].trmt[classe].Y.max);
					  VoieTampon[voie].trmt[classe].V += VoieTampon[voie].trmt[classe].Y.val[i2BCy];
					  VoieTampon[voie].trmt[classe].W -= VoieTampon[voie].trmt[classe].Y.val[i2BCy];
				  }
			  }
		  }
		  B = (float)TrmtCsnsm[parm].B;
		  Z = Yi - (VoieTampon[voie].trmt[classe].W / (2 * B))
			  - (VoieTampon[voie].trmt[classe].V * ((B + TrmtCsnsm[parm].C) / (B * B)));
		  if(VoieTampon[voie].trmt[classe].Z.nb < VoieTampon[voie].trmt[classe].Z.max) {
			  iz = VoieTampon[voie].trmt[classe].Z.nb;
			  VoieTampon[voie].trmt[classe].Z.nb += 1;
		  } else {
			  iz = VoieTampon[voie].trmt[classe].Z.prem;
			  VoieTampon[voie].trmt[classe].Z.prem += 1;
			  if(VoieTampon[voie].trmt[classe].Z.prem >= VoieTampon[voie].trmt[classe].Z.max)
				  VoieTampon[voie].trmt[classe].Z.prem = 0;
		  }
		  VoieTampon[voie].trmt[classe].Z.val[iz] = Z;
		  d = (int64)TrmtCsnsm[parm].D;
		  if(LectStampsLus >= d) {
			  iDz = Modulo(LectStampsLus - d,VoieTampon[voie].trmt[classe].Z.max);
			  if(reelle) *reelle = Z - VoieTampon[voie].trmt[classe].Z.val[iDz];
			  else *adrs = (int32)(Z - VoieTampon[voie].trmt[classe].Z.val[iDz]);
		  } else if(reelle) *reelle = Z;
		  else *adrs = (int32)Z;
		}
		dispo = 1;
		break;
		#endif /* A_LA_MARINIERE */
	#endif /* TRMT_ADDITIONNELS */
		case TRMT_FILTRAGE: {
			TypeFiltre *filtre; TypeSuiviFiltre *suivi; TypeCelluleFiltrante *cellule;
			int n,m,i; int etage; TypeDonneeFiltree brute,entree,filtree;
			int nbdata;
		#ifdef RL
			float *template; int dim;
		#endif
			
			if(!(filtre = VoieTampon[voie].trmt[classe].filtre)) { if(!reelle) *adrs = donnee; }
			else if(!(suivi = &(VoieTampon[voie].trmt[classe].suivi))) { if(!reelle) *adrs = donnee; }
			else {
				// t1 = (VerifTempsPasse? DateMicroSecs(): 0);
				nbdata = suivi->nb;
				if(nbdata < suivi->max) n = nbdata; else n =  suivi->prems;
				if(reelle) entree = (TypeDonneeFiltree)*reelle;
				else
				#ifdef DONNEES_NON_SIGNEES
					entree = (TypeDonneeFiltree)(donnee - 8192.0);
				#else
					entree = (TypeDonneeFiltree)donnee;
				#endif
				filtree = brute = entree;
				for(etage=0; etage<filtre->nbetg; etage++) {
					cellule = &(filtre->etage[etage]);
					if(nbdata >= filtre->nbmax) {
						if(cellule->nbdir > 0) filtree = cellule->direct[cellule->nbdir-1] * entree;
						else filtree = 0.0;
						if(etage == 0) for(i=cellule->nbdir-2,m=n; i>=0; i--) { /* X = donnees brutes */
							m -= 1;
							if(m < 0) m = suivi->max - 1;
							filtree += (cellule->direct[i] * suivi->brute[m]);
						} else for(i=cellule->nbdir-2,m=n; i>=0; i--) { /* X = resultat cellule precedente */
							m -= 1;
							if(m < 0) m = suivi->max - 1;
							filtree += (cellule->direct[i] * suivi->filtree[etage-1][m]);
						}
						for(i=cellule->nbinv-1,m=n; i>=0; i--) {
							m -= 1;
							if(m < 0) m = suivi->max - 1;
							filtree -= (cellule->inverse[i] * suivi->filtree[etage][m]);
						}
						entree = filtree;
					} else filtree = entree;
					if(etage == 0) suivi->brute[n] = brute;
					suivi->filtree[etage][n] = entree;
				}
#ifdef RL
				if(VoieManip[voie].def.trmt[TRMT_PRETRG].template && VoieTampon[voie].unite.val) {	// ajout etage de filtrage par evt unite;
					dim = VoieTampon[voie].unite.arrivee - VoieTampon[voie].unite.depart;
					template = VoieTampon[voie].unite.val + VoieTampon[voie].unite.depart;
					if(nbdata >= dim) {												// taille suivi superieure a taille template !
						if(dim > 0) filtree = template[dim-1] * entree; else filtree = 0.0;
						for(i=dim-2,m=n; i>=0; i--) { 
							m -= 1;
							if(m < 0) m = suivi->max - 1;
							filtree += (template[i] * suivi->filtree[filtre->nbetg-1][m]);
						}
						filtree *= VoieTampon[voie].norme_evt;
						entree = filtree; // pas vraiment utile si on s'arrete la pour le filtrage
					} else filtree = entree;
				}	
#endif
				if(reelle) *reelle = (double)filtree * (double)VoieManip[voie].def.trmt[classe].gain;
				else
				#ifdef DONNEES_NON_SIGNEES
					*adrs = (int32)(((filtree + 8192.0) * (double)VoieManip[voie].def.trmt[classe].gain) + 0.5);
				#else
					*adrs = (int32)((filtree * (double)VoieManip[voie].def.trmt[classe].gain) + 0.5); // *adrs = (int32)filtree;
				#endif
				if(nbdata < suivi->max) suivi->nb += 1;
				else {
					suivi->prems += 1;
					if(suivi->prems >= suivi->max) suivi->prems = 0;
				}
			}
		}
		dispo = 1;
		break;

		default: if(!reelle) *adrs = donnee; dispo = 1; break;
	}
	return(dispo);
}
// #define DEBUG_TRIGGER
/* ========================================================================== */
INLINE char SambaDeclenche(int voie, float reelle) {
	char seuil_depasse,evt_trouve;
	float amplitude,niveau;
	TypeVoieSuivi *suivi;
	int64 delta_points;
	float montee,duree;

	evt_trouve = 0;	
	suivi = &(VoieTampon[voie].suivi);
	if(VoieTampon[voie].trig.trgr->type == TRGR_ALEAT) {
		if(DernierPointTraite > VoieTampon[voie].signal.futur_evt) {
			float fluct;
			TrmtCandidatSignale(0,voie,DernierPointTraite,reelle,10+TRGR_ALGO_RANDOM);
			evt_trouve = 1;
		#ifdef TIRAGE_GAUSSIEN
			do HasardTirage(VoieTampon[voie].trig.trgr->fluct,12,fluct) while(fluct < -19.5);
			VoieTampon[voie].signal.futur_evt = DernierPointTraite + (int64)((1.0 + (0.05 * fluct)) * VoieTampon[voie].signal.periode); // TrmtInhibeAutresVoies remet date_evt a 0..
		#else
			// HasardTirage(VoieTampon[voie].signal.periode,12,fluct); suivi->date_evt = DernierPointTraite + (int64)(expf(-fluct));
			do HasardTirage(1.0,12,fluct) while(fluct <= 0.0);
			VoieTampon[voie].signal.futur_evt = DernierPointTraite - (int64)(logf(fluct) * VoieTampon[voie].signal.periode * 1.4);
		#endif
		}
		suivi->precedente = reelle;
		return(evt_trouve);
	}
	delta_points = VoieTampon[voie].trmt[TRMT_AU_VOL].compactage * VoieTampon[voie].trmt[TRMT_PRETRG].compactage;
#ifdef DEBUG_TRIGGER
	if(!TrmtDejaDit) printf("Pic  (t=%d%03d,%02d0) = %g ADU\n",
		(int)suivi->dernierpic/1000000,Modulo(suivi->dernierpic,1000000)/100,Modulo(suivi->dernierpic,1000000)%100,
		VoieTampon[voie].signal.base);
#endif
	/* Style de recherche */
	if(VoieTampon[voie].trig.cherche == TRGR_EXTREMA) {
#ifdef DEBUG_TRIGGER
		if(!TrmtDejaDit) printf("Recherche des extremas (signal %g -> %g, deja %d0 µs passees)\n",
								suivi->precedente,reelle,VoieTampon[voie].signal.climbs);
#endif
		/* On recherche un pic => on regarde le changement de derivee */
		if(reelle > suivi->precedente) suivi->pente = 1;
		else if(reelle < suivi->precedente) suivi->pente = -1;
		/* si reelle = precedente, la pente est consideree ne pas changer */
		if((suivi->pente * VoieTampon[voie].signal.climbs) < 0) {
			/* la derivee a change de signe! */
			niveau = suivi->precedente;
			amplitude = niveau - VoieTampon[voie].signal.base;
			montee = (float)(abs(VoieTampon[voie].signal.climbs)) * VoieEvent[voie].horloge;
		#ifdef DEBUG_TRIGGER
			if(!TrmtDejaDit) printf("Depuis %d%03d,%02d0 ms sur %3g µs: montee de %4d ADU a partir de %4d\n",
				(int)suivi->dernierpic/1000000,Modulo(suivi->dernierpic,1000000)/100,Modulo(suivi->dernierpic,1000000)%100,
				montee,amplitude,VoieTampon[voie].signal.base);
		#endif
		#ifdef DEBUG_FILTREES
			if(point_stocke < 3000) printf("(%s) [%d] dernier pic: %d, montee %d->%d ADU pendant %g ms\n",__func__,
				(int)DernierPointTraite,(int)suivi->dernierpic,VoieTampon[voie].signal.base,amplitude,montee/1000.0);
		#endif
			
			if(Trigger.type == TRGR_SCRPT) {
				duree = ((float)(DernierPointTraite - suivi->dernierpic) - 0.5) * VoieEvent[voie].horloge * 1000;
				*(VoieTampon[voie].adrs_amplitude) = amplitude;
				*(VoieTampon[voie].adrs_montee) = montee;
				*(VoieTampon[voie].adrs_niveau) = suivi->precedente - amplitude;
				*(VoieTampon[voie].adrs_duree) = duree;
			#ifdef DEBUG_PRGM_0
				if(!TrmtDejaDit) printf("Calcul pour amplitude %f sur %f µs au niveau %f\n",
					*(VoieTampon[voie].adrs_amplitude),*(VoieTampon[voie].adrs_montee),*(VoieTampon[voie].adrs_niveau));
				if(!TrmtDejaDit) CebExec(TrmtPrgm,1); else
			#endif
					CebExec(TrmtPrgm,0);
			#ifdef DEBUG_PRGM
				if(!TrmtDejaDit) printf("Resultat: %f\n",*TrgrResultat);
			#endif
				if(*TrgrResultat) {
					if(SettingsTempsEvt == TRGR_DATE_MONTEE) suivi->date_evt = suivi->dernierpic;
					else suivi->date_evt = DernierPointTraite - (1 * delta_points); /* (SettingsTempsEvt == TRGR_DATE_MAXI) */
					/*
					 printf("Evt #%d a %d%03d,%02d: montee de %d ADU a partir de %d\n",Acquis[AcquisLocale].etat.evt_trouves+1,
					 (int)suivi->date_evt/1000000,Modulo(suivi->date_evt,1000000)/100,Modulo(suivi->date_evt,1000000)%100,
					 amplitude,VoieTampon[voie].signal.base);
					 */
					TrmtCandidatSignale(0,voie,suivi->date_evt,niveau,10+TRGR_ALGO_SCRIPT);
					evt_trouve = 1;
				}
				
			} else { /* (Trigger.type == TRGR_CABLE) */
				if(VoieTampon[voie].trig.trgr->type == TRGR_FRONT) {
					if((suivi->pente * VoieTampon[voie].trig.signe) <= 0) { /* pic dans le sens du signal? */
						if(((-suivi->pente * VoieTampon[voie].signal.climbs) >= VoieTampon[voie].trig.mincount) /* temps de montee suffisamt? */
						#ifdef DONNEES_NON_SIGNEES
						   && (VoieTampon[voie].signal.base > 0)   /* ligne de base non nulle? */
						#endif
						   && ((-suivi->pente * amplitude) >= VoieTampon[voie].trig.trgr->minampl)) { /* amplitude suffisante? */
							if((suivi->pente * amplitude) > 0) {
								/* Pb! l'amplitude est dans le mauvais sens... */
								OpiumFail("ERREUR SYSTEME dans %s",__func__);
								return(0);
							}
							if(SettingsTempsEvt == TRGR_DATE_MONTEE) suivi->date_evt = suivi->dernierpic;
							else suivi->date_evt = DernierPointTraite - (1 * delta_points); /* (SettingsTempsEvt == TRGR_DATE_MAXI) */
						#ifdef DEBUG_FILTREES
							if(Acquis[AcquisLocale].etat.evt_trouves < 5) {
								printf("(%s) [%d] Evt #%d au point %d sur %s: montee de %d ADU a partir de %d\n",__func__,
									   (int)DernierPointTraite,Acquis[AcquisLocale].etat.evt_trouves+1,(int)suivi->date_evt,
									   VoieManip[voie].nom,amplitude,VoieTampon[voie].signal.base);
							} else Acquis[AcquisLocale].etat.active = 0;
						#endif
							TrmtCandidatSignale(0,voie,suivi->date_evt,niveau,10+TRGR_ALGO_FRONT);
							evt_trouve = 1;
						}
					}
				} else if(VoieTampon[voie].trig.trgr->type == TRGR_PORTE) {
					if((suivi->pente * VoieTampon[voie].trig.signe) <= 0) { /* pic dans le sens du signal? */
						/* la derivee N'a PAS change de signe! et on n'a pas encore passe la mi-hauteur */
						duree = ((float)(DernierPointTraite - suivi->dernierpic) - 0.5) * VoieEvent[voie].horloge * 1000;
						if((duree >= VoieTampon[voie].trig.mindelai) /* duree suffisante? */
						#ifdef DONNEES_NON_SIGNEES
						   && (VoieTampon[voie].signal.base > 0)   /* ligne de base non nulle? */
						#endif
						   && ((-suivi->pente * amplitude) >= VoieTampon[voie].trig.trgr->minampl)) { /* amplitude suffisante? */
							if((suivi->pente * amplitude) > 0) {
								/* Pb! l'amplitude est dans le mauvais sens... */
								OpiumFail("ERREUR SYSTEME dans %s",__func__);
								return(0);
							}
							if(SettingsTempsEvt == TRGR_DATE_MONTEE) suivi->date_evt = suivi->dernierpic;
							else suivi->date_evt = DernierPointTraite - (1 * delta_points); /* (SettingsTempsEvt == TRGR_DATE_MAXI) */
							/*
							 printf("Evt #%d a %d%03d,%02d: montee de %d ADU a partir de %d\n",Acquis[AcquisLocale].etat.evt_trouves+1,
							 (int)suivi->date_evt/1000000,Modulo(suivi->date_evt,1000000)/100,Modulo(suivi->date_evt,1000000)%100,
							 amplitude,VoieTampon[voie].signal.base);
							 */
							TrmtCandidatSignale(0,voie,suivi->date_evt,niveau,10+TRGR_ALGO_PORTE);
							evt_trouve = 1;
						}
					}
				#ifdef VISU_FONCTION_TRANSFERT
				} else {
					/* pour VoieTampon[voie].trig.trgr->type == TRGR_TEST, il n'y a rien a faire a cet instant */
					if(TrmtHaffiche && VoieTampon[voie].traitees->t.rval && (Hnb > 0)) {
						debut = k - Hnb + 1;
						if((VoieTampon[voie].traitees->nb >= Hnb) && (debut >= 0)) {
							if(CalculeFFTplan(&CalcPlanStd,Hnb,raison)) {
								normalisation = (VoieEvent[voie].horloge / 1000.0) / (float)Hnb; /* secondes, soit 1/norme en Hz */
								ptr_short = VoieTampon[voie].brutes->t.sval + debut; ptr_fft = CalcPlanStd.temps;
								for(t=0; t<Hnb; t++) *ptr_fft++ = (fftw_real)*ptr_short++;
								rfftw_one(CalcPlanStd.plan,CalcPlanStd.temps,CalcPlanStd.freq);
								CalculePuissance(CalcPlanStd.freq,Xfiltre,Hnb,normalisation,1.0);
								
								//ptr_short = VoieTampon[voie].brutes->t.sval + debut; ptr_fft = CalcPlanStd.temps;
								//for(t=0; t<Hnb; t++) *ptr_fft++ = (fftw_real)*ptr_short++;
								if(VoieTampon[voie].traitees->nb < VoieTampon[voie].traitees->max)
									j = VoieTampon[voie].traitees->nb - Hnb;
								else {
									j = VoieTampon[voie].traitees->prem - Hnb;
									if(j < 0) j += VoieTampon[voie].traitees->max;
								}
								ptr_short = VoieTampon[voie].traitees->t.rval + j; ptr_fft = CalcPlanStd.temps;
								for(t=0; t<Hnb; t++) {
									*ptr_fft++ = (fftw_real)*ptr_short++;
									if(++j >= VoieTampon[voie].traitees->max) {
										j = 0; ptr_short = VoieTampon[voie].traitees->t.rval;
									}
								}
								rfftw_one(CalcPlanStd.plan,CalcPlanStd.temps,CalcPlanStd.freq);
								CalculePuissance(CalcPlanStd.freq,Yfiltre,Hnb,normalisation,1.0);
								
								nb = (Hnb / 2) + 1;
								for(t=0; t<nb; t++)
									if(Xfiltre[t] != 0.0) Hfiltre[t] = Yfiltre[t] / Xfiltre[t];
									else Hfiltre[t] = 10.0;
								GraphAxisReset(gTrmtFiltre,GRF_YAXIS);
								GraphUse(gTrmtFiltre,nb);
								OpiumDisplay(gTrmtFiltre->cdr);
								evt_trouve = 1;
								TrmtHaffiche = 0; /* sinon, on n'a plus jamais la main... */
							}
						}
					}
				#endif
				}
			}
		#ifdef UN_PEU_PARTICULIER
			if(!evt_trouve) {
				/* Histogrammes utilisateurs */
				/* en histogrammant ici, on a le bruit pour l'ensemble du signal
				 et non uniquement le bruit au moment de l'evenement */
				HistoDeVoie vh;
				vh = VoieHisto[voie];
				while(vh) {
					if(vh->Xvar == MONIT_BRUIT) HistoFillFloatToInt(vh->hd,fabsf(amplitude),1);
					vh = vh->suivant;
				}
				/* Histogrammes bruit */
				if(voie == SettingsHistoVoie) {
					if(TrmtHampl) HistoFillFloatToInt(hdBruitAmpl,fabsf(amplitude),1);
					if(TrmtHmontee) HistoFillFloatToInt(hdBruitMontee,montee,1);
					if(TrmtH2D) H2DFillFloatFloatToInt(h2D,fabsf(amplitude),montee,1);
				}
			}
		#endif /* UN_PEU_PARTICULIER */
			suivi->mihauteur = suivi->precedente - (amplitude / 2);
			VoieTampon[voie].signal.base = suivi->precedente;
			VoieTampon[voie].signal.climbs = 0;
			suivi->dernierpic = DernierPointTraite - (1 * delta_points);
			// if(!TrmtDejaDit) printf("Point commun=%d => dernier pic=%d\n",(int)DernierPointTraite,(int)suivi->dernierpic);
		}
		VoieTampon[voie].signal.climbs += (suivi->pente * VoieTampon[voie].trmt[TRMT_PRETRG].compactage);
		
	} else /* (VoieTampon[voie].trig.cherche == TRGR_NIVEAU) */ {
		/* si un evt est en cours, on cherche son amplitude (maxi avant descente sous le niveau de trigger) */
		if(suivi->date_evt) {
			if(!VoieTampon[voie].delai_template) {
				/* pas de template, pas de surprise */
				if(VoieTampon[voie].signal.climbs > 0) {
					if(reelle >= suivi->mihauteur) {  /* mihauteur represente le max (calcul en cours) ... */
						suivi->mihauteur = reelle;
						VoieTampon[voie].signal.climbs = (int)(DernierPointTraite - suivi->date_evt) / VoieTampon[voie].trmt[TRMT_AU_VOL].compactage;								
					} else
						// if(reelle < VoieTampon[voie].trig.trgr->minampl)  /* on n'arrete pas au premier maxi, on attend de redescendre */
						evt_trouve = 1;
				} else {
					if(reelle <= suivi->mihauteur) {  /* mihauteur represente le max (calcul en cours) ... */
						suivi->mihauteur = reelle;
						VoieTampon[voie].signal.climbs = (int)(suivi->date_evt - DernierPointTraite) / VoieTampon[voie].trmt[TRMT_AU_VOL].compactage;
					} else
						// if(reelle > VoieTampon[voie].trig.trgr->maxampl) /* on n'arrete pas au premier maxi, on attend de redescendre */
						evt_trouve = 1;
				}
			} else {
				/* le filtrage par template peut generer des oscillations y compris inversees et AVANT le maxi */
				if(fabsf(reelle) >= suivi->mihauteur) {  /* mihauteur represente le max (calcul en cours) ... */
					suivi->mihauteur = fabsf(reelle);
					suivi->posmax = DernierPointTraite;
					//- printf("(%s) A p=%d: evenement vu sur %s demarrant p=%d: maxi %d a p=%d, precedent a p=%d\n",__func__,(int)DernierPointTraite,VoieManip[voie].nom,(int)suivi->date_evt,suivi->mihauteur,(int)suivi->posmax,(int)TrmtDernierTrig);
				} else if((DernierPointTraite - suivi->date_evt) > VoieTampon[voie].delai_template) evt_trouve = 1;
			}																					
			if(evt_trouve) {
				//- printf("(%s) A p=%d: evenement vu sur %s demarrant p=%d +%d +/-%d, precedent a p=%d\n",__func__,(int)DernierPointTraite,VoieManip[voie].nom,(int)suivi->date_evt,VoieTampon[voie].signal.climbs,(int)delta_points,(int)TrmtDernierTrig);
				if(!VoieTampon[voie].delai_template) {
					/* date_evt inchange si TRGR_DATE_MONTEE */
					if(SettingsTempsEvt != TRGR_DATE_MONTEE) suivi->date_evt = DernierPointTraite - (1 * delta_points); /* (SettingsTempsEvt == TRGR_DATE_MAXI) */
					//- printf("(%s) A p=%d: evenement vu sur %s date a p=%d +%d +/-%d, precedent a p=%d\n",__func__,(int)DernierPointTraite,VoieManip[voie].nom,(int)suivi->date_evt,VoieTampon[voie].signal.climbs,(int)delta_points,(int)TrmtDernierTrig);
				} else {
					suivi->date_evt = suivi->posmax - VoieTampon[voie].delai_template;	 
					//- printf("(%s) A p=%d: recule de p=%d a p=%d\n",__func__,(int)DernierPointTraite,(int)suivi->posmax,(int)suivi->date_evt);
				}
				niveau = suivi->mihauteur;
				// pas utilise: amplitude = niveau - VoieTampon[voie].signal.base;
				//- if((DernierPointTraite - suivi->date_evt) > 200) printf("(%s) A p=%d: evenement vu sur %s pour p=%d +%d +/-%d, precedent a p=%d\n",__func__,(int)DernierPointTraite,VoieManip[voie].nom,(int)suivi->date_evt,VoieTampon[voie].signal.climbs,(int)delta_points,(int)TrmtDernierTrig);
				TrmtCandidatSignale(0,voie,suivi->date_evt,niveau,10+TRGR_ALGO_NIVEAU);
				suivi->date_evt = 0;
			}
			/* sinon, on regarde le depassement d'un seuil */
		} else {
			seuil_depasse = 0; suivi->pente = VoieTampon[voie].trig.signe;
			if(Trigger.type == TRGR_SCRPT) {
				*(VoieTampon[voie].adrs_niveau) = reelle;
				CebExec(TrmtPrgm,0);
				if(*TrgrResultat) seuil_depasse = 1;
			} else /* (Trigger.type == TRGR_CABLE) */ {
				if(VoieTampon[voie].trig.signe > 0) seuil_depasse = (reelle > VoieTampon[voie].trig.trgr->minampl);
				else if(VoieTampon[voie].trig.signe < 0) seuil_depasse = (reelle < VoieTampon[voie].trig.trgr->maxampl);
				else {
					if(reelle > VoieTampon[voie].trig.trgr->minampl) {
						seuil_depasse = 1; suivi->pente = 1;
						// if(DernierPointTraite < 10) printf("==> SEUIL DEPASSE\n");
					} else if(reelle < VoieTampon[voie].trig.trgr->maxampl) {
						seuil_depasse = 1; suivi->pente = -1;
					}
				}
			}
			if(seuil_depasse) {
				suivi->date_evt = DernierPointTraite - (1 * delta_points);
				VoieTampon[voie].signal.base = suivi->precedente;
				VoieTampon[voie].signal.climbs = (suivi->pente * VoieTampon[voie].trmt[TRMT_PRETRG].compactage);
				suivi->mihauteur = reelle; /* teste en realite le max de l'evt */
			}
		}
	} /* Fin des cas de trigger */
	suivi->precedente = reelle;
	
	/* ...............  Fin de recherche d'evenement ........................................ */
	// if(evt_trouve && VoieTampon[voie].trig.inhibe) break;  /* on ne scanne pas les autres bolos */
	
	return(evt_trouve);
}
/* ========================================================================== */
static INLINE char LectPilesActives(short pleins, short vides, char pas_tous_secs, int64 t0, int64 t1, int *a_depiler_global, char log) {
	short actifs;
	int a_depiler,nb,dep;
	int rep,voie,vl;
	TypeRepartiteur *repart;

	repart = 0; // GCC
	actifs = 0;	
	vides = 0;
	*a_depiler_global = -1;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif && !(Repart[rep].arrete) && (Repart[rep].nbdonnees > 0)) {
		repart = &(Repart[rep]);
		if(log) printf("          %s: %d voie%s lue%s en stream\n",repart->nom,Accord2s(repart->nbdonnees));
		actifs++;
		if(repart->passages >= SettingsDepileMax) printf("          %s: %d acces consecutifs, soit au bout de %lld us: donnees=[%d %d[ dans pile[%d])\n",
			repart->nom,repart->passages+1,t1-t0,repart->bottom,repart->top,repart->maxdonnees); // ", %s recopiee",repart->s.pci.en_cours?"pas encore":"deja"
		if(log) printf("          %s: utilisation des donnees dans [%d .. %d]\n",repart->nom,repart->bottom,repart->top);
		a_depiler = repart->top - repart->bottom;
		if(repart->interf == INTERF_IP) a_depiler += repart->s.ip.manquant;
		if(repart->depot_nb) {
			nb = 0;
			for(dep=0; dep<REPART_RESERVOIR_MAX; dep++) if(repart->depot[dep].reserve != repart->depot[dep].fond) {
				a_depiler += (repart->depot[dep].reserve - repart->depot[dep].fond); nb++;
			}
			if(log) printf("          %s: + utilisation de %d reservoir%s, total = %d donnee%s\n",repart->nom,Accord1s(nb),Accord1s(a_depiler));
		}
		if(!a_depiler && (repart->famille != FAMILLE_CLUZEL)) vides++;
		if(*a_depiler_global < 0) *a_depiler_global = a_depiler;
		else if(a_depiler < *a_depiler_global) *a_depiler_global = a_depiler;
	};
	if(log) printf("          A depiler globalement: %d (sources %s)\n",*a_depiler_global,pas_tous_secs?"en cours":"videes");
	/* re-evaluation si blocage */
	if(*a_depiler_global) { LectDerniereNonVide = t0; LectSerieVide = 0; }
	else {
		LectSerieVide++;
		if(((t0 - LectDerniereNonVide) < 3000000) /* && (LectSerieVide < SettingsDepileMax) */ ) {
			if(pleins && ((pleins + vides) == actifs)) {
				*a_depiler_global = -1;
				for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) {
					repart = &(Repart[rep]);
					a_depiler = repart->top - repart->bottom;
					if(repart->interf == INTERF_IP) a_depiler += repart->s.ip.manquant;
					if(a_depiler) {
						//+ 16.12.10: en laissant *a_depiler_global<=0, on ne genere pas de constante
						//+ if(*a_depiler_global < 0) *a_depiler_global = a_depiler;
						//+ else if(a_depiler < *a_depiler_global) *a_depiler_global = a_depiler;
					} else {
						if(!repart->arrete) {
							printf("%s! %s n'envoie pas de donnee...\n",DateHeure(),repart->nom);
							repart->arrete = 1;
						}
						// if(repart->famille == FAMILLE_OPERA) repart->s.ip.numAVenir = -1;
					}
				}
			}
		} else {
			int64 nb_totales,nb_remplies,nb_vides;

			printf("%s! Rien a depiler depuis %lld us (au bout de %d fois)\n          Temps courant  : %lld us\n          Temps precedent: %lld us\n",
				   DateHeure(),(t0 - LectDerniereNonVide),LectSerieVide,t0 - LectT0Run,LectDerniereNonVide - LectT0Run);
			printf("          Lectures sur IP dernierement tentees: %d, reussies: %d [lors de la derniere activation du vidage des piles HW]\n",RepartIpEssais,RepartIpRecues);
			//a for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif && (Repart[rep].top == Repart[rep].bottom)) 
			//a 	printf("          Derniere trame pour %s a %.3f s\n",Repart[rep].nom,Repart[rep].temps_precedent);
			nb_totales = nb_remplies = nb_vides = 0;
			actifs = 0;
			for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) {
				repart = &(Repart[rep]);
				printf("          %-10s: %7lld demandes (%7lld utiles + %7lld vides), ",
					repart->nom,repart->acces_demandes,repart->acces_remplis,repart->acces_vides);
				nb_totales += repart->acces_demandes; nb_remplies += repart->acces_remplis; nb_vides += repart->acces_vides;
				if(repart->dernier_rempli > repart->dernier_vide) 
					printf("dernier appel utile a %lld (%d vide en cours)\n",repart->dernier_rempli,repart->serie_vide);
				else printf("dernier appel vide  a %lld (serie de %d en %lld us)\n",
					repart->dernier_vide,repart->serie_vide,repart->dernier_vide-repart->dernier_rempli);
				actifs++;
			}
			if(actifs > 1) printf("          %-10s: %7lld demandes (%7lld utiles + %7lld vides)\n",
				"total",nb_totales,nb_remplies,nb_vides);
			for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif && (Repart[rep].top == Repart[rep].bottom)) {
				int precedent; char on_arrete;
				on_arrete = (RepartNbActifs == 1);
				if(!on_arrete) {
					char buffer[80];
					printf("%s!!! %s arrete, ",DateHeure(),Repart[rep].nom);
					/* option: tester si ping, l'UDP sera relance par Depile */
					sprintf(buffer,COMMANDE_PING,IP_CHAMPS(Repart[rep].adrs));
					if(system(buffer) != 0) {
						printf(" et l'adresse %d.%d.%d.%d:%d ne repond plus.\n",IP_CHAMPS(Repart[rep].adrs),Repart[rep].ecr.port);
						on_arrete = 1;
					} else printf(" mais'adresse %d.%d.%d.%d:%d repond toujours.\n",IP_CHAMPS(Repart[rep].adrs),Repart[rep].ecr.port);
				}
				if(on_arrete) {
					Repart[rep].arrete = 1;
					Repart[rep].actif = 0;
					printf("%s!!! %s retire de l'acquisition. %d voie%s abandonnee%s",DateHeure(),
						   Repart[rep].nom,Accord2s(Repart[rep].nbdonnees));
					if(Repart[rep].nbdonnees < 3) printf(": "); else if(Repart[rep].nbdonnees > 0) printf(":");
					nb_remplies = 0; precedent = -1;
					for(vl=0; vl<Repart[rep].nbdonnees; vl++) {
						voie = SambaEchantillon[Repart[rep].premiere_donnee + vl].voie; // Repart[rep].donnee[vl];
						if(voie >= 0) {
							if((VoieManip[voie].det != precedent) && (Repart[rep].nbdonnees >= 3)) printf("\n             . ");
							else if(nb_remplies++) printf(", ");
							printf("%s",VoieManip[voie].nom);
							if(VoieTampon[voie].lue) VoieTampon[voie].lue = -1;
							precedent = VoieManip[voie].det;
						}
					}
					printf("\n");
					actifs = 0; LectRefStamp = -1;
					for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) {
						if((LectRefStamp < 0) && (Repart[rep].interf == INTERF_IP)) LectRefStamp = rep;
						actifs++;
					}
					if(LectModeStatus) printf("          . Nouvelle reference du timestamp: %s\n",(LectRefStamp >= 0)? Repart[LectRefStamp].nom: "aucune");
					if(!actifs) {
						printf("          ! Plus aucun repartiteur n'est actif!!\n");
						// LectDeclencheErreur(_ORIGINE_,56,LECT_EMPTY); return(0);
						// if(VerifErreurStoppe)
						RepartiteurStoppeTransmissions(repart,1);
						LectDeclencheErreur(_ORIGINE_,55,LECT_ARRETE); return(0);
					}
				}
			}
		}
	}
	return(1);
}
#define LECT_SAT_LNGR 1512
/* ========================================================================== */
void LectDepileSat() {
	int j,sat,nb;
	int64 t0,t1,tlim;
	clock_t c0,cpu_utilise;
	hexa lngr; byte trame[LECT_SAT_LNGR];
	TypeAcquisMsg *msg;

	if(!Acquis[AcquisLocale].etat.active) return;
	//	if(!VoiesLocalesNb) return;
	if(LectDepileEnCours) { LectDeclencheErreur(_ORIGINE_,55,LECT_OVERFLO); return; }
	LectDepileEnCours = 1;
	c0 = VerifConsoCpu? clock() : 0;
	t0 = t1 = DateMicroSecs(); // (VerifTempsPasse? DateMicroSecs(): 0);
	if(!LectDepileSousIt) printf("[%lld] %s appelee\n",t0,__func__);
	LectDepileAttente = t0 - LectDepileTfin;
	if((LectDepileAttente / 1000) > (2 * SettingsReadoutPeriod)) {
		printf("%s/ Delai entre lectures anormal: %lld us, niveau max precedent: PCI, %d; IP, %d\n",DateHeure(),LectDepileAttente,RepartPci.depilees,RepartIp.depilees);
		LectDelaisTropLong++;
	}
	if(LectDepileAttente > LectDepileIntervMax) LectDepileIntervMax = LectDepileAttente;
	TempsTotalDispo += LectDepileAttente;
	LectDepileNb++;

	Acquis[AcquisLocale].etat.evt_trouves = 0;
	for(j=0; j<Partit[PartitLocale].nbsat; j++)  {
		sat = Partit[PartitLocale].sat[j];
		tlim = t1 + 1000000;
		do {
			lngr = sizeof(TypeSocket);
			nb = recvfrom(Acquis[sat].ecoute.path,trame,LECT_SAT_LNGR,0,&(Acquis[sat].ecoute.skt),&lngr);
			if(nb == -1) { if(errno != EAGAIN) printf("%s/ ! %s hors service (%s)\n",DateHeure(),Acquis[sat].code,strerror(errno)); break; } 
			else if(nb > 0) {
				/* {	int j;
					printf("%s/ Lu: %d/%d [",DateHeure(),nb,sizeof(TypeAcquisMsg));
					for(j=0; j<nb; j++) printf("%c%02X",(j%4)?'.':' ',trame[j]);
					printf(" ]\n");
				} */
				if(BigEndianOrder) ByteSwappeShortArray((shex *)trame,2);
				msg = (TypeAcquisMsg *)trame;
				// printf("%9s msg: %04X.%04X/%s/%s/%d\n"," ",msg->catg,msg->sat,msg->m.etat.nom_run,msg->m.etat.duree_lue,msg->m.etat.evt_trouves);
				if(msg->sat != sat) { printf("%s/ ! Le satellite #%x se prend pour le #%X\n",DateHeure(),sat,msg->sat); /* break; */ }
				switch(msg->catg) {
					case SAT_ETAT:
						strcpy(Acquis[sat].etat.nom_run,msg->m.etat.nom_run);
						strcpy(Acquis[sat].etat.duree_lue,msg->m.etat.duree_lue);
						if(BigEndianOrder) {
							ByteSwappeInt((hexa *)(&msg->m.etat.evt_trouves));
							ByteSwappeInt((hexa *)(&msg->m.etat.evt_ecrits));
						}
						Acquis[sat].etat.evt_trouves = msg->m.etat.evt_trouves;
						Acquis[sat].etat.evt_ecrits = msg->m.etat.evt_ecrits;
						Acquis[AcquisLocale].etat.evt_trouves += Acquis[sat].etat.evt_trouves;
						Acquis[AcquisLocale].etat.evt_ecrits += Acquis[sat].etat.evt_ecrits;
						break;
					case SAT_EVT:
						if(BigEndianOrder) ByteSwappeLong(&msg->m.evt.timestamp);
						break;
				}
			}
			t1 = DateMicroSecs();
		} while(t1 < tlim); // timeout a prevoir
	}

	LectDepileTfin = t1; // (VerifTempsPasse? DateMicroSecs(): 0);
	LectDepileDuree = t1 - t0;
	TempsTotalLect += LectDepileDuree;
	TempsTempoLect += LectDepileDuree;
	cpu_utilise = VerifConsoCpu? clock() - c0: 0;
	CpuTempoLect += cpu_utilise;
	CpuTotalLect += cpu_utilise;
	LectDepileEnCours = 0;
	return;
}
/* ========================================================================== */
void LectDepileDonnees() {
	/* Veut vider les pile PCI et/ou IP, et memoriser les valeurs */
	int64 t0,t1,t2; // int64 blocs_avant;
	clock_t c0,cpu_utilise;
	TypeRepartiteur *repart;
	TypeLarge val32; TypeDonnee val16;
	char trmt; char echantillon_termine,pas_tous_secs,arrete,deadline; char episode_en_cours,lues;
	short pleins,vides;
#ifdef AVEC_PCI
	int a_depiler,dep;
#endif
	int a_depiler_global,voie,dispo;
	//a float dernier_temps;
	int nb,rep,fmt; // lu
	char log;
	int64 nb_avant,nb_lues;
#ifdef VERIFIE_TYPE
	TypeADU type;
	char verifie_type;
#endif

	/* struct timeval date; gettimeofday(&date,0);
	printf("(%s) t=%d.%06d\n",__func__,date.tv_sec & 0xFFFF,date.tv_usec); */

	log = 0; // VerifRunDebug;
	repart = 0; // GCC
	//#define APPELS 1000
	/* Attention: attente de 10 secondes insuffisante pendant les regens, gerees par LectTraiteStocke */
#ifdef APPELS
	if(LectDansTraiteStocke) {
		if(++LectDansTraiteStocke > APPELS) { LectDepileInhibe = 1; exit(EX_TEMPFAIL); }
	}
	/*	if(LectDansActionUtilisateur) {
		if(++LectDansActionUtilisateur > APPELS) { LectDepileInhibe = 1; return; }
	 } */
	if(LectDansDisplay) {
		if(++LectDansDisplay > APPELS) { LectDepileInhibe = 1; return; }
	}
#endif
	if(!Acquis[AcquisLocale].etat.active) return;
	// Acquis[AcquisLocale].etat.evt_trouves++; // pour simulation avec repart sans BB
	if(LectDepileEnCours) {
		printf("%s/ Delai entre lectures trop court: %lld us\n",DateHeure(),DateMicroSecs() - LectDepileTfin);
		LectDelaisTropCourt++;
		// if(LectDelaisTropCourt > 100) LectDeclencheErreur(_ORIGINE_,40,LECT_OVERFLO);
		return;
	}
	LectDepileEnCours = 1;
	c0 = VerifConsoCpu? clock() : 0;
	t0 = DateMicroSecs(); // (VerifTempsPasse? DateMicroSecs(): 0);
	t1 = t0; // GCC
	//+ if(!LectDepileSousIt) printf("[%lld] %s appelee\n",t0,__func__);
	LectDepileAttente = t0 - LectDepileTfin;
	if((LectDepileAttente / 1000) > (2 * SettingsReadoutPeriod)) {
		printf("%s/ Delai entre lectures anormal: %lld us, niveau piles max precedent: PCI, %d; IP, %d\n",DateHeure(),LectDepileAttente,RepartPci.depilees,RepartIp.depilees);
		LectDelaisTropLong++;
	}
	if(LectDepileAttente > LectDepileIntervMax) LectDepileIntervMax = LectDepileAttente;
	TempsTotalDispo += LectDepileAttente;
	LectDepileNb++;
	if(SambaPartage) SambaPartage->nb_depile += 1;
	
	/*
	 *  Mise a jour d'infos de deverminage
	 */
#ifdef DEBUG_DEPILE
	if(LectDepileStockes < MAXDEPILE) LectDepileStockes++;
	else {
		int i;
		for(i=0; i<LectDepileStockes-1; i++) LectSerie[i] = LectSerie[i+1];
	}
	LectSerie[LectDepileStockes-1] = 0;
#ifdef DEBUG_FILL
	if(LectDepileStockes > 1) {
		int64 qtte;
		
		qtte =  LectSerie[LectDepileStockes - 2];
		if(qtte > LectFillMax) {
			LectFillMax2 = LectFillMax;
			LectFillMax = qtte;
		} else if(qtte > LectFillMax2) LectFillMax2 = qtte;
	}
#endif /* DEBUG_FILL */
#endif /* DEBUG_DEPILE */
	
	/*
	 *  Remplissage des tampons primaires
	 */
	RepartIpEssais = 0;
	RepartIpRecues = 0;
	RepartIpNiveauPerte = 0;
	episode_en_cours = 0;
	if(log > 1) printf("%s/ Sequence de lecture #%d\n",DateHeure(),LectDepileNb);
	if(RepartSeul) {
        RepartSeul->passages = 0;
		if(log) { nb_avant = RepartSeul->donnees_recues; t1 = (VerifTempsPasse? DateMicroSecs(): 0); }
		do {
			pas_tous_secs = 0;
			pleins = 0;
			if(RepartSeul->famille == FAMILLE_CALI) lues = CaliVidePile(RepartSeul,&pas_tous_secs,&pleins,log);
			else lues = IpeVidePile(RepartSeul,&pas_tous_secs,&pleins,log);
			if(LectErreur.code) goto retour;
			if(RepartSeul->s.ip.en_defaut) episode_en_cours = 1;
		} while(pas_tous_secs && (++(RepartSeul->passages) < SettingsDepileMax));
		if(log) {
			nb_lues = RepartSeul->donnees_recues - nb_avant; t2 = (VerifTempsPasse? DateMicroSecs(): 0) - t1;
			printf("          %8lld[%3d] %lld/%3lld us\n",t1-LectT0Run,RepartSeul->passages,nb_lues,t2);
		}
	} else {
		if(RepartNb == 1) t1 = (VerifTempsPasse? DateMicroSecs(): 0);
		for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) Repart[rep].passages = 0;
		deadline = 0;
		do {
			pas_tous_secs = 0;
			pleins = 0;
			episode_en_cours = 0;
			for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) {
                repart = &(Repart[rep]);
				if(log > 1) printf("          %s: debut de lecture avec deja des donnees dans [%-5d .. %5d]\n",repart->nom,repart->bottom,repart->top);
				if(RepartNb > 1) t1 = (VerifTempsPasse? DateMicroSecs(): 0);
				lues = RepartVidePile(repart,&pas_tous_secs,&pleins,log);
				if(RepartNb > 1) LectUsageInterface[repart->interf] += ((VerifTempsPasse? DateMicroSecs(): 0) - t1);
				if(LectErreur.code) goto retour;
				if(repart->s.ip.en_defaut) episode_en_cours = 1;
				//- if(repart->simule) pas_tous_secs = 0;
				if(!deadline) deadline = (++(repart->passages) >= SettingsDepileMax);
				if(log && lues) printf("          %s: fin de lecture avec maintenant des donnees dans [%-5d .. %5d] (%s les repartiteurs sont secs)\n",repart->nom,repart->bottom,repart->top,pas_tous_secs?"pas tous":"tous");
			}
		} while(pas_tous_secs && !deadline);
		if(RepartNb == 1) LectUsageInterface[Repart[0].interf] += ((VerifTempsPasse? DateMicroSecs(): 0) - t1);
	}
	// if(RepartNbActifs > 1) LectUsageInterfacesToutes += ((VerifTempsPasse? DateMicroSecs(): 0) - t0);
	if(LectEpisodeEnCours && !episode_en_cours) {
		printf("%s/ Fin episode #%d\n",DateHeure(),LectEpisodeAgite);
		LectEpisodeEnCours = 0;
	}
	if(LectModeStatus) {
		if(LectRefStamp >= 0) {
			int64 d2;
			d2 =  Repart[LectRefStamp].s.ip.stampDernier / Diviseur2;
			/* if(d2 != SynchroD2lues)  printf("(%s) Stamp[%s]=%lld / D2=%d soit %lld SynchroD2 lues au lieu de %lld\n",
				__func__,Repart[LectRefStamp].nom,Repart[LectRefStamp].s.ip.stampDernier,Diviseur2,d2,SynchroD2lues); */
			SynchroD2lues = d2; // Repart[LectRefStamp].s.ip.stampDernier / Diviseur2;
		}
		goto fin_vidage;
	}
    vides = 0;
	if(!LectPilesActives(pleins,vides,pas_tous_secs,t0,t1,&a_depiler_global,log)) goto retour;

	/*
	 *  Boucle tant que les tampons contiennent quelque chose
	 */
	if(a_depiler_global <= 0) goto fin_vidage;
	LectNbDepilees = 0;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) Repart[rep].depile= 0;
#ifdef VERIFIE_TYPE
	verifie_type = 0;
#endif
	LectEnCours.rep = SambaEchantillon[LectEnCours.echantillon].rep;
	if(LectEnCours.rep >= 0) {
		repart = &(Repart[LectEnCours.rep]);
		LectVoieRep = LectEnCours.echantillon - repart->premiere_donnee;
		arrete = repart->arrete;
		if(log > 1) {
			printf("          Transfert commencant sur la voie %s:#%d [bloc %lld], %s le status\n",
				   Repart[LectEnCours.rep].nom,LectVoieRep,LectStampsLus+1,LectEnCours.is_status?"c'est":"ce n'est pas");
			printf("          A depiler finalement: %d valeur%s (%s %s et %s)\n",Accord1s(a_depiler_global),repart->nom,repart->actif?"actif":"inactif",repart->arrete?"arrete":"demarre");
		}
	} else { repart = 0; arrete = 0; }

	forever {
		voie = SambaEchantillon[LectEnCours.echantillon].voie;
		if(voie < 0) goto voie_suivante;
		if(repart) {
			// printf("          A depiler finalement: %d valeur%s (%s %s et %s)\n",Accord1s(a_depiler_global),repart->nom,repart->actif?"actif":"inactif",repart->arrete?"arrete":"demarre");
			if(repart->arrete || !repart->actif) LectEnCours.dispo = 0;
			/* 
			 *  Recupere la derniere valeur lue, la met dans LectEnCours.valeur, et teste l'etat de la FIFO
			 */
			else if(repart->interf == INTERF_IP) {
				// if(log > 2) printf("          Bas pile: %04X %04X, D3 %s\n",repart->data16[repart->bottom],repart->data16[repart->bottom+1],RepartD3Attendues?"attendue":"deja trouvee");
				if(RepartD3Attendues) { LectVoieRep = repart->nbdonnees; goto voie_suivante; }
				// remise en ligne a la main pour optimisation: if(!(LectEnCours.dispo = !RepartIpDepile1Donnee(repart,&LectEnCours.valeur))) break;
				if(repart->s.ip.manquant && (repart->bottom == repart->s.ip.saut)) {
					repart->s.ip.lu_entre_D3++;
					if(--repart->s.ip.manquant == 0) {
						repart->s.ip.en_defaut = 0;
						if(repart->s.ip.lu_entre_D3 >= repart->s.ip.dim_D3) repart->s.ip.lu_entre_D3 -= repart->s.ip.dim_D3;
					}
					if(voie >= 0) {
						if(VoieTampon[voie].brutes->type == TAMPON_INT16) LectEnCours.valeur = VoieTampon[voie].derniere_s16;
						else if(VoieTampon[voie].brutes->type == TAMPON_INT32) LectEnCours.valeur = VoieTampon[voie].derniere_i32;
					}
					LectEnCours.dispo = 1;
				} else if(repart->bottom < repart->top) {
					if(VoieTampon[voie].brutes->type == TAMPON_INT16) {
						if(BigEndianOrder) LectEnCours.valeur = (lhex)repart->data16[repart->bottom++];
						else if(repart->bottom & 1) LectEnCours.valeur = (lhex)repart->data16[repart->bottom++ - 1];
						else LectEnCours.valeur = (lhex)repart->data16[++repart->bottom];
					} else if(VoieTampon[voie].brutes->type == TAMPON_INT32) {
						if(BigEndianOrder) LectEnCours.valeur = repart->fifo32[repart->bottom++];
						else if(repart->bottom & 1) LectEnCours.valeur = repart->fifo32[repart->bottom++ - 1];
						else LectEnCours.valeur = repart->fifo32[++repart->bottom];
					}
					LectEnCours.dispo = 1;
					if(repart->bottom >= repart->top) repart->top = repart->bottom = repart->s.ip.saut = 0;
					repart->depile += 1;
				} else { repart->top = repart->bottom = repart->s.ip.saut = 0; LectEnCours.dispo = 0; }
				if(!LectEnCours.dispo) break;
			} else if(repart->famille == FAMILLE_ARCHIVE) {
				if(repart->bottom < FIFOdim) LectEnCours.valeur = (int32)repart->data16[repart->bottom++]; else break;
				LectEnCours.dispo = 1; repart->depile += 1;
			} else if(repart->famille == FAMILLE_CLUZEL) {
				if(!(LectEnCours.dispo = CluzelDepile1Donnee(repart,&LectEnCours.valeur,&nb,log))) break;
				LectNbDepilees += (nb - 1);
			} else if(repart->famille == FAMILLE_NI) {
				if(!(LectEnCours.dispo = NiDepile1Donnee(repart,&LectEnCours.valeur))) break;
			}
			LectNbDepilees++;
		}
		
		/* Verification du type de donnee */
	#ifdef VERIFIE_TYPE
		if(verifie_type) {
			if((type = DATATYPE(LectEnCours.valeur)) != LectTypePrevu) {
			#ifdef PAS_D_ERREUR_PERMISE
				LectDeclencheErreur(_ORIGINE_,49,type? type: LECT_UNKN); 
				#ifdef DEBUG_DEPILE
				LectSerie[LectDepileStockes-1] = LectNbDepilees;
				#endif /* DEBUG_DEPILE */
				LectErreursNb++;
				goto retour;
			#else /* PAS_D_ERREUR_PERMISE */
				if(!LectErreur1) {
					if(type) LectErreur1 = type; else LectErreur1 = LECT_UNKN;
					LectMarque1 = LectStampsLus;
				}
				if(LectErreursNb < 10) printf("%s/ Trouve %08X, attendu ****%04hX @bloc %lld [entree Rep%d.%d]\n",DateHeure(),LectEnCours.valeur,LectTypePrevu,LectStampsLus,LectEnCours.rep,LectVoieRep);
				// else Acquis[AcquisLocale].etat.active = 0;
				LectErreursNb++;
				break;
			#endif /* PAS_D_ERREUR_PERMISE */
			}
		}
	#endif /* VERIFIE_TYPE */		

		if(LecturePourRepart && repart && (repart == RepartConnecte)) {
			if(repart->mode && repart->check && (voie >= 0)) {
				char erreur; int32 masque,fixe,attendu,correct,precedente;
				if(!LectStampsLus) printf("%s/ %s.%d: trouve %08X\n",DateHeure(),repart->nom,LectVoieRep+1,LectEnCours.valeur);
				else {
					erreur = 0; fixe = attendu = 0; precedente = (int32)SambaEchantillon[LectEnCours.echantillon].precedente & 0xFFFF;
					if((masque = RepartMode[repart->mode].masque_fixe)) {
						fixe = precedente & masque;
						if((LectEnCours.valeur & masque) != fixe) erreur++;
					}
					if((masque = RepartMode[repart->mode].masque_rampe)) {
						attendu = precedente & masque; if(attendu == masque) attendu = 0; else attendu += RepartMode[repart->mode].offset;
						if((LectEnCours.valeur & masque) != attendu) erreur++;
					}
					if(erreur) {
						correct = fixe | attendu;
						printf("%s/ %s.%d: trouve %08X, attendu %08X (stamp %lld)\n",DateHeure(),repart->nom,LectVoieRep+1,LectEnCours.valeur,correct,LectStampsLus);
						RepartDataErreurs++;
						// LectEnCours.valeur = correct;
					}
				}
				SambaEchantillon[LectEnCours.echantillon].precedente = (TypeADU)(LectEnCours.valeur & 0xFFFF);
			}
			if(repart->status_demande) {
				RepartConnexionEnCours[LectVoieRep].valeur = LectEnCours.valeur;
				if(RepartConnexionEnCours[LectVoieRep].tampon.t.sval) {
					// val32 = (LectEnCours.valeur & 0x8000)? LectEnCours.valeur & 0x7FFF: LectEnCours.valeur | 0x8000;
					val32 = LectEnCours.valeur & repart->masque; if(val32 & repart->zero) val32 |= repart->extens;
					if(RepartConnexionEnCours[LectVoieRep].tampon.type == TAMPON_INT16) {
						if(RepartConnexionEnCours[LectVoieRep].tampon.nb < RepartConnexionEnCours[LectVoieRep].tampon.max) {
							RepartConnexionEnCours[LectVoieRep].tampon.t.sval[(RepartConnexionEnCours[LectVoieRep].tampon.nb)++] = (TypeDonnee)(val32 & 0xFFFF);
						} else {
							RepartConnexionEnCours[LectVoieRep].tampon.t.sval[(RepartConnexionEnCours[LectVoieRep].tampon.prem)++] = (TypeDonnee)(val32 & 0xFFFF);
							if(RepartConnexionEnCours[LectVoieRep].tampon.prem >= RepartConnexionEnCours[LectVoieRep].tampon.max)
								RepartConnexionEnCours[LectVoieRep].tampon.prem = 0;
						}
					} else if(RepartConnexionEnCours[LectVoieRep].tampon.type == TAMPON_INT32) {
						if(RepartConnexionEnCours[LectVoieRep].tampon.nb < RepartConnexionEnCours[LectVoieRep].tampon.max) {
							RepartConnexionEnCours[LectVoieRep].tampon.t.ival[(RepartConnexionEnCours[LectVoieRep].tampon.nb)++] = val32;
						} else {
							RepartConnexionEnCours[LectVoieRep].tampon.t.ival[(RepartConnexionEnCours[LectVoieRep].tampon.prem)++] = val32;
							if(RepartConnexionEnCours[LectVoieRep].tampon.prem >= RepartConnexionEnCours[LectVoieRep].tampon.max)
								RepartConnexionEnCours[LectVoieRep].tampon.prem = 0;
						}
					}
				}
				goto voie_suivante;
			}
		}

		if(LectCntl.LectMode != LECT_IDENT) /* dedie aux pseudo voies */  {
			if(voie >= 0) {
				TypeComposantePseudo *composante; float resul;
				if(VoieTampon[voie].brutes->type == TAMPON_INT16) {
					if(VoieManip[voie].pseudo) {
						resul = 0.0; composante = VoieManip[voie].pseudo;
						while(composante) {
							resul += composante->coef * (float)(VoieTampon[composante->srce].courante_s16);
							composante = composante->svt;
						}
						LectEnCours.valeur = (TypeDonnee)(resul + 0.5); LectEnCours.dispo = 1;
						VoieTampon[voie].derniere_s16 = LectEnCours.valeur;
					} else if(LectEnCours.dispo) VoieTampon[voie].derniere_s16 = LectEnCours.valeur;
					else { LectEnCours.valeur = VoieTampon[voie].derniere_s16; LectEnCours.dispo = 1; }
				} else if(VoieTampon[voie].brutes->type == TAMPON_INT32) {
					if(VoieManip[voie].pseudo) {
						resul = 0.0; composante = VoieManip[voie].pseudo;
						while(composante) {
							resul += composante->coef * (float)(VoieTampon[composante->srce].courante_i32);
							composante = composante->svt;
						}
						LectEnCours.valeur = (TypeLarge)(resul + 0.5); LectEnCours.dispo = 1;
						VoieTampon[voie].derniere_i32 = LectEnCours.valeur;
					} else if(LectEnCours.dispo) VoieTampon[voie].derniere_i32 = LectEnCours.valeur;
					else { LectEnCours.valeur = VoieTampon[voie].derniere_i32; LectEnCours.dispo = 1; }
				}
			} else { if(!LectEnCours.dispo) LectEnCours.valeur = 0; }
		}
		
		/* 
         *  Cherche a stocker la derniere valeur lue, LectEnCours.valeur, et avance les pointeurs
         */
		/* 
		 1/ On attendait le status: simple verification sans autre modif
		 */
		if((voie == SAMBA_ECHANT_STATUS) && (LectCntl.LectMode != LECT_IDENT)) {
			if(repart) {
			#ifdef DEBUG_STATUS_MIG
				val32 = LectEnCours.valeur & repart->masque;
				if(LectStatusNb < STATUSLIST_LNGR) { /*if(val32) */ LectStatusList[LectStatusNb++] = LectEnCours.valeur; }
			#endif
				LectSynchroD3 = ((LectEnCours.valeur & repart->p.pci.bitD2D3) != 0);
				if(LectSynchroD3) LectDerniereD3 = LectStampsLus;
				if(SettingsStatusCheck) {
					if((rep = (LectEnCours.valeur & LectMasqueStatus[LectEnCours.rep])) != 0) {
					#ifdef DEBUG_DEPILE
						LectSerie[LectDepileStockes-1] = LectNbDepilees;
					#endif
						LectDeclencheErreur(_ORIGINE_,50,rep); goto retour;
					}
				}
			}
			
		} else if(!arrete) {
			/* 
			 2/ Donnee courante
             */
           /* 2.a/ donnee pour identification */
            if(LectCntl.LectMode == LECT_IDENT) {
				if(repart) {
					int transmise;
					val32 = LectEnCours.valeur & repart->masque;
					transmise = LectEnCours.echantillon;
					if(VoieIdent[transmise].lus >= VoieIdent[transmise].dim) {
						int vt;
						for(vt=0; vt<VoieIdentMax; vt++) if(VoieIdent[vt].lus < VoieIdent[vt].dim) break;
						if(vt >= VoieIdentMax) Acquis[AcquisLocale].etat.active = 0;
					} else {
						*(VoieIdent[transmise].prochain)++ = (TypeADU)val32;
						VoieIdent[transmise].lus++;
					}
				}
				
			/* 2.b/ donnee provenant des detecteurs */
            } else if((voie >= 0) && VoieTampon[voie].brutes && VoieTampon[voie].brutes->max) {
                /* Decodage de la donnee. Note: DATAVALEUR(voie,val) := (val & VoieManip[voie].ADCmask) */
				if(repart && (repart->famille == FAMILLE_CALI)) {
					//+ val16 = (TypeDonnee)((LectEnCours.valeur & VoieManip[voie].ADCmask) - VoieManip[voie].zero); zero=0x2000 avec RP!!
					//* VoieManip[voie].zero a remplacer par qqchose issu de numeriseur->bits_valides
					if(repart->revision == 1) val16 = (TypeDonnee)(LectEnCours.valeur - (int)0x8000);
					else val16 = (TypeDonnee)((int)0x8000) - LectEnCours.valeur;
					// if(repart->depile < 32) printf("%4d/ recu: %04X(%d), extrait: %04X, retenu: %04X, valeur: %d\n",repart->depile, LectEnCours.valeur,LectEnCours.valeur,adu,val16,val16);
				// } else if((repart->famille == FAMILLE_OPERA) && (repart->r.opera.mode == code_acqui_EDW_BB2)) {
				// 		val16 = (LectEnCours.valeur & 0x8000)? LectEnCours.valeur & 0x7FFF: LectEnCours.valeur | 0x8000;
				} else if(VoieTampon[voie].brutes->type == TAMPON_INT16) {
					if(VoieManip[voie].ADCmask == 0xFFFF) val16 = (TypeDonnee)LectEnCours.valeur;
					else {
						val16 = DATAVALEUR(voie,LectEnCours.valeur);
						if(VoieManip[voie].signe) {
						#ifdef DONNEES_NON_SIGNEES
							if(val16 < VoieManip[voie].zero) val16 += VoieManip[voie].zero;
							else val16 -= VoieManip[voie].zero;
						#else
							#ifdef SIGNE_ARITHMETIQUE
							if(val16 >= VoieManip[voie].zero) val16 -= (2 * VoieManip[voie].zero);
							#else  /* SIGNE_ARITHMETIQUE */
							if(val16 & VoieManip[voie].zero) val16 |= VoieManip[voie].extens;
							#endif /* SIGNE_ARITHMETIQUE */
						#endif
						}
					}
				} else if(VoieTampon[voie].brutes->type == TAMPON_INT32) {
					if(VoieManip[voie].ADCmask == 0xFFFFFFFF) val32 = (TypeDonnee)LectEnCours.valeur;
					else {
						val32 = DATAVALEUR(voie,LectEnCours.valeur);
						if(VoieManip[voie].signe) {
						#ifdef DONNEES_NON_SIGNEES
							if(val32 < VoieManip[voie].zero) val32 += VoieManip[voie].zero;
							else val32 -= VoieManip[voie].zero;
						#else
							#ifdef SIGNE_ARITHMETIQUE
							if(val32 >= VoieManip[voie].zero) val32 -= (2 * VoieManip[voie].zero);
							#else  /* SIGNE_ARITHMETIQUE */
							if(val32 & VoieManip[voie].zero) val32 |= VoieManip[voie].extens;
							#endif /* SIGNE_ARITHMETIQUE */
						#endif
						}
					}
				}
				/* Traitement au vol de ladite */
				#define TRAITEMENT_AU_VOL
				trmt = VoieTampon[voie].trmt[TRMT_AU_VOL].utilise;
				if(VoieTampon[voie].brutes->type == TAMPON_INT16) {
					int32 resultat;
					dispo = SambaTraiteDonnee(TRMT_AU_VOL,trmt,voie,val16,LectSynchroD3,VoieTampon[voie].lus,&resultat,0);
					*VoieTampon[voie].prochain_s16 = (TypeDonnee)resultat;
				} else if(VoieTampon[voie].brutes->type == TAMPON_INT32)
					dispo = SambaTraiteDonnee(TRMT_AU_VOL,trmt,voie,val32,LectSynchroD3,VoieTampon[voie].lus,VoieTampon[voie].prochain_i32,0);
				/* Stockage de la meme */
				if(dispo) {
					TypeDonnee *prochain_s16; TypeLarge *prochain_i32;
					if(VoieTampon[voie].brutes->type == TAMPON_INT16) {
						prochain_s16 = VoieTampon[voie].prochain_s16;
						if(((VoieTampon[voie].brutes->prem + 1) == VoieTampon[voie].brutes->max) || ((VoieTampon[voie].brutes->nb + 1) == VoieTampon[voie].brutes->max))
							prochain_s16 = VoieTampon[voie].brutes->t.sval;
						else prochain_s16++;
						VoieTampon[voie].courante_s16 = *VoieTampon[voie].prochain_s16;  /* pour la reutilisation de voies (pseudos) */
					} else if(VoieTampon[voie].brutes->type == TAMPON_INT32) {
						prochain_i32 = VoieTampon[voie].prochain_i32;
						if(((VoieTampon[voie].brutes->prem + 1) == VoieTampon[voie].brutes->max) || ((VoieTampon[voie].brutes->nb + 1) == VoieTampon[voie].brutes->max))
							prochain_i32 = VoieTampon[voie].brutes->t.ival;
						else prochain_i32++;
						VoieTampon[voie].courante_i32 = *VoieTampon[voie].prochain_i32;  /* pour la reutilisation de voies (pseudos) */
					}
					VoieTampon[voie].lus++;
					/* Gestion du buffer circulaire */
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
						if(SettingsModeDonnees == 0) {
						#ifdef DEBUG_DEPILE
							LectSerie[LectDepileStockes-1] = LectNbDepilees;
						#endif
							LectDeclencheErreur(_ORIGINE_,51,LECT_FULL);
							goto retour;
						}
						VoieTampon[voie].circulaire = 1;
						VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien++;
						VoieTampon[voie].brutes->prem++;
						if(VoieTampon[voie].brutes->prem >= VoieTampon[voie].brutes->max) {
							// VoieTampon[voie].circulaire = 1; on le savait deja
							VoieTampon[voie].brutes->prem = 0;
							VoieTampon[voie].trmt[TRMT_AU_VOL].point0 = VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien;
						}
						if(VoieTampon[voie].brutes->type == TAMPON_INT16)
							VoieTampon[voie].prochain_s16 = VoieTampon[voie].brutes->t.sval + VoieTampon[voie].brutes->prem;
						else if(VoieTampon[voie].brutes->type == TAMPON_INT32)
							VoieTampon[voie].prochain_i32 = VoieTampon[voie].brutes->t.ival + VoieTampon[voie].brutes->prem;
					}
					if(VoieTampon[voie].brutes->type == TAMPON_INT16) {
						if(prochain_s16 != VoieTampon[voie].prochain_s16) {
							printf("%s/ !! Ecriture dans le buffer de %s a la mauvaise place [%lld]\n",VoieManip[voie].nom,++LectSyncRead);
						}
					} else if(VoieTampon[voie].brutes->type == TAMPON_INT32) {
						if(prochain_i32 != VoieTampon[voie].prochain_i32) {
							printf("%s/ !! Ecriture dans le buffer de %s a la mauvaise place [%lld]\n",VoieManip[voie].nom,++LectSyncRead);
						}
					}
				}
			}
			LectEnCours.dispo = 0;
		}

	voie_suivante:
		/* 3/ Mise a jour des pointeurs pour la prochaine donnee */
		if(SambaEchantillonLngr == 1) {
			LectSynchroD3 = 0;
			LectStampsLus++;
			if(LectDureeActive) for(fmt=0; fmt<ARCH_TYPEDATA; fmt++) ArchTrancheReste[fmt]--;
			if(!Acquis[AcquisLocale].etat.active) break;
			if(--LectAvantSynchroD2 == 0) { SynchroD2lues++; LectAvantSynchroD2 = Diviseur2; }
		} else {
#ifdef VERIFIE_TYPE
			LectTypePrevu = CLUZEL_MARQUE_NEANT; verifie_type = 0;
#endif
			echantillon_termine = 0;
			if(++LectEnCours.echantillon < SambaEchantillonLngr) {
				LectEnCours.rep = SambaEchantillon[LectEnCours.echantillon].rep;
				if(LectEnCours.rep >= 0) {
					repart = &(Repart[LectEnCours.rep]);
					LectVoieRep = LectEnCours.echantillon - repart->premiere_donnee;
				} else repart = 0;
			} else echantillon_termine = 1;
			if(echantillon_termine) {
				LectStampsLus++; /* Point en temps suivant */
				if(LectDureeActive) for(fmt=0; fmt<ARCH_TYPEDATA; fmt++) ArchTrancheReste[fmt]--;
				if(!Acquis[AcquisLocale].etat.active) break;
				LectEnCours.echantillon = 0;
				LectEnCours.rep = SambaEchantillon[LectEnCours.echantillon].rep;
				if(LectEnCours.rep >= 0) {
					repart = &(Repart[LectEnCours.rep]);
					LectVoieRep = LectEnCours.echantillon - repart->premiere_donnee;
				} else repart = 0;
				if(--LectAvantSynchroD2 == 0) /* tous les blocs ont ete lus: synchro D2 suivante */ {
					SynchroD2lues++;
					//	if(SettingsSynchroMax) {  /* Arret exact si prealablement prevu */
					//		if(SynchroD2lues >= SettingsSynchroMax) { Acquis[AcquisLocale].etat.active = 0; break; }
					//	}
					if(RepartD3Attendues) {
						if(!LectSynchroD3) /* synchro D3 attendue et pas encore arrivee: jeter tout ce qui a ete lu jusqu'ici */ {
							LectRazTampons(0); LectRazCompteurs();
						} else {
							Repart[LectEnCours.rep].D3trouvee = 1;
							RepartCompteCalagesD3();
						};
					};
					LectAvantSynchroD2 = Diviseur2;
				#ifdef VERIFIE_TYPE
					if(repart) LectTypePrevu = (ModeleRep[repart->type].niveau >= 0)? CLUZEL_MARQUE_MODULATION: CLUZEL_MARQUE_ECHANT; // traite la carte HD
					else LectTypePrevu = CLUZEL_MARQUE_NEANT;
				#endif
				}
				#ifdef VERIFIE_TYPE
					else LectTypePrevu = CLUZEL_MARQUE_ECHANT;
				#endif
				LectSynchroD3 = 0;
			#ifdef VERIFIE_TYPE
				if(repart) verifie_type = !LectSurFichier && (repart->famille == FAMILLE_CLUZEL); else verifie_type = 0;
			#endif
			}
		}
		
		LectEnCours.precedente = LectEnCours.valeur;
#ifdef ACCES_PCI_DIRECT
#ifdef AVEC_PCI
		if(PCIdemande && (LectEnCours.rep >= 0)) {
			LectEnCours.valeur = *(PCIadrs[Repart[LectEnCours.rep].adrs.val-1]); // IcaFifoRead(repart->p.pci.port);
			LectEnCours.dispo = 1;
#ifdef CLUZEL_FIFO_INTERNE
			if(repart->top < (repart->octets / sizeof(Ulong))) repart->top++;
			else { repart->bottom = 0; repart->top = 1; }
#endif
		}
#endif
#endif /* ACCES_PCI_DIRECT */
	}
	if(LectErreur.code) goto retour;
	//	if(LectSurFichier) printf("(LectDepileBoost) Lu les blocs de p=%d a p=%d\n",(int)blocs_avant,(int)LectStampsLus);
	if(PCIdemande) for(rep=0; rep<RepartNb; rep++) if((Repart[rep].famille == FAMILLE_CLUZEL) && Repart[rep].actif) {
		if(Repart[rep].depile >= (Repart[rep].depilable * 90 / 100)) {
			printf("%s/ %s: FIFO utilisee a %.2f %%%%\n",DateHeure(),
				   Repart[rep].nom,100.0*(float)Repart[rep].depile/(float)Repart[rep].depilable);
			printf("          Niveau FIFO precedent: %.2f %%%%, %lld us avant\n",
				   100.0*(float)RepartPci.depilees/(float)Repart[rep].depilable,LectDepileAttente);
		}
		RepartPci.depilees = Repart[rep].depile;
	}
	if(LectNbDepilees > LectMaxDepilees) LectMaxDepilees = LectNbDepilees;
#ifdef DEBUG_DEPILE
	LectSerie[LectDepileStockes-1] = LectNbDepilees;
#endif
#ifndef PAS_D_ERREUR_PERMISE
	if(LectErreursNb) { LectDeclencheErreur(_ORIGINE_,52,LECT_UNEXPEC); goto retour; }
#endif
	
fin_vidage:
	if(Acquis[AcquisLocale].etat.active) {
		if(LectSurFichier) { //rachid
			Repart[RepartFile].top = Repart[RepartFile].bottom = 0; LectEnCours.dispo = 0;
			if(repart->s.f.terminee) { LectDeclencheErreur(_ORIGINE_,53,LECT_EOF); /* Acquis[AcquisLocale].etat.active = 1; ?? */}
		}
		if(log > 1) printf("          Fin de sequence de lecture, transfert PCI %s\n",PCIdemande? "a prevoir": "ignore");
#ifdef AVEC_PCI
		if(PCIdemande) {
			char alerte;
			t2 = (VerifTempsPasse? DateMicroSecs(): 0);
			alerte = 0;
			if(RepartPci.date) {
				if((t2 - RepartPci.date) > RepartPci.delai) {
					alerte = 1;
					if(log > 1) printf("          "); else printf("%s/ ",DateHeure());
					printf("%s <%06lld>: alerte delai entre lectures PCI, deja %lld us\n",repart->nom,t2-t0,t2 - RepartPci.date);
					printf("          %s: pile %s ([%d %d]), %s recopiee\n",
						   repart->nom,(repart->top == 0)?"vide":"remplie",repart->bottom,repart->top,repart->s.pci.en_cours?"pas encore":"deja");
					a_depiler_global = -1;
					for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) {
						repart = &(Repart[rep]);
						if(repart->famille == FAMILLE_CLUZEL) printf("          %s <%06lld>: Test pile #%d ([%d %d]), %s recopiee\n",repart->nom,t2-t0,repart->passages+1,repart->bottom,repart->top,repart->s.pci.en_cours?"pas encore":"deja");
						printf("          %s: utilisation des donnees dans [%d .. %d]\n",repart->nom,repart->bottom,repart->top);
						a_depiler = repart->top - repart->bottom;
						if(repart->interf == INTERF_IP) a_depiler += repart->s.ip.manquant;
						if(repart->depot_nb) {
							nb = 0;
							for(dep=0; dep<REPART_RESERVOIR_MAX; dep++) if(repart->depot[dep].reserve != repart->depot[dep].fond) {
								a_depiler += (repart->depot[dep].reserve - repart->depot[dep].fond); nb++;
								printf("          %s:   . reservoir %d [%d..%d]\n",repart->nom,dep,repart->depot[dep].fond,repart->depot[dep].reserve);
							}
							printf("          %s: + utilisation de %d reservoir%s, total = %d donnee%s\n",repart->nom,Accord1s(nb),Accord1s(a_depiler));
						}
						if(!a_depiler && (repart->famille != FAMILLE_CLUZEL)) vides++;
						if(a_depiler_global < 0) a_depiler_global = a_depiler;
						else if(a_depiler < a_depiler_global) a_depiler_global = a_depiler;
					};
					printf("          A depiler globalement: %d (sources initialement %s)\n",a_depiler_global,pas_tous_secs?"en cours":"videes");
				}
			}
#ifndef ACCES_PCI_DIRECT
			for(rep=0; rep<RepartNb; rep++) if((Repart[rep].famille == FAMILLE_CLUZEL) && Repart[rep].actif) {
				repart = &(Repart[rep]);
#ifdef RESERVOIRS
				if(repart->top > 0) {
					int nb;
					dep = repart->depot_recent;
					if(!repart->depot[dep].reservoir) {
						repart->depot[dep].capacite = CLUZEL_FIFO_INTERNE / sizeof(Ulong);
						repart->depot[dep].reservoir = (Ulong *)malloc(repart->depot[dep].capacite * sizeof(Ulong));
						bzero(repart->depot[dep].reservoir,repart->depot[dep].capacite * sizeof(Ulong));
						repart->depot[dep].fond = repart->depot[dep].reserve = 0;
						repart->depot_nb += 1;
					}
					if(repart->depot[dep].reserve == 0) {
						nb = repart->top - repart->bottom;
						if(repart->depot[dep].capacite >= (/* repart->depot[dep].reserve + */ nb)) {
							//							memcpy(repart->depot[dep].reservoir+repart->depot[dep].reserve,repart->fifo32+repart->bottom,nb * sizeof(Ulong));
							memcpy(repart->depot[dep].reservoir,repart->fifo32+repart->bottom,nb * sizeof(Ulong));
							repart->depot[dep].reserve += nb;
							if(log || alerte) printf("          %s: remplissage du reservoir #%d a %d/%d\n",
													 repart->nom,dep,repart->depot[dep].reserve,repart->depot[dep].capacite);
							repart->top = repart->bottom = 0;
						} else printf("%s/ %s: reservoir #%d trop petit\n",DateHeure(),repart->nom,dep);
						dep++; repart->depot_recent = (dep < REPART_RESERVOIR_MAX)? dep: 0;
					} else if(alerte) printf("          %s: reservoir #%d inutilisable (deja %d donnee%s)\n",repart->nom,dep,Accord1s(repart->depot[dep].reserve));
				} else if(repart->s.pci.en_cours || alerte) printf("%s/ %s <%06lld>: pile pas encore recopiee\n",DateHeure(),repart->nom,t2-t0);
#endif /* RESERVOIRS */
				if(!(repart->s.pci.en_cours) && (repart->top <= 0)) {
					t1 = (VerifTempsPasse? DateMicroSecs(): 0);
					if(RepartPci.date) {
						if((t1 - RepartPci.date) > RepartPci.delai) printf("%s/ %s: delai entre lectures PCI = %lld us\n",DateHeure(),repart->nom,t1 - RepartPci.date);
					}
					RepartPci.date = t1;
					repart->acces_demandes++;
					IcaXferExec(repart->p.pci.port);
					repart->s.pci.en_cours = 1;
					if(log > 1) printf("          %s: relance du DMA #%lld\n",repart->nom,repart->acces_demandes);
					LectUsageInterface[repart->interf] += ((VerifTempsPasse? DateMicroSecs(): 0) - t1);
				} else if(log || alerte) printf("          %s: pile %s ([%d %d]), %s recopiee\n",
												repart->nom,(repart->top == 0)?"vide":"remplie",repart->bottom,repart->top,repart->s.pci.en_cours?"pas encore":"deja");
			}			
#endif /* ACCES_PCI_DIRECT */
			if(alerte) printf("          %s: alerte en cours\n",repart->nom);
			if(RepartNbActifs > 1) LectUsageInterfacesToutes += ((VerifTempsPasse? DateMicroSecs(): 0) - t2);
		}
#endif /* AVEC_PCI */
	}
	
retour:
	if(log > 1) {
		if(LectEnCours.rep >= 0) printf("          Fin de lecture sur la voie %s:#%d [bloc %lld], %s le status\n",
			Repart[LectEnCours.rep].nom,LectVoieRep,LectStampsLus+1,LectEnCours.is_status?"c'est":"ce n'est pas");
		else printf("          Fin de lecture sur la donnee #%d [bloc %lld] (%s le status)\n",
			LectEnCours.echantillon,LectStampsLus+1,LectEnCours.is_status?"c'est":"ce n'est pas");
	}
	if(LectErreur.code && PCIdemande) for(rep=0; rep<RepartNb; rep++) if((Repart[rep].famille == FAMILLE_CLUZEL) && Repart[rep].actif) {
		printf("%s/ Abandon du depilement code %04X\n",DateHeure(),(shex)LectErreur.code);
		printf("          %s: FIFO utilisee a %.2f %%%%\n",
			   Repart[rep].nom,100.0*(float)Repart[rep].depile/(float)Repart[rep].depilable);
		printf("          Niveau FIFO precedent: %.2f %%%%, %lld us avant\n",
			   100.0*(float)RepartPci.depilees/(float)Repart[rep].depilable,LectDepileAttente);
		RepartPci.depilees = Repart[rep].depile;
	}
#define RELANCE_DEPILE
#ifdef RELANCE_DEPILE
	if(IPactifs) {
		gettimeofday(&LectDateRun,0);
		if(LectDateRun.tv_sec >= LectDateRelance) {
			int rep;
			/* re-demande l'envoi des donnees */
			IPactifs = 0;
			for(rep=0; rep<RepartNb; rep++) if((Repart[rep].interf == INTERF_IP) && Repart[rep].actif) {
				RepartIpContinue(&(Repart[rep]),Repart[rep].s.ip.relance,1); IPactifs++;
			}
			if(IPactifs) {
				struct tm prochain;
				LectDateRelance = LectDateRun.tv_sec + LectIntervRelance;
				memcpy(&prochain,localtime(((time_t *)(&LectDateRelance))),sizeof(struct tm));
				/* verifie si les repartiteurs ont bien cause */
				if(!LectModeStatus) printf("%s/ %lld octets lus",DateHeure(),RepartIpOctetsLus);
				else printf("%s/ %lld trames traitees",DateHeure(),RepartIp.traitees);
				printf(", %d repartiteur%s IP relance%s jusqu'a %02d:%02d:%02d\n",
					Accord2s(IPactifs),prochain.tm_hour,prochain.tm_min,prochain.tm_sec);
			}
		}
	}
#endif /* RELANCE_DEPILE */
	// if(LectErreur.code) LectSend(CLUZEL_CODE_IDENT,0x0F,CLUZEL_IMMEDIAT,CLUZEL_STOP); /* au plus tot pour trigger l'analyseur et preserver la FIFO */
	LectDepileTfin = t1 = DateMicroSecs(); // (VerifTempsPasse? DateMicroSecs(): 0);
	LectDepileDuree = t1 - t0;
	TempsTotalLect += LectDepileDuree;
	TempsTempoLect += LectDepileDuree;
	cpu_utilise = VerifConsoCpu? clock() - c0: 0;
	CpuTempoLect += cpu_utilise;
	CpuTotalLect += cpu_utilise;
	LectDepileEnCours = 0;
	return;
}
/* ========================================================================== */
void Lect1Boucle() {
#ifdef NOUVEAU
	int64 depuis_depile;

	if(LectBoucleExterne && !Acquis[AcquisLocale].etat.active) {
		LectExecTermine();
		if(LectAcqEnCours == LECT_PRGM_ELEM) LectAcqElemTermine();
		if(LectAcqEnCours == LECT_PRGM_STD) LectAcqStdTermine();
		return;
	}

	if(LectDepileSousIt) {
#ifdef CODE_WARRIOR_VSN
		if(SynchroD2lues < LectNextTrmt) TimerMilliSleep(LectDepileWait/1000);
#else
		if(SynchroD2lues < LectNextTrmt) usleep(LectDepileWait);
#endif
		if(!LectModeStatus && !VoiesLocalesNb) SynchroD2lues++; /* ca nous fait des synchros a 10 ms tant que LectNextTrmt est bien gere */
	} else {
#ifdef A_TRAVAILLER
		if(LectPid == -1) LectDepileDonnees();
		else if(LectPid) {
			int stat;
			if(wait4(LectPid,&stat,WNOHANG,0) == LectPid) LectPid = 0;
		}
		if(!LectPid) {
			--LectDepileNb;
			LectPid = fork();
			if(LectPid) { if(LectPid == -1) LectDepileDonnees(); else usleep(LectDepileWait); }
			else if(!LectDepileEnCours) { LectDepileDonnees(); _exit(0); }
		}
#else
		LectDepileDonnees();
#endif
	}
	//			if(SettingsSynchroMax && (SynchroD2lues >= SettingsSynchroMax)) { Acquis[AcquisLocale].etat.active = 0; break; }
	if(LectDepileInhibe) {
		if(LectDansTraiteStocke > 1)
			printf("%s! L'execution de TraiteStocke dure plus de %.3f secondes\n",DateHeure(),(float)(LectDansTraiteStocke*SettingsReadoutPeriod)/1000.0);
		if(LectDansActionUtilisateur > 1)
			printf("%s! L'execution de ActionUtilisateur dure plus de %.3f secondes\n",DateHeure(),(float)(LectDansActionUtilisateur*SettingsReadoutPeriod)/1000.0);
		if(LectDansDisplay > 1)
			printf("%s! L'execution de Display dure plus de %.3f secondes\n",DateHeure(),(float)(LectDansDisplay*SettingsReadoutPeriod)/1000.0);
		LectErreur.code = LECT_OVERFLO;
	}
#ifndef CODE_WARRIOR_VSN
	gettimeofday(&LectDateRun,0);
	depuis_depile = ((int64)LectDateRun.tv_sec * 1000000) + (int64)LectDateRun.tv_usec - LectDepileTfin;
	if(LectDepileSousIt && (depuis_depile >= LectEntreDepile)) {
		LectEntreDepile *= 2;
		if(LectEntreDepile < (DureeTampons * 1000)) {
			printf("%s/ Delai depuis derniere lecture anormal: %lld us, relancee avec une tolerance de %lld us\n",DateHeure(),depuis_depile,LectEntreDepile);
			LectItRecue();
		} else {
			printf("%s/ Delai depuis derniere lecture anormal: %lld us, nouvelle session demandee\n",DateHeure(),depuis_depile);
			LectErreur.code = LECT_SYNC; Acquis[AcquisLocale].etat.active = 0;
		}
	}
	if(entretien_bolo) {
		char a_suivre; int b;
		if(LectDateRun.tv_sec >= LectDateRegulBolo) {
			LectDateRegulBolo = LectDateRun.tv_sec + SettingsDLUdetec;
			for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire && (Bolo[bolo].regul.bloc >= 0)) {
				if(!LectEntretienEnCours) {
					printf("%s/ ============================== Debut d'entretien des detecteurs ==============================\n",DateHeure());
					LectEntretienEnCours = 1;
					if(NomEntretienStart[0] && strcmp(NomEntretienStart,"neant")) ScriptExec(FichierEntretienStart,NomEntretienStart,"");
					if(RegenEnCours) LectRegenAffiche(0,0);
				}
				printf("%s/ Detecteur %s: activation du script d'entretien '%s'\n",DateHeure(),Bolo[bolo].nom,Bolo[bolo].regul.script);
				Bolo[bolo].exec.inst = DetecScriptsLibrairie.bloc[Bolo[bolo].regul.bloc].premiere;
				ScriptBoucleVide(&(Bolo[bolo].exec.boucle)); Bolo[bolo].exec.date = 0; /* pour dire "prochaine instruction: de suite" */
			}
			// printf("%s/ Prochaine maintenance detecteurs a %ld secondes\n",DateHeure(),LectDateRegulBolo);
		}
		ScriptKill = 0; a_suivre = 0;
		for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire && ((b = Bolo[bolo].regul.bloc) >= 0)) {
			short derniere;
			derniere = DetecScriptsLibrairie.bloc[b].derniere;
			if(Bolo[bolo].exec.inst < derniere) {
				if(Bolo[bolo].exec.date <= LectDateRun.tv_sec) {
					short secs; //- char prefixe[80];
					// printf("%s/ Detecteur %s: execution des instructions [%d, %d[\n",DateHeure(),Bolo[bolo].nom,Bolo[bolo].exec.inst,derniere);
					while(Bolo[bolo].exec.inst < derniere) {
						ScriptExecBatch(DetecScriptsLibrairie.action,HW_DETEC,(void *)&(Bolo[bolo]),&(Bolo[bolo].exec),derniere,&secs,"");
						if(secs) {
							Bolo[bolo].exec.date = LectDateRun.tv_sec + secs;
							if(LectDateRegulBolo < (Bolo[bolo].exec.date + SettingsDLUdetec)) LectDateRegulBolo = Bolo[bolo].exec.date + SettingsDLUdetec;
							// printf("%s/ Detecteur %s: prochaine execution a %ld s\n",DateHeure(),Bolo[bolo].nom,Bolo[bolo].exec.date);
							break;
						}
					}
					NumeriseurChargeFin(ScriptMarge());
					if(Bolo[bolo].exec.inst >= derniere) printf("%s/ Detecteur %s: script d'entretien termine\n",DateHeure(),Bolo[bolo].nom);
				}
				if(Bolo[bolo].exec.inst < derniere) a_suivre = 1;
			}
		}
		ScriptKill = 0;
		if(LectEntretienEnCours && !a_suivre) {
			printf("%s/ ==============================  Fin  d'entretien des detecteurs ==============================\n",DateHeure());
			LectEntretienEnCours = 0;
			if(NomEntretienStop[0] && strcmp(NomEntretienStop,"neant")) ScriptExec(FichierEntretienStop,NomEntretienStop,"");
			if(RegenEnCours) LectRegenTermine(0);
		}
	}
#ifdef RELANCE_EXEC
	if(IPactifs) {
		if(LectDateRun.tv_sec >= LectDateRelance) {
			int rep;
			/* re-demande l'envoi des donnees */
			IPactifs = 0;
			for(rep=0; rep<RepartNb; rep++) if((Repart[rep].interf == INTERF_IP) && Repart[rep].actif) {
				RepartIpContinue(&(Repart[rep]),Repart[rep].s.ip.relance,1); IPactifs++;
			}
			if(IPactifs) {
				struct tm prochain;
				LectDateRelance = LectDateRun.tv_sec + LectIntervRelance;
				memcpy(&prochain,localtime(((time_t *)(&LectDateRelance))),sizeof(struct tm));
				/* verifie si les repartiteurs ont bien cause */
				if(!LectModeStatus) {
					if(ip_avant == RepartIpOctetsLus) LectDeclencheErreur(_ORIGINE_,1,LECT_EMPTY);
					ip_avant = RepartIpOctetsLus;
					printf("%s/ %lld octets lus, %d repartiteur%s UDP relance%s jusqu'a %02d:%02d:%02d\n",
						   DateHeure(),RepartIpOctetsLus,Accord2s(IPactifs),
						   prochain.tm_hour,prochain.tm_min,prochain.tm_sec);
				} else {
					if(ip_avant == RepartIp.traitees) LectDeclencheErreur(_ORIGINE_,2,LECT_EMPTY);
					ip_avant = RepartIp.traitees;
					printf("%s/ %lld trames traitees, %d repartiteur%s UDP relance%s jusqu'a %02d:%02d:%02d\n",
						   DateHeure(),RepartIp.traitees,Accord2s(IPactifs),
						   prochain.tm_hour,prochain.tm_min,prochain.tm_sec);
				}
			}
		}
	}
#endif /* RELANCE_EXEC */
#endif /* !CODE_WARRIOR_VSN */
	if(SambaMaitre || (SambaEchantillonLngr == 0)) SynchroD2lues = (DateMicroSecs() - LectT0Run) / SynchroD2_us;
	// if((LectDepileTestees != LectDepileNb) && (SynchroD2traitees != SynchroD2lues)) printf("%s/ Maintenant %d lecture%s au lieu de %d, toujours %lld synchro%s lue%s, et pas de traitement a faire\n",DateHeure(),Accord1s(LectDepileNb),LectDepileTestees,Accord2s(SynchroD2traitees));
	if(SynchroD2traitees != SynchroD2lues) {
		SynchroD2traitees = SynchroD2lues;
		if(!LectTrmtSousIt) LectTraiteStocke();
		LectActionUtilisateur(); /* appelle LectDisplay si (SynchroD2lues >= LectNextDisp) */
	} else OpiumUserAction();
	LectDepileTestees = LectDepileNb;
#ifdef PAS_D_ERREUR_PERMISE
	if(LectErreursNb) printf("%s! Trouve %08X, attendu xxxx%04hX @bloc %lld[voie %d, rep %d]\n",DateHeure(),
							 LectEnCours.valeur,LectTypePrevu,LectStampsLus,LectVoieRep,LectEnCours.rep);
#else
	if(LectErreur1) {
		printf("%s! %d ERREUR%S sur le type de donnee! (entre le point %d,%05d0 (%04X) et le point %d,%05d0)\n",
			   DateHeure(),Accord1S(LectErreursNb),
			   (int)(LectMarque1/100000),Modulo(LectMarque1,100000),(int)LectErreur1 & 0xFFFF,
			   (int)(LectStampsLus/100000),Modulo(LectStampsLus,100000));
		if(LectErreursNb > MAX_ERREURS) {
			printf("%s/ Le maximum d'erreurs par lecture (%d) a ete depasse...\n",DateHeure(),MAX_ERREURS);
			LectDeclencheErreur(_ORIGINE_,3,LECT_UNEXPEC);
		} else LectErreur1 = 0;
	}
#endif
	/* A REMETTRE DES QUE POSSIBLE */
#ifdef OBSOLETE
	if(LectErreur.code == LECT_EOF) {
		LectArchFerme();
		LectTrancheRelue++;
		LectErreur.code = 0; Acquis[AcquisLocale].etat.active = 1;
		LectArchPrepare(0);
		if(!LectErreur.code) DetecteursLit(FichierPrefDetecteurs,LectSurFichier);
	}
#endif
	/* autres actions periodiques */
	if(BancEnTest && BancUrgence) Acquis[AcquisLocale].etat.active = 0;
	else if((LectRetard.stop.mode == LECT_RETARD_PREVU) || (LectRetard.stop.mode == LECT_RETARD_CALDR)) {
		maintenant = DateLong();
		if(maintenant >= LectRetard.stop.date) {
			Acquis[AcquisLocale].etat.active = 0;
			LectErreur.code = LECT_PREVU;
		} else {
			if(avant != LectDateRun.tv_sec) {
				secs = (int)(DateLongToSecondes(LectRetard.stop.date) - DateLongToSecondes(maintenant)); S_usPrint(delai,secs,0);
				sprintf(LectInfo[1],L_("Fin du run prevue dans %s"),delai);
				if(OpiumAlEcran(pLectRegen)) PanelRefreshVars(pLectRegen);
				if(pair) MenuItemAllume(mLectArret,1,L_("Prevu   "),GRF_RGB_ORANGE);
				else MenuItemAllume(mLectArret,1,L_("Stopper "),GRF_RGB_YELLOW);
				OpiumUserAction();
				pair = 1 - pair;
				avant = LectDateRun.tv_sec;
			}
		}
	}
	if(SambaInfos && !SambaInfos->en_cours) LectStop();
#endif /* NOUVEAU */
}
/* ========================================================================== */
static void LectItRecue() {
	if(!LectBoucleExterne) TimerContinue(LectTaskReadout);
	if(LectDepileSousIt) {
		// if(LectDepileInhibe > 0) LectDepileInhibe--; else LectDepileDonnees();
		if(!LectDepileInhibe) {
			if(SambaMaitre) LectDepileSat(); else LectDepileDonnees();
		}
	}
}
/* ========================================================================== 
static void LecTraiteFromIt() {
	
	TimerContinue(LectTaskReadout);
	if(LectTrmtSousIt && (SynchroD2lues >= LectNextTrmt)) {
		if(LectTrmtEnCours) LectDeclencheErreur(_ORIGINE_,33,LECT_OVERFLO);
		else {
			LectTrmtEnCours = 1;
			LectTraiteStocke();
			LectTrmtEnCours = 0;
			LectNextTrmt += LectIntervTrmt;
		}
	}
}
 ========================================================================== */

#pragma mark ---- Commandes executives ----
#define MAX_ERREURS 0

/* ========================================================================== */
static TypeADU LectExec(NUMER_MODE mode) {
/* /docFuncBeg {LectExec}
   	/docFuncReturn  {0 si OK, code d'erreur sinon (voir LectAcqStd)}
   	/docFuncSubject {Tache de lecture, avec retour seulement si erreur ou Acquis[AcquisLocale].etat.active=0}
*/
	char boostee,diese,log; FILE *f;
	char entretien_bolo,ok; int n,m;
	int bolo;
	int64 SynchroD2traitees,min_restart,ip_avant; int64 depuis_depile;
	int64 maintenant; char pair; int avant,secs; char delai[DATE_MAX];
	TypeADU erreur_acq;


/*
 * Initialisations
 * ---------------
 */
	if(SettingsMultitasks == 1) {
		if(SambaPartage && (SambaPartageId == -1)) {
			printf("* Liberation de la variable globale SambaPartage[%d] en 0x%08llX\n",SambaPartageDim,(IntAdrs)SambaPartage);
			free(SambaPartage); SambaPartage = 0;
		}
		if(!SambaPartage) {
			SambaPartageId = shmget(IPC_PRIVATE,SambaPartageDim,0644|IPC_CREAT|IPC_EXCL);
			/* printf("Identifieur cree: %d\n",SambaPartageId); */
			printf("* shmget(%d octets) a rendu SambaPartageId=%08X (errno=%d)\n",SambaPartageDim,SambaPartageId,errno);
			if(SambaPartageId != -1) SambaPartage = (TypeSambaPartagee *)shmat(SambaPartageId,0,SHM_RND);
		}
	} else {
		if(SambaPartage && (SambaPartageId != -1)) {
			printf("* Liberation de la variable partagee SambaPartage[%d] en 0x%08llX\n",SambaPartageDim,(IntAdrs)SambaPartage);
			int i; i = 256; while((shmdt(SambaPartage) != -1) && i) i-- ; SambaPartage = 0; SambaPartageId = -1;
		}
		if(!SambaPartage) SambaPartage = (TypeSambaPartagee *)malloc(SambaPartageDim);
	}
	if(SambaPartage) printf("* Variable %s SambaPartage[%d] allouee en 0x%08llX\n",(SambaPartageId == -1)? "globale": "partagee",SambaPartageDim,(IntAdrs)SambaPartage);
	else {
		printf("* Allocation de la variable %s SambaPartage[%d] en erreur: %s\n",(SambaPartageId == -1)? "globale": "partagee",SambaPartageDim,strerror(errno));
		printf("  => Lecture sous interruption en mode v4\n");
		SettingsMultitasks = 4;
	}
    log = (LectureLog || LectLogComp);

	boostee = (SettingsMultitasks && !LectParBloc);
	if(boostee) {
		if((SettingsMultitasks <= 1)
		|| (LectCntl.LectMode == LECT_IDENT)
		|| (LectCntl.LectMode == LECT_COMP)
		|| LectSurFichier) LectDepileSousIt = 0;
		else LectDepileSousIt = 1;
	} else {
		LectDepileSousIt = 0; /* gcc.. */
		OpiumFail("La lecture en serie n'est plus programmee");
		return(LectMessageArret(_ORIGINE_,7,LECT_NFND));
	}
/*	LectDepileSousIt = 0;  pour mise au point */
	LectDepileSousIt = 1;
	LectTrmtSousIt = 0; // LectDepileSousIt;
	LectErreursNb = 0;
#ifndef PAS_D_ERREUR_PERMISE
	LectErreur1 = 0;
#endif
	LectArretePoliment = 0;
	sprintf(RunDateDebut,"%s a %s",DateCivile(),DateHeure());
	LectJournalDemarrage();
	printf("* Demarrage le %s\n",RunDateDebut);

/*
 * Enregistrements divers
 * ----------------------
 */
	if(Archive.enservice && EdbServeur[0] && strcmp(EdbServeur,"neant")) {
		f = fopen("RunInfo","r");
		if(f) {
			DbSendFile(EdbServeur,ArchiveId,f);
			if(DbStatus) printf("%s/ Base de Donnees renseignee (Id: %s, rev: %s)\n",DateHeure(),DbId,DbRev);
			else printf("%s/ ! Renseignement Base de Donnees en erreur (%s, raison: %s)\n",DateHeure(),DbErreur,DbRaison);
			fclose(f);
			remove("RunInfo");
		}
	}
	if(Archive.enservice && (LectCntl.LectMode == LECT_DONNEES) && (LectSession < 2)) {
		FILE *f;
		//- strcat(strcpy(FichierCatalogue,CtlgPath),"Catalogue");
		f = fopen(FichierCatalogue,"r");
		if(!f) {
			RepertoireCreeRacine(FichierCatalogue);
			f = fopen(FichierCatalogue,"w");
			if(f) fprintf(f,"# Date    heure   nom       mode  type      duree  taille(MB)  expo(Kg.j)   evts\n");
		} else {
			int car;
			fseek(f,-1,SEEK_END);
			car = fgetc(f);
			fclose(f);
			f = fopen(FichierCatalogue,"a");
			if(car != '\n') {
				printf("%s/ Fermeture de la derniere ligne du catalogue\n",DateHeure());
				fprintf(f," inconnue  inconnue    inconnue     --\n");
			}
		}
		if(f) {
			char *c; int i,j,k; char *type;
			LectCommentComplet[0] = '\0'; c = &(LectComment[0][0]);
			for(i=0; i<MAXCOMMENTNB; i++) if(LectComment[i][0] != '\0') {
				c = &(LectComment[i][0]);
				for(j=0; j<MAXCOMMENTLNGR-2; j++) {
					if((LectComment[i][j] != ' ') && (LectComment[i][j] != 0x09)) c = &(LectComment[i][j]);
				}
				c++; *c = '\0';
				if(i) strcat(strcat(LectCommentComplet," "),LectComment[i]);
				else strcpy(LectCommentComplet,LectComment[i]);
			}
			if((SettingsRunFamille == RUN_FAMILLE_BANC) && (RunTypeName >= 0)) {
				EnvirVarPrintVal(&(EnvirVar[RunTypeVar]),RunTypeName,TYPERUN_NOM);
				type = RunTypeName;
			} else {
				ArgKeyGetText(RunCategCles,LectCntl.RunCategNum,RunCategName,TYPERUN_NOM);
				type = RunCategName;
			}
			fprintf(f,"%8s %8s %8s %6s %-6s",DateJour(),DateHeure(),Acquis[AcquisLocale].etat.nom_run,
					(Trigger.demande == NEANT)? "stream": "event",type);
			for(k=0; k<ExportPackNb; k++) if((ExportPack[k].support_type == EXPORT_CATALOGUE) && (ExportPack[k].quand == EXPORT_RUN_DEBUT)) {
				if(!diese) { fprintf(f," # "); diese = 1; }
				ExporteInfos(f,k);
			}
			fflush(f);
			fclose(f);
			printf("%s/ Mise a jour du catalogue (%s)\n",DateHeure(),FichierCatalogue);
		} else printf("%s/ Mise a jour du catalogue impossible, fichier inaccessible:\n            '%s'\n",
					  DateHeure(),FichierCatalogue);
		if(SettingsChargeBolos && !LectModeSpectresAuto && !RegenEnCours) DetecteurChargeTous(log?"         ":0);
	};
	if(TrmtRegulActive && ArchSauve[EVTS]) {
		int secs,usecs;
#ifndef CODE_WARRIOR_VSN
		gettimeofday(&LectDateRun,0);
		ArchT0sec = LectDateRun.tv_sec;
		ArchT0msec = LectDateRun.tv_usec;
#endif /* CODE_WARRIOR_VSN */
		SambaTempsEchantillon(DernierPointTraite,ArchT0sec,ArchT0msec,&secs,&usecs);					
		ArchiveSeuils(secs);
	}

/*
 * Depart sur une synchro (D2 ou D3)
 * ---------------------------------
 */
	boostee = 1; // c'est devenu obligatoire
	printf("%s/ Lecture classique %s\n",DateHeure(),LectDepileSousIt?"sous interruption":"en serie");

relance:
	min_restart = 100; // 5000; // 5s
	Acquis[AcquisLocale].etat.active = LectSynchro(mode);
	if(LectRunTime) LectRunStop = LectDateRun.tv_sec + LectRunTime;
	if(SambaInfos) {
		strncpy(SambaInfos->origine,__func__,SAMBA_SHR_FCTN);
		sprintf(SambaInfos->validite,"%s %s",DateCivile(),DateHeure());
		SambaInfos->en_cours = Acquis[AcquisLocale].etat.active;
	}
	if(!Acquis[AcquisLocale].etat.active) {
		LectAutreSequence = 0;
		return(LectMessageArret(_ORIGINE_,1,LECT_NFND));
	}
	Acquis[AcquisLocale].etat.status = SATELLITE_ACTIF;
	for(n=0; n<OscilloNb; n++) {
		OscilloReglage[n]->acq = Acquis[AcquisLocale].etat.active;
		if(OscilloReglage[n]->iAcq) {
			if(OpiumDisplayed((OscilloReglage[n]->iAcq)->cdr)) OpiumRefresh((OscilloReglage[n]->iAcq)->cdr);
		}
	}
	// on rappelle que LectEntretien = (BolosAentretenir && SettingsDLUactive && SettingsDLUdetec); dans LectFixeMode()
	entretien_bolo = (LectEntretien && (LectAcqLanceur == LECT_FROM_STD) && !LectModeSpectresAuto);
//?	ReglagesEtatDemande = 1; if(OpiumDisplayed(iReglagesEtatRun->cdr)) OpiumRefresh(iReglagesEtatRun->cdr);
//-	printf("%s/ Debut a T0 = %20lld us\n",DateHeure(),LectT0Run);

/*
 * Suite de la FIFO: cas de la lecture en parallele
 * ------------------------------------------------
 */
#ifdef DEBUG_DEPILE
	LectDepileStockes = 0;
#endif
	avant = 0; pair = 0;
	if(boostee) {
		BoloTrigge[0] = VoieTriggee[0] = TempsTrigger[0] = '\0'; MonitEvtAmpl = 0.0; MonitEvtMontee = 0.0;
		LectCntl.MonitEvtNum = 0;
		SynchroD2traitees = 0;
		LectDeclencheErreur(_ORIGINE_,0,0); 
		LectErreur.nom[0] = '\0';
		LectErreursNb = 0;
		LectDepileEnCours = 0;
		LectTrmtEnCours = 0;
		LectDepileDuree = 0;
		LectDepileIntervMax = 0;
		LectEntreDepile = (int64)SettingsReadoutPeriod * 10000;
		LectDelaisTropCourt = LectDelaisTropLong = 0;
		RepartDataErreurs = 0;
		RepartIpErreurs = 0;
		LectEpisodeAgite = 0;
		LectRelancePrevue = 0;
//1		if(mode == NUMER_MODE_ACQUIS) MenuItemAllume(mLectDemarrage,1,"Stopper ",GRF_RGB_GREEN);
//1		else MenuItemAllume(mLectDemarrage,1,"Stopper ",GRF_RGB_RED);
//2		if(mode == NUMER_MODE_ACQUIS) MenuItemAllume(mLectArret,1,"Stopper ",GRF_RGB_YELLOW);
//2		else MenuItemAllume(mLectArret,1,"Stopper ",GRF_RGB_RED);
		LectDepileInhibe = 0;
		if(SambaPartage) SambaPartage->nb_depile = 0;
		LectDansTraiteStocke = LectDansDisplay = LectDansActionUtilisateur = 0;
		ip_avant = RepartIpOctetsLus;
		LectDerniereNonVide = LectDepileTfin = LectT0Run;
		/* TimerTrig(LectTaskReadout); pour une lecture des que possible */

	#ifndef ACCES_PCI_DIRECT
		if(LectDepileSousIt) printf("%s/ Lecture activee, toutes les %g ms\n",DateHeure(),(float)LectEntreDepile/1000.0);
	#endif
		if(LectBoucleExterne) return(0);
		if(LectDepileSousIt) printf("%s/ Surveillance toutes les %g us\n",DateHeure(),LectDepileWait);
	#ifdef NOUVEAU
		while(Acquis[AcquisLocale].etat.active) Lect1Boucle();
	#else
		while(Acquis[AcquisLocale].etat.active) {
			if(LectDepileSousIt) {
			#ifdef CODE_WARRIOR_VSN
				if(SynchroD2lues < LectNextTrmt) TimerMilliSleep(LectDepileWait/1000);
			#else
				if(SynchroD2lues < LectNextTrmt) usleep(LectDepileWait);
			#endif
				if(!LectModeStatus && !VoiesLocalesNb) SynchroD2lues++;
				/* ca nous fait des synchros a 10 ms tant que LectNextTrmt est bien gere */
			} else {
			#ifdef A_TRAVAILLER
				if(LectPid == -1) LectDepileDonnees();
				else if(LectPid) {
					int stat;
					if(wait4(LectPid,&stat,WNOHANG,0) == LectPid) LectPid = 0;
				}
				if(!LectPid) {
					--LectDepileNb;
					LectPid = fork();
					if(LectPid) { if(LectPid == -1) LectDepileDonnees(); else usleep(LectDepileWait); }
					else if(!LectDepileEnCours) { LectDepileDonnees(); _exit(0); }
				}
			#else
				LectDepileDonnees();
			#endif
			}
			//			if(SettingsSynchroMax && (SynchroD2lues >= SettingsSynchroMax)) { Acquis[AcquisLocale].etat.active = 0; break; }
			if(LectDepileInhibe) {
				if(LectDansTraiteStocke > 1)
					printf("%s! L'execution de TraiteStocke dure plus de %.3f secondes\n",DateHeure(),(float)(LectDansTraiteStocke*SettingsReadoutPeriod)/1000.0);
				if(LectDansActionUtilisateur > 1)
					printf("%s! L'execution de ActionUtilisateur dure plus de %.3f secondes\n",DateHeure(),(float)(LectDansActionUtilisateur*SettingsReadoutPeriod)/1000.0);
				if(LectDansDisplay > 1)
					printf("%s! L'execution de Display dure plus de %.3f secondes\n",DateHeure(),(float)(LectDansDisplay*SettingsReadoutPeriod)/1000.0);
				LectErreur.code = LECT_OVERFLO;
			}
#ifndef CODE_WARRIOR_VSN
			gettimeofday(&LectDateRun,0);
			depuis_depile = ((int64)LectDateRun.tv_sec * 1000000) + (int64)LectDateRun.tv_usec - LectDepileTfin;
			if(LectDepileSousIt && (depuis_depile >= LectEntreDepile)) {
				LectEntreDepile *= 2;
				if(LectEntreDepile < (DureeTampons * 1000)) {
					printf("%s/ Delai depuis derniere lecture anormal: %lld us, relancee avec une tolerance de %lld us\n",DateHeure(),depuis_depile,LectEntreDepile);
					LectItRecue();
				} else {
					printf("%s/ Delai depuis derniere lecture anormal: %lld us, nouvelle session demandee\n",DateHeure(),depuis_depile);
					LectErreur.code = LECT_SYNC; Acquis[AcquisLocale].etat.active = 0;
				}
			}
			if(entretien_bolo) {
				char a_suivre; int b;
				if(LectDateRun.tv_sec >= LectDateRegulBolo) {
					LectDateRegulBolo = LectDateRun.tv_sec + SettingsDLUdetec;
					for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire && (Bolo[bolo].regul.bloc >= 0)) {
						if(!LectEntretienEnCours) {
							printf("%s/ ============================== Debut d'entretien des detecteurs ==============================\n",DateHeure());
							LectEntretienEnCours = 1;
							if(NomEntretienStart[0] && strcmp(NomEntretienStart,"neant")) ScriptExec(FichierEntretienStart,NomEntretienStart,"");
							if(RegenEnCours) LectRegenAffiche(0,0);
						}
						printf("%s/ Detecteur %s: activation du script d'entretien '%s'\n",DateHeure(),Bolo[bolo].nom,Bolo[bolo].regul.script);
						Bolo[bolo].exec.inst = DetecScriptsLibrairie.bloc[Bolo[bolo].regul.bloc].premiere;
						ScriptBoucleVide(&(Bolo[bolo].exec.boucle)); Bolo[bolo].exec.date = 0; /* pour dire "prochaine instruction: de suite" */
					}
					// printf("%s/ Prochaine maintenance detecteurs a %ld secondes\n",DateHeure(),LectDateRegulBolo);
				}
				ScriptKill = 0; a_suivre = 0;
				for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire && ((b = Bolo[bolo].regul.bloc) >= 0)) {
					short derniere;
					derniere = DetecScriptsLibrairie.bloc[b].derniere;
					if(Bolo[bolo].exec.inst < derniere) {
						if(Bolo[bolo].exec.date <= LectDateRun.tv_sec) {
							short secs; //- char prefixe[80];
							// printf("%s/ Detecteur %s: execution des instructions [%d, %d[\n",DateHeure(),Bolo[bolo].nom,Bolo[bolo].exec.inst,derniere);
							while(Bolo[bolo].exec.inst < derniere) {
								ScriptExecBatch(DetecScriptsLibrairie.action,HW_DETEC,(void *)&(Bolo[bolo]),&(Bolo[bolo].exec),derniere,&secs,"");
								if(secs) {
									Bolo[bolo].exec.date = LectDateRun.tv_sec + secs;
									if(LectDateRegulBolo < (Bolo[bolo].exec.date + SettingsDLUdetec)) LectDateRegulBolo = Bolo[bolo].exec.date + SettingsDLUdetec;
									// printf("%s/ Detecteur %s: prochaine execution a %ld s\n",DateHeure(),Bolo[bolo].nom,Bolo[bolo].exec.date);
									break;
								}
							}
							NumeriseurChargeFin(ScriptMarge());
							if(Bolo[bolo].exec.inst >= derniere) printf("%s/ Detecteur %s: script d'entretien termine\n",DateHeure(),Bolo[bolo].nom);
						}
						if(Bolo[bolo].exec.inst < derniere) a_suivre = 1;
					}
				}
				ScriptKill = 0;
				if(LectEntretienEnCours && !a_suivre) {
					printf("%s/ ==============================  Fin  d'entretien des detecteurs ==============================\n",DateHeure());
					LectEntretienEnCours = 0;
					if(NomEntretienStop[0] && strcmp(NomEntretienStop,"neant")) ScriptExec(FichierEntretienStop,NomEntretienStop,"");
					if(RegenEnCours) LectRegenTermine(0);
				}
			}
#ifdef RELANCE_EXEC
			if(IPactifs) {
				if(LectDateRun.tv_sec >= LectDateRelance) {
					int rep;
					/* re-demande l'envoi des donnees */
					IPactifs = 0;
					for(rep=0; rep<RepartNb; rep++) if((Repart[rep].interf == INTERF_IP) && Repart[rep].actif) {
						RepartIpContinue(&(Repart[rep]),Repart[rep].s.ip.relance,1); IPactifs++;
					}
					if(IPactifs) {
						struct tm prochain;
						LectDateRelance = LectDateRun.tv_sec + LectIntervRelance;
						memcpy(&prochain,localtime(((time_t *)(&LectDateRelance))),sizeof(struct tm));
						/* verifie si les repartiteurs ont bien cause */
						if(!LectModeStatus) {
							if(ip_avant == RepartIpOctetsLus) LectDeclencheErreur(_ORIGINE_,1,LECT_EMPTY);
							ip_avant = RepartIpOctetsLus;
							printf("%s/ %lld octets lus, %d repartiteur%s UDP relance%s jusqu'a %02d:%02d:%02d\n",
								   DateHeure(),RepartIpOctetsLus,Accord2s(IPactifs),
								   prochain.tm_hour,prochain.tm_min,prochain.tm_sec);
						} else {
							if(ip_avant == RepartIp.traitees) LectDeclencheErreur(_ORIGINE_,2,LECT_EMPTY);
							ip_avant = RepartIp.traitees;
							printf("%s/ %lld trames traitees, %d repartiteur%s UDP relance%s jusqu'a %02d:%02d:%02d\n",
								   DateHeure(),RepartIp.traitees,Accord2s(IPactifs),
								   prochain.tm_hour,prochain.tm_min,prochain.tm_sec);
						}
					}
				}
			}
#endif /* RELANCE_EXEC */
#endif /* !CODE_WARRIOR_VSN */
			if(SambaMaitre || (SambaEchantillonLngr == 0)) SynchroD2lues = (DateMicroSecs() - LectT0Run) / SynchroD2_us;
			// if((LectDepileTestees != LectDepileNb) && (SynchroD2traitees != SynchroD2lues)) printf("%s/ Maintenant %d lecture%s au lieu de %d, toujours %lld synchro%s lue%s, et pas de traitement a faire\n",DateHeure(),Accord1s(LectDepileNb),LectDepileTestees,Accord2s(SynchroD2traitees));
			if(SynchroD2traitees != SynchroD2lues) {
				SynchroD2traitees = SynchroD2lues;
				if(!LectTrmtSousIt) LectTraiteStocke();
				LectActionUtilisateur(); /* appelle LectDisplay si (SynchroD2lues >= LectNextDisp) */
			} else OpiumUserAction();
			LectDepileTestees = LectDepileNb;
#ifdef PAS_D_ERREUR_PERMISE
			if(LectErreursNb) printf("%s! Trouve %08X, attendu xxxx%04hX @bloc %lld[voie %d, rep %d]\n",DateHeure(),
									 LectEnCours.valeur,LectTypePrevu,LectStampsLus,LectVoieRep,LectEnCours.rep);
#else
			if(LectErreur1) {
				printf("%s! %d ERREUR%S sur le type de donnee! (entre le point %d,%05d0 (%04X) et le point %d,%05d0)\n",
					   DateHeure(),Accord1S(LectErreursNb),
					   (int)(LectMarque1/100000),Modulo(LectMarque1,100000),(int)LectErreur1 & 0xFFFF,
					   (int)(LectStampsLus/100000),Modulo(LectStampsLus,100000));
				if(LectErreursNb > MAX_ERREURS) {
					printf("%s/ Le maximum d'erreurs par lecture (%d) a ete depasse...\n",DateHeure(),MAX_ERREURS);
					LectDeclencheErreur(_ORIGINE_,3,LECT_UNEXPEC);
				} else LectErreur1 = 0;
			}
#endif
			/* A REMETTRE DES QUE POSSIBLE */
#ifdef OBSOLETE
			if(LectErreur.code == LECT_EOF) {
				LectArchFerme();
				LectTrancheRelue++;
				LectErreur.code = 0; Acquis[AcquisLocale].etat.active = 1;
				LectArchPrepare(0);
				if(!LectErreur.code) DetecteursLit(FichierPrefDetecteurs,LectSurFichier);
			}
#endif
			/* autres actions periodiques */
			if(BancEnTest && BancUrgence) Acquis[AcquisLocale].etat.active = 0;
			else if(LectRunTime && (LectRunStop < LectDateRun.tv_sec)) Acquis[AcquisLocale].etat.active = 0;
			else if((LectRetard.stop.mode == LECT_RETARD_PREVU) || (LectRetard.stop.mode == LECT_RETARD_CALDR)) {
				maintenant = DateLong();
				if(maintenant >= LectRetard.stop.date) {
					Acquis[AcquisLocale].etat.active = 0;
					LectErreur.code = LECT_PREVU;
				} else {
					if(avant != LectDateRun.tv_sec) {
						secs = (int)(DateLongToSecondes(LectRetard.stop.date) - DateLongToSecondes(maintenant)); S_usPrint(delai,secs,0);
						sprintf(LectInfo[1],L_("Fin du run prevue dans %s"),delai);
						if(OpiumAlEcran(pLectRegen)) PanelRefreshVars(pLectRegen);
						if(pair) MenuItemAllume(mLectArret,1,L_("Prevu   "),GRF_RGB_ORANGE);
						else MenuItemAllume(mLectArret,1,L_("Stopper "),GRF_RGB_YELLOW);
						OpiumUserAction();
						pair = 1 - pair;
						avant = LectDateRun.tv_sec;
					}
				}
			}
			if(SambaInfos && !SambaInfos->en_cours) LectStop();
		}
	#endif
		printf("%s/ Surveillance terminee\n",DateHeure());

		if(SambaInfos) {
			strncpy(SambaInfos->origine,__func__,SAMBA_SHR_FCTN);
			sprintf(SambaInfos->validite,"%s %s",DateCivile(),DateHeure());
			SambaInfos->en_cours = Acquis[AcquisLocale].etat.active;
		}
		if(LectErreur.code == LECT_ARRETE) LectRelancePrevue = 1; else
		LectRelancePrevue = (Archive.enservice
			&& (((shex)LectErreur.code == LECT_UNEXPEC)
			||  ((shex)LectErreur.code == CLUZEL_MARQUE_ECHANT)
			||  ((shex)LectErreur.code == CLUZEL_MARQUE_NEANT) 
			||  ((shex)LectErreur.code == LECT_SYNC) 
			||  ((shex)LectErreur.code == LECT_EMPTY) 
			||  ((shex)LectErreur.code == LECT_ARRETE) 
			||  ((shex)LectErreur.code == LECT_TOOMUCH) 
			||  ((shex)LectErreur.code == LECT_OVERFLO)) 
			&& (SynchroD2lues > min_restart));
		erreur_acq = LectMessageArret(_DECLENCHE_);
		if(LectErreur.code == LECT_PREVU) {
			DateLongPrint(delai,LectRetard.stop.date);
			printf("%s/ L'arret etait programme pour le %s\n",DateHeure(),delai);
		} else if(LectErreur.code) {
			if(LectEnCours.rep >= 0) printf("%s! Arret de type %04X au mot #%d/%d: %08X\n",DateHeure(),(shex)LectErreur.code,Repart[LectEnCours.rep].bottom,Repart[LectEnCours.rep].top,LectEnCours.valeur);
			else printf("%s! Arret de type %04X a la donnee #%d: %08X\n",DateHeure(),(shex)LectErreur.code,LectEnCours.echantillon,LectEnCours.valeur);
			if(!LectSurFichier && PCIdemande) {
				/* CluzelEnvoieCommande(PCIedw[PCInum].port,CLUZEL_SSAD_IDENT,0x0F,CLUZEL_STOP,CLUZEL_IMMEDIAT); pour trigger l'analyseur */
				printf("%s/ La pile a ete vue %d fois demi-pleine et %d fois pleine\n",DateHeure(),RepartPci.halffull,RepartPci.fullfull);
			}
			#ifdef DEBUG_FILL
			if(LectErreur.code == LECT_INCOHER) printf("%s/ %lld valeurs non nulles et LectFill appele %lld fois\n",DateHeure(),LectNonNulLectFillNb);
			#endif
			if(LectErreur.code == LECT_ARRETE) {
				int rep; char buffer[80];
				for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif && (Repart[rep].top == Repart[rep].bottom)) {
					printf("%s!!! %s arrete, ",DateHeure(),Repart[rep].nom);
					/* option: tester si ping, l'UDP sera relance par Depile */
					sprintf(buffer,COMMANDE_PING,IP_CHAMPS(Repart[rep].adrs));
					if(system(buffer) != 0) {
						printf(" et l'adresse %d.%d.%d.%d:%d ne repond plus.\n",IP_CHAMPS(Repart[rep].adrs),Repart[rep].ecr.port);
						LectRelancePrevue = 0;
					} else printf(" mais'adresse %d.%d.%d.%d:%d repond toujours.\n",IP_CHAMPS(Repart[rep].adrs),Repart[rep].ecr.port);
				}
			}
		}
#ifndef CODE_WARRIOR_VSN
		gettimeofday(&LectDateRun,0);
#endif
		printf("%s/ Lecture des donnees terminee\n",DateHeure());
		printf("%s/ Declenchement laisse %s\n",DateHeure(),(Trigger.actif)? "actif": "suspendu");
		if(LectCntl.LectMode == LECT_IDENT) TrmtSurBuffer();
		LectDepileInhibe = 0;
		LectDepileSousIt = 0;
		LectTrmtSousIt = 0;
//		printf("%s/ Erreur %04X/%04X detectee\n",DateHeure(),LectErreur.code,CLUZEL_MARQUE_ECHANT);
		if(LectRelancePrevue && (LectRunTime == 0)) {
			int rep,attente,patiente; char opera_presente;
			if((SynchroD2traitees != SynchroD2lues) && !LectTrmtSousIt) LectTraiteStocke();
			LectTexec += (VerifTempsPasse? (DateMicroSecs() - LectT0Run): 0);
			LectTermineStocke(0,1);
			LectJournalLaFin(erreur_acq,1,0);
			n = Acquis[AcquisLocale].etat.evt_trouves;
			m = TousLesEvts.utilises;
			LectStampPrecedents += LectStampsLus;
			opera_presente = 0;
			for(rep=0; rep<RepartNb; rep++) {
				if(Repart[rep].arrete) Repart[rep].actif = 1;
				if(Repart[rep].actif && (Repart[rep].famille == FAMILLE_OPERA)) opera_presente = 1;
			}
			attente = (opera_presente && (LectErreur.code == LECT_ARRETE))? LectAttenteRebootOpera: LectAttenteErreur;
			printf("%s! Relance #%d pour %s, dans %d secondes\n",DateHeure(),LectSession,LectSrceTexte,attente+LectAttenteErreur);
			LectSession++; if(OpiumAlEcran(pReprises)) OpiumRefresh(pReprises->cdr);
			if(attente) { patiente = attente; while(patiente--) { SambaMicrosec(1000000); OpiumUserAction(); } }
			// faut voir: printf("%s/ Remise a zero de l'electronique\n",DateHeure()); SambaRepartRestart(0);
			LogAddFile(RunJournal);
			//? if(attente) { patiente = attente; while(patiente--) { SambaMicrosec(1000000); OpiumUserAction(); } }
			LectFixeMode(LECT_DONNEES,0);
			ok = LectConstruitTampons(0);
			if(ok) { LectRazPartiel(); ok = TrmtRAZ(0); /* LectBuffStatus(); */ }
			if(ok) {
				int fmt;
				Acquis[AcquisLocale].etat.evt_trouves = n;
				TousLesEvts.utilises = m;
				ArchiveDefini(log);
				for(fmt=0; fmt<ARCH_TYPEDATA; fmt++) ArchTrancheReste[fmt] = -1;
				LectTimeStamp0 = LectTimeStampN = 0;
				LectJournalDemarrage();
				goto relance;
			}
		}
		for(n=0; n<OscilloNb; n++) {
			OscilloReglage[n]->acq = Acquis[AcquisLocale].etat.active;
			if(OscilloReglage[n]->iAcq) {
				if(OpiumDisplayed((OscilloReglage[n]->iAcq)->cdr)) OpiumRefresh((OscilloReglage[n]->iAcq)->cdr);
			}
		}
		ReglagesEtatDemande = 0; if(OpiumDisplayed(iReglagesEtatRun->cdr)) OpiumRefresh(iReglagesEtatRun->cdr);
		NumeriseurEtatDemande = 0; if(OpiumDisplayed(bNumeriseurEtat)) OpiumRefresh(bNumeriseurEtat);
		// if(OpiumDisplayed(iNumeriseurEtatMaj->cdr)) OpiumRefresh(iNumeriseurEtatMaj->cdr);
		LectModeStatus = 0; ReglagesEtatPeutStopper = NumeriseurEtatPeutStopper = NumeriseurPeutStopper = 0;
		// if(LectSurFichier) LectArchRewind(); else 
		if(LectErreur.code) { LectAutreSequence = 0; if(SettingsSecuPolar) LectRegenLance(0); }
	#ifndef CODE_WARRIOR_VSN
		gettimeofday(&LectDateRun,0);
	#endif
		if(LectArretePoliment && LectStoppeRepart) RepartiteursInitTous(REPART_STOPPE,Echantillonnage,0);
		if(OpiumAlEcran(pRepartXmit)) PanelRefreshVars(pRepartXmit);
		LectTexec += (VerifTempsPasse? (DateMicroSecs() - LectT0Run): 0);
		if(OpiumAlEcran(iEcran)) OpiumClear(iEcran->cdr); // iEcran = 0; } // plante si efface prealablement "a la main"
		if(LectAutreSequence) return(0); else return(erreur_acq);

	} else {
	/*
	 * Suite de la FIFO: cas de la lecture en serie
	 * --------------------------------------------
	 */
		OpiumFail("La lecture en serie n'est plus programmee");
		return(LectMessageArret(_ORIGINE_,7,LECT_NFND));
	}
}
/* /docFuncEnd */
/* ========================================================================== */
void LectExecTermine() {
}
/* ========================================================================== */
int LectIdentImprime() {
	int vi,rep,rep_avant,l,bb,bolo,cap,voie,vsnid,type,tilt,paumes,changes,neufs,zarbi,decon,bof; char autre;
	TypeADU lu; TypeDetecteur *detectr; TypeNumeriseur *numeriseur;
	shex connecteur;
	char code_entree[8];
	
	printf(" ____________________________________________________________________________________\n");
	printf("|                               Numeriseurs identifies                               |\n");
	printf("|____________________________________________________________________________________|\n");
	printf("| repart.  |               numeriseur          | prise | pos. |                      |\n");
	printf("|   entree | fibre   type         #serie ident | exter | cryo |  detecteur           |\n");
	printf("|__________|______|______________|______|______|_______|______|______________________|\n");

	tilt = paumes = changes = neufs = decon = zarbi = bof = 0;
	rep_avant = -1;
	for(vi=0; vi<VoieIdentMax; vi++) if(SambaEchantillon[vi].voie == 0) {
		rep = SambaEchantillon[vi].rep; l = SambaEchantillon[vi].entree;
		if((Repart[rep].famille == FAMILLE_IPE) && ((l && !(l % IPE_FLT_SIZE)) || ((rep_avant >= 0) && (rep != rep_avant)))) {
			printf("|__________|______|______________|______|______|_______|______|______________________|\n");
		}
		rep_avant = rep;
		RepartiteurCodeEntree(&(Repart[rep]),l,code_entree);
		printf("| %-4s %3s |",Repart[rep].code_rep,code_entree);
		numeriseur = (TypeNumeriseur *)(Repart[rep].entree[l].adrs);
		if(!numeriseur && (VoieIdent[vi].resultat == IDENT_ABSENT)) 
			printf(" %4s | ---          |  --  |  --  |       |      |                      |"," ");
		else {
			lu = VoieIdent[vi].lu; autre = 0;
			/* numeriseur */
			if((VoieIdent[vi].resultat == IDENT_ABSENT) || (VoieIdent[vi].resultat == IDENT_HS)) /* forcement <numeriseur> != 0, suite au test precedent */ {
				autre = (numeriseur->vsnid == 0); // pas BB
				if(numeriseur->def.serial == 0xFF) 
					printf(" %4s |%c%-12s |      |",numeriseur->def.fibre,autre?' ':'-',ModeleBN[numeriseur->def.type].nom);
				else printf(" %4s |%c%-12s | %3d  |",numeriseur->def.fibre,autre?' ':'-',ModeleBN[numeriseur->def.type].nom,numeriseur->def.serial);
				if(!autre) paumes++;
			} else if(VoieIdent[vi].resultat == IDENT_DESYNCHRO) {
				if(numeriseur) {
					if(numeriseur->def.serial == 0xFF) 
						printf(" %4s |?%-12s |      |",numeriseur->def.fibre,ModeleBN[numeriseur->def.type].nom);
					else printf(" %4s |?%-12s | %3d  |",numeriseur->def.fibre,ModeleBN[numeriseur->def.type].nom,numeriseur->def.serial);
				} else printf("      |?inconnu      |      |");
				tilt++;
			} else {
				if(VoieIdent[vi].resultat == IDENT_IMPREVU) {
					for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].ident == (lu & 0xFFF)) break;
					if(bb < NumeriseurNb) numeriseur = &(Numeriseur[bb]); else numeriseur = 0;
				}
				autre = (numeriseur != (TypeNumeriseur *)(Repart[rep].entree[l].adrs));
				if(numeriseur) {
					if(numeriseur->def.serial == 0xFF) 
						printf(" %4s |%c%-12s |      |",numeriseur->def.fibre,autre?'*':' ',ModeleBN[numeriseur->def.type].nom);
					else printf(" %4s |%c%-12s | %3d  |",numeriseur->def.fibre,autre?'*':' ',ModeleBN[numeriseur->def.type].nom,numeriseur->def.serial);
					if(autre) changes++;
				} else {
					vsnid = (lu >> 8) & 0xF;
					if(vsnid <= NUMER_BB_VSNMAX) type = NumeriseurTypeBB[vsnid]; else type = -1;
					if(type >= 0) { printf(" %4s |+%-12s | %3d  |"," ",ModeleBN[type].nom,lu & 0xFF); neufs++; }
					else printf(" %4s | inconnu      |      |"," ");
				}
			}
			/* ident */
			if(VoieIdent[vi].resultat == IDENT_DESYNCHRO) printf(" sync |");
			else if(!lu) printf("  --  |");
			else { if(lu > 0xFFF) printf(" %04X |",lu); else if(lu > 0xFF) printf("  %03X |",lu); else printf("  %02X  |",lu); }
			/* prise cryo */
			if(numeriseur) connecteur = numeriseur->fischer; else connecteur = 0;
			if(connecteur) printf("%5s  |",CablageEncodeConnecteur(connecteur)); else printf("  ext  |"); // pour ne pas imprimer "neant"
			/* position, nom et etat detecteur */
			if(numeriseur) {
				// pour boucler sur les vbb si plusieurs detec: if(numeriseur->voie[vbb] >= 0) detectr = &(Bolo[VoieManip[numeriseur->voie[vbb]].det]);
				// (a condition que la table soit complete..)
				for(bolo=0; bolo<BoloNb; bolo++) {
					detectr = &(Bolo[bolo]);
					for(cap=0; cap<detectr->nbcapt; cap++) if(detectr->captr[cap].bb.num == numeriseur->bb) break;
					if(cap < detectr->nbcapt) { voie = detectr->captr[cap].voie; break; }
				}
			} else bolo = BoloNb;
			if(bolo < BoloNb) {
				if(detectr->lu != DETEC_OK) bof++;
				if(connecteur) {
					if(detectr->pos == CABLAGE_POS_NOTDEF) printf(" nulle|");
					else printf(" %3s  |",CablageEncodePosition(detectr->pos));
				} else {
					if(detectr->pos == CABLAGE_POS_NOTDEF) printf("      |");
					else { printf(" %3s! |",CablageEncodePosition(detectr->pos)); decon++; }
				}
				if((voie >= 0) && (voie < VoiesNb)) printf("%c%-20s |",(detectr->lu != DETEC_OK)?'#':' ',VoieManip[voie].nom);
				else { printf("/%-20s |",detectr->nom); zarbi++; }
			} else printf("      | %-20s |",(numeriseur)?"(non connecte)":"");
		}
		printf("\n");
	}
	printf("|__________|______|______________|______|______|_______|______|______________________|\n");
	if(tilt)    printf("! marque '?': numeriseur desynchronise (non identifie)                               |\n");
	if(changes) printf("! marque '*': numeriseur imprevu (mais connu)                                        |\n");
	if(neufs)   printf("! marque '+': numeriseur absent de la configuration                                  |\n");
	if(paumes)  printf("! marque '-': numeriseur attendu, mais muet                                          |\n");
	if(decon)   printf("! marque '!': detecteur place mais non connecte                                      |\n");
	if(zarbi)   printf("! marque '/': detecteur connu mais voie non affectee                                 |\n");
	if(bof)     printf("| marque '#': detecteur non utilise                                                  |\n");
	if(tilt || paumes || changes || neufs || decon || zarbi || bof) 
		printf("|____________________________________________________________________________________|\n");
	return(0);
}
/* ========================================================================== */
int LectIdentBB() {
	int vi,vl,vbb,bolo,cap,l,rep,modl;
	int arret,d2,nbpts,rc;
	TypeADU erreur_acq;
	short rep_teste;
	char a_parle,a_panique,imprevus,ajouts,attendu;
	char texte[256],code_entree[8];
	TypeRepartiteur *repart; TypeNumeriseur *numeriseur;
	TypeADU lu;
	Panel panel;

//	if(AccesRestreint()) return(0);
	if(!RepartiteurRedefiniPerimetre(0,0)) return(0);
	printf("%s/ Reconnnaissance des numeriseurs\n",DateHeure());
	LectAcqLanceur = LECT_FROM_ELEM;
	if(SambaLecturePid > 0) { while(LectAcqLanceur != NEANT) sleep(1); return(0); }
	ArchInfoStatus = ARCH_STATUS_NONE;
	LectureLog = VerifIdentBB;
	if(!LectFixeMode(LECT_IDENT,1)) return(0);
	/* LectSelecteVoies(); delocalise les etrangers */
	if(SambaExploreHW && ((RepartNb > 1) || !Repart[0].local)) {
	#ifdef PERIMETRE_AVEC_PLANCHE
		RepartiteurRedefiniPerimetre(0,0);
	#else
		if(OpiumExec(pRepartLocal->cdr) == PNL_CANCEL) {
			printf("%s/ Reconnnaissance abandonnee\n",DateHeure());
			return(0);
		}
	#endif
	}
	rep_teste = 0;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) {
		repart = &(Repart[rep]);
		modl = repart->type;
		printf("%c %s (%s/%s %s)\n",rep_teste?'+':'.',repart->nom,ModeleRep[modl].nom,InterfaceListe[ModeleRep[repart->type].interface],repart->parm);
		repart->utile = 1;
		rep_teste++;
	} else Repart[rep].utile = 0;
	if(rep_teste) printf("\n= %d repartiteur%s a examiner\n",Accord1s(rep_teste));
	else {
		OpiumWarn("Rien a identifier");
		printf("! Aucun repartiteur n'est candidat a l'identification\n");
		return(0);
	}
	
//	RepartiteursElection(1);
	RepartiteursSelectionneVoiesLues(1,VerifIdentBB); printf("\n");
	if(SambaEchantillonLngr > 0) {
		if(VerifIdentBB) RepartImprimeEchantillon(0);
	} else {
		printf("! Aucune voie n'est selectionnable\n\n");
		OpiumWarn("Pas de voie a identifier");
		return(0);
	}
	VoieIdentMax = SambaEchantillonLngr;
	
	LectRazCompteurs();
	arret = SettingsSynchroMax; SettingsSynchroMax = 1;
    rc = 1;
	d2 = 100; // Diviseur2
    nbpts = d2 * VoieIdentMax;
    if(nbpts > LectTamponSize) {
        if(LectTamponInit) { free(LectTamponInit); LectTamponInit = 0; LectTamponSize = 0; }
        LectTamponInit = (TypeDonnee *)malloc(nbpts * sizeof(TypeDonnee));
        if(!LectTamponInit) {
            printf("  | Tampon d'Initialisation: %ld octets introuvables\n",nbpts * sizeof(TypeDonnee));
            goto hev;
        }
        LectTamponSize = nbpts;
		bzero(LectTamponInit,nbpts * sizeof(TypeDonnee));
    }
    LectTamponPtr = LectTamponInit;
    nbpts = d2;
    for(vi=0; vi<VoieIdentMax; vi++) {
        VoieIdent[vi].donnees = (TypeADU *)LectTamponPtr;
        LectTamponPtr += nbpts;
        VoieIdent[vi].prochain = VoieIdent[vi].donnees;
        VoieIdent[vi].dim = nbpts;
        VoieIdent[vi].lus = VoieIdent[vi].analyse = 0;
        VoieIdent[vi].essais = 0;
        VoieIdent[vi].erreurs = 0;
		VoieIdent[vi].resultat = IDENT_ABSENT;
		rep = SambaEchantillon[vi].rep; l = SambaEchantillon[vi].entree; vbb = SambaEchantillon[vi].voie;
		numeriseur = (rep >= 0)? (TypeNumeriseur *)(Repart[rep].entree[l].adrs): 0;
		VoieIdent[vi].attendu = (numeriseur && (vbb >= 0))? numeriseur->adc_id[vbb]: 0;
    }
    TrmtIdentPasFaite = 1;
	
	Acquis[AcquisLocale].etat.active = LectSynchro(NUMER_MODE_IDENT);
	if(!Acquis[AcquisLocale].etat.active) {
		printf("%s/ Synchro pas trouvee (derniere valeur lue: %08X)\n",DateHeure(),LectEnCours.valeur);
		printf("==================== Identification impossible ====================\n\n");
		rc = 0; // (int)LectMessageArret(_ORIGINE_,8,LECT_NFND);
	} else {
		printf("%s/ Lecture des identificateurs demarree\n",DateHeure());
	//	MenuItemAllume(mLectDemarrage,1,"Stopper ",GRF_RGB_RED);
		MenuItemAllume(mLectDemarrage,1,L_("En cours"),GRF_RGB_GREEN);
		MenuItemAllume(mLectArret,1,L_("Stopper "),GRF_RGB_RED);
		LectDeclencheErreur(_ORIGINE_,0,0);
		LectDepileDuree = 0;
		LectDepileTfin = DateMicroSecs(); // (VerifTempsPasse? DateMicroSecs(): 0);
		while(Acquis[AcquisLocale].etat.active) { LectDepileDonnees(); OpiumUserAction(); }
		erreur_acq = LectMessageArret(_DECLENCHE_);
		if(LectErreur.code) {
			if(LectEnCours.rep >= 0) printf("%s! Arret au mot #%d/%d: %08X\n",DateHeure(),Repart[LectEnCours.rep].bottom,Repart[LectEnCours.rep].top,LectEnCours.valeur);
			else printf("%s! Arret a la donnee #%d: %08X\n",DateHeure(),LectEnCours.echantillon,LectEnCours.valeur);
			if(PCIdemande) printf("%s/ La pile a ete vue %d fois demi-pleine et %d fois pleine\n",DateHeure(),RepartPci.halffull,RepartPci.fullfull);
		}
		MenuItemAllume(mLectDemarrage,1,L_("Demarrer"),GRF_RGB_YELLOW);
		MenuItemEteint(mLectArret,1,L_("Arrete  "));
		TrmtSurBuffer();
		//- perturbe le shifteur moyen a ce stade: if(erreur_acq) OpiumFail("Erreur %04X (%s), %s est stoppee.",erreur_acq,ArchExplics,LectSrceTexte);
		/* Debriefing */
		LectJournalLaFin(0,0,0); printf("\n");
	    if(VerifIdentBB) {
			int k; int vmin,vmax;
			{
				int lus,analyses,erreurs;
				lus = analyses = erreurs = 0;
				for(vi=0; vi<VoieIdentMax; vi++) {
					lus += VoieIdent[vi].lus;
					analyses += VoieIdent[vi].analyse;
					erreurs += VoieIdent[vi].erreurs;
				}
				printf(". Identifications lues: %d, analysees: %d, erronees: %d\n",lus,analyses,erreurs);
			}
			printf(". Donnees pour l'identification des voies:\n");
			vmin = 0;
			do {
				vmax = vmin + 18;
				if(vmax > VoieIdentMax) vmax = VoieIdentMax;
				printf("\n  #v"); for(vi=vmin; vi<vmax; vi++) printf(" %4d",vi+1); // printf("\n");
				printf("\n t\\f"); for(vi=vmin; vi<vmax; vi++) {
					rep = SambaEchantillon[vi].rep; l = SambaEchantillon[vi].entree;
					RepartiteurCodeEntree(&(Repart[rep]),l,code_entree);
					printf(" %4s",code_entree);
				}
				printf("\n");
				for(k=0; k<10; k++) {
					printf("%3d:",k); for(vi=vmin; vi<vmax; vi++) printf(" %04X",VoieIdent[vi].donnees[k]); printf("\n");
				}
				printf(" ---"); for(vi=vmin; vi<vmax; vi++) printf(" ----"); printf("\n");
				printf(" dim"); for(vi=vmin; vi<vmax; vi++) printf(" %4d",VoieIdent[vi].dim); printf("\n");
				printf(" lus"); for(vi=vmin; vi<vmax; vi++) printf(" %4d",VoieIdent[vi].lus); printf("\n");
				printf(" ana"); for(vi=vmin; vi<vmax; vi++) printf(" %4d",VoieIdent[vi].analyse); printf("\n");
				printf(" ess"); for(vi=vmin; vi<vmax; vi++) printf(" %4d",VoieIdent[vi].essais); printf("\n");
				printf(" err"); for(vi=vmin; vi<vmax; vi++) printf(" %4d",VoieIdent[vi].erreurs); printf("\n");
				vmin = vmax;
			} while(vmin < VoieIdentMax);
			printf("\n");
		}
	    if(VerifIdentRepart) {
			printf(". Attribution par repartiteur:");
			for(vi=0; vi<VoieIdentMax; vi++) {
				char a_la_ligne; int flt,fbr,fltabs;
				// rep = VoieIdent[vi].rep; vl = vi - Repart[rep].vi0;
				rep = SambaEchantillon[vi].rep; vl = vi - Repart[rep].premiere_donnee;
				l = SambaEchantillon[vi].entree; vbb = SambaEchantillon[vi].voie;
				modl = Repart[rep].type;
				a_la_ligne = 0;
				flt = IPE_FLT_NUM(l); fbr = IPE_FLT_FBR(l,flt); fltabs = flt + Repart[rep].r.ipe.decale;
				if(!vl) {
					printf("\n  | %8s %-4s",ModeleRep[modl].code,Repart[rep].code_rep);
					a_la_ligne = -1;
				} else {
					if(ModeleRep[modl].regroupe_numer) a_la_ligne = !vbb;
					else if(!Repart[rep].nbentrees) a_la_ligne = 1;
					else a_la_ligne = !(vl % Repart[rep].nbentrees);
					if(a_la_ligne) {
						if((Repart[rep].famille == FAMILLE_IPE) && (flt && !fbr)) printf("\n  |              ------");
						printf("\n  |              ");
					}
				}
				if(a_la_ligne) {
					if(Repart[rep].famille == FAMILLE_IPE) 
						printf(".%c%d %c",IPE_CODE_FIBRE(fltabs,fbr),(a_la_ligne < 0)?':':' ');
					else if(Repart[rep].nbentrees > 1) 
						printf(".%-2d %c",l,(a_la_ligne < 0)?':':' ');
					else printf("    :");
				}
				if(vbb >= 0) printf(" %04X",VoieIdent[vi].lu); else printf(" (%04X)",VoieIdent[vi].lu);
			}
			printf("\n");
		}

		/* Analyse */
		a_parle = a_panique = 0; imprevus = ajouts = 0;
		for(vi=0; vi<VoieIdentMax; vi++) {
			vbb = SambaEchantillon[vi].voie;
			lu = VoieIdent[vi].lu;
			if(((lu & 0xFFF) == 0xAAA) || ((lu & 0xFFF) == 0x555)) {
				VoieIdent[vi].resultat = IDENT_DESYNCHRO;
				if(!vbb) a_panique++;
			} else {
				if(lu == 0) VoieIdent[vi].resultat = IDENT_ABSENT;
				else if(lu == 0xFFFF) VoieIdent[vi].resultat = IDENT_HS;
				else if(((lu & 0xFF) < 0x7F) && !vbb) a_parle++;
				attendu = 0;
				rep = SambaEchantillon[vi].rep; l = SambaEchantillon[vi].entree;
				if((rep >= 0) && (l >= 0)) {
					numeriseur = (TypeNumeriseur *)(Repart[rep].entree[l].adrs);
					if(Repart[rep].famille == FAMILLE_OPERA) lu &= 0x7FFF; // bit 15 positionne par les Opera en mode Repart[rep].r.opera.mode = BB1
					if(numeriseur) for(bolo=0; bolo<BoloNb; bolo++) {
						for(cap=0; cap<Bolo[bolo].nbcapt; cap++) if(Bolo[bolo].captr[cap].bb.num == numeriseur->bb) break;
						if(cap < Bolo[bolo].nbcapt) { attendu = (Bolo[bolo].lu == DETEC_OK); break; }
					}
				} else numeriseur = 0;
				if(VoieIdent[vi].attendu == lu) {
					if(lu != 0) VoieIdent[vi].resultat = IDENT_NORMAL;
					if(numeriseur) {
						if(numeriseur->ident && (rep >= 0)) Repart[rep].bb = 1;
					} // on avait mis 0 dans RepartiteursSelectionneVoiesLues faute de savoir
				} else {
					if((VoieIdent[vi].attendu || !vbb) && VerifIdentBB) {
						if((rep >= 0) && (l >= 0)) {
							RepartiteurCodeEntree(&(Repart[rep]),l,code_entree);
							vl = vi - Repart[rep].premiere_donnee;
							printf("! %s.%s: on attendait %04X et on a lu %04X [donnee #%d, voie #%d]\n",
								Repart[rep].nom,code_entree,VoieIdent[vi].attendu,lu,vi+1,vl+1);
						} else printf("! On attendait %04X et on a lu %04X [donnee #%d]\n",VoieIdent[vi].attendu,lu,vi+1);
					}
					if(!vbb) { if(numeriseur) { if(attendu) imprevus++; } else ajouts++; }
					if(lu != 0) VoieIdent[vi].resultat = IDENT_IMPREVU;
				}
			}
		}
		
		if(a_panique) printf("! %d numeriseur%s desynchronise%s\n",Accord2s(a_panique));
		if(a_parle) {
			printf(". %d numeriseur%s detecte%s\n",Accord2s(a_parle));
			LectIdentImprime(); printf("\n");
			if(imprevus || ajouts) {
				if(erreur_acq) OpiumFail("%s a ete interrompue sur erreur %04X (%s)",LectSrceTexte,erreur_acq,ArchExplics);
				texte[0] = '\0';
				if(ajouts) sprintf(texte,"%d ajout%s ",Accord1s(ajouts));
				if(ajouts && imprevus) strcat(texte," + ");
				if(imprevus) sprintf(texte+strlen(texte),"%d changement%s ",Accord1s(imprevus));
				strcat(texte,"d'identificateur");
				if(AccesRestreint()) OpiumWarn("%s (voir log)",texte);
				else {
					if(OpiumChoix(2,SambaNonOui,"%s. On tient compte de cette identification?",texte)) {
						char change_repart,change_liste; int dernier_sauve;
						dernier_sauve = NumeriseurNb - 1;
						change_repart = RepartAjusteIdent();
						RepartiteursImprimeNumeriseurs();
						// if(OpiumDisplayed(bSchemaManip)) OrgaTraceTout(0,0);
						if(SambaIdentSauvable) {
							if(change_repart) {
								if(OpiumChoix(2,SambaNonOui,"On sauve ces connections?")) {
									if(RepartiteursSauve()) printf("  . Connections enregistrees\n");
									else OpiumFail("Impossible: %s",strerror(errno));
								}
							}
							panel = PanelCreate(NumeriseurNb);
							for(vi=0; vi<VoieIdentMax; vi++) if(SambaEchantillon[vi].voie == 0) {
								rep = SambaEchantillon[vi].rep; l = SambaEchantillon[vi].entree;
								numeriseur = (TypeNumeriseur *)(Repart[rep].entree[l].adrs);
								if(numeriseur) numeriseur->change_id = 0;
							}
							for(vi=0; vi<VoieIdentMax; vi++) {
								rep = SambaEchantillon[vi].rep; 
								l = SambaEchantillon[vi].entree; vbb = SambaEchantillon[vi].voie;
								numeriseur = (TypeNumeriseur *)(Repart[rep].entree[l].adrs);
								if(numeriseur && (vbb >= 0)) {
									if(numeriseur->adc_id[vbb] != VoieIdent[vi].lu) {
										if(!numeriseur->change_id) PanelBool(panel,numeriseur->nom,&(numeriseur->change_id));
										numeriseur->adc_id[vbb] = VoieIdent[vi].lu; numeriseur->change_id = 1;
									}
								}
							}
							if(PanelItemNb(panel)) {
								if(OpiumChoix(2,SambaNonOui,"On sauve ces numeriseurs?")) {
									if(OpiumExec(panel->cdr) == PNL_OK) {
										change_liste = 0;
										for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) {
											for(l=0; l<Repart[rep].nbentrees; l++) if((numeriseur = (TypeNumeriseur *)(Repart[rep].entree[l].adrs))) {
												if(numeriseur->change_id) {
													NumeriseursEcrit(NumeriseurFichierLu,NUMER_RESSOURCES,numeriseur->bb);
													numeriseur->change_id = 0;
													if(numeriseur->bb > dernier_sauve) change_liste = 1;
												}
											}
										}
										if(change_liste) NumeriseursEcrit(NumeriseurFichierLu,NUMER_CONNECTION,NUMER_TOUS);
									}
								}
							}
							PanelDelete(panel);
						}
					} else printf("* Identification NON CONSERVEE\n");
				}
			} else if(a_panique) OpiumWarn("%d numeriseur%s desynchronise%s, %d numeriseur%s detecte%s",Accord2s(a_panique),Accord2s(a_parle));
			else OpiumNote("Reconnaissance excellente");
			rc = 1;
		} else {
			printf("! AUCUN NUMERISEUR N'EST LISIBLE\n");
			printf("! Cette identification ne sera PAS enregistree\n");
			rc = 0;
		}
	}

hev:
	/* Restoration des parametres specialement modifies pour l'identification */
	LectBranche(1);
	SettingsSynchroMax = arret;
	return(rc);
}
/* ========================================================================== */
static int LectVerifieConnections(LECT_MODE mode) {
	int voie,vt;
	
	LectVoiesIntrouvables = 0;
    if(LectSurFichier) { /* on ne recupere a ce niveau que les voies sauvees */
		for(voie=0; voie<VoiesNb; voie++) VoieLue[voie] = (VoieTampon[voie].lue)? voie: -1;
		return(1);
	}
	if(SettingsIdentCheck) { if(!LectIdentBB()) return(0); }
	RepartiteursSelectionneVoiesLues(0,LectureLog);
	if(LectureLog && !SambaMaitre) RepartImprimeEchantillon(1);
	if(RepartNbActifs == 0) {
		OpiumFail("unmoegliche Lekture (schauen Sie das LogFenster an)");
		LectVoiesIntrouvables = 1;
		return(0);
	}
	LectCntl.LectMode = mode; /* en cas d'appel direct */

	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue /* && !VoieManip[voie].pseudo */ && (VoieTampon[voie].trig.trgr->origine == TRGR_INTERNE)) {
		for(vt=0; vt<SambaEchantillonLngr; vt++) if(SambaEchantillon[vt].voie == voie) break;
		if(vt >= SambaEchantillonLngr) {
			if(!LectVoiesIntrouvables) printf("\n");
			printf("! On ne sait pas ou lire le stream de la voie %s. Elle devient 'invalidee'\n",VoieManip[voie].nom);
			VoieTampon[voie].lue = 0;
			VoieManip[voie].def.trmt[TRMT_AU_VOL].type = TRMT_INVALID;
			LectVoiesIntrouvables++;
		}
	}
	if(LectVoiesIntrouvables) {
		OpiumFail("%d voie%s introuvable%s dans l'electronique (voir log)",Accord2s(LectVoiesIntrouvables));
		printf("\n");
	}
	
    LectIdentFaite = 1;
	return(1); // (!LectVoiesIntrouvables);
}
/* ========================================================================== */
int LectAcqElementaire(NUMER_MODE mode) {
	TypeADU erreur_acq; int fmt;

	LectAcqModeNumer = mode;
	LectAcqLanceur = LECT_FROM_ELEM;
	LectAcqEnCours = LECT_PRGM_ELEM;
	if(SambaLecturePid > 0) { while(LectAcqLanceur != NEANT) sleep(1); return(0); }
	LectureLog = 0;
	LectTexec = 0;
	MiniStream = 0; LectStreamNum = LectRegenNum = 0;
	for(fmt=0; fmt<ARCH_TYPEDATA; fmt++) LectTrancheNum[fmt] = 0;
	SequenceCourante = -1;
	LectSession = 1; LectStampPrecedents = 0; if(OpiumAlEcran(pReprises)) OpiumRefresh(pReprises->cdr);
	Trigger.actif = Trigger.enservice = 0;
	ArchInfoStatus = ARCH_STATUS_NONE;
	printf("%s/ Mise en route de la lecture\n",DateHeure());
	strcpy(LectInfo[0],L_("Acquisition simplifiee en cours"));
	if(OpiumAlEcran(pLectRegen)) PanelRefreshVars(pLectRegen);
	LectAutreSequence = 0;
	if(!LectFixeMode(LECT_DONNEES,1)) return(0);
//-	LectSelecteVoies();
	RepartiteursElection(1);
//-	if(!LectConstruitTampons(LectureLog)) return(0);
	LectRazCompteurs();
	LectLastEvt = Acquis[AcquisLocale].etat.evt_trouves;
	RunCategNum = LectCntl.RunCategNum;
//	SettingsRunEnvr = LectCntl.SettingsRunEnvr;
//-	ArchiveDefini(LectureLog);
	
	/* Demarrage de l'electronique et execution de la tache de lecture */
	MenuItemAllume(mLectDemarrage,1,L_("En cours"),GRF_RGB_GREEN);
	MenuItemAllume(mLectArret,1,L_("Stopper "),GRF_RGB_YELLOW);
	if(OpiumAlEcran(gEvtRunCtrl)) GraphBlank(gEvtRunCtrl);
	if(OpiumAlEcran(gEvtSolo)) GraphBlank(gEvtSolo);
	if(OpiumAlEcran(pLectEtat)) PanelRefreshVars(pLectEtat);
	int catg; for(catg=0; catg<MonitCatgNb+1; catg++) {
		TypeMonitCatg *categ = MonitCatgPtr[catg];
		if(OpiumAlEcran(categ->gSuivi)) GraphBlank(categ->gSuivi);
		if(OpiumAlEcran(categ->gEvt)) GraphBlank(categ->gEvt);
	}
	LectureLog = 1;
	LectSigne = 1;
	erreur_acq = LectExec(mode);

	LectAcqElemTermine();
	return(0);
}
/* ========================================================================== */
void LectAcqElemTermine() {
	MenuItemAllume(mLectDemarrage,1,L_("Demarrer"),GRF_RGB_YELLOW);
	MenuItemEteint(mLectArret,1,L_("Arrete  "));
	LectBranche(1);
	LectRepartStoppe(0);

	/* Traitements de fin de la tache de la lecture */
	Acquis[AcquisLocale].etat.status = SATELLITE_ATTENTE;
	/* Terminaison de l'affichage des donnees brutes */
	LectDisplay();
	printf("%s/ Sortie de la fonction %s\n",DateHeure(),__func__);
}
/* ========================================================================== */
int LectAcqStd() {
	int rep,voie,rc,i,k,l,num,fmt; char doit_terminer;
	TypeADU erreur_acq;
	float nb;
	char couleur[16],texte[80];
	char commande[1024 + (MAXCOMMENTNB * (MAXCOMMENTLNGR + 1))]; char com1;
	char note[1024 + (MAXCOMMENTNB * (MAXCOMMENTLNGR + 1))];
	FILE *f;
	struct stat infos;

	LectAcqLanceur = LECT_FROM_STD;
	LectAcqEnCours = LECT_PRGM_STD;
	if(SambaLecturePid > 0) { while(LectAcqLanceur != NEANT) sleep(1); return(0); }
	l = 0; // GCC
	LectureLog = 1;
	LectModeStatus = 0; ReglagesEtatPeutStopper = NumeriseurEtatPeutStopper = NumeriseurPeutStopper = 0;
	LectSession = 1; LectStampPrecedents = 0; if(OpiumAlEcran(pReprises)) OpiumRefresh(pReprises->cdr);
	LectTexec = 0;
	MiniStream = 0; LectStreamNum = LectRegenNum = 0;
	for(fmt=0; fmt<ARCH_TYPEDATA; fmt++) LectTrancheNum[fmt] = 0;
	SequenceCourante = -1;
	ArchInfoStatus = ARCH_STATUS_NONE;
	sprintf(ArchDuree,"%02dh%02d'%02d",0,0,0);
	ArchEvtTot = 0;
	for(fmt=0; fmt<ARCH_TYPEDATA; fmt++) ArchiveEcrits[fmt] = 0.0;
	if(pDureeSynchro) { if(OpiumDisplayed(pDureeSynchro->cdr)) PanelRefreshVars(pDureeSynchro); }
#ifndef CODE_WARRIOR_VSN
	LectAutreSequence = 1;
	for(num=0; num<OscilloNb; num++) if(OscilloReglage[num]->acq) { LectAutreSequence = 0; break; }
	if(LectAutreSequence) LectAutreSequence = !LectModeSpectresAuto && LectAutoriseSequences && !RegenEnCours;
	if(LectAutreSequence) {
		char derniere;
		SequenceDemarre(&derniere);
		if(derniere) LectAutreSequence = 0;
		gettimeofday(&LectDateRun,0);
		SequenceParms(0);
//		printf("* Prochaine sequence: #%d (%s)\n",SequenceCourante,LectAutreSequence?"a suivre":"unique");
	}
#else
	LectAutreSequence = 0;
#endif
	do {
		if(LectSession < 2) {
			if(!LectFixeMode(LECT_DONNEES,1)) return(0);
			for(rep=0; rep<RepartNb; rep++) {
				Repart[rep].mode = 0;
				Repart[rep].status_demande = 0;
			}
			LectSelecteVoies();
			SambaRunDate();
			if(Archive.enservice) {
				if(!LectModeSpectresAuto) {
					SambaRunNomNouveau();
					if((LectAcqCollective && LectStampMini) || !LectChangeParms) {
						ArgScan(FichierRunEnvir,LectEnvirDesc);
						LectChangeParms = 1;
					} else if((ScriptDateLimite <= 0) && (LectRunTime == 0)) {
						if(OpiumExec(pRunParms->cdr) == PNL_CANCEL) return(0);
					}
					SambaRunNomEnregistre();
					if(OpiumDisplayed(pRunMode->cdr)) PanelRefreshVars(pRunMode);
					for(fmt=0; fmt<ARCH_TYPEDATA; fmt++) if(ArchSauve[fmt]) {
						if((rc = RepertoireAbsent(ArchZone[fmt]))) {
							printf("* La creation du repertoire %s retourne %d (errno=%d, %s)\n",
								ArchZone[fmt],rc,errno,strerror(rc));
							OpiumFail("LA SAUVEGARDE EST ABANDONNEE");
							ArchSauve[fmt] = 0;
							// if(OpiumDisplayed(cArchive->cdr)) OpiumRefresh(cArchive->cdr);
							// if(OpiumDisplayed(pLectMode->cdr)) OpiumRefresh(pLectMode->cdr);
						}
						if(ArchiveSurRepert) {
							sprintf(commande,"mkdir %s",ArchiveChemin[fmt]);
							errno = 0;
							if((rc = system(commande))) {
								printf("! La creation de ce repertoire retourne %d (%s/ errno=%d, %s):\n",
									rc,strerror(rc),errno,strerror(errno));
								printf("  '%s'\n",ArchiveChemin[fmt]);
								OpiumWarn("Le partionnement des sauvegardes est abandonne");
								ArchiveSurRepert = 0;
								LectDureeActive = 0;
								// if(OpiumDisplayed(pLectHachoir->cdr)) PanelRefreshVars(pLectHachoir);
								if(OpiumDisplayed(bSauvegardes)) OpiumRefresh(bSauvegardes);
							}
						}
					}
					fmt = (ArchSauve[EVTS])? EVTS: STRM;
					if(ArchiveSurRepert) sprintf(RunJournal,"%s%s%s_log",ArchiveChemin[fmt],FILESEP,Acquis[AcquisLocale].etat.nom_run);
					else strcat(strcpy(RunJournal,ArchiveChemin[fmt]),"_log");
					printf("*** Journal egalement ecrit dans %s\n",RunJournal);
					LogOnFile(RunJournal);
					printf(" ____________________________________________________________________________________\n");
					l = printf("| Lecture %s: %s, du %s a %s (avec la version %s)",Trigger.demande? "declenchee": "continue",
						Acquis[AcquisLocale].etat.nom_run,DateCivile(),DateHeure(),SambaVersion);
					SambaCompleteLigne(86,l);
					l = printf("| ");
					if(SettingsRunFamille == RUN_FAMILLE_BANC) {
						if(RunTypeName >= 0) {
							EnvirVarPrintVal(&(EnvirVar[RunTypeVar]),RunTypeName,TYPERUN_NOM);
							l += printf("Run de %s ",RunTypeName);
						}
					} else {
						ArgKeyGetText(RunCategCles,LectCntl.RunCategNum,RunCategName,TYPERUN_NOM);
						l += printf("Run de %s ",RunCategName);
					}
					// ArgKeyGetText(RunEnvir,LectCntl.SettingsRunEnvr,texte,80); l += printf(" (%s)",texte);
					if(EnvirVarNb) {
						l += printf("avec:"); SambaCompleteLigne(86,l);
						for(i=0; i<EnvirVarNb; i++) if(i != RunTypeVar) {
							if(EnvirVar[i].explics[0] != '\0') l = printf("| . %s: ",EnvirVar[i].explics);
							else l = printf("| . %s = ",EnvirVar[i].nom);
							EnvirVarPrintVal(&(EnvirVar[i]),texte,80); l += printf("%s",texte);
							SambaCompleteLigne(86,l);
						}
					} else SambaCompleteLigne(86,l);
					com1 = 1;
					for(i=0; i<MAXCOMMENTNB; i++) if(LectComment[i][0] != '\0') {
						LectComment[i][MAXCOMMENTLNGR-1] = '\0';
						if(com1) { l = printf("| Note:"); com1 = 0; } else {
							if((l + 2 + strlen(LectComment[i])) > 86) {
								SambaCompleteLigne(86,l); l = printf("|      ");
							}
						}
						l += printf(" "); l += printf(LectComment[i]);
					}
					if(!com1) SambaCompleteLigne(86,l);
					if(Trigger.demande && (BolosUtilises > 1)) {
						l = printf("| Sauvegarde des detecteurs %s",ArchModeTexte[ArchDetecMode]);
						if(BolosAssocies) l += printf(", %s leurs associes",ArchAssocMode? "avec":"sans");
						SambaCompleteLigne(86,l);
					}
					if(ArchiveSurRepert) {
						OpiumPhotoDir(ArchiveChemin[fmt]);
						l = printf("| Photos sauvees dans %s",ArchiveChemin[fmt]); SambaCompleteLigne(86,l);
					}
					printf("|____________________________________________________________________________________|\n\n");
				}
				strcpy(note,Trigger.demande? "event ": "stream ");
				ArgKeyGetText(RunCategCles,LectCntl.RunCategNum,texte,TYPERUN_NOM);
				strcat(note,texte);
				// ArgKeyGetText(RunEnvir,LectCntl.SettingsRunEnvr,texte,80);
				// strcat(strcat(strcat(note," ("),texte),")");
				if(Trigger.demande) strcat(strcat(note,", sauvegarde detecteurs "),ArchModeTexte[ArchDetecMode]);
				com1 = 1;
				for(i=0; i<MAXCOMMENTNB; i++) if(LectComment[i][0] != '\0') {
					LectComment[i][MAXCOMMENTLNGR-1] = '\0';
					if(com1) { strcat(note,","); com1 = 0; }
					strcat(strcat(note," "),LectComment[i]);
				}
				SambaNote("Debut du run %s: %s\n",Acquis[AcquisLocale].etat.nom_run,note);
				if(LoggerAdrs[0]) {
					if(LoggerType == CAHIER_ELOG) {
						switch(AcquisLocale) {
							case 0: strcpy(couleur,"blue"); break;
							case 1: strcpy(couleur,"green"); break;
							case 2: strcpy(couleur,"red"); break;
							default: strcpy(couleur,"magenta"); break;
						}
						sprintf(commande,"%s -a \"Subject=Run Start\" \"Started [COLOR=%s][B]%s[/B][/COLOR]: %s\" &",LoggerCmde,couleur,Acquis[AcquisLocale].etat.nom_run,note);
						system(commande);
					} else {
						FILE *f; int voie;
						if((f = fopen(LoggerAdrs,"a"))) {
							fprintf(f,"%02d.%02d.%04d %02dh%02d %02d\": run %s\n",ExecJour,ExecMois,2000+ExecAn,ExecHeure,ExecMin,ExecSec,Acquis[AcquisLocale].etat.nom_run);
							/* user conditions: HV1, HV2, G */
							/* Tmin, Tmax, ADC */
							if(VoiesNb == 1) {
								voie = 0;
								fprintf(f,"                       ");
								fprintf(f,"Tmin=%f, ADC=%g\n",VoieTampon[voie].trig.trgr->montee,VoieTampon[voie].trig.trgr->minampl);
							}
							com1 = 1;
							for(i=0; i<MAXCOMMENTNB; i++) if(LectComment[i][0] != '\0') {
								LectComment[i][MAXCOMMENTLNGR-1] = '\0';
								if(!com1) { if((l + 2 + strlen(LectComment[i])) > 100) { fprintf(f,"\n"); com1 = 1; } }
								if(com1) { l = fprintf(f,"                       "); com1 = 0; }
								l += printf(" "); l += printf(LectComment[i]);
							}
							if(!com1) fprintf(f,"\n");
							fclose(f);
						}
					}
				}
				/* Debut de l'impression de l'entete de log du run */
				LectJournalLeDebut();
			} else /* !Archive.enservice */ {
                OpiumPhotoDir(0);
				if(LectModeSpectresAuto) {
					if(LectModeSpectresBanc <= 0) {
						if(!LectCompacteUtilisee) {
							if(!CalcSpectreAutoConstruit()) return(0);
						} else {
							LectCompacteAffiche(LECT_COMPACTE_BRUIT);
							OpiumUserAction();
						}
					}
				#ifdef SPECTRES_AUTO_ALLOC_DYN
					if(!LectSpectreTampons()) return(0); 
				#endif
				}
			}
			if(!LectVerifieConnections(LECT_DONNEES)) return(0);
			if(!SambaMaitre) RepartiteursElection(1);
		}
		
		/* Initialisation des variables globales de la branche Lecture */
		if(!LectConstruitTampons(LectureLog)) return(0);
		if(LectSession < 2) LectJournalTrmt();
		LectRazCompteurs();
		if((LectSession < 2) && (SequenceCourante >= 0)) {
			if(SequenceCodeCourant() == SEQUENCE_SPECTRES) {
				if(!LectSpectreTampons()) return(0);
			}
		}
		SequencesImprime();

		/* Initialisation des variables globales des autres branches */
		if(LectSurFichier && LectParBloc) {
			for(voie=0; voie<VoiesNb; voie++) VoieManip[voie].def.trmt[TRMT_AU_VOL].type = NEANT;
		}
		if(!TrmtRAZ(LectureLog)) {
			printf("\n========== Parametrage incorrect, pas de lecture ==========\n");
			return(0);
		}
		LectAffichePanique = 0;
		LectLastEvt = Acquis[AcquisLocale].etat.evt_trouves;
		RunCategNum = LectCntl.RunCategNum;
		// SettingsRunEnvr = LectCntl.SettingsRunEnvr;
		if(!LectAcqCollective || !LectStampMini) {
			ArgPrint(FichierRunEnvir,LectEnvirDesc);
			printf("  . Fichier %s modifie\n",FichierRunEnvir);
		}
		ArchiveDefini(LectureLog);
		LectJournalLaSuite((LectSession < 2));
		
		/* Demarrage de l'electronique */
		if(OpiumDisplayed(bLecture)) doit_terminer = OpiumRefreshBegin(bLecture); else doit_terminer = 0;
		if(OpiumAlEcran(mLectDemarrage)) MenuItemAllume(mLectDemarrage,1,L_("En cours"),GRF_RGB_GREEN);
		if(OpiumAlEcran(mLectArret)) MenuItemAllume(mLectArret,1,L_("Stopper "),GRF_RGB_YELLOW);
		if(OpiumAlEcran(mLectSuiviStart)) MenuItemAllume(mLectSuiviStart,1,L_("En cours"),GRF_RGB_GREEN);
		if(OpiumAlEcran(mLectSuiviStop)) MenuItemAllume(mLectSuiviStop,1,0,GRF_RGB_YELLOW);
		strcpy(LectInfo[0],L_("Acquisition normale en cours"));
		if(OpiumAlEcran(pLectRegen)) PanelRefreshVars(pLectRegen);
		if(OpiumAlEcran(pLectEvtNum)) PanelRefreshVars(pLectEvtNum);
		if(OpiumAlEcran(gEvtRunCtrl)) GraphBlank(gEvtRunCtrl);
		int catg; for(catg=0; catg<MonitCatgNb+1; catg++) {
			TypeMonitCatg *categ = MonitCatgPtr[catg];
			if(OpiumAlEcran(categ->gSuivi)) GraphBlank(categ->gSuivi);
			if(OpiumAlEcran(categ->gEvt)) GraphBlank(categ->gEvt);
		}
		if(OpiumAlEcran(gEvtSolo)) GraphBlank(gEvtSolo);
		if(doit_terminer) OpiumRefreshEnd(bLecture);

		if(LectCompacteUtilisee && mCalcSpectreControle->cdr && (LectCompacteNumero == LECT_COMPACTE_BRUIT)) {
			doit_terminer = OpiumRefreshBegin((mCalcSpectreControle->cdr)->planche);
			MenuItemAllume(mCalcSpectreControle,1,L_("Mesure   "),GRF_RGB_GREEN);
			MenuItemAllume(mCalcSpectreControle,2,0,GRF_RGB_YELLOW);
			MenuItemAllume(mCalcSpectreControle,3,0,GRF_RGB_YELLOW);
			MenuItemEteint(mCalcSpectreControle,4,0);
			if(doit_terminer) OpiumRefreshEnd((mCalcSpectreControle->cdr)->planche);
		}
	#ifndef STATUS_AVEC_TIMER
		if(OpiumAlEcran(pLectEtat)) PanelRefreshVars(pLectEtat);
	#endif
		LectSigne = !LectSurFichier;
	
		/* Execution de la tache de lecture */
		erreur_acq = LectExec(NUMER_MODE_ACQUIS);

		/* Remise en etat des affichages */
		if(OpiumDisplayed(bLecture)) doit_terminer = OpiumRefreshBegin(bLecture); else doit_terminer = 0;
		if(OpiumAlEcran(mLectDemarrage)) MenuItemAllume(mLectDemarrage,1,L_("Demarrer"),GRF_RGB_YELLOW);
		if(OpiumAlEcran(mLectArret)) MenuItemEteint(mLectArret,1,L_("Arrete  "));
		if(OpiumAlEcran(mLectSuiviStart)) MenuItemAllume(mLectSuiviStart,1,L_("Demarrer"),GRF_RGB_YELLOW);
		if(OpiumAlEcran(mLectSuiviStop)) MenuItemEteint(mLectSuiviStop,1,0);
		strcpy(LectInfo[0],L_("Acquisition terminee"));
		if(LectEntretien) strcpy(LectInfo[1],L_("Entretien detecteurs termine"));
		if(OpiumAlEcran(pLectRegen)) PanelRefreshVars(pLectRegen);
		if(doit_terminer) OpiumRefreshEnd(bLecture);
		if(OpiumDisplayed(bLecture)) OpiumRefresh(bLecture);

		if(mCalcSpectreControle->cdr) {
			if(OpiumDisplayed((mCalcSpectreControle->cdr)->planche))
				doit_terminer = OpiumRefreshBegin((mCalcSpectreControle->cdr)->planche);
			else doit_terminer = 0;
			MenuItemAllume(mCalcSpectreControle,1,L_("Mesurer  "),GRF_RGB_YELLOW);
			MenuItemEteint(mCalcSpectreControle,2,0);
			MenuItemEteint(mCalcSpectreControle,3,0);
			if(LectModeSpectresAuto && !erreur_acq) MenuItemAllume(mCalcSpectreControle,4,L_("Sauver   "),GRF_RGB_YELLOW);
			if(doit_terminer) OpiumRefreshEnd((mCalcSpectreControle->cdr)->planche);
		}
		LectBranche(1);

		/* Traitements de fin de la tache de la lecture */
		TrmtStop();
		Acquis[AcquisLocale].etat.status = SATELLITE_ATTENTE;
		/* Terminaison de l'archivage */
		LectTermineStocke(0,0); /* vidage=0, sinon on ne peut pas les visualiser apres la fin du run */
		if(Archive.enservice) {
			for(fmt=0; fmt<ARCH_TYPEDATA; fmt++) if(ArchSauve[fmt]) {
				if((f = ArchiveAccede(fmt))) {
					k = (int)ftell(f);
					printf("%s/ Les %s s'arretent dans le fichier a l'octet %d (0x%04x)\n",DateHeure(),ArchFmtNom[fmt],k,k);
					fprintf(f,"* Arret ");
					if(erreur_acq) fprintf(f,"sur erreur: %s\n",ArchExplics); else fprintf(f,"normal\n");
					ArchT0sec = LectDateRun.tv_sec;
					ArchT0msec = LectDateRun.tv_usec;
					ArgAppend(f,0,ArchHdrFin);
					fprintf(f,FIN_DE_BLOC);
					fclose(f);
				}
				printf("%s/ Sauvegarde des %s terminee\n",DateHeure(),ArchFmtNom[fmt]);
				if(stat(ArchiveFichier[fmt],&infos) != -1) ArchiveEcrits[fmt] += (float)infos.st_size / (float)(MB);
			}
		}
		/* Normalisation des patterns */
		for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue && VoieTampon[voie].pattern.dim) {
			if(VoieManip[voie].duree.periodes > 1) {
				for(i=0; i<VoieTampon[voie].pattern.max; i++) {
					VoieTampon[voie].pattern.val[i] /= VoieTampon[voie].pattern.nb[i];
					//				printf("nb.%s[%d] = %g / %g\n",VoieManip[voie].nom,i,VoieTampon[voie].pattern.nb[i],VoieManip[voie].duree.periodes);
				}
                printf("%s/ Pattern de la voie %s: normalise\n",DateHeure(),VoieManip[voie].nom);
			}
		}
		/* Normalisation des evenements moyens calcules */
		for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
			if(VoieTampon[voie].somme.val && (VoieTampon[voie].moyen.nb > 1)) {
				nb = (float)VoieTampon[voie].moyen.nb;
				for(i=0; i<VoieTampon[voie].somme.taille; i++)
					VoieTampon[voie].somme.val[i] /= nb;
				VoieTampon[voie].moyen.decal /= nb;
				VoieTampon[voie].moyen.nb = 1;
                printf("%s/ Evenement moyen de la voie %s: normalise\n",DateHeure(),VoieManip[voie].nom);
			}
		}
		/* Debriefing */
		gettimeofday(&LectDateRunFin,0);
		LectCpuRunFin = clock();
		if(getrusage(RUSAGE_SELF,&StatsSystemeFin) < 0) {
			printf("%s/ Statistiques systeme indisponibles (%s)\n",DateHeure(),strerror(errno));
			LectStatsValides = 0;
		}
		//-		printf("%s/ Fin d'acquisition, CPU utilise: %ld ticks\n",DateHeure(),LectCpuRunFin);
		LectJournalLaFin(erreur_acq,1,1);
		if(!LectAutreSequence) LogOffFile();
		/* Terminaison de l'affichage des donnees brutes */
		LectDisplay();
	} while(LectAutreSequence);
	if(Archive.enservice) {
		int64 total_simule;
		int hh,mn,ss,sec_simule;
		total_simule = (int64)((float)(LectStampPrecedents + LectStampsLus - TrmtPasActif) / (Echantillonnage / 1000.0));
		sec_simule = (int)(total_simule / 1000000);
		mn = sec_simule / 60;
		ss = sec_simule - (mn * 60);
		hh = mn / 60;
		mn = mn - (hh * 60);
		sprintf(ArchDuree,"%02dh%02d'%02d",hh,mn,ss);
		ArchEvtTot = Acquis[AcquisLocale].etat.evt_trouves;
		if(ArchSauve[STRM]) {
			if(ArchSauve[EVTS]) SambaNote(". Fin du run %s apres %s, ecrit: %.1f Mb (%d evenement%s) plus %.1f Mb de donnees brutes\n",
				Acquis[AcquisLocale].etat.nom_run,ArchDuree,ArchiveEcrits[EVTS],Accord1s(ArchEvtTot),ArchiveEcrits[STRM]);
			else SambaNote(". Fin du run %s apres %s, ecrit: %.1f Mb de donnees brutes\n",
				Acquis[AcquisLocale].etat.nom_run,ArchDuree,ArchiveEcrits[STRM]);
		} else if(ArchSauve[EVTS]) SambaNote(". Fin du run %s apres %s, ecrit: %.1f Mb (%d evenement%s)\n",
			Acquis[AcquisLocale].etat.nom_run,ArchDuree,ArchiveEcrits[EVTS],Accord1s(ArchEvtTot));
		/* Enregistrement au catalogue */
		if(ArchSauve[STRM] || ArchSauve[EVTS]) {
			FILE *f;
			f = fopen(FichierCatalogue,"r");
			if(!f) {
				RepertoireCreeRacine(FichierCatalogue);
				f = fopen(FichierCatalogue,"w");
				if(f) fprintf(f,"# Date    heure   nom       mode  type    duree  taille(MB)  expo(Kg.j)   evts\n");
			}
			if(f) {
				int car; char *type;
				fseek(f,-1,SEEK_END);
				car = fgetc(f);
				fclose(f);
				if(car == '\n') {
					f = fopen(FichierCatalogue,"a");
					if(f) {
						printf("%s/ Debut de ligne du catalogue absente\n",DateHeure());
						if((SettingsRunFamille == RUN_FAMILLE_BANC) && (RunTypeName >= 0)) {
							EnvirVarPrintVal(&(EnvirVar[RunTypeVar]),RunTypeName,TYPERUN_NOM);
							type = RunTypeName;
						} else {
							ArgKeyGetText(RunCategCles,LectCntl.RunCategNum,RunCategName,TYPERUN_NOM);
							type = RunCategName;
						}
						fprintf(f,"%8s    --    %-15s %-6s %s",DateJour(),Acquis[AcquisLocale].etat.nom_run,
							Trigger.enservice? "event": "stream",type);
						fclose(f);
					}
				}
			}
			f = fopen(FichierCatalogue,"a");
			if(f) {
				int sec_simule,usec_simule; int hh,mn,ss; char diese; // char condition[32];
				diese = 0;
				SambaTempsEchantillon(LectStampPrecedents + LectStampsLus - TrmtPasActif,0,0,&sec_simule,&usec_simule);
				mn = sec_simule / 60; ss = sec_simule - (mn * 60); hh = mn / 60; mn = mn - (hh * 60);
				fprintf(f," %02dh%02d'%02d %9.1f   %9.4f",hh,mn,ss,ArchiveEcrits[EVTS],Acquis[AcquisLocale].etat.KgJ);
				if(Trigger.enservice) fprintf(f," %6d",Acquis[AcquisLocale].etat.evt_trouves); else fprintf(f,"     --");
				// ArgKeyGetText(RunEnvir,LectCntl.SettingsRunEnvr,condition,32);
				// fprintf(f," # %s",condition);
				if(EnvirVarNb) {
					int j;
					fprintf(f," # "); diese = 1; j = 0;
					for(i=0; i<EnvirVarNb; i++) if(i != RunTypeVar) {
						EnvirVarPrintVal(&(EnvirVar[i]),texte,80);
						if(j++) fprintf(f,", ");
						fprintf(f,"%s=%s",EnvirVar[i].nom,texte);
					}
				}
				for(k=0; k<ExportPackNb; k++) if((ExportPack[k].support_type == EXPORT_CATALOGUE) && (ExportPack[k].quand == EXPORT_RUN_FIN)) {
					if(!diese) { fprintf(f," # "); diese = 1; }
					ExporteInfos(f,k);
				}
				if(LectCommentComplet[0]) fprintf(f," / %s",LectCommentComplet);
				fprintf(f,"\n");
				fclose(f);
			}
		}
		if(LoggerAdrs[0]) {
			if(LoggerType == CAHIER_ELOG) {
				sprintf(commande,"%s -a \"Subject=Run Stop\" \"Stopped [COLOR=%s][B]%s[/B][/COLOR] after %02dh%02d'%02d (%d events, %.1f Mb)\" &",
					LoggerCmde,couleur,Acquis[AcquisLocale].etat.nom_run,hh,mn,ss,Trigger.enservice? Acquis[AcquisLocale].etat.evt_trouves: 0,ArchiveEcrits[EVTS]);
				system(commande);
			} else {
				FILE *f;
				if((f = fopen(LoggerAdrs,"a"))) {
					fprintf(f,"                      ");
					fprintf(f,"time: %02dh%02d %02d\" (%'d events, %.1f Mb)\n",hh,mn,ss,Trigger.enservice? Acquis[AcquisLocale].etat.evt_trouves: 0,ArchiveEcrits[EVTS]);
					fclose(f);
				}
			}
		}
		if(EdbActive) {
			FILE *f;
			f = fopen("RunInfo","r"); if(f) { fclose(f); remove("RunInfo"); }
			ArchInfoStatus = erreur_acq? ARCH_STATUS_ERREUR: ARCH_STATUS_TERMINE;
			for(fmt=0; fmt<ARCH_TYPEDATA; fmt++) ArchiveNouvellePartition(fmt);
		}
	}
	LectModeSpectresAuto = 0;

	if(!LectArretePoliment && LectStoppeRepart) for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) RepartIpTransmission(&(Repart[rep]),REPART_ENVOI_DETEC,-1,LectureLog);
	/* mail -s "Samba alert" -c juillard@csnsm.in2p3.fr michel.gros@cea.fr */
	if(erreur_acq) OpiumFail("Erreur %04X (%s), %s est stoppee.",erreur_acq,ArchExplics,LectSrceTexte);
	else BancEnErreur = 0;

	return(0);
}
/* ========================================================================== */
void LectAcqStdTermine() {
}
#ifdef SPECTRES_SEQUENCES
/* ========================================================================== */
int LectSpectresClassiques() {
	
	if(!CalcSpectreAutoParms()) return(0);
	CalcGlissantEpsilon = 1.0 / (float)CalcSpectresMax;

	/* Panel jamais interroge...
	int bolo,i;
	if(pCalcSpectreAffiche) PanelErase(pCalcSpectreAffiche);
	else pCalcSpectreAffiche = PanelCreate(BoloNb);
	for(bolo=0; bolo<BoloNb; bolo++) if(CalcAutoBolo[bolo]) {
		CalcAutoAffiche[bolo] = 1;
		i = PanelBool(pCalcSpectreAffiche,Bolo[bolo].nom,&CalcAutoAffiche[bolo]);
		PanelItemColors(pCalcSpectreAffiche,i,SambaRougeVert,2);
	} */

	LectModeSpectresAuto = 1;
	OpiumFork(mCalcSpectreAutoEnCours->cdr);
	gettimeofday(&CalcDateSpectre,0);
	LectAcqStd();
	LectModeSpectresAuto = 0;
	if(OpiumDisplayed(mCalcSpectreAutoEnCours->cdr)) OpiumClear(mCalcSpectreAutoEnCours->cdr);
	OpiumExec(mCalcSpectreAutoFinis->cdr);
	return(0);
}
#endif /* SPECTRES_SEQUENCES */
/* ========================================================================== */
int LectSpectresAutoMesure(Menu menu, MenuItem item) {
	char affiche_evts,doit_terminer;

#ifdef SPECTRES_AUTO_ALLOC_DYN
	if(Acquis[AcquisLocale].etat.active) LectStop();
#else
	if(!LectSpectreTampons()) return(0);
#endif
	LectModeSpectresAuto = 1;
	if(Acquis[AcquisLocale].etat.active) {
		doit_terminer = OpiumRefreshBegin((mCalcSpectreControle->cdr)->planche);
		MenuItemAllume(mCalcSpectreControle,1,"Mesure   ",GRF_RGB_GREEN);
		MenuItemAllume(mCalcSpectreControle,2,0,GRF_RGB_YELLOW);
		MenuItemAllume(mCalcSpectreControle,3,0,GRF_RGB_YELLOW);
		MenuItemEteint(mCalcSpectreControle,4,0);
		if(doit_terminer) OpiumRefreshEnd((mCalcSpectreControle->cdr)->planche);
	} else {
		affiche_evts = MonitAffEvts; MonitAffEvts = 0;
		CalcSpectreAutoEfface();
		LectAcqStd();
		// while(Acquis[AcquisLocale].etat.active) usleep(100000);
		MonitAffEvts = affiche_evts;
	}
	if(!LectCompacteUtilisee) CalcSpectreAutoAffiche();
	
	return(0);
}
#ifdef SPECTRES_COMPACTE
/* ========================================================================== */
int LectCompacteSpectres(Menu menu, MenuItem item) {
	char affiche_evts;
	
#ifdef SPECTRES_AUTO_ALLOC_DYN
	if(Acquis[AcquisLocale].etat.active) LectStop();
#else
	if(!LectSpectreTampons()) return(0);
#endif
	LectModeSpectresAuto = 1;
	if(Acquis[AcquisLocale].etat.active) {
		MenuItemAllume(mLectSpectreControle,1,"Mesure  ",GRF_RGB_GREEN);
		MenuItemAllume(mLectSpectreControle,2,0,GRF_RGB_YELLOW);
		MenuItemEteint(mLectSpectreControle,3,0);
	} else {
		affiche_evts = MonitAffEvts; MonitAffEvts = 0;
		if(bLecture) {
			if(OpiumDisplayed(bLecture)) OpiumClear(bLecture);
			BoardTrash(bLecture);
		}
		CalcSpectreAutoEfface();
		LectAcqStd();
		MonitAffEvts = affiche_evts;
	}
	
	return(0);
}
#endif /* SPECTRES_COMPACTE */
/* ========================================================================== */
int LectVI() {
	int bolo,cap,voie,v,nv;
	TypeReglage *rampl,*rcomp; // ,*rcorr;
	int k; OpiumColorState couleur[DETEC_MAX];

	if(!DetecteurSelectionne()) return(0);
    if(OpiumExec(pReglViAuto->cdr) == PNL_CANCEL) return(0);
	printf("* Mesure des V(I) de %gV a %gV par %s %gV\n",ReglVi.dep,ReglVi.arr,ReglVi.oper?"facteur":"pas de",ReglVi.pas);
	ReglVi.cur = ReglVi.dep;
	ReglVi.nb_mesures = 0;
	ReglVi.nb_voies = 0;
	for(bolo=0; bolo<BoloNb; bolo++) if(ReglVi.g[bolo]) { GraphDelete(ReglVi.g[bolo]); ReglVi.g[bolo] = 0; }
	for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire && Bolo[bolo].sel) {
		nv = 0;
		for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
			if((voie = Bolo[bolo].captr[cap].voie) < 0) continue;
			if(VoieManip[voie].modulee) {
				if(!(rampl = ReglagePtr(voie,REGL_MODUL))) {
					printf("  ! Pas de reglage de modulation sur la voie %s\n",VoieManip[voie].nom);
					continue;
				}
				if(!(rcomp = ReglagePtr(voie,REGL_COMP))) {
					printf("  ! Pas de reglage de compensation sur la voie %s\n",VoieManip[voie].nom);
					continue;
				}
				nv++;
				ReglageChangeFloat(rampl,ReglVi.cur,0);
				ReglageChangeFloat(rcomp,ReglVi.cur,0);
				// laisser ce reglage a une valeur passe-partout: if((rcorr = ReglagePtr(voie,REGL_CORR))) ReglageChangeFloat(rcorr,0.0,0);
				ReglVi.voie[ReglVi.nb_voies].num = voie;
				ReglVi.nb_voies++;
			}
		}
		if(nv > 0) {
			ReglVi.g[bolo] = GraphCreate(300,300,2*nv);
			OpiumTitle(ReglVi.g[bolo]->cdr,"V(I)");
			GraphUse(ReglVi.g[bolo],0);
			OpiumColorInit(&(couleur[bolo]));
		}
	}
	if(!ReglVi.nb_voies) {
		printf("  ! Pas de voie candidate\n");
		OpiumWarn("Pas de voie candidate");
	} else {
		printf("  . %d voie%s:\n",ReglVi.nb_voies,(ReglVi.nb_voies>1)?"s":"");
		for(v=0; v<ReglVi.nb_voies; v++) printf("   | voie %s\n",VoieManip[ReglVi.voie[v].num].nom);
		for(v=0; v<ReglVi.nb_voies; v++) {
			k = OpiumColorNext(&(couleur[bolo])); k = OpiumColorNext(&(couleur[bolo]));
			voie = ReglVi.voie[v].num; bolo = VoieManip[voie].det;
			GraphAdd(ReglVi.g[bolo],GRF_FLOAT|GRF_XAXIS,ReglVi.voie[v].I,MAXVI);
			GraphDataRGB(ReglVi.g[bolo],GraphAdd(ReglVi.g[bolo],GRF_FLOAT|GRF_YAXIS,ReglVi.voie[v].V,MAXVI),OpiumColorRGB(k));
		}
		SambaAttends(ReglVi.attente*1000);
		ReglVi.actif = 1;
		if(!Acquis[AcquisLocale].etat.active) {
			ReglVi.arrete = 1;
			// LectAcqElementaire(NUMER_MODE_ACQUIS);
			LectAcqStd();
		} else ReglVi.arrete = 0;
	}

	return(0);
}
/* ========================================================================== */
int LectRT() {
#ifdef A_REVOIR_COMPLETEMENT
	strcat(strcat(strcpy(fichier,ResuPath),Acquis[AcquisLocale].etat.nom_run),".RT");
	RepertoireCreeRacine(fichier);
	f = fopen(fichier,"w");
	if(!f) {
		OpiumError("Sauvegarde des courbes R(T) impossible (%s)",strerror(errno));
		if(!OpiumChoix(2,SambaNonOui,"On continue quand meme?")) return(0);
	} else fclose(f);
	ecrit = 0;

	LogBlocBegin();
	printf(" __________________________________________________________________________\n");
	printf("|                                   R(T)                                   |\n");
	printf("|__________________________________________________________________________|\n");
	printf("|          Voie            |       T (K)   |      V (mV)   |      R (MO)   |\n");
	printf("|__________________________|_______________|_______________|_______________|\n");
	LectRtEnCours = 1;
	nb = 0;
	do {
		/* Compenser, ici, 1 fois */
		f = fopen(fichier,"a");
		for(vt = 0; vt<SambaEchantillonLngr; vt++) if((voie = SambaEchantillon[vt].voie) >= 0) {
			if(VoieManip[voie].modulee && VoieTampon[voie].lue) {
				if(resultat.ok) {
					TypeReglage *rcomp;
					if(VoieManip[voie].Rmodul != 0.0) VoieInfo[voie].I = 1000.0 *  amplitude_compensation / VoieManip[voie].Rmodul;
					else VoieInfo[voie].I = 0.0;
					ReglIbolo = VoieInfo[voie].I;
					hval = (TypeADU)compensation.valeur.courante;
					vval = hval en millivolts;
					VoieInfo[voie].V = vval / VoieManip[voie].gain_fixe;
					ReglVbolo = VoieInfo[voie].V;
					if(VoieInfo[voie].I != 0.0) VoieInfo[voie].R = VoieInfo[voie].V / VoieInfo[voie].I;
					else VoieInfo[voie].R = 0.0;
					ReglRbolo = VoieInfo[voie].R;
					ReglTbolo = *T_Bolo;
					printf("| %-24s | %12.3f  | %12.3f  | %12.3f  |\n",VoieManip[voie].nom,ReglTbolo,VoieInfo[voie].V,VoieInfo[voie].R);
					if(f) {
						fprintf(f,"%-24s %10.3f %10.3f %10.3f\n",VoieManip[voie].nom,ReglTbolo,VoieInfo[voie].V,VoieInfo[voie].R);
						ecrit = 1;
					}
					if((rcomp = ReglagePtr(voie,REGL_COMP))) {
						rcomp->hval = (TypeADU)compensation.valeur.courante;
						rcomp->rval = rcomp->hval // en volts pour RESS_COMP;
					}
				}
			}
		}
		if(f) fclose(f);
		prochain = SambaSecondes() + (long)LectRtPas;
		do { OpiumUserAction(); SambaAttends(500); } while((SambaSecondes() < prochain) && LectRtEnCours);
	} while((nb++ < LectRtNb) && LectRtEnCours);
	printf("|__________________________|_______________|_______________|_______________|\n\n");
	if(ecrit) printf(". Courbes R(T) sauvees sur '%s'\n",fichier);
	else printf("* Courbes R(T) NON sauvegardees\n");
	LogBlocEnd();
	LectRtEnCours = 0;
#endif
	return(0);
}
/* ========================================================================== */
int LectCompenseTout() {
	int voie,cap,bolo,bb,k; float moyenne,demi_mesure_enV,rval; // VoieTampon[voie].signal.moyenne == moyenne
	TypeReglage *rcomp; TypeBNModele *modele_bn; TypeBNModlRessource *ress;
	int bb_autorise;
	char log;

	log = 0; bb_autorise = -1;
	for(voie=0; voie<VoiesNb; voie++) if((VoieTampon[voie].lue > 0) && VoieManip[voie].modulee) {
		if(!(rcomp = ReglagePtr(voie,REGL_COMP))) {
			printf("%s/ Pas de compensation sur la voie %s\n",DateHeure(),VoieManip[voie].nom);
			continue;
		}
		if(VoieTampon[voie].signal.nb && VoieManip[voie].def.trmt[TRMT_AU_VOL].gain) 
			moyenne = VoieTampon[voie].signal.somme / (float)VoieTampon[voie].signal.nb / (float)VoieManip[voie].def.trmt[TRMT_AU_VOL].gain;
		else moyenne = 0.0;
		if(log) printf("(%s) Moyenne(%s) = %g/%d = %g\n",__func__,VoieManip[voie].nom,VoieTampon[voie].signal.somme,VoieTampon[voie].signal.nb,moyenne);
		demi_mesure_enV = COMP_COEF * moyenne * VoieManip[voie].ADUenV;
		if((cap = rcomp->capt) < 0) cap = 0;
		if((bolo = VoieManip[voie].det) < 0) bolo = 0;
		bb = Bolo[bolo].captr[cap].bb.num;
		k = rcomp->ress;
		modele_bn = &(ModeleBN[Numeriseur[bb].def.type]); ress = modele_bn->ress + k;
		if(ress->type == RESS_FLOAT) {
			rval = Numeriseur[bb].ress[k].val.r;
			if(log) printf("(%s) <%s> = %g V, reglage = %d V\n",__func__,VoieManip[voie].nom,demi_mesure_enV,rval);
			/* les excitations (a en deduire) seront a donner en amplitude et non crete-crete */
			Numeriseur[bb].ress[k].val.r = demi_mesure_enV + rval;
			if(log) printf("(%s) donc <%s>.%s = %g V\n",__func__,Numeriseur[bb].nom,ress->nom,Numeriseur[bb].ress[k].val.r);
		} else {
			rval = (float)Numeriseur[bb].ress[k].val.i;
			/* les excitations (a en deduire) seront a donner en amplitude et non crete-crete */
			Numeriseur[bb].ress[k].val.i = (int)(demi_mesure_enV + rval);
		}
		NumeriseurRessValChangee(&(Numeriseur[bb]),k);
		if(bb != bb_autorise) {
			if(bb_autorise >= 0) NumeriseurAutoriseAcces(&(Numeriseur[bb_autorise]),0,0);
			NumeriseurAutoriseAcces(&(Numeriseur[bb]),1,0);
			bb_autorise = bb;
		}
		NumeriseurRessourceCharge(&(Numeriseur[bb]),k,"*");
		ReglageFinalise(rcomp);
		strcpy(rcomp->std,rcomp->user);
		if(log) printf("(%s) .. soit %s(%s) = %s\n",__func__,Bolo[bolo].nom,ModeleDet[Bolo[bolo].type].regl[rcomp->num].nom,rcomp->std);
	}
	if(bb_autorise >= 0) NumeriseurAutoriseAcces(&(Numeriseur[bb_autorise]),0,0);
	return(0);
}
#ifdef FUTURISTE
/* ========================================================================== */
void LectLanceStream() {
	int rc;

	if(rc = RepertoireAbsent(ArchZone[STRM])) {
		printf("* La creation du repertoire %s retourne %d (%s/ errno=%d)\n",
			   ArchZone[STRM],rc,strerror(rc),errno);
		OpiumError("LA SAUVEGARDE EST ABANDONNEE");
		ArchSauve[STRM] = 0;
		// if(OpiumDisplayed(cArchive->cdr)) OpiumRefresh(cArchive->cdr);
		// if(OpiumDisplayed(pLectMode->cdr)) OpiumRefresh(pLectMode->cdr);
	}
	sprintf(ArchiveChemin[STRM],"%s%s",ArchZone[STRM],Acquis[AcquisLocale].etat.nom_run);
	if(ArchiveSurRepert && RepertoireAbsent(ArchiveChemin[STRM])) {
		sprintf(commande,"mkdir %s",ArchiveChemin[STRM]);
		errno = 0;
		if(rc = system(commande)) {
			printf("* La creation de ce repertoire retourne %d (%s/ errno=%d)\n",
				   rc,strerror(rc),errno);
			ArchiveSurRepert = 0;
			LectDureeActive = 0;
			// if(OpiumDisplayed(pLectHachoir->cdr)) PanelRefreshVars(pLectHachoir);
			if(OpiumDisplayed(bSauvegardes)) OpiumRefresh(bSauvegardes);
		}
	}
}
/* ========================================================================== */
void LectLanceEvents() {
	int rc;
	
	if(rc = RepertoireAbsent(ArchZone[EVTS])) {
		printf("* La creation du repertoire %s retourne %d (%s/ errno=%d)\n",
			   ArchZone[EVTS],rc,strerror(rc),errno);
		OpiumError("LA SAUVEGARDE EST ABANDONNEE");
		ArchSauve[EVTS] = 0;
		// if(OpiumDisplayed(cArchive->cdr)) OpiumRefresh(cArchive->cdr);
		// if(OpiumDisplayed(pLectMode->cdr)) OpiumRefresh(pLectMode->cdr);
	}
	sprintf(ArchiveChemin[EVTS],"%s%s",ArchZone[EVTS],Acquis[AcquisLocale].etat.nom_run);
	if(ArchiveSurRepert && RepertoireAbsent(ArchiveChemin[EVTS])) {
		sprintf(commande,"mkdir %s",ArchiveChemin[EVTS]);
		errno = 0;
		if(rc = system(commande)) {
			printf("* La creation de ce repertoire retourne %d (%s/ errno=%d)\n",
				   rc,strerror(rc),errno);
			ArchiveSurRepert = 0;
			LectDureeActive = 0;
			// if(OpiumDisplayed(pLectHachoir->cdr)) PanelRefreshVars(pLectHachoir);
			if(OpiumDisplayed(bSauvegardes)) OpiumRefresh(bSauvegardes);
		}
	}
}
#endif
/* ========================================================================== */
void LectRegenAffiche(char termine, char avec_source) {
	if(OpiumDisplayed(mLectRegen->cdr)) {
		MenuItemEnable(mLectRegen,1); OpiumRefresh(mLectRegen->cdr); OpiumUserAction();
	}
	if(RegenEnCours) {
		if(termine) {
			if(avec_source) {
				sprintf(LectInfo[0],L_("Regeneration depuis le %s a %s"),RegenDebutJour,RegenDebutHeure);
				printf("%s/ Regeneration en cours\n",DateHeure());
//				MenuItemAllume(mLectRegen,1," Stopper regeneration  ",GRF_RGB_YELLOW);
				MenuItemAllume(mLectRegen,1,L_(" Regeneration en cours "),GRF_RGB_YELLOW);
			} else {
				sprintf(LectInfo[0],L_("Polarisations a 0 depuis le %s a %s"),RegenDebutJour,RegenDebutHeure);
				printf("%s/ Detecteurs mis en securite\n",DateHeure());
//				MenuItemAllume(mLectRegen,1,"      Repolariser      ",GRF_RGB_YELLOW);
				MenuItemAllume(mLectRegen,1," Regeneration en cours ",GRF_RGB_YELLOW);
			}
		} else {
			strcpy(LectInfo[0],L_("Polarisations en cours"));
			printf("%s/ Polarisations en cours\n",DateHeure());
			MenuItemAllume(mLectRegen,1,L_(" Polarisation demandee "),GRF_RGB_ORANGE);
		}
	} else {
		if(termine) {
			strcpy(LectInfo[0],L_("Polarisations etablies"));
			printf("%s/ Polarisations etablies\n",DateHeure());
			MenuItemAllume(mLectRegen,1,L_(" Lancer  regeneration  "),GRF_RGB_GREEN);
		} else {
			if(avec_source) {
				sprintf(LectInfo[0],L_("Regeneration demandee le %s a %s"),RegenDebutJour,RegenDebutHeure);
				printf("%s/ Regeneration demandee\n",DateHeure());
				MenuItemAllume(mLectRegen,1,L_("Demarrage regeneration "),GRF_RGB_ORANGE);
			} else {
				sprintf(LectInfo[0],L_("Polarisations a 0 demandees le %s a %s"),RegenDebutJour,RegenDebutHeure);
				printf("%s/ Securisation en cours\n",DateHeure());
				MenuItemAllume(mLectRegen,1,L_("Demarrage securisation "),GRF_RGB_ORANGE);
			}
		}
	}
	if(OpiumDisplayed(mLectRegen->cdr)) {
		OpiumRefresh(mLectRegen->cdr); OpiumUserAction();
		if(RegenEnCours || !termine) MenuItemDisable(mLectRegen,1);
	}
	if(OpiumDisplayed(pLectRegen->cdr)) { PanelRefreshVars(pLectRegen); OpiumUserAction(); }
}
/* ========================================================================== */
void LectRegenTermine(char regen) {
	int fmt;
	
	AutomVerifieConditions("",0);
	if(regen >= 0) RegenEnCours = regen;
	ArgPrint(FichierEtatElec,LectEtatDesc);
	LectRegenAffiche(1,0);
	if(ArchEcrites[EVTS]) LectRegenNum++;
	for(fmt=0; fmt<ARCH_TYPEDATA; fmt++) {
		ArchiveOuverte[fmt] = 0; ArchEcrites[fmt] = 0;
	}
}
/* ========================================================================== */
void LectRegenLance(char avec_source) {
	int bolo,bb_autorise;
	int total,reste; int i;
	TypeReglage *regl;
	char fait;
	//- char prefixe[80];
#ifdef A_VIRER
	char active;
	char raison[80];
#endif
	
	/*
	 * Lancer la regeneration
	 */
	if(RegenEnCours || !SettingsRegen) return;
	// if(!strcmp(NomRegenStart,"neant")) return; ou bien on fait la procedure cablee?
	LectRegenAffiche(0,avec_source);
	//+ BoardReplace(bLecture,mLectRegen->cdr,pScriptWait->cdr);
	fait = 0;
	if(FichierRegenStart[0] && strcmp(NomRegenStart,"neant")) fait = ScriptExec(FichierRegenStart,NomRegenStart,"");
	if(!fait) {
		total = 0;
		for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire) total += 1;
		reste = total;
		if(avec_source) {
			printf("%s/ Demarrage de la regeneration ...\n",DateHeure());
			sprintf(LectInfo[0],L_("Demarrage regeneration, encore %d s"),reste);
		} else {
			printf("%s/ Mise en securite des detecteurs ...\n",DateHeure());
			sprintf(LectInfo[0],L_("Mise en securite, encore %d s"),reste);
		}
		if(OpiumDisplayed(pLectRegen->cdr)) { PanelRefreshVars(pLectRegen); OpiumUserAction(); }
		bb_autorise = -1;
		printf("%s/ Polarisations des voies ionisation a 0\n",DateHeure());
		for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire) {
			for(i=0; i<Bolo[bolo].nbreglages; i++) {
				regl = &(Bolo[bolo].reglage[i]);
				if((regl->cmde == REGL_POLAR) || (regl->cmde == REGL_FET_CC)) break;
			}
			if(i >= Bolo[bolo].nbreglages) continue;
			for(i=0; i<Bolo[bolo].nbreglages; i++) {
				regl = &(Bolo[bolo].reglage[i]);
				if(regl->cmde == REGL_POLAR) ReglageChargeFlottant(&(Bolo[bolo]),regl,&bb_autorise,"          ",0.0);
				else if(regl->cmde == REGL_FET_CC) ReglageChargeFlottant(&(Bolo[bolo]),regl,&bb_autorise,"          ",0.0);
			}
			--reste;
		}
		ReglageChargeTerminee(bb_autorise);
		LectSourceActive = AutomSourceDeplace(AUTOM_REGEN,1,avec_source,"");
	}
	//+ BoardReplace(bLecture,pScriptWait->cdr,mLectRegen->cdr);
	LectRegenTermine(1);
}
/* ========================================================================== */
void LectRegenStoppe(char avec_dialogue) {
	int bolo,bb_autorise,i;
	int total,reste,duree_raz,ecoule;
	TypeReglage *regl;
	char fait,source_retiree,reglage_en_cours; // ,long_raz
#ifdef A_VIRER
	char active;
	char raison[80];
#endif
	//- char prefixe[80];

	/*
	 * Arreter la regeneration et remettre les polars
	 */
	if(!RegenEnCours || !SettingsRegen) return;
	// if(!strcmp(NomRegenStop,"neant")) return; ou bien on fait la procedure cablee?
	LectRegenAffiche(0,0);
	fait = 0;
	if(FichierRegenStop[0] && strcmp(NomRegenStop,"neant")) fait = ScriptExec(FichierRegenStop,NomRegenStop,"");
	if(!fait) {
		char affiche;
		affiche = OpiumDisplayed(pLectRegen->cdr);
		source_retiree = 0;
		total = 0;
		LectSourceActive = AutomSourceDeplace(AUTOM_REGEN,0,1,"");
		// long_raz = 0;
		duree_raz = 0;
		for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire) {
			total += 1;
		//	if(!long_raz) for(i=0; i<Bolo[bolo].nbreglages; i++) if(Bolo[bolo].reglage[i].cmde == REGL_FET_CC) { long_raz = 1; break; }
			if(ModeleDet[Bolo[bolo].type].duree_raz > duree_raz) duree_raz = ModeleDet[Bolo[bolo].type].duree_raz;
		}
		// if(long_raz) duree_raz = (SettingsRazNbSiDuree > SettingsRazNtdDuree)? SettingsRazNbSiDuree: SettingsRazNtdDuree;
		// else duree_raz = SettingsRazNtdDuree;
		total += duree_raz;
		if(total < SettingsRegenDelai) total = SettingsRegenDelai;
		reste = total;
		printf("%s/ Polarisation des voies ionisation, et RAZ detecteurs, en cours ...\n",DateHeure());
		sprintf(LectInfo[0],L_("Polarisation en cours, encore %d s"),reste);
		if(affiche) { PanelRefreshVars(pLectRegen); OpiumUserAction(); }
		bb_autorise = -1;
		for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire) {
			for(i=0; i<Bolo[bolo].nbreglages; i++) {
				regl = &(Bolo[bolo].reglage[i]);
				if((regl->cmde == REGL_POLAR) || (regl->cmde == REGL_FET_MASSE) || (regl->cmde == REGL_RAZFET)) break;
			}
			if(i >= Bolo[bolo].nbreglages) continue;
			for(i=0; i<Bolo[bolo].nbreglages; i++) {
				regl = &(Bolo[bolo].reglage[i]);
				if(regl->cmde == REGL_POLAR) ReglageChargeStandard(&(Bolo[bolo]),regl,&bb_autorise,"         ");
				else if(regl->cmde == REGL_RAZFET) DetecteurRazFet(&(Bolo[bolo]),regl,&bb_autorise,"         ",1);
				else if(regl->cmde == REGL_FET_MASSE) ReglageChargeUser(&(Bolo[bolo]),regl,&bb_autorise,"         ","masse"); // ReglageChargeHexa(&(Bolo[bolo]),regl,&bb_autorise,"         ",0x0004);
			}
			--reste;
		}
		printf("%s/ Attente de %d secondes\n",DateHeure(),duree_raz);
		if(avec_dialogue) { OpiumProgresInit(duree_raz); ecoule = 0; }
		while(duree_raz--) {
			sprintf(LectInfo[0],L_("Polarisation en cours, encore %d s"),reste);
			if(affiche) { PanelRefreshVars(pLectRegen); OpiumUserAction(); }
			SambaAttends(1000);
			if(avec_dialogue) { if(!OpiumProgresRefresh(++ecoule)) break; }
			--reste;
		}
		if(avec_dialogue) OpiumProgresClose();
		if(duree_raz > 0) printf("          . Attente reduite a %d secondes\n",ecoule);
		for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire) {
			for(i=0; i<Bolo[bolo].nbreglages; i++) {
				for(i=0; i<Bolo[bolo].nbreglages; i++) {
					regl = &(Bolo[bolo].reglage[i]);
					if((regl->cmde == REGL_FET_MASSE) || (regl->cmde == REGL_FET_CC) || (regl->cmde == REGL_RAZFET)) break;
				}
				if(i >= Bolo[bolo].nbreglages) continue;
				for(i=0; i<Bolo[bolo].nbreglages; i++) {
					regl = &(Bolo[bolo].reglage[i]);
					if(regl->cmde == REGL_RAZFET) DetecteurRazFet(&(Bolo[bolo]),regl,&bb_autorise,"         ",0);
					else if(regl->cmde == REGL_FET_MASSE) {
						if(Bolo[bolo].razfet_en_cours) printf("            |                         %8s.FET          inchange\n",Bolo[bolo].nom);
						else ReglageChargeUser(&(Bolo[bolo]),regl,&bb_autorise,"         ","libre"); // ReglageChargeHexa(&(Bolo[bolo]),regl,&bb_autorise,"          ",0x0000);
					} else if(regl->cmde == REGL_FET_CC) ReglageChargeFlottant(&(Bolo[bolo]),regl,&bb_autorise,"          ",-10.0); /* anciennement, +10.0 */
				}
			}
		}
		ReglageChargeTerminee(bb_autorise);
		if(reste > 0) {
			if(source_retiree) {
				printf("%s/ Attente finale de %d secondes\n",DateHeure(),reste);
				ecoule = 0;
				if(avec_dialogue) OpiumProgresInit(reste);
				AttenteFinRegen = 1;
				reglage_en_cours = (OpiumDisplayed(OscilloReglage[OscilloNum]->ecran->cdr) && OscilloReglage[OscilloNum]->acq);
				while(reste--) {
					sprintf(LectInfo[0],L_("Attente en cours, encore %d s"),reste);
					if(affiche) PanelRefreshVars(pLectRegen);
					OpiumUserAction();
					if(reglage_en_cours && (OscilloReglage[OscilloNum]->acq == 0)) break;
					if(!AttenteFinRegen) break;
					SambaAttends(1000);
					ecoule++;
					if(avec_dialogue) { if(!OpiumProgresRefresh(ecoule)) break; }
				}
				if(avec_dialogue) OpiumProgresClose();
				if(reste > 0) printf("          . Attente reduite a %d secondes\n",ecoule);
			} else printf("%s/ Attente finale de %d secondes abandonnee, la source n'etait pas mise\n",DateHeure(),reste);
		}
	}
	LectRegenTermine(0);
}
/* ========================================================================== */
int LectRegenChange() {
	if(RegenEnCours) { LectRegenAffiche(1,0); return(0); /* jusqu'a nouvel ordre */ }
	strcpy(RegenDebutJour,DateCivile());
	strcpy(RegenDebutHeure,DateHeure());
	if(Acquis[AcquisLocale].etat.active && Archive.enservice) LectTermineStocke(0,1);
	if(RegenEnCours) { LectRegenStoppe(0); RegenManuelle = 0; }
	else { RegenManuelle = 1; LectRegenLance(1); }
	return(0);
}
/* ========================================================================== 
void LectLitEtatPolar() {
	FILE *f;
	char etat[4];
	
	f = fopen(FichierEtatElec,"r");
	if(f) {
		fscanf(f,"%s %g %s %s",etat,&(LectCntl.polar),
			   RegenDebutJour,RegenDebutHeure);
		RegenEnCours = ((etat[0] == 'r') || (etat[0] == 'R'));
		if(etat[1] == '\0') LectSourceActive = RegenEnCours;
		else LectSourceActive = ((etat[1] == 'a') || (etat[1] == 'A'));
		fscanf(f,"%s %s %s",etat,RelaisDebutJour,RelaisDebutHeure);
		RelaisFermes = ((etat[0] == 'f') || (etat[0] == 'F'));
		fclose(f);
	}
}
   ========================================================================== 
void LectEcritEtatPolar() {
	FILE *f;
	
	RepertoireCreeRacine(FichierEtatElec);
	f = fopen(FichierEtatElec,"w");
	if(f) {
		fprintf(f,"%c%c %g %s %s\n",
				RegenEnCours? 'R': 'P',LectSourceActive? 'A': 'S',
				LectCntl.polar,
				RegenDebutJour,RegenDebutHeure);
		fprintf(f,"%c %s %s\n",
				RelaisFermes? 'F': 'O',RelaisDebutJour,RelaisDebutHeure);
		fclose(f);
	}
}
   ========================================================================== */
#pragma mark ----- Commandes au menu ------
int LectMesureTransfert() {
#ifdef AVEC_PCI
	int taille;
	int nb;
	int64 t0,dt;

	PCInum = 0;
	printf("===== Mesure de duree du transfert =====\n");
	for(taille=1024; taille<=PCIedw[PCInum].octets*4; taille *= 2) {
//		IcaDmaSetSize(PCIedw[PCInum].port,taille);
		t0 = (VerifTempsPasse? DateMicroSecs(): 0);
		IcaXferSizeSet(PCIedw[PCInum].port,taille);
		IcaXferExec(PCIedw[PCInum].port);
		dt = 1000000000;
		dt = 100;
		do dt--; while(((nb = IcaXferDone(PCIedw[PCInum].port)) == 0) && (dt > 0));
		if(nb < 0) {
			printf("Transfert avorte sur erreur %d\n",IcaDerniereErreur());
			printf("===== Mesure terminee =====\n");
			return(0);
		}
		dt = (VerifTempsPasse? DateMicroSecs(): 0) - t0;
		printf("%3d,%06d secondes pour %6d mots, soit %3g ns/mot ou %g MB/s\n",
			(int)(dt / 1000000),Modulo(dt,1000000),taille/4,
			4000.0*(float)dt/(float)taille,(float)taille/(float)dt);
		fflush(stdout);
	}
	printf("===== Mesure terminee =====\n");
	IcaXferSizeSet(PCIedw[PCInum].port,0);
#endif
	return(0);
}
/* ========================================================================== */
int LectReglage() {
	int i;
	TypeMonitFenetre *f; Graph g;

#ifdef MSGS_RESEAU
	if(SambaMode == SAMBA_DISTANT) {
		CarlaMsgBegin("Reglages"); CarlaMsgEnd(&BalSuperviseur);
		printf("%s/ Envoi d'une commande 'Reglages' au Superviseur\n",DateHeure());
	} else {
#endif
		OscilloOuvert = LectReglageEnCours = 1;
#ifdef A_ADAPTER
		/* cf ReglageRazFet() */
		if(OpiumDisplayed(bBoloReglageBB1)) OpiumClear(bBoloReglageBB1);
		OpiumFork(bBoloReglageBB1);
#endif
		OpiumClear(bLecture);
		for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) {
			if((g = f->g)) { if(OpiumDisplayed(g->cdr)) OpiumClear(g->cdr); }
		}
#ifdef MSGS_RESEAU
	}
#endif
	return(0);
}
/* ========================================================================== */
int LectRazHistos() {
	TypeMonitFenetre *f; int i; Graph g;
	
	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if((f->type == MONIT_HISTO) && (g = f->g)) {
		if(f->p.h.histo) HistoRaz(f->p.h.histo);
		if(OpiumAlEcran(g)) OpiumRefresh(g->cdr);
	}

	HistoRaz(LectFenSpectre.p.h.histo);
	if((g = LectFenSpectre.g)) { if(OpiumDisplayed(g->cdr)) OpiumRefresh(g->cdr); }
	HistoRaz(LectFenBruit.p.h.histo);
	if((g = LectFenBruit.g)) { if(OpiumDisplayed(g->cdr)) OpiumRefresh(g->cdr); }

	return(0);
}
/* ========================================================================== */
static int LectRunProgramme(char log) {
	int i,n; char date[DATE_MAX]; int64 maintenant;
	
	maintenant = DateLong();
	if(log) { DateLongPrint(date,maintenant); printf("* Examen du calendier le %s:\n",date); }
	SambaRunCourant = -1; n = 0;
	for(i=0; i<SambaRunsNb; i++) if(!strcmp(SambaRunPrevu[i].titre,CalendrierRun)) {
		if(log) {
			printf("  . Run #%2d:",++n); 
			DateLongPrint(date,SambaRunPrevu[i].date.debut); printf(" du %s",date);
			DateLongPrint(date,SambaRunPrevu[i].date.fin)  ; printf(" au %s",date);
		}
		if(SambaRunPrevu[i].date.debut >= maintenant) break;
		if(log) printf(": trop tard\n");
	}
	if(log) {
		if(i >= SambaRunsNb) {
			OpiumWarn("Pas de run prevu au calendrier %s",CalendrierNom);
			printf("  > Pas de run prevu au calendrier %s\n",CalendrierNom);
		} else printf(": a executer\n");
	}
	if(i >= SambaRunsNb) return(0);
	SambaRunCourant = i;
	LectRetard.start.date = SambaRunPrevu[i].date.debut;
	LectRetard.stop.date = SambaRunPrevu[i].date.fin;

	return(1);
}
/* ========================================================================== */
static char LectDelaiEcoule() {
	char doit_terminer,pair;
	
	doit_terminer = OpiumRefreshBegin(bLecture);
	pair = 1;
	MenuItemAllume(mLectArret,1,L_("Stopper "),GRF_RGB_YELLOW);
	while(LectDemarrageDemande) {
		int64 maintenant; char delai[12]; int secs;
		maintenant = DateLong();
		// printf("  * maintenant=%12lld, depart=%12lld\n",maintenant,LectRetard.start.date);
		if(maintenant >= LectRetard.start.date) break;
		secs = (int)(DateLongToSecondes(LectRetard.start.date) - DateLongToSecondes(maintenant)); S_usPrint(delai,secs,0);
		sprintf(LectInfo[0],L_("Demarrage prevu dans %s"),delai);
		if(OpiumAlEcran(pLectRegen)) PanelRefreshVars(pLectRegen);
		if(pair) MenuItemAllume(mLectDemarrage,1,L_("Prevu   "),GRF_RGB_ORANGE);
		else MenuItemAllume(mLectDemarrage,1,L_("Prevu   "),GRF_RGB_YELLOW);
		OpiumUserAction();
		sleep(1);
		pair = 1 - pair;
	}
	if(!LectDemarrageDemande) {
		strcpy(LectInfo[0],L_("Demarrage differe abandonne"));
		if(OpiumAlEcran(pLectRegen)) PanelRefreshVars(pLectRegen);
		MenuItemAllume(mLectDemarrage,1,L_("Demarrer"),GRF_RGB_YELLOW);
		MenuItemEteint(mLectArret,1,L_("Arrete  "));
	}
	if(doit_terminer) OpiumRefreshEnd(bLecture);
	return(LectDemarrageDemande);
}
/* ========================================================================== */
int LectDemarre() {
#ifdef MSGS_RESEAU
	if(SambaMode == SAMBA_DISTANT) {
		CarlaMsgBegin("Demarrer"); CarlaMsgEnd(&BalSuperviseur);
		printf("%s/ Envoi d'une commande 'Demarrer' au Superviseur\n",DateHeure());
	} else {
#endif
//		if(Acquis[AcquisLocale].etat.active || LectureSuspendue) LectStop();
		if(LectureSuspendue) LectureSuspendue = 0;
		else if(Acquis[AcquisLocale].etat.active) return(0);
		else {
			if(Trigger.demande && (Trigger.type != TRGR_CABLE)) {
				OpiumWarn("Mode de recherche d'evenements type '%s' abandonne, impose: 'cable'",TrgrDefListe[Trigger.type]);
				Trigger.type = TRGR_CABLE;
			}
			if(ArchModeStream != ARCH_BRUTES) {
				OpiumWarn("Sauvegarde type %s abandonne, impose: %s",ArchNomStream[ArchModeStream],ArchNomStream[ARCH_BRUTES]);
				ArchModeStream = ARCH_BRUTES;
			}
			LectCntl.LectMode = LECT_DONNEES;
		#ifdef MODULES_RESEAU
			if(SambaMode == SAMBA_MAITRE) {
				int sat;
			#ifdef PARMS_RESEAU
				CarlaDataUpdate(IdCntlLecture);
			#endif
			#ifndef CODE_WARRIOR_VSN
				if(LectSynchroType == LECT_SYNC_SEC) {
					struct timeval heure_lue;
					do gettimeofday(&heure_lue,0); while((heure_lue.tv_usec < 400000) || (heure_lue.tv_usec > 600000));
					printf("\n= Ordre de demarrage a %ld.%06d secondes (soit %s)\n",heure_lue.tv_sec,(int)heure_lue.tv_usec,DateHeure());
				}
			#endif
				for(sat=0; sat<AcquisNb; sat++) if((sat != AcquisNb) && (Acquis[sat].etat.status >= SATELLITE_ATTENTE)) {
					CarlaMsgBegin("Demarrer"); CarlaMsgEnd(&(Acquis[sat].bal));
					printf("- Envoi d'une commande 'Demarrer' a l'Acquisition %s\n",Acquis[sat].code);
				} else printf("- L'Acquisition %s est %s\n",Acquis[sat].code,AcquisTexte[(int)Acquis[sat].etat.status]);
			} else if(SambaMode == SAMBA_SATELLITE) {
				OpiumDisplay(bLecture);
			}
		#endif /* MODULES_RESEAU */
			LectDemarrageDemande = 1; ReglagesEtatPeutStopper = NumeriseurEtatPeutStopper = NumeriseurPeutStopper = 0;
			if(LectRetard.start.mode == LECT_RETARD_PREVU) {
				LectRetard.start.date = DateJourHeureToLong(DateJourToInt(LectRetard.start.jour),DateHeureToInt(LectRetard.start.heure));
				LectRetard.stop.date = DateJourHeureToLong(DateJourToInt(LectRetard.stop.jour),DateHeureToInt(LectRetard.stop.heure));
				if(!LectDelaiEcoule()) return(0);
			} else if(LectRetard.start.mode == LECT_RETARD_CALDR) {
				if(LectRunProgramme(1)) LectDelaiEcoule();
				else LectDemarrageDemande = 0;
			} else if(LectRetard.start.mode == LECT_RETARD_AUTO) {
			}
			while(LectDemarrageDemande) {
				LectDemarrageDemande = 0;
				if(!MonitEvtClasses && !OpiumDisplayed(bLecture)) OpiumFork(bLecture);
			#ifdef LECT_MODE_ABANDONNE
				if(LectCntl.LectMode != LECT_DONNEES) {
					if(LectCntl.LectMode == LECT_COMP) LectCompensation();
					else LectIdent(0);
				} else
			#endif
				{
					if(NomScriptStart[0] && strcmp(NomScriptStart,"neant")) ScriptExec(FichierScriptStart,NomScriptStart,"");
					LectAcqStd();
					if(NomScriptStop[0] && strcmp(NomScriptStop,"neant")) ScriptExec(FichierScriptStop,NomScriptStop,"");
				}
				if(LectRetard.start.mode == LECT_RETARD_CALDR) {
					if(LectRunProgramme(1)) {
						LectRetardRefresh(); LectDemarrageDemande = 1; LectDelaiEcoule();
					}
				}
			}
		}
		LectAcqLanceur = NEANT;
#ifdef MSGS_RESEAU
	}
#endif
	return(0);
}
/* ========================================================================== */
int LectEtat() {
/* /docFuncBeg {LectEtat}
   	/docFuncReturn  {0}
   	/docFuncSubject {Affiche ou efface (en bascule) l'etat de la lecture}
   /docFuncEnd 
*/
	if(OpiumDisplayed(pLectEtat->cdr)) {
		OpiumClear(pLectEtat->cdr);
		if(pSettingsTrigger) OpiumClear(pSettingsTrigger->cdr);
		if(pSettingsModes) OpiumClear(pSettingsModes->cdr);
	} else {
		if(pSettingsModes) OpiumDisplay(pSettingsModes->cdr);
		if(pSettingsTrigger) {
			if(PanelItemNb(pSettingsTrigger))
				OpiumDisplay(pSettingsTrigger->cdr);
		}
		OpiumDisplay(pLectEtat->cdr);
#ifdef STATUS_AVEC_TIMER
		OpiumTimer(pLectEtat->cdr,500);
#endif
	}
	return(0);
}
/* ========================================================================== */
int LectStop() {
/* /docFuncBeg {LectStop}
   	/docFuncReturn  {0}
   	/docFuncSubject {Arrete la tache LectExec}
   /docFuncEnd 
*/
	int j,k,n;

	LectArretePoliment = 1;
	LectRepartAttendus = 0;
	ReglVi.actif = 0;
#ifdef MSGS_RESEAU
	if(SambaMode == SAMBA_DISTANT) {
		CarlaMsgBegin("Stopper"); CarlaMsgEnd(&BalSuperviseur);
		printf("%s/ Envoi d'une commande 'Stopper' au Superviseur\n",DateHeure());
	} else {
#endif
#ifdef MODULES_RESEAU
		if(SambaMode == SAMBA_MAITRE) {
			int sat;
			for(sat=0; sat<AcquisNb; sat++) if((sat != AcquisNb) && (Acquis[sat].etat.status >= SATELLITE_ATTENTE)) {
				CarlaMsgBegin("Stopper"); CarlaMsgEnd(&(Acquis[sat].bal));
				printf("%s/ Envoi d'une commande 'Stopper' a l'Acquisition %s\n",
					   DateHeure(),Acquis[sat].code);
			} else printf("%s/ L'Acquisition %s est %s\n",DateHeure(),
						  Acquis[sat].code,AcquisTexte[(int)Acquis[sat].etat.status]);
		}
#endif
		if(LectAcqCollective && !LectArretExterieur) {
			for(j=0; j<Partit[PartitLocale].nbsat; j++) if(Partit[PartitLocale].exter[j]) {
				SambaEcritSat1Court(Partit[PartitLocale].sat[j],SMB_ARRET);
			}
		}
		if(SambaMaitre && LectDepileSousIt) {
			k = 2; while(k--) {
				sleep(1);
				LectActionUtilisateur();
			}
		}
		LectArretExterieur = 0;
		Acquis[AcquisLocale].etat.status = SATELLITE_ATTENTE;
		LectAutreSequence = 0; AttenteFinRegen = 0; LectDemarrageDemande = 0;
		if(!Acquis[AcquisLocale].etat.active) return(0);
		Acquis[AcquisLocale].etat.active = 0;
		if(OpiumDisplayed(iLectPause->cdr)) OpiumClear(iLectPause->cdr);
		for(n=0; n<OscilloNb; n++) {
			if(OscilloReglage[n]->iAcq) {
				if(OpiumDisplayed((OscilloReglage[n]->iAcq)->cdr)) OpiumRefresh((OscilloReglage[n]->iAcq)->cdr);
			}
		}
		LectDeclencheErreur(0,0,0,0,0);
		LectureSuspendue = 0;
#ifdef MSGS_RESEAU
	}
#endif
	return(0);
}
/* ========================================================================== */
int LectSuspend() {
/* /docFuncBeg {LectSuspend}
   	/docFuncReturn  {0}
   	/docFuncSubject {Suspend la tache LectExec}
   /docFuncEnd 
*/
	if(LectureSuspendue) {
		if(OpiumDisplayed(iLectPause->cdr)) OpiumClear(iLectPause->cdr);
		LectureSuspendue = 0;
	} else {
		if(!OpiumDisplayed(iLectPause->cdr)) OpiumDisplay(iLectPause->cdr);
		LectureSuspendue = 1;
	}
	return(0);
}
/* ========================================================================== */
int Lect1Modul() {
/* /docFuncBeg {Lect1Modul}
   	/docFuncReturn  {0}
   	/docFuncSubject {Demande de lire une synchro D2 supplementaire}
   /docFuncEnd 
*/
	Acquis[AcquisLocale].etat.active = 1;
	Lect1ModulALire = 1;
	if(OpiumDisplayed(iLectPause->cdr)) OpiumClear(iLectPause->cdr);
	LectureSuspendue = 0;
	return(0);
}
/* ========================================================================== */
int LectCompacteDumpFifo() {
	LectDumpFifoFromRep(0,LectCompacteMaxFifo);
	return(0);
}
/* ========================================================================== */
int LectCompacteDumpBuff() {
	char ok; int i,j,nb,premier,dernier,total;
	int voie;
	int64 delta,debut_stocke;
	TypeTamponDonnees *tampon;
	TypeDonnee *ptr_int16; int smin,smax,smoy,sval;
	TypeSignal *ptr_float; float rmin,rmax,rmoy,rval; int64 lmin,lmax,lmoy,lval;
	int nval,lngr;
	FILE *dest;
	
	voie = Bolo[BoloNum].captr[CapteurNum].voie;
	if(!VoieTampon[voie].lue) {
		OpiumFail("La voie %s n'a pas ete lue",VoieManip[voie].nom);
		return(0);
	}
	VoieNum = voie;
	ok = 0;
	tampon = &(VoieTampon[voie].trmt[LectBuffOrigType].donnees);
	nval = tampon->nb;
	if(nval <= 0)  OpiumFail("La voie %s %s est vide",VoieManip[voie].nom,TrmtClassesListe[LectBuffOrigType]);
	else {
		lngr = tampon->max;
		if(LectBuffOrigType == 0) /* donnees brutes */ {
			delta = (VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien / 1000000000) * 1000000000;
			premier = (int)(VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien - delta);
		} else /* donnees traitees */ {
			if(VoieTampon[voie].traitees->max <= 0) {
				OpiumFail("La voie %s n'a pas de tampon de donnees traitees",VoieManip[voie].nom);
				return(0);
			}
			debut_stocke = (DernierPointTraite / (int64)VoieTampon[voie].trmt[TRMT_AU_VOL].compactage / (int64)VoieTampon[voie].trmt[TRMT_PRETRG].compactage) - nval;
			delta = (debut_stocke / 1000000000) * 1000000000;
			premier = (int)(debut_stocke - delta);
		}
		dernier = premier + nval;
		if(LectBuffVal0 < premier) LectBuffVal0 = premier;
		else if(LectBuffVal0 >= dernier) LectBuffVal0 = dernier - 1;
		if(LectBuffValN <= LectBuffVal0) LectBuffValN = LectBuffVal0 + 1;
		if(LectBuffValN > dernier) LectBuffValN = dernier;
		total = LectBuffValN - LectBuffVal0;
		if(total <= 0)
			OpiumFail("L'intervalle [%d,%d[ pour la voie %s est vide",LectBuffVal0,LectBuffValN,VoieManip[voie].nom);
		else {
			if(LectBuffDestType) {
				sprintf(LectBuffFichier,"%s%s%s%03d",VoieManip[voie].nom,
					TrmtClassesListe[LectBuffOrigType],DateCourante,NumeroLect);
				OpiumReadText("Fichier a creer",LectBuffFichier,40);
				RepertoireCreeRacine(LectBuffFichier);
				dest = fopen(LectBuffFichier,"w");
				if(!dest) {
					OpiumFail("Fichier %s inutilisable",LectBuffFichier);
					return(1);
				}
			} else {
				dest = stdout;
				fprintf(dest,"%s %s a partir de %d (%d/%d points)\n",
					VoieManip[voie].nom,TrmtClassesListe[LectBuffOrigType],
					LectBuffVal0,total,nval);
			}
			i = LectBuffVal0;
			debut_stocke = i + delta;
			j = Modulo(debut_stocke,lngr);
			if(tampon->type == TAMPON_INT16) {
				ptr_int16 = tampon->t.sval + j;
				smin = 999999; smax = -999999; smoy = 0;
				nb = 0;
				while(nb < total) {
					if(!LectBuffDestType) { if(!(nb % 10)) fprintf(dest,"%11d: ",i); }
					sval = (int)*ptr_int16++;
					if(smin > sval) smin = sval;
					if(smax < sval) smax = sval;
					smoy += sval;
					if(LectBuffHexa) {
						sval &= 0xFFFFF; fprintf(dest,"  %05X",sval);
					} else fprintf(dest,"%7d",sval);
					i++; if(++j == lngr) { ptr_int16 = tampon->t.sval; j = 0; }
					if(!(++nb % 10)) {
						fprintf(dest,"\n");
						if(!LectBuffDestType) { if(!(nb % 500)) WndStep(0); }
					}
				};
				if(nb % 10) fprintf(dest,"\n");
				if(LectBuffHexa) fprintf(dest,"min: %05X, max: %05X, moy: %05X\n",smin,smax,smoy/total);
				else fprintf(dest,"min: %d, max: %d, moy: %d\n",smin,smax,smoy/total);
			} else {
				ptr_float = tampon->t.rval + j;
				rmin = 999999.9; rmax = -999999.9; rmoy = 0.0;
				lmin = 999999999; lmax = -999999999; lmoy = 0;
				nb = 0;
				while(nb < total) {
					if(!LectBuffDestType) { if(!(nb % 10)) fprintf(dest,"%11d: ",i); }
					rval = *ptr_float++;
					if(rmin > rval) rmin = rval;
					if(rmax < rval) rmax = rval;
					rmoy += rval;
					if(LectBuffHexa) {
						lval = (int64)rval; lval &= 0xFFFFFFFF;
						if(lmin > lval) lmin = lval;
						if(lmax < lval) lmax = lval;
						lmoy += lval;
						fprintf(dest,"  %08llX",lval);
					} else fprintf(dest," %9.2f",rval);
					i++; if(++j == lngr) { ptr_float = tampon->t.rval; j = 0; }
					if(!(++nb % 10)) {
						fprintf(dest,"\n");
						if(!LectBuffDestType) { if(!(nb % 500)) WndStep(0); }
					}
				};
				if(nb % 10) fprintf(dest,"\n");
				if(LectBuffHexa) fprintf(dest,"min: %llX, max: %llX, moy: %llX\n",lmin,lmax,lmoy/(int64)total);
				else fprintf(dest,"min: %9.2f, max: %9.2f, moy: %9.2f\n",rmin,rmax,rmoy/(float)total);
			}
			if(LectBuffDestType) fclose(dest);
		}
	}

	return(0);
}
/* ========================================================================== */
int LectBuffStatus() {
/* /docFuncBeg {LectBuffStatus}
   	/docFuncReturn  {0}
   	/docFuncSubject {Impression de l'etat des tampons pour toutes les voies}
   /docFuncEnd 
*/
	int voie; int64 point_stocke;

	printf("--- Donnees brutes\n");
	printf("            voie (gain)      tampon       lus     dispo       1er   @tampon    @lu\n");
	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
		printf("%16s (x%3d):  %9d %9lld %9d %9lld   %08llX %08llX\n",VoieManip[voie].nom,VoieManip[voie].def.trmt[TRMT_AU_VOL].gain,
			VoieTampon[voie].brutes->max,VoieTampon[voie].lus,VoieTampon[voie].brutes->nb,VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien,
			(VoieTampon[voie].brutes->type == TAMPON_INT16)? (IntAdrs)VoieTampon[voie].brutes->t.sval: (IntAdrs)VoieTampon[voie].brutes->t.ival,
			(VoieTampon[voie].brutes->type == TAMPON_INT16)? (IntAdrs)VoieTampon[voie].prochain_s16: (IntAdrs)VoieTampon[voie].prochain_i32);
	}
	printf("--- Donnees traitees\n");
	printf("            voie (gain)      tampon       lus     dispo       1er   @tampon\n");
	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
		point_stocke = LectStampsLus / (int64)VoieTampon[voie].trmt[TRMT_AU_VOL].compactage / (int64)VoieTampon[voie].trmt[TRMT_PRETRG].compactage;
		printf("%16s (x%3d)%c  %9d %9lld %9d    %6lld   %08llX\n",VoieManip[voie].nom,VoieManip[voie].def.trmt[TRMT_PRETRG].gain,
			VoieTampon[voie].cree_traitee? ':': '?',VoieTampon[voie].traitees->max,
			point_stocke,VoieTampon[voie].traitees->nb,point_stocke-(int64)VoieTampon[voie].traitees->nb,
			(IntAdrs)VoieTampon[voie].traitees->t.rval);
	}
	
	return(0);
}
/* ========================================================================== */
int LectBuffPrintLimites(Panel panel, int num, void *arg) {
	int nval,lngr,naff; int voie;
	int premier,dernier;
	int64 delta,debut_stocke;

	if(panel) { PanelItemRelit(panel,num-1); PanelItemRelit(panel,num); }
	voie = Bolo[BoloNum].captr[CapteurNum].voie;
	if(LectBuffOrigType == 0) /* donnees brutes */ {
		lngr = VoieTampon[voie].brutes->max;
		nval = VoieTampon[voie].brutes->nb;
		delta = (VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien / 1000000000) * 1000000000;
		premier = (int)(VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien - delta);
	} else /* donnees traitees */ {
		if(VoieTampon[voie].traitees->max <= 0) {
			OpiumFail("La voie %s n'a pas de tampon de donnees traitees",VoieManip[voie].nom);
			return(0);
		}
		lngr = VoieTampon[voie].traitees->max;
		nval = VoieTampon[voie].traitees->nb;
		debut_stocke = (DernierPointTraite / (int64)VoieTampon[voie].trmt[TRMT_AU_VOL].compactage / (int64)VoieTampon[voie].trmt[TRMT_PRETRG].compactage) - nval;
		delta = (debut_stocke / 1000000000) * 1000000000;
		premier = (int)(debut_stocke - delta);
	}
	dernier = premier + nval;
	naff = LectBuffValN - LectBuffVal0; if(naff < 1) naff = 10;
	if(LectBuffVal0 < premier) { LectBuffVal0 = premier; LectBuffValN = LectBuffVal0 + naff; }
	else if(LectBuffVal0 > dernier) LectBuffVal0 = dernier - naff;
	if(LectBuffValN < LectBuffVal0) LectBuffValN = LectBuffVal0 + naff;
	if(LectBuffValN > dernier) LectBuffValN = dernier;
	if(panel) { if(OpiumDisplayed(panel->cdr)) PanelRefreshVars(panel); }
	return(1);
}
/* ========================================================================== */
int LectBuffDump() {
/* /docFuncBeg {LectBuffDump}
   	/docFuncReturn  {0}
   	/docFuncSubject {Impression du contenu des tampons d'une voie}
   /docFuncEnd 
*/
	int bolo,vm; // int cap,voie;

	bolo = BoloNum; vm = ModlNum;
	if(OpiumExec(pLectBuffPrint->cdr) == PNL_CANCEL) return(0);
	if(Bolo[BoloNum].lu == DETEC_HS) {
		OpiumFail("Le detecteur %s n'est pas lu",Bolo[BoloNum].nom);
		BoloNum = bolo; ModlNum = vm;
		return(0);
	}
	/* for(cap=0; cap<Bolo[BoloNum].nbcapt; cap++) {
		if((voie = Bolo[BoloNum].captr[cap].voie) < 0) continue;
		if(ModlNum == VoieManip[voie].type) break;
	}
	if(cap == Bolo[BoloNum].nbcapt) {
		OpiumError("Pas de voie '%s' pour le detecteur '%s'",ModeleVoie[ModlNum].nom,Bolo[BoloNum].nom);
		BoloNum = bolo; ModlNum = vm;
		return(0);
	} */
	LectCompacteDumpBuff();
	return(0);
}
/* ========================================================================== */
int LectHistosDump() {
	TypeMonitFenetre *f; int i;

	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if(f->type == MONIT_HISTO) {
		if(f->p.h.histo) HistoDump(f->p.h.histo);
	}
	if(TrmtHampl) HistoDump(hdBruitAmpl->histo);
	if(TrmtHmontee) HistoDump(hdBruitMontee->histo);
	if(TrmtH2D) H2DDump(h2D);
	return(0);
}
/* ========================================================================== */
int LectDecaleT0() {
	OpiumReadFloat("Origine des temps",&(VoieTampon[0].trmt[TRMT_AU_VOL].ab6[0]));
	return(0);
}
#pragma mark -------- Gestion interface -------
/* ========================================================================== */
static int LectSauveDetec(Panel p, void *ptr) {
	int bolo,num;
	
	for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].seuils_changes) {
		for(num=0; num<Bolo[bolo].nbreglages; num++) strcpy(Bolo[bolo].reglage[num].std,Bolo[bolo].reglage[num].user);
		DetecteurEcrit(0,&(Bolo[bolo]),bolo,1,DETEC_REGLAGES);
	}
	if(OpiumDisplayed(mLectSupplements->cdr)) MenuItemEteint(mLectSupplements,3,0);
	
	return(0);
}
/* ========================================================================== */
static int LectRepartStoppe(Menu menu) {
	RepartiteursInitTous(REPART_STOPPE,Echantillonnage,menu);
	if(OpiumAlEcran(pRepartXmit)) PanelRefreshVars(pRepartXmit);
	return(0);
}
/* ========================================================================== */
int LectSauve() { PasImplementee(); return(0); }
/* ========================================================================== */
Menu mLectTotal;
TypeMenuItem iLect[] = {
	{ "Etat",            MNU_FONCTION LectEtat },
	{ "Demarrer     d",  MNU_FONCTION LectAcqStd },
	{ "Stopper      s",  MNU_FONCTION LectStop },
	{ "Contenu tampons", MNU_FONCTION LectBuffDump },
	{ "Menu complet",    MNU_MENU   &mLectTotal },
	{ "Fermer",          MNU_RETOUR },
	MNU_END
};
TypeMenuItem iLectTotal[] = {
	{ "Mesure de temps", MNU_FONCTION LectMesureTransfert },
	{ "Etat",            MNU_FONCTION LectEtat },
	{ "Demarrer",        MNU_FONCTION LectAcqStd },
	{ "Pause",           MNU_FONCTION LectSuspend },
	{ "Une synchro D2",  MNU_FONCTION Lect1Modul },
	{ "Stopper",         MNU_FONCTION LectStop },
	{ "Contenu Fifo",    MNU_FONCTION LectDumpFifo },
	{ "Contenu tampons", MNU_FONCTION LectBuffDump },
	{ "Pointeurs tampons", MNU_FONCTION LectBuffStatus },
	//	{ "Origine des temps", MNU_FONCTION LectDecaleT0 },
	{ "Vidage histos",   MNU_FONCTION LectHistosDump },
	// { "Fichier",         MNU_SEPARATION },
	// { "Lire",            MNU_FONCTION LectArchOuvre },
	// { "Recommencer",     MNU_FONCTION LectArchRAZ },
	/*	{ "Archiver",        MNU_FONCTION LectArchArchive }, */
	/*
	{ "Preferences",     MNU_SEPARATION },
	{ "Recuperer",       MNU_FONCTION PasImplementee },
	{ "Modifier",        MNU_FONCTION PasImplementee },
	{ "Sauver",          MNU_FONCTION LectSauve },
	*/
	{ "\0",              MNU_SEPARATION },
	{ "Menu simplifie",  MNU_MENU   &mLect },
	{ "Fermer",          MNU_RETOUR },
	MNU_END
};

#ifdef SPECTRES_SEQUENCES
Menu mLectSequences;
#endif /* SPECTRES_SEQUENCES */

// int LectRunStart();
TypeMenuItem iLectRuns[] = {
//	{ "Enchainement auto",      MNU_FONCTION LectRunStart },
//	{ "\0",                     MNU_SEPARATION },
	{ "Configuration de run",   MNU_FONCTION LectConfigChoix },
	{ "Consignes Detecteurs",   MNU_PANEL  &(pReglagesConsignes[LECT_CONS_CMDE]) },
	{ "Traitement des voies",   MNU_PANEL  &pLectTraitements },
#ifdef SPECTRES_SEQUENCES
	{ "Sequences",              MNU_MENU   &mLectSequences },
#else
	{ "Sequences",              MNU_FONCTION SequencesEdite },
#endif /* SPECTRES_SEQUENCES */
	{ "Prise de donnees",       MNU_FORK   &bLecture },
	{ "Fermer",                 MNU_RETOUR },
	MNU_END
};

#ifdef SPECTRES_SEQUENCES
TypeMenuItem iLectSequences[] = {
	{ "Param.spectres",  MNU_FONCTION CalcSpectreAutoParms  },
	{ "Programme",       MNU_FONCTION SequencesEdite },
	{ "Fermer",          MNU_RETOUR },
	MNU_END
};
#endif /* SPECTRES_SEQUENCES */

#ifdef SPECTRES_COMPACTE
TypeMenuItem iLectSpectreControle[] = {
	{ "Mesurer  ",  MNU_FONCTION LectCompacteSpectres },
	{ "Stopper  ",  MNU_FONCTION LectStop },
	{ "Sauver   ",  MNU_FONCTION CalcSpectreAutoSauve },
	{ "Retrouver",  MNU_FONCTION CalcSpectreAutoRetrouve },
	MNU_END
};
#endif /* SPECTRES_COMPACTE */

static TypeMenuItem iLectDemarrage[] = { { "Demarrer", MNU_FONCTION LectDemarre }, MNU_END };
static TypeMenuItem iLectArret[]     = { { "Stopper ", MNU_FONCTION LectStop }, MNU_END };
static TypeMenuItem iLectRegen[]     = { { "Lancer  regeneration",   MNU_FONCTION LectRegenChange }, MNU_END };
static TypeMenuItem iLectSupplements[] = {
	{ "RAZ histos",           MNU_FONCTION LectRazHistos },
	{ "Compensation",         MNU_FONCTION LectCompenseTout },
	{ "Sauver detecteurs",    MNU_FONCTION LectSauveDetec },
	{ "Stopper repartiteurs", MNU_FONCTION LectRepartStoppe },
	MNU_END
};
Info infoLectSpectre;
TypeMenuItem itemsLectSpectre[] = { { "Non", MNU_RETOUR }, { "Oui", MNU_RETOUR }, MNU_END };

/* ========================================================================== */
int LectRunStart() {
	// OpiumDebugLevel[OPIUM_DEBUG_OPIUM] = 2;
	printf("(%s) Execution de l'objet @%08X: %08X\n",__func__,pReglagesConsignes[LECT_CONS_CMDE]->cdr,pReglagesConsignes[LECT_CONS_CMDE]);
	if(OpiumExec(pReglagesConsignes[LECT_CONS_CMDE]->cdr) == PNL_CANCEL) {
		OpiumFail("Le run n'a PAS ete lance");
		return(0);
	}

#ifdef A_REMETTRE
//	if(OpiumExec(pLectBolosTrmt->cdr) == PNL_CANCEL) {
	if(LectModifTrmt() < 0) {
		OpiumFail("Le run n'a PAS ete lance");
		return(0);
	}
//	if(OpiumExec(pLectBolosTrgr->cdr) == PNL_CANCEL) {
	if(LectModifTrgr() < 0) {
		OpiumFail("Le run n'a PAS ete lance");
		return(0);
	}
	if(SequencesEdite() == -1) {
		OpiumFail("Le run n'a PAS ete lance");
		return(0);
	}
#ifdef SPECTRES_SEQUENCES
	if(!CalcSpectreAutoParms()) {
		OpiumFail("Le run n'a PAS ete lance");
		return(0);
	}
#endif /* SPECTRES_SEQUENCES */
	if(OpiumExec(pReglagesConsignes[LECT_CONS_CMDE]->cdr) == PNL_CANCEL) {
		OpiumFail("Le run n'a PAS ete lance");
		return(0);
	}
#endif
	OpiumFork(bLecture);
	return(0);
}
/* ========================================================================== */
static int LectUpdateControl() {
#ifdef PARMS_RESEAU
	if(SambaMode != SAMBA_MONO) CarlaDataUpdate(IdCntlLecture);
#endif
	return(0);
}
/* ========================================================================== */
int LectUpdate_pIntervAff(Instrum i) {
//	if(OpiumDisplayed(pIntervAff->cdr)) PanelRefreshVars(pIntervAff);
	MonitIntervAff = (int)(MonitIntervSecs * 1000.0);
	LectIntervData = (int64)MonitIntervAff * SynchroD2_kHz;
	LectUpdateControl();
	return(0);
}
/* ========================================================================== */
int LectEvtAffiche(Panel p, int i, void *arg) {
	return(MonitEvtAffiche(0,0,0,(int)(IntAdrs)arg));
}
/* ========================================================================== */
int LectNomDetecteur(Panel p, void *arg) {
	int *val;

	// val = (int *)PanelApplyArg(p);
	val = (int *)arg;
	if((*val >= 0) && (*val < BoloNb)) BoloNum = *val;
	else LectBoloDemande = BoloNum;
	PanelRefreshVars(p);
	return(0);
}
/* ========================================================================== */
int LectEffaceMenu(Cadre cdr, void *arg) {
	Menu m;

	// m = (Menu)OpiumEnterArg(cdr);
	m = (Menu)arg; if(OpiumDisplayed(m->cdr)) OpiumClear(m->cdr);
	return(0);
}

#pragma mark ------ Panels de reglage -------
#define MAXGALETTES 10
#define MAXBRANCHES 4
#define MAXTROUS 12
#define MAXTOURS MAXTROUS*MAXBRANCHES
/* ========================================================================== */
void LectConsignesConstruit() {
	int bolo,voie,cap,vm,vmsvt,classe,der,num,nbitems;	int i,j,k,cols; char fin_de_serie; LECT_CONS_TYPE type_panel;
//	char nomvoie[VOIES_MAX][VOIE_NOM]; char *c,*r;
	int nb_canaux,nb_regl,max_regl,lig_cons;
	TypeReglage *regl;
	Panel p;
	
	// printf("(%s) %d detecteur%s\n",__func__,Accord1s(BoloNb));
	/* nombre de reglages a afficher */
	nb_canaux = 0; max_regl = 0;
	for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].local) {
		for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
			if(Bolo[bolo].captr[cap].bb.num < 0) continue;
			nb_regl = 0;
			for(num=0; num<Bolo[bolo].nbreglages; num++) {
				regl = &(Bolo[bolo].reglage[num]);
				if(regl->capt == cap) nb_regl++;
			}
			if(nb_regl) nb_canaux++;
			if(nb_regl > max_regl) max_regl = nb_regl;
		}
	}
	lig_cons = 1 + nb_canaux;
	cols = 5 + max_regl;
	nbitems = cols * lig_cons;
	
	/* dimensionnement du panel et de la table des reglages */
	for(type_panel=LECT_CONS_ETAT; type_panel<LECT_CONS_NB; type_panel++) {
		if(PanelItemNb(pReglagesConsignes[type_panel]) != nbitems) /* cas '>' mal gere */ {
			PanelDelete(pReglagesConsignes[type_panel]);
			pReglagesConsignes[type_panel] = 0;
		}
		if(!pReglagesConsignes[type_panel]) pReglagesConsignes[type_panel] = PanelCreate(nbitems);
	}
	if(DetecReglagesLocaux) { if(DetecReglagesLocauxNb != nbitems) { free(DetecReglagesLocaux); DetecReglagesLocaux = 0; } }
	if(!DetecReglagesLocaux) {
		DetecReglagesLocaux = (TypeReglage **)malloc(nbitems * sizeof(TypeReglage *));
		DetecReglagesLocauxNb = nbitems;
	}
	
	/* construction des panels et de la table des reglages */
	if(DetecReglagesLocaux) for(i=0; i<nbitems; i++) DetecReglagesLocaux[i] = 0;
	for(type_panel=LECT_CONS_ETAT; type_panel<LECT_CONS_NB; type_panel++) {
		p = pReglagesConsignes[type_panel];
		PanelErase(p); PanelColumns(p,cols);
		if(type_panel == LECT_CONS_CMDE) PanelMode(p,PNL_BYLINES);
		else PanelMode(p,PNL_RDONLY|PNL_DIRECT|PNL_BYLINES);
		/* entete */
		PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,"detec.",L_("etat"),5),0),1);
		i = PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"pos",5),0),1);
			PanelItemBarreDroite(p,i,1);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,L_("canal"),8),0),1);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0," BB",3),0),1);
		i = PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"Rep",3),0),1);
		PanelItemBarreDroite(p,i,1);
		for(k=0; k<max_regl; k++) PanelItemSouligne(p,PanelBlank(p),1);
		//+ PanelVerrouEntete(p,1);
		/* tour des bolos: for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].local) */
		pour_tout(j,BoloNb) {
			bolo = OrdreDetec[j].num;
			if(!Bolo[bolo].local) continue;
			/* le dernier capteur sera suivi d'une ligne de separation */
			der = 0;
			for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
				if(Bolo[bolo].captr[cap].bb.num < 0) continue;
				for(num=0; num<Bolo[bolo].nbreglages; num++) {
					regl = &(Bolo[bolo].reglage[num]);
					if(regl->capt == cap) { der = cap; break; }
				}
			}
			/* tour des capteurs, seuls ceux qui ont au moins un reglage accessible seront affiches */
			for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
				if(Bolo[bolo].captr[cap].bb.num < 0) continue;
				for(num=0; num<Bolo[bolo].nbreglages; num++) {
					regl = &(Bolo[bolo].reglage[num]);
					if(regl->capt == cap) break;
				}
				if(num >= Bolo[bolo].nbreglages) continue;
				fin_de_serie = (cap == der);
				if(cap == 0) {
					i = PanelKeyB(p,Bolo[bolo].nom,DETEC_STATUTS,&(Bolo[bolo].lu),5);
						PanelItemColors(p,i,SambaRougeVertJauneOrange,3);
						PanelItemSouligne(p,i,fin_de_serie);
					i = PanelText(p,0,Bolo[bolo].adresse,5);
						PanelItemSouligne(p,PanelItemSelect(p,i,0),fin_de_serie);
				} else for(k=0; k<2; k++) i = PanelItemSouligne(p,PanelBlank(p),fin_de_serie);
				PanelItemBarreDroite(p,i,1);
				i = PanelText(p,0,ModeleVoie[Bolo[bolo].captr[cap].modele].nom,8);
					PanelItemSouligne(p,PanelItemSelect(p,i,0),fin_de_serie);
				i = PanelOctet(p,0,&(Bolo[bolo].captr[cap].bb.serial));
					PanelItemFormat(p,i,"%3d"); PanelItemLngr(p,i,3);
					PanelItemSouligne(p,PanelItemSelect(p,i,0),fin_de_serie);
				i = PanelText(p,0,Numeriseur[Bolo[bolo].captr[cap].bb.num].code_rep,4);
					PanelItemFormat(p,i,"%-3s"); PanelItemLngr(p,i,4);
					PanelItemSouligne(p,PanelItemSelect(p,i,0),fin_de_serie);
				PanelItemBarreDroite(p,i,1);
				k = 0;
				for(num=0; num<Bolo[bolo].nbreglages; num++) {
					regl = &(Bolo[bolo].reglage[num]);
					if(regl->capt != cap) continue;
					PanelItemSouligne(p,LectConsigneAjoute(type_panel,bolo,regl),fin_de_serie);
					k++;
				}
				for( ; k<max_regl; k++) PanelItemSouligne(p,PanelBlank(p),fin_de_serie);
				fin_de_serie = 0;
			}
		}
		//- printf("  _____\n");
		if(type_panel == LECT_CONS_CMDE) {
			OpiumEnterFctn(p->cdr,LectConsignesDecode,0);
			PanelBoutonText(p,PNL_APPLY, L_("Charger tout"));  PanelOnApply(p,LectConsignesBoutonLoad,0);
			PanelBoutonText(p,PNL_CANCEL,L_("Fermer sans sauver"));
			PanelBoutonText(p,PNL_OK,    L_("Sauver et fermer")); PanelOnOK(p,LectConsignesBoutonSave,0);
			OpiumPosition(p->cdr,10,50);
		}
	}

	/*
		Panel "Traitement et trigger"
	 */
	/* nombre de traitements a afficher */
	nb_canaux = 0; k = 0; der = -1;
	if(BoloNb > 1) for(vm=0; vm<ModeleVoieNb; vm++) if(ModeleVoie[vm].utilisee) { nb_canaux++; k++; der = vm; }
	for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].local) {
		for(cap=0; cap<Bolo[bolo].nbcapt; cap++) if(Bolo[bolo].captr[cap].voie >= 0) nb_canaux++;
	}
	//? cols = 5 + (3 * TRMT_NBCLASSES) + 2 + ((VoiesNb > 1)? 1: 0);
	cols = 5 + (3 * TRMT_NBCLASSES) + 2 + 1;
	nbitems = cols * (1 + nb_canaux);

	/* dimensionnement du panel */
	if(PanelItemNb(pLectTraitements) != nbitems) /* cas '>' mal gere */ {
		PanelDelete(pLectTraitements);
		pLectTraitements = 0;
	}
	if(!pLectTraitements) pLectTraitements = PanelCreate(nbitems);

	p = pLectTraitements;
	PanelErase(p); PanelColumns(p,cols); PanelMode(p,PNL_BYLINES);
	//+ PanelVerrouEntete(p,1+k);
	/* entete */
	PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,"detec.",L_("etat"),4),0),1);
	i = PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"pos.",5),0),1);
		PanelItemBarreDroite(p,i,1);
	PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,L_("canal"),8),0),1);
	PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0," BB",3),0),1);
	i = PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"Rep",3),0),1);
		PanelItemBarreDroite(p,i,1);
	for(classe=0; classe<TRMT_NBCLASSES; classe++) {
		PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,TrmtClasseTrad[classe],15),0),1);
		PanelItemSouligne(p,PanelItemLngr(p,PanelBlank(p),3),1);
		i = PanelItemSouligne(p,PanelItemLngr(p,PanelBlank(p),3),1);
		PanelItemBarreDroite(p,i,1);
	}
	PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"Trigger",20),0),1);
	i = PanelItemSouligne(p,PanelItemLngr(p,PanelBlank(p),6),1);
		PanelItemBarreDroite(p,i,1);
	/* if(VoiesNb > 1) */ PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,L_("Usage"),6),0),1);
	/* traitements standards */
	k = 0;
	if(BoloNb > 1) for(vm=0; vm<ModeleVoieNb; vm++) if(ModeleVoie[vm].utilisee) {
		if(!k) { i = PanelText(p,"Standard"," ",8); k = 1; } // nom voie et etat
		else i = PanelBlank(p);
		fin_de_serie = 1; // en cas que (..!) c'est la derniere
		for(vmsvt=vm+1; vmsvt<ModeleVoieNb; vmsvt++) if(ModeleVoie[vmsvt].utilisee) {
			fin_de_serie = (ModeleVoie[vmsvt].physique != ModeleVoie[vm].physique);
			break;
		}
		PanelItemSouligne(p,PanelItemSelect(p,i,0),(vm == der));
		i = PanelItemSouligne(p,PanelBlank(p),(vm == der)); // position
			PanelItemBarreDroite(p,i,1);
		i = PanelText(p,0,ModeleVoie[vm].nom,8);
			PanelItemSouligne(p,PanelItemSelect(p,i,0),fin_de_serie);
		for(k=0; k<2; k++) i = PanelItemSouligne(p,PanelBlank(p),fin_de_serie);
		PanelItemBarreDroite(p,i,1);
		for(classe=0; classe<TRMT_NBCLASSES; classe++) {
			i = PanelListS(p,0,TrmtListe,&(VoieStandard[vm].numtrmt[classe]),MAXFILTRENOM);
			PanelItemLngr(p,i,MAXFILTRENOM); PanelItemSouligne(p,i,fin_de_serie);
		#ifdef A_LA_GG
			if(VoieStandard[vm].def.trmt[classe].type == TRMT_INTEDIF)
				i = PanelText(p,0,VoieStandard[vm].def.trmt[classe].texte,TRMT_TEXTE);
			else 
		#endif /* A_LA_GG */
				i = PanelInt(p,0,&(VoieStandard[vm].def.trmt[classe].p1));
			PanelItemLngr(p,i,3); PanelItemSouligne(p,i,fin_de_serie);
			i = PanelInt(p,"x",&(VoieStandard[vm].def.trmt[classe].gain));
			PanelItemLngr(p,i,3); PanelItemSouligne(p,i,fin_de_serie);
			PanelItemBarreMilieu(p,i,-1); PanelItemBarreDroite(p,i,1);
		}
		i = PanelText(p,0,VoieStandard[vm].txttrgr,TRGR_NOM);
		PanelItemLngr(p,i,20); PanelItemSouligne(p,i,fin_de_serie);
		i = PanelKeyB(p,0,L_("fixe/regule"),&(VoieStandard[vm].def.trgr.regul_demandee),6);
		PanelItemLngr(p,i,6); PanelItemSouligne(p,i,fin_de_serie);
		PanelItemColors(p,i,SambaJauneVertOrangeBleu,2);
		/* if(VoiesNb > 1) */ { i = PanelItemSelect(p,PanelBlank(p),0); PanelItemSouligne(p,i,fin_de_serie); }
	}
	/* tour des bolos: for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].local) */
	pour_tout(j,BoloNb) {
		bolo = OrdreDetec[j].num;
		if(!Bolo[bolo].local) continue;
		der = -1;
		for(cap=0; cap<Bolo[bolo].nbcapt; cap++) if(Bolo[bolo].captr[cap].voie >= 0) der = cap;
		/* tour des capteurs, le dernier capteur sera suivi d'une ligne de separation */
		for(cap=0; cap<Bolo[bolo].nbcapt; cap++) if((voie = Bolo[bolo].captr[cap].voie) >= 0) {
			fin_de_serie = (cap == der);
			if(cap == 0) {
				i = PanelKeyB(p,Bolo[bolo].nom,DETEC_STATUTS,&(Bolo[bolo].lu),4);
				PanelItemColors(p,i,SambaRougeVertJauneOrange,3);
				PanelItemSouligne(p,i,fin_de_serie);
				i = PanelText(p,0,Bolo[bolo].adresse,4);
				PanelItemSouligne(p,PanelItemSelect(p,i,0),fin_de_serie);
			} else for(k=0; k<2; k++) i = PanelItemSouligne(p,PanelBlank(p),fin_de_serie);
			PanelItemBarreDroite(p,i,1);
//			if(VoieManip[voie].pseudo) {
//				r = c = &(nomvoie[voie][0]); strcpy(r,VoieManip[voie].nom); ItemJusquA(' ',&r);
//				i = PanelText(p,0,c,8);
//			} else i = PanelText(p,0,ModeleVoie[Bolo[bolo].captr[cap].modele].nom,8);
			i = PanelText(p,0,VoieManip[voie].prenom,8);
				PanelItemSouligne(p,PanelItemSelect(p,i,0),fin_de_serie);
			if(Bolo[bolo].captr[cap].bb.serial == 0xFF) i = PanelBlank(p); 
			else { i = PanelOctet(p,0,&(Bolo[bolo].captr[cap].bb.serial)); PanelItemFormat(p,i,"%3d"); }
				PanelItemLngr(p,i,3);
				PanelItemSouligne(p,PanelItemSelect(p,i,0),fin_de_serie);
			i = PanelText(p,0,Numeriseur[Bolo[bolo].captr[cap].bb.num].code_rep,4);
				PanelItemFormat(p,i,"%-3s"); PanelItemLngr(p,i,4);
				PanelItemSouligne(p,PanelItemSelect(p,i,0),fin_de_serie);
			PanelItemBarreDroite(p,i,1);
			for(classe=0; classe<TRMT_NBCLASSES; classe++) {
				i = PanelListS(p,0,TrmtListe,&(VoieManip[voie].numtrmt[classe]),MAXFILTRENOM);
				PanelItemExit(p,i,LectConsigneTouche,&(Bolo[bolo]));
				PanelItemReturn(p,i,LectConsigneTouche,&(Bolo[bolo]));
				// printf("%s %s: trmt #%d (item #%d)\n",VoieManip[voie].nom,TrmtClasseTitre[classe],VoieManip[voie].numtrmt[classe],i);
				PanelItemLngr(p,i,15);
				PanelItemColors(p,i,SambaTrmtCouleurs,TRMT_NBTYPES+1);
				PanelItemSouligne(p,i,fin_de_serie);
			#if defined(TRMT_DECONVOLUE) || defined(A_LA_GG)
				if((VoieManip[voie].def.trmt[classe].type == TRMT_DECONVOLUE) || (VoieManip[voie].def.trmt[classe].type == TRMT_INTEDIF))
					i = PanelText(p,0,VoieManip[voie].def.trmt[classe].texte,TRMT_TEXTE);
				else 
			#endif /* TRMT_DECONVOLUE || A_LA_GG */
					i = PanelInt(p,0,&(VoieManip[voie].def.trmt[classe].p1));
				PanelItemExit(p,i,LectConsigneTouche,&(Bolo[bolo]));
				PanelItemReturn(p,i,LectConsigneTouche,&(Bolo[bolo]));
				PanelItemLngr(p,i,3); PanelItemSouligne(p,i,fin_de_serie);
				i = PanelInt(p,"x",&(VoieManip[voie].def.trmt[classe].gain));
				PanelItemExit(p,i,LectConsigneTouche,&(Bolo[bolo]));
				PanelItemReturn(p,i,LectConsigneTouche,&(Bolo[bolo]));
				PanelItemLngr(p,i,3); PanelItemSouligne(p,i,fin_de_serie);
				PanelItemBarreMilieu(p,i,-1); PanelItemBarreDroite(p,i,1);
			}
			i = PanelText(p,0,VoieManip[voie].txttrgr,TRGR_NOM);
			PanelItemExit(p,i,LectConsigneTouche,&(Bolo[bolo]));
			PanelItemReturn(p,i,LectConsigneTouche,&(Bolo[bolo]));
			// PanelItemLngr(p,i,TRGR_NOM);
			PanelItemSouligne(p,i,fin_de_serie);
			i = PanelKeyB(p,0,L_("fixe/regule"),&(VoieManip[voie].def.trgr.regul_demandee),6);
			PanelItemExit(p,i,LectConsigneTouche,&(Bolo[bolo]));
			PanelItemReturn(p,i,LectConsigneTouche,&(Bolo[bolo]));
			//- i = PanelKeyB(p,0," / ",&(VoieManip[voie].def.trgr.regul_demandee),6);
			PanelItemLngr(p,i,6);
			PanelItemColors(p,i,SambaJauneVertOrangeBleu,2);
			PanelItemSouligne(p,i,fin_de_serie);
			i = PanelKeyB(p,0,L_(ARCH_BUFF_CLES),&(VoieManip[voie].sauvee),6);
			PanelItemExit(p,i,LectConsigneTouche,&(Bolo[bolo]));
			PanelItemReturn(p,i,LectConsigneTouche,&(Bolo[bolo]));
			PanelItemLngr(p,i,6);
			PanelItemColors(p,i,SambaOrangeVertJauneBleuViolet,5);
			PanelItemSouligne(p,i,fin_de_serie);
		}
	}
	OpiumEnterFctn(p->cdr,LectTrmtEnter,0);
	PanelOnApply(p,LectTrmtExit,0); PanelBoutonText(p,PNL_APPLY,L_("Sauver"));
	PanelOnOK(p,LectTrmtExit,0); PanelBoutonText(p,PNL_OK,L_("Sauver et fermer"));

}
/* ========================================================================== */
static int LectConsignesDecode(Cadre cdr, void *arg) {
	int bolo,bb,num,k; TypeReglage *regl;
	
	printf("%s/ ----- Affichage des consignes -----\n",DateHeure());
	AutomVerifieConditions("          ",0);
	for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].local) {
		for(num=0; num<Bolo[bolo].nbreglages; num++) if((regl = &(Bolo[bolo].reglage[num]))) {
			if(((k = regl->ress) >= 0) && (bb = regl->bb)) {
				NumeriseurRessUserDecode(&(Numeriseur[bb]),k,regl->std,&(regl->val.i),&(regl->val.r));
			}
		}
	}
	return(0);
}
/* ========================================================================== */
static int LectConsigneAjoute(LECT_CONS_TYPE type, int bolo, TypeReglage *regl) {
	int bb,k,i; // char fmt[8];
	char *nom;
	TypeNumeriseur *numeriseur;
	TypeBNModlRessource *ress; Panel p;

	p = pReglagesConsignes[type];
	bb = regl->bb;
	numeriseur = &(Numeriseur[bb]);
	nom = ModeleDet[Bolo[bolo].type].regl[regl->num].surnom;
	i = 0;
	if((k = regl->ress) >= 0) {
		ress = ModeleBN[numeriseur->def.type].ress + k;
		switch(type) {
			case LECT_CONS_ETAT:
				switch(ress->type) {
					case RESS_INT:   i = PanelInt(p,nom,&(numeriseur->ress[k].val.i)); PanelItemFormat(p,i,"%5d"); PanelItemLngr(p,i,5); break;
					case RESS_FLOAT: i = PanelFloat(p,nom,&(numeriseur->ress[k].val.r)); PanelItemFormat(p,i,"%7.3f"); PanelItemLngr(p,i,8); break;
					case RESS_CLES:  i = PanelKey(p,nom,ress->lim_k.cles,&(numeriseur->ress[k].val.i),NUMER_RESS_VALTXT); PanelItemFormat(p,i,"%6s"); PanelItemLngr(p,i,6); break;
					case RESS_PUIS2: i = PanelText(p,nom,numeriseur->ress[k].cval,4); PanelItemFormat(p,i,"%4d"); PanelItemLngr(p,i,4); break;
				}
				break;
			case LECT_CONS_CMDE:
				switch(ress->type) {
					case RESS_INT:   i = PanelInt(p,nom,&(regl->val.i)); PanelItemFormat(p,i,"%-5d"); PanelItemLngr(p,i,5); break;
					case RESS_FLOAT: i = PanelFloat(p,nom,&(regl->val.r)); PanelItemFormat(p,i,"%-7.3f"); PanelItemLngr(p,i,8); break;
					case RESS_CLES:  i = PanelKey(p,nom,ress->lim_k.cles,&(regl->val.i),NUMER_RESS_VALTXT); PanelItemFormat(p,i,"%-6s"); PanelItemLngr(p,i,6); break;
					case RESS_PUIS2: i = PanelText(p,nom,regl->user,4); PanelItemFormat(p,i,"%-4d"); PanelItemLngr(p,i,4); break;
				}
				// if(ress->type == RESS_FLOAT) printf(". Ajoute '%s.(%s=%s)=%g ('%s')\n",Bolo[bolo].nom,nom,ModeleDet[Bolo[bolo].type].regl[regl->num].nom,regl->val.r,regl->std);
				break;
			default:  i = PanelItemSelect(p,PanelText(p,nom,"(tbd)",5),0);
		}
	} else {
		if(regl->user[0]) i = PanelItemSelect(p,PanelText(p,nom,regl->user,6),0);
		else i = PanelItemSelect(p,PanelText(p,nom,L_("neant"),5),0);
		PanelItemLngr(p,i,6);
	}
	/* switch(regl->cmde) {
		case REGL_MODUL: strcpy(fmt,"%6.3f"); break;
		case REGL_COMP:  strcpy(fmt,"%6.3f"); break;
		case REGL_CORR:  strcpy(fmt,"%5.2f"); break;
		case REGL_FET_MASSE: strcpy(fmt,"%6s"); if(numeriseur->vsnid >= 2) PanelItemSelect(p,i,0); break;
		case REGL_D2:    strcpy(fmt,"%4d"); break;
		case REGL_D3:    strcpy(fmt,"%4d"); break;
		case REGL_POLAR: strcpy(fmt,"%6.2f"); break;
		case REGL_GAIN:  if(numeriseur->vsnid == 1) strcpy(fmt,"%6s"); else strcpy(fmt,"%5d"); break;
		default: fmt[0] = '\0';
	} */
	if(i) {
		PanelItemBarreDroite(p,i,1);
		/* if(DetecReglagesLocaux && (type == LECT_CONS_CMDE)) {
			PanelItemReturn(p,i,LectConsigneModifie,0);
			DetecReglagesLocaux[i - 1] = regl;
		} */
		if(type == LECT_CONS_CMDE) {
			PanelItemExit(p,i,LectConsigneTouche,&(Bolo[bolo]));
			PanelItemReturn(p,i,LectConsigneModifie,&(Bolo[bolo].reglage[regl->num]));
		}
	}
	return(i);
}
/* ========================================================================== */
static void LectConsigneChange(TypeReglage *regl) {
	int bolo,cap,bb,num; char lu[NUMER_RESS_VALTXT];
	TypeDetecteur *detectr; TypeNumeriseur *numeriseur;
	TypeBNModele *modele_bn; TypeBNModlRessource *ress;

	if(!regl) return;
	num = regl->ress;
	bolo = regl->bolo;
	cap = regl->capt; if(cap < 0) cap = 0;
	detectr = Bolo + bolo;
	bb = regl->bb;
	if(bb >= 0) {
		numeriseur = &(Numeriseur[bb]);
		if(num >= 0) {
			modele_bn = &(ModeleBN[numeriseur->def.type]); ress = modele_bn->ress + num;
			//		if(ress->type != RESS_PUIS2) {
			//			if(ress->type == RESS_FLOAT) sprintf(lu,"%g",regl->val.r);
			//			else sprintf(lu,"%d",regl->val.i);
			//			NumeriseurRessUserDecode(numeriseur,num,lu,&(regl->val.i),&(regl->val.r)); /* proprifie lu */
			NumeriseurRessValEncode(numeriseur,num,lu,regl->val.i,regl->val.r);
			if(strcmp(regl->user,lu)) {
				if(!detectr->a_sauver) printf("> Detecteur %s modifie\n",detectr->nom);
				printf(". %s.%s modifie de %s a %s.\n",detectr->nom,regl->nom,regl->user,lu);
				strcpy(regl->user,lu);
				detectr->a_sauver = 1;
			}
			//		}
		} else printf("! %s.%s: pas de ressource associee dans la %s\n",
			detectr->nom,ModeleDet[detectr->type].regl[regl->num].nom,numeriseur->nom);
	} else printf("! %s.%s: pas de numeriseur designe\n",
		detectr->nom,ModeleDet[detectr->type].regl[regl->num].nom);
	ReglageEffetDeBord(regl,0); // "       "
	ReglageRafraichiTout(bb,num,regl);
}
/* ========================================================================== */
static void LectConsigneCharge(TypeReglage *regl, char autorise) {
	int bolo,cap,bb,num,k; // ,vsn;
	TypeADU hval; float fval; int ival;
	TypeDetecteur *detectr; TypeNumeriseur *numeriseur;
	TypeBNModele *modele_bn; TypeBNModlRessource *ress;

	if(!regl) return;
	/* if((regl->cmde == REGL_POLAR) && EtatRegen) {
		if(ress->type == RESS_PUIS2) sscanf(regl->user,"%g",&fval);
		else if(ress->type == RESS_FLOAT) fval = regl->val.r; else fval = (float)(regl->val.i);
		if((fval > 0.0) || (fval < 0.0)) {
			if(autorise) OpiumError("Source de regeneration active: polarisation inhibee");
			return;
		}
	} */
	// printf("(%s) %s = %d\n",__func__,regl->nom,regl->val.i);
	ival = 0; fval = 0.0; // GCC
	bolo = regl->bolo;
	cap = regl->capt; if(cap < 0) cap = 0;
	detectr = Bolo + bolo;
	bb = regl->bb;
	numeriseur = &(Numeriseur[bb]);
	if((num = regl->ress) >= 0) {
		modele_bn = &(ModeleBN[numeriseur->def.type]); ress = modele_bn->ress + num;
		if(ress->type == RESS_PUIS2) {
			strcpy(numeriseur->ress[num].cval,regl->user);
			NumeriseurRessUserChange(numeriseur,num);
		} else {
			if(ress->type == RESS_FLOAT) fval = numeriseur->ress[num].val.r = regl->val.r;
			else ival = numeriseur->ress[num].val.i = regl->val.i;
			NumeriseurRessValChangee(numeriseur,num);
		}
		regl->modifie = 1;
//		if(strcmp(regl->user,numeriseur->ress[num].cval)) {
			if(!detectr->a_sauver) printf("            > Detecteur %s modifie\n",detectr->nom);
			printf("            . %s.%s modifie de %s a %s.\n",detectr->nom,regl->nom,regl->user,numeriseur->ress[num].cval);
			strcpy(regl->user,numeriseur->ress[num].cval);
			detectr->a_sauver = 1;
//		}
		// vsn = numeriseur->vsnid;
	#ifdef CONTROLE_MANU
		if(autorise) NumeriseurAutoriseAcces(numeriseur,1,2);
		hval = NumeriseurRessourceCharge(numeriseur,num,0);
	#else
		hval = NumeriseurRessourceChargeAuto(numeriseur,num,0,0);
	#endif
		printf("            . %s: %12s  = %-8s (transmis: %s)\n",
			detectr->nom,ModeleDet[detectr->type].regl[regl->num].nom,numeriseur->ress[num].cval,RepartiteurValeurEnvoyee);
		if((k = regl->ressneg) >= 0) {
			ress = modele_bn->ress + k;
			if(ress->type == RESS_FLOAT) numeriseur->ress[k].val.r = -fval;
			else numeriseur->ress[k].val.i = -ival;
			NumeriseurRessValChangee(numeriseur,k);
		#ifdef CONTROLE_MANU
			hval = NumeriseurRessourceCharge(numeriseur,k,0);
		#else
			hval = NumeriseurRessourceChargeAuto(numeriseur,k,0,0);
		#endif
			printf("            . %s: %12s- = %-8s (transmis: %s)\n",
				detectr->nom,ModeleDet[detectr->type].regl[regl->num].nom,numeriseur->ress[k].cval,RepartiteurValeurEnvoyee);
		}
		if((k = regl->resspos) >= 0) {
			ress = modele_bn->ress + k;
			if(ress->type == RESS_FLOAT) numeriseur->ress[k].val.r = fval;
			else numeriseur->ress[k].val.i = ival;
			NumeriseurRessValChangee(numeriseur,k);
		#ifdef CONTROLE_MANU
			hval = NumeriseurRessourceCharge(numeriseur,k,0);
		#else
			hval = NumeriseurRessourceChargeAuto(numeriseur,k,0,0);
		#endif
			printf("            . %s: %12s+ = %-8s (transmis: %s)\n",
				detectr->nom,ModeleDet[detectr->type].regl[regl->num].nom,numeriseur->ress[k].cval,RepartiteurValeurEnvoyee);
		}
	#ifdef CONTROLE_MANU
		if(autorise) NumeriseurAutoriseAcces(numeriseur,0,2);
	#endif
	} else printf("            ! %s: %12s  : pas de ressource associee dans la %s\n",
		detectr->nom,ModeleDet[detectr->type].regl[regl->num].nom,numeriseur->nom);
	ReglageEffetDeBord(regl,"            ");
	ReglageRafraichiTout(bb,num,regl);
}
/* ========================================================================== */
static int LectConsigneTouche(Panel panel, int item, void *arg) {
	TypeDetecteur *detectr;

	detectr = (TypeDetecteur *)arg;
	if(PanelItemEstModifie(panel,item)) {
	#ifdef TRMT_DECONVOLUE
	/* !! Probleme !!: <voie> et <classe> indetermines ici (idem TRMT_INTEDIF) */
		if(VoieManip[voie].def.trmt[classe].type == TRMT_DECONVOLUE) {
			char *cur,*p1,*p2,texte[TRMT_TEXTE];
			strcpy(texte,VoieManip[voie].def.trmt[classe].texte);
			cur = texte; p1 = ItemJusquA('/',&cur); p2 = ItemJusquA('/',&cur);
			sscanf(p1,"%d",&(VoieManip[voie].def.trmt[classe].p1));
			sscanf(p2,"%d",&(VoieManip[voie].def.trmt[classe].p2));
			sscanf(cur,"%g",&(VoieManip[voie].def.trmt[classe].p3));
		} else
	#endif
	#ifdef A_LA_GG
		if(VoieManip[voie].def.trmt[classe].type == TRMT_INTEDIF) {
			char *cur,*p1,*p2,texte[TRMT_TEXTE];
			strcpy(texte,VoieManip[voie].def.trmt[classe].texte);
			cur = texte; p1 = ItemJusquA('/',&cur);
			sscanf(p1,"%d",&(VoieManip[voie].def.trmt[classe].p1));
			sscanf(cur,"%g",&(VoieManip[voie].def.trmt[classe].p3));
		}
	#endif
		if(!detectr->a_sauver) printf("> %s du detecteur %s modifie(e)s\n",(panel->cdr)->titre,detectr->nom);
		detectr->a_sauver = 1;
	}
	return(0);
}
/* ========================================================================== */
static int LectConsigneModifie(Panel panel, int item, void *arg) {
	TypeReglage *regl;
	
	LogAddFile(SambaJournal);
	// printf(". item #%-2d: %6s=%-6s (%s)\n",item-1,panel->items[item-1].texte,panel->items[item-1].ptrval,PanelItemEstModifie(panel,item)?"MODIFIE":"inchange");
	// printf("  . item #%d modifie: %s=%s (valeur en cours: 0x%08X)\n",item-1,panel->items[item-1].texte,panel->items[item-1].ptrval,*(hexa *)(panel->items[item-1].adrs));
	PanelItemRelit(panel,item);
	// printf("  . nouvelle valeur: 0x%08X\n",*(hexa *)(panel->items[item-1].adrs));
	if(DetecReglagesLocaux) {
//		LectConsigneCharge(DetecReglagesLocaux[item-1],1);
		regl = (TypeReglage *)arg;
		LectConsigneCharge(regl,1);
	#ifndef CONTROLE_MANU
		NumeriseurChargeFin(0);
	#endif
	}
	LogOffFile();
	return(0);
}
/* ========================================================================== */
static int LectConsignesBoutonLoad(Panel p, void *ptr) {
	int bolo,num,bb,i;
	TypeDetecteur *detectr; TypeReglage *regl;
#ifdef CONTROLE_MANU
	int bb_prec;
#endif
	
	if(RegenEnCours) {
		if(!OpiumChoix(2,SambaNonOui,"Actuellement en regeneration: es-tu sur de vouloir appliquer ces consignes?")) return(0);
	}

	printf("%s/ ========== Chargement de toutes les consignes des detecteurs actifs locaux ==========\n",DateHeure());
#ifdef CONTROLE_MANU
	bb_prec = -1;
#endif
	for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].local && (Bolo[bolo].lu == DETEC_OK)) {
		detectr = Bolo + bolo;
		for(num=0; num<detectr->nbreglages; num++) {
			regl = &(detectr->reglage[num]);
			bb = regl->bb;
		#ifdef CONTROLE_MANU
			if(bb != bb_prec) {
				if(bb_prec >= 0) NumeriseurAutoriseAcces(&(Numeriseur[bb_prec]),0,2);
				NumeriseurAutoriseAcces(&(Numeriseur[bb]),1,2);
			}
			bb_prec = bb;
		#endif
			if(Numeriseur[bb].bloque) {
				for(i=0; i<Numeriseur[bb].nbprocs; i++) if(!strcmp(Numeriseur[bb].proc[i].nom,"deblocage")) break;
				if(i < Numeriseur[bb].nbprocs) {
					NumeriseurProcExec(&(Numeriseur[bb]),i);
					Numeriseur[bb].bloque = 0;
					SambaNote("Deblocage du numeriseur %s\n",Numeriseur[bb].nom);
				}
			}
			LectConsigneCharge(regl,0);
		}
	}
#ifdef CONTROLE_MANU
	if(bb_prec >= 0) NumeriseurAutoriseAcces(&(Numeriseur[bb_prec]),0,2);
#else
	NumeriseurChargeFin(0);
#endif
	printf("%s/ ......................... Chargement des consignes termine ..........................\n",DateHeure());

	return(0);
}
/* ========================================================================== */
static int LectConsignesBoutonSave(Panel p, void *ptr) {
	int bolo,num;

	/* On change le standard de toutes facons */
	for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].local) {
		for(num=0; num<Bolo[bolo].nbreglages; num++) {
			LectConsigneChange(&(Bolo[bolo].reglage[num]));
			strcpy(Bolo[bolo].reglage[num].std,Bolo[bolo].reglage[num].user);
		}
	}
	DetecteursVoisinage(0); /* pour Bolo[].a_lire donc VoieManip[].lue, et les voisins aussi, du reste .. */
	DetecteurSauveSelectReglages(0);
	printf("%s/ ..... Consignes fermees .....\n",DateHeure());
	return(0);
}
/* ========================================================================== */
int LectTrmtEnter(Cadre cdr, void *ptr) {
	int bolo,cap,voie,vm,classe; int k;

	// printf("Conversion des donnees par %s\n",__func__);
	k = 0; while(FiltreListe[k][0]) k++;
	pour_tout(bolo,BoloNb) {
		pour_tout(cap,Bolo[bolo].nbcapt) if((voie = Bolo[bolo].captr[cap].voie) >= 0) {
			pour_tout(classe,TRMT_NBCLASSES) {
				if(VoieManip[voie].def.trmt[classe].std) VoieManip[voie].numtrmt[classe] = TRMT_FILTRAGE;
				else if(VoieManip[voie].def.trmt[classe].type == TRMT_FILTRAGE) {
					if((VoieManip[voie].def.trmt[classe].p1 < 0) || (VoieManip[voie].def.trmt[classe].p1 >= k))
					VoieManip[voie].def.trmt[classe].p1 = 0;
					VoieManip[voie].numtrmt[classe] = TRMT_NBTYPES + VoieManip[voie].def.trmt[classe].p1;
				} else VoieManip[voie].numtrmt[classe] = VoieManip[voie].def.trmt[classe].type;
			}
			SambaTrgrEncode(&(VoieManip[voie].def.trgr),VoieManip[voie].txttrgr,TRGR_NOM);
			strcpy(VoieManip[voie].trgravant,VoieManip[voie].txttrgr);
		}
	}
	bolo = BoloStandard;
	pour_tout(vm,ModeleVoieNb) {
		pour_tout(classe,TRMT_NBCLASSES) {
			if(VoieStandard[vm].def.trmt[classe].type == TRMT_FILTRAGE) {
				if((VoieStandard[vm].def.trmt[classe].p1 < 0) || (VoieStandard[vm].def.trmt[classe].p1 >= k))
				VoieStandard[vm].def.trmt[classe].p1 = 0;
				VoieStandard[vm].numtrmt[classe] = TRMT_NBTYPES + VoieStandard[vm].def.trmt[classe].p1;
			} else VoieStandard[vm].numtrmt[classe] = VoieStandard[vm].def.trmt[classe].type;
		}
		SambaTrgrEncode(&(VoieStandard[vm].def.trgr),VoieStandard[vm].txttrgr,TRGR_NOM);
	}
	// printf("Conversion des donnees terminee\n");
	return(0);
}
/* ========================================================================== */
int LectTrmtExit(Panel panel, void * ptr) {
	int bolo,cap,voie,vm,classe; int i; char txttrgr[TRGR_NOM];
	char *r,*c; char dlm1,dlm2,dlm3; char regul; char trig_change;
	int nb; TypeDetecteur *detectr;

	nb = PanelItemNb(pLectTraitements);
	for(i=0; i<nb; i++) if(PanelItemEstModifie(pLectTraitements,i+1)) {
		if((detectr = (TypeDetecteur *)PanelItemArg(pLectTraitements,i+1))) {
			if(!detectr->a_sauver) printf("> Detecteur %s modifie\n",detectr->nom);
			detectr->a_sauver = 1;
		}
	}
	trig_change = 0;
	bolo = BoloStandard;
	for(vm=0; vm<ModeleVoieNb; vm++) {
		for(classe=0; classe<TRMT_NBCLASSES; classe++) {
			if(VoieStandard[vm].numtrmt[classe] >= TRMT_NBTYPES) {
				VoieStandard[vm].def.trmt[classe].type = TRMT_FILTRAGE;
				VoieStandard[vm].def.trmt[classe].p1 = VoieStandard[vm].numtrmt[classe] - TRMT_NBTYPES;
			} else VoieStandard[vm].def.trmt[classe].type = VoieStandard[vm].numtrmt[classe];
		}
		// ===== A remplacer par:
		strcpy(txttrgr,VoieStandard[vm].txttrgr);
		r = txttrgr;
		c = ItemTrouve(&r,"<>@",&dlm1);
		if(strcmp(c,"--") && strcmp(c,"indetermine")) {
			VoieStandard[vm].def.trgr.sens = 1;
			if(!strcmp(c,"montee")) { i = TRGR_FRONT; VoieStandard[vm].def.trgr.sens = 2; }
			else if(!strcmp(c,"desc.")) { i = TRGR_FRONT; VoieStandard[vm].def.trgr.sens = 0; }
			else for(i=0; i<TRGR_NBCABLAGES; i++) if(!strcmp(c,TrgrTypeListe[i])) break;
			if(i >= TRGR_NBCABLAGES) VoieStandard[vm].def.trgr.type = NEANT;
			else {
				VoieStandard[vm].def.trgr.type = i;
				if((i == TRGR_SEUIL) || (i == TRGR_DERIVEE)) {
					c = ItemTrouve(&r,"<>=",&dlm2);
					if(dlm1 == '<') { sscanf(c,"%g",&(VoieStandard[vm].def.trgr.maxampl)); }
					else if(dlm1 == '>') { sscanf(c,"%g",&(VoieStandard[vm].def.trgr.minampl)); }
					if((dlm2 != '=') && (dlm2 != '\0')) {
						c = ItemTrouve(&r,"<>=",&dlm3);
						if(dlm2 == '<') { sscanf(c,"%g",&(VoieStandard[vm].def.trgr.maxampl)); }
						else if(dlm2 == '>') { sscanf(c,"%g",&(VoieStandard[vm].def.trgr.minampl)); }
					}
					if(((dlm1 == '<') && (dlm2 != '>')) || ((dlm2 == '<') && (dlm1 != '>'))) VoieStandard[vm].def.trgr.sens = 0;
					else if(((dlm1 == '>') && (dlm2 != '<')) || ((dlm2 == '>') && (dlm1 != '<'))) VoieStandard[vm].def.trgr.sens = 2;
					if((i == TRGR_DERIVEE) && ((dlm2 == '=') || (dlm3 == '='))) {
						c = ItemJusquA(0,&r);
						sscanf(c,"%g",&(VoieStandard[vm].def.trgr.montee));
					}
				} else if(i == TRGR_FRONT) {
					c = ItemJusquA('>',&r);
					sscanf(c,"%g",&(VoieStandard[vm].def.trgr.minampl));
					c = ItemJusquA(0,&r);
					sscanf(c,"%g",&(VoieStandard[vm].def.trgr.montee));
				} else if(i == TRGR_ALEAT) {
					c = ItemJusquA('*',&r);
					sscanf(c,"%g",&(VoieStandard[vm].def.trgr.porte));
					// c = ItemJusquA(0,&r);
					// sscanf(c,"%g",&(VoieStandard[vm].def.trgr.fluct));
				}
			}
		}
		// ======
	}
	for(bolo=0; bolo<BoloNb; bolo++) {
		for(cap=0; cap<Bolo[bolo].nbcapt; cap++) if((voie = Bolo[bolo].captr[cap].voie) >= 0) {
			vm = VoieManip[voie].type;
			for(classe=0; classe<TRMT_NBCLASSES; classe++) {
				if(VoieManip[voie].numtrmt[classe] == TRMT_FILTRAGE) /* c'est qu'on a mis 'Standard' */ {
					memcpy(&(VoieManip[voie].def.trmt[classe]),&(VoieStandard[vm].def.trmt[classe]),sizeof(TypeTraitement));
					VoieManip[voie].def.trmt[classe].std = 1;
				} else {
					VoieManip[voie].def.trmt[classe].std = 0;
					if(VoieManip[voie].numtrmt[classe] >= TRMT_NBTYPES) {
						VoieManip[voie].def.trmt[classe].type = TRMT_FILTRAGE;
						VoieManip[voie].def.trmt[classe].p1 = VoieManip[voie].numtrmt[classe] - TRMT_NBTYPES;
					} else VoieManip[voie].def.trmt[classe].type = VoieManip[voie].numtrmt[classe];
				}
			}
			regul = VoieManip[voie].def.trgr.regul_demandee;
			if(!strcmp(VoieManip[voie].trgravant,VoieManip[voie].txttrgr)) continue;
			trig_change = 1;
			vm = VoieManip[voie].type;
			strcpy(txttrgr,VoieManip[voie].txttrgr);
			r = txttrgr;
			c = ItemTrouve(&r,"<>@",&dlm1);
			if(!strcmp(c,"standard")) {
				VoieManip[voie].def.trgr.std = 1;
				memcpy(&(VoieManip[voie].def.trgr),&(VoieStandard[vm].def.trgr),sizeof(TypeTrigger));
			} else {
				VoieManip[voie].def.trgr.std = 0;
				if(strcmp(c,"--") && strcmp(c,"indetermine")) {
					VoieManip[voie].def.trgr.sens = 1;
					if(!strcmp(c,"montee")) { i = TRGR_FRONT; VoieManip[voie].def.trgr.sens = 2; }
					else if(!strcmp(c,"desc.")) { i = TRGR_FRONT; VoieManip[voie].def.trgr.sens = 0; }
					else for(i=0; i<TRGR_NBCABLAGES; i++) if(!strcmp(c,TrgrTypeListe[i])) break;
					if(i >= TRGR_NBCABLAGES) VoieManip[voie].def.trgr.type = NEANT;
					else {
						VoieManip[voie].def.trgr.type = i;
						if((i == TRGR_SEUIL) || (i == TRGR_DERIVEE)) {
							c = ItemTrouve(&r,"<>",&dlm2);
							if(dlm1 == '<') { sscanf(c,"%g",&(VoieManip[voie].def.trgr.maxampl)); }
							else if(dlm1 == '>') { sscanf(c,"%g",&(VoieManip[voie].def.trgr.minampl)); }
							if((dlm2 != '=') && (dlm2 != '\0')) {
								c = ItemTrouve(&r,"<>=",&dlm3);
								if(dlm2 == '<') { sscanf(c,"%g",&(VoieManip[voie].def.trgr.maxampl)); }
								else if(dlm2 == '>') { sscanf(c,"%g",&(VoieManip[voie].def.trgr.minampl)); }
							}
							if(((dlm1 == '<') && (dlm2 != '>')) || ((dlm2 == '<') && (dlm1 != '>'))) VoieManip[voie].def.trgr.sens = 0;
							else if(((dlm1 == '>') && (dlm2 != '<')) || ((dlm2 == '>') && (dlm1 != '<'))) VoieManip[voie].def.trgr.sens = 2;
							if((i == TRGR_DERIVEE) && ((dlm2 == '=') || (dlm3 == '='))) {
								c = ItemJusquA(0,&r);
								sscanf(c,"%g",&(VoieStandard[vm].def.trgr.montee));
							}
						} else if(i == TRGR_FRONT) {
							c = ItemJusquA('>',&r);
							sscanf(c,"%g",&(VoieManip[voie].def.trgr.minampl));
							c = ItemJusquA(0,&r);
							sscanf(c,"%g",&(VoieManip[voie].def.trgr.montee));
						} else if(i == TRGR_ALEAT) {
							c = ItemJusquA('*',&r);
							sscanf(c,"%g",&(VoieManip[voie].def.trgr.porte));
							// c = ItemJusquA(0,&r);
							// sscanf(c,"%g",&(VoieManip[voie].def.trgr.fluct));
						}
					}
				} else VoieManip[voie].def.trgr.type = NEANT;
			}
			VoieManip[voie].def.trgr.regul_demandee = regul;
			if(!Bolo[bolo].a_sauver) {
				printf("> Detecteur %s modifie\n",Bolo[bolo].nom);
				Bolo[bolo].a_sauver = 1;
			}
		}
	}

	if(trig_change && ArchSauve[EVTS]) {
		int usecs;
		printf("%s/ Ajustement manuel des seuils\n",DateHeure());
		SambaTempsEchantillon(DernierPointTraite,ArchT0sec,ArchT0msec,&SeuilVariationDate,&usecs);
		ArchiveSeuils(SeuilVariationDate);
		if(EdbActive) {
			sprintf(SeuilVariationId,"thr_%s_%d",Acquis[AcquisLocale].etat.nom_run,SeuilVariationDate);
			// ARG_STYLES ancien;
			// ancien = ArgStyle(ARG_STYLE_JSON); ArgPrint("SeuilVariation",SeuilVarListeDesc); ArgStyle(ancien);
			DbSendDesc(EdbServeur,SeuilVariationId,SeuilVarListeDesc);
			if(DbStatus) printf("%s/ Base de Donnees renseignee (Id: %s, rev: %s)\n",DateHeure(),DbId,DbRev);
			else {
				printf("%s/ ! Renseignement de la Base de Donnees par %s en erreur\n",DateHeure(),__func__);
				printf("%9s   . Nature: %s, raison: %s\n"," ",DbErreur,DbRaison);
				printf("%9s   . Id: %s, rev: %s\n"," ",SeuilVariationId,"nouvelle entree");
			}
		}
	}

	DetecteurSauveSelectReglages(0);
	return(0);
}
/* ========================================================================== */
static int LectConsignesRelit(Instrum instrum, int num) {
	int bolo,cap,bb,voie,rep,n; TypeRepartiteur *repart;
	char log; char explics[256];
	
	log = 1;
	printf("(%s) Affichage %s %sacquisition\n",__func__,ReglagesEtatDemande?"demande":"arrete",(Acquis[AcquisLocale].etat.active)?"pendant l'":"hors ");
	if(ReglagesEtatDemande) {
		for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].local) {
			for(cap=0; cap<Bolo[bolo].nbcapt; cap++) if((bb = Bolo[bolo].captr[cap].bb.num) >= 0) Numeriseur[bb].status.a_copier = 1;
		}
		if(!Acquis[AcquisLocale].etat.active) {
			if(log) printf("* Liste des repartiteurs a lire:\n");
			for(rep=0; rep<RepartNb; rep++) Repart[rep].utile = 0;
			n = 0;
			for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].local) {
				for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
					voie = Bolo[bolo].captr[cap].voie;
					if((bb = Bolo[bolo].captr[cap].bb.num) < 0) {
						if(log) printf("  ! Pas de numeriseur pour transmettre la voie %s\n",VoieManip[voie].nom);
						n++; continue;
					}
					if(!(repart = Numeriseur[bb].repart)) {
						if(log) printf("  ! Pas de repartiteur pour transmettre le numeriseur %s\n",Numeriseur[bb].nom);
						n++; continue;
					}
					if(!(repart->utile)) {
						if(!RepartiteurAdopte(repart,1,explics)) {
							n++; if(log) printf("  ! %s: %s\n",repart->nom,explics);
						} else if(log) printf("  + %s\n",repart->nom);
					}
				}
			}
			if(n && log) printf("  !!! %d repartiteur%s en defaut\n",Accord1s(n));
			RepartiteursSelectionneVoiesLues(1,1); printf("\n"); RepartImprimeEchantillon(0);
			LectModeStatus = 1; ReglagesEtatPeutStopper = 1;
			printf("(%s) Entree dans la boucle d'acquisition elementaire\n",__func__);
			LectAcqElementaire(NUMER_MODE_IDEM);
			printf("(%s) Sortie de la boucle d'acquisition elementaire\n",__func__);
		}
	} else if(ReglagesEtatPeutStopper) {
		Acquis[AcquisLocale].etat.active = 0;
		for(bb=0; bb<NumeriseurNb; bb++) Numeriseur[bb].status.a_copier = 0;
	}
	ReglagesEtatPeutStopper = 0;
	printf("(%s) Sortie de la fonction\n",__func__);
	return(0);
}
#pragma mark ----------- Generalites ----------
/* ========================================================================== */
void LectMenuBarre() { mLectRuns = MenuIntitule(iLectRuns,"Runs"); }
/* ========================================================================== */
static int LectRazMaxPile() {
	RepartIp.depilees = 0;
	printf("(%s) RepartIp.depilees remis a %d\n",__func__,RepartIp.depilees);
	return(0);
}
Menu mLectRazMaxPile;
TypeMenuItem iLectRazMaxPile[] = { { "C", MNU_FONCTION LectRazMaxPile }, MNU_END };

/* ========================================================================== */
static Graph LectMonitSignalConstruit(int larg, int haut) {
	int cap,nb;
	TypeMonitFenetre *f; TypeMonitTrace *t;
	int k; OpiumColorState s; /* suivi des couleurs   */

	OscilloInit(&LectOscilloSignal,Bolo[0].captr[0].voie);
	LectOscilloSignal.ampl = LectSignalAmpl;
	LectOscilloSignal.zero = LectSignalZero;
	LectOscilloSignal.temps = LectSignalTemps;
	f = &LectFenSignal;
	strncpy(f->titre,"Signal",80);
	f->type = MONIT_SIGNAL;
	f->affiche = MONIT_SHOW;
	f->change = 0;
	f->catg = 0;
	f->posx = f->posy = OPIUM_AUTOPOS;
	f->larg = larg; f->haut = haut;
	f->axeY.i.max = 0; f->axeX.pts = 100;
	f->p.h.histo = 0;
	// MonitFenVoiesDefinies(0,0,(void *)0);
	nb = 0;
	OpiumColorInit(&s); k = OpiumColorNext(&s); // k = OpiumColorNext(&s);
	for(cap=0; cap<Bolo[0].nbcapt; cap++) {
		if(nb >= MAXTRACES) break;
		t = &(f->trace[nb++]);
		t->bolo = 0; t->cap = cap; t->voie = Bolo[0].captr[cap].voie; /* t->vm = 0; */ t->var = TRMT_AU_VOL;
		k = OpiumColorNext(&s); memcpy(&(t->couleur),&(OpiumColorVal[k]),sizeof(OpiumColor));
	}
	f->nb = nb;
//	MonitFenAffiche(f,0); g == 0 car VoieTampon pas encore cree
	f->g = GraphCreate(f->larg,f->haut,2 * f->nb);
	LectOscilloSignal.ecran = f->g;
	
	return(f->g);
}
/* ========================================================================== */
static Graph LectMonitEvtConstruit(int larg, int haut) {
	int cap,nb;
	TypeMonitFenetre *f; TypeMonitTrace *t;
	int k; OpiumColorState s; /* suivi des couleurs   */
	
	f = &LectFenEvt;
	strncpy(f->titre,"Evenement",80);
	f->type = MONIT_EVENT;
	f->affiche = MONIT_SHOW;
	f->change = 0;
	f->catg = 0;
	f->posx = f->posy = OPIUM_AUTOPOS;
	f->larg = larg; f->haut = haut;
//	f->axeY.i.min = -5 * LectOscilloSignal.ampl; f->axeY.i.max = 5 * LectOscilloSignal.ampl;
	f->axeY.i.min = f->axeY.i.max = 0; // autorange
	f->axeX.r.max = 10.0; f->axeY.i.max = 0; f->axeX.pts = 100;
	f->p.h.histo = 0;
	// MonitFenVoiesDefinies(0,0,(void *)0);
	OpiumColorInit(&s); k = OpiumColorNext(&s); // k = OpiumColorNext(&s);
	nb = 0;
	for(cap=0; cap<Bolo[0].nbcapt; cap++) {
		if(nb >= MAXTRACES) break;
		t = &(f->trace[nb++]);
		t->bolo = 0; t->cap = cap; t->voie = Bolo[0].captr[cap].voie; /* t->vm = 0; */ t->var = TRMT_AU_VOL;
		k = OpiumColorNext(&s); memcpy(&(t->couleur),&(OpiumColorVal[k]),sizeof(OpiumColor));
	}
	for(cap=0; cap<Bolo[0].nbcapt; cap++) {
		if(nb >= MAXTRACES) break;
		t = &(f->trace[nb++]);
		t->bolo = 0; t->cap = cap; t->voie = Bolo[0].captr[cap].voie; /* t->vm = 0; */ t->var = TRMT_PRETRG;
		k = OpiumColorNext(&s); memcpy(&(t->couleur),&(OpiumColorVal[k]),sizeof(OpiumColor));
	}
	f->nb = nb;
	//	MonitFenAffiche(g,0); g == 0 car VoieTampon pas encore cree
	f->g = GraphCreate(f->larg,f->haut,2 * f->nb);
	
	return(f->g);
}
/* ========================================================================== */
static Graph LectMonitHistoConstruit(MONIT_VARIABLE var, TypeMonitFenetre *f, char *titre, int larg, int haut, char nouveau) {
	int cap,voie,j;
	HistoDeVoie vh,prec; Graph g; TypeMonitTrace *t;
	int k; OpiumColorState s; /* suivi des couleurs   */

	if(nouveau) {
		strncpy(f->titre,titre,80);
		f->larg = larg; f->haut = haut;
		f->type = MONIT_HISTO;
		f->posx = f->posy = OPIUM_AUTOPOS;
		f->affiche = MONIT_SHOW;
		f->change = 0;
		f->catg = 0;
		//	f->axeY.i.min = -5 * LectOscilloSignal.ampl; f->axeY.i.max = 5 * LectOscilloSignal.ampl;
		f->axeX.r.max = 0.0; f->axeY.i.max = 0;
		f->p.h.histo = 0;
	} else MonitFenDetach(f); // extrapoler le nouvel histo avec les valeurs de l'ancien

	// #ifdef FAIT_AVEC_TAMPONS
	f->p.h.histo = HistoCreateFloat(f->axeX.r.min,f->axeX.r.max,f->axeX.pts);
	// MonitFenVoiesDefinies(0,0,(void *)0);
	OpiumColorInit(&s); k = OpiumColorNext(&s); // k = OpiumColorNext(&s);
	j = 0;
	for(cap=0; cap<Bolo[0].nbcapt; cap++) {
		if(j >= MAXTRACES) break;
		t = &(f->trace[j]);
		voie = Bolo[0].captr[cap].voie; t->numx = -1;
		t->bolo = 0; t->cap = cap; t->voie = voie; /* t->vm = 0; */ t->var = var;
		k = OpiumColorNext(&s);
		memcpy(&(t->couleur),&(OpiumColorVal[k]),sizeof(OpiumColor));
		f->p.h.hd[j] = HistoAdd(f->p.h.histo,HST_INT);
		HistoDataName(f->p.h.hd[j],VoieManip[voie].nom);
		HistoDataSupportRGB(f->p.h.hd[j],WND_Q_ECRAN,OpiumColorRGB(k));
		vh = VoieHisto[voie]; prec = 0;
		while(vh) { prec = vh; vh = prec->suivant; }
		vh = (HistoDeVoie)malloc(sizeof(TypeHistoDeVoie));
		if(!vh) break;
		if(prec == 0) VoieHisto[voie] = vh; else prec->suivant = vh;
		vh->hd = f->p.h.hd[j];
		vh->D = 1;
		vh->Xvar = t->var;
		vh->suivant = 0;
		j++;
	}
// #endif
	f->nb = j;
	//	MonitFenAffiche(g,0); g == 0 car VoieTampon pas encore cree
	if(nouveau) f->g = GraphCreate(f->larg,f->haut,2 * f->nb);
	else GraphErase(f->g);
	g = f->g;
	if(g) {
		HistoGraphAdd(f->p.h.histo,g);
		if(nouveau) OpiumTitle(g->cdr,titre);
		GraphAxisDefine(g,0); /* defini deja l'axe des X pour GraphGetFloatRange */
		if(f->axeX.r.min == f->axeX.r.max) GraphAxisAutoRange(g,GRF_XAXIS);
		else GraphAxisFloatRange(g,GRF_XAXIS,f->axeX.r.min,f->axeX.r.max);
		//- if(MonitChgtUnite) GraphAxisChgtUnite(g,GRF_XAXIS,MonitUnitesADU);
		if(f->axeY.i.max) GraphAxisIntRange(g,GRF_YAXIS,0,f->axeY.i.max);
		else GraphAxisAutoRange(g,GRF_YAXIS);
		//- if(g->dim > 1) GraphDataTrace(g,1,GRF_PNT,0);
		GraphUse(g,f->axeX.pts);
		GraphParmsSave(g);
	}

	return(g);
}
/* ========================================================================== */
static int LectMonitAmplChangeMin(Panel p, int i, void *arg) {
	LectFenSpectre.axeX.r.min = LectMonitAmplMin;
	LectMonitHistoConstruit(MONIT_BRUTE,&LectFenSpectre,0,0,0,0);
	return(0);
}
/* ========================================================================== */
static int LectMonitAmplChangeMax(Panel p, int i, void *arg) {
	LectFenSpectre.axeX.r.max = LectMonitAmplMax;
	LectMonitHistoConstruit(MONIT_BRUTE,&LectFenSpectre,0,0,0,0);
	return(0);
}
/* ========================================================================== */
static int LectMonitRmsChangeMin(Panel p, int i, void *arg) {
	LectFenBruit.axeX.r.min = LectMonitRmsMin;
	LectMonitHistoConstruit(MONIT_BRUIT,&LectFenBruit,0,0,0,0);
	return(0);
}
/* ========================================================================== */
static int LectMonitRmsChangeMax(Panel p, int i, void *arg) {
	LectFenBruit.axeX.r.max = LectMonitRmsMax;
	LectMonitHistoConstruit(MONIT_BRUIT,&LectFenBruit,0,0,0,0);
	return(0);
}
/* ========================================================================== */
static int LectChangeSauvegarde() {
	char a_relancer;
	
	a_relancer = Acquis[AcquisLocale].etat.active;
	if(Acquis[AcquisLocale].etat.active) LectStop();
	switch(LectModeSauvegarde) {
	  case LECT_SVG_NEANT:
		Archive.demandee = 0;
		break;
	  case LECT_SVG_STREAM:
		Trigger.demande = 0;
		Archive.demandee = 1;
		break;
	  case LECT_SVG_EVENTS:
		Trigger.demande = 1;
		Archive.demandee = 1;
		break;
	}
	// if(a_relancer) LectAcqStd(); on ne sort de la qu'avec Acquis[AcquisLocale].etat.active = 0
	
	return(0);
}
#ifdef JAMAIS_UTILISE
/* ========================================================================== */
static void LectMonitSignalChangeTemps() {
	float largeur;
	
	largeur = LectOscilloSignal.temps * 10.0;
	LectOscilloSignal.dim = (int)(largeur / LectOscilloSignal.horloge);
//	if(LectOscilloSignal.traces > 1) {
//		if(LectOscilloSignal.moyenne) free(LectOscilloSignal.moyenne);
//		LectOscilloSignal.moyenne = (float *)malloc(LectOscilloSignal.dim * sizeof(float));
//	}
	
	LectOscilloSignal.ab6[0][0] = 0.0;
	LectOscilloSignal.ab6[0][1] = LectOscilloSignal.horloge;
	
	return;
}
/* ========================================================================== */
void LectMonitSignalChangeMinMax() {
	int echy,min,max;
	
	echy = 5 * LectOscilloSignal.ampl;
	min = LectOscilloSignal.zero - echy; if(min < SambaDonneeMin) min = SambaDonneeMin;
	max = LectOscilloSignal.zero + echy; if(max > SambaDonneeMax) max = SambaDonneeMax;
	LectOscilloSignal.marqueY[0] = (TypeADU)min; LectOscilloSignal.marqueY[1] = (TypeADU)max;
	GraphAxisIntRange((LectOscilloSignal.f).g,GRF_YAXIS,min,max);
	GraphAxisIntGrad((LectOscilloSignal.f).g,GRF_YAXIS,(int)LectOscilloSignal.ampl);
	return;
}
/* ========================================================================== */
int LectMonitSignalRefresh(Instrum instrum) {	
	if(LectOscilloSignal.grille == 2) GraphMode((LectOscilloSignal.f).g,GRF_2AXES|GRF_GRID);
	else if(LectOscilloSignal.grille == 1) GraphMode((LectOscilloSignal.f).g,GRF_2AXES);
	else GraphMode((LectOscilloSignal.f).g,GRF_0AXES|GRF_NOGRAD);
	OpiumRefresh(((LectOscilloSignal.f).g)->cdr);
	return(0);
}
/* ========================================================================== */
static int LectMonitSignalChangeiAmpl(Instrum instrum, void *arg) {
	PanelRefreshVars(LectOscilloSignal.pAmpl);
	LectMonitSignalChangeMinMax();
	LectMonitSignalRefresh(instrum);
	return(0);
}
/* ========================================================================== */
static int LectMonitSignalChangepAmpl(Panel panel, int item, void *arg) {
	InstrumRefreshVar(LectOscilloSignal.iAmpl);
	LectMonitSignalChangeMinMax();
	LectMonitSignalRefresh(0);
	return(0);
}
/* ========================================================================== */
static int LectMonitSignalChangeiZero(Instrum instrum, int num) {
	PanelRefreshVars(LectOscilloSignal.pZero);
	LectMonitSignalChangeMinMax();
	LectMonitSignalRefresh(instrum);
	return(0);
}
/* ========================================================================== */
static int LectMonitSignalChangepZero(Panel panel, int item, void *arg) {
	InstrumRefreshVar(LectOscilloSignal.iZero);
	LectMonitSignalChangeMinMax();
	LectMonitSignalRefresh(0);
	return(0);
}
/* ========================================================================== */
static int LectMonitSignalChangeiTemps(Instrum instrum, int num) {
	PanelRefreshVars(LectOscilloSignal.pTemps);
	LectMonitSignalChangeTemps();
	LectMonitSignalRefresh(instrum);
	return(0);
}
/* ========================================================================== */
static int LectMonitSignalChangepTemps(Panel panel, int item, void *arg) {
	InstrumRefreshVar(LectOscilloSignal.iTemps);
	LectMonitSignalChangeTemps();
	LectMonitSignalRefresh(0);
	return(0);
}
#endif /* JAMAIS_UTILISE */
/* ========================================================================== */
int LectSauveCaracteristiques(Menu menu, MenuItem item) {
	RepertoireCreeRacine(FichierRunCaract);
	if(ArgPrint(FichierRunCaract,LectCaractDesc) > 0)
		printf("* Caracteristiques sauvees sur %s\n",FichierRunCaract);
	else OpiumFail("Ecriture de '%s' impossible",FichierRunCaract);
	return(0);
}
/* ========================================================================== */
int LectLanceSequence(Menu menu, MenuItem item) {
	LectAutoriseSequences = 1;
	SequencesImprime();
	LectDemarre();
	LectAutoriseSequences = 0;
	return(0);
}
/* ========================================================================== */
static char LectRetardChgeStart(void *adrs, char *texte, void *arg) {
	LectRetard.start.mode = *(char *)adrs;
	if(LectRetard.start.mode == LECT_RETARD_PREVU) {
		if(!strcmp(LectRetard.start.jour,L_("direct")) || !strcmp(LectRetard.start.jour,L_("vide"))) {
			strcpy(LectRetard.start.jour,DateCivile());
			strcpy(LectRetard.start.heure,DateHeure());
		}
		PanelItemSelect(pLectRetardeur,3,1); PanelItemSelect(pLectRetardeur,5,1);
		// snprintf(LectInfo[0],LECT_INFOLNGR,"Demarrage prevu a %s",LectRetard.start.heure);
	} else if(LectRetard.start.mode == LECT_RETARD_CALDR) {
		if(LectRunProgramme(0)) {
			LectRetard.stop.mode = LECT_RETARD_CALDR;
			LectRetardRefresh(); return(1);
		} else {
			strcpy(LectRetard.start.jour,L_("vide"));
			strcpy(LectRetard.start.heure,"\0");
		}
		PanelItemSelect(pLectRetardeur,3,0); PanelItemSelect(pLectRetardeur,5,0);
	} else {
		strcpy(LectRetard.start.jour,L_("direct"));
		strcpy(LectRetard.start.heure,"\0");
		PanelItemSelect(pLectRetardeur,3,0); PanelItemSelect(pLectRetardeur,5,0);
	}
	PanelItemUpdate(pLectRetardeur,1,0);
	PanelItemChangeTexte(pLectRetardeur,3,LectRetard.start.jour);  PanelItemUpdate(pLectRetardeur,3,0);
	PanelItemChangeTexte(pLectRetardeur,5,LectRetard.start.heure); PanelItemUpdate(pLectRetardeur,5,0);
	
	return(1);
}
/* ========================================================================== */
static char LectRetardChgeStop(void *adrs, char *texte, void *arg) {
	LectRetard.stop.mode = *(char *)adrs;
	if(LectRetard.stop.mode == LECT_RETARD_PREVU) {
		if(!strcmp(LectRetard.stop.jour,L_("seulement")) || !strcmp(LectRetard.stop.jour,L_("vide"))) {
			strcpy(LectRetard.stop.jour,DateCivile());
			strcpy(LectRetard.stop.heure,DateHeure());
		}
		PanelItemSelect(pLectRetardeur,4,1); PanelItemSelect(pLectRetardeur,6,1);
		// snprintf(LectInfo[1],LECT_INFOLNGR,"Arret prevu a %s",LectRetard.stop.heure);
	} else if(LectRetard.stop.mode == LECT_RETARD_CALDR) {
		if(LectRunProgramme(0)) {
			DateIntToJour(LectRetard.stop.jour,DateLongToJour(LectRetard.stop.date));
			DateIntToHeure(LectRetard.stop.heure,DateLongToHeure(LectRetard.stop.date));
		} else {
			strcpy(LectRetard.stop.jour,L_("vide"));
			strcpy(LectRetard.stop.heure,"\0");
		}
		PanelItemSelect(pLectRetardeur,4,0); PanelItemSelect(pLectRetardeur,6,0);
	} else {
		strcpy(LectRetard.stop.jour,L_("seulement"));
		strcpy(LectRetard.stop.heure,"\0");
		PanelItemSelect(pLectRetardeur,4,0); PanelItemSelect(pLectRetardeur,6,0);
	}
	PanelItemUpdate(pLectRetardeur,2,0);
	PanelItemChangeTexte(pLectRetardeur,4,LectRetard.stop.jour);  PanelItemUpdate(pLectRetardeur,4,0);
	PanelItemChangeTexte(pLectRetardeur,6,LectRetard.stop.heure); PanelItemUpdate(pLectRetardeur,6,0);
	
	return(1);
}
/* ========================================================================== */
static void LectRetardRefresh() {
	DateIntToJour(LectRetard.start.jour,DateLongToJour(LectRetard.start.date));
	DateIntToJour(LectRetard.stop.jour,DateLongToJour(LectRetard.stop.date));
	DateIntToHeure(LectRetard.start.heure,DateLongToHeure(LectRetard.start.date));
	DateIntToHeure(LectRetard.stop.heure,DateLongToHeure(LectRetard.stop.date));
	if(OpiumAlEcran(pLectRetardeur)) {
		PanelItemUpdate(pLectRetardeur,1,1);
		PanelItemUpdate(pLectRetardeur,2,1);
		PanelItemChangeTexte(pLectRetardeur,3,LectRetard.start.jour);  PanelItemUpdate(pLectRetardeur,3,0);
		PanelItemChangeTexte(pLectRetardeur,4,LectRetard.stop.jour);  PanelItemUpdate(pLectRetardeur,4,0);
		PanelItemChangeTexte(pLectRetardeur,5,LectRetard.start.heure); PanelItemUpdate(pLectRetardeur,5,0);
		PanelItemChangeTexte(pLectRetardeur,6,LectRetard.stop.heure); PanelItemUpdate(pLectRetardeur,6,0);
	}
}
#ifdef ANCIEN_STYLE
/* ========================================================================== */
static void LectPlancheGeneraleAncienne() {
	int x,y,x0,xm,dx,dy,xg,yg,ab6,ord,i,j,bas_zone_cmde,x1,x2; int cols,nbinfos,rep,nb;
	char fin_de_serie;
	Panel p; Graph g; Figure fig;

	/* panel special scripts avec prise de donnees incluse */
	p = pLectConditions = PanelCreate(7);
	if(BolosAdemarrer) {
		i = PanelKeyB(p,L_("Demarre det."),L_("sans/active"),&SettingsStartactive,12); PanelItemLngr(p,i,12);
		PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	}
	if(BolosAentretenir) {
		i = PanelKeyB(p,"Maintenance",L_("sans/active"),&SettingsDLUactive,12); PanelItemLngr(p,i,12);
		PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	}
	if(BolosUtilises > 1) {
		i = PanelList(p,L_("Detecteur"),ArchModeNoms,&ArchDetecMode,12);
		PanelItemColors(p,i,SambaOrangeVertJauneBleuViolet,6); PanelItemSouligne(p,i,0); // SambaBOETMVVRJ
		if(BolosAssocies) {
			i = PanelKeyB(p,L_("Associes"),L_("sans/avec"),&ArchAssocMode,12);
			PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
		}
	}
	i = PanelKeyB(p,"Regulation",L_("sans/active"),&TrmtRegulDemandee,12); PanelItemLngr(p,i,12);
	PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	i = PanelKeyB(p,L_("Archivage"),L_("sans/demande"),&Archive.demandee,12); PanelItemLngr(p,i,12);
	PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);

	x = Dx; y = Dy;

	/* rappel de la configuration des detecteurs */
	if(ConfigNb > 1) {
		p = pLectConfigRun = PanelCreate(1); PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX); // PNL_RDONLY|
		i = PanelList(p,"Configuration",ConfigListe,&ConfigNum,CONFIG_NOM);
		PanelItemColors(p,i,SambaVertJauneOrangeRouge,4); PanelItemSouligne(p,i,0);
		PanelItemReturn(p,i,SambaConfigChange,0);
		BoardAddPanel(bLecture,pLectConfigRun,x,y,0);
		y += 2 * Dy;
	}

	/* programmateur */
	if(LectRetardOption) {
		pLectRetardeur = PanelCreate(6); PanelColumns(pLectRetardeur,3); PanelMode(pLectRetardeur,PNL_BYLINES); 
		PanelKeyB(pLectRetardeur,L_("Demarrage"),L_("au clic/prevu le/agenda"),&LectRetard.start.mode,9);
		PanelText(pLectRetardeur,0,LectRetard.start.jour,10); 
		PanelText(pLectRetardeur,L_("a"),LectRetard.start.heure,10);
		PanelKeyB(pLectRetardeur,L_("Arret"),L_("au clic/prevu le/agenda"),&LectRetard.stop.mode,9);
		PanelText(pLectRetardeur,0,LectRetard.stop.jour,10);
		PanelText(pLectRetardeur,L_("a"),LectRetard.stop.heure,10);
		PanelItemModif(pLectRetardeur,1,LectRetardChgeStart,0);
		PanelItemModif(pLectRetardeur,2,LectRetardChgeStop,0);
		PanelItemSelect(pLectRetardeur,3,0); PanelItemSelect(pLectRetardeur,5,0);
		PanelItemSelect(pLectRetardeur,4,0); PanelItemSelect(pLectRetardeur,6,0);
		PanelSupport(pLectRetardeur,WND_CREUX);
		BoardAddPanel(bLecture,pLectRetardeur,x+(9*Dx),y,0);
		y += (4 * Dy);
	}
	
	/* marche/arret */
	BoardAddMenu(bLecture,mLectDemarrage = MenuLocalise(iLectDemarrage),29 * Dx,y,0);
	OpiumSupport(mLectDemarrage->cdr,WND_PLAQUETTE);
	BoardAddMenu(bLecture,mLectArret = MenuLocalise(iLectArret),(29 * Dx) + (15 * Dx),y,0);
	OpiumSupport(mLectArret->cdr,WND_PLAQUETTE);
	
	if(SettingsRegen) {
		BoardAddMenu(bLecture,mLectRegen = MenuLocalise(iLectRegen),29 * Dx,y + (2 * Dy),0);
		OpiumSupport(mLectRegen->cdr,WND_PLAQUETTE);
		// if(RegenEnCours) MenuItemAllume(mLectRegen,1," Stopper regeneration ",GRF_RGB_YELLOW);
		if(RegenEnCours) MenuItemAllume(mLectRegen,1,L_(" Regeneration en cours "),GRF_RGB_YELLOW);
		else MenuItemAllume(mLectRegen,1,L_(" Lancer  regeneration  "),GRF_RGB_GREEN);
		// PRINT_OPIUM_ADRS(pScriptWait);
		BoardAdd(bLecture,pScriptWait->cdr,28 * Dx,y + (4 * Dy),0);
	} else mLectRegen = 0;
	
	/* partie "commande et etat" */
	p = pLectMode = PanelCreate(7);
	i = PanelKeyB(p,"Mode",L_("stream/evenements"),&Trigger.demande,12);
	PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	if(BolosAdemarrer) {
		i = PanelKeyB(p,L_("Demarre det."),L_("sans/active"),&SettingsStartactive,12); PanelItemLngr(p,i,12);
		PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	}
	if(BolosAentretenir) {
		i = PanelKeyB(p,"Maintenance",L_("sans/active"),&SettingsDLUactive,12); PanelItemLngr(p,i,12);
		PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	}
	if(BolosUtilises > 1) {
		i = PanelList(p,L_("Detecteur"),ArchModeNoms,&ArchDetecMode,12);
		PanelItemColors(p,i,SambaOrangeVertJauneBleuViolet,5); PanelItemSouligne(p,i,0); // SambaBOETMVVRJ
		if(BolosAssocies) {
			i = PanelKeyB(p,L_("Associes"),L_("sans/avec"),&ArchAssocMode,12);
			PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
		}
	}
	i = PanelKeyB(p,"Regulation",L_("sans/active"),&TrmtRegulDemandee,12); PanelItemLngr(p,i,12);
	PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	i = PanelKeyB(p,L_("Archivage"),L_("sans/demande"),&Archive.demandee,12); PanelItemLngr(p,i,12);
	PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	//	pLectConditions = PanelDuplique(p);
	//	i = PanelKeyB(p,"Synchro","neant/active",&LectSynchroType,12);
	//		PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	//	i = PanelBool(p,"Sequences",&LectAutoriseSequences); PanelItemLngr(p,i,12);
	//		PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
	BoardAdd(bLecture,p->cdr,x,y,0);
	
	y += ((PanelItemNb(p) + 1) * Dy);
	BoardAddBouton(bLecture,L_("Sauver conditions"),LectSauveCaracteristiques,x+(9*Dx),y,0);
	if(SequenceNb > 1) {
		BoardAddBouton(bLecture,L_("Sequence speciale"),LectLanceSequence,x+(34*Dx),y,0);
	}
	y += (2 * Dy);
	
	p = pLectRegen = PanelCreate(LECT_INFONB);
	for(i=0; i<LECT_INFONB; i++) 
		PanelItemSelect(p,PanelItemLngr(p,PanelText(p,0,LectInfo[i],LECT_INFOLNGR),LECT_INFOLNGR),0);
	PanelSupport(p,WND_CREUX); PanelMode(p,PNL_DIRECT);  /*  || PNL_RDONLY inefficace? */
	BoardAdd(bLecture,p->cdr,x,y,0);
	y += ((LECT_INFONB + 1) * Dy);
	
	x = (2 * Dx);
	if(RepartNb > 1) {
		p = pDureeSynchro = PanelCreate(1);
		i = PanelItemSelect(p,PanelInt(p,"Synchro (us)",&LectSyncMesure),0);
		PanelItemIScale(p,i,0,80000); PanelItemColors(p,i,SambaVertJauneOrangeRouge,4);
		PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
		BoardAdd(bLecture,p->cdr,x,y,0);
	} else pDureeSynchro = 0;
	
	p = pReprises = PanelCreate(1);
	i = PanelFormat(p,PanelItemLngr(p,PanelInt(p,"Sessions",&LectSession),3),"%3d");
	PanelItemIScale(p,i,1,16); PanelItemColors(p,i,SambaVertJauneOrangeRouge,4);
	PanelMode(p,PNL_DIRECT|PNL_RDONLY); PanelSupport(p,WND_CREUX);
	if(RepartNb > 1) BoardAdd(bLecture,p->cdr,OPIUM_A_DROITE_DE 0);
	else BoardAdd(bLecture,p->cdr,x,y,0);
	x = Dx; y += (2 * Dy);
	
	if(SambaMaitre) {
		int sat,k;
		cols = 5;
		nbinfos = cols * (1 + Partit[PartitLocale].nbsat);
		p = pLectEcoule = PanelCreate(nbinfos);
		PanelColumns(p,cols); PanelMode(p,PNL_BYLINES);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,"Id",4),0),1);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,"run",9),0),1);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,L_("duree"),12),0),1);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,L_("evts vus"),10),0),1);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,L_("sauves"),10),0),1);
		k = 0;
		for(j=0; j<Partit[PartitLocale].nbsat; j++)  {
			sat = Partit[PartitLocale].sat[j];
			fin_de_serie = !((k++ + 1) % 3);
			PanelItemSouligne(p,PanelText (p,0,Acquis[sat].code,4),fin_de_serie);
			// PanelItemSouligne(p,PanelListB(p,0,AcquisTexte,&(Acquis[sat].etat.status),12),fin_de_serie);
			PanelItemSouligne(p,PanelText (p,0,Acquis[sat].etat.nom_run,9),fin_de_serie);
			PanelItemSouligne(p,PanelText (p,0,Acquis[sat].etat.duree_lue,12),fin_de_serie);
			i = PanelItemSouligne(p,PanelInt(p,0,&Acquis[sat].etat.evt_trouves),fin_de_serie); PanelItemLngr(p,i,10); PanelFormat(p,i,"%-10d");
			i = PanelItemSouligne(p,PanelInt(p,0,&Acquis[sat].etat.evt_ecrits),fin_de_serie); PanelItemLngr(p,i,10); PanelFormat(p,i,"%-10d");
			//- PanelItemSouligne(p,PanelItemSelect(p,PanelFloat(p,"Exposition",&Acquis[sat].etat.KgJ),0),1);
		}
		dy = Partit[PartitLocale].nbsat;
	} else {
		//- printf("(%s) Acquis[sat].etat. @%08X, AcquisLocale=%d\n",__func__,(hexa)Acquis[sat].etat.,AcquisLocale);
		cols = 5;
		nbinfos = cols * 2;
		p = pLectEcoule = PanelCreate(nbinfos);
		PanelColumns(p,cols); PanelMode(p,PNL_BYLINES);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,"Id",4),0),1);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,"run",9),0),1);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,L_("duree"),12),0),1);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,L_("evts vus"),10),0),1);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,L_("sauves"),10),0),1);
		PanelText (p,0,Acquis[AcquisLocale].code,4);
		PanelText (p,0,Acquis[AcquisLocale].etat.nom_run,9);
		PanelText (p,0,Acquis[AcquisLocale].etat.duree_lue,12);
		i = PanelInt(p,0,&Acquis[AcquisLocale].etat.evt_trouves); PanelItemLngr(p,i,10); PanelFormat(p,i,"%-10d");
		i = PanelInt(p,0,&Acquis[AcquisLocale].etat.evt_ecrits); PanelItemLngr(p,i,10); PanelFormat(p,i,"%-10d");
		//- PanelItemSelect(p,PanelFloat(p,"Exposition",&Acquis[AcquisLocale].etat.KgJ),0);
		dy = 1;
	}
	PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
	BoardAdd(bLecture,p->cdr,Dx,y,0);
	y += (dy + 3) * Dy;
	
    x0 = 57 * Dx; bas_zone_cmde = y - Dy;
	LectLigneHori3.dx = x0 - (3 * Dx / 2);
	fig = FigureCreate(FIG_DROITE,(void *)&LectLigneHori3,0,0);
	BoardAddFigure(bLecture,fig,0,bas_zone_cmde,0);
	
	mLectSupplements = MenuLocalise(iLectSupplements);
	// MenuColumns(mLectSupplements,2);
	BoardAddMenu(bLecture,mLectSupplements,2*Dx,y,0);
	OpiumSupport(mLectSupplements->cdr,WND_PLAQUETTE);

	nb = 0; for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) nb++;
	if(nb > 0) {
		p = pRepartXmit = PanelCreate(2 * nb); PanelColumns(p,2); PanelMode(p,PNL_BYLINES);
		for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) {
			PanelItemColors(p,PanelKeyB(p,Repart[rep].nom,L_("connecte/simule"),&(Repart[rep].simule),9),SambaVertRouge,2); // code_rep
			PanelItemColors(p,PanelKeyB(p,0,"arrete/en route",&(Repart[rep].en_route),9),SambaOrangeVert,2);
		}
		PanelMode(p,PNL_DIRECT|PNL_RDONLY); PanelSupport(p,WND_CREUX);
		BoardAdd(bLecture,p->cdr,OPIUM_A_DROITE_DE OPIUM_PRECEDENT);
	} else pRepartXmit = 0;

	p = pEtatTrigger = PanelCreate(1);
	i = PanelKeyB(p,"Trigger",L_("suspendu/actif"),&(Trigger.actif),10);
	i = PanelItemSelect(p,i,0); PanelItemColors(p,i,SambaOrangeVert,2);
	PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
	if(pRepartXmit) BoardAdd(bLecture,p->cdr,OPIUM_EN_DESSOUS_DE OPIUM_PRECEDENT);
	else BoardAdd(bLecture,p->cdr,OPIUM_A_DROITE_DE OPIUM_PRECEDENT);

	// if(SambaProcsNb) BoardAddBouton(bLecture,"Lancer script ..",SambaScriptLance,24*Dx,y,0);
	
	/* partie "evenements" */
	x = x0; y = Dy;
	xg = 60 * Dx; yg = bas_zone_cmde - (12 * Dy); if(yg < (8 * Dy)) yg = 8 * Dy;
	g = gEvtRunCtrl = GraphCreate(xg,yg,6);
	OpiumTitle(g->cdr,L_("Evenements"));
	ab6 = GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitEvtAb6[0][0]),PointsEvt);
	ord = GraphAdd(g,GRF_SHORT|GRF_YAXIS,0,PointsEvt); /* VoieTampon[0].brutes->t.sval mais .brutes a 0 */
	GraphDataRGB(g,ord,GRF_RGB_GREEN);
#ifdef MONIT_EVT_TRAITE
	GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitTrmtAb6[0][0]),0);
	ord = GraphAdd(g,GRF_SHORT|GRF_YAXIS,0,PointsEvt); /* VoieTampon[0].traitees->t.rval mais .traitees a 0 */
	GraphDataRGB(g,ord,GRF_RGB_MAGENTA);
#endif
#ifdef CARRE_A_VERIFIER
	ab6 = GraphAdd(g,GRF_FLOAT|GRF_XAXIS,MomentEvt,5);
	ord = GraphAdd(g,GRF_SHORT|GRF_YAXIS,AmplitudeEvt,5);
	GraphDataRGB(g,ord,GRF_RGB_RED);
#endif
	GraphUse(g,0); GraphDataUse(g,ab6,5); GraphDataUse(g,ord,5);
	GraphMode(g,GRF_0AXES /* GRF_2AXES | GRF_NOGRAD */);
	GraphParmsSave(g);
	OpiumSupport(g->cdr,WND_CREUX);
 	BoardAdd(bLecture,g->cdr,x,y,0);
	y += yg + Dy;
	
	p = pLectEvtNum = PanelCreate(3);
	PanelInt(p,L_("Evenement"),&LectCntl.MonitEvtNum);
	PanelItemSelect(p,PanelText (p,"t (ms)",TempsTrigger,16),0);
	PanelItemSelect(p,PanelFloat(p,"t0 (s)",&MonitT0),0);
	PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
	PanelItemReturn(p,1,LectEvtAffiche,(void *)1);
	BoardAdd(bLecture,p->cdr,x,y,0);
	x += (32 * Dx);
	
	p = pLectEvtQui = PanelCreate(4);
	PanelItemSelect(p,PanelText (p,L_("detecteur"),BoloTrigge,DETEC_NOM),0);
	PanelItemSelect(p,PanelText (p,L_("capteur"),VoieTriggee,MODL_NOM),0);
	PanelItemSelect(p,PanelFloat(p,L_("amplitude"),&MonitEvtAmpl),0);
	PanelItemSelect(p,PanelFloat(p,L_("montee (ms)"),&MonitEvtMontee),0);
	PanelMode(p,PNL_DIRECT);
	PanelSupport(p,WND_CREUX);
 	BoardAdd(bLecture,p->cdr,x,y,0);
	xm = x;
	
	x = x0 + (0 * Dx); y += (4 * Dy);
 	BoardAddInstrum(bLecture,InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&MonitAffEvts,L_("fige/dernier")),x,y - (Dy / 2),0); x += (12 * Dx);
	BoardAddMenu(bLecture,MenuBouton(L_("Precedent"), MNU_FONCTION MonitEvtPrecedent),x,y,0); x += (10 * Dx);
 	BoardAddMenu(bLecture,MenuBouton(L_("Suivant"),   MNU_FONCTION MonitEvtSuivant),x,y,0); x += (9 * Dx);
	
	x = x0; y += (2 * Dy);
	p = pTauxGlobal = PanelCreate(2);
	i = PanelFloat(p,L_("taux actuel (Hz)"),&TousLesEvts.freq); // PanelItemSelect(p,i,0);
	i = PanelFloat(p,L_("taux global (Hz)"),&LectCntl.TauxGlobal); // PanelItemSelect(p,i,0);
	PanelMode(p,PNL_DIRECT|PNL_RDONLY); PanelSupport(p,WND_CREUX);
	BoardAdd(bLecture,p->cdr,x,y+Dy,0);
	
	InstrumRectDim(&LectGlissTempo,(20 * Dx),2*Dx);
	InstrumTitle(cIntervAff = InstrumFloat(INS_GLISSIERE,(void *)&LectGlissTempo,&MonitIntervSecs,0.05,5.0),L_("Rafraichissement (s)"));
	InstrumGradSet(cIntervAff,1.0);
	InstrumOnChange(cIntervAff,LectUpdate_pIntervAff,0);
	BoardAddInstrum(bLecture,cIntervAff,x0 + (36 * Dx),y,0); // xm
    x = x0 + xg + (4 * Dx);
	
    x1 = x0 - (3 * Dx / 2); x2 = x - (5 * Dx / 2);
	LectLigneVert1.dy = LectLigneVert2.dy = y + (4 * Dy);
    if(LectLigneVert1.dy < bas_zone_cmde) LectLigneVert1.dy = bas_zone_cmde;
	fig = FigureCreate(FIG_DROITE,(void *)&LectLigneVert1,0,0);
	BoardAddFigure(bLecture,fig,x1,0,0);
	
	fig = FigureCreate(FIG_DROITE,(void *)&LectLigneVert2,0,0);
	BoardAddFigure(bLecture,fig,x2,0,0);
    
    LectLigneHori1.dx = x2 - x1;
    fig = FigureCreate(FIG_DROITE,(void *)&LectLigneHori1,0,0);
    BoardAddFigure(bLecture,fig,x1,LectLigneVert1.dy,0);
	
	/* partie "etat du systeme" */
	dx = Colonne.largeur + (2 * Colonne.graduation) + (Colonne.nbchars * Dx) + (2 * Dx);
    y = Dy;
	InstrumTitle(cLectTauxPile = InstrumInt(INS_COLONNE,(void *)&Colonne,&LectCntl.LectTauxPile,0,100),L_("Pile"));
	InstrumSupport(cLectTauxPile,WND_CREUX);
 	BoardAddInstrum(bLecture,cLectTauxPile,x,y,0);
	
	InstrumTitle(cCpuTauxOccupe = InstrumInt(INS_COLONNE,(void *)&Colonne,&CpuTauxOccupe,0,100),"CPU");
	InstrumSupport(cCpuTauxOccupe,WND_CREUX);
 	BoardAddInstrum(bLecture,cCpuTauxOccupe,x + dx,y,0);
	
	InstrumTitle(cLectPerteIP = InstrumFloat(INS_COLONNE,(void *)&ColonneMoitie,&PerteIP,0.01,100.0),"IP");
	InstrumGradLog(cLectPerteIP,1);
	InstrumSupport(cLectPerteIP,WND_CREUX);
 	BoardAddInstrum(bLecture,cLectPerteIP,x + (2 * dx) + (2 * Dx),y,0);
	
	InstrumTitle(cTempsMort = InstrumFloat(INS_COLONNE,(void *)&ColonneMoitie,&TempsMort,0.01,100.0),"trmt");
	InstrumGradLog(cTempsMort,1);
	InstrumSupport(cTempsMort,WND_CREUX); InstrumCouleur(cTempsMort,WND_GC_ROUGE);
 	BoardAddInstrum(bLecture,cTempsMort,x + (2 * dx) + (2 * Dx),y + ColonneMoitie.longueur + (2 * Dy),0);
	
	InstrumTitle(cPerteEvts = InstrumFloat(INS_COLONNE,(void *)&ColonneMoitie,&PerteEvts,0.01,100.0),"evts");
	InstrumGradLog(cPerteEvts,1);
	InstrumSupport(cPerteEvts,WND_CREUX); InstrumCouleur(cPerteEvts,WND_GC_ROUGE);
 	BoardAddInstrum(bLecture,cPerteEvts,x + (2 * dx) + (2 * Dx),y + 2 * (ColonneMoitie.longueur + (2 * Dy)),0);
	
	y += Colonne.longueur + (3 * Dy);
	p = pLectPileMax = PanelCreate(1);
	PanelItemLngr(p,PanelInt(p,"max",&RepartIp.depilees),4);
	PanelMode(p,PNL_DIRECT|PNL_RDONLY); PanelSupport(p,WND_CREUX);
	BoardAdd(bLecture,p->cdr,x,y,0);
	
 	BoardAddMenu(bLecture,MenuBouton("C", MNU_FONCTION LectRazMaxPile),x - Dx,y,0);
	
	//	LectLigneHori2.dx = (12 * Dx) + (3 * dx);
	//	fig = FigureCreate(FIG_DROITE,(void *)&LectLigneHori2,0,0);
	//	BoardAddFigure(bLecture,fig,x - (5 * Dx / 2),y + (2 * Dy),0);
	
	BoardAddPoussoir(bLecture,"Debug",&VerifRunDebug,x - Dx,y + (2 * Dy),0);	
	
	MenuItemAllume(mLectDemarrage,1,L_("Demarrer"),GRF_RGB_YELLOW);

	OpiumEnterFctn(bLecture,LectAutorise,0);
}
#else /* ANCIEN_STYLE */
// static TypeFigZone LectAreaCntl,LectAreaEtat,LectAreaEvts,LectAreaSyst;
/* ========================================================================== */
static void LectPlancheGenerale() {
	int x,y,x0,xm,ym,yn,xt,yt,dx,dy,xg,yg,ab6,ord,i,j,h,h1,h2,bas_zone_cmde;
//	int x1,x2;
	int cols,nbinfos,rep,nb;
	char fin_de_serie;
	Panel p; Graph g; // Figure fig;

	/* panel particulier a destination plutot des scripts, avec prise de donnees incluse */
	p = pLectConditions = PanelCreate(7);
	if(BolosAdemarrer) {
		i = PanelKeyB(p,L_("Demarre det."),L_("sans/active"),&SettingsStartactive,12); PanelItemLngr(p,i,12);
		PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	}
	if(BolosAentretenir) {
		i = PanelKeyB(p,"Maintenance",L_("sans/active"),&SettingsDLUactive,12); PanelItemLngr(p,i,12);
		PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	}
	if(BolosUtilises > 1) {
		i = PanelList(p,L_("Detecteur"),ArchModeNoms,&ArchDetecMode,12);
		PanelItemColors(p,i,SambaOrangeVertJauneBleuViolet,6); PanelItemSouligne(p,i,0); // SambaBOETMVVRJ
		if(BolosAssocies) {
			i = PanelKeyB(p,L_("Associes"),L_("sans/avec"),&ArchAssocMode,12);
			PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
		}
	}
	i = PanelKeyB(p,"Regulation",L_("sans/active"),&TrmtRegulDemandee,12); PanelItemLngr(p,i,12);
	PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	i = PanelKeyB(p,L_("Archivage"),L_("sans/demande"),&Archive.demandee,12); PanelItemLngr(p,i,12);
	PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);

	x = 2 * Dx; y = 2 * Dy;

	BoardAreaOpen(L_("Controle du run"),WND_PLAQUETTE);
	/* rappel de la configuration des detecteurs */
	if(ConfigNb > 1) {
		p = pLectConfigRun = PanelCreate(1); PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX); // PNL_RDONLY|
		i = PanelList(p,"Configuration",ConfigListe,&ConfigNum,CONFIG_NOM);
		PanelItemColors(p,i,SambaVertJauneOrangeRouge,4); PanelItemSouligne(p,i,0);
		PanelItemReturn(p,i,SambaConfigChange,0);
		BoardAddPanel(bLecture,pLectConfigRun,x,y,0);
		y += 2 * Dy;
	}

	/* programmateur */
	if(LectRetardOption) {
		pLectRetardeur = PanelCreate(6); PanelColumns(pLectRetardeur,3); PanelMode(pLectRetardeur,PNL_BYLINES); 
		PanelKeyB(pLectRetardeur,L_("Demarrage"),L_("au clic/prevu le/agenda"),&LectRetard.start.mode,9);
		PanelText(pLectRetardeur,0,LectRetard.start.jour,10); 
		PanelText(pLectRetardeur,L_("a"),LectRetard.start.heure,10);
		PanelKeyB(pLectRetardeur,L_("Arret"),L_("au clic/prevu le/agenda"),&LectRetard.stop.mode,9);
		PanelText(pLectRetardeur,0,LectRetard.stop.jour,10);
		PanelText(pLectRetardeur,L_("a"),LectRetard.stop.heure,10);
		PanelItemModif(pLectRetardeur,1,LectRetardChgeStart,0);
		PanelItemModif(pLectRetardeur,2,LectRetardChgeStop,0);
		PanelItemSelect(pLectRetardeur,3,0); PanelItemSelect(pLectRetardeur,5,0);
		PanelItemSelect(pLectRetardeur,4,0); PanelItemSelect(pLectRetardeur,6,0);
		PanelSupport(pLectRetardeur,WND_CREUX);
		BoardAddPanel(bLecture,pLectRetardeur,x+(9*Dx),y,0);
		y += (4 * Dy);
	}
	yn = y;
	
	/* marche/arret */
	BoardAddMenu(bLecture,mLectDemarrage = MenuLocalise(iLectDemarrage),x + (29 * Dx),y,0);
	OpiumSupport(mLectDemarrage->cdr,WND_PLAQUETTE);
	BoardAddMenu(bLecture,mLectArret = MenuLocalise(iLectArret),x + (29 * Dx) + (15 * Dx),y,0);
	OpiumSupport(mLectArret->cdr,WND_PLAQUETTE);
	y += (2 * Dy);

	if(SettingsRegen) {
		BoardAddMenu(bLecture,mLectRegen = MenuLocalise(iLectRegen),x + (29 * Dx),y,0);
		OpiumSupport(mLectRegen->cdr,WND_PLAQUETTE);
		y += (2 * Dy);
		// if(RegenEnCours) MenuItemAllume(mLectRegen,1," Stopper regeneration ",GRF_RGB_YELLOW);
		if(RegenEnCours) MenuItemAllume(mLectRegen,1,L_(" Regeneration en cours "),GRF_RGB_YELLOW);
		else MenuItemAllume(mLectRegen,1,L_(" Lancer  regeneration  "),GRF_RGB_GREEN);
		// PRINT_OPIUM_ADRS(pScriptWait);
		BoardAdd(bLecture,pScriptWait->cdr,30 * Dx,y,0);
		y += (2 * Dy);
	} else mLectRegen = 0;

	mLectSupplements = MenuLocalise(iLectSupplements);
	OpiumSupport(mLectSupplements->cdr,WND_PLAQUETTE);
	yg = y; h1 = (MenuItemNb(mLectSupplements) + 1) * Dy;

	y = yn;
	/* partie "commande et etat" */
	p = pLectMode = PanelCreate(7);
	i = PanelKeyB(p,"Mode",L_("stream/evenements"),&Trigger.demande,12);
	PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	if(BolosAdemarrer) {
		i = PanelKeyB(p,L_("Demarre det."),L_("sans/active"),&SettingsStartactive,12); PanelItemLngr(p,i,12);
		PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	}
	if(BolosAentretenir) {
		i = PanelKeyB(p,"Maintenance",L_("sans/active"),&SettingsDLUactive,12); PanelItemLngr(p,i,12);
		PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	}
	if(BolosUtilises > 1) {
		i = PanelList(p,L_("Detecteur"),ArchModeNoms,&ArchDetecMode,12);
		PanelItemColors(p,i,SambaOrangeVertJauneBleuViolet,5); PanelItemSouligne(p,i,0); // SambaBOETMVVRJ
		if(BolosAssocies) {
			i = PanelKeyB(p,L_("Associes"),L_("sans/avec"),&ArchAssocMode,12);
			PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
		}
	}
	i = PanelKeyB(p,"Regulation",L_("sans/active"),&TrmtRegulDemandee,12); PanelItemLngr(p,i,12);
	PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	i = PanelKeyB(p,L_("Archivage"),L_("sans/demande"),&Archive.demandee,12); PanelItemLngr(p,i,12);
	PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	//	pLectConditions = PanelDuplique(p);
	//	i = PanelKeyB(p,"Synchro","neant/active",&LectSynchroType,12);
	//		PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	//	i = PanelBool(p,"Sequences",&LectAutoriseSequences); PanelItemLngr(p,i,12);
	//		PanelItemColors(p,i,SambaJauneVertOrangeBleu,2); PanelItemSouligne(p,i,0);
	PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
	BoardAdd(bLecture,p->cdr,x,y,0);
	
	y += ((PanelItemNb(p) + 1) * Dy);
	BoardAddBouton(bLecture,L_("Sauver conditions"),LectSauveCaracteristiques,x+(9*Dx),y,0);
	if(SequenceNb > 1) {
		BoardAddBouton(bLecture,L_("Sequence speciale"),LectLanceSequence,x+(34*Dx),y,0);
	}
	y += (2 * Dy);

	h2 = (((MenuItemNb(mLectSupplements) - 1) / 2) + 2) * Dy;
	if((y + h2) < (y + h1)) { MenuColumns(mLectSupplements,2); xg = 2 * Dx; h = h2; }
	else { xg = x + (32 * Dx); h = h1; }
	BoardAddMenu(bLecture,mLectSupplements,xg,y,0);
	y += h;

	BoardAreaEnd(bLecture,&xm,&ym);
	xt = xm; yt = ym;

	x = (2 * Dx); y = yt + (2 * Dy);
	BoardAreaOpen(L_("Etat"),WND_PLAQUETTE);
	p = pLectRegen = PanelCreate(LECT_INFONB);
	for(i=0; i<LECT_INFONB; i++) 
		PanelItemSelect(p,PanelItemLngr(p,PanelText(p,0,LectInfo[i],LECT_INFOLNGR),LECT_INFOLNGR),0);
	PanelSupport(p,WND_CREUX); PanelMode(p,PNL_DIRECT);  /*  || PNL_RDONLY inefficace? */
	BoardAdd(bLecture,p->cdr,x,y,0);
	y += ((LECT_INFONB + 1) * Dy);
	
	if(RepartNb > 1) {
		p = pDureeSynchro = PanelCreate(1);
		i = PanelItemSelect(p,PanelInt(p,"Synchro (us)",&LectSyncMesure),0);
		PanelItemIScale(p,i,0,80000); PanelItemColors(p,i,SambaVertJauneOrangeRouge,4);
		PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
		BoardAdd(bLecture,p->cdr,x,y,0);
	} else pDureeSynchro = 0;
	
	p = pReprises = PanelCreate(1);
	i = PanelFormat(p,PanelItemLngr(p,PanelInt(p,"Sessions",&LectSession),3),"%3d");
	PanelItemIScale(p,i,1,16); PanelItemColors(p,i,SambaVertJauneOrangeRouge,4);
	PanelMode(p,PNL_DIRECT|PNL_RDONLY); PanelSupport(p,WND_CREUX);
	if(RepartNb > 1) BoardAdd(bLecture,p->cdr,OPIUM_A_DROITE_DE 0);
	else BoardAdd(bLecture,p->cdr,x,y,0);
	y += (2 * Dy);
	
	if(SambaMaitre) {
		int sat,k;
		cols = 5;
		nbinfos = cols * (1 + Partit[PartitLocale].nbsat);
		p = pLectEcoule = PanelCreate(nbinfos);
		PanelColumns(p,cols); PanelMode(p,PNL_BYLINES);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,"Id",4),0),1);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,"run",9),0),1);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,L_("duree"),12),0),1);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,L_("evts vus"),10),0),1);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,L_("sauves"),10),0),1);
		k = 0;
		for(j=0; j<Partit[PartitLocale].nbsat; j++)  {
			sat = Partit[PartitLocale].sat[j];
			fin_de_serie = !((k++ + 1) % 3);
			PanelItemSouligne(p,PanelText (p,0,Acquis[sat].code,4),fin_de_serie);
			// PanelItemSouligne(p,PanelListB(p,0,AcquisTexte,&(Acquis[sat].etat.status),12),fin_de_serie);
			PanelItemSouligne(p,PanelText (p,0,Acquis[sat].etat.nom_run,9),fin_de_serie);
			PanelItemSouligne(p,PanelText (p,0,Acquis[sat].etat.duree_lue,12),fin_de_serie);
			i = PanelItemSouligne(p,PanelInt(p,0,&Acquis[sat].etat.evt_trouves),fin_de_serie); PanelItemLngr(p,i,10); PanelFormat(p,i,"%-10d");
			i = PanelItemSouligne(p,PanelInt(p,0,&Acquis[sat].etat.evt_ecrits),fin_de_serie); PanelItemLngr(p,i,10); PanelFormat(p,i,"%-10d");
			//- PanelItemSouligne(p,PanelItemSelect(p,PanelFloat(p,"Exposition",&Acquis[sat].etat.KgJ),0),1);
		}
		dy = Partit[PartitLocale].nbsat;
	} else {
		//- printf("(%s) Acquis[sat].etat. @%08X, AcquisLocale=%d\n",__func__,(hexa)Acquis[sat].etat.,AcquisLocale);
		cols = 5;
		nbinfos = cols * 2;
		p = pLectEcoule = PanelCreate(nbinfos);
		PanelColumns(p,cols); PanelMode(p,PNL_BYLINES);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,"Id",4),0),1);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,"run",9),0),1);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,L_("duree"),12),0),1);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,L_("evts vus"),10),0),1);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,L_("sauves"),10),0),1);
		PanelText (p,0,Acquis[AcquisLocale].code,4);
		PanelText (p,0,Acquis[AcquisLocale].etat.nom_run,9);
		PanelText (p,0,Acquis[AcquisLocale].etat.duree_lue,12);
		i = PanelInt(p,0,&Acquis[AcquisLocale].etat.evt_trouves); PanelItemLngr(p,i,10); PanelFormat(p,i,"%-10d");
		i = PanelInt(p,0,&Acquis[AcquisLocale].etat.evt_ecrits); PanelItemLngr(p,i,10); PanelFormat(p,i,"%-10d");
		//- PanelItemSelect(p,PanelFloat(p,"Exposition",&Acquis[AcquisLocale].etat.KgJ),0);
		dy = 1;
	}
	PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
	BoardAdd(bLecture,p->cdr,x,y,0);
	y += (dy + 3) * Dy;

//	x0 = 57 * Dx; bas_zone_cmde = y - Dy;
//	LectLigneHori3.dx = x0 - (3 * Dx / 2);
//	fig = FigureCreate(FIG_DROITE,(void *)&LectLigneHori3,0,0);
//	BoardAddFigure(bLecture,fig,0,bas_zone_cmde,0);
	
	nb = 0; for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) nb++;
	if(nb > 0) {
		p = pRepartXmit = PanelCreate(2 * nb); PanelColumns(p,2); PanelMode(p,PNL_BYLINES);
		for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) {
			PanelItemColors(p,PanelKeyB(p,Repart[rep].nom,L_("connecte/simule"),&(Repart[rep].simule),9),SambaVertRouge,2); // code_rep
			PanelItemColors(p,PanelKeyB(p,0,"arrete/en route",&(Repart[rep].en_route),9),SambaOrangeVert,2);
		}
		PanelMode(p,PNL_DIRECT|PNL_RDONLY); PanelSupport(p,WND_CREUX);
		BoardAdd(bLecture,p->cdr,OPIUM_EN_DESSOUS_DE OPIUM_PRECEDENT);
	} else pRepartXmit = 0;

	p = pEtatTrigger = PanelCreate(1);
	i = PanelKeyB(p,"Trigger",L_("suspendu/actif"),&(Trigger.actif),10);
	i = PanelItemSelect(p,i,0); PanelItemColors(p,i,SambaOrangeVert,2);
	PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
	BoardAdd(bLecture,p->cdr,OPIUM_A_DROITE_DE OPIUM_PRECEDENT);
	BoardAreaEnd(bLecture,&xm,&ym);
	if(xm > xt) xt = xm; if(ym > yt) yt = ym;

	// if(SambaProcsNb) BoardAddBouton(bLecture,"Lancer script ..",SambaScriptLance,24*Dx,y,0);
	
	BoardAreaOpen(L_("Evenements"),WND_PLAQUETTE);
	/* partie "evenements" */
	x = xt + (3 * Dx); y = 2 * Dy;
	x0 = x; bas_zone_cmde = yt - Dy;
	xg = 60 * Dx; yg = bas_zone_cmde - (12 * Dy); if(yg < (8 * Dy)) yg = 8 * Dy;
	g = gEvtRunCtrl = GraphCreate(xg,yg,6);
	OpiumTitle(g->cdr,L_("Evenements"));
	ab6 = GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitEvtAb6[0][0]),PointsEvt);
	ord = GraphAdd(g,GRF_SHORT|GRF_YAXIS,0,PointsEvt); /* VoieTampon[0].brutes->t.sval mais .brutes a 0 */
	GraphDataRGB(g,ord,GRF_RGB_GREEN);
#ifdef MONIT_EVT_TRAITE
	GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitTrmtAb6[0][0]),0);
	ord = GraphAdd(g,GRF_SHORT|GRF_YAXIS,0,PointsEvt); /* VoieTampon[0].traitees->t.rval mais .traitees a 0 */
	GraphDataRGB(g,ord,GRF_RGB_MAGENTA);
#endif
#ifdef CARRE_A_VERIFIER
	ab6 = GraphAdd(g,GRF_FLOAT|GRF_XAXIS,MomentEvt,5);
	ord = GraphAdd(g,GRF_SHORT|GRF_YAXIS,AmplitudeEvt,5);
	GraphDataRGB(g,ord,GRF_RGB_RED);
#endif
	GraphUse(g,0); GraphDataUse(g,ab6,5); GraphDataUse(g,ord,5);
	GraphMode(g,GRF_0AXES);
	// GraphMode(g,GRF_2AXES|GRF_NOGRAD);
	GraphParmsSave(g);
	OpiumSupport(g->cdr,WND_CREUX);
 	BoardAdd(bLecture,g->cdr,x,y,0);
	y += yg + Dy;
	
	p = pLectEvtNum = PanelCreate(3);
	PanelInt(p,L_("Evenement"),&LectCntl.MonitEvtNum);
	PanelItemSelect(p,PanelText (p,"t (ms)",TempsTrigger,16),0);
	PanelItemSelect(p,PanelFloat(p,"t0 (s)",&MonitT0),0);
	PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
	PanelItemReturn(p,1,LectEvtAffiche,(void *)1);
	BoardAdd(bLecture,p->cdr,x,y,0);
	x += (32 * Dx);
	
	p = pLectEvtQui = PanelCreate(4);
	PanelItemSelect(p,PanelText (p,L_("detecteur"),BoloTrigge,DETEC_NOM),0);
	PanelItemSelect(p,PanelText (p,L_("capteur"),VoieTriggee,MODL_NOM),0);
	PanelItemSelect(p,PanelFloat(p,L_("amplitude"),&MonitEvtAmpl),0);
	PanelItemSelect(p,PanelFloat(p,L_("montee (ms)"),&MonitEvtMontee),0);
	PanelMode(p,PNL_DIRECT);
	PanelSupport(p,WND_CREUX);
 	BoardAdd(bLecture,p->cdr,x,y,0);
	xm = x;
	
	x = x0 + (0 * Dx); y += (4 * Dy);
 	BoardAddInstrum(bLecture,InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&MonitAffEvts,L_("fige/dernier")),x,y - (Dy / 2),0); x += (12 * Dx);
	BoardAddMenu(bLecture,MenuBouton(L_("Precedent"), MNU_FONCTION MonitEvtPrecedent),x,y,0); x += (10 * Dx);
 	BoardAddMenu(bLecture,MenuBouton(L_("Suivant"),   MNU_FONCTION MonitEvtSuivant),x,y,0); x += (9 * Dx);
	
	x = x0; y += (2 * Dy);
	p = pTauxGlobal = PanelCreate(2);
	i = PanelFloat(p,L_("taux actuel (Hz)"),&TousLesEvts.freq); // PanelItemSelect(p,i,0);
	i = PanelFloat(p,L_("taux global (Hz)"),&LectCntl.TauxGlobal); // PanelItemSelect(p,i,0);
	PanelMode(p,PNL_DIRECT|PNL_RDONLY); PanelSupport(p,WND_CREUX);
	BoardAdd(bLecture,p->cdr,x,y+Dy,0);
	
	InstrumRectDim(&LectGlissTempo,(20 * Dx),2*Dx);
	InstrumTitle(cIntervAff = InstrumFloat(INS_GLISSIERE,(void *)&LectGlissTempo,&MonitIntervSecs,0.05,5.0),L_("Rafraichissement (s)"));
	InstrumGradSet(cIntervAff,1.0);
	InstrumOnChange(cIntervAff,LectUpdate_pIntervAff,0);
	BoardAddInstrum(bLecture,cIntervAff,x0 + (39 * Dx),y,0); // xm
    x = x0 + xg + (4 * Dx);
	BoardAreaEnd(bLecture,&xm,&ym);
	if(xm > xt) xt = xm; if(ym > yt) yt = ym;

 /*   x1 = x0 - (3 * Dx / 2); x2 = x - (5 * Dx / 2);
	LectLigneVert1.dy = LectLigneVert2.dy = y + (4 * Dy);
    if(LectLigneVert1.dy < bas_zone_cmde) LectLigneVert1.dy = bas_zone_cmde;
	fig = FigureCreate(FIG_DROITE,(void *)&LectLigneVert1,0,0);
	BoardAddFigure(bLecture,fig,x1,0,0);
	
	fig = FigureCreate(FIG_DROITE,(void *)&LectLigneVert2,0,0);
	BoardAddFigure(bLecture,fig,x2,0,0);
    
    LectLigneHori1.dx = x2 - x1;
    fig = FigureCreate(FIG_DROITE,(void *)&LectLigneHori1,0,0);
    BoardAddFigure(bLecture,fig,x1,LectLigneVert1.dy,0); */
	
	BoardAreaOpen(L_("Systeme"),WND_PLAQUETTE);
	/* partie "etat du systeme" */
	dx = Colonne.largeur + (2 * Colonne.graduation) + (Colonne.nbchars * Dx) + (2 * Dx);
	x = xt + (3 * Dx); y = (2 * Dy);
	InstrumTitle(cLectTauxPile = InstrumInt(INS_COLONNE,(void *)&Colonne,&LectCntl.LectTauxPile,0,100),L_("Pile"));
	InstrumSupport(cLectTauxPile,WND_CREUX);
 	BoardAddInstrum(bLecture,cLectTauxPile,x,y,0);
	
	InstrumTitle(cCpuTauxOccupe = InstrumInt(INS_COLONNE,(void *)&Colonne,&CpuTauxOccupe,0,100),"CPU");
	InstrumSupport(cCpuTauxOccupe,WND_CREUX);
 	BoardAddInstrum(bLecture,cCpuTauxOccupe,x + dx,y,0);
	
	InstrumTitle(cLectPerteIP = InstrumFloat(INS_COLONNE,(void *)&ColonneMoitie,&PerteIP,0.01,100.0),"IP");
	InstrumGradLog(cLectPerteIP,1);
	InstrumSupport(cLectPerteIP,WND_CREUX);
 	BoardAddInstrum(bLecture,cLectPerteIP,x + (2 * dx) + (2 * Dx),y,0);
	
	InstrumTitle(cTempsMort = InstrumFloat(INS_COLONNE,(void *)&ColonneMoitie,&TempsMort,0.01,100.0),"trmt");
	InstrumGradLog(cTempsMort,1);
	InstrumSupport(cTempsMort,WND_CREUX); InstrumCouleur(cTempsMort,WND_GC_ROUGE);
 	BoardAddInstrum(bLecture,cTempsMort,x + (2 * dx) + (2 * Dx),y + ColonneMoitie.longueur + (2 * Dy),0);
	
	InstrumTitle(cPerteEvts = InstrumFloat(INS_COLONNE,(void *)&ColonneMoitie,&PerteEvts,0.01,100.0),"evts");
	InstrumGradLog(cPerteEvts,1);
	InstrumSupport(cPerteEvts,WND_CREUX); InstrumCouleur(cPerteEvts,WND_GC_ROUGE);
 	BoardAddInstrum(bLecture,cPerteEvts,x + (2 * dx) + (2 * Dx),y + 2 * (ColonneMoitie.longueur + (2 * Dy)),0);
	
	y += Colonne.longueur + (3 * Dy);
	p = pLectPileMax = PanelCreate(1);
	PanelItemLngr(p,PanelInt(p,"max",&RepartIp.depilees),4);
	PanelMode(p,PNL_DIRECT|PNL_RDONLY); PanelSupport(p,WND_CREUX);
	BoardAdd(bLecture,p->cdr,x,y,0);
	
 	BoardAddMenu(bLecture,MenuBouton("C", MNU_FONCTION LectRazMaxPile),x - Dx,y,0);
	
	//	LectLigneHori2.dx = (12 * Dx) + (3 * dx);
	//	fig = FigureCreate(FIG_DROITE,(void *)&LectLigneHori2,0,0);
	//	BoardAddFigure(bLecture,fig,x - (5 * Dx / 2),y + (2 * Dy),0);
	
	BoardAddPoussoir(bLecture,"Debug",&VerifRunDebug,x - Dx,y + (2 * Dy),0);	
	BoardAreaEnd(bLecture,&xm,&ym);
	if(xm > xt) xt = xm; if(ym > yt) yt = ym;

	MenuItemAllume(mLectDemarrage,1,L_("Demarrer"),GRF_RGB_YELLOW);

	OpiumEnterFctn(bLecture,LectAutorise,0);
}
#endif
/* ========================================================================== */
int LectRenovePlanche() {
	char affichee;
	affichee = OpiumDisplayed(bLecture);
	if(affichee) OpiumClear(bLecture);
	BoardTrash(bLecture);
	LectPlancheGenerale();
	if(affichee) OpiumFork(bLecture);
	return(1);
}
/* ========================================================================== */
static void LectPlancheSuiviAllume() {
	int j,nb;

	nb = MenuItemNb(mLectSuiviFocus);
	for(j=1; j<=nb; j++)
		if(LectCategRangFocus[j] == MonitCategFocus) MenuItemAllume(mLectSuiviFocus,j,MenuItemGetText(mLectSuiviFocus,j),GRF_RGB_GREEN);
		else MenuItemAllume(mLectSuiviFocus,j,MenuItemGetText(mLectSuiviFocus,j),GRF_RGB_YELLOW);
}
/* ========================================================================== */
static int LectPlancheSuiviFocusGet(Menu menu, MenuItem item) {
	int focus_demande; char affiche;

	/* MonitCategFocus est decale de 1 (difference entre MonitCatg et MonitCatgPtr: MonitCatgPtr[0] = &TousLesEvts;) */
	focus_demande = LectCategRangFocus[MenuItemNum(menu,item)]; // MenuItemNum retourne 1..n
	// printf("(%s) Focus demande: #%d\n",__func__,focus_demande);
	if(focus_demande != MonitCategFocus) {
		affiche = MonitEvtClasses;
		MonitEvtClasses = 0;
		MonitCategFocus = focus_demande;
		// printf("(%s) Focus courant: #%d\n",__func__,MonitCategFocus);
//		if(MonitCategFocus > 0) {
//			char texte[80]; sprintf(texte,"On se concentre maintenant sur les evenements %s",MonitCatgPtr[MonitCategFocus]->nom);
//			printf("(%s) MonitCategFocus=%d: %s\n",__func__,MonitCategFocus,texte);
			// AudioSpeak(texte);
//		} else if(MonitCategFocus == 0) printf("(%s) MonitCategFocus=%d: On affiche tous les evenements\n",__func__,MonitCategFocus);
//		else printf("(%s) MonitCategFocus=%d: On n'affiche aucun evenement\n",__func__,MonitCategFocus);
		LectPlancheSuivi(1);
		MonitEvtClasses = affiche;
	}
	return(0);
}
/* ========================================================================== */
static int LectPlancheSuiviTrace(int catg, int x, int y, char focus) {
	TypeMonitCatg *categ; Cadre bouton_figer;
	int larg,haut,ab6,ord; char legende[80]; Panel p; int xp,yp,ybas;

	if((catg < 0) || (catg >= CATG_MAX)) {
		printf("(%s) Numero de categorie insense: %d!!!\n",__func__,catg);
		char appelant[32]; NomAppelant(appelant,32); printf("(%s) appele par %s\n",__func__,appelant);
		return(y);
	}
	GraphQualiteDefaut(FondPlots? WND_Q_ECRAN: WND_Q_PAPIER);
	categ = MonitCatgPtr[catg]; if(categ == 0) return(0);
	if(focus) { larg = MonitEvtFocsLarg; haut = MonitEvtFocsHaut; }
	else { larg = MonitEvtCatgLarg; haut = MonitEvtCatgHaut; }
	// printf("(%s) MonitCatgPtr[%d] = categ = %08llX\n",__func__,catg,(IntAdrs)categ);
	if(categ->gEvt == 0) categ->gEvt = GraphCreate(larg,haut,4);
	else {
		GraphResize(categ->gEvt,larg,haut);
		GraphErase(categ->gEvt);
	}
	if(catg) sprintf(legende,L_("Evenements %s"),categ->nom);
	else strcpy(legende,L_("Tous les evenements"));
	OpiumTitle((categ->gEvt)->cdr,legende);
	ab6 = GraphAdd(categ->gEvt,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,Tzero,PointsEvt); // &(MonitEvtAb6[0][0]),PointsEvt);
	ord = GraphAdd(categ->gEvt,GRF_SHORT|GRF_YAXIS,0,PointsEvt); /* VoieTampon[0].brutes->t.sval mais .brutes a 0 */
	GraphAxisLog(categ->gEvt,GRF_YAXIS,MonitLogFocus);
	if(MonitLogFocus) {
		int ygrad;
		categ->autoscale = 0;
		GraphAxisIntGet(categ->gEvt,GRF_YAXIS,&(categ->min),&(categ->max),&ygrad);
	} else if(categ->autoscale) GraphAxisAutoRange(categ->gEvt,GRF_YAXIS);
	else GraphAxisIntRange(categ->gEvt,GRF_YAXIS,categ->min,categ->max);
	GraphUse(categ->gEvt,0);
	// GraphDataName(categ->gEvt,ab6,"t(ms)");
	GraphAxisTitle(categ->gEvt,GRF_XAXIS,"t(ms)");
	GraphDataTrace(categ->gEvt,ord,GRF_HST,0);
	GraphDataRGB(categ->gEvt,ord,GRF_RGB_GREEN);
	GraphMode(categ->gEvt,GRF_2AXES | (FondPlots? GRF_QUALITE: GRF_GRID)); //  | GRF_LEGEND
	//? GraphParmsSave(categ->gEvt);
	OpiumSupport((categ->gEvt)->cdr,WND_CREUX);
	BoardAddGraph(bSuiviEvts,categ->gEvt,x,y,0);
	LectPlancheSuiviMargeX = x + larg;

	ybas = y + haut + Dy;

	xp = x; yp = ybas;
	if(SettingsDeconvPossible) {
		BoardAddPoussoir(bSuiviEvts,L_("Deconvolution"),&(categ->deconv),xp,yp,0); xp = OPIUM_ADJACENT; yp = 0;
	}
	if(focus) {
		bouton_figer = BoardAddPoussoir(bSuiviEvts,L_("Figer"),&MonitFigeFocus,xp,yp,0); xp = OPIUM_ADJACENT; yp = 0;
	}
	BoardAddPoussoir(bSuiviEvts,"Autoscale",&(categ->autoscale),xp,yp,0);

	if(focus) xp = OPIUM_ALIGNE; else xp = x + larg - (11 * Dx);
	// xp = xp + (21 * Dx);
	p = categ->pMinMax = PanelCreate(2); PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
	PanelInt(p,"max",&(categ->max)); PanelItemLngr(p,1,6);
	PanelInt(p,"min",&(categ->min)); PanelItemLngr(p,2,6);
	// BoardAddPanel(bSuiviEvts,p,xp,yp+(2*Dy),0);
	BoardAddPanel(bSuiviEvts,p,xp,ybas + (2 * Dy),0);

	// if(focus) { xp = OPIUM_ADJACENT ;  yp = ybas; } else { xp = x; yp = ybas + (2 * Dy); }
	if(focus) { xp = x + larg - (22 * Dx);  yp = ybas; } else { xp = x; yp = ybas + (2 * Dy); }
	p = categ->pEvtval = PanelCreate(3); PanelMode(p,PNL_RDONLY|PNL_DIRECT); PanelSupport(p,WND_CREUX);
	PanelFloat(p,"Charge (ADU)",&(categ->charge)); PanelItemLngr(p,PanelItemFormat(p,1,"%-8.1f"),8);
	PanelFloat(p,"Maxi (ADU)",&(categ->top)); PanelItemLngr(p,PanelItemFormat(p,2,"%-8.1f"),8);
	PanelFloat(p,L_("Duree (ms)"),&(categ->duree)); PanelItemLngr(p,PanelItemFormat(p,3,"%-7.3f"),8);
	BoardAddPanel(bSuiviEvts,p,xp,yp,0);

	if(focus) BoardAddPoussoir(bSuiviEvts,"log(y)",&MonitLogFocus,OPIUM_EN_DESSOUS_DE bouton_figer);

	return(yp + (4 * Dy));
}
/* ========================================================================== */
static int LectPlancheSuiviSauve(Menu menu, MenuItem item) {
	MonitEcranEcrit(); return(0);
}
/* ========================================================================== */
static void LectPlancheSonsAllume() {
	int j,nb;

	nb = MenuItemNb(mLectSonorise);
	for(j=1; j<=nb; j++)
		if(LectCategRangSon[j] == MonitCategSonore) MenuItemAllume(mLectSonorise,j,MenuItemGetText(mLectSonorise,j),GRF_RGB_GREEN);
		else MenuItemAllume(mLectSonorise,j,MenuItemGetText(mLectSonorise,j),GRF_RGB_YELLOW);
}
/* ========================================================================== */
static int LectPlancheSuiviSonGet(Menu menu, MenuItem item) {
	int sono_demandee; char affiche;

	sono_demandee = LectCategRangSon[MenuItemNum(menu,item)]; // MenuItemNum retourne 1..n
	if(sono_demandee != MonitCategSonore) {
		affiche = MonitEvtClasses;
		MonitEvtClasses = 0;
		MonitCategSonore = sono_demandee;
//		printf("(%s) MonitCategSonore=%d: Son pour %s\n",__func__,MonitCategSonore,
//			(MonitCategSonore==-2)?"aucun":((MonitCategSonore==-1)?"tous":(MonitCategSonore<MonitCatgNb)?MonitCatg[MonitCategSonore].nom:"indefini"));
		LectPlancheSuivi(1);
		MonitEvtClasses = affiche;
	}
	return(0);
}
/* ========================================================================== */
static int LectPlancheSuiviRefresh(Menu menu, MenuItem item) {
	int i; TypeMonitFenetre *f; Graph g;

	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if((f->affiche == MONIT_FOLW) && (f->type == MONIT_2DHISTO) && (g = f->g)) {
//		printf("(%s) ******** Refresh de %s, C=%08llX F=%08llX\n",__func__,f->titre,(IntAdrs)(g->cdr),(IntAdrs)((g->cdr)->f));
//		if(g) OpiumDumpConnexions(__func__,g->cdr);
		OpiumBloque(g->cdr,0);
//		OpiumDebug(OPIUM_DEBUG_OPIUM,1);
		OpiumDisplay(g->cdr);
//		OpiumDebug(OPIUM_DEBUG_OPIUM,0);
		// LectPlancheSuivi(1);
		OpiumBloque(g->cdr,1);
//		printf("(%s) ******** Refresh termine\n",__func__);
	}
	return(0);
}
/* ========================================================================== */
void LectPlancheSuivi(char affiche) {
	int catg; int i,x,y,xa,ya,dx,xdroit,xgauche,xm,ym,ab6,ord; char montre[CATG_MAX],aucune;
	int focus_nb,son_nb;
	char taux,evol,trace,evt,son_plat;
	char titre[40],legende[80];
	Panel p; Graph g;
	TypeMonitCatg *categ; TypeMonitFenetre *f;

//	printf("(%s) Demande de reconstruction: %s\n",__func__,LectPlancheSuiviEnCours?"deja en cours, refusee!":"demarree");
	// if(!MonitEvtClasses || LectPlancheSuiviEnCours) return;
	if(LectPlancheSuiviEnCours) return;
	aucune = 1; taux = evol = trace = 0;
	for(catg=0; catg<MonitCatgNb+1; catg++) {
		categ = MonitCatgPtr[catg];
		evt = categ->trace && (MonitCategFocus < 0);
		montre[catg] = (categ->utilisee && (categ->taux || categ->evol || evt));
		if(montre[catg]) {
			aucune = 0;
			if(categ->taux) taux = 1; if(categ->evol) evol = 1; if(evt) trace = 1;
		}
	}
	if(aucune) { OpiumWarn(L_("Pas de classe d'evenement a afficher")); return; }

	LectPlancheSuiviEnCours = 1;
	if(OpiumDisplayed(bSuiviEvts)) {
//		WndFrame f = bSuiviEvts->f;
		OpiumClear(bSuiviEvts);
//		WndStackPrint(__func__,f);
	}
	BoardTrash(bSuiviEvts);
	for(catg=0; catg<MonitCatgNb+1; catg++) {
		MonitCatgPtr[catg]->pTaux = MonitCatgPtr[catg]->pMinMax = MonitCatgPtr[catg]->pEvtval = 0;
		MonitCatgPtr[catg]->gSuivi = MonitCatgPtr[catg]->gEvt = 0;
	}
	// BoardTrash a deja appele OpiumDelete(graph->cdr); et fait free(graph);
	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if(f->incluse) { f->g = 0; f->incluse = 0; }

	OpiumTitle(bSuiviEvts,L_("Suivi des evenements par classe"));

	x = 3 * Dx; ym = 0;
	for(catg=0; catg<MonitCatgNb+1; catg++) if(montre[catg]) {
//		SelectionInit(&(VoieManip[voie].def.catg[catg].definition),&(VarSelection.classes));
		categ = MonitCatgPtr[catg];
		y = 2 * Dy;
		BoardAreaOpen(categ->nom,WND_PLAQUETTE);
		if(taux) {
			if(categ->taux) {
				p = categ->pTaux = PanelCreate(1); PanelSupport(p,WND_CREUX); PanelMode(p,PNL_DIRECT|PNL_RDONLY);
				PanelFloat(p,L_("Taux (Hz)"),&(categ->freq)); PanelFormat(p,1,"%.2f"); PanelItemLngr(p,1,7);
				OpiumName(p->cdr,"Taux (Hz)");
				BoardAddPanel(bSuiviEvts,p,x,y,0);
			}
			y += (3 * Dy); if(ym < y) ym = y;
		}
		if(evol) {
			if(categ->evol && categ->activite.val) {
				if(categ->gSuivi == 0) categ->gSuivi = GraphCreate(MonitEvtCatgLarg,MonitEvtCatgHaut,1);
				else GraphErase(categ->gSuivi);
				if(catg) sprintf(legende,"Taux %s (Hz)",categ->nom);
				else strcpy(legende,"Taux total (Hz)");
				OpiumTitle((categ->gSuivi)->cdr,legende);
				/* 12 heures d'affichage des taux, soit 43200 points */
				ab6 = GraphAdd(categ->gSuivi,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,LectTauxAb6,LectTauxMax);
				ord = GraphAdd(categ->gSuivi,GRF_FLOAT|GRF_YAXIS,categ->activite.val,LectTauxMax);
				GraphDataName(categ->gSuivi,ab6,"t(mn)"); GraphAxisTitle(categ->gSuivi,GRF_XAXIS,"t(mn)");
				GraphDataRGB(categ->gSuivi,ord,GRF_RGB_ORANGE);
				GraphDataName(categ->gSuivi,ord,legende);
				GraphDataTrace(categ->gSuivi,ord,GRF_HST,0);
				GraphAxisFloatMin(categ->gSuivi,GRF_YAXIS,0.0); // GraphAxisAutoMax(categ->gSuivi,GRF_YAXIS);
				GraphMode(categ->gSuivi,GRF_2AXES | (FondPlots? GRF_QUALITE: GRF_GRID)); //  | GRF_LEGEND
				GraphUse(categ->gSuivi,0);
				OpiumSupport((categ->gSuivi)->cdr,WND_CREUX);
				BoardAddGraph(bSuiviEvts,categ->gSuivi,x,y,0); // OPIUM_EN_DESSOUS_DE OPIUM_PRECEDENT);
			}
			y += MonitEvtCatgHaut + Dy; if(ym < y) ym = y;
		}
		if(categ->trace && (MonitCategFocus < 0)) y = LectPlancheSuiviTrace(catg,x,y,0);
		if(ym < y) ym = y;
		BoardAreaEnd(bSuiviEvts,&xa,&ya); x = xa + (3 * Dx);
	}
	LectPlancheSuiviMargeY = ym;

	xdroit = xa;
	xgauche = (xdroit - MonitEvtFocsLarg) / 2;
	x = 3 * Dx; y = ym + (2 * Dy);
	BoardAddMenu(bSuiviEvts,mLectSuiviStart = MenuLocalise(iLectDemarrage),x,y,0);
	OpiumSupport(mLectSuiviStart->cdr,WND_PLAQUETTE);
	dx = MenuLargeurPix(mLectSuiviStart);

	BoardAreaOpen(L_("Focus sur..."),WND_RAINURES);
		mLectSuiviFocus = MenuCreate(0); focus_nb = 1;
		if(MenuItemArray(mLectSuiviFocus,MonitCatgNb+2)) {
			MenuItemAdd(mLectSuiviFocus,L_("rien"),MNU_FONCTION (void *)LectPlancheSuiviFocusGet);
			LectCategRangFocus[focus_nb++] = -1;
			for(catg=0; catg<MonitCatgNb+1; catg++) if(MonitCatgPtr[catg]->utilisee) {
				categ = MonitCatgPtr[catg];
				MenuItemAdd(mLectSuiviFocus,categ->nom,MNU_FONCTION (void *)LectPlancheSuiviFocusGet);
				LectCategRangFocus[focus_nb++] = catg;
			}
		}
		MenuColumns(mLectSuiviFocus,MenuItemNb(mLectSuiviFocus));
		OpiumSupport(mLectSuiviFocus->cdr,WND_PLAQUETTE);
		LectPlancheSuiviAllume();
		if(MonitCategFocus >= 0) {
			xm = (MonitEvtFocsLarg - MenuLargeurPix(mLectSuiviFocus)) / 2;
			if(xm < dx) xm = dx;
			x = (2 * Dx) + xm;
		} else x = dx + (5 * Dx);
		BoardAddMenu(bSuiviEvts,mLectSuiviFocus,x,y,0);
	BoardAreaEnd(bSuiviEvts,&xa,&ya); x = xa + (2 * Dx);

	mLectSuiviStop = MenuLocalise(iLectArret); OpiumSupport(mLectSuiviStop->cdr,WND_PLAQUETTE);
	if(MonitCategFocus >= 0) {
		xm = (2 * Dx) + MonitEvtFocsLarg - MenuLargeurPix(mLectSuiviStop);
		if(xm > x) x = xm;
	} else x = xa + (2 * Dx);
	BoardAddMenu(bSuiviEvts,mLectSuiviStop,x,y - Dy,0);

	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if((f->affiche == MONIT_FOLW) && (f->type == MONIT_2DHISTO)) {
		if(MonitCategFocus >= 0) x = xa + (2 * Dx);
		BoardAddBouton(bSuiviEvts,"MAJ 2D",LectPlancheSuiviRefresh,x,y + Dy,0);
		x += (8 * Dx);
		break;
	}
	BoardAddBouton(bSuiviEvts,"Sauver",LectPlancheSuiviSauve,x,y + Dy,0); // OPIUM_EN_DESSOUS_DE OPIUM_PRECEDENT);
	x += (8 * Dx);
	xm = x;

	son_plat = (MonitCategFocus < 0);
	if(!son_plat) {
		for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if(f->affiche == MONIT_FOLW) son_plat = 1;
	}
	if(son_plat) strcpy(titre,L_("Sonorisation de ...")); else strcpy(titre,"Son");
	BoardAreaOpen(titre,WND_RAINURES);
		mLectSonorise = MenuCreate(0); son_nb = 1;
		if(MenuItemArray(mLectSonorise,MonitCatgNb+2)) {
			MenuItemAdd(mLectSonorise,L_("rien"),MNU_FONCTION (void *)LectPlancheSuiviSonGet);
			LectCategRangSon[son_nb++] = -1;
			for(catg=0; catg<MonitCatgNb+1; catg++) if(MonitCatgPtr[catg]->utilisee) {
				categ = MonitCatgPtr[catg];
				MenuItemAdd(mLectSonorise,categ->nom,MNU_FONCTION (void *)LectPlancheSuiviSonGet);
				LectCategRangSon[son_nb++] = catg;
			}
		}
		if(son_plat) {
			MenuColumns(mLectSonorise,MenuItemNb(mLectSonorise));
//			x = xdroit - MenuLargeurPix(mLectSonorise) - Dx;
		} // else
		x = xm + (2 * Dx);
		OpiumSupport(mLectSonorise->cdr,WND_PLAQUETTE);
		LectPlancheSonsAllume();
		if(x < xm) x = xm;
		BoardAddMenu(bSuiviEvts,mLectSonorise,x,y,0);
	BoardAreaEnd(bSuiviEvts,&xa,&ya);

	if(Acquis[AcquisLocale].etat.active) {
		MenuItemAllume(mLectSuiviStart,1,L_("En cours"),GRF_RGB_GREEN);
		MenuItemAllume(mLectSuiviStop,1,0,GRF_RGB_YELLOW);
	} else {
		MenuItemAllume(mLectSuiviStart,1,L_("Demarrer"),GRF_RGB_YELLOW);
		MenuItemEteint(mLectSuiviStop,1,0);
	}
	if(MonitCategFocus >= 0) {
		x = 3 * Dx; y += 4 * Dy;
		// printf("(%s) Focus a afficher: #%d\n",__func__,MonitCategFocus);
		LectPlancheSuiviTrace(MonitCategFocus,x,y,1);
	} else LectPlancheSuiviMargeX = 0;
	LectPlancheSuiviMargeY = ya;

	x = LectPlancheSuiviMargeX; y = LectPlancheSuiviMargeY + Dy;
	g = 0; // pour debug
	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) {
		if(f->affiche == MONIT_FOLW) {
			MonitFenMemo(f); MonitFenClear(f); // fait free((f->g)->cdr), free(f->g) et f->g=0
			g = MonitFenCreeGraphe(f,1);
			if(g) {
				x += 2 * Dx;
				OpiumSupport(g->cdr,WND_CREUX);
				BoardAddGraph(bSuiviEvts,g,x,y,0); x += f->larg;
				f->incluse = 1;
			} else f->incluse = 0;
		} else f->incluse = 0;
	}

	LectPlancheSuiviEnCours = 0;
	if(affiche) {
		OpiumFork(bSuiviEvts);
//		printf("(%s) Planche '%s' affichee: F=%08llX\n",__func__,bSuiviEvts->nom,(IntAdrs)bSuiviEvts->f);
//		WndStackPrint(0,bSuiviEvts->f);
//		if(g) OpiumDumpConnexions(__func__,g->cdr);
//		OpiumDumpConnexions(__func__,bSuiviEvts);
	}
}
/* ========================================================================== */
static void LectOscilloSeuilLimites(char retrace) {
	int echy,min,max; float total;
	
	echy = 5 * LectOscilloSignal.ampl;
	min = LectOscilloSignal.zero - echy; if(min < SambaDonneeMin) min = SambaDonneeMin;
	max = LectOscilloSignal.zero + echy; if(max > SambaDonneeMax) max = SambaDonneeMax;
//	InstrumILimits(LectInstrumSeuil,min,max);
//	OpiumRefresh(LectInstrumSeuil->cdr);
	total = (float)(max - min);
	if(total> 0.0) LectSeuilRelatif = (LectSeuilReel - (float)min) * 100.0 / total;
	if(retrace) InstrumRefreshVar(LectInstrumSeuil);	
}
/* ========================================================================== */
static int LectChangeSeuilInstrum(Instrum instrum) {
	int echy,min,max; float total;
	
	echy = 5 * LectOscilloSignal.ampl;
	min = LectOscilloSignal.zero - echy; if(min < SambaDonneeMin) min = SambaDonneeMin;
	max = LectOscilloSignal.zero + echy; if(max > SambaDonneeMax) max = SambaDonneeMax;
	total = (float)(max - min);
	// printf("(%s) echelle = %d +/- %d => dynamique = %d .. %d\n",__func__,LectOscilloSignal.zero,echy,min,max);
	LectSeuilReel = (LectSeuilRelatif * total / 100.0) + (float)min;
	// printf("(%s) seuil = %g * %d / 100 = %g\n",__func__,LectSeuilRelatif,(max - min),LectSeuilReel);
	PanelRefreshVars(LectPanelSeuil);
	VoieManip[0].def.trgr.minampl = LectSeuilReel;
	return(0);
}
/* ========================================================================== */
static int LectChangeSeuilPanel(Panel panel, int item, void *arg) {
	VoieManip[0].def.trgr.minampl = LectSeuilReel;
	LectOscilloSeuilLimites(1);
	return(0);
}
/* ========================================================================== */
static int LectOscilloMinMaxInstrum() {
	OscilloChangeiAmpl(0,&LectOscilloSignal);
	LectOscilloSeuilLimites(1);
	return(0);
}
/* ========================================================================== */
static int LectOscilloMinMaxPanel(Panel panel, int item, void *arg) {
	OscilloChangepAmpl(0,0,&LectOscilloSignal);
	LectOscilloSeuilLimites(1);
	return(0);
}
/* ========================================================================== */
static int LectOscilloZeroInstrum() {
	OscilloChangeiZero(0,&LectOscilloSignal);
	LectOscilloSeuilLimites(1);
	return(0);
}
/* ========================================================================== */
static int LectOscilloZeroPanel(Panel panel, int item, void *arg) {
	OscilloChangepZero(0,0,&LectOscilloSignal);
	LectOscilloSeuilLimites(1);
	return(0);
}
/* ========================================================================== */
static void LectCompacteBruit() {
	int x,y,xg,yg; int i;

	x = Dx; y = 2 * Dy;
	CalcSpectreAutoPlanche(bLecture,&x,&y);
	xg = x + (3 * Dx); yg = 2 * Dy;
	printf("(%s) CalcSpectreFenNb = %d\n",__func__,CalcSpectreFenNb);
	CalcSpectreAutoEfface();
	if(!CalcSpectreAutoConstruit()) {
		LectCompacteNumero = LECT_COMPACTE_DONNEES;
		return;
	}
	// if(!CalcSpectreFenNb) CalcSpectreAutoVide();
	for(i=0; i<CalcSpectreFenNb; i++) {
		// OpiumGetPosition(gCalcSpectreAffiche[i]->cdr,&xg,&yg);
		OpiumSupport(gCalcSpectreAffiche[i]->cdr,WND_CREUX);
		BoardAdd(bLecture,gCalcSpectreAffiche[i]->cdr,xg,yg,0);
		xg += (CalcSpectreAutoDimX + 20);
	}
}
/* ========================================================================== */
static void LectCompacteParms() {
	int x,y; int bolo,cap;

	x = Dx;
	bolo = BoloNum; // 0
	for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
		y = 4 * Dy;
		DetecteurParmsConstruitVoie(bLecture,Bolo[bolo].captr[cap].voie,&x,&y,cap);
		x += (2 * Dx);
	}
	BoardAddMenu(bLecture,MenuBouton(L_("Sauver"), MNU_FONCTION DetecteurParmsSave),20*Dx,y-5*Dy,0);
	CalcSpectreFenNb = 0;
}
/* ========================================================================== */
static void LectCompacteDonnees() {
	int x,y,x0,y0,x1,xm,dx,xe,ye,xg,yg,i,k,marge,ab6,ord; int cols,nbinfos;
	Panel p; Graph g; Instrum instrum;
	TypeMonitFenetre *f;
	// Figure fig; TypeMonitTrace *t; OpiumColor *c;
	// HistoDeVoie vh,prec; int voie; int j;

	Trigger.demande = 1;
	LectSignalTemps = 1000.0 / Echantillonnage;
	LectSeuilReel = VoieManip[0].def.trgr.minampl;

	x = 2 * Dx; y = 3 * Dy;
	xe = 62 * Dx; ye = 20 * Dy; xg = 100 * Dx; yg = 20 * Dy;
	
	g = LectMonitEvtConstruit(xe,ye);
	x0 = x + (xe + (3 * Dx));
	y0 = y;
	
	BoardAreaOpen(L_("Evenements"),WND_RAINURES); /* donne des reglettes */
	//             ----------
	gEvtRunCtrl = g;
//	printf("(%s) gEvtRunCtrl @%08X: %s.\n",__func__,(hexa)gEvtRunCtrl,(OpiumDisplayed((gEvtRunCtrl->cdr)->planche))? "affiche": "absent");
	OpiumTitle(g->cdr,L_("Evenements"));
	ab6 = GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitEvtAb6[0][0]),PointsEvt);
	ord = GraphAdd(g,GRF_SHORT|GRF_YAXIS,0,PointsEvt); /* VoieTampon[0].brutes->t.sval mais .brutes a 0 */
	GraphDataRGB(g,ord,GRF_RGB_GREEN);
#ifdef MONIT_EVT_TRAITE
	GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitTrmtAb6[0][0]),0);
	ord = GraphAdd(g,GRF_FLOAT|GRF_YAXIS,0,PointsEvt); /* VoieTampon[0].traitees->t.rval mais .traitees a 0 */
	GraphDataRGB(g,ord,GRF_RGB_MAGENTA);
#endif
#ifdef CARRE_A_VERIFIER
	ab6 = GraphAdd(g,GRF_FLOAT|GRF_XAXIS,MomentEvt,5);
	ord = GraphAdd(g,GRF_SHORT|GRF_YAXIS,AmplitudeEvt,5);
	GraphDataRGB(g,ord,GRF_RGB_RED);
#endif
	GraphUse(g,0); GraphDataUse(g,ab6,5); GraphDataUse(g,ord,5);
	GraphMode(g,GRF_0AXES /* GRF_2AXES | GRF_NOGRAD */);
	GraphParmsSave(g);
	OpiumSupport(g->cdr,WND_CREUX);
 	BoardAdd(bLecture,g->cdr,x,y,0);
	y += ye + Dy;
	
	p = pLectEvtNum = PanelCreate(3);
	PanelInt(p,L_("Evenement"),&LectCntl.MonitEvtNum);
	PanelItemSelect(p,PanelText (p,"t (ms)",TempsTrigger,16),0);
	PanelItemSelect(p,PanelFloat(p,"t0 (s)",&MonitT0),0);
	PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
	PanelItemReturn(p,1,LectEvtAffiche,(void *)1);
	BoardAdd(bLecture,p->cdr,x + Dx,y,0);
	x = xe - (25 * Dx);
	x1 = x;
	
	p = pLectEvtQui = PanelCreate(3);
//	PanelItemSelect(p,PanelText (p,"detecteur",BoloTrigge,DETEC_NOM),0);
	PanelItemSelect(p,PanelText (p,L_("capteur"),VoieTriggee,MODL_NOM),0);
	PanelItemSelect(p,PanelFloat(p,L_("amplitude"),&MonitEvtAmpl),0);
	PanelItemSelect(p,PanelFloat(p,L_("montee (ms)"),&MonitEvtMontee),0);
	PanelMode(p,PNL_DIRECT);
	PanelSupport(p,WND_CREUX);
 	BoardAdd(bLecture,p->cdr,x,y,0);
	
	x = 2 * Dx; y += (4 * Dy);
	BoardAddInstrum(bLecture,InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&MonitAffEvts,L_("fige/dernier")),x,y - (Dy / 2),0);
	BoardAddMenu(bLecture,MenuBouton(L_("Precedent"), MNU_FONCTION MonitEvtPrecedent),x + (11 * Dx),y,0);
 	BoardAddMenu(bLecture,MenuBouton(L_("Suivant"),   MNU_FONCTION MonitEvtSuivant),x + (21 * Dx),y,0);

	x = x1 - (4 * Dx);
	p = pTauxGlobal = PanelCreate(3);
	i = PanelKeyB(p,"Trigger",L_("suspendu/actif"),&(Trigger.actif),10); PanelItemColors(p,i,SambaOrangeVert,2);
	i = PanelFloat(p,L_("taux actuel (Hz)"),&TousLesEvts.freq); 
	i = PanelFloat(p,L_("taux global (Hz)"),&LectCntl.TauxGlobal); 
	PanelMode(p,PNL_DIRECT|PNL_RDONLY); PanelSupport(p,WND_CREUX);
	BoardAdd(bLecture,p->cdr,x,y - Dy,0);
	BoardAreaEnd(bLecture,&x,&y);

	x = x0; y = y0; marge = 7 + Dy;
	LectGlissSeuil.longueur = yg - InstrumRectLarg(&LectGlissSeuil) - (2 * marge);
	LectInstrumSeuil = InstrumFloat(INS_GLISSIERE,(void *)&LectGlissSeuil,&LectSeuilRelatif,0.0,100.0); // (max + 1) / 5);
	InstrumTitle(LectInstrumSeuil,L_("Seuil"));
	PanelMode(LectPanelSeuil= PanelCreate(1),PNL_DIRECT); PanelSupport(LectPanelSeuil,WND_CREUX);
	k = PanelFloat(LectPanelSeuil,0,&LectSeuilReel); PanelItemLngr(LectPanelSeuil,k,9); // PanelFormat(LectPanelSeuil,k,"%9.2f");
	InstrumOnChange(LectInstrumSeuil,LectChangeSeuilInstrum,(void *)LectPanelSeuil);
	BoardAddInstrum(bLecture,LectInstrumSeuil,x,y+marge,0);
	y += (yg + (3 * Dy));
	PanelItemExit(LectPanelSeuil,k,LectChangeSeuilPanel,(void *)LectInstrumSeuil);
	BoardAddPanel(bLecture,LectPanelSeuil,x - Dx,y - Dy,0);
	x0 += (InstrumRectLarg(&LectGlissSeuil) + (2 * Dx));
		
	
//	BoardAreaOpen("Signal",WND_PLAQUETTE); // donne des rainures
	//             ------
	x = x0; y = y0;
	//	BoardAreaOpen(L_("Signal courant"),WND_RAINURES); /* donne des reglettes */
	g = LectMonitSignalConstruit(xg,yg);
	OpiumTitle(g->cdr,"Signal");
	OpiumSupport(g->cdr,WND_CREUX);
	BoardAdd(bLecture,g->cdr,x,y,0);
	x += (xg + (3 * Dx));
	
	instrum = InstrumInt(INS_GLISSIERE,(void *)&Gliss6lignes,&(LectOscilloSignal.ampl),2,5000); // (max + 1) / 5);
	LectOscilloSeuilLimites(0);
	InstrumGradLog(instrum,-1);
	InstrumTitle(instrum,"ADU/div");
	InstrumOnChange(instrum,LectOscilloMinMaxInstrum,(void *)(&LectOscilloSignal));
	LectOscilloSignal.iAmpl = instrum;
	BoardAddInstrum(bLecture,instrum,x,y,0);
	y += (InstrumRectLngr(&Gliss6lignes) + (3 * Dy));
	
	PanelMode(p = PanelCreate(1),PNL_DIRECT); PanelSupport(p,WND_CREUX);
	k = PanelInt(p,0,&(LectOscilloSignal.ampl)); PanelItemLngr(p,k,7); // PanelFormat(p,k,"%5d");
	PanelItemILimits(p,k,2,5000); // (int)((max + 1) / 5));
	//	PanelItemExit(p,k,SambaUpdateInstrumFromItem,instrum); insuffisant
	PanelItemExit(p,k,LectOscilloMinMaxPanel,(void *)(&LectOscilloSignal));
	LectOscilloSignal.pAmpl = p;
	BoardAddPanel(bLecture,p,x - (2 * Dx),y,0);
	y += (3 * Dy);
	
	instrum = InstrumInt(INS_GLISSIERE,(void *)&Gliss6lignes,&LectOscilloSignal.zero,-30000,30000);
	//	InstrumGradSet(instrum,(float)((int)(max / 4)));
	InstrumTitle(instrum,L_("Milieu ecran"));
	InstrumOnChange(instrum,LectOscilloZeroInstrum,(void *)(&LectOscilloSignal));
	LectOscilloSignal.iZero = instrum;
	BoardAddInstrum(bLecture,instrum,x,y,0);
	y += (InstrumRectLngr(&Gliss6lignes) + (3 * Dy));
	
	PanelMode(p = PanelCreate(1),PNL_DIRECT);
	k = PanelInt(p,0,&LectOscilloSignal.zero);
	// insuffisant: PanelItemExit(p,k,SambaUpdateInstrumFromItem,instrum);
	PanelItemExit(p,k,LectOscilloZeroPanel,(void *)(&LectOscilloSignal));
	PanelItemILimits(p,k,-30000,30000);
	PanelItemLngr(p,k,7);
	PanelSupport(p,WND_CREUX);
	LectOscilloSignal.pZero = p;
	BoardAddPanel(bLecture,p,x - (2 * Dx),y,0);
	y += (2 * Dy);

	//	BoardAddBouton(bLecture,"-",OscilloTrouveMilieu,OPIUM_A_DROITE_DE 0);
	BoardAddPoussoir(bLecture,"-",&(LectOscilloSignal.bZero),OPIUM_A_DROITE_DE 0);
	xm = x + (12 * Dx);

	InstrumRectDim(&LectGlissTemps,24*Dx,2*Dx);
	x = x0 + xg - InstrumRectLngr(&LectGlissTemps) - Dx;
	//2 x = xm - InstrumRectLngr(&LectGlissTemps) - (3 * Dx);
	instrum = InstrumFloat(INS_GLISSIERE,(void *)&LectGlissTemps,&LectOscilloSignal.temps,0.01,10.0);
	InstrumGradLog(instrum,1);
	InstrumTitle(instrum,"ms/div");
	InstrumOnChange(instrum,OscilloChangeiTemps,(void *)(&LectOscilloSignal));
	LectOscilloSignal.iTemps = instrum;
	BoardAddInstrum(bLecture,instrum,x,y,0);
	
	PanelMode(p = PanelCreate(1),PNL_DIRECT);
	k = PanelFloat(p,0,&LectOscilloSignal.temps);
	// insuffisant: PanelItemExit(p,k,SambaUpdateInstrumFromItem,instrum);
	PanelItemExit(p,k,OscilloChangepTemps,(void *)(&LectOscilloSignal));
	PanelItemRLimits(p,1,0.01,10.0);
	PanelItemLngr(p,k,5);
	PanelSupport(p,WND_CREUX);
	LectOscilloSignal.pTemps = p;
	//	BoardAddPanel(bLecture,p,x+((InstrumRectLngr(&LectGlissTemps) - (10*Dx))/2),y+(4 * Dy),0);
	BoardAddPanel(bLecture,p,x + Dx,y+(3 * Dy)-(Dy / 2),0);
	//1?	x += InstrumRectLngr(&LectGlissTemps) + (6 * Dx);

#ifdef VERSION_LUXE
	x = x0 + Dx;
	instrum = InstrumInt(INS_POTAR,(void *)&Vernier,&LectOscilloSignal.traces,1,MAXREPET);
	InstrumTitle(instrum,"# Traces");
	InstrumOnChange(instrum,OscilloChangeVoie,(void *)(&LectOscilloSignal));
	BoardAddInstrum(bLecture,instrum,x,y,0);
	x += ((Vernier.rayon + Vernier.graduation + Dx) * 2) + (2 * Dx);
	
	instrum = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss3lignes,&LectOscilloSignal.mode,"moyenne/traces/tout");
	InstrumTitle(instrum,"Type trace");
	InstrumOnChange(instrum,OscilloChangeVoie,(void *)(&LectOscilloSignal));
	OpiumSupport(instrum->cdr,WND_RAINURES);
	LectOscilloSignal.iNb = instrum;
	BoardAddInstrum(bLecture,instrum,x,y,0);
	x += Gliss3lignes.largeur + Gliss3lignes.graduation + ((Gliss3lignes.nbchars + 2) * Dx);
	
	instrum = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss3lignes,&LectOscilloSignal.grille,"masquee/axes/affichee");
	InstrumTitle(instrum,"Grille");
	InstrumOnChange(instrum,OscilloRefresh,(void *)(&LectOscilloSignal));
	OpiumSupport(instrum->cdr,WND_RAINURES);
	BoardAddInstrum(bLecture,instrum,x,y,0);
	x += Gliss3lignes.largeur + Gliss3lignes.graduation + ((Gliss3lignes.nbchars + 2) * Dx) + (3 * Dx);
#endif

//	BoardAreaEnd(bLecture,&x,&y);

	x = x0 + (8 * Dx); y += (4 * Dy);
	BoardAreaOpen(L_("Controle"),WND_RAINURES); /* donne des reglettes */
	//                --------
	instrum = InstrumKeyB(INS_POTAR,(void *)&Select180,&LectModeSauvegarde,LECT_SVG_CLES);
	InstrumTitle(instrum,L_("Sauvegarde"));
	InstrumOnChange(instrum,LectChangeSauvegarde,0);
	BoardAddInstrum(bLecture,instrum,x,y-Dy,0);
	x += (14 * Dx);
	
	BoardAddMenu(bLecture,mLectDemarrage = MenuLocalise(iLectDemarrage),x + (2 * Dx),y,0);
	OpiumSupport(mLectDemarrage->cdr,WND_PLAQUETTE);
	BoardAddMenu(bLecture,mLectArret = MenuLocalise(iLectArret),x + (25 * Dx),y,0);
	OpiumSupport(mLectArret->cdr,WND_PLAQUETTE);
	MenuItemAllume(mLectDemarrage,1,L_("Demarrer"),GRF_RGB_YELLOW);
	y += (2 * Dy);

	cols = 3;
	nbinfos = cols * 2;
	p = pLectEcoule = PanelCreate(nbinfos);
	PanelColumns(p,cols); PanelMode(p,PNL_BYLINES);
	PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,"run",9),0),1);
	PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,"duree",12),0),1);
	PanelItemSouligne(p,PanelItemSelect(p,PanelText (p,0,"evts",12),0),1);
	PanelText (p,0,Acquis[AcquisLocale].etat.nom_run,9);
	PanelText (p,0,Acquis[AcquisLocale].etat.duree_lue,12);
	i = PanelInt  (p,0,&Acquis[AcquisLocale].etat.evt_trouves); PanelFormat(p,i,"%d");
	//- PanelItemSelect(p,PanelFloat(p,"Exposition",&Acquis[AcquisLocale].etat.KgJ),0);
	PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
	BoardAdd(bLecture,p->cdr,x,y,0);

	BoardAreaMargesHori(Dx/2,0);
	BoardAreaEnd(bLecture,&x,&y);

	InstrumRectDim(&LectGlissTempo,(11 * Dx),2*Dx);
	InstrumTitle(cIntervAff = InstrumFloat(INS_GLISSIERE,(void *)&LectGlissTempo,&MonitIntervSecs,0.05,5.0),L_("Rafraichissement (s)"));
	InstrumGradSet(cIntervAff,1.0);
	InstrumOnChange(cIntervAff,LectUpdate_pIntervAff,0);
	BoardAddInstrum(bLecture,cIntervAff,xm - LectGlissTempo.longueur - (3 * Dx),y - (3 * Dy),0);
	// xm = x + LectGlissTempo.longueur;

	y += (2 * Dy);
	y0 = y;

	BoardAreaOpen(L_("Analyse"),WND_RAINURES); /* donne des reglettes */
	//                -------
	x = (7 * Dx);
	p = PanelCreate(2);
	PanelColumns(p,2); PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
	PanelFloat(p,"Amplitude min",&LectMonitAmplMin); PanelItemLngr(p,1,6);
	PanelItemExit(p,1,LectMonitAmplChangeMin,0);
	PanelFloat(p,"max",&LectMonitAmplMax); PanelItemLngr(p,2,6);
	PanelItemExit(p,2,LectMonitAmplChangeMax,0);
	BoardAdd(bLecture,p->cdr,x,y,0);
	x += (52 * Dx);
	
	BoardAddBouton(bLecture,L_("RAZ histos"),LectRazHistos,x,y,0);
	x += (23 * Dx);

	p = PanelCreate(2);
	PanelColumns(p,2); PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
	PanelFloat(p,"RMS min",&LectMonitRmsMin); PanelItemLngr(p,1,5);
	PanelItemExit(p,1,LectMonitRmsChangeMin,0);
	PanelFloat(p,"max",&LectMonitRmsMax); PanelItemLngr(p,2,5);
	PanelItemExit(p,2,LectMonitRmsChangeMax,0);
	BoardAdd(bLecture,p->cdr,x,y,0);
	y += (3 * Dy);
	
	x = (2 * Dx);
	f = &LectFenSpectre;
	f->axeX.r.min = 0; f->axeX.r.max = LectMonitAmplMax; f->axeX.pts = 100;
	g = LectMonitHistoConstruit(MONIT_BRUTE,f,L_("Spectre amplitude"),62 * Dx,20 * Dy,1);
	OpiumSupport(g->cdr,WND_CREUX);
 	BoardAdd(bLecture,g->cdr,x,y,0);
	x += (f->larg + (3 * Dx));

	f = &LectFenBruit; 
	f->axeX.r.min = 0; f->axeX.r.max = LectMonitRmsMax; f->axeX.pts = 100;
	g = LectMonitHistoConstruit(MONIT_BRUIT,f,L_("RMS ligne de base"),45 * Dx,20 * Dy,1);
	OpiumSupport(g->cdr,WND_CREUX);
 	BoardAdd(bLecture,g->cdr,x,y,0);
	x += (f->larg + (3 * Dx));

	BoardAreaMargesVert(Dy/2,0);
	BoardAreaEnd(bLecture,&x,&y);
	x += (f->larg + (5 * Dx));
					
	/* partie "etat du systeme" */
	BoardAreaOpen(L_("Systeme"),WND_RAINURES); /* donne des reglettes */
	//                -------
	dx = Colonne.largeur + (2 * Colonne.graduation) + (Colonne.nbchars * Dx) + (2 * Dx);
	x = xm - (3 * dx) - Dx; y = y0 + Dy;
	InstrumTitle(cLectTauxPile = InstrumInt(INS_COLONNE,(void *)&Colonne,&LectCntl.LectTauxPile,0,100),L_("Pile"));
	InstrumSupport(cLectTauxPile,WND_CREUX);
 	BoardAddInstrum(bLecture,cLectTauxPile,x,y,0);
	
	InstrumTitle(cCpuTauxOccupe = InstrumInt(INS_COLONNE,(void *)&Colonne,&CpuTauxOccupe,0,100),"CPU");
	InstrumSupport(cCpuTauxOccupe,WND_CREUX);
 	BoardAddInstrum(bLecture,cCpuTauxOccupe,x + dx,y,0);
	
	InstrumTitle(cLectPerteIP = InstrumFloat(INS_COLONNE,(void *)&ColonneMoitie,&PerteIP,0.01,100.0),"IP");
	InstrumGradLog(cLectPerteIP,1);
	InstrumSupport(cLectPerteIP,WND_CREUX);
 	BoardAddInstrum(bLecture,cLectPerteIP,x + (2 * dx) + (2 * Dx),y,0);
	
	InstrumTitle(cTempsMort = InstrumFloat(INS_COLONNE,(void *)&ColonneMoitie,&TempsMort,0.01,100.0),"trmt");
	InstrumGradLog(cTempsMort,1);
	InstrumSupport(cTempsMort,WND_CREUX); InstrumCouleur(cTempsMort,WND_GC_ROUGE);
 	BoardAddInstrum(bLecture,cTempsMort,x + (2 * dx) + (2 * Dx),y + ColonneMoitie.longueur + (2 * Dy),0);
	
	InstrumTitle(cPerteEvts = InstrumFloat(INS_COLONNE,(void *)&ColonneMoitie,&PerteEvts,0.01,100.0),"evts");
	InstrumGradLog(cPerteEvts,1);
	InstrumSupport(cPerteEvts,WND_CREUX); InstrumCouleur(cPerteEvts,WND_GC_ROUGE);
 	BoardAddInstrum(bLecture,cPerteEvts,x + (2 * dx) + (2 * Dx),y + 2 * (ColonneMoitie.longueur + (2 * Dy)),0);
	
	y += Colonne.longueur + (3 * Dy);
	p = pLectPileMax = PanelCreate(1);
	PanelItemLngr(p,PanelInt(p,"max",&RepartIp.depilees),4);
	PanelMode(p,PNL_DIRECT|PNL_RDONLY); PanelSupport(p,WND_CREUX);
	BoardAdd(bLecture,p->cdr,x,y,0);
	
 	BoardAddMenu(bLecture,MenuBouton("C", MNU_FONCTION LectRazMaxPile),x - Dx,y,0);
	BoardAreaMargesVert(Dy/2,0);
	BoardAreaEnd(bLecture,&x,&y);
	
	OpiumTitle(bLecture,L_("Prise de donnees"));
	
	CalcSpectreFenNb = 0;
}
/* ========================================================================== */
int RepartiteurContacte();
int BasicCommandes();
TypeMenuItem iLectCompacteCom[] = {
	{ "Connexion repartiteurs",     MNU_FONCTION RepartiteurContacte },
	{ "Organigramme",               MNU_FONCTION OrgaMagasinVisite },
//	{ "Fonctions basiques",         MNU_FONCTION BasicCommandes },
//	{ "Contenu Fifo",               MNU_FONCTION LectDumpFifo },
//	{ "Contenu tampons",            MNU_FONCTION LectBuffDump },
//	{ "Pointeurs tampons",          MNU_FONCTION LectBuffStatus },
	MNU_END
};
/* ========================================================================== */
static void LectCompacteCom() {
	int i,x,y;
	Menu m; Panel p;

	x = Dx; y = 2 * Dy;
	m = MenuLocalise(iLectCompacteCom); // MenuColumns(m,2);
	OpiumSupport(m->cdr,WND_PLAQUETTE);
	BoardAddMenu(bLecture,m,x,y,0);
	y = 3 * Dy;

	x = 40 * Dx; 
	BoardAreaOpen(L_("Acces bas niveau"),WND_RAINURES); /* donne des reglettes */
	BoardAddBouton(bLecture,L_("Pointeurs"),LectBuffStatus,x+(15*Dx),y,0);
	y += (2 * Dy);
	p = PanelCreate(7); PanelSupport(p,WND_CREUX);
	if(BoloNb > 1) {
		i = PanelList(p,L_("Detecteur"),BoloNom,&BoloNum,DETEC_NOM);
		PanelItemExit(p,i,DetecteurListeCapteurs,0);
	}
	if((BoloNb > 1) || (Bolo[BoloNum].nbcapt > 1)) {
		PanelList(p,L_("Capteur"),CapteurNom,&CapteurNum,MODL_NOM);
	}
	i = PanelKeyB(p,"Donnees","brutes/traitees",&LectBuffOrigType,9);
	PanelItemExit(p,i,LectBuffPrintLimites,0);
	PanelInt (p,"Point initial",&LectBuffVal0);
	PanelInt (p,"Point final",&LectBuffValN);
	PanelKeyB(p,"Destination","journal/fichier",&LectBuffDestType,8);
	PanelKeyB(p,"Format","decimal/hexa",&LectBuffHexa,8);
	PanelBoutonText(p,PNL_APPLY,"Imprimer");
	PanelOnApply(p,LectCompacteDumpBuff,0);
	BoardAddPanel(bLecture,p,x,y,0);
	y += (PanelItemNb(p) + 2) * Dy;
	p = PanelCreate(1); PanelSupport(p,WND_CREUX);
	PanelInt (p,"FIFO profondeur",&LectCompacteMaxFifo);
	PanelBoutonText(p,PNL_APPLY,"Imprimer");
	PanelOnApply(p,LectCompacteDumpFifo,0);
	BoardAddPanel(bLecture,p,x,y,0);
	x += (35 * Dx);
	
	y = 5 * Dy;
	if(Repart[0].interf == INTERF_PCI) {
		PCInum = Repart[0].adrs.val - 1;
		m = MenuLocalise(iBasicPCI);
	} else if((Repart[0].famille == FAMILLE_OPERA) 
			  || (Repart[0].famille == FAMILLE_IPE)) m = MenuLocalise(iBasicIp);
	else if(Repart[0].famille == FAMILLE_CALI) m = MenuLocalise(iBasicCali);
	else if(Repart[0].famille == FAMILLE_SAMBA) m = MenuLocalise(iBasicSmb);
	OpiumSupport(m->cdr,WND_PLAQUETTE);
	BoardAddMenu(bLecture,m,x,y,0);
	
	
	BoardAreaEnd(bLecture,&x,&y);
	
	CalcSpectreFenNb = 0;
}
/* ========================================================================== */
static void LectCompacteConstruit() {
	cas_ou(LectCompacteNumero) {
	  vaut LECT_COMPACTE_BRUIT:   LectCompacteBruit();   break;
	  vaut LECT_COMPACTE_PARMS:   LectCompacteParms();   break;
	  vaut LECT_COMPACTE_DONNEES: LectCompacteDonnees(); break;
	  vaut LECT_COMPACTE_COM:     LectCompacteCom();     break;
	}
}
/* ========================================================================== */
static int LectCompacteCliquePlanche(Cadre planche, WndAction *e, void *arg) {
	return(0);
}
/* ========================================================================== */
static void LectCompacteAffiche(int numero) {
	if(bLecture) {
		if(OpiumDisplayed(bLecture)) OpiumClear(bLecture);
		BoardTrash(bLecture);
	} else bLecture = BoardCreate();
	if(!bLecture) return;
	OpiumTitle(bLecture,"Samba");
	BoardOnClic(bLecture,LectCompacteCliquePlanche,0);

	mLectDemarrage = mLectArret = 0;
	pLectMode = pLectRegen = pDureeSynchro = pReprises = 0;
	pLectEcoule = pLectPileMax = pLectEvtNum = pLectEtat = 0;
	mLectRegen = mLectSupplements = 0;
	gEvtRunCtrl = gEvtSolo = 0;
	cLectTauxPile = cCpuTauxOccupe = cLectPerteIP = cTempsMort = cPerteEvts = 0;
	mCalcSpectreControle = 0;
	
	LectCompacteNumero = numero;
	LectCompacteOnglets = OngletDefini(LectCompacteOngletNoms,LectCompacteAffiche);
	OngletDemande(LectCompacteOnglets,LectCompacteNumero);
	BoardAddOnglet(bLecture,LectCompacteOnglets,0);
	LectCompacteConstruit();

	OpiumFork(bLecture);
}
/* ========================================================================== */
int LectCompacteRefait() { LectCompacteAffiche(LectCompacteNumero); return(0); }
/* ========================================================================== */
void LectInit() {
	int i; LECT_CONS_TYPE type_panel;	
	FILE *f; char ligne[256]; int k,lngr;
/*
 * Initialisation generale
 */
	LectListeErreursConnues();
	LectASauver = 0;
	LectureLog = 1;
	LectDepileWait = 1000; // 500;
	LectSigne = 1;
	// LectAffEvt = 1;
	LectDemarrageDemande = 0;
	LectModeStatus = 0; ReglagesEtatPeutStopper = NumeriseurEtatPeutStopper = NumeriseurPeutStopper = 0;
	LectArretProgramme = 0;
	LectBolosTrmtCateg = LectBolosTrgrCateg = 0;
	LectAcqLanceur = NEANT;
	LectureSuspendue = 0;
	LectPlancheSuiviEnCours = 0;
	LectInfo[0][0] = LectInfo[1][0] = '\0';
	LectStampSpecifique = 0;
	LectReglagesOuverts = 0;
	LectBBaffiches = 0;
	LectRepAffiches = 0;
	LectModeSpectresAuto = 0; LectModeSpectresBanc = 0;
	LectAutoriseSequences = 0;
	LecturePourRepart = 0;
	Lect1ModulALire = 0;
	LectAttenteRebootOpera = 55;
	LectDateRelance = 0;
	LectDateRegulBolo = 0;
	LectDernierStocke = LectHeureStocke = 0;
	RepartPci.date = RepartPci.copie = 0;
	RepartPci.delai = 1000000;
	RepartIp.depilees = 0;
	LectSyncMesure = 0;
//	LectNBolosParVoie = 0;
	LectBuffVal0 = 0; LectBuffValN = 0; LectBuffOrigType = LectBuffDestType = 0; LectBuffHexa = 0;
	LectCompacteMaxFifo = 1024;
	strcpy(LectBuffFichier,"BufferSAMBA");
	LectCntl.LectTauxPile = 0;
	TousLesEvts.freq = 0.0;
	LectTauxMin = 0.1;
	LectCntl.LectMode = LECT_DONNEES;
	LectCntl.LectRunNouveau = 1;
	LectMonitAmplMin = 0.0;
	LectMonitAmplMax = 20000.0;
	LectMonitRmsMin = 0.0;
	LectMonitRmsMax = 10.0;
	LectFenSignal.affiche = 0;
	LectFenEvt.affiche = 0;
	LectFenSpectre.affiche = 0;
	LectFenBruit.affiche = 0;
	LectModeSauvegarde = LECT_SVG_NEANT;
	LectSeuilRelatif = 50.0;
	LectRetard.start.mode = LectRetard.stop.mode = NEANT;
	strcpy(LectRetard.start.jour,L_("direct")); LectRetard.start.heure[0] = '\0';
	strcpy(LectRetard.stop.jour,L_("seulement")); LectRetard.stop.heure[0] = '\0';
	LectDemarrageDemande = 0;
	LectPlancheSuiviMargeX = LectPlancheSuiviMargeY = 0;

	RegenEnCours = 0; LectSourceActive = 0; LectCntl.polar = 0.0;
	RelaisFermes = 0; RegenManuelle = 1;
	MiniStream = 0; LectStreamNum = 0;

	RepertoireCreeRacine(FichierRunCaract);
	if(SambaLogDemarrage) printf("= Lecture des caracteristiques          dans '%s'\n",FichierRunCaract);
	if(ArgScan(FichierRunCaract,LectCaractDesc) == 0) ArgPrint(FichierRunCaract,LectCaractDesc);
	if(ImprimeConfigs) {
		printf("#\n### Fichier: '%s' ###\n",FichierRunCaract);
		ArgPrint("*",LectCaractDesc);
	}
	Archive.demandee = Archive.demandee;
	
	RepertoireCreeRacine(FichierEtatElec);
	if(SambaLogDemarrage) printf("= Lecture de l'etat electrique          dans '%s'\n",FichierEtatElec);
	if(ArgScan(FichierEtatElec,LectEtatDesc) == 0) ArgPrint(FichierEtatElec,LectEtatDesc);
	if(ImprimeConfigs) {
		printf("#\n### Fichier: '%s' ###\n",FichierEtatElec);
		ArgPrint("*",LectEtatDesc);
	}
	if(!LectSurFichier) {
		if(RegenEnCours) sprintf(LectInfo[0],L_("Regeneration depuis le %s a %s"),RegenDebutJour,RegenDebutHeure);
		else strcpy(LectInfo[0],L_("Polarisation normale"));
	}
	
#ifdef NIDAQ
	for(i=0; i<NI_TACHES_MAX; i++) { LectNITache[i] = 0; LectNIactive[i] = 0; }
#endif
	SequencesLit();
	TempsTrigger[0] = TempsTrigger[15] = '\0';
	MonitP0 = 0; MonitT0 = 0.0;
	LectTamponInit = 0; LectTamponSize = 0;
	strcpy(LectSrceTexte,"une activite indefinie");
//	LectAllume = 0;
	LectChangeParms = 1;
	LectReglageEnCours = 0;
	LectDepileEnCours = LectTrmtEnCours = 0;
	LectRtEnCours = 0;
    LectIdentFaite = 0;
	LectDerniereD3 = 0;
    LectLogComp = 0;
	LectErreurFichier = 0;
	for(i=0; i<99; i++) LectDetectRtneNum[i] = i / 10;
	LectDetectRtneNum[33] = 6;
	DetecReglagesLocaux = 0; DetecReglagesLocauxNb = 0;
	for(type_panel=LECT_CONS_ETAT; type_panel<LECT_CONS_NB; type_panel++) pReglagesConsignes[type_panel] = 0;
	pLectTraitements = 0;
	LectDepileInhibe = 0;
	LectDepileSousIt = 0;
	LectTrmtSousIt = 0;

	if(SambaLogDemarrage) printf("= Commentaires sur le run               dans '%s'\n",FichierRunComment);
	k = 0;
	if((f = fopen(FichierRunComment,"r"))) {
		while(LigneSuivante(ligne,256,f)) {
			if((ligne[0] == '#') || (ligne[0] == '\n')) continue;
			lngr = strlen(ligne) - 1; if(ligne[lngr] == '\n') ligne[lngr] = '\0';
			strncpy(&(LectComment[k][0]),ligne,MAXCOMMENTLNGR-1); LectComment[k][MAXCOMMENTLNGR-1] = '\0';
			if(SambaLogDemarrage) printf("  | %s\n",&(LectComment[k][0]));
			if(++k >= MAXCOMMENTNB) break;
		}
		fclose(f);
		remove(FichierRunComment);
	} else if(SambaLogDemarrage) printf("  . Pas de commentaire predefini\n");
	while(k < MAXCOMMENTNB) LectComment[k++][0] = '\0';

#ifdef NOUVEAU
	LectBoucleExterne = OpiumTimeGiven(Lect1Boucle);
#else
	LectBoucleExterne = OpiumTimeGiven(LectDisplay); // 0;
#endif
	LectTaskReadout = TimerCreate(LectItRecue,0);
//	TimerStart(LectTaskReadout,SettingsReadoutPeriod*1000);

	if(ModeBatch) return;

/*
 * Creation de l'interface
 */
	mLect = MenuLocalise(iLect);
	mLectTotal = MenuIntitule(iLectTotal,"Lecture");
#ifdef SPECTRES_SEQUENCES
	mLectSequences = MenuLocalise(iLectSequences);
#endif /* SPECTRES_SEQUENCES */
	OpiumEnterFctn(mLectTotal->cdr,LectEffaceMenu,mLect);
	OpiumEnterFctn(mLect->cdr,LectEffaceMenu,mLectTotal);

	pLectConfigChoix = PanelCreate(1);
	i = PanelList(pLectConfigChoix,"Configuration a utiliser",ConfigListe,&ConfigNum,CONFIG_NOM);
	PanelItemColors(pLectConfigChoix,i,SambaVertJauneOrangeRouge,4); PanelItemSouligne(pLectConfigChoix,i,0);

	pLectMode = pLectRegen = pDureeSynchro = pReprises = pLectEcoule = pLectPileMax = 0;
	mLectRegen = mLectSupplements = 0;
	cLectTauxPile = cCpuTauxOccupe = cLectPerteIP = cTempsMort = cPerteEvts = 0;
	
//+	WndUserKeyAdd(XK_Alt_L,"d",LectDemarre);
//+	WndUserKeyAdd(XK_Alt_L,"s",LectStop);

}
/* ========================================================================== */
int LectSetup() {
	int bolo,voie,cap,y;
	// int nbitems,nbinfos,x,x0,xg,yg,xm,dx,dy,ab6,ord;
	// char fin_de_serie;
	// Graph g; Figure fig; Instrum instrum;
	Panel p;
	int i,ligs,cols;
	OpiumColorState s; int k;
	int occupant[MAXTOURS][MAXGALETTES];
	char tour_non_vide[MAXTOURS],galt_non_vide[MAXGALETTES];
	short galette,trou,branche; int tour,ba,capmax[MAXGALETTES];
	int gmin,gmax,tmin,tmax,gvide,tvide,gok,tok;
	int lngr;
	
	if(StartFromScratch) return(1);

	/* Description de l'environnement (experiment-dependent, donc defini dans les modeles) */
	if(!LectEnvirDesc) {
		ArgDesc *elt;
		LectEnvirDesc = ArgCreate(2);
		elt = LectEnvirDesc;
		ArgAdd(elt++,"Type",          DESC_KEY RunCategCles, &RunCategNum, "Type de run");
//		ArgAdd(elt++,"Environnement", DESC_KEY RunEnvir, &SettingsRunEnvr, "Environnement du run");
	}
	RepertoireCreeRacine(FichierRunEnvir);
	printf("= Lecture de l'environnement            dans '%s'\n",FichierRunEnvir);
	if(ArgScan(FichierRunEnvir,LectEnvirDesc) == 0) ArgPrint(FichierRunEnvir,LectEnvirDesc);
	if(ImprimeConfigs) {
		printf("#\n### Fichier: '%s' ###\n",FichierRunEnvir);
		ArgPrint("*",LectEnvirDesc);
		printf("\n");
	}
	LectCntl.RunCategNum = RunCategNum;
	// LectCntl.SettingsRunEnvr = SettingsRunEnvr;

/*
 * Panel "Consignes des detecteurs"
 */
	LectConsignesConstruit();

/* 
 * Planche de bord "prise de donnees"
 */
	if((BolosUtilises > 1) || !SettingsLectureGlobale) {
		LectCompacteUtilisee = 0;
		bLecture = BoardCreate();
		LectPlancheGenerale();
	} else {
		//- LectCompacteDonnees();
		LectCompacteUtilisee = 1;
		LectCompacteNumero = LECT_COMPACTE_DONNEES;
		bLecture = 0;
		LectCompacteRefait();
	}
	bSuiviEvts = BoardCreate();
	mLectSuiviFocus = 0;
	
/*
 * Spectres glissants
 */
	bLectSpectre = BoardCreate(); OpiumTitle(bLectSpectre,"Spectre au vol");
	InfoWrite(infoLectSpectre = InfoCreate(1,32),1,"Incorporation du jaune");
	BoardAddInfo(bLectSpectre,infoLectSpectre,Dx,Dy,0);
	mLectSpectre = MenuLocalise(itemsLectSpectre);
	MenuColumns(mLectSpectre,2);
	OpiumSupport(mLectSpectre->cdr,WND_PLAQUETTE);
	BoardAddMenu(bLectSpectre,mLectSpectre,WndColToPix(8),WndLigToPix(3),0);
//	OpiumPosition(mLectSpectre->cdr,10,50);
	OpiumPosition(bLectSpectre,450,400);

/*
 * Etat des detecteurs
 */
	bEtatDetecteurs = BoardCreate(); OpiumTitle(bEtatDetecteurs,L_("Etat des detecteurs"));
	p = pReglagesConsignes[LECT_CONS_ETAT]; PanelSupport(p,WND_CREUX);
	BoardAddPanel(bEtatDetecteurs,p,Dx,Dy,0);
	iReglagesEtatRun = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&ReglagesEtatDemande,L_("fige/lecture"));
	OpiumSupport(iReglagesEtatRun->cdr,WND_RAINURES);
	InstrumOnChange(iReglagesEtatRun,LectConsignesRelit,0);
	BoardAddInstrum(bEtatDetecteurs,iReglagesEtatRun,OPIUM_EN_DESSOUS_DE 0);
	p = pEtatDetecteurs = PanelCreate(2);
	PanelColumns(p,2); PanelMode(p,PNL_RDONLY|PNL_DIRECT|PNL_BYLINES); PanelSupport(p,WND_CREUX);
	PanelInt (p,"Synchro",&NumeriseurEtatStamp);
	PanelText(p,L_("a"),NumeriseurEtatDate,12);
	BoardAddPanel(bEtatDetecteurs,p,OPIUM_A_DROITE_DE iReglagesEtatRun->cdr);
	
/*
 * Lignes de base
 */
	bLectBases = BoardCreate();
	p = PanelCreate(2);
	PanelColumns(p,2);
	PanelInt(p,"Le detecteur #",&LectBoloDemande);
	PanelItemSelect(p,PanelList(p,"s'appelle",BoloNom,&BoloNum,DETEC_NOM),0);
	PanelOnApply(p,LectNomDetecteur,&LectBoloDemande);
	PanelSupport(p,WND_CREUX);
	BoardAddPanel(bLectBases,p,10,10,0);
	strcpy(LectBaseTexteNiv,"niveaux");
	strcpy(LectBaseTexteBruit,"bruits");
	p = pLectBaseValeurs = PanelCreate(2 * (VOIES_TYPES + 1));
	PanelColumns(p,2);
	PanelItemSelect(p,PanelText(p,0,LectBaseTexteNiv,10),0);
	for(i=0; i<ModeleVoieNb; i++)
		PanelItemSelect(p,PanelFloat(p,ModeleVoie[i].nom,&(LectBase1Niveau[i])),0);
	PanelItemSelect(p,PanelText(p,0,LectBaseTexteBruit,9),0);
	for(i=0; i<ModeleVoieNb; i++)
		PanelItemSelect(p,PanelFloat(p,0,&(LectBase1Bruit[i])),0);
	PanelSupport(p,WND_CREUX);
	BoardAddPanel(bLectBases,p,40,50,0);
	gLectBaseNiveaux = GraphCreate(200,200,2 * ModeleVoieNb);
	OpiumColorInit(&s);
	for(i=0; i<ModeleVoieNb; i++) {
		GraphAdd(gLectBaseNiveaux,GRF_INT|GRF_INDEX|GRF_XAXIS,0,DETEC_MAX);
		y = GraphAdd(gLectBaseNiveaux,GRF_FLOAT|GRF_YAXIS,LectBaseNiveau[i],DETEC_MAX);
		GraphDataTrace(gLectBaseNiveaux,y,GRF_HST,0);
		k = OpiumColorNext(&s);
		GraphDataRGB(gLectBaseNiveaux,y,OpiumColorRGB(k));
	}
	GraphUse(gLectBaseNiveaux,0);
	OpiumSupport(gLectBaseNiveaux->cdr,WND_CREUX);
	GraphParmsSave(gLectBaseNiveaux);
	BoardAdd(bLectBases,gLectBaseNiveaux->cdr,10,120,0);
	gLectBaseBruits = GraphCreate(200,200,2 * ModeleVoieNb);
	OpiumColorInit(&s);
	for(i=0; i<ModeleVoieNb; i++) {
		GraphAdd(gLectBaseBruits,GRF_INT|GRF_INDEX|GRF_XAXIS,0,DETEC_MAX);
		y = GraphAdd(gLectBaseBruits,GRF_FLOAT|GRF_YAXIS,LectBaseBruit[i],DETEC_MAX);
		GraphDataTrace(gLectBaseBruits,y,GRF_HST,0);
		k = OpiumColorNext(&s);
		GraphDataRGB(gLectBaseBruits,y,OpiumColorRGB(k));
	}
	GraphUse(gLectBaseBruits,0);
	OpiumSupport(gLectBaseBruits->cdr,WND_CREUX);
	GraphParmsSave(gLectBaseBruits);
	BoardAdd(bLectBases,gLectBaseBruits->cdr,220,120,0);
	OpiumTitle(bLectBases,"Lignes de base");
	
/*
 * Taux d'evenements
 */
	for(tour=0; tour<MAXTOURS; tour++) for(galette=0; galette<MAXGALETTES; galette++) occupant[tour][galette] = -1;
	tmin = MAXTOURS; gmin = MAXGALETTES;
	tmax = gmax = -1;
	for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].local) {
		if(VerifpTauxEvts) printf("%s: ",Bolo[bolo].nom);
		if(Bolo[bolo].pos == 0xFF) {
			char tout_seul;
			tout_seul = 1;
			for(k=0; k<Bolo[bolo].nbassoc; k++) {
				if((ba = Bolo[bolo].assoc[k]) >= 0) {
					CablageAnalysePosition(Bolo[ba].pos,&galette,&trou,&branche);
					tout_seul = 0;
				}
			}
			if(VerifpTauxEvts) { if(tout_seul) printf("isole\n"); else printf("associe "); }
			if(tout_seul) continue;
		} else CablageAnalysePosition(Bolo[bolo].pos,&galette,&trou,&branche);
		if((trou < MAXTROUS) && (galette < MAXGALETTES) && (branche < MAXBRANCHES)) {
			if(VerifpTauxEvts) printf(" au trou %d.%d de la galette %d\n",trou,branche,galette);
			tour = (trou * MAXBRANCHES) + branche;
			occupant[tour][galette] = bolo;
			if(tour < tmin) tmin = tour;
			if(tour > tmax) tmax = tour;
			if(galette < gmin) gmin = galette;
			if(galette > gmax) gmax = galette;
		} else if(VerifpTauxEvts) printf(" en orbite (trou %d.%d, galette %d)\n",trou,branche,galette);
	}
	for(tour=tmin; tour<=tmax; tour++) tour_non_vide[tour] = 0;
	gvide = 0; ligs = 0;
	for(galette=gmin; galette<=gmax; galette++) {
		capmax[galette] = 0;
		for(tour=tmin; tour<=tmax; tour++) if((bolo = occupant[tour][galette]) >= 0) {
			tour_non_vide[tour] = 1;
			if(Bolo[bolo].nbcapt > capmax[galette]) capmax[galette] = Bolo[bolo].nbcapt;
		}
		if(capmax[galette]) { galt_non_vide[galette] = 1; ligs += (1 + capmax[galette]); }
		else { galt_non_vide[galette] = 0; gvide++; }
		if(VerifpTauxEvts) printf("La galette %d a %d ligne%s (%s)\n",galette,Accord1s(capmax[galette]),galt_non_vide[galette]?"occupee":"vide");
	}
	tvide = 0; cols = 0; for(tour=tmin; tour<=tmax; tour++) if(tour_non_vide[tour]) cols++; else tvide++;
	if(VerifpTauxEvts) {
		tok = tmax - tmin + 1 - tvide;
		gok = gmax - gmin + 1 - gvide;
		printf("Tours    [%d, %d] - %d vide%s, soit %d representee%s\n",tmin,tmax,Accord1s(tvide),Accord1s(tok));
		printf("Galettes [%d, %d] - %d vide%s, soit %d representee%s\n",gmin,gmax,Accord1s(gvide),Accord1s(gok));
		printf("Panel avec %d colonne%s et %d ligne%s\n",Accord1s(cols),Accord1s(ligs));
	}
	if((cols > 0) && (ligs > 0)) {
		p = pTauxDetaille = PanelCreate(cols * ligs);
		PanelColumns(p,cols);
		PanelTitle(p,"Taux d'evenements");
		PanelMode(p,PNL_RDONLY);
		lngr = 8;
		for(tour=tmin; tour<=tmax; tour++) if(tour_non_vide[tour]) {
			for(galette=gmax; galette>=gmin; galette--) if(galt_non_vide[galette]) {
				bolo = occupant[tour][galette];
				if(bolo >= 0) {
					PanelItemSelect(p,PanelText(p,0,Bolo[bolo].nom,lngr),0);
					for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
						if((voie = Bolo[bolo].captr[cap].voie) < 0) i = PanelItemSelect(p,PanelText(p,0,"",lngr),0);
						else {
							i = PanelFloat(p,ModeleVoie[VoieManip[voie].type].nom,&(VoieTampon[voie].taux));
							PanelItemFormat(p,i,"%8f"); PanelItemLngr(p,i,8);
//-							PanelItemRScale(p,i,0.3,1.7);
							PanelItemRScale(p,i,TrmtRegulAttendu/4.0,TrmtRegulAttendu*4.0);
							PanelItemColors(p,i,SambaArcEnCiel,255);
						}
					}
					for(; cap<capmax[galette]; cap++) i = PanelItemSelect(p,PanelText(p,0,"",lngr),0);
				} else {
					PanelItemSelect(p,PanelText(p,0,"",lngr),0);
					for(cap=0; cap<capmax[galette]; cap++) i = PanelItemSelect(p,PanelText(p,0,"",lngr),0);
				}
				if(galette > gmin) PanelItemSouligne(p,i,1);
			}
		}
		OpiumPosition(p->cdr,20,500);
	} else pTauxDetaille = 0;
	/* Evolution des taux */
	gTauxDetaille = GraphCreate(MonitLarg,MonitHaut,1 + VoiesNb);
	OpiumPosition(gTauxDetaille->cdr,20,700);
	GraphTitle(gTauxDetaille,L_("Taux de declenchement"));

	/* Evolution des seuils */
	gSeuils = GraphCreate(MonitLarg,MonitHaut,1 + (2 * VoiesNb));
	OpiumPosition(gSeuils->cdr,20+MonitLarg+10,700);
	GraphTitle(gSeuils,L_("Seuils de declenchement"));

/* 
 * Demarrage d'un run sauvegarde
 */
	pRunParms = PanelCreate(EnvirVarNb + MAXCOMMENTNB);
	PanelAddDesc(pRunParms,LectEnvirDesc,16);
	for(i=0; i<MAXCOMMENTNB; i++) PanelText(pRunParms,L_("Commentaire"),LectComment[i],MAXCOMMENTLNGR);


/*
 * Autres fenetres...
 */
	pFichierLectLect = PanelCreate(1);
	PanelFile(pFichierLectLect,"Depuis le fichier",FichierPrefLecture,MAXFILENAME);
	pFichierLectEcri = PanelCreate(1);
	PanelFile(pFichierLectEcri,"Sauver sur le fichier",FichierPrefLecture,MAXFILENAME);

	iLectPause = InfoCreate(1,50);
	InfoWrite(iLectPause,1,"Lecture suspendue (refaire Pause ou Stopper)");

	pLectBuffPrint = PanelCreate(7);
	i = PanelList(pLectBuffPrint,"Detecteur",BoloNom,&BoloNum,DETEC_NOM);
	PanelItemExit(pLectBuffPrint,i,DetecteurListeCapteurs,0);
	PanelList(pLectBuffPrint,"Voie",CapteurNom,&CapteurNum,MODL_NOM);
	i = PanelKeyB(pLectBuffPrint,"Donnees","brutes/traitees",&LectBuffOrigType,9);
	PanelItemExit(pLectBuffPrint,i,LectBuffPrintLimites,0);
	PanelInt (pLectBuffPrint,"Point initial",&LectBuffVal0);
	PanelInt (pLectBuffPrint,"Point final",&LectBuffValN);
	PanelKeyB(pLectBuffPrint,"Destination","journal/fichier",&LectBuffDestType,8);
	PanelKeyB(pLectBuffPrint,"Format","decimal/hexa",&LectBuffHexa,8);

	pLectEtat = PanelCreate(6);
	PanelItemSelect(pLectEtat,PanelKeyB(pLectEtat,"Lecture","arretee/activee",&Acquis[AcquisLocale].etat.active,8),0);
	PanelItemSelect(pLectEtat,PanelInt (pLectEtat,"Numero de run",&NumeroLect),0);
/*	PanelItemSelect(pLectEtat,PanelInt(pLectEtat,"Points lus sur FIFO",&LectStampsLus),0); */
	PanelItemSelect(pLectEtat,PanelLong(pLectEtat,"Modulations lues sur FIFO",&SynchroD2lues),0);
	PanelItemSelect(pLectEtat,PanelInt(pLectEtat,"Evenements",&Acquis[AcquisLocale].etat.evt_trouves),0);
	PanelItemSelect(pLectEtat,PanelFormat(pLectEtat,PanelFloat(pLectEtat,"Taux (Hz)",&TousLesEvts.freq),"%.2f"),0);
	OpiumTitle(pLectEtat->cdr,"Etat de la lecture");

/*
 * Divers
 */
	pRunMode = PanelCreate(2);
	PanelKeyB(pRunMode,"Run","en cours/nouveau",&LectCntl.LectRunNouveau,9);
	PanelKeyB(pRunMode,"de",RunCategCles,&LectCntl.RunCategNum,TYPERUN_NOM);
	PanelSupport(pRunMode,WND_CREUX);

	pLectRtAuto = PanelCreate(2);
    PanelInt(pLectRtAuto,"Attente (s)",&LectRtPas);
    PanelInt(pLectRtAuto,"Nb mesures",&LectRtNb);
	
	iLectIdent = 0;

#ifdef MODULES_RESEAU
	// InstrumOnChange(cArchive,LectUpdateControl,0);
	PanelOnApply(pRunMode,LectUpdateControl,0);
	if(pDureeSynchro) PanelOnApply(pDureeSynchro,LectUpdateControl,0);
	PanelOnApply(pLectMode,LectUpdateControl,0);
	// OpiumTimer(cArchive->cdr,500);
	/* douteux:
	OpiumTimer(pRunMode->cdr,500);
	if(pDureeSynchro) OpiumTimer(pDureeSynchro->cdr,500);
	*/
#endif
	return(1); 
}
/* ========================================================================== */
int LectExit() {
	TimerStop(LectTaskReadout); TimerDelete(LectTaskReadout);
	return(SambaSauve(&LectASauver,"preferences de lecture",1,1,LectSauve));
}
