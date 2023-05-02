/*   Librairie pour construire des clients/serveur simples
     =====================================================
     
Cote serveur:
------------

	. inclure ce fichier
	. declarer
			extern TypeSvrBox SvrLink;
	. le programme principal, une fois ses initialisations faites, appelle:
			SvrRun(port);
		(c'est tout)
		Cette routine se charge de receptionner les elements de messages
		client, les re-assemble, verifie leur numerotation, agrandi le
		buffer de reception si necessaire, assure le traitement des erreurs.
		Quand un message est complet, il appelle SvrUser(), qui est a la
		charge du programmeur (qui y introduit la programmation specifique de
		son serveur)
	. il faut donc aussi creer les 2 routines suivantes:
		*	SvrUser()
		qui est appelee chaque fois qu'une requete client est arrivee entiere.
		On dispose de la commande client (char *) dans SvrLink.cmde, et
		des donnees eventuellement associees, dans SvrLink.data; la longueur
		des donnees est dans SvrLink.lngr
		On repond par un accuse de reception, soit positif avec 
			SvrOK(&SvrLink,<donnees>,<longueur>);
		(<donnees> vaut 0 si il n'y a pas de donnees a retransmettre)
		soit negatif avec
			SvrError(&SvrLink,<donnees>,<longueur>);
		(meme remarque)
		Si l'accuse de reception n'est pas fait dans SvrUser(), SvrRun() le
		rajoute automatiquement selon la valeur de retour de SvrUser():
			Message execute: retour > 0
			Message rejete: retour = 0
		L'instruction return(<valeur>); est donc obligatoire
		*	SvrStopped()
		qui effectue le nettoyage necessaire si le lien avec le client
		est interrompu pour cause d'erreur repetees (peut etre vide)

	Pour l'envoi de donnees hors accuse de reception, on dispose des
	memes routines que le client (voir ci-dessous).

	Routine accessoire:
		*	SvrMaxErr(<n>)
		donne le nombre maximum d'erreurs de transmission avant de casser
		la connection (et appeler alors SvrStopped())
		*	a venir: SvrTimeOut(<secondes>) limite le temps d'attente
		de la connexion, cote client
	               
     
Cote client:
-----------

	. inclure ce fichier
	. se donner une zone de travail par une instruction du type:
				TypeSvrBox link;
	. au moment d'effectuer une serie de requetes, appeler:
				SvrOpen(&link,<serveur>,<port>,<longueur max des donnees>);
		cette routine renvoie une valeur nulle si la connection n'a pas pu
		etre faite. Exemple de programmation:

	|	if(!SvrOpen(&link,Station,ISP_PORT_TIME,10)) {
	|		e = errno;
	|		prerr(0,_errmsg(errno,"connection avec %s impossible\n",Station));
	|		exit(e);
	|	}
	|	PeerName(link.skt,nom);
	|	printf("Connection avec %s etablie\n",nom);
		
	.	pour envoyer une requete, utiliser:
				SvrSend(&link,<commande>,<donnees>,<longueur des donnees>);
		(<donnees> vaut 0 si il n'y a pas de donnees a transmettre
		en plus de la commande)
	.	On doit tester l'arrivee de l'accuse de reception, avec:
				SvrRecv(&link,<texte>);
		ou <texte> est une zone de 80 caracteres contenant un message
		d'erreur. La valeur de retour de cette routine vaut 0 si la
		l'execution ou la reception a ete incorrecte, 1 sinon.
		Comme du cote serveur, on dispose de la reponse du serveur
		("OK" ou "Error") dans link.cmde, et des donnees eventuellement 
		associees (ou du texte d'explication) dans link.data; la longueur
		des donnees est dans link.lngr
	.	On peut preferer utiliser a la place, 
				SvrCheck(&link);
		qui rend un code d'erreur (en particulier SVR_SUCCESS si 
		l'execution de la requete est correcte. Exemple de programmation:

	|	SvrSend(&link,cmde,0,0);
	|	switch(rc = SvrCheck(&link)) {
	|	  case SVR_SUCCESS: printf("Temps sideral = %s\n",link.data); break;
	|	  case SVR_REJECTED: printf("Erreur: %s\n",link.data); break;
	|	  default: printf("Erreur serveur #%d (%s)\n",rc,link.data);
	|		if(rc == SVR_SYNCHRO) printf("Paquet envoye: %d, recu: %d\n",
	|			link.send,link.recv);
	|	}
	|	SvrClose(&link);

		On n'envoie pas d'accuse de reception sur un accuse de reception.
		Par contre, si le serveur envoie un message qui n'est pas un accuse
		de reception, il faut lui repondre par SvrOK(..) ou SvrError(..)
		(voir cote serveur).
*/

#ifndef SERVEUR_H
#define SERVEUR_H

#ifdef SERVEUR_C
	#define SERVEUR_VAR
#else
	#define SERVEUR_VAR extern
#endif

#include <tcp/sktlib.h>
#include <utils/defineVAR.h>

#define SVR_LEN_DEF 0x2000

#define SVR_LEN_CMDE 8
#define SVR_LEN_SERIAL 4
#define SVR_LEN_SIZE 4
#define SVR_HEADER (SVR_LEN_CMDE+SVR_LEN_SERIAL+SVR_LEN_SIZE)

#define SVR_LOC_SERIAL SVR_LEN_CMDE
#define SVR_LOC_SIZE   SVR_LOC_SERIAL+SVR_LEN_SERIAL

#define SVR_ALARM 9999
#define SVR_TIMEOUT (7 * 256 + 25)

#define SVR_FERMER "svrclos"
#define SVR_TERMINER "svrexit"

typedef enum {
	SVR_SUCCESS = 0,
	SVR_REJECTED,
	SVR_SYNCHRO,
	SVR_EMPTY,
	SVR_READ,
	SVR_LENGTH,
	SVR_OVER,
	SVR_UNDEF
} SVR_ERROR;

typedef enum {
	SVR_AUTOFORK,
	SVR_DEMON,
	SVR_CONNECTE
} SVR_MODE;

typedef struct {
	int     skt;                /* chemin d'acces via IP                       */
	char   *cmde;               /* commande client et buffer general           */
	int     max;                /* dimension du buffer                         */
	char   *data;               /* pointeur sur la partie donnees du buffer    */
	ssize_t lngr;               /* longueur de la partie donnees               */
	char   *next;               /* pointeur sur le prochain message client     */
	ssize_t dejalu;             /* nombre d'octet deja lus du prochain message */
	int     send,recv;          /* identifieur de message emis et recu         */
	char    sent;               /* temoin d'envoi de l'accuse de reception     */
	TypeSocket own_skt;         /* socket IP                                  */
} TypeSvrBox,*SvrBox;

SERVEUR_VAR TypeSvrBox SvrLink;
SERVEUR_VAR int SvrFils; /* > 0 s'il existe un fils */
SERVEUR_VAR int SvrPrecedente;

SERVEUR_VAR int SvrTimeOut
#ifdef SERVEUR_C
 = 4
#endif
;
SERVEUR_VAR int SvrDefaultLength
#ifdef SERVEUR_C
 = SVR_LEN_DEF
#endif
;
SERVEUR_VAR char SvrExit
#ifdef SERVEUR_C
 = 0
#endif
;
SERVEUR_VAR char SvrMulti
#ifdef SERVEUR_C
= 1
#endif
;
SERVEUR_VAR char SvrDebug
#ifdef SERVEUR_C
= 0
#endif
;

void SvrLogEnable(char autorise);
void SvrHandler(int signal);
void SvrOpenWait(int delai);
int SvrOpen(SvrBox link, char *serveur, int port, int lngr);
int SvrSend(SvrBox link, char *cmde, char *data, int lngr);
int SvrCheck(SvrBox link);
int SvrRecv(SvrBox link);
int SvrOK(SvrBox link, char *data, int lngr);
int SvrError(SvrBox link, char *data, int lngr);
void SvrMaxErr(int nb);
char *SvrText(int rc);
int SvrClose(SvrBox link);
void SvrSetArgs(int argc, char **argv);
int SvrRun(int port);
int SvrExec(int port, int base);
pid_t SvrFork(int base);
int SvrBegin(SvrBox link, char *cmde, int ltot);
int SvrHeader(SvrBox link, int serial, char *cmde, int ltot);
int SvrAdd(SvrBox link, char *data, int lngr);
int SvrEnd(SvrBox link);
INLINE void SvrPutByte(char *adrs, int val);
INLINE int SvrGetByte(char *adrs);
INLINE void SvrPutInt(char *adrs, int val);
INLINE int SvrGetInt(char *adrs);
INLINE void SvrPutShort(char *adrs, short val);
INLINE unsigned short SvrGetShort(char *adrs);
INLINE void SvrPutFloat(char *adrs, float f);
INLINE float SvrGetFloat(char *adrs);

/* A definir par l'utilisateur, ou prendre dans svrnul */
int SvrActive(void);
int SvrAborted(void);
int SvrQuited(void);
int SvrExited(void);
int SvrUser(void);

#endif
