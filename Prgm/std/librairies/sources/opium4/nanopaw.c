#include <stdio.h>
#ifdef macintosh
#pragma message(__FILE__)
#endif
#include "environnement.h"

#define LABEL_AXES
#define LIMITES_AXES
#define GRAFMAX 32
#define NOMMAX 32

#include <stdlib.h> /* pour malloc, free et system */
#include <math.h>
#include <errno.h>
#include <stdarg.h>

#ifndef WIN32
	#ifndef __MWERKS__
		#define STD_UNIX
	#endif
#endif

#ifdef STD_UNIX
	#include <strings.h>  /* pour strlen qui rend size_t si intrinseque */
#else
	#include <string.h>
#endif


#define FWHM (sqrt(8.0 * log(2.0)))
#define FWHM_POURCENT (float)(100.0 * FWHM)

#ifdef solaris
	#define sqrtf(x) (float)sqrt((double)x)
	#define expf(x) (float)exp((double)x)
#endif

#ifdef PROJECT_BUILDER
	#define sqrtf(x) (float)sqrt((double)x)
	#define expf(x) (float)exp((double)x)
#endif

#define NANOPAW_C
#include "defineVAR.h"
#include "decode.h"
#include "fitlib.h"
#include <opium.h>
#include <histos.h>
#include <nanopaw.h>

static char PlotModeBatch,PlotModeOpium;
static int PlotBatchNiveau;

static char *PlotName[MAXPLOTS+1];
static Panel pCourbe,pPlotFitMethode;
static Panel pHistoNeuf;   static int VarBins;
static Panel pHistoCree;
static Panel pH2DNeuf;     static int Ab6Bins,OrdBins;
static Panel pH2DCree;     static float Ab6Mini,Ab6Maxi,OrdMini,OrdMaxi;
static Panel pAxes;        static int NumAb6,NumOrd;
static Panel pFitLimites,pPlotLimit3D;
static Panel pFitLin,pFitDegre2,pFitParab;  static float FitMini,FitMaxi;
static Panel pPlotLimites; static TypeLimite Xlimite,Ylimite,Zlimite;
static Panel pFitGauss,pPlotStats;
static float PlotResolSigma,PlotResolFWHM,PlotResolStat;
static unsigned char PlotType;
static char PlotNtpSepare;
static char PlotNtpSep[PLOT_SEP_NB] = { ' ', ';', 0x09 };

#define MAXSELECTIONS 16
typedef struct {
	unsigned char oper;
	unsigned char marque;
	int var;
	float min,max;
	int valides,total;
} TypePlotCoupure;
static TypePlotCoupure Coupure[MAXSELECTIONS],CoupureLue;
// static char PlotCoupeNom[NOMMAX];
static char PlotCoupePath[MAXFILENAME];
static char PlotMarqueCles[256];

static ArgDesc PlotNtupleCoupDesc[6];
static ArgStruct CoupureAS = { (void *)Coupure, (void *)&CoupureLue, sizeof(TypePlotCoupure), PlotNtupleCoupDesc };
static ArgDesc JeuCoupureDesc[] = { { "Coupure", DESC_STRUCT_SIZE(PlotSelectNb,MAXSELECTIONS) &CoupureAS, 0 }, DESC_END };

static Panel pPlotCoupure; static unsigned char PlotCoupOper,PlotEvtMarque; static char PlotCoupAutre;
static Info iPlotZoneConseil;
static Panel pPlotComptesZone;
static float AireXmin,AireXmax,AireYmin,AireYmax,AireTaux;
static int AireNb;
static char AireTxtX[40],AireTxtY[40];
static int PlotEvtsTotal,PlotTempsTotal;
static float PlotEfficacite,PlotTaux;

static WndColor *PlotLut;
static WndColorLevel PlotFigR[WND_NBQUAL],PlotFigG[WND_NBQUAL],PlotFigB[WND_NBQUAL];
static WndColorLevel PlotFitR[WND_NBQUAL],PlotFitG[WND_NBQUAL],PlotFitB[WND_NBQUAL];
#define PLOT_COLOR_FIG(qual) PlotFigR[qual],PlotFigG[qual],PlotFigB[qual]
#define PLOT_COLOR_FIT(qual) PlotFitR[qual],PlotFitG[qual],PlotFitB[qual]

static char PlotRepert[MAXFILENAME];

#define PLOT_MAXCOURBES 16
#define PLOT_MAXCOURBENOM 16
#define PLOT_MAXCOEFS 4

typedef enum {
	PLOT_FIT_VALEURS = 0,
	PLOT_FIT_GRAPHIC,
	PLOT_FIT_NB
} PLOT_FIT_METHODES;
#define PLOT_FIT_CLES "par valeurs/graphique"
static char PlotFitMethode;
static Info iPlotFitConseil;

typedef enum {
	PLOT_POLYNOME = 0,
	PLOT_HYPERBOLE,
	PLOT_EXP,
	PLOT_LOG,
	PLOT_GAUSS,
	PLOT_NBCOURBES
} PLOT_TYPE_COURBE;

static char **PlotCourbeTraduit;
static char *PlotCourbeTypes[] = {
	"polynome",
	"hyperbole",
	"exponentielle",
	"logarithme",
	"gaussienne",
	"\0"
};
static int    PlotCourbeNb;
static char  *PlotCourbeNoms[PLOT_MAXCOURBES+1];
static char   PlotCourbeCoefNom[PLOT_MAXCOEFS][16];
static Panel pPlotCourbeDef,pPlotCourbeCoef,pPlotCourbeGraph;
static int    PlotCourbeNum;
static Term  tPlot;
static OpiumColor PlotVertJauneOrangeRouge[] = { { GRF_RGB_GREEN }, { GRF_RGB_YELLOW }, { GRF_RGB_ORANGE }, { GRF_RGB_MAGENTA }, { GRF_RGB_RED } };
static OpiumColor PlotRougeVert[] = { { GRF_RGB_RED }, { GRF_RGB_GREEN } };

/* static gene avec X11 ????? */
 struct StructPlotCourbe {
	char nom[PLOT_MAXCOURBENOM]; char type;
	OpiumColor couleur;
	int pts; float min,max,x0;
	int nb; float coef[PLOT_MAXCOEFS];
	float *donnees;
} PlotCourbe[PLOT_MAXCOURBES],PlotCourbeMod;

/* ===================================================================== */
void PlotLimites(int nbevts, int duree) {
	PlotNtupleValides = PlotEvtsTotal = nbevts; PlotTempsTotal = duree;
	PlotEfficacite = 100.0;
	if(PlotTempsTotal > 0.0) PlotTaux = PlotEvtsTotal / (float)PlotTempsTotal;
	else PlotTaux = 0.0;
}
/* ===================================================================== */
PlotEspace PlotEspaceCree(char *nom, int *dim) {
	PlotEspace espace;

	if(PlotEspaceNb >= MAXESPACES) return(0);
	espace = (PlotEspace)malloc(sizeof(TypePlotEspace));
	if(espace) {
		strncpy(espace->nom,nom,MAXNOMESPACE);
		espace->num = PlotEspaceNb;
		espace->dim = dim;
		espace->max = 0;
		espace->sel = 0;
		espace->pNtuple = 0;
		PlotEspacePtr[PlotEspaceNb] = espace;
		PlotEspacesList[PlotEspaceNb++] = nom; PlotEspacesList[PlotEspaceNb] = "\0";
	}
	return(espace);
}
/* ===================================================================== */
void PlotEspaceSupprime(PlotEspace espace) {
	int i;

	if(!espace) return;
	i = espace->num;
	PlotEspaceNb--;
	for( ; i<PlotEspaceNb; i++) {
		PanelDelete(PlotEspacePtr[i]->pNtuple);
		PlotEspacePtr[i] = PlotEspacePtr[i+1]; PlotEspacePtr[i]->num = i;
		PlotEspacesList[i] = PlotEspacesList[i+1];
	}
	PlotEspacesList[i] = "\0";
	if(espace->sel) free(espace->sel);
	free(espace);
}
/* ===================================================================== */
char PlotVarCree(TypePlotVar *var, char *nom, char *unite, float **adrs, PlotEspace espace) {
	size_t l;

	l = strlen(nom); var->nom = (char *)malloc(l+1); if(!(var->nom)) return(0); var->nom[l] = '\0';
	if(unite) l = strlen(unite); else l = 0; var->unite = (char *)malloc(l+1); if(!(var->unite)) return(0); var->unite[l] = '\0';
	*adrs = 0;  /* le pointeur utilisateur est mis a zero pour PlotNtupleSupprime */
	strcpy(var->nom,nom);
	strcpy(var->unite,unite);
	var->adrs = adrs;
	var->espace = espace;
	var->composante = 0;
	var->sel = 1;
	var->min = 0.0; var->max = 1.0;
	return(1);
}
/* ===================================================================== */
void PlotVarMinMax(TypePlotVar *var, float min, float max) {
	if(var) { var->min = min; var->max = max; }
}
/* ===================================================================== */
void PlotVarSelecte(TypePlotVar *var, char sel) { if(var) var->sel = sel; }
/* ===================================================================== */
static char PlotVarDecrit(void) {
	ArgDesc *desc; int i;

	if(!PlotVarList) return(0);

	i = 0;
	desc = PlotFctnDesc = ArgCreate(1);
	ArgAdd(desc++,"variable",DESC_LISTE PlotVarList, &PlotLu.ord,  0);
	PlotTypeDesc[i++] = PlotFctnDesc;

	desc = PlotHistoDesc = ArgCreate(4);
	ArgAdd(desc++,"variable", DESC_LISTE PlotVarList, &PlotLu.ab6,  0);
	ArgAdd(desc++,"min",      DESC_FLOAT              &PlotLu.xmin, 0);
	ArgAdd(desc++,"max",      DESC_FLOAT              &PlotLu.xmax, 0);
	ArgAdd(desc++,"nb_bins",  DESC_INT                &PlotLu.binx, 0);
	PlotTypeDesc[i++] = PlotHistoDesc;

	desc = Plot2DDesc = ArgCreate(8);
	ArgAdd(desc++,"x",         DESC_LISTE PlotVarList, &PlotLu.ab6,  0);
	ArgAdd(desc++,"x.min",     DESC_FLOAT              &PlotLu.xmin, 0);
	ArgAdd(desc++,"x.max",     DESC_FLOAT              &PlotLu.xmax, 0);
	ArgAdd(desc++,"x.nb_bins", DESC_INT                &PlotLu.binx, 0);
	ArgAdd(desc++,"y",         DESC_LISTE PlotVarList, &PlotLu.ord,  0);
	ArgAdd(desc++,"y.min",     DESC_FLOAT              &PlotLu.ymin, 0);
	ArgAdd(desc++,"y.max",     DESC_FLOAT              &PlotLu.ymax, 0);
	ArgAdd(desc++,"y.nb_bins", DESC_INT                &PlotLu.biny, 0);
	PlotTypeDesc[i++] = Plot2DDesc;

	desc = PlotBiDesc = ArgCreate(8);
	ArgAdd(desc++,"x",         DESC_LISTE PlotVarList,   &PlotLu.ab6,  0);
	ArgAdd(desc++,"x.limites", DESC_KEY "imposees/auto", &PlotLu.x.automatique, 0);
	ArgAdd(desc++,"x.min",     DESC_FLOAT                &PlotLu.x.min, 0);
	ArgAdd(desc++,"x.max",     DESC_FLOAT                &PlotLu.x.max, 0);
	ArgAdd(desc++,"y",         DESC_LISTE PlotVarList,   &PlotLu.ord,  0);
	ArgAdd(desc++,"y.limites", DESC_KEY "imposees/auto", &PlotLu.y.automatique, 0);
	ArgAdd(desc++,"y.min",     DESC_FLOAT                &PlotLu.y.min, 0);
	ArgAdd(desc++,"y.max",     DESC_FLOAT                &PlotLu.y.max, 0);
//	ArgAdd(desc++,0,           DESC_VARIANTE(PlotLu.x.automatique) PlotXlimDesc, 0);
//	ArgAdd(desc++,0,           DESC_VARIANTE(PlotLu.y.automatique) PlotYlimDesc, 0);
	PlotTypeDesc[i++] = PlotBiDesc;

	// printf("* Construction des panels de choix de variables\n");
	desc = &(PlotNtupleCoupDesc[0]);
	ArgAdd(desc++,0,     DESC_KEY "evts/et/ou",   &CoupureLue.oper,   0);
	ArgAdd(desc++,"var", DESC_LISTE PlotVarList,  &CoupureLue.var,    0);
	ArgAdd(desc++,"min", DESC_FLOAT               &CoupureLue.min,    0);
	ArgAdd(desc++,"max", DESC_FLOAT               &CoupureLue.max,    0);
	ArgAdd(desc++,"off", DESC_KEY PlotMarqueCles, &CoupureLue.marque, 0);

	return(1);
}
/* ===================================================================== */
int PlotVarsDeclare(TypePlotVar *var) {
	int i;

	if(PlotVar) {
		PanelDelete(pPlotVarSel);
		if(PlotVarList) free(PlotVarList);
		PlotVarList = 0;
		PlotVar = 0;
	}
	if(PlotFctnDesc) { free(PlotFctnDesc); PlotFctnDesc = 0; }
	if(PlotHistoDesc) { free(PlotHistoDesc); PlotHistoDesc = 0; }
	if(Plot2DDesc) { free(Plot2DDesc); Plot2DDesc = 0; }
	if(PlotBiDesc) { free(PlotBiDesc); PlotBiDesc = 0; }

	PlotVar = var;
	PlotVarNb = 0;
	if(!PlotVar) return(1);

	while(PlotVar[PlotVarNb].nom[0]) PlotVarNb++;
	PlotVarList = (char **)malloc((PlotVarNb + 1) * sizeof(char *));
	if(!PlotVarList) return(0);
	i = 0;
	while(PlotVar[i].nom[0]) {
		PlotVarList[i] = PlotVar[i].nom;
		i++;
	}
	PlotVarList[i] = PlotVar[i].nom;

	PlotVarDecrit();

	return(1);
}
/* ===================================================================== */
static int PlotVars1DMinMaxSet(Panel p, int item, void *arg) {
	PanelCode(&(p->items[item-1]),PNL_READ,0,0);
	VarMini = PlotVar[NumVar].min; VarMaxi = PlotVar[NumVar].max;
	PanelRefreshVars(p);
	return(0);
}
/* ===================================================================== */
static int PlotVars1DMinMaxGet(Panel p, void *arg) {
	PlotVar[NumVar].min = VarMini; PlotVar[NumVar].max = VarMaxi;
//	printf("(%s) %s.min = %g, .max = %g\n",__func__,
//		PlotVar[NumVar].nom,PlotVar[NumVar].min,PlotVar[NumVar].max);
	return(0);
}
/* ===================================================================== */
static int PlotVarsXMinMaxSet(Panel p, int item, void *arg) {
	PanelCode(&(p->items[item-1]),PNL_READ,0,0);
//	printf("(%s) %s.min = %g, .max = %g\n",__func__,
//		   PlotVar[NumAb6].nom,PlotVar[NumAb6].min,PlotVar[NumAb6].max);
	Ab6Mini = PlotVar[NumAb6].min; Ab6Maxi = PlotVar[NumAb6].max;
	PanelRefreshVars(p);
	return(0);
}
/* ===================================================================== */
static int PlotVarsYMinMaxSet(Panel p, int item, void *arg) {
	PanelCode(&(p->items[item-1]),PNL_READ,0,0);
	OrdMini = PlotVar[NumOrd].min; OrdMaxi = PlotVar[NumOrd].max;
	PanelRefreshVars(p);
	return(0);
}
/* ===================================================================== */
static int PlotVars2DMinMaxGet(Panel p, void *arg) {
	PlotVar[NumAb6].min = Ab6Mini; PlotVar[NumAb6].max = Ab6Maxi;
	PlotVar[NumOrd].min = OrdMini; PlotVar[NumOrd].max = OrdMaxi;
	return(0);
}
/* ===================================================================== */
void PlotVarsInterface(void) {
	/* Les variables utilisateur doivent avoir ete declarees,
	   car PlotVarList doit avoir ete cree, et il l'est par PlotVarsDeclare() */
	int i,j,h,k,x,y; int nb_vars[MAXESPACES];

    PanelErase(pCourbe);
	PanelList (pCourbe,"Ordonnee",PlotVarList,&NumVar,16);

    PanelErase(pHistoNeuf);
	PanelList (pHistoNeuf,"Variable",PlotVarList,&NumVar,16);
	PanelFloat(pHistoNeuf,"Minimum",&VarMini);
	PanelFloat(pHistoNeuf,"Maximum",&VarMaxi);
	PanelInt  (pHistoNeuf,"Nb bins",&VarBins);
	PanelListB(pHistoNeuf,"Marquage",PlotMarqueTexte,(char *)&VarMarque,8);
	PanelItemExit(pHistoNeuf,1,PlotVars1DMinMaxSet,0);
	PanelOnOK(pHistoNeuf,PlotVars1DMinMaxGet,0);

    PanelErase(pHistoCree);
	PanelList (pHistoCree,"Variable",PlotVarList,&NumVar,16);
	PanelFloat(pHistoCree,"Minimum",&VarMini);
	PanelFloat(pHistoCree,"Maximum",&VarMaxi);
	PanelListB(pHistoCree,"Marquage",PlotMarqueTexte,(char *)&VarMarque,8);
	PanelItemExit(pHistoCree,1,PlotVars1DMinMaxSet,0);
	PanelOnOK(pHistoCree,PlotVars1DMinMaxGet,0);

    PanelErase(pH2DNeuf);
	PanelSepare(pH2DNeuf,"Abscisse");
x = PanelList  (pH2DNeuf,"Variable",PlotVarList,&NumAb6,16);
	PanelFloat (pH2DNeuf,"Minimum",&Ab6Mini);
	PanelFloat (pH2DNeuf,"Maximum",&Ab6Maxi);
	PanelInt   (pH2DNeuf,"Nb bins",&Ab6Bins);
	PanelSepare(pH2DNeuf,"Ordonnee");
y = PanelList  (pH2DNeuf,"Variable",PlotVarList,&NumOrd,16);
	PanelFloat (pH2DNeuf,"Minimum",&OrdMini);
	PanelFloat (pH2DNeuf,"Maximum",&OrdMaxi);
	PanelInt   (pH2DNeuf,"Nb bins",&OrdBins);
	PanelSepare(pH2DNeuf,0);
	PanelListB (pH2DNeuf,"Marquage",PlotMarqueTexte,(char *)&VarMarque,8);
	PanelItemExit(pH2DNeuf,x,PlotVarsXMinMaxSet,0);
	PanelItemExit(pH2DNeuf,y,PlotVarsYMinMaxSet,0);
	PanelOnOK(pH2DNeuf,PlotVars2DMinMaxGet,0);

    PanelErase(pH2DCree);
	PanelSepare(pH2DCree,"Abscisse");
x = PanelList  (pH2DCree,"Variable",PlotVarList,&NumAb6,16);
	PanelFloat (pH2DCree,"Minimum",&Ab6Mini);
	PanelFloat (pH2DCree,"Maximum",&Ab6Maxi);
	PanelSepare(pH2DCree,"Ordonnee");
y = PanelList  (pH2DCree,"Variable",PlotVarList,&NumOrd,16);
	PanelFloat (pH2DCree,"Minimum",&OrdMini);
	PanelFloat (pH2DCree,"Maximum",&OrdMaxi);
	PanelSepare(pH2DCree,0);
	PanelListB (pH2DCree,"Marquage",PlotMarqueTexte,(char *)&VarMarque,8);
	PanelItemExit(pH2DCree,x,PlotVarsXMinMaxSet,0);
	PanelItemExit(pH2DCree,y,PlotVarsYMinMaxSet,0);
	PanelOnOK(pH2DCree,PlotVars2DMinMaxGet,0);

    PanelErase(pAxes);
	PanelList (pAxes,"Abcisse",PlotVarList,&NumAb6,16);
	PanelList (pAxes,"Ordonnee",PlotVarList,&NumOrd,16);
	// PanelListB(pAxes,"Marquage",PlotMarqueTexte,(char *)&VarMarque,8);

	PanelErase(pPlotCoupure);
	PanelKeyB (pPlotCoupure,"Coupure","nouvelle/precedente ET/precedente OU/tout",(char*)&PlotCoupOper,16);
	PanelList (pPlotCoupure,"Sur variable",PlotVarList,&NumVar,16);
	PanelFloat(pPlotCoupure,"Au minimum",&VarMini);
	PanelFloat(pPlotCoupure,"et maximum",&VarMaxi);
	PanelListB(pPlotCoupure,"Marquage exclus",PlotMarqueTexte,(char *)&PlotEvtMarque,8);
	PanelBool (pPlotCoupure,"Coupure a suivre",&PlotCoupAutre);

	for(j=0; j<PlotEspaceNb; j++) nb_vars[j] = 0;
	pPlotVarSel = PanelCreate(PlotVarNb);
	i = 0;
	while(PlotVar[i].nom[0]) {
		PanelBool(pPlotVarSel,PlotVar[i].nom,&(PlotVar[i].sel));
		nb_vars[(PlotVar[i].espace)->num] += 1;
		i++;
	}
	h = WndPixToLig(OpiumServerHeigth(0)) - 5;
	k = ((i - 1) / h) + 1;
	PanelColumns(pPlotVarSel,k);
	PanelTitle(pPlotVarSel,"Selection des variables du Ntuple");

	y = 40;
	for(j=0; j<PlotEspaceNb; j++) {
		PlotEspacePtr[j]->pNtuple = PanelCreate(nb_vars[j]);
		OpiumPosition((PlotEspacePtr[j]->pNtuple)->cdr,10,y);
		y += WndLigToPix(nb_vars[j]+1)+20;
	}
}
/* ===================================================================== */
char PlotNtupleAjoute(int i, int max, float init) {
    int j; float *f; PlotEspace espace; int dim;

    espace = PlotVar[i].espace;
    dim = espace->max = max; // avant: dim = *(espace->dim);
    if(!espace->sel) {
        espace->sel = (TypeGraphIndiv *)malloc(dim * sizeof(TypeGraphIndiv));
        if(espace->sel) for(j=0; j<dim; j++) {
            espace->sel[j].trace = PlotMarqueCode[GRF_POINT];
            espace->sel[j].r = espace->sel[j].g = espace->sel[j].b = 0xFF;
        } else return(0);
    }
    f = (float *)malloc(dim * sizeof(float));
    *PlotVar[i].adrs = f;
    if(!f) return(0);
    for(j=0; j<dim; j++) *f++ = init;

    return(1);
}
/* ===================================================================== */
char PlotNtupleCree(int max, float init) {
    int i;

	i = 0; while(PlotVar[i].nom[0]) {
        if(!(PlotNtupleAjoute(i,max,init))) return(0);
		i++;
	}

	return(1);
}
/* ===================================================================== */
void PlotNtupleSupprime(void) {
	int i; TypeGraphIndiv *sel;

    if(!PlotVar) return;
	i = 0;
	while(PlotVar[i].nom[0]) {
		if((sel = (PlotVar[i].espace)->sel)) { free(sel); (PlotVar[i].espace)->sel = 0; }
		if(*PlotVar[i].adrs) { free(*PlotVar[i].adrs); *PlotVar[i].adrs = 0; }
		i++;
	}
}
/* ===================================================================== */
int PlotIndexVar(char *nom) {
	int i;

    if(!PlotVar) return(-1);
	i = 0; while(PlotVar[i].nom[0]) { if(!strcmp(PlotVar[i].nom,nom)) break; i++; }
	if(PlotVar[i].nom[0]) return(i); else return(-1);
}
/* ===================================================================== */
void PlotNtupleSeparateur(char sep) { if((sep >= 0) && (sep < PLOT_SEP_NB)) PlotNtpSepare = sep; }
/* ===================================================================== */
int PlotNtupleEcrit(char *nom, char sel_var, char sel_evt, char ajoute) {
	int i,num,dim; char c,sep; char fini; PlotEspace espace;
	FILE *f; char valeur[16];

	if(ajoute) f = fopen(nom,"a"); else f = fopen(nom,"w");
	if(!f) return(0);
	sep = PlotNtpSep[PlotNtpSepare];
	for(i=0,c='\0'; i<PlotVarNb; i++) if(!sel_var || PlotVar[i].sel) {
		if(c) fprintf(f,"%c",c); else c = sep; fprintf(f,"%s",PlotVar[i].nom);
	}
	fprintf(f,"\n");

	num = 0;
	do {
		fini = 1;
		for(i=0,c='\0'; i<PlotVarNb; i++) if(!sel_var || PlotVar[i].sel) {
			espace = PlotVar[i].espace; dim = *(espace->dim);
			if(num < dim) { fini = 0; break; }
		}
		if(fini) break;
		for(i=0,c='\0'; i<PlotVarNb; i++) if(!sel_var || PlotVar[i].sel) {
			if(c) fprintf(f,"%c",c); else c = sep;
			espace = PlotVar[i].espace; dim = *(espace->dim);
			if(num < dim) {
				if(sel_evt) { if(PlotNtupleAbsent(espace,num)) continue; }
				snprintf(valeur,16,"%g",*(*PlotVar[i].adrs + num));
				if(!strcmp(valeur,"nan")) fputs("0.0",f);
				else fputs(valeur,f);
			}
		}
		fprintf(f,"\n");
		num++;
	} while(!fini);

	fclose(f);
	return(1);
}
/* ===================================================================== */
int PlotNtupleRelit(char *nom, char ajoute) {
	FILE *f;
#define LIGNE_MAX 8192
	char ligne[LIGNE_MAX]; char *c,*item,sep;
	int j,k,lig,cols,num,max,nt,nv,*nb; char termine;
	float ***adrs; int *esp; char *vu; PlotEspace espace;

	f = fopen(nom,"r");
	if(!f) {
		OpiumFail("Fichier %s illisible (%s)",nom,strerror(errno));
		return(0);
	}
	printf("* Relecture du fichier de Ntuples: %s\n",nom);
	// adrs = (float ***)malloc(PlotVarNb * sizeof(float **));
	// esp = (int *)malloc(PlotVarNb * sizeof(int)); vu = (char *)malloc(PlotEspaceNb * sizeof(char));
	adrs = Malloc(PlotVarNb,float **);
	esp = Malloc(PlotVarNb,int);
	vu = Malloc(PlotEspaceNb,char);
	termine = !adrs || !esp || !vu;
	lig = 0; nv = 0; max = 0; sep = 0; nt = 0;
	while(!termine && LigneSuivante(ligne,LIGNE_MAX,f)) {
		/* parcours de la ligne courante */
		for(k=0; k<PlotEspaceNb; k++) vu[k] = 0;
		c = ligne;
		nv = 0;
		do {
			if(*c == '\0') break;
			if(!sep) item = ItemAvant(" ;",&sep,&c);
			else item = ItemJusquA(sep,&c);
			if(lig == 0) {
				for(j=0; j<PlotVarNb; j++) if(!strcmp(PlotVar[j].nom,item)) break;
				if(j < PlotVarNb) {
					adrs[nv] = PlotVar[j].adrs;
					esp[nv] = PlotVar[j].espace->num;
					vu[esp[nv]] = 1;
				} else {
					printf("  ! La variable '%s' est inconnue et ne sera pas lue\n",item);
					adrs[nv] = 0;
				}
			} else if(adrs[nv] && (esp[nv] < PlotEspaceNb)) {
				espace = PlotEspacePtr[esp[nv]]; num = *(espace->dim); vu[esp[nv]] = 1;
				if(num < espace->max) sscanf(item,"%g",(*adrs[nv] + num)); // (*PlotVar[j].adrs + num)
				else {
					adrs[nv] = 0;
					printf("  ! L'espace '%s' est sature (%d Ntuples), colonne %d maintenant ignoree\n",espace->nom,espace->max,nv+1);
				}
			}
			nv++;
		} while(1);
		/* conclusion sur cette ligne */
		if(lig == 0) {
			cols = nv; // c'est vrai, mais on s'en fout, en fait
			for(k=0; k<PlotEspaceNb; k++) if(vu[k] && !ajoute) *(PlotEspacePtr[k]->dim) = 0;
		} else {
			nt++;
			termine = 1;
			for(k=0; k<PlotEspaceNb; k++) if(vu[k]) {
				termine = 0;
				nb = PlotEspacePtr[k]->dim;
				*nb += 1; if(*nb > max) max = *nb;
			}
		}
		lig++;
	}
	fclose(f);
	if(adrs) free(adrs); if(esp) free(esp); if(vu) free(vu);
	// OpiumNote?
	printf("  > Relu %d ntuple%s de %d variable%s\n",Accord1s(nt),Accord1s(nv));
	if(ajoute) printf("  > en complement avec les precedents, soit %d ntuple%s au total\n",Accord1s(max));

	return(max+1);
}
/* ===================================================================== */
void PlotNtupleAffiche(PlotEspace espace, int num, int x, int y, int h, int v) {
	char titre[80]; int i,j,k,nbligs;

	if((num < 0) || (num >= *(espace->dim))) return;
	if(PlotVar) {
		if(OpiumDisplayed((espace->pNtuple)->cdr)) OpiumClear((espace->pNtuple)->cdr);
		PanelErase(espace->pNtuple);
		i = j = 0;
		while(PlotVar[i].nom[0]) {
			if((PlotVar[i].espace == espace) && PlotVar[i].sel) {
				PanelFloat(espace->pNtuple,PlotVar[i].nom,*PlotVar[i].adrs + num); j++;
			}
			i++;
		}
		nbligs = WndPixToLig(OpiumServerHeigth(0)) - 5;
		k = ((j - 1) / nbligs) + 1;
		PanelColumns(espace->pNtuple,k);
		sprintf(titre,"Ntuple %s #%d",espace->nom,num+1);
		OpiumTitle((espace->pNtuple)->cdr,titre);
		if((x >= 0) && (y >= 0)) {
			int xn,yn,hn,vn;
			OpiumSizeChanged((espace->pNtuple)->cdr);
			OpiumGetSize((espace->pNtuple)->cdr,&hn,&vn);
			xn = x - hn - WND_ASCINT_WIDZ - 2; yn = y;
			if(xn <= OpiumMargeX) xn = x + h + WND_ASCINT_WIDZ;
			// printf("(%s) Evt %dx%d @(%d, %d), Ntp %dx%d @(%d, %d)\n",__func__,h,v,x,y,hn,vn,xn,yn);
			OpiumPosition((espace->pNtuple)->cdr,xn,yn);
		}
		OpiumDisplay((espace->pNtuple)->cdr);
	} else return;
}
/* =====================================================================
void PlotNtupleTitre(int x, int y, char *texte) { OpiumTitle((espace->pNtuple)->cdr,texte); }
   =====================================================================
void PlotNtuplePosition(int x, int y) { OpiumPosition((espace->pNtuple)->cdr,x,y); }
   =====================================================================
void PlotNtupleGetPos(int *x, int *y) { OpiumGetPosition((espace->pNtuple)->cdr,x,y); }
   =====================================================================
void PlotNtupleGetSize(int *h, int *v) { OpiumGetSize((espace->pNtuple)->cdr,h,v); }
   =====================================================================
void PlotNtupleRefresh(void) { OpiumClear((espace->pNtuple)->cdr); OpiumDisplay((espace->pNtuple)->cdr); }
   ===================================================================== */
int PlotNtuplePointe(PlotEspace *espace) {
	int p; int dim; TypeGraphIndiv *sel;
	int ix,iy; float rxpnt,rypnt,rxinf,ryinf,rxsup,rysup;
	int num; float *xadrs,*yadrs;
	int h,v;
	float xmin,xmax,ymin,ymax,dx,dy,grad;
	int essais;

	if((p = PlotChoixFig2D()) < 0) return(-1);
	*espace = PlotVar[Plot[p].ab6].espace;
	dim = *((*espace)->dim);
	GraphGetPoint(Plot[p].g,&ix,&rxpnt,&iy,&rypnt);
	// printf("(%s) pointage sur %s termine\n",__func__,Plot[p].titre);
	OpiumGetSize((Plot[p].g)->cdr,&h,&v);
	GraphAxisFloatGet(Plot[p].g,GRF_XAXIS,&xmin,&xmax,&grad);
	GraphAxisFloatGet(Plot[p].g,GRF_YAXIS,&ymin,&ymax,&grad);
	num = -1;
	sel = (*espace)->sel;
	dx = (xmax - xmin) / (float)h; /* dimension user d'un point (a peu pres) */
	dx *= 4;                       /* soit precision de pointage = 4 points  */
	dy = (ymax - ymin) / (float)v; /* dimension user d'un point (a peu pres) */
	dy *= 4;                       /* soit precision de pointage = 4 points  */
	essais = 4;
	do {
		rxinf = rxpnt - dx; rxsup = rxpnt + dx;
		ryinf = rypnt - dy; rysup = rypnt + dy;
		// WndPrint("Incertitude: (+/- %g, +/- %g)\n",dx,dy);
		// WndPrint("Carre de recherche: ([%g .. %g], [%g .. %g])\n",rxinf,rxsup,ryinf,rysup);
		xadrs = *PlotVar[Plot[p].ab6].adrs;
		yadrs = *PlotVar[Plot[p].ord].adrs;
		for(num=0; num<dim; num++) {
			if(GRF_SHOWN(sel[num].trace)) {
				if((*xadrs >= rxinf) && (*xadrs <= rxsup)
				&& (*yadrs >= ryinf) && (*yadrs <= rysup)) break;
			}
			xadrs++; yadrs++;
		}
		if(num < dim) break;
		dx *= 2; dy *= 2;
	} while(--essais);
	if(num >= dim) num = -1; // PlotNtuplePointeUneFois ne peut pas tester sur dim
	// printf("(%s) %s: evenement %d pointe\n",__func__,Plot[p].titre,num);

	return(num);
}
/* ===================================================================== */
static int PlotNtuplePointeUneFois(void) {
	int num; PlotEspace espace;

	if((num = PlotNtuplePointe(&espace)) < 0) OpiumWarn("Pas de n-tuple pointe");
	else PlotNtupleAffiche(espace,num,-1,-1,-1,-1);

	return(0);
}
/* ===================================================================== */
int PlotNtupleRect(void) {
	int p; PlotEspace espace; int dim; TypeGraphIndiv *sel;
	int ix0,iy0,ix1,iy1; float rxpnt0,rypnt0,rxpnt1,rypnt1;
	int num; float *xadrs,*yadrs;
	int nb;

	if((p = PlotChoixFig2D()) < 0) return(0);
	espace = PlotVar[Plot[p].ab6].espace;
	dim = *(espace->dim);
	GraphGetRect(Plot[p].g,&ix0,&rxpnt0,&iy0,&rypnt0,&ix1,&rxpnt1,&iy1,&rypnt1);
	sel = espace->sel;
	xadrs = *PlotVar[Plot[p].ab6].adrs;
	yadrs = *PlotVar[Plot[p].ord].adrs;
	nb = 0;
	for(num=0; num<dim; num++) {
		if(GRF_SHOWN(sel[num].trace)) {
			if((*xadrs >= rxpnt0) && (*xadrs <= rxpnt1) && (*yadrs >= rypnt0) && (*yadrs <= rypnt1)) nb++;
			else GRF_HIDE(sel[num].trace);
		}
		xadrs++; yadrs++;
	}
	return(nb);
}
/* ===================================================================== */
void PlotNtupleColore(PlotEspace espace, int num, WndColorLevel r, WndColorLevel g, WndColorLevel b) {
	espace->sel[num].r = WND_COLOR8(r);
	espace->sel[num].g = WND_COLOR8(g);
	espace->sel[num].b = WND_COLOR8(b);
}
/* ===================================================================== */
void PlotNtupleMarque(PlotEspace espace, int num, unsigned char marque) {
	espace->sel[num].trace = PlotMarqueCode[marque];
}
/* ===================================================================== */
char PlotNtupleInclu(PlotEspace espace, int num, unsigned char marque) {
	return(espace->sel[num].trace == PlotMarqueCode[marque]);
}
/* ===================================================================== */
char PlotNtupleExclu(PlotEspace espace, int num, unsigned char marque) {
	return(espace->sel[num].trace != PlotMarqueCode[marque]);
}
/* ===================================================================== */
char PlotNtuplePresent(PlotEspace espace, int num) {
	return GRF_SHOWN(espace->sel[num].trace);
}
/* ===================================================================== */
char PlotNtupleAbsent(PlotEspace espace, int num) {
	return GRF_HIDDEN(espace->sel[num].trace);
}
/* ===================================================================== */
void PlotNtupleMontre(PlotEspace espace, int num) {
	GRF_SHOW(espace->sel[num].trace,GRF_PNT);
}
/* ===================================================================== */
void PlotNtupleCache(PlotEspace espace, int num) {
	GRF_HIDE(espace->sel[num].trace);
}
/* ===================================================================== */
char PlotNtupleCandidat(PlotEspace espace, int num, unsigned char marque) {
	return((PlotNtuplePresent(espace,num) && ((VarMarque == GRF_TOUTE) || PlotNtupleInclu(espace,num,VarMarque))));
}
/* ===================================================================== */
void PlotNtupleAspect(PlotEspace espace, int num, TypeGraphIndiv *etiquette) {
	memcpy(etiquette,&(espace->sel[num]),sizeof(TypeGraphIndiv));
}
/* ===================================================================== */
void PlotNtupleMasque(PlotEspace espace) {
	if(OpiumDisplayed((espace->pNtuple)->cdr)) OpiumClear((espace->pNtuple)->cdr);
}
/* ========================================================================== */
int PlotNtupleCoupeResultat(PlotEspace espace) {
	int evt,dim,valides;
	// ajustement des delais: fait par ProdCalculeDelai()

	dim = *(espace->dim);
	valides = 0;
	for(evt=0; evt<dim; evt++) if(PlotNtuplePresent(espace,evt)) valides++;
	PlotNtupleValides = valides;
	if(PlotEvtsTotal > 0) PlotEfficacite = (float)100.0 * (float)PlotNtupleValides / (float)PlotEvtsTotal;
	else PlotEfficacite = 0.0;
	if(PlotTempsTotal > 0.0) PlotTaux = PlotNtupleValides / (float)PlotTempsTotal;
	else PlotTaux = 0.0;
	if(PlotModeOpium) { if(OpiumDisplayed(pPlotValidite->cdr)) PanelRefreshVars(pPlotValidite); }

	return(valides);
}
/* ========================================================================== */
int PlotNtupleCoupeOpere(unsigned char oper, int numvar, float mini, float maxi, unsigned char marque) {
	int evt; float *adrs;
	PlotEspace espace; int dim;

//	printf(". Coupure a effectuer: %g < %s < %g\n",VarMini,PlotVarList[NumVar],VarMaxi);
	adrs = *PlotVar[numvar].adrs;
	espace = PlotVar[numvar].espace;
	dim = *(espace->dim);
//	printf(". Variable @%08X[%d]\n",(hexa)adrs,dim);
	switch(oper) {
	  case PLOT_OPER_NOUVEAU: /* nouvelle coupure */
		for(evt=0; evt<dim; evt++,adrs++) {
			if((*adrs >= mini) && (*adrs  <=  maxi)) PlotNtupleMontre(espace,evt);
			else PlotNtupleMarque(espace,evt,marque);
		}
		PlotEspaceSelecte = espace;
		break;
	  case PLOT_OPER_ET: /* coupure precedente ET celle qui vient */
		for(evt=0; evt<dim; evt++,adrs++) if(PlotNtuplePresent(espace,evt)) {
			if((*adrs < mini) || (*adrs  >  maxi)) PlotNtupleMarque(espace,evt,marque);
		}
		break;
	  case PLOT_OPER_OU: /* coupure precedente OU celle qui vient */
		for(evt=0; evt<dim; evt++,adrs++) if(PlotNtupleAbsent(espace,evt)) {
			if((*adrs >= mini) && (*adrs  <=  maxi)) PlotNtupleMarque(espace,evt,marque);
		}
		break;
	  case PLOT_OPER_TOUT: /* tout recuperer */
			for(evt=0; evt<dim; evt++) {
				PlotNtupleMontre(espace,evt);
			}
		PlotEspaceSelecte = espace;
		break;
	}
	return(PlotNtupleCoupeResultat(espace));
}
/* ========================================================================== */
void PlotNtupleCoupExec(unsigned char oper, int numvar, float mini, float maxi, unsigned char marque) {

	PlotNtupleValides = PlotNtupleCoupeOpere(oper,numvar,mini,maxi,marque);
	switch(oper) {
	  case PLOT_OPER_NOUVEAU: /* nouvelle coupure */
		PlotSelectNb = 0;
		PlotCoupOper = 1;
		break;
	  case PLOT_OPER_TOUT: /* tout recuperer */
		PlotSelectNb = -1;
		PlotCoupOper = 0;
		PlotCoupAutre = 0;
		break;
	}
	if(PlotSelectNb < MAXSELECTIONS) {
		if(PlotSelectNb >= 0) {
			Coupure[PlotSelectNb].oper = oper; Coupure[PlotSelectNb].var = numvar;
			Coupure[PlotSelectNb].min = mini; Coupure[PlotSelectNb].max = maxi;
			Coupure[PlotSelectNb].marque = marque;
			Coupure[PlotSelectNb].valides = PlotNtupleValides;
			Coupure[PlotSelectNb].total = *(PlotVar[numvar].espace->dim);
		}
		PlotSelectNb++;
	} else PlotPrint(OPIUM_ERROR,"Coupure non enregistree");
}
/* ========================================================================== */
char PlotNtupleCoupeSur(unsigned char oper, char *nom, float mini, float maxi, char imprime) {
	int num;

	if(oper == PLOT_OPER_TOUT) num = 0;
	else if((num = PlotIndexVar(nom)) < 0) {
		PlotPrint(OPIUM_ERROR,"Variable non definie: '%s'",nom);
		return(0);
	}
	PlotNtupleCoupExec(oper,num,mini,maxi,GRF_ABSENT);
	if(imprime) printf(". %d evenement%s valide%s par %g < %s < %g\n",Accord2s(PlotNtupleValides),mini,nom,maxi);
	return(1);
}
/* ========================================================================== */
static void PlotNtupleCoupeReporte(Term term, char conclusion) {
	int i;

	if(PlotModeOpium) {
		TermEmpty(term);
		TermHold(term);
		TermPrint(term,"\n");
		if(PlotSelectNb == 0) { if(conclusion) TermPrint(term,"=== Aucune coupure ===\n"); }
		else {
			TermPrint(term," %s     %16s dans [%g .. %g]: %d/%d",
					  (PlotSelectNb > 1)? "1/":"  ",PlotVarList[Coupure[0].var],Coupure[0].min,Coupure[0].max,Coupure[0].valides,Coupure[0].total);
			if(Coupure[0].marque != GRF_ABSENT) TermPrint(term," (exclus: marques %s)",PlotMarqueTexte[Coupure[0].marque]);
			TermPrint(term,"\n");
			for(i=1; i<PlotSelectNb; i++) {
				TermPrint(term,"%2d/  %s %16s dans [%g .. %g]: %d/%d",
						  i+1,(Coupure[i].oper == 1)? "et": "ou",PlotVarList[Coupure[i].var],Coupure[i].min,Coupure[i].max,Coupure[i].valides,Coupure[i].total);
				if(Coupure[i].marque != GRF_ABSENT) TermPrint(term," (exclus: marques %s)",PlotMarqueTexte[Coupure[i].marque]);
				TermPrint(term,"\n");
			}
			if(!conclusion) TermPrint(term,"\n");
		}
		if(conclusion) TermPrint(term,"=== (%d evenement%s valide%s) ===\n",Accord2s(PlotNtupleValides));
		else TermPrint(term,"(%d coupure%s)\n",Accord1s(PlotSelectNb));
		TermRelease(term);
		if(OpiumDisplayed(term->cdr)) OpiumRefresh(term->cdr); else OpiumDisplay(term->cdr);
	}
}
/* ========================================================================== */
void PlotNtupleCoupeRefresh() {
	int i;

	if(PlotModeOpium) {
		PlotRefreshSiUniqueBiplot();
		if(tCoupures) PlotNtupleCoupeReporte(tCoupures,0);
		else if(tSelect) PlotNtupleCoupeReporte(tSelect,1);
	}

	if(PlotSelectNb == 0) printf("  > Coupure courante: aucune\n");
	else {
		printf("  > Coupure courante:\n");
		if(PlotSelectNb == 1) printf("    |          %16s dans [%g .. %g]: %d/%d (marque exclus: %s)\n",PlotVarList[Coupure[0].var],
									 Coupure[0].min,Coupure[0].max,Coupure[0].valides,Coupure[0].total,PlotMarqueTexte[Coupure[0].marque]);
		else printf("    |   1/     %16s dans [%g .. %g]: %d/%d (marque exclus: %s)\n",PlotVarList[Coupure[0].var],
			Coupure[0].min,Coupure[0].max,Coupure[0].valides,Coupure[0].total,PlotMarqueTexte[Coupure[0].marque]);
		for(i=1; i<PlotSelectNb; i++) {
			printf("    |  %2d/  %s %16s dans [%g .. %g]: %d/%d (marque exclus: %s)\n",i+1,
				(Coupure[i].oper == 1)? "et": "ou",
				PlotVarList[Coupure[i].var],Coupure[i].min,Coupure[i].max,Coupure[i].valides,Coupure[i].total,
				PlotMarqueTexte[Coupure[i].marque]);
		}
	}
	printf("  ==  %d evenement%s valide%s\n",Accord2s(PlotNtupleValides));
}
/* ========================================================================== */
int PlotNtupleCoupeAdd(Menu menu, MenuItem item) {
	do { // int i;
//		printf(". Coupure selon les %d variables suivantes:\n",PlotVarNb);
//		i = 0; while(PlotVarList[i][0]) { printf("  | %2d: '%s'\n",i,PlotVarList[i]); if(++i > 99) break; }
//		if(OpiumReadInt("No variable",&NumVar) == PNL_CANCEL) return(0);
		if(OpiumExec(pPlotCoupure->cdr) == PNL_CANCEL) return(0);
		printf(". Coupure demandee: %g < %s < %g\n",VarMini,PlotVarList[NumVar],VarMaxi);
//		if(OpiumReadInt("No variable",&NumVar) == PNL_CANCEL) return(0);
		PlotNtupleCoupExec(PlotCoupOper,NumVar,VarMini,VarMaxi,PlotEvtMarque);
		PlotNtupleCoupeRefresh();
	} while(PlotCoupAutre);
	return(0);
}
/* ========================================================================== */
int PlotNtupleCoupeZone(Menu menu, MenuItem item) {
	int p; int posx,posy,larg,haut,px,py,dy;
	int ix0,iy0,ix1,iy1;

	if((p = PlotChoixFig2D()) < 0) return(0);
	OpiumGetPosition((Plot[p].g)->cdr,&posx,&posy); OpiumGetSize((Plot[p].g)->cdr,&larg,&haut);
	px = posx + larg + WndColToPix(3);
	dy = WndLigToPix(6);
	if(posy > dy) py = posy - dy; else py = posy + haut + WndLigToPix(3);
	InfoWrite(iPlotZoneConseil,1,"Pointes d'abord le coin haut-gauche,");
	InfoWrite(iPlotZoneConseil,2,"et termines par le coin bas-droit");
	OpiumPosition(iPlotZoneConseil->cdr,posx,py);
	OpiumDisplay(iPlotZoneConseil->cdr);
	GraphGetRect(Plot[p].g,&ix0,&AireXmin,&iy0,&AireYmin,&ix1,&AireXmax,&iy1,&AireYmax);
	OpiumClear(iPlotZoneConseil->cdr);

	PlotEvtMarque = GRF_ABSENT;
	PlotCoupOper = PlotSelectNb? PLOT_OPER_ET: PLOT_OPER_NOUVEAU;
	NumVar = Plot[p].ab6; VarMini = AireXmin; VarMaxi = AireXmax;
	printf(". Coupure demandee: %g < %s < %g\n",VarMini,PlotVarList[NumVar],VarMaxi);
	PlotNtupleCoupExec(PlotCoupOper,NumVar,VarMini,VarMaxi,PlotEvtMarque);
	PlotCoupOper = PLOT_OPER_ET;
	NumVar = Plot[p].ord; VarMini = AireYmin; VarMaxi = AireYmax;
	printf(". Coupure demandee: %g < %s < %g\n",VarMini,PlotVarList[NumVar],VarMaxi);
	PlotNtupleCoupExec(PlotCoupOper,NumVar,VarMini,VarMaxi,PlotEvtMarque);
	PlotNtupleCoupeRefresh();

	return(0);
}
/* ========================================================================== */
int PlotNtupleCoupeRemove(Menu menu, MenuItem item) {
	int num,i;

	num = PlotSelectNb;
	if(OpiumReadInt("Laquelle",&num) == PNL_CANCEL) return(0);
	if((num <= 0) || (num > PlotSelectNb)) {
		OpiumError("Pas de coupure #%d! (maxi:%d)",num,PlotSelectNb);
		return(0);
	}
	num--;
	PlotSelectNb--;
	for(i=num; i<PlotSelectNb; i++)
		memcpy(&(Coupure[i]),&(Coupure[i+1]),sizeof(TypePlotCoupure));
	Coupure[0].oper = PLOT_OPER_NOUVEAU;
	for(i=0; i<PlotSelectNb; i++) {
		PlotNtupleValides = PlotNtupleCoupeOpere(Coupure[i].oper,Coupure[i].var,Coupure[i].min,Coupure[i].max,Coupure[i].marque);
		Coupure[i].valides = PlotNtupleValides; Coupure[i].total = *(PlotVar[Coupure[i].var].espace->dim);
	}
	PlotNtupleCoupeRefresh();
	return(0);
}
/* ========================================================================== */
int PlotNtupleCoupeRAZ(Menu menu, MenuItem item) {
	if(PlotSelectNb > 0) {
		PlotNtupleCoupExec(PLOT_OPER_TOUT,Coupure[PlotSelectNb - 1].var,0,0,PLOT_OPER_TOUT);
		PlotNtupleCoupeRefresh();
	}
	return(0);
}
/* ========================================================================== */
void PlotNtupleCoupePath(char *nom) { strcpy(PlotCoupePath,nom); }
/* ========================================================================== */
int PlotNtupleCoupeJeu(char *nom) {
	int i; char nom_complet[MAXFILENAME];

	PlotSelectNb = 0;
	strcat(strcpy(nom_complet,PlotCoupePath),nom);
	errno = 0;
	if(ArgScan(nom_complet,JeuCoupureDesc) <= 0) PlotPrint(OPIUM_ERROR,"Relecture de %s en erreur (%s)",nom_complet,strerror(errno));
	else PlotPrint(OPIUM_NOTE,"%d coupure%s relue%s dans '%s'",Accord2s(PlotSelectNb),nom_complet);
	Coupure[0].oper = PLOT_OPER_NOUVEAU;
	for(i=0; i<PlotSelectNb; i++) {
		PlotNtupleValides = PlotNtupleCoupeOpere(Coupure[i].oper,Coupure[i].var,Coupure[i].min,Coupure[i].max,Coupure[i].marque);
		Coupure[i].valides = PlotNtupleValides; Coupure[i].total = *(PlotVar[Coupure[i].var].espace->dim);
	}
	PlotNtupleCoupeRefresh();

	return(0);
}
/* ========================================================================== */
int PlotNtupleCoupeSauve(Menu menu, MenuItem item) {
	char *liste[MAXSELECTIONS+1],nom[NOMMAX]; int nb,i,j;
	char nom_complet[MAXFILENAME];

	if(!PlotSelectNb) { OpiumError("Pas de coupure a sauver"); return(0); }
	RepertoireListeCree(0,PlotCoupePath,liste,MAXSELECTIONS,&nb);
	if(nb) {
		liste[nb] = "(nouveau)";
		liste[nb+1] = "\0";
		i = nb - 1; // nb;
		if(OpiumReadList("Jeu de coupures",liste,&i,NOMMAX) == PNL_CANCEL) return(0);
	} else i = 0;
	if(i == nb) {
		sprintf(nom,"Jeu_%02d",i+1);
		if(OpiumReadText("Nom du jeu de coupures",nom,NOMMAX) == PNL_CANCEL) return(0);
		for(j=0; j<nb; j++) if(!strcmp(liste[j],nom)) {
			OpiumError("Ce nom existe deja!");
			RepertoireListeLibere(liste,nb-1);
			return(0);
		}
		liste[nb] = nom;
	}
	strcat(strcpy(nom_complet,PlotCoupePath),liste[i]);
	RepertoireCreeRacine(nom_complet);
	if(ArgPrint(nom_complet,JeuCoupureDesc) <= 0) printf("! Sauvegarde impossible (%s)\n",strerror(errno));
	else printf("* %d coupure%s sauvee%s dans '%s'\n",Accord2s(PlotSelectNb),nom_complet);

	RepertoireListeLibere(liste,nb-1); // erreur si free("\0")
	return(0);
}
/* ========================================================================== */
int PlotNtupleCoupeRelit(Menu menu, MenuItem item) {
	char *liste[MAXSELECTIONS+1]; int nb,i;

	RepertoireListeCree(0,PlotCoupePath,liste,MAXSELECTIONS,&nb);
	if(!nb) { OpiumError("Pas de jeu de coupures sauve"); return(0); }
	i = 0;
	if(OpiumReadList("Jeu de coupures",liste,&i,NOMMAX) == PNL_OK) PlotNtupleCoupeJeu(liste[i]);
	RepertoireListeLibere(liste,nb);

	return(0);
}
/* ========================================================================== */
int PlotNtupleCompteZone() {
	int p; int posx,posy,larg,haut,px,py,dy;
	int ix0,iy0,ix1,iy1;
	int evt; float *x,*y;
	PlotEspace espace; int dim,nb;
	char texte[255];

	if((p = PlotChoixFig2D()) < 0) return(0);
	OpiumGetPosition((Plot[p].g)->cdr,&posx,&posy); OpiumGetSize((Plot[p].g)->cdr,&larg,&haut);
	px = posx + larg + WndColToPix(3);
	dy = WndLigToPix(6);
	if(posy > dy) py = posy - dy; else py = posy + haut + WndLigToPix(3);
	InfoWrite(iPlotZoneConseil,1,"Pointes d'abord le coin haut-gauche,");
	InfoWrite(iPlotZoneConseil,2,"et termines par le coin bas-droit");
	OpiumPosition(iPlotZoneConseil->cdr,posx,py);
	OpiumDisplay(iPlotZoneConseil->cdr);
	GraphGetRect(Plot[p].g,&ix0,&AireXmin,&iy0,&AireYmin,&ix1,&AireXmax,&iy1,&AireYmax);
	OpiumClear(iPlotZoneConseil->cdr);

	espace = PlotVar[Plot[p].ab6].espace;
	dim = *(espace->dim);
	x = *PlotVar[Plot[p].ab6].adrs;
	y = *PlotVar[Plot[p].ord].adrs;
	nb = 0;
	for(evt=0; evt<dim; evt++,x++,y++) {
		if((*x >= AireXmin) && (*x <= AireXmax)
		   && (*y >= AireYmin) && (*y <= AireYmax)) nb++;
	}
	sprintf(texte,"%d evenement%s dans [%g < %s < %g], [%g < %s < %g] (%.3g Hz)",
		Accord1s(nb),AireXmin,PlotVarList[Plot[p].ab6],AireXmax,
		AireYmin,PlotVarList[Plot[p].ord],AireYmax,
		PlotTempsTotal? (float)nb/(float)PlotTempsTotal: 0);
	printf("* %s\n",texte);
	sprintf(AireTxtX,"%s min",PlotVarList[Plot[p].ab6]);
	sprintf(AireTxtY,"%s min",PlotVarList[Plot[p].ord]);
	AireNb = nb; AireTaux = PlotTempsTotal? (float)nb / (float)PlotTempsTotal: 0.0;
	OpiumPosition(pPlotComptesZone->cdr,px,posy);
	if(OpiumAlEcran(pPlotComptesZone)) OpiumRefresh(pPlotComptesZone->cdr);
	else OpiumDisplay(pPlotComptesZone->cdr);

	return(0);
}
/* ===================================================================== */
int PlotChoix(void) {
	int p;

	if(!PlotsNb) { OpiumError("Pas de plot utilisable"); return(-1); }
	if(PlotCourant >= PlotsNb) PlotCourant = PlotsNb - 1;
	p = PlotCourant;
	if(PlotsNb > 1) {
		if(OpiumReadList("Quel plot",PlotName,&p,16) == PNL_CANCEL) return(-1);
		if((p < 0) || (p >= PlotsNb)) {
			OpiumError("Le plot #%d n'a pas encore ete trace",p);
			return(-1);
		}
	}
	if(Plot[p].vide) {
		OpiumError("Le plot %s est vide",Plot[p].titre);
		return(-1);
	}
	PlotCourant = p;
	return(p);
}
/* ===================================================================== */
int PlotSelect(char *titre) {
	int p;

	for(p=0; p<PlotsNb; p++) if(!strcmp(titre,PlotName[p])) break;
	if(p < PlotsNb) { PlotCourant = p; return(p); }
	else return(-1);
}
/* ===================================================================== */
int PlotChoixFig1D(void) {
	int m,n,p;

	n = 0;
	m = -1;  /* pour gcc */
	for(p=0; p<PlotsNb; p++)
		if(!Plot[p].vide && ((Plot[p].type == PLOT_HISTO) || (Plot[p].type == PLOT_FONCTION))) {
			m = p; n++;
		}
	if(!n) return(-1);
	else if(n == 1) return(m);
	else do {
		if((p = PlotChoix()) < 0) break;
		if((Plot[p].type == PLOT_HISTO) || (Plot[p].type == PLOT_FONCTION)) break;
		OpiumError("La figure '%s' represente deux variables",Plot[p].titre);
	} while(1);
	return(p);
}
/* ===================================================================== */
int PlotChoixFig2D(void) {
	int m,n,p;

	n = 0;
	m = -1;  /* pour gcc */
	for(p=0; p<PlotsNb; p++)
		if(!Plot[p].vide && ((Plot[p].type == PLOT_H2D) || (Plot[p].type == PLOT_BIPLOT))) {
			m = p; n++;
		}
	if(!n) return(-1);
	else if(n == 1) return(m);
	else do {
		if((p = PlotChoix()) < 0) break;
		if((Plot[p].type == PLOT_H2D) || (Plot[p].type == PLOT_BIPLOT)) break;
		OpiumError("La figure '%s' represente une seule variable",Plot[p].titre);
	} while(1);
	return(p);
}
/* ===================================================================== */
int PlotChoixH2D(void) {
	int m,n,p;

	n = 0;
	m = -1;  /* pour gcc */
	for(p=0; p<PlotsNb; p++)
		if(!Plot[p].vide && (Plot[p].type == PLOT_H2D)) {
			m = p; n++;
		}
	if(!n) return(-1);
	else if(n == 1) return(m);
	else do {
		if((p = PlotChoix()) < 0) break;
		if(Plot[p].type == PLOT_H2D) break;
		OpiumError("La figure '%s' represente une seule variable",Plot[p].titre);
	} while(1);
	return(p);
}
/* ===================================================================== */
int PlotNouveau(unsigned char type) {
	int p;

	for(p=0; p<PlotsNb; p++) if(Plot[p].vide) break;
	if(p >= MAXPLOTS) {
		OpiumError("Deja %d figures! Il faut purger...",p);
		return(-1);
	}
	Plot[p].type = type;
	Plot[p].h = 0;
	Plot[p].hd = 0;
	Plot[p].g = 0;
	Plot[p].avec_fit = FIT_NEANT;
	Plot[p].dim = 0; Plot[p].xcourbe = Plot[p].ycourbe = 0;
	Plot[p].x.automatique = Plot[p].y.automatique = 0;
	Plot[p].x.min = Plot[p].x.max = Plot[p].y.min = Plot[p].y.max = 0.0;
	return(p);
}
/* ===================================================================== */
static int PlotBuild(unsigned char type) {
	int p;

	if((p = PlotNouveau(type)) < 0) return(0);
	// parm 'nouveau' utilise plusieurs fois dans PlotTrace()
	PlotType = type;
	return(PlotTrace(p,-1));
}
/* ===================================================================== */
int PlotBuildFonction(void) { return(PlotBuild(PLOT_FONCTION)); }
/* ===================================================================== */
int PlotBuildHisto(void) { return(PlotBuild(PLOT_HISTO)); }
/* ===================================================================== */
int PlotBuild2D(void) { return(PlotBuild(PLOT_H2D)); }
/* ===================================================================== */
int PlotBuildBiplot(void) { return(PlotBuild(PLOT_BIPLOT)); }
/* ===================================================================== */
int PlotRedraw(void) { return(PlotTrace(PlotChoix(),0)); }
/* ===================================================================== */
int PlotRefresh(void) {
	int p;

	if((p = PlotChoix()) < 0) return(0);
	/*
	if(OpiumDisplayed(Plot[p].g->cdr)) OpiumClear(Plot[p].g->cdr);
	*/
	OpiumDisplay(Plot[p].g->cdr);
	return(0);
}
/* ===================================================================== */
void PlotRefreshSiUniqueBiplot(void) {
	int m,n,p;
	int zmin,zmax;

	n = 0;
	m = -1;  /* pour gcc */
	for(p=0; p<PlotsNb; p++) if(!Plot[p].vide && (Plot[p].type == PLOT_BIPLOT)) { m = p; n++; }
	if(n == 1) {
		p = m;
		if(Plot[p].type == PLOT_H2D) {
			H2DGetExtremaInt((H2D)Plot[p].h,&zmin,&zmax);
			GraphDataIntGamma(Plot[p].g,2,0,zmax,0,OPIUM_LUT_DIM-1);
		}
		OpiumDisplay(Plot[p].g->cdr);
	}
}
/* ===================================================================== */
void PlotRempliHisto(int p, int numvar, float mini, float maxi, int bins) {
	int num,qual; float *adrs; PlotEspace espace; int dim;
	char nom_plot[OPIUM_MAXTITRE];

	Plot[p].h = HistoCreateFloat(mini,maxi,bins);
	Plot[p].hd = HistoAdd(Plot[p].h,HST_FLOAT);
	for(qual=0; qual<WND_NBQUAL; qual++)
		HistoDataSupportRGB(Plot[p].hd,qual,PLOT_COLOR_FIG(qual));
	Plot[p].g = GraphCreate(400,300,4);
	HistoGraphAdd(Plot[p].h,Plot[p].g);
	Plot[p].ab6 = numvar;
	adrs = *PlotVar[numvar].adrs;
	espace = PlotVar[numvar].espace; dim = *(espace->dim);
	for(num=0; num<dim; num++, adrs++) if(PlotNtupleCandidat(espace,num,VarMarque)) HistoFillFloatToFloat(Plot[p].hd,*adrs,1.0);
#ifdef LABEL_AXES
	GraphAxisTitle(Plot[p].g,GRF_XAXIS,PlotVar[numvar].nom);
#endif
	Plot[p].vide = 0;
#ifdef LIMITES_AXES
	if(Plot[p].x.automatique)
		GraphAxisAutoRange(Plot[p].g,GRF_XAXIS);
	else  GraphAxisFloatRange(Plot[p].g,GRF_XAXIS,Plot[p].x.min,Plot[p].x.max);
	if(Plot[p].y.automatique)
		GraphAxisAutoRange(Plot[p].g,GRF_YAXIS);
	else  GraphAxisFloatRange(Plot[p].g,GRF_YAXIS,Plot[p].y.min,Plot[p].y.max);
#endif
	sprintf(Plot[p].titre,"%s",PlotVar[numvar].nom);
	OpiumTitle(Plot[p].g->cdr,Plot[p].titre);
	snprintf(nom_plot,OPIUM_MAXTITRE,"%d: %s %s",p+1,PlotTypeTexte[Plot[p].type],Plot[p].titre);
	OpiumName(Plot[p].g->cdr,nom_plot);
	PlotName[p] = (Plot[p].g->cdr)->nom;
	PlotCourant = p;
	if(p == PlotsNb) PlotsNb++;
}
/* ===================================================================== */
HistoData PlotGetData(int p) {
	if((p >= 0) && (p < PlotsNb)) return(Plot[p].hd);
	else return(0);
}
/* ===================================================================== */
static void PlotAxeNom(int var, char *nom, int dim) {
	nom[0] = '\0';
	if(PlotVar[var].unite) {
		if(PlotVar[var].unite[0])
			snprintf(nom,dim,"%s (%s)",PlotVar[var].nom,PlotVar[var].unite);
	}
	if(nom[0] == '\0') strncpy(nom,PlotVar[var].nom,dim);
}
/* ===================================================================== */
static char PlotConstruitFonction(char nouveau, int p) {
	PlotEspace espace; int dim; int y;
	char nom_ord[80],nom_donnee[80];

	sprintf(Plot[p].titre,"%s",PlotVar[Plot[p].ord].nom);
	Plot[p].h = 0;
	espace = PlotVar[Plot[p].ord].espace; dim = *(espace->dim);
	if(nouveau) {
		Plot[p].g = GraphCreate(400,200,5);
		/* voir PlotLimite */
		Xlimite.automatique = Ylimite.automatique = 1;
		Xlimite.min = Xlimite.max = 0;
		Ylimite.min = Ylimite.max = 0;
		memcpy(&(Plot[p].x),&Xlimite,sizeof(TypeLimite));
		memcpy(&(Plot[p].y),&Ylimite,sizeof(TypeLimite));
	} else GraphErase(Plot[p].g);
	GraphAdd(Plot[p].g,GRF_INT|GRF_INDEX|GRF_XAXIS,PlotCoordIndex,dim);
	y = GraphAdd(Plot[p].g,GRF_FLOAT|GRF_HST|GRF_YAXIS,*PlotVar[Plot[p].ord].adrs,dim);
	PlotAxeNom(Plot[p].ord,nom_ord,80);
#ifdef LABEL_AXES
	GraphAxisTitle(Plot[p].g,GRF_XAXIS,espace->nom);
	GraphAxisTitle(Plot[p].g,GRF_YAXIS,nom_ord);
#endif
	snprintf(nom_donnee,80,"%s:%d",PlotVar[Plot[p].ord].nom,p+1);
	GraphDataName(Plot[p].g,y,nom_donnee);
	return(1);
}
/* ===================================================================== */
static char PlotConstruitHisto(char nouveau, int p) {
	PlotEspace espace; int dim;
	int num,qual; float *adrs;
	char nom_ab6[80],nom_donnee[80];

	sprintf(Plot[p].titre,"%s",PlotVar[Plot[p].ab6].nom);
	if(nouveau) {
		Plot[p].h = HistoCreateFloat(Plot[p].xmin,Plot[p].xmax,Plot[p].binx);
		Plot[p].hd = HistoAdd(Plot[p].h,HST_FLOAT);
		for(qual=0; qual<WND_NBQUAL; qual++)
			HistoDataSupportRGB(Plot[p].hd,qual,PLOT_COLOR_FIG(qual));
		// for(qual=0; qual<WND_NBQUAL; qual++)
		// 	printf("(%s) fig %s: %04X.%04X.%04X\n",__func__,WndSupportNom[qual],PLOT_COLOR_FIG(qual));
		Plot[p].g = GraphCreate(400,300,4);
		HistoGraphAdd(Plot[p].h,Plot[p].g);
		PlotAxeNom(Plot[p].ab6,nom_ab6,80);
		GraphDataName(Plot[p].g,0,nom_ab6);
		snprintf(nom_donnee,80,"histo:%d",p+1);
		GraphDataName(Plot[p].g,1,nom_donnee);
	#ifdef LABEL_AXES
		GraphAxisTitle(Plot[p].g,GRF_XAXIS,nom_ab6);
	#endif
	} else {
		HistoRaz(Plot[p].h); // seulement si une seule courbe: HistoDataRaz(Plot[p].hd);
		HistoLimitsFloat(Plot[p].h,Plot[p].xmin,Plot[p].xmax);
	}

	adrs = *PlotVar[Plot[p].ab6].adrs;
	espace = PlotVar[Plot[p].ab6].espace; dim = *(espace->dim);
	for(num=0; num<dim; num++, adrs++) if(PlotNtupleCandidat(espace,num,VarMarque)) HistoFillFloatToFloat(Plot[p].hd,*adrs,1.0);

	return(1);
}
/* ===================================================================== */
static char PlotConstruit2D(char nouveau, int p) {
	PlotEspace espace; int dim;
	int num; float *xadrs; float *yadr;
	int zmin,zmax;
	char nom_ab6[80],nom_ord[80],nom_donnee[80];

	sprintf(Plot[p].titre,"%s vs %s",PlotVar[Plot[p].ord].nom,PlotVar[Plot[p].ab6].nom);
	espace = PlotVar[Plot[p].ab6].espace; dim = *(espace->dim);
	if(espace != PlotVar[Plot[p].ord].espace) {
		OpiumFail("%s et %s n'appartiennent pas au meme espace",PlotVar[Plot[p].ord].nom,PlotVar[Plot[p].ab6].nom);
		return(0);
	}
	if(nouveau) {
		Plot[p].h = (Histo)H2DCreateFloatFloatToInt(
			Plot[p].xmin,Plot[p].xmax,Plot[p].binx,Plot[p].ymin,Plot[p].ymax,Plot[p].biny);
		if(!Plot[p].h) {
			OpiumFail("Histo %dx%d (%s[%g..%g] vs %s[%g..%g]) pas cree (%s)",Plot[p].binx,Plot[p].biny,
				PlotVar[Plot[p].ord].nom,Plot[p].ymin,Plot[p].ymax,
				PlotVar[Plot[p].ab6].nom,Plot[p].xmin,Plot[p].xmax,HistoMsg[HistoError]);
			return(0);
		}
		Plot[p].hd = 0;
		H2DLUT((H2D)Plot[p].h,PlotLut,OPIUM_LUT_DIM);
		Plot[p].g = GraphCreate(400,400,3);
		H2DGraphAdd((H2D)Plot[p].h,Plot[p].g);
		PlotAxeNom(Plot[p].ab6,nom_ab6,80); GraphDataName(Plot[p].g,0,nom_ab6);
		PlotAxeNom(Plot[p].ord,nom_ord,80); GraphDataName(Plot[p].g,1,nom_ord);
		snprintf(nom_donnee,80,"histo 2D:%d",p+1); GraphDataName(Plot[p].g,2,nom_donnee);
	#ifdef LABEL_AXES
		GraphAxisTitle(Plot[p].g,GRF_XAXIS,nom_ab6);
		GraphAxisTitle(Plot[p].g,GRF_YAXIS,nom_ord);
	#endif
	} else {
		H2DLimitsFloat((H2D)Plot[p].h,HST_AXEX,Plot[p].xmin,Plot[p].xmax);
		H2DLimitsFloat((H2D)Plot[p].h,HST_AXEY,Plot[p].ymin,Plot[p].ymax);
		H2DRaz((H2D)Plot[p].h);
	}

	xadrs = *PlotVar[Plot[p].ab6].adrs; yadr = *PlotVar[Plot[p].ord].adrs;
	for(num=0; num<dim; num++, xadrs++,yadr++) {
		if(PlotNtupleCandidat(espace,num,VarMarque))
			H2DFillFloatFloatToInt((H2D)Plot[p].h,*xadrs,*yadr,1);
	}
	H2DGetExtremaInt((H2D)Plot[p].h,&zmin,&zmax);
	Plot[p].z.min = (float)zmin;
	Plot[p].z.max = (float)zmax;
	GraphDataIntGamma(Plot[p].g,2,0,zmax,0,OPIUM_LUT_DIM-1);
	return(1);
}
/* ===================================================================== */
static char PlotConstruitBiplot(char nouveau, int p) {
	PlotEspace espace; int dim; int y; int qual;
	char nom_ab6[80],nom_ord[80];

	snprintf(Plot[p].titre,80,"%s vs %s",PlotVar[Plot[p].ord].nom,PlotVar[Plot[p].ab6].nom);
	espace = PlotVar[NumAb6].espace; dim = *(espace->dim);
	/* { int k,num,larg;
		larg = 20;
		printf("(%s) %s[%d]:",__func__,espace->nom,dim);
		printf("\n|     "); for(k=0; k<larg; k++) printf(" %2d",k);
		printf("\n|     "); for(k=0; k<larg; k++) printf("...");
		for(num=0; num<dim; num++) {
			if(!(num % larg)) printf("\n| %03d:",num);
			printf(" %02x",espace->sel[num].trace);
		}
		printf("\n");
	} */
	Plot[p].h = 0;
	if(nouveau) Plot[p].g = GraphCreate(400,400,5);
	else GraphErase(Plot[p].g);
	GraphAdd(Plot[p].g,GRF_FLOAT|GRF_XAXIS,*PlotVar[Plot[p].ab6].adrs,dim);
	// printf("(%s) Definitions individuelles @%08llX\n",__func__,(IntAdrs)(espace->sel));
	GraphAdd(Plot[p].g,GRF_INDIV,espace->sel,dim);
	y = GraphAdd(Plot[p].g,GRF_FLOAT|GRF_YAXIS,*PlotVar[Plot[p].ord].adrs,dim);
	for(qual=0; qual<WND_NBQUAL; qual++)
		GraphDataSupportTrace(Plot[p].g,y,qual,GRF_PNT,GRF_POINT,1);
#ifdef LIMITES_AXES
	if(Plot[p].x.automatique || (Plot[p].x.min == Plot[p].x.max))
		GraphAxisAutoRange(Plot[p].g,GRF_XAXIS);
	else  GraphAxisFloatRange(Plot[p].g,GRF_XAXIS,Plot[p].x.min,Plot[p].x.max);
	if(Plot[p].y.automatique || (Plot[p].y.min == Plot[p].y.max))
		GraphAxisAutoRange(Plot[p].g,GRF_YAXIS);
	else  GraphAxisFloatRange(Plot[p].g,GRF_YAXIS,Plot[p].y.min,Plot[p].y.max);
#endif
	PlotAxeNom(Plot[p].ab6,nom_ab6,80); GraphDataName(Plot[p].g,0,nom_ab6);
	PlotAxeNom(Plot[p].ord,nom_ord,80); GraphDataName(Plot[p].g,y,nom_ord);
#ifdef LABEL_AXES
	GraphAxisTitle(Plot[p].g,GRF_XAXIS,nom_ab6);
	GraphAxisTitle(Plot[p].g,GRF_YAXIS,nom_ord);
#endif

	return(1);
}
/* ===================================================================== */
static char PlotConstruit(char nouveau, int p) {
	char ok; char nom_plot[OPIUM_MAXTITRE];

	ok = 0;
	switch(Plot[p].type) {
		case PLOT_FONCTION: ok = PlotConstruitFonction(nouveau,p); break;
		case PLOT_HISTO:    ok = PlotConstruitHisto(nouveau,p); break;
		case PLOT_H2D:      ok = PlotConstruit2D(nouveau,p); break;
		case PLOT_BIPLOT:   ok = PlotConstruitBiplot(nouveau,p); break;
	}
	if(!ok) return(0);
	Plot[p].vide = 0;
	OpiumTitle(Plot[p].g->cdr,Plot[p].titre);
	snprintf(nom_plot,OPIUM_MAXTITRE,"%d: %s %s",p+1,PlotTypeTexte[Plot[p].type],Plot[p].titre);
	OpiumName(Plot[p].g->cdr,nom_plot);
	PlotName[p] = (Plot[p].g->cdr)->nom;
	if(OpiumDisplayed(Plot[p].g->cdr)) OpiumClear(Plot[p].g->cdr);
	OpiumDisplay(Plot[p].g->cdr);
	PlotCourant = p;
	return(1);
}
/* ===================================================================== */
int PlotTrace(int p, char nouveau) {
	unsigned char type; int action;
	int numvar,numab6,numord;
	float xmini,xmaxi,xdelta; int xbins;
	float ymini,ymaxi,ydelta; int ybins;

	if(p < 0) return(0);
	if(nouveau) {
		type = PlotType;
		if(nouveau > 0) {
			if(OpiumReadListB("Type de trace",PlotTypeTraduit,(char *)&type,10) == PNL_CANCEL) return(0);
		}
		Plot[p].type = type;
	} else type = Plot[p].type;

/*
 *	Graphiques de type 'FONCTION'
 */
	if(type == PLOT_FONCTION) {
		numvar = NumVar;
		if(!nouveau) { NumVar = Plot[p].ord; }
		if(OpiumExec(pCourbe->cdr) == PNL_CANCEL) {
			NumVar = numvar;
			return(0);
		}
		Plot[p].ab6 = 0; Plot[p].ord = NumVar;

/*
 *	Graphiques de type 'HISTO'
 */
	} else if(type == PLOT_HISTO) {
		numvar = NumVar;
		xmini = VarMini; xmaxi = VarMaxi; xbins = VarBins;
		if(nouveau) {
//-			OpiumDebug(OPIUM_DEBUG_PANEL,1);
			if(OpiumExec(pHistoNeuf->cdr) == PNL_CANCEL) {
				NumVar = numvar;
				VarMini = xmini; VarMaxi = xmaxi; VarBins = xbins;
				return(0);
			}
//-			OpiumDebug(OPIUM_DEBUG_PANEL,0);
		} else {
			NumVar = Plot[p].ab6;
			HistoGetBinFloat(Plot[p].h,&VarMini,&xdelta,&VarBins);
			VarMaxi = VarMini + ((float)VarBins * xdelta);
			if(OpiumExec(pHistoCree->cdr) == PNL_CANCEL) {
				NumVar = numvar;
				VarMini = xmini; VarMaxi = xmaxi; VarBins = xbins;
				return(0);
			}
		}
		Plot[p].ab6 = NumVar; Plot[p].ord = 0;
		Plot[p].xmin = VarMini; Plot[p].xmax = VarMaxi; Plot[p].binx = VarBins;

/*
 *	Graphiques de type 'HISTO a 2 dimensions'
 */
	} else if(type == PLOT_H2D) {
		numab6 = NumAb6; numord = NumOrd;
		xmini = Ab6Mini; xmaxi = Ab6Maxi; xbins = Ab6Bins;
		ymini = OrdMini; ymaxi = OrdMaxi; ybins = OrdBins;
		if(nouveau) action = OpiumExec(pH2DNeuf->cdr);
		else {
			NumAb6 = Plot[p].ab6; NumOrd = Plot[p].ord;
			H2DGetBinFloat((H2D)Plot[p].h,HST_AXEX,&Ab6Mini,&xdelta,&Ab6Bins);
			H2DGetBinFloat((H2D)Plot[p].h,HST_AXEY,&OrdMini,&ydelta,&OrdBins);
			Ab6Maxi = Ab6Mini + ((float)Ab6Bins * xdelta);
			OrdMaxi = OrdMini + ((float)OrdBins * ydelta);
			action = OpiumExec(pH2DCree->cdr);
		}
		if(action == PNL_CANCEL) {
			NumAb6 = numab6; NumOrd = numord;
			Ab6Mini = xmini; Ab6Maxi = xmaxi; Ab6Bins = xbins;
			OrdMini = ymini; OrdMaxi = ymaxi; OrdBins = ybins;
			return(0);
		}
		Plot[p].ab6 = NumAb6; Plot[p].ord = NumOrd;
		Plot[p].xmin = Ab6Mini; Plot[p].xmax = Ab6Maxi; Plot[p].binx = Ab6Bins;
		Plot[p].ymin = OrdMini; Plot[p].ymax = OrdMaxi; Plot[p].biny = OrdBins;

/*
 *	Graphiques de type 'NUAGE DE POINTS pour deux variables'
 */
	} else if(type == PLOT_BIPLOT) {
		numab6 = NumAb6; numord = NumOrd;
		if(!nouveau) { NumAb6 = Plot[p].ab6; NumOrd = Plot[p].ord; }
		if(OpiumExec(pAxes->cdr) == PNL_CANCEL) {
			NumAb6 = numab6; NumOrd = numord;
			return(0);
		}
		if(PlotVar[NumAb6].espace != PlotVar[NumOrd].espace) {
			OpiumFail("'%s' et '%s' n'appartiennent pas au meme espace",PlotVar[NumOrd].nom,PlotVar[NumAb6].nom);
			NumAb6 = numab6; NumOrd = numord;
			return(0);
		}
		if(nouveau) {
			/* voir PlotLimite */
			Xlimite.automatique = Ylimite.automatique = 0;
			if(OpiumExec(pPlotLimites->cdr) == PNL_CANCEL) return(0);
			memcpy(&(Plot[p].x),&Xlimite,sizeof(TypeLimite));
			memcpy(&(Plot[p].y),&Ylimite,sizeof(TypeLimite));
		}
		Plot[p].ab6 = NumAb6; Plot[p].ord = NumOrd;
	}

	PlotType = type;
	if(PlotConstruit(nouveau,p)) {
		if(p == PlotsNb) {
			PlotsNb++;
			printf("* %d%s fenetre definie: %s %s\n",PlotsNb,(PlotsNb>1)?"eme":"ere",PlotTypeTexte[Plot[p].type],Plot[p].titre);
		}
	}

	return(0);
}
/* ===================================================================== */
static void PlotPointDesigne(int p, int *x, int *y) {
	int ix,iy; float rxpnt,rypnt;
	float dx,dy;

	GraphGetPoint(Plot[p].g,&ix,&rxpnt,&iy,&rypnt);
	dx = (Plot[p].xmax - Plot[p].xmin) / (float)Plot[p].binx;
	*x = (int)((rxpnt - Plot[p].xmin + (dx / 2.0)) / dx);
	dy = (Plot[p].ymax - Plot[p].ymin) / (float)Plot[p].biny;
	*y = (int)((rypnt - Plot[p].ymin + (dy / 2.0)) / dy);

	return;
}
/* ===================================================================== */
static int PlotExtraitBin(void) {
	int p; int x,y,k,nb;
	H2D h; int *tab; float vx,vy;

	if((p = PlotChoixFig2D()) < 0) return(0);
	PlotPointDesigne(p,&x,&y);
	k = x + (y * Plot[p].binx);
	h = (H2D)Plot[p].h;
	nb = h->x.nb * h->y.nb;
	if((k >= 0) && (k < nb)) {
		vx = ((float)x * (Plot[p].xmax - Plot[p].xmin) / (float)Plot[p].binx) + Plot[p].xmin;
		vy = ((float)y * (Plot[p].ymax - Plot[p].ymin) / (float)Plot[p].biny) + Plot[p].ymin;
		tab = (int *)(h->data.adrs);
		printf(". Valeur de %s en (%g, %g): %d\n",Plot[p].titre,vx,vy,tab[k]);
	}

	return(0);
}
/* ===================================================================== */
int PlotExtraitProfilX(void) {
	int p,p2; int x,y,xl,nb,qual; float vy;
	float *adrs; H2D h; int *tab;
	char nom_donnee[GRF_MAX_TITRE]; char nom_plot[OPIUM_MAXTITRE];

	if((p2 = PlotNouveau(PLOT_FONCTION)) < 0) return(0);
	if((p = PlotChoixH2D()) < 0) return(0);
	PlotPointDesigne(p,&x,&y);
	vy = ((float)y * (Plot[p].ymax - Plot[p].ymin) / (float)Plot[p].biny) + Plot[p].ymin;

	PlotType = PLOT_HISTO;
	Plot[p2].type = PLOT_HISTO;
	Plot[p2].ab6 = Plot[p].ab6; Plot[p2].ord = 0;
	Plot[p2].xmin = Plot[p].xmin; Plot[p2].xmax = Plot[p].xmax; Plot[p2].binx = Plot[p].binx;
	snprintf(Plot[p2].titre,MAXTITREPLOT,"%s @%s=%g",PlotVar[Plot[p2].ab6].nom,PlotVar[Plot[p].ord].nom,vy);
	Plot[p2].h = HistoCreateFloat(Plot[p2].xmin,Plot[p2].xmax,Plot[p2].binx);
	Plot[p2].hd = HistoAdd(Plot[p2].h,HST_FLOAT);
	for(qual=0; qual<WND_NBQUAL; qual++)
		HistoDataSupportRGB(Plot[p2].hd,qual,PLOT_COLOR_FIG(qual));
	Plot[p2].g = GraphCreate(400,300,4);
	HistoGraphAdd(Plot[p2].h,Plot[p2].g);
	snprintf(nom_donnee,GRF_MAX_TITRE,"profilX:%d",p);
	GraphDataName(Plot[p2].g,0,PlotVar[Plot[p2].ab6].nom);
	GraphDataName(Plot[p2].g,1,nom_donnee);
	GraphAxisTitle(Plot[p2].g,GRF_XAXIS,PlotVar[Plot[p2].ab6].nom);
	OpiumTitle(Plot[p2].g->cdr,Plot[p2].titre);
	snprintf(nom_plot,OPIUM_MAXTITRE,"%d: %s %s",p2+1,PlotTypeTexte[Plot[p2].type],Plot[p2].titre);
	OpiumName(Plot[p2].g->cdr,nom_plot);
	PlotName[p2] = (Plot[p2].g->cdr)->nom;
	Plot[p2].vide = 0;

	PlotCourant = p2;
	if(p2 == PlotsNb) {
		PlotsNb++;
		printf("* %d%s fenetre definie: %s %s\n",PlotsNb,(PlotsNb>1)?"eme":"ere",PlotTypeTexte[Plot[p2].type],Plot[p2].titre);
	}

	adrs = (float *)(Plot[p2].hd->adrs);
	h = (H2D)Plot[p].h; tab = (int *)(h->data.adrs);
	nb = Plot[p].binx;
	for(x=0, xl=(y*nb); x<nb; x++) *adrs++ = tab[++xl];

	OpiumDisplay(Plot[p2].g->cdr);

	return(0);
}
/* ===================================================================== */
int PlotExtraitProfilY(void) {
	int p,p2; int x,y,yl,dy,nb,qual; float vx;
	float *adrs; H2D h; int *tab;
	char nom_donnee[GRF_MAX_TITRE]; char nom_plot[OPIUM_MAXTITRE];

	if((p2 = PlotNouveau(PLOT_FONCTION)) < 0) return(0);
	if((p = PlotChoixH2D()) < 0) return(0);
	PlotPointDesigne(p,&x,&y);
	vx = ((float)x * (Plot[p].xmax - Plot[p].xmin) / (float)Plot[p].binx) + Plot[p].xmin;

	PlotType = PLOT_HISTO;
	Plot[p2].type = PLOT_HISTO;
	Plot[p2].ab6 = Plot[p].ord; Plot[p2].ord = 0;
	Plot[p2].xmin = Plot[p].ymin; Plot[p2].xmax = Plot[p].ymax; Plot[p2].binx = Plot[p].biny;
	snprintf(Plot[p2].titre,MAXTITREPLOT,"%s @%s=%g",PlotVar[Plot[p2].ab6].nom,PlotVar[Plot[p].ab6].nom,vx);
	Plot[p2].h = HistoCreateFloat(Plot[p2].xmin,Plot[p2].xmax,Plot[p2].binx);
	Plot[p2].hd = HistoAdd(Plot[p2].h,HST_FLOAT);
	for(qual=0; qual<WND_NBQUAL; qual++)
		HistoDataSupportRGB(Plot[p2].hd,qual,PLOT_COLOR_FIG(qual));
	Plot[p2].g = GraphCreate(400,300,4);
	HistoGraphAdd(Plot[p2].h,Plot[p2].g);
	snprintf(nom_donnee,GRF_MAX_TITRE,"profilY:%d",p);
	GraphDataName(Plot[p2].g,0,PlotVar[Plot[p2].ab6].nom);
	GraphDataName(Plot[p2].g,1,nom_donnee);
	GraphAxisTitle(Plot[p2].g,GRF_XAXIS,PlotVar[Plot[p2].ab6].nom);
	OpiumTitle(Plot[p2].g->cdr,Plot[p2].titre);
	snprintf(nom_plot,OPIUM_MAXTITRE,"%d: %s %s",p2+1,PlotTypeTexte[Plot[p2].type],Plot[p2].titre);
	OpiumName(Plot[p2].g->cdr,nom_plot);
	PlotName[p2] = (Plot[p2].g->cdr)->nom;
	Plot[p2].vide = 0;

	PlotCourant = p2;
	if(p2 == PlotsNb) {
		PlotsNb++;
		printf("* %d%s fenetre definie: %s %s\n",PlotsNb,(PlotsNb>1)?"eme":"ere",PlotTypeTexte[Plot[p2].type],Plot[p2].titre);
	}

	adrs = (float *)(Plot[p2].hd->adrs);
	h = (H2D)Plot[p].h; tab = (int *)(h->data.adrs);
	nb = Plot[p].biny;
	yl = x + 1; dy = Plot[p].binx;
	for(x=0; x<nb; x++) { *adrs++ = tab[yl]; yl += dy; }

	OpiumDisplay(Plot[p2].g->cdr);

	return(0);
}
#ifdef INUTILISE
/* ===================================================================== */
static int PlotExtraitProjectX(void) {
	int p,p2; int x0,y0,x1,y1;

	if((p2 = PlotNouveau(PLOT_FONCTION)) < 0) return(0);
	if((p = PlotChoixFig2D()) < 0) return(0);
	PlotPointDesigne(p,&x0,&y0);
	PlotPointDesigne(p,&x1,&y1);
	return(0);
}
/* ===================================================================== */
static int PlotExtraitProjectY(void) {
	return(0);
}
#endif
/* ===================================================================== */
static void PlotImprimeListe(Cadre *liste) {
	Cadre *cdr; char premier; int rc;

	premier = 1;
	cdr = liste;
	while(*cdr) {
		// OpiumTitle(*cdr,"\0");
		// OpiumName(*cdr,Plot[PlotCourant].titre);
		OpiumPScadre = *cdr;
		if(premier) {
			rc = OpiumPSDemarre();
			if(rc) return;
			premier = 0;
		} else OpiumPSAjoute();
		cdr++;
	}
	OpiumPSTermine();
	cdr = liste;
	while(*cdr) {
		OpiumClear(*cdr);
		OpiumDisplay(*cdr);
		cdr++;
	}
}
/* ===================================================================== */
static int PlotAfficheFit(Graph graph, WndAction *e) {
	char texte[2][32];
	Cadre cdr; WndFrame f; int nb,i,k,x,y,dy; size_t l,n;

	switch(PlotFitMode) {
	  case FIT_LINEAIRE:
		sprintf(&(texte[0][0]),"rate=%g",ResultatFit.lin.pente);
		sprintf(&(texte[1][0]),"offset=%g",ResultatFit.lin.base);
		nb = 2;
		break;
	  case FIT_PARABOLIQUE:
		sprintf(&(texte[0][0]),"mean=%g",ResultatFit.par.x0);
		nb = 1;
		break;
	  case FIT_GAUSSIEN:
		sprintf(&(texte[0][0]),"mean=%g",ResultatFit.gau.x0);
		sprintf(&(texte[1][0]),"sigma=%.1f%%",PlotResolSigma);
		nb = 2;
		break;
	  default: nb = 0;
	}
	n = 0; // compilo..
	for(i=0; i<nb; i++) {
		l = strlen(&(texte[i][0]));
		if(i == 0) n = l; else if(n < l) n = l;
	}
	k = WndColToPix((int)n);
	// WndWrite(WndFrame f, WndContextPtr gc, int lig, int col, char *texte)
	cdr = graph->cdr; f = cdr->f; dy = WndLigToPix(1);
	x = cdr->dh - k - 4; y = 2 * dy;
	if(f->qualite == WND_Q_PAPIER) {
		x -= WndColToPix(4); y += dy;
	}
	WndRectangle(f,0,x-2,y-2,k+4,(nb*dy)+4);
	for(i=0; i<nb; i++) { y += dy; WndString(f,0,x,y,&(texte[i][0])); }
	return(0);
}
/* ===================================================================== */
int PlotLineaire(void) {
	PlotEspace espace; int dim;
	int p,y;
	float hmin,hbin; int hdim,imin;
	int ix,iy; float rypnt;
	int posx,posy,larg,haut,px,py,dy;
	float *xadrs,*yadrs,xmin,xmax,grad;
	char *valide;
	int i,qual; char fit_fait;
	char nom_ab6[80],nom_ord[80];

	float a,b,erra,errb,khi2;
	float *eadrs;
	int var_erreur;
	char texte[80];

	if((p = PlotChoix()) < 0) return(0);
	OpiumGetPosition((Plot[p].g)->cdr,&posx,&posy); OpiumGetSize((Plot[p].g)->cdr,&larg,&haut);
	px = posx + larg + WndColToPix(3);
	dy = WndLigToPix(6);
	if(posy > dy) py = posy - dy; else py = posy + haut + WndLigToPix(3);

	espace = (PlotEspace)0;  /* pour gcc */
	if(Plot[p].type == PLOT_HISTO) {
		if(!HistoGetBinFloat(Plot[p].h,&hmin,&hbin,&hdim)) {
			OpiumFail("Histogramme %s vide",Plot[p].titre);
			return(0);
		}
		if(PlotFitMethode == PLOT_FIT_VALEURS) {
			if(OpiumDisplayed(pFitLimites->cdr)) OpiumClear(pFitLimites->cdr);
			if(OpiumExec(pFitLimites->cdr) == PNL_CANCEL) return(0);
		} else /* (PlotFitMethode == PLOT_FIT_GRAPHIC) */ {
			InfoWrite(iPlotFitConseil,1,"Pointes le minimum de la partie a fitter");
			InfoWrite(iPlotFitConseil,2,"(pour commencer)");
			OpiumPosition(iPlotFitConseil->cdr,posx,py);
			OpiumDisplay(iPlotFitConseil->cdr);
			if(!GraphGetPoint(Plot[p].g,&ix,&FitMini,&iy,&rypnt)) {
				FitMaxi = FitMini + (float)(3.0 * hbin);
				OpiumWarn("Echec du pointage du minimum. On garde [%g, %g]",FitMini,FitMaxi);
			}
			FitMaxi = FitMini+ (float)(3.0 * hbin); OpiumPosition(pFitLimites->cdr,px,posy); OpiumDisplay(pFitLimites->cdr);
			InfoWrite(iPlotFitConseil,1,"Minimum pointe: %g",FitMini);
			InfoWrite(iPlotFitConseil,2,"Pointes le MAXIMUM de la partie a fitter");
			OpiumRefresh(iPlotFitConseil->cdr);
			if(!GraphGetPoint(Plot[p].g,&ix,&FitMaxi,&iy,&rypnt)) {
				OpiumWarn("Echec du pointage du maximum. On garde [%g, %g]",FitMini,FitMaxi);
			}
			OpiumClear(iPlotFitConseil->cdr);
			OpiumRefresh(pFitLimites->cdr);
		}
		imin = (int)((FitMini - hmin) / hbin);
		dim = (int)((FitMaxi - hmin) / hbin) - imin + 1;
		if(dim < 2) {
			OpiumFail("Intervalle trop petit (%d points seulement)",dim);
			return(0);
		}
		if(!(valide = (char *)malloc(dim))) {
			OpiumFail("Manque de memoire pour <valide> (%d octets)",dim);
			return(0);
		}
		xmin = FitMini; xmax = FitMaxi;
		xadrs = (float *)malloc(dim * sizeof(float));
		if(!xadrs) {
			OpiumFail("Manque de memoire pour <xadrs> (%d octets)",dim * sizeof(float));
			free(valide);
			return(0);
		}
		yadrs = (float *)(Plot[p].hd)->adrs + imin;
		eadrs = (float *)malloc(dim * sizeof(float));
		if(!eadrs) {
			OpiumFail("Manque de memoire pour <eadrs> (%d octets)",dim * sizeof(float));
			free(valide); free(xadrs);
			return(0);
		}
		hmin = ((float)imin/* + 0.5 */) * hbin;
		for(i=0; i<dim; i++) { xadrs[i] = hmin + ((float)i * hbin); eadrs[i] = 1.0; }
		for(i=0; i<dim; i++) valide[i] = 1;
	} else if(Plot[p].type == PLOT_BIPLOT) {
		var_erreur = 0;
		sprintf(texte,"Erreur sur %s",PlotVar[Plot[p].ord].nom);
		if(OpiumReadList(texte,PlotVarList,&var_erreur,16) == PNL_CANCEL) return(0);
		espace = PlotVar[Plot[p].ab6].espace; dim = *(espace->dim);
		if(!(valide = (char *)malloc(dim))) {
			OpiumFail("Manque de memoire pour <valide> (%d octets)",dim);
			return(0);
		}
		GraphAxisFloatGet(Plot[p].g,GRF_XAXIS,&xmin,&xmax,&grad);
		xadrs = *PlotVar[Plot[p].ab6].adrs;
		yadrs = *PlotVar[Plot[p].ord].adrs;
		eadrs = *PlotVar[var_erreur].adrs;
		for(i=0; i<dim; i++)
			valide[i] = PlotNtuplePresent(espace,i)? 1: 0;
	} else {
		OpiumFail("Le fit lineaire ne gere pas (encore) ce type de trace");
		return(0);
	}

	fit_fait = FitLineaire(&b,&errb,&a,&erra,&khi2,xadrs,yadrs,eadrs,valide,dim);
	free(valide); if(Plot[p].type == PLOT_HISTO) { free(xadrs); free(eadrs); }
	if(!fit_fait) {
		OpiumFail("Fit impossible (%s)",FitError);
		return(1);
	}
	ResultatFit.lin.pente = a; ResultatFit.lin.base = b;
	ResultatFit.lin.err_pente = erra; ResultatFit.lin.err_base = errb;
	ResultatFit.lin.khi2 = khi2;
	PlotFitMode = FIT_LINEAIRE;
	GraphAjoute(Plot[p].g,PlotAfficheFit);
	OpiumPosition(pFitLin->cdr,px,posy);
	OpiumDisplay(pFitLin->cdr);

	/* d'un autre cote, cette methode tolere les changements d'axe par l'utilisateur...
	adrs = *PlotVar[Plot[p].ab6].adrs;
	xmin = xmax = *adrs;
	for(i=0; i<dim; i++,adrs++) {
		if(*adrs < xmin) xmin = *adrs;
		if(*adrs > xmax) xmax = *adrs;
	}
	*/
	Plot[p].xdroite[0] = xmin; Plot[p].ydroite[0] = (a * xmin) + b;
	Plot[p].xdroite[1] = xmax; Plot[p].ydroite[1] = (a * xmax) + b;

	if(OpiumDisplayed(Plot[p].g->cdr)) OpiumClear(Plot[p].g->cdr);
	if(Plot[p].avec_fit != FIT_LINEAIRE) {
		char nom[OPIUM_MAXNOM];
		strcpy(nom,(Plot[p].g->cdr)->nom);
		GraphErase(Plot[p].g);
		if(Plot[p].type == PLOT_HISTO) HistoGraphAdd(Plot[p].h,Plot[p].g);
		else if(Plot[p].type == PLOT_BIPLOT) {
			GraphAdd(Plot[p].g,GRF_FLOAT|GRF_XAXIS,*PlotVar[Plot[p].ab6].adrs,dim);
			GraphAdd(Plot[p].g,GRF_INDIV,espace->sel,dim);
			y = GraphAdd(Plot[p].g,GRF_FLOAT|GRF_YAXIS,*PlotVar[Plot[p].ord].adrs,dim);
			GraphDataTrace(Plot[p].g,y,GRF_PNT,GRF_POINT);
		}
		GraphAdd(Plot[p].g,GRF_FLOAT|GRF_XAXIS,Plot[p].xdroite,2);
		y = GraphAdd(Plot[p].g,GRF_FLOAT|GRF_YAXIS,Plot[p].ydroite,2);
		for(qual=0; qual<WND_NBQUAL; qual++)
			GraphDataSupportRGB(Plot[p].g,y,qual,PLOT_COLOR_FIT(qual));
		OpiumTitle(Plot[p].g->cdr,Plot[p].titre);
		OpiumName(Plot[p].g->cdr,nom);
#ifdef LABEL_AXES
		if(Plot[p].type == PLOT_HISTO) {
			PlotAxeNom(NumVar,nom_ab6,80);
			GraphAxisTitle(Plot[p].g,GRF_XAXIS,nom_ab6);
		} else {
			PlotAxeNom(NumAb6,nom_ab6,80);
			PlotAxeNom(NumOrd,nom_ord,80);
			GraphAxisTitle(Plot[p].g,GRF_XAXIS,nom_ab6);
			GraphAxisTitle(Plot[p].g,GRF_YAXIS,nom_ord);
		}
#endif
		Plot[p].vide = 0;
#ifdef LIMITES_AXES
		if(Plot[p].x.automatique)
			GraphAxisAutoRange(Plot[p].g,GRF_XAXIS);
		else  GraphAxisFloatRange(Plot[p].g,GRF_XAXIS,Plot[p].x.min,Plot[p].x.max);
		if(Plot[p].y.automatique)
			GraphAxisAutoRange(Plot[p].g,GRF_YAXIS);
		else  GraphAxisFloatRange(Plot[p].g,GRF_YAXIS,Plot[p].y.min,Plot[p].y.max);
#endif
		Plot[p].avec_fit = FIT_LINEAIRE;
	}

	OpiumDisplay(Plot[p].g->cdr);
	return(0);
}
/* ===================================================================== */
int PlotFitNonlin(int p, char type) {
	PlotEspace espace; int dim;
	float hmin,hbin; int hdim,imin;
	int ix,iy; float rypnt;
	int posx,posy,larg,haut,px,py,dy,decal;
	float *xadrs,*yadrs,*erreur,xmin,xmax,grad;
	char *valide;
	int i,y,qual; char fit_fait;
	float a,b,c;
	float fond,amplitude,sigma,x0;
	char nom_ab6[80],nom_ord[80];

	if(p < 0) p = PlotChoix();
	if(p < 0) return(0);
	OpiumGetPosition((Plot[p].g)->cdr,&posx,&posy); OpiumGetSize((Plot[p].g)->cdr,&larg,&haut);
	px = posx + larg + WndColToPix(3);
	dy = WndLigToPix(6);
	if(posy > dy) py = posy - dy; else py = posy + haut + WndLigToPix(3);

	espace = (PlotEspace)0;  /* pour gcc */
	if(Plot[p].type == PLOT_HISTO) {
		if(!HistoGetBinFloat(Plot[p].h,&hmin,&hbin,&hdim)) {
			OpiumFail("Histogramme %s vide",Plot[p].titre);
			return(0);
		}
		if(PlotFitMethode == PLOT_FIT_VALEURS) {
			if(OpiumDisplayed(pFitLimites->cdr)) OpiumClear(pFitLimites->cdr);
			if(OpiumExec(pFitLimites->cdr) == PNL_CANCEL) return(0);
		} else /* (PlotFitMethode == PLOT_FIT_GRAPHIC) */ {
			InfoWrite(iPlotFitConseil,1,"Pointes le minimum de la partie a fitter");
			InfoWrite(iPlotFitConseil,2,"(pour commencer)");
			OpiumPosition(iPlotFitConseil->cdr,posx,py);
			OpiumDisplay(iPlotFitConseil->cdr);
			if(!GraphGetPoint(Plot[p].g,&ix,&FitMini,&iy,&rypnt)) {
				FitMaxi = FitMini+ ((float)3.0 * hbin);
				OpiumWarn("Echec du pointage du minimum. On garde [%g, %g]",FitMini,FitMaxi);
			}
			FitMaxi = FitMini+ ((float)3.0 * hbin); OpiumPosition(pFitLimites->cdr,px,posy); OpiumDisplay(pFitLimites->cdr);
			InfoWrite(iPlotFitConseil,1,"Minimum pointe: %g",FitMini);
			InfoWrite(iPlotFitConseil,2,"Pointes le MAXIMUM de la partie a fitter");
			OpiumRefresh(iPlotFitConseil->cdr);
			if(!GraphGetPoint(Plot[p].g,&ix,&FitMaxi,&iy,&rypnt)) {
				OpiumWarn("Echec du pointage du maximum. On garde [%g, %g]",FitMini,FitMaxi);
			}
			OpiumClear(iPlotFitConseil->cdr);
			OpiumRefresh(pFitLimites->cdr);
		}
		imin = (int)((FitMini - hmin) / hbin);
		dim = (int)((FitMaxi - hmin) / hbin) - imin + 1;
		if(dim < 3) {
			OpiumFail("Intervalle trop petit (%d points seulement)",dim);
			return(0);
		}
		if(!(valide = (char *)malloc(dim))) {
			OpiumFail("Manque de memoire pour la gestion (%d octets)",dim);
			return(0);
		}
		xmin = FitMini; xmax = FitMaxi;
		xadrs = (float *)malloc(dim * sizeof(float));
		if(!xadrs) {
			OpiumFail("Manque de memoire pour <xadrs> (%d octets)",dim * sizeof(float));
			free(valide);
			return(0);
		}
		erreur = (float *)malloc(dim * sizeof(float));
		if(!erreur) {
			OpiumFail("Manque de memoire pour <erreur> (%d octets)",dim * sizeof(float));
			free(xadrs);
			free(valide);
			return(0);
		}
		yadrs = (float *)(Plot[p].hd)->adrs + imin;
		hmin = hmin + ((float)imin/* + 0.5 */) * hbin;
		for(i=0; i<dim; i++) {
			xadrs[i] = hmin + ((float)i * hbin);
			erreur[i] = 1.0;
			valide[i] = 1;
		}
	} else if(Plot[p].type == PLOT_BIPLOT) {
		espace = PlotVar[Plot[p].ab6].espace; dim = *(espace->dim);
		if(!(valide = (char *)malloc(dim))) {
			OpiumFail("Manque de memoire pour <valide> (%d octets)",dim);
			return(0);
		}
		erreur = (float *)malloc(dim * sizeof(float));
		if(!erreur) {
			OpiumFail("Manque de memoire pour <erreur> (%d octets)",dim * sizeof(float));
			free(valide);
			return(0);
		}
		GraphAxisFloatGet(Plot[p].g,GRF_XAXIS,&xmin,&xmax,&grad);
		xadrs = *PlotVar[Plot[p].ab6].adrs;
		yadrs = *PlotVar[Plot[p].ord].adrs;
		for(i=0; i<dim; i++) {
			erreur[i] = 1.0;
			valide[i] = PlotNtuplePresent(espace,i)? 1: 0;
		}
	} else {
		OpiumWarn("Le fit non lineaire ne gere pas (encore) ce type de trace");
		return(0);
	}

	if(type == FIT_PARABOLIQUE) {
		fit_fait = FitSecondDegre(xadrs,yadrs,erreur,valide,dim,&a,&b,&c);
		fond = 0.0;  /* apres tout... */
	} else /* if(type == FIT_GAUSSIEN) */ {
/*		fond = (yadrs[0] + yadrs[dim - 1]) / 2.0; */
		fond = (yadrs[0] < yadrs[dim - 1])? yadrs[0]: yadrs[dim - 1]; /* en attendant mieux */
		fond = 0.0;  /* apres tout... */
		fit_fait = FitGaussienne(xadrs,yadrs,valide,dim,fond,&amplitude,&sigma,&x0);
	}
	free(valide); free(erreur); if(Plot[p].type == PLOT_HISTO) free(xadrs);
	decal = 0;
	if(fit_fait) {
		if(type == FIT_PARABOLIQUE) {
			ResultatFit.par.a = a;
			ResultatFit.par.b = b;
			ResultatFit.par.c = c;
			if(OpiumDisplayed(pFitGauss->cdr)) OpiumClear(pFitGauss->cdr);
			if(a != 0.0) {
				ResultatFit.par.x0 = -b / ((float)2.0 * a);
				ResultatFit.par.base = c - ((b * b) / ((float)4.0 * a));
				PlotFitMode = FIT_PARABOLIQUE;
				GraphAjoute(Plot[p].g,PlotAfficheFit);
				OpiumPosition(pFitParab->cdr,px,posy);
				if(OpiumDisplayed(pFitDegre2->cdr)) OpiumClear(pFitDegre2->cdr);
				if(OpiumDisplayed(pFitParab->cdr)) OpiumRefresh(pFitParab->cdr);
				else OpiumDisplay(pFitParab->cdr);
			} else {
				OpiumPosition(pFitDegre2->cdr,px,posy);
				if(OpiumDisplayed(pFitParab->cdr)) OpiumClear(pFitParab->cdr);
				if(OpiumDisplayed(pFitDegre2->cdr)) OpiumRefresh(pFitDegre2->cdr);
				else OpiumDisplay(pFitDegre2->cdr);
			}
		} else /* if(type == FIT_GAUSSIEN) */ {
			ResultatFit.gau.amplitude = amplitude;
			ResultatFit.gau.sigma = sigma;
			ResultatFit.gau.x0 = x0;
			ResultatFit.gau.base = fond;
			if(ResultatFit.gau.x0 != 0.0) {
				PlotResolSigma = (float)100.0 * ResultatFit.gau.sigma / ResultatFit.gau.x0;
				PlotResolFWHM = FWHM_POURCENT * ResultatFit.gau.sigma / ResultatFit.gau.x0;
			} else PlotResolSigma = PlotResolFWHM = 0.0;
			PlotFitMode = FIT_GAUSSIEN;
			GraphAjoute(Plot[p].g,PlotAfficheFit);
			OpiumPosition(pFitGauss->cdr,px,posy); decal = WndLigToPix(9);
			if(OpiumDisplayed(pFitDegre2->cdr)) OpiumClear(pFitDegre2->cdr);
			if(OpiumDisplayed(pFitParab->cdr)) OpiumClear(pFitParab->cdr);
			if(OpiumDisplayed(pFitGauss->cdr)) OpiumRefresh(pFitGauss->cdr);
			else OpiumDisplay(pFitGauss->cdr);
		}

		if(Plot[p].dim != dim) {
			if(Plot[p].type == PLOT_HISTO) {
				if(Plot[p].xcourbe) free(Plot[p].xcourbe);
				Plot[p].xcourbe = (float *)malloc(dim * sizeof(float));
				if(!Plot[p].xcourbe) {
					OpiumFail("Manque de memoire pour <xcourbe> (%d octets)",dim * sizeof(float));
					return(0);
				}
			}
			if(Plot[p].ycourbe) { free(Plot[p].ycourbe); Plot[p].dim = 0; }
			Plot[p].ycourbe = (float *)malloc(dim * sizeof(float));
			if(!Plot[p].ycourbe) {
				OpiumFail("Manque de memoire pour <ycourbe> (%d octets)",dim * sizeof(float));
				return(0);
			}
			Plot[p].avec_fit = FIT_NEANT;  /* pour forcer la reconnexion sur ycourbe (et xcourbe) */
			Plot[p].dim = dim;
		}
		if(Plot[p].type == PLOT_HISTO) {
			for(i=0; i<dim; i++) Plot[p].xcourbe[i] = hmin + ((float)i * hbin);
			xadrs = Plot[p].xcourbe;
		} else Plot[p].xcourbe = xadrs;
		yadrs = Plot[p].ycourbe;
		if(type == FIT_PARABOLIQUE) for(i=0; i<dim; i++,xadrs++,yadrs++)
			*yadrs = (a * *xadrs * *xadrs) + (b * *xadrs) + c;
		else /* if(type == FIT_GAUSSIEN) */ for(i=0; i<dim; i++,xadrs++,yadrs++)
			*yadrs = fond + (amplitude * expf(-(*xadrs - x0) * (*xadrs - x0)/((float)2.0 * sigma * sigma)));

		if(OpiumDisplayed(Plot[p].g->cdr)) OpiumClear(Plot[p].g->cdr);
		if(Plot[p].avec_fit != type) {
			char nom[OPIUM_MAXNOM];
			strcpy(nom,(Plot[p].g->cdr)->nom);
			GraphErase(Plot[p].g);
			if(Plot[p].type == PLOT_HISTO) HistoGraphAdd(Plot[p].h,Plot[p].g);
			else {
				GraphAdd(Plot[p].g,GRF_FLOAT|GRF_XAXIS,*PlotVar[Plot[p].ab6].adrs,dim);
				GraphAdd(Plot[p].g,GRF_INDIV,espace->sel,dim);
				y = GraphAdd(Plot[p].g,GRF_FLOAT|GRF_YAXIS,*PlotVar[Plot[p].ord].adrs,dim);
				GraphDataTrace(Plot[p].g,y,GRF_PNT,GRF_POINT);
			}
			GraphAdd(Plot[p].g,GRF_FLOAT|GRF_XAXIS,Plot[p].xcourbe,dim);
			y = GraphAdd(Plot[p].g,GRF_FLOAT|GRF_YAXIS,Plot[p].ycourbe,dim);
			// for(qual=0; qual<WND_NBQUAL; qual++)
			// 	printf("(%s) fit %s: %04X.%04X.%04X\n",__func__,WndSupportNom[qual],PLOT_COLOR_FIT(qual));
			for(qual=0; qual<WND_NBQUAL; qual++)
				GraphDataSupportRGB(Plot[p].g,y,qual,PLOT_COLOR_FIT(qual));
			OpiumTitle(Plot[p].g->cdr,Plot[p].titre);
			OpiumName(Plot[p].g->cdr,nom);
		#ifdef LABEL_AXES
			if(Plot[p].type == PLOT_HISTO) {
				PlotAxeNom(NumVar,nom_ab6,80);
				GraphAxisTitle(Plot[p].g,GRF_XAXIS,nom_ab6);
			} else {
				PlotAxeNom(NumAb6,nom_ab6,80);
				PlotAxeNom(NumOrd,nom_ord,80);
				GraphAxisTitle(Plot[p].g,GRF_XAXIS,nom_ab6);
				GraphAxisTitle(Plot[p].g,GRF_YAXIS,nom_ord);
			}
		#endif
			Plot[p].vide = 0;
		#ifdef LIMITES_AXES
			if(Plot[p].x.automatique)
				GraphAxisAutoRange(Plot[p].g,GRF_XAXIS);
			else  GraphAxisFloatRange(Plot[p].g,GRF_XAXIS,Plot[p].x.min,Plot[p].x.max);
			if(Plot[p].y.automatique)
				GraphAxisAutoRange(Plot[p].g,GRF_YAXIS);
			else  GraphAxisFloatRange(Plot[p].g,GRF_YAXIS,Plot[p].y.min,Plot[p].y.max);
		#endif
			Plot[p].avec_fit = type;
		}
	} else OpiumWarn("Fit impossible (%s)",FitError);
	if(type == FIT_GAUSSIEN) {
		float x,ponderee,carres;

		if(!HistoGetBinFloat(Plot[p].h,&hmin,&hbin,&hdim)) {
			OpiumWarn("Histogramme %s vide",Plot[p].titre);
			return(0);
		}
		// printf("* Statistiques sur l'histo %s [%g .. %g]\n",Plot[p].titre,FitMini,FitMaxi);
		imin = (int)((FitMini - hmin) / hbin);
		dim = (int)((FitMaxi - hmin) / hbin) - imin + 1;
		if((imin + dim) > hdim) dim = hdim - imin;
		yadrs = (float *)(Plot[p].hd)->adrs + imin;

		PlotStat.somme = 0.0;
		PlotStat.xmax = 0.0;
		PlotStat.ymax = 0.0;
		PlotStat.median = 0.0;
		PlotStat.moyenne = 0.0;
		PlotStat.sigma = 0.0;
		PlotStat.bin = hbin;

		ponderee = carres = 0.0;
		for(i=0; i<dim; i++) {
			PlotStat.somme += *yadrs;
			x = (float)i /* + 0.5 */;
			ponderee += (x * *yadrs);
			carres += (x * x * *yadrs);
			if(*yadrs > PlotStat.ymax) {
				PlotStat.xmax = (x * hbin) + FitMini;
				PlotStat.ymax = *yadrs;
			}
			yadrs++;
		}
		PlotStat.moyenne = (ponderee / PlotStat.somme);
		PlotStat.sigma = sqrtf((carres / PlotStat.somme) - (PlotStat.moyenne * PlotStat.moyenne));
		PlotStat.moyenne = (PlotStat.moyenne * hbin) + FitMini;
		PlotStat.sigma *= hbin;
		if(PlotStat.moyenne != 0.0) PlotResolStat = FWHM_POURCENT * PlotStat.sigma / PlotStat.moyenne;
		else PlotResolStat = 0.0;
		// printf("= %g +/- %g (%g FWHM)\n",PlotStat.moyenne,PlotStat.sigma,PlotResolStat);
		OpiumPosition(pPlotStats->cdr,px,posy + decal);
		// printf("> Resultat place en (%d, %d)\n",px,posy);
		OpiumDisplay(pPlotStats->cdr);
	}

	if(OpiumDisplayed(pFitLimites->cdr)) OpiumClear(pFitLimites->cdr);
	OpiumDisplay(Plot[p].g->cdr);

	return(0);
}
/* ===================================================================== */
int PlotParabolique(void) { return(PlotFitNonlin(-1,FIT_PARABOLIQUE)); }
/* ===================================================================== */
int PlotGaussien(void) { return(PlotFitNonlin(-1,FIT_GAUSSIEN)); }
/* ===================================================================== */
static int PlotFitGaussImprime(void) {
	Cadre *cdr,liste[8];

	cdr = liste;
	*cdr++ = Plot[PlotCourant].g->cdr;
	*cdr++ = pFitGauss->cdr;
	*cdr++ = pPlotStats->cdr;
	*cdr = 0;
	PlotImprimeListe(liste);
	return(0);
}
/* ===================================================================== */
int PlotLimite(void) {
	int p; unsigned char type;
	int zmin,zmax;

	if((p = PlotChoix()) < 0) return(0);
	type = Plot[p].type;

	memcpy(&Xlimite,&(Plot[p].x),sizeof(TypeLimite));
	memcpy(&Ylimite,&(Plot[p].y),sizeof(TypeLimite));
	Xlimite.automatique = Ylimite.automatique = 0;
	if(type == PLOT_H2D) {
		memcpy(&Zlimite,&(Plot[p].z),sizeof(TypeLimite));
		if(OpiumExec(pPlotLimit3D->cdr) == PNL_CANCEL) return(0);
		memcpy(&(Plot[p].z),&Zlimite,sizeof(TypeLimite));
	} else {
		if(OpiumExec(pPlotLimites->cdr) == PNL_CANCEL) return(0);
	}
	memcpy(&(Plot[p].x),&Xlimite,sizeof(TypeLimite));
	memcpy(&(Plot[p].y),&Ylimite,sizeof(TypeLimite));

	if(Plot[p].x.automatique)
		GraphAxisAutoRange(Plot[p].g,GRF_XAXIS);
	else  GraphAxisFloatRange(Plot[p].g,GRF_XAXIS,Plot[p].x.min,Plot[p].x.max);
	if(Plot[p].y.automatique)
		GraphAxisAutoRange(Plot[p].g,GRF_YAXIS);
	else  GraphAxisFloatRange(Plot[p].g,GRF_YAXIS,Plot[p].y.min,Plot[p].y.max);
	if(type == PLOT_H2D) {
		if(Plot[p].z.automatique) {
			H2DGetExtremaInt((H2D)Plot[p].h,&zmin,&zmax);
			Plot[p].z.min = (float)zmin;
			Plot[p].z.max = (float)zmax;
			GraphDataIntGamma(Plot[p].g,2,0,zmax,0,OPIUM_LUT_DIM-1);
		} else {
			zmin = (int)Plot[p].z.min;
			zmax = (int)Plot[p].z.max;
			GraphDataIntGamma(Plot[p].g,2,zmin,zmax,0,OPIUM_LUT_DIM-1);
		}
	}

	OpiumDisplay(Plot[p].g->cdr);
	return(0);
}
/* ===================================================================== */
int PlotStatsCalcule(int p, float mini, float maxi) {
	int dim; int i;
	float hmin,hbin; int hdim,imin;
	float *yadrs;
	float x,ponderee,carres;

	if(!HistoGetBinFloat(Plot[p].h,&hmin,&hbin,&hdim)) {
		OpiumFail("Histogramme %s vide",Plot[p].titre);
		return(0);
	}
	imin = (int)((mini - hmin) / hbin);
	dim = (int)((maxi - hmin) / hbin) - imin + 1;
	if((imin + dim) > hdim) dim = hdim - imin;
	yadrs = (float *)(Plot[p].hd)->adrs + imin;

	PlotStat.somme = 0.0;
	PlotStat.xmax = 0.0;
	PlotStat.ymax = 0.0;
	PlotStat.median = 0.0;
	PlotStat.moyenne = 0.0;
	PlotStat.sigma = 0.0;
	PlotStat.bin = hbin;

	ponderee = carres = 0.0;
	for(i=0; i<dim; i++) {
		PlotStat.somme += *yadrs;
		x = (float)i /* + 0.5 */;
		ponderee += (x * *yadrs);
		carres += (x * x * *yadrs);
		if(*yadrs > PlotStat.ymax) {
			PlotStat.xmax = (x * hbin) + mini;
			PlotStat.ymax = *yadrs;
		}
		yadrs++;
	}
	PlotStat.moyenne = (ponderee / PlotStat.somme);
	PlotStat.sigma = sqrtf((carres / PlotStat.somme) - (PlotStat.moyenne * PlotStat.moyenne));
	PlotStat.moyenne = (PlotStat.moyenne * hbin) + mini;
	PlotStat.sigma *= hbin;
	if(PlotStat.moyenne != 0.0) PlotResolStat = FWHM_POURCENT * PlotStat.sigma / PlotStat.moyenne;
	else PlotResolStat = 0.0;
	return(0);
}
/* ===================================================================== */
int PlotStatsDemande(Menu menu, MenuItem item) {
	int p;

	if((p = PlotChoixFig1D()) < 0) return(0);
	if(Plot[p].type != PLOT_HISTO) {
		OpiumFail("Le plot %s n'est pas un histo 1D",Plot[p].titre);
		return(1);
	}
	if(OpiumExec(pFitLimites->cdr) == PNL_CANCEL) return(0);
	PlotStatsCalcule(p,FitMini,FitMaxi);
	OpiumDisplay(pPlotStats->cdr);

	return(0);
}
/* ===================================================================== */
int PlotAnnule(int p) {
	int i;

	if(p == PlotsNb) {
		for(i=0; i<PlotsNb; i++) if(!Plot[i].vide) {
			if(Plot[i].g) {
				if(OpiumDisplayed(Plot[i].g->cdr)) OpiumClear(Plot[i].g->cdr);
				GraphDelete(Plot[i].g);
				Plot[i].g = 0;
			}
			if(Plot[i].hd) { HistoDataDelete(Plot[i].hd); Plot[i].hd = 0; }
			if(Plot[i].h) { HistoDelete(Plot[i].h); Plot[i].h = 0; }
			Plot[i].vide = 1;
			sprintf(Plot[i].titre,"%d: plot vide",i+1);
			PlotName[i] = "\0";
		}
		PlotsNb = 0;
	} else if(!Plot[p].vide) {
		if(Plot[p].g) {
			if(OpiumDisplayed(Plot[p].g->cdr)) OpiumClear(Plot[p].g->cdr);
			GraphDelete(Plot[p].g);
			Plot[p].g = 0;
		}
		if(Plot[p].hd) { HistoDataDelete(Plot[p].hd); Plot[p].hd = 0; }
		if(Plot[p].h) { HistoDelete(Plot[p].h); Plot[p].h = 0; }
		Plot[p].vide = 1;
		sprintf(Plot[p].titre,"%d: plot vide",p+1);
		PlotName[p] = Plot[p].titre;
	}
	return(0);
}
/* ===================================================================== */
int PlotRepertoire(char *nom) {
	/* ATTENTION: doit etre appele APRES PlotInit */
	strncpy(PlotRepert,nom,MAXFILENAME);
	return(0);
}
/* ===================================================================== */
int PlotSauve(void) {
	char *liste[GRAFMAX+1],nom[NOMMAX]; int nb,i,j;
	char nom_complet[MAXFILENAME];

	if(!PlotsNb) { OpiumError("Pas de plot a sauver"); return(0); }
	RepertoireListeCree(0,PlotRepert,liste,GRAFMAX,&nb);
	if(nb) {
		liste[nb] = "(nouveau)";
		liste[nb+1] = "\0";
		i = nb - 1; // nb;
		if(OpiumReadList("jeu de graphiques existant",liste,&i,NOMMAX) == PNL_CANCEL) return(0);
	} else i = 0;
	if(i == nb) {
		sprintf(nom,"Jeu_%02d",i+1);
		if(OpiumReadText("nom du nouveau jeu de graphiques",nom,NOMMAX) == PNL_CANCEL) return(0);
		for(j=0; j<nb; j++) if(!strcmp(liste[j],nom)) {
			OpiumError("Ce nom existe deja!");
			return(0);
		}
		strcat(strcpy(nom_complet,PlotRepert),nom);
		liste[nb] = nom;
	}
	strcat(strcpy(nom_complet,PlotRepert),liste[i]);
	if(ArgPrint(nom_complet,PlotListeDesc) <= 0) printf("! Sauvegarde impossible (%s)\n",strerror(errno));
	else printf("* %d description%s de graphique sauvee%s dans '%s'\n",Accord2s(PlotsNb),nom_complet);
	return(0);
}
/* ===================================================================== */
int PlotRestore(void) {
	char *liste[GRAFMAX]; int nb,i,p;
	char nom_complet[MAXFILENAME];

	RepertoireListeCree(0,PlotRepert,liste,GRAFMAX,&nb);
	if(!nb) {
		OpiumError("Pas de jeu de graphiques sauve"); return(0);
	}
	i = 0;
	if(OpiumReadList("jeu de graphiques",liste,&i,NOMMAX) == PNL_CANCEL) return(0);
	PlotAnnule(PlotsNb);
	strcat(strcpy(nom_complet,PlotRepert),liste[i]);
	if(ArgScan(nom_complet,PlotListeDesc) <= 0) printf("! Relecture impossible (%s)\n",strerror(errno));
	else printf("* %d graphique%s relu%s dans '%s'\n",Accord2s(PlotsNb),nom_complet);
	for(p=0; p<PlotsNb; p++) PlotConstruit(1,p);
	return(0);
}
/* ===================================================================== */
int PlotErase(void) {
	int i,m;

	m = PlotsNb;
	PlotName[m] = "tous";
	PlotName[m + 1] = "\0";
	for(i=0; i<PlotsNb; i++) if(!Plot[i].vide) break;
	OpiumReadList("Lequel",PlotName,&i,16);
	PlotAnnule(i);
//	PlotName[m] = Plot[m].titre;
//	PlotName[m + 1] = Plot[m + 1].titre;
	PlotName[m] = "\0";
	return(0);
}
/* ===================================================================== */
int PlotCourbeEdite(char creation) {
	int i,j,p,n; void *adrs; float *courbe,x,xn,dx;

	if(OpiumExec(pPlotCourbeDef->cdr) == PNL_CANCEL) return(0);
	if(PlotCourbeMod.pts < 2) {
		OpiumError("Il faut au moins 2 points pour tracer");
		return(0);
	}
	for(i=0; i<PlotCourbeNb; i++) {
		if(PlotCourbeMod.nom[0] == '\0') {
			OpiumError("Il FAUT donner un nom");
			return(0);
		}
		if(!creation && (i == PlotCourbeNum)) continue;
		if(!strcmp(PlotCourbeMod.nom,PlotCourbe[i].nom)) {
			OpiumError("La courbe #%d a deja ce nom",i+1);
			return(0);
		}
	}

	PanelErase(pPlotCourbeCoef);
	switch(PlotCourbeMod.type) {
	  case PLOT_POLYNOME:
		if((PlotCourbeMod.nb <= 0) || (PlotCourbeMod.nb > PLOT_MAXCOEFS)) {
			PlotCourbeMod.nb = PLOT_MAXCOEFS;
			OpiumError("Tu en demandes trop! (maximum acceptable: %d)",PLOT_MAXCOEFS);
			return(0);
		}
		sprintf(PlotCourbeCoefNom[0],"=");
		if(PlotCourbeMod.nb > 1) sprintf(PlotCourbeCoefNom[1],"+ (x-x0) *");
		for(i=2; i<PlotCourbeMod.nb; i++) sprintf(PlotCourbeCoefNom[i],"+ (x-x0)**%d *",i);
		for(i=0; i<PlotCourbeMod.nb; i++)
			PanelFloat(pPlotCourbeCoef,PlotCourbeCoefNom[i],&(PlotCourbeMod.coef[i]));
		PanelFloat(pPlotCourbeCoef,"avec x0",&(PlotCourbeMod.x0));
		break;
	  case PLOT_HYPERBOLE:
		if((PlotCourbeMod.nb <= 0) || (PlotCourbeMod.nb > PLOT_MAXCOEFS)) {
			PlotCourbeMod.nb = PLOT_MAXCOEFS;
			OpiumError("Tu en demandes trop! (maximum acceptable: %d)",PLOT_MAXCOEFS);
			return(0);
		}
		sprintf(PlotCourbeCoefNom[0],"=");
		if(PlotCourbeMod.nb > 1) sprintf(PlotCourbeCoefNom[1],"+ 1/(x-x0) *");
		for(i=2; i<PlotCourbeMod.nb; i++) sprintf(PlotCourbeCoefNom[i],"+ 1/(x-x0)**%d *",i);
		for(i=0; i<PlotCourbeMod.nb; i++)
			PanelFloat(pPlotCourbeCoef,PlotCourbeCoefNom[i],&(PlotCourbeMod.coef[i]));
		PanelFloat(pPlotCourbeCoef,"avec x0",&(PlotCourbeMod.x0));
		break;
	  case PLOT_EXP:
		break;
	  case PLOT_LOG:
		break;
	  case PLOT_GAUSS:
		break;
	}
	if(OpiumExec(pPlotCourbeCoef->cdr) == PNL_CANCEL) return(0);

	if(!creation) {
		for(p=0; p<PlotsNb; p++) {
			n = GraphDimGet(Plot[p].g);
			for(i=0; i<n; i++) {
				adrs = GraphDataGet(Plot[p].g,i,&j);
				if(adrs == PlotCourbeMod.donnees) {
					GraphDataRemove(Plot[p].g,i-1);
					GraphDataRemove(Plot[p].g,i-1); /* eh oui, car on vient de tout faire descendre */
				}
			}
		}
		free(PlotCourbeMod.donnees);
	}
	courbe = (float *)malloc((2 + PlotCourbeMod.pts) * sizeof(float));
	if(!courbe) {
		OpiumError("Pas assez de memoire pour la courbe en cours");
		return(0);
	}
	dx = (PlotCourbeMod.max - PlotCourbeMod.min) / (float)(PlotCourbeMod.pts - 1);
	switch(PlotCourbeMod.type) {
	  case PLOT_POLYNOME:
		for(i=0, x=PlotCourbeMod.min; i<PlotCourbeMod.pts; i++, x+=dx) {
			courbe[i] = 0.0; xn = 1.0;
			for(j=0; j<PlotCourbeMod.nb; j++) {
				courbe[i] += (PlotCourbeMod.coef[j] * xn);
				xn *= (x - PlotCourbeMod.x0);
			}
		}
		break;
	  case PLOT_HYPERBOLE:
		for(i=0, x=PlotCourbeMod.min; i<PlotCourbeMod.pts; i++, x+=dx) {
			courbe[i] = 0.0; xn = 1.0;
			for(j=0; j<PlotCourbeMod.nb; j++) {
				courbe[i] += (PlotCourbeMod.coef[j] * xn);
				xn /= (x - PlotCourbeMod.x0);
			}
		}
		break;
	  case PLOT_EXP:
		break;
	  case PLOT_LOG:
		break;
	  case PLOT_GAUSS:
		break;
	}
	courbe[i++] = PlotCourbeMod.min;
	courbe[i++] = dx;
	PlotCourbeMod.donnees = courbe;
	memcpy(&(PlotCourbe[PlotCourbeNum]),&PlotCourbeMod,sizeof(struct StructPlotCourbe));

	return(1);
}
/* ===================================================================== */
int PlotCourbeCree(void) {
	if(PlotCourbeNb >= PLOT_MAXCOURBES) {
		OpiumError("Pas plus de %d courbes",PLOT_MAXCOURBES);
		return(0);
	}
	PlotCourbeNum = PlotCourbeNb;
	if(PlotCourbeEdite(1)) {
		PlotCourbeNb++;
		if(PlotCourbeNb < PLOT_MAXCOURBES) PlotCourbe[PlotCourbeNb].nom[0] = '\0';
	}
	return(0);
}
/* ===================================================================== */
int PlotCourbeModifie(void) {
	if(OpiumReadList("Laquelle",PlotCourbeNoms,&PlotCourbeNum,PLOT_MAXCOURBENOM) == PNL_CANCEL) return(0);
	memcpy(&PlotCourbeMod,&(PlotCourbe[PlotCourbeNum]),sizeof(struct StructPlotCourbe));
	PlotCourbeEdite(0);
	return(0);
}
/* ===================================================================== */
char PlotCourbeSupprime(void) {
	int i,j,p,n; void *adrs;

	if(OpiumReadList("Laquelle",PlotCourbeNoms,&PlotCourbeNum,PLOT_MAXCOURBENOM) == PNL_CANCEL) return(0);
	memcpy(&PlotCourbeMod,&(PlotCourbe[PlotCourbeNum]),sizeof(struct StructPlotCourbe));
	for(p=0; p<PlotsNb; p++) {
		n = GraphDimGet(Plot[p].g);
		for(i=0; i<n; i++) {
			adrs = GraphDataGet(Plot[p].g,i,&j);
			if(adrs == PlotCourbeMod.donnees) {
				GraphDataRemove(Plot[p].g,i-1);
				GraphDataRemove(Plot[p].g,i-1); /* eh oui, car on vient de tout faire descendre */
			}
		}
	}
	free(PlotCourbeMod.donnees);
	PlotCourbeNb--;
	for(i=PlotCourbeNum; i<PlotCourbeNb; i++)
		memcpy(&(PlotCourbe[i]),&(PlotCourbe[i+1]),sizeof(struct StructPlotCourbe));
	PlotCourbe[PlotCourbeNb].nom[0] = '\0';
	return(0);
}
/* ===================================================================== */
int PlotCourbeAffiche(void) {
	int i,j; char suite;

	OpiumDisplay(tPlot->cdr);
	TermEmpty(tPlot);
	TermHold(tPlot);
	for(i=0; i<PlotCourbeNb; i++) {
		suite = 0;
		TermPrint(tPlot,"%3d/ %16s =",i+1,PlotCourbe[i].nom);
		for(j=0; j<PlotCourbe[i].nb; j++) {
			if(PlotCourbe[i].coef[j] == 0.0) continue;
			if(suite) {
				if(PlotCourbe[i].coef[j] > 0.0) TermPrint(tPlot," +");
				else TermPrint(tPlot," ");
			}
			if((PlotCourbe[i].type != PLOT_POLYNOME) || (PlotCourbe[i].coef[j] != 1.0))
				TermPrint(tPlot," %g",PlotCourbe[i].coef[j]);
			else TermPrint(tPlot," ");
			if(j > 0) {
				if(PlotCourbe[i].x0 == 0.0) {
					switch(PlotCourbe[i].type) {
					  case PLOT_POLYNOME: TermPrint(tPlot,"x"); break;
					  case PLOT_HYPERBOLE: TermPrint(tPlot,"/x"); break;
					}
				} else {
					switch(PlotCourbe[i].type) {
					  case PLOT_POLYNOME: TermPrint(tPlot,"(x-x0)"); break;
					  case PLOT_HYPERBOLE: TermPrint(tPlot,"/(x-x0)"); break;
					}
				}
				if(j > 1) TermPrint(tPlot,"%d",j);
			}
			suite = 1;
		}
		if(PlotCourbe[i].x0 == 0.0) TermPrint(tPlot,"\n");
		else TermPrint(tPlot," {x0=%g}\n",PlotCourbe[i].x0);
		TermPrint(tPlot,"                        x:[%g %g]",PlotCourbe[i].min,PlotCourbe[i].max);
		TermPrint(tPlot," sur %d points / couleur=%04X/%04X/%04X\n",PlotCourbe[i].pts,
			PlotCourbe[i].couleur.r,PlotCourbe[i].couleur.g,PlotCourbe[i].couleur.b);
	}
	TermPrint(tPlot,"---------- (%d courbe%s) ----------\n",PlotCourbeNb,
		(PlotCourbeNb>1)?"s":"");
	TermRelease(tPlot);
	return(0);
}
/* ===================================================================== */
char PlotCourbeAjoute(void) {
	int p,y; int dim;

	if(!PlotsNb) { OpiumError("Pas de plot utilisable"); return(-1); }
	if(PlotCourant >= PlotsNb) PlotCourant = PlotsNb - 1;
	p = PlotCourant;
	if(OpiumExec(pPlotCourbeGraph->cdr) == PNL_CANCEL) { PlotCourant = p; return(0); }
	if((PlotCourant < 0) || (PlotCourant >= PlotsNb)) {
		OpiumError("Le plot #%d n'a pas encore ete trace",PlotCourant);
		PlotCourant = p;
		return(-1);
	}
	if(Plot[PlotCourant].vide) {
		OpiumError("Le plot '%s' est vide",Plot[PlotCourant].titre);
		PlotCourant = p;
		return(-1);
	}
	dim = PlotCourbe[PlotCourbeNum].pts;
	GraphAdd(Plot[PlotCourant].g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,PlotCourbe[PlotCourbeNum].donnees+dim,dim);
	y = GraphAdd(Plot[PlotCourant].g,GRF_FLOAT|GRF_YAXIS,PlotCourbe[PlotCourbeNum].donnees,dim);
	GraphDataRGB(Plot[PlotCourant].g,y,
		PlotCourbe[PlotCourbeNum].couleur.r,PlotCourbe[PlotCourbeNum].couleur.g,PlotCourbe[PlotCourbeNum].couleur.b);
	OpiumRefresh((Plot[PlotCourant].g)->cdr);
	return(0);
}
/* ===================================================================== */
char PlotCourbeEfface(void) {
	int i,j,p,n; void *adrs;

	if(!PlotsNb) { OpiumError("Pas de plot utilisable"); return(-1); }
	if(PlotCourant >= PlotsNb) PlotCourant = PlotsNb - 1;
	p = PlotCourant;
	if(OpiumExec(pPlotCourbeGraph->cdr) == PNL_CANCEL) { PlotCourant = p; return(0); }
	if((PlotCourant < 0) || (PlotCourant >= PlotsNb)) {
		OpiumError("Le plot #%d n'a pas encore ete trace",PlotCourant);
		PlotCourant = p;
		return(-1);
	}
	if(Plot[PlotCourant].vide) {
		OpiumError("Le plot '%s' est vide",Plot[PlotCourant].titre);
		PlotCourant = p;
		return(-1);
	}
	memcpy(&PlotCourbeMod,&(PlotCourbe[PlotCourbeNum]),sizeof(struct StructPlotCourbe));
	n = GraphDimGet(Plot[PlotCourant].g);
	for(i=0; i<n; i++) {
		adrs = GraphDataGet(Plot[PlotCourant].g,i,&j);
		if(adrs == PlotCourbeMod.donnees) {
			GraphDataRemove(Plot[PlotCourant].g,i-1);
			GraphDataRemove(Plot[PlotCourant].g,i-1); /* eh oui, car on vient de tout faire descendre */
		}
	}
	OpiumRefresh((Plot[PlotCourant].g)->cdr);
	return(0);
}
/* ===================================================================== */
Menu mPlotCourbeDef,mPlotCourbeAdd;

TypeMenuItem iPlot[] = {
//	{ "Nouveau",            MNU_FONCTION   PlotBuild },
	{ "Nouveau",            MNU_MENU     &mPlotTypes },
	{ "Retracer",           MNU_FONCTION   PlotRedraw },
	{ "Extractions",        MNU_MENU    &mPlotExtract },
#ifdef LIMITES_AXES
	{ "Limites axes",       MNU_FONCTION   PlotLimite },
#endif
	{ "Fitter",             MNU_MENU     &mPlotFits },
	{ "Supplements",        MNU_MENU     &mPlotCourbes },
	{ "Infos",              MNU_MENU     &mPlotInfos },
	{ "Rafraichir",         MNU_FONCTION   PlotRefresh },
	{ "Sauver",             MNU_FONCTION   PlotSauve },
	{ "Relit",              MNU_FONCTION   PlotRestore },
	{ "Effacer",            MNU_FONCTION   PlotErase },
	{ "Quitter",            MNU_RETOUR },
	MNU_END
};
TypeMenuItem iPlotExtract[] = {
	{ "Valeur d'un bin",    MNU_FONCTION   PlotExtraitBin },
	{ "Profil X",           MNU_FONCTION   PlotExtraitProfilX },
	{ "Profil Y",           MNU_FONCTION   PlotExtraitProfilY },
	//+ { "projection X",       MNU_FONCTION   PlotExtraitProjectX },
	//+ { "projection Y",       MNU_FONCTION   PlotExtraitProjectY },
	{ "Quitter",            MNU_RETOUR },
	MNU_END
};
TypeMenuItem iPlotFits[] = {
	{ "Interface",          MNU_PANEL    &pPlotFitMethode },
	{ "Lineaire",           MNU_FONCTION   PlotLineaire },
	{ "Parabolique",        MNU_FONCTION   PlotParabolique },
	{ "Gaussien",           MNU_FONCTION   PlotGaussien },
	{ "Quitter",            MNU_RETOUR },
	MNU_END
};
TypeMenuItem iPlotCourbes[] = {
	{ "Definitions",        MNU_MENU     &mPlotCourbeDef },
	{ "Affichage",          MNU_MENU     &mPlotCourbeAdd },
	{ "Quitter",            MNU_RETOUR },
	MNU_END
};
TypeMenuItem iPlotCourbeDef[] = {
	{ "Creer",              MNU_FONCTION   PlotCourbeCree },
	{ "Modifier",           MNU_FONCTION   PlotCourbeModifie },
	{ "Supprimer",          MNU_FONCTION   PlotCourbeSupprime },
	{ "Lister",             MNU_FONCTION   PlotCourbeAffiche },
	{ "Quitter",            MNU_RETOUR },
	MNU_END
};
TypeMenuItem iPlotCourbeAdd[] = {
	{ "Ajouter",            MNU_FONCTION   PlotCourbeAjoute },
	{ "Effacer",            MNU_FONCTION   PlotCourbeEfface },
	{ "Quitter",            MNU_RETOUR },
	MNU_END
};
TypeMenuItem iPlotInfos[] = {
	{ "Statistiques",         MNU_FONCTION   PlotStatsDemande },
 	{ "Imprime fit gaussien", MNU_FONCTION   PlotFitGaussImprime },
	{ "Pointer un ntuple",    MNU_FONCTION   PlotNtuplePointeUneFois },
	{ "Quitter",              MNU_RETOUR },
	MNU_END
};
TypeMenuItem iPlotCoupures[] = {
	{ "Couper variable",    MNU_FONCTION   PlotNtupleCoupeAdd },
	{ "Couper zone",        MNU_FONCTION   PlotNtupleCoupeZone },
	//?	{ "Marquer",            MNU_FONCTION   Marquage },
	{ "Retirer coupure",    MNU_FONCTION   PlotNtupleCoupeRemove },
	{ "Supprimer toutes",   MNU_FONCTION   PlotNtupleCoupeRAZ },
	{ "Sauvegarde",         MNU_SEPARATION },
	{ "Sauver",             MNU_FONCTION   PlotNtupleCoupeSauve },
	{ "Refaire",            MNU_FONCTION   PlotNtupleCoupeRelit },
	{ "Quitter",            MNU_RETOUR },
	MNU_END
};


/* ===================================================================== */
int PlotInit(void) {
	int i;

	PlotModeBatch = 0; PlotModeOpium = 1; PlotBatchNiveau = 0;
	PlotVar = 0; PlotVarList = 0; PlotVarNb = 0; PlotEspaceNb = 0;
	PlotFctnDesc = PlotHistoDesc = Plot2DDesc = PlotBiDesc = 0;
	PlotFitMode = FIT_NEANT;
	PlotFitMethode = PLOT_FIT_GRAPHIC;
	PlotFigR[WND_Q_ECRAN] = 0x0000; PlotFigG[WND_Q_ECRAN] = 0xFFFF; PlotFigB[WND_Q_ECRAN] = 0x0000;
	PlotFitR[WND_Q_ECRAN] = 0xFFFF; PlotFitG[WND_Q_ECRAN] = 0xFFFF; PlotFitB[WND_Q_ECRAN] = 0x0000;
	PlotFigR[WND_Q_PAPIER] = 0x0000; PlotFigG[WND_Q_PAPIER] = 0x0000; PlotFigB[WND_Q_PAPIER] = 0xBFFF;
	PlotFitR[WND_Q_PAPIER] = 0xBFFF; PlotFitG[WND_Q_PAPIER] = 0x0000; PlotFitB[WND_Q_PAPIER] = 0x7FFF;

	NumVar = NumAb6 = NumOrd = 0;
	PlotsNb = 0; PlotCourant = 0;
	VarMini = Ab6Mini = OrdMini = 0.0;
	VarMaxi = Ab6Maxi = OrdMaxi = 500.0;
	VarBins = Ab6Bins = OrdBins = 100;
	VarMarque = GRF_TOUTE;

	PlotEspaceSelecte = 0;
	PlotTempsTotal = 0;
	PlotEfficacite = 100.0;
	PlotTaux = 0.0;
	PlotCoupOper = 0;
	PlotEvtMarque = GRF_ABSENT;
	PlotCoupAutre = 0;
	PlotSelectNb = 0;

	for(i=0; i<MAXPLOTS; i++) Plot[i].vide = 1;
	for(i=0; i<MAXPLOTS; i++) PlotName[i] = "\0" /* Plot[i].titre */; PlotName[i] = "\0";
	FitMini = VarMini; FitMaxi = VarMaxi;
	for(i=0; i<MAXVARFITTEES; i++) ResultatFit.var[i] = 0.0;
	strcpy(PlotRepert,"Graphiques");
	PlotType = PLOT_FONCTION;
	PlotTypeTraduit = LL_(PlotTypeTexte);
	ArgKeyFromList(PlotTypeCles,PlotTypeTraduit);
	PlotNtpSepare = PLOT_SEP_BLANC;

	ArgKeyFromList(PlotMarqueCles,PlotMarqueTexte);

	PlotCourbeNb = 0;
	// printf("(PlotInit) Affectation du type %d\n",PLOT_POLYNOME);
	PlotCourbeMod.type = PLOT_POLYNOME;
	// printf("(PlotInit) Affectation d'un nom\n");
	strcpy(PlotCourbeMod.nom,"supplement");
	PlotCourbeMod.pts = 2;
	PlotCourbeMod.min = 0.0; PlotCourbeMod.max = 1.0;
	PlotCourbeMod.x0 = 0.0;
	PlotCourbeMod.nb = 1;
	// printf("(PlotInit) PlotCourbeMod.coef[0..%d] = 0.0\n",PLOT_MAXCOEFS-1);
	for(i=0; i<PLOT_MAXCOEFS; i++) PlotCourbeMod.coef[i] = 0.0;
	// printf("(PlotInit) PlotCourbeNoms[0..%d] = PlotCourbe[0..%d].nom\n",PLOT_MAXCOURBES-1,PLOT_MAXCOURBES-1);
	for(i=0; i<PLOT_MAXCOURBES; i++) PlotCourbeNoms[i] = PlotCourbe[i].nom;
	// printf("(PlotInit) Tableaux initialises\n");
	PlotCourbeNoms[i] = "\0";
	PlotCourbe[PlotCourbeNb].nom[0] = '\0';
	PlotCourbeNum = 0;
	// printf("(PlotInit) Tableaux termines\n");

	PlotStat.somme = 0.0;
	PlotStat.xmax = 0.0;
	PlotStat.ymax = 0.0;
	PlotStat.median = 0.0;
	PlotStat.moyenne = 0.0;
	PlotStat.sigma = 0.0;
	PlotStat.bin = 0;

	return(1);
}
/* ========================================================================== */
void PlotInterface(char batch, char opium) {
	PlotModeBatch = batch; PlotModeOpium = opium;
}
/* ========================================================================== */
void PlotRang(int num) { PlotBatchNiveau = num; }
/* ========================================================================== */
void PlotPrint(char niveau, char *fmt, ...) {
	char texte[256]; va_list argptr; int i;

	if(PlotModeBatch) for(i=0; i<PlotBatchNiveau+1; i++) printf("| ");
	if(fmt) {
		va_start(argptr, fmt);
		vsprintf(texte, fmt, argptr);
		va_end(argptr);
		if(PlotModeOpium) OpiumMessage(niveau,texte);
		else switch(niveau) {
			case OPIUM_NOTE:  printf("* %s\n",texte); break;
			case OPIUM_WARN:  printf("> %s\n",texte); break;
			case OPIUM_ERROR: printf("! %s\n",texte); break;
			default:          printf("%s\n",texte);
		}
	}
}
/* ===================================================================== */
void PlotBuildUI(void) {
	int i;

	if(!PlotModeOpium) return;

	PlotLut = GraphLUTCreate(OPIUM_LUT_DIM,GRF_LUT_ALL);

	// printf("(PlotInit) Creation des menus\n");
	mPlot = MenuCreate(iPlot);
	mPlotFits = MenuCreate(iPlotFits);
	mPlotExtract = MenuCreate(iPlotExtract);
	mPlotCourbes = MenuCreate(iPlotCourbes);
	mPlotCourbeDef = MenuCreate(iPlotCourbeDef);
	mPlotCourbeAdd = MenuCreate(iPlotCourbeAdd);
	mPlotInfos = MenuCreate(iPlotInfos);
	mPlotCoupures = MenuCreate(iPlotCoupures);

	mPlotTypes = MenuTitled("Nouveau trace",0);
	if(MenuItemArray(mPlotTypes,PLOT_NBTYPES+1)) {
		MenuItemAdd(mPlotTypes,PlotTypeTexte[PLOT_FONCTION],MNU_FONCTION PlotBuildFonction);
		MenuItemAdd(mPlotTypes,PlotTypeTexte[PLOT_HISTO],   MNU_FONCTION PlotBuildHisto);
		MenuItemAdd(mPlotTypes,PlotTypeTexte[PLOT_H2D],     MNU_FONCTION PlotBuild2D);
		MenuItemAdd(mPlotTypes,PlotTypeTexte[PLOT_PROFILX], MNU_FONCTION PlotExtraitProfilX);
		MenuItemAdd(mPlotTypes,PlotTypeTexte[PLOT_PROFILY], MNU_FONCTION PlotExtraitProfilY);
		MenuItemAdd(mPlotTypes,PlotTypeTexte[PLOT_BIPLOT],  MNU_FONCTION PlotBuildBiplot);
		MenuItemAdd(mPlotTypes,"En fait,non",MNU_RETOUR);
	}
	MenuRelocalise(mPlotTypes);

	pPlotFitMethode = PanelCreate(1);
	PanelKeyB(pPlotFitMethode,"Entree des limites de fit",PLOT_FIT_CLES,&PlotFitMethode,12);

	iPlotFitConseil = InfoCreate(2,50);
	InfoTitle(iPlotFitConseil,"Designation des limites du fit");
	OpiumPosition(iPlotFitConseil->cdr,WndColToPix(5),WndLigToPix(9));

	iPlotZoneConseil = InfoCreate(2,50);
	InfoTitle(iPlotZoneConseil,"Designation des limites de zone de coupure");
	OpiumPosition(iPlotZoneConseil->cdr,WndColToPix(5),WndLigToPix(9));

	pPlotCoupure = PanelCreate(6);

	pPlotComptesZone = PanelCreate(6); PanelColumns(pPlotComptesZone,2); PanelMode(pPlotComptesZone,PNL_BYLINES|PNL_RDONLY);
	i = PanelInt  (pPlotComptesZone,"Evenements",&AireNb);
	PanelItemColors(pPlotComptesZone,i,PlotRougeVert,2);
	PanelItemSouligne(pPlotComptesZone,i,1);
	i = PanelFloat(pPlotComptesZone,"taux",&AireTaux);
	PanelItemRScale(pPlotComptesZone,i,-1.0,1.0);
	PanelItemColors(pPlotComptesZone,i,PlotRougeVert,2);
	PanelItemSouligne(pPlotComptesZone,i,1);
	PanelFloat(pPlotComptesZone,AireTxtX,&AireXmin); PanelFloat(pPlotComptesZone,"max",&AireXmax);
	i = PanelFloat(pPlotComptesZone,AireTxtY,&AireYmin);
	// PanelItemSouligne(pPlotComptesZone,i,1);
	i = PanelFloat(pPlotComptesZone,"max",&AireYmax);
	// PanelItemSouligne(pPlotComptesZone,i,1);

	pPlotValidite = PanelCreate(3); PanelMode(pPlotValidite,PNL_DIRECT|PNL_RDONLY);
	PanelInt(pPlotValidite,L_("Evenements restant"),&PlotNtupleValides);
	PanelFloat(pPlotValidite,L_("Efficacite"),&PlotEfficacite);
	PanelFloat(pPlotValidite,L_("Taux (Hz)"),&PlotTaux);

	pFitLimites = PanelCreate(2);
	PanelTitle(pFitLimites,"Limites du fit");
	OpiumPosition(pFitLimites->cdr,WndColToPix(5),WndLigToPix(14));
	PanelFloat(pFitLimites,"Abcisse minimum",&FitMini);
	PanelFloat(pFitLimites,"Abcisse maximum",&FitMaxi);

	pFitLin = PanelCreate(5); PanelTitle(pFitLin,"Fit lineaire");
	PanelColumns(pFitLin,2); PanelMode(pFitLin,PNL_BYLINES);
	PanelFloat(pFitLin,"pente",&ResultatFit.lin.pente); PanelFloat(pFitLin,"+/-",&ResultatFit.lin.err_pente);
	PanelFloat(pFitLin,"base",&ResultatFit.lin.base);   PanelFloat(pFitLin,"+/-",&ResultatFit.lin.err_base);
	PanelFloat(pFitLin,"khi2",&ResultatFit.lin.khi2);

	pFitDegre2 = PanelCreate(4); PanelTitle(pFitDegre2,"Fit 2eme degre");
	PanelFloat(pFitDegre2,"a",&ResultatFit.par.a);
	PanelFloat(pFitDegre2,"b",&ResultatFit.par.b);
	PanelFloat(pFitDegre2,"c",&ResultatFit.par.c);

	pFitParab = PanelCreate(3); PanelTitle(pFitParab,"Fit parabolique");
	PanelFloat(pFitParab,"centre",&ResultatFit.par.x0);
	PanelFloat(pFitParab,"base",&ResultatFit.par.base);
	PanelFloat(pFitParab,"amplitude",&ResultatFit.par.a);

	pFitGauss = PanelCreate(6); PanelTitle(pFitGauss,"Fit Gaussien");
	OpiumPosition(pFitGauss->cdr,WndColToPix(5),WndLigToPix(20));
	PanelFloat(pFitGauss,"Centre",&ResultatFit.gau.x0);
	PanelFloat(pFitGauss,"Amplitude",&ResultatFit.gau.amplitude);
	PanelFloat(pFitGauss,"Sigma",&ResultatFit.gau.sigma);
	PanelFloat(pFitGauss,"Base",&ResultatFit.gau.base);
	i = PanelFloat(pFitGauss,"Resolution Sigma",&PlotResolSigma); PanelFormat(pFitGauss,i,"%.1f");
	PanelItemRScale(pFitGauss,i,0.0,10.0); PanelItemColors(pFitGauss,i,PlotVertJauneOrangeRouge,5);
	i = PanelFloat(pFitGauss,"Resolution FWHM",&PlotResolFWHM); PanelFormat(pFitGauss,i,"%.1f");
	PanelItemRScale(pFitGauss,i,0.0,23.5); PanelItemColors(pFitGauss,i,PlotVertJauneOrangeRouge,5);

	pPlotLimites = PanelCreate(8);
	PanelSepare(pPlotLimites,"Axe X");
	PanelBool  (pPlotLimites,"Automatique",&(Xlimite.automatique));
	PanelFloat (pPlotLimites,"Sinon, minimum",&(Xlimite.min));
	PanelFloat (pPlotLimites,"et maximum",&(Xlimite.max));
	PanelSepare(pPlotLimites,"Axe Y");
	PanelBool  (pPlotLimites,"Automatique",&(Ylimite.automatique));
	PanelFloat (pPlotLimites,"Sinon, minimum",&(Ylimite.min));
	PanelFloat (pPlotLimites,"et maximum",&(Ylimite.max));

	pPlotLimit3D = PanelCreate(14);
	PanelSepare(pPlotLimit3D,"Axe X");
	PanelBool  (pPlotLimit3D,"Automatique",&(Xlimite.automatique));
	PanelFloat (pPlotLimit3D,"Sinon, minimum",&(Xlimite.min));
	PanelFloat (pPlotLimit3D,"et maximum",&(Xlimite.max));
	PanelSepare(pPlotLimit3D,"Axe Y");
	PanelBool  (pPlotLimit3D,"Automatique",&(Ylimite.automatique));
	PanelFloat (pPlotLimit3D,"Sinon, minimum",&(Ylimite.min));
	PanelFloat (pPlotLimit3D,"et maximum",&(Ylimite.max));
	PanelSepare(pPlotLimit3D,"Axe Z");
	PanelBool  (pPlotLimit3D,"Automatique",&(Zlimite.automatique));
	PanelFloat (pPlotLimit3D,"Sinon, minimum",&(Zlimite.min));
	PanelFloat (pPlotLimit3D,"et maximum",&(Zlimite.max));
	PanelSepare(pPlotLimit3D,0);
	PanelListB (pPlotLimit3D,"Marquage",PlotMarqueTexte,(char *)&VarMarque,8);

	pPlotStats = PanelCreate(10); PanelTitle(pPlotStats,"Statistiques");
	OpiumPosition(pPlotStats->cdr,WndColToPix(5),WndLigToPix(28));
	PanelItemSelect(pPlotStats,PanelFloat(pPlotStats,"Abcisse minimum",&FitMini),0);
	PanelItemSelect(pPlotStats,PanelFloat(pPlotStats,"Abcisse maximum",&FitMaxi),0);
	PanelFloat(pPlotStats,"Somme",&(PlotStat.somme));
	PanelFloat(pPlotStats,"Position maxi",&(PlotStat.xmax));
	PanelFloat(pPlotStats,"Valeur maxi",&(PlotStat.ymax));
	PanelFloat(pPlotStats,"Mediane",&(PlotStat.median));
	PanelFloat(pPlotStats,"Moyenne",&(PlotStat.moyenne));
	PanelFloat(pPlotStats,"Sigma",&(PlotStat.sigma));
	PanelFloat(pPlotStats,"Largeur bin",&(PlotStat.bin));
	i = PanelFloat(pPlotStats,"Resolution FWHM",&PlotResolStat);
	PanelFormat(pPlotStats,i,"%.1f");
	PanelItemRScale(pPlotStats,i,0.0,15.0); PanelItemColors(pPlotStats,i,PlotVertJauneOrangeRouge,5);

	pPlotCourbeDef = PanelCreate(9);
	PanelText (pPlotCourbeDef,"Denomination",PlotCourbeMod.nom,PLOT_MAXCOURBENOM);
	PlotCourbeTraduit = LL_(PlotCourbeTypes);
	PanelListB(pPlotCourbeDef,"Type de courbe",PlotCourbeTraduit,&PlotCourbeMod.type,16);
	PanelSHex (pPlotCourbeDef,"Couleur rouge",(short *)&PlotCourbeMod.couleur.r);
	PanelSHex (pPlotCourbeDef,"Couleur verte",(short *)&PlotCourbeMod.couleur.g);
	PanelSHex (pPlotCourbeDef,"Couleur bleue",(short *)&PlotCourbeMod.couleur.b);
	PanelFloat(pPlotCourbeDef,"Abscisse mini",&PlotCourbeMod.min);
	PanelFloat(pPlotCourbeDef,"Abscisse maxi",&PlotCourbeMod.max);
	PanelInt  (pPlotCourbeDef,"Nombre de points",&PlotCourbeMod.pts);
	PanelInt  (pPlotCourbeDef,"Nombre de coefficients",&PlotCourbeMod.nb);
	pPlotCourbeCoef = PanelCreate(PLOT_MAXCOEFS+1);
	pPlotCourbeGraph = PanelCreate(2);
	PanelList(pPlotCourbeGraph,"Courbe",PlotCourbeNoms,&PlotCourbeNum,PLOT_MAXCOURBENOM);
	PanelList(pPlotCourbeGraph,"Trace",PlotName,&PlotCourant,16);

	pCourbe = PanelCreate(1);
	pHistoNeuf = PanelCreate(5);
	pHistoCree = PanelCreate(5);
	pH2DNeuf = PanelCreate(12);
	pH2DCree = PanelCreate(10);
	pAxes = PanelCreate(3);

	// printf("(PlotInit) Creation d'un terminal\n");
	tPlot = TermCreate(12,80,4096);
	OpiumTitle(tPlot->cdr,"Liste des courbes utilisables");

	tSelect = TermCreate(10,80,1024);
	TermTitle(tSelect,"Coupure courante");
	tListe = TermCreate(24,100,4096);
	TermTitle(tListe,"Runs pris en compte");

	tCoupures = 0;

}
/* ===================================================================== */
void PlotColors(int qual, char *fig, char *fit) {
	char val[6];

	strncpy(val,fig  ,4); val[4] = '\0'; sscanf(val,"%hx",&(PlotFigR[qual]));
	strncpy(val,fig+4,4); val[4] = '\0'; sscanf(val,"%hx",&(PlotFigG[qual]));
	strncpy(val,fig+8,4); val[4] = '\0'; sscanf(val,"%hx",&(PlotFigB[qual]));
	strncpy(val,fit  ,4); val[4] = '\0'; sscanf(val,"%hx",&(PlotFitR[qual]));
	strncpy(val,fit+4,4); val[4] = '\0'; sscanf(val,"%hx",&(PlotFitG[qual]));
	strncpy(val,fit+8,4); val[4] = '\0'; sscanf(val,"%hx",&(PlotFitB[qual]));
	// printf("(%s) fig %s: %04X.%04X.%04X\n",__func__,WndSupportNom[qual],PLOT_COLOR_FIG(qual));
	// printf("(%s) fit %s: %04X.%04X.%04X\n",__func__,WndSupportNom[qual],PLOT_COLOR_FIT(qual));
}
/* ===================================================================== */
int PlotExit(void) { PlotVarsDeclare(0); if(PlotMarqueTraduit) free(PlotMarqueTraduit); return(1); }

