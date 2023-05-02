#ifdef macintosh
	#pragma message(__FILE__)
#endif

#include <stdio.h>
#include <errno.h>
#include <math.h>

#ifdef solaris
	#define sqrtf(x) (float)sqrt((double)x)
	#define expf(x) (float)exp((double)x)
	#define logf(x) (float)log((double)x)
#endif

#include <environnement.h>

#include <defineVAR.h>
#include <decode.h>
#include <dateheure.h>
#include <fitlib.h>
#include <opium.h>
#include <nanopaw.h>

#include <tango.h>
#include <calib.h>
#include <filtres.h>

#include <rundata.h>
#include <detecteurs.h>
#include <archive.h>
#include <evtread.h>
#include <edwdb.h>
#undef VAR
#define VAR extern
#include "analyse.h"

#define COMPARE_NTUPLES

static char ProdVarsModifiees;
#ifdef BOLOMETRES
	static int  VoieChaleur,VoieCentre,VoieGarde;
#endif

       float *Base[VOIES_MAX];
static float *Bruit[VOIES_MAX];
static float *Amplitude[VOIES_MAX];
static float *Depot[VOIES_MAX];
// static float *AmplAdu[VOIES_MAX];
static float *Integrale[VOIES_MAX];
static float *Longueur[VOIES_MAX];
static float *Montee[VOIES_MAX];
static float *Descente[VOIES_MAX];
static float *RCmesure[VOIES_MAX];
static float *Duree[VOIES_MAX];
static float *Pente[VOIES_MAX];
static float *Retard[VOIES_MAX];
       float *Energie[VOIES_MAX];
static float *EvtDateMesuree;
static float *EvtDelaiMesure;
static float *EvtDecalage;
static float *Khi2[VOIES_MAX];
static float *Categ[VOIES_MAX];

#ifdef COMPARE_NTUPLES
	static float *DelaiSamba;
	static float *BaseSamba[VOIES_MAX];
	static float *BruitSamba[VOIES_MAX];
	static float *AmplitudeSamba[VOIES_MAX];
	static float *MonteeSamba[VOIES_MAX];
#endif
#ifdef ETUDIE_CALAGE
	static float *DTmax[VOIES_MAX];
	static float *DTtheo[VOIES_MAX];
#endif
#ifdef CALCULE_SURFACE
	static int   FinEvt[VOIES_MAX];
	static float *Surface[VOIES_MAX];
#endif
// static float *t2[VOIES_MAX];
static float *BoloTouche;
static float *EcartT0;
static float *NumEvt;
#ifdef BOLOMETRES
	static float *Tevt[VOIES_MAX];
	static float *Ionisation[DETEC_MAX];
	static float *VoieIonisee;
	static float *Chaleur;
	static float *Ionisation;
	static float *Recul;
	static float *Quenching;
#endif
#ifdef CONDITIONS_MESURE
	/* A generaliser */
	static float *TemperLue;
#endif
static float *NbTouches;

#define AJOUTSVOIE_MAX 4
#define AJOUTSDETEC_MAX 8
static float *VarVoie[AJOUTSVOIE_MAX][VOIES_MAX];
static float *VarDetec[AJOUTSDETEC_MAX][DETEC_MAX];
static Term tProdVar;

static TypeS_us ProdDatePrecedent;
static  float ProdDateApprox;

static float Coef[EDW_NBMESURES][VOIES_MAX];
static float Etalon[EDW_NBMESURES][VOIES_MAX];
static char  ProdCalibFaite[EDW_NBMESURES][VOIES_MAX];

// static float *ProdTempsAvant,*ProdTempsStart,*ProdTempsStop,*ProdTempsApres;
static int   TempsMontee[DETEC_CAPT_MAX],TempsDescente[DETEC_CAPT_MAX];
static char  TxtMontee[DETEC_CAPT_MAX][40],TxtDescente[DETEC_CAPT_MAX][40];

//- static int t_max,t_max_centre,voiecentre;

#define MAXINTERVCHALEUR 8
#define MAXNONLIN 4
typedef struct {
	float inf,sup;       /* intervalle de validite des coefficients */
	float c[MAXNONLIN];  /* coefficients y = somme(c[i]*x**i)       */
	int nb;              /* nombre de coefficients                  */
} TypeCorrection;

static Panel pHdrGeneral,pTrigger;

static TypeCorrection CoefChal[DETEC_MAX][MAXINTERVCHALEUR];
static int NbIntervChal[DETEC_MAX];
static Panel pCorrigeInterv;
static char CorrigeIntervTxt[MAXNONLIN][12];

#define VOIES_PERIM_CLES "la voie choisie/les voies du detecteur/toutes les voies"
typedef enum {
	PERIM_UNIQUE = 0,
	PERIM_DETEC,
	PERIM_TOUTES,
	PERIM_NB
} PERIMETRE_VOIES;

static char  ProdToutes,ProdEvtManu;
static float ProdAmplMin,ProdNiveauMin,ProdTauxDt[2];
static float ProdCorrectionCT;
static Panel pProdEvtCalib,pProdVoieUtilisee;
static Panel pQuenching,pEnergies;
static Panel pEvtNtupleOptions;
static Panel pProdUniteZoneFit,pProdUniteChoixVoie;
static Panel pProdCalibVoie; static float Actuelle;
static Panel pProdCalibCoefs; static char *ProdCalibCoefTxt[VOIES_MAX];
static Panel pProdCalibSauve,pProdChaleurSauve;
static Panel pAfficheAmplMin,pProdEvtHdrAffichage;

static Panel pEvtDump;
static char  ProdFormatHexa;
static Panel pProdResolutions;
#ifdef CROSSTALK
	static Panel pProdCrosstalk;
#endif
static Graph gTauxEvts;

#define ZONE_NBPOINTS 100
static float ZoneDiagX[2],ZoneDiagY[2];
static float ZoneQ[2],ZoneQegale1[2];
static float ZoneAb6[2];
static float ZoneResol[ZONE_NBPOINTS];
static float ZoneGammasMin[ZONE_NBPOINTS],ZoneGammasMax[ZONE_NBPOINTS];
static float ZoneNeutronsMin[ZONE_NBPOINTS],ZoneNeutronsMax[ZONE_NBPOINTS],ZoneNeutronsMil[ZONE_NBPOINTS];

#ifdef BOLOMETRES
	static Menu mProdChaleurAjuste;
	static float ProdCorrecChaleurX[MAXINTERVCHALEUR+1],ProdCorrecChaleurY[MAXINTERVCHALEUR+1];
	static Panel pProdChaleurFit;
#endif
static float ProdChaleurFitMin,ProdChaleurFitMax;
static int   ProdChaleurFitNb;
static float ProdKhi2ChalMax,ProdKhi2IonisMax;

static Panel pProdTraitements;
// static Panel pProdChoixFiltres;
static Panel pProdChoixTemps;
// static Panel pProdTempsArg;

static Cadre bProdChoixTemps;

enum {
	PROD_STOPPE = 1,
	PROD_CONTINUE,
	PROD_RECULE,
	PROD_ANNULE
};

static char *ProdHistoBoutons[4] = { "Abandon calib", "A retracer", "Oui", "\0" };

#define PROD_DECONV_COLS 4

#define PROD_TEMPS_COLS 6
typedef enum {
	TEMPS_SAMBA = 0,
	TEMPS_REL,
	TEMPS_ABS,
	TEMPS_NBREFS
} TEMPS_REF;
#define TEMPS_REF_CLES "samba/pourcent/millisec"
#define TEMPS_REF_LNGR 12
static TypeVoieTemps VoieTempsLue;
static int VoieTempsNb;
static char ProdTempsStandard;

static ArgDesc ProdTempsDeconvDesc[] = {
	{ "lissage.avant",    DESC_INT   &VoieTempsLue.deconv.pre,     0 },
	{ "lissage.apres",    DESC_INT   &VoieTempsLue.deconv.post,    0 },
	{ "descente_ms",      DESC_FLOAT &VoieTempsLue.deconv.rc,      0 },
	{ "integre.seuil",    DESC_FLOAT &VoieTempsLue.deconv.seuil,   0 },
DESC_END
};
static ArgDesc ProdTempsNulDesc[] = { DESC_END };
static ArgDesc ProdTempsRelDesc[] = {
	{ "base_start",    DESC_FLOAT    &VoieTempsLue.pcent.start, 0 }, // Debut relatif du calcul de la ligne de base
	{ "base_stop",     DESC_FLOAT    &VoieTempsLue.pcent.stop,  0 }, // Fin relative du calcul de la ligne de base
	{ "fit_descente",  DESC_FLOAT    &VoieTempsLue.pcent.apres, 0 }, // Temps a utiliser apres le maximum, pour l'evt unite
	{ "RC",            DESC_NONE },
	DESC_END
};
static ArgDesc ProdTempsAbsDesc[] = {
	{ "base_start",    DESC_FLOAT    &VoieTempsLue.absol.start, 0 }, // Debut en ms du calcul de la ligne de base
	{ "base_stop",     DESC_FLOAT    &VoieTempsLue.absol.stop,  0 }, // Fin en ms du calcul de la ligne de base
	{ "fit_descente",  DESC_FLOAT    &VoieTempsLue.absol.apres, 0 }, // Temps a utiliser apres le maximum, pour l'evt unite
	{ "RC",            DESC_NONE },
	DESC_END
};
static ArgDesc *ProdTempsRefDesc[TEMPS_NBREFS] = { ProdTempsNulDesc, ProdTempsRelDesc, ProdTempsAbsDesc };
static ArgDesc ProdTempsVoieDesc[] = {
	{ 0,                 DESC_LISTE VoieNom,      &VoieTempsLue.num,       0},
	{ "reference",       DESC_DEVIENT("reference.temps") },
	{ "reference.temps", DESC_KEY TEMPS_REF_CLES, &VoieTempsLue.ref,       0},
	{ 0,                 DESC_VARIANTE(VoieTempsLue.ref) ProdTempsRefDesc, 0},
	{ "deconvolution",   DESC_STRUCT               ProdTempsDeconvDesc,    0 },
	DESC_END
};
static ArgStruct ProdTempsDescAS = { (void *)VoieTemps, (void *)&VoieTempsLue, sizeof(TypeVoieTemps), ProdTempsVoieDesc };

static ArgDesc ProdTempsDesc[] = {
	{ "montee_10",     DESC_FLOAT &ProdMontee10,     "Pourcentage de l'amplitude pour le debut de la montee"},
	{ "montee_90",     DESC_FLOAT &ProdMontee90,     "Pourcentage de l'amplitude pour la fin de la montee"},
	{ "montee_50",     DESC_FLOAT &ProdQuotaMontee,  "Quota de montee a retirer de la duree"},
	{ "pre-lissage",   DESC_INT   &ProdPreLissage,   "Largeur de lissage avant deconvolution, par defaut"},
	{ "post-lissage",  DESC_INT   &ProdPostLissage,  "Largeur de lissage apres deconvolution, par defaut"},
	{ "deconv-RC",     DESC_FLOAT &ProdDeconvRC,     "Temps de descente de l'ampli, par defaut"},
	{ "multiple_si",   DESC_INT   &ProdPaquetAmpl,   "Amplitude minimum pour compter un evenement multiple"},
	{ "pendant_maxi",  DESC_FLOAT &ProdPaquetDuree,  "Duree maximum d'un evenement multiple (s)"},
	{ "voies",         DESC_STRUCT_SIZE(VoieTempsNb,VOIES_MAX) &ProdTempsDescAS, 0},
	DESC_END
};

void ProdVarDump(char *fctn, int n);
int ellfcalc(char modele, char type, short degre, float dbr, int maxi,
			 float omega1, float omega2, float omega3, int *ncoef, double *dir, double *inv);

#ifdef CALCULE_SURFACE
/* ========================================================================== */
void ProdChannelUniteOld(char *nom, void **adrs, int *dim, TANGO_TYPE *type,
	float **base, int *dep, int *arr,
	float **energie, float **khi2) {

	sprintf(nom,"%s",VoieManip[VoieNum].nom);
	*adrs = (void *)VoieEvent[VoieNum].brutes.t.sval;
	*dim = VoieEvent[VoieNum].lngr;
	*type = TANGO_SHORT;
	*base = Base[VoieNum];
	*dep = VoieEvent[VoieNum].avant_evt / 2;
//	*arr = t2[VoieNum][evt];
	*arr = FinEvt[VoieNum];
	*energie = Energie[VoieNum];
	*khi2 = Khi2[VoieNum];
}
#endif
/* ========================================================================== */
void ProdChannelUnite(char *nom, void **adrs, int *dim, TANGO_TYPE *type,
	float **base, int *dep, int *arr,
	float **energie, float **khi2, float **bruit) {

	strcpy(nom,VoieManip[VoieNum].nom);
	*adrs = (void *)VoieEvent[VoieNum].brutes.t.sval;	
	*type = TANGO_SHORT;
	*base = Base[VoieNum];
//	*arr = t2[VoieNum][evt];	
	*energie = Energie[VoieNum];
	*khi2 = Khi2[VoieNum];
	*dim = VoieEvent[VoieNum].lngr;
	*bruit = Bruit[VoieNum];
	if(VoiesNb == 1) {
		TempsMontee[VoieManip[VoieNum].type] = (int)(EvtUniteMont / VoieEvent[VoieNum].horloge);
		TempsDescente[VoieManip[VoieNum].type] = (int)(EvtUniteDesc / VoieEvent[VoieNum].horloge);
	}
	*dep = VoieEvent[VoieNum].avant_evt - TempsMontee[VoieManip[VoieNum].type];
	*arr = VoieEvent[VoieNum].avant_evt + TempsDescente[VoieManip[VoieNum].type];	
}
/* ========================================================================== */
static void ProdTempsRelToAbs(int msr) {
	VoieTemps[msr].absol.start = VoieTemps[msr].avant * VoieTemps[msr].pcent.start / 100.0;
	VoieTemps[msr].absol.stop  = VoieTemps[msr].avant * VoieTemps[msr].pcent.stop  / 100.0;
	VoieTemps[msr].absol.apres = VoieTemps[msr].post  * VoieTemps[msr].pcent.apres / 100.0;
}
/* ========================================================================== */
static void ProdTempsAbsToRel(int msr) {
	if(VoieTemps[msr].avant > 0.0) {
		VoieTemps[msr].pcent.start = VoieTemps[msr].absol.start * 100.0 / VoieTemps[msr].avant;
		VoieTemps[msr].pcent.stop  = VoieTemps[msr].absol.stop  * 100.0 / VoieTemps[msr].avant;
	}
	if(VoieTemps[msr].post > 0.0)
		VoieTemps[msr].pcent.apres = VoieTemps[msr].absol.apres * 100.0 / VoieTemps[msr].post;
}
/* ========================================================================== */
static void ProdTempsBase(int voie, int msr) {
	if(voie < 0) voie = 0; if(msr < 0) msr = 0;
	VoieTemps[voie].mesure = msr;
	VoieTemps[msr].avant = VoieManip[voie].def.evt.duree * VoieManip[voie].def.evt.pretrigger / 100.0;
	VoieTemps[msr].post  = VoieManip[voie].def.evt.duree - VoieTemps[msr].avant;
	VoieTemps[msr].prec  = VoieTemps[msr].ref;
}
/* ========================================================================== */
static void ProdTempsSamba(int voie, int msr) {
	if(voie < 0) voie = 0;

	VoieTemps[msr].ref   = TEMPS_SAMBA;
	VoieTemps[msr].pcent.start  = VoieManip[voie].def.evt.base_dep;
	VoieTemps[msr].pcent.stop   = VoieManip[voie].def.evt.base_arr;
	VoieTemps[msr].pcent.apres  = 100.0;
	ProdTempsRelToAbs(msr);
	printf("  | Par defaut pour %s: base=[%g - %g[, pretrigger=%g, posttrigger=%g\n",
		   VoieManip[voie].nom,VoieTemps[msr].absol.start,VoieTemps[msr].absol.stop,
		   VoieTemps[msr].avant,VoieTemps[msr].absol.apres);
}
/* ========================================================================== */
static void ProdTempsDefaut(int voie, int msr) {
	if(voie < 0) voie = 0;

	ProdTempsSamba(voie,msr);
	VoieTemps[msr].deconv.pre   = ProdPreLissage;
	VoieTemps[msr].deconv.post  = ProdPostLissage;
	VoieTemps[msr].deconv.rc    = VoieManip[voie].RC;
	VoieTemps[msr].deconv.seuil = 10.0;
}
/* ========================================================================== */
static void ProdTempsInit(int voie, int msr) {
	/* Cree une mesure a partir des temps SAMBA */
	if(VoieManip[voie].RC < VoieEvent[voie].horloge) VoieManip[voie].RC = ProdDeconvRC;
	VoieTemps[msr].num = voie;
	ProdTempsBase(voie,msr);
	ProdTempsDefaut(voie,msr);
	FiltreVoies[voie] = 0; /* filtrage SAMBA par defaut */
	ProdCalibFaite[EDW_AMPL][voie] = 0; Coef[EDW_AMPL][voie] = 1.0;
#ifdef CALCULE_SURFACE
	// FinEvt[voie] = 0; tous les evenements ne seraient pas de la meme longueur?
	FinEvt[voie] = (float)VoieEvent[voie].lngr;
	if(variante == EDW_SURF) { ProdCalibFaite[EDW_SURF][voie] = 0; Coef[EDW_SURF][voie] = 1.0; }
#endif
}
/* ========================================================================== */
static char ProdTempsVerifie(int voie, int msr) {
	char ok; // char ref[TEMPS_REF_LNGR];

// 	ArgKeyGetText(TEMPS_REF_CLES,VoieTemps[msr].ref,ref,TEMPS_REF_LNGR);
// 	printf("  | Verification des temps pour la voie %s (%s)",VoieManip[voie].nom,ref);
	ok = 1;
	if(VoieTemps[msr].ref != TEMPS_SAMBA) {
		if(VoieTemps[msr].ref == TEMPS_REL) ProdTempsRelToAbs(msr);
		if((VoieTemps[msr].absol.start < VoieEvent[voie].horloge) || (VoieTemps[msr].absol.start >= VoieTemps[msr].avant)) {
			ok = 0;
			VoieTemps[msr].pcent.start = VoieManip[voie].def.evt.base_dep;
			VoieTemps[msr].absol.start = VoieTemps[msr].avant * VoieTemps[msr].pcent.start / 100.0;
		}
		if((VoieTemps[msr].absol.stop <= VoieTemps[msr].absol.start) || (VoieTemps[msr].absol.stop >= VoieTemps[msr].avant)) {
			ok = 0;
			VoieTemps[msr].pcent.stop = VoieManip[voie].def.evt.base_arr;
			VoieTemps[msr].absol.stop  = VoieTemps[msr].avant * VoieTemps[msr].pcent.stop  / 100.0;
		}
		if((VoieTemps[msr].absol.apres < VoieEvent[voie].horloge) || (VoieTemps[msr].absol.apres >= VoieTemps[msr].post)) {
			ok = 0;
			VoieTemps[msr].pcent.apres = 100.0;
			VoieTemps[msr].absol.apres = VoieTemps[msr].post;
		}
		if(VoieTemps[msr].deconv.rc <= 0.0) VoieTemps[msr].deconv.rc = VoieManip[voie].RC;
		if(VoieTemps[msr].deconv.rc < VoieEvent[voie].horloge) {
			ok = 0;
			VoieTemps[msr].deconv.rc = ProdDeconvRC;
		}
	}
// 	if(ok) printf(": corrects\n");
// 	else printf(" corriges ainsi:\n  | > debut base=%g, fin base=%g, descente=%g\n",VoieTemps[msr].start,VoieTemps[msr].stop,VoieTemps[msr].apres);

	return(ok);
}
/* ========================================================================== */
static void ProdTempsComplete() {
	int voie,msr;

	printf("* Verification des temps definis dans %s\n",ProdTempsFile);
	for(voie=0; voie<VoiesNb; voie++) if((msr = VoieTemps[voie].mesure) >= 0) ProdTempsVerifie(voie,msr);
	else { ProdTempsInit(voie,VoieTempsNb); VoieTempsNb++; }
	printf("  | %d definition%s des temps en service\n",Accord1s(VoieTempsNb));
}
/* .......................................................................... */
// static int ProdTempsReference(Panel p, int numitem, void *arg);
static int ProdTempsModifies(Panel p, void *arg);
/* ========================================================================== */
static void ProdDeconvPanelFill(Panel p) {
	int voie,msr; int k;

	PanelErase(p);
	PanelColumns(p,PROD_DECONV_COLS); PanelMode(p,PNL_BYLINES);
	PanelDansPlanche(p);
	PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"prelissage",12),0),1);
	PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"postlissage",12),0),1);
	PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"RC (ms)",8),0),1);
	PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"seuil",8),0),1);
	k = 0;
	for(voie=0; voie<VoiesNb; voie++) {
		char fin_de_serie; int i;
		fin_de_serie = !((k++ + 1) % 3);
		msr = VoieTemps[voie].mesure; if(msr < 0) continue;
		i = PanelInt(p,VoieManip[voie].nom,&(VoieTemps[msr].deconv.pre));
		PanelItemSouligne(p,i,fin_de_serie);
		i = PanelInt(p,0,&(VoieTemps[msr].deconv.post)); PanelLngr(p,i,12);
		PanelItemSouligne(p,i,fin_de_serie);
		i = PanelFloat(p,0,&(VoieTemps[msr].deconv.rc)); PanelLngr(p,i,8);
		PanelItemSouligne(p,i,fin_de_serie);
		i = PanelFloat(p,0,&(VoieTemps[msr].deconv.seuil)); PanelLngr(p,i,8);
		PanelItemSouligne(p,i,fin_de_serie);
	}
}
/* ========================================================================== */
static void ProdTempsPanelFill(Panel p) {
	int voie,msr; int k; char actif; char ref[TEMPS_REF_LNGR];

	// printf("(%s) Panel @%08llX\n",__func__,(hex64)p);
	PanelErase(p);
	PanelColumns(p,PROD_TEMPS_COLS); PanelMode(p,PNL_BYLINES);
	PanelDansPlanche(p);
	PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"reference",10),0),1);
	PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"evt",7),0),1);
	PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"pretrg",7),0),1);
	PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"debut base",11),0),1);
	PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"fin base",9),0),1);
	PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"descente",9),0),1);
	k = 0;
	for(voie=0; voie<VoiesNb; voie++) {
		char fin_de_serie; int i;
		fin_de_serie = !((k++ + 1) % 3);
		msr = VoieTemps[voie].mesure; if(msr < 0) continue;
		ArgKeyGetText(TEMPS_REF_CLES,VoieTemps[msr].ref,ref,TEMPS_REF_LNGR);
		// printf("(%s) | Voie #%d (%s): mesure #%d de type %s\n",__func__,voie,VoieManip[voie].nom,msr,ref);
		i = PanelKeyB(p,VoieManip[voie].nom,TEMPS_REF_CLES,&(VoieTemps[msr].ref),9);
		PanelItemSouligne(p,i,fin_de_serie);
		// PanelItemExit(p,i,ProdTempsReference,&msr);
		i = PanelFloat(p,0,&(VoieManip[voie].def.evt.duree)); PanelFormat(p,i,"%.2f");
		PanelItemSouligne(p,i,fin_de_serie); PanelItemSelect(p,i,0); PanelLngr(p,i,7);
		i = PanelFloat(p,0,&(VoieManip[voie].def.evt.pretrigger)); PanelFormat(p,i,"%.2f");
		PanelItemSouligne(p,i,fin_de_serie); PanelItemSelect(p,i,0); PanelLngr(p,i,7);
		if(VoieTemps[msr].ref == TEMPS_REL) {
			i = PanelFloat(p,0,&(VoieTemps[msr].pcent.start)); PanelLngr(p,i,11);
			PanelItemSouligne(p,i,fin_de_serie);
			i = PanelFloat(p,0,&(VoieTemps[msr].pcent.stop)); PanelLngr(p,i,9);
			PanelItemSouligne(p,i,fin_de_serie);
			i = PanelFloat(p,0,&(VoieTemps[msr].pcent.apres)); PanelLngr(p,i,9);
			PanelItemSouligne(p,i,fin_de_serie);
		} else {
			actif = (VoieTemps[msr].ref != TEMPS_SAMBA);
			i = PanelFloat(p,0,&(VoieTemps[msr].absol.start));
			PanelItemSouligne(p,i,fin_de_serie); PanelItemSelect(p,i,actif); PanelLngr(p,i,11);
			i = PanelFloat(p,0,&(VoieTemps[msr].absol.stop));
			PanelItemSouligne(p,i,fin_de_serie); PanelItemSelect(p,i,actif); PanelLngr(p,i,9);
			i = PanelFloat(p,0,&(VoieTemps[msr].absol.apres));
			PanelItemSouligne(p,i,fin_de_serie); PanelItemSelect(p,i,actif); PanelLngr(p,i,9);
		}
		VoieTemps[msr].prec = VoieTemps[msr].ref;
	}
	PanelOnApply(p,ProdTempsModifies,0);
	PanelOnOK(p,ProdTempsModifies,0);
}
static void ProdTempsReset(Menu menu, MenuItem item);
static int ProdTempsSauve();
/* ========================================================================== */
void ProdTempsBoardFill() {
	Panel p; int x,y,xm,ym;

	BoardAreaOpen("Montee",WND_RAINURES);
	p = PanelCreate(3); PanelSupport(p,WND_CREUX);
	PanelFloat(p,"Debut",&ProdMontee10);
	PanelFloat(p,"Fin",&ProdMontee90);
	PanelFloat(p,"Quota",&ProdQuotaMontee);
	BoardAddPanel(bProdChoixTemps,p,2 * Dx,2 * Dy,0);
//-	BoardAreaMargesVert(0,Dy);
	BoardAreaEnd(bProdChoixTemps,&xm,&ym);

	BoardAreaOpen("Multiplicite",WND_RAINURES);
	p = PanelCreate(2); PanelSupport(p,WND_CREUX);
	PanelInt(p,"si Amplitude >",&ProdPaquetAmpl);
	PanelFloat(p,"pour duree maxi (s)",&ProdPaquetDuree); PanelBlank(p);
	BoardAddPanel(bProdChoixTemps,p,xm + (2 * Dx),2 * Dy,0);
//-	BoardAreaMargesVert(0,Dy);
	BoardAreaEnd(bProdChoixTemps,&xm,&y);

	BoardAreaOpen("Deconvolution par defaut",WND_RAINURES);
	p = PanelCreate(3); PanelSupport(p,WND_CREUX);
	PanelInt(p,"Pre-lissage", &ProdPreLissage);
	PanelInt(p,"Post-lissage",&ProdPostLissage);
	PanelFloat(p,"RC (ms)",&ProdDeconvRC);
	BoardAddPanel(bProdChoixTemps,p,xm + (2 * Dx),2 * Dy,0);
//-	BoardAreaMargesVert(0,Dy);
	BoardAreaEnd(bProdChoixTemps,&xm,&y);

	BoardAreaOpen("Evaluation ligne de base (ms)",WND_RAINURES);
	p = PanelCreate(PROD_TEMPS_COLS * (1 + VoiesNb)); // printf("(%s) Panel @%08llX\n",__func__,(hex64)p);
	ProdTempsPanelFill(p);
	BoardAddPanel(bProdChoixTemps,p,2 * Dx,ym + (3 * Dy),0);
	BoardAddBouton(bProdChoixTemps,"Valeurs standard",ProdTempsReset,32 * Dx,ym + ((3 + (1 + VoiesNb) + 2) * Dy),0);
//-	BoardAreaMargesVert(0,Dy);
	BoardAreaEnd(bProdChoixTemps,&xm,&ym);

	BoardAreaOpen("Deconvolution",WND_RAINURES);
	p = PanelCreate(PROD_DECONV_COLS * (1 + VoiesNb)); // printf("(%s) Panel @%08llX\n",__func__,(hex64)p);
	ProdDeconvPanelFill(p);
	BoardAddPanel(bProdChoixTemps,p,10 * Dx,ym + (2 * Dy),0);
	//-	BoardAreaMargesVert(0,Dy);
	BoardAreaEnd(bProdChoixTemps,&x,&y);
	
	BoardAddBouton(bProdChoixTemps,"Sauver",ProdTempsSauve,(xm / 2) - (3 * Dx),y + Dy,0);
}
/* ==========================================================================
static int ProdTempsReference(Panel p, int numitem, void *arg) {
	int msr;

	// printf("(%s) Panel @%08llX\n",__func__,(hex64)p);
	msr = *(int *)arg;
	// printf("(%s) msr=%d\n",__func__,msr);
	if(VoieTemps[msr].prec != VoieTemps[msr].ref) {
		//- ProdTempsPanelFill(p); OpiumRefreshBoard(bProdChoixTemps);
		OpiumClear(bProdChoixTemps); BoardTrash(bProdChoixTemps);
		ProdTempsBoardFill(); OpiumFork(bProdChoixTemps);
	}
	return(1);
}
   ========================================================================== */
static int ProdTempsModifies(Panel p, void *arg) {
	int voie,msr; char ok,refait; // char ref[TEMPS_REF_LNGR],prec[TEMPS_REF_LNGR];

	// printf("(%s) Panel @%08llX\n",__func__,(hex64)p);
	refait = 0;
	for(voie=0; voie<VoiesNb; voie++) {
		if((msr = VoieTemps[voie].mesure) >= 0) {
			// ArgKeyGetText(TEMPS_REF_CLES,VoieTemps[msr].prec,prec,TEMPS_REF_LNGR);
			// ArgKeyGetText(TEMPS_REF_CLES,VoieTemps[msr].ref,ref,TEMPS_REF_LNGR);
			// printf("(%s) | Voie %s: mesure #%d de type %s -> %s\n",__func__,VoieManip[voie].nom,msr,prec,ref);
			if((VoieTemps[msr].ref == TEMPS_SAMBA) && (VoieTemps[msr].prec != TEMPS_SAMBA)) {
				ProdTempsSamba(voie,msr); refait = 1; ok = 1;
			} else ok = ProdTempsVerifie(voie,msr);
		} else { msr = VoieTempsNb; ProdTempsDefaut(voie,msr); VoieTempsNb++; ok = 0; }
		if(!ok || (VoieTemps[msr].prec != VoieTemps[msr].ref)) refait = 1;
		TempsMontee[VoieManip[voie].type] = (int)((VoieTemps[msr].avant - VoieTemps[msr].absol.stop) / VoieEvent[voie].horloge);
		TempsDescente[VoieManip[voie].type] = (int)(VoieTemps[msr].absol.apres / VoieEvent[voie].horloge);
		if(voie == VoieNum) {
			EvtUniteMont = VoieTemps[msr].avant - VoieTemps[msr].absol.stop;
			EvtUniteDesc = VoieTemps[msr].absol.apres;
		}
	}
	// printf("(%s) | Panel %s\n",__func__,refait? "a refaire": "inchange");
	if(refait && p) {
		// ProdTempsPanelFill(p); OpiumRefreshBoard(bProdChoixTemps);
		OpiumClear(bProdChoixTemps); BoardTrash(bProdChoixTemps);
		ProdTempsBoardFill(); OpiumFork(bProdChoixTemps);
	}

	return(0);
}
/* ========================================================================== */
static void ProdTempsReset(Menu menu, MenuItem item) {
	int voie;

	VoieTempsNb = 0; for(voie=0; voie<VoiesNb; voie++) { ProdTempsInit(voie,VoieTempsNb); VoieTempsNb++; }
	ProdTempsStandard = 1;
	// if(yc_fichier) remove(ProdTempsFile);
	OpiumClear(bProdChoixTemps); BoardTrash(bProdChoixTemps);
	ProdTempsBoardFill(); OpiumFork(bProdChoixTemps);
}
/* ========================================================================== */
static int ProdTempsSauve() {
	if(!ArgPrint(ProdTempsFile,ProdTempsDesc))
		OpiumFail("Fichier %s inutilisable (%s)",ProdTempsFile,strerror(errno));
	else printf("* %d temps pour mesures sauves dans le fichier %s\n",VoieTempsNb,ProdTempsFile);
	return(0);
}
/* ========================================================================== */
static int ProdPostTrmts() {
	int voie;

	ArchEvtPattern(ProdPatternActif);
	if(VoiesNb > 1) ArchEvtCrosstalk(ProdCrosstalkActif);
	ArchEvtFilter(ProdFiltreActif);
	for(voie=0; voie<VoiesNb; voie++) 
		if(FiltreVoies[voie] <= 0) ArchSetFilter(voie,0);
		else ArchSetFilter(voie,&(FiltreGeneral[FiltreVoies[voie]-1].coef));
	if((VoiesNb > 1) && ProdCrosstalkActif)
		OpiumReadFloat("Coefficient de crosstalk",&ProdCorrectionCT);

	return(1);
}
/* ==========================================================================
static int ProdPostFiltrage() {
	int voie;

	for(voie=0; voie<VoiesNb; voie++) 
		if(FiltreVoies[voie] <= 0) ArchSetFilter(voie,0);
		else ArchSetFilter(voie,&(FiltreGeneral[FiltreVoies[voie]-1].coef));
	return(1);
}
   ========================================================================== */
inline int AutoriseEvt(int evt) { if(EvtSel) return(EvtSel[evt]); else return(1); }
#ifdef CALCULE_SURFACE
/* ========================================================================== */
void Prod1EvtSurfaces(int voie, int evt, int vt, TypeSignee *adrs, int dim) {
	int t;
	int t_milieu1,t_fin1,t_max1,t_max2;
	int t_bruit,t_first,t_depart,t_milieu,t_fin;
	float val,base,bruit,somme;
	TypeSignee *adrs_debut,max1,max2;
	float ampl_neg,ampl_pos;

	if((adrs == 0) || (dim <= 0)) {
		Amplitude[voie][evt] = 0.0;
//		AmplAdu[voie][evt] = 0.0;
		Integrale[voie][evt] = 0.0;
		Longueur[voie][evt] = 0.0;
		Surface[voie][evt] = 0.0;
		Montee[voie][evt] = 0.0;
		Descente[voie][evt] = 0.0;
	#ifdef BOLOMETRES
		Tevt[voie][evt] = 0.0;
	#endif
		FinEvt[voie] = 0.0;
	#ifdef ETUDIE_CALAGE
		DTmax[voie][evt] = 0.0;
		DTtheo[voie][evt] = 0.0;
	#endif
		return;
	}

	adrs_debut = adrs;

/* Calculations of basic values: niveau et bruit de la ligne de base */
	t_bruit = VoieEvent[voie].avant_evt / 2;
	base = bruit = 0.0;
	for(t=0; t<t_bruit; t++,adrs++) {
		val = (float)*adrs;
		base += val; bruit += (val * val);
	}
	base /= (float)t_bruit;
	Base[voie][evt]  = base;
	Bruit[voie][evt] = sqrtf((bruit / (float)t_bruit) - (base * base));
	Categ[voie][evt] = ArchEvt.categ;

/* Amplitude simple (a ajouter dans le fichier LS) */
	ampl_pos = ampl_neg = 0.0;
	for( ; t<dim; t++,adrs++) {
		val = (float)*adrs - base;
		if(val < ampl_neg) ampl_neg = val;
		if(val > ampl_pos) ampl_pos = val;
	}
	Amplitude[voie][evt] = (ampl_pos > (-ampl_neg))? ampl_pos: ampl_neg;
//	AmplAdu[voie][evt] = Amplitude[voie][evt];

/* Integrale sur les deux premiers pics */
	Surface[voie][evt] = -10000.0;
	bruit = Bruit[voie][evt];
/* find signal first and second zeros */
/* initial start for the signal is set by pretrig (t_bruit) */
	/* localisation du premier pic (plus grand maximum positif) */
	t = VoieEvent[voie].avant_evt / 2; adrs = adrs_debut + t;
	max1 = 0;
	for( ; t<dim; t++,adrs++) {
		if(abs(*adrs) > max1) { max1 = abs(*adrs); t_max1 = t; }
	}

	/* t_first: debut du premier pic (fixe au pretrigger si pic dans le bruit) */
	t_first = VoieEvent[voie].avant_evt;
	if (((float)max1) > (10.0 * bruit)) {
		t = t_max1; adrs = adrs_debut + t;
		for( ; t>VoieEvent[voie].avant_evt/2; t--,adrs--) {
			if((float)abs(*adrs) < ((float)max1 / 10.0)) {
				t_first = t ;
				break;
			}
		}
	}
#ifdef BOLOMETRES
	Tevt[voie][evt] = (float)t_first;
#endif

	/* t_milieu1: fin du premier pic */
	t = t_max1; adrs = adrs_debut + t;
	for( ; t<dim; t++,adrs++) {		
		if ((*adrs * *(adrs+1)) < 0) break;
	}
	t_milieu1 = t;

	/* localisation du deuxieme pic (plus grand maximum positif ou negatif) */
	max2 = 0;
	for( ; t<dim; t++,adrs++) {
		if(abs(*adrs) > max2) { max2 = abs(*adrs); t_max2 = t; }
	}

	/* t_fin: fin du deuxieme pic */
	t = t_max2; adrs = adrs_debut+t;	
	for( ; t<dim; t++,adrs++) {		
		if ((*adrs * *(adrs+1)) < 0) break;
	}
	t_fin1 = t;

	if(voie == 0) {
		t_depart = t_first;
		t_milieu = t_milieu1;
		t_fin    = t_fin1; 
		if(abs(t_depart - 384) > 5) t_depart = 384;
		if(abs(t_milieu - 415) > 5) t_milieu = 415;
		if(abs(t_fin    - 500) > 5) t_fin    = 500;
	} else if(voie >= 1) {
		t_depart = t_first;
		t_milieu = t_milieu1;
		t_fin    = t_fin1;
		if(abs(t_depart - 1024) > 5) t_depart = 1024;
		if(abs(t_milieu - 1044) > 5) t_milieu = 1044;
		if(abs(t_fin    - 1124) > 5) t_fin    = 1124;	
	}

#ifdef AVEC_T
	t0[voie][evt] = (float)t_depart;
	t1[voie][evt] = (float)t_milieu;
	t2[voie][evt] = (float)t_fin;
#endif

#ifdef AVEC_PROTECTIONS
	if((t_milieu1 - t_max1) < 6) return;
	if((t_max2 - t_milieu1) < 6) return;	
	if((t_fin1 - t_max2) < 6)    return;
#endif

	if(t_fin > FinEvt[voie]) FinEvt[voie] = t_fin;
       
/* evaluation of the surface (+first peak -second peak) */
	t = t_depart; adrs = adrs_debut + t;
	somme = 0.0;
	for( ; t<t_milieu; t++,adrs++) {
		val = (float)*adrs;
		somme = somme + (val - base);
	}
	for( ; t<t_fin; t++,adrs++) {
		val = (float)*adrs;
		somme = somme - (val - base);
	}
	Surface[voie][evt] = somme / 1000.0;	

	return;
}
#endif /* CALCULE_SURFACE */
/* ========================================================================== */
void Prod1EvtSamba(int voie, int evt, int vt) {
	Base[voie][evt] = ArchEvt.voie[vt].val[MONIT_BASE];
	Bruit[voie][evt] = ArchEvt.voie[vt].val[MONIT_BRUIT];
	Categ[voie][evt] = ArchEvt.categ;
	Amplitude[voie][evt] = ArchEvt.voie[vt].val[MONIT_AMPL];
//	AmplAdu[voie][evt] = Amplitude[voie][evt];
	Montee[voie][evt] = ArchEvt.voie[vt].val[MONIT_MONTEE];
#ifdef BOLOMETRES
	Tevt[voie][evt] = (float)VoieEvent[voie].avant_evt;
#endif
#ifdef CALCULE_SURFACE
	FinEvt[voie] = (float)VoieEvent[voie].lngr;
#endif
#ifdef COMPARE_NTUPLES
	DelaiSamba[evt] = ArchEvt.delai;
	BaseSamba[voie][evt] = Base[voie][evt];
	BruitSamba[voie][evt] = Bruit[voie][evt];
	AmplitudeSamba[voie][evt] = Amplitude[voie][evt];
	MonteeSamba[voie][evt] = Montee[voie][evt];
#endif
}
/* ========================================================================== */
void ProdChannelBasics(float *adrs, int dim, float *base) {
	int voie,msr,debut_examen,duree_base,t; float *adrs_courante;

	voie = VoieNum;
	msr = VoieTemps[voie].mesure; if(msr < 0) return;
	debut_examen = (int)(VoieTemps[msr].absol.start / VoieEvent[voie].horloge);
	if(debut_examen < 0) debut_examen = 0;
	duree_base = (int)((VoieTemps[msr].absol.stop - VoieTemps[msr].absol.start) / VoieEvent[voie].horloge);
	if(duree_base <= 0) duree_base = 1;
	adrs_courante = adrs + debut_examen;
	*base = 0.0; for(t=0; t<duree_base; t++) *base += *adrs_courante++;
	*base /= (float)duree_base;
}
/* ========================================================================== */
void Prod1Amplitude(int voie, int evt, int vt, TypeSignee *adrs_evt, int duree_evt) {
	int msr;
	int t,t_max,tprec,t25,t75;
#ifdef ETUDIE_CALAGE
	int compactage,decalage;
#endif
	int debut_examen,fin_examen,fin_base,duree_examen,duree_base,duree_montee,duree_descente,duree_integrale;
	char p90_trouve,p50_trouve,p10_trouve,pente_trouvee;
	double val,vprec,base,bruit;
	double ampl,ampl_neg,ampl_pos,somme,amp25,amp75;
	double signe,p10,p90,p50_inf,p50_sup,montee;
	TypeSignee *adrs; double limite_inf,limite_sup,mi_hauteur; int t_neg,t_pos; /* pour le fit */
	double threshold,pente_inverse;
	char log;

	log = 0; // log = (evt == 305);
	/* On evacue le cas ou il vaut mieux s'abstenir */
	if((voie < 0) || (voie >= VoiesNb)) { printf("! (%s) evt#%d: No voie=%d\n",__func__,evt,voie); exit(0); }

	Prod1EvtSamba(voie,evt,vt);
	if((adrs_evt == 0) || (duree_evt <= 0)) {
		Integrale[voie][evt] = 0.0;
		Longueur[voie][evt] = 0.0;
		Descente[voie][evt] = 0.0;
		Duree[voie][evt] = 0.0;
		Pente[voie][evt] = 0.0;
		Retard[voie][evt] = 0.0;
		Categ[voie][evt] = 0;
	#ifdef BOLOMETRES
		Tevt[voie][evt] = 0.0;
	#endif
	#ifdef ETUDIE_CALAGE
		DTmax[voie][evt] = 0.0;
		DTtheo[voie][evt] = 0.0;
	#endif
		return;
	}

	/* Restrictions des zones de calcul */
	/* finalement, duree_montee = (int)((VoieTemps[msr].avant -  VoieTemps[msr].stop) / VoieEvent[voie].horloge); */
	msr = VoieTemps[voie].mesure; if(msr < 0) return;
	debut_examen = (int)(VoieTemps[msr].absol.start / VoieEvent[voie].horloge); if(debut_examen < 0) debut_examen = 0;
	fin_base = (int)(VoieTemps[msr].absol.stop / VoieEvent[voie].horloge); if(fin_base <= debut_examen) fin_base = debut_examen + 1;
	duree_base = fin_base - debut_examen;
	duree_montee = (int)(VoieTemps[msr].avant / VoieEvent[voie].horloge) - fin_base;
	duree_descente = (int)(VoieTemps[msr].absol.apres / VoieEvent[voie].horloge);

	/* On s'assure que ces zones sont valides */
	fin_examen = VoieEvent[voie].avant_evt + duree_descente; // == duree_evt, en principe
	if(fin_examen > VoieEvent[voie].brutes.max) fin_examen = VoieEvent[voie].brutes.max;
	duree_examen = fin_examen - debut_examen;
	if(log) printf("evt %d: examen voie %s @%d, bruit +%d, montee +%d, fin @%d\n",evt,VoieManip[voie].nom,debut_examen,duree_base,duree_montee,fin_examen);

	/* Etude de la ligne de base */
	base = bruit = 0.0;
	t=debut_examen; adrs = adrs_evt + t;
	for(; t<fin_base; t++,adrs++) {
		val = (double)*adrs;
		base += val; bruit += (val * val);
	}
	base /= (double)duree_base;
	Base[voie][evt]  = base;
	Bruit[voie][evt] = sqrtf((bruit / (double)duree_base) - (base * base));
	Categ[voie][evt] = ArchEvt.categ;
//-	Tevt[voie][evt] = (float)VoieEvent[voie].avant_evt;

	/* Recherche du maximum maximurum quel que soit le sens */
	ampl_pos = ampl_neg = 0.0;
	t_pos = t_neg = t;
	for( ; t<fin_examen; t++,adrs++) {
		val = (double)*adrs - base;
		if(val < ampl_neg) { ampl_neg = val; t_neg = t; }
		if(val > ampl_pos) { ampl_pos = val; t_pos = t; }
	}
//	Amplitude[voie][evt] = (ampl_pos > (-ampl_neg))? ampl_pos: ampl_neg;
	if(ampl_pos > (-ampl_neg)) {
		signe = 1.0;
		ampl = ampl_pos;
		t_max = t_pos;
	} else {
		signe = -1.0;
		ampl = -ampl_neg;
		t_max = t_neg;
	}
	if(log) printf("evt %d: vmax=%g, tmax=%d (%g ms)\n",evt,ampl,t_max,(float)(t_max + debut_examen) * VoieEvent[voie].horloge);

	/* Etude de la montee */
	pente_inverse = 0.0;
	somme = 0.0; duree_integrale = 0;
	limite_inf = ampl * ProdMontee10 / 100.0;
	limite_sup = ampl * ProdMontee90 / 100.0;
	if(log) printf("evt %d: mesure montee entre %g%% (%g) et %g%% (%g)\n",evt,ProdMontee10,limite_inf,ProdMontee90,limite_sup);
	
	if(vt < ArchEvt.nbvoies) {
		threshold = (double)ArchEvt.voie[vt].trig_pos;
		if(VoieManip[voie].def.trgr.type == TRGR_SEUIL) threshold -= base;
	} else threshold = limite_inf;

	mi_hauteur = ampl * 0.50;
	p10 = (double)(t_max - duree_montee); p90 = (float)t_max;
	p50_inf = (double)(t_max - (duree_montee / 2)); 
	p50_sup = (double)(t_max + (duree_descente / 2)); 
	p90_trouve = p50_trouve = p10_trouve = pente_trouvee = 0; /* p50_trouve pour p50_inf */
	t = t_max; adrs = adrs_evt + t;
	vprec = signe * ((double)*adrs - base); tprec = t;
	while(t >= fin_base) {
		--adrs;
		--t;
		val = signe * ((double)*adrs - base);
		somme += val; duree_integrale++;
		if(log) printf("evt %d: v(%d) = %d - %g = %g\n",evt,t,*adrs,base,val);
		if((val <= limite_sup) && (vprec != val)) {
			if(!pente_trouvee) {
				if(val < threshold) {
					pente_inverse = (double)(tprec - t) / (vprec - val);
					pente_trouvee = 1;
				}
			}
			if(!p90_trouve) {
				p90 = (double)t + ((limite_sup - val) / (vprec - val)); /* t_prec > t */
				if(log) printf("evt %d: passage 90 @%d pour v:%g -> %g, soit p90=%g\n",evt,t,vprec,val,p90);
				if(log) printf("evt %d: decalage=(%g - %g)/(%g - %g)=%g\n",evt,limite_sup,val,vprec,val,((limite_sup - val) / (vprec - val)));
				p90_trouve = 1;
			}
			if(!p50_trouve && (val <= mi_hauteur)) {
				p50_inf = (double)t + ((mi_hauteur - val) / (vprec - val)); /* t_prec > t */
				p50_trouve = 1;
			}
			if(!p10_trouve && (val <= limite_inf)) {
				p10 = (double)t + ((limite_inf - val) / (vprec - val)); /* t_prec > t */
				if(log) printf("evt %d: passage 10 @%d pour v:%g -> %g, soit p10=%g\n",evt,t,vprec,val,p10);
				if(log) printf("evt %d: decalage=(%g - %g)/(%g - %g)=%g\n",evt,limite_inf,val,vprec,val,((limite_inf - val) / (vprec - val)));
				p10_trouve = 1;
			}
		}
		vprec = val; tprec = t;
	}
	montee = p90 - p10;
	if(log) printf("evt %d: montee calculee: %g\n",evt,montee);
	if(ProdMontee90 > ProdMontee10) montee *= (80.0 / (ProdMontee90 - ProdMontee10));
	if(log) {
		printf("evt %d: correction: x %g\n",evt,(ProdMontee90 > ProdMontee10)? (80.0 / (ProdMontee90 - ProdMontee10)):1.0);
		printf("evt %d: montee retenue: %g\n",evt,montee);
	}

	/* Etude de la descente */
	p50_trouve = 0; /* pour p50_sup, cette fois */
	amp75 = 0.75 * ampl; amp25 = 0.25 * ampl;
	t75 = t25 = -1;
	t = t_max; adrs = adrs_evt + t;
	vprec = signe * ((double)*adrs - base);
	for( ; t<fin_examen; t++,adrs++) {
		val = signe * ((double)*adrs - base);
		if(!p50_trouve) {
			if((val <= mi_hauteur) && (vprec != val)) {
				p50_sup = (double)t - ((mi_hauteur - val) / (vprec - val)); /* t_prec < t */
				p50_trouve = 1;
			} else vprec = val;
		}
		if((t75 < 0) && (val < amp75)) { t75 = (float)t; amp75 = val; }
		if((t25 < 0) && (val < amp25)) { t25 = (float)t; amp25 = val; }
		somme += val; duree_integrale++;
	}

	VoieDeconv[voie] = SambaAssure(VoieDeconv[voie],duree_evt,&(VoieDeconvDim[voie]));
	if(VoieDeconv[voie]) {
		float perte,seuil;
		perte = expf(-VoieEvent[voie].horloge / VoieTemps[msr].deconv.rc);
		seuil = VoieTemps[msr].deconv.seuil;
		Depot[voie][evt] = SambaDeconvolue(VoieDeconv[voie],adrs_evt,duree_evt,0,duree_evt,
			base,VoieTemps[msr].deconv.pre,VoieTemps[msr].deconv.post,perte,seuil,CHARGE_TOTALE);
	}

	/* Affectation du n-tuple */
	Amplitude[voie][evt] = (float)(signe * ampl);
//	AmplAdu[voie][evt] = Amplitude[voie][evt];
	if(duree_integrale) Integrale[voie][evt] = (float)(signe * somme) / (float)duree_integrale; //  * VoieEvent[voie].horloge;
	if(Amplitude[voie][evt] != 0.0) Longueur[voie][evt] = Integrale[voie][evt] / Amplitude[voie][evt];
	Montee[voie][evt] = (float)montee * VoieEvent[voie].horloge;
	Duree[voie][evt] = (float)(p50_sup - p50_inf) * VoieEvent[voie].horloge;
	Descente[voie][evt] = (float)(p50_sup - p50_inf - (ProdQuotaMontee * montee)) * VoieEvent[voie].horloge;
	if((amp25 > 0.0) && (amp75 > 0.0)) RCmesure[voie][evt] = ((t75 - t25) / logf(amp25 / amp75)) * VoieEvent[voie].horloge;
	Pente[voie][evt] = (float)pente_inverse * VoieEvent[voie].horloge * 1000.0; // us/ADU
	if((ArchEvt.voie[vt].maxigiga == 0) && (ArchEvt.voie[vt].maxistamp == 0)) {
		// if(evt == 100) printf("(%s) Evt a %2d.%09d\n",__func__,ArchEvt.gigastamp,ArchEvt.stamp);
		Retard[voie][evt] = ((float)(ArchEvt.voie[vt].sec - (ArchEvt.sec - ArchT0sec)) * 1000.0) + ((float)(ArchEvt.voie[vt].msec - (ArchEvt.msec - ArchT0msec)) / 1000.0)
		 + ((float)(ArchEvt.voie[vt].avant_evt) * VoieEvent[ArchEvt.voie[vt].num].horloge);
	} else {
		/* if(evt == 100) {
			printf("(%s) Ptm a %2d.%09d\n",__func__,(int)(ArchEvt.voie[vt].ptmax/1000000000),Modulo((ArchEvt.voie[vt].ptmax),1000000000));
			printf("(%s) Evt a %2d.%09d\n",__func__,ArchEvt.gigastamp,ArchEvt.stamp);
			printf("(%s) T0  a %2d.%09d\n",__func__,GigaStamp0,TimeStamp0);
			printf("(%s) Max a %2d.%09d\n",__func__,(ArchEvt.gigastamp - GigaStamp0),(ArchEvt.stamp - TimeStamp0));
			printf("(%s) V%d  a %2d.%09d\n",__func__,voie,ArchEvt.voie[vt].maxigiga,ArchEvt.voie[vt].maxistamp);
			printf("(%s) Dif = %2d.%09d\n",__func__,(ArchEvt.voie[vt].maxigiga - (ArchEvt.gigastamp - GigaStamp0)),(ArchEvt.voie[vt].maxistamp - (ArchEvt.stamp - TimeStamp0)));
		} */
		Retard[voie][evt] = (float)(((ArchEvt.voie[vt].maxigiga - (ArchEvt.gigastamp - GigaStamp0)) * 1000000000)
									+ (ArchEvt.voie[vt].maxistamp - (ArchEvt.stamp - TimeStamp0))) * VoieEvent[voie].horloge;
	}

#ifdef BOLOMETRES
	Tevt[voie][evt] = (float)t_max;
#endif
#ifdef CALCULE_SURFACE
	if(FinEvt[voie] < (float)fin_examen) FinEvt[voie] = (float)fin_examen;
#endif
#ifdef ETUDIE_CALAGE
	compactage = (int)((VoieEvent[voie].horloge * Echantillonage) + 0.3);
	decalage = (int)(((ArchEvt.gigastamp - ArchEvt.voie[vt].gigastamp) * 1000000000) + (ArchEvt.stamp - ArchEvt.voie[vt].stamp));
	DTmax[voie][evt] = decalage - (t_max * compactage);
	DTtheo[voie][evt] = decalage - (VoieEvent[voie].avant_evt * compactage);
#endif
    
	if(log) printf("evt %d: voie %s terminee\n",evt,VoieManip[voie].nom);

	return;
}
#define MAXLIGNENTP 1024
/* ========================================================================== */
int ProdPaquetsTrouve(Menu menu, MenuItem item) {
	char paquet_en_cours; TypeS_us paquet_duree,paquet_debut,paquet_fin; float paquet_numero;
	int evt,voie;
	char log;

	if(EvtDateExacte == 0) { OpiumWarn("Pas possible avec l'option -m=0"); return(0); }
	else if(!EvtPaquetNum || !EvtDecalage) { OpiumWarn("Pas programme si plusieurs voies"); return(0); }
	log = 0;
	paquet_en_cours = 0;
	paquet_debut.s_us = paquet_fin.s_us = 0; paquet_numero = 0.0; PaqNb = 0;
	paquet_duree.s_us = S_usFromFloat(ProdPaquetDuree);
	printf("* Recherche de paquets de %d,%06d sec parmi %d evenement%s\n",paquet_duree.t.s,paquet_duree.t.us,Accord1s(EvtNb));
	for(evt=0; evt<EvtNb; evt++) {
		float diff;
//		log = (evt < 20);
		diff = paquet_en_cours? S_usFloat_us(S_usOper(&(EvtDateExacte[evt]),S_US_MOINS,&paquet_debut)): -1.0;
		voie = 0;
//		if(log) printf("Evt #%d (%g/%d @%d,%06d/%d,%06d): paquet %s\n",evt,
//			Amplitude[voie][evt],ProdPaquetAmpl,EvtDateExacte[evt].t.s,EvtDateExacte[evt].t.us,paquet_fin.t.s,paquet_fin.t.us,
//			paquet_en_cours?"en cours":"indefini");
		if(paquet_en_cours) {
			TypeS_us dt;
			dt.s_us = S_usOper(&(EvtDateExacte[evt]),S_US_MOINS,&paquet_fin);
//			if(log) printf(". decalage de %d,%06d\n",dt.t.s,dt.t.us);
			if(dt.t.s >= 0) paquet_en_cours = 0;
			else {
				PaqAmpTotale[PaqNb-1] += Amplitude[voie][evt];
				PaqMultiplicite[PaqNb-1] += 1.0;
				PaqEtendue[PaqNb-1] = diff;
			}
		}
		if(!paquet_en_cours && (Amplitude[voie][evt] > ProdPaquetAmpl)) {
			paquet_debut.s_us = EvtDateExacte[evt].s_us;
			paquet_fin.s_us = S_usOper(&paquet_debut,S_US_PLUS,&paquet_duree);
			paquet_numero += 1.0;
			paquet_en_cours = 1;
//			if(log) printf("-> paquet #%g se terminant a %d,%06d\n",paquet_numero,paquet_fin.t.s,paquet_fin.t.us);
			diff = 0.0;
			PaqEvtPremier[PaqNb] = evt;
			PaqAmpPremier[PaqNb] = Amplitude[voie][evt];
			PaqAmpTotale[PaqNb] = Amplitude[voie][evt];
			PaqMultiplicite[PaqNb] = 1.0;
			PaqEtendue[PaqNb] = diff;
			PaqNb++;
		}
		EvtPaquetNum[evt] = paquet_en_cours? paquet_numero: 0.0;
		EvtDecalage[evt] = paquet_en_cours? diff: -1.0;
		/* if(log) {
			TypeS_us dt;
			dt.s_us = S_usOper(&(EvtDateExacte[evt]),S_US_MOINS,&paquet_debut);
			printf(". evt #%d dans le paquet %g avec un decalage de %d,%06d, soit %g sec.\n",evt,EvtPaquetNum[evt],dt.t.s,dt.t.us,EvtDecalage[evt]);
		} */
	}
	printf("* %d Paquet%s trouve%s (dernier numero: %g)\n",Accord2s(PaqNb),paquet_numero);
	return(0);
}
/* ========================================================================== */
static int ProdCalculeDelai(int evt) {
	if(EvtDateExacte) {
		if(ProdDatePrecedent.s_us == 0) EvtDelaiMesure[evt] = 0.0;
		else EvtDelaiMesure[evt] = S_usFloat_us(S_usOper(&EvtDateExacte[evt],S_US_MOINS,&ProdDatePrecedent));
		ProdDatePrecedent.s_us = EvtDateExacte[evt].s_us;
	} else {
		if(ProdDateApprox < 0.0) EvtDelaiMesure[evt] = 0.0;
		else EvtDelaiMesure[evt] = EvtDateMesuree[evt] - ProdDateApprox;
		ProdDateApprox = EvtDateMesuree[evt];
	}
	return(0);
}
/* ========================================================================== */
int ProdAjusteDelais(Menu menu, MenuItem item) {
	int evt;
	
	printf("* Ajustement des delais parmi %d evenement%s passant la coupure\n",Accord1s(PlotNtupleValides));
	ProdDatePrecedent.s_us = 0; ProdDateApprox = -1.0;
	for(evt=0; evt<EvtNb; evt++) if(PlotNtuplePresent(EvtEspace,evt)) ProdCalculeDelai(evt);
	printf("* %d delai%s ajuste%s\n",Accord2s(PlotNtupleValides));

	return(0);
}
/* ========================================================================== */
int ProdCalculeTaux(Menu menu, MenuItem item) {
	float base,t; int evt; int nbval,i,n;
	char titre[80]; int temps,taux;

	if(EvtDateMesuree == 0) { OpiumFail(L_("Dates non calculees")); return(0); }
	base = ProdTauxDt[1];
	do {
		if(OpiumReadFloat(L_("Base de temps (s)"),&base) == PNL_CANCEL) return(0);
		if(base > 0.0) break; else OpiumFail(L_("Base de temps invalide: %g"),base);
	} while(1);
	ProdTauxDt[1] = base;
	nbval = (short)((float)RunTempsTotal)/ProdTauxDt[1];
	if(nbval > ProdTauxMax) {
		if(ProdTauxTab) { free(ProdTauxTab); ProdTauxMax = 0; }
		ProdTauxTab = (float *)malloc(nbval * sizeof(nbval));
		if(ProdTauxTab) ProdTauxMax = nbval;
	}
	if(!ProdTauxTab) {
		OpiumFail("Manque de memoire pour %d taux (%d/%g)\n",nbval,RunTempsTotal,ProdTauxDt[1]);
		return(1);
	}
	sprintf(titre,L_("Taux d'evenements (Hz) [Dt = %g s]"),ProdTauxDt[1]);
	OpiumProgresTitle(titre);
	OpiumProgresInit(EvtNb);
	i = 0; t = ProdTauxDt[1]; n = 0;
	for(evt=0; evt<EvtNb; evt++) {
		if(!OpiumProgresRefresh(evt)) break;
		if(EvtDateMesuree[evt] >= t) {
			do {
				if(i >= ProdTauxMax) break;
				ProdTauxTab[i++] = (float)n / ProdTauxDt[1];
				t += ProdTauxDt[1]; n = 0;
			} while(t < EvtDateMesuree[evt]);
		}
		if(PlotNtuplePresent(EvtEspace,evt)) n++;
	}
	OpiumProgresClose();
	if((n > 0) && (evt < EvtNb) && (i < ProdTauxMax)) {
		base = EvtDateMesuree[evt] - (t - ProdTauxDt[1]);
		if(base > 0.0) ProdTauxTab[i++] = (float)n / base;
	}
	ProdTauxNb = i;

	if(gTauxEvts) {
		if(OpiumDisplayed(gTauxEvts->cdr)) OpiumClear(gTauxEvts->cdr);
		GraphErase(gTauxEvts);
	} else gTauxEvts = GraphCreate(ArchGraphLarg,ArchGraphHaut,4);
	GraphMode(gTauxEvts,GRF_2AXES | GRF_LEGEND);
	GraphParmsSave(gTauxEvts);
	OpiumTitle(gTauxEvts->cdr,titre);
	temps = GraphAdd(gTauxEvts,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,ProdTauxDt,ProdTauxNb); GraphDataName(gTauxEvts,temps,"Temps (s)");
	taux = GraphAdd(gTauxEvts,GRF_FLOAT|GRF_YAXIS,ProdTauxTab,ProdTauxNb); GraphDataName(gTauxEvts,taux,"Evts/s (Hz)");
	GraphDataRGB(gTauxEvts,taux,GRF_RGB_GREEN);
	GraphAxisAutoRange(gTauxEvts,GRF_XAXIS); GraphAxisAutoRange(gTauxEvts,GRF_YAXIS);
	GraphUse(gTauxEvts,ProdTauxNb); GraphAxisReset(gTauxEvts,GRF_XAXIS); GraphAxisReset(gTauxEvts,GRF_YAXIS);

	OpiumDisplay(gTauxEvts->cdr);

// #define AFFICHE_COUPURES
#ifdef AFFICHE_COUPURES
	WndFrame f;
	WndContextPtr gc_ab6; int xaxe1,lx,yp; char texte[80];
	if(OpiumCoordFenetre(gTauxEvts->cdr,&f)) {
		/* GraphScale *xscale; 
		xscale = gTauxEvts->axe;
		gc_ab6 = (gTauxEvts->data_ptr[xscale->num])->gc[f->qualite];
		if(xscale->reel) xaxe1 = GraphPosFloat(1,xscale->u.r.axe,xscale);
		 else xaxe1 = GraphPosInt(1,xscale->u.i.axe,xscale); */
		gc_ab6 = (gTauxEvts->data_ptr[0])->gc[FondPlots];
		xaxe1 = 4 * Dx;
		lx = xaxe1 + Dx; yp = Dy;
		if(PlotSelectNb == 0) WndString(f,gc_ab6,lx,yp,"(sans coupure)");
		else {
			sprintf(texte,"   %16s dans [%g .. %g]: %d/%d\n",PlotVarList[Coupure[0].var],
				Coupure[0].min,Coupure[0].max,Coupure[0].valides,Coupure[0].total); yp += Dy;
			WndString(f,gc_ab6,lx,yp,texte);
			for(i=1; i<PlotSelectNb; i++) {
				sprintf(texte,"%2s %16s dans [%g .. %g]: %d/%d\n",(Coupure[i].oper == 1)? "et": "ou",
					PlotVarList[Coupure[i].var],Coupure[i].min,Coupure[i].max,Coupure[i].valides,Coupure[i].total);
				WndString(f,gc_ab6,lx,yp,texte);
				yp += Dy;
			}
		}
		sprintf(texte,"%d evenement%s valide%s",Accord2s(PlotNtupleValides));
		WndString(f,gc_ab6,lx,yp,texte);
	}
#endif

	return(0);
}
/* ========================================================================== */
static float ProdIntervalle(int t1sec, int t1msec, int t0sec, int t0msec) {
	return((float)(t1sec - t0sec) + ((float)(t1msec - t0msec) / 1000000.0));
}
/* ========================================================================== */
int ProdMixte(char variante, char raz) {
	int evt,centieme; char debug;
	int voie,bolo,bolo_touche,vt,dim,var;
	TypeSignee *adrs;
	char nom_complet[256]; char ligne[MAXLIGNENTP]; FILE *ntp; int num_fichier;
	char *item,*c; int i; char rc;
//	int delta_t; 
	float energie_max,coef;
	char paquet_en_cours; TypeS_us paquet_duree,paquet_debut,paquet_fin; float paquet_numero;
#ifdef BOLOMETRES
	int capt,vcent,vgard;
	float chaleur,centre;
#endif

	if(!EvtNb) { PlotPrint(OPIUM_ERROR,"Pas d'evenement lu, calcul abandonne"); return(1); }
	MesureFaite = variante;
	centieme = EvtNb / 100; 
	if(!ModeScript) {
	#ifdef NTP_PROGRES_INTEGRE
		TangoProdReset(EvtNb);
	#else
		char titre[80];
		sprintf(titre,"Prod %s",NomMesure[variante]);
		OpiumProgresTitle(titre);
		OpiumProgresInit(EvtNb);
	#endif
		printf("- Production du ntuple par la routine %s(%s,%s)\n",__func__,NomMesure[variante],raz?"RAZ":"continue");
	} else printf(". Calcul du ntuple demarre (%d evenements)\n",EvtNb);
	coef = TauxCentrePur / (1.0 - TauxCentrePur);

/* Boucle sur tous les evenements enregistres */
	paquet_en_cours = 0;
	paquet_debut.s_us = paquet_fin.s_us = 0; paquet_numero = 0.0; PaqNb = 0;
	paquet_duree.s_us = S_usFromFloat(ProdPaquetDuree);
	ntp = 0; num_fichier = 0; 
	// delta_t = 0;
	ProdDatePrecedent.s_us = 0; ProdDateApprox = -1.0;
	evt = EvtsAnalyses - 1;
	do {
		if(++evt >= EvtNb) break;
		debug = 0; // (evt >= 2354);
		if(debug) printf("========== Traitement de l'evenement #%d ==========\n",evt);
		if(!ModeScript) {
		#ifdef NTP_PROGRES_INTEGRE
			if(!TangoProdContinue(evt)) break;
		#else
			if(!OpiumProgresRefresh(evt)) break;
		#endif
		} else if(!(evt % centieme)) { printf(". %d%% des evenements ont ete traites\r",evt/centieme); fflush(stdout); }
//		if(!AutoriseEvt(evt)) continue;
		if(EvtSel) { if(!EvtSel[evt]) continue; }
		rc = EvtLitASuivre(evt,!ModeScript);
		if(debug) printf(". Evenement #%d lu\n",evt);
		if(rc == 0) break;
		else if(rc < 0) {
			if(EvtSel) EvtSel[evt] = 0;
			continue;
		}
		PlotNtupleMarque(EvtEspace,evt,ArchEvt.tag.num);
		PlotNtupleColore(EvtEspace,evt,ArchEvt.tag.r,ArchEvt.tag.g,ArchEvt.tag.b);
		NumEvt[evt] = (float)(ArchEvt.num);
		if(VsnManip == 1) {
			if(EvtRunInfo[num_fichier].premier == evt) {
				if(ntp) fclose(ntp);
				strcat(strcpy(nom_complet,EvtRunInfo[num_fichier].nom),".NTP");
				if((ntp = TangoFileOpen(nom_complet,"r"))) LigneSuivante(ligne,MAXLIGNENTP,ntp);
			}
		#ifdef CONDITIONS_MESURE
			TemperLue[evt] = 0.001;  /* soit 1 mK */
		#endif
			if(ntp)	{
				if(LigneSuivante(ligne,MAXLIGNENTP,ntp)) {
					sscanf(ligne,"%g",&(EvtDateMesuree[evt]));
					/* Lecture de la temperature (!...) */
					c = ligne;
					item = ItemSuivant(&c,' '); /* date */
					item = ItemSuivant(&c,' '); /* code trigger */
					for(voie=0; voie<6; voie++) /* 6 voies enregistrees (!?) */
						for(i=0; i<5; i++)      /* 5 infos par voie */
							item = ItemSuivant(&c,' ');
					item = ItemSuivant(&c,' '); /* TimeShift */
					item = ItemSuivant(&c,' '); /* Voie maitre */
					item = ItemSuivant(&c,' ');
				#ifdef CONDITIONS_MESURE
					if(item[0]) sscanf(item,"%g",&(TemperLue[evt]));
				#endif
				}
			} else EvtDateMesuree[evt] = 0.0;
		} else if(VsnManip == 2) {
			//- EvtDateMesuree[evt] = (float)(ArchEvt.sec - EvtRunInfo[0].t0sec) + ((float)(ArchEvt.msec - EvtRunInfo[0].t0msec) / 1000000.0);
			if(EvtDateExacte) {
                TypeS_us t1; S_us t2;
				t1.s_us = S_usFromInt(ArchEvt.sec,ArchEvt.msec);
//				if(evt < 20) printf("Evt #%d @%d,%06d soit @%d,%06d pour T0=%d,%06d\n",ArchEvt.sec,ArchEvt.msec,t1.t.s,t1.t.us,EvtRunT0.t.s,EvtRunT0.t.us);
				EvtDateExacte[evt].s_us = S_usOper(&t1,S_US_MOINS,&EvtRunT0);
//				if(evt < 20) printf("   soit date=%d,%06d\n",EvtDateExacte[evt].t.s,EvtDateExacte[evt].t.us);
                t2 = &(EvtDateExacte[evt]);
                EvtDateMesuree[evt] = S_usFloat(t2);
			} else {
				EvtDateMesuree[evt] = ProdIntervalle(ArchEvt.sec,ArchEvt.msec,EvtRunInfo[0].t0sec,EvtRunInfo[0].t0msec);
			}
		#ifdef CONDITIONS_MESURE
			TemperLue[evt] = ReglTbolo;
		#endif
		}
		ProdCalculeDelai(evt);
		if(ArchEvt.nbvoies > 1) {
			//  printf("(%s) Evt[%d]:\n",__func__,evt);
			//1 printf("(%s) | %-12s @%3d+%09d\n",__func__,VoieManip[ArchEvt.voie[1].num].prenom,ArchEvt.voie[1].gigastamp,ArchEvt.voie[1].stamp);
			//1 printf("(%s) | %-12s @%3d+%09d\n",__func__,VoieManip[ArchEvt.voie[0].num].prenom,ArchEvt.voie[0].gigastamp,ArchEvt.voie[0].stamp);
			//2 printf("(%s) | %-12s @%3d,%06d\n",__func__,VoieManip[ArchEvt.voie[1].num].prenom,ArchEvt.voie[1].sec,ArchEvt.voie[1].msec);
			//2 printf("(%s) | %-12s @%3d,%06d\n",__func__,VoieManip[ArchEvt.voie[0].num].prenom,ArchEvt.voie[0].sec,ArchEvt.voie[0].msec);
			//2 printf("(%s) | > Dsec = %g, Dmsec = %g, Dpre = %g\n",__func__,(float)((ArchEvt.voie[1].sec - ArchEvt.voie[0].sec) * 1000),
			//2    ((float)(ArchEvt.voie[1].msec - ArchEvt.voie[0].msec) / 1000.0),
			//2    ((float)(ArchEvt.voie[1].avant_evt) * VoieEvent[ArchEvt.voie[1].num].horloge) - ((float)(ArchEvt.voie[0].avant_evt) * VoieEvent[ArchEvt.voie[0].num].horloge));
			EcartT0[evt] = (float)((ArchEvt.voie[1].sec - ArchEvt.voie[0].sec) * 1000) + ((float)(ArchEvt.voie[1].msec - ArchEvt.voie[0].msec) / 1000.0)
				+ ((float)(ArchEvt.voie[1].avant_evt) * VoieEvent[ArchEvt.voie[1].num].horloge) - ((float)(ArchEvt.voie[0].avant_evt) * VoieEvent[ArchEvt.voie[0].num].horloge);
			//2 printf("(%s) | >> Dmax = %g\n",__func__,EcartT0[evt]);
		#ifdef BOLOMETRES
			Quenching[evt] = Recul[evt] = Chaleur[evt] = Ionisation[evt] = 0.0;
		#endif
		} else if(VoiesNb > 1) EcartT0[evt] = 0.0;
		bolo_touche = 0; energie_max = 0.0; bolo = -1;

		for(vt=0; vt<ArchEvt.nbvoies; vt++) {
			voie = ArchEvt.voie[vt].num;
			if((voie < 0) || (voie >= VoiesNb)) {
				printf("! (%s) evt#%d @%llx, voie#%d/%d: Num voie=%d\n",__func__,evt,EvtPos[evt],vt,ArchEvt.nbvoies,voie);
				break;
			}
			if(debug) printf(". Voie #%d examinee\n",voie);
			adrs = (TypeSignee *)VoieEvent[voie].brutes.t.sval;
			dim = ArchEvt.voie[vt].dim; // si dim==0, VoieEvent n'est pas modifie p.r.precedent (et VoieEvent[voie].lngr peut etre non nul...)
			Base[voie][evt] = 0.0; // ArchEvt.voie[vt].base;
			switch(variante) {
			  case EDW_SAMBA: Prod1EvtSamba(voie,evt,vt); break;
			  case EDW_AMPL: Prod1Amplitude(voie,evt,vt,adrs,dim); break;
			#ifdef CALCULE_SURFACE
			  case EDW_SURF: Prod1EvtSurfaces(voie,evt,vt,adrs,dim); break;
			#endif
			}
            for(var=0; var<ProdVarNb; var++) if(ProdVar[var].a_calculer && (ProdVar[var].niveau == 1)) {
                EvtVarCalcule(evt,ProdVar[var].var1,ProdVar[var].oper,ProdVar[var].var2,VarVoie[ProdVar[var].num][voie]);
            }
			if(debug) printf(". Ntuple calcule\n");
		#ifdef BOLOMETRES
			if(VoiesNb > 1) {
				if(VoieManip[voie].type == VoieChaleur) {
					switch(variante) {
					  case EDW_AMPL: case EDW_SAMBA: chaleur = fabsf(Amplitude[voie][evt]); break;
					#ifdef CALCULE_SURFACE
					  case EDW_SURF: chaleur = fabsf(Surface[voie][evt]); break;
					#endif
					}
					if(chaleur > energie_max) {
						energie_max = chaleur;
						bolo_touche = VoieManip[voie].det;
					}
				} else if(VoieManip[voie].type == VoieCentre) {
					vcent = voie; vgard = -1;
					bolo = VoieManip[voie].det;
					for(capt=0; capt<Bolo[bolo].nbcapt; capt++) {
						if(VoieManip[Bolo[bolo].captr[capt].voie].type == VoieGarde)  vgard = Bolo[bolo].captr[capt].voie;
					}
					if(vgard >= 0) {
						if(abs(Amplitude[vgard][evt]) > abs(Amplitude[vcent][evt])) {
							Amplitude[vcent][evt] = *((TypeSignee *)VoieEvent[vcent].brutes.t.sval+(int)Tevt[vgard][evt]) - Base[vcent][evt];
						} else {
							Amplitude[vgard][evt] = *((TypeSignee *)VoieEvent[vgard].brutes.t.sval+(int)Tevt[vcent][evt]) - Base[vgard][evt];
						}
					}
				}
			}
	#endif /* BOLOMETRES */
		}
		if(vt < ArchEvt.nbvoies) break;
        for(var=0; var<ProdVarNb; var++) if(ProdVar[var].a_calculer && (ProdVar[var].niveau == 0)) {
            for(bolo=0; bolo<BoloNb; bolo++)
                EvtVarCalcule(evt,ProdVar[var].var1,ProdVar[var].oper,ProdVar[var].var2,VarDetec[ProdVar[var].num][bolo]);
        }
		if(EvtPaquetNum && EvtDecalage && EvtDateExacte) {
			float diff;
			diff = paquet_en_cours? S_usFloat_us(S_usOper(&(EvtDateExacte[evt]),S_US_MOINS,&paquet_debut)): -1.0;
			voie = 0;
			if(paquet_en_cours) {
//				if(EvtDateExacte[evt] > paquet_fin) paquet_en_cours = 0;
				TypeS_us dt;
				dt.s_us = S_usOper(&(EvtDateExacte[evt]),S_US_MOINS,&paquet_fin);
				if(dt.t.s > 0) paquet_en_cours = 0;
				else {
					PaqAmpTotale[PaqNb-1] += Amplitude[voie][evt];
					PaqMultiplicite[PaqNb-1] += 1.0;
					PaqEtendue[PaqNb-1] = diff;
				}
			}
			if(!paquet_en_cours && (Amplitude[voie][evt] > ProdPaquetAmpl)) {
				paquet_debut.s_us = EvtDateExacte[evt].s_us;
				paquet_fin.s_us = S_usOper(&paquet_debut,S_US_PLUS,&paquet_duree);
				paquet_numero += 1.0;
				paquet_en_cours = 1;
				diff = 0.0;
				PaqEvtPremier[PaqNb] = evt;
				PaqAmpPremier[PaqNb] = Amplitude[voie][evt];
				PaqAmpTotale[PaqNb] = Amplitude[voie][evt];
				PaqMultiplicite[PaqNb] = 1.0;
				PaqEtendue[PaqNb] = diff;
				PaqNb++;
			}
			EvtPaquetNum[evt] = paquet_en_cours? paquet_numero: 0.0;
			EvtDecalage[evt] = paquet_en_cours? diff: -1.0;
		}

		if(VoiesNb > 1) {
			BoloTouche[evt] = (float)bolo_touche + 1.0;
		#ifdef BOLOMETRES
			VoieIonisee[evt] = 1.0;
			vcent = vgard = -1;
			if(bolo >= 0) for(capt=0; capt<Bolo[bolo].nbcapt; capt++) {
				voie = Bolo[bolo].captr[capt].voie;
				if(VoieManip[voie].type == VoieCentre) vcent = voie;
				else if(VoieManip[voie].type == VoieGarde) vgard = voie;
			}
			if((vcent >= 0) && (vgard >= 0)) {
				if(ProdCrosstalkActif) {
					centre = Amplitude[vcent][evt];
					Amplitude[vcent][evt] -= ProdCorrectionCT * Amplitude[vgard][evt];
					Amplitude[vgard][evt] -= ProdCorrectionCT * centre;
				#ifdef CALCULE_SURFACE
					if(variante == EDW_SURF) {
						centre = Surface[vcent][evt];
						Surface[vcent][evt] -= ProdCorrectionCT * Surface[vgard][evt];
						Surface[vgard][evt] -= ProdCorrectionCT * centre;
					}
				#endif
				}
				switch(variante) {
				  case EDW_AMPL: case EDW_SAMBA:
					VoieIonisee[evt] = 
						(float)(((fabsf(Amplitude[vcent][evt]) > (coef * fabsf(Amplitude[vgard][evt])))? VoieCentre: VoieGarde) + 1);
					break;
				#ifdef CALCULE_SURFACE
				  case EDW_SURF:
					VoieIonisee[evt] = 
						(float)(((fabsf(Surface[vcent][evt]) > (coef * fabsf(Surface[vgard][evt])))? VoieCentre: VoieGarde) + 1);
					break;
				#endif
				}
			}
		#endif /* BOLOMETRES */
		}
		EvtsAnalyses = evt + 1;
		if(!ModeScript) { if(OpiumDisplayed(pEvtAnalyses->cdr)) PanelRefreshVars(pEvtAnalyses); }
	} while(1);

/* On ferme la boutique */
	//- printf("(%s) Production terminee, %d/%d evenements traites",__func__,EvtsAnalyses,EvtNb);
	if(ntp)	fclose(ntp);
	//- EvtsAnalyses = evt;
	if(!ModeScript) {
		if(EvtsAnalyses < EvtNb) OpiumError("Fichier abandonne au bout de %d evenements (%d%%)",EvtsAnalyses,(EvtsAnalyses * 100 / EvtNb));
		else {
			// printf("(%s) Fichier termine, %d evenements traites\n",__func__,EvtsAnalyses);
			OpiumNote("Fichier termine, %d evenements traites",EvtsAnalyses);
			// OpiumNote("Fichier termine, tous les evenements ont ete traites");
			ModeScript = 1; EvtNtupleEcrit(); ModeScript = 0;
		}
	#ifndef NTP_PROGRES_INTEGRE
		OpiumProgresClose();
	#endif
		if(OpiumDisplayed(pEvtAnalyses->cdr)) PanelRefreshVars(pEvtAnalyses);
	}
	if(EvtsAnalyses < EvtNb) printf(". Fichier abandonne au bout de %d evenements (%d%%)\n",EvtsAnalyses,(EvtsAnalyses * 100 / EvtNb));
	else printf(". Fichier termine, %d evenements traites\n",EvtsAnalyses);
	printf("* %d Paquet%s trouve%s (dernier numero: %g)\n",Accord2s(PaqNb),paquet_numero);
	AfficheNtuple = 1;
//	ProdCalibApplique();
#ifdef CALCULE_SURFACE
	for(voie=0; voie<VoiesNb; voie++)
		if(FinEvt[voie]) printf(". Normalisation %s: [%d..%d[\n",
			VoieManip[voie].nom,VoieEvent[voie].avant_evt/2,FinEvt[voie]);
		else printf(". Pas de %s dans ce run\n",VoieManip[voie].nom);
#endif

	return(1);
}
/* ========================================================================== */
int ProdSauveNtuples() {
	if(EvtsAnalyses) {
		ModeScript = 1; EvtNtupleEcrit(); ModeScript = 0;
		if(OpiumAlEcran(mNtpSauve)) MenuItemAllume(mNtpSauve,1,0,GRF_RGB_GREEN);
	} else OpiumWarn("Pas de ntuple calcule");
	return(0);
}
#ifdef CALCULE_SURFACE
/* ========================================================================== */
int ProdSurfaces() { return(ProdMixte(EDW_SURF,1)); }
#endif
/* ========================================================================== */
int ProdAmplitudes() { return(ProdMixte(EDW_AMPL,1)); }
/* ========================================================================== */
int ProdSamba() { return(ProdMixte(EDW_SAMBA,1)); }
/* ========================================================================== */
int ProdComplete() {
	int rc; char explic[256];
	
#define RELECTURE_PARTIELLE
#ifdef RELECTURE_PARTIELLE
	EvtRunNb--;
	OctetsDejaLus = EvtRunInfo[EvtRunNb].debut;
	EvtNb = EvtRunInfo[EvtRunNb].premier;
	EventPartDebut = TrancheRelue;
#endif
	
	sprintf(explic,"Incident indetermine depuis %s",__func__);
	if(!(rc = ArchRunOpenRange(FichierEvents,EventPartDebut,EventPartFin,1,explic))) OpiumError(explic);
	return(ProdMixte(MesureFaite,0));
}
#ifdef BOLOMETRES
/* ========================================================================== */
int CalculeEnergies() {
	float *mesure[VOIES_MAX]; int voie;
	int evt,vt,bolo,touche; int capt,vcent,vgard;
	float chaleur;
	int n,i; double s,c,cn;

	if(VoiesNb < 2) {
		OpiumError("Ce calcul n'est pas applicable a ce detecteur");
		return(0);
	}
	if(OpiumExec(pEnergies->cdr) == PNL_CANCEL) return(0);
	switch(MesurePourCalib) {
	  case EDW_AMPL: 
		for(voie=0; voie<VoiesNb; voie++) mesure[voie] = Amplitude[voie];
		break;
#ifdef CALCULE_SURFACE
	  case EDW_SURF: 
		for(voie=0; voie<VoiesNb; voie++) mesure[voie] = Surface[voie];
		break;
#endif
	  case EDW_EVT: 
		for(voie=0; voie<VoiesNb; voie++) mesure[voie] = Energie[voie];
		break;
	}
	printf("* Calcul des energies (d'apres les %s)\n",NomMesure[MesurePourCalib]);
	for(evt=0; evt<EvtNb; evt++) {
		touche = (int)(BoloTouche[evt] - 0.9);
		vcent = vgard = -1;
		for(capt=0; capt<Bolo[touche].nbcapt; capt++) {
			voie = Bolo[touche].captr[capt].voie;
			if(VoieManip[voie].type == VoieCentre) vcent = voie;
			else if(VoieManip[voie].type == VoieGarde) vgard = voie;
		}
		if((vcent < 0) || (vgard < 0)) continue;
		Ionisation[evt] = mesure[vcent][evt] + mesure[vgard][evt];
/*		Chaleur[evt] = mesure[0][touche][evt]; */
		/* On regarde la chaleur de tous les bolos pour voir combien sont touches */
		NbTouches[evt] = 0.0;
	#ifdef BOLOMETRES
		for(vt=0; vt<ArchEvt.nbvoies; vt++) {
			voie = ArchEvt.voie[vt].num;
			if(VoieManip[voie].type == VoieChaleur) {
				bolo = VoieManip[voie].det;
				c = (double)mesure[voie][evt];
				for(n=0; n<NbIntervChal[bolo]; n++) {
					if((Ionisation[evt] >= CoefChal[bolo][n].inf) && (Ionisation[evt] < CoefChal[bolo][n].sup)) {
						s = 0.0;
						cn = 1.0;
						for(i=0; i<CoefChal[bolo][n].nb; i++) {
							s += ((double)CoefChal[bolo][n].c[i] * cn);
							cn *= c;
						}
						chaleur = (float)s;
						break;
					}
				}
				if(n == NbIntervChal[bolo]) chaleur = (float)c;
				if(bolo == touche) Chaleur[evt] = chaleur;
				if(chaleur > SeuilChal) NbTouches[evt] += 1.0;
			}
		}
	#endif
	}
	return(0);
}
/* ========================================================================== */
int CalculeQuenching() {
	int evt;

	if(VoiesNb < 2) {
		OpiumError("Ce calcul n'est pas applicable a ce detecteur");
		return(0);
	}
	if(OpiumExec(pQuenching->cdr) == PNL_CANCEL) return(0);
	printf("* Calcul du facteur de quenching\n");
	for(evt=0; evt<EvtNb; evt++) {
		Recul[evt] = (1.0 + (PolarIonisation / 3.0)) * Chaleur[evt]
			- (PolarIonisation / 3.0) * Ionisation[evt];
		if(Recul[evt] != 0.0) Quenching[evt] = Ionisation[evt] / Recul[evt];
	}
	return(0);
}
/* ========================================================================== */
void ProdChaleurImprime() {
	int i,n;

	if(VoiesNb < 2) {
		OpiumError("Cete fonction n'est pas applicable a ce detecteur");
		return;
	}
	if(Bolo[BoloNum].lu != DETEC_EN_SERVICE) {
		OpiumError("Le bolo '%s' n'est pas utilisable",Bolo[BoloNum].nom);
		return;
	}
	printf("- Correction chaleur pour %s:",Bolo[BoloNum].nom);
	if(NbIntervChal[BoloNum] == 0) printf(" neant");
	else for(n=0; n<NbIntervChal[BoloNum]; n++) {
		printf("\n  %2d/ [%5.1f %5.1f]: Chaleur =",n+1,CoefChal[BoloNum][n].inf,CoefChal[BoloNum][n].sup);
		for(i=0; i<CoefChal[BoloNum][n].nb; i++) {
			if(i > 0) printf(" +");
			printf(" %g",CoefChal[BoloNum][n].c[i]);
			if(i > 0) {
				printf(" x %s[chaleur]",NomMesure[MesurePourCalib]);
				if(i > 1) printf("%d",i);
			}
		}
	}
	printf("\n");
}
/* ========================================================================== */
int ProdChaleurSauve() {
	int i,n; int bolo;
	FILE *f;
	char nom_complet[MAXFILENAME];

	if(VoiesNb < 2) {
		OpiumError("Cete fonction n'est pas applicable a ce detecteur");
		return(0);
	}
	if(OpiumExec(pProdChaleurSauve->cdr) == PNL_CANCEL) return(0);
	RepertoireTermine(ProdCalibPath);
	strcat(strcpy(nom_complet,ProdCalibPath),ProdChaleurName);
	RepertoireCreeRacine(nom_complet);
	if(!(f = TangoFileOpen(nom_complet,"w"))) {
		OpiumError("%s:%d Fichier inutilisable (%s)",__FILE__, __LINE__, strerror(errno));
		return(0);
	}
	for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].lu == DETEC_EN_SERVICE) {
		fprintf(f,"#\n= %s",Bolo[bolo].nom);
		if(NbIntervChal[bolo] == 0) fprintf(f,": neant");
		else for(n=0; n<NbIntervChal[bolo]; n++) {
			fprintf(f,"\n %g > %g :",CoefChal[bolo][n].inf,CoefChal[bolo][n].sup);
			for(i=0; i<CoefChal[bolo][n].nb; i++) {
				if(i > 0) fprintf(f," +");
				fprintf(f," %g",CoefChal[bolo][n].c[i]);
			}
		}
		fprintf(f,"\n");
	}
	fclose(f);
	return(0);
}
/* ========================================================================== */
int ProdChaleurRelit() {
	FILE *f;
	char nom_complet[MAXFILENAME];
	char ligne[1024],*item,*c;
	int bolo; int i,n;

	if(VoiesNb < 2) {
		OpiumError("Cete fonction n'est pas applicable a ce detecteur");
		return(0);
	}
	if(OpiumExec(pProdChaleurSauve->cdr) == PNL_CANCEL) return(0);
	RepertoireTermine(ProdCalibPath);
	strcat(strcpy(nom_complet,ProdCalibPath),ProdChaleurName);
	if(!(f = TangoFileOpen(nom_complet,"r"))) {
		OpiumError("Fichier illisible (%s)",strerror(errno));
		return(0);
	}
	printf("- Calibration des voies chaleur d'apres le fichier:\n    %s\n",
		nom_complet);

	/* Relecture des coefficients */
	bolo = -1;
	do {
		if(!LigneSuivante(ligne,1024,f)) break;
		c = ligne;
		if((*c == '\n') || (*c == '\0') || (*c == '#')) continue;
		if(*c == '=') {
			c++;
			item = ItemSuivant(&c,':');
			for(bolo=0; bolo<BoloNb; bolo++) if(!strcmp(item,Bolo[bolo].nom)) break;
			if(bolo >= BoloNb) {
				OpiumError("Fichier chaleur: rencontre le bolo '%s' (inconnu)",item);
				continue;
			} else if(Bolo[bolo].lu != DETEC_EN_SERVICE) continue;
			NbIntervChal[bolo] = 0;
		} else {
			if(bolo < 0) {
				OpiumError("Fichier chaleur: coefficients sans bolo affecte");
				continue;
			}
			n = NbIntervChal[bolo];
			if(n >= MAXINTERVCHALEUR) {
				OpiumError("Deja %d intervalles definis pour %s! intervalle ignore",
					MAXINTERVCHALEUR,Bolo[bolo].nom);
				continue;
			}
			item = ItemSuivant(&c,'>');
			sscanf(item,"%g",&(CoefChal[bolo][n].inf));
			item = ItemSuivant(&c,':');
			sscanf(item,"%g",&(CoefChal[bolo][n].sup));
			i = 0;
			do {
				item = ItemSuivant(&c,'+');
				if(item[0] == '\0') break;
				sscanf(item,"%g",&(CoefChal[bolo][n].c[i]));
			} while(++i < MAXNONLIN);
			CoefChal[bolo][n].nb = i;
			NbIntervChal[bolo] = n + 1;
		}
	} while(1);
	fclose(f);

	/* Application des nouveaux coefficients */
	bolo = BoloNum;
	for(BoloNum=0; BoloNum<BoloNb; BoloNum++) ProdChaleurImprime();
	BoloNum = bolo;
	CalculeQuenching();
	PlotRefreshSiUniqueBiplot();

	return(0);
}
/* ========================================================================== */
int ProdChaleurAjuste() {
	int p; char cal[80],ref[80];
	int ix,iy; float rx,ry; float a,b,chal,ionis; float y;
	Info info; int action;
	int i,n; int ab6,ord,dim; void *adrs;
	float mini,maxi; int num;
	char encore;

	if(VoiesNb < 2) {
		OpiumError("Cete fonction n'est pas applicable a ce detecteur");
		return(0);
	}
	if((p = PlotChoixFig2D()) < 0) return(0);
	/* Si ProdCorrecChaleurX a ete ajoutee, l'autre aussi */
	n = GraphDimGet(Plot[p].g);
	for(i=0; i<n; i++) {
		adrs = GraphDataGet(Plot[PlotCourant].g,i,&dim);
		if(adrs == ProdCorrecChaleurX) break;
	}
	if(i >= n) {
		ab6 = GraphAdd(Plot[p].g,GRF_FLOAT|GRF_XAXIS,ProdCorrecChaleurX,MAXINTERVCHALEUR+1);
		ord = GraphAdd(Plot[p].g,GRF_FLOAT|GRF_YAXIS,ProdCorrecChaleurY,MAXINTERVCHALEUR+1);
		GraphDataRGB(Plot[p].g,ord,GRF_RGB_BLUE);
	} else {
		ab6 = i; ord = ab6 + 1;
		GraphDataUse(Plot[p].g,ab6,0);
		GraphDataUse(Plot[p].g,ord,0);
	}
	strcpy(ref,PlotVar[Plot[p].ab6].nom);
	strcpy(cal,PlotVar[Plot[p].ord].nom);
//	if(!(!strcmp(cal,"chaleur") && !strcmp(ref,"ionisation")) 
//	&& !(!strcmp(cal,"q") && !strcmp(ref,"recul"))) {
	if((strcmp(cal,"chaleur") || strcmp(ref,"ionisation")) 
	&& (strcmp(cal,"ionisation") || strcmp(ref,"chaleur")) 
	&& (strcmp(cal,"q") || strcmp(ref,"recul"))
	&& (strcmp(cal,"q") || strcmp(ref,"ionisation"))) {
		OpiumError("Les variables '%s' et '%s ne peuvent recalibrer la chaleur",ref,cal);
		return(0);
	}
	if(!strcmp(cal,"q")) {
		a = 1.0 + (PolarIonisation / 3.0); b = -PolarIonisation / 3.0;
		if(a == 0.0) {
			OpiumError("Chaleur indeterminee sur cette figure, car PolarIonisation=3V");
			return(0);
		}
	}
	if(OpiumReadList("Cette correction chaleur concerne le bolo",BoloNom,&BoloNum,DETEC_NOM) == PNL_CANCEL) return(0);
	if(Bolo[BoloNum].lu != DETEC_EN_SERVICE) {
		OpiumError("Ce bolo n'est pas utilisable");
		return(0);
	}

/*	Remise de la chaleur dans son etat non-corrige (un seul etage de corrections) */
	NbIntervChal[BoloNum] = 0;
	CalculeQuenching();

/*	Coupure (non memorisee) sur le numero de bolo touche */
	if((num = PlotIndexVar("bolo")) < 0) {
		OpiumError("Variable non definie: 'bolo'");
		return(0);
	}
	mini = (float)(BoloNum + 1) - 0.2;
	maxi = (float)(BoloNum + 1) + 0.2;
	/*	PlotNtupleCoupeOpere(PLOT_OPER_ET,num,mini,maxi,MARQUE_ABSENT); memorisation de la coupure a chaque appel... */
	EvtCoupeOpere(PLOT_OPER_ET,num,mini,maxi,MARQUE_ABSENT);  /* coupure non memorisee */
	printf(". %d evenement%s valide%s par %g < bolo < %g\n",Accord2s(PlotNtupleValides),mini,maxi);

	info = InfoCreate(2,133);
	n = 0; encore = 1;
	do {
		InfoWrite(info,1,"Pointes une valeur moyenne de %s,",cal);
		if(n == 0) InfoWrite(info,2,"pour %s proche de l'origine",ref);
		else InfoWrite(info,2,"pour %s > %d (ou au meme endroit pour terminer)",ref,(int)rx);
		OpiumDisplay(info->cdr);
		GraphGetPoint(Plot[p].g,&ix,&rx,&iy,&ry);
		OpiumClear(info->cdr);
		if(!strcmp(ref,"ionisation")) ionis = rx;
		else if(!strcmp(cal,"ionisation")) ionis = ry;
		else if(!strcmp(ref,"recul") && !strcmp(cal,"q")) ionis = rx * ry;
		else OpiumError("Tout compte fait, on ne sait pas calibrer la chaleur sur cette figure");
		if(!strcmp(cal,"chaleur")) chal = ry;
		else if(!strcmp(ref,"chaleur")) chal = rx;
		else if(!strcmp(ref,"recul") && !strcmp(cal,"q")) chal = rx * (1.0 - (b * ry)) / a;
		else if(!strcmp(ref,"ionisation") && !strcmp(cal,"q")) chal = rx * ((1.0 / ry) - b) / a;
		else OpiumError("Tout compte fait, on ne sait pas calibrer la chaleur sur cette figure");
		if(chal == 0.0) {
			OpiumError("Chaleur nulle, correction impossible");
			continue;
		}
		if(n == 0) {
			ProdCorrecChaleurX[0] = 0.0;
			if(!strcmp(cal,"chaleur")) ProdCorrecChaleurY[0] = 0.0;
			else if(!strcmp(cal,"q")) ProdCorrecChaleurY[0] = 1.0;
/* v1			ProdCorrecChaleurY[0] = ry - rx;
			CoefChal[BoloNum][n].c[0] = ionis - chal;
			CoefChal[BoloNum][n].c[1] = 1.0; */
			CoefChal[BoloNum][n].c[0] = 0.0;
			CoefChal[BoloNum][n].c[1] = ionis / chal;
			CoefChal[BoloNum][n].inf = 0.0;
		} else {
			if(ionis == CoefChal[BoloNum][n-1].sup) {
				break;
			} else if(ionis < CoefChal[BoloNum][n-1].sup) {
				action = OpiumExec(mProdChaleurAjuste->cdr);
				switch(action) {
				  case PROD_RECULE:
					--n;
					GraphDataUse(Plot[p].g,ab6,n+1);
					GraphDataUse(Plot[p].g,ord,n+1);
				  case PROD_CONTINUE:
					rx = ProdCorrecChaleurX[n];
					break;
				  case PROD_ANNULE:
					n = 0;
				  case PROD_STOPPE:
					encore = 0;
					break;
				}
				continue;
			}
			y = (ionis * ionis) / chal;
			CoefChal[BoloNum][n].c[1] = 
				(y - (CoefChal[BoloNum][n-1].c[1] * CoefChal[BoloNum][n-1].sup) - CoefChal[BoloNum][n-1].c[0])
				/ (ionis - CoefChal[BoloNum][n-1].sup);
			CoefChal[BoloNum][n].c[0] = y - (CoefChal[BoloNum][n].c[1] * ionis);
			CoefChal[BoloNum][n].inf = CoefChal[BoloNum][n-1].sup;
		}
		CoefChal[BoloNum][n].sup = ionis;
		n++;
		ProdCorrecChaleurX[n] = rx;
		ProdCorrecChaleurY[n] = ry;
		GraphDataUse(Plot[p].g,ab6,n+1);
		GraphDataUse(Plot[p].g,ord,n+1);
		/*		OpiumRefresh((Plot[p].g)->cdr); */
	} while(encore && (n < MAXINTERVCHALEUR));
	InfoDelete(info);
	NbIntervChal[BoloNum] = n;

	ProdChaleurImprime();
	CalculeQuenching();
	OpiumDisplay((Plot[p].g)->cdr);
	return(0);
}
/* ========================================================================== */
int ProdChaleurFitParabole(Panel panel) {
	float diff,*bins,*histo,*nb,*erreur,*fittee; char *valide;
	float largeur;
	char nom[80]; int voie;
	float mini,maxi;
	int bin,evt,rc,y;
	float a,b,c;
	char *p;
	Graph g;
//	int bin_test;

	if(VoiesNb < 2) {
		OpiumError("Cete fonction n'est pas applicable a ce detecteur");
		return(0);
	}
	if(OpiumExec(pProdChaleurFit->cdr) == PNL_CANCEL) return(0);
	if(Bolo[BoloNum].lu != DETEC_EN_SERVICE) {
		OpiumError("Ce bolo n'est pas utilisable");
		return(0);
	}

/* Creation et initialisation des differents tableaux */
	bins = (float *)malloc(ProdChaleurFitNb * sizeof(float));
	histo = (float *)malloc(ProdChaleurFitNb * sizeof(float));
	nb = (float *)malloc(ProdChaleurFitNb * sizeof(float));
	erreur = (float *)malloc(ProdChaleurFitNb * sizeof(float));
	fittee = (float *)malloc(ProdChaleurFitNb * sizeof(float));
	valide = (char *)malloc(ProdChaleurFitNb * sizeof(char));
	if(!valide) {
		OpiumError("Manque de place memoire pour %d bins",ProdChaleurFitNb);
		if(bins) free(bins);
		if(histo) free(histo);
		if(nb) free(nb);
		if(erreur) free(erreur);
		if(fittee) free(fittee);
		return(0);
	}
	largeur = (ProdChaleurFitMax - ProdChaleurFitMin) / (float)ProdChaleurFitNb;
	for(bin=0; bin<ProdChaleurFitNb; bin++) {
		histo[bin] = nb[bin] = erreur[bin] = 0.0;
		bins[bin] = ((float)bin + 0.5) * largeur;
		valide[bin] = 1;
	}

/* Coupure sur le numero de bolo touche */
	mini = (float)(BoloNum + 1) - 0.2;
	maxi = (float)(BoloNum + 1) + 0.2;
	if(!PlotNtupleCoupeSur(PLOT_OPER_NOUVEAU,"bolo",mini,maxi,1)) return(0);

/* Coupure sur le numero de voie ionisation touchee (obligatoirement centre si voie chaleur) */
	sprintf(nom,"voie-ionis");
	if(!PlotNtupleCoupeSur(PLOT_OPER_ET,nom,1.8,2.2,1)) return(0); /* evts centre uniquement */

/* Coupure des evts avec un bon fit */
	for(voie=0; voie<VoiesNb; voie++) if(VoieManip[voie].det == BoloNum) {
		sprintf(nom,"khi2-%s",VoieManip[voie].nom);
		p = nom; while(*p) { if(*p == ' ') *p = '-'; p++; };
		if(voie == 0) {
			if(!PlotNtupleCoupeSur(PLOT_OPER_ET,nom,0.0,ProdKhi2ChalMax,1)) return(0);
		} else {
			if(!PlotNtupleCoupeSur(PLOT_OPER_ET,nom,0.0,ProdKhi2IonisMax,1)) return(0);
		}
	}

/* Construction de l'histogramme */
//	bin_test = 7;
//	if(OpiumReadInt("Verification du bin",&bin_test) == PNL_CANCEL) return(0);
	for(evt=0; evt<EvtNb; evt++) if(EvtAdmis(evt)) {
		if(Chaleur[evt] < SeuilChal) continue;
		if(Ionisation[evt] < SeuilIonis) continue;
		bin = Chaleur[evt] / largeur;
		if((bin < 0) || (bin >= ProdChaleurFitNb)) continue;
		diff = Chaleur[evt] - Ionisation[evt];
		histo[bin] += diff;
		nb[bin] += 1.0;
		erreur[bin] += (diff * diff);
//		if((bin == bin_test) && (nb[bin] < 50.0)) {
//			printf("%6d: C=%12.3g (bin #%2d), I=%12.3g => C-I=%12.3g, H=%12.3g / %g\n",evt,
//				Chaleur[evt],bin,Ionisation[evt],diff,histo[bin],nb[bin]);
//		}
	}
	for(bin=0; bin<ProdChaleurFitNb; bin++) if(nb[bin]) {
		double erreur2;
		histo[bin] /= nb[bin];
		erreur2 = (erreur[bin] / nb[bin]) - (histo[bin] * histo[bin]);
		if(erreur2 < 0.0) {
			printf("*** Erreur2 negative dans le bin #%d! (%g)\n",bin,erreur2);
			erreur[bin] = 0.0;
			valide[bin] = 0;
		} else {
			erreur[bin] = sqrtf(erreur2);
			if(erreur[bin] < 0.0001) valide[bin] = 0;
		}
	}

/* Calcul de la fonction de correction */
	printf(".    bin          Chaleur       Ec-Ei        erreur\n");
	for(bin=0; bin<ProdChaleurFitNb; bin++) {
		printf("  %2d[%3g]: %12.3g %12.3g %12.3g %s\n",bin,nb[bin],bins[bin],histo[bin],erreur[bin],valide[bin]? "":"(invalide)");
	}

/* On montre comment on est beaux */
	if(FitSecondDegre(bins,histo,erreur,valide,ProdChaleurFitNb,&a,&b,&c)) {
		printf(". Fit avec C -= %g x C2    %+g x C    %+g\n",a,b,c);
		for(bin=0; bin<ProdChaleurFitNb; bin++)
			fittee[bin] = (((a * bins[bin]) + b) * bins[bin]) + c;
		g = GraphCreate(300,300,4);
		GraphAdd(g,GRF_FLOAT|GRF_XAXIS,bins,ProdChaleurFitNb);
		y = GraphAdd(g,GRF_FLOAT|GRF_YAXIS,fittee,ProdChaleurFitNb);
		GraphDataRGB(g,y,GRF_RGB_BLUE);
		y = GraphAdd(g,GRF_FLOAT|GRF_YAXIS,erreur,ProdChaleurFitNb);
		GraphDataRGB(g,y,GRF_RGB_RED);
		GraphDataTrace(g,y,GRF_ERR,0);
		y = GraphAdd(g,GRF_FLOAT|GRF_YAXIS,histo,ProdChaleurFitNb);
		GraphDataRGB(g,y,GRF_RGB_RED);
		GraphDataTrace(g,y,GRF_PNT,GRF_TRGLE);
		OpiumDisplay(g->cdr);
	} else OpiumError("Fit impossible (%s)",FitError);

/* Il est temps de conclure */
	if(OpiumCheck("Application de cette correction")) for(evt=0; evt<EvtNb; evt++) {
		Chaleur[evt] -= (((a * Chaleur[evt]) + b) * Chaleur[evt]) + c;
	}

	OpiumClear(g->cdr);
	GraphDelete(g);
	free(bins);
	free(histo);
	free(nb);
	free(erreur);
	free(valide);
	return(0);
}
/* ========================================================================== */
int ProdChaleurPanel() {
	int n,i;

	if(VoiesNb < 2) {
		OpiumError("Cete fonction n'est pas applicable a ce detecteur");
		return(0);
	}
	if(Bolo[BoloNum].lu != DETEC_EN_SERVICE) return(0);
	PanelErase(pCorrigeInterv);
	PanelColumns(pCorrigeInterv,NbIntervChal[BoloNum]);
#ifdef TROP_DETAILLE
	for(n=0; n<NbIntervChal[BoloNum]; n++) {
		PanelFloat(pCorrigeInterv,"Entre",&(CoefChal[BoloNum][n].inf));
		PanelFloat(pCorrigeInterv,"et",&(CoefChal[BoloNum][n].sup));
		for(i=0; i<CoefChal[BoloNum][n].nb; i++)
			PanelFloat(pCorrigeInterv,CorrigeIntervTxt[i],&(CoefChal[BoloNum][n].c[i]));
	}
#else
	for(n=0; n<NbIntervChal[BoloNum]; n++) {
		if(n == 0) {
			PanelFloat(pCorrigeInterv,"Entre",&(CoefChal[BoloNum][n].inf));
			PanelFloat(pCorrigeInterv,"et",&(CoefChal[BoloNum][n].sup));
			for(i=0; i<CoefChal[BoloNum][n].nb; i++)
				PanelFloat(pCorrigeInterv,CorrigeIntervTxt[i],&(CoefChal[BoloNum][n].c[i]));
		} else {
			PanelRemark(pCorrigeInterv,"et du precedent,");
			PanelFloat(pCorrigeInterv,"jusqu'a",&(CoefChal[BoloNum][n].sup));
			PanelRemark(pCorrigeInterv,"= constante");
			for(i=1; i<CoefChal[BoloNum][n].nb; i++)
				PanelFloat(pCorrigeInterv,CorrigeIntervTxt[i],&(CoefChal[BoloNum][n].c[i]));
		}
	}
#endif
	return(0);
}
/* ========================================================================== */
int ProdChaleurDynamic(Panel panel, int num) {
	if(VoiesNb < 2) {
		OpiumError("Cete fonction n'est pas applicable a ce detecteur");
		return(0);
	}
	//	OpiumClear(pCorrigeInterv->cdr);
	ProdChaleurPanel();
	OpiumDisplay(pCorrigeInterv->cdr);
	return(0);
}


/* ========================================================================== */
int ProdChaleurCorrige() {
#ifndef TROP_DETAILLE
	int i,n; float x,xn,y;
#endif

	if(VoiesNb < 2) {
		OpiumError("Cete fonction n'est pas applicable a ce detecteur");
		return(0);
	}
	if(OpiumReadList("Cette correction chaleur concerne le bolo",BoloNom,&BoloNum,DETEC_NOM) == PNL_CANCEL) return(0);
	if(Bolo[BoloNum].lu != DETEC_EN_SERVICE) {
		OpiumError("Ce bolo n'est pas utilisable");
		return(0);
	}
	if(OpiumReadInt("Nombre de morceaux",&(NbIntervChal[BoloNum])) == PNL_CANCEL) return(0);
	if(NbIntervChal[BoloNum] > 0) {
		ProdChaleurPanel();
		OpiumExec(pCorrigeInterv->cdr);
	#ifndef TROP_DETAILLE
		for(n=1; n<NbIntervChal[BoloNum]; n++) {
			CoefChal[BoloNum][n].inf = CoefChal[BoloNum][n-1].sup;
			x = CoefChal[BoloNum][n].inf;
			y = 0.0; xn = 1.0;
			for(i=0; i<CoefChal[BoloNum][n-1].nb; i++) {
				y += (CoefChal[BoloNum][n-1].c[i] * xn);
				xn *= x;
			}
			xn = x;
			for(i=1; i<CoefChal[BoloNum][n].nb; i++) {
				y -= (CoefChal[BoloNum][n].c[i] * xn);
				xn *= x;
			}
			CoefChal[BoloNum][n].c[0] = y;
		}
	#endif
	}
	ProdChaleurImprime();
	CalculeEnergies();
	CalculeQuenching();
	PlotRefreshSiUniqueBiplot();
	return(0);
}
#endif /* BOLOMETRES */
#ifdef CROSSTALK
/* ========================================================================== */
int ProdCrosstalk() {
	int bolo,evt; float centre;
	int voie,capt,vcent,vgard;

	if(VoiesNb < 2) {
		OpiumError("Cete fonction n'est pas applicable a ce detecteur");
		return(0);
	}
	if(OpiumExec(pProdCrosstalk->cdr) == PNL_CANCEL) return(0);
	if(VoiesNb < 2) ProdToutes = PERIM_UNIQUE;
	if(ProdToutes == PERIM_TOUTES) {
		for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].lu == DETEC_EN_SERVICE) {
			vcent = vgard = -1;
			for(capt=0; capt<Bolo[bolo].nbcapt; capt++) {
				voie = Bolo[bolo].captr[capt].voie;
				if(VoieManip[voie].type == VoieCentre) vcent = voie;
				else if(VoieManip[voie].type == VoieGarde) vgard = voie;
			}
			if((vcent < 0) || (vgard < 0)) continue;
			for(evt=0; evt<EvtNb; evt++) {
				centre = Energie[vcent][evt];
				Energie[vcent][evt] -= ProdCorrectionCT * Energie[vgard][evt];
				Energie[vgard][evt] -= ProdCorrectionCT * centre;
			}
		}
	} else {
		if(Bolo[BoloNum].lu != DETEC_EN_SERVICE) {
			OpiumError("Ce bolo n'est pas utilisable");
			return(0);
		}
		bolo = BoloNum;
		vcent = vgard = -1;
		for(capt=0; capt<Bolo[bolo].nbcapt; capt++) {
			voie = Bolo[bolo].captr[capt].voie;
			if(VoieManip[voie].type == VoieCentre) vcent = voie;
			else if(VoieManip[voie].type == VoieGarde) vgard = voie;
		}
		if((vcent < 0) || (vgard < 0)) return(0);
		for(evt=0; evt<EvtNb; evt++) {
			centre = Energie[vcent][evt];
			Energie[vcent][evt] -= ProdCorrectionCT * Energie[vgard][evt];
			Energie[vgard][evt] -= ProdCorrectionCT * centre;
		}
	}
	return(0);
}
#endif /* CROSSTALK */
/* ========================================================================== */
int ProdCalibChoixMesure() {
	if(OpiumReadListB("Mesures a utiliser",NomMesure,&MesurePourCalib,12) == PNL_CANCEL) return(0);
	printf("= Travail sur les %s\n",NomMesure[MesurePourCalib]);
	return(0);
}
/* ========================================================================== */
int ProdCalibApplique() {
	FILE *f;
	char nom_complet[MAXFILENAME];
	char ligne[1024],*item,*c;
	int voie,evt,bolo_avant,voie_avant; int k; float coef;
#ifdef BOLOMETRES
	int bolo,vcent,vgard,capt,vt,i;
	char bolo_calcule[DETEC_MAX];
#endif
	char ok;
	
	switch(MesurePourCalib) {
		case EDW_AMPL:
	#ifdef CALCULE_SURFACE
		case EDW_SURF:
	#endif
			if(OpiumExec(pProdCalibSauve->cdr) == PNL_CANCEL) return(0);
			RepertoireTermine(ProdCalibPath);
			strcat(strcpy(nom_complet,ProdCalibPath),ProdCalibName);
			if(!(f = TangoFileOpen(nom_complet,"r"))) {
				OpiumFail("Fichier %s illisible (%s)",ProdCalibName,strerror(errno));
				return(0);
			}
			printf("- Calibration des %s d'apres le fichier:\n    %s\n",
				   NomMesure[MesurePourCalib],nom_complet);
			/* On ramene les valeurs au coefficient 1 */
			for(voie=0; voie<VoiesNb; voie++) {
				coef = Coef[MesurePourCalib][voie];
				for(evt=0; evt<EvtNb; evt++) {
					switch(MesurePourCalib) {
						case EDW_AMPL:
							Bruit[voie][evt] /= coef;
							Amplitude[voie][evt] /= coef;
							Integrale[voie][evt] /= coef;
							break;
					#ifdef CALCULE_SURFACE
						case EDW_SURF:
							Surface[voie][evt] /= coef;
							break;
					#endif
					}
				}
			}
			
			/* Relecture des coefficients */
			ok = 0;
			do {
				if(!LigneSuivante(ligne,1024,f)) break;
				c = ligne;
				if((*c == '\n') || (*c == '\0') || (*c == '#')) continue;
				if(*c == '=') {
					if(ok) break;
					c++;
					item = ItemSuivant(&c,' ');
					ok = !strcmp(item,NomMesure[MesurePourCalib]);
					if(ok) for(voie=0; voie<VoiesNb; voie++) {
						Coef[MesurePourCalib][voie] = 1.0;
						Etalon[MesurePourCalib][voie] = 1.0;
					}
				} else if(!ok) continue;
				else {
					item = ItemSuivant(&c,'=');
					for(voie=0; voie<VoiesNb; voie++) if(!strcmp(item,VoieManip[voie].nom)) break;
					if( voie >= VoiesNb) {
						OpiumFail("Fichier des coefficients: rencontre la voie '%s' (inconnue)",item);
						continue;
					}
					sscanf(ItemSuivant(&c,'@'),"%g",&(Coef[MesurePourCalib][voie]));
					if(*c == '\0') Etalon[MesurePourCalib][voie] = 122.0;
					else sscanf(ItemSuivant(&c,' '),"%g",&(Etalon[MesurePourCalib][voie]));
				}
			} while(1);
			fclose(f);
			
			/* Application des nouveaux coefficients */
			for(voie=0; voie<VoiesNb; voie++) {
				coef = Coef[MesurePourCalib][voie];
				for(evt=0; evt<EvtNb; evt++) {
					switch(MesurePourCalib) {
						case EDW_AMPL:
							Bruit[voie][evt] *= coef;
							Amplitude[voie][evt] *= coef;
							Integrale[voie][evt] *= coef;
							break;
					#ifdef CALCULE_SURFACE
						case EDW_SURF:
							Surface[voie][evt] *= coef;
							break;
					#endif
					}
				}
				ProdCalibFaite[MesurePourCalib][voie] = 1;
			}
			break;
			
		case EDW_EVT:
			if(OpiumReadFile("Depuis le fichier",UniteEventFile,MAXFILENAME) == PNL_CANCEL) return(0);
			EvtUniteRelit(UniteEventFile);
			bolo_avant = BoloNum; voie_avant = VoieNum;
			// OpiumProgresTitle("Normalisation");
			// OpiumProgresInit(VoiesNb);
			k = 0;
			for(VoieNum=0; VoieNum<VoiesNb; VoieNum++) {
				// if(!OpiumProgresRefresh(k++)) { VoieNum = VoiesNb; break; }
				EvtUniteFitteEnergies(VoieManip[VoieNum].nom,1);
			}
			// OpiumProgresClose();
			if(ProdCrosstalkActif) {
			#ifdef BOLOMETRES
				float centre;
				for(evt=0; evt<EvtNb; evt++) {
					if(EvtLitASuivre(evt,0) <= 0) break;
					for(i=0; i<DETEC_MAX; i++) bolo_calcule[i] = 0;
					for(vt=0; vt<ArchEvt.nbvoies; vt++) {
						voie = ArchEvt.voie[vt].num;
						bolo = VoieManip[voie].det;
						if(bolo_calcule[i]) continue;
						vcent = vgard = -1;
						for(capt=0; capt<Bolo[bolo].nbcapt; capt++) {
							voie = Bolo[bolo].captr[capt].voie;
							if(VoieManip[voie].type == VoieCentre) vcent = voie;
							else if(VoieManip[voie].type == VoieGarde) vgard = voie;
						}
						if((vcent >= 0) && (vgard >= 0)) {
							centre = Energie[vcent][evt];
							Energie[vcent][evt] -= ProdCorrectionCT * Energie[vgard][evt];
							Energie[vgard][evt] -= ProdCorrectionCT * centre;
						}
						bolo_calcule[i] = 1;
					}
				}
			#endif
			}
			BoloNum = bolo_avant; VoieNum = voie_avant;
			break;
	}
	
	if(OpiumAlEcran(mNtpSauve)) MenuItemAllume(mNtpSauve,1,0,GRF_RGB_YELLOW);
	return(0);
}
/* ========================================================================== */
int ProdCalibSauve() {
	FILE *f;
	char nom_complet[MAXFILENAME];
	int voie,mode;
	
	switch(MesurePourCalib) {
		case EDW_AMPL:
	#ifdef CALCULE_SURFACE
		case EDW_SURF:
	#endif
			if(OpiumExec(pProdCalibSauve->cdr) == PNL_CANCEL) return(0);
			RepertoireTermine(ProdCalibPath);
			strcat(strcpy(nom_complet,ProdCalibPath),ProdCalibName);
			RepertoireCreeRacine(nom_complet);
			if(!(f = TangoFileOpen(nom_complet,"w"))) {
				OpiumFail("%s.%d: Fichier %s inutilisable (%s)",__FILE__, __LINE__,ProdCalibName,strerror(errno));
				return(0);
			}
			for(mode=EDW_AMPL; mode<EDW_EVT; mode++) {
				fprintf(f,"#\n=%s\n",NomMesure[mode]);
				for(voie=0; voie<VoiesNb; voie++) {
					if(ProdCalibFaite[mode][voie]) fprintf(f,"  %s = %g @%g\n",
														   VoieManip[voie].nom,Coef[mode][voie],Etalon[mode][voie]);
				}
			}
			fclose(f);
			printf("- Coefficients de calibration sauves sur:\n    %s\n",nom_complet);
			break;
			
		case EDW_EVT:
			if(OpiumReadFile("Sur le fichier",UniteEventFile,MAXFILENAME) == PNL_CANCEL) return(0);
			RepertoireCreeRacine(UniteEventFile);
			EvtUniteSauve(UniteEventFile);
			break;
	}
	return(0);
}
/* ========================================================================== */
int ProdCalibCalcule1Voie(char affiche) {
	int bolo,voie,evt; int var; int p,p2; int i,rep;
	char nomvar[80]; char nom[MAXEVTNOM];
	float mini,maxi; int bins;
	float hmin,hbin;
	float *adrs; float coef; float ref;
	int pos,bot,top;
	char *c;
	
	voie = VoieNum;  /* OBLIGATOIRE cause appels a ProdChannelUnite dans Calib* */
	bolo = VoieManip[voie].det;
	printf("*** Calcul des %s pour %s\n",NomMesure[MesurePourCalib],VoieManip[voie].nom);
	
	/* Coupure sur le numero de bolo touche */
	if(BoloNb > 1) {
		mini = (float)(bolo + 1) - 0.2E0;
		maxi = (float)(bolo + 1) + 0.2E0;
		if(!PlotNtupleCoupeSur(PLOT_OPER_NOUVEAU,"bolo",mini,maxi,1)) return(0);
	} else PlotNtupleCoupeSur(PLOT_OPER_TOUT,"\0",0.0,0.0,1);
	
#ifdef BOLOMETRES
	/* Coupure sur le numero de voie ionisation touchee (obligatoirement centre si voie chaleur) */
	if(VoiesNb > 1) {
		if((VoieManip[voie].type == VoieChaleur) || (VoieManip[voie].type == VoieCentre) || (VoieManip[voie].type == VoieGarde)) {
			sprintf(nomvar,"voie-ionis");
			if((VoieManip[voie].type == VoieCentre) || (VoieManip[voie].type == VoieGarde)) {  /* i.e. c'est une voie ionisation */
				mini = (float)(VoieManip[voie].type + 1) - 0.2;
				maxi = (float)(VoieManip[voie].type + 1) + 0.2;
			} else { mini = (float)VoieCentre + 0.8; maxi = (float)VoieCentre + 1.2; }  /* evts centre si chaleur etudiee */
			c = nomvar; while(*c) { if(*c == ' ') *c = '-'; c++; };
			if(!PlotNtupleCoupeSur(PLOT_OPER_ET,nomvar,mini,maxi,1)) return(0);
		}
	}
#endif
	
	/* Coupure sur le timing de l'evenement
	 sprintf(nomvar,"tevt-%s",VoieManip[voie].nom);
	 mini = (float)(VoieEvent[voie].avant_evt) * 0.75;
	 maxi = (float)(VoieEvent[voie].avant_evt) * 1.25;
	 c = nomvar; while(*c) { if(*c == ' ') *c = '-'; c++; };
	 if(!PlotNtupleCoupeSur(PLOT_OPER_ET,nomvar,mini,maxi,1)) return(0); */
	
	if(!PlotNtupleValides) {
		OpiumError("Aucun evenement ne passe les coupures");
		return(0);
	}
	
	/* Coupure sur le khi2 de l'evenement */
	if(MesurePourCalib == EDW_EVT) {
		sprintf(nomvar,"khi2-%s",VoieManip[voie].nom);
		//	mini = -1;
		//	maxi = 0.002;
		c = nomvar; while(*c) { if(*c == ' ') *c = '-'; c++; };
		//	if(!PlotNtupleCoupeSur(PLOT_OPER_ET,nomvar,mini,maxi,1)) return(0);
		if(VoieManip[voie].type == 0) {
			if(!PlotNtupleCoupeSur(PLOT_OPER_ET,nomvar,0.0,ProdKhi2ChalMax,1)) return(0);
		} else {
			if(!PlotNtupleCoupeSur(PLOT_OPER_ET,nomvar,0.0,ProdKhi2IonisMax,1)) return(0);
		} 
		if(!PlotNtupleValides) {
			OpiumError("Aucun evenement ne passe la coupure sur le khi2");
			return(0);
		}
	}
	
	/* Histogrammation de la variable a moyenner */
	if((p = PlotNouveau(PLOT_HISTO)) < 0) return(0);
	switch(MesurePourCalib) {
		case EDW_AMPL: case EDW_EVT: 
			sprintf(nomvar,"ampl-%s",VoieManip[voie].nom); break;
#ifdef CALCULE_SURFACE
		case EDW_SURF: sprintf(nomvar,"surf-%s",VoieManip[voie].nom); break;
#endif
	}
	c = nomvar; while(*c) { if(*c == ' ') *c = '-'; c++; };
	if((var = PlotIndexVar(nomvar)) < 0) {
		OpiumError("Variable non definie: %s",nomvar);
		return(0);
	}
	if(ProdCalibFaite[MesurePourCalib][voie]) {
		mini = Etalon[MesurePourCalib][voie] / 3.0E0;
		maxi = Etalon[MesurePourCalib][voie] * 1.5E0;
	} else {
#ifdef A_AMELIORER
		mini = 0.0; maxi = 0.0;
		for(evt=0; evt<EvtNb; evt++) {
			if(Amplitude[voie][evt] > maxi) maxi = Amplitude[voie][evt];
			if(Amplitude[voie][evt] < mini) mini = Amplitude[voie][evt];
		}
#else
		if(voie) ref = RefIonis; else ref = RefChal;
		if(ref > 0.0) { mini = ref / 3.0E0; maxi = ref * 1.5E0; }
		else { mini = ref * 1.5E0; maxi = ref / 3.0E0; }
#endif
	}
	mini=-10000;
	maxi=10000;
	
	printf(". %s dans [%g .. %g]\n",nomvar,mini,maxi);
	bins = 100;
	PlotRempliHisto(p,var,mini,maxi,bins);
	
	/* Identification du pic principal */
	OpiumDisplay(Plot[p].g->cdr);
	//	while(OpiumCheck("Retracer l'histogramme?")) PlotTrace(p,0);
	while((rep = OpiumChoix(3,ProdHistoBoutons,"Histogramme correct?")) == 1) PlotTrace(p,0);
	if(rep == 0) return(0);
	/* L'histo devrait avoir ete modifie pour cadrer au mieux le pic de reference */
	if(!HistoGetBinFloat(Plot[p].h,&hmin,&hbin,&bins)) {
		OpiumError("Histogramme %s vide",Plot[p].titre);
		return(0);
	}
	adrs = HistoGetDataFloat(Plot[p].hd);
	maxi = 0.0; pos = 0;
	for(i=0; i<bins; i++) if(adrs[i] > maxi) {
		maxi = adrs[i];
		pos = i;
	}
	printf(". maxi au point %d (%g) pour h=%g\n",pos,hmin + ((float)pos * hbin),maxi);
	
	/* Coupure sur la variable a moyenner (largeur a <ProdNiveauMin>hauteur) */
	//	ProdNiveauMin = 0.75;
	//	if(OpiumReadFloat("Niveau minimum",&ProdNiveauMin) == PNL_CANCEL) return(0);
	maxi *= ProdNiveauMin;
	for(bot=pos; bot>=0; --bot) if(adrs[bot] < maxi) break;
	for(top=pos; top<bins; top++) if(adrs[top] < maxi) break;
	if(top <= bot) top = bot + 1;
	maxi = hmin + ((float)top * hbin);
	mini = hmin + ((float)bot * hbin);
	//	printf(". intervalle choisi: [%d .. %d]\n",bot,top);
	
	switch(MesurePourCalib) {
		case EDW_AMPL:
#ifdef CALCULE_SURFACE
		case EDW_SURF:
#endif
			PlotStatsCalcule(p,mini,maxi);
			coef = EnergieRef / PlotStat.moyenne;
			printf(". moyenne de %s dans [%g .. %g]: %g => coef=%g\n",nomvar,mini,maxi,PlotStat.moyenne,coef);
			Coef[MesurePourCalib][voie] *= coef;
			Etalon[MesurePourCalib][voie] = EnergieRef;
			for(evt=0; evt<EvtNb; evt++) {
				switch(MesurePourCalib) {
					case EDW_AMPL:
						Bruit[voie][evt] *= coef;
						Amplitude[voie][evt] *= coef;
						Integrale[voie][evt] *= coef;
						break;
#ifdef CALCULE_SURFACE
					case EDW_SURF:
						Surface[voie][evt] *= coef;
						break;
#endif
				}
			}
			ProdCalibFaite[MesurePourCalib][voie] = 1;
			break;
		case EDW_EVT:
			PlotNtupleCoupeOpere(PLOT_OPER_ET,var,mini,maxi,GRF_ABSENT);
			printf(". %d evenements %sselectionnes par %g < %s < %g\n",PlotNtupleValides,ProdEvtManu?"pre-":"",mini,nomvar,maxi);
			/* Maintenant, on peut calculer un evenement unite propre et tout normaliser dessus */
			strncpy(nom,VoieManip[VoieNum].nom,MAXEVTNOM);
			if(OpiumReadText("Nom de l'evenement unite cree",nom,MAXEVTNOM) == PNL_CANCEL) return(0);
			EvtUniteCalcule(nom,EnergieRef,ProdEvtManu,affiche);
			EvtUniteFitteEnergies(nom,affiche);
			/* Histogrammation de la variable a moyenner */
			if((p2 = PlotNouveau(PLOT_HISTO)) < 0) return(0);
			sprintf(nomvar,"ener-%s",VoieManip[voie].nom);
			c = nomvar; while(*c) { if(*c == ' ') *c = '-'; c++; };
			if((var = PlotIndexVar(nomvar)) < 0) {
				OpiumError("Variable non definie: %s",nomvar);
				return(0);
			}
			mini = EnergieRef * 0.8E0;
			maxi = EnergieRef * 1.2E0;
			printf(". %s dans [%g .. %g]\n",nomvar,mini,maxi);
			bins = 50;
			PlotRempliHisto(p2,var,mini,maxi,bins);
			
			/* Identification du pic principal */
			OpiumDisplay(Plot[p2].g->cdr);
			// while(OpiumCheck("Retracer l'histogramme?")) PlotTrace(p2,0);
			/* L'histo devrait avoir ete modifie pour cadrer au mieux le pic de reference */
			if(!HistoGetBinFloat(Plot[p2].h,&hmin,&hbin,&bins)) {
				OpiumError("Histogramme %s vide",Plot[p2].titre);
				return(0);
			}
			adrs = HistoGetDataFloat(Plot[p2].hd);
			maxi = 0.0; pos = 0;
			for(i=0; i<bins; i++) if(adrs[i] > maxi) {
				maxi = adrs[i];
				pos = i;
			}
			printf(". maxi au point %d (%g) pour h=%g\n",pos,hmin + ((float)pos * hbin),maxi);
			/* Coupure sur la variable a moyenner (largeur a <ProdNiveauMin>hauteur) */
			// if(OpiumReadFloat("Niveau minimum",&ProdNiveauMin) == PNL_CANCEL) return(0);
			maxi *= ProdNiveauMin;
			for(bot=pos; bot>=0; --bot) if(adrs[bot] < maxi) break;
			for(top=pos; top<bins; top++) if(adrs[top] < maxi) break;
			if(top <= bot) top = bot + 1;
			maxi = hmin + ((float)top * hbin);
			mini = hmin + ((float)bot * hbin);
			//	printf(". intervalle choisi: [%d .. %d]\n",bot,top);
			
			PlotStatsCalcule(p2,mini,maxi);
			Actuelle =  PlotStat.moyenne;
			coef = EnergieRef / Actuelle;
			printf("... moyenne de %s dans [%g .. %g]: %g => coef=%g\n",nomvar,mini,maxi,PlotStat.moyenne,coef);
#ifdef RECALIBAUTO
		{
			float *evt_unite; int num,lngr,dep,arr;
			if((num = EvtUniteCherche(nom,&evt_unite,&lngr,&dep,&arr)) < 0) {
				OpiumError("L'evenement unite %s n'est pas calcule",nom);
				return(0);
			}
			for(i=0; i<lngr; i++) evt_unite[i] /= coef;
			EvtUniteFitteEnergies(nom,affiche);
		}
#endif
			ProdCalibFaite[MesurePourCalib][voie] = 1;
			PlotAnnule(p2);
			break;
	}
	PlotNtupleCoupeOpere(PLOT_OPER_TOUT,var,mini,maxi,GRF_POINT);
	printf(". %s normalisee\n",nomvar);
	PlotAnnule(p);
	
	return(0);
}
/* ========================================================================== */
int ProdCalibCorrige1Voie(char affiche) {
	int evt;
	float *evt_unite; int lngr,dep,arr;
	int i,num; float coef;
	char nom[MAXEVTNOM];
	
	Actuelle = PlotStat.moyenne; /* ResultatFit.gau.x0 */
	if(OpiumExec(pProdCalibVoie->cdr) == PNL_CANCEL) return(0);
	if(Actuelle == 0.0) {
		OpiumError("Valeur actuelle invalide");
		return(0);
	}
	coef = EnergieRef / Actuelle;
	switch(MesurePourCalib) {
		case EDW_AMPL:
#ifdef CALCULE_SURFACE
		case EDW_SURF:
#endif
			Coef[MesurePourCalib][VoieNum] *= coef;
			Etalon[MesurePourCalib][VoieNum] = EnergieRef;
			for(evt=0; evt<EvtNb; evt++) {
				switch(MesurePourCalib) {
					case EDW_AMPL:
						Bruit[VoieNum][evt] *= coef;
						Amplitude[VoieNum][evt] *= coef;
						Integrale[VoieNum][evt] *= coef;
						break;
#ifdef CALCULE_SURFACE
					case EDW_SURF:
						Surface[VoieNum][evt] *= coef;
						break;
#endif
				}
			}
			break;
		case EDW_EVT:
			strncpy(nom,VoieManip[VoieNum].nom,MAXEVTNOM);
			//- if(OpiumReadText("Nom de l'evenement unite cree",nom,MAXEVTNOM) == PNL_CANCEL) return(0);
			if((num = EvtUniteCherche(nom,&evt_unite,&lngr,&dep,&arr)) < 0) {
				OpiumWarn("L'evenement unite %s n'est pas calcule",nom);
				return(0);
			}
			for(i=0; i<lngr; i++) evt_unite[i] /= coef;
			EvtUniteFitteEnergies(nom,affiche);
			break;
	}
	printf("- Re-calibration %s de %s par un facteur %g\n",NomMesure[MesurePourCalib],
		   VoieManip[VoieNum].nom,coef);
	return(0);
}
/* ========================================================================== */
int ProdCalibCalcule() {
	int voie; int k;
	
	if(OpiumExec(pProdEvtCalib->cdr) == PNL_CANCEL) return(0);
	if(VoiesNb < 2) ProdToutes = PERIM_UNIQUE;
	if(ProdToutes == PERIM_TOUTES) {  /* toutes les voies pour tous les bolos */
		voie = VoieNum;
		k = 0;
		for(VoieNum=0; VoieNum<VoiesNb; VoieNum++) ProdCalibCalcule1Voie(1);
		VoieNum = voie;
	} else if(ProdToutes == PERIM_DETEC) {  /* toutes les voies pour le bolo selectionne */
		voie = VoieNum;
		for(VoieNum=0; VoieNum<VoiesNb; VoieNum++) if(VoieManip[VoieNum].det == BoloNum) ProdCalibCalcule1Voie(1);
		VoieNum = voie;
	} else {
		ProdCalibCalcule1Voie(1);
	}
	if(OpiumAlEcran(mNtpSauve)) MenuItemAllume(mNtpSauve,1,0,GRF_RGB_YELLOW);
	return(0);
}
/* ========================================================================== */
int ProdCalibCorrige() {
	int voie; int k;
	
	if(VoiesNb < 2) ProdToutes = PERIM_UNIQUE;
	else if(OpiumExec(pProdVoieUtilisee->cdr) == PNL_CANCEL) return(0);
	if(ProdToutes == PERIM_TOUTES) {  /* toutes les voies pour tous les bolos */
		voie = VoieNum;
		k = 0;
		for(VoieNum=0; VoieNum<VoiesNb; VoieNum++) ProdCalibCorrige1Voie(1);
		VoieNum = voie;
	} else if(ProdToutes == PERIM_DETEC) {  /* toutes les voies pour le bolo selectionne */
		voie = VoieNum;
		for(VoieNum=0; VoieNum<VoiesNb; VoieNum++) if(VoieManip[VoieNum].det == BoloNum) ProdCalibCorrige1Voie(1);
		VoieNum = voie;
	} else {
		ProdCalibCorrige1Voie(1);
	}
	if(OpiumAlEcran(mNtpSauve)) MenuItemAllume(mNtpSauve,1,0,GRF_RGB_YELLOW);
	return(0);
}
/* ========================================================================== */
int ProdCalibPanel(int bolo) {
	int voie;
	char texte[80];
	
	PanelErase(pProdCalibCoefs);
	PanelColumns(pProdCalibCoefs,2);
	for(voie=0; voie<VoiesNb; voie++) if(VoieManip[voie].det == bolo) 
		PanelFloat(pProdCalibCoefs,ProdCalibCoefTxt[voie],&(Coef[MesurePourCalib][voie]));
	for(voie=0; voie<VoiesNb; voie++) if(VoieManip[voie].det == bolo) 
		PanelFloat(pProdCalibCoefs,"pour E",&(Etalon[MesurePourCalib][voie]));
	sprintf(texte,"Calibration des %s de %s",NomMesure[MesurePourCalib],Bolo[bolo].nom);
	PanelTitle(pProdCalibCoefs,texte);
	return(0);
}
/* ========================================================================== */
int ProdCalibMontre() {
	if(OpiumDisplayed(pProdCalibCoefs->cdr)) OpiumClear(pProdCalibCoefs->cdr);
	if(BoloNb > 1) { if(OpiumReadInt("Pour quel detecteur",&BoloNum) == PNL_CANCEL) return(0); }
	ProdCalibPanel(BoloNum);	
	OpiumDisplay(pProdCalibCoefs->cdr);
	return(0);
}
/* ========================================================================== */
int ProdUniteChoixZone() {
	if(OpiumExec(pProdUniteZoneFit->cdr) == PNL_CANCEL) return(0);
	if(VoiesNb == 1) {
		EvtUniteMont = (float)TempsMontee[VoieManip[VoieNum].type] * VoieEvent[VoieNum].horloge;
		EvtUniteDesc = (float)TempsDescente[VoieManip[VoieNum].type] * VoieEvent[VoieNum].horloge;
	}
	return(0);
}
/* ========================================================================== */
int ProdUniteChoixVoie() {
	int capt,vm,voie,bolo;
	
	if(VoiesNb < 2) { OpiumWarn("Une seule voie, choix inutile"); return(0); }
	bolo = BoloNum; vm = ModlNum;
	if(OpiumExec(pProdUniteChoixVoie->cdr) == PNL_CANCEL) return(0);
	if(!Bolo[BoloNum].lu) {
		OpiumFail("Le detecteur %s n'est pas lu",Bolo[BoloNum].nom);
		BoloNum = bolo; ModlNum = vm;
		return(0);
	}
	for(capt=0; capt<Bolo[BoloNum].nbcapt; capt++) {
		voie = Bolo[BoloNum].captr[capt].voie;
		if(ModlNum == VoieManip[voie].type) break;
	}
	if(capt >= Bolo[BoloNum].nbcapt) {
		OpiumFail("Pas de voie '%s' pour le detecteur '%s'",ModeleVoie[ModlNum].nom,Bolo[BoloNum].nom);
		BoloNum = bolo; ModlNum = vm;
		return(0);
	}
	/*	if(!FinEvt[voie]) {
	 OpiumError("Pas de %s dans ce run (ou ntuple vide)",VoieManip[voie].nom);
	 BoloNum = bolo; ModlNum = vm;
	 return(0);
	 } */
	VoieNum = voie;
	printf("= Travail sur la voie %s\n",VoieManip[VoieNum].nom);
	return(0);
}
/* ========================================================================== */
int ProdUniteFitEvts() {
	int voie; int k;
	char nom[MAXEVTNOM];

	if(VoiesNb < 2) ProdToutes = PERIM_UNIQUE;
	else if(OpiumExec(pProdVoieUtilisee->cdr) == PNL_CANCEL) return(0);
	if(ProdToutes == PERIM_TOUTES) {
		if(Bolo[BoloNum].lu != DETEC_EN_SERVICE) {
			OpiumError("Ce detecteur n'est pas utilisable");
			return(0);
		}
		voie = VoieNum;
		OpiumProgresTitle("Normalisation");
		OpiumProgresInit(VoiesNb);
		k = 0;
		for(VoieNum=0; VoieNum<VoiesNb; VoieNum++) {
			if(!OpiumProgresRefresh(k++)) break;
			EvtUniteFitteEnergies(VoieManip[VoieNum].nom,0);
		}
		OpiumProgresClose();
		VoieNum = voie;
	} else if(ProdToutes == PERIM_DETEC) {
		if(Bolo[BoloNum].lu != DETEC_EN_SERVICE) {
			OpiumError("Ce detecteur n'est pas utilisable");
			return(0);
		}
		voie = VoieNum;
		OpiumProgresTitle("Normalisation");
		OpiumProgresInit(VoiesNb);
		k = 0;
		for(VoieNum=0; VoieNum<VoiesNb; VoieNum++) if(VoieManip[VoieNum].det == BoloNum) {
			if(!OpiumProgresRefresh(k++)) break;
			EvtUniteFitteEnergies(VoieManip[VoieNum].nom,0);
		}
		OpiumProgresClose();
		VoieNum = voie;
	} else {
		strncpy(nom,VoieManip[VoieNum].nom,MAXEVTNOM);
//-		if(OpiumReadText("Nom de l'evenement unite cree",nom,MAXEVTNOM) == PNL_CANCEL) return(0);
		EvtUniteFitteEnergies(nom,1);
	}
	return(0);
}
/* ========================================================================== */
static void ProdUniteSauve1Template(int voie) {
	EdwEvtInfo.arrivee = 0;
	if(EvtUniteCherche(VoieManip[voie].nom,
	&(EdwEvtInfo.val),&(EdwEvtInfo.taille),&(EdwEvtInfo.depart),&(EdwEvtInfo.arrivee)) >= 0) {
		strcpy(EdwEvtInfo.nom,VoieManip[voie].nom);
		EdwEvtInfo.horloge = VoieEvent[voie].horloge;
		EdwEvtInfo.amplitude = 1.0;
		strcpy(EdwEvtInfo.unite,"keV");
		EdwEvtInfo.pre_trigger = VoieEvent[voie].avant_evt;
		EdwDbPrint(DbazPath,VoieManip[voie].det,VoieManip[voie].type,&EdwEvt);
		printf("- Evenement unite %s archive\n",VoieManip[voie].nom);
	} else printf(". L'evenement unite %s, n'etant pas calcule, n'est pas archive\n",VoieManip[voie].nom);
}
/* ========================================================================== */
static int ProdUniteCreeTemplate() {
	int voie;
	
	if(VoiesNb < 2) ProdToutes = PERIM_UNIQUE;
	else if(OpiumReadKeyB("Pour",VOIES_PERIM_CLES,&ProdToutes,23) == PNL_CANCEL) return(0);
	if(ProdToutes == PERIM_TOUTES) {  /* toutes les voies pour tous les detecteurs */
		for(voie=0; voie<VoiesNb; voie++) ProdUniteSauve1Template(voie);
	} else if(ProdToutes == PERIM_DETEC) {  /* toutes les voies pour le bolo selectionne */
		for(voie=0; voie<VoiesNb; voie++) if(VoieManip[voie].det == BoloNum) ProdUniteSauve1Template(voie);
	} else ProdUniteSauve1Template(VoieNum);
	return(0);
}
/* ========================================================================== */
int ProdRunHdrAffiche() {
	if(OpiumDisplayed(pHdrGeneral->cdr)) OpiumClear(pHdrGeneral->cdr);
	else OpiumDisplay(pHdrGeneral->cdr);
	return(0);
}
/* ========================================================================== */
int ProdTrgrAffiche() {
	if(OpiumDisplayed(pTrigger->cdr)) OpiumClear(pTrigger->cdr);
	else OpiumDisplay(pTrigger->cdr);
	return(0);
}
/* ========================================================================== */
int ProdRunDirMontre() {
	char commande[MAXFILENAME];
	sprintf(commande,"open %s",FichierEvents); system(commande);
	return(0);
}
/* ========================================================================== */
int ProdBolosAffiche() {
	if(OpiumDisplayed(pArchBolos->cdr)) OpiumClear(pArchBolos->cdr);
	else OpiumDisplay(pArchBolos->cdr);
	return(0);
}
/* ========================================================================== */
int ProdDiagAffiche() {
	int i,n,p,y; void *adrs; float xmin,xmax;

	if((p = PlotChoixFig2D()) < 0) return(0);
	xmin = Plot[p].x.min; xmax = Plot[p].x.max;
	ZoneDiagX[0] = xmin; ZoneDiagX[1] = xmax;
	ZoneDiagY[0] = xmin; ZoneDiagY[1] = xmax;
	/* Si ZoneDiagX a ete ajoutee, l'autre aussi */
	n = GraphDimGet(Plot[p].g);
	for(i=0; i<n; i++) {
		adrs = GraphDataGet(Plot[PlotCourant].g,i,&y);
		if(adrs == ZoneDiagX) break;
	}
	if(i >= n) {
		GraphAdd(Plot[p].g,GRF_FLOAT|GRF_XAXIS,ZoneDiagX,2);
		y = GraphAdd(Plot[p].g,GRF_FLOAT|GRF_YAXIS,ZoneDiagY,2);
		GraphDataRGB(Plot[p].g,y,GRF_RGB_RED);
	}
	OpiumRefresh((Plot[p].g)->cdr);
	return(0);
}
/* ========================================================================== */
float ProdErreurSurQ(float x, float q) {
	float erreur_chal,erreur_ionis,dq_chal,dq_ionis;

	erreur_chal = ResolChalBase + ((ResolChalRef - ResolChalBase) * x / EnergieRef);
	erreur_ionis = ResolIonisBase + ((ResolIonisRef - ResolIonisBase) * x / EnergieRef);
	dq_chal = (1.0 + (PolarIonisation / 3.0)) * q * erreur_chal;
	dq_ionis = (1.0 + (PolarIonisation * q / 3.0)) * erreur_ionis;
	return(sqrtf((dq_chal * dq_chal) + (dq_ionis * dq_ionis)) / x);
}
/* ========================================================================== */
int ProdZonesAffiche() {
	int i,n,p,y; void *adrs; float xmin,xmax,dx,x;
	float luke_chal,luke_ionis;
	float seuil_chal,seuil_ionis;
	float ecart_90,q,dq;
/* Somme[0..X](exp(-x2/2 sigma2)dx)= 0.90 pour X=1.64485 sigma
                                     0.95        1.95996 sigma
                                     0.98        2.32635 sigma
   (voir fichier "ConfidenceLevel" dans G4MiG:Users:gros:Edelweiss:Analyse) */

	if((p = PlotChoixFig2D()) < 0) return(0);
	if(OpiumExec(pProdResolutions->cdr) == PNL_CANCEL) return(0);
	ecart_90 = 1.64485;
	xmax = Plot[p].x.max;
	ZoneQ[0] = Plot[p].x.min; ZoneQ[1] = xmax;
	ZoneQegale1[0] = ZoneQegale1[1] = 1.0;
	xmin = 1.0; dx = (xmax - xmin) / (float)(ZONE_NBPOINTS - 1);
	ZoneAb6[0] = xmin; ZoneAb6[1] = dx;
	luke_chal = 1.0 + (PolarIonisation / 3.0);
	luke_ionis = PolarIonisation / 3.0;
	for(i=0, x=xmin; i<ZONE_NBPOINTS; i++, x+= dx) {
		seuil_chal = ((luke_chal * SeuilChal) - x) / (x * luke_ionis);
		seuil_ionis = SeuilIonis / x;
		ZoneResol[i] = (seuil_chal > seuil_ionis)? seuil_chal: seuil_ionis;
		q = 1.0;
		dq = ProdErreurSurQ(x,q);
		ZoneGammasMin[i] = q - (ecart_90 * dq);
		ZoneGammasMax[i] = q + (ecart_90 * dq);
		q = NeutronsFact * expf(NeutronsExp * logf(x));
		dq = ProdErreurSurQ(x,q);
		ZoneNeutronsMil[i] = q;
		ZoneNeutronsMin[i] = q - (ecart_90 * dq);
		ZoneNeutronsMax[i] = q + (ecart_90 * dq);
	}

	/* Si ZoneAb6 a ete ajoutee, toutes les autres aussi */
	n = GraphDimGet(Plot[p].g);
	for(i=0; i<n; i++) {
		adrs = GraphDataGet(Plot[PlotCourant].g,i,&y);
		if(adrs == ZoneAb6) break;
	}
	if(i >= n) {
		GraphAdd(Plot[p].g,GRF_FLOAT|GRF_XAXIS,ZoneQ,2);
		y = GraphAdd(Plot[p].g,GRF_FLOAT|GRF_YAXIS,ZoneQegale1,2);
		GraphDataRGB(Plot[p].g,y,GRF_RGB_MAGENTA);
		GraphAdd(Plot[p].g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,ZoneAb6,ZONE_NBPOINTS);
		y = GraphAdd(Plot[p].g,GRF_FLOAT|GRF_YAXIS,ZoneResol,ZONE_NBPOINTS);
		GraphDataRGB(Plot[p].g,y,0x0000,0xFFFF,0xFFFF);
		y = GraphAdd(Plot[p].g,GRF_FLOAT|GRF_YAXIS,ZoneGammasMin,ZONE_NBPOINTS);
		GraphDataRGB(Plot[p].g,y,GRF_RGB_MAGENTA);
		y = GraphAdd(Plot[p].g,GRF_FLOAT|GRF_YAXIS,ZoneGammasMax,ZONE_NBPOINTS);
		GraphDataRGB(Plot[p].g,y,GRF_RGB_MAGENTA);
		y = GraphAdd(Plot[p].g,GRF_FLOAT|GRF_YAXIS,ZoneNeutronsMil,ZONE_NBPOINTS);
		GraphDataRGB(Plot[p].g,y,0xFFFF,0xBFFF,0);
		y = GraphAdd(Plot[p].g,GRF_FLOAT|GRF_YAXIS,ZoneNeutronsMin,ZONE_NBPOINTS);
		GraphDataRGB(Plot[p].g,y,0xFFFF,0xBFFF,0);
		y = GraphAdd(Plot[p].g,GRF_FLOAT|GRF_YAXIS,ZoneNeutronsMax,ZONE_NBPOINTS);
		GraphDataRGB(Plot[p].g,y,0xFFFF,0xBFFF,0);
	}
	OpiumRefresh((Plot[p].g)->cdr);
	return(0);
}
/* ========================================================================== */
int ProdBoloCible() {
	int evt; int bolo; char vu;

	if(BoloNb > 1) {
		if(OpiumReadList("Detecteur touche",BoloNom,&BoloNum,DETEC_NOM) == PNL_CANCEL) return(0);
	}
	bolo = BoloNum;
	vu = 0;
	evt = EvtDemande;
	OpiumProgresInit(EvtNb);
	do {
		if(EvtLitASuivre(++evt,1) > 0) {
			if(ArchEvt.bolo_hdr == bolo) { vu = 1; break; }
		} else {
			OpiumWarn("Pas d'autre evenement sur %s (numero maxi: %d)",Bolo[bolo].nom,EvtNb);
			break;
		}
		if(!OpiumProgresRefresh(evt)) break;
	} while(1);
	OpiumProgresClose();
	if(vu) ArchEvtMontre(evt);

	return(0);
}
/* ========================================================================== */
int ProdMemeBolo() {
	int evt; int bolo; char vu;

	bolo = ArchEvt.bolo_hdr;
	vu = 0;
	evt = EvtDemande;
	OpiumProgresInit(EvtNb);
	do {
		if(EvtLitASuivre(++evt,1) > 0) {
			if(ArchEvt.bolo_hdr == bolo) {
				if(fabsf(ArchEvt.voie[ArchEvt.voie_evt].val[MONIT_AMPL]) > ProdAmplMin) {
					vu = 1; break; 
				}
			}
		} else {
			OpiumWarn("Pas d'autre evenement sur %s (numero maxi: %d)",Bolo[bolo].nom,EvtNb);
			break;
		}
		if(!OpiumProgresRefresh(evt)) break;
	} while(1);
	OpiumProgresClose();
	if(vu) ArchEvtMontre(evt); 

	return(0);
}
/* ========================================================================== */
int ProdEvtDump1voie(int vt) {
	int voie,bolo,dim,t;
	TypeSignee *adrs,val;
	char texte[80];

	voie = ArchEvt.voie[vt].num;
	bolo = VoieManip[voie].det;
	texte[0] = '\0';
#ifdef BOLOMETRES
	if((ProdPatternActif == 1) && (Bolo[bolo].lu != BN_AUXILIAIRE) && ((VoieManip[voie].type == VoieCentre) || (VoieManip[voie].type == VoieGarde)))
		strcat(texte,"depatterne ");
#endif
	if((ProdCrosstalkActif == 1)  && (Bolo[bolo].lu != BN_AUXILIAIRE)) strcat(texte,"sans diaphonie ");
	if(ProdFiltreActif && (Bolo[bolo].lu != BN_AUXILIAIRE)) strcat(texte,"refiltre ");
	if(texte[0] == '\0') strcpy(texte,"brut");
	printf("\n===== Evt %d %s %s ===================================\n",EvtDemande,VoieManip[voie].nom,texte);
	dim = ArchEvt.voie[vt].dim;
	adrs = (TypeSignee *)VoieEvent[voie].brutes.t.sval;
	for(t=0; t<dim; t++,adrs++) {
		val = *adrs;
		if(!(t%10)) {
			if(t) printf("\n");
			printf("%4d:",t);
		}
		if(ProdFormatHexa) printf(" %04X",(int)val & 0xFFFF); else printf(" %6d",val);
	}
	printf("\n");

	return(0);
}
/* ========================================================================== */
int ProdEvtDump() {
	int evt; int vt,nb;

	evt = EvtDemande;
	if(OpiumExec(pEvtDump->cdr) == PNL_CANCEL) return(0);
	if(EvtLitUnSeul(EvtDemande,0)) {
		if(ProdToutes == PERIM_TOUTES) {
			for(vt=0; vt<ArchEvt.nbvoies; vt++) {
				if(vt) {
					printf("Fin de la voie %s. ",VoieManip[ArchEvt.voie[vt-1].num].nom);
					WndStep(0);
				}
				ProdEvtDump1voie(vt);
			}
		} else if(ProdToutes == PERIM_DETEC) {
			if((BoloNum < 0) || (BoloNum >= BoloNb)) {
				OpiumError("Le detecteur a ete mal (ou pas) choisie: son index vaut %d ...",BoloNum);
				return(0);
			}
			nb = 0;
			for(vt=0; vt<ArchEvt.nbvoies; vt++) if(VoieManip[ArchEvt.voie[vt].num].det == BoloNum) { ProdEvtDump1voie(vt); nb++; }
			if(!nb) {
				OpiumError("Pas de detecteur %s dans l'evenement #%d",Bolo[BoloNum].nom,EvtDemande);
				return(0);
			}
		} else {
			if((VoieNum < 0) || (VoieNum >= VoiesNb)) {
				OpiumError("La voie a ete mal (ou pas) choisie: son index vaut %d ...",VoieNum);
				return(0);
			}
			for(vt=0; vt<ArchEvt.nbvoies; vt++) if(ArchEvt.voie[vt].num == VoieNum) break;
			if(vt >= ArchEvt.nbvoies) {
				OpiumError("Pas de voie %s dans l'evenement #%d",VoieManip[VoieNum].nom,EvtDemande);
				return(0);
			}
			ProdEvtDump1voie(vt);
		}
	} else {
		OpiumError("Evenement #%d inutilisable (numero maxi: %d)",EvtDemande,EvtNb);
		EvtDemande = evt;
	}
	return(0);
}
/* ========================================================================== */
int ProductionTotale() { EvtNb = EvtLus; EvtsAnalyses = 0; ProdAmplitudes(); return(0); }
/* ========================================================================== */

TypeMenuItem iProd[] = {
	{ "Calculer",           MNU_FONCTION   ProductionTotale },
	{ "Relire",             MNU_FONCTION   EvtNtupleRelit },
	{ "Connaisseurs",       MNU_MENU     &mProdExperte },
	{ "Quitter",            MNU_RETOUR },
	MNU_END
};
TypeMenuItem iProdExperte[] = {
	//+	{ "Rattraper SAMBA",    MNU_FONCTION   ProdComplete },
	{ "Afficher",            MNU_FONCTION   EvtNtupleAffiche },
#ifdef ETALONNE
	{ "Etalonner",           MNU_FONCTION   Etalonne },
#endif
	{ "Selection variables", MNU_PANEL    &pPlotVarSel },
	{ "Options",             MNU_PANEL    &pEvtNtupleOptions },
	//-	{ "Sauver",             MNU_FONCTION   EvtNtupleEcrit },
	{ "Profondeur",          MNU_FONCTION   EvtNtupleProfond },
	{ "Quitter",             MNU_RETOUR },
	MNU_END
};

Menu mCalcul;
TypeMenuItem iCalcul[] = {
	"Valeurs SAMBA",        MNU_FONCTION   ProdSamba,
	"Amplitudes",           MNU_FONCTION   ProdAmplitudes,
#ifdef CALCULE_SURFACE
	"Surfaces",             MNU_FONCTION   ProdSurfaces,
#endif
	"Quitter",              MNU_RETOUR,
	MNU_END
};

int ProdBoloCible();
int ProdMemeBolo();
int ProdEvtDump();
TypeMenuItem iProdAffEvts[] = {
	//	"Recherche standard",        MNU_MENU     &mEvts,
	"Detecteurs enregistres",    MNU_FONCTION   ProdBolosAffiche,
	"Recherche detecteur",       MNU_FONCTION   ProdBoloCible,
	"Meme detecteur, plus tard", MNU_FONCTION   ProdMemeBolo,
	".. avec amplitude minimum", MNU_PANEL    &pAfficheAmplMin,
	MNU_END
};

#ifdef BOLOMETRES
Menu mProdAnalyse,mProdChaleur;
TypeMenuItem iProdAnalyse[] = {
	"Correction chaleur",   MNU_MENU     &mProdChaleur,
	"Quenching",            MNU_FONCTION   CalculeQuenching,
	"Energies",             MNU_FONCTION   CalculeEnergies,
	"Quitter",              MNU_RETOUR,
	MNU_END
};
TypeMenuItem iProdChaleur[] = {
	"Analytique",           MNU_FONCTION   ProdChaleurCorrige,
	"Graphique",            MNU_FONCTION   ProdChaleurAjuste,
	"Fit parabole",         MNU_FONCTION   ProdChaleurFitParabole,
	"Sauver",               MNU_FONCTION   ProdChaleurSauve,
	"Recuperer",            MNU_FONCTION   ProdChaleurRelit,
	"Quitter",              MNU_RETOUR,
	MNU_END
};
TypeMenuItem iProdChaleurAjuste[] = {
	"Segment negatif",              MNU_SEPARATION,
	"Fin de la saisie",             MNU_CONSTANTE PROD_STOPPE,
	"Ou bien on annule..",          MNU_SEPARATION,
	"..ce segment",                 MNU_CONSTANTE PROD_CONTINUE,
	"..le precedent segment",       MNU_CONSTANTE PROD_RECULE,
	"..toute la procedure",         MNU_CONSTANTE PROD_ANNULE,
	MNU_END
};
#endif

Menu mProd,mProdSupplement,mProdCalcul,mProdCalib,mProdMoyen;
TypeMenuItem iProdDialogue[] = {
	"Ntuple",               MNU_MENU     &mProd,
#ifdef NTP_PROGRES_INTEGRE
	"Calculs",              MNU_MENU     &mProdCalcul,
#endif
	"Affichages",          MNU_MENU     &mProdSupplement,
#ifdef CROSSTALK
	"Correction crosstalk", MNU_FONCTION   ProdCrosstalk,
#endif
#ifdef BOLOMETRES
	"Analyse Bolometres",   MNU_MENU     &mProdAnalyse,
#endif
	"Evenement moyen",      MNU_MENU     &mProdMoyen,
	"Calibration",          MNU_MENU     &mProdCalib,
	"Quitter",              MNU_RETOUR,
	MNU_END
};
int ProdVarCree(Menu menu, MenuItem item);
int ProdVarModifie(Menu menu, MenuItem item);
int ProdVarSupprime(Menu menu, MenuItem item);
int ProdVarListe(Menu menu, MenuItem item);
int ProdVarLit(Menu menu, MenuItem item);
int ProdVarEcrit(Menu menu, MenuItem item);
Menu mProdVar;
TypeMenuItem iProdVar[] = {
    "Creer",                MNU_FONCTION   ProdVarCree,
    "Modifier",             MNU_FONCTION   ProdVarModifie,
    "Supprimer",            MNU_FONCTION   ProdVarSupprime,
    "Lister",               MNU_FONCTION   ProdVarListe,
    "Sauver",               MNU_FONCTION   ProdVarEcrit,
    "Relire",               MNU_FONCTION   ProdVarLit,
    "Quitter",              MNU_RETOUR,
    MNU_END
};
TypeMenuItem iNtpCalcul[] = {
#ifdef PANEL_MULTI_V1
	"Choix temps mesures",   MNU_PANEL    &pProdChoixTemps,
#else
	"Temps pour mesures",    MNU_FORK     &bProdChoixTemps,
#endif
	"Detection des paquets", MNU_FONCTION   ProdPaquetsTrouve,
	"Recalcul des delais",   MNU_FONCTION   ProdAjusteDelais,
//	"Choix temps mesures",   MNU_PANEL   &pProdTempsArg,
	"Traitements",       	 MNU_PANEL    &pProdTraitements,
    "Variables",         	 MNU_MENU     &mProdVar,
	MNU_END
};
TypeMenuItem iProdCalcul[] = {
	"Choix temps mesures",  MNU_PANEL    &pProdChoixTemps,
	"Traitements",       	MNU_PANEL    &pProdTraitements,
//	"Choix Filtres",		MNU_PANEL    &pProdChoixFiltres,
	"Quitter",              MNU_RETOUR,
	MNU_END
};
// int TangoQualiteGraph();
TypeMenuItem iProdAffGeneral[] = {
	"Entete generale",           MNU_FONCTION   ProdRunHdrAffiche,
	"Trigger",                   MNU_FONCTION   ProdTrgrAffiche,
	"Modes evenement",           MNU_PANEL    &pProdEvtHdrAffichage,
	"Dumper evenement",          MNU_FONCTION   ProdEvtDump,
	"Repertoire du run",         MNU_FONCTION   ProdRunDirMontre,
//	"Detecteurs affiches",       MNU_PANEL    &pAfficheBolo,
//	"Voies affichees",           MNU_PANEL    &pAfficheVoie,
	"Dimensions graphiques",     MNU_PANEL    &pAfficheDim,
//	"Qualite des graphiques",    MNU_FONCTION   TangoQualiteGraph,
	MNU_END
};

TypeMenuItem iProdSupplement[] = {
	"Sur les biplots",           MNU_SEPARATION,
	"Zones plot Q",              MNU_FONCTION   ProdZonesAffiche,
	"Diagonale",                 MNU_FONCTION   ProdDiagAffiche,
	"Quitter",                   MNU_RETOUR,
	MNU_END
};

TypeMenuItem iProdCalib[] = {
	"Choix mesure calib",   MNU_FONCTION   ProdCalibChoixMesure,
	"Appliquer fichier",    MNU_FONCTION   ProdCalibApplique,
	"Calculer (auto)",      MNU_FONCTION   ProdCalibCalcule,
	"Modifier (manu)",      MNU_FONCTION   ProdCalibCorrige,
	"Afficher coefs",       MNU_FONCTION   ProdCalibMontre,
	"Sauver coefs",         MNU_FONCTION   ProdCalibSauve,
	"Sauver ntuples",       MNU_FONCTION   ProdSauveNtuples,
#ifdef BOLOMETRES
	"Energies",             MNU_FONCTION   CalculeEnergies,
#endif
	"Quitter",              MNU_RETOUR,
	MNU_END
};
TypeMenuItem iProdMoyen[] = {
	"Zones de fit",         MNU_FONCTION   ProdUniteChoixZone,
	"Choix d'une voie",     MNU_FONCTION   ProdUniteChoixVoie,
	"Fit evenements",       MNU_FONCTION   ProdUniteFitEvts,
	"Creer template SAMBA", MNU_FONCTION   ProdUniteCreeTemplate,
	"Quitter",              MNU_RETOUR,
	MNU_END
};

/* ========================================================================== */
//int ProductionComplement() {
//	//-	if(ModeScript) ProdAmplitudes(); else OpiumExec(mCalcul->cdr);
//	ProdAmplitudes();
//	return(0);
//}
/* ========================================================================== */
static char defini_1_voie(int *num, char *nom, char *unite, int voie, float **adrs, float min, float max) {
	char titre[80]; char *c;

	if(*num >= MAX_VARS_NTUPLE) {
		printf("! Limitations du nombre total de ntuples a MAX_VARS_NTUPLE=%d. Ntuples suivants pas crees\n",MAX_VARS_NTUPLE);
		return(0);
	}
	if(ProdNtpNb[voie] >= MAX_NTP_VOIE) {
		printf("! Limitations du nombre de ntuples par voie a MAX_NTP_VOIE=%d. Ntuples suivants pas crees\n",MAX_NTP_VOIE);
		return(0);
	}
	if(VoiesNb > 1) sprintf(titre,"%s-%s",nom,VoieManip[voie].nom);
	else sprintf(titre,"%s",nom);
	if(BoloNb == 1) { c = titre; while(*c) { if(*c == ' ') { *c = '\0'; break; } c++; } }
	else { c = titre; while(*c) { if(*c == ' ') *c = '-'; c++; }; }
	if(!PlotVarCree(&(Ntuple[*num]),titre,unite,&(adrs[voie]),EvtEspace)) return(0);
	PlotVarMinMax(&(Ntuple[*num]),min,max);
    // printf("Ntuple[%d]: %s @%08X\n",*num,titre,(hexa)Ntuple[*num].adrs);
	ProdNtpNum[voie][ProdNtpNb[voie]] = *num;
	//+ printf("+ NtpVoie[%s][%d] = %d avec Ntuple[%d] = %s\n",VoieManip[voie].nom,ProdNtpNb[voie],ProdNtpNum[voie][ProdNtpNb[voie]],*num,Ntuple[*num].nom);
	ProdNtpNb[voie] += 1;
	PlotVarSelecte(&(Ntuple[*num]),0);
	*num += 1;
	return(1);
}
/* ==========================================================================
static void defini_voie(int *num, int *tot, char *nom, char *unite, float *var[] ) {
	int voie; float **adrs;

	for(voie=0; voie<VoiesNb; voie++) {
		adrs = &(var[voie]);
		defini_1_voie(num,nom,unite,voie,adrs);
		*num += 1; *tot += 1;
	}
}
   ========================================================================== */
static char defini_bolo(int *num, char *nom, int bolo, float **adrs) {
	char titre[80];

	if(*num >= MAX_VARS_NTUPLE) return(0);
	sprintf(titre,"%s-%s",nom,Bolo[bolo].nom);
	if(!PlotVarCree(&(Ntuple[*num]),titre,"",adrs,EvtEspace)) return(0);
	// printf("Ntuple[%d]: %s @%08X\n",*num,titre,(hexa)Ntuple[*num].adrs);
	*num += 1;
	return(1);
}
/* ========================================================================== */
static char defini_glob(int *num, char *nom, char *unite, float **adrs) {
	if(*num >= MAX_VARS_NTUPLE) return(0);
	if(!PlotVarCree(&(Ntuple[*num]),nom,unite,adrs,EvtEspace)) return(0);
	// printf("Ntuple[%d]: %s @%08X\n",*num,nom,(hexa)Ntuple[*num].adrs);
	*num += 1;
	return(1);
}
/* ========================================================================== */
static char defini_paquet(int *num, char *nom, char *unite, float **adrs) {
	if(*num >= MAX_VARS_NTUPLE) return(0);
	if(!PlotVarCree(&(Ntuple[*num]),nom,unite,adrs,PaqEspace)) return(0);
	// printf("Ntuple[%d]: %s @%08X\n",*num,nom,(hexa)Ntuple[*num].adrs);
	*num += 1;
	return(1);
}
/* ========================================================================== */
typedef struct {
	char *nom;
	char *unite;
	float **adrs;
	float min,max;
} TypeNtupleVoieDef;
static TypeNtupleVoieDef NtupleVoieDef[] = {
	{ "montee",       "ms",     Montee,    0.0, 0.2   },
	{ "amplitude",    "ADU",    Amplitude, 0.0, 10000 },
	{ "integrale",    "",       Integrale, 0.0, 1000  },
	{ "rc",           "ms",     RCmesure,  0.0, 0.500 },
	{ "charges",      "ADU",    Depot,     0.0, 0.2   },
	{ "longueur",     "ms",     Longueur,  0.0, 10    },
	{ "descente",     "ms",     Descente,  0.0, 0.5   },
	{ "duree",        "ms",     Duree,     0.0, 0.5   },
	{ "pente",        "us/ADU", Pente,     0.0, 1.0   },
	{ "retard",       "ms",     Retard,    0.0, 2.0   },
	{ "base",         "ADU",    Base,      -1000.0, 1000.0 },
	{ "bruit",        "ADU",    Bruit,     0.0, 50.0  },
	{ "categorie",    "",       Categ,     0.0, 5.0  },
#ifdef COMPARE_NTUPLES
	{ "samba_base",   "ADU",    BaseSamba,      -1000.0, 1000.0 },
	{ "samba_bruit",  "ADU",    BruitSamba,     0.0, 50.0  },
	{ "samba_ampl",   "ADU",    AmplitudeSamba, 0.0, 10000 },
	{ "samba_montee", "ms",     MonteeSamba,    0.0, 0.2   },
#endif
	{ "ener",         "keV",    Energie, 0.0, 10.0    },
	{ "khi2",         "",       Khi2,    0.0, 1000.0  },
#ifdef CALCULE_SURFACE
	{ "surf",         "ADU",    Surface, 0.0, 10000.0 }, /* Integrale sur les deux premieres oscillations */
#endif
#ifdef BOLOMETRES
	{ "tevt",         "stamp",  Tevt,    0.0, 2000.0  }, /* position du maximum dans la fenetre de l'evenement */
#endif
#ifdef ETUDIE_CALAGE
	{ "dtmax",        "stamp",  DTmax,   0.0, 2000.0  },
	{ "dtthe0",       "stamp",  DTtheo,  0.0, 2000.0  },
#endif
	{ 0,              0,        0,       0.0, 1.0 }
};
/* ========================================================================== */
static int ProdVarNtupleVoie(char *nom, char *unite, float **adrs, float min, float max, int *nbntp, int appels) {
	int voie;

	for(voie=0; voie<VoiesNb; voie++) {
		if(!defini_1_voie(nbntp,L_(nom),unite,voie,adrs,min,max)) break;
		appels += 1;
	}
//	printf("- %d %s%s cree%s, total %d/%d Ntuple%s\n",voie,nom,AccordAvec(voie),AccordAvec(voie),*nbntp,Accord1s(appels));
	return(appels);
}
/* ========================================================================== */
static char ProdVarNtupleStandard() {
	int i,j,voie; char dbg;
	TypeNtupleVoieDef *ntuple;

	dbg = 0;

	/* Definition du ntuple */
	for(voie=0; voie<VoiesNb; voie++) ProdNtpNb[voie] = 0;
	i = 0; j = 0;
	defini_glob(&i,L_("numero"),"evt",&NumEvt); j++;
	defini_glob(&i,L_("date"),"s",&EvtDateMesuree); j++;
	// PlotVarMinMax(&(Ntuple[i-1]),0.0,(float)RunTempsTotal);
	defini_glob(&i,L_("delai"),"s",&EvtDelaiMesure); j++;
	if(VoiesNb > 1) { defini_glob(&i,"ecartT0","ms",&EcartT0); j++; EvtPaquetNum = EvtDecalage = 0; }
	else {
		defini_glob(&i,L_("paquet"),"",&EvtPaquetNum); j++;
		defini_glob(&i,L_("decalage"),"us",&EvtDecalage); j++;
	}
	ntuple = NtupleVoieDef;
	while(ntuple->nom) {
		j = ProdVarNtupleVoie(ntuple->nom,ntuple->unite,ntuple->adrs,ntuple->min,ntuple->max,&i,j);
		ntuple++;
	}
	defini_glob(&i,L_("samba_delai"),"s",&DelaiSamba); j++;
	if(VoiesNb > 1) {
		defini_glob(&i,"nbtouches","",&NbTouches); j++;
		defini_glob(&i,"bolo","det",&BoloTouche); j++;
	#ifdef BOLOMETRES
		for(bolo=0; bolo<BoloNb; bolo++) /* if(Bolo[bolo].lu != DETEC_ARRETE) */ {
			if(!defini_bolo(&i,"voie-ion",bolo,&(Ionisation[bolo]))) break;
			j++;
		}
		if(ModeleVoieNb > 1) {
			defini_glob(&i,"voie-ionis","",&VoieIonisee); j++;
			defini_glob(&i,"chaleur","ADU",&Chaleur); j++;
			defini_glob(&i,"ionisation","ADU",&Ionisation); j++;
			defini_glob(&i,"recul","",&Recul); j++;
			defini_glob(&i,"q","",&Quenching); j++;
		}
		//- defini_glob(&i,"crosstalk","%",&ctEvt); j++;
	#endif
	}
#ifdef CONDITIONS_MESURE
	/* A generaliser */
	defini_glob(&i,"temperature","K",&TemperLue); j++;
#endif

#ifdef A_AJUSTER
	for(bolo=0; bolo<BoloNb; bolo++)  if(Bolo[bolo].lu != DETEC_ARRETE) {
		if(!defini_bolo(&i,"correlation",bolo,&(crosstalk[bolo]))) break;
		j++;
	}
#endif

	defini_paquet(&i,L_("amp_premier"),"ADU",&PaqAmpPremier); j++;
	defini_paquet(&i,L_("amp_totale"),"ADU",&PaqAmpTotale); j++;
	defini_paquet(&i,L_("nb_evts"),"evts",&PaqMultiplicite); j++;
	defini_paquet(&i,L_("etendue"),"us",&PaqEtendue); j++;

	if(i < j) {
		PlotPrint(OPIUM_ERROR,"Toutes les variables n'ont pas ete creees (%d/%d)",i,j);
		return(0);
	} else if(i >= MAX_VARS_NTUPLE) {
		PlotPrint(OPIUM_ERROR,"Toutes les variables n'ont pas ete creees (maxi: %d)",MAX_VARS_NTUPLE-1);
		return(0);
	} else PlotPrint(OPIUM_NOTE,"%d variable%s creee%s",Accord2s(i));
	Ntuple[i].nom = "\0";
	NtTotNb = NtStdNb = i;

	return(1);
}
/* ========================================================================== */
static int ProdVarAjouteDansNtuple(int var) {
//	int voie;
	int bolo;
    
    printf("* Variable utilisateur #%d: %s = %s %c %s\n",var+1,
        ProdVar[var].nom,PlotVarList[ProdVar[var].var1],
        Opers[2*ProdVar[var].oper],PlotVarList[ProdVar[var].var2]);
    if(ProdVar[var].niveau == PRODVAR_VOIE) /* niveau voies */ {
        if(ProdVarVoieNb >= AJOUTSVOIE_MAX)
            PlotPrint(OPIUM_ERROR,"Seulement %d variables de niveau 'voie' permises",AJOUTSVOIE_MAX);
        else {
//            for(voie=0; voie<VoiesNb; voie++) {
//				if(!defini_1_voie(&NtTotNb,ProdVar[var].nom,"",voie,&(VarVoie[ProdVarVoieNb][voie]))) break;
//            }
			ProdVarNtupleVoie(ProdVar[var].nom,"",VarVoie[ProdVarVoieNb],0.0,1.0,&NtTotNb,NtTotNb);
            ProdVar[var].a_calculer = 1;
            ProdVar[var].num = ProdVarVoieNb++;
        }
    } else /* PRODVAR_DETEC: niveau detecteur */ {
        if(ProdVarDetecNb >= AJOUTSDETEC_MAX)
            PlotPrint(OPIUM_ERROR,"Seulement %d variables de niveau 'detecteur' permises",AJOUTSDETEC_MAX);
        else {
            for(bolo=0; bolo<BoloNb; bolo++) /* if(Bolo[bolo].lu != DETEC_ARRETE) */ {
                if(!defini_bolo(&NtTotNb,ProdVar[var].nom,bolo,&(VarDetec[ProdVarDetecNb][bolo]))) break;
            }
            ProdVar[var].a_calculer = 1;
            ProdVar[var].num = ProdVarDetecNb++;
        }
    }
    return(1);
}
/* ========================================================================== */
static void ProdVarRestoreNtuple() {
    int var;
    
    PlotNtupleSupprime();
    while(NtTotNb > NtStdNb) {
        NtTotNb--; if(Ntuple[NtTotNb].nom) free(Ntuple[NtTotNb].nom);
    }
    for(var=0; var<ProdVarNb; var++) ProdVarAjouteDansNtuple(var);
    Ntuple[NtTotNb].nom = "\0";
    if(!PlotVarsDeclare(Ntuple)) {
        PlotPrint(OPIUM_ERROR,"Manque de memoire pour la declaration des variables");
        return;
    }
    PlotNtupleCree(EvtNb,0.0);
    printf(". Ntuple recree (%d variables)\n",NtTotNb);
	// PlotNtupleCreeDesc();
}
/* ========================================================================== */
int ProdVarCree(Menu menu, MenuItem item) {
    if(ProdVarNb >= PRODVAR_MAX) {
        PlotPrint(OPIUM_ERROR,"Pas plus de %d variables supplementaires",PRODVAR_MAX);
        return(0);
    }
    if(OpiumExec(pProdVarDef->cdr) == PNL_CANCEL) return(0);
    memcpy(&(ProdVar[ProdVarNb]),&ProdVarAjoutee,sizeof(TypeProdVar));
    ProdVarNb++;
    ProdVar[ProdVarNb].nom[0] = '\0';
    ProdVarRestoreNtuple();
    ProdVarsModifiees = 1;
    if(OpiumCheck("Recalculer le Ntuple")) ProductionTotale();
    return(0);
}
/* ========================================================================== */
int ProdVarModifie(Menu menu, MenuItem item) {
    if(!ProdVarNb) {
        OpiumWarn("Pas de variable utilisateur a modifier");
        return(0);
    }
    if(ProdVarNb > 1) {
        if(OpiumExec(pProdVarQui->cdr) == PNL_CANCEL) return(0);
    } else ProdVarNum = 0;
    memcpy(&ProdVarAjoutee,&(ProdVar[ProdVarNum]),sizeof(TypeProdVar));
    if(OpiumExec(pProdVarMod->cdr) == PNL_CANCEL) return(0);
    memcpy(&(ProdVar[ProdVarNum]),&ProdVarAjoutee,sizeof(TypeProdVar));
    ProdVarsModifiees = 1;
    if(OpiumCheck("Recalculer le Ntuple")) ProductionTotale();
    return(0);
}
/* ========================================================================== */
int ProdVarSupprime(Menu menu, MenuItem item) {
    int var;
    
    if(!ProdVarNb) {
        OpiumWarn("Pas de variable utilisateur a supprimer");
        return(0);
    }
    if(OpiumExec(pProdVarQui->cdr) == PNL_CANCEL) return(0);
    --ProdVarNb;
    for(var=ProdVarNum; var<ProdVarNb; var++) {
        memcpy(&(ProdVar[var]),&(ProdVar[var+1]),sizeof(TypeProdVar));
    }
    ProdVar[ProdVarNb].nom[0] = '\0';
    ProdVarsModifiees = 1;
    if(OpiumCheck("Recalculer le Ntuple")) ProductionTotale();
    
    return(0);
}
/* ========================================================================== */
int ProdVarListe(Menu menu, MenuItem item) {
    int var;
    
    if(!ProdVarNb) { OpiumWarn("Pas de variable utilisateur a lister"); return(0); }
    if(!OpiumDisplayed(tProdVar->cdr)) OpiumDisplay(tProdVar->cdr);
    TermEmpty(tProdVar);
    TermHold(tProdVar);
    for(var=0; var<ProdVarNb; var++) TermPrint(tProdVar,"%s[%s] = %s %c %s\n",
        ProdVar[var].nom,ProdVar[var].niveau? "voie":"detec",
        PlotVar[ProdVar[var].var1].nom,Opers[2*ProdVar[var].oper],PlotVar[ProdVar[var].var2].nom);
    TermPrint(tProdVar,"(%d variable%s)\n",Accord1s(ProdVarNb));
    TermRelease(tProdVar);
    OpiumRefresh(tProdVar->cdr);
    
    return(0);
}
#define MAXLIGNE 80
/* ========================================================================== */
int ProdVarLit(Menu menu, MenuItem item) {
    int var; int i;
    FILE *f; char ligne[MAXLIGNE],*r,*symbole; char delim; char ok;
    
    if((f = fopen(ProdVarsFile,"r"))) {
        ok = 1;
        var = 0;
        while(LigneSuivante(ligne,MAXLIGNE,f)) {
            r = ligne; if(*r == '#') continue;
            strncpy(ProdVarAjoutee.nom,ItemAvant("-=",&delim,&r),PRODVAR_NOM); ProdVarAjoutee.nom[PRODVAR_NOM-1] = '\0';
            if(var < PRODVAR_MAX) {
                strcpy(ProdVar[var].nom,ProdVarAjoutee.nom);
                ProdVar[var].niveau = PRODVAR_DETEC;
                if(delim == '-') {
                    symbole = ItemJusquA('=',&r);
                    if(!strcmp(symbole,"voie")) ProdVar[var].niveau = PRODVAR_VOIE;
                }
                symbole = ItemJusquA(' ',&r);
                if((ProdVar[var].var1 = PlotIndexVar(symbole)) < 0) {
                    printf("! Operande inconnu: %s. Nouvelle variable %s ignoree\n",symbole,ProdVar[var].nom);
                    ok = 0;
                }
                symbole = ItemJusquA(' ',&r);
                if(symbole[1] != '\0') {
                    printf("! Operateur invalide: %s. Nouvelle variable %s ignoree\n",symbole,ProdVar[var].nom);
                    ok = 0;
                }
                ProdVar[var].oper = 0;
                for(i=0; i<4; i++) if(*symbole == Opers[2*i]) { ProdVar[var].oper = i; break; }
                symbole = ItemJusquA(' ',&r);
                if((ProdVar[var].var2 = PlotIndexVar(symbole)) < 0) {
                    printf("! Operande inconnu: %s. Nouvelle variable %s ignoree\n",symbole,ProdVar[var].nom);
                    ok = 0;
                }
                if(ok) var++;
            }
            if(var >= PRODVAR_MAX) { printf("! Variable en excedent: %s\n",ProdVarAjoutee.nom); ok = 0; }
        };
        fclose(f);
        ProdVarNb = var;
        ProdVar[ProdVarNb].nom[0] = '\0';
        if(!ok) PlotPrint(OPIUM_ERROR,"Definition des variables en defaut (voir log)");
    }
    return(0);
}
/* ========================================================================== */
int ProdVarEcrit(Menu menu, MenuItem item) {
    int var;
    FILE *f;
    
    if(!ProdVarNb) {
        OpiumWarn("Pas de variable utilisateur a sauver");
        return(0);
    }
    if((f = fopen(ProdVarsFile,"w"))) {
        for(var=0; var<ProdVarNb; var++) {
            fprintf(f,"%s-%s = %s %c %s\n",ProdVar[var].nom,ProdVar[var].niveau? "voie":"detec",
                    PlotVar[ProdVar[var].var1].nom,Opers[2*ProdVar[var].oper],PlotVar[ProdVar[var].var2].nom);
        }
        fclose(f);
        printf("Le fichier %s a ete mis a jour\n",ProdVarsFile);
    } else OpiumFail("Le fichier %s n'a pas pu etre ecrit",ProdVarsFile);
    ProdVarsModifiees = 0;
    return(0);
}
/* ========================================================================== */
void ProdVarDump(char *fctn,int n) {
	int i;

	printf("(%s) Etape #%2d:\n",fctn,n);
	for(i=0; i<NtTotNb; i++) printf("Ntuple[%d] @%08X: %s\n",i+1,(hexa)Ntuple[i].adrs,Ntuple[i].nom);
	printf("(%s) PlotVar @%08X\n",fctn,(hexa)PlotVar);
}
/* ========================================================================== */
void ProdCreeUI() {
	// Panel p;
	int i,n; int bolo,voie,vm; char Amin_vu,Amax_vu,Tmin_vu,Tmax_vu;
	// int x,y,xm,ym;
	int cols;

	PlotVarsInterface();

	mProd = MenuLocalise(iProd);
	mProdExperte = MenuLocalise(iProdExperte);
	mCalcul = MenuLocalise(iCalcul);
	mProdDialogue = MenuLocalise(iProdDialogue);
	mProdSupplement = MenuLocalise(iProdSupplement);
	mProdCalcul = MenuLocalise(iProdCalcul);
	mProdCalib = MenuLocalise(iProdCalib);
	mProdVar = MenuLocalise(iProdVar);
#ifdef BOLOMETRES
	mProdAnalyse = MenuLocalise(iProdAnalyse);
	mProdChaleur = MenuLocalise(iProdChaleur);
	mProdChaleurAjuste = MenuLocalise(iProdChaleurAjuste);
#endif
	mProdMoyen = MenuLocalise(iProdMoyen);

	pEvtNtupleOptions = PanelCreate(7);
	PanelFile(pEvtNtupleOptions,"Fichier",FichierNtuple,MAXFILENAME);
	PanelKeyB(pEvtNtupleOptions,"Variables a sauver","toutes/selection actuelle",&SelecteVar,20);
	// PanelList(pEvtNtupleOptions,"Espace",PlotEspacesList,&EspaceNum,MAXNOMESPACE);
	PanelKeyB(pEvtNtupleOptions,"Coupure sur l'espace","aucune/actuelle",&SelecteEvt,10);
	PanelKeyB(pEvtNtupleOptions,"Separateur",TANGO_SEP_CLES,&Separateur,8);
	PanelKeyB(pEvtNtupleOptions,"Ecriture Ntuple","creation/ajout",&AjouteNtuple,10);
	PanelKeyB(pEvtNtupleOptions,"Lecture Ntuple","remplacement/extension",&AjouteNtuple,15);

	pHdrGeneral = PanelDesc(ArchHdrGeneral,1);
	OpiumPosition(pHdrGeneral->cdr,50,50);

	cols = 5;
	Amin_vu = ArgVarLue(ArchHdrVoieDef,"Voie.trigger.plancher"); if(Amin_vu) cols++;
	Amax_vu = ArgVarLue(ArchHdrVoieDef,"Voie.trigger.plafond");  if(Amax_vu) cols++;
	Tmin_vu = ArgVarLue(ArchHdrVoieDef,"Voie.trigger.mintime");  if(Tmin_vu) cols++;
	Tmax_vu = ArgVarLue(ArchHdrVoieDef,"Voie.trigger.deadline"); if(Tmax_vu) cols++;

	pTrigger = PanelCreate((1 + VoiesNb) * cols);
	PanelColumns(pTrigger,cols); PanelMode(pTrigger,PNL_RDONLY|PNL_BYLINES);
	//- OpiumPosition(pTrigger->cdr,50,50);
	PanelItemSouligne(pTrigger,PanelItemSelect(pTrigger,PanelText(pTrigger,"voie","type",8),0),1);
	PanelItemSouligne(pTrigger,PanelItemSelect(pTrigger,PanelText(pTrigger,0,"sens",8),0),1);
	PanelItemSouligne(pTrigger,PanelItemSelect(pTrigger,PanelText(pTrigger,0,"min",6),0),1);
	PanelItemSouligne(pTrigger,PanelItemSelect(pTrigger,PanelText(pTrigger,0,"max",6),0),1);
	PanelItemSouligne(pTrigger,PanelItemSelect(pTrigger,PanelText(pTrigger,0,"temps",6),0),1);
	if(Amin_vu) PanelItemSouligne(pTrigger,PanelItemSelect(pTrigger,PanelText(pTrigger,0,"Amin",6),0),1);
	if(Amax_vu) PanelItemSouligne(pTrigger,PanelItemSelect(pTrigger,PanelText(pTrigger,0,"Amax",6),0),1);
	if(Tmin_vu) PanelItemSouligne(pTrigger,PanelItemSelect(pTrigger,PanelText(pTrigger,0,"Tmin",6),0),1);
	if(Tmax_vu) PanelItemSouligne(pTrigger,PanelItemSelect(pTrigger,PanelText(pTrigger,0,"Tmax",6),0),1);
	for(voie=0; voie<VoiesNb; voie++) {
		i = PanelKeyB(pTrigger,VoieManip[voie].nom,TrgrTypeCles,&(VoieManip[voie].def.trgr.type),8);
		i = PanelKeyB(pTrigger,0,TRGR_SENS_CLES,&(VoieManip[voie].def.trgr.sens),8);
		if(VoieManip[voie].def.trgr.sens > 0) i = PanelFloat(pTrigger,0,&(VoieManip[voie].def.trgr.minampl)); else i = PanelBlank(pTrigger);
		PanelLngr(pTrigger,i,6);
		if(VoieManip[voie].def.trgr.sens < 2) i = PanelFloat(pTrigger,0,&(VoieManip[voie].def.trgr.maxampl)); else i = PanelBlank(pTrigger);
		PanelLngr(pTrigger,i,6);
		i = PanelFloat(pTrigger,0,&(VoieManip[voie].def.trgr.montee)); PanelLngr(pTrigger,i,6);
		if(Amin_vu) { i = PanelFloat(pTrigger,0,&(VoieManip[voie].def.trgr.condition.underflow)); PanelLngr(pTrigger,i,6); }
		if(Amax_vu) { i = PanelFloat(pTrigger,0,&(VoieManip[voie].def.trgr.condition.overflow));  PanelLngr(pTrigger,i,6); }
		if(Tmin_vu) { i = PanelFloat(pTrigger,0,&(VoieManip[voie].def.trgr.condition.montmin));   PanelLngr(pTrigger,i,6); }
		if(Tmax_vu) { i = PanelFloat(pTrigger,0,&(VoieManip[voie].def.trgr.condition.montmax));   PanelLngr(pTrigger,i,6); }
	}
	
#ifdef CLASSES_D_EVENEMENTS
	TypeConfigDefinition *config; int k,nb; char encore;
	cols = 3;
	nb = 0;
	for(voie=0; voie<VoiesNb; voie++) if(VoieManip[voie].def.nbcatg) {
		nb++;
		config = &(VoieManip[voie].def);
		for(k=0; k<config->nbcatg; k++) nb += (1 + config->catg[k].definition.nbvars);
	}
	if(nb) {
		pTrigger = PanelCreate(nb * cols);
		PanelColumns(pTrigger,cols); PanelMode(pTrigger,PNL_RDONLY|PNL_BYLINES);
		for(voie=0; voie<VoiesNb; voie++) if(VoieManip[voie].def.nbcatg) {
			PanelText(pTrigger,L_("Voie"),VoieManip[voie].nom,VOIE_NOM); PanelBlank(pTrigger); PanelBlank(pTrigger);
			config = &(VoieManip[voie].def);
			for(k=0; k<config->nbcatg; k++) {
				PanelText(pTrigger,L_("Classe"),config->catg[k].nom,CATG_NOM); PanelBlank(pTrigger); PanelBlank(pTrigger);
				int num; for(num=0; num<config->catg[k].definition.nbvars ; num++) {
					char titre[80];
					encore = ((voie < (VoiesNb - 1)) && (num == (config->catg[k].definition.nbvars - 1)));
					snprintf(titre,80,L_("%s: si >="),VarName[config->catg[k].definition.var[num].ntuple]);
					i = PanelFloat(pTrigger,titre,&(config->catg[k].definition.var[num].min)); if(encore) PanelItemSouligne(pTrigger,i,1);
					i = PanelFloat(pTrigger,L_("et si <="),&(config->catg[k].definition.var[num].max)); if(encore) PanelItemSouligne(pTrigger,i,1);
					i = PanelKeyB(pTrigger,":",L_("accepte/exclue"),&(config->catg[k].definition.var[num].inverse),8); if(encore) PanelItemSouligne(pTrigger,i,1);
					PanelItemColors(pTrigger,i,TangoVertRouge,2);
				}
			}
		}
	} else {
		pTrigger = PanelCreate(1);
		PanelColumns(pTrigger,cols); PanelMode(pTrigger,PNL_RDONLY|PNL_BYLINES);
		PanelText(pTrigger,0,L_("Le trigger n'a pas ete recupere"),32); PanelBlank(pTrigger); PanelBlank(pTrigger);
	}
#endif

	pArchBolos = PanelCreate(BoloNb);
	for(bolo=0; bolo<BoloNb; bolo++) {
		i = PanelKeyB(pArchBolos,Bolo[bolo].nom,"absent/lu/faux",&(Bolo[bolo].lu),6);
		PanelItemColors(pArchBolos,i,TangoRougeVertJaune,3);
	}
	PanelTitle(pArchBolos,"Liste des detecteurs");

	/* pAfficheBolo = PanelCreate(BoloNb + 2);
	PanelKeyB(pAfficheBolo,"Mode","detecteur trigge seul/liste ci-dessous",&AfficheBoloDelaListe,22);
	PanelSepare(pAfficheBolo,"Liste a utiliser");
	for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].lu != DETEC_ARRETE) {
		i = PanelBool(pAfficheBolo,Bolo[bolo].nom,&(AfficheBolo[bolo]));
		PanelItemColors(pAfficheBolo,i,TangoRougeVertJaune,2);
		AfficheBolo[bolo] = 1; // (Bolo[bolo].lu == DETEC_EN_SERVICE);
	} else AfficheBolo[bolo] = 0;
	AfficheBoloDelaListe = 1;
	// OpiumPosition(pAfficheBolo->cdr,150,50);

	pAfficheVoie = PanelCreate(DETEC_CAPT_MAX);
	for(vm=0; vm<ModeleVoieNb; vm++) {  // necessite SambaAlloc donc ArchRunEdwx
		i = PanelBool(pAfficheVoie,ModeleVoie[vm].nom,&(AfficheVoie[vm]));
		PanelItemColors(pAfficheVoie,i,TangoRougeVertJaune,2);
		AfficheVoie[vm] = 1;
	} */
	// OpiumPosition(pAfficheVoie->cdr,150,50);

	pAfficheDim = PanelCreate(4);
	PanelInt(pAfficheDim,"Position X",&ArchGraphPosX);
	PanelInt(pAfficheDim,"Position Y",&ArchGraphPosY);
	PanelInt(pAfficheDim,"Largeur",&ArchGraphLarg);
	PanelInt(pAfficheDim,"Hauteur",&ArchGraphHaut);
	// OpiumPosition(pAfficheDim->cdr,150,50);

	pProdVoieUtilisee = PanelCreate(1);
	PanelKeyB(pProdVoieUtilisee,"Pour",VOIES_PERIM_CLES,&ProdToutes,23);

	pProdEvtHdrAffichage = PanelCreate(8 + ModeleVoieNb + BoloNb + 2);
	PanelBool(pProdEvtHdrAffichage,"Entete evenement",&AfficheHdrEvt);
	PanelBool(pProdEvtHdrAffichage,"Entete voies",&AfficheVoies);
	PanelBool(pProdEvtHdrAffichage,"Ntuple",&AfficheNtuple);
	if(VoiesNb > 1) {
		PanelKeyB(pProdEvtHdrAffichage,"Ligne de base",TANGO_EVT_LDB,&AfficheMemeLdB,9);
		PanelKeyB(pProdEvtHdrAffichage,"Temps maxi",TANGO_EVT_MAXI,&AfficheMemeMaxi,9);
	}
	if(BoloNb > 1) PanelBool(pProdEvtHdrAffichage,"Loupe trig",&AfficheLoupe);
	PanelKeyB(pProdEvtHdrAffichage,"Evt unite",TANGO_EVT_TRACE,&AfficheEvtCalib,9);
	PanelKeyB(pProdEvtHdrAffichage,"Deconvolution",TANGO_EVT_TRACE,&AfficheEvtDeconv,9);
	if(ModeleVoieNb > 1) {
		PanelSepare(pProdEvtHdrAffichage,"Voies a afficher");
		for(vm=0; vm<ModeleVoieNb; vm++) {  // necessite SambaAlloc donc ArchRunEdwx
			i = PanelBool(pProdEvtHdrAffichage,ModeleVoie[vm].nom,&(AfficheVoie[vm]));
			PanelItemColors(pProdEvtHdrAffichage,i,TangoRougeVertJaune,2);
			AfficheVoie[vm] = 1;
		}
	} else AfficheVoie[0] = 1;
	if(BoloNb > 1) {
		PanelKeyB(pProdEvtHdrAffichage,"Detecteurs","detecteur trigge seul/liste ci-dessous",&AfficheBoloDelaListe,22);
		PanelSepare(pProdEvtHdrAffichage,"Liste des detecteurs a utiliser");
		for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].lu != DETEC_ARRETE) {
			i = PanelBool(pProdEvtHdrAffichage,Bolo[bolo].nom,&(AfficheBolo[bolo]));
			PanelItemColors(pProdEvtHdrAffichage,i,TangoRougeVertJaune,2);
			AfficheBolo[bolo] = 1; // (Bolo[bolo].lu == DETEC_EN_SERVICE);
		} else AfficheBolo[bolo] = 0;
	} else AfficheBolo[0] = 1;
	AfficheBoloDelaListe = 1;

	pEvtDump = PanelCreate(3);
	PanelInt(pEvtDump,"Evenement no",&EvtDemande);
	if(VoiesNb > 1) PanelKeyB(pEvtDump,"Pour",VOIES_PERIM_CLES,&ProdToutes,23);
	PanelKeyB(pEvtDump,"Format","decimal/hexa",&ProdFormatHexa,8);

	pProdEvtCalib = PanelCreate(6);
	if(VoiesNb > 1) PanelKeyB(pProdEvtCalib,"Pour",VOIES_PERIM_CLES,&ProdToutes,23);
	PanelFloat(pProdEvtCalib,"Niveau minimum",&ProdNiveauMin);
	PanelBool (pProdEvtCalib,"Examen individuel",&ProdEvtManu);
	PanelFloat(pProdEvtCalib,"Energie moyenne",&EnergieRef);
#ifdef BOLOMETRES
	PanelFloat(pProdEvtCalib,"Khi2 chaleur maximum",&ProdKhi2ChalMax);
	PanelFloat(pProdEvtCalib,"Khi2 ionisation maximum",&ProdKhi2IonisMax);
#endif

#ifdef CROSSTALK
	pProdCrosstalk = PanelCreate(2);
	PanelKeyB(pProdCrosstalk,"Pour","le bolo choisi/les voies du detecteur/tous les bolos",&ProdToutes,23);
	PanelFloat(pProdCrosstalk,"Coefficient",&ProdCorrectionCT);
#endif

	strcpy(CorrigeIntervTxt[0],"Chaleur =");
	strcpy(CorrigeIntervTxt[1],"+ X x");
	for(i=2; i<MAXNONLIN; i++) sprintf(CorrigeIntervTxt[i],"+ X^%d x",i);
	pCorrigeInterv = PanelCreate(MAXINTERVCHALEUR * (MAXNONLIN + 3));

	pProdTraitements = PanelCreate(4+VoiesNb);
	PanelBool(pProdTraitements,"Soustraction pattern",&ProdPatternActif);
	if(VoiesNb > 1) PanelBool(pProdTraitements,"Correction Crosstalk",&ProdCrosstalkActif);
	PanelBool(pProdTraitements,"Filtrage",&ProdFiltreActif);
	PanelSepare(pProdTraitements,"Filtres si demande");
	for(voie=0; voie<VoiesNb; voie++) {
		PanelList(pProdTraitements,VoieManip[voie].nom,FiltreAutorises,&(FiltreVoies[voie]),MAXFILTRENOM);
	}
	PanelOnApply(pProdTraitements,ProdPostTrmts,0);
	PanelOnOK(pProdTraitements,ProdPostTrmts,0);

	/* Filtres Tango disponibles
		pProdChoixFiltres = PanelCreate(VoiesNb);
		for(voie=0; voie<VoiesNb; voie++) {
	 		PanelList(pProdChoixFiltres,VoieManip[voie].nom,FiltreAutorises,&(FiltreVoies[voie]),MAXFILTRENOM);
		}
		PanelOnApply(pProdChoixFiltres,ProdPostFiltrage,0);
		PanelOnOK(pProdChoixFiltres,ProdPostFiltrage,0); */

	/* Choix des zones de temps pour le calcul du n-tuple */
#ifdef PANEL_ZONES_V1
	p = pProdChoixTemps = PanelCreate(3 * (5 + 2 + VoiesNb + 2)); ???
	PanelColumns(p,3); PanelMode(p,PNL_BYLINES);
	PanelFloat(p,"Debut montee",&ProdMontee10); PanelBlank(p); PanelBlank(p);
	PanelFloat(p,"Fin   montee",&ProdMontee90); PanelBlank(p); PanelBlank(p);
	PanelFloat(p,"Quota montee",&ProdQuotaMontee); PanelBlank(p); PanelBlank(p);
	PanelInt  (p,"Pre-lissage", &ProdPreLissage);  PanelBlank(p); PanelBlank(p);
	PanelInt  (p,"Post-lissage",&ProdPostLissage); PanelBlank(p); PanelBlank(p);
	PanelSepare(p,0); PanelSepare(p,"temps en ms"); PanelSepare(p,0);
	PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"debut base",10),0),1);
	PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"fin base",10),0),1);
	PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,"descente",10),0),1);
	k = 0;
	for(voie=0; voie<VoiesNb; voie++) {
		char fin_de_serie;
		fin_de_serie = !((k++ + 1) % 3);
		i = PanelFloat(p,VoieManip[voie].nom,&(VoieTemps[voie].start));
		PanelItemSouligne(p,i,fin_de_serie);
		i = PanelFloat(p,0,&(VoieTemps[voie].stop));
		PanelItemSouligne(p,i,fin_de_serie);
		i = PanelFloat(p,0,&(VoieTemps[voie].apres));
		PanelItemSouligne(p,i,fin_de_serie);
	}
	PanelSepare(p,0); PanelSepare(p,"temps en s"); PanelSepare(p,0);
	PanelInt(p,"Multiple si Amplitude >",&ProdPaquetAmpl);
	PanelFloat(p,"pour duree maxi",&ProdPaquetDuree);
	PanelBlank(p);
#else  /* PANEL_ZONES_V1 */
	bProdChoixTemps = BoardCreate();
	ProdTempsBoardFill();

#endif /* PANEL_ZONES_V1 */

	pProdCalibVoie = PanelCreate(2);
	PanelFloat(pProdCalibVoie,"Valeur actuelle",&Actuelle);
	PanelFloat(pProdCalibVoie,"Valeur visee",&EnergieRef);

	pEnergies = PanelCreate(1);
	PanelFloat(pEnergies,"Seuil chaleur",&SeuilChal);
	PanelTitle(pEnergies,"Calcul des energies");

	pAfficheAmplMin = PanelCreate(1);
	PanelFloat(pAfficheAmplMin,"Amplitude min (absolue)",&ProdAmplMin);

	pQuenching = PanelCreate(1);
	PanelFloat(pQuenching,"Polarisation ionisation",&PolarIonisation);
	PanelTitle(pQuenching,"Calcul du facteur de quenching");

	n = 0;
	for(vm=0; vm<ModeleVoieNb; vm++) if(ModeleVoie[vm].utilisee) n++;
	//	for(vm=0; vm<ModeleVoieNb; vm++) printf(": Voie de type %s %s\n",ModeleVoie[vm].nom,ModeleVoie[vm].utilisee? "utilisee": "absente");
	//	printf(": %d type(s) de voie utilise(s)\n",n);
	pProdUniteZoneFit = PanelCreate(2 * n);
	for(vm=0; vm<ModeleVoieNb; vm++) if(ModeleVoie[vm].utilisee) {
		sprintf(&(TxtMontee[vm][0]),"Nb points montee voies %s",ModeleVoie[vm].nom);
		PanelInt(pProdUniteZoneFit,&(TxtMontee[vm][0]),&(TempsMontee[vm]));
		sprintf(&(TxtDescente[vm][0]),"Nb points descente voies %s",ModeleVoie[vm].nom);
		PanelInt(pProdUniteZoneFit,&(TxtDescente[vm][0]),&(TempsDescente[vm]));
	}

	pProdUniteChoixVoie = PanelCreate(2);
	if(BoloNb > 1) PanelList(pProdUniteChoixVoie,"Detecteur concerne",BoloNom,&BoloNum,DETEC_NOM);
	if(VoiesNb > 1) PanelList(pProdUniteChoixVoie,"Voie concernee",ModeleVoieListe,&ModlNum,MODL_NOM);

	pProdCalibSauve = PanelCreate(2);
	PanelText(pProdCalibSauve,"Chemin",ProdCalibPath,MAXFILENAME);
	PanelText(pProdCalibSauve,"Fichier",ProdCalibName,MAXFILENAME);
	PanelTitle(pProdCalibSauve,"Coefficients de calibration");

	pProdChaleurSauve = PanelCreate(2);
	PanelText(pProdChaleurSauve,"Chemin",ProdCalibPath,MAXFILENAME);
	PanelText(pProdChaleurSauve,"Fichier",ProdChaleurName,MAXFILENAME);
	PanelTitle(pProdChaleurSauve,"Coefficients voie chaleur");

#ifdef BOLOMETRES
	pProdChaleurFit = PanelCreate(8);
	PanelFloat(pProdChaleurFit,"Energie chaleur min",&ProdChaleurFitMin);
	PanelFloat(pProdChaleurFit,"Energie chaleur max",&ProdChaleurFitMax);
	PanelInt  (pProdChaleurFit,"Nombre de bins",&ProdChaleurFitNb);
	PanelFloat(pProdChaleurFit,"Seuil chaleur",&SeuilChal);
	PanelFloat(pProdChaleurFit,"Seuil ionisation",&SeuilIonis);
	PanelFloat(pProdChaleurFit,"Khi2 chaleur maximum",&ProdKhi2ChalMax);
	PanelFloat(pProdChaleurFit,"Khi2 ionisation maximum",&ProdKhi2IonisMax);
	PanelList (pProdChaleurFit,"Bolo concerne",BoloNom,&BoloNum,DETEC_NOM);
#endif

	pProdCalibCoefs = PanelCreate(VoiesNb * 2);
	for(voie=0; voie<VoiesNb; voie++) {
		ProdCalibCoefTxt[voie] = (char*)malloc(80);
		if(ProdCalibCoefTxt[voie]) sprintf(ProdCalibCoefTxt[voie],"Coefficient %s",VoieManip[voie].nom);
	}

	pProdResolutions = PanelCreate(15); /* voir ArgDesc ProdCtes[] */
	PanelFloat (pProdResolutions,"Polarisation ionisation",&PolarIonisation);
	PanelFloat (pProdResolutions,"Energie du signal de reference",&EnergieRef);
	PanelSepare(pProdResolutions,"Voie chaleur");
	PanelFloat (pProdResolutions,"Seuil",&SeuilChal);
	PanelFloat (pProdResolutions,"Resolution ligne de base",&ResolChalBase);
	PanelFloat (pProdResolutions,"Resolution signal",&ResolChalRef);
	PanelFloat (pProdResolutions,"Amplitude de calibration (ADU)",&RefChal);
	PanelSepare(pProdResolutions,"Voie ionisation");
	PanelFloat (pProdResolutions,"Seuil",&SeuilIonis);
	PanelFloat (pProdResolutions,"Resolution ligne de base",&ResolIonisBase);
	PanelFloat (pProdResolutions,"Resolution signal",&ResolIonisRef);
	PanelFloat (pProdResolutions,"Amplitude de calibration (ADU)",&RefIonis);
	PanelSepare(pProdResolutions,"Zone Neutrons");
	PanelFloat (pProdResolutions,"Facteur multiplicatif",&NeutronsFact);
	PanelFloat (pProdResolutions,"Exposant",&NeutronsExp);

	tProdVar = TermCreate(PRODVAR_MAX+2,100,(PRODVAR_MAX+2)*100);
	TermTitle(tProdVar,L_("Variables utilisateur"));
}
/* ========================================================================== */
int ProdInit(char a_nouveau) {
	int i,n; int bolo,voie,vm,mode,msr; char dbg;

//	printf("%s, %s\n",__func__,a_nouveau? "a nouveau": "premiere fois");

	if(a_nouveau) return(1);
	dbg = 0;
	
	/* Initialisation des variables utilisees par la prod */
	ProdToutes = PERIM_TOUTES;
	ProdFormatHexa = 0;
	ModlNum = 0; BoloNum = 0; VoieNum = 0;
	Actuelle = 1.0;
    ProdVarsModifiees = 0;
	ProdEvtManu = 1;
	ProdPreLissage = 3;
	ProdPostLissage = 3;
	ProdDeconvRC = SambaRCdefaut;
	ProdNiveauMin = 80.0;
	ProdTauxDt[0] = 0.0; ProdTauxDt[1] = 10.0; gTauxEvts = 0;
	ProdAmplMin = 0.0;
	ProdChaleurFitMin = 0.0; ProdChaleurFitMax = 200.0;
	ProdChaleurFitNb = 20;
	ProdKhi2ChalMax = .002;
	ProdKhi2IonisMax = .05;
	ProdPatternActif = 0;
	ProdFiltreActif = 0;	
	ProdCrosstalkActif = 0; ProdCorrectionCT = 1.0 / 32.0;
	ProdPaquetAmpl = 20000; ProdPaquetDuree = 5.0;
	PaqNb = 0;
	for(vm=0; vm<DETEC_CAPT_MAX; vm++) {
		TempsMontee[vm] = 128;
		TempsDescente[vm] = 256;
	}
	MesureFaite = EDW_AMPL;
	MesurePourCalib = EDW_EVT;

	for(voie=0; voie<VOIES_MAX; voie++) VoieTemps[voie].mesure = -1;
	for(msr=0; msr<VOIES_MAX; msr++) VoieTemps[msr].num = -1;
	ArgScan(ProdTempsFile,ProdTempsDesc);
	for(msr=0; msr<VoieTempsNb; msr++) {
		voie = VoieTemps[msr].num; ProdTempsBase(voie,msr);
		switch(VoieTemps[msr].ref) {
		  case TEMPS_SAMBA: ProdTempsSamba(voie,msr); break;
		  case TEMPS_REL:   ProdTempsRelToAbs(msr);    break;
		  case TEMPS_ABS:   ProdTempsAbsToRel(msr);    break;
		}
	}
	ProdTempsComplete(); //dbg
	ArgPrint("*",ProdTempsDesc);
	ProdTempsStandard = (VoieTempsNb == 0);

	VoieNum = 0;

#ifdef BOLOMETRES
	VoieChaleur = VoieCentre = VoieGarde = -1;
	for(vm=0; vm<ModeleVoieNb; vm++) {
		if(!strcmp(ModeleVoie[vm].nom,NOM_CHALEUR)) VoieChaleur = vm;
		else if(!strcmp(ModeleVoie[vm].nom,NOM_CENTRE)) VoieCentre = vm;
		else if(!strcmp(ModeleVoie[vm].nom,NOM_GARDE)) VoieGarde = vm;
	}
#endif
	
	for(mode=0; mode<EDW_NBMESURES; mode++)
		for(voie=0; voie<VOIES_MAX; voie++) {
			ProdCalibFaite[mode][voie] = 0;
			Etalon[mode][voie] = 1.0;
			Coef[mode][voie] = 1.0;
	}
	for(bolo=0; bolo<BoloNb; bolo++) {
		NbIntervChal[bolo] = 0;
		for(n=0; n<MAXINTERVCHALEUR; n++) {
			CoefChal[bolo][n].inf = 0.0;
			CoefChal[bolo][n].sup = 0.0;
			CoefChal[bolo][n].nb = 2;
			CoefChal[bolo][n].c[0] = 0.0;
			CoefChal[bolo][n].c[1] = 1.0;
			for(i=2; i<MAXNONLIN; i++) CoefChal[bolo][n].c[i] = 0.0;
		}
	}

	/* Liste des filtres disponibles (dont le filtre d'origine) */
	FiltreAutorises[0] = "Filtre SAMBA";
	for(i=0; i<MAXFILTRES+1; i++) FiltreAutorises[i+1] = FiltreGeneral[i].nom;
	
	BoloNom[BoloNb] = "\0";

	if(!ProdVarNtupleStandard()) return(0);
	if(!PlotVarsDeclare(Ntuple)) {
		PlotPrint(OPIUM_ERROR,"Manque de memoire pour la declaration des variables");
		return(0);
	}
    ProdVarLit(0,0);
    ProdVarRestoreNtuple();
	// ProdVarDump(__func__,5);
	printf(". Ntuple complet (%d variables)\n",NtTotNb);

	return(1);
}
/* ========================================================================== */
void ProdExit() {
    if(ProdVarsModifiees && ProdVarNb) ProdVarEcrit(0,0);
}
