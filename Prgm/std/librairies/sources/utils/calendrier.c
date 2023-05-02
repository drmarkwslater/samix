/*
 *  calendrier.c
 *  Librairies
 *
 *  Created by Michel GROS on 28.11.16.
 *  Copyright 2016 __MyCompanyName__. All rights reserved.
 *
 */
#include <string.h>
#include <errno.h>

#include <calendrier.h>
#include "claps.h"
#include "decode.h"
#include "dateheure.h"
#define CALDR_MAX 128

#define CALDR_NOMMAX 32
static struct {
	char dossier[MAXFILENAME];
	char definition[CALDR_NOMMAX];
	char evenements[MAXFILENAME];
} CaldrReference;

static ArgDesc CaldrRefDesc[] = {
	{ "dossier",      DESC_STR(MAXFILENAME)   CaldrReference.dossier,    0 },
	{ "definition",   DESC_STR(MAXFILENAME)   CaldrReference.definition, 0 },
	{ "evenements",   DESC_STR(MAXFILENAME)   CaldrReference.evenements, 0 },
	DESC_END
};
static char *CaldrNom[CALDR_MAX]; static int CaldrNb;

#define CALDR_EVTMAX 256

static char *CaldrIcs[CALDR_EVTMAX]; static int CaldrIcsNb;

/* ========================================================================== */
char CaldrInit(char *fichier) {
	char rc;

	CaldrNb = 0;
	RepertoireInit(CaldrReference.dossier); strcat(CaldrReference.dossier,"Library/Calendars");
	strcpy(CaldrReference.definition,"Info.plist");
	strcpy(CaldrReference.evenements,"Events");
	if((rc = ArgScan(fichier,CaldrRefDesc)) < 1) ArgPrint(fichier,CaldrRefDesc);
	RepertoireListeCree(REPERT_REP,CaldrReference.dossier,CaldrNom,CALDR_MAX,&CaldrNb);
	RepertoireTermine(CaldrReference.dossier);
	// printf("> Trouve %d calendrier%s dans %s\n",Accord1s(CaldrNb),CaldrReference.dossier);
	return(rc);
}
/* ========================================================================== */
int CaldrOuvre(char *nom) {
	int cal; char cherche;
	char nom_complet[MAXFILENAME],ligne[MAXFILENAME],*r,*cle;
	FILE *f;
	
	// printf("> Recherche le calendrier %s\n",nom);
	for(cal=0; cal<CaldrNb; cal++) {
		strcat(strcpy(nom_complet,CaldrReference.dossier),CaldrNom[cal]); RepertoireTermine(nom_complet);
		// printf("  + Regarde si ce calendrier est dans %s\n",nom_complet);
		strcat(nom_complet,CaldrReference.definition);
		if(!(f = fopen(nom_complet,"r"))) { /* printf("  ! %s: %s!\n",nom_complet,strerror(errno)); */ continue; }
		// printf("> Fichier ouvert: %s\n",nom_complet);
		cherche = 1;
		while(LigneSuivante(ligne,MAXFILENAME,f)) {
			// printf("    | %s",ligne);
			r = ligne;
			ItemJusquA('<',&r);
			cle = ItemJusquA('>',&r);
			if(cherche) {
				if(strcmp(cle,"key")) continue;
				cle = ItemJusquA('<',&r);
				if(strcmp(cle,"Title")) continue;
				cherche = 0;
				ItemJusquA('<',&r);
				if(*r == '\0') continue;
				else cle = ItemJusquA('>',&r);
			}
			if(strcmp(cle,"string")) continue; // en fait, gros probleme de syntaxe dans ce fichier..
			cle = ItemJusquA('<',&r);
			// printf("    | Calendrier #%d: %s\n",cal+1,cle);
			if(!strcmp(cle,nom)) { fclose(f); return(cal + 1); } else break;
		}
		fclose(f);
	}
	return(0);
}
/* ========================================================================== */
int CaldrEvenements(int cal, CaldrEvt evt, int max) {
	char nom_complet[MAXFILENAME],nom_evt[MAXFILENAME],ligne[MAXFILENAME],*r,*cle,*val;
	FILE *f;
	int d,i,j,k,l; int jour,heure;
	TypeCaldrEvt evt_courant;
	
	strcat(strcpy(nom_complet,CaldrReference.dossier),CaldrNom[cal-1]); RepertoireTermine(nom_complet);
	strcat(nom_complet,CaldrReference.evenements);
	RepertoireListeCree(REPERT_FIC,nom_complet,CaldrIcs,CALDR_EVTMAX,&CaldrIcsNb);
	RepertoireTermine(nom_complet);
	if(max < CaldrIcsNb) d = CaldrIcsNb - max; else d = 0;
	j = 0;
	for(i=d; i<CaldrIcsNb; i++) {
		strcat(strcpy(nom_evt,nom_complet),CaldrIcs[i]);
		// printf("  * Lecture du fichier #%d pour l'evenement #%d: %s\n",i+1,j+1,nom_evt);
		if(!(f = fopen(nom_evt,"r"))) continue;
		bzero(&evt_courant,sizeof(TypeCaldrEvt));
		while(LigneSuivante(ligne,MAXFILENAME,f)) {
			// printf("    | %s",ligne);
			r = ligne;
			cle = ItemJusquA(':',&r);
			val = ItemJusquA(0,&r);
			if(!strncmp(cle,"SUMMARY",7)) {
				strncpy(evt_courant.titre,val,CALDR_EVTNOM); evt_courant.titre[CALDR_EVTNOM-1] = '\0';
			} else if(!strncmp(cle,"DTSTART",7)) {
				sscanf(val+2,"%06d",&jour); sscanf(val+9,"%06d",&heure);
				evt_courant.date.debut = DateJourHeureToLong(jour,heure);
				if(evt_courant.date.fin == 0) evt_courant.date.fin = evt_courant.date.debut;
			} else if(!strncmp(cle,"DTEND",5)) {
				sscanf(val+2,"%06d",&jour); sscanf(val+9,"%06d",&heure);
				evt_courant.date.fin = DateJourHeureToLong(jour,heure);
			} else if(!strncmp(cle,"DESCRIPTION",8)) {
				strncpy(evt_courant.texte,val,CALDR_EVTTXT); evt_courant.texte[CALDR_EVTTXT-1] = '\0';
			}
		}
		if(j) {
			for(k=j-1; k>=0; ) {
				if(evt[k].date.debut < evt_courant.date.debut) break;
				--k;
			}
			k++;
			for(l=j; l>k; ) { memcpy(evt+l,evt+l-1,sizeof(TypeCaldrEvt)); --l; }
		} else k = j;
		memcpy(evt+k,&evt_courant,sizeof(TypeCaldrEvt));
		fclose(f); j++;
	}
	return(j);
}
