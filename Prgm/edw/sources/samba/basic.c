#define BRANCHE_BASIC
#ifdef macintosh
#pragma message("Compilation de "__FILE__)
#endif
#include <environnement.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
//#ifndef macintosh
#include <fcntl.h>
//#endif
#ifdef CODE_WARRIOR_VSN
#include <string.h>
#else
#include <strings.h>
#endif

#ifdef CODE_WARRIOR_VSN
	#include <claps.h>
	#include <dateheure.h>
	#include <opium_demande.h>
	#include <opium.h>
	#ifdef AVEC_PCI
		#include <IcaLib.h>
	#endif
#else
	#include <utils/claps.h>
	#include <utils/dateheure.h>
	#include <opium_demande.h>
	#include <opium4/opium.h>
	#ifdef AVEC_PCI
		#include <PCI/IcaLib.h>
	#endif
#endif
#include <samba.h>
#include <repartiteurs.h>
#include <numeriseurs.h>
#include <diags.h>

#define PCI_DIRECT
#define BASIC_IPBLOC 4096

#define MAXPARMS 1024
#define MAXBLOCS 32
#define MAXINDEX 5        /* nombre de caracteres pour l'index dans un bloc */
#define MAXTEXTE DETEC_NOM+2+MAXINDEX
#define MAXMODIFS 24
/* #define CHANGE_DEFAUT(nom) strcmp(nom,"=") */
#define CHANGE_DEFAUT lire

typedef struct {
	char nom[DETEC_NOM];
	int32 debut;
	int32 taille;
} TypeBasicBloc;

static TypeBasicBloc BasicBloc[MAXBLOCS];

static Ulong BasicPCIout,BasicPCIin;  /* buffer bidon pour tests sans carte PCI */
static Ulong *BasicCmdes,*BasicDonnees,*BasicReset;
static int  BasicBlocsNb,BasicParmsNb;
static char *BasicBlocNom[MAXBLOCS + 1];
static int32 BasicParms[MAXPARMS];
static char BasicUserBlocs[MAXFILENAME],BasicFichierBlocs[MAXFILENAME];
static char *BasicParmText[MAXMODIFS];
static int  BasicParmDebut,BasicParmFin;
static char BasicBlocDemande[DETEC_NOM];
static int  BasicBlocNum;
static int  BasicAttenteModul;
static char BasicReadOnly,BasicWait;
static char BasicDumpHexa,BasicASauver;

static int BasicRep;
static int BasicOctet;
static char BasicIpCde;
static char BasicIpTexte[80];
static byte BasicSmbCmde[2],BasicSmbC1,BasicSmbC2;
static shex BasicSmbVal;

#ifdef CLAPS_H
static ArgDesc BasicDesc[] = {
	{ "DumpHexa",      DESC_BOOL &BasicDumpHexa,     "Dump des trames IP en hexadecimal" },
	{ "Fichier-Blocs", DESC_TEXT  BasicFichierBlocs, "Fichier des blocs a ecrire" },
	{ "AdressePCI",    DESC_NONE },
	{ "LectureSeule",  DESC_BOOL &BasicReadOnly,     "Pas d'impression pendant la lecture" },
	{ "FIFOEmpty",     DESC_KEY "count/wait", &BasicWait, "si FIFO vide: compter ou attendre" },
	{ "AttenteModul",  DESC_INT  &BasicAttenteModul, "Attente pour synchro sur une synchronisation" },
	DESC_END
};
#endif

static Panel pBasicFichierLect,pBasicFichierEcri;
static Panel pBasicBlocCree,pBasicBlocDemande,pBasicParmDebut,pBasicParmRange,pBasicModifs;

static Menu mBasicDestin,mBasicStyleMiG;
static int   BasicAdrsICA,BasicValICA,BasicAdrsCC,BasicSsAdrsCC,BasicValCC;
static Panel pBasicDirectICA,pBasicDirectCC,pBasicOctet;
static Panel pBasicSmbEnvoi;
#ifdef AVEC_PCI
static int   BasicIcaVal[ICA_REG_NB];
static char  BasicIcaNom[ICA_REG_NB][80];
static Panel pBasicRegsICA;
#endif
static int   BasicStatusCC;
static Term  tBasicStatusCC,tBasicDirectLog;

static int   BasicNbLect;
static Panel pBasicNbLect;

static Info BasicPancarte;
static Term tBasicBlocs;

/* ========================================================================== */
int BasicRecupere() {
	int i; size_t l;
	char *r,ligne[80];
	FILE *f;
	int parms_avant;

	if(OpiumExec(pBasicFichierLect->cdr) == PNL_CANCEL) return(0);
	f = fopen(BasicFichierBlocs,"r");
	if(!f) {
		OpiumError("Fichier '%s' inaccessible: %s",BasicFichierBlocs,strerror(errno));
		return(0);
	}
	parms_avant = BasicParmsNb;
	do {
		r = fgets(ligne,80,f);
		if(!r) break;
		else if((*r == '#') || (*r == '\n')) continue;
		if(*r == '=') {
			l = strlen(ligne); if(ligne[l-1] == '\n') ligne[l-1] = '\0';
			if(BasicBlocsNb) {
				if(BasicBlocsNb >= MAXBLOCS) {
					OpiumError("Pas plus de %d blocs. Derniers blocs non relus",MAXBLOCS);
					break;
				}
				for(i=0; i<BasicBlocsNb; i++) if(!strcmp(r+1,BasicBloc[i].nom)) break;
				if(i < BasicBlocsNb) {
					OpiumError("Le bloc '%s' est deja connu! Derniers blocs non relus",r);
					break;
				}
				BasicBloc[BasicBlocsNb-1].taille = BasicParmsNb - BasicBloc[BasicBlocsNb-1].debut;
			}
			strncpy(BasicBloc[BasicBlocsNb].nom,r+1,DETEC_NOM);
			BasicBloc[BasicBlocsNb].debut = BasicParmsNb;
			BasicBlocsNb++;
		} else {
			if(BasicParmsNb >= MAXPARMS) {
				OpiumError("Pas plus de %d parametres. Derniers blocs non relus",MAXPARMS);
				break;
			}
			sscanf(ligne,"%lx",&(BasicParms[BasicParmsNb++]));
		}
	} forever;

	fclose(f);
	if(BasicBlocsNb) {
		BasicBloc[BasicBlocsNb-1].taille = BasicParmsNb - BasicBloc[BasicBlocsNb-1].debut;
		BasicBlocNum = BasicBlocsNb - 1;
	}
	BasicASauver = (parms_avant > 0);
	return(0);
}
/* ========================================================================== */
int BasicSauve() {
	int i,j,k;
	FILE *f;

	if(OpiumExec(pBasicFichierEcri->cdr) == PNL_CANCEL) return(0);
	RepertoireCreeRacine(BasicFichierBlocs);
	f = fopen(BasicFichierBlocs,"w");
	if(!f) {
		OpiumError("Fichier '%s' inutilisable: %s",BasicFichierBlocs,strerror(errno));
		return(0);
	}

	for(i=0; i<BasicBlocsNb; i++) {
		fprintf(f,"#\n=%s\n",BasicBloc[i].nom);
		for(j=0, k=BasicBloc[i].debut; j<BasicBloc[i].taille; j++,k++) {
			fprintf(f,"%03lx\n",BasicParms[k]);
		}
	}
	fclose(f);
	BasicASauver = 0;
	return(0);
}
/* ========================================================================== */
int BasicDump() {
	int i,j,k;

	if(!(tBasicBlocs->cdr->displayed)) OpiumDisplay(tBasicBlocs->cdr);
	TermHold(tBasicBlocs);
	for(i=0; i<BasicBlocsNb; i++) {
		TermPrint(tBasicBlocs,"=== %s[%d] @ %d ===\n",BasicBloc[i].nom,BasicBloc[i].taille,BasicBloc[i].debut);
		for(j=0, k=BasicBloc[i].debut; j<BasicBloc[i].taille; k++) {
			TermPrint(tBasicBlocs," %03x",BasicParms[k]);
			j++;
			if(!(j%10)) TermPrint(tBasicBlocs,"\n");
		}
		if(j%10) TermPrint(tBasicBlocs,"\n");
	}
	TermPrint(tBasicBlocs,"=== parametres ===\n");
	for(k=0; k<BasicParmsNb; ) {
		TermPrint(tBasicBlocs," %02d:%03x",k,BasicParms[k]);
		k++;
		if(!(k%10)) TermPrint(tBasicBlocs,"\n");
	}
	if(k%10) TermPrint(tBasicBlocs,"\n");
	TermRelease(tBasicBlocs);
	return(0);
}
/* ========================================================================== */
int BasicRepertoire() {
	int i;
	char texte[26];

	if(!(tBasicBlocs->cdr->displayed)) OpiumDisplay(tBasicBlocs->cdr);
	TermHold(tBasicBlocs);
	TermPrint(tBasicBlocs,"===== Repertoire =====\n");
	for(i=0; i<BasicBlocsNb; ) {
		sprintf(texte,"%s [%ld]",BasicBloc[i].nom,BasicBloc[i].taille);
		TermPrint(tBasicBlocs,"%-26s",texte);
		i++; if(!(i%3)) TermPrint(tBasicBlocs,"\n");
	}
	if(i%3) TermPrint(tBasicBlocs,"\n");
	TermRelease(tBasicBlocs);
	return(0);
}
/* ========================================================================== */
static int trouve_bloc() {
	if(BasicBlocNum < 0) {
		OpiumError("Aucun bloc n'est encore defini!");
		return(0);
	}
	if(OpiumExec(pBasicBlocDemande->cdr) == PNL_CANCEL) return(0);
	return(1);
}
/* ========================================================================== */
int BasicCree() {
	int i;

	if(BasicBlocsNb >= MAXBLOCS) {
		OpiumError("Deja %d blocs definis!",MAXBLOCS);
		return(0);;
	}
	if(OpiumExec(pBasicBlocCree->cdr) == PNL_CANCEL) return(0);
	for(i=0; i<BasicBlocsNb; i++) if(!strcmp(BasicBlocDemande,BasicBloc[i].nom)) break;
	if(i < BasicBlocsNb) {
		OpiumError("Le bloc '%s' existe deja!",BasicBlocDemande);
		return(0);
	}
	BasicBlocNum = BasicBlocsNb++;
	strncpy(BasicBloc[BasicBlocNum].nom,BasicBlocDemande,DETEC_NOM);
	BasicBloc[BasicBlocNum].debut = BasicParmsNb;
	BasicBloc[BasicBlocNum].taille = 0;
	BasicASauver = 1;
	return(0);
}
/* ========================================================================== */
int BasicListe() {
	int taille,i,k;

	if(!(tBasicBlocs->cdr->displayed)) OpiumDisplay(tBasicBlocs->cdr);
	TermHold(tBasicBlocs);
	TermPrint(tBasicBlocs,"===== Bloc %s =====\n",BasicBloc[BasicBlocNum].nom);
	taille = BasicBloc[BasicBlocNum].taille;
	i = BasicBloc[BasicBlocNum].debut;
	for(k=0; k<taille; ) {
		TermPrint(tBasicBlocs," %2d: %03x",++k,BasicParms[i++]);
		if(!(k%8)) TermPrint(tBasicBlocs,"\n");
	}
	if(k%8) TermPrint(tBasicBlocs,"\n");
	TermRelease(tBasicBlocs);
	return(0);
}
/* ========================================================================== */
static int trouve_index(int *debut, int *fin) {
	int taille;
	int k,l;

	taille = BasicBloc[BasicBlocNum].taille;
	if(!taille) {
		OpiumError("Aucun parametre n'est encore associe!");
		return(0);
	}

	k = BasicParmDebut;
	l = BasicParmFin;
	if(OpiumExec(pBasicParmRange->cdr) == PNL_CANCEL) return(0);
	if(BasicParmDebut <= 0)
		OpiumError("Reponse incorrecte, consideree comme un refus...");
	else if (BasicParmDebut > taille)
		OpiumError("Mais, le bloc '%s' ne contient que %d parametre%s!",
			BasicBloc[BasicBlocNum].nom,Accord1s(taille));
	else if(BasicParmFin < BasicParmDebut)
		OpiumError("Reponse incorrecte, consideree comme un refus...");
	else {
		if(BasicParmFin > taille) {
			OpiumError("Valeur de fin (%d) ramenee a %d",BasicParmFin,taille);
			BasicParmFin = taille;
		}
		*debut = BasicBloc[BasicBlocNum].debut + BasicParmDebut - 1;
		*fin = BasicBloc[BasicBlocNum].debut + BasicParmFin;
		return(1);
	}
	BasicParmDebut = k;
	BasicParmFin = l;
	return(0);
}
/* ========================================================================== */
int BasicAjoute() {
	int debut,taille;
	int i,j,k;
	int val;
	char texte[80];

	debut = BasicBloc[BasicBlocNum].debut;
	taille = BasicBloc[BasicBlocNum].taille;
	if((debut + taille) < BasicParmsNb) {
		/* on place les parametres en fin de tableau pour "optimiser" (?) */
		for(i=debut,j=BasicParmsNb,k=0; k<taille; i++,j++,k++) BasicParms[j] = BasicParms[i];
		/* on recupere la memoire utilisee primitivement */
		for(i=debut; i<BasicParmsNb; i++) BasicParms[i] = BasicParms[i+taille];
		/* on met a jour le bloc */
		BasicBloc[BasicBlocNum].debut = BasicParmsNb;
		/* on met a jour tous les autres pointeurs */
		for(i=0; i<BasicBlocsNb; i++) if(BasicBloc[i].debut > debut) BasicBloc[i].debut -= taille;
	}
	if(taille) {
		if(OpiumExec(pBasicParmDebut->cdr) == PNL_CANCEL) return(0);
		k = BasicParmDebut - 1;
		if(k < 0) k = 0; else if(k > taille) k = taille;
	} else k = 0;
	i = BasicBloc[BasicBlocNum].debut + k;
	val = (i > 0)? BasicParms[i - 1]: 0;
	do {
		sprintf(texte,"%s[%d] [(cancel) pour terminer]",BasicBloc[BasicBlocNum].nom,k+1);
		if(OpiumReadHex(texte,&val) == PNL_CANCEL) break;
		/* on fait de la place */
		for(j=BasicParmsNb; j > i; ) { BasicParms[j] = BasicParms[j - 1]; --j; }
		BasicParmsNb++;
		/* on met a jour le bloc */
		BasicBloc[BasicBlocNum].taille += 1;
		BasicParms[i] = val;
		i++; k++;
		BasicASauver = 1;
	} forever;
	return(0);
}
/* ========================================================================== */
int BasicModifie() {
	int debut,fin;
	int i,imin,imax,j,nb;

	if(!trouve_index(&debut,&fin)) return(0);

	nb = ((fin - debut - 1) / MAXMODIFS) + 1;
	imin = debut;
	for(j=0; j<nb; j++) {
		imax = imin + MAXMODIFS;
		if(imax > fin) imax = fin;
		PanelErase(pBasicModifs);
		for(i=imin; i<imax; i++) {
			sprintf(BasicParmText[i],"%s[%ld]",
				BasicBloc[BasicBlocNum].nom,i-BasicBloc[BasicBlocNum].debut+1);
			PanelHex(pBasicModifs,BasicParmText[i],(int *)&(BasicParms[i]));
		}
		if(OpiumExec(pBasicModifs->cdr) != PNL_CANCEL) BasicASauver = 1;
	}

	return(0);
}
/* ========================================================================== */
int BasicRetire() {
	int i,k,m;
	int debut,fin;

	if(!trouve_index(&debut,&fin)) return(0);

	k = debut - BasicBloc[BasicBlocNum].debut;
	for(i=debut; i<fin; ) {
		if(!OpiumChoix(2,SambaNonOui,"Es-tu sur de vouloir retirer %s[%d] = %03X",
			BasicBloc[BasicBlocNum].nom,++k,BasicParms[i])) {
			OpiumError("Le parametre n'a pas ete retire!");
			break;
		}
		BasicParmsNb--;
		for(m=i ; m<BasicParmsNb; m++) BasicParms[m] = BasicParms[m+1];
		BasicBloc[BasicBlocNum].taille -= 1;
		/* on met a jour tous les pointeurs */
		for(m=0; m<BasicBlocsNb; m++) if(BasicBloc[m].debut > i) BasicBloc[m].debut--;
		fin--;
		BasicASauver = 1;
	}
	return(0);
}
Menu mBasicEdite;
/* ========================================================================== */
int BasicEdite() {
	if(!trouve_bloc()) return(0);
	OpiumTitle(mBasicEdite->cdr,BasicBloc[BasicBlocNum].nom);
	OpiumExec(mBasicEdite->cdr);
	return(0);
}
/* ========================================================================== */
int BasicEfface() {
	int bloc,i;
	int debut,taille;

	bloc = BasicBlocNum;
	if(!trouve_bloc()) return(0);
	if(!OpiumChoix(2,SambaNonOui,"Es-tu sur de vouloir effacer le bloc %s[%d]",
		BasicBloc[BasicBlocNum].nom,BasicBloc[BasicBlocNum].taille)) {
		OpiumError("Le bloc '%s' n'a pas ete retire!",BasicBloc[BasicBlocNum].nom);
		BasicBlocNum = bloc;
		return(0);
	}
	/* on recupere la memoire utilisee pour les parametres */
	debut = BasicBloc[BasicBlocNum].debut;
	taille = BasicBloc[BasicBlocNum].taille;
	BasicParmsNb -= taille;
	for(i=debut; i<BasicParmsNb; i++) BasicParms[i] = BasicParms[i+taille];
	/* on supprime le bloc */
	BasicBlocsNb--;
	for(i=BasicBlocNum; i<BasicBlocsNb; i++) memcpy(BasicBloc+i,BasicBloc+i+1,sizeof(TypeBasicBloc));
	/* on met a jour tous les pointeurs */
	for(i=0; i<BasicBlocsNb; i++) if(BasicBloc[i].debut > debut) BasicBloc[i].debut -= taille;
	if(bloc < BasicBlocNum) BasicBlocNum = bloc; else BasicBlocNum = bloc - 1;
	if(BasicBlocNum < 0) BasicBlocNum = BasicBlocsNb - 1;
	BasicASauver = 1;
	return(0);
}
/* ========================================================================== */
int BasicEcritures() {
	int32 taille,*parm;

	if(!trouve_bloc()) return(0);
	InfoWrite(BasicPancarte,1,"Ecriture du bloc '%s' en cours",BasicBloc[BasicBlocNum].nom);
	OpiumDisplay(BasicPancarte->cdr);

{
	int i;
	taille = BasicBloc[BasicBlocNum].taille;
	parm = BasicParms + BasicBloc[BasicBlocNum].debut;
	i = 0;
	while(taille--) {
		if(!(i%4)) {
			if(i) printf("%n");
			printf("===== Ecriture de ");
		}
		printf("%02X",*parm++); i++;
	}
	printf("\n");
}

	taille = BasicBloc[BasicBlocNum].taille;
	parm = BasicParms + BasicBloc[BasicBlocNum].debut;
	while(taille--) *BasicDonnees = *parm++;

	OpiumClear(BasicPancarte->cdr);
	OpiumError("Ecriture du bloc '%s' terminee",BasicBloc[BasicBlocNum].nom);
	return(0);
}
/* ========================================================================== */
int BasicResetFIFO() {
	*BasicReset = 0;
	printf("===== Ecriture de 0 a l'adresse %08X\n",BasicReset);
	TermPrint(tBasicBlocs,"===== FIFO resetee =====\n");
	return(0);
}
/* ========================================================================== */
int BasicDirect() {
	int qui,reg,bit;

	if(PCIedw[PCInum].type == CARTE_ICA) while((qui = OpiumExec(mBasicDestin->cdr)) != 9) {
		OpiumDisplay(mBasicDestin->cdr);
		if(!(OpiumDisplayed(tBasicDirectLog->cdr))) OpiumDisplay(tBasicDirectLog->cdr);
		TermHold(tBasicDirectLog);
		if(qui == 1) {
			if(OpiumExec(pBasicDirectICA->cdr) == PNL_CANCEL) break;
			BasicAdrsICA &= 0xFF; 
			reg = BasicAdrsICA / 4;
		#ifdef AVEC_PCI
			if((reg < 0) || (reg > ICA_REG_NB)) OpiumError("Registre ICA invalide");
			else {
				BasicAdrsICA = reg * 4;
				TermPrint(tBasicDirectLog,"ICAwrite @%02X,%08X\n",BasicAdrsICA,BasicValICA);
				TermRelease(tBasicDirectLog);
				IcaRegWrite(PCIedw[PCInum].port,reg,BasicValICA);
				for(reg=0; reg<ICA_REG_NB; reg++) BasicIcaVal[reg] = IcaRegRead(PCIedw[PCInum].port,reg);
				if(!OpiumDisplayed(pBasicRegsICA->cdr)) OpiumDisplay(pBasicRegsICA->cdr);
				else PanelRefreshVars(pBasicRegsICA);
			}
		#endif
		} else if(qui == 2) {
			if(OpiumExec(pBasicDirectCC->cdr) == PNL_CANCEL) break;
			BasicAdrsCC &= 0xFF;
			BasicSsAdrsCC &= 0xFF;
			BasicValCC &= 0xFFFF;
			printf("===== Ecriture de %02X.%02X.%04X\n",BasicAdrsCC,BasicSsAdrsCC,BasicValCC);
			TermPrint(tBasicDirectLog,"CCwrite  @%02X,%02X.%04X\n",BasicAdrsCC,BasicSsAdrsCC,BasicValCC);
			TermRelease(tBasicDirectLog);
			CluzelEnvoieBasique(PCIedw[PCInum].port,0,BasicAdrsCC,BasicSsAdrsCC,BasicValCC);
		#ifdef AVEC_PCI
			BasicStatusCC = IcaRegRead(PCIedw[PCInum].port,ICA_REG_STATUS);
		#endif
			TermEmpty(tBasicStatusCC);
			OpiumDisplay(tBasicStatusCC->cdr);
			TermHold(tBasicStatusCC);
			TermPrint(tBasicStatusCC,"Status CC: %08X\n",BasicStatusCC);
			TermPrint(tBasicStatusCC,"ICA version %d.%d\n",(BasicStatusCC >> 29) & 0x7,(BasicStatusCC >> 27) & 0x3);
			TermPrint(tBasicStatusCC,"CC  version %d.%d\n",(BasicStatusCC >> 24) & 0x7,(BasicStatusCC >> 22) & 0x3);
			for(bit=16; bit; ) {
				--bit;
				if(BasicStatusCC & BIT(bit)) 
					TermPrint(tBasicStatusCC,"  %7s %s\n",(bit < 8)? "erreur:": "",DiagTexteStatusCC[bit]);
			}
			TermRelease(tBasicStatusCC);
		}
		OpiumClear(mBasicDestin->cdr);
	} else /* (PCIedw[PCInum].type == CARTE_MIG) */ while((qui = OpiumExec(mBasicStyleMiG->cdr)) != 9) {
		if(qui == 1) {
			if(OpiumExec(pBasicDirectCC->cdr) == PNL_CANCEL) break;
			BasicAdrsCC &= 0xFF;
			BasicSsAdrsCC &= 0xFF;
			BasicValCC &= 0xFFFF;
			printf("===== Ecriture de %02X.%02X.%04X\n",BasicAdrsCC,BasicSsAdrsCC,BasicValCC);
			TermPrint(tBasicDirectLog,"CCwrite  @%02X,%02X.%04X\n",BasicAdrsCC,BasicSsAdrsCC,BasicValCC);
			TermRelease(tBasicDirectLog);
			CluzelEnvoieBasique(PCIedw[PCInum].port,0,BasicAdrsCC,BasicSsAdrsCC,BasicValCC);
		} else if(qui == 2) {
			// if(OpiumReadByte("Octet a ecrire",&BasicOctet) == PNL_CANCEL) break; format %d alors qu'on aimerait %x
			if(OpiumExec(pBasicOctet->cdr) == PNL_CANCEL) break;
		#ifdef AVEC_PCI
			IcaUtilWrite(PCIedw[PCInum].port,BasicOctet);
		#endif
			printf("===== Ecriture de %02X sur le bus UTIL\n",BasicOctet);
		}
	}
#ifdef AVEC_PCI
	if(OpiumDisplayed(pBasicRegsICA->cdr)) OpiumClear(pBasicRegsICA->cdr);
#endif
	if(OpiumDisplayed(tBasicStatusCC->cdr)) OpiumClear(tBasicStatusCC->cdr);
	if(OpiumDisplayed(tBasicDirectLog->cdr)) OpiumClear(tBasicDirectLog->cdr);
	return(0);
}
/* ========================================================================== */
int BasicLectures() {
	int nb,col;
	int64 t0,t1; int sec,usec;
	hexa nbzero;
	int attente;
	int num_bloc,taille_bloc;
	int32 val; int nbatt;
	int32 *tab,*ptr;

	if(OpiumExec(pBasicNbLect->cdr) == PNL_CANCEL) {
		OpiumError("Lecture abandonnee");
		return(0);
	}
	/* oct.2005: passe de /TermPrint(tBasicBlocs,/ a /printf(/ suite
	a BoloGeres = 32, ce qui fait sauter la couche Term */
//++	if(!(tBasicBlocs->cdr->displayed)) OpiumDisplay(tBasicBlocs->cdr);
//++	TermHold(tBasicBlocs);
//	printf("Commandes @%08X, Donnees @%08X, Reset @%08X\n",BasicCmdes,BasicDonnees,BasicReset);
	val = 0xCACA;
	taille_bloc = Repart[BasicRep].nbdonnees;
	printf("%d detecteur%s x %d voie%s/detecteur, soit %d voie%s/bloc,\n.. accedee%s directement a l'adresse %08llX.\n",
		Accord1s(Repart[BasicRep].nbentrees),Accord1s(Repart[BasicRep].voies_par_numer),Accord2s(Repart[BasicRep].nbdonnees),(IntAdrs)BasicDonnees);
	if(ModeleRep[(int)Repart[BasicRep].type].status) taille_bloc++;
	if((attente = BasicAttenteModul)) {
		nbatt = nbzero = 0;
		tab = (int32 *)malloc(attente * sizeof(int32));
		ptr = tab;
		do {
			val = *BasicDonnees & 0xFFFF;
			*ptr = val;
			if((val & 0xC000) == 0x4000) break;
			if(val) { --attente; nbatt++; ptr++; } else nbzero++;
		} while(attente);
		printf("%d attente%s, %d FIFO vide%s\n",Accord1s(nbatt),Accord1s(nbzero));
		if(!attente) {
			ptr = tab;
			for(col=0; col<nbatt; ) {
				if(!(col % 10)) printf("%6d:",col / 10);
				printf(" %04X",*ptr++);
				col++;
				if(!(col % 10)) printf("\n");
			}
			if(col % 10) printf("\n");
			free(tab);
			printf("===== Debut de synchronisation introuvable apres %d lecture%s =====\n",
				Accord1s(BasicAttenteModul));
//++			TermRelease(tBasicBlocs);
			OpiumError("Lecture de %d mot%s abandonnee",Accord1s(BasicNbLect));
			return(0);
		}
		free(tab);
	}
	printf("===== Lecture de %d mot%s @%08llX =====\n",Accord1s(BasicNbLect),(IntAdrs)BasicDonnees);
	nb = BasicNbLect;
	if(BasicReadOnly) {
		if(BasicWait) {
			nbzero = 0;
			/* Microseconds((UnsignedWide *)(&t0)); */
			t0 = DateMicroSecs();
			while(nb--) {
				while(!(sec = *BasicDonnees & 0xFFFF)) if(++nbzero > 1000000000) break;
			}
			/* Microseconds((UnsignedWide *)(&t1)); */
			t1 = DateMicroSecs();
		} else {
			nbzero = 0;
			/* Microseconds((UnsignedWide *)(&t0)); */
			t0 = DateMicroSecs();
			while(nb--) {
				if(!(sec = *BasicDonnees & 0xFFFF)) nbzero++;
			}
			/* Microseconds((UnsignedWide *)(&t1)); */
			t1 = DateMicroSecs();
		}
		t1 -= t0;
		sec = (int)(t1 / 1000000);
		usec = (int)(t1 - (sec * 1000000));
		printf("Temps de lecture: %d,%06d s, %d acces avec FIFO vide\n",sec,usec,nbzero);
	} else {
		col = 0; num_bloc = 0;
		/* Microseconds((UnsignedWide *)(&t0)); */
		t0 = DateMicroSecs();
		while(nb--) {
			if(col == 0) {
				printf("%6d: ",num_bloc++);
			} else if((col % Repart[BasicRep].nbentrees) == 0) printf("  ");
			if(attente) {
				printf(" %04X",val);
				attente = 0;
			} else printf(" %04X",*BasicDonnees & 0xFFFF);
			col++;
			if(col == taille_bloc) { printf("\n"); col = 0; }
		}
		/* Microseconds((UnsignedWide *)(&t1)); */
		t1 = DateMicroSecs();
		if(col) printf("\n");
		t1 -= t0;
		sec = (int)(t1 / 1000000);
		usec = (int)(t1 - (sec * 1000000));
		printf("Temps de lecture: %d,%06d s\n",sec,usec);
	}
//++	TermRelease(tBasicBlocs);
	OpiumError("Lecture de %d mot%s terminee",Accord1s(BasicNbLect));
	return(0);
}
#ifdef OBSOLETE
/* ========================================================================== */
static int BasicIpMaj() {
	/* extrait de CVS_EDW/Programmes/OPERA/OPERA_EDW/CEW_linux/cew.c:

	case 0	:	mise_a_jour_et_redemarrage("/mnt/flash/startup","-0");		break;
	case 1	:	mise_a_jour_et_redemarrage("/mnt/flash/startup","-1");		break;
	case 2	:	mise_a_jour_bbv2(1);		break;   [ ndlr: (1) => commence par un startup -2, puis poursuit comme (0) ... ]
	case 3	:	mise_a_jour_bbv2(0);		break;
	case 4	:	mise_a_jour_et_redemarrage("/mnt/flash/startup","-a");		break;
	case 5	:	sauve_config();		break;

		startup -0: arrete cew, le recharge, le dezippe, l'execute
		startup -1: arrete cew, recharge cew_c1, le dezippe, recharge l'Altera, execute cew
		startup -2: recharge bbv2.rbf et le dezippe
		startup -a: arrete cew et vire les gz, recharge tout, dezippe tout, recharge l'Altera, execute cew
		startup   : dezippe tout, recharge l'Altera, execute cew
	*/
	char niveau; byte buffer[4];
	
	if(!RepartIpEcriturePrete(&(Repart[BasicRep]),"              ")) {
		OpiumError("Connexion pour ecriture sur %s:%d impossible",Repart[BasicRep].parm,Repart[BasicRep].ecr.port);
		return(0);
	}
	niveau = 0;
	if(OpiumReadListB("Niveau",OperaCodesMaj,&niveau,14) == PNL_CANCEL) return(0);
	printf("          * Mise a jour de la programmation interne\n");
	buffer[0] = 'Q';
	buffer[1] = (byte)(niveau & 0xFF);
	RepartIpEcrit(&(Repart[BasicRep]),buffer,2); SambaMicrosec(ModeleRep[Repart[BasicRep].type].delai_msg);
	printf("            . Ecrit: %c.%02X\n",buffer[0],buffer[1]);
	return(0);
}
#endif
/* ========================================================================== */
int BasicIpTousMaj() {
	/* voir actions reelles dans BasicIpMaj */
	char niveau; byte buffer[4];
	int rep;
	
	niveau = 0;
	if(OpiumReadListB("Niveau",OperaCodesMaj,&niveau,14) == PNL_CANCEL) return(0);
	printf("          * Mise a jour de la programmation interne de tous les boitiers OPERA\n");
	for(rep=0; rep<RepartNb; rep++) if((Repart[rep].famille == FAMILLE_OPERA) || (Repart[rep].famille == FAMILLE_IPE)) {
		printf("            > Repartiteur @%d.%d.%d.%d: ",IP_CHAMPS(Repart[rep].adrs));
		if(RepartIpEcriturePrete(&(Repart[rep]),"              ")) {
			if(Repart[rep].lec.ip.path < 0) {
				buffer[0] = 'Q';
				buffer[1] = (byte)(niveau & 0xFF);
				RepartIpEcrit(&(Repart[rep]),buffer,2); SambaMicrosec(ModeleRep[Repart[rep].type].delai_msg);
				printf("ecrit %c.%02X\n",buffer[0],buffer[1]);
			} else printf("le chemin de lecture (par IP) n'est pas en service\n");
		} else printf("connexion pour ecriture impossible sur le port %d\n",Repart[rep].ecr.port);
	}
	return(0);
}
/* ========================================================================== */
int BasicIpRaz() {
	int port; byte buffer[4]; ssize_t k;

	printf("          * Arret/demarrage de la lecture sur %s:%d\n",Repart[BasicRep].parm,Repart[BasicRep].ecr.port);
	port = PortLectRep + BasicRep;
	if(Repart[BasicRep].lec.ip.path >= 0) {
		printf("            > Suspension de l'abonnement aux donnees\n");
		buffer[0] = 'P'; buffer[1] = 0;
		buffer[2] = (byte)(port & 0xFF);
		buffer[3] = (byte)((port >> 8) & 0xFF);
		k = RepartIpEcrit(&(Repart[BasicRep]),buffer,4); SambaMicrosec(ModeleRep[Repart[BasicRep].type].delai_msg);
		printf("              . Ecrit: %c.%02X.%02X.%02X, transmis: %ld/%d\n",buffer[0],buffer[1],buffer[2],buffer[3],k,4);
		RepartIpLectureFerme(&(Repart[BasicRep]));
		printf("              . Fermeture de la liaison precedente\n");
	}
	SambaAttends(1000); /* ms */ /* le temps de la boite OPERA se rende compte (i.e. a chaque D3) */
	printf("            > Affectation du port %d pour la lecture\n",port);
	if(Repart[BasicRep].simule) Repart[BasicRep].lec.ip.path = BasicRep;
	else {
		if((Repart[BasicRep].lec.ip.path = SocketUDP(port)) < 0) {
			printf("            !!! Connexion pour lecture sur %s:%d impossible (%s)\n",Repart[BasicRep].parm,port,strerror(errno));
			return(0);
		}
		printf("            > Connexion prete pour la lecture (chemin #%d)\n",Repart[BasicRep].lec.ip.path);
		if(fcntl(Repart[BasicRep].lec.ip.path,F_SETFL,O_NONBLOCK) == -1) {
			printf("              ! Socket bloquante (%s)\n",strerror(errno));
		}
	}
	return(0);
}
/* ========================================================================== */
int BasicIpStart() {
	int port; byte buffer[4]; ssize_t k;
	
	if(!RepartIpEcriturePrete(&(Repart[BasicRep]),"              ")) {
		OpiumError("Connexion pour ecriture sur %s:%d impossible",Repart[BasicRep].parm,Repart[BasicRep].ecr.port);
		return(0);
	}
	port = PortLectRep + BasicRep;
	printf("          * Souscription de l'abonnement aux donnees\n");
	buffer[0] = 'P'; buffer[1] = 0x7F;
	buffer[2] = (byte)(port & 0xFF);
	buffer[3] = (byte)((port >> 8) & 0xFF);
	k = RepartIpEcrit(&(Repart[BasicRep]),buffer,4); SambaMicrosec(ModeleRep[Repart[BasicRep].type].delai_msg);
	printf("            > Ecrit: %c.%02X.%02X.%02X, transmis: %ld/%d\n",buffer[0],buffer[1],buffer[2],buffer[3],k,4);
	return(0);
}
/* ========================================================================== */
int BasicIpEcrit() { RepartiteurIpBasique(&(Repart[BasicRep])); return(0); }
/* ========================================================================== */
int BasicIpLit() {
	byte buffer[BASIC_IPBLOC]; ssize_t nb_lus;

	if(Repart[BasicRep].simule) {
		Repart[BasicRep].gene.started = 1;
		Repart[BasicRep].gene.max = Repart[BasicRep].gene.num_trame + 1;
	}
	nb_lus = RepartIpLitTrame(&(Repart[BasicRep]),buffer,BASIC_IPBLOC);
	Repart[BasicRep].gene.started = 0;
	if(nb_lus < 0) printf("          ! Pas de trame venant de %s (%s)\n",Repart[BasicRep].nom,strerror(errno));
	else if(!nb_lus) printf("          ! Trame de %s vide\n",Repart[BasicRep].nom);
	else {
		printf("          * Trame de %s avec %ld octets\n",Repart[BasicRep].nom,nb_lus);
		if(Repart[BasicRep].famille == FAMILLE_CALI) {
			if(!BigEndianOrder) CaliSwappeTrame(&(Repart[BasicRep]),(CaliTrame)buffer,nb_lus);
			CaliDump(&(Repart[BasicRep]),buffer,nb_lus,BasicDumpHexa);
		} else {
			if(BigEndianOrder) ByteSwappeIntArray((hexa *)buffer,nb_lus / 4);
			if(Repart[BasicRep].famille == FAMILLE_OPERA) OperaDump(buffer,nb_lus,BasicDumpHexa);
			else if(Repart[BasicRep].famille == FAMILLE_IPE) IpeDump(buffer,nb_lus,BasicDumpHexa);
		}
	}

	return(0);
}
/* ========================================================================== */
int BasicCaliRaz() {
	int port,l; char envoye[80];
	TypeRepartiteur *repart;

	repart = &(Repart[BasicRep]);
	printf("          * Arret/demarrage de la liaison IP\n");
	if(!RepartIpEcriturePrete(repart,"              ")) {
		OpiumError("Connexion pour ecriture sur %s:%d impossible",repart->parm,repart->ecr.port);
		return(0);
	}
	port = PortLectRep + BasicRep;
	if(repart->lec.ip.path >= 0) {
		printf("            > Suspension de l'abonnement aux donnees\n");
		l = sprintf(envoye,"w %x %x\r\n",CALI_REG_TIMEOUT,0);
		RepartIpEcrit(repart,envoye,l); SambaMicrosec(ModeleRep[repart->type].delai_msg);
		envoye[l - 2] = '\0';
		printf("              . Ecrit: '%s'\n",envoye);
		RepartIpLectureFerme(repart);
		repart->lec.ip.path = -1;
		printf("              . Fermeture de la liaison precedente\n");
		SambaAttends(100); /* ms */ /* le temps de la boite se rende compte (i.e. a chaque D3) */
	}
	printf("            > Affectation du port %d pour la lecture\n",port);
	if(repart->simule) repart->lec.ip.path = BasicRep;
	else {
		if((repart->lec.ip.path = SocketUDP(port)) < 0) {
			printf("          !!! Connexion pour lecture sur %s:%d impossible (%s)\n",repart->parm,port,strerror(errno));
			return(0);
		}
		printf("            > Connexion prete pour la lecture (chemin #%d)\n",repart->lec.ip.path);
		if(fcntl(repart->lec.ip.path,F_SETFL,O_NONBLOCK) == -1) {
			printf("              ! Socket bloquante (%s)\n",strerror(errno));
		}
	}
	l = sprintf(envoye,"p %d %x",port,repart->s.ip.relance);
	RepartIpEcrit(repart,envoye,l+1); SambaMicrosec(ModeleRep[repart->type].delai_msg);
	printf("            > Ecrit: '%s'\n",envoye);
	return(0);
}
/* ========================================================================== */
int BasicCaliEcrit() {
	char texte[80]; int l; ssize_t n;
	
	if(!RepartIpEcriturePrete(&(Repart[BasicRep]),"              ")) {
		OpiumError("Connexion pour ecriture sur %s:%d impossible",Repart[BasicRep].parm,Repart[BasicRep].ecr.port);
		return(0);
	}
	if(OpiumReadText("Texte commande",BasicIpTexte,77) == PNL_CANCEL) return(0);
	l = sprintf(texte,"%s\r\n",BasicIpTexte);
	n = RepartIpEcrit(&(Repart[BasicRep]),texte,l); SambaMicrosec(ModeleRep[Repart[BasicRep].type].delai_msg);
	printf("          * Ecriture de '%s' [%ld/%d]\n",BasicIpTexte,n,l);
	if(n < 0) printf("            ! en erreur: %s\n",strerror(errno));
	return(0);
}
/* ========================================================================== */
int BasicCaliLit() {
	byte buffer[BASIC_IPBLOC]; ssize_t nb_lus;

	if(Repart[BasicRep].simule) {
		Repart[BasicRep].gene.started = 1;
		Repart[BasicRep].gene.max = Repart[BasicRep].gene.num_trame + 1;
	}
	nb_lus = RepartIpLitTrame(&(Repart[BasicRep]),buffer,BASIC_IPBLOC);
	Repart[BasicRep].gene.started = 0;
	if(nb_lus < 0) printf("          ! Pas de trame venant de %s (%s)\n",Repart[BasicRep].nom,strerror(errno));
	else if(!nb_lus) printf("          ! Trame de %s vide\n",Repart[BasicRep].nom);
	else {
		printf("          * Trame de %s avec %ld octets\n",Repart[BasicRep].nom,nb_lus);
		if(!BigEndianOrder) CaliSwappeTrame(&(Repart[BasicRep]),(CaliTrame)buffer,nb_lus);
		CaliDump(&(Repart[BasicRep]),buffer,nb_lus,BasicDumpHexa);
	}
	return(0);
}
/* ========================================================================== */
int BasicSmbRaz() {
	TypeRepartiteur *repart; int port;

	repart = &(Repart[BasicRep]);
	printf("          * Arret/demarrage de la liaison IP\n");
	if(!RepartIpEcriturePrete(repart,"              ")) {
		OpiumError("Connexion pour ecriture sur %s:%d impossible",repart->parm,repart->ecr.port);
		return(0);
	}
	port = PortLectRep + BasicRep;
	if(repart->lec.ip.path >= 0) {
		printf("            > Suspension de l'abonnement aux donnees\n");
		SmbEcrit(repart,SMB_ARRET);
		printf("              . Ecrit: '%s'\n",RepartiteurValeurEnvoyee);
		RepartIpLectureFerme(repart);
		repart->lec.ip.path = -1;
		printf("              . Fermeture de la liaison precedente\n");
		SambaAttends(100); /* ms */ /* le temps de la boite se rende compte (i.e. a chaque D3) */
	}
	printf("            > Affectation du port %d pour la lecture\n",port);
	if(repart->simule) repart->lec.ip.path = BasicRep;
	else {
		if((repart->lec.ip.path = SocketUDP(port)) < 0) {
			printf("            !!! Connexion pour lecture sur %s:%d impossible (%s)\n",repart->parm,port,strerror(errno));
			return(0);
		}
		printf("            > Connexion prete pour la lecture (chemin #%d)\n",repart->lec.ip.path);
		if(fcntl(repart->lec.ip.path,F_SETFL,O_NONBLOCK) == -1) {
			printf("              ! Socket bloquante (%s)\n",strerror(errno));
		}
	}
	SmbEcrit(repart,SMB_CONNECT,(shex)port);
	printf("          > Ecrit: '%s'\n",RepartiteurValeurEnvoyee);
	return(0);
}
/* ========================================================================== */
int BasicSmbEcrit() {
	int n;
	
	if(!RepartIpEcriturePrete(&(Repart[BasicRep]),"              ")) {
		OpiumError("Connexion pour ecriture sur %s:%d impossible",Repart[BasicRep].parm,Repart[BasicRep].ecr.port);
		return(0);
	}
	if(OpiumExec(pBasicSmbEnvoi->cdr) == PNL_CANCEL) return(0);
	n = SmbEcrit(&(Repart[BasicRep]),BasicSmbCmde[0],BasicSmbC1,BasicSmbC2,BasicSmbVal);
	printf("          * Ecriture de '%s' via <%d> [%d/5]\n",RepartiteurValeurEnvoyee,Repart[BasicRep].ecr.ip.path,n);
	if(n < 0) printf("          ! en erreur: %s\n",strerror(errno));
	return(0);
}
/* ========================================================================== */
int BasicSmbLit() {
	byte buffer[BASIC_IPBLOC]; ssize_t nb_lus;
	
	nb_lus = RepartIpLitTrame(&(Repart[BasicRep]),buffer,BASIC_IPBLOC);
	if(nb_lus < 0) printf("          ! Pas de trame (%s)\n",strerror(errno));
	else if(!nb_lus) printf("          ! Trame vide\n");
	else {
		if(!BigEndianOrder) CaliSwappeTrame(&(Repart[BasicRep]),(CaliTrame)buffer,nb_lus);
		CaliDump(&(Repart[BasicRep]),buffer,nb_lus,BasicDumpHexa);
	}
	return(0);
}
/* ========================================================================== */
static int BasicChangeAdrs(Panel p, void *arg) {
	if(BasicCmdes) { BasicDonnees = BasicCmdes; BasicReset = (Ulong *)((long)BasicCmdes + 8); }
	else { BasicReset = BasicCmdes = &BasicPCIout; BasicDonnees = &BasicPCIin; }
	printf("          * Lecture PCI a l'adresse %08llX\n",(IntAdrs)BasicDonnees);
	return(1);
}
/* ========================================================================== */
TypeMenuItem iBasicDefinitions[] = {
	{ "Recuperer",    MNU_FONCTION BasicRecupere },
	{ "Lister",       MNU_FONCTION BasicRepertoire },
	{ "Creer",        MNU_FONCTION BasicCree },
	{ "Modifier",     MNU_FONCTION BasicEdite },
	{ "Effacer",      MNU_FONCTION BasicEfface },
	{ "Sauver",       MNU_FONCTION BasicSauve },
	{ "Dumper",       MNU_FONCTION BasicDump },
	{ "Fermer",       MNU_RETOUR },
	MNU_END
};
TypeMenuItem iBasicEdite[] = {
	{ "Lister",       MNU_FONCTION BasicListe },
	{ "Ajouter",      MNU_FONCTION BasicAjoute },
	{ "Modifier",     MNU_FONCTION BasicModifie },
	{ "Retirer",      MNU_FONCTION BasicRetire },
	{ "Dumper",       MNU_FONCTION BasicDump },
	{ "Fermer",       MNU_RETOUR },
	MNU_END
};
TypeMenuItem iBasicDestin[] = {
	{ "pci",     MNU_CONSTANTE 1 },
	{ "cc",      MNU_CONSTANTE 2 },
	{ "quitter", MNU_CONSTANTE 9 },
	MNU_END
};
TypeMenuItem iBasicStyleMiG[] = {
	{ "commande", MNU_CONSTANTE 1 },
	{ "bus util", MNU_CONSTANTE 2 },
	{ "quitter",  MNU_CONSTANTE 9 },
	MNU_END
};
static Menu mBasicPCI,mBasicIp,mBasicCali,mBasicSmb;
#ifdef DEPLACE_DANS_SAMBA_H
static TypeMenuItem iBasicPCI[] = {
	{ "Definitions",  MNU_MENU    &mBasicDefinitions },
	{ "Ecriture",     MNU_FONCTION  BasicEcritures },
	{ "Acces direct", MNU_FONCTION  BasicDirect },
	{ "Lecture",      MNU_FONCTION  BasicLectures },
	{ "Reset FIFO",   MNU_FONCTION  BasicResetFIFO },
	{ "Preferences",  MNU_PANEL   &pBasicDesc },
	{ "Fermer",       MNU_RETOUR },
	MNU_END
};
static TypeMenuItem iBasicIp[] = {
	{ "Ecriture",     MNU_FONCTION  BasicIpEcrit },
//	{ "Mise a jour",  MNU_FONCTION  BasicIpMaj }, // remplace par procedures init et reset
//-	{ "MAJ globale",  MNU_FONCTION  BasicIpTousMaj },
//-	{ "Parametrage",  MNU_FONCTION  BasicIpParms },
	{ "Vidage",       MNU_FONCTION  BasicIpRaz },
	{ "Demarrage",    MNU_FONCTION  BasicIpStart },
	{ "Lecture",      MNU_FONCTION  BasicIpLit },
	{ "Fermer",       MNU_RETOUR },
	MNU_END
};
static TypeMenuItem iBasicCali[] = {
	{ "Preferences",  MNU_PANEL   &pBasicDesc },
	{ "Demarrage",    MNU_FONCTION  BasicCaliRaz },
	{ "Ecriture",     MNU_FONCTION  BasicCaliEcrit },
	{ "Lecture",      MNU_FONCTION  BasicCaliLit },
	{ "Fermer",       MNU_RETOUR },
	MNU_END
};
static TypeMenuItem iBasicSmb[] = {
	{ "Demarrage",    MNU_FONCTION  BasicSmbRaz },
	{ "Ecriture",     MNU_FONCTION  BasicSmbEcrit },
	{ "Lecture",      MNU_FONCTION  BasicSmbLit },
	{ "Fermer",       MNU_RETOUR },
	MNU_END
};
#endif /* DEPLACE_DANS_SAMBA_H */

/* ========================================================================== */
int BasicCommandes() {
	if(AccesRestreint()) return(0);
	if(RepartNb > 1) {
		if(OpiumReadList("Repartiteur",RepartListe,&BasicRep,12) == PNL_CANCEL) return(0);
	} else if(RepartNb > 0) BasicRep = 0;
	else { OpiumError("Pas de repartiteur declare!"); return(0); }
	
	printf("%s/ Acces direct au repartiteur %s\n",DateHeure(),RepartListe[BasicRep]);
	if(Repart[BasicRep].interf == INTERF_PCI) {
		PCInum = Repart[BasicRep].adrs.val - 1;
		OpiumExec(mBasicPCI->cdr);
	} else if((Repart[BasicRep].famille == FAMILLE_OPERA) 
		|| (Repart[BasicRep].famille == FAMILLE_IPE)) OpiumFork(mBasicIp->cdr);
	else if(Repart[BasicRep].famille == FAMILLE_CALI) OpiumFork(mBasicCali->cdr);
	else if(Repart[BasicRep].famille == FAMILLE_SAMBA) OpiumFork(mBasicSmb->cdr);
	else OpiumError("Interface non supportee");
	return(0);
}
/* ========================================================================== */
void BasicInit() {
	int i;
#ifdef AVEC_PCI
	int j;
#endif

/* Initialisation generale */
	for(i=0; i<MAXMODIFS; i++) BasicParmText[i] = (char *)malloc(MAXTEXTE);
	for(i=0; i<MAXBLOCS; i++) BasicBlocNom[i] = BasicBloc[i].nom;
	BasicBlocNom[i] = "\0";
	BasicParmDebut = 1;
	BasicParmFin = 1;

	BasicParmsNb = BasicBlocsNb = 0;
	BasicDumpHexa = 1;
	BasicReadOnly = 0;
	BasicWait = 0;
	BasicAttenteModul = 0;
	BasicBlocNum = -1;
	BasicStatusCC = BasicAdrsCC = BasicSsAdrsCC = BasicValCC = 0;
	BasicNbLect = 1;
	BasicASauver = 0;
	BasicRep = 0;
	BasicOctet = 0;
	BasicIpCde = 0; /* h */
	BasicIpTexte[0] = '\0';
	strcpy((char *)BasicSmbCmde,"m"); BasicSmbC1 = BasicSmbC2 = 0; BasicSmbVal = 0x100;
#ifdef ADRS_PCI_DISPO
	BasicCmdes = PCIadrs[PCInum];
#endif
	strcpy(BasicUserBlocs,"Commandes");
	strcat(strcpy(BasicFichierBlocs,PrefPath),BasicUserBlocs);

/* Creation de l'interface */
	mBasicPCI = MenuLocalise(iBasicPCI);
	mBasicIp = MenuLocalise(iBasicIp);
	mBasicCali = MenuLocalise(iBasicCali);
	mBasicSmb = MenuLocalise(iBasicSmb);
	mBasicDefinitions = MenuLocalise(iBasicDefinitions);
	mBasicEdite = MenuLocalise(iBasicEdite);
//-	OpiumEnterFctn(mBasic->cdr,AccesRestreint,0);

	pBasicDesc = PanelDesc(BasicDesc,1);
	PanelTitle(pBasicDesc,"Preferences");
	PanelOnApply(pBasicDesc,BasicChangeAdrs,0);
	PanelOnOK(pBasicDesc,BasicChangeAdrs,0);

	pBasicSmbEnvoi = PanelCreate(4);
	PanelText(pBasicSmbEnvoi,"Code",(char *)BasicSmbCmde,2);
	PanelByte(pBasicSmbEnvoi,"Octet 0",&BasicSmbC1);
	PanelByte(pBasicSmbEnvoi,"Octet 1",&BasicSmbC2);
	PanelSHex(pBasicSmbEnvoi,"Valeur",(short *)&BasicSmbVal);

	pBasicFichierLect = PanelCreate(1);
	PanelFile(pBasicFichierLect,"Depuis quel fichier",BasicFichierBlocs,MAXFILENAME);
	pBasicFichierEcri = PanelCreate(1);
	PanelFile(pBasicFichierEcri,"Sur quel fichier",BasicFichierBlocs,MAXFILENAME);

	pBasicBlocCree = PanelCreate(1);
	PanelText(pBasicBlocCree,"Sous quel nom",BasicBlocDemande,DETEC_NOM);
	PanelTitle(pBasicBlocCree,"Creation de bloc");
	pBasicBlocDemande = PanelCreate(1);
	PanelList(pBasicBlocDemande,"Nom du bloc",BasicBlocNom,&BasicBlocNum,DETEC_NOM);
	pBasicParmDebut = PanelCreate(1);
	PanelInt(pBasicParmDebut,"A partir de quel numero de parametre",&BasicParmDebut);
	pBasicParmRange = PanelCreate(2);
	PanelInt(pBasicParmRange,"A partir de quel numero de parametre",&BasicParmDebut);
	PanelInt(pBasicParmRange,"Jusqu'a quel numero de parametre",&BasicParmFin);
	pBasicModifs = PanelCreate(MAXMODIFS);

	mBasicDestin = MenuLocalise(iBasicDestin);
	pBasicDirectICA = PanelCreate(3);
	PanelItemFormat(pBasicDirectICA,PanelHex(pBasicDirectICA,"Adresse",&BasicAdrsICA),"%02X");
	PanelItemFormat(pBasicDirectICA,PanelHex(pBasicDirectICA,"Valeur",&BasicValICA),"%08X");
	pBasicDirectCC = PanelCreate(3);
	PanelItemFormat(pBasicDirectCC,PanelHex(pBasicDirectCC,"Adresse",&BasicAdrsCC),"%02X");
	PanelItemFormat(pBasicDirectCC,PanelHex(pBasicDirectCC,"Sous-adresse",&BasicSsAdrsCC),"%02X");
	PanelItemFormat(pBasicDirectCC,PanelHex(pBasicDirectCC,"Valeur",&BasicValCC),"%04X");
	mBasicStyleMiG = MenuLocalise(iBasicStyleMiG);
	pBasicOctet = PanelCreate(1);
	PanelItemFormat(pBasicOctet,PanelHex(pBasicOctet,"Octet a ecrire",&BasicOctet),"%02X");
#ifdef AVEC_PCI
	pBasicRegsICA = PanelCreate(ICA_REG_NB);
	for(i=0; i<ICA_REG_NB; i++) {
		sprintf(BasicIcaNom[i],"ICA.%02X",i*4);
		j = PanelHex(pBasicRegsICA,BasicIcaNom[i],&(BasicIcaVal[i]));
		PanelItemSelect(pBasicRegsICA,PanelItemFormat(pBasicRegsICA,j,"%08X"),0);
	}
	OpiumPosition(pBasicRegsICA->cdr,300,200);
#endif
	tBasicStatusCC = TermCreate(20,40,1000);
	tBasicDirectLog = TermCreate(24,25,65536);
	OpiumPosition(mBasicDestin->cdr,50,200);
	OpiumPosition(pBasicDirectICA->cdr,50,220);
	OpiumPosition(pBasicDirectCC->cdr,50,220);
	OpiumPosition(mBasicStyleMiG->cdr,50,200);
	OpiumPosition(pBasicOctet->cdr,50,220);
	OpiumPosition(tBasicStatusCC->cdr,450,200);
	OpiumPosition(tBasicDirectLog->cdr,10,300);

	pBasicNbLect = PanelCreate(1);
	PanelInt(pBasicNbLect,"Nombre de mots a lire",&BasicNbLect);

	tBasicBlocs = TermCreate(24,80,65536);

	BasicPancarte = InfoCreate(1,40);
	InfoTitle(BasicPancarte,"Information");

	return;
}
/* ========================================================================== */
int BasicSetup() {
	/* Modifications utilisateur (preferences) */
#ifdef _claps_h
	ArgScan(FichierPrefBasic,BasicDesc);
#ifndef PCI_DIRECT
	BasicCmdes = 0;
#endif
#endif
	if(BasicCmdes) { BasicDonnees = BasicCmdes; BasicReset = (Ulong *)((long)BasicCmdes + 8); }
	else { BasicReset = BasicCmdes = &BasicPCIout; BasicDonnees = &BasicPCIin; }
	if(CompteRendu.config.hw) printf("    > Acces basique au PCI via l'adresse %08X (BasicSetup)\n",BasicDonnees);
	
	return(1);
}
/* ========================================================================== */
int BasicExit() {
	int rc;

	if((rc = SambaSauve(&BasicASauver,"commandes basiques",1,1,BasicSauve))) {
#ifdef _claps_h
		if(BasicCmdes == &BasicPCIout) BasicCmdes = (Ulong *)0;
		ArgPrint(FichierPrefBasic,BasicDesc);
#endif
	}
	return(rc);
}
