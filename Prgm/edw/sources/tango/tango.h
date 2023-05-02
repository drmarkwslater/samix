#ifndef TANGO_H
#define TANGO_H

#ifdef CODE_WARRIOR_VSN
#include <stat.h>  /* pour avoir off_t */
#include <nanopaw.h>
#else
#include <sys/stat.h>  /* pour avoir off_t */
#include <opium4/nanopaw.h>
#endif

#ifdef TANGO_C
#define TANGO_VAR
#else
#define TANGO_VAR extern
#endif

#define MAXFILTRES 16

typedef enum {
	TANGO_BYTE,
	TANGO_UBYTE,
	TANGO_SHORT,
	TANGO_USHORT,
	TANGO_INT,
	TANGO_UINT,
	TANGO_LONG,
	TANGO_ULONG,
	TANGO_FLOAT,
	TANGO_NBTYPES
} TANGO_TYPE;

#ifdef BOLOMETRES
	#define NOM_CHALEUR "chaleur"
	#define NOM_CENTRE "centre"
	#define NOM_GARDE "garde"
	#define NOM_RAPIDE "rapide"
#endif

extern char *TangoVersion,*TangoCompile;

TANGO_VAR char   XcodeTangoDebug;
TANGO_VAR char   AutoriseOpium;
TANGO_VAR int    Dx,Dy;
TANGO_VAR Panel pHdrRun,pHdrEvt,pEvtAnalyses;
// TANGO_VAR Panel pEvtUniteFichier;
TANGO_VAR char   EvtTempsName[MAXFILENAME];
TANGO_VAR char   GrafPath[MAXFILENAME],FichierFiltres[MAXFILENAME];
TANGO_VAR char   CalibPath[MAXFILENAME],UniteEventFile[MAXFILENAME],UniteTempsFile[MAXFILENAME];
TANGO_VAR char   TangoColrFig[WND_NBQUAL][16],TangoColrFit[WND_NBQUAL][16];
TANGO_VAR Menu  mNtpSauve;
TANGO_VAR Panel pQualiteGraph;
TANGO_VAR Cadre cTangoAnalyse;
TANGO_VAR char   AjouteNtuple,SelecteVar,SelecteEvt;
TANGO_VAR int    ProdTauxNb,ProdTauxMax;
TANGO_VAR float *ProdTauxTab;

#define MAXSELECTIONS 16
typedef struct {
	unsigned char oper;
	unsigned char marque;
	int var;
	float min,max;
	int valides,total;
} TypeCoupure;
TANGO_VAR TypeCoupure Coupure[MAXSELECTIONS];

TANGO_VAR int    EspaceNum;

TANGO_VAR PlotEspace EvtEspace;
TANGO_VAR int    PlotNtupleValides,EvtDemande,EvtsAnalyses;
TANGO_VAR int    EventPartDebut,EventPartFin;
TANGO_VAR float  EvtUniteMont,EvtUniteDesc;

TANGO_VAR PlotEspace PaqEspace;
TANGO_VAR int *PaqEvtPremier;
TANGO_VAR int PaqNb;

#define PRODVAR_MAX 16
#define PRODVAR_NOM 16
typedef struct {
    char nom[PRODVAR_NOM];
    int var1,var2;
    char oper,niveau;  // niveau de detail, voir PRODVAR_NIV
    char num;          // numero de la variable du niveau specifie
    char a_calculer;
} TypeProdVar;
TANGO_VAR TypeProdVar ProdVar[PRODVAR_MAX],ProdVarAjoutee;
TANGO_VAR int ProdVarNb,ProdVarNum;
TANGO_VAR char *ProdVarNom[PRODVAR_MAX+1];
TANGO_VAR Panel pProdVarDef,pProdVarMod,pProdVarQui;
TANGO_VAR char Opers[8]
#ifdef TANGO_C
 = "+/-/*/:"
#endif
;
typedef enum {
    PRODVAR_DETEC = 0,
    PRODVAR_VOIE,
    PRODVAR_NIV
} PRODVAR_NIVEAUX;
TANGO_VAR char Niveaux[16]
#ifdef TANGO_C
 = "detecteur/voie"
#endif
;
TANGO_VAR char ProdVarDetecNb,ProdVarVoieNb;

TANGO_VAR ArgDesc EvtUniteTempsDesc[]
#ifdef TANGO_C
 = {
	{ "duree.montee",   DESC_FLOAT &EvtUniteMont, "Duree utilisee avant le maximum (ms)" },
	{ "duree.descente", DESC_FLOAT &EvtUniteDesc, "Duree utilisee apres le maximum (ms)" },
	DESC_END
}
#endif
;


#ifdef WIN32
extern WndIdent WndRoot;
#endif

TANGO_VAR Menu mProdDialogue,mProdAffGeneral,mProdAffEvts,mEvts,mProdExperte;
TANGO_VAR OpiumColor TangoVertRouge[]
#ifdef TANGO_C
 = { { GRF_RGB_GREEN }, { GRF_RGB_RED } }
#endif
;
TANGO_VAR OpiumColor TangoRougeVertJaune[]
#ifdef TANGO_C
 = { { GRF_RGB_RED }, { GRF_RGB_GREEN }, { GRF_RGB_YELLOW } }
#endif
;
TANGO_VAR OpiumColor TangoRougeOrangeJauneVert[]
#ifdef TANGO_C
 = { { GRF_RGB_RED }, { GRF_RGB_ORANGE }, { GRF_RGB_YELLOW }, { GRF_RGB_GREEN } }
#endif
;
TANGO_VAR OpiumColor TangoBlancNoir[]
#ifdef TANGO_C
= { { GRF_RGB_WHITE }, { GRF_RGB_BLACK } }
#endif
;

// #define printf LogBook

// #define NTP_PROGRES_INTEGRE
#ifdef NTP_PROGRES_INTEGRE
void TangoProdReset(int total);
char TangoProdContinue(int actuel);
#endif

FILE *TangoFileOpen(char *nom, char *mode);
void TangoCreeCoupureDesc();
void TangoCreePanelsVars();

void PlotPrint(char niveau, char *fmt, ...);

int  EvtPrecedent();
int  EvtSuivant();
int  EvtDemandeMontre();
char EvtLitUnSeul(int evt, char affiche);
char EvtLitASuivre(int evt, char affiche);
int  EvtNtupleEcrit();
int  EvtNtupleRelit();
int  EvtNtupleAffiche();
int  EvtNtupleProfond();
void EvtVarCalcule(int evt, int var1, unsigned char oper, int var2, float *resultat);
char EvtAdmis(int evt);
int  AutoriseEvt(int evt);

int  ArchInit(char *optn, char *nom);
int  ArchExit();
void ArchCreeUI();
void ArchStrmRazDisplay();
int  ArchEvtLastNum();
int  ArchEvtRead(int f, int num, off_t *retour);
void ArchEvtMontre(int evt);
int  ArchEvtSave(char avec_graph);
void ArchEvtErase();
char ArchEvtKeepFree();

int  ProdInit(char a_nouveau);
void ProdCreeUI();
void ProdChannelUnite(char *nom, void **adrs, int *dim, TANGO_TYPE *type,
					  float **base, int *dep, int *arr,
					  float **energie, float **khi2, float **bruit);
void ProdChannelBasics(float *adrs, int dim, float *base);

#endif
