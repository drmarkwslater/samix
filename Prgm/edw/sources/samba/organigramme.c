/*
 *  organigramme.c
 *  pour le projet Samba
 *
 *  Created by Michel GROS on 23.10.12.
 *  Copyright 2012+2013 CEA/DSM/DAPNIA. All rights reserved.
 *
 */

#define ORGANIGRAMME_C

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <decode.h>
#include <dateheure.h>

#include <samba.h>
#include <objets_samba.h>
#include <detecteurs.h>
#include <numeriseurs.h>
#include <cablage.h>
#include <repartiteurs.h>

#include <organigramme.h>
// #include <Icones/IconeMac.h>
#include <Icones/OrgaIconeMacMini.h>
#include <Icones/OrgaIconeDetec.h>
#include <Icones/OrgaIconeNumer.h>
#include <Icones/OrgaIconeConnec.h>
#include <Icones/OrgaIconeSlot.h>
#include <Icones/OrgaIconeCable.h>
#include <Icones/OrgaIconeDetecGd.h>
#include <Icones/OrgaIconeCableGd.h>
#include <Icones/OrgaIconeNumerGd.h>
#include <Icones/OrgaIconeRepart.h>
#include <Icones/OrgaIconeGalette.h>
#include <Icones/OrgaIconeAcquis.h>
#include <Icones/OrgaIconePoubelle.h>

#ifdef AVEC_BMP
#include <Icones/IconeEdw.h>
#endif

#define AVEC_ICONE
#define AVEC_ICONE_NUMER
#define AVEC_ICONE_CONNEC

// #define CLIC_LARGE
#ifdef CLIC_LARGE
#define ICONE_HAUTEUR 20
#define ICONE_MARGE 10
#define ICONE_ETAGE 40
#define ICONE_INTERVALLE 20
#else
#define ICONE_HAUTEUR 14
#define ICONE_MARGE 6
//#define ICONE_ETAGE 32
#define ICONE_ETAGE 16
#define ICONE_INTERVALLE 16
#endif
#define ICONE_DECAL 2

#define ORGA_MAX_ETAGERE 128

static char *OrgaMagListe[ORGA_MAG_MAX+2] = {
	"  Lecture  ", "Repartition", "Numeriseurs", "  Cablage  ", "Detecteurs ", "\0"
};
#ifdef IDEE_NO_1
static char *OrgaMagCles = "lecture/detecteurs/cablage/numeriseurs";
#endif

// ORGA_REF_OBJET
typedef enum {
	/* references d'objets deplacables */
	ORGA_REF_MODELE = 0,
	ORGA_REF_REPART,
	ORGA_REF_NUMER,
	ORGA_REF_CABLE,
	ORGA_REF_DETEC,
	/* references d'objets non deplacables */
	ORGA_REF_POSITION,
	ORGA_REF_FISCHER,
	ORGA_REF_FIBRE,
	ORGA_REF_AJOUTE,
	ORGA_REF_RETIRE,
	ORGA_REF_TOUT,
	ORGA_REF_RIEN,
	
	ORGA_REF_NB
} ORGA_REF;
static char *OrgaRefListe[ORGA_REF_NB+1] = {
	"modele", "repartiteur", "numeriseur", "cablage", "detecteur",
	"position", "connecteur", "fibre", "ajout", "retrait", "\0"
};

typedef struct {
	char type;
	void *adrs;
	Cadre cdr;
} TypeOrgaRef,*OrgaRef;

typedef struct {
	TypeRepartiteur *repart;
	int l;
} TypeOrgaRepEntree,*OrgaRepEntree;

#define ORGA_MAXOBJ_NUMER (NUMER_TYPES + ORGA_MAX_ETAGERE + NUMER_MAX)
#define ORGA_MAXOBJ_DETEC (DETEC_TYPES + ORGA_MAX_ETAGERE + DETEC_MAX)
#define ORGA_MAXOBJ_CABLG (CABLAGE_TYPE_MAX + CABLAGE_POS_MAX)
#define ORGA_MAXOBJ (ORGA_MAXOBJ_NUMER + ORGA_MAXOBJ_DETEC + ORGA_MAXOBJ_CABLG)
static TypeOrgaRef OrgaObjet[ORGA_MAXOBJ];
static TypeOrgaLiaisons LiaisonsDaqRepart,LiaisonsRepartNumer,LiaisonsCablage;

#ifdef DEPLACE_FANTOME
static TypeFigZone  OrgaCadreVide     = {  80, ICONE_HAUTEUR, WND_RAINURES,  0x0000, 0x0000, 0x0000 };
#endif

static TypeFigZone   OrgaZoneMenu     = {  80, ICONE_HAUTEUR, WND_PLAQUETTE, 0xBFFF, 0xBFFF, 0xBFFF };
static TypeFigZone   OrgaZoneMac      = {  80, ICONE_HAUTEUR, WND_PLAQUETTE, 0x7FFF, 0x7FFF, 0xFFFF };
static TypeFigZone   OrgaZoneRepartOK = { 100, ICONE_HAUTEUR, WND_PLAQUETTE, GRF_RGB_GREEN  };
static TypeFigZone   OrgaZoneNumer    = { 100, ICONE_HAUTEUR, WND_PLAQUETTE, GRF_RGB_ORANGE };
static TypeFigZone   OrgaZoneSlot     = {  25, ICONE_HAUTEUR, WND_RAINURES,  0xBFFF, 0x3FFF, 0xFFFF  };
static TypeFigZone   OrgaZoneVoie     = { 100, ICONE_HAUTEUR, WND_PLAQUETTE, GRF_RGB_YELLOW };
static TypeFigZone   OrgaZoneDetec    = {  80, ICONE_HAUTEUR, WND_PLAQUETTE, 0x7FFF, 0x7FFF, 0xFFFF };

static TypeFigZone   OrgaZoneAdc      = {  30, ICONE_HAUTEUR, WND_RAINURES,  0x7FFF, 0x3FFF, 0x0000  };
static TypeFigZone   OrgaZoneCapt     = {  60, ICONE_HAUTEUR, WND_RAINURES,  GRF_RGB_YELLOW };
static TypeFigZone   OrgaZoneSoft     = {  30, ICONE_HAUTEUR, WND_RAINURES,  0x3FFF, 0x1FFF, 0x0000  };
// static TypeFigZone OrgaZoneEmetteur  = {   5, 5, WND_PLAQUETTE,  0xBFFF, 0x3FFF, 0xFFFF  };

static TypeFigZone  OrgaZoneInactive  = { 100, ICONE_HAUTEUR, WND_RAINURES,  0x7FFF, 0x0000, 0x3FFF };
static TypeFigZone  OrgaZoneRepartHS  = { 100, ICONE_HAUTEUR, WND_PLAQUETTE, 0x0000, 0x7FFF, 0x0000 };
static TypeFigZone  OrgaZoneNumerHS   = { 100, ICONE_HAUTEUR, WND_PLAQUETTE, 0x7FFF, 0x3FFF, 0x0000 };

static TypeFigDroite OrgaTraitArrivee = { 30, 0, WND_LIGNES, 0x3FFF, 0x1FFF, 0x0000  };
static TypeFigZone   OrgaZoneFibre    = { 30, 5, WND_PLAQUETTE, 0xFFFF, 0xBFFF, 0x7FFF  };

// static TypeFigZone   OrgaZoneIpe      = {  80, ICONE_HAUTEUR, WND_PLAQUETTE, GRF_RGB_GREEN   };
// static TypeFigZone   OrgaZoneFLT      = {  70, (6*(ICONE_HAUTEUR+ICONE_MARGE)) - ICONE_MARGE, WND_RAINURES, 0x3FFF, 0x7FFF, 0x3FFF }; // WND_PLAQUETTE?
static TypeFigZone   OrgaZoneIpe      = { 105, ICONE_HAUTEUR, WND_PLAQUETTE, GRF_RGB_GREEN   };
static TypeFigZone   OrgaZoneFLT      = {  95, (6*(ICONE_HAUTEUR+ICONE_MARGE)) - ICONE_MARGE, WND_RAINURES, 0x3FFF, 0x7FFF, 0x3FFF }; // WND_PLAQUETTE?

static TypeFigZone   OrgaZoneModl     = {  80, ICONE_HAUTEUR, WND_PLAQUETTE, 0x7FFF, 0x7FFF, 0xFFFF };
static TypeFigZone   OrgaZoneModlDble = {  80, (2 * ICONE_HAUTEUR) + 7, WND_RAINURES,  0x7FFF, 0x7FFF, 0xFFFF };
static TypeFigZone   OrgaZonePlus     = {  60, ICONE_HAUTEUR, WND_PLAQUETTE, GRF_RGB_GREEN   };
static TypeFigZone   OrgaZoneMoins    = {  60, ICONE_HAUTEUR, WND_PLAQUETTE, 0xFFFF, 0xBFFF, 0x7FFF };
static TypeFigZone   OrgaZoneSelOui   = {  80, ICONE_HAUTEUR, WND_PLAQUETTE, GRF_RGB_GREEN   };
static TypeFigZone   OrgaZoneSelNon   = {  80, ICONE_HAUTEUR, WND_PLAQUETTE, 0xFFFF, 0xBFFF, 0x7FFF };
static TypeFigZone   OrgaZoneSelOut   = {  80, ICONE_HAUTEUR, WND_PLAQUETTE, 0x7FFF, 0x7FFF, 0x7FFF };

static TypeFigDroite OrgaTraitSepareMag = { 0, 0, WND_REGLETTES, 0x3FFF, 0x3FFF, 0x3FFF };
static TypeFigDroite OrgaTraitSepareGal = { 0, 0, WND_REGLETTES, 0x3FFF, 0x3FFF, 0x3FFF };
static TypeFigDroite OrgaTraitSepareIcn = { 0, 0, WND_RAINURES,  0x3FFF, 0x3FFF, 0x3FFF };

static TypeFigZone   OrgaZoneRepart[REPART_MAX];

typedef struct { void *modl; short larg,haut; } TypeOrgaFig;
static FIG_TYPE OrgFigType[ORGA_STYLES_NB] = { FIG_ZONE, FIG_ICONE };

//#define ORGAFIG(nom) { (void *)&nom, nom.larg, nom.haut }
//static TypeOrgaFig OrgaFigDetec[ORGA_STYLES_NB] = { ORGAFIG(OrgaZoneDetec), ORGAFIG(OrgaIconeDetec) };

#define OrgaDeclareFigures(nom,zone,icone) \
	{ \
		nom[ORGA_ZONES].modl = (void *)(&zone);   \
		nom[ORGA_ZONES].larg = zone.larg;         \
		nom[ORGA_ZONES].haut = zone.haut;         \
		nom[ORGA_ICONES].modl = (void *)(&icone); \
		nom[ORGA_ICONES].larg = icone.larg;       \
		nom[ORGA_ICONES].haut = icone.haut;       \
	}

static TypeOrgaFig OrgaFigDetec[ORGA_STYLES_NB];
static TypeOrgaFig OrgaFigNumer[ORGA_STYLES_NB];
static TypeOrgaFig OrgaFigConnec[ORGA_STYLES_NB];
static TypeOrgaFig OrgaFigCable[ORGA_STYLES_NB];
static TypeOrgaFig OrgaFigSlot[ORGA_STYLES_NB];
static TypeOrgaFig OrgaFigGalette[ORGA_STYLES_NB];
static TypeOrgaFig OrgaFigRepart[ORGA_STYLES_NB];
static TypeOrgaFig OrgaFigDetecGd[ORGA_STYLES_NB];
static TypeOrgaFig OrgaFigCableGd[ORGA_STYLES_NB];
static TypeOrgaFig OrgaFigNumerGd[ORGA_STYLES_NB];
static TypeOrgaFig OrgaFigLecVoie[ORGA_STYLES_NB];

#ifdef AVEC_BMP
static WndIcone OrgaIcnEdw,OrgaIcnDetec;
#endif

static char  *OrgaFeuillesNom[] = { "Donnees", "Cablage", /* "Detecteurs", "Numeriseurs", */ "\0" };
static Onglet OrgaFeuilles;

#define ORGA_GALETTES_LARG 2
#define ORGA_GALETTES_MAX 16
#define ORGA_TOURS_MAX 16
static int OrgaGalettesNb;
// static char  *OrgaGalettesNom[OrgaGalettesNb + 1];
static char **OrgaGalettesNom;
static Onglet OrgaGalettes;

#define ORGA_QUADRANT_MAX 8
#define ORGA_RAIL_MAX 32

static Figure OrgaNumerFig[NUMER_MAX+1];
static char   OrgaSelectNumerUnique;
static Figure OrgaDetecFig[DETEC_MAX+1];
static char   OrgaSelectDetecUnique;

static char   OrgaPrete;
static int    OrgaChassisX0,OrgaChassisY0,OrgaDxQuadrant,OrgaDxGalette,OrgaDyTour,OrgaLngrFibre;
static int    OrgaCablageMontre;
static int    OrgaSelectNumerNb,OrgaSelectDetecNb;
static Panel pOrgaSelectNumerNb,pOrgaSelectDetecNb;
static short  OrgaDerniereAction;
static Figure OrgaDerniereFigure,OrgaFigurePressee;
static Figure OrgaPoubelle,OrgaDetecPoubelle;
static int    OrgaFinUsine,OrgaFinEtagere,OrgaDetecLargeur;
static char  *OrgaNumerListe[ORGA_MAX_ETAGERE];
static int    OrgaNumerNb;
static int    OrgaNumerNum[ORGA_MAX_ETAGERE];
static char  *OrgaDetecListe[ORGA_MAX_ETAGERE];
static int    OrgaDetecNb;
static int    OrgaDetecNum[ORGA_MAX_ETAGERE];
static int    OrgaGaletteDx;
static short  OrgaGalMin,OrgaGalMax;
static struct { shex xinf,yinf,xsup,ysup; } OrgaPositionSlot[ORGA_GALETTES_MAX][ORGA_TOURS_MAX];
static struct { shex xinf,yinf,xsup,ysup; } OrgaPositionFischer[ORGA_QUADRANT_MAX][ORGA_RAIL_MAX];
static OrgaRepEntree OrgaRefEntree;
static int OrgaNbEntrees;

extern ArgStruct RepartiteurAS;

int ReglageClique(Figure fig, WndAction *e);
void RepartiteurBranche(TypeRepartiteur *repart, int l, TypeNumeriseur *numeriseur);

static void OrgaMagasinOnglet(int numero);
static int  OrgaMagasinReconstruit(Menu menu, MenuItem item);
static void OrgaRafraichi(int numero);
static void OrgaEspaceRepartition(char mode, int xmin, int ymin);
static int OrgaEspaceDetecMontre();
static int OrgaEspaceNumerMontre();
static int OrgaCliqueSurSlot(Figure cliquee, WndAction *e);
static int OrgaCliqueSurFlt(Figure fig, WndAction *e);
static int OrgaCablageChoixAdc(Figure fig, WndAction *e);

#ifdef A_CONTINUER
static Menu mCliqueSurMac; 
static TypeMenuItem iCliqueSurMac[] = {
	{ "Ajout repartiteur", MNU_FONCTION RepartiteurAjoute },
	{ "Fermer",            MNU_RETOUR },
	MNU_END
};
#endif

/* ========================================================================== 
static int OrgaSelection(Menu menu, MenuItem item) {
	int j,n;

	OrgaMagMode = MenuItemGetIndex(menu,(int)item);
	n = MenuItemNb(menu);
	for(j=0; j<n; j++) if(j == OrgaMagMode) MenuItemAllume(menu,(int)item,0,GRF_RGB_GREEN);
	else MenuItemEteint(menu,(int)item,0);
	return(0);
}
   ========================================================================== 
static Menu OrgaSelecteur(char *choix, char *var) {
	Menu menu; int i,n; char texte[80];
	
	menu = MenuLocalise(0);
	n = ArgKeyGetNb(choix);
	if(MenuItemArray(menu,n)) for(i=0; i<n; i++) {
		ArgKeyGetText(choix,i,texte,80); // texte pas copie, donc affichage aleatoire
		MenuItemAdd(menu,texte,MNU_FONCTION OrgaSelection);
	} else { MenuDelete(menu); menu = 0; }
	return(menu);
}
   .......................................................................... 
static int OrgaAnnule(Figure cliquee, WndAction *e) {
	OrgaReponse = -1; return(1);
}
   .......................................................................... 
static int OrgaContinue(Figure cliquee, WndAction *e) {
	OrgaReponse = 1; return(1);
}
   .......................................................................... 
static Menu mOrgaStopStart;
static TypeMenuItem iOrgaStopStart[] = {
 	{ "Annuler",  MNU_FONCTION OrgaAnnule },
 	{ "Utiliser", MNU_FONCTION OrgaContinue },
 	MNU_END
};
   ========================================================================== */
static int OrgaFermeSelectionNumeriseurs(Cadre cdr, void *arg) {
	//	if(!OrgaNumeriseursChoisis) OrgaNumeriseursChoisis = -1;
	OrgaNumeriseursChoisis = (bNumeriseurSelecte->code_rendu == 1)? -1: 1;
	return(1);
}
/* .......................................................................... */
static int OrgaFermeSelectionDetecteurs(Cadre cdr, void *arg) {
	OrgaDetecteursChoisis = (bDetecteurSelecte->code_rendu == 1)? -1: 1;
	return(1);
}
/* ========================================================================== */

static TypeMenuItem iOrgaProcs[] = {
	{ "Demarrage repartiteurs",     MNU_FONCTION RepartiteurDemarre },
	{ "Chargement FPGA numeriseur", MNU_FONCTION SambaChargeFpga },
	{ "Demarrage numeriseurs",      MNU_FONCTION NumeriseursDemarre },
	{ "Reconnaissance numeriseurs", MNU_FONCTION LectIdentBB },
	{ "Mise en service detecteurs", MNU_FONCTION DetecteurMiseEnService },
//	{ "Spectres de bruit",          MNU_FONCTION LectSpectres },
	{ "Spectres de bruit",          MNU_FORK   &bLectSpectres },
	{ "Inversion polarisation",     MNU_FONCTION DetecteurFlip },
	MNU_END
};
/* ========================================================================== */
void OrgaInit() {
	int mode,bb,bolo;

	OrgaPrete = 0;
	OrgaMagMode = 0; // soit mode=lecture par defaut [avant: ORGA_MAG_REPART];
	bSchemaManip = bEspaceNumer = bEspaceDetec = bOrgaCryostat = bSchemaCablage = 0;
	LiaisonsDaqRepart.lien = LiaisonsRepartNumer.lien = LiaisonsCablage.lien = 0;
	OrgaNbEntrees = 0; OrgaRefEntree = 0;
	OrgaZoneFibre.larg = OrgaLngrFibre = WndColToPix(4);
	OrgaDetecLargeur = OrgaLngrFibre + OrgaZoneNumer.larg + (2 * OrgaZoneSlot.larg) + OrgaZoneAdc.larg + OrgaZoneCapt.larg + OrgaZoneDetec.larg + (3 * ICONE_ETAGE) + 8;
	OrgaDxQuadrant = OrgaLngrFibre + 2 + OrgaZoneSlot.larg + OrgaZoneRepartOK.larg + 4 + ICONE_ETAGE;
	OrgaDxGalette = OrgaZoneSlot.larg + OrgaZoneRepartOK.larg + ICONE_ETAGE;
	OrgaDyTour = OrgaZoneSlot.haut + ICONE_MARGE;
	OrgaCablageMontre = 0;
	OrgaSelectNumerNb = 0;
	OrgaDerniereAction = -1;
	OrgaDerniereFigure = OrgaFigurePressee = 0;
	OrgaPoubelle = OrgaDetecPoubelle = 0;
	bNumeriseurSelecte = bDetecteurSelecte = 0;
	OrgaSelectNumerUnique = OrgaSelectDetecUnique = 1;
	OrgaNumeriseursChoisis = OrgaDetecteursChoisis = 0;
	pOrgaSelectNumerNb = pOrgaSelectDetecNb = 0;
	OrgaRepartAfficheCryo = 0;
	for(mode=ORGA_MAG_REPART; mode<ORGA_MAG_MAX; mode++) OrgaMagModifie[mode] = 0;
	for(bb=0; bb<NUMER_MAX; bb++) OrgaNumerFig[bb] = 0;
	for(bolo=0; bolo<DETEC_MAX; bolo++) OrgaDetecFig[bolo] = 0;
//--	mOrgaStopStart = MenuLocalise(iOrgaStopStart); MenuColumns(mOrgaStopStart,2); OpiumSupport(mOrgaStopStart->cdr,WND_PLAQUETTE);
	
	OrgaDeclareFigures(OrgaFigDetec,OrgaZoneDetec,OrgaIconeDetec);
	OrgaDeclareFigures(OrgaFigNumer,OrgaZoneRepartOK,OrgaIconeNumer); /* OrgaZoneNumer */
	OrgaDeclareFigures(OrgaFigConnec,OrgaZoneSlot,OrgaIconeConnec);
	OrgaDeclareFigures(OrgaFigCable,OrgaZoneNumer,OrgaIconeCable);
	OrgaDeclareFigures(OrgaFigSlot,OrgaZoneSlot,OrgaIconeSlot);
	OrgaDeclareFigures(OrgaFigGalette,OrgaZoneRepartOK,OrgaIconeGalette);
	OrgaDeclareFigures(OrgaFigRepart,OrgaZoneRepartOK,OrgaIconeRepart);
	OrgaDeclareFigures(OrgaFigDetecGd,OrgaZoneDetec,OrgaIconeDetecGd);
	OrgaDeclareFigures(OrgaFigCableGd,OrgaZoneNumer,OrgaIconeCableGd);
	OrgaDeclareFigures(OrgaFigNumerGd,OrgaZoneRepartOK,OrgaIconeNumerGd); /* OrgaZoneNumer */
	OrgaDeclareFigures(OrgaFigLecVoie,OrgaZoneCapt,OrgaIconeAcquis);
#ifdef AVEC_BMP
	{
		char explics[256]; char nom_complet[MAXFILENAME]; 
		sprintf(nom_complet,"%sedw.bmp",SambaInfoDir);
		if(!(OrgaIcnEdw = WndIconeFromBmpFile(nom_complet,explics))) printf("! %s: %s",nom_complet,explics);
		sprintf(nom_complet,"%sdetec.bmp",SambaInfoDir);
		if(!(OrgaIcnDetec = WndIconeFromBmpFile(nom_complet,explics))) printf("! %s: %s",nom_complet,explics);
	}
#endif

}
/* ========================================================================== */
void OrgaSetup() {
	TypeChassisAxe chassis;
	int i; short m,n; char prem[8],der[8];
	
	OrgaGaletteDx = (OrgaLngrFibre + OrgaZoneRepartOK.larg + OrgaZoneSlot.larg +  OrgaZoneAdc.larg + OrgaZoneCapt.larg + OrgaZoneDetec.larg + (5 * ICONE_ETAGE));
	memcpy(&chassis,&(ChassisDetec[0]),sizeof(TypeChassisAxe));
	OrgaGalettesNb = (((ChassisDetec[0].nb - 1) / ORGA_GALETTES_LARG) + 1);
	OrgaGalettesNom = (char **)malloc((OrgaGalettesNb+1)*sizeof(char *));
	for(i=0; i<OrgaGalettesNb; i++) {
		if((OrgaGalettesNom[i] = (char *)malloc(8))) {
			n = ORGA_GALETTES_LARG * i;
			m = ORGA_GALETTES_LARG * (i + 1); if(m > GALETTES_NB) m = GALETTES_NB; m--;
			// sprintf(OrgaGalettesNom[i],"%c - %c",'a'+n,'a'+m-1);
			if((chassis.codage == CHASSIS_CODE_a) || (chassis.codage == CHASSIS_CODE_A) || (chassis.codage == CHASSIS_CODE_1)) {
				n += CHASSIS_OF7; m += CHASSIS_OF7;
			}
			SambaPositionEncode(&chassis,1,&n,prem);
			if(m > n) {
				SambaPositionEncode(&chassis,1,&m,der);
				sprintf(OrgaGalettesNom[i],"%s - %s",prem,der);
			} else sprintf(OrgaGalettesNom[i],"%s",prem);
		} else break;
	}
	OrgaGalettesNom[i] = "\0";
}
#pragma mark ------ Librairie de tracage ------
/* ========================================================================== */
/*  Librairie de tracage                                                      */
/* ========================================================================== */
static OrgaRef OrgaNouvelleRef(OrgaRef ref, char type, void *adrs, int *k, int max) {
	OrgaRef courante;

	if(*k >= max) return(0);
	courante = ref + *k; *k += 1;
	courante->type = type; courante->adrs = adrs; courante->cdr = 0;
	return(courante);
}
/* ========================================================================== 
static void OrgaCoude(TypeFigLien *lien, int nbneg, int nbpos, int *ineg, int *ipos) {
	if(lien->dy < 0) { lien->coude = (lien->dx / 2) + ((*ineg - (nbneg / 2)) * ICONE_DECAL); *ineg += 1; }
	else if(lien->dy > 0) { lien->coude = (lien->dx / 2) + (((nbpos / 2) - *ipos) * ICONE_DECAL); *ipos += 1; }
	else lien->coude = 0;
}
   ========================================================================== */
static void OrgaLiaisonsInit(TypeOrgaLiaisons *liaisons, Cadre planche, int max) {
	TypeOrgaLien *lien; int n;

	liaisons->planche = planche;
	lien = (TypeOrgaLien *)malloc(max * sizeof(TypeOrgaLien));
	liaisons->lien = lien; if(lien) liaisons->max = max; else liaisons->max = 0;
	liaisons->nb = 0; liaisons->derniere = 0; liaisons->croise = 0;
	liaisons->mont_nb = liaisons->desc_nb = 0;
	if(lien) for(n=0; n<max; n++) {
		lien[n].x = 0; lien[n].y = 0; lien[n].trace.dx = 0; lien[n].trace.dy = 0;
		lien[n].trace.r = 0x3FFF; lien[n].trace.g = 0x1FFF; lien[n].trace.b = 0; lien[n].trace.relief = WND_LIGNES;
		lien[n].trace.horiz = 1; lien[n].trace.coude = 0;
	}
	return;
}
/* ========================================================================== */
static void OrgaLiaisonAjoute(TypeOrgaLiaisons *liaisons, int x0, int y0, int x1, int y1) {
	int n;

	if((n = liaisons->nb) >= liaisons->max) return;
	if(x1 == -1) x1 = x0; if(y1 == -1) y1 = y0;
	if((x0 == x1) && (y0 == y1)) return;
	liaisons->lien[n].x = x0 + 2; liaisons->lien[n].y = y0;
	liaisons->lien[n].trace.dx = x1 - 2 - liaisons->lien[n].x;
	liaisons->lien[n].trace.dy = y1 - liaisons->lien[n].y;
	if(liaisons->lien[n].trace.dy < 0) liaisons->mont_nb++; 
	else if(liaisons->lien[n].trace.dy > 0) liaisons->desc_nb++;
	liaisons->nb += 1;
}
/* ========================================================================== */
static void OrgaLiaisonCroisee(TypeOrgaLiaisons *liaisons, char croisee) {
	liaisons->croise = croisee;
}
/* ========================================================================== */
static void OrgaLiaisonTrace(TypeOrgaLiaisons *liaisons) {
	int n,mont_num,desc_num; Figure fig; TypeFigLien *trace;
	
	mont_num = desc_num = 0;
	for(n=liaisons->derniere; n<liaisons->nb; n++) {
		trace = &(liaisons->lien[n].trace);
		if((liaisons->lien[n].x + trace->dx) < OpiumServerWidth(0)) {
			if(liaisons->croise && trace->dy) {
				trace->coude = (trace->dx / 2) + ((mont_num - ((liaisons->mont_nb + liaisons->desc_nb) / 2)) * ICONE_DECAL); mont_num++;
			} else if(trace->dy < 0) { trace->coude = (trace->dx / 2) + ((mont_num - (liaisons->mont_nb / 2)) * ICONE_DECAL); mont_num++; }
			else if(trace->dy > 0) { trace->coude = (trace->dx / 2) + (((liaisons->desc_nb / 2) - desc_num) * ICONE_DECAL); desc_num++; }
			else trace->coude = 0;
			fig = FigureCreate(FIG_LIEN,(void *)trace,0,0);
			BoardAddFigure(liaisons->planche,fig,liaisons->lien[n].x,liaisons->lien[n].y,0);
		}
	}
	liaisons->derniere = liaisons->nb;
	liaisons->mont_nb = liaisons->desc_nb = 0;
}
/* ========================================================================== */
static int OrgaCliquePlanche(Cadre planche, WndAction *e, void *arg) {
	Figure fig; int x,y; Cadre cdr; OrgaRef ref;
	
	if((e->type != WND_PRESS) || (OrgaDerniereAction != WND_PRESS) || !OrgaDerniereFigure) return(0);
	fig = OrgaDerniereFigure;
	OrgaDerniereAction = e->type;
	OpiumDernierClic(&x,&y);
	if(e->type == WND_PRESS) {
		ref = (OrgaRef)(fig->adrs); if(!ref) return(0);
		if(ref->type >= ORGA_REF_POSITION) return(0);
	#ifdef DEPLACE_FANTOME
		fig->type = FIG_CADRE; fig->forme.zone->support = WND_RAINURES;
	#endif
		cdr = fig->cdr;
		cdr->x = (x - (cdr->larg / 2));
		cdr->y = (y - (cdr->haut / 2));
		cdr->f = 0;
		OpiumRefresh(planche);
	// } else if(e->type == WND_RELEASE) {
	// 	OrgaEspaceNumerMontre();
	}
	return(0);
}
#pragma mark ---- Selection de numeriseurs ----
/* ========================================================================== */
/*  Selection de numeriseurs                                                  */
/* ========================================================================== */
static void OrgaSelectionNumeriseurColorie(int bb) {
	TypeFigZone *zone;

	if(!OrgaNumerFig[bb]) return;
	zone = (Numeriseur[bb].sel)? &OrgaZoneSelOui: &OrgaZoneSelNon;
	FigureChangeForme(OrgaNumerFig[bb],FIG_ZONE,(void *)zone);
	OpiumRefresh(OrgaNumerFig[bb]->cdr);
}
/* ========================================================================== */
static int OrgaSelectionNumerClique(Figure cliquee, WndAction *e) {
	OrgaRef ref; TypeBNModele *modele_bn; TypeNumeriseur *numeriseur; int bb;
	TypeRepartiteur *repart; char out;
	
	ref = (OrgaRef)(cliquee->adrs);
	switch(ref->type) {
	  case ORGA_REF_AJOUTE: 
		modele_bn = (TypeBNModele *)(ref->adrs);
		ModlNumerChoisi = modele_bn->num;
		for(bb=0; bb<NumeriseurNb; bb++) {
			out = 1; if((repart = Numeriseur[bb].repart)) { if(repart->local) out = 0 ; } if(out) continue;
			if(OrgaSelectNumerUnique) Numeriseur[bb].sel = (Numeriseur[bb].def.type == modele_bn->num);
			else if(Numeriseur[bb].def.type == modele_bn->num) Numeriseur[bb].sel = 1;
			//- if(Numeriseur[bb].sel) OrgaSelectNumerNb++;
			OrgaSelectionNumeriseurColorie(bb);
		}
		break;
	  case ORGA_REF_RETIRE: 
		modele_bn = (TypeBNModele *)(ref->adrs);
		for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].def.type == modele_bn->num) {
			out = 1; if((repart = Numeriseur[bb].repart)) { if(repart->local) out = 0 ; } if(out) continue;
			Numeriseur[bb].sel = 0;
			OrgaSelectionNumeriseurColorie(bb);
		}
		ModlNumerChoisi = -1;
		break;
	  case ORGA_REF_TOUT:
		for(bb=0; bb<NumeriseurNb; bb++) {
			out = 1; if((repart = Numeriseur[bb].repart)) { if(repart->local) out = 0 ; } if(out) continue;
			if(!Numeriseur[bb].sel) {
				//- OrgaSelectNumerNb++;
				Numeriseur[bb].sel = 1;
				OrgaSelectionNumeriseurColorie(bb);
			}
		}
		break;
	  case ORGA_REF_RIEN:
		for(bb=0; bb<NumeriseurNb; bb++) {
			out = 1; if((repart = Numeriseur[bb].repart)) { if(repart->local) out = 0 ; } if(out) continue;
			if(Numeriseur[bb].sel) {
				//- --OrgaSelectNumerNb;
				Numeriseur[bb].sel = 0;
				OrgaSelectionNumeriseurColorie(bb);
			}
		}
		break;
	  case ORGA_REF_NUMER:
		numeriseur = (TypeNumeriseur *)(ref->adrs);
		out = 1; if((repart = numeriseur->repart)) { if(repart->local) out = 0 ; } if(out) break;
		modele_bn = &(ModeleBN[numeriseur->def.type]);
		numeriseur->sel = 1 - numeriseur->sel;
		if(numeriseur->sel) {
			if(ModlNumerChoisi < 0) ModlNumerChoisi = modele_bn->num;
			else if(OrgaSelectNumerUnique && (ModlNumerChoisi != modele_bn->num)) {
				OpiumFail(L_("Types de FPGA inhomogenes. Numeriseur %s abandonne"),numeriseur->nom);
				numeriseur->sel = 0;
				return(0);
			}
		} else {
			for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].sel) break;
			if(bb >= NumeriseurNb) ModlNumerChoisi = -1;
		}
		OrgaSelectionNumeriseurColorie(numeriseur->bb);
		break;
	}
	OrgaSelectNumerNb = 0; for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].sel) OrgaSelectNumerNb++;
	PanelRefreshVars(pOrgaSelectNumerNb);

	return(0);
}
/* ========================================================================== */
int OrgaSelectionNumeriseurConstruit(char exclusive) {
	int i,k,x,y; int bb,modl; int xslot,yslot,xnum,ynum,nb_ext,yext; int dx_quadrant;
	char out;
	char slot[8];
	short quadrant,distance; TypeCableConnecteur connecteur;
	TypeRepartiteur *repart;
	Figure fig; OrgaRef ref; // Menu m;
	
	if(bNumeriseurSelecte) {
		if(OpiumDisplayed(bNumeriseurSelecte)) OpiumClear(bNumeriseurSelecte);
		BoardTrash(bNumeriseurSelecte);
		if(LiaisonsCablage.lien) { free(LiaisonsCablage.lien); LiaisonsCablage.lien = 0; }
	} else bNumeriseurSelecte = BoardCreate();
	if(!bNumeriseurSelecte) return(0);
	OpiumTitle(bNumeriseurSelecte,L_("Selection Numeriseurs"));
	OrgaSelectNumerUnique = exclusive;
	if(OrgaSelectNumerUnique) {
		modl = -1;
		for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].sel) {
			if(modl < 0) modl = Numeriseur[bb].def.type;
			else if(Numeriseur[bb].def.type != modl) Numeriseur[bb].sel = 0;
		}
	}
	
	OrgaFinUsine = Dx + OrgaZoneModlDble.larg  + OrgaZonePlus.larg + ICONE_INTERVALLE;
	dx_quadrant = OrgaZoneSelOui.larg + 4 + OrgaZoneSlot.larg + ICONE_ETAGE;
	OrgaTraitSepareMag.dy = 0;
	k = 0;
	
	/* selection de modeles */
	x = Dx; y = WndLigToPix(2);
	for(i=0; i<ModeleBNNb; i++) {
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneModlDble),0,0); FigureTexte(fig,ModeleBN[i].nom);
		BoardAddFigure(bNumeriseurSelecte,fig,x,y,0);
		ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_AJOUTE,(void *)(&(ModeleBN[i])),&k,ORGA_MAXOBJ);
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZonePlus),(void *)ref,OrgaSelectionNumerClique); FigureTexte(fig,L_("Choisir"));
		BoardAddFigure(bNumeriseurSelecte,fig,x+OrgaZoneModlDble.larg+6,y+1,0);
		ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_RETIRE,(void *)(&(ModeleBN[i])),&k,ORGA_MAXOBJ);
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneMoins),(void *)ref,OrgaSelectionNumerClique); FigureTexte(fig,L_("Exclure"));
		BoardAddFigure(bNumeriseurSelecte,fig,x+OrgaZoneModlDble.larg+6,y+OrgaZonePlus.haut+7,0);
		y += (OrgaZoneModlDble.haut + ICONE_INTERVALLE);
	}
	if(!exclusive) {
		ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_TOUT,0,&k,ORGA_MAXOBJ);
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZonePlus),(void *)ref,OrgaSelectionNumerClique); FigureTexte(fig,L_("Tous"));
		BoardAddFigure(bNumeriseurSelecte,fig,x,y,0);
	}
	ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_RIEN,0,&k,ORGA_MAXOBJ);
	fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneMoins),(void *)ref,OrgaSelectionNumerClique); FigureTexte(fig,L_("Aucun"));
	BoardAddFigure(bNumeriseurSelecte,fig,x+OrgaZoneModlDble.larg+6,y,0);
	y += (OrgaZoneMoins.haut + Dy);
	if(OrgaTraitSepareMag.dy < y) OrgaTraitSepareMag.dy = y;
	
	/* positionnement sur le cryostat */
	x = OrgaFinUsine + ICONE_INTERVALLE; y = WndLigToPix(2);	
	OrgaChassisX0 = x; OrgaChassisY0 = y;
	xslot = x;
	for(quadrant=0; quadrant<QUADRANT_NB; quadrant++) {
		yslot = y;
		for(distance=0; distance<QUADRANT_LNGR; distance++) {
			connecteur = (quadrant * QUADRANT_OF7) + distance + 1;
			printf("(%s) Quadrant %d, distance %d soit connecteur=%d\n",__func__,quadrant,distance,connecteur);
			fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSlot),0,0);
			if(!ChassisNumerUnique) {
				strcpy(slot,CablageEncodeConnecteur(connecteur)); FigureTexte(fig,slot);
			}
			BoardAddFigure(bNumeriseurSelecte,fig,xslot + OrgaZoneSelOui.larg + 4,yslot,0);
			yslot += OrgaDyTour;
		}
		if(OrgaTraitSepareMag.dy < yslot) OrgaTraitSepareMag.dy = yslot;
		xslot += OrgaDxQuadrant;
	}
	
	x = OrgaFinUsine + ICONE_INTERVALLE; y = WndLigToPix(2);
	nb_ext = 0; yext = y + (QUADRANT_LNGR * OrgaDyTour);	
	/* selection par numeriseur particulier */
	for(bb=0; bb<NumeriseurNb; bb++) {
		if(Numeriseur[bb].fischer != CABLAGE_CONN_DIR) {
			int k;
			k = Numeriseur[bb].fischer - 1;
			quadrant = k / QUADRANT_OF7; distance = k - (quadrant * QUADRANT_OF7);
			xnum = x + (quadrant * OrgaDxQuadrant); ynum = y + (distance * OrgaDyTour);
		} else {
			xnum = x + (nb_ext * OrgaDxQuadrant); ynum = yext;
			fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSlot),0,0);
			if(!ChassisNumerUnique) {
				strcpy(slot,CablageEncodeConnecteur(CABLAGE_CONN_DIR)); FigureTexte(fig,slot);
			}
			BoardAddFigure(bNumeriseurSelecte,fig,xnum + OrgaZoneSelOui.larg + 4,ynum,0);
			if(++nb_ext >= QUADRANT_NB) { nb_ext = 0; yext += OrgaDyTour; }
		}
		ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_NUMER,(void *)(&(Numeriseur[bb])),&k,ORGA_MAXOBJ);
		out = 1; if((repart = Numeriseur[bb].repart)) { if(repart->local) out = 0 ; }
		if(out) fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSelOut),0,0);
		else if(Numeriseur[bb].sel) fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSelOui),(void *)ref,OrgaSelectionNumerClique);
		else fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSelNon),(void *)ref,OrgaSelectionNumerClique);
		FigureTexte(fig,Numeriseur[bb].nom);
		OrgaNumerFig[bb] = fig;
		BoardAddFigure(bNumeriseurSelecte,fig,xnum,ynum+1,0);
		OrgaLiaisonTrace(&LiaisonsCablage);
		if(OrgaTraitSepareMag.dy < ynum) OrgaTraitSepareMag.dy = ynum;
	}
	y = yext; // + OrgaDyTour;
	OrgaTraitSepareMag.dy += (2 * Dy);
	if(OrgaTraitSepareMag.dy < y) OrgaTraitSepareMag.dy = y;

	OrgaSelectNumerNb = 0; for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].sel) OrgaSelectNumerNb++;

	pOrgaSelectNumerNb = PanelCreate(1);
	PanelMode(pOrgaSelectNumerNb,PNL_DIRECT|PNL_RDONLY); PanelSupport(pOrgaSelectNumerNb,WND_CREUX);
	PanelItemLngr(pOrgaSelectNumerNb,PanelInt(pOrgaSelectNumerNb,"Selection",&OrgaSelectNumerNb),3);
	BoardAddPanel(bNumeriseurSelecte,pOrgaSelectNumerNb,Dx,OrgaTraitSepareMag.dy - (2 * Dy),0);

//	MenuColumns((m = MenuLocalise(iOrgaStopStart)),2); OpiumSupport(m->cdr,WND_PLAQUETTE);
//	BoardAddMenu(bNumeriseurSelecte,m,Dx,yext,0);

	fig = FigureCreate(FIG_DROITE,(void *)(&OrgaTraitSepareMag),0,0);
	BoardAddFigure(bNumeriseurSelecte,fig,OrgaFinUsine,Dy,0);
	
	BoardBoutonText(bNumeriseurSelecte,SambaSelectionText);

	OpiumExitFctn(bNumeriseurSelecte,OrgaFermeSelectionNumeriseurs,0);
	return(0);
}
#pragma mark ---- Selection de detecteurs ----
/* ========================================================================== */
/*  Selection de detecteurs                                                  */
/* ========================================================================== */
static void OrgaSelectionDetecteurColorie(int bolo) {
	TypeFigZone *zone;

	if(!OrgaDetecFig[bolo]) return;
	zone = (Bolo[bolo].sel)? &OrgaZoneSelOui: &OrgaZoneSelNon;
	FigureChangeForme(OrgaDetecFig[bolo],FIG_ZONE,(void *)zone);
	OpiumRefresh(OrgaDetecFig[bolo]->cdr);
}
/* ========================================================================== */
static int OrgaSelectionDetecClique(Figure cliquee, WndAction *e) {
	OrgaRef ref; TypeDetModele *modele_det; TypeDetecteur *detecteur; int bolo;
	
	ref = (OrgaRef)(cliquee->adrs);
	switch(ref->type) {
	  case ORGA_REF_AJOUTE: 
		modele_det = (TypeDetModele *)(ref->adrs);
		ModlDetecChoisi = modele_det->num;
		for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].local) {
			if(OrgaSelectDetecUnique) Bolo[bolo].sel = (Bolo[bolo].type == modele_det->num);
			else if(Bolo[bolo].type == modele_det->num) Bolo[bolo].sel = 1;
			OrgaSelectionDetecteurColorie(bolo);
		}
		break;
	  case ORGA_REF_RETIRE: 
		modele_det = (TypeDetModele *)(ref->adrs);
		for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].local && (Bolo[bolo].type == modele_det->num)) {
			Bolo[bolo].sel = 0;
			OrgaSelectionDetecteurColorie(bolo);
		}
		ModlDetecChoisi = -1;
		break;
	  case ORGA_REF_TOUT:
		for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].local && !Bolo[bolo].sel) {
			Bolo[bolo].sel = 1;
			OrgaSelectionDetecteurColorie(bolo);
		}
		break;
	  case ORGA_REF_RIEN:
		for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].local && Bolo[bolo].sel) {
			Bolo[bolo].sel = 0;
			OrgaSelectionDetecteurColorie(bolo);
		}
		break;
	  case ORGA_REF_DETEC:
		detecteur = (TypeDetecteur *)(ref->adrs);
		if(!(detecteur->local)) break;
		modele_det = &(ModeleDet[detecteur->type]);
		detecteur->sel = 1 - detecteur->sel;
		if(detecteur->sel) {
			if(ModlDetecChoisi < 0) ModlDetecChoisi = modele_det->num;
			else if(OrgaSelectDetecUnique && (ModlDetecChoisi != modele_det->num)) {
				OpiumFail(L_("Types de detecteurs inhomogenes. Detecteur %s abandonne"),detecteur->nom);
				detecteur->sel = 0;
				return(0);
			}
		} else {
			for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].sel) break;
			if(bolo >= BoloNb) ModlDetecChoisi = -1;
		}
		OrgaSelectionDetecteurColorie(detecteur->numero);
		break;
	}
	OrgaSelectDetecNb = 0; for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].sel) OrgaSelectDetecNb++;
	PanelRefreshVars(pOrgaSelectDetecNb);

	return(0);
}
/* ========================================================================== */
int OrgaSelectionDetecteurConstruit(char exclusive) {
	int i,k,x,y; int bolo,modl; int xslot,yslot,xdet,ydet,nb_ext,yext; int dx_quadrant;
	char slot[8];
	short galette,tour,branche;
	Figure fig; OrgaRef ref; // Menu m;
	
	if(bDetecteurSelecte) {
		if(OpiumDisplayed(bDetecteurSelecte)) OpiumClear(bDetecteurSelecte);
		BoardTrash(bDetecteurSelecte);
		if(LiaisonsCablage.lien) { free(LiaisonsCablage.lien); LiaisonsCablage.lien = 0; }
	} else bDetecteurSelecte = BoardCreate();
	if(!bDetecteurSelecte) return(0);
	OpiumTitle(bDetecteurSelecte,L_("Selection Detecteurs"));
	OrgaSelectDetecUnique = exclusive;
	if(OrgaSelectDetecUnique) {
		modl = -1;
		for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].sel) {
			if(modl < 0) modl = Bolo[bolo].type;
			else if(Bolo[bolo].type != modl) Bolo[bolo].sel = 0;
		}
	}
	
	OrgaFinUsine = Dx + OrgaZoneModlDble.larg  + OrgaZonePlus.larg + ICONE_INTERVALLE;
	dx_quadrant = OrgaZoneSelOui.larg + 4 + OrgaZoneSlot.larg + ICONE_ETAGE;
	OrgaTraitSepareMag.dy = 0;
	k = 0;
	
	/* selection de modeles */
	x = Dx; y = WndLigToPix(2);
	for(i=0; i<ModeleDetNb; i++) {
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneModlDble),0,0); FigureTexte(fig,ModeleDet[i].nom);
		BoardAddFigure(bDetecteurSelecte,fig,x,y,0);
		ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_AJOUTE,(void *)(&(ModeleDet[i])),&k,ORGA_MAXOBJ);
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZonePlus),(void *)ref,OrgaSelectionDetecClique); FigureTexte(fig,L_("Choisir"));
		BoardAddFigure(bDetecteurSelecte,fig,x+OrgaZoneModlDble.larg+6,y+1,0);
		ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_RETIRE,(void *)(&(ModeleDet[i])),&k,ORGA_MAXOBJ);
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneMoins),(void *)ref,OrgaSelectionDetecClique); FigureTexte(fig,L_("Exclure"));
		BoardAddFigure(bDetecteurSelecte,fig,x+OrgaZoneModlDble.larg+6,y+OrgaZonePlus.haut+7,0);
		y += (OrgaZoneModlDble.haut + ICONE_INTERVALLE);
	}
	if(!exclusive) {
		ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_TOUT,0,&k,ORGA_MAXOBJ);
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZonePlus),(void *)ref,OrgaSelectionDetecClique); FigureTexte(fig,L_("Tous"));
		BoardAddFigure(bDetecteurSelecte,fig,x,y,0);
	}
	ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_RIEN,0,&k,ORGA_MAXOBJ);
	fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneMoins),(void *)ref,OrgaSelectionDetecClique); FigureTexte(fig,L_("Aucun"));
	BoardAddFigure(bDetecteurSelecte,fig,x+OrgaZoneModlDble.larg+6,y,0);
	y += (OrgaZoneMoins.haut + Dy);
	if(OrgaTraitSepareMag.dy < y) OrgaTraitSepareMag.dy = y;
	
	/* positionnement dans le cryostat */
	x = OrgaFinUsine + ICONE_INTERVALLE; y = WndLigToPix(2);	
	OrgaChassisX0 = x; OrgaChassisY0 = y;
	xslot = x;
	for(galette=0; galette<GALETTES_NB; galette++) {
		yslot = y;
		for(tour=0; tour<TOURS_NB; tour++) {
			printf("(%s) Galette %d, tour %d soit position=%d\n",__func__,galette,tour,CABLAGE_POSITION(galette,tour));
			fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSlot),0,0);
			if(!ChassisDetecUnique) {
				strcpy(slot,CablageEncodeCoord(CABLAGE_POSITION(galette,tour))); FigureTexte(fig,slot);
			}
			BoardAddFigure(bDetecteurSelecte,fig,xslot + OrgaZoneSelOui.larg + 4,yslot,0);
			yslot += OrgaDyTour;
			if(OrgaTraitSepareMag.dy < yslot) OrgaTraitSepareMag.dy = yslot;
		}
		xslot += OrgaDxGalette;
	}
	
	x = OrgaFinUsine + ICONE_INTERVALLE; y = WndLigToPix(2);
	nb_ext = 0; yext = y + (TOURS_NB * OrgaDyTour);
	/* selection par detecteur particulier */
	for(bolo=0; bolo<BoloNb; bolo++) {
		if(Bolo[bolo].pos != CABLAGE_POS_NOTDEF) {
			CablageAnalysePosition(Bolo[bolo].pos,&galette,&tour,&branche);
			xdet = x + (galette * OrgaDxGalette); ydet = y + (tour * OrgaDyTour);
		} else {
			xdet = x + (nb_ext * OrgaDxGalette); ydet = yext;
			strcpy(slot,CablageEncodeCoord(CABLAGE_POS_NOTDEF));
			fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSlot),0,0);
			if(!ChassisDetecUnique) {
				strcpy(slot,CablageEncodeCoord(CABLAGE_POS_NOTDEF)); FigureTexte(fig,slot);
			}
			BoardAddFigure(bDetecteurSelecte,fig,xdet,ydet,0);
			if(++nb_ext >= GALETTES_NB) { nb_ext = 0; yext += OrgaDyTour; }
		}
		ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_DETEC,(void *)(&(Bolo[bolo])),&k,ORGA_MAXOBJ);
		if(!Bolo[bolo].local) fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSelOut),0,0);
		else if(Bolo[bolo].sel) fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSelOui),(void *)ref,OrgaSelectionDetecClique);
		else fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSelNon),(void *)ref,OrgaSelectionDetecClique);
		FigureTexte(fig,Bolo[bolo].nom);
		OrgaDetecFig[bolo] = fig;
		BoardAddFigure(bDetecteurSelecte,fig,xdet,ydet+1,0);
		OrgaLiaisonTrace(&LiaisonsCablage);
		if(OrgaTraitSepareMag.dy < ydet) OrgaTraitSepareMag.dy = ydet;
	}
	OrgaTraitSepareMag.dy += (2 * Dy);
	y = yext; // + OrgaDyTour;
	if(OrgaTraitSepareMag.dy < y) OrgaTraitSepareMag.dy = y;

	OrgaSelectDetecNb = 0; for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].sel) OrgaSelectDetecNb++;

	pOrgaSelectDetecNb = PanelCreate(1);
	PanelMode(pOrgaSelectDetecNb,PNL_DIRECT|PNL_RDONLY); PanelSupport(pOrgaSelectDetecNb,WND_CREUX);
	PanelItemLngr(pOrgaSelectDetecNb,PanelInt(pOrgaSelectDetecNb,"Selection",&OrgaSelectDetecNb),3);
	BoardAddPanel(bDetecteurSelecte,pOrgaSelectDetecNb,Dx,OrgaTraitSepareMag.dy - (2 * Dy),0);

//	MenuColumns((m = MenuLocalise(iOrgaStopStart)),2); OpiumSupport(m->cdr,WND_PLAQUETTE);
//	BoardAddMenu(bDetecteurSelecte,m,Dx,yext,0);

	fig = FigureCreate(FIG_DROITE,(void *)(&OrgaTraitSepareMag),0,0);
	BoardAddFigure(bDetecteurSelecte,fig,OrgaFinUsine,Dy,0);
	
	BoardBoutonText(bDetecteurSelecte,SambaSelectionText);

	OpiumExitFctn(bDetecteurSelecte,OrgaFermeSelectionDetecteurs,0);
	return(0);
}
#pragma mark ---- Librairie de construction ----
/* ========================================================================== */
/*  Librairie de construction                                                 */
#ifdef A_METTRE_AU_POINT
/* ========================================================================== */
static void OrgaRepartRetire(Figure fig, WndAction *e, void *adrs) {
	int rep,l,bb;
	TypeRepartiteur *repart; TypeNumeriseur *numeriseur;
	
	rep = (int)(IntAdrs)adrs;
	repart = &(Repart[rep]);
	for(l=0; l<repart->nbentrees; l++) if((numeriseur = (TypeNumeriseur *)(repart->entree[l].adrs))) {
		numeriseur->repart = 0;
		numeriseur->entree = 0;
		printf("* Numeriseur %s debranche\n",numeriseur->nom);
	}
	for(bb=0; bb<NumeriseurNb; bb++) {
		repart = Numeriseur[bb].repart;
		if(repart->rep > rep) Numeriseur[bb].repart = &(Repart[repart->rep-1]);
	}
	RepartNb--;
	for( ; rep<RepartNb; rep++) {
		memcpy(&(Repart[rep]),&(Repart[rep+1]),sizeof(TypeRepartiteur));
		Repart[rep].rep = rep;
	}
	printf("* Repartiteur %s poubellise\n",repart->nom);
	OrgaMagModifie[ORGA_MAG_REPART] = 1;
	//+ OrgaEspaceNumeriseur(0,0);
}
#endif
/* ========================================================================== */
static void OrgaRepartAjoute(Figure fig, WndAction *e, void *adrs, int x, int y) {
	int modl,rep,num; TypeRepartiteur *repart; Panel p; char rc;
	char adresse[80]; char message[256];

	/* if(x >= OrgaFinEtagere) tracasserie inutile? */ {
		if(RepartNb >= REPART_MAX) {
			OpiumError("Maximum de repartiteurs deja atteint (%d)",REPART_MAX);
			return;
		}
		modl = (int)(IntAdrs)adrs;
		adresse[0] = '\0';
		switch(ModeleRep[modl].interface) {
			case INTERF_IP:
				num = RepartNumLibre[ModeleRep[modl].famille];
				if(OpiumReadInt("Numero",&num) == PNL_CANCEL) return;
				switch(ModeleRep[modl].famille) {
					case FAMILLE_OPERA: sprintf(adresse,"192.168.0.%d",num); break;
					case FAMILLE_IPE:   sprintf(adresse,"192.168.0.%d",90+num); break;
					case FAMILLE_SAMBA: sprintf(adresse,"192.168.1.%d",20+num); break;
					case FAMILLE_CALI:  sprintf(adresse,"192.168.2.%d",num); break;
				}
				RepartNumLibre[ModeleRep[modl].famille] = num + 1;
				break;
			case INTERF_PCI:
				if(PCInb < 2) num = 1; else {
					num = PCInum; if(OpiumReadInt("Acces PCI",&num) == PNL_CANCEL) return;
				}
				sprintf(adresse,"%d",num);
				break;
		}
		RepartiteurInit(modl,&RepartLu,ModeleRep[modl].interface);
		p = PanelDesc(RepartOptnFamDesc[RepartLu.famille],1);
		rc = OpiumExec(p->cdr);
		PanelDelete(p);
		if(rc == PNL_CANCEL) return;
		rep = RepartNb++; repart = &(Repart[rep]);
//		RepartiteurInit(modl,repart,ModeleRep[modl].interface);
		memcpy(repart,&RepartLu,sizeof(TypeRepartiteur));
		strcpy(repart->parm,adresse);
		repart->rep = rep;
		RepartiteurVerifieParmsEtFIFO(repart,message);
		if(message[0]) printf("  ! %s. Repartiteur %s invalide.\n",message,repart->nom);
		if(repart->famille == FAMILLE_IPE) RepartIpEcriturePrete(repart,"  ");
		printf("* Repartiteur %s ajoute\n",repart->nom);
	} // tracasserie inutile?: else OpiumError("Glisser dans le chassis pour ajouter",x,OrgaFinEtagere);
	OrgaMagModifie[ORGA_MAG_REPART] = 1;
	OrgaMagasinRefait();
	if(OpiumDisplayed(bSchemaManip)) OrgaRafraichi(0);
}
/* ========================================================================== */
static int OrgaFibreSurRepartiteur(Figure fig, WndAction *e) {
	OrgaRepEntree ref; TypeNumeriseur *numeriseur; TypeRepartiteur *repart; int l;
	char texte[80]; char ancienne[FIBRE_NOM]; char code_entree[8];
	int rep,j,bb,ancien;
	
	ref = (OrgaRepEntree)(fig->adrs); if(!ref) return(0);
	repart = ref->repart; l = ref->l;
	strcpy(ancienne,repart->entree[l].fibre);
	RepartiteurCodeEntree(repart,l,code_entree);
	DicoFormatte(&OpiumDico,texte,"Fibre a brancher sur %s",code_entree);
	if(OpiumReadText(texte,repart->entree[l].fibre,FIBRE_NOM) == PNL_OK) {
		if(strcmp(ancienne,repart->entree[l].fibre)) {
			OrgaMagModifie[ORGA_MAG_REPART] = 1;
			if(repart->entree[l].fibre[0]) {
				for(rep=0; rep<RepartNb; rep++)  {
					for(j=0; j<Repart[rep].maxentrees; j++) if(((rep != repart->rep) || (j != l)) && !strcmp(Repart[rep].entree[j].fibre,repart->entree[l].fibre)) break;
					if(j < Repart[rep].maxentrees) break;
				}
				if(rep < RepartNb) {
					RepartiteurCodeEntree(&(Repart[rep]),j,code_entree);
					if(OpiumChoix(2,SambaNonOui,L_("Fibre %s actuellement branchee sur %s: la debrancher?"),Repart[rep].entree[j].fibre,code_entree)) {
						RepartiteurDebranche(&(Repart[rep]),j);
						Repart[rep].entree[j].fibre[0] = '\0';
					} else {
						OpiumWarn("Branchement de fibre inchange");
						strcpy(repart->entree[l].fibre,ancienne);
						return(0);
					}
				}
			}
			FigureTexte(fig,repart->entree[l].fibre);
			bb = NumeriseurAvecFibre(repart->entree[l].fibre);
			if((numeriseur = (TypeNumeriseur *)(repart->entree[l].adrs))) {
				if(strcmp(numeriseur->def.fibre,ancienne)) {
					OpiumFail("ERREUR SYSTEME! fibre sur %s: %s, sur %s: %s",numeriseur->nom,numeriseur->def.fibre,code_entree,ancienne);
					return(0);
				}
				if(numeriseur->def.fibre[0]) {
					RepartiteurDebranche(repart,l);
					if(bb >= 0) RepartiteurBranche(repart,l,&(Numeriseur[bb]));
				} else {
					if(bb >= 0) {
						if(OpiumChoix(2,SambaNonOui,L_("Fibre %s actuellement branchee sur %s: le debrancher?"),Numeriseur[bb].def.fibre,Numeriseur[bb].nom)) {
							RepartiteurDebranche(repart,l);
							Numeriseur[bb].def.fibre[0] = '\0';
						} else {
							OpiumError("Branchement de fibre inchange");
							strcpy(repart->entree[l].fibre,ancienne);
							return(0);
						}
					}
					strcpy(numeriseur->def.fibre,repart->entree[l].fibre);
				}
			} else {
				ancien = NumeriseurAvecFibre(ancienne);
				if(ancien >= 0) RepartiteurDebranche(repart,l);
				if(bb >= 0) RepartiteurBranche(repart,l,&(Numeriseur[bb]));
			}
			// definir detectr!! if(OpiumDisplayed(detectr->schema.planche)) OrgaDetecteurMontre(0,(MenuItem)detectr);
			OpiumRefresh(fig->cdr);
			OrgaMagasinRefait();
			if(OpiumDisplayed(bEspaceNumer)) OrgaEspaceNumerMontre();
			if(OpiumDisplayed(bSchemaManip) && (OngletCourant(OrgaFeuilles) == 1)) OrgaRafraichi(1);
		}
	}
	return(0);
}
/* ========================================================================== */
static int OrgaFibreSurNumeriseur(Figure fig, WndAction *e) {
	TypeNumeriseur *numeriseur; TypeRepartiteur *repart;
	char texte[80]; char ancienne[FIBRE_NOM]; char code_entree[8];
	int rep,l,j,bb,ancien;
	
	numeriseur = (TypeNumeriseur *)(fig->adrs);
	DicoFormatte(&OpiumDico,texte,"Fibre a brancher sur %s",numeriseur->nom);
	strcpy(ancienne,numeriseur->def.fibre);
	if(OpiumReadText(texte,numeriseur->def.fibre,FIBRE_NOM) == PNL_OK) {
		if(strcmp(ancienne,numeriseur->def.fibre)) {
			OrgaMagModifie[ORGA_MAG_NUMER] = 1;
			if(numeriseur->def.fibre[0]) {
				/* Cote numeriseurs */
				for(bb=0; bb<NumeriseurNb; bb++) if((bb != numeriseur->bb) && !strcmp(Numeriseur[bb].def.fibre,numeriseur->def.fibre)) break;
				if(bb < NumeriseurNb) {
					if(OpiumChoix(2,SambaNonOui,L_("Fibre %s actuellement branchee sur %s: la debrancher?"),Numeriseur[bb].def.fibre,Numeriseur[bb].nom)) {
						Numeriseur[bb].def.fibre[0] = '\0';
						/* Ce test verifie en fait aussi le branchement cote repartiteurs */
						if((repart = Numeriseur[bb].repart)) {
							l = Numeriseur[bb].entree;
							RepartiteurDebranche(repart,l);
						}
					} else {
						OpiumWarn("Branchement de fibre inchange");
						strcpy(numeriseur->def.fibre,ancienne);
						return(0);
					}
				}
				FigureTexte(fig,numeriseur->def.fibre);
				/* Cote Repartiteurs */
				if((repart = numeriseur->repart)) /* repartiteur courant */ {
					l = numeriseur->entree;
					if(strcmp(repart->entree[l].fibre,ancienne)) /* on devrait avoir l'ancienne fibre */ {
						RepartiteurCodeEntree(repart,l,code_entree);
						OpiumFail("ERREUR SYSTEME! fibre sur %s: %s, sur %s: %s",numeriseur->nom,numeriseur->def.fibre,code_entree,ancienne);
						return(0);
					}
					RepartiteurDebranche(repart,l); /* entree courante: la deconnecte */
				} else {
					ancien = RepartiteurAvecFibre(ancienne,&j);
					if(ancien >= 0) RepartiteurDebranche(&(Repart[ancien]),j);
				}
				rep = RepartiteurAvecFibre(numeriseur->def.fibre,&l); /* arrivee nouvelle fibre */
				if(rep >= 0) RepartiteurBranche(&(Repart[rep]),l,numeriseur);
			} else {
				if((repart = numeriseur->repart)) {
					l = numeriseur->entree;
					RepartiteurDebranche(repart,l);
				}
			}
			// definir detectr!! if(OpiumDisplayed(detectr->schema.planche)) OrgaDetecteurMontre(0,(MenuItem)detectr);
			OpiumRefresh(fig->cdr);
			OrgaMagasinRefait();
			if(OpiumDisplayed(bEspaceNumer)) OrgaEspaceNumerMontre();
			if(OpiumDisplayed(bSchemaManip) && (OngletCourant(OrgaFeuilles) == 1)) OrgaRafraichi(1);
		}
	}
	return(0);
}
/* ========================================================================== */
static void OrgaNumerChangeDetec(TypeNumeriseur *numeriseur, TypeCableConnecteur connecteur, char ajoute) {
	TypeDetecteur *detectr; int bolo,cap;
	int nb_erreurs = 0;

	// printf("(%s) Branchement de %s (%d) sur le connecteur %s\n",__func__,numeriseur->nom,numeriseur->bb,CablageEncodeConnecteur(numeriseur->fischer));
	for(bolo=0; bolo<BoloNb; bolo++) {
		detectr = &(Bolo[bolo]);
		for(cap=0; cap<detectr->nbcapt; cap++) if(connecteur && (CablageSortant[detectr->pos].captr[cap].connecteur == connecteur)) {
			printf("  . %s %s modifiee: le numeriseur %s vient d'etre ",ModeleVoie[detectr->captr[cap].modele].nom,detectr->nom,numeriseur->nom);
			if(ajoute) printf("branche sur le"); else printf("debranche du");
			printf(" connecteur %s\n",CablageEncodeConnecteur(connecteur));
			break;
		}
		if(cap < detectr->nbcapt) {
			printf("  . %s rebranche\n",detectr->nom);
			DetecteurDebrancheNumeriseurs(detectr);
			DetecteurBrancheNumeriseurs(detectr);
			DetecteurConnecteVoies(detectr,&nb_erreurs);
			DetecteurConnecteReglages(detectr,&nb_erreurs);
			printf("  . %d erreur%s generee%s\n",Accord2s(nb_erreurs));
			DetecteurEcrit(0,detectr,bolo,1,DETEC_REGLAGES);
			if(OpiumDisplayed(detectr->schema.planche)) OrgaDetecteurMontre(0,(MenuItem)detectr);
		}
	}
}
/* ========================================================================== */
static void OrgaNumerVerifieDetec(TypeNumeriseur *numeriseur, TypeCableConnecteur ancien) {
	OrgaNumerChangeDetec(numeriseur,ancien,0);
	OrgaNumerChangeDetec(numeriseur,numeriseur->fischer,1);
}
/* ========================================================================== */
static void OrgaNumerAjoute(Figure fig, WndAction *e, void *adrs) {
	TypeBNModele *modele_bn; int bb,num; short ident;
	char nom_numer[DETEC_NOM],texte[80];
	char racine[MAXFILENAME],nom_complet[MAXFILENAME];

	modele_bn = (TypeBNModele *)adrs;
	num = modele_bn->suivant;
	// sprintf(texte,"Numero d'un nouveau %s",modele_bn->nom);
	DicoFormatte(&OpiumDico,texte,"Numero d'un nouveau %s",modele_bn->nom);
	if(OpiumReadInt(texte,&num) == PNL_CANCEL) return;
	if(num >= modele_bn->suivant) modele_bn->suivant = num;
	sprintf(nom_numer,"%s_%d",modele_bn->nom,num);
	printf("* Creation du numeriseur %s\n",nom_numer);
	bb = NumeriseurNb;
	ident = num;
	NumeriseurNeuf(&(Numeriseur[bb]),modele_bn->num,ident,0);
	Numeriseur[bb].bb = bb;
	NumeriseurNb++;
	RepertoireIdentique(NumeriseurFichierLu,NumeriseurStock,racine);
	snprintf(nom_complet,MAXFILENAME,"%s%s",racine,Numeriseur[bb].fichier);
	NumeriseursEcrit(nom_complet,NUMER_RESSOURCES,bb);
	--NumeriseurNb;
	NumeriseurTermineAjout();
	OrgaMagModifie[ORGA_MAG_NUMER] = 1;
	if(OrgaMagMode == ORGA_SPC_NUMER) OrgaEspaceNumeriseur(0,0);
	else OrgaMagasinVisite(0,0);
}
/* ========================================================================== */
static void OrgaNumerRetire(Figure fig, WndAction *e, void *adrs) {
	int i; char *nom_clique;

	i = (int)(IntAdrs)adrs;
	nom_clique = OrgaNumerListe[i];
	OpiumFail("Suppression %s de %s indisponible",(OrgaNumerNum[i])? "directe": "definitive",nom_clique);
	OrgaMagModifie[ORGA_MAG_NUMER] = 1;
	//+ OrgaEspaceNumeriseur(0,0);
}
/* ========================================================================== */
static void OrgaNumerBranche(Figure fig, WndAction *e, void *adrs) {
	int i,bb; char texte[80];
	TypeNumeriseur *numeriseur; char fibre[FIBRE_NOM];

	if(!OpiumCheck(L_("Fonction non standard. Continuer?"))) return;
	i = (int)(IntAdrs)adrs; if((bb = (OrgaNumerNum[i] - 1)) < 0) return;
	numeriseur = &(Numeriseur[bb]);
	DicoFormatte(&OpiumDico,texte,"Fibre a brancher sur %s",OrgaNumerListe[i]);
	strcpy(fibre,numeriseur->def.fibre);
	if(OpiumReadText(texte,numeriseur->def.fibre,FIBRE_NOM) == PNL_OK) {
		if(strcmp(fibre,numeriseur->def.fibre)) {
			OrgaMagModifie[ORGA_MAG_NUMER] = 1;
			if(numeriseur->def.fibre[0]) {
				for(bb=0; bb<NumeriseurNb; bb++) if((bb != numeriseur->bb) && !strcmp(Numeriseur[bb].def.fibre,numeriseur->def.fibre)) break;
				if(bb < NumeriseurNb) {
					if(OpiumChoix(2,SambaNonOui,L_("Fibre actuellement branchee sur %s: la debrancher?"),Numeriseur[bb].nom)) {
						Numeriseur[bb].def.fibre[0] = '\0';
						//-- OrgaNumerVerifieDetec(&(Numeriseur[bb]));
					} else {
						OpiumWarn("Branchement de fibre inchange");
						strcpy(numeriseur->def.fibre,fibre);
						return;
					}
				}
			}
			//-- OrgaNumerVerifieDetec(numeriseur);
			if(OrgaMagMode == ORGA_SPC_NUMER) OrgaEspaceNumerMontre(); else OrgaMagasinRefait();
			if(OpiumDisplayed(bSchemaManip) && (OngletCourant(OrgaFeuilles) == 1)) OrgaRafraichi(1);
		}
	}
}
/* ========================================================================== */
static void OrgaNumerDeplace(Figure fig, WndAction *e, void *adrs, int x, int y) {
	int i,j,bb,bn; size_t l; int xc,yc,xinf; char *nom_clique,nom_fichier[MAXFILENAME];
	char racine[MAXFILENAME],nom_complet[MAXFILENAME]; FILE *fichier;
	char replace; short quadrant,distance;
	TypeCableConnecteur connecteur,ancien;
	Cadre cdr;

	i = (int)(IntAdrs)adrs;
	nom_clique = OrgaNumerListe[i]; bb = OrgaNumerNum[i] - 1;
	replace = 0; connecteur = 0;
	if(x < OrgaFinEtagere) {
		replace = (OrgaNumerNum[i]);
		printf("* Numeriseur %s %s sur l'etagere\n",nom_clique,replace? "repose": "garde");
		OrgaNumerNum[i] = 0;
	} else {
		if(OrgaMagMode == ORGA_SPC_NUMER) {
			quadrant = (x - OrgaChassisX0) / OrgaDxQuadrant;
			distance = (y - OrgaChassisY0) / OrgaDyTour;
			if((quadrant < 0) || ((distance < 0) || (distance >= QUADRANT_LNGR))) quadrant = QUADRANT_NB;
		} else {
			cdr = fig->cdr;
			xc = cdr->x + (fig->forme.zone->larg / 2);
			yc = cdr->y + (fig->forme.zone->haut / 2);
			for(quadrant=0; quadrant<QUADRANT_NB; quadrant++) {
				for(distance=0; distance<QUADRANT_LNGR; distance++) {
					xinf = OrgaPositionFischer[quadrant][distance].xinf - fig->forme.zone->larg;
					if((yc >= OrgaPositionFischer[quadrant][distance].yinf) && (yc <= OrgaPositionFischer[quadrant][distance].ysup)
					   && (xc >= xinf) && (xc <= OrgaPositionFischer[quadrant][distance].xsup)) break;
				}
				if(distance < QUADRANT_LNGR) break;
			}
		}
		if(quadrant < QUADRANT_NB) {
			connecteur = (quadrant * QUADRANT_OF7) + distance + 1;
			for(bn=0; bn<NumeriseurNb; bn++) if((bn != bb) && (Numeriseur[bn].fischer == connecteur)) break;
			if(bn < NumeriseurNb) {
				if(OpiumChoix(2,SambaNonOui,"Numeriseur %s actuellement branche sur %s: le debrancher?",Numeriseur[bn].nom,CablageEncodeConnecteur(connecteur))) {
					printf("* Numeriseur %s place sur l'etagere\n",Numeriseur[bn].nom);
					for(j=0; j<OrgaNumerNb; j++) if(OrgaNumerNum[j] == (bn + 1)) break;
					if(j < OrgaNumerNb) OrgaNumerNum[j] = 0;
					Numeriseur[bn].fischer = 0;
					OrgaNumerVerifieDetec(&(Numeriseur[bn]),connecteur);
					OrgaMagModifie[ORGA_MAG_NUMER] = 1;
				} else {
					OpiumError("Branchement de numeriseur annule");
					return;
				}
			}
		} else {
			replace = (OrgaNumerNum[i]);
			printf("* Numeriseur %s %s sur l'etagere\n",nom_clique,replace? "envoye": "remis");
			OrgaNumerNum[i] = 0;
		}
		if(bb >= 0) {
			replace = (Numeriseur[bb].fischer != connecteur);
			printf("* Numeriseur %s %s sur le connecteur %s\n",Numeriseur[bb].nom,
				replace? "deplace": "garde",CablageEncodeConnecteur(connecteur));
		} else if(connecteur) {
			replace = 1;
			printf("* Numeriseur %s place sur le connecteur %s\n",nom_clique,CablageEncodeConnecteur(connecteur));
			/* peut-etre dans la liste des numeriseurs lus (== precedemment mis sur etagere) */
			// for(bb=0; bb<NumeriseurNb; bb++) printf("  . %s %s\n",strcmp(Numeriseur[bb].nom,nom_clique)? "Ce n'est pas": "C'est",Numeriseur[bb].nom);
			for(bb=0; bb<NumeriseurNb; bb++) if(!strcmp(Numeriseur[bb].nom,nom_clique)) break;
			if(bb >= NumeriseurNb) {
				printf("  . Utilisation d'un nouveau numeriseur: %s, branche sur %s\n",nom_clique,CablageEncodeConnecteur(connecteur));
				bb = NumeriseurNb;
				RepertoireIdentique(NumeriseurFichierLu,NumeriseurStock,racine);
				strcpy(nom_fichier,nom_clique);
				l = strlen(nom_fichier); while(l--) if(nom_fichier[l] == '#') nom_fichier[l] = '_';
				snprintf(nom_complet,MAXFILENAME,"%s%s",racine,nom_fichier);
				if((fichier = fopen(nom_complet,"r"))) { NumeriseurPlace(fichier,nom_fichier,connecteur); fclose(fichier); }
				else {
					printf("  ! Impossible de lire le numeriseur sur '%s'\n  (%s)\n",nom_complet,strerror(errno));
					return;
				}
			} else printf("  . Numeriseur recycle: %s (%d/%d)\n",Numeriseur[bb].nom,bb+1,NumeriseurNb);
			OrgaNumerNum[i] = bb + 1;
		}
	}
	if(replace) {
		if(bb >= 0) {
			ancien = Numeriseur[bb].fischer;
			Numeriseur[bb].fischer = connecteur;
			OrgaNumerVerifieDetec(&(Numeriseur[bb]),ancien);
		}
		OrgaMagModifie[ORGA_MAG_NUMER] = 1;
		if(OrgaMagMode == ORGA_SPC_NUMER) OrgaEspaceNumerMontre(); else OrgaMagasinRefait();
		if(OpiumDisplayed(bSchemaManip)) OrgaRafraichi(0);
	}
}
/* ========================================================================== */
static TypeCablePosition OrgaCablageSlotVise(Figure fig) {
	Cadre cdr; short galette,tour,tourmin,tourmax; int xc,yc,xinf;
	TypeCablePosition pos_hexa;
	
	cdr = fig->cdr;
	xc = cdr->x + (fig->forme.zone->larg / 2);
	yc = cdr->y + (fig->forme.zone->haut / 2);
	tourmin = (ChassisDetec[1].codage == CHASSIS_CODE_1)? 1: 0;
	tourmax = tourmin + ChassisDetec[1].nb;
	for(galette=OrgaGalMin; galette<OrgaGalMax; galette++) {
		for(tour=tourmin; tour<tourmax; tour++) {
			xinf = OrgaPositionSlot[galette][tour].xinf - fig->forme.zone->larg;
			if((yc >= OrgaPositionSlot[galette][tour].yinf) && (yc <= OrgaPositionSlot[galette][tour].ysup)
			   && (xc >= xinf) && (xc <= OrgaPositionSlot[galette][tour].xsup)) break;
		}
		if(tour < tourmax) break;
	}
	if(galette >= OrgaGalMax) return(CABLAGE_POS_NOTDEF);
	pos_hexa = CABLAGE_POSITION(galette,tour);
	return(pos_hexa);
}
/* ========================================================================== */
static TypeCablePosition OrgaCablageBrancheSlot(TypeCablePosition pos_hexa, void *adrs) {
	TypeModeleCable *modele_cable;
	int modl,bolo; char *bouton[2];

	if(pos_hexa == CABLAGE_POS_NOTDEF) return(pos_hexa);
	if(CablageSortant[pos_hexa].type != -1) {
		bouton[0] = "Abandonner"; bouton[1] = "Remplacer";
		if(!OpiumChoix(2,bouton,"Emplacement %s deja occupe",CablageEncodePosition(pos_hexa))) return(CABLAGE_POS_NOTDEF);
	}
	modl = (int)(IntAdrs)adrs;
	modele_cable = &(ModeleCable[modl]);
	if((bolo = CablageSortant[pos_hexa].bolo) < 0) {
		for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].pos == pos_hexa) break;
		if(bolo >= BoloNb) bolo = -1;
	}
	sprintf(CablageSortant[pos_hexa].nom,"Cable#%02x",pos_hexa);
	CablageSortant[pos_hexa].defini = 1;
	CablageSortant[pos_hexa].type = modl;
	CablageSortant[pos_hexa].bolo = bolo;
	printf("(%s) Cablage[%02X].bolo = %d\n",__func__,pos_hexa,bolo);
	OrgaMagModifie[ORGA_MAG_CABLG] = 1;
	return(pos_hexa);
}
/* ========================================================================== */
static void OrgaCablageFischerAdefinir(TypeCablePosition pos_hexa) {
	int m,capteur; TypeModeleCable *modele_cable;

	if(pos_hexa == CABLAGE_POS_NOTDEF) return;
	modele_cable = &(ModeleCable[CablageSortant[pos_hexa].type]);
	CablageSortant[pos_hexa].nb_fischer = modele_cable->nb_numer;
	for(m=0; m<CablageSortant[pos_hexa].nb_fischer; m++) CablageSortant[pos_hexa].fischer[m] = 0;
	CablageSortant[pos_hexa].nbcapt = modele_cable->nb_capt;
	for(capteur=0; capteur<CablageSortant[pos_hexa].nbcapt; capteur++) {
		CablageSortant[pos_hexa].captr[capteur].gain = CABLAGE_DEFAUT_GAIN;
		CablageSortant[pos_hexa].captr[capteur].capa = CABLAGE_DEFAUT_CAPA; // 8.2;
		CablageSortant[pos_hexa].captr[capteur].connecteur = 0;
		CablageSortant[pos_hexa].captr[capteur].vbb = modele_cable->capteur[capteur].numer_adc - 1;
		// printf("(%s) %02x.%d -> %d.%d\n",__func__,pos_hexa,capteur,CablageSortant[pos_hexa].captr[capteur].connecteur,CablageSortant[pos_hexa].captr[capteur].vbb);
	}
	OrgaMagModifie[ORGA_MAG_CABLG] = 1;
}
/* ========================================================================== */
static void OrgaCablageFischerCopie(TypeCablePosition pos_nouv, TypeCablePosition pos_hexa) {
	int m,capteur;
	
	if((pos_nouv == CABLAGE_POS_NOTDEF) || (pos_hexa == CABLAGE_POS_NOTDEF)) return;
	CablageSortant[pos_nouv].nb_fischer = CablageSortant[pos_hexa].nb_fischer;
	for(m=0; m<CablageSortant[pos_nouv].nb_fischer; m++) CablageSortant[pos_nouv].fischer[m] = CablageSortant[pos_hexa].fischer[m];
	CablageSortant[pos_nouv].nbcapt = CablageSortant[pos_hexa].nbcapt;
	for(capteur=0; capteur<CablageSortant[pos_nouv].nbcapt; capteur++) 
		memcpy(&(CablageSortant[pos_nouv].captr[capteur]),&(CablageSortant[pos_hexa].captr[capteur]),sizeof(TypeCablageCaptr));
	OrgaMagModifie[ORGA_MAG_CABLG] = 1;
}
/* ========================================================================== */
static void OrgaCablageAjoute(Figure fig, WndAction *e, void *adrs) {
	OrgaCablageFischerAdefinir(OrgaCablageBrancheSlot(OrgaCablageSlotVise(fig),adrs));
	OrgaMagasinRefait();
}
/* ========================================================================== */
static void OrgaCablageRetire(Figure fig, WndAction *e, void *adrs) {
	int bolo; TypeCablePosition pos_hexa;

	pos_hexa = (TypeCablePosition)(IntAdrs)adrs;
	if(pos_hexa == CABLAGE_POS_NOTDEF) return;
	bolo = CablageSortant[pos_hexa].bolo;
	CablageNeuf(pos_hexa);
	OrgaMagModifie[ORGA_MAG_CABLG] = 1;
	if(bolo >= 0) {
		printf("  . %s debranche\n",Bolo[bolo].nom);
		DetecteurDebrancheNumeriseurs(&(Bolo[bolo]));
		if(OpiumDisplayed(Bolo[bolo].schema.planche)) OrgaDetecteurMontre(0,(MenuItem)(&(Bolo[bolo])));
	}
}
/* ========================================================================== */
static void OrgaConnectSoude(Figure fig, WndAction *e, void *adrs) {
	int code,cap,modl,rang_bb,m; char fischer[8];
	TypeCablePosition pos_hexa,pos_autre; TypeCableConnecteur connecteur;

	code = (int)(IntAdrs)adrs;
	rang_bb = code & 0xFFFF;
	pos_hexa = (TypeCablePosition)((code >> 16) & 0xFFFF);
	modl = CablageSortant[pos_hexa].type;
	strcpy(fischer,CablageEncodeConnecteur(CablageSortant[pos_hexa].fischer[rang_bb]));
	if(OpiumReadText("Soudure sur le connecteur",fischer,8) == PNL_CANCEL) return;
	connecteur = CablageDecodeConnecteur(fischer);
	for(pos_autre=0; pos_autre<CABLAGE_POS_MAX; pos_autre++) {
		for(m=0; m<CablageSortant[pos_autre].nb_fischer; m++) {
			if( CablageSortant[pos_autre].fischer[m] == connecteur) {
				OpiumError("Le connecteur %s est deja utilise pour la position %s",fischer,CablageEncodePosition(pos_autre));
				return;
			}
		}
	}
	CablageSortant[pos_hexa].fischer[rang_bb] = connecteur;
	for(cap=0; cap<ModeleCable[modl].nb_capt; cap++) if(ModeleCable[modl].capteur[cap].numer_index == rang_bb) {
		CablageSortant[pos_hexa].captr[cap].connecteur = connecteur;
		CablageSortant[pos_hexa].captr[cap].vbb = ModeleCable[modl].capteur[cap].numer_adc - 1;
		// printf("(%s) %02x.%d -> %d.%d\n",__func__,pos_hexa,cap,CablageSortant[pos_hexa].captr[cap].connecteur,CablageSortant[pos_hexa].captr[cap].vbb);
	}
	if(connecteur) strcpy(fischer,CablageEncodeConnecteur(connecteur)); else strcpy(fischer,"ext");
	if(OrgaStyle == ORGA_ICONES) FigureLegende(fig,FIG_A_DROITE,fischer); else FigureTexte(fig,fischer);
	OrgaMagModifie[ORGA_MAG_CABLG] = 1;
//	OpiumRefresh(fig->cdr);
	OpiumRefreshFigure(fig->cdr);
}
/* ========================================================================== */
static void OrgaCablageDeplace(Figure fig, WndAction *e, void *adrs, int x, int y) {
	int bolo; TypeCablePosition pos_hexa,pos_nouv; int nb_erreurs;
	
	if(x < OrgaFinEtagere) OrgaCablageRetire(fig,e,adrs);
	else {
		pos_nouv = OrgaCablageSlotVise(fig);
		if(pos_nouv == CABLAGE_POS_NOTDEF) return;
		pos_hexa = (TypeCablePosition)(IntAdrs)adrs;
		if(pos_nouv != pos_hexa) {
			OrgaCablageBrancheSlot(pos_nouv,(void *)(IntAdrs)(CablageSortant[pos_hexa].type));
			OrgaCablageFischerCopie(pos_nouv,pos_hexa);
			CablageNeuf(pos_hexa);
			if((bolo = CablageSortant[pos_hexa].bolo) >= 0) {
				printf("  . %s debranche\n",Bolo[bolo].nom);
				DetecteurDebrancheNumeriseurs(&(Bolo[bolo]));
				if(OpiumDisplayed(Bolo[bolo].schema.planche)) OrgaDetecteurMontre(0,(MenuItem)(&(Bolo[bolo])));
			}
			if((bolo = CablageSortant[pos_nouv].bolo) >= 0) {
				DetecteurBrancheNumeriseurs(&(Bolo[bolo]));
				DetecteurConnecteVoies(&(Bolo[bolo]),&nb_erreurs);
				DetecteurConnecteReglages(&(Bolo[bolo]),&nb_erreurs);
				DetecteurEcrit(0,&(Bolo[bolo]),bolo,1,DETEC_REGLAGES);
			}
			OrgaMagModifie[ORGA_MAG_CABLG] = 1;
			OrgaMagasinRefait();
		}
	}
}
/* ========================================================================== */
static int OrgaDetecAjoute(Figure fig, WndAction *e, void *adrs) {
	TypeDetModele *modele_det; int bolo; char nom_detec[DETEC_NOM];
	char texte[80];

	modele_det = (TypeDetModele *)adrs;
	sprintf(nom_detec,"%s%d",modele_det->nom,modele_det->suivant);
	DicoFormatte(&OpiumDico,texte,"Nom d'un nouveau %s",modele_det->nom);
	if(OpiumReadText(texte,nom_detec,DETEC_NOM) == PNL_CANCEL) return(0);
	DetecteurNouveau(modele_det->num,nom_detec);
	bolo = BoloNb - 1;
	BoloStandard = BoloNb;
	DetecteurEcrit(0,&(Bolo[bolo]),bolo,1,DETEC_REGLAGES);
	--BoloNb;
	DetecteurAjouteStd();

	OrgaMagModifie[ORGA_MAG_DETEC] = 1;
	if(OrgaMagMode == ORGA_SPC_DETEC) OrgaEspaceDetecteur(0,0);
	else OrgaMagasinVisite(0,0);
	
	return(0);
}
/* ========================================================================== */
static void OrgaDetecDeplace(Figure fig, WndAction *e, void *adrs, int x, int y) {
	char *nom_clique; int i,bolo; int xc,yc,xsup;
	char racine[MAXFILENAME],nom_complet[MAXFILENAME]; FILE *fichier;
	char lien[80];
	char replace; short galette,tour,tourmin,tourmax; int nb_erreurs;
	TypeDetecteur *detectr; TypeCablePosition pos_hexa;
	
	pos_hexa = 0;
	i = (int)(IntAdrs)adrs;
	nom_clique = OrgaDetecListe[i];
	/* if(x < OrgaFinUsine) { printf("* Detecteur %s supprime\n",nom_clique); } else */ 
	if(x < OrgaFinEtagere) {
		printf("* Detecteur %s %s sur l'etagere\n",nom_clique,(OrgaDetecNum[i])? "remis": "garde");
		if((bolo = OrgaDetecNum[i] - 1) >= 0) {
			detectr = Bolo + bolo;
			DetecteurDebrancheNumeriseurs(detectr);
			DetecteurRetire(detectr);
		}
		OrgaDetecNum[i] = 0;
	} else {
		replace = 0;
		if(OrgaMagMode == ORGA_SPC_DETEC) {
			galette = (x - OrgaChassisX0) / OrgaDxGalette;
			tour = (y - OrgaChassisY0) / OrgaDyTour;
			if((galette >= 0) && (galette < GALETTES_NB) && (tour >= 0) && (tour < TOURS_NB)) {
				pos_hexa = CABLAGE_POSITION(galette,tour);
			}
		} else {
			Cadre cdr;
			cdr = fig->cdr;
			xc = cdr->x + (fig->forme.zone->larg / 2);
			yc = cdr->y + (fig->forme.zone->haut / 2);
			tourmin = (ChassisDetec[1].codage == CHASSIS_CODE_1)? 1: 0;
			tourmax = tourmin + ChassisDetec[1].nb;
			for(galette=OrgaGalMin; galette<OrgaGalMax; galette++) {
				for(tour=tourmin; tour<tourmax; tour++) {
					xsup = OrgaPositionSlot[galette][tour].xsup + fig->forme.zone->larg;
					if((yc >= OrgaPositionSlot[galette][tour].yinf) && (yc <= OrgaPositionSlot[galette][tour].ysup)
					   && (xc >= OrgaPositionSlot[galette][tour].xinf) && (xc <= xsup)) break;
				}
				if(tour < tourmax) break;
			}
			if(galette >= OrgaGalMax) { OrgaMagasinRefait(); return; }
			pos_hexa = CABLAGE_POSITION(galette,tour);
		}
		if(((bolo = OrgaDetecNum[i] - 1) >= 0) && (Bolo[bolo].pos != pos_hexa)) 
			printf("* Detecteur %s deplace sur la position %s\n",nom_clique,CablageEncodePosition(pos_hexa));
		else printf("* Detecteur %s %s %s sur la position %s\n",(bolo >= 0)?Bolo[bolo].nom:nom_clique,(bolo >= 0)?"":"(?)",(OrgaDetecNum[i])? "garde": "place",CablageEncodePosition(pos_hexa));
		nb_erreurs = 0;
		if(OrgaDetecNum[i] == 0) {
			for(bolo=0; bolo<BoloNb; bolo++) if(!strcmp(Bolo[bolo].nom,OrgaDetecListe[i])) break;
			if(bolo >= BoloNb) {
				bolo = BoloNb;
				RepertoireIdentique(FichierPrefDetecteurs,DetecteurStock,racine);
				snprintf(nom_complet,MAXFILENAME,"%s%s",racine,nom_clique);
				if((fichier = fopen(nom_complet,"r"))) {
					sprintf(lien,"*@x");
					DetecteursLitFichier(fichier,nom_clique,lien,pos_hexa,0,0); // hote = "local"?
					DetecteurTermine(&(Bolo[bolo]),0);
					DetecteurAjouteStd();
				} else {
					OpiumError("Impossible de lire le detecteur sur '%s'\n  (%s)",nom_complet,strerror(errno));
					return;
				}
			}
			OrgaDetecNum[i] = bolo + 1;
		}
		// printf("* Ce detecteur etait en position %02X et passe en position %02X\n",Bolo[bolo].pos,pos_hexa);
		if(Bolo[bolo].pos != pos_hexa) {
			detectr = &(Bolo[bolo]); DetecteurPlace(detectr,pos_hexa);
			DetecteurDebrancheNumeriseurs(detectr);
			DetecteurBrancheNumeriseurs(detectr);
			DetecteurConnecteVoies(detectr,&nb_erreurs);
			DetecteurConnecteReglages(detectr,&nb_erreurs);
			// printf("* Reglages maintenant connectes, %s a %s dans '%s'\n",detectr->nom,detectr->a_sauver?"sauver":"ne PAS sauver",detectr->fichier);
			DetecteurEcrit(0,detectr,bolo,1,DETEC_REGLAGES);
			// DetecteurEcritTous(DETEC_LISTE,-1); seulement si "Enregistrer"
			if(OpiumDisplayed(detectr->schema.planche)) OrgaDetecteurMontre(0,(MenuItem)detectr);
		}
	}
	OrgaMagModifie[ORGA_MAG_DETEC] = 1;
	if(OrgaMagMode == ORGA_SPC_DETEC) OrgaEspaceDetecMontre(); else OrgaMagasinRefait();
	if(OpiumDisplayed(bSchemaManip)) OrgaRafraichi(0);
}
/* ========================================================================== */
static void OrgaDetecRetire(Figure fig, WndAction *e, void *adrs) {
	int i; char *nom_clique;
	
	i = (int)(IntAdrs)adrs;
	nom_clique = OrgaDetecListe[i];
	OpiumError("Suppression %s de %s indisponible",(OrgaDetecNum[i])? "directe": "definitive",nom_clique);
	OrgaMagModifie[ORGA_MAG_DETEC] = 1;
}
#pragma mark ------ Construction generale -----
/* ========================================================================== */
/*  Construction generale                                                     */
/* ========================================================================== */
static int OrgaAtelierClique(Figure fig, WndAction *e) {
	OrgaMagMode = (char)(IntAdrs)(fig->adrs);
	OrgaMagasinOnglet(OrgaCablageMontre);
	return(0);
}
/* ========================================================================== */
static int OrgaGaletteClique(Figure fig, WndAction *e) {
	OrgaMagasinOnglet((int)(IntAdrs)(fig->adrs));
	return(0);
}
/* ========================================================================== */
static int OrgaMagasinClique(Figure cliquee, WndAction *e) {
	Figure fig; int x,y; Cadre cdr; OrgaRef ref;
	TypeCablePosition pos_hexa;
	char poubellise;
	
	if(OrgaDerniereAction == WND_PRESS) fig = OrgaDerniereFigure;
	else OrgaDerniereFigure = fig = cliquee;
	OrgaDerniereAction = e->type;
	OpiumDernierClic(&x,&y);
	if(e->type == WND_PRESS) {
		// printf("(%s) bouton presse, ref=%x\n",__func__,(hexa)(fig->adrs));
		ref = (OrgaRef)(fig->adrs); if(!ref) return(0);
		// printf("(%s) type ref=%d\n",__func__,ref->type);
		if(ref->type >= ORGA_REF_POSITION) return(0);
	#ifdef DEPLACE_FANTOME
		if(!OrgaFigurePressee) {
			OrgaFigurePressee = fig;
			fig = FigureCreate(FIG_CADRE,(void *)(&OrgaCadreVide),(void *)OrgaFigurePressee->adrs,OrgaMagasinClique);
			FigureTexte(fig,OrgaFigurePressee->texte);
			if(OrgaFigurePressee->type == FIG_ICONE) {
				fig->forme.zone->larg = OrgaFigurePressee->forme.icone->larg;
				fig->forme.zone->haut = OrgaFigurePressee->forme.icone->haut;
			}
		}
	#endif
		cdr = fig->cdr;
		cdr->x = (x - (cdr->larg / 2));
		cdr->y = (y - (cdr->haut / 2));
		cdr->f = 0;
	#ifdef DEPLACE_FANTOME
		BoardReplace(bOrgaCryostat,OrgaFigurePressee->cdr,fig->cdr);
	#endif
		OpiumRefresh(bOrgaCryostat);
	} else if(e->type == WND_RELEASE) {
		OrgaDerniereFigure = 0;
	#ifdef DEPLACE_FANTOME
		if(OrgaFigurePressee) {
			BoardReplace(bOrgaCryostat,fig->cdr,OrgaFigurePressee->cdr);
			OrgaFigurePressee = 0;
		}
		//- fig->type = FIG_ZONE; if(fig->forme.zone->larg > 50) fig->forme.zone->support = WND_PLAQUETTE;
	#endif
		ref = (OrgaRef)(fig->adrs); if(!ref) return(0);
		// printf("(%s) clic sur %s avec la reference %d\n",__func__,fig->texte,ref->type);
		poubellise = 0;
		if(OrgaPoubelle) {
			cdr = OrgaPoubelle->cdr;
			if(cdr) poubellise = ((x > cdr->x) && (x < (cdr->x + cdr->larg)) && (y > cdr->y) && (y < (cdr->y + cdr->haut)));
		}
		if(poubellise) {
		#ifdef A_METTRE_AU_POINT
			if(OpiumCheck("Suppression d'un %s: confirmer!",OrgaRefListe[ref->type])) switch(ref->type) {
				case ORGA_REF_REPART: OrgaRepartRetire(fig,e,ref->adrs);   break;
				case ORGA_REF_NUMER:  OrgaNumerRetire(fig,e,ref->adrs);   break;
				case ORGA_REF_CABLE:  OrgaCablageRetire(fig,e,ref->adrs); break;
				case ORGA_REF_DETEC:  OrgaDetecRetire(fig,e,ref->adrs);   break;
			}
		#else
			OpiumError("Poubelle desactivee");
		#endif
			OrgaMagasinRefait();
		} else if(cliquee == OrgaPoubelle) {
			/* Clic direct: vidage de poubelle? */
			OpiumError("Pas de %s a jeter",OrgaRefListe[ref->type]);
		} else switch(ref->type) {
			case ORGA_REF_MODELE:  switch(OrgaMagMode) {
				case ORGA_REF_REPART: OrgaRepartAjoute(fig,e,ref->adrs,x,y);   break;
				case ORGA_MAG_NUMER:  OrgaNumerAjoute(fig,e,ref->adrs);   break;
				case ORGA_MAG_CABLG:  OrgaCablageAjoute(fig,e,ref->adrs); break;
				case ORGA_MAG_DETEC:  OrgaDetecAjoute(fig,e,ref->adrs);   break;
			}; break;
			case ORGA_REF_NUMER:   OrgaNumerDeplace(fig,e,ref->adrs,x,y);   break;
			case ORGA_REF_CABLE:
				if(e->code == WND_MSELEFT) OrgaCablageDeplace(fig,e,ref->adrs,x,y); 
				else if(e->code == WND_MSERIGHT) {
					pos_hexa = (TypeCablePosition)(IntAdrs)(ref->adrs);
					CablageDeplie[pos_hexa] = 1 - CablageDeplie[pos_hexa];
					OrgaMagasinRefait();
				}
				break;
			case ORGA_REF_DETEC:   OrgaDetecDeplace(fig,e,ref->adrs,x,y);   break;
			case ORGA_REF_FISCHER: OrgaConnectSoude(fig,e,ref->adrs);       break;
			case ORGA_REF_FIBRE:   OrgaNumerBranche(fig,e,ref->adrs);       break;
			case ORGA_REF_POSITION: 
				if(e->code == WND_MSERIGHT) {
					pos_hexa = (TypeCablePosition)(IntAdrs)(ref->adrs);
					CablageDeplie[pos_hexa] = 1 - CablageDeplie[pos_hexa];
					OrgaMagasinRefait();
				}
				break;
		}
	}
	return(0);
}
/* ========================================================================== */
static void OrgaMagasinSauve(char mode) {
	switch(mode) {
		case ORGA_MAG_REPART: RepartiteursSauve(); SambaSauveReseau(); break;
		case ORGA_MAG_NUMER:  NumeriseursEcrit(NumeriseurFichierLu,NUMER_CONNECTION,NUMER_TOUS); break;
		case ORGA_MAG_CABLG:  CablageEcrit(CablageFichierLu); break;
		case ORGA_MAG_DETEC:  DetecteurEcritTous(DETEC_LISTE,-1); break;
	};
	OrgaMagModifie[mode] = 0;
	OrgaMagModifie[0] = 0; /* sera re-evalue au moment venu, s'il faut */
}
/* ========================================================================== */
int OrgaMagasinEnregistre(Menu menu, MenuItem item) {
	if(OrgaMagMode) OrgaMagasinSauve(OrgaMagMode);
	else {
		char mode; Panel p;
		p = PanelCreate(ORGA_MAG_MAX);
		for(mode=ORGA_MAG_REPART; mode <= ORGA_MAG_MAX; mode++) PanelBool(p,OrgaMagListe[mode],&(OrgaMagModifie[mode]));
		if(OpiumExec(p->cdr) == PNL_OK) {
			for(mode=ORGA_MAG_REPART; mode <= ORGA_MAG_MAX; mode++) if(OrgaMagModifie[mode]) OrgaMagasinSauve(mode);
		}
		PanelDelete(p);
	}
	return(0);
}
/* ========================================================================== */
static int OrgaTraceCablage(Cadre planche, int x0, int y0, short galette, short tour, TypeOrgaLiaisons *liens, int *k) {
	int bolo,cap,voie,bb,prec,svt,fin,code,rang_bb,rang_prec;
	TypeCablePosition pos_hexa;
	TypeCablage *cablage; OrgaRef ref;
	TypeModeleCable *modele_cable; TypeDetModele *modele_det; TypeDetecteur *detectr;
	TypeCableConnecteur connecteur; short quadrant,distance;
	Figure fig;
	int x,xrep,xfib,xslt,xbb,xadc,xcap,xdet; int ybb,yvbb,ydet,ycap; int decal,decal_dr,decal_ht,lconn; int i;
	int yadc[DETEC_CAPT_MAX];
	char titre[32],slot[8]; char *nom_voie; char log;
	
	log = 0;
	pos_hexa = CABLAGE_POSITION(galette,tour);
	if(log > 1) printf("(%s) Trace du cablage de %02X en (%d, %d)\n",__func__,pos_hexa,x0,y0);
	xrep = x0;
	xfib = xrep /* + OrgaZoneRepartOK.larg + ICONE_ETAGE */ ;
	xbb = xfib + OrgaLngrFibre + 4;
	xslt = xbb + OrgaZoneRepartOK.larg + 4;
	xadc = xslt + OrgaZoneSlot.larg + ICONE_ETAGE; if(OrgaStyle == ORGA_ICONES) xadc += (3 * Dx);
	xcap = xadc + OrgaZoneAdc.larg + ICONE_ETAGE;
#ifdef AVEC_ICONE
	xdet = xadc + OrgaFigCable[OrgaStyle].larg + ((OrgaStyle == ORGA_ZONES)? ICONE_ETAGE: 0);
#else
	xdet = xcap + OrgaZoneCapt.larg + ICONE_ETAGE;
#endif
	ybb = 0;
	detectr = 0;

	OrgaLiaisonsInit(liens,planche,1024);
	cablage = &(CablageSortant[pos_hexa]);
	if(log) printf("(%s) Cablage[%s] type %s en (%d, %d) sur bolo#%d\n",__func__,CablageEncodePosition(pos_hexa),
		(cablage->type >= 0)?ModeleCable[cablage->type].nom:"a definir",x0,y0,cablage->bolo);
	if(cablage->type >= 0) {
		modele_cable = &(ModeleCable[cablage->type]);
		modele_det = &(ModeleDet[modele_cable->modl_bolo]);
		bolo = cablage->bolo; if(bolo >= BoloNb) bolo = -1; detectr = (bolo >= 0)? &(Bolo[bolo]): 0;
		if(CablageDeplie[pos_hexa]) ydet = y0 - Dx + ((modele_det->nbcapt * (OrgaZoneVoie.haut + ICONE_MARGE)) / 2);
#ifdef AVEC_ICONE
		else ydet = y0 - Dx + ((cablage->nb_fischer * (OrgaFigCable[OrgaStyle].haut + ICONE_MARGE)) / 2);
#else
		else ydet = y0 - Dx + ((cablage->nb_fischer * (OrgaZoneNumer.haut + ICONE_MARGE)) / 2);
#endif
		if(log) printf("(%s) %d capteur%s, detecteur %s\n",__func__,Accord1s(modele_det->nbcapt),detectr? detectr->nom: "a trouver");
		yvbb = y0;
		rang_prec = -1;
		for(cap=0; cap<modele_det->nbcapt; cap++) yadc[cap] = 0;
		for(cap=0; cap<modele_det->nbcapt; cap++) {
			connecteur = cablage->captr[cap].connecteur;
			if(log) printf("(%s) connecteur=%d\n",__func__,connecteur);
//			if(connecteur) {
				if(!ChassisNumerUnique) strcpy(slot,CablageEncodeConnecteur(connecteur)); else slot[0] = '\0';
//			} else strcpy(slot,"ext");
			lconn = (int)strlen(slot);
			rang_bb = modele_cable->capteur[cap].numer_index;
			for(prec=0; prec<cap; prec++) if(modele_cable->capteur[prec].numer_index == rang_bb) break;
			if(prec < cap) continue;
			if(rang_bb != rang_prec) {
				ybb = yvbb;
				OrgaLiaisonTrace(liens);
#ifdef AVEC_ICONE_CONNEC
				if(OrgaMagMode) {
					code = (pos_hexa << 16) | rang_bb;
					ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_FISCHER,(void *)(IntAdrs)code,k,ORGA_MAXOBJ);
					fig = FigureCreate(OrgFigType[OrgaStyle],OrgaFigConnec[OrgaStyle].modl,(void *)ref,OrgaMagasinClique);
				} else fig = FigureCreate(OrgFigType[OrgaStyle],OrgaFigConnec[OrgaStyle].modl,0,0);
				if(OrgaStyle == ORGA_ICONES) FigureLegende(fig,FIG_A_DROITE,slot); else FigureTexte(fig,slot);
				// if(OrgaStyle == ORGA_ICONES) FigureEcrit(fig,WND_GC_LITE);
#else
				fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSlot),(void *)ref,OrgaMagasinClique); FigureTexte(fig,slot);
#endif
				BoardAddFigure(planche,fig,xslt,ybb-1,0);
				if(connecteur && (connecteur != CABLAGE_CONN_DIR)) {
					CablageAnalyseConnecteur(connecteur,&quadrant,&distance);
					if((quadrant < ORGA_QUADRANT_MAX) && (distance < ORGA_RAIL_MAX)) {
						OrgaPositionFischer[quadrant][distance].xinf = xslt; 
						OrgaPositionFischer[quadrant][distance].xsup = xslt + OrgaZoneSlot.larg; 
						OrgaPositionFischer[quadrant][distance].yinf = ybb; 
						OrgaPositionFischer[quadrant][distance].ysup = ybb + OrgaZoneSlot.haut;
					}
				}
				if(detectr) bb = detectr->captr[cap].bb.num; else bb = -1;
				if((bb < 0) && connecteur) /* regarder aussi si le BB est deja branche (detecteur pas encore en place) */ {
					for(bb=0; bb<NumeriseurNb; bb++) if(connecteur == Numeriseur[bb].fischer) break;
					if(bb >= NumeriseurNb) bb = -1;
				}
				if(bb >= 0) {
					decal = decal_ht = 0;
					if(OrgaMagMode) {
						for(i=0; i<OrgaNumerNb; i++) if(!strcmp(Numeriseur[bb].fichier,OrgaNumerListe[i])) break;
						if(i >= OrgaNumerNb) {
							for(i=0; i<OrgaNumerNb; i++) if(!strcmp(Numeriseur[bb].nom,OrgaNumerListe[i])) break;
						}
						if(i < OrgaNumerNb) ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_NUMER,(void *)(IntAdrs)i,k,ORGA_MAXOBJ); else ref = 0;
#ifdef AVEC_ICONE_NUMER
						fig = FigureCreate(OrgFigType[OrgaStyle],OrgaFigNumer[OrgaStyle].modl,(void *)ref,OrgaMagasinClique);
						if(OrgaStyle == ORGA_ICONES) {
							FigureLegende(fig,FIG_A_GAUCHE,Numeriseur[bb].nom);
							decal = WndColToPix(12 - (int)strlen(Numeriseur[bb].nom)) + 2;
							decal_ht = -2;
						} else FigureTexte(fig,Numeriseur[bb].nom);
#else
						fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneRepartOK),(void *)ref,OrgaMagasinClique);
						FigureTexte(fig,Numeriseur[bb].nom);
#endif
					} else {
#ifdef AVEC_ICONE_NUMER
						fig = FigureCreate(OrgFigType[OrgaStyle],OrgaFigNumer[OrgaStyle].modl,(void *)(&(Numeriseur[bb])),NumeriseurLectClique);
						if(OrgaStyle == ORGA_ICONES) {
							FigureLegende(fig,FIG_A_GAUCHE,Numeriseur[bb].nom); 
							decal = WndColToPix(12 - (int)strlen(Numeriseur[bb].nom)) + 2;
							decal_ht = -2;
						} else FigureTexte(fig,Numeriseur[bb].nom);
#else
						fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneRepartOK),(void *)(&(Numeriseur[bb])),NumeriseurLectClique);
						FigureTexte(fig,Numeriseur[bb].nom);
#endif
					}
					BoardAddFigure(planche,fig,xbb+decal,ybb+decal_ht,0);
			
					if(OrgaMagMode) fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneFibre),(void *)&(Numeriseur[bb]),OrgaFibreSurNumeriseur);
					else fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneFibre),0,0);
					if(Numeriseur[bb].def.fibre[0]) FigureTexte(fig,Numeriseur[bb].def.fibre);
					BoardAddFigure(planche,fig,xfib-2,ybb+(OrgaZoneRepartOK.haut/2)-1,0);
					if(OrgaStyle == ORGA_ICONES) OrgaLiaisonAjoute(liens,
						xfib + OrgaZoneFibre.larg,ybb+(OrgaZoneRepartOK.haut/2)-1+(OrgaZoneFibre.haut/2),xbb+decal,-1);
				}
			}
			rang_prec = rang_bb;
			fin = modele_det->nbcapt;
			if(CablageDeplie[pos_hexa]) for(svt=cap; svt<fin; svt++) {
				x = xslt + OrgaFigConnec[OrgaStyle].larg; if(OrgaStyle == ORGA_ICONES)  x += ((lconn + 1) * Dx);
				decal_dr = 0;
				if(modele_cable->capteur[svt].numer_index != rang_bb) continue;
				decal = (svt - (modele_det->nbcapt / 2)) * ICONE_DECAL; decal_dr = decal;
				yadc[svt] = yvbb + (OrgaZoneAdc.haut / 2);
#ifdef AVEC_ICONE_CONNEC
				OrgaLiaisonAjoute(liens,x,ybb + (OrgaFigConnec[OrgaStyle].haut / 2) + decal,xadc,yadc[svt] + decal_dr);
#else
				OrgaLiaisonAjoute(liens,xslt + OrgaZoneSlot.larg,ybb + (OrgaZoneRepartOK.haut / 2) + decal,xadc,yadc[svt] + decal_dr);
#endif
				fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneAdc),(void *)(&(detectr->captr[svt])),OrgaCablageChoixAdc);
				sprintf(titre,"ADC%d",cablage->captr[svt].vbb+1); FigureTexte(fig,titre); 
				BoardAddFigure(planche,fig,xadc,yvbb,0);
				yadc[svt] += decal;
				yvbb += (OrgaZoneRepartOK.haut + ICONE_MARGE);
			} else {
				x = xslt + OrgaFigConnec[OrgaStyle].larg - 2; if(OrgaStyle == ORGA_ICONES)  x += ((lconn + 1) * Dx);
				decal = (rang_bb - (cablage->nb_fischer / 2)) * ICONE_DECAL;
#ifdef AVEC_ICONE_CONNEC
				OrgaLiaisonAjoute(liens,x,yvbb + (OrgaFigConnec[OrgaStyle].haut / 2) + decal,
								  xadc,ydet + (OrgaFigCable[OrgaStyle].haut / 2) + decal);
#else
				OrgaLiaisonAjoute(liens,xslt + OrgaZoneSlot.larg,yvbb + (OrgaZoneRepartOK.haut / 2) + decal,
								  xadc,ydet + (OrgaZoneNumer.haut / 2) + decal);
#endif
				if(cap == 0) {
					ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_CABLE,(void *)((IntAdrs)pos_hexa),k,ORGA_MAXOBJ);
#ifdef AVEC_ICONE
					fig = FigureCreate(OrgFigType[OrgaStyle],OrgaFigCable[OrgaStyle].modl,(void *)ref,OrgaMagasinClique);
#else
					fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneNumer),(void *)ref,OrgaMagasinClique);
#endif
					FigureTexte(fig,modele_cable->nom); 
					BoardAddFigure(planche,fig,xadc,ydet,0);
				}
				yvbb += (OrgaZoneRepartOK.haut + ICONE_MARGE);
			}
		}
		OrgaLiaisonTrace(liens);
		
		if(CablageDeplie[pos_hexa]) {
			ycap = y0; 
			for(cap=0; cap<modele_det->nbcapt; cap++) {
				if(yadc[cap]) OrgaLiaisonAjoute(liens,xadc + OrgaZoneAdc.larg,yadc[cap],xcap,ycap + (OrgaZoneCapt.haut / 2));
				else {
					fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSoft),0,0); FigureTexte(fig,"soft"); 
					BoardAddFigure(planche,fig,xadc,ycap,0);
					OrgaLiaisonAjoute(liens,xadc + OrgaZoneSoft.larg,ycap + (OrgaZoneSoft.haut / 2),xcap,ycap + (OrgaZoneCapt.haut / 2));
				}
				nom_voie = ModeleVoie[modele_det->type[cap]].nom;
				if(!OrgaMagMode && detectr) voie = detectr->captr[cap].voie; else voie = -1;
				if(voie >= 0) fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneCapt),(void *)(IntAdrs)voie,ReglageClique);
				else fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneCapt),0,0);
				FigureTexte(fig,nom_voie);
				BoardAddFigure(planche,fig,xcap,ycap,0);
				ycap += (OrgaZoneVoie.haut + ICONE_MARGE);
			}
			OrgaLiaisonCroisee(liens,1);
			OrgaLiaisonTrace(liens);
			OrgaLiaisonCroisee(liens,0);
			
			ycap = y0;
			for(cap=0; cap<modele_det->nbcapt; cap++) {
				decal = (cap - (modele_det->nbcapt / 2)) * ICONE_DECAL;
				OrgaLiaisonAjoute(liens,xcap + OrgaZoneCapt.larg,ycap + (OrgaZoneCapt.haut / 2),
								  xdet,ydet + (OrgaZoneRepartOK.haut / 2) + decal);
				ycap += (OrgaZoneVoie.haut + ICONE_MARGE);
			}
			ycap -= ICONE_MARGE;
		} else {
			if(OrgaStyle == ORGA_ZONES) OrgaLiaisonAjoute(liens,xadc + OrgaZoneNumer.larg,ydet + (OrgaZoneNumer.haut / 2),
				xdet,ydet + (OrgaZoneRepartOK.haut / 2));
			ycap = yvbb;
		}
	} else { ydet = y0 + (OrgaZoneSlot.haut + ICONE_MARGE); ycap = ydet; }
	
	if(log) printf("(%s) affichage du slot en (%d, %d)\n",__func__,xdet,ydet-1);
	if(!ChassisDetecUnique) strcpy(slot,CablageEncodePosition(pos_hexa)); else slot[0] = '\0';
	ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_POSITION,(void *)((IntAdrs)pos_hexa),k,ORGA_MAXOBJ);
#ifdef AVEC_ICONE
	fig = FigureCreate(OrgFigType[OrgaStyle],OrgaFigSlot[OrgaStyle].modl,(void *)ref,OrgaMagasinClique);
	if(OrgaStyle == ORGA_ICONES) FigureLegende(fig,FIG_EN_DESSOUS,slot); else FigureTexte(fig,slot);
	if(ycap < (ydet + OrgaFigSlot[OrgaStyle].haut)) ycap = ydet + OrgaFigSlot[OrgaStyle].haut;
#else
	fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSlot),(void *)ref,OrgaMagasinClique);
	FigureTexte(fig,slot);
	if(ycap < (ydet + OrgaZoneSlot.haut)) ycap = ydet + OrgaZoneSlot.haut;
#endif
	BoardAddFigure(planche,fig,xdet,ydet-1,0);
	OrgaPositionSlot[galette][tour].xinf = xdet; //  - OrgaZoneNumer.larg; 
	OrgaPositionSlot[galette][tour].xsup = xdet + OrgaZoneSlot.larg;
	OrgaPositionSlot[galette][tour].yinf = ydet; 
	OrgaPositionSlot[galette][tour].ysup = ydet + OrgaZoneSlot.haut; 
	if(!detectr) {
		int bolo;
		for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].pos == pos_hexa) break;
		if(bolo < BoloNb) detectr = &(Bolo[bolo]);
	}
	if(detectr) {
#ifdef AVEC_ICONE
		if(OrgaMagMode) {
			for(i=0; i<OrgaDetecNb; i++) if(!strcmp(detectr->nom,OrgaDetecListe[i])) break;
			if(i >= OrgaDetecNb) {
				for(i=0; i<OrgaDetecNb; i++) if(!strcmp(detectr->nom,OrgaDetecListe[i])) break;
			}
			if(i < OrgaDetecNb) ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_DETEC,(void *)(IntAdrs)i,k,ORGA_MAXOBJ); else ref = 0;
			fig = FigureCreate(OrgFigType[OrgaStyle],OrgaFigDetec[OrgaStyle].modl,(void *)ref,OrgaMagasinClique);
		} else fig = FigureCreate(OrgFigType[OrgaStyle],OrgaFigDetec[OrgaStyle].modl,0,0);
		if(OrgaStyle == ORGA_ICONES) {
			FigureLegende(fig,FIG_A_DROITE,detectr->nom); 
			BoardAddFigure(planche,fig,xdet,ydet,0);
		} else {
			FigureTexte(fig,detectr->nom);
			BoardAddFigure(planche,fig,xdet+OrgaZoneSlot.larg+4,ydet,0);
		}
#else
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneDetec),(void *)ref,OrgaMagasinClique);
		FigureTexte(fig,detectr->nom);
		BoardAddFigure(planche,fig,xdet+OrgaZoneSlot.larg+4,ydet,0);
#endif
	}
	
	OrgaLiaisonTrace(liens);

	return(ycap + ICONE_MARGE - y0);
}
/* ========================================================================== */
static void OrgaAjouteIconeBarre(TypeOrgaFig *orgafig, char *titre, ORGA_MODES mode, int *x, int y, int *ymin) {
	Figure fig; int lngr;
	
	fig = FigureCreate(OrgFigType[OrgaStyle],orgafig[OrgaStyle].modl,(void *)(IntAdrs)mode,OrgaAtelierClique);
	if(OrgaStyle == ORGA_ICONES) FigureLegende(fig,FIG_EN_DESSOUS,titre); else FigureTexte(fig,titre);
	FigureEcrit(fig,(OrgaMagMode == mode)? WND_GC_VERT: WND_GC_NOIR);
	BoardAddFigure(bOrgaCryostat,fig,*x,y,0);
	lngr = WndColToPix((int)strlen(titre)); if(lngr < orgafig[OrgaStyle].larg) lngr = orgafig[OrgaStyle].larg;
	*x += (lngr + (2 * Dx));
	if(*ymin < OrgaFigLecVoie[OrgaStyle].haut) *ymin = OrgaFigLecVoie[OrgaStyle].haut;
}
/* ========================================================================== */
int OrgaMagasinMontre() {
	int i,k,x,y,xmin,ymin,ytitre,xcryo,yetagere,larg,mode; int xslot,yslot; int debut_cryo;
	short galette,tour,tourmin,tourmax; short quadrant,distance;
	Figure fig; OrgaRef ref; Menu menu;

	OrgaTraitSepareMag.dy = 0;
	
#ifdef IDEE_NO_1
	{
		Panel p;
		p = PanelCreate(1);
		PanelKeyB(p,"Mode",OrgaMagCles,&OrgaMagMode,12); PanelSupport(p,WND_CREUX);
		// PanelMode(p,PNL_DIRECT); PanelItemReturn(p,1,OrgaMagasinRefait,0);
		PanelOnApply(p,OrgaMagasinRefait,0);
		BoardAddPanel(bOrgaCryostat,p,Dx,WndLigToPix(3),0);
	}
#endif

	xmin = 0;
	if(OrgaStyle == ORGA_ICONES) {
		x = Dx; y = Dy; ymin = 0;
//		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaCadreVide),(void *)0,OrgaAtelierClique); FigureEcrit(fig,WND_GC_LITE);
//		if(OrgaStyle == ORGA_ICONES) FigureLegende(fig,FIG_EN_DESSOUS,"Lecture"); else FigureTexte(fig,"Lecture");
//		BoardAddFigure(bOrgaCryostat,fig,x,y,0); x += (OrgaCadreVide.larg + 32);
		OrgaAjouteIconeBarre(OrgaFigLecVoie,L_("Lecture"),0,&x,y,&ymin);
		OrgaAjouteIconeBarre(OrgaFigRepart, L_("Repartiteurs"),ORGA_MAG_REPART,&x,y,&ymin);
		OrgaAjouteIconeBarre(OrgaFigNumerGd,L_("Numeriseurs"),ORGA_MAG_NUMER,&x,y,&ymin);
		OrgaAjouteIconeBarre(OrgaFigCableGd,L_("Cablage"),ORGA_MAG_CABLG,&x,y,&ymin);
		OrgaAjouteIconeBarre(OrgaFigDetecGd,L_("Detecteurs"),ORGA_MAG_DETEC,&x,y,&ymin);
		xmin = x;
		ymin += (y + (3 * Dy));

	} else {
		menu = MenuLocalise(0);
		k = 0; while(OrgaMagListe[k][0]) k++;
		MenuColumns(menu,k); OpiumSupport(menu->cdr,WND_PLAQUETTE);
		if(MenuItemArray(menu,k)) for(i=0; i<k; i++) {
			MenuItemAdd(menu,&(OrgaMagListe[i][0]),MNU_FONCTION OrgaMagasinReconstruit);
			if(i == OrgaMagMode) MenuItemAllume(menu,i+1,0,GRF_RGB_GREEN);
			else MenuItemEteint(menu,i+1,0);
		} else { MenuDelete(menu); menu = 0; }
		BoardAddMenu(bOrgaCryostat,menu,Dx,WndLigToPix(3),0);
		ymin = WndLigToPix(6);
	}
		
	k = 0;
#ifdef AVEC_ICONE
	OrgaGaletteDx = (OrgaLngrFibre + OrgaFigNumer[OrgaStyle].larg + OrgaFigConnec[OrgaStyle].larg + 
					 OrgaFigCable[OrgaStyle].larg + OrgaFigSlot[OrgaStyle].larg + OrgaFigDetec[OrgaStyle].larg + (2 * ICONE_ETAGE) + 20 + Dx);
	if(OrgaStyle == ORGA_ICONES) OrgaGaletteDx += WndColToPix(12 + 4 + 8); // textes a cote des icones */
#else
	OrgaGaletteDx = (OrgaLngrFibre + OrgaZoneRepartOK.larg + OrgaZoneSlot.larg +  OrgaZoneAdc.larg + OrgaZoneCapt.larg + OrgaZoneDetec.larg + (5 * ICONE_ETAGE));
#endif
	OrgaFinUsine = Dx;
	
	/* .......... magasin .......... */
	x = OrgaFinUsine; y = ymin;
	if(OrgaMagMode && (OrgaMagMode != ORGA_MAG_CABLG)) {
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneInactive),0,0); FigureTexte(fig,L_("Usine"));
		BoardAddFigure(bOrgaCryostat,fig,x,y,0); y += (OrgaZoneInactive.haut + (2 * ICONE_MARGE));
		OrgaFinUsine += (OrgaZoneInactive.larg + (2 * ICONE_INTERVALLE));
	}
	if(OrgaMagMode == ORGA_MAG_REPART) {
		for(i=0; i<ModeleRepNb; i++) {
			ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_MODELE,(void *)(IntAdrs)i,&k,ORGA_MAXOBJ);
			fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneModl),(void *)ref,OrgaMagasinClique); FigureTexte(fig,ModeleRep[i].nom);
			BoardAddFigure(bOrgaCryostat,fig,x,y,0); y += (OrgaZoneModl.haut + ICONE_MARGE);
		}
	} else if(OrgaMagMode == ORGA_MAG_NUMER) {
		for(i=0; i<ModeleBNNb; i++) {
			ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_MODELE,(void *)(&(ModeleBN[i])),&k,ORGA_MAXOBJ);
			fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneModl),(void *)ref,OrgaMagasinClique); FigureTexte(fig,ModeleBN[i].nom);
			BoardAddFigure(bOrgaCryostat,fig,x,y,0); y += (OrgaZoneModl.haut + ICONE_MARGE);
		}
	} else if(OrgaMagMode == ORGA_MAG_DETEC) {
		for(i=0; i<ModeleDetNb; i++) {
			ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_MODELE,(void *)(&(ModeleDet[i])),&k,ORGA_MAXOBJ);
			fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneModl),(void *)ref,OrgaMagasinClique); FigureTexte(fig,ModeleDet[i].nom);
			BoardAddFigure(bOrgaCryostat,fig,x,y,0); y += (OrgaZoneModl.haut + ICONE_MARGE);
		}
	}
	if(y > OrgaTraitSepareMag.dy) OrgaTraitSepareMag.dy = y;
	
	OrgaFinEtagere = OrgaFinUsine;
	if(OrgaMagMode == ORGA_MAG_REPART) {
		ymin -= Dy;
		OrgaEspaceRepartition(OrgaMagMode,OrgaFinUsine,ymin);
		return(0);
	}

	/* .......... etagere .......... */
	x = OrgaFinUsine; y = ymin; larg = ICONE_INTERVALLE + OrgaZoneInactive.larg;
	if(OrgaMagMode) {
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneInactive),0,0); FigureTexte(fig,L_("Etagere"));
		BoardAddFigure(bOrgaCryostat,fig,x,y,0); y += (OrgaZoneInactive.haut + (2 * ICONE_MARGE));
	}
	if(OrgaMagMode == ORGA_MAG_NUMER) {
		larg = OrgaZoneNumer.larg;
		for(i=0; i<OrgaNumerNb; i++) if(!OrgaNumerNum[i]) {
			ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_NUMER,(void *)(IntAdrs)i,&k,ORGA_MAXOBJ);
#ifdef AVEC_ICONE_NUMER
			fig = FigureCreate(OrgFigType[OrgaStyle],OrgaFigNumer[OrgaStyle].modl,(void *)ref,OrgaMagasinClique);
			if(OrgaStyle == ORGA_ICONES) FigureLegende(fig,FIG_A_DROITE,OrgaNumerListe[i]); else FigureTexte(fig,OrgaNumerListe[i]);
#else
			fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneRepartOK),(void *)ref,OrgaMagasinClique); FigureTexte(fig,OrgaNumerListe[i]);
#endif
			BoardAddFigure(bOrgaCryostat,fig,x,y,0); y += (OrgaZoneRepartOK.haut + ICONE_MARGE);
		}
	} else if(OrgaMagMode == ORGA_MAG_CABLG) {
		larg = OrgaFigCable[OrgaStyle].larg;
		for(i=0; i<ModeleCableNb; i++) {
			ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_MODELE,(void *)(IntAdrs)i,&k,ORGA_MAXOBJ);
#ifdef AVEC_ICONE
			fig = FigureCreate(OrgFigType[OrgaStyle],OrgaFigCable[OrgaStyle].modl,(void *)ref,OrgaMagasinClique);
#else
			fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneNumer),(void *)ref,OrgaMagasinClique);
#endif
			FigureTexte(fig,ModeleCable[i].nom);
			BoardAddFigure(bOrgaCryostat,fig,x,y,0); y += (OrgaZoneNumer.haut + ICONE_MARGE);
		}
	} else if(OrgaMagMode == ORGA_MAG_DETEC) {
		larg = OrgaFigDetec[OrgaStyle].larg;
		for(i=0; i<OrgaDetecNb; i++) if(!OrgaDetecNum[i]) {
			ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_DETEC,(void *)(IntAdrs)i,&k,ORGA_MAXOBJ);
#ifdef AVEC_ICONE
			fig = FigureCreate(OrgFigType[OrgaStyle],OrgaFigDetec[OrgaStyle].modl,(void *)ref,OrgaMagasinClique);
			if(OrgaStyle == ORGA_ICONES) FigureLegende(fig,FIG_A_DROITE,OrgaDetecListe[i]); else FigureTexte(fig,OrgaDetecListe[i]);
#else
			fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneNumer),(void *)ref,OrgaMagasinClique);
			FigureTexte(fig,OrgaDetecListe[i]);
#endif
			BoardAddFigure(bOrgaCryostat,fig,x,y,0); y += (OrgaZoneNumer.haut + ICONE_MARGE);
		}
	}
	if(OrgaMagMode) {
		if(larg < (ICONE_INTERVALLE + OrgaZoneInactive.larg)) larg = ICONE_INTERVALLE + OrgaZoneInactive.larg;
		OrgaFinEtagere += (larg + ICONE_INTERVALLE);
	} else OrgaFinEtagere += Dx;
	if(y > OrgaTraitSepareMag.dy) OrgaTraitSepareMag.dy = y;
	yetagere = OrgaTraitSepareMag.dy;
	
	/* .......... cryostat .......... */
	debut_cryo = OrgaFinEtagere;
	x = debut_cryo; y = ymin;
	if(OrgaMagMode) {
		xcryo = (((OrgaGalMax - OrgaGalMin) * OrgaGaletteDx) - OrgaZoneInactive.larg) / 2;
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneInactive),0,0); FigureTexte(fig,L_("Zone mesures"));
		BoardAddFigure(bOrgaCryostat,fig,x+xcryo,y,0); y += (OrgaZoneInactive.haut + (2 * ICONE_MARGE));
	}
	for(quadrant=0; quadrant<ORGA_QUADRANT_MAX; quadrant++) {
		for(distance=0; distance<ORGA_RAIL_MAX; distance++) {
			OrgaPositionFischer[quadrant][distance].xinf = 0; 
			OrgaPositionFischer[quadrant][distance].xsup = 0; 
			OrgaPositionFischer[quadrant][distance].yinf = 0; 
			OrgaPositionFischer[quadrant][distance].ysup = 0; 
		}
	}
	OrgaChassisX0 = x; OrgaChassisY0 = y;
	xslot = x + Dx;
	for(galette=OrgaGalMin; galette<OrgaGalMax; galette++) {
		yslot = y;
		tourmin = (ChassisDetec[1].codage == CHASSIS_CODE_1)? 1: 0;
		tourmax = tourmin + ChassisDetec[1].nb;
		for(tour=tourmin; tour<tourmax; tour++) {
			yslot += OrgaTraceCablage(bOrgaCryostat,xslot,yslot,galette,tour,&LiaisonsCablage,&k);
			if(OrgaTraitSepareMag.dy < yslot) OrgaTraitSepareMag.dy = yslot;
		}
		xslot += OrgaGaletteDx;
	}

	if(OrgaMagMode) {
		yetagere += ((2 * Dy) + OrgaIconePoubelle.haut);
		if(yetagere > OrgaTraitSepareMag.dy) OrgaTraitSepareMag.dy = yetagere;
		OrgaPoubelle = FigureCreate(FIG_ICONE,(void *)(&OrgaIconePoubelle),0,OrgaMagasinClique);
		BoardAddFigure(bOrgaCryostat,OrgaPoubelle,OrgaFinUsine,OrgaTraitSepareMag.dy-OrgaIconePoubelle.haut,0);
	}

	/* .......... separations .......... */
	fig = FigureCreate(FIG_DROITE,(void *)(&OrgaTraitSepareIcn),0,0);
	BoardAddFigure(bOrgaCryostat,fig,0,OrgaTraitSepareMag.dy,0);
	menu = MenuBouton(L_("Enregistrer"), MNU_FONCTION OrgaMagasinEnregistre);
	if(!OrgaMagMode) for(mode=ORGA_MAG_REPART; mode <= ORGA_MAG_MAX; mode++) if(OrgaMagModifie[mode]) { OrgaMagModifie[OrgaMagMode] = 1; break; }
	if(OrgaMagModifie[OrgaMagMode]) MenuItemAllume(menu,1,0,GRF_RGB_YELLOW);
	BoardAddMenu(bOrgaCryostat,menu,OrgaFinEtagere+Dx,OrgaTraitSepareMag.dy+Dy,0);
	ymin -= Dy;
	OrgaTraitSepareMag.dy -= ymin;
	fig = FigureCreate(FIG_DROITE,(void *)(&OrgaTraitSepareMag),0,0);
	if(OrgaFinUsine > ICONE_INTERVALLE) {
		fig = FigureCreate(FIG_DROITE,(void *)(&OrgaTraitSepareMag),0,0);
		BoardAddFigure(bOrgaCryostat,fig,OrgaFinUsine-ICONE_INTERVALLE,ymin,0);
	}
	if(OrgaFinEtagere > ICONE_INTERVALLE) {
		fig = FigureCreate(FIG_DROITE,(void *)(&OrgaTraitSepareMag),0,0);
		BoardAddFigure(bOrgaCryostat,fig,OrgaFinEtagere-ICONE_INTERVALLE,ymin,0);
	}
	x = OrgaChassisX0 - (ICONE_ETAGE / 2);
	ytitre = OrgaMagMode? OrgaChassisY0 - ymin: 0;
	OrgaTraitSepareGal.dy = OrgaTraitSepareMag.dy - ytitre;
	for(galette=OrgaGalMin; galette<OrgaGalMax-1; galette++) {
		x += OrgaGaletteDx;
		fig = FigureCreate(FIG_DROITE,(void *)(&OrgaTraitSepareGal),0,0);
		BoardAddFigure(bOrgaCryostat,fig,x,ymin + ytitre,0);
	}
	if(x < xmin) x = xmin;
	OrgaTraitSepareIcn.dx = x + OrgaGaletteDx;
	if(OrgaStyle == ORGA_ICONES) {
		if(OrgaGalettesNb > 1) for(i=0; i<OrgaGalettesNb; i++) {
			fig = FigureCreate(OrgFigType[OrgaStyle],OrgaFigGalette[OrgaStyle].modl,(void *)(IntAdrs)i,OrgaGaletteClique);
			if(OrgaStyle == ORGA_ICONES) FigureLegende(fig,FIG_EN_DESSOUS,OrgaGalettesNom[i]); else FigureTexte(fig,OrgaGalettesNom[i]);
			FigureEcrit(fig,(OrgaCablageMontre == i)? WND_GC_VERT: WND_GC_NOIR);
			BoardAddFigure(bOrgaCryostat,fig,x,Dy,0); x += (OrgaFigGalette[OrgaStyle].larg + ICONE_ETAGE);
		}
	}
	fig = FigureCreate(FIG_DROITE,(void *)(&OrgaTraitSepareIcn),0,0);
	BoardAddFigure(bOrgaCryostat,fig,0,ymin,0);

	return(0);
}
/* ========================================================================== */
static void OrgaMagasinOnglet(int numero) {

	if(!OrgaPrete) return;
	if(bOrgaCryostat) {
		if(OpiumDisplayed(bOrgaCryostat)) OpiumClear(bOrgaCryostat);
		BoardTrash(bOrgaCryostat);
		OrgaPoubelle = 0;
		if(LiaisonsCablage.lien) { free(LiaisonsCablage.lien); LiaisonsCablage.lien = 0; }
	} else bOrgaCryostat = BoardCreate();
	if(!bOrgaCryostat) return;
	OpiumTitle(bOrgaCryostat,"Atelier de Cablage");

	if(OrgaGalettesNb > 1) {
		OrgaCablageMontre = numero;
		if(OrgaStyle == ORGA_ZONES) {
			OrgaGalettes = OngletDefini(OrgaGalettesNom,OrgaMagasinOnglet);
			OngletDemande(OrgaGalettes,OrgaCablageMontre);
			BoardAddOnglet(bOrgaCryostat,OrgaGalettes,0);
		}
		OrgaGalMin = ORGA_GALETTES_LARG * OrgaCablageMontre;
		OrgaGalMax = ORGA_GALETTES_LARG * (OrgaCablageMontre+1); if(OrgaGalMax > GALETTES_NB) OrgaGalMax = GALETTES_NB;
	} else {
		OrgaCablageMontre = numero; // 0;
		OrgaGalMin = 0; OrgaGalMax = GALETTES_NB;
	}
	OrgaMagasinMontre();
	BoardOnClic(bOrgaCryostat,OrgaCliquePlanche,0);
	
	OpiumFork(bOrgaCryostat);
}
/* ========================================================================== */
int OrgaMagasinRefait() { OrgaMagasinOnglet(OrgaCablageMontre); return(0); }
/* ========================================================================== */
static int OrgaMagasinReconstruit(Menu menu, MenuItem item) {
	int k;
	
	k = 0;
	while(OrgaMagListe[k][0]) {
		if(item->texte == OrgaMagListe[k]) { OrgaMagMode = k; break; }
		k++;
	}
	OrgaMagasinOnglet(OrgaCablageMontre);
	return(0);
}
/* ========================================================================== */
int OrgaMagasinVisite(Menu menu, MenuItem item) {
	char nom_complet[MAXFILENAME]; int i,bolo,bb; size_t l;// int k,ke,ks;
	
	RepertoireIdentique(FichierPrefDetecteurs,DetecteurStock,nom_complet);
	RepertoireListeCree(0,nom_complet,OrgaDetecListe,ORGA_MAX_ETAGERE,&OrgaDetecNb);
	for(i=0; i<OrgaDetecNb; i++) {
		OrgaDetecNum[i] = -1;
		if(!strncmp(OrgaDetecListe[i],"liste",5)) continue;  // faire plus propre
		if(!strncmp(OrgaDetecListe[i],DETEC_STANDARD,8)) continue;  // faire plus propre
		for(bolo=0; bolo<BoloNb; bolo++) if(!strcmp(Bolo[bolo].fichier,OrgaDetecListe[i])) break;
		if(bolo >= BoloNb) {
			for(bolo=0; bolo<BoloNb; bolo++) if(!strcmp(Bolo[bolo].nom,OrgaDetecListe[i])) break;
		}
		OrgaDetecNum[i] = (bolo < BoloNb)? bolo + 1: 0;
	}
	
	RepertoireIdentique(NumeriseurFichierLu,NumeriseurStock,nom_complet);
	RepertoireListeCree(0,nom_complet,OrgaNumerListe,ORGA_MAX_ETAGERE,&OrgaNumerNb);
	// printf("(%s) %d fichier%s dans %s\n",__func__,Accord1s(OrgaNumerNb),nom_complet); ke = ks = 0;
	for(i=0; i<OrgaNumerNb; i++) {
		OrgaNumerNum[i] = -1;
		if(!strncmp(OrgaNumerListe[i],"liste",5)) continue;  // faire plus propre
		OrgaNumerNum[i] = 0;
		l = strlen(OrgaNumerListe[i]);
		for(bb=0; bb<NumeriseurNb; bb++) if(!strcmp(Numeriseur[bb].fichier,OrgaNumerListe[i])) break;
		if((bb < NumeriseurNb) && (l >= strlen(Numeriseur[bb].nom))) strcpy(OrgaNumerListe[i],Numeriseur[bb].nom);
		else {
			while(l--) if(OrgaNumerListe[i][l] == '_') OrgaNumerListe[i][l] = '#';
			if(bb >= NumeriseurNb) { for(bb=0; bb<NumeriseurNb; bb++) if(!strcmp(Numeriseur[bb].nom,OrgaNumerListe[i])) break; }
		}
		if(bb < NumeriseurNb) { if(Numeriseur[bb].fischer) OrgaNumerNum[i] = bb + 1; }
		// if(OrgaNumerNum[i] == 0) { ke++; k = ke; } else if(OrgaNumerNum[i] > 0) { ks++; k = ks; }
		// printf("(%s) | %3d | %7s #%-2d (%4s) | %s\n",__func__,i+1,
		// 	(OrgaNumerNum[i] < 0)? "masque": ((OrgaNumerNum[i] == 0)? "etagere": "connecteur"),k,
		// 	(bb < NumeriseurNb)? ((Numeriseur[bb].fischer)? CablageEncodeConnecteur(Numeriseur[bb].fischer) : "pose"): "jete",
		// 	OrgaNumerListe[i]);
	}
	// printf("(%s) %d sur etagere, %d sur connecteur\n",__func__,ke,ks);
	OrgaPrete = 1;
	
	OrgaMagasinRefait();
	
	return(0);
}
#pragma mark ------ Espaces specialises -------
/* ========================================================================== */
/*  Espaces specialises                                                       */
/* ========================================================================== */
static int OrgaCliqueSurMac(Figure fig, WndAction *e) {
	if(e->code == WND_MSERIGHT) {
#ifdef TABLES_A_TERMINER
		int rep; char message[256];
		OpiumTableExec("Repartiteurs",ARG_TYPE_STRUCT,(void *)&RepartiteurAS,&RepartNb,1,0);
		for(rep=0; rep<RepartNb; rep++) {
			// RepartiteurInit(); inutile?
			RepartiteurVerifieParmsEtFIFO(&(Repart[rep]),message);
			if(message[0]) printf("! %s. Repartiteur %s invalide.\n",message,Repart[rep].nom);
			if(!Repart[rep].valide) Repart[rep].nbentrees = 0;
		}
		RepartiteurMajListes();
		OrgaTraceTout(0,0);
#endif
		RepartiteurRedefiniPerimetre(0,0);
	} else {
		OpiumFork(bLecture);
	}
	return(0);
}
/* ========================================================================== */
static int OrgaNumerEnregistre(Menu menu, MenuItem item) {
	NumeriseursEcrit(NumeriseurFichierLu,NUMER_CONNECTION,NUMER_TOUS);
	return(0);
}
/* ========================================================================== */
static int OrgaEspaceNumerClique(Figure cliquee, WndAction *e) {
	Figure fig; int x,y; Cadre cdr; OrgaRef ref;
	
	if(OrgaDerniereAction == WND_PRESS) fig = OrgaDerniereFigure;
	else OrgaDerniereFigure = fig = cliquee;
	OrgaDerniereAction = e->type;
	OpiumDernierClic(&x,&y);
	if(e->type == WND_PRESS) {
		ref = (OrgaRef)(fig->adrs); if(!ref) return(0);
		if(ref->type >= ORGA_REF_POSITION) return(0);
		fig->type = FIG_CADRE; fig->forme.zone->support = WND_RAINURES;
		cdr = fig->cdr;
		cdr->x = (x - (cdr->larg / 2));
		cdr->y = (y - (cdr->haut / 2));
		cdr->f = 0;
		if(OpiumDisplayed(bOrgaCryostat)) OpiumRefresh(bOrgaCryostat);
		if(OpiumDisplayed(bEspaceNumer)) OpiumRefresh(bEspaceNumer);
	} else if(e->type == WND_RELEASE) {
		OrgaDerniereFigure = 0;
		fig->type = FIG_ZONE; fig->forme.zone->support = WND_PLAQUETTE;
		ref = (OrgaRef)(fig->adrs);
		if(cliquee == OrgaPoubelle) switch(ref->type) {
			case ORGA_REF_NUMER:  OrgaNumerRetire(fig,e,ref->adrs); break;
		} else switch(ref->type) {
			case ORGA_REF_MODELE: OrgaNumerAjoute(fig,e,ref->adrs);  break;
			case ORGA_REF_NUMER:  OrgaNumerDeplace(fig,e,ref->adrs,x,y); break;
			case ORGA_REF_FIBRE:  OrgaNumerBranche(fig,e,ref->adrs); break;
		}
	}
	return(0);
}
/* ========================================================================== */
int OrgaEspaceNumeriseur(Menu menu, MenuItem item) {
	char nom_complet[MAXFILENAME]; int i,bb; size_t l; char normaliser;
	
	RepertoireIdentique(NumeriseurFichierLu,NumeriseurStock,nom_complet);
	RepertoireListeCree(0,nom_complet,OrgaNumerListe,ORGA_MAX_ETAGERE,&OrgaNumerNb);
	for(i=0; i<OrgaNumerNb; i++) {
		OrgaNumerNum[i] = -1;
		if(!strncmp(OrgaNumerListe[i],"liste",5)) continue;  // faire plus propre
		normaliser = 1;
		l = strlen(OrgaNumerListe[i]);
		OrgaNumerNum[i] = 0;
//		for(bb=0; bb<NumeriseurNb; bb++) if(!strcmp(Numeriseur[bb].nom,OrgaNumerListe[i])) break;
		for(bb=0; bb<NumeriseurNb; bb++) if(!strcmp(Numeriseur[bb].fichier,OrgaNumerListe[i])) break;
		if(bb < NumeriseurNb) {
			if(l >= strlen(Numeriseur[bb].nom)) {
				strcpy(OrgaNumerListe[i],Numeriseur[bb].nom);
				normaliser = 0;
			}
			if(Numeriseur[bb].fischer) OrgaNumerNum[i] = bb + 1;
		}
		if(normaliser) while(l--) if(OrgaNumerListe[i][l] == '_') OrgaNumerListe[i][l] = '#';

	}
	OrgaEspaceNumerMontre();
	
	return(0);
}
/* ========================================================================== */
static int OrgaEspaceNumerMontre() {
	int i,k,x,y; int xslot,yslot,xnum,ynum,nb_ext,yext;
	short quadrant,distance; TypeCableConnecteur connecteur;
	Figure fig; OrgaRef ref;
	int bb; char slot[8];
	
	if(bEspaceNumer) {
		if(OpiumDisplayed(bEspaceNumer)) OpiumClear(bEspaceNumer);
		BoardTrash(bEspaceNumer);
		if(LiaisonsCablage.lien) { free(LiaisonsCablage.lien); LiaisonsCablage.lien = 0; }
	} else bEspaceNumer = BoardCreate();
	if(!bEspaceNumer) return(0);
	OpiumTitle(bEspaceNumer,"Magasin Numeriseurs");
	
	OrgaFinUsine = Dx + OrgaZoneInactive.larg + ICONE_INTERVALLE;
	OrgaFinEtagere = OrgaFinUsine + ICONE_INTERVALLE + OrgaZoneInactive.larg + ICONE_INTERVALLE;
	OrgaTraitSepareMag.dy = 0;
	k = 0;
	
	/* catalogue usine */
	x = Dx; y = Dy;
	fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneInactive),0,0); FigureTexte(fig,"Usine");
	BoardAddFigure(bEspaceNumer,fig,x,y,0); y += (OrgaZoneInactive.haut + (2 * ICONE_MARGE));
	for(i=0; i<ModeleBNNb; i++) {
		ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_MODELE,(void *)(&(ModeleBN[i])),&k,ORGA_MAXOBJ);
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneModl),(void *)ref,OrgaEspaceNumerClique); FigureTexte(fig,ModeleBN[i].nom);
		BoardAddFigure(bEspaceNumer,fig,x,y,0); y += (OrgaZoneModl.haut + ICONE_MARGE);
	}
	if(OrgaTraitSepareMag.dy < y) OrgaTraitSepareMag.dy = y;
	
	/* etagere */
	x = OrgaFinUsine + ICONE_INTERVALLE; y = Dy;
	fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneInactive),0,0); FigureTexte(fig,"Etagere");
	BoardAddFigure(bEspaceNumer,fig,x,y,0); y += (OrgaZoneInactive.haut + (2 * ICONE_MARGE));
	for(i=0; i<OrgaNumerNb; i++) if(!OrgaNumerNum[i]) {
		ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_NUMER,(void *)(IntAdrs)i,&k,ORGA_MAXOBJ);
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneNumer),(void *)ref,OrgaEspaceNumerClique); FigureTexte(fig,OrgaNumerListe[i]);
		BoardAddFigure(bEspaceNumer,fig,x,y,0); y += (OrgaZoneNumer.haut + ICONE_MARGE);
	}
	if(OrgaTraitSepareMag.dy < y) OrgaTraitSepareMag.dy = y;
	
	/* connecteur ou position */
	x = OrgaFinEtagere + ICONE_INTERVALLE; y = Dy;
	fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneInactive),0,0); FigureTexte(fig,"Chassis");
	BoardAddFigure(bEspaceNumer,fig,x,y,0); y += (OrgaZoneInactive.haut + (2 * ICONE_MARGE));
	
	OrgaChassisX0 = x; OrgaChassisY0 = y;
	xslot = x;
	for(quadrant=0; quadrant<QUADRANT_NB; quadrant++) {
		yslot = y;
		for(distance=0; distance<QUADRANT_LNGR; distance++) {
			connecteur = (quadrant * QUADRANT_OF7) + distance + 1;
			printf("(%s) Quadrant %d, distance %d soit connecteur=%d\n",__func__,quadrant,distance,connecteur);
			// ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_POSITION,(void *)connecteur,&k,ORGA_MAXOBJ);
			// fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSlot),(void *)ref,OrgaEspaceNumerClique); FigureTexte(fig,slot);
			fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSlot),0,0);
			if(!ChassisNumerUnique) {
				strcpy(slot,CablageEncodeConnecteur(connecteur)); FigureTexte(fig,slot);
			}
			BoardAddFigure(bEspaceNumer,fig,xslot + OrgaLngrFibre + 6 + OrgaZoneRepartOK.larg + 4,yslot,0);
			yslot += OrgaDyTour;
		}
		if(OrgaTraitSepareMag.dy < yslot) OrgaTraitSepareMag.dy = yslot;
		xslot += OrgaDxQuadrant;
	}
	
	nb_ext = 0; yext = y + (QUADRANT_LNGR * OrgaDyTour);
	for(i=0; i<OrgaNumerNb; i++) if((bb = OrgaNumerNum[i]) > 0) {
		--bb;
		// xnum = x; ynum = y;
		if(Numeriseur[bb].fischer != CABLAGE_CONN_DIR) {
			int k;
			k = Numeriseur[bb].fischer - 1;
			quadrant = k / QUADRANT_OF7; distance = k - (quadrant * QUADRANT_OF7);
			xnum = x + (quadrant * OrgaDxQuadrant); ynum = y + (distance * OrgaDyTour);
		} else {
			xnum = x + (nb_ext * OrgaDxQuadrant); ynum = yext;
			strcpy(slot,CablageEncodeConnecteur(CABLAGE_CONN_DIR));
			fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSlot),0,0); FigureTexte(fig,slot);
			BoardAddFigure(bEspaceNumer,fig,xnum + OrgaZoneRepartOK.larg + 4,ynum,0);
			if(++nb_ext >= QUADRANT_NB) { nb_ext = 0; yext += OrgaDyTour; }
		}
		ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_FIBRE,(void *)(IntAdrs)i,&k,ORGA_MAXOBJ);
		// fig = FigureCreate(FIG_DROITE,(void *)(&OrgaZoneFibre),(void *)ref,OrgaEspaceNumerClique);
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneFibre),(void *)ref,OrgaEspaceNumerClique);
		if(Numeriseur[bb].def.fibre[0]) FigureTexte(fig,Numeriseur[bb].def.fibre);
		BoardAddFigure(bEspaceNumer,fig,xnum,ynum+(OrgaZoneRepartOK.haut/2),0);
		ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_NUMER,(void *)(IntAdrs)i,&k,ORGA_MAXOBJ);
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneRepartOK),(void *)ref,OrgaEspaceNumerClique); FigureTexte(fig,Numeriseur[bb].nom);
		BoardAddFigure(bEspaceNumer,fig,xnum+OrgaLngrFibre+6,ynum+1,0);
		OrgaLiaisonTrace(&LiaisonsCablage);
		if(OrgaTraitSepareMag.dy < ynum) OrgaTraitSepareMag.dy = ynum;
	}
	
	y = yext + OrgaDyTour;
	BoardAddBouton(bEspaceNumer,"Enregistrer",OrgaNumerEnregistre,x,y,0); y += Dy;
	if(OrgaTraitSepareMag.dy < y) OrgaTraitSepareMag.dy = y;
	fig = FigureCreate(FIG_DROITE,(void *)(&OrgaTraitSepareMag),0,0);
	BoardAddFigure(bEspaceNumer,fig,OrgaFinUsine,Dy,0);
	fig = FigureCreate(FIG_DROITE,(void *)(&OrgaTraitSepareMag),0,0);
	BoardAddFigure(bEspaceNumer,fig,OrgaFinEtagere,Dy,0);
	
	BoardOnClic(bEspaceNumer,OrgaCliquePlanche,0);
	OpiumFork(bEspaceNumer);
	return(0);
}
/* ========================================================================== */
static int OrgaDetecEnregistre(Menu menu, MenuItem item) {
	DetecteurEcritTous(DETEC_LISTE,-1);
	return(0);
}
/* ========================================================================== */
static int OrgaEspaceDetecClique(Figure cliquee, WndAction *e) {
	Figure fig; int x,y; Cadre cdr; OrgaRef ref;

	if(OrgaDerniereAction == WND_PRESS) fig = OrgaDerniereFigure;
	else OrgaDerniereFigure = fig = cliquee;
	OrgaDerniereAction = e->type;
	OpiumDernierClic(&x,&y);
	if(e->type == WND_PRESS) {
		ref = (OrgaRef)(fig->adrs); if(!ref) return(0);
		if(ref->type >= ORGA_REF_POSITION) return(0);
		fig->type = FIG_CADRE; fig->forme.zone->support = WND_RAINURES;
		cdr = fig->cdr;
		cdr->x = (x - (cdr->larg / 2));
		cdr->y = (y - (cdr->haut / 2));
		cdr->f = 0;
		if(OpiumDisplayed(bOrgaCryostat)) OpiumRefresh(bOrgaCryostat);
		if(OpiumDisplayed(bEspaceDetec)) OpiumRefresh(bEspaceDetec);
	} else if(e->type == WND_RELEASE) {
		OrgaDerniereFigure = 0;
		fig->type = FIG_ZONE; fig->forme.zone->support = WND_PLAQUETTE;		
		ref = (OrgaRef)(fig->adrs);
		if(cliquee == OrgaDetecPoubelle) switch(ref->type) {
			case ORGA_REF_DETEC:  OrgaDetecRetire(fig,e,ref->adrs); break;
		} else switch(ref->type) {
			case ORGA_REF_MODELE: OrgaDetecAjoute(fig,e,ref->adrs);  break;
			case ORGA_REF_DETEC:  OrgaDetecDeplace(fig,e,ref->adrs,x,y); break;
			case ORGA_REF_FIBRE:  OrgaNumerBranche(fig,e,ref->adrs); break;
		}
	}
	return(0);
}
/* ========================================================================== */
int OrgaEspaceDetecteur(Menu menu, MenuItem item) {
	char nom_complet[MAXFILENAME]; int i,bolo;

	RepertoireIdentique(FichierPrefDetecteurs,DetecteurStock,nom_complet);
	RepertoireListeCree(0,nom_complet,OrgaDetecListe,ORGA_MAX_ETAGERE,&OrgaDetecNb);
	for(i=0; i<OrgaDetecNb; i++) {
		OrgaDetecNum[i] = -1;
		if(!strncmp(OrgaDetecListe[i],"liste",5)) continue;  // faire plus propre
		for(bolo=0; bolo<BoloNb; bolo++) if(!strcmp(Bolo[bolo].nom,OrgaDetecListe[i])) break;
		OrgaDetecNum[i] = (bolo < BoloNb)? bolo + 1: 0;
	}
	OrgaEspaceDetecMontre();
	
	return(0);
}
/* ========================================================================== */
int OrgaEspaceDetecMontre() {
	int i,k,x,y; int xslot,yslot,xdet,ydet,nb_ext,yext;
	short galette,tour,branche;
	Figure fig; OrgaRef ref;
	int bolo; char slot[8];
	
	if(bEspaceDetec) {
		if(OpiumDisplayed(bEspaceDetec)) OpiumClear(bEspaceDetec);
		BoardTrash(bEspaceDetec);
		if(LiaisonsCablage.lien) { free(LiaisonsCablage.lien); LiaisonsCablage.lien = 0; }
	} else bEspaceDetec = BoardCreate();
	if(!bEspaceDetec) return(0);
	OpiumTitle(bEspaceDetec,"Magasin Detecteurs");
	
	OrgaFinUsine = Dx + OrgaZoneInactive.larg + ICONE_INTERVALLE;
	OrgaFinEtagere = OrgaFinUsine + ICONE_INTERVALLE + OrgaZoneInactive.larg + ICONE_INTERVALLE;
	OrgaTraitSepareMag.dy = 0;
	k = 0;
	
	/* catalogue usine */
	x = Dx; y = Dy;
	fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneInactive),0,0); FigureTexte(fig,"Usine");
	BoardAddFigure(bEspaceDetec,fig,x,y,0); y += (OrgaZoneInactive.haut + (2 * ICONE_MARGE));
	for(i=0; i<ModeleDetNb; i++) {
		ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_MODELE,(void *)(&(ModeleDet[i])),&k,ORGA_MAXOBJ);
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneModl),(void *)ref,OrgaEspaceDetecClique); FigureTexte(fig,ModeleDet[i].nom);
		BoardAddFigure(bEspaceDetec,fig,x,y,0); y += (OrgaZoneModl.haut + ICONE_MARGE);
	}
	if(OrgaTraitSepareMag.dy < y) OrgaTraitSepareMag.dy = y;

	/* etagere */
	x = OrgaFinUsine + ICONE_INTERVALLE; y = Dy;
	fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneInactive),0,0); FigureTexte(fig,"Etagere");
	BoardAddFigure(bEspaceDetec,fig,x,y,0); y += (OrgaZoneInactive.haut + (2 * ICONE_MARGE));
	for(i=0; i<OrgaDetecNb; i++) if(!OrgaDetecNum[i]) {
		ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_DETEC,(void *)(IntAdrs)i,&k,ORGA_MAXOBJ);
	#ifdef AVEC_BMP
		fig = FigureCreate(FIG_ZONE,(void *)OrgaIcnDetec,(void *)ref,OrgaEspaceDetecClique); FigureTexte(fig,OrgaDetecListe[i]);
		BoardAddFigure(bEspaceDetec,fig,x,y,0); y += (OrgaIcnDetec->haut + ICONE_MARGE);
	#else
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneNumer),(void *)ref,OrgaEspaceDetecClique); FigureTexte(fig,OrgaDetecListe[i]);
		BoardAddFigure(bEspaceDetec,fig,x,y,0); y += (OrgaZoneNumer.haut + ICONE_MARGE);
	#endif
	}
	if(OrgaTraitSepareMag.dy < y) OrgaTraitSepareMag.dy = y;
	
	/* connecteur ou position */
	x = OrgaFinEtagere + ICONE_INTERVALLE; y = Dy;
	fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneInactive),0,0); FigureTexte(fig,"Chassis");
	BoardAddFigure(bEspaceDetec,fig,x,y,0); y += (OrgaZoneInactive.haut + (2 * ICONE_MARGE));

	OrgaMagMode = ORGA_MAG_REPART; // NEANT;
	OrgaChassisX0 = x; OrgaChassisY0 = y;
	xslot = x;
	for(galette=0; galette<GALETTES_NB; galette++) {
		yslot = y;
		for(tour=0; tour<TOURS_NB; tour++) {
			// ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_POSITION,(void *)pos_hexa,&k,ORGA_MAXOBJ);
			// fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSlot),(void *)ref,OrgaEspaceDetecClique); FigureTexte(fig,slot);
			fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSlot),0,0);
			if(!ChassisDetecUnique) {
				strcpy(slot,CablageEncodeCoord(CABLAGE_POSITION(galette,tour))); FigureTexte(fig,slot);
			}
			BoardAddFigure(bEspaceDetec,fig,xslot,yslot,0);
			yslot += OrgaDyTour;
			if(OrgaTraitSepareMag.dy < yslot) OrgaTraitSepareMag.dy = yslot;
		}
		xslot += OrgaDxGalette;
	}
	
	nb_ext = 0; yext = y + (TOURS_NB * OrgaDyTour);
	for(i=0; i<OrgaDetecNb; i++) if((bolo = OrgaDetecNum[i]) > 0) {
		--bolo;
		// xdet = x; ydet = y;
		if(Bolo[bolo].pos != CABLAGE_POS_NOTDEF) {
			CablageAnalysePosition(Bolo[bolo].pos,&galette,&tour,&branche);
			xdet = x + (galette * OrgaDxGalette); ydet = y + (tour * OrgaDyTour);
		} else {
			xdet = x + (nb_ext * OrgaDxGalette); ydet = yext;
			strcpy(slot,CablageEncodeCoord(CABLAGE_POS_NOTDEF));
			fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSlot),0,0); FigureTexte(fig,slot);
			BoardAddFigure(bEspaceDetec,fig,xdet,ydet,0);
			if(++nb_ext >= GALETTES_NB) { nb_ext = 0; yext += OrgaDyTour; }
		}
		ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_DETEC,(void *)(IntAdrs)i,&k,ORGA_MAXOBJ);
#ifdef AVEC_BMP
		fig = FigureCreate(FIG_ICONE,(void *)OrgaIcnDetec,(void *)ref,OrgaEspaceDetecClique); FigureTexte(fig,Bolo[bolo].nom);
#else
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneRepartOK),(void *)ref,OrgaEspaceDetecClique); FigureTexte(fig,Bolo[bolo].nom);
#endif
		BoardAddFigure(bEspaceDetec,fig,xdet + OrgaZoneSlot.larg + 5,ydet+1,0);
		// strcpy(slot,CablageEncodePosition(Bolo[bolo].pos));
		// fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSlot),0,0); FigureTexte(fig,slot);
		// BoardAddFigure(bEspaceDetec,fig,xdet + (OrgaZoneRepartOK.larg + 4),ydet-1,0);
		// ydet += (OrgaZoneRepartOK.haut + ICONE_MARGE);
		OrgaLiaisonTrace(&LiaisonsCablage);
		if(OrgaTraitSepareMag.dy < ydet) OrgaTraitSepareMag.dy = ydet;
	}

	y = yext + (2 * OrgaDyTour);
	BoardAddBouton(bEspaceDetec,"Enregistrer",OrgaDetecEnregistre,x,y,0); y += Dy;
	if(OrgaTraitSepareMag.dy < y) OrgaTraitSepareMag.dy = y;
	fig = FigureCreate(FIG_DROITE,(void *)(&OrgaTraitSepareMag),0,0);
	BoardAddFigure(bEspaceDetec,fig,OrgaFinUsine,Dy,0);
	fig = FigureCreate(FIG_DROITE,(void *)(&OrgaTraitSepareMag),0,0);
	BoardAddFigure(bEspaceDetec,fig,OrgaFinEtagere,Dy,0);

	BoardOnClic(bEspaceDetec,OrgaCliquePlanche,0);
	OpiumFork(bEspaceDetec);
	return(0);
}
/* ========================================================================== */
static int OrgaCablageChoixAdc(Figure fig, WndAction *e) {
	TypeCapteur *captr; TypeDetecteur *detectr; int cap,autre,bb,adc,prec;
	char explic[256];

	captr = (TypeCapteur *)(fig->adrs);
	// printf(". Demande de changement d'ADC du numeriseur #%d\n",captr->bb.num);
	if((bb = captr->bb.num) < 0) return(0);
	adc = prec = captr->bb.adc;
	if(OpiumReadInt("ADC a utiliser",&adc) == PNL_CANCEL) return(0);
	if((adc <= 0) || (adc > Numeriseur[bb].nbadc)) {
		OpiumFail("Numero illegal (maxi: %d)",Numeriseur[bb].nbadc);
		return(0);
	} else if(adc == prec) return(0);
	detectr = &(Bolo[captr->bolo]); cap = captr->capt;
	for(autre=0; autre<detectr->nbcapt; autre++) if(autre != cap) {
		if(detectr->captr[autre].bb.adc == adc) {
			if(OpiumCheck("ADC%d actuellement utilise pour la voie %s. La debrancher?",adc,VoieManip[detectr->captr[autre].voie].nom,autre)) {
				DetecteurDebrancheCapteur(detectr,autre);
				CablageSortant[detectr->pos].captr[autre].vbb = 0;
				break;
			} else return(0);
		}
	}
	CablageSortant[detectr->pos].captr[cap].vbb = adc - 1;
	DetecteurDebrancheCapteur(detectr,cap);
	if(!DetecteurBrancheCapteur(detectr,cap,bb,adc-1,captr->bb.adrs,explic)) OpiumFail(explic);
	// printf("(%s) Planche pour %s %s tracee\n",__func__,detectr->nom,(OpiumDisplayed(detectr->schema.planche))?"deja":"pas encore");
	if(OpiumDisplayed(detectr->schema.planche)) OrgaDetecteurMontre(0,(MenuItem)detectr);
	if(OpiumDisplayed(bOrgaCryostat)) OrgaMagasinRefait();
	return(1);
}
/* ========================================================================== */
static int OrgaDetecteurTrace(TypeDetecteur *detectr, char croise, Cadre planche, TypeOrgaLiaisons *cablage, int x0, int y0) {
	int cap,voie,vm,bb,prec,svt,fin,bbprec; char nouveau_lien,voie_normale;
	TypeCableConnecteur connecteur,connect_prec;
	Figure fig; // TypeRepartiteur *repart;
	int xrep,xfib,xslt,xbb,xadc,xcap,xdet; int ybb,yvbb,ydet,ycap; int decal,decal_dr;
	int yadc[DETEC_CAPT_MAX];
	char titre[32],slot[8]; char *nom_voie;
	
	// printf("(%s) Demarre l'affichage de %s\n",__func__,detectr->nom);
	ybb = decal = 0;
	xrep = x0;
	xfib = xrep /* + OrgaZoneRepartOK.larg + ICONE_ETAGE */ ;
	xbb = xfib + OrgaLngrFibre + 4;
	xslt = xbb + OrgaZoneRepartOK.larg + 4;
	xadc = xslt + OrgaZoneSlot.larg + ICONE_ETAGE;
	xcap = xadc + OrgaZoneAdc.larg + ICONE_ETAGE;
	xdet = xcap + OrgaZoneCapt.larg + ICONE_ETAGE;

	yvbb = y0;
	bbprec = -1; connect_prec = CABLAGE_CONN_DIR;
	for(cap=0; cap<detectr->nbcapt; cap++) yadc[cap] = 0;
	for(cap=0; cap<detectr->nbcapt; cap++) {
		connecteur = CablageSortant[detectr->pos].captr[cap].connecteur;
		bb = detectr->captr[cap].bb.num;
		// printf("(%s) Capteur #%d: numeriseur #%d et connecteur #%d\n",__func__,cap,bb,connecteur);
		if(connecteur) nouveau_lien = (connecteur != connect_prec);
		else if(bb >= 0) nouveau_lien = (bb != bbprec);
		else continue;
		// printf("(%s) lien %s et %s\n",__func__,nouveau_lien?"nouveau":"deja vu",croise?"croise":"droit");
		if(croise) {
			for(prec=0; prec<cap; prec++) {
				if(connecteur) {
					if(CablageSortant[detectr->pos].captr[prec].connecteur == connecteur) break;
				} else if(bb >= 0) {
					if(detectr->captr[prec].bb.num == bb) break;
				}
			}
			if(prec < cap) continue;
		}
		if(nouveau_lien) {
			ybb = yvbb;
			OrgaLiaisonTrace(cablage);
			if(bb >= 0) {
				fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneFibre),(void *)&(Numeriseur[bb]),OrgaFibreSurNumeriseur); if(Numeriseur[bb].def.fibre[0]) FigureTexte(fig,Numeriseur[bb].def.fibre);
				BoardAddFigure(planche,fig,xfib-2,ybb+(OrgaZoneRepartOK.haut/2)-1,0);
				/* if((repart = Numeriseur[bb].repart)) {
					fig = FigureCreate(FIG_ZONE,(void *)&OrgaZoneRepartOK,0,0)); FigureTexte(fig,repart->nom);
					BoardAddFigure(planche,fig,xrep,ybb,0);
					decal = (Numeriseur[bb].entree - (repart->nbentrees / 2)) * ICONE_DECAL;
					OrgaLiaisonAjoute(cablage,xrep + OrgaZoneRepartOK.larg,ybb + (OrgaZoneRepartOK.haut / 2) + decal,
						xbb,ybb + (OrgaZoneRepartOK.haut / 2));
				} */
				fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneRepartOK),(void *)(&(Numeriseur[bb])),NumeriseurOrgaClique); FigureTexte(fig,Numeriseur[bb].nom);
				BoardAddFigure(planche,fig,xbb,ybb,0);
			}
			fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSlot),0,0);
			if(!ChassisNumerUnique) {
				strcpy(slot,CablageEncodeConnecteur(connecteur)); FigureTexte(fig,slot);
			}
			BoardAddFigure(planche,fig,xslt,ybb-1,0);
			if(!croise) bbprec = bb;
		}
		fin = croise? detectr->nbcapt: cap+1;
		for(svt=cap; svt<fin; svt++) {
			// printf("(%s) Ajout du lien du capteur #%d\n",__func__,svt);
			decal_dr = 0;
			if(connecteur) {
				if(CablageSortant[detectr->pos].captr[svt].connecteur != connecteur) continue;
				decal = (svt - (detectr->nbcapt / 2)) * ICONE_DECAL; decal_dr = decal;
			} else if(bb >= 0) {
				if(detectr->captr[svt].bb.num != bb) continue;
				decal = (detectr->captr[svt].bb.adc - (Numeriseur[bb].nbadc / 2)) * ICONE_DECAL;
			}
			yadc[svt] = yvbb + (OrgaZoneAdc.haut / 2);
			OrgaLiaisonAjoute(cablage,xslt + OrgaZoneSlot.larg,ybb + (OrgaZoneRepartOK.haut / 2) + decal,
				xadc,yadc[svt] + decal_dr);
			fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneAdc),(void *)(&(detectr->captr[svt])),OrgaCablageChoixAdc);
			sprintf(titre,"ADC%d",detectr->captr[svt].bb.adc); FigureTexte(fig,titre); 
			BoardAddFigure(planche,fig,xadc,yvbb,0);
			if(croise) yadc[svt] += decal;
			yvbb += (OrgaZoneRepartOK.haut + ICONE_MARGE);
		}
	}
	OrgaLiaisonTrace(cablage);

	ycap = y0; ydet = y0 - Dx + (detectr->nbcapt * (OrgaZoneVoie.haut + ICONE_MARGE)) / 2;
	for(cap=0; cap<detectr->nbcapt; cap++) {
		if(yadc[cap]) OrgaLiaisonAjoute(cablage,xadc + OrgaZoneAdc.larg,yadc[cap],xcap,ycap + (OrgaZoneCapt.haut / 2));
		else {
			fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSoft),0,0); FigureTexte(fig,"soft"); 
			BoardAddFigure(planche,fig,xadc,ycap,0);
			OrgaLiaisonAjoute(cablage,xadc + OrgaZoneSoft.larg,ycap + (OrgaZoneSoft.haut / 2),xcap,ycap + (OrgaZoneCapt.haut / 2));
		}
		voie_normale = 0;
		if((voie = detectr->captr[cap].voie) >= 0) {
			if(VoieManip[voie].pseudo) nom_voie = detectr->captr[cap].nom;
			else if((vm = VoieManip[voie].type) >= 0) {
				voie_normale = 1; nom_voie = ModeleVoie[vm].nom;
			} else nom_voie = "(bizarre)";
		} else nom_voie = "(indefinie)";
		if(voie_normale) fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneCapt),(void *)(IntAdrs)voie,ReglageClique);
		else fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneCapt),0,0);
		FigureTexte(fig,nom_voie);
		BoardAddFigure(planche,fig,xcap,ycap,0);
		ycap += (OrgaZoneVoie.haut + ICONE_MARGE);
	}
	if(croise) OrgaLiaisonCroisee(cablage,1);
	OrgaLiaisonTrace(cablage);
	if(croise) OrgaLiaisonCroisee(cablage,0);

	ycap = y0;
	for(cap=0; cap<detectr->nbcapt; cap++) {
		decal = (cap - (detectr->nbcapt / 2)) * ICONE_DECAL;
		OrgaLiaisonAjoute(cablage,xcap + OrgaZoneCapt.larg,ycap + (OrgaZoneCapt.haut / 2),
			xdet,ydet + (OrgaZoneRepartOK.haut / 2) + decal);
		ycap += (OrgaZoneVoie.haut + ICONE_MARGE);
	}
	fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSlot),0,0);
	if(!ChassisDetecUnique) {
		strcpy(slot,CablageEncodePosition(detectr->pos)); FigureTexte(fig,slot);
	}
	BoardAddFigure(planche,fig,xdet,ydet-1,0);
	fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneDetec),0,0); FigureTexte(fig,detectr->nom);
	BoardAddFigure(planche,fig,xdet+OrgaZoneSlot.larg+4,ydet,0);

	OrgaLiaisonTrace(cablage);
	// printf("(%s) Termine l'affichage de %s\n",__func__,detectr->nom);

	return(ycap - y0);
}
/* ========================================================================== */
int OrgaDetecteurMontre(Menu menu, MenuItem item) {
	TypeDetecteur *detectr;
	TypeOrgaLiaisons *liens; Cadre planche; char titre[80];

	if(menu) {
		if(OpiumReadList(L_("Detecteur"),BoloNom,&BoloNum,DETEC_NOM) == PNL_CANCEL) return(1);
		detectr = &(Bolo[BoloNum]);
	} else detectr = (TypeDetecteur *)item;
	planche = detectr->schema.planche; liens = &(detectr->schema.cablage);
	// printf("(%s) Demande l'affichage de la planche @%08X du detecteur '%s'\n",__func__,(hexa)planche,detectr->nom);
	if(planche) {
		if(OpiumDisplayed(planche)) OpiumClear(planche);
		BoardTrash(planche);
		if(detectr->schema.cablage.lien) { free(detectr->schema.cablage.lien); detectr->schema.cablage.lien = 0; }
	} else {
		planche = detectr->schema.planche = BoardCreate();
		if(!planche) return(0);
		DicoFormatte(&OpiumDico,titre,"Cablage %s",detectr->nom);
		OpiumTitle(planche,titre);
	}
	OrgaLiaisonsInit(liens,planche,(detectr->nbcapt * 4) + 1);
	OrgaDetecteurTrace(detectr,1,planche,liens,Dx,Dy);
	OpiumFork(planche);

	return(0);
}
/* .......................................................................... */
static int OrgaRepartPerimetreAucun(Menu menu, MenuItem item) {
	int rep;
	for(rep=0; rep<RepartNb; rep++) Repart[rep].local = 0;
	BoardRefreshVars(bOrgaCryostat);
	return(0);
}
/* .......................................................................... */
static int OrgaRepartPerimetreTous(Menu menu, MenuItem item) {
	int rep;
	for(rep=0; rep<RepartNb; rep++) Repart[rep].local = 1;
	BoardRefreshVars(bOrgaCryostat);
	return(0);
}
/* .......................................................................... */
static int OrgaRepartPerimetreApplique(Panel p, void *arg) {
	if(Acquis[AcquisLocale].etat.active) {
		Acquis[AcquisLocale].etat.active = 0; NumeriseurEtatDemande = 0;
	} else if(OpiumDisplayed(bNumeriseurEtat)) NumeriseurRapport(0,0);
	RepartiteurAppliquePerimetre();
	SambaModifiePerimetre();
	LectConsignesConstruit();
	OrgaMagasinRefait();
	return(0);
}
/* .......................................................................... */
static TypeMenuItem iOrgaRepartLocaux[] = {
	{ "Aucun", MNU_FONCTION OrgaRepartPerimetreAucun },
	{ "Tous",  MNU_FONCTION OrgaRepartPerimetreTous },
	MNU_END
};
/* ========================================================================== */
static void OrgaEspaceRepartition(char mode, int xmin, int ymin) {
	int rep,l,vbb,voie,i,j,n,flt,nbflt,nbcols;
	TypeRepartiteur *repart; TypeNumeriseur *numeriseur;
	Figure fig; TypeFigZone *icone; Menu m; Panel p;
	int nbactifs,nbentrees,nbnumer; int actif;
	int x[4],y[4],x0[4],y0[4],yp,decal,dxfibre,xmax,ymax,ymarge;
	char local[REPART_MAX];
	char debug;
	char avec_liens;
	
	if(SambaExploreHW && ((RepartNb > 1) || !Repart[0].local)) {
#ifdef PERIMETRE_AVEC_PLANCHE
		RepartiteurRedefiniPerimetre(0,0);
#else
		if(OpiumExec(pRepartLocal->cdr) == PNL_CANCEL);
#endif
	}

	debug = 0; avec_liens = 1;
	
	nbactifs = nbentrees = nbnumer = 0;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) {
		nbactifs++;
		repart = &(Repart[rep]);
		// if(repart->famille == FAMILLE_IPE) n = (IPE_FLT_NBUSED(repart) * IPE_FLT_SIZE); else n = repart->nbentrees;
		n = repart->maxentrees;
		nbentrees += n; nbnumer += n;
		for(l=0; l<repart->nbentrees; l++) if((numeriseur = (TypeNumeriseur *)repart->entree[l].adrs)) {
			if(numeriseur->nbadc) nbnumer += numeriseur->nbadc;
		}
	}
	if(avec_liens) {
		OrgaLiaisonsInit(&LiaisonsRepartNumer,bOrgaCryostat,nbnumer);
		if(nbentrees > OrgaNbEntrees) {
			if(OrgaRefEntree) free(OrgaRefEntree);
			OrgaRefEntree = (OrgaRepEntree)malloc(nbentrees * sizeof(TypeOrgaRepEntree));
			if(OrgaRefEntree) OrgaNbEntrees = nbentrees; else OrgaNbEntrees = 0;
		}
		OrgaLiaisonsInit(&LiaisonsDaqRepart,bOrgaCryostat,nbactifs);
	}

	ymarge = (2 * Dy);
	x[0] = xmin + Dx; y[0] = ymin + ymarge;
	y[0] += (ICONE_MARGE + (nbactifs * OrgaZoneRepartOK.haut)) / 2;
	xmax = 0; ymax = 0;

#ifdef REMPLACABLE_PAR_MODELES
	// if(OrgaMode =  ORGA_MAG_ORDI) 
	fig = FigureCreate(FIG_ICONE,(void *)(&OrgaIconeMacMini),0,OrgaCliqueSurMac);
	FigureTexte(fig,NomOrdiLocal);
	BoardAddFigure(bOrgaCryostat,fig,x[0],y[0],0);
	x0[0] = x[0] + OrgaIconeMacMini.larg; y0[0] = y[0] + (OrgaIconeMacMini.haut / 2); // position depart liens sur repartiteurs
	y[0] += ((2 * ICONE_MARGE) + OrgaIconeMacMini.haut);
	x[1] = x[0] + OrgaIconeMacMini.larg + ICONE_ETAGE; y[1] = ymin + ymarge;           // position icone courante repartiteur
#endif
	x0[0] = x[0]; y0[0] = y[0]; // position depart liens sur repartiteurs
	y[0] += ((2 * ICONE_MARGE));
	x[1] = x[0] + ICONE_ETAGE; y[1] = ymin + ymarge;           // position icone courante repartiteur	
	
//	x[1] = Dx; y[1] = ymin + ymarge; // position icone courante repartiteur
// if(OrgaMagMode == ORGA_MAG_ORDI) 
	if((RepartNb > 1) || !Repart[0].local) {
//		RepartPerimetreReponse = 0;
		for(rep=0; rep<RepartNb; rep++) local[rep] = Repart[rep].local;
		MenuColumns((m = MenuLocalise(iOrgaRepartLocaux)),2); OpiumSupport(m->cdr,WND_PLAQUETTE);
		BoardAddMenu(bOrgaCryostat,m,x[0],y[0],0); yp = y[0] + (2 * Dy);
		nbcols = ((RepartNb - 1) / 50) + 1;
		PanelColumns((p = PanelCreate(RepartNb)),nbcols);
		PanelSupport(p,WND_CREUX);
		for(rep=0; rep<RepartNb; rep++) {
			i = PanelBool(p,Repart[rep].nom,&(Repart[rep].local));
			PanelItemColors(p,i,SambaRougeVert,2);
		}
		PanelOnApply(p,OrgaRepartPerimetreApplique,0);
		BoardAddPanel(bOrgaCryostat,p,x[0],yp,0);
		x[1] += (nbcols * ((REPART_NOM + 4) * Dx));
		yp += ((RepartNb + 2) * Dy);
		if(ymax < yp) ymax = yp;
	}
	
	x[2] = x[1] + OrgaZoneRepartOK.larg + OrgaZoneFibre.larg + 10; y[2] = y[1]; // position icone courante numeriseur
	actif = 0; j = 0;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) {
		repart = &(Repart[rep]);
		if(OrgaMagMode == ORGA_MAG_ORDI) {
			decal = (actif - (nbactifs / 2)) * ICONE_DECAL; actif++;
			if(avec_liens) OrgaLiaisonAjoute(&LiaisonsDaqRepart,x0[0],y0[0]+decal,x[1],y[1] + (OrgaZoneRepartOK.haut / 2));
		}
		if(repart->famille == FAMILLE_IPE) {
			int dxflt,yflt;
			n = IPE_FLT_NBUSED(repart);
			nbflt = 0; for(flt=0; flt<n; flt++) if(repart->r.ipe.flt[flt].branchee) nbflt++;
			if(!nbflt) {
				memcpy(&(OrgaZoneRepart[rep]),&OrgaZoneRepartHS,sizeof(TypeFigZone));
				fig = FigureCreate(FIG_ZONE,(void *)(&(OrgaZoneRepart[rep])),(void *)repart,RepartiteursClique);
				FigureTexte(fig,repart->nom);
				BoardAddFigure(bOrgaCryostat,fig,x[1],y[1],0);
				x0[1] = x[1] + OrgaZoneRepart[rep].larg; y0[1] = y[1] + (ICONE_HAUTEUR / 2) + 3; // position depart lien sur numeriseurs
				if(ymax < (y[1] + OrgaZoneRepart[rep].haut)) ymax = y[1] + OrgaZoneRepart[rep].haut;
			} else {
				memcpy(&(OrgaZoneRepart[rep]),&OrgaZoneIpe,sizeof(TypeFigZone));
				OrgaZoneRepart[rep].haut = (nbflt * OrgaZoneFLT.haut) + ((nbflt - 1) * (2 * ICONE_MARGE)) + 10;
				fig = FigureCreate(FIG_ZONE,(void *)&OrgaZoneRepart[rep],(void *)repart,RepartiteursClique);
				BoardAddFigure(bOrgaCryostat,fig,x[1],y[1],0);
				dxflt = (OrgaZoneRepartOK.larg - OrgaZoneFLT.larg) / 2;
				yflt = y[1] + 5;
				x0[1] = x[1] + dxflt + OrgaZoneFLT.larg + 5; y0[1] = yflt + (ICONE_HAUTEUR / 2); // position depart lien sur numeriseurs
				if(ymax < (y[1] + OrgaZoneRepart[rep].haut)) ymax = y[1] + OrgaZoneRepart[rep].haut;
				y[2] = yflt;
				nbflt = 0;
				for(flt=0; flt<n; flt++) if(repart->r.ipe.flt[flt].branchee) {
					fig = FigureCreate(FIG_ZONE,(void *)&OrgaZoneFLT,(void *)(&(repart->r.ipe.flt[flt])),OrgaCliqueSurFlt);
					FigureTexte(fig,&(IpeFltNom[IPE_FLT_ABS(repart,flt)][0]));
					BoardAddFigure(bOrgaCryostat,fig,x[1]+dxflt,yflt,0);
					if(ymax < (yflt + OrgaZoneFLT.haut)) ymax = yflt + OrgaZoneFLT.haut;
					yflt += (OrgaZoneFLT.haut + (2 * ICONE_MARGE));
					nbflt++;
				}
			}
		} else {
			memcpy(&(OrgaZoneRepart[rep]),(repart->valide)?&OrgaZoneRepartOK:&OrgaZoneRepartHS,sizeof(TypeFigZone));
			OrgaZoneRepart[rep].haut = (repart->maxentrees * OrgaZoneNumer.haut) + ((repart->maxentrees - 1) * ICONE_MARGE);
			fig = FigureCreate(FIG_ZONE,(void *)(&(OrgaZoneRepart[rep])),(void *)repart,RepartiteursClique);
			FigureTexte(fig,repart->nom);
			BoardAddFigure(bOrgaCryostat,fig,x[1],y[1],0);
			x0[1] = x[1] + OrgaZoneRepart[rep].larg + 4; y0[1] = y[1] + (OrgaZoneRepart[rep].haut / 2); // position depart lien sur numeriseurs
			if(ymax < (y[1] + OrgaZoneRepart[rep].haut)) ymax = y[1] + OrgaZoneRepart[rep].haut;
		}
		
		for(l=0; l<repart->maxentrees; l++) {
			if((l < repart->nbentrees) && (numeriseur = (TypeNumeriseur *)repart->entree[l].adrs)) {
				if(numeriseur->fischer) icone = &OrgaZoneNumer;
				else {
					for(vbb=0; vbb<numeriseur->nbadc; vbb++) if(numeriseur->voie[vbb] >= 0) break;
					if(vbb < numeriseur->nbadc) icone = &OrgaZoneNumer;
					else icone = &OrgaZoneNumerHS;
				}
			} else { icone = &OrgaZoneNumerHS; numeriseur = 0; }
			if(repart->famille == FAMILLE_IPE) {
				if(!(repart->r.ipe.flt[IPE_FLT_NUM(l)].branchee)) continue;
				y0[1] = y[2] + (OrgaZoneNumer.haut / 2); decal = 0;
			} else decal = (l - (repart->nbentrees / 2)) * ICONE_DECAL;
#ifdef FIBRE_DANS_DONNEES
			dxfibre = OrgaZoneFibre.larg + 6;
#else
			dxfibre = OrgaTraitArrivee.dx;
#endif
			dxfibre = 0;
			if(j < OrgaNbEntrees) {
				OrgaRefEntree[j].repart = repart; OrgaRefEntree[j].l = l;
				fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneFibre),(void *)(&(OrgaRefEntree[j])),OrgaFibreSurRepartiteur);
				j++;
			} else fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneFibre),0,0);
			if(repart->entree[l].fibre[0]) FigureTexte(fig,repart->entree[l].fibre);
			BoardAddFigure(bOrgaCryostat,fig,x0[1],y[2]+(OrgaZoneNumer.haut/2)-(OrgaZoneFibre.haut/2)+1,0); // y0[1]-(OrgaZoneFibre.haut/2)+1
			fig = FigureCreate(FIG_ZONE,(void *)icone,(void *)numeriseur,NumeriseurOrgaClique);
			if(numeriseur) FigureTexte(fig,numeriseur->nom); else FigureTexte(fig,"(ignore)");
			BoardAddFigure(bOrgaCryostat,fig,x[2]+dxfibre,y[2],0);
			if(numeriseur && (icone == &OrgaZoneNumer)) {
				x0[2] = x[2] + OrgaZoneNumer.larg + dxfibre; y0[2] = y[2] + (OrgaZoneNumer.haut / 2); // position depart liens sur voies
				x[3] = x0[2] + 4; y[3] = y[2]; // position icone courante voie
				// if(numeriseur->fischer) {
				fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSlot),(void *)numeriseur,OrgaCliqueSurSlot);
				if(!ChassisNumerUnique) FigureTexte(fig,CablageEncodeConnecteur(numeriseur->fischer));
				BoardAddFigure(bOrgaCryostat,fig,x[3],y[3]-1,0);
				x0[2] = x[3] + OrgaZoneSlot.larg;
				// }
				x[3] += (OrgaZoneSlot.larg + ICONE_INTERVALLE);
				if(OrgaRepartAfficheCryo) {
#ifdef AFFICHE_CRYO
					TypeCablage *cablage; TypeModeleCable *modele_cable; // TypeDetModele *modele_det; TypeDetecteur *detectr;
					int capteur;
					for(pos_hexa=0; pos_hexa<CABLAGE_POS_MAX; pos_hexa++) {
						cablage = &(CablageSortant[pos_hexa]);
						for(capteur=0; capteur<cablage->nbcapt; capteur++) {
							if(cablage->captr[capteur].connecteur == numeriseur->fischer) break;
						}
						if(capteur < cablage->nbcapt) {
							x = xslt + OrgaFigConnec[OrgaStyle].larg - 2; if(OrgaStyle == ORGA_ICONES)  x += ((lconn + 1) * Dx);
							decal = (rang_bb - (cablage->nb_fischer / 2)) * ICONE_DECAL;
						#ifdef AVEC_ICONE_CONNEC
							OrgaLiaisonAjoute(liens,x,yvbb + (OrgaFigConnec[OrgaStyle].haut / 2) + decal,
											  xadc,ydet + (OrgaFigCable[OrgaStyle].haut / 2) + decal);
						#else
							OrgaLiaisonAjoute(liens,xslt + OrgaZoneSlot.larg,yvbb + (OrgaZoneRepartOK.haut / 2) + decal,
											  xadc,ydet + (OrgaZoneNumer.haut / 2) + decal);
						#endif
							if(cap == 0) {
								ref = OrgaNouvelleRef(OrgaObjet,ORGA_REF_CABLE,(void *)((int)pos_hexa),k,ORGA_MAXOBJ);
							#ifdef AVEC_ICONE
								fig = FigureCreate(OrgFigType[OrgaStyle],OrgaFigCable[OrgaStyle].modl,(void *)ref,OrgaMagasinClique);
							#else
								fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneNumer),(void *)ref,OrgaMagasinClique);
							#endif
								FigureTexte(fig,modele_cable->nom); 
								BoardAddFigure(planche,fig,xadc,ydet,0);
							}
						}
					}
#endif
				} else {
					for(vbb=0; vbb<numeriseur->nbadc; vbb++) {
						if(avec_liens) OrgaLiaisonAjoute(&LiaisonsRepartNumer,x0[2],y0[2],x[3],y0[2]);
						voie = numeriseur->voie[vbb];
						if(voie >= 0) {
							fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneVoie),(void *)(IntAdrs)voie,ReglageClique);
							FigureTexte(fig,VoieManip[voie].nom);
						} else {
							fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneInactive),0,0);
							FigureTexte(fig,"(ignoree)");
						}
						BoardAddFigure(bOrgaCryostat,fig,x[3],y[3],0);
						x0[2] = x[3] + OrgaZoneVoie.larg;
						x[3] += (OrgaZoneVoie.larg + ICONE_INTERVALLE);
					}
				}
				if(xmax < x0[2]) xmax = x0[2];
				if(ymax < (y[3] + OrgaZoneVoie.haut)) ymax = y[3] + OrgaZoneVoie.haut;
			}
			y[2] += (OrgaZoneNumer.haut + ICONE_MARGE);
			if((repart->famille == FAMILLE_IPE) && !((l + 1) % IPE_FLT_SIZE)) {
				// BoardAddFigure(bOrgaCryostat,FigureCreate(FIG_LIEN,(void *)&OrgaTraitSepareFLT,0,0),x[2],y[2],0);
				y[2] += ICONE_MARGE;
			}
		}
		y[1] += (OrgaZoneRepart[rep].haut + ICONE_MARGE);
		if(y[1] < y[2]) y[1] = y[2];
		if(avec_liens) OrgaLiaisonTrace(&LiaisonsRepartNumer);
	}
	if(avec_liens) OrgaLiaisonTrace(&LiaisonsDaqRepart);

	OrgaTraitSepareIcn.dx = xmax;
	fig = FigureCreate(FIG_DROITE,(void *)(&OrgaTraitSepareIcn),0,0);
	BoardAddFigure(bOrgaCryostat,fig,0,ymin,0);
	ymax += (OrgaZoneVoie.haut + ICONE_MARGE);
	if(ymax < (OrgaTraitSepareMag.dy + ymin)) ymax = (OrgaTraitSepareMag.dy + ymin);
	fig = FigureCreate(FIG_DROITE,(void *)(&OrgaTraitSepareIcn),0,0);
	BoardAddFigure(bOrgaCryostat,fig,0,ymax,0);
	OrgaTraitSepareMag.dy = ymax - ymin;
	if(OrgaFinUsine > ICONE_INTERVALLE) {
		fig = FigureCreate(FIG_DROITE,(void *)(&OrgaTraitSepareMag),0,0);
		BoardAddFigure(bOrgaCryostat,fig,OrgaFinUsine-ICONE_INTERVALLE,ymin,0);
	}
	BoardAddBouton(bOrgaCryostat,"Enregistrer",OrgaMagasinEnregistre,OrgaFinUsine+Dx,ymax+Dy,0);
}
#pragma mark --------- Organigrammes ----------
/* ========================================================================== */
/*  Organigrammes                                                             */
/* ========================================================================== */
static int OrgaCliqueSurMenu(Figure fig, WndAction *e) {
	Menu menu;
	
	menu = (Menu)(fig->adrs);
	OpiumFork(menu->cdr);
	return(0);
}
/* ========================================================================== */
static int OrgaCliqueSurSlot(Figure cliquee, WndAction *e) {
	TypeNumeriseur *numeriseur; TypeDetecteur *detectr; int vbb,voie,bolo;
	TypeOrgaLiaisons *cablage; Cadre planche;
	char titre[80];
	
	// printf("(%s) Appel avec numeriseur @%08X\n",__func__,(hexa)(cliquee->adrs));
	if((numeriseur = (TypeNumeriseur *)(cliquee->adrs))) {
		for(vbb=0; vbb<numeriseur->nbadc; vbb++) if((voie = numeriseur->voie[vbb]) >= 0) {
			if((bolo = VoieManip[voie].det) >= 0) {
				detectr = &(Bolo[bolo]);
				planche = detectr->schema.planche; cablage = &(detectr->schema.cablage);
				// printf("(%s) Demande l'affichage de la planche @%08X du detecteur '%s'\n",__func__,(hexa)planche,detectr->nom);
				if(planche) {
					if(OpiumDisplayed(planche)) OpiumClear(planche);
					BoardTrash(planche);
					if(detectr->schema.cablage.lien) { free(detectr->schema.cablage.lien); detectr->schema.cablage.lien = 0; }
				} else {
					planche = detectr->schema.planche = BoardCreate();
					if(!planche) return(0);
					sprintf(titre,"Cablage %s",detectr->nom);
					OpiumTitle(planche,titre);
				}
				OrgaLiaisonsInit(cablage,planche,(detectr->nbcapt * 4) + 1);
				OrgaDetecteurTrace(detectr,(e->code == WND_MSELEFT),planche,cablage,Dx,Dy);
				OpiumFork(planche);
				break;
			}
		}
	}
	
	return(0);
}
/* ========================================================================== */
static int OrgaCliqueSurFlt(Figure fig, WndAction *e) {
	TypeIpeFlt *flt; TypeRepartiteur *repart;
	int l;
	
	flt = (TypeIpeFlt *)(fig->adrs);
	if(!flt) return(0);
	if(!(repart = (TypeRepartiteur *)(flt->repart))) return(0);
	l = flt->l0 + (e->y / (OrgaZoneNumer.haut + ICONE_MARGE));
	printf("* Clique dans une FLT, pixel %d/%d soit fibre=%d+%d/%d\n",e->y,OrgaZoneNumer.haut,flt->l0,e->y / (OrgaZoneNumer.haut + ICONE_MARGE),repart->nbentrees);
	if(l < repart->nbentrees) NumeriseurActionSouris((TypeNumeriseur *)repart->entree[l].adrs,e);
	return(0);
}
/* ========================================================================== */
static void OrgaRafraichiDonnees() {
	int rep,l,vbb,voie,n,flt,nbflt;
	TypeRepartiteur *repart; TypeNumeriseur *numeriseur;
	Figure fig; TypeFigZone *icone; Menu m;
	int nbactifs,nbnumer; int actif;
	int x[4],y[4],x0[4],y0[4],ydep,ymin,decal,dxfibre;
	char debug;
#define AVEC_ICONE_MAC
	
	if(SambaExploreHW && ((RepartNb > 1) || !Repart[0].local)) {
	#ifdef PERIMETRE_AVEC_PLANCHE
		RepartiteurRedefiniPerimetre(0,0);
	#else
		if(OpiumExec(pRepartLocal->cdr) == PNL_CANCEL);
	#endif
	}

	debug = 0;
	nbactifs = 0; nbnumer = 0;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) {
		nbactifs++;
		repart = &(Repart[rep]);
		if(repart->famille == FAMILLE_IPE) nbnumer += (IPE_FLT_NBUSED(repart) * IPE_FLT_SIZE);
		else nbnumer += repart->nbentrees;
		for(l=0; l<repart->nbentrees; l++) if((numeriseur = (TypeNumeriseur *)repart->entree[l].adrs)) {
			if(numeriseur->nbadc) nbnumer += numeriseur->nbadc;
		}
	}
	OrgaLiaisonsInit(&LiaisonsDaqRepart,bSchemaManip,nbactifs);
	OrgaLiaisonsInit(&LiaisonsRepartNumer,bSchemaManip,nbnumer);

	ydep = WndLigToPix(3);
	x[0] = ICONE_MARGE; y[0] = ydep; // position coin HG icone "DAQ"
	// fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneMenu),(void *)mSambaProcs,OrgaCliqueSurMenu); FigureTexte(fig,"Procedures");
	// BoardAddFigure(bSchemaManip,fig,x[0],y[0],0);
	// y[0] += (OrgaZoneMenu.haut + ICONE_MARGE);
	m = MenuLocalise(iOrgaProcs); OpiumSupport(m->cdr,WND_PLAQUETTE);
	BoardAddMenu(bSchemaManip,m,x[0],y[0],0);
	OrgaZoneMenu.larg = MenuLargeurPix(m);
	y[0] += ((MenuItemNb(m) * Dy) + ICONE_MARGE);
	fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneMenu),(void *)mMonitBarre,OrgaCliqueSurMenu); FigureTexte(fig,"Graphiques");
	BoardAddFigure(bSchemaManip,fig,x[0],y[0],0);

	ymin = y[0] + OrgaZoneMenu.haut + (2 * ICONE_MARGE);
	y[0] = (ICONE_MARGE + (nbactifs * OrgaZoneRepartOK.haut)) / 2;
	if(y[0] < ymin) y[0] = ymin;
	OrgaZoneMac.larg = OrgaZoneMenu.larg;
#ifdef AVEC_BMP
	fig = FigureCreate(FIG_ICONE,(void *)OrgaIcnEdw,0,OrgaCliqueSurMac); x[0] += (OrgaZoneMac.larg -  OrgaIcnEdw->larg);
#else
	#ifdef AVEC_ICONE_MAC
		fig = FigureCreate(FIG_ICONE,(void *)(&OrgaIconeMacMini),0,OrgaCliqueSurMac); x[0] += (OrgaZoneMac.larg - OrgaIconeMacMini.larg);
	#else
		fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneMac),0,OrgaCliqueSurMac);
	#endif
#endif
	FigureTexte(fig,NomOrdiLocal);
	BoardAddFigure(bSchemaManip,fig,x[0],y[0],0);
#ifdef AVEC_BMP
	x0[0] = x[0] + OrgaIcnEdw.larg; y0[0] = y[0] + (OrgaIcnEdw.haut / 2); // position depart liens sur repartiteurs
	y[0] += ((2 * ICONE_MARGE) + OrgaIcnEdw.haut);
	x[1] = x[0] + OrgaIcnEdw->larg + ICONE_ETAGE; y[1] = ydep;            // position icone courante repartiteur
#else
	#ifdef AVEC_ICONE_MAC
		x0[0] = x[0] + OrgaIconeMacMini.larg; y0[0] = y[0] + (OrgaIconeMacMini.haut / 2); // position depart liens sur repartiteurs
		y[0] += ((2 * ICONE_MARGE) + OrgaIconeMacMini.haut);
		x[1] = x[0] + OrgaIconeMacMini.larg + ICONE_ETAGE; y[1] = ydep;           // position icone courante repartiteur
	#else
		x0[0] = x[0] + OrgaZoneMac.larg; y0[0] = y[0] + (OrgaZoneMac.haut / 2); // position depart liens sur repartiteurs
		y[0] += ((2 * ICONE_MARGE) + OrgaZoneMac.haut);
		x[1] = x[0] + OrgaZoneMac.larg + ICONE_ETAGE; y[1] = ydep;              // position icone courante repartiteur
	#endif
#endif

	x[2] = x[1] + OrgaZoneRepartOK.larg + ICONE_ETAGE; y[2] = ydep; // position icone courante numeriseur
	actif = 0;
	for(rep=0; rep<RepartNb; rep++) if(Repart[rep].local) {
		repart = &(Repart[rep]);
		decal = (actif - (nbactifs / 2)) * ICONE_DECAL; actif++;
		OrgaLiaisonAjoute(&LiaisonsDaqRepart,x0[0],y0[0]+decal,x[1],y[1] + (OrgaZoneRepartOK.haut / 2));
		if(repart->famille == FAMILLE_IPE) {
			int dxflt,yflt;
			n = IPE_FLT_NBUSED(repart);
			nbflt = 0; for(flt=0; flt<n; flt++) if(repart->r.ipe.flt[flt].branchee) nbflt++;
			if(!nbflt) {
				fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneRepartHS),(void *)repart,RepartiteursClique);
				FigureTexte(fig,repart->nom);
				BoardAddFigure(bSchemaManip,fig,x[1],y[1],0);
				x0[1] = x[1] + OrgaZoneRepartHS.larg; y0[1] = y[1] + (ICONE_HAUTEUR / 2); // position depart lien sur numeriseurs
			} else {
				OrgaZoneIpe.haut = (nbflt * OrgaZoneFLT.haut) + ((nbflt - 1) * (2 * ICONE_MARGE)) + 10;
				fig = FigureCreate(FIG_ZONE,(void *)&OrgaZoneIpe,(void *)repart,RepartiteursClique);
				BoardAddFigure(bSchemaManip,fig,x[1],y[1],0);
				dxflt = (OrgaZoneRepartOK.larg - OrgaZoneFLT.larg) / 2;
				yflt = y[1] + 5;
				nbflt = 0;
				for(flt=0; flt<n; flt++) if(repart->r.ipe.flt[flt].branchee) {
					fig = FigureCreate(FIG_ZONE,(void *)&OrgaZoneFLT,(void *)(&(repart->r.ipe.flt[flt])),OrgaCliqueSurFlt);
					FigureTexte(fig,&(IpeFltNom[IPE_FLT_ABS(repart,flt)][0]));
					BoardAddFigure(bSchemaManip,fig,x[1]+dxflt,yflt,0);
					yflt += (OrgaZoneFLT.haut + (2 * ICONE_MARGE));
					nbflt++;
				}
				x0[1] = x[1] + dxflt + OrgaZoneFLT.larg; y0[1] = y[1] + (ICONE_HAUTEUR / 2); // position depart lien sur numeriseurs
			}
		} else {
			if(repart->valide) fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneRepartOK),(void *)repart,RepartiteursClique);
			else fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneRepartHS),(void *)repart,RepartiteursClique);
			FigureTexte(fig,repart->nom);
			BoardAddFigure(bSchemaManip,fig,x[1],y[1],0);
			x0[1] = x[1] + OrgaZoneRepartOK.larg; y0[1] = y[1] + (OrgaZoneRepartOK.haut / 2); // position depart lien sur numeriseurs
		}
		
		for(l=0; l<repart->nbentrees; l++) {
			if((numeriseur = (TypeNumeriseur *)repart->entree[l].adrs)) {
				if(numeriseur->fischer) icone = &OrgaZoneNumer;
				else {
					for(vbb=0; vbb<numeriseur->nbadc; vbb++) if(numeriseur->voie[vbb] >= 0) break;
					if(vbb < numeriseur->nbadc) icone = &OrgaZoneNumer;
					else icone = &OrgaZoneNumerHS;
				}
			} else icone = &OrgaZoneNumerHS;
			if(repart->famille == FAMILLE_IPE) {
				if(!(repart->r.ipe.flt[IPE_FLT_NUM(l)].branchee)) continue;
				y0[1] = y[2] + (OrgaZoneNumer.haut / 2); decal = 0;
			} else decal = (l - (repart->nbentrees / 2)) * ICONE_DECAL;
#ifdef FIBRE_DANS_DONNEES
			dxfibre = OrgaZoneFibre.larg + 6;
#else
			dxfibre = OrgaTraitArrivee.dx;
#endif
			if(icone == &OrgaZoneNumer)
				OrgaLiaisonAjoute(&LiaisonsRepartNumer,x0[1],y0[1]+decal,x[2]-2,y[2] + (OrgaZoneNumer.haut / 2));
			else OrgaLiaisonAjoute(&LiaisonsRepartNumer,x0[1],y0[1]+decal,x[2]+dxfibre-2,y[2] + (OrgaZoneNumer.haut / 2));
			fig = FigureCreate(FIG_ZONE,(void *)icone,(void *)numeriseur,NumeriseurOrgaClique);
			if(numeriseur) FigureTexte(fig,numeriseur->nom); else FigureTexte(fig,"(ignore)");
			BoardAddFigure(bSchemaManip,fig,x[2]+dxfibre,y[2],0);
			if(numeriseur && (icone == &OrgaZoneNumer)) {
#ifdef FIBRE_DANS_DONNEES
				fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneFibre),0,0);
#else
				fig = FigureCreate(FIG_DROITE,(void *)(&OrgaTraitArrivee),0,0);
#endif
				if(numeriseur->def.fibre[0]) FigureTexte(fig,numeriseur->def.fibre);
#ifdef FIBRE_DANS_DONNEES
				BoardAddFigure(bSchemaManip,fig,x[2],y[2]+4,0);
#else
				BoardAddFigure(bSchemaManip,fig,x[2]-3,y[2] + (OrgaZoneNumer.haut / 2),0);
#endif
				x0[2] = x[2] + OrgaZoneNumer.larg + dxfibre; y0[2] = y[2] + (OrgaZoneNumer.haut / 2); // position depart liens sur voies
				x[3] = x0[2] + 4; y[3] = y[2]; // position icone courante voie
				// if(numeriseur->fischer) {
					fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneSlot),(void *)numeriseur,OrgaCliqueSurSlot);
					if(!ChassisNumerUnique) FigureTexte(fig,CablageEncodeConnecteur(numeriseur->fischer));
					BoardAddFigure(bSchemaManip,fig,x[3],y[3]-1,0);
					x0[2] = x[3] + OrgaZoneSlot.larg;
				// }
				x[3] += (OrgaZoneSlot.larg + ICONE_INTERVALLE);
				for(vbb=0; vbb<numeriseur->nbadc; vbb++) {
					OrgaLiaisonAjoute(&LiaisonsRepartNumer,x0[2],y0[2],x[3],y0[2]);
					voie = numeriseur->voie[vbb];
					if(voie >= 0) {
						fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneVoie),(void *)(IntAdrs)voie,ReglageClique);
						FigureTexte(fig,VoieManip[voie].nom);
					} else {
						fig = FigureCreate(FIG_ZONE,(void *)(&OrgaZoneInactive),0,0);
						FigureTexte(fig,"(ignoree)");
					}
					BoardAddFigure(bSchemaManip,fig,x[3],y[3],0);
					x0[2] = x[3] + OrgaZoneVoie.larg;
					x[3] += (OrgaZoneVoie.larg + ICONE_INTERVALLE);
				}
			}
			y[2] += (OrgaZoneNumer.haut + ICONE_MARGE);
			if((repart->famille == FAMILLE_IPE) && !((l + 1) % IPE_FLT_SIZE)) {
				// BoardAddFigure(bSchemaManip,FigureCreate(FIG_LIEN,(void *)&OrgaTraitSepareFLT,0,0),x[2],y[2],0);
				y[2] += ICONE_MARGE;
			}
		}
		y[1] += (OrgaZoneRepartOK.haut + ICONE_MARGE);
		if(y[1] < y[2]) y[1] = y[2];
		OrgaLiaisonTrace(&LiaisonsRepartNumer);
	}
	OrgaLiaisonTrace(&LiaisonsDaqRepart);
	y[0] += (OrgaZoneMac.haut + ICONE_MARGE);
}
/* ========================================================================== */
static void OrgaRafraichiCablage() {
	int x,y,v; int bolo,max;
	
	max = 0;
	for(bolo=0; bolo<BoloNb; bolo++) max += ((Bolo[bolo].nbcapt * 4) + 1);
	OrgaLiaisonsInit(&LiaisonsCablage,bSchemaManip,max);
	
	x = Dx; y = 3 * Dy; v = 0;
	for(bolo=0; bolo<BoloNb; bolo++) {
		if((y + (2 * v)) > OpiumServerHeigth(0)) {
			x += OrgaDetecLargeur; y = 2 * Dy;
		}
		// printf("(%s) Demande l'affichage de la planche @%08X du detecteur '%s'\n",__func__,(hexa)bSchemaManip,Bolo[bolo].nom);
		v = OrgaDetecteurTrace(&(Bolo[bolo]),1,bSchemaManip,&LiaisonsCablage,x,y);
		y += v;
	}	
	return;
}
/* ========================================================================== */
static void OrgaRafraichi(int numero) {
	
	if(bSchemaManip) {
		if(OpiumDisplayed(bSchemaManip)) OpiumClear(bSchemaManip);
		BoardTrash(bSchemaManip);
		if(LiaisonsCablage.lien) { free(LiaisonsCablage.lien); LiaisonsCablage.lien = 0; }
	} else bSchemaManip = BoardCreate();
	if(!bSchemaManip) return;
	
	OrgaFeuilles = OngletDefini(OrgaFeuillesNom,OrgaRafraichi);
	OngletDemande(OrgaFeuilles,numero);
	BoardAddOnglet(bSchemaManip,OrgaFeuilles,0);
	switch(numero) {
		case 0: OrgaRafraichiDonnees(); break;
		case 1: OrgaRafraichiCablage(); break;
	}
	
	OpiumFork(bSchemaManip);
}
/* ========================================================================== */
int OrgaTraceTout(Menu menu, MenuItem item) {
	OrgaRafraichi(0);
	return(0);
}
#pragma mark --------- Import/Export ----------
/* ========================================================================== */
/*  Import/Export                                                             */
/* ========================================================================== */
int OrgaImporte(Menu menu, MenuItem item) {
	char nom_partiel[MAXFILENAME],nom_complet[MAXFILENAME];
	FILE *f;
	int lig,fait,ajout_repart,ajout_numer,nb_erreurs; 
	char adrs_ip[16],txt_erreur[80]; int rep,l,bb,num,modl;
	char sauve,opera_ajoutee;
	TypeCableConnecteur connecteur;
	char ligne[256],*r,*s,*code_entree,*code_rep,*nom_bb,*type_bb;
	
	strcpy(nom_partiel,"OrgaImport.csv");
	if(OpiumReadText("depuis le fichier",nom_partiel,MAXFILENAME) == PNL_CANCEL) return(0);
	SambaAjoutePrefPath(nom_complet,nom_partiel);
	if((f = fopen(nom_complet,"r"))) {
		printf("* Importation du setup depuis '%s'\n",nom_complet);
		ajout_repart = ajout_numer = 0;
		lig = 0; nb_erreurs = 0;
		while(LigneSuivante(ligne,256,f)) {
			lig++;
			if((ligne[0] == '#') || (ligne[0] == '\n')) continue;
			r = ligne;
			/*  repartiteur.entree */
			code_entree = ItemJusquA(';',&r);
			if(*code_entree == '\0') continue;
			s = code_entree; code_rep = ItemJusquA('.',&s);
			if(*s) { sscanf(ItemJusquA(' ',&s),"%d",&l); --l; } else l = 0;
			if(!strncmp(code_rep,"sc",2) && (CluzelLue >= 0)) rep = CluzelLue;
			else for(rep=0; rep<RepartNb; rep++) if(!strcmp(code_rep,Repart[rep].code_rep)) break;
			/* numeriseur */
			nom_bb = ItemJusquA(';',&r);
			for(bb=0; bb<NumeriseurNb; bb++) if(!strcmp(nom_bb,Numeriseur[bb].nom)) break;
			/* connecteur */
			connecteur = CablageDecodeConnecteur(ItemJusquA(';',&r));
			/* prise en compte */
			txt_erreur[0] = '\0'; opera_ajoutee = 0;
			if(rep >= RepartNb) {
				if(OpiumCheck("Repartiteur '%s' inconnu: on le cree?",code_rep)) {
					if(RepartNb >= REPART_MAX) sprintf(txt_erreur,"Deja %d repartiteurs connus! Repartiteur abandonne",RepartNb);
					else {
						TypeRepartiteur *repart; int j;
						fait = 0;
						rep = RepartNb; repart = &(Repart[rep]);
						if(*code_rep == 'o') {
							sscanf(code_rep+1,"%d",&num); sprintf(adrs_ip,"192.168.0.%d",num);
							fait = OperaNouvelle(repart,adrs_ip,DONNEES_STD,code_acqui_EDW_BB2,36,1); /* externe */
							opera_ajoutee = fait;
						} else if(*code_rep == 'k') {
							sscanf(code_rep+1,"%d",&num); sprintf(adrs_ip,"192.168.0.%d",num);
							fait = CaliNouvelle(repart,adrs_ip,DONNEES_STD);
						} else if(*code_rep == 'S') fait = CluzelNouvelle(repart,1,DONNEES_STD);
						if(fait) {
							repart->maxentrees = ModeleRep[repart->type].max_numer;
							repart->nbentrees = 1; repart->rep = rep;
							repart->entree = (TypeRepartEntree *)malloc(ModeleRep[repart->type].max_numer * sizeof(TypeRepartEntree));
							for(j=0; j<ModeleRep[repart->type].max_numer; j++) repart->entree[j].adrs = 0;
							// RepartiteurInit(); a definir: repart->type indetermine si ni CLuzel ni OPERA!!!
							RepartiteurVerifieParmsEtFIFO(repart,txt_erreur);
							if(txt_erreur[0] == '\0') {
								RepartNb++;
								printf("  + Repartiteur '%s' cree\n",Repart[rep].nom);
								ajout_repart++;
							}
						} else sprintf(txt_erreur,"Ligne %d: Type de repartiteur imprevu: '%c'",lig,*code_rep);
					}
				} else sprintf(txt_erreur,"Ligne %d: Repartiteur '%s' inconnu",lig,code_rep);
			}
			if(bb >= NumeriseurNb) {
				if(OpiumCheck("Numeriseur '%s' inconnu: on le cree?",nom_bb)) {
					if(NumeriseurNb >= NUMER_MAX) sprintf(txt_erreur,"Deja %d numeriseurs connus! Numeriseur abandonne",NumeriseurNb);
					else {
						s = nom_bb;
						type_bb = ItemJusquA('#',&s); sscanf(s,"%d",&num);
						for(modl=0; modl<ModeleBNNb; modl++) if(!strcmp(type_bb,ModeleBN[modl].nom)) break;
						if(modl >= ModeleBNNb) sprintf(txt_erreur,"Type de numeriseur inconnu: '%s'",type_bb);
						else {
							bb = NumeriseurNb;
							NumeriseurNeuf(&(Numeriseur[bb]),modl,num,(rep < RepartNb)?&(Repart[rep]):0);
							Numeriseur[bb].bb = bb; NumeriseurNb++;
							NumeriseurTermineAjout();
							NumeriseursEcrit(NumeriseurFichierLu,NUMER_RESSOURCES,bb);
							ajout_numer++;
						}
					}
				} else sprintf(txt_erreur,"Ligne %d: Numeriseur '%s' inconnu",lig,nom_bb);
			}
			if((rep < RepartNb) && (bb < NumeriseurNb)) {
				if((l < 0) || (l > Repart[rep].maxentrees)) sprintf(txt_erreur,"Ligne %d: Entree #%d illegale pour une %s",lig,l+1,ModeleRep[Repart[rep].type].nom);
				else {
					if(opera_ajoutee) switch(Numeriseur[bb].vsnid) {
						case 0: Repart[rep].r.opera.mode = code_acqui_ADC_moy2; Repart[rep].r.opera.retard = 2;
							Repart[rep].r.opera.clck = Repart[rep].r.opera.sync = 0;
							break;
						case 1: Repart[rep].r.opera.mode = code_acqui_EDW_BB1; Repart[rep].r.opera.retard = 12; break;
						default:  Repart[rep].r.opera.mode = code_acqui_EDW_BB2; Repart[rep].r.opera.retard = 36; break;
					}
					if(l > Repart[rep].nbentrees) Repart[rep].nbentrees = l + 1;
					Repart[rep].entree[l].adrs = (TypeAdrs)(&(Numeriseur[bb]));
					Numeriseur[bb].fischer = connecteur;
					printf("  . %s[%d] <- %s @%s\n",Repart[rep].nom,l+1,Numeriseur[bb].nom,CablageEncodeConnecteur(Numeriseur[bb].fischer));
				}
			}
			if(txt_erreur[0]) { printf("  ! %s\n",txt_erreur); OpiumError(txt_erreur); nb_erreurs++; }
		}
		fclose(f);
		printf(" . Importation terminee\n");
		if(ajout_repart) printf("  + %d repartiteur%s cree%s\n",Accord2s(ajout_repart));
		if(ajout_numer) printf("  + %d numeriseur%s cree%s\n",Accord2s(ajout_numer));
		if(nb_erreurs) {
			printf("  ! %d erreur%s dans le fichier importe",Accord1s(nb_erreurs));
			OpiumError("%d erreur%s lue%s (voir log)\n",Accord2s(nb_erreurs));
		}
		RepartiteurMajListes();
		sauve = 0;
		OpiumReadBool("On sauve ce cablage",&sauve);
		if(sauve) {
			if(RepartiteursSauve()) printf("  . Connections enregistrees\n");
			else OpiumError("Impossible: %s",strerror(errno));
			NumeriseursEcrit(NumeriseurFichierLu,NUMER_CONNECTION,NUMER_TOUS);
		}
	} else OpiumError("Fichier %s illisible (%s)",nom_complet,strerror(errno));
	return(0);
}
/* ========================================================================== */
char OrgaEcritCSV(char *fichier) {
	FILE *f;
	int bolo,bb,cap,vbb,vm; int i; char hote[20];
	TypeDetecteur *detectr; TypeReglage *regl;
	TypeDetModele *det_modele; TypeBNModlRessource *ress;
	shex connecteur;

	if(!(f = fopen(fichier,"w"))) return(0);
	fprintf(f,";etat;detecteur         ; position ;  capteur  ; cryostat ; numeriseur ; ADC ; Rep  ; reglage ; adrs ; valeur ;\n");
	for(bolo=0; bolo<BoloNb; bolo++) {
		detectr = &(Bolo[bolo]);
		strncpy(hote,detectr->hote,16); hote[16] = '\0';
		for(cap=0; cap<detectr->nbcapt; cap++) {
			vm = detectr->captr[cap].modele;
			bb = detectr->captr[cap].bb.num;
			if(bb >= 0) { connecteur = Numeriseur[bb].fischer; vbb = detectr->captr[cap].bb.adc - 1; }
			else if(detectr->pos != CABLAGE_POS_NOTDEF) {
				connecteur = CablageSortant[detectr->pos].captr[cap].connecteur;
				vbb = CablageSortant[detectr->pos].captr[cap].vbb;
			} else { connecteur = 0; vbb = -1; }
			fprintf(f,"; %2s ; %-16s ;    %-3s   ;",detectr->lu?"ok":"HS",detectr->nom,Bolo[bolo].adresse);
			fprintf(f," %-9s ;",ModeleVoie[detectr->captr[cap].modele].nom);
			if(connecteur) fprintf(f,"  %5s   ;",CablageEncodeConnecteur(connecteur)); else fprintf(f,"     -    ;");
			if(bb >= 0) fprintf(f," %9s  ;",Numeriseur[bb].nom); else fprintf(f,"    NEANT   ;");
			fprintf(f,"  %2d ;",vbb);
			if(bb >= 0) fprintf(f,"  %3s ;",Numeriseur[bb].code_rep);
			else fprintf(f,"  --- ;");
			det_modele = &(ModeleDet[detectr->type]);
			for(i=0; i<detectr->nbreglages; i++) {
				regl = &(detectr->reglage[i]);
				if(cap != regl->capt) continue;
				fprintf(f," %-13s ;",det_modele->regl[i].nom);
				if(regl->ress >= 0) {
					ress = &(ModeleBN[Numeriseur[bb].def.type].ress[regl->ress]);
					fprintf(f," %s",ress->nom);
				}
				if(regl->ressneg >= 0) {
					ress = &(ModeleBN[Numeriseur[bb].def.type].ress[regl->ressneg]);
					fprintf(f," -%s",ress->nom);
				}
				if(regl->resspos >= 0) {
					ress = &(ModeleBN[Numeriseur[bb].def.type].ress[regl->resspos]);
					fprintf(f," +%s",ress->nom);
				}
				fprintf(f,"; %s ;",regl->user);
			}
			fprintf(f,"\n");
		}
	}
	fclose(f);
	return(1);
}
/* ========================================================================== */
int OrgaExporte() {
	char fichier[MAXFILENAME];

	strcat(strcpy(fichier,PrefPath),"OrgaExport.csv");
	if(OrgaEcritCSV(fichier)) printf("%s/ Fichier %s cree\n",DateHeure(),fichier);
	else OpiumError("Le fichier %s n'a pas pu etre cree (%s)",fichier,strerror(errno));
	
	return(0);
}
