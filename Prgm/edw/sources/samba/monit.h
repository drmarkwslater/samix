/*
 *  monit.h
 *  Samba
 *
 *  Created by Michel GROS on 18.07.12.
 *  Copyright 2012 CEA/DSM/DAPNIA. All rights reserved.
 *
 */

#ifndef MONIT_H
#define MONIT_H

#ifdef MONIT_C
#define MONIT_VAR
#else
#define MONIT_VAR extern
#endif

// #include <rundata.h>
// #include <samba.h>
#include <edw_types.h>

typedef enum {
	MONIT_SIGNAL = 0,
	MONIT_HISTO,
	MONIT_FFT,
	MONIT_FONCTION,
	MONIT_2DHISTO,
	MONIT_BIPLOT,
	MONIT_EVENT,
	MONIT_MOYEN,
	MONIT_PATTERN,
	MONIT_NBTYPES
} MONIT_TYPE;

MONIT_VAR char *MonitFenTypeName[MONIT_NBTYPES+1]
#ifdef MONIT_C
 = {
	"signal", "histo", "fft", "fonction", "2d-histo", "biplot", "evenement", "evt moyen", "pattern", "\0"
 }
#endif
;

MONIT_VAR char MonitNtpCalcule[MONIT_NBVARS]
#ifdef MONIT_C
 = { 1, 1, 0, 0,  1, 1, 0, 1,  1, 1, 1, 1,   1, 1, 1, 1, 1,  0 }
#endif
;
MONIT_VAR int MonitNtpNum[MONIT_NBVARS][VOIES_MAX];

typedef enum {
	MONIT_IMPR_BOLOS_BB1 = 0,
	MONIT_IMPR_BOLOS_MIXTE,
	MONIT_NBIMPR_BOLOS
} MONIT_IMPR_BOLOS;

typedef enum {
	MONIT_ADU = 0,
	MONIT_ENTREE_ADC,
	MONIT_ENTREE_BB,
	MONIT_SORTIE_BOLO,
	MONIT_KEV,
	MONIT_NBUNITES
} MONIT_UNITE;
#define MONIT_UNITE_CLES "ADU/mV ADC/mV BB/nV detec/keV"

#ifdef INUTILISE
	typedef enum {
		MONIT_EVTCOURANT = 0,
		MONIT_EVTJUSTE,
		MONIT_NBEVTS
	} MONIT_EVT;
	MONIT_VAR char *MonitFenEvtName[MONIT_NBEVTS+1]
	#ifdef MONIT_C
	= { "courant", "ajuste", "\0" }
	#endif
	;
#endif /* INUTILISE */

typedef enum {
	MONIT_HIDE = 0,
	MONIT_SHOW,
	MONIT_ALWS,
	MONIT_FOLW,
	MONIT_FEN_NBDISPLAYS
} MONIT_FEN_DISPLAY;
#define MONIT_DISPLAY_CLES "non/oui/tjrs/suivi"

typedef struct {
	float r,g,b;
} TypeMonitRgb;

typedef struct {
//	short bolo,cap,voie;
	int bolo;
	int cap;
	int voie;
	int var;        /* 0: amplitude / 1: montee / etc...  ou 0: brutes / 1: traitees */
	short numx;     /* numero de donnee effectif dans le graphe                      */
	char ajuste;    /* vrai si ajustement automatique du graphique demande           */
	char voie_lue[MODL_NOM];
	char *liste_voies[DETEC_CAPT_MAX+1];
	TypeMonitRgb rgb;
	OpiumColor couleur;
} TypeMonitTrace;

typedef struct {
	int pts;
	union {
		struct {
			int min;             /* ADU                                 */
			int max;             /* ADU                                 */
		} i;
		struct {
			float min;           /* FFT: nV/rac(Hz)                     */
			float max;           /* FFT: nV/rac(Hz)                     */
		} r;
	};
} TypeMonitAxe;

#define MONIT_FENTITRE 80
typedef struct {
	char titre[MONIT_FENTITRE];
	char type;      /* DonneesBrutes, Evenement, Histo ou FFT  */ // unsigned?
	char affiche;            /* affichage demande                       */
	char grille;             /* grille demandee                         */
	char change;             /* changement de detecteur demande         */
	char incluse;            /* incluse dans la planche de suivi        */
	char nom_catg[CATG_NOM]; /* classe d'evt en toutes lettres          */
	int  catg;               /* classe d'evt concernee                  */
	int  dernier_evt;        /* dernier evenement utilise [BIPLOT]      */
	int posx,posy,larg,haut; /* position et dimensions                  */
	TypeMonitAxe axeX,axeY;
	int nb;                  /* nombre de traces                        */
	TypeMonitTrace trace[MAXTRACES];
	Graph g;
	int pts;                 /* #bins (histo) ou echantillons (fft)     */
	union {
		struct {
			Histo histo;
			HistoData hd[MAXTRACES];
		} h;                     /* (propre au type 'histo' et 'fonction') */
		struct { H2D histo; } s; /* (propre au type 'histo 2D')         */
		struct { int nb; } b;    /* (propre au type 'biplot')         */
		struct {
			char excl;           /* 0:indifferent, 1:exclusion evts     */
			char xlog;
			int intervalles;
//-			float min;
			float freq[MAXTRACES][2];
			float *spectre[MAXTRACES];
		} f;                     /* (propre au type 'fft')              */
	} p;
} TypeMonitFenetre;

#define CATG_ALL "tous"
typedef struct {
	char nom[CATG_NOM];
	char num,utilisee,taux,evol,trace,deconv,autoscale,scalechge;
	int  min,max;
	int64 stamp_utilise; /* stamp utilise pour le dernier calcul du taux */
	int64 stamp_trouve; /* dernier stamp trouve pour calculer le delai entre evenements */
	int trouves,utilises,affichable,affiche;
	float delai,freq;
	float top;
	float charge,duree;
	float prelim,avance;
	TypeTamponFloat activite;
	Panel pTaux,pMinMax,pEvtval;
	Graph gSuivi,gEvt;
} TypeMonitCatg;
MONIT_VAR TypeMonitCatg MonitCatg[CATG_MAX],TousLesEvts,*MonitCatgPtr[CATG_MAX+1];
MONIT_VAR char *MonitCatgNom[CATG_MAX+2];
MONIT_VAR int MonitCatgNb;
MONIT_VAR int MonitCategSuivi,MonitCategFocus,MonitCategSonore;

typedef struct {
	char nom[CATG_NOM];
	char p; char sonorisee;
	OpiumColor c;
	char son[MAXFILENAME];
	int64 duree_us;
} TypeTagDisplay;
MONIT_VAR TypeTagDisplay TagDef[CATG_MAX];
MONIT_VAR int TagDefNb;
MONIT_VAR OpiumColorState TagCouleur[VOIES_MAX];
MONIT_VAR int64 TagPlayEnd;

MONIT_VAR int MonitFenNb,MonitFenNum;
MONIT_VAR TypeMonitFenetre MonitFen[MAXMONIT+2],MonitFenLue,*MonitFenEnCours;
MONIT_VAR char *MonitFenTitre[MAXMONIT+2];
MONIT_VAR char  MonitAffModes,MonitAffTrig,MonitAffEtat,MonitAffNet;
MONIT_VAR char  MonitAffBases,MonitAffgSeuils;
MONIT_VAR Panel pMonitBoloVoie;
MONIT_VAR float MonitEvtAmpl,MonitEvtMontee;
MONIT_VAR float MonitADUenmV;                      /* Valeur d'1 ADU en mV a l'entree de l'ADC         */
MONIT_VAR float MonitADUenkeV;                     /* Valeur d'1 ADU en keV                            */
MONIT_VAR float MonitIntervSecs;
MONIT_VAR int   MonitIntervAff;
MONIT_VAR char  MonitAffEvts;                      /* Affichage du dernier evenement                   */
MONIT_VAR char  MonitFigeFocus;                    /* Fige l'affichage du focus sur PlancheEvts        */
MONIT_VAR char  MonitLogFocus;                     /* Echelle log pour Y                               */
MONIT_VAR char  MonitAffpTaux;                     /* Affichage du panel des taux d'evenements         */
MONIT_VAR char  MonitAffgTaux;                     /* Affichage du graphique des taux d'evenements     */
MONIT_VAR float MonitT0;                           /* Origine des temps pour l'affichage               */
MONIT_VAR int64 MonitP0;                           /* Timestamp correspondant                          */
MONIT_VAR char  MonitSynchro;                      /* Synchronisation de l'affichage sur la modulation */
MONIT_VAR char  MonitEvtClasses;                   /* Planche de visualisation des categories d'evts   */
MONIT_VAR char  MonitChgtUnite;                    /* Changement d'unite: ADU -> Volts ou keV          */
MONIT_VAR char  MonitEvtAvecRun;                   /* Affichage de l'evt dans la planche Run (sinon, une fenetre detachee) */
MONIT_VAR char  MonitModeAuto;                     /* axe Y autoscale (sinon, extremas fixes) */
MONIT_VAR char  MonitModeSon;                      /* emission d'un son a chaque evt          */
MONIT_VAR int   MonitModeYmin,MonitModeYmax;       /* extremas axe Y si !MonitModeAuto */
MONIT_VAR int   MonitHaut,MonitLarg;               /* utilises dans monit.c, settings.c et calculs.c */
MONIT_VAR int   MonitEvtCatgLarg,MonitEvtCatgHaut; /* utilises dans la planche des evenements */
MONIT_VAR int   MonitEvtFocsLarg,MonitEvtFocsHaut; /* utilises dans la planche des evenements */
MONIT_VAR char  MonitHampl,MonitHmontee,MonitH2D;
MONIT_VAR float MonitEvtAb6[VOIES_MAX][2],MonitTrmtAb6[VOIES_MAX][2];
MONIT_VAR int   MonitEvtAlEcran;
MONIT_VAR float CoupureAmpl[2];   MONIT_VAR int   MonitHamplY[2];
MONIT_VAR float CoupureMontee[2]; MONIT_VAR int   MonitHmonteeY[2];
MONIT_VAR float Coup2DAmplX[2];   MONIT_VAR float Coup2DAmplY[2];
MONIT_VAR float Coup2DMonteeX[2]; MONIT_VAR float Coup2DMonteeY[2];
MONIT_VAR int   MonitH2DY[2];
MONIT_VAR Panel pMonitDesc;

int MonitChoisiBoloVoie(int *voie_choisie);
int MonitFenMemo(TypeMonitFenetre *f);
Graph MonitFenCreeGraphe(TypeMonitFenetre *f, char nouveau);
int MonitFenAffiche(TypeMonitFenetre *f, char reellement);
void MonitFenClear(TypeMonitFenetre *f);
void MonitFenDetach(TypeMonitFenetre *f);
float MonitUnitesADU(Graph g, int sens, float val);
int MonitEcranEcrit();
int MonitEvtAffiche(TypeMonitCatg *categ_demandee, int lequel, void *qui, int affiche);
int MonitEvtPrecedent(Menu menu, MenuItem item);
int MonitEvtSuivant(Menu menu, MenuItem item);
int MonitSauve();

#endif
