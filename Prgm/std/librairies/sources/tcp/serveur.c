#include "environnement.h"
// #ifdef macintosh
// #pragma message("Compilation de "__FILE__)
// #endif

#define SERVEUR_C
#ifdef XCODE
	#define UNIX
#endif
/* #define DEBUG */
/* #define DEBUG2 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>  /* pour isprint */
#include <string.h>
#include <errno.h>
#include <signal.h>
//#define ETIMEDOUT 60
//#define EISCONN   56
#define	_BSD_CLOCK_T_DEFINED_
#include <time.h>

#include "serveur.h"

static char SvrPereUnique = 1;
static int SvrTol = 8;
static char SvrPPC = 0;
static int SvrSignal = 0;
static int SvrArgc; static char **SvrArgv,*SvrArgvTxt;

static char  SvrTxtRecv[256];
static char *SvrTxtREJECTED = "Requete recue mais rejetee";
static char *SvrTxtSYNCHRO = "Desynchronisation: au moins une requete perdue";
static char *SvrTxtSUCCESS = "Requete recue et acceptee";
static char *SvrTxtEMPTY = "Paquet vide";
static char *SvrTxtREAD = "Paquet illisible";
static char *SvrTxtLENGTH = "Longueur de bloc erronee";
static char *SvrTxtOVER = "Pas assez de place memoire";
static char *SvrTxtUNDEF = "Erreur non repertoriee";

static char SvrHeureTexte[16];

static void SvrAlign(char *ptr, char *buffet, ssize_t ldata);

/* ========================================================================== */
void SvrLogEnable(char autorise) { SvrDebug = autorise; }
/* ========================================================================== */
void SvrHandler(signal)  int signal; {
	SvrSignal = signal;
}
/* ========================================================================== */
static char *SvrHeureTxt(void) {
  /* identique a HeureTxt de haure.c (sauf utilisation de SvrHeureTexte) */
  time_t temps;
  struct tm date;

  time(&temps);
  memcpy(&date,localtime(&temps),sizeof(struct tm));
  sprintf(SvrHeureTexte,"%02d:%02d:%02d",date.tm_hour,date.tm_min,date.tm_sec);
  return(SvrHeureTexte);
}
/* ========================================================================== */
static char SvrEndianIsBig(void) {
	union {
		char octet[2];
		unsigned short mot;
	} preuve;
	preuve.octet[0] = 1; preuve.octet[1] = 0;
	return(preuve.mot > 255);
}
/* ========================================================================== */
void SvrOpenWait(int delai) { SvrTimeOut = delai; }
/* ========================================================================== */
int SvrOpen(SvrBox link, char *serveur, int port, int lngr) {
	int rc,k,interr;
#ifdef OS9
	int aid;
#endif
#ifdef UNIX
	sigset_t masque,msk_avant;
	struct sigaction alarme,alm_avant;
#endif

	SvrPPC = SvrEndianIsBig();
	SvrSignal = 0;
#ifdef OS9
	interr = E_SIGNAL; /* pas tres sur ... */
	intercept(SvrHandler);
	aid = alm_set(SVR_ALARM,0x80000000+(SvrTimeOut*256));
#endif
#ifdef UNIX
	interr = EINTR;
	sigemptyset(&masque);
	sigaddset(&masque,SIGUSR2);
	sigprocmask(SIG_BLOCK,&masque,&msk_avant);

	alarme.sa_handler = SvrHandler;
	sigemptyset(&alarme.sa_mask);
#ifdef hpux
	alarme.sa_flags = SA_RESETHAND;
#else
	alarme.sa_flags = SA_RESTART;
#endif
	sigaction(SIGALRM,&alarme,&alm_avant);

	alarm(SvrTimeOut);
#endif

	if(SvrDebug) printf("[%d] (SvrOpen) Connexion lancee vers %s a %s\n",getpid(),serveur,SvrHeureTxt());
	link->skt = SocketClient(serveur,port,&link->own_skt);
#ifdef OS9
	alm_delete(aid);
	intercept(0);
#endif
#ifdef UNIX
/* 	sigaction(SIGALRM,&alm_avant,&alarme); */
	rc = alarm(0);
	sigprocmask(SIG_SETMASK,&msk_avant,NULL);
#endif
	if(SvrDebug) printf("[%d] (SvrOpen) Connexion code %d a %s avec signal=%d (restait %ds)\n",
	  getpid(),errno,SvrHeureTxt(),SvrSignal,rc);
/* 	if(errno) { */
	if(link->skt == -1) {
	  if(SvrSignal == SIGALRM) errno = ETIMEDOUT;
	  if(errno == interr) {
	    k = SvrTimeOut;
	    while(k--) {
	      sleep(1);
/* 	      if(connect(link->skt,&link->own_skt,sizeof(TypeSocket)) != -1) break; */
	      if((link->skt = SocketConnect(&link->own_skt)) >= 0) break;
	      if(SvrDebug) printf("[%d] (SvrOpen) Connexion #%d code %d a %s\n",
	        getpid(),SvrTimeOut-k,errno,SvrHeureTxt());
	      if(errno == EISCONN) break;
	    }
/* 	    if(k < 0) link->skt = -1; */
	  } /* else link->skt = -1; */
	  if(link->skt == -1) return(0);
	}

	link->max = lngr + SVR_HEADER;
	link->cmde = (char *)malloc(link->max);
	if(link->cmde == 0) return(-1);
	link->data = link->cmde + SVR_HEADER;
	link->lngr = 0;
	link->next = link->cmde;
	link->dejalu = 0;
	link->send = 0;
	link->recv = 0;
	link->sent = 0;
	return(1);
}
/* ========================================================================== */
int SvrSend(SvrBox link, char *cmde, char *data, int lngr) {
	SvrBegin(link,cmde,lngr);
	SvrAdd(link,data,lngr);
	return(SvrEnd(link));
}
/* ========================================================================== */
int SvrCheck(SvrBox link) {
	int rc;

	rc = SvrRecv(link);
	if(!rc) {
		if(!link->lngr) {
		  strcpy(link->data,SvrTxtRecv);
		  link->lngr = (ssize_t)strlen(SvrTxtRecv);
		}
		if(SvrTxtRecv[2] == ':') sscanf(SvrTxtRecv,"%d",&rc);
		if(!rc) rc = SVR_UNDEF;
		return(rc);
	} else if(!strncmp(link->cmde,"Err",3)) return(SVR_REJECTED);
	else if(link->send != link->recv) return(SVR_SYNCHRO);
	else return(SVR_SUCCESS);
}
/* ========================================================================== */
int SvrRecv(SvrBox link) {
	IntAdrs bord,l; size_t size; ssize_t alire;
	register ssize_t lngr,lu,tmp;
	register char *next,*ptr;

	ptr = link->cmde;
	size = link->max;
	bord = (IntAdrs)ptr + size;
	if((link->dejalu > 0) && (IntAdrs)link->next > (IntAdrs)ptr) {
		SvrAlign(link->next,ptr,link->dejalu);
		link->next = ptr;
	}
	while(link->dejalu < SVR_HEADER) {
		lu = (int)read(link->skt,ptr+link->dejalu,size-link->dejalu);
		if(!lu) {
			sprintf(SvrTxtRecv,"%02d: %s",SVR_EMPTY,SvrTxtEMPTY);
			goto boum;
		} else if(lu < 0) {
			sprintf(SvrTxtRecv,"%02d: %s, code #%03d:%03d",
				SVR_READ,SvrTxtREAD,errno>>8,errno&255);
			goto boum;
		}
		link->dejalu += lu;
	}
	lu = link->dejalu;
	if(lu < size) ptr[lu] = '\0';
	link->recv = SvrGetInt(ptr + SVR_LOC_SERIAL);
	lngr = SvrGetInt(ptr + SVR_LOC_SIZE);
	if(SvrDebug) {
	  printf("[%d]   recu: %08lX %08lX %08lX %08lX  [",getpid(),
		*(unsigned long *)ptr,*(unsigned long *)(ptr+4),*(unsigned long *)(ptr+8),*(unsigned long *)(ptr+12));
	  for(l=0; l<SVR_LEN_CMDE; l++)
	    if(isprint(ptr[l])) putchar(ptr[l]); else putchar(' ');
	  printf("] + %ld/%ld octet(s)\n",lu-SVR_HEADER,lngr);
	}
	if(lngr < 0) {
		sprintf(SvrTxtRecv,"%02d: %s (%ld)",SVR_LENGTH,SvrTxtLENGTH,lngr);
		goto boum;
	}
	alire = lngr + SVR_HEADER;
	if(alire > size) {
		size = alire;
		link->max = (int)alire;
		next = (char *)malloc(size);
		if(!next) {
			sprintf(SvrTxtRecv,
				"%02d: %s (requetes de %ld octets)",SVR_OVER,SvrTxtOVER,size);
			goto boum;
		}
		if(lu > 0) SvrAlign(ptr,next,lu);
		free(ptr);
		ptr = link->cmde = next;
		link->data = ptr + SVR_HEADER;
		bord = (long)ptr + size;
	}
	while(lu < alire) {
		tmp = read(link->skt,ptr+lu,size-lu);
		if(!tmp) {
			sprintf(SvrTxtRecv,"%02d: Paquet vide",SVR_EMPTY);
			goto boum;
		} else if(tmp < 0) {
			sprintf(SvrTxtRecv,"%02d: %s, code #%03d:%03d",
				SVR_READ,SvrTxtREAD,errno>>8,errno&255);
			goto boum;
		}
		lu += tmp;
		if(SvrDebug) printf("[%d]   recu: %ld octet(s) (total: %ld/%ld)\n",
			getpid(),tmp,lu-SVR_HEADER,alire-SVR_HEADER);
	}
	link->lngr = lngr;
	link->next = ptr + alire;
	link->dejalu = lu - (ssize_t)alire;
	return(1);
boum:
	lu = link->dejalu;
	link->lngr = lu;
	link->next = ptr;
	/*
	if(link->dejalu >= SVR_HEADER)
		printf("Deja recu: %08lX %08lX %08lX %08lX + %d octet(s)\n",
		*(int *)ptr,*(int *)(ptr+4),*(int *)(ptr+8),*(int *)(ptr+12),lngr);
	*/
	link->dejalu = 0;
	tmp = (int)strlen(SvrTxtRecv);
	sprintf(SvrTxtRecv+tmp,". %ld octet(s) perdu(s)",lu);
	return(0);
}
/* ========================================================================== */
int SvrOK(SvrBox link, char *data, int lngr) {
	SvrHeader(link,link->recv,"OK",lngr);
	SvrAdd(link,data,lngr);
	return(SvrEnd(link));
}
/* ========================================================================== */
int SvrError(SvrBox link, char *data, int lngr) {
	SvrHeader(link,link->recv,"Error",lngr);
	SvrAdd(link,data,lngr);
	return(SvrEnd(link));
}
/* ========================================================================== */
void SvrMaxErr(int nb) { SvrTol = nb; }
/* ========================================================================== */
char *SvrText(int rc) {
	switch(rc){
	  case SVR_REJECTED: return(SvrTxtREJECTED); break;
	  case SVR_SYNCHRO: return(SvrTxtSYNCHRO); break;
	  case SVR_SUCCESS: return(SvrTxtSUCCESS); break;
	  case SVR_EMPTY: return(SvrTxtEMPTY); break;
	  case SVR_READ: return(SvrTxtREAD); break;
	  case SVR_LENGTH: return(SvrTxtLENGTH); break;
	  case SVR_OVER: return(SvrTxtOVER); break;
	  default: return(SvrTxtUNDEF);
	}
}
/* ========================================================================== */
int SvrClose(SvrBox link) {
	SvrSend(link,SVR_FERMER,0,0);
	SocketFermee(link->skt);
	link->skt = -1;
	free(link->cmde);
	link->cmde = 0;
	link->max = 0;
	link->lngr = 0;
	link->next = 0;
	link->dejalu = 0;
	link->send = 0;
	link->recv = 0;
	link->sent = 1;
	return(1);
}
/* ========================================================================== */
void SvrSetArgs(argc,argv)  int argc; char *argv[]; {
	size_t i,l;

	SvrArgc = argc;
	l = 0;
	for(i=0; i<argc; i++) l += (strlen(argv[i]) + 1);
	SvrArgvTxt = (char *)malloc(l);
	SvrArgv = (char **)malloc(4 * (argc + 2));
	l = 0;
	for(i=0; i<argc; i++) {
		SvrArgv[i] = SvrArgvTxt + l;
		strcpy(SvrArgv[i],argv[i]);
		l += (strlen(argv[i]) + 1);
	}
	SvrArgv[i] = 0;
	SvrArgv[i+1] = 0;
}
/* ========================================================================== */
pid_t SvrFork(base) int base; {
#ifdef OS9
	int os9forkc();
	extern char **_environ;
	char arg[16];

	sprintf(arg,"-:=%d",base);
	SvrArgv[SvrArgc] = arg;
	/*
	if(SvrDebug) {
		int i,j;
		j = os9exec(os9forkc,SvrArgv[0],SvrArgv,_environ,0,0,base+1);
		printf("[%d] Forked: ",getpid());
		i = 0;
		while(SvrArgv[i]) printf("%s ",SvrArgv[i++]);
		printf("=> process %d {error %d}\n",j,errno); fflush(stdout);
		return(j);
	} else
	*/
	return(os9exec(os9forkc,SvrArgv[0],SvrArgv,_environ,0,0,base+1));
#endif
#ifdef macintosh
	return(fork());
#endif
#ifdef UNIX
#ifndef macintosh
	return(fork());
#endif
#endif
}
/* ========================================================================== */
int SvrRun(int port) { return(SvrExec(port,-1)); }
/* ========================================================================== */
int SvrExec(int port, int base) {
	char pas_fini;
	char nom[80],req[SVR_LEN_CMDE+1],*cmde;
	int b,nerr,rc;
	TypeSocket rem_skt;

	printf("\n---------------------------------- Server [%d] started on port #%d\n",
		getpid(),port);
	SvrPPC = SvrEndianIsBig();
	if(base < 0) {
		b = SocketBase(port,&SvrLink.own_skt);
		if(b == -1) exit(errno);
	} else b = base;
	SvrLink.skt = -1;
	SvrLink.max = SvrDefaultLength;
	SvrLink.cmde = (char *)malloc(SvrLink.max);
	if(SvrLink.cmde == 0) {
		fprintf(stderr,"(SvrRun) Memory failed (%dK needed), aborted\n",SvrLink.max);
		exit(errno);
	}
	SvrLink.data = SvrLink.cmde + SVR_HEADER;
	SvrLink.lngr = 0;
	SvrLink.next = SvrLink.cmde;
	SvrLink.dejalu = 0;
	SvrLink.send = 0;
	SvrLink.recv = 0;
	SvrLink.sent = 0;

	SvrPrecedente = SvrLink.skt;
	while(!SvrExit) {
		SvrLink.skt = SocketBranchee(b,&rem_skt);
		SvrPrecedente = SvrLink.skt;
		if(SvrLink.skt == -1) { sleep(1); continue; }
		if(SvrMulti) SvrFils = SvrFork(b); else SvrFils = -1;
		/* SvrFils > 0: je suis parent, ceci est le pid du fils;
		   SvrFils = 0: je suis le fils;
		   SvrFils = -1: erreur <errno>, je suis le parent, pas de fils cree  */
		if(SvrFils) {
			PeerName(SvrLink.skt,nom);
			printf("\n[%d] Socket #%d branchee sur %s\n",getpid(),SvrLink.skt,nom);
			if(SvrPereUnique) continue;
		} else {
			printf("\n---------------------------------- Server [%d] started on port #%d\n",
				getpid(),port);
//			SocketFermee(SvrLink.skt); ce n'est pas au fils de la fermer tout de suite? non, a la fin!
			continue;
		}
		pas_fini = 1;
		nerr = 0;
		do {
			rc = SvrRecv(&SvrLink);
			if(!rc) {
				printf("[%d] Erreur %s\n",getpid(),SvrTxtRecv);
				SvrError(&SvrLink,SvrTxtRecv,(int)strlen(SvrTxtRecv)+1);
				if(++nerr > SvrTol) break; else continue;
			}
			cmde = SvrLink.cmde;
			if(!strncmp(cmde,SVR_FERMER,7)) pas_fini = 0;
			else if(!strncmp(cmde,SVR_TERMINER,7)) SvrExit = 1;
			else {
				if(SvrUser()) {
					if(!SvrLink.sent) SvrOK(&SvrLink,0,0);
				} else {
					strncpy(req,cmde,SVR_LEN_CMDE);
					sprintf(SvrTxtRecv,"%02d: %s (\'%s\' + %ld octets)",
						SVR_REJECTED,SvrTxtREJECTED,req,SvrLink.lngr);
					if(!SvrLink.sent)
						SvrError(&SvrLink,SvrTxtRecv,(int)strlen(SvrTxtRecv)+1);
					printf("[%d] Erreur %s\n",getpid(),SvrTxtRecv);
					if(++nerr > SvrTol) break; else continue;
				}
				SvrLink.sent = 0;
			}
		} while(pas_fini && !SvrExit);
		if(nerr > SvrTol) {
			printf("[%d] %d erreurs. Service interrompu.\n",getpid(),nerr);
			SvrAborted();
		}
		SocketFermee(SvrLink.skt);
		printf("[%d] Socket #%d sur %s debranchee\n",getpid(),SvrLink.skt,nom);
		SvrLink.skt = -1;
		SvrQuited();
		if(SvrFils > 0) SvrExit = 1;
	};
//	if(SvrPrecedente >= 0) SocketFermee(SvrPrecedente);
	printf("---------------------------------- Server [%d] stopped on port #%d\n",getpid(),port);
	fflush(stdout);
	SvrExited();
	close(b);
	return(0);
}
/* ========================================================================== */
int SvrBegin(SvrBox link, char *cmde, int ltot) {
	return(SvrHeader(link,++(link->send),cmde,ltot));
}
/* ========================================================================== */
int SvrHeader(SvrBox link, int serial, char *cmde, int ltot) {
	char header[SVR_HEADER];
	char *adrs;
	int l;
	int rc;

	l = 0;
	while(l < SVR_LEN_CMDE) {
		if(!(cmde[l])) break;
		header[l] = cmde[l];
		l++;
	}
	while(l < SVR_HEADER) header[l++] = '\0';
	adrs = header + SVR_LOC_SERIAL;
	SvrPutInt(adrs,serial); adrs += 4;
	SvrPutInt(adrs,ltot);
	if(SvrDebug) {
	  printf("[%d]  envoi: %08lX %08lX %08lX %08lX  [",getpid(),
		*(unsigned long *)(header),*(unsigned long *)(header+4),*(unsigned long *)(header+8),
		*(unsigned long *)(header+12));
	  for(l=0; l<SVR_LEN_CMDE; l++)
	    if(isprint(header[l])) putchar(header[l]); else putchar(' ');
	  printf("]\n");
	}
	rc = (int)write(link->skt,header,SVR_HEADER);
	if(rc < 0) {
		printf("[%d] Header pas ecrit... (%s)\n",getpid(),strerror(errno));
		strcpy(header,"Error");
		l = 5;
		while(l < SVR_HEADER) header[l++] = '\0';
		write(link->skt,header,SVR_HEADER);
	}
	return(1);
}
/* ========================================================================== */
int SvrAdd(SvrBox link, char *data, int lngr) {
	int rc,l;
	char *zero; int n;

	if(SvrDebug) printf("[%d]  envoi: %d octet(s)\n",getpid(),lngr);
	if(data && (lngr > 0)) rc = (int)write(link->skt,data,lngr); else rc = 0;
	if(rc < 0) {
		printf("[%d]  donnees pas ecrites... (%s)\n",getpid(),strerror(errno));
		l = (int)strlen(strerror(errno)) + 1;
		write(link->skt,strerror(errno),l);
		n = lngr - l;
		zero = (char *)malloc(n);
		while(n--) zero[n] = '\0';
		write(link->skt,zero,lngr - l);
	}
	return(1);
}
/* ========================================================================== */
int SvrEnd(SvrBox link) {
	link->sent = 1;
	if(SvrDebug) printf("[%d]  envoi  termine\n",getpid());
	return(1);
}
/* ========================================================================== */
static void SvrAlign(char *ptr, char *buffet, ssize_t ldata) {
	register char *anc,*nouv;
	register ssize_t i;

	nouv = buffet;
	anc = ptr;
#ifdef OS9v2
	i = ldata - 1;
	;
@enc move.b (a2)+,(a3)+
@    dbra   d4,enc
	;
#else
	i = ldata;
	while(i--) *nouv++ = *anc++;
#endif
}
/* ========================================================================== */
INLINE void SvrPutByte(adrs,val)  char *adrs; int val; {
	if(SvrPPC) *(adrs + 3) = val & 0xFF; else *(int *)adrs = val;
}
/* ========================================================================== */
INLINE int SvrGetByte(adrs)  char *adrs; {
	if(SvrPPC) {
		unsigned int l3;
		l3 = (unsigned int)(*(adrs + 3)) & 0xFF;
		return(l3);
	} else return(*(int *)adrs);
}
/* ========================================================================== */
INLINE void SvrPutInt(char *adrs, int val) {
	if(SvrPPC) {
	#ifdef OLDBYTESWAP
		*(adrs + 3) = val & 0xFF; val /= 256;
		*(adrs + 2) = val & 0xFF; val /= 256;
		*(adrs + 1) = val & 0xFF; val /= 256;
		* adrs      = val & 0xFF;
	#else
		*(adrs + 3) = val & 0xFF; val = val >> 8;
		*(adrs + 2) = val & 0xFF; val = val >> 8;
		*(adrs + 1) = val & 0xFF; val = val >> 8;
		* adrs      = val & 0xFF;
	#endif
	} else *(int *)adrs = val;
}
/* ========================================================================== */
INLINE int SvrGetInt(char *adrs) {
	if(SvrPPC) {
		int l0,l1,l2,l3,val;
		l0 = (int)(*adrs) & 0xFF;
		l1 = (int)(*(adrs + 1)) & 0xFF;
		l2 = (int)(*(adrs + 2)) & 0xFF;
		l3 = (int)(*(adrs + 3)) & 0xFF;
		val = (((((l0 << 8) | l1) << 8) | l2) << 8) | l3;
		return(val);
	} else return(*(int *)adrs);
}
/* ========================================================================== */
INLINE void SvrPutShort(char *adrs, short val) {
	if(SvrPPC) {
	#ifdef OLDBYTESWAP
		*(adrs + 1) = val & 0xFF; val /= 256;
		* adrs      = val & 0xFF;
	#else
		*(adrs + 1) = val & 0xFF; val = val >> 8;
		* adrs      = val & 0xFF;
	#endif
	} else *(short *)adrs = val;
}
/* ========================================================================== */
INLINE unsigned short SvrGetShort(adrs)  char *adrs; {
	if(SvrPPC) {
		int l0,l1; unsigned short val;

		l0 = (int)(*adrs) & 0xFF;
		l1 = (int)(*(adrs + 1)) & 0xFF;
		val = (unsigned short)(((l0 << 8) | l1) & 0xFFFF);
		return(val);
	} else return(*(unsigned short *)adrs);
}
/* ========================================================================== */
INLINE void SvrPutFloat(char *adrs, float f) {
	if(SvrPPC) {
	#ifdef OLDBYTESWAP
		union {
		  int i;
		  float f;
		} val;

		val.f = f;
		*(adrs + 3) = val.i & 0xFF; val.i /= 256;
		*(adrs + 2) = val.i & 0xFF; val.i /= 256;
		*(adrs + 1) = val.i & 0xFF; val.i /= 256;
		* adrs      = val.i & 0xFF;
	#else
		union {
		  int i;
		  float f;
		} val;

		val.f = f;
		*(adrs + 3) = val.i & 0xFF; val.i = val.i >> 8;
		*(adrs + 2) = val.i & 0xFF; val.i = val.i >> 8;
		*(adrs + 1) = val.i & 0xFF; val.i = val.i >> 8;
		* adrs      = val.i & 0xFF;
	#endif
	} else *(float *)adrs = f;
}
/* ========================================================================== */
INLINE float SvrGetFloat(adrs)  char *adrs; {
	if(SvrPPC) {
		int l0,l1,l2,l3;
		union {
		  int i;
		  float f;
		} val;

		l0 = (int)(*adrs) & 0xFF;
		l1 = (int)(*(adrs + 1)) & 0xFF;
		l2 = (int)(*(adrs + 2)) & 0xFF;
		l3 = (int)(*(adrs + 3)) & 0xFF;
		val.i = (((((l0 << 8) | l1) << 8) | l2) << 8) | l3;
		return(val.f);
	} else return(*(float *)adrs);
}
