#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#define FILTRES_C
#include <filtres.h>

#include "decode.h"
#include "dateheure.h"
#define OPIUM_INDIFFERENT
#include "opium.h"

#define NOM_FILTRES_IMPOSES
//#define FILTRES_MODELE_MIG

char  FiltreNomDirect[MAXFILTRECOEF][8],FiltreNomInverse[MAXFILTRECOEF][8];
static char *FiltreChoixType[] = { "Standard", "Manuel", "\0" };
static float FiltreEchant;
static int   FiltreDegreMax;

#ifdef OPIUM_H
	Menu mFiltres;

	static Panel pFiltreDef,pFiltreCoef;
	static Term tFiltres;
	static TypeMenuItem iFiltres[] = {
		{ "Creer",            MNU_FONCTION FiltresAjout      },
		{ "Modifier",         MNU_FONCTION FiltresModif      },
		{ "Personnaliser",    MNU_FONCTION FiltresPerso      },
		{ "Lister",           MNU_FONCTION FiltresDump       },
		{ "Supprimer",        MNU_FONCTION FiltresRetrait    },
		{ "Recuperer",        MNU_FONCTION FiltresRestore    },
		{ "Sauver",           MNU_FONCTION FiltresSauve      },
		{ "Quitter",          MNU_RETOUR                     },
		MNU_END
	};
#endif

int ellfcalc(char modele, char type, short degre, float dbr, int maxi,
			 float omega1, float omega2, float omega3, int *ncoef, double *dir, double *inv);

#ifdef FILTRES_MODELE_MIG
#include <rfftw.h>
/* ==========================================================================
static void ImprimeFiltre(TypeFiltre *coef) {
	int k,l,n,m;

	printf("  . %d etage%s\n",(int)coef->nbetg,(coef->nbetg > 1)? "s": "");
	for(k=0; k<coef->nbetg; k++) {
		printf("  . etage %d: %d directs, %d inverses\n",k,(int)coef->etage[k].nbdir,(int)coef->etage[k].nbinv);
		m = (coef->etage[k].nbdir > coef->etage[k].nbinv)? coef->etage[k].nbdir: coef->etage[k].nbinv;
		printf("      a%-2d = %16.9e\n",0,coef->etage[k].direct[0]);
		for(l=0; l<m; l++) {
			n = l + 1;
			if(n < (coef->etage[k].nbdir)) printf("      a%-2d = %16.9e",n,coef->etage[k].direct[n]);
			else printf("                             ");
			if(l < (coef->etage[k].nbinv)) printf("      b%-2d = %16.9e\n",n,coef->etage[k].inverse[l]);
			else printf("\n");
		}
	}
} */
/* ========================================================================== */
static int FiltreCalculeMiG(float omega, char modele, char type, short degre, TypeFiltre *coef) {
	int etg,k;
	/* Butterworth */
	double omepi;
	double alpha_k,a_k0;
	char impair;
	/* Direct */
	int coupure; int i;
	char raison[80];
	fftw_real *ptr_fft;

	coef->nbetg = 0;
	if((omega <= 0.0) || (omega > 1.0)) {
		sprintf(FiltreErreur,"Omega invalide"); return(0);
	}
//	printf("- Construction d'un filtre %s %s d'ordre %d\n",
//		   FiltreModeles[(int)modele],FiltreTypes[(int)type],(int)degre);
	omepi = PI * (double)omega;
	switch(modele) {
	  case FILTRE_MIG:
		impair = (degre % 2);
		if(MAXFILTRECOEF < 3) {
			sprintf(FiltreErreur,"Pas assez coefficients prevus (maxi: %d)",MAXFILTRECOEF);
			return(0);
		}
		coef->nbetg = degre / 2;
		if(coef->nbetg >= MAXFILTREETG) {
			sprintf(FiltreErreur,"Trop d'etages (maxi: %d)",MAXFILTREETG);
			coef->nbetg = 0;
			return(0);
		}
		switch(type) {
		  case FILTRE_PBAS:
			etg = 0;
			if(impair) {
				coef->nbetg += 1;
				coef->etage[etg].nbdir = 2;
				coef->etage[etg].direct[0] = omepi / (omepi - 1.0);   /* 1    .. */
				coef->etage[etg].direct[1] = omepi / (omepi - 1.0);   /* + Z  .. */
				coef->etage[etg].nbinv = 1;
				coef->etage[etg].inverse[0] = (omepi + 1.0) / (omepi - 1.0);
				etg++;
			}
			for(k=1 ; etg<coef->nbetg; etg++,k++) {
				if(impair) alpha_k = 2.0 * cos(PI * (double)k / (double)degre);
				else alpha_k = 2.0 * cos(PI * ((2.0 * (double)k) - 1.0) / (2.0 * (double)degre));
				a_k0 = 1.0 / (1.0 + (omepi * alpha_k) + (omepi * omepi));
				coef->etage[etg].nbdir = 3;
				coef->etage[etg].direct[0] = a_k0 * (omepi * omepi);               /* 1    .. */
				coef->etage[etg].direct[1] = 2.0 * a_k0 * (omepi * omepi);         /* +2 Z .. */
				coef->etage[etg].direct[2] = a_k0 * (omepi * omepi);               /* +Z2     */
				coef->etage[etg].nbinv = 2;
				coef->etage[etg].inverse[0] = 2.0 * a_k0 * (omepi * omepi - 1.0);
				coef->etage[etg].inverse[1] = a_k0 * (1.0 - (omepi * alpha_k) + (omepi * omepi));
			}
			break;
		  case FILTRE_PHAUT:
			etg = 0;
			if(impair) {
				coef->nbetg += 1;
				coef->etage[etg].nbdir = 2;
				coef->etage[etg].direct[0] = 1.0 / (1.0 - omepi);    /* 1    .. */
				coef->etage[etg].direct[1] = -1.0 / (1.0 - omepi);   /* - Z  .. */
				coef->etage[etg].nbinv = 1;
				coef->etage[etg].inverse[0] = (omepi + 1.0) / (omepi - 1.0);
				etg++;
			}
			for(k=1 ; etg<coef->nbetg; etg++,k++) {
				if(impair) alpha_k = 2.0 * cos(PI * (double)k / (double)degre);
				else alpha_k = 2.0 * cos(PI * ((2.0 * (double)k) - 1.0) / (2.0 * (double)degre));
				a_k0 = 1.0 / (1.0 + (omepi * alpha_k) + (omepi * omepi));
				coef->etage[etg].nbdir = 3;
				coef->etage[etg].direct[0] = a_k0;               /* 1    .. */
				coef->etage[etg].direct[1] = -2.0 * a_k0;        /* -2 Z .. */
				coef->etage[etg].direct[2] = a_k0;               /* +Z2     */
				coef->etage[etg].nbinv = 2;
				coef->etage[etg].inverse[0] = 2.0 * a_k0 * (omepi * omepi - 1.0);
				coef->etage[etg].inverse[1] = a_k0 * (1.0 - (omepi * alpha_k) + (omepi * omepi));
			}
			break;
		  default:
			sprintf(FiltreErreur,"Type de filtre invalide (%d)",type);
			return(0);
		}
		break;
	  case FILTRE_DIR:
		coef->nbetg = 1;
		if(coef->nbetg >= MAXFILTREETG) {
			sprintf(FiltreErreur,"Trop d'etages (maxi: %d)",MAXFILTREETG);
			coef->nbetg = 0;
			return(0);
		}
		if(degre <= MAXFILTRECOEF) {
			coef->etage[0].nbdir = degre;
			coef->etage[0].nbinv = 0;
		} else { sprintf(FiltreErreur,"Degre %d trop eleve (maxi: %d)",degre,MAXFILTRECOEF); return(0); }
		if(CalculeFFTplan(degre,raison)) {
			double f,g;
			coupure = (int)(omega * (float)degre);
			ptr_fft = CalcFftFreq;
			switch(type) {
			  case FILTRE_PBAS:
				for(i=0; i<degre; i++)
					*ptr_fft++ = (fftw_real)(1.0 / (1.0 + ((float)(i + 1) / (float)coupure)));
				break;
			  case FILTRE_PHAUT:
//1				for(i=0; i<coupure; i++) *ptr_fft++ = (fftw_real)0.0;
//1				for( ; i<degre; i++) *ptr_fft++ = (fftw_real)1.0;
//2				for(i=0; i<degre; i++)
//2					*ptr_fft++ = (fftw_real)(1.0 / (1.0 + ((float)coupure / (float)(i + 1))));
//3				for(i=0; i<degre/2; i++) {
//3					CalcFftFreq[i] = (fftw_real)(1.0 / (1.0 + ((float)coupure / (float)(i + 1))));
//3					CalcFftFreq[degre - i - 1] = CalcFftFreq[i];
//3				}
//4				coupure = (int)(omega * (float)(degre / 2));
//4				for(i=0; i<degre/2; i++) {
//4					f = (float)coupure / (float)(i + 1);
//4					g = exp(6.0 * log((double)f));                 /* (OmegaC/Omega)**(2*n) avec n=3 */
//4					CalcFftFreq[i] = (fftw_real)(1.0 / (1.0 + g));
//4					CalcFftFreq[degre - i - 1] = CalcFftFreq[i];
//4				}
				for(i=0; i<degre; i++) {
					f = (float)coupure / (float)(i + 1);
					g = exp(8.0 * log((double)f));                 /* (OmegaC/Omega)**(2*n) avec n=4 */
					CalcFftFreq[i] = (fftw_real)(1.0 / (1.0 + g));
				}
				break;
			  default:
				sprintf(FiltreErreur,"Type de filtre invalide (%d)",type);
				return(0);
			}
			rfftw_one(CalcPlan,CalcFftFreq,CalcFftTemps);
			ptr_fft = CalcFftTemps;
			for(i=0; i<degre; i++) coef->etage[0].direct[i] = (double)*ptr_fft++ / (double)degre;
		} else { sprintf(FiltreErreur,raison); return(0); }
		break;
	  default:
		sprintf(FiltreErreur,"Modele de filtre invalide (%d)",modele);
		return(0);
	}
	coef->nbmax = 0;
	for(k=0; k<coef->nbetg; k++) {
		if((coef->etage[k].nbdir - 1) > coef->nbmax) coef->nbmax = coef->etage[k].nbdir - 1;
		if(coef->etage[k].nbinv > coef->nbmax) coef->nbmax = coef->etage[k].nbinv;
	}
//	ImprimeFiltre(coef);
	return(1);
}
#endif
/* ========================================================================== */
int FiltreCalculeStd(float omega1, float omega2, char modele, char type, short degre, TypeFiltre *coef) {
	int nbetg,ncoef,j;
	float omega3,dbr;
	double dir[MAXFILTRECOEF],inv[MAXFILTRECOEF];
	char log;

	log = 0;
	coef->nbetg = 0;
	if(modele >= FILTRE_MIG) {
#ifdef FILTRES_MODELE_MIG
		return(FiltreCalculeMiG(omega1,modele,type,degre,coef));
#else
		sprintf(FiltreErreur,"L'implementation des filtres MiG est supprimee");
		return(0);
#endif
	}
#ifdef CODE_WARRIOR_VSN
	sprintf(FiltreErreur,"Calcul des filtres pas encore implemente sous CodeWarrior");
	return(0);
#else
	if((type < FILTRE_PBAS) || (type > FILTRE_COUPEB)) {
		sprintf(FiltreErreur,"Type de filtre invalide: #%d",type); return(0);
	}
	nbetg = degre / 2; if((degre % 2)) nbetg++;
	if((type == FILTRE_PASSEB) || (type == FILTRE_COUPEB)) nbetg = (nbetg * 2) + 1; // pas tres sur...
	if(nbetg >= MAXFILTREETG) {
		sprintf(FiltreErreur,"Degre de filtrage %d dementiel (%d etages maxi)",degre,MAXFILTREETG); return(0);
	}
	// printf("(%s) O1 = %g\n",__func__,omega1);
	if((omega1 <= 0.0) || (omega1 > 1.0)) {
		sprintf(FiltreErreur,"Omega1 invalide: %g",omega1); return(0);
	}

	if(log) {
		printf("    > Construction d'un filtre %s %s d'ordre %d %s Omega ",
			FiltreModeles[(int)modele],FiltreTypes[(int)type],(int)degre,(type == FILTRE_PASSEB)?"passant":"coupant");
		if(type == FILTRE_PBAS) printf("> %g\n",omega1);
		else if(type == FILTRE_PHAUT) printf("< %g\n",omega1);
		else printf("dans [%g %g]\n",omega1,omega2); // FILTRE_PASSEB (passant) ou FILTRE_COUPEB (coupant)
	}

	if((modele >= FILTRE_BUT) && (modele <= FILTRE_ELL)) {
		omega3 = -40.0; dbr = 0.5; ncoef = 123;
		if(log > 1) {
			printf("ellfcalc(modele=%d, type=%d, degre=%d, dbr=%g, (int)MAXFILTRECOEF=%d,\n omega1=%g, omega2=%g, omega3=%g, &ncoef=%08llX, dir=%08llX, inv=%08llX)\n",
				modele, type, degre, dbr, (int)MAXFILTRECOEF, omega1, omega2, omega3, (IntAdrs)&ncoef, (IntAdrs)dir, (IntAdrs)inv);
		}
		if(!ellfcalc(modele,type,degre,dbr,(int)MAXFILTRECOEF,omega1,omega2,omega3,&ncoef,dir,inv)) {
			sprintf(FiltreErreur,"Calcul des coefficients impossible (degre %d, maxi: %d)",ncoef+1,MAXFILTRECOEF);
			return(0);
		}
		coef->calcule = 1;
		coef->nbetg = 1;
		coef->etage[0].nbdir = (char)ncoef + 1;
		coef->etage[0].nbinv = (char)ncoef;
		coef->nbmax = (char)ncoef;
		for(j=0; j<coef->etage[0].nbdir; j++) coef->etage[0].direct[j] = (TypeDonneeFiltree)dir[j];
		for(j=0; j<coef->etage[0].nbinv; j++) coef->etage[0].inverse[j] = (TypeDonneeFiltree)inv[j];
	} else {
		sprintf(FiltreErreur,"Modele de filtre invalide (%d)",modele); return(0);
	}

//	ImprimeFiltre(coef);
	return(1);
#endif
}
/* ========================================================================== */
int FiltresLit(char *nom, char log) {
	FILE *fichier;
	char ligne[10000],*r,*item,*u,*unite;
	char directs; double coef;
	char nbcoef,nbmax; char num,etage;
	char i; size_t l;

	num = etage = 0; /* gcc */
	strncpy(FiltreFichier,nom,MAXFILENAME);
	if(log) printf("= Lecture des filtres                   dans '%s'\n",nom);
	if(!(fichier = fopen(nom,"r"))) {
		if(log) printf("  ! Fichier des filtres illisible: '%s' (%s)\n",nom,strerror(errno));
		return(0);
	}
	FiltreNb = 0;
	while(LigneSuivante(ligne,10000,fichier)) {
		r = ligne;
		if((*r == '#') || (*r == '\n')) continue;
		if(*r == '=') {
			if(FiltreNb >= FiltreMax) {
				if(log) printf("  ! Pas plus de %d filtres. Derniers filtres non relus\n",FiltreMax);
				break;
			}
			r++;
			num = (char)FiltreNb;
			strncpy(FiltreGeneral[num].nom,ItemJusquA(':',&r),MAXFILTRENOM);
			/* filtre a priori 'manuel' */
			FiltreGeneral[num].modele = -1;
			FiltreGeneral[num].unite1 = FILTRE_REL;
			FiltreGeneral[num].unite2 = FILTRE_REL;
			FiltreGeneral[num].coef.calcule = 0;
			FiltreGeneral[num].coef.nbmax = 0;
			FiltreGeneral[num].coef.nbetg = 0; etage = 0;
			FiltreGeneral[num].coef.etage[etage].nbdir = FiltreGeneral[num].coef.etage[etage].nbinv = 0;
			item = ItemJusquA('\n',&r);
			strncpy(FiltreGeneral[num].commentaire,item,80);
//			printf("(FiltresLit) Filtre #%d: '%s'\n",num,FiltreGeneral[num].nom);
			FiltreNb++;
		} else {
			item = ItemJusquA('=',&r);
			if(*item == '\0') {
				item = ItemJusquA('/',&r); l = strlen(item);
				for(i=0; i<FILTRE_NBMODELES; i++) if(!strncmp(item,FiltreModeles[i],l)) break;
				if(i < FILTRE_NBMODELES) FiltreGeneral[num].modele = i;
				else {
					if(log) printf("  ! Modele de filtre non traite (%s)\n",item);
					FiltreGeneral[num].modele = -1;
					continue;
				}
//				printf("(FiltresLit) Le filtre %s est du modele #%d\n",FiltreGeneral[num].nom,i);
				item = ItemJusquA('*',&r); l = strlen(item);
				for(i=0; i<FILTRE_NBTYPES; i++) if(!strncmp(item,FiltreTypes[i],l)) break;
				if(i < FILTRE_NBTYPES) FiltreGeneral[num].type = i;
				else {
					if(log) printf("  ! Type de filtre non traite (%s)\n",item);
					FiltreGeneral[num].modele = -1;
					continue;
				}
				item = ItemJusquA('@',&r);
				if(*item) sscanf(item,"%hd",&(FiltreGeneral[num].degre));
				else FiltreGeneral[num].degre = 1;
				item = ItemJusquA('@',&r);
				u = item; ItemJusquA(' ',&u); unite = ItemJusquA(' ',&u);
				if(*item) sscanf(item,"%g",&(FiltreGeneral[num].omega1));
				else FiltreGeneral[num].omega1 = (float)0.1;
				if(unite[0]) {
					for(i=0; i<FILTRE_NBUNITES; i++) if(!strncmp(unite,FiltreUnites[i],3)) break;
					if(i < FILTRE_NBUNITES) FiltreGeneral[num].unite1 = i;
				}
				item = ItemJusquA(0,&r);
				u = item; ItemJusquA(' ',&u); unite = ItemJusquA(' ',&u);
				if(*item) sscanf(item,"%g",&(FiltreGeneral[num].omega2));
				else FiltreGeneral[num].omega2 = (float)0.0001;
				if(unite[0]) {
					for(i=0; i<FILTRE_NBUNITES; i++) if(!strncmp(unite,FiltreUnites[i],3)) break;
					if(i < FILTRE_NBUNITES) FiltreGeneral[num].unite2 = i;
				}
				//				printf("(FiltresLit) et du type #%d degre %d\n",FiltreGeneral[num].nom,i,FiltreGeneral[num].degre);
				if((FiltreGeneral[num].unite1 == FILTRE_REL) && (FiltreGeneral[num].unite2 == FILTRE_REL)) {
					printf(" . Calcul des coefficients du filtre #%d: %s\n",num+1,FiltreGeneral[num].nom);
					if(!FiltreCalculeStd(FiltreGeneral[num].omega1,FiltreGeneral[num].omega2,
						FiltreGeneral[num].modele,FiltreGeneral[num].type,FiltreGeneral[num].degre,
										 &(FiltreGeneral[num].coef))) printf(" ! Filtre inutilisable: %s\n",FiltreErreur);
				}
			} else if(!strcmp(item,"Echantillonnage")) sscanf(ItemJusquA(' ',&r),"%g",&FiltreEchant);
			else {
				if(FiltreGeneral[num].modele >= 0) continue;
				if(*item == '+') {
					if(++etage >= MAXFILTREETG) {
						if(log) printf("  ! Pas plus de %d etages. Derniers filtres non relus\n",MAXFILTREETG);
						break;
					}
					FiltreGeneral[num].coef.etage[etage].nbdir = FiltreGeneral[num].coef.etage[etage].nbinv = 0;
					item++;
				}
				directs = ((*item == 'd') || (*item == 'D'));
				nbcoef = 0;
				do {
					item = ItemJusquA(' ',&r);
					if(*item == '\0') break;
					if(nbcoef >= MAXFILTRECOEF) {
						if(log) printf("  ! Pas plus de %d coefficients par polynome. Derniers filtres non relus\n",MAXFILTRECOEF);
						break;
					}
					sscanf(item,"%lg",&coef);
					if(directs) FiltreGeneral[num].coef.etage[etage].direct[nbcoef] = coef;
					else FiltreGeneral[num].coef.etage[etage].inverse[nbcoef] = coef;
					nbcoef++;
				} while(1);
				if(nbcoef) FiltreGeneral[num].coef.nbetg = etage + 1;
				if(directs) {
					FiltreGeneral[num].coef.etage[etage].nbdir = nbcoef;
					nbmax = nbcoef - 1;
				} else {
					FiltreGeneral[num].coef.etage[etage].nbinv = nbcoef;
					nbmax = nbcoef;
				}
				if(FiltreGeneral[num].coef.nbmax < nbmax) FiltreGeneral[num].coef.nbmax = nbmax;
				FiltreGeneral[num].coef.calcule = 1;
			}
		}
	}
	if(!feof(fichier)) { if(log) printf("  ! Lecture des filtres en erreur (%s)\n",strerror(errno)); }
	fclose(fichier);
	strcpy(FiltreGeneral[FiltreNb].nom,"\0");
	FiltreNum = 0;
	// for(i=0; i<FiltreNb; i++) if(log) printf("%d/ %s @%08lX\n",i,FiltreGeneral[i].nom,&(FiltreGeneral[i].coef));
	return(1);
}
#define MAXFREQNOM 16
/* ========================================================================== */
void FiltreStandardiseNom(TypeFiltreDef *filtrage) {
	char nom[4],freq1[MAXFREQNOM],freq2[MAXFREQNOM];

	strncpy(nom,FiltreModeles[filtrage->modele],3); nom[3] = '\0';
	if(filtrage->unite1 == FILTRE_REL) snprintf(freq1,MAXFREQNOM,"%5.3f",filtrage->omega1);
	else snprintf(freq1,MAXFREQNOM,"%g%s",filtrage->omega1,FiltreUnites[filtrage->unite1]);
	if(filtrage->unite2 == FILTRE_REL) snprintf(freq2,MAXFREQNOM,"%5.3f",filtrage->omega2);
	else snprintf(freq2,MAXFREQNOM,"%g%s",filtrage->omega2,FiltreUnites[filtrage->unite2]);
	switch(filtrage->type) {
	  case FILTRE_PBAS: case FILTRE_PHAUT:
		snprintf(filtrage->nom,MAXFILTRENOM,"%s%c%d/%s",
			nom,toupper(FiltreTypes[filtrage->type][0]),filtrage->degre,freq1);
		filtrage->omega2 = (float)0.0001;
		break;
	  case FILTRE_PASSEB: case FILTRE_COUPEB:
		snprintf(filtrage->nom,MAXFILTRENOM,"%s%c%d/%s/%s",
			nom,toupper(FiltreTypes[filtrage->type][0]),filtrage->degre,freq1,freq2);
		break;
	  default:
		snprintf(filtrage->nom,MAXFILTRENOM,"%s%c%d",
			nom,toupper(FiltreTypes[filtrage->type][0]),filtrage->degre);
	}
}
/* ========================================================================== */
static int FiltresChange(void) {
	int rc;
	TypeFiltreDef *filtrage; TypeFiltre *filtre;

	rc = 0;
	filtrage = &(FiltreGeneral[FiltreNum]);
	filtre = &(filtrage->coef);
	if(filtrage->modele >= 0) {
#ifdef OPIUM_H
		PanelErase(pFiltreDef);
	#ifndef NOM_FILTRES_IMPOSES
		PanelItemSelect(pFiltreDef,PanelText (pFiltreDef,"Nom",filtrage->nom,MAXFILTRENOM),0);
	#endif
		PanelListB(pFiltreDef,"Modele",FiltreModeles,&(filtrage->modele),MAXFILTRENOM);
		PanelListB(pFiltreDef,"Type",FiltreTypes,&(filtrage->type),MAXFILTRENOM);
		PanelShort(pFiltreDef,"Degre",&(filtrage->degre));
		PanelFloat(pFiltreDef,"Coupure",&(filtrage->omega1));
		PanelListB(pFiltreDef,"Unite",FiltreUnites,&(filtrage->unite1),4);
		PanelFloat(pFiltreDef,"(Coupure haute)",&(filtrage->omega2));
		PanelListB(pFiltreDef,"(Unite)",FiltreUnites,&(filtrage->unite2),4);
		PanelText (pFiltreDef,"Commentaire",filtrage->commentaire,80);

		rc = OpiumExec(pFiltreDef->cdr);
		if(rc == PNL_OK) {
	#ifdef NOM_FILTRES_IMPOSES
			FiltreStandardiseNom(filtrage);
			OpiumNote("Ce nouveau filtre s'appelle: \"%s\"",filtrage->nom);
	#endif
			filtre->calcule = 0;
			if((filtrage->unite1 == FILTRE_REL) && (filtrage->unite2 == FILTRE_REL)) {
				if(!FiltreCalculeStd(filtrage->omega1,filtrage->omega2,filtrage->modele,
									 filtrage->type,filtrage->degre,filtre)) {
					rc = PNL_CANCEL;
				}
			} else filtrage->coef.nbetg = 0;
		}
	} else {
		int i,j;
		if(OpiumReadByte("Degre final",&(filtre->nbmax)) == PNL_CANCEL) return(0);
		if(pFiltreCoef) {
			if(filtre->nbmax != FiltreDegreMax) {
				PanelDelete(pFiltreCoef); pFiltreCoef = 0;
			} else PanelErase(pFiltreCoef);
		}
		filtre->calcule = 0;
		filtre->nbetg = 1;
		filtre->etage[0].nbdir = filtre->nbmax + 1;
		filtre->etage[0].nbinv = filtre->nbmax;
		if(!pFiltreCoef) {
			FiltreDegreMax = filtre->nbmax;
			pFiltreCoef = PanelCreate(2 * (filtre->nbmax + 4));
		}
		PanelColumns(pFiltreCoef,2);
		PanelText (pFiltreCoef,"Nom",filtrage->nom,MAXFILTRENOM);
		i = PanelText (pFiltreCoef,"Commentaire",filtrage->commentaire,16);
		PanelItemSouligne(pFiltreCoef,i,1);
		PanelItemSouligne(pFiltreCoef,PanelItemSelect(pFiltreCoef,PanelText(pFiltreCoef,0,"directs",7),0),1);
		for(j=0; j<filtre->etage[0].nbdir; j++) {
			PanelDouble(pFiltreCoef,&(FiltreNomDirect[j][0]),&(filtre->etage[0].direct[j]));
		}
		PanelRemark(pFiltreCoef,0); i = PanelRemark(pFiltreCoef,0);
		PanelItemSouligne(pFiltreCoef,i,1);
		PanelItemSouligne(pFiltreCoef,PanelItemSelect(pFiltreCoef,PanelText(pFiltreCoef,0,"inverses",8),0),1);
		PanelItemSelect(pFiltreCoef,PanelText(pFiltreCoef,0,"",7),0);
		for(j=0; j<filtre->etage[0].nbinv; j++) {
			PanelDouble(pFiltreCoef,&(FiltreNomInverse[j][0]),&(filtre->etage[0].inverse[j]));
		}
		rc = OpiumExec(pFiltreCoef->cdr);
#endif
	}

	return(rc);
}
/* ========================================================================== */
static int FiltresNouveau(int std, int original) {
	char nom[MAXFILTRENOM];
	int avant;

	if(FiltreNb >= FiltreMax) {
		sprintf(FiltreErreur,"Deja %d filtres",FiltreMax);
		return(0);
	}
	sprintf(nom,"Filtre #%d",FiltreNb+1);
#ifdef OPIUM_H
	#ifndef NOM_FILTRES_IMPOSES
	if(OpiumReadText("Nouveau nom",nom,MAXFILTRENOM) == PNL_CANCEL) return(0);
	#endif
#endif
	avant = FiltreNum;
	FiltreErreur[0] = '\0';
	FiltreNum = FiltreNb;
	FiltreGeneral[FiltreNum].coef.calcule = 0;
	if(original < 0) {
		FiltreGeneral[FiltreNum].commentaire[0] = '\0';
		if(std) {
			FiltreGeneral[FiltreNum].modele = FILTRE_BUT;
			FiltreGeneral[FiltreNum].type = FILTRE_PHAUT;
			FiltreGeneral[FiltreNum].degre = 4;
			FiltreGeneral[FiltreNum].unite1 = FILTRE_REL;
			FiltreGeneral[FiltreNum].unite2 = FILTRE_REL;
			FiltreGeneral[FiltreNum].omega1 = (float)0.1;
			FiltreGeneral[FiltreNum].omega2 = (float)0.0001;
		} else {
			FiltreGeneral[FiltreNum].modele = -1;
			FiltreGeneral[FiltreNum].coef.nbmax = 2;
		}
	} else {
		memcpy(&(FiltreGeneral[FiltreNum]),&(FiltreGeneral[original]),sizeof(TypeFiltreDef));
		if(std) {
			if(FiltreGeneral[FiltreNum].modele < 0) FiltreGeneral[FiltreNum].modele = FILTRE_BUT;
		} else FiltreGeneral[FiltreNum].modele = -1;
	}
	strcpy(FiltreGeneral[FiltreNum].nom,nom);
	if(FiltresChange() == PNL_OK) {
		FiltreNb++;
		strcpy(FiltreGeneral[FiltreNb].nom,"\0");
		return(1);
	} else {
		if(FiltreErreur[0]) printf("! Filtre #%d (%s): %s\n",FiltreNum+1,FiltreGeneral[FiltreNum].nom,FiltreErreur);
		FiltreNum = avant;
		return(0);
	}
}
/* ========================================================================== */
int FiltresAjout(void) {
#ifdef OPIUM_H
	int std;
	std = 1 - OpiumChoix(2,FiltreChoixType,"Filtres standard, ou entree des coefficients a la main?");
	if(!FiltresNouveau(std,-1) && FiltreErreur[0]) OpiumFail(FiltreErreur);
#endif
	return(0);
}
/* ========================================================================== */
int FiltresModif(void) {
#ifdef OPIUM_H
	if(FiltreNb <= 0) {
		OpiumWarn("Pas de filtre en memoire");
		return(0);
	}
	if(OpiumReadList("Filtre a modifier",FiltreListe,&FiltreNum,MAXFILTRENOM) == PNL_CANCEL) return(0);
	FiltresChange();
#endif
	return(0);
}
/* ========================================================================== */
int FiltresPerso(void) {
#ifdef OPIUM_H
	float facteur,unite,omega1,omega2;
	TypeFiltreDef *filtrage; TypeFiltre *filtre;

	if(FiltreNb <= 0) {
		OpiumWarn("Pas de filtre en memoire");
		return(0);
	}
	if(OpiumReadList("Filtre a personnaliser",FiltreListe,&FiltreNum,MAXFILTRENOM) == PNL_CANCEL) return(0);
	filtrage = &(FiltreGeneral[FiltreNum]);
	filtre = &(filtrage->coef);
	filtre->calcule = 0;
	if((filtrage->unite1 != FILTRE_REL) || (filtrage->unite2 != FILTRE_REL)) {
		if(OpiumReadFloat("Frequence d'echantillonnage (kHz)",&FiltreEchant) == PNL_CANCEL) return(0);
		facteur = (float)1.0 / FiltreEchant;
		if(filtrage->unite1 != FILTRE_REL) {
			switch(filtrage->unite1) {
				case FILTRE_HZ:  unite = (float)1.0e-3; break;
				case FILTRE_KHZ: unite = 1.0; break;
				case FILTRE_MHZ: unite = 1.0e3; break;
				case FILTRE_GHZ: unite = 1.0e6; break;
				default: unite = 1.0; break;
			}
			omega1 = filtrage->omega1 * unite * facteur;
		} else omega1 = filtrage->omega1;
		if(filtrage->unite2 != FILTRE_REL) {
			switch(filtrage->unite2) {
				case FILTRE_HZ:  unite = (float)1.0e-3; break;
				case FILTRE_KHZ: unite = 1.0; break;
				case FILTRE_MHZ: unite = 1.0e3; break;
				case FILTRE_GHZ: unite = 1.0e6; break;
				default: unite = 1.0; break;
			}
			omega2 = filtrage->omega2 * unite * facteur;
		} else omega2 = filtrage->omega2;
		FiltreCalculeStd(omega1,omega2,filtrage->modele,filtrage->type,filtrage->degre,filtre);
	}
	if(!FiltresNouveau(0,FiltreNum) && FiltreErreur[0]) OpiumFail(FiltreErreur);
#endif
	return(0);
}
/* ========================================================================== */
int FiltresDump(void) {
#ifdef OPIUM_H
	int i,j,etage;

	if(FiltreNb <= 0) {
		OpiumWarn("Pas de filtre en memoire");
		return(0);
	}
	if(!OpiumDisplayed(tFiltres->cdr)) OpiumDisplay(tFiltres->cdr); /* pour bien afficher des la premiere fois */
	TermEmpty(tFiltres);
	TermHold(tFiltres);
	for(i=0; i<FiltreNb; i++) {
		TermPrint(tFiltres,"%d/ %s : %s\n",i+1,FiltreGeneral[i].nom,FiltreGeneral[i].commentaire);
		if(FiltreGeneral[i].modele >= 0) {
			TermPrint(tFiltres,"    =%s/%s *%d",
				FiltreModeles[FiltreGeneral[i].modele],FiltreTypes[FiltreGeneral[i].type],
				FiltreGeneral[i].degre);
			if((FiltreGeneral[i].type == FILTRE_PASSEB) || (FiltreGeneral[i].type == FILTRE_COUPEB))
				TermPrint(tFiltres," @%g %s @%g %s\n",
					FiltreGeneral[i].omega1,FiltreUnites[FiltreGeneral[i].unite1],
					FiltreGeneral[i].omega2,FiltreUnites[FiltreGeneral[i].unite2]);
			else TermPrint(tFiltres," @%g %s\n",FiltreGeneral[i].omega1,FiltreUnites[FiltreGeneral[i].unite1]);
		}/* else { */
			if(FiltreGeneral[i].coef.nbetg) for(etage=0; etage<FiltreGeneral[i].coef.nbetg; etage++) {
				TermPrint(tFiltres,"   %c%d directs:",etage?'+':' ',FiltreGeneral[i].coef.etage[etage].nbdir);
				for(j=0; j<FiltreGeneral[i].coef.etage[etage].nbdir; j++)
					TermPrint(tFiltres," %.16lg",FiltreGeneral[i].coef.etage[etage].direct[j]);
				TermPrint(tFiltres,"\n    %d inverses:",FiltreGeneral[i].coef.etage[etage].nbinv);
				for(j=0; j<FiltreGeneral[i].coef.etage[etage].nbinv; j++)
					TermPrint(tFiltres," %.16lg",FiltreGeneral[i].coef.etage[etage].inverse[j]);
				TermPrint(tFiltres,"\n");
			} else TermPrint(tFiltres,"    (coefficients non calcules)\n");
		/* } */
	}
	TermPrint(tFiltres,"===== %d filtre(s) =====\n",FiltreNb);
	TermRelease(tFiltres);
	OpiumRefresh(tFiltres->cdr);
#endif
	return(0);
}
/* ========================================================================== */
int FiltresRetrait(void) {
	int i;

	if(FiltreNb <= 0) {
		sprintf(FiltreErreur,"Pas de filtre en memoire");
		return(0);
	}
#ifdef OPIUM_H
	if(OpiumReadList("Filtre a supprimer",FiltreListe,&FiltreNum,MAXFILTRENOM) == PNL_CANCEL) return(0);
#endif
	FiltreNb--;
	for(i=FiltreNum; i<FiltreNb; i++) memcpy(FiltreGeneral+i,FiltreGeneral+i+1,sizeof(TypeFiltreDef));
	strcpy(FiltreGeneral[FiltreNb].nom,"\0");
	if(FiltreNum >= FiltreNb) FiltreNum = FiltreNb - 1;
	return(0);
}
/* ========================================================================== */
int FiltresEcrit(char *nom) {
	int i,j,etage; FILE *f;

	if(FiltreNb <= 0) {
		sprintf(FiltreErreur,"Pas de filtre en memoire");
		return(0);
	}
	if(!(f = fopen(nom,"w"))) {
		sprintf(FiltreErreur,"Fichier inutilisble (%s)",strerror(errno));
		return(0);
	}
	fprintf(f,"# Definition des filtres, enregistree le %s a %s\n",DateCivile(),DateHeure());
	fprintf(f,"#\nEchantillonnage = %g   # Echantillonnage (kHz) pour personnalisation des standards\n#\n",FiltreEchant);
	for(i=0; i<FiltreNb; i++) {
		fprintf(f,"#\n=%s : %s\n",FiltreGeneral[i].nom,FiltreGeneral[i].commentaire);
		if(FiltreGeneral[i].modele >= 0) fprintf(f,"    =%s/%s *%d @%g %s @%g %s\n",
			FiltreModeles[FiltreGeneral[i].modele],FiltreTypes[FiltreGeneral[i].type],
			FiltreGeneral[i].degre,
			FiltreGeneral[i].omega1,FiltreUnites[FiltreGeneral[i].unite1],
			FiltreGeneral[i].omega2,FiltreUnites[FiltreGeneral[i].unite2]);
		else {
			for(etage=0; etage<FiltreGeneral[i].coef.nbetg; etage++) {
				fprintf(f,"   %cdirects=",etage?'+':' ');
				for(j=0; j<FiltreGeneral[i].coef.etage[etage].nbdir; j++)
					fprintf(f," %g",FiltreGeneral[i].coef.etage[etage].direct[j]);
				fprintf(f,"\n    inverses=");
				for(j=0; j<FiltreGeneral[i].coef.etage[etage].nbinv; j++)
					fprintf(f," %g",FiltreGeneral[i].coef.etage[etage].inverse[j]);
				fprintf(f,"\n");
			}
		}
	}
	fclose(f);
	return(0);
}
/* ========================================================================== */
int FiltresRestore(void) {
#ifdef OPIUM_H
	if(OpiumReadFile("Depuis quel fichier",FiltreFichier,MAXFILENAME) == PNL_CANCEL) return(0);
#endif
	if(!FiltresLit(FiltreFichier,1)) sprintf(FiltreErreur,"Erreur(s) dans le fichier (voir journal)");
	return(0);
}
/* ========================================================================== */
int FiltresSauve(void) {
#ifdef OPIUM_H
	if(OpiumReadFile("Sur quel fichier",FiltreFichier,MAXFILENAME) == PNL_CANCEL) return(0);
#endif
	if(!FiltresEcrit(FiltreFichier)) sprintf(FiltreErreur,"FiltreErreur");
	return(0);
}
/* ========================================================================== */
int FiltresInit(int max) {
	int i; char home[MAXFILENAME];

	FiltreMax = max;
	FiltreNum = FiltreNb = 0;
	FiltreEchant = 1.0;
	FiltreErreur[0] = '\0';
	mFiltres = 0; pFiltreDef = 0; tFiltres = 0;
	getcwd(home,MAXFILENAME);
	sprintf(FiltreFichier,"%s/filtres",home);
	FiltreGeneral = (TypeFiltreDef *)malloc((max + 1) * sizeof(TypeFiltreDef));
	if(!FiltreGeneral) {
		sprintf(FiltreErreur,"Filtres generaux: %d octets introuvables",(max + 1)*(int)sizeof(TypeFiltreDef));
		FiltreMax = 0;
		return(0);
	}
	strcpy(FiltreGeneral[0].nom,"neant");
	FiltreGeneral[1].nom[0] = '\0';
	FiltreListe = (char **)malloc((max + 2) * sizeof(char *));
	if(FiltreListe) {
		for(i=0; i<max+1; i++) FiltreListe[i] = FiltreGeneral[i].nom;
		FiltreListe[i] = "\0";
	} else {
		sprintf(FiltreErreur,"Noms des filtres: %d octets introuvables",(max + 2)*(int)sizeof(char *));
		FiltreMax = 0;
		return(0);
	}
	return(1);
}
/* ========================================================================== */
void FiltresDialogueCreate(void) {
#ifdef OPIUM_H
	int j;

	mFiltres = MenuLocalise(iFiltres);
	OpiumTitle(mFiltres->cdr,"Filtres");
	for(j=0; j<MAXFILTRECOEF; j++) {
		sprintf(&(FiltreNomDirect[j][0]),"a%d",j);
		sprintf(&(FiltreNomInverse[j][0]),"b%d",j+1);
	}
	pFiltreDef = PanelCreate(9);
	pFiltreCoef = 0; FiltreDegreMax = 0;
	tFiltres = TermCreate(24,80,8192);
#endif
	return;
}
/* ========================================================================== */
void FiltresExit(void) {
	FiltreMax = 0;
	FiltreNum = FiltreNb = 0;
	FiltreErreur[0] = '\0';
#ifdef OPIUM_H
	if(mFiltres) MenuDelete(mFiltres);
	if(pFiltreDef) PanelDelete(pFiltreDef);
	if(tFiltres) TermDelete(tFiltres);
	if(FiltreGeneral) free(FiltreGeneral);
	if(FiltreListe) free(FiltreListe);
#endif
	return;
}
