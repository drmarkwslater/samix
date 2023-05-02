#ifdef macintosh
#pragma message(__FILE__)
#endif
#define BANC_C

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/time.h>

#include <environnement.h>

#include <claps.h>
#include <dateheure.h>
#include <timeruser.h>
#include <opium_demande.h>
#include <opium.h>
#include <impression.h>

#include <samba.h>
#include <repartiteurs.h>
#include <numeriseurs.h>
#include <cablage.h>
#include <detecteurs.h>

#include <banc.h>

#define IP_LNGR 16
#define BANC_MAXREPNOM 64
#define BANC_MAXVARNOM 32
#define BANC_MAXSCPTNOM 32
#define BANC_MAXETATS 1024
#define BANC_MAXPOINTS 256
#define BANC_SPTR_LARG 90
#define BANC_SPTR_HAUT 20

#define BANC_ETAT_TXT 9
#define BANC_RESUL_TXT 8

static char BancAlimIpAdrs[IP_LNGR];
static int  BancAlimIpPort;

#ifdef AVEC_GENE
static char stHostnameGene33220A[IP_LNGR];
static int portnoGene33220A;
#endif

static float BancAlimNominale,BancAlimLimite,BancAlimCourant;
static int BancAlimVoie;
static int BancMesureDuree;
static float BancMesureDelai;
static int BancMesurePoints;
static float BancMesureTolerance;
static char BancScptRepert[MAXFILENAME];
static char BancScptAlim[NUMER_PROC_NOM],BancScptRef[NUMER_PROC_NOM];
static int BancSptrPoints,BancSptrMax;
static int BancSptrNbSauves;
static GraphAxeParm BancGraphiquesParms[2];

static ArgDesc BancAlimParmsDesc[] = {
	{ "alim.adrs",       DESC_STR(IP_LNGR)        BancAlimIpAdrs,	    "Adresse IP de l'alim" },
	{ "alim.port",       DESC_INT                &BancAlimIpPort,	    "Port IP pour l'alim" },
	{ "alim.voie",       DESC_INT                &BancAlimVoie,	        "Voie utilisee" },
	{ "alim.tension",    DESC_FLOAT              &BancAlimNominale,	    "Tension nominale (V)" },
	{ "alim.courant",    DESC_FLOAT              &BancAlimLimite,	    "Courant limite (A)" },
	{ "mesure.duree",    DESC_INT                &BancMesureDuree,	    "Temps de mesure du courant (s)" },
	{ "mesure.delai",    DESC_FLOAT              &BancMesureDelai,	    "Intervalle de temps entre 2 mesures (s)" },
#ifdef BANC_MAXPOINTS
	{ "mesure.points",   DESC_NONE },
#else
	{ "mesure.points",   DESC_INT                &BancMesurePoints,	    "Nb points de mesure du courant" },
#endif
	{ "script.repert",   DESC_STR(BANC_MAXREPNOM) BancScptRepert,       "Repertoire des scripts de test" },
	{ "script.alim",     DESC_STR(NUMER_PROC_NOM) BancScptAlim,         "Script de mise sous tension" },
	{ "script.ref",      DESC_STR(NUMER_PROC_NOM) BancScptRef,          "Script d'activation des references" },
#ifdef AVEC_GENE
	{ "gene.adrs",       DESC_STR(IP_LNGR)        stHostnameGene33220A,	"Adresse IP du generateur" },
	{ "gene.port",       DESC_INT                &portnoGene33220A,	    "Port IP pour le generateur" },
#endif
	DESC_END
};
static ArgDesc BancGraphiquesDesc[] = {
	{ "spectres.dim",  DESC_INT                 &BancSptrPoints,                   "Nombre de points de la FFT" },
	{ "spectres.nb",   DESC_INT                 &BancSptrMax,                      "Nombre de spectres a sommer" },
	{ "Axe.X.type",    DESC_KEY "lineaire/log", &BancGraphiquesParms[0].log,       "Type d'axe (pour les X)" },
	{ "Axe.X.limites", DESC_KEY "manu/auto",    &BancGraphiquesParms[0].autom,     "Limitation de l'axe (pour les X)" },
	{ "Axe.X.min",     DESC_FLOAT               &BancGraphiquesParms[0].lim.r.min, "Minimum sur l'axe des X, si limites manuelles" },
	{ "Axe.X.max",     DESC_FLOAT               &BancGraphiquesParms[0].lim.r.max, "Maximum sur l'axe des X, si limites manuelles" },
	{ "Axe.Y.type",    DESC_KEY "lineaire/log", &BancGraphiquesParms[1].log,       "Type d'axe (pour les Y)" },
	{ "Axe.Y.limites", DESC_KEY "manu/auto",    &BancGraphiquesParms[1].autom,     "Limitation de l'axe (pour les Y)" },
	{ "Axe.Y.min",     DESC_FLOAT               &BancGraphiquesParms[1].lim.r.min, "Minimum sur l'axe des Y, si limites manuelles" },
	{ "Axe.Y.max",     DESC_FLOAT               &BancGraphiquesParms[1].lim.r.max, "Maximum sur l'axe des Y, si limites manuelles" },
	DESC_END
};
static ArgDesc *BancAlimParmsPtr,*BancAlimTolerDesc,*BancSptrParmsDesc,*BancGraphiquesPtr;
static ArgDesc BancSetup[] = {
	DESC_INCLUDE(BancAlimParmsPtr),
	DESC_INCLUDE(BancSptrParmsDesc),
	DESC_INCLUDE(BancAlimTolerDesc),
	DESC_INCLUDE(BancGraphiquesPtr),
	DESC_END
};

static int BancAlimSkt,BancGeneSkt;
static TypeNumeriseur *BancNumer;
static Panel pBancConnexions,pBancMessage,pBancAlimDemande,pBancAlimMesure;
static Menu BancBoutonEnchaine,BancBoutonStoppe;
static float BancAlimTension;
static char BancScptRacine[MAXFILENAME];
static char BancResultat;

typedef enum {
	BANC_PARMS = 0,
	BANC_CONSOS,
//	BANC_ADC,
//	BANC_AMPLI,
	BANC_FET,
	BANC_NBPHASES
} BANC_PHASE;
static char *BancFeuillesNom[BANC_NBPHASES+1] = {
	"Parametres",
	"Consommations", 
//	"ADC", 
//	"Ampli et regulation", 
	"Spectres", 
	"\0"
};
static Onglet BancFeuilles;
static int BancPhase;
static TypeFigDroite BancSeparation = { 0, 0, WND_RAINURES,  0x3FFF, 0x3FFF, 0x3FFF };
static TypeFigZone   BancBoutonFet  = { 0, 0, WND_PLAQUETTE, GRF_RGB_YELLOW };
static TypeInstrumColonne BancAlimColonne = {0, 6, 0, 0, 3, 4};
static TypeInstrumColonne BancSptrColonne = {0, 6, 0, 0, 3, 4};

typedef enum {
	BANC_A_FAIRE = 0,
	BANC_EN_COURS,
	BANC_TERMINE,
	BANC_NBETATS
} BANC_ETATS;
static char BancEtatCles[32] = "a faire/en cours/termine";
static OpiumColor BancJauneOrangeVert[] = { { GRF_RGB_YELLOW }, { GRF_RGB_ORANGE }, { GRF_RGB_GREEN } };

typedef enum {
	BANC_INDETERMINE = 0,
	BANC_MAUVAIS,
	BANC_OK,
	BANC_NBRESULTATS
} BANC_RESULTATS;
static char BancResultatCles[32] = "neant/negatif/correct";
static OpiumColor BancNoirRougeVert[] = { { GRF_RGB_BLACK }, { GRF_RGB_RED }, { GRF_RGB_GREEN } };

static float BancTemps[2],BancFrequence[2];

/*
 * ******************************* Consommations *******************************
 */
typedef enum {
	BANC_CONSOS_0V = 0,
	BANC_CONSOS_NOMINAL,
	BANC_CONSOS_FPGA,
	BANC_CONSOS_BB_ALIM,
	BANC_CONSOS_BB_REF,
	BANC_CONSOS_NBETAPES
} BANC_CONSOS_ETAPES;
static char *BancConsosEtape[BANC_CONSOS_NBETAPES+1] = {
	"Alim generale 0V",
	"Alim generale nominale",
	"Chargement FPGA", 
	"Alim numeriseur",
	"Reference numeriseur",
	"\0"
};
static char *BancConsosCode[BANC_CONSOS_NBETAPES+1] = {
	"alim_0V", "alim_nominale", "fpga",  "alim_numeriseur", "references", "\0"
};

typedef struct {
	int etape;
	void *fctn;
	int prgs;
	float duree,delai;
	float Imin,Imax;
	float tension,courant;
	char etat; char resultat;
	char date[12],heure[12];
	struct {
		int nb;
	#ifdef BANC_MAXPOINTS
		float I[BANC_MAXPOINTS];
	#else
		float *I;
	#endif
	} trace;
	Menu bouton; Panel panel; Graph graph; Instrum colonne;
} TypeBancAlimMesure;
static TypeBancAlimMesure BancAlimMesure[BANC_CONSOS_NBETAPES],BancAlimMesureLue;
static int BancAlimNb;

static ArgDesc BancAlimDesc[] = {
	{ 0,             DESC_LISTE BancConsosCode, &BancAlimMesureLue.etape, 0 },
	{ "etat",        DESC_KEY   BancEtatCles,   &BancAlimMesureLue.etat, 0 },
	{ "jour",        DESC_STR(12)                BancAlimMesureLue.date, 0 },
	{ "heure",       DESC_STR(12)                BancAlimMesureLue.heure, 0 },
	{ "duree_s",     DESC_FLOAT                 &BancAlimMesureLue.duree, 0 },
	{ "horloge_s",   DESC_FLOAT                 &BancAlimMesureLue.delai, 0 },
	{ "tension_V",   DESC_FLOAT                 &BancAlimMesureLue.tension, 0 },
	{ "courant_A",   DESC_FLOAT                 &BancAlimMesureLue.courant, 0 },
	{ "Imin_A",      DESC_FLOAT                 &BancAlimMesureLue.Imin, 0 },
	{ "Imax_A",      DESC_FLOAT                 &BancAlimMesureLue.Imax, 0 },
	{ "resultat",    DESC_KEY BancResultatCles, &BancAlimMesureLue.resultat, 0 },
#ifdef BANC_MAXPOINTS
	{ "transitoire", DESC_FLOAT_SIZE(BancAlimMesureLue.trace.nb,BANC_MAXPOINTS) BancAlimMesureLue.trace.I, 0 },
#else
	{ "transitoire", DESC_FLOAT_ADRS(BancAlimMesureLue.trace.nb) BancAlimMesureLue.trace.I, 0 },
#endif
	DESC_END
};

static char BancConsosVariable[2 * BANC_CONSOS_NBETAPES][BANC_MAXVARNOM];

/*
 * ********************************* Spectres **********************************
 */
typedef enum {
	BANC_SPECTRE_1nF = 0,
	BANC_SPECTRE_100nF,
	BANC_SPECTRE_R,
	BANC_SPECTRE_NBETAPES
} BANC_SPECTRE_ETAPES;
static char *BancSptrEtape[BANC_SPECTRE_NBETAPES+1] = {
	"FET et 1nF", "FET et 100nF", "Resistance", "\0"
};
static char *BancSptrCode[BANC_SPECTRE_NBETAPES+1] = {
	"FET1nF", "FET100nF", "Resistance", "\0"
};

typedef struct {
	int etape;
	char script[BANC_MAXSCPTNOM];
	int prgs;
	char etat; char resultat;
	char date[12],heure[12];
	struct {
		int nb; int nbadc;
		float *bruit[NUMER_ADC_MAX];
	} trace;
	OpiumColorState couleur;
	Figure bouton; Panel panel; Graph graph; Instrum colonne;
} TypeBancSptrResultat;
static TypeBancSptrResultat BancSptrResultat[BANC_SPECTRE_NBETAPES],BancSptrResultatLu;
static int BancSptrNb;

#ifdef A_FINIR
static ArgDesc BancSptrDesc[] = {
	{ "ADC",   DESC_FLOAT_SIZE(BancSptrResultatLu.trace.nb,BANC_MAXPOINTS) BancSptrResultatLu.trace.bruit, 0 },
	DESC_END
};
static ArgStruct BancSptrMesureAS = { (void *)BancSptrResultatLu.trace.bruit, (void *)&ConnectReglLue, sizeof(TypeCablageConnectRegl), BancSptrMesureDesc };
#endif
static ArgDesc BancSptrDesc[] = {
	{ 0,           DESC_LISTE BancSptrCode,    &BancSptrResultatLu.etape, 0 },
	{ "etat",      DESC_KEY   BancEtatCles,    &BancSptrResultatLu.etat, 0 },
	{ "jour",      DESC_STR(12)                 BancSptrResultatLu.date, 0 },
	{ "heure",     DESC_STR(12)                 BancSptrResultatLu.heure, 0 },
	{ "resultat",  DESC_KEY BancResultatCles,  &BancSptrResultatLu.resultat, 0 },
#ifdef A_FINIR
	{ "spectres",  DESC_STRUCT_SIZE(BancSptrResultatLu.trace.nbadc,NUMER_ADC_MAX) &BancSptrMesureAS, 0 },
#endif
	DESC_END
};

static char BancSptrVariable[BANC_SPECTRE_NBETAPES][BANC_MAXVARNOM];

/* ************************************************************************** */
static ArgStruct BancEtatAS = { (void *)BancAlimMesure, (void *)&BancAlimMesureLue, sizeof(TypeBancAlimMesure), BancAlimDesc };
static ArgStruct BancSptrAS = { (void *)BancSptrResultat, (void *)&BancSptrResultatLu, sizeof(TypeBancSptrResultat), BancSptrDesc };
static ArgDesc BancEtatDesc[] = {
	{ "Alim",     DESC_STRUCT_SIZE(BancAlimNb,BANC_CONSOS_NBETAPES)  &BancEtatAS, 0 },
	{ "Spectres", DESC_STRUCT_SIZE(BancSptrNb,BANC_SPECTRE_NBETAPES) &BancSptrAS, 0 },
	DESC_END
};

static void BancInitStructures();
static char BancAlimLit(float *val, char *cmde, ...);

/* ========================================================================== */
static int BancConnecte(Menu menu, MenuItem item) {
	char commande[80]; char ok;

	ok = 0;
	sprintf(commande,"ping -c 1 -t 1 -noQq %s >/dev/null",BancAlimIpAdrs);
	if(system(commande) != 0) {
		sprintf(BancMessage,"L'adresse %s (alim) ne repond pas sur Internet",BancAlimIpAdrs);
	} else {
		BancAlimSkt = SocketTCPClientFromName(BancAlimIpAdrs,BancAlimIpPort);
		if(BancAlimSkt > 0) { BancAlimConnectee = 1; ok = 1; }
		else strcpy(BancMessage,"Alimentation generale non connectee");
	}
#ifdef AVEC_GENE
	ok = 0; /* remise en cause tant que tout n'a pas ete connecte */
	sprintf(commande,COMMANDE_PING,stHostnameGene33220A);
	if(system(commande) != 0) {
		sprintf(BancMessage,"L'adresse %s (gene) ne repond pas sur Internet",stHostnameGene33220A);
	} else {
		BancGeneSkt = SocketTCPClientFromName(stHostnameGene33220A,portnoGene33220A);
		if(BancGeneSkt > 0) { BancGeneConnecte = 1; ok = 1; }
		else if(BancAlimSkt <= 0)
			strcpy(BancMessage,"Ni l'alimentation generale ni le generateur de signaux ne peuvent etre connectes");
		else strcpy(BancMessage,"Generateur de signaux non connecte");
		
	}
#endif
	if(OpiumDisplayed(pBancAlimDemande->cdr)) PanelRefreshVars(pBancAlimDemande);
	if(ok) {
		if(BancAlimLit(&BancAlimVmesure,"MEAS:VOLT? (@%d)\n",BancAlimVoie)
		&& BancAlimLit(&BancAlimImesure,"MEAS:CURR? (@%d)\n",BancAlimVoie)) {
			if(OpiumDisplayed(pBancAlimMesure->cdr)) PanelRefreshVars(pBancAlimMesure);
		}
		strcpy(BancMessage,"Connexion reussie, QUEL TALENT!");
	}
	if(OpiumDisplayed(pBancMessage->cdr)) OpiumRefresh(pBancMessage->cdr);
	if(OpiumDisplayed(pBancConnexions->cdr)) OpiumRefresh(pBancConnexions->cdr);
	return(0);
}
/* ========================================================================== */
static char BancAlimEcrit(char *cmde, ...) {
	char texte[256]; va_list argptr; int nb,lngr; int rc;
	
	va_start(argptr, cmde); nb = vsprintf(texte, cmde, argptr); va_end(argptr);
	lngr = strlen(texte);
	if((rc = send(BancAlimSkt,(const char *)texte,lngr,0)) <= 0) {
		sprintf(BancMessage,"Commande alim: %s",strerror(errno));
		return(0);
	}
	return(1);
}
/* ========================================================================== */
static char BancAlimLit(float *val, char *cmde, ...) {
	char texte[256]; va_list argptr; int nb,lngr; int rc;
	
	va_start(argptr, cmde); nb = vsprintf(texte, cmde, argptr); va_end(argptr);
	lngr = strlen(texte);
	if((rc = send(BancAlimSkt,(const char *)texte,lngr,0)) <= 0) {
		sprintf(BancMessage,"Commande alim: %s",strerror(errno)); return(0);
	}
	if((rc = recv(BancAlimSkt,texte,256,0)) <= 0) {
		sprintf(BancMessage,"Commande alim: %s",strerror(errno)); return(0);
	}
	texte[rc] = '\0';
	sscanf(texte,"%g",val);
	return(1);
}
/* ========================================================================== */
static int BancArretUrgence(Menu menu, MenuItem item) {
	BancUrgence = 1; return(0);
}
/* ========================================================================== */
static void BancAlimAllumeBouton(int etape) {
	cas_ou(BancAlimMesure[etape].resultat) {
	  vaut BANC_INDETERMINE:
		MenuItemAllume(BancAlimMesure[etape].bouton,1,0,GRF_RGB_YELLOW); break;
	  vaut BANC_MAUVAIS:
		MenuItemAllume(BancAlimMesure[etape].bouton,1,0,GRF_RGB_RED); break;
	  vaut BANC_OK:
		MenuItemAllume(BancAlimMesure[etape].bouton,1,0,GRF_RGB_GREEN); break;
	}
}
/* ========================================================================== */
static char BancAlimDemande(float tension, float courant) {
	
	if(tension > 18.2) {
		sprintf(BancMessage,"Tension demandee trop grande (%.2fV pour 18.2V max)",tension);
		return(0);
	}
	BancAlimTension = tension; BancAlimCourant = courant;
	if(OpiumDisplayed(pBancAlimDemande->cdr)) OpiumRefresh(pBancAlimDemande->cdr);
	if(BancAlimSkt <= 0) { strcpy(BancMessage,"Alimentation generale non connectee"); return(0); }
	if(!BancAlimEcrit("VOLT %f, (@%d)\n",tension,BancAlimVoie)) return(0);
	if(!BancAlimEcrit("CURR %f, (@%d)\n",courant,BancAlimVoie)) return(0);
	if(!BancAlimEcrit("OUTP ON, (@%d)\n",BancAlimVoie)) return(0);

	return(1);
}
/* ========================================================================== */
static int BancAlimEteint(Menu menu, MenuItem item) {
	if(BancAlimDemande(0.0,0.0)) {
		BancAlimEcrit("OUTP OFF, (@%d)\n",BancAlimVoie);
		sleep(1);
		if(BancAlimLit(&BancAlimVmesure,"MEAS:VOLT? (@%d)\n",BancAlimVoie)
		&& BancAlimLit(&BancAlimImesure,"MEAS:CURR? (@%d)\n",BancAlimVoie)) {
			if(OpiumDisplayed(pBancAlimMesure->cdr)) PanelRefreshVars(pBancAlimMesure);
		}
	}
	return(0);
}
/* ========================================================================== */
static char BancAlimVerifie(int etape) {
	int64 t0,t1,t_lim; char table_definie;
	int nb; Graph g;

	BancAlimMesure[etape].etat = BANC_EN_COURS; BancResultat = BANC_INDETERMINE;
	BancUrgence = 0;
	MenuItemAllume(BancAlimMesure[etape].bouton,1,0,GRF_RGB_ORANGE);
	MenuItemAllume(BancBoutonStoppe,1,0,GRF_RGB_YELLOW);
	g = BancAlimMesure[etape].graph;
	BancAlimMesure[etape].trace.nb = 0;
	if(g) { GraphUse(g,BancAlimMesure[etape].trace.nb); if(OpiumDisplayed(g->cdr)) OpiumRefresh(g->cdr); }
	BancAlimMesure[etape].prgs = 0; InstrumCouleur(BancAlimMesure[etape].colonne,WND_GC_ORANGE);
	if(OpiumDisplayed((BancAlimMesure[etape].colonne)->cdr)) InstrumRefreshVar(BancAlimMesure[etape].colonne);
	t0 = DateMicroSecs(); t1 = t0;
	t_lim = t0 + ((int64)BancMesureDuree * 1000000); nb = 0;
	do {
		SambaAttends((int)(BancMesureDelai*1000.0));
		if(BancAlimSkt <= 0) { strcpy(BancMessage,"Alimentation generale non connectee"); break; }
		if(!BancAlimLit(&BancAlimVmesure,"MEAS:VOLT? (@%d)\n",BancAlimVoie)) break;
		OpiumUserAction();
		if(!BancAlimLit(&BancAlimImesure,"MEAS:CURR? (@%d)\n",BancAlimVoie)) break;
		t1 = DateMicroSecs(); nb++;
		BancAlimMesure[etape].duree = (float)(t1 - t0) / 1000000.0;
		BancAlimMesure[etape].tension = BancAlimVmesure;
		BancAlimMesure[etape].courant = BancAlimImesure;
		if(BancMesureDuree) BancAlimMesure[etape].prgs = (int)((t1 - t0) / BancMesureDuree) / 10000;
		else BancAlimMesure[etape].prgs = 100;
	#ifdef BANC_MAXPOINTS
		table_definie = 1;
	#else
		table_definie = (BancAlimMesure[etape].trace.I);
	#endif
		if(table_definie && (BancAlimMesure[etape].trace.nb < BancMesurePoints)) {
			BancAlimMesure[etape].trace.I[BancAlimMesure[etape].trace.nb] = BancAlimImesure;
			BancAlimMesure[etape].trace.nb += 1;
		}
		if(OpiumDisplayed(pBancAlimMesure->cdr)) PanelRefreshVars(pBancAlimMesure);
		if(OpiumDisplayed((BancAlimMesure[etape].panel)->cdr)) PanelRefreshVars(BancAlimMesure[etape].panel);
		if(BancAlimMesure[etape].prgs < 100) {
			if(OpiumDisplayed((BancAlimMesure[etape].colonne)->cdr)) InstrumRefreshVar(BancAlimMesure[etape].colonne);
		}
		if(g) {
			BancTemps[1] = BancAlimMesure[etape].duree / (float)nb;
			BancAlimMesure[etape].delai = BancTemps[1];
			GraphUse(g,BancAlimMesure[etape].trace.nb); if(OpiumDisplayed(g->cdr)) OpiumRefresh(g->cdr);
		}
		OpiumUserAction();
		/* test sur la mesure de courant */
		if((BancAlimImesure < BancAlimMesure[etape].Imin) || (BancAlimImesure > BancAlimMesure[etape].Imax)) {
			BancResultat = BANC_MAUVAIS; break;
		}
		/* stockage dans un tableau pour graphique */
	} while(!BancUrgence && (t1 < t_lim));
	printf("  | %d mesure%s en %.3f s.\n",Accord1s(nb),BancAlimMesure[etape].duree);
	BancAlimMesure[etape].etat = BancUrgence? BANC_A_FAIRE: BANC_TERMINE;
	if(BancUrgence) sprintf(BancMessage,"Arret du test demande en urgence");
	else if(BancResultat == BANC_MAUVAIS) {
		sprintf(BancMessage,"Courant pour %s: %.3f A, hors norme ([%.3f .. %.3f])",
			BancConsosEtape[etape],BancAlimImesure,BancAlimMesure[etape].Imin,BancAlimMesure[etape].Imax);
		BancAlimMesure[etape].prgs = 100;
		InstrumCouleur(BancAlimMesure[etape].colonne,WND_GC_ROUGE);
		if(OpiumDisplayed((BancAlimMesure[etape].colonne)->cdr)) InstrumRefreshVar(BancAlimMesure[etape].colonne);
	} else if(t1 >= t_lim) {
		sprintf(BancMessage,"Courant final pour %s: %.3f A, dans la norme ([%.3f .. %.3f])",
			BancConsosEtape[etape],BancAlimImesure,BancAlimMesure[etape].Imin,BancAlimMesure[etape].Imax);
		BancResultat = BANC_OK;
		BancAlimMesure[etape].prgs = 100;
		InstrumCouleur(BancAlimMesure[etape].colonne,WND_GC_VERT);
		if(OpiumDisplayed((BancAlimMesure[etape].colonne)->cdr)) InstrumRefreshVar(BancAlimMesure[etape].colonne);
	}
	strcpy(BancAlimMesure[etape].date,DateCivile()); 
	strcpy(BancAlimMesure[etape].heure,DateHeure()); 
	BancAlimMesure[etape].resultat = BancResultat;
	if(OpiumDisplayed((BancAlimMesure[etape].panel)->cdr)) PanelRefreshVars(BancAlimMesure[etape].panel);
	BancAlimAllumeBouton(etape);
	MenuItemEteint(BancBoutonStoppe,1,0);
	printf("  | %s\n",BancMessage);
	
	return(1);
}
/* ========================================================================== */
static int BancAlim0V(Menu menu, MenuItem item) {
	printf("* Verification de la consommation pour 0V\n");
	do {
		if(!BancAlimDemande(0.0,BancAlimLimite)) continue;
		if(!BancAlimVerifie(BANC_CONSOS_0V)) continue;
	} while(0);
	BoardRefreshVars(bBanc);
	return(0);
}
/* ========================================================================== */
static int BancAlimEnService(Menu menu, MenuItem item) {
	printf("* Verification de la consommation pour %gV\n",BancAlimNominale);
	do {
		if(!BancAlimDemande(BancAlimNominale,BancAlimLimite)) continue;
		if(!BancAlimVerifie(BANC_CONSOS_NOMINAL)) continue;
	} while(0);
	OpiumRefresh(pBancMessage->cdr);
	return(0);
}
/* ========================================================================== */
static int BancAlimFpga(Menu menu, MenuItem item) {
	TypeBNModele *modele_bn; int bb; int etape;
	Graph g;
	
	printf("* Verification de la consommation apres le chargement du FPGA\n");
	etape = BANC_CONSOS_FPGA;
	BancAlimMesure[etape].etat = BANC_EN_COURS; BancResultat = BANC_INDETERMINE;
	BancUrgence = 0;
	MenuItemAllume(BancAlimMesure[etape].bouton,1,0,GRF_RGB_ORANGE);
	MenuItemAllume(BancBoutonStoppe,1,0,GRF_RGB_YELLOW);
	g = BancAlimMesure[etape].graph;
	BancAlimMesure[etape].trace.nb = 0;
	if(g) { GraphUse(g,BancAlimMesure[etape].trace.nb); if(OpiumDisplayed(g->cdr)) OpiumRefresh(g->cdr); }
	BancAlimMesure[etape].prgs = 0; InstrumCouleur(BancAlimMesure[etape].colonne,WND_GC_ORANGE);
	if(OpiumDisplayed((BancAlimMesure[etape].colonne)->cdr)) InstrumRefreshVar(BancAlimMesure[etape].colonne);

	for(bb=0; bb<NumeriseurNb; bb++) Numeriseur[bb].sel = 0;
	BancNumer->sel = 1;
	modele_bn = &(ModeleBN[BancNumer->def.type]);
	ModlNumerChoisi = modele_bn->num;
	BancEnTest = 1; BancProgression = BancAlimMesure[etape].colonne; BancPrgs = &(BancAlimMesure[etape].prgs);
	if(RepartChargeFpga(BancMessage)) BancAlimVerifie(etape);
	else {
		sprintf(BancMessage,"Chargement du FPGA en erreur");
		BancAlimMesure[etape].etat = BANC_A_FAIRE;
		BancAlimMesure[etape].prgs = 100;
		InstrumCouleur(BancAlimMesure[etape].colonne,WND_GC_ROUGE);
		if(OpiumDisplayed((BancAlimMesure[etape].colonne)->cdr)) InstrumRefreshVar(BancAlimMesure[etape].colonne);
		strcpy(BancAlimMesure[etape].date,DateCivile()); 
		strcpy(BancAlimMesure[etape].heure,DateHeure()); 
		BancAlimMesure[etape].resultat = BancResultat;
		if(OpiumDisplayed((BancAlimMesure[etape].panel)->cdr)) PanelRefreshVars(BancAlimMesure[etape].panel);
		BancAlimAllumeBouton(etape);
		MenuItemEteint(BancBoutonStoppe,1,0);
		printf("  | %s\n",BancMessage);
	}
	BancEnTest = 0;
	OpiumRefresh(pBancMessage->cdr);
	return(0);
}
/* ========================================================================== */
static int BancAlimBB(Menu menu, MenuItem item) {
	TypeBNModele *modele_bn; char nom_complet[MAXFILENAME];

	printf("* Verification de la consommation avec le numeriseur sous tension\n");
	modele_bn = &(ModeleBN[BancNumer->def.type]);
	SambaAjoutePrefPath(BancScptRacine,BancScptRepert); RepertoireTermine(BancScptRacine);
	strcat(strcpy(nom_complet,BancScptRacine),modele_bn->nom); RepertoireTermine(nom_complet);
	strcat(nom_complet,BancScptAlim);
	if(ScriptExecType(HW_NUMER,(void *)BancNumer,nom_complet,"  | ")) {
		sprintf(BancMessage,"Execute %s",nom_complet);
		OpiumRefresh(pBancMessage->cdr);
		OpiumUserAction();
		// NumeriseurMontreModifs(BancNumer);
		BancAlimVerifie(BANC_CONSOS_BB_ALIM);
	} else sprintf(BancMessage,"Script %s/%s en erreur",modele_bn->nom,BancScptAlim);
	OpiumRefresh(pBancMessage->cdr);

	return(0);
}
/* ========================================================================== */
static int BancAlimRef(Menu menu, MenuItem item) {
	TypeBNModele *modele_bn; char nom_complet[MAXFILENAME];
//	int i;
	
	printf("* Verification de la consommation avec la reference du numeriseur activee\n");
	modele_bn = &(ModeleBN[BancNumer->def.type]);
	SambaAjoutePrefPath(BancScptRacine,BancScptRepert); RepertoireTermine(BancScptRacine);
	strcat(strcpy(nom_complet,BancScptRacine),modele_bn->nom); RepertoireTermine(nom_complet);
	strcat(nom_complet,BancScptRef);
	if(ScriptExecType(HW_NUMER,(void *)BancNumer,nom_complet,"  | ")) {
		sprintf(BancMessage,"Execute %s",nom_complet);
		OpiumRefresh(pBancMessage->cdr);
		OpiumUserAction();
		// NumeriseurMontreModifs(BancNumer);
		BancAlimVerifie(BANC_CONSOS_BB_REF);
	} else sprintf(BancMessage,"Script %s/%s en erreur",modele_bn->nom,BancScptAlim);
	OpiumRefresh(pBancMessage->cdr);

	return(0);
}
/* ========================================================================== */
static int BancAlimTout(Menu menu, MenuItem item) {
	int etape;

	MenuItemAllume(BancBoutonEnchaine,1,0,GRF_RGB_GREEN);
	pour_tout(etape,BANC_CONSOS_NBETAPES) {
//		fctn = (TypeMenuFctn)BancAlimMesure[etape].fctn; (*fctn)(menu,item);
		(*(TypeMenuFctn)BancAlimMesure[etape].fctn)(menu,item);
		if(BancUrgence || (BancResultat != BANC_OK)) break;
	}
	MenuItemEteint(BancBoutonEnchaine,1,0);
	return(0);
}
/* ========================================================================== */
static Graph BancSptrGraphique(int etape, int dim, int nb) {
	Graph g; int i,k; int vbb;
	char nom_adc[8];
	
	g = BancSptrResultat[etape].graph = GraphCreate(BANC_SPTR_LARG*Dx,BANC_SPTR_HAUT*Dy,1+BancNumer->nbadc);
	if(g) {
		GraphMode(g,GRF_2AXES|GRF_GRID|GRF_LEGEND);
		GraphAxisTitle(g,GRF_XAXIS,"f (Hz)");
		GraphAxisTitle(g,GRF_YAXIS,"bruit (nV/rac(Hz))");
		OpiumColorInit(&(BancSptrResultat[etape].couleur));
		BancFrequence[1] = Echantillonnage / (float)BancSptrPoints; /* kHz */
		i = GraphAdd(g,GRF_FLOAT|GRF_XAXIS|GRF_INDEX,BancFrequence,dim);
		GraphDataName(g,i,"f(kHz)");
		pour_tout(vbb,BancSptrResultat[etape].trace.nbadc) {
			i = GraphAdd(g,GRF_FLOAT|GRF_YAXIS,BancSptrResultat[etape].trace.bruit[vbb],dim);
			k = OpiumColorNext(&(BancSptrResultat[etape].couleur));
			GraphDataRGB(g,i,OpiumColorRGB(k));
			sprintf(nom_adc,"ADC %d",vbb+1);
			GraphDataName(g,i,nom_adc);
			// printf("(%s.%d='%s') Ajoute trace#%d @%08X pour %s\n",__func__,etape,BancSptrEtape[etape],
			//	i,(hexa)(BancSptrResultat[etape].trace.bruit[vbb]),nom_adc);
		}
		GraphAjusteParms(g,GRF_XAXIS,&(BancGraphiquesParms[0]));
		GraphAjusteParms(g,GRF_YAXIS,&(BancGraphiquesParms[1]));
		GraphUse(g,nb);
	}
	return(g);
}
/* ========================================================================== */
static void BancSptrAllumeBouton(int etape) {
	cas_ou(BancSptrResultat[etape].resultat) {
	  vaut BANC_INDETERMINE:
		FigureColor(BancSptrResultat[etape].bouton,GRF_RGB_YELLOW); break;
	  vaut BANC_MAUVAIS:
		FigureColor(BancSptrResultat[etape].bouton,GRF_RGB_RED); break;
	  vaut BANC_OK:
		FigureColor(BancSptrResultat[etape].bouton,GRF_RGB_GREEN); break;
	}
	FigureRedessine(BancSptrResultat[etape].bouton,0);
//	OpiumUserAction();
}
/* ========================================================================== */
void BancSptrSelecteVoies(char log) {
	int vbb,voie;

	pour_tout(vbb,BancNumer->nbadc) if((voie = BancNumer->voie[vbb]) >= 0) {
		VoieTampon[voie].lue = 1;
		if(log) printf("  + %s (lue par %s ADC#%d)\n",VoieManip[voie].nom,BancNumer->nom,vbb+1);
	}
}
/* ========================================================================== */
static char BancSptrExec(int etape, char *explics) {
	int vbb,voie,phys; int nbfreq,dim,n;
	// char affiche_evts;

	// printf("%s/ Preparation du calcul des spectres\n",DateHeure());
	CalcSpectresMax = BancSptrMax;
	pour_tout(vbb,BancSptrResultat[etape].trace.nbadc) {
		voie = BancNumer->voie[vbb];
		CalcSpectreAutoNb[voie] = 0.0;
		phys = ModeleVoie[VoieManip[voie].type].physique;
		CalcSpectreAuto[phys].dim = BancSptrPoints;
		if(!(dim = CalcSpectreAuto[phys].dim)) continue;
		nbfreq = CalcSpectreAuto[phys].nbfreq = (CalcSpectreAuto[phys].dim / 2) + 1;
		if(CalcAutoSpectreDim[voie] != nbfreq) {
			if(CalcAutoCarreSomme[voie]) { free(CalcAutoCarreSomme[voie]); CalcAutoCarreSomme[voie] = 0; }
			if(CalcAutoSpectreMoyenne[voie]) { free(CalcAutoSpectreMoyenne[voie]); CalcAutoSpectreMoyenne[voie] = 0 ; }
			CalcAutoSpectreDim[voie] = 0;
		}
		n = nbfreq * sizeof(float);
		if(!CalcAutoCarreSomme[voie]) {
			CalcAutoCarreSomme[voie] = (float *)malloc(n);
			if(!CalcAutoCarreSomme[voie]) {
				sprintf(explics,"Manque de place memoire pour le spectre**2 somme (%d octets)",n);
				return(0);
			}
		}
		if(!CalcAutoSpectreMoyenne[voie]) {
			CalcAutoSpectreMoyenne[voie] = (float *)malloc(n);
			if(!CalcAutoSpectreMoyenne[voie]) {
				sprintf(explics,"Manque de place memoire pour le spectre moyenne (%d octets)",n);
				return(0);
			}
			CalcAutoSpectreDim[voie] = nbfreq;
		}
		if(CalcAutoCarreSomme[voie]) for(n=0; n<nbfreq; n++) CalcAutoCarreSomme[voie][n]= 0.0;
		// printf("%s/ . Voie %s prete\n",DateHeure(),VoieManip[voie].nom);
	}	
	CalcGlissantEpsilon = 1.0 / (float)CalcSpectresMax;
	// printf("%s/ Spectres automatiques prets\n",DateHeure());
	
	LectModeSpectresAuto = 1; LectModeSpectresBanc = etape + 1;
	// affiche_evts = MonitAffEvts; MonitAffEvts = 0;
	LectAcqStd();
	// MonitAffEvts = affiche_evts;
	LectModeSpectresAuto = 0; LectModeSpectresBanc = 0;
	
	return(1);
}
/* ========================================================================== */
void BancSptrRafraichi(int avance, int etape) {
	int vbb,voie,k;

	BancSptrResultat[etape].prgs = avance;
	if(OpiumDisplayed((BancSptrResultat[etape].colonne)->cdr)) InstrumRefreshVar(BancSptrResultat[etape].colonne);
	pour_tout(vbb,BancSptrResultat[etape].trace.nbadc) {
		voie = BancNumer->voie[vbb];
		pour_tout(k,BancSptrResultat[etape].trace.nb) BancSptrResultat[etape].trace.bruit[vbb][k] = CalcAutoSpectreMoyenne[voie][k];
	}
	GraphUse(BancSptrResultat[etape].graph,BancSptrResultat[etape].trace.nb);
	OpiumDisplay((BancSptrResultat[etape].graph)->cdr);
}
/* ========================================================================== */
static void BancSptrMesure(int etape) {
	TypeBNModele *modele_bn; char nom_complet[MAXFILENAME];
	
	printf("* Mesure du spectre de bruit avec %s\n",BancSptrEtape[etape]);
	BancSptrResultat[etape].etat = BANC_EN_COURS; BancResultat = BANC_INDETERMINE;
	BancUrgence = 0;
	FigureColor(BancSptrResultat[etape].bouton,GRF_RGB_ORANGE);
	FigureRedessine(BancSptrResultat[etape].bouton,0);
	MenuItemAllume(BancBoutonStoppe,1,0,GRF_RGB_YELLOW);
	BancSptrResultat[etape].prgs = 0; InstrumCouleur(BancSptrResultat[etape].colonne,WND_GC_ORANGE);
	if(OpiumDisplayed((BancSptrResultat[etape].colonne)->cdr)) InstrumRefreshVar(BancSptrResultat[etape].colonne);
//	OpiumUserAction();
	modele_bn = &(ModeleBN[BancNumer->def.type]);
	SambaAjoutePrefPath(BancScptRacine,BancScptRepert); RepertoireTermine(BancScptRacine);
	strcat(strcpy(nom_complet,BancScptRacine),modele_bn->nom); RepertoireTermine(nom_complet);
	strcat(nom_complet,BancSptrResultat[etape].script);
	BancResultat = BANC_MAUVAIS;
	sprintf(BancMessage,"Execute %s",nom_complet);
	OpiumRefresh(pBancMessage->cdr); OpiumUserAction();
	if(ScriptExecType(HW_NUMER,(void *)BancNumer,nom_complet,"  | ")) {
		sprintf(BancMessage,"%s termine, spectre en cours",BancSptrResultat[etape].script);
		OpiumRefresh(pBancMessage->cdr); OpiumUserAction();
		// NumeriseurMontreModifs(BancNumer);
		BancEnErreur = 1;
		BancEnTest = 1; BancProgression = BancAlimMesure[etape].colonne; BancPrgs = &(BancAlimMesure[etape].prgs);
		if(BancSptrExec(etape,BancMessage)) {
			BancEnTest = 0;
			// affichage du spectre
			if(BancEnErreur) sprintf(BancMessage,"Execution de la lecture en erreur");
			else {
				sprintf(BancMessage,"Spectre termine normalement");
				BancSptrRafraichi(100,etape);
				BancResultat = BANC_OK;
				InstrumCouleur(BancSptrResultat[etape].colonne,WND_GC_VERT);
			}
		}
	} else sprintf(BancMessage,"Script %s/%s en erreur",modele_bn->nom,BancSptrResultat[etape].script);
	OpiumRefresh(pBancMessage->cdr);
	if(BancResultat == BANC_MAUVAIS) InstrumCouleur(BancSptrResultat[etape].colonne,WND_GC_ROUGE);
	if(OpiumDisplayed((BancSptrResultat[etape].colonne)->cdr)) InstrumRefreshVar(BancSptrResultat[etape].colonne);
	strcpy(BancSptrResultat[etape].date,DateCivile()); 
	strcpy(BancSptrResultat[etape].heure,DateHeure()); 
	BancSptrResultat[etape].etat = BANC_TERMINE;
	BancSptrResultat[etape].resultat = BancResultat;
	if(OpiumDisplayed((BancSptrResultat[etape].panel)->cdr)) PanelRefreshVars(BancSptrResultat[etape].panel);
	BancSptrAllumeBouton(etape);
	MenuItemEteint(BancBoutonStoppe,1,0);
}
/* ========================================================================== */
static int BancSptrLance(Figure fig, WndAction *e) {
	BancSptrMesure((int)((IntAdrs)(fig->adrs))); return(0);
}
/* ========================================================================== */
static int BancSptrTout(Menu menu, MenuItem item) {
	int etape;
	
	MenuItemAllume(BancBoutonEnchaine,1,0,GRF_RGB_GREEN);
	pour_tout(etape,BANC_SPECTRE_NBETAPES)  {
		BancSptrMesure(etape);
		if(BancUrgence || (BancResultat != BANC_OK)) break;
	}
	MenuItemEteint(BancBoutonEnchaine,1,0);
	return(0);
}
/* ========================================================================== */
static int BancParmsSauve(Menu menu, MenuItem item) {
	ArgPrint(FichierPrefBanc,BancSetup);
	return(0);
}
/* ========================================================================== */
static int BancSauve(Menu menu, MenuItem item) {
	char nom_complet[MAXFILENAME],nom_bb[NUMER_NOM];
	int l;

	
	strcpy(nom_bb,BancNumer->nom); l = strlen(nom_bb);
	while(l--) if(nom_bb[l] == '#') nom_bb[l] = '_';
	sprintf(nom_complet,"%s%s/Etat_%lld",ResuPath,nom_bb,DateLong());
	RepertoireCreeRacine(nom_complet);
	ArgPrint(nom_complet,BancEtatDesc);
	printf("*  Resultats imprimes dans %s\n",nom_complet);
	return(0);
}
/* ========================================================================== */
static int BancRelit(Menu menu, MenuItem item) {
	char *fichiers[BANC_MAXETATS],*date[BANC_MAXETATS]; int nb;
	int i,n; // char commande[256]; pour relire le PDF
	char racine[MAXFILENAME],nom_complet[MAXFILENAME],nom_bb[NUMER_NOM];
	int l,etape;
	
	strcpy(nom_bb,BancNumer->nom); l = strlen(nom_bb);
	while(l--) if(nom_bb[l] == '#') nom_bb[l] = '_';
	sprintf(racine,"%s%s",ResuPath,nom_bb);
	printf("* Resultats cherches dans %s\n",racine);
	RepertoireListeCree(0,racine,fichiers,BANC_MAXETATS,&nb);
	n = 0; for(i=0; i<nb; i++) {
		if(!strncmp(fichiers[i],"Etat_",5)) date[n++] = fichiers[i] + 5;
	}
	date[n] = "\0";
	i = n - 1;
	if(OpiumReadList("Date",date,&i,13) == PNL_OK) {
		sprintf(nom_complet,"%s/Etat_%s",racine,date[i]);
		printf("  | Resultats relus dans %s\n",nom_complet);
		ArgScan(nom_complet,BancEtatDesc);
		BancInitStructures();
		pour_tout(etape,BANC_CONSOS_NBETAPES) if(BancAlimMesure[etape].delai > 0.0) {
			BancTemps[1] = BancAlimMesure[etape].delai;
			break;
		}
	}
	RepertoireListeLibere(fichiers,nb);
	return(0);
}
/* ========================================================================== */
static int BancDocument(Menu menu, MenuItem item) {
	int etape; int l; Graph g;
	char nom_bb[NUMER_NOM]; char etat[BANC_ETAT_TXT],resultat[BANC_RESUL_TXT];
	char nom_complet[MAXFILENAME],commande[MAXFILENAME];
	float fl,fh,larg;

	strcpy(nom_bb,BancNumer->nom); l = strlen(nom_bb);
	while(l--) if(nom_bb[l] == '#') nom_bb[l] = '_';
	sprintf(nom_complet,"%s%s/Doc_%lld.ps",ResuPath,nom_bb,DateLong());
	RepertoireCreeRacine(nom_complet);

	ImpressionSurSupport(IMPRESSION_FICHIER,nom_complet);
	if(!ImpressionPrete("portrait")) return(0);
	
	ImprimeNouvelleLigne(7);
#ifdef V1
	ImprimeLimite(0,90,0); ImprimeColonnes(5,0,31,45,67,90);
	ImprimeEnCol(1,"Caracterisation de numeriseur");
	ImprimeEnCol(32,"Type %s",ModeleBN[BancNumer->def.type].nom);
	ImprimeFond(37,strlen(ModeleBN[BancNumer->def.type].nom),0xFFFF,0xF000,0x8000);
	ImprimeEnCol(46,"Numero de serie: %3d",BancNumer->def.serial);
	ImprimeFond(63,3,0xFFFF,0xF000,0x8000);
	ImprimeEnCol(68,"Le %s, %s",DateCivile(),DateHeure());
	ImprimeLimite(0,90,1); ImprimeColonnes(0);
#else
	ImprimeLimite(6,78,0); ImprimeColonnes(3,6,61,84);
	ImprimeEnCol(21,"Numeriseur - mesure de performances");
	ImprimeEnCol(62,"Le %s, %s",DateCivile(),DateHeure());
	ImprimeLimite(6,78,1); ImprimeColonnes(0);
	ImprimeNouvelleLigne(1); ImprimeColonnes(3,6,20,84);
	ImprimeEnCol(7,"Type %s",ModeleBN[BancNumer->def.type].nom);
	ImprimeFond(12,strlen(ModeleBN[BancNumer->def.type].nom),0xFFFF,0xF000,0x8000);
	ImprimeEnCol(21,"Numero de serie: %3d",BancNumer->def.serial);
	ImprimeFond(38,3,0xFFFF,0xF000,0x8000);
	ImprimeLimite(6,78,1); ImprimeColonnes(0);
#endif
	
	ImprimeNouvelleLigne(3);
	// ImprimeTableauDebut(7,0,25,34,53,-61,-68,78);
	ImprimeTableauDebut(7,6,31,40,59,-67,-74,84);
	ImprimeTableauCol(0,"Test de consommation"); ImprimeTableauCol(1,"Etat"); ImprimeTableauCol(2,"Date");
	ImprimeTableauCol(3,"V (V)"); ImprimeTableauCol(4,"I (A)"); ImprimeTableauCol(5,"Resultat");
	ImprimeNouvelleLigne(1); ImprimeTableauTrait(0,5,2);
	pour_tout(etape,BANC_CONSOS_NBETAPES) {
		ArgKeyGetText(BancEtatCles,BancAlimMesure[etape].etat,etat,BANC_ETAT_TXT);
		ArgKeyGetText(BancResultatCles,BancAlimMesure[etape].resultat,resultat,BANC_RESUL_TXT);
		ImprimeNouvelleLigne(1);
		ImprimeTableauCol(0,BancConsosEtape[etape]);
		ImprimeTableauCol(1,etat);
		ImprimeTableauCol(2,"%s %s",BancAlimMesure[etape].date,BancAlimMesure[etape].heure);
		ImprimeTableauCol(3,"%-6.3f",BancAlimMesure[etape].tension);
		ImprimeTableauCol(4,"%5.3f",BancAlimMesure[etape].courant);
		ImprimeTableauCol(5,resultat);
		cas_ou(BancAlimMesure[etape].resultat) {
			vaut BANC_INDETERMINE: ImprimeFond(ImprimeTableauPos(5)+1,BANC_RESUL_TXT,0xFFFF,0xF000,0x8000); break;
			vaut BANC_MAUVAIS:     ImprimeFond(ImprimeTableauPos(5)+1,BANC_RESUL_TXT,0xFFFF,0x0000,0x0000); break;
			vaut BANC_OK:          ImprimeFond(ImprimeTableauPos(5)+1,BANC_RESUL_TXT,0x0000,0xFFFF,0x0000); break;
		}
	}
	ImprimeTableauFin();

	ImprimeNouvelleLigne(1);
	// ImprimeTableauDebut(5,0,25,34,53,78);
	ImprimeTableauDebut(5,6,31,40,59,84);
	ImprimeTableauCol(0,"Test de bruit"); ImprimeTableauCol(1,"Etat"); ImprimeTableauCol(2,"Date");
	ImprimeTableauCol(3,"Resultat");
	ImprimeNouvelleLigne(1); ImprimeTableauTrait(0,5,2);
	pour_tout(etape,BANC_SPECTRE_NBETAPES)  {
		ArgKeyGetText(BancEtatCles,BancSptrResultat[etape].etat,etat,BANC_ETAT_TXT);
		ArgKeyGetText(BancResultatCles,BancSptrResultat[etape].resultat,resultat,BANC_RESUL_TXT);
		ImprimeNouvelleLigne(1);
		ImprimeTableauCol(0,BancSptrEtape[etape]);
		ImprimeTableauCol(1,etat);
		ImprimeTableauCol(2,"%s %s",BancSptrResultat[etape].date,BancSptrResultat[etape].heure);
		ImprimeTableauCol(3,resultat);
		cas_ou(BancSptrResultat[etape].resultat) {
			vaut BANC_INDETERMINE: ImprimeFond(ImprimeTableauPos(3)+1,BANC_RESUL_TXT,0xFFFF,0xF000,0x8000); break;
			vaut BANC_MAUVAIS:     ImprimeFond(ImprimeTableauPos(3)+1,BANC_RESUL_TXT,0xFFFF,0x0000,0x0000); break;
			vaut BANC_OK:          ImprimeFond(ImprimeTableauPos(3)+1,BANC_RESUL_TXT,0x0000,0xFFFF,0x0000); break;
		}
	}
	ImprimeTableauFin();
	ImpressionFerme();
	
	pour_tout(etape,BANC_SPECTRE_NBETAPES) /* if(BancSptrResultat[etape].resultat > BANC_INDETERMINE) */ {
		if(!(g = BancSptrResultat[etape].graph)) {
			if(!(g = BancSptrGraphique(etape,(BancSptrPoints / 2) + 1,BancSptrResultat[etape].trace.nb))) continue;
		}
		GraphTitle(g,BancSptrEtape[etape]);
		OpiumDisplay(g->cdr);
	}
	fl = (21.0 - ((float)OPIUM_PSMARGE_X / WND_PIXEL_CM)) / (float)(BANC_SPTR_LARG * Dx);
	fh = (29.7 - ((float)OPIUM_PSMARGE_Y / WND_PIXEL_CM)) / (float)(BANC_SPTR_HAUT * Dy) / 2.0;
	larg = 20.0; if((fl > fh) && (fl > 0.0)) larg *= (fh / fl);	
	OpiumPSlargeur(larg);
	if(OpiumPSInit(nom_complet,1)) {
		// OpiumPSPosition(OPIUM_PSMARGE_X,11*(ImpressionLigneCourante()+2));
		// printf("(%s) Dessin en (%d, %d)\n",__func__,OPIUM_PSMARGE_X,11*(ImpressionLigneCourante()+2));
		OpiumPSNouvellePage();
		pour_tout(etape,BANC_SPECTRE_NBETAPES) /* if(BancSptrResultat[etape].resultat > BANC_INDETERMINE) */ {
			if((g = BancSptrResultat[etape].graph)) {
				OpiumPSRecopie(g->cdr);
				if(OpiumDisplayed(g->cdr)) { OpiumClear(g->cdr); BancSptrResultat[etape].graph = 0; }
			}
		}
		OpiumPSClose();
		OpiumPSformat(OPIUM_PS_ECRAN);
	}
	
	ImpressionPStoPDF(nom_complet);
	printf("*  Document cree dans %s\n",nom_complet);
	sprintf(commande,"open -a /Applications/Preview.app %s &",nom_complet);
	system(commande);

	return(0);
}
/* ========================================================================== */
static Menu mBancSauvegardes;
static TypeMenuItem iBancSauvegardes[] = {
	{ "Enregistrer parametres", MNU_FONCTION BancParmsSauve },
	{ "Enregistrer resultats",  MNU_FONCTION BancSauve },
	{ "Reprendre resultats",    MNU_FONCTION BancRelit },
	{ "Creer document",         MNU_FONCTION BancDocument },
	MNU_END
};
/* ========================================================================== */
static void BancAfficheOnglet(int numero) {
	Panel p; Figure fig; Graph g; Instrum instrum; Cadre cdr; int x,y;
	int i,vbb,voie,bolo,sptr_dim,nb_prec; long etape; int nbligs;
	char table_definie;
	
	sptr_dim = (BancSptrPoints / 2) + 1;
	if(bBanc) {
		if(OpiumDisplayed(bBanc)) OpiumClear(bBanc);
		BoardTrash(bBanc);
		pour_tout(etape,BANC_SPECTRE_NBETAPES) BancSptrResultat[etape].graph = 0;
	#ifndef BANC_MAXPOINTS
		pour_tout(etape,BANC_CONSOS_NBETAPES) if(BancAlimMesure[etape].trace.I) {
			free(BancAlimMesure[etape].trace.I);
			BancAlimMesure[etape].trace.I = 0;
			BancAlimMesure[etape].trace.nb = 0;
		}
	#endif
		pour_tout(etape,BANC_SPECTRE_NBETAPES) {
			if(BancSptrResultat[etape].trace.nb != sptr_dim) {
				pour_tout(vbb,BancSptrResultat[etape].trace.nbadc) {
					if(BancSptrResultat[etape].trace.bruit[vbb]) {
						free(BancSptrResultat[etape].trace.bruit[vbb]);
						BancSptrResultat[etape].trace.bruit[vbb] = 0;
					}
				}
				BancSptrResultat[etape].trace.nb = 0;
			}
		}
	} else bBanc = BoardCreate();
	if(!bBanc) return;
	BancNumer = NumeriseurChoix.adrs;
	OpiumTitle(bBanc,"Banc de test Numeriseur");
	
	BancFeuilles = OngletDefini(BancFeuillesNom,BancAfficheOnglet);
	OngletDemande(BancFeuilles,numero);
	BoardAddOnglet(bBanc,BancFeuilles,0);
	
	BancPhase = numero;
	
	x = Dx; y = 3 * Dy;
	
	PrevenirSi(BancPhase >= BANC_NBPHASES);
	cas_ou(BancPhase) {
	  vaut BANC_PARMS:
		p = PanelArgs(BancAlimParmsDesc); // PanelDesc(BancAlimParmsDesc,1); pour avoir les commentaires
		PanelSupport(p,WND_CREUX);
		BoardAddPanel(bBanc,p,x,y,0);
		nbligs = PanelItemNb(p);
		cdr = p->cdr;
		
		p = PanelArgs(BancAlimTolerDesc);
		PanelSupport(p,WND_CREUX);
		BoardAddPanel(bBanc,p,OPIUM_A_DROITE_DE 0);
		if(nbligs < PanelItemNb(p)) nbligs = PanelItemNb(p);
		
		p = PanelArgs(BancSptrParmsDesc);
		PanelSupport(p,WND_CREUX);
		BoardAddPanel(bBanc,p,OPIUM_EN_DESSOUS_DE cdr);
		cdr = p->cdr;
		
		p = PanelArgs(BancGraphiquesDesc); // PanelDesc(BancAlimParmsDesc,1); pour avoir les commentaires
		PanelSupport(p,WND_CREUX);
		BoardAddPanel(bBanc,p,70 * Dx,(nbligs + 6) * Dy,0);
		
		mBancSauvegardes = MenuLocalise(iBancSauvegardes);
		OpiumSupport(mBancSauvegardes->cdr,WND_PLAQUETTE);
		BoardAddMenu(bBanc,mBancSauvegardes,20 * Dx,(nbligs + 13) * Dy,0); // OPIUM_EN_DESSOUS_DE cdr);
		
		break;
		
	  vaut BANC_CONSOS:
		p = PanelCreate(1); PanelMode(p,PNL_DIRECT|PNL_RDONLY);
		PanelText(p,"Numeriseur en test",BancNumer->nom,NUMER_NOM);
		PanelSupport(p,WND_CREUX);
		BoardAddPanel(bBanc,p,x,y,0);
		
		p = pBancMessage = PanelCreate(1); PanelMode(p,PNL_DIRECT|PNL_RDONLY);
		PanelSupport(p,WND_CREUX);
		PanelText(p,0,BancMessage,BANC_MSG_LNGR);
		BoardAddPanel(bBanc,p,OPIUM_A_DROITE_DE 0);
		
		y += (2 * Dy);
		fig = FigureCreate(FIG_DROITE,(void *)(&BancSeparation),0,0);
		BoardAddFigure(bBanc,fig,x,y,0);
		y += Dy;
		
		BoardAddBouton(bBanc,"Connexion",(void *)BancConnecte,x+Dx,y,0); 

		p = pBancConnexions = PanelCreate(2); PanelMode(p,PNL_BYLINES|PNL_DIRECT|PNL_RDONLY); PanelColumns(p,2);
		PanelKeyB(p,"Alim","Inutilisable/Connectee",&BancAlimConnectee,13); PanelItemColors(p,1,SambaRougeVertJauneOrange,2);
		PanelKeyB(p,"Gene","Inutilisable/Connecte",&BancGeneConnecte,13); PanelItemColors(p,2,SambaRougeVertJauneOrange,2);
		PanelSupport(p,WND_CREUX);
		BoardAddPanel(bBanc,p,x + (11 * Dx),y,0);
		
		p = pBancAlimDemande = PanelCreate(2); PanelMode(p,PNL_DIRECT);
		PanelSupport(p,WND_CREUX);
		i = PanelFloat(p,"Tension nominale",&BancAlimTension); PanelItemLngr(p,PanelItemFormat(p,i,"%.3f"),7);
		i = PanelFloat(p,"Courant limite",&BancAlimCourant); PanelItemLngr(p,PanelItemFormat(p,i,"%.3f"),7);
		BoardAddPanel(bBanc,p,OPIUM_A_DROITE_DE 0);
		
		//			cdr = p->cdr;
		p = pBancAlimMesure = PanelCreate(2); PanelMode(p,PNL_DIRECT|PNL_RDONLY);
		i = PanelFloat(p,"Tension effective",&BancAlimVmesure); PanelItemLngr(p,PanelItemFormat(p,i,"%.3f"),7);
		i = PanelFloat(p,"Courant effectif ",&BancAlimImesure); PanelItemLngr(p,PanelItemFormat(p,i,"%.3f"),7);
		PanelSupport(p,WND_CREUX);
		BoardAddPanel(bBanc,p,OPIUM_A_DROITE_DE 0);

		BoardAddBouton(bBanc,"Eteindre",(void *)BancAlimEteint,OPIUM_A_DROITE_DE 0); 

		y += (3 * Dy);
		fig = FigureCreate(FIG_DROITE,(void *)(&BancSeparation),0,0);
		BoardAddFigure(bBanc,fig,x,y,0);
		y += Dy;
		
		cdr = BoardAddBouton(bBanc,"Enchainer",BancAlimTout,x+Dx,y,0); // x + (57 * Dx),y,0);
		BancBoutonEnchaine = (Menu)(cdr->adrs);
		cdr = BoardAddBouton(bBanc,"Stopper",BancArretUrgence,OPIUM_A_DROITE_DE 0);
		BancBoutonStoppe = (Menu)(cdr->adrs);
		y += (2 * Dy);
		
		nbligs = 7;
		pour_tout(etape,BANC_CONSOS_NBETAPES) {
			//				y += Dy;
			fig = FigureCreate(FIG_DROITE,(void *)(&BancSeparation),0,0);
			BoardAddFigure(bBanc,fig,x,y,0);
			y += Dy;
			cdr = BoardAddBouton(bBanc,BancConsosEtape[etape],BancAlimMesure[etape].fctn,x+Dx,y,0);
			BancAlimMesure[etape].bouton = (Menu)(cdr->adrs);
			BancAlimAllumeBouton(etape);
			
			p = BancAlimMesure[etape].panel = PanelCreate(7); PanelMode(p,PNL_DIRECT|PNL_RDONLY);
			PanelSupport(p,WND_CREUX);
			i = PanelKeyB (p,"Etat",BancEtatCles,&BancAlimMesure[etape].etat,BANC_ETAT_TXT);
			PanelItemColors(p,i,BancJauneOrangeVert,3);
			i = PanelText(p,"le",BancAlimMesure[etape].date,9); PanelItemLngr(p,i,8);
			i = PanelText(p,"a",BancAlimMesure[etape].heure,9); PanelItemLngr(p,i,8);
			i = PanelFloat(p,"Duree (s)",&BancAlimMesure[etape].duree); PanelItemLngr(p,PanelItemFormat(p,i,"%.3f"),7);
			i = PanelFloat(p,"Tension effective (V)",&BancAlimMesure[etape].tension); PanelItemLngr(p,PanelItemFormat(p,i,"%.3f"),7);
			i = PanelFloat(p,"Courant effectif  (A)",&BancAlimMesure[etape].courant); PanelItemLngr(p,PanelItemFormat(p,i,"%.3f"),7);
			i = PanelKeyB (p,"Resultat",BancResultatCles,&BancAlimMesure[etape].resultat,BANC_RESUL_TXT);
			PanelItemColors(p,i,BancNoirRougeVert,3);
			BoardAddPanel(bBanc,p,x+(26 * Dx),y,0);
			
			BancAlimColonne.longueur = -7 * Dy;
			instrum = BancAlimMesure[etape].colonne = InstrumInt(INS_COLONNE,(void *)&BancAlimColonne,&BancAlimMesure[etape].prgs,0,100);
			InstrumSupport(instrum,WND_CREUX); InstrumCouleur(instrum,WND_GC_VERT);
			BoardAddInstrum(bBanc,instrum,x+(61 * Dx),y,0);

		#ifdef BANC_MAXPOINTS
			table_definie = 1;
		#else
			BancAlimMesure[etape].trace.I = Malloc(BancMesurePoints,float);
			table_definie = (BancAlimMesure[etape].trace.I);
		#endif
			if(table_definie) {
				nbligs = 7;
				g = BancAlimMesure[etape].graph = GraphCreate(50*Dx,nbligs*Dy,2);
				GraphMode(g,GRF_2AXES|GRF_GRID); //  | GRF_LEGEND
				GraphAxisTitle(g,GRF_XAXIS,"t (s)");
				GraphAxisTitle(g,GRF_YAXIS,"I (A)");
				GraphAdd(g,GRF_FLOAT|GRF_XAXIS|GRF_INDEX,BancTemps,BancMesurePoints);
				i = GraphAdd(g,GRF_FLOAT|GRF_YAXIS,BancAlimMesure[etape].trace.I,BancMesurePoints);
				GraphDataRGB(g,i,GRF_RGB_GREEN);
				GraphUse(g,BancAlimMesure[etape].trace.nb);
				GraphParmsSave(g);
				OpiumSupport(g->cdr,WND_CREUX);
				BoardAddGraph(bBanc,g,x+(71 * Dx),y,0);
			} else BancAlimMesure[etape].graph = 0;
			y += ((nbligs + 1) * Dy);
		}
		break;
		
	  vaut BANC_FET:
		for(bolo=0; bolo<BoloNb; bolo++) CalcAutoBolo[bolo] = 0;
		voie = BancNumer->voie[0];
		if((bolo = VoieManip[voie].det) >= 0) CalcAutoBolo[bolo] = 1;
		
		p = PanelCreate(1); PanelMode(p,PNL_DIRECT|PNL_RDONLY);
		PanelText(p,"Numeriseur en test",BancNumer->nom,NUMER_NOM);
		PanelSupport(p,WND_CREUX);
		BoardAddPanel(bBanc,p,x,y,0);
		
		p = pBancMessage = PanelCreate(1); PanelMode(p,PNL_DIRECT|PNL_RDONLY);
		PanelSupport(p,WND_CREUX);
		PanelText(p,0,BancMessage,BANC_MSG_LNGR);
		BoardAddPanel(bBanc,p,OPIUM_A_DROITE_DE 0);
		
		y += (2 * Dy);
		fig = FigureCreate(FIG_DROITE,(void *)(&BancSeparation),0,0);
		BoardAddFigure(bBanc,fig,x,y,0);
		y += Dy;
		
		cdr = BoardAddBouton(bBanc,"Enchainer",BancSptrTout,x + Dx,y,0);
		BancBoutonEnchaine = (Menu)(cdr->adrs);
		cdr = BoardAddBouton(bBanc,"Stopper",BancArretUrgence,OPIUM_A_DROITE_DE 0);
		BancBoutonStoppe = (Menu)(cdr->adrs);
		y += (2 * Dy);
		
		nbligs = BANC_SPTR_HAUT;
		pour_tout(etape,BANC_SPECTRE_NBETAPES) {
			BancSptrResultat[etape].trace.nbadc = BancNumer->nbadc;
			nb_prec = BancSptrResultat[etape].trace.nb;
			if(BancSptrResultat[etape].trace.nb == 0) {
				pour_tout(vbb,BancSptrResultat[etape].trace.nbadc) {
					BancSptrResultat[etape].trace.bruit[vbb] = Malloc(sptr_dim,float);
					if(!(BancSptrResultat[etape].trace.bruit[vbb])) break;
				}
				if(vbb < BancSptrResultat[etape].trace.nbadc) {
					OpiumError("Espace memoire insuffisant"); break;
				}
				BancSptrResultat[etape].trace.nb = sptr_dim;
			}

			fig = FigureCreate(FIG_DROITE,(void *)(&BancSeparation),0,0);
			BoardAddFigure(bBanc,fig,x,y,0);
			y += Dy;
			//		cdr = BoardAddBouton(bBanc,BancSptrEtape[etape],BancSptrResultat[etape].fctn,x+Dx,y,0);
			//		BancSptrResultat[etape].bouton = (Menu)(cdr->adrs);
			//		BancAlimAllumeBouton(etape);
			fig = BancSptrResultat[etape].bouton = FigureCreate(FIG_ZONE,(void *)(&BancBoutonFet),(void *)etape,BancSptrLance);
			FigureTexte(fig,BancSptrEtape[etape]);
			// BancSptrAllumeBouton(etape); (FigureRedessine) Figure figure68: pas dans une fenetre
			BoardAddFigure(bBanc,fig,x+Dx,y,0);
			
			p = BancSptrResultat[etape].panel = PanelCreate(4); PanelMode(p,PNL_DIRECT|PNL_RDONLY);
			PanelSupport(p,WND_CREUX);
			i = PanelKeyB (p,"Etat",BancEtatCles,&BancSptrResultat[etape].etat,9);
			PanelItemColors(p,i,BancJauneOrangeVert,3);
			i = PanelText(p,"le",BancSptrResultat[etape].date,9); PanelItemLngr(p,i,8);
			i = PanelText(p,"a",BancSptrResultat[etape].heure,9); PanelItemLngr(p,i,8);
			i = PanelKeyB (p,"Resultat",BancResultatCles,&BancSptrResultat[etape].resultat,8);
			PanelItemColors(p,i,BancNoirRougeVert,3);
			BoardAddPanel(bBanc,p,x+(26 * Dx),y,0);
		
			BancSptrColonne.longueur = -nbligs * Dy;
			instrum = BancSptrResultat[etape].colonne = InstrumInt(INS_COLONNE,(void *)&BancSptrColonne,&BancSptrResultat[etape].prgs,0,100);
			InstrumSupport(instrum,WND_CREUX); InstrumCouleur(instrum,WND_GC_VERT);
			BoardAddInstrum(bBanc,instrum,x+(49 * Dx),y,0);
			
			if((g = BancSptrGraphique(etape,sptr_dim,nb_prec))) {
				GraphParmsSave(g);
				OpiumSupport(g->cdr,WND_CREUX);
				BoardAddGraph(bBanc,g,x+(59 * Dx),y,0);
			}

			y += ((nbligs + 1) * Dy);
			
		}
		break;
		
	  au_pire: break;
	}
	printf("\n= Utilisation du banc de tests pour %s %s =====\n",BancFeuillesNom[BancPhase],BancNumer->nom);
	
	OpiumFork(bBanc);
}
/* ========================================================================== */
int BancAffichePlanche(Menu menu, MenuItem item) { BancAfficheOnglet(BancPhase); return(0); }
/* ========================================================================== */
static void BancInitStructures() {
	int etape;

	pour_tout(etape,BANC_CONSOS_NBETAPES) BancAlimMesure[etape].etape = etape;
	BancAlimMesure[BANC_CONSOS_0V].fctn = (void *)BancAlim0V;
	BancAlimMesure[BANC_CONSOS_NOMINAL].fctn = (void *)BancAlimEnService;
	BancAlimMesure[BANC_CONSOS_FPGA].fctn = (void *)BancAlimFpga;
	BancAlimMesure[BANC_CONSOS_BB_ALIM].fctn = (void *)BancAlimBB;
	BancAlimMesure[BANC_CONSOS_BB_REF].fctn = (void *)BancAlimRef;
	BancAlimNb = BANC_CONSOS_NBETAPES;
	pour_tout(etape,BANC_SPECTRE_NBETAPES)  {
		BancSptrResultat[etape].etape = etape;
		snprintf(BancSptrResultat[etape].script,BANC_MAXSCPTNOM,"Test%s",BancSptrCode[etape]);
	}
}
/* ========================================================================== */
int BancInit() {
	int j; int etape; ArgDesc *elt;

	BancBoutonFet.larg = 22 * Dx;
	BancBoutonFet.haut = Dy;
	BancSeparation.dx = 132 * Dx; // pour seulement les infos "etape": 60 * Dx;
	BancInitStructures();
	BancSptrNbSauves = 90 * Dx; // la largeur du graphique en pixels

	strcpy(BancAlimIpAdrs,"132.166.33.18"); BancAlimIpPort = 5025;
#ifdef AVEC_GENE
	strcpy(stHostnameGene33220A,"132.166.33.15"); portnoGene33220A = 5025;
#endif
	BancAlimSkt = BancGeneSkt = 0;
	BancAlimNominale = 18.0; BancAlimLimite = 1.0;
	BancAlimTension = BancAlimCourant = 0.0;
	BancAlimVoie = 1;
	BancMesureDuree = 10;
	BancMesureDelai = 0.2;
#ifdef BANC_MAXPOINTS
	BancMesurePoints = BANC_MAXPOINTS;
#else
	BancMesurePoints = 256;
#endif
	BancMesureTolerance = 5.0;
	BancSptrPoints = 131072;
	BancSptrMax = 20;
	strcpy(BancScptRepert,"$maitre/../modeles_EDW/ScriptsNumeriseurs/");
	strcpy(BancScptAlim,"alim_on");
	strcpy(BancScptRef,"references");
	BancGraphiquesParms[0].log = 0;
	BancGraphiquesParms[0].autom = 1;
	BancGraphiquesParms[0].lim.r.min = 0;
	BancGraphiquesParms[0].lim.r.max = 50;
	BancGraphiquesParms[1].log = 1;
	BancGraphiquesParms[1].autom = 1;
	BancGraphiquesParms[1].lim.r.min = 0.1;
	BancGraphiquesParms[1].lim.r.max = 1000;
	BancAlimParmsPtr = BancAlimParmsDesc;
	BancGraphiquesPtr = BancGraphiquesDesc;
	BancAlimTolerDesc = ArgCreate(2 * BANC_CONSOS_NBETAPES);
	elt = BancAlimTolerDesc; j = 0;
	pour_tout(etape,BANC_CONSOS_NBETAPES) {
		BancAlimMesure[etape].duree = 0;
		BancAlimMesure[etape].tension = BancAlimMesure[etape].courant = 0.0;
		BancAlimMesure[etape].etat = BANC_A_FAIRE;
		strcpy(BancAlimMesure[etape].date,"---");
		strcpy(BancAlimMesure[etape].heure,"---");
		BancAlimMesure[etape].resultat = BANC_INDETERMINE;
		BancAlimMesure[etape].prgs = 0;
		BancAlimMesure[etape].trace.nb = 0;
	#ifndef BANC_MAXPOINTS
		BancAlimMesure[etape].trace.I = 0;
	#endif
		BancAlimMesure[etape].bouton = 0;
		BancAlimMesure[etape].panel = 0;
		BancAlimMesure[etape].graph = 0;
		BancAlimMesure[etape].colonne = 0;
		snprintf(&(BancConsosVariable[j][0]),BANC_MAXVARNOM,"I.min.%s",BancConsosCode[etape]);
		ArgAdd(elt++,&(BancConsosVariable[j++][0]),DESC_FLOAT &(BancAlimMesure[etape].Imin),0);
		snprintf(&(BancConsosVariable[j][0]),BANC_MAXVARNOM,"I.max.%s",BancConsosCode[etape]);
		ArgAdd(elt++,&(BancConsosVariable[j++][0]),DESC_FLOAT &(BancAlimMesure[etape].Imax),0);
	}
	
	BancSptrParmsDesc = ArgCreate(BANC_SPECTRE_NBETAPES);
	elt = BancSptrParmsDesc;
	pour_tout(etape,BANC_SPECTRE_NBETAPES)  {
		BancSptrResultat[etape].etat = BANC_A_FAIRE;
		strcpy(BancSptrResultat[etape].date,"---");
		strcpy(BancSptrResultat[etape].heure,"---");
		BancSptrResultat[etape].resultat = BANC_INDETERMINE;
		BancSptrResultat[etape].prgs = 0;
		BancSptrResultat[etape].trace.nb = 0;
		BancSptrResultat[etape].trace.nbadc = 0;
		pour_tout(j,NUMER_ADC_MAX) BancSptrResultat[etape].trace.bruit[j] = 0;
		BancSptrResultat[etape].bouton = 0;
		BancSptrResultat[etape].panel = 0;
		BancSptrResultat[etape].graph = 0;
		BancSptrResultat[etape].colonne = 0;
		snprintf(&(BancSptrVariable[etape][0]),BANC_MAXVARNOM,"script.%s",BancSptrCode[etape]);
		ArgAdd(elt++,&(BancSptrVariable[etape][0]),DESC_STR(BANC_MAXSCPTNOM) &(BancSptrResultat[etape].script),0);
	}
	BancSptrNb = BANC_SPECTRE_NBETAPES;

	ArgScan(FichierPrefBanc,BancSetup);
	
	BancAlimConnectee = BancGeneConnecte = 0;
	BancAlimVmesure = BancAlimImesure = 0.0;
	BancMessage[0] = '\0'; BancUrgence = 0;
	BancPhase = BANC_CONSOS;
	BancTemps[0] = 0.0; BancTemps[1] = 1.0;
	BancFrequence[0] = 0.0; BancFrequence[1] = 1.0;
	
	bBanc = 0;

	return(0);
}
