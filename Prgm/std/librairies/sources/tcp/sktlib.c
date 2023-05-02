#include "environnement.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "sktlib.h"

static char SocketMessage[256];
/* ======================================================================= */
// #define SocketGenereMsg perror
static void SocketGenereMsg(char *origine) {
	sprintf(SocketMessage,"%s: %s [%d]",origine,strerror(errno),errno);
}
/* ======================================================================= */
char *SocketErreur(void) { return(SocketMessage); }
/* ======================================================================= */
int SendViaIP(char *nom, char *message, int port) {
	size_t l; int p;
	TypeSocket skt;

	if((p = SocketClient(nom,port,&skt)) == -1) return(0);
	l = strlen(message);
	send(p,message,l,0);
	SocketFermee(p);
	return(1);
}
#ifdef OS9
/* ========================================================================== */
int SocketRaw(unsigned short prot, TypeEthLink *bdct, TypeEthLink *dest,
			  unsigned char ethadrs[6]) {
/* usage:
  p = SocketRaw(protocole,&bdctadrs,[&destadrs | 0],ethadrs);
  sendto(p,&buffer,lngr,0,&destadrs,sizeof(destadrs));
  recvfrom(p,&buffer,lngr,0,&fromadrs,&lngadrs);
*/
	int p,i,j;

	SocketMessage[0] = '\0';
	if((p = socket(AF_ETHER,SOCK_RAW,0)) == -1) {
		SocketGenereMsg("SocketRaw/socket"); return(-1);
	}
	bdct->sen_family = AF_ETHER;
	bdct->sen_type = prot;
	for(i=0; i<6; i++) bdct->sen_addr.ena_addr[i] = 0xFF;
	if(bind(p,bdct,sizeof(TypeEthLink)) == -1) {
		SocketGenereMsg("SocketRaw/bind"); return(-1);
	}
	if(dest) {
		dest->sen_family = AF_ETHER;
		dest->sen_type = prot;
		for(i=0; i<6; i++) dest->sen_addr.ena_addr[i] = ethadrs[i];
	}
	return(p);
}
#endif
/* ========================================================================== */
int SocketInit(const char *nom, int port, TypeSocket *skt) {
	TypeHoteIP *serveur;
	TypeSocketIP *ipskt;

	SocketMessage[0] = '\0';
	serveur = HostName(nom,0); if(!serveur) return(0);

	ipskt = (TypeSocketIP *)skt;
	ipskt->sin_family = AF_INET;
	if(port) ipskt->sin_port = htons(port); else ipskt->sin_port = 0;
//	memcpy(&(ipskt->sin_addr),serveur->h_addr,serveur->h_length);
	ipskt->sin_addr.s_addr = *(in_addr_t *)serveur->h_addr_list[0];
	return(1);
}
/* ========================================================================== */
int SocketInitNum(int num, int port, TypeSocket *skt) {
	TypeSocketIP *ipskt;

	SocketMessage[0] = '\0';
	ipskt = (TypeSocketIP *)skt;
	ipskt->sin_family = AF_INET;
	if(port) ipskt->sin_port = htons(port); else ipskt->sin_port = 0;
	ipskt->sin_addr.s_addr = num;
	return(1);
}
/* ========================================================================== */
int SocketInitServer(struct hostent *serveur, int port, TypeSocket *skt) {
	TypeSocketIP *ipskt;

	SocketMessage[0] = '\0';
	ipskt = (TypeSocketIP *)skt;
	ipskt->sin_family = AF_INET;
	if(port) ipskt->sin_port = htons(port); else ipskt->sin_port = 0;
//	memcpy(&(ipskt->sin_addr),serveur->h_addr,serveur->h_length);
	ipskt->sin_addr.s_addr = *(in_addr_t *)serveur->h_addr_list[0];
	return(1);
}
/* ========================================================================== */
int SocketConnect(TypeSocket *skt) {
	TypeSocketIP *ipskt; int p;

	ipskt = (TypeSocketIP *)skt;
	if(ipskt->sin_port) p = socket(ipskt->sin_family,SOCK_STREAM,IPPROTO_IP);
	else p = socket(ipskt->sin_family,SOCK_RAW,0);
	if(p == -1) SocketGenereMsg("SocketConnect/socket");
	else if(connect(p,skt,sizeof(TypeSocket)) == -1) {
		SocketGenereMsg("SocketConnect/connect");
		close(p);
		p = -1;
	}
	return(p);
}
/* ========================================================================== */
int SocketConnectUDP(TypeSocket *skt) {
	TypeSocketIP *ipskt; int p;

	ipskt = (TypeSocketIP *)skt;
	if(ipskt->sin_port) p = socket(ipskt->sin_family,SOCK_DGRAM,IPPROTO_UDP);
	else p = -1;
	if(p == -1) SocketGenereMsg("SocketConnect/socket");
	else if(connect(p,skt,sizeof(TypeSocket)) == -1) {
		SocketGenereMsg("SocketConnect/connect");
		close(p);
		p = -1;
	}
	return(p);
}
/* ========================================================================== */
int SocketOpenUDP(TypeSocket *skt) {
	TypeSocketIP *ipskt; int p;

	ipskt = (TypeSocketIP *)skt;
	if(ipskt->sin_port) p = socket(ipskt->sin_family,SOCK_DGRAM,IPPROTO_UDP);
	else p = -1;
	return(p);
}
/* ========================================================================== */
int SocketClient(char *nom, int port, TypeSocket *skt) {
	if(SocketInit(nom,port,skt) > 0) return(SocketConnect(skt));
	else return(-1);
}
/* ========================================================================== */
int SocketBase(int port, TypeSocket *skt) {
	TypeSocketIP *ipskt; int s0; char optn;

	if((s0 = socket(AF_INET,SOCK_STREAM,IPPROTO_IP)) == -1) {
		SocketGenereMsg("SocketBase/socket"); return(-1);
	}

	ipskt = (TypeSocketIP *)skt;
	memset(ipskt,0,sizeof(TypeSocketIP));
	ipskt->sin_family = AF_INET;
	if(port) ipskt->sin_port = htons(port); else ipskt->sin_port = 0;
	ipskt->sin_addr.s_addr = htonl(INADDR_ANY);
	optn = 1;
	setsockopt(s0,SOL_SOCKET,SO_REUSEADDR|SO_KEEPALIVE,(char *)&optn,sizeof(char));
	if(bind(s0,skt,sizeof(TypeSocket)) == -1) {
		SocketGenereMsg("SocketBase/bind"); return(-1);
	}
	if(listen(s0,1 /* SOMAXCONN */ ) == -1) { SocketGenereMsg("SocketBase/listen"); return(-1); }
	return(s0);
}
/* ========================================================================== */
int SocketBranchee(int s0, TypeSocket *client) {
	int p; unsigned int lngr;

	lngr = sizeof(TypeSocket);
	if((p = accept(s0,client,&lngr)) == -1) SocketGenereMsg("SocketBranchee/accept");
	return(p);
}
/* ========================================================================== */
int SocketServeur(int port, TypeSocket *skt, TypeSocket *client) {
	int s0,p;

	if((s0 = SocketBase(port,skt)) == -1) return(-1);
	p = SocketBranchee(s0,client);
	close(s0);
	return(p);
}
/* ========================================================================== */
int SocketFermee(int p) {
	int rc;

	rc = 1;
	if(shutdown(p,SHUT_RDWR) == -1) { SocketGenereMsg("SocketFermee/shutdown"); rc = -1; }
	if(p >= 0) {
		if(close(p) == -1) { SocketGenereMsg("SocketFermee/close"); rc = -1; }
	}
	return(rc);
}
/* ========================================================================== */
unsigned int HostInt(const char *nom_demande) {
	TypeHoteIP *serveur;
	struct in_addr ipadr;
	in_addr_t adrs;

	if(!strcmp(nom_demande,".")) return(0x0100007F);
	if(!(serveur = gethostbyname(nom_demande))) {
		if((ipadr.s_addr = inet_addr(nom_demande)) != -1)
			serveur = gethostbyaddr((char *)&ipadr,sizeof(ipadr),AF_INET);
		if(!serveur) {
			sprintf(SocketMessage,"HostInt: machine inconnue (%s)\n",nom_demande);
			return(0);
		}
	}
	adrs = *(in_addr_t *)serveur->h_addr_list[0];
	return(adrs);
}
/* ========================================================================== */
TypeHoteIP *HostName(const char *nom_demande, char *nom_officiel) {
	TypeHoteIP *serveur;
	struct in_addr ipadr;

/*	inet_addr != -1 des que 1er caractere du nom = hexadecimal (a-f) !!!
	if((ipadr.s_addr = inet_addr(nom)) == -1) serveur = gethostbyname(nom);
	else serveur = gethostbyaddr(&ipadr,sizeof(ipadr),AF_INET);
	if(!serveur) {
		prerr(0,_errmsg(0,"Machine inconnue: %s\n",nom)); return(-1);
	} */

	if(!(serveur = gethostbyname(nom_demande))) {
		if((ipadr.s_addr = inet_addr(nom_demande)) != -1)
			serveur = gethostbyaddr((char *)&ipadr,sizeof(ipadr),AF_INET);
		if(!serveur) {
			sprintf(SocketMessage,"HostName: machine inconnue (%s)\n",nom_demande);
			return(0);
		}
	}
	if(nom_officiel) strcpy(nom_officiel,serveur->h_name);
	return(serveur);
}
/* ========================================================================= */
unsigned int HostLocal(char *nom_externe, int max, char *adrs_ip) {
	//	union { unsigned int val; unsigned char champ[4]; } adrs;
	TypeIpAdrs adrs;

	gethostname(nom_externe,max);
    HostName(nom_externe,nom_externe);
	adrs.val = HostInt(nom_externe); // inet_addr(nom_externe);
	if((int)(adrs.val) == -1) { sprintf(adrs_ip,"<AdrsIpInconnue>"); return(0); }
	else {
		if(adrs.val == 0) adrs.val = 0x0100007F;
		sprintf(adrs_ip,"%d.%d.%d.%d",IP_CHAMPS(adrs));
	}
	return(adrs.val);
}
/* ========================================================================== */
TypeHoteIP *SocketName(int p, char *nom) {
	TypeSocket rem_skt; TypeSocketIP *ip_skt;
	TypeHoteIP *h;
	unsigned int lngr;

	h = 0;
	lngr = sizeof(rem_skt);
	if(getsockname(p,&rem_skt,&lngr)) strcpy(nom,"xxxxxx");
	else {
		ip_skt = (TypeSocketIP *)&rem_skt;
		h = gethostbyaddr((char *)&(ip_skt->sin_addr),sizeof(struct in_addr),AF_INET);
		if(h) strcpy(nom,h->h_name);
		else strcpy(nom,inet_ntoa(ip_skt->sin_addr));
	}
	return(h);
}
/* ========================================================================== */
TypeHoteIP *PeerName(int p, char *nom) {
	TypeSocket rem_skt; TypeSocketIP *ip_skt;
	TypeHoteIP *h;
	unsigned int lngr;

	h = 0;
	lngr = sizeof(rem_skt);
	if(getpeername(p,&rem_skt,&lngr)) strcpy(nom,"xxxxxx");
	else {
		ip_skt = (TypeSocketIP *)&rem_skt;
		h = gethostbyaddr((char *)&(ip_skt->sin_addr),sizeof(struct in_addr),AF_INET);
		if(h) strcpy(nom,h->h_name);
		else strcpy(nom,inet_ntoa(ip_skt->sin_addr));
	}
	return(h);
}
/* ========================================================================== */
// sscanf(adrs_ip,"%d.%d.%d.%d",&adrs->champ[0],&adrs->champ[1],&adrs->champ[2],&adrs->champ[3]);
// INLINE unsigned int HostAdrsToInt(const char *adrs_ip) { return(inet_addr(adrs_ip)); }
/* ========================================================================== */
TypeHoteIP *HostFromInt(int ipval) {
	TypeHoteIP *serveur;
	struct in_addr ipadr;

	ipadr.s_addr = ipval;
	serveur = gethostbyaddr((char *)&ipadr,sizeof(ipadr),AF_INET);
	if(!serveur) {
		TypeIpAdrs adrs;
		adrs.val = ipval;
		sprintf(SocketMessage,"HostFromInt: machine inconnue (%d.%d.%d.%d)\n",IP_CHAMPS(adrs));
//			(ipval>>24)&0xFF,(ipval>>16)&0xFF,(ipval>>8)&0xFF,ipval&0xFF);
		return(0);
	}
	return(serveur);
}
/* ========================================================================== */
TypeSocketIP *SocketCreateFromInt(int adrs, int port) {
	TypeSocketIP *ipskt;

	ipskt = (TypeSocketIP *)malloc(sizeof(TypeSocketIP));
	ipskt->sin_family = AF_INET;
	if(port) ipskt->sin_port = htons(port); else ipskt->sin_port = 0;
	ipskt->sin_addr.s_addr = adrs;
	return(ipskt);
}
/* ========================================================================== */
TypeSocketIP *SocketCreateFromHost(TypeHoteIP *serveur, int port) {
	in_addr_t adrs;

	adrs = *(in_addr_t *)serveur->h_addr_list[0];
	return(SocketCreateFromInt(adrs,port));
}
/* ========================================================================== */
TypeSocketIP *SocketCreateFromName(const char *nom, int port) {
	TypeHoteIP *serveur;

	serveur = HostName(nom,0); if(!serveur) return(0);
	return(SocketCreateFromHost(serveur,port));
}
/* ========================================================================== */
int SocketTCPServeur(int port) {
	int p; TypeSocketIP *ipskt;

	if(port) p = socket(AF_INET,SOCK_STREAM,IPPROTO_IP);
	else p = socket(AF_INET,SOCK_RAW,0);
	if(p == -1) { SocketGenereMsg("SocketTCPServeur/socket"); return(-1); }

	if((ipskt = SocketCreateFromInt(INADDR_ANY,port)) == 0) {
		SocketGenereMsg("SocketTCPServeur/create"); return(-1);
	}
	if(bind(p,(TypeSocket *)ipskt,sizeof(TypeSocketIP)) == -1) {
		SocketGenereMsg("SocketTCPServeur/bind"); return(-1);
	}

	if(listen(p,1) == -1) { SocketGenereMsg("SocketTCPServeur/listen"); return(-1); }
	return(p);
}
/* ========================================================================== */
int SocketTCPBranchee(int p0) {
	/* contenu de ipskt retourne par accept() */
	int p; unsigned int lngr;
	TypeSocket client;

	lngr = sizeof(TypeSocket);
	if((p = accept(p0,&client,&lngr)) == -1) SocketGenereMsg("SocketTCPBranchee/accept");
	return(p);
}
/* ========================================================================== */
int SocketTCPClientFromCreated(TypeSocketIP *ipskt) {
	int p;

	if(ipskt->sin_port) p = socket(AF_INET,SOCK_STREAM,IPPROTO_IP);
	else p = socket(AF_INET,SOCK_RAW,0);
	/* {
		int dim_send,dim_recv,sans_delai;
		dim_send = 16384; dim_recv = 65536; dim_recv = 1;
		setsockopt(p,SOL_SOCKET,SO_SNDBUF,(char *)&dim_send,sizeof(int));
		setsockopt(p,SOL_SOCKET,SO_RCVBUF,(char *)&dim_recv,sizeof(int));
		setsockopt(p,IPPROTO_TCP,TCP_NODELAY,(char *)&sans_delai,sizeof(actif));
	 } */
	if(p == -1) SocketGenereMsg("SocketTCPClientFromCreated/socket");
	else if(connect(p,(TypeSocket *)ipskt,sizeof(TypeSocketIP)) == -1) {
		SocketGenereMsg("SocketTCPClientFromCreated/connect");
		close(p);
		p = -1;
	}
	return(p);
}
/* ========================================================================== */
int SocketTCPClientFromInt(int adrs, int port) {
	TypeSocketIP *ipskt;

	if((ipskt = SocketCreateFromInt(adrs,port)) == 0) {
		SocketGenereMsg("SocketTCPClientFromInt/SocketCreateFromInt"); return(-1);
	}
	return(SocketTCPClientFromCreated(ipskt));
}
/* ========================================================================== */
int SocketTCPClientFromName(const char *nom, int port) {
	TypeSocketIP *ipskt;

	if((ipskt = SocketCreateFromName(nom,port)) == 0) {
		SocketGenereMsg("SocketTCPClientFromName/SocketCreateFromName"); return(-1);
	}
	return(SocketTCPClientFromCreated(ipskt));
}
/* ========================================================================== */
int SocketUDP(int port) {
	int p; TypeSocketIP *ipskt;

	if(port) p = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	else p = -1;
	if(p == -1) { SocketGenereMsg("SocketUDP/socket"); return(-1); }

	if((ipskt = SocketCreateFromInt(INADDR_ANY,port)) == 0) {
		SocketGenereMsg("SocketUDP/create"); return(-1);
	}

	if(bind(p,(TypeSocket *)ipskt,sizeof(TypeSocketIP)) == -1) {
		SocketGenereMsg("SocketUDP/connect");
		close(p); p = -1;
	}
	return(p);
}
