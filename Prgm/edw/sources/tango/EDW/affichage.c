#define VERSION "1.1"
#ifdef macintosh
#pragma message(__FILE__)
#endif

#include <environnement.h>

#include <stdio.h>
#ifdef CODE_WARRIOR_VSN
	#include <types.h>
#else
	#include <sys/types.h>
#endif
#include <unistd.h>
#include <math.h>

#include <errno.h>
#ifndef WIN32
	#include <strings.h>  /* pour strcpy */
#else
	#include <string.h>
	#include <io.h>
#endif
#include <fcntl.h>

#include <utils/decode.h>
#include <opium4/opium.h>
#include <opium4/histos.h>
#include <tango.h>
#include <calib.h>
#include <utils/filtres.h>
#include <opium4/nanopaw.h>

#include <rundata.h>
#include <detecteurs.h>
#include <archive.h>       /* EDW2 */
#include <evtread.h>
#define MAINSAMBA
#undef VAR
#define VAR
#include "analyse.h"
/* #include <salsa/objets_samba.h>  EDW2 */

#define VERIFIE_VERSION
#define MAXARCHNOM 80
#define MAXPAQUET 256

extern float *Base[VOIES_MAX];
extern float *Energie[VOIES_MAX];

typedef struct {
	WndColorLevel r[WND_NBQUAL+1],g[WND_NBQUAL+1],b[WND_NBQUAL+1];
} TypeArchCouleur;
static TypeArchCouleur ArchCoulStd[VOIES_MAX],ArchCoulDec[VOIES_MAX];

static char ArchInitDisplayTodo;
static char ArchEvtGroupe;
static char ArchPosHdrEvt,ArchPosVoies,ArchPosTouches;
static int ArchPosHdrEvtX,ArchPosHdrEvtY;
static int ArchPosVoiesX,ArchPosVoiesY;
static int ArchPosTouchesX,ArchPosTouchesY;
static int ArchVoiesMaxAff;
static TypeEvtColoriage ArchEvtPalette;
static Panel pTriggee,pVoieEvt;
static Graph gEvt,gDonnee[VOIES_MAX];
static Graph gPaq;
static char ArchNbTraces;
static Info iVide;
static float ArchAb6[VOIES_MAX][2];
static float ArchPaqTemps[2];
static float ArchPaqDate[MAXPAQUET][2];
#ifdef CHANGE_UNITE
	static float ArchCalib[VOIES_MAX];
#endif

static int RunNumber; static char RunType[12]; static char RunDate[12];
static char NomArchive[MAXFILENAME];
static ArgDesc ArchArgs[] = {
	{ "v",   DESC_INT   &VsnManip,        "Version d'EDELWEISS" },
	{ "X",   DESC_INT   &ArchGraphPosX,   "Abcisse des graphiques des evenements" },
	{ "Y",   DESC_INT   &ArchGraphPosY,   "Ordonnee des graphiques des evenements" },
	{ "L",   DESC_INT   &ArchGraphLarg,   "Largeur des graphiques des evenements" },
	{ "H",   DESC_INT   &ArchGraphHaut,   "Hauteur des graphiques des evenements" },
	{ "v",   DESC_TEXT   ProdVarsName,    "Fichier des variables utilisateur" },
	{ "t",   DESC_TEXT   ProdTempsName,   "Fichier des zones de temps" },
	{ "a",   DESC_TEXT   ProdCtesName,    "Fichier des parametres d'analyse" },
	{ "c",   DESC_TEXT   ProdCalibName,   "Fichier des coefficients de calibration" },
	{ "m10", DESC_NONE }, // DESC_FLOAT &ProdMontee10,    "Pourcentage de l'amplitude pour le debut de la montee" },
	{ "m90", DESC_NONE }, // DESC_FLOAT &ProdMontee90,    "Pourcentage de l'amplitude pour la fin de la montee" },
	{ "m50", DESC_NONE }, // DESC_FLOAT &ProdQuotaMontee, "Quota de montee a retirer de la duree" },
	{ 0,     DESC_NONE }, // DESC_TEXT  NomArchive,      "Nom du fichier a analyser" },
	{ "j",   DESC_NONE }, // DESC_TEXT  RunDate,         "Code de la date du run (nom EDELWEISS 2 type 'banc')" },
	{ "k",   DESC_NONE }, // DESC_TEXT  RunType,         "Categorie de run (nom EDELWEISS 2 type 'manip')" },
	{ "r",   DESC_NONE }, // DESC_INT  &RunNumber,       "Numero de run" },
	DESC_END
};

static ArgDesc ProdCtes[] = {
	{ "Debut.montee",                 DESC_NONE }, // DESC_FLOAT &ProdMontee10,    "Pourcentage de l'amplitude pour le debut de la montee" },
	{ "Fin.montee",                   DESC_NONE }, // DESC_FLOAT &ProdMontee90,    "Pourcentage de l'amplitude pour la fin de la montee" },
	{ "Quota.montee.dans.duree",      DESC_NONE }, // DESC_FLOAT &ProdQuotaMontee, "Quota de montee intervenant dans la duree (ms)" },
	{ "Polarisation",                 DESC_FLOAT &PolarIonisation, "Polarisation du detecteur (V)" },
	{ "Energie.reference",            DESC_FLOAT &EnergieRef,      "Energie du signal dans les runs de calibration (keV)" },
	{ "Chaleur.reference",            DESC_FLOAT &RefChal,         "Amplitude du signal chaleur de calibration (ADU)" },
	{ "Chaleur.seuil",                DESC_FLOAT &SeuilChal,       "Seuil chaleur (keV)" },
	{ "Chaleur.resolution.base",      DESC_FLOAT &ResolChalBase,   "Resolution chaleur sur la ligne de base (keV)" },
	{ "Chaleur.resolution.signal",    DESC_FLOAT &ResolChalRef,    "Resolution chaleur sur le signal de calibration (keV)" },
	{ "Ionisation.reference",         DESC_FLOAT &RefIonis,        "Amplitude du signal ionisation de calibration (ADU)" },
	{ "Ionisation.seuil",             DESC_FLOAT &SeuilIonis,      "Seuil ionisation (keV)" },
	{ "Ionisation.resolution.base",   DESC_FLOAT &ResolIonisBase,  "Resolution ionisation sur la ligne de base (keV)" },
	{ "Ionisation.resolution.signal", DESC_FLOAT &ResolIonisRef,   "Resolution ionisation sur le signal de calibration (keV)" },
	{ "Neutrons.facteur",             DESC_FLOAT &NeutronsFact,    "Facteur multiplicatif pour la zone neutrons" },
	{ "Neutrons.exposant",            DESC_FLOAT &NeutronsExp,     "Exposant pour la zone neutrons" },
//	{ "Calibration.chaleur",          DESC_TEXT  ProdChaleurName,  "Fichier des corrections chaleur" },
//  { "Mesures",                      DESC_LIST  ClesMesure ,&MesurePourCalib,  "Mesures a utiliser" },
	DESC_END
};

static void ArchEvtFenetres(int stocke, char trace_unique, Graph gevt, int x0, int y0, int dx, int dy);
static void ArchNtuplePlace(Graph g, char un_seul_graphique, int evt);

#ifdef WIN32
/* ========================================================================== */
char *rindex(char *s, int c) {
	char k,*x;

	k = (char)c; x = s + strlen(s);
	while(--x >= s) if(*x == k) return(x);
	return(0);
	
}
#endif
/* ========================================================================== */
void ArchStrmRazDisplay() {
	int voie;

	if(!ArchInitDisplayTodo) return;
	for(voie=0; voie<VoiesNb; voie++) gDonnee[voie] = 0;
	ArchInitDisplayTodo = 0;
}
/* ========================================================================== */
static void ArchEvtInitDisplay() {
	int voie,bolo,y;

	gEvt = 0; for(voie=0; voie<VoiesNb; voie++) gDonnee[voie] = 0;
	gPaq = 0;
	ArchNbTraces = 1;
	iVide = 0;
	ArchVoiesMaxAff = 0;
	for(bolo=0; bolo<BoloNb; bolo++) {
		if(Bolo[bolo].nbcapt > ArchVoiesMaxAff) ArchVoiesMaxAff = Bolo[bolo].nbcapt;
	}
	pTriggee = PanelCreate(2);
	PanelList(pTriggee,"Detecteur touche",BoloNom,&BoloTrigge,DETEC_NOM);
	PanelList(pTriggee,"Voie touchee",ModeleVoieListe,&ModlTrigge,MODL_NOM);
	y = OpiumServerHeigth(0) - 2*(ArchVoiesMaxAff * (ArchGraphHaut + 40)) - WndLigToPix(5);
	OpiumPosition(pTriggee->cdr,20,y);

	if(ArchEvtGroupe) {
		ArchGraphPosX = OpiumServerWidth(0) - ArchGraphLarg - 15; if(ArchGraphPosX < 15) ArchGraphPosX = 15;
		ArchGraphPosY = OpiumServerHeigth(0) - ArchGraphHaut - 40; if(ArchGraphPosY < 40) ArchGraphPosY = 40;
	} else { ArchGraphPosX = 15; ArchGraphPosY = 40; }
	ArchGraphLarg = 500; ArchGraphHaut = 300;

	ArchInitDisplayTodo = 0;

}
#ifdef CHANGE_UNITE
/* ========================================================================== */
static float ArchUnitesADU(Graph g, int sens, float val) {
	int voie;

	for(voie=0; voie<VoiesNb; voie++) if(AfficheVoie[(int)VoieManip[voie].type]) {
		// if(g == gDonnee[voie]) {
			return(sens? val * ArchCalib[voie]: val / ArchCalib[voie]);
		// }
	}
	return(val);
}
#endif /* CHANGE_UNITE */
/* ========================================================================== */
static int ArchEvtNbTraces(TypeEvt *event) {
	int vt,bolo,voie,vm,nb_traces;

	nb_traces = 0;
	for(vt=0; vt<event->nbvoies; vt++) if(event->voie[vt].dim > 0) {
		voie = event->voie[vt].num;
		if(VoieEvent[voie].lngr == 0) continue;
		vm = VoieManip[voie].type;
		if(AfficheVoie[vm]) {
			bolo = VoieManip[voie].det;
			if(AfficheBolo[bolo]) nb_traces++;
		}
	}
	return(nb_traces);
}
/* ========================================================================== */
void ArchEvtErase() {
	int vt,voie;
	
	EvtAffiche = -1;
	if(OpiumAlEcran(pHdrEvt)) OpiumClear(pHdrEvt->cdr);
	if(OpiumAlEcran(gEvt)) OpiumClear(gEvt->cdr);
	for(vt=0; vt<ArchEvt.nbvoies; vt++) {
		voie = ArchEvt.voie[vt].num;
		if(gDonnee[voie]) {
			if(OpiumAlEcran(gDonnee[voie])) OpiumClear(gDonnee[voie]->cdr);
		}
	}
	PlotNtupleMasque(EvtEspace);
}
/* ========================================================================== */
static void ArchEvtCopyTo(TypeEvtStocke *evtstk) {
	int vt,voie;
	
	memcpy(&(evtstk->evt),&ArchEvt,sizeof(TypeEvt));
	if(evtstk->signaux) for(vt=0; vt<ArchEvt.nbvoies; vt++) {
		voie = ArchEvt.voie[vt].num;
		memcpy(&(evtstk->signaux[vt]),&(VoieEvent[voie]),sizeof(TypeVoieDonnees));
		evtstk->signaux[vt].filtre = 0; // pas sympa
		if(VoieEvent[voie].lngr > 0) {
			evtstk->signaux[vt].brutes.t.sval = (TypeDonnee *)malloc(VoieEvent[voie].lngr * sizeof(TypeDonnee));
			if(evtstk->signaux[vt].brutes.t.sval)
				memcpy(evtstk->signaux[vt].brutes.t.sval,VoieEvent[voie].brutes.t.sval,VoieEvent[voie].lngr * sizeof(TypeDonnee));
		} else evtstk->signaux[vt].brutes.t.sval = 0;
	}
}
/* ========================================================================== */
int ArchEvtSave(char avec_graph) {
	int i,j;
	
	if(avec_graph && (EvtAffiche < 0)) { OpiumError("Pas d'evenement affiche"); return(0); }
	if((j = ArchEvtSaved(EvtDemande))) return(j);
	else {
		i = EvtStockNb;
		EvtStock[i] = (TypeEvtStocke *)malloc(sizeof(TypeEvtStocke));
		if(EvtStock[i]) {
			EvtStock[i]->index = EvtDemande;
			EvtStock[i]->evt.nbvoies = ArchEvt.nbvoies;
			if(avec_graph) {
				EvtStock[i]->nb_traces = ArchNbTraces; EvtStock[i]->g = gEvt; gEvt = 0;
			} else {
				EvtStock[i]->nb_traces = ArchEvtNbTraces(&ArchEvt); EvtStock[i]->g = 0;
			}
			if(EvtStock[i]->evt.nbvoies) {
				EvtStock[i]->signaux = (TypeVoieDonnees *)malloc(EvtStock[i]->evt.nbvoies * sizeof(TypeVoieDonnees));
				if(EvtStock[i]->signaux) {
					ArchEvtCopyTo(EvtStock[i]); EvtStockNb++;
					if(EvtStock[i]->g) {
						GraphErase(EvtStock[i]->g);
						ArchEvtFenetres(i+1,(EvtStock[i]->nb_traces < 2),EvtStock[i]->g,0,0,0,0);
					}
					return(i+1);
				} else {
					OpiumFail("Defaut d'espace memoire pour les traces (deja %d evenement%s stocke%s)\n",Accord2s(EvtStockNb));
					free(EvtStock[i]); EvtStock[i] = 0;
				}
			} else {
				EvtStock[i]->signaux = 0;
				ArchEvtCopyTo(EvtStock[i]); EvtStockNb++;
				return(i+1);
			}
		} else OpiumFail("Plus assez d'espace memoire (deja %d evenement%s stocke%s)\n",Accord2s(EvtStockNb));
		return(0);
	}
}
#define MAXDETAFFICHES 64
#define MAXDETENLARGEUR 5
/* ========================================================================== */
void ArchEvtMontre(int evt) {
	int bolo,voie,vm,cap; //,nb_vus;
	int i,nb_traces; int vt;
	int x0,y0,dx,dy,x,y;
	// int rang_x,rang_y;
	//- int xn,yn,hn,vn;
	int stocke;
	TypeEvt *event; Graph g;
	TypeVoieArchivee *ajoutee;
	char titre[64];
	char trace_unique,un_seul_graphique,regroupe,refait;

	if(ArchInitDisplayTodo) ArchEvtInitDisplay();

	refait = (EvtDemande == evt);
	EvtDemande = evt;
	if((stocke = ArchEvtSaved(evt))) {
		event = &(EvtStock[stocke-1]->evt);
		g = EvtStock[stocke-1]->g;
		nb_traces = EvtStock[stocke-1]->nb_traces;
	} else {
		event = &ArchEvt; g = gEvt;
		nb_traces = ArchNbTraces = ArchEvtNbTraces(event);
	}
	trace_unique = (nb_traces < 2);
	regroupe = !trace_unique && ArchEvtGroupe;
	un_seul_graphique = trace_unique || ArchEvtGroupe;
//+	if(stocke) printf("* Evt[%d] < %d%s stocke, g @%08X\n",evt,stocke,(stocke<2)?"er":"eme",(hexa)g);
//+	else printf("* Evt[%d] : a lire sur fichier, g @%08X\n",evt,(hexa)g);
	
	x = 5; y = 50;
	
	if(OpiumAlEcran(pHdrEvt)) OpiumClear(pHdrEvt->cdr);
	if(AfficheHdrEvt) {
		if(stocke) memcpy(&ArchEvt,event,sizeof(TypeEvt));
		sprintf(titre,"Entete de l'evenement #%d",event->num);
		OpiumTitle(pHdrEvt->cdr,titre);
		if(ArchPosHdrEvt) {
			OpiumDisplay(pHdrEvt->cdr);
			OpiumGetSize(pHdrEvt->cdr,&dx,&dy); x = OpiumServerWidth(0) - dx;
			ArchPosHdrEvtX = x; ArchPosHdrEvtY = y; y += (dy + 40);
			ArchPosHdrEvt = 0;
			OpiumClear(pHdrEvt->cdr);
		}
		OpiumPosition(pHdrEvt->cdr,ArchPosHdrEvtX,ArchPosHdrEvtY);
		OpiumDisplay(pHdrEvt->cdr);
	}

	if(pVoieEvt) {
		if(OpiumAlEcran(pVoieEvt)) OpiumClear(pVoieEvt->cdr);
		PanelDelete(pVoieEvt); pVoieEvt = 0;
	}
	BoloTrigge = event->bolo_hdr;
	if(event->nbvoies == 0) OpiumNote("Evenement a entete seule");
	else if(AfficheVoies) {
		cap = 0;
		for(vt=0; vt<event->nbvoies; vt++) /* if(event->voie[vt].dim > 0) */ {
			voie = event->voie[vt].num;
			vm = VoieManip[voie].type;
			if(AfficheVoie[vm]) {
				bolo = VoieManip[voie].det;
				if(bolo == BoloTrigge) cap++;
			}
		}
		if(cap) {
			pVoieEvt = PanelCreate(13 * cap);
			PanelColumns(pVoieEvt,cap);
			cap = 0;
			for(vt=0; vt<event->nbvoies; vt++) /* if(event->voie[vt].dim > 0) */ {
				voie = event->voie[vt].num;
				bolo = VoieManip[voie].det;
				if(bolo != BoloTrigge) continue;
				vm = VoieManip[voie].type;
				if(AfficheVoie[vm]) {
					ajoutee = &(event->voie[vt]);
					i = PanelText (pVoieEvt,0,ModeleVoie[vm].nom,MODL_NOM); PanelItemSouligne(pVoieEvt,PanelItemSelect(pVoieEvt,i,0),1);
					i = PanelInt  (pVoieEvt,cap? 0: "Debut trace  (s)",&(ajoutee->sec)); // if(!cap) PanelItemBarreMilieu(pVoieEvt,i,1);
					i = PanelInt  (pVoieEvt,cap? 0: "Debut trace (us)",&(ajoutee->msec)); PanelFormat(pVoieEvt,i,"%06d");
					i = PanelFloat(pVoieEvt,cap? 0: "Horloge     (ms)",&(ajoutee->horloge));
					i = PanelInt  (pVoieEvt,cap? 0: "Nb points",&(ajoutee->dim));
					i = PanelInt  (pVoieEvt,cap? 0: "Gigastamp debut",&(ajoutee->gigastamp));
					i = PanelInt  (pVoieEvt,cap? 0: "Timestamp debut",&(ajoutee->stamp)); PanelFormat(pVoieEvt,i,"%09d");
					i = PanelInt  (pVoieEvt,cap? 0: "Pretrigger",&(ajoutee->avant_evt));
					i = PanelFloat(pVoieEvt,cap? 0: "Seuil positif",&(ajoutee->trig_pos));
					i = PanelFloat(pVoieEvt,cap? 0: "Seuil negatif",&(ajoutee->trig_neg));
					i = PanelFloat(pVoieEvt,cap? 0: "Base",&(ajoutee->val[MONIT_BASE]));
					i = PanelFloat(pVoieEvt,cap? 0: "Bruit",&(ajoutee->val[MONIT_BRUIT]));
					i = PanelFloat(pVoieEvt,cap? 0: "Amplitude",&(ajoutee->val[MONIT_AMPL]));
					cap++;
				}
			}
			sprintf(titre,"Voies de l'evenement #%d",event->num);
			OpiumTitle(pVoieEvt->cdr,titre);
			if(ArchPosVoies) {
				OpiumDisplay(pVoieEvt->cdr);
				if(x <= 0) { OpiumGetSize(pVoieEvt->cdr,&dx,&dy); x = OpiumServerWidth(0) - dx; }
				ArchPosVoiesX = x; ArchPosVoiesY = y; y += (dy + 40);
				ArchPosVoies = 0;
				OpiumClear(pVoieEvt->cdr);
			}
			OpiumPosition(pVoieEvt->cdr,ArchPosVoiesX,ArchPosVoiesY);
			OpiumDisplay(pVoieEvt->cdr);
		}
		if(OpiumAlEcran(pTriggee)) OpiumClear(pTriggee->cdr);
		if(VoiesNb > 1) {
			BoloTrigge = event->bolo_hdr;
			if((event->voie_hdr >= 0) && (event->voie_hdr < VoiesNb)) ModlTrigge = VoieManip[event->voie_hdr].type;
			if((BoloTrigge >= 0) && (BoloTrigge < BoloNb) && (ModlTrigge >= 0) && (ModlTrigge < ModeleVoieNb)) {
				sprintf(titre,"Voie touchee dans l'evenement #%d",event->num);
				OpiumTitle(pTriggee->cdr,titre);
				if(ArchPosTouches) {
					OpiumDisplay(pTriggee->cdr);
					if(x <= 0) { OpiumGetSize(pTriggee->cdr,&dx,&dy); x = OpiumServerWidth(0) - dx; }
					ArchPosTouchesX = x; ArchPosTouchesY = y; y += (dy + 40);
					ArchPosTouches = 0;
					OpiumClear(pTriggee->cdr);
				}
				OpiumPosition(pTriggee->cdr,ArchPosTouchesX,ArchPosTouchesY);
				OpiumDisplay(pTriggee->cdr);
			}
		}
	}
	OpiumUserAction();

	if(g) {
		if(stocke || refait) {
//			if(OpiumRefresh(g->cdr)) return; else { GraphDelete(g); g = 0; }
			if(OpiumAlEcran(g)) { OpiumRefresh(g->cdr); OpiumActive(g->cdr); }
			else OpiumDisplay(g->cdr);
			ArchNtuplePlace(g,un_seul_graphique,evt);
			return;
		} else {
			OpiumGetPosition(g->cdr,&ArchGraphPosX,&ArchGraphPosY);
			OpiumGetSize(g->cdr,&ArchGraphLarg,&ArchGraphHaut);
			GraphDelete(g); g = 0;
		}
	}
	if((un_seul_graphique && !g && (nb_traces > 0)) || (!g && stocke)) {
		g = GraphCreate(ArchGraphLarg,ArchGraphHaut,2*nb_traces); //+ printf("  | refait g @%08X\n",(hexa)g);
		if(g) { sprintf(titre,"Evenement #%d",event->num); OpiumTitle(g->cdr,titre); }
	}
	if(stocke) EvtStock[stocke-1]->g = g; else gEvt = g;
//+	if(stocke) printf("  | g[%d] @%08X\n",stocke,(hexa)(EvtStock[i]->g));
//+	else printf("  | gEvt @%08X\n",(hexa)gEvt);
	if(un_seul_graphique && !g) {
		if(!iVide) iVide = InfoCreate(1,80);
		if(iVide) {
			sprintf(titre,"Evenement #%d",event->num);
			InfoTitle(iVide,titre);
			InfoWrite(iVide,1,"Evenement #%d vide",event->num);
			OpiumDisplay(iVide->cdr);
		}
		return;
	} else if(iVide) {
		OpiumClear(iVide->cdr);
		InfoDelete(iVide);
		iVide = 0;
	}

	for(voie=0; voie<VoiesNb; voie++) {
		if(gDonnee[voie]) { if(OpiumAlEcran(gDonnee[voie])) OpiumClear(gDonnee[voie]->cdr); }
		for(i=0; i<ProdNtpNb[voie]; i++) PlotVarSelecte(&(Ntuple[ProdNtpNum[voie][i]]),0);
	}

	EvtAffiche = event->num;
	x0 = ArchGraphPosX; // OpiumPosition(cTangoAnalyse,ArchGraphPosX+ArchGraphLarg+WND_ASCINT_WIDZ+5,ArchGraphPosY);
	y0 = ArchGraphPosY; // OpiumServerHeigth(0) - 2*(ArchVoiesMaxAff * dy);
#ifdef EVT_A_DROITE
	{ int hp,vp;
		OpiumPosition(cTangoAnalyse,ArchGraphPosX,ArchGraphPosY);
		OpiumGetSize(cTangoAnalyse,&hp,&vp);
		x0 = ArchGraphPosX; // OpiumServerWidth(0) - (BoloGeres * dx);
		y0 = ArchGraphPosY + vp + WND_ASCINT_WIDZ + 5;
	}
#endif
	dx = ArchGraphLarg + WND_ASCINT_WIDZ + 5;
	dy = ArchGraphHaut + (2 * WND_ASCINT_WIDZ);
	
	ArchEvtFenetres(stocke,trace_unique,g,x0,y0,dx,dy);
}
/* ========================================================================== */
char ArchPaqMontre(int paq) {
	int i,j,m,p,t,s; int evt,evt1,evtN,evt_nb,vt,voie; // int bolo;
	int decalage_avant,decalage_apres,total,duree,debut_evt;
	int x,y,h,v; int smin,smax;
	float horloge;
	char rc;
	char titre[64];
	TypeS_ns t1,tN,ti,dt;
	TypeEvtStocke *evtstk; TypeEvt *evtptr; TypeVoieDonnees *signal;

	total = 0; p = 0;

	evt_nb = (int)PaqMultiplicite[paq];
	printf("* Paquet #%d: %d evenement%s a partir du #%d\n",paq+1,Accord1s(evt_nb),PaqEvtPremier[paq]+1);
	evt = evt1 = EvtDemande = PaqEvtPremier[paq];
	//2 printf("  . Lecture de l'evenement #%d\n",evt+1);
	rc = EvtLitUnSeul(evt,1); if(!rc) goto pas_possible;
	if((evt_nb > 1) && (p = ArchEvtSave(0))) { evtstk = EvtStock[p-1]; evtptr = &(evtstk->evt); }
	else { p = 0; evtstk = 0; evtptr = &ArchEvt; }
	//2 printf("  . Stock[%d]: evenement #%d\n",p,(p > 0)? evtptr->num: -1);
	//1 printf("  . Timestamp[%d]  = %d.%09d\n",evtptr->num,evtptr->gigastamp,evtptr->stamp);
	t1.s_us = S_nsFromInt(evtptr->gigastamp,evtptr->stamp);
	vt = evtptr->voie_evt; voie = evtptr->voie[vt].num;
	if(evtstk) signal = &(evtstk->signaux[vt]); else signal = &(VoieEvent[voie]);
	horloge = signal->horloge;
	decalage_avant = signal->avant_evt;
	duree = signal->lngr;

	if(evt_nb > 1) {
		evt = evtN = EvtDemande = PaqEvtPremier[paq] + evt_nb - 1;
		//2 printf("  . Lecture de l'evenement #%d\n",evt+1);
		rc = EvtLitUnSeul(evt,1); if(!rc) goto pas_possible;
		if((p = ArchEvtSave(0))) { evtstk = EvtStock[p-1]; evtptr = &(evtstk->evt); }
		else { evtstk = 0; evtptr = &ArchEvt; }
		//2 printf("  . Stock[%d]: evenement #%d\n",p,(p > 0)? evtptr->num: -1);
		//1 printf("  . Timestamp[%d]  = %d.%09d\n",evtptr->num,evtptr->gigastamp,evtptr->stamp);
		tN.s_us = S_nsFromInt(evtptr->gigastamp,evtptr->stamp);
		dt.s_us = S_nsOper(&tN,S_US_MOINS,&t1);
		//1 printf("  . difference = %d.%09d\n",dt.t.s,dt.t.us);
		total = (int)S_nsHex64(dt);
		vt = evtptr->voie_evt;
		if(evtstk) signal = &(evtstk->signaux[vt]); else signal = &(VoieEvent[evtptr->voie[vt].num]);
	}
	decalage_apres = signal->lngr - signal->avant_evt;

	total += decalage_avant + decalage_apres;
	printf("  . Temps total: %d echantillons (soit %g ms)\n",total,(float)total * horloge);

	if(gPaq) {
		OpiumGetPosition(gPaq->cdr,&x,&y);
		OpiumGetSize(gPaq->cdr,&h,&v);
		GraphDelete(gPaq); gPaq = 0;
	} else {
		x = Dx; y = ArchGraphPosY;
		h = OpiumServerWidth(0) - WND_ASCINT_WIDZ - 5; v = ArchGraphHaut;
	}
	gPaq = GraphCreate(h,v,1+(2*evt_nb));
	if(gPaq) { sprintf(titre,"Paquet #%d, evenements %d-%d",paq+1,evt1+1,evtN+1); OpiumTitle(gPaq->cdr,titre); }
	ArchPaqTemps[0] = -(float)decalage_avant * horloge; // 0.0;
	ArchPaqTemps[1] = horloge;
	t = GraphAdd(gPaq,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,ArchPaqTemps,total); GraphDataName(gPaq,t,"Temps(ms)");

	m = 0;
	for(i=0; i<evt_nb; i++) {
		evt = PaqEvtPremier[paq]+i;
		if(evt_nb > 1) {
			if((rc = EvtLitUnSeul(evt,1))) {
				EvtDemande = evt;
				if((p = ArchEvtSave(0))) { evtstk = EvtStock[p-1]; evtptr = &(evtstk->evt); }
				else { evtstk = 0; evtptr = &ArchEvt; }
				//2 printf("  > %2d/ Stock[%d]: evenement #%d\n",i+1,p,(p > 0)? evtptr->num: -1);
				vt = evtptr->voie_evt; // for(vt=0; vt<evtptr->nbvoies; vt++) /* if(evtptr->voie[vt].dim > 0) */ {
				voie = evtptr->voie[vt].num; // bolo = VoieManip[voie].det; if(bolo != evtptr->bolo_hdr) continue;
				if(evtstk) signal = &(evtstk->signaux[vt]); else signal = &(VoieEvent[voie]);
				ti.s_us = S_nsFromInt(evtptr->gigastamp,evtptr->stamp);
				dt.s_us = S_nsOper(&ti,S_US_MOINS,&t1);
				debut_evt = (int)S_nsHex64(dt);
			}
		} else debut_evt = 0;
		if(rc) {
			//1 printf("  > %2d/ Evt[%d], paquet #%g: debut a %d echantillons (soit %g ms)\n",i+1,
			//	   evt+1,EvtPaquetNum[evt],debut_evt,(float)debut_evt*signal->horloge);
			printf("  > %2d/ Evt[%d] @ %g ms\n",i+1,evt+1,(float)debut_evt*signal->horloge);
			for(j=0; j<signal->lngr; j++) {
				if(m == 0) { smin = smax = signal->brutes.t.sval[j]; m = 1; }
				else {
					if(signal->brutes.t.sval[j] > smax) smax = signal->brutes.t.sval[j];
					if(signal->brutes.t.sval[j] < smin) smin = signal->brutes.t.sval[j];
				}
			}
			ArchPaqDate[i][0] = (float)debut_evt * signal->horloge; // 0.0;
			ArchPaqDate[i][1] = signal->horloge;
			GraphAdd(gPaq,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(ArchPaqDate[i][0]),signal->lngr);
			s = GraphAdd(gPaq,GRF_SHORT|GRF_YAXIS,signal->brutes.t.sval,signal->lngr);
			GraphDataTrace(gPaq,s,GRF_HST,0); GraphDataStyle(gPaq,s,GRAPH_HISTO_FLOTTANT);
			// if(GraphQualite == WND_Q_PAPIER) GraphDataRGB(gPaq,s,0,WND_COLOR_3QUARTS,0);
			// else GraphDataRGB(gPaq,s,GRF_RGB_GREEN);
			GraphDataSupportRGB(gPaq,s,WND_Q_PAPIER,0,WND_COLOR_3QUARTS,0);
			GraphDataSupportRGB(gPaq,s,WND_Q_ECRAN,GRF_RGB_GREEN);
			// pas au point dans ce cas: GraphZoom(gPaq,10*total/duree,1);
			// }
		}
	}
	GraphAxisAutoRange(gPaq,GRF_XAXIS); GraphAxisIntRange(gPaq,GRF_YAXIS,smin,smax);
	// GraphResize(gPaq,h,v); OpiumSizeChanged(gPaq->cdr);
	OpiumPosition(gPaq->cdr,x,y);
	GraphUse(gPaq,total); OpiumDisplay(gPaq->cdr);
	PlotNtupleAffiche(PaqEspace,paq,x,y+v,0,0);
	OpiumUserAction();
	return(1);

pas_possible:
	OpiumError("Affichage du paquet %s abandonne",paq); return(0);
}
#ifdef OPTION_STREAM
/* ========================================================================== */
static void ArchStrmFenetre(int voie) {
	Graph g,gStream[VOIES_MAX];

	if(!(g = gDonnee[voie])) {
		g = gDonnee[voie] = GraphCreate(ArchGraphLarg,ArchGraphHaut,4);
		if(!g) return;
	}
	GraphUse(g,0);
	GraphMode(g,GRF_2AXES | GRF_LEGEND);
	GraphParmsSave(g);
#ifdef A_TERMINER
	/* abcisse en millisecondes */
	x = GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,VoieTampon[voie].trmt[TRMT_AU_VOL].ab6,VoieTampon[voie].brutes->max);
	y = GraphAdd(g,GRF_SHORT|GRF_YAXIS,VoieTampon[voie].brutes->t.sval,VoieTampon[voie].brutes->max);
	GraphDataName(g,x,"t(ms)");
	GraphDataName(g,y,VoieManip[voie].nom);
	if(StrmAffiche > StrmStocke) StrmAffiche = StrmStocke;
	zoom = StrmStocke / StrmAffiche;
	// printf("Zoom pour %s: %g / %g = %d\n",f->titre,total,f->duree,zoom);
	GraphZoom(g,zoom,1);
	if(kx >= 0) GraphAxisDefine(g,2 * kx);
	if(f->m.i.min == f->m.i.max) GraphAxisAutoRange(g,GRF_YAXIS);
	else GraphAxisIntRange(g,GRF_YAXIS,f->m.i.min,f->m.i.max);
	GraphAxisAutoGrad(g,GRF_YAXIS);
	if(MonitChgtUnite) GraphAxisChgtUnite(g,GRF_YAXIS,MonitUnitesADU);
#endif
}
#endif /* OPTION_STREAM */
/* ========================================================================== */
static void ArchCouleursCree(OpiumColorState *teinte, TypeArchCouleur *couleur) {
	int voie,k; int niveau;

	for(voie = 0; voie < VoiesNb; voie++) {
		k = OpiumColorNext(teinte);
		couleur[voie].r[WND_Q_ECRAN] = OpiumColorVal[k].r;
		couleur[voie].g[WND_Q_ECRAN] = OpiumColorVal[k].g;
		couleur[voie].b[WND_Q_ECRAN] = OpiumColorVal[k].b;
		niveau = (3 * OpiumColorVal[k].r) / 4; couleur[voie].r[WND_Q_PAPIER] = (WndColorLevel)niveau;
		niveau = (3 * OpiumColorVal[k].g) / 4; couleur[voie].g[WND_Q_PAPIER] = (WndColorLevel)niveau;
		niveau = (3 * OpiumColorVal[k].b) / 4; couleur[voie].b[WND_Q_PAPIER] = (WndColorLevel)niveau;
	}
}
/* ========================================================================== */
static void ArchEvtFenetres(int stocke, char trace_unique, Graph gevt, int x0, int y0, int dx, int dy) {
	TypeEvt *event; TypeVoieDonnees *signaux; Graph gv; OpiumColorState *teinte;
	TypeDonnee base;
	char titre[64]; // char nom[MAXARCHNOM];
	int bolo,voie,vm,cap,msr,bolo_vu[MAXDETAFFICHES],nb_vus,num_vu,nb_graphs;
	int i,dim,dep,arr; int vt;
	int x,y,h,v; // xp,yp; pour positionner le n-tuple
	int temps,brut,fit,deconv; char sans_fit,sans_deconv;
	float *evt_unite; int num,max,lngr;
	char un_seul_graphique,regroupe,legende,verifie,premiere;
	
	// if(stocke) printf("(%s) Affichage demande pour l'evenement sauve #%d\n",__func__,stocke);
	// else printf("(%s) Affichage demande pour un nouvel evenement\n",__func__);
	verifie = 0;
	event = stocke? &(EvtStock[stocke-1]->evt): &ArchEvt;
	teinte = stocke? (OpiumColorState *)&(EvtStock[stocke-1]->palette): (OpiumColorState *)&ArchEvtPalette;
	OpiumColorInit(teinte); OpiumColorNext(teinte);
	ArchCouleursCree(teinte,ArchCoulStd);
	if(AfficheEvtDeconv) ArchCouleursCree(teinte,ArchCoulDec);
	regroupe = !trace_unique && ArchEvtGroupe;
	un_seul_graphique = trace_unique || ArchEvtGroupe;
	if(verifie) printf("*** Evenement #%d %s stocke: %d voie%s\n",event->num,stocke?"deja":"non",Accord1s(event->nbvoies));

	max = 0;
	if(regroupe) {
		for(vt=0; vt<event->nbvoies; vt++) if(event->voie[vt].dim > 0) {
			voie = event->voie[vt].num;
			signaux = stocke? &(EvtStock[stocke-1]->signaux[vt]): &(VoieEvent[voie]);
			if(signaux->brutes.max > max) max = signaux->brutes.max;
		}
	}

	GraphErase(gevt);
	x = y = h = v = 0; gv = 0; // GCC
	nb_vus = 0; nb_graphs = 0;
	if(!un_seul_graphique) for(vt=0; vt<event->nbvoies; vt++) if(event->voie[vt].dim > 0) {
		voie = event->voie[vt].num;
		signaux = stocke? &(EvtStock[stocke-1]->signaux[vt]): &(VoieEvent[voie]);
		if(signaux->lngr == 0) continue;
		if(AfficheVoie[VoieManip[voie].type]) {
			bolo = VoieManip[voie].det;
			if(AfficheBolo[bolo] && !regroupe) {
				if(gDonnee[voie]) GraphErase(gDonnee[voie]);
				else gDonnee[voie] = GraphCreate(ArchGraphLarg,ArchGraphHaut,4);
			}
		}
	}
	premiere = 1;
	for(vt=0; vt<event->nbvoies; vt++) if(event->voie[vt].dim > 0) {
		voie = event->voie[vt].num;
		signaux = stocke? &(EvtStock[stocke-1]->signaux[vt]): &(VoieEvent[voie]);
		if(signaux->lngr == 0) continue;
		vm = VoieManip[voie].type;
		if(verifie) {
			printf("      Entete #%d: voie %d (%s) @%08X modele #%d/%d %s, longueur %d @%g ms\n",vt,voie,
                VoieManip[voie].nom,(hexa)signaux->brutes.t.sval,vm,VOIES_TYPES,AfficheVoie[vm]?"affichable":"masque",
                signaux->brutes.max,signaux->horloge);
			printf("      Voie %s type %s: %d points sauves, dont %d avant le trigger @%g ms\n",VoieManip[voie].nom,
                ModeleVoie[vm].nom,event->voie[vt].dim,event->voie[vt].avant_evt,event->voie[vt].horloge);
		}
		// strcpy(nom,VoieManip[voie].nom);
		if(AfficheVoie[vm]) {
			bolo = VoieManip[voie].det;
			if(verifie) printf("      bolo #%d (%s) %s\n",bolo,Bolo[bolo].nom,AfficheBolo[bolo]? "affiche": "masque");
			if(AfficheBolo[bolo]) {
				for(num_vu=0; num_vu<nb_vus; num_vu++) if(bolo_vu[num_vu] == bolo) break;
				if(num_vu >= nb_vus) {
					if(nb_vus >= MAXDETAFFICHES) break;
					bolo_vu[num_vu] = bolo; nb_vus++;
				}
				if(verifie) printf("      bolo #%d (%d vus dans cet evenement)\n",num_vu,nb_vus);
				for(cap=0; cap<Bolo[bolo].nbcapt; cap++) if(Bolo[bolo].captr[cap].voie == voie) break;
				// if(cap >= Bolo[bolo].nbcapt) cap = Bolo[bolo].nbcapt - 1;
				for(i=0; i<ProdNtpNb[voie]; i++) {
					//+ printf("|  NtpVoie[%s][%d] = %d: selecte %s\n",VoieManip[voie].nom,i,ProdNtpNum[voie][i],Ntuple[ProdNtpNum[voie][i]].nom);
					PlotVarSelecte(&(Ntuple[ProdNtpNum[voie][i]]),1);
				}
				if(signaux->brutes.max > signaux->lngr) signaux->lngr = signaux->brutes.max;
				sprintf(titre,"Trace evenement #%d",event->num);
 				if(un_seul_graphique) { gv = gevt; legende = (Bolo[bolo].nbcapt > 1); }
				else {
					if(!(gv = gDonnee[voie])) continue;
					if(event->nbvoies > 1) sprintf(titre,"%s, evt #%d",VoieManip[voie].nom,event->num);
					legende = 0;
					if(verifie) printf("      -> gDonnee[voie #%d] = %08X\n",voie,(hexa)(gDonnee[voie]));
				}
				if(verifie) printf(": Temps des maxima %s\n",AfficheMemeMaxi? "alignes": "reels");
				if(AfficheMemeMaxi) ArchAb6[voie][0] = -(float)(signaux->avant_evt) * signaux->horloge;
				else ArchAb6[voie][0] = ((event->voie[vt].sec - (event->sec - EvtRunInfo[0].t0sec)) * 1000.)
					 + ((event->voie[vt].msec - (event->msec - EvtRunInfo[0].t0msec)) / 1000.);
				// ou on laisse a 0.0 ??
				ArchAb6[voie][1] = signaux->horloge;
				nb_graphs++;
				OpiumTitle(gv->cdr,titre);
				temps = GraphAdd(gv,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,&(ArchAb6[voie][0]),signaux->lngr); GraphDataName(gv,temps,"Temps(ms)");
				brut = GraphAdd(gv,GRF_SHORT|GRF_YAXIS,signaux->brutes.t.sval,signaux->lngr); GraphDataName(gv,brut,VoieManip[voie].prenom);
				if(verifie) printf(": Niveau des lignes de base %s\n",AfficheMemeLdB? "alignes": "reels");
				if(AfficheMemeLdB) {
					if(premiere) base = event->voie[vt].val[MONIT_BASE];
					else GraphDataIntOffset(gv,brut,(int)(base-event->voie[vt].val[MONIT_BASE]));
				}
				GraphDataTrace(gv,brut,GRF_HST,0); GraphDataStyle(gv,brut,GRAPH_HISTO_FLOTTANT);
				GraphAxisAutoRange(gv,GRF_XAXIS); GraphAxisAutoRange(gv,GRF_YAXIS);
//+				printf("  | ajoute %s @%08X\n",VoieManip[voie].nom,(hexa)(signaux->brutes.t.sval));
				if(verifie) printf("      Ajoute donnees %d et %d @%08X\n",temps,brut,(hexa)gv);
				if(regroupe) GraphDataRGB(gv,brut,
					ArchCoulStd[voie].r[GraphQualite],ArchCoulStd[voie].g[GraphQualite],ArchCoulStd[voie].b[GraphQualite]);
				else if(!un_seul_graphique && (bolo == BoloTrigge) && (vm == ModlTrigge)) GraphDataRGB(gv,brut,GRF_RGB_ORANGE);
				else {
					// if(GraphQualite == WND_Q_PAPIER) GraphDataRGB(gv,brut,0,WND_COLOR_3QUARTS,0);
					// else GraphDataRGB(gv,brut,GRF_RGB_GREEN);
					GraphDataSupportRGB(gv,brut,WND_Q_PAPIER,0,WND_COLOR_3QUARTS,0);
					GraphDataSupportRGB(gv,brut,WND_Q_ECRAN,GRF_RGB_GREEN);
				}
				
				if(un_seul_graphique) {
					h = ArchGraphLarg; v = ArchGraphHaut;
					x = ArchGraphPosX; y = ArchGraphPosY;
				} else if((bolo == BoloTrigge) && AfficheLoupe && (VoiesNb > 1)) {
					h = ArchGraphLarg * 2; v = ArchGraphHaut * 2;
					x = x0;
					if(x < ArchGraphPosX) x = ArchGraphPosX; else if(x >= OpiumServerWidth(0)) x = OpiumServerWidth(0) - dx;
					y = (cap * (2 * dy)) + y0;
					if(y < ArchGraphPosY) y = ArchGraphPosY; else if(y >= OpiumServerHeigth(0)) y = OpiumServerHeigth(0) - dy;
					// xp = x; yp = y; pour positionner le n-tuple
				} else /* if(VoiesNb > 1) */ {
					h = ArchGraphLarg; v = ArchGraphHaut;
					// OpiumPosition(gv->cdr,(num_vu * dx) + x0,(cap * dy) + y0);
					x = ((num_vu % MAXDETENLARGEUR) * dx) + x0;
					if(x >= OpiumServerWidth(0)) x = OpiumServerWidth(0) - dx;
					y = ((cap + ArchVoiesMaxAff * ((num_vu / MAXDETENLARGEUR) % MAXDETENLARGEUR)) * dy) + y0 + 10 * ((num_vu / MAXDETENLARGEUR) % MAXDETENLARGEUR);
					if(y < ArchGraphPosY) y = ArchGraphPosY; else if(y >= OpiumServerHeigth(0)) y = OpiumServerHeigth(0) - dy;
					// xp = x; yp = y; pour positionner le n-tuple
				}
				if(h < 50) h = 50; if(v < 40) v = 40; if(x < 15) x = 15; if(y < 40) y = 40;
				GraphResize(gv,h,v);
				OpiumSizeChanged(gv->cdr);
				OpiumPosition(gv->cdr,x,y);
				//-	printf("(%s) Event#%d %d x %d place en (%d, %d)\n",__func__,event->num,ArchGraphLarg,ArchGraphHaut,x,y);
				if(regroupe) GraphUse(gv,max); else GraphUse(gv,signaux->brutes.max);
				sans_fit = 1;
				if(AfficheEvtCalib) {
					if(verifie) printf("      Superposition de l'evt fitte prevue\n");
					if((num = EvtUniteCherche(VoieManip[voie].nom,&evt_unite,&lngr,&dep,&arr)) >= 0) {
						if(verifie) printf("      EvtUniteCherche retourne %d\n",num);
						dim = signaux->brutes.max;
						if(!VoieFittee[voie]) {
							VoieFittee[voie] = (float *)malloc(dim * sizeof(float));
							if(verifie) printf("      malloc(%d x 4) retourne VoieFittee[voie #%d] = %08X\n",dim,voie,(hexa)(VoieFittee[voie]));
						}
						if(dim > lngr) dim = lngr;
						if(VoieFittee[voie]) {
							for(i=0; i<dim; i++) {
								VoieFittee[voie][i] = (evt_unite[i] * Energie[voie][EvtDemande]) + Base[voie][EvtDemande];
							}
							fit = GraphAdd(gv,GRF_FLOAT|GRF_YAXIS,VoieFittee[voie],dim);
							GraphDataTrace(gv,fit,GRF_HST,0); GraphDataStyle(gv,fit,GRAPH_HISTO_FLOTTANT);
							// if(GraphQualite == WND_Q_PAPIER) GraphDataRGB(gv,fit,GRF_RGB_RED);
							// else GraphDataRGB(gv,fit,GRF_RGB_YELLOW);
							GraphDataSupportRGB(gv,fit,WND_Q_PAPIER,WND_COLOR_DEMI,WND_COLOR_DEMI,0);
							GraphDataSupportRGB(gv,fit,WND_Q_ECRAN,GRF_RGB_YELLOW);
							sprintf(titre,"fit %s",VoieManip[voie].nom);
							GraphDataName(gv,fit,titre); legende = 1;
						#ifdef CHANGE_UNITE
							ArchCalib[voie] = EvtUniteAmplitude(VoieManip[voie].nom);
							GraphAxisChgtUnite(gv,GRF_YAXIS,(float(*)(void))ArchUnitesADU);
							// if((bolo == 1) && (voie == 0)) printf("calib[%d][%d]=%g\n",bolo,voie,ArchCalib[voie]);
						#endif /* CHANGE_UNITE */
							sans_fit = 0;
						}
					}
				}
				sans_deconv = 1;
				dim = signaux->brutes.max;
				VoieDeconv[voie] = SambaAssure(VoieDeconv[voie],dim,&(VoieDeconvDim[voie]));
				msr = VoieTemps[voie].mesure;
				/* montre_int(signaux->brutes.max); montre_int(VoieDeconvDim[voie]); */
				if(AfficheEvtDeconv && VoieDeconv[voie] && dim && (msr >= 0)) {
					float perte,seuil;
					if(verifie) printf("      Superposition de l'evt deconvolue prevue\n");
					perte = expf(-signaux->horloge / VoieTemps[msr].deconv.rc);
					seuil = VoieTemps[msr].deconv.seuil;
					SambaDeconvolue(VoieDeconv[voie],(TypeSignee *)signaux->brutes.t.sval,dim,0,dim,
						Base[voie][event->num-1],VoieTemps[msr].deconv.pre,VoieTemps[msr].deconv.post,perte,seuil,CHARGE_NORMALISE);
					deconv = GraphAdd(gv,GRF_FLOAT|GRF_YAXIS,VoieDeconv[voie],dim);
					GraphDataTrace(gv,deconv,GRF_HST,0); GraphDataStyle(gv,deconv,GRAPH_HISTO_FLOTTANT);
					// if(GraphQualite == WND_Q_PAPIER) GraphDataRGB(gv,deconv,GRF_RGB_RED);
					// else GraphDataRGB(gv,deconv,GRF_RGB_YELLOW);
					if(regroupe) GraphDataRGB(gv,deconv,
						ArchCoulDec[voie].r[GraphQualite],ArchCoulDec[voie].g[GraphQualite],ArchCoulDec[voie].b[GraphQualite]);
					else {
						GraphDataSupportRGB(gv,deconv,WND_Q_PAPIER,0,0,WND_COLOR_3QUARTS);
						GraphDataSupportRGB(gv,deconv,WND_Q_ECRAN,GRF_RGB_BLUE);
					}
					sprintf(titre,"%s deconv",VoieManip[voie].nom);
					GraphDataName(gv,deconv,titre); legende = 1;
					sans_deconv = 0;
				}
				if(verifie) {
					if(sans_fit) printf("      Pas de superposition de l'evt fitte\n");
					if(sans_deconv) printf("      Pas de superposition de l'evt deconvolue\n");
					printf("      Affichage de gDonnee[voie #%d] = %08X\n",voie,(hexa)gv);
				}
				if(legende) GraphMode(gv,GRF_2AXES|GRF_GRID|GRF_LEGEND);
				else GraphMode(gv,GRF_2AXES|GRF_GRID);
				GraphAxisReset(gv,GRF_XAXIS);
				GraphAxisReset(gv,GRF_YAXIS);
				if(OpiumAlEcran(gv)) OpiumClear(gv->cdr);
				if(un_seul_graphique || (bolo != BoloTrigge) || !AfficheLoupe) OpiumDisplay(gv->cdr);
				premiere = 0;
			}
		}
	}
	if(!nb_graphs) {
		if(!iVide) iVide = InfoCreate(1,80);
		if(iVide) {
			sprintf(titre,"Evenement #%d",event->num);
			InfoTitle(iVide,titre);
			InfoWrite(iVide,1,"Evenement #%d vide",event->num);
			OpiumDisplay(iVide->cdr);
		}
		printf(". Evenement #%d vide\n",event->num);
		return;
	} else if(iVide) {
		OpiumClear(iVide->cdr);
		InfoDelete(iVide);
		iVide = 0;
	}

	EvtDemandeMontre();
	if(!regroupe && AfficheLoupe && (VoiesNb > 1)) {
		for(vt=0; vt<event->nbvoies; vt++) if(event->voie[vt].dim > 0) {
			voie = event->voie[vt].num;
			vm = VoieManip[voie].type;
			if(AfficheVoie[vm]) {
				bolo = VoieManip[voie].det;
				if(bolo == BoloTrigge) {
					//- printf("(%s) Affichage evt #%d, bolo #%d voie #%d (modele #%d), graphe @%08X\n",__func__,event->num,bolo,voie,vm,(hexa)(gDonnee[voie]));
					if(gDonnee[voie]) OpiumDisplay(gDonnee[voie]->cdr);
					/* else {
						if(nb_vus >= MAXDETAFFICHES) OpiumError("Detecteur %s pas affiche (maxi: %d)",Bolo[bolo].nom,MAXDETAFFICHES);
						else OpiumError("Memoire insuffisante pour l'affichage de la voie %s",VoieManip[voie].nom);
					} */
				}
			}
		}
	}

	ArchNtuplePlace(gv,un_seul_graphique,event->num);
	if(verifie) printf(". Evenement #%d affiche\n",event->num);
}
/* ========================================================================== */
static void ArchNtuplePlace(Graph g, char un_seul_graphique, int evt) {
	int x,y,h,v,hn,vn; int paq;

	if(!AfficheNtuple) return;
	if(g) {
		if(un_seul_graphique) {
			OpiumGetPosition(g->cdr,&x,&y); OpiumGetSize(g->cdr,&h,&v);
		} else x = y = h = v = -1;
		PlotNtupleAffiche(EvtEspace,evt-1,x,y,h,v);
		if(EvtPaquetNum) {
			paq = (int)(EvtPaquetNum[evt-1]) - 1;
			OpiumUserAction();
			OpiumGetSize((EvtEspace->pNtuple)->cdr,&hn,&vn);
			PlotNtupleAffiche(PaqEspace,paq,x - hn - WND_ASCINT_WIDZ - 2,y,h,v);
		}
		OpiumUserAction();
	}
}
/* ========================================================================== */
void ArchCreeUI() {
	pHdrRun = PanelDesc(ArchHdrRun,1);
	OpiumPosition(pHdrRun->cdr,100,50);
	pHdrEvt = PanelDesc(ArchHdrEvt,1);
	OpiumPosition(pHdrEvt->cdr,800,50);
	pVoieEvt = 0;
	OpiumProgresCreate();
}
/* ========================================================================== */
int ArchInit(char *optn, char *nom) {
	char nom_complet[MAXFILENAME];
    int voie;

	if(*nom) printf("= Archive demandee sous la reference: '%s'\n",nom);
	else printf("= L'archive a relire n'a pas ete precisee au moment de l'appel\n");
	printf("= Parametres specifiques demandes: '%s'\n",optn);
	ArchInitDisplayTodo = 1;
	ArchEvtGroupe = 1;
	AfficheLoupe = ~ArchEvtGroupe;
	ArchPosHdrEvt = ArchPosVoies = ArchPosTouches = 1; /* vrai si positionnement a faire */
	ArchPosHdrEvtX = 500; ArchPosHdrEvtY = 50;
	ArchPosVoiesX = 500; ArchPosVoiesY = 400;
	ModeleVoieNb = 2;
	VsnManip = 2;
	RunNumber = 0; RunDate[0] = '\0'; strcpy(RunType,"fond");
	NomArchive[0] = '\0';
	strcpy(ProdCalibPath,EvtsPath);
	strcpy(ProdVarsName,"Variables");
	strcpy(ProdTempsName,"ZonesTemps");
	strcpy(ProdCalibName,"CoefsCalibration");
	strcpy(ProdCtesName,"CoefsAnalyse");
	strcpy(ProdChaleurName,"CoefsChaleur");
	EvtAffiche = -1;
	EvtStockNb = 0;
	for(voie=0; voie<VOIES_MAX; voie++) {
		VoieFittee[voie] = VoieDeconv[voie] = 0;
		VoieDeconvDim[voie] = 0;
		VoieRC[voie] = 0.0;
	}
#ifdef CHANGE_UNITE
	{
		for(voie=0; voie<VOIES_MAX; voie++) ArchCalib[voie] = 1.0;
	}
#endif
	
	/* Valeurs par defaut pour les variables relues */
	Version = 0.0;
//	SambaDefaults();
//	SettingsDefaults();
//	TrmtNomme();
	ArchT0sec = 0; ArchT0msec = 0; sprintf(ArchHeure,"(inconnue)");

	/* Initialisation des variables propres de Tango-EDW */
	ArchGraphPosX = 15; ArchGraphPosY = 40;
	ArchGraphLarg = 500; ArchGraphHaut = 300;
	BoloTrigge = ModlTrigge = 0;

	/* Variables deduites de l'analyse elle-meme */
	PolarIonisation = 4.0;
	EnergieRef = 122.0;
	RefChal = 4000.0; RefIonis = -2600;
	SeuilChal = SeuilIonis = 4.5;
	ResolChalBase = 0.5;
	ResolIonisBase = 1.0;
	ResolChalRef = 2.0;
	ResolIonisRef = 1.5;
	NeutronsFact = 0.16;
	NeutronsExp = 0.18;
	TauxCentrePur = 0.75;

/* EDW2 */
	ArchEvt.num = 0;
	ProdMontee10 = 10.0;
	ProdMontee90 = 90.0;
	ProdQuotaMontee = 0.5;

	if(optn[0]) {
		ArgSplit(optn,'/',ArchArgs);
	/*	strcat(strcpy(nom,EvtsPath),"amjjrrr"); */
		if(NomArchive[0] == ':') strcpy(nom,NomArchive+1);
		else if(NomArchive[0]) strcpy(nom,NomArchive);
		else if(RunDate[0]) sprintf(nom,"%s%03d",RunDate,RunNumber);
		else if((VsnManip == 1) && (RunNumber > 0)) sprintf(nom,"R%06d",RunNumber);
		else if((VsnManip == 2) && (RunNumber > 0)) sprintf(nom,"%s%03d",RunType,RunNumber);
	}
	printf("= Parametres specifiques utilises:\n");
	ArgIndent(1); ArgPrint("*",ArchArgs); ArgIndent(0);

	strcat(strcpy(nom_complet,AnaPath),ProdCtesName);
	ArgScan(nom_complet,ProdCtes);
	printf("= Coefficients d'analyse utilises\n  (definis dans %s)\n",nom_complet);
	ArgIndent(1); ArgPrint("*",ProdCtes); ArgIndent(0);

	strcat(strcpy(ProdVarsFile,AnaPath),ProdVarsName);
	strcat(strcpy(ProdTempsFile,AnaPath),ProdTempsName);
	strcat(strcpy(ProdCalibFile,AnaPath),ProdCalibName);
	strcat(strcpy(ProdCtesFile,AnaPath),ProdCtesName);
	strcat(strcpy(ProdChaleurFile,AnaPath),ProdChaleurName);

	return(1);
}
/* ========================================================================== */
int ArchExit() {
	char nom_complet[MAXFILENAME];

	strcat(strcpy(nom_complet,AnaPath),ProdCtesName);
	ArgPrint(nom_complet,ProdCtes);
	return(1);
}
