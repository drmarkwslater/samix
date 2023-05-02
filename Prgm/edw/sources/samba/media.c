/*
 *  Created by Michel GROS on 07.10.14.
 *  Copyright 2014 CEA/DSM/IRFU. All rights reserved.
 */

#include <strings.h>

#define MEDIA_C
#include "media.h"

#include <defineVAR.h>
#include <utils/claps.h>
#include <utils/decode.h> // pour Accord1s()

#define MEDIA_TABLEMAX 16

static TypeMedia MediaTable[MEDIA_TABLEMAX],MediaLu;
static int MediaNb;

static ArgDesc MediaDesc[] = {
	{ 0,      DESC_STR(MEDIA_MAXNOM)    MediaLu.nom,  0 },
	{ "type", DESC_KEY MediaClesType,  &MediaLu.type, 0 },
	{ "port", DESC_STR(MEDIA_MAXPORT)   MediaLu.adrs, 0 },
	DESC_END
};
static ArgStruct MediaAS = { (void *)MediaTable, (void *)&MediaLu, sizeof(TypeMedia), MediaDesc };
static ArgDesc MediaTableDesc[] = {
	{ "media", DESC_STRUCT_SIZE(MediaNb,MEDIA_TABLEMAX) &MediaAS, 0 },
	DESC_END
};

/* ========================================================================== */
Media MediaCreate(char *nom, char type, char *adrs) {
	Media media;
	
	if((media = Malloc(1,TypeMedia))) MediaDefini(media,nom,adrs,type);
	return(media);
}
/* ========================================================================== */
static void MediaComplete(Media media) {
	char *c;
	
	cas_ou(media->type) {
		vaut MEDIA_USB:                       media->mode = MEDIA_RDWR; break;
		vaut MEDIA_TCP_WR: case MEDIA_UDP_WR: media->mode = MEDIA_WR;   break;
		vaut MEDIA_TCP_RD: case MEDIA_UDP_RD: media->mode = MEDIA_RD;   break;
		au_pire:                              media->mode = 0;          break;
	}
	if(media->type == MEDIA_USB) media->port = 0;
	else if(media->mode == MEDIA_WR) {
		if((c = index(media->adrs,':'))) { *c++ = '\0'; sscanf(c,"%d",&(media->port)); }
		else media->port = 7;
	} else sscanf(media->adrs,"%d",&(media->port));
	media->ip.path = -1;
}
/* .......................................................................... */
static void MediaImprime(Media media) {
	char port[16];

	if(media->port > 0) snprintf(port,16,":%d",media->port); else port[0] = '\0';
	printf("%-8s de type %-6s (%s <%d>) @%s%s\n",media->nom,MediaNomType[media->type],
		(media->mode == MEDIA_RD)? "Rd":((media->mode == MEDIA_WR)? "Wr":"RW"),media->ip.path,
		((media->type == MEDIA_USB) || (media->mode == MEDIA_WR))? media->adrs: "",port);
}
/* ========================================================================== */
void MediaDefini(Media media, char *nom, char *port, char type) {
	if(!media) return;
	strncpy(media->nom,nom,MEDIA_MAXNOM); media->nom[MEDIA_MAXNOM-1] = '\0';
	strncpy(media->adrs,port,MEDIA_MAXPORT); media->adrs[MEDIA_MAXPORT-1] = '\0';
	media->type = type;
	MediaComplete(media);
	// printf("(%s) Defini le media ",__func__); MediaImprime(media);
}
/* ========================================================================== */
int MediaConnecte(Media media) {	
	if(media) switch(media->type) {
		case MEDIA_USB: media->ip.path = UsbOpen(media->adrs); break;
		case MEDIA_TCP_WR: case MEDIA_UDP_WR:
			SocketInit(media->adrs,media->port,&(media->ip.skt));
			if(media->type == MEDIA_TCP_WR) media->ip.path = SocketConnect(&(media->ip.skt));
			else media->ip.path = SocketConnectUDP(&(media->ip.skt));
			break;
		case MEDIA_TCP_RD: case MEDIA_UDP_RD:
			if(media->type == MEDIA_TCP_RD) media->ip.path = SocketTCPServeur(media->port);
			else media->ip.path = SocketUDP(media->port);
			if(media->ip.path >= 0) fcntl(media->ip.path,F_SETFL,O_NONBLOCK);
			break;
	} else return(-1);
	return(media->ip.path);
}
/* ========================================================================== */
static void MediaTableInit() { MediaNb = 0; }
/* .......................................................................... */
static char MediaTableAjoute(char *nom, char *adrs, char type) {
	if(MediaNb >= MEDIA_TABLEMAX) return(0);
	MediaDefini(&(MediaTable[MediaNb++]),nom,adrs,type);
	return(1);
}
/* ========================================================================== */
char MediaLit(char *nom_complet, char cree) {
	int m;
	char *liste[MEDIA_TABLEMAX]; int nb_usb;
	
	if(ArgScan(nom_complet,MediaTableDesc) <= 0) {
		MediaTableInit();
		if(cree) {
			UsbListeCree(liste,MEDIA_TABLEMAX,&nb_usb);
			//	if(nb_usb < MEDIA_TABLEMAX) *liste[nb_usb] = '\0';
			if(nb_usb) MediaTableAjoute("sy127",liste[0],MEDIA_USB);
			MediaTableAjoute("orca","134.158.176.235:4667",MEDIA_TCP_WR);
			MediaSauve(nom_complet);
		}
	} else for(m=0; m<MediaNb; m++) {
		MediaComplete(&(MediaTable[m]));
		MediaConnecte(&(MediaTable[m]));
	}
	/*
	int rc;
	rc = ArgScan(nom_complet,MediaTableDesc);
	printf("* Table lue, retour=%d, %d media%s:\n",rc,Accord1s(MediaNb));
	for(m=0; m<MediaNb; m++) { printf("  | %2d:",m+1); MediaImprime(&(MediaTable[m])); }
	printf("\n");
	if(rc <= 0) { }
	 */

	return(1);
}
/* ========================================================================== */
void MediaSauve(char *nom_complet) { ArgPrint(nom_complet,MediaTableDesc); }

/* ========================================================================== */
int MediaListeInit(TypeMediaListe *liste, int max) {
	if(!liste) return(0);
	if(max) {
		if((liste->media = (Media *)Malloc(max,TypeMedia))) {
			liste->max = max;
			liste->nb = 0;
		} else liste->max = 0;
	} else if(liste->media) {
		free(liste->media); liste->max = 0;
		liste->nb = 0;
	}
	return(liste->max);		
}
/* ========================================================================== */
int MediaListeAjouteDef(TypeMediaListe *liste, char *nom, char *adrs, char type) {
	if(!liste) return(0);
	if(liste->nb >= liste->max) return(0);
	MediaDefini(liste->media[(liste->nb)++],nom,adrs,type);
	return(liste->nb);
}
/* ========================================================================== */
int MediaListeAjouteRef(TypeMediaListe *liste, Media media) {
	if(!liste) return(0);
	if(liste->nb >= liste->max) return(0);
	liste->media[(liste->nb)++] = media;
	return(liste->nb);
}
/* ========================================================================== */
int MediaListeAjouteLus(TypeMediaListe *liste) {
	int m;
	
	if(!liste) return(0);
	for(m=0; m<MediaNb; m++) {
		if(liste->nb >= liste->max) return(0);
		liste->media[(liste->nb)++] = &(MediaTable[m]);
	}
	return(liste->nb);
}
/* ========================================================================== */
int MediaListeDim(TypeMediaListe *liste) { if(liste) return(liste->nb); else return(0); }
/* ========================================================================== */
Media MediaListeNum(TypeMediaListe *liste, int num) {
	if(liste && (num >= 0) && (num < liste->nb)) return(liste->media[num]);
	else return(0);
}
/* ========================================================================== */
int MediaListeIndex(TypeMediaListe *liste, char *nom, char mode) {
	int m;
	
	if(liste) for(m=0; m<liste->nb; m++) 
		if(!strcmp(nom,liste->media[m]->nom) && (liste->media[m]->mode & mode)) return(m);
	return(-1);
}
/* ========================================================================== */
Media MediaListeAdrs(TypeMediaListe *liste, char *nom, char mode) {
	int m;
	
	if(liste) for(m=0; m<liste->nb; m++) 
		if(!strcmp(nom,liste->media[m]->nom) && (liste->media[m]->mode & mode)) return(liste->media[m]);
	return(0);
}
/* ========================================================================== */
int MediaListeChemin(TypeMediaListe *liste, char *nom, char mode) {
	int m;

	if(liste) for(m=0; m<liste->nb; m++) 
		if(!strcmp(nom,liste->media[m]->nom) && (liste->media[m]->mode & mode)) return(liste->media[m]->ip.path);
	return(-1);
}
/* ========================================================================== */
void MediaListeDump(TypeMediaListe *liste, char *titre) {
	int m;

	printf("%s",titre);
	if(!liste) { printf(": non definie\n"); return; }
	printf(", %d media%s:\n",Accord1s(liste->nb));
	for(m=0; m<liste->nb; m++) { printf("  | %2d: ",m+1); MediaImprime(liste->media[m]); }
	printf("\n");
}
