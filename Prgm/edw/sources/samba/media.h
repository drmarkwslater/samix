/*
 *  Created by Michel GROS on 07.10.14.
 *  Copyright 2014 CEA/DSM/IRFU. All rights reserved.
 */

#ifndef MEDIA_H
#define MEDIA_H

#ifdef MEDIA_C
#define MEDIA_VAR
#else
#define MEDIA_VAR extern
#endif

#include <environnement.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <defineVAR.h>
#include <utils/claps.h>
#include <tcp/sktlib.h>

/*
#define FMT_MAXELTS 16

typedef enum {
	FMT_VAL = 0,
	FMT_NOM
} FMT_MODE;
typedef enum {
	FMT_HEXA = 0,
	FMT_INT,
	FMT_FLOAT,
	FMT_TEXTE,
	FMT_NBTYPES
} FMT_TYPE;

MEDIA_VAR char *FmtNomType[FMT_NBTYPES+1]
#ifdef MEDIA_C
 = { "hexa", "int", "float", "texte", "\0" }
#endif
;
MEDIA_VAR char FmtNomCles[24]
#ifdef MEDIA_C
 = "hexa/int/float/texte"
#endif
;
typedef struct {
	char mode;
	char lngr;
	char type;
	union {
		int64 valeur;
		char nom[16];
	} c;
} TypeFmtElt;
MEDIA_VAR TypeFmtElt FmtLu[FMT_MAXELTS];
MEDIA_VAR TypeFmtElt FmtEltLu;
MEDIA_VAR int FmtEltNbLus;

MEDIA_VAR ArgDesc FmtEltDesc[]
#ifdef MEDIA_C
 = {
	{ 0,      DESC_KEY FmtNomCles, &FmtEltLu.type, 0 },
	{ 0,      DESC_STR(16)          FmtEltLu.c.nom, 0 },
	{ "lngr", DESC_OCTET           &FmtEltLu.lngr, 0 },
	DESC_END
}
#endif
;
*/

/*
# BB
commande = binaire, format = (
	{ hexa := ident, lngr=8 },
	{ hexa := adresse, lngr=8 },
	{ hexa := donnee, lngr=16 }
}
#CALI
commande = texte, format = (
	{ valeur := "w ", lngr=0 },
	{ hexa := adresse, lngr=0 },
	{ valeur := " ", lngr=0 },
	{ hexa := donnee, lngr=0 } 
)
# SY127
commande = texte, format = (
	{ valeur := "1BBA", lngr=0 },
	{ texte := nom, lngr=0 },
	{ valeur := "\rC", lngr=0 },
	{ int := donnee, lngr=0 ,
	{ valeur := "\r1", lngr=0 },
)
# OPERA
 commande = binaire, format = (
	{ valeur := 0x57F0, lngr=16 },
	{ hexa := donnee, lngr=32 }
)
# OPERA
 commande = binaire, format = (
	{ texte := W, lngr=8 },
	{ valeur := 0xF0, lngr=8 },
	{ hexa := donnee, lngr=32 }
)
# Serie
commande = texte, format = (
	{ texte := donnee, lngr=0 }
 )
 */


#define MEDIA_MAXNOM 16
#define MEDIA_MAXPORT 64
#define MEDIA_MAXCLESNOMS 64
typedef enum {
	MEDIA_USB = 0,
	MEDIA_TCP_WR,
	MEDIA_UDP_WR,
	MEDIA_TCP_RD,
	MEDIA_UDP_RD,
	MEDIA_NBTYPES
} MEDIA_TYPE;

MEDIA_VAR char *MediaNomType[MEDIA_NBTYPES+1]
#ifdef MEDIA_C
 = { "USB", "TCP:Wr", "UDP:Wr", "TCP:Rd", "UDP:Rd", "\0" }
#endif
;
MEDIA_VAR char MediaClesType[MEDIA_MAXCLESNOMS]
#ifdef MEDIA_C
 = "USB/TCP:Wr/UDP:Wr/TCP:Rd/UDP:Rd"
#endif
;

typedef enum {
	MEDIA_RD = 0x01,
	MEDIA_WR = 0x02,
	MEDIA_RDWR = 0x03 // (MEDIA_RD | MEDIA_WR)
} MEDIA_MODE;

typedef struct {
	char nom[MEDIA_MAXNOM];
	char adrs[MEDIA_MAXPORT];
	char type; char mode;
	int port;       // media IP, essentiellement
	TypeIpLink ip;  // <path> utilise aussi par media non-IP
} TypeMedia,*Media;

typedef struct {
	Media *media;
	int max,nb;
} TypeMediaListe;

Media MediaCreate(char *nom, char type, char *adrs);
void  MediaDefini(Media media, char *nom, char *port, char type);
int   MediaConnecte(Media media);
char  MediaLit(char *nom_complet, char cree);
void  MediaSauve(char *nom_complet);

int   MediaListeInit(TypeMediaListe *liste, int max);
int   MediaListeAjouteDef(TypeMediaListe *liste, char *nom, char *adrs, char type);
int   MediaListeAjouteRef(TypeMediaListe *liste, Media media);
int   MediaListeAjouteLus(TypeMediaListe *liste);
int   MediaListeDim(TypeMediaListe *liste);
Media MediaListeNum(TypeMediaListe *liste, int num);
int   MediaListeIndex(TypeMediaListe *liste, char *nom, char mode);
Media MediaListeAdrs(TypeMediaListe *liste, char *nom, char mode);
int   MediaListeChemin(TypeMediaListe *liste, char *nom, char mode);
void  MediaListeDump(TypeMediaListe *liste, char *titre);

#endif /* MEDIA_H */
