#ifdef macintosh
#pragma message(__FILE__)
#endif

#define TRAITMT
// #define DEBUG_TRIGGER
// #define DEBUG_PRGM

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <environnement.h>

#ifndef CODE_WARRIOR_VSN
/* pour getpriority */
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <dateheure.h>
//++ #include <utils/Paw.h>
#include <ceb.h>
#include <decode.h>
#include <opium_demande.h>
#include <opium.h>
#include <nanopaw.h>

#include <samba.h>
// #include <cmdes.h>
#include <cablage.h>
#include <gene.h>
#include <archive.h>
#include <objets_samba.h>
#include <monit.h>

/* ................... structures propres au traitement ................ */
static CebVar *TrmtVar;

static char TrmtDebug = 0;
static char TrmtDbgCale = 0;
static char TrmtDbgPmax = 0;
static char TrmtMesureFineActive,TrmtMesureFineTerminee;
static int TrmtAppelsNb;
static int TrmtDejaDit;
static char TrmtImmediat;
static char TrmtModePanique;
static char TrmtCategIndefinie;
static int TrmtPanicNb,TrmtPanicTotal;
static int64 TrmtPanicAjoute;
static int TrmtTropTard;
static int TrmtGenere;
static int64 TrmtHeureDernier;
static int TrmtVoieInitiale;
static int TrmtDataNb;

// tableau intermediaire pour le mode ARCH_COINC
static TypeVoieArchivee TrmtMesuree[VOIES_MAX]; /* liste des voies mesurees (pointe par 'en_cours') */

#ifdef PAW_H
static int HbId = 99;
static char *HbVarNoms = "Voie,Type,Base,Ampl,Mont,Desc,Bruit,Delai"; 
typedef struct {
	float Voie ;
	float Type ;
	float Base ; 
	float Ampl ;
	float Mont ;
	float Desc ;
	float Bruit;
	float Delai;
} TypeHbNtuple;
static TypeHbNtuple HbVar;
#endif

#ifdef VISU_FONCTION_TRANSFERT
static float *Xfiltre,*Yfiltre,*Hfiltre;
static int Hnb; static float TrmtFx[2];
static Graph gTrmtFiltre;
#endif

typedef struct {
    int64 point_traite;    /* points stockes */
    int64 debut_recherche; /* points stockes */
	int64 delai;           /* points absolus */
	int64 date_sortie;     /* points absolus */
    int trig;
    char evt_en_cours;
	char reprise;
	short accepte;
#ifdef UTILISE_EN_DEMARRAGE
	char en_demarrage;
#endif
} TypeTrgrEtat;
static TypeTrgrEtat TrgrEtat[VOIES_MAX];

typedef struct {
	int reference;
	char objet;
	char prevu;
	char demande;
	char a_faire;
} TypeTrmtDate;
static TypeTrmtDate Calage;   // Initialament, moment du calcul de 'amplitude
#ifdef CENTRAGE_INDEPENDANT
	static TypeTrmtDate Centrage; // Alignement des intervalles sauvegardes
#endif
static char CalageSurTriggee,CalageExtreme;

static struct {
	int64 date;
	float amplitude;
	int voie;
	char est_defini;
} TrgrRef[PHYS_TYPES+1];

static TypeEvtDetecte *EvtATraiter;
static int64 TrmtDernierTrig;
static float AmplitudeMaxi;

static char TrmtAbandonne,TrmtAccepte;
static char TrmtAccepteParDefaut,TrmtNonConformeParDefaut;
static char TrmtNonConforme,VetoPossible,VetoMis;

static int DerniereVoieSignalee;

static void TrmtMaxConvolution(int voie, TypeTamponDonnees *tampon, int lngr, int adrs_mini,
	float base, float *evtunite, int dim, int nb, float *dev, int *temps);

void MonitHistosDynamique();
int TrmtPersoRAZ();
void TrgrPerso();

/* ========================================================================== */
//-- #ifdef TRAITMT
		/* B[bolo].wsliPtr[*] represente les donnees brutes sur un bloc
			(de taille BlocBins) lissees par un facteur SliceBins
			(soit BlocSlices valeurs).
		   B[bolo].wsliprevbloc est la derniere valeur du bloc precedent,
		   lequel s'appelle prevbloctofilter.
		   Le bloc courant s'appelle, lui, bloctofilter
		   La dessus, on cherche le trigger...
		*/
/* ========================================================================== */
void TrmtInit() {
	TrmtAttendComplet = 0;
	BitTriggerDim = 0;
	EvtATraiter = &(EvtStocke[0]);

#ifdef VISU_FONCTION_TRANSFERT
	Hnb = 0; Xfiltre = Yfiltre = Hfiltre = 0; gTrmtFiltre = 0;
	if(!ModeBatch) gTrmtFiltre = GraphCreate(300,300,4);
#endif
#ifdef PAW_H
	PawInit();
#endif
}
/* ========================================================================== */
void TrmtRAZvoie(int voie) {
	int j; TypeRegulInterv *parm;

	TrgrEtat[voie].point_traite = -1;
	TrgrEtat[voie].debut_recherche = 0;
	TrgrEtat[voie].reprise = 0;
	TrgrEtat[voie].evt_en_cours = 0;
	if(VoieTampon[voie].trig.trgr->conn.out) {
		if(VoieTampon[voie].trig.trgr->conn.delai <= 0) TrgrEtat[voie].delai = 1;
		else TrgrEtat[voie].delai = (int64)(VoieTampon[voie].trig.trgr->conn.delai * Echantillonnage);
	} else TrgrEtat[voie].delai = -1;
	if(VoieTampon[voie].trig.trgr->conn.in) {
		int bolo,cap;
		bolo = VoieTampon[voie].trig.trgr->conn.bolo;
		cap = VoieTampon[voie].trig.trgr->conn.cap;
		if((bolo >= 0) && (cap >= 0) && (cap < Bolo[bolo].nbcapt)) TrgrEtat[voie].accepte = Bolo[bolo].captr[cap].voie;
	} else TrgrEtat[voie].accepte = -1;
	TrgrEtat[voie].date_sortie = -1;
	bzero(&(VoieTampon[voie].suivi),sizeof(TypeVoieSuivi));
	VoieTampon[voie].signal.climbs = 0;
	VoieTampon[voie].signal.base = 0;
	VoieTampon[voie].signal.nb = 0;
	VoieTampon[voie].signal.somme = 0.0;
	VoieTampon[voie].signal.carre = 0.0;
	// VoieTampon[voie].signal.moyenne = 0.0;
	VoieTampon[voie].signal.bruit = 0.0;
	VoieTampon[voie].gene.futur_evt = 0;
	VoieTampon[voie].gene.max = 0.0;
	VoieTampon[voie].gene.etape = -1;
	VoieTampon[voie].gene.demiperiode = Bolo[VoieManip[voie].det].d2 / 2;
	VoieTampon[voie].gene.phase = -VoieTampon[voie].gene.demiperiode;
	VoieTampon[voie].taux = 0.0;
	if(VoieTampon[voie].regul_active == TRMT_REGUL_TAUX) {
		for(j=0; j<MAXECHELLES; j++) VoieTampon[voie].rgul.taux.nbevts[j] = 0;
	} else if(VoieTampon[voie].regul_active == TRMT_REGUL_INTERV) {
		for(j=0; j<MAXREGULINTERV; j++) {
			VoieTampon[voie].rgul.interv.min[j] = VoieTampon[voie].rgul.interv.max[j] = 0.0;
			VoieTampon[voie].rgul.interv.evtp[j] = 0;
			VoieTampon[voie].rgul.interv.evtm[j] = 0;
		}
		// if(VoieTampon[voie].regul_standard) parm = &TrmtRegulInterv; else parm = &(VoieManip[voie].interv);
		parm = &(VoieManip[voie].interv);
		VoieTampon[voie].rgul.interv.lngr = parm->duree * 1000.0 / VoieEvent[voie].horloge;
		VoieTampon[voie].rgul.interv.limp = (TypeDonnee)(VoieTampon[voie].trig.trgr->minampl * parm->seuil);
		VoieTampon[voie].rgul.interv.limm = (TypeDonnee)(VoieTampon[voie].trig.trgr->maxampl * parm->seuil);
		VoieTampon[voie].rgul.interv.derp = VoieTampon[voie].rgul.interv.derm = 0.0;
		VoieTampon[voie].rgul.interv.moyp = VoieTampon[voie].rgul.interv.moym = 0.0;
		VoieTampon[voie].rgul.interv.nbp  = VoieTampon[voie].rgul.interv.nbm  = 0.0;
		VoieTampon[voie].rgul.interv.ovrp = VoieTampon[voie].rgul.interv.ovrm = 0;
		VoieTampon[voie].rgul.interv.used = 0;
	}
}
/* ========================================================================== */
int TrmtImprimeTrgr() {
	size_t l; int k,lngr;
	int voie,vm,nb; int j,lig;
	char fichier_scrpt[256],ligne[256]; FILE *f;
	char action[16];
	char std[VOIES_TYPES],std_vu;

	if(Trigger.enservice) {
		nb = 0;
		for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue && (VoieTampon[voie].trig.trgr->type != NEANT)) nb++;
		lngr = 
		printf(" ____________________________________________________________________________________\n");
		printf("|                               Recherche d'evenements                               |\n");
		printf("|____________________________________________________________________________________|\n");
		k = printf("| Toutes les %d synchros D2 (soit %g ms).",
			(int)LectIntervTrmt,(float)LectIntervTrmt/SynchroD2_kHz); SambaCompleteLigne(lngr,k);
		k = printf("| Calcul des evenements %s par defaut.",TrmtLibelleListe[SettingsCalculEvt]); SambaCompleteLigne(lngr,k);
		k = printf("| Les evenements sont calcules %s%s sur les donnees %s.",TrmtImmediat? "des leur declenchement": "quand ils sont termines",
			(VoiesLocalesNb > 1)? ",": ".",SettingsCalcPretrig? "du trigger":"brutes"); SambaCompleteLigne(lngr,k);
		if(VoiesLocalesNb > 1) {
		#ifdef CENTRAGE_INDEPENDANT
			if(Calage.prevu == CALAGE_PAR_PHYSIQUE) k = printf("|   mais les amplitudes sont mesurees au temps de la plus elevee des voies du MEME type.");
			else if(Calage.prevu == CALAGE_UNIQUE) {
				if(Calage.reference < ModelePhysNb)
					k = printf("|   mais les amplitudes sont mesurees au temps de la plus elevee des voies %s.",ModelePhys[Calage.reference].nom);
				else k = printf("|   mais les amplitudes sont mesurees au temps de la voie qui a declenche.");
			} else k = printf("|   et les amplitudes sont mesurees sur le temps du maximum local.");
			SambaCompleteLigne(lngr,k);
			if(Centrage.prevu == CALAGE_PAR_PHYSIQUE) k = printf("| Les enregistrements sont centres comme celui de la plus elevee des voies du MEME type.");
			else if(Centrage.prevu == CALAGE_UNIQUE) {
				if(Centrage.reference < ModelePhysNb)
					k = printf("| Les enregistrements sont centres comme celui de la plus elevee des voies %s.",ModelePhys[Centrage.reference].nom);
				else k = printf("| Les enregistrements sont centres comme celui de la voie qui a declenche.");
			} else k = printf("| Les enregistrements sont centres conformement a leur definition.");
			SambaCompleteLigne(lngr,k);
		#else
/*			if(SettingsCalageObj == CALAGE_ENREG) {
				if(Calage.prevu == CALAGE_PAR_PHYSIQUE) k = printf("| Les enregistrements sont centres comme celui de la plus elevee des voies du MEME type.");
				else if(Calage.prevu == CALAGE_UNIQUE) {
					if(Calage.reference < ModelePhysNb)
						k = printf("| Les enregistrements sont centres comme celui de la plus elevee des voies %s.",ModelePhys[Calage.reference].nom);
					else k = printf("| Les enregistrements sont centres comme celui de la voie qui a declenche.");
				} else k = printf("| Les enregistrements sont centres conformement a leur definition.");
			} else {
				if(Calage.prevu == CALAGE_PAR_PHYSIQUE) k = printf("|   mais les amplitudes sont mesurees au temps de la plus elevee des voies du MEME type.");
				else if(Calage.prevu == CALAGE_UNIQUE) {
					if(Calage.reference < ModelePhysNb)
						k = printf("|   mais les amplitudes sont mesurees au temps de la plus elevee des voies %s.",ModelePhys[Calage.reference].nom);
					else k = printf("|   mais les amplitudes sont mesurees au temps de la voie qui a declenche.");
				} else k = printf("|   et les amplitudes sont bien mesurees sur le temps du maximum local.");
			}
			SambaCompleteLigne(lngr,k); */
			if(Calage.prevu == CALAGE_UNIQUE) {
				if(SettingsCalageObj == CALAGE_AMPL) {
					if(Calage.reference < ModelePhysNb)
						k = printf("|   mais les amplitudes sont mesurees au temps de la plus elevee des voies %s,",ModelePhys[Calage.reference].nom);
					else k = printf("|   mais les amplitudes sont mesurees au temps de la voie qui a declenche,");
					SambaCompleteLigne(lngr,k);
					k = printf("|   et les enregistrements sont centres sur ce temps."); SambaCompleteLigne(lngr,k);
				} else /* if(SettingsCalageObj == CALAGE_ENREG) */ {
					if(Calage.reference < ModelePhysNb)
						k = printf("| Les enregistrements sont centres comme celui de la plus elevee des voies %s.",ModelePhys[Calage.reference].nom);
					else k = printf("| Les enregistrements sont centres comme celui de la voie qui a declenche.");
					SambaCompleteLigne(lngr,k);
				}
			} else if(Calage.prevu == CALAGE_PAR_PHYSIQUE) {
				if(SettingsCalageObj == CALAGE_AMPL) {
					k = printf("|   mais les amplitudes sont mesurees au temps de la plus elevee des voies du MEME type,"); SambaCompleteLigne(lngr,k);
					k = printf("|   et les enregistrements sont centres sur ce temps."); SambaCompleteLigne(lngr,k);
				} else /* if(SettingsCalageObj == CALAGE_ENREG) */ {
					k = printf("| Les enregistrements sont centres comme celui de la plus elevee des voies du MEME type.");
					SambaCompleteLigne(lngr,k);
				}
			} else { k = printf("| Les enregistrements sont centres conformement a leur definition."); SambaCompleteLigne(lngr,k); }
		#endif
		}
		k = printf("| Le controle de conformite a posteriori est %s.",SettingsConformite?"actif":"abandonne"); SambaCompleteLigne(lngr,k);
		k = printf("| Les coupures sur amplitude, duree et bruit %s utilisees.",SettingsCoupures?"sont":"ne sont pas"); SambaCompleteLigne(lngr,k);
		if(SettingsCoupures) {
			k = printf("| Il %s coupure, et %s veto",TrmtAccepteParDefaut? "n'y a pas de": "y a au moins une",VetoPossible? "y a au moins un": "n'y a pas de");
			SambaCompleteLigne(lngr,k);
		}
		if(SettingsTrigInOut) {
			int n_out,conn,v_in,n_in;
			n_out = 0;
			for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue && VoieTampon[voie].trig.trgr->conn.out) {
				k = printf("| La voie %s communique son trigger (apres %g ms)",VoieManip[voie].nom,VoieTampon[voie].trig.trgr->conn.delai);
				n_out++; n_in = 0;
				for(conn=0; conn<VoiesNb; conn++) if(VoieTampon[conn].lue && (conn != voie) && (TrgrEtat[conn].accepte == voie)) { n_in++; v_in = conn; }
				if(!n_in) k += printf(", MAIS aucune voie ne l'utilise");
				else if(n_in == 1) k += printf(" a la voie %s",VoieManip[v_in].nom);
				else for(conn=0; conn<VoiesNb; conn++) if(VoieTampon[conn].lue && (conn != voie) && (TrgrEtat[conn].accepte == voie))
					k += printf("| - a la voie %s",VoieManip[conn].nom);
				SambaCompleteLigne(lngr,k);
			}
			if(!n_out) { k = printf("| Aucune voie ne communique son trigger."); SambaCompleteLigne(lngr,k); }
		} else { k = printf("| Il n'y a pas de transmission de trigger."); SambaCompleteLigne(lngr,k); }
//		if(Trigger.demande && (BolosUtilises > 1)) {
			k = printf("| Sauvegarde des detecteurs %s",ArchModeTexte[ArchDetecMode]);
			if(BolosAssocies) k += printf(", %s leurs associes",ArchAssocMode? "avec":"sans");
			SambaCompleteLigne(lngr,k);
//		}
		k = printf("| Le declenchement est actuellement %s.",(Trigger.actif)? "actif": "suspendu"); SambaCompleteLigne(lngr,k);
#ifdef NTUPLES_ONLINE
		if(CalcNtpDemande) k = printf("| Jusqu'a %d ntuple%s de type '%s'.",Accord1s(EvtEspace->max),EvtEspace->nom);
		else k = printf("| Ntuples desactives (voir le fichier %s).",NomParms);
		SambaCompleteLigne(lngr,k);
#endif
		if(Trigger.type == TRGR_SCRPT) {
			strcat(strcpy(fichier_scrpt,PrefPath),Trigger.nom_scrpt);
			k = printf("| Conditions definies dans le fichier %s.",fichier_scrpt); SambaCompleteLigne(lngr,k);
			f = fopen(fichier_scrpt,"r");
			if(!f) { k = printf("| FICHIER ILLISIBLE. Recherche abandonnee."); SambaCompleteLigne(lngr,k); return(0); }
			k = printf("|    par l'expression:"); SambaCompleteLigne(lngr,k);
			lig = 1;
			do {
				if(!fgets(ligne,256,f)) break;	
				if(ligne[(int)strlen(ligne) - 1] == '\n') ligne[(int)strlen(ligne) - 1] = '\0';
				k = printf("|    %3d: %s",lig++,ligne); SambaCompleteLigne(lngr,k);
			} until(feof(f));
			fclose(f);
			for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
				if(VoieTampon[voie].trig.cherche == NEANT) continue;
				k = printf("|  Pour la voie %s, %s",VoieManip[voie].nom,
					(VoieTampon[voie].trig.cherche == TRGR_EXTREMA)? "Recherche des extremas":
					((VoieTampon[voie].trig.cherche == TRGR_NIVEAU)? "Surveillance du niveau":
					"Recherche non repertoriee"));
				SambaCompleteLigne(lngr,k);
				k = printf("|    Dimension evenement        : %g ms, soit %d points (pre-trigger a %g%%)",
					VoieManip[voie].def.evt.duree,VoieEvent[voie].lngr,VoieManip[voie].def.evt.pretrigger);
				SambaCompleteLigne(lngr,k);
				k = printf("|    Temps mort apres evenement : %g ms demandees, soit %d points (%g ms)",
					VoieManip[voie].def.evt.tempsmort,VoieTampon[voie].trig.inhibe,VoieTampon[voie].trig.inhibe*VoieEvent[voie].horloge);
				SambaCompleteLigne(lngr,k);
				k = printf("|    Temps mort evt>%-7d ADU : %g ms demandees, soit %d points",VoieManip[voie].def.trgr.alphaampl,
					VoieManip[voie].def.trgr.alphaduree,VoieTampon[voie].trig.sautlong);
				SambaCompleteLigne(lngr,k);
			}
		} else if(Trigger.type == TRGR_PERSO) {
			k = printf("| Par procedure particuliere..."); SambaCompleteLigne(lngr,k);
		} else if(Trigger.type == TRGR_CABLE) {
			if(VoiesLocalesNb > 0) {
				int mm,md,mf;
				printf("|____________________________________________________________________________________|\n");
				printf("|      voie          |   ms  | points | pretrig | susp | condition                   |\n");
				printf("|____________________|_______|________|_________|______|_____________________________|\n");
				mm = md = mf = 0;
				for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
					char tactique[8]; char c;
					if(VoieTampon[voie].trig.trgr->origine == TRGR_DEPORTE) { c = '*'; md++; }
					else if(VoieTampon[voie].trig.trgr->origine == TRGR_FPGA) { c = '#'; mf++; }
					else c = '\0';
					k = printf("| %18s |",VoieManip[voie].nom);
					k += printf(" %5.2f | %6d | %7.1f |", VoieManip[voie].def.evt.duree,VoieEvent[voie].lngr,VoieManip[voie].def.evt.pretrigger);
					switch(VoieTampon[voie].trig.trgr->reprise) {
						case TRGR_REPRISE_FIXE: sprintf(tactique,"%5d ",VoieTampon[voie].trig.inhibe); break;
						case TRGR_REPRISE_SEUIL: strcpy(tactique,"seuil"); break;
						case TRGR_REPRISE_RENOUV: strcpy(tactique,"report"); break;
					}
					k += printf("%6s|",tactique);
					if(c) k += printf("(%c)",c);
					if(VoieTampon[voie].moyen.calcule) { k += printf("(+)"); mm++; }
					selon_que(VoieTampon[voie].trig.trgr->type) {
					  vaut NEANT:
						k += printf("  --");
						break;
					  vaut TRGR_SEUIL:
						k += printf(" %-5s",TrgrTypeListe[(int)VoieTampon[voie].trig.trgr->type]);
						if(VoieTampon[voie].trig.signe <= 0) k += printf(" <%-5g",VoieTampon[voie].trig.trgr->maxampl);
						if(VoieTampon[voie].trig.signe >= 0) k += printf(" >%-5g",VoieTampon[voie].trig.trgr->minampl);
						break;
					  vaut TRGR_FRONT:
						if(VoieTampon[voie].trig.signe > 0) k += printf(" %-6s","montee");
						else if(VoieTampon[voie].trig.signe < 0) k += printf(" %-6s","desc.");
						else k += printf(" %-6s",TrgrTypeListe[(int)VoieTampon[voie].trig.trgr->type]);
						k += printf(">%-5g ",VoieTampon[voie].trig.trgr->minampl);
						k += printf(">%-5.3f ",VoieTampon[voie].trig.trgr->montee);
						break;
					  vaut TRGR_DERIVEE:
						k += printf(" %-5.5s",TrgrTypeListe[(int)VoieTampon[voie].trig.trgr->type]);
						if(VoieTampon[voie].trig.signe <= 0) k += printf(" <%-5g",VoieTampon[voie].trig.trgr->maxampl);
						if(VoieTampon[voie].trig.signe >= 0) k += printf(" >%-5g",VoieTampon[voie].trig.trgr->minampl);
						k += printf("/%-5.3f ",VoieTampon[voie].trig.trgr->montee);
						break;
					  vaut TRGR_ALEAT:
						k += printf("%s",TrgrTypeListe[(int)VoieTampon[voie].trig.trgr->type]);
						k += printf(" @%g Hz",VoieTampon[voie].trig.trgr->porte);
						// k += printf(" * %g %%",VoieTampon[voie].trig.trgr->fluct);
						break;
					  au_pire:
						k += printf(" indetermine");
						break;
					}
					SambaLigneTable(' ',k,lngr);
					k = printf("|                          Temps mort evt >%7d ADU |%5d",VoieManip[voie].def.trgr.alphaampl,VoieTampon[voie].trig.sautlong);
					SambaLigneTable(' ',k,lngr);
					if(VoieTampon[voie].trig.coupe) {
						char condition[8];
						if(VoieTampon[voie].trig.trgr->veto) strncpy(condition,"VETO",7); else strncpy(condition,L_("coupure"),7);
					#ifdef SANS_SELECTION
						if((VoieTampon[voie].trig.trgr->condition.overflow > 0.0) || (VoieTampon[voie].trig.trgr->condition.underflow > 0.0)) {
							k = printf("|                      %7s si ampl.  %s",condition,VoieTampon[voie].trig.trgr->condition.ampl_inv?"dans":"hors de");
							if(VoieTampon[voie].trig.trgr->condition.overflow <= 0.0)  k += printf(" >%g",VoieTampon[voie].trig.trgr->condition.underflow);
							else if(VoieTampon[voie].trig.trgr->condition.underflow <= 0.0)  k += printf(" <%g",VoieTampon[voie].trig.trgr->condition.overflow);
							else k += printf(" [%g, %g]",VoieTampon[voie].trig.trgr->condition.underflow,VoieTampon[voie].trig.trgr->condition.overflow);
							SambaLigneTable(' ',k,lngr);
							strcpy(condition,"ou");
						}
						k = printf("|                      %7s si montee %s [%.3f, %.3f] ms",
							condition,VoieTampon[voie].trig.trgr->condition.mont_inv?"dans":"hors de",
							VoieTampon[voie].trig.trgr->condition.montmin,VoieTampon[voie].trig.trgr->condition.montmax);
						SambaLigneTable(' ',k,lngr);
						k = printf("|                           ou si bruit > %g ADU",VoieTampon[voie].trig.trgr->condition.maxbruit);
						SambaLigneTable(' ',k,lngr);
					#else  /* !SANS_SELECTION */
						TypeCritereJeu *critere; int var; unsigned char ntuple;
						critere = &(VoieManip[voie].def.coup);
						for(var=0; var<critere->nbvars; var++) {
							ntuple = critere->var[var].ntuple;
							k = printf("|                      %7s si %-7s  %-7s",condition,NtpName[ntuple],critere->var[var].inverse?"dans":"hors de");
							k += printf(" [%g, %g]",critere->var[var].min,critere->var[var].max);
							SambaLigneTable(' ',k,lngr);
							strcpy(condition,"ou");
						}
					#endif /* !SANS_SELECTION */
					} else if((VoieTampon[voie].trig.trgr->type != NEANT) && !TrmtAccepteParDefaut && !SettingsConformite) {
						k = printf("|                      controle de conformite a posteriori: retabli"); SambaCompleteLigne(lngr,k);
					}
				}
				k = printf("|____________________|_______|________|_________|______|"); SambaLigneTable('_',k,lngr);
				if(mm || md || mf) {
					if(md) { k = printf("| (*) logique du trigger deportee pour %s",(md<2)? "cette voie":"ces voies"); SambaLigneTable(' ',k,lngr); }
					if(mf) { k = printf("| (#) logique du trigger cablee   pour %s",(mf<2)? "cette voie":"ces voies"); SambaLigneTable(' ',k,lngr); }
					if(mm) { k = printf("| (+) calcul de l'evenement moyen pour %s",(mm<2)? "cette voie":"ces voies"); SambaLigneTable(' ',k,lngr); }
					SambaLigneTable('_',0,lngr);
				}
			} else {
				printf("| SAUF qu'il n'y a PAS DE VOIE LUE LOCALEMENT !!"); SambaLigneTable(' ',k,lngr);
				SambaLigneTable('_',0,lngr);
			}
		}
		if(TrmtRegulActive) {
			k = printf("| La regulation generale des seuils de declenchement est activee"); SambaLigneTable(' ',k,lngr);
			for(vm=0; vm<VOIES_TYPES; vm++) std[vm] = 0; std_vu = 0;
			for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
				if(VoieTampon[voie].regul_active == TRMT_REGUL_TAUX) {
					/*
					 k = printf("| Regulation du taux d'evenements par ajustement dynamique des seuils:");
					 SambaCompleteLigne(lngr,k);
					 for(j=0; j<MAXECHELLES; j++) if(TrmtRegulTaux[j].active) {
						 ArgKeyGetText(TrmtRegulCles,TrmtRegulTaux[j].min.action,action,10);
						 k = printf("| %1d/ sur %3d mn, si evts <%3d: %9s",j+1,
									TrmtRegulTaux[j].duree,TrmtRegulTaux[j].min.nb,action);
						 if(TrmtRegulTaux[j].min.action) k += printf(" de %3g",TrmtRegulTaux[j].min.parm);
						 ArgKeyGetText(TrmtRegulCles,TrmtRegulTaux[j].max.action,action,10);
						 k += printf("; si evts >%4d: %9s",TrmtRegulTaux[j].max.nb,action);
						 if(TrmtRegulTaux[j].max.action) k += printf(" de %+g",TrmtRegulTaux[j].max.parm);
						 SambaCompleteLigne(lngr,k);
					 }
					 */
					k = printf("| %-16s: Regulation du taux par ajustement %s des seuils",VoieManip[voie].nom,VoieManip[voie].def.rgul.std? "standard": "particulier");
					SambaCompleteLigne(lngr,k);
					if(!VoieManip[voie].def.rgul.std) {
						for(j=0; j<MAXECHELLES; j++) if(VoieManip[voie].echelle[j].active) {
							char qqchose;
							k = printf("|   %1d/ sur %3d mn : ",j+1,VoieManip[voie].echelle[j].duree); qqchose = 0;
							if(VoieManip[voie].echelle[j].min.action) {
								ArgKeyGetText(TrmtRegulCles,VoieManip[voie].echelle[j].min.action,action,16);
								l = strlen(action); while(l--) if(action[l] == '_') action[l] = ' ';
								k += printf("si evts <%3d, %12s %-3g",VoieManip[voie].echelle[j].min.nb,action,VoieManip[voie].echelle[j].min.parm);
								qqchose = 1;
							}
							if(VoieManip[voie].echelle[j].max.action) {
								if(qqchose) k += printf("; ");
								ArgKeyGetText(TrmtRegulCles,VoieManip[voie].echelle[j].max.action,action,16);
								l = strlen(action); while(l--) if(action[l] == '_') action[l] = ' ';
								k += printf("si evts >%4d, %12s %+g",VoieManip[voie].echelle[j].max.nb,action,VoieManip[voie].echelle[j].max.parm);
								qqchose = 1;
							}
							if(!qqchose) k += printf("pas de variation des seuils");
							SambaCompleteLigne(lngr,k);
						}
						k = printf("|                   limites des seuils: ");
						if(VoieTampon[voie].trig.signe <= 0) {
							k += printf("[%g .. %g]",VoieManip[voie].def.rgul.maxsup,VoieManip[voie].def.rgul.maxinf);
						}
						if(VoieTampon[voie].trig.signe >= 0) {
							if(VoieTampon[voie].trig.signe == 0) k += printf(", ");
							k += printf("[%g .. %g]",VoieManip[voie].def.rgul.mininf,VoieManip[voie].def.rgul.minsup);
						}
						SambaCompleteLigne(lngr,k);
					} else { std[VoieManip[voie].type] = 1; std_vu = 1; }
				} else if(VoieTampon[voie].regul_active == TRMT_REGUL_INTERV) {
					/*
					 k = printf("| Ajustement dynamique des seuils par evaluation du maximum du bruit LdB");
					 SambaCompleteLigne(lngr,k);
					 */
					k = printf("| %-16s: Ajustement des seuils par evaluation du bruit LdB",VoieManip[voie].nom);
					SambaCompleteLigne(lngr,k);
					k = printf("|      La limitation de l'ajustement n'est pas implementee");
					SambaCompleteLigne(lngr,k);
				} else {
					/*
					 k = printf("| Les seuils sont fixes"); SambaCompleteLigne(lngr,k);
					 */
					k = printf("| %-16s: Les seuils sont fixes",VoieManip[voie].nom);
					SambaCompleteLigne(lngr,k);
				}
			}
			if(std_vu) {
				for(vm=0; vm<ModeleVoieNb; vm++) if(std[vm]) {
					k = printf("| Regulation standard %s:",ModeleVoie[vm].nom); SambaCompleteLigne(lngr,k);
					for(j=0; j<MAXECHELLES; j++) if(VoieStandard[vm].echelle[j].active) {
						char qqchose;
						k = printf("|   %1d/ sur %3d mn : ",j+1,VoieStandard[vm].echelle[j].duree); qqchose = 0;
						if(VoieStandard[vm].echelle[j].min.action) {
							ArgKeyGetText(TrmtRegulCles,VoieStandard[vm].echelle[j].min.action,action,16);
							l = strlen(action); while(l--) if(action[l] == '_') action[l] = ' ';
							k += printf("si evts <%3d, %12s %-3g",VoieStandard[vm].echelle[j].min.nb,action,VoieStandard[vm].echelle[j].min.parm);
							qqchose = 1;
						}
						if(VoieStandard[vm].echelle[j].max.action) {
							if(qqchose) k += printf("; ");
							ArgKeyGetText(TrmtRegulCles,VoieStandard[vm].echelle[j].max.action,action,16);
							l = strlen(action); while(l--) if(action[l] == '_') action[l] = ' ';
							k += printf("si evts >%4d, %12s %+g",VoieStandard[vm].echelle[j].max.nb,action,VoieStandard[vm].echelle[j].max.parm);
							qqchose = 1;
						}
						if(!qqchose) k += printf("pas de variation des seuils");
						SambaCompleteLigne(lngr,k);
					}
					k = printf("|                   limites des seuils: ");
					if(VoieStandard[vm].def.trgr.sens <= 1) {
						k += printf("[%g .. %g]",VoieStandard[vm].def.rgul.maxsup,VoieStandard[vm].def.rgul.maxinf);
					}
					if(VoieStandard[vm].def.trgr.sens >= 1) {
						if(VoieStandard[vm].def.trgr.sens == 1) k += printf(", ");
						k += printf("[%g .. %g]",VoieStandard[vm].def.rgul.mininf,VoieStandard[vm].def.rgul.minsup);
					}
					SambaCompleteLigne(lngr,k);
				}
			}
		} else printf("| Pas de regulation des seuils de declenchement                                      |\n");
		printf("|____________________________________________________________________________________|\n");
	} else {
		if(LectCntl.LectMode != LECT_COMP) {
			printf(" ____________________________________________________________________________________\n");
			printf("| Pas de recherche d'evenements                                                      |\n");
			printf("|____________________________________________________________________________________|\n\n");
		}
	}
	return(0);
}
/* ========================================================================== */
static void TrmtRAZCateg(TypeMonitCatg *categ) {
	categ->stamp_trouve = categ->stamp_utilise = COMPTEUR_0;
	categ->affichable = categ->affiche =0;
	categ->trouves = categ->utilises = 0;
	categ->delai = 0.0;
	categ->freq = 0.0;
}
/* ========================================================================== */
static void TrmtCategStore(TypeMonitCatg *categ, int n, int min, int max, float top,
	float charge, float duree, float prelim, float avance) {
	categ->affichable = Evt[n].num;
	categ->delai = (float)(Evt[n].stamp_ref - categ->stamp_trouve) * EnSecondes;
	categ->stamp_trouve = Evt[n].stamp_ref;
	categ->top = top;
	categ->charge = charge; categ->duree = duree;
	categ->prelim = prelim; categ->avance = avance;
	categ->trouves++; /* idem Acquis[AcquisLocale].etat.evt_trouves */
	if(categ->autoscale) {
		if(categ->scalechge) {
			if(min < categ->min) categ->min = min;
			if(max > categ->max) categ->max = max;
		} else {
			categ->scalechge = 1; categ->min = min; categ->max = max;
			GraphAxisAutoRange(categ->gEvt,GRF_YAXIS);
		}
	} else if(categ->scalechge) {
		categ->scalechge = 0;
		GraphAxisIntRange(categ->gEvt,GRF_YAXIS,categ->min,categ->max);
	}
}
/* ========================================================================== */
int TrmtRAZ(char log) {
	int voie,gene,catg; int i,n;
	char nom[CEB_MAXNOM],fichier_scrpt[256];
	FILE *f; CebInstr *suite;
	char nom_complet[MAXFILENAME]; TypeAudioWaveHdr entete;
	// char calage_impose,centrage_impose;
	char rc;
	float duree;

	if(LectCntl.LectMode == LECT_IDENT) { TrmtIdentPasFaite = 1; return(1); }
//	TrmtDebug = 1;
	TagPlayEnd = 0;
	TrmtPanicTotal = 0; TrmtPanicNb = 0;
	TrmtMesureFineActive = 0; TrmtMesureFineTerminee = 0;
	TrmtAppelsNb = 0;
	TrmtVoieInitiale = 0;
	TempsTotalTrmt = 0;
	TrmtHeureDernier = VerifTempsPasse? DateMicroSecs(): 0;
	DerniereVoieSignalee = -1;
	TrmtPasActif = 0;
	TrmtTempsEvt = 0;
	DernierPointTraite = COMPTEUR_0;
	TrmtRAZCateg(&TousLesEvts);
	for(catg=0; catg<MonitCatgNb; catg++) TrmtRAZCateg(&(MonitCatg[catg]));
	if(!SettingsDeconvPossible) for(n=0; n<EvtNbMax; n++) if(Evt[n].deconvolue) { Evt[n].deconv_max = 0; Evt[n].deconvolue = 0; }
    Acquis[AcquisLocale].etat.evt_recus = Acquis[AcquisLocale].etat.evt_trouves = 0;
	LectCntl.MonitEvtNum = 0;
    LectCntl.TauxGlobal = 0.0;
	TrmtNbLus = 0;
	TrmtTropTard = 0;
	TrmtModePanique = 0; TrmtApanique = 0;
	TrmtAbandonne = 0;
	EvtsEmpiles = 0;
	EvtSupprimes = 0;
	TrmtEvtJetes = 0;
//	EvtATraiter->trig = -1;
//	EvtATraiter->fin = -1; /* sinon, TrmtCandidatSignale peut empiler indument avec EvtATraiter->trig=-1
//		et l'evt svt est directement traite avant celui de la pile... [pb du trou dans l'histo des delais] */
	Trigger.analyse = DernierPointTraite = TousLesEvts.stamp_trouve;
	PileUp = 0;
#ifdef BCP_EVTS
	CalcNb = 0;
#endif
	LectDebutEvts = -1;
	EvtPrecedent.num = -1;
	ArchiveNettoie(0,0,__func__); /* entraine EvtASauver = EvtAffichable = 0 */
	NtUsed = 0;

	Calage.prevu = ((VoiesLocalesNb > 1) && !LectReglagesOuverts)? SettingsCalageMode: 0;
#ifdef CENTRAGE_INDEPENDANT
	Centrage.prevu = (VoiesLocalesNb > 1)? SettingsCentrageType: 0;
#endif

	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
		if(SettingsDeconvPossible && VoieManip[voie].duree.deconv.calcule && (VoieManip[voie].duree.deconv.rc <= 0.0)) {
			OpiumFail(L_("La voie %s ne peut pas etre deconvoluee: temps de descente invalide (%g)"),VoieManip[voie].nom,VoieManip[voie].duree.deconv.rc);
			return(0);
		}
		/* nombres de points absolus */
		VoieTampon[voie].signal.analyse = VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien;
		VoieTampon[voie].norme_evt = TemplatesNorme(voie);
	#ifdef UTILISE_EN_DEMARRAGE
		TrgrEtat[voie].en_demarrage = 1;
	#endif
		if(Calage.prevu) VoieTampon[voie].trig.calcule = TRMT_AMPLMAXI;
		VoieTampon[voie].trig.cherche = NEANT;
		TrmtRAZvoie(voie); /* pour Voie.avant_evt dans l'archivage */
        if(VoieTampon[voie].trig.trgr->type == TRGR_ALEAT) {
            if(VoieTampon[voie].trig.trgr->porte > 0.0)
                VoieTampon[voie].signal.periode = Echantillonnage * 1000.0 / VoieTampon[voie].trig.trgr->porte;
            else VoieTampon[voie].signal.periode = Echantillonnage * 3000.0; // 1/3 Hz
            VoieTampon[voie].signal.futur_evt = (int64)(VoieTampon[voie].signal.periode); // 400000, mais pourquoi??
            // printf("(%s) Evenement aleatoire sur la voie %s: toutes les %g Msamples (1er @%lld)\n",__func__,VoieManip[voie].nom,VoieTampon[voie].signal.periode/1.0e6,VoieTampon[voie].signal.futur_evt);
        }
		if(!VoieTampon[voie].trig.special) {
			VoieTampon[voie].trig.coupe = SettingsCoupures? VoieTampon[voie].trig.trgr->coupures: 0;
			if((VoieTampon[voie].trig.trgr->type == NEANT) && (VoieTampon[voie].trig.coupe != TRGR_COUPE_TJRS)) VoieTampon[voie].trig.coupe = 0;
			VoieTampon[voie].trig.conformite = (VoieTampon[voie].trig.trgr->type != NEANT)? SettingsConformite: 0;
		} else {
			VoieTampon[voie].trig.trgr->origine = TRGR_INTERNE;
			VoieTampon[voie].trig.coupe = TRGR_COUPE_SANS;
			VoieTampon[voie].trig.conformite = 0;
		}
	} else {
		VoieTampon[voie].trig.coupe = TRGR_COUPE_SANS; VoieTampon[voie].trig.conformite = 0;
	}
	for(gene=0; gene<GeneNb; gene++) Gene[gene].mode_manu = 0;
	TrmtCategIndefinie = GRF_POINT;
	for(n=0; n<TagDefNb; n++) {
		if(TagDef[n].p == GRF_POINT) TrmtCategIndefinie = GRF_ABSENT;
		if((TagDef[n].sonorisee = MonitTagsSonorisee(&(TagDef[n])))) {
			strcat(strcpy(nom_complet,PrefPath),TagDef[n].son);
			if((f = AudioWaveRead(nom_complet,&entete))) {
				TagDef[n].duree_us = (int64)(AudioWaveDuree(&entete) * 1000000.0);
				fclose(f);
			} else { TagDef[n].sonorisee = 0; TagDef[n].duree_us = 200000.0; } // 0.2 s
			printf("(%s) Duree de %s: %.3f s\n",__func__,TagDef[n].son,(float)(TagDef[n].duree_us) / 1000000.0);
		}
	}

	TrmtAccepteParDefaut = 1; TrmtNonConformeParDefaut = VetoPossible = 0;
	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
		if(VoieTampon[voie].trig.coupe) {
			TrmtAccepteParDefaut = 0; if(VoieTampon[voie].trig.trgr->veto) VetoPossible = 1;
		}
		if((VoieTampon[voie].trig.trgr->type != NEANT) && VoieTampon[voie].trig.conformite) TrmtNonConformeParDefaut = 1;
	}
	VetoMis = 0;

#ifdef HISTO_TIMING_TRMT
	for(voie=0; voie<MAXHISTOTIMING; voie++) HistoTiming[voie] = 0; /* ici, voie n'est pas une voie */
#endif
	TrmtDernierTrig = 0;
	EvtSignales = TrmtNbNouveaux = TrmtNbEchecs = TrmtNbPremat = TrmtNbRelances = 0;
#ifdef HISTOS_DELAIS_EVT
	for(voie=0; voie<MAXHISTOTRIG; voie++) HistoTrig[voie] = HistoEvts[voie] = 0; /* ici, voie n'est pas une voie */
#endif
	if(Trigger.enservice) {
		n = (((BitTriggerNiveau? VoiesNb: BoloNb) - 1) / 32) + 1;
		if(n > EVT_BITTRIGGER_MAX) n = EVT_BITTRIGGER_MAX;
		if(n != BitTriggerDim) {
			if(ArchTriggerInfo) { free(ArchTriggerInfo); ArchTriggerInfo = 0; BitTriggerDim = 0; }
			if(n) {
				ArgDesc *elt;
				BitTriggerDim = n;
				// printf("(%s) %d %s soit %d int32\n",__func__,BitTriggerNiveau? VoiesNb: BoloNb,BitTriggerNiveau? "voies": "bolos",BitTriggerDim);
				ArchTriggerInfo = ArgCreate(BitTriggerDim);
				elt = ArchTriggerInfo;
				for(i=0; i<BitTriggerDim; i++) {
					sprintf(&(BitTriggerNom[i][0]),"Liste:%02d-%02d",((i+1)*32)-1,i*32);
					sprintf(&(BitTriggerCom[i][0]),"1 bit par %s %s (%02d..%02d)",
						BitTriggerNiveau? "voie touchee": "detecteur touche",
						BitTriggerNums? "locale": "globale",((i+1)*32)-1,i*32);
					ArgAdd(elt++,&(BitTriggerNom[i][0]),DESC_INT &ArchEvt.liste[i],&(BitTriggerCom[i][0]));
				}
			}
		}
	
		// if((calage_impose = (SettingsCalageRef < ModelePhysNb))) Calage.reference = SettingsCalageRef; else /* a tout hasard */ Calage.reference = 0;
		// if((centrage_impose = (SettingsCentrageRef < ModelePhysNb))) Centrage.reference = SettingsCentrageRef; else /* a tout hasard */ Centrage.reference = 0;
		// if(!calage_impose || !centrage_impose)  {
		//	duree = 1.0e9; /* ms */
		//	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue && (VoieTampon[voie].trig.trgr->type != NEANT) && (VoieManip[voie].def.evt.duree < duree)) {
		//		duree = VoieManip[voie].def.evt.duree;
		//		if(!calage_impose) Calage.reference = ModeleVoie[VoieManip[voie].type].physique;
		//		if(!centrage_impose) Centrage.reference = ModeleVoie[VoieManip[voie].type].physique;
		//	}
		//}
		// SettingsCalageRef=0: "auto"; =ModelePhysNb+1: "triggee"
		Calage.reference = SettingsCalageRef - 1; /* a tout hasard */ /* soit [-1..ModelePhysNb] */
		if(!SettingsCalageRef) /* auto demande */ {
			duree = 1.0e9; /* ms */
			for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue && (VoieTampon[voie].trig.trgr->type != NEANT) && (VoieManip[voie].def.evt.duree < duree)) {
				duree = VoieManip[voie].def.evt.duree;
				Calage.reference = ModeleVoie[VoieManip[voie].type].physique;
			}
		}
	#ifdef CENTRAGE_INDEPENDANT
		Centrage.reference = SettingsCentrageRef - 1; /* a tout hasard */
		if(!SettingsCentrageRef)  {
			duree = 1.0e9; /* ms */
			for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue && (VoieTampon[voie].trig.trgr->type != NEANT) && (VoieManip[voie].def.evt.duree < duree)) {
				duree = VoieManip[voie].def.evt.duree;
				Centrage.reference = ModeleVoie[VoieManip[voie].type].physique;
			}
		}
	#endif

		if(Trigger.type == TRGR_SCRPT) {
			size_t l;
			CebInit(TrmtPrgm);
			TrmtVar = CebVarCreateLocale("Trigger",0.0);
			TrgrResultat = CebVarRefAdrs(TrmtVar);
			for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
				sprintf(nom,"Montee_%s",VoieManip[voie].nom); for(l=strlen(nom)-1; l; --l) if(nom[l] == ' ') nom[l] = '_';
				VoieTampon[voie].var_montee = CebVarCreateLocale(nom,0.0);
				VoieTampon[voie].adrs_montee = CebVarRefAdrs(VoieTampon[voie].var_montee);
				sprintf(nom,"Amplitude_%s",VoieManip[voie].nom); for(l=strlen(nom)-1; l; --l) if(nom[l] == ' ') nom[l] = '_';
				VoieTampon[voie].var_amplitude = CebVarCreateLocale(nom,0.0);
				VoieTampon[voie].adrs_amplitude = CebVarRefAdrs(VoieTampon[voie].var_amplitude);
				sprintf(nom,"Niveau_%s",VoieManip[voie].nom); for(l=strlen(nom)-1; l; --l) if(nom[l] == ' ') nom[l] = '_';
				VoieTampon[voie].var_niveau = CebVarCreateLocale(nom,0.0);
				VoieTampon[voie].adrs_niveau = CebVarRefAdrs(VoieTampon[voie].var_niveau);
				sprintf(nom,"Duree_%s",VoieManip[voie].nom); for(l=strlen(nom)-1; l; --l) if(nom[l] == ' ') nom[l] = '_';
				VoieTampon[voie].var_duree = CebVarCreateLocale(nom,0.0);
				VoieTampon[voie].adrs_duree = CebVarRefAdrs(VoieTampon[voie].var_duree);
			}
			strcat(strcpy(fichier_scrpt,PrefPath),Trigger.nom_scrpt);
			f = fopen(fichier_scrpt,"r");
			rc = CebCompile(f,TrmtPrgm,&suite);
			if(!rc) {
				printf("! Erreur a la compilation: %s\n! Fichier compile: %s\n",CebErrorText(),fichier_scrpt);
				OpiumFail(CebErrorText());
				rewind(f);
				CebAnalyse(f,TrmtPrgm,&suite);
			}
			fclose(f);
/*			printf("  Expression compilee:\n");
			CebPrgmList(TrmtPrgm);
			printf("  Liste des variables:\n");
			CebVarList(); */
			for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
				if((CebVarRefEtat(VoieTampon[voie].var_montee) == CEB_REF)
				|| (CebVarRefEtat(VoieTampon[voie].var_amplitude) == CEB_REF)
				|| (CebVarRefEtat(VoieTampon[voie].var_duree) == CEB_REF)) {
					VoieTampon[voie].trig.cherche = TRGR_EXTREMA;
				} else if(CebVarRefEtat(VoieTampon[voie].var_niveau) == CEB_REF) {
					VoieTampon[voie].trig.cherche = TRGR_NIVEAU;
				}
			}
			if(!rc) return(0);
		} else if(Trigger.type == TRGR_PERSO) {
			if(!TrmtPersoRAZ()) return(0);
		} else if(Trigger.type == TRGR_CABLE) {
        #ifdef ANCIENNE_PRESENTATION
			char autre_voie; autre_voie = 0;
        #endif /* ANCIENNE_PRESENTATION */
			TrmtAttendComplet = 0;
			for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue && (VoieTampon[voie].trig.trgr->type != NEANT) && (VoieTampon[voie].trig.trgr->type != TRGR_ALEAT)) {
				if((VoieTampon[voie].trig.calcule > TRMT_LDB) || VoieTampon[voie].moyen.calcule) TrmtAttendComplet = 1;
				switch(VoieTampon[voie].trig.trgr->type) {
				  case TRGR_TEST: case TRGR_FRONT: case TRGR_PORTE: VoieTampon[voie].trig.cherche = TRGR_EXTREMA; break;
				  case TRGR_DERIVEE: case TRGR_SEUIL: VoieTampon[voie].trig.cherche = TRGR_NIVEAU; break;
				  default:
                #ifndef VISU_FONCTION_TRANSFERT
					OpiumFail("Trigger de type %s inactive. Pas de trigger sur %s",
						TrgrTypeListe[(int)VoieTampon[voie].trig.trgr->type],VoieManip[voie].nom);
					VoieTampon[voie].trig.trgr->type = NEANT;
                #endif /* VISU_FONCTION_TRANSFERT */
				}
				if(VoieTampon[voie].trig.cherche == NEANT) {
					printf("! ERREUR SYSTEME, Trigger sur la voie %s = %d (imprevu)\n",VoieManip[voie].nom,VoieTampon[voie].trig.trgr->type);
					return(0);
				}
                
            #ifdef ANCIENNE_PRESENTATION
				printf("  %s %s %s sur la voie %s\n",autre_voie? "OU BIEN par": "Par",
					TrgrTypeListe[(int)VoieTampon[voie].trig.trgr->type],
					VoieTampon[voie].trig.signe? ((VoieTampon[voie].trig.signe < 0)? "negatif": "positif"):"indifferent",
					VoieManip[voie].nom);
				printf("    %s,\n",
					(VoieTampon[voie].trig.cherche == TRGR_EXTREMA)? "Recherche des extremas":
					((VoieTampon[voie].trig.cherche == TRGR_NIVEAU)? "Surveillance du niveau":
					"Recherche non repertoriee"));
				switch(VoieTampon[voie].trig.trgr->type) {
				  case TRGR_SEUIL:
					if(VoieTampon[voie].trig.signe >= 0)
						printf("    Amplitude minimum          : %g ADU\n",VoieTampon[voie].trig.trgr->minampl);
					if(VoieTampon[voie].trig.signe <= 0)
						printf("    Amplitude maximum          : %g ADU\n",VoieTampon[voie].trig.trgr->maxampl);
					break;
				  case TRGR_FRONT:
					printf("    Amplitude minimum          : %g ADU\n",VoieTampon[voie].trig.trgr->minampl);
					printf("    Temps de montee minimum    : %g 탎, soit %d points de %g 탎\n",
						VoieTampon[voie].trig.trgr->montee,VoieTampon[voie].trig.mincount,(VoieEvent[voie].horloge * 1000));
					printf("    Temps de montee maximum    : %g 탎, soit %d points de %g 탎\n",
						VoieTampon[voie].trig.trgr->condition.montmax,(int)(VoieTampon[voie].trig.trgr->condition.montmax / VoieEvent[voie].horloge),(VoieEvent[voie].horloge * 1000));
					break;
				  case TRGR_PORTE:
					printf("    Amplitude minimum          : %g ADU\n",VoieTampon[voie].trig.trgr->minampl);
					printf("    Duree minimum              : %g 탎, soit %d points de %g 탎\n",
						VoieTampon[voie].trig.trgr->porte,VoieTampon[voie].trig.mindelai,(VoieEvent[voie].horloge * 1000));
					break;
				  case TRGR_DERIVEE:
					if(VoieTampon[voie].trig.signe >= 0)
						printf("    Amplitude minimum          : %g ADU\n",VoieTampon[voie].trig.trgr->minampl);
					if(VoieTampon[voie].trig.signe <= 0)
						printf("    Amplitude maximum          : %g ADU\n",VoieTampon[voie].trig.trgr->maxampl);
					printf("    Duree de montee            : %g 탎, soit %d points de %g 탎\n",
						VoieTampon[voie].trig.trgr->montee,VoieTampon[voie].trig.mincount,(VoieEvent[voie].horloge * 1000));
					break;
				  case TRGR_TEST:
                #ifdef VISU_FONCTION_TRANSFERT
					if(Hnb != VoieEvent[voie].lngr) {
						nb = (VoieEvent[voie].lngr / 2) + 1;
						if(Xfiltre) free(Xfiltre);
						Xfiltre = (float *)malloc(nb * sizeof(float));
						if(Yfiltre) free(Yfiltre);
						Yfiltre = (float *)malloc(nb * sizeof(float));
						if(Hfiltre) free(Hfiltre);
						Hfiltre = (float *)malloc(nb * sizeof(float));
						if(Hfiltre) {
							Hnb = VoieEvent[voie].lngr;
							GraphErase(gTrmtFiltre);
							GraphAdd(gTrmtFiltre,GRF_FLOAT|GRF_INDEX|GRF_XAXIS,TrmtFx,nb);
							GraphAdd(gTrmtFiltre,GRF_FLOAT|GRF_YAXIS,Xfiltre,nb);
							GraphDataRGB(gTrmtFiltre,1,GRF_RGB_YELLOW);
							GraphAdd(gTrmtFiltre,GRF_FLOAT|GRF_YAXIS,Yfiltre,nb);
							GraphDataRGB(gTrmtFiltre,2,GRF_RGB_GREEN);
							GraphAdd(gTrmtFiltre,GRF_FLOAT|GRF_YAXIS,Hfiltre,nb);
							GraphAxisTitle(gTrmtFiltre,GRF_XAXIS,"Frequence (kHz)");
							GraphAxisTitle(gTrmtFiltre,GRF_YAXIS,"Filtre");
							OpiumTitle(gTrmtFiltre->cdr,"Fonction de transfert");
							OpiumPosition(gTrmtFiltre->cdr,30,500);
							GraphParmsSave(gTrmtFiltre);
							TrmtFx[0] = TrmtFx[1] = 1.0 / (VoieEvent[voie].horloge * (float)Hnb); /* kHz */
						} else {
							if(Xfiltre) { free(Xfiltre); Xfiltre = 0; }
							if(Yfiltre) { free(Yfiltre); Yfiltre = 0; }
							Hnb = 0;
						}
					}
					printf("    Calcul du filtre sur %d points\n",Hnb);
                #endif /* VISU_FONCTION_TRANSFERT */
					break;
				}
				printf("    Dimension evenement        : %g ms, soit %d points (pre-trigger a %g%%)\n",
					   VoieManip[voie].def.evt.duree,VoieEvent[voie].lngr,VoieManip[voie].def.evt.pretrigger);
				printf("    Temps mort apres evenement : %g ms demandees, soit %d points (%g ms)\n",
					   VoieManip[voie].def.evt.tempsmort,VoieTampon[voie].trig.inhibe,VoieTampon[voie].trig.inhibe*VoieEvent[voie].horloge);
				autre_voie = 1;
				if(VoieTampon[voie].moyen.calcule) printf("    Un evenement moyen est calcule\n");
            #endif /* ANCIENNE_PRESENTATION */
			}
			TrmtImmediat = (!TrmtAttendComplet && (SambaMode == SAMBA_MONO));
		}
			
		/* On efface le trace du dernier evenement */
		if(MonitAffEvts && gEvtSolo && OpiumDisplayed(gEvtSolo->cdr)) OpiumClear(gEvtSolo->cdr);

		/* Histo 'amplitude' */
		if(TrmtHampl) {
			HistoDataRaz(hdBruitAmpl);
			HistoDataRaz(hdEvtAmpl);
		}
		/* Histo 'temps de montee' */
		if(TrmtHmontee) {
			HistoDataRaz(hdBruitMontee);
			HistoDataRaz(hdEvtMontee);
		}
		/* Histo 'amplitude' x 'temps de montee' */
		if(TrmtH2D) H2DRaz(h2D);
		MonitHistosDynamique();

	} else {
		if(gEvtRunCtrl) { if(OpiumDisplayed(gEvtRunCtrl->cdr)) GraphUse(gEvtRunCtrl,0); }
	}

	TrmtGenere = 0;
	TrmtDejaDit = 0;
	TrmtDataNb = 1000;

#ifdef ARCH_DEBUG
	ArchEvt2emeLot = 1000000;
#endif

	i = 0;
	for(voie=0; voie<VoiesNb; voie++) {
		VoieTampon[voie].cree_traitee = (VoieTampon[voie].lue &&
            ((VoieTampon[voie].trig.cherche != NEANT) || (VoieTampon[voie].trig.trgr->type == TRGR_ALEAT) || VoieTampon[voie].post));
		if(VoieTampon[voie].cree_traitee && VoieTampon[voie].traitees->t.rval
		&& ((VoieTampon[voie].trig.calcule == TRMT_EVTMOYEN) || (VoieManip[voie].def.trmt[TRMT_PRETRG].template != NEANT)) && !LectModeSpectresAuto) {
			if(log) i++;
			TemplatesFiltre(voie,i);
		}
 	}
	if(i) printf("|__________________________|_________________________________________________________|\n");
	i = 0;
	if(1) { // log
		for(voie=0; voie<VoiesNb; voie++) {
			if(VoieTampon[voie].lue && !VoieTampon[voie].cree_traitee) {
				int l;
				if(!i) {
					printf("\n");
					printf(" ________________________________ \n");
					printf("| Voies sans stockage pre-triger |\n");
					printf("|________________________________|\n");
					i++;
				}
				l = printf("| %s",VoieManip[voie].nom); SambaCompleteLigne(34,l);
			}
		}
		if(i) printf("|________________________________|\n\n");
		TrmtImprimeTrgr();
	}

	return(1);
}
#define DEBUG_MODUL
/* ========================================================================== */
INLINE static void TrmtInhibeVoie(int voie) {
	if(voie < 0) return;
	if((VoieTampon[voie].lue > 0) && (VoieManip[voie].def.trmt[TRMT_PRETRG].type != TRMT_INVALID)) {
		if((VoieManip[voie].def.trgr.reprise != TRGR_REPRISE_SEUIL) || (VoieTampon[voie].trig.cherche != TRGR_NIVEAU) || !(TrgrEtat[voie].evt_en_cours)) {
			TrgrEtat[voie].debut_recherche = TrgrEtat[voie].point_traite + VoieTampon[voie].trig.inhibe;
			LectStampSautes += VoieTampon[voie].trig.inhibe;
			if(VoieManip[voie].def.trgr.reprise == TRGR_REPRISE_RENOUV) TrgrEtat[voie].reprise = 1;
		}
		VoieTampon[voie].suivi.date_evt = 0;
	}
	if(VerifSuiviTrmtEvts) {
		if(voie == DerniereVoieSignalee) printf("(%s) Date[%s]=%lld\n",__func__,VoieManip[voie].nom,VoieTampon[voie].suivi.date_evt);
	}
}
/* ========================================================================== */
INLINE static void TrmtInhibeDetecteur(int bolo) {
	int cap;
	if(Bolo[bolo].lu == DETEC_OK) for(cap=0; cap<Bolo[bolo].nbcapt; cap++) TrmtInhibeVoie(Bolo[bolo].captr[cap].voie);
}
/* ========================================================================== */
static void TrmtInhibeAutresVoies(int voietrig) {
	int bolotrig,bolo,bv,ba,voie; int k,mode; short tour;

	bolotrig = VoieManip[voietrig].det;
	mode = (Bolo[bolotrig].mode_arch != ARCH_NBMODES)? Bolo[bolotrig].mode_arch: ArchDetecMode;
	switch(mode) {
	  case ARCH_COINC: 
	  case ARCH_TOUS:  for(voie=0; voie<VoiesNb; voie++) TrmtInhibeVoie(voie); break;
	  case ARCH_VOISINS:
		for(bv=0; bv<Bolo[bolotrig].nbvoisins; bv++) {
			bolo = Bolo[bolotrig].voisin[bv];
			TrmtInhibeDetecteur(bolo);
			if(ArchAssocMode) { for(k=0; k<Bolo[bolo].nbassoc; k++) if((ba = Bolo[bolo].assoc[k]) >= 0) TrmtInhibeDetecteur(ba); }
		}
		break;
	  case ARCH_POSY:
		tour = CABLAGE_POSY(Bolo[bolotrig].pos);
		for(bolo=0; bolo<BoloNb; bolo++) if(CABLAGE_POSY(Bolo[bolo].pos) == tour) {
			TrmtInhibeDetecteur(bolo);
			if(ArchAssocMode) { for(k=0; k<Bolo[bolo].nbassoc; k++) if((ba = Bolo[bolo].assoc[k]) >= 0) TrmtInhibeDetecteur(ba); }
		}
		break;
	  case ARCH_INDIV:  /* Aussi utilise par defaut */
	  default:
		TrmtInhibeDetecteur(bolotrig);
		if(ArchAssocMode) { for(k=0; k<Bolo[bolotrig].nbassoc; k++) if((ba = Bolo[bolotrig].assoc[k]) >= 0) TrmtInhibeDetecteur(ba); }
		break;
	}
}
/* ========================================================================== */
static int TrmtCandidatPlace(char async, int voietrig, int64 moment) {
	/* rend la place <n> du prochain evenement a stocker */
	int n,j;

	n = EvtsEmpiles;
	if(EvtsEmpiles) {
		if(async || (voietrig < 0)) {
			for(n=EvtsEmpiles; n; ) if(EvtStocke[--n].date <= moment) { n++; break; }
		}
		if(n && (EvtStocke[n-1].trig == voietrig) && (EvtStocke[n-1].date == moment)) {
			printf("%s/ ! Evenement %s en double @%lld\n",DateHeure(),VoieManip[voietrig].nom,moment);
			EvtSupprimes++;
			return(-1);
		}
		if(EvtsEmpiles >= MAX_EMPILES) { PileUp++; return(-1); }
		if(async || (voietrig < 0)) for(j=EvtsEmpiles; j>n; --j) memcpy(&(EvtStocke[j]),&(EvtStocke[j-1]),sizeof(TypeEvtDetecte));
	}
	return(n);
}
/* ========================================================================== */
static void TrmtCandidatStocke(int n, int voietrig, int64 moment, float niveau, short origine) {
	if(VerifSuiviEtatEvts) printf("(%s)  Evt[%d/%d]: date %12s = %lld\n",__func__,n,EvtsEmpiles,VoieManip[voietrig].nom,moment);
	EvtStocke[n].trig = voietrig;
	EvtStocke[n].niveau = niveau;
	EvtStocke[n].origine = origine;
	EvtStocke[n].date = moment;
	EvtStocke[n].debut = moment - (int64)(VoieEvent[voietrig].avant_evt * VoieTampon[voietrig].trmt[TRMT_AU_VOL].compactage);
	EvtStocke[n].fin = moment + (int64)VoieEvent[voietrig].apres_evt + (int64)(VoieTampon[voietrig].interv / 2);
	if(n > 0) EvtStocke[n].delai = (int)(moment - EvtStocke[n-1].date);
	else EvtStocke[n].delai = (int)(moment - TrmtDernierTrig);
	if(VerifSuiviEtatEvts) {
		if(n > 0) printf("(%s)  | compare avec Evt[%d].date(%d) = %lld",__func__,n-1,EvtStocke[n-1].trig,EvtStocke[n-1].date);
		else printf("(%s)  | compare avec dernier trgr = %lld",__func__,TrmtDernierTrig);
		printf(", soit delai = %d\n",EvtStocke[n].delai);
	}
	if(EvtsEmpiles && (n < EvtsEmpiles)) EvtStocke[n+1].delai = (int)(EvtStocke[n+1].date - EvtStocke[n].date);
	if(EvtsEmpiles && (n < EvtsEmpiles)) printf("(%s) modifie Evt[%d].date(%d) = %lld, soit delai[%d] = %d\n",__func__,n+1,EvtStocke[n+1].trig,EvtStocke[n+1].date,n+1,EvtStocke[n].delai);
	EvtStocke[n].nbvoies = 0;
	EvtsEmpiles++;
	if(EvtsEmpiles > 0) TrmtDernierTrig = EvtStocke[EvtsEmpiles-1].date;
	else TrmtDernierTrig = moment;
	if(VerifSuiviEtatEvts) printf("(%s)  | nouveau dernier trgr      = %lld\n",__func__,TrmtDernierTrig);
	Acquis[AcquisLocale].etat.evt_recus++;
}
/* ========================================================================== */
int TrmtCandidatSignale(char async, int voietrig, int64 moment, float niveau, short origine) {
	int i,j,n;
	
	if(origine == TRGR_ALGO_CONN) {
		int gene; int t,k,rang,rep; float ldb; int64 courant,dernier; TypeRepartiteur *repart;
		if((gene = VoieManip[voietrig].gene) >= 0) {
			Gene[gene].mode_manu = 1;
			if(((rang = VoieTampon[voietrig].rang_sample) >= 0) && ((rep = SambaEchantillon[rang].rep) >= 0)) {
				repart = &(Repart[rep]);
				courant = DernierPointTraite / (int64)VoieTampon[voietrig].trmt[TRMT_AU_VOL].compactage;
				dernier = LectStampsLus / (int64)VoieTampon[voietrig].trmt[TRMT_AU_VOL].compactage;
				for(t=0; t<VoieTampon[voietrig].unite.taille; t++) {
					k = Modulo(courant,VoieTampon[voietrig].brutes->max);
					ldb = (float)*(VoieTampon[voietrig].brutes->t.sval + k);
					VoieTampon[voietrig].brutes->t.sval[k] = (TypeDonnee)RepartGenereEvt(repart,voietrig,gene,ldb);
					if(++courant > dernier) break;
				}
				if(t >= VoieTampon[voietrig].unite.taille) Gene[gene].mode_manu = 0; // sinon, 2eme evt lors de la trame suivante
			}
			return(-1);
		}
	}
	// TrmtDbgCale = (EvtASauver < 5); // (EvtASauver && ((Evt[EvtASauver-1].num >= 1) && (Evt[EvtASauver-1].num <= 3)));
	// TrmtDebug = (Acquis[AcquisLocale].etat.evt_trouves < 5);
	VerifCalculEvts &= (Acquis[AcquisLocale].etat.evt_trouves < 5);
	
	if(VerifSuiviEtatEvts) printf("\n(%s) [%8lld] Signalement #%-3d: trouve evt %s %s = %g ADUs sur %s (@ %g ms)\n",__func__,
		moment,EvtSignales,TrgrOrigineTexte[origine%10],(origine>=10)?"en ligne":"",niveau,VoieManip[voietrig].nom,(float)moment/Echantillonnage);
	EvtSignales++;
	/*? demande la voie rapide {
		int bolo; TypeReglage *regl_trig;
		bolo = VoieManip[voietrig].det;
		if((regl_trig = Bolo[bolo].regl_trig)) {
		}
	} */
	if(LectAcqCollective && (origine != TRGR_EXTERNE)) {
		int bolotrig; char mode;
		bolotrig = (voietrig >= 0)? VoieManip[voietrig].det: -1;
		mode = ((bolotrig >= 0) && (Bolo[bolotrig].mode_arch != ARCH_NBMODES))?  Bolo[bolotrig].mode_arch: ArchDetecMode;
		if(mode == ARCH_MANIP) /* ((mode == ARCH_COINC) || (mode == ARCH_TOUS)) */ {
			for(j=0; j<Partit[PartitLocale].nbsat; j++) if(Partit[PartitLocale].exter[j]) SambaEcritSatLong(Partit[PartitLocale].sat[j],SMB_E,moment);
		} /* sinon, regarder si les bolos concernes sont sur un autre satellite */
	}
	DerniereVoieSignalee = voietrig;
	if(VoieTampon[voietrig].trig.trgr->conn.out) TrgrEtat[voietrig].date_sortie = moment + TrgrEtat[voietrig].delai;
	TrmtInhibeAutresVoies(voietrig);
	if(TrmtImmediat) {
		if(VerifSuiviEtatEvts) printf("          Traitement immediat du nouvel evenement\n");
		TrmtCandidatStocke(0,voietrig,moment,niveau,origine); n = 0;
		TrmtConstruitEvt();
	} else {
		if((n = TrmtCandidatPlace(async,voietrig,moment)) < 0) return(n);
		TrmtCandidatStocke(n,voietrig,moment,niveau,origine);
		if(TrmtDebug || VerifEvtMoyen || VerifSuiviTrmtEvts) {
			printf("          + Evenement signale sur %s a p=%lld, fin fixee a p=%lld, recherchera apres p=%lld, en attente #%d\n",
				VoieManip[voietrig].nom,moment,EvtStocke[n].fin,
				TrgrEtat[voietrig].debut_recherche * VoieTampon[voietrig].trmt[TRMT_AU_VOL].compactage,n+1);
		}
		if(VerifSuiviEtatEvts) {
			printf("(%s) Pile pour %d evenement%s signale%s:\n",__func__,Accord2s(EvtsEmpiles));
			for(i=EvtsEmpiles; i; ) {
				--i; printf("(%s) %2d| %4d: %16s @ %10lld (%+7d)\n",__func__,i+1,Acquis[AcquisLocale].etat.evt_trouves+1+i,VoieManip[EvtStocke[i].trig].nom,EvtStocke[i].date,EvtStocke[i].delai);
			}
		}
	}

	if(VerifSuiviTrmtEvts) {
		printf("(%s) [%8lld] Evt '%s' %s stocke #%-3d (rang %d): @p=%lld, termine a p=%lld [%s%6lld] sur la voie %s",__func__,
			DernierPointTraite,TrgrOrigineTexte[origine%10],(origine>=10)?"en ligne":"",Acquis[AcquisLocale].etat.evt_trouves+1+EvtsEmpiles,
			EvtStocke[n].niveau,EvtStocke[n].date,EvtStocke[n].fin,EvtsEmpiles?"en cours->":"precedent:",EvtATraiter->fin,VoieManip[voietrig].nom);
		printf("(%s) delai=%-+5d (precedent a p=%lld)\n",__func__,EvtStocke[n].delai,TrmtDernierTrig);
#ifdef HISTOS_DELAIS_EVT
		{	int k;
			k = (EvtStocke[n].delai + 500) / 1000;
			if(k < 0) k = 0; else if(k >= MAXHISTOTRIG) k = MAXHISTOTRIG - 1;
			HistoTrig[k] += 1;
		}
#endif
	}
	return(n);
}
/* ========================================================================== */
static float TrmtCalculeOf7(int voie, TypeTamponDonnees *tampon, int lngr, int64 reduction, int adrs_mini, float *rms) {
	/* Calcule l'offset, ou ligne de base, d'une trace demarrant exactement a <adrs_mini> */
	int i,k,nb;
	double base,bruit;
	TypeDonnee *ptr_int16,sval;
	TypeLarge  *ptr_int32,ival;
	TypeSignal *ptr_float,rval;
	
	k = adrs_mini + (VoieEvent[voie].base_dep / reduction); if(k >= lngr) k -= lngr;
	nb = VoieEvent[voie].base_lngr / reduction;
//+	if(VerifCalculEvts) printf("(%s) Calage %s: calcul de la LdB dans [%d, %d[, ",__func__,
//+		Calage.a_faire? "en cours": "termine ",(int)debut+VoieEvent[voie].base_dep,(int)debut+VoieEvent[voie].base_dep+VoieEvent[voie].base_lngr);
	if(VerifCalculEvts) printf("(%s) Recherche dans [%d, +%d=%d[\n",__func__,k,nb,k+nb);
	base = bruit = 0.0;
	if(tampon->type == TAMPON_INT16) {
		ptr_int16 = tampon->t.sval + k;
//		if(VerifCalculEvts) printf("%d donnees examinees @%08X =",nb,ptr_int16);
		for(i=0; i<nb; i++) {
			sval = *ptr_int16++;
			// if(VerifCalculEvts) { if(!(i % 10)) printf("\n%4d/",i); printf(" %6d",sval); }
			base += (double)sval; bruit += (double)(sval * sval);
			if(++k >= lngr) { k = 0; ptr_int16 = tampon->t.sval; }
		}
	} else if(tampon->type == TAMPON_INT32) {
		ptr_int32 = tampon->t.ival + k;
		//		if(VerifCalculEvts) printf("%d donnees examinees @%08X =",nb,ptr_int16);
		for(i=0; i<nb; i++) {
			ival = *ptr_int32++;
			// if(VerifCalculEvts) { if(!(i % 10)) printf("\n%4d/",i); printf(" %6d",sval); }
			base += (double)ival; bruit += (double)(ival * ival);
			if(++k >= lngr) { k = 0; ptr_int32 = tampon->t.ival; }
		}
	} else {
		ptr_float = tampon->t.rval + k;
		if(VerifCalculEvts) printf("(%s) %d donnees examinees @%08X =",__func__,nb,ptr_float);
		for(i=0; i<nb; i++) {
			rval = *ptr_float++;
			if(VerifCalculEvts) { if(!(i % 10)) printf("\n%4d/",i); printf(" %6g",rval); }
			base += (double)rval; bruit += (double)(rval * rval);
			if(++k >= lngr) { k = 0; ptr_float = tampon->t.rval; }
		}
	}
	if(VerifCalculEvts) printf("\n");
	base = (base / (double)nb);
	bruit = sqrt((bruit / (double)nb) - (base * base));
	*rms = (float)bruit;
	if(TrmtDbgCale || VerifCalculEvts) printf("(%s) Calage %s: niveau ligne de base = %g\n",__func__,Calage.a_faire? "en cours": "termine ",(float)base);
	return((float)base);
}
/* ========================================================================== */
static INLINE float TrmtCalculeAmpl(int voie, TypeTamponDonnees *tampon, int lngr,
	int64 reduction, int adrs_mini, float *base, float *rms) {
	/* Calcule l'amplitude d'un evenement demarrant exactement a <debut>.
	L'offset, ou ligne de base, est pris dans le "pretrigger" a partir de <debut>.
	*/
	int i,k,nb,decalage;
	TypeDonnee *ptr_int16; TypeLarge *ptr_int32; TypeSignal *ptr_float; // of7;
	float val,ampl_min,ampl_max;
	int temps_max;
	float dev;
	float *template; int dim; int depart;

	/* 1. Au debut de cet intervalle, evaluation de la ligne de base */
	*base = TrmtCalculeOf7(voie,tampon,lngr,reduction,adrs_mini,rms);
	if(VerifCalculEvts) printf("(%s) Base = %g\n",__func__,*base);
//	if(*base > 0) of7 = (TypeDonnee)(*base + 0.5);
//	else of7 = (TypeDonnee)(*base - 0.5);
	
	switch(VoieTampon[voie].trig.calcule) {
	  case TRMT_AMPLMAXI: case TRMT_INTEGRAL:
		/* 2a. Localisation du vrai maximum */
		decalage = (VoieEvent[voie].base_dep + VoieEvent[voie].base_lngr) / reduction;
		nb = ((VoieEvent[voie].avant_evt + VoieTampon[voie].trig.inhibe) / reduction) - decalage; if(nb < 1) nb = 1;
		k = adrs_mini + decalage; if(k >= lngr) k -= lngr;
//-		if(VerifCalculEvts) printf("(%s) Calcul du maximum dans [%d, %d[, ",__func__,(int)debut,(int)fin);
		ampl_max = ampl_min = 0.0;
		if(tampon->type == TAMPON_INT16) {
			ptr_int16 = tampon->t.sval + k;
			if(VerifCalculEvts) printf("%d donnees examinees @%08X =",nb,ptr_int16);
			for(i=0; i<nb; i++) {
				val = (float)(*ptr_int16++) - *base;
				if(VerifCalculEvts) { if(!(i % 10)) printf("\n%4d/",i); printf(" %6g",val); }
				if(i == 0) ampl_max = ampl_min = val;
				else if(val > ampl_max) ampl_max = val;
				else if(val < ampl_min) ampl_min = val;
				if(++k >= lngr) { k = 0; ptr_int16 = tampon->t.sval; }
			}
		} else if(tampon->type == TAMPON_INT32) {
			ptr_int32 = tampon->t.ival + k;
			if(VerifCalculEvts) printf("%d donnees examinees @%08X =",nb,ptr_int32);
			for(i=0; i<nb; i++) {
				val = (float)(*ptr_int32++) - *base;
				if(VerifCalculEvts) { if(!(i % 10)) printf("\n%4d/",i); printf(" %6g",val); }
				if(i == 0) ampl_max = ampl_min = val;
				else if(val > ampl_max) ampl_max = val;
				else if(val < ampl_min) ampl_min = val;
				if(++k >= lngr) { k = 0; ptr_int32 = tampon->t.ival; }
			}
		} else {
			ptr_float = tampon->t.rval + k;
			if(VerifCalculEvts) printf("%d donnees examinees @%08X =",nb,ptr_float);
			for(i=0; i<nb; i++) {
				val = *ptr_float++ - *base;
				if(VerifCalculEvts) { if(!(i % 10)) printf("\n%4d/",i); printf(" %6g",val); }
				if(i == 0) ampl_max = ampl_min = val;
				else if(val > ampl_max) ampl_max = val;
				else if(val < ampl_min) ampl_min = val;
				if(++k >= lngr) { k = 0; ptr_float = tampon->t.rval; }
			}
		}
		if(VerifCalculEvts) printf("\n");
            if(TrgrGardeSigne && (VoieTampon[voie].trig.signe != 0)) {
                if(VoieTampon[voie].trig.signe > 0) dev = ampl_max;
                else /* if(VoieTampon[voie].trig.signe < 0) */ dev = ampl_min;
            } else {
                if(ampl_max > -ampl_min) dev = ampl_max; else dev = ampl_min;
            }
		break;
	  case TRMT_EVTMOYEN:
		/* 2b1. produit scalaire */
		if(VerifEvtMoyen) printf("(%s) Calcul du max sur la voie %s par fit\n",__func__,VoieManip[voie].nom);
		nb = 1;
		depart = adrs_mini + (VoieTampon[voie].unite.depart / reduction); if(depart >= lngr) depart -= lngr;
		template = VoieTampon[voie].unite.val + VoieTampon[voie].unite.depart;
		dim = VoieTampon[voie].unite.arrivee - VoieTampon[voie].unite.depart;
		TrmtMaxConvolution(voie,tampon,lngr,depart,*base,template,dim,nb,&dev,&temps_max);
		dev *= VoieTampon[voie].norme_evt;
		/* 2b.2 Sensibilite dans la ligne de base */
		nb = VoieEvent[voie].base_lngr - dim + 1;
		if(nb > 0) {
			if(VerifEvtMoyen) printf("(%s) Calcul du bruit sur la voie %s par fit\n",__func__,VoieManip[voie].nom);
			depart = adrs_mini + (VoieEvent[voie].base_dep / reduction); if(depart >= lngr) depart -= lngr;
			TrmtMaxConvolution(voie,tampon,lngr,depart,*base,template,dim,nb,rms,&temps_max);
			*rms *= VoieTampon[voie].norme_evt;
		} 
		break;
	}
	if(TrmtDbgCale || VerifCalculEvts || VerifEvtMoyen) printf("(%s) Calage %s, amplitude calculee pour %s: %g\n",
		__func__,Calage.a_faire? "en cours": "termine ",VoieManip[voie].nom,dev);
	return(dev);
}
/* ========================================================================== */
static INLINE float TrmtCalculeMax(int voie, TypeTamponDonnees *tampon, int lngr,
	int64 reduction, int adrs_mini, float base) {
	/* Calcule l'amplitude d'un evenement demarrant exactement a <debut>.
	L'offset, ou ligne de base, est pris dans le "pretrigger" a partir de <debut>.
	*/
	int i,k,nb,decalage;
	TypeDonnee *ptr_int16; TypeLarge *ptr_int32; TypeSignal *ptr_float; // of7;
	float val,ampl_min,ampl_max;
	float dev;
	
	if(VerifCalculEvts) printf("(%s) Base = %g\n",__func__,base);
	decalage = (VoieEvent[voie].base_dep + VoieEvent[voie].base_lngr) / reduction;
	nb = (VoieEvent[voie].lngr / reduction) - decalage; if(nb < 1) nb = 1;
	k = adrs_mini + decalage; if(k >= lngr) k -= lngr;
	ampl_max = ampl_min = 0.0;
	if(tampon->type == TAMPON_INT16) {
		ptr_int16 = tampon->t.sval + k;
		if(VerifCalculEvts) printf("%d donnees examinees @%08X =",nb,ptr_int16);
		for(i=0; i<nb; i++) {
			val = (float)(*ptr_int16++) - base;
			if(i == 0) ampl_max = ampl_min = val;
			else if(val > ampl_max) ampl_max = val;
			else if(val < ampl_min) ampl_min = val;
			if(++k >= lngr) { k = 0; ptr_int16 = tampon->t.sval; }
		}
	} else if(tampon->type == TAMPON_INT32) {
		ptr_int32 = tampon->t.ival + k;
		if(VerifCalculEvts) printf("%d donnees examinees @%08X =",nb,ptr_int32);
		for(i=0; i<nb; i++) {
			val = (float)(*ptr_int32++) - base;
			if(i == 0) ampl_max = ampl_min = val;
			else if(val > ampl_max) ampl_max = val;
			else if(val < ampl_min) ampl_min = val;
			if(++k >= lngr) { k = 0; ptr_int32 = tampon->t.ival; }
		}
	} else {
		ptr_float = tampon->t.rval + k;
		if(VerifCalculEvts) printf("%d donnees examinees @%08X =",nb,ptr_float);
		for(i=0; i<nb; i++) {
			val = *ptr_float++ - base;
			if(i == 0) ampl_max = ampl_min = val;
			else if(val > ampl_max) ampl_max = val;
			else if(val < ampl_min) ampl_min = val;
			if(++k >= lngr) { k = 0; ptr_float = tampon->t.rval; }
		}
	}
    if(TrgrGardeSigne && (VoieTampon[voie].trig.signe != 0)) {
        if(VoieTampon[voie].trig.signe > 0) dev = ampl_max;
        else /* if(VoieTampon[voie].trig.signe < 0) */ dev = ampl_min;
    } else {
        if(ampl_max > -ampl_min) dev = ampl_max; else dev = ampl_min;
    }
	if(TrmtDbgCale || VerifCalculEvts || VerifEvtMoyen) printf("(%s) Amplitude calculee pour %s: %g\n",__func__,VoieManip[voie].nom,dev);
	return(dev);
}
/* ========================================================================== */
static float TrmtTrouveMax(int voie, TypeTamponDonnees *tampon, int lngr, int64 reduction,
	int adrs_mini, int nb, int64 *pmax, float *base, float *rms) {
/*	int64 debut_mini, int64 debut_maxi
	Recherche un maximum dans un intervalle [debut_mini, debut_maxi[ marquant le debut
	possible d'un evenement.
	L'intervalle de recherche du maxi est donc decale de la zone de calcul de la ligne de base.
	L'offset, ou ligne de base, est pris dans le pretrigger demarrant a <debut_mini>.
	Dans ces conditions, il peut etre fausse si l'evenement demarre plutot vers <debut_maxi>.
*/
	int i,k,decalage; int dim;
	TypeDonnee *ptr_int16; TypeLarge *ptr_int32; TypeSignal *ptr_float; // of7;
//	TypeDonnee val,ampl_min,ampl_max;
	int depart;
	float val,ampl_min,ampl_max;
	int debut_recherche,temps_min,temps_max,debut_max;
	float dev,*template;

	/* 1. Au debut de cet intervalle, evaluation de la ligne de base */
	*base = TrmtCalculeOf7(voie,tampon,lngr,reduction,adrs_mini,rms);
	if(TrmtDbgCale || VerifCalculEvts) printf("(%s) Base = %g\n",__func__,*base);

	/* 2. Dans la suite de cet intervalle, recherche du maximum */
	if(TrmtDbgPmax) printf("(%s) Recherche dans [%d, +%d=%d[\n",__func__,adrs_mini,nb,adrs_mini+nb);
	// decalage = (VoieEvent[voie].base_dep + VoieEvent[voie].base_lngr) / reduction;
	decalage = VoieEvent[voie].avant_evt / reduction;
	debut_recherche = adrs_mini + decalage;

	switch(VoieTampon[voie].trig.calcule) {
	  case TRMT_AMPLMAXI: case TRMT_INTEGRAL:
		/* 2a. Localisation du vrai maximum */
		if(debut_recherche > lngr) debut_recherche -= lngr;
		if(TrmtDbgCale || VerifCalculEvts)
			printf("(%s) Calage %s: recherche du maximum dans [%d, %d[, ",__func__,Calage.a_faire? "en cours": "termine ",debut_recherche,debut_recherche + nb);
		k = debut_recherche;
		ampl_max = ampl_min = 0.0; temps_max = temps_min = 0;
		if(tampon->type == TAMPON_INT16) {
			ptr_int16 = tampon->t.sval + k;
			if(TrmtDbgCale || VerifCalculEvts) printf("%d donnees examinees @%08X =",nb,ptr_int16);
			for(i=0; i<nb; i++) {
				val = (float)(*ptr_int16++) - *base;
				if(TrmtDbgCale || VerifCalculEvts) { if(!(i % 10)) printf("\n%4d/",i); printf(" %6g",val); }
				// if(val > 0.0) { if(val > ampl_max) { ampl_max = val; temps_max = i; } } 
				// else          { if(val < ampl_min) { ampl_min = val; temps_min = i; } }
				if(i == 0) { ampl_max = ampl_min = val; temps_max = temps_min = i; }
				else if(val > ampl_max) { ampl_max = val; temps_max = i; }
				else if(val < ampl_min) { ampl_min = val; temps_min = i; }
				if(++k >= lngr) { k = 0; ptr_int16 = tampon->t.sval; }
			}
		} else if(tampon->type == TAMPON_INT32) {
			ptr_int32 = tampon->t.ival + k;
			if(TrmtDbgCale || VerifCalculEvts) printf("%d donnees examinees @%08X =",nb,ptr_int32);
			for(i=0; i<nb; i++) {
				val = (float)(*ptr_int32++) - *base;
				if(TrmtDbgCale || VerifCalculEvts) { if(!(i % 10)) printf("\n%4d/",i); printf(" %6g",val); }
				// if(val > 0.0) { if(val > ampl_max) { ampl_max = val; temps_max = i; } }
				// else          { if(val < ampl_min) { ampl_min = val; temps_min = i; } }
				if(i == 0) { ampl_max = ampl_min = val; temps_max = temps_min = i; }
				else if(val > ampl_max) { ampl_max = val; temps_max = i; }
				else if(val < ampl_min) { ampl_min = val; temps_min = i; }
				if(++k >= lngr) { k = 0; ptr_int32 = tampon->t.ival; }
			}
		} else {
			ptr_float = tampon->t.rval + k;
			if(TrmtDbgCale || VerifCalculEvts) printf("%d donnees examinees @%08X =",nb,ptr_float);
			for(i=0; i<nb; i++) {
				val = *ptr_float++ - *base;
				if(TrmtDbgCale || VerifCalculEvts) { if(!(i % 10)) printf("\n%4d/",i); printf(" %6g",val); }
				// if(val > 0.0) { if(val > ampl_max) { ampl_max = val; temps_max = i; } } 
				// else          { if(val < ampl_min) { ampl_min = val; temps_min = i; } }
				if(i == 0) { ampl_max = ampl_min = val; temps_max = temps_min = i; }
				else if(val > ampl_max) { ampl_max = val; temps_max = i; }
				else if(val < ampl_min) { ampl_min = val; temps_min = i; }
				if(++k >= lngr) { k = 0; ptr_float = tampon->t.rval; }
			}
		}
		if(TrmtDbgCale || VerifCalculEvts) printf("\n");
		if(TrmtDbgPmax) printf("(%s) pmax = %d + %d = %d\n",__func__,decalage,temps_max,decalage + temps_max);
        if(TrgrGardeSigne && (VoieTampon[voie].trig.signe != 0)) {
            if(VoieTampon[voie].trig.signe > 0) {
                dev = ampl_max; *pmax = ((decalage + temps_max) * reduction);
            } else /* if(VoieTampon[voie].trig.signe < 0) */ {
                dev = ampl_min; *pmax = ((decalage + temps_min) * reduction);
            }
        } else {
            if(ampl_max > -ampl_min) { dev = ampl_max; *pmax = ((decalage + temps_max) * reduction); }
            else                     { dev = ampl_min; *pmax = ((decalage + temps_min) * reduction); }
        }
		break;
	  case TRMT_EVTMOYEN:
		/* 2b. Recherche par produit glissant */
		if(VerifEvtMoyen) printf("(%s) Recherche du max sur la voie %s par convolution\n",__func__,VoieManip[voie].nom);
		depart = debut_recherche + (VoieTampon[voie].unite.depart / reduction); if(depart >= lngr) depart -= lngr;
		template = VoieTampon[voie].unite.val + VoieTampon[voie].unite.depart;
		dim = VoieTampon[voie].unite.arrivee - VoieTampon[voie].unite.depart;
		TrmtMaxConvolution(voie,tampon,lngr,depart,*base,template,dim,nb,&dev,&debut_max);
		dev *= VoieTampon[voie].norme_evt;
		*pmax = (debut_max * reduction) + (int64)VoieTampon[voie].unite.pre_trigger;
		break;
	}
	if(VerifCalculEvts) printf("(%s) Calage %s, maximum trouve a %lld sur %s = %g\n",__func__,Calage.a_faire? "en cours": "termine ",*pmax-decalage,VoieManip[voie].nom,dev);
	return(dev);
}
/* ========================================================================== */
static void TrmtMaxConvolution(int voie, TypeTamponDonnees *tampon, int lngr, int adrs_mini,
	float base, float *evtunite, int dim, int nb, float *dev, int *temps) {
/*  Calcule le plus grand produit scalaire glissant (convolution)
	du tampon de donnees <tampon> (de taille <lngr>) en commencant a <debut>,
	avec le modele <template> (de taille <dim>).
	Les valeurs du tampon de donnees sont decalees de <base>.
	Le nombre de points testes (glissement) est <nb>.
	Le plus grand produit absolu est mis dans <dev>,
	et le decalage du debut est dans <temps> (compris entre 0 et <nb>-1) */
	int i,j,k;
	int debut_min,debut_max;
	double rval,dev_min,dev_max;
	TypeDonnee *ptr_int16; TypeLarge *ptr_int32; TypeSignal *ptr_float; float *template;
#ifdef ALTIVEC
	float *vec1;

	if (SettingsAltivec) { 
		vec1 = (float *)malloc((nb - 1 + dim) * sizeof(float));
		k = adrs_mini;
		if(tampon->type == TAMPON_INT16) {
			ptr_int16 = tampon->t.sval + k;
			for(j=0; j<(nb - 1 + dim); j++) {
				vec1[j] = (float)(*ptr_int16++) - base;
				if(++k >= lngr) { k = 0; ptr_int16 = tampon->t.sval; }
			}
		} else {
			ptr_float = tampon->t.rval + k;
			for(j=0; j<(nb - 1 + dim); j++) {
				vec1[j] = (float)(*ptr_float++) - base;
				if(++k >= lngr) { k = 0; ptr_float = tampon->t.rval; }
			}
		}
	}
#endif			
	dev_max = dev_min = 0; debut_max = debut_min = 0;
	for(i=0; i<nb; i++) {
#ifdef ALTIVEC
		if (SettingsAltivec) dotpr(vec1+i,1,evtunite,1,&rval,dim);	
		else {
#endif
			template = evtunite;
			rval = 0.0; 
			k = adrs_mini;
			if(tampon->type == TAMPON_INT16) {
				ptr_int16 = tampon->t.sval + k;
				for(j=0; j<dim; j++) {
					// if(i == 0) printf("%03d) + %d x %g ",j,*ptr_int16-base,*template);
					rval += ((double)(*ptr_int16++) - (double)base) * (double)*template++;
					// if(i == 0) printf("= %g\n",rval);
					if(++k >= lngr) { k = 0; ptr_int16 = tampon->t.sval; }
				}
			} else if(tampon->type == TAMPON_INT32) {
				ptr_int32 = tampon->t.ival + k;
				for(j=0; j<dim; j++) {
					// if(i == 0) printf("%03d) + %d x %g ",j,*ptr_int16-base,*template);
					rval += ((double)(*ptr_int32++) - (double)base) * (double)*template++;
					// if(i == 0) printf("= %g\n",rval);
					if(++k >= lngr) { k = 0; ptr_int32 = tampon->t.ival; }
				}
			} else {
				ptr_float = tampon->t.rval + k;
				for(j=0; j<dim; j++) {
					// if(i == 0) printf("%03d) + %d x %g ",j,*ptr_float-base,*template);
					rval += ((double)(*ptr_float++) - (double)base) * (double)*template++;
					// if(i == 0) printf("= %g\n",rval);
					if(++k >= lngr) { k = 0; ptr_float = tampon->t.rval; }
				}
			}
#ifdef ALTIVEC
		}	
#endif
		// printf("%03d/ rval = %g\n",i,rval);
		// if(rval > 0.0) { if(rval > dev_max) { dev_max = rval; debut_max = i; } } 
		// else           { if(rval < dev_min) { dev_min = rval; debut_min = i; } }
		if(rval > dev_max) { dev_max = rval; debut_max = i; }
		else if(rval < dev_min) { dev_min = rval; debut_min = i; }
	}
#ifdef ALTIVEC
	if(SettingsAltivec) { if(vec1) free(vec1); }
#endif			
    if(TrgrGardeSigne && (VoieTampon[voie].trig.signe != 0)) {
        if(VoieTampon[voie].trig.signe > 0) {
            *dev = (float)dev_max; *temps = debut_max;
        } else /* if(VoieTampon[voie].trig.signe < 0) */ {
            *dev = (float)dev_min; *temps = debut_min;
        }
    } else {
        if(dev_max > -dev_min) { *dev = (float)dev_max; *temps = debut_max; }
        else { *dev = (float)dev_min; *temps = debut_min; }
    }
//	printf("soit dev = %g\n",*dev);
//+	if(VerifEvtMoyen) printf("(%s) %d fit a t=%lld avec un template de %d valeurs: dev=%g\n",__func__,nb,debut,dim,*dev);
}
/* ========================================================================== */
static void TrmtAbandonneEvts(int64 premier, char *ou) {
	int i,j; int64 precedente;

	precedente = 0; // GCC
	for(j=0; j<EvtsEmpiles; j++) if(EvtStocke[j].debut > premier) break;
	if(j) {
		if(j < EvtsEmpiles) precedente = EvtStocke[j - 1].date;
		EvtsEmpiles -= j; // si auparavant j == EvtsEmpiles, EvtsEmpiles devient 0 donc precedente inutilisee
		for(i=0; i<EvtsEmpiles; i++) {
			memcpy(&(EvtStocke[i]),&(EvtStocke[i+j]),sizeof(TypeEvtDetecte));
			// printf("          ! PileEvt[%d->%d].date: %lld\n",i+j,i,EvtStocke[i].date);
		}
		TrmtEvtJetes += j;
		printf("%s/ Perte de %d evenement%s (sur %d en attente): %s hors delai\n",DateHeure(),Accord1s(j),EvtsEmpiles+j,ou);
		if(EvtsEmpiles) EvtATraiter->delai = (int)(EvtATraiter->date - precedente);
	}
}
/* ========================================================================== */
static char TrmtRelanceSuivi() {
	int64 decale,avant;
	int voie,bolo,gene,i,compte,modul,parm; int64 nb,compte_reel;
	
	decale = LectStampsLus - (LngrTampons - 100) - DernierPointTraite; // UsageTampons? // ((int)LectIntervTrmt * Diviseur2) a la place de 100
	if(decale < 0) {
		printf("          | Lu: %lld (mini %lld), traite: %lld. Mode 'SURCHARGE' abandonne.\n",LectStampsLus,LectStampsLus - (LngrTampons - 100),DernierPointTraite);
		return(0);
	}
	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue > 0) {
		// printf("%13s: signal.nb=%d, trmt[TRMT_PRETRG].suivi.nb=%d\n",
		// 	VoieManip[voie].nom,VoieTampon[voie].signal.nb,VoieTampon[voie].trmt[TRMT_PRETRG].suivi.nb);
		TrmtRAZvoie(voie);
		if(VoieTampon[voie].traitees->max) {
			nb = ((int)decale / (VoieTampon[voie].trmt[TRMT_AU_VOL].compactage * VoieTampon[voie].trmt[TRMT_PRETRG].compactage));
			VoieTampon[voie].traitees->prem += nb;
			VoieTampon[voie].trmt[TRMT_PRETRG].plus_ancien += nb;
			while(VoieTampon[voie].traitees->prem >= VoieTampon[voie].traitees->max) VoieTampon[voie].traitees->prem -= VoieTampon[voie].traitees->max;
		}
		VoieTampon[voie].trmt[TRMT_PRETRG].point0 = VoieTampon[voie].trmt[TRMT_PRETRG].plus_ancien - VoieTampon[voie].traitees->prem;
		if(VoieManip[voie].def.trmt[TRMT_PRETRG].type == TRMT_DEMODUL) {
			bolo = VoieManip[voie].det;
			modul = Bolo[bolo].d2;
			parm = VoieManip[voie].def.trmt[TRMT_PRETRG].p1;
			compte_reel = (int64)VoieTampon[voie].trmt[TRMT_PRETRG].compte + decale;
			VoieTampon[voie].trmt[TRMT_PRETRG].compte = compte = Modulo(compte_reel,Bolo[bolo].d2);
			if(compte <= parm) VoieTampon[voie].trmt[TRMT_PRETRG].phase = 0;
			else if(compte <= (modul / 2)) VoieTampon[voie].trmt[TRMT_PRETRG].phase = 1;
			else if(compte <= ((modul / 2) + parm)) VoieTampon[voie].trmt[TRMT_PRETRG].phase = 2;
			else VoieTampon[voie].trmt[TRMT_PRETRG].phase = 3;
		} else {
			VoieTampon[voie].trmt[TRMT_PRETRG].compte = 0;
			VoieTampon[voie].trmt[TRMT_PRETRG].phase = 0;
		}
		VoieTampon[voie].trmt[TRMT_PRETRG].valeur = 0;
		VoieTampon[voie].trmt[TRMT_PRETRG].suivi.nb = 0;
		VoieTampon[voie].trmt[TRMT_PRETRG].suivi.prems = 0;
		if(VoieTampon[voie].pattern.val) {
			for(i=0; i<VoieTampon[voie].pattern.max; i++) {
				VoieTampon[voie].pattern.val[i] = 0.0;
				VoieTampon[voie].pattern.nb[i] = 0.0;
			}
		}
	}
	for(gene=0; gene<GeneNb; gene++) Gene[gene].mode_manu = 0;
	avant = DernierPointTraite;
	LectStampJetes += decale;
	DernierPointTraite += decale;
	if(VerifErreurLog) {
		printf("          |    Echantillons lus: %16lld\n",LectStampsLus);
		printf("          |         Plus ancien: %16lld\n",LectStampsLus - LngrTampons);
		printf("          | Traitement arrete a: %16lld\n",avant);
		printf("          |          Temps mort: %16lld echantillons (%.2f ms)\n",decale,(float)decale / Echantillonnage);
		printf("          | Traitement repris a: %16lld\n",DernierPointTraite);
	} // else printf("          | Temps mort: %lld echantillons (%.2f ms)\n",decale,(float)decale / Echantillonnage);
	TrmtPanicAjoute += decale; TrmtPanicNb++;
	TrmtModePanique = 1; TrmtApanique = 1;
	// ils seront bien elimines plus tard... TrmtAbandonneEvts(LectStampsLus - LngrTampons,"traitement");
	// EvtATraiter->trig = -1; EvtATraiter->fin = -1;
	return(1);
}
/* ========================================================================== */
static char TrmtSeuilDepasse(TypeVoieArchivee *ajoutee, float base, float ampl, char sinon) {
	int voie; char depasse_le_seuil;
	
	voie = ajoutee->num;
	if(VoieTampon[voie].trig.trgr->type == TRGR_SEUIL) {
		float max; max = base + ampl;
		depasse_le_seuil = (((VoieTampon[voie].trig.trgr->sens > 0) && (max > VoieTampon[voie].trig.trgr->minampl)) 
						 || ((VoieTampon[voie].trig.trgr->sens < 2) && (max < VoieTampon[voie].trig.trgr->maxampl)));
	} else if((VoieTampon[voie].trig.trgr->type == TRGR_FRONT) || (VoieTampon[voie].trig.trgr->type == TRGR_DERIVEE)) {
		depasse_le_seuil = (((VoieTampon[voie].trig.trgr->sens > 0) && (ampl > VoieTampon[voie].trig.trgr->minampl))
						 || ((VoieTampon[voie].trig.trgr->sens < 2) && (ampl < VoieTampon[voie].trig.trgr->maxampl)));
	} else depasse_le_seuil = sinon;
	return(depasse_le_seuil);
}
/* ========================================================================== */
static INLINE char TrmtCalageVoie(int n, int voie, int va, int voietrig, int64 moment, float niveau) {
	int i,k,kmax,nb,ph,canal,ntp;
	int64 pmax,compactage,reduction,date_locale,premier,dernier,debut,deplace,retard_si_template;
	char depasse_le_seuil,phases_en_cours;
	float *template; int dim;
	int deb_integrale[DETEC_PHASES_MAX],fin_integrale[DETEC_PHASES_MAX];
	int lngr,fin_ldb,lngr_analyse,lngr_montee,adresse,t10,t90,t75,t50,t25;
	double base,bruit,integrale[DETEC_PHASES_MAX],total;
	float dev,ampl,niv0,of7,rms,montee,descente,rc,fval,fprec;
	float f10,f90,f25,f50,f75,ampl10,ampl90,ampl25,ampl75,ampl50,ampl100;
	TypeTamponDonnees *tampon;
	TypeDonnee *ptr_int16,sval;
	TypeSignal *ptr_float,rval;
	TypeVoieArchivee *en_cours;
	int phys_cal,ref_cal;
	char dbg_cal;

	if(VerifSuiviEtatEvts || TrmtDbgCale || VerifEvtMoyen)
		printf("(%s)          Evt[%d] (#%d) Ajout #%d: voie %12s, calage %s\n",__func__,
			   n,Acquis[AcquisLocale].etat.evt_trouves+1,va+1,VoieManip[voie].nom,Calage.a_faire? "a faire": "fait");
	if(VerifSuiviTrmtEvts)
		printf("(%s)          Evt[%d] (#%d) Ajout #%d @p=%lld (traite a p=%lld)\n",__func__,
			   n,Acquis[AcquisLocale].etat.evt_trouves+1,va+1,moment,DernierPointTraite);
	if(va >= VOIES_MAX) {
		OpiumFail("On essaie de sauver plus que VOIES_MAX=%d voies (voir rundata.h)",VOIES_MAX);
		Acquis[AcquisLocale].etat.active = 0;
		return(0);
	}
	if(VetoMis) return(0);

	dbg_cal = 0;
	if(dbg_cal) printf("(%s) [%3d] Ajout #%d = voie %s (calage %s, %s sur %s)\n",__func__,n,va,VoieManip[voie].prenom,Calage.demande?"demande":"a oublier",Calage.a_faire?"a faire":"fait",(SettingsCalageObj == CALAGE_AMPL)? "l'amplitude":"le centrage");
	if(dbg_cal) printf("(%s)              Met voie %s dans l'ajout %d\n",__func__,VoieManip[voie].prenom,va);
	en_cours = &(TrmtMesuree[va]); // &(Evt[n].voie[va]); amsi en mode COINC, on peut ne pas garder une voie si < seuil
	compactage = (int64)VoieTampon[voie].trmt[TRMT_AU_VOL].compactage;

	/* a titre d'initialisation */
	date_locale = moment / compactage;
	for(canal=0; canal<EvtATraiter->nbvoies; canal++) if(EvtATraiter->voie[canal].indx == voie) break;
	if(canal >= EvtATraiter->nbvoies) canal = -1;

	en_cours->num = voie;
	en_cours->stream = (canal < 0);
	en_cours->mesuree = 0;
	en_cours->enregistree = VoieManip[voie].num_hdr[EVTS];
	en_cours->trig_pos = VoieTampon[voie].trig.trgr->minampl;
	en_cours->trig_neg = VoieTampon[voie].trig.trgr->maxampl;
	en_cours->date_locale = date_locale;
	en_cours->debut = date_locale - (int64)VoieEvent[voie].avant_evt;
	if(en_cours->stream) {
		en_cours->adresse = Modulo(en_cours->debut,VoieTampon[voie].brutes->max);
		en_cours->dim = VoieEvent[voie].lngr;
	} else {
		en_cours->adresse = EvtATraiter->voie[canal].adrs;
		en_cours->dim = EvtATraiter->voie[canal].lngr;
		if(VerifCalculEvts) printf("(%s)         Evt[%d] (#%d) @p=%lld, voie %s demarre a %lld sur %lld points\n",__func__,
			n,Acquis[AcquisLocale].etat.evt_trouves+1,moment,VoieManip[voie].nom,en_cours->adresse,en_cours->dim);
	}
	en_cours->ref = TRMT_AU_VOL;
	for(ntp=0; ntp<MONIT_NBVARS; ntp++) en_cours->val[ntp] = 0.0;
	en_cours->amplimontee = 0.0;
	en_cours->depasse_le_seuil = 0;
	en_cours->total = 0.0;
	if(TrmtDbgPmax) {
		printf("(%s) %s evt %d: Moment = %lld soit date_locale temporaire = %lld\n",__func__,VoieManip[voie].nom,n,moment,date_locale);
		printf("(%s) %s evt %d: Temps local = temps general / %lld, pretriger = %d [local]\n",__func__,VoieManip[voie].nom,n,compactage,VoieEvent[voie].avant_evt);
	}

	tampon = 0; lngr = 0; pmax = 0; reduction = 1; ampl100 = 0.0; // GCC
	debut = deplace = 0; /* pour faire plaisir au compilo */
	dev = 0.0; /* idem */
	retard_si_template = 0;
	total = 0.0; for(ph=0; ph<DETEC_PHASES_MAX; ph++) integrale[ph] = 0.0;

	if(TrmtDbgPmax) printf("(%s) evt %d [%s]: calage %s, %s\n",__func__,n,VoieManip[voie].prenom,
						   Calage.demande?"demande":"a oublier",Calage.a_faire?"a faire":"fait");
	//	printf("ajout de la voie %s\n",VoieManip[voie].nom);

	/*
	 * Localisation en temps de l'evenement
	 */
	if(((VoieTampon[voie].trig.calcule == NEANT) || (VoieTampon[voie].trig.calcule == TRMT_LDB))) {
		/* pas non plus si histos predefinis */
		/* Utilisation de l'intervalle d'origine */
		tampon = VoieTampon[voie].brutes; lngr = tampon->max;
		if(en_cours->stream) {
			int64 recale;
			if((voie == voietrig) || (voietrig < 0)) recale = moment;
			else recale = moment + (int64)(VoieTampon[voie].decal - VoieTampon[voietrig].decal);
			date_locale = recale / compactage;
			debut = date_locale - (int64)VoieEvent[voie].avant_evt + (int64)VoieEvent[voie].base_dep;
			if(debut < VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien) {
				//  TrgrEtat[voie].debut_recherche = recale + VoieTampon[voie].trig.inhibe;
				if(Acquis[AcquisLocale].etat.evt_trouves > 0) {
					// VoieTampon[voie].jetees += (VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien - debut);
					printf("%s/ On n'a pas utilise a temps la voie %s pour l'evenement #%d @%lld!\n",
						   DateHeure(),VoieManip[voie].nom,Acquis[AcquisLocale].etat.evt_trouves,moment);
					TrmtAbandonneEvts(VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien * compactage,"calcul de ligne de base");
				}
				//  printf("debut voie%d = %d avant %d\n",voie,(int)debut,(int)VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien);
				if(TrmtDebug) printf("(%s) Debut de la voie <%s> a p=%d (avant le premier:%d)\n",__func__,VoieManip[voie].nom,(int)debut,(int)VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien);
				//  EvtATraiter->trig = -1; EvtATraiter->fin = -1;
				TrmtAbandonne = 1;
				return(0);
			}
			adresse = Modulo(debut,lngr);
		} else {
			adresse = en_cours->adresse + VoieEvent[voie].base_dep; if(adresse > lngr) adresse -= lngr;
		}
		if(VoieTampon[voie].trig.calcule == TRMT_LDB) {
			/* Affinage de la ligne de base d'apres une partie du pre-trigger */
			k = adresse;
			nb = VoieEvent[voie].base_lngr;
			if(VerifCalculEvts) printf("(%s) Calcul de la LdB et de l'amplitude dans [%d, %d[\n",__func__,
									   (int)debut+VoieEvent[voie].base_dep,(int)debut+VoieEvent[voie].base_dep+nb);
			base = bruit = 0.0;
			if(tampon->type == TAMPON_INT16) {
				ptr_int16 = tampon->t.sval + k;
				for(i=0; i<nb; i++) {
					sval = *ptr_int16++;
					base += (double)sval; bruit += (double)(sval * sval);
					if(++k >= lngr) { k = 0; ptr_int16 = tampon->t.sval; }
				}
			} else {
				ptr_float = tampon->t.rval + k;
				for(i=0; i<nb; i++) {
					rval = *ptr_float++;
					base += (double)rval; bruit += (double)(rval * rval);
					if(++k >= lngr) { k = 0; ptr_float = tampon->t.rval; }
				}
			}
			base = (base / (double)nb);
			bruit = sqrt((bruit / (double)nb) - (base * base));
		} else {
			base = (double)VoieTampon[voie].signal.base;
			bruit = 0.0;
		}
		en_cours->ref = TRMT_AU_VOL;
		niv0 = (float)base;
		rms = (float)bruit;
		dev = niveau - niv0;
		en_cours->date_locale = date_locale;
		en_cours->val[MONIT_BASE] = niv0;
		en_cours->val[MONIT_BRUIT] = rms;
		en_cours->val[MONIT_AMPL] = dev;
		en_cours->val[MONIT_SEUIL] = rms;
		en_cours->val[MONIT_DECAL] = (float)VoieTampon[voie].decal / Echantillonnage;
		en_cours->val[MONIT_MONTEE] = (float)(abs(VoieTampon[voie].signal.climbs)) * VoieEvent[voie].horloge;
		en_cours->val[MONIT_DESCENTE] = 0.0;
		en_cours->val[MONIT_BRUTE] = dev;
		en_cours->val[MONIT_MAX] = en_cours->val[MONIT_AMPL];
		en_cours->amplimontee = dev;
		en_cours->mesuree = 1;
		if(TrmtDbgCale || VerifCalculEvts) printf("(%s) Niveau ligne de base = %g, amplitude calculee pour %s: %g\n",__func__,(float)base,VoieManip[voie].prenom,dev);
	} else {
		/* Affinage de l'amplitude et calcul du decalage entre voies */
		int64 moment_mini,moment_maxi; // position de l'evenement (points absolus)
		int64 debut_mini;    // index du debut de trace (points stockes == compactes)
		int adrs_mini;       // index du debut de trace (points sur tampon utilise ==  reduits, en plus)

		/* 1.1 Pas de calage du maximum selon la physique de la voie */
		if(TrmtDbgPmax) printf("(%s) %s evt %d: calage %s, %s\n",__func__,VoieManip[voie].nom,n,Calage.demande?"demande":"a oublier",Calage.a_faire?"a faire":"fait");
		if(Calage.a_faire) {
			phys_cal = ModeleVoie[VoieManip[voie].type].physique;
			if(CalageSurTriggee && (voie == voietrig)) phys_cal = Calage.reference;
			ref_cal = (Calage.demande == CALAGE_UNIQUE)? Calage.reference: phys_cal;
			// printf("(%s) %7s: physique voie #%d, reference #%d pour calage type %d\n",__func__,VoieManip[voie].prenom,phys_cal,ref_cal,Calage.demande);
			if(en_cours->stream) {
				/* idem, d'ailleurs, si datation unifiee et voie non eligible */
				if((SettingsCalageObj == CALAGE_AMPL) && (phys_cal != ref_cal)) {
					if(TrmtDbgPmax) printf("(%s) %s evt %d: ne participe pas au calage (%d|%d), calage abandonne\n",__func__,VoieManip[voie].prenom,n,phys_cal,ref_cal);
					return(1);
				}
			}
		}
		if(TrmtDbgPmax) printf("(%s) %s evt %d: Recherche du maxi activee\n",__func__,VoieManip[voie].prenom,n);

		/* 1.2 Tampon de travail: donnees brutes ou traitees? */
		retard_si_template = 0;
		if(en_cours->stream && SettingsCalcPretrig && VoieTampon[voie].cree_traitee && VoieTampon[voie].traitees->t.rval) {
			tampon = VoieTampon[voie].traitees; en_cours->ref = TRMT_PRETRG;
			reduction = (int64)VoieTampon[voie].trmt[TRMT_PRETRG].compactage;
			if(VoieManip[voie].def.trmt[TRMT_PRETRG].template && VoieTampon[voie].delai_template)
				retard_si_template = (int64)(VoieManip[voie].def.evt.dimtemplate * reduction);
			dernier = DernierPointTraite;
			premier = (dernier / compactage) - tampon->nb;
			if(TrmtDebug > 1) printf("(%s) Tampon %s de %d (stocke) a %d (absolu)\n",__func__,VoieManip[voie].nom,(int)premier,(int)dernier);
			if(TrmtDbgPmax) {
				if(tampon->type == TAMPON_INT16)
					printf("(%s) %s evt %d: tampon Strig utilise (%08llx)\n",__func__,VoieManip[voie].prenom,n,(IntAdrs)tampon->t.sval);
				else printf("(%s) %s evt %d: tampon Rtrig utilise (%08llx)\n",__func__,VoieManip[voie].prenom,n,(IntAdrs)tampon->t.rval);
			}
		} else {
			tampon = VoieTampon[voie].brutes; en_cours->ref = TRMT_AU_VOL;
			reduction = 1;
			dernier = LectStampsLus; /* pas VoieTampon[voie].lus car on regarde les pts absolus */
			premier = VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien; /* points stockes */
			if(TrmtDbgPmax) {
				if(tampon->type == TAMPON_INT16)
					printf("(%s) %s evt %d: tampon Sbrut utilise (%08llx)\n",__func__,VoieManip[voie].prenom,n,(IntAdrs)tampon->t.sval);
				else printf("(%s) %s evt %d: tampon Rbrut utilise (%08llx)\n",__func__,VoieManip[voie].prenom,n,(IntAdrs)tampon->t.rval);
			}
		}
		lngr = tampon->max;

		/* 1.3 Determination de l'intervalle de recherche [debut, fin[ */
		/*    avec debut et fin pointant finalement sur les debut et fin possibles d'evenement */
		int decal,interv; // nb de points absolus
		if(VoieTampon[voie].trig.trgr->type == TRGR_FRONT) interv = VoieEvent[voie].lngr * (int)compactage;
		else { interv = VoieTampon[voie].interv; if(interv <= 0) interv = VoieEvent[voie].lngr * (int)compactage; }
		interv /= 2;
		if(en_cours->stream) {
			decal = VoieTampon[voie].decal - ((voietrig >= 0)? VoieTampon[voietrig].decal: 0);
			moment += (retard_si_template * compactage);
			if(TrmtDbgPmax) printf("(%s) %s evt %d: Retarde la date absolue de %lld\n",__func__,VoieManip[voie].prenom,n,retard_si_template * compactage);
			moment_mini = moment + (int64)(decal - interv); moment_maxi = moment + (int64)(decal + interv);
			if(TrmtDbgPmax) printf("(%s) %s evt %d: Maximum entre %lld et %lld (%lld abs)\n",__func__,VoieManip[voie].prenom,n,moment_mini,moment_maxi,moment_maxi-moment_mini);
			//- k = Modulo(moment_mini,lngr);
			if(TrmtDebug) {
				printf("(%s) Fenetre <%s> de %d points, decalee de %d (absolus)\n",__func__,VoieManip[voie].prenom,(int)VoieTampon[voie].interv,(int)VoieTampon[voie].decal);
				printf("(%s) Trigger <%s>             antidecale  de %d (absolus)\n",__func__,(voietrig >= 0)?VoieManip[voietrig].prenom:"externe",(int)((voietrig >= 0)? VoieTampon[voietrig].decal: 0));
				printf("(%s) Recherche maxi de %d a %d (absolus)\n",__func__,(int)moment_mini,(int)moment_maxi);
				printf("(%s) Evt possible: - %d (stockes) + %d (absolus)\n",__func__,VoieEvent[voie].avant_evt,VoieEvent[voie].apres_evt);
			}
			debut_mini = (moment_mini / compactage) - (int64)VoieEvent[voie].avant_evt; /* points stockes */
			if(TrmtDebug > 1) printf("(%s) soit evt de %d (stockes) a %d (absolus)\n",__func__,(int)debut_mini,(int)moment_maxi);
			if(debut_mini < premier) {
				// TrgrEtat[voie].debut_recherche = debut_mini + (int64)VoieEvent[voie].avant_evt + VoieTampon[voie].trig.inhibe;
				if(debut_mini >= 0) TrmtTropTard++;
				if(TrmtTropTard) {
					// VoieTampon[voie].jetees += (premier - debut_mini);
					printf("%s/ Traitement hors delai (#%d) pour la voie %s pour l'evenement #%d @%lld!\n",
						   DateHeure(),TrmtTropTard,VoieManip[voie].nom,Acquis[AcquisLocale].etat.evt_trouves,moment);
					printf("          . Recherche entre %lld et %lld (%d points/%lld), tampon entre %lld et %lld (%d points/%lld)\n",
						   debut_mini*compactage,moment_maxi,(int)(moment_maxi-(debut_mini*compactage)),compactage,premier*compactage,dernier,(int)(dernier-(premier*compactage)),compactage);
					printf("          . Tampon mini recommande: %d points au lieu de %d\n",(int)((dernier / compactage) - debut_mini),lngr);
					// deplace = (premier - debut_mini) * compactage; moment_maxi = TrgrEtat[voie].debut_recherche * compactage;
					// printf("          . Decalage demande de %lld, minimum %lld - %lld = %lld\n",deplace,moment_maxi,DernierPointTraite,moment_maxi - DernierPointTraite);
					// if(deplace < (moment_maxi - DernierPointTraite)) deplace = moment_maxi - DernierPointTraite;
					TrmtAbandonneEvts(premier * compactage,"recherche du maximum");
				} else {
					if(TrmtDebug || VerifEvtMoyen || VerifSuiviTrmtEvts)
						printf("(%s) Debut de la voie <%s> a p=%d, avant le premier=%d (stockes) => abandon\n",__func__,VoieManip[voie].nom,(int)debut_mini,(int)premier);
				}
				//				EvtATraiter->trig = -1; EvtATraiter->moment_maxi = -1;
				TrmtAbandonne = 1;
				if(TrmtDbgPmax) printf("(%s) %s evt %d: debut_mini < premier, abandon\n",__func__,VoieManip[voie].nom,n);
				return(0);
			}
			moment_maxi += (int64)VoieEvent[voie].apres_evt;  /* moment_maxi evt en points absolus */
			if(moment_maxi > dernier) {
				EvtATraiter->fin = moment_maxi;
				if(TrmtDebug || VerifEvtMoyen || VerifSuiviTrmtEvts)
					printf("(%s) [%8lld] Fin de la voie %s a p=%lld (au dela du dernier=%lld) => report\n",__func__,
						   DernierPointTraite,VoieManip[voie].nom,moment_maxi,dernier);
				TrmtAbandonne = -1;
				if(TrmtDbgPmax) printf("(%s) %s evt %d: moment_maxi > dernier, abandon\n",__func__,VoieManip[voie].nom,n);
				return(0);
			}
			adrs_mini = Modulo(debut_mini / reduction,lngr);
			nb = (2 * interv) / compactage;
			if(TrmtDbgPmax) printf("(%s) %s evt %d: Maximum entre %lld et %lld (%d loc)\n",__func__,VoieManip[voie].prenom,n,debut_mini,debut_mini+(int64)nb,nb);
			if(TrmtDbgPmax) printf("(%s) %s evt %d: Debut entre %d et %d (%d loc)\n",__func__,VoieManip[voie].prenom,n,adrs_mini,adrs_mini+nb,nb);
		} else {
			debut_mini = date_locale - (int64)VoieEvent[voie].avant_evt;
			adrs_mini = en_cours->adresse; nb = 1;
		}

		/* 1.4 Calcul des caracteristiques de la trace */
		dev = TrmtTrouveMax(voie,tampon,lngr,reduction,adrs_mini,nb,&pmax,&niv0,&rms);
		en_cours->val[MONIT_DECAL] = (float)(debut_mini + pmax - date_locale) * VoieEvent[voie].horloge;
		if(SettingsTempsEvt == TRGR_DATE_MAXI) date_locale = debut_mini + pmax;
		if(dbg_cal) printf("(%s) %7s: debut mesure  = %lld\n",__func__,VoieManip[voie].prenom,date_locale - (int64)VoieEvent[voie].avant_evt);
		if(TrmtDbgPmax) printf("(%s) %s evt %d: Calcule date_locale = %lld + %lld = %lld\n",__func__,VoieManip[voie].prenom,n,debut_mini,pmax,date_locale);
		if(VerifCalculEvts) printf("(%s)         Evt[%d] (#%d) @p=%lld, maximum a %lld\n",__func__,
			n,Acquis[AcquisLocale].etat.evt_trouves+1,moment,date_locale * compactage);
		if(VerifCalculEvts) printf("(%s<TrmtTrouveMax) Base = %g, Amplitude = %g\n",__func__,niv0,dev);
		en_cours->date_locale = date_locale - retard_si_template;
		en_cours->val[MONIT_BASE] = niv0;
		en_cours->val[MONIT_BRUIT] = rms;
		en_cours->val[MONIT_AMPL] = dev;
		en_cours->mesuree = 1;

		/* 1.4bis Deconvolution et calcul de la charge (= integrale de la deconvoluee) */
		if(SettingsDeconvPossible && VoieManip[voie].duree.deconv.calcule) {
			VoieTampon[voie].deconvoluee = SambaAssure(VoieTampon[voie].deconvoluee,VoieEvent[voie].lngr,&(VoieTampon[voie].dim_deconv));
			if(VoieTampon[voie].deconvoluee) {
				int pre,post; float perte,seuil; float sens; char positif; int duree,avance;
				pre = VoieManip[voie].duree.deconv.pre;
				post = VoieManip[voie].duree.deconv.post;
				perte = expf(-VoieEvent[voie].horloge / VoieManip[voie].duree.deconv.rc);
				seuil = VoieManip[voie].duree.deconv.seuil;
				sens = SambaDeconvolue(VoieTampon[voie].deconvoluee,
					VoieTampon[voie].brutes->t.sval,VoieTampon[voie].brutes->max,en_cours->adresse,en_cours->dim,
					niv0,pre,post,perte,seuil,0);
				positif = (sens > 0.0);
				en_cours->val[MONIT_CHARGE] = SambaCalculeCharges(VoieTampon[voie].deconvoluee,en_cours->dim,positif,seuil,
					&(en_cours->val[MONIT_TOP]),&(en_cours->val[MONIT_PRELIM]),&duree,&avance);
				en_cours->val[MONIT_DISPER] = (float)duree  * VoieEvent[voie].horloge;
				en_cours->val[MONIT_AVANCE] = (float)avance * VoieEvent[voie].horloge;
				/* {	int i;
					printf("(%s) %s deconvoluee, %d valeur%s",__func__,VoieManip[voie].nom,Accord1s(en_cours->dim));
					for(i=0; i<250; i++) {
						if(!(i % 10)) printf("\n|     %5d:",i);
						printf(" %8.1f",VoieTampon[voie].deconvoluee[i]);
					}
					printf("\n");
				} */
			}
		}

		/* 1.5 Memorisation du temps de la meilleure voie, mais si elle ne participe pas au trigger, dehors! */
		if(Calage.a_faire && (VoieTampon[voie].trig.trgr->type != NEANT)) {
			// if(VerifEvtMoyen) printf("(%s) %s evt %d: ne participe pas au trigger, calage abandonne\n",__func__,VoieManip[voie].nom,n); return(1);
			if(dbg_cal) printf("(%s) %7s: debut utilise = %lld\n",__func__,VoieManip[voie].prenom,en_cours->date_locale - (int64)VoieEvent[voie].avant_evt);
			ampl = fabsf(dev * VoieManip[voie].ADUennV);
			if(ampl > TrgrRef[ref_cal].amplitude) {
				TrgrRef[ref_cal].date = en_cours->date_locale * compactage;
				if(TrmtDbgPmax) {
					printf("(%s) %s evt %d: Avance la reference locale de %lld\n",__func__,VoieManip[voie].nom,n,retard_si_template);
					printf("(%s) %s evt %d: Reference de date de calage = %lld (abs)\n",__func__,VoieManip[voie].nom,n,TrgrRef[ref_cal].date);
				}
				TrgrRef[ref_cal].amplitude = ampl;
				TrgrRef[ref_cal].voie = voie;
				TrgrRef[ref_cal].est_defini = 1;
				if(TrmtDbgCale) printf("Calage %s: maxi=%g (%g nV)  et date_locale=%d pour les voies de type %d\n",Calage.a_faire? "en cours": "termine ",
									   dev,(float)ampl,(int)date_locale,ref_cal);
			}
		}
		if(dbg_cal) printf("(%s) %7s (ajout %d) = %g + %g +/- %g\n",__func__,VoieManip[en_cours->num].prenom,va,en_cours->val[MONIT_BASE],en_cours->val[MONIT_AMPL],en_cours->val[MONIT_BRUIT]);

		/* 1.6 Sensibilite dans la ligne de base */
		if((VoieTampon[voie].trig.calcule == TRMT_EVTMOYEN) && VoieTampon[voie].unite.val) {
			float calib;
			en_cours->val[MONIT_MAX] = TrmtCalculeMax(voie,tampon,lngr,reduction,en_cours->adresse,en_cours->val[MONIT_BASE]);
			dim = VoieTampon[voie].unite.arrivee - VoieTampon[voie].unite.depart;
			nb = VoieEvent[voie].base_lngr - dim + 1;
			if(nb > 0) {
				int temps_local;
				if(VerifEvtMoyen) printf("(%s) Calcul du bruit sur la voie %s par fit\n",__func__,VoieManip[voie].nom);
				if(en_cours->stream) {
					debut_mini = date_locale - (int64)VoieEvent[voie].avant_evt + VoieEvent[voie].base_dep;
					adrs_mini = Modulo(debut_mini / reduction,lngr);
				} else {
					debut_mini = date_locale - (int64)VoieEvent[voie].avant_evt + VoieEvent[voie].base_dep;
					adrs_mini = en_cours->adresse + VoieEvent[voie].base_dep; if(adrs_mini >= lngr) adrs_mini -= lngr;
				}
				template = VoieTampon[voie].unite.val + VoieTampon[voie].unite.depart;
				TrmtMaxConvolution(voie,tampon,lngr,adrs_mini,en_cours->val[MONIT_BASE],template,dim,nb,&rms,&temps_local);
				rms *= VoieTampon[voie].norme_evt;
			}
			calib = VoieTampon[voie].unite.valeur / VoieTampon[voie].unite.amplitude;
			en_cours->val[MONIT_ENERGIE] = en_cours->val[MONIT_AMPL] * calib;
			en_cours->val[MONIT_SEUIL] = rms * calib;
			// printf("Fit sur evt #%d %s = %g\n",Acquis[AcquisLocale].etat.evt_trouves,VoieManip[voie].nom,dev);
		} else {
			en_cours->val[MONIT_MAX] = en_cours->val[MONIT_AMPL];
			en_cours->val[MONIT_SEUIL] = en_cours->val[MONIT_BRUIT];
		}

		/* --- Calcul du temps de montee: toujours sur les donnees brutes --- */
		/* 2. Complements sur les donnees BRUTES (temps de montee, integrales y.c. fraction rapide du signal) */
		if(SettingsMontee) {
			depasse_le_seuil = TrmtSeuilDepasse(en_cours,niv0,dev,1);
			montee = descente = rc = VoieManip[voie].def.evt.duree;
			fin_ldb = VoieEvent[voie].base_dep + VoieEvent[voie].base_lngr;
			lngr_montee = VoieEvent[voie].avant_evt - fin_ldb;
			lngr_analyse = VoieEvent[voie].lngr - fin_ldb; // ou encore lngr_montee + (VoieEvent[voie].lngr - VoieEvent[voie].avant_evt);
			if(SettingsIntegrale) for(ph=0; ph<DETEC_PHASES_MAX; ph++) if(VoieTampon[voie].phase[ph].dt) {
				deb_integrale[ph] = lngr_montee + VoieTampon[voie].phase[ph].t0;
				fin_integrale[ph] = deb_integrale[ph] + VoieTampon[voie].phase[ph].dt;
			}
			if(en_cours->ref == TRMT_PRETRG) /* ce qui n'est jamais le cas si !en_cours->stream */ {
				debut_mini = date_locale - (int64)VoieEvent[voie].avant_evt;
				adrs_mini = Modulo(debut_mini,VoieTampon[voie].brutes->max);
				ampl100 = TrmtCalculeAmpl(voie,VoieTampon[voie].brutes,VoieTampon[voie].brutes->max,1,adrs_mini,&of7,&rms);
			} else { of7 = niv0; ampl100 = dev; }
			ampl10 = ampl100 * VoieManip[voie].def.evt.ampl_10 / 100.0;
			ampl90 = ampl100 * VoieManip[voie].def.evt.ampl_90 / 100.0;
			ampl75 = ampl100 * 0.75; ampl50 = ampl100 * 0.50; ampl25 = ampl100 * 0.25;
			// printf("(%s) ampl 100: %g / 75: %g / 50: %g / 25: %g\n",__func__,ampl100,ampl75,ampl50,ampl25);
			t10 = t90 = t25 = t50 = t75 = depasse_le_seuil? -1: VoieEvent[voie].avant_evt;
			f10 = f90 = 0.0; /* (float)lngr_montee? */ ; f50 = (float)lngr_analyse; f75 = (float)lngr_montee; f25 = (float)lngr_analyse;
			if(en_cours->stream) kmax = Modulo(date_locale,VoieTampon[voie].brutes->max);
			else { kmax = en_cours->adresse + VoieEvent[voie].avant_evt; if(kmax >= lngr) kmax -= lngr; }
			if(VerifCalculEvts) printf("(%s)          Evt[%d] (#%d) @p=%lld, montee jusqu'a %lld\n",__func__,
				n,Acquis[AcquisLocale].etat.evt_trouves+1,moment,date_locale * compactage);

			/* temps de montee (a partir du maximum jusqu'a la fin de la base) */
			k = kmax; ptr_int16 = VoieTampon[voie].brutes->t.sval + k;
			fprec = (float)(*ptr_int16) - of7;
			for(i=lngr_montee; i; ) {
				--i; --ptr_int16; if(--k < 0) { k = VoieTampon[voie].brutes->max - 1; ptr_int16 = VoieTampon[voie].brutes->t.sval + k; }
				fval = (float)(*ptr_int16) - of7; 
				if(t90 < 0) {
					if(dev > 0.0) { if(fval <= ampl90) t90 = i; } else { if(fval >= ampl90) t90 = i; };
					if(t90 >= 0) {
						if(fval != fprec) f90 = (float)t90 + ((fval - ampl90) / (fval - fprec)); else f90 = (float)t90 + 0.5;
					}
				}
				if(t10 < 0) {
					if(dev > 0.0) { if(fval <= ampl10) t10 = i; } else { if(fval >= ampl10) t10 = i; };
					if(t10 >= 0) {
						if(fval != fprec) f10 = (float)t10 + ((fval - ampl10) / (fval - fprec)); else f10 = (float)t10 + 0.5;
						if(t90 < 0) {
							t90 = i;
							if(fval != fprec) f90 = (float)t90 + ((fval - ampl90) / (fval - fprec)); else f90 = (float)t90 + 0.5;
						}
						break;
					}
				}
			}

			/* temps de descente, integrales partielles, integrale totale */
			k = kmax; ptr_int16 = VoieTampon[voie].brutes->t.sval + k;
			fprec = (float)(*ptr_int16) - of7;
			// printf("(%s) On part de S[%d] = %g\n",__func__,k,fprec);
			for(i=lngr_montee+1; i<lngr_analyse; i++) {
				ptr_int16++; if(++k >= VoieTampon[voie].brutes->max) { k = 0; ptr_int16 = VoieTampon[voie].brutes->t.sval; }
				fval = (float)(*ptr_int16) - of7;  // a partir du maximum jusqu'a la fin de l'evenement
				// printf("(%s) On compare S[%d] = %g a Sp[%d] = %g\n",__func__,k,fval,k-1,fprec);
				if(t75 < 0) {
					if(dev > 0.0) { if(fval <= ampl75) t75 = i; } else { if(fval >= ampl75) t75 = i; };
					if(t75 >= 0) {
						if(fval != fprec) f75 = (float)t75 + ((fval - ampl75) / (fval - fprec)); else f75 = (float)t75 + 0.5;
					}
				}
				if(t50 < 0) {
					if(dev > 0.0) { if(fval <= ampl50) t50 = i; } else { if(fval >= ampl50) t50 = i; };
					if(t50 >= 0) {
						if(fval != fprec) f50 = (float)t50 - ((fval - ampl50) / (fval - fprec)); else f50 = (float)t50 - 0.5;
						if(t75 < 0) {
							t75 = i;
							if(fval != fprec) f75 = (float)t75 + ((fval - ampl75) / (fval - fprec)); else f75 = (float)t75 + 0.5;
						}
					}
				}
				if(t25 < 0) {
					if(dev > 0.0) { if(fval <= ampl25) t25 = i; } else { if(fval >= ampl25) t25 = i; };
					if(t25 >= 0) {
						if(fval != fprec) f25 = (float)t25 + ((fval - ampl25) / (fval - fprec)); else f25 = (float)t25 + 0.5;
						if(t75 < 0) {
							t75 = i;
							if(fval != fprec) f75 = (float)t75 + ((fval - ampl75) / (fval - fprec)); else f75 = (float)t75 + 0.5;
						}
						if(t50 < 0) {
							t50 = i;
							if(fval != fprec) f50 = (float)t50 + ((fval - ampl50) / (fval - fprec)); else f50 = (float)t50 + 0.5;
						}
					}
				}
				phases_en_cours = 0;
				if(SettingsIntegrale) {
					for(ph=0; ph<DETEC_PHASES_MAX; ph++) if(VoieTampon[voie].phase[ph].dt) {
						if(i < fin_integrale[ph]) {
							phases_en_cours = 1;
							if(i >= deb_integrale[ph]) {
								integrale[ph] += (double)fval;
								if(VerifIntegrales && (Acquis[AcquisLocale].etat.evt_trouves < 3)) printf("  | . integrale#%d = %g\n",ph,integrale[ph]);
							}
						}
					}
				}
				if(VoieTampon[voie].trig.calcule == TRMT_INTEGRAL) total += (double)fval;
				else if((t25 >= 0) && !phases_en_cours) break;
				fprec = fval;
			}
			montee = (f90 - f10) * VoieEvent[voie].horloge;
			//- if(n < 3) printf("4.Evt[%d]: f90=%g, f10=%g, montee=%g\n",n,f90,f10,montee);
			if(VoieManip[voie].def.evt.ampl_90 > VoieManip[voie].def.evt.ampl_10) montee *= (80.0 / (VoieManip[voie].def.evt.ampl_90 - VoieManip[voie].def.evt.ampl_10));
			//- if(n < 3) printf("5.Evt[%d]: f90=%g, f10=%g, montee=%g\n",n,f90,f10,montee);
			descente = (f50 - (float)lngr_montee) * VoieEvent[voie].horloge;
			if(((ampl25 > 0.0) && (ampl75 > 0.0)) || ((ampl25 < 0.0) && (ampl75 < 0.0))) rc = ((f75 - f25) / logf(ampl25 / ampl75)) * VoieEvent[voie].horloge;
			en_cours->val[MONIT_MONTEE] = montee;
			en_cours->val[MONIT_DESCENTE] = descente;
			en_cours->val[MONIT_RC] = rc;
			en_cours->val[MONIT_BRUTE] = ampl100;
			en_cours->amplimontee = ampl90 - ampl10;
			en_cours->total = (float)total;
			if(SettingsIntegrale) for(ph=0; ph<DETEC_PHASES_MAX; ph++) en_cours->integrale[ph] = (float)integrale[ph];
		}
	}

	return(1);

}

#ifdef SANS_SELECTION
/* ========================================================================== */
static char TrmtPasseConditions(TypeVoieArchivee *en_cours, TypeEvtCriteres *condition, char *raison) {
	char passe; int voie; float amplabs;

	passe = 1;
	voie = en_cours->num;
	amplabs = (en_cours->val[MONIT_AMPL] >= 0.0)? en_cours->val[MONIT_AMPL]: -en_cours->val[MONIT_AMPL];
	if(!condition->mont_inv && (en_cours->val[MONIT_MONTEE] < condition->montmin)) {
		if(VerifSuiviEtatEvts) sprintf(raison,"montee=%f < mini=%f",en_cours->val[MONIT_MONTEE],condition->montmin);
		passe = 0;
	} else if(!condition->mont_inv && (en_cours->val[MONIT_MONTEE] > condition->montmax)) {
		if(VerifSuiviEtatEvts) sprintf(raison,"montee=%f > maxi=%f",en_cours->val[MONIT_MONTEE],condition->montmax);
		passe = 0;;
	} else if(condition->mont_inv && (en_cours->val[MONIT_MONTEE] >= condition->montmin) && (en_cours->val[MONIT_MONTEE] <= condition->montmax)) {
		if(VerifSuiviEtatEvts) sprintf(raison,"montee=%f dans [%f, %f]",en_cours->val[MONIT_MONTEE],condition->montmin,condition->montmax);
		passe = 0;;
	} else if(!condition->ampl_inv && (amplabs < condition->underflow)) {
		if(VerifSuiviEtatEvts) sprintf(raison,"abs(amplitude)=%f < plancher=%g",amplabs,condition->underflow);
		passe = 0;;
	} else if(!condition->ampl_inv && (amplabs > condition->overflow)) {
		if(VerifSuiviEtatEvts) sprintf(raison,"abs(amplitude)=%f > plafond=%g",amplabs,condition->overflow);
		passe = 0;;
	} else if(condition->ampl_inv && (amplabs >= condition->underflow) && (amplabs <= condition->overflow)) {
		if(VerifSuiviEtatEvts) sprintf(raison,"abs(amplitude)=%f dans [%g, %g]",amplabs,condition->underflow,condition->overflow);
		passe = 0;;
	} else if(!condition->bruit_inv && (en_cours->val[MONIT_BRUIT] > condition->maxbruit)) {
		if(VerifSuiviEtatEvts) sprintf(raison,"bruit=%f > plafond=%f",en_cours->val[MONIT_BRUIT],condition->maxbruit);
		passe = 0;;
	} else if(condition->bruit_inv && (en_cours->val[MONIT_BRUIT] <= condition->maxbruit)) {
		if(VerifSuiviEtatEvts) sprintf(raison,"bruit=%f <= plancher=%f",en_cours->val[MONIT_BRUIT],condition->maxbruit);
		passe = 0;;
	} else if(((en_cours->val[MONIT_MONTEE] * 0.5) + en_cours->val[MONIT_DESCENTE]) < condition->porte) {
		if(VerifSuiviEtatEvts) sprintf(raison,"duree=%f+%f=%f < porte=%f",en_cours->val[MONIT_MONTEE] * 0.5,en_cours->val[MONIT_DESCENTE],
			(en_cours->val[MONIT_MONTEE] * 0.5) + en_cours->val[MONIT_DESCENTE],condition->porte);
		passe = 0;;
	}
	return(passe);
}
#else  /* !SANS_SELECTION */
/* ========================================================================== */
static char TrmtPasseCriteres(TypeVoieArchivee *en_cours, TypeCritereJeu *critere, char *raison) {
	int var;

	for(var=0; var<critere->nbvars; var++) {
		unsigned char ntuple; float val;
		ntuple = critere->var[var].ntuple;
		if(ntuple == MONIT_AMPL) val = (en_cours->val[ntuple] >= 0.0)? en_cours->val[ntuple]: -en_cours->val[ntuple];
		else val = en_cours->val[ntuple];
		if(!critere->var[var].inverse && (val < critere->var[var].min)) {
			if(VerifSuiviEtatEvts) sprintf(raison,"%s=%f < mini=%f",NtpName[ntuple],val,critere->var[var].min);
			return(0);
		} else if(!critere->var[var].inverse && (val > critere->var[var].max)) {
			if(VerifSuiviEtatEvts) sprintf(raison,"%s=%f > maxi=%f",NtpName[ntuple],val,critere->var[var].max);
			return(0);
		} else if(critere->var[var].inverse && (val >= critere->var[var].min) && (val <= critere->var[var].max)) {
			if(VerifSuiviEtatEvts) sprintf(raison,"%s=%f dans [%f, %f]",NtpName[ntuple],val,critere->var[var].min,critere->var[var].max);
			return(0);
		}
	}
	return(1);
}
#endif /* !SANS_SELECTION */
/* ========================================================================== */
static INLINE char TrmtCoupures(TypeVoieArchivee *en_cours) {
	int voie; float niveau; char evt_pas_sur,coupee,hors_limites;
	char pourquoi[256],dailleurs[256];

	//	if(!Calage.a_faire && (((VoieTampon[voie].trig.coupe == TRGR_COUPE_TJRS) || ((VoieTampon[voie].trig.trgr->type != NEANT) && VoieTampon[voie].trig.coupe)) || VetoPossible))
	//	if(!Calage.a_faire && (VoieTampon[voie].trig.coupe || VetoPossible))
	voie = en_cours->num;
	if(VoieTampon[voie].trig.coupe || VetoPossible) {
		niveau = en_cours->val[MONIT_BASE] + en_cours->val[MONIT_AMPL];
		evt_pas_sur = (!TrmtAccepteParDefaut && !VoieTampon[voie].trig.coupe);
		coupee = 0; pourquoi[0] = '\0';
		if((!TrmtAccepte || VetoPossible) && VoieTampon[voie].trig.coupe) {
		#ifdef SANS_SELECTION
			if(!TrmtPasseConditions(en_cours,&(VoieTampon[voie].trig.trgr->condition),pourquoi)) coupee = 1;
		#else
			if(!TrmtPasseCriteres(en_cours,&(VoieManip[voie].def.coup),pourquoi)) coupee = 1;
		#endif
		}
		hors_limites = 0; dailleurs[0] = '\0';
		if(TrmtNonConforme || evt_pas_sur) {
			if(en_cours->val[MONIT_AMPL] > 0.0) {
				if(VoieTampon[voie].trig.signe < 0) {
					if(VerifSuiviEtatEvts) sprintf(dailleurs,"amplitude=%f > 0.0",en_cours->val[MONIT_AMPL]);
					hors_limites = 1;
				} else {
					if(VoieTampon[voie].trig.trgr->type == TRGR_SEUIL) {
						if(niveau < VoieTampon[voie].trig.trgr->minampl) {
							if(VerifSuiviEtatEvts) sprintf(dailleurs,"niveau=%f < plancher=%g",niveau,VoieTampon[voie].trig.trgr->minampl);
							hors_limites = 1;
						}
					} else if(VoieTampon[voie].trig.trgr->type == TRGR_FRONT) {
						if(en_cours->val[MONIT_AMPL] < VoieTampon[voie].trig.trgr->minampl) {
							if(VerifSuiviEtatEvts) sprintf(dailleurs,"amplitude=%f < seuil=%g",en_cours->val[MONIT_AMPL],VoieTampon[voie].trig.trgr->minampl);
							hors_limites = 1;
						}
					} else if(VoieTampon[voie].trig.trgr->type == TRGR_DERIVEE) {
						if(en_cours->val[MONIT_AMPL] < VoieTampon[voie].trig.trgr->minampl) {
							if(VerifSuiviEtatEvts) sprintf(dailleurs,"amplitude=%f < seuil=%g",en_cours->val[MONIT_AMPL],VoieTampon[voie].trig.trgr->minampl);
							hors_limites = 1;
						} else if((en_cours->val[MONIT_MONTEE] > 0.0) && (VoieTampon[voie].trig.trgr->montee > 0.0)) {
							float derivee,seuil;
							derivee = en_cours->amplimontee / en_cours->val[MONIT_MONTEE];
							seuil = VoieTampon[voie].trig.trgr->minampl / VoieTampon[voie].trig.trgr->montee;
							if(derivee < seuil) {
								if(VerifSuiviEtatEvts) sprintf(dailleurs,"derivee=%f < seuil=%g",derivee,seuil);
								hors_limites = 1;
							}
						}
					}
				}
			} else {
				if(VoieTampon[voie].trig.signe > 0) {
					if(VerifSuiviEtatEvts) sprintf(dailleurs,"amplitude=%f < 0.0",en_cours->val[MONIT_AMPL]);
					hors_limites = 1;
				} else {
					if(VoieTampon[voie].trig.trgr->type == TRGR_SEUIL) {
						if(niveau > VoieTampon[voie].trig.trgr->maxampl) {
							if(VerifSuiviEtatEvts) sprintf(dailleurs,"niveau=%f > plafond=%g",niveau,VoieTampon[voie].trig.trgr->minampl);
							hors_limites = 1;
						}
					} else if(VoieTampon[voie].trig.trgr->type == TRGR_FRONT) {
						if(en_cours->val[MONIT_AMPL] > VoieTampon[voie].trig.trgr->minampl) {
							if(VerifSuiviEtatEvts) sprintf(dailleurs,"amplitude=%f < seuil=%g",-en_cours->val[MONIT_AMPL],VoieTampon[voie].trig.trgr->minampl);
							hors_limites = 1;
						}
					} else if(VoieTampon[voie].trig.trgr->type == TRGR_DERIVEE) {
						if(en_cours->val[MONIT_AMPL] > VoieTampon[voie].trig.trgr->maxampl) {
							if(VerifSuiviEtatEvts) sprintf(dailleurs,"amplitude=%f > seuil=%g",en_cours->val[MONIT_AMPL],VoieTampon[voie].trig.trgr->minampl);
							hors_limites = 1;
						} else if((en_cours->val[MONIT_MONTEE] > 0.0) && (VoieTampon[voie].trig.trgr->montee > 0.0)) {
							float derivee,seuil;
							derivee = en_cours->amplimontee / en_cours->val[MONIT_MONTEE];
							seuil = VoieTampon[voie].trig.trgr->minampl / VoieTampon[voie].trig.trgr->montee;
							if(derivee < seuil) {
								if(VerifSuiviEtatEvts) sprintf(dailleurs,"derivee=%f < seuil=%g",derivee,seuil);
								hors_limites = 1;
							}
						}
					}
				}
			}
			if(VoieTampon[voie].trig.trgr->type == TRGR_FRONT) {
				if(en_cours->val[MONIT_MONTEE] < VoieTampon[voie].trig.trgr->montee) {
					if(VerifSuiviEtatEvts) sprintf(dailleurs,"montee=%f < seuil=%f",en_cours->val[MONIT_MONTEE],VoieTampon[voie].trig.trgr->montee);
					hors_limites = 1;
				}
			}
		}
		if(!hors_limites) TrmtNonConforme = 0; else {
			// pas sur: if(VoieTampon[voie].trig.trgr->veto) VetoMis = 1;
			if(VerifSuiviEtatEvts) printf("(%s)          ! %s non conforme (%s)\n",__func__,VoieManip[voie].nom,dailleurs);
			if(evt_pas_sur) { coupee = 1; strcpy(pourquoi,"non conforme"); }
		}
		if(!coupee) TrmtAccepte = 1; else {
			if(VoieTampon[voie].trig.trgr->veto) VetoMis = 1;
			if(VerifSuiviEtatEvts) printf("(%s)          ! %s coupee (%s)\n",__func__,VoieManip[voie].nom,pourquoi);
		}
		if(VetoMis) {
			if(VerifSuiviEtatEvts) printf("(%s)          ! %s met son veto\n",__func__,VoieManip[voie].nom);
			return(0);
		}
	}

	return(1);
}
/* ========================================================================== */
char TrmtCalageDetecteur(int bolo, int n, int voietrig, int64 moment, float niveau, int *va) {
	int cap,voie;
	char dbg;

	dbg = TrmtDbgPmax; // dbg = 1;
	for(cap=0; cap<Bolo[bolo].nbcapt; cap++) {
		voie = Bolo[bolo].captr[cap].voie;
        if(dbg) printf("(%s)     Evt[%d]: voie %s ",__func__,n,VoieManip[voie].nom);
		// if((VoieTampon[voie].lue > 0) && (VoieManip[voie].def.trmt[TRMT_PRETRG].type != TRMT_INVALID))
		if((VoieTampon[voie].lue > 0) && (voie != voietrig)) {
            if(dbg) printf(" examinee\n");
			if(TrmtCalageVoie(n,voie,*va,voietrig,moment,niveau)) *va += 1; else return(0);
		} else if(dbg) printf(" abandonnee\n");
	}
	return(1);
}
/* ========================================================================== */
static void TrmtAjouteVoie(int n, int va, int64 moment, char regul_ok) {
	int i,j,k,l,nb,voie,bolo,evtdim;
	int64 evtstart,debut,compactage;
	int dev;
	TypeDonnee *tampon,*ptr_data,val; int lngr;
	TypeVoieArchivee *ajoutee,*en_cours; TypeFiltre *filtre; TypeSuiviFiltre *suivi;

    en_cours = &(TrmtMesuree[va]);
	voie = en_cours->num;
	bolo = VoieManip[voie].det;
	VoieTampon[voie].dernier_evt = n;
	// printf("(%s) nouvel evt voie %d: #%d\n",__func__,voie,VoieTampon[voie].dernier_evt);
	if((VoieManip[voie].def.trgr.reprise != TRGR_REPRISE_SEUIL) || (VoieTampon[voie].trig.cherche != TRGR_NIVEAU) || !(TrgrEtat[voie].evt_en_cours)) {
		float amplabs;
		amplabs = (en_cours->val[MONIT_AMPL] >= 0.0)? en_cours->val[MONIT_AMPL]: -en_cours->val[MONIT_AMPL];
		TrgrEtat[voie].debut_recherche = en_cours->date_locale + ((amplabs < (float)VoieManip[voie].def.trgr.alphaampl)? VoieTampon[voie].trig.inhibe: VoieTampon[voie].trig.sautlong);
		if(VoieManip[voie].def.trgr.reprise == TRGR_REPRISE_RENOUV) TrgrEtat[voie].reprise = 1;
	}

/*
 * Rempli le "bit-trigger"
 */
	if(en_cours->depasse_le_seuil) {
		if(BitTriggerNiveau == SAMBA_BITNIV_DETEC) k = (BitTriggerNums == SAMBA_BITNUM_GLOBAL)? bolo: Bolo[bolo].num_hdr;
		else /* (BitTriggerNiveau == SAMBA_BITNIV_VOIE) */ {
			if(BitTriggerBloc) {
				i = (BitTriggerNums == SAMBA_BITNUM_GLOBAL)? bolo: Bolo[bolo].num_hdr;
				k = (i * BitTriggerBloc) + VoieManip[voie].cap;
			} else k = (BitTriggerNums == SAMBA_BITNUM_GLOBAL)? voie: VoieManip[voie].num_hdr[EVTS];
		}
		l = k / 32;
		if(l < EVT_BITTRIGGER_MAX) Evt[n].liste[l] = Evt[n].liste[l] | (1 << (k % 32));
		VoieTampon[voie].date_dernier = moment; VoieTampon[voie].nbevts += 1;
		if(regul_ok && (VoieTampon[voie].regul_active == TRMT_REGUL_TAUX)) for(j=0; j<MAXECHELLES; j++) VoieTampon[voie].rgul.taux.nbevts[j] += 1;
	}

/*
 * Autres caracteristiques de l'evenement
 */
	compactage = (int64)VoieTampon[voie].trmt[TRMT_AU_VOL].compactage;
	evtstart = en_cours->debut;
	evtdim = en_cours->dim;
	debut = evtstart * compactage;

	if(VerifIntegrales && (Acquis[AcquisLocale].etat.evt_trouves < 3)) 
		printf("  = Ajout voie  evt#%d %s a t=%d sur lngr=%d, base=%g\n",Acquis[AcquisLocale].etat.evt_trouves,VoieManip[voie].nom,(int)evtstart,evtdim,en_cours->val[MONIT_BASE]);
	SambaTempsEchantillon(debut,0,0,&en_cours->sec,&en_cours->msec);
	en_cours->stamp_ref = en_cours->date_locale * VoieTampon[voie].trmt[TRMT_AU_VOL].compactage;
	en_cours->maxigiga = (int)(en_cours->stamp_ref / 1000000000);
	en_cours->maxistamp = Modulo((en_cours->stamp_ref),1000000000);
	en_cours->gigastamp = (int)((evtstart * compactage) / 1000000000);
	en_cours->stamp = Modulo((evtstart * compactage),1000000000);
	en_cours->horloge =  VoieEvent[voie].horloge * (float)VoieManip[voie].duree.post_moyenne;
	en_cours->avant_evt = VoieEvent[voie].avant_evt;
	//	en_cours->val[MONIT_MONTEE] = (float)climbs * VoieEvent[voie].horloge;
	//	en_cours->up = climbs * compactage;
	en_cours->fin = DernierPointTraite / compactage;
	en_cours->filtrees = 0;
	en_cours->demdim   = 0;
	en_cours->demarrage = 0;
	en_cours->nbfiltres = 0;
	en_cours->recalcul = 0;

    ajoutee = &(Evt[n].voie[Evt[n].nbvoies]);
	memcpy(ajoutee,en_cours,sizeof(TypeVoieArchivee));
	Evt[n].nbvoies += 1;

/*
 * Donne le debut de tous les evenements actuellement stockes
 */
	if(n == 0) {
//-		if(va == 0) LectDebutEvts = debut; else if(LectDebutEvts > debut) LectDebutEvts = debut;
		if(LectDebutEvts < 0) LectDebutEvts = debut; else if(LectDebutEvts > debut) LectDebutEvts = debut;
//		printf("(%s) Debut %s a p=%d, soit debut evts a p=%d\n",__func__,VoieManip[voie].nom,(int)debut,(int)LectDebutEvts);
	}
#ifndef SAUTE_EVT
/*
 * Redemarre le processus de demotification
 */
	if(SettingsSauteEvt && VoieTampon[voie].pattern.val) {
		for(i=0; i<VoieTampon[voie].pattern.max; i++) {
			VoieTampon[voie].pattern.val[i] = 0.0;
			VoieTampon[voie].pattern.nb[i] = 0.0;
		}
	}
#endif
/*
 * Elimination des evenements dont la voie est deja sauvee ceans
	for(i=0; i<EvtsEmpiles; i++) if(EvtStocke[i].trig == voie) {
		int64 point;
		point = EvtStocke[i].date / (int64)VoieTampon[voie].trmt[TRMT_AU_VOL].compactage;
		if(point < TrgrEtat[voie].debut_recherche) {
			EvtsEmpiles--; EvtSupprimes++;
			if(TrmtDebug || VerifEvtMoyen || VerifSuiviTrmtEvts) printf("(%s) Supprime evenement sur %s a p=%d, reste %d evts dans la pile\n",__func__,
				VoieManip[voie].nom,(int)EvtStocke[i].date,EvtsEmpiles);
			for(j=i; j<EvtsEmpiles; j++) memcpy(&(EvtStocke[i]),&(EvtStocke[j+1]),sizeof(TypeEvtDetecte));
			--i;
		}
	}
 */
/*
 * Calcul eventuel de l'evenement moyen
 */
	dev = (int)fabsf(ajoutee->val[MONIT_AMPL]);
	if(VoieTampon[voie].moyen.calcule && VoieTampon[voie].somme.val
	&& (dev >= VoieManip[voie].def.evt.min_moyen) && (dev <= VoieManip[voie].def.evt.max_moyen)) {
		tampon = VoieTampon[voie].brutes->t.sval; lngr = VoieTampon[voie].brutes->max;
		k = Modulo(evtstart,lngr);
		ptr_data = tampon + k;
		nb = evtdim; if(nb > VoieTampon[voie].somme.taille) nb = VoieTampon[voie].somme.taille;
		for(i=0; i<nb; i++) {
			val = *ptr_data++ - ajoutee->val[MONIT_BASE];
			VoieTampon[voie].somme.val[i] += (float)val;
			if(++k >= lngr) { k = 0; ptr_data = tampon; }
		}
		VoieTampon[voie].moyen.decal += ajoutee->val[MONIT_DECAL];
		VoieTampon[voie].moyen.nb += 1;
	}
/*
 * Donnees permettant de reconstituer le filtrage
 */
	if(SettingsStockeFiltrees) {
		int j,nb,depuis_debut_evt,etg;
		depuis_debut_evt = (int)(ajoutee->fin - ajoutee->debut);
		/* Pour l'affichage */
		ajoutee->max = 0;
		if(VoieTampon[voie].traitees->t.rval) {
//			nb = VoieTampon[voie].traitees->nb;
			nb = depuis_debut_evt / VoieTampon[voie].trmt[TRMT_PRETRG].compactage;
			if(VoieTampon[voie].traitees->nb < VoieTampon[voie].traitees->max) {
				if(nb <= VoieTampon[voie].traitees->nb)
					j = VoieTampon[voie].traitees->nb - nb;
				else { nb = VoieTampon[voie].traitees->nb; j = 0; }
			} else {
				if(nb <= VoieTampon[voie].traitees->max) {
					j = VoieTampon[voie].traitees->prem - nb;
					if(j < 0) j += VoieTampon[voie].traitees->max;
				} else {
					nb = VoieTampon[voie].traitees->max;
					j = VoieTampon[voie].traitees->prem;
				}
			}
			if(nb > 0) ajoutee->filtrees = malloc(nb * sizeof(TypeDonnee));
			if(ajoutee->filtrees) {
				ajoutee->max = nb;
				k = 0;
				while(nb--) {
					ajoutee->filtrees[k++] = VoieTampon[voie].traitees->t.rval[j++];
					if(j >= VoieTampon[voie].traitees->max) j = 0;
				}
			}
		}
		/* Pour l'archivage */
		if((filtre = VoieTampon[voie].trmt[TRMT_PRETRG].filtre)) {
			nb = filtre->nbmax;
			etg = filtre->nbetg - 1;
			suivi = &(VoieTampon[voie].trmt[TRMT_PRETRG].suivi);
//			int recul;
//			if(depuis_debut_evt <= (suivi->nb - nb)) recul = depuis_debut_evt;
//			else recul = suivi->nb - nb;
//			if(recul >= 0) {
			if(depuis_debut_evt <= (suivi->nb - nb)) {
				j = Modulo(ajoutee->debut+nb,suivi->max);
				ajoutee->recalcul = 0; 
			} else {
				if(suivi->nb < suivi->max) j = nb; else j = suivi->prems;
				ajoutee->recalcul = depuis_debut_evt - (suivi->nb - nb); 
			}
			if(j <= suivi->nb) {
				if(nb > 0) ajoutee->demarrage = (double *)malloc(nb * sizeof(double));
				if(ajoutee->demarrage) {
					ajoutee->demdim = nb; /* par souci de coherence, mais utilise seulement dans tango */
					ajoutee->nbfiltres = nb;
//					ajoutee->recalcul = depuis_debut_evt - recul;
//					if(suivi->nb < suivi->max) j = suivi->nb; else j = suivi->prems;
					while(nb--) {
						j -= 1;
						if(j < 0) j = suivi->max - 1;
						ajoutee->demarrage[nb] = suivi->filtree[etg][j];
					}
				}
			}
		}
	}
}
/* ========================================================================== */
int TrgrAjouteEvtINUTILISEE(int64 moment) {
	int64 t0,n0; int delai; int n;

	t0 = (VerifTempsPasse? DateMicroSecs(): 0); n0 = TempsTotalLect;
	TrmtNbNouveaux++;
	n = EvtASauver;
	delai = (int)(moment - TousLesEvts.stamp_trouve);
	TousLesEvts.stamp_trouve = moment;
	TousLesEvts.delai = (float)delai * EnSecondes; /* secondes car Echantillonnage en kHz */

	Evt[n].num = ++(Acquis[AcquisLocale].etat.evt_trouves);
	Evt[n].regen = 0;
	Evt[n].mode = ARCH_INDIV;
	Evt[n].delai = TousLesEvts.delai; // depuis le dernier stocke
	Evt[n].trig = TousLesEvts.delai; // (float)EvtATraiter->delai  * EnSecondes; // depuis le dernier signale
	Evt[n].gigastamp = (int)(moment/1000000000);
	Evt[n].stamp = Modulo(moment,1000000000);
	Evt[n].nbvoies = 0;
	EvtASauver++; /* apres ajout des voies car LectDebutEvts est utilise par LectActionUtilisateur() des que EvtASauver > 0 */
	EvtAffichable = n;
	LectCntl.MonitEvtNum = Evt[EvtAffichable].num; /* meme genre de remarque, cf LectDisplay */
	if(moment != TousLesEvts.stamp_utilise) {
		TousLesEvts.freq = (float)(Acquis[AcquisLocale].etat.evt_trouves - TousLesEvts.utilises)
		/ ((float)(moment - TousLesEvts.stamp_utilise) * EnSecondes);
	}
	return(n);
}
/* ========================================================================== */
void TrgrAjouteVoie(int n, int64 debut) {
}
/* ========================================================================== */
void TrmtConstruitEvt(/* int voietrig, int64 moment, float niveau */) {
	int voietrig; int64 moment; float niveau;
	int i,n,k; int voie,vt,va,phys,bolo,bolotrig,ba,bv,nbvoies,cap,nb,catg,min,max;
	int mode,delai; short tour; char une_seule_voie,regul_ok; char raison[256];
	int64 stamp_max;
	TypeVoieArchivee *triggee,*ajoutee; HistoDeVoie vh; TypeConfigDefinition *config;
	TypeCateg *categ; int tag;

	int64 t0,n0,temps_lecture;

	t0 = (VerifTempsPasse? DateMicroSecs(): 0); n0 = TempsTotalLect;
	// TrmtDbgPmax = (Acquis[AcquisLocale].etat.evt_trouves < 5) && (Acquis[AcquisLocale].etat.evt_trouves > 0);
	TrmtNbNouveaux++;
	n = EvtASauver;
	voietrig = EvtATraiter->trig;
	niveau = EvtATraiter->niveau;
	moment = EvtATraiter->date;
	delai = EvtATraiter->delai;
	bolotrig = (voietrig >= 0)? VoieManip[voietrig].det: -1;
	mode = ((bolotrig >= 0) && (Bolo[bolotrig].mode_arch != ARCH_NBMODES))?  Bolo[bolotrig].mode_arch: ArchDetecMode;
	if(VerifSuiviEtatEvts)
		printf("(%s) =====> Evt[%d] (#%d) pour %12s, en mode %s\n",__func__,n,Acquis[AcquisLocale].etat.evt_trouves+1,VoieManip[voietrig].nom,ArchModeNoms[mode]);
	if(VerifSuiviTrmtEvts) printf("(%s)        [%8lld] Evt[%d] %7g a p=%lld (%g), delai=%-+5d (precedent a p=%lld)\n",__func__,
		DernierPointTraite,n,niveau,moment,(float)moment/Echantillonnage,delai,TousLesEvts.stamp_trouve);
	if(TrmtDebug || VerifEvtMoyen || VerifSuiviTrmtEvts)
		printf("(%s)    +++ Trigger[%d] (#%d) @p=%lld, sur %s (traite a p=%lld)\n",__func__,
			   n,Acquis[AcquisLocale].etat.evt_trouves+1,moment,(voietrig >= 0)?VoieManip[voietrig].nom:"voie externe",DernierPointTraite);
	//+ if(VerifCalculTauxEvts) printf("Dernier evenement a %d\n",(int)TousLesEvts.stamp_trouve);
	voie = 0;  /* pour le compilo */
	TrmtVoieInitiale = voietrig;
/*
 * Examen de toutes les voies contributrices [l'evenement est-il complet? report eventuel]
 */
	for(phys=0; phys<ModelePhysNb+1; phys++) {
		TrgrRef[phys].est_defini = 0;
		TrgrRef[phys].amplitude = 0.0; /* amplitude en keV */
		TrgrRef[phys].date = moment;
		TrgrRef[phys].voie = voietrig;
	}
	for(va=0; va<VoiesNb; va++) { TrmtMesuree[va].num = -1; TrmtMesuree[va].mesuree = 0; }
	AmplitudeMaxi = 0.0;
	une_seule_voie = 1;
	if(Calage.prevu && (VoiesLocalesNb > 1)) while(une_seule_voie) {
		if((mode == ARCH_INDIV) || (BoloNb == 1)) {
			nb = 0;
			for(cap=0; cap<Bolo[bolotrig].nbcapt; cap++) {
				voie = Bolo[bolotrig].captr[cap].voie;
				if(VoieTampon[voie].lue > 0) nb++;
			}
			if(nb == 1) break;
		}
		une_seule_voie = 0;
	}
	if(une_seule_voie) Calage.demande = 0; else Calage.demande = Calage.prevu;  Calage.a_faire = Calage.demande;
	CalageSurTriggee = ((Calage.demande == CALAGE_UNIQUE) && (Calage.reference == ModelePhysNb));
	CalageExtreme = (CalageSurTriggee && (SettingsCalageObj == CALAGE_AMPL));
	TrmtAccepte = TrmtAccepteParDefaut;
	TrmtNonConforme = TrmtNonConformeParDefaut;
	TrmtAbandonne = 0;
	VetoMis = 0;

	/* 1.- Constitution de la liste des voies, et elaboration de la reference en temps */
	//+ printf("\n(%s) ===> Evt[%d] sur voie %s\n",__func__,n,VoieManip[voietrig].prenom);
	nbvoies = 0;
	if(!RegenEnCours) {
		if(TrmtCalageVoie(n,voietrig,nbvoies,voietrig,moment,niveau)) nbvoies++; else goto sortie;
		if(CalageExtreme) Calage.a_faire = 0;
		switch(mode) {
		  case ARCH_COINC:
		  case ARCH_TOUS: 
			for(voie=0; voie<VoiesNb; voie++) if((VoieTampon[voie].lue > 0) && (voie != voietrig)) {
				if(TrmtCalageVoie(n,voie,nbvoies,voietrig,moment,niveau)) nbvoies++; else goto sortie;
			}
			break;
		  case ARCH_VOISINS:
			bolo = bolotrig;
			if(!TrmtCalageDetecteur(bolo,n,voietrig,moment,niveau,&nbvoies)) goto sortie;
			if(ArchAssocMode) { 
				for(k=0; k<Bolo[bolo].nbassoc; k++) if((ba = Bolo[bolo].assoc[k]) >= 0) {
					if(Bolo[ba].lu != DETEC_OK) continue;
					if(!TrmtCalageDetecteur(ba,n,voietrig,moment,niveau,&nbvoies)) goto sortie;
				}
			}
			for(bv=0; bv<Bolo[bolotrig].nbvoisins; bv++) {
				bolo = Bolo[bolotrig].voisin[bv];
				if((bolo == bolotrig) || (Bolo[bolo].lu != DETEC_OK)) continue;
				if(!TrmtCalageDetecteur(bolo,n,voietrig,moment,niveau,&nbvoies)) goto sortie;
				if(ArchAssocMode) { 
					for(k=0; k<Bolo[bolo].nbassoc; k++) if((ba = Bolo[bolo].assoc[k]) >= 0) {
						if(Bolo[ba].lu != DETEC_OK) continue;
						if(!TrmtCalageDetecteur(ba,n,voietrig,moment,niveau,&nbvoies)) goto sortie;
					}
				}
			}
			break;
		  case ARCH_POSY:
			tour = CABLAGE_POSY(Bolo[bolotrig].pos);
			for(bolo=0; bolo<BoloNb; bolo++) if(CABLAGE_POSY(Bolo[bolo].pos) == tour) {
				if(Bolo[bolo].lu != DETEC_OK) continue;
				if(!TrmtCalageDetecteur(bolo,n,voietrig,moment,niveau,&nbvoies)) goto sortie;
				if(ArchAssocMode) { 
					for(k=0; k<Bolo[bolo].nbassoc; k++) if((ba = Bolo[bolo].assoc[k]) >= 0) {
						if(Bolo[ba].lu != DETEC_OK) continue;
						if(!TrmtCalageDetecteur(ba,n,voietrig,moment,niveau,&nbvoies)) goto sortie;
					}
				}
			}
			break;
		  case ARCH_INDIV:  /* Aussi utilise par defaut */
		  default:
			if(!TrmtCalageDetecteur(bolotrig,n,voietrig,moment,niveau,&nbvoies)) goto sortie;
			if(ArchAssocMode) { 
				for(k=0; k<Bolo[bolotrig].nbassoc; k++) if((ba = Bolo[bolotrig].assoc[k]) >= 0) {
					if(!TrmtCalageDetecteur(ba,n,voietrig,moment,niveau,&nbvoies)) goto sortie;
				}
			}
			break;
		}
	} else /* if(RegenEnCours) */ {
		if(!TrmtCalageDetecteur(bolotrig,n,voietrig,moment,niveau,&nbvoies)) goto sortie;
		if(ArchAssocMode) { 
			for(k=0; k<Bolo[bolotrig].nbassoc; k++) if((ba = Bolo[bolotrig].assoc[k]) >= 0) {
				if(!TrmtCalageDetecteur(ba,n,voietrig,moment,niveau,&nbvoies)) goto sortie;
			}
		}
	}
	/* 2.- Caracteristiques pour toutes les voies de la liste */
	for(va=0; va<nbvoies; va++) {
		TypeVoieArchivee *en_cours;
		en_cours = &(TrmtMesuree[va]); // &(Evt[n].voie[va]); amsi en mode COINC, on peut ne pas garder une voie si < seuil
		voie = en_cours->num;
		if(en_cours->stream) {
			if(Calage.demande) {
				int ref_cal,voie_ref; int64 compactage;

				compactage = (int64)VoieTampon[voie].trmt[TRMT_AU_VOL].compactage;
				if(Calage.demande == CALAGE_UNIQUE) ref_cal = Calage.reference; else ref_cal = ModeleVoie[VoieManip[voie].type].physique;
				voie_ref = TrgrRef[ref_cal].voie;
				en_cours->date_locale = (TrgrRef[ref_cal].date + VoieTampon[voie].decal - VoieTampon[voie_ref].decal) / compactage;
				en_cours->debut = en_cours->date_locale - (int64)VoieEvent[voie].avant_evt;
				if(voie != voie_ref) {
					int64 premier,dernier,debut_mini,moment_maxi;
					premier = VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien; /* points stockes */
					dernier = LectStampsLus; /* pas VoieTampon[voie].lus car on regarde les pts absolus */
					/* printf("(%s) Date de reference[%d] (%8s) : %12lld\n",__func__,ref_cal,VoieManip[voie_ref].prenom,TrgrRef[ref_cal].date);
					printf("(%s) - antidecalage de     %8s  : %12d\n",__func__,VoieManip[voie_ref].prenom,VoieTampon[voie_ref].decal);
					printf("(%s) + decalage de         %8s  : %12d\n",__func__,VoieManip[voie].prenom,VoieTampon[voie].decal);
					printf("(%s) = Date locale         %8s  : %12lld\n\n",__func__,VoieManip[voie].prenom,en_cours->date_locale * compactage); */
					debut_mini = en_cours->debut;
					if(debut_mini < premier) {
						if(debut_mini >= 0) TrmtTropTard++;
						if(TrmtTropTard) {
							printf("%s/ Traitement hors delai (#%d) pour la voie %s pour l'evenement #%d @%lld!\n",
								   DateHeure(),TrmtTropTard,VoieManip[voie].nom,Acquis[AcquisLocale].etat.evt_trouves,moment);
							printf("          . Recherche a partir de %lld, tampon entre %lld et %lld (%d points/%lld)\n",
								   debut_mini*compactage,premier*compactage,dernier,(int)(dernier-(premier*compactage)),compactage);
							TrmtAbandonneEvts(premier * compactage,"recherche du maximum");
						} else {
							if(TrmtDebug || VerifEvtMoyen || VerifSuiviTrmtEvts)
								printf("(%s) Debut de la voie <%s> a p=%d, avant le premier=%d (stockes) => abandon\n",__func__,VoieManip[voie].nom,(int)debut_mini,(int)premier);
						}
						TrmtAbandonne = 1;
						if(TrmtDbgPmax) printf("(%s) %s evt %d: debut_mini < premier, abandon\n",__func__,VoieManip[voie].nom,n);
						goto sortie;
					}
					moment_maxi = (en_cours->date_locale * compactage) + (int64)VoieEvent[voie].apres_evt;  /* moment_maxi evt en points absolus */
					if(moment_maxi > dernier) {
						EvtATraiter->fin = moment_maxi;
						if(TrmtDebug || VerifEvtMoyen || VerifSuiviTrmtEvts)
							printf("(%s) [%8lld] Fin de la voie %s a p=%lld (au dela du dernier=%lld) => report\n",__func__,
								   DernierPointTraite,VoieManip[voie].nom,moment_maxi,dernier);
						TrmtAbandonne = -1;
						if(TrmtDbgPmax) printf("(%s) %s evt %d: moment_maxi > dernier, abandon\n",__func__,VoieManip[voie].nom,n);
						goto sortie;
					}
					if(SettingsCalageObj == CALAGE_AMPL) {
						TypeTamponDonnees *tampon; int lngr; int adrs_mini,k; float niv0,dev,rms;
						tampon = VoieTampon[voie].brutes; lngr = tampon->max;
						adrs_mini = Modulo(debut_mini,lngr);
						// printf("(%s) %7s: debut impose = %lld\n",__func__,VoieManip[voie].prenom,debut_mini);
						niv0 = TrmtCalculeOf7(voie,tampon,lngr,1,adrs_mini,&rms);
						if(VerifCalculEvts) printf("(%s<TrmtCalculeOf7) Base = %g +/- %g\n",__func__,niv0,rms);
						k = Modulo(en_cours->date_locale,lngr);
						dev = ((tampon->type == TAMPON_INT16)? (float)tampon->t.sval[k]: ((tampon->type == TAMPON_INT32)? (float)tampon->t.ival[k]: tampon->t.rval[k])) - niv0;
						en_cours->val[MONIT_BASE] = niv0;
						en_cours->val[MONIT_BRUIT] = rms;
						en_cours->val[MONIT_AMPL] = dev;
						en_cours->val[MONIT_SEUIL] = rms;
						en_cours->mesuree = 1;
					}
				}
			} else en_cours->debut = en_cours->date_locale - (int64)VoieEvent[voie].avant_evt;
			en_cours->adresse = Modulo(en_cours->debut,VoieTampon[voie].brutes->max);
		}
		if(!en_cours->mesuree) {
			printf("%s/ !!! Evenement #%d: voie %s sans mesure\n",DateHeure(),
				Acquis[AcquisLocale].etat.evt_trouves+1,VoieManip[voie].nom);
			Acquis[AcquisLocale].etat.active = 0;
			goto sortie;
		}
		if(!TrmtCoupures(en_cours)) break; // goto sortie;
		if(voie == TrmtVoieInitiale) en_cours->depasse_le_seuil = 1;
		else en_cours->depasse_le_seuil = TrmtSeuilDepasse(en_cours,en_cours->val[MONIT_BASE],en_cours->val[MONIT_AMPL],0);
	}
	if(VetoMis && VerifSuiviTrmtEvts) printf("(%s) ?! Veto mis?\n",__func__);
	if(!TrmtAccepte || TrmtNonConforme || VetoMis) goto sortie;

/*
 * Examen passe avec brio. L'evenement est accepte!
 */
	if(TrmtDebug || VerifEvtMoyen || VerifSuiviTrmtEvts) 
		printf("(%s)    --- Trigger[%d] (#%d) @p=%lld, accepte pour %s\n",
			  __func__,n,Acquis[AcquisLocale].etat.evt_trouves+1,moment,(voietrig >= 0)?VoieManip[voietrig].nom:"voie externe");
	// printf("(%s) Post-controle: montee=%g avec maxi=%g\n",__func__,TrmtMesuree[0].montee,VoieManip[voietrig].trgr.condition.montmax);
#ifdef HISTOS_DELAIS_EVT
	{
		int k;
		k = (delai + 500) / 1000;
		if(k < 0) k = 0; else if(k >= MAXHISTOTRIG) k = MAXHISTOTRIG - 1;
		HistoEvts[k] += 1;
	}
#endif
	if(TrmtDeconvolue) {
		// Utiliser SambaDeconvolue(), voir dans TrmtCalageVoie() avec calcule=0
		// Recherche evts au dessus d'un seuil
		// boucle ci-dessous pour chaque evt trouve
		// parametres: seuil relatif, temps mort, longueur a sauver (tous voie-dependants en principe)
	}
	Evt[n].num = ++(Acquis[AcquisLocale].etat.evt_trouves);
	Evt[n].regen = RegenEnCours;
	Evt[n].mode = mode;
	if(Calage.demande) {
		phys = ModeleVoie[VoieManip[voietrig].type].physique;
		voietrig = TrgrRef[phys].voie;
		bolotrig = VoieManip[voietrig].det;
		if(VerifSuiviTrmtEvts) printf("(%s)        Nouvelle voie trig: %s\n",__func__,VoieManip[voietrig].nom);
	}
	if(VerifSuiviTrmtEvts) {
		if(voietrig != TrmtVoieInitiale) printf("(%s) Evt #%d: voie triggee initialement=%d, retenue=%d\n",__func__,Evt[n].num,TrmtVoieInitiale,voietrig);
	}
	Evt[n].voie_hdr = VoieManip[voietrig].num_hdr[EVTS];
	Evt[n].bolo_hdr = Bolo[bolotrig].num_hdr;
	regul_ok = (Modulo(moment/LectDureeD3,TrgrRegulClock) > TrgrRegulDelai);
	if(TrmtRegulActive && !regul_ok) printf("%s/ L'evenement #%d date a %lld n'est pas utilise pour la regulation\n",DateHeure(),Evt[n].num,moment);
	for(i=0; i<EVT_BITTRIGGER_MAX; i++) Evt[n].liste[i] = 0;
	Evt[n].nbvoies = 0;
	if(mode == ARCH_COINC) {
		char touche[DETEC_MAX];
		for(i=0; i<DETEC_MAX; i++) touche[i] = 0;
		for(va=0; va<nbvoies; va++) if(TrmtMesuree[va].depasse_le_seuil) {
			touche[VoieManip[TrmtMesuree[va].num].det] = 1;
		}
		for(va=0; va<nbvoies; va++) if(touche[VoieManip[TrmtMesuree[va].num].det]) TrmtAjouteVoie(n,va,moment,regul_ok);
	} else for(va=0; va<nbvoies; va++) TrmtAjouteVoie(n,va,moment,regul_ok);
	//+ printf("(%s) <=== Decalage=%lld\n",__func__,TrmtMesuree[1].debut-TrmtMesuree[0].debut);
	Evt[n].voie_evt = 0; /* par securite informatique */
	for(vt=0; vt<Evt[n].nbvoies; vt++) {
		if(VerifSuiviTrmtEvts) printf("(%s)        Regarde si l'ajout#%d (%s) est la voie triggee (%s)\n",__func__,
			vt+1,VoieManip[Evt[n].voie[vt].num].nom,VoieManip[voietrig].nom);
		if(Evt[n].voie[vt].num == voietrig) { Evt[n].voie_evt = vt; break; }
	}
	vt = Evt[n].voie_evt; triggee = &(Evt[n].voie[vt]); voie = triggee->num;
	if(VerifSuiviTrmtEvts) printf("(%s)        Voie trig = ajout#%d, pointe sur la voie #%d (%s)\n",__func__,
		vt+1,triggee->num,VoieManip[voie].nom);

	if(VoieManip[voie].duree.deconv.calcule) {
		min = (int)(-triggee->val[MONIT_BRUIT] - 0.5);
		max = (int)(triggee->val[MONIT_BASE] + triggee->val[MONIT_TOP] + 0.5);
	} else {
		min = (int)(triggee->val[MONIT_BASE] - triggee->val[MONIT_BRUIT] - 0.5);
		max = (int)(triggee->val[MONIT_BASE] + triggee->val[MONIT_AMPL] + 0.5);
	}
	MonitEvtAmpl = triggee->val[MONIT_AMPL];
	MonitEvtMontee = triggee->val[MONIT_MONTEE];
	Evt[n].stamp_ref = triggee->stamp_ref;
	SambaTempsEchantillon(Evt[n].stamp_ref,ArchT0sec,ArchT0msec,&(Evt[n].sec),&(Evt[n].msec));
	SambaTempsEchantillon(LectStampJetes,0,0,&(Evt[n].TMsec),&(Evt[n].TMmsec));
	stamp_max = Evt[n].stamp_ref + LectTimeStamp0;
	Evt[n].gigastamp = (int)(stamp_max/1000000000);
	Evt[n].stamp = Modulo(stamp_max,1000000000);
	if(SettingsDeconvPossible && VoieManip[voie].duree.deconv.calcule && VoieTampon[voie].deconvoluee) {
		Evt[n].deconvolue = SambaAssure(Evt[n].deconvolue,VoieEvent[voie].lngr,&(Evt[n].deconv_max));
		if(Evt[n].deconvolue) memcpy((void *)Evt[n].deconvolue,(void *)VoieTampon[voie].deconvoluee,VoieEvent[voie].lngr*sizeof(float));
	}
	/* Normalement, TousLesEvts.trouves == Acquis[AcquisLocale].etat.evt_trouves */
	TrmtCategStore(&TousLesEvts,n,min,max,triggee->val[MONIT_TOP],
		triggee->val[MONIT_CHARGE],triggee->val[MONIT_DISPER],
		triggee->val[MONIT_PRELIM],triggee->val[MONIT_AVANCE]);
	config = &(VoieManip[voietrig].def);
#ifdef SANS_SELECTION
	for(k=0; k<config->nbcatg; k++) if(TrmtPasseConditions(triggee,&(config->catg[k].condition),raison)) break;
#else
	for(k=0; k<config->nbcatg; k++) if(TrmtPasseCriteres(triggee,&(config->catg[k].definition),raison)) break;
#endif
	if(k < config->nbcatg) {
		char sonorisee;
		categ = &(config->catg[k]);
		strcpy(Evt[n].nom_catg,categ->nom); Evt[n].categ = tag = categ->num;
		if(tag <= TagDefNb) {
			Evt[n].tag.num = TagDef[tag].p;
			Evt[n].tag.r = TagDef[tag].c.r; Evt[n].tag.g = TagDef[tag].c.g; Evt[n].tag.b = TagDef[tag].c.b;
			sonorisee = TagDef[tag].sonorisee;
		} else sonorisee = 0;
		catg = categ->monit; // vraie numero de categorie
		TrmtCategStore(&(MonitCatg[catg]),n,min,max,triggee->val[MONIT_TOP],
			triggee->val[MONIT_CHARGE],triggee->val[MONIT_DISPER],
			triggee->val[MONIT_PRELIM],triggee->val[MONIT_AVANCE]);
		if(MonitModeSon && ((MonitCategSonore == 0) || (MonitCategSonore == (catg + 1))) && sonorisee) {
			char commande[MAXFILENAME]; int64 lancement;
			lancement = DateMicroSecs();
			if(lancement > TagPlayEnd) {
				TagPlayEnd = lancement + TagDef[tag].duree_us;
				sprintf(commande,"afplay %s%s &",PrefPath,TagDef[tag].son);
				system(commande);
			}
		}
	} else {
		//++? printf("(%s) Evt[%d]: %s\n",__func__,Evt[n].num,"hors categorie");
		strcpy(Evt[n].nom_catg,"lambda"); /* Evt[n].nom_catg[0] = '\0'; */ Evt[n].categ = -1; Evt[n].tag.num = TrmtCategIndefinie;
		Evt[n].tag.r = Evt[n].tag.g = Evt[n].tag.b = WND_COLORMAX;
		catg = -1;
	}
	catg++; // index de MonitCatgPtr
	Evt[n].trig = (float)EvtATraiter->delai * EnSecondes; // depuis le dernier signale
	Evt[n].delai = TousLesEvts.delai; // depuis le dernier stocke
	EvtASauver++; /* apres ajout des voies car LectDebutEvts est utilise par LectActionUtilisateur() des que EvtASauver > 0 */
	EvtAffichable = n;
	/* Valeur utilisee des qu'il y a au moins un evenement */
	LectCntl.MonitEvtNum = Evt[EvtAffichable].num; /* meme genre de remarque, cf LectDisplay */
//	EvtATraiter->trig = -1; EvtATraiter->fin = -1;
	if(VerifSuiviEtatEvts)
		printf("(%s) <===== Evt[%d] (#%d) pour %12s, detecte a %lld, max a %lld, termine a p=%lld\n",
		__func__,n,Evt[n].num,VoieManip[voietrig].nom,moment,Evt[n].stamp_ref,DernierPointTraite);
	if(TrmtDebug || VerifEvtMoyen || VerifSuiviTrmtEvts) 
		printf("(%s)  |==== Trigger[%d] (#%d) @p=%lld, attribue a <%s> pour p=%lld\n",
			   __func__,n,Evt[n].num,moment,(voietrig >= 0)?VoieManip[voietrig].nom:"voie externe",Evt[n].stamp_ref);
	
/*
 * Post-traitements divers
 */
//	voietrig = triggee->num;
//	printf("%4d %-16s %7.1f %+7.1f @%6d\n",
//		Evt[n].num,VoieManip[voietrig].nom,triggee->val[MONIT_BASE],triggee->val[MONIT_AMPL],
//		(int)triggee->debut+VoieTampon[voietrig].avant_evt);
    
#ifdef BCP_EVTS
	/* zones de non-evenement, c.a.d de bruit */
	evtend = triggee->debut + (int64)(triggee->dim);
	t1 = (double)evtend * (double)VoieTampon[voietrig].horloge / 1000.0;   /* secondes */
	if(CalcNb == 0) CalcTi[CalcNb++] = t1;
	else if(CalcMax == 1) CalcTi[0] = t1;
	else {
		t0prec = (int64)(CalcTi[CalcNb - 1] * 1000.0 / (double)VoieTampon[voietrig].horloge);
		while((moment - t0prec) > CalcDim) {
			if(CalcNb < CalcMax) CalcTi[CalcNb++] = t1;
			else {
				for(k=0; k<(CalcNb-1); k++) CalcTi[k] = CalcTi[k+1];
				CalcTi[CalcNb - 1] = t1;
			}
		}
		else CalcTi[CalcNb - 1] = t1;
	}
#endif

	/* Partage de l'evenement avec les systemes en reseau */
#ifdef PARMS_RESEAU
	if(SambaMode != SAMBA_MONO) {
		int i,j;
		TypeDonnee *ptr_donnee;

		EvtPartage->num = Evt[n].num;
		EvtPartage->voie = voietrig;
		EvtPartage->bolo = VoieManip[voietrig].det;
		EvtPartage->amplitude = triggee->val[MONIT_AMPL];
		EvtPartage->lngr = triggee->dim;
		if(EvtPartage->lngr > MAXEVTPARTAGE) EvtPartage->lngr = MAXEVTPARTAGE;
		j = triggee->adresse;
		ptr_donnee = VoieTampon[voietrig].brutes->t.sval + j;
		for(i=0; i<EvtPartage->lngr; i++) {
			EvtPartage->val[i] = *ptr_donnee++;
			if(++j >= VoieTampon[voietrig].brutes->max) {
				j = 0;
				ptr_donnee = VoieTampon[voietrig].brutes->t.sval;
			}
		}
	}
#endif
			
	/* Histogrammes 'bruit' */
	if(voietrig == SettingsHistoVoie) {
//		if(TrmtHampl) HistoFillFloatToInt(hdEvtAmpl,fabsf(triggee->val[�]),1);
		if(TrmtHampl) HistoFillFloatToInt(hdEvtAmpl,triggee->val[MONIT_AMPL],1);
		if(TrmtHmontee) HistoFillFloatToInt(hdEvtMontee,triggee->val[MONIT_MONTEE],1);
		if(TrmtH2D) H2DFillFloatFloatToInt(h2D,fabsf(triggee->val[MONIT_AMPL]),triggee->val[MONIT_MONTEE],1);
	}
	/* Histogrammes utilisateurs */
	for(vt=0; vt<Evt[n].nbvoies; vt++) {
		voie = Evt[n].voie[vt].num;
		if(VoieTampon[voie].lue > 0) {
			ajoutee = &(Evt[n].voie[vt]);
			vh = VoieHisto[voie];
			while(vh) {
				if((vh->catg == 0) || (vh->catg == catg)) {
					if(vh->D == 1) {
						if(vh->Xvar == MONIT_ABS) HistoFillFloatToInt(vh->hd,Abs(ajoutee->val[MONIT_AMPL]),1);
						else HistoFillFloatToInt(vh->hd,ajoutee->val[vh->Xvar],1);
					} else if(vh->D == 2) {
						int vy; TypeVoieArchivee *ordonnee; float xval,yval;
						for(vy=0; vy < Evt[n].nbvoies; vy++) if(Evt[n].voie[vy].num == vh->Yvoie) {
							ordonnee = &(Evt[n].voie[vy]);
							xval = (vh->Xvar == MONIT_ABS)? Abs(ajoutee->val[vh->Xvar]): ajoutee->val[vh->Xvar];
							yval = (vh->Yvar == MONIT_ABS)? Abs(ordonnee->val[vh->Yvar]): ordonnee->val[vh->Yvar];
							H2DFillFloatFloatToInt((H2D)(vh->hd),xval,yval,1);
							break;
						}
					}
				}
				vh = vh->suivant;
			}
		}
	}
	/* MonitNtp de nanopaw (voies du detecteur qui a trigge) */
	// printf("(%s) Evt #%d: Ntp demande niveau %d, %d/%d ntp deja crees\n",__func__,n,CalcNtpDemande,NtUsed,NtDim);
#ifdef NTUPLES_ONLINE
	if(CalcNtpDemande && (NtUsed < NtDim)) {
	// EvtDateMesuree[evt] = ProdIntervalle(ArchEvt.sec,ArchEvt.msec,EvtRunRef[0].t0sec,EvtRunRef[0].t0msec);
	// avec: float ProdIntervalle(int t1sec, int t1msec, int t0sec, int t0msec) { return((float)(t1sec - t0sec) + ((float)(t1msec - t0msec) / 1000000.0)); }
	// voir: SambaTempsEchantillon(stamp_max,ArchT0sec,ArchT0msec,&(Evt[n].sec),&(Evt[n].msec));
		if(BolosUtilises > 1) NtDet[NtUsed] = (float)bolotrig;
		for(va=0; va<Evt[n].nbvoies; va++) {
			int var_num,ntp_type;
			ajoutee = &(Evt[n].voie[va]);
			voie = ajoutee->num;
			if(VoieManip[voie].det != bolotrig) continue;
			if(VoieTampon[voie].lue <= 0) continue;
			if(CalcNtpDemande == NTP_MODELE) {
				var_num = VoieManip[voie].type; // cad vm
				if(!ModeleVoie[var_num].utilisee) continue;
			} else var_num = voie; /* (CalcNtpDemande == NTP_VOIE) */
			for(ntp_type=0; ntp_type<MONIT_NBVARS; ntp_type++) if(MonitNtpCalcule[ntp_type]) {
				// printf("(%s) Evt #%d: %s = %g\n",__func__,n,MonitNtp[MonitNtpNum[ntp_type][var_num]].nom,ajoutee->val[ntp_type]);
				NtVoie[ntp_type][var_num][NtUsed] = ajoutee->val[ntp_type];
			}
		}
		NtDate[NtUsed] = (float)Evt[n].stamp_ref / (Echantillonnage * 1000.0);
		NtDelai[NtUsed] = Evt[n].delai;
		k = Evt[n].categ; if((k >= 0) && (k < TagDefNb)) {
			PlotNtupleMarque(EvtEspace,NtUsed,TagDef[k].p);
			/* PlotNtupleColore(EvtEspace,NtUsed,TagDef[k].c.r,TagDef[k].c.g,TagDef[k].c.b);
			ou bien couleur selon voie, de toutes facons pas encore exploite */
		} else PlotNtupleMarque(EvtEspace,NtUsed,TrmtCategIndefinie);
		NtUsed++;
	}
#endif
	if(VerifBittrigger && (Evt[n].liste[0] == 0)) {
		voie = TrgrRef[Calage.reference].voie;
		printf("(%s) Evt[%d]: calage voie#%02d         ={%g %g} avec amplitude=%.2f+%.2f=%.2f a t=%d\n",__func__,
			n,voie,VoieTampon[voie].trig.trgr->maxampl,VoieTampon[voie].trig.trgr->minampl,
			triggee->val[MONIT_BASE],triggee->val[MONIT_AMPL],triggee->val[MONIT_BASE]+triggee->val[MONIT_AMPL],(int)TrgrRef[Calage.reference].date);
		for(va=0; va<Evt[n].nbvoies; va++) {
			float fval; char depasse_le_seuil;
			ajoutee = &(Evt[n].voie[va]);
			voie = ajoutee->num;
			fval = ajoutee->val[MONIT_AMPL]; if(VoieTampon[voie].trig.trgr->type == TRGR_SEUIL) fval += ajoutee->val[MONIT_BASE];
			depasse_le_seuil = TrmtSeuilDepasse(ajoutee,ajoutee->val[MONIT_BASE],ajoutee->val[MONIT_AMPL],-1);
			if(depasse_le_seuil < 0) printf("(%s) Evt[%d]:        voie#%02d (%2d/%-2d) pas examinee\n",__func__,n,voie,va,Evt[n].nbvoies);
			else {
				printf("(%s) Evt[%d]: seuil  voie#%02d (%2d/%-2d) = {%g %g} %13s %.2f+%.2f=%.2f",__func__,
					n,voie,va,Evt[n].nbvoies,VoieTampon[voie].trig.trgr->maxampl,VoieTampon[voie].trig.trgr->minampl,
					depasse_le_seuil? "depasse par  ": "plus haut que");
				if(depasse_le_seuil) printf(" (bit %d, masque %08X)\n",VoieManip[voie].det,(1 << (VoieManip[voie].det % 32)));
				else printf("\n");
			}
		}
		printf("(%s) Evt[%d]: #%d, declenche sur voie#%02d @%g a t=%d (#%d dans le ntp), et Bit Trigger a 0\n",__func__,
			n,Evt[n].num,TrmtVoieInitiale,niveau,(int)moment,VoieManip[TrmtVoieInitiale].num_hdr[EVTS]);
		// Acquis[AcquisLocale].etat.active = 0;
	}
	
	/* Il est peut-etre maintenant urgent de vider tout ca sur fichier... */
	if(SettingsPauseEvt) SettingsPauseEvt = OpiumChoix(2,SambaNonOui,"Evenement #%d",EvtASauver);
//	if(EvtASauver >= 5) Acquis[AcquisLocale].etat.active = 0;
	if(EvtASauver >= EvtNbMax) ArchiveVide(ArchSauve[EVTS],"Max evts");

sortie:
	if(VetoMis) {
		if(VerifSuiviTrmtEvts) printf("(%s) Evt[%d] (#%d) a p=%lld coupe par veto\n",__func__,n,Acquis[AcquisLocale].etat.evt_trouves+1,moment);
	}
	if(TrmtAbandonne > 0) {
		if(TrmtTropTard) {
			TrmtNbEchecs++;
			if(VerifSuiviTrmtEvts) printf("(%s) Evt[%d] (#%d) a p=%lld abandonne lors de sa datation (echec #%d)\n",__func__,n,Acquis[AcquisLocale].etat.evt_trouves+1,moment,TrmtNbEchecs);
			if(VerifSuiviEtatEvts) printf("(%s) <===== [%8lld] Evt[%d] (#%d) perdu   a p=%lld\n",
				__func__,DernierPointTraite,n,Acquis[AcquisLocale].etat.evt_trouves+1,moment);
		} else {
			TrmtNbPremat++;
			if(VerifSuiviTrmtEvts) printf("(%s) Evt[%d] (#%d) a p=%lld abandonne lors de sa datation (premature #%d)\n",__func__,n,Acquis[AcquisLocale].etat.evt_trouves+1,moment,TrmtNbPremat);
			if(VerifSuiviEtatEvts) printf("(%s) <===== [%8lld] Evt[%d] (#%d) precoce a p=%lld\n",
				__func__,DernierPointTraite,n,Acquis[AcquisLocale].etat.evt_trouves+1,moment);
		}
	} else if(TrmtAbandonne < 0) {
		TrmtNbRelances++;
		if(VerifSuiviTrmtEvts) printf("(%s) Evt[%d] (#%d) a p=%lld non traite lors de sa datation\n",__func__,n,Acquis[AcquisLocale].etat.evt_trouves+1,moment);
		if(VerifSuiviEtatEvts) printf("(%s) <===== [%8lld] Evt[%d] (#%d) reporte a p=%lld\n",
			__func__,DernierPointTraite,n,Acquis[AcquisLocale].etat.evt_trouves+1,moment);
	} else if(!TrmtAccepte) {
		EvtNbCoupes++;
		if(VerifSuiviEtatEvts) printf("(%s) <===== [%8lld] Evt[%d] (#%d) coupe a p=%lld\n",
									  __func__,DernierPointTraite,n,Acquis[AcquisLocale].etat.evt_trouves+1,moment);
	} else if(TrmtNonConforme) {
		EvtNbIncoherents++;
		if(VerifSuiviEtatEvts) printf("(%s) <===== [%8lld] Evt[%d] (#%d) non conforme a p=%lld\n",
									  __func__,DernierPointTraite,n,Acquis[AcquisLocale].etat.evt_trouves+1,moment);
	}

	temps_lecture = TempsTotalLect - n0;
	TrmtTempsEvt += (VerifTempsPasse? DateMicroSecs() - t0 - temps_lecture: 0);
	if(TrmtAbandonne >= 0) {
		if(EvtsEmpiles) {
			int i;
			EvtsEmpiles--; for(i=0; i<EvtsEmpiles; i++) memcpy(&(EvtStocke[i]),&(EvtStocke[i+1]),sizeof(TypeEvtDetecte));
			if(TrmtDebug || VerifEvtMoyen || VerifSuiviTrmtEvts)
				printf("(%s)   | Evt[%d] (#%d) traite, il en reste %d. %s sur <%s> a p=%lld, fin fixee a p=%lld\n",__func__,
					EvtASauver-1,Acquis[AcquisLocale].etat.evt_trouves,EvtsEmpiles,(EvtsEmpiles>1)?"Le prochain est":"Celui-ci etait",
					VoieManip[EvtATraiter->trig].nom,EvtATraiter->date,EvtATraiter->fin);
		} else if(VerifSuiviTrmtEvts) printf("(%s)   | Evt[%d] (#%d) traite, pas d'autre evenement pour le moment\n",__func__,
											 EvtASauver-1,Acquis[AcquisLocale].etat.evt_trouves);
	}
	return;
}
#ifdef ALTIVEC
	#ifdef FILTRE_DOUBLE
		#define produit_vectoriel dotprD
	#else
		#define produit_vectoriel dotpr
	#endif
/* ========================================================================== */
TypeDonneeFiltree TrmtFiltreAltivec(TypeDonneeFiltree *entree, TypeSuiviFiltre *suivi,
	int nbdata, int n, TypeFiltre *filtre, TypeDonneeFiltree brute) {
	TypeCelluleFiltrante *cellule; TypeDonneeFiltree filtree,coef,*vec1,*vec2,produit;
	int m,i; int etage;

	filtree = *entree;
	for(etage=0; etage<filtre->nbetg; etage++) {
		cellule = &(filtre->etage[etage]);
		if(nbdata >= filtre->nbmax) {
			if(cellule->nbdir > 0) {
				coef = (TypeDonneeFiltree)(cellule->direct[cellule->nbdir-1]);
				filtree = coef * *entree;
			} else filtree = 0.0;
			m = n - 1; if(m < 0) m = suivi->max - 1;
			// Coefficients directs
			if(m >= (cellule->nbdir - 2)) {  // un produit scalaire
				vec1= cellule->direct;
				vec2= suivi->filtree[etage] + m - (cellule->nbdir-2);
				produit_vectoriel(vec1,1,vec2,1,&produit,cellule->nbdir-1);
				filtree += produit;
			} else { // deux produits vectoriels
				vec1=cellule->direct+cellule->nbdir-m-2;
				vec2=suivi->filtree[etage];
				produit_vectoriel(vec1,1,vec2,1,&produit,m+1);
				filtree += produit;
				vec1= cellule->direct;
				vec2= suivi->filtree[etage] + suivi->max - 1 - cellule->nbdir+m+3;
				produit_vectoriel(vec1,1,vec2,1,&produit,cellule->nbdir-m-2);
				filtree += produit;
			}
			// Coefficients inverses
			if(m>=cellule->nbinv-1) {     // un produit vectoriel
				vec1= cellule->inverse;
				vec2= suivi->filtree[etage+1]+ m-(cellule->nbinv-1);
				produit_vectoriel(vec1,1,vec2,1,&produit,cellule->nbinv);
				filtree -= produit;
			} else { // deux produits vectoriels
				vec1= cellule->inverse+cellule->nbinv-m-1;
				vec2= suivi->filtree[etage+1];
				produit_vectoriel(vec1,1,vec2,1,&produit,m+1);
				filtree -= produit;
				vec1= cellule->inverse;
				vec2= suivi->filtree[etage+1] + suivi->max - 1 -cellule->nbinv+m+2 ;
				if(TrmtDataNb < 300) for(i=cellule->nbinv-m-2; i>=0; i--) {
					printf("%8.3f %8.3f \n",(float)vec1[i],(float)vec2[i]);
				}
				produit_vectoriel(vec1,1,vec2,1,&produit,cellule->nbinv-m-1);
				filtree -= produit;
			}
			*entree = filtree;
			// *entree = (TypeDonneeFiltree)((int)((filtree >= 0.0)? filtree + 0.5: filtree - 0.5));
		} else filtree = *entree;
		if(etage == 0) suivi->brute[n] = brute;
		if(etage == 0) suivi->filtree[etage][n] = (TypeDonneeFiltree)val32;
		/*	suivi->filtree[etage][n] = filtree; */
		suivi->filtree[etage+1][n] = *entree;
	}

	return(filtree);
}
#endif /* ALTIVEC */
/* ========================================================================== */
TypeDonneeFiltree TrmtFiltreStd(TypeDonneeFiltree *entree, TypeSuiviFiltre *suivi,
	int nbdata, int n, TypeFiltre *filtre, TypeDonneeFiltree brute) {
	TypeCelluleFiltrante *cellule; TypeDonneeFiltree filtree;
	int m,i; int etage;

	filtree = *entree;
	for(etage=0; etage<filtre->nbetg; etage++) {
		cellule = &(filtre->etage[etage]);
		if(nbdata >= filtre->nbmax) {
			if(cellule->nbdir > 0) filtree = cellule->direct[cellule->nbdir-1] * *entree;
			else filtree = 0.0;
			if(etage == 0) for(i=cellule->nbdir-2,m=n; i>=0; i--) { /* X = donnees brutes */
				m -= 1; if(m < 0) m = suivi->max - 1;
				filtree += (cellule->direct[i] * suivi->brute[m]);
			} else for(i=cellule->nbdir-2,m=n; i>=0; i--) { /* X = resultat cellule precedente */
				m -= 1; if(m < 0) m = suivi->max - 1;
				filtree += (cellule->direct[i] * suivi->filtree[etage-1][m]);
			}
			for(i=cellule->nbinv-1,m=n; i>=0; i--) {
				m -= 1; if(m < 0) m = suivi->max - 1;
				filtree -= (cellule->inverse[i] * suivi->filtree[etage][m]);
			}
			*entree = filtree;
		} else filtree = *entree;
		if(etage == 0) suivi->brute[n] = brute;
		suivi->filtree[etage][n] = *entree;
	}

	return(filtree);
}
/* ========================================================================== */
TypeDonneeFiltree TrmtFiltreTemplate(TypeDonneeFiltree *entree, TypeSuiviFiltre *suivi, int voie,
	int nbdata, int n, TypeFiltre *filtre, int *nb_convol) {
	float *template; int m,i,dim; TypeDonneeFiltree filtree;

	filtree = *entree;
	if(VoieManip[voie].def.trmt[TRMT_PRETRG].template && VoieTampon[voie].unite.val) {	// ajout etage de filtrage par evt unite;
		dim = VoieTampon[voie].unite.arrivee - VoieTampon[voie].unite.depart;
		template = VoieTampon[voie].unite.val + VoieTampon[voie].unite.depart;
		if(nbdata >= dim) { // taille suivi superieure a taille template !
			if(dim > 0) filtree = template[dim-1] * *entree; else filtree = 0.0; *nb_convol += 1;
			for(i=dim-2,m=n; i>=0; i--) {
				m -= 1; if(m < 0) m = suivi->max - 1;
				filtree += (template[i] * suivi->filtree[filtre->nbetg-1][m]);
				*nb_convol += 1;
			}
			filtree *= VoieTampon[voie].norme_evt;
			*entree = filtree; // pas vraiment utile si on s'arrete la pour le filtrage
		}
	}

	return(filtree);
}
/* ========================================================================== */
static void TrmtMesureCpu(char *moment, int64 t2, int *nbptr) {
	int64 temps_utilise;

	temps_utilise = (VerifTempsPasse? DateMicroSecs(): 0) - t2;
	if(nbptr) {
		int nb_voies;
		nb_voies = *nbptr;
		printf("          ! Duree traitmt: %14s @%lld: %lld us (%.1f us/voie * %d voie%s)\n",
		   moment,DernierPointTraite,temps_utilise,(float)temps_utilise/(float)nb_voies,Accord1s(nb_voies));
	} else printf("          ! Duree traitmt: %14s @%lld: %lld us\n",
		moment,DernierPointTraite,temps_utilise);
}
/* ========================================================================== */
void TrmtSurBuffer() {
	int voie,vt,vm,phys,bolo; char trmt;
	TypeLarge val32,courante;
	double reelle,valDD;
	int a_sauver; char log;
#ifdef COMPENSATION_PAR_MESURE
	TypeCompensation *comp;
	TypeADU donnee;
	int rep;
	float p,total,difference;
#endif
	int j,k;
//	int nb,avance;
	char inhibe_en_cours,saute_evt,evt_trouve,mesure_globale,mesure_avant,mesure_tres_fine;
	int64 points_dispo,point_stocke,point_vire,debut_mesures,limite_fine;
	int64 t0,t1,t2,t3,t4,t5,tt,n0,n1,n2,n3,temps_lecture,temps_utilise;
	int64 vrai_dernier;
	int64 cumul_filtrage,cumul_lecture; clock_t c0,cpu_utilise;
	struct timeval trmt_debut,trmt_fin;
	int nb_msgs,nb_voies,nb_fltr,nb_convol,declenche; char debug_cpu;
#ifdef SELECTION_HORS_EVTS
	TypeDonnee of7;
#endif
#ifdef VISU_FONCTION_TRANSFERT
	char raison[80]; TypeDonnee *ptr_short; fftw_real *ptr_fft;
	float normalisation;
	int t,nb,debut;
#endif
#ifdef UTILISE_EN_DEMARRAGE
	char voie_traitee;
#endif
	int j1,k1;
	// int64 point_decale,debut_evts,debut_tampons;
#ifdef TRIGGER_DEVELOPPE
//	char pente;
//	TypeDonnee precedente,mihauteur;
//	int64 dernierpic,date_evt;
//#ifdef RL
//	int64 posmax;
//#endif
	TypeVoieSuivi *suivi;
	char seuil_depasse;
	float amplitude,niveau;
	int64 delta_points;
	float montee,duree;
#endif

	c0 = VerifConsoCpu? clock() : 0;
	t0 = (VerifTempsPasse? DateMicroSecs(): 0); n0 = TempsTotalLect;
	//+ printf("%8lld      debut traitement\n",t0-LectT0Run);
	log = VerifRunDebug;
	if(log) printf("%s/ Traitement des points [%lld .. %lld[ (%lld)\n",DateHeure(),DernierPointTraite,LectStampsLus,LectStampsLus-DernierPointTraite);
	else if(VerifCpuCalcul) printf("%s/ deb traitmt [%7lld .. %7lld[ @%lld (cumul lecture=%lld)\n",DateHeure(),DernierPointTraite,LectStampsLus,t0,n0);
	mesure_tres_fine = 0;
	cumul_filtrage = cumul_lecture = 0;
	temps_utilise = n2 = t2 = vrai_dernier = 0;
#ifdef HISTO_TIMING_TRMT
	if(VerifTempsPasse) {
		th = t0 - TrmtHeureDernier;
		k = (int)((th + 50000) / 100000);
		if(k < 0) k = 0; else if(k >= MAXHISTOTIMING) k = MAXHISTOTIMING - 1;
		HistoTiming[k] += 1;
	}
#endif
/*
==== Initialisation de la lecture standard ====
*/
/* #define REP_AVEC_BIT0  pour le FPGA programme 4166 (le bit 0 est colle) */
	if(LectCntl.LectMode == LECT_IDENT) {
		TypeADU *ptr,ident;
		if(TrmtIdentPasFaite) {
			for(vt=0; vt<VoieIdentMax; vt++) {
				VoieIdent[vt].lu = VoieIdent[vt].donnees[0];
			}
			TrmtIdentPasFaite = 0;
		}
		for(vt=0; vt<VoieIdentMax; vt++) {
				DernierPointTraite = VoieIdent[vt].analyse;
				k = Modulo(DernierPointTraite,VoieIdent[vt].dim);
				ptr = VoieIdent[vt].donnees + k;
				do {
					ident = *ptr++;
					VoieIdent[vt].essais += 1;
					if(ident != VoieIdent[vt].lu) VoieIdent[vt].erreurs += 1;
					if(++DernierPointTraite >= VoieIdent[vt].lus) break;
					if(++k >= VoieIdent[vt].dim) {
						k = 0; ptr = VoieIdent[vt].donnees;
					}
				} forever;
				VoieIdent[vt].analyse = DernierPointTraite;
		}
		return;

	}

/*
==== Recherche d'evenements ====
*/
/*  Plusieurs raisons pour laisser tomber... */
	if(!Trigger.actif && !TrmtDemande) {
		TrmtPasActif += (LectStampsLus - Trigger.analyse);
		Trigger.analyse = LectStampsLus;
		return;
	}
	points_dispo = LectStampsLus;
    //+ printf("Points global dispo initialement: %lld (lus: %lld)\n",points_dispo,LectStampsLus);
	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue > 0) {
		point_stocke = (points_dispo - 1) / (int64)VoieTampon[voie].trmt[TRMT_AU_VOL].compactage;
		// printf("          Voie %s, trigger #%d prevu a %lld recherche apres %lld, tps mort #%d\n",VoieManip[voie].nom,VoieTampon[voie].trig.trgr->type,VoieTampon[voie].signal.futur_evt,TrgrEtat[voie].debut_recherche,VoieManip[voie].def.trgr.reprise);
        //+ printf("Points %s stockes: %lld, lus: %lld\n",VoieManip[voie].nom,point_stocke,VoieTampon[voie].lus);
		if(point_stocke >= VoieTampon[voie].lus) {
			if((point_stocke > VoieTampon[voie].trmt[TRMT_AU_VOL].compactage) && (VoieTampon[voie].lus == 0)) {
				OpiumFail("Le traitement au vol de la voie %s ne produit pas de donnee. Lecture abandonnee",VoieManip[voie].nom);
				LectDeclencheErreur(_ORIGINE_,61,LECT_NODATA);
				return;
			}
			points_dispo = ((VoieTampon[voie].lus - 1) * (int64)VoieTampon[voie].trmt[TRMT_AU_VOL].compactage) + 1;
            //+ printf("Soit points global dispo finalement: %lld\n",points_dispo);
		}
	}
	if((Trigger.type == TRGR_SCRPT) && (CebVarRefEtat(TrmtVar) != CEB_DEF)) return; /* encore que... */
	else if(Trigger.type == TRGR_PERSO) TrgrPerso();
	else if(!VoiesLocalesNb) Trigger.analyse = points_dispo;

	else { /* (Trigger.type == TRGR_CABLE) || (Trigger.type == TRGR_SCRPT) */

/* .... Debut de la boucle sur le temps .............................................................................. */
		/* DernierPointTraite: compteur (de temps) 'absolu', i.e. a la frequence <Echantillonnage> (kHz), 
		   1 point stocke pour <compactage au vol> DernierPointTraite
		   1 point utilise pour <compactage pre-trigger> point_stocke
		   soit <delta_points> points absolus par point examine ici */
		TrmtAppelsNb++;
		nb_msgs = 0; declenche = 0; debug_cpu = 0;
		vrai_dernier = DernierPointTraite;
		if(DernierPointTraite != Trigger.analyse) {
			int perdu;
			perdu = (int)(DernierPointTraite - Trigger.analyse);
			printf("%s! %d donnee%s non traitee%s entre %lld et %lld\n",DateHeure(),Accord2s(perdu),Trigger.analyse,DernierPointTraite);
			DernierPointTraite = Trigger.analyse;
		}
  		if(log || VerifCalculDetail) printf("%s/ Analyse des donnees dans [%7lld .. %7lld[ (%lld)\n",DateHeure(),DernierPointTraite,points_dispo,points_dispo-DernierPointTraite);
		if(VerifCpuCalcul) nb_fltr = nb_convol = 0;
		TrmtModePanique = 0;
		if(TrmtApanique) {
			if(DernierPointTraite < points_dispo) TrmtApanique = 0;
			else {
				printf("%s/ Traitement deborde: %16lld points \"traites\"\n",DateHeure(),DernierPointTraite);
				printf("                           >= %16lld points dispo\n",points_dispo);
			}
		}
		mesure_avant = 0;
		mesure_globale = 0;
		if(LectModeSpectresAuto) DernierPointTraite = points_dispo;
		else while(DernierPointTraite < points_dispo) {
			/* debug_cpu = VerifCalculDetail && (DernierPointTraite < 400000) && (nb_msgs < 500); */
			if(!mesure_avant) { // (DernierPointTraite == vrai_dernier)
				// mesure_globale = ((TrmtAppelsNb > 10) && (TrmtAppelsNb < 16));
				if(mesure_globale) {
					gettimeofday(&trmt_debut,0);
					printf("%s.%06d: Traitement prevu  de %16lld a %16lld\n",DateHeure(),trmt_debut.tv_usec,DernierPointTraite,points_dispo);
					if(!mesure_avant && !TrmtMesureFineTerminee) TrmtMesureFineActive = 1;
					debut_mesures = DernierPointTraite;
					limite_fine = DernierPointTraite + 6;
					mesure_avant = 1;
				}
			}
			if(mesure_globale && !TrmtMesureFineTerminee) { // limite_fine indefinie si pas mesure_globale
				TrmtMesureFineActive = (DernierPointTraite < limite_fine);
				if(!TrmtMesureFineActive) TrmtMesureFineTerminee = 1;
			}
			if(!TrmtMesureFineActive) mesure_tres_fine = 0;
			if(LectDebutEvts >= 0) {
				a_sauver = (int)(LectStampsLus - LectDebutEvts);
				if(a_sauver > UsageTampons) {
				#ifdef EXPLIQUE_EVTS_PERDUS
					debut_evts = LectDebutEvts;
					debut_tampons = LectStampsLus - LngrTampons;
				#endif
					t1 = (VerifTempsPasse? DateMicroSecs(): 0); n1 = TempsTotalLect;
					ArchiveVide(ArchSauve[EVTS],"Trmt 3/4");
					temps_lecture = TempsTotalLect - n1;
					TrmtTempsArchive += ((VerifTempsPasse? DateMicroSecs(): 0) - t1 - temps_lecture);
					if(ArchHorsDelai) {
						TrmtAbandonneEvts(LectStampsLus - (int64)LngrTampons,"sauvegarde d'evenement");
					#ifdef EXPLIQUE_EVTS_PERDUS
						point_decale = DernierPointTraite;
						// EvtATraiter->trig = -1; EvtATraiter->fin = -1;
						if(VerifTempsPasse) tt = t0 - TrmtHeureDernier; else tt = 0;
						printf("%s/ Traitement precedent termine pour p=%6d,%05d a t=%6d,%06d\n",DateHeure(),
							(int)(vrai_dernier/100000),Modulo(vrai_dernier,100000),
							(int)(TrmtHeureDernier/1000000),Modulo(TrmtHeureDernier,1000000));
						printf("          Traitement actuel   commence pour p=%6d,%05d a t=%6d,%06d (delai: %d,%06d)\n",
							(int)(Trigger.analyse/100000),Modulo(Trigger.analyse,100000),
							(int)(t0/1000000),Modulo(t0,1000000),(int)(tt/1000000),Modulo(tt,1000000));
						printf("          Decompte temps mort decide      a p=%6d,%05d\n",
							(int)(point_decale/100000),Modulo(point_decale,100000));
						printf("          Blocs lus:     %6d,%05d          synchro D2 lues: %12lld\n",
							(int)(LectStampsLus/100000),Modulo(LectStampsLus,100000),SynchroD2lues);
						printf("          Debut tampons: %6d,%05d      prochain traitement: %12lld +%d D2\n",
							(int)(debut_tampons/100000),Modulo(debut_tampons,100000),LectNextTrmt,(int)LectIntervTrmt);
						printf("          Debut evts:    %6d,%05d\n",
							(int)(debut_evts/100000),Modulo(debut_evts,100000));
					#endif
						ArchHorsDelai = 0;
					}
				}
			}

			if(TrmtMesureFineActive) { t2 = (VerifTempsPasse? DateMicroSecs(): 0); n2 = TempsTotalLect; nb_voies = 0; }
			/* ........ Debut de la boucle sur les voies ............................................................. */
			for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue > 0) {
				if(VoieTampon[voie].trig.trgr->conn.out && (DernierPointTraite == TrgrEtat[voie].date_sortie)) {
					int conn;
					for(conn=0; conn<VoiesNb; conn++) if((conn != voie) && (TrgrEtat[conn].accepte == voie)) {
						TrmtCandidatSignale(1,conn,DernierPointTraite,niveau,TRGR_ALGO_CONN);
					}
				}
				point_stocke = DernierPointTraite / (int64)VoieTampon[voie].trmt[TRMT_AU_VOL].compactage;
				/* if(point_stocke >= VoieTampon[voie].lus) {
					printf("(%s) %s[%lld/%lld] utilise (%.1f ms)\n",__func__,VoieManip[voie].nom,
						point_stocke,VoieTampon[voie].lus,(float)point_stocke * VoieEvent[voie].horloge);
					printf("(%s) bloc %lld/%lld\n",__func__,DernierPointTraite,points_dispo);
				} */
				if(point_stocke < VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien) {
					gettimeofday(&LectDateRun,0);
					// printf("%s.%06d: Traitement panique a %16lld\n",DateHeure(),LectDateRun.tv_usec,DernierPointTraite);
					if(!TrmtModePanique) {
						printf("%s/ Le Traitement passe en mode 'SURCHARGE', %d lecture%s%s en cours\n",DateHeure(),
							   Accord1s(LectDepileNb),LectDepileEnCours?" avec une":", aucune");
						printf("          | On n'a pas traite a temps la voie %s. Donnees:\n",VoieManip[voie].nom);
						printf("          | . lues au total     : %lld,\n",VoieTampon[voie].lus);
						printf("          | . plus ancienne lue : %lld,\n",VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien);
						printf("          | . derniere traitee  : %lld.\n",point_stocke);
						mesure_globale = 1; mesure_avant = 0;
						MonitModeSon = 0;
						TrmtPanicAjoute = 0; TrmtPanicNb = 0; TrmtPanicTotal++;
					} else if(TrmtPanicTotal > TrmtPanicMax) { LectDeclencheErreur(_ORIGINE_,21,LECT_OVERFLO); return; }
					if(TrmtRelanceSuivi()) break;
				}
				if(log) printf("%s/ Analyse de la voie %s dans [%7lld .. %7lld[ (%lld)\n",DateHeure(),VoieManip[voie].nom,TrgrEtat[voie].point_traite,point_stocke,point_stocke-TrgrEtat[voie].point_traite);
				if(point_stocke <= TrgrEtat[voie].point_traite) continue;
				if(TrmtMesureFineActive) nb_voies++;
				if(VerifCalculDetail) { t3 = (VerifTempsPasse? DateMicroSecs(): 0); n3 = TempsTotalLect; }
				TrgrEtat[voie].point_traite = point_stocke;
				
				/*
				 * La recherche d'un nouvel evenement peut etre inhibee en cas d'evenement en cours
				 */
				if((VoieManip[voie].def.trgr.reprise != TRGR_REPRISE_SEUIL) || (VoieTampon[voie].trig.cherche != TRGR_NIVEAU) || !(TrgrEtat[voie].evt_en_cours))
					inhibe_en_cours = (point_stocke < TrgrEtat[voie].debut_recherche);
				else inhibe_en_cours = 1;
				// if(inhibe_en_cours) printf("          ! Voie %s inhibee: point_stocke=%d, debut_recherche=%d\n",VoieManip[voie].nom,point_stocke,TrgrEtat[voie].debut_recherche);
			#ifdef SAUTE_EVT
				saute_evt = inhibe_en_cours || (DernierPointTraite < EvtATraiter->fin);
			#else
				saute_evt = 0;
			#endif
				/*
				 * recuperation du dernier point de la voie courante (dans <val>)
				 */
			#ifdef UTILISE_EN_DEMARRAGE
				voie_traitee = 0;
			#endif
				k = Modulo(point_stocke,VoieTampon[voie].brutes->max);
				if(VoieTampon[voie].brutes->type == TAMPON_INT16) val32 = (int32)*(VoieTampon[voie].brutes->t.sval + k);
				else val32 = *(VoieTampon[voie].brutes->t.ival + k);
				reelle = (double)val32;

				/*
				 * Evaluation du bruit RMS
				 */
				if(mesure_tres_fine) TrmtMesureCpu("Bruit RMS",t2,0);
				if(VoieTampon[voie].signal.nb < VoieManip[voie].duree.mesureLdB) {
					VoieTampon[voie].signal.somme += reelle;
					VoieTampon[voie].signal.carre += reelle * reelle;
					VoieTampon[voie].signal.nb += 1;
				} else /* duree.mesureLdB devrait etre inferieur a brutes.max */ {
					j = Modulo(point_stocke - VoieManip[voie].duree.mesureLdB,VoieTampon[voie].brutes->max);
					if(VoieTampon[voie].brutes->type == TAMPON_INT16) valDD = (double)VoieTampon[voie].brutes->t.sval[j];
					else valDD = (double)VoieTampon[voie].brutes->t.ival[j];
					VoieTampon[voie].signal.somme += (reelle - valDD);
					VoieTampon[voie].signal.carre += (reelle - valDD) * (reelle + valDD);
				}
				VoieTampon[voie].signal.mesure = 1;
				if(mesure_tres_fine) TrmtMesureCpu("Mesure de V(I)",t2,0);
				if(VoieManip[voie].modulee) {
					VoieInfo[voie].moyenne = VoieTampon[voie].signal.somme / (float)VoieTampon[voie].signal.nb;
					VoieInfo[voie].mesure_enV = VoieInfo[voie].moyenne * VoieManip[voie].ADUenV;
					if(SynchroD2lues >= LectNextDisp) DetecteurViMesuree(voie);
				}

				/*
				 * Traitement demande, trigger eventuellement inclus
				 */
				if(VoieTampon[voie].cree_traitee) {

					// ............... regulation ........................................................................
					if(mesure_tres_fine) TrmtMesureCpu("Regulation",t2,0);
					if(VoieTampon[voie].regul_active == TRMT_REGUL_INTERV) {
						int n; float prec; TypeRegulInterv *parm;
						// if(VoieTampon[voie].regul_standard) parm = &TrmtRegulInterv; else parm = &(VoieTampon[voie].rgul.interv.parm);
						if(debug_cpu) { printf("%-13s @%7lld: regulation demandee\n",VoieManip[voie].nom,DernierPointTraite); nb_msgs++; }
						parm = &(VoieManip[voie].interv);
						n = (int)(point_stocke / (int64)VoieTampon[voie].rgul.interv.lngr);
						j = n % parm->nb;
						if(val32 > VoieTampon[voie].rgul.interv.limp) {
							if(!VoieTampon[voie].rgul.interv.evtp[j]) {
								if(++(VoieTampon[voie].rgul.interv.ovrp) > parm->max) {
									prec = VoieTampon[voie].trig.trgr->minampl;
									VoieTampon[voie].trig.trgr->minampl += parm->incr;
									printf("%s/ %12s, change seuil min de %g a %g\n",DateHeure(),VoieManip[voie].nom,prec,VoieTampon[voie].trig.trgr->minampl);
									VoieTampon[voie].rgul.interv.limp = (TypeDonnee)(VoieTampon[voie].trig.trgr->minampl * parm->seuil);
									VoieTampon[voie].rgul.interv.ovrp = 0;
									TrmtRegulModifs++;
								}
							}
							VoieTampon[voie].rgul.interv.evtp[j] = 1;
							VoieTampon[voie].rgul.interv.max[j] = VoieTampon[voie].rgul.interv.moyp / VoieTampon[voie].rgul.interv.nbp;
						} else if(val32 < VoieTampon[voie].rgul.interv.limm) {
							if(!VoieTampon[voie].rgul.interv.evtm[j]) {
								if(++(VoieTampon[voie].rgul.interv.ovrm) > parm->max) {
									prec = VoieTampon[voie].trig.trgr->maxampl;
									VoieTampon[voie].trig.trgr->maxampl -= parm->incr;
									printf("%s/ %12s, change seuil max de %g a %g\n",DateHeure(),VoieManip[voie].nom,prec,VoieTampon[voie].trig.trgr->maxampl);
									VoieTampon[voie].rgul.interv.limm = (TypeDonnee)(VoieTampon[voie].trig.trgr->maxampl * parm->seuil);
									VoieTampon[voie].rgul.interv.ovrm = 0;
									TrmtRegulModifs++;
								}
							}
							VoieTampon[voie].rgul.interv.evtm[j] = 1;
							VoieTampon[voie].rgul.interv.min[j] = VoieTampon[voie].rgul.interv.moym / VoieTampon[voie].rgul.interv.nbm;
						}
						if(VoieTampon[voie].rgul.interv.used == n) {
							if(!VoieTampon[voie].rgul.interv.evtp[j]) {
								if(reelle > VoieTampon[voie].rgul.interv.max[j]) VoieTampon[voie].rgul.interv.max[j] = reelle;
							}
							if(!VoieTampon[voie].rgul.interv.evtm[j]) {
								if(reelle < VoieTampon[voie].rgul.interv.min[j]) VoieTampon[voie].rgul.interv.min[j] = reelle;
							}
						} else {
							j1 = j - 1; if(j1 < 0) j1 = parm->nb - 1;
							if(!VoieTampon[voie].rgul.interv.evtp[j1]) {
								VoieTampon[voie].rgul.interv.moyp += VoieTampon[voie].rgul.interv.max[j1];
								VoieTampon[voie].rgul.interv.nbp += 1.0;
								VoieTampon[voie].rgul.interv.ovrp = 0;
							} else VoieTampon[voie].rgul.interv.evtp[j1] = 0;
							if(!VoieTampon[voie].rgul.interv.evtm[j1]) {
								VoieTampon[voie].rgul.interv.moym += VoieTampon[voie].rgul.interv.min[j1];
								VoieTampon[voie].rgul.interv.nbm += 1.0;
								VoieTampon[voie].rgul.interv.ovrm = 0;
							} else VoieTampon[voie].rgul.interv.evtm[j1] = 0;
							if(VoieTampon[voie].rgul.interv.nbp > parm->nb) {
								VoieTampon[voie].rgul.interv.moyp -= VoieTampon[voie].rgul.interv.derp;
								VoieTampon[voie].rgul.interv.nbp -= 1.0;
							}
							if(VoieTampon[voie].rgul.interv.nbm > parm->nb) {
								VoieTampon[voie].rgul.interv.moym -= VoieTampon[voie].rgul.interv.derm;
								VoieTampon[voie].rgul.interv.nbm -= 1.0;
							}
							prec = VoieTampon[voie].trig.trgr->minampl;
							VoieTampon[voie].trig.trgr->minampl = (VoieTampon[voie].rgul.interv.moyp / VoieTampon[voie].rgul.interv.nbp) + parm->ajuste;
							if(prec != VoieTampon[voie].trig.trgr->minampl) {
								printf("%s/ %12s, change seuil min de %g a %g\n",DateHeure(),VoieManip[voie].nom,prec,VoieTampon[voie].trig.trgr->minampl);
								VoieTampon[voie].rgul.interv.limp = (TypeDonnee)(VoieTampon[voie].trig.trgr->minampl * parm->seuil);
								TrmtRegulModifs++;
							}
							prec = VoieTampon[voie].trig.trgr->maxampl;
							VoieTampon[voie].trig.trgr->maxampl = (VoieTampon[voie].rgul.interv.moym / VoieTampon[voie].rgul.interv.nbm) - parm->ajuste;
							if(prec != VoieTampon[voie].trig.trgr->maxampl) {
								printf("%s/ %12s, change seuil max de %g a %g\n",DateHeure(),VoieManip[voie].nom,prec,VoieTampon[voie].trig.trgr->maxampl);
								VoieTampon[voie].rgul.interv.limm = (TypeDonnee)(VoieTampon[voie].trig.trgr->maxampl * parm->seuil);
								TrmtRegulModifs++;
							}
							VoieTampon[voie].rgul.interv.derp = VoieTampon[voie].rgul.interv.max[j];
							VoieTampon[voie].rgul.interv.derm = VoieTampon[voie].rgul.interv.min[j];
							VoieTampon[voie].rgul.interv.min[j] = VoieTampon[voie].rgul.interv.max[j] = reelle;
						}
						VoieTampon[voie].rgul.interv.used = n;
					}

					// ............... calcul et soustraction eventuels du pattern moyen, avant .................................
					if(mesure_tres_fine) TrmtMesureCpu("Pattern",t2,0);
					if((SettingsSoustra == TRMT_PTN_AVANT) && VoieTampon[voie].pattern.val && !(SettingsSauteEvt && inhibe_en_cours)) {
						if(debug_cpu) { printf("%-13s @%7lld: soustraction pattern prealable demandee\n",VoieManip[voie].nom,DernierPointTraite); nb_msgs++; }
						j1 = Modulo(point_stocke,VoieTampon[voie].pattern.max);
						//-- if(point_stocke < VoieTampon[voie].pattern.total) 
						if(VoieTampon[voie].pattern.nb[j1] < VoieManip[voie].duree.periodes) {
							if(!saute_evt) {
								VoieTampon[voie].pattern.val[j1] += reelle;
								VoieTampon[voie].pattern.nb[j1] += 1.0;
							}
						} else {
						#ifdef SAUTE_EVT
							if(!saute_evt) {
								point_vire = point_stocke - VoieTampon[voie].pattern.total;
								while(point_vire < VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien) {
									point_vire += VoieTampon[voie].pattern.dim;
									if(point_vire > point_stocke) break;
									if(point_vire >= TrgrEtat[voie].debut_recherche) {
										k1 = Modulo(point_vire,VoieTampon[voie].brutes->max);				     
										VoieTampon[voie].pattern.val[j1] += (float)(VoieTampon[voie].brutes->t.sval[k] - VoieTampon[voie].brutes->t.sval[k1]);
									}
								}
							}
						#else
							point_vire = point_stocke - VoieTampon[voie].pattern.total;
							k1 = Modulo(point_vire,VoieTampon[voie].brutes->max);				     
							VoieTampon[voie].pattern.val[j1] += (float)(VoieTampon[voie].brutes->t.sval[k] - VoieTampon[voie].brutes->t.sval[k1]);
						#endif
						}
						reelle = reelle - (VoieTampon[voie].pattern.val[j1] / VoieTampon[voie].pattern.nb[j1]);
					}

					// ............... demodulation pre-trigger .............................................................
					if(mesure_tres_fine) TrmtMesureCpu("Differe",t2,0);
					if(VoieManip[voie].differe) {
						if(debug_cpu) { printf("%-13s @%7lld: traitement au vol differe\n",VoieManip[voie].nom,DernierPointTraite); nb_msgs++; }
						if(SettingsInlineProcess) {
							if(!SambaTraiteDonnee(TRMT_AU_VOL,TRMT_DEMODUL,voie,val32,0,point_stocke,&val32,&reelle)) continue;
						} else /* (cas !SettingsInlineProcess) */ {
						#ifdef TRAITEMENT_DEVELOPPE
							int parm,nbdata,compte,modul; char dispo;
							dispo = 0;
							nbdata = VoieTampon[voie].trmt[TRMT_AU_VOL].nbdata;
							/* if(LectSynchroD3) {
								if(VoieTampon[voie].trmt[TRMT_AU_VOL].compte > 1) {
									reelle = VoieTampon[voie].trmt[TRMT_AU_VOL].reelle * (float)VoieManip[voie].def.trmt[TRMT_AU_VOL].gain / (float)nbdata;
									dispo = 1;
									LectRecaleD3++;
								}
								VoieTampon[voie].trmt[TRMT_AU_VOL].phase = 0;
								compte = 2;
							} else */ compte = VoieTampon[voie].trmt[TRMT_AU_VOL].compte + 1;
							parm = VoieManip[voie].def.trmt[TRMT_AU_VOL].p1;
							modul = Bolo[VoieManip[voie].det].d2;
							VoieTampon[voie].trmt[TRMT_AU_VOL].compte = compte;
							switch(VoieTampon[voie].trmt[TRMT_AU_VOL].phase) {
							  case 0:   /* phase 0: 1ere marge                    */
								if(compte > parm) {
									VoieTampon[voie].trmt[TRMT_AU_VOL].reelle = reelle;
									VoieTampon[voie].trmt[TRMT_AU_VOL].phase++;
								}
								break;
							  case 1:   /* phase 1: 1ere demi-periode hors marges */
								if(compte <= ((modul / 2) /* pas de marge en fin de demi-periode */)) {
									VoieTampon[voie].trmt[TRMT_AU_VOL].reelle += reelle;
								} else VoieTampon[voie].trmt[TRMT_AU_VOL].phase++;
								break;
							  case 2:  /* phase 2: marges inter-periodes         */
								if(compte > ((modul / 2) + parm)) {
									VoieTampon[voie].trmt[TRMT_AU_VOL].reelle -= reelle;
									VoieTampon[voie].trmt[TRMT_AU_VOL].phase++;
								}
								break;
							  case 3:  /* phase 3: 2eme demi-periode hors marges */
								VoieTampon[voie].trmt[TRMT_AU_VOL].valeur -= reelle;
								if(compte == modul) {
									reelle = VoieTampon[voie].trmt[TRMT_AU_VOL].reelle * (float)VoieManip[voie].def.trmt[TRMT_AU_VOL].gain / (float)nbdata;
									dispo = 1;
									VoieTampon[voie].trmt[TRMT_AU_VOL].compte = 0;
									VoieTampon[voie].trmt[TRMT_AU_VOL].phase = 0;
								}
								break;
							}
							if(!dispo) continue;
						#endif /* TRAITEMENT_DEVELOPPE */
						}
					}

					// ............... autre traitement pre-trigger .............................................................
					t4 = (VerifTempsPasse? DateMicroSecs(): 0);
					// if(debug_cpu && declenche) { printf("%13s @%7lld: filtrage mesure deb @%lld\n",VoieManip[voie].nom,DernierPointTraite,t4); nb_msgs++; }
					if(mesure_tres_fine) TrmtMesureCpu("Filtrage",t2,0);
					trmt = LectTrmtUtilise(voie,TRMT_PRETRG);
					if((trmt == NEANT) || SettingsSansTrmt) courante = val32;
					else {
						if(SettingsInlineProcess) {
							if(!SambaTraiteDonnee(TRMT_PRETRG,trmt,voie,val32,0,point_stocke,&courante,&reelle)) continue;
						} else /* (cas !SettingsInlineProcess) */ {
							t5 = (VerifTempsPasse? DateMicroSecs(): 0);
							// if(debug_cpu && declenche) { printf("%13s @%7lld: filtrage pre-trigger @%lld\n",VoieManip[voie].nom,DernierPointTraite,t5); nb_msgs++; }
						#ifdef TRAITEMENT_DEVELOPPE
							if(trmt == TRMT_FILTRAGE) {
								/* ------------------------------------------------------------ */
								/* |     debut du filtrage                                    | */
								/* ------------------------------------------------------------ */
								TypeFiltre *filtre; TypeSuiviFiltre *suivi; TypeDonneeFiltree brute,entree,filtree;
								int nbdata,n;

								if((filtre = VoieTampon[voie].trmt[TRMT_PRETRG].filtre)
								&& (suivi = &(VoieTampon[voie].trmt[TRMT_PRETRG].suivi))) {
									if(VerifCpuCalcul) nb_fltr++;
									nbdata = suivi->nb;
									if(nbdata < suivi->max) n = nbdata; else n =  suivi->prems;
									brute = filtree = entree = (TypeDonneeFiltree)reelle;
								#ifdef ALTIVEC
									if(SettingsAltivec) filtree = TrmtFiltreAltivec(&entree,suivi,nbdata,n,filtre,brute); else
								#endif /* ALTIVEC */
									filtree = TrmtFiltreStd(&entree,suivi,nbdata,n,filtre,brute);
								#ifdef RL
									filtree = TrmtFiltreTemplate(&entree,suivi,voie,nbdata,n,filtre,&nb_convol);
								#endif
									reelle = (float)filtree * (float)VoieManip[voie].def.trmt[TRMT_PRETRG].gain;
									if(nbdata < suivi->max) suivi->nb += 1;
									else {
										suivi->prems += 1;
										if(suivi->prems >= suivi->max) suivi->prems = 0;
									}
								}
								/* ------------------------------------------------------------ */
								/* |     fin du filtrage                                      | */
								/* ------------------------------------------------------------ */
							} 
						#ifdef TRMT_ADDITIONNELS
							else if(trmt == TRMT_LISSAGE) {
								/* X: valeur d'origine (pour pouvoir lisser) */
								if(VoieTampon[voie].trmt[TRMT_PRETRG].X.nb < VoieTampon[voie].trmt[TRMT_PRETRG].X.max) {
									VoieTampon[voie].trmt[TRMT_PRETRG].X.val[VoieTampon[voie].trmt[TRMT_PRETRG].X.nb] = reelle;
									VoieTampon[voie].trmt[TRMT_PRETRG].X.nb += 1;
								} else {
									// printf("%s @%08llX + %08llX\n",VoieManip[voie].nom,(IntAdrs)VoieTampon[voie].trmt[TRMT_PRETRG].X.val,
									//       (IntAdrs)VoieTampon[voie].trmt[TRMT_PRETRG].X.prem);
									VoieTampon[voie].trmt[TRMT_PRETRG].X.val[VoieTampon[voie].trmt[TRMT_PRETRG].X.prem] = reelle;
									VoieTampon[voie].trmt[TRMT_PRETRG].X.prem += 1;
									if(VoieTampon[voie].trmt[TRMT_PRETRG].X.prem >= VoieTampon[voie].trmt[TRMT_PRETRG].X.max)
										VoieTampon[voie].trmt[TRMT_PRETRG].X.prem = 0;
								}
								/* lissage sur le tableau X */
								if(VoieTampon[voie].trmt[TRMT_PRETRG].compte < VoieManip[voie].def.trmt[TRMT_PRETRG].p1) {
									VoieTampon[voie].trmt[TRMT_PRETRG].reelle += reelle;
									VoieTampon[voie].trmt[TRMT_PRETRG].compte += 1;
								} else {
									int k1;
									k1 = Modulo(point_stocke - VoieManip[voie].def.trmt[TRMT_PRETRG].p1,VoieTampon[voie].trmt[TRMT_PRETRG].X.max);
									VoieTampon[voie].trmt[TRMT_PRETRG].reelle += (reelle - VoieTampon[voie].trmt[TRMT_PRETRG].X.val[k1]);
								}
								reelle = VoieTampon[voie].trmt[TRMT_PRETRG].reelle * (float)VoieManip[voie].def.trmt[TRMT_PRETRG].gain / (float)VoieTampon[voie].trmt[TRMT_PRETRG].compte;
							} else if(trmt == TRMT_MOYENNE) {
								VoieTampon[voie].trmt[TRMT_PRETRG].reelle += reelle;
								VoieTampon[voie].trmt[TRMT_PRETRG].compte += 1;
								if(VoieTampon[voie].trmt[TRMT_PRETRG].compte < VoieManip[voie].def.trmt[TRMT_PRETRG].p1) continue;
								else {
									reelle = VoieTampon[voie].trmt[TRMT_PRETRG].reelle * (float)VoieManip[voie].def.trmt[TRMT_PRETRG].gain / (float)VoieTampon[voie].trmt[TRMT_PRETRG].compte;
									VoieTampon[voie].trmt[TRMT_PRETRG].reelle = 0.0;
									VoieTampon[voie].trmt[TRMT_PRETRG].compte = 0;
								}
							} else if(trmt == TRMT_MAXIMUM) {
								if(VoieTampon[voie].trmt[TRMT_PRETRG].compte == 0) VoieTampon[voie].trmt[TRMT_PRETRG].reelle = reelle;
								else if(reelle > VoieTampon[voie].trmt[TRMT_PRETRG].reelle) VoieTampon[voie].trmt[TRMT_PRETRG].reelle = reelle;
								VoieTampon[voie].trmt[TRMT_PRETRG].compte += 1;
								if(VoieTampon[voie].trmt[TRMT_PRETRG].compte < VoieManip[voie].def.trmt[TRMT_PRETRG].p1) continue;
								else {
									reelle = VoieTampon[voie].trmt[TRMT_PRETRG].reelle * (float)VoieManip[voie].def.trmt[TRMT_PRETRG].gain;
									VoieTampon[voie].trmt[TRMT_PRETRG].reelle = 0.0;
									VoieTampon[voie].trmt[TRMT_PRETRG].compte = 0;
								}
							}
						#endif /* TRMT_ADDITIONNELS */
							else if(!SambaTraiteDonnee(TRMT_PRETRG,trmt,voie,val32,0,point_stocke,&courante,&reelle)) continue;
						#endif /* TRAITEMENT_DEVELOPPE */
							tt = (VerifTempsPasse? DateMicroSecs(): 0);
							temps_utilise = tt - t5;
							// if(debug_cpu && declenche) { printf("%13s @%7lld: filtrage termine    @%lld (%lld us)\n",VoieManip[voie].nom,DernierPointTraite,tt,temps_utilise); nb_msgs++; }
						}
					}
					tt = (VerifTempsPasse? DateMicroSecs(): 0);
					t4 = tt - t4;
					// if(debug_cpu && declenche) { printf("%13s @%7lld: filtrage mesure fin @%lld (%lld us)\n",VoieManip[voie].nom,DernierPointTraite,tt,t4); nb_msgs++; }
					if(debug_cpu) { if(1 /* t4 > 30 */) { printf("        ! Duree filtrage %13s @%lld: %lld us [+]\n",VoieManip[voie].nom,DernierPointTraite,t4); nb_msgs++; declenche++; } }

					// ............... soustraction et calcul eventuels du pattern moyen, apres .............................................
					if(mesure_tres_fine) TrmtMesureCpu("Post-pattern",t2,0);
					if((SettingsSoustra == TRMT_PTN_APRES) && VoieTampon[voie].pattern.val && !(SettingsSauteEvt && inhibe_en_cours)) {
						if(debug_cpu) { printf("%-13s @%7lld: soustraction pattern finale demandee\n",VoieManip[voie].nom,DernierPointTraite); nb_msgs++; }
						val32 = courante;
						j1 = Modulo(point_stocke,VoieTampon[voie].pattern.max);
						//-- if(point_stocke < VoieTampon[voie].pattern.total) 
						if(VoieTampon[voie].pattern.nb[j1] < VoieManip[voie].duree.periodes) {
							if(!saute_evt) {
								VoieTampon[voie].pattern.val[j1] += reelle;
								VoieTampon[voie].pattern.nb[j1] += 1.0;
							}
						} else {
						#ifdef SAUTE_EVT
							if(!saute_evt) {
								point_vire = point_stocke - VoieTampon[voie].pattern.total;
								while(point_vire < VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien) {
									point_vire += VoieTampon[voie].pattern.dim;
									if(point_vire > point_stocke) break;
									if(point_vire >= TrgrEtat[voie].debut_recherche) {
										k1 = Modulo(point_vire,VoieTampon[voie].brutes->max);				     
										VoieTampon[voie].pattern.val[j1] += (float)(VoieTampon[voie].brutes->t.sval[k] - VoieTampon[voie].brutes->t.sval[k1]);
									}
								}
							}
						#else
							point_vire = point_stocke - VoieTampon[voie].pattern.total;
							k1 = Modulo(point_vire,VoieTampon[voie].brutes->max);				     
							VoieTampon[voie].pattern.val[j1] += (float)(VoieTampon[voie].brutes->t.sval[k] - VoieTampon[voie].brutes->t.sval[k1]);
						#endif
						}
						reelle = reelle - (VoieTampon[voie].pattern.val[j1] / VoieTampon[voie].pattern.nb[j1]);
					}

					// ............... stockage du signal incluant tout le traitement pre-trigger .............................................
					if(mesure_tres_fine) TrmtMesureCpu("Stockage",t2,0);
					courante = (TypeDonnee)(reelle + 0.5);
					if(VoieTampon[voie].traitees->t.rval) {
						float biais;
						biais = 0.0;
						if(VoieTampon[voie].traitees->nb < VoieTampon[voie].traitees->max) {
							if((VoieTampon[voie].trig.trgr->type == TRGR_DERIVEE) && (VoieTampon[voie].traitees->nb >= VoieTampon[voie].trig.mincount)) {
								j1 = VoieTampon[voie].traitees->nb - VoieTampon[voie].trig.mincount;
								biais = VoieTampon[voie].traitees->t.rval[j1];
							}
							*(VoieTampon[voie].traitees->t.rval + VoieTampon[voie].traitees->nb) = reelle; // courante;
							VoieTampon[voie].traitees->nb += 1;
						} else {
							if((VoieTampon[voie].trig.trgr->type == TRGR_DERIVEE) && (VoieTampon[voie].traitees->nb >= VoieTampon[voie].trig.mincount)) {
								j1 = VoieTampon[voie].traitees->prem - VoieTampon[voie].trig.mincount;
								if(j1 < 0) j1 += VoieTampon[voie].traitees->max;
								biais = VoieTampon[voie].traitees->t.rval[j1];
							}
							*(VoieTampon[voie].traitees->t.rval + VoieTampon[voie].traitees->prem) = reelle; // courante;
							VoieTampon[voie].trmt[TRMT_PRETRG].plus_ancien++;
							VoieTampon[voie].traitees->prem++;
							if(VoieTampon[voie].traitees->prem >= VoieTampon[voie].traitees->max) {
								VoieTampon[voie].traitees->prem = 0;
								VoieTampon[voie].trmt[TRMT_PRETRG].point0 = VoieTampon[voie].trmt[TRMT_PRETRG].plus_ancien;
							}
						}
						reelle = reelle - biais;
					}
					if(log) printf("%s/ Stockage de la voie %s @%08llX: %d point%s, premier=%d\n",DateHeure(),VoieManip[voie].nom,(IntAdrs)(VoieTampon[voie].traitees->t.rval),Accord1s(VoieTampon[voie].traitees->nb),VoieTampon[voie].traitees->prem);
					
					// ............... verification si on doit encore faire autre chose (rechercher des evenements, en fait) ...................
				#ifdef UTILISE_EN_DEMARRAGE
					voie_traitee = 1;
                    if(log || debug_cpu) { printf("%-13s @%7lld: Recherche d'evenement envisagee, voie %s et %s\n",VoieManip[voie].nom,DernierPointTraite,
                                           TrgrEtat[voie].en_demarrage?"en demarrage":"demarree",inhibe_en_cours?"inhibee":"libre"); nb_msgs++; }
				#endif
					if((VoieTampon[voie].trig.trgr->type == NEANT) || (VoieTampon[voie].trig.trgr->origine != TRGR_INTERNE) || !Trigger.actif || MiniStream) continue;
					if(mesure_tres_fine) TrmtMesureCpu("Evenements",t2,0);
					if(/* TrgrEtat[voie].en_demarrage || */ inhibe_en_cours) {
						VoieTampon[voie].suivi.precedente = reelle;
						if(VoieTampon[voie].trig.cherche == TRGR_NIVEAU) {
							if((VoieManip[voie].def.trgr.reprise == TRGR_REPRISE_SEUIL) && TrgrEtat[voie].evt_en_cours) {
								if(VoieTampon[voie].trig.signe > 0) seuil_depasse = (reelle > VoieTampon[voie].trig.trgr->minampl);
								else if(VoieTampon[voie].trig.signe < 0) seuil_depasse = (reelle < VoieTampon[voie].trig.trgr->maxampl);
								else {
									if(reelle > VoieTampon[voie].trig.trgr->minampl) seuil_depasse = 1;
									else if(reelle < VoieTampon[voie].trig.trgr->maxampl) seuil_depasse = 1;
									else seuil_depasse = 0;
								}
								if(!seuil_depasse) {
									TrgrEtat[voie].evt_en_cours = 0;
									TrgrEtat[voie].debut_recherche = point_stocke;
								}
							}
						} else VoieTampon[voie].signal.base = reelle;
					} else if((VoieTampon[voie].trig.cherche == TRGR_NIVEAU) && (VoieManip[voie].def.trgr.reprise == TRGR_REPRISE_RENOUV) && TrgrEtat[voie].reprise) {
						if(VoieTampon[voie].trig.signe > 0) seuil_depasse = (reelle > VoieTampon[voie].trig.trgr->minampl);
						else if(VoieTampon[voie].trig.signe < 0) seuil_depasse = (reelle < VoieTampon[voie].trig.trgr->maxampl);
						else {
							if(reelle > VoieTampon[voie].trig.trgr->minampl) {
								seuil_depasse = 1; VoieTampon[voie].suivi.pente = 1;
								// if(DernierPointTraite < 10) printf("==> SEUIL DEPASSE\n");
							} else if(reelle < VoieTampon[voie].trig.trgr->maxampl) {
								seuil_depasse = 1; VoieTampon[voie].suivi.pente = -1;
							}
						}
						if(seuil_depasse) {
							TrgrEtat[voie].debut_recherche += VoieTampon[voie].trig.inhibe;
							inhibe_en_cours = 1;
						} else TrgrEtat[voie].reprise = 0;
					}
					if(!inhibe_en_cours) {

						if(debug_cpu) { printf("%-13s @%7lld: Recherche d'evenement demandee\n",VoieManip[voie].nom,DernierPointTraite); nb_msgs++; }

						/* ................ Recherche d'evenement avec ce nouveau point .............................. */
						if(SettingsInlineProcess) {
							evt_trouve = SambaDeclenche(voie,reelle);
						} else /* (cas !SettingsInlineProcess) */ {
						#ifdef TRIGGER_DEVELOPPE
							duree = 0.0;
							evt_trouve = 0;
							suivi = &(VoieTampon[voie].suivi);
							if(VerifCalculDetail) {
								if(suivi->date_evt) printf("(%s) Date[%s]=%lld @%lld, valeur=%g\n",__func__,VoieManip[voie].nom,suivi->date_evt,DernierPointTraite,reelle);
							}
							delta_points = VoieTampon[voie].trmt[TRMT_AU_VOL].compactage * VoieTampon[voie].trmt[TRMT_PRETRG].compactage;
						#ifdef DEBUG_TRIGGER
							if(!TrmtDejaDit) printf("Pic  (t=%d%03d,%02d0) = %4d ADU\n",
								(int)suivi->dernierpic/1000000,Modulo(suivi->dernierpic,1000000)/100,Modulo(suivi->dernierpic,1000000)%100,
								VoieTampon[voie].signal.base);
						#endif
							if(VoieTampon[voie].trig.trgr->type == TRGR_ALEAT) {
								if(DernierPointTraite > VoieTampon[voie].signal.futur_evt) {
									float fluct;
									niveau = reelle;
									TrmtCandidatSignale(0,voie,DernierPointTraite,niveau,TRGR_ALGO_RANDOM);
									evt_trouve = 1;
								#ifdef TIRAGE_GAUSSIEN
									do HasardTirage(VoieTampon[voie].trig.trgr->fluct,12,fluct) while(fluct < -19.5);
									VoieTampon[voie].signal.futur_evt = DernierPointTraite + (int64)((1.0 + (0.05 * fluct)) * VoieTampon[voie].signal.periode);
								#else
									do HasardTirage(1.0,12,fluct) while(fluct <= 0.0);
									VoieTampon[voie].signal.futur_evt = DernierPointTraite - (int64)(logf(fluct) * VoieTampon[voie].signal.periode * 1.4);
									// printf("(%s) Evt #%d signale @%lld, tirage=log(%g)=%g => prochain @%lld\n",__func__,LectCntl.MonitEvtNum,DernierPointTraite,fluct,-logf(fluct),VoieTampon[voie].signal.futur_evt);
								#endif
								}
							} else if(VoieTampon[voie].trig.cherche == TRGR_EXTREMA) {
							#ifdef DEBUG_TRIGGER
								if(!TrmtDejaDit) printf("Recherche des extremas (signal %g -> %g, deja %d0 탎 passees)\n",
									suivi->precedente,reelle,VoieTampon[voie].signal.climbs);
							#endif
								/* On recherche un pic => on regarde le changement de derivee */
								if(reelle > suivi->precedente) suivi->pente = 1;
								else if(reelle < suivi->precedente) suivi->pente = -1;
								/* si courante = precedente, la pente est consideree ne pas changer */
								if((suivi->pente * VoieTampon[voie].signal.climbs) < 0) {
									/* la derivee a change de signe! */
									niveau = suivi->precedente;
									amplitude = niveau - VoieTampon[voie].signal.base;
									montee = (float)(abs(VoieTampon[voie].signal.climbs)) * VoieEvent[voie].horloge;
								#ifdef DEBUG_TRIGGER
									if(!TrmtDejaDit) printf("Depuis %d%03d,%02d0 ms sur %3g ms: montee de %4d ADU a partir de %4d\n",
										(int)suivi->dernierpic/1000000,Modulo(suivi->dernierpic,1000000)/100,Modulo(suivi->dernierpic,1000000)%100,
										montee,amplitude,VoieTampon[voie].signal.base);
								#endif
								#ifdef DEBUG_FILTREES
									if(point_stocke < 3000) printf("(TrmtSurBuffer) [%d] dernier pic: %d, montee %d->%d ADU pendant %g ms\n",
										(int)DernierPointTraite,(int)suivi->dernierpic,VoieTampon[voie].signal.base,amplitude,montee);
								#endif

									if(Trigger.type == TRGR_SCRPT) {
										*(VoieTampon[voie].adrs_amplitude) = (float)amplitude;
										*(VoieTampon[voie].adrs_montee) = montee;
										*(VoieTampon[voie].adrs_niveau) = (float)(suivi->precedente - amplitude);
										*(VoieTampon[voie].adrs_duree) = duree;
									#ifdef DEBUG_PRGM_0
										if(!TrmtDejaDit) printf("Calcul pour amplitude %f sur %f 탎 au niveau %f\n",
											*(VoieTampon[voie].adrs_amplitude),
											*(VoieTampon[voie].adrs_montee),
											*(VoieTampon[voie].adrs_niveau));
										if(!TrmtDejaDit) CebExec(TrmtPrgm,1); else
									#endif
										CebExec(TrmtPrgm,0);
									#ifdef DEBUG_PRGM
										if(!TrmtDejaDit) printf("Resultat: %f\n",*TrgrResultat);
									#endif
										if(*TrgrResultat) {
											if(SettingsTempsEvt == TRGR_DATE_MONTEE) suivi->date_evt = suivi->dernierpic;
											else suivi->date_evt = DernierPointTraite - (1 * delta_points); /* (SettingsTempsEvt == TRGR_DATE_MAXI) */
											/*
											printf("Evt #%d a %lld,%02ld: montee de %d ADU a partir de %d\n",Acquis[AcquisLocale].etat.evt_trouves+1,
												suivi->date_evt/100,Modulo(suivi->date_evt,100),amplitude,VoieTampon[voie].signal.base);
											*/
											TrmtCandidatSignale(0,voie,suivi->date_evt,niveau,TRGR_ALGO_SCRIPT);
											evt_trouve = 1;
										}

									} else { /* (Trigger.type == TRGR_CABLE) */
										if(VoieTampon[voie].trig.trgr->type == TRGR_FRONT) {
											if((suivi->pente * VoieTampon[voie].trig.signe) <= 0) { /* pic dans le sens du signal? */
												if(((-suivi->pente * VoieTampon[voie].signal.climbs) >= VoieTampon[voie].trig.mincount) /* temps de montee suffisamt? */
												&& ((-suivi->pente * amplitude) >= VoieTampon[voie].trig.trgr->minampl)) { /* amplitude suffisante? */
													if((suivi->pente * amplitude) > 0) /* Pb! l'amplitude est dans le mauvais sens... */ {
														OpiumFail("ERREUR SYSTEME dans TrmtSurBuffer"); break;
													}
													if(SettingsTempsEvt == TRGR_DATE_MONTEE) suivi->date_evt = suivi->dernierpic;
													else suivi->date_evt = DernierPointTraite - (1 * delta_points); /* (SettingsTempsEvt == TRGR_DATE_MAXI) */
													TrmtCandidatSignale(0,voie,suivi->date_evt,niveau,TRGR_ALGO_FRONT);
													evt_trouve = 1;
												}
											}
										} else if(VoieTampon[voie].trig.trgr->type == TRGR_PORTE) {
											if((suivi->pente * VoieTampon[voie].trig.signe) <= 0) { /* pic dans le sens du signal? */
												if((duree >= VoieTampon[voie].trig.mindelai) /* duree suffisante? */
												&& ((-suivi->pente * amplitude) >= VoieTampon[voie].trig.trgr->minampl)) { /* amplitude suffisante? */
													if((suivi->pente * amplitude) > 0) /* Pb! l'amplitude est dans le mauvais sens... */ {
														OpiumFail("ERREUR SYSTEME dans TrmtSurBuffer"); break;
													}
													if(SettingsTempsEvt == TRGR_DATE_MONTEE) suivi->date_evt = suivi->dernierpic;
													else suivi->date_evt = DernierPointTraite - (1 * delta_points); /* (SettingsTempsEvt == TRGR_DATE_MAXI) */
													/* printf("Evt #%d a %d%03d,%02d: montee de %d ADU a partir de %d\n",Acquis[AcquisLocale].etat.evt_trouves+1,
														(int)suivi->date_evt/1000000,Modulo(suivi->date_evt,1000000)/100,Modulo(suivi->date_evt,1000000)%100,
														amplitude,VoieTampon[voie].signal.base); */
													TrmtCandidatSignale(0,voie,suivi->date_evt,niveau,TRGR_ALGO_PORTE);
													evt_trouve = 1;
												}
											}
									#ifdef VISU_FONCTION_TRANSFERT
										} else {
											/* pour VoieTampon[voie].trig.trgr->type == TRGR_TEST, il n'y a rien a faire a cet instant */
											if(TrmtHaffiche && VoieTampon[voie].traitees->t.rval && (Hnb > 0)) {
												debut = k - Hnb + 1;
												if((VoieTampon[voie].traitees->nb >= Hnb) && (debut >= 0)) {
													if(CalculeFFTplan(&CalcPlanStd,Hnb,raison)) {
														normalisation = (VoieEvent[voie].horloge / 1000.0) / (float)Hnb; /* secondes, soit 1/norme en Hz */
														ptr_short = VoieTampon[voie].brutes->t.sval + debut; ptr_fft = CalcPlanStd.temps;
														for(t=0; t<Hnb; t++) *ptr_fft++ = (fftw_real)*ptr_short++;
														rfftw_one(CalcPlanStd.plan,CalcPlanStd.temps,CalcPlanStd.freq);
														CalculePuissance(CalcPlanStd.freq,Xfiltre,Hnb,normalisation,1.0);

														//ptr_short = VoieTampon[voie].brutes->t.sval + debut; ptr_fft = CalcPlanStd.temps;
														//for(t=0; t<Hnb; t++) *ptr_fft++ = (fftw_real)*ptr_short++;
														if(VoieTampon[voie].traitees->nb < VoieTampon[voie].traitees->max)
															j = VoieTampon[voie].traitees->nb - Hnb;
														else {
															j = VoieTampon[voie].traitees->prem - Hnb;
															if(j < 0) j += VoieTampon[voie].traitees->max;
														}
														ptr_short = VoieTampon[voie].traitees->t.rval + j; ptr_fft = CalcPlanStd.temps;
														for(t=0; t<Hnb; t++) {
															*ptr_fft++ = (fftw_real)*ptr_short++;
															if(++j >= VoieTampon[voie].traitees->max) {
																j = 0; ptr_short = VoieTampon[voie].traitees->t.rval;
															}
														}
														rfftw_one(CalcPlanStd.plan,CalcPlanStd.temps,CalcPlanStd.freq);
														CalculePuissance(CalcPlanStd.freq,Yfiltre,Hnb,normalisation,1.0);

														nb = (Hnb / 2) + 1;
														for(t=0; t<nb; t++)
															if(Xfiltre[t] != 0.0) Hfiltre[t] = Yfiltre[t] / Xfiltre[t];
															else Hfiltre[t] = 10.0;
														GraphAxisReset(gTrmtFiltre,GRF_YAXIS);
														GraphUse(gTrmtFiltre,nb);
														OpiumDisplay(gTrmtFiltre->cdr);
														evt_trouve = 1;
														TrmtHaffiche = 0; /* sinon, on n'a plus jamais la main... */
													}
												}
											}
									#endif /* VISU_FONCTION_TRANSFERT */
										}
									}
								#ifdef UN_PEU_PARTICULIER
									if(!evt_trouve) {
										/* Histogrammes utilisateurs */
										/* en histogrammant ici, on a le bruit pour l'ensemble du signal
										 et non uniquement le bruit au moment de l'evenement */
										HistoDeVoie vh;
										vh = VoieHisto[voie];
										while(vh) {
											if(vh->Xvar == MONIT_BRUIT) HistoFillFloatToInt(vh->hd,fabsf(amplitude),1);
											vh = vh->suivant;
										}
										/* Histogrammes bruit */
										if(voie == SettingsHistoVoie) {
											if(TrmtHampl) HistoFillFloatToInt(hdBruitAmpl,fabsf(amplitude),1);
											if(TrmtHmontee) HistoFillFloatToInt(hdBruitMontee,montee,1);
											if(TrmtH2D) H2DFillFloatFloatToInt(h2D,fabsf(amplitude),montee,1);
										}
									}
								#endif /* UN_PEU_PARTICULIER */
									suivi->mihauteur = suivi->precedente - (amplitude / 2);
									duree = 0.0;
									VoieTampon[voie].signal.base = suivi->precedente;
									VoieTampon[voie].signal.climbs = 0;
									suivi->dernierpic = DernierPointTraite - (1 * delta_points);
		//							if(!TrmtDejaDit) printf("Point commun=%d => dernier pic=%d\n",(int)DernierPointTraite,(int)suivi->dernierpic);
								} else if(duree <= 0.0) {
									/* la derivee N'a PAS change de signe! et on n'a pas encore passe la mi-hauteur */
									if((suivi->pente * reelle) > (suivi->pente * suivi->mihauteur))
										duree = ((float)(DernierPointTraite - suivi->dernierpic) - 0.5) * VoieEvent[voie].horloge * 1000;
								}
								VoieTampon[voie].signal.climbs += (suivi->pente * VoieTampon[voie].trmt[TRMT_PRETRG].compactage);

							} else /* (VoieTampon[voie].trig.cherche == TRGR_NIVEAU) */ {
								/* si un evt est en cours, on cherche son amplitude (maxi avant descente sous le niveau de trigger) */
								if(suivi->date_evt) {
									if(!VoieTampon[voie].delai_template) {
										/* pas de template, pas de surprise */
										if(VoieTampon[voie].signal.climbs > 0) {
											if(reelle >= suivi->mihauteur) {  /* mihauteur represente le max (calcul en cours) ... */
												suivi->mihauteur = reelle;
												VoieTampon[voie].signal.climbs = (int)(DernierPointTraite - suivi->date_evt) / VoieTampon[voie].trmt[TRMT_AU_VOL].compactage;								
											} else
												// if(reelle < VoieTampon[voie].trig.trgr->minampl)  /* on n'arrete pas au premier maxi, on attend de redescendre */
												evt_trouve = 1;
										} else {
											if(reelle <= suivi->mihauteur) {  /* mihauteur represente le max (calcul en cours) ... */
												suivi->mihauteur = reelle;
												VoieTampon[voie].signal.climbs = (int)(suivi->date_evt - DernierPointTraite) / VoieTampon[voie].trmt[TRMT_AU_VOL].compactage;
											} else
												// if(reelle > VoieTampon[voie].trig.trgr->maxampl) /* on n'arrete pas au premier maxi, on attend de redescendre */
												evt_trouve = 1;
										}
									} else {
										/* le filtrage par template peut generer des oscillations y compris inversees et AVANT le maxi */
										if((float)fabs(reelle) >= suivi->mihauteur) {  /* mihauteur represente le max (calcul en cours) ... */
											suivi->mihauteur = (float)fabs(reelle);
											suivi->posmax = DernierPointTraite;
											//- printf("(%s) A p=%d: evenement vu sur %s demarrant p=%d: maxi %d a p=%d, precedent a p=%d\n",__func__,(int)DernierPointTraite,VoieManip[voie].nom,(int)suivi->date_evt,suivi->mihauteur,(int)suivi->posmax,(int)TrmtDernierTrig);
										}
										if((DernierPointTraite - suivi->date_evt) > VoieTampon[voie].delai_template) evt_trouve = 1;
									}																					
									if(evt_trouve) {
										//- printf("(%s) A p=%d: evenement vu sur %s demarrant p=%d +%d +/-%d, precedent a p=%d\n",__func__,(int)DernierPointTraite,VoieManip[voie].nom,(int)suivi->date_evt,VoieTampon[voie].signal.climbs,(int)delta_points,(int)TrmtDernierTrig);
										if(!VoieTampon[voie].delai_template) {
											/* date_evt inchange si TRGR_DATE_MONTEE */
											if(SettingsTempsEvt != TRGR_DATE_MONTEE) suivi->date_evt = DernierPointTraite - (1 * delta_points); /* (SettingsTempsEvt == TRGR_DATE_MAXI) */
											//- printf("(%s) A p=%d: evenement vu sur %s date a p=%d +%d +/-%d, precedent a p=%d\n",__func__,(int)DernierPointTraite,VoieManip[voie].nom,(int)suivi->date_evt,VoieTampon[voie].signal.climbs,(int)delta_points,(int)TrmtDernierTrig);
										} else {
											suivi->date_evt = suivi->posmax - VoieTampon[voie].delai_template;	 
											if(VerifSuiviTrmtEvts) printf("(%s) %s a p=%lld: recule de p=%lld a p=%lld\n",__func__,VoieManip[voie].nom,DernierPointTraite,suivi->posmax,suivi->date_evt);
										}
										niveau = suivi->mihauteur;
										amplitude = niveau - VoieTampon[voie].signal.base;
										//- if((DernierPointTraite - suivi->date_evt) > 200) printf("(%s) A p=%d: evenement vu sur %s pour p=%d +%d +/-%d, precedent a p=%d\n",__func__,(int)DernierPointTraite,VoieManip[voie].nom,(int)suivi->date_evt,VoieTampon[voie].signal.climbs,(int)delta_points,(int)TrmtDernierTrig);
										TrmtCandidatSignale(0,voie,suivi->date_evt,niveau,TRGR_ALGO_NIVEAU);
										suivi->date_evt = 0;
									}
								/* sinon, on regarde le depassement d'un seuil */
								} else {
									seuil_depasse = 0; suivi->pente = VoieTampon[voie].trig.signe;
									if(Trigger.type == TRGR_SCRPT) {
										*(VoieTampon[voie].adrs_niveau) = reelle;
										CebExec(TrmtPrgm,0);
										if(*TrgrResultat) seuil_depasse = 1;
									} else /* (Trigger.type == TRGR_CABLE) */ {
										if(VoieTampon[voie].trig.signe > 0) seuil_depasse = (reelle > VoieTampon[voie].trig.trgr->minampl);
										else if(VoieTampon[voie].trig.signe < 0) seuil_depasse = (reelle < VoieTampon[voie].trig.trgr->maxampl);
										else {
											if(reelle > VoieTampon[voie].trig.trgr->minampl) {
												seuil_depasse = 1; suivi->pente = 1;
		//										if(DernierPointTraite < 10) printf("==> SEUIL DEPASSE\n");
											} else if(reelle < VoieTampon[voie].trig.trgr->maxampl) {
												seuil_depasse = 1; suivi->pente = -1;
											}
										}
									}
									if(seuil_depasse) {
                                        /* if(EvtASauver < 20)
                                            printf("Evt %2d: Voie %s = %g, min = %g avec signe %hhd\n",
                                                   Acquis[AcquisLocale].etat.evt_trouves,
                                                VoieManip[voie].nom,reelle,VoieTampon[voie].trig.trgr->minampl,VoieTampon[voie].trig.signe); */
										suivi->date_evt = DernierPointTraite - (1 * delta_points);
										VoieTampon[voie].signal.base = suivi->precedente;
										VoieTampon[voie].signal.climbs = (suivi->pente * VoieTampon[voie].trmt[TRMT_PRETRG].compactage);
										suivi->mihauteur = reelle; /* teste en realite le max de l'evt */
										suivi->posmax = DernierPointTraite;
										TrgrEtat[voie].evt_en_cours = 1;
									}
								}
							} /* Fin des cas de trigger */

		/* ...............  Fin de recherche d'evenement ........................................ */
//							VoieTampon[voie].suivi.precedente = courante;
//							VoieTampon[voie].suivi.pente = pente;
//							VoieTampon[voie].suivi.dernierpic = dernierpic;
//							VoieTampon[voie].suivi.date_evt = date_evt;
//							VoieTampon[voie].suivi.mihauteur = mihauteur;
//						#ifdef RL
//							VoieTampon[voie].suivi.posmax = posmax;
//						#endif
						#endif /* TRIGGER_DEVELOPPE */
							VoieTampon[voie].suivi.precedente = reelle;
						}
					}

					if(inhibe_en_cours) continue;
#ifdef UTILISE_EN_DEMARRAGE
					if(TrgrEtat[voie].en_demarrage) {
						if(voie_traitee) {
							/* printf("          . Voie %s demarree, recherche mode %d %s, %sen ministream\n",VoieManip[voie].nom,
								VoieTampon[voie].trig.cherche,(Trigger.actif)?"active":"suspendue",MiniStream?"":"pas "); */
							TrgrEtat[voie].en_demarrage = 0;
						}
						continue;
					}
#endif
				}
				if(debug_cpu) {
					temps_lecture = TempsTotalLect - n3;
					tt = (VerifTempsPasse? DateMicroSecs(): 0);
					temps_utilise = tt - t3;
					// if(debug_cpu && declenche) { printf("%13s @%7lld: traitmt  mesure fin @%lld (%lld us)\n",VoieManip[voie].nom,DernierPointTraite,tt,temps_utilise); nb_msgs++; }
					if(temps_utilise > 30) { printf("        ! Duree traitmt  %13s @%lld: %lld - %lld us\n",VoieManip[voie].nom,DernierPointTraite,temps_utilise,temps_lecture); nb_msgs++; }
				}
			}
/* ........ Fin de la boucle sur les voies ............................................ */
			if(TrmtMesureFineActive) {
				TrmtMesureCpu("Total",t2,&nb_voies);
				if(limite_fine > (debut_mesures + 20)) { // points_dispo
					printf("!! Abandon par jet de l'eponge de l'appel #%d demarrant a %lld, limite prevue a %lld\n",TrmtAppelsNb,vrai_dernier,limite_fine);
					TrmtMesureFineActive = 0; TrmtMesureFineTerminee = 1;
				}
			}
			if(VerifCpuCalcul) {
				temps_lecture = TempsTotalLect - n2; // ne joue pas, on est en multi-thread (cf ci-dessous "ABANDONNE")
			#ifdef ABANDONNE
				if(debug_cpu) {
					if(temps_utilise > 30) { printf("        ! Duree traitmt  %13s @%lld: %lld - %lld us (%.3f/voie * %d voie%s)\n",
						"total",DernierPointTraite,temps_utilise,temps_lecture,(float)(temps_utilise-temps_lecture)/(float)nb_voies,Accord1s(nb_voies)); nb_msgs++; /* declenche = 0; */ }
				}
			#endif
				// temps_utilise -= temps_lecture;
				TrmtTempsFiltrage += temps_utilise;
				if(CpuMaxEvt < temps_utilise) { CpuMaxEvt = temps_utilise; CpuStampEvt = DernierPointTraite; }
				cumul_filtrage += temps_utilise; cumul_lecture += temps_lecture;
			}
			if(TrmtRegulModifs) {
				printf("%s/ Ajustement de %d seuil%s\n",DateHeure(),Accord1s(TrmtRegulModifs));
				if(ArchSauve[EVTS]) {
					int secs,usecs;
					SambaTempsEchantillon(DernierPointTraite,ArchT0sec,ArchT0msec,&secs,&usecs);					
					ArchiveSeuils(secs);
				}
				TrmtRegulModifs = 0;
			}
			DernierPointTraite++;
			if(EvtsEmpiles) {
				if(DernierPointTraite > EvtATraiter->fin) {
					if(VerifEvtMoyen || VerifSuiviTrmtEvts) printf("(%s) p=%d: temps traite suffisant pour reconstruire le prochain evenement\n",__func__,(int)DernierPointTraite);
					TrmtConstruitEvt();
				}
			}
		#ifdef DEBUG_TRIGGER
			if(DernierPointTraite > 4000) TrmtDejaDit = 1;
		#endif
/*			if(!Acquis[AcquisLocale].etat.active) break; */
		}
		if(mesure_globale) {
			int nbsecs,nbusecs,trmt_duree;
			gettimeofday(&trmt_fin,0);
			printf("%s.%06d: Traitement termine                    a %16lld\n",DateHeure(),trmt_fin.tv_usec,DernierPointTraite);
			nbsecs = (int)(trmt_fin.tv_sec - trmt_debut.tv_sec);
			nbusecs = trmt_fin.tv_usec - trmt_debut.tv_usec;
			trmt_duree = (nbsecs * 1000000) + nbusecs;
			if(SettingsIntervTrmt > 0) {
				float facteur;
				facteur = 0.1 * (float)trmt_duree / (float)SettingsIntervTrmt;
				if(facteur > 100.0) {
					printf("          ! Traitement non temps-reel, d'un facteur %.3f\n",facteur/100.0);
					/* if(OpiumDisplayed(pLectRegen->cdr) {
					 sprintf(LectInfo[0],"Traitement non temps-reel, d'un facteur %.3f",facteur);
					 PanelRefreshVars(pLectRegen);
					 } */
				} else printf("          > Duree du traitement: %.1f%% du temps-reel\n",facteur);
			}
		}
		if(TrmtModePanique) {
			printf("          | Temps mort: %lld echantillons (%.2f ms) en %d fois\n",
				TrmtPanicAjoute,(float)TrmtPanicAjoute / Echantillonnage,TrmtPanicNb);
			printf("%s/ Le Traitement sort du mode 'SURCHARGE'\n",DateHeure());
			TrmtModePanique = 0; TrmtPanicTotal = 0; mesure_globale = 0;
		}
		if(VerifCpuCalcul)
			printf("%s/ Temps Filtrage  : %7lld + %7lld us pour t = [%7lld .. %7lld[ (%6d donnee%s), %.3f/voie/donnee, %d terme%s de convolution\n",
				DateHeure(),cumul_filtrage,cumul_lecture,vrai_dernier,DernierPointTraite,Accord1s((int)(DernierPointTraite-vrai_dernier)),(float)cumul_filtrage/(float)nb_fltr,Accord1s(nb_convol));
		//* .... Fin   de la boucle sur le temps ................................................. */
	
	#ifdef DEBUG_TRIGGER
		printf("(TrmtSurBuffer) Analyse terminee a p = %d%06d / %d%06d\n",
			(int)(DernierPointTraite/1000000),Modulo(DernierPointTraite,1000000),
			(int)(points_dispo/1000000),Modulo(points_dispo,1000000));
	#endif
		Trigger.analyse = DernierPointTraite;
		// printf("(TrmtSurBuffer) Prochaine analyse a p=%d\n",(int)Trigger.analyse);

/* .... Special production automatique de spectres de bruit ............................. */
		if(LectModeSpectresAuto) {
			int i,j,dim,nbfreq;
			int a_faire,faits; float total;
			int64 pt0,lus,premier;
			float norme;
			fftw_real *ptr_temps; fftw_real niveau_dc;
			char raison[256];
			TypeTamponDonnees *tampon; TypeDonnee *ptr_int16; TypeSignal *ptr_float;
		#define RETIRE_DC

			a_faire = faits = 0; total = 0.0; lus = premier = 0;
			for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue > 0) {
				float unite;
				if(!CalcAutoCarreSomme[voie]) continue;
				if((bolo = VoieManip[voie].det) >= 0) {
					if(!CalcAutoBolo[bolo]) continue;
				}
//				printf("Examen de la voie %s a t1=%d\n",VoieManip[voie].nom,(int)VoieTampon[voie].lus);
				vm = VoieManip[voie].type;
				phys = ModeleVoie[vm].physique;
				if(!(dim = CalcSpectreAuto[phys].dim)) continue;
				a_faire++;
				if((CalcModeAuto != CALC_SPTA_GLISSANT) && (CalcSpectreAutoNb[voie] >= CalcSpectresMax)) {
					faits++; total += CalcSpectreAutoNb[voie]; continue;
				}
				unite = VoieManip[voie].nV_reels; if(unite < 0.0) unite = -unite; // classiquement ADUennV au lieu de nV_reels
				tampon = &(VoieTampon[voie].trmt[CalcDonnees].donnees);
				if(CalcDonnees == TRMT_AU_VOL) {
					// tampon = &(VoieTampon[voie].brutes);
					lus = VoieTampon[voie].lus;
					premier = VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien;
					// a revoir					if((lus < (int64)dim) && (premier <= 0)) goto suite1;
				} else if(CalcDonnees == TRMT_PRETRG) {
					// tampon = &(VoieTampon[voie].traitees);
					lus = tampon->prem + tampon->nb;
					premier = tampon->prem;
				}
				if(dim > tampon->max) dim = tampon->max;
				nbfreq = (dim / 2) + 1;
				pt0 = lus - (int64)dim;
			#ifdef SELECTION_HORS_EVTS
				if(CalcModeAuto == CALC_SPTA_TRIGGE) {
					p1 = tampon->nb;
					of7 = tampon->prem;
					k = CalcSpectresMax - (int)CalcSpectreAutoNb[voie]; if(k == 0) k = 1;
					evt = EvtASauver - 1;
					do {
						p0 = p1 - dim;
						if(p0 < 0) break;
						ok = 1;
						if(saute_evts) {
							debut_brut = (p0 + of7) * compactage;
							while(evt >= 0) {
								for(va=0; va<Evt[evt].nbvoies; va++) if(Evt[evt].voie[va].num == voie) break;
								if(va < Evt[evt].nbvoies) {
									if((Evt[evt].voie[va].debut + (int64)Evt[evt].voie[va].dim) <= debut_brut) break;
									p1 = Evt[evt].voie[va].adresse - of7;
									if(p1 < 0) {
										if(Evt[evt].voie[va].debut < VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien) evt = -1;
										else p1 += VoieTampon[voie].brutes->max;
									}
									ok = 0;
								}
								// printf("- Pb avec evt #%d: %ld + %d > %ld\n",evt,Evt[evt].debut,Evt[evt].dim,debut);
								if(--evt < 0) break;
							}
						}
						if(ok) {
							CalcPi[--k] = p0 + of7;
							// printf("- L'intervalle %d demarre a %g\n",k,CalcTi[k]);
							p1 = p0;
						}
					} while(k > 0);
					//				printf("Intervalles demandes: %d, non calcules: %d\n",f->p.f.intervalles,k);
					if(k < f->p.f.intervalles)
						ok = CalculeFFTVoie(FFT_PUISSANCE,trmt,voie,CalcPi+k,f->p.f.intervalles-k,f->pts,f->p.f.spectre[j],raison);
				}
			#endif
				if(pt0 < premier) continue;
				if(!CalculeFFTplan(&CalcPlanStd,dim,raison)) {
					printf("%s/ ! PAS DE SPECTRE AUTOMATIQUE pour la voie %s: %s\n",DateHeure(),VoieManip[voie].nom,raison);
					LectModeSpectresAuto = 0;
					Acquis[AcquisLocale].etat.active = 0;
					break;
				}
				j = Modulo(pt0,tampon->max);
				ptr_temps = CalcPlanStd.temps; niveau_dc = 0.0;
			#ifdef RETIRE_MOYENNE
				ptr_int16 = tampon->t.sval + j;
				for(i=0; i<dim; i++) {
					// *ptr_temps++ = (fftw_real)*ptr_int16++;
					niveau_dc += (*ptr_temps++ = (fftw_real)*ptr_int16++);
					if(++j >= tampon->max) { j = 0; ptr_int16 = tampon->t.sval; }
				}
				// printf("(%s) niveau DC = %g/%d",__func__,niveau_dc,dim);
				ptr_temps = CalcPlanStd.temps; niveau_dc = niveau_dc / (fftw_real)dim;
				// printf(" =  %g\n",niveau_dc);
				for(i=0; i<dim; i++) *ptr_temps++ -= niveau_dc;
				// printf("(%s) Signal a analyser filtre:",__func__); ptr_temps = CalcPlanStd.temps;
				// for(i=0; i<dim; i++) { if(!(i % 20)) printf("\n%6d:"); printf(" %6.1f",*ptr_temps++); }
				// printf("\n");
			#else
				if(tampon->type == TAMPON_INT16) {
					ptr_int16 = tampon->t.sval + j;
					for(i=0; i<dim; i++) {
						*ptr_temps++ = (fftw_real)*ptr_int16++;
						if(++j >= tampon->max) { j = 0; ptr_int16 = tampon->t.sval; }
					}
				} else {
					ptr_float = tampon->t.rval + j;
					for(i=0; i<dim; i++) {
						*ptr_temps++ = (fftw_real)*ptr_float++;
						if(++j >= tampon->max) { j = 0; ptr_float = tampon->t.rval; }
					}
				}
			#endif

				rfftw_one(CalcPlanStd.plan,CalcPlanStd.temps,CalcPlanStd.freq);
				if(!CalcPlanStd.freq) continue;
				// printf("(%s) Transformee de Fourier:",__func__); ptr_temps = CalcPlanStd.freq;
				// for(i=0; i<dim; i++) { if(!(i % 20)) printf("\n%6d:"); printf(" %6.1f",*ptr_temps++); }
				// printf("\n");
				norme = (VoieEvent[voie].horloge / 1000.0) / (float)dim; /* secondes, soit 1/norme en Hz */
				// printf("(%s) Norme: %g (1/rac(Hz))\n",__func__,norme);
				CalcSpectreAutoNb[voie] += 1.0;
				if(CalcModeAuto == CALC_SPTA_GLISSANT) {
				#ifndef RETIRE_DC
					CalcAutoCarreSomme[voie][0] = (CalcAutoCarreSomme[voie][0] * (1.0 - CalcGlissantEpsilon))
						+ (CalcGlissantEpsilon * CalcPlanStd.freq[0] * CalcPlanStd.freq[0] * norme);
				#endif
					for(i=1; i<nbfreq; i++) {
						CalcAutoCarreSomme[voie][i] = (CalcAutoCarreSomme[voie][i] * (1.0 - CalcGlissantEpsilon))
						+ (CalcGlissantEpsilon * ((CalcPlanStd.freq[i] * CalcPlanStd.freq[i]) + (CalcPlanStd.freq[dim - i] * CalcPlanStd.freq[dim - i])) * norme);
					}
					if(!(dim % 2)) CalcAutoCarreSomme[voie][dim / 2] = (CalcAutoCarreSomme[voie][dim / 2] * (1.0 - CalcGlissantEpsilon))
						+ (CalcGlissantEpsilon * CalcPlanStd.freq[dim / 2] * CalcPlanStd.freq[dim / 2] * norme);
					// printf("(%s) Spectre somme %d fois avec epsilon=%g:",__func__,(int)(CalcSpectreAutoNb[voie]+0.5),CalcGlissantEpsilon);
					// for(i=0; i<nbfreq; i++) { if(!(i % 20)) printf("\n%6d:"); printf(" %6.1f",CalcAutoCarreSomme[voie][i]); }
					// printf("\n");
					// printf("(%s) Spectre %s[%d] calcule sur %d mesure%s\n",__func__,VoieManip[voie].nom,nbfreq,Accord1s((int)(CalcSpectreAutoNb[voie]+0.5))); 
					for(i=0; i<nbfreq; i++) CalcAutoSpectreMoyenne[voie][i] = unite * sqrtf(CalcAutoCarreSomme[voie][i]);
				} else /* if(CalcModeAuto == CALC_SPTA_SOMME ou _TRIGGE) */ {
				#ifndef RETIRE_DC
					CalcAutoCarreSomme[voie][0] += (CalcPlanStd.freq[0] * CalcPlanStd.freq[0] * norme);
				#endif
					for(i=1; i<nbfreq; i++) {
						CalcAutoCarreSomme[voie][i] += (((CalcPlanStd.freq[i] * CalcPlanStd.freq[i]) + (CalcPlanStd.freq[dim - i] * CalcPlanStd.freq[dim - i])) * norme);
					}
					if(!(dim % 2)) CalcAutoCarreSomme[voie][dim / 2] += (CalcPlanStd.freq[dim / 2] * CalcPlanStd.freq[dim / 2] * norme);
					// printf("(%s) Spectre %s[%d] moyenne sur %d mesure%s\n",__func__,VoieManip[voie].nom,nbfreq,Accord1s((int)(CalcSpectreAutoNb[voie]+0.5))); 
					for(i=0; i<nbfreq; i++) {
						CalcAutoSpectreMoyenne[voie][i] = unite * sqrtf(CalcAutoCarreSomme[voie][i] / CalcSpectreAutoNb[voie]);
					}
					if(CalcSpectreAutoNb[voie] >= CalcSpectresMax) faits++;
				}
			#ifdef RETIRE_DC
				CalcAutoCarreSomme[voie][0] = CalcAutoCarreSomme[voie][1];
			#endif
				total += CalcSpectreAutoNb[voie];
			}
			CalcAvance = 100.0 * total / (float)(a_faire * CalcSpectresMax);
			if(OpiumDisplayed(iCalcAvancement->cdr)) InstrumRefreshVar(iCalcAvancement);
			if(LectModeSpectresBanc > 0) BancSptrRafraichi((int)CalcAvance,LectModeSpectresBanc-1);
			// printf("%s/ %d fait%s, avancement: %g/(%d x %d), soit %g pour-cent\n",DateHeure(),Accord1s(faits),total,a_faire,CalcSpectresMax,CalcAvance);
			if(faits >= a_faire) {
				LectModeSpectresAuto = 0; LectNextSeq = 0;
				if(LectAutreSequence) CalcSpectreAutoSauve(); else {
					// printf("%s/ %d faits => avancement: %g/(%d x %d), soit %g pour-cent\n",DateHeure(),faits,total,a_faire,CalcSpectresMax,CalcAvance);
					Acquis[AcquisLocale].etat.active = 0; printf("%s/ %d spectre%s termine%s\n",DateHeure(),Accord2s(faits));
				}
			}
		}
	}
	temps_lecture = TempsTotalLect - n0;
	TrmtNbLus += temps_lecture;
	if(VerifTempsPasse) {
		TrmtHeureDernier = DateMicroSecs();
		//	t1 = TrmtHeureDernier - t0;
		//	if(t1 > 30000000) printf("%s/ Temps de traitement: %6d,%06d secs\n",DateHeure(),(int)(t1/1000000),Modulo(t1,1000000));
		temps_utilise = TrmtHeureDernier - t0 - temps_lecture;
		TempsTotalTrmt += temps_utilise;
		TempsTempoTrmt += temps_utilise;
		if(CpuMaxTrigger < temps_utilise) { CpuMaxTrigger = temps_utilise; CpuStampTrigger = vrai_dernier; CpuFinTrigger = Trigger.analyse; }
	}
	if(VerifCalculDetail) {
		printf("%s/ Temps Processing: %7lld + %7lld us pour t = [%7lld .. %7lld[ (%6d donnee%s), %.3f/donnee\n",
			DateHeure(),temps_utilise-cumul_filtrage,temps_lecture-cumul_lecture,vrai_dernier,Trigger.analyse,Accord1s((int)(Trigger.analyse-vrai_dernier)),(float)(temps_utilise-cumul_filtrage)/(float)(Trigger.analyse-vrai_dernier));
		printf("          -----\n");
	}
	if(VerifCpuCalcul) printf("%s/ fin traitmt [%7lld .. %7lld[ @%lld (cumul lecture=%lld), %.3f + %.3f /donnee\n",DateHeure(),vrai_dernier,Trigger.analyse,TrmtHeureDernier,TempsTotalLect,
		(float)(TrmtHeureDernier - t0)/(float)(Trigger.analyse-vrai_dernier),(float)temps_lecture/(float)(Trigger.analyse-vrai_dernier));
	//+ printf("%8lld      fin   traitement\n",TrmtHeureDernier-LectT0Run);
	cpu_utilise = VerifConsoCpu? clock() - c0: 0;
	CpuTempoTrmt += cpu_utilise;
	CpuTotalTrmt += cpu_utilise;
	// printf("(%s) %lld echantillons traites\n",__func__,DernierPointTraite);
}
/* ========================================================================== */
void TrmtStop() {
#ifdef PAW_H
	if(ArchSauveNtp) PawFileClose(HbId);
#endif
}
//-- #else
//-- /* ========================================================================== */
//-- int TrmtRAZ() { return(1); }
//-- /* ========================================================================== */
//-- void TrmtSurBuffer() { }
//-- #endif
