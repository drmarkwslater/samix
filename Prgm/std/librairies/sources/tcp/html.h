/*
 *  html.h
 *  Librairies
 *
 *  Created by Michel GROS on 22.02.12.
 *  Copyright 2012 CEA/DSM/DAPNIA. All rights reserved.
 *
 */

#ifndef HTML_H
#define HTML_H

#ifdef HTML_C
	#define HTML_VAR
#else
	#define HTML_VAR extern
#endif

#include "serveur.h"

#define HTMLHDRMAX 256
#define HTMLREPMAX 65536
#define HTMLREQTMAX 256
#define HTMLLIGNEMAX 1024

typedef enum {
	HTTP_CLIENT_ORDI = 0,
	HTTP_CLIENT_IPHONE,
	HTTP_CLIENT_NB
} HTTP_CLIENT;

HTML_VAR int HttpSkt;
HTML_VAR TypeSocket HttpSocket;
HTML_VAR char HttpPageRecue[HTMLREPMAX];
HTML_VAR char HttpUrlDemandee[HTMLREQTMAX];
HTML_VAR char HttpDemandeur[HTMLREQTMAX];
HTML_VAR char HttpReqtRecue[HTMLREQTMAX];
HTML_VAR char HttpClientType;
HTML_VAR int  HttpMajorLue,HttpMinorLue;
HTML_VAR char HttpServeurNom[256];
HTML_VAR char HttpTermine;

void HttpPageDebut(char *titre,int couleur);
int  HttpEcrit(char *fmt, ...);
void HttpPageFin(void);
void HttpRefreshAuto(int secs);
void HttpFermeInstance(void);
// int  HttpRepond(char *requete);
int  HttpServeur(int port);

#ifdef PAS_COMME_CA
typedef struct {
	char adrs_srce[80];
	char adrs_dest[80];
	int  port_dest;
	short max_requete,max_recu;
	char *requete;
	char *page_recue;
} TypeHttpClient;
int  HttpServeurUnique(int port, int maxusers, int maxidle);
int  HttpRepondClient(TypeHttpClient *client);
#endif /* PAS_COMME_CA */

int HttpTraiteRequete(char *url, char *demandeur, char *requete);
int HttpRepond(char *requete);
int HttpPageEcrit(char *fmt, ...);
int HttpPageRetour();

#endif
