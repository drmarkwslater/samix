// #ifdef macintosh
// #pragma message(__FILE__)
// #endif
#include <environnement.h>
#define DEFAUT_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef CODE_WARRIOR_VSN
	#include <sys/types.h>  // tout ca ..
	#include <sys/uio.h>	// .. pour ..
	#include <unistd.h>     // .. read !!
#endif

#ifdef CMDE_UNIX
	#define L_(texte) texte
	#define LL_(liste) liste
#else
	#include <opium.h>
#endif
#include <decode.h>
#include <rundata.h>
#include <archive.h>
#ifdef Samba /* pour ModeleBN, pas necessaire dans TANGO */
	#include <samba.h>
	#include <numeriseurs.h>
	#include <cablage.h>
	#include <repartiteurs.h>
#else
	#define CABLAGE_INDEFINI 0xFF
	#define CABLAGE_GALETTE(pos_hexa) ((pos_hexa >> 4) & 0xF)
	#define CABLAGE_TOUR(pos_hexa) (pos_hexa & 0xF)
#endif
// #include <objets_samba.h>

static char SambaCodePosition[32];

void RazVoiesManip();
void ReglageRazBolo(TypeDetecteur *detectr, char *nom);

/* ========================================================================== */
void SambaDefaults() {
	
	/* Initialisation du logiciel */
	strcpy(ArchiveChemin[EVTS],"inconnu");
	strcpy(ArchiveChemin[STRM],"inconnu");
	ArchNom[0] = '\0';
	strcpy(ArchDate,"inconnue");
	/*	Repartiteur et numeriseurs version 0
	 Voie8bits = 3;
	 strcpy(VoieStatus,VoieManip[Voie8bits].nom);
	 strcpy(VoiesConnues,"chaleur~/centre/garde"); */
	strcpy(VoieStatus,"neant"); Voie8bits = VOIES_TYPES;
	BoloNb = 0;
	
	/* Intialisation du parametrage */
	strcpy(SambaIntitule,"Standard");
	BoloGeres = 8;
	ModeleVoieNb = 3;
	VoiesGerees = BoloGeres * ModeleVoieNb;
	HorlogeSysteme = 60.0; Diviseur0 = 10; Diviseur1 = 60;
	Echantillonnage = 100.0;
	Diviseur2 = 100;   /* arbitraire.. mais il le faut, si! si! */
	Diviseur3 = 1000;  /* arbitraire.. mais il le faut aussi, si, si.. */
	VersionIdentification = 1.0;
	FIFOdim = 256 * KB;  /* mettre 8K en standard */
	strcpy(RunCategCles,"labo/calb/fond/dbug"); // "test/co60/co57/cesm/calf/b133/laser/fond");
	// strcpy(RunEnvir,"fond chateau ferme/fond chateau ouvert/calibration gamma/calibration compton/calibration neutron/calibration/test");
	SrceListe[SRC_SIMU] = "simu";
	SrceListe[SRC_REEL] = "reel";
	SrceListe[SRC_EDW0] = "edw0";
	SrceListe[SRC_EDW1] = "edw1";
	SrceListe[SRC_EDW2] = "edw2";
	SrceListe[SRC_NB]   = "\0";
	SrceNoms = LL_(SrceListe); ArgKeyFromList(SrceCles,SrceNoms);
	SrceType = SRC_REEL;    /* --> Vraie acquisition !!      */
	SrceName[0] = '\0';
	strcpy(LangDir,"standard");
	strcpy(LangueDemandee,"native");
	ArchEnvirDesc = EnvirModlDesc;
	LectEnvirDesc = 0;
	ArchAutomInfo = 0;
	ArchTriggerInfo = 0;
}
/* ========================================================================== */
void TangoDefaults() {
	strcpy(SauvVol,"donnees");
	strcpy(EvtsDir,"events");
	OptnSpec[0] = '\0';
	EventName[0] = '\0';
	MaxEvts = 10000;
	strcpy(PrefsTango,"prefs");
	strcpy(ResuPath,"resultats");
	strcpy(FigPath,"figures");
	strcpy(DbazPath,"db");
	CommandeDirecte[0] = '\0';
	Rapide = 1; Tolerant = 0;
	ModeScript = SousUnix = 0;
	FondNoir = 1;
	strcpy(NomFiltres,"Filtres");
	strcpy(AnaDir,"Analyse");
	strcpy(CalibName,"evt");
	strcpy(LangDir,"standard");
	strcpy(LangueDemandee,"native");
	ArchEnvirDesc = EnvirModlDesc;
	LectEnvirDesc = 0;
	ArchAutomInfo = 0;
	ArchTriggerInfo = 0;
}
/* ========================================================================== */
void SettingsDefaults() {
	DureeTampons = 10000;         /* ms */
	SettingsModeDonnees = 1;      /* 0:fixe / 1:circulaire         */
	SettingsMultitasks = 3;       /* 0: lecture monotache synchrone / 1: multitaches asynchrone */
	SettingsReadoutPeriod = 10;   /* intervalle d'activation du readout (ms) */
	SettingsDepileMax = 1024;
	SettingsStatusCheck = 1;
	SettingsAutoSync = 0;
	SettingsChargeReglages = 0;
	SettingsChargeBolos = 1;
	SettingsPreTrigger = 25;
	SettingsCalculEvt = NEANT;
	SettingsCoupures = 0;
	SettingsCoherenceEvt = 0;
	TrmtDeconvolue = 0;
	
	SettingsTempsEvt = TRGR_DATE_MAXI;
#ifdef ALTIVEC
	SettingsAltivec = 1;
#else
	SettingsAltivec = 0;
#endif
	SettingsSoustra = TRMT_PTN_AVANT;
	SettingsSauteEvt = 0;
	SettingsSansTrmt = 0;
	SettingsStartactive = 1;
	SettingsDLUactive = 0;
	SettingsCalageType = CALAGE_PAR_PHYSIQUE;
	SettingsCentrageType = NEANT;
	SettingsCalcPretrig = 1;
	strcpy(SettingsCalageNom,"auto");
	strcpy(SettingsCentrageNom,"auto");
}
/* ========================================================================== */
void TrmtNomme() {

#undef printf
	TrmtClasseNom[TRMT_AU_VOL] = "au vol";
	TrmtClasseNom[TRMT_PRETRG] = "pretrigger"; // "Sur tampon";
	TrmtClasseNom[TRMT_NBCLASSES] = "\0";

	TrmtClasseTitre[TRMT_AU_VOL] = "Au vol";
	TrmtClasseTitre[TRMT_PRETRG] = "Pretrigger"; // "Sur tampon";
	TrmtClasseTitre[TRMT_NBCLASSES] = "\0";
//	TrmtClasseNoms = LL_(TrmtClasseTitre);

	TrmtTypeListe[NEANT] = "neant";
	TrmtTypeListe[TRMT_DEMODUL] = "demodulation";
#ifdef TRMT_ADDITIONNELS
	TrmtTypeListe[TRMT_LISSAGE] = "lissage";
	TrmtTypeListe[TRMT_MOYENNE] = "moyenne";
	TrmtTypeListe[TRMT_MAXIMUM] = "maximum";
	TrmtTypeListe[TRMT_DECONV] = "deconvolue";
	#ifdef A_LA_GG
		TrmtTypeListe[TRMT_INTEDIF] = "intedif";
	#endif /* A_LA_GG */
	#ifdef A_LA_MARINIERE
		TrmtTypeListe[TRMT_CSNSM] = "csnsm";
	#endif /* A_LA_MARINIERE */
#endif /* TRMT_ADDITIONNELS */
	TrmtTypeListe[TRMT_FILTRAGE] = "filtrage";
	TrmtTypeListe[TRMT_INVALID] = "invalidation";
	TrmtTypeListe[TRMT_NBTYPES] = "\0";
	/* { int i;
		printf("(%s) TrmtTypeListe[%d] = {\n",__func__,TRMT_NBTYPES);
		i = 0; while(TrmtTypeListe[i][0] != '\0') {
			if( i && !(i%5)) printf("\n%c",0x09); else if(i) printf(", "); else printf("%c",0x09);
			printf("[%d]: '%-15s'",i,TrmtTypeListe[i]); i++;
		}
		printf("\n}\n");
	} */
	TrmtTypeNoms = LL_(TrmtTypeListe);
	ArgKeyFromList(TrmtTypeCles,TrmtTypeNoms);
	// printf("(%s) TrmtTypeCles = '%s'\n",__func__,TrmtTypeCles);

	TrmtCalculListe[NEANT] = "neant";
	TrmtCalculListe[TRMT_LDB] = "LdB";
	TrmtCalculListe[TRMT_AMPLMAXI] = "max";
	TrmtCalculListe[TRMT_INTEGRAL] = "integ";
	TrmtCalculListe[TRMT_EVTMOYEN] = "evt-unite";
	TrmtCalculListe[TRMT_NBEVTCALCULS] = "\0";
	TrmtCalculNoms = LL_(TrmtCalculListe);
	ArgKeyFromList(TrmtCalculCles,TrmtCalculNoms);
	
	TrmtLibelleListe[NEANT] = "sommaire";
	TrmtLibelleListe[TRMT_LDB] = "avec lissage ligne de base";
	TrmtLibelleListe[TRMT_AMPLMAXI] = "par verification de l'amplitude max";
	TrmtLibelleListe[TRMT_INTEGRAL] = "amplitude max + integrale";
	TrmtLibelleListe[TRMT_EVTMOYEN] = "par convolution avec l'evenement moyen";
	TrmtLibelleListe[TRMT_NBEVTCALCULS] = "\0";
	TrmtLibelleNoms = LL_(TrmtLibelleListe);
	
	TrmtTemplateListe[NEANT] = "neant";
	TrmtTemplateListe[TRMT_TEMPL_DB] = "database";
	TrmtTemplateListe[TRMT_TEMPL_ANA] = "analytique";
	TrmtTemplateListe[TRMT_NBTEMPL] = "\0";
	TrmtTemplateNoms = LL_(TrmtTemplateListe);
	ArgKeyFromList(TrmtTemplateCles,TrmtTemplateNoms);
	
	TrgrTypeListe[NEANT] = "neant";
	TrgrTypeListe[TRGR_SEUIL] = "seuil";
	TrgrTypeListe[TRGR_FRONT] = "front";
	TrgrTypeListe[TRGR_DERIVEE] = "derivee";
	TrgrTypeListe[TRGR_ALEAT] = "random";
	TrgrTypeListe[TRGR_PORTE] = "porte";
	TrgrTypeListe[TRGR_EXT] = "externe";
	TrgrTypeListe[TRGR_TEST] = "test";
	TrgrTypeListe[TRGR_NBCABLAGES] = "\0";
	TrgrTypeNoms = LL_(TrgrTypeListe);
	ArgKeyFromList(TrgrTypeCles,TrgrTypeNoms);
	
	TrgrDefListe[NEANT] = "neant";
	TrgrDefListe[TRGR_CABLE] = "cable";
	TrgrDefListe[TRGR_SCRPT] = "scrpt";
	TrgrDefListe[TRGR_PERSO] = "perso";
	TrgrDefListe[TRGR_NBPRGM] = "\0";
	TrgrDefNoms = LL_(TrgrDefListe);
	ArgKeyFromList(TrgrDefCles,TrgrDefNoms);
	
	TrgrOrigineListe[TRGR_INTERNE] = "interne";
	TrgrOrigineListe[TRGR_DEPORTE] = "deporte";
	TrgrOrigineListe[TRGR_FPGA] = "fpga";
	TrgrOrigineListe[TRGR_NBORIGINES] = "\0";
	TrgrOrigineNoms = LL_(TrgrOrigineListe);
	ArgKeyFromList(TrgrOrigineCles,TrgrOrigineNoms);

	TrgrDateListe[TRGR_DATE_MONTEE] = "montee";
	TrgrDateListe[TRGR_DATE_MAXI] = "maxi";
	TrgrDateListe[TRGR_NBDATES] = "\0";
	TrgrDateNoms = LL_(TrgrDateListe);
	ArgKeyFromList(TrgrDateCles,TrgrDateNoms);
	
	strcpy(Trigger.nom_scrpt,"script_trigger");
	
	ArgKeyFromList(ArchClesStream,ArchNomStream); /* utilise aussi dans les repartiteurs (FAMILLE_ARCHIVE) */

	strcpy(TrmtRegulAction,L_(TrmtRegulTextes));
	
	ArchHdrNbDesc[STRM] = ArchHdrStrmNbDesc; ArchHdrNbDesc[EVTS] = ArchHdrEvtsNbDesc;
	
/*	printf("Options de traitement:\n");
	printf("  Types de traitement  : %s\n",TrmtTypeCles);
	printf("  Infos d'evenement    : %s\n",TrmtCalculCles);
	printf("  Origines de template : %s\n",TrmtTemplateCles);
	printf("  Origines de trigger  : %s\n",TrgrDefCles);
	printf("  Cablages de trigger  : %s\n",TrgrTypeCles);
	printf("  Datations d'evenement: %s\n",TrgrDateCles);
	printf("  Formats de streams   : %s\n\n",ArchClesStream); */
}
/* ========================================================================== */
char SambaAlloc() {
	int bolo,voie;
	char nom[16];
	
	//- if(Echantillonnage < 0.0) Echantillonnage = (HorlogeSysteme * 1000.0) / (float)(Diviseur0 * Diviseur1); /* kHz */
	if(VoiesGerees > VOIES_MAX) {
		printf("SAMBA ne gere que VOIES_MAX=%d voies au maximum, et non %d (voir rundata.h)\n",VOIES_MAX,VoiesGerees);
		return(0);
	}
	DetecteursMax = DETEC_MAX;

#ifdef SETUP_RESEAU
	if((SambaMode == SAMBA_DISTANT)) {
		int taille_bolos;
		if(BoloGeres >= DETEC_MAX) /* vraiment utile? */ {
			printf("SAMBA ne gere que DETEC_MAX=%d detecteurs au maximum, et non %d (voir rundata.h)\n",DETEC_MAX,BoloGeres);
			return(0);
		}
		DetecteursMax = BoloGeres;
		taille_bolos = (BoloGeres + 1) * sizeof(TypeDetecteur);
		taille_voies = VoiesGerees * sizeof(TypeVoieAlire);
		Bolo = (TypeDetecteur *)CarlaDataShareDyn("Bolometres",&IdBolos,taille_bolos,CARLA_NOINIT,CARLA_TEMPO);
		if(Bolo == 0) {
			printf("Allocation de l'objet Bolo[%d] impossible\n",taille_bolos);
			return(0);
		} //  else printf("Bolo       @%08X\n",(hexa)Bolo);
		VoieManip = (TypeVoieAlire *)CarlaDataShareDyn("Voies",&IdVoies,taille_voies,CARLA_NOINIT,CARLA_TEMPO);
		if(VoieManip == 0) {
			printf("Allocation de l'objet VoieManip[%d] impossible\n",taille_voies);
			return(0);
		} //  else printf("VoieManip  @%08X\n",(hexa)VoieManip);
	}
#endif

	VoieEvent = Malloc(VoiesGerees,TypeVoieDonnees);
	if(VoieEvent == 0) {
		printf("Allocation de VoieEvent[%d] impossible\n",VoiesGerees);
		return(0);
	}
    for(voie=0; voie<VoiesGerees; voie++) {
        VoieEvent[voie].brutes.max = 0; // VoieEvent[voie].lngr;
        VoieEvent[voie].brutes.t.sval = 0; // (TypeDonnee *)malloc(VoieEvent[voie].lngr * sizeof(TypeDonnee));
    }
    // printf("Allocation de %d voies d'evenement, initialisation des tampons 'brutes.t.sval' a 0\n",VOIES_MAX);
    
	for(bolo=0; bolo<DetecteursMax+1; bolo++) {
		BoloNom[bolo] = Bolo[bolo].nom;
		nom[0] = '\0'; ReglageRazBolo(Bolo+bolo,nom); // sprintf(nom,"Bolo%d",bolo);
	}
	BoloNom[bolo] = "\0";
	for(voie=0; voie<VOIES_MAX; voie++) VoieNom[voie] = VoieManip[voie].nom;
	VoieNom[voie] = "\0";
	RazVoiesManip();
	
	return(1);
}
/* ========================================================================== */
void RazVoiesManip() {
	int voie,classe;
	
	/* Definition des voies en general */
	for(voie=0; voie<VoiesGerees; voie++) {
		bzero(&(VoieManip[voie]),sizeof(TypeVoieAlire));
		sprintf(VoieManip[voie].nom,"voie %d",voie+1);
		VoieManip[voie].def.evt.duree = 512;
		VoieManip[voie].def.evt.delai = 0;
		VoieManip[voie].def.evt.interv = 100;
		VoieManip[voie].min = -8192;   /* Mini et maxi du signal recu */
		VoieManip[voie].max = 8191; // (1 << ADCbits) - 1;
		VoieManip[voie].modulee = 0;
		VoieManip[voie].gain_fixe = 1.0;
		VoieManip[voie].gain_variable = 1.0;
#ifdef RL
		VoieManip[voie].def.evt.dimtemplate		= 64;   /*nombre de point actif pour filtrage template*/
		VoieManip[voie].def.evt.montee			= 2. ;  /* temps de montee de l'evenement (ms) */ 
		VoieManip[voie].def.evt.descente1		= 15.;  /* temps de descente rapide (ms) */
		VoieManip[voie].def.evt.descente2		= 100;  /* temps de descente lente (ms) */
		VoieManip[voie].def.evt.facteur2		= 0.3;  /* fraction de descente lente  */
#endif
		/* Aucun traitement par defaut */
		for(classe=0; classe<TRMT_NBCLASSES; classe++) {
			VoieManip[voie].def.trmt[classe].type = NEANT;
			VoieManip[voie].def.trmt[classe].parm = 1;
			VoieManip[voie].def.trmt[classe].gain = 1;
			//			strcpy(VoieManip[voie].def.trmt[classe].texte,"neant");
		}
		/* Aucun trigger par defaut */
		VoieManip[voie].def.trgr.type = NEANT;
		VoieManip[voie].def.trgr.sens = 2;
		VoieManip[voie].def.trgr.minampl = 50.0;
		VoieManip[voie].def.trgr.maxampl = -VoieManip[voie].def.trgr.minampl;
		VoieManip[voie].def.trgr.montee = VoieManip[voie].def.evt.duree / 50.0;
		VoieManip[voie].def.trgr.porte = VoieManip[voie].def.trgr.montee * 2;
		VoieManip[voie].def.trgr.underflow = 0.0;
		VoieManip[voie].def.trgr.overflow = 0.0;
		VoieManip[voie].def.trgr.montmin = VoieManip[voie].def.evt.duree / 1000.0;
		VoieManip[voie].def.trgr.montmax = VoieManip[voie].def.evt.duree * 50.0;
		VoieManip[voie].def.trgr.maxbruit = 999999.9;
		VoieEvent[voie].filtre = 0;
	}
}
/* ========================================================================== */
void ReglageRazBolo(TypeDetecteur *detectr, char *nom) {
	int cap,vm;
	
	bzero(detectr,sizeof(TypeDetecteur));
	if(nom) strncpy(detectr->nom,nom,DETEC_NOM);
	detectr->pos = CABLAGE_INDEFINI;
	detectr->ref = -1;
	detectr->lu = DETEC_HS;
	detectr->mode_arch = ARCH_NBMODES;
	detectr->masse = 0.0;
	for(vm=0; vm<DETEC_ASSOC_MAX; vm++) detectr->assoc[vm] = -1;
	for(cap=0; cap<DETEC_CAPT_MAX; cap++) {
		detectr->captr[cap].voie = -1;
		detectr->captr[cap].bb.num = -1; detectr->captr[cap].bb.serial = 0xFF; detectr->captr[cap].bb.adrs = 0xFF;
	}
	detectr->d2 = 100;
	detectr->d3 = 1000;
}
/* ========================================================================== */
static char *SambaEncodePosition(unsigned char pos_hexa) {
	if(pos_hexa == 0xFF) strcpy(SambaCodePosition,"---");
	else sprintf(SambaCodePosition,"%c%-2d",'a'+CABLAGE_GALETTE(pos_hexa),CABLAGE_TOUR(pos_hexa));
	return(SambaCodePosition);
}
/* ========================================================================== */
void SambaEnvirCreeDesc(void *arg) {
	ArgDesc *desc; int i;

	//- printf("(%s) Execution demarree pour %d variable%s demandee%s\n",__func__,Accord2s(EnvirVarNb));
	RunTypeName[0] = '\0'; RunTypeVar = -1;
	if(EnvirVarNb) {
		LectEnvirDesc = ArgCreate(EnvirVarNb);
		desc = LectEnvirDesc;
		for(i=0; i<EnvirVarNb; i++) {
			if((EnvirVar[i].type == ARG_TYPE_TEXTE) || ((EnvirVar[i].type >= ARG_TYPE_LISTE) && (EnvirVar[i].type != ARG_TYPE_RIEN))) {
				printf("  ! Type de variable ingerable: %s\n",ArgNomType[EnvirVar[i].type]);
#ifdef OPIUM_H
				OpiumError("Type de variable d'environnement ingerable: %s",ArgNomType[EnvirVar[i].type]);
#endif
				continue;
			}
			if(EnvirVar[i].type == ARG_TYPE_KEY)
				ArgAdd(desc++,EnvirVar[i].nom,DESC_KEY EnvirVar[i].mot_cles,(void *)&(EnvirVar[i].var),EnvirVar[i].explics);
			else ArgAdd(desc++,EnvirVar[i].nom,0,(char)EnvirVar[i].type,0,0,1,0,0,(void *)&(EnvirVar[i].var),EnvirVar[i].explics);
			if(!strcmp(EnvirVar[i].nom,"Type")) RunTypeVar = i;
			switch(EnvirVar[i].type) {
				case ARG_TYPE_KEY: case ARG_TYPE_CHAR: case ARG_TYPE_OCTET: case ARG_TYPE_BYTE: case ARG_TYPE_BOOL:
					*(char *)(&(EnvirVar[i].var)) = 0; break;
				case ARG_TYPE_INT: case ARG_TYPE_HEXA: *(int *)(&(EnvirVar[i].var)) = 0; break;
				case ARG_TYPE_SHORT: case ARG_TYPE_SHEX: *(short *)(&(EnvirVar[i].var)) = 0; break;
				case ARG_TYPE_FLOAT: *(float *)(&(EnvirVar[i].var)) = 0.0; break;
				case ARG_TYPE_LETTRE: *(char *)(&(EnvirVar[i].var)) = 'a'; break;
			}
		}
	} else desc = LectEnvirDesc;
//	printf("(%s:%d) Execution terminee, %d variable%s enregistree%s @ %08lX\n",__func__,__LINE__,Accord2s((int)(desc-LectEnvirDesc)),(lhex)LectEnvirDesc);
	// if(LectEnvirDesc && EnvirVarNb) { printf("(%s:%d) %d variable%s d'environnement:\n",__func__,__LINE__,Accord1s(EnvirVarNb)); ArgPrint("*",LectEnvirDesc); }
}
/* ========================================================================== */
void SambaDumpBolos() {
	int bolo; char hote[20];
	
	printf(" ____________________________________________________________________________________\n");
	printf("|                               Cablage des detecteurs                               |\n");
	printf("|____________________________________________________________________________________|\n");
	printf("|                        | position |  prise   |    numeriseur    | systeme d'       |\n");
	printf("|       detecteur        | interne  | cryostat | #serie   adresse | acquisition      |\n");
	printf("|________________________|__________|__________|________|_________|__________________|\n");
	for(bolo=0; bolo<BoloNb; bolo++) {
		strncpy(hote,Bolo[bolo].hote,16); hote[16] = '\0';
		printf("| %2s: %-18s |   %-4s   |  .....   |  #%03d  |    %02X   | %-16s |\n",
			Bolo[bolo].lu?"ok":"HS",Bolo[bolo].nom,
			SambaEncodePosition(Bolo[bolo].pos),Bolo[bolo].captr[0].bb.serial,Bolo[bolo].captr[0].bb.adrs,hote);
	}	
	printf("|________________________|__________|__________|________|_________|__________________|\n\n");
}
/* ========================================================================== */
int SambaDumpVoies(char toutes, char composition) {
	int voie,bolo; char pseudo_trouvee;
	TypeComposantePseudo *composante; float coef;

	pseudo_trouvee = 0;
    printf(" ____________________________________________________________________________________\n");
    printf("|                              Liste des voies %-7s                               |\n",toutes? "": "locales");
    printf("|____________________________________________________________________________________|\n");
    printf("|      Nom              | type             detecteur      | bits |  gain   R ADU(uV) |\n");
    printf("|_______________________|_________________________________|______|___________________|\n"); 
    for(voie=0; voie<VoiesNb; voie++) {
		if((bolo = VoieManip[voie].det) >= 0) {
			if(Bolo[bolo].local || toutes) {
				if(VoieManip[voie].pseudo) {
					printf("| %3d: %16s | %-16s %-14s | %04X | %5.0f %5.0f %6.2f|\n",voie+1,VoieManip[voie].nom,
						"(composee)",Bolo[bolo].nom,
						VoieManip[voie].ADCmask,VoieManip[voie].gain_fixe,VoieManip[voie].Rmodul,
						VoieManip[voie].ADUenmV*1000.0);
					pseudo_trouvee = 1;
				} else printf("| %3d: %16s | %-16s %-14s | %04X | %5.0f %5.0f %6.2f|\n",voie+1,VoieManip[voie].nom,
					ModeleVoie[(int)VoieManip[voie].type].nom,Bolo[bolo].nom,
					VoieManip[voie].ADCmask,VoieManip[voie].gain_fixe,VoieManip[voie].Rmodul,
					VoieManip[voie].ADUenmV*1000.0);
			} else printf("| %3d: %16s | %-16s %-14s | etranger                 |\n",voie+1,VoieManip[voie].nom,
						  ModeleVoie[(int)VoieManip[voie].type].nom,Bolo[bolo].nom);
		} else printf("| %3d: %16s | %-16s %-14s | %04X |                   |\n",voie+1,VoieManip[voie].nom,
				"directe","",VoieManip[voie].ADCmask);
    }
    printf("|_______________________|_________________________________|______|___________________|\n\n");

	if(composition && pseudo_trouvee) {
		int lngr,k;
		lngr = 
		printf(" ____________________________________________________________________________________\n");
		printf("|                               Liste des voies composees                            |\n");
		printf("|____________________________________________________________________________________|\n");
		printf("|      Nom              | Composition                                                |\n");
		printf("|_______________________|____________________________________________________________|\n");
		for(voie=0; voie<VoiesNb; voie++) if((bolo = VoieManip[voie].det) >= 0) {
			if((Bolo[bolo].local || toutes) && VoieManip[voie].pseudo) {
				k = printf("| %3d: %16s | ",voie+1,VoieManip[voie].nom);
				composante = VoieManip[voie].pseudo;
				k += printf(" %g x %s",composante->coef,VoieManip[composante->srce].nom);
				composante = composante->svt;
				while(composante) {
					if(composante->coef >= 0.0) { k += printf(" + "); coef = composante->coef; }
					else { k += printf(" - "); coef = -composante->coef; }
					k += printf(" %g x %s",coef,VoieManip[composante->srce].nom);
					composante = composante->svt;
				}
			#ifdef Samba
				SambaLigneTable(' ',k,lngr);
			#else
				{ int i; for(i=k; i<(lngr-1); i++) printf(" "); printf("|\n"); }
			#endif
			}
		}
		printf("|_______________________|____________________________________________________________|\n\n"); 
	}
	return(0);
}
/* ========================================================================== */
int SambaDumpVoiesToutes() { return(SambaDumpVoies(1,1)); }
/* ========================================================================== 
int SambaDumpVoiesLues() {
	int vt,voie,bolo;

    printf(" ____________________________________________________________________________________\n");
    printf("|                              Liste des voies lues                                  |\n");
    printf("|____________________________________________________________________________________|\n");
    printf("|      Nom              | type             detecteur      | bits |  gain   R ADU(uV) |\n");
    printf("|_______________________|_________________________________|______|___________________|\n"); 
	for(vt=0; vt<VoiesLues; vt++) {
		if((voie = SambaEchantillon[vt].voie) >= 0) {
			bolo = VoieManip[voie].det;
			if(bolo >= 0) printf("| %3d: %16s | %-16s %-14s | %04X | %5g %5.0f %6.2f|\n",voie+1,VoieManip[voie].nom,
				ModeleVoie[(int)VoieManip[voie].type].nom,Bolo[bolo].nom,
				VoieManip[voie].ADCmask,VoieManip[voie].gain_fixe,VoieManip[voie].Rmodul,
				VoieManip[voie].ADUenmV*1000.0);
			else printf("| %3d: %16s | %-16s %-14s | %04X |                   |\n",voie+1,VoieManip[voie].nom,
				"directe","",VoieManip[voie].ADCmask);
		} else printf("| %3d: %16s |                                 |      |                   |\n",voie+1,"(ignoree)");
    }
    printf("|_______________________|_________________________________|______|___________________|\n\n");
	return(0);
}
   ========================================================================== */
float SambaDeconvolue(float *deconvolue, TypeDonnee *adrs, int dim, float ldb, int pre, int post, float perte, char calcule) {
	// amortismt = 1/RC (RC ~ t) ramene en nombre de points
	int i,di,fin; TypeDonnee *adu;
	float tempo[65536],*entree,*sortie,val,prec,dec;
	float somme,nb,omax,omin,dmax,dmin,norme,depot;
	char positif,sens,min_premier;
	float vmax,vmin; int tmax[65536],tmin[65536]; int nbmax,nbmin; int ppal,dep,arr; float top;

	adu = adrs;
/* pre-lissage */
	omax = omin = 0.0;
	di = pre / 2; if(di < 0) di = 0;
	somme = 0.0; nb = 0.0;
	fin = dim - di;
	entree = sortie = tempo;
	for(i=0; i<dim; i++) {
		val = (float)(*adu++);
		if(di) {
			*entree++ = val;
			if(i < pre) { somme += val; nb += 1.0; }
			else somme += (val - *sortie++);
			val = somme / nb;
		}
		val -= ldb;
		if(i >= di) deconvolue[i - di] = val;
		if((di > 0) && (i >= fin)) deconvolue[i] = val;
		if(val > omax) omax = val;
		if(val < omin) omin = val;
	}
/* deconvolution avec post-lissage */
	dmax = dmin = 0.0; depot = 0.0;
	di = post / 2; if(di < 0) di = 0;
	somme = 0.0; nb = 0.0;
	fin = dim - di;
	entree = sortie = tempo;
	for(i=0; i<dim; i++) {
		val = deconvolue[i];
		if(i == 0) dec = 0.0;
		else dec = val - (prec * perte);
		if(di) {
			*entree++ = dec;
			if(i < pre) { somme += dec; nb += 1.0; }
			else somme += (dec - *sortie++);
			dec = somme / nb;
		}
		if(i >= di) { deconvolue[i - di] = dec; /* depot += dec; trop sommaire si petite charge p.r. bruit */ }
		if((di > 0) && (i >= fin)) deconvolue[i] = dec;
		if(!calcule) {
			if(dec > dmax) dmax = dec;
			if(dec < dmin) dmin = dec;
		}
		prec = val;
	}
	positif = (omax > -omin);

/* evaluation de la charge ... */
	if(calcule) {
		prec = deconvolue[0];
		nbmin = nbmax = 0; sens = 0; ppal = 0;
		tmax[0] = tmin[0] = 0;
		vmax = vmin = top = deconvolue[0];
		for(i=1; i<dim; i++) {
			if(deconvolue[i] > prec) {
				if(sens < 0) {
					vmax = deconvolue[i];
					if(!positif && (vmin < top)) { ppal = nbmin; top = vmin; }
					tmin[nbmin++] = i - 1;
				} else if(deconvolue[i] > vmax) vmax = deconvolue[i];
				sens = 1;
			} else if(deconvolue[i] < prec) {
				if(sens > 0) {
					vmin = deconvolue[i];
					if(positif && (vmax > top)) { ppal = nbmax; top = vmax; }
					tmax[nbmax++] = i - 1;
				} else if(deconvolue[i] < vmin) vmin = deconvolue[i];
				sens = -1;
			}
			prec = deconvolue[i];
		}
		min_premier = (tmin[0] < tmax[0]);
		if(positif) {
			if(min_premier) { dep = tmin[ppal]; arr = tmin[ppal + 1]; }
			else { dep = (ppal > 0)? tmin[ppal-1]: 0; arr = tmin[ppal]; }
		} else {
			if(min_premier) { dep = (ppal > 0)? tmax[ppal-1]: 0; arr = tmax[ppal]; }
			else { dep = tmax[ppal]; arr = tmax[ppal + 1]; }
		}
		depot = 0.0;
		for(i=dep; i<arr; i++) depot += deconvolue[i];

/* ... ou ajustement pour l'affichage */
	} else {
		if(positif && (dmax > 0.0)) norme = (float)omax / dmax;
		else if(dmin < 0.0) norme = (float)omin / dmin;
		else norme = 1.0;
		sortie = deconvolue;
		for(i=0; i<dim; i++, sortie++) *sortie = (*sortie * norme) + ldb;
	}

	return(depot);
}
/* ========================================================================== */
int ArchEdw2Hdr(int f, char imprime_actions, char imprime_entetes, char *explic) {
	char ligne[1024],*c,*item,*nom_bolo; char nom[256];
	float version_lue;
	int i,l; int bolos_geres_avant,voies_gerees_avant;
	char byteswappe;
	char debug_modeles;
	int voie,bolo,cap,vm; int nb,etage;
	TypeFiltre filtre;

	debug_modeles = 0;
	if(imprime_actions) printf("- Lecture de la signature\n");
	if(!EnregSuivant(ligne,1024,f)) {
		sprintf(explic,"Premiere ligne illisible (%s). Lecture abandonnee",strerror(errno));
		return(0);
	}
	l = strlen(ligne); if(l) { if(ligne[l - 1] == '\n') ligne[l - 1] = '\0'; } 
	if(strcmp(ligne,SIGNATURE)) {
		sprintf(explic,"Fichier etranger (%s). Lecture abandonnee",ligne);
		return(0);
	}
	if(imprime_entetes) printf(". Type de fichier: '%s'\n",ligne);
	if(!EnregSuivant(ligne,1024,f)) {
		sprintf(explic,"Deuxieme ligne illisible (%s). Lecture abandonnee",strerror(errno));
		return(0);
	}
	l = strlen(ligne); if(l) { if(ligne[l - 1] == '\n') ligne[l - 1] = '\0'; }
	l = strlen(LIGNE_VERSION);
	if(!strncmp(ligne,LIGNE_VERSION,l)) {
		sscanf(ligne+l,"%g",&version_lue);
		if(imprime_entetes) printf(". Version d'origine de cette archive: %g\n",version_lue);
	} else {
		version_lue = 6.3;
		printf(". Version d'origine, irrecuperable. Version par defaut: %g\n",version_lue);
	}

	bolos_geres_avant = BoloGeres;
	voies_gerees_avant = VoiesGerees;
	BigEndianOrder = 1; /* par defaut */
	ArchHdrNbInc = ArchHdrNbDesc[EVTS];
	if(imprime_actions) printf("> Lecture de l'entete generale\n");
	ArgFromPath(f,ArchHdrGeneral,LIMITE_ENTETE);
	if(imprime_actions) printf("> Entete generale lue\n");
	BigEndianSource = BigEndianOrder;
	BigEndianOrder = EndianIsBig(); /* pour restaurer la bonne valeur */
	byteswappe = (BigEndianOrder != BigEndianSource);
//	BoloGeres = bolos_geres_avant;
//	VoiesGerees = voies_gerees_avant;
	BoloGeres = BoloNb;
	VoiesGerees = VoiesNb;
	if(imprime_entetes) ArgPrint("*",ArchHdrGeneral);

	strcpy(ModelePhys[0].nom,"phonon");
	ModelePhys[0].duree         = 100000.0; /* Evenement chaleur:  1 ms      */
	ModelePhys[0].tempsmort     = -1.0;     /* Temps mort apres max evt (ms) */
	ModelePhys[0].delai         = 0.0;	    /* Delai relatif de l'evt (ms)   */
	ModelePhys[0].interv        = 100.0;	/* jitter propre (ms)            */
#ifdef RL
    ModelePhys[0].dimtemplate	= 64;   /* nombre de point actif pour filtrage template*/
	ModelePhys[0].montee		= 2.0;  /* temps de montée de l'evenement (ms) */ 
	ModelePhys[0].descente1		= 15.0; /* temps de descente rapide (ms)       */
	ModelePhys[0].descente2		= 100;  /* temps de descente lente (ms)        */
	ModelePhys[0].facteur2		= 0.3;  /* fraction de descente lente          */
#endif
	strcpy(ModelePhys[1].nom,"charge");
	ModelePhys[1].duree         = 1000.0; /* Evenement chaleur:  10 ms     */
	ModelePhys[1].tempsmort     = -1.0;   /* Temps mort apres max evt (ms) */
	ModelePhys[1].delai         = -1.0;	  /* Delai relatif de l'evt (ms)   */
	ModelePhys[1].interv        = 1.0;	  /* jitter propre (ms)            */
#ifdef RL
    ModelePhys[1].dimtemplate	= 64;     /* nombre de point actif pour filtrage template*/
	ModelePhys[1].montee		= 0.0;    /* temps de montée de l'evenement (ms) */
	ModelePhys[1].descente1		= 10.0;   /* temps de descente rapide (ms)       */
	ModelePhys[1].descente2		= 0.0;    /* temps de descente lente (ms)        */
	ModelePhys[1].facteur2		= 0.0;    /* fraction de descente lente          */
#endif
	ModelePhysNb = 2;
	
	
//	if(!ModeleVoie) ModeleVoie = (TypeVoieModele *)malloc(VOIES_TYPES * sizeof(TypeVoieModele));
//	if(ModeleVoie == 0) {
//		sprintf(explic,"! Allocation de l'objet ModelesVoie[%d] impossible",VOIES_TYPES * sizeof(TypeVoieModele));
//		return(0);
//	}
	for(vm=0; vm<VOIES_TYPES; vm++) ModeleVoieListe[vm] = ModeleVoie[vm].nom;
	ModeleVoieListe[vm] = "\0";
//	if((version_lue < 8.16) || (version_lue >= 8.3) || ((version_lue >= 8.2) && (version_lue < 8.21))) {
//		c = VoiesConnues; while(*c) { if(*c == '+') *c = '~'; c++; }
//	}
	if(!SambaAlloc()) {
		sprintf(explic,"Memoire indisponible. Lecture abandonnee");
		return(0);
	}

	voie = 0; /* Si un filtre est declare avant toute voie, il est affecte a la voie 0 par defaut */
	BoloNb = 0;
	VoiesNb = 0;
	ModeleVoieNb = 0;
	do {
		if(!EnregSuivant(ligne,1024,f)) break;

		if(!strncmp(ligne,"* Det",5) || !strncmp(ligne,"* Bolo",6)) {
			c = ligne + 12;
			item = ItemSuivant(&c,':');
			if(!strcmp(item,"indetermine")) {
				if(imprime_actions) printf("- Le detecteur #%d suivant est %s et ne sera pas lu\n",BoloNb,item);
			} else {
				if(imprime_actions) {
                    if(imprime_entetes) printf("\n");
                    printf("- Lecture des caracteristiques du detecteur #%d, '%s'\n",BoloNb,item);
                }
                ReglageRazBolo(&BoloEcrit,item);
				ArgFromPath(f,ArchHdrBoloDef,LIMITE_ENTETE);
				if(!strncmp(BoloEcrit.hote,"EDWACQ-",7)) {
					strcpy(nom,BoloEcrit.hote+7);
					strcpy(BoloEcrit.hote,nom);
				}
				if(imprime_entetes) ArgPrint("*",ArchHdrBoloDef);
				if(BoloNb < BoloGeres) {
					memcpy(Bolo+BoloNb,&BoloEcrit,sizeof(TypeDetecteur)); /* !!! reglages pas recopies */
//						strncpy(Bolo[BoloNb].nom,item,DETEC_NOM); fait par ReglageRazBolo
					Bolo[BoloNb].local = 1;
					Bolo[BoloNb].a_lire = 1;
                    if(version_lue < 7.0) {
                    /* A cette epoque, un seul bolo etait de toutes facons sauve.
                       On eut pu, s'il y en eu plusieurs, partir de VoiesConnues qui donne ModeleVoie
                       et generer toutes les VoieManip */
						for(cap=0; cap<DETEC_CAPT_MAX; cap++) Bolo[BoloNb].captr[cap].bb.adrs = 0x33;
						for(voie=0; voie<VoiesNb; voie++) {
							strcat(strcat(VoieManip[voie].nom," "),Bolo[BoloNb].nom);
                            if((cap = Bolo[BoloNb].nbcapt) < DETEC_CAPT_MAX) {
                                Bolo[BoloNb].captr[cap].voie = voie;
                                Bolo[BoloNb].nbcapt += 1;
                            }
                        }
                    } else {
                        for(voie=0; voie<VoiesNb; voie++) {
                            strcpy(nom,VoieManip[voie].nom);
                            c = nom;
                            ItemSuivant(&c,' ');
                            nom_bolo = ItemSuivant(&c,' ');
                            if(!strcmp(Bolo[BoloNb].nom,nom_bolo)) {
								VoieManip[voie].det = BoloNb;
                                if((cap = Bolo[BoloNb].nbcapt) < DETEC_CAPT_MAX) {
                                    Bolo[BoloNb].captr[cap].voie = voie;
									printf("  . Bolo[%d].voie[%d] = #%d\n",BoloNb,cap,voie);
                                    Bolo[BoloNb].nbcapt += 1;
									if(VoieManip[voie].type < VOIES_TYPES) ModeleVoie[VoieManip[voie].type].utilisee = 1;
                                }
                            }
                        }
                    }
					if(imprime_actions) {
                        if(imprime_entetes) printf("=> ");
						if(Bolo[BoloNb].lu == DETEC_OK) {
							printf(". Bolo #%02d %3gg: %16s, place en %03x, lu par %s via BB#%d @%02X\n",
								BoloNb,Bolo[BoloNb].masse,Bolo[BoloNb].nom,Bolo[BoloNb].pos,
								Bolo[BoloNb].hote,Bolo[BoloNb].captr[0].bb.serial,Bolo[BoloNb].captr[0].bb.adrs);
						} else {
							printf(". Bolo #%02d non lu: '%s'\n",BoloNb,Bolo[BoloNb].nom);
						}
					}
                    BoloNb++;
 //					BoloGeres = BoloNb;
				} else {
					if(imprime_actions) printf(". Ce detecteur est excedentaire (maxi: %d) et ne sera pas memorise\n",BoloGeres);
				}
			}

		} else if(!strncmp(ligne,"* Physique",10)) {
			ArgFromPath(f,ArchHdrPhysDef,LIMITE_ENTETE);
			if(imprime_entetes) ArgPrint("*",ArchHdrPhysDef);

		} else if(!strncmp(ligne,"* Voie",6)) {
			c = ligne + 7;
			item = ItemSuivant(&c,':');
			voie = VoiesNb;
			memcpy(&VoieEcrite,VoieManip+voie,sizeof(TypeVoieAlire));
			// arriver a executer VoieTampon[voie].lue = 1;
			if(imprime_actions) {
                if(imprime_entetes) printf("\n");
                printf("- Lecture de la definition de la voie '%s'\n",item);
            }
			VoieEcrite.gain_variable = 1.0;  /* oubli dans V7.2 */
			//- VoieEcrite.copie = -1; /* ajoute depuis V7.19 */
			VoieEcrite.pseudo = 0;
			VoieEcrite.def.evt.base_dep = 25.0; /* oublie jusqu'au moins V9.1.8 */
			VoieEcrite.def.evt.base_arr = 75.0; /* oublie jusqu'au moins V9.1.8 */
			VoieEcrite.def.evt.ampl_10 = 10.0; /* ajoute a la version 9.17 */
			VoieEcrite.def.evt.ampl_90 = 90.0; /* ajoute a la version 9.17 */
			VoieEcrite.RC = TangoRCdefaut;
			ArgFromPath(f,ArchHdrVoieDef,LIMITE_ENTETE);
			if(VoiesNb < VoiesGerees) {
				if(version_lue < 7.0) {
					/* on ne fait pas dans la dentelle, mais de toutes facons, cela ne concerne qu'UN SEUL fichier! [ !!: + NOSTOS] */
					VoieEcrite.det = 0;
					VoieEcrite.type = voie;
					VoieEcrite.def.evt.pretrigger = 25.0;
					VoieEcrite.def.evt.tempsmort = 25.0;
					VoieEcrite.ADCmask = 0x3FFF;
					VoieEcrite.ADUenmV = 250.0;
					VoieEcrite.gain_fixe = 1.0;
					VoieEcrite.Rmodul = 2200.0 / 8.2;
				}
				if(imprime_entetes) ArgPrint("*",ArchHdrVoieDef);
				memcpy(VoieManip+voie,&VoieEcrite,sizeof(TypeVoieAlire));
				/* malheureusement, les trois donnees suivantes sont obsoletes a ce niveau... donc VoieEvent indefini!!! */
			#ifdef OBSOLETE
				VoieEvent[voie].horloge = ArchVoie.horloge;
				VoieEvent[voie].lngr = ArchVoie.dim;
				VoieEvent[voie].avant_evt = ArchVoie.avant_evt;
			#endif
				VoieEvent[voie].filtre = 0;
				strncpy(VoieManip[voie].nom,item,VOIE_NOM);
	//			if(VoieEvent[voie].avant_evt <= 0) VoieEvent[voie].avant_evt = (VoieEvent[voie].lngr * 3) / 4;
				strcpy(nom,VoieManip[voie].nom);
				c = nom;
				ItemJusquA(' ',&c);
                if(debug_modeles) printf("  . Recherche du detecteur \"%s\" pour la voie \"%s\"\n",c,nom);
				if(BoloNb == 1) bolo = 0;
				else {
					if(*c == '\0') c = nom; /* plusieurs detecteurs, mais 1 seul type de voie */
					for(bolo=0; bolo<BoloNb; bolo++) if(!strcmp(Bolo[bolo].nom,c)) break;
				}
				if(bolo < BoloNb) VoieManip[voie].det = bolo; else VoieManip[voie].det = -1;
				if(debug_modeles) printf("  . Detecteur #%d/%d: \"%s\"\n",bolo+1,BoloNb,(bolo < BoloNb)? Bolo[bolo].nom: "(inconnu)");
                if(debug_modeles) printf("  . Recherche du modele pour la meme voie\n");
				for(vm=0; vm<ModeleVoieNb; vm++) if(!strcmp(nom,ModeleVoie[vm].nom)) break;
                if(debug_modeles) printf("  . Modele #%d/%d: \"%s\"\n",vm+1,ModeleVoieNb,(vm < ModeleVoieNb)? ModeleVoie[vm].nom: "(inconnu)");
				if((vm >= ModeleVoieNb) && (ModeleVoieNb < VOIES_TYPES)) {
					int p,ph;
					if(!strncmp(nom,"Chal",4) || !strncmp(nom,"chal",4) || !strncmp(nom,"lum",3)) p = ModeleVoie[vm].physique = 0;
					else p = ModeleVoie[vm].physique = 1;
					strncpy(ModeleVoie[vm].nom,nom,MODL_NOM);
					ModeleVoie[vm].duree = ModelePhys[p].duree;
					ModeleVoie[vm].delai = ModelePhys[p].delai;
					ModeleVoie[vm].tempsmort = ModelePhys[p].tempsmort;
					ModeleVoie[vm].interv = ModelePhys[p].interv;
					ModeleVoie[vm].coinc = 20.0;
					ModeleVoie[vm].pretrigger = 25.0;
					ModeleVoie[vm].base_dep = 25.0;
					ModeleVoie[vm].base_arr = 75.0;
					for(ph=0; ph<DETEC_PHASES_MAX; ph++) {
						ModeleVoie[vm].phase[ph].t0 = 0.0;
						ModeleVoie[vm].phase[ph].dt = 0.0;
					}
					ModeleVoie[vm].utilisee = 0;
#ifdef RL
					ModeleVoie[vm].dimtemplate = ModelePhys[p].dimtemplate;
					ModeleVoie[vm].montee = ModelePhys[p].montee;
					ModeleVoie[vm].descente1 = ModelePhys[p].descente1;
					ModeleVoie[vm].descente2 = ModelePhys[p].descente2;
					ModeleVoie[vm].facteur2 = ModelePhys[p].facteur2;
#endif
					ModeleVoie[vm].min_moyen = 1000.0;
					ModeleVoie[vm].max_moyen = 8000.0;
					vm = ModeleVoieNb++;
					ModeleVoie[vm+1].nom[0] = '\0';
				} else vm = 0;
				ModeleVoie[vm].utilisee = 1;
//				if(imprime_entetes) printf(": Utilisation d'une voie de type %s\n",ModeleVoie[vm].nom);
				strncpy(VoieManip[voie].prenom,nom,MODL_NOM);
				VoieManip[voie].type = vm;
				VoieManip[voie].extens = 0xFFFF - (unsigned short)VoieManip[voie].ADCmask;
				VoieManip[voie].zero = (VoieManip[voie].ADCmask + 1) / 2;
				if(version_lue >= 7.0) {
					if(VoieManip[voie].gain_fixe == 0.0) VoieManip[voie].gain_fixe = 1.0; /* V7.3 et avant */
					if(VoieManip[voie].nVenkeV <= 0.0) VoieManip[voie].nVenkeV = 1.0;
					nom_bolo = ItemSuivant(&c,' ');
					for(bolo=0; bolo<BoloNb; bolo++) {
						if(!strcmp(Bolo[bolo].nom,nom_bolo)) {
							VoieManip[voie].det = bolo;
							if((cap = Bolo[bolo].nbcapt) < DETEC_CAPT_MAX) {
								Bolo[bolo].captr[cap].voie = voie;
								// printf("  . Bolo[%d].voie[%d] = #%d\n",bolo,cap,voie);
								Bolo[bolo].nbcapt += 1;
							}
							break;
						}
					}
				}
				VoiesNb++;
				VoiesLues = VoiesNb;
			} else {
				if(imprime_actions) printf(". Cette voie est excedentaire (maxi: %d) et ne sera pas memorisee\n",VoiesGerees);
			}

		} else if(!strncmp(ligne,"* Filtre",8)) { /* <voie> est la derniere voie lue */
			read(f,&nb,sizeof(int));
		#ifndef CODE_WARRIOR_VSN
			if(byteswappe) ByteSwappeInt((unsigned int *)&nb);
		#endif
			filtre.nbetg = nb;
			for(etage=0; etage<filtre.nbetg; etage++) {
				read(f,&nb,sizeof(int));
			#ifndef CODE_WARRIOR_VSN
				if(byteswappe) ByteSwappeInt((unsigned int *)&nb);
			#endif
//				if(voie < 2) printf("Filtre %s: %d etages\n",VoieManip[voie].nom,nb);
				filtre.etage[etage].nbdir = nb;
				for(i=0; i<nb; i++) 
					read(f,&(filtre.etage[etage].direct[i]),sizeof(double));
				if(byteswappe) ByteSwappeDoubleArray(filtre.etage[etage].direct,nb);
				read(f,&nb,sizeof(int));
			#ifndef CODE_WARRIOR_VSN
				if(byteswappe) ByteSwappeInt((unsigned int *)&nb);
			#endif
				filtre.etage[etage].nbinv = nb;
				for(i=0; i<nb; i++)
					read(f,&(filtre.etage[etage].inverse[i]),sizeof(double));
				if(byteswappe) ByteSwappeDoubleArray(filtre.etage[etage].inverse,nb);
			}
			if(!VoieEvent[voie].filtre) VoieEvent[voie].filtre = (TypeFiltre *)malloc(sizeof(TypeFiltre));
			if(VoieEvent[voie].filtre) memcpy(VoieEvent[voie].filtre,&filtre,sizeof(TypeFiltre));
			

		} else if(!strncmp(ligne,"* Run",5)) {
			if(imprime_actions) {
                if(imprime_entetes) printf("\n");
                printf("- Lecture de l'entete de run\n");
            }
			strncpy(nom,SrceName,80);
			ArgFromPath(f,ArchHdrRun,LIMITE_ENTETE);
			strncpy(SrceName,nom,80);
			/* if(!strncmp(Acquis[AcquisLocale].code,"EDWACQ-",7)) {
				strcpy(nom,Acquis[AcquisLocale].code+7);
				strncpy(Acquis[AcquisLocale].code,nom,CODE_MAX);
			}
			if(!strncmp(Starter,"EDWACQ-",7)) {
				strcpy(nom,Starter+7);
				strcpy(Starter,nom);
			} */
			// Acquis[AcquisLocale].runs[0] = 's';
			if(imprime_entetes) ArgPrint("*",ArchHdrRun);

		} else if(!strncmp(ligne,"* Donnees",9)) break;

		else {
			l = strlen(ligne); if(ligne[l - 1] == '\n') ligne[l - 1] = '\0';
			printf(". Ligne abandonnee (%s)\n",ligne);
		}

	} forever;

	/* si on a declare un bloc global, il doit etre mis en fin de liste, meme si DetecteurStandardLocal.taille == 0 */
	ReglageRazBolo(Bolo+BoloNb,"Standard");
#ifdef Samba
	DetecteurAjouteStd();
#else
	{
		BoloStandard = BoloNb;
		memcpy(Bolo+BoloStandard,&DetecteurStandardLocal,sizeof(TypeDetecteur));
		Bolo[BoloStandard + 1].nom[0] = '\0';
		printf("  . Detecteur standard: #%d\n",BoloStandard);
	}
#endif

	if(imprime_entetes) SambaDumpBolos(); else if(imprime_actions) printf("\n");
	return(1);
}
