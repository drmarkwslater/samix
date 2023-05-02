#include <stdio.h>
#include <stdlib.h>  // malloc
#include <string.h>  // strcmp, strlen
#include <stdarg.h>
#include "decode.h"
#include "dico.h"

/* ========================================================================== */
DicoLexique DicoInit(Dico dico, char *nom, int max) {
	strncpy(dico->nom,nom,MAXDICONOM);
	dico->nbtermes = 0;
	dico->maxtermes = max;
	if(max > 0) dico->terme = (DicoLexique)malloc(dico->maxtermes * sizeof(TypeDicoLexique));
	else dico->terme = 0;
	dico->enrichi = DICO_FIXE;
	return(dico->terme);
}
/* ========================================================================== */
DicoLexique DicoReset(Dico dico, char *nom, int max) {
	if(nom) strncpy(dico->nom,nom,MAXDICONOM);
	dico->nbtermes = 0;
	if((max >= 0) && (max != dico->maxtermes)) {
		if(dico->terme) free(dico->terme);
		if(max > 0) dico->terme = (DicoLexique)malloc(max * sizeof(TypeDicoLexique));
		else dico->terme = 0;
		if(dico->terme) dico->maxtermes = max;
		else dico->maxtermes = 0;
	}
	dico->enrichi = DICO_FIXE;
	return(dico->terme);
}
/* ========================================================================== */
void DicoRaz(Dico dico) { dico->nbtermes = 0; }
/* ========================================================================== */
char DicoLit(Dico dico, char *fichier) {
	FILE *f; char ligne[256],*r,sep;
	char *accepte,*officiel; int nb;

	if(!fichier) return(0);
	if(fichier[0] == '\0') return(0);
	if((f = fopen(fichier,"r"))) {
		nb = 0;
		while(LigneSuivante(ligne,256,f)) {
			if((ligne[0] == '#') || (ligne[0] == '\n')) continue;
			r = ligne;
			accepte = ItemAvant(":=",&sep,&r);
			officiel = ItemAvant(";",&sep,&r);
			DicoInsere(dico,accepte,officiel); nb++;
		}
		fclose(f);
		// printf("* %d ligne%s lue%s dans %s, %d terme%s defini%s dans le dictionnaire %s\n",Accord2s(nb),fichier,Accord2s(dico->nbtermes),dico->nom);
		return(1);
	} else return(0);
}
/* ========================================================================== */
void DicoEnrichi(Dico dico, DICO_MODE demande) { dico->enrichi = (char)demande; }
/* ========================================================================== */
void DicoAjouteTerme(DicoLexique terme, char *accepte, char *officiel) {
	terme->accepte = (char *)malloc(strlen(accepte)+1);
	strcpy(terme->accepte,accepte);
	terme->officiel = (char *)malloc(strlen(officiel)+1);
	strcpy(terme->officiel,officiel);
}
/* ========================================================================== */
static INLINE char DicoTrouve(Dico dico, char *cherche, int *index) {
	int i,di,inf,sup; int diff; char init;

	if(!cherche) { *index = 0; return(1); }
	if(dico->nbtermes == 0) { *index = 0; return(0); }
	inf = 0; sup = dico->nbtermes - 1; init = 1;
	i = sup;
	do {
		if(dico->terme[i].accepte) {
			if(dico->terme[i].officiel[0]) 
				diff = strcmp(cherche,dico->terme[i].officiel);
			else diff = 1;
			if(diff) diff = strcmp(cherche,dico->terme[i].accepte);
		} else diff = 1;
		if(diff > 0) {
			if(i >= (dico->nbtermes - 1)) { *index = dico->nbtermes; return(0); }
			di = (sup - i) / 2; 
			if(di == 0) { *index = i + 1; return(0); }
			inf = i; i += di;
		} else if(diff < 0) {
			if(i <= 0) { *index = i; return(0); }
			if(init) di = sup - inf; else di = (i - inf) / 2;
			if(di == 0) { *index = i; return(0); }
			sup = i; i -= di; init = 0;
		} else /* (diff == 0) */ { *index = i; return(1); }
	} while(1);

	return(0);
}
/* ========================================================================== */
char DicoAjouteLigne(Dico dico, int i, char *accepte, char *officiel) {
	int j; DicoLexique terme;

	if(dico->nbtermes >= dico->maxtermes) {
		terme = dico->terme;
		if(dico->maxtermes) dico->maxtermes *= 2; else dico->maxtermes = 16;
		dico->terme = (DicoLexique)malloc(dico->maxtermes * sizeof(TypeDicoLexique));
		if(!dico->terme) {
			dico->terme = terme;
			dico->maxtermes = dico->nbtermes;
			return(0); /* plein et pas possible de rallonger */
		}
		for(j=0; j<dico->nbtermes; j++) {
			dico->terme[j].accepte = terme[j].accepte;
			dico->terme[j].officiel = terme[j].officiel;
		}
		free(terme);
	}
	for(j=dico->nbtermes; j>i; ) {
		dico->terme[j].accepte = dico->terme[j-1].accepte;
		dico->terme[j].officiel = dico->terme[j-1].officiel;
		--j;
	}
	dico->nbtermes += 1;
	terme = &(dico->terme[i]);
	terme->accepte = (char *)malloc(strlen(accepte)+1);
	if(terme->accepte) strcpy(terme->accepte,accepte);
	if(officiel) {
		terme->officiel = (char *)malloc(strlen(officiel)+1);
		if(terme->officiel) strcpy(terme->officiel,officiel);
	} else {
		terme->officiel = (char *)malloc(1);
		if(terme->officiel) terme->officiel[0] = '\0';
	}
	
//	printf("* Dans dico %s[%d]: ajout de '%s' (%s)\n",dico->nom,i+1,terme->accepte,terme->officiel[0]?terme->officiel:"a traduire");
/*	 for(i=0; i<dico->nbtermes; i++) {
		printf("    %2d/ \"%s\": \"%s\"\n",i+1,dico->terme[i].accepte,dico->terme[i].officiel);
	 }
	 printf("    (%d termes)\n",dico->nbtermes); */

	return(1);
}
/* ========================================================================== */
char DicoInsere(Dico dico, char *accepte, char *officiel) {
	int i;

	if(DicoTrouve(dico,accepte,&i)) return(0); /* deja insere ou vide */
	return(DicoAjouteLigne(dico,i,accepte,officiel));
}
/* ========================================================================== */
char DicoTraduit(Dico dico, char **texte) {
	int i;

	if((*texte == 0) || (dico->terme == 0)) return(0);
	if(DicoTrouve(dico,*texte,&i)) {
		if(dico->terme[i].officiel[0]) { *texte = dico->terme[i].officiel; return(1); }
	}
	return(0);
}
/* ========================================================================== */
void DicoFormatte(Dico dico, char *texte, char *fmt, ...) {
	va_list argptr; int cnt;
	char *utilise;
	
	utilise = DicoLocale(dico,fmt);
	va_start(argptr, fmt);
	cnt = vsprintf(texte, utilise, argptr);
	va_end(argptr);
}
/* ========================================================================== */
DicoLexique DicoTerme(Dico dico, char *recherche, int *i) {
	if((recherche == 0) || (dico->terme == 0)) return(0);
	if(recherche[0] == '\0') return(0);
	if(DicoTrouve(dico,recherche,i)) return(&(dico->terme[*i]));
	else return(0);
}
/* ========================================================================== */
char *DicoLocale(Dico dico, char *accepte) {
	int i;

//	if(accepte == 0) printf("(%s) terme accepte inexistant\n",__func__);
//	if(dico->terme == 0) printf("(%s) dictionnaire vide\n",__func__);
	if((accepte == 0) || (dico->terme == 0)) return(accepte);
//	if(accepte[0] == '\0') printf("(%s) terme accepte vide\n",__func__);
	if(accepte[0] == '\0') return(accepte);
	if(DicoTrouve(dico,accepte,&i)) {
//		printf("(%s) \"%s\" -> \"%s\"\n",__func__,accepte,(dico->terme[i].officiel[0])?dico->terme[i].officiel:"(non traduit)");
		if(dico->terme[i].officiel[0]) return(dico->terme[i].officiel);
		else return(dico->terme[i].accepte);
	}
	if(dico->enrichi == DICO_MAJ) DicoAjouteLigne(dico,i,accepte,"");
	return(accepte);
}
/* ========================================================================== */
char **DicoLocaleListeNouvelle(Dico dico, char **liste) {
	char **traduite; int i,nb;

	nb = 0; while(liste[nb][0] != '\0') nb++;
	if(!nb) return(liste);
	if(!(traduite = (char **)malloc((nb + 1) * sizeof(char *)))) return(liste);
	for(i=0; i<nb; i++) traduite[i] = DicoLocale(dico,liste[i]);
	traduite[i] = "\0";
	return(traduite);
}
/* ========================================================================== */
char **DicoLocaleListeChange(Dico dico, char **liste, char **ancienne) {
	char **traduite; int i,nb;
	
	if(ancienne) { if(ancienne != liste) free(ancienne); }
	nb = 0; while(liste[nb][0] != '\0') nb++;
	if(!nb) return(liste);
	if(!(traduite = (char **)malloc((nb + 1) * sizeof(char *)))) return(liste);
	for(i=0; i<nb; i++) traduite[i] = DicoLocale(dico,liste[i]);
	traduite[i] = "\0";
	return(traduite);
}
/* ========================================================================== */
void DicoLocaleListeLibere(Dico dico, char **originale, char **ancienne) {
	if(ancienne) { if(ancienne != originale) free(ancienne); }
}
/* ========================================================================== 
char **DicoListeLocaleUneFois(Dico dico, char **liste) {
	char **traduite; int i,nb;
	
	nb = 0; while(liste[nb][0] != '\0') nb++;
	if(!nb) return(liste);
	if(!(traduite = (char **)malloc((nb + 1) * sizeof(char *)))) return(liste);
	for(i=0; i<nb; i++) traduite[i] = DicoLocale(dico,liste[i]);
	traduite[i] = "\0";
	return(traduite);
}
   .......................................................................... 
void DicoLocalise(Dico dico, char *accepte, char **officiel, char avec_maj) {
	int i;

	*officiel = accepte;
	if((accepte == 0) || (dico->terme == 0)) return;
	if(accepte[0] == '\0') return;
	if(DicoTrouve(dico,accepte,&i)) {
		if(dico->terme[i].officiel[0]) *officiel = dico->terme[i].officiel;
	} else if(avec_maj) DicoAjouteLigne(dico,i,accepte,"");
	return;
}
  .......................................................................... 
char **DicoListeLocalise(Dico dico, char **liste) {
	char **traduite; int i,nb;
	
	nb = 0; while(liste[nb][0] != '\0') nb++;
	if(!nb) return(liste);
	if(!(traduite = (char **)malloc((nb + 1) * sizeof(char *)))) return(liste);
	for(i=0; i<nb; i++) traduite[i] = DicoLocale(dico,liste[i]);
	traduite[i] = "\0";
	return(traduite);
}
   .......................................................................... 
char **DicoListeCree(Dico dico, char **liste, char avec_maj) {
	char **traduite; int i,nb;
	
	nb = 0; while(liste[nb][0] != '\0') nb++;
	if(!nb) return(0);
	if(!(traduite = (char **)malloc((nb + 1) * sizeof(char *)))) return(0);
	for(i=0; i<nb; i++) traduite[i] = DicoLocale(dico,liste[i]);
	traduite[i] = "\0";
	return(traduite);
}
   ========================================================================== */
void DicoCopie(Dico dico2, Dico dico1) {
	int i;
	
	strncpy(dico2->nom,dico1->nom,MAXDICONOM);
	dico2->nbtermes = dico1->nbtermes;
	dico2->enrichi = DICO_FIXE;
	if(dico1->nbtermes > 0)
		dico2->terme = (DicoLexique)malloc(dico2->nbtermes * sizeof(TypeDicoLexique));
	else dico2->terme = 0;
	for(i=0; i<dico1->nbtermes; i++) {
		dico2->terme[i].accepte = (char *)malloc(strlen(dico1->terme[i].accepte)+1);
		strcpy(dico2->terme[i].accepte,dico1->terme[i].accepte);
		dico2->terme[i].officiel = (char *)malloc(strlen(dico1->terme[i].officiel)+1);
		strcpy(dico2->terme[i].officiel,dico1->terme[i].officiel);
	}
}
/* ========================================================================== */
void DicoImprime(Dico dico, char *prefixe) {
	int i,k; size_t l;

	if(prefixe) l = strlen(prefixe); else l = 0;
	if(prefixe) fputs(prefixe,stdout); printf("Dictionnaire '%s':\n",dico->nom);
	for(i=0; i<dico->nbtermes; i++) {
		if(prefixe) { k = (int)l; while(k--) printf(" "); }; printf("| '%s' est accepte pour '%s'\n",dico->terme[i].accepte,dico->terme[i].officiel);
	}
	if(prefixe) { k = (int)l; while(k--) printf(" "); }; printf("| %d termes\n",dico->nbtermes);
}
/* ========================================================================== */
char DicoSauve(Dico dico, char *fichier) {
	FILE *f; int i;
	
	if((f = fopen(fichier,"w"))) {
		for(i=0; i<dico->nbtermes; i++) if(dico->terme[i].accepte && dico->terme[i].officiel) {
			if(dico->terme[i].officiel[0])
				fprintf(f,"\"%s\": \"%s\"\n",dico->terme[i].accepte,dico->terme[i].officiel);
			else if(dico->enrichi != DICO_NET) fprintf(f,"\"%s\": \n",dico->terme[i].accepte);
		}
		fclose(f);
		return(1);
	} else return(0);
}
/* ========================================================================== */
void DicoVide(Dico dico) {
	int i;
	
	for(i=0; i<dico->nbtermes; i++) {
		free(dico->terme[i].accepte); dico->terme[i].accepte = 0;
		free(dico->terme[i].officiel); dico->terme[i].officiel = 0;
	}
	if(dico->terme) free(dico->terme); dico->terme = 0;
	dico->nom[0] = '\0';
}
