#define NUMERISEURS_C
#include <environnement.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>

//#ifdef AVEC_OPERA
//#include <sys/types.h>
//#include <sys/socket.h>
//#endif

#include <utils/claps.h>
#include <utils/decode.h>
#include <utils/dateheure.h>
#include <opium_demande.h>
#include <opium4/opium.h>

#include <samba.h>
#include <numeriseurs.h>
#include <repartiteurs.h>
#include <cablage.h>
#include <organigramme.h>
#include <objets_samba.h> /* pour Acquis->etat */
#include <banc.h>

#ifdef AVEC_PCI
//#include <PCI/plx.h>
#include <PCI/IcaLib.h>
#endif

static char NumeriseurFichierModeles[MAXFILENAME];
static char NumeriseurProcedureDir[80];
static int  NumeriseurDejaAutorise;

static Panel pNumeriseurChoix,pNumeriseurDeplace;

static char NumeriseurDiffuse[NUMER_MAX];
static int  NumeriseursDiffusesNb;
// static Panel pNumeriseursDiffuses;
static Menu mNumeriseurBoutons,mNumeriseurDiffusion;

static TypeModeleBnProc NumeriseurProcsListe[NUMER_PROC_MAX];
static ArgStruct NumeriseurProcAS = { (void *)NumeriseurProcsListe, (void *)&ModeleBnProcLue, sizeof(TypeModeleBnProc), ModeleBnProcDesc };
static int NumeriseurProcsNb;

typedef struct {
	char nom[NUMER_RESS_NOM];
	float valeur;
} TypeNumeriseurCorr;

static TypeNumeriseurCorr NumeriseurCorrLue,NumeriseurCorrListe[NUMER_RESS_MAX];
static int NumeriseurNbCorr;
static ArgDesc NumeriseurCorrDesc[] = {
	{ 0, DESC_STR(NUMER_RESS_NOM) NumeriseurCorrLue.nom,    0 },
	{ 0, DESC_FLOAT             &NumeriseurCorrLue.valeur, 0 },
	DESC_END
};
static ArgStruct NumeriseurCorrAS = { (void *)NumeriseurCorrListe, (void *)&NumeriseurCorrLue, sizeof(TypeNumeriseurCorr), NumeriseurCorrDesc };

static char *NumeriseurNomTrace[NUMER_TRACE_NB+1] = { "DAC", "ADC", "\0" };

static char NumeriseurTracer[NUMER_TRACE_NB];
static int NumeriseurTraceHaut,NumeriseurTraceLarg,NumeriseurTraceVisu;
static float NumeriseurStatusHorloge[2];
static Panel pNumerDemarre;
static char NumerDemarreScript[80]; // ,NumerDemarreReglages[80];
static char NumerDemarreFpga,NumerDemarreModlActif,NumerDemarreGenlActif;

int LectAcqElementaire(NUMER_MODE mode);

#pragma mark ---------- modeles ----------

static void NumeriseurRessDefaut(TypeNumeriseur *numeriseur, TypeBNModele *modele_bn);

static struct {
	int ModeleADCNb,FpgaNb,AccesNb,ModeleBNNb;
	int NumeriseurNb;
} NumerInitial;
/* .......................................................................... */
void NumeriseurBasique(int type_bn, char *nom, int nbadc, int type_adc, float gain) {
	int i;

	strcpy(ModeleBN[type_bn].nom,nom); ModeleBN[type_bn].version = 1.0;
	ModeleBN[type_bn].ident = 0; ModeleBN[type_bn].a_debloquer = 0; ModeleBN[type_bn].fpga = 0;
	ModeleBN[type_bn].nbress = 0; ModeleBN[type_bn].nbprocs = 0; ModeleBN[type_bn].nbenregs = 0;
	for(i=0; i<nbadc; i++) {
		ModeleBN[type_bn].adc[i] = type_adc;
		ModeleBN[type_bn].gain[i] = gain;
	}
	ModeleBN[type_bn].nbadc = ModeleBN[type_bn].nbgains = nbadc;
}
/* .......................................................................... */
void NumeriseurZero() {
	int i; int type_bn,bb;
	int adc_bo,adc_rp,adc_16; int cali;

	i = 0;
	strcpy(ModeleADC[i].nom,"LTC1566"); ModeleADC[i].bits = 14; ModeleADC[i].signe = 1; ModeleADC[i].polar = 0.6; adc_bo = i; i++;
	strcpy(ModeleADC[i].nom,"LTC2145"); ModeleADC[i].bits = 14; ModeleADC[i].signe = 1; ModeleADC[i].polar = 2.0; adc_rp = i; i++;
	strcpy(ModeleADC[i].nom,"LTC2203"); ModeleADC[i].bits = 16; ModeleADC[i].signe = 1; ModeleADC[i].polar = 2.5; adc_16 = i; i++;
	ModeleADCNb = i;

	FpgaNb = 0;

	type_bn = 0;
	NumeriseurBasique(i,"AdcBO",1,adc_bo,1.0); type_bn++;
	NumeriseurBasique(i,"AdcRP",2,adc_rp,1.0); type_bn++;
	NumeriseurBasique(i,"AdcCali",4,adc_16,1.0); cali = type_bn; type_bn++;
	ModeleBNNb = type_bn;

	bb = 0;
	NumeriseurNeuf(&(Numeriseur[bb]),cali,1,0); bb++;
	NumeriseurNb = bb;

}
/* .......................................................................... */
char NumeriseurBuilder(Menu menu, MenuItem item) {
	NumeriseurNb = 1; // a mettre dans NumeriseurBuilder

	NumerInitial.ModeleADCNb = ModeleADCNb; NumerInitial.FpgaNb = FpgaNb;
	NumerInitial.AccesNb = AccesNb; NumerInitial.ModeleBNNb = ModeleBNNb;
	NumerInitial.NumeriseurNb= NumeriseurNb;

etape1:
	OpiumReadTitle("Numeriseurs");
	forever {
		if(OpiumReadInt("Nombre de numeriseurs",&NumeriseurNb) == PNL_CANCEL) return(0);
		if(NumeriseurNb > NUMER_MAX) OpiumFail("Pas plus de %d numeriseurs autorises",NUMER_MAX);
		else break;
	}

//+ etape2:
	OpiumReadTitle("Types de numeriseur");
	while(NumeriseurNb > 1) {
		if(OpiumReadInt("Nombre de TYPES de numeriseur differents",&ModeleBNNb) == PNL_CANCEL) goto etape1;
		if(ModeleBNNb > NUMER_TYPES) OpiumFail("Pas plus de %d types de numeriseur autorises",NUMER_TYPES);
		else break;
	}

	ChassisNumerDim = 1; ChassisNumer[0].nb = NumeriseurNb;  ChassisNumer[0].codage = CHASSIS_CODE_1; ChassisNumer[1].cles = 0;
	NumeriseursEcrit(NumeriseurFichierLu,NUMER_TOUT,NUMER_TOUS);
	return(0);
}
/* .......................................................................... */
/*                                M O D E L E S                               */
/* ========================================================================== */
int NumerModlRessIndex(TypeBNModele *modele_bn, char *nom) {
	int k;
	
	for(k=0; k<modele_bn->nbress; k++) if(!strcmp(modele_bn->ress[k].nom,nom)) break;
	if(k >= modele_bn->nbress) k = -1;
	return(k);
}
/* ========================================================================== */
static int NumerModlRessInt(TypeBNModele *modele_bn, short col, char *nom, shex ssadrs, int bits, 
				   char categ, char *unite, int min, int max) {
	int i; TypeBNModlRessource *ress;
	
	if((i = modele_bn->nbress) >= NUMER_RESS_MAX) return(-1);
	ress = modele_bn->ress + i;
	strncpy(ress->nom,nom,NUMER_RESS_NOM);
	ress->type = RESS_INT;
	ress->ssadrs = ssadrs;
	ress->categ = categ;
	ress->masque = (1 << bits) - 1;
	ress->bit0 = -1;
	ress->init = 0; ress->defaut = 0;
	ress->status = -1;
	if(unite) strncpy(ress->unite,unite,NUMER_RESS_NOM); else ress->unite[0] = '\0';
	ress->col = col;
	ress->lim_i.min = min;
	ress->lim_i.max = max;
	//? ress->usertohexa = 0;  /* la conversion est standard */
	//? ress->hexatouser = 0;  /* la conversion est standard */
	modele_bn->nbress += 1;
	return(i);
}
/* ========================================================================== */
static int NumerModlRessFloat(TypeBNModele *modele_bn, short col, char *nom, shex ssadrs, int bits, 
	char categ, char *unite, float min, float max) {
	int i; TypeBNModlRessource *ress;

	if((i = modele_bn->nbress) >= NUMER_RESS_MAX) return(1);
	ress = modele_bn->ress + i;
	strncpy(ress->nom,nom,NUMER_RESS_NOM);
	ress->categ = categ;
	ress->type = RESS_FLOAT;
	if(unite) strncpy(ress->unite,unite,NUMER_RESS_NOM); else ress->unite[0] = '\0';
	ress->ssadrs = ssadrs;
	ress->masque = (1 << bits) - 1;
	ress->bit0 = -1;
	ress->init = 0; ress->defaut = 0;
	ress->status = -1;
	ress->col = col;
	ress->lim_r.min = min;
	ress->lim_r.max = max;
	//? ress->usertohexa = 0;  /* la conversion est standard */
	//? ress->hexatouser = 0;  /* la conversion est standard */
	modele_bn->nbress += 1;
	return(i);
}
/* ========================================================================== */
static int NumerModlRessCles(TypeBNModele *modele_bn, short col, char *nom, shex ssadrs, int bits, 
	char categ, char *unite, char *cles, char *hval) {
	int i; TypeBNModlRessource *ress;
	int j; char *c,*d,liste[256];

	if((i = modele_bn->nbress) >= NUMER_RESS_MAX) return(1);
	ress = modele_bn->ress + i;
	strncpy(ress->nom,nom,NUMER_RESS_NOM);
	ress->categ = categ;
	ress->type = RESS_CLES;
	if(unite) strncpy(ress->unite,unite,NUMER_RESS_NOM); else ress->unite[0] = '\0';
	ress->ssadrs = ssadrs;
	ress->masque = (1 << bits) - 1;
	ress->bit0 = -1;
	ress->init = 0; ress->defaut = 0;
	ress->status = -1;
	ress->col = col;
	strncpy(ress->lim_k.cles,cles,NUMER_RESS_KEYSTR);
	strcpy(liste,hval);
	j = 0; c = liste;
	while(j < NUMER_CLES_MAX) {
		d = ItemSuivant(&c,'/');
		if(*d) {
			if(!strncmp(d,"0x",2) || !strncmp(d,"0X",2))
				sscanf(d,"%hx",&(ress->lim_k.hval[j]));
			else sscanf(d,"%hd",&(ress->lim_k.hval[j]));
		} else break;
		j++;
	}
	ress->lim_k.nb = j;
	//? ress->usertohexa = 0;  /* la conversion est standard */
	//? ress->hexatouser = 0;  /* la conversion est standard */
	modele_bn->nbress += 1;
	return(i);
}
/* ========================================================================== */
static int NumerModlRessPuiss(TypeBNModele *modele_bn, short col, char *nom, shex ssadrs, int bits, 
	char categ, char *unite, int bit_min, int val_min) {
	int i; TypeBNModlRessource *ress;

	if((i = modele_bn->nbress) >= NUMER_RESS_MAX) return(-1);
	ress = modele_bn->ress + i;
	strncpy(ress->nom,nom,NUMER_RESS_NOM);
	ress->categ = categ;
	ress->type = RESS_PUIS2;
	if(unite) strncpy(ress->unite,unite,NUMER_RESS_NOM); else ress->unite[0] = '\0';
	ress->ssadrs = ssadrs;
	ress->masque = (1 << bits) - 1;
	ress->bit0 = -1;
	ress->init = 0; ress->defaut = 0;
	ress->status = -1;
	ress->col = col;
	ress->lim_i.min = bit_min;
	ress->lim_i.max = val_min;
	//? ress->usertohexa = 0;  /* la conversion est standard */
	//? ress->hexatouser = 0;  /* la conversion est standard */
	modele_bn->nbress += 1;
	return(i);
}
/* ========================================================================== */
static void NumerModlRessChamp(TypeBNModele *modele_bn, int i, char bit0) {
	TypeBNModlRessource *ress;

	if((i < 0) || (i >= modele_bn->nbress)) return;
	ress = modele_bn->ress + i;
	ress->bit0 = bit0;
}
/* ========================================================================== */
static void NumerModlRessInit(TypeBNModele *modele_bn, int i, shex hval) {
	TypeBNModlRessource *ress;

	if((i < 0) || (i >= modele_bn->nbress)) return;
	ress = modele_bn->ress + i;
	ress->init = 1; ress->defaut = hval;
}
/* ========================================================================== */
static void NumerModlRessGroupeStatus(TypeBNModele *modele_bn, int i, int mot, int nb) {
	int j;

	if((i < 0) || (i >= modele_bn->nbress)) return;
	for(j=0; j<nb; j++) {
		modele_bn->numress[mot + j] = i + j;
		modele_bn->ress[i + j].status = mot + j;
	}
}
/* ========================================================================== */
static void NumerModlRessFinaliseStatus() {
	TypeBNModele *modele_bn; int i,j,k;
	
	modele_bn = &(ModeleBN[0]);
	for(i=0; i<ModeleBNNb; i++,modele_bn++) {
		for(j=0; j<NUMER_STATUS_DIM; j++) {
			k = ModeleBN[i].numress[j];
			if((k >= 0) && (k < modele_bn->nbress)) {
				if((modele_bn->ress[k].bit0 >= 0) && (modele_bn->acces.mode == NUMER_ACCES_BB)) {
					byte ssadrs; int n;
					ssadrs = modele_bn->ress[k].ssadrs;
					for(n=0; n<modele_bn->nbress; n++) if((modele_bn->ress[n].bit0 >= 0) && (modele_bn->ress[n].ssadrs == ssadrs)) modele_bn->ress[n].status = j;
				} else modele_bn->ress[k].status = j;
			}
		}
	}
}
/* ========================================================================== */
static void NumerModlStandard() {
	int i,j;
	TypeBNModele *bn;
	float g_modul,g_compens,f_trngl,f_carre;
	int adc_bb1,adc_ab,adc_nostos,adc_ni;
	
	for(i=0; i<NUMER_TYPES; i++) for(j=0; j<NUMER_STATUS_DIM; j++) ModeleBN[i].numress[j] = -1;

	ModeleADCNb = 0;
#ifdef NUMER_EDW2
	strcpy(ModeleADC[ModeleADCNb].nom,"LTC1417v0");
	ModeleADC[ModeleADCNb].bits = 14; ModeleADC[ModeleADCNb].signe = 1;
	ModeleADC[ModeleADCNb].polar = 2.048;
	ModeleADCNb++;
#endif
	adc_bb1 = ModeleADCNb;
	strcpy(ModeleADC[ModeleADCNb].nom,"LTC1417v1");
	ModeleADC[ModeleADCNb].bits = 14; ModeleADC[ModeleADCNb].signe = 1;
	ModeleADC[ModeleADCNb].polar = 2.5;
	ModeleADCNb++;

	adc_nostos = ModeleADCNb;
#ifdef NUMER_EDW2
	strcpy(ModeleADC[ModeleADCNb].nom,"LTC2248"); /* utilise aux debuts de NOSTOS */
	ModeleADC[ModeleADCNb].bits = 14; ModeleADC[ModeleADCNb].signe = 1;
	ModeleADC[ModeleADCNb].polar = 2.5 /* ?? */;
	ModeleADCNb++;
#endif
	
	adc_ab = ModeleADCNb;
	strcpy(ModeleADC[ModeleADCNb].nom,"LTC1864-14");
	ModeleADC[ModeleADCNb].bits = 16 - 2; ModeleADC[ModeleADCNb].signe = 1;
	ModeleADC[ModeleADCNb].polar = 2.25 / 4.0;
	ModeleADCNb++;
	strcpy(ModeleADC[ModeleADCNb].nom,"LTC1566"); /* dans le bouchon Opera */
	ModeleADC[ModeleADCNb].bits = 14; ModeleADC[ModeleADCNb].signe = 1;
	ModeleADC[ModeleADCNb].polar = 0.3;
	ModeleADCNb++;
	
	adc_ni = ModeleADCNb;
#ifdef NIDAQ
	strcpy(ModeleADC[ModeleADCNb].nom,"NI");
	ModeleADC[ModeleADCNb].bits = 16; ModeleADC[ModeleADCNb].signe = 1;
	ModeleADC[ModeleADCNb].polar = 5.0;
	ModeleADCNb++;
#endif

	bn = &(ModeleBN[0]);
#ifdef NUMER_EDW2
	strcpy(bn->nom,"BB0");
	bn->version = 0.1;
	bn->a_debloquer = 0;
	bn->ident = 0;
	bn->nbress = 0;
	i = NumerModlRessInt  (bn,0,"D2",      0x02,12,RESS_D2,0,0,4095); NumerModlRessInit(bn,i,100);
	i = NumerModlRessInt  (bn,0,"D3",      0x03,16,RESS_D3,0,0,65535); NumerModlRessInit(bn,i,1000);
	i = NumerModlRessCles (bn,0,"acquis",  0x08, 1,RESS_DIVERS,0,"marche/arret","0x00/0x01"); NumerModlRessInit(bn,i,1);
	i = NumerModlRessCles (bn,0,"ident",   0x09, 1,RESS_DIVERS,0,"marche/arret","0x00/0x01"); NumerModlRessInit(bn,i,0);
	    NumerModlRessFloat(bn,1,"DAC1",    0x18,12,RESS_MODUL,"Volts",0.0,10.0);
	    NumerModlRessFloat(bn,1,"DAC2",    0x19,12,RESS_COMP ,"Volts",0.0,10.0);
	    NumerModlRessFloat(bn,1,"DAC3",    0x1A,12,RESS_CORR ,"% ampl",0.0,100.0);
	    NumerModlRessFloat(bn,1,"DAC4",    0x1B,12,RESS_CORR ,"% ampl",0.0,100.0);
        NumerModlRessCles (bn,2,"gain-v1", 0x20, 6,RESS_GAIN,
	    	0,"2/5.1/8.1/21/36/39.9","0x01/0x02/0x04/0x08/0x10/0x20");
	    NumerModlRessCles (bn,2,"gain-v2", 0x21, 6,RESS_GAIN,
	    	0,"2/5.1/8.1/21/36/39.9","0x01/0x02/0x04/0x08/0x10/0x20");
	    NumerModlRessCles (bn,2,"gain-v3", 0x22, 6,RESS_GAIN,
	    	0,"2/5.1/8.1/21/36/39.9","0x01/0x02/0x04/0x08/0x10/0x20");
	i = NumerModlRessCles (bn,2,"cntl-v1", 0x28, 3,RESS_FET_MASSE,0,"libre/masse","0x0000/0x0004"); NumerModlRessInit(bn,i,0);
	i = NumerModlRessFloat(bn,0,"Rmodul",  0xFF,16,RESS_DIVERS,"kOhms",0.0,65535.0); NumerModlRessInit(bn,i,100);
	i = NumerModlRessFloat(bn,0,"Cmodul",  0xFF,16,RESS_DIVERS,"nF",0.0,65535.0); NumerModlRessInit(bn,i,22);
	bn->nbadc = 3; for(i=0; i<bn->nbadc; i++) bn->adc[i] = 0;
	bn->nbgains = 3; bn->gain[0] = 1010.0; bn->gain[1] = 40.0; bn->gain[2] = 40.0;
	bn->nbprocs = 0;
	bn->nbenregs = 0;
	bn++;

	strcpy(bn->nom,"BB1IPNL");
	bn->version = 1.0;
	bn->a_debloquer = 0;
	bn->ident = 1;
	bn->nbress = 0;
	i = NumerModlRessInt  (bn,0,"D2",      0x02,12,RESS_D2,0,0,4095); NumerModlRessInit(bn,i,100);
	i = NumerModlRessInt  (bn,0,"D3",      0x03,16,RESS_D3,0,0,65535); NumerModlRessInit(bn,i,1000);
	i = NumerModlRessCles (bn,0,"acquis",  0x08, 1,RESS_DIVERS,0,"marche/arret","0x00/0x01"); NumerModlRessInit(bn,i,1);
	i = NumerModlRessCles (bn,0,"ident",   0x09, 1,RESS_DIVERS,0,"marche/arret","0x00/0x01"); NumerModlRessInit(bn,i,0);
	    NumerModlRessFloat(bn,1,"DAC1",    0x18,12,RESS_MODUL,"Volts",0.0,10.0);
	    NumerModlRessFloat(bn,1,"DAC2",    0x19,12,RESS_COMP ,"Volts",0.0,10.0);
	    NumerModlRessFloat(bn,1,"DAC3",    0x1A,12,RESS_CORR ,"% ampl",0.0,100.0);
	    NumerModlRessFloat(bn,1,"DAC4",    0x1B,12,RESS_CORR ,"% ampl",0.0,100.0);
	    NumerModlRessFloat(bn,1,"DAC5",    0x1C,12,RESS_POLAR,"Volts",-10.0,10.0);
	    NumerModlRessFloat(bn,1,"DAC6",    0x1D,12,RESS_POLAR,"Volts",-10.0,10.0);
	    NumerModlRessCles (bn,2,"gain-v1", 0x20, 6,RESS_GAIN,
	    	0,"2/5.1/8.1/21/36/39.9","0x01/0x02/0x04/0x08/0x10/0x20");
	    NumerModlRessCles (bn,2,"gain-v2", 0x21, 6,RESS_GAIN,
	    	0,"2/5.1/8.1/21/36/39.9","0x01/0x02/0x04/0x08/0x10/0x20");
	    NumerModlRessCles (bn,2,"gain-v3", 0x22, 6,RESS_GAIN,
	    	0,"2/5.1/8.1/21/36/39.9","0x01/0x02/0x04/0x08/0x10/0x20");
	i = NumerModlRessCles (bn,2,"cntl-v1", 0x28, 3,RESS_FET_MASSE,0,"libre/masse","0x0000/0x0004"); NumerModlRessInit(bn,i,0);
	i = NumerModlRessFloat(bn,0,"Rmodul",  0xFF,16,RESS_DIVERS,"kOhms",0.0,65535.0); NumerModlRessInit(bn,i,100);
	i = NumerModlRessFloat(bn,0,"Cmodul",  0xFF,16,RESS_DIVERS,"nF",0.0,65535.0); NumerModlRessInit(bn,i,22);
	bn->nbadc = 3; for(i=0; i<bn->nbadc; i++) bn->adc[i] = 0;
	bn->nbgains = 3; bn->gain[0] = 1010.0; bn->gain[1] = 40.0; bn->gain[2] = 40.0;
	bn->nbprocs = 0;
	bn->nbenregs = 0;
	bn++;
#endif
	
	strcpy(bn->nom,"BB1-SAC");
	bn->version = 1.1;
	bn->a_debloquer = 1;
	bn->ident = 1;
	bn->nbress = 0;
	i = NumerModlRessInt  (bn,0,"D2",      0x02,12,RESS_D2,0,0,4095); NumerModlRessInit(bn,i,100); NumerModlRessInit(bn,i,100);
	i = NumerModlRessInt  (bn,0,"D3",      0x03,16,RESS_D3,0,0,65535); NumerModlRessInit(bn,i,1000); NumerModlRessInit(bn,i,1000);
	i = NumerModlRessCles (bn,0,"acquis",  0x08, 1,RESS_DIVERS,0,"marche/arret","0x00/0x01"); NumerModlRessInit(bn,i,1);
	i = NumerModlRessCles (bn,0,"ident",   0x09, 1,RESS_DIVERS,0,"marche/arret","0x00/0x01"); NumerModlRessInit(bn,i,0);
	    NumerModlRessFloat(bn,1,"DAC1",    0x18,12,RESS_MODUL,"Volts",0.0,10.0);
	    NumerModlRessFloat(bn,1,"DAC2",    0x19,12,RESS_COMP ,"Volts",0.0,10.0);
	    NumerModlRessFloat(bn,1,"DAC3",    0x1A,12,RESS_CORR ,"% ampl",0.0,100.0);
	    NumerModlRessFloat(bn,1,"DAC4",    0x1B,12,RESS_CORR ,"% ampl",0.0,100.0);
	    NumerModlRessFloat(bn,1,"DAC5",    0x1C,12,RESS_POLAR,"Volts",-10.0,10.0);
	    NumerModlRessFloat(bn,1,"DAC6",    0x1D,12,RESS_DIVERS,"Volts",-10.0,10.0);
	    NumerModlRessFloat(bn,1,"DAC7",    0x1E,12,RESS_DIVERS,"Volts",-2.0,2.0);
	    NumerModlRessFloat(bn,1,"DAC8",    0x1F,12,RESS_DIVERS,"Volts",-2.0,2.0);
	    NumerModlRessCles (bn,2,"gain-v1", 0x20, 8,RESS_GAIN,
	    	0,"0.9/2.5/4.2/12.3/20.4/34.8","0x01/0x02/0x04/0x08/0x10/0x20");
	    NumerModlRessCles (bn,2,"gain-v2", 0x21, 8,RESS_GAIN,
	    	0,"0.9/2.5/4.2/12.3/20.4/34.8","0x01/0x02/0x04/0x08/0x10/0x20");
	    NumerModlRessCles (bn,2,"gain-v3", 0x22, 8,RESS_GAIN,
	    	0,"0.9/2.5/4.2/12.3/20.4/34.8","0x01/0x02/0x04/0x08/0x10/0x20");
	i = NumerModlRessCles (bn,2,"cntl-v1", 0x28, 3,RESS_FET_MASSE,0,"libre/masse","0x0000/0x0004"); NumerModlRessInit(bn,i,0);
	i = NumerModlRessFloat(bn,0,"Rmodul",  0xFF,16,RESS_DIVERS,"kOhms",0.0,65535.0); NumerModlRessInit(bn,i,100);
	i = NumerModlRessFloat(bn,0,"Cmodul",  0xFF,16,RESS_DIVERS,"nF",0.0,65535.0); NumerModlRessInit(bn,i,22);
	bn->nbadc = 3; for(i=0; i<bn->nbadc; i++) bn->adc[i] = adc_bb1;
	bn->nbgains = 3; bn->gain[0] = 1010.0; bn->gain[1] = 22.0; bn->gain[2] = 22.0;
	bn->nbprocs = 1;
	strcpy(bn->proc[0].nom,"deblocage"); strcpy(bn->proc[0].fichier,"procedures_BB1SAC/deblocage");
	bn->nbenregs = 0;
	bn++;

	strcpy(bn->nom,"BB2");
	bn->version = 2.0;
	bn->a_debloquer = 0;
	bn->ident = 2;
	bn->nbress = 0;
	i = NumerModlRessInt  (bn,0,"regul-gain-v1",0x39,4,RESS_DIVERS,0,0,15); NumerModlRessChamp(bn,i,0); NumerModlRessInit(bn,i,4);
	i = NumerModlRessInt  (bn,0,"regul-gain-v2",0x3A,4,RESS_DIVERS,0,0,15); NumerModlRessChamp(bn,i,0); NumerModlRessInit(bn,i,4);
	i = NumerModlRessInt  (bn,0,"regul-gain-v3",0x3B,4,RESS_DIVERS,0,0,15); NumerModlRessChamp(bn,i,0); NumerModlRessInit(bn,i,0);
	i = NumerModlRessInt  (bn,0,"regul-gain-v4",0x3C,4,RESS_DIVERS,0,0,15); NumerModlRessChamp(bn,i,0); NumerModlRessInit(bn,i,0);
	i = NumerModlRessInt  (bn,0,"regul-gain-v5",0x3D,4,RESS_DIVERS,0,0,15); NumerModlRessChamp(bn,i,0); NumerModlRessInit(bn,i,4);
	i = NumerModlRessInt  (bn,0,"regul-gain-v6",0x3E,4,RESS_DIVERS,0,0,15); NumerModlRessChamp(bn,i,0); NumerModlRessInit(bn,i,4);
	i = NumerModlRessInt  (bn,1,"regul-dacA-v1",0x39,4,RESS_DIVERS,0,0,15); NumerModlRessChamp(bn,i,8); NumerModlRessInit(bn,i,1);
	i = NumerModlRessInt  (bn,1,"regul-dacA-v2",0x3A,4,RESS_DIVERS,0,0,15); NumerModlRessChamp(bn,i,8); NumerModlRessInit(bn,i,3);
	i = NumerModlRessInt  (bn,1,"regul-dacA-v3",0x3B,4,RESS_DIVERS,0,0,15); NumerModlRessChamp(bn,i,8); NumerModlRessInit(bn,i,0);
	i = NumerModlRessInt  (bn,1,"regul-dacA-v4",0x3C,4,RESS_DIVERS,0,0,15); NumerModlRessChamp(bn,i,8); NumerModlRessInit(bn,i,0);
	i = NumerModlRessInt  (bn,1,"regul-dacA-v5",0x3D,4,RESS_DIVERS,0,0,15); NumerModlRessChamp(bn,i,8); NumerModlRessInit(bn,i,11);
	i = NumerModlRessInt  (bn,1,"regul-dacA-v6",0x3E,4,RESS_DIVERS,0,0,15); NumerModlRessChamp(bn,i,8); NumerModlRessInit(bn,i,12);
	i = NumerModlRessCles (bn,2,"regul-sensA-v1",0x39,1,RESS_DIVERS,0,"+/-","0x00/0x01"); NumerModlRessChamp(bn,i,4); NumerModlRessInit(bn,i,0);
	i = NumerModlRessCles (bn,2,"regul-sensA-v2",0x3A,1,RESS_DIVERS,0,"+/-","0x00/0x01"); NumerModlRessChamp(bn,i,4); NumerModlRessInit(bn,i,0);
	i = NumerModlRessCles (bn,2,"regul-sensA-v3",0x3B,1,RESS_DIVERS,0,"+/-","0x00/0x01"); NumerModlRessChamp(bn,i,4); NumerModlRessInit(bn,i,0);
	i = NumerModlRessCles (bn,2,"regul-sensA-v4",0x3C,1,RESS_DIVERS,0,"+/-","0x00/0x01"); NumerModlRessChamp(bn,i,4); NumerModlRessInit(bn,i,0);
	i = NumerModlRessCles (bn,2,"regul-sensA-v5",0x3D,1,RESS_DIVERS,0,"+/-","0x00/0x01"); NumerModlRessChamp(bn,i,4); NumerModlRessInit(bn,i,0);
	i = NumerModlRessCles (bn,2,"regul-sensA-v6",0x3E,1,RESS_DIVERS,0,"+/-","0x00/0x01"); NumerModlRessChamp(bn,i,4); NumerModlRessInit(bn,i,0);
	i = NumerModlRessInt  (bn,3,"regul-dacB-v1",0x39,4,RESS_DIVERS,0,0,15); NumerModlRessChamp(bn,i,12); NumerModlRessInit(bn,i,0);
	i = NumerModlRessInt  (bn,3,"regul-dacB-v2",0x3A,4,RESS_DIVERS,0,0,15); NumerModlRessChamp(bn,i,12); NumerModlRessInit(bn,i,0);
	i = NumerModlRessInt  (bn,3,"regul-dacB-v3",0x3B,4,RESS_DIVERS,0,0,15); NumerModlRessChamp(bn,i,12); NumerModlRessInit(bn,i,0);
	i = NumerModlRessInt  (bn,3,"regul-dacB-v4",0x3C,4,RESS_DIVERS,0,0,15); NumerModlRessChamp(bn,i,12); NumerModlRessInit(bn,i,0);
	i = NumerModlRessInt  (bn,3,"regul-dacB-v5",0x3D,4,RESS_DIVERS,0,0,15); NumerModlRessChamp(bn,i,12); NumerModlRessInit(bn,i,0);
	i = NumerModlRessInt  (bn,3,"regul-dacB-v6",0x3E,4,RESS_DIVERS,0,0,15); NumerModlRessChamp(bn,i,12); NumerModlRessInit(bn,i,0);
	i = NumerModlRessCles (bn,4,"regul-sensB-v1",0x39,1,RESS_DIVERS,0,"+/-","0x00/0x01"); NumerModlRessChamp(bn,i,5); NumerModlRessInit(bn,i,0);
	    NumerModlRessGroupeStatus(bn,i,status_regul+1,6); /* implique aussi les 24 ressources ci-dessus */
	i = NumerModlRessCles (bn,4,"regul-sensB-v2",0x3A,1,RESS_DIVERS,0,"+/-","0x00/0x01"); NumerModlRessChamp(bn,i,5); NumerModlRessInit(bn,i,0);
	i = NumerModlRessCles (bn,4,"regul-sensB-v3",0x3B,1,RESS_DIVERS,0,"+/-","0x00/0x01"); NumerModlRessChamp(bn,i,5); NumerModlRessInit(bn,i,0);
	i = NumerModlRessCles (bn,4,"regul-sensB-v4",0x3C,1,RESS_DIVERS,0,"+/-","0x00/0x01"); NumerModlRessChamp(bn,i,5); NumerModlRessInit(bn,i,0);
	i = NumerModlRessCles (bn,4,"regul-sensB-v5",0x3D,1,RESS_DIVERS,0,"+/-","0x00/0x01"); NumerModlRessChamp(bn,i,5); NumerModlRessInit(bn,i,0);
	i = NumerModlRessCles (bn,4,"regul-sensB-v6",0x3E,1,RESS_DIVERS,0,"+/-","0x00/0x01"); NumerModlRessChamp(bn,i,5); NumerModlRessInit(bn,i,0);
	i = NumerModlRessPuiss(bn,0,"regul-periode",0x38,4,RESS_DIVERS,"secondes",1,1); NumerModlRessChamp(bn,i,0); NumerModlRessInit(bn,i,0);
	i = NumerModlRessPuiss(bn,0,"regul-gain",   0x38,4,RESS_GAIN,0,1,1); NumerModlRessChamp(bn,i,8); NumerModlRessInit(bn,i,0);
	    NumerModlRessGroupeStatus(bn,i,status_regul,1);
	i = NumerModlRessInt  (bn,0,"Ident",   0xFF,16,RESS_IDENT,0,0,65535);
	    NumerModlRessGroupeStatus(bn,i,status_nserie,1);
//	i = NumerModlRessCles (bn,0,"erreurs", 0xFF, 4,RESS_DIVERS,0,"neant/STATUS/SYNCHRO/TOUT","0x00/0x01/0x02/0x03"); NumerModlRessChamp(bn,i,2);
	i = NumerModlRessCles (bn,0,"synchro", 0xFF, 1,RESS_DIVERS,0,"OK/ERREUR","0x00/0x01"); NumerModlRessChamp(bn,i,2);
	    NumerModlRessGroupeStatus(bn,i,status_tempe,2);
	i = NumerModlRessFloat(bn,0,"temper",  0xFF,12,RESS_DIVERS,"C",0.0,256.0);             NumerModlRessChamp(bn,i,4);
//	i = NumerModlRessCles (bn,0,"alim",    0x0A, 4,RESS_ALIM,0,"eteint/liberer/allume/bloquer/charger/stoppe/verrou","0x00/0x37/0x01/0x5/0x0F/0x02/0x03"); NumerModlRessInit(bn,i,0);
	i = NumerModlRessCles (bn,0,"alim",    0x0A, 4,RESS_ALIM,0,"bloquer/liberer/eteint/actif/charger/allume/bizarre/bloque","0x00/0x06/0x02/0x03/0x07/0x01/0x04/0x05"); NumerModlRessInit(bn,i,0x06);
		NumerModlRessChamp(bn,i,0);
	    NumerModlRessGroupeStatus(bn,i,status_alim,1);
//	i = NumerModlRessCles (bn,0,"mode",    0x0A, 2,RESS_DIVERS,0,"neant/rapide/lent","0x00/0x01/0x02"); NumerModlRessInit(bn,i,0);
//		NumerModlRessChamp(bn,i,4);
	i = NumerModlRessInt  (bn,0,"D2",      0x02,12,RESS_D2,0,0,4095); NumerModlRessInit(bn,i,100);
	    NumerModlRessGroupeStatus(bn,i,status_div,2);
	i = NumerModlRessInt  (bn,0,"D3",      0x03,16,RESS_D3,0,0,65535); NumerModlRessInit(bn,i,1000);
	    //+ NumerModlRessCles (bn,0,0x08,"mode",0,-1,RESS_DIVERS,"ident/acquis","0x00/0x01");
	i = NumerModlRessFloat(bn,1,"DAC1",    0x11,16,RESS_AUXI,"Volts",-10.0,10.0); NumerModlRessInit(bn,i,0x8000);
	    NumerModlRessGroupeStatus(bn,i,status_dacBN,12);
	i = NumerModlRessFloat(bn,1,"DAC2",    0x12,16,RESS_AUXI,"Volts",-10.0,10.0); NumerModlRessInit(bn,i,0x8000);
	i = NumerModlRessFloat(bn,1,"DAC3",    0x13,16,RESS_AUXI,"Volts",-10.0,10.0); NumerModlRessInit(bn,i,0x8000);
	i = NumerModlRessFloat(bn,1,"DAC4",    0x14,16,RESS_AUXI,"Volts",-10.0,10.0); NumerModlRessInit(bn,i,0x8000);
	i = NumerModlRessFloat(bn,1,"DAC5",    0x15,16,RESS_AUXI,"Volts",-10.0,10.0); NumerModlRessInit(bn,i,0x8000);
	i = NumerModlRessFloat(bn,1,"DAC6",    0x16,16,RESS_AUXI,"Volts",-10.0,10.0); NumerModlRessInit(bn,i,0x8000);
	i = NumerModlRessFloat(bn,1,"DAC7",    0x17,16,RESS_AUXI,"Volts",-10.0,10.0); NumerModlRessInit(bn,i,0x8000);
	i = NumerModlRessFloat(bn,1,"DAC8",    0x18,16,RESS_AUXI,"Volts",-10.0,10.0); NumerModlRessInit(bn,i,0x8000);
	i = NumerModlRessFloat(bn,1,"DAC9",    0x19,16,RESS_AUXI,"Volts",-10.0,10.0); NumerModlRessInit(bn,i,0x8000);
	i = NumerModlRessFloat(bn,1,"DAC10",   0x1A,16,RESS_AUXI,"Volts",-10.0,10.0); NumerModlRessInit(bn,i,0x8000);
	i = NumerModlRessFloat(bn,1,"DAC11",   0x1B,16,RESS_AUXI,"Volts",-10.0,10.0); NumerModlRessInit(bn,i,0x8000);
	i = NumerModlRessFloat(bn,1,"DAC12",   0x1C,16,RESS_AUXI,"Volts",-10.0,10.0); NumerModlRessInit(bn,i,0x8000);
	i = NumerModlRessCles (bn,0,"relais-1",0x1D,1,RESS_AUXI,0,"ouvert/ferme","0x00/0x01"); NumerModlRessChamp(bn,i,0); NumerModlRessInit(bn,i,0);
	    NumerModlRessGroupeStatus(bn,i,status_dacBN+12,2);
	i = NumerModlRessCles (bn,0,"relais-2",0x1D,1,RESS_AUXI,0,"ouvert/ferme","0x00/0x01"); NumerModlRessChamp(bn,i,1); NumerModlRessInit(bn,i,0);
	i = NumerModlRessCles (bn,0,"relais-3",0x1D,1,RESS_AUXI,0,"ouvert/ferme","0x00/0x01"); NumerModlRessChamp(bn,i,2); NumerModlRessInit(bn,i,0);
	    NumerModlRessCles (bn,0,"mode",    0x08,1,RESS_DIVERS,0,"acquis/ident","0x00/0x01");
	i = NumerModlRessCles (bn,0,"reference",0x1E,1,RESS_AUXI,0,"absente/presente","0x00/0x01"); NumerModlRessInit(bn,i,0);
	    NumerModlRessGroupeStatus(bn,i,status_dacBN+13,1);
	i = NumerModlRessInt  (bn,3,"gain-v1",     0x31, 4,RESS_GAIN,0,1,16); NumerModlRessChamp(bn,i,0);
	i = NumerModlRessInt  (bn,3,"gain-v2",     0x32, 4,RESS_GAIN,0,1,16); NumerModlRessChamp(bn,i,0);
	i = NumerModlRessInt  (bn,3,"gain-v3",     0x33, 4,RESS_GAIN,0,1,16); NumerModlRessChamp(bn,i,0);
	i = NumerModlRessInt  (bn,3,"gain-v4",     0x34, 4,RESS_GAIN,0,1,16); NumerModlRessChamp(bn,i,0);
	i = NumerModlRessInt  (bn,3,"gain-v5",     0x35, 4,RESS_GAIN,0,1,16); NumerModlRessChamp(bn,i,0);
	i = NumerModlRessInt  (bn,3,"gain-v6",     0x36, 4,RESS_GAIN,0,1,16); NumerModlRessChamp(bn,i,0);
	i = NumerModlRessFloat(bn,2,"fltr-v1",     0x31, 4,RESS_DIVERS,"kHz",0.0,150.0); NumerModlRessChamp(bn,i,4); NumerModlRessInit(bn,i,5);
	    NumerModlRessGroupeStatus(bn,i,status_filtre,6); /* implique aussi les 6 gains ci-dessus */
	i = NumerModlRessFloat(bn,2,"fltr-v2",     0x32, 4,RESS_DIVERS,"kHz",0.0,150.0); NumerModlRessChamp(bn,i,4); NumerModlRessInit(bn,i,5);
	i = NumerModlRessFloat(bn,2,"fltr-v3",     0x33, 4,RESS_DIVERS,"kHz",0.0,150.0); NumerModlRessChamp(bn,i,4); NumerModlRessInit(bn,i,5);
	i = NumerModlRessFloat(bn,2,"fltr-v4",     0x34, 4,RESS_DIVERS,"kHz",0.0,150.0); NumerModlRessChamp(bn,i,4); NumerModlRessInit(bn,i,5);
	i = NumerModlRessFloat(bn,2,"fltr-v5",     0x35, 4,RESS_DIVERS,"kHz",0.0,150.0); NumerModlRessChamp(bn,i,4); NumerModlRessInit(bn,i,5);
	i = NumerModlRessFloat(bn,2,"fltr-v6",     0x36, 4,RESS_DIVERS,"kHz",0.0,150.0); NumerModlRessChamp(bn,i,4); NumerModlRessInit(bn,i,5);
/*	i = NumerModlRessInt  (bn,4,"ADC1",        0xFF,16,RESS_ADC,"ADU",-32768,32767);
	    NumerModlRessInt  (bn,4,"ADC2",        0xFF,16,RESS_ADC,"ADU",-32768,32767);
	    NumerModlRessInt  (bn,4,"ADC3",        0xFF,16,RESS_ADC,"ADU",-32768,32767);
	    NumerModlRessInt  (bn,4,"ADC4",        0xFF,16,RESS_ADC,"ADU",-32768,32767);
	    NumerModlRessInt  (bn,4,"ADC5",        0xFF,16,RESS_ADC,"ADU",-32768,32767);
	    NumerModlRessInt  (bn,4,"ADC6",        0xFF,16,RESS_ADC,"ADU",-32768,32767); */
	i = NumerModlRessInt  (bn,4,"ADC1",        0xFF,14,RESS_ADC,"ADU",-8192,8191);
	    NumerModlRessInt  (bn,4,"ADC2",        0xFF,14,RESS_ADC,"ADU",-8192,8191);
	    NumerModlRessInt  (bn,4,"ADC3",        0xFF,14,RESS_ADC,"ADU",-8192,8191);
	    NumerModlRessInt  (bn,4,"ADC4",        0xFF,14,RESS_ADC,"ADU",-8192,8191);
	    NumerModlRessInt  (bn,4,"ADC5",        0xFF,14,RESS_ADC,"ADU",-8192,8191);
	    NumerModlRessInt  (bn,4,"ADC6",        0xFF,14,RESS_ADC,"ADU",-8192,8191);
	    NumerModlRessGroupeStatus(bn,i,status_adc,6);
		g_modul = 0.1;              /* gain hard avant DAC */
		g_compens = 2.0 / 1.1;      /* gain hard avant ADC */
		f_trngl = 3000.0 / 256.0;   /* facteur soft, =delta_V a chaque coup d'horloge, resultat en mV */
		f_carre = 0.5;              /* facteur soft permanent sur le carre */
	i = NumerModlRessFloat(bn,2,"comp-tri-v5", 0x21,16,RESS_COMP, "mV/ech", -4.5*g_compens*f_trngl, 4.5*g_compens*f_trngl);
	    NumerModlRessFloat(bn,2,"comp-tri-v6", 0x22,16,RESS_COMP, "mV/ech", -4.5*g_compens*f_trngl, 4.5*g_compens*f_trngl);
	    NumerModlRessFloat(bn,2,"ampl-tri-d9", 0x23,16,RESS_MODUL,"mV/ech", -4.5*g_modul*  f_trngl, 4.5*g_modul  *f_trngl);
	    NumerModlRessFloat(bn,2,"ampl-tri-d10",0x24,16,RESS_MODUL,"mV/ech", -4.5*g_modul*  f_trngl, 4.5*g_modul  *f_trngl);
	    NumerModlRessFloat(bn,2,"ampl-tri-d11",0x25,16,RESS_MODUL,"mV/ech", -4.5*g_modul*  f_trngl, 4.5*g_modul  *f_trngl);
	    NumerModlRessFloat(bn,2,"ampl-tri-d12",0x26,16,RESS_MODUL,"mV/ech", -4.5*g_modul*  f_trngl, 4.5*g_modul  *f_trngl);
	    NumerModlRessGroupeStatus(bn,i,status_triangle,6);
	i = NumerModlRessFloat(bn,3,"comp-car-v5", 0x27,16,RESS_COMP, "Volts", -4.5*g_compens*f_carre, 4.5*g_compens*f_carre);
	    NumerModlRessFloat(bn,3,"comp-car-v6", 0x28,16,RESS_COMP, "Volts", -4.5*g_compens*f_carre, 4.5*g_compens*f_carre);
	    NumerModlRessFloat(bn,3,"ampl-car-d9", 0x29,16,RESS_CORR, "Volts", -4.5*g_modul  *f_carre, 4.5*g_modul  *f_carre);
	    NumerModlRessFloat(bn,3,"ampl-car-d10",0x2A,16,RESS_CORR, "Volts", -4.5*g_modul  *f_carre, 4.5*g_modul  *f_carre);
	    NumerModlRessFloat(bn,3,"ampl-car-d11",0x2B,16,RESS_CORR, "Volts", -4.5*g_modul  *f_carre, 4.5*g_modul  *f_carre);
	    NumerModlRessFloat(bn,3,"ampl-car-d12",0x2C,16,RESS_CORR, "Volts", -4.5*g_modul  *f_carre, 4.5*g_modul  *f_carre);
	    NumerModlRessGroupeStatus(bn,i,status_carre,6);
	i = NumerModlRessInt  (bn,4,"retard-v5", 0x41,16,RESS_CORR, 0,0,65535);
	    NumerModlRessInt  (bn,4,"retard-v6", 0x42,16,RESS_CORR, 0,0,65535);
	    NumerModlRessInt  (bn,4,"retard-d9", 0x43,16,RESS_CORR, 0,0,65535);
	    NumerModlRessInt  (bn,4,"retard-d10",0x44,16,RESS_CORR, 0,0,65535);
	    NumerModlRessInt  (bn,4,"retard-d11",0x45,16,RESS_CORR, 0,0,65535);
	    NumerModlRessInt  (bn,4,"retard-d12",0x46,16,RESS_CORR, 0,0,65535);
	    NumerModlRessGroupeStatus(bn,i,status_retard,6);
		bn->nbadc = 6;
	for(i=0; i<bn->nbadc; i++) bn->adc[i] = adc_ab;
	bn->nbgains = bn->nbadc; for(i=0; i<bn->nbgains; i++) bn->gain[i] = 228.76;
	i = 0;
	strcpy(bn->proc[i++].nom,"demarrage");
	strcpy(bn->proc[i++].nom,"redemarrage");
	strcpy(bn->proc[i++].nom,"deblocage");
	strcpy(bn->proc[i++].nom,"blocage");
	strcpy(bn->proc[i++].nom,"croisiere");
	strcpy(bn->proc[i++].nom,"charge_DACs");
	bn->nbprocs = i;
	for(i=0; i<bn->nbprocs; i++) sprintf(bn->proc[i].fichier,"procedures_BB2/%s",bn->proc[i].nom);
	bn->nbenregs = 0;
	bn++;

	strcpy(bn->nom,"Bouchon");
	bn->version = 0.3;
	bn->a_debloquer = 0;
	bn->ident = 0;
	bn->nbress = 0;
	bn->nbadc = 1; bn->adc[0] = adc_ab + 1;
	bn->nbgains = bn->nbadc; for(i=0; i<bn->nbgains; i++) bn->gain[i] = 1.0;
	bn->nbenregs = 0;
	bn++;

#ifdef NUMER_EDW2
	strcpy(bn->nom,"DC782"); /* utilisee aux debuts de NOSTOS */
	bn->version = 0.2;
	bn->a_debloquer = 0;
	bn->ident = 0;
	bn->nbress = 0;
	bn->nbadc = 1; bn->adc[0] = adc_nostos;
	bn->nbgains = bn->nbadc; for(i=0; i<bn->nbgains; i++) bn->gain[i] = 1.0;
	bn->nbprocs = 0;
	bn->nbenregs = 0;
	bn++;
#endif
	
#ifdef NIDAQ
	strcpy(bn->nom,"NI");
	bn->version = 1.0; // 6251.0;
	bn->a_debloquer = 0;
	bn->ident = 1;
	bn->nbress = 0;
	NumerModlRessInt  (bn,0,"bloc", 0xFF,18,RESS_DIVERS,"echantillons",0,262143);
	bn->nbadc = 8; for(i=0; i<bn->nbadc; i++) {
		char nom[16];
		bn->adc[i] = adc_ni;
		sprintf(nom,"polar-v%d",i+1);
		NumerModlRessFloat(bn,1,nom,0xFF,16,RESS_POLAR,"Volts",-10.0,10.0);
		sprintf(nom,"source-v%d",i+1);
		NumerModlRessCles (bn,0,nom,0xFF, 2,RESS_DIVERS,"FS/GS","floating/ground-ref","0/1");
	}
	bn->nbgains = bn->nbadc; for(i=0; i<bn->nbgains; i++) bn->gain[i] = 1.0;
	bn->nbprocs = 0;
	bn->nbenregs = 0;
	bn++;
#endif
	
	printf("  . %d types de numeriseurs\n",bn-ModeleBN);
//	ModeleBNNb = bn - &(ModeleBN[0]);
	ModeleBNNb = bn - ModeleBN;

	NumerModlRessFinaliseStatus();
	
}
/* ========================================================================== */
char NumeriseursModlLit(char *fichier) {
	FILE *f; char nom_complet[MAXFILENAME];
	
	if(fichier) {
		strcpy(NumeriseurFichierModeles,fichier);
		RepertoireIdentique(NumeriseurFichierLu,NumeriseurFichierModeles,nom_complet);
	} else {
		NumeriseurFichierModeles[0] = '\0';
		strcpy(nom_complet,FichierModlNumeriseurs);
	}
	if((f = fopen(nom_complet,"r"))) {
		if(SambaLogDemarrage) printf("  . Lecture des modeles de numeriseurs  dans '%s'\n",nom_complet);
		//- ArgDebug(2);
		ArgFromFile(f,NumerModlDesc,0);
		//- ArgDebug(0);
		fclose(f);
		NumeriseursModlProcedures(nom_complet);
	} else {
		printf("  ! Fichier %s inutilisable (%s). Modeles standard utilises\n",nom_complet,strerror(errno));
		return(0);
	}
	/* printf("  * Fichier des modeles = {\n"); ArgIndent(1); ArgAppend(stdout,0,NumerModlDesc); printf("  }\n"); */

	return(1);
}
/* ========================================================================== */
void NumeriseurModlAccesInit(void *bloc) {
	TypeAccesModl *acces;

	// mode = binaire, format = "8:8:16", variables = ( ident, adrs, valeur )
	acces = (TypeAccesModl *)bloc;
	//- printf("(%s) Initialisation d'un acces\n",__func__);
	acces->mode = NUMER_ACCES_BB;
	strcpy(acces->fmt,"8:8:16");
	acces->nb = 3;
	acces->var[0].cle = NUMER_VAR_IDENT;
	acces->var[1].cle = NUMER_VAR_ADRS;
	acces->var[2].cle = NUMER_VAR_VAL;
}
/* ========================================================================== */
void NumeriseurModlInit(void *bloc) {
	TypeBNModele *modele_bn;

	// mode = binaire, format = "8:8:16", variables = ( ident, adrs, valeur )
	modele_bn = (TypeBNModele *)bloc;
	//- printf("(%s) Initialisation d'un modele de numeriseur\n",__func__);
	modele_bn->acces.mode = NUMER_ACCES_BB;
	strcpy(modele_bn->acces.fmt,"8:8:16");
	modele_bn->acces.nb = 3;
	modele_bn->acces.var[0].cle = NUMER_VAR_IDENT;
	modele_bn->acces.var[1].cle = NUMER_VAR_ADRS;
	modele_bn->acces.var[2].cle = NUMER_VAR_VAL; 
}
/* ========================================================================== */
void NumeriseursModlProcedures(char *chemin_modeles) {
	int i,j; char tempo[MAXFILENAME];

	for(i=0; i<ModeleBNNb; i++) {
		for(j=0; j<ModeleBN[i].nbprocs; j++) {
			RepertoireIdentique(chemin_modeles,ModeleBN[i].proc[j].fichier,tempo);
			strcpy(ModeleBN[i].proc[j].fichier,tempo);
		}
	}
}
/* ========================================================================== */
void NumeriseursModlEpilogue() {
	int i,j,k,n; char avec_status;
	TypeBNModele *modele_bn; TypeBNModlRessource *ress;

	if(CompteRendu.modeles) printf("  . Examen des modeles de numeriseurs\n");
	for(i=0; i<= NUMER_BB_VSNMAX; i++) NumeriseurTypeBB[i] = -1;
	NumeriseurTypeCALI = -1;

	modele_bn = &(ModeleBN[0]);
	for(i=0; i<ModeleBNNb; i++,modele_bn++) {
		modele_bn->num = i;
		modele_bn->fpga -= 1;

	/* NumerModlRessVerifie */
		for(k=0; k<modele_bn->nbress; k++) {
			ress = modele_bn->ress + k;
			if((ress->type != RESS_INT) && (ress->masque == 0)) {
				ress->masque = 1;
				printf("    ! %s.%s.masque trouve =0, mis a 1\n",modele_bn->nom,ress->nom);
			}
			if(ress->type == RESS_INT) {
				if(ress->lim_i.max == ress->lim_i.min) {
					ress->lim_i.max = ress->lim_i.min + 1;
					printf("    ! %s.%s.max = min=%d, mis a %d\n",modele_bn->nom,ress->nom,ress->lim_i.min,ress->lim_i.max);
				}
			} else if(ress->type == RESS_FLOAT) {
				if(ress->lim_r.max == ress->lim_r.min) {
					ress->lim_r.max = ress->lim_r.min + 1.0;
					printf("    ! %s.%s.max = min=%g, mis a %g\n",modele_bn->nom,ress->nom,ress->lim_r.min,ress->lim_r.max);
				}
			}
		}

	/* NumerModlRessDefiniStatus */
		/* va savoir pourquoi, ce tableau a entretemps ete remis a 0.. (sans doute par ArgFromFile) */
		avec_status = 0;
		for(j=0; j<NUMER_STATUS_DIM; j++) modele_bn->numress[j] = -1;
		for(j=0; j<NUMER_RAPPORT_NB; j++) modele_bn->rapport[j] = -1;
		for(k=0; k<modele_bn->nbress; k++) {
			j = modele_bn->ress[k].status;  if((j >= 0) && (j < NUMER_STATUS_DIM)) { modele_bn->numress[j] = k; avec_status = 1; }
			j = modele_bn->ress[k].rapport; if((j > 0) && (j < NUMER_RAPPORT_NB))    modele_bn->rapport[j] = k;
		}
		modele_bn->avec_status = avec_status;
		n = 0;
		if((modele_bn->nbenregs == 1) && !strcmp(&(modele_bn->enreg_nom[j][0]),"*")) {
			for(k=0; k<modele_bn->nbress; k++) modele_bn->enreg_sts[n++] = modele_bn->ress[k].status;
		} else {
			for(j=0; j<modele_bn->nbenregs; j++) {
				for(k=0; k<modele_bn->nbress; k++) if(!strcmp(modele_bn->ress[k].nom,&(modele_bn->enreg_nom[j][0]))) break;
				if(k < modele_bn->nbress) modele_bn->enreg_sts[n++] = modele_bn->ress[k].status; // modele_bn->ress[k].enreg = n++;
				else printf("    ! Enregistrement de %s dans un numeriseur de type %s: ressource inconnue\n",&(modele_bn->enreg_nom[j][0]),modele_bn->nom);
			}
		}
		modele_bn->enreg_dim = n;
		/* printf("(%s) ========== Numeriseur de type %s ==========\n",__func__,modele_bn->nom);
		 for(k=0; k<modele_bn->nbress; k++) printf("(%s) Colonne du rapport pour la ressource #%d (%s): #%d/%d\n",__func__,k,modele_bn->ress[k].nom,modele_bn->ress[k].rapport,NUMER_RAPPORT_NB);
		 for(j=0; j<NUMER_RAPPORT_NB; j++) printf("(%s) Colonne %d du rapport (%s): ressource #%d\n",__func__,j+1,NumeriseurRapportItem[j],modele_bn->rapport[j]); */

	/* NumeriseurIdentifieTypes */
		if(!strncmp(modele_bn->nom,"BB1",3)) NumeriseurTypeBB[0] = NumeriseurTypeBB[1] = i;
		else if(!strncmp(modele_bn->nom,"BB2",3)) NumeriseurTypeBB[2] = i;
		else if(!strncmp(modele_bn->nom,"BB3",3)) NumeriseurTypeBB[3] = i;
		else if(!strncmp(modele_bn->nom,"AdcCali",7)) NumeriseurTypeCALI = i;

	}

	if(CompteRendu.modeles) {
		printf("    . Boite Bolo");
		for(i=1; i<= NUMER_BB_VSNMAX; i++) {
			if(i > 1) printf(",");
			printf(" version %d: %s",i,(NumeriseurTypeBB[i] >= 0)? ModeleBN[NumeriseurTypeBB[i]].nom: "pas definie");
		}
		printf("\n");
		printf("    . Numeriseur type CALI: %s\n",(NumeriseurTypeCALI >= 0)? ModeleBN[NumeriseurTypeCALI].nom: "pas defini");
	}
	
	/* NumerModlRessImprimeStatus */
	if(CompteRendu.numer.sauveg) {
		modele_bn = &(ModeleBN[0]);
		for(i=0; i<ModeleBNNb; i++,modele_bn++) {
			printf("    . Sauvegarde des ressources des %-8s:",modele_bn->nom);
			if(modele_bn->enreg_dim) {
				for(n=0; n<modele_bn->enreg_dim; n++) {
					if(!(n % 10)) printf("\n      | "); else printf(", ");
					printf(modele_bn->ress[modele_bn->numress[modele_bn->enreg_sts[n]]].nom);
				}
				printf("\n");
			} else printf(" neant\n");
		}
	}
	
/*	printf("    . Acces:");
	if(AccesNb) {
		char texte[20];
		for(i=0; i<AccesNb; i++) {
			ArgKeyGetText(ACCES_FORMATS,AccesModl[i].mode,texte,20);
			if(i) printf("      |"); else if(AccesNb > 1) printf("\n      |");
			printf(" %16s: format %s",AccesModl[i].nom,texte);
			if(AccesModl[i].mode == NUMER_ACCES_TXT) printf(" = (\"%s\",",AccesModl[i].fmt);
			else if(AccesModl[i].mode == NUMER_ACCES_BIN) printf(" = <");
			for(j=0; j<AccesModl[i].nb; j++) {
				if(j) printf(",");
				ArgKeyGetText(ACCES_VARIABLES,AccesModl[i].var[j],texte,20);
				printf("%s",texte);
			}
			if(AccesModl[i].mode == NUMER_ACCES_TXT) printf(");");
			else if(AccesModl[i].mode == NUMER_ACCES_BIN) printf(">");
			printf("\n"); 
		}
	} else printf(" aucun defini\n"); */

	// ArgPrint("*",NumerModlDesc);
}
/* ========================================================================== */
void NumeriseursModlAssure() {
	char nom_complet[MAXFILENAME];

	// printf("(%s) appel avec %d modele%s\n",__func__,Accord1s(ModeleBNNb));
	if(!ModeleBNNb) {
		NumerModlStandard();
		strcpy(NumeriseurFichierModeles,"ModelesNumeriseurs");
		RepertoireIdentique(NumeriseurFichierLu,NumeriseurFichierModeles,nom_complet);
		// ArgPrint(nom_complet,NumerModlDesc);
		// printf("(%s) termine avec %d modele%s\n",__func__,Accord1s(ModeleBNNb));
		NumeriseursModlProcedures(nom_complet);
		NumeriseursModlEpilogue();
	}
}

#pragma mark --- materiel utilise ---
/* .......................................................................... */
/*                        M A T E R I E L   U T I L I S E                     */
/* ========================================================================== */
static void NumeriseursDecodeDef(char *modl, char *ligne, TypeNumeriseurDef *def, char *fin) {
	/* syntaxe: <#serie> [ '@' <adrs> ] [ 'x' <gain_v0> ... ] [ '>' <fibre> ] [ '{' ]
	   avec le modele deja extrait */
	int type_bn,cap;
	shex serial,adrs; /* 0xFFFF: pas de num. de serie */
	char *item; char *r,d;

	def->fibre[0] = '\0';
	if(modl[0]) {
		for(type_bn=0; type_bn<ModeleBNNb; type_bn++) if(!strcmp(modl,ModeleBN[type_bn].nom)) break;
		if(type_bn >= ModeleBNNb) {
			type_bn = 0;
			printf("  ! Type de numeriseur inconnu: '%s'. Remplace par '%s'\n",modl,ModeleBN[type_bn].nom);
		}
	} else type_bn = 0;
	if(ModeleBN[type_bn].nbgains) {
		for(cap=0; cap<ModeleBN[type_bn].nbgains; cap++) def->gain[cap] = ModeleBN[type_bn].gain[cap];
		def->nbgains = ModeleBN[type_bn].nbgains;
	}
	r = ligne;
	item = ItemAvant("@x>{",&d,&r);
	if(((*item == '*') || !strcmp(item,"0") || !strcmp(item,"00")) && (BNserial > 0)) serial = BNserial++;
	else { sscanf(item,"%hd",&serial); if(serial == 0) serial = 0xFFFF; }
	if(d == '@') {
		item = ItemAvant("x>{",&d,&r);
		sscanf(item,"%hx",&adrs);
		printf("(%s) Adresse speciale pour %s#%d: %x\n",__func__,ModeleBN[type_bn].nom,serial,adrs);
	} else adrs = serial;
	if(d == 'x') {
		cap = 0;
		while(*r && (d != '>') && (d != '{')) {
			item = ItemAvant(">{,",&d,&r);
			if(cap < NUMER_ADC_MAX) sscanf(item,"%g",&(def->gain[cap++]));
		}
		def->nbgains = cap;
	}
	if(d == '>') {
		strncpy(def->fibre,ItemJusquA('{',&r),FIBRE_NOM); def->fibre[FIBRE_NOM-1] = '\0';
	}
	def->type = type_bn;
	def->serial = (serial & 0xFF);
	def->adrs = (adrs & 0xFF);
	// printf("(%s) La %s#%d est a l'adresse %x\n",__func__,ModeleBN[type_bn].nom,def->serial,def->adrs);
	*fin = d;
}
/* ========================================================================== */
static void NumeriseursCreeDesc(TypeNumeriseur *numeriseur) {
	TypeBNModele *modele_bn; TypeBNModlRessource *ress; int k;
	ArgDesc *desc;

	if(numeriseur->desc) free(numeriseur->desc);
	if((numeriseur->def.type < 0) || (numeriseur->def.type >= NUMER_TYPES)) numeriseur->def.type = 0;
	modele_bn = &(ModeleBN[numeriseur->def.type]);
	numeriseur->desc = ArgCreate(modele_bn->nbress + 4);
	desc = numeriseur->desc;
	for(k=0; k<modele_bn->nbress; k++) {
		numeriseur->ress[k].init = 0;
		ress = &(modele_bn->ress[k]);
		switch(ress->type) {
			case RESS_INT:
			case RESS_PUIS2:
				ArgAdd(desc++,ress->nom,DESC_INT &(numeriseur->ress[k].val.i),ress->unite);
				break;
			case RESS_FLOAT:
				ArgAdd(desc++,ress->nom,DESC_FLOAT &(numeriseur->ress[k].val.r),ress->unite);
				break;
			case RESS_CLES:
				ArgAdd(desc++,ress->nom,DESC_KEY ress->lim_k.cles,&(numeriseur->ress[k].val.i),ress->unite);
				break;
			default:
				ArgAdd(desc++,ress->nom,DESC_NONE);
				break;
		}
	}
	ArgAdd(desc++,"gain",DESC_FLOAT_SIZE(numeriseur->def.nbgains,NUMER_ADC_MAX) numeriseur->def.gain,0);
	ArgAdd(desc++,"ident",DESC_SHEX_SIZE(numeriseur->nbident,NUMER_ADC_MAX) numeriseur->adc_id,0);
	ArgAdd(desc++,"corrections",DESC_STRUCT_SIZE(NumeriseurNbCorr,NUMER_RESS_MAX) &NumeriseurCorrAS,0);
	ArgAdd(desc++,"procedures",DESC_STRUCT_SIZE(NumeriseurProcsNb,NUMER_PROC_MAX) &NumeriseurProcAS,0);
}
/* ========================================================================== */
static void NumeriseurRaz(TypeNumeriseur *numeriseur, TypeBNModele *modele_bn, byte adrs) {
	int i,k;
	
	if(numeriseur->def.serial == 0xFF) snprintf(numeriseur->nom,NUMER_NOM,"%s",modele_bn->nom);
	else {
		snprintf(numeriseur->nom,NUMER_NOM,"%s#%02d",modele_bn->nom,numeriseur->def.serial);
		if(numeriseur->def.serial >= modele_bn->suivant) modele_bn->suivant = numeriseur->def.serial + 1;
	}
	numeriseur->version = modele_bn->version;
	numeriseur->bloque = modele_bn->a_debloquer;
	numeriseur->verrouille = 1; /* a priori */
	numeriseur->sel = 0;
	numeriseur->exec.inst = 0; numeriseur->exec.boucle = 0; numeriseur->exec.date = -1;
	numeriseur->status.demande = numeriseur->status.a_copier = 0;
	numeriseur->avec_status = modele_bn->avec_status;
	numeriseur->vsnid = modele_bn->ident;
	if(numeriseur->vsnid == 0) numeriseur->ident = 0;
	else if(numeriseur->vsnid == 1) numeriseur->ident = adrs;
	else numeriseur->ident = adrs | ((numeriseur->vsnid << 8) & 0x0F00);
	// printf("  * La %s @%x (vsnid %d) recoit l'identifieur %x\n",numeriseur->nom,adrs,numeriseur->vsnid,numeriseur->ident);
	numeriseur->nbadc = modele_bn->nbadc;
	numeriseur->nbident = numeriseur->nbadc;
	numeriseur->change_id = 0;
	numeriseur->voie = (short *)malloc(numeriseur->nbadc * sizeof(short));
	for(k=0; k<numeriseur->nbadc; k++) {
		if(numeriseur->vsnid <= 1) switch(k) {
			case 0:  numeriseur->adc_id[k] = numeriseur->ident; break;
			case 1:  numeriseur->adc_id[k] = (numeriseur->vsnid)? 6: 0; break;
			default: numeriseur->adc_id[k] = 0;
		} else numeriseur->adc_id[k] = (k < 5)? ((((k < 4)? (k + 2): 1) << 12) &  0xF000) | numeriseur->ident: 0;
		numeriseur->voie[k] = -1;
		// printf("  ! L'ADC #%d de la %s n'est pas encore connecte\n",k+1,numeriseur->nom);
	}
	if(!numeriseur->def.nbgains) {
		numeriseur->def.nbgains = modele_bn->nbgains; 
		for(k=0; k<numeriseur->def.nbgains; k++) numeriseur->def.gain[k] = modele_bn->gain[k];
	}
	for(k=0; k<modele_bn->nbress; k++) {
		numeriseur->ress[k].correctif = 1.0;
		numeriseur->ress[k].regl = 0;
	}
	numeriseur->nbprocs = modele_bn->nbprocs;
	for(i=0; i<modele_bn->nbprocs; i++) memcpy(&(numeriseur->proc[i]),&(modele_bn->proc[i]),sizeof(TypeModeleBnProc));
	numeriseur->proc[numeriseur->nbprocs].nom[0] = '\0';
	numeriseur->desc = 0;
	NumeriseursCreeDesc(numeriseur);
	/* affectation des ressources */
	NumeriseurRessDefaut(numeriseur,modele_bn);
	numeriseur->enreg_txt[0] = '\0';
}
/* ========================================================================== */
void NumeriseurNeuf(TypeNumeriseur *numeriseur, int type, shex ident, TypeRepartiteur *repart) {
	TypeBNModele *modele_bn; int k; byte adrs;

	numeriseur->bb = -1;
	adrs = (ident & 0xFF);
	numeriseur->def.adrs = numeriseur->def.serial = adrs;
	numeriseur->def.nbgains = 0;
	numeriseur->change_id = 0;
	numeriseur->local	= 0;
	numeriseur->fischer = 0;
	numeriseur->bloque = numeriseur->verrouille = 0; /* a priori, mais... */
	strcpy(numeriseur->status.lu,"neant");
	numeriseur->status.stamp = 0;
	numeriseur->fichier[0] = '\0';
	if(type >= 0) {
		numeriseur->def.type = type;
		modele_bn = &(ModeleBN[type]);
		NumeriseurRaz(numeriseur,modele_bn,adrs);
		if(numeriseur->def.serial == 0xFF) 
			sprintf(numeriseur->fichier,"%s",modele_bn->nom);
		else sprintf(numeriseur->fichier,"%s_%02d",modele_bn->nom,numeriseur->def.serial);
		for(k=0; k<modele_bn->nbress; k++) {
			numeriseur->ress[k].hval = modele_bn->ress[k].defaut;
			NumeriseurRessHexaChange(numeriseur,k);
			numeriseur->ress[k].init = modele_bn->ress[k].init;
			if((modele_bn->ress[k].categ == RESS_MODUL) 
			|| (modele_bn->ress[k].categ == RESS_COMP)
			|| (modele_bn->ress[k].categ == RESS_CORR)
			|| (modele_bn->ress[k].categ == RESS_AUXI)) {
				if(modele_bn->ress[k].type == RESS_FLOAT) numeriseur->ress[k].val.r = 0.0;
				else numeriseur->ress[k].val.i = 0;
				NumeriseurRessValChangee(numeriseur,k);
			}
		}
		printf("  + Numeriseur cree: %s\n",numeriseur->nom);
	} else {
		strcpy(numeriseur->nom,"inconnu");
		numeriseur->avec_status = 0;
		numeriseur->vsnid = (ident & 0xF00) >> 8;
		numeriseur->ident = ident;
	}
	numeriseur->repart = repart;
	if(repart) strcpy(numeriseur->code_rep,repart->code_rep); // provisoire peut-etre
	else strcpy(numeriseur->code_rep,"---");
}
/* ========================================================================== */
FILE *NumeriseurDefLit(char *deflue, TypeNumeriseurDef *def, char *nom_local, char *fichier_bb, char *fin) {
	FILE *g; char ligne_bb[256],*r,d,*item,*fibre; int cap;

	g = 0; nom_local[0] = '\0'; *fin = '\0';
	for(cap=0; cap<NUMER_ADC_MAX; cap++) def->gain[cap] = 1.0;
	def->gain[0] = 1000.0; def->gain[1] = 40.0; def->gain[2] = 40.0;  def->nbgains = 3;
	r = deflue;
	item = ItemAvant("#@",&d,&r);
	if(d == '@') {
		item = ItemJusquA('>',&r); fibre = r;
		strcpy(nom_local,item);
		RepertoireIdentique(NumeriseurFichierLu,nom_local,fichier_bb);
		if((g = fopen(fichier_bb,"r"))) {
			while(LigneSuivante(ligne_bb,256,g)) {
				if((ligne_bb[0] == '#') || (ligne_bb[0] == '\n')) continue;
				r = ligne_bb; item = ItemJusquA('#',&r);
				break;
			}
		} else {
			printf("  ! Fichier de numeriseur illisible: %s (%s)\n",fichier_bb,strerror(errno));
			return(0);
		}
	}
	NumeriseursDecodeDef(item,r,def,fin);
	if(d == '@') {
		strncpy(def->fibre,ItemJusquA(' ',&fibre),FIBRE_NOM); def->fibre[FIBRE_NOM-1] = '\0';
	}

	return(g);
}
/* ========================================================================== */
static void NumeriseurRessDefaut(TypeNumeriseur *numeriseur, TypeBNModele *modele_bn) {
	/* numeriseur->def doit etre renseigne (a cause de NumeriseurRessHexaChange) */
	int k;
	// printf("  * Valeurs par defaut pour %s[%d] type %s\n",numeriseur->nom,modele_bn->nbress,modele_bn->nom);
	for(k=0; k<modele_bn->nbress; k++) {
		numeriseur->ress[k].hval = modele_bn->ress[k].defaut;
		NumeriseurRessHexaChange(numeriseur,k);
	}
}
/* ========================================================================== */
static char NumeriseurRessLit(TypeNumeriseur *numeriseur, TypeBNModele *modele_bn, FILE *h) {
	int i,k,m; char deja_sauve,log; TypeBNModlRessource *ress;

	log = 0;
	if(log) printf("  * %s lu dans le fichier '%s'\n",numeriseur->nom,numeriseur->fichier);
	deja_sauve = 0;
	ArgFromFile(h,numeriseur->desc,"}");
	if((numeriseur->def.nbgains == 0) && modele_bn->nbgains) numeriseur->def.nbgains = modele_bn->nbgains;
	if(log >= 2) {
		printf("    = {\n");
		ArgIndent(3); ArgPrint("*",numeriseur->desc); ArgIndent(0);
		printf("    }\n");
	} 
	for(k=0; k<modele_bn->nbress; k++) {
		ress = modele_bn->ress + k;
		if(ress->type == RESS_INT) {
			if(ress->lim_i.max != ress->lim_i.min) numeriseur->ress[k].rval_to_hex = (float)ress->masque / (ress->lim_i.max - ress->lim_i.min);
			else numeriseur->ress[k].rval_to_hex = (float)ress->masque;
			if(ress->masque > 0) numeriseur->ress[k].hex_to_rval = (ress->lim_i.max - ress->lim_i.min) / (float)ress->masque;
			else numeriseur->ress[k].hex_to_rval = (ress->lim_i.max - ress->lim_i.min) / 65536.0;
		} else if(ress->type == RESS_FLOAT) {
			// printf("(%s) %s: [%g .. %g] -> %04X\n",__func__,ress->nom,ress->lim_r.min,ress->lim_r.max,ress->masque);
			if(ress->lim_r.max != ress->lim_r.min) numeriseur->ress[k].rval_to_hex = (float)ress->masque / (ress->lim_r.max - ress->lim_r.min);
			else numeriseur->ress[k].rval_to_hex = (float)ress->masque;
			if(ress->masque > 0) numeriseur->ress[k].hex_to_rval = (ress->lim_r.max - ress->lim_r.min) / (float)ress->masque;
			else numeriseur->ress[k].hex_to_rval = (ress->lim_r.max - ress->lim_r.min) / 65536.0;
			// printf("(%s) soit rval_to_hex=%g et hex_to_rval=%g\n",__func__,numeriseur->ress[k].rval_to_hex,numeriseur->ress[k].hex_to_rval);
		}
		if(ArgItemLu(numeriseur->desc,k)) {
			NumeriseurRessValChangee(numeriseur,k);
			numeriseur->ress[k].init = 1;
			deja_sauve = 1;
		} else numeriseur->ress[k].init = 0;
		if(log) printf("    | Ressource %-13s : %s = %s %s [%04X]\n",
			numeriseur->ress[k].init? "initialisable": "inchangee",modele_bn->ress[k].nom,
			numeriseur->ress[k].cval,modele_bn->ress[k].unite,numeriseur->ress[k].hval);
	}
	/* supplement de procedures */
	m = modele_bn->nbprocs;
	for(i=0; i<NumeriseurProcsNb; i++) if(m < NUMER_PROC_MAX)
		memcpy(&(numeriseur->proc[m++]),&(NumeriseurProcsListe[i]),sizeof(TypeModeleBnProc));
	numeriseur->nbprocs = m;
	if(m < NUMER_PROC_MAX) numeriseur->proc[m].nom[0] = '\0';
	for(i=0; i<NumeriseurNbCorr; i++) {
		for(k=0; k<modele_bn->nbress; k++) if(!strcmp(NumeriseurCorrListe[i].nom,modele_bn->ress[k].nom)) break;
		if(k < modele_bn->nbress) numeriseur->ress[k].correctif = NumeriseurCorrListe[i].valeur;
		else printf("    ! Ressource a corriger inconnue (%s, %g)\n",NumeriseurCorrListe[i].nom,NumeriseurCorrListe[i].valeur);
	}
	if(log) {
		printf("    | Gains : (",numeriseur->nom);
		for(i=0; i<numeriseur->def.nbgains; i++) {
			if(i) printf(", ");
			printf("%g",numeriseur->def.gain[i]);
		}
		printf(")\n");
	}
	return(deja_sauve);
}
/* ========================================================================== 
static void NumeriseurRessImprime(int bb) {
	int k; TypeNumeriseur *numeriseur; TypeBNModele *modele_bn;
	
	numeriseur = &(Numeriseur[bb]);
	modele_bn = &(ModeleBN[numeriseur->def.type]);
	printf("  * Ressources %s (fichier %s)\n",numeriseur->nom,numeriseur->fichier);
	for(k=0; k<modele_bn->nbress; k++) {
		printf("    | %s = %s %s [%04X] (%s)\n",modele_bn->ress[k].nom,
			numeriseur->ress[k].cval,modele_bn->ress[k].unite,numeriseur->ress[k].hval,
			numeriseur->ress[k].init? "dans le fichier": "par defaut");
	}
}
   ========================================================================== */
void NumeriseurComplete(int bb, TypeBNModele *modele_bn, TypeCableConnecteur connecteur, 
	TypeNumeriseurDef *def, FILE *h, char *nom_local, char fin) {
	TypeNumeriseur *numeriseur;
	int k; char deja_sauve;

	numeriseur = &(Numeriseur[bb]);
	memcpy(&(numeriseur->def),def,sizeof(TypeNumeriseurDef));
	numeriseur->bb = bb;
	numeriseur->fischer = connecteur;
	if(CompteRendu.cablage.numer) printf("  . %s#%02d branche en %d\n",modele_bn->nom,numeriseur->def.serial,numeriseur->fischer);
	strncpy(numeriseur->fichier,nom_local,MAXFILENAME);
	numeriseur->repart = 0; strcpy(numeriseur->code_rep,"---");
	numeriseur->status.nb = 0;
	strcpy(numeriseur->status.lu,"neant");
	numeriseur->status.stamp = 0;
	NumeriseurRaz(numeriseur,modele_bn,def->adrs);
	if(fin == '{') deja_sauve = NumeriseurRessLit(numeriseur,modele_bn,h);
	else deja_sauve = 0;
	if(!deja_sauve) for(k=0; k<modele_bn->nbress; k++) numeriseur->ress[k].init = modele_bn->ress[k].init;
}
#ifdef A_TERMINER
/* ========================================================================== */
int NumeriseurLitSeulement(TypeNumeriseur *numeriseur) {
	int k,m;
	char nom_complet[MAXFILENAME];
	FILE *f;
	char ligne[256],*r,*item,*v,fin;

	RepertoireIdentique(NumeriseurFichierLu,Numeriseur[bb].fichier,nom_complet);
	if((f = fopen(nom_complet,"r"))) {
		while(LigneSuivante(ligne,256,f)) {
			if((ligne[0] == '#') || (ligne[0] == '\n')) continue;
			r = ligne;
			item = ItemJusquA('#',&r);
			NumeriseursDecodeDef(item,r,&def,&fin);
			ArgFromFile(f,Numeriseur[bb].desc,"}");
			for(k=0; k<modele_bn->nbress; k++) if(ArgItemLu(Numeriseur[bb].desc,k)) {
				NumeriseurRessValChangee(&(Numeriseur[bb]),k);
				Numeriseur[bb].ress[k].init = 1;
				deja_sauve = 1;
			};
			m = modele_bn->nbprocs;
			for(i=0; i<NumeriseurProcsNb; i++) if(m < NUMER_PROC_MAX)
				memcpy(&(Numeriseur[bb].proc[m++]),&(NumeriseurProcsListe[i]),sizeof(TypeModeleBnProc));
			Numeriseur[bb].nbprocs = m;
			if(m < NUMER_PROC_MAX) Numeriseur[bb].proc[m].nom[0] = '\0';
		}
		fclose(f);
	} else printf("  ! Fichier de numeriseur illisible: %s (%s)\n",nom_complet,strerror(errno));
	return(bb);
}
#endif
/* ========================================================================== */
char NumeriseurPlace(FILE *f, char *nom_local, TypeCableConnecteur connecteur) {
	int cap,bb;
	float r_modul,c_modul;
	TypeNumeriseurDef def; TypeBNModele *modele_bn;
	char ligne[256],*r,*item,fin;

	while(LigneSuivante(ligne,256,f)) {
		if((ligne[0] == '#') || (ligne[0] == '\n')) continue;
		r_modul = 100.0; c_modul = 22.0; /* actuellement pas definis a partir de la version 9!!! */
		for(cap=0; cap<NUMER_ADC_MAX; cap++) def.gain[cap] = 1.0;
		def.gain[0] = 1000.0; def.gain[1] = 40.0; def.gain[2] = 40.0;
		r = ligne;
		item = ItemJusquA('#',&r);
		NumeriseursDecodeDef(item,r,&def,&fin);
		modele_bn = &(ModeleBN[def.type]);
		bb = NumeriseurNb;
		Numeriseur[bb].local = 0;
		NumeriseurComplete(bb,modele_bn,connecteur,&def,f,nom_local,fin);
		NumeriseurNb++;
		NumeriseurTermineAjout();
		printf("  . Numeriseur integre dans la liste: '%s > %s' [%d/%d]\n",CablageEncodeConnecteur(connecteur),Numeriseur[bb].nom,bb+1,NumeriseurNb);
		break;
	}
	return(1);
}
/* ========================================================================== */
static int NumeriseursRelit(FILE *f, int demande) {
	int nb_erreurs;
	int cap,bb;
	shex serial,adrs;
	TypeCableConnecteur connecteur;
	char nom_local[MAXFILENAME],nom_complet[MAXFILENAME],fischer[80];
	float r_modul,c_modul;
	TypeNumeriseurDef def; TypeBNModele *modele_bn; TypeBNModlRessource *ress; int k;
	FILE *g,*h;
	char ligne[256],*r,*item,*v,fin;

	bb = (demande == -1)? NumeriseurNb: 0;
	nb_erreurs = 0;
	if(f) {
		if(demande == -1) printf("= Lecture des numeriseurs               dans '%s'\n",NumeriseurFichierLu);
		else printf("= Lecture du numeriseur #%3d             dans '%s'\n",demande+1,NumeriseurFichierLu);
		while(LigneSuivante(ligne,256,f)) {
			if((ligne[0] == '#') || (ligne[0] == '\n')) continue;
			if((demande == -1) && (NumeriseurNb >= NUMER_MAX)) {
				printf("  ! Deja %d numeriseurs lus! Numeriseurs suivants abandonnes\n",NumeriseurNb);
				++nb_erreurs;
				break;
			}
			/* version <= 8.27: [ <modele> '#' ] <serial> <connecteur> <adresse_hexa> [ '/' [ <Rmodul> <Cmodul> ] [ '/' <gain> { ','<gain> } ] ] 
			   version >= 9.0:   <connecteur> '>' <definition>  avec <definition>:
				 soit:   [ <modele> ] '#' <serial> [ '@' <adresse_hexa> ] [ 'x' <gain> { ',' <gain> } ] [ '{' / { <ressource> '=' <valeur> / } '}' ]
				 ou bien: @<fichier> avec fichier: 
					[ <modele> ] '#' <serial> [ '@' <adrs> ] [ 'x' <gain_v0> ... ] '{'
						<ressource> '=' <valeur>
					'}'
			*/
			r_modul = 100.0; c_modul = 22.0; /* actuellement pas definis a partir de la version 9!!! */
			g = 0; nom_local[0] = '\0';
			for(cap=0; cap<NUMER_ADC_MAX; cap++) def.gain[cap] = 1.0;
			def.gain[0] = 1000.0; def.gain[1] = 40.0; def.gain[2] = 40.0;
			r = ligne; item = ItemAvant("=/>@",&fin,&r);
			if((fin == '@') || (fin == '=')) /* on attend surtout la definition des modeles */ {
				char *repertoire;
				if(!strncmp(item,"modele",6)) {
					if(fin == '@') /* modeles dans un fichier separe */ {
						strcpy(NumeriseurFichierModeles,ItemSuivant(&r,' '));
						RepertoireIdentique(NumeriseurFichierLu,NumeriseurFichierModeles,nom_complet);
						repertoire = nom_complet;
						h = fopen(nom_complet,"r");
						if(!h) {
							printf("  ! Fichier %s inutilisable (%s). Modeles standard utilises\n",nom_complet,strerror(errno));
							continue;
						}
					} else {
						h = f; /* modeles inclus dans ce fichier */
						repertoire = NumeriseurFichierLu;
					}
					if((demande < 0) && (ModeleBNNb == 0)) {
						printf("  . Lecture des modeles dans '%s'\n",(h == f)? NumeriseurFichierLu: nom_complet);
						if(h == f) ArgFromFile(h,NumerModlDesc,"}");
						else { ArgFromFile(h,NumerModlDesc,0); fclose(h); }
						NumeriseursModlProcedures(repertoire);
						NumeriseursModlEpilogue();
					} else if(h == f) ArgSkipOnFile(h,NumerModlDesc,"}");
				} else if(!strncmp(item,"stock",5)) {
					strcpy(NumeriseurStock,ItemSuivant(&r,' '));
					RepertoireTermine(NumeriseurStock);
					RepertoireIdentique(NumeriseurFichierLu,NumeriseurStock,nom_complet);
					printf("  . Stock de numeriseurs dans '%s'\n",nom_complet);
				} else printf("  ! Syntaxe imprevue, ligne abandonnee: '%s %c ...'\n",item,fin);
				continue;
			} else if(fin == '>') /* format des versions 9 et posterieures */ {
				NumeriseursModlAssure();
				connecteur = CablageDecodeConnecteur(item);
				// printf("  > %s  > ... a brancher sur %d\n",r,connecteur);
				g = NumeriseurDefLit(r,&def,nom_local,nom_complet,&fin);
				if(g && (demande == bb)) printf("  . Fichier specifique: %s\n",nom_complet);
			} else /* format des versions anterieures a 8 incluses */ {
				NumeriseursModlAssure();
				v = ligne; item = ItemJusquA('#',&v);
				if(*v) {
					for(def.type=0; def.type<ModeleBNNb; def.type++)
						if(!strcmp(item,ModeleBN[def.type].nom)) break;
					if(def.type >= ModeleBNNb) {
						def.type = 0;
						printf("  ! Type de numeriseur inconnu: '%s'. Remplace par '%s'\n",item,ModeleBN[def.type].nom);
					}
				} else {
					def.type = 0;
					v = ligne;
				}
				sscanf(v,"%hd %s %hx",&serial,fischer,&adrs);
				connecteur = CablageDecodeConnecteur(fischer);
				if((serial == 0) && (BNserial > 0)) serial = BNserial++;
				def.serial = serial;
				if(adrs) def.adrs = (adrs & 0xFF); else def.adrs = def.serial;
				if(fin == '/') {
					if(*r) {
						item = ItemAvant("{/=",&fin,&r);
						if(item[0]) sscanf(item,"%g %g",&r_modul,&c_modul);
						def.nbgains = 0;
						while(*r && (fin != '{') && (def.nbgains < NUMER_ADC_MAX)) {
							item = ItemAvant("{,",&fin,&r);
							sscanf(item,"%g",&(def.gain[def.nbgains++]));
						}
					}
				}
			}
			modele_bn = &(ModeleBN[def.type]);
			if(g) h = g; else { h = f; NumeriseurListeExiste = 0; }
			/* nb registres = nb registres du modele (le modele est donc a definir avec sa liste de registres,
			   au lieu que les registres soient affectes a partir d'une liste globale de tous les registres possibles).
			   Le cablage doit relier un reglage de detecteur a un registre de numeriseur. */
			Numeriseur[bb].local = 0;
			if(demande == -1) {
				NumeriseurComplete(bb,modele_bn,connecteur,&def,h,nom_local,fin);
				//- if((k = NumerModlRessIndex(modele_bn,"Rmodul")) >= 0) { Numeriseur[bb].ress[k].val.r = r_modul; NumeriseurRessValChangee(&(Numeriseur[bb]),k); }
				//- if((k = NumerModlRessIndex(modele_bn,"Cmodul")) >= 0) { Numeriseur[bb].ress[k].val.r = c_modul; NumeriseurRessValChangee(&(Numeriseur[bb]),k); }
			} else if(demande == bb) {
				NumeriseurRessDefaut(&(Numeriseur[bb]),modele_bn);
				NumeriseurRessLit(&(Numeriseur[bb]),modele_bn,h);
			} else if(!g && (fin == '{')) {
				ArgDesc *desc,*var;
				printf("  . Ressources du numeriseur #%d ignorees\n",bb+1);
				desc = ArgCreate(modele_bn->nbress + 4);
				for(k=0, ress=modele_bn->ress, var=desc; k<modele_bn->nbress; k++) ArgAdd(var++,(ress++)->nom,DESC_NONE);
				ArgAdd(var++,"gain",DESC_NONE);
				ArgAdd(var++,"ident",DESC_NONE);
				ArgAdd(var++,"corrections",DESC_NONE);
				ArgAdd(var++,"procedures",DESC_NONE);
				ArgSkipOnFile(h,desc,"}");
				free(desc);
			}
			if(g) fclose(g);
			if(demande == -1) {
				if(modele_bn->fpga > 0) NumeriseurAcharger = 1;
				if(modele_bn->ident > 0) NumeriseurIdentifiable = 1;
				if(modele_bn->avec_status > 0) NumeriseurAvecStatus = 1;
				if(Numeriseur[bb].nbprocs > 0) NumeriseurAdemarrer = 1;
				NumeriseurNb++;
			} else if(demande == bb) break;
			bb++;
		}
		if(!feof(f) && (demande == -1)) {
			printf("  ! Lecture des numeriseurs en erreur (%s)\n",strerror(errno));
			nb_erreurs++;
		}
		fclose(f);
		if(demande == -1) {
			printf("  . %d numeriseur%s connu%s\n",Accord2s(NumeriseurNb));
			NumeriseurTermineAjout();
		}
	} else { /* Fichier des numeriseurs inaccessible */
		printf("! Fichier des numeriseurs illisible: '%s' (%s)\n",NumeriseurFichierLu,strerror(errno));
		nb_erreurs++;
	}
/*	for(bb=0; bb<NumeriseurNb; bb++) {
		int num; TypeNumeriseur *numeriseur;
		numeriseur = &(Numeriseur[bb]);
		modele_bn = &(ModeleBN[numeriseur->def.type]);
		for(num=0; num<modele_bn->nbress; num++) {
			ress = modele_bn->ress + num;
			if(ress->type == RESS_INT) {
				if(ress->lim_i.max != ress->lim_i.min) numeriseur->ress[num].rval_to_hex = (float)ress->masque / (ress->lim_i.max - ress->lim_i.min);
				else numeriseur->ress[num].rval_to_hex = (float)ress->masque;
				if(ress->masque > 0) numeriseur->ress[num].hex_to_rval = (ress->lim_i.max - ress->lim_i.min) / (float)ress->masque;
				else numeriseur->ress[num].hex_to_rval = (ress->lim_i.max - ress->lim_i.min) / 65536.0;
			} else if(ress->type == RESS_FLOAT) {
				printf("(%s) %s: [%g .. %g] -> %04X\n",__func__,ress->nom,ress->lim_r.min,ress->lim_r.max,ress->masque);
				if(ress->lim_r.max != ress->lim_r.min) numeriseur->ress[num].rval_to_hex = (float)ress->masque / (ress->lim_r.max - ress->lim_r.min);
				else numeriseur->ress[num].rval_to_hex = (float)ress->masque;
				if(ress->masque > 0) numeriseur->ress[num].hex_to_rval = (ress->lim_r.max - ress->lim_r.min) / (float)ress->masque;
				else numeriseur->ress[num].hex_to_rval = (ress->lim_r.max - ress->lim_r.min) / 65536.0;
				printf("(%s) soit rval_to_hex=%g et hex_to_rval=%g\n",__func__,numeriseur->ress[num].rval_to_hex,numeriseur->ress[num].hex_to_rval);
			}

		}
	} */

	return(nb_erreurs);
}
/* ========================================================================== */
void NumeriseurTermineAjout() {
	strcpy(Numeriseur[NumeriseurNb].nom,"neant");
	Numeriseur[NumeriseurNb].bb = NumeriseurNb;
	Numeriseur[NumeriseurNb+1].nom[0] = '\0';
}
/* ========================================================================== */
void NumeriseurStructInit() {
	int i,j,bb; NUMER_TRACE_TYPE t;
	
	for(i=0; i<NUMER_ADC_TYPES; i++) ModeleADCListe[i] = ModeleADC[i].nom; ModeleADCListe[i] = "\0";
	j = 0; FpgaCode[j++] = "neant"; for(i=0; i<NUMER_FPGA_TYPES; i++) FpgaCode[j++] = FpgaModl[i].code; FpgaCode[j++] = "\0";
	for(i=0; i<NUMER_TYPES; i++) ModeleNumerListe[i] = ModeleBN[i].nom;  ModeleNumerListe[i] = "\0";
	for(i=0; i<NUMER_TYPES; i++) {
		for(j=0; j<NUMER_STATUS_DIM; j++) ModeleBN[i].numress[j] = -1;
		for(j=0; j<NUMER_RAPPORT_NB; j++) ModeleBN[i].rapport[j] = -1;
		ModeleBN[i].suivant = 1;
	}
	ModeleADCNb = 0; ModeleBNNb = 0;
	NumeriseurNb = 0;
	NumeriseurDejaAutorise = -1;
	strcpy(NumeriseurStock,"./");
	ModlNumerChoisi = -1;
	NumeriseurListeExiste = 1;
	NumerDemarreFpga = 0; NumerDemarreModlActif = NumerDemarreGenlActif = 1; sprintf(NumerDemarreScript,"demarrage");
	NumeriseurAcharger = NumeriseurAdemarrer = NumeriseurIdentifiable = NumeriseurAvecStatus = 0;
	NumeriseurFichierModeles[0] = '\0';
	strcpy(NumeriseurProcedureDir,"procedures_BB2");
	//- sprintf(NumerDemarreReglages,"reset_bolo");
	
	for(bb=0; bb<NUMER_MAX; bb++) {
		bzero(&(Numeriseur[bb]),sizeof(TypeNumeriseur));
		NumeriseurListe[bb] = Numeriseur[bb].nom;
		for(i=0; i<NUMER_PROC_MAX; i++) Numeriseur[bb].nomproc[i] = Numeriseur[bb].proc[i].nom;
		Numeriseur[bb].nomproc[i] = "\0";
	}
	NumeriseurListe[bb] = "\0";
	for(bb=0; bb<NUMER_ANONM_MAX; bb++) {
		bzero(&(NumeriseurAnonyme[bb]),sizeof(TypeNumeriseur));
		snprintf(NumeriseurAnonyme[bb].nom,NUMER_NOM,"Entree #%d",bb);
		for(i=0; i<NUMER_PROC_MAX; i++) NumeriseurAnonyme[bb].nomproc[i] = NumeriseurAnonyme[bb].proc[i].nom;
		NumeriseurAnonyme[bb].nomproc[i] = "\0";
	}
	NumeriseursDiffusesNb = 0; for(bb=0; bb<NUMER_MAX; bb++) NumeriseurDiffuse[bb] = 0;

	for(t=0; t<NUMER_TRACE_NB; t++) NumeriseurTracer[t] = 0;
	NumeriseurTraceLarg = 8192;
	NumeriseurTraceHaut = 300;
	NumeriseurTraceVisu = 512;
	NumeriseurStatusHorloge[0] = 0.0; NumeriseurStatusHorloge[1] = 1.0;

	PanelMaxChamp(132);
	NumeriseurInstrNb = 0;
	NumeriseurChoix.bb = 0;
	NumeriseurChoix.oper = NUMER_ECHANGE;
	NumeriseurChoix.optn.codees = NumeriseurChoix.optn.brutes = NumeriseurChoix.optn.hexa = 1;
	NumeriseurChoix.optn.niv = 1;
	pNumeriseurChoix = PanelCreate(5);
	PanelList(pNumeriseurChoix,"Nom",NumeriseurListe,&NumeriseurChoix.bb,NUMER_NOM);
	PanelBool(pNumeriseurChoix,"Valeurs codees",&NumeriseurChoix.optn.codees);
	PanelBool(pNumeriseurChoix,"Valeurs brutes",&NumeriseurChoix.optn.brutes);
	PanelKeyB(pNumeriseurChoix,"Codage brutes","decimal/hexa",&NumeriseurChoix.optn.hexa,8);
	//	PanelInt (pNumeriseurChoix,"Niveau",&NumeriseurChoix.optn.niv);

	pNumerDemarre = PanelCreate(6); PanelColumns(pNumerDemarre,2);
	PanelBlank(pNumerDemarre);
	PanelItemColors(pNumerDemarre,PanelKeyB(pNumerDemarre,0,"-/+",&NumerDemarreModlActif,2),SambaJauneVertOrangeBleu,2);
	PanelItemColors(pNumerDemarre,PanelKeyB(pNumerDemarre,0,"-/+",&NumerDemarreGenlActif,2),SambaJauneVertOrangeBleu,2);
	PanelBool(pNumerDemarre,"FPGA inclus",&NumerDemarreFpga);
	PanelText(pNumerDemarre,"Script 'numeriseur' de demarrage",NumerDemarreScript,80);
	PanelList(pNumerDemarre,"Script general de reglages",SambaProcedure,&SambaProcsNum,80);

	pNumeriseurDeplace = PanelCreate(2);
	PanelList (pNumeriseurDeplace,"Numeriseur a deplacer",NumeriseurListe,&NumeriseurChoix.bb,NUMER_NOM);
	PanelListB(pNumeriseurDeplace,"Operation effectuee",NumeriseurOper,&NumeriseurChoix.oper,8);
}
/* ========================================================================== */
int NumeriseursLit(char *fichier) {
	int bb,i,l,nb_erreurs; char *nom; // NUMER_TRACE_TYPE t;
	FILE *f; char nom_complet[MAXFILENAME];

	strcpy(NumeriseurFichierLu,fichier);
	nom = rindex(NumeriseurFichierLu,SLASH); l = nom - NumeriseurFichierLu + 1;
	strncpy(NumeriseurRacine,NumeriseurFichierLu,l); NumeriseurRacine[l] = '\0';
	NumeriseurNb = 0;

	f = fopen(NumeriseurFichierLu,"r");
	if(!f && StartFromScratch) {
		int type_bb2,type_bouchon;
		printf("> Installe des numeriseurs dans '%s'\n",NumeriseurFichierLu);
		NumerModlStandard();
		strcpy(NumeriseurFichierModeles,"ModelesNumeriseurs");
		RepertoireIdentique(NumeriseurFichierLu,NumeriseurFichierModeles,nom_complet);
		ArgPrint(nom_complet,NumerModlDesc);
		type_bb2 = type_bouchon = 0;
		for(i=0; i<ModeleBNNb; i++) {
			if(!strncmp(ModeleBN[i].nom,"BB2",3)) type_bb2 = i;
			else if(!strncmp(ModeleBN[i].nom,"Bouchon",7)) type_bouchon = i;
		}
		bb = 0;
		NumeriseurNeuf(&(Numeriseur[bb]),type_bb2,1,0);
		Numeriseur[bb].fischer = 1; Numeriseur[bb].bb = bb; bb++;
		NumeriseurNeuf(&(Numeriseur[bb]),type_bouchon,1,0);
		Numeriseur[bb].fischer = 2; Numeriseur[bb].bb = bb; bb++;
		NumeriseurNb = bb;
		NumeriseurTermineAjout();
		NumeriseursEcrit(NumeriseurFichierLu,NUMER_TOUT,NUMER_TOUS);
		f = fopen(NumeriseurFichierLu,"r");
	}
	NumeriseurNb = 0;
	nb_erreurs = NumeriseursRelit(f,-1);

	bNumeriseurEtat = 0; iNumeriseurEtatMaj = 0;
	NumeriseurEtatDemande = 0;
	NumeriseurEtatStamp = 0;
	strcpy(NumeriseurEtatDate,"inconnu");
	for(bb=0; bb<NumeriseurNb; bb++) strcpy(NumeriseurEtat[bb].texte,"neant");
	
//	pNumeriseursDiffuses = PanelCreate(NumeriseurNb);
//	for(bb=0; bb<NumeriseurNb; bb++) {
//		i = PanelBool(pNumeriseursDiffuses,Numeriseur[bb].nom,&(NumeriseurDiffuse[bb]));
//			PanelItemColors(pNumeriseursDiffuses,i,SambaJauneVertOrangeBleu,2);
//			PanelItemSouligne(pNumeriseursDiffuses,i,0);
//	}

	return(nb_erreurs);
}
/* ========================================================================== */
void NumeriseursEcrit(char *fichier, char quoi, int qui) {
	/* quoi:  NUMER_CONNECTION (0): connectique
	          NUMER_RESSOURCES (1): ressources (on suppose que les gains fixes ne changent pas)
	          NUMER_TOUT       (2): les deux..
	   qui:   NUMER_TOUS (-1): tous
			  <bb>: un bb particulier */
	FILE *f,*g,*h; int bb,bn,i; 
	char nom_complet[MAXFILENAME];
	TypeBNModele *modele_bn;
	char connection,ressources,ecrit_def;
	char connecteur[8];

	g = 0; // GCC
	if(quoi == NUMER_RESSOURCES) {
		for(bb=0; bb<NumeriseurNb; bb++) if(((qui == NUMER_TOUS) || (bb == qui)) && (Numeriseur[bb].fichier[0] == '\0')) {
			printf("! La %s n'est pas definie dans un fichier propre: les connections seront aussi re-ecrites\n",Numeriseur[bb].nom);
			quoi = NUMER_TOUT; break;
		}
	};
	connection = ((quoi == NUMER_CONNECTION) || (quoi == NUMER_TOUT)); // soit (quoi != NUMER_RESSOURCES)
	ressources = ((quoi == NUMER_RESSOURCES) || (quoi == NUMER_TOUT)); // soit (quoi != NUMER_CONNECTION)
	printf("  + ");
	if(connection) { printf("Liste des numeriseurs"); if(quoi == NUMER_TOUT) printf(" et "); }
	if(ressources) {
		if(qui == NUMER_TOUS) printf("Reglages des numeriseurs");
		else printf("Reglages du numeriseur %s",Numeriseur[qui].nom);
	}
	f = 0;
	for(bb=0; bb<NumeriseurNb; bb++) {
		/* Erreur interne de manipulation? on garde la derniere version! */
		if(Numeriseur[bb].fichier[0]) {
			for(bn=bb+1; bn<NumeriseurNb; bn++) if(!strcmp(Numeriseur[bn].fichier,Numeriseur[bb].fichier)) break;
			if(bn < NumeriseurNb)  continue;
		}
		if(connection) {
			if(Numeriseur[bb].fischer == CABLAGE_CONN_DIR) continue;
			if(!f) {
				RepertoireCreeRacine(fichier);
				if((f = fopen(fichier,"w"))) {
					fprintf(f,"# Numeriseurs\n#\n");
					fprintf(f,"# syntaxe:    <connecteur> '>' ( <definition>  | '@' <fichier> [ '>' <fibre> ] ) -- fichier contenant la definition, relatif au repertoire contenant le present fichier\n");
					fprintf(f,"# definition:                  [ <modele> ] '#' <serial> [ '@' <adresse_hexa> ] [ 'x' <gain> { ',' <gain> } ] [ '>' <fibre> ] [ '{' / { <ressource '=' <valeur> / } '}' ]\n#\n");
					if(NumeriseurFichierModeles[0]) fprintf(f,"modeles @%s\n#\n",NumeriseurFichierModeles);
					if(strcmp(NumeriseurStock,"./")) fprintf(f,"stock @%s\n#\n",NumeriseurStock);
				} else {
					printf(" non sauvegardee (%s: %s)\n",fichier,strerror(errno));
					return;
				}
			}
			strcpy(connecteur,CablageEncodeConnecteur(Numeriseur[bb].fischer));
			fprintf(f,"%s > ",connecteur);
		}
		modele_bn = &(ModeleBN[(int)Numeriseur[bb].def.type]);
		g = 0;
		if(Numeriseur[bb].fichier[0]) {
			if(ressources && ((qui == NUMER_TOUS) || (bb == qui))) {
				RepertoireIdentique(fichier,Numeriseur[bb].fichier,nom_complet);
				RepertoireCreeRacine(nom_complet);
				if(!(g = fopen(nom_complet,"w"))) printf(" NON SAUVEGARDES dans %s (%s)\n! Reglages",nom_complet,strerror(errno));
				ecrit_def = 1;
			} else ecrit_def = 0;
			if(connection && (!ecrit_def || g)) {
				fprintf(f,"@%s",Numeriseur[bb].fichier);
				if(Numeriseur[bb].def.fibre[0]) fprintf(f," > %s\n",Numeriseur[bb].def.fibre); else fprintf(f,"\n");
			}
		} else if(connection) ecrit_def = 1; else ecrit_def = 0;
		if(g) h = g; else h = f;
		if(!h) continue;
		if(ecrit_def) {
			if(Numeriseur[bb].def.serial == 0xFF) 
				fprintf(h,"%s#",modele_bn->nom);
			else fprintf(h,"%s#%02d",modele_bn->nom,Numeriseur[bb].def.serial);
			if(Numeriseur[bb].def.serial != Numeriseur[bb].def.adrs) fprintf(h," @%02x",Numeriseur[bb].def.adrs);
			/*	if((nbadc = modele_bn->nbadc)) {
				fprintf(h," x");
				for(adc=0; adc<nbadc; adc++) {
					if(adc) fprintf(h,",");
					fprintf(h," %g",Numeriseur[bb].def.gain[adc]);
				}
			} */
			if(!g && Numeriseur[bb].def.fibre[0]) fprintf(h," > %s",Numeriseur[bb].def.fibre);
			if(Numeriseur[bb].desc) {
				int k,m;
				Numeriseur[bb].nbident = Numeriseur[bb].nbadc;
				for(k=0; k<modele_bn->nbress; k++) Numeriseur[bb].desc[k].lu = Numeriseur[bb].ress[k].init;
				ArgVarUtilise(Numeriseur[bb].desc,"gain",1);
				ArgVarUtilise(Numeriseur[bb].desc,"ident",1);
				NumeriseurNbCorr = 0;
				for(k=0; k<modele_bn->nbress; k++) if(Numeriseur[bb].ress[k].correctif != 1.0) {
					strcpy(NumeriseurCorrListe[NumeriseurNbCorr].nom,modele_bn->ress[k].nom);
					NumeriseurCorrListe[NumeriseurNbCorr].valeur = Numeriseur[bb].ress[k].correctif;
					NumeriseurNbCorr++;
				};
				ArgVarUtilise(Numeriseur[bb].desc,"corrections",(NumeriseurNbCorr > 0));
				m = modele_bn->nbprocs;
				for(i=0; m<Numeriseur[bb].nbprocs; i++)
					memcpy(&(NumeriseurProcsListe[i]),&(Numeriseur[bb].proc[m++]),sizeof(TypeModeleBnProc));
				NumeriseurProcsNb = i;
				ArgVarUtilise(Numeriseur[bb].desc,"procedures",(NumeriseurProcsNb > 0));
				fprintf(h," {\n");
				ArgIndent(1); ArgAppendModif(h,0,Numeriseur[bb].desc); ArgIndent(0);
				fprintf(h,"}");
			}
			fprintf(h,"\n");
			if(g) { fclose(g); if(quoi == NUMER_RESSOURCES) break; }
		}
	};
	if(connection) {
		if(quoi == NUMER_TOUT) printf(" resp.");
		if(f) {
			fclose(f);
			if(quoi == NUMER_TOUT) printf(" sauves dans:\n  "); else printf(" sauvee dans ");
			printf("'%s'\n",fichier);
		} else printf(" vide\n");
		if(quoi == NUMER_TOUT) printf("  et ");
	}
	if(ressources) {
		if(g) {
			if(!connection || !f) printf(" sauves dans ");
			if(qui == NUMER_TOUS) printf("les fichiers individuels\n");
			else printf("'../%s'\n",Numeriseur[qui].fichier);
		} else printf(" vide\n");
	}

}
/* ========================================================================== */
void NumeriseursModlImprime() {
	int i,j,k;
	char ligne_en_cours;
	
	printf(" ____________________________________________________________________________________\n");
	printf("|                                     Numeriseurs                                    |\n");
	printf("|____________________________________________________________________________________|\n");
	printf("| type             | ADC, registres   |                                              |\n");
	printf("|__________________|__________________|______________________________________________|\n");
	for(i=0; i<ModeleBNNb; i++) {
		printf("| %-16s ",ModeleBN[i].nom); ligne_en_cours = 1;
		for(j=0; j<ModeleBN[i].nbadc; j++) {
			if(!ligne_en_cours) printf("|                  "); ligne_en_cours = 0;
			k = ModeleBN[i].adc[j];
			if(ModeleADC[k].nom[0]) printf("| %-16s ",ModeleADC[k].nom);
			else printf("|                      ");
			printf("| %2d bits %-10s / %6.3fV |  %-5s        |\n",
				   ModeleADC[k].bits,ModeleADC[k].signe? "signes": "non signes",ModeleADC[k].polar,
				   /* ModeleADC[k].ident? "ident": */ "");
		}
#ifdef REMPLACER_PAR_RESSOURCES
		int cols,m,n;
		if(!ligne_en_cours && ModeleBN[i].nbregs) 
			printf("|                  |__________________|______________________________|_______________|\n");
		cols = 2;
		n = ((ModeleBN[i].nbregs - 1) / cols) + 1;
		for(j=0; j<n; j++) {
			if(!ligne_en_cours) printf("|                  "); ligne_en_cours = 0;
			for(m=j,k=0; m<ModeleBN[i].nbregs; m += n, k++) {
				printf("| %-16s | @%04X =%04X ",
					   ModeleBN[i].reg[m].nom,ModeleBN[i].reg[m].ss_adrs,ModeleBN[i].reg[m].valeur);
			}
			while(k++ < cols) printf("|                  |             ");
			printf("|\n");
		}
#endif
		if(ligne_en_cours) {
			printf("|                  |                                              |\n");
			printf("|__________________|__________________|______________________________________________|\n");
		} else printf("|__________________|__________________|_____________|__________________|_____________|\n");
	}
	printf("\n");
}
/* ========================================================================== */
short NumeriseurMaxVoies(TypeNumeriseur *numeriseur) {
	short vpn;
	shex d1; int voies_par_mot;
	TypeRepartiteur *repart;

	if(!numeriseur) return(0);
	vpn = 1;
	if(numeriseur->ident) {
		if((repart = numeriseur->repart)) {
			if(repart->famille == FAMILLE_OPERA) OperaDimensions(repart->r.opera.mode,&d1,&voies_par_mot,&vpn);
			else if(repart->famille == FAMILLE_IPE) vpn = IpeVpn[repart->r.ipe.modeBB[numeriseur->entree]]; 
			else repart = 0;
		}
		if(!repart) switch(numeriseur->vsnid) {
		  case 0: vpn = numeriseur->nbadc; break;
		  case 1: vpn = 3; break;
		  case 2: vpn = 6; break;
		  case 3: vpn = 5; break;
		};
	} else vpn = numeriseur->nbadc;
	// printf("    | %s(%s) = %d voie%s\n",__func__,numeriseur->nom,Accord1s(vpn));
	return(vpn);
}
/* ========================================================================== */
static void NumeriseurMontreModifs(TypeNumeriseur *numeriseur) {
	TypeNumeriseurCmde *interface;
	interface = &(numeriseur->interface);
	if(interface->planche && OpiumDisplayed(interface->planche)) {
		BoardRefreshVars(interface->planche);
	}
}
/* ========================================================================== */
void NumeriseursBB1Debloque(TypeNumeriseur *numeriseur, char toujours, char *prefixe) {
	int i;

	/* Tout ca pour les bidouilles a HD ... */
	/* DAC1 .. DAC4 (0x18 .. 0x1B) = 0x0000, DAC5 + DAC6 (0x1C, 0x1D) = 0x07FF */
	if(numeriseur->bloque || toujours) {
		printf("%s == Deblocage des DACs:\n",prefixe?prefixe:"");
		for(i=0; i<numeriseur->nbprocs; i++) if(!strcmp(numeriseur->proc[i].nom,"deblocage")) break;
		if(i < numeriseur->nbprocs) NumeriseurProcExec(numeriseur,i);
		else printf("%s  ! Procedure non definie pour la %s\n",prefixe?prefixe:"",numeriseur->nom);
		printf("%s == Fin de la sequence de deblocage des DACs\n",prefixe?prefixe:"");
		numeriseur->bloque = 0;
	}
	/* Fin bidouilles HD */
}
/* ========================================================================== */
void NumeriseursSelPrint() {
	int i,bb;
	
	i = 0; printf("          Numeriseurs selectionnes:");
	for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].sel) {
		if(i) printf(", ");
		if(!(i % 10)) printf("\n          | ");
		printf("%10s",Numeriseur[bb].nom); i++;
	}
	if(!i) printf(" aucun!"); printf("\n");
}
/* ========================================================================== */
int NumeriseursSelectionneLarge() { NumeriseurSelection(0); return(0); }
/* ========================================================================== */
int NumeriseursDebloqueTous() {
	int bb,i;

	if(!NumeriseurNb) { OpiumError("Pas de numeriseur connu"); return(0); }
	// if(OpiumExec(pNumeriseursDiffuses->cdr) == PNL_CANCEL) return(0);
	for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].sel) {
		for(i=0; i<Numeriseur[bb].nbprocs; i++) if(!strcmp(Numeriseur[bb].proc[i].nom,"deblocage")) break;
		if(i < Numeriseur[bb].nbprocs) {
			NumeriseurProcExec(&(Numeriseur[bb]),i);
			Numeriseur[bb].bloque = 0;
		}
	}
	SambaNote("Deblocage des boitiers numeriseurs\n");
	return(0);
}
/* ========================================================================== */
int NumeriseursRebloqueTous() {
	int bb,i;
	
	if(!NumeriseurNb) { OpiumError("Pas de numeriseur connu"); return(0); }
	// if(OpiumExec(pNumeriseursDiffuses->cdr) == PNL_CANCEL) return(0);
	for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].sel) {
		for(i=0; i<Numeriseur[bb].nbprocs; i++) if(!strcmp(Numeriseur[bb].proc[i].nom,"blocage")) break;
		if(i < Numeriseur[bb].nbprocs) {
			NumeriseurProcExec(&(Numeriseur[bb]),i);
			Numeriseur[bb].bloque = 1;
		}
	}
	SambaNote("Reblocage des boitiers numeriseurs\n");
	return(0);
}
/* ========================================================================== */
int NumeriseurAvecFibre(char *fibre) {
	int bb;
	
	if(fibre[0] == '\0') return(-1);
	for(bb=0; bb<NumeriseurNb; bb++) if(!strcmp(Numeriseur[bb].def.fibre,fibre)) break;
	if(bb < NumeriseurNb) return(bb); else return(-1);
}
/* ========================================================================== */
int NumeriseurSelection(char exclusive) {
	OrgaSelectionNumeriseurConstruit(exclusive);
	OrgaNumeriseursChoisis = 0;
	OpiumFork(bNumeriseurSelecte);
	until(OrgaNumeriseursChoisis) { OpiumUserAction(); usleep(100000); }
	return((OrgaNumeriseursChoisis > 0)? 1: 0);
}
/* ========================================================================== */
int NumeriseursInitTous() {
	int bb,k,nb; char log;
	TypeBNModele *modele_bn;
	
	if(!NumeriseurNb) { OpiumError("Pas de numeriseur connu"); return(0); }
	// if(OpiumExec(pNumeriseursDiffuses->cdr) == PNL_CANCEL) return(0);
	log = 1;
	if(log) printf("%s/ Initialisation des boitiers numeriseurs\n",DateHeure());
	for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].sel) {
		modele_bn = &(ModeleBN[(int)Numeriseur[bb].def.type]);
		nb = 0;
		for(k=0; k<modele_bn->nbress; k++) if(Numeriseur[bb].ress[k].init && (modele_bn->ress[k].ssadrs != 0xFF)) nb++;
		if(nb) NumeriseurCharge(&(Numeriseur[bb]),0,log);
	}
	if(log) printf("%s/ Initialisation terminee\n",DateHeure());
	SambaNote("Initialisation des boitiers numeriseurs\n");
	return(0);
}
/* ========================================================================== */
static void NumeriseurDeconnecte(TypeNumeriseur *numeriseur) {
	int vbb,voie,bolo; char explic[256];
	
	for(vbb=0; vbb<numeriseur->nbadc; vbb++) {
		if((voie = numeriseur->voie[vbb]) < 0) continue;
		numeriseur->voie[vbb] = -1;
		bolo = VoieManip[voie].det; if(bolo < 0) continue;
		DetecteurBrancheCapteur(&(Bolo[bolo]),VoieManip[voie].cap,-1,vbb,0xFF,explic);
	}
}
/* ========================================================================== */
static void NumeriseurReconnecte(TypeNumeriseur *numeriseur) {
	int bolo,cap,vbb,bb;
	TypeCablePosition pos_hexa;
	TypeCableConnecteur connecteur;
	shex adrs;
	char explic[256];
	
	if(numeriseur->fischer == 0) return;
	for(bolo=0; bolo<BoloNb; bolo++) {
		pos_hexa = Bolo[bolo].pos;
		for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
			if((connecteur = CablageSortant[pos_hexa].captr[cap].connecteur) != numeriseur->fischer) continue;
			vbb = CablageSortant[pos_hexa].captr[cap].vbb;
			bb = numeriseur->bb; adrs = numeriseur->def.adrs;
			if(!DetecteurBrancheCapteur(&(Bolo[bolo]),cap,bb,vbb,adrs,explic)) {
				printf("  ! %s...\n",explic);
				Bolo[bolo].lu = DETEC_HS;
			}
		}
	}
}
/* ========================================================================== */
int NumeriseursDemarre() {
	TypeNumeriseur *numeriseur; TypeRepartiteur *repart;
	int numproc,bb; char out; int nb_erreurs;
	char nom_complet[MAXFILENAME],message[256];

	if(!NumeriseurSelection(NumerDemarreFpga)) return(0);
	if(OpiumExec(pNumerDemarre->cdr) == PNL_CANCEL) return(0);
	if(!NumerDemarreFpga && !NumerDemarreModlActif && !NumerDemarreGenlActif) {
		OpiumError("Pas de procedure demandee");
		return(0);
	}
	nb_erreurs = 0;
	printf("%s/ Procedure de demarrage des numeriseurs\n",DateHeure());
	
	if(NumerDemarreFpga) {
		if((ModlNumerChoisi < 0) || (ModlNumerChoisi >= ModeleBNNb)) {
			printf("%s ! Modele de numeriseur choisi: %d/%d, illegal\n",DateHeure(),ModlNumerChoisi,ModeleBNNb);
			nb_erreurs++;
			goto fin_demarrage;
		}
		if(!RepartChargeFpga(message)) {
			printf("%s ! Chargement des FPGA en defaut, par consequent..\n",DateHeure());
			nb_erreurs++;
			goto fin_demarrage;
		}
	}

	if(NumerDemarreModlActif) for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].sel) {
		numeriseur = &(Numeriseur[bb]);
		out = 1; if((repart = Numeriseur[bb].repart)) { if(repart->local) out = 0 ; } if(out) continue;
		for(numproc=0; numproc<numeriseur->nbprocs; numproc++) if(!strcmp(Numeriseur[bb].proc[numproc].nom,NumerDemarreScript)) break;
		if(numproc < numeriseur->nbprocs) NumeriseurProcExec(numeriseur,numproc);
		else {
			printf("%s/ ! Le numeriseur %s n'a pas de procedure de type '%s'\n",DateHeure(),Numeriseur[bb].nom,NumerDemarreScript);
		   nb_erreurs++;
		}
	}
	if(NumerDemarreGenlActif) {
		if(bScriptCntl) {
			OpiumFork(bScriptCntl);
			strcat(strcpy(nom_complet,FichierPrefProcedures),SambaProcedure[SambaProcsNum]);
			printf("%s/ Execution du script '%s' ----------------------------\n",DateHeure(),SambaProcedure[SambaProcsNum]);
			if(ScriptExec(nom_complet,SambaProcedure[SambaProcsNum],"          "))
				printf("%s/ Script '%s' termine ---------------------------------\n",DateHeure(),SambaProcedure[SambaProcsNum]);
			else {
				printf("%s/ ! Execution du script general '%s' en erreur\n",DateHeure(),SambaProcedure[SambaProcsNum]);
				nb_erreurs++;
			}
		} else OpiumWarn("Aucun script n'a ete defini");
	}

fin_demarrage:
	printf("%s/ Fin de la procedure de demarrage des numeriseurs: ",DateHeure());
	if(nb_erreurs) {
		printf("%d erreur%s detectee%s\n",Accord2s(nb_erreurs));
		OpiumError("%d erreur%s detectee%s (voir journal)",Accord2s(nb_erreurs));
	} else printf("pas d'erreur detectee\n");
	return(0);
}
/* ========================================================================== 
int NumeriseurDeplace() {
	int autre_bb; TypeCableConnecteur nouveau_connecteur; char deja_connu;

	if(OpiumExec(pNumeriseurDeplace->cdr) == PNL_CANCEL) return(0);
	deja_connu = 0;
	switch(NumeriseurChoix.oper) {
	  case NUMER_ENTREE:
		if(Numeriseur[NumeriseurChoix.bb].fischer) {
			OpiumError("%s est deja branche sur le connecteur %d",Numeriseur[NumeriseurChoix.bb].nom,Numeriseur[NumeriseurChoix.bb].fischer);
			return(0);
		}
		nouveau_connecteur = Numeriseur[NumeriseurChoix.bb].fischer;
	#if (CABLAGE_CONNECT_MAX <= 256)
		if(OpiumReadByte("Sur le connecteur"CABLAGE_EXPLIC,&nouveau_connecteur) == PNL_CANCEL) return(0);
	#else
		if(OpiumReadShort("Sur le connecteur"CABLAGE_EXPLIC,&nouveau_connecteur) == PNL_CANCEL) return(0);
	#endif
		if(nouveau_connecteur == Numeriseur[NumeriseurChoix.bb].fischer) return(0);
		for(autre_bb=0; autre_bb<NumeriseurNb; autre_bb++) if(Numeriseur[autre_bb].fischer == nouveau_connecteur) break;
		if(autre_bb >= NumeriseurNb) {
			Numeriseur[NumeriseurChoix.bb].fischer = nouveau_connecteur;
			printf(". Le numeriseur %s est maintenant branche sur le connecteur %s\n",Numeriseur[NumeriseurChoix.bb].nom,CablageEncodeConnecteur(Numeriseur[NumeriseurChoix.bb].fischer));
			NumeriseurReconnecte(&(Numeriseur[NumeriseurChoix.bb]));
			break;
		} else {
			if(!OpiumChoix(2,SambaNonOui,"OUPS: il y a deja une %s a cet endroit! La retirer?",Numeriseur[autre_bb].nom)) return(0);
			deja_connu = 1;
		}
		break;
	  case NUMER_ECHANGE:
		if(!deja_connu) {
			autre_bb = NumeriseurChoix.bb;
			if(OpiumReadList("Avec le numeriseur",NumeriseurListe,&autre_bb,NUMER_NOM) == PNL_CANCEL) return(0);
			if(autre_bb == NumeriseurChoix.bb) return(0);
		}
		NumeriseurDeconnecte(&(Numeriseur[NumeriseurChoix.bb]));
		NumeriseurDeconnecte(&(Numeriseur[autre_bb]));
		nouveau_connecteur = Numeriseur[autre_bb].fischer;
		Numeriseur[autre_bb].fischer = Numeriseur[NumeriseurChoix.bb].fischer;
		Numeriseur[NumeriseurChoix.bb].fischer = nouveau_connecteur;
		if(Numeriseur[NumeriseurChoix.bb].fischer) printf(". Le numeriseur %s passe sur le connecteur %s\n",Numeriseur[NumeriseurChoix.bb].nom,CablageEncodeConnecteur(Numeriseur[NumeriseurChoix.bb].fischer));
		else printf(". Le numeriseur %s est maintenant debranche\n",Numeriseur[NumeriseurChoix.bb].nom);
		if(Numeriseur[autre_bb].fischer) printf(". Le numeriseur %s vient sur le connecteur %s\n",Numeriseur[autre_bb].nom,CablageEncodeConnecteur(Numeriseur[autre_bb].fischer));
		else printf(". Le numeriseur %s est maintenant debranche\n",Numeriseur[autre_bb].nom);
		NumeriseurReconnecte(&(Numeriseur[NumeriseurChoix.bb]));
		NumeriseurReconnecte(&(Numeriseur[autre_bb]));
		break;
	  case NUMER_SORTIE:
		printf(". Le numeriseur %s n'est PLUS sur le connecteur %s\n",Numeriseur[NumeriseurChoix.bb].nom,CablageEncodeConnecteur(Numeriseur[NumeriseurChoix.bb].fischer));
		Numeriseur[NumeriseurChoix.bb].fischer = 0;
		NumeriseurDeconnecte(&(Numeriseur[NumeriseurChoix.bb]));
		break;
	}
	NumeriseursEcrit(FichierPrefNumeriseurs,NUMER_CONNECTION,NUMER_TOUS);
    LectIdentFaite = 0;
	if(bSchemaManip) { if(OpiumDisplayed(bSchemaManip)) OrgaTraceTout(0,0); }
	return(0);
}
   ========================================================================== */
#pragma mark --- conversion des valeurs ---
shex NumeriseurRessUserDecode(TypeNumeriseur *numeriseur, int num, char *cval, int *i, float *r) {
	/* Prend une valeur utilisateur texte, cval[], et la transforme en valeurs numeriques, 
	 soit i, soit r selon le type de la ressource (pour les cles, i est l'index de la cle).
	 Rend hval.
	 Fonction ne modifiant pas les ressources numeriseurs */
	TypeBNModele *modele_bn; TypeBNModlRessource *ress;
	shex hval; int ival; float rval,fval;

	modele_bn = &(ModeleBN[numeriseur->def.type]); ress = modele_bn->ress + num;
	// printf("(%s) valeur user %s type %d\n",__func__, ress->nom,ress->type);
	//? if(ress->usertohexa) hval = (*(ress->usertohexa))(cval); else
	if(ress->type == RESS_INT) {
		if(*cval) sscanf(cval,"%d",&ival); else ival = 0;
		if(ress->lim_i.min < ress->lim_i.max) {
			if(ival < ress->lim_i.min) ival = ress->lim_i.min;
			if(ival > ress->lim_i.max) ival = ress->lim_i.max;
		} else {
			if(ival > ress->lim_i.min) ival = ress->lim_i.min;
			if(ival < ress->lim_i.max) ival = ress->lim_i.max;
		}
		sprintf(cval,"%d",ival); /* proprifie cval */
		if(ress->lim_i.max != ress->lim_i.min)
			hval = (shex)((ival - ress->lim_i.min) * ress->masque / (ress->lim_i.max - ress->lim_i.min));
		else hval = (shex)(*(hexa *)&ival & 0xFFFF);
		if(i) *i = ival;
	} else if(ress->type == RESS_FLOAT) {
		if(*cval) sscanf(cval,"%g",&rval); else rval = 0.0;
		fval = rval * numeriseur->ress[num].correctif;
		if(ress->lim_r.min < ress->lim_r.max) {
			if(fval < ress->lim_r.min) { fval = ress->lim_r.min; rval = fval / numeriseur->ress[num].correctif; }
			if(fval > ress->lim_r.max) { fval = ress->lim_r.max; rval = fval / numeriseur->ress[num].correctif; }
		} else {
			if(fval > ress->lim_r.min) { fval = ress->lim_r.min; rval = fval / numeriseur->ress[num].correctif; }
			if(fval < ress->lim_r.max) { fval = ress->lim_r.max; rval = fval / numeriseur->ress[num].correctif; }
		}
		if((rval < -100.0) || (rval >= 100.0)) sprintf(cval,"%6.3f",rval); else sprintf(cval,"%6.4f",rval); /* proprifie cval */
		hval = (shex)(((fval - ress->lim_r.min) * numeriseur->ress[num].rval_to_hex) + 0.5);
		// printf("(%s) %s = %g %s = 0x%04X\n",__func__,ress->nom,rval,ress->unite,hval);
		if(r) *r = rval;
	} else if(ress->type == RESS_CLES) {
		if(*cval) ival = ArgKeyGetIndex(ress->lim_k.cles,cval); else ival = 0;
		if(ival < 0) ival = 0;
		if(ival >= ress->lim_k.nb) ival = ress->lim_k.nb - 1;
		ArgKeyGetText(ress->lim_k.cles,ival,cval,NUMER_RESS_VALTXT); /* proprifie cval */
		hval = ress->lim_k.hval[ival];
		if(i) *i = ival;
	} else if(ress->type == RESS_PUIS2) {
		if(*cval) sscanf(cval,"%d",&ival); else ival = 0;
		if(ress->lim_i.max) ival /= ress->lim_i.max;
		hval = 0; while(ival) { ival = ival >> 1; hval++; }
		hval += (ress->lim_i.min - 1);
		if(hval > ress->masque) hval = ress->masque;
		if(hval < ress->lim_i.min) ival = 0;
		else ival = ress->lim_i.max * (1 << ((int)hval - ress->lim_i.min));
		sprintf(cval,"%d",ival); /* rectifie cval en cas de valeur incorrecte */ rval = (float)ival;
		if(i) *i = ival;
	} else hval = 0;

	return(hval);
}
/* ========================================================================== */
void NumeriseurRessFloatDecode(TypeNumeriseur *numeriseur, short num, float rval, shex *hval) {
	/* Prend la valeur numerique, rval, et la transforme en hval (reellement a ecrire).
	 Fonction ne modifiant pas les ressources numeriseurs */
	TypeBNModele *modele_bn; TypeBNModlRessource *ress;
	float fval; int ival;
	
	modele_bn = &(ModeleBN[numeriseur->def.type]); ress = modele_bn->ress + num;
	if(ress->type == RESS_INT) {
		fval = rval;
		if(ress->lim_i.min < ress->lim_i.max) {
			if(fval < (float)ress->lim_i.min) { numeriseur->ress[num].val.i = ress->lim_i.min; fval = (float)ress->lim_i.min; }
			if(fval > (float)ress->lim_i.max) { numeriseur->ress[num].val.i = ress->lim_i.max; fval = (float)ress->lim_i.max; }
		} else {
			if(fval > (float)ress->lim_i.min) { numeriseur->ress[num].val.i = ress->lim_i.min; fval = (float)ress->lim_i.min; }
			if(fval < (float)ress->lim_i.max) { numeriseur->ress[num].val.i = ress->lim_i.max; fval = (float)ress->lim_i.max; }
		}
		ival = (int)(((fval - (float)ress->lim_i.min) * numeriseur->ress[num].rval_to_hex) + 0.5);
		*hval = (shex)(*(hexa *)&ival & 0xFFFF);
	} else if(ress->type == RESS_FLOAT) {
		fval = rval * numeriseur->ress[num].correctif;																				
		if(ress->lim_r.min < ress->lim_r.max) {
			if(fval < ress->lim_r.min) { fval = ress->lim_r.min; numeriseur->ress[num].val.r = fval / numeriseur->ress[num].correctif; }
			if(fval > ress->lim_r.max) { fval = ress->lim_r.max; numeriseur->ress[num].val.r = fval / numeriseur->ress[num].correctif; }
		} else {
			if(fval > ress->lim_r.min) { fval = ress->lim_r.min; numeriseur->ress[num].val.r = fval / numeriseur->ress[num].correctif; }
			if(fval < ress->lim_r.max) { fval = ress->lim_r.max; numeriseur->ress[num].val.r = fval / numeriseur->ress[num].correctif; }
		}
		*hval = (shex)(((fval - ress->lim_r.min) * numeriseur->ress[num].rval_to_hex) + 0.5);
	} else if(ress->type == RESS_CLES) {
	#ifdef RVAL_EST_INDEX_CLE /* inusite, en fait */
		ival = (int)rval;
		if(ival < 0) ival = 0; if(ival >= ress->lim_k.nb) ival = ress->lim_k.nb - 1;
		ArgKeyGetText(ress->lim_k.cles,ival,cval,NUMER_RESS_VALTXT);
		*hval = ress->lim_k.hval[ival];
	#else /* sinon, rval est la valeur a obtenir (ATTENTION: utilise pour les gains) */
		char texte[80];
		sprintf(texte,"%g",rval);
		ival = ArgKeyGetIndex(ress->lim_k.cles,texte);
		if(ival < 0) ival = 0; if(ival >= ress->lim_k.nb) ival = ress->lim_k.nb - 1;
		*hval = ress->lim_k.hval[ival];
	#endif
	} else if(ress->type == RESS_PUIS2) {
		ival = (int)rval; if(ress->lim_i.max) ival /= ress->lim_i.max;
		*hval = 0; while(ival) { ival = ival >> 1; (*hval) += 1; }
		*hval += (ress->lim_i.min - 1);
		if(*hval > ress->masque) *hval = ress->masque;
	}
}
/* ========================================================================== */
shex NumeriseurRessValEncode(TypeNumeriseur *numeriseur, int num, char *cval, int ival, float rval) {
	/* Prend la valeur numerique, ival ou rval, et la transforme  en cval[] (texte utilisateur).
	 Pour les cles, ival est l'index de la cle.
	 Rend hval (a ecrire reellement).
	 Fonction ne modifiant pas les ressources numeriseurs */
	TypeBNModele *modele_bn; TypeBNModlRessource *ress;
	shex hval;
	float fval;

	modele_bn = &(ModeleBN[numeriseur->def.type]); ress = modele_bn->ress + num;
	// printf("(%s) valeur user %s type %d\n",__func__, ress->nom,ress->type);
	//? if(ress->usertohexa) hval = (*(ress->usertohexa))(cval); else
	if(ress->type == RESS_INT) {
		// printf("(%s) = %d\n",__func__,ival);
		if(ress->lim_i.min < ress->lim_i.max) {
			if(ival < ress->lim_i.min) numeriseur->ress[num].val.i = ival = ress->lim_i.min;
			if(ival > ress->lim_i.max) numeriseur->ress[num].val.i = ival = ress->lim_i.max;
		} else {
			if(ival > ress->lim_i.min) numeriseur->ress[num].val.i = ival = ress->lim_i.min;
			if(ival < ress->lim_i.max) numeriseur->ress[num].val.i = ival = ress->lim_i.max;
		}
		sprintf(cval,"%d",ival);
		if(ress->lim_i.max != ress->lim_i.min)
			hval = (shex)((ival - ress->lim_i.min) * ress->masque / (ress->lim_i.max - ress->lim_i.min));
		else hval = (shex)(*(hexa *)&ival & 0xFFFF);
	} else if(ress->type == RESS_FLOAT) {
		fval = rval * numeriseur->ress[num].correctif;
		// printf("(%s) %s = %g * %g = %g\n",__func__,ress->nom,rval,numeriseur->ress[num].correctif,fval);
		if(ress->lim_r.min < ress->lim_r.max) {
			if(fval < ress->lim_r.min) { fval = ress->lim_r.min; numeriseur->ress[num].val.r = rval = fval / numeriseur->ress[num].correctif; }
			if(fval > ress->lim_r.max) { fval = ress->lim_r.max; numeriseur->ress[num].val.r = rval = fval / numeriseur->ress[num].correctif; }
		} else {
			if(fval > ress->lim_r.min) { fval = ress->lim_r.min; numeriseur->ress[num].val.r = rval = fval / numeriseur->ress[num].correctif; }
			if(fval < ress->lim_r.max) { fval = ress->lim_r.max; numeriseur->ress[num].val.r = rval = fval / numeriseur->ress[num].correctif; }
		}
		if((rval < -100.0) || (rval >= 100.0)) sprintf(cval,"%6.3f",rval); else sprintf(cval,"%6.4f",rval);
		hval = (shex)(((fval - ress->lim_r.min) * numeriseur->ress[num].rval_to_hex) + 0.5);
		// printf("(%s) %s = (%g %s - %g) * %g = 0x%04X\n",__func__,ress->nom,fval,ress->unite,ress->lim_r.min,numeriseur->ress[num].rval_to_hex,hval);
	} else if(ress->type == RESS_CLES) {
		// printf("(%s) = %d\n",__func__,ival);
		if(ival < 0) ival = 0;
		if(ival >= ress->lim_k.nb) ival = ress->lim_k.nb - 1;
		ArgKeyGetText(ress->lim_k.cles,ival,cval,NUMER_RESS_VALTXT);
		hval = ress->lim_k.hval[ival];
	} else if(ress->type == RESS_PUIS2) {
		// printf("(%s) = %d\n",__func__,numeriseur->ress[num].val.i);
		if(ress->lim_i.max) ival /= ress->lim_i.max;
		hval = 0; while(ival) { ival = ival >> 1; hval++; }
		hval += (ress->lim_i.min - 1);
		if(hval > ress->masque) hval = ress->masque;
		if(hval < ress->lim_i.min) ival = 0;
		else ival = ress->lim_i.max * (1 << ((int)hval - ress->lim_i.min));
		sprintf(cval,"%d",ival); /* rectifie cval en cas de valeur incorrecte */
	} else hval = 0;
	return(hval);
}
/* ========================================================================== */
shex NumeriseurRessHexaEncode(TypeNumeriseur *numeriseur, int num, shex hval, char *cval, int *i, float *r) {
	/* Prend la valeur a ecrire reellement, .hval, et la transforme a la fois
	 en valeur utilisateur cval[] (texte) et en valeurs numeriques. Pour les cles, i est l'index de la cle.
	 Fonction ne modifiant pas les ressources numeriseurs */
	TypeBNModele *modele_bn; TypeBNModlRessource *ress;
	int ival; float rval;
	int dval,min,mval;

	modele_bn = &(ModeleBN[numeriseur->def.type]); ress = modele_bn->ress + num;
	//? if(ress->hexatouser) (*(ress->hexatouser))(hval,cval); else
	if(ress->type == RESS_INT) {
		if(ress->masque)  ival = (hval * (ress->lim_i.max - ress->lim_i.min) / ress->masque) + ress->lim_i.min;
		else ival = hval;
		if(cval) sprintf(cval,"%d",ival);
		if(i) *i = ival;
	} else if(ress->type == RESS_FLOAT) {
		if(ress->masque == 0) ress->masque = 1;
		rval = ((float)hval * numeriseur->ress[num].hex_to_rval) + ress->lim_r.min;
		rval /= numeriseur->ress[num].correctif;
		if(cval) { if((rval < -100.0) || (rval >= 100.0)) sprintf(cval,"%6.3f",rval); else sprintf(cval,"%6.4f",rval); }
		if(r) *r = rval;
	} else if(ress->type == RESS_CLES) {
		min = 0xFFFF; mval = 0;
		for(ival=0; ival<ress->lim_k.nb; ival++) {
			if(hval == ress->lim_k.hval[ival]) break;
			/* si on ne tombe pas juste, on cherche au moins la plus proche valeur */
			dval = (int)hval - (int)ress->lim_k.hval[ival];
			if(dval > 0) { if(dval < min) { min = dval; mval = ival; } }
			else { if(-dval < min) { min = -dval; mval = ival; } }
		}
		if(ival >= ress->lim_k.nb) { ival = mval; hval = ress->lim_k.hval[ival]; }
		if(cval) ArgKeyGetText(ress->lim_k.cles,ival,cval,NUMER_RESS_VALTXT);
		if(i) *i = ival;
	} else if(ress->type == RESS_PUIS2) {
		if(hval < ress->lim_i.min) ival = 0;
		else ival = ress->lim_i.max * (1 << ((int)hval - ress->lim_i.min));
		if(cval) sprintf(cval,"%d",ival);
		if(i) *i = ival;
	}

	return(hval);
}
/* ========================================================================== */
shex NumeriseurRessCurrentVal(TypeNumeriseur *numeriseur, int num, int *i, float *r) {
	/* Prend la valeur reellement ecrite, .hval, et la transforme en valeur utilisateur i ou r.
	 Pour les cles, i est l'index de la cle.
	 Fonction ne modifiant pas les ressources numeriseurs */
	return(NumeriseurRessHexaEncode(numeriseur,num,numeriseur->ress[num].hval,0,i,r));
}
/* ========================================================================== */
void NumeriseurRessUserChange(TypeNumeriseur *numeriseur, int num) {
	/* Prend la valeur utilisateur texte, .cval[], et la transforme en valeurs numeriques, 
	 	a la fois .val[].* (utilisateur) et .hval (reellement ecrite); pour les cles, .val[].i est l'index de la cle.
	 	Fonction modifiant les ressources numeriseurs, contrairement a NumeriseurRessUserDecode */
//	printf("(%s) Decode '%s'\n",__func__,numeriseur->ress[num].cval);
	numeriseur->ress[num].hval = NumeriseurRessUserDecode(numeriseur,num,numeriseur->ress[num].cval,
		&(numeriseur->ress[num].val.i),&(numeriseur->ress[num].val.r));
//	printf("(%s) Retourne '%s'\n",__func__,numeriseur->ress[num].cval);
}
/* ========================================================================== */
void NumeriseurRessValChangee(TypeNumeriseur *numeriseur, int num) {
	/* Prend la valeur numerique, .val.*, et la transforme a la fois en .cval[] (texte utilisateur)
	 et .hval (reellement ecrite). Pour les cles, .val.i est l'index de la cle.
	 Fonction utilisant les ressources numeriseurs, contrairement a NumeriseurRessValEncode */
//-	printf("(%s) Recoit %g\n",__func__,numeriseur->ress[num].val.r);
	numeriseur->ress[num].hval = NumeriseurRessValEncode(numeriseur,num,numeriseur->ress[num].cval,
		numeriseur->ress[num].val.i,numeriseur->ress[num].val.r);
}
/* ========================================================================== */
void NumeriseurRessHexaChange(TypeNumeriseur *numeriseur, int num) {
	/* Prend la valeur reellement ecrite, .hval, et la transforme en valeur utilisateur, 
	   a la fois .val[].* (numerique) et .cval[] (texte). Pour les cles, .val[].i est l'index de la cle.
	 Fonction modifiant les ressources numeriseurs */
	numeriseur->ress[num].hval = NumeriseurRessHexaEncode(numeriseur,num,numeriseur->ress[num].hval,
		numeriseur->ress[num].cval,&(numeriseur->ress[num].val.i),&(numeriseur->ress[num].val.r));
}
#ifdef OBSOLETE
/* ========================================================================== */
shex NumeriseurRessCurrentVal(TypeNumeriseur *numeriseur, int num, int *i, float *r) {
	/* Prend la valeur reellement ecrite, .hval, et la transforme en valeur utilisateur i ou r.
	 Pour les cles, i est l'index de la cle */
	TypeBNModele *modele_bn; TypeBNModlRessource *ress;
	shex hval; int ival; float rval;
	int dval,min,mval;
	
	modele_bn = &(ModeleBN[numeriseur->def.type]); ress = modele_bn->ress + num;
	hval = numeriseur->ress[num].hval;
	//? if(ress->hexatouser) (*(ress->hexatouser))(hval,cval); else
	if(ress->type == RESS_INT) {
		if(ress->masque == 0) {
			ress->masque = 1;
			printf("  ! %s.%s.masque trouve =0, mis a 1 (%s.%d.hval=%04X)\n",modele_bn->nom,ress->nom,
				   numeriseur->nom,num,hval);
		}
		*i = (hval * (ress->lim_i.max - ress->lim_i.min) / ress->masque) + ress->lim_i.min;
	} else if(ress->type == RESS_FLOAT) {
		rval = ((float)hval * numeriseur->ress[num].hex_to_rval) + ress->lim_r.min;
		if(numeriseur->ress[num].correctif) rval /= numeriseur->ress[num].correctif;
		*r = rval;
	} else if(ress->type == RESS_CLES) {
		min = 0xFFFF; mval = 0;
		for(ival=0; ival<ress->lim_k.nb; ival++) {
			if(hval == ress->lim_k.hval[ival]) break;
			/* si on ne tombe pas juste, on cherche au moins la plus proche valeur */
			dval = (int)hval - (int)ress->lim_k.hval[ival];
			if(dval > 0) { if(dval < min) { min = dval; mval = ival; } }
			else { if(-dval < min) { min = -dval; mval = ival; } }
		}
		if(ival >= ress->lim_k.nb) {
			ival = mval;
			numeriseur->ress[num].hval = ress->lim_k.hval[ival];
		}
		*i = ival;
	} else if(ress->type == RESS_PUIS2) {
		if(hval < ress->lim_i.min) *i = 0;
		else *i = ress->lim_i.max * (1 << ((int)hval - ress->lim_i.min));
	}
	return(hval);
}
/* ========================================================================== */
void NumeriseurRessHexaChange(TypeNumeriseur *numeriseur, int num) {
	/* Prend la valeur reellement ecrite, .hval, et la transforme en valeur utilisateur,
	 a la fois .val[].* (numerique) et .cval[] (texte). Pour les cles, .val[].i est l'index de la cle */
	TypeBNModele *modele_bn; TypeBNModlRessource *ress;
	shex hval; char *cval;
	int ival; float rval;
	int dval,min,mval;
	
	cval = numeriseur->ress[num].cval;
	hval = numeriseur->ress[num].hval;
	modele_bn = &(ModeleBN[numeriseur->def.type]); ress = modele_bn->ress + num;
	//? if(ress->hexatouser) (*(ress->hexatouser))(hval,cval); else
	if(ress->type == RESS_INT) {
		if(ress->masque == 0) {
			ress->masque = 1;
			printf("  ! %s.%s.masque trouve =0, mis a 1 (%s.%d.hval=%04X)\n",modele_bn->nom,ress->nom,
				numeriseur->nom,num,hval);
		}
		ival = (hval * (ress->lim_i.max - ress->lim_i.min) / ress->masque) + ress->lim_i.min;
		sprintf(cval,"%d",ival);
		numeriseur->ress[num].val.i = ival;
	} else if(ress->type == RESS_FLOAT) {
		rval = ((float)hval * numeriseur->ress[num].hex_to_rval) + ress->lim_r.min;
		rval /= numeriseur->ress[num].correctif;
		if((rval < -100.0) || (rval >= 100.0)) sprintf(cval,"%5.2f",rval); else sprintf(cval,"%5.3f",rval);
		numeriseur->ress[num].val.r = rval;
	} else if(ress->type == RESS_CLES) {
		min = 0xFFFF; mval = 0;
		for(ival=0; ival<ress->lim_k.nb; ival++) {
			if(hval == ress->lim_k.hval[ival]) break;
			/* si on ne tombe pas juste, on cherche au moins la plus proche valeur */
			dval = (int)hval - (int)ress->lim_k.hval[ival];
			if(dval > 0) { if(dval < min) { min = dval; mval = ival; } }
			else { if(-dval < min) { min = -dval; mval = ival; } }
		}
		if(ival >= ress->lim_k.nb) {
			ival = mval;
			numeriseur->ress[num].hval = ress->lim_k.hval[ival];
		}
		ArgKeyGetText(ress->lim_k.cles,ival,cval,NUMER_RESS_VALTXT);
		numeriseur->ress[num].val.i = ival;
	} else if(ress->type == RESS_PUIS2) {
		if(hval < ress->lim_i.min) ival = 0;
		else ival = ress->lim_i.max * (1 << ((int)hval - ress->lim_i.min));
		sprintf(cval,"%d",ival);
		numeriseur->ress[num].val.i = ival;
	}
	// printf("(NumeriseurRessHexaChange) hexa: %04X => texte: '%s'\n",numeriseur->ress[num].hval,cval);
}
/* ========================================================================== */
void NumeriseurRessValChangee(TypeNumeriseur *numeriseur, int num) {
	/* Prend la valeur numerique, .val.*,
	 et la transforme a la fois en .cval[] (texte utilisateur)
	 et .hval (reellement ecrite)
	 Pour les cles, .val.i est l'index de la cle */
	TypeBNModele *modele_bn; TypeBNModlRessource *ress;
	shex hval; char *cval;
	int ival; float rval,fval;

	cval = numeriseur->ress[num].cval;
	modele_bn = &(ModeleBN[numeriseur->def.type]); ress = modele_bn->ress + num;
	//? if(ress->usertohexa) hval = (*(ress->usertohexa))(cval); else 
	// printf("(%s) valeur user %s type %d\n",__func__, ress->nom,ress->type);
	if(ress->type == RESS_INT) {
		ival = numeriseur->ress[num].val.i;
		// printf("(%s) = %d\n",__func__,ival);
		if(ress->lim_i.min < ress->lim_i.max) {
			if(ival < ress->lim_i.min) numeriseur->ress[num].val.i = ival = ress->lim_i.min;
			if(ival > ress->lim_i.max) numeriseur->ress[num].val.i = ival = ress->lim_i.max;
		} else {
			if(ival > ress->lim_i.min) numeriseur->ress[num].val.i = ival = ress->lim_i.min;
			if(ival < ress->lim_i.max) numeriseur->ress[num].val.i = ival = ress->lim_i.max;
		}
		sprintf(cval,"%d",ival);
		hval = (shex)((ival - ress->lim_i.min) * ress->masque / (ress->lim_i.max - ress->lim_i.min));
	} else if(ress->type == RESS_FLOAT) {
		rval = numeriseur->ress[num].val.r;
		// printf("(%s) = %g\n",__func__,rval);
		fval = rval * numeriseur->ress[num].correctif;																				
		if(ress->lim_r.min < ress->lim_r.max) {
			if(fval < ress->lim_r.min) { fval = ress->lim_r.min; numeriseur->ress[num].val.r = rval = fval / numeriseur->ress[num].correctif; }
			if(fval > ress->lim_r.max) { fval = ress->lim_r.max; numeriseur->ress[num].val.r = rval = fval / numeriseur->ress[num].correctif; }
		} else {
			if(fval > ress->lim_r.min) { fval = ress->lim_r.min; numeriseur->ress[num].val.r = rval = fval / numeriseur->ress[num].correctif; }
			if(fval < ress->lim_r.max) { fval = ress->lim_r.max; numeriseur->ress[num].val.r = rval = fval / numeriseur->ress[num].correctif; }
		}
		if((rval < -100.0) || (rval >= 100.0)) sprintf(cval,"%5.2f",rval); else sprintf(cval,"%5.3f",rval);
		hval = (shex)(((fval - ress->lim_r.min) * numeriseur->ress[num].rval_to_hex) + 0.5);
	} else if(ress->type == RESS_CLES) {
		ival = numeriseur->ress[num].val.i;
		// printf("(%s) = %d\n",__func__,ival);
		if(ival < 0) ival = 0;
		if(ival >= ress->lim_k.nb) ival = ress->lim_k.nb - 1;
		ArgKeyGetText(ress->lim_k.cles,ival,cval,NUMER_RESS_VALTXT);
		hval = ress->lim_k.hval[ival];
	} else if(ress->type == RESS_PUIS2) {
		// printf("(%s) = %d\n",__func__,numeriseur->ress[num].val.i);
		ival = numeriseur->ress[num].val.i; if(ress->lim_i.max) ival /= ress->lim_i.max;
		hval = 0; while(ival) { ival = ival >> 1; hval++; }
		hval += (ress->lim_i.min - 1);
		if(hval > ress->masque) hval = ress->masque;
		if(hval < ress->lim_i.min) ival = 0;
		else ival = ress->lim_i.max * (1 << ((int)hval - ress->lim_i.min));
		sprintf(cval,"%d",ival); /* rectifie cval en cas de valeur incorrecte */
    } else hval = 0;
	numeriseur->ress[num].hval = hval;
	// printf("(%s) texte: '%s', hexa: %04X\n",__func__,cval,numeriseur->ress[num].hval);
}
#endif /* OBSOLETE */
/* ========================================================================== */
INLINE char NumeriseurChargeValeurBB(TypeNumeriseur *numeriseur, hexa ss_adrs, TypeADU valeur) {
	if(SambaMaitre) {
		int n;
		n = SambaEcritSatCourt((numeriseur->repart)->sat,SMB_NUMER,SMB_NUMER_MANU_ADRS,0,ss_adrs,2,valeur,(shex)(numeriseur->bb));
		strcpy(RepartiteurValeurEnvoyee,AcquisMsgEnvoye);
		return(n);
	} else return(RepartiteurEcritRessourceBB(numeriseur->repart,numeriseur->entree,numeriseur->def.adrs,ss_adrs,valeur));
}
/* 
#define adrs(j) (cptr[j])? cptr[j]: ((hptr[j])? *(hptr[j]): ((iptr[j])? *(iptr[j]): *(fptr[j])))
	char *cptr[NUMER_ACCES_MAXVAR];
	shex hloc,*hptr[NUMER_ACCES_MAXVAR];
	TypeADU iloc,*iptr[NUMER_ACCES_MAXVAR];
	float floc,*fptr[NUMER_ACCES_MAXVAR]; 
	for(j=0; j<modele_bn->acces.nb; j++) {
		cptr[j] = 0; hptr[j] = 0; iptr[j] = 0; fptr[j] = 0;
		switch(modele_bn->acces.var[j].cle) {
			case NUMER_VAR_IDENT: cptr[j] = (char *)(&(numeriseur->def.adrs)); break;
			case NUMER_VAR_ADRS:  hptr[j] = (shex *)(&(ress->ssadrs)); break;
			case NUMER_VAR_NOM:   cptr[j] = ress->nom; break;
			case NUMER_VAR_HEXA:  hloc = hval; hptr[j] = &hloc; break;
			case NUMER_VAR_VAL:
			if(ress->type == RESS_FLOAT) { floc = fval; fptr[j] = &floc; } .. float* et void* incompatibles
			else if(ress->type == RESS_INT) { iloc = ival; iptr[j] = &iloc; }
			break;
		}
	}
*/
#define adrs(j) &(ecrit[j][0])
/* ========================================================================== */
char NumeriseurCommandeEnvoi(TypeNumeriseur *numeriseur, int k, shex hval, TypeADU ival, float fval) {
	byte cmde[80]; int j,l;
	char ecrit[NUMER_ACCES_MAXVAR][NUMER_ACCES_VARSTR];
	TypeBNModele *modele_bn; TypeBNModlRessource *ress;

    l = 0;
	if((numeriseur->def.type < 0) || (numeriseur->def.type >= NUMER_TYPES)) numeriseur->def.type = 0;
	modele_bn = &(ModeleBN[numeriseur->def.type]); ress = modele_bn->ress + k;
	/* char mode[12]; ArgKeyGetText(ACCES_FORMATS,modele_bn->acces.mode,mode,12);
	printf("(%s) Ecrit la commande en mode %s pour %s.%s = (0x%04X, %d, %g)\n",__func__,mode,numeriseur->nom,ress->nom,hval,ival,fval); */
	if(SambaMaitre) {
		int n;
		n = SambaEcritSatCourt((numeriseur->repart)->sat,SMB_NUMER,SMB_NUMER_MANU_ADRS,0,ress->ssadrs,2,ival,(shex)(numeriseur->bb));
		strcpy(RepartiteurValeurEnvoyee,AcquisMsgEnvoye);
		return(n);
		
	} else if(modele_bn->acces.mode == NUMER_ACCES_BB) {
		// return(RepartiteurEcritRessourceBB(numeriseur->repart,numeriseur->entree,numeriseur->def.adrs,ress->ss_adrs,ival));
		l = 0;
		cmde[l++] = numeriseur->def.adrs;
		cmde[l++] = ress->ssadrs;
		cmde[l++] = (byte)((hval >> 8) & 0xFF);
		cmde[l++] = (byte)(hval & 0xFF);
		
		/* CAEN SY127: format = "1BAA%s\rC%d\r1" */
	} else if(modele_bn->acces.mode == NUMER_ACCES_TXT) {
		for(j=0; j<modele_bn->acces.nb; j++) {
			switch(modele_bn->acces.var[j].cle) {
			  case NUMER_VAR_IDENT: snprintf(&(ecrit[j][0]),NUMER_ACCES_VARSTR,modele_bn->acces.var[j].fmt,numeriseur->def.adrs); break;
			  case NUMER_VAR_ADRS:  snprintf(&(ecrit[j][0]),NUMER_ACCES_VARSTR,modele_bn->acces.var[j].fmt,ress->ssadrs); break;
			  case NUMER_VAR_NOM:   snprintf(&(ecrit[j][0]),NUMER_ACCES_VARSTR,modele_bn->acces.var[j].fmt,ress->nom); break;
			  case NUMER_VAR_HEXA:  snprintf(&(ecrit[j][0]),NUMER_ACCES_VARSTR,modele_bn->acces.var[j].fmt,hval); break;
			  case NUMER_VAR_VAL:
				if(ress->type == RESS_FLOAT) snprintf(&(ecrit[j][0]),NUMER_ACCES_VARSTR,modele_bn->acces.var[j].fmt,fval);
				else /* if(ress->type == RESS_INT) */
					snprintf(&(ecrit[j][0]),NUMER_ACCES_VARSTR,modele_bn->acces.var[j].fmt,ival);
				break;
			}
		}
		switch(modele_bn->acces.nb) {
			case 0: strcpy((char *)cmde,modele_bn->acces.fmt); l = strlen((char *)cmde); break;
			case 1: l = sprintf((char *)cmde,modele_bn->acces.fmt,adrs(0)); break;
			case 2: l = sprintf((char *)cmde,modele_bn->acces.fmt,adrs(0),adrs(1)); break;
			case 3: l = sprintf((char *)cmde,modele_bn->acces.fmt,adrs(0),adrs(1),adrs(2)); break;
			case 4: l = sprintf((char *)cmde,modele_bn->acces.fmt,adrs(0),adrs(1),adrs(2),adrs(3)); break;
		}
		// printf("(%s) Commande a envoyer: <%s>\n",__func__,cmde);
		
	} else if(modele_bn->acces.mode == NUMER_ACCES_BIN) {
		OpiumFail("Commandes binaires non programmees");
	}
	
	return(RepartiteurEcritNumeriseur(numeriseur->repart,numeriseur->entree,cmde,l));
}
/* ========================================================================== */
INLINE void NumeriseurAutoriseBB2(TypeNumeriseur *numeriseur, char autorise, char log) {
	/*	autorise =  0: bloque
		autorise =  1: debloque
		autorise = -1: charge DACs 
	    ss_adrs_alim defini en dur dans structure.h
	 */
	short alim;

	if(numeriseur->vsnid < 2) return;
	alim = autorise? ((autorise == -1)? _charge_dac_filtres: _autorise_commandes): _bloque_commandes;
	if((numeriseur->repart)->ecr.ip.path >= 0) NumeriseurChargeValeurBB(numeriseur,ss_adrs_alim,alim);
	if(log) {
		if(log == 2) printf("            . envoye"); else printf("* Envoye");
		printf(": %s",RepartiteurValeurEnvoyee);
		printf(" / alim = %s\n",autorise? ((autorise == -1)? "charge DACs": "libere"): "bloque");
	}
	numeriseur->verrouille = !autorise;
}
/* ========================================================================== */
INLINE char NumeriseurAutoriseAcces(TypeNumeriseur *numeriseur, char autorise, char log) {
	TypeRepartiteur *repart;

	if(!(repart = numeriseur->repart)) {
		sprintf(RepartiteurValeurEnvoyee,"! Numeriseur %s inaccessible",numeriseur->nom);
		return(0);
	}
	if((repart->famille == FAMILLE_IPE) && autorise) IpeAutoriseFibre(repart,numeriseur->entree,1);
	if(numeriseur->vsnid >= 2) NumeriseurAutoriseBB2(numeriseur,autorise,log);
	if((repart->famille == FAMILLE_IPE) && !autorise) IpeAutoriseFibre(repart,numeriseur->entree,0);
	NumeriseurDejaAutorise = autorise? numeriseur->bb: -1;
	return(1);
}
/* ========================================================================== */
INLINE static char NumeriseurAccesAuto(TypeNumeriseur *numeriseur, char autorise, char *suite, char *marge) {
	TypeNumeriseur *ancien; TypeRepartiteur *repart,*rep_avant; char ecrit;
	int flt,fbr; int flt_avant; char ok; char *prefixe;

	ecrit = 0;
	if(!(repart = numeriseur->repart)) return(0); // en principe, deja teste, mais tant qu'a faire de recuperer repart...
	prefixe = suite;
	if(autorise) {
		if(NumeriseurDejaAutorise == numeriseur->bb) return(0);
		flt_avant = flt = -1;
		if(NumeriseurDejaAutorise >= 0) {
			ancien = &(Numeriseur[NumeriseurDejaAutorise]); rep_avant = ancien->repart;
			// NumeriseurAccesAuto(ancien,0,prefixe); mais on regroupe les acces IPE_FLT_ACCES_BB
			if(ancien->vsnid >= 2) {
				if(rep_avant->ecr.ip.path >= 0) NumeriseurChargeValeurBB(ancien,ss_adrs_alim,_bloque_commandes);
				//? unless(ecrit) { printf("\n"); ecrit = 1; }
				if(prefixe) printf("%s ",prefixe); else printf("%s/   . ",DateHeure());
				printf("%s | %8s: %13s = bloque\n",RepartiteurValeurEnvoyee,ancien->nom,"alim");
				prefixe = marge;
			}
			if((rep_avant->famille == FAMILLE_IPE) && rep_avant->r.ipe.ports_BB) {
				flt_avant = IPE_FLT_NUM(ancien->entree); fbr = IPE_FLT_FBR(ancien->entree,flt_avant);
				rep_avant->r.ipe.flt[flt_avant].fibres_fermees |= (1 << fbr);
			}
		} else rep_avant = repart;
		if((repart->famille == FAMILLE_IPE) && repart->r.ipe.ports_BB) {
			flt = IPE_FLT_NUM(numeriseur->entree); fbr = IPE_FLT_FBR(numeriseur->entree,flt);
			repart->r.ipe.flt[flt].fibres_fermees &= ~(1 << fbr);
		}
		if((flt_avant >= 0) && ((rep_avant != repart) || (flt_avant != flt))) {
			//? unless(ecrit) { printf("\n"); ecrit = 1; }
			ok = IpeEcritFlt(rep_avant,flt_avant,IPE_FLT_ACCES_BB,repart->r.ipe.flt[flt_avant].fibres_fermees,0);
			if(prefixe) printf("%s ",prefixe); else printf("%s/   . ",DateHeure());
			if(ok) printf("%s -> %s\n",RepartiteurValeurEnvoyee,rep_avant->nom);
			else printf("! Echec de l'ecriture de %s vers %s\n",RepartiteurValeurEnvoyee,rep_avant->nom);
			prefixe = marge;
		}
		if(flt >= 0) {
			//? unless(ecrit) { printf("\n"); ecrit = 1; }
			ok = IpeEcritFlt(repart,flt,IPE_FLT_ACCES_BB,repart->r.ipe.flt[flt].fibres_fermees,0);
			if(prefixe) printf("%s ",prefixe); else printf("%s/   . ",DateHeure());
			if(ok) printf("%s -> %s\n",RepartiteurValeurEnvoyee,repart->nom);
			else printf("! Echec de l'ecriture de %s vers %s\n",RepartiteurValeurEnvoyee,repart->nom);
			prefixe = marge;
		}
		if(numeriseur->vsnid >= 2) {
			if(repart->ecr.ip.path >= 0) NumeriseurChargeValeurBB(numeriseur,ss_adrs_alim,_autorise_commandes);
			//? unless(ecrit) { printf("\n"); ecrit = 1; }
			if(prefixe) printf("%s ",prefixe); else printf("%s/   . ",DateHeure());
			printf("%s | %8s: %13s = libere\n",RepartiteurValeurEnvoyee,numeriseur->nom,"alim");
			prefixe = marge;
		}
	} else {
		if(NumeriseurDejaAutorise != numeriseur->bb) return(0);
		ancien = &(Numeriseur[NumeriseurDejaAutorise]); rep_avant = ancien->repart;
		if(ancien->vsnid >= 2) {
			if(rep_avant->ecr.ip.path >= 0) NumeriseurChargeValeurBB(ancien,ss_adrs_alim,_bloque_commandes);
			//? unless(ecrit) { printf("\n"); ecrit = 1; }
			if(prefixe) printf("%s ",prefixe); else printf("%s/   . ",DateHeure());
			printf("%s | %8s: %13s = bloque\n",RepartiteurValeurEnvoyee,ancien->nom,"alim");
			prefixe = marge;
		}
		if((rep_avant->famille == FAMILLE_IPE) && rep_avant->r.ipe.ports_BB) {
			flt_avant = IPE_FLT_NUM(ancien->entree); fbr = IPE_FLT_FBR(ancien->entree,flt_avant);
			rep_avant->r.ipe.flt[flt_avant].fibres_fermees |= (1 << fbr);
			//? unless(ecrit) { printf("\n"); ecrit = 1; }
			ok = IpeEcritFlt(rep_avant,flt_avant,IPE_FLT_ACCES_BB,repart->r.ipe.flt[flt_avant].fibres_fermees,0);
			if(prefixe) printf("%s ",prefixe); else printf("%s/   . ",DateHeure());
			if(ok) printf("%s -> %s\n",RepartiteurValeurEnvoyee,rep_avant->nom);
			else printf("! Echec de l'ecriture de %s vers %s\n",RepartiteurValeurEnvoyee,rep_avant->nom);
			prefixe = marge;
		}
	}
	NumeriseurDejaAutorise = autorise? numeriseur->bb: -1;
	ecrit = 1;
	return(ecrit);
}
/* ========================================================================== */
static char NumeriseurRessourceValide(TypeNumeriseur *numeriseur, int k, char *explics) {
	TypeBNModele *modele_bn; TypeBNModlRessource *ress; TypeRepartiteur *repart;
	hexa ssadrs;

	explics[0] = '\0';
	modele_bn = &(ModeleBN[numeriseur->def.type]); ress = modele_bn->ress + k;
	if(!(repart = numeriseur->repart)) sprintf(explics,"! Numeriseur %s inaccessible, %s inchange",numeriseur->nom,ress->nom);
	else if(!repart->local) sprintf(explics,"* Numeriseur %s hors du perimetre, %s inchange",numeriseur->nom,ress->nom);
//	else if(!repart->local) return(0);
	else if((ssadrs = ress->ssadrs) == 0xFF) sprintf(explics,"! Transmis: neant (la ressource %s est en lecture seule)\n",ress->nom);

	return(explics[0] == '\0');
}
/* ========================================================================== */
static char NumeriseurRessourceEnvoi(TypeNumeriseur *numeriseur, int k, shex *ecrit, char *explics) {
	TypeBNModele *modele_bn; TypeBNModlRessource *ress; TypeRepartiteur *repart;
	hexa ssadrs; shex hval;
	int i; char reconstruit,ok;

	modele_bn = &(ModeleBN[numeriseur->def.type]);
	/* char mode[12]; ArgKeyGetText(ACCES_FORMATS,modele_bn->acces.mode,mode,12);
	printf("(%s) Demande la commande en mode %s pour %s.%s\n",__func__,mode,numeriseur->nom,modele_bn->ress[k].nom); */
	hval = 0; ok = 0;
	if(SambaMaitre) /* pas bon: la ressource n'a pas ete mise a jour */ {
		if((repart = numeriseur->repart)) SambaEcritSatCourt(repart->sat,SMB_NUMER,SMB_NUMER_AUTO_RESS,0,(byte)k,1,(shex)(numeriseur->bb));
		else return(0);
		strcpy(explics,AcquisMsgEnvoye);
	} else {
		ress = modele_bn->ress + k; ssadrs = ress->ssadrs;
		if(ress->bit0 >= 0) {
			reconstruit = 0;
			hval = 0;
			for(i=0; i<modele_bn->nbress; i++) if((modele_bn->ress[i].bit0 >= 0) && (modele_bn->ress[i].ssadrs == ssadrs)) {
				hval = hval | (numeriseur->ress[i].hval << modele_bn->ress[i].bit0); reconstruit = 1;
			}
			unless(reconstruit) hval = numeriseur->ress[k].hval << modele_bn->ress[k].bit0;
		} else hval = numeriseur->ress[k].hval;
		if(modele_bn->acces.mode == NUMER_ACCES_BB) {
			//- ok = NumeriseurChargeValeurBB(numeriseur,ssadrs,hval);
			ok = NumeriseurCommandeEnvoi(numeriseur,k,hval,0,0);
		} else {
			if(ress->type == RESS_FLOAT) ok = NumeriseurCommandeEnvoi(numeriseur,k,hval,0,numeriseur->ress[k].val.r);
			else ok = NumeriseurCommandeEnvoi(numeriseur,k,hval,numeriseur->ress[k].val.i,0);
		}
		strcpy(explics,RepartiteurValeurEnvoyee);
	}
	*ecrit = hval;

	return(ok);
}
/* ========================================================================== */
shex NumeriseurRessourceChargeAuto(TypeNumeriseur *numeriseur, int k, char *suite, char *marge) {
	char msg[80],*prefixe; char ecrit,ok; shex hval; TypeBNModele *modele_bn;

	prefixe = suite; hval = 0;
	if(NumeriseurRessourceValide(numeriseur,k,msg)) {
		if(SambaMaitre) ecrit = 0;
		else ecrit = NumeriseurAccesAuto(numeriseur,1,prefixe,marge);
		ok = NumeriseurRessourceEnvoi(numeriseur,k,&hval,msg);
		if(ecrit) prefixe = marge;
	}
	if(prefixe) {
		if(*prefixe == '\0') printf("%s/",DateHeure());
		if(ok) {
			modele_bn = &(ModeleBN[numeriseur->def.type]);
			printf("%s %s | %8s: %13s = %s %s\n",prefixe,
				msg,numeriseur->nom,modele_bn->ress[k].nom,numeriseur->ress[k].cval,modele_bn->ress[k].unite);
		} else printf("%s %s\n",prefixe,msg);
	}
	return(hval);
}
/* ========================================================================== */
shex NumeriseurAdrsChargeAuto(TypeNumeriseur *numeriseur, hexa ssadrs, shex hval, char *suite, char *marge) {
	TypeRepartiteur *repart;
	char ok,ecrit;
	char explics[80],*msg,*prefixe;
	
    msg = explics; ok = 0; prefixe = suite; ecrit = 0;
	if(!(repart = numeriseur->repart)) sprintf(explics,"! Numeriseur %s inaccessible, sous-adresse %02X inchangee",numeriseur->nom,ssadrs);
	else if(!repart->local) return(0); // sprintf(explics,"* Numeriseur %s hors du perimetre, %s inchange",numeriseur->nom,ress->nom);
	else if(ssadrs != 0) {
		if(SambaMaitre) {
			SambaEcritSatCourt(repart->sat,SMB_NUMER,SMB_NUMER_AUTO_ADRS,0,ssadrs,2,hval,(shex)(numeriseur->bb));
			msg = AcquisMsgEnvoye;
		} else {
			ecrit = NumeriseurAccesAuto(numeriseur,1,prefixe,marge);
			ok = NumeriseurChargeValeurBB(numeriseur,ssadrs,hval);
			msg = RepartiteurValeurEnvoyee;
		}
		if(ecrit) prefixe = marge;
	} else sprintf(explics,"! Sous-adresse %02X inutilisable",ssadrs);
	if(prefixe) {
		if(*prefixe == '\0') printf("%s/",DateHeure());
		if(ok) printf("%s %s | %8s: @%-12X = 0x%04X\n",prefixe,msg,numeriseur->nom,ssadrs,hval);
		else printf("%s %s\n",prefixe,msg);
	}
	return(hval);
}
/* ========================================================================== */
void NumeriseurChargeFin(char *prefixe) {
	if(NumeriseurDejaAutorise >= 0) {
		NumeriseurAccesAuto(&(Numeriseur[NumeriseurDejaAutorise]),0,prefixe,prefixe);
	}
}
/* ========================================================================== */
shex NumeriseurRessourceCharge(TypeNumeriseur *numeriseur, int k, char *prefixe) {
	char msg[80]; shex hval; char ok; TypeBNModele *modele_bn;

	hval = 0; ok = 0;
	/* modele_bn = &(ModeleBN[numeriseur->def.type]);
	char mode[12]; ArgKeyGetText(ACCES_FORMATS,modele_bn->acces.mode,mode,12);
	printf("(%s) Charge la ressource en mode %s pour %s.%s\n",__func__,mode,numeriseur->nom,modele_bn->ress[k].nom); */
	if(NumeriseurRessourceValide(numeriseur,k,msg)) {
		// printf("(%s) Ressource valide\n",__func__);
		ok = NumeriseurRessourceEnvoi(numeriseur,k,&hval,msg);
		// printf("(%s) Envoi %s\n",__func__,ok?"fait":"abandonne");
	} // else printf("(%s) Ressource INVALIDE\n",__func__);
	if(prefixe) {
		if(*prefixe == '\0') printf("%s/",DateHeure());
		if(ok) {
			modele_bn = &(ModeleBN[numeriseur->def.type]);
			printf("%s %s | %8s: %13s = %s %s\n",prefixe,msg,
				numeriseur->nom,modele_bn->ress[k].nom,numeriseur->ress[k].cval,modele_bn->ress[k].unite);
		} else printf("%s %s\n",prefixe,msg);
	}
	return(hval);
}
/* ========================================================================== */
int NumeriseurCharge(TypeNumeriseur *numeriseur, char tout, char log) {
	int k; char deja_envoye[NUMER_RESS_MAX],ok; char msg[80]; shex hval; TypeBNModele *modele_bn;

	hval = 0;
	if(log) printf("          * %s:\n",numeriseur->nom);
	NumeriseurAutoriseAcces(numeriseur,1,2);
	modele_bn = &(ModeleBN[numeriseur->def.type]);
	for(k=0; k<modele_bn->nbress; k++) deja_envoye[k] = 0;
	for(k=0; k<modele_bn->nbress; k++) {
		if(!tout && !(numeriseur->ress[k].init)) continue;
		ok = NumeriseurRessourceEnvoi(numeriseur,k,&hval,msg);
		if(log) {
			if(ok) printf("            . envoye: %s / %s = %s\n",msg,modele_bn->ress[k].nom,numeriseur->ress[k].cval);
			else printf("            %s\n",msg);
		}
	};
	NumeriseurAutoriseAcces(numeriseur,0,2);
	return(0);
}
/* ========================================================================== */
INLINE void NumeriseurRecopieStatus(TypeNumeriseur *numeriseur, short index, hexa lu) {
	TypeBNModele *modele_bn; TypeNumeriseurRess *ress; int k;
	
	if(index >= NUMER_STATUS_DIM) return;
	modele_bn = &(ModeleBN[numeriseur->def.type]);
	if((k = modele_bn->numress[index]) < 0) return;
	if(modele_bn->acces.mode != NUMER_ACCES_BB) return; // a recuperer...
	// printf("          | Status[%d] = Ressource[%d] (%s.%s) = %08X\n",index,k,modele_bn->nom,modele_bn->ress[k].nom,lu);

	lu = lu & 0xFFFF; // dans le cas 'int lu'
	ress = &(numeriseur->ress[k]);
	//* printf("(%s) %s du reglage %s.%s\n",__func__,(ress->a_charger)?"Maintien":"Mise a jour",numeriseur->nom,modele_bn->ress[k].nom);
	unless(ress->a_charger) {
		if(modele_bn->ress[k].bit0 >= 0) {
			int status,n;
			status = modele_bn->ress[k].status;
			for(n=0; n<modele_bn->nbress; n++) if((modele_bn->ress[n].bit0 >= 0) && (modele_bn->ress[n].status == status)) {
				numeriseur->ress[n].hval = (lu >> modele_bn->ress[n].bit0) & modele_bn->ress[n].masque;
			}
		} else ress->hval = lu;
		NumeriseurRessHexaChange(numeriseur,k);
		// printf("%2d: %s[%d:%s]=%s %s (%04X)\n",index,numeriseur->nom,k,modele_bn->ress[k].nom,numeriseur->ress[k].cval,modele_bn->ress[k].unite,lu);
	#ifdef REPERCUTE_STATUS
		ReglageEffetDeBord(numeriseur->ress[k].regl,0);
		if((modele_bn->ress[k].categ == RESS_IDENT) 
		   && ((numeriseur->ident == 0) || (numeriseur->ident == 0xFF))
		   && ((ress->hval != 0) && (ress->hval != 0xFF))) {
			numeriseur->ident = ress->hval;
			numeriseur->def.adrs = ress->hval & 0xFF;
			numeriseur->def.serial = ress->hval & 0xFF;
			numeriseur->vsnid = (ress->hval & 0xF00) >> 8;
			if(numeriseur->def.serial == 0xFF) 
				snprintf(numeriseur->nom,NUMER_NOM,"%s",modele_bn->nom);
			else snprintf(numeriseur->nom,NUMER_NOM,"%s#%02d",modele_bn->nom,numeriseur->def.serial);
		}
	#endif
		(numeriseur->interface).a_rafraichir = 1;
	}
	if(ress->historique.tampon.t.sval) {
		shex hval;
		hval = (ress->hval & 0x8000)? ress->hval & 0x7FFF: ress->hval | 0x8000;
		if(ress->historique.tampon.nb < ress->historique.tampon.max) {
			ress->historique.tampon.t.sval[(ress->historique.tampon.nb)++] = hval;
		} else {
			ress->historique.tampon.t.sval[(ress->historique.tampon.prem)++] = hval;
			if(ress->historique.tampon.prem >= ress->historique.tampon.max) ress->historique.tampon.prem = 0;
		}
		(numeriseur->interface).a_rafraichir = 1;
	}
}
/* ========================================================================== */
char NumeriseurProcExec(TypeNumeriseur *numeriseur, int p) {
	char nom_complet[MAXFILENAME];

	RepertoireIdentique(NumeriseurFichierLu,numeriseur->proc[p].fichier,nom_complet);
	printf("- Application de la procedure '%s' sur la %s\n",numeriseur->proc[p].nom,numeriseur->nom);
	NumeriseurAutoriseAcces(numeriseur,1,1);
	if(ScriptExecType(HW_NUMER,(void *)numeriseur,nom_complet,"  | ")) NumeriseurMontreModifs(numeriseur);
	NumeriseurAutoriseAcces(numeriseur,0,1);
	return(0);
}
/* ========================================================================== */
void NumeriseurRapportEtabli(TypeNumeriseur *numeriseur) {
	int i,k,l,item,prems,der; int bb; shex hval; /* float rval; */ char *texte;
	TypeBNModele *modele_bn;

	if(!numeriseur) {
		texte = NumeriseurEtatEntete;
	#ifdef NUMER_ETAT_CONDENSE
		l = snprintf(texte,NUMER_ETAT_LNGR,"%12s: BB [iden] T(C)  d2/d3  alim r1234 r1 r2  adc  Rt+g  dac          | detecteur","stamp");
	#else
//		l = snprintf(texte,NUMER_ETAT_LNGR,"%12s: BB [iden] T(C)  d2/d3 alim r1234 r1 r2 adc             Rt+g  dac                                  | detecteur","stamp");
		l = snprintf(texte,NUMER_ETAT_LNGR,"%12s: BB [iden] T(C)  d2/d3  alim r1234 r1 r2   A  B  C  D  H Rt+g   rA PA rB PB rC PC rD PD eH cH tH  - s1 s2 s3 s4 s5 s6 | detecteur","stamp");
	#endif
		// printf("(%s) Largeur panel entete: %d/%d\n",__func__,l,NUMER_ETAT_LNGR);
	} else {
		bb = numeriseur->bb; texte = NumeriseurEtat[bb].texte;
		modele_bn = &(ModeleBN[numeriseur->def.type]);
		memset((void *)texte,(int)' ',NUMER_ETAT_LNGR);
		texte[NUMER_ETAT_LNGR] = '\0';
		/* attention: snprintf introduit un 0 final, donc ne pas le sauter pour la colonne suivante */
		l = snprintf(texte,NUMER_ETAT_LNGR,
			"%12d: %02d",numeriseur->status.stamp,numeriseur->def.serial);
		if((k = modele_bn->rapport[NUMER_RAPPORT_IDENT]) >= 0) snprintf(texte+l,NUMER_ETAT_LNGR-l,
			" [%04X]",numeriseur->ress[k].hval); else snprintf(texte+l,NUMER_ETAT_LNGR-l," [....]"); l += 7;
		if((k = modele_bn->rapport[NUMER_RAPPORT_TEMPER]) >= 0) snprintf(texte+l,NUMER_ETAT_LNGR-l,
			"%5.1f",numeriseur->ress[k].val.r); else snprintf(texte+l,NUMER_ETAT_LNGR-l,"   .. "); l += 5;
		if((k = modele_bn->rapport[NUMER_RAPPORT_D2]) >= 0) snprintf(texte+l,NUMER_ETAT_LNGR-l,
			"%4d",numeriseur->ress[k].val.i); else snprintf(texte+l,NUMER_ETAT_LNGR-l,"...."); l += 4;
		if((k = modele_bn->rapport[NUMER_RAPPORT_D3]) >= 0) snprintf(texte+l,NUMER_ETAT_LNGR-l,
			"/%-4d ",numeriseur->ress[k].val.i); else snprintf(texte+l,NUMER_ETAT_LNGR-l,"/.... "); l += 6;
		if((k = modele_bn->rapport[NUMER_RAPPORT_ALIM]) >= 0) {
			hval = numeriseur->ress[k].hval;
			texte[l] = (hval & 1)? ((hval & 0x40)?'D':((hval & 0x20)?'R':'A')): 'o';
			texte[l+1] = (hval & 2)?'*':'b';
		}; l += 3;

		prems = NUMER_RAPPORT_REF; der = NUMER_RAPPORT_ADCON4;
		if((l + (der - prems + 2)) >= NUMER_ETAT_LNGR) goto hev;
		l++; for(item = prems; item <= der; item++) {
			if((k = modele_bn->rapport[item]) >= 0) texte[l] = numeriseur->ress[k].hval? '+': 'o'; l++;
		}
		l++;
	
		prems = NUMER_RAPPORT_R1; der = NUMER_RAPPORT_R2;
		if((l + (der - prems + 1)) >= NUMER_ETAT_LNGR) goto hev;
		for(item = prems; item <= der; item++) {
			i = item - NUMER_RAPPORT_R1;
			if((k = modele_bn->rapport[item]) >= 0) {
				snprintf(texte+l,NUMER_ETAT_LNGR-l," %c ",NumeriseurRapportRelais[numeriseur->ress[k].hval]);
			} else snprintf(texte+l,NUMER_ETAT_LNGR-l," . "); l += 3;
		}

		prems = NUMER_RAPPORT_ADCVAL1; der = NUMER_RAPPORT_ADCVAL5;
		if((l + (der - prems + 2)) >= NUMER_ETAT_LNGR) goto hev;
		for(item = prems; item <= der; item++) {
			// printf("(%s) Colonne %s (%d), ressource rapportee en colonne %d: %s (%d) = %04X\n",__func__,
			//	NumeriseurRapportItem[item],item,l,
			//	(modele_bn->rapport[item] >= 0)? modele_bn->ress[modele_bn->rapport[item]].nom:"neant",modele_bn->rapport[item],
			//	(modele_bn->rapport[item] >= 0)? numeriseur->ress[modele_bn->rapport[item]].hval: 0);
			//1 if((k = modele_bn->rapport[item]) >= 0) texte[l] = NUMER_RAPPORT_ADC(numeriseur->ress[k].val.i); l++;
			if((k = modele_bn->rapport[item]) >= 0) {
				hval = numeriseur->ress[k].hval;
			#ifdef NUMER_ETAT_CONDENSE
				texte[l] = NUMER_RAPPORT_HEXA(hval); l++;
			#else
				texte[l++] = ' ';
				texte[l] = NUMER_RAPPORT_HEXA1(hval); l++;
				texte[l] = NUMER_RAPPORT_HEXA2(hval); l++;
			#endif
			}
		}

		prems = NUMER_RAPPORT_RT; der = NUMER_RAPPORT_RG4;
		if((l + (der - prems + 2)) >= NUMER_ETAT_LNGR) goto hev;
		l++; for(item = prems; item <= der; item++) {
			if((k = modele_bn->rapport[item]) >= 0) {
				hval = numeriseur->ress[k].hval; texte[l] = (hval < 10)? ('0' + hval): ('A' + hval - 10);
			}; l++;
		}
		
		prems = NUMER_RAPPORT_DACVAL1; der = NUMER_RAPPORT_DACVAL12;
		if((l + (der - prems + 2)) >= NUMER_ETAT_LNGR) goto hev;
		l++; for(item = prems; item <= der; item++) {
			if((k = modele_bn->rapport[item]) >= 0) {
				// rval = numeriseur->ress[k].val.r; texte[l] = NUMER_RAPPORT_DAC_REEL(rval); l++; (ou, mieux, DAC_HEXA)
				hval = numeriseur->ress[k].hval; 
			#ifdef NUMER_ETAT_CONDENSE
				texte[l] = NUMER_RAPPORT_HEXA(hval); l++;
			#else
				texte[l++] = ' ';
				texte[l] = NUMER_RAPPORT_HEXA1(hval); l++;
				texte[l] = NUMER_RAPPORT_HEXA2(hval); l++;
			#endif
			}
		}
	
		prems = NUMER_RAPPORT_SLOW1; der = NUMER_RAPPORT_SLOW6;
		if((l + (der - prems + 2)) >= NUMER_ETAT_LNGR) goto hev;
		l++; for(item = prems; item <= der; item++) {
			if((k = modele_bn->rapport[item]) >= 0) {
				// rval = numeriseur->ress[k].val.r; texte[l] = NUMER_RAPPORT_DAC_REEL(rval); l++; (ou, mieux, DAC_HEXA)
				hval = numeriseur->ress[k].hval;
#ifdef NUMER_ETAT_CONDENSE
				texte[l] = NUMER_RAPPORT_HEXA(hval); l++;
#else
				texte[l++] = ' ';
				texte[l] = NUMER_RAPPORT_HEXA1(hval); l++;
				texte[l] = NUMER_RAPPORT_HEXA2(hval); l++;
#endif
			}
		}

		if(NumeriseurEtat[bb].detec && (l < NUMER_ETAT_LNGR)) {
			// printf("(%s) Largeur panel BB#%d: %d/%d + @%08X\n",__func__,bb,l,NUMER_ETAT_LNGR,(hexa)(NumeriseurEtat[bb].detec));
			k = snprintf(texte+l,NUMER_ETAT_LNGR-l," | %s",NumeriseurEtat[bb].detec); l += k;
			texte[NUMER_ETAT_LNGR] = '\0';
		}
	}
		
hev: texte[l] = '\0';
}
/* ========================================================================== */
int NumeriseurEtatEnCours(Instrum instrum, void *adrs) {
	int bb;
	printf("(%s) Affichage des numeriseurs %s (%sacquisition)\n",__func__,NumeriseurEtatDemande?"demande":"arrete",(Acquis[AcquisLocale].etat.active)?"pendant l'":"hors ");
	if(NumeriseurEtatDemande) {
		for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].status.demande) Numeriseur[bb].status.a_copier = 1;
		if(!Acquis[AcquisLocale].etat.active) {
			RepartiteursSelectionneVoiesLues(1,1); printf("\n"); RepartImprimeEchantillon(0);
			LectModeStatus = 1; NumeriseurEtatPeutStopper = 1;
			printf("(%s) Entree dans la boucle d'acquisition elementaire\n",__func__);
			LectAcqElementaire(NUMER_MODE_IDEM);
			printf("(%s) Sortie de la boucle d'acquisition elementaire\n",__func__);
			NumeriseurEtatDemande = 0;
		}
	} else {
		for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].status.demande) Numeriseur[bb].status.a_copier = 0;
		if(NumeriseurEtatPeutStopper) Acquis[AcquisLocale].etat.active = 0;
	}

	NumeriseurEtatPeutStopper = 0;
	printf("(%s) Sortie de la fonction avec affichage %s (%sacquisition)\n",__func__,NumeriseurEtatDemande?"demande":"arrete",(Acquis[AcquisLocale].etat.active)?"pendant l'":"hors ");
	return(0);
}
/* ========================================================================== */
static int NumeriseurRapportFerme(Cadre cdr, void *arg) {
	int bb;
	
	NumeriseurEtatDemande = 0;
	NumeriseurEtatEnCours(0,0);
	for(bb=0; bb<NumeriseurNb; bb++) Numeriseur[bb].status.demande = 0;
	return(1);
}
/* ========================================================================== */
int NumeriseurRapport(Menu menu, MenuItem item) {
	Panel p; Cadre precedent; Instrum instrum;
	TypeNumeriseur *numeriseur; TypeRepartiteur *repart; int bb,bolo,cap;
	int i,j,x,y; int groupe_precedent,groupe_courant;

	if(bNumeriseurEtat) {
		if(OpiumDisplayed(bNumeriseurEtat)) OpiumClear(bNumeriseurEtat);
		BoardTrash(bNumeriseurEtat);
	} else bNumeriseurEtat = BoardCreate();
	if(!bNumeriseurEtat) return(0);
	OpiumTitle(bNumeriseurEtat,"Etat Numeriseurs");
	x = Dx; y = Dy;

	p = PanelCreate(NumeriseurNb+1); // longeur affichee limitee par PanelMaxChamp(132); (appele en initialisation)
	PanelMode(p,PNL_RDONLY|PNL_DIRECT); PanelSupport(p,WND_CREUX);
	i = PanelText(p,"Entree",NumeriseurEtatEntete,NUMER_ETAT_LNGR);
	PanelItemSouligne(p,i,1);
	NumeriseurRapportEtabli(0);
	groupe_precedent = -1; groupe_courant = 0;
	for(j=0; j<NumeriseurNb; j++) {
		bb = OrdreNumer[j].num;
		if(bb < 0) { OpiumFail("Numeriseurs pas ordonnes"); return(0); }
		numeriseur = &(Numeriseur[bb]);
		// printf("(%s) Preparation de l'etat de la %s [%d]\n",__func__,numeriseur->nom,numeriseur->bb);
		numeriseur->status.demande = numeriseur->status.a_copier = 0;
		NumeriseurEtat[bb].detec = 0;
		if(!(repart = numeriseur->repart) || !numeriseur->avec_status) continue;
		if(!repart->local) continue;
		if(repart->famille == FAMILLE_IPE) {
			switch(ClassementCourant[CLASSE_NUMER]) {
			  case CLASSE_NOM: case CLASSE_FIBRE: case CLASSE_FISCHER: groupe_courant = j / 3; break;
			  case CLASSE_ENTREE: groupe_courant = IPE_FLT_ABS(repart,IPE_FLT_NUM(Numeriseur[bb].entree)); break;
			  case CLASSE_POSITION: case CLASSE_POSY: groupe_courant = (int)(OrdreNumer[j].ref) >> 4; break;
			}
			if((groupe_precedent >= 0) && (groupe_courant != groupe_precedent)) PanelItemSouligne(p,i,1);
		}
		repart->utile = 1;
		numeriseur->status.demande = 1;
		if(repart->famille == FAMILLE_IPE) {
			sprintf(NumeriseurEtat[bb].nom,"%s.%c%d/%s",repart->code_rep,IPE_CODE_ENTREE(repart,numeriseur->entree),
				IpeCodesAdcMode[repart->r.ipe.modeBB[numeriseur->entree]]);
		} else if(repart->famille == FAMILLE_OPERA) 
			sprintf(NumeriseurEtat[bb].nom,"%s.%d/%s",repart->code_rep,numeriseur->entree+1,OperaCodesAcquis[repart->r.opera.mode]);
		else sprintf(NumeriseurEtat[bb].nom,"%s.%d",repart->code_rep,numeriseur->entree+1);
		for(bolo=0; bolo<BoloNb; bolo++) {
			for(cap=0; cap<Bolo[bolo].nbcapt; cap++) if(Bolo[bolo].captr[cap].bb.num == bb) break;
			if(cap < Bolo[bolo].nbcapt) break;
		}
		NumeriseurEtat[bb].detec = (bolo < BoloNb)? Bolo[bolo].nom: 0;
		i = PanelText(p,NumeriseurEtat[bb].nom,NumeriseurEtat[bb].texte,NUMER_ETAT_LNGR);
		if(repart->famille == FAMILLE_IPE) groupe_precedent = groupe_courant;
		else PanelItemSouligne(p,i,(((i - 1) % 3) == 0));
		sprintf(NumeriseurEtat[bb].texte,"%12s: %02d","",numeriseur->def.serial);
	}
	BoardAddPanel(bNumeriseurEtat,p,x,y,0);
	
	precedent = p->cdr;
	p = /* pNumeriseurEtatDate = */ PanelCreate(2);
	PanelColumns(p,2); PanelMode(p,PNL_RDONLY|PNL_DIRECT|PNL_BYLINES); PanelSupport(p,WND_CREUX);
	PanelInt (p,"Synchro",&NumeriseurEtatStamp);
	PanelText(p,"a",NumeriseurEtatDate,12);
	BoardAddPanel(bNumeriseurEtat,p,OPIUM_EN_DESSOUS_DE precedent);

	precedent = p->cdr;
	iNumeriseurEtatMaj = instrum = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&NumeriseurEtatDemande,"fige/a jour");
	InstrumOnChange(instrum,NumeriseurEtatEnCours,0);
	OpiumSupport(instrum->cdr,WND_RAINURES); InstrumTitle(instrum,"Status");
	BoardAddInstrum(bNumeriseurEtat,instrum,OPIUM_A_DROITE_DE precedent);
	precedent = instrum->cdr;
	
	OpiumExitFctn(bNumeriseurEtat,NumeriseurRapportFerme,0);
	OpiumFork(bNumeriseurEtat);

	return(0);
}
#pragma mark --- actions utilisateur ---
/* ========================================================================== */
static void NumeriseurRessFinalise(TypeNumeriseur *numeriseur, int j, int k) {
    TypeReglage *reglage; int i;

	/* TypeBNModele *modele_bn; modele_bn = &(ModeleBN[numeriseur->def.type]);
	char mode[12]; ArgKeyGetText(ACCES_FORMATS,modele_bn->acces.mode,mode,12);
	printf("(%s) Finalise la ressource en mode %s pour %s.%s\n",__func__,mode,numeriseur->nom,modele_bn->ress[k].nom); */
	NumeriseurAutoriseAcces(numeriseur,1,2);
	NumeriseurRessourceCharge(numeriseur,k,""); // "*");
	if((reglage = NumeriseurInstr[j].reglage) != 0) {
		ReglageFinalise(reglage);
		// printf("(%s) Reglage de %s.%s finalise\n",__func__,numeriseur->nom,reglage->nom);
	} else for(i=0; i<NumeriseurInstrNb; i++) if((NumeriseurInstr[i].numeriseur == numeriseur) && (NumeriseurInstr[i].num == k)) {
		switch(NumeriseurInstr[i].type) {
			case OPIUM_INSTRUM: if(OpiumDisplayed(((Instrum)NumeriseurInstr[i].instrum)->cdr)) InstrumRefreshVar((Instrum)NumeriseurInstr[i].instrum); break;
			case OPIUM_PANEL: if(OpiumDisplayed(((Panel)NumeriseurInstr[i].instrum)->cdr)) PanelRefreshVars((Panel)NumeriseurInstr[i].instrum); break;
		}
	}
	NumeriseurAutoriseAcces(numeriseur,0,2);
	numeriseur->ress[k].a_charger = 0;
	// printf("(%s) Modif de la ressource %s.%d terminee\n",__func__,numeriseur->nom,k);
}
/* ========================================================================== */
static int NumeriseurRessTouche(int j, char adu) {
	int k,bb;
	TypeBNModele *modele_bn; TypeNumeriseur *numeriseur; TypeBNModlRessource *ress;
	shex hval;

	// printf("(%s) reglage #%d, mode %d\n",__func__,j,adu);
	numeriseur = NumeriseurInstr[j].numeriseur;
	k = NumeriseurInstr[j].num;
	modele_bn = &(ModeleBN[numeriseur->def.type]); ress = modele_bn->ress + k;
	if(adu < 0) /* special cles, valeur prise directement en numerique et non d'apres le texte */ {
		int ival;
		ival = numeriseur->ress[k].val.i;
		ArgKeyGetText(ress->lim_k.cles,ival,numeriseur->ress[k].cval,NUMER_RESS_VALTXT);
		hval = ress->lim_k.hval[ival];
		numeriseur->ress[k].hval = hval;
	} else if(adu == 2) NumeriseurRessValChangee(numeriseur,k);
	else if(adu == 1) NumeriseurRessHexaChange(numeriseur,k);
	else NumeriseurRessUserChange(numeriseur,k);

	NumeriseurRessFinalise(numeriseur,j,k);
	if(NumeriseursDiffusesNb > 1) {
		int nb;
		nb = 1;
		for(bb=0; bb<NumeriseurNb; bb++) if(NumeriseurDiffuse[bb] && (bb != numeriseur->bb)) {
			strcpy(Numeriseur[bb].ress[k].cval,numeriseur->ress[k].cval);
			NumeriseurRessUserChange(&(Numeriseur[bb]),k);
			NumeriseurRessFinalise(&(Numeriseur[bb]),j,k);
			if(++nb >= NumeriseursDiffusesNb) break;
		}
	}
	return(0);
}
/* ========================================================================== */
int NumeriseurRessFromVal(void *instrum, void *arg) {
	return(NumeriseurRessTouche((int)arg,2));
}
/* ========================================================================== */
int NumeriseurRessFromADU(Panel panel, int num, void *arg) {
	return(NumeriseurRessTouche((int)arg,1)); // arg == PanelItemArg((Panel)panel,num))
}
/* ========================================================================== */
int NumeriseurRessFromUser(Panel panel, int num, void *arg) {
	return(NumeriseurRessTouche((int)arg,0));
}
/* ========================================================================== */
int NumeriseurRessFromKey(Panel panel, int num, void *arg) {
	return(NumeriseurRessTouche((int)arg,-1));
}
/* ========================================================================== */
char NumeriseurRessEnCours(void *adrs, char *texte, void *arg) {
	int j; TypeNumeriseur *numeriseur; int k;

	j = (int)arg;
	numeriseur = NumeriseurInstr[j].numeriseur;
	k = NumeriseurInstr[j].num;
	numeriseur->ress[k].a_charger = 1;
	// printf("Modif en cours du reglage %s.%d\n",numeriseur->nom,k);
	return(1);
}
/* ========================================================================== */
int NumeriseurRessDown(Menu menu) {
	int j; TypeNumeriseur *numeriseur; int k;
	TypeBNModele *modele_bn; TypeBNModlRessource *ress;

	for(j=0; j<NumeriseurInstrNb; j++) if((Menu)NumeriseurInstr[j].instrum == menu) break;
	if(j >= NumeriseurInstrNb) return(0);
	numeriseur = NumeriseurInstr[j].numeriseur;
    k = NumeriseurInstr[j].num;
	numeriseur->ress[k].a_charger = 1;
	// printf("Modif en cours du reglage %s.%d\n",numeriseur->nom,k);
	modele_bn = &(ModeleBN[numeriseur->def.type]);
	ress = &(modele_bn->ress[k]);
	if(ress->type == RESS_CLES) {
		numeriseur->ress[k].val.i -= 1;
		if(numeriseur->ress[k].val.i < 0) numeriseur->ress[k].val.i = ress->lim_k.nb - 1;
		NumeriseurRessTouche(j,-1);
	} else if(modele_bn->acces.mode == NUMER_ACCES_BB) {
		if(numeriseur->ress[k].hval) numeriseur->ress[k].hval -= 1;
		NumeriseurRessTouche(j,1);
	} else if(ress->type == RESS_INT) {
		numeriseur->ress[k].val.i -= ress->bit0;
		if(numeriseur->ress[k].val.i < ress->lim_i.min) numeriseur->ress[k].val.i = ress->lim_i.min;
		NumeriseurRessTouche(j,2);
	} else if(ress->type == RESS_FLOAT) {
		numeriseur->ress[k].val.r -= (float)(ress->bit0);
		if(numeriseur->ress[k].val.r < ress->lim_r.min) numeriseur->ress[k].val.r = ress->lim_r.min;
		NumeriseurRessTouche(j,2);
	}
	return(0);
}
/* ========================================================================== */
int NumeriseurRessUp(Menu menu) {
	int j; TypeNumeriseur *numeriseur; int k;
	TypeBNModele *modele_bn; TypeBNModlRessource *ress;
	
	for(j=0; j<NumeriseurInstrNb; j++) if((Menu)NumeriseurInstr[j].instrum == menu) break;
	if(j >= NumeriseurInstrNb) return(0);
	numeriseur = NumeriseurInstr[j].numeriseur;
    k = NumeriseurInstr[j].num;
	numeriseur->ress[k].a_charger = 1;
	// printf("Modif en cours du reglage %s.%d\n",numeriseur->nom,k);
	modele_bn = &(ModeleBN[numeriseur->def.type]);
	ress = &(modele_bn->ress[k]);
	if(ress->type == RESS_CLES) {
		numeriseur->ress[k].val.i += 1;
		if(numeriseur->ress[k].val.i >= ress->lim_k.nb) numeriseur->ress[k].val.i = 0;
		NumeriseurRessTouche(j,-1);
	} else if(modele_bn->acces.mode == NUMER_ACCES_BB) {
		if(numeriseur->ress[k].hval < ress->masque) numeriseur->ress[k].hval += 1;
		NumeriseurRessTouche(j,1);
	} else if(ress->type == RESS_INT) {
		numeriseur->ress[k].val.i += ress->bit0;
		if(numeriseur->ress[k].val.i > ress->lim_i.max) numeriseur->ress[k].val.i = ress->lim_i.max;
		NumeriseurRessTouche(j,2);
	} else if(ress->type == RESS_FLOAT) {
		numeriseur->ress[k].val.r += (float)(ress->bit0);
		if(numeriseur->ress[k].val.r < ress->lim_r.max) numeriseur->ress[k].val.r = ress->lim_r.max;
		NumeriseurRessTouche(j,2);
	}
	return(0);
}
/* ========================================================================== */
static int NumeriseurDiffusion(Menu menu, MenuItem item) {
	int bb; int type; int rc;

	if(!NumeriseurNb) { OpiumError("Pas de numeriseur connu"); return(0); }
	type = ((TypeNumeriseur *)(NumeriseurChoix.adrs))->def.type;
	/* for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].def.type == type) NumeriseurDiffuse[bb] = 1;
	NumeriseursDiffusesNb = 0;
	if(OpiumExec(pNumeriseursDiffuses->cdr) == PNL_CANCEL) {
		for(bb=0; bb<NumeriseurNb; bb++) NumeriseurDiffuse[bb] = 0;
		return(0);
	}
	for(bb=0; bb<NumeriseurNb; bb++) if(NumeriseurDiffuse[bb]) {
		if(Numeriseur[bb].def.type != type) {
			OpiumError("Les types %s et %s sont melanges, et c'est interdit",ModeleBN[type].nom,ModeleBN[Numeriseur[bb].def.type].nom);
			NumeriseurDiffuse[bb] = 0;
		} else NumeriseursDiffusesNb++;
	} */
	NumeriseursDiffusesNb = 0;
	for(bb=0; bb<NumeriseurNb; bb++) Numeriseur[bb].sel = NumeriseurDiffuse[bb] = 0;
	((TypeNumeriseur *)(NumeriseurChoix.adrs))->sel = 1;
	if((rc = NumeriseurSelection(1))) {
		if(ModlNumerChoisi != type) {
			OpiumError("Le type de numeriseur devrait etre le meme que celui affiche (%s), et non %s",
				ModeleBN[type].nom,ModeleBN[ModlNumerChoisi].nom);
			return(0);
		}
		for(bb=0; bb<NumeriseurNb; bb++) if((NumeriseurDiffuse[bb] = Numeriseur[bb].sel)) NumeriseursDiffusesNb++;
	}

	if(NumeriseursDiffusesNb > 1) {
		MenuItemSelect(mNumeriseurBoutons,1,"",0);
		MenuItemSelect(mNumeriseurBoutons,2,"",0);
		MenuItemSelect(mNumeriseurBoutons,3,"",0);
		MenuItemSelect(mNumeriseurBoutons,4,"",0);
		MenuItemSelect(mNumeriseurBoutons,5,"",0);
		MenuItemSelect(mNumeriseurBoutons,6,"",0);
		MenuItemAllume(mNumeriseurDiffusion,1,0,GRF_RGB_GREEN);
	} else {
		MenuItemSelect(mNumeriseurBoutons,1,"Relire",1);
		MenuItemSelect(mNumeriseurBoutons,2,"Defaut",1);
		MenuItemSelect(mNumeriseurBoutons,3,"RAZ",1);
		MenuItemSelect(mNumeriseurBoutons,4,"Init",1);
		MenuItemSelect(mNumeriseurBoutons,5,"Correctifs",1);
		MenuItemSelect(mNumeriseurBoutons,6,"Sauver",1);
		MenuItemEteint(mNumeriseurDiffusion,1,0);
	}
	return(0);
}
/* ========================================================================== */
static int NumeriseurChange(Menu menu, MenuItem item) {
	int bb; int x,y;

	if(!NumeriseurNb) { OpiumError("Pas de numeriseur sauve"); return(0); }
	bb = NumeriseurChoix.bb;
	// if(OpiumReadList("Ident",NumeriseurListe,&NumeriseurChoix.bb,NUMER_NOM) == PNL_CANCEL) return(0);
	if(Numeriseur[bb].interface.planche) {
		OpiumGetPosition(Numeriseur[bb].interface.planche,&x,&y);
		OpiumPosition(pNumeriseurChoix->cdr,x,y+WndLigToPix(7));
	}
	if(OpiumExec(pNumeriseurChoix->cdr) == PNL_CANCEL) return(0);
	if(bb != NumeriseurChoix.bb) {
		if(Numeriseur[bb].interface.planche) OpiumClear(Numeriseur[bb].interface.planche);
		NumeriseurAfficheChoix(&(Numeriseur[NumeriseurChoix.bb]));
	}
	return(0);
}
#pragma mark --- fenetre de connection ---
/* ========================================================================== */
INLINE static char NumeriseurRessourceCandidate(TypeBNModele *modele_bn, int k, NUMER_TRACE_TYPE t) {
	char candidate;
	
	switch(t) {
		case NUMER_TRACE_DAC: candidate = (modele_bn->ress[k].categ == RESS_AUXI); break;
		case NUMER_TRACE_ADC: candidate = (modele_bn->ress[k].categ == RESS_ADC); break;
		default: candidate = 0; break;
	}
	return(candidate);
}
/* ========================================================================== */
static char NumeriseurPrepareTrace(TypeNumeriseur *numeriseur, NUMER_TRACE_TYPE t) {
	int i,k,m,n;
	int c; OpiumColorState s; /* suivi des couleurs   */
	TypeBNModele *modele_bn; TypeTamponDonnees *tampon;
	Panel p; int liste[NUMER_RESS_MAX]; char ok[NUMER_RESS_MAX]; int rc; char tampons_changes;
	Graph g; int x,y; char titre[80];

	modele_bn = &(ModeleBN[numeriseur->def.type]);
	p = PanelCreate(modele_bn->nbress);
	i = 0; m = 0;
	for(k=0; k<modele_bn->nbress; k++) {
		if(NumeriseurRessourceCandidate(modele_bn,k,t)) {
			liste[i] = k; ok[i] = (numeriseur->ress[k].historique.tampon.t.sval)? 1: 0;
			if(ok[i]) m++;
			PanelBool(p,modele_bn->ress[k].nom,&(ok[i]));
			i++;
		}
	};
	n = i;
	if(!m) for(i=0; i<n; i++) ok[i] = 1;
	/* patch special BBv2 EDW version complete */
	if(!m) {
		for(i=0; i<n; i++) ok[i] = 0;
		switch(t) {
			case NUMER_TRACE_DAC: ok[0] = ok[2] = 1; break;
			case NUMER_TRACE_ADC: ok[0] = ok[1] = 1; break;
			default: break;
		}
	} /* fin patch */
	rc = OpiumExec(p->cdr);
	PanelDelete(p);
	if(rc == PNL_CANCEL) return(0);
	printf("%s/ Demande de trace des %ss\n",DateHeure(),NumeriseurNomTrace[t]);
	tampons_changes = 0;
	m = 0;
	for(i=0; i<n; i++) {
		k = liste[i];
		tampon = &(numeriseur->ress[k].historique.tampon);
		if(ok[i]) {
			printf("          . Trace de %s\n",modele_bn->ress[k].nom);
			/* affecter numeriseur->ress[k].historique.tampon.val */
			tampon->dim = NumeriseurTraceLarg;
			if(tampon->t.sval && (tampon->dim != tampon->max)) {
				free(tampon->t.sval);
				tampon->t.sval = 0; tampon->max = 0;
				tampons_changes = 1;
			}
			if(!tampon->t.sval) {
				tampon->t.sval = (TypeDonnee *)malloc(tampon->dim * sizeof(TypeDonnee));
				if(tampon->t.sval) { tampon->max = tampon->dim; m++; tampons_changes = 1; }
				else tampon->max = 0;
				tampon->type = TAMPON_INT16;
			} else m++;
			tampon->nb = 0;
			tampon->prem = 0;
		} else if(tampon->t.sval) {
			free(tampon->t.sval);
			tampon->t.sval = 0; tampon->max = 0;
			tampons_changes = 1;
		}
	}
	if(m) {
		if(!(g = numeriseur->trace[t].g)) {
			g = numeriseur->trace[t].g = GraphCreate(NumeriseurTraceVisu,NumeriseurTraceHaut,m+1);
			if(g) {
				sprintf(titre,"%ss %s",NumeriseurNomTrace[t],numeriseur->nom);
				OpiumTitle(g->cdr,titre);
			}
		}
		if(g) {
			if(tampons_changes) {
				// printf("graphique pour la trace de type %d @%08X\n",t,g);
				GraphErase(g);
				x = GraphAdd(g,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,NumeriseurStatusHorloge,NumeriseurTraceLarg);
				GraphDataName(g,x,"t(s)");
				OpiumColorInit(&s);
				for(i=0; i<n; i++) if(ok[i]) {
					k = liste[i];
					tampon = &(numeriseur->ress[k].historique.tampon);
					if(tampon->t.sval) {
						y = GraphAdd(g,GRF_SHORT|GRF_YAXIS,tampon->t.sval,tampon->max);
						GraphDataName(g,y,modele_bn->ress[k].nom);
						c = OpiumColorNext(&s);
						GraphDataRGB(g,y,OpiumColorRGB(c));
						numeriseur->ress[k].historique.g = g;
						// printf("graphique pour %s @%08X\n",modele_bn->ress[k].nom,g);
						numeriseur->ress[k].historique.y = y;
					}
				}
				GraphAxisAutoRange(g,GRF_YAXIS);
				GraphMode(g,GRF_2AXES | GRF_LEGEND);
				GraphParmsSave(g);
			}
			GraphUse(g,0);
			numeriseur->trace[t].nb = 0;
		}
	} else printf("          . Aucune ressource selectionnee\n");
	return(1);
}
/* ========================================================================== */
static void NumeriseurEffaceTrace(TypeNumeriseur *numeriseur, NUMER_TRACE_TYPE t) {
	TypeBNModele *modele_bn; int k; TypeTamponDonnees *tampon;

	if(numeriseur->trace[t].g) printf("%s/ Arret du trace des %ss\n",DateHeure(),NumeriseurNomTrace[t]);
	modele_bn = &(ModeleBN[numeriseur->def.type]);
	for(k=0; k<modele_bn->nbress; k++) {
		if(NumeriseurRessourceCandidate(modele_bn,k,t)) {
			tampon = &(numeriseur->ress[k].historique.tampon);
			if(tampon->t.sval) {
				free(tampon->t.sval);
				tampon->t.sval = 0; tampon->max = 0;
			}
		}
	}
	if(numeriseur->trace[t].g) {
		GraphDelete(numeriseur->trace[t].g);
		numeriseur->trace[t].g = 0;
	}
}
/* ========================================================================== */
static int NumeriseurAcquiert(Instrum instrum, void *adrs) {
	TypeNumeriseur *numeriseur; TypeRepartiteur *repart; int l; int rep;
	char local[REPART_MAX];

	numeriseur = (TypeNumeriseur *)adrs;
	if(!numeriseur) {
		OpiumError("Numeriseur non defini");
		return(0);
	}
	if(!(repart = numeriseur->repart)) {
		OpiumError("Le repartiteur sense transmettre %s, n'est pas defini",numeriseur->nom);
		return(0);
	}
	l = numeriseur->entree;
	if(repart->nbentrees <= l) {
		OpiumError("Le repartiteur %s sense transmettre %s sur son entree %d, n'en a que %d",repart->nom,numeriseur->nom,l+1,repart->nbentrees);
		return(0);
	}	
	printf("  . %s lu via %s[%d]\n",numeriseur->nom,repart->nom,l);
	repart->status_demande = numeriseur->status.a_copier; //a numeriseur->status.demande;
	repart->status_relu = 0;
	if(/*a numeriseur->status.demande */ numeriseur->status.a_copier) {
		LectLitStatus = 1;
		//a numeriseur->status.a_copier = 1;
		if((numeriseur->interface).planche) {
			if(OpiumDisplayed((numeriseur->interface).planche)) BoardRefreshVars((numeriseur->interface).planche);
		}
		if(iRepartRafraichi && OpiumDisplayed(iRepartRafraichi->cdr)) InstrumRefreshVar(iRepartRafraichi);
		if(pRepartDonnees && OpiumDisplayed(pRepartDonnees->cdr)) PanelRefreshVars(pRepartDonnees);
		if(!Acquis[AcquisLocale].etat.active) {
			LectModeStatus = 1; NumeriseurPeutStopper = 1;
			for(rep=0; rep<RepartNb; rep++) { local[rep] = Repart[rep].local; Repart[rep].local = Repart[rep].utile = 0; }
			repart->local = repart->utile = 1; RepartiteursSelectionneVoiesLues(1,1); printf("\n"); RepartImprimeEchantillon(0);
			LectAcqElementaire(NUMER_MODE_IDEM);
			for(rep=0; rep<RepartNb; rep++) Repart[rep].local = local[rep];
		}
	} else if(NumeriseurPeutStopper) Acquis[AcquisLocale].etat.active = 0;

	//a numeriseur->status.a_copier = numeriseur->status.demande;
	NumeriseurPeutStopper = 0;
	RepartiteurRafraichi();
	
	return(0);
}
/* ========================================================================== */
char NumeriseurRafraichi(TypeNumeriseur *numeriseur) {
	TypeNumeriseurCmde *interface;
	int nbress,k; NUMER_TRACE_TYPE t;
	Cadre cdr; Graph g; int y; hexa zoom;
	char bn_affiches,doit_terminer;

	// numeriseur->status.demande = Acquis[AcquisLocale].etat.active;
	numeriseur->status.a_copier = Acquis[AcquisLocale].etat.active;
	bn_affiches = 0;
	interface = &(numeriseur->interface);
	if(interface->planche) {
		if(OpiumDisplayed(interface->planche)) {
			bn_affiches = 1;
			if(!numeriseur->status.termine) return(bn_affiches);
			if(interface->a_rafraichir && !(interface->verrouillee)) {
				doit_terminer = OpiumRefreshBegin(interface->planche);
				nbress = ModeleBN[numeriseur->def.type].nbress;
				//? for(k=0; k<nbress; k++) if(!numeriseur->ress[k].a_charger) NumeriseurRessHexaChange(numeriseur,k);
				BoardRefreshVars(interface->planche);
				for(k=0; k<nbress; k++) if(numeriseur->ress[k].historique.tampon.t.sval) {
					if((g = numeriseur->ress[k].historique.g)) {
						for(t=0; t<NUMER_TRACE_NB; t++) if((g == numeriseur->trace[t].g) && NumeriseurTracer[t]) break;
						if(t >= NUMER_TRACE_NB) continue;
						y = numeriseur->ress[k].historique.y;
						GraphDataUse(g,y,numeriseur->ress[k].historique.tampon.nb);
						if(numeriseur->trace[t].nb < numeriseur->ress[k].historique.tampon.nb) {
							numeriseur->trace[t].nb = numeriseur->ress[k].historique.tampon.nb;
							GraphDataUse(g,0,numeriseur->trace[t].nb);
							if(numeriseur->trace[t].nb > NumeriseurTraceVisu) {
								zoom = ((numeriseur->trace[t].nb - 1) / NumeriseurTraceVisu) + 1; GraphZoom(g,zoom,1);
							}
						}
						GraphDataRescroll(g,y,numeriseur->ress[k].historique.tampon.prem);
					}
				};
				if(doit_terminer) OpiumRefreshEnd(interface->planche);
				for(t=0; t<NUMER_TRACE_NB; t++) if(NumeriseurTracer[t] && (g = numeriseur->trace[t].g)) {
					cdr = g->cdr; OpiumScrollXmax(cdr); OpiumDisplay(cdr);
				}
				interface->a_rafraichir = 0;
			}
			numeriseur->status.termine = 0;
		}
	}
	return(bn_affiches);
}
/* ========================================================================== */
void NumeriseurArreteTraces(TypeNumeriseur *numeriseur) {
	NUMER_TRACE_TYPE t;

	for(t=0; t<NUMER_TRACE_NB; t++) {
		NumeriseurTracer[t] = 0;
		NumeriseurEffaceTrace(numeriseur,t);
	}
}
/* ========================================================================== */
static int NumeriseurFerme(Cadre cdr, void *adrs) {
	TypeNumeriseur *numeriseur; TypeRepartiteur *repart;
	
	if((numeriseur = (TypeNumeriseur *)adrs)) {
		printf("%s/ ----- Fin d'utilisation de la %s\n",DateHeure(),numeriseur->nom);
		NumeriseurArreteTraces(numeriseur);
		//a numeriseur->status.a_copier = numeriseur->status.demande;
		//	int bb; for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].status.a_copier) break;
		//	if(bb >= NumeriseurNb) NumeriseurPeutStopper = 1;
		if(NumeriseurPeutStopper) {
			Acquis[AcquisLocale].etat.active = 0;
			NumeriseurPeutStopper = 0;
			RepartiteurRafraichi();
		}
		if((repart = numeriseur->repart)) {
			if(repart->famille == FAMILLE_IPE) IpeAutoriseFibre(repart,numeriseur->entree,0);
		}
	}
	if(NumeriseursDiffusesNb > 1) {
		MenuItemSelect(mNumeriseurBoutons,1,"Relire",1);
		MenuItemSelect(mNumeriseurBoutons,2,"Defaut",1);
		MenuItemSelect(mNumeriseurBoutons,3,"RAZ",1);
		MenuItemSelect(mNumeriseurBoutons,4,"Init",1);
		MenuItemSelect(mNumeriseurBoutons,5,"Correctifs",1);
		MenuItemSelect(mNumeriseurBoutons,6,"Sauver",1);
		MenuItemEteint(mNumeriseurDiffusion,1,0);
		NumeriseursDiffusesNb = 0;
	}
	
	return(1);
}
/* ========================================================================== */
static int NumeriseurTracage(Instrum instrum, void *adrs) {
	TypeNumeriseurTrace *trace; TypeNumeriseur *numeriseur;
	NUMER_TRACE_TYPE t;

	trace = (TypeNumeriseurTrace *)adrs;
	numeriseur = (TypeNumeriseur *)(trace->numeriseur);
	t = trace->type;
	if(NumeriseurTracer[t]) {
		if(NumeriseurPrepareTrace(numeriseur,t)) {
			numeriseur->status.a_copier = 1; // numeriseur->status.demande = 1;
			NumeriseurAcquiert(0,(void *)numeriseur);
		} else NumeriseurTracer[t] = 0;
	} // else NumeriseurEffaceTrace(numeriseur,t);

	return(0);
}
/* ========================================================================== */
static TypeNumeriseur *NumeriseurOrigineDuPave(Menu menu) {
	TypeNumeriseur *numeriseur; int bb;

	numeriseur = 0;
	for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].interface.pave == menu) break;
	if(bb < NumeriseurNb) numeriseur = &(Numeriseur[bb]);
	else {
		for(bb=0; bb<NUMER_ANONM_MAX; bb++) if(NumeriseurAnonyme[bb].interface.pave == menu) break;
		if(bb < NUMER_ANONM_MAX) numeriseur = &(NumeriseurAnonyme[bb]);
	}
	if(!numeriseur) {
		OpiumError("! Ce pave de commandes est deconnecte");
		printf("! Menu appele: @%08X\n",(hexa)menu);
		printf("! Connection des numeriseurs connus:");
		if(NumeriseurNb) for(bb=0; bb<NumeriseurNb; bb++)
			printf("\n! @%08X: %s",(hexa)(Numeriseur[bb].interface.pave),Numeriseur[bb].nom);
		else printf(" neant");
		printf("\n! Connection des nouveaux numeriseurs:\n");
		for(bb=0; bb<NUMER_ANONM_MAX; bb++)
			printf("! @%08X: %s\n",(hexa)(NumeriseurAnonyme[bb].interface.pave),NumeriseurAnonyme[bb].nom);
	}
	return(numeriseur);
}
/* ========================================================================== */
static TypeNumeriseur *NumeriseurOrigineDuBouton(Menu menu) {
	TypeNumeriseur *numeriseur; int bb;
	
	numeriseur = 0;
	for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].interface.bouton == menu) break;
	if(bb < NumeriseurNb) numeriseur = &(Numeriseur[bb]);
	else {
		for(bb=0; bb<NUMER_ANONM_MAX; bb++) if(NumeriseurAnonyme[bb].interface.bouton == menu) break;
		if(bb < NUMER_ANONM_MAX) numeriseur = &(NumeriseurAnonyme[bb]);
	}
	if(!numeriseur) {
		OpiumError("! Ce bouton de commandes est deconnecte");
		printf("! Menu appele: @%08X\n",(hexa)menu);
		printf("! Connection des numeriseurs connus:");
		if(NumeriseurNb) for(bb=0; bb<NumeriseurNb; bb++)
			printf("\n! @%08X: %s",(hexa)(Numeriseur[bb].interface.bouton),Numeriseur[bb].nom);
		else printf(" neant");
		printf("\n! Connection des nouveaux numeriseurs:\n");
		for(bb=0; bb<NUMER_ANONM_MAX; bb++)
			printf("! @%08X: %s\n",(hexa)(NumeriseurAnonyme[bb].interface.bouton),NumeriseurAnonyme[bb].nom);
	}
	return(numeriseur);
}
/* ========================================================================== */
static TypeNumeriseur *NumeriseurOrigineDuBlocage(Menu menu) {
	TypeNumeriseur *numeriseur; int bb;
	
	numeriseur = 0;
	for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].interface.blocage == menu) break;
	if(bb < NumeriseurNb) numeriseur = &(Numeriseur[bb]);
	else {
		for(bb=0; bb<NUMER_ANONM_MAX; bb++) if(NumeriseurAnonyme[bb].interface.blocage == menu) break;
		if(bb < NUMER_ANONM_MAX) numeriseur = &(NumeriseurAnonyme[bb]);
	}
	if(!numeriseur) {
		OpiumError("! Ce pave de commandes est deconnecte");
		printf("! Menu appele: @%08X\n",(hexa)menu);
		printf("! Connection des numeriseurs connus:");
		if(NumeriseurNb) for(bb=0; bb<NumeriseurNb; bb++)
			printf("\n! @%08X: %s",(hexa)(Numeriseur[bb].interface.blocage),Numeriseur[bb].nom);
		else printf(" neant");
		printf("\n! Connection des nouveaux numeriseurs:\n");
		for(bb=0; bb<NUMER_ANONM_MAX; bb++)
			printf("! @%08X: %s\n",(hexa)(NumeriseurAnonyme[bb].interface.blocage),NumeriseurAnonyme[bb].nom);
	}
	return(numeriseur);
}
/* ========================================================================== */
static int NumeriseurDebloque(Menu menu, MenuItem item) {
	TypeNumeriseur *numeriseur;

	if(NumeriseursDiffusesNb > 1) {
		int bb,nb;
		nb = 0;
		for(bb=0; bb<NumeriseurNb; bb++) if(NumeriseurDiffuse[bb]) {
			NumeriseurAutoriseAcces(&(Numeriseur[bb]),1,1);
			if(++nb >= NumeriseursDiffusesNb) break;
		}
	} else if((numeriseur = NumeriseurOrigineDuBlocage(menu)))
		NumeriseurAutoriseAcces(numeriseur,1,1);
	return(0);
}
/* ========================================================================== */
static int NumeriseurBloque(Menu menu, MenuItem item) {
	TypeNumeriseur *numeriseur;
	
	if(NumeriseursDiffusesNb > 1) {
		int bb,nb;
		nb = 0;
		for(bb=0; bb<NumeriseurNb; bb++) if(NumeriseurDiffuse[bb]) {
			NumeriseurAutoriseAcces(&(Numeriseur[bb]),0,1);
			if(++nb >= NumeriseursDiffusesNb) break;
		}
	} else if((numeriseur = NumeriseurOrigineDuBlocage(menu)))
		NumeriseurAutoriseAcces(numeriseur,0,1);
	return(0);
}
/* ========================================================================== */
static int NumeriseurEnvoiDACs(Menu menu, MenuItem item) {
	TypeNumeriseur *numeriseur;
	
	if(NumeriseursDiffusesNb > 1) {
		int bb,nb;
		nb = 0;
		for(bb=0; bb<NumeriseurNb; bb++) if(NumeriseurDiffuse[bb]) {
			NumeriseurAutoriseBB2(&(Numeriseur[bb]),-1,1);
			if(++nb >= NumeriseursDiffusesNb) break;
		}
	} else if((numeriseur = NumeriseurOrigineDuBlocage(menu)))
		NumeriseurAutoriseBB2(numeriseur,-1,1);
	return(0);
}
/* ========================================================================== */
static char NumeriseurProcEnCours(void *adrs, char *texte, void *arg) {
	TypeNumeriseur *numeriseur; TypeNumeriseurCmde *interface;
	
	numeriseur = (TypeNumeriseur *)arg; // numeriseur->status.demande = numeriseur->status.a_copier = 0;
	interface = &(numeriseur->interface); interface->verrouillee = 1;
	return(1);
}
/* ========================================================================== */
static int NumeriseurProcAnnulee(Panel panel, void *arg) {
	TypeNumeriseur *numeriseur; TypeNumeriseurCmde *interface;
	
	numeriseur = (TypeNumeriseur *)arg; // numeriseur->status.demande = 1;
	interface = &(numeriseur->interface); interface->verrouillee = 0;
	return(1);
}
/* ========================================================================== */
static int NumeriseurProcAppelee(Panel panel, void *adrs) {
	TypeNumeriseur *numeriseur;
	TypeNumeriseurCmde *interface;
	int numproc;

	numeriseur = (TypeNumeriseur *)adrs;
	if(NumeriseursDiffusesNb > 1) {
		int bb,nb;
		numproc = numeriseur->numproc; /* !! suppose que tous ces numeriseurs ont la meme liste de procedures !! */
		nb = 0;
		for(bb=0; bb<NumeriseurNb; bb++) if(NumeriseurDiffuse[bb]) {
			numeriseur = &(Numeriseur[bb]);
			// numeriseur->status.demande = 1;
			interface = &(numeriseur->interface); interface->verrouillee = 0;
			NumeriseurProcExec(numeriseur,numproc);
			if(++nb >= NumeriseursDiffusesNb) break;
		}
	} else {
		// numeriseur->status.demande = 1;
		interface = &(numeriseur->interface); interface->verrouillee = 0;
		NumeriseurProcExec(numeriseur,numeriseur->numproc);
	}
	return(0);
}
/* ========================================================================== */
static int NumeriseurRecharge(Menu menu, MenuItem item) {
	int bb; int l; TypeRepartiteur *repart; FILE *f;
	
	for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].interface.pave == menu) break;
	if(bb < NumeriseurNb) {
		f = fopen(NumeriseurFichierLu,"r");
		NumeriseursRelit(f,bb);
		repart = Numeriseur[bb].repart; l = Numeriseur[bb].entree;
		Numeriseur[bb].repart = repart; Numeriseur[bb].entree = l;
		NumeriseurMontreModifs(&(Numeriseur[bb]));
	} else OpiumError("Ce numeriseur n'etait pas declare dans le fichier\n");
	return(0);
}
/* ========================================================================== */
static int NumeriseurDefaut(Menu menu, MenuItem item) {
	TypeNumeriseur *numeriseur; TypeBNModele *modele_bn; int k;
	
	if((numeriseur = NumeriseurOrigineDuPave(menu))) {
		printf("- Ecriture des ressources %s a leur valeur par defaut:\n",numeriseur->nom);
		modele_bn = &(ModeleBN[numeriseur->def.type]);
		NumeriseurAutoriseAcces(numeriseur,1,1);
		for(k=0; k<modele_bn->nbress; k++) if(modele_bn->ress[k].init) {
			numeriseur->ress[k].hval = modele_bn->ress[k].defaut;
			NumeriseurRessHexaChange(numeriseur,k);
			NumeriseurRessourceCharge(numeriseur,k,"*");
		}
		NumeriseurAutoriseAcces(numeriseur,0,1);
		NumeriseurMontreModifs(numeriseur);
	}
	
	return(0);
}
/* ========================================================================== */
static int NumeriseurRAZ(Menu menu, MenuItem item) {
	TypeNumeriseur *numeriseur; TypeBNModele *modele_bn;
	int k;

	if((numeriseur = NumeriseurOrigineDuPave(menu))) {
		printf("- RAZ des DACs %s:\n",numeriseur->nom);
		modele_bn = &(ModeleBN[numeriseur->def.type]);
		NumeriseurAutoriseAcces(numeriseur,1,1);
		for(k=0; k<modele_bn->nbress; k++) 
			if((modele_bn->ress[k].categ == RESS_MODUL) 
			|| (modele_bn->ress[k].categ == RESS_COMP)
			|| (modele_bn->ress[k].categ == RESS_CORR)
			|| (modele_bn->ress[k].categ == RESS_AUXI)) {
			if(modele_bn->ress[k].type == RESS_FLOAT) numeriseur->ress[k].val.r = 0.0;
			else numeriseur->ress[k].val.i = 0;
			NumeriseurRessValChangee(numeriseur,k);
			NumeriseurRessourceCharge(numeriseur,k,"*");
		}
		NumeriseurAutoriseAcces(numeriseur,0,1);
		NumeriseurMontreModifs(numeriseur);
	}
	return(0);
}
/* ========================================================================== */
static int NumeriseurInit(Menu menu, MenuItem item) {
	Panel p; int rc;
	TypeNumeriseur *numeriseur; TypeBNModele *modele_bn; int k;
	
	if((numeriseur = NumeriseurOrigineDuPave(menu))) {
		modele_bn = &(ModeleBN[numeriseur->def.type]);
		p = PanelCreate(modele_bn->nbress);
		PanelColumns(p,((modele_bn->nbress - 1) / 40) + 1);
		for(k=0; k<modele_bn->nbress; k++) PanelBool(p,modele_bn->ress[k].nom,&(numeriseur->ress[k].init));
		rc = OpiumExec(p->cdr);
		PanelDelete(p);
		if(rc == PNL_CANCEL) return(0);
		printf("- Initialisation des ressources du numeriseur %s:\n",numeriseur->nom);
		// a faire avec Recharge: NumeriseursRelit(f,numeriseur->bb);
		NumeriseurAutoriseAcces(numeriseur,1,1);
		for(k=0; k<modele_bn->nbress; k++) if(numeriseur->ress[k].init) NumeriseurRessourceCharge(numeriseur,k,"*");
		NumeriseurAutoriseAcces(numeriseur,0,1);
		NumeriseurMontreModifs(numeriseur);
	}

	return(0);
}
/* ========================================================================== */
static int NumeriseurCorrectifs(Menu menu, MenuItem item) {
	Panel p;
	TypeNumeriseur *numeriseur; TypeBNModele *modele_bn; int k;
	
	if((numeriseur = NumeriseurOrigineDuPave(menu))) {
		modele_bn = &(ModeleBN[numeriseur->def.type]);
		p = PanelCreate(modele_bn->nbress);
		PanelColumns(p,((modele_bn->nbress - 1) / 40) + 1);
		for(k=0; k<modele_bn->nbress; k++) PanelFloat(p,modele_bn->ress[k].nom,&(numeriseur->ress[k].correctif));
		OpiumExec(p->cdr);
	}

	return(0);
}
/* ========================================================================== */
static char NumeriseurCree(TypeNumeriseur *numeriseur) {
	int bb,rc; Panel p; char ajout;
	TypeBNModele *modele_bn; int k;
	
	modele_bn = &(ModeleBN[numeriseur->def.type]);
	p = PanelCreate(modele_bn->nbress);
	PanelColumns(p,((modele_bn->nbress - 1) / 40) + 1);
	for(k=0; k<modele_bn->nbress; k++) PanelBool(p,modele_bn->ress[k].nom,&(numeriseur->ress[k].init));
	rc = OpiumExec(p->cdr);
	PanelDelete(p);
	
	if(rc == PNL_CANCEL) return(0);
	ajout = 0;
	if((bb = numeriseur->bb) < 0) {
		for(bb=0; bb<NumeriseurNb; bb++) 
			if((Numeriseur[bb].def.type == numeriseur->def.type)
			&& (Numeriseur[bb].def.serial == numeriseur->def.serial)) break;
		if(bb < NUMER_MAX) {
			if(bb == NumeriseurNb) {
				NumeriseurNb++;
				ajout = 1;
				numeriseur->fischer = 0;
			#if (CABLAGE_CONNECT_MAX <= 256)
				OpiumReadByte("Connecteur"CABLAGE_EXPLIC,(char *)&(numeriseur->fischer));
			#else
				OpiumReadShort("Connecteur"CABLAGE_EXPLIC,(short *)&(numeriseur->fischer));
			#endif
				// printf("  . Numeriseur branche sur le connecteur #%d\n",numeriseur->fischer);
			}
			numeriseur->bb = bb;
			if(numeriseur->def.serial == 0xFF) 
				snprintf(numeriseur->fichier,MAXFILENAME,"%s",modele_bn->nom);
			else snprintf(numeriseur->fichier,MAXFILENAME,"%s_%02d",modele_bn->nom,numeriseur->def.serial);
			memcpy(&(Numeriseur[bb]),numeriseur,sizeof(TypeNumeriseur));
			printf("* Sauvegarde du %s numeriseur %s (#%d) dans le fichier %s\n",
				   ajout? "": "nouveau",Numeriseur[bb].nom,bb,Numeriseur[bb].fichier);
		} else OpiumFail(L_("Trop de numeriseurs definis! (maximum: %d)"),NUMER_MAX);
	}
	if((bb >= 0) && (bb < NumeriseurNb)) {
		if(ajout) NumeriseursEcrit(NumeriseurFichierLu,NUMER_CONNECTION,NUMER_TOUS);
		NumeriseursEcrit(NumeriseurFichierLu,NUMER_RESSOURCES,bb);
	}

	return(1);
}
/* ========================================================================== */
static int NumeriseurSauve(Menu menu, MenuItem item) {
	TypeNumeriseur *numeriseur;
	if((numeriseur = NumeriseurOrigineDuPave(menu))) NumeriseurCree(numeriseur);
	return(0);
}
/* ========================================================================== */
static int NumeriseurEnreg(Menu menu, MenuItem item) {
	Panel p; int rc;
	TypeNumeriseur *numeriseur; TypeBNModele *modele_bn; int k,n;
	char enregistre[NUMER_RESS_MAX];
	
	if((numeriseur = NumeriseurOrigineDuBouton(menu))) {
		modele_bn = &(ModeleBN[numeriseur->def.type]);
		p = PanelCreate(modele_bn->nbress);
		PanelColumns(p,((modele_bn->nbress - 1) / 40) + 1);
		for(k=0; k<modele_bn->nbress; k++) {
			PanelBool(p,modele_bn->ress[k].nom,&(enregistre[k]));
			enregistre[k] = 0;
		}
		/* pb avec les ressources composees (meme place dans le status) */
		for(n=0; n<modele_bn->enreg_dim; n++) enregistre[modele_bn->numress[modele_bn->enreg_sts[n]]] = 1;
		rc = OpiumExec(p->cdr);
		PanelDelete(p);
		if(rc == PNL_CANCEL) return(0);
		EnConstruction();
		/* pourquoi ce report dans la definition du modele ????
		n = 0;
		for(k=0; k<modele_bn->nbress; k++) if(enregistre[k]) modele_bn->enreg_sts[n++] = modele_bn->ress[k].status;
		modele_bn->enreg_dim = n;
		if(OpiumCheck("Sauver cette definition")) {
			if(NumeriseurFichierModeles[0]) RepertoireIdentique(NumeriseurFichierLu,NumeriseurFichierModeles,nom_complet);
			else strcpy(nom_complet,FichierModlNumeriseurs);
			ArgPrint(nom_complet,NumerModlDesc);
		} */
	}
	
	return(0);
}
/* ========================================================================== */
static TypeMenuItem iNumeriseurBoutons[] = {
	{ "Relire",     MNU_FONCTION NumeriseurRecharge },
	{ "Defaut",     MNU_FONCTION NumeriseurDefaut },
	{ "RAZ",        MNU_FONCTION NumeriseurRAZ },
	{ "Init",       MNU_FONCTION NumeriseurInit },
	{ "Correctifs", MNU_FONCTION NumeriseurCorrectifs },
	{ "Sauver",     MNU_FONCTION NumeriseurSauve },
	MNU_END
};
static TypeMenuItem iNumeriseurBlocage[] = {
	{ "Debloquer",  MNU_FONCTION NumeriseurDebloque },
	{ "Bloquer",    MNU_FONCTION NumeriseurBloque },
	{ "Envoi DACs", MNU_FONCTION NumeriseurEnvoiDACs },
	MNU_END
};
#define MAXPLANCHECOLS 12
/* ========================================================================== */
int NumeriseurAfficheChoix(TypeNumeriseur *numeriseur) {
	int bb,k; int i,j,l,m,n; NUMER_TRACE_TYPE t; 
	int x,y;
	int xl[MAXPLANCHECOLS],xc[MAXPLANCHECOLS],yc[MAXPLANCHECOLS],xm,xp,xs,col;
	char optn_brutes;
//	char avec_status;
	TypeNumeriseurCmde *interface; TypeBNModele *modele_bn; TypeBNModlRessource *ress;
	Menu menu; Panel panel; Instrum instrum; Cadre cdr; char titre[80];
	char *nom,*unite;
	byte mode;

	unite = 0;
	//printf("(%s) Affiche numeriseur @%08X\n",__func__,numeriseur);
	if(!numeriseur) {
		numeriseur = &(NumeriseurAnonyme[0]);
		if(numeriseur->def.type == 0) {
			OpiumError("On ne sait pas quel type de numeriseur afficher");
			return(0);
		}
	}
	NumeriseursDiffusesNb = 0;
	for(bb=0; bb<NumeriseurNb; bb++) {
		NumeriseurDiffuse[bb] = (bb == numeriseur->bb);
//		PanelItemSelect(pNumeriseursDiffuses,bb+1,(bb != numeriseur->bb));
		if(NumeriseurDiffuse[bb]) NumeriseursDiffusesNb++;
	}
	NumeriseurChoix.adrs = numeriseur;
	if(!(numeriseur->repart)) {
		OpiumError("On ne sait pas par quel repartiteur acceder a %s",numeriseur->nom);
		return(0);
	}
	modele_bn = &(ModeleBN[numeriseur->def.type]);
	if(modele_bn->nbress == 0) {
		OpiumError("Un numeriseur de type %s n'a pas de reglage",modele_bn->nom);
		return(0);
	}
	printf("%s/ ----- Utilisation du numeriseur %s (adresse 0x%02X) -----\n",DateHeure(),numeriseur->nom,numeriseur->def.adrs);
//	avec_status = 0;
//	for(j=0; j<NUMER_STATUS_DIM; j++) if(modele_bn->numress[j] >= 0) { avec_status = 1; break; }
	if(!(numeriseur->avec_status)) strcpy(numeriseur->status.lu,L_("sans objet"));
	optn_brutes = (NumeriseurChoix.optn.brutes /* && (modele_bn->acces.mode != NUMER_ACCES_TXT) */);
	for(t=0; t<NUMER_TRACE_NB; t++) NumeriseurTracer[t] = 0;
	RepartConnecte = numeriseur->repart;
	if(RepartConnecte->famille == FAMILLE_IPE) IpeAutoriseFibre(RepartConnecte,numeriseur->entree,1);
	NumeriseurPeutStopper = 0;
	interface = &(numeriseur->interface);

	if(interface->planche) {
		int j,k,m,n;

		if(!memcmp(&(interface->optn),&(NumeriseurChoix.optn),sizeof(TypeNumeriseurPlancheOptn))) goto construite;
		j = interface->prems; m = j;
		n = interface->nb;
		k = j + n;
		NumeriseurInstrNb -= n;
		interface->nb = 0;
		while(j < NumeriseurInstrNb) {
			memcpy(&(NumeriseurInstr[j]),&(NumeriseurInstr[k]),sizeof(TypeNumeriseurInstrum));
			k++; j++;
		}
		for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].interface.prems > m) Numeriseur[bb].interface.prems -= n;
		// printf("(%s-2.2) mNumeriseurDiffusion: @%08X < %08X)\n",__func__,(hexa)mNumeriseurDiffusion,(hexa)(mNumeriseurDiffusion->cdr));
		BoardTrash(interface->planche); /* detruit aussi les ->cdr => tout est a re-creer!! */
		// printf("(%s-2.1) mNumeriseurDiffusion: @%08X < %08X)\n",__func__,(hexa)mNumeriseurDiffusion,(hexa)(mNumeriseurDiffusion->cdr));
	} else interface->planche = BoardCreate();
	sprintf(titre,L_("Numeriseur %s"),numeriseur->nom);
	OpiumTitle(interface->planche,titre);
	
	x = 0; y = Dy;

	PanelMode(panel = PanelCreate(4),PNL_RDONLY|PNL_DIRECT); PanelSupport(panel,WND_CREUX);
	PanelColumns(panel,2);
	PanelItemSelect(panel,PanelText (panel,"type",modele_bn->nom,8),0);
	PanelItemSelect(panel,PanelSHex (panel,"ident",(short *)&(numeriseur->ident)),0);
	if(RepartConnecte->rep >= 0) PanelItemSelect(panel,PanelList(panel,L_("repartiteur"),RepartListe,&(RepartConnecte->rep),REPART_NOM),0);
	else PanelItemSelect(panel,PanelText(panel,L_("repartiteur"),"(tempo)",REPART_NOM),0);
	PanelItemSelect(panel,PanelText (panel,L_("heure status"),numeriseur->status.lu,10),0);
	BoardAddPanel(interface->planche,panel,x,y,0); cdr = panel->cdr; x += (31 + REPART_NOM) * Dx;

	// BoardAddMenu(interface->planche,MenuBouton(L_("Autre numeriseur"),MNU_FONCTION NumeriseurChange),0,y + (3 * Dy),0);
	if(NumeriseurNb > 1) BoardAddBouton(interface->planche,L_("Autre numeriseur"),NumeriseurChange,0,y + (3 * Dy),0);
	menu = mNumeriseurDiffusion = MenuBouton(L_("Diffusion"),MNU_FONCTION NumeriseurDiffusion);
	BoardAddMenu(interface->planche,menu,20 * Dx,y + (3 * Dy),0);

	if(numeriseur->avec_status) {
		menu = MenuBouton(L_("Enregistrement"),MNU_FONCTION NumeriseurEnreg); OpiumSupport(menu->cdr,WND_PLAQUETTE);
		interface->bouton = menu;
		BoardAddMenu(interface->planche,menu,31 * Dx,y + (3 * Dy),0);

		instrum = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&(numeriseur->status.a_copier) /*a &(numeriseur->status.demande) */,"ignore/relu");
		InstrumOnChange(instrum,NumeriseurAcquiert,(void *)numeriseur);
		OpiumSupport(instrum->cdr,WND_RAINURES); InstrumTitle(instrum,"Status");
		// BoardAddInstrum(interface->planche,instrum,OPIUM_A_DROITE_DE cdr); cdr = instrum->cdr;
		BoardAddInstrum(interface->planche,instrum,x,y,0); cdr = instrum->cdr; x += 12 *  Dx;

		for(t=0; t<NUMER_TRACE_NB; t++) {
			char titre[80];
			numeriseur->trace[t].numeriseur = (void *)numeriseur;
			numeriseur->trace[t].type = t;
			instrum = InstrumKeyB(INS_GLISSIERE,(void *)&Gliss2lignes,&(NumeriseurTracer[t]),L_("masquer/montrer"));
			InstrumOnChange(instrum,NumeriseurTracage,(void *)(&(numeriseur->trace[t])));
			sprintf(titre,L_("Traces %ss"),NumeriseurNomTrace[t]);
			OpiumSupport(instrum->cdr,WND_RAINURES); InstrumTitle(instrum,titre);
			BoardAddInstrum(interface->planche,instrum,x,y,0); cdr = instrum->cdr; x += 12 *  Dx;
		}
	}

	/* implementation sous conditions (exemple: CALI, USB) */
	menu = mNumeriseurBoutons = MenuLocalise(iNumeriseurBoutons); MenuColumns(menu,3); OpiumSupport(menu->cdr,WND_PLAQUETTE);
	interface->pave = menu;
	//- BoardAddMenu(interface->planche,menu,OPIUM_A_DROITE_DE cdr); cdr = menu->cdr;
	BoardAddMenu(interface->planche,menu,x,y,0); cdr = menu->cdr; OpiumSizeMenu(cdr,0);
	BoardAddBouton(interface->planche,L_("Banc de tests"),BancAffichePlanche,x + (10 * Dx),y + (3 * Dy),0);
	x += cdr->larg + Dx;

	if((numeriseur->nbprocs == 0) && (numeriseur->bb == -1)) {
		char nom_complet[MAXFILENAME]; char *liste[NUMER_PROC_MAX];
		RepertoireIdentique(NumeriseurFichierLu,NumeriseurProcedureDir,nom_complet);
		RepertoireListeCree(0,nom_complet,liste,NUMER_PROC_MAX,&(numeriseur->nbprocs));
		for(l=0; l<numeriseur->nbprocs; l++) {
			strncpy(numeriseur->proc[l].nom,liste[l],NUMER_PROC_NOM);
			strcat(strcat(strcpy(numeriseur->proc[l].fichier,NumeriseurProcedureDir),"/"),liste[l]);
			numeriseur->nomproc[l] = numeriseur->proc[l].nom;
			free(liste[l]);
		}
		if(numeriseur->nbprocs) free(liste[numeriseur->nbprocs]);
	}
	if(numeriseur->nbprocs) {
		numeriseur->numproc = 0;
		panel = PanelCreate(1); PanelSupport(panel,WND_CREUX);
		i = PanelList(panel,"procedure",numeriseur->nomproc,&(numeriseur->numproc),NUMER_PROC_NOM);
		PanelItemModif(panel,i,NumeriseurProcEnCours,(void *)numeriseur);
		PanelOnReset(panel,NumeriseurProcAnnulee,(void *)numeriseur);
		PanelOnApply(panel,NumeriseurProcAppelee,(void *)numeriseur);
		BoardAddPanel(interface->planche,panel,OPIUM_A_DROITE_DE cdr); cdr = panel->cdr; OpiumSizePanel(cdr,0); x += cdr->larg + Dx;
	} else {
		if(ModeleBN[numeriseur->def.type].ident == 2) {
			menu = MenuLocalise(iNumeriseurBlocage); OpiumSupport(menu->cdr,WND_PLAQUETTE); interface->blocage = menu;
			BoardAddMenu(interface->planche,menu,OPIUM_A_DROITE_DE cdr); cdr = menu->cdr; OpiumSizeMenu(cdr,0); x += cdr->larg + Dx;
		}
	}

	for(col=0; col<MAXPLANCHECOLS; col++) { xl[col] = 0; yc[col] = 6 * Dy; }
	for(i=0; i<modele_bn->nbress; i++) if(modele_bn->ress[i].categ != RESS_NUL) {
		col = modele_bn->ress[i].col;
		if(col < 0) col = 0; else if(col >= MAXPLANCHECOLS) col = MAXPLANCHECOLS - 1;
		l = strlen(modele_bn->ress[i].nom);
		if(optn_brutes) { m = strlen(modele_bn->ress[i].unite); if(l < m) l = m; }
		xm = l + 15;
		if(xm > xl[col]) xl[col] = xm;
	}
	xc[0] = 1; for(col=1; col<MAXPLANCHECOLS; col++) xc[col] = xc[col - 1] + xl[col - 1];

	j = NumeriseurInstrNb;
	interface->prems = j;
	for(k=0; k<modele_bn->nbress; k++) {
		numeriseur->ress[k].a_charger = 0;
		ress = &(modele_bn->ress[k]);
		if(ress->categ == RESS_NUL) continue;
		NumeriseurRessHexaChange(numeriseur,k);
		col = ress->col;
		if(col < 0) col = 0; else if(col >= MAXPLANCHECOLS) col = MAXPLANCHECOLS - 1;
		x = xc[col]; y = yc[col];

		if(j >= NUMER_INSTRM_MAX) break;
		mode = (ress->ssadrs == 0xFF)? PNL_RDONLY|PNL_DIRECT: PNL_DIRECT;
		PanelMode(panel = PanelCreate(2),mode); PanelSupport(panel,WND_CREUX);
		if(!NumeriseurChoix.optn.codees) nom = ress->nom;
		else if(!optn_brutes) unite = ress->nom;
		else {
			if(ress->unite[0]) { nom = ress->nom; unite = ress->unite; }
			else { nom = 0; unite =  ress->nom; }
		}
		l = 0;
		n = 0;
		/* valeur envoyee telle quelle */ 
		if(optn_brutes) {
			if(nom) { m = strlen(nom); if(m > l) l = m; }
			if(NumeriseurChoix.optn.hexa) {
				i = PanelSHex(panel,nom,(short *)&(numeriseur->ress[k].hval));
				PanelItemLngr(panel,i,5);
			} else {
				if(ress->type == RESS_FLOAT) i = PanelAdu(panel,nom,(shex *)&(numeriseur->ress[k].hval));
				else i = PanelShort(panel,nom,(short *)&(numeriseur->ress[k].hval));
				PanelItemLngr(panel,i,NUMER_DIGITS_DAC);
			}
			if(ress->ssadrs == 0xFF) PanelItemSelect(panel,i,0);
			else {
				PanelItemModif(panel,i,NumeriseurRessEnCours,(void *)(long)j);
				PanelItemExit(panel,i,NumeriseurRessFromADU,(void *)(long)j);
			}
			n++;
		}
		/* valeur au format utilisateur */
		if(NumeriseurChoix.optn.codees) {
			if(unite) { m = strlen(unite); if(m > l) l = m; }
			if(ress->type == RESS_CLES) {
				i = PanelKey(panel,unite,ress->lim_k.cles,&(numeriseur->ress[k].val.i),NUMER_RESS_VALTXT);
				if(ress->ssadrs == 0xFF) PanelItemSelect(panel,i,0);
				else {
					PanelItemModif(panel,i,NumeriseurRessEnCours,(void *)(long)j);
					PanelItemExit(panel,i,NumeriseurRessFromKey,(void *)(long)j);
				}
			} else {
				i = PanelText(panel,unite,numeriseur->ress[k].cval,NUMER_RESS_VALTXT);
				if(ress->ssadrs == 0xFF) PanelItemSelect(panel,i,0);
				else {
					PanelItemModif(panel,i,NumeriseurRessEnCours,(void *)(long)j);
					PanelItemExit(panel,i,NumeriseurRessFromUser,(void *)(long)j);
				}
			}
			PanelItemLngr(panel,i,NUMER_DIGITS_DAC);
			n++;
		}
		/* place dans la planche */
		xp = xl[col] - 15 - l; if(xp < 0) xp = 0;
		BoardAddPanel(interface->planche,panel,(x+xp)*Dx,y,0);
		NumeriseurInstr[j].type = OPIUM_PANEL; NumeriseurInstr[j].instrum = (void *)panel; 
		NumeriseurInstr[j].numeriseur = numeriseur; NumeriseurInstr[j].num = k; NumeriseurInstr[j].reglage = 0;
		j++;
		xs = xp + l + NUMER_DIGITS_DAC;

		/* boutons +/- */
		if(ress->ssadrs != 0xFF) {
			if(j >= NUMER_INSTRM_MAX) break;
			menu = MenuLocalise(iNumeriseurRessUpDown); OpiumSupport(menu->cdr,WND_PLAQUETTE);
			if(!optn_brutes || !NumeriseurChoix.optn.codees) MenuColumns(menu,2);
			BoardAddMenu(interface->planche,menu,(x+xs+3)*Dx,y,0);
			NumeriseurInstr[j].type = OPIUM_MENU; NumeriseurInstr[j].instrum = (void *)menu; 
			NumeriseurInstr[j].numeriseur = numeriseur; NumeriseurInstr[j].num = k; NumeriseurInstr[j].reglage = 0;
			j++;			
		}

		yc[col] += ((n + 1) * Dy); /* une ou deux lignes dans le panel + 1 pour separer */
	}
	NumeriseurInstrNb = j;
	interface->nb = NumeriseurInstrNb - interface->prems;
	
construite:
	OpiumExitFctn(interface->planche,NumeriseurFerme,(void *)numeriseur);
	interface->a_rafraichir = 0;
	interface->verrouillee = 0;
	OpiumUse(interface->planche,OPIUM_FORK);
	OpiumUserAction();
	return(0);
}
/* ========================================================================== */
int NumeriseurActionSouris(TypeNumeriseur *numeriseur, WndAction *e) {
	char donnees; NUMER_MODE mode;
	TypeRepartiteur *repart; 

	if(!numeriseur) return(0);
	NumeriseurChoix.bb = numeriseur->bb;
	if(e->code == WND_MSERIGHT) {
		if((repart = numeriseur->repart)) {
			donnees = 1;
			if(OpiumReadKeyB("Donnees envoyees","identification/detecteur",&donnees,16) == PNL_CANCEL) return(0);
			mode = donnees? NUMER_MODE_ACQUIS: NUMER_MODE_IDENT;
			RepartiteurDemarreNumeriseurs(repart,mode,numeriseur->def.adrs,1);
		}
	} else NumeriseurAfficheChoix(numeriseur);
	
	return(0);
}
/* ========================================================================== */
int NumeriseurLectClique(Figure fig, WndAction *e) {
	NumeriseurActionSouris((TypeNumeriseur *)(fig->adrs),e);
	return(0);
}
/* ========================================================================== */
int NumeriseurOperationSouris(TypeNumeriseur *numeriseur, WndAction *e) {
	int bb,autre_bb,autre_entree; TypeCableConnecteur nouveau_connecteur;
	char oper; char deja_connu,ok,chgt_fibre;
	TypeRepartiteur *repart,*nouveau; TypeNumeriseur *inattendu;

	if(!numeriseur) return(0);
	chgt_fibre = 0;
	NumeriseurChoix.bb = numeriseur->bb;
	if(e->code == WND_MSERIGHT) {
		deja_connu = 0;
		bb = NumeriseurChoix.bb;
		if(numeriseur->fischer) {
			oper = 0;
			if(OpiumReadListB("Operation effectuee",NumeriseurOper+1,&oper,8) == PNL_CANCEL) return(0);
			oper += 1;
		} else oper = NUMER_ENTREE;
		switch(oper) {
			case NUMER_ENTREE:
				nouveau_connecteur = 0;
			#if (CABLAGE_CONNECT_MAX <= 256)
				if(OpiumReadByte("Branchement sur le connecteur"CABLAGE_EXPLIC,(char *)&nouveau_connecteur) == PNL_CANCEL) return(0);
			#else
				if(OpiumReadShort("Branchement sur le connecteur"CABLAGE_EXPLIC,(short *)&nouveau_connecteur) == PNL_CANCEL) return(0);
			#endif
				if(nouveau_connecteur == 0) return(0);
				for(autre_bb=0; autre_bb<NumeriseurNb; autre_bb++) if(Numeriseur[autre_bb].fischer == nouveau_connecteur) break;
				if(autre_bb >= NumeriseurNb) {
					numeriseur->fischer = nouveau_connecteur;
					printf(". Le numeriseur %s est maintenant branche sur le connecteur %s\n",numeriseur->nom,CablageEncodeConnecteur(numeriseur->fischer));
					NumeriseurReconnecte(numeriseur);
					if(OpiumCheck("Connecter aussi les fibres?")) {
						ok = 0;
						forever {
							if(numeriseur->repart) {
								RepartDemande = ((TypeRepartiteur *)(numeriseur->repart))->rep;
								RepartEntreeDemandee = numeriseur->entree;
							} else { RepartDemande = 0; RepartEntreeDemandee = 0; }
							if(OpiumExec(pRepartChoix->cdr) == PNL_CANCEL) break;
							repart = &(Repart[RepartDemande]);
							if((RepartEntreeDemandee < 0) || (RepartEntreeDemandee >= repart->maxentrees))
								OpiumError("Numero d'entree illegal, %d maxi autorise",repart->maxentrees);
							else { ok = 1; break; }
						}
						if(!ok) break;
						if((inattendu = (TypeNumeriseur *)repart->entree[RepartEntreeDemandee].adrs)) {
							if(!OpiumChoix(2,SambaNonOui,"OUPS: les fibres sont deja la %s ! La retirer?",inattendu->nom)) break;
							inattendu->repart = 0;
						}
						numeriseur->repart = repart;
						numeriseur->entree = RepartEntreeDemandee;
						repart->entree[RepartEntreeDemandee].adrs = (TypeAdrs)numeriseur;
						chgt_fibre = 1;
					}
					break;
				} else {
					if(!OpiumChoix(2,SambaNonOui,"OUPS: il y a deja la %s a cet endroit! La retirer?",Numeriseur[autre_bb].nom)) return(0);
					deja_connu = 1;
				}
			case NUMER_ECHANGE:
				if(!deja_connu) {
					autre_bb = bb;
					if(OpiumReadList("Avec le numeriseur",NumeriseurListe,&autre_bb,NUMER_NOM) == PNL_CANCEL) return(0);
					if(autre_bb == bb) return(0);
				}
				NumeriseurDeconnecte(numeriseur);
				NumeriseurDeconnecte(&(Numeriseur[autre_bb]));
				nouveau_connecteur = Numeriseur[autre_bb].fischer;
				Numeriseur[autre_bb].fischer = numeriseur->fischer;
				numeriseur->fischer = nouveau_connecteur;
				if(numeriseur->fischer) printf(". Le numeriseur %s passe sur le connecteur %s\n",numeriseur->nom,CablageEncodeConnecteur(numeriseur->fischer));
				else printf(". Le numeriseur %s est maintenant debranche\n",numeriseur->nom);
				if(Numeriseur[autre_bb].fischer) printf(". Le numeriseur %s vient sur le connecteur %s\n",Numeriseur[autre_bb].nom,CablageEncodeConnecteur(Numeriseur[autre_bb].fischer));
				else printf(". Le numeriseur %s est maintenant debranche\n",Numeriseur[autre_bb].nom);
				NumeriseurReconnecte(numeriseur);
				NumeriseurReconnecte(&(Numeriseur[autre_bb]));
				if(OpiumCheck("Echanger aussi les fibres")) {
					nouveau = Numeriseur[autre_bb].repart;
					autre_entree = Numeriseur[autre_bb].entree;
					Numeriseur[autre_bb].repart = numeriseur->repart;
					Numeriseur[autre_bb].entree = numeriseur->entree;
					if((repart = Numeriseur[autre_bb].repart)) repart->entree[Numeriseur[autre_bb].entree].adrs = (TypeAdrs)&(Numeriseur[autre_bb]);
					numeriseur->repart = nouveau;
					numeriseur->entree = autre_entree;
					if((repart = numeriseur->repart)) repart->entree[numeriseur->entree].adrs = (TypeAdrs)numeriseur;
					chgt_fibre = 1;
				}
			break;
			case NUMER_SORTIE:
				printf(". Le numeriseur %s n'est PLUS sur le connecteur %s\n",numeriseur->nom,CablageEncodeConnecteur(numeriseur->fischer));
				numeriseur->fischer = 0;
				NumeriseurDeconnecte(numeriseur);
				if(OpiumCheck("Retirer aussi les fibres")) {
					if((repart = numeriseur->repart)) repart->entree[numeriseur->entree].adrs = 0;
					numeriseur->repart = 0;
					chgt_fibre = 1;
				}
				break;
		}
		NumeriseursEcrit(FichierPrefNumeriseurs,NUMER_CONNECTION,NUMER_TOUS);
		LectIdentFaite = 0;
		if(chgt_fibre) { RepartCompteDonnees(repart); RepartiteursASauver = 1; }
		OrgaTraceTout(0,0);
	} else NumeriseurAfficheChoix(numeriseur);
	
	return(0);
}
/* ========================================================================== */
int NumeriseurOrgaClique(Figure fig, WndAction *e) {
	NumeriseurOperationSouris((TypeNumeriseur *)(fig->adrs),e);
	return(0);
}
/* ========================================================================== */
int NumeriseurExec() {
	if(NumeriseurChoix.bb >= NumeriseurNb) NumeriseurChoix.bb = 0;
	if(NumeriseurNb > 1) {
		if(OpiumExec(pNumeriseurChoix->cdr) == PNL_CANCEL) return(0);
	} else NumeriseurChoix.bb = 0;
	NumeriseurAfficheChoix(&(Numeriseur[NumeriseurChoix.bb]));
	return(0);
}
