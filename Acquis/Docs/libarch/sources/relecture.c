#include <environnement.h>

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#ifndef WIN32
#include <strings.h>  /* pour strcpy */
#else
#include <string.h>
#include <io.h>
#endif
#include <fcntl.h>

#include <decode.h>

// #define ACCEPTE_EDW1
#define CORRIGE_PATTERN
// #define CORRIGE_CROSSTALK
#define REFAIT_FILTRAGE

#ifdef REFAIT_FILTRAGE
#include <filtres.h>
#endif

#define ARCHIVE_C
#define DETECTEURS_C
#include <rundata.h>
#include <detecteurs.h>
#include <archive.h>       /* EDW2 */
#include <evtread.h>

static TypeFiltre *FiltreVoie[VOIES_MAX]; // Pointeur sur coefficient de filtrage

static char ByteSwappe;
static char Depatterne,Decrosstalke,Refiltre;
static int  VoieChaleur,VoieCentre,VoieGarde;

#define MAXARCHENTETE 80
#define BLOC_SCAN 1024

#define NOM_CHALEUR "chaleur"
#define NOM_CENTRE "centre"
#define NOM_GARDE "garde"
#define NOM_RAPIDE "rapide"

void SambaDefaults();
void SettingsDefaults();
void TrmtNomme();
int ArchEdw2Hdr(int f, char imprime_actions, char imprime_entetes, char *explic);

/* ========================================================================== */
char ArchDataInit() {
	int voie,vt;

	SambaDefaults();
	SettingsDefaults();
	TrmtNomme();
	Depatterne = Decrosstalke = Refiltre = 0;
	for(voie=0; voie<VOIES_MAX; voie++) FiltreVoie[voie] = 0; /* filtrage SAMBA par defaut */
	for(vt=0; vt<VOIES_MAX; vt++) {
		ArchEvt.voie[vt].filtrees = 0;
		ArchEvt.voie[vt].nbfiltres = 0;
		ArchEvt.voie[vt].demdim = 0;
		ArchEvt.voie[vt].demarrage = 0;
        /* Attribution d'une adresse pour toutes les voies d'un evenement */
	}
	return(1);
}

#ifdef ACCEPTE_EDW1
static off_t ArchLngrEdw1;
/* ========================================================================== */
static void ArchEdw1Hdr(int fdata, char *nom) {
	char nom_complet[256];
	char ligne[80];
	int dim; int bolo,voie,vm;
	FILE *frame0;
	int64 t0;

	strcat(strcpy(nom_complet,nom),".txt");
	dim = 2;
	if(frame0 = TangoFileOpen(nom_complet,"r")) {
		while(LigneSuivante(ligne,80,frame0)) {
			if(!strncmp(ligne+2,"but de run",10)) {  /* 'Debut' ave l'accent! (xE9) */
				fscanf(frame0,"%ld",&t0);
				ArchT0sec = (int)(t0 - 3000000000L); ArchT0msec = 0;
			} else if(!strncmp(ligne,"Nombre de voies acquises",24)) {
				fscanf(frame0,"%d",&dim);
				VoiesNb = VoiesGerees = dim;
				if(dim == 9) { ModeleVoieNb = 3; BoloGeres = 3; }
				else if(dim == 3) { ModeleVoieNb = 3; BoloGeres = 1; }
				else if(dim == 2) { ModeleVoieNb = 2; BoloGeres = 1; }
				if(!SambaAlloc()) {
					printf(". Memoire indisponible. Lecture abandonnee\n");
					return;
				}
			} else if(!strncmp(ligne,"Liste des longueurs",19)) {
				for(voie=0; voie<VoiesNb; voie++) {
					fscanf(frame0,"%d",&(VoieEvent[voie].lngr));
					printf(". Voie #%d: %d echantillons\n",voie,VoieEvent[voie].lngr);
				}
			} else if(!strncmp(ligne,"Liste des time sampling",23)) {
				for(voie=0; voie<VoiesNb; voie++) {
					fscanf(frame0,"%g",&(VoieEvent[voie].horloge));
					VoieEvent[voie].horloge *= 1000.0;
					printf(". Voie #%d: %g ms/echantillon\n",voie,VoieEvent[voie].horloge);
				}
			} else if(!strncmp(ligne,"Liste des nombres de pretrig samples",36)) {
				for(voie=0; voie<VoiesNb; voie++) {
					fscanf(frame0,"%d",&(VoieEvent[voie].avant_evt));
					printf(". Voie #%d: %d echantillons dans le pretrigger\n",voie,VoieEvent[voie].avant_evt);
				}
			}
		}
		fclose(frame0);
	} else {
		HorlogeSysteme = 100; /* MHz */ Diviseur0 = 20; Diviseur1 = 50;
		VersionRepartiteur = 0.1;
		BoloGeres = 1;
		VoiesNb = VoiesGerees = BoloGeres * ModeleVoieNb;
		if(!SambaAlloc()) {
			printf(". Memoire indisponible. Lecture abandonnee\n");
			return;
		}
#define PLANAR
#ifdef EDW1
		VoieEvent[0].lngr = 8192; VoieEvent[0].horloge = 0.5;    /* ms */
		VoieEvent[1].lngr = 2048; VoieEvent[1].horloge = 5.0e-3; /* ms */
		VoieEvent[0].avant_evt = 384;
		VoieEvent[1].avant_evt = VoieEvent[2].avant_evt = 1024;
#endif
#ifdef PLANAR
		VoieEvent[0].lngr = 500; VoieEvent[0].horloge = 0.001;    /* ms */
		VoieEvent[0].avant_evt = 100;
#endif
	}

	for(bolo=0; bolo<BoloGeres; bolo++) {
		Bolo[bolo].lu = DETEC_EN_SERVICE;
		Bolo[bolo].bb.adrs = 0;
		sprintf(Bolo[bolo].nom,"Bolo%d",bolo+1);
	}
	/* Un peu naif... 
	for(voie=0; voie<VoiesNb; voie++)
		for(bolo=0; bolo<BoloGeres; bolo++) {
			dim = fread(VoieEvent[voie].brutes.t.sval,2,VoieEvent[voie].lngr,ArchFile);
			if(dim != VoieEvent[voie].lngr) return(0);
		}
	  ... alors qu'en fait, c'est du Chardin! */
	ModeleVoie[ModeleVoieNb].nom[0] = '\0';
	ArchLngrEdw1 = 0;
	for(voie=0; voie<VoiesNb; voie++) {
		if(voie < BoloGeres) {
			vm = 0;
			bolo = voie;
		} else {
			vm = ((voie - BoloGeres) % (ModeleVoieNb - 1)) + 1;
			bolo = (voie - BoloGeres) / (ModeleVoieNb - 1);
		}
		VoieManip[voie].type = vm;
		VoieManip[voie].det = bolo;
		sprintf(VoieManip[voie].nom,"voie%d bolo%d",vm+1,bolo+1);
		printf(". Voie #%d: %s (type #%d)\n",voie,VoieManip[voie].nom,VoieManip[voie].type);
		ArchLngrEdw1 += VoieEvent[voie].lngr;
	}
	ArchLngrEdw1 *= sizeof(TypeDonnee);
	for(bolo=0; bolo<BoloGeres; bolo++) {
		Bolo[bolo].nbcapt = 0;
		for(vm=0; vm<ModeleVoieNb; vm++) {
			for(voie=0; voie<VoiesNb; voie++) {
				if((VoieManip[voie].det == bolo) && (VoieManip[voie].type == vm)) break;
			}
			if(voie < VoiesNb) {
				Bolo[bolo].voie[vm] = voie;
				Bolo[bolo].nbcapt += 1;
			}
		}
	}
	BigEndianSource = 1;
	BigEndianOrder = EndianIsBig();
	return;
}
#endif
/* ========================================================================== */
int ArchRunHeader(int f, int *t0sec, int *t0msec, char log, char *explic) {
	int vm,nb;
#ifdef ACCEPTE_EDW1
	char nom_complet[256];
	int vsn_lue,dim,lus; char entete[32]; char *c;

	sprintf(explic,"Incident indetermine depuis %s",__func__);
	/* Type de fichier selon format interne apparent */
#ifdef VERIFIE_VERSION
	dim = 32;
	lus = read(f,entete,dim);
	if(lus != dim) {
		strcpy(explic,strerror(errno));
		return(-1);
	}
	if(memcmp(entete,SIGNATURE,strlen(SIGNATURE))) vsn_lue = 1;
	else vsn_lue = 2;
	if(vsn_lue != VsnManip) {
		sprintf(explic,"%s n'est pas de version %d, mais %d",
			nom_complet,VsnManip,vsn_lue);
		return(0);
	}
	lseek(f,0,SEEK_SET);
#endif

	if(VsnManip == 1) ArchEdw1Hdr(f,nom); else 
#else /* ACCEPTE_EDW1 */
	VsnManip = 2;
#endif /* ACCEPTE_EDW1 */

	ArchEdw2Hdr(f,log,log,explic);
	if(log) printf("  . Chemin ouvert: #%d\n",f);

	ByteSwappe = (BigEndianSource != BigEndianOrder);
	nb = ModeleVoieNb; if(nb > VOIES_MAXI) nb = VOIES_MAXI;
	for(vm=0; vm<nb; vm++) VoieListe[vm] = ModeleVoie[vm].nom;
	VoieListe[vm] = "\0";
	VoieChaleur = VoieCentre = VoieGarde = -1;
	for(vm=0; vm<ModeleVoieNb; vm++) {
		if(!strcmp(ModeleVoie[vm].nom,NOM_CHALEUR)) VoieChaleur = vm;
		else if(!strcmp(ModeleVoie[vm].nom,NOM_CENTRE)) VoieCentre = vm;
		else if(!strcmp(ModeleVoie[vm].nom,NOM_GARDE)) VoieGarde = vm;
	}
	if(log) SambaDumpVoies(0,1);
	*t0sec = ArchT0sec;
	*t0msec = ArchT0msec;
//-	ArchVoieNbfiltres = 0;
	
	return(1);
}
/* ========================================================================== */
int ArchRunFree() {
	int voie;

	for(voie=0; voie<VoiesNb; voie++) {
		if(VoieEvent[voie].brutes.t.sval) {
            // printf("(%s) Liberation %s @%08X\n",__func__,VoieManip[voie].nom,(hexa)VoieEvent[voie].brutes.t.sval);
			free(VoieEvent[voie].brutes.t.sval);
            VoieEvent[voie].brutes.t.sval = 0;
			VoieEvent[voie].brutes.max = 0;
		}
	}
	return(1);
}
/* ========================================================================== */
int ArchEvtSkip(int f, char *explic) {
	int vt; int rc,bolo_lu;
	off_t lngr,pos;
	char entete[MAXARCHENTETE],c,*r;

#ifdef ACCEPTE_EDW1
	if(VsnManip == 1) {
		ArchEvt.num = EvtNb;
		if(lseek(f,ArchLngrEdw1,SEEK_CUR) == -1) {
			sprintf(explic,"Positionnement dans le fichier impossible (%s)",strerror(errno));
			return(0);
		}
	} else 
#endif
		if(VsnManip == 2) {
		// printf("#%2d: Debut d'evenement @ %08X\n",EvtNb,(long)lseek(f,0,SEEK_CUR));
		do {
			if((rc = read(f,entete,13)) == 0) {
				sprintf(explic,"Fin de fichier rencontree, sans marque de fin d'archivage");
				return(0);
			} else if(rc < 0) {
				sprintf(explic,"Lecture de la marque d'evenement en erreur (%s)",strerror(errno));
				return(rc);
			}
			entete[13] = '\0';
			do {
				if((rc = read(f,&c,1)) == 0) {
					sprintf(explic,"Fin de fichier rencontree, sans marque de fin d'archivage");
					return(0);
				} else if(rc < 0) {
					sprintf(explic,"Recherche de fin de marque d'evenement en erreur (%s)",strerror(errno));
					return(rc);
				}
			} while((c != '\n') && (c != '\0'));
			if(!strncmp(entete,"* Arret ",8)) {
				sprintf(explic,"Marque de fin d'archivage rencontree");
				return(0);
			}
			else if(!strncmp(entete,"* Evenement ",12)) break;
		} while(1);
		ArchEvt.nbvoies = 1;
		if((rc = ArgFromPath(f,ArchHdrEvt,LIMITE_ENTETE))) {
			if(rc > 0) sprintf(explic,"Entete d'evenement en erreur (%s)",strerror(errno));
			else sprintf(explic,"Entete d'evenement illisible (%s)",strerror(errno));
			return(0);
		}
		ArchEvt.nbvoies = ArchVoiesNb;
		if(Version < 7.0) {
			read(f,&bolo_lu,sizeof(int));
			ArchEvt.nbvoies = ModeleVoieNb;
		}
		for(vt=0; vt<ArchEvt.nbvoies; vt++) {
			if(Version > 6.9) {
//				if((rc = read(f,entete,MAXARCHENTETE)) <= 0) {
//					sprintf(explic,"Lecture de la marque de voie en erreur (%s)",strerror(errno));
//					return(rc);
//				}
//				entete[MAXARCHENTETE-1] = '\0';
				pos = lseek(f,0,SEEK_CUR);
				r = entete;
				do {
					if((rc = read(f,r,1)) == 0) {
						sprintf(explic,"Fin de fichier rencontree, voie #%d attendue, sans marque de fin d'archivage",vt+1);
						return(0);
					} else if(rc < 0) {
//						sprintf(explic,"Recherche de fin de marque de voie en erreur (%s)",strerror(errno));
						sprintf(explic,"Lecture de la marque de voie en erreur (%s)",strerror(errno));
						return(rc);
					}
					if((*r == '\n') || (*r == '\0')) break;
				} while((++r - entete) < MAXARCHENTETE);
				*r = '\0';
				if(strncmp(entete,"* Voie",6)) {
					if(!strncmp(entete,"* Evenement ",12)) {
						ArchEvt.nbvoies = vt;
						lseek(f,pos,SEEK_SET);
						break;
					} else if(!strncmp(entete,"* Arret ",8)) {
						ArchEvt.nbvoies = vt;
						sprintf(explic,"Marque de fin d'archivage rencontree");
						return(0);
					}
					sprintf(explic,"Entete de voie #%d en defaut dans l'evt #%d (%s...)",vt,ArchEvt.num,entete);
					return(0);
				}
				ArgFromPath(f,ArchHdrVoieEvt,LIMITE_ENTETE);
				if(ArchVoie.dim <= 0) continue;
	#define BIDOUILLE_TEMPORAIRE
	#ifdef BIDOUILLE_TEMPORAIRE
				if((rc = read(f,entete,6)) <= 0) return(rc);
				entete[6] = '\0';
				lseek(f,-6,SEEK_CUR);
				if(!strncmp(entete,"* Voie",6) || !strncmp(entete,"####",4)
				|| !strncmp(entete,"* Evenement ",6) || !strncmp(entete,"* Sauvegarde ",6)) continue;
	#endif
				lseek(f,ArchVoie.nbfiltres * sizeof(double),SEEK_CUR);
			} else {
				int voie,dim,nbfiltres,recalcul;
				read(f,&voie,sizeof(int));
				read(f,&dim,sizeof(int));
				read(f,&nbfiltres,sizeof(int));
				read(f,&recalcul,sizeof(int));
				if(ByteSwappe) ByteSwappeInt((unsigned int *)(&voie));
				ArchVoie.num = voie;
				if(ByteSwappe) ByteSwappeInt((unsigned int *)(&nbfiltres));
				if(nbfiltres) lseek(f,nbfiltres * sizeof(double),SEEK_CUR);
			}
			lngr = ArchVoie.dim * sizeof(TypeDonnee);
			lseek(f,lngr,SEEK_CUR);
		}
		if(Version < 7.0) read(f,&bolo_lu,sizeof(int)); /* on doit retrouver -1 */
//		printf("#%2d:   Fin d'evenement @ %08X\n",EvtNb,(long)lseek(f,0,SEEK_CUR));
	}
	return(1);
}
/* ========================================================================== */
int ArchEvtLastNum() { return(ArchEvt.num); }
/* ========================================================================== */
static off_t ArchEvtSearch(int f, int num, int *lu, off_t *of7) {
	/* retourne -2 si evt avant debut, -3 si evt apres fin
	   num_lu et of7 donnent le dernier evt lu en tout etat de cause */
	off_t propose,pos,debut_bloc,prochain_bloc,trop_loin,debut_scan;
	int recule; char trouve;
	int num_lu;
	int rc;
	char explic[256];
	char bloc[BLOC_SCAN]; int l,lngr;
#define ENTETE_LNGR 80
	char entete[ENTETE_LNGR],*r;

	propose = lseek(f,0,SEEK_CUR);
	trop_loin = lseek(f,0,SEEK_END);
	debut_bloc = debut_scan = lseek(f,propose,SEEK_SET);
//1	printf("(ArchEvtSearch) debut scan @%lld, limite=%lld\n",debut_scan,trop_loin);
	recule = 0; trouve = 0; *of7 = -1; * lu = -1;
	do {
		lngr = BLOC_SCAN;
		if((debut_bloc + lngr) >= trop_loin) lngr = (int)(trop_loin - debut_bloc - 1);
//2		printf("(ArchEvtSearch) lecture bloc[%d] @%lld (vrai %lld)\n",lngr,debut_bloc,lseek(f,0,SEEK_CUR));
		if((rc = read(f,bloc,lngr)) <= 0) return(rc);
		prochain_bloc = debut_bloc + lngr;
		for(l=0; l<lngr; l++) {
			if(bloc[l] == '*') {
//2				printf("(ArchEvtSearch) marqueur trouve @%lld\n",debut_bloc + l);
				pos = debut_bloc + l;
				if((pos + 8) >= trop_loin) { recule = 2; break; }
				else {
					lseek(f,pos,SEEK_SET);
					if((rc = read(f,entete,8)) <= 0) return(rc);
					if(!strncmp(entete,"* Arret ",8)) recule = 2;
					else if(!strncmp(entete,"* Evenement ",8)) {
						r = entete;
						do { if((rc = read(f,r,1)) <= 0) return(rc); } while((*r++ != ' ') && ((r - entete) < ENTETE_LNGR));
						*of7 = pos;
						r = entete;
						do { if((rc = read(f,r,1)) <= 0) return(rc); } while((*r++ != '\n') && ((r - entete) < ENTETE_LNGR));
//2						*(r-1) = '\0';
//2						printf("(ArchEvtSearch) Trouvee fin entete: [%s]\n",entete);
						sscanf(entete,"%d",&num_lu);
//1						printf("(ArchEvtSearch) Trouve evenement %d\n",num_lu);
						*lu = num_lu;
						if(Discontinu) trouve = 1;
						if(num_lu > num) recule = num_lu - num + 1;
						else if(num_lu == num) trouve = 1; /* du coup, on est apres '* Evenement..' et donc bien positionnes */
						else if(num_lu < num) {
							lseek(f,pos,SEEK_SET);
							for(l=num_lu; l<num; l++) {
								pos = lseek(f,0,SEEK_CUR);
								if(!ArchEvtSkip(f,explic)) break; /* la par contre, on s'arrete avant, et il faudra sauter le marqueur */
								else *of7 = pos;
							}
//1							printf("(ArchEvtSearch) Saute %d evenement(s) en plus\n",l-num_lu-1);
							*lu = ArchEvt.num;
							if(l == num) trouve = 1;
							else return(-3); /* Attention, valeur particuliere (voir appel) */
						}
						break;
					}
					if(!recule) lseek(f,prochain_bloc,SEEK_SET);
				}
			}
		}
		if(trouve) break;
		if(lngr < BLOC_SCAN) recule = 2;
		if(recule) {
			if(debut_scan == 0) return(-2); /* Attention, valeur particuliere (voir appel) */
			prochain_bloc = debut_scan - (recule * EvtDim);
			if(prochain_bloc < 0) prochain_bloc = 0;
			trop_loin = debut_scan;
			debut_scan = prochain_bloc;
			lseek(f,prochain_bloc,SEEK_SET);
			recule = 0;
//1			printf("(ArchEvtSearch) debut scan @%lld, limite=%lld\n",debut_scan,trop_loin);
		}
		debut_bloc = prochain_bloc;
	} while(1);

	return(1);
}
/* ========================================================================== */
int ArchEvtPattern(char actif) {
	Depatterne = actif;
	return(1);
}
/* ========================================================================== */
int ArchEvtCrosstalk(char actif) {
	Decrosstalke = actif;
	return(1);
}
/* ========================================================================== */
int ArchEvtFilter(char actif) {
	Refiltre = actif;
	return(1);
}
/* ========================================================================== */
void ArchSetFilter(int voie, TypeFiltre *filtre) {
	if((voie >= 0) && (voie < VOIES_MAX)) FiltreVoie[voie] = filtre;
}
#ifdef CORRIGE_CROSSTALK
/* ========================================================================== */
static int ArchCrossTalk(TypeDonnee *x, int modx, TypeDonnee *y, int mody, int npt, float *ct, float *cor) {
	double  s1, sx, sxx, sy, sxy, syy, d, e;
	int i, ax, ay;
	
	s1 = sx = sxx = sy = sxy = syy = d = e = 0.0;      
	ax = 0;
	ay = 0;
	for(i=0; i<npt; i++) {
		ax++; if(ax == modx) { x++;  ax = 0; }
		ay++; if(ay == mody) { y++;  ay = 0; }
		s1  += 1;		
		sx  += ((double)*x); sy  += ((double)*y);
		sxx += ((double)*x) * ((double)*x);
		syy += ((double)*y) * ((double)*y);
		sxy += ((double)*x) * ((double)*y);
	}
	d = (s1 * sxx) - (sx * sx);   
    e = (s1 * syy) - (sy * sy);
    *ct = 0.0;
	*cor = 0.0;
	if(d > 0.0) *ct = (float)(((s1 * sxy) - (sx * sy) ) / d);   
	if((d * e) > 0.0) *cor = (float)(((s1 * sxy) - (sx * sy)) / sqrt(d * e));   
	
	return (1);
}
#endif
/* ========================================================================== */
int ArchEvtBack(int f, int *num_lu, off_t *of7) {
	off_t propose;

#ifdef ACCEPTE_EDW1
	if(VsnManip == 1) {
		if((*of7 = lseek(f,-ArchLngrEdw1,SEEK_CUR)) == -1) {
			printf(". Positionnement dans le fichier impossible (%s)\n",strerror(errno));
			return(0);
		}
		--ArchEvt.num;
	} else 
#endif
	if(VsnManip == 2) {
		propose = lseek(f,0,SEEK_CUR) - (2 * EvtDim);
		if(propose < 0) propose = 0;
		lseek(f,propose,SEEK_SET);
		ArchEvtSearch(f,99999999,num_lu,of7);
	}
	return(1);
}
/* ========================================================================== */
char ArchEvtRead(int f, int num, off_t *retour, char *explic) {
	int voie,vt,nb,nbfiltres,i,j,num_lu,bolo_lu;
	char rc;
	off_t pos;
	int bolo;
	double *demarrage;
	char entete[ENTETE_LNGR],c,*r;
	char guil;
	char log;
	char saute_entete_voie;

#ifdef CORRIGE_PATTERN
    int patterndim;
#endif
#ifdef CORRIGE_CROSSTALK
	int npoint,voiea,capt,voiecentre;
	int avant_evt,shift,shift_centre,shift_a, modulation, modulation_a;
	float ct,cor;
#endif
#ifdef REFAIT_FILTRAGE
	TypeFiltre *filtre;
	TypeDonneeFiltree val, entree, filtree;
	TypeCelluleFiltrante *cellule;
  	double *filtrees;
	int etage;
	TypeDonnee *brutes;
	float coef;
	int max,n,ip,k,recalcul;
#endif

	log = 0; // (num >= 37569);
	sprintf(explic,"pas documentee");
	bolo = 0; k = 0; filtree = 0.0; // GCC
#ifdef ACCEPTE_EDW1
	if(VsnManip == 1) {
		ArchEvt.num = num;
		ArchEvt.nbvoies = VoiesNb;
		for(vt=0; vt<ArchEvt.nbvoies; vt++) {
			voie = ArchEvt.voie[vt].num = vt;
			ArchEvt.voie[vt].nbfiltres = 0;
			ArchEvt.voie[vt].dim = VoieEvent[vt].lngr;
			if(ArchEvt.voie[vt].dim > VoieEvent[voie].brutes.max) {
				if(VoieEvent[voie].brutes.max) {
                    // printf("(%s) Liberation %s @%08X\n",__func__,VoieManip[voie].nom,(hexa)VoieEvent[voie].brutes.t.sval);
					if(VoieEvent[voie].brutes.t.sval) free(VoieEvent[voie].brutes.t.sval);
					if(gDonnee[voie]) { GraphDelete(gDonnee[voie]); gDonnee[voie] = 0; }
					VoieEvent[voie].brutes.max = 0;
				}
				VoieEvent[voie].brutes.t.sval = (TypeDonnee *)malloc(ArchEvt.voie[vt].dim * sizeof(TypeDonnee));
				if(!VoieEvent[voie].brutes.t.sval) {
					sprintf(explic,"VoieEvent[%d].brutes.t.sval[%d]: place memoire insuffisante",voie,ArchEvt.voie[vt].dim);
					return(-1);
				}
                // printf("(%s) Allocation %s @%08X\n",__func__,VoieManip[voie].nom,(hexa)VoieEvent[voie].brutes.t.sval);
				VoieEvent[voie].brutes.max = ArchEvt.voie[vt].dim;
			}
		}
		for(voie=0; voie<VoiesNb; voie++) {
			nb = read(f,VoieEvent[voie].brutes.t.sval,VoieEvent[voie].lngr * sizeof(TypeDonnee));
			if((nb / sizeof(TypeDonnee)) != VoieEvent[voie].lngr) {
				sprintf(explic,"%s:%d Erreur pendant la lecture du fichier: %s",__FILE__,__LINE__,strerror(errno));
				return(-1);
			}
			if(ByteSwappe) ByteSwappeShortArray((unsigned short *)VoieEvent[voie].brutes.t.sval,VoieEvent[voie].lngr);
		}

	} else 
#endif
	if(VsnManip == 2) {
		if(log) printf("====== Evenement#%d relu @ %08llX ======\n",num,lseek(f,0,SEEK_CUR));
		/*
		 * .......... Recherche du debut de l'evenement
		 */
		/* si retour != 0, rechercher l'entete telle que ArchEvt.num = num et stocker la position dans *retour */
		num_lu = -1;
		if(retour) {
			if((rc = (char)ArchEvtSearch(f,num,&num_lu,&pos)) == 0) {
				sprintf(explic,"Fin de fichier rencontree, sans marque de fin d'archivage");
				return(0);
			} else if(rc < 0) {
				if(rc == -2) *retour = -1;
				else if(rc == -3) *retour = lseek(f,0,SEEK_END) + 1;
				sprintf(explic,"Recherche de l'evenement %d en echec (ArchEvtSearch: %s)",num,strerror(errno));
				return(rc);
			}
			if(num_lu == num) *retour = pos;
			else /* if(num_lu == (num - 1)) */ *retour = lseek(f,0,SEEK_CUR);
		}
		if(num_lu != num) {
			do {
				pos = lseek(f,0,SEEK_CUR);
				if((rc = (char)read(f,entete,8)) == 0) {
					sprintf(explic,"Fin de fichier rencontree, sans marque de fin d'archivage");
					return(0);
				} else if(rc < 0) {
					sprintf(explic,"Marque de l'evenement %d absente (ArchEvtSearch: %s)",num,strerror(errno));
					return(rc);
				}
				entete[8] = '\0';
				do {
					if((rc = (char)read(f,&c,1)) == 0) {
						sprintf(explic,"Fin de fichier rencontree, sans marque de fin d'archivage");
						return(0);
					} else if(rc < 0) {
						sprintf(explic,"Marque de l'evenement %d en echec (ArchEvtSearch: %s)",num,strerror(errno));
						return(rc);
					}
				} while(c != '\n');
				if(!strncmp(entete,"* Arret ",8)) {
					sprintf(explic,"Texte '* Arret' rencontre @%lld",lseek(f,0,SEEK_CUR));
					return(0);
				} else if(!strncmp(entete,"* Evenement ",8)) break;
			} while(1);
		}
//		if(retour) *retour = pos;
		/*
		 * .......... Lecture de l'entete d'evenement
		 */
		memset(&ArchEvt,0,sizeof(TypeEvt));
		ArchEvt.nbvoies = 1;
		if((rc = (char)ArgFromPath(f,ArchHdrEvt,LIMITE_ENTETE))) {
			if(rc > 0) sprintf(explic,"Entete de l'evenement %d en echec (ArchEvtSearch: %s) ",num,strerror(errno));
			return(-9);
		}
		ArchEvt.nbvoies = ArchVoiesNb;
		if(log) ArgPrint("*",ArchHdrEvt);
	#ifdef CORRIGE_CROSSTALK
		log = 1;
		if(Decrosstalke) {		
			if(log) printf("-----------------------------------------------------\n");
			if(log) printf("Evenement numero %d  correlation et coeff. dir. (x/y) \n",(int)(ArchEvt.num-1));
			if(log) printf("-----------------------------------------------------\n");
		    for(voie=0; voie<VoiesNb; voie++)  if(VoieEvent[voie].brutes.max) {
                // printf("(%s) Liberation %s @%08X\n",__func__,VoieManip[voie].nom,(hexa)VoieEvent[voie].brutes.t.sval);
				if(VoieEvent[voie].brutes.t.sval) free(VoieEvent[voie].brutes.t.sval);
                VoieEvent[voie].brutes.t.sval = 0;
				if(gDonnee[voie]) { GraphDelete(gDonnee[voie]); gDonnee[voie] = 0; }
				VoieEvent[voie].brutes.max = 0;
			}
		}
	#endif
		if(Version < 7.0) {
			read(f,&bolo_lu,sizeof(int));
			ArchEvt.nbvoies = ModeleVoieNb;
		}
		for(vt=0; vt<ArchEvt.nbvoies; vt++) ArchEvt.voie[vt].dim = 0;
		/*
		 * .......... Lecture de toutes les voies associees
		 */
		for(vt=0; vt<ArchEvt.nbvoies; vt++) {
			off_t pos;
			if(Version > 6.9) {
				pos = lseek(f,0,SEEK_CUR);
				if((rc = (char)read(f,entete,6)) == 0) {
					ArchEvt.nbvoies = vt;
					lseek(f,pos,SEEK_SET);
					break;
				} else if(rc < 0) {
					sprintf(explic,"Marque de la voie #%d (evt %d) en echec (ArchEvtSearch: %s)",vt+1,ArchEvt.num,strerror(errno)); return(rc);
				}
				entete[6] = '\0';
				if(log) printf(". lu (a) @%08llX: %s\n",pos,entete);
				saute_entete_voie = 0;
				if(strncmp(entete,"* Voie",6)) {
					if(!strncmp(entete,"* Evenement ",6) || !strncmp(entete,"* Arret ",6)) {
						ArchEvt.nbvoies = vt;
						lseek(f,pos,SEEK_SET);
						break;
					}
					sprintf(explic,"Marque de voie #%d (evt %d) absente [%s...] (ArchEvtSearch: %s)",vt+1,ArchEvt.num,entete,strerror(errno));
					return(-8);
				}
				r = entete; guil = 0;
				do {
					if((rc = (char)read(f,r,1)) == 0) {
						sprintf(explic,"Fin de fichier rencontree, sans marque de fin d'archivage");
						return(0);
					} else if(rc < 0) {
						sprintf(explic,"Marque de voie #%d (evt %d) en defaut [%s...] (ArchEvtSearch: %s)",vt+1,ArchEvt.num,entete,strerror(errno));
						return(rc);
					}
					if(*r == '\n') break;
					if(*r == '\"') { if(!guil) guil = 1; else guil = 0; }
					else { if((*r != ' ') || guil) r++; }
				} while((r - entete) < 80);
				*r = '\0';
				memset(&ArchVoie,0,sizeof(TypeVoieArchivee));
				ArgFromPath(f,ArchHdrVoieEvt,LIMITE_ENTETE);
				if(log) ArgPrint("*",ArchHdrVoieEvt);
			#define NUMVOIE_ECRIT_PAR_SAMBA
			#ifdef NUMVOIE_ECRIT_PAR_SAMBA
				voie = ArchVoie.num;
				if(strcmp(entete,VoieManip[voie].nom)) {
					if(log) printf("Entete de voie #%d (evt %d) inattendue (prevue: %s, trouvee: %s)\n",vt+1,ArchEvt.num,VoieManip[voie].nom,entete);
					for(voie=0; voie<VoiesNb; voie++) if(!strcmp(VoieManip[voie].nom,entete)) break;
					if(voie >= VoiesNb) {
						if(log) printf("En fait, cette entete est carrement incorrecte..\n");
						sprintf(explic,"Entete de voie #%d (evt %d) inattendue (prevue: %s, trouvee: %s)",vt+1,ArchEvt.num,VoieManip[voie].nom,entete);
						return(-7);
					}
				}
			#else
				for(voie=0; voie<VoiesNb; voie++) if(!strcmp(VoieManip[voie].nom,entete)) break;
				if(voie >= VoiesNb) {
					if(log) printf("Entete de voie #%d (evt %d) incorrecte dans l'evt #%d: '%s'\n",vt+1,ArchEvt.num,entete);
				}
			#endif
				ArchVoie.num = voie;
				if(log) printf("Evt #%d: entete #%d = voie #%2d (%s) @%08X[%d]\n",ArchEvt.num,vt,voie,VoieManip[voie].nom,(unsigned int)(VoieEvent[voie].brutes.t.sval),ArchVoie.dim);
				memcpy(&(ArchEvt.voie[vt]),&ArchVoie,sizeof(TypeVoieArchivee));
				if(ArchVoie.dim <= 0) continue;
				bolo = VoieManip[voie].det;										
				demarrage = ArchEvt.voie[vt].demarrage;
				if(ArchEvt.voie[vt].nbfiltres > ArchEvt.voie[vt].demdim) {
					if(ArchEvt.voie[vt].demdim) { if(demarrage) free(demarrage); }
					ArchEvt.voie[vt].demarrage = (double *)malloc(ArchEvt.voie[vt].nbfiltres * sizeof(double));
					if(!ArchEvt.voie[vt].demarrage) {
						sprintf(explic,"ArchEvt[%d].voie[%d].demarrage[%d]: place memoire insuffisante",
							ArchEvt.num,voie,ArchEvt.voie[vt].nbfiltres);
						ArchEvt.voie[vt].demdim = 0;
					} else ArchEvt.voie[vt].demdim = ArchEvt.voie[vt].nbfiltres;
				} else ArchEvt.voie[vt].demarrage = demarrage;
	#define BIDOUILLE_TEMPORAIRE
	#ifdef BIDOUILLE_TEMPORAIRE
				if(log) pos = lseek(f,0,SEEK_CUR);
				if((rc = (char)read(f,entete,6)) == 0) {
					sprintf(explic,"Fin de fichier rencontree, sans marque de fin d'archivage");
					return(0);
				} else if(rc < 0) {
					sprintf(explic,"Entete supp. de voie #%d (evt %d) en erreur (ArchEvtSearch: %s)",vt+1,ArchEvt.num,strerror(errno));
					return(rc);
				}
				entete[6] = '\0';
				if(log) printf(". lu (b) @%08llX: %s\n",pos,entete);
				lseek(f,-6,SEEK_CUR);
				if(!strncmp(entete,"* Voie",6) /* || !strncmp(entete,"####",4) */ ) {
					ArchEvt.voie[vt].dim = ArchVoie.dim = 0;
					ArchEvt.voie[vt].nbfiltres = 0;
	//				printf("(ArchEvtRead) ArchEvt.voie[%d].dim = %d\n",vt,ArchEvt.voie[vt].dim);
					if(log) printf(". Les donnees de la voie #%d ne sont pas lues\n",vt);
					continue;
				}
	#endif
	//			printf("(ArchEvtRead) ArchEvt.voie[%d].dim = %d\n",vt,ArchEvt.voie[vt].dim);
				if(ArchEvt.voie[vt].nbfiltres) {
					if(ArchEvt.voie[vt].demarrage) {
						for(i=0; i<ArchEvt.voie[vt].nbfiltres; i++) 
							read(f,&(ArchEvt.voie[vt].demarrage[i]),sizeof(double));
						if(ByteSwappe) ByteSwappeLongArray((int64 *)(ArchEvt.voie[vt].demarrage),ArchVoie.nbfiltres);
					} else {
						lseek(f,ArchEvt.voie[vt].nbfiltres * sizeof(double),SEEK_CUR);
						ArchEvt.voie[vt].nbfiltres = 0;
					}
				} //?? else lseek(f,ArchEvt.voie[vt].nbfiltres * sizeof(double),SEEK_CUR);
			} else {
				int dim,nbfiltres,recalcul;
				read(f,&voie,sizeof(int));
				read(f,&dim,sizeof(int));
				read(f,&nbfiltres,sizeof(int));
				read(f,&recalcul,sizeof(int));
				if(ByteSwappe) {
					ByteSwappeInt((unsigned int *)(&voie));
					ByteSwappeInt((unsigned int *)(&dim));
					ByteSwappeInt((unsigned int *)(&nbfiltres));
					ByteSwappeInt((unsigned int *)(&recalcul));
				}
				ArchVoie.num = voie;
				if(nbfiltres) lseek(f,nbfiltres * sizeof(double),SEEK_CUR);
				// ArchVoie.dim = dim;
				ArchEvt.voie[vt].dim = ArchVoie.dim;
			}
			VoieEvent[voie].horloge = ArchVoie.horloge;
			VoieEvent[voie].lngr = ArchVoie.dim;
			VoieEvent[voie].avant_evt = ArchVoie.avant_evt;
			if(ArchVoie.dim > VoieEvent[voie].brutes.max) {
				if(VoieEvent[voie].brutes.max) {
                    // printf("(%s) Liberation %s @%08X\n",__func__,VoieManip[voie].nom,(hexa)VoieEvent[voie].brutes.t.sval);
					if(VoieEvent[voie].brutes.t.sval) free(VoieEvent[voie].brutes.t.sval);
					/* probleme ch... a regler
					GraphDelete(gDonnee[voie]); gDonnee[voie] = 0; */
					VoieEvent[voie].brutes.max = 0;
				}
				VoieEvent[voie].brutes.t.sval = (TypeDonnee *)malloc(ArchVoie.dim * sizeof(TypeDonnee));
				if(!VoieEvent[voie].brutes.t.sval) {
					sprintf(explic,"VoieEvent[%d].brutes.t.sval[%d]: place memoire insuffisante",
						voie,ArchVoie.dim);
					return(-1);
				}
                // printf("(%s) Allocation %s @%08X\n",__func__,VoieManip[voie].nom,(hexa)VoieEvent[voie].brutes.t.sval);
				VoieEvent[voie].brutes.max = ArchVoie.dim;
			}
			if(log) printf("      Entete #%d: voie %d (%s) @%08X, longueur %d @%g ms\n",vt,voie,
				   VoieManip[voie].nom,(hexa)VoieEvent[voie].brutes.t.sval,
				   VoieEvent[voie].brutes.max,VoieEvent[voie].horloge);
			nb = read(f,VoieEvent[voie].brutes.t.sval,ArchVoie.dim * sizeof(TypeDonnee));
			if((nb / sizeof(TypeDonnee)) != ArchVoie.dim) {
				sprintf(explic,"%s:%d Erreur pendant la lecture du fichier: %s",__FILE__,__LINE__,strerror(errno));
				return(nb? -1: 0);
			}
			if(ByteSwappe) ByteSwappeShortArray((unsigned short *)VoieEvent[voie].brutes.t.sval,ArchVoie.dim);
			if(log) {
				int i;
				printf("Donnees voie %s:",VoieManip[voie].nom);
				for(i=0; i<ArchVoie.dim; i++) {
					if(!(i % 10)) printf("\n%4d:",i);
					printf(" %7d",VoieEvent[voie].brutes.t.sval[i]);
				}
				printf("\n");
			}
			// printf("Evt #%d: entete #%d = voie #%d (%s) @%08X\n",ArchEvt.num,vt,voie,VoieManip[voie].nom,VoieEvent[voie].brutes.t.sval);

		#ifdef CORRIGE_PATTERN
			/* soustraction du pattern moyen
               ----------------------------- */
			if((Depatterne == 1) && (Bolo[bolo].lu != BN_AUXILIAIRE) && ((VoieManip[voie].type == VoieCentre) || (VoieManip[voie].type == VoieGarde))) {
				patterndim = Bolo[VoieManip[voie].det].d2;  // a recuperer sur l'entete
				if(log) printf("+ Demotification sur une periode de %d echantillons\n",patterndim);
				if(!VoieEvent[voie].pattern.val) VoieEvent[voie].pattern.val = (float *)malloc(patterndim * sizeof(float));			
				if(!VoieEvent[voie].pattern.val) {
					sprintf(explic,"VoieEvent[%d].brutes.t.sval[%d]: place memoire insuffisante",voie,ArchVoie.dim);
					return(-1);
				}
				for (j=0; j<patterndim; j++) VoieEvent[voie].pattern.val[j] = 0.0;
				j = 0;
				//	VoieEvent[voie].pattern.periodes = (int)(ArchVoie.dim / patterndim);
				// La on utilisait tous les points au lieu du pre-trigger!!!
				// Je garde en bonus (en dur pour l'instant) un facteur 0.9 pour ne pas mordre sur le debut de l'evt
				VoieEvent[voie].pattern.periodes = (int)(ArchVoie.avant_evt * 0.9 / patterndim);	
				// printf("(ArchEvtRead) %d %d %d\n",(int)VoieEvent[voie].pattern.periodes,(int)ArchVoie.dim,(int)patterndim);
				for(i=0; i<patterndim*VoieEvent[voie].pattern.periodes; i++) {			
					if(j == patterndim) j = 0;
					VoieEvent[voie].pattern.val[j] += VoieEvent[voie].brutes.t.sval[i];
					j++;
				}
				for(j=0; j<patterndim; j++) {
					VoieEvent[voie].pattern.val[j] /= (float)VoieEvent[voie].pattern.periodes;
				}
				j=0;	
				for(i=0; i<ArchVoie.dim; i++) {					
					if (j == patterndim) j = 0;
					 VoieEvent[voie].brutes.t.sval[i] -= (float)VoieEvent[voie].pattern.val[j];
					j++;
				}
            }
			else if(log) printf("* Pas de demotification\n");
		#endif

		#ifdef CORRIGE_CROSSTALK
			/* cross-talks
               ----------- */
			if((Decrosstalke == 1)  && (Bolo[bolo].lu != BN_AUXILIAIRE)) /* crosstalk centre-garde */ {  
				if(log) printf("+ Correction de diaphonie\n");
				if(VoieManip[voie].type == VoieGarde) {
					for(capt=0; capt<Bolo[bolo].nbcapt; capt++) /* recherche voie centre correspondant a la voie garde */ {
						if (VoieManip[Bolo[bolo].voie[capt]].type == VoieCentre)  voiecentre = Bolo[bolo].voie[capt];
					}	
					if (VoieEvent[voie].avant_evt > VoieEvent[voiecentre].avant_evt) /* gestion pour si tailles differentes des voies */ { 
						avant_evt = VoieEvent[voiecentre].avant_evt;				
						shift = VoieEvent[voie].avant_evt - VoieEvent[voiecentre].avant_evt;
						shift_centre = 0;			
					} else {
						avant_evt = VoieEvent[voie].avant_evt;				
						shift_centre = VoieEvent[voiecentre].avant_evt - VoieEvent[voie].avant_evt;
						shift = 0;						
					}
					npoint = (int)(0.9 * avant_evt) ; // cross talk calcule sur 90% du plus petit pre-trigger;
					modulation = 1;
					ArchCrossTalk(VoieEvent[voie].brutes.t.sval+shift,modulation,VoieEvent[voiecentre].brutes.t.sval+shift_centre,modulation,npoint,&ct,&crosstalk[bolo][ArchEvt.num-1]);	
					// printf("evt=%d, cross-talk= %f BoloGeres = %d\n",npoint,crosstalk[bolo][ArchEvt.num-1],BoloGeres);
					// application soustraction voie avec cross talk de couplage electronique centre-garde pour premier runs 
					/* for (i=0; i<ArchVoie.dim; i++) {										
						*(VoieEvent[voiecentre].brutes.t.sval+i) = *(VoieEvent[voiecentre].brutes.t.sval+i) - *(VoieEvent[voie].brutes.t.sval+i)*crosstalk[bolo][ArchEvt.num-1];
					} 		
					for (i=0; i<ArchVoie.dim; i++) {													
						*(VoieEvent[voie].brutes.t.sval+i) = -*(VoieEvent[voiecentre].brutes.t.sval+i)/crosstalk[bolo][ArchEvt.num-1];
					} */
				}
				
			   if(log) {
					if (VoieManip[voie].type == VoieChaleur) modulation = Bolo[VoieManip[voie].det].modulation; else modulation = 1;
					printf("%15s","");
					for (voiea=0; voiea <= voie; voiea++) if(VoieEvent[voiea].brutes.max) printf("%15s",VoieManip[voiea].nom);
					printf("\n%15s",VoieManip[voie].nom);
					for (voiea=0; voiea <= voie; voiea++) { 		    
						// if ((VoieEvent[voiea].brutes.t.sval)&&(VoieManip[voiea].type != VoieChaleur)&&(VoieManip[voie].type !=VoieChaleur)) {
						if (VoieEvent[voiea].brutes.max) {
							if(VoieManip[voiea].type == VoieChaleur) modulation_a = Bolo[VoieManip[voiea].det].modulation; else modulation_a = 1;			
							if(modulation*VoieEvent[voie].avant_evt > (modulation_a * VoieEvent[voiea].avant_evt)) /* gestion pour si tailles differentes des voies */ {
								avant_evt = modulation_a * VoieEvent[voiea].avant_evt;				
								shift     = modulation * VoieEvent[voie].avant_evt-modulation_a * VoieEvent[voiea].avant_evt;
								shift_a   = 0;
							} else {
								avant_evt = modulation * VoieEvent[voie].avant_evt;				
								shift_a   = (modulation_a * VoieEvent[voiea].avant_evt) - (modulation * VoieEvent[voie].avant_evt);
								shift	  = 0;						
							}	
							npoint = (int)(0.9*avant_evt) ;
							ArchCrossTalk(VoieEvent[voie].brutes.t.sval+shift,modulation,VoieEvent[voiea].brutes.t.sval+shift_a,modulation_a,npoint,&ct,&cor);
							printf("%7.2f %7.2f",cor,ct);
						}			    
					}
					printf("\n");
				}
			}
			else if(log) printf("* Pas de correction de diaphonie\n");
		#endif

		#ifdef REFAIT_FILTRAGE
			/* re-filtrage 
              ----------- */
			if(Refiltre && (Bolo[bolo].lu != BN_AUXILIAIRE)) { 	
				if(log) printf("+ Filtrage\n");
				brutes = VoieEvent[voie].brutes.t.sval;
				if((filtre = FiltreVoie[voie]) == 0) /* utilise le filtre Samba */ {
					filtre = VoieEvent[voie].filtre;
					nbfiltres = (int)ArchEvt.voie[vt].nbfiltres;
					recalcul = ArchEvt.voie[vt].recalcul;
				} else /* utilise un filtre, mais pas celui de Samba */{
					// Avec ca ca semble marcher...		nbfiltres = (filtre->etage[0]).nbinv;
					nbfiltres = filtre->nbmax;
					if(ArchEvt.voie[vt].demdim < nbfiltres) {
						if(ArchEvt.voie[vt].demdim) free(ArchEvt.voie[vt].demarrage);
						ArchEvt.voie[vt].demarrage = (double *)malloc(nbfiltres * sizeof(double));
						if(!ArchEvt.voie[vt].demarrage) {
							sprintf(explic,"ArchEvt.voie[%d].demarrage[%d]: place memoire insuffisante",
									   voie,ArchEvt.voie[vt].nbfiltres);
							ArchEvt.voie[vt].demdim = 0;
							continue;
						}
						ArchEvt.voie[vt].demdim = nbfiltres;
						ArchEvt.voie[vt].nbfiltres = nbfiltres;
					}
					for (ip=0; ip<nbfiltres-1; ip++) ArchEvt.voie[vt].demarrage[ip] = 0;
					recalcul = 0;
				}
				if(!ArchEvt.voie[vt].demarrage) continue;
				filtrees = ArchEvt.voie[vt].demarrage;
				max = nbfiltres;
				n=max; 
				for (ip=recalcul; ip<ArchVoie.dim-max; ip++) {
					entree=(TypeDonneeFiltree)(brutes[ip+max]); 
					for(etage=0; etage<filtre->nbetg; etage++) {
						cellule = &(filtre->etage[etage]);
			
						// if(cellule->nbdir > 0) filtree = (TypeDonneeFiltree)cellule->direct[0] * entree;
						if(cellule->nbdir > 0) coef = (TypeDonneeFiltree)(cellule->direct[cellule->nbdir-1]);	   
						// coef = (TypeDonneeFiltree)cellule->direct[0];	
						if(cellule->nbdir > 0) filtree = coef * entree;
						else filtree = 0.0;	   
						//	if(ip < recalcul+3) printf("   (%d) %8.3f x %9.3f = %11.3f -> %11.3f\n",etage,cellule->direct[0],entree,filtree,filtree);
						if(etage == 0) for(i=cellule->nbdir-2,k=ip+max; i>=0; i--) { /* X = donnees brutes */
							k -= 1;
							coef = (TypeDonneeFiltree)(cellule->direct[i]);
							val = (TypeDonneeFiltree)(brutes[k]);
							filtree += coef * val;
							//	if(ip < recalcul+3) printf(" + %8.3f x (%d %d )%9.3f = %11.3f -> %11.3f\n",cellule->direct[i],k,ip,(float)(brutes[k]),(TypeDonneeFiltree)cellule->direct[i] * (TypeDonneeFiltree)(brutes[k]),filtree);
						} else for(i=cellule->nbdir-2,k=n; i>=0; i--) { /* X = resultat cellule precedente */
							k -= 1;
							if(k < 0) k = max - 1;	
							// filtree += (TypeDonneeFiltree)cellule->direct[i] * filtree[etage-1][k];	
							coef = (TypeDonneeFiltree)cellule->direct[i];
							filtree += coef * filtrees[k];
							//	if(ip < recalcul+3) printf(" + %8.3f x %9.3f = %11.3f -> %11.3f\n",cellule->direct[i],(float)(filtrees[k]),(TypeDonneeFiltree)cellule->direct[i] * filtrees[k],filtree);
						}

						for(i=cellule->nbinv-1,k=n; i>=0; i--) {
							k -= 1;
							if(k < 0) k = max - 1;
							coef = (TypeDonneeFiltree)cellule->inverse[i];
							val = (TypeDonneeFiltree)(filtrees[k]);	
							filtree -= coef * val; 	
							//	if(ip < recalcul+3)printf(" - %8.3f x (%d %d) %9.3f = %11.3f -> %11.3f\n",cellule->inverse[i],k,n,filtrees[k],-(TypeDonneeFiltree)cellule->inverse[i] * filtrees[k],filtree); 
						}	
						// filtrees[n] = (TypeDonneeFiltree)filtree;
					}
					brutes[ip] = (TypeDonnee)(filtrees[k]);
					filtrees[k] = (TypeDonneeFiltree)filtree;
					n++;
					if(n > max) n = 1;
				}
				for (i=0;i<max;i++) { // derniers points du filtrage
					k++;
					if(k>=max) k=0;
					brutes[ArchVoie.dim-max+i] = (TypeDonnee)(filtrees[k]);
					//	printf("%d %d %d %d %d %d %d \n",ArchVoie.dim-max+i,ArchVoie.dim,max,i,(int)(filtrees[k]),(int)(VoieEvent[voie].brutes.t.sval[ArchVoie.dim-max+i]),k);
				}   
				for (i=0;i<recalcul;i++) { // premier points mis a zero si recalcul!=0
					brutes[i] = 0;
				} 		
			} 
			else if(log) printf("* Pas de filtrage\n");
		#endif
		}
		if(Version < 7.0) read(f,&bolo_lu,sizeof(int)); /* on doit retrouver -1 */
	}
	return(1);
}
