#ifndef SKTLIB_H
#define SKTLIB_H

#include "environnement.h"

#ifdef OS9
#include <modes.h>
#include <types.h>
#include <inet/socket.h>
#include <inet/in.h>
#include <inet/netdb.h>
#ifndef OS9v2
#include <inet/eth.h>
#endif
#define HOST_LOCAL "me"
#else

/* #ifdef solaris
#include <sys/stat.h>
#include <sys/vnode.h>
#endif
#ifndef HPUX
#ifndef macintosh
#include <sys/mode.h>
#endif
#endif */

#ifdef CODE_WARRIOR_VSN
#include <Types.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#define HOST_LOCAL "localhost"
#endif

/* -----------------------------------------------------------------------

SocketInitxxx rempli une structure TypeSocket qui permettra
              de pointer sur une socket lointaine
socket()      cree un chemin d'entree/sortie (type int), a
              utiliser avec read/write
connect()     relie ce chemin a la structure remplie par SocketInitxxx

----------------------------------------------------------------------- */

#ifdef CODE_WARRIOR_VSN
typedef long TypeSocket;
typedef long TypeSocketIP;
typedef long TypeEthLink;
typedef long TypeHoteIP;
#else

typedef struct sockaddr TypeSocket;
typedef struct sockaddr_in TypeSocketIP;
typedef struct sockaddr_eth TypeEthLink;
typedef struct hostent TypeHoteIP;
#endif

typedef struct { TypeSocket skt; int path; } TypeIpLink;
typedef union { unsigned int val; unsigned char champ[4]; } TypeIpAdrs;
#define IP_CHAMPS(adrs) adrs.champ[0],adrs.champ[1],adrs.champ[2],adrs.champ[3]
#define HostAdrsToInt(adrs_ip) inet_addr(adrs_ip)

char *SocketErreur(void);
int SendViaIP(char *nom, char *message, int port);
#ifdef OS9
int SocketRaw(prot,bdct,dest,ethadrs)  unsigned short prot;
	TypeEthLink *bdct,*dest; unsigned char ethadrs[6];
#endif
int SocketInit(const char *nom, int port, TypeSocket *ipskt);
int SocketInitNum(int num, int port, TypeSocket *ipskt);
int SocketInitServer(struct hostent *serveur, int port, TypeSocket *ipskt);
int SocketConnect(TypeSocket *ipskt);
int SocketConnectUDP(TypeSocket *ipskt);
int SocketOpenUDP(TypeSocket *ipskt);
int SocketClient(char *nom, int port, TypeSocket *ipskt);
int SocketBase(int port,TypeSocket *ipskt);
int SocketBranchee(int s0,TypeSocket *client);
int SocketServeur(int port, TypeSocket *ipskt, TypeSocket *client);
int SocketFermee(int skt);

unsigned int HostInt(const char *nom_demande);
TypeHoteIP *HostName(const char *nom_demande, char *nom_officiel);
unsigned int HostLocal(char *nom_externe, int max, char *adrs_ip);
TypeHoteIP *SocketName(int p, char *nom);
TypeHoteIP *PeerName(int skt, char *nom);
INLINE unsigned int HostAdrsToInt(const char *adrs_ip);
TypeHoteIP *HostFromInt(int adrs);
TypeSocketIP *SocketCreateFromInt(int adrs, int port);
TypeSocketIP *SocketCreateFromHost(TypeHoteIP *serveur, int port);
TypeSocketIP *SocketCreateFromName(const char *nom, int port);
int SocketTCPServeur(int port);
int SocketTCPBranchee(int p0);
int SocketTCPClientFromCreated(TypeSocketIP *ipskt);
int SocketTCPClientFromInt(int adrs, int port);
int SocketTCPClientFromName(const char *nom, int port);
int SocketUDP(int port);

#endif
