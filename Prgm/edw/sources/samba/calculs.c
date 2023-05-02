#ifdef macintosh
#pragma message(__FILE__)
#endif
#define BRANCHE_CALC
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <environnement.h>

/* #define PI (double)3.141592653589793238 deja defini dans opium.h */

#ifdef PROJECT_BUILDER
	#define sqrtf(x) (float)sqrt((double)x)
	#define expf(x) (float)exp((double)x)
#endif
#ifdef CODE_WARRIOR_VSN
	#include <claps.h>
	#include <dateheure.h>
	#include <impression.h>
	#include <opium_demande.h>
	#include <opium.h>
	#include <rfftw.h>
#else
	#include <utils/claps.h>
	#include <utils/dateheure.h>
	#include <opium_demande.h>
	#include <opium4/opium.h>
	#include <opium4/impression.h>
#endif
#include <samba.h>
#include <numeriseurs.h>
#include <detecteurs.h>
#include <objets_samba.h>
#include <monit.h>
#include <autom.h>

typedef enum {
	AVEC_T1 = 0,
	NB_POINTS
} CALCUL_MODE;

typedef enum {
	CALC_CUMULE = 0,
	CALC_CONTINUE,
	CALC_FERME,
	CALC_MONTRE
} CALCUL_SVGFINS;
static char *CalCulSvgFins = "superpose/laisse ouvert/ferme/ferme et montre";

#define MAXSPECTRES 64
typedef struct {
	char   titre[80];
	int voie;                /* voie representee           */
	float  nb;               /* nombre de FFT moyennees    */
	int    dim;              /* nombre de points de la FFT */
	float  freq[2];          /* frequences (ab6 du graphe) */
	float *fft;              /* spectre correspondant      */
} TypeCalcSpectre;

#define MAXCALCFEN 16
#define MAXFFTFEN 8
typedef struct {
	char titre[80];
	int nb;                 /* nombre de spectres   */
	int spectre[MAXFFTFEN]; /* indexes des spectres */
	OpiumColorState s;      /* suivi des couleurs   */
	Graph g;
} TypeCalcFenetre;

static int CalcBolo;
static TypeCalcSpectre CalcSpectre[MAXSPECTRES];
static int CalcSpectreNb,CalcSpectreNum;
static char *CalcSpectreNom[MAXSPECTRES+1];
static TypeCalcFenetre CalcFen[MAXCALCFEN];
static int CalcFenNb,CalcFenNum;
static char *CalcFenNom[MAXCALCFEN+1];

static char CalcSpectreFichier[MAXFILENAME];

static float CalcSpectreMoyenneFreq[2];
static float *CalcSpectreMoyenne; int CalcSpectreMoyenneDim;
static Graph gCalcSpectreMoyenne;

static OpiumColorState oCouleur[DETEC_MAX][PHYS_TYPES];

static char CalcASauver;
static Panel pFichierCalcLect,pFichierCalcEcri;
static Panel pCalcSauve;
// static Panel pCalcDesc; /* pas defini au 09.05.01 */

static Graph gCalcSpectreAutoPrems;
#define NOTE_MAX 79
static char CalcSpectreAutoNote[NOTE_MAX];
static Panel pCalcStatsInterv1,pCalcStatsIntervN,pCalcStatsResul;
static Panel pCalcSpectre1,pCalcSpectreN,pCalcSpectreDetecParm;
static float CalcT0,CalcT1; int64 CalcP0,CalcP1;
static fftw_real *CalcFftSolo;
static int CalcSoloNb;
static float CalcMoyenne,CalcSigma;
static TypeInstrumColonne CalcNiveau = { 200, 8, 1, 0, 3, 1 };
static char CalcSpectreSauvePDF,CalcSpectreSauveDB,CalcSpectreSauveMontre;
static char CalcSpectreSauveNom[MAXFILENAME];
static Panel pCalcSpectreSauve,pCalcSpectreFerme;

static Panel pCalcSelect; static Info iCalcSelect;
static Panel pCalcSelPt1,pCalcSelPt2;
static char CalcSelMode;

static Panel pCalcSuperpose;

/* #define HISTOS_USER */
static Panel pCalcHistos;
static Term tCalc;

static TypeTrmtCsnsm TrmtCsnsmLu;
/* ArgDesc TrmtCsnsmDesc[] = {
	{ 0,   DESC_STR(16) TrmtCsnsmLu.nom, 0 },
	{ "A", DESC_INT    &TrmtCsnsmLu.A, "Retard de l'entree (echantillons)" },
	{ "B", DESC_INT    &TrmtCsnsmLu.B, "Demi ligne de base (echantillons)" },
	{ "C", DESC_INT    &TrmtCsnsmLu.C, "<base> - <max> (echantillons)" },
	{ "D", DESC_INT    &TrmtCsnsmLu.D, "<max> - <min> (echantillons)" },
	DESC_END
}; */
static ArgDesc TrmtCsnsmDesc[] = {
	{ 0,   DESC_STR(16) TrmtCsnsmLu.nom, 0 },
	{ "A", DESC_INT    &TrmtCsnsmLu.A, 0 },
	{ "B", DESC_INT    &TrmtCsnsmLu.B, 0 },
	{ "C", DESC_INT    &TrmtCsnsmLu.C, 0 },
	{ "D", DESC_INT    &TrmtCsnsmLu.D, 0 },
	DESC_END
};
static ArgStruct TrmtCsnsmAS = { (void *)TrmtCsnsm, (void *)&TrmtCsnsmLu, sizeof(TypeTrmtCsnsm), TrmtCsnsmDesc };

static ArgDesc CalcPhysDesc[] = {
	{ 0,           DESC_STR(MODL_NOM)        CalcSpectreAutoLu.nom_phys,           0 },
	{ "dimension", DESC_INT                 &CalcSpectreAutoLu.dim,                0 },
	{ "X.echelle", DESC_KEY "lineaire/log", &CalcSpectreAutoLu.parms[0].log,       0 },
	{ "X.limites", DESC_KEY "manu/auto",    &CalcSpectreAutoLu.parms[0].autom,     0 },
	{ "X.min",     DESC_FLOAT               &CalcSpectreAutoLu.parms[0].lim.r.min, 0 },
	{ "X.max",     DESC_FLOAT               &CalcSpectreAutoLu.parms[0].lim.r.max, 0 },
	{ "Y.echelle", DESC_KEY "lineaire/log", &CalcSpectreAutoLu.parms[1].log,       0 },
	{ "Y.limites", DESC_KEY "manu/auto",    &CalcSpectreAutoLu.parms[1].autom,     0 },
	{ "Y.min",     DESC_FLOAT               &CalcSpectreAutoLu.parms[1].lim.r.min, 0 },
	{ "Y.max",     DESC_FLOAT               &CalcSpectreAutoLu.parms[1].lim.r.max, 0 },
	DESC_END
};
static ArgStruct CalcPhysAS = { (void *)CalcSpectreAutoPrefs, (void *)&CalcSpectreAutoLu, sizeof(CalcSpectreAutoLu), CalcPhysDesc };

/* static ArgStruct CalcPhysVerifAS = { (void *)CalcSpectreAuto, (void *)&CalcSpectreAutoLu, sizeof(CalcSpectreAutoLu), CalcPhysDesc };
static ArgDesc CalculVerifDesc[] = {
	{ "Physique",      DESC_STRUCT_SIZE(CalcPhysNb,PHYS_TYPES) &CalcPhysVerifAS, 0 },
	DESC_END
}; */

static ArgDesc CalculDesc[] = {
	{ DESC_COMMENT "Spectres" },
	{ "Donnees",        DESC_LISTE CalcDonneesName, &CalcDonnees,           "Type de donnees utilisees" },
	{ "PseudoVoies",    DESC_KEY "exclues/aussi",   &CalcSpectreAutoPseudo, "Prise en compte des pseudo voies (exclues/aussi)" },
	{ "Invalidees",     DESC_KEY "exclues/aussi",   &CalcSpectreAutoInval,  "Prise en compte des voies invalidees au vol(exclues/aussi)" },
	{ "Mode.auto",      DESC_KEY "somme/trigge/glissant", &CalcModeAuto,    "Methode de calcul des spectres automatiques (somme/trigge/glissant)" },
	{ "Spectres.max",   DESC_INT                    &CalcSpectresMax,       "Nb spectres si somme ou glissant" },
	{ "Spectres.larg",  DESC_INT                    &CalcSpectreAutoDimX,   "Largeur de la fenetre d'affichage" },
	{ "Spectres.haut",  DESC_INT                    &CalcSpectreAutoDimY,   "Hauteur de la fenetre d'affichage" },
	{ DESC_COMMENT 0 },
	{ DESC_COMMENT "Spectres multiples" },
	{ "Physique",       DESC_STRUCT_SIZE(CalcPhysNb,PHYS_TYPES) &CalcPhysAS, 0 },
	{ DESC_COMMENT 0 },
	{ DESC_COMMENT "Traitements speciaux" },
	{ DESC_COMMENT "CSNSM: A=Retard de l'entree (echantillons), B=Demi ligne de base (echantillons)," },
	{ DESC_COMMENT "       C=<base> - <max> (echantillons), D=<max> - <min> (echantillons)" },
	{ "CSNSM.filtres",  DESC_STRUCT_SIZE(TrmtCsnsmNb,TRMT_CSNSM_MAX) &TrmtCsnsmAS, 0 },
	DESC_END
};

/* ========================================================================== */
int CalculeSelect() {
	int ix,iy; float rx,ry; int k;
	int64 tour,nbtours,of7;
	int voie;
	double t0;
	TypeMonitTrace *t;
	Graph g; WndContextPtr gc;

	if(OpiumExec(pCalcSelect->cdr) == PNL_CANCEL) return(0);
	if(CalcSelMode == NB_POINTS) {
		if(OpiumExec(pCalcSelPtNb->cdr) == PNL_CANCEL) return(0);
		if(CalcMax > MAXINTERVALLES) CalcMax = MAXINTERVALLES;
	} else CalcMax = 1;

	t = MonitFen[MonitFenNum].trace;
	voie = t->voie;  /* premiere voie declaree dans la fenetre */
	tour = VoieTampon[voie].brutes->max;
	nbtours = VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien / tour;
	of7 = tour * nbtours;
	t0 = (double)of7 * VoieEvent[voie].horloge / 1000.0; /* secondes */

	if(!MonitFen[MonitFenNum].g) MonitFenAffiche(&(MonitFen[MonitFenNum]),0);
	if(!(g = MonitFen[MonitFenNum].g)) {
		OpiumError("Fenetre '%s' pas affichee (vide, ou tampon pas cree)",
			MonitFen[MonitFenNum].titre);
		return(0);
	}
	gc = WndContextCreate((g->cdr)->f);
	WndContextFgndRGB((g->cdr)->f,gc,GRF_RGB_RED);

	k = 0;
	while(k < CalcMax) {
		if(CalcMax == 1) InfoWrite(iCalcSelect,1,"Cliques sur le temps initial");
		else InfoWrite(iCalcSelect,1,"Cliques sur le temps initial #%d",k+1);
		OpiumDisplay(iCalcSelect->cdr);
		do {
			if(GraphGetPoint(g,&ix,&rx,&iy,&ry) == PNL_CANCEL) {
				CalcMax = k;
				if(CalcMax > 1) OpiumError("Seulement %d intervalles saisis",CalcMax);
				else OpiumError("%sintervalle saisi",CalcMax? "Un seul ": "Pas d'");
				break;
			}
			if(CalcSelMode == AVEC_T1)
				GraphUserScrollX(g,0,rx -  10.0 * VoieEvent[voie].horloge); /* recule de 10 points */
			else /* avance au bout de l'intervalle moins 10 points */
				GraphUserScrollX(g,0,rx +  ((float)CalcDim - 10.0) * VoieEvent[voie].horloge);
				/* if(OpiumDisplayed(g->cdr)) { x = GraphScreenXFloat(g,rx); GraphAxisYDraw(g,x,gc,GRF_GRADNONE); } */
			CalcT0 = rx; /* millisecondes apres t0 */
		} while(OpiumExec(pCalcSelPt1->cdr) == PNL_CANCEL);
		OpiumClear(iCalcSelect->cdr);
		if(k < CalcMax) CalcTi[k++] = (double)CalcT0;
	}
	CalcT0 = (float)CalcTi[0];

	if(CalcSelMode == AVEC_T1) {
		InfoWrite(iCalcSelect,1,"Cliques sur le temps final");
		OpiumDisplay(iCalcSelect->cdr);
		do {
			if(GraphGetPoint(g,&ix,&rx,&iy,&ry) == PNL_CANCEL) {
				CalcT1 = CalcT0 + ((float)CalcDim * VoieEvent[voie].horloge);
				break;
			}
			/* if(OpiumDisplayed(g->cdr)) { x = GraphScreenXFloat(g,rx); GraphAxisYDraw(g,x,gc,GRF_GRADNONE); } */
			CalcT1 = rx; /* millisecondes apres t0 */
		} while(OpiumExec(pCalcSelPt2->cdr) == PNL_CANCEL);
		OpiumClear(iCalcSelect->cdr);
		if(CalcT0 > CalcT1) {
			CalcTi[0] = CalcT1;
			CalcT1 = CalcT0;
			CalcT0 = CalcTi[0];
		}
		CalcDim = (int)(((CalcT1 - CalcT0) / VoieEvent[voie].horloge) + 0.5);
	} else if(CalcMax == 1) CalcT1 = CalcT0 + ((float)CalcDim * VoieEvent[voie].horloge);

	for(k=0; k<CalcMax; k++) {
		CalcPi[k] = (int64)(CalcTi[k] / (double)VoieEvent[voie].horloge) + of7;
		CalcTi[k] = (CalcTi[k] / 1000.0) + t0; /* secondes depuis demarrage */
	}
	if(CalcMax == 1) {
		CalcP0 = (int64)(CalcT0 / (double)VoieEvent[voie].horloge) + of7;
		CalcP1 = (int64)(CalcT1 / (double)VoieEvent[voie].horloge) + of7;
		CalcT0 = (float)((double)CalcT0 / 1000.0) + t0;
		CalcT1 = (float)((double)CalcT1 / 1000.0) + t0;
	}

	WndContextFree((g->cdr)->f,gc);
	return(0);
}
/* ========================================================================== */
int CalculeStats() {
	int voie,bolo,vm,cap;
	int i,j,k,dim;
	int64 p0,p1;
	TypeDonnee *ptr_data;
	double val,somme,carre,nb;

	bolo = CalcBolo; vm = ModlNum; voie = 0;
	if(CalcMax == 1) {
		if(OpiumExec(pCalcStatsInterv1->cdr) == PNL_CANCEL) return(0);
	} else {
		if(OpiumExec(pCalcStatsIntervN->cdr) == PNL_CANCEL) return(0);
	}
	if(Bolo[CalcBolo].lu == DETEC_HS) {
		OpiumError("Le detecteur %s n'est pas lu",Bolo[CalcBolo].nom);
		CalcBolo = bolo; ModlNum = vm;
		return(0);
	}
	for(cap=0; cap<Bolo[CalcBolo].nbcapt; cap++) {
		voie = Bolo[CalcBolo].captr[cap].voie;
		if(ModlNum == VoieManip[voie].type) break;
	}
	if(cap == Bolo[CalcBolo].nbcapt) {
		OpiumError("Pas de voie '%s' pour le detecteur '%s'",ModeleVoie[ModlNum].nom,Bolo[CalcBolo].nom);
		CalcBolo = bolo; ModlNum = vm;
		return(0);
	}
	VoieNum = voie;
	somme = carre = nb = 0.0;
	for(k=0; k<CalcMax; k++) {
		p0 = CalcPi[k];
		if(p0 < VoieTampon[VoieNum].trmt[TRMT_AU_VOL].plus_ancien) p0 = VoieTampon[VoieNum].trmt[TRMT_AU_VOL].plus_ancien;
		if(CalcMax == 1) p1 = CalcP1; else p1 = p0 + CalcDim;
		if(p1 > VoieTampon[VoieNum].lus) p1 = VoieTampon[VoieNum].lus;
		dim = (int)(p1 - p0);
		j = Modulo(p0,VoieTampon[VoieNum].brutes->max);
		ptr_data = VoieTampon[VoieNum].brutes->t.sval + j;
		for(i=0; i<dim; i++) {
			val = (double)*ptr_data++;
			somme += val;
			carre += (val * val);
			nb += 1.0;
			if(++j >= VoieTampon[VoieNum].brutes->max) { j = 0; ptr_data = VoieTampon[VoieNum].brutes->t.sval; }
		}
	}
	CalcMoyenne = (float)(somme / nb);
	CalcSigma = (float)sqrt((carre / nb) - (somme * somme));
	OpiumDisplay(pCalcStatsResul->cdr);

	return(0);
}
/* ========================================================================== */
char CalculeFFTplan(CalcFftBase base, int nbpts, char *raison) {
	int taille;

	if(nbpts != base->nbpts) {
		if(base->nbpts) {
			rfftw_destroy_plan(base->plan);
			if(base->temps) free(base->temps);
			if(base->freq) free(base->freq);
			base->nbpts = 0;
		}
		if(!nbpts) {
			sprintf(raison,"Nombre de points d'analyse egal a 0");
			return(0);
		}
		base->plan = rfftw_create_plan(nbpts,FFTW_REAL_TO_COMPLEX,FFTW_ESTIMATE);
		base->nbpts = nbpts;
		taille = nbpts * sizeof(fftw_real);
		base->temps = (fftw_real *)malloc(taille);
		if(!base->temps) {
			sprintf(raison,"Manque de place memoire pour le tableau en temps (%d octets)",taille);
			return(0);
		}
		base->freq = (fftw_real *)malloc(taille);
		if(!base->freq) {
			sprintf(raison,"Manque de place memoire pour le tableau en frequence (%d octets)",taille);
			return(0);
		}
//		printf("(%s) Tableaux FFT crees avec %d valeurs (temps @%08X et freq %08X)\n",__func__,nbpts,CalcPlanStd.temps,CalcPlanStd.freq);
	}
	return(1);
}
/* ========================================================================== */
void CalculePuissance(fftw_real *fft, float *spectre, int nbpts, float dt, float unite) {
    /* unite: ADU -> nV */
	int i; float f;

	f = (unite > 0.0)? unite: -1.0 * unite;
	spectre[0] = f * sqrtf(fft[0] * fft[0] * dt);
	for(i=1; i<(nbpts + 1) / 2; i++)
		spectre[i] = f * sqrtf(((fft[i] * fft[i]) + (fft[nbpts - i] * fft[nbpts - i])) * dt);
	if(!(nbpts % 2)) spectre[nbpts / 2] = f * sqrtf(fft[nbpts / 2] * fft[nbpts / 2] * dt);
	return;
}
/* ===================================================================== 
char CalculeFFTTable(TypeDonnee *entree, int nbpts, char *raison) {
	TypeDonnee *ptr_data; fftw_real *ptr_temps;
	int i;

	if(nbpts == 0) return(1);
	if(!CalculeFFTplan(&CalcPlanStd,nbpts,raison)) return(0);
	ptr_data = entree;
	ptr_temps = CalcPlanStd.temps;
	for(i=0; i<nbpts; i++) *ptr_temps++ = (fftw_real)*ptr_data++;
	rfftw_one(CalcPlanStd.plan,CalcPlanStd.temps,CalcPlanStd.freq);
	return(1);
}
   ===================================================================== */
char CalculeFFTVoie(char mode, int trmt, int voie, int64 *pt, int nbinterv, int nbpts, 
	float *spectre, char *raison) {
	int i,j,k;
	int64 pt0,lus,premier;
	int taille;
	fftw_real *ptr_temps,*ptr_fft,*ptr_solo,*ptr_mont,*ptr_desc; float nb; float normalisation;
	TypeDonnee *ptr_int16; TypeSignal *ptr_float; TypeTamponDonnees *tampon; float *ptr_evt;

	if((nbinterv == 0) || (nbpts == 0)) return(1);
	if(!CalculeFFTplan(&CalcPlanStd,nbpts,raison)) return(0);
	tampon = 0; premier = 0; lus = nbpts;
	// if(trmt == TRMT_AU_VOL) tampon = &(VoieTampon[voie].brutes);
	// else if(trmt == TRMT_PRETRG) tampon = &(VoieTampon[voie].traitees);
	tampon = &(VoieTampon[voie].trmt[trmt].donnees);
	if(tampon) {
		lus = tampon->prem + tampon->nb;
		premier = tampon->prem;
	}
//	printf("(CalculeFFTVoie) %d intervalles:",nbinterv);
//	for(k=0; k<nbinterv; k++) printf(" [%d] %d",k,(int)pt[k]);
//	printf("\n");
	
	if(mode == FFT_SUR_MOYENNE) {
		if(trmt >= TRMT_NBCLASSES) {
			strcpy(raison,"incompatibility between parameters (tiens, tiens!)");
			return(0);
		}
		nb = 0; ptr_temps = CalcPlanStd.temps; for(i=0; i<nbpts; i++) *ptr_temps++ = 0.0;
		for(k=0; k<nbinterv; k++) {
			pt0 = pt[k];
			if((pt0 + (int64)nbpts) > lus) pt0 = lus - (int64)nbpts;
			if(pt0 < premier) {
				pt0 = premier;
				if((pt0 + (int64)nbpts) > lus) nbpts = (int)(lus - pt0);
			}
			//	printf("(CalculeFFTVoie) %d/%d FFT depuis %g ms sur %d points\n",
			//		k+1,nbinterv,(double)pt0*(double)VoieEvent[voie].horloge,nbpts);
			ptr_temps = CalcPlanStd.temps;
			j = Modulo(pt0,tampon->max);
			if(tampon->type == TAMPON_INT16) {
				ptr_int16 = tampon->t.sval + j;
				for(i=0; i<nbpts; i++) {
					*ptr_temps += (fftw_real)*ptr_int16++;
					ptr_temps++;
					if(++j >= tampon->max) { j = 0; ptr_int16 = tampon->t.sval; }
				}
			} else {
				ptr_float = tampon->t.rval + j;
				for(i=0; i<nbpts; i++) {
					*ptr_temps += (fftw_real)*ptr_float++;
					ptr_temps++;
					if(++j >= tampon->max) { j = 0; ptr_float = tampon->t.rval; }
				}
			}
			nb += 1.0;
		}
		ptr_temps = CalcPlanStd.temps; for(i=0; i<nbpts; i++) { *ptr_temps /= nb; ptr_temps++; }
		rfftw_one(CalcPlanStd.plan,CalcPlanStd.temps,CalcPlanStd.freq);

	} else if(nbinterv == 1) {
		if(trmt >= TRMT_NBCLASSES) /* c'est a dire evenement unite */ {
			if(!VoieTampon[voie].unite.val) {
				sprintf(raison,"L'evenement moyen %s n'a pas ete memorise",VoieManip[voie].nom);
				return(0);
			}
			ptr_evt = VoieTampon[voie].unite.val;
			ptr_temps = CalcPlanStd.temps;
			for(i=0; i<nbpts; i++) *ptr_temps++ = (fftw_real)(*ptr_evt++);
		} else {
			pt0 = *pt;
			if((pt0 + (int64)nbpts) > lus) pt0 = lus - (int64)nbpts;
			if(pt0 < premier) {
				pt0 = premier;
				if((pt0 + (int64)nbpts) > lus) nbpts = (int)(lus - pt0);
			}
			//	printf("(CalculeFFTVoie) %d/%d FFT depuis %g ms sur %d points\n",
			//		1,nbinterv,(double)pt0*(double)VoieEvent[voie].horloge,nbpts);
			ptr_temps = CalcPlanStd.temps;
			j = Modulo(pt0,tampon->max);
			if(tampon->type == TAMPON_INT16) {
				ptr_int16 = tampon->t.sval + j;
				for(i=0; i<nbpts; i++) {
					*ptr_temps++ = (fftw_real)*ptr_int16++;
					if(++j >= tampon->max) { j = 0; ptr_int16 = tampon->t.sval; }
				}
			} else {{
				ptr_float = tampon->t.rval + j;
				for(i=0; i<nbpts; i++) {
					*ptr_temps++ = (fftw_real)*ptr_float++;
					if(++j >= tampon->max) { j = 0; ptr_float = tampon->t.rval; }
				}
			}
			}
		}
		rfftw_one(CalcPlanStd.plan,CalcPlanStd.temps,CalcPlanStd.freq);

	} else /* (mode == FFT_MOYENNEE) || (mode == FFT_SOMMEE) || (mode == FFT_PUISSANCE) */ {
		if(trmt >= TRMT_NBCLASSES) {
			strcpy(raison,"Incompatibility between parameters (tiens, tiens!)");
			return(0);
		}
		if(nbpts > CalcSoloNb) {
			if(CalcFftSolo) free(CalcFftSolo);
			taille = nbpts * sizeof(fftw_real);
			CalcFftSolo = (fftw_real *)malloc(taille);
			if(!CalcFftSolo) {
				sprintf(raison,"Manque de place memoire pour le tableau auxiliaire (%d octets)",taille);
				return(0);
			}
			CalcSoloNb = nbpts;
		}
		nb = 0; ptr_fft = CalcPlanStd.freq; for(i=0; i<nbpts; i++) *ptr_fft++ = 0.0;
		for(k=0; k<nbinterv; k++) {
			pt0 = pt[k];
			if((pt0 + (int64)nbpts) > lus) pt0 = lus - (int64)nbpts;
			if(pt0 < premier) {
				pt0 = premier;
				if((pt0 + (int64)nbpts) > lus) nbpts = (int)(lus - pt0);
			}
			//	printf("(CalculeFFTVoie) %d/%d FFT depuis %g ms sur %d points\n",
			//		k+1,nbinterv,(double)pt0*(double)VoieEvent[voie].horloge,nbpts);
//			printf("(CalculeFFTVoie) %d/%d: FFT sur %d[%d]\n",k,nbinterv,(int)pt0,nbpts);
			ptr_temps = CalcPlanStd.temps;
			j = Modulo(pt0,tampon->max);
			if(tampon->type == TAMPON_INT16) {
				ptr_int16 = tampon->t.sval + j;
				for(i=0; i<nbpts; i++) {
					*ptr_temps++ = (fftw_real)*ptr_int16++;
					if(++j >= tampon->max) { j = 0; ptr_int16 = tampon->t.sval; }
				}
			} else {
				ptr_float = tampon->t.rval + j;
				for(i=0; i<nbpts; i++) {
					*ptr_temps++ = (fftw_real)*ptr_float++;
					if(++j >= tampon->max) { j = 0; ptr_float = tampon->t.rval; }
				}
			}
			rfftw_one(CalcPlanStd.plan,CalcPlanStd.temps,CalcFftSolo);
			ptr_fft = CalcPlanStd.freq;
			if(mode == FFT_PUISSANCE) {
				ptr_mont = CalcFftSolo; ptr_desc = CalcFftSolo + nbpts;
				*ptr_fft += (*ptr_mont * *ptr_mont); i = 1; ptr_fft++;
				for( ; i<(nbpts + 1) / 2; i++) {
					--ptr_desc;
					*ptr_fft += ((*ptr_mont * *ptr_mont) + (*ptr_desc * *ptr_desc));
					ptr_fft++; ptr_mont++;
				}
				if(!(nbpts % 2)) *ptr_fft += (*ptr_mont * *ptr_mont);
			} else {
				ptr_solo = CalcFftSolo;
				for(i=0; i<nbpts; i++) { *ptr_fft += *ptr_solo++; ptr_fft++; }
			}
			nb += 1.0;
		}
		if(mode == FFT_MOYENNEE) {
			ptr_fft = CalcPlanStd.freq; for(i=0; i<nbpts; i++) { *ptr_fft /= nb; ptr_fft++; }
		} else if (mode == FFT_PUISSANCE) {
			normalisation = (VoieEvent[voie].horloge / 1000.0) / (float)nbpts; /* secondes, soit 1/norme en Hz */
			ptr_fft = CalcPlanStd.freq;
			for(i=0; i<(nbpts / 2) + 1; i++) {
				spectre[i] = VoieManip[voie].ADUennV * sqrtf((*ptr_fft / nb) * normalisation);
				ptr_fft++;
			}
			return(1);
		}
	}

	normalisation = (VoieEvent[voie].horloge / 1000.0) / (float)nbpts; /* secondes, soit 1/norme en Hz */
	CalculePuissance(CalcPlanStd.freq,spectre,nbpts,normalisation,VoieManip[voie].ADUennV);
//	if(pt0 < 10000) {
//      int nbfreq;
//		printf("Voie %s: 1 ADU = %g mV x gain ampli=%g x variable=%g\n    ... soit 1 ADU = %g nV dans le detecteur\n",
//			VoieManip[voie].nom,VoieManip[voie].ADUenmV,VoieManip[voie].gain_fixe,VoieManip[voie].gain_variable,unite);
//      nbfreq = (nbpts / 2) + 1;
//		printf("%4d points dans %08X",nbfreq,(hexa)spectre);
//		for(i=0; i<128; i++) {
//			if(!(i % 10)) printf("\n%4d: ",i);
//			printf(" %10g",spectre[i]);
//		}
//		printf("\n");
//	}
	return(1);
}
/* ========================================================================== */
int CalculeFftDetec() { return(0); }
#ifdef SPECTRES_SEQUENCES
/* ========================================================================== */
int CalcSpectreAutoParms() {
	int phys;
	
	if(OpiumExec(pCalcSpectreAuto->cdr) == PNL_CANCEL) return(0);
	for(phys=0; phys<ModelePhysNb; phys++) CalcSpectreAuto[phys].nbfreq = (CalcSpectreAuto[phys].dim / 2) + 1;
	if(OpiumExec(pCalcSpectreBolo->cdr) == PNL_CANCEL) return(0);
	return(1);
}
/* ========================================================================== */
#endif /* SPECTRES_SEQUENCES */

int LectSpectresAutoMesure(),LectStop();
int CalcSpectreAutoAffiche(),CalcSpectreAutoSauve(),CalcSpectreAutoEfface(),CalcSpectreAutoRetrouve();
TypeMenuItem iCalcSpectreControle[] = {
	{ "Mesurer  ",  MNU_FONCTION LectSpectresAutoMesure },
	{ "Afficher ",  MNU_FONCTION CalcSpectreAutoAffiche },
	{ "Stopper  ",  MNU_FONCTION LectStop },
	{ "Sauver   ",  MNU_FONCTION CalcSpectreAutoSauve },
	{ "Effacer  ",  MNU_FONCTION CalcSpectreAutoEfface },
	{ "Retrouver",  MNU_FONCTION CalcSpectreAutoRetrouve },
	MNU_END
};
/* ========================================================================== */
void CalcSpectreAutoPlanche(Cadre planche, int *xa, int *ya) {
	Menu m; Panel p;
	int bolo,local,nb,phys,ligs; char entete;
	int x,y,ys;

	x = *xa; y = *ya;
	
	local = 0; // GCC
	nb = 0; for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].local) { if(++nb == 1) local = bolo; } else CalcAutoBolo[bolo] = 0;
	if(nb > 1) {
		p = PanelCreate(BoloNb); PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
		for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].local) {
			CalcAutoBolo[bolo] = Bolo[bolo].lu;
			PanelItemColors(p,PanelBool(p,Bolo[bolo].nom,&CalcAutoBolo[bolo]),SambaRougeVert,2);
		}
		BoardAddPanel(planche,p,x,y,0); x += (15 * Dx);
	} else if(nb == 1) CalcAutoBolo[local] = 1;
	/* else bouton pour demander de rendre des bolos locaux */
	
	if(ModelePhysNb > 1) {
		p = PanelCreate(ModelePhysNb); PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
		for(phys=0; phys<ModelePhysNb; phys++) {
			CalcAutoPhys[phys] = CalcSpectreAuto[phys].dim? 1: 0;
			PanelItemColors(p,PanelBool(p,ModelePhys[phys].nom,&CalcAutoPhys[phys]),SambaRougeVert,2);
		}
		BoardAddPanel(planche,p,x,y,0); x += (15 * Dx);
	} else if(ModelePhysNb == 1) CalcAutoPhys[0] = CalcSpectreAuto[0].dim? 1: 0;
	
	p = PanelCreate(8); PanelSupport(p,WND_CREUX);
	PanelList(p,L_("Donnees"),CalcDonneesName,&CalcDonnees,12);
	PanelKeyB(p,L_("Trmt au vol"),L_("sans/inclus"),&CalcSpectreAutoAuVol,8);
	PanelKeyB(p,L_("Pseudo voies"),L_("exclues/aussi"),&CalcSpectreAutoPseudo,8);
	PanelKeyB(p,L_("Invalidees"),L_("exclues/aussi"),&CalcSpectreAutoInval,8);
	PanelKeyB(p,L_("Mode"),L_("somme/trigge/glissant"),&CalcModeAuto,8);
	PanelInt (p,L_("Nb spectres"),&CalcSpectresMax);
	PanelInt (p,L_("Largeur"),&CalcSpectreAutoDimX);
	PanelInt (p,L_("Hauteur"),&CalcSpectreAutoDimY);
	BoardAddPanel(planche,p,x + (5 * Dx),y,0);
	ys = y + ((PanelItemNb(p) + 2) * Dy);
	
	p = PanelCreate(1); PanelMode(p,PNL_DIRECT); PanelSupport(p,WND_CREUX);
	PanelText(p,"Note",CalcSpectreAutoNote,NOTE_MAX);
	BoardAddPanel(planche,p,x /* + (5 * Dx) */,ys,0);
	ys = ys + ((PanelItemNb(p) + 1) * Dy);
	*xa = x + ((6 + NOTE_MAX) * Dx);
	
	m = mCalcSpectreControle = MenuLocalise(iCalcSpectreControle); OpiumSupport(m->cdr,WND_PLAQUETTE);
	MenuTitle(m,"Generation de spectres");
	MenuItemAllume(mCalcSpectreControle,1,L_("Mesurer  "),GRF_RGB_YELLOW);
	BoardAddMenu(planche,m,x + (36 * Dx),y,0);
	
	iCalcAvancement = InstrumFloat(INS_COLONNE,(void *)&CalcNiveau,&CalcAvance,0.0,100.0); InstrumSupport(iCalcAvancement,WND_CREUX);
	BoardAddInstrum(planche,iCalcAvancement,x + (50 * Dx),y,0);
	
	y = ys;
	ligs = 11; if(ModelePhysNb > 1) ligs += 1;
	p = PanelCreate(ligs * ModelePhysNb); PanelSupport(p,WND_CREUX); PanelColumns(p,ModelePhysNb);
	for(phys=0; phys<ModelePhysNb; phys++) {
		if(ModelePhysNb > 1) PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,ModelePhys[phys].nom,MODL_NOM),0),1);
		entete = (phys == 0);
		PanelInt   (p,entete? L_("Nombre de points"): 0,&CalcSpectreAuto[phys].dim);
		PanelRemark(p,entete? L_("-- Axe des X --            ."): 0);
		PanelKeyB  (p,entete? L_("type"): 0,L_("lineaire/log"),&CalcSpectreAuto[phys].parms[0].log,10);
		PanelKeyB  (p,entete? L_("limites"): 0,L_("manu/auto"),&CalcSpectreAuto[phys].parms[0].autom,6);
		PanelFloat (p,entete? L_("min si manu"): 0,&CalcSpectreAuto[phys].parms[0].lim.r.min);
		PanelFloat (p,entete? L_("max si manu"): 0,&CalcSpectreAuto[phys].parms[0].lim.r.max);
		PanelRemark(p,entete? L_("-- Axe des Y --            ."): 0);
		PanelKeyB  (p,entete? L_("type"): 0,L_("lineaire/log"),&CalcSpectreAuto[phys].parms[1].log,10);
		PanelKeyB  (p,entete? L_("limites"): 0,L_("manu/auto"),&CalcSpectreAuto[phys].parms[1].autom,6);
		PanelFloat (p,entete? L_("min si manu"): 0,&CalcSpectreAuto[phys].parms[1].lim.r.min);
		PanelFloat (p,entete? L_("max si manu"): 0,&CalcSpectreAuto[phys].parms[1].lim.r.max);
	}
	BoardAddPanel(planche,p,x,y,0); y += ((ligs + 2) * Dy);
	
	x += (35 * Dx);
	BoardAddBouton(planche,"Enregistrer",CalculeSauve,x,y,0);
	/* *xa = x + (12 * Dx); */ *ya = y + Dy;
	
}
/* ========================================================================== */
void CalcSpectreAutoVide() {
	int x,y,nbfreq;

	CalcSpectreFenNb = 0;
	gCalcSpectreAutoDefaut = GraphCreate(CalcSpectreAutoDimX,CalcSpectreAutoDimY,1+PHYS_TYPES);
	GraphAxisTitle(gCalcSpectreAutoDefaut,GRF_XAXIS,L_("Frequence (kHz)"));
	GraphAxisTitle(gCalcSpectreAutoDefaut,GRF_YAXIS,L_("Bruit (nV/Rac(Hz))"));
	GraphMode(gCalcSpectreAutoDefaut,GRF_2AXES | GRF_GRID | GRF_LEGEND);
	CalcSpectreAuto[0].ab6[0] = 0.0;
	CalcSpectreAuto[0].ab6[1] = Echantillonnage / CalcSpectreAuto[0].dim; /* kHz */
	nbfreq = CalcSpectreAuto[0].nbfreq;
	x = GraphAdd(gCalcSpectreAutoDefaut,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,CalcSpectreAuto[0].ab6,nbfreq);
	GraphDataName(gCalcSpectreAutoDefaut,x,"f(kHz)");
	y = GraphAdd(gCalcSpectreAutoDefaut,GRF_FLOAT|GRF_YAXIS,0,nbfreq);
	GraphAxisLog(gCalcSpectreAutoDefaut,GRF_XAXIS,1);
	GraphAxisFloatRange(gCalcSpectreAutoDefaut,GRF_XAXIS,CalcSpectreAuto[0].parms[0].lim.r.min,CalcSpectreAuto[0].parms[0].lim.r.max);
	GraphAxisLog(gCalcSpectreAutoDefaut,GRF_YAXIS,1);
	GraphAxisFloatRange(gCalcSpectreAutoDefaut,GRF_YAXIS,CalcSpectreAuto[0].parms[1].lim.r.min,CalcSpectreAuto[0].parms[1].lim.r.max);
	gCalcSpectreAffiche[CalcSpectreFenNb++] = gCalcSpectreAutoDefaut;
}
/* ========================================================================== */
int CalcSpectreAutoConstruit() {
	int bolo,cap,c,voie,prec,phys; int dim,nbfreq;
	int x,y,k,posx,posy,nexty; char titre[80];
	
//	printf("(%s) Construction des graphiques\n",__func__);
	posx = 10; posy = 50;
	CalcSpectreFenNb = 0;
	CalcSpectreAutoDuree = 0;

	for(bolo=0; bolo<BoloNb; bolo++) if(CalcAutoBolo[bolo]) {
		for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
			voie = Bolo[bolo].captr[cap].voie;
			if(!LectCompacteUtilisee && !VoieTampon[voie].lue) continue;
			phys = ModeleVoie[VoieManip[voie].type].physique;
			for(c=0; c<cap; c++) {
				prec = Bolo[bolo].captr[c].voie;
				if(ModeleVoie[VoieManip[prec].type].physique == phys) break;
			}
			if(c < cap) continue; // type <ModelePhys[phys].nom> deja pris en compte via la voie <VoieManip[prec].nom>
			if(CalcAutoPhys[phys] && ((dim = CalcSpectreAuto[phys].dim) > 0)) {
				if(dim > CalcSpectreAutoDuree) CalcSpectreAutoDuree = dim;
				if((CalcSpectreSauveMontre == CALC_CUMULE) && gCalcSpectreBolo[bolo][phys]) {
					for(c=cap; c<Bolo[bolo].nbcapt; c++) {
						voie = Bolo[bolo].captr[c].voie;
						if(!VoieTampon[voie].lue) continue;
						if(ModeleVoie[VoieManip[voie].type].physique == phys) {
							y = GraphAdd(gCalcSpectreBolo[bolo][phys],GRF_FLOAT|GRF_YAXIS,CalcAutoSpectreMoyenne[voie],CalcSpectreAuto[phys].nbfreq);
							k = OpiumColorNext(&(oCouleur[bolo][phys]));
							GraphDataRGB(gCalcSpectreBolo[bolo][phys],y,OpiumColorRGB(k));
							GraphDataName(gCalcSpectreBolo[bolo][phys],y,VoieManip[voie].nom);
							gCalcSpectreY[bolo][phys] = y;
						}
					}
					gCalcSpectreAffiche[CalcSpectreFenNb++] = gCalcSpectreBolo[bolo][phys];
				} else {
					if(!gCalcSpectreBolo[bolo][phys]) {
						// printf("(%s) Creation du graphique des spectres moyennes: voies %s %s",__func__,ModelePhys[phys].nom,Bolo[bolo].nom);
						if(gCalcSpectreAutoPrems) {
							OpiumGetSize(gCalcSpectreAutoPrems->cdr,&CalcSpectreAutoDimX,&CalcSpectreAutoDimY);
							BoardRefreshVars(bLectSpectres);
						}
						gCalcSpectreBolo[bolo][phys] = GraphCreate(CalcSpectreAutoDimX,CalcSpectreAutoDimY,1+PHYS_TYPES);
						if(!gCalcSpectreAutoPrems) gCalcSpectreAutoPrems = gCalcSpectreBolo[bolo][phys];
					} else GraphErase(gCalcSpectreBolo[bolo][phys]);
					if(Bolo[bolo].nbcapt > 1)
						sprintf(titre,L_("Spectres de bruit des voies %s %s"),ModelePhys[phys].nom,Bolo[bolo].nom);
					else sprintf(titre,L_("Spectre de bruit de %s"),Bolo[bolo].nom);
					if(CalcSpectreAutoNote[0]) strcat(strcat(strcat(titre," ("),CalcSpectreAutoNote),")");
					OpiumTitle(gCalcSpectreBolo[bolo][phys]->cdr,titre);
					GraphAxisTitle(gCalcSpectreBolo[bolo][phys],GRF_XAXIS,L_("Frequence (kHz)"));
					GraphAxisTitle(gCalcSpectreBolo[bolo][phys],GRF_YAXIS,L_("Bruit (nV/Rac(Hz))"));
					if(Bolo[bolo].nbcapt > 1) GraphMode(gCalcSpectreBolo[bolo][phys],GRF_2AXES | GRF_GRID | GRF_LEGEND);
					else GraphMode(gCalcSpectreBolo[bolo][phys],GRF_2AXES | GRF_GRID);
					nexty = posy + CalcSpectreAutoDimY + 40;
					if(nexty > OpiumServerHeigth(0)) { posx += (CalcSpectreAutoDimX + 20); posy = 50; nexty = posy + CalcSpectreAutoDimY + 40; }
					OpiumPosition(gCalcSpectreBolo[bolo][phys]->cdr,posx,posy);
					posy = nexty;
					OpiumColorInit(&(oCouleur[bolo][phys]));
					CalcSpectreAuto[phys].ab6[0] = 0.0;
					CalcSpectreAuto[phys].ab6[1] = 1.0 / (VoieEvent[voie].horloge * CalcSpectreAuto[phys].dim); /* kHz */
					//				printf("(%s) dF(%s) = 1/(%g ms x %d) = %g kHz\n",__func__,ModelePhys[phys].nom,VoieEvent[voie].horloge,CalcSpectreAuto[phys].dim,CalcSpectreAuto[phys].ab6[1]);
					nbfreq = CalcSpectreAuto[phys].nbfreq;
					x = GraphAdd(gCalcSpectreBolo[bolo][phys],GRF_FLOAT|GRF_INDEX|GRF_XAXIS,CalcSpectreAuto[phys].ab6,nbfreq);
					GraphDataUse(gCalcSpectreBolo[bolo][phys],x,nbfreq-1); GraphDataRescroll(gCalcSpectreBolo[bolo][phys],x,1);
					GraphDataName(gCalcSpectreBolo[bolo][phys],x,"f(kHz)");
					for(c=cap; c<Bolo[bolo].nbcapt; c++) {
						voie = Bolo[bolo].captr[c].voie;
						//					printf("(%s) %s (type %s): %s\n",__func__,VoieManip[voie].nom,ModelePhys[ModeleVoie[VoieManip[voie].type].physique].nom,
						//						   VoieTampon[voie].lue? "lue": "ignoree");
						if(!VoieTampon[voie].lue) continue;
						if(ModeleVoie[VoieManip[voie].type].physique == phys) {
							y = GraphAdd(gCalcSpectreBolo[bolo][phys],GRF_FLOAT|GRF_YAXIS,CalcAutoSpectreMoyenne[voie],CalcSpectreAuto[phys].nbfreq);
							GraphDataUse(gCalcSpectreBolo[bolo][phys],y,nbfreq-1); GraphDataRescroll(gCalcSpectreBolo[bolo][phys],y,1);
							k = OpiumColorNext(&(oCouleur[bolo][phys]));
							GraphDataRGB(gCalcSpectreBolo[bolo][phys],y,OpiumColorRGB(k));
							GraphDataName(gCalcSpectreBolo[bolo][phys],y,VoieManip[voie].nom);
							//						printf("(%s) %s ajoutee au graphique des %ss\n",__func__,VoieManip[voie].nom,ModelePhys[phys].nom);
							gCalcSpectreY[bolo][phys] = y;
						}
					}
				#ifdef SIMPLIFIE
					if(CalcSpectreAuto[phys].parms[0].log) GraphAxisLog(gCalcSpectreBolo[bolo][phys],GRF_XAXIS,1);
					else GraphAxisLog(gCalcSpectreBolo[bolo][phys],GRF_XAXIS,0);
					if(CalcSpectreAuto[phys].parms[0].autom) {
						GraphAxisAutoRange(gCalcSpectreBolo[bolo][phys],GRF_XAXIS);
						if(CalcSpectreAuto[phys].parms[0].log) GraphAxisFloatMin(gCalcSpectreBolo[bolo][phys],GRF_XAXIS,0.1);
						else GraphAxisFloatMin(gCalcSpectreBolo[bolo][phys],GRF_XAXIS,0.0);
					} else GraphAxisFloatRange(gCalcSpectreBolo[bolo][phys],GRF_XAXIS,CalcSpectreAuto[phys].parms[0].lim.r.min,CalcSpectreAuto[phys].parms[0].lim.r.max);
					if(CalcSpectreAuto[phys].parms[1].log) GraphAxisLog(gCalcSpectreBolo[bolo][phys],GRF_YAXIS,1);
					else GraphAxisLog(gCalcSpectreBolo[bolo][phys],GRF_YAXIS,0);
					if(CalcSpectreAuto[phys].parms[1].autom) {
						GraphAxisAutoRange(gCalcSpectreBolo[bolo][phys],GRF_YAXIS);
						if(CalcSpectreAuto[phys].parms[1].log) GraphAxisFloatMin(gCalcSpectreBolo[bolo][phys],GRF_YAXIS,1000.0);
						else GraphAxisFloatMin(gCalcSpectreBolo[bolo][phys],GRF_YAXIS,0.0);
					} else GraphAxisFloatRange(gCalcSpectreBolo[bolo][phys],GRF_YAXIS,CalcSpectreAuto[phys].parms[1].lim.r.min,CalcSpectreAuto[phys].parms[1].lim.r.max);
				#else
					GraphAjusteParms(gCalcSpectreBolo[bolo][phys],GRF_XAXIS,&(CalcSpectreAuto[phys].parms[0]));
					GraphAjusteParms(gCalcSpectreBolo[bolo][phys],GRF_YAXIS,&(CalcSpectreAuto[phys].parms[1]));
				#endif
					if(!LectCompacteUtilisee) OpiumDisplay(gCalcSpectreBolo[bolo][phys]->cdr);
					gCalcSpectreAffiche[CalcSpectreFenNb++] = gCalcSpectreBolo[bolo][phys];
					//				printf("%s/ Spectres pour les voies %s de %s affiches sur la fenetre #%d\n",DateHeure(),ModelePhys[phys].nom,Bolo[bolo].nom,CalcSpectreFenNb);
				}
			} else if(gCalcSpectreBolo[bolo][phys] && !LectCompacteUtilisee) {
				if(OpiumDisplayed(gCalcSpectreBolo[bolo][phys]->cdr)) OpiumClear(gCalcSpectreBolo[bolo][phys]->cdr);
			}
		}
	}
	
//	printf("%s/ %d fenetre%s de spectres affichee%s\n",DateHeure(),Accord2s(CalcSpectreFenNb));
	if(!CalcSpectreFenNb) { OpiumFail("Pas de spectre calculable"); return(0); }
	// printf("(%s) CalcSpectreFenNb = %d\n",__func__,CalcSpectreFenNb);
	OpiumUserAction();
	return(1);
}
/* ========================================================================== */
int CalcSpectreAutoAffiche() {
	int i;
	
	for(i=0; i<CalcSpectreFenNb; i++) {
		if(!LectCompacteUtilisee) OpiumDisplay(gCalcSpectreAffiche[i]->cdr);
		else OpiumRefresh(gCalcSpectreAffiche[i]->cdr);
	}
	
	return(0);
}
/* ========================================================================== */
int CalcSpectreAutoEfface() {
	int bolo,cap,voie,phys;
	int i;
	
	if(!LectCompacteUtilisee) {
//		for(bolo=0; bolo<BoloNb; bolo++) {
//			for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
//				voie = Bolo[bolo].captr[cap].voie;
//				phys = ModeleVoie[VoieManip[voie].type].physique;
//				if(gCalcSpectreBolo[bolo][phys]) {
//					if(OpiumDisplayed(gCalcSpectreBolo[bolo][phys]->cdr)) OpiumClear(gCalcSpectreBolo[bolo][phys]->cdr);
//				}
//			}
//		}
		for(i=0; i<CalcSpectreFenNb; i++) if(gCalcSpectreAffiche[i]) {
			if(OpiumDisplayed(gCalcSpectreAffiche[i]->cdr)) OpiumClear(gCalcSpectreAffiche[i]->cdr);
		}
	} else {
		for(bolo=0; bolo<BoloNb; bolo++) {
			for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
				voie = Bolo[bolo].captr[cap].voie;
				phys = ModeleVoie[VoieManip[voie].type].physique;
				gCalcSpectreBolo[bolo][phys] = 0;
			}
		}
		for(i=0; i<CalcSpectreFenNb; i++) gCalcSpectreAffiche[i] = 0;
		CalcSpectreFenNb = 0;
		printf("(%s) CalcSpectreFenNb = %d\n",__func__,CalcSpectreFenNb);
	}

	return(0);
}
/* ========================================================================== */
int CalcSpectreAutoRetrouve(Menu menu, MenuItem item) {
#define SPECTRES_SAUVES_MAX 1024
	char *fichiers[SPECTRES_SAUVES_MAX],*date[SPECTRES_SAUVES_MAX]; int nb;
	int i,n; char commande[256];
	
	RepertoireListeCree(0,ResuPath,fichiers,SPECTRES_SAUVES_MAX,&nb);
	n = 0;
	for(i=0; i<nb; i++) {
		if(!strncmp(fichiers[i],"Spectres_",9)) date[n++] = fichiers[i] + 9;
	}
	date[n] = "\0";
	i = n - 1;
	if(OpiumReadList("Date",date,&i,13) == PNL_OK) {
		sprintf(commande,"open -a /Applications/Preview.app %sSpectres_%s &",ResuPath,date[i]);
		system(commande);
	}
	RepertoireListeLibere(fichiers,nb);
	
	return(0);
}
/* ========================================================================== */
int CalcSpectreAutoSauve() {
	int bolo,cap,voie,vm,phys,type_bn,c,prec,mesure;
	int dim,nbfreq;
	char automate_lu;
	char commande[256],raison[256],legende[80];
	int i,k,l,y,num,nb,lngr; int64 lus; // int64 premier;
	float fl,fh,larg;
	char b; int trait_premier,trait_dernier;

	if(CalcSpectreSauveNom[0] == '\0') {
		if(OpiumExec(pCalcSpectreSauve->cdr) == PNL_CANCEL) return(0);
	} else {
		if(OpiumExec(pCalcSpectreFerme->cdr) == PNL_CANCEL) return(0);
	}
	lus = 0; mesure = 0;

	if(CalcSpectreSauvePDF) {
		if(CalcSpectreSauveNom[0] == 0) {
			sprintf(CalcSpectreSauveNom,"%sSpectres_%lld",ResuPath,DateLong());
			printf("* Impression des spectres dans %s\n",CalcSpectreSauveNom);
			RepertoireCreeRacine(CalcSpectreSauveNom);
			
			ImpressionSurSupport(IMPRESSION_FICHIER,CalcSpectreSauveNom);
			if(!ImpressionPrete("portrait")) return(0);
			
			nb = 0; for(bolo=0; bolo<BoloNb; bolo++) if(CalcAutoBolo[bolo]) { if(++nb == 1) mesure = bolo; }
			// ImprimeNouvelleLigne((ImpressionHauteurCourante() / 2) - 5);
			ImprimeNouvelleLigne(30);
			trait_dernier = 90; // ImpressionLargeurCourante();
			trait_premier = 20; lngr = trait_dernier - (2 * trait_premier);
			ImprimeLimite(trait_premier,lngr,0); ImprimeColonnes(2,trait_premier,trait_premier+lngr);
			ImprimeNouvelleLigne(2);
			l = (int)strlen(SambaIntitule); k = (trait_dernier - l) / 2;
			ImprimeEnCol(k,SambaIntitule); ImprimeFond(k,l,0xFFFF,0xF000,0xD000);
			ImprimeNouvelleLigne(2);
			k = (trait_dernier - 22) / 2;
			if(nb > 1) {
				ImprimeEnCol(k,L_("Spectres des detecteurs:"));
				ImprimeNouvelleLigne(1);
				k = (trait_dernier - 32) / 2;
				num = 2;
				for(bolo=0; bolo<BoloNb; bolo++) if(CalcAutoBolo[bolo]) {
					ImprimeNouvelleLigne(1);
					ImprimeEnCol(k,"%10s  .  .  .  . : page %2d",Bolo[bolo].nom,num++);
				}
			} else {
				ImprimeEnCol(k,L_("Spectres du detecteur "),Bolo[mesure].nom);
				ImprimeNouvelleLigne(1);
			}
			ImprimeNouvelleLigne(3);
			k = trait_premier + 2;
			if(CalcSpectreAutoNote[0] && (CalcSpectreSauveMontre != CALC_CONTINUE)) {
				ImprimeEnCol(k,"Note: "); // ,CalcSpectreAutoNote);
				l = (int)strlen(CalcSpectreAutoNote);
				i = 0;
				do {
					c = lngr - 10;
					if((i + c) < l) { while(--c >= 0) if(CalcSpectreAutoNote[i + c] == ' ') break; }
					else c = l - i;
					if(c >= 0) {
						CalcSpectreAutoNote[i + c] = '\0';
						ImprimeEnCol(k+6,CalcSpectreAutoNote+i);
						i = i + c + 1;
					} else {
						c = lngr - 10;
						b = CalcSpectreAutoNote[i + c];
						CalcSpectreAutoNote[i + c] = '\0';
						ImprimeEnCol(k+6,CalcSpectreAutoNote+i);
						CalcSpectreAutoNote[i + c] = b;
						i = i + c;
					}
					ImprimeNouvelleLigne(1);
					while(i < l) { if(CalcSpectreAutoNote[i] != ' ') break; i++; }
				} while(i < l);
			}
			ImprimeLimite(trait_premier,lngr,1);
			ImprimeNouvelleLigne(2);
			k = (trait_dernier - 34) / 2;
			ImprimeEnCol(k,L_("Cahier cree le %s a %s"),DateCivile(),DateHeure());
			ImprimeNouvelleLigne(1);
			ImprimeLimite(trait_premier,lngr,1);
			ImprimeColonnes(0);
			ImpressionFerme();
		}

		if(CalcSpectreSauveMontre == CALC_CUMULE) {
			for(bolo=0; bolo<BoloNb; bolo++) if(CalcAutoBolo[bolo]) {
				for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
					voie = Bolo[bolo].captr[cap].voie;
					if(!LectCompacteUtilisee && !VoieTampon[voie].lue) continue;
					phys = ModeleVoie[VoieManip[voie].type].physique;
					for(c=0; c<cap; c++) {
						prec = Bolo[bolo].captr[c].voie;
						if(ModeleVoie[VoieManip[prec].type].physique == phys) break;
					}
					if(c < cap) continue;
					if(CalcAutoPhys[phys] && ((dim = CalcSpectreAuto[phys].dim) > 0) && gCalcSpectreBolo[bolo][phys]) {
						GraphMode(gCalcSpectreBolo[bolo][phys],GRF_2AXES | GRF_GRID | GRF_LEGEND);
						y = gCalcSpectreY[bolo][phys];
						strcpy(legende,VoieManip[voie].nom);
						if(OpiumReadText(L_("Legende"),legende,80)) GraphDataName(gCalcSpectreBolo[bolo][phys],y,legende);
						else GraphDataName(gCalcSpectreBolo[bolo][phys],y,VoieManip[voie].nom);
						CalcAutoSpectreMoyenne[voie] = 0;
						OpiumRefresh(gCalcSpectreBolo[bolo][phys]->cdr);
					}
				}
			}
		} else {
			if(!OpiumPSInit(CalcSpectreSauveNom,1)) return(0);
			// OpiumPSformat(OPIUM_PS_A4);
			fl = (21.0 - ((float)OPIUM_PSMARGE_X / WND_PIXEL_CM)) / (float)CalcSpectreAutoDimX;
			fh = (29.7 - ((float)OPIUM_PSMARGE_Y / WND_PIXEL_CM)) / (float)CalcSpectreAutoDimY / 2.0;
			larg = 20.0; if((fl > fh) && (fl > 0.0)) larg *= (fh / fl);
			OpiumPSlargeur(larg);
			
			for(bolo=0; bolo<BoloNb; bolo++) if(CalcAutoBolo[bolo]) {
				OpiumPSentete(Bolo[bolo].nom);
				OpiumPSNouvellePage();
				for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
					voie = Bolo[bolo].captr[cap].voie;
					phys = ModeleVoie[VoieManip[voie].type].physique;
					for(c=0; c<cap; c++) {
						prec = Bolo[bolo].captr[c].voie;
						if(ModeleVoie[VoieManip[prec].type].physique == phys) break;
					}
					if(c < cap) continue;
					if(gCalcSpectreBolo[bolo][phys]) OpiumPSRecopie(gCalcSpectreBolo[bolo][phys]->cdr);
				}
			}
			OpiumPSClose();
			OpiumPSformat(OPIUM_PS_ECRAN);
			for(bolo=0; bolo<BoloNb; bolo++) if(CalcAutoBolo[bolo]) {
				for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
					voie = Bolo[bolo].captr[cap].voie;
					phys = ModeleVoie[VoieManip[voie].type].physique;
					for(c=0; c<cap; c++) {
						prec = Bolo[bolo].captr[c].voie;
						if(ModeleVoie[VoieManip[prec].type].physique == phys) break;
					}
					if(c < cap) continue;
					if(gCalcSpectreBolo[bolo][phys]) OpiumRefresh(gCalcSpectreBolo[bolo][phys]->cdr);
				}
			}
			if(CalcSpectreSauveMontre > CALC_CONTINUE) {
				ImpressionPStoPDF(CalcSpectreSauveNom); // supprime le .ps
				if(CalcSpectreSauveMontre == CALC_MONTRE) {
					sprintf(commande,"open -a /Applications/Preview.app %s &",CalcSpectreSauveNom);
					system(commande);
				}
				OpiumNote("Spectres disponibles dans %s",CalcSpectreSauveNom);
				CalcSpectreSauveNom[0] = 0;
			}
		}
	}

	if(CalcSpectreSauveDB) {
		TypeTamponDonnees *tampon;
		TypeDonnee *ptr_int16,*tampon_sauve,*ptr_sauve;
		TypeSignal *ptr_float,signal;

		tampon = 0; /* gcc */
		AutomDataQuery(raison); automate_lu = 0;
		for(bolo=0; bolo<BoloNb; bolo++) if(CalcAutoBolo[bolo]) {
			for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
				voie = Bolo[bolo].captr[cap].voie;
				vm = VoieManip[voie].type;
				phys = ModeleVoie[vm].physique;
				if(!(dim = CalcSpectreAuto[phys].dim)) continue;
				tampon = &(VoieTampon[voie].trmt[CalcDonnees].donnees);
				if(CalcDonnees == TRMT_AU_VOL) {
					lus = VoieTampon[voie].lus; // premier = VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien;
				} else if(CalcDonnees == TRMT_PRETRG) {
					lus = tampon->prem + tampon->nb; // premier = tampon->prem;
				}
				if(dim > tampon->max) dim = tampon->max;
				nbfreq = CalcSpectreAuto[phys].nbfreq;
				lngr = tampon->max;
				nb = dim; if(nb > lngr) nb = lngr;
				k = Modulo(lus - nb,lngr);
				tampon_sauve = (TypeDonnee *)malloc(nb * sizeof(TypeDonnee));
				if(tampon_sauve) {
					ptr_sauve = tampon_sauve;
					if(tampon->type == TAMPON_INT16) {
						ptr_int16 = tampon->t.sval + k;
						for(i=0; i<nb; i++) {
							*ptr_sauve++ = *ptr_int16++;
							if(++k >= lngr) { k = 0; ptr_int16 = tampon->t.sval; }
						} 
					} else {
						ptr_float = tampon->t.rval + k;
						for(i=0; i<nb; i++) {
							signal = *ptr_float++;
							*ptr_sauve++ = (TypeDonnee)((signal >= 0.0)? ((signal < 32767.0)? signal: 0x7FFF): ((signal > -32768.0)? signal: 0xFFFF));
							if(++k >= lngr) { k = 0; ptr_float = tampon->t.rval; }
						}
					}
				} else {
					OpiumFail("Manque %d octets pour sauver les donnees de %s\n",nb * sizeof(TypeDonnee),VoieManip[voie].nom);
					nb = 0;
				}
				if(!automate_lu) {
					if(AutomDataGet(raison)) automate_lu = 1;
					else printf("%s/ %s: %s\n",DateHeure(),__func__,raison);
				}
				strcpy(EdwSpBruitInfo.nom,VoieManip[voie].nom);
				EdwSpBruitInfo.type = CalcDonnees;
	//			EdwSpBruitInfo.au_vol = VoieManip[voie].def.trmt[TRMT_AU_VOL].type;
	//			EdwSpBruitInfo.sur_tampon = VoieManip[voie].def.trmt[TRMT_PRETRG].type;
				ArgKeyGetText(TrmtTypeCles,VoieManip[voie].def.trmt[TRMT_AU_VOL].type,EdwSpBruitInfo.au_vol,MAXTRMTNOM);
				ArgKeyGetText(TrmtTypeCles,VoieManip[voie].def.trmt[TRMT_PRETRG].type,EdwSpBruitInfo.sur_tampon,MAXTRMTNOM);
				type_bn = Numeriseur[Bolo[bolo].captr[cap].bb.num].def.type;
				if(type_bn < ModeleBNNb) strcpy(EdwSpBruitInfo.type_bn,ModeleBN[type_bn].nom);
				else sprintf(EdwSpBruitInfo.type_bn,"No%d",type_bn);
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
				EdwSpBruitInfo.nbtemps = nb;
				EdwSpBruitInfo.temps = tampon_sauve;
				EdwSpBruitInfo.nbfreq = nbfreq;
				EdwSpBruitInfo.freq = CalcAutoSpectreMoyenne[voie];
				EdwDbPrint(DbazPath,VoieManip[voie].det,VoieManip[voie].type,&EdwSpBruit);
				if(tampon_sauve) free(tampon_sauve);
			}
		}
		OpiumNote("Spectres sauves dans le repertoire %s",DbazPath);
	}

	return(0);
}
/* ========================================================================== */
int CalculeFftCree() {
	int voie,bolo,vm,cap;
	int dim,nb;
	char raison[80];
	int nbfreq,i,j,k,n; int x,y;
	char titre[80];

	bolo = CalcBolo; vm = ModlNum; voie = 0;
	if(CalcMax == 1) {
		if(OpiumExec(pCalcSpectre1->cdr) == PNL_CANCEL) return(0);
		CalcTi[0] = (double)CalcT0;
	} else {
		if(OpiumExec(pCalcSpectreN->cdr) == PNL_CANCEL) return(0);
	}
	if(Bolo[CalcBolo].lu == DETEC_HS) {
		OpiumError("Le detecteur %s n'est pas lu",Bolo[CalcBolo].nom);
		CalcBolo = bolo; ModlNum = vm;
		return(0);
	}
	for(cap=0; cap<Bolo[CalcBolo].nbcapt; cap++) {
		voie = Bolo[CalcBolo].captr[cap].voie;
		if(ModlNum == VoieManip[voie].type) break;
	}
	if(cap == Bolo[CalcBolo].nbcapt) {
		OpiumError("Pas de voie '%s' pour le detecteur '%s'",ModeleVoie[ModlNum].nom,Bolo[CalcBolo].nom);
		CalcBolo = bolo; ModlNum = vm;
		return(0);
	}
	VoieNum = voie;
	if(CalcSpectreNb >= MAXSPECTRES) {
		OpiumError("Deja %d spectres stockes!",CalcSpectreNb);
		return(0);
	}
	if(CalcFenNb >= MAXCALCFEN) {
		OpiumError("Deja %d fenetres de spectres!",CalcFenNb);
		return(0);
	}
	if(CalcDonnees < TRMT_NBCLASSES) {
		if(CalcMax == 1) CalcPi[0] = (int64)((CalcTi[0] * 1000.0) / (double)VoieEvent[VoieNum].horloge);
		if(CalcDim <= 0) {
			OpiumError("Nombre d'echantillons invalide: %d <= 0",CalcDim);
			return(0);
		}
		if(CalcMax == 1) sprintf(titre,"%s a t=%g s",VoieManip[VoieNum].nom,CalcTi[0]);
		else sprintf(titre,"%s sur %d intervalles",VoieManip[VoieNum].nom,CalcMax);
		dim = CalcDim;
		nb = CalcMax;
	} else {
		sprintf(titre,"evt.unite %s",VoieManip[VoieNum].nom);
		if(CalcDim < VoieTampon[VoieNum].unite.taille) dim = CalcDim;
		else dim = VoieTampon[VoieNum].unite.taille;
		nb = 1;
	}
	
	i = CalcSpectreNb;
	nbfreq = (dim / 2) + 1;
	n = nbfreq * sizeof(float);
	CalcSpectre[i].fft = (float *)malloc(n);
	if(!CalcSpectre[i].fft) {
		OpiumError("Manque de place memoire pour le spectre (%d octets)",n);
		return(0);
	}
	strncpy(CalcSpectre[i].titre,titre,80);
	CalcSpectre[i].voie = VoieNum;
	CalcSpectre[i].nb = (float)nb;
	CalcSpectre[i].dim = dim;
	CalcSpectre[i].freq[0] = 0.0;
	CalcSpectre[i].freq[1] = 1.0 / (VoieEvent[VoieNum].horloge * dim); /* kHz */
	CalcSpectreNb++;
	strcpy(CalcSpectre[CalcSpectreNb].titre,"\0");

	if(!CalculeFFTVoie(FFT_PUISSANCE,CalcDonnees,VoieNum,CalcPi,nb,dim,CalcSpectre[i].fft,raison)) {
		OpiumError(raison);
		return(0);
	}

/*	{
		int k;
		printf("Spectre de bruit de %s, de %g a %d x %g:",
			VoieManip[VoieNum].nom,CalcSpectre[i].freq[0],nbfreq,CalcSpectre[i].freq[1]);
		for(k=0; k<nbfreq; k++) {
			if(!(k % 10)) printf("\n%3d:",k);
			printf(" %10g",CalcSpectre[i].fft[k]);
		}
		printf("\n");
	} */
	j = CalcFenNb;
	strncpy(CalcFen[j].titre,titre,80);
	CalcFen[j].nb = 1;
	CalcFen[j].spectre[0] =  i;
	OpiumColorInit(&(CalcFen[j].s));
	CalcFen[j].g = GraphCreate(300,300,2);
	GraphMode(CalcFen[j].g,GRF_2AXES | GRF_LEGEND);

	x = GraphAdd(CalcFen[j].g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,CalcSpectre[i].freq,nbfreq);
	y = GraphAdd(CalcFen[j].g,GRF_FLOAT|GRF_YAXIS,CalcSpectre[i].fft,nbfreq);
	GraphDataName(CalcFen[j].g,x,"f(kHz)");
	GraphDataName(CalcFen[j].g,y,VoieManip[VoieNum].nom);
	k = OpiumColorNext(&(CalcFen[j].s));
	GraphDataRGB(CalcFen[j].g,y,OpiumColorRGB(k));
	GraphAxisTitle(CalcFen[j].g,GRF_XAXIS,"Frequence (kHz)");
	GraphAxisTitle(CalcFen[j].g,GRF_YAXIS,"Bruit (nV/Rac(Hz))");
	OpiumPosition(CalcFen[j].g->cdr,50,500);
	OpiumTitle(CalcFen[j].g->cdr,titre);
	if(CalcAuVolParms[0].log) {
		GraphAxisFloatMin(CalcFen[j].g,GRF_XAXIS,1.0);
		GraphAxisLog(CalcFen[j].g,GRF_XAXIS,1);
	} else {
		GraphAxisFloatMin(CalcFen[j].g,GRF_XAXIS,0.0);
		GraphAxisLog(CalcFen[j].g,GRF_XAXIS,0);
	}
	if(CalcAuVolParms[1].log) GraphAxisLog(CalcFen[j].g,GRF_YAXIS,1);
	else GraphAxisLog(CalcFen[j].g,GRF_YAXIS,0);
	CalcFenNb++;
	strcpy(CalcFen[CalcFenNb].titre,"\0");

	OpiumDisplay(CalcFen[j].g->cdr);

	return(0);
}
/* ========================================================================== */
int CalculeFftInsere() {
	int voie,bolo,vm,cap;
	int dim,nb;
	char raison[80];
	int nbfreq,i,j,k,n; int x,y;
	char titre[80];

	bolo = CalcBolo; vm = ModlNum; voie = 0;
	if(CalcMax == 1) {
		if(OpiumExec(pCalcSpectre1->cdr) == PNL_CANCEL) return(0);
		CalcTi[0] = (double)CalcT0;
	} else {
		if(OpiumExec(pCalcSpectreN->cdr) == PNL_CANCEL) return(0);
	}
	if(Bolo[CalcBolo].lu == DETEC_HS) {
		OpiumError("Le detecteur %s n'est pas lu",Bolo[CalcBolo].nom);
		CalcBolo = bolo; ModlNum = vm;
		return(0);
	}
	for(cap=0; cap<Bolo[CalcBolo].nbcapt; cap++) {
		voie = Bolo[CalcBolo].captr[cap].voie;
		if(ModlNum == VoieManip[voie].type) break;
	}
	if(cap == Bolo[CalcBolo].nbcapt) {
		OpiumError("Pas de voie '%s' pour le detecteur '%s'",ModeleVoie[ModlNum].nom,Bolo[CalcBolo].nom);
		CalcBolo = bolo; ModlNum = vm;
		return(0);
	}
	VoieNum = voie;
	if(CalcSpectreNb >= MAXSPECTRES) {
		OpiumError("Deja %d spectres stockes!",CalcSpectreNb);
		return(0);
	}
	if(OpiumReadList("Dans quelle fenetre",CalcFenNom,&CalcFenNum,80) == PNL_CANCEL) return(0);
	j = CalcFenNum;
	if(CalcFen[j].nb >= MAXFFTFEN) {
		OpiumError("Deja %d spectres dans cette fenetre",CalcFen[j].nb);
		return(0);
	}

	if(CalcDonnees < TRMT_NBCLASSES) {
		if(CalcMax == 1) CalcPi[0] = (int64)((CalcTi[0] * 1000.0) / (double)VoieEvent[VoieNum].horloge);
		if(CalcDim <= 0) {
			OpiumError("Nombre d'echantillons invalide: %d <= 0",CalcDim);
			return(0);
		}
		if(CalcMax == 1) sprintf(titre,"%s a t=%g s",VoieManip[VoieNum].nom,CalcTi[0]);
		else sprintf(titre,"%s sur %d intervalles",VoieManip[VoieNum].nom,CalcMax);
		dim = CalcDim;
		nb = CalcMax;
	} else {
		sprintf(titre,"evt.unite %s",VoieManip[VoieNum].nom);
		if(CalcDim < VoieTampon[VoieNum].unite.taille) dim = CalcDim;
		else dim = VoieTampon[VoieNum].unite.taille;
		nb = 1;
	}
	
	i = CalcSpectreNb;
	nbfreq = (dim / 2) + 1;
	n = nbfreq * sizeof(float);
	CalcSpectre[i].fft = (float *)malloc(n);
	if(!CalcSpectre[i].fft) {
		OpiumError("Manque de place memoire pour le spectre (%d octets)",n);
		return(0);
	}
	strncpy(CalcSpectre[i].titre,titre,80);
	CalcSpectre[i].nb = (float)nb;
	CalcSpectre[i].voie = VoieNum;
	CalcSpectre[i].dim = dim;
	CalcSpectre[i].freq[0] = 0.0;
	CalcSpectre[i].freq[1] = 1.0 / (VoieEvent[VoieNum].horloge * dim); /* kHz */
	CalcSpectreNb++;
	strcpy(CalcSpectre[CalcSpectreNb].titre,"\0");

	if(!CalculeFFTVoie(FFT_PUISSANCE,CalcDonnees,VoieNum,CalcPi,nb,dim,CalcSpectre[i].fft,raison)) {
		OpiumError(raison);
		return(0);
	}

	CalcFen[j].spectre[CalcFen[j].nb] = i;
	CalcFen[j].nb += 1;
	nbfreq = (CalcSpectre[i].dim / 2) + 1;
	x = GraphAdd(CalcFen[j].g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,CalcSpectre[i].freq,nbfreq);
	y = GraphAdd(CalcFen[j].g,GRF_FLOAT|GRF_YAXIS,CalcSpectre[i].fft,nbfreq);
	GraphDataName(CalcFen[j].g,x,"f(kHz)");
	GraphDataName(CalcFen[j].g,y,VoieManip[VoieNum].nom);
	k = OpiumColorNext(&(CalcFen[j].s));
	GraphDataRGB(CalcFen[j].g,y,OpiumColorRGB(k));

//	if(OpiumDisplayed(CalcFen[j].g->cdr)) OpiumClear(CalcFen[j].g->cdr);
	OpiumDisplay(CalcFen[j].g->cdr);
	return(0);
}
/* ========================================================================== */
int CalculeFftSomme() {
	int voie,bolo,vm,cap;
	int dim,nb;
	char raison[80];
	int nbfreq,i,j,k,n;
	char titre[256];
	float *spectre; float total;

	bolo = CalcBolo; vm = ModlNum; voie = 0;
	if(CalcMax == 1) {
		if(OpiumExec(pCalcSpectre1->cdr) == PNL_CANCEL) return(0);
		CalcTi[0] = (double)CalcT0;
	} else {
		if(OpiumExec(pCalcSpectreN->cdr) == PNL_CANCEL) return(0);
	}
	if(Bolo[CalcBolo].lu == DETEC_HS) {
		OpiumError("Le detecteur %s n'est pas lu",Bolo[CalcBolo].nom);
		CalcBolo = bolo; ModlNum = vm;
		return(0);
	}
	for(cap=0; cap<Bolo[CalcBolo].nbcapt; cap++) {
		voie = Bolo[CalcBolo].captr[cap].voie;
		if(ModlNum == VoieManip[voie].type) break;
	}
	if(cap == Bolo[CalcBolo].nbcapt) {
		OpiumError("Pas de voie '%s' pour le detecteur '%s'",ModeleVoie[ModlNum].nom,Bolo[CalcBolo].nom);
		CalcBolo = bolo; ModlNum = vm;
		return(0);
	}
	VoieNum = voie;
	if(CalcDonnees < TRMT_NBCLASSES) {
		if(CalcMax == 1) CalcPi[0] = (int64)((CalcTi[0] * 1000.0) / (double)VoieEvent[VoieNum].horloge);
		if(CalcDim <= 0) {
			OpiumError("Nombre d'echantillons invalide: %d <= 0",CalcDim);
			return(0);
		}
		dim = CalcDim;
		nb = CalcMax;
	} else {
		dim =  VoieTampon[VoieNum].somme.taille;
		nb = 1;
	}
	
	if(OpiumReadList("Sur quel spectre",CalcSpectreNom,&CalcSpectreNum,80) == PNL_CANCEL) return(0);
	i = CalcSpectreNum;
	if(dim != CalcSpectre[i].dim) {
		OpiumError("Nombre d'echantillons invalide: %d != %d",dim,CalcSpectre[i].dim);
		return(0);
	}
	if(VoieNum != CalcSpectre[i].voie) {
		OpiumError("Sommation de la voie #%d sur le spectre d'une voie differente (#d)",VoieNum,CalcSpectre[i].voie);
		return(0);
	}
	
	nbfreq = (dim / 2) + 1;
	n = nbfreq * sizeof(float);
	spectre = (float *)malloc(n);
	if(!spectre) {
		OpiumError("Manque de place memoire pour le spectre (%d octets)",n);
		return(0);
	}

	if(!CalculeFFTVoie(FFT_SOMMEE,CalcDonnees,VoieNum,CalcPi,nb,dim,spectre,raison)) {
		OpiumError(raison);
		return(0);
	}
	total = CalcSpectre[i].nb + nb;
	for(k=0; k<CalcSpectre[i].dim; k++)
		CalcSpectre[i].fft[k] = ((CalcSpectre[i].fft[k] * CalcSpectre[i].nb) + spectre[k]) / total;
	free(spectre);
		
	sprintf(titre,"%s sur %g intervalles",VoieManip[VoieNum].nom,total);
	strncpy(CalcSpectre[i].titre,titre,80);
	CalcSpectre[i].nb = total;

	for(j=0; j<CalcFenNb; j++) {
		for(i=0; i<CalcFen[j].nb; i++) if(CalcFen[j].spectre[i] == CalcSpectreNum) {
			if(OpiumDisplayed(CalcFen[j].g->cdr)) OpiumRefresh(CalcFen[j].g->cdr);
			break;
		}
	}

	return(0);
}
/* ========================================================================== */
int CalculeFftSauve() {
	FILE *f; int i,nbfreq;
	TypeCalcSpectre *s; double fi;

	if(OpiumExec(pCalcSauve->cdr) == PNL_CANCEL) return(0);
	RepertoireCreeRacine(CalcSpectreFichier);
	if(!(f = fopen(CalcSpectreFichier,"w"))) {
		OpiumError("Fichier inutilisable (%s)",strerror(errno));
		return(0);
	}
	s = CalcSpectre + CalcSpectreNum;
	fprintf(f,"# Spectre de %s\n",s->titre);
	if(s->nb > 1.0) fprintf(f,"# moyenne de %g spectres analyses sur %d points\n",s->nb,s->dim);
	else fprintf(f,"# spectre brut analyse sur %d points\n",s->dim);
	nbfreq = (s->dim / 2) + 1;
	for(i=0, fi=(double)s->freq[0]; i<nbfreq; i++, fi+=(double)s->freq[1])
		fprintf(f,"%g %g\n",(float)fi,s->fft[i]);
	fclose(f);
	return(0);
}
/* ========================================================================== */
int CalculeFftSupprime() {
	int i,j,k,l;

	if(OpiumReadList("Quel spectre",CalcSpectreNom,&CalcSpectreNum,80) == PNL_CANCEL) return(0);
	if((CalcSpectreNum < 0) || (CalcSpectreNum >= CalcSpectreNb)) {
		OpiumError("STOP! y'a plus de spectre en memoire...");
		return(0);
	}
	free(CalcSpectre[CalcSpectreNum].fft);
	for(j=0; j<CalcFenNb; j++) {
		for(i=0; i<CalcFen[j].nb; i++) {
			k = CalcFen[j].spectre[i];
			if(k == CalcSpectreNum) {
				CalcFen[j].nb -= 1;
				for(l=i; l<CalcFen[j].nb; l++) CalcFen[j].spectre[l] = CalcFen[j].spectre[l+1];
				if(i < CalcFen[j].nb) {
					if(CalcFen[j].spectre[i] > CalcSpectreNum) CalcFen[j].spectre[i] -= 1;
				}
			} else if(k > CalcSpectreNum) CalcFen[j].spectre[i] -= 1;
		}
	}
	if(CalcSpectreNb) {
		CalcSpectreNb--;
		for(i=CalcSpectreNum; i<CalcSpectreNb; i++) 
			memcpy(CalcSpectre+i,CalcSpectre+i+1,sizeof(TypeCalcSpectre));
		if(CalcSpectreNum) CalcSpectreNum--;
	}
	strcpy(CalcSpectre[CalcSpectreNb].titre,"\0");
	return(0);
}
/* ========================================================================== */
int CalculeFftSurMoyenne() {
	int voie,bolo,vm,cap;
	char raison[80];
	int nbfreq,n; int x,y;
	char titre[80];

	bolo = CalcBolo; vm = ModlNum; voie = 0;
	if(CalcMax == 1) {
		if(OpiumExec(pCalcSpectre1->cdr) == PNL_CANCEL) return(0);
		CalcTi[0] = (double)CalcT0;
	} else {
		if(OpiumExec(pCalcSpectreN->cdr) == PNL_CANCEL) return(0);
	}
	if(CalcDonnees >= TRMT_NBCLASSES) {
		strcpy(raison,"Cette fonction n'est pas appelable sur les evenemens moyens");
		return(0);
	}
	if(Bolo[CalcBolo].lu == DETEC_HS) {
		OpiumError("Le detecteur %s n'est pas lu",Bolo[CalcBolo].nom);
		CalcBolo = bolo; ModlNum = vm;
		return(0);
	}
	for(cap=0; cap<Bolo[CalcBolo].nbcapt; cap++) {
		voie = Bolo[CalcBolo].captr[cap].voie;
		if(ModlNum == VoieManip[voie].type) break;
	}
	if(cap == Bolo[CalcBolo].nbcapt) {
		OpiumError("Pas de voie '%s' pour le detecteur '%s'",ModeleVoie[ModlNum].nom,Bolo[CalcBolo].nom);
		CalcBolo = bolo; ModlNum = vm;
		return(0);
	}
	VoieNum = voie;
	if(CalcMax == 1) CalcPi[0] = (int64)((CalcTi[0] * 1000.0) / (double)VoieEvent[VoieNum].horloge);
	if(CalcDim <= 0) {
		OpiumError("Nombre d'echantillons invalide: %d <= 0",CalcDim);
		return(0);
	}
	
	if(CalcDim > CalcSpectreMoyenneDim) {
		if(CalcSpectreMoyenne) free(CalcSpectreMoyenne);
		nbfreq = (CalcDim / 2) + 1;
		n = nbfreq * sizeof(float);
		CalcSpectreMoyenne = (float *)malloc(n);
		if(!CalcSpectreMoyenne) {
			OpiumError("Manque de place memoire pour le spectre (%d octets)",n);
			return(0);
		}
		GraphErase(gCalcSpectreMoyenne);
		x = GraphAdd(gCalcSpectreMoyenne,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,CalcSpectreMoyenneFreq,nbfreq);
		y = GraphAdd(gCalcSpectreMoyenne,GRF_FLOAT|GRF_YAXIS,CalcSpectreMoyenne,nbfreq);
		GraphDataName(gCalcSpectreMoyenne,x,"f(kHz)");
		GraphDataName(gCalcSpectreMoyenne,y,VoieManip[VoieNum].nom);
		GraphDataRGB(gCalcSpectreMoyenne,y,GRF_RGB_GREEN);
		GraphAxisTitle(gCalcSpectreMoyenne,GRF_XAXIS,"Frequence (kHz)");
		GraphAxisTitle(gCalcSpectreMoyenne,GRF_YAXIS,"Bruit (nV/Rac(Hz))");
/*		obsolete depuis les MonitFen: OpiumPosition(gCalcSpectreMoyenne->cdr,
			(CalcBolo * MonitLarg) + 10,(VoieNum * (MonitHaut + 40)) + 200); */
		OpiumPosition(gCalcSpectreMoyenne->cdr,50,500);
		GraphParmsSave(gCalcSpectreMoyenne);
		CalcSpectreMoyenneDim = CalcDim;
	}

	if(!CalculeFFTVoie(FFT_SUR_MOYENNE,TRMT_AU_VOL,VoieNum,CalcPi,CalcMax,CalcDim,CalcSpectreMoyenne,raison)) {
		OpiumError(raison);
		return(0);
	}

/* Refresh du graphe */
	CalcSpectreMoyenneFreq[0] = 0.0;
	CalcSpectreMoyenneFreq[1] = 1.0 / (VoieEvent[VoieNum].horloge * CalcDim); /* kHz */
/*	for(i=0; i<nbfreq; i++) printf("%4d: %8g\n",i,CalcSpectreMoyenne[i]); */
	if(CalcMax == 1)
		sprintf(titre,"%s a t=%g s",VoieManip[VoieNum].nom,CalcTi[0]);
	else
		sprintf(titre,"%s sur %d intervalles",VoieManip[VoieNum].nom,CalcMax);
	OpiumTitle(gCalcSpectreMoyenne->cdr,titre);
	if(CalcAuVolParms[0].log) {
		GraphAxisFloatMin(gCalcSpectreMoyenne,GRF_XAXIS,1.0);
		GraphAxisLog(gCalcSpectreMoyenne,GRF_XAXIS,1);
	} else {
		GraphAxisFloatMin(gCalcSpectreMoyenne,GRF_XAXIS,0.0);
		GraphAxisLog(gCalcSpectreMoyenne,GRF_XAXIS,0);
	}
	if(CalcAuVolParms[1].log) {
//		GraphAxisFloatMin(gCalcSpectreMoyenne,GRF_YAXIS,1.0);
		GraphAxisLog(gCalcSpectreMoyenne,GRF_YAXIS,1);
	} else {
//		GraphAxisFloatMin(gCalcSpectreMoyenne,GRF_YAXIS,0.0);
		GraphAxisLog(gCalcSpectreMoyenne,GRF_YAXIS,0);
	}
/*	Le graphe se retrouve en violet a peine visible, et les graduations dec...
	if(OpiumDisplayed(gCalcSpectreMoyenne->cdr)) OpiumRefresh(gCalcSpectreMoyenne->cdr);
	else OpiumDisplay(gCalcSpectreMoyenne->cdr);
*/
	if(OpiumDisplayed(gCalcSpectreMoyenne->cdr)) OpiumClear(gCalcSpectreMoyenne->cdr);
	OpiumDisplay(gCalcSpectreMoyenne->cdr);

	return(0);
}
/* ========================================================================== */
int CalculeFftNouvelle() {
	int j;

	if(CalcFenNb >= MAXCALCFEN) {
		OpiumError("Deja %d fenetres de spectres!",CalcFenNb);
		return(0);
	}
	j = CalcFenNb;
	sprintf(CalcFen[j].titre,"Spectres, serie #%d",j+1);
	CalcFen[j].nb = 0;
	CalcFen[j].spectre[0] =  0;
	OpiumColorInit(&(CalcFen[j].s));
	CalcFen[j].g = GraphCreate(300,300,2);
	GraphMode(CalcFen[j].g,GRF_2AXES | GRF_LEGEND);
	GraphAxisTitle(CalcFen[j].g,GRF_XAXIS,"Frequence (kHz)");
	GraphAxisTitle(CalcFen[j].g,GRF_YAXIS,"Bruit (nV/Rac(Hz))");
	OpiumPosition(CalcFen[j].g->cdr,50,500);
	OpiumTitle(CalcFen[j].g->cdr,CalcFen[j].titre);
	if(CalcAuVolParms[0].log) {
		GraphAxisFloatMin(CalcFen[j].g,GRF_XAXIS,1.0);
		GraphAxisLog(CalcFen[j].g,GRF_XAXIS,1);
	} else {
		GraphAxisFloatMin(CalcFen[j].g,GRF_XAXIS,0.0);
		GraphAxisLog(CalcFen[j].g,GRF_XAXIS,0);
	}
	if(CalcAuVolParms[1].log) GraphAxisLog(CalcFen[j].g,GRF_YAXIS,1);
	else GraphAxisLog(CalcFen[j].g,GRF_YAXIS,0);
	CalcFenNb++;
	strcpy(CalcFen[CalcFenNb].titre,"\0");

	return(0);
}
/* ========================================================================== */
int CalculeFftSuperpose() {
	int i,j,k,x,y; int nbfreq; int voie;

	if(OpiumExec(pCalcSuperpose->cdr) == PNL_CANCEL) return(0);

	i = CalcSpectreNum;
	j = CalcFenNum;
	if(CalcFen[j].nb >= MAXFFTFEN) {
		OpiumError("Deja %d spectres dans cette fenetre",CalcFen[j].nb);
		return(0);
	}
	CalcFen[j].spectre[CalcFen[j].nb] = i;
	CalcFen[j].nb += 1;

	nbfreq = (CalcSpectre[i].dim / 2) + 1;
	x = GraphAdd(CalcFen[j].g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,CalcSpectre[i].freq,nbfreq);
	y = GraphAdd(CalcFen[j].g,GRF_FLOAT|GRF_YAXIS,CalcSpectre[i].fft,nbfreq);
	voie = CalcSpectre[i].voie;
	GraphDataName(CalcFen[j].g,x,"f(kHz)");
	GraphDataName(CalcFen[j].g,y,VoieManip[voie].nom);
	k = OpiumColorNext(&(CalcFen[j].s));
	GraphDataRGB(CalcFen[j].g,y,OpiumColorRGB(k));

//	if(OpiumDisplayed(CalcFen[j].g->cdr)) OpiumClear(CalcFen[j].g->cdr);
	OpiumDisplay(CalcFen[j].g->cdr);
	return(0);
}
/* ========================================================================== */
int CalculeFftListe() {
	int i,j,n;

	if(!OpiumDisplayed(tCalc->cdr)) OpiumDisplay(tCalc->cdr); /* pour bien afficher des la premiere fois */
	TermEmpty(tCalc);
	TermHold(tCalc);
	if(CalcSpectreNb) {
		TermPrint(tCalc,"===== Spectres =====\n");
		for(i=0; i<CalcSpectreNb; i++) {
			n = (CalcSpectre[i].dim / 2) + 1;
			TermPrint(tCalc,"%2d/ %40s[%6d]",i+1,CalcSpectre[i].titre,CalcSpectre[i].dim);
			TermPrint(tCalc," *%-3g  (%g - %g kHz)\n",CalcSpectre[i].nb,CalcSpectre[i].freq[0],
				CalcSpectre[i].freq[0] + ((float)(n - 1) * CalcSpectre[i].freq[1]));
		}
	} else TermPrint(tCalc,"===== Pas de spectre =====\n");
	if(CalcFenNb) {
		TermPrint(tCalc,"===== Fenetres =====\n");
		for(j=0; j<CalcFenNb; j++) {
			TermPrint(tCalc,"%2d/ %40s[%6d]:\n",j+1,CalcFen[j].titre,CalcFen[j].nb);
			for(i=0; i<CalcFen[j].nb; i++) TermPrint(tCalc,"                        %-40s\n",
				CalcSpectre[CalcFen[j].spectre[i]].titre);
		}
	} else TermPrint(tCalc,"===== Pas de fenetre =====\n");
	TermPrint(tCalc,"====================\n");
	TermRelease(tCalc);
	OpiumRefresh(tCalc->cdr);
	return(0);
}
/* ========================================================================== */
int CalculeFftRenomme() {
	if(OpiumReadList("Fenetre a renommer",CalcFenNom,&CalcFenNum,80) == PNL_CANCEL) return(0);
	if(OpiumReadText("Nouveau nom",CalcFen[CalcFenNum].titre,80) == PNL_CANCEL) return(0);
	OpiumTitle(CalcFen[CalcFenNum].g->cdr,CalcFen[CalcFenNum].titre);
	if(OpiumDisplayed(CalcFen[CalcFenNum].g->cdr)) {
		OpiumClear(CalcFen[CalcFenNum].g->cdr); OpiumDisplay(CalcFen[CalcFenNum].g->cdr);
	}
	return(0);
}
/* ========================================================================== */
int CalculeFftEfface() {
	int j;

	if(OpiumReadList(L_("Quelle fenetre"),CalcFenNom,&CalcFenNum,80) == PNL_CANCEL) return(0);
	if((CalcFenNum < 0) || (CalcFenNum >= CalcFenNb)) {
		OpiumError("STOOOP! y'a plus de fenetre de spectre en memoire...");
		return(0);
	}
	if(OpiumDisplayed(CalcFen[CalcFenNum].g->cdr)) OpiumClear(CalcFen[CalcFenNum].g->cdr);
	GraphDelete(CalcFen[CalcFenNum].g);
	if(CalcFenNb) {
		CalcFenNb--;
		for(j=CalcFenNum; j<CalcFenNb; j++) 
			memcpy(CalcFen+j,CalcFen+j+1,sizeof(TypeCalcFenetre));
		if(CalcFenNum) CalcFenNum--;
	}
	strcpy(CalcFen[CalcFenNb].titre,"\0");
	return(0);
}
#ifdef HISTOS_USER
/* ========================================================================== */
int CalculeHistos() {
	int i,nb;
	TypeMonitFenetre *f;

	nb = 0;
	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if(f->type == MONIT_HISTO) nb++;
	if(nb == 0) {
		OpiumError("Il n'y a PAS d'histogramme utilisateur defini");
		return(0);
	}
	if(pCalcHistos) {
		if(PanelItemMax(pCalcHistos) < nb) {
			PanelDelete(pCalcHistos);
			pCalcHistos = 0;
		} else PanelErase(pCalcHistos);
	}
	if(pCalcHistos == 0) pCalcHistos = PanelCreate(nb);
	for(i=0, f=MonitFen; i<MonitFenNb; i++,f++) if(f->type == MONIT_HISTO) {
		PanelBool(pCalcHistos,MonitFen[i].titre,&(remplir));
	}
	return(0);
}
#endif
/* ========================================================================== */
int CalculePrefs() {
	int phys,vl,j,bolo;

	if(OpiumReadFile("Depuis le fichier",FichierPrefCalcul,MAXFILENAME) == PNL_CANCEL) return(0);
	if(ArgScan(FichierPrefCalcul,CalculDesc) <= 0) {
		OpiumError("Lecture sur '%s' impossible",FichierPrefCalcul);
	}
	for(phys=0; phys<ModelePhysNb; phys++) {
		for(vl=0; vl<CalcPhysNb; vl++) if(!strcmp(ModelePhys[phys].nom,CalcSpectreAuto[vl].nom_phys)) break;			
		if(vl < CalcPhysNb) {
			memcpy(CalcSpectreAuto+phys,CalcSpectreAutoPrefs+vl,sizeof(CalcSpectreAutoLu));
			CalcSpectreAuto[phys].lu = vl;
			CalcSpectreAutoPrefs[vl].lu = phys;
		} else {
			strcpy(CalcSpectreAuto[phys].nom_phys,ModelePhys[phys].nom);
			if(CalcPhysNb < PHYS_TYPES) { CalcSpectreAuto[phys].lu = vl = CalcPhysNb++; CalcSpectreAutoPrefs[vl].lu = phys; }
			else CalcSpectreAuto[phys].lu = -1;
		}
	}
	for(vl=0; vl<CalcPhysNb; vl++) if(CalcSpectreAutoPrefs[vl].lu < 0) {
		--CalcPhysNb;
		for(j=vl; j<CalcPhysNb; j++) memcpy(CalcSpectreAutoPrefs+j,CalcSpectreAutoPrefs+j+1,sizeof(CalcSpectreAutoLu));
		for(phys=0; phys<PHYS_TYPES; phys++) if(CalcSpectreAuto[phys].lu > vl) CalcSpectreAuto[phys].lu = CalcSpectreAuto[phys].lu - 1;
	}
	if((CalcBolo < 0) || (CalcBolo >= BoloNb)) for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].lu == DETEC_OK) { CalcBolo = bolo; break; }
	CalcASauver = 0;
	return(0);
}
/* ========================================================================== */
int CalculeImprime() {
	ArgPrint("*",CalculDesc);
	return(0);
}
/* ========================================================================== */
int CalculeSauve() {
	int phys,vl;
	
    if(SettingsSauveChoix) {
        if(OpiumReadFile("Sauver sur le fichier",FichierPrefCalcul,MAXFILENAME) == PNL_CANCEL) return(0);
    }
	for(phys=0; phys<PHYS_TYPES; phys++) if((vl = CalcSpectreAuto[phys].lu) >= 0) {
		memcpy(CalcSpectreAutoPrefs+vl,CalcSpectreAuto+phys,sizeof(CalcSpectreAutoLu));
	}
	if(ArgPrint(FichierPrefCalcul,CalculDesc) > 0) {
		CalcASauver = 0;
		printf("* Preferences de calcul sauvees dans %s\n",FichierPrefCalcul);
	} else OpiumError("Sauvegarde sur '%s' impossible",FichierPrefCalcul);
	return(0);
}
/* ========================================================================== */
int CalculeAvecTango(Menu menu, MenuItem item) {
	char commande[1024];

	if(Acquis[AcquisLocale].etat.nom_run[0] == '\0') {
		OpiumError("Pas de donnees sauvegardees avec cette session de SAMBA");
		return(0);
	}

	// il manque le log: sprintf(commande,"cd %s; ./executables/Tango.app/Contents/MacOS/Tango &",TopDir);
	// ne lance finalement rien: sprintf(commande,"cd %s; open -a Terminal executables/LanceTango &",TopDir);
	//?	sprintf(commande,"cd %s; ./executables/LanceTango.app/Contents/MacOS/Tango %s &",TopDir,Acquis[AcquisLocale].etat.nom_run);
	// parm vu comme 2eme fichier!:	sprintf(commande,"cd %s; open -a Terminal ./executables/Tango.app/Contents/MacOS/Tango %s",TopDir,Acquis[AcquisLocale].etat.nom_run);
	//	sprintf(commande,"cd %s; open -a Terminal `./executables/Tango.app/Contents/MacOS/Tango %s` &",TopDir,Acquis[AcquisLocale].etat.nom_run);
	//	sprintf(commande,"cd %s; open -nW `executables/LanceTango %s` &",FullTopDir,Acquis[AcquisLocale].etat.nom_run);
	sprintf(commande,"cd %s; tcsh -c \"%sexecutables/Tango.app/Contents/MacOS/Tango %s\" &",FullTopDir,FullTopDir,Acquis[AcquisLocale].etat.nom_run);
	printf("%s/ Lancement de Tango par '%s'\n",DateHeure(),commande);
	system(commande);
	return(0);
}
/* ========================================================================== */
int MonitHistosDef();

TypeMenuItem iCalc[] = {
//	{ "Spectre en ligne",            MNU_FONCTION   CalculeFftAuVol }, demenage dans "Affichages"
#ifdef NTUPLES_ONLINE
	{ "Graphiques evenements",       MNU_MENU     &mPlot },
	{ "Coupures evenements",         MNU_MENU     &mPlotCoupures },
#endif
	{ "Lancement de Tango",          MNU_FONCTION   CalculeAvecTango },
//	{ "Suivi gain",                  MNU_MENU     &mMonitRaies }, demenage dans "Affichages"
	{ "Mesures du bruit en differe", MNU_SEPARATION },
	{ "Pre-selection pour stats",    MNU_FONCTION   CalculeSelect },
	{ "Statistiques ligne de base",  MNU_FONCTION   CalculeStats },
	{ "Construction de spectres",    MNU_MENU     &mCalcFft },
#ifdef HISTOS_USER
	{ "Histos utilisateurs",         MNU_FONCTION   CalculeHistos },
#endif
//	{ "Histos bruit",                MNU_FONCTION   MonitHistosDef },
#ifdef PREFERENCES_PAR_MODULE
	{ "Preferences",                 MNU_SEPARATION },
	{ "Recuperer",                   MNU_FONCTION   CalculePrefs },
//	{ "Verifier",                    MNU_PANEL    &pCalculDesc },
	{ "Verifier",                    MNU_FONCTION   CalculeImprime },
	{ "Sauver",                      MNU_FONCTION   CalculeSauve },
#endif
	{ "\0",                          MNU_SEPARATION },
	{ "Fermer",                      MNU_RETOUR },
	MNU_END
};

TypeMenuItem iCalcFft[] = {
	{ "Detecteurs",			 MNU_FONCTION   CalculeFftDetec },
	{ "Individuels",         MNU_SEPARATION },
//	{ "Au vol",				 MNU_FONCTION   CalculeFftAuVol },
//	{ "RAZ",				 MNU_FONCTION   CalculeFftRazVol },
	{ "Creer",               MNU_FONCTION   CalculeFftCree },
	{ "Inserer",             MNU_FONCTION   CalculeFftInsere },
	{ "Sommer",              MNU_FONCTION   CalculeFftSomme },
	{ "Sauver",              MNU_FONCTION   CalculeFftSauve },
	{ "Supprimer",           MNU_FONCTION   CalculeFftSupprime },
	{ "Moyenne signal",      MNU_FONCTION   CalculeFftSurMoyenne },
	{ "Fenetrage",           MNU_SEPARATION },
	{ "Nouvelle",            MNU_FONCTION   CalculeFftNouvelle },
	{ "Superposer",          MNU_FONCTION   CalculeFftSuperpose },
	{ "Lister",              MNU_FONCTION   CalculeFftListe },
	{ "Renommer",            MNU_FONCTION   CalculeFftRenomme },
	{ "Effacer",             MNU_FONCTION   CalculeFftEfface },
	{ "Fermer",              MNU_RETOUR },
	MNU_END
};

TypeMenuItem iCalcSpectreAutoEnCours[] = {
	{ "Arret",      MNU_FONCTION LectStop },
	{ "Affichage",  MNU_FONCTION CalcSpectreAutoAffiche },
	MNU_END
};
TypeMenuItem iCalcSpectreAutoFinis[] = {
	{ "Afficher",  MNU_FONCTION CalcSpectreAutoAffiche },
	{ "Sauver",    MNU_FONCTION CalcSpectreAutoSauve },
	{ "Effacer ",  MNU_FONCTION CalcSpectreAutoEfface },
	{ "Revoir  ",  MNU_FONCTION CalcSpectreAutoRetrouve },
	{ "Fermer",    MNU_RETOUR },
	MNU_END
};

/* ========================================================================== */
void CalcMenuBarre() {
	mCalc = MenuLocalise(iCalc);
	mCalcFft = MenuLocalise(iCalcFft);
}
/* ========================================================================== */
/* ========================================================================== */
void CalcInit() {
	int i,k,phys,voie,bolo; Panel p;
	int DetecteurListeCapteurs(Panel panel, int num, void *arg);

/* Initialisation generale */
	CalcASauver = 0;
	bzero((void *)(&CalcPlanStd),sizeof(TypeCalcFftBase));
	CalcSpectreFenNb = 0;

	for(k=0; k<TRMT_CSNSM_MAX; k++) {
		TrmtCsnsmNom[k] = TrmtCsnsm[k].nom;
		TrmtCsnsm[k].nom[0] = '\0';
	}
	TrmtCsnsmNom[k] = "\0";
	TrmtCsnsmNb = 2;
	strcpy(TrmtCsnsm[0].nom,"basique");
	TrmtCsnsm[0].A = 7; TrmtCsnsm[0].B = 64; TrmtCsnsm[0].C = 96; TrmtCsnsm[0].D = 32;
	strcpy(TrmtCsnsm[1].nom,"meilleur");
	TrmtCsnsm[1].A = 10; TrmtCsnsm[1].B = 128; TrmtCsnsm[1].C = 96; TrmtCsnsm[1].D = 128;
	CalcPlanStd.plan = 0;
	CalcFftSolo = 0; CalcSoloNb = 0;
	CalcModeAuto = CALC_SPTA_SOMME;
	CalcSpectreAutoAuVol = CalcSpectreAutoPseudo = CalcSpectreAutoInval = 0;
	CalcSpectresMax = 20;
	CalcSpectreSauvePDF = 1; CalcSpectreSauveDB = 0;
	CalcSpectreSauveMontre = CALC_MONTRE;
	CalcSpectreSauveNom[0] = '\0';
	CalcSelMode = NB_POINTS;
	CalcT0 = 0; CalcDim = 1024; CalcMax = 1;
	CalcAuVolParms[0].log = 0; CalcAuVolParms[1].log = 1;
	CalcAuVolParms[0].autom = CalcAuVolParms[1].autom = 1;
	CalcAuVolParms[0].lim.r.min = 0.1;
	CalcAuVolParms[0].lim.r.max = 50.0;
	CalcAuVolParms[1].lim.r.min = 1.0;
	CalcAuVolParms[1].lim.r.max = 1000.0;
	for(voie=0; voie<VOIES_MAX; voie++) {
		CalcAutoCarreSomme[voie] = CalcAutoSpectreMoyenne[voie] = 0;
		CalcAutoSpectreDim[voie] = 0;
	}
	for(bolo=0; bolo < DETEC_MAX; bolo++) CalcAutoBolo[bolo] = Bolo[bolo].a_lire;
	CalcPhysNb = 0;
	for(phys=0; phys<PHYS_TYPES; phys++) {
		sprintf(CalcSpectreAuto[phys].nom_phys,"Physique%d",phys+1);
		CalcSpectreAuto[phys].lu = -1;
		CalcSpectreAuto[phys].dim = CalcDim;
		CalcSpectreAuto[phys].parms[0].log = 0;
		CalcSpectreAuto[phys].parms[0].autom = 1;
		CalcSpectreAuto[phys].parms[0].lim.r.min = 0.1;
		CalcSpectreAuto[phys].parms[0].lim.r.max = 50.0;
		CalcSpectreAuto[phys].parms[1].log = 1;
		CalcSpectreAuto[phys].parms[1].autom = 1;
		CalcSpectreAuto[phys].parms[1].lim.r.min = 1.0;
		CalcSpectreAuto[phys].parms[1].lim.r.max = 1000.0;
	}
	for(phys=0; phys<PHYS_TYPES; phys++) memcpy(CalcSpectreAutoPrefs+phys,CalcSpectreAuto+phys,sizeof(CalcSpectreAutoLu));
	CalcSpectreAutoDimX = 200; CalcSpectreAutoDimY = 150;
	CalcSpectreAutoNote[0] = '\0';
	gCalcSpectreAutoPrems = 0;
	CalcAvance = 0.0;
	
	for(i=0; i<MAXSPECTRES; i++) CalcSpectreNom[i] = CalcSpectre[i].titre;
	CalcSpectreNom[i] = "\0";
	for(i=0; i<MAXCALCFEN; i++) CalcFenNom[i] = CalcFen[i].titre;
	CalcFenNom[i] = "\0";

	CalcSpectreNb = CalcSpectreNum = 0;
	strcpy(CalcSpectre[CalcSpectreNb].titre,"\0");
	CalcFenNb = CalcFenNum = 0;
	strcpy(CalcFen[CalcFenNb].titre,"\0");

	strcpy(CalcSpectreFichier,"spectre");

	/* Lecture de la configuration */
	if(SambaLogDemarrage) SambaLogDef("= Lecture des prefs de calcul","dans",FichierPrefCalcul);
	if(ArgScan(FichierPrefCalcul,CalculDesc) <= 0) {
		if(CreationFichiers) ArgPrint(FichierPrefCalcul,CalculDesc);
		else if(SambaLogDemarrage) printf("  ! Lecture sur '%s' impossible (%s)",FichierPrefCalcul,strerror(errno));
	}
	if(ImprimeConfigs && SambaLogDemarrage) {
		printf("#\n### Fichier: '%s' ###\n",FichierPrefCalcul);
		ArgPrint("*",CalculDesc);
	}

	if(ModeBatch) return;

	pFichierCalcLect = PanelCreate(1);
	PanelFile(pFichierCalcLect,"Depuis quel fichier",FichierPrefCalcul,MAXFILENAME);
	pFichierCalcEcri = PanelCreate(1);
	PanelFile(pFichierCalcEcri,"Sur quel fichier",FichierPrefCalcul,MAXFILENAME);

	pCalcSelect = PanelCreate(2);
	PanelKeyB(pCalcSelect,"Mode","duree/nb points",&CalcSelMode,12);
	PanelList(pCalcSelect,"D'apres la fenetre",MonitFenTitre,&MonitFenNum,32);
	pCalcSelPtNb = PanelCreate(2);
	PanelInt(pCalcSelPtNb,"Nombre de points",&CalcDim);
	PanelInt(pCalcSelPtNb,"Nombre d'intervalles",&CalcMax);
	PanelItemILimits(pCalcSelPtNb,2,0,MAXINTERVALLES);
	pCalcSelPt1 = PanelCreate(1);
	PanelFloat(pCalcSelPt1,"Temps initial (ms)",&CalcT0);
	pCalcSelPt2 = PanelCreate(2);
	PanelItemSelect(pCalcSelPt2,PanelFloat(pCalcSelPt2,"Temps initial (ms)",&CalcT0),0);
	PanelFloat(pCalcSelPt2,"Temps final (ms)",&CalcT1);

	pCalcSuperpose = PanelCreate(2);
	PanelList(pCalcSuperpose,"Afficher le spectre",CalcSpectreNom,&CalcSpectreNum,80);
	PanelList(pCalcSuperpose,"dans la fenetre",CalcFenNom,&CalcFenNum,80);
	pCalcSauve = PanelCreate(2);
	PanelList(pCalcSauve,"Spectre a sauver",CalcSpectreNom,&CalcSpectreNum,80);
	PanelFile(pCalcSauve,"Fichier a creer",CalcSpectreFichier,MAXFILENAME);

	pCalcSpectreSauve = p = PanelCreate(3);
	// PanelColumns(p,2); PanelMode(p,PNL_BYLINES);
	PanelItemColors(p,PanelBool(p,"Fichier PDF",&CalcSpectreSauvePDF),SambaRougeVertJauneOrange,3);
	PanelKeyB(p,"a la fin",CalCulSvgFins,&CalcSpectreSauveMontre,16);
	PanelItemColors(p,PanelBool(p,"Database",&CalcSpectreSauveDB),SambaRougeVertJauneOrange,3);

	pCalcSpectreFerme = p = PanelCreate(1);
	PanelKeyB(p,"a la fin",CalCulSvgFins,&CalcSpectreSauveMontre,16);
	
	pCalcHistos = 0;

	tCalc = TermCreate(24,90,8192);

	gCalcSpectreMoyenne = GraphCreate(300,300,2);
	GraphMode(gCalcSpectreMoyenne,GRF_2AXES | GRF_LEGEND);
	CalcSpectreMoyenneDim = 0;

	iCalcSelect = InfoCreate(2,40);
	OpiumPosition(iCalcSelect->cdr,50,400);
	InfoWrite(iCalcSelect,2,"(touche Delete pour annuler)");
}
/* ========================================================================== */
int CalcSetup() {
	int bolo,phys; int x,y;
	
	pCalculDesc = PanelDesc(CalculDesc,1);
	PanelTitle(pCalculDesc,"Preferences");
	PanelOnOK(pCalculDesc,CalculeSauve,0);
	/* printf("#\n### Fichier: '%s' ###\n",FichierPrefCalcul);
	ArgPrint("*",CalculDesc);
	printf("#\n### Interne ###\n");
	ArgPrint("*",CalculVerifDesc); */
	
	pCalcStatsInterv1 = PanelCreate(4);
	PanelTitle(pCalcStatsInterv1,"Statistiques sur...");
	PanelList (pCalcStatsInterv1,"Nom du bolo",BoloNom,&CalcBolo,DETEC_NOM);
	PanelList (pCalcStatsInterv1,"Nom de la voie",ModeleVoieListe,&ModlNum,MODL_NOM);
	PanelFloat(pCalcStatsInterv1,"Temps initial (s)",&CalcT0);
	PanelFloat(pCalcStatsInterv1,"Temps final (s)",&CalcT1);
	pCalcStatsIntervN = PanelCreate(4);
	PanelTitle(pCalcStatsIntervN,"Statistiques sur...");
	PanelList (pCalcStatsIntervN,"Nom du bolo",BoloNom,&CalcBolo,DETEC_NOM);
	PanelList (pCalcStatsIntervN,"Nom de la voie",ModeleVoieListe,&ModlNum,MODL_NOM);
	PanelInt  (pCalcStatsIntervN,"Nombre de points",&CalcDim);
	PanelInt  (pCalcStatsIntervN,"Nombre d'intervalles",&CalcMax);
	pCalcStatsResul = PanelCreate(2);
	PanelMode(pCalcStatsResul,PNL_DIRECT);
	PanelItemSelect(pCalcStatsResul,PanelFloat(pCalcStatsResul,"Moyenne",&CalcMoyenne),0);
	PanelItemSelect(pCalcStatsResul,PanelFloat(pCalcStatsResul,"Sigma",&CalcSigma),0);

	pCalcSpectre1 = PanelCreate(8);
	PanelTitle(pCalcSpectre1,"Spectre sur...");
	PanelList (pCalcSpectre1,"Nom du bolo",BoloNom,&CalcBolo,DETEC_NOM);
	PanelList (pCalcSpectre1,"Nom de la voie",ModeleVoieListe,&ModlNum,MODL_NOM);
	PanelList (pCalcSpectre1,"Donnees",CalcDonneesName,&CalcDonnees,12);
	PanelKeyB (pCalcSpectre1,"Axe des X","lineaire/log",&CalcAuVolParms[0].log,10);
	PanelKeyB (pCalcSpectre1,"Axe des Y","lineaire/log",&CalcAuVolParms[1].log,10);
	PanelRemark(pCalcSpectre1,"Pour donnees brutes");
	PanelFloat(pCalcSpectre1,"Temps initial (s)",&CalcT0);
	PanelInt  (pCalcSpectre1,"Nombre de points",&CalcDim);
	pCalcSpectreN = PanelCreate(8);
	PanelTitle(pCalcSpectreN,"Spectre sur...");
	PanelList (pCalcSpectreN,"Nom du bolo",BoloNom,&CalcBolo,DETEC_NOM);
	PanelList (pCalcSpectreN,"Nom de la voie",ModeleVoieListe,&ModlNum,MODL_NOM);
	PanelList (pCalcSpectreN,"Donnees",CalcDonneesName,&CalcDonnees,12);
	PanelKeyB (pCalcSpectreN,"Axe des X","lineaire/log",&CalcAuVolParms[0].log,10);
	PanelKeyB (pCalcSpectreN,"Axe des Y","lineaire/log",&CalcAuVolParms[1].log,10);
//	PanelRemark(pCalcSpectreN,"Pour donnees brutes")
//	PanelInt  (pCalcSpectreN,"Nombre de points",&CalcDim);
//	PanelInt  (pCalcSpectreN,"Nombre d'intervalles",&CalcMax);
	pCalcSpectreDetecParm = PanelCreate(3);
	PanelKeyB (pCalcSpectreDetecParm,"Axe des X","lineaire/log",&CalcAuVolParms[0].log,10);
	PanelKeyB (pCalcSpectreDetecParm,"Axe des Y","lineaire/log",&CalcAuVolParms[1].log,10);
	PanelList (pCalcSpectreDetecParm,"Donnees",CalcDonneesName,&CalcDonnees,12);

#ifdef SPECTRES_SEQUENCES
	p = pCalcSpectreAuto = PanelCreate(18 * ModelePhysNb);
	PanelColumns(p,ModelePhysNb);
	//	PanelMode(p,PNL_BYLINES);
	for(phys=0; phys<ModelePhysNb; phys++) {
		if(phys == 0) {
			PanelList(p,"Donnees",CalcDonneesName,&CalcDonnees,12);
			PanelKeyB(p,"Mode","somme/trigge/glissant",&CalcModeAuto,8);
			PanelInt (p,"Nb spectres",&CalcSpectresMax);
			PanelInt (p,"Largeur",&CalcSpectreAutoDimX);
			PanelInt (p,"hauteur",&CalcSpectreAutoDimY);
		} else for(j=0; j<5; j++) PanelText(p,0,0,MODL_NOM);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,0,MODL_NOM),0),1);
		PanelItemSouligne(p,PanelItemSelect(p,PanelText(p,0,ModelePhys[phys].nom,MODL_NOM),0),1);
		entete = (phys == 0);
		PanelInt   (p,entete? "Nombre de points": 0,&CalcSpectreAuto[phys].dim);
		PanelRemark(p,entete? "-- Axe des X --": 0);
		PanelKeyB  (p,entete? "type": 0,"lineaire/log",&CalcSpectreAuto[phys].parms[0].log,10);
		PanelKeyB  (p,entete? "limites": 0,"manu/auto",&CalcSpectreAuto[phys].parms[0].autom,6);
		PanelFloat (p,entete? "min si manu": 0,&CalcSpectreAuto[phys].parms[0].lim.r.min);
		PanelFloat (p,entete? "max si manu": 0,&CalcSpectreAuto[phys].parms[0].lim.r.max);
		PanelRemark(p,entete? "-- Axe des Y --": 0);
		PanelKeyB  (p,entete? "type": 0,"lineaire/log",&CalcSpectreAuto[phys].parms[1].log,10);
		PanelKeyB  (p,entete? "limites": 0,"manu/auto",&CalcSpectreAuto[phys].parms[1].autom,6);
		PanelFloat (p,entete? "min si manu": 0,&CalcSpectreAuto[phys].parms[1].lim.r.min);
		PanelFloat (p,entete? "max si manu": 0,&CalcSpectreAuto[phys].parms[1].lim.r.max);
	}
	pCalcSpectreBolo = PanelCreate(BoloNb);
	for(bolo=0; bolo<BoloNb; bolo++) PanelBool(pCalcSpectreBolo,Bolo[bolo].nom,&CalcAutoBolo[bolo]);
	pCalcSpectreAffiche = 0;
	mCalcSpectreAutoEnCours = MenuIntitule(iCalcSpectreAutoEnCours,"Spectres en cours");
	mCalcSpectreAutoFinis = MenuIntitule(iCalcSpectreAutoFinis,"Spectres termines");
#endif /* SPECTRES_SEQUENCES */

/*
 * Spectres multiples en automatique
 */
	if((CalcBolo < 0) || (CalcBolo >= BoloNb)) for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire) { CalcBolo = bolo; break; }

	for(bolo=0; bolo<BoloNb; bolo++) for(phys=0; phys<PHYS_TYPES; phys++) gCalcSpectreBolo[bolo][phys] = 0;
	bLectSpectres = BoardCreate(); OpiumTitle(bLectSpectres,"Spectrometrie multiple");
	if(!LectCompacteUtilisee) {
		x = Dx; y = Dy; CalcSpectreAutoPlanche(bLectSpectres,&x,&y);
	}
	
	return(1);
}
/* ========================================================================== */
int CalcExit() {
	return(SambaSauve(&CalcASauver,"preferences de calcul",1,1,CalculeSauve));
}
