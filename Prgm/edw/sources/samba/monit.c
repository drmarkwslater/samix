#define BRANCHE_MONIT
#ifdef macintosh
	#pragma message(__FILE__)
#endif

#define MONIT_C

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <environnement.h>
#include <dateheure.h>
#include <claps.h>
#include <decode.h>
#include <opium_demande.h>
#include <opium.h>
// int GraphDataRGB(Graph graph, int num, WndColorLevel r, WndColorLevel g, WndColorLevel b);
#include <samba.h>
#include <repartiteurs.h>
#include <gene.h>
#include <objets_samba.h>
#include <monit.h>

// A recuperer de MonitFenTypeName, genre ArgKeyFromList(MonitFenTypeKeys,MonitFenTypeName)
#define MONIT_FEN_TYPES "signal/histo/FFT/fonction/2D-histo/biplot/evenement/evtmoyen/pattern"

static int MonitFenTypeAvecTrmt[] = { 1, 0, 1, 0, 0, 0, 1, 0, 0 };
static int MonitFenTypeAvecVar[]  = { 0, 1, 0, 1, 1, 1, 0, 0, 0 };

static char   MonitASauver;
static Panel pMonitGeneral; // pFichierMonitLect,pFichierMonitEcri,
static Panel pMonitDivers;
static Panel pMonitLimites1Voie;
static Menu  mMonitEvts,mMonitSncf;

static int MonitFftBolo,MonitFftModl;
static Panel pMonitFftAuVol;
static char *MonitFftSuite[] = { "Arreter", "Recommencer", "\0" };

static Menu mMonitRaies;
static Panel pMonitRaies;

static Panel pMonitHistosDef,pMonitHistosDyn;
static Term  tMonit;
static char   MonitTitre[80];
static WndColor *MonitLUTred,*MonitLUTall;
static char   MonitUser[MAXFILENAME];
static char   MonitNomTypes[256];
static short  MonitEvtIndex[2];
static char   MonitColrFig[WND_NBQUAL][16],MonitColrFit[WND_NBQUAL][16];

/* #ifdef X11
#define Jaune "yellow"
#define Vert  "green"
#define Bleu  "blue"
#endif
#ifdef QUICKDRAW
#define Jaune "FFFFFFFF0000"
#define Vert  "0000FFFF0000"
#define Bleu  "00000000FFFF"
#endif */

static float MonitDuree;   /* duree de donnees brutes affichees (ms) (voie par voie) */
static ArgDesc MonitDesc[] = {
	{ DESC_COMMENT 0 },
	{ DESC_COMMENT "Parametres generaux" },
	{ "Graphiques.hauteur",   DESC_INT    &MonitHaut,          "Hauteur des graphiques (pixels)" },
	{ "Graphiques.largeur",   DESC_INT    &MonitLarg,          "Largeur des graphiques (pixels)" },
	{ "Graphique.synchro",    DESC_BOOL   &MonitSynchro,       "Synchronisation avec la synchronisation" },
	{ "Donnees.duree",        DESC_FLOAT  &MonitDuree,         "Duree de donnees affichee (ms)" },
	{ "Donnees.unite.chgt",   DESC_KEY MONIT_UNITE_CLES, &MonitChgtUnite, "Changement d'unite des donnees ("MONIT_UNITE_CLES")" },
	{ "Donnees.unite.mV",     DESC_FLOAT  &MonitADUenmV,       "Valeur d'un ADU en mV" },
	{ "Donnees.unite.keV",    DESC_FLOAT  &MonitADUenkeV,      "Valeur d'un ADU en keV" },
	{ "Donnees.evts",         DESC_NONE }, // DESC_INT   &MonitEvts,          "Nombre d'evenements maxi affiches en histo" },
	{ DESC_COMMENT 0 },
	{ DESC_COMMENT "Contenu de l'ecran" },
	{ "Affichage.donnees",    DESC_TEXT    MonitUser,          "Fichier de definition de l'affichage des graphiques utilisateurs" },
	{ "Affichage.interv",     DESC_INT    &MonitIntervAff,     "Intervalle entre affichage des donnees (ms)" },
	{ "Affichage.modes",      DESC_BOOL   &MonitAffModes,      "Affichage des modes de fonctionnement" },
	{ "Affichage.trigger",    DESC_BOOL   &MonitAffTrig,       "Affichage du trigger demande" },
	{ "Affichage.ldb",        DESC_BOOL   &MonitAffBases,      "Affichage des lignes de base" },
	{ "Affichage.seuils",     DESC_BOOL   &MonitAffgSeuils,    "Affichage du graphique des seuils" },
	{ "Affichage.etat",       DESC_BOOL   &MonitAffEtat,       "Affichage de l'etat de l'acquisition" },
	{ "Affichage.reseau",     DESC_BOOL   &MonitAffNet,        "Affichage de l'etat du reseau" },
	{ "Affichage.evt",        DESC_BOOL   &MonitAffEvts,       "Affichage du dernier evenement" },
	{ "Affichage.taux.panel", DESC_BOOL   &MonitAffpTaux,      "Affichage du panel des taux d'evenements" },
	{ "Affichage.taux.graph", DESC_BOOL   &MonitAffgTaux,      "Affichage du graphique des taux d'evenements" },
	{ "Affichage.classes",    DESC_BOOL   &MonitEvtClasses,    "Affichage de la planche des evenements par classe" },
	{ DESC_COMMENT 0 },
	{ DESC_COMMENT "Modes pour les evenements" },
	{ "Evt.fenetre_integree", DESC_BOOL   &MonitEvtAvecRun,    "Fenetre du dernier evenement integree a la planche du run" },
	{ "Evt.sonorisation",     DESC_BOOL   &MonitModeSon,       "Sonorisation en fonction de la categorie d'evenement (si definie)" },
	{ "Evt.Y.automatique",    DESC_BOOL   &MonitModeAuto,      "Vrai si calcul automatique des extremas en Y pour chaque evenement" },
	{ DESC_COMMENT "Si echelle Y fixe" },
	{ "Evt.Ymin",             DESC_INT    &MonitModeYmin,      "Valeur du minimum en Y pour chaque evenement" },
	{ "Evt.Ymax",             DESC_INT    &MonitModeYmax,      "Valeur du maximum en Y pour chaque evenement" },
	// { DESC_COMMENT 0 },
	// { DESC_COMMENT "Planche de suivi du taux d'evenement" },
	{ "Evt.planche_taux",     DESC_NONE }, // DESC_DEVIENT("Evt.planche.taux") },
	{ "Evt.planche_largeur",  DESC_NONE }, // DESC_DEVIENT("Evt.planche.categ.largeur") },
	{ "Evt.planche_hauteur",  DESC_NONE }, // DESC_DEVIENT("Evt.planche.categ.hauteur") },
	{ "Evt.planche.taux",     DESC_NONE }, // DESC_DEVIENT("Evt.planche.affiche") },
	{ "Evt.planche.largeur",  DESC_NONE }, // DESC_DEVIENT("Evt.planche.categ.largeur") },
	{ "Evt.planche.hauteur",  DESC_NONE }, // DESC_DEVIENT("Evt.planche.categ.hauteur") },
	{ "Evt.planche.affiche",  DESC_DEVIENT("Affichage.classes") }, // DESC_BOOL  &MonitEvtClasses,    "Affichage de la planche des taux d'evenements" },
	{ "Evt.planche.categ.largeur",  DESC_NONE }, // DESC_INT   &MonitEvtCatgLarg,   "Largeur des graphiques par categorie dans la planche des taux d'evenements" },
	{ "Evt.planche.categ.hauteur",  DESC_NONE }, // DESC_INT   &MonitEvtCatgHaut,   "hauteur des graphiques par categorie dans la planche des taux d'evenements" },
	{ "Evt.planche.focus.largeur",  DESC_NONE }, // DESC_INT   &MonitEvtFocsLarg,   "Largeur des graphiques cibles dans la planche des taux d'evenements" },
	{ "Evt.planche.focus.hauteur",  DESC_NONE }, // DESC_INT   &MonitEvtFocsHaut,   "hauteur des graphiques cibles dans la planche des taux d'evenements" },
	{ DESC_COMMENT 0 },
	{ DESC_COMMENT "Spectres au vol" },
	{ "Bolo.num",             DESC_INT    &MonitFftBolo,          "Numero de detecteur a etudier" },
	{ "Bolo.voie",            DESC_INT    &MonitFftModl,          "Modele de voie de ce detectur" },
	{ "Longueur",             DESC_INT    &MonitFftDim,           "Nombre de points de la FFT" },
	{ "ModeAuVol",            DESC_NONE },
	{ "Mode.auvol",           DESC_KEY "manu/surv/auto/glissant", &MonitFftMode,       "Methode de calcul de spectre au vol (manu/surv/auto/glissant)" },
	{ "Axe.X.type",           DESC_KEY "lineaire/log",    &MonitFftParm[0].log,       "Type d'axe pour les X (lineaire/log)" },
	{ "Axe.X.limites",        DESC_KEY "manu/auto",       &MonitFftParm[0].autom,     "Limitation de l'axe X (manu/auto)" },
	{ "Axe.X.min",            DESC_FLOAT                  &MonitFftParm[0].lim.r.min, "Minimum sur l'axe des X, si limites manuelles" },
	{ "Axe.X.max",            DESC_FLOAT                  &MonitFftParm[0].lim.r.max, "Maximum sur l'axe des X, si limites manuelles" },
	{ "Axe.Y.type",           DESC_KEY "lineaire/log",    &MonitFftParm[1].log,       "Type d'axe pour les Y (lineaire/log)" },
	{ "Axe.Y.limites",        DESC_KEY "manu/auto",       &MonitFftParm[1].autom,     "Limitation de l'axe Y (manu/auto)" },
	{ "Axe.Y.min",            DESC_FLOAT                  &MonitFftParm[1].lim.r.min, "Minimum sur l'axe des Y, si limites manuelles" },
	{ "Axe.Y.max",            DESC_FLOAT                  &MonitFftParm[1].lim.r.max, "Maximum sur l'axe des Y, si limites manuelles" },
	{ DESC_COMMENT 0 },
	{ DESC_COMMENT "Suivi de spectres" },
	{ "Pics.dim",             DESC_INT    &PicSuiviDim,        "Dimension du suivi (echantillons)" },
	{ "Pics.nb",              DESC_INT    &PicsNb,             "Nombre de pics a suivre" },
	{ "Pics.1.min",           DESC_SHORT  &Pic[0].min,         "Minimum du pic #1 (ADU)" },
	{ "Pics.1.max",           DESC_SHORT  &Pic[0].max,         "Maximum du pic #1 (ADU)" },
	{ "Pics.2.min",           DESC_SHORT  &Pic[1].min,         "Minimum du pic #2 (ADU)" },
	{ "Pics.2.max",           DESC_SHORT  &Pic[1].max,         "Maximum du pic #2 (ADU)" },
	{ "Pics.3.min",           DESC_SHORT  &Pic[2].min,         "Minimum du pic #3 (ADU)" },
	{ "Pics.3.max",           DESC_SHORT  &Pic[2].max,         "Maximum du pic #3 (ADU)" },
	{ DESC_COMMENT 0 },
	{ DESC_COMMENT "Histogrammes predefinis" },
	{ "Histo.ampl",           DESC_BOOL   &MonitHampl,         "Affichage de l'histogramme des amplitudes" },
	{ "Histo.ampl.min",       DESC_INT    &(MonitHamplY[1]),   "Nb evts minimum pour l'histogramme des amplitudes" },
	{ "Histo.ampl.max",       DESC_INT    &(MonitHamplY[1]),   "Nb evts maximum pour l'histogramme des amplitudes" },
	{ "Histo.montee",         DESC_BOOL   &MonitHmontee,       "Affichage de l'histogramme des temps de montee" },
	{ "Histo.montee.min",     DESC_INT    &(MonitHmonteeY[0]), "Nb evts minimum pour l'histogramme des temps de montee" },
	{ "Histo.montee.max",     DESC_INT    &(MonitHmonteeY[1]), "Nb evts maximum pour l'histogramme des temps de montee" },
	{ "Histo.2D",             DESC_BOOL   &MonitH2D,           "Affichage de l'histo amplitude x tps montee" },
	{ "Histo.2D.min",         DESC_INT    &(MonitH2DY[0]),     "Nb evts minimum pour l'histo amplitude x tps montee" },
	{ "Histo.2D.max",         DESC_INT    &(MonitH2DY[1]),     "Nb evts maximum pour l'histo amplitude x tps montee" },

	DESC_END
};

/* ========================================================================== */

/* ................ Parametres communs a toutes les fenetres ................ */

static ArgDesc MonitFenPosDesc[] = {
	{ "x",        DESC_INT   &(MonitFenLue.posx), 0 },
	{ "y",        DESC_INT   &(MonitFenLue.posy), 0 },
	DESC_END
};
static ArgDesc MonitFenDimDesc[] = {
	{ "l",        DESC_INT   &(MonitFenLue.larg), 0 },
	{ "h",        DESC_INT   &(MonitFenLue.haut), 0 },
	DESC_END
};

/* ... Complements a la definition generale de la fenetre, en fonction de son type ... */

static ArgDesc MonitFenComplSignalDesc[] = {
	{ "min",      DESC_INT   &(MonitFenLue.axeY.i.min), 0 },
	{ "max",      DESC_INT   &(MonitFenLue.axeY.i.max), 0 },
	{ "duree_ms", DESC_FLOAT &(MonitFenLue.axeX.r.max), 0 },
	DESC_END
};
static ArgDesc MonitFenComplHistoDesc[] = {
	{ "bin_min",  DESC_FLOAT &(MonitFenLue.axeX.r.min), 0 },
	{ "bin_max",  DESC_FLOAT &(MonitFenLue.axeX.r.max), 0 },
	{ "nb.bins",  DESC_INT   &(MonitFenLue.axeX.pts), 0 },
	{ "max.evts", DESC_INT   &(MonitFenLue.axeY.i.max), 0 },
	DESC_END
};
static ArgDesc MonitFenComplFftDesc[] = {
	{ "nb.points",   DESC_INT   &(MonitFenLue.axeX.pts), 0 },
	{ "freq_min",    DESC_FLOAT &(MonitFenLue.axeX.r.min), 0 },
	{ "freq_max",    DESC_FLOAT &(MonitFenLue.axeX.r.max), 0 },
	{ "bruit_min",   DESC_FLOAT &(MonitFenLue.axeY.r.min), 0 },
	{ "bruit_max",   DESC_FLOAT &(MonitFenLue.axeY.r.max), 0 },
	{ "intervalles", DESC_INT   &(MonitFenLue.p.f.intervalles), 0 },
	{ "axe_X",       DESC_KEY "lineaire/log", &(MonitFenLue.p.f.xlog), 0 },
	{ "hors_evts",   DESC_KEY "non/active",   &(MonitFenLue.p.f.excl), 0 },
	DESC_END
};
static ArgDesc MonitFenComplFonctionDesc[] = {
	{ "min",      DESC_FLOAT &(MonitFenLue.axeY.r.min), 0 },
	{ "max",      DESC_FLOAT &(MonitFenLue.axeY.r.max), 0 },
	{ "nb.evts",  DESC_INT   &(MonitFenLue.axeX.i.max), 0 },
	DESC_END
};
static ArgDesc MonitFenComplBiplotDesc[] = {
	DESC_END
};
static ArgDesc MonitFenComplH2dDesc[] = {
	DESC_END
};
static ArgDesc MonitFenComplEvtDesc[] = {
	{ "min",      DESC_INT   &(MonitFenLue.axeY.i.min), 0 },
	{ "max",      DESC_INT   &(MonitFenLue.axeY.i.max), 0 },
	{ "duree_ms", DESC_FLOAT &(MonitFenLue.axeX.r.max), 0 },
	DESC_END
};
static ArgDesc MonitFenComplMoyenDesc[] = {
	{ "min", DESC_INT &(MonitFenLue.axeY.i.min), 0 },
	{ "max", DESC_INT &(MonitFenLue.axeY.i.max), 0 },
	DESC_END
};
static ArgDesc MonitFenComplPatternDesc[] = {
	{ "min", DESC_INT &(MonitFenLue.axeY.i.min), 0 },
	{ "max", DESC_INT &(MonitFenLue.axeY.i.max), 0 },
	DESC_END
};
// "signal/histo/FFT/fonction/2D-histo/biplot/evenement/evtmoyen/pattern"
static ArgDesc *MonitFenComplDesc[] = {
	MonitFenComplSignalDesc,     MonitFenComplHistoDesc,    MonitFenComplFftDesc,
	MonitFenComplFonctionDesc,   MonitFenComplH2dDesc,      MonitFenComplBiplotDesc,
	MonitFenComplEvtDesc,        MonitFenComplMoyenDesc,    MonitFenComplPatternDesc,
	0
};

/* ......... Definition des traces en fonction du type de la fenetre ........ */

static TypeMonitTrace MonitTraceLue;

/* ... Nom de la voie (detecteur + capteur) ... */
static ArgDesc MonitTraceBoloCaptDesc[] = {
//!	{ "detecteur", DESC_LISTE BoloNom,           &MonitTraceLue.bolo, 0 },
//!	{ DESC_EXEC MonitTraceListeCapt, pointeur_sur_donnees }, // void MonitTraceListeCapt(void *pointeur_sur_donnees)
//!	{ "capteur",   DESC_STR_DIM(MODL_NOM,DETEC_CAPT_MAX) MonitTraceLue.liste_voies, 0 },
	{ "capteur",   DESC_STR(MODL_NOM)   MonitTraceLue.voie_lue, 0 },
	{ "detecteur", DESC_LISTE BoloNom, &MonitTraceLue.bolo, 0 },
	DESC_END
}; static ArgDesc *MonitTraceBoloCaptDescPtr = MonitTraceBoloCaptDesc;

/* ... Couleur de la trace ... */
static ArgDesc MonitTracePcentDesc[] = {
	{ "rouge",   DESC_FLOAT &(MonitTraceLue.rgb.r), 0 },
	{ "vert",    DESC_FLOAT &(MonitTraceLue.rgb.g), 0 },
	{ "bleu",    DESC_FLOAT &(MonitTraceLue.rgb.b), 0 },
	DESC_END
};
static ArgDesc MonitTraceColorDesc[] = {
	{ "couleur", DESC_STRUCT &MonitTracePcentDesc, 0 },
	DESC_END
}; static ArgDesc *MonitTraceColorDescPtr = MonitTraceColorDesc;

/* ........... Liste des traces en fonction du type de la fenetre ........... */

/* ... Donnee des tampons, brute ou traitee: "Signal" ... */
static ArgDesc MonitTraceSignalDesc[] = {
	DESC_INCLUDE(MonitTraceBoloCaptDescPtr),
	{ "etat",    DESC_LISTE TrmtClassesListe, &(MonitTraceLue.var), 0 }, // nom du tampon
	DESC_INCLUDE(MonitTraceColorDescPtr),
	DESC_END
};
static ArgStruct MonitFenTracesSignalAS  = { (void *)MonitFenLue.trace, (void *)&MonitTraceLue, sizeof(TypeMonitTrace), MonitTraceSignalDesc };
static ArgDesc MonitFenTracesSignalDesc[] = {
	{ "donnees", DESC_STRUCT_SIZE(MonitFenLue.nb,MAXTRACES) &MonitFenTracesSignalAS, 0 },
	DESC_END
};

/* ... Histogramme d'un ntuple: "Histo" ... */
static ArgDesc MonitTraceVarsDesc[] = {
	{ "var",     DESC_LISTE VarName, &MonitTraceLue.var, 0 }, // nom du ntuple
	DESC_INCLUDE(MonitTraceBoloCaptDescPtr),
	DESC_INCLUDE(MonitTraceColorDescPtr),
	DESC_END
};
static ArgStruct MonitFenTracesHistoAS  = { (void *)MonitFenLue.trace, (void *)&MonitTraceLue, sizeof(TypeMonitTrace), MonitTraceVarsDesc };
static ArgDesc MonitFenTracesHistoDesc[] = {
	{ "donnees", DESC_STRUCT_SIZE(MonitFenLue.nb,MAXTRACES) &MonitFenTracesHistoAS, 0 },
	{ "classe",  DESC_STR(CATG_NOM) &(MonitFenLue.nom_catg), 0 },
	DESC_END
};

/* ... Resultat d'un traitement des donnees (pattern ou evt moyen): "Resul" ... */
static ArgDesc MonitTraceResulDesc[] = {
	DESC_INCLUDE(MonitTraceBoloCaptDescPtr),
	DESC_INCLUDE(MonitTraceColorDescPtr),
	DESC_END
};
static ArgStruct MonitFenTracesResulAS  = { (void *)MonitFenLue.trace, (void *)&MonitTraceLue, sizeof(TypeMonitTrace), MonitTraceResulDesc };
static ArgDesc MonitFenTracesResulDesc[] = {
	{ "donnees", DESC_STRUCT_SIZE(MonitFenLue.nb,MAXTRACES) &MonitFenTracesResulAS, 0 },
	DESC_END
};

/* ... Histo 2D: on doit avoir 2 et seulement 2 "traces" de type variable de voie ... */
static ArgDesc MonitTraceXvarDesc[] = {
	{ "var",       DESC_LISTE VarName,         &(MonitFenLue.trace[0].var),     0 }, // nom de la variable
	{ "capteur",   DESC_STR(MODL_NOM)            MonitFenLue.trace[0].voie_lue, 0 },
	{ "detecteur", DESC_LISTE BoloNom,         &(MonitFenLue.trace[0].bolo),    0 },
	{ "bin_min",   DESC_FLOAT                  &(MonitFenLue.axeX.r.min),       0 },
	{ "bin_max",   DESC_FLOAT                  &(MonitFenLue.axeX.r.max),       0 },
	{ "nb.bins",   DESC_INT                    &(MonitFenLue.axeX.pts),         0 },
	DESC_END
};
static ArgDesc MonitTraceYvarDesc[] = {
	{ "var",       DESC_LISTE VarName,         &(MonitFenLue.trace[1].var),     0 }, // nom de la variable
	{ "capteur",   DESC_STR(MODL_NOM)            MonitFenLue.trace[1].voie_lue, 0 },
	{ "detecteur", DESC_LISTE BoloNom,         &(MonitFenLue.trace[1].bolo),    0 },
	{ "bin_min",   DESC_FLOAT                  &(MonitFenLue.axeY.r.min),       0 },
	{ "bin_max",   DESC_FLOAT                  &(MonitFenLue.axeY.r.max),       0 },
	{ "nb.bins",   DESC_INT                    &(MonitFenLue.axeY.pts),         0 },
	DESC_END
};
static ArgDesc MonitFenTracesH2dDesc[] = {
	{ "X",       DESC_STRUCT &MonitTraceXvarDesc, 0 },
	{ "Y",       DESC_STRUCT &MonitTraceYvarDesc, 0 },
	{ "classe",  DESC_STR(CATG_NOM) &(MonitFenLue.nom_catg), 0 },
	DESC_END
};

/* ... Bi-plot: on doit avoir 2 et seulement 2 "traces" de type MonitNtp ... */
/* PAS POSSIBLE AVEC PlotVarList DYNAMIQUE:
static ArgDesc MonitTraceXntpDesc[] = {
	{ 0,         DESC_LISTE PlotVarList, &(MonitFenLue.trace[0].var),     0 }, // nom du MonitNtp
	{ "min",     DESC_FLOAT &(MonitFenLue.axeX.r.min), 0 },
	{ "max",     DESC_FLOAT &(MonitFenLue.axeX.r.max), 0 },
	DESC_END
}; */
#ifdef NTUPLES_ONLINE
	static ArgDesc MonitTraceXntpDesc[4], MonitTraceYntpDesc[4];
#else
	static ArgDesc MonitTraceXntpDesc[] = { DESC_END };
	static ArgDesc MonitTraceYntpDesc[] = { DESC_END };
#endif
static ArgDesc MonitFenTracesBiplotDesc[] = {
	{ "nb",      DESC_INT    &MonitFenLue.p.b.nb, 0 },
	{ "X",       DESC_STRUCT &MonitTraceXntpDesc, 0 },
	{ "Y",       DESC_STRUCT &MonitTraceYntpDesc, 0 },
	{ "classe",  DESC_STR(CATG_NOM) &(MonitFenLue.nom_catg), 0 },
	DESC_END
};
// "signal/histo/FFT/fonction/2D-histo/biplot/evenement/evtmoyen/pattern"
static ArgDesc *MonitFenTracesDesc[] = {
	MonitFenTracesSignalDesc, MonitFenTracesHistoDesc,  MonitFenTracesSignalDesc,
	MonitFenTracesHistoDesc,  MonitFenTracesH2dDesc,    MonitFenTracesBiplotDesc,
	MonitFenTracesSignalDesc, MonitFenTracesResulDesc,  MonitFenTracesResulDesc,
	0
};

/* .......................... Structure du fichier .......................... */

// #define MONIT_FEN_TYPES "signal/histo/FFT/fonction/biplot/histo2d/evenement/evtmoyen/pattern"
//                            0      1    2     3       4       5       6         7        8
static ArgDesc MonitFenLueDesc[] = {
	{ 0, DESC_STR(MONIT_FENTITRE)                 MonitFenLue.titre,    0 },
	{ 0, DESC_KEY MONIT_FEN_TYPES,              &(MonitFenLue.type),    0 },
	{ 0, DESC_VARIANTE(MonitFenLue.type)          MonitFenComplDesc,    0 },
	{ 0, DESC_VARIANTE(MonitFenLue.type)          MonitFenTracesDesc,   0 },
	{ "dimension", DESC_STRUCT                   &MonitFenDimDesc,      0 },
	{ "position",  DESC_STRUCT                   &MonitFenPosDesc,      0 },
	{ "affichage", DESC_KEY MONIT_DISPLAY_CLES, &(MonitFenLue.affiche), 0 },
	{ "grille",    DESC_BOOL                    &(MonitFenLue.grille),  0 },
	DESC_END
};
static ArgStruct MonitFenAS  = { (void *)MonitFen, (void *)&MonitFenLue, sizeof(TypeMonitFenetre), MonitFenLueDesc };
static ArgDesc MonitFenDesc[] = {
	{ "Fenetres", DESC_STRUCT_SIZE(MonitFenNb,MAXMONIT) &MonitFenAS, 0 },
	DESC_END
};

/* .......................... Etiquettes .................................... */

static char MonitTagsKeys[128];
static TypeTagDisplay TagDefLue;
static ArgDesc TagDefDesc[] = {
	{ 0,         DESC_STR(CATG_NOM)          TagDefLue.nom, 0 },
	{ "symbole", DESC_KEY MonitTagsKeys,    &TagDefLue.p,   0 },
	{ "r",       DESC_SHEX                  &TagDefLue.c.r, 0 },
	{ "g",       DESC_SHEX                  &TagDefLue.c.g, 0 },
	{ "b",       DESC_SHEX                  &TagDefLue.c.b, 0 },
	{ "son",     DESC_STR(MAXFILENAME)       TagDefLue.son, 0 },
	DESC_END
};
static ArgStruct MonitTagsAS = { (void *)TagDef, (void *)&TagDefLue, sizeof(TypeTagDisplay), TagDefDesc };
static ArgDesc MonitTagsDesc[] = {
	{ "Etiquettes",    DESC_STRUCT_SIZE(TagDefNb,CATG_MAX) &MonitTagsAS, 0 },
	DESC_END
};

/* ............... Affichage par categorie d'evenements ..................... */

static TypeMonitCatg MonitCatgLue;
static ArgDesc MonitCatgTousDesc[] = {
	{ "taux.valeur", DESC_BOOL        &(TousLesEvts.taux),       0 },
	{ "taux.courbe", DESC_BOOL        &(TousLesEvts.evol),       0 },
	{ "trace_evt",   DESC_BOOL        &(TousLesEvts.trace),      0 },
	{ "deconvolue",  DESC_BOOL        &(TousLesEvts.deconv),     0 },
	{ "autoscale",   DESC_BOOL        &(TousLesEvts.autoscale),  0 },
	{ "scale.min",   DESC_INT         &(TousLesEvts.min),        0 },
	{ "scale.max",   DESC_INT         &(TousLesEvts.max),        0 },
	DESC_END
};
static ArgDesc MonitCatgSuppDesc[] = {
	{ 0,             DESC_STR(CATG_NOM) MonitCatgLue.nom,        0 },
	{ "utilisee",    DESC_BOOL        &(MonitCatgLue.utilisee),  0 },
	{ "taux.valeur", DESC_BOOL        &(MonitCatgLue.taux),      0 },
	{ "taux.courbe", DESC_BOOL        &(MonitCatgLue.evol),      0 },
	{ "trace_evt",   DESC_BOOL        &(MonitCatgLue.trace),     0 },
	{ "deconvolue",  DESC_BOOL        &(MonitCatgLue.deconv),    0 },
	{ "autoscale",   DESC_BOOL        &(MonitCatgLue.autoscale), 0 },
	{ "scale.min",   DESC_INT         &(MonitCatgLue.min),       0 },
	{ "scale.max",   DESC_INT         &(MonitCatgLue.max),       0 },
	DESC_END
};
static ArgDesc MonitCatgGrafGenDesc[] = {
	{ "largeur", DESC_INT &MonitEvtCatgLarg, 0 },
	{ "hauteur", DESC_INT &MonitEvtCatgHaut, 0 },
	DESC_END
};
static ArgDesc MonitCatgGrafFocDesc[] = {
	{ "largeur", DESC_INT &MonitEvtFocsLarg, 0 },
	{ "hauteur", DESC_INT &MonitEvtFocsHaut, 0 },
	DESC_END
};
static ArgDesc MonitCatgGrafDesc[] = {
	{ "par_classe", DESC_STRUCT MonitCatgGrafGenDesc, 0 },
	{ "focus",      DESC_STRUCT MonitCatgGrafFocDesc, 0 },
	DESC_END
};
static ArgStruct MonitCatgAS = { (void *)MonitCatg, (void *)&MonitCatgLue, sizeof(TypeMonitCatg), MonitCatgSuppDesc };
static ArgDesc MonitCatgDesc[] = {
	{ "graphiques",    DESC_STRUCT MonitCatgGrafDesc, 0 },
	{ "tous-les-evts", DESC_STRUCT MonitCatgTousDesc, 0 },
	{ "particulieres", DESC_STRUCT_SIZE(MonitCatgNb,CATG_MAX) &MonitCatgAS, 0 },
	DESC_END
};

/* ========================================================================== */

static char  MonitNouvelle[80];
static int   MonitVoieVal0,MonitVoieValN;
static Panel pMonitVoiePrint;

static Panel pMonitFen,pMonitFenZoomData,pMonitFenZoomEvts,pMonitFenZoomHisto,pMonitFenZoomFft;
static Panel pMonitFenZoomFctn,pMonitFenZoomBiplot,pMonitFenZoomH2D,pMonitFenZoomMoyen;
static Panel pMonitMode,pMonitTraces,pMonitFenAffiche,pMonitFenDetecteur,pMonitFenChange;
static int   MonitBoloAvant;
// static TypeMonitAxe MonitX,MonitY;
// static int   MonitMin,MonitMax;
// static float MonitRMin,MonitRMax;
// static float MonitFMin;
static char  MonitEditTraces,MonitFenModifiees,MonitCatgModifie,MonitTagModifie;
static Panel pMonitFenUser;

static Graph gFiltree[VOIES_MAX];
#ifdef MONIT_PAR_VOIE
	static float FiltreeAb6[2];
#endif

static int MonitFenEncode();
static Graph MonitFenBuild(TypeMonitFenetre *f);

/* ========================================================================== */
static int MonitGeneral() {
	if(OpiumExec(pMonitGeneral->cdr) == PNL_CANCEL) return(0);
	MonitFenLue.larg = MonitLarg;
	MonitFenLue.haut = MonitHaut;
	MonitASauver = 1;

	switch(MonitChgtUnite) {
	  case MONIT_ADU:
		ValeurADU = 1.0;
		break;
	  case MONIT_ENTREE_ADC:
		ValeurADU = MonitADUenmV;
		break;
	  case MONIT_KEV:
		ValeurADU = MonitADUenkeV;
		break;
	  case MONIT_ENTREE_BB:
//		ValeurADU = MonitADUenmV / GainBB[voie][bolo];
//		break;
	  case MONIT_SORTIE_BOLO:
//		ValeurADU = MonitADUenmV * 1000000.0 / (GainBB[voie][bolo] * Bolo[bolo].gainFET[voie]);
//		break;
	  default:
		OpiumError("Unite non programmee. On revient aux ADUs");
		MonitChgtUnite = MONIT_ADU;
		ValeurADU = 1.0;
		break;
	}
	MonitSauve();

	return(0);
}
/* ========================================================================== */
static void MonitCouleurVersPcent(OpiumColor *c, TypeMonitRgb *rgb) {
	rgb->r = (float)(c->r) * WND_COLORFLOAT;
	rgb->g = (float)(c->g) * WND_COLORFLOAT;
	rgb->b = (float)(c->b) * WND_COLORFLOAT;
}
/* ========================================================================== */
static void MonitPcentVersCouleur(TypeMonitRgb *rgb, OpiumColor *c) {
	c->r = rgb->r / WND_COLORFLOAT;
	c->g = rgb->g / WND_COLORFLOAT;
	c->b = rgb->b / WND_COLORFLOAT;
}
/* ========================================================================== */
static void MonitCouleurDefaut(OpiumColor *c) {
	c->r = WndLevelBgnd[WND_Q_ECRAN];   // 0x0000
	c->g = WndLevelText[WND_Q_ECRAN];   // 0xFFFF
	c->b = WndLevelBgnd[WND_Q_ECRAN];   // 0x0000
}
/* ========================================================================== */
static void MonitVarName(char *nomvoie, int ntp_type, char *nomvar) {
	strcpy(nomvar,L_(NtpName[ntp_type]));
	if(nomvoie) { if(*nomvoie != '\0') strcat(strcat(nomvar,"-"),nomvoie); }
}
/* ========================================================================== */
static void MonitVarTitre(int voie, int ntp_type, char *nomvar) {
	char *nomvoie;

	if(VoiesNb > 1) { nomvoie = (BolosUtilises > 1)? VoieManip[voie].nom: VoieManip[voie].prenom; }
	else nomvoie = 0;
	MonitVarName(nomvoie,ntp_type,nomvar);
	if(VarUnit[ntp_type][0] != '\0') strcat(strcat(strcat(nomvar," ("),&(VarUnit[ntp_type][0])),")");
}
/* ========================================================================== */
static int MonitSuiviAffiche(Menu menu, MenuItem item) {
	MonitEvtClasses = 1; LectPlancheSuivi(1); return(0);
}
/* ========================================================================== */
static int MonitSuiviIncludes() {
	TypeMonitFenetre *f; int i; char affiche;

	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) {
		if(((f->affiche == MONIT_FOLW) && !(f->incluse)) || ((f->affiche != MONIT_FOLW) && f->incluse)) break;
	}
	if(i < MonitFenNb) {
		affiche = OpiumDisplayed(bSuiviEvts);
		LectPlancheSuivi(affiche);
	}

	return(1);
}
/* ========================================================================== */
float MonitUnitesADU(Graph g, int sens, float val) {
	return(sens? val / ValeurADU: val * ValeurADU);
}
/* ========================================================================== */
void MonitFenDump(TypeMonitFenetre *f) {
	TypeMonitTrace *t; TypeMonitRgb *nuance; int j;

	printf("Fenetre %-32s, %s ",f->titre,MonitFenTypeName[f->type]);
	if((f->type == MONIT_HISTO) || (f->type == MONIT_2DHISTO) || (f->type == MONIT_BIPLOT))
		printf("[%-5g .. %5g] ",f->axeX.r.min,f->axeX.r.max);
	else if((f->type == MONIT_FFT) || (f->type == MONIT_FONCTION))
		printf("[%-5g .. %5g] ",f->axeY.r.min,f->axeY.r.max);
	else printf("[%-5d .. %5d] ",f->axeY.i.min,f->axeY.i.max);
	if((f->type == MONIT_2DHISTO) || (f->type == MONIT_BIPLOT))
		printf("x [%-5g .. %5g] ",f->axeY.r.min,f->axeY.r.max);
	switch(f->type) {
	  case MONIT_SIGNAL:
		printf(L_("(%g ms de signal)"),f->axeX.r.max);
		break;
	  case MONIT_EVENT:
		break;
	  case MONIT_HISTO:
		printf(L_("(sur %d bins)"),f->axeX.pts);
		break;
	  case MONIT_FFT:
		printf(L_("(sur %d x %d echantillons %s evts)"),
			   f->p.f.intervalles,f->axeX.pts,f->p.f.excl? "avec": "hors");
		break;
	  case MONIT_PATTERN:
	  case MONIT_MOYEN:
		printf("(%s%s)",MonitFenTypeName[f->type],f->nb?"s":"");
		break;
	  case MONIT_FONCTION:
		break;
	  case MONIT_2DHISTO:
	  case MONIT_BIPLOT:
		printf(L_("(sur %d x %d bins)"),f->axeX.pts,f->axeY.pts);
		break;
	}
	printf(": %d voie%c\n",f->nb,(f->nb > 1)? 's': ' ');
	t = f->trace;
	for(j=0; j<f->nb; j++,t++) {
		switch(f->type) {
		  case MONIT_PATTERN:
		  case MONIT_MOYEN:
			printf("|   %-16s",VoieManip[(int)t->voie].nom);
			break;
		  case MONIT_SIGNAL:
		  case MONIT_EVENT:
		  case MONIT_FFT:
			printf("|   %-16s %-8s",VoieManip[t->voie].nom,TrmtClassesListe[t->var]);
			break;
		  case MONIT_HISTO:
		  case MONIT_FONCTION:
		  case MONIT_2DHISTO:
			printf("|   %-12s %-16s",VarName[t->var],VoieManip[t->voie].nom);
			break;
		  case MONIT_BIPLOT:
			printf("|   %-12s",PlotVarList[t->var]);
			break;
		}
		if((f->type != MONIT_2DHISTO) && ((f->type != MONIT_BIPLOT) || j)) {
			nuance = &(t->rgb);
			printf(" (%6.2f%% rouge, %6.2f%% verte, %6.2f%% bleue)\n",nuance->r,nuance->g,nuance->b);
		} else printf("\n");
	}
	printf("|_______________________________________ %d x %d en (%d, %d)\n",f->larg,f->haut,f->posx,f->posy);
}
/* ========================================================================== */
static int MonitFenListeCapteurs(Panel panel, int num, void *arg) {
	int bolo,cap; int i;
	
	i = (int)(IntAdrs)arg;
	bolo = MonitFenEnCours->trace[i].bolo;
	for(cap=0; cap<Bolo[bolo].nbcapt; cap++) MonitFenEnCours->trace[i].liste_voies[cap] = Bolo[bolo].captr[cap].nom;
	MonitFenEnCours->trace[i].liste_voies[cap] = "\0";
	if(MonitFenEnCours->trace[i].cap >= Bolo[bolo].nbcapt) MonitFenEnCours->trace[i].cap = 0;
//	printf("(%s) Liste des voies du detecteur '%s':\n",__func__,Bolo[bolo].nom);
//	for(cap=0; cap<Bolo[bolo].nbcapt; cap++) printf("'%s'\n",MonitFenEnCours->trace[i].liste_voies[cap]);
	
	return(0);
}
/* ========================================================================== */
static void MonitFenDefault(TypeMonitFenetre *f, char *nom) {
	TypeMonitTrace *t;

	strncpy(f->titre,nom,80);
	f->type = MONIT_SIGNAL;
	f->affiche = MONIT_SHOW; f->grille = 0;
	f->change = 0;
	f->catg = 0;
	f->posx = f->posy = OPIUM_AUTOPOS;
	f->larg = MonitLarg; f->haut = MonitHaut;
//	f->axeX.r.min = 0; f->axeX.r.max = MonitDuree; f->axeX.pts = 100;
//	f->axeY.i.min = -8192; f->axeY.i.max = 8191; f->axeY.i.max = MonitEvts;
	f->axeX.i.min = 0; f->axeX.i.max = 0; f->axeX.pts = 100;
	f->axeY.i.min = 0; f->axeY.i.max = 0; f->axeY.pts = 100;
	f->g = 0;
	f->p.h.histo = 0;
	f->nb = 0;
	t = &(f->trace[0]);
	t->bolo = 0; t->cap = 0; t->voie = 0; /* t->vm = 0; */ t->var = TRMT_AU_VOL;
	MonitFenEnCours = f; MonitFenListeCapteurs(0,0,(void *)0);
	MonitCouleurDefaut(&(t->couleur));
	MonitCouleurVersPcent(&(t->couleur),&(t->rgb));
}
/* ========================================================================== */
static inline void MonitFenDimMin(TypeMonitFenetre *f) {
	if(f->larg < 40) f->larg = 40; if(f->haut < 40) f->haut = 40;
}
/* ========================================================================== */
static char MonitFenDecodeLigne(int l, char *r, char *entree_traces, int *nb_averts, int *nb_erreurs,
	TypeMonitFenetre **fcur) {
	int i,j; char *nom,*parametre;
	char *ptrbolo,*ptrvoie,*ptrvar,*ptr; char titre[80];
	int bolo,voie,cap; int var;
	char nombolo[DETEC_NOM],nomvoie[MODL_NOM],nomvar[16];
	TypeMonitFenetre *f; TypeMonitTrace *t; OpiumColor *c;

	if((*r == '#') || (*r == '\n')) return(1);
	f = *fcur;
	if(*r == '+') {
		i = MonitFenNb;
		if(MonitFenNb < MAXMONIT) {
			if(i) {
				if(f->affiche != MONIT_HIDE) MonitFenAffiche(&(MonitFen[i - 1]),0);
				f++;
				*fcur = f;
			}
			MonitFenNb++;
			strcpy(MonitFen[MonitFenNb].titre,MonitNouvelle);
			MonitFen[MonitFenNb + 1].titre[0] = '\0';
		} else 	{
			printf("  ! ligne %d: Pas plus de %d fenetres. Dernieres fenetres non relues\n",l,MAXMONIT);
			*nb_erreurs += 1;
			return(0);
		}
		r++;
		nom = ItemSuivant(&r,'/');
		if(*nom != '\0') {
			if(!strcmp(nom,MonitNouvelle)) {
				printf("  ! ligne %d: Titre interdit\n",l);
				*nb_averts += 1;
				*nom = '\0';
			} else {
				for(j=0; j<i; j++) if(!strcmp(nom,MonitFen[j].titre)) break;
				if(j < i) {
					printf("  ! ligne %d: Titre '%s' deja utilise par la fenetre #%d\n",l,nom,j+1);
					*nb_averts += 1;
					*nom = '\0';
				}
			}
		}
		if(*nom == '\0') {
			sprintf(titre,"Fenetre #%d",i+1);
			MonitFenDefault(f,titre);
		} else MonitFenDefault(f,nom);
		f->nb = 0;
		*entree_traces = (*r == '\0');
	}
	if(!*entree_traces) {
		while(*r != '\0') {
			parametre = ItemSuivant(&r,'/');
			if(*parametre == '\0') continue;
			nom = ItemSuivant(&parametre,'=');
			if(!strcmp(nom,"donnees")) {
				for(f->type=0; f->type<MONIT_NBTYPES; f->type += 1) {
					if(!strcmp(parametre,MonitFenTypeName[f->type])) break;
				}
				if(f->type == MONIT_NBTYPES) f->type = MONIT_SIGNAL;
				if(f->type == MONIT_HISTO) {
					f->p.h.histo = 0;
					f->axeX.r.min = 0.0; f->axeX.r.max = 0.0;
				} else if(f->type == MONIT_FFT) {
					f->axeX.r.min = 0.0;  f->axeX.r.max = 100.0;
					f->axeY.r.min = 0.01; f->axeY.r.max = 10000.0;
					f->p.f.xlog = 0;
					f->p.f.excl = 0;
					f->p.f.intervalles = 1;
				}
			}
			else if(!strcmp(nom,"duree"))
				sscanf(parametre,"%g",&(f->axeX.r.max));
			else if(!strcmp(nom,"freq-min"))
				sscanf(parametre,"%g",&(f->axeX.r.min));
			else if(!strcmp(nom,"freq-max"))
				sscanf(parametre,"%g",&(f->axeX.r.max));
			else if(!strcmp(nom,"bins"))
				sscanf(parametre,"%d",&(f->axeX.pts));
			else if(!strcmp(nom,"echantillons"))
				sscanf(parametre,"%d",&(f->axeX.pts));
			else if(!strcmp(nom,"evts")) {
				if(f->type == MONIT_HISTO) sscanf(parametre,"%d",&(f->axeY.i.max));
				else if(f->type == MONIT_FFT) f->p.f.excl = strcmp(parametre,"avec")? 0: 1;
			} else if(!strcmp(nom,"axeX")) {
				if(!strcmp(parametre,"log")) f->p.f.xlog = 1;
				else f->p.f.xlog = 0;
			} else if(!strcmp(nom,"interv")) {
				sscanf(parametre,"%d",&(f->p.f.intervalles));
				if(f->p.f.intervalles <= 0) f->p.f.intervalles = 1;
			} else if(!strcmp(nom,"limites")) {
				if(f->type == MONIT_HISTO)
					sscanf(parametre,"%g %g",&(f->axeX.r.min),&(f->axeX.r.max));
				// printf("- Limites[%s] = [%g, %g]\n",f->titre,f->axeX.r.min,f->axeX.r.max);
				else if(f->type == MONIT_FFT)
					sscanf(parametre,"%g %g",&(f->axeY.r.min),&(f->axeY.r.max));
				// printf("- Limites[%s] = [%g, %g]\n",f->titre,f->axeY.r.min,f->axeY.r.max);
				else sscanf(parametre,"%d %d",&(f->axeY.i.min),&(f->axeY.i.max));
			} else if(!strcmp(nom,"dimensions")) {
				sscanf(parametre,"%d %d",&(f->larg),&(f->haut));
				MonitFenDimMin(f);
				// printf("- Dimensions[%s] = (%d, %d)\n",f->titre,f->larg,f->haut);
			} else if(!strcmp(nom,"position"))
				sscanf(parametre,"%d %d",&(f->posx),&(f->posy));
			else if(!strcmp(nom,"affichage")) {
				if(*parametre == '\0') f->affiche = MONIT_SHOW;
				else f->affiche = ((*parametre == 'n') || (*parametre == '0'))? MONIT_HIDE: ((*parametre == 't')? MONIT_ALWS: MONIT_SHOW);
			} else if(!strcmp(nom,"grille")) {
				if(*parametre == '\0') f->grille = 1;
				else f->grille = ((*parametre == 'n') || (*parametre == '0'))? 0: 1;
			} else if(!strncmp(nom,"trace",5)) {
				*entree_traces = 1;
				r = parametre;
				break;
			}
		}
	}
	if(*entree_traces && (*r != '\0')) {
		if(f->nb >= MAXTRACES) {
			printf("  ! ligne %d: Pas plus de %d traces par fenetre\n",l,MAXTRACES);
			*nb_erreurs += 1;
			return(1);
		}
		t = f->trace + f->nb;
		t->ajuste = 0;
		c = &(t->couleur);
		nom = ItemSuivant(&r,'(');
		if(*r) {
			/* nouveau format: couleurs entre (), donc noms reperables */
			parametre = ItemSuivant(&r,')');
			sscanf(parametre,"%hX %hX %hX",&(c->r),&(c->g),&(c->b));
			MonitCouleurVersPcent(c,&(t->rgb));
			r = nom;
			if((f->type == MONIT_HISTO) || (f->type == MONIT_FFT)) {
				ptrvar = ItemSuivant(&r,' ');
				ptrvoie = ItemSuivant(&r,' ');
				ptrbolo = ItemSuivant(&r,' ');
				// printf("(%s.%d) Lu: '%s' '%s' '%s'\n",__func__,__LINE__,ptrvar,ptrvoie,ptrbolo);
				if(ptrbolo[0] == '\0') { if(BoloNb > 1) { ptr = ptrbolo; ptrbolo = ptrvoie; ptrvoie = ptr; } } /* cas de la voie ou du bolo unique */
				for(var=0; var<MONIT_NBVARS; var++) if(!strcmp(ptrvar,VarName[var])) break;
				if(var == MONIT_NBVARS) {
					printf("  ! ligne %d: Variable '%s' inconnue pour un type %s. Variables connues:",l,ptrvar,MonitFenTypeName[f->type]);
					for(var=0; var<MONIT_NBVARS; var++) {
						if(!(var % 10)) printf("\n  ! "); printf("%-10s",VarName[var]); if((var + 1) < MONIT_NBVARS) printf(",");
					}
					printf("\n");
					var = 0;
					*nb_erreurs += 1;
					return(1);
				}
				t->var = var;
			} else {
				ptrvoie = ItemSuivant(&r,' ');
				ptrbolo = ItemSuivant(&r,' ');
				ptrvar = ItemSuivant(&r,' ');
				// printf("(%s.%d) Lu: '%s' '%s' '%s'\n",__func__,__LINE__,ptrvoie,ptrbolo,ptrvar);
				if(ptrvar[0] == '\0') { ptr = ptrvar; ptrvar = ptrbolo; if(BoloNb > 1) { ptrbolo = ptrvoie; ptrvoie = ptr; } } /* cas de la voie ou du bolo unique */
				if(ptrvar[0] == '\0') t->var = TRMT_AU_VOL;
				else {
					for(var=0; var<TRMT_NBCLASSES; var++) if(!strcmp(ptrvar,TrmtClassesListe[var])) break;
					if(var == TRMT_NBCLASSES) {
						printf("  ! ligne %d: Type de signal '%s' inconnu pour un type %s. Types connus:",l,ptrvar,MonitFenTypeName[f->type]);
						for(var=0; var<TRMT_NBCLASSES; var++) {
							if(!(var % 10)) printf("\n  ! "); printf("%-10s",TrmtClassesListe[var]); if((var + 1) < TRMT_NBCLASSES) printf(",");
						}
						printf("\n");
						var = 0;
						*nb_erreurs += 1;
						return(1);
					}
					t->var = var;
				}
			}
			if(BoloNb == 1) bolo = 0; /* cas du detecteur unique */
			else {
				if(ptrbolo[0] == '\0') { ptr = ptrbolo; ptrbolo = ptrvoie; ptrvoie = ptr; } /* cas de la voie unique? */
				for(bolo=0; bolo<BoloNb; bolo++) if(!strcmp(ptrbolo,Bolo[bolo].nom)) break;
				if(bolo == BoloNb) {
					printf("  ! ligne %d: Detecteur '%s' inconnu pour un type %s\n",l,ptrbolo,MonitFenTypeName[f->type]);
					*nb_erreurs += 1;
					return(1);
					/* Finalement, c'est pas graaave
					 } else if(Bolo[bolo].lu == DETEC_HS) {
					 MonitBoloAvant = bolo; f->change = 1;
					 if(!*nb_erreurs) printf("* Fichier %s en erreur:\n",MonitFenFichier);
					 printf("  . ligne %d: Bolo '%s' hors service\n",l,ptrbolo);
					 *nb_erreurs += 1; */
					/* return(1); si on accepte, c'est possible de garder la fenetre */
				}
			}
			if((ptrvoie[0] == '\0') || (Bolo[bolo].nbcapt == 1)) cap = 0;
			else for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
				if(!strcmp(ptrvoie,Bolo[bolo].captr[cap].nom)) break;
			}
			if(cap == Bolo[bolo].nbcapt) {
				printf("  ! ligne %d: Pas de voie '%s' pour le detecteur '%s'\n",l,ptrvoie,Bolo[bolo].nom);
				*nb_erreurs += 1;
				voie = Bolo[bolo].captr[0].voie;
			} else {
				voie = Bolo[bolo].captr[cap].voie;
			}
		} else {
			/* ancien format: couleurs a la volee, donc noms pas reperables */
			if(f->type == MONIT_HISTO) {
				if(BoloNb > 1) sscanf(nom,"%s %s %s %hX %hX %hX\n",nomvar,nomvoie,nombolo,&(c->r),&(c->g),&(c->b));
				else sscanf(nom,"%s %s %hX %hX %hX\n",nomvar,nomvoie,&(c->r),&(c->g),&(c->b));
				for(var=0; var<MONIT_NBVARS; var++) if(!strcmp(nomvar,VarName[var])) break;
				if(var == MONIT_NBVARS) {
					printf("  ! ligne %d: Variable '%s' inconnue\n",l,nomvar);
					*nb_erreurs += 1;
					return(1);
				}
				t->var = var;
			} else if(f->type == MONIT_SIGNAL) {
				if(BoloNb > 1) sscanf(nom,"%s %s %s %hX %hX %hX\n",nomvoie,nombolo,nomvar,&(c->r),&(c->g),&(c->b));
				else sscanf(nom,"%s %s %hX %hX %hX\n",nomvoie,nomvar,&(c->r),&(c->g),&(c->b));
				for(var=0; var<TRMT_NBCLASSES; var++) if(!strcmp(nomvar,TrmtClassesListe[var])) break;
				if(var == TRMT_NBCLASSES) {
					printf("  ! ligne %d: Type de signal '%s' inconnu\n",l,nomvar);
					*nb_erreurs += 1;
					return(1);
				}
				t->var = var;
			} else {
				if(BoloNb > 1) sscanf(nom,"%s %s %hX %hX %hX\n",nomvoie,nombolo,&(c->r),&(c->g),&(c->b));
				else sscanf(nom,"%s %hX %hX %hX\n",nomvoie,&(c->r),&(c->g),&(c->b));
				t->var = TRMT_AU_VOL;
			}
			MonitCouleurVersPcent(c,&(t->rgb));
			if(BoloNb > 1) {
				for(bolo=0; bolo<BoloNb; bolo++) if(!strcmp(nombolo,Bolo[bolo].nom)) break;
				if(bolo == BoloNb) {
					printf("  ! ligne %d: Detecteur '%s' inconnu\n",l,nombolo);
					*nb_erreurs += 1;
					return(1);
					/* Finalement, c'est pas graaave
					 } else if(Bolo[bolo].lu == DETEC_HS) {
						MonitBoloAvant = bolo; f->change = 1;
						if(!*nb_erreurs) printf("* Fichier %s en erreur:\n",MonitFenFichier);
						printf("  . ligne %d: Bolo '%s' hors service\n",l,nombolo);
						*nb_erreurs += 1; */
					/* return(1); si on accepte, c'est possible de garder la fenetre */
				}
			} else bolo = 0;
			for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
				if(!strcmp(nomvoie,Bolo[bolo].captr[cap].nom)) break;
			}
			if(cap == Bolo[bolo].nbcapt) {
				printf("  ! ligne %d: Pas de voie '%s' pour le detecteur '%s'\n",l,nomvoie,nombolo);
				*nb_erreurs += 1;
				return(1);
			} else {
				voie = Bolo[bolo].captr[cap].voie;
			}
		}
		t->bolo = bolo; t->cap = cap; t->voie = voie;
		for(cap=0; cap<Bolo[bolo].nbcapt; cap++) t->liste_voies[cap] = Bolo[bolo].captr[cap].nom;
		t->liste_voies[cap] = "\0";
		if(f->type == MONIT_FFT) f->p.f.spectre[f->nb] = 0;
		f->nb += 1;
	}
	return(1);
}
/* ========================================================================== */
static int MonitFenDecode() {
	FILE *fichier;
	int i,j,l,bolo,cap,catg; int nb_averts,nb_erreurs; char entree_traces;
	char *r,ligne[256];
	TypeMonitFenetre *f;


	fichier = fopen(MonitFenFichier,"r");
	if(!fichier && StartFromScratch) {
		TypeMonitFenetre *f; TypeMonitTrace *t; int voie;
		printf("> Installe des graphiques  dans '%s'\n",MonitFenFichier);
		f = MonitFen;
		voie = 0;
		sprintf(f->titre,"Trace %s",VoieManip[voie].nom);
		f->type = MONIT_SIGNAL;
		f->axeX.r.max = 1000.0 / Echantillonnage;
		f->axeY.i.min = -8192; f->axeY.i.max = 8191;
		f->larg = 400; f->haut = 250; f->posx = 20; f->posy = 200; f->affiche = MONIT_SHOW; f->grille = 0;
		f->nb = 1;
		f->catg = 0;
		t = f->trace;
		t->voie = voie;
		MonitCouleurDefaut(&(t->couleur));
		MonitCouleurVersPcent(&(t->couleur),&(t->rgb));
		MonitFenNb++;
		MonitFenEncode();
		fichier = fopen(MonitFenFichier,"r");
	}
	if(!fichier) {
		printf("! Fichier des graphiques illisible: '%s' (%s)\n",MonitFenFichier,strerror(errno));
		return(0);
	}
	printf("= Lecture des graphiques                dans '%s'\n",MonitFenFichier);

	MonitEditTraces = 1;
	MonitFenNb = 0; MonitFenNum = 0;
	strcpy(MonitFen[MonitFenNb].titre,MonitNouvelle);
	MonitFen[MonitFenNb + 1].titre[0] = '\0';
	nb_erreurs = nb_averts = 0;

	i = 0; f = MonitFen; l = 0;
	entree_traces = 0;

	do {
		r = LigneSuivante(ligne,256,fichier);
		if(!r) break;
		ligne[255] = '\0';
		l++;
        if((l == 1) && !strncmp(r+2,DESC_SIGNE,strlen(DESC_SIGNE))) {
			// ArgDebug(1);
			ArgFromFile(fichier,MonitFenDesc,0);
			// ArgDebug(0);
			for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) {
				MonitFenEnCours = f;
				if(f->type == MONIT_HISTO) f->p.h.histo = 0;
				else if(f->type == MONIT_2DHISTO) { f->p.s.histo = 0; f->nb = 2; }
				else if(f->type == MONIT_BIPLOT) f->nb = 2;
				if(f->nom_catg[0] == '\0') catg = MonitCatgNb;
				else for(catg=0; catg<MonitCatgNb; catg++) if(!strcmp(f->nom_catg,MonitCatgNom[catg])) break;
				if(catg < MonitCatgNb) f->catg = catg; else { f->catg = 0; strcpy(f->nom_catg,MonitCatgNom[f->catg]); }
				for(j=0; j<f->nb; j++) {
					if(f->type != MONIT_BIPLOT) {
						if(f->type != MONIT_2DHISTO) MonitPcentVersCouleur(&(f->trace[j].rgb),&(f->trace[j].couleur));
						bolo = f->trace[j].bolo;
						for(cap=0; cap<Bolo[bolo].nbcapt; cap++) if(!strcmp(f->trace[j].voie_lue,Bolo[bolo].captr[cap].nom)) break;
						if(cap >= Bolo[bolo].nbcapt) {
							cap = 0;
							printf("! Le detecteur %s n'a pas de capteur %s\n",Bolo[bolo].nom,f->trace[j].voie_lue);
							nb_erreurs++;
						}
						f->trace[j].cap = cap;
						f->trace[j].voie = Bolo[bolo].captr[cap].voie;
						if(f->type == MONIT_FFT) f->p.f.spectre[j] = 0;
						MonitFenListeCapteurs(0,0,(void *)(long)j);
					}
				}
			}
			break;
        } else if(!MonitFenDecodeLigne(l,r,&entree_traces,&nb_averts,&nb_erreurs,&f)) break;
	} forever;
	MonitFenModifiees = 0;

	if(i && (f->affiche != MONIT_HIDE)) MonitFenAffiche(&(MonitFen[i - 1]),0);
	if(nb_averts) fprintf(stderr,"  = %d avertissement%s\n",Accord1s(nb_averts));
	if(nb_erreurs) fprintf(stderr,"  ! %d erreur%s, fichier a revoir\n",Accord1s(nb_erreurs));
	if(fichier) fclose(fichier);
	if(nb_averts || nb_erreurs) OpiumFail(L_("Graphiques utilisateur a revoir (cf journal)"));
	return(0);
}
#ifdef ENCODE_V1
/* ========================================================================== */
static int MonitFenEncodeV1() {
	int i,j; TypeMonitFenetre *f; TypeMonitTrace *t;

	RepertoireCreeRacine(MonitFenFichier);
	FILE *fichier; int l; OpiumColor *c; Cadre cdr;
	fichier = fopen(MonitFenFichier,"w");
	if(!fichier) {
		OpiumError(L_("Fichier '%s' inutilisable: %s"),MonitFenFichier,strerror(errno));
		return(0);
	}

	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) {
		MonitFenMemo(f);
		fprintf(fichier,"+ %s /donnees=%s ",f->titre,MonitFenTypeName[f->type]);
		switch(f->type) {
		  case MONIT_SIGNAL:
		  case MONIT_EVENT:
			fprintf(fichier,"/duree=%g",f->axeX.r.max);
			break;
		  case MONIT_HISTO:
			fprintf(fichier,"/bins=%d/evts=%d",f->axeX.pts,f->axeY.i.max);
			break;
		  case MONIT_FFT:
			fprintf(fichier,"/freq-min=%g/freq-max=%g",f->axeX.r.min,f->axeX.r.max);
			fprintf(fichier,"/echantillons=%d/axeX=%s/interv=%d/evts=%s",
				f->axeX.pts,f->p.f.xlog?"log":"lin",f->p.f.intervalles,f->p.f.excl?"avec":"sans");
			break;
		}
		if(f->type == MONIT_HISTO)
			fprintf(fichier,"/limites=%g %g",f->axeX.r.min,f->axeX.r.max);
		else if(f->type == MONIT_FFT)
			fprintf(fichier,"/limites=%g %g",f->axeY.r.min,f->axeY.r.max);
		else fprintf(fichier,"/limites=%d %d",f->axeY.i.min,f->axeY.i.max);
		fprintf(fichier,"\n");
		if(f->g) {
			cdr = (f->g)->cdr;
			if(OpiumDisplayed(cdr)) {
				OpiumGetPosition(cdr,&(f->posx),&(f->posy));
				OpiumGetSize    (cdr,&(f->larg),&(f->haut));
			}
		}
		MonitFenDimMin(f);
		l = strlen(f->titre) + 3;
		while(l--) fprintf(fichier," ");
		fprintf(fichier,"/dimensions=%d %d/position=%d %d/affichage=%s/grille=%s /traces=\n",
			f->larg,f->haut,f->posx,f->posy,f->affiche? ((f->affiche == MONIT_ALWS)? "tjrs": "oui"):"non",f->grille? "oui":"non");
		t = f->trace;
		for(j=0; j<f->nb; j++,t++) {
			c = &(t->couleur);
			switch(f->type) {
			  case MONIT_PATTERN:
			  case MONIT_MOYEN:
				fprintf(fichier,"       %-16s\t (%04X %04X %04X)\n",
					VoieManip[t->voie].nom,c->r,c->g,c->b);
				break;
			  case MONIT_SIGNAL:
			  case MONIT_EVENT:
			  case MONIT_FFT:
				fprintf(fichier,"       %-16s %-8s\t (%04X %04X %04X)\n",
					VoieManip[t->voie].nom,TrmtClassesListe[t->var],c->r,c->g,c->b);
				break;
			  case MONIT_HISTO:
				fprintf(fichier,"       %-12s %-16s\t (%04X %04X %04X)\n",
					VarName[t->var],VoieManip[t->voie].nom,c->r,c->g,c->b);
				break;
			}
		}
	}
	fclose(fichier);
	MonitFenModifiees = 0;

	return(0);
}
#endif /* ENCODE_V1 */
/* ========================================================================== */
static int MonitFenEncode() {
	int i,j; int bolo,cap; TypeMonitFenetre *f; TypeMonitTrace *t;

	RepertoireCreeRacine(MonitFenFichier);
	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) {
		if((f->type == MONIT_2DHISTO) || (f->type == MONIT_BIPLOT)) f->nb = 2;
		t = f->trace;
		for(j=0; j<f->nb; j++,t++) {
			bolo = t->bolo;
			if((cap = t->cap) >= Bolo[bolo].nbcapt) cap = 0;
			strcpy(t->voie_lue,Bolo[bolo].captr[cap].nom);
		}
	}
	if(ArgPrint(MonitFenFichier,MonitFenDesc) > 0) {
		MonitFenModifiees = 0;
		printf("%s/ Graphiques utilisateur sauves dans %s\n",DateHeure(),MonitFenFichier);
	} else OpiumError("Sauvegarde sur '%s' impossible",MonitFenFichier);

	return(0);
}
/* ========================================================================== */
static int MonitFenLit(Menu menu, MenuItem item) {
	if(OpiumExec(pMonitFenUser->cdr) == PNL_CANCEL) return(0);
	SambaAjoutePrefPath(MonitFenFichier,MonitUser);
	return(MonitFenDecode());
}
/* ========================================================================== */
static int MonitFenEcrit(Menu menu, MenuItem item) {
	if(SettingsSauveChoix) {
		if(OpiumExec(pMonitFenUser->cdr) == PNL_CANCEL) return(0);
	}
	SambaAjoutePrefPath(MonitFenFichier,MonitUser);
	return(MonitFenEncode());
}
/* ========================================================================== */
static int MonitFenSuivies(Panel panel, void *arg) {
	return(MonitSuiviIncludes());
}
/* ========================================================================== */
static int MonitFenPanel(Panel panel, int num, void *arg) {
	Panel p = (Panel)panel;
//	printf("* %s(%08X, %d, %08X)\n",__func__,(hexa)p,num,(hexa)arg);
	if(p) { PanelApply(p,1); if(OpiumDisplayed(p->cdr)) OpiumClear(p->cdr); }
//	printf("  %s: fenetre de type %d (%s)\n",__func__,MonitFenLue.type,MonitFenTypeName[MonitFenLue.type]);
	PanelErase(pMonitFen);
	switch(MonitFenLue.type) {
	  case MONIT_SIGNAL   : PanelTitle(pMonitFen,L_("Fenetre de signal")); break;
	  case MONIT_HISTO    : PanelTitle(pMonitFen,L_("Fenetre d'histogrammes")); break;
	  case MONIT_FFT      : PanelTitle(pMonitFen,L_("Fenetre en frequence")); break;
	  case MONIT_FONCTION : PanelTitle(pMonitFen,L_("Fonction n-tuple(evt)")); break;
	  case MONIT_2DHISTO  : PanelTitle(pMonitFen,L_("Histo 2D")); break;
	  case MONIT_BIPLOT   : PanelTitle(pMonitFen,L_("Bi-plot n-tuple")); break;
	  case MONIT_EVENT    : PanelTitle(pMonitFen,L_("Fenetre d'evenement")); break;
	  case MONIT_MOYEN    : PanelTitle(pMonitFen,L_("Evenement moyen")); break;
	  case MONIT_PATTERN  : PanelTitle(pMonitFen,L_("Motif soustrait")); break;
	}
	PanelText (pMonitFen,"Titre",MonitFenLue.titre,32);
	PanelItemExit(pMonitFen,PanelKeyB(pMonitFen,L_("Donnees presentees"),MonitNomTypes,(char *)&(MonitFenLue.type),11),MonitFenPanel,0);
	switch(MonitFenLue.type) {
	  case MONIT_SIGNAL:
	  case MONIT_EVENT:
		PanelFloat(pMonitFen,L_("Duree affichee (ms)"),&(MonitFenLue.axeX.r.max));
	  case MONIT_PATTERN:
	  case MONIT_MOYEN:
		PanelInt  (pMonitFen,L_("Valeur minimum"),&(MonitFenLue.axeY.i.min));
		PanelInt  (pMonitFen,L_("Valeur maximum"),&(MonitFenLue.axeY.i.max));
		break;
	  case MONIT_HISTO:
		PanelFloat(pMonitFen,"Bin minimum",&(MonitFenLue.axeX.r.min));
		PanelFloat(pMonitFen,"Bin maximum",&(MonitFenLue.axeX.r.max));
		PanelInt  (pMonitFen,L_("Nombre de bins"),&(MonitFenLue.axeX.pts));
		PanelInt  (pMonitFen,L_("Nb.evts affiches"),&(MonitFenLue.axeY.i.max));
		PanelList (pMonitFen,L_("Classe d'evenements"),MonitCatgNom,&(MonitFenLue.catg),CATG_MAX);
		break;
	  case MONIT_FFT:
		PanelInt  (pMonitFen,L_("Points echantillonnes"),&(MonitFenLue.axeX.pts));
		PanelFloat(pMonitFen,L_("Frequence min (0:auto)"),&(MonitFenLue.axeX.r.min));
		PanelFloat(pMonitFen,L_("Frequence max (0:auto)"),&(MonitFenLue.axeX.r.max));
		PanelFloat(pMonitFen,L_("Bruit minimum"),&(MonitFenLue.axeY.r.min));
		PanelFloat(pMonitFen,L_("Bruit maximum"),&(MonitFenLue.axeY.r.max));
		PanelKeyB (pMonitFen,L_("Axe X"),L_("lineaire/log"),&(MonitFenLue.p.f.xlog),10);
		PanelInt  (pMonitFen,L_("Intervalles sommes"),&(MonitFenLue.p.f.intervalles));
		PanelKeyB (pMonitFen,L_("Exclusion evts"),L_("non/active"),&(MonitFenLue.p.f.excl),8);
		break;
	  case MONIT_FONCTION:
		PanelFloat(pMonitFen,L_("Valeur minimum"),&(MonitFenLue.axeY.r.min));
		PanelFloat(pMonitFen,L_("Valeur maximum"),&(MonitFenLue.axeY.r.max));
		PanelInt  (pMonitFen,L_("Nb.evts affiches"),&(MonitFenLue.axeX.i.max));
		PanelList (pMonitFen,L_("Classe d'evenements"),MonitCatgNom,&(MonitFenLue.catg),CATG_MAX);
		break;
	  case MONIT_2DHISTO:
		PanelFloat(pMonitFen,"Bin X minimum",&(MonitFenLue.axeX.r.min));
		PanelFloat(pMonitFen,"Bin X maximum",&(MonitFenLue.axeX.r.max));
		PanelInt  (pMonitFen,L_("Nombre de bins en X"),&(MonitFenLue.axeX.pts));
		PanelFloat(pMonitFen,"Bin Y minimum",&(MonitFenLue.axeY.r.min));
		PanelFloat(pMonitFen,"Bin Y maximum",&(MonitFenLue.axeY.r.max));
		PanelInt  (pMonitFen,L_("Nombre de bins en Y"),&(MonitFenLue.axeY.pts));
		PanelList (pMonitFen,L_("Classe d'evenements"),MonitCatgNom,&(MonitFenLue.catg),CATG_MAX);
		break;
	  case MONIT_BIPLOT:
		PanelInt  (pMonitFen,"nb pts",&(MonitFenLue.p.b.nb));
		PanelFloat(pMonitFen,"X min",&(MonitFenLue.axeX.r.min));
		PanelFloat(pMonitFen,"X max",&(MonitFenLue.axeX.r.max));
		PanelFloat(pMonitFen,"Y min",&(MonitFenLue.axeY.r.min));
		PanelFloat(pMonitFen,"Y max",&(MonitFenLue.axeY.r.max));
		PanelList (pMonitFen,L_("Classe d'evenements"),MonitCatgNom,&(MonitFenLue.catg),CATG_MAX);
		break;
	}
	PanelKeyB (pMonitFen,L_("Affichage"),L_(MONIT_DISPLAY_CLES),&(MonitFenLue.affiche),6);
	PanelKeyB (pMonitFen,L_("Avec grille"),L_("non/oui"),&(MonitFenLue.grille),6);
	PanelBool (pMonitFen,L_("Editer les traces"),&MonitEditTraces);

	PanelOnApply(pMonitFen,MonitFenSuivies,0);
	PanelOnOK(pMonitFen,MonitFenSuivies,0);
	if(p) { OpiumUse(p->cdr,OPIUM_EXEC); OpiumRefresh(p->cdr); /* PanelRefreshVars(p); */ }
	return(0);
}
/* ========================================================================== */
static int MonitFenEdite(Menu menu, MenuItem item) {
	int i,j,l,m,n,x,y,cols,nbitems; int bolo;
	int posx,posy;
	char existe_deja,manque_infos,replace; int trmt_demande,var_demandee;
	char action[MAXTRACES],titre[256],*nom;
	TypeMonitFenetre *f; TypeMonitTrace *t; // OpiumColor *c;
	char *item_appelant;

	item_appelant = MenuItemGetText(menu,MenuItemNum(menu,item));
	if(!strncmp(item_appelant,"Creer",5)) MonitFenNum = MonitFenNb;
	else if(MonitFenNb > 1) {
		if(OpiumReadList(L_("Quelle fenetre"),MonitFenTitre,&MonitFenNum,32) == PNL_CANCEL) return(0);
	} else MonitFenNum = 0;

	f = &(MonitFen[MonitFenNum]);
	existe_deja = (MonitFenNum < MonitFenNb);
	if(!strncmp(item_appelant,"Organiser",9)) {
		MonitFenMemo(f);
		MonitLarg = f->larg; MonitHaut = f->haut;
		posx = f->posx + f->larg + WND_ASCINT_WIDZ + 6;
		posy = f->posy;
		for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if(i != MonitFenNum) {
			if((f->affiche == MONIT_HIDE) || (f->affiche == MONIT_FOLW))  continue;
			f->larg = MonitLarg; f->haut = MonitHaut;
			f->posx = posx; f->posy = posy;
			posx += (f->larg + WND_ASCINT_WIDZ + 6);
			if(posx > (OpiumServerWidth(0)  - f->larg)) {
				posx = MonitFen[MonitFenNum].posx;
				posy += (MonitHaut + WND_ASCINT_WIDZ + 30);
				if(posy > OpiumServerHeigth(0)) break;
			}
			/* Graph g; if((g = f->g)) {
				if(replace = OpiumDisplayed(g->cdr)) OpiumClear(g->cdr);
				GraphResize(g,f->larg,f->haut);
				OpiumSizeGraph(g->cdr,0);
				OpiumPosition(g->cdr,f->posx,f->posy);
				if(replace) OpiumDisplay(g->cdr);
			} */
			MonitFenAffiche(f,1);
		}
		return(0);
	} else if(!strncmp(item_appelant,"Dupliquer",9)) {
		nom = f->titre;
		MonitFenMemo(f);
		memcpy(&MonitFenLue,f,sizeof(TypeMonitFenetre));
		strcpy(MonitFenLue.titre,MonitNouvelle);
		MonitFenLue.posx = MonitFenLue.posy = OPIUM_AUTOPOS;
		MonitFenLue.g = 0;
		MonitFenLue.p.h.histo = 0;
		if(MonitFenLue.type == MONIT_HISTO) MonitFenLue.p.h.histo = 0;
		else if(MonitFenLue.type == MONIT_2DHISTO) { MonitFenLue.p.s.histo = 0; MonitFenLue.nb = 2; }
		else if(MonitFenLue.type == MONIT_FFT) {
			for(i=0; i<MonitFenLue.nb; i++) MonitFenLue.p.f.spectre[i] = 0;
		}
		if(MonitFenLue.nb) {
			MonitBoloAvant = MonitFenLue.trace[0].bolo;
			BoloNum = MonitBoloAvant;
			if(OpiumExec(pMonitFenDetecteur->cdr) != PNL_CANCEL) {
				for(i=0; i<MonitFenLue.nb; i++) {
					if(MonitFenLue.trace[i].bolo == MonitBoloAvant) MonitFenLue.trace[i].bolo = BoloNum;
				}
				l = (int)strlen(nom); n = (int)strlen(Bolo[MonitBoloAvant].nom);
				for(i=0; i<=(l - n); i++) if(!strncmp(nom+i,Bolo[MonitBoloAvant].nom,n)) break;
				if(i <= (l - n)) {
					strncpy(MonitFenLue.titre,nom,i);
					strncpy(MonitFenLue.titre+i,Bolo[BoloNum].nom,80-i);
					if((i + n) < l) {
						m = (int)strlen(Bolo[BoloNum].nom);
						strncpy(MonitFenLue.titre+i+m,nom+i+n,80-i-m);
					}
				}
			}
		}
		MonitFenNum = MonitFenNb;
		f = &(MonitFen[MonitFenNum]);
		existe_deja = 0;
	} else if(existe_deja) {
		// printf("(%s:%d) ",__func__,__LINE__); MonitFenDump(f);
		MonitFenMemo(f);
		MonitFenDetach(&(MonitFen[MonitFenNum])); /* avant memcpy de facon a ne pas garder non-nulles les adresses liberees ici */
		memcpy(&MonitFenLue,f,sizeof(TypeMonitFenetre));
	} else MonitFenDefault(&MonitFenLue,MonitNouvelle);
	MonitFenEnCours = &MonitFenLue;
	MonitFenPanel(0,0,0);
	PanelTitle(pMonitFen,MonitFenLue.titre);
	do {
		if(OpiumExec(pMonitFen->cdr) == PNL_CANCEL) return(0);
		if(!strcmp(MonitFenLue.titre,MonitNouvelle)) OpiumError(L_("Titre interdit"));
		else {
			for(i=0; i<MonitFenNb; i++) {
				if((i != MonitFenNum) && !strcmp(MonitFenLue.titre,MonitFen[i].titre)) break;
			}
			if(i == MonitFenNb) break;
			OpiumError(L_("Titre deja utilise par la fenetre #%d"),i+1);
		}
	} forever;
	if(!existe_deja) {
		if(MonitFenNb < MAXMONIT) {
			strcpy(MonitFen[MonitFenNb].titre,MonitNouvelle);
			MonitFenNb++;
			MonitFen[MonitFenNb].titre[0] = '\0';
		} else {
			OpiumError(L_("Il y a deja %d fenetre%s definie%s, maximum atteint"),Accord2s(MonitFenNb));
			return(0);
		}
	}
	strcpy(MonitFenLue.nom_catg,MonitCatgNom[MonitFenLue.catg]);
	if(MonitFenLue.type == MONIT_FFT) { if(MonitFenLue.p.f.intervalles <= 0) MonitFenLue.p.f.intervalles = 1; }

	if(MonitEditTraces) do {
//		float rouge[MAXTRACES],vert[MAXTRACES],bleu[MAXTRACES];
		if(MonitFenLue.nb == 0) {
			if((MonitFenLue.type == MONIT_2DHISTO) || (MonitFenLue.type == MONIT_BIPLOT)) {
				t = &(MonitFenLue.trace[0]); t->bolo = 0; t->cap = 0; t->voie = 0; t->var = 0; MonitFenListeCapteurs(0,0,(void *)0);
				t = &(MonitFenLue.trace[1]); t->bolo = 0; t->cap = 0; t->voie = 0; t->var = 1; MonitFenListeCapteurs(0,0,(void *)1);
				MonitFenLue.nb = 2;
			} else {
				t = &(MonitFenLue.trace[0]); t->bolo = 0; t->cap = 0; t->voie = 0; t->var = 0; MonitFenListeCapteurs(0,0,(void *)0);
				MonitCouleurDefaut(&(t->couleur)); MonitCouleurVersPcent(&(t->couleur),&(t->rgb));
				MonitFenLue.nb = 1;
			}
		}
//		trmt_demande = ((MonitFenLue.type == MONIT_SIGNAL) || (MonitFenLue.type == MONIT_EVENT) || (MonitFenLue.type == MONIT_FFT))? 1: 0;
//		var_demandee = ((MonitFenLue.type == MONIT_HISTO)  || (MonitFenLue.type == MONIT_2DHISTO)
//					 || (MonitFenLue.type == MONIT_BIPLOT) || (MonitFenLue.type == MONIT_FONCTION))? 1: 0;
		if(MonitFenLue.type == MONIT_BIPLOT) cols = 2;
		else {
			trmt_demande = MonitFenTypeAvecTrmt[MonitFenLue.type];
			var_demandee = MonitFenTypeAvecVar[MonitFenLue.type];
			cols = 6 + trmt_demande + var_demandee;
			if(MonitFenLue.type == MONIT_2DHISTO) cols -= 3; // pas de couleur demandee
		}
		nbitems = cols * (MonitFenLue.nb + 1);
		if(pMonitTraces && (PanelItemMax(pMonitTraces) != nbitems)) {
			OpiumGetPosition(pMonitTraces->cdr,&x,&y); replace = 1;
			PanelDelete(pMonitTraces);
			pMonitTraces = 0;
		} else replace = 0;
		if(pMonitTraces == 0) pMonitTraces = PanelCreate(nbitems);
		PanelErase(pMonitTraces);
		PanelColumns(pMonitTraces,cols);
		for(i=0; i<MonitFenLue.nb; i++) action[i] = EDITLIST_GARDE;
		if(MonitFenLue.type == MONIT_BIPLOT) {
			/* ... colonne 1 ... */
			PanelBlank(pMonitTraces);
			PanelRemark(pMonitTraces,"Axe X");
			PanelRemark(pMonitTraces,"Axe Y");
			/* ... colonne 2 ... */
			PanelRemark(pMonitTraces,"variable");
			for(i=0; i<MonitFenLue.nb; i++) PanelList(pMonitTraces,0,PlotVarList,&(MonitFenLue.trace[i].var),16);
		} else {
			/* ... colonne 1 ... */
			if(MonitFenLue.type == MONIT_2DHISTO) {
				PanelBlank(pMonitTraces);
				PanelRemark(pMonitTraces,"Axe X");
				PanelRemark(pMonitTraces,"Axe Y");
			} else {
				PanelRemark(pMonitTraces,"action");
				for(i=0; i<MonitFenLue.nb; i++)
					PanelItemColors(pMonitTraces,PanelKeyB(pMonitTraces,0,L_(EDITLIST_ACTION_CLES),&(action[i]),8),SambaRougeVertJauneOrange,3);
			}
			/* ... colonne 2 ... */
			if(var_demandee) {
				PanelRemark(pMonitTraces,"variable");
				for(i=0; i<MonitFenLue.nb; i++)
					PanelList(pMonitTraces,0,VarName,&(MonitFenLue.trace[i].var),12);
			}
			/* ... colonne 3 ... */
			PanelRemark(pMonitTraces,L_("detecteur"));
			for(i=0; i<MonitFenLue.nb; i++) {
				int k;
				k = PanelList(pMonitTraces,0,BoloNom,&(MonitFenLue.trace[i].bolo),8 /* DETEC_NOM */ );
				PanelItemExit(pMonitTraces,k,MonitFenListeCapteurs,(void *)(long)i);
				// PanelItemExit(pMonitTraces,k,DetecteurListeCapteurs,0);
			}
			/* ... colonne 4 ... */
			PanelRemark(pMonitTraces,L_("capteur"));
			for(i=0; i<MonitFenLue.nb; i++)
				//			PanelListB(pMonitTraces,0,ModeleVoieListe,&(MonitFenLue.trace[i].cap),8 /* MODL_NOM */ );
				PanelList(pMonitTraces,0,MonitFenLue.trace[i].liste_voies,&(MonitFenLue.trace[i].cap),8 /* MODL_NOM */ );
			/* ... colonne 5 (optionnelle) ... */
			if(trmt_demande) {
				PanelRemark(pMonitTraces,L_("type"));
				for(i=0; i<MonitFenLue.nb; i++)
					PanelList(pMonitTraces,0,TrmtClassesListe,&(MonitFenLue.trace[i].var),12);
			}
			/* ... colonnes de couleur (sauf 2DHISTO) ... */
			if(MonitFenLue.type != MONIT_2DHISTO) {
				for(i=0; i<MonitFenLue.nb; i++) MonitCouleurVersPcent(&(MonitFenLue.trace[i].couleur),&(MonitFenLue.trace[i].rgb));
				PanelRemark(pMonitTraces,L_("rouge"));
				for(i=0; i<MonitFenLue.nb; i++) {
					// PanelSHex(pMonitTraces,0,&(MonitFenLue.trace[i].couleur.r));
					PanelItemFormat(pMonitTraces,PanelFloat(pMonitTraces,0,&(MonitFenLue.trace[i].rgb.r)),"%4.0f");
				}
				PanelRemark(pMonitTraces,L_("vert"));
				for(i=0; i<MonitFenLue.nb; i++) {
					// PanelSHex(pMonitTraces,0,&(MonitFenLue.trace[i].couleur.g));
					PanelItemFormat(pMonitTraces,PanelFloat(pMonitTraces,0,&(MonitFenLue.trace[i].rgb.g)),"%4.0f");
				}
				PanelRemark(pMonitTraces,L_("bleu"));
				for(i=0; i<MonitFenLue.nb; i++) {
					// PanelSHex(pMonitTraces,0,&(MonitFenLue.trace[i].couleur.b));
					PanelItemFormat(pMonitTraces,PanelFloat(pMonitTraces,0,&(MonitFenLue.trace[i].rgb.b)),"%4.0f");
				}
			}
		}
		sprintf(titre,L_("Traces pour la fenetre '%s'"),MonitFenLue.titre);
		PanelTitle(pMonitTraces,titre);
		if(replace) OpiumPosition(pMonitTraces->cdr,x,y);

		if(OpiumExec(pMonitTraces->cdr) == PNL_CANCEL) {
			if(!existe_deja) MonitFen[--MonitFenNb].titre[0] = '\0';
			return(0);
		}
		manque_infos = 0;
		if((MonitFenLue.type == MONIT_2DHISTO) || (MonitFenLue.type == MONIT_BIPLOT)) for(i=0; i<MonitFenLue.nb; i++) {
			MonitPcentVersCouleur(&(MonitFenLue.trace[i].rgb),&(MonitFenLue.trace[i].couleur));
			switch(action[i]) {
			  case EDITLIST_INSERE:  /* inserer trace */
				if(MonitFenLue.nb >= MAXTRACES) {
					OpiumError(L_("Pas plus de %d traces par fenetre"),MAXTRACES);
					i = MonitFenLue.nb;
					break;
				}
				for(j=MonitFenLue.nb; j>i; --j) {
					memcpy(&(MonitFenLue.trace[j]),&(MonitFenLue.trace[j-1]),sizeof(TypeMonitTrace));
					if(j > (i + 1)) action[j] = action[j-1]; else action[j] = 0;
				}
				i++;
				MonitFenLue.nb += 1;
				manque_infos = 1;
				break;
			  case EDITLIST_EFFACE:  /* retirer trace */
				MonitFenLue.nb -= 1;
				for(j=i; j<MonitFenLue.nb; j++) {
					memcpy(&(MonitFenLue.trace[j]),&(MonitFenLue.trace[j+1]),sizeof(TypeMonitTrace));
					action[j] = action[j+1];
				}
				i -= 1;
				break;
			}
		}
		for(i=0; i<MonitFenLue.nb; i++) {
			bolo = MonitFenLue.trace[i].bolo;
			MonitFenLue.trace[i].voie = Bolo[bolo].captr[MonitFenLue.trace[i].cap].voie;
		}
	} while(manque_infos);

	memcpy(f,&MonitFenLue,sizeof(TypeMonitFenetre));
	MonitFenModifiees = 1;
	if((f->affiche == MONIT_SHOW) || (f->affiche == MONIT_ALWS)) MonitFenAffiche(f,1);
	else MonitFenBuild(f);

	return(0);
}
#ifdef UTILISE_SAMBAPLOT_AVEC_NANOPAW
/* ========================================================================== */
Graph MonitFenCreePlot(TypeMonitFenetre *f) {
	int p;

	switch(f->type) {
		case MONIT_SIGNAL: case MONIT_EVENT:
			p = PlotNouveau(PLOT_FONCTION);
			strncpy(Plot[p].titre,f->titre,80);
			Plot[p].g = f->g;
			break;
		case MONIT_HISTO:
			p = PlotNouveau(PLOT_HISTO);
			strncpy(Plot[p].titre,f->titre,80);
			Plot[p].g = f->g;
			Plot[p].h = f->p.h.histo;
			Plot[p].hd = f->p.h.hd[0];
			break;
		default: return(0);
	}
}
#endif
/* ========================================================================== */
Graph MonitFenCreeGraphe(TypeMonitFenetre *f, char nouveau) {
	int i,n,x,y,nbfreq,type,qual; float horloge; char nom[80];
	hexa zoom;
	OpiumColor *c; Graph g;
	// HistoData hd; 
	HistoDeVoie vh,prec;
	int j,kx; int voie; float duree,total; TypeMonitTrace *t;

	if(f->nb <= 0) return(0);
	if(MonitFenTypeAvecTrmt[f->type]) {
		n = 0;
		for(i=0; i<f->nb; i++) {
			voie = f->trace[i].voie; if(!VoieTampon[voie].lue) continue;
//-			switch(f->trace[i].var) {
//-			  case TRMT_AU_VOL: if(VoieTampon[voie].brutes->t.data)   n++; break;
//-			  case TRMT_PRETRG: if(VoieTampon[voie].traitees->t.data) n++; break;
//-			}
			if(VoieTampon[voie].trmt[f->trace[i].var].donnees.t.data) n++;
		}
		if(n <= 0) return(0);
	}

	f->dernier_evt = 0;
	if(nouveau || (f->g == 0)) { MonitFenDimMin(f); f->g = GraphCreate(f->larg,f->haut,2 * f->nb); }
	else GraphErase(f->g);
	g = f->g; OpiumTitle(g->cdr,f->titre);

	/* ... Allocations annexes ... */
	switch(f->type) {
	  case MONIT_HISTO:
		f->p.h.histo = HistoCreateFloat(f->axeX.r.min,f->axeX.r.max,f->axeX.pts);
		break;
	  case MONIT_2DHISTO:
		f->p.s.histo = H2DCreateFloatFloatToInt(f->axeX.r.min,f->axeX.r.max,f->axeX.pts,
			f->axeY.r.min,f->axeY.r.max,f->axeY.pts);
		H2DLUT(f->p.s.histo,MonitLUTall,OPIUM_LUT_DIM);
		H2DGraphAdd(f->p.s.histo,g);
		break;
	  case MONIT_FONCTION:
		x = GraphAdd(g,GRF_SHORT|GRF_INDEX|GRF_XAXIS,MonitEvtIndex,f->axeX.i.max);
		GraphDataName(g,x,"Evt#");
		break;
	  default: break;
	}

	/* ... Elements voie-dependants ... */
	for(j=0, t=f->trace; j<f->nb; j++,t++) {
		t->ajuste = 0;
		voie = t->voie; t->numx = -1;
		if(!VoieTampon[voie].lue) continue;
		c = &(t->couleur);
		x = -1; y = -1;
		switch(f->type) {
		  case MONIT_SIGNAL: /* abcisse en millisecondes */
			switch(t->var) {
			  case TRMT_AU_VOL:
				if(!VoieTampon[voie].brutes->t.data) break;
					if(VoieTampon[voie].brutes->type == TAMPON_INT16) type = GRF_SHORT; else type = GRF_INT;
				x = GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,VoieTampon[voie].trmt[TRMT_AU_VOL].ab6,VoieTampon[voie].brutes->max);
				y = GraphAdd(g,type|GRF_YAXIS,VoieTampon[voie].brutes->t.data,VoieTampon[voie].brutes->max);
				break;
			  case TRMT_PRETRG:
				if(!VoieTampon[voie].traitees->t.data) break;
				x = GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,VoieTampon[voie].trmt[TRMT_PRETRG].ab6,VoieTampon[voie].traitees->max);
				y = GraphAdd(g,GRF_FLOAT|GRF_YAXIS,VoieTampon[voie].traitees->t.data,VoieTampon[voie].traitees->max);
				break;
			}
			GraphDataName(g,x,"t(ms)");
			GraphDataName(g,y,VoieManip[voie].nom);
			GraphDataStyle(g,y,GRF_CODE_MINMAX);
			t->numx = x;
			if(y >= 0) GraphDataRGB(g,y,c->r,c->g,c->b);
			break;
		  case MONIT_FONCTION:
		  case MONIT_HISTO:
			f->p.h.hd[j] = HistoAdd(f->p.h.histo,HST_INT);
			if(f->type == MONIT_HISTO) {
				HistoDataName(f->p.h.hd[j],VoieManip[voie].nom);
				HistoDataSupportRGB(f->p.h.hd[j],WND_Q_ECRAN,c->r,c->g,c->b);
				HistoDataSupportRGB(f->p.h.hd[j],WND_Q_PAPIER,c->r,c->g,c->b);
			}
			vh = VoieHisto[voie]; prec = 0;
			while(vh) { prec = vh; vh = prec->suivant; }
			vh = (HistoDeVoie)malloc(sizeof(TypeHistoDeVoie));
			if(!vh) break;
			if(prec == 0) VoieHisto[voie] = vh;
			else prec->suivant = vh;
			vh->catg = (char)f->catg;
			vh->D = 1;
			vh->hd = f->p.h.hd[j];
			vh->Xvar = t->var;
			vh->suivant = 0;
			if(f->type == MONIT_FONCTION) {
//				y = GraphAdd(g,GRF_INT|GRF_YAXIS,(int *)((vh->hd)->adrs),f->axeX.i.max);
//				GraphDataName(g,y,VarName[t->var]);
			}
			break;
		  case MONIT_2DHISTO:
			vh = VoieHisto[voie]; prec = 0;
			while(vh) { prec = vh; vh = prec->suivant; }
			vh = (HistoDeVoie)malloc(sizeof(TypeHistoDeVoie));
			if(!vh) break;
			if(prec == 0) VoieHisto[voie] = vh;
			else prec->suivant = vh;
			vh->catg = (char)f->catg;
			vh->D = 2;
			vh->hd = (HistoData)f->p.s.histo;
			vh->Xvar = t->var;
			t++; j++;
			vh->Yvar = t->var; vh->Yvoie = t->voie;
			vh->suivant = 0;
			MonitVarTitre(voie,vh->Xvar,nom); GraphAxisTitle(g,GRF_XAXIS,nom); // inclue GraphDataName;
			MonitVarTitre(vh->Yvoie,vh->Yvar,nom); GraphAxisTitle(g,GRF_YAXIS,nom); // inclue GraphDataName;
			break;
		  case MONIT_BIPLOT:
			if(j == 0) {
				x = GraphAdd(g,GRF_FLOAT|GRF_XAXIS,*(MonitNtp[t->var].adrs),NtDim);
				GraphAxisTitle(g,GRF_XAXIS,PlotVarList[t->var]); // inclue GraphDataName(g,x,PlotVarList[t->var]);
				if(EvtEspace) GraphAdd(g,GRF_INDIV,EvtEspace->sel,NtDim);
				t->numx = x;
			} else /* j == 1 */ {
				y = GraphAdd(g,GRF_FLOAT|GRF_YAXIS,*(MonitNtp[t->var].adrs),NtDim);
				GraphAxisTitle(g,GRF_YAXIS,PlotVarList[t->var]); // inclue GraphDataName(g,y,PlotVarList[t->var]);
				for(qual=0; qual<WND_NBQUAL; qual++)
					GraphDataSupportTrace(g,y,qual,GRF_PNT,GRF_POINT,1);
			}
			break;
		  case MONIT_FFT:
			horloge = VoieTampon[voie].trmt[t->var].ab6[1];
			if(t->var == TRMT_AU_VOL) { if(!VoieTampon[voie].brutes->t.sval) break; }
			else if(t->var == TRMT_PRETRG) { if(!VoieTampon[voie].traitees->t.rval) break; }
			else break;
			nbfreq = (f->axeX.pts / 2) + 1;
			n = nbfreq * sizeof(float);
			f->p.f.spectre[j] = (float *)malloc(n);
			if(!f->p.f.spectre[j]) {
				OpiumError("Manque de place memoire pour le spectre (%d octets)",n);
				return(0);
			}
			f->p.f.freq[j][0] = 0.0; f->p.f.freq[j][1] = 1.0 / (horloge * f->axeX.pts); /* kHz */
			x = GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,f->p.f.freq[j],nbfreq);
			y = GraphAdd(g,GRF_FLOAT|GRF_YAXIS,f->p.f.spectre[j],nbfreq);
//			printf("Ajoute le tableau %08X[%d] a la trace F%d[%d]\n",(hexa)(f->p.f.spectre[j]),nbfreq,num,j);
			GraphDataName(g,x,"f(kHz)");
			GraphDataName(g,y,VoieManip[voie].nom);
			if(y >= 0) GraphDataRGB(g,y,c->r,c->g,c->b);
			GraphAxisTitle(g,GRF_YAXIS,L_("Bruit"));
			GraphAxisTitle(g,GRF_XAXIS,L_("Frequence (kHz)"));
			t->numx = x;
			break;
		  case MONIT_EVENT:
			switch(t->var) {
			  case TRMT_AU_VOL:
				if(!VoieTampon[voie].brutes->t.sval) break;
				x = GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitEvtAb6[voie][0]),VoieTampon[voie].brutes->max);
				// x = GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,VoieTampon[voie].trmt[TRMT_AU_VOL].ab6,PointsEvt);
				y = GraphAdd(g,GRF_SHORT|GRF_YAXIS,VoieTampon[voie].brutes->t.sval,PointsEvt);
				GraphDataName(g,x,"t(ms)");
				GraphDataName(g,y,VoieManip[voie].nom);
				break;
			  case TRMT_PRETRG:
				if(!VoieTampon[voie].traitees->t.rval) break;
				x = GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitTrmtAb6[voie][0]),VoieTampon[voie].traitees->max);
				// x = GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,VoieTampon[voie].trmt[TRMT_PRETRG].ab6,PointsEvt);
				y = GraphAdd(g,GRF_FLOAT|GRF_YAXIS,VoieTampon[voie].traitees->t.rval,PointsEvt);
				GraphDataName(g,x,"t(ms)");
				GraphDataName(g,y,VoieManip[voie].nom);
				break;
			}
			t->numx = x;
			if(y >= 0) GraphDataRGB(g,y,c->r,c->g,c->b);
			break;
		  case MONIT_MOYEN:
			x = GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitEvtAb6[voie][0]),VoieTampon[voie].somme.taille);
			y = GraphAdd(g,GRF_FLOAT|GRF_YAXIS,VoieTampon[voie].somme.val,VoieTampon[voie].somme.taille);
			GraphDataName(g,x,"t(ms)");
			GraphDataName(g,y,VoieManip[voie].nom);
			if(y >= 0) GraphDataRGB(g,y,c->r,c->g,c->b);
			t->numx = x;
			break;
		  case MONIT_PATTERN:
			x = GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitEvtAb6[voie][0]),VoieTampon[voie].pattern.dim);
			y = GraphAdd(g,GRF_FLOAT|GRF_YAXIS,VoieTampon[voie].pattern.val,VoieTampon[voie].pattern.dim);
			GraphDataName(g,x,"t(ms)");
			GraphDataName(g,y,VoieManip[voie].nom);
			if(y >= 0) GraphDataRGB(g,y,c->r,c->g,c->b);
			t->numx = x;
			break;
		}
	}

	/* ... Elements au niveau fenetre, les axes venant d'etre definis ... */
	switch(f->type) {
	  case MONIT_SIGNAL:
		total = 0.0; kx = -1;
		for(j=0, t=f->trace; j<f->nb; j++,t++) {
			voie = t->voie; if(!VoieTampon[voie].lue) continue;
			switch(t->var) {
			  case TRMT_AU_VOL:
				if(!VoieTampon[voie].brutes->t.sval) break;
				if(DureeTampons > total) { total = DureeTampons; kx = t->numx; }
				break;
			  case TRMT_PRETRG:
				if(!VoieTampon[voie].traitees->t.rval) break;
				duree = (float)VoieTampon[voie].traitees->max * VoieTampon[voie].trmt[TRMT_PRETRG].ab6[1];
				if(duree > total) { total = duree; kx = t->numx; }
				break;
			}
		}
		if(f->axeX.r.max > total) f->axeX.r.max = total;
		zoom = total / f->axeX.r.max;
		// printf("Zoom pour %s: %g / %g = %d\n",f->titre,total,f->axeX.r.max,zoom);
		GraphZoom(g,zoom,1);
		if(kx >= 0) GraphAxisDefine(g,2 * kx);
		if(f->axeY.i.min == f->axeY.i.max) GraphAxisAutoRange(g,GRF_YAXIS);
		else GraphAxisIntRange(g,GRF_YAXIS,f->axeY.i.min,f->axeY.i.max);
		GraphAxisAutoGrad(g,GRF_YAXIS);
		if(MonitChgtUnite) GraphAxisChgtUnite(g,GRF_YAXIS,MonitUnitesADU);
		GraphUse(g,0);
		break;
	  case MONIT_EVENT:
		if(f->axeY.i.min == f->axeY.i.max) GraphAxisAutoRange(g,GRF_YAXIS);
		else GraphAxisIntRange(g,GRF_YAXIS,f->axeY.i.min,f->axeY.i.max);
		GraphAxisReset(g,GRF_YAXIS);
		GraphUse(g,0);
	 	if(MonitChgtUnite) GraphAxisChgtUnite(g,GRF_YAXIS,MonitUnitesADU);
		break;
	  case MONIT_HISTO:
		HistoGraphAdd(f->p.h.histo,g);
		GraphAxisDefine(g,0); /* defini deja l'axe des X pour GraphGetFloatRange */
		if(f->axeX.r.min == f->axeX.r.max) GraphAxisAutoRange(g,GRF_XAXIS);
		else GraphAxisFloatRange(g,GRF_XAXIS,f->axeX.r.min,f->axeX.r.max);
	 	if(MonitChgtUnite) GraphAxisChgtUnite(g,GRF_XAXIS,MonitUnitesADU);
		if(f->axeY.i.max) GraphAxisIntRange(g,GRF_YAXIS,0,f->axeY.i.max);
		else GraphAxisAutoRange(g,GRF_YAXIS);
		GraphUse(g,f->axeX.pts);
		break;
	  case MONIT_2DHISTO:
		break;
	  case MONIT_BIPLOT:
		GraphAxisDefine(g,0); /* defini deja l'axe des X pour GraphGetFloatRange */
		GraphAxisDefine(g,1); /* defini deja l'axe des Y pour GraphGetFloatRange */
		GraphDataUse(g,0,f->p.b.nb);
		GraphDataUse(g,1,f->p.b.nb);
		if(f->axeX.r.min == f->axeX.r.max) GraphAxisAutoRange(g,GRF_XAXIS);
		else GraphAxisFloatRange(g,GRF_XAXIS,f->axeX.r.min,f->axeX.r.max);
		if(f->axeY.r.min == f->axeY.r.max) GraphAxisAutoRange(g,GRF_YAXIS);
		else GraphAxisFloatRange(g,GRF_YAXIS,f->axeY.r.min,f->axeY.r.max);
		break;
	  case MONIT_FFT:
		GraphAxisDefine(g,0); /* defini deja l'axe des X pour GraphGetFloatRange */
		GraphAxisDefine(g,1); /* defini deja l'axe des Y pour GraphGetFloatRange */
		if(f->axeX.r.min < 0.0) f->axeX.r.min = 1.0;
		if(f->axeX.r.min > f->axeX.r.max) f->axeX.r.max = f->axeX.r.min * 10.0;
		if(f->axeX.r.min < f->axeX.r.max) GraphAxisFloatRange(g,GRF_XAXIS,f->axeX.r.min,f->axeX.r.max);
		else GraphAxisAutoRange(g,GRF_XAXIS);
		if(f->axeY.r.min == f->axeY.r.max) GraphAxisAutoRange(g,GRF_YAXIS);
		else {
			if(f->axeY.r.min <= 0.0) f->axeY.r.min = 0.1;
			if(f->axeY.r.max <= f->axeY.r.min) f->axeY.r.max = f->axeY.r.min * 10.0;
			GraphAxisFloatRange(g,GRF_YAXIS,f->axeY.r.min,f->axeY.r.max);
		}
		GraphAxisLog(g,GRF_XAXIS,f->p.f.xlog);
		GraphAxisLog(g,GRF_YAXIS,1);
		break;
	  case MONIT_PATTERN:
	  case MONIT_MOYEN:
		if(f->axeY.i.min == f->axeY.i.max) GraphAxisAutoRange(g,GRF_YAXIS);
		else GraphAxisIntRange(g,GRF_YAXIS,f->axeY.i.min,f->axeY.i.max);
		GraphAxisReset(g,GRF_YAXIS);
		GraphUse(g,0);
		if(MonitChgtUnite) GraphAxisChgtUnite(g,GRF_YAXIS,MonitUnitesADU);
		break;
	}

	GraphMode(g,GRF_2AXES | GRF_LEGEND | (f->grille? GRF_GRID: 0));
	GraphParmsSave(g);

	return(g);
}
/* ========================================================================== */
static Graph MonitFenBuild(TypeMonitFenetre *f) {
	Graph g;
	
	MonitFenClear(f);
	MonitFenDetach(f);
	g = MonitFenCreeGraphe(f,1);
	if(f->posx == OPIUM_AUTOPOS) f->posx = 20;
	if(f->posy == OPIUM_AUTOPOS) f->posy = 100;
	if(g) OpiumPosition(g->cdr,f->posx,f->posy);
	return(g);
}
/* ========================================================================== */
void MonitFenClear(TypeMonitFenetre *f) {
	Graph g;

//	printf("(%s) Doit effacer la fenetre %s '%s' avec g=%08llX\n",__func__,(f->incluse)?"incluse":"volante",f->titre,(IntAdrs)(f->g));
	if(!f->incluse && (g = f->g)) {
//		printf("(%s) Cadre C=%08llX a effacer\n",__func__,(IntAdrs)g->cdr);
		if(OpiumDisplayed(g->cdr)) OpiumClear(g->cdr);
//		printf("(%s) Graph G=%08llX a effacer\n",__func__,(IntAdrs)g);
		GraphDelete(g);
//		printf("(%s) Fenetre sans graphique defini\n",__func__);
		f->g = 0;
	}
}
/* ========================================================================== */
void MonitFenDetach(TypeMonitFenetre *f) {
	TypeMonitTrace *t;
	HistoData hd; HistoDeVoie vh,prec,svt;
	int j; int voie;

	if(f->type == MONIT_HISTO) {
//		printf("(MonitFenDetach) Liberation de la fenetre #%d (%s), histo @%08X\n",num,f->titre,(hexa)(f->p.h.histo));
		if(f->p.h.histo) {
			for(j=0, t=f->trace; j<f->nb; j++,t++) {
				voie = t->voie;
				hd = f->p.h.hd[j]; f->p.h.hd[j] = 0;
				vh = VoieHisto[voie]; prec = 0;
				while(vh) {
					svt = vh->suivant;
					if(vh->hd == hd) {
						if(prec == 0) VoieHisto[voie] = svt;
						else prec->suivant = svt;
						free(vh);
						break; /* une HistoData ne peut etre partagee, donc elle est referencee 1 seule fois */
					} else prec = vh;
					vh = svt;
				}
			}
			/* HistoErase(f->p.h.histo); */ HistoDelete(f->p.h.histo); f->p.h.histo = 0;
//			printf("(MonitFenDetach) Fenetre #%d (%s) liberee, histo @%08X\n",num,f->titre,(hexa)(f->p.h.histo));
		}
	} else if(f->type == MONIT_2DHISTO) {
			if(f->p.s.histo) {
//				printf("(%s) Detache les donnees de l'histo2D '%s' @%08llX\n",__func__,f->titre,(IntAdrs)(f->p.s.histo));
				hd = (HistoData)f->p.s.histo;
				for(j=0, t=f->trace; j<f->nb; j++,t++) {
					voie = t->voie;
					vh = VoieHisto[voie]; prec = 0;
					while(vh) {
						svt = vh->suivant;
						if(vh->hd == hd) {
							if(prec == 0) VoieHisto[voie] = svt;
							else prec->suivant = svt;
//							printf("(%s) Efface les donnees de la voie %s @%08llX\n",__func__,VoieManip[voie].nom,(IntAdrs)vh);
							free(vh); VoieHisto[voie] = 0;
							break; /* une HistoData ne peut etre partagee, donc elle est referencee 1 seule fois */
						} else prec = vh;
						vh = svt;
					}
				}
//				printf("(%s) Efface l'histo2D '%s' @%08llX\n",__func__,f->titre,(IntAdrs)(f->p.s.histo));
				H2DDelete(f->p.s.histo); f->p.s.histo = 0;
			}
	} else if(f->type == MONIT_FFT) {
		for(j=0; j<f->nb; j++) if(f->p.f.spectre[j]) {
			free(f->p.f.spectre[j]);
			f->p.f.spectre[j] = 0;
		}
	}
}
/* ========================================================================== 
void MonitFenNettoie(int voie, char type_tampon, char type_fen) {
	int i,j;
	TypeMonitFenetre *f; TypeMonitTrace *t; Graph g;

	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if(f->type == type_fen) {
		for(j=0, t=f->trace; j<f->nb; j++,t++) {
			if((t->voie == voie) && (t->var == type_tampon)) break;
		}
		if(j < f->nb) {
			MonitFenClear(f); MonitFenDetach(f);
		}
	}
}
   ========================================================================== */
static int MonitFenRetire(Menu menu, MenuItem item) {
	int i; Graph g;

	if(MonitFenNb) {
		if(OpiumReadList(L_("Quelle fenetre"),MonitFenTitre,&MonitFenNum,32) == PNL_CANCEL)
			return(0);
	} else MonitFenNum = 0;

	if(MonitFenNum >= MonitFenNb) {
		OpiumError(L_("Choix pas tres logique..."));
		return(0);
	}
	if((g = MonitFen[MonitFenNum].g)) {
		OpiumGetPosition(g->cdr,&(MonitFen[MonitFenNum].posx),&(MonitFen[MonitFenNum].posy));
		OpiumGetSize    (g->cdr,&(MonitFen[MonitFenNum].larg),&(MonitFen[MonitFenNum].haut));
		MonitFenClear(&(MonitFen[MonitFenNum]));
	}
	MonitFenDetach(&(MonitFen[MonitFenNum]));
	memcpy(&MonitFenLue,&(MonitFen[MonitFenNum]),sizeof(TypeMonitFenetre));
	MonitFenNb--;
	for(i=MonitFenNum; i<MonitFenNb; i++)
		memcpy(MonitFen + i,MonitFen + i + 1,sizeof(TypeMonitFenetre));
	strcpy(MonitFen[MonitFenNb].titre,MonitNouvelle);
	MonitFen[MonitFenNb + 1].titre[0] = '\0';
	MonitFenModifiees = 1;

	return(0);
}
/* ========================================================================== */
static int MonitFenListe(Menu menu, MenuItem item) {
	int i; TypeMonitFenetre *f;

#ifdef AVEC_OPIUM
	int j,l,hlim;
	TypeMonitTrace *t; TypeMonitRgb *nuance;

	if(!OpiumDisplayed(tMonit->cdr)) OpiumDisplay(tMonit->cdr); /* pour bien afficher des la premiere fois */
	TermEmpty(tMonit);
	TermHold(tMonit);
	TermPrint(tMonit,"------------------------------------------------------------\n");
	l = 2;
	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) l += (2 + f->nb);
	hlim = WndPixToLig(OpiumServerHeigth(0)) - 5;
	if(l > hlim) l = hlim;
	TermLines(tMonit,l);
	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) {
		MonitFenMemo(f);
		TermPrint(tMonit,"%2d/ %-32s ",i+1,f->titre);
		if(f->type == MONIT_HISTO)
			TermPrint(tMonit,"[%-5g .. %5g] ",f->axeX.r.min,f->axeX.r.max);
		else if((f->type == MONIT_BIPLOT) || (f->type == MONIT_2DHISTO))
			TermPrint(tMonit,"[%-5g .. %5g] x [%-5g .. %5g] ",f->axeX.r.min,f->axeX.r.max,f->axeY.r.min,f->axeY.r.max);
		else if((f->type == MONIT_FFT) || (f->type == MONIT_FONCTION))
			TermPrint(tMonit,"[%-5g .. %5g] ",f->axeY.r.min,f->axeY.r.max);
		else TermPrint(tMonit,"[%-5d .. %5d] ",f->axeY.i.min,f->axeY.i.max);
		switch(f->type) {
		  case MONIT_SIGNAL:
			TermPrint(tMonit,L_("(%g ms de signal)"),f->axeX.r.max);
			break;
		  case MONIT_HISTO:
			TermPrint(tMonit,L_("(%s sur %d bins)"),MonitFenTypeName[f->type],f->axeX.pts);
			break;
		  case MONIT_FFT:
			TermPrint(tMonit,L_("(%s sur %d x %d echantillons %s evts)"),MonitFenTypeName[f->type],
				f->p.f.intervalles,f->axeX.pts,f->p.f.excl? "avec": "hors");
			break;
		  case MONIT_FONCTION:
		  case MONIT_BIPLOT:
			TermPrint(tMonit,L_("(%s[evts])"),MonitFenTypeName[f->type]);
			break;
		  case MONIT_2DHISTO:
			TermPrint(tMonit,L_("(%s sur %d x %d bins)"),MonitFenTypeName[f->type],f->axeX.pts,f->axeY.pts);
			break;
		  case MONIT_EVENT:
			TermPrint(tMonit,"(%s)",MonitFenTypeName[f->type]);
			break;
		  case MONIT_MOYEN:
		  case MONIT_PATTERN:
			TermPrint(tMonit,"(%s%s)",MonitFenTypeName[f->type],f->nb?"s":"");
			break;
		}
		TermPrint(tMonit,L_(": %d voie%c\n"),f->nb,(f->nb > 1)? 's': ' ');
		t = f->trace;
		for(j=0; j<f->nb; j++,t++) {
			switch(f->type) {
			  case MONIT_SIGNAL:
			  case MONIT_FFT:
			  case MONIT_EVENT:
				TermPrint(tMonit,"    | %-16s %-8s     ",VoieManip[t->voie].nom,TrmtClassesListe[t->var]);
				break;
			  case MONIT_HISTO:
			  case MONIT_2DHISTO:
			  case MONIT_FONCTION:
				TermPrint(tMonit,"    | %-12s %-16s ",VarName[t->var],VoieManip[t->voie].nom);
				break;
			  case MONIT_BIPLOT:
				TermPrint(tMonit,"    | %-12s ",PlotVarList[t->var]);
				break;
			  case MONIT_MOYEN:
			  case MONIT_PATTERN:
				TermPrint(tMonit,"    | %-16s              ",VoieManip[(int)t->voie].nom);
				break;
			}
			if(f->type != MONIT_2DHISTO) {
				nuance = &(t->rgb);
				TermPrint(tMonit,L_(" (%6.2f%% rouge, %6.2f%% verte, %6.2f%% bleue)"),nuance->r,nuance->g,nuance->b);
			}
			TermPrint(tMonit,"\n");
		}
		TermPrint(tMonit,L_("    .                                fenetre %d x %d en (%d, %d)\n"),
			f->larg,f->haut,f->posx,f->posy);
	}
	TermPrint(tMonit,"------------------------------------------------------------\n");
	TermRelease(tMonit);
//	if((tMonit->cdr)->displayed) OpiumClear(tMonit->cdr);
	OpiumDisplay(tMonit->cdr);
#else /* !AVEC_OPIUM */
	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) {
		MonitFenMemo(f); MonitFenDump(f);
	}
#endif /* !AVEC_OPIUM */

	return(0);
}
/* ========================================================================== */
static int MonitFenDimensions(Menu menu, MenuItem item) {
	int i; TypeMonitFenetre *f; Panel p;
	char refait_suivi; int larg[MAXMONIT],haut[MAXMONIT];

	refait_suivi = 0;
	p = PanelCreate(2 * (MonitFenNb + 1)); PanelColumns(p,2); PanelMode(p,PNL_BYLINES);
	// PanelRemark(p,"largeur"); PanelRemark(p,"hauteur");
	PanelItemSelect(p,PanelText(p,0," X",5),0);
	PanelItemSelect(p,PanelText(p,0," Y",5),0);
	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) {
		MonitFenMemo(f); larg[i] = f->larg; haut[i] = f->haut;
		PanelLngr(p,PanelInt(p,f->titre,&(f->larg)),5);
		PanelLngr(p,PanelInt(p,0,&(f->haut)),5);
	}
	if(OpiumExec(p->cdr) == PNL_CANCEL) {
		for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) {
			f->larg = larg[i]; f->haut = haut[i];
		}
	}
	PanelDelete(p);

	return(0);
}
/* ========================================================================== */
static int MonitFenAutorise(Menu menu, MenuItem item) {
	int i,j;
	TypeMonitFenetre *f; Graph g;

	if(MonitFenNb == 0) {
		OpiumError(L_("Il n'y a PAS de fenetre definie"));
		return(0);
	}
	if(pMonitFenAffiche) {
		if(PanelItemMax(pMonitFenAffiche) < MonitFenNb) {
			PanelDelete(pMonitFenAffiche);
			pMonitFenAffiche = 0;
		} else PanelErase(pMonitFenAffiche);
	}
	if(pMonitFenAffiche == 0) pMonitFenAffiche = PanelCreate(MonitFenNb);
	for(i=0; i<MonitFenNb; i++) {
		j = PanelKeyB(pMonitFenAffiche,MonitFen[i].titre,L_(MONIT_DISPLAY_CLES),&(MonitFen[i].affiche),6);
		PanelItemColors(pMonitFenAffiche,j,SambaRougeVertJauneOrange,4);
	}
	PanelOnApply(pMonitFenAffiche,MonitFenSuivies,0);
	PanelOnOK(pMonitFenAffiche,MonitFenSuivies,0);
	if(OpiumExec(pMonitFenAffiche->cdr) == PNL_CANCEL) return(0);
	MonitFenModifiees = 1;
	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if(f->affiche != MONIT_FOLW) {
		if((f->affiche == MONIT_SHOW) || (f->affiche == MONIT_ALWS)) MonitFenAffiche(f,1);
		else if((g = f->g)) { if(OpiumDisplayed(g->cdr)) OpiumClear(g->cdr); }
	}

	return(0);
}
/* ========================================================================== */
int MonitFenMemo(TypeMonitFenetre *f) {
	Cadre cdr; Graph g;
	
	if((g = f->g)) {
		cdr = g->cdr;
		if(!OpiumDisplayed(cdr)) return(0);
		OpiumGetPosition(cdr,&(f->posx),&(f->posy));
		OpiumGetSize    (cdr,&(f->larg),&(f->haut));
		if(f->posx <  5) f->posx =  5; if(f->posx > (OpiumServerWidth(0)  - 40)) f->posx = OpiumServerWidth(0)  - 40;
		if(f->posy < 15) f->posy = 15; if(f->posy > (OpiumServerHeigth(0) - 40)) f->posy = OpiumServerHeigth(0) - 40;
		if(f->type == MONIT_HISTO) {
			if(f->axeX.r.min != f->axeX.r.max) {
				GraphGetFloatRange(g,GRF_XAXIS,&(f->axeX.r.min),&(f->axeX.r.max));
				if(f->axeX.r.min >= f->axeX.r.max) f->axeX.r.min = f->axeX.r.max / 10.0;
			}
		} else if(f->type == MONIT_FFT) {
			if(f->axeY.r.min != f->axeY.r.max) {
				GraphGetFloatRange(g,GRF_YAXIS,&(f->axeY.r.min),&(f->axeY.r.max));
				if(f->axeY.r.min >= f->axeY.r.max) f->axeY.r.min = f->axeY.r.max / 10.0;
			}
			GraphGetFloatRange(g,GRF_XAXIS,&(f->axeX.r.min),&(f->axeX.r.max));
			f->p.f.xlog = GraphAxisIsLog(g,GRF_XAXIS);
			if(f->p.f.xlog) {
				if(f->axeX.r.min <= 0.0) f->axeX.r.min = 1.0;
				if(f->axeX.r.max <= 0.0) f->axeX.r.max = 1000.0;
				if(f->axeX.r.min >= f->axeX.r.max) f->axeX.r.min = f->axeX.r.max / 10.0;
			}
		} else if((f->type == MONIT_2DHISTO) || (f->type == MONIT_BIPLOT)) {
			if(f->axeX.r.min != f->axeX.r.max) {
				GraphGetFloatRange(g,GRF_XAXIS,&(f->axeX.r.min),&(f->axeX.r.max));
				if(f->axeX.r.min >= f->axeX.r.max) f->axeX.r.min = f->axeX.r.max / 10.0;
			}
			if(f->axeY.r.min != f->axeY.r.max) {
				GraphGetFloatRange(g,GRF_YAXIS,&(f->axeY.r.min),&(f->axeY.r.max));
				if(f->axeY.r.min >= f->axeY.r.max) f->axeY.r.min = f->axeY.r.max / 10.0;
			}
		} else if(f->type == MONIT_FONCTION) {
			if(f->axeY.r.min != f->axeY.r.max) {
				GraphGetFloatRange(g,GRF_YAXIS,&(f->axeY.r.min),&(f->axeY.r.max));
				if(f->axeY.r.min >= f->axeY.r.max) f->axeY.r.min = f->axeY.r.max / 10.0;
			}
		} else {
			if(f->axeY.i.min != f->axeY.i.max) {
				GraphGetIntRange(g,GRF_YAXIS,&(f->axeY.i.min),&(f->axeY.i.max));
				if(f->axeY.i.min >= f->axeY.i.max) f->axeY.i.min = f->axeY.i.max - 1;
			}
		}
	}
	MonitFenDimMin(f);

	return(0);
}
/* ========================================================================== */
void MonitTraceAjusteY(TypeMonitTrace *t, TypeTamponDonnees *donnees, Graph g) {
	int dim; int64 debut;
	TypeDonnee *sptr,sval,smin,smax;
	TypeSignal *rptr,rval,rmin,rmax;
	int i,j; 
	
	dim = donnees->nb / 2;
	if(dim) {
#ifdef EN_AVANCANT
		debut = donnees->prem + (donnees->nb - dim);
		j = Modulo(debut,donnees->max);
		if(donnees->type) {
			rptr = donnees->t.rval + j; 
			rmin = rmax = 0.0;
			for(i=0; i<dim; i++) {
				rval = (double)*rptr++;
				if(i) { if(rval < rmin) rmin = rval; if(rval > rmax) rmax = rval; }
				else rmin = rmax = rval;
				if(++j == donnees->max) { j = 0; rptr = donnees->t.rval; }
			}
			GraphAxisFloatRange(g,GRF_YAXIS,rmin,rmax);
		} else { 
			sptr = donnees->t.sval + j;
			smin = smax = 0;
			for(i=0; i<dim; i++) {
				sval = *sptr++;
				if(i) { if(sval < smin) smin = sval; if(sval > smax) smax = sval; }
				else smin = smax = sval;
				if(++j == donnees->max) { j = 0; sptr = donnees->t.sval; }
			}
			GraphAxisIntRange(g,GRF_YAXIS,smin,smax);
		}
#else  /* !EN_AVANCANT */
		debut = donnees->prem + donnees->nb;
		j = Modulo(debut,donnees->max);
		if(donnees->type) {
			rptr = donnees->t.rval + j; 
			rmin = rmax = 0.0;
			for(i=0; i<dim; i++) {
				rval = (double)*(--rptr);
				if(i) { if(rval < rmin) rmin = rval; if(rval > rmax) rmax = rval; }
				else rmin = rmax = rval;
				if(--j < 0) { j = donnees->max; rptr = donnees->t.rval + j; }
			}
			GraphAxisFloatRange(g,GRF_YAXIS,rmin,rmax);
		} else { 
			sptr = donnees->t.sval + j;
			smin = smax = 0;
			for(i=0; i<dim; i++) {
				sval = *(--sptr);
				if(i) { if(sval < smin) smin = sval; if(sval > smax) smax = sval; }
				else smin = smax = sval;
				if(--j < 0) { j = donnees->max; sptr = donnees->t.sval + j; }
			}
			GraphAxisIntRange(g,GRF_YAXIS,smin,smax);
		}
#endif /* !EN_AVANCANT */
		t->ajuste = 0;
	}
}
/* ========================================================================== */
int MonitFenAjusteY() {
	int i,j,k,num,nb;
	TypeMonitFenetre *f; TypeMonitTrace *t;
	char *fenetre[MAXMONIT+2]; short origine[MAXMONIT+2];
	char *trace[MAXTRACES+2]; char nomtrace[MAXTRACES][32];

	nb = 0;
	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if(f->type == MONIT_SIGNAL) {
		fenetre[nb] = MonitFen[i].titre; origine[nb] = i; nb++;
	}
	fenetre[nb] = "\0";
	if(nb == 0) { OpiumNote(L_("Il n'y a PAS de fenetre concernee")); return(0); }
	else if(nb > 1) {
		num = 0;
		if(OpiumReadList(L_("De quelle fenetre"),fenetre,&num,32) == PNL_CANCEL) return(0);
	} else num = 0;
	f = &(MonitFen[origine[num]]);
	if(f->nb == 0) { OpiumNote(L_("Cette fenetre est vide")); return(0); }
	else if(f->nb > 1) {
		for(j=0, t=f->trace; j<f->nb; j++,t++) {
			snprintf(&(nomtrace[j][0]),32,"%s",VoieManip[t->voie].nom);
			trace[j] = &(nomtrace[j][0]);
		}
		trace[j] = "\0";
		k = 0;
		if(OpiumReadList(L_("d'apres la trace"),trace,&k,32) == PNL_CANCEL) return(0);
	} else k = 0;
	t = &(f->trace[k]);
	t->ajuste = 1;
	if(!Acquis[AcquisLocale].etat.active) {
		int voie; char trmt; TypeTamponDonnees *donnees; Graph g;
		if((g = f->g)) {
			// printf("%s/ Ajustement de l'axe Y demande pour la fenetre '%s', d'apres sa trace #%d\n",DateHeure(),f->titre,k+1);
			voie = t->voie; if(VoieTampon[voie].lue <= 0) return(0);
			trmt = t->var; donnees = &(VoieTampon[voie].trmt[trmt].donnees);
			MonitTraceAjusteY(t,donnees,g);
			// printf("%s/ Ajustement de l'axe Y termine pour la fenetre '%s', d'apres sa trace #%d\n",DateHeure(),f->titre,k+1);
			if(OpiumAlEcran(g)) OpiumRefresh(g->cdr);
		}
	}

	return(0);
}
/* ========================================================================== */
static int MonitFenDetecteur(Menu menu, MenuItem item) {
	int i,j,k,nb;
	int cap;
	char nomvoie[MODL_NOM];
	TypeMonitFenetre *f; TypeMonitTrace *t;
	
	if(MonitFenNb == 0) {
		OpiumError(L_("Il n'y a PAS de fenetre definie"));
		return(0);
	}

	if(OpiumExec(pMonitFenDetecteur->cdr) == PNL_CANCEL) return(0);
	if(pMonitFenChange) {
		if(PanelItemMax(pMonitFenChange) < MonitFenNb) {
			PanelDelete(pMonitFenChange);
			pMonitFenChange = 0;
		} else PanelErase(pMonitFenChange);
	}
	if(pMonitFenChange == 0) pMonitFenChange = PanelCreate(MonitFenNb);
	nb = 0 ;
	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) {
		t = f->trace;
		for(j=0; j<f->nb; j++,t++) if(t->bolo == MonitBoloAvant) break;
		if(j < f->nb) {
			k = PanelBool(pMonitFenChange,f->titre,&(f->change));
			PanelItemColors(pMonitFenChange,k,SambaRougeVertJauneOrange,2);
			nb++;
		} else f->change = 0;
	}
	if(!nb) {
		OpiumError(L_("Aucun graphique n'utilise %s"),Bolo[MonitBoloAvant].nom);
		return(0);
	}
	if(OpiumExec(pMonitFenChange->cdr) == PNL_CANCEL) return(0);
	MonitFenModifiees = 1;
	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if(f->change && (BoloNum != MonitBoloAvant)) {
		t = f->trace;
		for(j=0; j<f->nb; j++,t++) if(t->bolo == MonitBoloAvant) {
			t->bolo = BoloNum;
			strcpy(nomvoie,VoieManip[t->voie].nom);
			for(cap=0; cap<Bolo[BoloNum].nbcapt; cap++) {
				if(!strcmp(nomvoie,Bolo[BoloNum].captr[cap].nom)) break;
			}
			if(cap == Bolo[BoloNum].nbcapt) {
				OpiumError(L_("Pas de voie '%s' pour le detecteur '%s'"),nomvoie,Bolo[BoloNum].nom);
			} else {
				printf(". Dans %s, %s ",f->titre,VoieManip[t->voie].nom);
				t->cap = cap; t->voie = Bolo[BoloNum].captr[cap].voie;
				printf("est remplacee par %s\n",VoieManip[t->voie].nom);
			}
			for(cap=0; cap<Bolo[BoloNum].nbcapt; cap++) t->liste_voies[cap] = Bolo[BoloNum].captr[cap].nom;
			t->liste_voies[cap] = "\0";
		}
	}
	
	return(0);
}
/* ========================================================================== */
static int MonitFenMemoToutes(Menu menu, MenuItem item) {
	TypeMonitFenetre *f; int i;

	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) MonitFenMemo(f);
	return(0);
}
/* ========================================================================== */
static int MonitFenZoom(Menu menu, MenuItem item) {
	TypeMonitFenetre *f; Graph g;
	hexa zoom;
	int j; int voie; float duree,total; TypeMonitTrace *t;

	if(MonitFenNb > 1) {
		if(OpiumReadList(L_("Quelle fenetre"),MonitFenTitre,&MonitFenNum,32) == PNL_CANCEL)
			return(0);
	} else MonitFenNum = 0;
	if(MonitFenNum >= MonitFenNb) {
		OpiumError("Choix pas tres logique..."); // il(elle) a repondu: "(nouvelle fenetre)" !! :(
		return(0);
	}
	f = &(MonitFen[MonitFenNum]);
	MonitFenMemo(f);
	memcpy(&MonitFenLue,f,sizeof(TypeMonitFenetre));
	switch(f->type) {
	  case MONIT_SIGNAL:
//		MonitY.i.min = f->axeY.i.min; MonitY.i.max = f->axeY.i.max;
//		MonitX.r.max = f->axeX.r.max;
		PanelTitle(pMonitFenZoomData,MonitFenTitre[MonitFenNum]);
		if(OpiumExec(pMonitFenZoomData->cdr) == PNL_CANCEL) return(0);
		memcpy(f,&MonitFenLue,sizeof(TypeMonitFenetre));
//		f->axeY.i.min = MonitY.i.min; f->axeY.i.max = MonitY.i.max;
//		f->axeX.r.max = MonitX.r.max;
		total = 0.0;
		for(j=0, t=f->trace; j<f->nb; j++,t++) {
			voie = t->voie;
			switch(t->var) {
			  case TRMT_AU_VOL:
				if(!VoieTampon[voie].brutes->t.sval) break;
				if(total < DureeTampons) total = DureeTampons;
				break;
			  case TRMT_PRETRG:
				if(!VoieTampon[voie].traitees->t.rval) break;
				duree = (float)VoieTampon[voie].traitees->max * VoieTampon[voie].trmt[TRMT_PRETRG].ab6[1];
				if(total < duree) total = duree;
				break;
			}
		}
		if(f->axeX.r.max > total) f->axeX.r.max = total;
		zoom = total / f->axeX.r.max;
		// f->p.b.cadre = f->larg * zoom;
		if((g = f->g)) {
			// GraphResize(g,f->p.b.cadre,f->haut);
			GraphZoom(g,zoom,1);
			GraphAxisReset(g,GRF_XAXIS);
			if(f->axeY.i.min == f->axeY.i.max) GraphAxisAutoRange(g,GRF_YAXIS);
			else GraphAxisIntRange(g,GRF_YAXIS,f->axeY.i.min,f->axeY.i.max);
		}
		break;
	  case MONIT_EVENT:
//		MonitY.i.min = f->axeY.i.min; MonitY.i.max = f->axeY.i.max;
		PanelTitle(pMonitFenZoomEvts,MonitFenTitre[MonitFenNum]);
		if(OpiumExec(pMonitFenZoomEvts->cdr) == PNL_CANCEL) return(0);
		memcpy(f,&MonitFenLue,sizeof(TypeMonitFenetre));
//		f->axeY.i.min = MonitY.i.min; f->axeY.i.max = MonitY.i.max;
		if((g = f->g)) {
			GraphAxisReset(g,GRF_XAXIS);
			if(f->axeY.i.min == f->axeY.i.max) GraphAxisAutoRange(g,GRF_YAXIS);
			else GraphAxisIntRange(g,GRF_YAXIS,f->axeY.i.min,f->axeY.i.max);
		}
		break;
	  case MONIT_PATTERN:
	  case MONIT_MOYEN:
		PanelTitle(pMonitFenZoomMoyen,MonitFenTitre[MonitFenNum]);
		if(OpiumExec(pMonitFenZoomMoyen->cdr) == PNL_CANCEL) return(0);
		memcpy(f,&MonitFenLue,sizeof(TypeMonitFenetre));
		if((g = f->g)) {
			GraphAxisReset(g,GRF_XAXIS);
			if(f->axeY.i.min == f->axeY.i.max) GraphAxisAutoRange(g,GRF_YAXIS);
			else GraphAxisIntRange(g,GRF_YAXIS,f->axeY.i.min,f->axeY.i.max);
		}
		break;
	  case MONIT_HISTO:
//		MonitX.r.min = f->axeX.r.min; MonitX.r.max = f->axeX.r.max; MonitEvts = f->axeY.i.max;
		PanelTitle(pMonitFenZoomHisto,MonitFenTitre[MonitFenNum]);
		if(OpiumExec(pMonitFenZoomHisto->cdr) == PNL_CANCEL) return(0);
		memcpy(f,&MonitFenLue,sizeof(TypeMonitFenetre));
//		f->axeX.r.min = MonitX.r.min; f->axeX.r.max = MonitX.r.max; f->axeY.i.max = MonitEvts;
		if((g = f->g)) {
			if(f->axeX.r.min == f->axeX.r.max) GraphAxisAutoRange(g,GRF_XAXIS);
			else GraphAxisFloatRange(g,GRF_XAXIS,f->axeX.r.min,f->axeX.r.max);
			if(f->axeY.i.max) GraphAxisIntRange(g,GRF_YAXIS,0,f->axeY.i.max);
			else GraphAxisAutoRange(g,GRF_YAXIS);
		}
		break;
	  case MONIT_FONCTION:
//		MonitX.i.max = f->axeX.i.max;
//		MonitY.r.min = f->axeY.r.min; MonitY.r.max = f->axeY.r.max;
		PanelTitle(pMonitFenZoomFctn,MonitFenTitre[MonitFenNum]);
		if(OpiumExec(pMonitFenZoomFctn->cdr) == PNL_CANCEL) return(0);
		memcpy(f,&MonitFenLue,sizeof(TypeMonitFenetre));
//		f->axeX.i.max = MonitX.i.max;
//		f->axeY.r.min = MonitY.r.min; f->axeY.r.max = MonitY.r.max;
		if((g = f->g)) {
			if(f->axeX.i.max > 0.0) GraphAxisIntRange(g,GRF_XAXIS,1,f->axeX.i.max);
			else GraphAxisAutoRange(g,GRF_XAXIS);
			if(f->axeY.r.min == f->axeY.r.max) GraphAxisAutoRange(g,GRF_YAXIS);
			else {
				if(f->axeY.r.max <= f->axeY.r.min) f->axeY.r.max = f->axeY.r.min * 10.0;
				GraphAxisFloatRange(g,GRF_YAXIS,f->axeY.r.min,f->axeY.r.max);
			}
		}
		break;
	  case MONIT_2DHISTO:
		PanelTitle(pMonitFenZoomH2D,MonitFenTitre[MonitFenNum]);
		if(OpiumExec(pMonitFenZoomH2D->cdr) == PNL_CANCEL) return(0);
		memcpy(f,&MonitFenLue,sizeof(TypeMonitFenetre));
		if((g = f->g)) {
			if(f->axeX.r.min == f->axeX.r.max) GraphAxisAutoRange(g,GRF_XAXIS);
			else GraphAxisFloatRange(g,GRF_XAXIS,f->axeX.r.min,f->axeX.r.max);
			if(f->axeY.r.min == f->axeY.r.max) GraphAxisAutoRange(g,GRF_YAXIS);
			else GraphAxisFloatRange(g,GRF_YAXIS,f->axeY.r.min,f->axeY.r.max);
		}
		break;
	  case MONIT_BIPLOT:
		PanelTitle(pMonitFenZoomBiplot,MonitFenTitre[MonitFenNum]);
		if(OpiumExec(pMonitFenZoomBiplot->cdr) == PNL_CANCEL) return(0);
		memcpy(f,&MonitFenLue,sizeof(TypeMonitFenetre));
		if((g = f->g)) {
			if(f->axeX.r.min == f->axeX.r.max) GraphAxisAutoRange(g,GRF_XAXIS);
			else GraphAxisFloatRange(g,GRF_XAXIS,f->axeX.r.min,f->axeX.r.max);
			if(f->axeY.r.min == f->axeY.r.max) GraphAxisAutoRange(g,GRF_YAXIS);
			else GraphAxisFloatRange(g,GRF_YAXIS,f->axeY.r.min,f->axeY.r.max);
		}
		break;
	  case MONIT_FFT:
//		MonitX.r.min = f->axeX.r.min; MonitX.r.max = f->axeX.r.max;
//		MonitY.r.min = f->axeY.r.min; MonitY.r.max = f->axeY.r.max;
		PanelTitle(pMonitFenZoomFft,MonitFenTitre[MonitFenNum]);
		if(OpiumExec(pMonitFenZoomFft->cdr) == PNL_CANCEL) return(0);
		memcpy(f,&MonitFenLue,sizeof(TypeMonitFenetre));
//		f->axeX.r.min = MonitX.r.min; f->axeX.r.max = MonitX.r.max;
//		f->axeY.r.min = MonitY.r.min; f->axeY.r.max = MonitY.r.max;
		if((g = f->g)) {
			if(f->axeX.r.max > 0.0) GraphAxisFloatRange(g,GRF_XAXIS,0.001,f->axeX.r.max);
			else GraphAxisAutoRange(g,GRF_XAXIS);
			if(f->axeY.r.min == f->axeY.r.max) GraphAxisAutoRange(g,GRF_YAXIS);
			else {
				if(f->axeY.r.min <= 0) f->axeY.r.min = 0.1;
				if(f->axeY.r.max <= f->axeY.r.min) f->axeY.r.max = f->axeY.r.min * 10.0;
				GraphAxisFloatRange(g,GRF_YAXIS,f->axeY.r.min,f->axeY.r.max);
			}
		}
		break;
	}

	if((g = f->g)) { if(OpiumDisplayed(g->cdr)) OpiumRefresh(g->cdr); }

	return(0);
}
/* ========================================================================== */
int MonitFenAffiche(TypeMonitFenetre *f, char reellement) {
	int j,voie,numx,numy;
	TypeMonitTrace *t; Graph g;
	TypeTamponDonnees *donnees;

//	if(!(g = f->g)) { /* suppose que 1 fen creee == 1 fen qui a son graphique */		;
		if(!(g = MonitFenBuild(f))) return(0);
//	}
	if(f->type == MONIT_SIGNAL) {
		if(OpiumDisplayed(g->cdr)) OpiumClear(g->cdr);
		for(j=0, t=f->trace; j<f->nb; j++,t++) {
			voie = t->voie; numx = 2 * j; numy = numx + 1;
		#ifdef ALAMBIQUE
			if(t->var == TRMT_AU_VOL) {
				int point1;
				GraphDataUse(g,numx,VoieTampon[voie].brutes->nb);
				GraphDataUse(g,numy,VoieTampon[voie].brutes->nb);
				//- if((VoieTampon[voie].brutes->nb >= VoieTampon[voie].brutes->max) && (VoieTampon[voie].brutes->max > 0)) {
					//- point1 = Modulo(VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien,VoieTampon[voie].brutes->max); /* point1 == brutes.prem? */
					point1 = VoieTampon[voie].brutes->prem;
					GraphDataRescroll(g,numx,point1);
					GraphDataRescroll(g,numy,point1);
				//- }
			} else {  /* (t->var == TRMT_PRETRG) */
				GraphDataUse(g,numx,VoieTampon[voie].traitees->nb);
				GraphDataUse(g,numy,VoieTampon[voie].traitees->nb);
				//- if(VoieTampon[voie].traitees->nb >= VoieTampon[voie].traitees->max) {
					GraphDataRescroll(g,numx,VoieTampon[voie].traitees->prem);
					GraphDataRescroll(g,numy,VoieTampon[voie].traitees->prem);
				//- }
			}
		#else  /* ALAMBIQUE */
			donnees = &(VoieTampon[voie].trmt[t->var].donnees);
			GraphDataUse(g,numx,donnees->nb);
			GraphDataUse(g,numy,donnees->nb);
			GraphDataRescroll(g,numx,donnees->prem);
			GraphDataRescroll(g,numy,donnees->prem);
		#endif /* ALAMBIQUE */
		}
	}
	if(reellement) OpiumDisplay(g->cdr);
	return(1);
}

static OpiumTableDeclaration MonitTagsTable;
/* ========================================================================== */
static void MonitTagsParDefaut(TypeTagDisplay *tag) {
	strcpy(tag->nom,"alpha"); tag->p = GRF_CARRE; OpiumColorCopy(&(tag->c),&(OpiumColorVal[OPIUM_COLORS_AUTO-1]));
	strcpy(tag->son,"neant");
}
/* ========================================================================== */
char MonitTagsSonorisee(TypeTagDisplay *tag) {
	return((tag->son[0] != '\0') && strcmp(tag->son,"neant")
	&& strcmp(tag->son,"none") && strcmp(tag->son,"."));
}
/* ========================================================================== */
static void MonitTagsInit() {
	int voie; int n; OpiumTableDeclaration decl;

	ArgKeyFromList(MonitTagsKeys,GraphMarkName);
	TagDefNb = 0;
	for(voie=0; voie<VOIES_MAX; voie++) OpiumColorInit(&(TagCouleur[voie]));
	OpiumTableCreate(&MonitTagsTable);
	decl = OpiumTableAdd(&MonitTagsTable,"Etiquettes",ARG_TYPE_STRUCT,(void *)&MonitTagsAS,&TagDefNb,0);
	OpiumTableFichier(decl,FichierPrefTagDefs,0);
	mSettingsTags = decl->menu; // OpiumTableMenuCreate(MonitTagsTable);

	RepertoireCreeRacine(FichierPrefTagDefs);
	SambaLogDef("Lecture des etiquettes d'evenements",0,FichierPrefTagDefs);
	if(ArgScan(FichierPrefTagDefs,MonitTagsDesc) == 0) {
		n = 0;
		/* printf("    . Affichage des etiquettes cree par defaut\n");
		 strcpy(TagDef[n].nom,"alpha"); TagDef[n].p = GRF_POINT; OpiumColorCopy(&(TagDef[n].c),&(OpiumColorVal[OPIUM_COLORS_AUTO-1])); n++;
		 strcpy(TagDef[n].nom,"beta");  TagDef[n].p = GRF_TRGLE; OpiumColorCopy(&(TagDef[n].c),&(OpiumColorVal[OPIUM_COLORS_AUTO-1])); n++;
		 strcpy(TagDef[n].nom,"gamma"); TagDef[n].p = GRF_CROSS; OpiumColorCopy(&(TagDef[n].c),&(OpiumColorVal[OPIUM_COLORS_AUTO-1])); n++; */
		MonitTagsParDefaut(&(TagDef[n]));
		TagDefNb = n;
		ArgPrint(FichierPrefTagDefs,MonitTagsDesc);
	}
	if(!TagDefNb) printf("  . Pas d'etiquette definie\n");
	MonitTagModifie = 0;
	if(ImprimeConfigs) {
		printf("#\n### Fichier: '%s' ###\n",FichierPrefTagDefs);
		ArgPrint("*",MonitTagsDesc);
	}
}
/* ========================================================================== */
static int MonitTagsEcrit() {
	if(ArgPrint(FichierPrefTagDefs,MonitTagsDesc) >0) {
		MonitTagModifie = 0;
		printf("%s/ Definition des etiquettes sauvee dans %s\n",DateHeure(),FichierPrefTagDefs);
	} else OpiumError("Sauvegarde sur '%s' impossible",FichierPrefTagDefs);
	return(0);
}
/* ========================================================================== */
void MonitTagsGlobal(int v, TypeConfigDefinition *utilisee) {
	int j,k,l,n; char sauve;

	if(!utilisee) return;
	sauve = 0;
	for(k=0; k<utilisee->nbcatg; k++) {
		l = (int)strlen(utilisee->catg[k].nom); while(l--) { if(utilisee->catg[k].nom[l] != ' ') break; };
		utilisee->catg[k].nom[++l] = '\0';
		for(n=0; n<TagDefNb; n++) if(!strcmp(TagDef[n].nom,utilisee->catg[k].nom)) break;
		if(n >= TagDefNb) {
			if(TagDefNb >= CATG_MAX)
				OpiumFail(L_("Deja %d etiquettes definies: pas d'etiquette '%s'"),CATG_MAX,utilisee->catg[k].nom);
			else {
				strcpy(TagDef[n].nom,utilisee->catg[k].nom);
				j = OpiumColorNext(&(TagCouleur[v]));
				TagDef[n].p = (n < GRF_NBMARKS)? n: GRF_POINT;
				OpiumColorCopy(&(TagDef[n].c),&(OpiumColorVal[j]));
				TagDefNb++;
				sauve = 1;
			}
		}
		utilisee->catg[k].num = n;
	}
	if(sauve) MonitTagsEcrit();
}
/* ========================================================================== */
int MonitTagsManage(Menu menu, MenuItem item) {
	OpiumExec(mSettingsTags->cdr);
	return(0);
}
/* ========================================================================== */
int MonitChoisiBoloVoie(int *voie_choisie) {
	int voie,bolo,vm,cap;

	bolo = BoloNum; vm = ModlNum;
	if(OpiumExec(pMonitBoloVoie->cdr) == PNL_CANCEL) return(0);
	if(Bolo[BoloNum].lu == DETEC_HS) {
		OpiumError("Le detecteur %s n'est pas lu",Bolo[BoloNum].nom);
		BoloNum = bolo; ModlNum = vm;
		return(0);
	}
	voie = 0;
	for(cap=0; cap<Bolo[BoloNum].nbcapt; cap++) {
		if((voie = Bolo[BoloNum].captr[cap].voie) < 0) continue;
		if(ModlNum == VoieManip[voie].type) break;
	}
	if(cap == Bolo[BoloNum].nbcapt) {
		OpiumError("Pas de voie '%s' pour le detecteur '%s'",ModeleVoie[ModlNum].nom,Bolo[BoloNum].nom);
		BoloNum = bolo; ModlNum = vm;
		return(0);
	}
	VoieNum = voie;
//	printf("(MonitChoisiBoloVoie) BoloNum=%d/%d, VoieNum=%d/%d\n",BoloNum,BoloNb,VoieNum,VoiesNb);
	*voie_choisie = voie;
	return(1);
}
/* ========================================================================== */
static int MonitFftAuVol(Menu menu, MenuItem item) {
	int voie,bolo,vm,cap;
	int n,i; int x,y;
	int nbfreq; // int dim;
	char moyenne_refait,nouveau_refait;
	char titre[80]; char raison[80]; // char nom[80];

	if(OpiumAlEcran(gMonitFftAuVol)) OpiumClear(gMonitFftAuVol->cdr);
	if(menu) {
		if(MonitFftDemande) {
			/* if(!OpiumChoix(2,SambaNonOui,"Recommencer une mesure du bruit au vol")) */
			if(!OpiumChoix(2,MonitFftSuite,L_("Nouvelle mesure du bruit au vol"))) {
				MonitFftDemande = 0;
				printf("* Spectre au vol arrete\n");
				return(0);
			}
		}
	} else if(!MonitFftDemande) return(0);
	MonitFftDemande = 0; // preventivement [si return(0);]
	bolo = MonitFftBolo; vm = MonitFftModl; voie = 0; // dim = MonitFftDim;
	if(OpiumExec(pMonitFftAuVol->cdr) == PNL_CANCEL) return(0);
	if(Bolo[MonitFftBolo].lu == DETEC_HS) {
		OpiumFail(L_("Le detecteur %s n'est pas lu"),Bolo[MonitFftBolo].nom);
		MonitFftBolo = bolo; MonitFftModl = vm;
		return(0);
	}
	for(cap=0; cap<Bolo[MonitFftBolo].nbcapt; cap++) {
		voie = Bolo[MonitFftBolo].captr[cap].voie;
		if(MonitFftModl == VoieManip[voie].type) break;
	}
	if(cap == Bolo[MonitFftBolo].nbcapt) {
		OpiumFail(L_("Pas de voie '%s' pour le detecteur '%s'"),ModeleVoie[MonitFftModl].nom,Bolo[MonitFftBolo].nom);
		MonitFftBolo = bolo; MonitFftModl = vm;
		return(0);
	}
	MonitFftVoie = voie;
	VoieNum = MonitFftVoie;

	//	CalcPi[0] = (int64)((CalcTi[0] * 1000.0) / (double)VoieTampon[MonitFftVoie].horloge); ??
	if(MonitFftDim <= 0) {
		OpiumFail(L_("Nombre d'echantillons invalide: %d <= 0"),MonitFftDim);
		return(0);
	}
	if(!CalculeFFTplan(&MonitFftBase,MonitFftDim,raison)) {
		OpiumFail(raison);
		return(0);
	}
	MonitFftNorme = (VoieEvent[MonitFftVoie].horloge / 1000.0) / (float)MonitFftDim; /* secondes, soit 1/norme en Hz */
	if(MonitFftDonnees == TRMT_PRETRG) MonitFftNorme *= VoieTampon[MonitFftVoie].trmt[MonitFftDonnees].compactage;
	nbfreq = (MonitFftDim / 2) + 1;
	if(MonitFftNbFreq != nbfreq) {
		if(MonitFftCarreSomme) { free(MonitFftCarreSomme); MonitFftCarreSomme = 0; }
		if(MonitFftCarreMoyenne) { free(MonitFftCarreMoyenne); MonitFftCarreMoyenne = 0; }
		if(MonitFftNouveau) { free(MonitFftNouveau); MonitFftNouveau = 0; }
		if(MonitFftMoyenne) { free(MonitFftMoyenne); MonitFftMoyenne = 0 ; }
		MonitFftNbFreq = nbfreq;
	}
	n = MonitFftNbFreq * sizeof(float);
	if(!MonitFftCarreSomme) {
		MonitFftCarreSomme = (float *)malloc(n);
		if(!MonitFftCarreSomme) {
			OpiumFail(L_("Manque de place memoire pour le spectre**2 somme (%d octets)"),n);
			return(0);
		}
	}
	if(!MonitFftCarreMoyenne) {
		MonitFftCarreMoyenne = (float *)malloc(n);
		if(!MonitFftCarreMoyenne) {
			OpiumFail(L_("Manque de place memoire pour le spectre**2 moyenne (%d octets)"),n);
			return(0);
		}
	}
	if(!MonitFftMoyenne) {
		MonitFftMoyenne = (float *)malloc(n);
		if(!MonitFftMoyenne) {
			OpiumFail(L_("Manque de place memoire pour le spectre moyenne (%d octets)"),n);
			return(0);
		}
		moyenne_refait = 1;
	} else moyenne_refait = 0;
	if(!MonitFftNouveau) {
		MonitFftNouveau = (float *)malloc(n);
		if(!MonitFftNouveau) {
			OpiumError(L_("Manque de place memoire pour le spectre a ajouter (%d octets)"),n);
			return(0);
		}
		nouveau_refait = 1;
	} else nouveau_refait = 0;
	MonitFftDemande = 1;
	MonitFftNbSpectres = 0.0;
	MonitFftEpsilon = 1.0 / (float)MonitFftNb;
	for(i=0; i<MonitFftNbFreq; i++) MonitFftCarreSomme[i] = MonitFftCarreMoyenne[i] = 0.0;
	for(i=0; i<MonitFftNbFreq; i++) MonitFftNouveau[i] = MonitFftMoyenne[i] = 1.0;

	MonitFftAb6[0] = 0.0;
	MonitFftAb6[1] = 1.0 / (VoieEvent[MonitFftVoie].horloge * MonitFftDim); /* kHz */
	if((MonitFftDonnees == TRMT_PRETRG) && (VoieTampon[MonitFftVoie].trmt[MonitFftDonnees].compactage > 0))
		MonitFftAb6[1] /= VoieTampon[MonitFftVoie].trmt[MonitFftDonnees].compactage;
	if(!gMonitFftAuVol) {
		gMonitFftAuVol = GraphCreate(400,300,3);
		GraphMode(gMonitFftAuVol,GRF_2AXES | GRF_GRID | GRF_LEGEND);
		GraphAxisTitle(gMonitFftAuVol,GRF_XAXIS,L_("Frequence (kHz)"));
		GraphAxisTitle(gMonitFftAuVol,GRF_YAXIS,L_("Bruit (nV/Rac(Hz))"));
		OpiumPosition(gMonitFftAuVol->cdr,50,400);
		x = GraphAdd(gMonitFftAuVol,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,MonitFftAb6,MonitFftNbFreq);
		GraphDataName(gMonitFftAuVol,x,"f(kHz)");
		nouveau_refait = moyenne_refait = 1;
	} else {
		GraphDataReplace(gMonitFftAuVol,0,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,MonitFftAb6,MonitFftNbFreq);
		if(moyenne_refait) GraphDataRemove(gMonitFftAuVol,1);
		if(nouveau_refait) GraphDataRemove(gMonitFftAuVol,2);
	}
	if(moyenne_refait) {
		y = GraphAdd(gMonitFftAuVol,GRF_FLOAT|GRF_YAXIS,MonitFftMoyenne,MonitFftNbFreq);
		GraphDataRGB(gMonitFftAuVol,y,GRF_RGB_GREEN);
		GraphDataName(gMonitFftAuVol,y,L_("Moyenne"));
	}
	if(nouveau_refait) {
		y = GraphAdd(gMonitFftAuVol,GRF_FLOAT|GRF_YAXIS,MonitFftNouveau,MonitFftNbFreq);
		GraphDataRGB(gMonitFftAuVol,y,GRF_RGB_YELLOW);
		GraphDataName(gMonitFftAuVol,y,L_("A ajouter"));
	}
	sprintf(titre,L_("Spectre moyenne sur %s"),VoieManip[MonitFftVoie].nom);
	OpiumTitle(gMonitFftAuVol->cdr,titre);
	printf("* Spectre au vol demande pour la voie %s\n",VoieManip[MonitFftVoie].nom);

#ifdef SIMPLIFIE
	if(MonitFftParm[0].log) GraphAxisLog(gMonitFftAuVol,GRF_XAXIS,1);
	else GraphAxisLog(gMonitFftAuVol,GRF_XAXIS,0);
	if(MonitFftParm[0].autom) {
		GraphAxisAutoRange(gMonitFftAuVol,GRF_XAXIS);
		if(MonitFftParm[0].log) GraphAxisFloatMin(gMonitFftAuVol,GRF_XAXIS,0.001);
		else GraphAxisFloatMin(gMonitFftAuVol,GRF_XAXIS,0.0);
	} else GraphAxisFloatRange(gMonitFftAuVol,GRF_XAXIS,MonitFftParm[0].lim.r.min,MonitFftParm[0].lim.r.max);

	if(MonitFftParm[1].log) GraphAxisLog(gMonitFftAuVol,GRF_YAXIS,1);
	else GraphAxisLog(gMonitFftAuVol,GRF_YAXIS,0);
	if(MonitFftParm[1].autom) {
		GraphAxisAutoRange(gMonitFftAuVol,GRF_YAXIS);
		//		if(MonitFftParm[1].log) GraphAxisFloatMin(gMonitFftAuVol,GRF_YAXIS,1.0);
		//		else GraphAxisFloatMin(gMonitFftAuVol,GRF_YAXIS,0.0);
	} else GraphAxisFloatRange(gMonitFftAuVol,GRF_YAXIS,MonitFftParm[1].lim.r.min,MonitFftParm[1].lim.r.max);
#else
	GraphAjusteParms(gMonitFftAuVol,GRF_XAXIS,&(MonitFftParm[0]));
	GraphAjusteParms(gMonitFftAuVol,GRF_YAXIS,&(MonitFftParm[1]));
#endif

	GraphUse(gMonitFftAuVol,MonitFftNbFreq);
	if(OpiumAlEcran(pMonitDivers)) PanelRefreshVars(pMonitDivers);

	return(0);
}
/* ========================================================================== */
static int MonitRaiesDefini(Menu menu, MenuItem item) {
	int bolo,cap,k,rc;
	Panel p; char titre[MAXPICS][12];

	PicsActif = 0; // preventivement
	bolo = BoloNum; cap = CapteurNum;
	if(OpiumExec(pMonitRaies->cdr) == PNL_CANCEL) return(0);
	if(Bolo[BoloNum].lu == DETEC_HS) {
		OpiumError(L_("Le detecteur %s n'est pas lu"),Bolo[BoloNum].nom);
		BoloNum = bolo; CapteurNum = cap;
		return(0);
	}
	if(CapteurNum >= Bolo[BoloNum].nbcapt) {
		OpiumError(L_("Capteur illegal pour le detecteur '%s'"),Bolo[BoloNum].nom);
		BoloNum = bolo; CapteurNum = cap;
		return(0);
	}
	if(PicsNb) {
		p = PanelCreate(3 * (PicsNb + 1)); PanelColumns(p,3); PanelMode(p,PNL_BYLINES);
		PanelItemSelect(p,PanelText(p,0,0,7),0);
		PanelItemSelect(p,PanelText(p,0,"min (ADU)",10),0);
		PanelItemSelect(p,PanelText(p,0,"max (ADU)",10),0);
		for(k=0; k<PicsNb; k++) {
			sprintf(&(titre[k][0]),L_("Pic %d"),k+1);
			PanelItemSelect(p,PanelText(p,0,&(titre[k][0]),10),0);
			PanelShort(p,0,&Pic[k].min);
			PanelShort(p,0,&Pic[k].max);
		}
		rc = OpiumExec(p->cdr);
		PanelDelete(p);
		if(rc == PNL_OK) {
			VoieNum = Bolo[BoloNum].captr[CapteurNum].voie;
			SettingsHistoVoie = VoieNum;
			SettingsAmplMin = SettingsAmplMax = 0;
			for(k=0; k<PicsNb; k++) {
				if(!k) { SettingsAmplMin = Pic[k].min; SettingsAmplMax = Pic[k].max; }
				else {
					if(Pic[k].min < SettingsAmplMin) SettingsAmplMin = Pic[k].min;
					if(Pic[k].max > SettingsAmplMax) SettingsAmplMax = Pic[k].max;
				}
			}
			HistoLimitsFloat(hdEvtAmpl->histo,SettingsAmplMin,SettingsAmplMax);
			printf("* Limites d'amplitude pour le suivi du gain: [%g .. %g]\n",SettingsAmplMin,SettingsAmplMax);
			PicsActif = 1; TrmtHampl = 1;
		}
	} else PicsActif = 0;
	if(OpiumAlEcran(pMonitDivers)) PanelRefreshVars(pMonitDivers);

	return(0);
}
/* ========================================================================== */
static int MonitRaiesRaz(Menu menu, MenuItem item) { HistoDataRaz(hdEvtAmpl); return(0); }
/* ==========================================================================
int MonitFftRaz() {
	int i;

	if(MonitFftCarreSomme && MonitFftCarreMoyenne)
		for(i=0; i<MonitFftNbFreq; i++) MonitFftCarreSomme[i] = MonitFftCarreMoyenne[i] = 0.0;
	if(MonitFftNouveau && MonitFftMoyenne)
		for(i=0; i<MonitFftNbFreq; i++) MonitFftNouveau[i] = MonitFftMoyenne[i] = 1.0;
	MonitFftNbSpectres = 0.0;
	return(0);
}
   ========================================================================== */
#ifdef MONIT_PAR_VOIE
int MonitVoieImprime() {
	int voie,bolo,vm,cap;
	int i,j,nb,dernier,total;
	int min,max,moy;
	TypeDonnee *val;

	if(OpiumExec(pMonitVoiePrint->cdr) == PNL_CANCEL) return(0);
	bolo = BoloNum; vm = ModlNum;
	if(Bolo[BoloNum].lu == DETEC_HS) {
		OpiumError("Le detecteur %s n'est pas lu",Bolo[BoloNum].nom);
		BoloNum = bolo; ModlNum = vm;
		return(0);
	}
	voie = 0;
	for(cap=0; cap<Bolo[BoloNum].nbcapt; cap++) {
		voie = Bolo[BoloNum].captr[cap].voie;
		if(ModlNum == VoieManip[voie].type) break;
	}
	if(cap == Bolo[BoloNum].nbcapt) {
		OpiumError("Pas de voie '%s' pour le detecteur '%s'",ModeleVoie[ModlNum].nom,Bolo[BoloNum].nom);
		BoloNum = bolo; ModlNum = vm;
		return(0);
	}
	VoieNum = voie;

	if(!VoieTampon[voie].brutes->t.sval) {
		OpiumError("Le tampon de la voie %s n'est pas defini",VoieManip[voie].nom);
		return(0);
	}
	TermHold(tMonit);
	dernier = (int)(VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien + VoieTampon[voie].brutes->nb);
	if(MonitVoieVal0 < VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien)
		MonitVoieVal0 = (int)(VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien);
	else if(MonitVoieVal0 >= dernier)
		MonitVoieVal0 = dernier - 1;
	if(MonitVoieValN > dernier )
		MonitVoieValN = dernier;
	total = MonitVoieValN - MonitVoieVal0;
	if(total <= 0) return(0);

	TermPrint(tMonit,"%s a partir de %d (%d/%d points)\n",
		VoieManip[voie].nom,MonitVoieVal0,total,VoieTampon[voie].brutes->nb);
	i= MonitVoieVal0; j = i % VoieTampon[voie].brutes->max;
	val = VoieTampon[voie].brutes->t.sval + j;
	min = 999999; max = 0; moy = 0;
	nb = 0;
	while(nb < total) {
		if(!(nb % 10)) TermPrint(tMonit,"%11d: ",i);
		if(min > *val) min = *val;
		if(max < *val) max = *val;
		moy += *val;
		TermPrint(tMonit," %05X",*val++); i++;
		if(++j == VoieTampon[voie].brutes->max) val = VoieTampon[voie].brutes->t.sval;
		++nb;
		if(!(nb % 10)) TermPrint(tMonit,"\n");
		else if(!(nb % 5)) TermPrint(tMonit,"  ");
	};
	if(nb % 10) TermPrint(tMonit,"\n");
	TermPrint(tMonit,"min: %05X, max: %05X, moy: %05X\n",min,max,moy/total);
	TermRelease(tMonit);
	if(!(tMonit->cdr->displayed)) OpiumDisplay(tMonit->cdr);

	return(0);
}
/* ========================================================================== */
static char MonitVoieCree(int voie) {
	int bolo, y;
	char titre[80];

	
	if(!VoieTampon[voie].brutes->t.sval)
		OpiumError("Le tampon de la voie %s n'est pas defini",VoieManip[voie].nom);
	else {
		gDonnee[voie] = GraphCreate(MonitLarg,MonitHaut,2);
		if(!gDonnee[voie]) return(0);
		sprintf(titre,"%s",VoieManip[voie].nom);
		OpiumTitle(gDonnee[voie]->cdr,titre);
		/* abcisse en millisecondes */
		GraphAdd(gDonnee[voie],GRF_FLOAT|GRF_INDEX|GRF_XAXIS,VoieTampon[voie].trmt[TRMT_AU_VOL].ab6,VoieTampon[voie].brutes->max);
		y = GraphAdd(gDonnee[voie],GRF_SHORT|GRF_YAXIS,
			VoieTampon[voie].brutes->t.sval,VoieTampon[voie].brutes->max);
		GraphDataRGB(gDonnee[voie],y,GRF_RGB_GREEN);
		bolo = VoieManip[voie].det;
		OpiumPosition(gDonnee[voie]->cdr,(bolo * MonitLarg) + 10,
			(voie * (MonitHaut + 40)) + 200);
		GraphParmsSave(gDonnee[voie]);
		return(1);
	}
	return(0);
}
/* ========================================================================== */
static void MonitVoieReset(int voie) {
	int nbpts;
	hexa zoom;
	// int larg;

	nbpts = MonitDuree * Echantillonnage;
	if(nbpts > VoieTampon[voie].brutes->max) {
		nbpts = VoieTampon[voie].brutes->max;
		MonitDuree = (float)nbpts / Echantillonnage;
	}
	zoom = ((VoieTampon[voie].brutes->max - 1) / nbpts) + 1;
	// larg = MonitLarg * zoom;
	// GraphResize(gDonnee[voie],larg,MonitHaut);
	GraphZoom(gDonnee[voie],zoom,1);
	GraphAxisIntRange(gDonnee[voie],GRF_YAXIS,VoieManip[voie].min,VoieManip[voie].max);
	GraphUse(gDonnee[voie],VoieTampon[voie].brutes->nb);
	GraphRescroll(gDonnee[voie],Modulo(VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien,VoieTampon[voie].brutes->max));
	GraphAxisReset(gDonnee[voie],GRF_XAXIS);
	GraphAxisReset(gDonnee[voie],GRF_YAXIS);
	OpiumDisplay(gDonnee[voie]->cdr);
	VoieManip[voie].affiche = 1;
}
/* ========================================================================== */
int MonitVoieAffiche() {
	int i,j; int voie;

	if(!MonitChoisiBoloVoie(&voie)) return(0);
	PanelErase(pMonitLimites1Voie);
	OpiumTitle(pMonitLimites1Voie->cdr,VoieManip[voie].nom);
	i = PanelInt(pMonitLimites1Voie,"Valeur minimum (ADU)",&(VoieManip[voie].min)); /* samba.c */
	j = PanelInt(pMonitLimites1Voie,"Valeur maximum (ADU)",&(VoieManip[voie].max)); /* samba.c */
	PanelFloat(pMonitLimites1Voie,"Duree affichee (ms)",&MonitDuree);
	PanelInt  (pMonitLimites1Voie,"Hauteur graphique",&MonitHaut);
	PanelInt  (pMonitLimites1Voie,"Largeur graphique",&MonitLarg);
	PanelItemILimits(pMonitLimites1Voie,i,-VoieManip[voie].zero,VoieManip[voie].zero-1);
	PanelItemILimits(pMonitLimites1Voie,j,-VoieManip[voie].zero,VoieManip[voie].zero-1);

	if(OpiumExec(pMonitLimites1Voie->cdr) == PNL_CANCEL) {
		if(gDonnee[voie]) {
			if(OpiumDisplayed(gDonnee[voie]->cdr)) OpiumClear(gDonnee[voie]->cdr);
		}
		VoieManip[voie].affiche = 0;
	} else {
		if(!gDonnee[voie]) {
			if(!MonitVoieCree(voie)) return(0);
		} else {
			if(OpiumDisplayed(gDonnee[voie]->cdr)) OpiumClear(gDonnee[voie]->cdr);
		}
		MonitVoieReset(voie);
	}
	return(0);
}
/* ========================================================================== */
int MonitVoieRafraichi() {
	int voie;

	for(voie=0; voie<VoiesNb; voie++) {
		if(!gDonnee[voie]) continue;
		if(OpiumDisplayed(gDonnee[voie]->cdr)) OpiumClear(gDonnee[voie]->cdr);
		MonitVoieReset(voie);
	}
	return(0);
}
/* ========================================================================== */
int MonitVoieFiltrage() {
	int voie;
	char titre[80];
	int i,j,y;
	int nb_brutes,nb_filtrees;
	int64 prems_brute;

	if(!MonitChoisiBoloVoie(&voie)) return(0);
	if(!VoieTampon[voie].traitees->t.rval) {
		OpiumError("Pas de resultat du filtrage pour la voie %s",
			VoieManip[voie].nom);
		return(0);
	}
	PanelErase(pMonitLimites1Voie);
	OpiumTitle(pMonitLimites1Voie->cdr,VoieManip[voie].nom);
	i = PanelInt(pMonitLimites1Voie,"Valeur minimum (ADU)",&(VoieManip[voie].min));
	j = PanelInt(pMonitLimites1Voie,"Valeur maximum (ADU)",&(VoieManip[voie].max));
	PanelInt(pMonitLimites1Voie,"Hauteur graphique",&MonitHaut);
	PanelInt(pMonitLimites1Voie,"Largeur graphique",&MonitLarg);
	PanelItemILimits(pMonitLimites1Voie,i,-VoieManip[voie].zero,VoieManip[voie].zero-1);
	PanelItemILimits(pMonitLimites1Voie,j,-VoieManip[voie].zero,VoieManip[voie].zero-1);

	if(OpiumExec(pMonitLimites1Voie->cdr) == PNL_CANCEL) {
		if(gFiltree[voie]) {
			if(OpiumDisplayed(gFiltree[voie]->cdr)) OpiumClear(gFiltree[voie]->cdr);
		}
	} else {
		if(!gFiltree[voie]) {
			gFiltree[voie] = GraphCreate(MonitLarg,MonitHaut,4);
			if(!gFiltree[voie]) return(0);
			sprintf(titre,"%s filtree",VoieManip[voie].nom);
			OpiumTitle(gFiltree[voie]->cdr,titre);
			/* abcisse en millisecondes */
			GraphAdd(gFiltree[voie],GRF_FLOAT|GRF_INDEX|GRF_XAXIS,VoieTampon[voie].trmt[TRMT_AU_VOL].ab6,VoieTampon[voie].brutes->max);
			y = GraphAdd(gFiltree[voie],GRF_SHORT|GRF_YAXIS,
				VoieTampon[voie].brutes->t.sval,VoieTampon[voie].brutes->max);
			GraphDataRGB(gFiltree[voie],y,GRF_RGB_GREEN);
			GraphAdd(gFiltree[voie],GRF_FLOAT|GRF_INDEX|GRF_XAXIS,FiltreeAb6,VoieTampon[voie].traitees->max);
			y = GraphAdd(gFiltree[voie],GRF_SHORT|GRF_YAXIS,
				VoieTampon[voie].traitees->t.rval,VoieTampon[voie].traitees->max);
			GraphDataRGB(gFiltree[voie],y,GRF_RGB_MAGENTA);
		} else {
			if(OpiumDisplayed(gFiltree[voie]->cdr))
				OpiumClear(gFiltree[voie]->cdr);
		}
		GraphAxisIntRange(gFiltree[voie],GRF_YAXIS,VoieManip[voie].min,VoieManip[voie].max);
		nb_filtrees = VoieTampon[voie].traitees->nb;
		nb_brutes = nb_filtrees * VoieTampon[voie].trmt[TRMT_PRETRG].compactage;
		prems_brute = VoieTampon[voie].lus - (int64)nb_brutes;
		/* sauf que signal[].prems correspond a LectStampsLus au dernier appel a TrmtSurBuffer,
		   il faut donc affiner la synchronisation */
		prems_brute = ((prems_brute / (int64)nb_brutes) * (int64)nb_brutes)
			 + (int64)(VoieTampon[voie].traitees->prem * VoieTampon[voie].trmt[TRMT_PRETRG].compactage);
		GraphDataUse(gFiltree[voie],0,nb_brutes);
		GraphDataUse(gFiltree[voie],1,nb_brutes);
		GraphDataRescroll(gFiltree[voie],0,Modulo(prems_brute,VoieTampon[voie].brutes->max));
		GraphDataRescroll(gFiltree[voie],1,Modulo(prems_brute,VoieTampon[voie].brutes->max));

		FiltreeAb6[1] = VoieEvent[voie].horloge * VoieTampon[voie].trmt[TRMT_PRETRG].compactage;
		FiltreeAb6[0] = ((float)prems_brute * VoieEvent[voie].horloge)
			 - (VoieTampon[voie].traitees->prem * FiltreeAb6[1]);
		GraphDataUse(gFiltree[voie],2,nb_filtrees);
		GraphDataUse(gFiltree[voie],3,nb_filtrees);
		GraphDataRescroll(gFiltree[voie],2,VoieTampon[voie].traitees->prem);
		GraphDataRescroll(gFiltree[voie],3,VoieTampon[voie].traitees->prem);
		GraphAxisReset(gFiltree[voie],GRF_XAXIS);
		GraphAxisReset(gFiltree[voie],GRF_YAXIS);
//		GraphParmsSave(gFiltree[voie]);
		OpiumDisplay(gFiltree[voie]->cdr);
/*		{	int i,j;
			j = VoieTampon[voie].traitees->prem;
			printf("Tampon des valeurs filtrees:");
			for(i=0; i<VoieTampon[voie].traitees->nb; i++) {
				if(!(i%10)) printf("\n%4d:",i+(int)prems_brute);
				printf("%6d",VoieTampon[voie].traitees->t.rval[j]);
				j++;
				if(j >= VoieTampon[voie].traitees->max) j = 0;
			}
			printf("\n");
		} */
	}
	return(0);
}
/* ========================================================================== */
int MonitVoieEfface() {
	int voie;

	for(voie=0; voie<VoiesNb; voie++) {
		if(!gDonnee[voie]) continue;
		if(OpiumDisplayed(gDonnee[voie]->cdr)) OpiumClear(gDonnee[voie]->cdr);
		VoieManip[voie].affiche = 0;
	}
	return(0);
}
#endif /* MONIT_PAR_VOIE */
/* ========================================================================== */
int MonitEvtMoyenVoie(Menu menu, MenuItem item) {
	int voie; int y;

	if(!MonitChoisiBoloVoie(&voie)) return(0);
	if(VoieTampon[voie].moyen.nb <= 0) {
		OpiumError(L_("Pas d'evenement calcule pour la voie %s"),VoieManip[voie].nom);
		return(0);
	}
	MonitEvtAb6[voie][0] = 0.0;
	if(OpiumDisplayed(gEvtMoyen->cdr)) OpiumClear(gEvtMoyen->cdr); /* pour que le titre change ET que le nom disparaisse dans le menu Fenetres */
	GraphErase(gEvtMoyen);
	sprintf(MonitTitre,"Evenement moyen, voie %s",VoieManip[voie].nom);
	OpiumTitle(gEvtMoyen->cdr,MonitTitre);
	GraphAdd(gEvtMoyen,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitEvtAb6[voie][0]),VoieTampon[voie].somme.taille);
	y = GraphAdd(gEvtMoyen,GRF_FLOAT|GRF_YAXIS,VoieTampon[voie].somme.val,VoieTampon[voie].somme.taille);
	GraphDataRGB(gEvtMoyen,y,GRF_RGB_GREEN);
	GraphUse(gEvtMoyen,VoieTampon[voie].somme.taille);
	if(MonitChgtUnite) GraphAxisChgtUnite(gEvtMoyen,GRF_YAXIS,MonitUnitesADU);
	GraphAxisReset(gEvtMoyen,GRF_XAXIS);
	GraphAxisReset(gEvtMoyen,GRF_YAXIS);
	OpiumDisplay(gEvtMoyen->cdr);
	
	return(0);
}
/* ========================================================================== */
int MonitEvtMoyenBolo(Menu menu, MenuItem item) {
	int bolo,cap,voie; int y;
	float tdeb,tfin,tmin,tmax;
	char prems;
	OpiumColorState s; int k;

	bolo = BoloNum;
	if(OpiumReadList("Pour le detecteur",BoloNom,&bolo,DETEC_NOM) == PNL_CANCEL) return(0);
	BoloNum = bolo;
	if(OpiumDisplayed(gEvtMoyen->cdr)) OpiumClear(gEvtMoyen->cdr); /* pour que le titre change ET que le nom disparaisse dans le menu Fenetres */
	GraphErase(gEvtMoyen);
	OpiumColorInit(&s);
	sprintf(MonitTitre,"Evenement moyen, bolo %s",Bolo[BoloNum].nom);
	OpiumTitle(gEvtMoyen->cdr,MonitTitre);
	prems = 1;
	tmin = tmax = 0.0; /* gnu */
	for(cap=0; cap<Bolo[BoloNum].nbcapt; cap++) {
		voie = Bolo[BoloNum].captr[cap].voie;
		if(VoieTampon[voie].moyen.nb <= 0) continue;
		tdeb = VoieTampon[voie].moyen.decal;
		tfin =  tdeb +  (VoieTampon[voie].somme.taille * VoieEvent[voie].horloge);
		if(prems) { tmin = tdeb; tmax = tfin; prems = 0; }
		else { if(tmin > tdeb) tmin = tdeb; if(tmax < tfin) tmax = tfin; }
		MonitEvtAb6[voie][0] = tdeb;
		GraphAdd(gEvtMoyen,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitEvtAb6[voie][0]),VoieTampon[voie].somme.taille);
		y = GraphAdd(gEvtMoyen,GRF_FLOAT|GRF_YAXIS,VoieTampon[voie].somme.val,VoieTampon[voie].somme.taille);
		k = OpiumColorNext(&s);
		GraphDataRGB(gEvtMoyen,y,OpiumColorRGB(k));
	}
//	GraphUse(gEvtMoyen,VoieTampon[voie].somme.taille);
//	GraphAxisReset(gEvtMoyen,GRF_XAXIS);
	GraphAxisFloatRange(gEvtMoyen,GRF_XAXIS,tmin,tmax);
	if(MonitChgtUnite) GraphAxisChgtUnite(gEvtMoyen,GRF_YAXIS,MonitUnitesADU);
	GraphAxisReset(gEvtMoyen,GRF_YAXIS);
	OpiumDisplay(gEvtMoyen->cdr);

	return(0);
}
/* ========================================================================== */
void MonitEvtValeurs(TypeVoieArchivee *archivee) {
	int voietrig; int64 evtmax;
	int secs,msecs,usecs; // a ajouter dans MonitEvtAffiche si re-integration

	voietrig = archivee->num;
	evtmax = archivee->debut + (int64)VoieEvent[voietrig].avant_evt;
	strcpy(BoloTrigge,Bolo[VoieManip[voietrig].det].nom);
	strcpy(VoieTriggee,VoieManip[voietrig].prenom);
	SambaTempsEchantillon(evtmax,0 /* -(int)MonitT0 */,0,&secs,&usecs);
	msecs = usecs / 1000; usecs = usecs - (msecs * 1000);
	snprintf(TempsTrigger,15,"%d%03d,%03d",secs,msecs,usecs);
	MonitEvtAmpl = archivee->val[MONIT_AMPL];
	MonitEvtMontee = archivee->val[MONIT_MONTEE];
}
/* ========================================================================== */
static float MonitTraiteeOf7(int voie, TypeTamponDonnees *tampon, int lngr, int64 debut, int nb) {
	/* Calcule l'offset, ou ligne de base, d'un evenement demarrant exactement a <debut> */
	int i,k; double base;
	TypeDonnee *ptr_int16,sval;
	TypeSignal *ptr_float,rval;
	
	k = Modulo(debut,lngr);
	base = 0.0;
	if(tampon->type == TAMPON_INT16) {
		ptr_int16 = tampon->t.sval + k;
		for(i=0; i<nb; i++) {
			sval = *ptr_int16++; base += (double)sval;
			if(++k >= lngr) { k = 0; ptr_int16 = tampon->t.sval; }
		}
	} else {
		ptr_float = tampon->t.rval + k;
		for(i=0; i<nb; i++) {
			rval = *ptr_float++; base += (double)rval;
			if(++k >= lngr) { k = 0; ptr_float = tampon->t.rval; }
		}
	}
	base = (base / (double)nb);
	return((float)base);
}
/* ========================================================================== */
int MonitEvtAffiche(TypeMonitCatg *categ_demandee, int lequel, void *qui, int affiche_message) {
	int voie,vt,vm,nb,voietrig; int evtprems,evtindex,evtdim,evtnum,pos_evt,pos_trmt;
	int64 evtstart,evtend;
	int64 trmtstart,trmtend,ldbstart,ldbend; int trmtdim,ldbdim; int64 reduction;
	int64 debut_traitees,fin_traitees,t0_evt,t0_trmt,decalage;
	char avec_traite,planche_run,planche_classes;
	TypeMonitCatg *categ; TypeConfigDefinition *config;
	TypeEvt *evt; TypeVoieArchivee *archivee;
	Graph g; int x,y,rc; char ajoute;
#ifdef CARRE_A_VERIFIER
	int64 evtmax;
#endif

	if(!EvtASauver) { if(affiche_message) OpiumError("Pas d'evenement en memoire"); return(0); }
	for(evtprems = 0; evtprems < EvtASauver; evtprems++) {
		evt = &(Evt[evtprems]); vt = evt->voie_evt; voie = evt->voie[vt].num;
		evtstart = evt->voie[vt].debut;
		/* evtdim = evt->dim / VoieTampon[voie].trmt[TRMT_AU_VOL].compactage;
			if((evtstart + evtdim) >= VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien) break; */
		if(evtstart >= VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien) break;
	}
	if(evtprems >= EvtASauver) { if(affiche_message) OpiumError("Plus d'evenement en memoire"); return(0); }
	planche_run = (OpiumDisplayed((gEvtRunCtrl->cdr)->planche) && MonitEvtAvecRun);
	planche_classes = (categ_demandee != 0);
	if(!lequel) {
		categ = categ_demandee;
		if(!planche_classes) {
			evtnum = LectCntl.MonitEvtNum;
			if((evtnum == 0) || (evtnum == MonitEvtAlEcran)) return(0);
		} else evtnum = categ->affichable;
		// ImprimeAppelant("%s "); printf("demande evt courant #%d\n",evtnum);
		evtindex = evtnum - Evt[0].num;
	} else {
		categ = planche_classes? categ_demandee: &TousLesEvts;
		// ImprimeAppelant("%s "); printf("demande evt[%d] pour '%s'\n",lequel,categ->nom);
		evtindex = lequel;
	}
	if(planche_classes) g = categ->gEvt;
	else if(qui == (void *)mMonitEvts) g = gEvtSolo;
	else if(planche_run) g = gEvtRunCtrl;
	else if(OpiumDisplayed(bSuiviEvts)) return(0);
	else g = gEvtSolo;

	if((evtindex < 0) || (evtindex >= EvtASauver)) {
		if(affiche_message) OpiumError("Evt[%d] illegal (%d en memoire)",evtindex,EvtASauver);
		return(0);
	}
	evt = &(Evt[evtindex]); evtnum = evt->num;
	if((evtindex < evtprems) || (evtnum > Evt[EvtASauver - 1].num)) {
		if(affiche_message) OpiumError("Evenement #%d illegal ou archive (accessibles: [%d .. %d])",evtnum,evt->num,Evt[EvtASauver - 1].num);
		if(evtnum < evt->num) evtnum = evt->num;
		else if(evtnum > Evt[EvtASauver - 1].num) evtnum = Evt[EvtASauver - 1].num;
		if(categ_demandee == 0) LectCntl.MonitEvtNum = evtnum;
		else categ->affichable = evtnum;
		return(0);
	}
	vt = evt->voie_evt; archivee = &(evt->voie[vt]); voietrig = archivee->num;
	config = &(VoieManip[voietrig].def);
	// printf("(%s) Affiche evt[%d] #%d\n",__func__,evtindex,evt->num);

	ajoute = 0; avec_traite = 1;
//	if((qui == (void *)mMonit) || (qui == (void *)mMonitEvts)) g = gEvtSolo;
//	else if((g = (Graph)qui) == 0) { if(planche_run) g = gEvtRunCtrl; else g = gEvtSolo; }
//	else { avec_traite = 0; planche_classes = 1; }

	if(g == gEvtSolo) {
	#define CHANGE_TITRE
	#ifdef CHANGE_TITRE
		if(OpiumDisplayed(gEvtSolo->cdr)) OpiumClear(gEvtSolo->cdr); /* pour que le titre change ET que le nom disparaisse dans le menu Fenetres */
	#endif
		nb = 2 * evt->nbvoies;
	#ifdef MONIT_EVT_TRAITE
		if(avec_traite && VoieTampon[evt->voie[vt].num].traitees->t.rval) nb *= 2;
	#endif
		if(nb > GraphDim(gEvtSolo)) {
			GraphDelete(gEvtSolo);
			g = gEvtSolo = GraphCreate(MonitLarg,MonitHaut,nb);
			ajoute = 1;
		}
		vt = 0;
	#ifdef CHANGE_TITRE
		sprintf(MonitTitre,"Evenement #%d: voie %s",evtnum,VoieManip[archivee->num].nom);
		OpiumTitle(gEvtSolo->cdr,MonitTitre);
	#endif
		if(MonitChgtUnite) GraphAxisChgtUnite(gEvtSolo,GRF_YAXIS,MonitUnitesADU);
	}
	MonitEvtValeurs(archivee);

	x = 0; y = 1;
	while(vt < evt->nbvoies) {
		archivee = &(evt->voie[vt]); voie = archivee->num; vm = VoieManip[voie].type;
		evtstart = archivee->debut;
		evtdim = archivee->dim;
		evtend = evtstart + evtdim;
	#ifdef CARRE_A_VERIFIER
		evtmax = evtstart + (int64)VoieEvent[voie].avant_evt;
	#endif
		pos_evt = archivee->adresse;
		if(archivee->stream) {
			if(evtend < VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien) {
				if(affiche_message) OpiumError("L'evenement '%s'#%d n'est plus dans le tampon",VoieManip[voie].nom,evtnum);
				continue;
			}
			trmtstart = evtstart; trmtend = evtend;
			if(VerifMonitEvts == 1) printf("(%s) Evt#%03d: [%lld .. %lld[ mesure\n",__func__,evt->num,evtstart,evtend);
			if((decalage = evtstart - VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien) < 0) {
				evtstart = VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien;
				evtdim = (int)(evtend - evtstart);
				pos_evt -= decalage; // decalage est negatif, ici
			}
			if(evtend > VoieTampon[voie].lus) evtdim = (int)(VoieTampon[voie].lus - evtstart);
			if(evtdim <= 0) {
				if(affiche_message) OpiumWarn("Voie %s @%lld + %d: dimension disponible = %d",VoieManip[voie].nom,archivee->debut,archivee->dim,evtdim);
				continue;
			}
		}
		if(VerifMonitEvts == 1) printf("(%s)          [%lld .. %lld[ stocke\n",__func__,evtstart,evtend);

	#ifdef CARRE_A_VERIFIER
		/* ab6 recalcule seulement si signal voie affiche dans une fenetre user. Sinon voir avec MonitT0*1000.0 */
		//	MomentEvt[2] = MomentEvt[3] = (((float)evtmax * VoieEvent[voie].horloge)) - VoieTampon[voie].trmt[TRMT_PRETRG].ab6[0];
		MomentEvt[2] = MomentEvt[3] = (float)(evtmax  - (MonitP0 / VoieTampon[voie].trmt[TRMT_AU_VOL].compactage)) * VoieEvent[voie].horloge;
		MomentEvt[0] = MomentEvt[1] = MomentEvt[4] = MomentEvt[2] - archivee->val[MONIT_MONTEE];
		AmplitudeEvt[0] = AmplitudeEvt[3] = AmplitudeEvt[4] = (short)archivee->val[MONIT_BASE];
		AmplitudeEvt[1] = AmplitudeEvt[2] = AmplitudeEvt[0] + (short)archivee->val[MONIT_AMPL];
		if(VerifMonitEvts == 1) printf("(%s)          { (%g, %d), (%g, %d), (%g, %d), (%g, %d), (%g, %d) }\n",__func__,
			MomentEvt[0],AmplitudeEvt[0],MomentEvt[1],AmplitudeEvt[1],MomentEvt[2],AmplitudeEvt[2],
			MomentEvt[3],AmplitudeEvt[3],MomentEvt[4],AmplitudeEvt[4]);
	#endif

		//	printf("(%s) mMonit @%08X, mMonitEvts @%08X, appelant @%08X\n",__func__,(hexa)mMonit,(hexa)mMonitEvts,(hexa)qui);
		//	printf("(%s) gEvtRunCtrl @%08X: %s.\n",__func__,(hexa)gEvtRunCtrl,(OpiumDisplayed((gEvtRunCtrl->cdr)->planche))? "affiche": "absent");
		//	printf("(%s) Affiche graph @%08X (%s)\n",__func__,(hexa)g,(OpiumDisplayed(g->cdr))? "actif": "masque");
		/* pos_evt ~ archivee->adresse, et t0_evt sert a fixer une date commune avec trmt */
		// t0_evt = (evtstart / (int64)VoieTampon[voie].brutes->max) * (int64)VoieTampon[voie].brutes->max;
		// pos_evt = (int)(evtstart - t0_evt);
		t0_evt = evtstart - (int64)pos_evt;
		if(VerifMonitEvts == 1) printf("(%s)          au vol[0]= %lld, debut evt +%d\n",__func__,t0_evt,pos_evt);
		if(categ) MonitEvtAb6[voie][0] = 0.0;
		else MonitEvtAb6[voie][0] = (float)(((double)t0_evt * (double)VoieTampon[voie].trmt[TRMT_AU_VOL].ab6[1]) - (double)(MonitT0 * 1000.0));

		if(SettingsDeconvPossible && categ && (categ->deconv) && VoieManip[voie].duree.deconv.calcule && evt->deconvolue) {
			evtdim = VoieEvent[voie].lngr; pos_evt = 0; avec_traite = 0;
			GraphDataReplace(g,x,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitEvtAb6[voie][0]),evtdim);
			rc = GraphDataReplace(g,y,GRF_FLOAT|GRF_YAXIS,evt->deconvolue,evtdim);
			if(rc < 0) printf("! (%s) le pointage sur l'evenement deconvolue a echoue\n",__func__);
			GraphDataRGB(g,y,GRF_RGB_TURQUOISE);
			GraphMode(g,GRF_2AXES | (FondPlots? GRF_QUALITE: GRF_GRID)); //  | GRF_LEGEND
			/* {	int i;
				printf("(%s) %s deconvoluee, %d valeur%s",__func__,VoieManip[voie].nom,Accord1s(evtdim));
				for(i=0; i<250; i++) {
					if(!(i % 10)) printf("\n|     %5d:",i);
					printf(" %8.1f",evt->deconvolue[i]);
				}
				printf("\n");
			} */
			// printf("(%s) %d donnee%s remplacee%s de la voie %s pour l'evenement %d\n",__func__,Accord2s(VoieEvent[voie].lngr),VoieManip[voie].nom,evtindex);
		} else if(ajoute) {
			x = GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitEvtAb6[voie][0]),VoieTampon[voie].brutes->max);
			y = GraphAdd(g,GRF_SHORT|GRF_YAXIS,VoieTampon[voie].brutes->t.sval,VoieTampon[voie].brutes->max);
			GraphDataName(g,x,"t(ms)"); GraphDataRGB(gEvtSolo,y,GRF_RGB_GREEN);
			GraphDataName(g,y,"ADU");
			rc = 0;
		} else {
			GraphDataReplace(g,x,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitEvtAb6[voie][0]),VoieTampon[voie].brutes->max);
			rc = GraphDataReplace(g,y,GRF_SHORT|GRF_YAXIS,VoieTampon[voie].brutes->t.sval,VoieTampon[voie].brutes->max);
			if(rc < 0) printf("! (%s) le pointage sur l'evenement brut a echoue\n",__func__);
			GraphDataRGB(g,y,GRF_RGB_GREEN);
		}
		// char titre[32];
		// if(MonitCategFocus == 0) sprintf(titre,"Evt #%d: %s",evt->num,evt->nom_catg); else sprintf(titre,"Evt #%d",evt->num);
		// GraphAxisTitle(g,GRF_YAXIS,titre);
		GraphAxisAutoRange(g,GRF_XAXIS);
		if(!MonitModeAuto) GraphAxisIntRange(g,GRF_YAXIS,MonitModeYmin,MonitModeYmax);
		if(rc >= 0) {
			float base_traitee,base_brute;
			// GraphDataIntOffset(g,1,-(int)base);
			GraphDataUse(g,x,evtdim);
			GraphDataUse(g,y,evtdim);
			GraphDataRescroll(g,x,pos_evt);
			GraphDataRescroll(g,y,pos_evt);
			if(VerifMonitEvts == 1) printf("(%s)          Ab6[%d] au vol = %g + %d x %g, soit [%g .. %g[\n",__func__,
				voie,MonitEvtAb6[voie][0],pos_evt,MonitEvtAb6[voie][1],
				MonitEvtAb6[voie][0]+((float)pos_evt*MonitEvtAb6[voie][1]),
				MonitEvtAb6[voie][0]+((float)(pos_evt+evtdim)*MonitEvtAb6[voie][1]));
		#ifdef MONIT_EVT_TRAITE
			if(avec_traite && VoieTampon[voie].traitees->t.rval) {
				reduction = (int64)VoieTampon[voie].trmt[TRMT_PRETRG].compactage;
				/* unite: point du tampon au vol */
				fin_traitees = ((DernierPointTraite - 1) / (int64)VoieTampon[voie].trmt[TRMT_AU_VOL].compactage) + 1;
				debut_traitees = fin_traitees - (VoieTampon[voie].traitees->nb * reduction);
				ldbstart = trmtstart + VoieEvent[voie].base_dep; ldbend = ldbstart + VoieEvent[voie].base_lngr;
				if(ldbstart < debut_traitees) ldbstart = debut_traitees;
				if(ldbend < debut_traitees) ldbend = debut_traitees; else if(ldbend > fin_traitees) ldbend = fin_traitees;
				if(trmtstart < debut_traitees) trmtstart = debut_traitees;
				if(trmtend > fin_traitees) trmtend = fin_traitees;
				/* unite: point du tampon pre-trigger */
				trmtstart /= reduction; trmtend /= reduction; trmtdim = (int)(trmtend - trmtstart);
				ldbstart /= reduction; trmtend /= reduction; ldbdim = (int)(ldbend - ldbstart);
				if(VerifMonitEvts == 1) printf("(%s)          [%lld .. %lld[ filtre\n",__func__,trmtstart,trmtend);
				if(trmtdim > 0) {
					t0_trmt = (trmtstart / (int64)VoieTampon[voie].traitees->max) * (int64)VoieTampon[voie].traitees->max;
					pos_trmt = (int)(trmtstart - t0_trmt);
					if(VerifMonitEvts == 1) printf("(%s)          traite[0]= %lld, debut evt +%d\n",__func__,t0_trmt,pos_trmt);
					MonitTrmtAb6[voie][0] = (float)(((double)t0_trmt * (double)VoieTampon[voie].trmt[TRMT_PRETRG].ab6[1]) - (double)(MonitT0 * 1000.0));
					// faux: MonitTrmtAb6[voie][0] = MonitEvtAb6[voie][0] + (float)((double)(t0_trmt - t0_evt) * (double)VoieTampon[voie].trmt[TRMT_PRETRG].ab6[1]);
					if(ajoute) {
						x = GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitTrmtAb6[voie][0]),VoieTampon[voie].traitees->max);
						y = GraphAdd(g,GRF_FLOAT|GRF_YAXIS,VoieTampon[voie].traitees->t.rval,VoieTampon[voie].traitees->max);
					} else {
						x += 2; y += 2;
						GraphDataReplace(g,x,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitTrmtAb6[voie][0]),VoieTampon[voie].traitees->max);
						rc = GraphDataReplace(g,y,GRF_FLOAT|GRF_YAXIS,VoieTampon[voie].traitees->t.rval,VoieTampon[voie].traitees->max);
					}
					if(ldbdim) {
						base_traitee = MonitTraiteeOf7(voie,VoieTampon[voie].traitees,VoieTampon[voie].traitees->max,ldbstart,ldbdim);
						if(archivee->ref == TRMT_AU_VOL) GraphDataFloatOffset(g,y,archivee->val[MONIT_BASE] - base_traitee);
						else {
							ldbstart = evtstart + VoieEvent[voie].base_dep; ldbend = ldbstart + VoieEvent[voie].base_lngr;
							ldbdim = (int)(ldbend - ldbstart);
							base_brute = MonitTraiteeOf7(voie,VoieTampon[voie].brutes,VoieTampon[voie].brutes->max,ldbstart,ldbdim);
							GraphDataFloatOffset(g,y,base_brute - base_traitee);
						}
					}
					if(rc < 0) printf("! (%s) le pointage sur l'evenement traite a echoue\n",__func__);
					else {
						GraphDataUse(g,x,trmtdim);
						GraphDataUse(g,y,trmtdim);
						GraphDataRescroll(g,x,pos_trmt);
						GraphDataRescroll(g,y,pos_trmt);
						if(VerifMonitEvts == 1) printf("(%s)          Ab6[%d] tampon = %g + %d x %g, soit [%g .. %g[\n",__func__,
							voie,MonitTrmtAb6[voie][0],pos_trmt,MonitTrmtAb6[voie][1],
							MonitTrmtAb6[voie][0]+((float)pos_trmt*MonitTrmtAb6[voie][1]),
							MonitTrmtAb6[voie][0]+((float)(pos_trmt+trmtdim)*MonitTrmtAb6[voie][1]));
					}
				} else { GraphDataUse(g,x+2,0); GraphDataUse(g,y+2,0); }
			}
		#endif /* MONIT_EVT_TRAITE */
			if(planche_classes) OpiumRefreshGraph(g->cdr);
			else if(g == gEvtSolo) OpiumDisplay(gEvtSolo->cdr);
			else if(planche_run) {
				OpiumRefresh(g->cdr);
				if(config->nbcatg > 1) GraphPrint(g,Dx,Dy,evt->nom_catg);
				if(OpiumAlEcran(pLectEvtNum)) PanelRefreshVars(pLectEvtNum);
				if(OpiumAlEcran(pLectEvtQui)) PanelRefreshVars(pLectEvtQui);
			}
			if(categ_demandee == 0) MonitEvtAlEcran = evtnum; else categ->affiche = evtnum;
			// printf("(%s) A l'ecran: #%d\n",__func__,(categ_demandee == 0)? MonitEvtAlEcran: categ->affiche);
		}
	#ifdef REMETTRE_COMME_LECTDISPLAY
		/* le meme intervalle de temps pour tous ou le dernier evt de chacun? */
		evtduree = evtdim * VoieEvent[voie].horloge;
		origine = (evtstart - VoieTampon[voie].decal) * VoieTampon[voie].trmt[TRMT_AU_VOL].compactage;
		for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) {
			if(((f->affiche == MONIT_SHOW) || (f->affiche == MONIT_ALWS)) && (f->type == MONIT_EVENT) && (g = f->g)) {
				for(j=0, t=f->trace; j<f->nb; j++,t++) {
					int bolo,voie;
					bolo = t->bolo;
					voie = t->voie;
					evtstart = (origine / (int64)VoieTampon[voie].trmt[TRMT_AU_VOL].compactage) + VoieTampon[voie].decal;
					// MonitEvtAb6[voie][0] = ((float)evtstart * VoieEvent[voie].horloge) - (MonitT0 * 1000.0);
					// debut = VoieTampon[voie].brutes->t.sval + Modulo(evtstart,VoieTampon[voie].brutes->max);
					evtdim = evtduree / VoieEvent[voie].horloge;
					y += 1;
					rc = GraphDataReplace(g,y,GRF_SHORT|GRF_YAXIS,debut,VoieTampon[voie].brutes->max);
					if(rc < 0) printf("! (%s[fenetre evt]) le pointage sur l'evenement a echoue\n",__func__);
					else GraphDataUse(g,y,evtdim);
				}
				if(f->axeY.i.min == f->axeY.i.max) GraphAxisReset(g,GRF_YAXIS);
				OpiumDisplay(g->cdr);
			}
		}
	#endif
		if(!ajoute) { x += 2; y += 2; }
		if(g == gEvtSolo) vt++; else break;
	}

	return(0);
}
/* ========================================================================== */
int MonitEvtListe(Menu menu, MenuItem item) {
	int vt,voietrig,n;
	float taux;
	TypeVoieArchivee *archivee;

	if(!EvtASauver) {
		OpiumError("Pas d'evenement en memoire");
		return(0);
	}
	if(!((tMonit->cdr)->displayed)) OpiumDisplay(tMonit->cdr);
	TermHold(tMonit);

	if(EvtASauver == 1)
		TermPrint(tMonit,L_("========== %s/ 1 evenement en memoire ==========\n"),DateHeure());
	else if(EvtASauver > 1)
		TermPrint(tMonit,L_("========== %s/ %d evenements en memoire: #%d a #%d ==========\n"),DateHeure(),
			EvtASauver,Evt[0].num,Evt[EvtASauver-1].num);
	else { TermRelease(tMonit); return(0); }
	for(n=0; n<EvtASauver; n++) {
		if(!(n % 16)) TermPrint(tMonit,"#Evt  Classe Voie             Base +Amplitude Bruit      Debut     Date(s)  Montee  Descente  Delai(ms)\n");
		for(vt=0; vt<Evt[n].nbvoies; vt++) {
			archivee = &(Evt[n].voie[vt]);
			if(!archivee) {
				TermPrint(tMonit,"%4d ! adresse de la voie archivee: nulle pour l'evt [%d]/[%d]\n",Evt[n].num,n,EvtASauver);
				continue;
			}
			voietrig = archivee->num;
			if(vt == 0) TermPrint(tMonit,"%4d %6s",Evt[n].num,Evt[n].nom_catg); else TermPrint(tMonit,"           ");
			TermPrint(tMonit," %c%-16s %7.1f %+7.1f %5.1f %5d%06d %3d,%06d %7.3f %8.3f",(vt == Evt[n].voie_evt)?'*':' ',
				VoieManip[voietrig].nom,archivee->val[MONIT_BASE],archivee->val[MONIT_AMPL],archivee->val[MONIT_BRUIT],
				(int)archivee->debut/1000000,Modulo(archivee->debut,1000000),archivee->sec,archivee->msec,
				archivee->val[MONIT_MONTEE],archivee->val[MONIT_DESCENTE]);
			if(vt == 0) TermPrint(tMonit,"   %9.3f\n",Evt[n].delai*1000.0); else TermPrint(tMonit,"\n");
		}
	}
	taux = (float)(EvtASauver - 1)/
		((float)(Evt[EvtASauver-1].voie[0].sec - Evt[0].voie[0].sec) + ((float)(Evt[EvtASauver-1].voie[0].msec - Evt[0].voie[0].msec) / 1000000.0));
	TermPrint(tMonit,L_("========== %f evenements/seconde ==========\n"),taux);

	TermRelease(tMonit);
	return(0);
}
/* ========================================================================== */
int MonitEvtDemande(Menu menu, MenuItem item) {
	if(!EvtASauver) {
		OpiumError("Pas d'evenement en memoire");
		return(0);
	}
	if(OpiumReadInt(L_("Numero d'evenement"),&LectCntl.MonitEvtNum) == PNL_CANCEL) return(0);
	return(MonitEvtAffiche(0,0,menu,1));
}
/* ========================================================================== */
int MonitEvtPrecedent(Menu menu, MenuItem item) {
	LectCntl.MonitEvtNum--; return(MonitEvtAffiche(0,0,menu,1));
}
/* ========================================================================== */
int MonitEvtSuivant(Menu menu, MenuItem item) {
	LectCntl.MonitEvtNum++; return(MonitEvtAffiche(0,0,menu,1));
}
/* ========================================================================== */
int MonitHistosDef(Menu menu, MenuItem item) {
	Histo histo;
	//-	int bins_ampl,bins_montee;
	int voie,bolo,vm,cap;

	//-	bins_ampl = SettingsAmplBins;
	//-	bins_montee = SettingsMonteeBins;
	bolo = BoloNum; vm = ModlNum; voie = 0;
	if(OpiumExec(pMonitHistosDef->cdr) == PNL_CANCEL) return(0);
	SettingsParmsASauver = 1;
	if(Bolo[BoloNum].lu == DETEC_HS) {
		OpiumError("Le detecteur %s n'est pas lu",Bolo[BoloNum].nom);
		BoloNum = bolo; ModlNum = vm;
		return(0);
	}
	for(cap=0; cap<Bolo[BoloNum].nbcapt; cap++) {
		voie = Bolo[BoloNum].captr[cap].voie;
		if(ModlNum == VoieManip[voie].type) break;
	}
	if(cap == Bolo[BoloNum].nbcapt) {
		OpiumError("Pas de voie '%s' pour le detecteur '%s'",ModeleVoie[ModlNum].nom,Bolo[BoloNum].nom);
		BoloNum = bolo; ModlNum = vm;
		return(0);
	}
	if(OpiumExec(pMonitHistosDyn->cdr) == PNL_CANCEL) return(0);
	MonitASauver = 1;
	VoieNum = voie;
	SettingsHistoVoie = VoieNum;

	histo = HistoOf(hdBruitAmpl);
	HistoLimitsFloat(histo,SettingsAmplMin,SettingsAmplMax);
	//	HistoRebin(histo,SettingsAmplBins);
	H2DLimitsFloat(h2D,HST_AXEX,SettingsAmplMin,SettingsAmplMax);
	H2DRebin(h2D,HST_AXEX,SettingsAmplBins);
	Coup2DMonteeX[0] = SettingsAmplMin; Coup2DMonteeX[0] = SettingsAmplMax;

	histo = HistoOf(hdBruitMontee);
	HistoLimitsFloat(histo,SettingsMonteeMin,SettingsMonteeMax);
	//	HistoRebin(histo,SettingsMonteeBins);
	H2DLimitsFloat(h2D,HST_AXEY,SettingsMonteeMin,SettingsMonteeMax);
	//	H2DRebin(h2D,HST_AXEY,SettingsMonteeBins);
	Coup2DAmplY[0] = SettingsMonteeMin; Coup2DAmplY[1] = SettingsMonteeMax;

	return(0);
}
/* ========================================================================== */
void MonitHistosDynamique() {

	CoupureAmpl[0] = CoupureAmpl[1] = VoieManip[SettingsHistoVoie].def.trgr.minampl;
	if(MonitHamplY[0] == MonitHamplY[1])
		GraphAxisAutoRange(gBruitAmpl,GRF_YAXIS);
	else GraphAxisIntRange(gBruitAmpl,GRF_YAXIS,MonitHamplY[0],MonitHamplY[1]);

	CoupureMontee[0] = CoupureMontee[1] = VoieManip[SettingsHistoVoie].def.trgr.montee;
	if(MonitHmonteeY[0] == MonitHmonteeY[1])
		GraphAxisAutoRange(gBruitMontee,GRF_YAXIS);
	else GraphAxisIntRange(gBruitMontee,GRF_YAXIS,MonitHmonteeY[0],MonitHmonteeY[1]);

	Coup2DAmplX[0] = Coup2DAmplX[1] = VoieManip[SettingsHistoVoie].def.trgr.minampl;
	Coup2DAmplY[0] = SettingsMonteeMin; Coup2DAmplY[1] = SettingsMonteeMax;
	Coup2DMonteeX[0] = SettingsAmplMin; Coup2DMonteeX[0] = SettingsAmplMax;
	Coup2DMonteeY[0] = Coup2DMonteeY[1] = VoieManip[SettingsHistoVoie].def.trgr.montee;
	GraphDataIntGamma(g2D,2, /* a 'variabliser' */ MonitH2DY[0],MonitH2DY[1],0,OPIUM_LUT_DIM - 1);

}
#ifdef MONIT_PAR_VOIE
/* ========================================================================== */
static int MonitRefresh() {
	int voie;

	for(voie=0; voie<VoiesNb; voie++) {
		if(OpiumDisplayed(gDonnee[voie]->cdr))
			OpiumRefresh(gDonnee[voie]->cdr);
	}
	if(OpiumDisplayed(gBruitAmpl->cdr)) OpiumRefresh(gBruitAmpl->cdr);
	if(OpiumDisplayed(gBruitMontee->cdr)) OpiumRefresh(gBruitMontee->cdr);
	if(OpiumDisplayed(g2D->cdr)) OpiumRefresh(g2D->cdr);

	return(0);
}
#endif
/* ========================================================================== */
static int MonitChangeParms(Panel panel, void *arg) {
	MonitASauver = 1;
	return(1);
}
/* ========================================================================== */
static int MonitChangeGeneral(Panel panel, void *arg) {
	GraphQualiteDefaut(FondPlots? WND_Q_ECRAN: WND_Q_PAPIER);
	// MonitSuiviIncludes();
	if(MonitEvtClasses) LectPlancheSuivi(1); else OpiumClear(bSuiviEvts);
	MonitASauver = 1;
	return(1);
}
/* ========================================================================== */
static int MonitChangePerso(Panel panel, void *arg) {
	if(MonitEvtClasses) LectPlancheSuivi(1); else OpiumClear(bSuiviEvts);
	MonitFenModifiees = 1;
	MonitASauver = 1;
	return(1);
}
/* ========================================================================== */
static int MonitChangeDivers(Panel panel, void *arg) {
	if(MonitFftDemande) MonitFftAuVol(0,0);
	else if(OpiumAlEcran(gMonitFftAuVol)) OpiumClear(gMonitFftAuVol->cdr);
	if(PicsActif) MonitRaiesDefini(0,0);
	else {
		if(OpiumAlEcran(gPicsPositions)) OpiumClear(gPicsPositions->cdr);
		if(OpiumAlEcran(gPicsSigmas)) OpiumClear(gPicsSigmas->cdr);
	}
	MonitASauver = 1;
	return(1);
}
/* ========================================================================== */
static int MonitChangeCatg(Panel panel, void *arg) {
	MonitCatgModifie = 1;
	if(OpiumDisplayed(bSuiviEvts)) LectPlancheSuivi(1);
	return(1);
}
/* ========================================================================== */
static void MonitEcranInit() {
	int catg;

	MonitCatgPtr[0] = &TousLesEvts; MonitCatgNom[0] = "toutes";
	for(catg=0; catg<CATG_MAX; catg++) {
		MonitCatgPtr[catg + 1] = &(MonitCatg[catg]);
		MonitCatgNom[catg + 1] = MonitCatg[catg].nom;
	}
	MonitCatgNom[CATG_MAX + 1] = "\0";
	for(catg=0; catg<=CATG_MAX; catg++) {
		TypeMonitCatg *categ = MonitCatgPtr[catg];
		categ->nom[0] = '\0'; categ->num = catg;
		categ->utilisee = categ->evol = categ->trace = categ->deconv = 0; categ->taux = categ->autoscale = 1;
		categ->scalechge = 0; categ->min = -5000; categ->max = 5000;
		categ->stamp_trouve = categ->stamp_utilise = 0;
		categ->trouves = categ->utilises = 0;
		categ->delai = categ->freq = 0.0;
		categ->activite.val = 0; categ->activite.nb = categ->activite.dim = categ->activite.max = 0;
		categ->pTaux = categ->pMinMax = categ->pEvtval = 0;
		categ->gSuivi = categ->gEvt = 0;
	}

	RepertoireCreeRacine(FichierPrefTagDefs);
	SambaLogDef("Lecture des affichages d'evenements",0,FichierPrefMonitCatg);
	strcpy(TousLesEvts.nom,CATG_ALL);
	if(ArgScan(FichierPrefMonitCatg,MonitCatgDesc) == 0) {
		printf("  . Pas de planche definie\n");
		strcpy(TousLesEvts.nom,CATG_ALL);
		MonitCatgNb = 0;
		ArgPrint(FichierPrefMonitCatg,MonitCatgDesc);
	}
	MonitCatgModifie = 0;
	if(ImprimeConfigs) {
		printf("#\n### Fichier: '%s' ###\n",FichierPrefMonitCatg);
		ArgPrint("*",MonitCatgDesc);
	}
}
/* ========================================================================== */
int MonitEcranEcrit() {
	if(SettingsSauveChoix) {
		if(OpiumReadFile("Sauver sur le fichier",FichierPrefMonitCatg,MAXFILENAME) == PNL_CANCEL) return(0);
	}
	if(ArgPrint(FichierPrefMonitCatg,MonitCatgDesc) > 0) {
		MonitCatgModifie = 0;
		printf("%s/ Affichage par classe sauve dans %s\n",DateHeure(),FichierPrefMonitCatg);
	} else OpiumError("Sauvegarde sur '%s' impossible",FichierPrefMonitCatg);
	return(0);
}
/* ========================================================================== */
int MonitEcranPanel() {
	int i,j,nb;
	TypeMonitFenetre *f;

	if(Acquis[AcquisLocale].etat.active) {
		iEcran = InfoNotifie(iEcran,L_("Modification de l'ecran impossible pendant une acquisition"));
		return(0);
	}
	nb = MonitFenNb + 28;
	if(pMonitEcran) {
		if(PanelItemMax(pMonitEcran) < nb) {
			PanelDelete(pMonitEcran);
			pMonitEcran = 0;
		} else PanelErase(pMonitEcran);
	}
	if(pMonitEcran == 0) pMonitEcran = PanelCreate(nb);
	PanelTitle(pMonitEcran,"Affichages");

		PanelRemark(pMonitEcran,L_("=========== Statique ==========="));
//	j = PanelBool  (pMonitEcran,"Mode d'acquisition",&MonitAffModes);
//		PanelItemColors(pMonitEcran,j,SambaRougeVertJauneOrange,2);
//	j = PanelBool  (pMonitEcran,"Parametres trigger",&MonitAffTrig);
//		PanelItemColors(pMonitEcran,j,SambaRougeVertJauneOrange,2);
//	j = PanelBool  (pMonitEcran,"Etat d'acquisition",&MonitAffEtat);
//			PanelItemColors(pMonitEcran,j,SambaRougeVertJauneOrange,2);
#ifdef MODULES_RESEAU
	j = PanelBool  (pMonitEcran,L_("Etat du reseau"),&MonitAffNet);
			PanelItemColors(pMonitEcran,j,SambaRougeVertJauneOrange,2);
			PanelItemLngr(pMonitEcran,j,14);
#endif
		PanelRemark(pMonitEcran,L_("======== Periodiquement ========"));
		PanelFloat (pMonitEcran,L_("Intervalle (s)"),&MonitIntervSecs); /* concerne aussi les histos et le freq.metre */
	j = PanelListB (pMonitEcran,L_("Qualite"),WndSupportNom,&FondPlots,8); PanelItemColors(pMonitEcran,j,SambaBlancNoir,2);
	j = PanelBool  (pMonitEcran,L_("Lignes de base"),&MonitAffBases);
		PanelItemColors(pMonitEcran,j,SambaRougeVertJauneOrange,2);
		PanelItemLngr(pMonitEcran,j,14);
	j = PanelBool  (pMonitEcran,L_("Panel taux evt"),&MonitAffpTaux);
		PanelItemColors(pMonitEcran,j,SambaRougeVertJauneOrange,2);
		PanelItemLngr(pMonitEcran,j,14);
	j = PanelBool  (pMonitEcran,L_("Graph.taux evt"),&MonitAffgTaux);
		PanelItemColors(pMonitEcran,j,SambaRougeVertJauneOrange,2);
		PanelItemLngr(pMonitEcran,j,14);
	j = PanelInt   (pMonitEcran,L_("Mesure taux (s)"),&SettingsTauxPeriode); /* pour l'affichage du taux d'evts */
		if(Acquis[AcquisLocale].etat.active) PanelItemSelect(pMonitEcran,j,0);
	j = PanelBool  (pMonitEcran,L_("Graph.seuils"),&MonitAffgSeuils);
		PanelItemColors(pMonitEcran,j,SambaRougeVertJauneOrange,2);
		PanelItemLngr(pMonitEcran,j,14);
	j = PanelBool  (pMonitEcran,L_("Spectre du bruit"),&MonitFftDemande);
		PanelItemColors(pMonitEcran,j,SambaRougeVertJauneOrange,2);
		PanelItemLngr(pMonitEcran,j,14);
	j = PanelBool  (pMonitEcran,L_("Synchro modulation"),&MonitSynchro);
		PanelItemColors(pMonitEcran,j,SambaRougeVertJauneOrange,2);
		PanelItemLngr(pMonitEcran,j,14);
		PanelSepare(pMonitEcran,L_("Graphiques utilisateur"));
	for(i=0; i<MonitFenNb; i++) {
		j = PanelKeyB(pMonitEcran,MonitFen[i].titre,L_(MONIT_DISPLAY_CLES),&(MonitFen[i].affiche),6);
			PanelItemColors(pMonitEcran,j,SambaRougeVertJauneOrange,4);
			PanelItemLngr(pMonitEcran,j,14);
	}
		PanelSepare(pMonitEcran,L_("Histogrammes bruit"));
		PanelRemark(pMonitEcran,L_(".... Amplitude ................."));
	j = PanelBool  (pMonitEcran,L_("Affichage"),&MonitHampl);
			PanelItemColors(pMonitEcran,j,SambaRougeVertJauneOrange,2);
			PanelItemLngr(pMonitEcran,j,14);
		PanelInt   (pMonitEcran,L_("Bin mini"),&(MonitHamplY[0]));
		PanelInt   (pMonitEcran,L_("Bin maxi"),&(MonitHamplY[1]));
		PanelRemark(pMonitEcran,L_(".... Temps de montee ..........."));
	j = PanelBool  (pMonitEcran,L_("Affichage"),&MonitHmontee);
			PanelItemColors(pMonitEcran,j,SambaRougeVertJauneOrange,2);
			PanelItemLngr(pMonitEcran,j,14);
		PanelInt   (pMonitEcran,L_("Bin mini"),&(MonitHmonteeY[0]));
		PanelInt   (pMonitEcran,L_("Bin maxi"),&(MonitHmonteeY[1]));
		PanelRemark(pMonitEcran,L_(".... Amplitude x Tps montee ...."));
	j = PanelBool  (pMonitEcran,L_("Affichage"),&MonitH2D);
			PanelItemColors(pMonitEcran,j,SambaRougeVertJauneOrange,2);
			PanelItemLngr(pMonitEcran,j,14);
		PanelInt   (pMonitEcran,L_("Bin mini"),&MonitH2DY[0]);
		PanelInt   (pMonitEcran,L_("Bin maxi"),&MonitH2DY[1]);
		PanelRemark(pMonitEcran,L_("======== Selon arrivage ========"));
	j = PanelBool  (pMonitEcran,L_("Dernier evenement"),&MonitAffEvts);
			PanelItemColors(pMonitEcran,j,SambaRougeVertJauneOrange,2);
			PanelItemLngr(pMonitEcran,j,14);
	PanelOnApply(pMonitEcran,MonitChangeGeneral,0);
	PanelOnOK(pMonitEcran,MonitChangeGeneral,0);

	if(OpiumExec(pMonitEcran->cdr) == PNL_CANCEL) return(0);
	MonitIntervAff = (int)(MonitIntervSecs * 1000.0);
	GraphQualiteDefaut(FondPlots? WND_Q_ECRAN: WND_Q_PAPIER);

	if(pSettingsModes) {
		if(MonitAffModes) {
			if(OpiumDisplayed(pSettingsModes->cdr)) OpiumRefresh(pSettingsModes->cdr);
			else OpiumDisplay(pSettingsModes->cdr);
		} else if(OpiumDisplayed(pSettingsModes->cdr)) OpiumClear(pSettingsModes->cdr);
	}

	if(pSettingsTrigger) {
		if(MonitAffTrig) {
			if(OpiumDisplayed(pSettingsTrigger->cdr)) OpiumRefresh(pSettingsTrigger->cdr);
			else if(PanelItemNb(pSettingsTrigger)) OpiumDisplay(pSettingsTrigger->cdr);
		} else if(OpiumDisplayed(pSettingsTrigger->cdr)) OpiumClear(pSettingsTrigger->cdr);
	}

	if(bLectBases) {
		if(MonitAffBases) {
			if(OpiumDisplayed(bLectBases)) {
				if(pLectBaseValeurs) PanelRefreshVars(pLectBaseValeurs);
				if(gLectBaseNiveaux) OpiumRefresh(gLectBaseNiveaux->cdr);
				if(gLectBaseBruits) OpiumRefresh(gLectBaseBruits->cdr);
			} else OpiumDisplay(bLectBases);
		} else if(OpiumDisplayed(bLectBases)) OpiumClear(bLectBases);
	}
	if(pTauxDetaille) {
		if(MonitAffpTaux) OpiumDisplay(pTauxDetaille->cdr);
		else if(OpiumDisplayed(pTauxDetaille->cdr)) OpiumClear(pTauxDetaille->cdr);
	}
	if(gTauxDetaille) {
		if(MonitAffgTaux) OpiumDisplay(gTauxDetaille->cdr);
		else if(OpiumDisplayed(gTauxDetaille->cdr)) OpiumClear(gTauxDetaille->cdr);
	}
	if(gSeuils) {
		if(MonitAffgSeuils) OpiumDisplay(gSeuils->cdr);
		else if(OpiumDisplayed(gSeuils->cdr)) OpiumClear(gSeuils->cdr);
	}
	if(pLectEtat) {
		if(MonitAffEtat) OpiumDisplay(pLectEtat->cdr);
		else if(OpiumDisplayed(pLectEtat->cdr)) OpiumClear(pLectEtat->cdr);
	}

#ifdef MODULES_RESEAU
	if((SambaMode == SAMBA_MAITRE) && pAcquisEtat) {
		if(MonitAffNet) OpiumDisplay(pAcquisEtat->cdr);
		else if(OpiumDisplayed(pAcquisEtat->cdr)) OpiumClear(pAcquisEtat->cdr);
	}
#endif

	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if(f->affiche != MONIT_FOLW) {
		if((f->affiche == MONIT_SHOW) || (f->affiche == MONIT_ALWS)) MonitFenAffiche(f,1);
		else if((f->g)) { if(OpiumDisplayed((f->g)->cdr)) OpiumClear((f->g)->cdr); }
	}

	MonitHistosDynamique();
	if(MonitHampl) OpiumDisplay(gBruitAmpl->cdr);
	else if(OpiumDisplayed(gBruitAmpl->cdr)) OpiumClear(gBruitAmpl->cdr);

	if(MonitHmontee) OpiumDisplay(gBruitMontee->cdr);
	else if(OpiumDisplayed(gBruitMontee->cdr)) OpiumClear(gBruitMontee->cdr);

	if(MonitH2D) OpiumDisplay(g2D->cdr);
	else if(OpiumDisplayed(g2D->cdr)) OpiumClear(g2D->cdr);

	return(0);
}
/* ========================================================================== */
int Monit2DRefresh(Menu menu, MenuItem item) {
	int i; TypeMonitFenetre *f; Graph g;

	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if((f->type == MONIT_2DHISTO) && (g = f->g)) {
		OpiumBloque(g->cdr,0); OpiumDisplay(g->cdr); OpiumBloque(g->cdr,1);
	}
	return(0);
}
/* ========================================================================== */
int MonitEcranRefresh(Menu menu, MenuItem item) {
	int i; TypeMonitFenetre *f;

	MonitIntervAff = (int)(MonitIntervSecs * 1000.0);
	GraphQualiteDefaut(FondPlots? WND_Q_ECRAN: WND_Q_PAPIER);

	if(pSettingsModes) {
		if(MonitAffModes) {
			if(OpiumDisplayed(pSettingsModes->cdr)) OpiumRefresh(pSettingsModes->cdr);
			else OpiumDisplay(pSettingsModes->cdr);
		} else if(OpiumDisplayed(pSettingsModes->cdr)) OpiumClear(pSettingsModes->cdr);
	}

	if(pSettingsTrigger) {
		if(MonitAffTrig) {
			if(OpiumDisplayed(pSettingsTrigger->cdr)) OpiumRefresh(pSettingsTrigger->cdr);
			else if(PanelItemNb(pSettingsTrigger)) OpiumDisplay(pSettingsTrigger->cdr);
		} else if(OpiumDisplayed(pSettingsTrigger->cdr)) OpiumClear(pSettingsTrigger->cdr);
	}

	if(bLectBases) {
		if(MonitAffBases) {
			if(OpiumDisplayed(bLectBases)) {
				if(pLectBaseValeurs) PanelRefreshVars(pLectBaseValeurs);
				if(gLectBaseNiveaux) OpiumRefresh(gLectBaseNiveaux->cdr);
				if(gLectBaseBruits) OpiumRefresh(gLectBaseBruits->cdr);
			} else OpiumDisplay(bLectBases);
		} else if(OpiumDisplayed(bLectBases)) OpiumClear(bLectBases);
	}
	if(pTauxDetaille) {
		if(MonitAffpTaux) OpiumDisplay(pTauxDetaille->cdr);
		else if(OpiumDisplayed(pTauxDetaille->cdr)) OpiumClear(pTauxDetaille->cdr);
	}
	if(gTauxDetaille) {
		if(MonitAffgTaux) OpiumDisplay(gTauxDetaille->cdr);
		else if(OpiumDisplayed(gTauxDetaille->cdr)) OpiumClear(gTauxDetaille->cdr);
	}
	if(gSeuils) {
		if(MonitAffgSeuils) OpiumDisplay(gSeuils->cdr);
		else if(OpiumDisplayed(gSeuils->cdr)) OpiumClear(gSeuils->cdr);
	}
	if(pLectEtat) {
		if(MonitAffEtat) OpiumDisplay(pLectEtat->cdr);
		else if(OpiumDisplayed(pLectEtat->cdr)) OpiumClear(pLectEtat->cdr);
	}

#ifdef MODULES_RESEAU
	if((SambaMode == SAMBA_MAITRE) && pAcquisEtat) {
		if(MonitAffNet) OpiumDisplay(pAcquisEtat->cdr);
		else if(OpiumDisplayed(pAcquisEtat->cdr)) OpiumClear(pAcquisEtat->cdr);
	}
#endif

	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if(f->affiche != MONIT_FOLW) {
		if((f->affiche == MONIT_SHOW) || (f->affiche == MONIT_ALWS)) MonitFenAffiche(f,1);
		else if((f->g)) { if(OpiumDisplayed((f->g)->cdr)) OpiumClear((f->g)->cdr); }
	}

	MonitHistosDynamique();
	if(MonitHampl) OpiumDisplay(gBruitAmpl->cdr);
	else if(OpiumDisplayed(gBruitAmpl->cdr)) OpiumClear(gBruitAmpl->cdr);

	if(MonitHmontee) OpiumDisplay(gBruitMontee->cdr);
	else if(OpiumDisplayed(gBruitMontee->cdr)) OpiumClear(gBruitMontee->cdr);

	if(MonitH2D) OpiumDisplay(g2D->cdr);
	else if(OpiumDisplayed(g2D->cdr)) OpiumClear(g2D->cdr);
	
	return(0);
}
/* ========================================================================== */
int MonitEcranPlanche() {
	int i,j,k,nb,utilises; int x,y,xa,ya,xb,yb,xc,yc,xm,ym; char affiche; unsigned short direct;
	Panel p; TypeMonitFenetre *f;

	if(Acquis[AcquisLocale].etat.active) {
		iEcran = InfoNotifie(iEcran,L_("Redefinition de l'ecran impossible pendant une acquisition"));
		return(0);
	}
	if((affiche = OpiumDisplayed(bMonitEcran))) OpiumClear(bMonitEcran);
	BoardTrash(bMonitEcran); DetecteursEvtCateg();
	OpiumTitle(bMonitEcran,L_("Affichage a l'ecran"));
	x = 2 * Dx; y = 2 * Dy;
	direct = 0; // direct = PNL_DIRECT;

	BoardAreaOpen(L_("Parametres"),WND_PLAQUETTE);
	nb = 6; if(ModulationExiste) nb++;
#ifdef MODULES_RESEAU
	nb++;
#endif
	p = PanelCreate(nb); PanelSupport(p,WND_CREUX);
	j = PanelFloat(p,L_("Periode affichages (s)"),&MonitIntervSecs); PanelItemLngr(p,j,6); /* concerne aussi les histos et le freq.metre */
	j = PanelInt  (p,L_("Mesure taux evts (s)"),&SettingsTauxPeriode); PanelItemLngr(p,j,6);
		if(Acquis[AcquisLocale].etat.active) PanelItemSelect(p,j,0);
	j = PanelBool (p,L_("Dernier evenement"),&MonitAffEvts); PanelItemLngr(p,j,6); PanelItemColors(p,j,SambaJauneVert,2);
	j = PanelBool (p,L_("Evenements par classe"),&MonitEvtClasses); PanelItemLngr(p,j,6); PanelItemColors(p,j,SambaJauneVert,2);
	j = PanelBool (p,L_("Sonorisation"),&MonitModeSon);
	j = PanelListB(p,L_("Qualite des graphiques"),WndSupportNom,&FondPlots,6); PanelItemColors(p,j,SambaBlancNoir,2);
	if(ModulationExiste) {
		j = PanelBool (p,L_("Synchro modulation"),&MonitSynchro); PanelItemLngr(p,j,6); PanelItemColors(p,j,SambaRougeVert,2);
	}
#ifdef MODULES_RESEAU
	j = PanelBool  (p,L_("Etat du reseau"),&MonitAffNet); PanelItemColors(p,j,SambaRougeVert,2); PanelItemLngr(p,j,6);
#endif
	BoardAddPanel(bMonitEcran,p,x,y,0);
	PanelOnApply(p,MonitChangeGeneral,0); PanelOnOK(p,MonitChangeGeneral,0);
	BoardAreaCurrentPos(bMonitEcran,&xa,&ya,&xc,&yc);
	BoardAddBouton(bMonitEcran,L_("Sauver"),MonitSauve,((xa+xc)/2)-(3*Dx),yc+Dy,0);
	BoardAreaEnd(bMonitEcran,&xa,&ya); xm = xa; ym = ya;
	y = ya + Dy;

	x = ((xa - (2 * Dx) - (13 * Dx)) / 2) + (2 * Dx);
	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if((f->affiche > MONIT_HIDE) && (f->type == MONIT_2DHISTO)) {
		BoardAddBouton(bMonitEcran,L_("MAJ histos 2D"),Monit2DRefresh,x,y,0);
		y += (2 * Dy);
		break;
	}

	x = 3 * Dx; y += (3 * Dy);
	BoardAreaOpen(L_("Fenetres predefinies"),WND_PLAQUETTE);
		BoardAreaOpen(L_("Histogrammes"),WND_RAINURES);
		p = PanelCreate(3); PanelSupport(p,WND_CREUX); PanelMode(p,direct);
		j = PanelBool(p,L_("Amplitude"),&MonitHampl); PanelItemColors(p,j,SambaRougeVert,2);
		j = PanelBool(p,L_("Temps de montee"),&MonitHmontee); PanelItemColors(p,j,SambaRougeVert,2);
		j = PanelBool(p,L_("Ampl. x Tps montee"),&MonitH2D); PanelItemColors(p,j,SambaRougeVert,2);
		BoardAddPanel(bMonitEcran,p,x,y,0);
		BoardAreaEnd(bMonitEcran,&xb,&yb);

		y = yb + (2 * Dy);
		BoardAreaOpen(L_("Divers"),WND_RAINURES);
		p = pMonitDivers = PanelCreate(6); PanelSupport(p,WND_CREUX); PanelMode(p,direct);
		j = PanelBool(p,L_("Panel taux evt"),  &MonitAffpTaux);   PanelItemColors(p,j,SambaRougeVert,2);
		j = PanelBool(p,L_("Graph.taux evt"),  &MonitAffgTaux);   PanelItemColors(p,j,SambaRougeVert,2);
		j = PanelBool(p,L_("Lignes de base"),  &MonitAffBases);   PanelItemColors(p,j,SambaRougeVert,2);
		j = PanelBool(p,L_("Graph.seuils"),    &MonitAffgSeuils); PanelItemColors(p,j,SambaRougeVert,2);
		j = PanelBool(p,L_("Spectre du bruit"),&MonitFftDemande); PanelItemColors(p,j,SambaRougeVert,2);
		j = PanelBool(p,L_("Suivi du gain"),   &PicsActif);       PanelItemColors(p,j,SambaRougeVert,2);
		PanelOnApply(p,MonitChangeDivers,0); PanelOnOK(p,MonitChangeDivers,0);
		BoardAddPanel(bMonitEcran,p,x,y,0);
		BoardAreaEnd(bMonitEcran,&xb,&yb);
	BoardAreaEnd(bMonitEcran,&xa,&ya); xb = xa; yb = ya;

	x = xm + (3 * Dx); y = 2 * Dy;
	BoardAreaOpen(L_("Personalise"),WND_PLAQUETTE);
	p = PanelCreate(MonitFenNb); PanelSupport(p,WND_CREUX); PanelMode(p,direct);
	for(i=0; i<MonitFenNb; i++) {
		j = PanelKeyB(p,MonitFen[i].titre,L_(MONIT_DISPLAY_CLES),&(MonitFen[i].affiche),6);
		PanelItemColors(p,j,SambaRougeVertJauneViolet,4);
	}
	PanelOnApply(p,MonitChangePerso,0); PanelOnOK(p,MonitChangePerso,0);
	BoardAddPanel(bMonitEcran,p,x,y,0);
	BoardAreaCurrentPos(bMonitEcran,&xa,&ya,&xc,&yc);
	BoardAddBouton(bMonitEcran,L_("Sauver"),MonitFenEcrit,((xa+xc)/2)-(3*Dx),yc+Dy,0);
	BoardAreaEnd(bMonitEcran,&xa,&ya); if(ya > ym) ym = ya;

	x = xb + (3 * Dx); y = ym + (2 * Dy);
	BoardAreaOpen(L_("Evenements par classe"),WND_PLAQUETTE);
	utilises = 0; for(k=0; k<MonitCatgNb; k++) if(MonitCatg[k].utilisee) utilises++;
	p = PanelCreate(4 * (3 + utilises)); PanelSupport(p,WND_CREUX); PanelColumns(p,4); PanelMode(p,direct|PNL_BYLINES);
	j = PanelBlank(p);
	j = PanelItemSelect(p,PanelText(p,0,L_("Valeur"),7),0);
	j = PanelItemSelect(p,PanelText(p,0,L_("Courbe"),7),0);
	j = PanelBlank(p);
	j = PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,L_("Classe"),7),0),1);
	j = PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,L_("taux"),7),0),1);
	j = PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,L_("taux"),7),0),1);
	j = PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,L_("Trace"),7),0),1);
	for(k=0; k<MonitCatgNb+1; k++) if(MonitCatgPtr[k]->utilisee) {
		j = PanelItemSelect(p,PanelText(p,0,MonitCatgPtr[k]->nom,7),0); // PanelItemSouligne(p,j,1);
		j = PanelBool(p,0,&MonitCatgPtr[k]->taux);  PanelItemColors(p,j,SambaRougeVert,2); // PanelItemSouligne(p,j,1);
		j = PanelBool(p,0,&MonitCatgPtr[k]->evol);  PanelItemColors(p,j,SambaRougeVert,2); // PanelItemSouligne(p,j,1);
		j = PanelBool(p,0,&MonitCatgPtr[k]->trace); PanelItemColors(p,j,SambaRougeVert,2); // PanelItemSouligne(p,j,1);
	}
	PanelOnApply(p,MonitChangeCatg,0); PanelOnOK(p,MonitChangeCatg,0);
	BoardAddPanel(bMonitEcran,p,x,y,0);
	// int xt,yt;
	// BoardAreaOpen(L_("Dimension graphiques"),WND_RAINURES);
	p = PanelCreate(9); PanelSupport(p,WND_CREUX); PanelColumns(p,3); PanelMode(p,direct|PNL_BYLINES);
	// j = PanelBlank(p);
	j = PanelItemSelect(p,PanelText(p,0,L_("dim graph."),11),0);
	j = PanelItemSelect(p,PanelText(p,0,L_("largeur"),7),0);
	j = PanelItemSelect(p,PanelText(p,0,L_("hauteur"),7),0);
	j = PanelItemSelect(p,PanelText(p,0,L_("par classe"),11),0);
	j = PanelInt(p,0,&MonitEvtCatgLarg); PanelItemLngr(p,j,7);
	j = PanelInt(p,0,&MonitEvtCatgHaut); PanelItemLngr(p,j,7);
	j = PanelItemSelect(p,PanelText(p,0,L_("sous focus"),11),0);
	j = PanelInt(p,0,&MonitEvtFocsLarg); PanelItemLngr(p,j,7);
	j = PanelInt(p,0,&MonitEvtFocsHaut); PanelItemLngr(p,j,7);
	PanelOnApply(p,MonitChangeCatg,0); PanelOnOK(p,MonitChangeCatg,0);
	BoardAddPanel(bMonitEcran,p,OPIUM_EN_DESSOUS_DE OPIUM_PRECEDENT);
	// BoardAreaEnd(bMonitEcran,&xt,&yt);
	BoardAreaCurrentPos(bMonitEcran,&xa,&ya,&xc,&yc);
	BoardAddBouton(bMonitEcran,L_("Sauver"),MonitEcranEcrit,((xa+xc)/2)-(3*Dx),yc+Dy,0);
	BoardAreaEnd(bMonitEcran,&xa,&ya); if(ya > yb) yb = ya;

	BoardAddBouton(bMonitEcran,L_("Rafraichir"),MonitEcranRefresh,(xa / 2) - (5 * Dx),yb + Dy,0);

	/* if(affiche) */ OpiumFork(bMonitEcran);
	return(0);
}
/* ========================================================================== */
int MonitSncfStart(Menu menu, MenuItem item) {
	char commande[1024]; int rc;

	if(OpiumReadInt("Via le port",&SambaMemPort) == PNL_CANCEL) return(0);
	sprintf(commande,"killall sncf; %ssncf %d &",ExecDir,SambaMemPort);
	if((rc = system(commande)) != 0) OpiumWarn("Le lancement du serveur a echoue (erreur %d)",rc);
	sprintf(commande,"ps | grep sncf | grep -v grep");
	if((rc = system(commande)) != 0) OpiumWarn("Le serveur a deja disparu (erreur %d)",rc);

	return(0);
}
/* ========================================================================== */
int MonitSncfOuvre(Menu menu, MenuItem item) {
	char commande[1024]; int rc;

	sprintf(commande,"open -a Safari http://localhost:%d",SambaMemPort);
	if((rc = system(commande)) != 0) OpiumWarn("Le serveur SNCF n'est pas operationnel (erreur %d)",rc);

	return(0);
}
/* ========================================================================== */
int MonitSncfStop(Menu menu, MenuItem item) {
	if(system("killall sncf")) OpiumNote(L_("Il n'y a plus de serveur SNCF actif"));
	return(0);
}
/* ========================================================================== */
int MonitRecupere() {
	if(OpiumReadFile("Depuis le fichier",FichierPrefMonit,MAXFILENAME) == PNL_CANCEL) return(0);
	if(ArgScan(FichierPrefMonit,MonitDesc) <= 0) {
		OpiumError("Lecture sur '%s' impossible",FichierPrefMonit);
	}
	MonitIntervSecs = (float)MonitIntervAff / 1000.0;
	MonitASauver = 0;
	return(0);
}
/* ========================================================================== */
int MonitSauve() {
    if(SettingsSauveChoix) {
        if(OpiumReadFile("Sauver sur le fichier",FichierPrefMonit,MAXFILENAME) == PNL_CANCEL) return(0);
    }
	if(ArgPrint(FichierPrefMonit,MonitDesc) > 0) {
		MonitASauver = 0;
		printf("%s/ Parametres d'affichage sauves dans %s\n",DateHeure(),FichierPrefMonit);
	} else OpiumError("Sauvegarde sur '%s' impossible",FichierPrefMonit);
	return(0);
}
/* ========================================================================== */
Menu mMonitUserDefine;

#ifdef MONIT_PAR_VOIE
	Menu mMonitParVoie;
	TypeMenuItem iMonitParVoie[] = {
		{ "Impression voie",     MNU_FONCTION MonitVoieImprime },
		{ "Affichage voie",      MNU_FONCTION MonitVoieAffiche },
		{ "Rafraichir",          MNU_FONCTION MonitVoieRafraichi },
		{ "Filtrage",            MNU_FONCTION MonitVoieFiltrage },
		{ "Tout effacer",        MNU_FONCTION MonitVoieEfface },
		{ "Fermer",              MNU_RETOUR },
		MNU_END
	};
#endif
#ifdef ANCIEN_MENU_AFFICHAGES
	TypeMenuItem iMonit[] = {
		{ "Parametres par defaut",  MNU_FONCTION MonitGeneral },
		{ "Graphiques utilisateur", MNU_MENU   &mMonitUserDefine },
	#ifdef MONIT_PAR_VOIE
		{ "Par voie",               MNU_MENU   &mMonitParVoie },
	#endif
	#ifdef BCP_EVTS
		{ "Parametres FFT",         MNU_PANEL  &pCalcSelPtNb },
	#endif
		{ "Evenements",             MNU_MENU   &mMonitEvts },
		{ "Ecran",                  MNU_FONCTION MonitEcranPanel },
	#ifdef MONIT_PAR_VOIE
		{ "Rafraichir",             MNU_FONCTION MonitRefresh }, // ???
	#endif
		{ "Preferences",            MNU_SEPARATION },
		{ "Recuperer",              MNU_FONCTION MonitRecupere },
		{ "Verifier",               MNU_PANEL  &pMonitDesc },
		{ "Sauver",                 MNU_FONCTION MonitSauve },
		{ "\0",                     MNU_SEPARATION },
		{ "Fermer",                 MNU_RETOUR },
		MNU_END
	};
#endif

int NumeriseurRapport(Menu menu, MenuItem item);
int CalcSpectreAutoRetrouve(Menu menu, MenuItem item);
TypeMenuItem iMonitBarre[] = {
	{ "Contenu de l'ecran",      MNU_FONCTION  MonitEcranPlanche },
//	{ "Ecran (panel)",           MNU_FONCTION  MonitEcranPanel },
	{ "Graphiques utilisateur",  MNU_MENU    &mMonitUserDefine },
//	{ "Voie par voie",           MNU_MENU    &mMonitParVoie },
	{ "Spectre du bruit",        MNU_FONCTION  MonitFftAuVol },
	{ "Suivi du gain",           MNU_MENU    &mMonitRaies },
	{ "Evenements",              MNU_MENU    &mMonitEvts },
	{ "Histogrammes predefinis", MNU_FONCTION  MonitHistosDef },
//	{ "Spectres sauves",         MNU_FONCTION  CalcSpectreAutoRetrouve }, deja dans "Procedures > bLectSpectres"
	{ "",                        MNU_SEPARATION },
	{ "Etat Repartiteurs",       MNU_FONCTION  RepartiteursStatus },
	{ "Etat Numeriseurs",        MNU_FONCTION  NumeriseurRapport },
	{ "Etat Detecteurs",         MNU_FORK    &bEtatDetecteurs }, // MNU_PANEL  &(pReglagesConsignes[LECT_CONS_ETAT]) },
	{ "Oscilloscope",            MNU_FONCTION  DetecteurOscillo },
	{ "Classes d'evenements",    MNU_FONCTION  MonitSuiviAffiche },
	{ "Generateur",              MNU_FONCTION  GegeneOpen },
	{ "Support  ",               MNU_FORK    &bAutomate },
	{ "Serveur http",            MNU_MENU    &mMonitSncf },
	{ "",                        MNU_SEPARATION },
	{ "Parametres par defaut",   MNU_FONCTION  MonitGeneral },
#ifdef BCP_EVTS
	{ "Parametres FFT",         MNU_PANEL   &pCalcSelPtNb },
#endif
	{ "Fermer",                 MNU_RETOUR },
	MNU_END
};
TypeMenuItem iMonitUserDefine[] = {
	{ "Lire definitions",     MNU_FONCTION MonitFenLit },
	{ "Modifications",        MNU_SEPARATION },
	{ "Creer fenetre",        MNU_FONCTION MonitFenEdite },
	{ "Editer fenetre",       MNU_FONCTION MonitFenEdite },
	{ "Dupliquer fenetre",    MNU_FONCTION MonitFenEdite },
	{ "Changer detecteur",    MNU_FONCTION MonitFenDetecteur },
	{ "Retirer fenetre",      MNU_FONCTION MonitFenRetire },
	{ "Lister fenetres",      MNU_FONCTION MonitFenListe },
	{ "Affichage",            MNU_SEPARATION },
	{ "Valeurs representees", MNU_FONCTION MonitFenZoom },
	{ "Autoriser affichage",  MNU_FONCTION MonitFenAutorise },
	{ "Dimensions affichage", MNU_FONCTION MonitFenDimensions },
	{ "Ajuster l'axe Y",      MNU_FONCTION MonitFenAjusteY },
	{ "Organiser",            MNU_FONCTION MonitFenEdite },
	{ "Etiquettes",           MNU_FONCTION MonitTagsManage },
	{ "Sauvegarde",           MNU_SEPARATION },
	{ "Memoriser position",   MNU_FONCTION MonitFenMemoToutes },
	{ "Sauver definitions",   MNU_FONCTION MonitFenEcrit },
	{ "Fermer",               MNU_RETOUR },
	MNU_END
};
TypeMenuItem iMonitRaies[] = {
	{ "Definition",          MNU_FONCTION   MonitRaiesDefini },
	{ "RAZ histo",           MNU_FONCTION   MonitRaiesRaz },
	{ "Fermer",              MNU_RETOUR },
	MNU_END
};
TypeMenuItem iMonitEvts[] = {
	{ "Mode d'affichage",     MNU_PANEL   &pMonitMode },
	{ "Etiquettes",           MNU_FONCTION MonitTagsManage },
	{ "Liste en memoire",     MNU_FONCTION MonitEvtListe },
	{ "Evenement No..",       MNU_FONCTION MonitEvtDemande },
	{ "Precedent",            MNU_FONCTION MonitEvtPrecedent },
	{ "Suivant",              MNU_FONCTION MonitEvtSuivant },
	{ "Evt moyen",            MNU_SEPARATION },
	{ "par voie",             MNU_FONCTION MonitEvtMoyenVoie },
	{ "par detecteur",        MNU_FONCTION MonitEvtMoyenBolo },
	{ "Fermer",               MNU_RETOUR },
	MNU_END
};
TypeMenuItem iMonitSncf[] = {
	{ "Demarrer",             MNU_FONCTION MonitSncfStart },
	{ "Ouvrir",               MNU_FONCTION MonitSncfOuvre },
	{ "Arreter",              MNU_FONCTION MonitSncfStop },
	{ "Fermer",               MNU_RETOUR },
	MNU_END
};

/* ========================================================================== */
void MonitMenuBarre() { mMonitBarre = MenuLocalise(iMonitBarre); }
/* ========================================================================== */
void MonitInit() {
	int voie;
	int x,y; int i,k;

/* Initialisation generale */
	MonitASauver = 0;
	LectCntl.MonitEvtNum = 0;
	for(voie=0; voie<VoiesGerees; voie++) {
		MonitEvtAb6[voie][0] = 0.0;  /* premier point de l'evenement */
		MonitEvtAb6[voie][1] = 1.0;  /* ou plutot, horloge (sera rectifie dans LectConstruitTampons) */
	}
	MonitEvtIndex[0] = 0; MonitEvtIndex[1] = 1;
	MonitBoloAvant = 0;
	MonitVoieVal0 = 0; MonitVoieValN = 0;
	strcpy(MonitNomTypes,MonitFenTypeName[0]);
	for(i=1; i<=MONIT_EVENT; i++) strcat(strcat(MonitNomTypes,"/"),MonitFenTypeName[i]); // MONIT_NBTYPES
	strcpy(MonitNouvelle,L_("(nouvelle fenetre)"));

/* Variables sujettes a modification par l'utilisateur (cf MonitDesc) */
	strcpy(MonitUser,L_("Graphiques"));
	MonitLarg = 400; MonitHaut = 250;      /* largeur/hauteur d'un graphique standard */
	MonitEvtCatgLarg = 200; MonitEvtCatgHaut = 125;   /* largeur/hauteur d'un graphique par categorie */
	MonitEvtFocsLarg = 400; MonitEvtFocsHaut = 250;   /* largeur/hauteur d'un graphique cible du suivi evts */
	MonitEvtClasses = 0;
	MonitEvtAvecRun = MonitModeAuto = 1; MonitModeSon = 0;
	MonitCategSuivi = MonitCategFocus = MonitCategSonore = -1;
	MonitModeYmin = -30000; MonitModeYmax = 30000;

	MonitDuree = 10.0;      /* duree affichee (ms)             */
//	MonitEvts = 0;          /* nb. evenements maxi affiches    */
	MonitSynchro = 1;

	MonitFftDim = 65536;
	MonitFftMode = CALC_SPTV_GLIS;
	MonitFftNb = 20;
	MonitFftNbFreq = 0;
	MonitFftCarreSomme = MonitFftMoyenne = 0;
	MonitFftCarreMoyenne = 0;
	MonitFftNouveau = 0;

	MonitHampl = MonitHmontee = MonitH2D = 0;
	MonitHamplY[0] = 0; MonitHamplY[1] = 10000;
	MonitHmonteeY[0] = 0; MonitHmonteeY[1] = 1;
	MonitH2DY[0] = 0;
	MonitH2DY[1] = 5;

	for(voie=0; voie<VoiesNb; voie++) VoieManip[voie].affiche = 0;
	MonitAffEvts = 1; MonitFigeFocus = MonitLogFocus = 0;
	MonitAffModes = MonitAffTrig = MonitAffpTaux = 0;
	MonitAffBases = MonitAffgSeuils = MonitAffEtat = MonitAffNet = 0;
	TousLesEvts.taux = TousLesEvts.evol = TousLesEvts.trace = 0;
	MonitIntervAff = 1000;
	MonitChgtUnite = MONIT_ADU;
	ValeurADU = 1.0; /* valeur d'un ADU dans l'unite ci-dessus */
	/* les gains plus amonts dependent de la voie/bolo, et encore, meme la polar des ADC...!
	pos_gain = Bolo[bolo].tension[CMDE_GAIN_VOIE1 + voie];
	if((pos_gain >= 0) && (pos_gain < 6)) sscanf(ReglGains[pos_gain],"%g",&(Bolo[bolo].captr[cap].bb.gain[voie]));
	else Bolo[bolo].captr[cap].bb.gain[voie] = 1.0; */

	PicSuiviDim = 1000;
	for(k=0; k<MAXPICS; k++) {
		Pic[k].min = 100; Pic[k].max = 500;
		Pic[k].t = 0; Pic[k].position = 0; Pic[k].sigma = 0;
		Pic[k].prems = 0; Pic[k].nb = Pic[k].dim = 0;
	}
	PicsNb = 0; PicsActif = 0;

/* Fenetres de trace des donnees brutes */
	MonitEditTraces = 1;
	MonitFenNb = 0; MonitFenNum = 0;
	strcpy(MonitFen[MonitFenNb].titre,MonitNouvelle);
	MonitFen[MonitFenNb + 1].titre[0] = '\0';
	for(i=0; i<MAXMONIT; i++) {
		MonitFen[i].incluse = 0;
		MonitFenTitre[i] = MonitFen[i].titre;
	}
	MonitFenDefault(&MonitFenLue,L_("(a definir!)"));

/* Lecture de la configuration (exceptionnellement en local, pour Monit) */
	if(SambaLogDemarrage) printf("= Lecture des affichages                dans '%s'\n",FichierPrefMonit);
	if(ArgScan(FichierPrefMonit,MonitDesc) <= 0) {
		if(CreationFichiers) ArgPrint(FichierPrefMonit,MonitDesc);
		else if(SambaLogDemarrage) printf("  ! Lecture sur '%s' impossible (%s)",FichierPrefMonit,strerror(errno));
	}
	SambaAjoutePrefPath(MonitFenFichier,MonitUser);
	MonitIntervSecs = (float)MonitIntervAff / 1000.0;
	if(ImprimeConfigs && SambaLogDemarrage) {
		printf("#\n### Fichier: '%s' ###\n",FichierPrefMonit);
		ArgPrint("*",MonitDesc);
	}
	if(ModeBatch) return;

/* Creation de l'interface */
#ifdef ANCIEN_MENU_AFFICHAGES
	mMonit = MenuLocalise(iMonit);
#endif
#ifdef MONIT_PAR_VOIE
	mMonitParVoie = MenuLocalise(iMonitParVoie);
#endif

	mMonitUserDefine = MenuIntitule(iMonitUserDefine,"Graphiques utilisateur");
	mMonitEvts = MenuIntitule(iMonitEvts,"Evenements");
	mMonitSncf = MenuIntitule(iMonitSncf,"SNCF");

	pMonitDesc = PanelDesc(MonitDesc,1);
	PanelTitle(pMonitDesc,"Preferences");
	PanelOnOK(pMonitDesc,MonitSauve,0);

//	pFichierMonitLect = PanelCreate(1);
//	PanelFile(pFichierMonitLect,"Depuis quel fichier",FichierPrefMonit,MAXFILENAME);
//	pFichierMonitEcri = PanelCreate(1);
//	PanelFile(pFichierMonitEcri,"Sur quel fichier",FichierPrefMonit,MAXFILENAME);

	pMonitGeneral = PanelCreate(6);
	PanelText (pMonitGeneral,L_("Jeu de fenetres"),MonitUser,16);
	PanelFloat(pMonitGeneral,L_("Duree affichee (ms)"),&MonitDuree);
	PanelKeyB (pMonitGeneral,L_("Unite"),MONIT_UNITE_CLES,&MonitChgtUnite,8);
	PanelFloat(pMonitGeneral,L_("Valeur d'un ADU en keV"),&MonitADUenkeV);
	PanelInt  (pMonitGeneral,L_("Hauteur graphique"),&MonitHaut);
	PanelInt  (pMonitGeneral,L_("Largeur graphique"),&MonitLarg);

	pMonitFenZoomData = PanelCreate(3);
	PanelTitle(pMonitFenZoomData,L_("Fenetre de donnees"));
	PanelFloat(pMonitFenZoomData,L_("Duree affichee (ms)"),&(MonitFenLue.axeX.r.max));
	PanelInt  (pMonitFenZoomData,L_("Y min"),&(MonitFenLue.axeY.i.min));
	PanelInt  (pMonitFenZoomData,L_("Y max"),&(MonitFenLue.axeY.i.max));

	pMonitFenZoomHisto = PanelCreate(3);
	PanelTitle(pMonitFenZoomHisto,L_("Histogramme"));
	PanelFloat(pMonitFenZoomHisto,L_("X min"),&(MonitFenLue.axeX.r.min));
	PanelFloat(pMonitFenZoomHisto,L_("X max"),&(MonitFenLue.axeX.r.max));
	PanelInt(pMonitFenZoomHisto,L_("#evts maxi"),&(MonitFenLue.axeY.i.max)); // MonitEvts);

	pMonitFenZoomFft = PanelCreate(4);
	PanelTitle(pMonitFenZoomFft,"FFT");
	PanelFloat(pMonitFenZoomFft,L_("Frequence min (0:auto)"),&(MonitFenLue.axeX.r.min));
	PanelFloat(pMonitFenZoomFft,L_("Frequence max (0:auto)"),&(MonitFenLue.axeX.r.max));
	PanelFloat(pMonitFenZoomFft,L_("Bruit min"),&(MonitFenLue.axeY.r.min));
	PanelFloat(pMonitFenZoomFft,L_("Bruit max"),&(MonitFenLue.axeY.r.max));

	pMonitFenZoomFctn = PanelCreate(3);
	PanelTitle(pMonitFenZoomFctn,L_("Fonction"));
	PanelInt  (pMonitFenZoomFctn,L_("Nb.evts affiches"),&(MonitFenLue.axeX.i.max));
	PanelFloat(pMonitFenZoomFctn,L_("Y min"),&(MonitFenLue.axeY.r.min));
	PanelFloat(pMonitFenZoomFctn,L_("Y max"),&(MonitFenLue.axeY.r.max));

	pMonitFenZoomH2D = PanelCreate(4);
	PanelTitle(pMonitFenZoomH2D,"Histo 2D");
	PanelFloat(pMonitFenZoomH2D,"X min",&(MonitFenLue.axeX.r.min));
	PanelFloat(pMonitFenZoomH2D,"X max",&(MonitFenLue.axeX.r.max));
	PanelFloat(pMonitFenZoomH2D,"Y min",&(MonitFenLue.axeY.r.min));
	PanelFloat(pMonitFenZoomH2D,"Y max",&(MonitFenLue.axeY.r.max));

	pMonitFenZoomBiplot = PanelCreate(4);
	PanelTitle(pMonitFenZoomBiplot,"Biplot");
	PanelFloat(pMonitFenZoomBiplot,"X min",&(MonitFenLue.axeX.r.min));
	PanelFloat(pMonitFenZoomBiplot,"X max",&(MonitFenLue.axeX.r.max));
	PanelFloat(pMonitFenZoomBiplot,"Y min",&(MonitFenLue.axeY.r.min));
	PanelFloat(pMonitFenZoomBiplot,"Y max",&(MonitFenLue.axeY.r.max));

	pMonitFenZoomEvts = PanelCreate(3);
	PanelTitle(pMonitFenZoomEvts,L_("Fenetre d'evenement"));
	PanelFloat(pMonitFenZoomEvts,L_("Duree affichee (ms)"),&(MonitFenLue.axeX.r.max));
	PanelInt  (pMonitFenZoomEvts,L_("Y min"),&(MonitFenLue.axeY.i.min));
	PanelInt  (pMonitFenZoomEvts,L_("Y max"),&(MonitFenLue.axeY.i.max));

	pMonitFenZoomMoyen = PanelCreate(2);
	PanelTitle(pMonitFenZoomMoyen,L_("Donnees moyennes"));
	PanelInt  (pMonitFenZoomMoyen,L_("Y min"),&(MonitFenLue.axeY.i.min));
	PanelInt  (pMonitFenZoomMoyen,L_("Y max"),&(MonitFenLue.axeY.i.max));

	pMonitFenUser = PanelCreate(1);
	PanelText(pMonitFenUser,"Nom du jeu de fenetres",MonitUser,MAXFILENAME);

	tMonit = TermCreate(48,110,16384);

	pMonitTraces = 0;
	MonitFenModifiees = 0;
	pMonitFenAffiche = 0;
	pMonitFenChange = 0;
	pMonitEcran = 0;
	bMonitEcran = BoardCreate();

	pMonitFen = PanelCreate(14);
	MonitFenPanel(0,0,0);

	pMonitMode = PanelCreate(9);
	PanelInt  (pMonitMode,L_("Largeur graphique"),&MonitLarg);
	PanelInt  (pMonitMode,L_("Hauteur graphique"),&MonitHaut);
	PanelBool (pMonitMode,L_("Sonorisation"),&MonitModeSon);
	PanelBool (pMonitMode,L_("Affichage par classe"),&MonitEvtClasses);
	PanelBool (pMonitMode,L_("Trace integree au run"),&MonitEvtAvecRun);
	PanelBool (pMonitMode,L_("Echelle Y automatique"),&MonitModeAuto);
	PanelSepare(pMonitMode,L_("Si echelle Y fixe"));
	PanelInt  (pMonitMode,"Y min",&MonitModeYmin);
	PanelInt  (pMonitMode,"Y max",&MonitModeYmax);
	PanelOnApply(pMonitMode,MonitChangeParms,0); PanelOnOK(pMonitMode,MonitChangeParms,0);

	pMonitBoloVoie = PanelCreate(2);
	PanelList(pMonitBoloVoie,L_("Nom du bolo"),BoloNom,&BoloNum,DETEC_NOM);
	PanelList(pMonitBoloVoie,L_("Nom de la voie"),ModeleVoieListe,&ModlNum,MODL_NOM);

	pMonitVoiePrint = PanelCreate(4);
	PanelList(pMonitVoiePrint,L_("Nom du bolo"),BoloNom,&BoloNum,DETEC_NOM);
	PanelList(pMonitVoiePrint,L_("Nom de la voie"),ModeleVoieListe,&ModlNum,MODL_NOM);
	PanelInt (pMonitVoiePrint,L_("Temps initial"),&MonitVoieVal0);
	PanelInt (pMonitVoiePrint,L_("Temps final"),&MonitVoieValN);

	pMonitLimites1Voie = PanelCreate(5);

/* Donnees brutes */
	for(voie=0; voie<VoiesNb; voie++) gDonnee[voie] = gFiltree[voie] = 0;

/* Evenements */
	/* cree aussi dans lect (=gEvtRunCtrl) pour inclusion dans la planche de commande */
	gEvtSolo = GraphCreate(MonitLarg,MonitHaut,6);
	OpiumTitle(gEvtSolo->cdr,L_("Dernier evenement"));
	x = GraphAdd(gEvtSolo,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitEvtAb6[0][0]),PointsEvt);
	y = GraphAdd(gEvtSolo,GRF_SHORT|GRF_YAXIS,VoieTampon[0].trmt[TRMT_AU_VOL].donnees.t.sval,PointsEvt);
	GraphDataName(gEvtSolo,x,"t(ms)");
	GraphDataRGB(gEvtSolo,y,GRF_RGB_GREEN);
#ifdef MONIT_EVT_TRAITE
	x = GraphAdd(gEvtSolo,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(MonitTrmtAb6[0][0]),PointsEvt);
	y = GraphAdd(gEvtSolo,GRF_FLOAT|GRF_YAXIS,VoieTampon[0].trmt[TRMT_PRETRG].donnees.t.rval,PointsEvt);
	GraphDataRGB(gEvtSolo,y,GRF_RGB_MAGENTA);
	GraphDataName(gEvtSolo,x,"t(ms)");
	GraphDataName(gEvtSolo,y,L_("filtree"));
#endif
#ifdef CARRE_A_VERIFIER
	x = GraphAdd(gEvtSolo,GRF_FLOAT|GRF_XAXIS,MomentEvt,5);
	y = GraphAdd(gEvtSolo,GRF_SHORT|GRF_YAXIS,AmplitudeEvt,5);
	GraphDataRGB(gEvtSolo,y,GRF_RGB_RED);
	GraphDataName(gEvtSolo,x,"t.trig");
	GraphDataName(gEvtSolo,y,"ADU.trig");
	GraphDataUse(gEvtSolo,x,5); GraphDataUse(gEvtSolo,y,5);
#endif
	OpiumPosition(gEvtSolo->cdr,50,100);
	GraphUse(gEvtSolo,0);

	gEvtMoyen = GraphCreate(MonitLarg,MonitHaut,2*VOIES_TYPES);
	OpiumTitle(gEvtMoyen->cdr,L_("Evenement moyen"));
	OpiumPosition(gEvtMoyen->cdr,50,100);
	GraphParmsSave(gEvtMoyen);
	GraphUse(gEvtMoyen,0);

	gPattern = GraphCreate(MonitLarg,MonitHaut,2*VOIES_TYPES);
	OpiumTitle(gPattern->cdr,"Pattern");
	OpiumPosition(gPattern->cdr,250,100);
	GraphParmsSave(gPattern);
	GraphUse(gPattern,0);

	MonitTagsInit();
	MonitEcranInit();

	/* spectre au vol */
	pMonitFftAuVol = PanelCreate(16);
	PanelTitle (pMonitFftAuVol,L_("Spectre moyenne sur..."));
	PanelList  (pMonitFftAuVol,L_("Nom du bolo"),BoloNom,&MonitFftBolo,DETEC_NOM);
	PanelList  (pMonitFftAuVol,L_("Nom de la voie"),ModeleVoieListe,&MonitFftModl,MODL_NOM);
	PanelList  (pMonitFftAuVol,L_("Donnees"),CalcDonneesName,&MonitFftDonnees,12);
	PanelInt   (pMonitFftAuVol,L_("Nombre de points"),&MonitFftDim);
	PanelKeyB  (pMonitFftAuVol,L_("Mode"),L_("manu/surv/auto/glissant"),&MonitFftMode,8);
	PanelInt   (pMonitFftAuVol,L_("Nb spectres si glissant"),&MonitFftNb);
	PanelRemark(pMonitFftAuVol,L_("----- Axe des X -----"));
	PanelKeyB  (pMonitFftAuVol,L_("type"),L_("lineaire/log"),&MonitFftParm[0].log,10);
	PanelKeyB  (pMonitFftAuVol,L_("limites"),L_("manu/auto"),&MonitFftParm[0].autom,6);
	PanelFloat (pMonitFftAuVol,L_("min si manu"),&MonitFftParm[0].lim.r.min);
	PanelFloat (pMonitFftAuVol,L_("max si manu"),&MonitFftParm[0].lim.r.max);
	PanelRemark(pMonitFftAuVol,L_("----- Axe des Y -----"));
	PanelKeyB  (pMonitFftAuVol,L_("type"),L_("lineaire/log"),&MonitFftParm[1].log,10);
	PanelKeyB  (pMonitFftAuVol,L_("limites"),L_("manu/auto"),&MonitFftParm[1].autom,6);
	PanelFloat (pMonitFftAuVol,L_("min si manu"),&MonitFftParm[1].lim.r.min);
	PanelFloat (pMonitFftAuVol,L_("max si manu"),&MonitFftParm[1].lim.r.max);

	/* suivi de pics */
	gPicsPositions = GraphCreate(400,MonitHaut,2*MAXPICS);
	OpiumTitle(gPicsPositions->cdr,L_("Positions des pics"));
	OpiumPosition(gPicsPositions->cdr,250,100);
	GraphParmsSave(gPicsPositions);
	GraphUse(gPicsPositions,0);
	gPicsSigmas = GraphCreate(400,MonitHaut,2*MAXPICS);
	OpiumTitle(gPicsSigmas->cdr,L_("Sigmas des pics"));
	OpiumPosition(gPicsSigmas->cdr,250,100+MonitHaut+40);
	GraphParmsSave(gPicsSigmas);
	GraphUse(gPicsSigmas,0);
	
}
#ifdef NTUPLES_ONLINE
/* ========================================================================== */
static void MonitNtupleCree() {
	int var_num,var_nb,var_max;
	int ntp_num,ntp_nb,ntp_lim;
	int voie,vm; char *nomvoie; char nomvar[80];

	if(CalcNtpDemande) {
		EvtEspace = PlotEspaceCree("Evenements",&NtUsed);
		ntp_nb = 2; /* 'Date' et 'Delai' */
		if(BolosUtilises > 1) ntp_nb++; /* 'Det' */
		if(CalcNtpDemande == NTP_MODELE) {
			var_nb = 0; for(vm=0; vm<ModeleVoieNb; vm++) if(ModeleVoie[vm].utilisee) var_nb++;
			var_max = ModeleVoieNb;
		} else /* (CalcNtpDemande == NTP_VOIE) */ {
			var_nb = VoiesNb; var_max = VoiesNb;
		}
		ntp_nb += (SAMBA_NTPVOIE_NB * var_nb); /* SAMBA_NTPVOIE_NB=9 variables par voie ou modele de voie dans un detecteur */
		ntp_lim = ntp_nb;
		ntp_nb++; /* pour la marque de fin de liste */
		printf("  > Creation du n-tuple 'Tango' de %d variable%s, dont %d par %svoie\n",Accord1s(ntp_lim),
			SAMBA_NTPVOIE_NB,(CalcNtpDemande == NTP_MODELE)?"modele de ": "");
		MonitNtp = (TypePlotVar *)malloc(ntp_nb * sizeof(TypePlotVar));
		if(MonitNtp) {
			ntp_num = 0;
			if((BolosUtilises > 1) && (ntp_num < ntp_nb)) PlotVarCree(&(MonitNtp[ntp_num++]),"Det","",&NtDet,EvtEspace);
			for(var_num=0; var_num<var_max; var_num++) {
				if(CalcNtpDemande == NTP_MODELE) {
					vm = var_num;
					if(!ModeleVoie[vm].utilisee) continue;
					nomvoie = ModeleVoie[vm].nom;
				} else /* (CalcNtpDemande == NTP_VOIE) */ {
					voie = var_num;
					if(VoiesNb > 1) { nomvoie = (BolosUtilises > 1)? VoieManip[voie].nom: VoieManip[voie].prenom; }
					else nomvoie = 0;
				}
				if(ntp_num >= ntp_lim) break;
				int ntp_type;
				for(ntp_type = 0; ntp_type<MONIT_NBVARS; ntp_type++) if(MonitNtpCalcule[ntp_type]) {
					MonitVarName(nomvoie,ntp_type,nomvar);
					MonitNtpNum[ntp_type][var_num] = ntp_num;
					PlotVarCree(&(MonitNtp[ntp_num++]),nomvar,VarUnit[ntp_type],&(NtVoie[ntp_type][var_num]),EvtEspace);
					if(ntp_num >= ntp_lim) break;
				}
			}
			if(ntp_num < ntp_lim) PlotVarCree(&(MonitNtp[ntp_num++]),L_("Date"),"s",&NtDate,EvtEspace);
			if(ntp_num < ntp_lim) PlotVarCree(&(MonitNtp[ntp_num++]),L_("Delai"),"s",&NtDelai,EvtEspace);
			MonitNtp[ntp_num].nom = "\0"; // (i < ntp_nb) oblige, sinon pas de fin de table, et plantage nanopaw
			PlotVarsDeclare(MonitNtp);
			PlotVarsInterface();
			if(PlotNtupleCree(SettingsNtupleMax,0.0)) NtDim = SettingsNtupleMax;
			MenuItemEnable(mCalc,(IntAdrs)"Graphiques evenements");
		}
	} else {
		printf("  > Pas de nTuple 'Tango'\n");
		MenuItemDisable(mCalc,(IntAdrs)"Graphiques evenements");
	}
	CalcNtpAffiche = CalcNtpDemande;
}
/* ========================================================================== */
void MonitNtupleDump() {
	int i = 0;

	printf("  * Variables d'analyse:");
	while(PlotVarList[i][0]) {
		if(!(i % 8)) printf("\n    . ");
		else if(i) printf(", ");
		printf("%s",PlotVarList[i]);
		i++;
	}
	printf("\n");
	printf("  * n-tuple 'Samba' complet:\n");
	i = 0;
	while(MonitNtp[i].nom[0]) {
		printf("    %2d [%08llX]: %s\n",i+1,(IntAdrs)MonitNtp[i].adrs,MonitNtp[i].nom);
		i++;
	}
}
/* ========================================================================== */
static void MonitNtupleSupprime() {
	printf("  > Supression du n-tuple 'Tango'\n");
	PlotVarsDeclare(0);
	if(MonitNtp) free(MonitNtp); MonitNtp = 0;
	PlotEspaceSupprime(EvtEspace); EvtEspace = 0;
	MenuItemDisable(mCalc,(IntAdrs)"Graphiques evenements");
	CalcNtpAffiche = 0;
}
/* ========================================================================== */
int MonitNtupleChange() {
	if(CalcNtpDemande != CalcNtpAffiche) MonitNtupleSupprime();
	if(CalcNtpDemande) MonitNtupleCree();
	return(1);
}
#endif /* NTUPLES_ONLINE */
/* ========================================================================== */
int MonitSetup() {
	int i,j,y,bolo; Panel p; Histo histo;

	MonitFftBolo = 0; for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire) { MonitFftBolo = bolo; break; }
	MonitFftModl = VoieManip[Bolo[MonitFftBolo].captr[0].voie].type;

	if(PlotInit()) {
		int qual;
		strcpy(&(MonitColrFig[WND_Q_ECRAN][0]),"0000FFFF0000");
		strcpy(&(MonitColrFit[WND_Q_ECRAN][0]),"FFFFFFFF0000");
		strcpy(&(MonitColrFig[WND_Q_PAPIER][0]),"00000000BFFF");
		strcpy(&(MonitColrFit[WND_Q_PAPIER][0]),"BFFF00007FFF");
		for(qual=0; qual<WND_NBQUAL; qual++) PlotColors(qual,&(MonitColrFig[qual][0]),&(MonitColrFit[qual][0]));
		PlotInterface(0,1);
		SambaAjouteTopDir(PrefsTango,TangoDir);
		SambaCreePath(TangoDir,1,AnaDir,AnaPath);
		SambaCreePath(AnaPath,1,CoupeDir,CoupePath);
		PlotNtupleCoupePath(CoupePath);
		PlotBuildUI();
	} else OpiumError("Parametrisation des histos impossible");

	NtUsed = NtDim = 0;
#ifdef NTUPLES_ONLINE
	MonitNtupleCree(); /* PlotVarList cree dans MonitNtupleCree() via PlotVarsDeclare() */
	ArgDesc *desc;
	desc = &(MonitTraceXntpDesc[0]);
	ArgAdd(desc++,0    ,DESC_LISTE PlotVarList,&(MonitFenLue.trace[0].var),0);
	ArgAdd(desc++,"min",DESC_FLOAT &(MonitFenLue.axeX.r.min), 0);
	ArgAdd(desc++,"max",DESC_FLOAT &(MonitFenLue.axeX.r.max), 0);
	desc = &(MonitTraceYntpDesc[0]);
	ArgAdd(desc++,0    ,DESC_LISTE PlotVarList,&(MonitFenLue.trace[1].var),0);
	ArgAdd(desc++,"min",DESC_FLOAT &(MonitFenLue.axeY.r.min), 0);
	ArgAdd(desc++,"max",DESC_FLOAT &(MonitFenLue.axeY.r.max), 0);
#endif

	SambaAjoutePrefPath(MonitFenFichier,MonitUser);
	MonitFenDecode();

	pMonitFenDetecteur = PanelCreate(2);
	PanelList(pMonitFenDetecteur,L_("Detecteur abandonne"),BoloNom,&MonitBoloAvant,DETEC_NOM);
	PanelList(pMonitFenDetecteur,L_("Detecteur a utiliser"),BoloNom,&BoloNum,DETEC_NOM);

	gMonitFftAuVol = 0;

	mMonitRaies = MenuLocalise(iMonitRaies);
	pMonitRaies = PanelCreate(5);
	if(BoloNb > 1) {
		i = PanelList(pMonitRaies,L_("Detecteur"),BoloNom,&BoloNum,DETEC_NOM);
		PanelItemExit(pMonitRaies,i,DetecteurListeCapteurs,0);
	}
	if((BoloNb > 1) || (Bolo[BoloNum].nbcapt > 1)) {
		PanelList(pMonitRaies,L_("Capteur"),CapteurNom,&CapteurNum,MODL_NOM);
	}
	PanelInt  (pMonitRaies,L_("Max evts montres"),&PicSuiviDim);
	PanelInt  (pMonitRaies,L_("Nombre de bins"),&SettingsAmplBins);
	PanelInt  (pMonitRaies,L_("Pics a suivre"),&PicsNb);

	pMonitDivers = 0;

	/* Histos predefinis */
	pMonitHistosDef = PanelCreate(14);
	PanelTitle(pMonitHistosDef,L_("Definition des histos"));
	PanelList (pMonitHistosDef,L_("Nom du detecteur"),BoloNom,&BoloNum,DETEC_NOM);
	PanelList (pMonitHistosDef,L_("Nom de la voie"),ModeleVoieListe,&ModlNum,MODL_NOM);
	PanelSepare(pMonitHistosDef,L_("Amplitude"));
	PanelBool (pMonitHistosDef,L_("Mis a jour"),&TrmtHampl);
	PanelFloat(pMonitHistosDef,L_("Amplitude mini"),&SettingsAmplMin);
	PanelFloat(pMonitHistosDef,L_("Amplitude maxi"),&SettingsAmplMax);
	PanelItemSelect(pMonitHistosDef,PanelInt  (pMonitHistosDef,L_("Nombre de bins"),&SettingsAmplBins),0);
	PanelSepare(pMonitHistosDef,L_("Temps de montee"));
	PanelBool (pMonitHistosDef,L_("Mis a jour"),&TrmtHmontee);
	PanelFloat(pMonitHistosDef,L_("Tps montee mini"),&SettingsMonteeMin);
	PanelFloat(pMonitHistosDef,L_("Tps montee maxi"),&SettingsMonteeMax);
	PanelItemSelect(pMonitHistosDef,PanelInt  (pMonitHistosDef,L_("Nombre de bins"),&SettingsMonteeBins),0);
	PanelSepare(pMonitHistosDef,L_("Ampl. x Tps montee"));
	PanelBool (pMonitHistosDef,L_("Mis a jour"),&TrmtH2D);

	pMonitHistosDyn = p = PanelCreate(9);
	PanelSepare(pMonitHistosDyn,L_("Amplitude"));
	j = PanelInt(pMonitHistosDyn,L_("Nb evts mini"),&(MonitHamplY[0])); PanelItemLngr(p,j,6);
	j = PanelInt(pMonitHistosDyn,L_("Nb evts maxi"),&(MonitHamplY[1])); PanelItemLngr(p,j,6);
	PanelSepare(pMonitHistosDyn,L_("Temps de montee"));
	j = PanelInt(pMonitHistosDyn,L_("Nb evts mini"),&(MonitHmonteeY[0])); PanelItemLngr(p,j,6);
	j = PanelInt(pMonitHistosDyn,L_("Nb evts maxi"),&(MonitHmonteeY[1])); PanelItemLngr(p,j,6);
	PanelSepare(pMonitHistosDyn,L_("Ampl. x Tps montee"));
	j = PanelInt(pMonitHistosDyn,L_("Nb evts mini"),&MonitH2DY[0]); PanelItemLngr(p,j,6);
	j = PanelInt(pMonitHistosDyn,L_("Nb evts maxi"),&MonitH2DY[1]); PanelItemLngr(p,j,6);
	
	/* creation de l'histo amplitude */
	histo = HistoCreateFloat(SettingsAmplMin,SettingsAmplMax,SettingsAmplBins);
/*--	histo->r = histo->g = histo->b = 0; */
	hdBruitAmpl = HistoAdd(histo,HST_INT);
	hdEvtAmpl = HistoAdd(histo,HST_INT);
	HistoDataSupportRGB(hdBruitAmpl,WND_Q_ECRAN,GRF_RGB_BLUE);
	HistoDataSupportRGB(hdEvtAmpl,WND_Q_ECRAN,GRF_RGB_YELLOW);
	gBruitAmpl = GraphCreate(MonitLarg,MonitHaut,5);
	OpiumTitle(gBruitAmpl->cdr,"Amplitude");
	HistoGraphAdd(histo,gBruitAmpl);
	GraphAxisFloatRange(gBruitAmpl,GRF_XAXIS,SettingsAmplMin,SettingsAmplMax);
	/* coupure amplitude */
	GraphAdd(gBruitAmpl,GRF_XAXIS|GRF_FLOAT,CoupureAmpl,2);
	y = GraphAdd(gBruitAmpl,GRF_YAXIS|GRF_INT,MonitHamplY,2);
	GraphDataRGB(gBruitAmpl,y,GRF_RGB_RED);
	OpiumPosition(gBruitAmpl->cdr,20,500);

	/* creation de l'histo temps de montee */
	histo = HistoCreateFloat(SettingsMonteeMin,SettingsMonteeMax,SettingsMonteeBins);
	hdBruitMontee = HistoAdd(histo,HST_INT);
	hdEvtMontee = HistoAdd(histo,HST_INT);
	HistoDataSupportRGB(hdBruitMontee,WND_Q_ECRAN,GRF_RGB_BLUE);
	HistoDataSupportRGB(hdEvtMontee,WND_Q_ECRAN,GRF_RGB_YELLOW);
	gBruitMontee = GraphCreate(MonitLarg,MonitHaut,5);
	OpiumTitle(gBruitMontee->cdr,L_("Temps de montee (us)"));
	HistoGraphAdd(histo,gBruitMontee);
	GraphAxisFloatRange(gBruitMontee,GRF_XAXIS,SettingsMonteeMin,SettingsMonteeMax);
	/* coupure temps de montee */
	/* cMontee = CoupureCreate(HST_FLOAT,HST_INT);
		HistoYrangeInt(histo,0,500000);
		CoupureGraphAdd(gBruitMontee,cMontee); */
	GraphAdd(gBruitMontee,GRF_XAXIS|GRF_FLOAT,CoupureMontee,2);
	y = GraphAdd(gBruitMontee,GRF_YAXIS|GRF_INT,MonitHmonteeY,2);
	GraphDataRGB(gBruitMontee,y,GRF_RGB_RED);
	OpiumPosition(gBruitMontee->cdr,320,500);

	MonitLUTall = GraphLUTCreate(OPIUM_LUT_DIM,GRF_LUT_ALL);
	MonitLUTred = GraphLUTCreate(OPIUM_LUT_DIM,GRF_LUT_RED);

	h2D = H2DCreateFloatFloatToInt(
		SettingsAmplMin,SettingsAmplMax,SettingsAmplBins,
		SettingsMonteeMin,SettingsMonteeMax,SettingsMonteeBins);
	H2DLUT(h2D,MonitLUTred,OPIUM_LUT_DIM);
	g2D = GraphCreate(MonitLarg,MonitHaut,7);
	OpiumTitle(g2D->cdr,L_("Tps montee x Ampl"));
	H2DGraphAdd(h2D,g2D);
	/* coupure amplitude */
	GraphAdd(g2D,GRF_XAXIS|GRF_FLOAT,Coup2DAmplX,2);
	y = GraphAdd(g2D,GRF_YAXIS|GRF_FLOAT,Coup2DAmplY,2);
	GraphDataRGB(g2D,y,GRF_RGB_RED);
	/* coupure temps de montee */
	GraphAdd(g2D,GRF_XAXIS|GRF_FLOAT,Coup2DMonteeX,2);
	y = GraphAdd(g2D,GRF_YAXIS|GRF_FLOAT,Coup2DMonteeY,2);
	GraphDataRGB(g2D,y,GRF_RGB_RED);
	OpiumPosition(g2D->cdr,20,520);

	return(1);
}
/* ========================================================================== */
int MonitExit() {
	if(!SambaSauve(&MonitASauver,L_("affichage general"),-1,0,MonitSauve)) return(0);
	if(!SambaSauve(&MonitCatgModifie,L_("affichage evenements"),-1,0,MonitEcranEcrit)) return(0);
	if(!SambaSauve(&MonitTagModifie,L_("definition des etiquettes"),1,0,MonitTagsEcrit)) return(0);
	return(SambaSauve(&MonitFenModifiees,L_("graphiques utilisateur"),0,1,MonitFenEcrit));
}
