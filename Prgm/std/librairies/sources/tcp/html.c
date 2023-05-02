/*
 *  html.c
 *  Librairies
 *
 *  Created by Michel GROS on 22.02.12.
 *  Copyright 2012 CEA/DSM/DAPNIA. All rights reserved.
 *
 */

#define HTML_C

#include <stdio.h>
#include <unistd.h> // pour getpid()
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#include "html.h"
#include "decode.h"
#define Accord2s(var) var,(var>1)?"s":"",(var>1)?"s":""

static char SvrPereUnique = 1;
// static int SvrTol = 8;
static char SvrPPC = 0;

#define HTMLPAGEMAX HTMLHDRMAX+HTMLREPMAX
static char HttpReponse[HTMLREPMAX];
static int  HttpRepDim;
static char HttpPageTotale[HTMLPAGEMAX];
static int  HttpPageDim;
static int  HttpRefreshDelay;
static char HttpActivated;
static char HttpContinue;

/* ========================================================================== */
int HttpEcrit(char *fmt, ...) {
	va_list args; int lngr;

	va_start(args,fmt); lngr = vsnprintf(HttpReponse+HttpRepDim,HTMLREPMAX-HttpRepDim,fmt,args); va_end(args);
	HttpRepDim += lngr;
	return(lngr);
}
/* ========================================================================== */
int HttpPageEcrit(char *fmt, ...) {
	va_list args; int lngr;

	va_start(args,fmt); lngr = vsnprintf(HttpPageTotale+HttpPageDim,HTMLPAGEMAX-HttpPageDim,fmt,args); va_end(args);
	HttpPageDim += lngr;
	return(lngr);
}
/* ========================================================================== */
static void HttpRepReset(void) { HttpRepDim = 0; }
/* ========================================================================== */
static void HttpPageReset(void) { HttpPageDim = 0; }
/* ========================================================================== */
static void HttpPageIntro() {
	HttpPageEcrit("HTTP/1.1 1\n");
	//1 HttpPageEcrit("Content-type: text/html;charset=ISO-8859-1\n");
	HttpPageEcrit("Content-Type: text/html\n");
	HttpPageEcrit("Content-Length: %d\n",HttpRepDim);
	HttpPageEcrit("Connection: %s\n",HttpTermine? "close": "keep-alive");
	HttpPageEcrit("Cache-Control: no-cache\n");
	HttpPageEcrit("\n");
}
/* ========================================================================= */
void HttpPageDebut(char *titre,int couleur) {
	if(SvrDebug) printf("[%d] Delai de refresh auto: %d seconde%s\n",getpid(),Accord1s(HttpRefreshDelay));
	HttpRepReset();
	HttpEcrit("<html>\n");
	HttpEcrit("<head>");
	if(titre && (titre[0] != '\0')) HttpEcrit("<title>%s</title>",titre);
	if(HttpRefreshDelay > 0) {
	#ifdef UTILISE_JAVA
		HttpEcrit("<script>\n");
		HttpEcrit("	setTimeout(function() {\n");
		HttpEcrit("		location.reload();\n");
		HttpEcrit("	},%s);\n",HttpRefreshDelay*1000);
		HttpEcrit("</script>\n");
	#else
		HttpEcrit("<meta http-equiv=\"refresh\" content=\"%d\" />\n",HttpRefreshDelay);
	#endif
	}
	HttpEcrit("</head>\n");
	HttpEcrit("<body bgcolor='#%06X'>\n",couleur);
	HttpEcrit("<font face='Courier' size=2>\n\n");
}
/* ========================================================================= */
void HttpPageFin(void) {
	/* termine la reponse d'abord, afin d'avoir HttpRepDim total dans Content-Length */
	HttpEcrit("\n</font>\n");
	HttpEcrit("</body>\n");
	HttpEcrit("</html>\n");
	HttpReponse[HttpRepDim] = '\0';
	/* maintenant, on peut remplir la page a envoyer */
	HttpPageReset();
	HttpPageIntro();
	memcpy(HttpPageTotale+HttpPageDim,HttpReponse,HTMLPAGEMAX-HttpPageDim);
	HttpPageDim += HttpRepDim; HttpPageTotale[HttpPageDim] = '\0';
}
/* ========================================================================== */
static int HttpPageEnvoi(int skt) {
	// if(SvrDebug) printf("[%d] <<<<<<<<<< envoi de la page[%d] via <%d>:\n%s[%d] >>>>>>>>>> fin de la page a envoyer\n",
	//	getpid(),HttpPageDim,skt,HttpPageTotale,getpid());
	if(SvrDebug) printf("[%d] <<<<<<<<<< envoi d'une page[%d] via <%d> >>>>>>>>>>\n",getpid(),HttpPageDim,skt);
	if(skt >= 0) return((int)send(skt,HttpPageTotale,HttpPageDim,0)); // MSG_DONTROUTE
	else return(0);
}
/* ========================================================================== */
void HttpFermeInstance() { HttpTermine = 1; }
/* ========================================================================== */
void HttpRefreshAuto(int secs) {
	HttpRefreshDelay = secs;
	if(SvrDebug) printf("[%d] Delai de refresh demande: %d seconde%s\n",getpid(),Accord1s(HttpRefreshDelay));
}
/* ========================================================================== */
int HttpServeur(int port) {
	char page_retour;
	char nom[80]; char *r,*s,*d,*v;
	int base,rc,nb,l;
	TypeSocket rem_skt;

	printf("---------------------------------- [%d] Daemon lance sur le port #%d\n",getpid(),port);
	SvrPPC = EndianIsBig();
	base = SocketBase(port,&HttpSocket);
	if(base == -1) {
		printf("---------------------------------- [%d] Daemon plante (port #%d)\n",getpid(),port);
		printf("                                   (%d) %s\n",errno,strerror(errno));
		exit(errno);
	}
	// SvrDebug = 0;
    SvrMulti = 1; SvrFils = 0;
    SvrPereUnique = 0;
	HttpSkt = -1;
	HttpActivated = 0;
	HttpTermine = 0;
	HttpRefreshDelay = 0;
	HttpClientType = HTTP_CLIENT_ORDI;
	SvrPrecedente = HttpSkt;
	if(!SvrActive()) {
		printf("---------------------------------- [%d] Daemon elimine (port #%d)\n",getpid(),port);
		exit(ECONNRESET);
	}
	HttpActivated = 1;

	while(!HttpTermine) {
		char type_connu;
		HttpSkt = SocketBranchee(base,&rem_skt);
		SvrPrecedente = HttpSkt;
		if(HttpSkt == -1) { sleep(1); continue; }
		SocketName(HttpSkt,HttpServeurNom);
		PeerName(HttpSkt,nom);
        if(SvrMulti) {
            if(SvrFils == 0) {
                SvrFils = SvrFork(base);
                /* SvrFils = 0:  je suis le fils;
				 SvrFils > 0:  je suis parent, ceci est le pid du fils;
				 SvrFils = -1: je suis egalement le parent, mais pas de fils cree (erreur <errno> */
                if(SvrFils > 0) {
					if(SvrDebug) {
						printf("\n[%d] Socket #%d branchee sur le client %s:%d, prochain serveur: [%d]\n",getpid(),HttpSkt,nom,port,SvrFils);
						printf("[%d] Reference serveur = %s:%d\n",getpid(),HttpServeurNom,port);
					}
					HttpMajorLue = 0; HttpMinorLue = 9;
                    if(SvrPereUnique) continue;
                } else if(SvrFils < 0) {
                    printf("\n[%d] Socket #%d branchee sur le client %s, mais prochain serveur en erreur (retour %d -> %d: %s)\n",getpid(),HttpSkt,nom,SvrFils,errno,strerror(errno));
                    if(SvrPereUnique) continue;
                } else {
                    if(SvrDebug) printf("---------------------------------- Nouveau serveur [%d] lance on %s:%d\n",getpid(),HttpServeurNom,port);
                    // SocketFermee(HttpSkt); // ce n'est pas au fils de la fermer tout de suite? non, a la fin!
                    if(!SvrPereUnique) continue;
                }
            }
        } else SvrFils = -1;
		type_connu = 0;
		HttpContinue = 1;
		do {
			page_retour = (HttpRefreshDelay > 0);
			rc = (int)recv(HttpSkt,HttpPageRecue,HTMLREPMAX,0);
			if(rc < 0) {
				printf("[%d] lecture de la requete en erreur (retour %d -> %d: %s)\n",getpid(),rc,errno,strerror(errno));
				break; // sleep(1); continue;
			}
			if(rc > 0) {
				if(SvrDebug) printf("[%d] .................... %d octet%s recu%s via <%d>\n",getpid(),Accord2s(rc),HttpSkt);
				HttpPageRecue[HTMLREPMAX-1] = '\0';
				if(SvrDebug > 1) printf("[%d] <<<<<<<<<< recu (%d) via <%d>:\n[%d] %s\n[%d] >>>>>>>>>> fin de la page recue\n",
					getpid(),rc,HttpSkt,getpid(),HttpPageRecue,getpid());
				r = HttpPageRecue;
				do {
					if(SvrDebug > 3) printf("{ %4d } %s\n",(int)(r-HttpPageRecue),r);
					if(!strncmp(r,"GET /",5)) {
						r += 5; d = r;
						s = r;
						while((*r != ' ') && (*r != '\0')) r++;
						if(*r != '\0') {
							if(s == r) s++; *r++ = '\0';
							if(!strcmp(r,"HTTP/")) {
								r += 5; v = r;
								while((*v != '.') && (*v != '\0')) v++;
								if(*v != '\0') *v++ = '\0';
								sscanf(r,"%d",&HttpMajorLue);
								sscanf(v,"%d",&HttpMinorLue);
								s = r;
							} else if(*r) s = r;
						}
						r = d; l = 0;
						while((*r != '?') && (*r != '\0') && (l < HTMLREQTMAX)) { r++; l++; }
						if(*r == '?') { if(s == r) s++; *r++ = '\0'; }
						strncpy(HttpUrlDemandee,d,HTMLREQTMAX); HttpUrlDemandee[HTMLREQTMAX-1] = '\0';
 						strncpy(HttpReqtRecue,r,HTMLREQTMAX); HttpReqtRecue[HTMLREQTMAX-1] = '\0';
						if(SvrDebug > 1) {
							printf("[%d] <<<<<<<<<< variables via <%d>:",getpid(),HttpSkt);
							if(HttpReqtRecue[0]) {
								printf("\n[%d] %s",getpid(),HttpReqtRecue);
								if(HttpReqtRecue[strlen(HttpReqtRecue)-1] != '\n') printf("\n");
								printf("[%d] >>>>>>>>>> fin des variables lues\n",getpid());
							} else printf(" neant >>>>>>>>>>\n");
						}
						r = s;
						if(SvrDebug > 3) printf("> %4d < %s\n",(int)(r-HttpPageRecue),r);
						while((*r != '\n') && (*r != '\r') && (*r != '\0')) r++;
					} else if(!strncmp(r,"User-Agent: ",12)) {
						r += 12; v = r;
						while((*v != '\n') && (*v != '\0')) v++;
						if(*v == '\n') *v++ = '\0';
						strncpy(HttpDemandeur,r,HTMLREQTMAX); HttpUrlDemandee[HTMLREQTMAX-1] = '\0';
						if(SvrDebug > 1) printf("[%d] <<<<<<<<<< demandeur via <%d>:\n[%d] %s\n[%d] >>>>>>>>>>\n",
							getpid(),HttpSkt,getpid(),HttpDemandeur,getpid());
						r = v;
					}
					if(*r) r++;
				} while(*r);
				if(SvrDebug) printf("[%d] Fin de la recuperation de la requete\n",getpid());
				if(!type_connu) { HttpClientType = strstr(HttpDemandeur,"iPhone")? HTTP_CLIENT_IPHONE: HTTP_CLIENT_ORDI; type_connu = 1; }
				page_retour = (HttpTraiteRequete(HttpUrlDemandee,HttpDemandeur,HttpReqtRecue) > 0);
				if(!page_retour) { if(SvrDebug) printf("[%d] .................... Traitement de la requete en erreur, AUCUN octet envoye via <%d>\n",getpid(),HttpSkt); }
				//-- if(SvrFils > 0) HttpContinue = 0;
			} // else if(SvrDebug > 2) printf("[%d] .................... Lecture vide\n",getpid()); cas si pas d'action du cote client
			if(page_retour) {
				if(rc == 0) { if(SvrDebug) printf("[%d] Pas d'action client, mais refresh auto (%d seconde%s)\n",getpid(),Accord1s(HttpRefreshDelay)); }
				if(HttpPageRetour()) {
					nb = HttpPageEnvoi(HttpSkt);
					if(SvrDebug) printf("[%d] .................... %d octet%s envoye%s via <%d>\n",getpid(),Accord2s(nb),HttpSkt);
				} else if(SvrDebug) printf("[%d] .................... Pas de reponse prevue, AUCUN octet envoye via <%d>\n",getpid(),HttpSkt);
			}
			// if(HttpRefreshDelay > 0) sleep(HttpRefreshDelay); else
			sleep(1);
		} while(HttpContinue && !HttpTermine);
		SocketFermee(HttpSkt);
		if(SvrDebug) printf("[%d] Socket #%d sur %s debranchee\n",getpid(),HttpSkt,nom);
		// if((SvrFils == 0) && SvrPereUnique) return(0);
		HttpSkt = -1; // ?
		if(SvrFils > 0) HttpTermine = 1;
	};
	if(HttpActivated) { SvrExited(); HttpActivated = 0; }
	//	if(SvrPrecedente >= 0) SocketFermee(SvrPrecedente);
	if(SvrDebug) printf("---------------------------------- [%d] Daemon stopped on port #%d\n",getpid(),port);
	fflush(stdout);
	close(base);
//	if(SvrFils > 0) {
//		if(SvrDebug) printf("---------------------------------- [%d] Server stopped on port #%d\n",SvrFils,port);
//		kill(SvrFils,SIGSTOP);
//	}

	return(1);
}
