#define BRANCHE_SETTINGS
#include <environnement.h>

//#ifdef macintosh
//#pragma message(__FILE__)
//#endif
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef CODE_WARRIOR_VSN
#include <string.h>
#else
#include <strings.h>
#endif
#include <math.h>
#include <claps.h>
#include <opium_demande.h>
#include <opium.h>
#include <impression.h>
#include <decode.h>
#include <dateheure.h>
#include <samba.h>
#include <detecteurs.h>
#include <numeriseurs.h>
#include <repartiteurs.h>
#include <cablage.h>
#include <gene.h>
#include <organigramme.h>
// #include <cmdes.h>
#include <filtres.h>
#include <autom.h>
#include <objets_samba.h>
#include <archive.h>
#include <monit.h>
// #include <autom.h>
#include <export.h>

/* #define PLANCHE_PANELS */
/* #define MODES_DYNAMIQUE */
#define DESTRUC_MANU

static char SettingsFinessesASauver;
// static char SettingsTrmtTexte[TRMT_NBCLASSES][DETEC_CAPT_MAX][MODL_NOM+16];
// static char SettingsCablTexte[DETEC_CAPT_MAX][MODL_NOM+10];
static WndContextPtr SettingsGC;

static TypeFigZone SettingsTitreSecteur  = { 200, 14, WND_RAINURES,  0x7FFF, 0x7FFF, 0xFFFF };
// static TypeInstrumPotar SettingsContact  = { -(PI / 3.0), (PI / 3.0), 15.0, 10.0 };
static TypeFigDroite SettingsOptVert1    = { 0, 0, WND_RAINURES, 0x0000, 0x0000, 0x0000 };
static TypeFigDroite SettingsOptHori1    = { 0, 0, WND_RAINURES, 0x0000, 0x0000, 0x0000 };
// static TypeFigDroite SettingsLigneHoriz  = { 0, 0, WND_REGLETTES, 0x0000, 0x0000, 0x0000 };
#ifdef CENTRAGE_INDEPENDANT
	static Instrum cCentrage;
#endif

static Instrum cRunFamille,cModeDonnees,cImprBolos,cTrmtInline,cModeCalcEvt,cModeDateEvt,cTamponUtilise,cGenePresent;
static Instrum cPeriodes,cNtuples;
//- static Instrum cSrcType,cEtatFinal,cAltivec;
static Instrum cMultitasks,cSansTpn,cSoustra,cSauteEvt,cTrigOnline,cDeconv,cCoupures,cConformite,cCalage,cLisStatus;
static Instrum cIdentCheck,cStatusCheck;
static Instrum cModeLecture;
#ifdef MODULES_RESEAU
	static Instrum cTrigger
#endif

static Panel pFolderActn; // pFolder deja pris (voir OpenScripting/FinderRegistry.h)

static char   SettingsDocFormat,SettingsDocSupport;
static char   SettingsDocCablage,SettingsDocModeles,SettingsDocAutom,SettingsDocScripts;
static Panel pSettingsImprimeForme,pSettingsImprimeSvr;
static int  NbTirages;
static char NumPrinter;
static char ListePrinters[32];

static Panel pBuffer;

static Menu mGestParms,mGestCal,mGestExport,mGestAppel,mGestConfig,mGestGeneraux;
#ifdef PARMS_PAR_MENUS
	static Menu mGestRegulEvt,mGestAffichage,mGestCalcul,mGestVerif,mGestFinesses;
#endif
static Menu mTemplates,mPattern;
static Panel pSambaDesc,pConfigDesc;
extern ArgDesc SambaDesc[],Setup[];

static Panel pTemplate;
static float TemplateValeur;
static int TemplateDepart,TemplateArrivee;
static char TemplateUniteNum;
typedef enum {
	TRMT_UNITE_ADU = 0,
	TRMT_UNITE_NV,
	TRMT_UNITE_KEV,
	TRMT_UNITE_MAX
} TRMT_UNITE;
static char *TemplateUniteListe[TRMT_UNITE_MAX+1] = { "ADU", "nV", "keV", "\0" };

static Panel pViChoix;

static Panel pSettingsDesc;
static ArgDesc SettingsDesc[] = {
	{ "Echantillonnage",         DESC_FLOAT &Echantillonnage,          "Frequence d'echantillonnage demandee (kHz)" },
	{ "Partition.reelle",        DESC_KEY SAMBA_PARTIT_CLES, &SettingsPartition,            "Partition reellement utilisee ("SAMBA_PARTIT_CLES")" },
	{ "Classement.numeriseurs",  DESC_LISTE ClasseInfo, &(ClassementDemande[CLASSE_NUMER]), "Ordre de presentation des numeriseurs" },
	{ "Classement.detecteurs",   DESC_LISTE ClasseInfo, &(ClassementDemande[CLASSE_DETEC]), "Ordre de presentation des detecteurs" },
	{ "Longueur.fibres",         DESC_INT   &SettingsFibreLngr,        "Longueur des fibres Numeriseurs-OPERA (m)" },
	{ "Donnees.tampon",          DESC_INT   &DureeTampons,             "Profondeur tampon par bolo et par voie (ms)" },
	{ "Compensation.mini",       DESC_NONE }, // DESC_FLOAT &CompensationApprox,     "Valeur de depart pour la compensation" },
	{ "Compensation.attente",    DESC_FLOAT &CompensationAttente,      "Attente apres chgt amplitude pour la compensation (ms)" },
	{ "Detec.entretien.periode", DESC_INT   &SettingsDLUdetec,         "Delai entre deux regulations de detecteurs (s)" },
	{ "Detec.charge_v1",         DESC_BOOL  &SettingsChargeReglages,   "Chargement prealable des reglages au lancement des reglages de voie" },
	{ "Detec.charge_run",        DESC_BOOL  &SettingsChargeBolos,      "Chargement des numeriseurs avant run si sauvegarde" },
	{ DESC_COMMENT 0 },
	{ DESC_COMMENT "Lecture des donnees" },
	/* Lect.* a passer dans lect */
	{ "Lect.retardateur",        DESC_BOOL  &LectRetardOption,         "Presence d'un retardateur dans la planche de prise de donnees" },
	{ "Lect.IP.validite",        DESC_FLOAT &RepartUdpRelance,         "Duree d'abonnement aux donnees UDP (s)" },
	{ "Lect.stoppe.repart",      DESC_BOOL  &LectStoppeRepart,         "Arrete les repartiteurs sur 'stop run'" },
	{ "Lect.taux.periode",       DESC_INT   &SettingsTauxPeriode,      "Periode de calcul du taux d'evenement (s)" },
	{ "Lect.taux.seuil",         DESC_FLOAT &LectTauxSeuil,            "Seuil d'alarme de taux d'evenement (Hz)" },
	{ "Lect.delai.mini",         DESC_FLOAT &LectDelaiMini,            "Seuil de delai pour enregistrement (ms)" },
	{ "Lect.duree.reprise",      DESC_INT   &SettingsRegenDelai,       "Duree minimum pour reprise apres regen (s)" },
	{ "Lect.duree.raz.NTD",      DESC_INT   &SettingsRazNtdDuree,      "Duree du RAZ des FET NTD (s)" },
	{ "Lect.duree.raz.NbSi",     DESC_INT   &SettingsRazNbSiDuree,     "Duree du RAZ des FET NbSi apres polarisation (s)" },
	{ "Lect.duree.limite",       DESC_KEY "sans/active",
	                                      &LectDureeActive,            "Limitation de la lecture" },
	{ "Lect.duree.maxi",         DESC_INT   &LectDureeMaxi,            "Duree maximum de la lecture (mn)" },
	{ "Lect.duree.tranche",      DESC_INT   &LectDureeTranche,         "Duree d'une tranche de sauvegarde (mn)" },
	{ "Lect.sequencement",       DESC_BOOL  &LectAutoriseSequences,    "Activation des sequences" },
	{ "Lect.signal.temps",       DESC_NONE }, // DESC_INT   &LectSignalTemps,       "Graduation en temps pour affichage signal" },
	{ "Lect.signal.amplitude",   DESC_INT   &LectSignalAmpl,           "Graduation en ADU pour affichage signal" },
	{ "Lect.signal.zero",        DESC_INT   &LectSignalZero,           "Milieu ecran  pour affichage signal" },
	{ "Lect.start.synchro",      DESC_NONE }, // DESC_KEY "neant/repart/seconde", &LectSynchroType, "Calage du demarrage de la lecture (neant/repart/seconde)" },
	{ "Lect.synchro.type",       DESC_KEY LECT_SYNCHRO_CLES,     &LectSynchroType, "Calage du demarrage de la lecture ("LECT_SYNCHRO_CLES")" },
	{ "Lect.BO.redemande",       DESC_NONE }, // DESC_BOOL  &RepartIpInsiste,       "Redemande les trames de BO qui manquent" },
	{ "Lect.IP.erreur.log",      DESC_NONE }, // DESC_BOOL  &VerifErreurLog,        "Imprime les details des erreurs de transmission" },
	{ "Lect.IP.erreur.stoppe",   DESC_NONE }, // DESC_BOOL  &VerifErreurStoppe,     "Arrete le repartiteur en cas d'erreur de transmission" },
	{ "Lect.IP.perdus",          DESC_KEY REPART_CORRECTION_CLES,  &RepartIpCorrection, "Action a prendre sur trames manquantes ("REPART_CORRECTION_CLES")" },
	{ "Lect.restart.delai",      DESC_INT   &LectAttenteErreur,        "Duree minimum pour reprise apres erreur (s)" },
	{ "Lect.BN.decode",          DESC_BOOL  &LectDecodeStatus,         "Decode l'etat des numeriseurs [fait maintenant par les BO]" },
	{ "Lect.BN.status",          DESC_DEVIENT("Lect.BN.status.lit")    },
	{ "Lect.BN.sauve",           DESC_DEVIENT("Lect.BN.status.sauve")  },
	{ "Lect.BN.status.lit",      DESC_BOOL  &LectLitStatus,            "Lit l'etat decode (trame -1) des numeriseurs" },
	{ "Lect.BN.status.sauve",    DESC_BOOL  &LectSauveStatus,          "Sauve l'etat decode (trame -1) des numeriseurs" },
	{ "Lect.ident-check",        DESC_BOOL  &SettingsIdentCheck,       "Verification de l'identification des numeriseurs" },
	{ "Lect.status-check",       DESC_BOOL  &SettingsStatusCheck,      "Verification du status (10eme mot du bloc)" },
	{ "Lect.trig_online",        DESC_BOOL  &LectTrigOnline,           "Traitement et trigger online dans la lecture" },
	{ "Lect.source.acces",       DESC_NONE }, // DESC_BOOL  &AutomSrceAccede,          "Autorisation de deplacer les sources" },
	{ "Lect.regen.nom.1",        DESC_NONE }, // DESC_STR(16) &(AutomRegenNom[0][0]),  "Variable de commande de la 1ere source pour la regeneration" },
	{ "Lect.regen.nom.2",        DESC_NONE }, // DESC_STR(16) &(AutomRegenNom[1][0]),  "Variable de commande de la 2eme source pour la regeneration" },
	{ "Lect.calib.nom.1",        DESC_NONE }, // DESC_STR(16) &(AutomCalibNom[0][0]),  "Variable de commande de la 1ere source pour la calibration" },
	{ "Lect.calib.nom.2",        DESC_NONE }, // DESC_STR(16) &(AutomCalibNom[1][0]),  "Variable de commande de la 2eme source pour la calibration" },
	{ "Lect.secu.polar0",        DESC_BOOL  &SettingsSecuPolar,        "Polarisation a 0 automatiquement en cas d'erreur de lecture" },
	{ "Interv.sequence",         DESC_INT   &SettingsIntervSeq,        "Intervalle entre sequencements (ms)" },
	{ "Gene.autorise",           DESC_BOOL  &SettingsGenePossible,     "Permet l'utilisation des generateurs meme si lecture reelle" },
	{ "Deconvolution.autorise",  DESC_BOOL  &SettingsDeconvPossible,   "Permet de calculer la deconvoluee des evenements" },

	{ DESC_COMMENT 0 },
	{ DESC_COMMENT "Recherche des evenements" },
	{ "Trigger.postcontrole",    DESC_DEVIENT("Trigger.coupures") },
	{ "Trigger.coherence",       DESC_DEVIENT("Trigger.conformite") },
	{ "Trigger.demande",         DESC_BOOL  &Trigger.demande,          "Detection d'evenement a activer (sinon, c'est un stream)" },
	{ "Trigger.coupures",        DESC_BOOL  &SettingsCoupures,         "Coupures (apres calcul evt) sur max ampl,duree,bruit" },
	{ "Trigger.conformite",      DESC_BOOL  &SettingsConformite,       "Controle si l'evt calcule est conforme a l'evt demande" },
	{ "Trigger.connexion",       DESC_BOOL  &SettingsTrigInOut,        "Permet de transmettre l'info 'trigger' d'une voie a  d'autres" },
	{ "Trigger.bits.level",      DESC_KEY SAMBA_BITNIV_CLES,  &BitTriggerNiveau,  "Signification des bits des mots du 'bit-trigger' ("SAMBA_BITNIV_CLES")" },
	{ "Trigger.bits.numeros",    DESC_KEY SAMBA_BITNUM_CLES,  &BitTriggerNums,    "Numerotation des bits des mots du 'bit-trigger' ("SAMBA_BITNUM_CLES")" },
	{ "Trigger.bits.bloc",       DESC_INT                     &BitTriggerBloc,    "Nb. bits/detecteur (0: selon detecteur)" },
	{ DESC_COMMENT 0 },
	{ DESC_COMMENT "Traitement des evenements" },
	{ "Trmt.deconvolue",         DESC_BOOL  &TrmtDeconvolue,           "Deconvolution de chaque evt trigge en sous-evts sur donnees deconvoluees" },
	{ "Trmt.regul.evt",          DESC_BOOL  &SettingsRegulEvt,         "Regulation du taux d'evenements par fluctuation du seuil" },
	{ "Trmt.regul.delai",        DESC_INT   &TrgrRegulDelai,           "Delai de perturbation d'une regulation de DAC pour la regulation du taux d'evenements" },
	{ "Trmt.calcul",    DESC_KEY TrmtCalculCles, &SettingsCalculEvt,   "Mode de calcul de l'evenement sur les voies ajoutees" },
	{ "Trmt.datation",  DESC_KEY TrgrDateCles,   &SettingsTempsEvt,    "Datation de l'evenement (montee/maxi)" },
	{ "Trmt.sur_pretrig",        DESC_BOOL  &SettingsCalcPretrig,      "Evalue l'evenement sur le tampon du trigger (les donnees brutes sinon)" },
	{ "Trmt.montee",             DESC_BOOL  &SettingsMontee,           "Evalue les temps de montee et de descente" },
	{ "Trmt.integrales",         DESC_BOOL  &SettingsIntegrale,        "Evalue les integrales de l'evenement a differentes phases" },
	{ "Trmt.pattern",            DESC_KEY TRMT_PTN_TEXTES, &SettingsSoustra, "Soustraction du pattern moyen ("TRMT_PTN_TEXTES")" },
	{ "Trmt.saute_evt",          DESC_BOOL  &SettingsSauteEvt,         "Sauter evt pour calculer le pattern moyen" },
	{ "Trmt.calage",             DESC_NONE }, // &SettingsCalageMode,        "Calage des temps" },
	{ "Trmt.fen.centrage",       DESC_NONE }, // DESC_KEY CALAGES_PORTEE_CLES, &SettingsCentrageType, "Centrage des evenements ("CALAGES_PORTEE_CLES")" },
	{ "Trmt.fen.ref",            DESC_NONE }, // DESC_STR(MODL_NOM)            &SettingsCentrageNom,  "Type de voie de reference si centrage 'unique' [auto=evt le plus court]" },
	{ "Trmt.evt.calage",   DESC_KEY CALAGES_PORTEE_CLES,         &SettingsCalageMode,   "Calage des temps d'evenement ("CALAGES_PORTEE_CLES")" },
	{ "Trmt.evt.ref",            DESC_STR(MODL_NOM)              &SettingsCalageModl,   "Type de voie de reference si calage 'unique' [auto=evt le plus court]" },
	{ "Trmt.evt.calage.mode",    DESC_KEY CALAGES_PORTEE_CLES,   &SettingsCalageMode,   "Reference de calage en temps des evenement ("CALAGES_PORTEE_CLES")" },
	{ "Trmt.evt.calage.objet",   DESC_KEY CALAGES_OBJET_CLES,    &SettingsCalageObj,    "Niveau de calage en temps des evenement ("CALAGES_OBJET_CLES")" },
	{ "Trmt.evt.calage.ref",     DESC_STR(MODL_NOM)              &SettingsCalageModl,   "Modele de voie de reference si calage 'unique' [auto=evt le plus court]" },
	{ "Trmt.ntuples",      DESC_KEY SAMBA_NTP_NIVEAU, &CalcNtpDemande, "Calcule les n-tuples pour l'analyse (niveau: "SAMBA_NTP_NIVEAU")" },
	{ "Trmt.filtrees",           DESC_BOOL  &SettingsStockeFiltrees,   "Stocke aussi le resultat du traitement pre-trigger" },
	{ "Trmt.suivi.max",          DESC_INT   &SettingsMaxSuivi,         "Longueur du suivi du filtrage" },
	{ "Trmt.archivage",          DESC_NONE }, // DESC_BOOL  &SettingsArchive, "Archivage des evenements ou des donnees brutes" },
    { "Trgr.garde_signe",        DESC_BOOL  &TrgrGardeSigne,           "Amplitudes du signe du trigger demande" },
	{ DESC_COMMENT 0 },
	{ DESC_COMMENT "Sauvegarde des evenements" },
	{ "Sauvegarde.evt", DESC_LISTE ArchModeNoms, &ArchDetecMode,       "Mode de sauvegarde si evts (seul/coinc/voisins/tous/manip)" },
	{ "Sauvegarde.assoc",        DESC_BOOL  &ArchAssocMode,            "Sauvegarde egalement des detecteurs associes" },
	{ "Sauvegarde.max",          DESC_INT   &EvtNbMax,                 "Nombre d'evts maxi avant vidage" },
	{ "Sauvegarde.reduction",    DESC_INT   &ArchReduc,                "Taux de reduction du nombre d'evenements sauves" },
	{ "Sauvegarde.regen",        DESC_INT   &ArchRegenTaux,            "Inverse du taux d'evenements sauves pendant la regeneration" },
	{ "Sauvegarde.auxi.duree",   DESC_NONE }, // DESC_FLOAT &BnAuxiDuree,              "Duree de signal auxiliaire sauve au total (ms)" },
	{ "Sauvegarde.auxi.pretrig", DESC_NONE }, // DESC_FLOAT &BnAuxiPretrig,          "Proportion de signal auxiliaire sauve avant evt (%)" },
	{ "Sauvegarde.Evts",         DESC_NONE }, // &ArchSauveTrace,      "Sauvegarde des traces des evenements" },
	{ "Sauvegarde.traces",       DESC_INT   &ArchSauveTrace,           "Sauvegarde des traces des evenements" },
	{ "Sauvegarde.Ntuple.dim",   DESC_DEVIENT("Sauvegarde.MonitNtp.dim") },
	{ "Sauvegarde.Ntuple.paw",   DESC_DEVIENT("Sauvegarde.MonitNtp.paw") },
	{ "Sauvegarde.MonitNtp.dim", DESC_INT   &SettingsNtupleMax,        "Dimension du ntuple memorise en cours d'acquisition" },
	{ "Sauvegarde.MonitNtp.paw", DESC_KEY NTP_MODE_CLES,  &ArchSauveNtp, "Calcul et sauvegarde du ntuple par evenement ("NTP_MODE_CLES")" },
	{ "Sauvegarde.entete",       DESC_KEY "locale/manip", &SettingsHdrGlobal, "Portee des detecteurs et voies dans l'entete (locale/manip)" },
	DESC_END
};

static Panel pSettingsFinesses;
static ArgDesc SettingsFinessesDesc[] = {
	{ "Retard.OPERA",           DESC_INT   &SettingsRetard,           "Retard de base des boites OPERA" },
	{ "Info.definition",        DESC_INT   &SambaDefMax,              "Longueur de la definition des infos rapportees" },
	{ "Interv.user",            DESC_INT   &SettingsIntervUser,       "Intervalle entre actions utilisateur (ms)" },
    { "Sauver.fichier.choix",   DESC_BOOL  &SettingsSauveChoix,       "Laisse le choix du fichier a sauver" },
	{ "Detec.reglages.v8",      DESC_NONE }, // DESC_BOOL  &SettingsDetecV8,      "'oui' si les reglages sont au format de la version 8 (ou anterieure)" },
	{ "Explore.hw",             DESC_NONE }, // DESC_BOOL  &SambaExploreHW,      "Permet le choix des repartiteurs a chaque action" },
	{ "Chassis.optimise",       DESC_BOOL  &SambaChassisOptimal,      "Reduit la dimension du chassis selon nombre de detecteurs/numeriseurs" },
	{ "HW.explore",             DESC_BOOL  &SambaExploreHW,           "Permet le choix des repartiteurs a chaque action" },
	{ "HW.ident.fiable",        DESC_BOOL  &SambaIdentSauvable,       "Permet de sauver le resultat de l'identification" },
	{ "Style.PlancheRuns",      DESC_KEY "normal/global",   &SettingsLectureGlobale, "Style de la planche des runs si detecteur unique (normal/global)" },
	{ "Style.organigramme",     DESC_KEY "boutons/icones",  &OrgaStyle,              "Style d'affichage de l'organigramme (boutons/icones)" },
	{ "Style.voies.nom",        DESC_KEY VOIE_NOM_CLES,     &SettingsVoiesNom,       "Style de nommage des voies ("VOIE_NOM_CLES")" },
	{ "Impr.detecteurs",        DESC_KEY "bb1/mixte",       &SettingsImprBolosVsn,   "Style d'impression des detecteurs (bb1/mixte)" },
	{ "Lect.run.famille",       DESC_KEY "banc/manip",      &SettingsRunFamille,     "Type de nom de run (banc/manip)" },
	{ "Lect.run.type",          DESC_KEY RunCategCles,      &RunCategNum,            "Type de run (test/<source>/fond)" },
	{ "Lect.buffer",            DESC_KEY "fixe/circulaire", &SettingsModeDonnees,    "Mode d'utilisation du buffer de lecture" },
	{ "Lect.IPE.wait",          DESC_INT   &SettingsIPEwait,          "Attente apres l'arret des transmissions (s)" },
	{ "Lect.D3.wait",           DESC_INT   &SettingsD3wait,           "Attente apres la premiere D3, avant toutes les autres (ms)" },
	{ "Lect.duree.marge",       DESC_FLOAT &SettingsDelaiIncertitude, "Marge d'erreur sur delai avant relance UDP repartiteur (s)" },
	{ "Lect.trames.maxi",       DESC_BOOL  &SettingsTramesMaxi,       "On bourre les trames avec des echantillons partiels" },
	{ "Lect.FIFO.wait",         DESC_INT   &SettingsFIFOwait,         "Nb d'acces limite a la FIFO si elle est vide" },
	{ "Lect.FIFO.tache",        DESC_KEY "serie/parallele/boostee/optimisee/v4",
		                                   &SettingsMultitasks,       "Tache de lecture (serie/parallele/boostee/optimisee/v4)" },
	{ "Lect.FIFO.periode",      DESC_INT   &SettingsReadoutPeriod,    "Periode d'activation de la lecture (ms)" },
	{ "Lect.FIFO.max",          DESC_INT   &SettingsDepileMax,        "Max boucles pour que tous soient secs (DepileOptimise)" },
	{ "Lect.auto-synchro",      DESC_BOOL  &SettingsAutoSync,         "Resynchronisation auto sur donnees perdues" },
	{ "Lect.synchro.max",       DESC_INT   &SettingsSynchroMax,       "Nb synchronisations maxi avant pause lecture" },
	{ "Lect.charge.bolos",      DESC_NONE }, // DESC_BOOL  &SettingsChargeBolos, "Chargement des numeriseurs avant run si sauvegarde" },
	{ "Lect.moduls.max",        DESC_NONE },
	{ "Lect.moduls.trmt",       DESC_NONE },
	{ "Lect.impr.vars",         DESC_NONE },
	{ "Lect.run.type",          DESC_NONE },
	{ "Lect.run.condition",     DESC_NONE },
	{ "Lect.sequence.acq",      DESC_NONE }, // DESC_INT   &LectDureeAcq,        "Temps d'acquisition avant regeneration (mn)" },
	{ "Lect.sequence.regen",    DESC_NONE }, // DESC_INT   &LectDureeRegen,      "Temps de regeneration (mn)" },
	{ "Lect.sequence.stream",   DESC_NONE }, // DESC_INT  &LectDureeStream,     "Temps de ministream (mn)" },

	{ "Teste.inlined",          DESC_BOOL  &SettingsInlineProcess,   "Teste le traitement et le trigger en fonctions inlined" },
	{ "Trigger.type",  DESC_KEY TrgrDefCles, &Trigger.type,          "Type de detection des evenements si demandes (neant/cable/scrpt/perso)" },
	{ "Trigger.scrpt",          DESC_TEXT   Trigger.nom_scrpt,       "Nom du fichier si trigger de type scrpt" },
	{ "Trigger.inhibe",         DESC_NONE }, // DESC_FLOAT &SettingsInhibe,       "Inhibition sur trigger (ms) (<0 si = duree evt)" },
	{ "Trigger.temps_mort",     DESC_NONE }, // DESC_KEY "normal/threshold", &SettingsAttendsSeuil, "Temps mort normal ou attente seuil (normal/threshold)" },
	{ "Trigger.pause",          DESC_BOOL  &SettingsPauseEvt,        "Pause sur evenement" },
	{ "Trmt.attente",           DESC_INT   &SettingsIntervTrmt,      "Intervalle entre traitements pre-trigger (ms)" },
	{ "Trmt.altivec",           DESC_BOOL  &SettingsAltivec,         "Calcul avec altivec" },
	{ "Sauvegarde.stream", DESC_KEY ArchClesStream, &ArchModeStream, "Mode de sauvegarde si stream (blocs/brutes/direct)" },
	{ "Sauvegarde.format", DESC_KEY "edw1/edw2",     &ArchFormat,    "Format de sauvegarde si evts (edw1/edw2)" },

	/* Histo.* a passer dans calculs */
	{ "Histo.voie",             DESC_INT   &SettingsHistoVoie,       "Voie histogrammee" },
	{ "Histo.ampl",             DESC_BOOL  &TrmtHampl,               "Calcul de l'histogramme des amplitudes" },
	{ "Histo.ampl.inf",         DESC_FLOAT &SettingsAmplMin,         "Amplitude minimum" },
	{ "Histo.ampl.sup",         DESC_FLOAT &SettingsAmplMax,         "Amplitude maximum" },
	{ "Histo.ampl.bin",         DESC_INT   &SettingsAmplBins,        "Nb bins pour l'histogramme des amplitudes" },
	{ "Histo.montee",           DESC_BOOL  &TrmtHmontee,             "Calcul de l'histogramme des temps de montee" },
	{ "Histo.montee.inf",       DESC_FLOAT &SettingsMonteeMin,       "Temps de montee minimum" },
	{ "Histo.montee.sup",       DESC_FLOAT &SettingsMonteeMax,       "Temps de montee maximum" },
	{ "Histo.montee.bin",       DESC_INT   &SettingsMonteeBins,      "Nb bins pour l'histogramme des temps de montee" },
	{ "Histo.2D",               DESC_BOOL  &TrmtH2D,                 "Calcul de l'histo amplitude x tps montee" },
	
	DESC_END
};

#ifdef PREFERENCES_PAR_MODULE
static ArgDesc SettingsTotal[] = { DESC_INCLUDE(SettingsDesc), DESC_INCLUDE(SettingsFinessesDesc), DESC_END };
#endif

static Panel pSettingsVerif;
static ArgDesc SettingsVerifDesc[] = {
	{ "AvecIP",              DESC_BOOL  &VerifRepertIP,       "Verification presence des repartiteurs sur IP" },
	{ "IpLog",               DESC_BOOL  &VerifIpLog,          "Imprime les details de la transmission" },
	{ "ErreurLog",           DESC_BOOL  &VerifErreurLog,      "Imprime les details des erreurs de transmission" },
	{ "ErreurStoppe",        DESC_BOOL  &VerifErreurStoppe,   "Arrete le repartiteur en cas d'erreur de transmission" },
	{ "DepileOpt",           DESC_BOOL  &VerifRunDebug,       "Verification quand le bouton 'Debug' est pousse" },
	{ "TempsCpu",            DESC_BOOL  &VerifTempsPasse,     "Verification des temps passes" },
	{ "DetailCpu",           DESC_BOOL  &VerifConsoCpu,       "Verification detaillee des temps CPU" },
	{ "TempsCalcul",         DESC_BOOL  &VerifCpuCalcul,      "Verification des temps de calcul" },
	{ "DetailCalcul",        DESC_BOOL  &VerifCalculDetail,   "Verification detaillee des temps de calcul" },
	{ "IdentBB",             DESC_BOOL  &VerifIdentBB,        "Identification des numeriseurs" },
	{ "IdentRep",            DESC_BOOL  &VerifIdentRepart,    "Identification classee par repartiteur" },
	{ "Compensation",        DESC_BOOL  &VerifCompens,        "Elements du calcul de la compensation" },
	{ "Display",             DESC_BOOL  &VerifDisplay,        "Affichage des donnees traitees" },
	{ "MonitEvts",           DESC_BOOL  &VerifMonitEvts,      "Affichage des evenements" },
	{ "EtatFinal",           DESC_BOOL  &VerifEtatFinal,      "Impression des variables internes en fin de lecture" },
	{ "pTauxEvts",           DESC_BOOL  &VerifpTauxEvts,      "Construction du panel 'pTauxDetaille'" },
	{ "TauxEvts",            DESC_BOOL  &VerifCalculTauxEvts, "Calcul du taux d'evenements par voie" },
	{ "BitTrigger",          DESC_BOOL  &VerifBittrigger,     "Verification si le BitTrigger est nul" },
	{ "Integrales",          DESC_BOOL  &VerifIntegrales,     "Deverminage des integrales" },
	{ "EvtMoyen",            DESC_BOOL  &VerifEvtMoyen,       "Deverminage de l'utilisation de l'evt moyen" },
	{ "CalculEvts",          DESC_BOOL  &VerifCalculEvts,     "Verification du calcul de la LdB et de l'amplitude" },
	{ "SuiviEtatEvts",       DESC_BOOL  &VerifSuiviEtatEvts,  "Suivi de l'etat de chaque evenement, du trigger a la memorisation" },
	{ "SuiviTrmtEvts",       DESC_BOOL  &VerifSuiviTrmtEvts,  "Suivi du traitement de chaque evenement, du trigger a la memorisation" },
	{ "SuiviEtatStrm",       DESC_BOOL  &VerifOctetsStream,   "Verification du decompte des octets transmis en mode stream" },
	{ "GestionConfigs",      DESC_BOOL  &GestionConfigs,      "Gestion des configurations de voie" },
	DESC_END
};

/* Sert comme valeurs par defaut [DetecteurParmsDefaut(DETEC_PARM_RGUL)] pour la regulation voie par voie */
static Panel pSettingsRegulEvt;
static ArgDesc SettingsRegulDesc[] = {
	{ "Taux_attendu",        DESC_FLOAT                &TrmtRegulAttendu,            "Taux d'evenements attendu (pour l'affichage)" },
	{ "Trmt.regul.evt",      DESC_KEY "non/taux/intervalles",
		                                             &TrmtRegulType, "Type de regulation du taux d'evenements (non/taux/intervalles)" },
//+ #ifdef REGUL_UNIQUE
	{ DESC_COMMENT 0 },
	{ DESC_COMMENT "Regulation par le taux d'evenements" },
	{ DESC_COMMENT 0 },
	{ "Active.1",            DESC_BOOL                 &TrmtRegulTaux[0].active,     "Echelle de temps #1 en service" },
	{ "Echelle.1",           DESC_INT                  &TrmtRegulTaux[0].duree,      "Echelle de temps #1 (mn)" },
	{ "Nb.1.min",            DESC_INT                  &TrmtRegulTaux[0].min.nb,     "Nombre d'evt minimum a l'echelle #1" },
	{ "Action.1.min.type",   DESC_KEY TrmtRegulCles,   &TrmtRegulTaux[0].min.action, "Type action si nb1 < min1" },
	{ "Action.1.min.parm",   DESC_FLOAT                &TrmtRegulTaux[0].min.parm,   "Valeur action si nb1 < min1" },
	{ "Nb.1.max",            DESC_INT                  &TrmtRegulTaux[0].max.nb,     "Nombre d'evt maximum a l'echelle #1" },
	{ "Action.1.max.type",   DESC_KEY TrmtRegulCles,   &TrmtRegulTaux[0].max.action, "Type action si nb1 > max1" },
	{ "Action.1.max.parm",   DESC_FLOAT                &TrmtRegulTaux[0].max.parm,   "Valeur action si nb1 > max1" },
	{ DESC_COMMENT 0 },
	{ "Active.2",            DESC_BOOL                 &TrmtRegulTaux[1].active,     "Echelle de temps #2 en service" },
	{ "Echelle.2",           DESC_INT                  &TrmtRegulTaux[1].duree,      "Echelle de temps #2 (mn)" },
	{ "Nb.2.min",            DESC_INT                  &TrmtRegulTaux[1].min.nb,     "Nombre d'evt minimum a l'echelle #2" },
	{ "Action.2.min.type",   DESC_KEY TrmtRegulCles,   &TrmtRegulTaux[1].min.action, "Type action si nb2 < min2" },
	{ "Action.2.min.parm",   DESC_FLOAT                &TrmtRegulTaux[1].min.parm,   "Valeur action si nb2 < min2" },
	{ "Nb.2.max",            DESC_INT                  &TrmtRegulTaux[1].max.nb,     "Nombre d'evt maximum a l'echelle #2" },
	{ "Action.2.max.type",   DESC_KEY TrmtRegulCles,   &TrmtRegulTaux[1].max.action, "Type action si nb2 > max2" },
	{ "Action.2.max.parm",   DESC_FLOAT                &TrmtRegulTaux[1].max.parm,   "Valeur action si nb2 > max2" },
	{ DESC_COMMENT 0 },
	{ "Active.3",            DESC_BOOL                 &TrmtRegulTaux[2].active,     "Echelle de temps #3 en service" },
	{ "Echelle.3",           DESC_INT                  &TrmtRegulTaux[2].duree,      "Echelle de temps #3 (mn)" },
	{ "Nb.3.min",            DESC_INT                  &TrmtRegulTaux[2].min.nb,     "Nombre d'evt minimum a l'echelle #3" },
	{ "Action.3.min.type",   DESC_KEY TrmtRegulCles,   &TrmtRegulTaux[2].min.action, "Type action si nb3 < min3" },
	{ "Action.3.min.parm",   DESC_FLOAT                &TrmtRegulTaux[2].min.parm,   "Valeur action si nb3 < min3" },
	{ "Nb.3.max",            DESC_INT                  &TrmtRegulTaux[2].max.nb,     "Nombre d'evt maximum a l'echelle #3" },
	{ "Action.3.max.type",   DESC_KEY TrmtRegulCles,   &TrmtRegulTaux[2].max.action, "Type action si nb3 > max3" },
	{ "Action.3.max.parm",   DESC_FLOAT                &TrmtRegulTaux[2].max.parm,   "Valeur action si nb3 > max3" },
	{ DESC_COMMENT 0 },
	{ DESC_COMMENT "Regulation par intervalles" },
	{ DESC_COMMENT 0 },
	{ "Interv.nb",           DESC_INT     &TrmtRegulInterv.nb,     "Nombre d'intervalles" },
	{ "Interv.duree",        DESC_FLOAT   &TrmtRegulInterv.duree,  "Longueur (s)" },
	{ "Interv.ajuste",       DESC_FLOAT   &TrmtRegulInterv.ajuste, "Ajustement seuil (ADU)" },
	{ "Interv.evt",          DESC_FLOAT   &TrmtRegulInterv.seuil,  "Facteur decision" },
	{ "Interv.min",          DESC_INT     &TrmtRegulInterv.max,    "Delai mini (nb.interv.)" },
	{ "Interv.incr",         DESC_FLOAT   &TrmtRegulInterv.incr,   "Increment si bruit (ADU)" },
//+ endif
	DESC_END
};

int SettingsTriggerCtrl();
Menu mSettingsTriggerCtrl; TypeMenuItem iSettingsTriggerCtrl[]   = { { "Suspendre trigger",  MNU_FONCTION SettingsTriggerCtrl }, MNU_END };
extern Menu mFiltres;
static char SettingsTxtStd[16];

typedef struct {
	char type;            /* type d'interface associee   */
	void *instrum;        /* adresse de l'interface      */
} TypeSettingsInstr;

#define SETTINGS_VOIE_PAR_BOLO 10
#define SETTINGS_EVTS_MAX ((1 * SETTINGS_VOIE_PAR_BOLO) + 2) /* max d'instruments pour les evenements d'un bolo donne */
TypeSettingsInstr SettingsEvts[SETTINGS_EVTS_MAX];
int SettingsEvtsNb;

// int SettingsEvtMemorise();

static Menu  mSettingsDetecEvts;
static Panel pSettingsDetecEvts;
static Panel pSettingsDetec;
// static Panel pTrmt[TRMT_NBCLASSES][DETEC_CAPT_MAX];
// static Panel pTrgr[DETEC_CAPT_MAX];
// static Panel pRegulParms[DETEC_CAPT_MAX],pRegulPlafs[DETEC_CAPT_MAX];

int GegeneAutorise(Instrum instrum, void *arg);


/* ................... formes d'evenements predefinies ...................... */
#pragma mark --- templates ---
/* ========================================================================== */
float TemplatesNorme(int voie) {
	int i; float denominateur;

	if(!VoieTampon[voie].unite.val) return(1.0);
	denominateur = 0.0;
	for(i=VoieTampon[voie].unite.depart; i<VoieTampon[voie].unite.arrivee; i++) 
		denominateur += (VoieTampon[voie].unite.val[i] * VoieTampon[voie].unite.val[i]);
	if(denominateur > 0.0) return(1.0 / denominateur); else return(1.0);
}
/* ========================================================================== */
int TemplatesMemorise() {
	int voie,i; float maxsup,maxinf,max; // ,unite

	if(!MonitChoisiBoloVoie(&voie)) return(0);
	if(VoieTampon[voie].somme.val && VoieTampon[voie].moyen.nb)  {
		if(OpiumExec(pTemplate->cdr) == PNL_CANCEL) return(0);
		if((TemplateValeur == 0.0) || (TemplateArrivee < TemplateDepart)) {
			OpiumFail(L_("Au moins une valeur est inadmissible"));
			return(0);
		}
		if(VoieTampon[voie].unite.val) free(VoieTampon[voie].unite.val);		
		VoieTampon[voie].unite.val = (float *)malloc(VoieTampon[voie].somme.taille * sizeof(float));
		if(VoieTampon[voie].unite.val) VoieTampon[voie].unite.taille = VoieTampon[voie].somme.taille;
		else VoieTampon[voie].unite.taille = 0;
		maxsup = maxinf = 0.0;
		for(i=0; i<VoieTampon[voie].somme.taille; i++) {
			if(VoieTampon[voie].somme.val[i] > maxsup) maxsup = VoieTampon[voie].somme.val[i];
			if(VoieTampon[voie].somme.val[i] < maxinf) maxinf = VoieTampon[voie].somme.val[i];
		}
		if(maxsup > -maxinf) max = maxsup; else max = -maxinf; if(max <= 0.0) max = 1.0;
		for(i=0; i<VoieTampon[voie].unite.taille; i++) 
			VoieTampon[voie].unite.val[i] = VoieTampon[voie].somme.val[i] / max;
		/*		switch(TemplateUniteNum) {
			case TRMT_UNITE_ADU: unite = 1.0; break;
			case TRMT_UNITE_NV : unite = VoieManip[voie].ADUennV; break;
			case TRMT_UNITE_KEV: unite = VoieManip[voie].ADUenkeV; break;
			default: unite = 1.0; break;
		} */
		strcpy(VoieTampon[voie].unite.nom,VoieManip[voie].nom);
		VoieTampon[voie].unite.horloge = VoieEvent[voie].horloge;
		VoieTampon[voie].unite.valeur = TemplateValeur;
		VoieTampon[voie].unite.amplitude = max;
		strncpy(VoieTampon[voie].unite.unite,TemplateUniteListe[TemplateUniteNum],UNITE_NOM);
		VoieTampon[voie].unite.pre_trigger = VoieEvent[voie].avant_evt;
		VoieTampon[voie].unite.depart = TemplateDepart;
		VoieTampon[voie].unite.arrivee = TemplateArrivee + 1;
	} else OpiumWarn(L_("L'evenement moyen %s n'a pas ete calcule"),VoieManip[voie].nom);
	return(0);
}
/* ========================================================================== */
int TemplatesAnalytique() {
	int voie; Panel p; int rc;
	
	if(!MonitChoisiBoloVoie(&voie)) return(0);
	p = PanelCreate(4);
	PanelFloat(p,L_("Temps de montee (ms)"),&(VoieManip[voie].def.evt.montee));
	PanelFloat(p,L_("1er temps de descente (ms)"),&(VoieManip[voie].def.evt.descente1));
	PanelFloat(p,L_("2eme temps de descente (ms)"),&(VoieManip[voie].def.evt.descente2));
	PanelFloat(p,L_("Gain 2eme descente (fact.2)"),&(VoieManip[voie].def.evt.facteur2));
	rc = OpiumExec(p->cdr);
	PanelDelete(p);
	if(rc == PNL_CANCEL) return(0);
	TemplatesCalcule(voie,0);
	return(0);
}
/* ========================================================================== */
int TemplatesFiltrer() {
	int voie;
	
	if(!MonitChoisiBoloVoie(&voie)) return(0);
	TemplatesFiltre(voie,0);
	return(0);
}
/* ========================================================================== */
int TemplatesAffiche() {
	int voie; int x,y; char titre[80];

	if(!MonitChoisiBoloVoie(&voie)) return(0);
	if(VoieTampon[voie].unite.val)  {
		MonitEvtAb6[voie][0] = 0.0;
		if(OpiumDisplayed(gEvtMoyen->cdr)) OpiumClear(gEvtMoyen->cdr); /* pour que le titre change ET que le nom disparaisse dans le menu Fenetres */
		GraphErase(gEvtMoyen);
		sprintf(titre,"Evenement unite, voie %s",VoieManip[voie].nom);
		OpiumTitle(gEvtMoyen->cdr,titre);
		x = GraphAdd(gEvtMoyen,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitEvtAb6[voie][0]),VoieTampon[voie].unite.taille);
		y = GraphAdd(gEvtMoyen,GRF_FLOAT|GRF_YAXIS,VoieTampon[voie].unite.val,VoieTampon[voie].unite.taille);
		GraphDataName(gEvtMoyen,x,"t(ms)");
//		GraphDataName(gEvtMoyen,y,VoieManip[voie].nom);
		GraphDataRGB(gEvtMoyen,y,GRF_RGB_GREEN);
		GraphUse(gEvtMoyen,VoieTampon[voie].unite.taille);
//		if(MonitChgtUnite) GraphAxisChgtUnite(gEvtMoyen,GRF_YAXIS,MonitUnitesADU);
		GraphAxisReset(gEvtMoyen,GRF_XAXIS);
		GraphAxisReset(gEvtMoyen,GRF_YAXIS);
		OpiumDisplay(gEvtMoyen->cdr);
	} else OpiumWarn(L_("L'evenement unite %s n'a pas ete memorise"),VoieManip[voie].nom);
	return(0);
}
/* ========================================================================== */
int TemplatesSauve() {
	int voie;

	if(!MonitChoisiBoloVoie(&voie)) return(0);
	if(VoieTampon[voie].unite.val) {
		memcpy(&EdwEvtInfo,&(VoieTampon[voie].unite),sizeof(TypeEvtTemplate));
		EdwDbPrint(DbazPath,VoieManip[voie].det,VoieManip[voie].type,&EdwEvt);
	} else OpiumWarn(L_("L'evenement unite %s n'a pas ete memorise"),VoieManip[voie].nom);
	return(0);
}
/* ========================================================================== */
int TemplatesLit() {
	int voie;

	if(!MonitChoisiBoloVoie(&voie)) return(0);
	SettingsDate = 0; // DateJourInt();
	if(OpiumExec(pSettingsDate->cdr) == PNL_CANCEL) return(0);
	TemplatesDecode(voie,1,0);
	return(0);
}
/* ========================================================================== */
static void TemplateLogHdr(char log) {
	if(log != 1) return;
	printf("\n");
	printf(" ____________________________________________________________________________________________ \n");
	printf("|                                    Filtrage par templates                                  |\n");
	printf("|____________________________________________________________________________________________|\n");
	printf("| Voie                | type                 |  Tmont  |  Tdesc1 |  Tdesc2 |  Fact2  | delta |\n");
	printf("|_____________________|______________________|_________|_________|_________|_________|_______|\n");
}
/* ========================================================================== */
void TemplateLogFin(char log) {
	if(!log) return;
	printf("|_____________________|______________________|_________|_________|_________|_________|_______|\n");
	printf("| Template analytique: A(t) = [1 - e(-t/Tm)] * [e(-t/T1) + F2 * e(-t/T2)]                    |\n");
	printf("|____________________________________________________________________________________________|\n\n");
}
/* ========================================================================== */
static void TemplateDimensionne(int voie) {
	if(((VoieManip[voie].def.trgr.type != NEANT) || VoieTampon[voie].post) 
	&& VoieTampon[voie].traitees->t.rval 
	&& VoieManip[voie].def.trmt[TRMT_PRETRG].template 
	&& VoieTampon[voie].unite.val) {
		VoieTampon[voie].delai_template = VoieManip[voie].def.evt.dimtemplate
			* VoieTampon[voie].trmt[TRMT_AU_VOL].compactage
			* VoieTampon[voie].trmt[TRMT_PRETRG].compactage;
	} else VoieTampon[voie].delai_template = 0;
}
/* ========================================================================== */
int TemplatesAbsent(int voie, char log) {
	TemplateDimensionne(voie);
	if(log) {
		TemplateLogHdr(log);
		printf("| %-19s | ---                  |         |         |         |         | %5d |\n",VoieManip[voie].nom,
			   (int)VoieTampon[voie].delai_template);
	}
	return(1);
}
/* ========================================================================== */
int TemplatesDecode(int voie, char embetant, char log) {
	int jour;
	
	EdwEvtInfo.val = 0; /* creation du tampon ainsi assuree par EdwDbScan */
	jour = EdwDbScan(DbazPath,SettingsDate,VoieManip[voie].det,VoieManip[voie].type,&EdwEvt,log);
	if(!jour) {
		if(embetant) OpiumWarn("%s pas trouve",EdwEvt.type);
		// else printf("  donc, %s pas recupere..\n",EdwEvt.type);
		return(0);
	}
	if(strcmp(EdwEvtInfo.nom,VoieManip[voie].nom)) {
		OpiumWarn("On a recupere un %s pour la voie %s",EdwEvt.type,EdwEvtInfo.nom);
		return(0);
	}
/*	{ int i;
		printf("Evenement moyen relu @%08X:",(hexa)EdwEvtInfo.val);
		for(i=0; i<EdwEvtInfo.taille; i++) {
			if(!(i % 10)) printf("\n");
			printf(" %8.1f",EdwEvtInfo.val[i]);
		}
		printf("\n");
	} */
	if(VoieTampon[voie].unite.val) {
		free(VoieTampon[voie].unite.val); /* et on libere bien la memoire precedemment utilisee */
		VoieTampon[voie].unite.val = 0;
		VoieTampon[voie].unite.taille = 0;
	}
	memcpy(&(VoieTampon[voie].unite),&EdwEvtInfo,sizeof(TypeEvtTemplate));
	TemplateDimensionne(voie);
	if(log) {
		TemplateLogHdr(log);
		printf("| %-19s | Database du %02d.%02d.%02d |         |         |         |         | %5d |\n",
			EdwEvtInfo.nom,jour%100,(jour/100)%100,jour/10000,(int)VoieTampon[voie].delai_template);
	} else printf("* Recuperation %s %s en date du %02d.%02d.%02d\n",EdwEvt.type,EdwEvtInfo.nom,jour%100,(jour/100)%100,jour/10000);

	// printf("  |%s|**2 = 1/%g = %g\n",VoieManip[voie].nom,denominateur,VoieTampon[voie].norme_evt);
	/*	{ int i;
		printf("Evenement moyen stocke @%08X:",(hexa)VoieTampon[voie].unite.val);
		for(i=0; i<VoieTampon[voie].unite.taille; i++) {
			if(!(i % 10)) printf("\n");
			printf(" %8.1f",VoieTampon[voie].unite.val[i]);
		}
		printf("\n");
	} */
	return(1);
}
/* ========================================================================== */
#ifdef RL
int TemplatesCalcule(int voie, char log) {
	int i; double temps; float limite,max;

	if(VoieTampon[voie].unite.val) { free(VoieTampon[voie].unite.val); VoieTampon[voie].unite.val = 0; }
	if(VoieManip[voie].def.evt.dimtemplate) VoieTampon[voie].unite.val = (float *)malloc(VoieManip[voie].def.evt.dimtemplate * sizeof(float));
	if(VoieTampon[voie].unite.val) VoieTampon[voie].unite.taille = VoieManip[voie].def.evt.dimtemplate;
	else VoieTampon[voie].unite.taille = 0;
//	printf("(%s) VoieTampon[%s].unite.val[%d] @%08X\n",__func__,VoieManip[voie].nom,VoieManip[voie].def.evt.dimtemplate,(hexa)VoieTampon[voie].unite.taille);

	strcpy(VoieTampon[voie].unite.nom,VoieManip[voie].nom);
	VoieTampon[voie].unite.horloge = VoieEvent[voie].horloge;
	VoieTampon[voie].unite.amplitude = 1.0;
	strncpy(VoieTampon[voie].unite.unite,TemplateUniteListe[TemplateUniteNum],UNITE_NOM);
	VoieTampon[voie].unite.pre_trigger = 0;
	VoieTampon[voie].unite.depart = 0;
	VoieTampon[voie].unite.arrivee = VoieManip[voie].def.evt.dimtemplate;

	limite = VoieTampon[voie].unite.horloge / 2.0;
	if(VoieManip[voie].def.evt.montee < limite)	   VoieManip[voie].def.evt.montee = limite;
	if(VoieManip[voie].def.evt.descente1 < limite) VoieManip[voie].def.evt.descente1 = limite;
	if(VoieManip[voie].def.evt.descente2 < limite) VoieManip[voie].def.evt.descente2 = limite;

	if(VoieTampon[voie].unite.val) {
		max = 0.0;
		/* [1 - e(-t/Tm)] * [e(-t/T1) + F2 * e(-t/T2)] */
		for(i=VoieTampon[voie].unite.depart; i<VoieTampon[voie].unite.arrivee; i++) {
			temps = (double)i * VoieTampon[voie].unite.horloge; /* ms */
			VoieTampon[voie].unite.val[i] = (1.0 - exp(-temps / (double)VoieManip[voie].def.evt.montee)) * (exp(-temps / (double)VoieManip[voie].def.evt.descente1) + (VoieManip[voie].def.evt.facteur2 * exp(-temps / (double)VoieManip[voie].def.evt.descente2)));		
			if(fabs(VoieTampon[voie].unite.val[i]) > max) max = fabs(VoieTampon[voie].unite.val[i]);
		}
		for(i=0; i<VoieTampon[voie].unite.taille; i++) 
			VoieTampon[voie].unite.val[i] = VoieTampon[voie].unite.val[i] / max;
	}

	TemplateDimensionne(voie);
	if(log) {
		TemplateLogHdr(log);
		printf("| %-19s | Analytique %-9s | %7.3f | %7.2f | %7.2f | %7.3f | %5d |\n",
			VoieManip[voie].nom,(VoieTampon[voie].unite.val)? " ": ", HS",
			VoieManip[voie].def.evt.montee,VoieManip[voie].def.evt.descente1,VoieManip[voie].def.evt.descente2,VoieManip[voie].def.evt.facteur2,
			(int)VoieTampon[voie].delai_template);
	// } else {
	//	printf("* Calcul du template analytique A(t) = [1 - e(-t/Tm)] * [e(-t/T1) + F2 * e(-t/T2)]\n");
	//	printf("  pour la voie %s avec Tm =%7.3f ms, T1 =%7.3f ms, T2 =%7.3f ms, F2 =%g\n",VoieManip[voie].nom,
	//		   VoieManip[voie].def.evt.montee,VoieManip[voie].def.evt.descente1,VoieManip[voie].def.evt.descente2,VoieManip[voie].def.evt.facteur2);
	}

	return(0);
}
/* ========================================================================== */
#endif
void TemplatesFiltre(int voie, char log) {
	TypeFiltre *filtre; TypeSuiviFiltre *suivi; TypeCelluleFiltrante *cellule;
	int n,i,j,l,m; int etage; TypeDonneeFiltree brute,entree,filtree;
	int nbdata; 
		
	if(log) {
		if(log == 1) {
			printf("\n");
			printf(" ____________________________________________________________________________________ \n");
			printf("|                              Modification des templates                            |\n");
			printf("|____________________________________________________________________________________|\n");
			printf("| Voie                     | Action                                                  |\n");
			printf("|__________________________|_________________________________________________________|\n");
		}
		l = printf("| %-24s |",VoieManip[voie].nom);
		if(VoieTampon[voie].trmt[TRMT_PRETRG].filtre) {
			l += printf("filtrage demande");
			SambaCompleteLigne(86,l);
		} else {
			l += printf("filtre non defini, template inchange");
			SambaCompleteLigne(86,l);
			return;
		}
	} else {
		if(VoieTampon[voie].trmt[TRMT_PRETRG].filtre) 
			printf("* Filtrage du template de la voie %s\n",VoieManip[voie].nom);
		else {
			printf("! Le filtre de la voie %s n'est pas defini: template inchange\n",VoieManip[voie].nom);
			return;
		}
	}
	filtre = VoieTampon[voie].trmt[TRMT_PRETRG].filtre;
	suivi = &(VoieTampon[voie].trmt[TRMT_PRETRG].suivi);
	suivi->nb = suivi->max;
	suivi->prems = 0;
	for (i=0; i<suivi->max; i++) {
		suivi->brute[i] = 0.0;
		for(etage=0; etage<filtre->nbetg; etage++) suivi->filtree[etage][i] = 0.0;
	}
	
	for(j=0; j<VoieTampon[voie].unite.taille; j++) {			
		nbdata = suivi->nb;
		if(nbdata < suivi->max) n = nbdata; else n =  suivi->prems;
		entree = (TypeDonneeFiltree)VoieTampon[voie].unite.val[j];
		brute = entree;
		filtree = entree;																		
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
				};
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
		VoieTampon[voie].unite.val[j] = (float)filtree;
		if(nbdata < suivi->max) suivi->nb += 1;
		else {
			suivi->prems += 1;
			if(suivi->prems >= suivi->max) suivi->prems = 0;
		}
	}
}

/* .................. formes de perturbation periodique ..................... */
#pragma mark --- patterns ---
/* ========================================================================== */
int PatternMemorise() {
	int voie,taille; // float *ref,*val;
	
	if(!MonitChoisiBoloVoie(&voie)) return(0);
	if(VoieTampon[voie].pattern.val)  {
		if(VoieTampon[voie].pattern.max != VoieTampon[voie].pattref.taille) {
			if(VoieTampon[voie].pattref.val) free(VoieTampon[voie].pattref.val);
			VoieTampon[voie].pattref.val = 0;
			VoieTampon[voie].pattref.taille = 0;
		}
		taille = VoieTampon[voie].pattern.max * sizeof(float);
		if(!VoieTampon[voie].pattref.val) {
			VoieTampon[voie].pattref.val = (float *)malloc(taille);	
			if(!VoieTampon[voie].pattref.val) {
				VoieTampon[voie].pattref.taille = 0;
				OpiumFail("Manque %d octets pour memoriser le pattern de la voie %s",taille,VoieManip[voie].nom);
				return(0);
			}
		}
		strcpy(VoieTampon[voie].pattref.nom,VoieManip[voie].nom);
		VoieTampon[voie].pattref.horloge = VoieEvent[voie].horloge;
		VoieTampon[voie].pattref.taille = VoieTampon[voie].pattern.max;
		memcpy(VoieTampon[voie].pattref.val,VoieTampon[voie].pattern.val,taille);
		/* int i;			
		 val = VoieTampon[voie].pattern.val;
		 ref = VoieTampon[voie].pattref.val;
		 for (i=0; i<VoieTampon[voie].pattern.max; i++)
		 *ref++ = *val++ / VoieManip[voie].duree.periodes; */
	} else OpiumFail("Le pattern de la voie %s n'a pas ete calcule",VoieManip[voie].nom);
	return(0);
}
/* ========================================================================== */
int PatternAffiche() {
	int voie; int x,y; char titre[80];
	
	if(!MonitChoisiBoloVoie(&voie)) return(0);
	if(VoieTampon[voie].pattref.val)  {
		if(OpiumDisplayed(gPattern->cdr)) OpiumClear(gPattern->cdr); /* pour que le titre change ET que le nom disparaisse dans le menu Fenetres */
		GraphErase(gPattern);
		sprintf(titre,"Pattern voie %s",VoieManip[voie].nom);
		OpiumTitle(gPattern->cdr,titre);
		x = GraphAdd(gPattern,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitEvtAb6[voie][0]),VoieTampon[voie].pattref.taille);
		y = GraphAdd(gPattern,GRF_FLOAT|GRF_YAXIS,VoieTampon[voie].pattref.val,VoieTampon[voie].pattref.taille);
		GraphDataName(gPattern,x,"t(ms)");
		//		GraphDataName(gPattern,y,VoieManip[voie].nom);
		GraphAxisAutoRange(gPattern,GRF_YAXIS);
		GraphDataRGB(gPattern,y,GRF_RGB_GREEN);
		GraphUse(gPattern,VoieTampon[voie].pattref.taille);
		//		if(MonitChgtUnite) GraphAxisChgtUnite(gPattern,GRF_YAXIS,MonitUnitesADU);
		GraphAxisReset(gPattern,GRF_XAXIS);
		GraphAxisReset(gPattern,GRF_YAXIS);
		OpiumDisplay(gPattern->cdr);
	} else OpiumFail("Le pattern de la voie %s n'a pas ete memorise",VoieManip[voie].nom);
	return(0);
}
/* ========================================================================== */
int PatternSauve() {
	int voie;
	
	if(!MonitChoisiBoloVoie(&voie)) return(0);
	if(VoieTampon[voie].pattref.val)  {
		memcpy(&EdwPatternInfo,&(VoieTampon[voie].pattref),sizeof(TypePattern));
		EdwDbPrint(DbazPath,VoieManip[voie].det,VoieManip[voie].type,&EdwPattern);
	} else OpiumFail("Le pattern de la voie %s n'a pas ete memorise",VoieManip[voie].nom);
	return(0);
}
/* ========================================================================== */
int PatternLit() {
	int voie,jour;
	
	if(!MonitChoisiBoloVoie(&voie)) return(0);
	SettingsDate = DateJourInt();
	if(OpiumExec(pSettingsDate->cdr) == PNL_CANCEL) return(0);
	EdwPatternInfo.val = 0; /* creation du tampon ainsi assuree par EdwDbScan */
	jour = EdwDbScan(DbazPath,SettingsDate,VoieManip[voie].det,VoieManip[voie].type,&EdwPattern,1);
	if(!jour) { OpiumFail("%s pas trouve",EdwPattern.type); return(0); }
	if(strcmp(EdwPatternInfo.nom,VoieManip[voie].nom)) {
		OpiumNote("On a recupere un %s pour la voie %s",EdwPattern.type,EdwPatternInfo.nom);
		return(0);
	}
	printf("* Recuperation %s %s en date de %06d\n",EdwPattern.type,EdwPatternInfo.nom,jour);
	/*	{ int i;
	 printf("Pattern relu @%08X:",(hexa)EdwPatternInfo.val);
	 for(i=0; i<EdwPatternInfo.taille; i++) {
	 if(!(i % 10)) printf("\n");
	 printf(" %8.1f",EdwPatternInfo.val[i]);
	 }
	 printf("\n");
	 } */
	if(VoieTampon[voie].pattref.val) {
		/* et on libere bien la memoire precedemment utilisee */
		free(VoieTampon[voie].pattref.val);
		VoieTampon[voie].pattref.val = 0;
		VoieTampon[voie].pattref.taille = 0;
	}
	memcpy(&(VoieTampon[voie].pattref),&EdwPatternInfo,sizeof(TypePattern));
	return(0);
}

/* ............................. courbes V(I) ............................... */
#pragma mark --- V(i) ---
/* ========================================================================== */
int ViVisu() {
	GraphUse(gGestVi,ReglTabNb);
	OpiumDisplay(gGestVi->cdr);
	return(0);
}
/* ========================================================================== */
int ViRetire() {
    Info info;
    int ix,iy; float Ir,Vr; float dx,dy,dist,min; int proche;
    int n,m;

	info = InfoCreate(1,133);
    InfoWrite(info,1,"Pointes le point a retirer");
    OpiumDisplay(info->cdr);
    GraphGetPoint(gGestVi,&ix,&Ir,&iy,&Vr);
    min = 99999999.0; proche = ReglTabNb;
    for(n=0; n<ReglTabNb; n++) {
        dx = Ir - ReglTabI[n]; dy = Vr - ReglTabV[n];
        dist = (dx * dx) + (dy * dy);
        if(dist < min) { min = dist; proche = n; }
    }
    OpiumClear(info->cdr);
	InfoDelete(info);
    n = proche;
    if(n < ReglTabNb) {
        if(OpiumChoix(2,SambaNonOui,"Retirer le point #%d (%g, %g)?",n+1,ReglTabI[n],ReglTabV[n])) {
            ReglTabNb -= 1;
            for(m=n; m<ReglTabNb; m++) {
                ReglTabI[m] = ReglTabI[m + 1];
                ReglTabV[m] = ReglTabV[m + 1];
                ReglTabR[m] = ReglTabR[m + 1];
                ReglTabT[m] = ReglTabT[m + 1];
            }
            GraphUse(gGestVi,ReglTabNb);
            OpiumDisplay(gGestVi->cdr);
        } else OpiumError("Courbe V(I) courante inchangee");
    } else OpiumError("Pas de point proche de (%g, %g)",Ir,Vr);
    return(0);
}
/* ========================================================================== */
int ViRaz() {
    ReglTabNb = 0;
	GraphUse(gGestVi,ReglTabNb);
    if(OpiumDisplayed(gGestVi->cdr)) OpiumRefresh(gGestVi->cdr);
    return(0);
}
/* ========================================================================== */
int ViMemorise() {
    int n;
    TypeCourbeVi *courbe,*courante,*suivante,*precedente;

	if(ReglTabNb <= 0) {
		OpiumError("Pas de point enregistre dans la courbe courante");
		return(0);
	}
    courbe = (TypeCourbeVi *)malloc(sizeof(TypeCourbeVi));
    if(!courbe) { OpiumError("Pas assez de memoire pour cela"); return(0);  }
    courbe->I = (float *)malloc(ReglTabNb * sizeof(float));
    if(!(courbe->I)) { OpiumError("Pas assez de memoire pour cela"); return(0); }
    courbe->V = (float *)malloc(ReglTabNb * sizeof(float));
    if(!(courbe->V)) { OpiumError("Pas assez de memoire pour cela"); return(0); }

    strcpy(courbe->nom,ReglTabNom);
    courbe->tempe = ReglTabT[0];
    courbe->date = DateJourInt();
    courbe->heure = DateHeureInt();
    courbe->nb = ReglTabNb;
    for(n=0; n<ReglTabNb; n++) { courbe->I[n] = ReglTabI[n]; courbe->V[n] = ReglTabV[n]; }

	precedente = 0; // GCC
    courante = CourbeVi; suivante = 0;
    while(courante) {
        suivante = courante->suivante;
        if(courbe->tempe < courante->tempe) break;
        precedente = courante;
        courante = suivante;
    }
    if(!courante) {
        courbe->suivante = 0;
        if(CourbeVi) precedente->suivante = courbe;
        else CourbeVi = courbe;
    } else {
        precedente->suivante = courbe;
        courbe->suivante = courante;
    }
	return(0);
}
/* ========================================================================== */
int ViEcrit() {
#ifdef ANCIEN_STYLE
	FILE *fichier;
    int n;
    TypeCourbeVi *courbe;

    if(!CourbeVi) {
        OpiumError("Pas de courbe V(I) memorisee");
        return(0);
    }
	if(OpiumReadFile("Sur quel fichier",FichierPrefVi,MAXFILENAME) == PNL_CANCEL) return(0);
	if(!(fichier = fopen(FichierPrefVi,"a"))) {
		OpiumError("Sauvegarde des courbes V(I) impossible (%s)",strerror(errno));
		return(0);
	}
    courbe = CourbeVi;
    while(courbe) {
        fprintf(fichier,"#\n=%s @%g /%06d:%06d [%d]\n",courbe->nom,
            courbe->tempe,courbe->date,courbe->heure,courbe->nb);
        for(n=0; n<courbe->nb; n++) fprintf(fichier,"\t%-11.7g\t%-11.7g\n",courbe->I[n],courbe->V[n]);
        courbe = courbe->suivante;
    }
	fclose(fichier);
#endif
    TypeCourbeVi *courbe,*courante;
	float ecart,mini;

	if(OpiumExec(pViChoix->cdr) == PNL_CANCEL) return(0);
    courante = CourbeVi; courbe = 0; mini = 1000.0;
    while(courante) {
        if(!strcmp(courante->nom,Bolo[BoloNum].nom)) {
			ecart = courante->tempe - ReglTbolo;
			if(ecart < 0.0) ecart = -ecart;
			if(ecart < mini) {
				mini = ecart;
				courbe = courante;
			}
		}
        courante = courante->suivante;
    }
	if(!courbe) {
		OpiumError("Pas trouve de V(I) pour %s",Bolo[BoloNum].nom);
	} else if(OpiumChoix(2,SambaNonOui,"Archivage de V(I) @%gK du %06d:%06d",courante->tempe,courbe->date,courbe->heure)) {
		memcpy(&EdwViInfo,courbe,sizeof(TypeCourbeVi));
		EdwDbPrint(DbazPath,BoloNum,-1,&EdwVi);
	}
	return(0);
}
/* ========================================================================== */
int ViLit() {
#ifdef ANCIEN_STYLE
	char ligne[256],*r,*item;
	FILE *fichier;
    TypeCourbeVi *courbe,*courante,*suivante;
    int n;
    
	if(!(fichier = fopen(FichierPrefVi,"r"))) {
		OpiumError("Lecture des courbes V(I) impossible (%s)",strerror(errno));
		return(0);
	}
    
    courbe = CourbeVi;
    while(courbe) {
        suivante = courbe->suivante;
        free(courbe->I); free(courbe->V);
        free(courbe);
        courbe = suivante;
    }
    CourbeVi = 0;
    
    courbe = 0;
	while(LigneSuivante(ligne,256,fichier)) {
		r = ligne;
		if((*r == '#') || (*r == '\n')) continue;
		if(*r == '=') {
            r++;
            courbe = (TypeCourbeVi *)malloc(sizeof(TypeCourbeVi));
            if(!courbe) { OpiumError("Manque de memoire pour V(I)"); return(0);  }
            item = ItemSuivant(&r,'@'); strncpy(courbe->nom,item,DETEC_NOM);
            item = ItemSuivant(&r,'/'); sscanf(item,"%g",&(courbe->tempe));
            item = ItemSuivant(&r,':'); sscanf(item,"%d",&(courbe->date));
            item = ItemSuivant(&r,'['); sscanf(item,"%d",&(courbe->heure));
            item = ItemSuivant(&r,']'); sscanf(item,"%d",&(courbe->nb));
            courbe->I = (float *)malloc(courbe->nb * sizeof(float));
            if(!(courbe->I)) { OpiumError("Manque de memoire pour V(I)"); return(0); }
            courbe->V = (float *)malloc(courbe->nb * sizeof(float));
            if(!(courbe->V)) { OpiumError("Manque de memoire pour V(I)"); return(0); }
            n = 0;
            courbe->suivante = 0;
            if(!CourbeVi) CourbeVi = courbe;
            else courante->suivante = courbe;
            courante = courbe;
		} else {
            if(!courbe) {
                OpiumError("Points d'une courbe V(I) sans entete");
                continue;
            }
            if(n < courbe->nb) {
                sscanf(ligne,"%g %g",&(courbe->I[n]),&(courbe->V[n]));
                n++;
            } else OpiumError("Points de la courbe V(I) %s(%06d) en excedent",courbe->nom,courbe->date);
		}
	}
    
	fclose(fichier);
#endif
	int jour;
    TypeCourbeVi *courbe,*courante,*suivante,*precedente;
	
	if(OpiumExec(pViChoix->cdr) == PNL_CANCEL) return(0);
	SettingsDate = DateJourInt();
	if(OpiumExec(pSettingsDate->cdr) == PNL_CANCEL) return(0);
	EdwViInfo.I = 0; /* creation du tampon ainsi assuree par EdwDbScan */
	EdwViInfo.V = 0; /* creation du tampon ainsi assuree par EdwDbScan */
	jour = EdwDbScan(DbazPath,SettingsDate,BoloNum,-1,&EdwVi,1);
	if(!jour) { OpiumError("%s pas trouve",EdwVi.type); return(0); }
	if(strcmp(EdwViInfo.nom,Bolo[BoloNum].nom)) {
		OpiumError("On a recupere un %s pour le bolo %s",EdwVi.type,EdwViInfo.nom);
		return(0);
	}
	printf("* Recuperation %s %s en date de %06d\n",EdwVi.type,EdwViInfo.nom,jour);
    courbe = (TypeCourbeVi *)malloc(sizeof(TypeCourbeVi));
    if(!courbe) { OpiumError("Pas assez de memoire pour cela"); return(0);  }
	memcpy(courbe,&EdwViInfo,sizeof(TypeCourbeVi));
	precedente = 0; // GCC
    courante = CourbeVi; suivante = 0;
    while(courante) {
        suivante = courante->suivante;
        if(courbe->tempe < courante->tempe) break;
        precedente = courante;
        courante = suivante;
    }
    if(!courante) {
        courbe->suivante = 0;
        if(CourbeVi) precedente->suivante = courbe;
        else CourbeVi = courbe;
    } else {
        precedente->suivante = courbe;
        courbe->suivante = courante;
    }
	
	return(0);
}
/* ========================================================================== */
int ViExit() { if(OpiumDisplayed(gGestVi->cdr)) OpiumClear(gGestVi->cdr); return(0); }

/* ....................... Tables et Impressions PDF ........................ */
#pragma mark --- Impressions PDF ---
/* ========================================================================== */
static int SambaMontreSetup(Menu menu, MenuItem item) {
	char base[MAXFILENAME],commande[MAXFILENAME];

	if(OpiumExec(pFolderActn->cdr) == PNL_CANCEL) return(0);
	if(FolderRef[FolderNum].nom) {
		if(FolderActn == 1) {
			RepertoireExtrait(FolderRef[FolderNum].nom,base,0);
			strcat(strcpy(commande,"open "),base);
		} else strcat(strcpy(commande,"open "),FolderRef[FolderNum].nom);
		system(commande);
	}
	return(0);
}
/* ========================================================================== */
void SambaImprimeGeneral() {
	int voie,l,virg;
#ifdef MODULES_RESEAU
	int sat;
#endif
	
	printf(" ____________________________________________________________________________________\n");
	printf("| Configuration | %-66s |\n",SambaIntitule);
	printf("|_______________|____________________________________________________________________|\n");
#ifdef MODULES_RESEAU
	for(sat=0; sat<AcquisNb; sat++) {
		if(sat == 0) l = printf("| Reseau        | "); else l = printf("|               | ");
		l += printf("%6s: %-32s (%s)",Acquis[sat].code,Acquis[sat].adrs,AcquisTexte[(int)Acquis[sat].etat.status]);
		SambaCompleteLigne(86,l);
	}
	printf("|_______________|____________________________________________________________________|\n");
#endif
	printf("\n");
	if(CompteRendu.config.elec) {
		printf(" ____________________________________________________________________________________\n");
		printf("|                           Definition de l'electronique                             |\n");
		printf("|____________________________________________________________________________________|\n");
		//	printf("| Identification | version %3.1f",VersionIdentification); SambaCompleteLigne(86,l);
		l = printf("| Echantillonnage | %g kHz (%g us)",Echantillonnage,1000.0/Echantillonnage); SambaCompleteLigne(86,l);
		l = printf("| Synchro D2      | %d echantillons (%g kHz)",Diviseur2,Echantillonnage/(float)Diviseur2); SambaCompleteLigne(86,l);
		l = printf("| Detecteurs      | %-3d gere%s",Accord1s(BoloGeres)); SambaCompleteLigne(86,l);
		if(PasMaitre) { l = printf("|                 | %-3d connu%s",Accord1s(BoloNb)); SambaCompleteLigne(86,l); }
		//	printf("|                | %-3d voie(s) transmise(s) par detecteur                            |\n",
		//		   ModeleVoieNb);
		l = printf("| Types de voies  |");
		virg = 0;
		for(voie = 0; voie<ModeleVoieNb; voie++) {
			if((l + 3 + strlen(ModeleVoie[voie].nom)) >= 85) {
				l += printf(","); SambaCompleteLigne(86,l);
				l = printf("|                 |"); virg = 0;
			}
			l += printf("%s%s",virg?", ":" ",ModeleVoie[voie].nom);
			virg = 1;
		}
		SambaCompleteLigne(86,l);
		l = printf("| Etat sur voie   | %-16s",VoieStatus); SambaCompleteLigne(86,l);
		l = printf("| Total voies     | %-3d geree%s",VoiesGerees,(VoiesGerees>1)?"s":" "); SambaCompleteLigne(86,l);
		if(PasMaitre) { l = printf("|                 | %-3d connue%s",VoiesNb,(VoiesNb>1)?"s":" "); SambaCompleteLigne(86,l); }
		l = printf("| FIFO repart.    | %-7d valeurs",FIFOdim); SambaCompleteLigne(86,l);
		printf("|________________|___________________________________________________________________|\n\n");
	}
	
}
/* ========================================================================== */
int SettingsImprimeBolos(Menu menu, MenuItem item) { DetecteursImprime(1); return(0); }
/* ========================================================================== */
int SambaImprimeFiltres(Menu menu, MenuItem item) {
	int i,j,l,etage;
	
    printf(" ____________________________________________________________________________________\n");
    printf("|                                  Liste des filtres                                 |\n");
    printf("|____________________________________________________________________________________|\n");
	if(!FiltreNb) {
		printf("|                                       neant                                        |\n");
		printf("|____________________________________________________________________________________|\n");
	} else for(i=0; i<FiltreNb; i++) {
		l = printf("| %-20s |",FiltreGeneral[i].nom);
		if(FiltreGeneral[i].commentaire[0]) {
			printf(" %-59s |\n",FiltreGeneral[i].commentaire);
			l = printf("|                      |");
		}
		if(FiltreGeneral[i].modele >= 0) {
			l += printf(" %-12s %-16s *%-2d ",
				FiltreModeles[(int)FiltreGeneral[i].modele],FiltreTypes[(int)FiltreGeneral[i].type],
				FiltreGeneral[i].degre);
			switch(FiltreGeneral[i].type) {
			  case FILTRE_PBAS: case FILTRE_PHAUT:
				l += printf("@ %5.4f%-3s           ",FiltreGeneral[i].omega1,FiltreUnites[FiltreGeneral[i].unite1]);
				break;
			  case FILTRE_PASSEB: case FILTRE_COUPEB:
				l += printf("@[%5.4f%-3s, %5.4f%-3s]",FiltreGeneral[i].omega1,FiltreUnites[FiltreGeneral[i].unite1],
					FiltreGeneral[i].omega2,FiltreUnites[FiltreGeneral[i].unite2]);
				break;
			default: l += printf("                     ");
			}
			SambaCompleteLigne(86,l);
		} else {
			for(etage=0; etage<FiltreGeneral[i].coef.nbetg; etage++) {
				/* if(etage) l = printf("|                      |"); */
				l += printf(" %cdirects=",etage?'+':' ');
				for(j=0; j<FiltreGeneral[i].coef.etage[etage].nbdir; j++) {
					if(j && !(j % 3)) {
						SambaCompleteLigne(86,l);
						l = printf("|                      |");
					}
					l += printf(" %14g",FiltreGeneral[i].coef.etage[etage].direct[j]);
				}
				SambaCompleteLigne(86,l);
				l = printf("|                      | inverses=");
				for(j=0; j<FiltreGeneral[i].coef.etage[etage].nbinv; j++) {
					if(j && !(j % 3)) {
						SambaCompleteLigne(86,l);
						l = printf("|                      |");
					}
					l += printf(" %14g",FiltreGeneral[i].coef.etage[etage].inverse[j]);
				}
				SambaCompleteLigne(86,l);
			}
		}
	}
	printf("|______________________|_____________________________________________________________|\n\n");
	return(0);
}
/* ========================================================================== */
int SettingsDocImprimeCablage(Menu menu, MenuItem item) {
	int bolo; char premier;
	int trait_nom,trait_pos,trait_cap,trait_rgl,trait_fis,trait_num,trait_res; // int trait_adr;
	int col_nom,col_pos,col_cap,col_rgl,col_fis,col_num,col_res; // int col_adr;
	int trait_dernier;
	
	ImprimeSautPage();

	trait_dernier = 0;
	ImpressionPlaceColonne(&trait_nom,16,&col_nom,&trait_dernier);
	ImpressionPlaceColonne(&trait_pos,4,&col_pos,&trait_dernier);
	ImpressionPlaceColonne(&trait_cap,8,&col_cap,&trait_dernier);
	ImpressionPlaceColonne(&trait_rgl,12,&col_rgl,&trait_dernier);
	ImpressionPlaceColonne(&trait_fis,3,&col_fis,&trait_dernier);
	ImpressionPlaceColonne(&trait_num,12,&col_num,&trait_dernier);
	// ImpressionPlaceColonne(&trait_adr,3,&col_adr,&trait_dernier);
	ImpressionPlaceColonne(&trait_res,16,&col_res,&trait_dernier);
	
	ImprimeLimite(0,trait_dernier,0);
	premier = 1;
	for(bolo=0; bolo<BoloNb; bolo++) /* if(Bolo[bolo].local) ou si "tous" */ {
		int bb,cap,vbb,vm,voie; int i,prec_num,prec_conn;
		char nomBN[NUMER_NOM];
		TypeDetecteur *detectr; TypeReglage *regl;
		TypeDetModele *det_modele; TypeBNModlRessource *ress;
		shex connecteur;
		
		if(premier) {
			ImprimeColonnes(2,0,trait_dernier);
			ImprimeEnCol((trait_dernier - 22) / 2,"Cablage des detecteurs");
			ImprimeLimite(0,trait_dernier,1);
			ImprimeNouvelleLigne(1);
			ImprimeColonnes(3,0,trait_fis,trait_dernier);
			ImprimeEnCol((trait_fis-9)/2,"detecteur");
			ImprimeEnCol((trait_dernier-trait_fis-10)/2+col_fis,"numeriseur");
			ImprimeLimite(0,trait_dernier,1);
			ImprimeNouvelleLigne(1);
			// ImprimeColonnes(9,0,trait_pos,trait_cap,trait_rgl,trait_fis,trait_num,trait_adr,trait_res,trait_dernier);
			ImprimeColonnes(8,0,trait_pos,trait_cap,trait_rgl,trait_fis,trait_num,trait_res,trait_dernier);
			ImprimeEnCol(col_nom," nom");
			ImprimeEnCol(col_pos,"pos.");
			ImprimeEnCol(col_cap,"capteur");
			ImprimeEnCol(col_rgl,"reglage");
			ImprimeEnCol(col_fis,"pos.");
			ImprimeEnCol(col_num,"nom");
			// ImprimeEnCol(col_adr,"adrs");
			ImprimeEnCol(col_res," ressource");
			ImprimeLimite(0,trait_dernier,1);
		}
		
		prec_num = -2; prec_conn = -2;
		detectr = &(Bolo[bolo]);
		det_modele = &(ModeleDet[detectr->type]);
		for(cap=0; cap<detectr->nbcapt; cap++) {
			vm = detectr->captr[cap].modele;
			bb = detectr->captr[cap].bb.num;
			if((bb != prec_num) && (prec_num != -2)) {
				ImprimeLimite(trait_fis,trait_dernier-trait_fis,1);
			}
			ImprimeNouvelleLigne(1);
			if(bb >= 0) { connecteur = Numeriseur[bb].fischer; vbb = detectr->captr[cap].bb.adc - 1; }
			else if(detectr->pos != CABLAGE_POS_NOTDEF) {
				connecteur = CablageSortant[detectr->pos].captr[cap].connecteur;
				vbb = CablageSortant[detectr->pos].captr[cap].vbb;
			} else { connecteur = 0; vbb = -1; prec_conn = -2; }
			// if((bb == prec_num) && (connecteur == prec_conn)) continue;
			if(cap == 0) {
				ImprimeEnCol(col_nom,"%c%-12s",detectr->lu?' ':'!',detectr->nom);
				ImprimeEnCol(col_pos,detectr->adresse);
			}
			if((voie = detectr->captr[cap].voie) >= 0) {
				if(VoieManip[voie].pseudo) {
					TypeComposantePseudo *composant; float coef;
					char def[256]; int k;
					ImprimeEnCol(col_cap,detectr->captr[cap].nom);
					ImprimeStyle(IMPRESSION_ITALIQUE); ImprimeEnCol(col_rgl,"(composee)"); ImprimeStyle(IMPRESSION_REGULIER);
					composant = VoieManip[voie].pseudo; k = 0;
					do {
						coef = composant->coef;
						if(composant != VoieManip[voie].pseudo) {
							if(coef >= 0.0) k += sprintf(def+k," + ");
							else { k += sprintf(def+k," - "); coef = -coef; }
						}
						k += sprintf(def+k,"%g x %s",coef,VoieManip[composant->srce].nom);
						composant = composant->svt;
					} while(composant);
					def[k] = '\0';
					ImprimeEnCol(col_fis,def);
					continue;
				}
			}
			ImprimeEnCol(col_cap,ModeleVoie[vm].nom);
			ImprimeStyle(IMPRESSION_ITALIQUE); ImprimeEnCol(col_rgl,"(mesure)"); ImprimeStyle(IMPRESSION_REGULIER);
			if(connecteur != prec_conn) {
				if(connecteur) ImprimeEnCol(col_fis,CablageEncodeConnecteur(connecteur)); else ImprimeEnCol(col_fis," --");
				prec_conn = connecteur;
			}
			if(bb != prec_num) {
				if(bb >= 0) strcpy(nomBN,Numeriseur[bb].nom); else strcpy(nomBN,"inconnu");
				ImprimeEnCol(col_num,nomBN);
				prec_num = bb;
				// if(detectr->captr[cap].bb.adrs) ImprimeEnCol(col_adr,"%02X",detectr->captr[cap].bb.adrs); else ImprimeEnCol(col_adr," --");
			}
			if(vbb >= 0) { ImprimeStyle(IMPRESSION_ITALIQUE);  ImprimeEnCol(col_res," ADC%d",vbb+1); ImprimeStyle(IMPRESSION_REGULIER); }
			forever {
				int n;
				for(i=0; i<detectr->nbreglages; i++) {
					regl = &(detectr->reglage[i]);
					if(regl->capt != cap) continue;
					ImprimeNouvelleLigne(1);
					if(cap < 0) ImprimeEnCol(col_cap,"(general)");
					ImprimeEnCol(col_rgl,det_modele->regl[i].nom);
					bb = regl->bb;
					if(bb != prec_num) {
						if(bb >= 0) strcpy(nomBN,Numeriseur[bb].nom); else strcpy(nomBN,"inconnu");
						ImprimeEnCol(col_num,nomBN);
						prec_num = bb;
					}
					n = 0;
					if(regl->ress >= 0) {
						ress = &(ModeleBN[Numeriseur[bb].def.type].ress[regl->ress]);
						ImprimeEnCol(col_res," %-13s",ress->nom); n++;
					}
					if(regl->ressneg >= 0) {
						ress = &(ModeleBN[Numeriseur[bb].def.type].ress[regl->ressneg]);
						if(n) ImprimeNouvelleLigne(1);
						ImprimeEnCol(col_res,"-%-13s",ress->nom); n++;
					}
					if(regl->resspos >= 0) {
						ress = &(ModeleBN[Numeriseur[bb].def.type].ress[regl->resspos]);
						if(n) ImprimeNouvelleLigne(1);
						ImprimeEnCol(col_res,"+%-13s",ress->nom); n++;
					}
					if(!n) ImprimeEnCol(col_res," ...");
				}
				if(cap == (detectr->nbcapt - 1)) cap = -1;
				else { if(cap == -1) cap = detectr->nbcapt; break; }
			}
		}
		ImprimeLimite(0,trait_dernier,1);
		premier = 0;
	}
	if(premier) {
		ImprimeColonnes(2,0,trait_dernier);
		ImprimeEnCol(3,"! PAS DE DETECTEUR LU PAR CET ORDINATEUR");
		ImprimeFond(5,38,0xFFFF,0x0000,0x0000);
	}
	ImprimeColonnes(0);
	ImprimeNouvelleLigne(1);
	return(0);
}
/* ========================================================================== */
int SettingsDocImprimeModeles(Menu menu, MenuItem item) {
	int i,cap,k; char autre;
	
	ImprimeSautPage();
	ImprimeTableauLargeurs(0,4,20,20,20,15);
	ImprimeTableauChapeau("Modeles de detecteurs");
	ImprimeTableauCol(0,"nom");
	ImprimeTableauCol(1,"capteur");
	ImprimeTableauCol(2,"reglage");
	ImprimeTableauCol(3,"type");
	ImprimeTableauLimite(0,3);
	for(i=0; i<ModeleDetNb; i++) {
		if(i) ImprimeTableauLimite(0,3); // ImprimeTableauTrait(0,3,1);
		ImprimeTableauCol(0,ModeleDet[i].nom);
		for(cap=0; cap<ModeleDet[i].nbcapt; cap++) {
			if(cap) { ImprimeNouvelleLigne(1); ImprimeTableauTrait(1,3,1); }
			ImprimeTableauCol(1,ModeleVoie[ModeleDet[i].type[cap]].nom);
			autre = 0;
			for(k=0; k<ModeleDet[i].nbregl; k++) if(ModeleDet[i].regl[k].capteur == ModeleDet[i].type[cap]) {
				if(autre) ImprimeNouvelleLigne(1);
				ImprimeTableauCol(2,ModeleDet[i].regl[k].nom);
				ImprimeTableauCol(3,ReglCmdeListe[ModeleDet[i].regl[k].cmde]);
				autre = 1;
			}
			if(!autre) ImprimeTableauCol(2,"--");
		}
	}
	ImprimeTableauFin();

	ImprimeSautPage();
	ImprimeTableauLargeurs(0,6,12,17,14,40,7,7);
	ImprimeTableauChapeau("Modeles de numeriseurs");
	ImprimeTableauCol(0,"nom");
	ImprimeTableauCol(1,"ressource");
	ImprimeTableauCol(2,"categorie");
	ImprimeTableauCol(3,"valeurs"); /* RESS_TYPETEXTE */
	ImprimeTableauCol(4,"adrs"); /* adrs:bit0 */
	ImprimeTableauCol(5,"masque");
	ImprimeTableauLimite(0,5);
	for(i=0; i<ModeleBNNb; i++) {
		TypeBNModlRessource *ress; char textetype[8],texteadrs[4]; char vu;
		char textevaleur[40];
		if(i) ImprimeTableauLimite(0,5); // ImprimeTableauTrait(0,5,1);
		ImprimeTableauCol(0,ModeleBN[i].nom);
		vu = 0;
		for(k=0; k<ModeleBN[i].nbress; k++) {
			if(k) ImprimeNouvelleLigne(1);
			ress = &(ModeleBN[i].ress[k]);
			ImprimeTableauCol(1,ress->nom);
			ImprimeTableauCol(2,RessCategListe[ress->categ]);
			ArgKeyGetText(RESS_TYPETEXTE,ress->type,textetype,8);
			switch(ress->type) {
			  case RESS_INT:
				snprintf(textevaleur,40,"%d .. %d",ress->lim_i.min,ress->lim_i.max);
				break;
			  case RESS_FLOAT:
				snprintf(textevaleur,40,"%g .. %g",ress->lim_r.min,ress->lim_r.max);
				break;
			  case RESS_CLES:
				strncpy(textevaleur,ress->lim_k.cles,40);
				break;
			  case RESS_PUIS2:
				if(ress->lim_i.max == 1) {
					if(ress->lim_i.min) snprintf(textevaleur,40,"2^(x-%d)",ress->lim_i.min);
					else strcpy(textevaleur,"2^x");
				} else {
					if(ress->lim_i.min) snprintf(textevaleur,40,"%d * 2^(x-%d)",ress->lim_i.max,ress->lim_i.min);
					else snprintf(textevaleur,40,"%d * 2^x",ress->lim_i.max);
				}
				break;
			}
			if(ress->unite[0]) ImprimeTableauCol(3,"%s: %s %s",textetype,textevaleur,ress->unite);
			else ImprimeTableauCol(3,"%s: %s",textetype,textevaleur);
			if(ress->ssadrs == 0xFF) strcpy(texteadrs,"..");
			else sprintf(texteadrs,"%02x",ress->ssadrs);
			if(ress->bit0 < 0) ImprimeTableauCol(4,texteadrs);
			else ImprimeTableauCol(4,"%2s:%02d",texteadrs,ress->bit0);
			ImprimeTableauCol(5,"%4X",ress->masque);
			vu = 1;
		}
		if(!vu) ImprimeTableauCol(1,"--");
	}
	ImprimeTableauFin();

	return(0);
}
/* ========================================================================== */
int SettingsDocImprimeAutom(Menu menu, MenuItem item) {
	int i; char texte[80];
	
	ImprimeSautPage();
	ImprimeTableauLargeurs(0,5,32,15,8,14,14);
	ImprimeTableauChapeau("Variables de l'automate");
	ImprimeTableauCol(0,"nom");
	ImprimeTableauCol(1,"type");
	ImprimeTableauCol(2,"code");
	ImprimeTableauCol(3,"etat");
	ImprimeTableauCol(4,"valeur");
	ImprimeTableauLimite(0,4);
	for(i=0; i<AutomVarNb; i++) {
		if(AutomVar[i].size < 2) ImprimeTableauCol(0,AutomVar[i].nom);
		else ImprimeTableauCol(0,"%s[%d]",AutomVar[i].nom,AutomVar[i].size);
		ImprimeTableauCol(1,"%-12s",AutomTypeTexte(AutomVar[i].contenu));
		ImprimeTableauCol(2,"%-13s",AutomCodeNom[AutomVar[i].code]);
		ImprimeTableauCol(3,"%-12s",AutomEtatTexte(AutomVar[i].etat));
		switch(AutomVar[i].code) {
		  case AUTOM_BOOL:  ImprimeTableauCol(4,"%12s",AutomVar[i].val.b? "oui": "non"); break;
		  case AUTOM_CLE:   ArgKeyGetText(AutomVar[i].cles,AutomVar[i].val.b,texte,80);
		                    ImprimeTableauCol(4,"%12s",texte); break;
		  case AUTOM_INT:   ImprimeTableauCol(4,"%12d",AutomVar[i].val.i); break;
		  case AUTOM_FLOAT: ImprimeTableauCol(4,"%12g",AutomVar[i].val.r); break;
		}
		ImprimeNouvelleLigne(1);
	}
	ImprimeTableauFin();
	return(0);
}
/* ========================================================================== */
int SettingsDocImprimeScripts(Menu menu, MenuItem item) {
	int trait_dernier,i; char *ligne;

	ImprimeSautPage();
	trait_dernier = 102;
	ImprimeLimite(0,trait_dernier,0);
	ImprimeColonnes(2,0,trait_dernier);
	i = 0; ligne = ScriptSyntaxe[i];
	ImprimeNouvelleLigne(1); ImprimeEnCol((trait_dernier-19)/2,ligne); ImprimeLimite(0,trait_dernier,1); ImprimeNouvelleLigne(1);
	ligne = ScriptSyntaxe[++i];
	while(*ligne) { ImprimeNouvelleLigne(1); ImprimeEnCol(1,"%s",ligne); ligne = ScriptSyntaxe[++i]; }
	ImprimeLimite(0,trait_dernier,1); ImprimeColonnes(0); ImprimeNouvelleLigne(1);

	return(0);
}
/* ========================================================================== */
int SettingsDocImprime(Menu menu, MenuItem item) {
	char fichier[MAXFILENAME]; int k,l;
	int hauteur_page; int trait_premier,trait_dernier,lngr;
	
	hauteur_page = 54;
	if(OpiumExec(pSettingsImprimeForme->cdr) == PNL_CANCEL) return(0);

	if((SettingsDocSupport == IMPRESSION_TERMINAL) && (SettingsDocFormat == IMPRESSION_TEXTE)) {
		SettingsDocSupport = IMPRESSION_FICHIER;
		strcpy(fichier,"*");
	} else {
		if(Acquis[AcquisLocale].etat.active && Archive.enservice) {
			if(ArchiveSurRepert) sprintf(fichier,"%s%s%s_info",ArchiveChemin[EVTS],FILESEP,Acquis[AcquisLocale].etat.nom_run);
			else strcat(strcpy(fichier,ArchiveChemin[EVTS]),"_info");
		} else strcat(strcat(strcat(strcpy(fichier,PrefPath),"Info"),"_"),Acquis[AcquisLocale].code); // ResuPath?
		if(SettingsDocSupport == IMPRESSION_PAPIER) {
			char nom_imprimante[32];
			if(OpiumExec(pSettingsImprimeSvr->cdr) == PNL_CANCEL) return(0);
			ArgKeyGetText(ListePrinters,NumPrinter,nom_imprimante,32);
			ImpressionServeur(nom_imprimante,fichier,NbTirages);
		}
		if(SettingsDocFormat != IMPRESSION_TEXTE) strcat(fichier,".ps");
	}
	ImpressionFormatte(SettingsDocFormat);
	ImpressionSurSupport(SettingsDocSupport,fichier);
	if(!ImpressionPrete("landscape")) return(0);
	// if(hauteur_page < ImpressionHauteurCourante()) ImpressionHauteurPage(hauteur_page);

	printf("-------------------------- Documentation pour le materiel --------------------------\n");
	/* Entete generale */
	trait_dernier = 110; // ImpressionLargeurCourante();
	ImprimeEnCol(trait_dernier - 20,DateCivile()); ImprimeEnCol(trait_dernier - 11,DateHeure());

	ImprimeNouvelleLigne((ImpressionHauteurCourante() / 2) - 5);
	trait_premier = 20; lngr = trait_dernier - (2 * trait_premier);
	ImprimeLimite(trait_premier,lngr,0); ImprimeColonnes(2,trait_premier,trait_premier+lngr);
	ImprimeNouvelleLigne(2);
	
	l = strlen(SambaIntitule);
	k = (trait_dernier - l) / 2;
	ImprimeEnCol(k,SambaIntitule); ImprimeFond(k,l,0xFFFF,0xF000,0xD000);
	ImprimeNouvelleLigne(2);
	k = (trait_dernier - 22) / 2;
	ImprimeEnCol(k,"Documentation generale");
	ImprimeNouvelleLigne(2);
	ImprimeLimite(trait_premier,lngr,1); ImprimeColonnes(0);

	/* Documentations demandees */
	if(SettingsDocCablage) SettingsDocImprimeCablage(menu,item);
	if(SettingsDocModeles) SettingsDocImprimeModeles(menu,item);
	if(SettingsDocAutom)   SettingsDocImprimeAutom(menu,item);
	if(SettingsDocScripts) SettingsDocImprimeScripts(menu,item);

	ImpressionFin(1);
	printf("%s/ Documentation ecrite dans %s\n",DateHeure(),fichier);
	
	return(0);
}

/* ............... fonctions de gestion figurant au menu mGest .............. */
#pragma mark --- gestion ---
/* ========================================================================== */
static int SettingsEdite(Menu menu, MenuItem item) {
	char fichier[MAXFILENAME]; char ligne[256];

	strcat(strcpy(fichier,TopDir),"nom");
	if(OpiumReadFile("Nom",fichier,MAXFILENAME) == PNL_CANCEL) return(0);
#ifdef macintosh
	sprintf(ligne,"open -e %s",fichier);
#else
	sprintf(ligne,"xterm -e emacs %s &",fichier);
#endif
	system(ligne);
	return(0);
}
/* ========================================================================== */
static int SettingsPasse(Menu menu, MenuItem item) {
	if(!AccesRestreint()) ExpertConnecte = 1 - ExpertConnecte;
	OpiumNote("Samba est maintenant en acces %s",ExpertConnecte? "libre": "restreint");
	return(1);
}
/* ========================================================================== */
static void SettingsCorrigePlanches() {
	if(OpiumDisplayed(SettingsVolet[SETTINGS_TRMT].planche)) SettingsDetecAffiche(SETTINGS_TRMT); // si on a touche SettingsDeconvPossible
	if(OpiumDisplayed(SettingsVolet[SETTINGS_TRGR].planche)) SettingsDetecAffiche(SETTINGS_TRGR); // si on a touche SettingsCoupures et SettingsTrigInOut
	if(OpiumDisplayed(bSuiviEvts)) LectPlancheSuivi(1); // si on a touche SettingsDeconvPossible
}
/* ========================================================================== */
int SettingsOrgaSauve() { ArgPrint(ConfigGeneral,Setup); return(0); }
#ifdef PREFERENCES_PAR_MODULE
/* ========================================================================== */
static int SettingsParmsLit(Menu menu, MenuItem item) {
	FILE *f;

	if(OpiumReadFile("Depuis quel fichier",FichierPrefParms,MAXFILENAME) == PNL_CANCEL) return(0);
	f = fopen(FichierPrefParms,"r");
	if(f) {
		ArgFromFile(f,SettingsTotal,0);
		Trigger.enservice = (Trigger.type == NEANT)? 0: 1;
		fclose(f);
		SettingsParmsASauver = 0;
	} else OpiumError("Lecture sur '%s' impossible",FichierPrefParms);
	return(0);
}
#endif
/* ========================================================================== */
void SettingsParmsVerif() {
	char modifie;

	modifie = 0;
	if(Trigger.type != TRGR_CABLE) { Trigger.type = TRGR_CABLE; modifie = 1; }
	if(ArchModeStream != ARCH_BRUTES) { ArchModeStream = ARCH_BRUTES; modifie = 1; }
	if(SambaDefMax != SambaDefInitial) modifie = 1;
	if(modifie) {
		if(ArgPrint(FichierPrefFinesses,SettingsFinessesDesc) > 0) {
			printf("* Fignolages rectifies sur %s\n",FichierPrefFinesses);
			SettingsFinessesASauver = 0;
		} else printf("! Sauvegarde sur '%s' impossible\n",FichierPrefFinesses);
	}
}
/* ========================================================================== */
int SettingsParmsChange() {
	SettingsParmsASauver = 1;
#ifdef EDW_ORIGINE
	int i;
	for(i=0; i<AUTOM_REGEN_MAX; i++) {
		AutomSrceRegenIndex[i] = AutomRang(AutomRegenNom[i]);
		printf("- Index de la variable automate '%s' (source regeneration): %d\n",AutomRegenNom[i],AutomSrceRegenIndex[i]);
		AutomSrceRegenAdrs[i] = AutomAdrsKey(AutomSrceRegenIndex[i]);
		if(AutomSrceRegenAdrs[i] == 0) {
			printf("* La position de la source de regeneration (%s) n'est pas lue\n",AutomRegenNom[i]);
			AutomSrceRegenAdrs[i] = &CharBidon;
		}
	}
	for(i=0; i<AUTOM_CALIB_MAX; i++) {
		AutomSrceCalibIndex[i] = AutomRang(AutomCalibNom[i]);
		printf("- Index de la variable automate '%s' (source  calibration): %d\n",AutomCalibNom[i],AutomSrceCalibIndex[i]);
		AutomSrceCalibAdrs[i] = AutomAdrsKey(AutomSrceCalibIndex[i]);
		if(AutomSrceCalibAdrs[i] == 0) {
			printf("* La position de la source de calibration (%s) n'est pas lue\n",AutomCalibNom[i]);
			AutomSrceCalibAdrs[i] = &CharBidon;
		}
	}
#endif
	return(0);
}
/* ========================================================================== */
static int SettingsParmsEcrit(Menu menu, MenuItem item) {
	FILE *f; Cadre boitier;

	if(SettingsSauveChoix) {
        if(OpiumReadFile("Sauver sur le fichier",FichierPrefParms,MAXFILENAME) == PNL_CANCEL) return(0);
    }
	SettingsCorrigePlanches();
	RepertoireCreeRacine(FichierPrefParms);
	f = fopen(FichierPrefParms,"w");
	if(f) {
		ArgAppend(f,0,SettingsDesc); fclose(f);
		printf("* Parametres sauves sur %s\n",FichierPrefParms);
		boitier = (Cadre)bPrmCourants->adrs; while(boitier) { boitier->var_modifiees = 0; boitier = boitier->suivant; }
		bPrmCourants->var_modifiees = 0;
		SettingsParmsASauver = 0;
//		printf("> (%s) bPrmCourants->var_modifiees @%08X: %d\n",__func__,(hexa)(&(bPrmCourants->var_modifiees)),bPrmCourants->var_modifiees);
//		printf("> (%s) SettingsParmsASauver @%08X: %d\n",__func__,(hexa)(&SettingsParmsASauver),SettingsParmsASauver);
	} else OpiumError("Sauvegarde sur '%s' impossible",FichierPrefParms);
	return(0);
}
/* ========================================================================== */
static int SettingsFinessesEcrit(Menu menu, MenuItem item) {
	if(SettingsSauveChoix) {
        if(OpiumReadFile("Sauver sur le fichier",FichierPrefFinesses,MAXFILENAME) == PNL_CANCEL) return(0);
    }
	if(ArgPrint(FichierPrefFinesses,SettingsFinessesDesc) > 0) {
		printf("* Fignolages sauves sur %s\n",FichierPrefFinesses);
		SettingsFinessesASauver = 0;
	} else OpiumError("Sauvegarde sur '%s' impossible",FichierPrefFinesses);
	return(0);
}
/* ==========================================================================
static int SettingsPanique(Menu menu, MenuItem item) {
	LectAffichePanique = 1 - LectAffichePanique;
	printf("%s/ L'acquisition est mise %s par l'utilisateur\n",DateHeure(),LectAffichePanique? "en panique": "au calme");
	return(0);
}
   ========================================================================== */
static int SettingsSauveTout(Menu menu, MenuItem item) {
	SettingsParmsEcrit(0,0);
	SettingsFinessesEcrit(0,0);
	// planche ne possedant pas ce bouton: if(bPrmCourants) bPrmCourants->var_modifiees = 0;
	if(bPrmDetailles) bPrmDetailles->var_modifiees = 0;
	return(0);
}
/* ========================================================================== */
static int SettingsRegulEcrit(Menu menu, MenuItem item) {
	if(SettingsSauveChoix) {
        if(OpiumReadFile("Sauver sur le fichier",FichierPrefRegulEvt,MAXFILENAME) == PNL_CANCEL) return(0);
    }
	if(ArgPrint(FichierPrefRegulEvt,SettingsRegulDesc) <= 0)
		OpiumError("Sauvegarde sur '%s' impossible",FichierPrefRegulEvt);
	return(0);
}
/* ========================================================================== */
static int SettingsVerifEcrit(Menu menu, MenuItem item) {
	if(SettingsSauveChoix) {
        if(OpiumReadFile("Sauver sur le fichier",FichierPrefVerif,MAXFILENAME) == PNL_CANCEL) return(0);
    }
	if(ArgPrint(FichierPrefVerif,SettingsVerifDesc) <= 0)
		OpiumError("Sauvegarde sur '%s' impossible",FichierPrefVerif);
	return(0);
}
/* ========================================================================== */
static int SettingsVerifCR(Menu menu, MenuItem item) {
	if(SettingsSauveChoix) {
        if(OpiumReadFile("Sauver sur le fichier",FichierCrParms,MAXFILENAME) == PNL_CANCEL) return(0);
    }
	if(ArgPrint(FichierCrParms,SambaCrDesc) <= 0)
		OpiumError("Sauvegarde sur '%s' impossible",FichierCrParms);
	return(0);
}
/* ========================================================================== */
static int SettingsFinder(Menu menu, MenuItem item) { BoardRepertoireOuvre(PrefPath,REPERT_ALL); return(0); }
/* ========================================================================== */
static int EffaceDB() {
	Panel p; int rc;
	char id[DB_LNGRTEXTE],rev[DB_LNGRTEXTE];
	
	if(EdbActive) {
		DbGetLastId(id,rev);
		p = PanelCreate(2);
		PanelText(p,"Id",id,DB_LNGRTEXTE);
		PanelText(p,"Rev",rev,DB_LNGRTEXTE);
		rc = OpiumExec(p->cdr);
		if(rc == PNL_OK) {
			DbRemove(EdbServeur,id,rev);
			if(DbStatus) printf("%s/ Base de Donnees modifiee (Id: %s, rev: %s)\n",DateHeure(),DbId,DbRev);
			else printf("%s/ ! Modification de la Base de Donnees en erreur (%s, raison: %s)\n",DateHeure(),DbErreur,DbRaison);
		}
		PanelDelete(p);
	} else OpiumWarn("Pas de serveur de Base de Donnees");

	return(0);
}
/* ========================================================================== */
static int SettingsBombe() {
	int i;

	i = 0;
	if(OpiumReadList("Message",LectListeExplics,&i,64) == PNL_CANCEL) return(1);
	if((LectErreur.code = LectCodeConnu[i])) {
		Acquis[AcquisLocale].etat.active = 0;
		return(1);
	}
	if(OpiumReadSHex("Code",(short *)&LectErreur.code) == PNL_OK) Acquis[AcquisLocale].etat.active = 0;

	return(1);
}
/* ========================================================================== */
static int SettingsTraductions() { OpiumDicoEdite(&OpiumDico); return(0); }
/* ========================================================================== */
// static int SettingsSuit() { printf("(%s) Configuration: '%s'\n",__func__,SambaIntitule); return(0); }
/* ========================================================================== */
#pragma mark --- menus ---

TypeMenuItem iSettings[] = {
	{ "Histogrammes",     MNU_FONCTION MonitHistosDef },
	{ "Ecran",            MNU_FONCTION MonitEcranPanel   },
	{ "Finesses",         MNU_PANEL  &pSettingsFinesses  },
#ifdef PREFERENCES_PAR_MODULE
	{ "Preferences",      MNU_SEPARATION                 },
	{ "Recuperer",        MNU_FONCTION SettingsParmsLit  },
	{ "Verifier",         MNU_PANEL  &pSettingsDesc      },
	{ "Sauver",           MNU_FONCTION SettingsSauveTout },
	{ "\0",               MNU_SEPARATION                 },
#endif
	{ "Acces libre",      MNU_FONCTION SettingsPasse     },
	{ "Fermer",           MNU_RETOUR                     },
	MNU_END
};

#define XTERM "xterm +bdc +cm -aw -sb -rightbar -rv -cr green -ms yellow -geometry 132x60 -ls /bin/tcsh &"
int SambaTri(Menu menu, MenuItem item);
TypeMenuItem iGest[] = {
	{ "Parametres",        MNU_MENU   &mGestParms           },
	{ "Tous les fichiers", MNU_FONCTION SambaMontreSetup    },
	{ "Disque sauvegarde", MNU_FONCTION SambaDisque         },
	{ "Finder",            MNU_FONCTION SettingsFinder      },
	{ "Edition fichier",   MNU_FONCTION SettingsEdite       },
//	{ "Shell",             MNU_COMMANDE "open -a Terminal"  },
//	{ "Shell",             MNU_COMMANDE "bash &"  },
//	{ "Shell",             MNU_COMMANDE "xterm &"           },
	{ "Shell",             MNU_COMMANDE XTERM               },
	{ "\0",                MNU_SEPARATION                   },
	{ "Filtres",           MNU_MENU   &mFiltres             },
	{ "Generateurs",       MNU_MENU   &mGeneGere            },
	{ "Classement",        MNU_FONCTION SambaTri            },
	{ "Exportations",      MNU_MENU   &mGestExport          },
	{ "Calendrier",        MNU_MENU   &mGestCal             },
	{ "\0",                MNU_SEPARATION                   },
	{ "Langue",            MNU_FONCTION SambaLangueDemande  },
	{ "Traduction",        MNU_FONCTION SettingsTraductions    },
	{ "\0",                MNU_SEPARATION                   },
	{ "Pattern",           MNU_MENU   &mPattern             },
	{ "Evt unite",         MNU_MENU   &mTemplates           },
	{ "Courbes V(I)",      MNU_MENU   &mGestVI              },
	{ "Effacer enreg DB",  MNU_FONCTION EffaceDB            },
	{ "\0",                MNU_SEPARATION                   },
	{ "Version",           MNU_FONCTION SambaAfficheVersion },
	{ "Acces libre",       MNU_FONCTION SettingsPasse       },
	{ "Declencher une erreur", MNU_FONCTION SettingsBombe      },
//	{ "Verifie variable",  MNU_FONCTION SettingsSuit           },
	{ "\0",                MNU_SEPARATION                   },
	{ "Fermer",            MNU_RETOUR                       },
	MNU_END
};

static TypeMenuItem iGestCal[] = {
	{ "Chargement",        MNU_FONCTION SambaLitCalendrier },
	{ "Liste",             MNU_FONCTION PasImplementee     },
	{ "Fermer",            MNU_RETOUR                      },
	MNU_END
};

static TypeMenuItem iGestExport[] = {
	{ "Creer",             MNU_FONCTION ExportAjoute       },
	{ "Retirer",           MNU_FONCTION ExportRetire       },
	{ "Modifier",          MNU_FONCTION ExportChange       },
	{ "Variables",         MNU_FONCTION ExportVariables    },
	{ "Fermer",            MNU_RETOUR                      },
	MNU_END
};

static TypeMenuItem iGestParms[] = {
	{ "Appel Samba",      MNU_MENU  &mGestAppel           },
	{ "Config",           MNU_MENU  &mGestConfig          },
	{ "Generaux",         MNU_MENU  &mGestGeneraux        },
#ifdef PARMS_PAR_MENUS
	{ "Regulation",       MNU_MENU  &mGestRegulEvt        },
	{ "Affichage",        MNU_MENU  &mGestAffichage       },
	{ "Calcul",           MNU_MENU  &mGestCalcul          },
	{ "Finesses",         MNU_MENU  &mGestFinesses        },
	{ "Debug",            MNU_MENU  &mGestVerif           },
#else  /* PARMS_PAR_MENUS */
	{ "Regulation",       MNU_PANEL &pSettingsRegulEvt    },
	{ "Affichage",        MNU_PANEL &pMonitDesc           },
	{ "Calcul",           MNU_PANEL &pCalculDesc         },
	{ "Finesses",         MNU_PANEL &pSettingsFinesses    },
	{ "Debug",            MNU_PANEL &pSettingsVerif       },
#endif /* PARMS_PAR_MENUS */
	{ "Compte-rendus",    MNU_PANEL &pSettingsCompteRendu },
	{ "Fermer",           MNU_RETOUR                      },
	MNU_END
};

static TypeMenuItem iGestAppel[] = {
	{ "Verifier",         MNU_PANEL  &pSambaDesc         },
	{ "Sauver",           MNU_FONCTION SambaSauveArgs    },
	{ "Fermer",           MNU_RETOUR                     },
	MNU_END
};
static TypeMenuItem iGestConfig[] = {
	{ "Verifier",         MNU_PANEL  &pConfigDesc        },
	{ "Sauver",           MNU_FONCTION SettingsOrgaSauve  },
	{ "Fermer",           MNU_RETOUR                     },
	MNU_END
};
static TypeMenuItem iGestGeneraux[] = {
	{ "Modifier",         MNU_PANEL  &pSettingsDesc       },
	{ "Sauver",           MNU_FONCTION SettingsParmsEcrit },
	{ "Fermer",           MNU_RETOUR                      },
	MNU_END
};

#ifdef PARMS_PAR_MENUS
	static TypeMenuItem iGestRegulEvt[] = {
		{ "Modifier", MNU_PANEL  &pSettingsRegulEvt      },
		{ "Sauver",   MNU_FONCTION SettingsRegulEcrit    },
		{ "Fermer",   MNU_RETOUR                         },
		MNU_END
	};
	static TypeMenuItem iGestAffichage[] = {
		{ "Modifier", MNU_PANEL  &pMonitDesc            },
		{ "Sauver",   MNU_FONCTION MonitSauve           },
		{ "Fermer",   MNU_RETOUR                        },
		MNU_END
	};
	static TypeMenuItem iGestCalcul[] = {
		{ "Modifier", MNU_PANEL  &pCalculDesc           },
		{ "Sauver",   MNU_FONCTION CalculeSauve         },
		{ "Fermer",   MNU_RETOUR				        },
		MNU_END
	};
	static TypeMenuItem iGestFinesses[] = {
		{ "Modifier", MNU_PANEL  &pSettingsFinesses      },
		{ "Sauver",   MNU_FONCTION SettingsFinessesEcrit },
		{ "Fermer",   MNU_RETOUR                         },
		MNU_END
	};
	static TypeMenuItem iGestVerif[] = {
		{ "Modifier", MNU_PANEL  &pSettingsVerif         },
		{ "Sauver",   MNU_FONCTION SettingsVerifEcrit    },
		{ "Fermer",   MNU_RETOUR                         },
		MNU_END
	};
#endif /* PARMS_PAR_MENUS */

static TypeMenuItem iPattern[] = {
	{ "Memoriser",        MNU_FONCTION PatternMemorise   },
	{ "Afficher",         MNU_FONCTION PatternAffiche    },
	{ "Sauver",           MNU_FONCTION PatternSauve      },
	{ "Lire",             MNU_FONCTION PatternLit        },
	{ "Fermer",           MNU_RETOUR                     },
	MNU_END
};

static TypeMenuItem iTemplates[] = {
	{ "Memoriser",        MNU_FONCTION TemplatesMemorise },
	{ "Analytique",       MNU_FONCTION TemplatesAnalytique },
	{ "Filtrer",          MNU_FONCTION TemplatesFiltrer  },
	{ "Afficher",         MNU_FONCTION TemplatesAffiche  },
	{ "Sauver",           MNU_FONCTION TemplatesSauve    },
	{ "Lire",             MNU_FONCTION TemplatesLit      },
	{ "Fermer",           MNU_RETOUR                     },
	MNU_END
};

static TypeMenuItem iGestVI[] = {
	{ "Visualiser courante",   MNU_FONCTION ViVisu       },
	{ "Retirer un point",      MNU_FONCTION ViRetire     },
    { "Effacer courante",      MNU_FONCTION ViRaz        },
    { "Reprendre precedentes", MNU_FONCTION ViLit        },
	{ "Memoriser courante",    MNU_FONCTION ViMemorise   },
//    { "Retirer courbe",        MNU_FONCTION ViSupprime   },
	{ "Enregistrer",           MNU_FONCTION ViEcrit      },
	{ "Fermer",                MNU_RETOUR                },
	MNU_END
};

int SequencesImprime();
static TypeMenuItem iImprim[] = {
	{ "Fenetre en JPEG",  MNU_FONCTION OpiumPhotoSauve      },
	{ "Detecteurs",       MNU_FONCTION SettingsImprimeBolos },
	{ "Cablage",          MNU_FONCTION DetecteursDump       },
	{ "Voies connues",    MNU_FONCTION SambaDumpVoiesToutes },
//+	{ "Identification",   MNU_FONCTION LectIdentImprime     }, utilise VoieIdent
	{ "Sequences",        MNU_FONCTION SequencesImprime },
	{ "Syntaxe scripts",  MNU_FONCTION ScriptEcritSyntaxe     },
	{ "Filtres",          MNU_FONCTION SambaImprimeFiltres  },
	{ "Traitements",      MNU_FONCTION LectJournalTrmt      },
//	{ "Declenchement",    MNU_FONCTION TrmtImprimeTrgr      },
	{ "Declenchement",    MNU_FONCTION TrmtRAZ              },
	{ "Documentation",    MNU_FONCTION SettingsDocImprime      },
	{ "Fermer",           MNU_RETOUR                        },
	MNU_END
};

/* ........................ fonctions generales ............................. */
#pragma mark --- gestion commune ---

#define STORE_CONFIG(volet,acqinfo,titre) \
int bolo,cap,voie; char globale; \
TypeConfigVoie *config_voie; TypeConfigDefinition *utilisee,*config; int num_config; \
\
if(((cap = (int)v2) >= 0) && ((bolo = SettingsVolet[volet].bolo) >= 0)) { \
	if((bolo != BoloStandard) && ((voie = Bolo[bolo].captr[cap].voie) >= 0)) { \
		if(GestionConfigs) printf("(%s)  ____ Change %s %s\n",__func__,titre,VoieManip[voie].nom); \
		utilisee = &(VoieManip[voie].def); globale = utilisee->global.acqinfo; \
		utilisee->defini.acqinfo = 1; \
		config_voie = &(VoieManip[voie].config); \
		num_config = SettingsDetecConfigLocale(config_voie,voie,globale); \
		config_voie->num[globale].acqinfo = num_config; \
		config = &(config_voie->def[num_config]); \
		config->global.acqinfo = globale; \
		if(GestionConfigs) printf("(%s) | %s voie %s: %s config #%d (%s)\n",__func__,titre,VoieManip[voie].nom, \
			(config->defini.acqinfo)?"recopie sur":"defini",num_config,config->nom); \
		memcpy(&(config->acqinfo),&(utilisee->acqinfo),sizeof(TypeTraitement)); \
		config->defini.acqinfo = 1; \
		if(GestionConfigs) { \
			printf("(%s) | %s voie %s: represente globale #%d (%s) donc config #%d (%s)\n",__func__,titre,VoieManip[voie].nom, \
				   utilisee->global.acqinfo,ConfigCle[utilisee->global.acqinfo],num_config,config->nom); \
			printf("(%s) |____ Changement pris en compte\n",__func__); \
		} \
	} \
}

/* ========================================================================== */
static int SettingsDetecConfigLocale(TypeConfigVoie *config_voie, int voie, char globale) {
	int num_config;

	if(GestionConfigs) printf("(%s) Cherche la config correspondant a la globale #%d (%s)\n",__func__,globale,ConfigCle[globale]);
	for(num_config=0; num_config<config_voie->nb; num_config++) {
		if(!strcmp(config_voie->def[num_config].nom,ConfigCle[globale])) break;
	}
	if(num_config >= config_voie->nb) {
		if(GestionConfigs) printf("(%s) | Nouvelle configuration a definir: '%s'\n",__func__,ConfigCle[globale]);
		if(config_voie->nb < CONFIG_MAX) {
			num_config = config_voie->nb;
			strncpy(config_voie->def[num_config].nom,ConfigCle[globale],CONFIG_NOM);
			config_voie->nb += 1;
		} else {
			OpiumFail("! Deja %d configurations definies pour la voie %s. En supprimer une, avant nouvel ajout",
				CONFIG_MAX,VoieManip[voie].nom);
			return(0);
		}
	}
	if(GestionConfigs) printf("(%s) |_ Configuration trouvee: #%d\n",__func__,num_config);

	return(num_config);
}
/* ========================================================================== */
static int SettingsDetecDeclareModif(char volet, char change) {
	MenuItemAllume(SettingsVolet[volet].general,1,0,GRF_RGB_YELLOW);
	if(SettingsVolet[volet].bolo == BoloStandard) {
		SettingsVolet[volet].standard_a_jour = 0;
		MenuItemAllume(SettingsVolet[volet].general,2,0,GRF_RGB_YELLOW);
	}
	if(change) printf("> Parametres des %ss, modifies pour le detecteur %s\n",SettingsVoletTitre[volet],Bolo[SettingsVolet[volet].bolo].nom);
	Bolo[SettingsVolet[volet].bolo].a_sauver = 1;
	return(0);
}
/* ========================================================================== */
static int SettingsDetecCopieStd(Menu menu, MenuItem item) {
	int volet,bolo,cap,vm,voie,classe,j;
	
	if(menu) {
		for(volet=SETTINGS_TRMT; volet<SETTINGS_NBVOLETS; volet++) if(menu == SettingsVolet[volet].general) break;
		if(volet >= SETTINGS_NBVOLETS) return(0);
	} else volet = (int)item;
	for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].local) {
		for(cap=0; cap<Bolo[bolo].nbcapt; cap++) if((voie = Bolo[bolo].captr[cap].voie) >= 0) {
			vm = VoieManip[voie].type;
			switch(volet) {
			  case SETTINGS_TRMT:
				for(classe=0; classe<TRMT_NBCLASSES; classe++) if(VoieManip[voie].def.trmt[classe].std && !(VoieManip[voie].pseudo)) {
					memcpy(&(VoieManip[voie].def.trmt[classe]),&(VoieStandard[vm].def.trmt[classe]),sizeof(TypeTraitement));
					printf(". Reglages de traitement %s %s standard reportes dans la voie %s\n",TrmtClasseNom[classe],VoieStandard[vm].def.evt.nom,VoieManip[voie].nom);
					Bolo[bolo].a_sauver = 1;
				}
				break;
			  case SETTINGS_TRGR:
				if(VoieManip[voie].def.trgr.std && !(VoieManip[voie].pseudo)) {
					memcpy(&(VoieManip[voie].def.trgr),&(VoieStandard[vm].def.trgr),sizeof(TypeTrigger));
					printf(". Reglages de trigger %s standard reportes dans la voie %s\n",VoieStandard[vm].def.evt.nom,VoieManip[voie].nom);
					memcpy(&(VoieManip[voie].def.coup),&(VoieStandard[vm].def.coup),sizeof(TypeCritereJeu));
					VoieManip[voie].def.coup_std = VoieStandard[vm].def.coup_std;
					printf(". Reglages de coupures %s standard reportes dans la voie %s\n",VoieStandard[vm].def.evt.nom,VoieManip[voie].nom);
					Bolo[bolo].a_sauver = 1;
				}
				break;
			#ifdef SETTINGS_COUP
			  case SETTINGS_COUP:
				if(VoieManip[voie].def.coup_std && !(VoieManip[voie].pseudo)) {
					memcpy(&(VoieManip[voie].def.coup),&(VoieStandard[vm].def.coup),sizeof(TypeCritereJeu));
					printf(". Reglages de coupures %s standard reportes dans la voie %s\n",VoieStandard[vm].def.evt.nom,VoieManip[voie].nom);
					Bolo[bolo].a_sauver = 1;
				}
				break;
			#endif
			  case SETTINGS_RGUL:
				if(VoieManip[voie].def.rgul.std && !(VoieManip[voie].pseudo)) {
					memcpy(&(VoieManip[voie].def.rgul),&(VoieStandard[vm].def.rgul),sizeof(TypeRegulParms));
					for(j=0; j<MAXECHELLES; j++) memcpy(&(VoieManip[voie].echelle[j]),&(VoieStandard[vm].echelle[j]),sizeof(TypeRegulTaux));
					memcpy(&(VoieManip[voie].interv),&(VoieStandard[vm].interv),sizeof(TypeRegulInterv));
					printf(". Reglages de regulation %s standard reportes dans la voie %s\n",VoieStandard[vm].def.evt.nom,VoieManip[voie].nom);
					Bolo[bolo].a_sauver = 1;
				}
				break;
			  case SETTINGS_CATG:
				if(VoieManip[voie].def.catg_std && !(VoieManip[voie].pseudo)) {
					printf(". Reglages de classification %s standard reportes dans la voie %s\n",VoieStandard[vm].def.evt.nom,VoieManip[voie].nom);
					SambaConfigCopieCatg(&(VoieManip[voie].def),&(VoieStandard[vm].def));
//					VoieManip[voie].def.nbcatg = VoieStandard[vm].def.nbcatg;
//					VoieManip[voie].def.catg_std = VoieStandard[vm].def.catg_std;
//					for(k=0; k<VoieStandard[vm].def.nbcatg; k++)
//						memcpy(&(VoieManip[voie].def.catg[k]),&(VoieStandard[vm].def.catg[k]),sizeof(TypeCateg));
					Bolo[bolo].a_sauver = 1;
				}
				break;
			}
		}
	}
	MenuItemEteint(SettingsVolet[volet].general,1,0);
	SettingsVolet[volet].standard_a_jour = 1;
	return(0);
}
/* ========================================================================== */
static int SettingsDetecEnreg(Menu menu, MenuItem item) {
	int bolo; int volet;
	
	for(volet=SETTINGS_TRMT; volet<SETTINGS_NBVOLETS; volet++) if(menu == SettingsVolet[volet].general) break;
	if(volet >= SETTINGS_NBVOLETS) return(0);
	bolo = SettingsVolet[volet].bolo;
	// printf("* Sauvegarde du detecteur #%d (%s) demandee\n",bolo,Bolo[bolo].nom);
	DetecteurSauveReglages(bolo);
	Bolo[bolo].a_sauver = 0;
	if(bolo == BoloStandard) MenuItemEteint(SettingsVolet[volet].general,2,0);
	else MenuItemEteint(SettingsVolet[volet].general,1,0);
	return(0);
}
/* ========================================================================== */
static int SettingsDetecVerifie(Cadre cdr, void *arg) {
	int volet,bolo;

	for(volet=SETTINGS_TRMT; volet<SETTINGS_NBVOLETS; volet++) if(cdr == SettingsVolet[volet].planche) break;
	if(volet < SETTINGS_NBVOLETS) {
		bolo = (int)arg;
		if((bolo == BoloStandard) && !SettingsVolet[volet].standard_a_jour) SettingsDetecCopieStd(0,(MenuItem)(IntAdrs)volet);
	#ifdef TRMT_DECONVOLUE
		if(volet == SETTINGS_TRMT) {
			int cap,voie;
			for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
				voie = Bolo[bolo].captr[cap].voie;
				printf("(%s) au vol: p1 = %d\n",__func__,VoieManip[voie].def.trmt[TRMT_AU_VOL].p1);
				printf("(%s) au vol: p2 = %d\n",__func__,VoieManip[voie].def.trmt[TRMT_AU_VOL].p2);
				printf("(%s) au vol: rc = %g\n",__func__,VoieManip[voie].def.trmt[TRMT_AU_VOL].p3);
				printf("(%s) pretrg: p1 = %d\n",__func__,VoieManip[voie].def.trmt[TRMT_PRETRG].p1);
				printf("(%s) pretrg: p2 = %d\n",__func__,VoieManip[voie].def.trmt[TRMT_PRETRG].p2);
				printf("(%s) pretrg: rc = %g\n",__func__,VoieManip[voie].def.trmt[TRMT_PRETRG].p3);
			}
		} else
	#endif
		if(volet == SETTINGS_CATG) {
			if(OpiumDisplayed(bMonitEcran)) MonitEcranPlanche(); // contient DetecteursEvtCateg();
			else DetecteursEvtCateg();
		}
	}
	return(0);
}

/* .................... fonctions pour les evenements ........................ */
#pragma mark --- evenements ---
/* ========================================================================== */
int SettingsDetecEvtsEnregistre() {
	DetecteurSauveReglages(BoloNum);
	MenuItemEteint(mSettingsDetecEvts,1,0);
	return(0);
}
/* ========================================================================== */
TypeMenuItem iSettingsDetecEvts[] = {
	{ "Enregistrer",  MNU_FONCTION SettingsDetecEvtsEnregistre },
//	{ "Standardiser", MNU_FONCTION SettingsEvtMemorise },
	MNU_END
};
/* ========================================================================== */
static char SettingsDetecEvtsBoloStandardise(int bolo) {
	char change; int cap,voie,vm;

	change = 0;
	for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
		voie = Bolo[bolo].captr[cap].voie;
		if(voie < 0) continue;
		if(VoieManip[voie].def.evt.std) {
			vm = VoieManip[voie].type;
			memcpy(&(VoieManip[voie].def.evt),&(VoieStandard[vm].def.evt),sizeof(TypeVoieModele));
			change = 1;
		}
	}
	if(change) {
		printf("> Parametres des evenements mis au standard pour le detecteur %s\n",Bolo[bolo].nom);
		Bolo[bolo].a_sauver = 1;
	}
	return(change);
}
/* ========================================================================== */
static char SettingsDetecEvtsBoloChange(int bolo, char modifie) {
	int cap,voie; char globale,change;
	TypeConfigVoie *config_voie; TypeConfigDefinition *utilisee,*config; int num_config;

	change = modifie;
	for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
		if((voie = Bolo[bolo].captr[cap].voie) < 0) continue;
		if(GestionConfigs) printf("(%s)  ____ Change %s %s\n",__func__,"evenement",VoieManip[voie].nom);
		utilisee = &(VoieManip[voie].def); globale = utilisee->global.evt;
		utilisee->defini.evt = 1;
		config_voie = &(VoieManip[voie].config);
		num_config = SettingsDetecConfigLocale(config_voie,voie,globale);
		if(config_voie->num[globale].evt != num_config) {
			change = 1; config_voie->num[globale].evt = num_config;
		}
		config = &(config_voie->def[num_config]);
		if(config->global.evt != globale) {
			change = 1; config->global.evt = globale;
		}
		if(GestionConfigs) printf("(%s) | %s voie %s: %s config #%d (%s)\n",__func__,"evenement",VoieManip[voie].nom,
			(config->defini.evt)?"recopie sur":"defini",num_config,config->nom);
		// if(config->defini.evt) memcpy(&(config->evt),&(utilisee->evt),sizeof(TypeVoieModele));
		// else config->defini.evt = 1;
		memcpy(&(config->evt),&(utilisee->evt),sizeof(TypeVoieModele));
		config->defini.evt = 1;
		if(GestionConfigs) {
			printf("(%s) | %s voie %s: represente globale #%d (%s) donc config #%d (%s)\n",__func__,"Evenement",VoieManip[voie].nom,
				utilisee->global.evt,ConfigCle[utilisee->global.evt],num_config,config->nom);
			printf("(%s) |____ Changement pris en compte\n",__func__);
		}
	}
	if(change) {
		printf("> Parametres des evenements, modifies pour le detecteur %s\n",Bolo[bolo].nom);
		// printf("> Parametres des evenements, modifies pour le detecteur %s\n",Bolo[BoloNum].nom);
		Bolo[bolo].a_sauver = 1;
	}
	return(change);
}
/* ========================================================================== */
int SettingsDetecEvtStandardise1(Panel panel, int item, void *v2) {
	int bolo,cap,voie; char *nom;

	PanelApply(panel,1);
	if((bolo = BoloNum) == BoloStandard) return(0);
	cap = (int)v2;
	if((voie = Bolo[bolo].captr[cap].voie) < 0) return(0);
	if((nom = DetecteurCanalStandardise(DETEC_PARM_EVTS,bolo,cap,-1))) {
		printf(". Reglages d'evenement %s standard reportes dans la voie %s\n",VoieStandard[VoieManip[voie].type].def.evt.nom,VoieManip[voie].nom);
	} else printf(". Reglages d'evenement particuliers pour la voie %s\n",VoieManip[voie].nom);
	printf("> Parametres des evenements mis au standard pour le detecteur %s\n",Bolo[bolo].nom);
	Bolo[bolo].a_sauver = 1;
	SettingsDetecEvtsCree(panel,0);

	return(0);
}
/* ==========================================================================
int SettingsEvtMemorise() {
	int bolo,cap,vm,voie;
	
	if(BoloNum == BoloStandard) {
		for(bolo=0; bolo<BoloNb; bolo++) {
			for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
				voie = Bolo[bolo].captr[cap].voie;
				if(voie < 0) continue;
				vm = VoieManip[voie].type;
				if(VoieManip[voie].def.evt.std) {
					memcpy(&(VoieManip[voie].def.evt),&(VoieStandard[vm].def.evt),sizeof(TypeVoieModele));
				}
			}
		}
	}
	printf("> Parametres des evenements, modifies pour le detecteur %s\n",Bolo[BoloNum].nom);
	Bolo[BoloNum].a_sauver = 1;
	MenuItemEteint(mSettingsDetecEvts,2,0);
	MenuItemAllume(mSettingsDetecEvts,1,0,GRF_RGB_YELLOW);
	return(0);
}
   ========================================================================== */
static int SettingsDetecEvtsChange(Panel panel, void *vide) {
	int bolo; char change;

	PanelApply(panel,1);
	change = 0;
	if(BoloNum == BoloStandard) {
		for(bolo=0; bolo<BoloNb; bolo++) if(SettingsDetecEvtsBoloStandardise(bolo)) change = 1;
	} else {
		change = SettingsDetecEvtsBoloStandardise(BoloNum);
		if(!change) change = SettingsDetecEvtsBoloChange(BoloNum,(panel->cdr)->var_modifiees);
	}
	if(change) {
		SettingsDetecEvtsCree(panel,0);
		MenuItemAllume(mSettingsDetecEvts,1,0,GRF_RGB_YELLOW);
	}
	return(0);
}
/* ========================================================================== */
static int SettingsDetecEvtsQuit(Cadre cdr, void *arg) {
	int bolo;

//	PanelApply((Panel)cdr->adrs,1);
	if(cdr->var_modifiees) {
		if(BoloNum == BoloStandard) for(bolo=0; bolo<BoloNb; bolo++) {
			for(bolo=0; bolo<BoloNb; bolo++) if(SettingsDetecEvtsBoloStandardise(bolo)) SettingsDetecEvtsBoloChange(bolo,1);
		} else if((bolo = (int)arg) >= 0) SettingsDetecEvtsBoloChange(bolo,0);
	}
	return(0);
}
/*!
 @function
 @@abstract Changement de fenetre de definition des evenements
 @param cdr Cadre contenant la planche de definition
 @param arg Inutilise
 @warning Appelee directement en cas de changement de valeurs
 @return 0 pour assurer la continuite
 */

/* ========================================================================== */
int SettingsDetecEvtsCree(Panel appelant, void *v2) {
	int bolo,voie,cap,vm;
	int x,y,i,n,cols;
	char prems,affiche,actif;
	char *nom;
//	int xmax,l,n,c;
//	Instrum instrum;
	Panel p; Info info; void *elt;
	
	bolo = BoloNum;
	// printf("(%s) test affichage planche @%08X, depuis panel @%08X\n",__func__,(hexa)bDefEvts,(hexa)appelant);
	if((affiche = OpiumDisplayed(bDefEvts))) {
		// printf("(%s) planche %s\n",__func__,affiche?"affichee":"effacee");
		MenuItemSetContext(mSettingsDetecEvts,1,0); // MenuItemSetContext(mSettingsDetecEvts,2,0);
		// printf("(%s) menu mSettingsDetecEvts nettoye %s\n",__func__);
		OpiumClear(bDefEvts);
		// printf("(%s) planche %s\n",__func__,OpiumDisplayed(bDefEvts)?"affichee":"effacee");
	}
	BoardDismount(bDefEvts);
//+	printf("(%s) planche demontee\n",__func__);
	for(i=0; i<SettingsEvtsNb; i++) {
		elt = (void *)(SettingsEvts[i].instrum);
		switch(SettingsEvts[i].type) {
			case OPIUM_MENU:    MenuDelete((Menu)elt); break;
			case OPIUM_PANEL:   PanelDelete((Panel)elt); break;
			case OPIUM_TERM:    TermDelete((Term)elt); break;
			case OPIUM_GRAPH:   GraphDelete((Graph)elt); break;
			case OPIUM_INFO:    InfoDelete((Info)elt); break;
			case OPIUM_PROMPT:  break;
			case OPIUM_BOARD:   BoardTrash((Cadre)elt); BoardDelete((Cadre)elt); break;  /* ne supprime pas les composants */
			case OPIUM_INSTRUM: InstrumDelete((Instrum)elt); break;
		}
	}
//+	printf("(%s) appareils supprimes\n",__func__);
	SettingsEvtsNb = 0; p = 0;

	x = Dy; y = Dy;
	if(BoloNb > 1) { BoardAddPanel(bDefEvts,pSettingsDetecEvts,x,y,0); y += (3 * Dy); }
	BoardAddMenu(bDefEvts,mSettingsDetecEvts,x,y,0); y += 3 * Dy;
//+	printf("(%s) menu mSettingsDetecEvts ajoute\n",__func__);
	OpiumSupport(mSettingsDetecEvts->cdr,WND_PLAQUETTE);
	// if(bDefEvts->var_modifiees) MenuItemAllume(mSettingsDetecEvts,2,0,GRF_RGB_YELLOW);
	if(Bolo[bolo].a_sauver) MenuItemAllume(mSettingsDetecEvts,1,0,GRF_RGB_YELLOW);
	if(ConfigNb > 1) {
		p = PanelCreate(1); PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
		i = PanelList(p,"Configuration",ConfigListe,&ConfigNum,CONFIG_NOM);
		PanelItemColors(p,i,SambaVertJauneOrangeRouge,4); PanelItemSouligne(p,i,0);
		PanelItemReturn(p,i,SambaConfigChange,0);
		BoardAddPanel(bDefEvts,p,OPIUM_A_DROITE_DE mSettingsDetecEvts->cdr);
//+		printf("(%s) panel Configuration ajoute\n",__func__);
		SettingsEvts[SettingsEvtsNb].instrum = (void *)p;
		SettingsEvts[SettingsEvtsNb].type = OPIUM_PANEL;
		SettingsEvtsNb++;
		if(SettingsEvtsNb >= SETTINGS_EVTS_MAX) goto hev;
		y += 2 * Dy;
	}

	if(bolo == BoloStandard) {
		int nb;
		if(!(nb = DetecteurVoieModelesActifs())) return(0);
#ifdef RL
		p = PanelCreate(25 * nb);
#else
		p = PanelCreate(19 * nb);
#endif
		PanelColumns(p,nb);
		prems = 1;
		for(vm=0; vm<ModeleVoieNb; vm++) if(ModeleVoie[vm].utilisee) {
			PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,ModeleVoie[vm].nom,11),0),1);
			PanelFloat(p,prems?L_("Duree (ms)"):0,&(VoieStandard[vm].def.evt.duree));
			PanelFloat(p,prems?L_("Pre-trigger (%)"):0,&(VoieStandard[vm].def.evt.pretrigger));
		i = PanelFloat(p,prems?L_("Temps mort (ms)"):0,&(VoieStandard[vm].def.evt.tempsmort)); PanelItemSouligne(p,i,1);
			PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,L_("Analyse"),11),0),1);
			PanelFloat(p,prems?L_("Debut base (% pretrig)"):0,&(VoieStandard[vm].def.evt.base_dep));
			PanelFloat(p,prems?L_("Fin base (% pretrig)"):0,&(VoieStandard[vm].def.evt.base_arr));
			PanelFloat(p,prems?L_("Debut montee (% ampl.)"):0,&(VoieStandard[vm].def.evt.ampl_10));
		i = PanelFloat(p,prems?L_("Fin montee (% ampl.)"):0,&(VoieStandard[vm].def.evt.ampl_90)); PanelItemSouligne(p,i,1);
			PanelFloat(p,prems?L_("Decalage (ms)"):0,&(VoieStandard[vm].def.evt.delai));
		i = PanelFloat(p,prems?L_("Fenetre recherche (ms)"):0,&(VoieStandard[vm].def.evt.interv)); PanelItemSouligne(p,i,1);
		#ifdef RL
			PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,L_("Template Evt"),11),0),1);
			PanelInt    (p,prems?L_("Dim. template (points)"):0,&(VoieStandard[vm].def.evt.dimtemplate));
			PanelFloat  (p,prems?L_("Montee (millisec)"):0,&(VoieStandard[vm].def.evt.montee));
			PanelFloat  (p,prems?L_("Descente1 (millisec)"):0,&(VoieStandard[vm].def.evt.descente1));
			PanelFloat  (p,prems?L_("Descente2 (millisec)"):0,&(VoieStandard[vm].def.evt.descente2));
		i = PanelFloat  (p,prems?L_("Facteur2"):0,&(VoieStandard[vm].def.evt.facteur2)); PanelItemSouligne(p,i,1);
		#endif
//				PanelFloat(p,prems?"Coincidence (ms)":0,&(VoieStandard[vm].def.evt.coinc));
			PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,L_("Integrales"),11),0),1);
			PanelFloat(p,prems?L_("Debut rapide (ms)"):0,&(VoieStandard[vm].def.evt.phase[0].t0));
		i = PanelFloat(p,prems?L_("Duree rapide (ms)"):0,&(VoieStandard[vm].def.evt.phase[0].dt)); PanelItemSouligne(p,i,1);
			PanelFloat(p,prems?L_("Debut longue (ms)"):0,&(VoieStandard[vm].def.evt.phase[1].t0));
		i = PanelFloat(p,prems?L_("Duree longue (ms)"):0,&(VoieStandard[vm].def.evt.phase[1].dt)); PanelItemSouligne(p,i,1);
			PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"Calibration",11),0),1);
			PanelInt  (p,prems?L_("Min evt moyen (ADU)"):0,&(VoieStandard[vm].def.evt.min_moyen));
			PanelInt  (p,prems?L_("Max evt moyen (ADU)"):0,&(VoieStandard[vm].def.evt.max_moyen));
			prems = 0;
		}
		PanelOnApply(p,SettingsDetecEvtsChange,0);

	} else if(Bolo[bolo].local) {
		cols = 0; for(cap=0; cap<Bolo[bolo].nbcapt; cap++) if(Bolo[bolo].captr[cap].voie >= 0) cols++;
		if(cols) {
			n = 17; 
	#ifdef RL
			n += 6;
	#endif
			if(BoloNb > 1) n += 1;
			if(VoiesNb > 1) n += 2;
			if(ConfigNb > 1) n += 1;
			p = PanelCreate(n * cols); 
			PanelColumns(p,cols);
			prems = 1;
			for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
				voie = Bolo[bolo].captr[cap].voie;
				if(voie < 0) continue;
				if(Bolo[bolo].captr[cap].nom[0]) nom = Bolo[bolo].captr[cap].nom;
				else { vm = VoieManip[voie].type; nom = ModeleVoie[vm].nom; }
				// if(!VoieTampon[voie].lue) continue; defini dans satellite et non superviseur
				actif = !VoieManip[voie].def.evt.std;
				PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,nom,11),0),1); // PanelRemark(p,ModeleVoie[vm].nom); joli... mais ne marche pas pour la 1ere colonne!!
				if(BoloNb > 1) PanelItemExit  (p,PanelKeyB (p,prems?"Definition":0,L_("particuliere/standard"),&(VoieManip[voie].def.evt.std),13),SettingsDetecEvtStandardise1,(void *)(long)cap);
				if(ConfigNb > 1) {
					// printf("(%s) Config globale#%d (%s)\n",__func__,config->global.trmt[classe],ConfigCle[config->global.trmt[classe]]);
				i = PanelItemSelect(p,PanelListB(p,"Configuration",ConfigCle,&(VoieManip[voie].def.global.evt),9),actif);
					PanelItemColors(p,i,SambaRougeVertJauneOrange,3); PanelItemSouligne(p,i,0);
				}
				PanelItemSelect(p,PanelFloat(p,prems?L_("Duree (ms)"):0,&(VoieManip[voie].def.evt.duree)),actif);
				PanelItemSelect(p,PanelFloat(p,prems?L_("Pre-trigger (%)"):0,&(VoieManip[voie].def.evt.pretrigger)),actif);
				PanelItemSelect(p,PanelFloat(p,prems?L_("Temps mort (ms)"):0,&(VoieManip[voie].def.evt.tempsmort)),actif);
				PanelRemark(p,prems?"Caracterisation .":0);
				PanelItemSelect(p,PanelFloat(p,prems?L_("Debut base (% pretrig)"):0,&(VoieManip[voie].def.evt.base_dep)),actif);
			i = PanelItemSelect(p,PanelFloat(p,prems?L_("Fin base (% pretrig)"):0,&(VoieManip[voie].def.evt.base_arr)),actif); PanelItemSouligne(p,i,1);
				PanelItemSelect(p,PanelFloat(p,prems?L_("Debut montee (% ampl.)"):0,&(VoieManip[voie].def.evt.ampl_10)),actif);
			i = PanelItemSelect(p,PanelFloat(p,prems?L_("Fin montee (% ampl.)"):0,&(VoieManip[voie].def.evt.ampl_90)),actif); PanelItemSouligne(p,i,1);
				PanelItemSelect(p,PanelFloat(p,prems?L_("Fenetre recherche (ms)"):0,&(VoieManip[voie].def.evt.interv)),actif);
			if(VoiesNb > 1) {
				PanelItemSelect(p,PanelFloat(p,prems?L_("Decalage (ms)"):0,&(VoieManip[voie].def.evt.delai)),actif);
			}
			#ifdef RL
				PanelRemark(p,prems?L_("Template"):0);
				PanelItemSelect(p,PanelInt  (p,prems?L_("Dimension (points)"):0,&(VoieManip[voie].def.evt.dimtemplate)),actif);
				PanelItemSelect(p,PanelFloat(p,prems?L_("Montee (ms)"):0,&(VoieManip[voie].def.evt.montee)),actif);
				PanelItemSelect(p,PanelFloat(p,prems?L_("Descente1 (ms)"):0,&(VoieManip[voie].def.evt.descente1)),actif);
				PanelItemSelect(p,PanelFloat(p,prems?L_("Descente2 (ms)"):0,&(VoieManip[voie].def.evt.descente2)),actif);
				PanelItemSelect(p,PanelFloat(p,prems?L_("Facteur2"):0,&(VoieManip[voie].def.evt.facteur2)),actif);
			#endif
				PanelRemark(p,prems?L_("Integrales"):0);
				PanelItemSelect(p,PanelFloat(p,prems?L_("Debut rapide (ms)"):0,&(VoieManip[voie].def.evt.phase[0].t0)),actif);
			i = PanelItemSelect(p,PanelFloat(p,prems?L_("Duree rapide (ms)"):0,&(VoieManip[voie].def.evt.phase[0].dt)),actif); PanelItemSouligne(p,i,1);
				PanelItemSelect(p,PanelFloat(p,prems?L_("Debut longue (ms)"):0,&(VoieManip[voie].def.evt.phase[1].t0)),actif);
				PanelItemSelect(p,PanelFloat(p,prems?L_("Duree longue (ms)"):0,&(VoieManip[voie].def.evt.phase[1].dt)),actif);
				PanelRemark(p,prems?"Calibration":0);
				PanelItemSelect(p,PanelInt  (p,prems?L_("Min evt moyen (ADU)"):0,&(VoieManip[voie].def.evt.min_moyen)),actif);
				PanelItemSelect(p,PanelInt  (p,prems?L_("Max evt moyen (ADU)"):0,&(VoieManip[voie].def.evt.max_moyen)),actif);
				prems = 0;
			}
			PanelOnApply(p,SettingsDetecEvtsChange,0);
		} else {
			p = 0;
			InfoWrite(info = InfoCreate(1,40),1,L_("Aucune voie de ce detecteur n'est lue"));
			OpiumSupport(info->cdr,WND_CREUX);
			BoardAddInfo(bDefEvts,info,x,y,0);
			SettingsEvts[SettingsEvtsNb].instrum = (void *)info;
			SettingsEvts[SettingsEvtsNb].type = OPIUM_INFO;
			SettingsEvtsNb++;
			if(SettingsEvtsNb >= SETTINGS_EVTS_MAX) goto hev;
		}
	} else if(appelant) OpiumWarn(L_("Le detecteur %s n'est pas gere par %s"),Bolo[bolo].nom,Acquis[AcquisLocale].code);
	if(p) {
		PanelSupport(p,WND_CREUX);
		BoardAddPanel(bDefEvts,p,x,y,0);
		SettingsEvts[SettingsEvtsNb].instrum = (void *)p;
		SettingsEvts[SettingsEvtsNb].type = OPIUM_PANEL;
		SettingsEvtsNb++;
		if(SettingsEvtsNb >= SETTINGS_EVTS_MAX) goto hev;
//		printf("(%s) Cree instrum #%d de type %d @%08X (total: %d)\n",__func__,SettingsEvtsNb-1,OPIUM_PANEL,p,SettingsEvtsNb);
	}
	
hev:
//	OpiumSize(bDefEvts,x,y+260);
//	OpiumEnterFctn(bDefEvts,SettingsDetecEvtsCree,0); si on fait ca, pas d'affichage !!
	OpiumExitFctn(bDefEvts,SettingsDetecEvtsQuit,(void *)(long)BoloNum);
//?	if(appelant) { if(affiche) OpiumUse(bDefEvts,OPIUM_FORK); /* OpiumRefresh(p->cdr); */ }
	if(affiche) OpiumUse(bDefEvts,OPIUM_FORK);

	return(0);
}

/* .................... fonctions pour le traitement ........................ */
#pragma mark --- traitement ---
/* ========================================================================== */
int SettingsDetecTrmtCree(Panel appelant, void *v2) {
	return(SettingsDetecCree(SETTINGS_TRMT,appelant,v2));
}
/* ========================================================================== */
static int SettingsDetecTrmtModifie(Panel panel, void *v2) {
	SettingsDetecDeclareModif(SETTINGS_TRMT,1); return(0);
}
/* ========================================================================== */
static int SettingsDetecTrmtVolStandardise(Panel panel, void *v2) {
	int bolo,cap,voie; char *nom;
	
	bolo = SettingsVolet[SETTINGS_TRMT].bolo;
	if(bolo == BoloStandard) return(0);
	cap = (int)v2;
	if((voie = Bolo[bolo].captr[cap].voie) < 0) return(0);
	if((nom = DetecteurCanalStandardise(DETEC_PARM_TVOL,bolo,cap,-1))) {
		printf(". Reglages de traitement au vol %s standard reportes dans la voie %s\n",VoieStandard[VoieManip[voie].type].def.evt.nom,VoieManip[voie].nom);
	} else printf(". Reglages de traitement au vol particuliers pour la voie %s\n",VoieManip[voie].nom);
	/* if(Bolo[bolo].a_sauver) */ SettingsDetecDeclareModif(SETTINGS_TRMT,1);
	SettingsDetecCree(SETTINGS_TRMT,panel,v2);

	return(0);
}
/* ========================================================================== */
static int SettingsDetecTrmtTpnStandardise(Panel panel, void *v2) {
	int bolo,cap,voie; char *nom;
	
	bolo = SettingsVolet[SETTINGS_TRMT].bolo;
	if(bolo == BoloStandard) return(0);
	cap = (int)v2;
	if((voie = Bolo[bolo].captr[cap].voie) < 0) return(0);
	if((nom = DetecteurCanalStandardise(DETEC_PARM_PREP,bolo,cap,-1))) {
		printf(". Reglages de traitement pre-trigger %s standard reportes dans la voie %s\n",VoieStandard[VoieManip[voie].type].def.evt.nom,VoieManip[voie].nom);
	} else printf(". Reglages de traitement pre-trigger particuliers pour la voie %s\n",VoieManip[voie].nom);
	/* if(Bolo[bolo].a_sauver) */ SettingsDetecDeclareModif(SETTINGS_TRMT,1);
	SettingsDetecCree(SETTINGS_TRMT,panel,v2);

	return(0);
}
/* ========================================================================== */
static void SettingsDetecTrmtVariable(Panel p, TypeTraitement *trmt, int classe, int num) {
	int k;

	PanelErase(p);
	switch(trmt[classe].type) {
	  case NEANT: break;
	  case TRMT_DEMODUL:
		// strcpy(SettingsTrmtTexte[classe][num],L_("Marge demodulation"));
		// PanelInt(p,SettingsTrmtTexte[classe][num],&(trmt[classe].p1));
		PanelInt(p,L_("Marge demodulation"),&(trmt[classe].p1));
		break;
#ifdef TRMT_ADDITIONNELS
	  case TRMT_LISSAGE:
	  case TRMT_MOYENNE:
	  case TRMT_MAXIMUM:
		// strcpy(SettingsTrmtTexte[classe][num],L_("Largeur"));
		// PanelInt(p,SettingsTrmtTexte[classe][num],&(trmt[classe].p1));
		PanelInt(p,L_("Longueur"),&(trmt[classe].p1));
		break;
	#ifdef TRMT_DECONVOLUE
	  case TRMT_DECONV:
		PanelInt(p,L_("lissage avant"),&(trmt[classe].p1));
		PanelInt(p,L_("lissage apres"),&(trmt[classe].p2));
		PanelFloat(p,"RC (ms)",&(trmt[classe].p3));
		break;
	#endif
	#ifdef A_LA_GG
	  case TRMT_INTEDIF:
		// strcpy(SettingsTrmtTexte[classe][num],L_("integ/diff"));
		// PanelText(p,SettingsTrmtTexte[classe][num],trmt[classe].texte,TRMT_TEXTE);
		//- PanelText(p,"integ/diff",trmt[classe].texte,TRMT_TEXTE);
		PanelInt(p,L_("Longueur integration"),&(trmt[classe].p1));
		PanelFloat(p,L_("Duree Montee (ms)"),&(trmt[classe].p3));
		break;
	#endif /* A_LA_GG */
	#ifdef A_LA_MARINIERE
	  case TRMT_CSNSM:
		if((trmt[classe].p1 < 0) || (trmt[classe].p1 >= TrmtCsnsmNb)) trmt[classe].p1 = 0;
		// strcpy(SettingsTrmtTexte[classe][num],L_("Filtre"));
		// PanelItemLngr(p,PanelList(p,SettingsTrmtTexte[classe][num],TrmtCsnsmNom,&(trmt[classe].p1),MAXFILTRENOM),16);
		PanelItemLngr(p,PanelList(p,L_("Filtre"),TrmtCsnsmNom,&(trmt[classe].p1),MAXFILTRENOM),16);
		break;
	#endif /* A_LA_MARINIERE */
#endif /* TRMT_ADDITIONNELS */
	  case TRMT_FILTRAGE:
		k = 0 ; while(FiltreListe[k][0]) k++;
		if((trmt[classe].p1 < 0) || (trmt[classe].p1 >= k)) trmt[classe].p1 = 0;
		// strcpy(SettingsTrmtTexte[classe][num],L_("Filtre"));
		// PanelItemLngr(p,PanelList(p,SettingsTrmtTexte[classe][num],FiltreListe,&(trmt[classe].p1),MAXFILTRENOM),16);
		PanelItemLngr(p,PanelList(p,L_("Filtre"),FiltreListe,&(trmt[classe].p1),MAXFILTRENOM),16);
		break;
	  case TRMT_INVALID: default: break;
	}
	PanelOnApply(p,SettingsDetecTrmtModifie,0);
	
	OpiumSizeChanged(p->cdr);
}
/* ========================================================================== */
static void SettingsDetecTrmtChange(Panel panel, void *v2, int classe) {
	STORE_CONFIG(SETTINGS_TRMT,trmt[classe],TrmtClasseNom[classe]);
	SettingsDetecDeclareModif(SETTINGS_TRMT,1);
	SettingsDetecTrmtCree(panel,v2);
}
/* ========================================================================== */
static int SettingsDetecTrmtVolChangeType(Panel panel, void *v2) {
	SettingsDetecTrmtChange(panel,v2,TRMT_AU_VOL);
	return(0);
}
/* ========================================================================== */
static int SettingsDetecTrmtTpnChangeType(Panel panel, void *v2) {
	SettingsDetecTrmtChange(panel,v2,TRMT_PRETRG);
	return(0);
}
/* ========================================================================== */
static void SettingsDetecTrmtDuree(int num, TypeVoieDurees *duree, char actif, int *x, int *y) {
	Panel panel1,panel2; int xtop,ytop; int m3,m4;

	xtop = *x + (2 * Dx); ytop = *y + Dy;
	BoardAreaOpen(L_("Parametres avances"),WND_RAINURES);

	panel1 = PanelCreate(4); PanelSupport(panel1,WND_CREUX);
	PanelInt  (panel1,L_("Dimension evaluation LdB"),&(duree->mesureLdB));
	PanelInt  (panel1,L_("Dimension traitement"),&(duree->traitees));
	PanelFloat(panel1,L_("Periodes pattern"),&(duree->periodes));
	PanelInt  (panel1,L_("Moyennage evt sauve"),&(duree->post_moyenne));
	m3 = PanelItemNb(panel1); if(actif) m3++; else PanelMode(panel1,PNL_DIRECT|PNL_RDONLY);
	PanelOnApply(panel1,SettingsDetecTrmtModifie,(void *)(long)num);
	BoardAddPanel(SettingsVolet[SETTINGS_TRMT].planche,panel1,xtop,ytop,0);
	ytop += ((m3 + 1) * Dy);

	if(SettingsDeconvPossible) {
		panel2 = PanelCreate(5); PanelSupport(panel2,WND_CREUX);
		PanelBool (panel2,L_("Deconvolution + charge"),&duree->deconv.calcule);
		PanelInt  (panel2,L_("Lissage avant"),&(duree->deconv.pre));
		PanelInt  (panel2,L_("Lissage apres"),&(duree->deconv.post));
		PanelFloat(panel2,L_("Temps de decroissance (ms)"),&(duree->deconv.rc));
		PanelFloat(panel2,L_("Seuil pour calcul charge"),&(duree->deconv.seuil));
		m4 = PanelItemNb(panel2); if(actif) m4++; else PanelMode(panel2,PNL_DIRECT|PNL_RDONLY);
		PanelOnApply(panel2,SettingsDetecTrmtModifie,(void *)(long)num);
		BoardAddPanel(SettingsVolet[SETTINGS_TRMT].planche,panel2,xtop,ytop,0);
		ytop += ((m4 + 1) * Dy);
	}

	BoardAreaMargesVert(0,-Dy/2);
	BoardAreaEnd(SettingsVolet[SETTINGS_TRMT].planche,x,y);
}
/* ========================================================================== */
static void SettingsDetecTrmt(int num, TypeConfigDefinition *config, int classe, char actif, int *x, int *y) {
	Panel panel1,panel2; int i,n,m1,m2; int xtop,ytop; char *titre;

	xtop = *x + (2 * Dx); ytop = *y + Dy;

	panel1 = PanelCreate(4); PanelSupport(panel1,WND_CREUX);
	switch(classe) {
		case TRMT_AU_VOL: titre = L_("Au vol"); break;
		case TRMT_PRETRG: titre = L_("Pre-trigger"); break;
		default: titre = 0; // compilo
	}
	if(ConfigNb > 1) {
		// printf("(%s) Config globale#%d (%s)\n",__func__,config->global.trmt[classe],ConfigCle[config->global.trmt[classe]]);
		i = PanelListB(panel1,"Configuration",ConfigCle,&(config->global.trmt[classe]),9);
		PanelItemColors(panel1,i,SambaRougeVertJauneOrange,3); PanelItemSouligne(panel1,i,0);
	}
	PanelListB(panel1,titre,TrmtTypeListe,&(config->trmt[classe].type),13);
	PanelInt  (panel1,L_("Gain logiciel"),&(config->trmt[classe].gain));
#ifdef RL
	if(classe == TRMT_PRETRG) PanelKeyB (panel1,"Template",TrmtTemplateCles,&(config->trmt[classe].template),13);
#endif
	if(actif) switch(classe) {
		case TRMT_AU_VOL: PanelOnApply(panel1,SettingsDetecTrmtVolChangeType,(void *)(long)num); break;
		case TRMT_PRETRG: PanelOnApply(panel1,SettingsDetecTrmtTpnChangeType,(void *)(long)num); break;
	}
	m1 = PanelItemNb(panel1); if(actif) m1++; else PanelMode(panel1,PNL_DIRECT|PNL_RDONLY);
	BoardAddPanel(SettingsVolet[SETTINGS_TRMT].planche,panel1,xtop,ytop,0);
	n = m1;

	xtop += (28 * Dx);
	/* pTrmt[classe][num] = */ panel2 = PanelCreate(3);
	SettingsDetecTrmtVariable(panel2,config->trmt,classe,num);
	PanelSupport(panel2,WND_CREUX);
	m2 = PanelItemNb(panel2); if((m2 > 0) && actif) m2++; else PanelMode(panel2,PNL_DIRECT|PNL_RDONLY);
	if(n < m2) n = m2;
	BoardAddPanel(SettingsVolet[SETTINGS_TRMT].planche,panel2,xtop,ytop,0);

	BoardAreaMargesVert(0,-Dy/2);
	BoardAreaEnd(SettingsVolet[SETTINGS_TRMT].planche,x,y);
	*x = xtop + (33 * Dx);
	*y = ytop + ((n + 1) * Dy);
}

/* ..................... fonctions pour le trigger .......................... */
#pragma mark --- trigger ---
/* ========================================================================== */
int SettingsDetecTrgrCree(Panel appelant, void *v2) {
	return(SettingsDetecCree(SETTINGS_TRGR,appelant,v2));
}
/* ========================================================================== */
static int SettingsDetecTrgrStandardise(Panel panel, void *v2) {
	int bolo,cap,voie; char *nom;
	
	bolo = SettingsVolet[SETTINGS_TRGR].bolo;
	if(bolo == BoloStandard) return(0);
	cap = (int)v2;
	if((voie = Bolo[bolo].captr[cap].voie) < 0) return(0);
	if((nom = DetecteurCanalStandardise(DETEC_PARM_TRGR,bolo,cap,-1))) {
		printf(". Reglages de trigger %s standard reportes dans la voie %s\n",VoieStandard[VoieManip[voie].type].def.evt.nom,VoieManip[voie].nom);
	} else printf(". Reglages de trigger particuliers pour la voie %s\n",VoieManip[voie].nom);
	/* if(Bolo[bolo].a_sauver) */ SettingsDetecDeclareModif(SETTINGS_TRGR,1);
	SettingsDetecCree(SETTINGS_TRGR,panel,v2);
	return(0);
}
/* ========================================================================== */
static int SettingsDetecTrgrChange(Panel panel, void *v2) {
	int bolo,cap,voie; char globale;
	TypeConfigVoie *config_voie; TypeConfigDefinition *utilisee,*config; int num_config;

	if(((cap = (int)v2) >= 0) && ((bolo = SettingsVolet[SETTINGS_TRGR].bolo) >= 0)) {
		if((bolo != BoloStandard) && ((voie = Bolo[bolo].captr[cap].voie) >= 0)) {
			if(GestionConfigs) printf("(%s)  ____ Change %s %s\n",__func__,"Trigger",VoieManip[voie].nom);
			utilisee = &(VoieManip[voie].def); globale = utilisee->global.trgr;
			utilisee->defini.trgr = 1;
			utilisee->defini.coup = 1;
			config_voie = &(VoieManip[voie].config);
			num_config = SettingsDetecConfigLocale(config_voie,voie,globale);
			config_voie->num[globale].trgr = num_config; config_voie->num[globale].coup = num_config;
			config = &(config_voie->def[num_config]);
			config->global.trgr = globale; config->global.coup = globale;
			if(GestionConfigs) printf("(%s) | %s voie %s: %s config #%d (%s)\n",__func__,"Trigger",VoieManip[voie].nom,
				(config->defini.trgr)?"recopie sur":"defini",num_config,config->nom);
			memcpy(&(config->trgr),&(utilisee->trgr),sizeof(TypeTrigger));
			config->defini.trgr = 1;
			if(GestionConfigs) printf("(%s) | %s voie %s: %s config #%d (%s)\n",__func__,"Coupures",VoieManip[voie].nom,
				(config->defini.coup)?"recopie sur":"defini",num_config,config->nom);
			memcpy(&(config->coup),&(utilisee->coup),sizeof(TypeCritereJeu));
			config->coup_std = utilisee->coup_std;
			config->defini.coup = 1;
			if(GestionConfigs) {
				printf("(%s) | %s voie %s: represente globale #%d (%s) donc config #%d (%s)\n",__func__,"Trigger",VoieManip[voie].nom,
					utilisee->global.trgr,ConfigCle[utilisee->global.trgr],num_config,config->nom);
				printf("(%s) | %s voie %s: represente globale #%d (%s) donc config #%d (%s)\n",__func__,"Coupures",VoieManip[voie].nom,
					utilisee->global.coup,ConfigCle[utilisee->global.coup],num_config,config->nom);
				printf("(%s) |____ Changement pris en compte\n",__func__);
			}
		}
	}
	SettingsDetecDeclareModif(SETTINGS_TRGR,1);
	return(0);
}
/* ========================================================================== */
static int SettingsDetecTrgrRefait(Panel panel, void *v2) {
	SettingsDetecTrgrChange(panel,v2);
	SettingsDetecTrgrCree(panel,v2);
	return(0);
}
#ifdef CRITERES_COUPURES_FIXES
/* ========================================================================== */
static int SettingsDetecTrgrRecoupe(Panel panel, void *v2) {
	SettingsDetecTrgrChange(panel,v2);
	if(v2) {
		int i,d,m3; TypeTrigger *trgr;
		trgr = (TypeTrigger *)v2;
		m3 = PanelItemNb(panel);
		d = SettingsCoupures? 1: 0;
		for(i=d; i<m3; i++) PanelItemSelect(panel,i+1,SettingsCoupures? trgr->coupures: 0);
		// deja demande: PanelOnApply(panel,SettingsDetecTrgrRecoupe,(void *)trgr);
		PanelRefreshVars(panel);
	}
	SettingsDetecDeclareModif(SETTINGS_TRGR,1);
	return(0);
}
#endif
/* ========================================================================== */
static void SettingsDetecTrgrVariable(Panel p, TypeTrigger *trgr) {

	PanelErase(p);
	switch(trgr->type) {
	  case TRGR_SEUIL:
		if(trgr->sens == 2) PanelFloat(p,L_("Seuil (minimum)"),&trgr->minampl);
		else if(trgr->sens == 0) PanelFloat(p,L_("Seuil (maximum)"),&trgr->maxampl);
		else {
			PanelFloat(p,L_("Seuil pour positifs"),&trgr->minampl);
			PanelFloat(p,L_("Seuil pour negatifs"),&trgr->maxampl);
		}
		PanelKeyB(p,L_("Temps mort"),TRGR_REPRISE_CLES,&trgr->reprise,14);
		break;
	  case TRGR_FRONT:
		PanelFloat(p,L_("Amplitude minimum"),&trgr->minampl);
		PanelFloat(p,L_("Montee mini (ms)"),&trgr->montee);
		break;
	  case TRGR_DERIVEE:
		if(trgr->sens == 2) PanelFloat(p,"Minimum",&trgr->minampl);
		else if(trgr->sens == 0) PanelFloat(p,"Maximum",&trgr->maxampl);
		else {
			PanelFloat(p,L_("Minimum positifs"),&trgr->minampl);
			PanelFloat(p,L_("Maximum negatifs"),&trgr->maxampl);
		}
		PanelFloat(p,L_("Duree Montee (ms)"),&trgr->montee);
		PanelKeyB(p,L_("Temps mort"),TRGR_REPRISE_CLES,&trgr->reprise,14);
		break;
	  case TRGR_ALEAT:
		PanelFloat(p,L_("Taux (Hz)"),&trgr->porte);
		// PanelFloat(p,"Excursion (pcent)",&trgr->fluct);
		break;
	  case TRGR_PORTE:
		PanelFloat(p,L_("Amplitude minimum"),&trgr->minampl);
		PanelFloat(p,L_("Duree mini (ms)"),&trgr->porte);
		break;
	  case TRGR_TEST:
		break;
	}

	PanelOnApply(p,SettingsDetecTrgrChange,0);
	OpiumSizeChanged(p->cdr);
}
/* ========================================================================== */
static void SettingsDetecTrgr(int num, TypeConfigDefinition *config, char actif, int *x, int *y) {
	Panel panel,panel1,panel2; int m1,m2,m3,n,i,d; int xm,ym; TypeTrigger *trgr; /* num: rang de la voie ou du modele */

	trgr = &(config->trgr);
	*x += (2 * Dx); *y += Dy;

	panel1 = PanelCreate(4);
	if(ConfigNb > 1) {
		// printf("(%s) Config globale#%d (%s)\n",__func__,config->global.trmt[classe],ConfigCle[config->global.trmt[classe]]);
		i = PanelListB(panel1,"Configuration",ConfigCle,&(config->global.trgr),9);
		PanelItemColors(panel1,i,SambaRougeVertJauneOrange,3); PanelItemSouligne(panel1,i,0);
	}
	PanelListB(panel1,"Trigger",TrgrTypeNoms,&trgr->type,13);
	PanelKeyB (panel1,L_("Sens du pulse"),L_(TRGR_SENS_CLES),&trgr->sens,12);
	PanelListB(panel1,L_("Origine"),TrgrOrigineNoms,&trgr->origine,12);
	if(actif) PanelOnApply(panel1,SettingsDetecTrgrRefait,(void *)(long)num);
	PanelSupport(panel1,WND_CREUX);
	m1 = PanelItemNb(panel1); if(actif) m1++; else PanelMode(panel1,PNL_DIRECT|PNL_RDONLY);
	
	/* pTrgr[num] = */ panel2 = PanelCreate(4);
	// OpiumSize(panel2->cdr,150,WndLigToPix(6));
	SettingsDetecTrgrVariable(panel2,trgr);
	PanelSupport(panel2,WND_CREUX);
	m2 = PanelItemNb(panel2); if(m2) { if(actif) m2++; else PanelMode(panel2,PNL_DIRECT|PNL_RDONLY); }
	
//	BoardAreaOpen("Detection",WND_RAINURES);
	BoardAddPanel(SettingsVolet[SETTINGS_TRGR].planche,panel1,*x,*y,0);
	if(m2) BoardAddPanel(SettingsVolet[SETTINGS_TRGR].planche,panel2,*x,*y+((m1+1)*Dy),0);
	BoardAreaMargesVert(0,-Dy/2);
	BoardAreaEnd(SettingsVolet[SETTINGS_TRGR].planche,&xm,&ym);
	*x += (37 * Dx); // xm trop variable selon trigger en service

	if(SettingsCoupures) {
	#ifdef CRITERES_COUPURES_FIXES
		#ifdef C_ETAIT_MIEUX_AVANT
			panel = PanelCreate(8);
			if(SettingsCoupures) PanelKeyB (panel,L_("Activation"),L_(TRGR_COUPURE_CLES),&trgr->coupures,4);
			else PanelKeyB (panel,L_("Activation"),L_(TRGR_COUPURE_CLES),&SettingsCoupures,4); // forcement "non", mais il faut bien l'indiquer...
			PanelItemColors(panel,1,SambaVertJauneOrangeRouge,2);
			PanelBool (panel,L_("Fonction veto"),&trgr->veto); PanelItemColors(panel,2,SambaBleuOrangeVertRougeJaune,2);
			PanelFloat(panel,L_("Plancher ampl."),&trgr->condition.underflow);
			PanelFloat(panel,L_("Plafond  ampl."),&trgr->condition.overflow);
			PanelFloat(panel,L_("Montee mini (ms)"),&trgr->condition.montmin);
			PanelFloat(panel,L_("Montee maxi (ms)"),&trgr->condition.montmax);
			PanelFloat(panel,L_("Bruit max"),&trgr->condition.maxbruit);
			PanelFloat(panel,L_("Duree mini (ms)"),&trgr->porte);
			PanelSupport(panel,WND_CREUX);
			m3 = PanelItemNb(panel);
			if(actif) {
				d = SettingsCoupures? 1: 0;
				for(i=d; i<m3; i++) PanelItemSelect(panel,i+1,SettingsCoupures? trgr->coupures: 0);
				m3++;
				PanelOnApply(panel,SettingsDetecTrgrRecoupe,trgr);
			} else PanelMode(panel,PNL_DIRECT|PNL_RDONLY);
		#else  /* C_ETAIT_MIEUX_AVANT */
			panel = PanelCreate(12); PanelColumns(panel,3); PanelMode(panel,PNL_BYLINES); PanelSupport(panel,WND_CREUX);
			if(SettingsCoupures) PanelKeyB (panel,L_("Activation"),L_(TRGR_COUPURE_CLES),&trgr->coupures,8);
			else PanelKeyB (panel,L_("Activation"),L_(TRGR_COUPURE_CLES),&SettingsCoupures,4); // forcement "non", mais il faut bien l'indiquer...
			PanelItemColors(panel,1,SambaVertJauneOrangeRouge,2);
			PanelBool (panel,L_("Fonction veto"),&trgr->veto); PanelItemColors(panel,2,SambaBleuOrangeVertRougeJaune,2);
			PanelBlank(panel);
			PanelFloat(panel,L_("Amplitude: si >="),&trgr->condition.underflow);
			PanelFloat(panel,L_("et si <="),&trgr->condition.overflow);
			i = PanelKeyB(panel,":",L_("passe/rejet"),&trgr->condition.ampl_inv,7); PanelItemColors(panel,i,SambaVertRouge,2);
			PanelFloat(panel,L_("Montee (ms): si >="),&trgr->condition.montmin);
			PanelFloat(panel,L_("et si <="),&trgr->condition.montmax);
			i = PanelKeyB(panel,":",L_("passe/rejet"),&trgr->condition.mont_inv,7); PanelItemColors(panel,i,SambaVertRouge,2);
			PanelFloat(panel,L_("Duree mini (ms)"),&trgr->porte);
			PanelFloat(panel,L_("Bruit max"),&trgr->condition.maxbruit);
			i = PanelKeyB(panel,":",L_("passe/rejet"),&trgr->condition.bruit_inv,7); PanelItemColors(panel,i,SambaVertRouge,2);
			m3 = PanelItemNb(panel);
			if(actif) {
				d = SettingsCoupures? 1: 0;
				for(i=d; i<m3; i++) PanelItemSelect(panel,i+1,SettingsCoupures? trgr->coupures: 0);
				PanelOnApply(panel,SettingsDetecTrgrRecoupe,trgr);
				m3 = (m3 / 3) + 1;
			} else { PanelMode(panel,PNL_DIRECT|PNL_RDONLY); m3 = (m3 / 3); }

		#endif /* C_ETAIT_MIEUX_AVANT */
	#else  /* !CRITERES_COUPURES_FIXES */
		TypeCritereJeu *coup; int k; char titre[80];
		coup = &(config->coup);
		if(GestionConfigs) printf("(%s) Jeu de coupures utilisee: %s (%d variables)\n",__func__,config->coup_std?"standard":"particulier",config->coup.nbvars);
		panel = PanelCreate(3 * (coup->nbvars + 1)); PanelColumns(panel,3); PanelMode(panel,PNL_BYLINES); PanelSupport(panel,WND_CREUX);
		if(SettingsCoupures) PanelKeyB (panel,L_("Activation"),L_(TRGR_COUPURE_CLES),&trgr->coupures,8);
		else PanelKeyB (panel,L_("Activation"),L_(TRGR_COUPURE_CLES),&SettingsCoupures,4); // forcement "non", mais il faut bien l'indiquer...
		PanelItemColors(panel,1,SambaVertJauneOrangeRouge,2);
		PanelBool (panel,L_("Fonction veto"),&trgr->veto); PanelItemColors(panel,2,SambaBleuOrangeVertRougeJaune,2);
		PanelBlank(panel);
		for(k=0; k<coup->nbvars; k++) {
			snprintf(titre,80,L_("%s: si >="),VarName[coup->var[k].ntuple]);
			PanelFloat(panel,titre,&(coup->var[k].min));
			PanelFloat(panel,L_("et si <="),&(coup->var[k].max));
			i = PanelKeyB(panel,":",L_(TRGR_COUPURE_ACTION),(char *)&(coup->var[k].inverse),7); PanelItemColors(panel,i,SambaVertRouge,2);
		}
		m3 = PanelItemNb(panel);
		if(actif) {
			d = SettingsCoupures? 1: 0;
			for(i=d; i<m3; i++) PanelItemSelect(panel,i+1,SettingsCoupures? trgr->coupures: 0);
			PanelOnApply(panel,SettingsDetecTrgrRefait,(void *)(long)num);
			m3 = (m3 / 3) + 1;
		} else { PanelMode(panel,PNL_DIRECT|PNL_RDONLY); m3 = (m3 / 3); }
	#endif /* !CRITERES_COUPURES_FIXES */

		BoardAreaOpen(L_("Coupures"),WND_RAINURES);
		BoardAddPanel(SettingsVolet[SETTINGS_TRGR].planche,panel,*x,*y,0);
		BoardAreaMargesVert(0,-Dy/2);
		BoardAreaEnd(SettingsVolet[SETTINGS_TRGR].planche,&xm,&ym);
	} else m3 = 0;

	m2 += m1;
	n = (m2 > m3)? m2: m3;
	*y += ((n + 1) * Dy);
}
/* ========================================================================== */
static void SettingsDetecTrgrConnecte(int voie, TypeTrigger *trgr, int *x, int *y, Cadre ref) {
	Panel panel; int num;
	
	if(voie >= 0) num = VoieManip[voie].cap; else num = -1;
	if(num >= 0) BoardAreaOpen(L_("Trigger externe"),WND_RAINURES);
	if(BoloNb > 1) {
		int n;
		panel = PanelCreate(6); PanelColumns(panel,2); PanelSupport(panel,WND_CREUX);
		PanelBool(panel,"Trigger OUT",&trgr->conn.out);
		PanelFloat(panel,L_("differe de (ms)"),&trgr->conn.delai);
		PanelBlank(panel);
		DetecteurListeCapteurs(0,0,0);
		PanelBool(panel,"Trigger IN",&trgr->conn.in);
		n = PanelList(panel,L_("depuis"),BoloNom,&trgr->conn.bolo,DETEC_NOM);
		PanelList(panel,L_("capteur"),CapteurNom,&trgr->conn.cap,MODL_NOM);
		PanelItemExit(panel,n,DetecteurListeCapteurs,0);
	} else if(Bolo[BoloNum].nbcapt > 1) {
		panel = PanelCreate(4); PanelColumns(panel,2); PanelSupport(panel,WND_CREUX);
		PanelBool(panel,"Trigger OUT",&trgr->conn.out);
		PanelFloat(panel,L_("differe de (ms)"),&trgr->conn.delai);
		PanelBool(panel,"Trigger IN",&trgr->conn.in);
		DetecteurListeCapteurs(0,0,0);
		PanelList(panel,L_("depuis"),CapteurNom,&trgr->conn.cap,MODL_NOM);
	} else return;
	PanelOnApply(panel,SettingsDetecTrgrChange,(void *)(long)voie);
	BoardAddPanel(SettingsVolet[SETTINGS_TRGR].planche,panel,*x,*y,ref);
	if(num >= 0) BoardAreaEnd(SettingsVolet[SETTINGS_TRGR].planche,x,y);
}
/* ========================================================================== */
static void SettingsDetecTrgrSupplement(int voie, TypeConfigDefinition *config, int *x, int *y) {
	Panel panel; Instrum instrum; TypeTrigger *trgr; int num,xt,yt;

	trgr = &(config->trgr);
	if(voie >= 0) num = VoieManip[voie].cap; else num = -1;
	xt = *x; yt = *y;
	if(num >= 0) BoardAreaOpen(L_("Supplements"),WND_RAINURES);
	if(SettingsCoupures) {
		panel = PanelCreate(5); PanelSupport(panel,WND_CREUX); // PanelMode(panel,PNL_DIRECT);
		PanelInt  (panel,L_("Evt long si ampl >"),&trgr->alphaampl);
		PanelFloat(panel,L_("Tps mort long (ms)"),&trgr->alphaduree);
		PanelKeyB(panel,L_("Evenement moyen"),L_("neant/calcul"),&(VoieTampon[voie].moyen.calcule),7);
		BoardAddPanel(SettingsVolet[SETTINGS_TRGR].planche,panel,xt,yt,0);
	} else {
		panel = PanelCreate(3); PanelSupport(panel,WND_CREUX); // PanelMode(panel,PNL_DIRECT);
		PanelInt  (panel,L_("Evt long si ampl >"),&trgr->alphaampl);
		PanelFloat(panel,L_("Tps mort long (ms)"),&trgr->alphaduree);
		BoardAddPanel(SettingsVolet[SETTINGS_TRGR].planche,panel,xt,yt - (7 * Dy),0);
		// instrum = InstrumKeyB(INS_LEVIER,(void *)&Levier,&(VoieTampon[voie].moyen.calcule),"neant/calcul");
		instrum = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&(VoieTampon[voie].moyen.calcule),L_("neant/calcul"));
		InstrumTitle(instrum,L_("evt moyen"));
		BoardAddInstrum(SettingsVolet[SETTINGS_TRGR].planche,instrum,OPIUM_A_DROITE_DE panel->cdr); // x-2*Dx,y-2*Dy
	}
	PanelOnApply(panel,SettingsDetecTrgrChange,(void *)(long)voie);
	if(num >= 0) BoardAreaEnd(SettingsVolet[SETTINGS_TRGR].planche,x,y);
	if(SettingsTrigInOut) {
		if(SettingsCoupures) { *x = xt + (36 * Dx); *y = yt; } else { *x = xt; *y = *y + (2 * Dy); }
		SettingsDetecTrgrConnecte(voie,trgr,x,y,0);
	}
}
/* .................... fonctions pour les coupures ......................... */
#ifdef SETTINGS_COUP
#pragma mark --- coupures ---
/* ========================================================================== */
int SettingsDetecCoupCree(Panel appelant, void *v2) {
	return(SettingsDetecCree(SETTINGS_COUP,appelant,v2));
}
/* ========================================================================== */
static int SettingsDetecCoupModifie(Panel panel, void *v2) {
	if(v2) {
		int i,m3; TypeTrigger *trgr;
		trgr = (TypeTrigger *)v2;
		m3 = PanelItemNb(panel);
		for(i=1; i<m3; i++) PanelItemSelect(panel,i+1,trgr->coupures);
		PanelRefreshVars(panel);
	}
	SettingsDetecDeclareModif(SETTINGS_COUP,1);
	return(0);
}
/* ========================================================================== */
static int SettingsDetecCoupStandardise(Panel panel, void *v2) {
	int bolo,cap,voie; char *nom;

	bolo = SettingsVolet[SETTINGS_COUP].bolo;
	if(bolo == BoloStandard) return(0);
	cap = (int)v2;
	if((voie = Bolo[bolo].captr[cap].voie) < 0) return(0);
	if((nom = DetecteurCanalStandardise(DETEC_PARM_COUP,bolo,cap,-1))) {
		printf(". Reglages de coupures %s standard reportes dans la voie %s\n",VoieStandard[VoieManip[voie].type].def.evt.nom,VoieManip[voie].nom);
	} else printf(". Reglages de coupures particuliers pour la voie %s\n",VoieManip[voie].nom);
	/* if(Bolo[bolo].a_sauver) */ SettingsDetecDeclareModif(SETTINGS_COUP,1);
	SettingsDetecCree(SETTINGS_COUP,panel,v2);
	return(0);
}
/* ========================================================================== */
static void SettingsDetecCoup(int num, TypeConfigDefinition *config, char actif, int *x, int *y) {
}
#endif

/* .................... fonctions pour la regulation ........................ */
#pragma mark --- regulation ---
/* ========================================================================== */
int SettingsDetecRgulCree(Panel appelant, void *v2) {
	return(SettingsDetecCree(SETTINGS_RGUL,appelant,v2));
}
/* ========================================================================== */
static int SettingsDetecRgulModifie(Panel panel, void *v2) {
	SettingsDetecDeclareModif(SETTINGS_RGUL,1);
	return(0);
}
/* ========================================================================== */
static int SettingsDetecRgulStandardise(Panel panel, void *v2) {
	int bolo,cap,voie; char *nom;
	
	bolo = SettingsVolet[SETTINGS_RGUL].bolo;
	if(bolo == BoloStandard) return(0);
	cap = (int)v2;
	if((voie = Bolo[bolo].captr[cap].voie) < 0) return(0);
	if((nom = DetecteurCanalStandardise(DETEC_PARM_RGUL,bolo,cap,-1))) {
		printf(". Reglages de regulation %s standard reportes dans la voie %s\n",VoieStandard[VoieManip[voie].type].def.evt.nom,VoieManip[voie].nom);
	} else printf(". Reglages de regulation particuliers pour la voie %s\n",VoieManip[voie].nom);
	/* if(Bolo[bolo].a_sauver) */ SettingsDetecDeclareModif(SETTINGS_RGUL,1);
	SettingsDetecCree(SETTINGS_RGUL,panel,v2);
	return(0);
}
/* ========================================================================== */
static void SettingsDetecRgulParms(Panel p, TypeRegulParms *rgul, TypeRegulTaux *echelle, TypeRegulInterv *interv) {
	int i,j;
	
	PanelErase(p);
	switch(rgul->type) {
	  case TRMT_REGUL_TAUX:
		PanelColumns(p,8);
		PanelMode(p,PNL_BYLINES);
		for(j=0; j<MAXECHELLES; j++) {
			i = PanelKeyB(p,0,L_("annulee/active"),&(echelle[j].active),8);
			PanelItemColors(p,i,SambaRougeVertJauneOrange,2);
			i = PanelInt (p,L_("Sur"),&(echelle[j].duree));
			PanelItemLngr(p,i,4); PanelItemFormat(p,i,"%-4d");
			i = PanelInt (p,L_("mn, si nb.evt <"),&(echelle[j].min.nb));
			PanelItemLngr(p,i,4);
			i = PanelKeyB(p,0,TrmtRegulTrad,&(echelle[j].min.action),13);
			i = PanelFloat(p,0,&(echelle[j].min.parm));
			PanelItemLngr(p,i,4);
			i = PanelInt (p,L_("; et si nb.evt >"),&(echelle[j].max.nb));
			PanelItemLngr(p,i,4);
			i = PanelKeyB(p,0,TrmtRegulTrad,&(echelle[j].max.action),13);
			i = PanelFloat(p,0,&(echelle[j].max.parm));
			PanelItemLngr(p,i,4);
		}
		break;
	  case TRMT_REGUL_INTERV:
		PanelColumns(p,2);
		PanelMode(p,0);
		PanelInt  (p,L_("Nombre d'intervalles"),&interv->nb);
		PanelFloat(p,L_("Longueur (s)"),&interv->duree);
		PanelFloat(p,L_("Ajustement seuil (ADU)"),&interv->ajuste);
		PanelFloat(p,L_("Facteur decision"),&interv->seuil);
		PanelInt  (p,L_("Delai maxi (nb.interv.)"),&interv->max);
		PanelFloat(p,L_("Increment si bruit (ADU)"),&interv->incr);
		break;
	}
	PanelSupport(p,WND_CREUX);
	OpiumSizeChanged(p->cdr);
	PanelOnApply(p,SettingsDetecRgulModifie,0);
}
/* ========================================================================== */
static void SettingsDetecRgulPlafs(Panel p, TypeRegulParms *rgul) {
	PanelErase(p);
	if(rgul->type) {
		PanelFloat(p,L_("Plafond  seuil min"),&(rgul->minsup));
		PanelFloat(p,L_("Plancher seuil min"),&(rgul->mininf));
		PanelFloat(p,L_("Plancher seuil max"),&(rgul->maxinf));
		PanelFloat(p,L_("Plafond  seuil max"),&(rgul->maxsup));
		OpiumSizeChanged(p->cdr);
	}
	PanelSupport(p,WND_CREUX);
	PanelOnApply(p,SettingsDetecRgulModifie,0);
	OpiumSizeChanged(p->cdr);
}
/* ========================================================================== 
static int SettingsDetecRgulVariable(int num, TypeRegulParms *rgul, TypeRegulTaux *echelle, TypeRegulInterv *interv) {
	SettingsDetecRgulParms(pRegulParms[num],rgul,echelle,interv);
	SettingsDetecRgulPlafs(pRegulPlafs[num],rgul);
	return(0);
}
   ========================================================================== */
static int SettingsDetecRgulChangeType(Panel panel, void *v2) {
#ifdef DEVELOPPE
	int bolo,cap,voie; char globale;
	TypeConfigVoie *config_voie; TypeConfigDefinition *utilisee,*config; int num_config;

	if((bolo = SettingsVolet[SETTINGS_RGUL].bolo) < 0) return(0);
	cap = (int)v2; if(cap < 0) return(0);
	if(bolo != BoloStandard) {
		if((voie = Bolo[bolo].captr[cap].voie) < 0) return(0);
		if(GestionConfigs) printf("(%s)  ____ Change %s %s\n",__func__,"regulation",VoieManip[voie].nom);
		utilisee = &(VoieManip[voie].def);
		globale = utilisee->global.rgul;
		config_voie = &(VoieManip[voie].config);
		for(num_config=0; num_config<config_voie->nb; num_config++) {
			if(!strcmp(config_voie->def[num_config].nom,ConfigCle[globale])) break;
		}
		if(num_config >= config_voie->nb) {
			if(GestionConfigs) printf("(%s) | Nouvelle configuration a definir: '%s'\n",__func__,ConfigCle[globale]);
			if(config_voie->nb < CONFIG_MAX) {
				num_config = config_voie->nb;
				strncpy(config_voie->def[num_config].nom,ConfigCle[globale],CONFIG_NOM);
				config_voie->nb += 1;
			} else {
				OpiumFail("! Deja %d configurations definies pour la voie %s. En supprimer une, avant nouvel ajout",CONFIG_MAX,VoieManip[voie].nom);
				return(0);
			}
		}
		config = &(config_voie->def[num_config]);
		if(GestionConfigs) printf("(%s) | %s voie %s: %s config #%d (%s)\n",__func__,"Regulation",VoieManip[voie].nom,
			(config->defini.rgul)?"recopie sur":"defini",num_config,config->nom);
		if(config->defini.rgul) {
			memcpy(&(config->rgul),&(utilisee->rgul),sizeof(TypeRegulParms));
		} else {
			config->defini.rgul = 1;
			config->global.rgul = globale;
			config_voie->num[globale].rgul = num_config;
		}
		if(GestionConfigs) printf("(%s) | %s voie %s: represente globale #%d (%s) donc config #%d (%s)\n",__func__,"Regulation",VoieManip[voie].nom,
								  utilisee->global.rgul,ConfigCle[utilisee->global.rgul],num_config,config->nom);
		if(GestionConfigs) printf("(%s) |____ Changement pris en compte\n",__func__);
	}
#else  /* DEVELOPPE */
	STORE_CONFIG(SETTINGS_RGUL,rgul,SettingsVoletTitre[SETTINGS_RGUL]);
#endif /* DEVELOPPE */
	SettingsDetecDeclareModif(SETTINGS_RGUL,1);
	SettingsDetecRgulCree(panel,v2);

	return(0);
}
/* ========================================================================== */
static void SettingsDetecRgul(int num, TypeConfigDefinition *config, TypeRegulTaux *echelle, TypeRegulInterv *interv,
	char actif, int *x, int *y) {
	Panel panel; int m1,m3,m4,i,n; int xm,ym;
	TypeRegulParms *rgul;
	
	rgul = &(config->rgul);
	*x += (2 * Dx); *y += Dy;
	
	panel = PanelCreate(2);
	if(ConfigNb > 1) {
		// printf("(%s) Config globale#%d (%s)\n",__func__,config->global.trmt[classe],ConfigCle[config->global.trmt[classe]]);
		i = PanelListB(panel,"Configuration",ConfigCle,&(config->global.rgul),9);
		PanelItemColors(panel,i,SambaRougeVertJauneOrange,3); PanelItemSouligne(panel,i,0);
	}
	PanelKeyB(panel,"Regulation",L_("neant/taux/intervalles"),&(rgul->type),12);
	if(actif) PanelOnApply(panel,SettingsDetecRgulChangeType,(void *)(long)num);
	PanelSupport(panel,WND_CREUX);
	m1 = PanelItemNb(panel); if(actif) m1++; else PanelMode(panel,PNL_DIRECT|PNL_RDONLY);
	BoardAddPanel(SettingsVolet[SETTINGS_RGUL].planche,panel,*x,*y,0);
	if(m1 < 2) m1 = 2; // si plusieurs voies, panel "standard/particuliere" a prendre en compte
	BoardAreaEnd(SettingsVolet[SETTINGS_RGUL].planche,&xm,&ym);

	/* pRegulParms[num] = */ panel = PanelCreate(8 * MAXECHELLES);
	SettingsDetecRgulParms(panel,rgul,echelle,interv);
	if(!actif) PanelMode(panel,PNL_DIRECT|PNL_RDONLY);
	if((m3 = PanelItemNb(panel))) {
		BoardAreaOpen(L_("Algorithme"),WND_RAINURES);
		BoardAddPanel(SettingsVolet[SETTINGS_RGUL].planche,panel,*x+(27*Dx),*y,0);
		BoardAreaMargesVert(0,-Dy/2);
		BoardAreaEnd(SettingsVolet[SETTINGS_RGUL].planche,&xm,&ym);
	}
	m3 /= 8; // 8 = nombre de colonnes (il faut maintenant le nombre de lignes, en fait)
	if(actif) m3++;
	if(m3 < m1) m3 = m1; // hauteur du 1er panel ici present, au minimum

	/* pRegulPlafs[num] = */ panel = PanelCreate(4);
	SettingsDetecRgulPlafs(panel,rgul);
	if(!actif) PanelMode(panel,PNL_DIRECT|PNL_RDONLY);
	if((m4 = PanelItemNb(panel))) {
		BoardAreaOpen(L_("Securites"),WND_RAINURES);
		BoardAddPanel(SettingsVolet[SETTINGS_RGUL].planche,panel,*x+(132*Dx),*y,0);
		BoardAreaMargesVert(0,-Dy/2);
		BoardAreaEnd(SettingsVolet[SETTINGS_RGUL].planche,&xm,&ym);
	}
	if(actif) m4++;

	n = (m3 > m4)? m3: m4;
	*x += (164 * Dx);
	*y += ((n + 0) * Dy);
}

/* ................... fonctions pour les categories ........................ */
#pragma mark --- categories ---
/* ========================================================================== */
int SettingsDetecCatgCree(Panel appelant, void *v2) {
	return(SettingsDetecCree(SETTINGS_CATG,appelant,v2));
}
/* ========================================================================== */
static int SettingsDetecCatgModifie(Panel panel, void *v2) {
	int bolo,cap,voie,vm,v,k,l; char ca_passe;
	TypeConfigVoie *config_voie; TypeConfigDefinition *utilisee;

	bolo = SettingsVolet[SETTINGS_CATG].bolo; cap = (int)v2;
	if((bolo >= 0) && (cap >= 0) && (cap < Bolo[bolo].nbcapt)) {
		voie = vm = v = 0; // GCC
		utilisee = 0;
		if(bolo == BoloStandard) {
			if((vm = Bolo[bolo].captr[cap].modele) >= 0) {
				utilisee = &(VoieStandard[vm].def); config_voie = &(VoieStandard[vm].config); v = vm;
			}
		} else {
			if((voie = Bolo[bolo].captr[cap].voie) >= 0) {
				utilisee = &(VoieManip[voie].def); config_voie = &(VoieManip[voie].config); v = voie;
			}
		}
		ca_passe = 1;
		for(k=0; k<utilisee->nbcatg; k++) {
			l = (int)strlen(utilisee->catg[k].nom); while(l--) { if(utilisee->catg[k].nom[l] != ' ') break; };
			utilisee->catg[k].nom[++l] = '\0';
			if(!strcmp(utilisee->catg[k].nom,CATG_ALL)) {
				sprintf(utilisee->catg[k].nom,"Categ%d",k+1);
				OpiumFail(L_("Le nom '%s' est reserve. Remplace par '%s'"),CATG_ALL,utilisee->catg[k].nom);
				ca_passe = 0;
			}
		}
		if(ca_passe) MonitTagsGlobal(v,utilisee);
	}
	SettingsDetecDeclareModif(SETTINGS_CATG,1);
	if(OpiumDisplayed(bMonitEcran)) MonitEcranPlanche();

	return(0);
}
/* ========================================================================== */
static int SettingsDetecCatgStandardise(Panel panel, void *v2) {
	int bolo,cap,voie; char *nom;

	bolo = SettingsVolet[SETTINGS_CATG].bolo; if(bolo == BoloStandard) return(0);
	cap = (int)v2; if((voie = Bolo[bolo].captr[cap].voie) < 0) return(0);
	if((nom = DetecteurCanalStandardise(DETEC_PARM_CATG,bolo,cap,-1))) {
		printf(". Definitions des categories pour les voies %s standard reportees dans la voie %s\n",VoieStandard[VoieManip[voie].type].def.evt.nom,VoieManip[voie].nom);
	} else printf(". Definitions des categories particulieres pour la voie %s\n",VoieManip[voie].nom);
	/* if(Bolo[bolo].a_sauver) */ SettingsDetecDeclareModif(SETTINGS_CATG,1);
	SettingsDetecCree(SETTINGS_CATG,panel,v2);
	return(0);
}
/* ========================================================================== */
static int SettingsDetecCatgAjoute(Menu menu, MenuItem item) {
	int bolo,cap,voie,vm,num_config; int k;
	TypeConfigVoie *config_voie; TypeConfigDefinition *utilisee,*exportee;
	TypeCritereJeu *critere_utilise; TypeVarJeu *jeu_utilisateur;
	char ggale; int nb_erreurs; int globale;

	if((bolo = SettingsVolet[SETTINGS_CATG].bolo) < 0) return(0);
	for(cap=0; cap<DETEC_CAPT_MAX; cap++) if(SettingsVolet[SETTINGS_CATG].menu_voie[cap] == menu) break;
	if(cap >= DETEC_CAPT_MAX) return(0);
	voie = vm = 0; // GCC
	if(bolo == BoloStandard) {
		if((vm = Bolo[bolo].captr[cap].modele) < 0) return(0);
		utilisee = &(VoieStandard[vm].def); config_voie = &(VoieStandard[vm].config);
	} else {
		if((voie = Bolo[bolo].captr[cap].voie) < 0) return(0);
		utilisee = &(VoieManip[voie].def); config_voie = &(VoieManip[voie].config);
	}
	nb_erreurs = 0;

	k = utilisee->nbcatg;
	snprintf(utilisee->catg[k].nom,CATG_NOM,"Classe%d",k+1);
#ifdef SANS_SELECTION
	if(k > 0) memcpy(&(utilisee->catg[k].condition),&(utilisee->catg[k-1].condition),sizeof(TypeEvtCriteres));
	else {
		utilisee->catg[k].condition.underflow = 0;
		utilisee->catg[k].condition.overflow = 100;
		utilisee->catg[k].condition.ampl_inv = 0;
		utilisee->catg[k].condition.montmin = 0.005;
		utilisee->catg[k].condition.montmax = 0.05;
		utilisee->catg[k].condition.mont_inv = 0;
		utilisee->catg[k].porte = 0;
		utilisee->catg[k].condition.maxbruit = 9999;
		utilisee->catg[k].condition.bruit_inv = 0;
	}
#else
	if(k > 0) memcpy(&(utilisee->catg[k].definition),&(utilisee->catg[k-1].definition),sizeof(TypeCritereJeu));
	else {
		jeu_utilisateur = &(VarSelection.classes);
		critere_utilise = &(utilisee->catg[k].definition);
		critere_utilise->catg = k;
		critere_utilise->nbvars = jeu_utilisateur->nbvars;
		int var; for(var=0; var<jeu_utilisateur->nbvars; var++) {
			critere_utilise->var[var].ntuple = jeu_utilisateur->ntuple[var];
			critere_utilise->var[var].min = 0.0;
			critere_utilise->var[var].max = 1.0;
			critere_utilise->var[var].inverse = 0;
		}
	}
#endif
	utilisee->nbcatg += 1;
	if(bolo != BoloStandard) utilisee->catg_std = 0;

	if(!k) { if(GestionConfigs) printf("  . Configuration %s pour classification de la voie %s\n",CONFIG_STD,VoieManip[voie].nom); }
	num_config = DetecConfigUtilisee(CONFIG_STD,1,voie,vm,&ggale,&cap,&nb_erreurs);
	globale = ggale + 1;
	utilisee->defini.catg = 1; utilisee->global.catg = globale;

	if(num_config >= 0) {
		config_voie->num[globale].catg = num_config;
		exportee = &(config_voie->def[num_config]);
		SambaConfigCopieCatg(exportee,utilisee);
	}
	SettingsDetecDeclareModif(SETTINGS_CATG,1);
	SettingsDetecCatgCree(0,(void *)(long)cap);

	return(0);
}
/* ========================================================================== */
static int SettingsDetecCatgRetire(Menu menu, MenuItem item) {
	int bolo,cap,voie,vm,num_config; int k;
	char *catg_definie[DETEC_CAPT_MAX+1];
	TypeConfigVoie *config_voie; TypeConfigDefinition *utilisee,*exportee;
	char ggale; int nb_erreurs;

	if((bolo = SettingsVolet[SETTINGS_CATG].bolo) < 0) return(0);
	for(cap=0; cap<DETEC_CAPT_MAX; cap++) if(SettingsVolet[SETTINGS_CATG].menu_voie[cap] == menu) break;
	if(cap >= DETEC_CAPT_MAX) return(0);
	voie = vm = 0; // GCC
	if(bolo == BoloStandard) {
		if((vm = Bolo[bolo].captr[cap].modele) < 0) return(0);
		utilisee = &(VoieStandard[vm].def); config_voie = &(VoieStandard[vm].config);
	} else {
		if((voie = Bolo[bolo].captr[cap].voie) < 0) return(0);
		utilisee = &(VoieManip[voie].def); config_voie = &(VoieManip[voie].config);
	}
	nb_erreurs = 0;

	for(k=0; k<utilisee->nbcatg; k++) catg_definie[k] = utilisee->catg[k].nom; catg_definie[k] = "\0";
	k = 0;
	if(OpiumReadList(L_("Retirer la classe"),catg_definie,&k,CATG_NOM) == PNL_CANCEL) return(0);
	utilisee->nbcatg -= 1;
	for( ;k<utilisee->nbcatg; k++) memcpy(&(utilisee->catg[k]),&(utilisee->catg[k+1]),sizeof(TypeCateg));
	if(bolo != BoloStandard) utilisee->catg_std = 0;

	num_config = DetecConfigUtilisee(CONFIG_STD,1,voie,vm,&ggale,&cap,&nb_erreurs);
	if(num_config >= 0) {
		exportee = &(config_voie->def[num_config]);
		SambaConfigCopieCatg(exportee,utilisee);
	}
	SettingsDetecDeclareModif(SETTINGS_CATG,1);
	SettingsDetecCatgCree(0,(void *)(long)cap);

	return(0);
}
/* ========================================================================== */
TypeMenuItem iSettingsCatg[] = {
	{ "Ajouter", MNU_FONCTION SettingsDetecCatgAjoute },
	{ "Retirer", MNU_FONCTION SettingsDetecCatgRetire },
	MNU_END
};
/* ========================================================================== */
static void SettingsDetecCatg(int num, TypeConfigDefinition *config, char actif, int *x, int *y) {
	int i,k;
	Menu menu; Panel panel; int nb; int ym,y0;

	ym = *y; y0 = ym;
	if(ConfigNb > 1) {
		panel = PanelCreate(1); PanelSupport(panel,WND_CREUX);
		i = PanelListB(panel,"Configuration",ConfigCle,&(config->global.catg),9);
		PanelItemColors(panel,i,SambaRougeVertJauneOrange,3); PanelItemSouligne(panel,i,0);
		nb = PanelItemNb(panel); if(actif) nb++; else PanelMode(panel,PNL_DIRECT|PNL_RDONLY);
		PanelOnApply(panel,SettingsDetecCatgModifie,0);
		BoardAddPanel(SettingsVolet[SETTINGS_CATG].planche,panel,*x,*y,0);
		ym += (nb * Dy);
	}
	// revoir la boucle sur les voies: BoardAreaOpen(0,0);
	if(GestionConfigs) printf("(%s) %d categories dans la configuration '%s'\n",__func__,config->nbcatg,config->nom);
	for(k=0; k<config->nbcatg; k++) {
		if(GestionConfigs) printf("(%s) %d variables dans la categorie '%s'\n",__func__,config->catg[k].definition.nbvars,config->catg[k].nom);
#ifdef SANS_SELECTION
		panel = PanelCreate(9); PanelColumns(panel,3); PanelMode(panel,PNL_BYLINES); PanelSupport(panel,WND_CREUX);
		PanelText(panel,L_("Nom"),config->catg[k].nom,CATG_NOM);
		PanelFloat(panel,L_("Bruit max"),&(config->catg[k].condition.maxbruit));
		i = PanelKeyB(panel,":",L_("accepte/exclue"),&(config->catg[k].condition.bruit_inv),8); PanelItemColors(panel,i,SambaVertRouge,2);
		PanelFloat(panel,L_("Amplitude: si >="),&(config->catg[k].condition.underflow));
		PanelFloat(panel,L_("et si <="),&(config->catg[k].condition.overflow));
		i = PanelKeyB(panel,":",L_("accepte/exclue"),&(config->catg[k].condition.ampl_inv),8); PanelItemColors(panel,i,SambaVertRouge,2);
		PanelFloat(panel,L_("Montee (ms): si >="),&(config->catg[k].condition.montmin));
		PanelFloat(panel,L_("et si <="),&(config->catg[k].condition.montmax));
		i = PanelKeyB(panel,":",L_("accepte/exclue"),&(config->catg[k].condition.mont_inv),8); PanelItemColors(panel,i,SambaVertRouge,2);
#else
		panel = PanelCreate(3 * (1 + config->catg[k].definition.nbvars)); PanelColumns(panel,3);
		PanelMode(panel,PNL_BYLINES); PanelSupport(panel,WND_CREUX);
		PanelText(panel,L_("Nom"),config->catg[k].nom,CATG_NOM); PanelBlank(panel); PanelBlank(panel);
		int num; for(num=0; num<config->catg[k].definition.nbvars ; num++) {
			char titre[80];
			snprintf(titre,80,L_("%s: si >="),VarName[config->catg[k].definition.var[num].ntuple]);
			PanelFloat(panel,titre,&(config->catg[k].definition.var[num].min));
			PanelFloat(panel,L_("et si <="),&(config->catg[k].definition.var[num].max));
			i = PanelKeyB(panel,":",L_("accepte/exclue"),(char *)&(config->catg[k].definition.var[num].inverse),8);
			PanelItemColors(panel,i,SambaVertRouge,2);
		}
#endif
		// PanelFloat(panel,L_("Duree mini (ms)"),&(config->catg[k].porte));
		PanelOnApply(panel,SettingsDetecCatgModifie,(void *)(long)num);
		nb = PanelItemNb(panel);
		BoardAddPanel(SettingsVolet[SETTINGS_CATG].planche,panel,*x,ym,0); ym += (nb/3 + 2) * Dy;
	}
	SettingsVolet[SETTINGS_CATG].menu_voie[num] = menu = MenuLocalise(iSettingsCatg);
	MenuColumns(menu,2); MenuSupport(menu,WND_PLAQUETTE);
	BoardAddMenu(SettingsVolet[SETTINGS_CATG].planche,menu,*x + (20 * Dx),ym,0); ym += Dy;
	*y = ym;
	// revoir la boucle sur les voies: BoardAreaCancel(SettingsVolet[SETTINGS_CATG].planche,x,y); *y = y0;

}

/* ......................... fonctions d'entree ............................. */
#pragma mark --- acces principal ---
#define TITRE_VOIE 12+VOIE_NOM
/* ========================================================================== */
int SettingsDetecCree(char volet, Panel appelant, void *v2) {
	int bolo,voie,cap,vm; char *adrs_std,*adrs_tpn;
	int i,x,y,xb,xs,ys,nm,np,nb,blocs;
	//	int xmax,larg,l,n,c;
	char affiche,actif,act2;
//	char *au_vol,*pre_trg;
	char *nomcible,nomaffiche[TITRE_VOIE];
	Menu menu; Panel panel;
#ifdef LIGNE_SEPARE_VOIES
	Figure fig;
#endif
	
	bolo = SettingsVolet[volet].bolo = BoloNum;
	// PRINT_GC(SettingsVolet[volet].general_det[0].gc);
	if((affiche = OpiumDisplayed(SettingsVolet[volet].planche))) OpiumClear(SettingsVolet[volet].planche);
	// PRINT_GC(SettingsVolet[volet].general_det[0].gc);
	BoardTrash(SettingsVolet[volet].planche);
	// PRINT_GC(SettingsVolet[volet].general_det[0].gc);
	act2 = 0; adrs_std = adrs_tpn = 0; // GCC
	
	x = 2 * Dx; y = 0;
	if(BoloNb > 1) {
		panel = PanelCreate(1); PanelSupport(panel,WND_CREUX);
		PanelList(panel,L_("Detecteur"),BoloNom,&BoloNum,DETEC_NOM);
		PanelOnApply(panel,SettingsVolet[volet].creation,0);
		BoardAddPanel(SettingsVolet[volet].planche,panel,x,y,0); x += ((15 + DETEC_NOM) * Dx);
		np = 2;
	} else np = 0;
	if(BoloNum == BoloStandard) menu = MenuLocalise(SettingsVolet[volet].general_std);
	else menu = MenuLocalise(SettingsVolet[volet].general_det);
	// PRINT_GC(SettingsVolet[volet].general_det[0].gc);
	OpiumSupport(menu->cdr,WND_PLAQUETTE);
	SettingsVolet[volet].general = menu;
	if(Bolo[SettingsVolet[volet].bolo].a_sauver) SettingsDetecDeclareModif(volet,0);
	// PRINT_GC(SettingsVolet[volet].general->items[0].gc);
	BoardAddMenu(SettingsVolet[volet].planche,menu,x,y,0);

	/* rappel de la configuration des detecteurs */
	if(ConfigNb > 1) {
		panel = PanelCreate(1); PanelMode(panel,PNL_DIRECT); PanelSupport(panel,WND_CREUX);
		SettingsVolet[volet].config = panel;
		i = PanelList(panel,"Configuration",ConfigListe,&ConfigNum,CONFIG_NOM);
		PanelItemColors(panel,i,SambaVertJauneOrangeRouge,4); PanelItemSouligne(panel,i,0);
		PanelItemReturn(panel,i,SambaConfigChange,0);
//		BoardAddPanel(SettingsVolet[volet].planche,panel,OPIUM_A_DROITE_DE OPIUM_PRECEDENT); // menu->cdr: adresse de <menu> volatile...
		BoardAddPanel(SettingsVolet[volet].planche,panel,x+(15*Dx),y,0);
		y += 2 * Dy;
	}

	nomcible = 0; nomaffiche[0] = '\0'; // GCC
	nm = MenuItemNb(menu); nb = (nm > np)? nm: np;
	y = (nb + 3) * Dy; blocs = 0;
#ifdef LIGNE_SEPARE_VOIES
	SettingsLigneHoriz.dx = 0;
#endif
	if(bolo == BoloStandard) {
		int nb;
		if(!(nb = DetecteurVoieModelesActifs())) return(0);
		for(vm=0; vm<ModeleVoieNb; vm++) if(ModeleVoie[vm].utilisee) blocs++;
		for(vm=0; vm<ModeleVoieNb; vm++) if(ModeleVoie[vm].utilisee) {
			nomcible = ModeleVoie[vm].nom;
		#ifdef LIGNE_SEPARE_VOIES
			x = 0;
			if(SettingsLigneHoriz.dx > 0) {
				fig = FigureCreate(FIG_DROITE,(void *)&SettingsLigneHoriz,0,0);
				BoardAddFigure(SettingsVolet[volet].planche,fig,x,y-Dy,0);
			}
			panel = PanelCreate(1); PanelMode(panel,PNL_RDONLY|PNL_DIRECT); PanelSupport(panel,WND_CREUX);
			// PanelItemSelect(panel,PanelText(panel,nomcible,SettingsTxtStd,13),0);
			PanelItemSelect(panel,PanelText(panel,0,nomcible,13),0);
			BoardAddPanel(SettingsVolet[volet].planche,panel,x,y,0);
			x += (26 * Dx);
		#else
			if(blocs > 1) {
				snprintf(nomaffiche,TITRE_VOIE,"%s %s",L_("Voies"),nomcible);
				x = Dx; BoardAreaOpen(nomaffiche,WND_RAINURES);
			} else x = 0;
		#endif
			
			switch(volet) {
			  case SETTINGS_TRMT:
				xs = x; ys = y;
				SettingsDetecTrmt(vm,&(VoieStandard[vm].def),TRMT_AU_VOL,1,&x,&y);
				SettingsDetecTrmtDuree(vm,&(VoieStandard[vm].duree),1,&x,&ys);
				x = xs;
				SettingsDetecTrmt(vm,&(VoieStandard[vm].def),TRMT_PRETRG,1,&x,&y);
				break;
			  case SETTINGS_TRGR: SettingsDetecTrgr(vm,&(VoieStandard[vm].def),1,&x,&y); break;
			#ifdef SETTINGS_COUP
			  case SETTINGS_COUP: SettingsDetecCoup(vm,&(VoieStandard[vm].def),1,&x,&y); break;
			#endif
			  case SETTINGS_RGUL: SettingsDetecRgul(vm,&(VoieStandard[vm].def),VoieStandard[vm].echelle,&(VoieStandard[vm].interv),1,&x,&y); break;
			  case SETTINGS_CATG: SettingsDetecCatg(vm,&(VoieStandard[vm].def),1,&x,&y); break;
			}
		#ifdef LIGNE_SEPARE_VOIES
			if(!SettingsLigneHoriz.dx) SettingsLigneHoriz.dx = x;
		#else
			if(blocs > 1) BoardAreaEnd(SettingsVolet[volet].planche,&x,&y);
		#endif
		}
	} else {
		for(cap=0; cap<Bolo[bolo].nbcapt; cap++) if((voie = Bolo[bolo].captr[cap].voie) >= 0) blocs++;
		if(!Bolo[bolo].local) {
			if(appelant) OpiumWarn("Le detecteur %s n'est pas gere par %s",Bolo[bolo].nom,Acquis[AcquisLocale].code);
		} else for(cap=0; cap<Bolo[bolo].nbcapt; cap++) if((voie = Bolo[bolo].captr[cap].voie) >= 0) {
			switch(volet) {
			  case SETTINGS_TRMT:
				adrs_std = &(VoieManip[voie].def.trmt[TRMT_AU_VOL].std);
				adrs_tpn = &(VoieManip[voie].def.trmt[TRMT_PRETRG].std);
				act2 = !(*adrs_tpn);
				break;
			  case SETTINGS_TRGR: adrs_std = &(VoieManip[voie].def.trgr.std); break;
			#ifdef SETTINGS_COUP
			  case SETTINGS_COUP: adrs_std = &(VoieManip[voie].def.coup_std); break;
			#endif
			  case SETTINGS_RGUL: adrs_std = &(VoieManip[voie].def.rgul.std); break;
			  case SETTINGS_CATG: adrs_std = &(VoieManip[voie].def.catg_std); break;
			}
			actif = !(*adrs_std);
			if(BoloNb > 1) {
				if(Bolo[bolo].captr[cap].nom[0]) nomcible = Bolo[bolo].captr[cap].nom;
				else { vm = VoieManip[voie].type; nomcible = ModeleVoie[vm].nom; }
			} else nomcible = VoieManip[voie].prenom;
		#ifdef LIGNE_SEPARE_VOIES
			x = 0;
			if(SettingsLigneHoriz.dx > 0) {
				fig = FigureCreate(FIG_DROITE,(void *)&SettingsLigneHoriz,0,0);
				BoardAddFigure(SettingsVolet[volet].planche,fig,0,y-Dy,0);
			}
		#else
			if(blocs > 1) {
				snprintf(nomaffiche,TITRE_VOIE,"%s %s",L_("Voie"),nomcible);
				x = 2 * Dx; BoardAreaOpen(nomaffiche,WND_PLAQUETTE);
				nomcible = "\0";
			} else x = 0;
		#endif
			switch(volet) {
				case SETTINGS_TRMT: BoardAreaOpen(L_("Traitement au vol"),WND_RAINURES); break;
				default: if(volet != SETTINGS_CATG) BoardAreaOpen("Definition",WND_RAINURES); break;
			}
			xb = x;
			if(BoloNb > 1) {
				panel = PanelCreate(1); PanelSupport(panel,WND_CREUX);
				PanelKeyB(panel,L_("Parametres"),L_("particuliers/standard"),adrs_std,13);
				PanelOnApply(panel,SettingsVolet[volet].standardise,(void *)(long)cap);
				BoardAddPanel(SettingsVolet[volet].planche,panel,x+Dx,y+Dy,0);
				x += (10 * Dx) /* pour "Parametres" */; x += (14 * Dx) /* pour "particuliers" */;
			}
			switch(volet) {
			  case SETTINGS_TRMT:
				xs = x; ys = y;
				SettingsDetecTrmt(cap,&(VoieManip[voie].def),TRMT_AU_VOL,actif,&x,&y);
				SettingsDetecTrmtDuree(cap,&(VoieManip[voie].duree),actif,&x,&ys);

				x = xs;
				BoardAreaOpen(L_("Traitement pre-trigger"),WND_RAINURES);
				if(BoloNb > 1) {
					panel = PanelCreate(1); PanelSupport(panel,WND_CREUX);
					PanelKeyB(panel,L_("Parametres"),L_("particuliers/standard"),adrs_tpn,13);
					PanelOnApply(panel,SettingsVolet[volet].standard2,(void *)(long)cap);
					BoardAddPanel(SettingsVolet[volet].planche,panel,x+Dx,y+Dy,0);
					xs += (24 * Dx);
				}
				SettingsDetecTrmt(cap,&(VoieManip[voie].def),TRMT_PRETRG,act2,&x,&y);
				break;
			  case SETTINGS_TRGR:
				xs = x + (37 * Dx); ys = y;
				SettingsDetecTrgr(cap,&(VoieManip[voie].def),actif,&x,&ys);
				y += (8 * Dy);
				SettingsDetecTrgrSupplement(voie,&(VoieManip[voie].def),&x,&y); // xs
				break;
			#ifdef SETTINGS_COUP
			  case SETTINGS_COUP:
				xs = x + (37 * Dx);
				SettingsDetecCoup(cap,&(VoieManip[voie].def),actif,&x,&y);
				break;
			#endif
			  case SETTINGS_RGUL:
				SettingsDetecRgul(cap,&(VoieManip[voie].def),VoieManip[voie].echelle,&(VoieManip[voie].interv),actif,&x,&y);
				break;
			  case SETTINGS_CATG:
				if(GestionConfigs) printf("(%s) %d categories dans la configuration '%s'\n",__func__,VoieManip[voie].def.nbcatg,VoieManip[voie].def.nom);
				SettingsDetecCatg(cap,&(VoieManip[voie].def),actif,&x,&y);
				break;
			}
		#ifdef LIGNE_SEPARE_VOIES
			if(!SettingsLigneHoriz.dx) SettingsLigneHoriz.dx = x;
		#else
			if(blocs > 1) {
				BoardAreaMargesVert(0,-Dy/2);
				BoardAreaEnd(SettingsVolet[volet].planche,&x,&y);
			}
		#endif
			y += (2 * Dy);
			// if(volet == SETTINGS_TRMT) printf("(%s) -------- Termine pour la voie %s\n",__func__,VoieManip[voie].nom);
		}
	}
#ifdef LIGNE_SEPARE_VOIES
	if(SettingsLigneHoriz.dx > 0) {
		fig = FigureCreate(FIG_DROITE,(void *)&SettingsLigneHoriz,0,0);
		BoardAddFigure(SettingsVolet[volet].planche,fig,0,(nb + 2) * Dy,0);
	}
#endif
	
	//	OpiumSize(SettingsVolet[volet].planche,x,y+400);
	//	OpiumEnterFctn(SettingsVolet[volet].planche,SettingsTrgrCree,0); si on fait ca, pas d'affichage !!
	OpiumExitFctn(SettingsVolet[volet].planche,SettingsDetecVerifie,(void *)(long)(SettingsVolet[volet].bolo));
	if(affiche) OpiumUse(SettingsVolet[volet].planche,OPIUM_FORK);
	
	return(1);
}
/* ========================================================================== */
void SettingsDetecAffiche(char volet) {
	if(SettingsVolet[volet].bolo == BoloStandard) {
		if(OpiumExec(pSettingsDetec->cdr) == PNL_CANCEL) return;
	}
	if(SettingsDetecCree(volet,0,0)) OpiumFork(SettingsVolet[volet].planche);
}
/* ========================================================================== */
int SettingsTrmtAffiche(Menu menu, MenuItem item) {
	SettingsDetecAffiche(SETTINGS_TRMT); return(0);
}
/* ========================================================================== */
int SettingsEvtsAffiche(Menu menu, MenuItem item) {
	SettingsDetecEvtsCree(0,0); OpiumFork(bDefEvts); return(0);
}
/* ========================================================================== */
int SettingsTrgrAffiche(Menu menu, MenuItem item) {
	SettingsDetecAffiche(SETTINGS_TRGR); return(0);
}
#ifdef SETTINGS_COUP
/* ========================================================================== */
int SettingsCoupAffiche(Menu menu, MenuItem item) {
	SettingsDetecAffiche(SETTINGS_COUP); return(0);
}
#endif
/* ========================================================================== */
int SettingsRgulAffiche(Menu menu, MenuItem item) {
	SettingsDetecAffiche(SETTINGS_RGUL); return(0);
}
/* ========================================================================== */
int SettingsCatgAffiche(Menu menu, MenuItem item) {
	SettingsDetecAffiche(SETTINGS_CATG); return(0);
}

/* ......................... operations diverses ............................ */
#pragma mark --- bidouilles ---
/* ========================================================================== */
static int SettingsPartitionChange() {
	int rep,vm,bolo,cap;

	switch(SettingsPartition) {
	  case SAMBA_PARTIT_STD: case SAMBA_PARTIT_SOLO: RepartiteursLocalise(); break;
	  case SAMBA_PARTIT_GLOBALE:
		for(rep=0; rep<RepartNb; rep++) Repart[rep].local = 1;
		break;
	}
	RepartiteurAppliquePerimetre();
	SambaModifiePerimetre();
	LectConsignesConstruit();
	for(vm=0; vm<ModeleVoieNb; vm++) ModeleVoie[vm].utilisee = 0;
	for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].local) {
		for(cap=0; cap<Bolo[bolo].nbcapt; cap++) ModeleVoie[ModeleDet[Bolo[bolo].type].type[cap]].utilisee = 1;
	}

	return(1);
}
/* ========================================================================== */
int SettingsTriggerCtrl() {
#ifdef MSGS_RESEAU
	if(SambaMode == SAMBA_DISTANT) {
		CarlaMsgBegin("Suspendre"); CarlaMsgEnd(&BalSuperviseur);
		printf("%s/ Envoi d'une commande 'Suspendre' au Superviseur\n",DateHeure());
	} else {
#endif
		if(Trigger.actif) {
			TousLesEvts.freq = 0.0;
			MenuItemAllume(mSettingsTriggerCtrl,1,L_(" Activer "),GRF_RGB_YELLOW);
			Trigger.actif = 0;
		} else {
			MenuItemAllume(mSettingsTriggerCtrl,1,L_("Suspendre"),GRF_RGB_GREY);
			Trigger.actif = 1;
		}
		printf("%s/ Recherche d'evenements %s\n",DateHeure(),Trigger.actif? "reactivee": "suspendue");
#ifdef MSGS_RESEAU
	}
#endif
	return(0);
}

/* ........................... initialisation .............................. */
#pragma mark --- initialisation ---
/* ========================================================================== */
void GestMenuBarre() { mGest = MenuLocalise(iGest); mImprim = MenuLocalise(iImprim); }
/* ========================================================================== */
void SettingsInit() {
	int i,j,n,nb;
	FILE *f;

/* Initialisation generale */
	SettingsParmsASauver = 0;
	SettingsFinessesASauver = 0;
	CalcNtpDemande = CalcNtpAffiche = 0;
	SettingsStockeFiltrees = 1;
	SettingsPartition = SAMBA_PARTIT_STD;
	SettingsIntervTrmt = 100;
	SettingsIntervSeq = 60000;
	SettingsIntervUser = 100;
	RepartUdpRelance = 300.0;
	ArchSauveTrace = 1;
	SettingsNtupleMax = 1024;
	ArchSauveNtp = NEANT;
	SettingsHdrGlobal = 1;
	SettingsMaxSuivi = 4096;
	SettingsSecuPolar = 0;
	SettingsRetard = 2;
	SettingsFibreLngr = 16;
	ReglTabNb = 0;
    CourbeVi = 0;
	TrmtCsnsmNb = 0;
	TemplateValeur = 1.0;
	TemplateUniteNum = TRMT_UNITE_ADU;
	SettingsGC = 0;

/* Variables sujettes a modification par l'utilisateur */
	SettingsDefaults();
	SettingsDocFormat = IMPRESSION_PDF;
	SettingsDocSupport = IMPRESSION_FICHIER;
	SettingsDocCablage = SettingsDocModeles = SettingsDocAutom = SettingsDocScripts = 1;
	NbTirages = 1; NumPrinter = 0; strcpy(ListePrinters,"OCE/Jet");
	SettingsLectureGlobale = 0;
	SettingsImprBolosVsn = MONIT_IMPR_BOLOS_BB1;
	SettingsRunFamille = RUN_FAMILLE_BANC;
	SettingsVoiesNom = VOIE_NOM_STD;
	RunCategNum = 0;
	SettingsInlineProcess = 0;
	SettingsGenePossible = 0;
	SettingsDeconvPossible = 0;
	OrgaStyle = ORGA_ICONES; // ORGA_ZONES;
	LectCntl.RunCategNum = 0;
//	LectCntl.SettingsRunEnvr = 0;
	LectSignalAmpl = 5000; LectSignalZero = 0; // impose dans LectSetup: LectSignalTemps = 1.0;
	SettingsIdentCheck = 0;
	SettingsStatusCheck = 0;
	CompensationApprox = 0.98;
	CompensationAttente = 10.0;
	SettingsDLUdetec = 3600;
	SettingsDelaiIncertitude = 10.0;
	TrgrRegulDelai = 1;
	for(i=0; i<TRMT_NBTYPES; i++) TrmtListe[i] = TrmtTypeListe[i];
	TrmtListe[TRMT_FILTRAGE] = "standard"; /* petite magouille */
	for(i=0; i<FiltreMax+1; i++) TrmtListe[TRMT_NBTYPES+i] = FiltreGeneral[i].nom;
	strcpy(SettingsTxtStd,"standard");
	LectTrigOnline = 0;
#ifdef EDW_ORIGINE
	strcpy(AutomCalibNom[0],"Ba_grotte");
	strcpy(AutomCalibNom[1],"Ba_nemo");
	strcpy(AutomRegenNom[0],"Co_France");
	strcpy(AutomRegenNom[1],"Co_Italie");
#endif /* EDW_ORIGINE */

	TrmtHampl = TrmtHmontee = TrmtH2D = 1;
	SettingsHistoVoie = 0;
	SettingsAmplMin = 0.0; SettingsAmplMax = 10000.0; SettingsAmplBins = 100; /* Limites histo amplitude */
	SettingsMonteeMin = 0.0; SettingsMonteeMax = 0.01; SettingsMonteeBins = 100; /* idem tps de montee */

	SettingsIPEwait = 0;
	SettingsD3wait = 100;
	SettingsFIFOwait = 150;       /* a vue de nez, 139 boucles avant un transfert de 1 mot */
	SettingsSynchroMax = 0; // 999999;
	LectSynchroType = LECT_SYNC_D3; // remettre 0;
	SettingsTauxPeriode = 10;
	LectAttenteErreur = 5; // 120 avec Opera
	LectTauxSeuil = 1.0;
	LectDelaiMini = 100.0;
	SettingsRegenDelai = 300;
	SettingsRazNtdDuree = 60;
	SettingsRazNbSiDuree = 300;
	LectDureeActive  = 1;
	LectDureeMaxi = DUREE_ILLIMITEE; // 120;
	LectDureeTranche = 5;
	LectDureeAcq = 600;
	LectDureeRegen = 15;
	LectDureeStream = 5;
	RepartIpCorrection = REPART_IP_GENERE;
	LectRetardOption = 0;
	LectStoppeRepart = 1;
	LectLitStatus = 1;
	LectSauveStatus = 1;
	LectDecodeStatus = 0;
	//- BnAuxiDuree = 20.48; BnAuxiPretrig = 50.0;

	Trigger.type = 1;
	Trigger.demande = 0;
	Archive.demandee = 0;

	SettingsPauseEvt = 0;
	// SettingsAttendsSeuil = 0; /* normal */

	ArchModeStream = ARCH_BRUTES;
	ArchDetecMode = ARCH_INDIV;
	ArchAssocMode = 1;
	ArchFormat = ARCH_FMT_EDW2;
	ArchRegenTaux = 100;

	/* Lecture de la configuration */
	if(SambaLogDemarrage) SambaLogDef("= Lecture des parametres","dans",FichierPrefParms);
	if(ArgScan(FichierPrefParms,SettingsDesc) <= 0) /* SettingsTotal */ {
		if(CreationFichiers) {
			RepertoireCreeRacine(FichierPrefParms);
			ArgPrint(FichierPrefParms,SettingsDesc);
		} else OpiumError("Lecture de '%s' impossible",FichierPrefParms);
	}
	if(SambaLogDemarrage) SambaLogDef("= Lecture des details","dans",FichierPrefFinesses);
	if(ArgScan(FichierPrefFinesses,SettingsFinessesDesc) <= 0) {
		if(CreationFichiers) {
			RepertoireCreeRacine(FichierPrefFinesses);
			ArgPrint(FichierPrefFinesses,SettingsFinessesDesc);
		} else OpiumWarn("Lecture de '%s' impossible",FichierPrefFinesses);
	}
	if(SettingsMultitasks != 1) SettingsMultitasks = 4; /* plus trop le choix! */
	Trigger.enservice = (Trigger.type == NEANT)? 0: 1;
	SambaDefInitial = SambaDefMax;
	TrmtRegulAttendu = 1.0;
	TrmtRegulType = TrmtRegulActive = 0;
	//+ #ifdef REGUL_UNIQUE
	/* Sert a definir une valeur par defaut pour les voies (donc, dans DetecteursLit) */
	for(j=0; j<MAXECHELLES; j++) {
		TrmtRegulTaux[j].duree = (int)(exp((double)j * log(10.0)));
		TrmtRegulTaux[j].active = 1;
		TrmtRegulTaux[j].min.nb = (int)(0.5 * (float)TrmtRegulTaux[j].duree);
		TrmtRegulTaux[j].min.action = TRMT_REGUL_ADD;
		TrmtRegulTaux[j].min.parm = -2.0;
		TrmtRegulTaux[j].max.nb = 2 * TrmtRegulTaux[j].duree;
		TrmtRegulTaux[j].max.action = TRMT_REGUL_ADD;
		TrmtRegulTaux[j].max.parm = 2.0;
	}
	TrmtRegulInterv.nb = 5;
	TrmtRegulInterv.duree = 5.0;
	TrmtRegulInterv.ajuste = 0.0;
	TrmtRegulInterv.seuil = 1.5;
	TrmtRegulInterv.max = 2;
	TrmtRegulInterv.incr = 2.0;
	//+ endif
	if(SambaLogDemarrage) SambaLogDef("= Lecture de la regulation","dans",FichierPrefRegulEvt);
	if(ArgScan(FichierPrefRegulEvt,SettingsRegulDesc) <= 0) {
		if(CreationFichiers) {
			RepertoireCreeRacine(FichierPrefRegulEvt);
			ArgPrint(FichierPrefRegulEvt,SettingsRegulDesc);
		} else OpiumError("Lecture de '%s' impossible",FichierPrefRegulEvt);
	}
	
	VerifEtatFinal = VerifRepertIP = VerifDisplay = VerifMonitEvts = VerifpTauxEvts = 0;
	VerifCalculTauxEvts = VerifBittrigger = VerifIntegrales = 0;
	VerifEvtMoyen = VerifCalculEvts = VerifSuiviEtatEvts = VerifSuiviTrmtEvts = VerifOctetsStream = 0;
	VerifErreurStoppe = VerifIpLog = VerifErreurLog = VerifRunDebug = VerifCpuCalcul = VerifCalculDetail = 0;
	VerifTempsPasse = VerifConsoCpu = 0;
	VerifCompens = 0; GestionConfigs = 0;
	VerifIdentBB = VerifIdentRepart = 0;
	if(SambaLogDemarrage) SambaLogDef("= Lecture du deverminage","dans",FichierPrefVerif);
	if(ArgScan(FichierPrefVerif,SettingsVerifDesc) <= 0) {
		if(CreationFichiers) {
			RepertoireCreeRacine(FichierPrefVerif);
			ArgPrint(FichierPrefVerif,SettingsVerifDesc);
		} else OpiumError("Lecture de '%s' impossible",FichierPrefVerif);
	}
	
	if(ImprimeConfigs) {
		printf("#\n### FichierCrParms: '%s' ###\n",FichierCrParms);
		ArgPrint("*",SambaCrDesc);
		printf("#\n### FichierPrefParms: '%s' ###\n",FichierPrefParms);
		ArgPrint("*",SettingsDesc);
		printf("#\n### FichierPrefRegulEvt: '%s' ###\n",FichierPrefRegulEvt);
		ArgPrint("*",SettingsRegulDesc);
		printf("#\n### FichierPrefVerif: '%s' ###\n",FichierPrefVerif);
		ArgPrint("*",SettingsVerifDesc);
		printf("#\n### FichierPrefFinesses: '%s' ###\n",FichierPrefFinesses);
		ArgPrint("*",SettingsFinessesDesc);
		printf("\n");
	}

	if(LectTrigOnline) SettingsInlineProcess = 1;
		
/*	PointsMax = DureeTampons * Echantillonnage;  pour etre tranquille avec les index dans les graphes */
#ifdef FILTRES_PARTAGES
	if(PasMaitre) {
		FiltreNb = SambaSetup->FiltreNb;
		if(!CarlaDataRequest(AcqCible,IdFiltreGen))
			printf("  ! Recuperation des filtres sur %s: %s\n",AcqCible,CarlaDataError());
	} else {
#endif
		f = fopen(FichierPrefFiltres,"r");
		if(f) fclose(f);
		else if(StartFromScratch) {
			FiltreNum = 0;
			FiltreGeneral[FiltreNum].commentaire[0] = '\0';
			FiltreGeneral[FiltreNum].modele = FILTRE_BUT;
			FiltreGeneral[FiltreNum].type = FILTRE_PHAUT;
			FiltreGeneral[FiltreNum].degre = 4;
			FiltreGeneral[FiltreNum].unite1 = FILTRE_REL;
			FiltreGeneral[FiltreNum].unite2 = FILTRE_REL;
			FiltreGeneral[FiltreNum].omega1 = 0.1;
			FiltreGeneral[FiltreNum].omega2 = 0.0001;
			FiltreStandardiseNom(&(FiltreGeneral[FiltreNum]));
			FiltreNb = FiltreNum + 1;
			FiltresEcrit(FichierPrefFiltres);
		}
		FiltresLit(FichierPrefFiltres,SambaLogDemarrage);
#ifdef FILTRES_PARTAGES
		if(SambaMode == SAMBA_MAITRE) {
			SambaSetup->FiltreNb = FiltreNb;
		}
	}
#endif
//	FiltresDump();
	if(Evt) free(Evt); Evt = (TypeEvt *)malloc(EvtNbMax * sizeof(TypeEvt));
	if(Evt == 0) printf("  ! Meme pas %d octets pour stocker %d evenements!\n",EvtNbMax * sizeof(TypeEvt),EvtNbMax);
	for(n=0; n<EvtNbMax; n++) { Evt[n].deconv_max = 0; Evt[n].deconvolue = 0; }

	if(ModeBatch) return;

/* Creation de l'interface */
	mSettings = MenuLocalise(iSettings);
	mSettingsDetecEvts  = MenuLocalise(iSettingsDetecEvts);
	
	mTemplates = MenuLocalise(iTemplates);
	mPattern = MenuLocalise(iPattern);
	mGestVI = MenuLocalise(iGestVI); OpiumExitFctn(mGestVI->cdr,ViExit,0);
/*	OpiumSupport(mFiltres->cdr,WND_PLAQUETTE); */
	mGestCal = MenuLocalise(iGestCal);
	mGestParms = MenuLocalise(iGestParms);
	mGestExport = MenuLocalise(iGestExport);
	mGestAppel = MenuLocalise(iGestAppel);
	mGestConfig = MenuLocalise(iGestConfig);
	mGestGeneraux = MenuLocalise(iGestGeneraux);
#ifdef PARMS_PAR_MENUS
	mGestRegulEvt = MenuLocalise(iGestRegulEvt);
	mGestAffichage = MenuLocalise(iGestAffichage);
	mGestCalcul = MenuLocalise(iGestCalcul);
	mGestVerif = MenuLocalise(iGestVerif);
	mGestFinesses = MenuLocalise(iGestFinesses);
#endif /* PARMS_PAR_MENUS */

	FolderNum = 0; FolderActn = 0;
	pFolderActn = PanelCreate(2);
	PanelList(pFolderActn,L_("Fichier ou repertoire"),FolderListe,&FolderNum,32);
	PanelKeyB(pFolderActn,L_("Montrer"),L_("contenu/dossier"),&FolderActn,8);

	ImpressionInit();
	
	pSambaDesc = PanelDesc(SambaDesc,1); PanelMode(pSambaDesc,PNL_RDONLY);
	pConfigDesc = PanelDesc(Setup,1); PanelMode(pConfigDesc,PNL_RDONLY);

	pSettingsDesc = PanelDesc(SettingsDesc,1);
	PanelTitle(pSettingsDesc,"Preferences");
	nb = PanelItemNb(pSettingsDesc);
	for(i=nb/2; i<nb; i++) PanelItemLngr(pSettingsDesc,i+1,64);
	for(i=0; i<nb/2; i++) if(i && !(i % 5)) {
		PanelItemSouligne(pSettingsDesc,i+1,1);
		PanelItemSouligne(pSettingsDesc,i+1+nb/2,1);
	}
	PanelOnApply(pSettingsDesc,SettingsParmsChange,0);
	PanelOnOK(pSettingsDesc,SettingsParmsChange,0);
	
	pSettingsRegulEvt = PanelDesc(SettingsRegulDesc,1);
	PanelTitle(pSettingsRegulEvt,"Regulation evts");
	PanelOnOK(pSettingsRegulEvt,(TypePanelFctnArg)SettingsRegulEcrit,0);

	pSettingsFinesses = PanelDesc(SettingsFinessesDesc,1);
	PanelTitle(pSettingsFinesses,L_("Finesses"));
	PanelOnOK(pSettingsFinesses,(TypePanelFctnArg)SettingsFinessesEcrit,0);

	pSettingsVerif = PanelDesc(SettingsVerifDesc,1);
	PanelTitle(pSettingsVerif,"Verifications");
	PanelOnOK(pSettingsVerif,(TypePanelFctnArg)SettingsVerifEcrit,0);

	pSettingsCompteRendu = PanelDesc(SambaCrDesc,1);
	PanelTitle(pSettingsCompteRendu,"Compte-rendus");
	PanelOnOK(pSettingsCompteRendu,(TypePanelFctnArg)SettingsVerifCR,0);
	
	pTemplate = PanelCreate(4);
	PanelFloat(pTemplate,L_("Valeur mesuree"),&TemplateValeur);
	PanelListB(pTemplate,L_("Unite"),TemplateUniteListe,&TemplateUniteNum,UNITE_NOM);
	PanelInt  (pTemplate,L_("Premier point valide"),&TemplateDepart);
	PanelInt  (pTemplate,L_("Dernier point valide"),&TemplateArrivee);

	pViChoix = PanelCreate(2);
	PanelList(pViChoix,L_("Detecteur"),BoloNom,&BoloNum,DETEC_NOM);
	PanelFloat(pViChoix,"Temperature",&ReglTbolo);

	gGestVi = GraphCreate(300,300,2);
	OpiumTitle(gGestVi->cdr,"V(I)");
	GraphAdd(gGestVi,GRF_FLOAT|GRF_XAXIS,ReglTabI,MAXVI);
	GraphDataRGB(gGestVi,GraphAdd(gGestVi,GRF_FLOAT|GRF_YAXIS,ReglTabV,MAXVI),GRF_RGB_GREEN);
	GraphUse(gGestVi,0);

	pSettingsImprimeForme = PanelCreate(6);
	PanelListB(pSettingsImprimeForme,"Media",ImpressionSupports,&SettingsDocSupport,IMPRESSION_SUPPORT_LNGR);
	PanelListB(pSettingsImprimeForme,"Format",ImpressionFormats,&SettingsDocFormat,IMPRESSION_FORMAT_LNGR);
	PanelBool (pSettingsImprimeForme,"Cablage",&SettingsDocCablage);
	PanelBool (pSettingsImprimeForme,"Modeles",&SettingsDocModeles);
	PanelBool (pSettingsImprimeForme,"Support",&SettingsDocAutom);
	PanelBool (pSettingsImprimeForme,"Scripts",&SettingsDocScripts);
	pSettingsImprimeSvr = PanelCreate(2);
	PanelKeyB(pSettingsImprimeSvr,"Sur quelle imprimante",ListePrinters,&NumPrinter,16);
	PanelInt(pSettingsImprimeSvr,"En combien d'exemplaires",&NbTirages);
	OpiumTitle(pSettingsImprimeSvr->cdr,"Options d'impression");

	pSettingsDate = PanelCreate(1);
	PanelItemFormat(pSettingsDate,PanelInt(pSettingsDate,L_("Date preferentielle (aammjj, 0 si derniere)"),&SettingsDate),"%06d");
	
#ifdef MODULES_RESEAU
//	InstrumOnChange(cModeLecture,LectUpdateControl,0);
//	InstrumOnChange(cTrigger,LectUpdateControl,0);
	OpiumTimer(cModeLecture->cdr,500);
	OpiumTimer(cTrigger->cdr,500);
#endif

	pSettingsTrigger = PanelCreate(7 + ((TRMT_NBCLASSES + 2) * (ModeleVoieNb+1))); /* rempli dans SettingsModes selon modes */

}
/* ========================================================================== */
TypeMenuItem iSettingsDetecStd[] = {
	{ "Recopier   ",  MNU_FONCTION SettingsDetecCopieStd },
	{ "Enregistrer",  MNU_FONCTION SettingsDetecEnreg },
	MNU_END
};
TypeMenuItem iSettingsDetecReel[] = {
	{ "Enregistrer",  MNU_FONCTION SettingsDetecEnreg },
	MNU_END
};
/* ========================================================================== */
static int SettingsOptionTrmtCalage(Panel p, void *arg) {
	strcpy(SettingsCalageModl,TrmtDatationListe[SettingsCalageRef]); return(1);
}
#ifdef CENTRAGE_INDEPENDANT
/* ========================================================================== */
static int SettingsOptionTrmtCentrage(Panel p, void *arg) {
	strcpy(SettingsCentrageNom,TrmtDatationListe[SettingsCentrageRef]); return(1);
}
#endif
/* ========================================================================== */
static int SettingsOptionTrmtReferences(Panel p, void *arg) {
	strcpy(SettingsCalageModl,TrmtDatationListe[SettingsCalageRef]);
	strcpy(SettingsCentrageNom,TrmtDatationListe[SettingsCentrageRef]);
	return(1);
}
/* ========================================================================== */
static int SettingsOptionsFerme(Cadre cdr, void *arg) {
	printf("> (%s) cdr->var_modifiees      @%08X: %d\n",__func__,(hexa)(&(cdr->var_modifiees)),cdr->var_modifiees);
	if(cdr->var_modifiees) {
		SettingsCorrigePlanches();
		SettingsParmsASauver = 1;
	}
	return(0);
}
/* ========================================================================== */
static int SettingsOptionsConstruit(Instrum instrum, void *arg) {
	char affiche; int x,y,x0,y0;
	Panel p; Instrum inst;

	if((affiche = OpiumDisplayed(bPrmCourants))) OpiumClear(bPrmCourants);
	BoardTrash(bPrmCourants);
//	printf("(%s) Debut de construction\n",__func__);

	x = 3 * Dx; y = 2 * Dy;
	BoardAreaOpen(L_("Gestion du temps"),WND_PLAQUETTE);
	p = PanelCreate(5); PanelSupport(p,WND_CREUX);
	PanelFloat(p,L_("Echantillonnage (kHz)"),&Echantillonnage);
	PanelInt  (p,L_("Duree run maximum (mn)"),&LectDureeMaxi);
	PanelInt  (p,L_("Duree fichier (mn)"),&LectDureeTranche);
	if(BolosAentretenir) PanelInt  (p,L_("Entretien detecteurs (s)"),&SettingsDLUdetec);
	PanelFloat(p,L_("Taux maxi d'evenements (Hz)"),&LectTauxSeuil);
	BoardAdd(bPrmCourants,p->cdr,x,y,0); // x += (46 * Dx);
	BoardAreaEnd(bPrmCourants,&x,&y);
//	printf("(%s) Zone 'Gestion du temps' terminee\n",__func__);

	x += Dx;
	if(RepartNb > 1) {
		InstrumTitle(inst = InstrumKey(INS_GLISSIERE,(void *)&Gliss3lignes,&SettingsPartition,SAMBA_PARTIT_CLES),"Partition");
		InstrumOnChange(inst,SettingsPartitionChange,0);
		BoardAdd(bPrmCourants,inst->cdr,x,Dy,0); // x += (15 * Dx);
	}
	// y += ((PanelItemNb(p) + 2) * Dy);

	if(VoiesNb > 1) {
		BoardAreaOpen(L_("Evenements"),WND_PLAQUETTE);
		BoardAreaOpen(L_("Suppression"),WND_RAINURES);
		x = 7 * Dx; y += (3 * Dy);
	} else {
		BoardAreaOpen(L_("Suppression d'evenements"),WND_PLAQUETTE);
		x = 10 * Dx; y += (2 * Dy);
	}
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsConformite,L_("non/oui")),L_("Conformite"));
	BoardAdd(bPrmCourants,inst->cdr,x,y+Dy,0); x += (15 * Dx);
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsCoupures,L_("non/oui")),L_("Coupures"));
	InstrumOnChange(inst,SettingsTrgrAffiche,0);
	BoardAdd(bPrmCourants,inst->cdr,x,y+Dy,0); // x += (15 * Dx);
	BoardAreaEnd(bPrmCourants,&x,&y);
//	printf("(%s) Zone 'Suppression d'evenements' terminee\n",__func__);

	if(VoiesNb > 1) {
		x = 4 * Dx; y += (2 * Dy); x0 = x; y0 = y;
		BoardAreaOpen(L_("Synchro traces"),WND_RAINURES);
		InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss3lignes,&SettingsCalageMode,L_(CALAGES_PORTEE_CLES)),L_("Portee"));
		InstrumOnChange(inst,SettingsOptionsConstruit,0);
		BoardAdd(bPrmCourants,inst->cdr,x,y,0); x += (20 * Dx);
		if(SettingsCalageMode) {
			InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsCalageObj,L_(CALAGES_OBJET_CLES)),L_("cible"));
			BoardAdd(bPrmCourants,inst->cdr,x,y,0);
			if(SettingsCalageMode == CALAGE_UNIQUE) {
				p = PanelCreate(1); PanelSupport(p,WND_CREUX);
				PanelListB(p,L_("voie de reference"),TrmtDatationListe,&SettingsCalageRef,MODL_NOM);
				PanelOnApply(p,SettingsOptionTrmtCalage,0);
				BoardAddPanel(bPrmCourants,p,x0+(11*Dx),y+(3*Dy),0);
			}
		}
		BoardAreaEnd(bPrmCourants,&x,&y);

	#ifdef CENTRAGE_INDEPENDANT
		x = 3 * Dx; y += (2 * Dy); // x += (3 * Dx); y = y0;
		BoardAreaOpen(L_("Centrage"),WND_PLAQUETTE);
		InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss3lignes,&SettingsCentrageType,L_(CALAGES_PORTEE_CLES)),L_("portee"));
		BoardAdd(bPrmCourants,inst->cdr,x,y,0); x += (14 * Dx);
		p = PanelCreate(1); PanelSupport(p,WND_CREUX);
		PanelListB(p,L_("Reference"),TrmtDatationListe,&SettingsCentrageRef,MODL_NOM);
		PanelOnApply(p,SettingsOptionTrmtCentrage,0);
		BoardAddPanel(bPrmCourants,p,x,y+2*Dy,0);
		BoardAreaEnd(bPrmCourants,&x,&y);
	#endif /* CENTRAGE_INDEPENDANT */
	}

	if(VoiesNb > 1) BoardAreaEnd(bPrmCourants,&x,&y);

	x = 3 * Dx; y += (2 * Dy); x0 = x; y0 = y;
	BoardAreaOpen(L_("Sauvegardes auxiliaires"),WND_PLAQUETTE);
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss3lignes,&ArchSauveNtp,L_("sans/ASCII/PAW")),"n-Tuple");
	BoardAddInstrum(bPrmCourants,inst,x,y,0); x += (15 * Dx);
	p = PanelCreate(1); PanelSupport(p,WND_CREUX); PanelInt(p,"Reduction",&ArchReduc); PanelItemLngr(p,1,4);
	BoardAddPanel(bPrmCourants,p,x,y,0); x += (15 * Dx);
	PanelMode(p = PanelCreate(1),PNL_DIRECT); PanelSupport(p,WND_CREUX);
	PanelInt(p,"Export(ms)",&ExportPack[0].periode); PanelItemILimits(p,1,1,10000); PanelItemFormat(p,1,"%d"); PanelItemLngr(p,1,5);
	BoardAddPanel(bPrmCourants,p,x,y,0); x += (15 * Dx);
	BoardAreaEnd(bPrmCourants,&x,&y);
//	printf("(%s) Zone 'Sauvegardes auxiliaires' terminee\n",__func__);

	x = 3 * Dx; y += (2 * Dy); x0 = x; y0 = y;
	BoardAreaOpen("Plugins",WND_PLAQUETTE);
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&LectRetardOption,L_("absent/present")),L_("Retardateur"));
	InstrumOnChange(inst,LectRenovePlanche,0);
	BoardAdd(bPrmCourants,inst->cdr,x,y,0); x += (12 * Dx);
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsTrigInOut,L_("absent/present")),"Trig in/out");
	InstrumOnChange(inst,SettingsTrgrAffiche,0);
	BoardAdd(bPrmCourants,inst->cdr,x,y,0); x += (12 * Dx);
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsGenePossible,L_("ignoree/possible")),"Generation");
	InstrumOnChange(inst,GegeneAutorise,0);
	BoardAdd(bPrmCourants,inst->cdr,x,y,0); x += (12 * Dx);
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsDeconvPossible,L_("ignorer/permettre")),L_("Deconvoluer"));
	InstrumOnChange(inst,GegeneAutorise,0);
	BoardAdd(bPrmCourants,inst->cdr,x,y,0); x += (12 * Dx);
	BoardAreaEnd(bPrmCourants,&x,&y);
//	printf("(%s) Zone 'Plugins' terminee\n",__func__);

	BoardAddBouton(bPrmCourants,L_("Sauver"),SettingsParmsEcrit,(x - (6 * Dx)) / 2,y + (2 * Dy),0);
	//+ BoardAddBouton(bPrmCourants,"Panique",SettingsPanique,OPIUM_A_DROITE_DE OPIUM_PRECEDENT);
	OpiumExitFctn(bPrmCourants,SettingsOptionsFerme,0);

	if(affiche) OpiumUse(bPrmCourants,OPIUM_FORK);
//	printf("(%s) Fin de construction\n",__func__);
	return(0);
}
/* ========================================================================== */
static int SettingsControleFerme(Cadre cdr, void *arg) {
	if(cdr->var_modifiees) {
		SettingsCorrigePlanches();
		SettingsParmsASauver = 1;
		SettingsFinessesASauver = 1;
	}
	return(0);
}
/* ========================================================================== */
static int SettingsControleConstruit(Instrum instrum, void *arg) {
	Panel p; Instrum inst; Figure fig; int x,y,y0,xmax,ybas,quinzelettres,troislignes;
	char affiche;

	if((affiche = OpiumDisplayed(bPrmDetailles))) OpiumClear(bPrmDetailles);
	BoardTrash(bPrmDetailles);

	quinzelettres = 15 * Dx; troislignes = 3 * Dy;
	
// #define EMBOITER
#ifdef EMBOITER
	bTableauGeneral = BoardCreate(); x = 40; y = 0;
	BoardAdd(bTableauGeneral,pBuffer->cdr,x,y,0); y += 80;

	bTrmtAuVol = BoardCreate(); OpiumTitle(bTrmtAuVol,L_("Traitements au vol")); OpiumSupport(bTrmtAuVol,WND_PLAQUETTE);
	bTrmtTampon = BoardCreate(); OpiumTitle(bTrmtTampon,L_("Traitements pre-trigger")); OpiumSupport(bTrmtTampon,WND_PLAQUETTE);
	for(vm=0; vm<ModeleVoieNb; vm++) {
		pTrmtAuVol[vm] = PanelCreate(2);
		OpiumSize(pTrmtAuVol[vm]->cdr,290,troislignes);
		pTrmtTampon[vm] = PanelCreate(5);
		OpiumSize(pTrmtTampon[vm]->cdr,290,WndLigToPix(6));
	}
	x = 100; y = 0;
	for(voie=0; voie<ModeleVoieNb; voie++) {
		cTrmtAuVol[voie] = InstrumListB(INS_POTAR,(void *)&Select270,&(VoieTampon[voie].trmt[TRMT_AU_VOL].type),TrmtTypeListe);
		InstrumTitle(cTrmtAuVol[voie],VoieTampon[voie].nom);
		BoardAddInstrum(bTrmtAuVol,cTrmtAuVol[voie],x,y,0);
		InstrumOnChange(cTrmtAuVol[voie],SettingsTrmtVolChange,(void *)voie);
		pTrmtAuVol[voie] = PanelCreate(2);
		OpiumSize(pTrmtAuVol[voie]->cdr,290,troislignes);
		SettingsTrmtVolVariable(voie);
		PanelSupport(pTrmtAuVol[voie],WND_CREUX);
		BoardAddPanel(bTrmtAuVol,pTrmtAuVol[voie],x,y+130,0);

		cTrmtTampon[voie] = InstrumListB(INS_POTAR,(void *)&Select270,&(VoieTampon[voie].trmt[TRMT_PRETRG].type),TrmtTypeListe);
		InstrumTitle(cTrmtTampon[voie],VoieTampon[voie].nom);
		BoardAddInstrum(bTrmtTampon,cTrmtTampon[voie],x,y,0);
		InstrumOnChange(cTrmtTampon[voie],SettingsTrmtTpnChange,(void *)voie);
		pTrmtTampon[voie] = PanelCreate(6);
		OpiumSize(pTrmtTampon[voie]->cdr,290,WndLigToPix(6));
		SettingsTrmtTpnVariable(voie);
		PanelSupport(pTrmtTampon[voie],WND_CREUX);
		BoardAddPanel(bTrmtTampon,pTrmtTampon[voie],x,y+130,0);
		x += 290;
	}
	OpiumSize(bTrmtAuVol,x,240);
	OpiumSize(bTrmtTampon,x,260);
	y = 60;
	BoardAdd(bTableauGeneral,bTrmtAuVol,10,y,0); y += 270;
	BoardAdd(bTableauGeneral,bTrmtTampon,10,y,0);
	OpiumSize(bTableauGeneral,x+20,y+300);
	OpiumExitFctn(bTableauGeneral,SettingsModifieGeneral,0);
#endif /* EMBOITER */

	x = Dx; y = Dy;
	//                                                                                 -------
	fig = FigureCreate(FIG_ZONE,(void *)(&SettingsTitreSecteur),0,0); FigureTexte(fig,"Timming");
	BoardAddFigure(bPrmDetailles,fig,x+7*Dx,y,0); y += SettingsTitreSecteur.haut;
	p = pBuffer = PanelCreate(14);
	PanelFloat(p,L_("Echantillonnage (kHz)"),&Echantillonnage);
	PanelInt  (p,L_("Duree tampons (ms)"),&DureeTampons);
	PanelFloat(p,L_("Marge UDP (s)"),&SettingsDelaiIncertitude);
	PanelFloat(p,L_("Relance UDP (s)"),&RepartUdpRelance);
	if(RepartIpeNb) PanelInt  (p,L_("Attente apres stop IPE (s)"),&SettingsIPEwait);
	PanelInt  (p,L_("Attente avant D3 finale (ms)"),&SettingsD3wait);
	PanelInt  (p,L_("Periode de calcul du taux (s)"),&SettingsTauxPeriode);
	PanelInt  (p,L_("Temps mort regulation (D3)"),&TrgrRegulDelai);
	if(BolosAentretenir) PanelInt  (p,L_("Entretien detecteurs (s)"),&SettingsDLUdetec);
	if(SettingsRegen)    PanelInt  (p,L_("Attente mini apres regen (s)"),&SettingsRegenDelai);
	PanelInt  (p,L_("Attente mini apres erreur (s)"),&LectAttenteErreur);
	PanelInt  (p,L_("Longueur suivi filtrage"),&SettingsMaxSuivi);
	PanelInt  (p,L_("Nb. max de n-tuples"),&SettingsNtupleMax);
	PanelListB(p,L_("Reference calage"),TrmtDatationListe,&SettingsCalageRef,MODL_NOM);
#ifdef CENTRAGE_INDEPENDANT
	PanelListB(p,L_("Reference centrage"),TrmtDatationListe,&SettingsCentrageRef,MODL_NOM);
#endif
	PanelOnApply(p,SettingsOptionTrmtReferences,0);
	PanelSupport(p,WND_CREUX);
	BoardAdd(bPrmDetailles,p->cdr,x,y,0);
	ybas = WndLigToPix(PanelItemNb(p)+4);
	x = WndColToPix(51); y = Dy;
	xmax = x;
	
	//                                                                                 ----------
	fig = FigureCreate(FIG_ZONE,(void *)(&SettingsTitreSecteur),0,0); FigureTexte(fig,L_("Traitement evenements"));
	BoardAddFigure(bPrmDetailles,fig,x+7*Dx,y,0); y += (SettingsTitreSecteur.haut + Dy);
	y0 = y;
	InstrumTitle(cModeCalcEvt = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss6lignes,&SettingsCalculEvt,L_(TrmtCalculCles)),L_("Niveau"));
	BoardAdd(bPrmDetailles,cModeCalcEvt->cdr,x,y,0); y += (InstrumRectLngr(&Gliss6lignes) + troislignes);
	InstrumTitle(cTrigOnline = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&LectTrigOnline,L_("non/oui")),"Trig.online");
	BoardAdd(bPrmDetailles,cTrigOnline->cdr,x,y,0); y += (InstrumRectLngr(&Gliss2lignes) + troislignes);
	InstrumTitle(cDeconv = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&TrmtDeconvolue,L_("non/oui")),"Deconvoluer");
	BoardAdd(bPrmDetailles,cDeconv->cdr,x,y+Dy,0); y += (InstrumRectLngr(&Gliss2lignes) + troislignes);
	y -= (2 * Dy); if(ybas < y) ybas = y;
	x += quinzelettres; if(xmax < x) xmax = x;

	y = y0;
	InstrumTitle(cModeDateEvt = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsTempsEvt,L_(TrgrDateCles)),"Date");
	BoardAdd(bPrmDetailles,cModeDateEvt->cdr,x,y,0); y += (InstrumRectLngr(&Gliss2lignes) + troislignes);
	InstrumTitle(cTamponUtilise = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsCalcPretrig,L_("brutes/trigger")),L_("Donnees utilisees"));
	BoardAdd(bPrmDetailles,cTamponUtilise->cdr,x,y,0); y += (InstrumRectLngr(&Gliss2lignes) + troislignes);
	// InstrumTitle(cCalage = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss3lignes,&SettingsCalageMode,L_(CALAGES_PORTEE_CLES)),L_("Temps du maxi"));
	InstrumTitle(cCalage = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss3lignes,&SettingsCalageMode,L_(CALAGES_PORTEE_CLES)),L_("Calage trace"));
	BoardAdd(bPrmDetailles,cCalage->cdr,x,y,0); y += (InstrumRectLngr(&Gliss3lignes) + troislignes);
//	InstrumTitle(cNtuples = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&CalcNtpDemande,L_("non/oui")),"Ntuples");
//	InstrumOnChange(cNtuples,MonitNtupleChange,0);
//	BoardAdd(bPrmDetailles,cNtuples->cdr,x,y,0); y += (InstrumRectLngr(&Gliss2lignes) + troislignes);
	InstrumTitle(cLisStatus = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&LectLitStatus,L_("ignore/lu")),L_("Status Numeriseurs"));
	BoardAdd(bPrmDetailles,cLisStatus->cdr,x,y,0); y += (InstrumRectLngr(&Gliss2lignes) + troislignes);
	y -= (2 * Dy); if(ybas < y) ybas = y;
	x += WndColToPix(20); if(xmax < x) xmax = x;

	y = y0;
	if(xmax < x) xmax = x;
	InstrumTitle(cCoupures = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsCoupures,L_("non/oui")),L_("Coupures"));
	InstrumOnChange(cCoupures,SettingsTrgrAffiche,0);
	BoardAdd(bPrmDetailles,cCoupures->cdr,x,y,0); y += (InstrumRectLngr(&Gliss2lignes) + troislignes);
	InstrumTitle(cConformite = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsConformite,L_("non/oui")),L_("Conformite"));
	BoardAdd(bPrmDetailles,cConformite->cdr,x,y,0); y += (InstrumRectLngr(&Gliss2lignes) + troislignes);
#ifdef CENTRAGE_INDEPENDANT
	InstrumTitle(cCentrage = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss3lignes,&SettingsCentrageType,L_(CALAGES_PORTEE_CLES)),L_("Centrage"));
	BoardAdd(bPrmDetailles,cCentrage->cdr,x,y,0); y += (InstrumRectLngr(&Gliss3lignes) + troislignes);
#endif
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsCalageObj,L_(CALAGES_OBJET_CLES)),L_("Niveau calage"));
	BoardAdd(bPrmDetailles,inst->cdr,x,y,0); y += (InstrumRectLngr(&Gliss2lignes) + troislignes);
	InstrumTitle(cNtuples = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss3lignes,&CalcNtpDemande,L_(SAMBA_NTP_NIVEAU)),"Ntuples");
	InstrumOnChange(cNtuples,MonitNtupleChange,0);
	BoardAdd(bPrmDetailles,cNtuples->cdr,x,y,0); y += (InstrumRectLngr(&Gliss3lignes) + troislignes);
	y -= (2 * Dy); if(ybas < y) ybas = y;
	x += quinzelettres; if(xmax < x) xmax = x;

	ybas += Dy;
	SettingsOptVert1.dy = ybas;
	fig = FigureCreate(FIG_DROITE,(void *)&SettingsOptVert1,0,0);
	BoardAddFigure(bPrmDetailles,fig,WndColToPix(49),0,0);

	x = Dx; y = ybas + Dy;	
	//                                                                                 -------
	fig = FigureCreate(FIG_ZONE,(void *)(&SettingsTitreSecteur),0,0); FigureTexte(fig,"Actions");
	BoardAddFigure(bPrmDetailles,fig,x+32*Dx,y,0); y += (SettingsTitreSecteur.haut + Dy);
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsChargeReglages,L_("inchanges/charges")),L_("Reglages voies"));
	BoardAdd(bPrmDetailles,inst->cdr,x,y,0); x += quinzelettres;
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsChargeBolos,L_("inchangees/chargees")),L_("Consignes run"));
	BoardAdd(bPrmDetailles,inst->cdr,x,y,0); x += quinzelettres;
	InstrumTitle(cSansTpn = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsSansTrmt,L_("oui/non")),L_("Trmt.tampon"));
	BoardAdd(bPrmDetailles,cSansTpn->cdr,x,y,0); x += quinzelettres;
	InstrumTitle(cSauteEvt = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsSauteEvt,L_("non/oui")),L_("Saut evt"));
	BoardAdd(bPrmDetailles,cSauteEvt->cdr,x,y,0); x += quinzelettres;
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss3lignes,&RepartIpCorrection,L_(REPART_CORRECTION_CLES)),L_("Trames perdues"));
	BoardAdd(bPrmDetailles,inst->cdr,x,y,0); x += quinzelettres;
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsTramesMaxi,L_("standard/maxi")),L_("Remplissage"));
	BoardAdd(bPrmDetailles,inst->cdr,x,y,0); x += quinzelettres;
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&ExpertConnecte,L_("active/absente")),L_("Securite"));
	BoardAdd(bPrmDetailles,inst->cdr,x,y,0); x += quinzelettres;
//	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsDLUactive,L_("non/oui")),L_("Entretien detec."));
//	BoardAdd(bPrmDetailles,inst->cdr,x,y,0); x += (15 * Dx);
	y += (InstrumRectLngr(&Gliss2lignes) + troislignes);
	if(xmax < x) xmax = x;
	
	x = Dx; //- y += WndLigToPix(4);
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&VerifIpLog,L_("non/oui")),"Log Xmit IP");
	BoardAdd(bPrmDetailles,inst->cdr,x,y,0); x += quinzelettres;
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&VerifErreurLog,L_("non/oui")),L_("Detail episode"));
	BoardAdd(bPrmDetailles,inst->cdr,x,y,0); x += quinzelettres;
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsSecuPolar,L_("non/oui")),L_("Polar=0/erreur"));
	BoardAdd(bPrmDetailles,inst->cdr,x,y,0); x +=  quinzelettres;
	InstrumTitle(cSoustra = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss3lignes,&SettingsSoustra,L_(TRMT_PTN_TEXTES)),L_("Soust.pattern"));
	BoardAdd(bPrmDetailles,cSoustra->cdr,x,y-Dy,0); x += quinzelettres;
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&LectStoppeRepart,L_("absence/stop run")),L_("Arret repart"));
	BoardAdd(bPrmDetailles,inst->cdr,x,y,0); x += quinzelettres;
	BoardAddMenu(bPrmDetailles,mSettingsTriggerCtrl = MenuLocalise(iSettingsTriggerCtrl),x,y,0);
	OpiumSupport(mSettingsTriggerCtrl->cdr,WND_PLAQUETTE); x += (2 * quinzelettres);
	if(xmax < x) xmax = x;
	y += (InstrumRectLngr(&Gliss2lignes) + WndLigToPix(1));
	//- y += WndLigToPix(4);

	SettingsOptHori1.dx = xmax;
	fig = FigureCreate(FIG_DROITE,(void *)&SettingsOptHori1,0,0);
	BoardAddFigure(bPrmDetailles,fig,0,ybas,0);
	
	fig = FigureCreate(FIG_DROITE,(void *)&SettingsOptHori1,0,0);
	BoardAddFigure(bPrmDetailles,fig,0,y+Dy,0);

	x = Dx; y += 2*Dy;
	//                                                                                   ----------
	fig = FigureCreate(FIG_ZONE,(void *)(&SettingsTitreSecteur),0,0); FigureTexte(fig,L_("Sauvegarde"));
	BoardAddFigure(bPrmDetailles,fig,x+32*Dx,y,0); y += SettingsTitreSecteur.haut + Dy;
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsHdrGlobal,L_("locale/manip")),L_("Entete"));
	BoardAddInstrum(bPrmDetailles,inst,x,y-2*Dy,0);
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&ArchFormat,"edw1/edw2"),"Format");
	BoardAddInstrum(bPrmDetailles,inst,x,y+2*Dy,0);
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsRunFamille,L_("banc/manip")),L_("Nom de run"));
	BoardAdd(bPrmDetailles,inst->cdr,x,y+6*Dy,0); x += (15 * Dx);
	//	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss3lignes,&ArchModeStream,ArchClesStream),"streams");
	//	BoardAddInstrum(bPrmDetailles,inst,x,y+3*Dy,0); x += (15 * Dx);
	InstrumTitle(inst = InstrumList(INS_GLISSIERE,(void *)&Gliss6lignes,&ArchDetecMode,ArchModeNoms),L_("Evenements"));
	BoardAddInstrum(bPrmDetailles,inst,x,y-Dy,0); x += (15 * Dx);
	p = PanelCreate(1); PanelSupport(p,WND_CREUX); PanelInt(p,"Reduction",&ArchReduc); PanelItemLngr(p,1,4);
	BoardAddPanel(bPrmDetailles,p,x,y,0);
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&ArchAssocMode,L_("sans/avec")),L_("Associes"));
	BoardAddInstrum(bPrmDetailles,inst,x,y+4*Dy,0); x += (17 * Dx);
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&ArchSauveTrace,L_("non/oui")),L_("Traces evts"));
	BoardAddInstrum(bPrmDetailles,inst,x,y,0);
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss3lignes,&ArchSauveNtp,L_("sans/ASCII/PAW")),"MonitNtp evt");
	BoardAddInstrum(bPrmDetailles,inst,x,y+3*Dy,0); x += (15 * Dx);
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&BitTriggerNiveau,SAMBA_BITNIV_CLES),L_("Niveau BitTrig"));
	BoardAddInstrum(bPrmDetailles,inst,x,y+Dy,0);
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&BitTriggerNums,SAMBA_BITNUM_CLES),"Perim. BitTrig");
	BoardAddInstrum(bPrmDetailles,inst,x,y+4*Dy,0); x += (15 * Dx);
	p = PanelCreate(1); PanelSupport(p,WND_CREUX); PanelInt(p,"Bits/detec",&BitTriggerBloc); PanelItemLngr(p,1,3);
	BoardAddPanel(bPrmDetailles,p,x,y,0);
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&LectSauveStatus,L_("perdu/sauve")),L_("Status Numeriseurs"));
	BoardAddInstrum(bPrmDetailles,inst,x+3*Dx,y+4*Dy,0);
	PanelMode(p = PanelCreate(1),PNL_DIRECT); PanelSupport(p,WND_CREUX);
	PanelInt(p,"Export(ms)",&ExportPack[0].periode); PanelItemILimits(p,1,1,10000); PanelItemFormat(p,1,"%d"); PanelItemLngr(p,1,5);
	BoardAddPanel(bPrmDetailles,p,x-2*Dx,y+7*Dy,0); x += (15 * Dx);
	if(xmax < x) xmax = x;
	x = 15 * Dx;
	PanelMode(p = PanelCreate(1),PNL_DIRECT); PanelKeyB(p,L_("Type de run"),RunCategCles,&RunCategNum,5); PanelSupport(p,WND_CREUX);
	BoardAddPanel(bPrmDetailles,p,x-(2*Dx),y+7*Dy,0); x += (20 * Dx);
	y += (9 * Dy);

	fig = FigureCreate(FIG_DROITE,(void *)&SettingsOptHori1,0,0);
	BoardAddFigure(bPrmDetailles,fig,0,y,0);

	x = Dx; y += Dy;
	//                                                                                    -------
	fig = FigureCreate(FIG_ZONE,(void *)(&SettingsTitreSecteur),0,0); FigureTexte(fig,L_("Limites"));
	BoardAddFigure(bPrmDetailles,fig,x+32*Dx,y,0); y += SettingsTitreSecteur.haut;
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&LectDureeActive,L_("libre/limitee")),L_("Lecture"));
	BoardAddInstrum(bPrmDetailles,inst,x,y,0); x += (15 * Dx);
	p = PanelCreate(4); PanelColumns(p,2);
	PanelInt  (p,L_("Duree maximum (mn)"),&LectDureeMaxi);
	PanelInt  (p,L_("Duree fichier (mn)"),&LectDureeTranche);
	PanelFloat(p,L_("Taux maxi d'evenements (Hz)"),&LectTauxSeuil);
	PanelFloat(p,L_("Delai mini entre evenements (ms)"),&LectDelaiMini);
	PanelSupport(p,WND_CREUX);
	BoardAddPanel(bPrmDetailles,p,x,y,0);
	x += (80 * Dx);
	if(xmax < x) xmax = x;
	y += (3 * Dy);
	
	fig = FigureCreate(FIG_DROITE,(void *)&SettingsOptHori1,0,0);
	BoardAddFigure(bPrmDetailles,fig,0,y+Dy,0);
	
	x = Dx; y += 2*Dy;
	//                                                                                   -------------
	fig = FigureCreate(FIG_ZONE,(void *)(&SettingsTitreSecteur),0,0); FigureTexte(fig,L_("Algorithmique"));
	BoardAddFigure(bPrmDetailles,fig,x+32*Dx,y,0); y += (SettingsTitreSecteur.haut + Dy);
	//- InstrumTitle(cSrcType = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss5lignes,&SrceType,SrceCles),"Source");
	//-- InstrumOnChange(cSrcType,LectArchPrepare,(void *)1); /* chgmt stream au risques & perils de l'utilisateur..! */
	//- BoardAdd(bFinesses,cSrcType->cdr,x,y,0); x += quinzelettres;
	InstrumTitle(cMultitasks = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss5lignes,&SettingsMultitasks,L_("mot/mono/std/opt/v4")),L_("Lecture"));
	BoardAdd(bPrmDetailles,cMultitasks->cdr,x,y-(2*Dy),0); x += quinzelettres;

	cPeriodes = InstrumInt(INS_GLISSIERE,(void *)&Gliss5lignes,&SettingsIntervTrmt,1,1000);
	PanelMode(pPeriodes = PanelCreate(1),PNL_DIRECT);
	PanelItemExit(pPeriodes,PanelInt(pPeriodes,0,&SettingsIntervTrmt),SambaUpdateInstrumFromItem,cPeriodes);
	PanelItemILimits(pPeriodes,1,1,1000);
	PanelItemFormat(pPeriodes,1,"%3d"); PanelItemLngr(pPeriodes,1,5);
	PanelSupport(pPeriodes,WND_CREUX);
	InstrumOnChange(cPeriodes,SambaUpdatePanel,pPeriodes);
	InstrumTitle(cPeriodes,L_("Traitmt(ms)"));
	BoardAdd(bPrmDetailles,cPeriodes->cdr,x,y-(2*Dy),0);
	BoardAdd(bPrmDetailles,pPeriodes->cdr,x,y+InstrumRectLngr(&Gliss5lignes),0); x += quinzelettres;

	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss3lignes,&LectSynchroType,L_(LECT_SYNCHRO_CLES)),L_("Demarrage"));
	BoardAdd(bPrmDetailles,inst->cdr,x,y,0);
	InstrumTitle(cModeLecture = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss3lignes,&LectCntl.LectMode,L_("donnees/ident/test")),"Mode");
	BoardAddInstrum(bPrmDetailles,cModeLecture,x,y,0); x += quinzelettres;
    InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&TrgrGardeSigne,L_("non/oui")),L_("Signe conserve"));
    BoardAdd(bPrmDetailles,inst->cdr,x,y+Dy,0); x += quinzelettres;
	InstrumTitle(cImprBolos = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsImprBolosVsn,"BBv1/mixte"),L_("Impr.detec."));
	BoardAdd(bPrmDetailles,cImprBolos->cdr,x,y+Dy,0); x += quinzelettres;
//-	InstrumTitle(cTrigger = InstrumListB(INS_GLISSIERE,(void *)&Gliss4lignes,&Trigger.type,TrgrDefListe),"Trigger");
//-	BoardAdd(bPrmDetailles,cTrigger->cdr,x,y-Dy,0);
//>	BoardAddMenu(bPrmDetailles,mSettingsTriggerCtrl = MenuLocalise(iSettingsTriggerCtrl),x,y+(InstrumRectLngr(&Gliss4lignes) + Dy),0);
//>	OpiumSupport(mSettingsTriggerCtrl->cdr,WND_PLAQUETTE); x += quinzelettres;
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&LectRetardOption,L_("absent/present")),L_("Retardateur"));
	InstrumOnChange(inst,LectRenovePlanche,0);
	BoardAdd(bPrmDetailles,inst->cdr,x,y-(2*Dy),0); x += quinzelettres;
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsTrigInOut,L_("absent/present")),"Trig in/out");
	InstrumOnChange(inst,SettingsTrgrAffiche,0);
	BoardAdd(bPrmDetailles,inst->cdr,x,y-(2*Dy),0); x += quinzelettres;

	x = Dx; y += InstrumRectLngr(&Gliss5lignes);

	InstrumTitle(cModeDonnees = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsModeDonnees,L_("fixe/circulaire")),L_("Tampon"));
	BoardAdd(bPrmDetailles,cModeDonnees->cdr,x,y,0); x += quinzelettres; x += quinzelettres;
	InstrumTitle(cIdentCheck = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsIdentCheck,L_("zappe/actif")),"Test ident");
	BoardAdd(bPrmDetailles,cIdentCheck->cdr,x,y,0); x += quinzelettres;
	InstrumTitle(cStatusCheck = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsStatusCheck,L_("zappe/actif")),"Test status");
	BoardAdd(bPrmDetailles,cStatusCheck->cdr,x,y,0); x += quinzelettres;
	InstrumTitle(cTrmtInline = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsInlineProcess,L_("non/actif")),"Test inline");
	BoardAdd(bPrmDetailles,cTrmtInline->cdr,x,y,0); x += quinzelettres;
	InstrumTitle(inst = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&OrgaStyle,L_("boutons/icones")),L_("Organigramme"));
	InstrumOnChange(inst,OrgaMagasinRefait,0);
	BoardAdd(bPrmDetailles,inst->cdr,x,y-(3*Dy),0);
	InstrumTitle(cRunFamille = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SambaExploreHW,L_("fixe/demande")),L_("Choix repart."));
	BoardAdd(bPrmDetailles,cRunFamille->cdr,x,y,0); x += quinzelettres;
	InstrumTitle(cGenePresent = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SettingsGenePossible,L_("ignoree/possible")),"Generation");
	InstrumOnChange(cGenePresent,GegeneAutorise,0);
	BoardAdd(bPrmDetailles,cGenePresent->cdr,x,y-(3*Dy),0);
	InstrumTitle(cGenePresent = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&SambaChassisOptimal,L_("inchange/optimise")),"Chassis");
	InstrumOnChange(cGenePresent,GegeneAutorise,0);
	BoardAdd(bPrmDetailles,cGenePresent->cdr,x,y,0); x += quinzelettres;

	x = Dx; y += (InstrumRectLngr(&Gliss2lignes) + WndLigToPix(1));
	
	fig = FigureCreate(FIG_DROITE,(void *)&SettingsOptHori1,0,0);
	BoardAddFigure(bPrmDetailles,fig,0,y+Dy,0);
	
	//1 BoardAddPoussoir(bPrmDetailles,L_("Securite",&ExpertConnecte,Dx,y+2*Dy,0);
	//2 InstrumTitle(inst = InstrumKeyB(INS_POTAR,(void *)&SettingsContact,&ExpertConnecte,L_("controle/libre")),0);
	//2 BoardAdd(bPrmDetailles,inst->cdr,10*Dx,y+Dy,0);
	BoardAddBouton(bPrmDetailles,L_("Sauver"),SettingsSauveTout,(xmax-6*Dx)/2,y+2*Dy,0);
	OpiumEnterFctn(bPrmDetailles,AccesRestreint,0);
	OpiumExitFctn(bPrmDetailles,SettingsControleFerme,0);

	if(affiche) OpiumUse(bPrmDetailles,OPIUM_FORK);
	return(0);
}
/* ========================================================================== */
int SettingsSetup() {
	int volet,cap;

	if(StartFromScratch) return(1);

	/* Planche OPTIONS */
	bPrmCourants = BoardCreate(); SettingsOptionsConstruit(0,0);

	/* Planche CONTROLE */
	bPrmDetailles = BoardCreate(); SettingsControleConstruit(0,0);

	/* Planche generale, sauf pour la definition des evenements */
	pSettingsDetec = PanelCreate(1);
	PanelList(pSettingsDetec,L_("Detecteur"),BoloNom,&BoloNum,DETEC_NOM);

	// PRINT_GC(iSettingsDetecReel[0].gc);
	for(volet=SETTINGS_TRMT; volet<SETTINGS_NBVOLETS; volet++) {
		SettingsVolet[volet].bolo = BoloNum;
		SettingsVolet[volet].standard_a_jour = 1;
		SettingsVolet[volet].planche = BoardCreate();
		SettingsVolet[volet].general_std = iSettingsDetecStd;
		SettingsVolet[volet].general_det = iSettingsDetecReel;
		SettingsVolet[volet].general = 0;
		SettingsVolet[volet].config = 0;
		SettingsVolet[volet].creation = 0;
		SettingsVolet[volet].standardise = 0;
		SettingsVolet[volet].standard2 = 0;
		for(cap=0; cap<DETEC_CAPT_MAX; cap++) SettingsVolet[volet].menu_voie[cap] = 0;
	}
	SettingsVolet[SETTINGS_TRMT].creation    = SettingsDetecTrmtCree;
	SettingsVolet[SETTINGS_TRMT].standardise = SettingsDetecTrmtVolStandardise;
	SettingsVolet[SETTINGS_TRMT].standard2   = SettingsDetecTrmtTpnStandardise;
	SettingsVolet[SETTINGS_TRGR].creation    = SettingsDetecTrgrCree;
	SettingsVolet[SETTINGS_TRGR].standardise = SettingsDetecTrgrStandardise;
	//	SettingsVolet[SETTINGS_COUP].creation    = SettingsDetecCoupCree;
	//	SettingsVolet[SETTINGS_COUP].standardise = SettingsDetecCoupStandardise;
	SettingsVolet[SETTINGS_RGUL].creation    = SettingsDetecRgulCree;
	SettingsVolet[SETTINGS_RGUL].standardise = SettingsDetecRgulStandardise;
	SettingsVolet[SETTINGS_RGUL].creation    = SettingsDetecCatgCree;
	SettingsVolet[SETTINGS_RGUL].standardise = SettingsDetecCatgStandardise;

	for(volet=SETTINGS_TRMT; volet<SETTINGS_NBVOLETS; volet++) SettingsDetecCree(volet,0,0);

	/* int num; for(num=0; num<DETEC_CAPT_MAX; num++) {
		pTrmt[TRMT_AU_VOL][num] = pTrmt[TRMT_PRETRG][num] = 0;
		pTrgr[num] = 0;
		pRegulParms[num] = pRegulPlafs[num] = 0;
	} */

	/* Planche bDefEvts, speciale definition des evenements */
	bDefEvts = BoardCreate();
	pSettingsDetecEvts = PanelCreate(1);
	PanelList(pSettingsDetecEvts,L_("Detecteur"),BoloNom,&BoloNum,DETEC_NOM);
	PanelOnApply(pSettingsDetecEvts,SettingsDetecEvtsCree,0);
	PanelSupport(pSettingsDetecEvts,WND_CREUX);
	SettingsEvtsNb = 0;
	SettingsDetecEvtsCree(0,0);

	return(1);
}
/* ========================================================================== */
int SettingsExit() {
	int rc;

//	printf("> (%s.1) SettingsParmsASauver @%08X: %d\n",__func__,(hexa)(&SettingsParmsASauver),SettingsParmsASauver);
	rc = SambaSauve(&SettingsParmsASauver,L_("parametrage"),0,0,SettingsParmsEcrit);
//	printf("> (%s.2) SettingsParmsASauver @%08X: %d\n",__func__,(hexa)(&SettingsParmsASauver),SettingsParmsASauver);
	if(!rc) return(0);
	return(SambaSauve(&SettingsFinessesASauver,L_("fignolage"),0,0,SettingsFinessesEcrit));
}
