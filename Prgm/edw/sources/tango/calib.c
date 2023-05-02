#ifdef macintosh
#pragma message(__FILE__)
#endif

#include <stdio.h>
#include <stdlib.h> /* pour malloc, free et system */
#include <math.h>
#include <errno.h>
#ifndef WIN32
#include <strings.h>  /* pour strlen qui rend size_t si intrinseque */
#else
#include <string.h>
#endif

#include <environnement.h>

#include <defineVAR.h>
#include <decode.h>
#include <opium.h>
#include <nanopaw.h>  /* pour PlotNtupleExclu */
#include <fitlib.h>

#include <tango.h>
#include <rundata.h>
#include <calib.h>
#include <evtread.h>

typedef enum {
	CALIB_STOPPE = 0,
	CALIB_REFUSE,
	CALIB_ACCEPTE,
	CALIB_NB
} CALIB_ACTIONS;
static char *CalibBoutons[CALIB_NB+1] = { "Ca suffit", "Refuse", "Accepte", "\0" };

/* ========================================================================== */
int EvtUniteCherche(char *nom, float **adrs, int *lngr, int *dep, int *arr) {
	int i;

	for(i=0; i<EvtUniteNb; i++) if(!strcmp(nom,EvtUnite[i].nom)) break;
	if(i < EvtUniteNb) {
		*adrs = EvtData + EvtUnite[i].debut;
		*lngr = EvtUnite[i].taille;
		if(*arr == 0) {
			*dep = EvtUnite[i].dep;
			*arr = EvtUnite[i].arr;
		}
		return(i);
	} else {
		*adrs = EvtData;
		*lngr = 0;
		return(-1);
	}
}
/* ========================================================================== */
float EvtUniteAmplitude(char *nom) {
	int i;

	for(i=0; i<EvtUniteNb; i++) if(!strcmp(nom,EvtUnite[i].nom)) break;
	if(i < EvtUniteNb) return(EvtUnite[i].amplitude);
	else return(1.0);
}
/* ========================================================================== */
int CalibEvtRetire(int evt) {
	int i,debut,taille;

	debut = EvtUnite[evt].debut;
	taille = EvtUnite[evt].taille;
	EvtDataNb -= taille;
	for(i=debut; i<EvtDataNb; i++) EvtData[i] = EvtData[i + taille];
	EvtUniteNb--;
	for(i=evt; i<EvtUniteNb; i++) memcpy(EvtUnite+i,EvtUnite+i+1,sizeof(TypeEvtUnite));
	for(i=0; i<EvtUniteNb; i++) if(EvtUnite[i].debut > debut) EvtUnite[i].debut -= taille;
	return(0);
}
/* ========================================================================== */
int EvtUniteCalcule(char *nom, float energie_moyenne, char manu, char affichages) {
	void *adrs; int dim; TANGO_TYPE type;
	float *base,*energie,*khi2, *bruit; int dep,arr;
	int num,evt; float *evt_unite;
	int action;
	char *i8; short *i16; // int *i32; long *i64;
//	unsigned char *u8; unsigned short *u16; unsigned int *u32; unsigned long *u64;
	double *m,*p,n; int i;
	float val,ampl_neg,ampl_pos,base_moyenne;

	if(energie_moyenne == 0.0) {
		OpiumError("L'energie de l'evenement unite est nulle, et c'est illegal !!");
		return(0);
	}
/* Boucle sur tous les evenements enregistres ET valides */
	/* On n'a besoin que de adrs, dim et type */
	ProdChannelUnite(nom,&adrs,&dim,&type,&base,&dep,&arr,&energie,&khi2,&bruit);
	if((num = EvtUniteCherche(nom,&evt_unite,&i,&dep,&arr)) >= 0) CalibEvtRetire(num);
	if((EvtDataNb + dim) >= MAXEVTDATA) {
		OpiumError("Pas plus de %d valeurs au total. Evenement unite [%d] a v=%d non memorisable",MAXEVTDATA,dim,EvtDataNb);
		return(0);
	}
	m = (double *)malloc(dim * sizeof(double));
	if(m == 0) {
		OpiumError("Allocation de l'evenement tampon (%d valeurs) impossible",dim);
		return(0);
	}
	printf("- Calcul de l'evenement moyen %s calibre a E=%g\n",nom,energie_moyenne);
	p = m; i = dim; while(i--) *p++ = 0.0;
	strncpy(EvtUnite[EvtUniteNb].nom,nom,MAXEVTNOM);
	EvtUnite[EvtUniteNb].debut = EvtDataNb;
	EvtUnite[EvtUniteNb].taille = 0;
	n = 0.0;
	evt = -1;
	if(affichages) {
		OpiumProgresTitle("Calcul evenement unite");
		OpiumProgresInit(EvtNb);
	}
//	while((encore = EvtLitASuivre(++evt,0))) {
	do {
		if(++evt >= EvtNb) break;
		if(affichages) { if(!OpiumProgresRefresh(evt)) break; }
		if(EvtSel) { if(!EvtSel[evt]) continue; }
		if(PlotNtupleAbsent(EvtEspace,evt)) continue;
		if(!EvtLitASuivre(evt,0)) break;
		if(manu) {
			ArchEvtMontre(evt);
//			if(!OpiumCheck("%g%s evenement acceptable?",n+1.0,(n<0.9)?"er":"eme")) continue;
			action = OpiumChoix(3,CalibBoutons,"Utiliser ce %g%s evenement pour la moyenne?",n+1.0,(n<0.9)?"er":"eme");
			if(action == CALIB_STOPPE) break;
			else if(action == CALIB_REFUSE) continue;
		}
		switch(type) {
		  case TANGO_BYTE:
			i8 = (char *)adrs;
			for(i=0, p=m; i<dim; i++,p++,i8++) *p = *p + ((double)*i8 - (double)(base[evt]));
			break;
		  case TANGO_SHORT:
			i16 = (short *)adrs;
			for(i=0, p=m; i<dim; i++,p++,i16++) *p = *p + ((double)*i16 - (double)(base[evt]));
			break;
		  default: OpiumError("Type de donnees pas encore programme"); return(0);
		}
		n += 1.0;
	} while(1);
	
	if(n > 0.0) {
		printf(". %g evenements utilises\n",n);
		EvtDataNb += dim;
		EvtUnite[EvtUniteNb].taille = dim;
		evt_unite = EvtData + EvtUnite[EvtUniteNb].debut;
		for(i=0, p=m; i<dim; i++,p++) evt_unite[i] = (float)(*p / n);
		ProdChannelBasics(evt_unite,dim,&base_moyenne);
		ampl_pos = ampl_neg = 0.0;
		for(i=0; i<dim; i++) {
			evt_unite[i] = (evt_unite[i] - base_moyenne) / energie_moyenne;
			val = evt_unite[i];
			if(val < ampl_neg) ampl_neg = val;
			if(val > ampl_pos) ampl_pos = val;
		}
		EvtUnite[EvtUniteNb].amplitude = (ampl_pos > (-ampl_neg))? ampl_pos: ampl_neg;
		EvtUnite[EvtUniteNb].dep = dep;
		EvtUnite[EvtUniteNb].arr = arr;
		EvtUniteNb++;
	} else printf("* Aucun evenement n'est entre dans la moyenne, evenement moyen inchange\n");
	if(m) free(m);
	if(affichages) {
		OpiumProgresClose();
		if(evt < EvtNb) OpiumError("Seulement %d/%d evenements examines",evt,EvtNb);
//		else OpiumError("Fichier termine");
	}
	return(0);
}
/* ========================================================================== */
int EvtUniteSauve(char *nom) {
	int evt;
	FILE *fichier;
	int j,k;

	if(!(fichier = TangoFileOpen(nom,"w"))) {
		OpiumError("Sauvegarde de l'evenement unite impossible");
		printf("- Impossible de sauver l'evenement unite sur:\n    %s\n    (%s)\n",
			nom,strerror(errno));
		return(-1);
	}
	for(evt=0; evt<EvtUniteNb; evt++) {
		fprintf(fichier,"#\n=%s : %g (%d - %d)",EvtUnite[evt].nom,EvtUnite[evt].amplitude,
			EvtUnite[evt].dep,EvtUnite[evt].arr);
		for(j=0, k=EvtUnite[evt].debut; j<EvtUnite[evt].taille; j++,k++) {
			if(!(j%10)) fprintf(fichier,"\n");
//			fprintf(fichier," %08x",EvtData[k]);
			fprintf(fichier," %g",EvtData[k]);
		}
		fprintf(fichier,"\n");
	}
	fclose(fichier);
	printf("- Evenement unite sauve sur:\n    %s\n",nom);

	return(0);
}
/* ========================================================================== */
int EvtUniteRelit(char *nom) {
	FILE *fichier;
	char ligne[256],*r;
	char *lu,*hexa;
	int i,nb_erreurs;

	if(!(fichier = TangoFileOpen(nom,"r"))) {
		printf("- Impossible de lire l'evenement unite d'apres:\n  %s\n  (%s)\n",
			nom,strerror(errno));
		return(-1);
	}
	EvtUniteNb = 0; EvtDataNb = 0; /* Vidage prealable, mais pas obligatoire ... */
	nb_erreurs = 0;
	while(LigneSuivante(ligne,256,fichier)) {
		r = ligne;
		if((*r == '#') || (*r == '\n')) continue;
		if(*r == '=') {
			if(EvtUniteNb) EvtUnite[EvtUniteNb-1].taille = EvtDataNb - EvtUnite[EvtUniteNb-1].debut;
			if(EvtUniteNb >= MAXEVTMOYEN) {
				printf("- Pas plus de %d voies. Dernieres voies non relues\n",MAXEVTMOYEN);
				nb_erreurs++;
				break;
			}
			r++;
			lu = ItemSuivant(&r,':');
			for(i=0; i<EvtUniteNb; i++) if(!strcmp(lu,EvtUnite[i].nom)) break;
			if(i < EvtUniteNb) {
				printf("- La voie '%s' est deja connue! Dernieres voies non relues\n",lu);
				nb_erreurs++;
				break;
			}
			strncpy(EvtUnite[EvtUniteNb].nom,lu,MAXEVTNOM);
			EvtUnite[EvtUniteNb].debut = EvtDataNb;
			EvtUnite[EvtUniteNb].taille = 0;
			lu = ItemSuivant(&r,'(');
			sscanf(lu,"%g",&(EvtUnite[EvtUniteNb].amplitude));
			lu = ItemSuivant(&r,'-');
			if(lu[0]) sscanf(lu,"%d",&(EvtUnite[EvtUniteNb].dep)); else EvtUnite[EvtUniteNb].dep = 0;
			lu = ItemSuivant(&r,')');
			if(lu[0]) sscanf(lu,"%d",&(EvtUnite[EvtUniteNb].arr)); else EvtUnite[EvtUniteNb].arr = 1024;
			EvtUniteNb++;
		} else if(*r == ' ') {
			do {
				hexa = ItemSuivant(&r,' ');
				if(*hexa == '\0') break;
				if(EvtDataNb >= MAXEVTDATA) {
					printf("- Pas plus de %d valeurs au total. Dernieres voies non relues\n",MAXEVTDATA);
					nb_erreurs++;
					break;
				}
//				sscanf(hexa,"%x",&(EvtData[EvtDataNb++]));
				sscanf(hexa,"%g",&(EvtData[EvtDataNb++]));
			} while(1);
		}
	}
	if(EvtUniteNb) EvtUnite[EvtUniteNb-1].taille = EvtDataNb - EvtUnite[EvtUniteNb-1].debut;
	if(!feof(fichier)) {
		printf("- Lecture de l'evenement unite en erreur (%d!) sur:\n  %s\n  (Au niveau Systeme: %s)",
			nb_erreurs,nom,strerror(errno));
		nb_erreurs++;
	}
	fclose(fichier);
	printf("- Evenement unite relu sur:\n    %s\n",nom);
	return(0);
}
/* ========================================================================== */
int EvtUniteFitteEnergies(char *nom, char affichages) {
	void *adrs; TANGO_TYPE type;
	float *base,*energie,*khi2,*bruit;
	float *evt_unite; int lngr;
	int num,evt; char encore;
	int i,dep,arr,dim;
	float k,n;
	short *i16; // char *i8; int *i32; long *i64;
//	unsigned char *u8; unsigned short *u16; unsigned int *u32; unsigned long *u64;
	float numerateur,denominateur,diff;

	/* condition egalement utilisee dans fitlib */
#ifdef ALEXEI
	int j;              // shift of fitting process
	int jleft = 5;      // number of point taking into interpolation on the left
	int jright = 3;     // number of point taking into interpolation on the right
	int jshift = jleft + jright + 1;
	double num_tmp[jshift];
	int precision = 10; // number of point for divide interval between points.
	char avec_spline;
	avec_spline = 1;
#endif

	ProdChannelUnite(nom,&adrs,&dim,&type,&base,&dep,&arr,&energie,&khi2,&bruit);
	printf("- Normalisation des evenements %s\n",nom);
	if((num = EvtUniteCherche(nom,&evt_unite,&lngr,&dep,&arr)) < 0) {
		OpiumFail("L'evenement unite %s n'est pas calcule",nom);
		return(0);
	}
	if(affichages) {
		OpiumProgresTitle("Normalisation evenements");
		OpiumProgresInit(EvtNb);
	}
	dim = (lngr > dim)? dim: lngr;
	if(arr > dim) arr = dim;
	if(dep > arr) dep = arr;

	printf(". Normalisation sur [%d .. %d[\n",dep,arr);
/*	dep = 100; arr = 200;
	printf(". Normalisation sur [%d .. %d[\n",dep,arr); */
	n = (float)(arr - dep);
	if(n <= 0.0) {
		OpiumError("Evenement unite fourni sur %d valeurs: calcul sans interet",(int)n);
		return(0);
	}

	denominateur = 0.0;
	for(i=dep; i<arr; i++) denominateur += (evt_unite[i] * evt_unite[i]);
	if(denominateur <= 0.0) {
		OpiumError("Evenement unite plat: pas d'ajustement possible");
		return(0);
	}
	evt = -1;
	while((encore = EvtLitASuivre(++evt,0))) {
		if(affichages) { if(!OpiumProgresRefresh(evt)) break; }
//		if(!AutoriseEvt(evt)) continue;
		if(EvtSel) { if(!EvtSel[evt]) continue; }
		switch(type) {
		  case TANGO_SHORT:
			i16 = (short *)adrs;
#ifdef ALEXEI
			if(avec_spline) { 
				for (j = -jleft; j<jright; j++) /* Counting numerateur with shift */ {
					num_tmp[jleft+j] = 0.0;
					for(i=dep; i<arr; i++) {
						num_tmp[j+jleft] += (((float)i16[i+j] - base[evt]) * evt_unite[i]);
					}		
				} // End fitting j
				numerateur = Interpolate(jleft, jright, num_tmp, 3, 2, precision);
			} else 
#endif
			{
				numerateur = 0.0;
				for(i=dep; i<arr; i++) numerateur += (((float)i16[i] - base[evt]) * evt_unite[i]);
			}
			energie[evt] = numerateur / denominateur;
			k = 0.0; n = 0.0;
			for(i=dep; i<arr; i++) if(energie[evt] != 0.0) {
				diff = (((float)i16[i] - base[evt]) - (evt_unite[i] * energie[evt])) / energie[evt];
				k += (diff * diff);
				n += 1.0;
			}
			if(n > 0.0) khi2[evt] = k / n; else khi2[evt] = 0.0;
			break;
		  default: OpiumFail("Type de donnees pas encore programme"); return(0);
		}
		/* Seulement si c'est un vrai residu:
		if(energie[evt] != 0.0) residu[evt] /= energie[evt];
		else {
			printf("! Energie nulle pour l'evenement #%d\n",evt);
			residu[evt] = 1000.0;
		} */
	}
	if(affichages) {
		if(encore) OpiumWarn("Fichier abandonne au bout de %d evenements (%d%%)",evt,(evt *100 / EvtNb));
//		else OpiumNote("Fichier termine apres %d evenements",evt);
		OpiumProgresClose();
	}
	return(0);
}
#ifdef TROP_SIMPLE
/* ========================================================================== */
int CalibNormaliseSurAmplitude(float validite, char affichages) {
	char nom[80]; void *adrs; TANGO_TYPE type;
	float *ampl,*base,*bruit,*energie,*khi2;
	float *evt_unite; int lngr;
	int num,evt; char encore;
	int i,dep,arr,dim;
	float mini,diff; char positif; float k,n;
	char *i8; short *i16; int *i32; long *i64;
	unsigned char *u8; unsigned short *u16; unsigned int *u32; unsigned long *u64;
	float *r;

	if(validite > 0.9999) validite = 0.9999;
	/* pour l'instant, on se passe de ampl et bruit */
	ProdChannelAdrs(nom,&adrs,&dim,&type,&base,&ampl,&bruit,&energie,&khi2);
	printf("- Normalisation des evenements %s sur %g%% de l'amplitude moyenne maximum\n",
		nom,validite*100.0);
	if((num = EvtUniteCherche(nom,&evt_unite,&lngr,&dep,&arr)) < 0) {
		OpiumError("L'evenement unite %s n'est pas calcule",nom);
		return(0);
	}
	if(affichages) {
		OpiumProgresTitle("Normalisation evenements");
		OpiumProgresInit(EvtNb);
	}
	dim = (lngr > dim)? dim: lngr;
	mini = EvtUnite[num].amplitude * validite;
	positif = (EvtUnite[num].amplitude > 0.0);
	/* il faut qu'il y ait au moins un point de comparaison */
	if(positif) {
		for(dep=0; dep<dim; dep++) if(evt_unite[dep] >= mini) break;
		for(arr=dep; arr<dim; arr++) if(evt_unite[arr] < mini) break;
	} else {
		for(dep=0; dep<dim; dep++) if(evt_unite[dep] <= mini) break;
		for(arr=dep; arr<dim; arr++) if(evt_unite[arr] > mini) break;
	}
	if(dep == dim) {
		OpiumError("Intervalle de validite nul");
		return(0);
	}
	for(i=dep; i<arr; i++) if(evt_unite[i] == 0.0) break;
	if(i < arr) {
		OpiumError("Evt unite[%d] = 0. Calcul abandonne",i);
		return(0);
	}
	printf(". Normalisation sur [%d .. %d[\n",dep,arr);
	n = (float)(arr - dep);
	evt = -1;
	while((encore = EvtLitASuivre(++evt,0))) {
		if(affichages) { if(!OpiumProgresRefresh(evt)) break; }
//		if(!AutoriseEvt(evt)) continue;
		if(EvtSel) { if(!EvtSel[evt]) continue; }
		switch(type) {
		  case TANGO_SHORT:
			i16 = (short *)adrs;
			k = 0.0;
			for(i=dep; i<arr; i++) 
				k += ((float)i16[i] - base[evt]) / evt_unite[i];
			energie[evt] = k / n;
			k = 0.0;
			for(i=dep; i<arr; i++) {
				diff = (((float)i16[i] - base[evt]) - (evt_unite[i] * energie[evt]));
				k += (diff * diff);
			}
			khi2[evt] = k / n;
			break;
		  default: OpiumError("Type de donnees pas encore programme"); return(0);
		}
	}
	if(affichages) {
		if(encore) OpiumError("Fichier abandonne au bout de %d evenements (%d%%)\n",evt,(evt *100 / EvtNb));
		else OpiumError("Fichier termine apres %d evenements",evt);
		OpiumProgresClose();
	}
	return(0);
}
#endif
/* ========================================================================== */
void EvtUniteInit() {
	EvtUniteNb = 0;
	EvtDataNb = 0;
}

