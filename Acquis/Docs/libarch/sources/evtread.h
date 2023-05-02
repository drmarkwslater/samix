#ifndef EVTREAD_H
#define EVTREAD_H

#include <unistd.h>
// #ifndef CMDE_UNIX
#include <dateheure.h>
// #endif

#undef EVTREAD_VAR
#ifdef EVTREAD_C
	#define EVTREAD_VAR
#else
	#define EVTREAD_VAR extern
#endif

typedef enum {
	DETEC_ARRETE = 0,
	DETEC_EN_SERVICE,
	BN_AUXILIAIRE,
	DETEC_ETATS_NB
} DETEC_ETATS;

EVTREAD_VAR int TrancheRelue;
EVTREAD_VAR char EvtAuPif,Discontinu;
EVTREAD_VAR int EvtRunNb;
EVTREAD_VAR int EvtNb,EvtLus,EvtDejaPresents;
EVTREAD_VAR int EvtRunNum;
EVTREAD_VAR int EvtOpenNb;
EVTREAD_VAR int EvtOpenMax;
EVTREAD_VAR int EvtsMax;
EVTREAD_VAR int EvtStockNb;
EVTREAD_VAR int EvtsParFichier;
EVTREAD_VAR off_t EvtDim;
EVTREAD_VAR char *EvtSel;
EVTREAD_VAR off_t *EvtPos;
EVTREAD_VAR off_t OctetsDejaLus;
#ifdef DATEHEURE_H
	EVTREAD_VAR TypeS_us EvtRunT0;
	EVTREAD_VAR TypeS_us *EvtDateExacte;
#endif
EVTREAD_VAR int EvtAffiche;
EVTREAD_VAR int RunTempsInitial,RunTempsTotal;

#define VOIES_MAXI 32
EVTREAD_VAR char *VoieListe[VOIES_MAXI+1];

#define MAXFILENAME 256
#define MAXFICHIERS 2048
typedef struct {
	char nom[MAXFILENAME];
	char fichier[80];
	int path;
	off_t debut,taille;
	int premier;
	int num0,numN;
	int t0sec,t0msec;
} TypeEvtRun;
EVTREAD_VAR TypeEvtRun EvtRunInfo[MAXFICHIERS + 1];
EVTREAD_VAR char FichierNtuple[MAXFILENAME],FichierSupp[MAXFILENAME];

/* Affichage des evenements */
typedef struct {
	int max; /* nombre de couleurs (2,4,8,16,..) */
	int num; /* numero de couleur (1,3,5,..,max-1) */
} EvtCouleur;
typedef struct {
	int index;
	TypeEvt evt;
	TypeVoieDonnees *signaux;
	Graph g;
	EvtCouleur couleur;
	int nb_traces;
} TypeEvtStocke;
EVTREAD_VAR TypeEvtStocke **EvtStock,EvtStockLu;

char ArchReadInit(int max);
char ArchEvtKeepFree();
char ArchEvtFilesFull();
char ArchRunOpen(char *nom, char log, char *raison);
char ArchRunOpenRange(char *nom, int premiere, int derniere, char log, char *raison);
void ArchRunClose();
int  ArchEvtSaved(int evt);
char ArchEvtRestore(int i);
char ArchEvtGet(int num, char *explic);

#endif
