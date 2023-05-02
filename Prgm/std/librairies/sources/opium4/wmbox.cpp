#include <string.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
extern char *__progname;

#include "environnement.h"
#include "defineVAR.h"
#include "shared_memory.h"

#define WMBOX_C
#include "wmbox.h"
static char WmboxSource[80];
#define _SRCE_ strcat(strcat(strcpy(WmboxSource,__progname),"."),__func__)

#include "actioncodes.h"

/* ========================================================================== */
Wmbox WmboxCreate() {
	Wmbox mbx;

	if((mbx = (Wmbox)malloc(sizeof(TypeWmbox)))) {
		mbx->id = -1; mbx->nom[0] = '\0';
		mbx->verrou = SEM_FAILED;
	}
	return(mbx);
}
/* .......................................................................... */
void WmboxShare(Wmbox mbx, char *nom) {
	mbx->verrou = sem_open(nom,O_CREAT,TOUTPERMIS); // SEM_VALUE_MAX maxi
	printf("  Semaphore '%s' pour %s: S=%08llX\n",nom,__progname,(IntAdrs)(mbx->verrou));
	if(mbx->verrou == SEM_FAILED)
		printf("  ! Pas de verrouillage de la BAL (%d, %s)\n",errno,errno?strerror(errno):"successful");
	sem_post(mbx->verrou);
	if(sem_trywait(mbx->verrou) < 0)
		printf("  ! BAL verrouillee pour %s (%d, %s)\n",__progname,errno,errno?strerror(errno):"successful");
	else {
		printf("  Acces BAL autorise pour %s\n",__progname);
		sem_post(mbx->verrou);
		printf("  Semaphore S=%08llX libere\n",(IntAdrs)(mbx->verrou));
	}
	return;
}
/* .......................................................................... */
void WmboxRaz(Wmbox mbx) {
	int cde,evt;

	mbx->cdelog = mbx->evtlog = 0;
	mbx->poly.nb = mbx->poly.dim = 0; mbx->poly.val = 0; // mbx->poly.reserve = 0;

	mbx->cde_a_ecrire = mbx->cde_a_lire = 0;
	mbx->evt_a_ecrire = mbx->evt_a_lire = 0;

	for(cde=0; cde<WMB_CDE_MAX; cde++) {
		mbx->commande[cde].num = cde;
		mbx->commande[cde].status = WMB_STS_LIBRE;
		mbx->commande[cde].reponse_attendue = 0;
	}
	for(evt=0; evt<WMB_EVT_MAX; evt++) {
		mbx->event[evt].num = evt;
		mbx->event[evt].status = WMB_STS_LIBRE;
	}
}
/* .......................................................................... */
void WmboxRequestLog(Wmbox mbx, char log) { mbx->cdelog = log; }
/* .......................................................................... */
void WmboxEventLog(Wmbox mbx, char log) { mbx->evtlog = log; }
/* .......................................................................... */
void WmboxRequestStatus(Wmbox mbx, WmboxCommande commande, const char *fctn) {
	if(!commande->log) return;
	printf("(%s) Commande %4d/%d %s: %-13s %3s (a traiter: %d)\n",fctn,
		commande->num,WMB_CDE_MAX,WmboxStsListe[commande->status],
		WMB_CDE_NAME(commande->cmde),commande->reponse_attendue?"[R]":" ",mbx->cde_a_lire);
}
/* .......................................................................... */
void WmboxEventStatus(Wmbox mbx, WmboxEvent event, const char *fctn) {
	if(mbx->evtlog) {
		if(event) printf("(%s) Evenement %4d/%d %s: %-12s (a traiter: %d)\n",fctn,event->num,WMB_EVT_MAX,
			WmboxStsListe[event->status],WND_ACTIONNAME(event->type),mbx->evt_a_lire);
		else printf("(%s) Evenement PERDU (a ecrire: %d, a traiter: %d)\n",fctn,mbx->evt_a_ecrire,mbx->evt_a_lire);
	}
}
/* .......................................................................... */
void WmboxRequestError(Wmbox mbx, WmboxCommande commande, const char *cause, const char *fctn) {
	if(commande) printf("(%s) Commande %4d/%d [%s] %s (a ecrire: %d, a traiter: %d)\n",fctn,
		commande->num,WMB_CDE_MAX,WMB_CDE_NAME(commande->cmde),cause,mbx->cde_a_ecrire,mbx->cde_a_lire);
	else printf("(%s) Commande indeterminee/%d [%s] %s (a ecrire: %d, a traiter: %d)\n",fctn,
		WMB_CDE_MAX,"inconnue",cause,mbx->cde_a_ecrire,mbx->cde_a_lire);
#ifdef MBX_PMD
	printf("(%s) Etat des commandes:\n(%s.%s)      |",fctn,__progname,fctn);
	int cde; for(cde=0; cde<10; cde++) printf("   %-12d",cde);
	printf("\n(%s)      |",fctn); for(cde=0; cde<10; cde++) printf(" --------------");
	for(cde=0; cde<WMB_CDE_MAX; cde++) {
		if(!(cde % 10)) printf("\n(%s) %4d |",fctn,cde);
		if(mbx->commande[cde].status == WMB_STS_LIBRE) printf(" %-14s",WmboxStsListe[mbx->commande[cde].status]);
		else printf(" %d%-13s",mbx->commande[cde].status,WMB_CDE_NAME(mbx->commande[cde].cmde));
	}
	printf("\n! Arret de l'application\n");
	exit(0);
#endif /* MBX_PMD */
}
/* .......................................................................... */
WmboxCommande WmboxRequestPtr(Wmbox mbx, unsigned char cmde) {
	WmboxCommande commande; short limite; char accordee;

	if(mbx->cdelog) printf("(%s) %s\n",_SRCE_,WmboxCmdeListe[cmde]);
	limite = 10000; accordee = 0;
	do {
		// printf("(%s) Attente du semaphore S=%08llX\n",_SRCE_,(IntAdrs)(mbx->verrou));
		if(sem_trywait(mbx->verrou) < 0) {
			if(mbx->cdelog) printf("(%s.sem_trywait) ! BAL B=%08llX indisponible (%d, %s), en attente..\n",_SRCE_,(IntAdrs)mbx,errno,errno?strerror(errno):"successful");
			if(sem_wait(mbx->verrou) < 0) {
				if(mbx->cdelog) printf("(%s.sem_wait)    ! BAL B=%08llX indisponible (%d, %s), en attente..\n",_SRCE_,(IntAdrs)mbx,errno,errno?strerror(errno):"successful");
			}
			if(mbx->cdelog) printf("(%s) Attente terminee\n",_SRCE_);
		}
		// printf("(%s) Acces BAL B=%08llX autorise\n",_SRCE_,(IntAdrs)mbx);
		commande = &(mbx->commande[mbx->cde_a_ecrire]);
		if(commande->status == WMB_STS_LIBRE) {
			commande->status = WMB_STS_RESERVEE;
			commande->log = mbx->cdelog;
			if(commande->log) printf("(%s) Commande %4d/%d %s: %s\n",_SRCE_,commande->num,WMB_CDE_MAX,WmboxStsListe[commande->status],WMB_CDE_NAME(cmde));
			commande->cmde = cmde;
			if(++(mbx->cde_a_ecrire) >= WMB_CDE_MAX) mbx->cde_a_ecrire = 0;
			accordee = 1;
		}
		sem_post(mbx->verrou);
		// printf("(%s) Semaphore S=%08llX libere%s\n",_SRCE_,(IntAdrs)(mbx->verrou),accordee?"":", BAL pleine");
		if(accordee) break; else usleep(1000);
	} while(limite--);

	// printf("(%s) Commande %s %s (#%d)\n",_SRCE_,WmboxCmdeListe[cmde],accordee?"accordee":"refusee",accordee?commande->num:-1);
	return(accordee? commande: 0);
}
/* .......................................................................... */
char WmboxRequestSend(WmboxCommande commande) {
	if(commande) {
		commande->reponse_attendue = 0;
		commande->status = WMB_STS_REDIGEE;
		return(1);
	} else return(0); // WmboxRequestError(mbx,commande,"PERDUE",__func__);
}
/* .......................................................................... */
char WmboxRequestReply(Wmbox mbx, WmboxCommande commande) {
	if(commande->log) printf("(%s) Commande %s\n",_SRCE_,commande?WmboxCmdeListe[commande->cmde]:"perdue!");
	if(!commande) return(0);
	commande->reponse_attendue = 1;
	commande->status = WMB_STS_REDIGEE;
	short limite = 1000; do {
		if((commande->status == WMB_STS_REPONDUE) || (commande->status == WMB_STS_LIBRE)) return(1);
		usleep(1000);
	} while(--limite);
	WmboxRequestClose(mbx,commande);
	return(0); // WmboxRequestError(mbx,commande,"INACHEVEE",__func__);
}
/* .......................................................................... */
void WmboxRequestReuse(WmboxCommande commande, unsigned char cmde) {
	if(commande) {
		commande->cmde = cmde;
		commande->status = WMB_STS_RESERVEE;
		// printf("(%s) Commande %s reutilisee (#%d)\n",_SRCE_,WmboxCmdeListe[cmde],commande->num);
	}
}
/* .......................................................................... */
void WmboxRequestClose(Wmbox mbx, WmboxCommande commande) {
	if(commande) {
		sem_wait(mbx->verrou);
		commande->status = WMB_STS_LIBRE;
		// if(commande->log) printf("(%s)  Commande %4d/%d %s\n",_SRCE_,commande->num,WMB_CDE_MAX,WmboxStsListe[commande->status]);
		if(++(mbx->cde_a_lire) >= WMB_CDE_MAX) mbx->cde_a_lire = 0;
//		if(commande->num == mbx->cde_a_lire) WmboxRequestError(mbx,commande,"INACCESSIBLE",__func__);
		sem_post(mbx->verrou);
	}
}
/* ========================================================================== */
WmboxEvent WmboxEventPtr(Wmbox mbx, WmboxEventCode type) {
	WmboxEvent event; short limite; char accorde;

	limite = 1000; accorde = 0;
	event = &(mbx->event[mbx->evt_a_ecrire]);
	do {
		sem_wait(mbx->verrou);
		if(event->status == WMB_STS_LIBRE) {
			event->status = WMB_STS_RESERVE;
			// printf("(%s) Evenement %4d/%d %s\n",_SRCE_,event->num,WMB_EVT_MAX,WmboxStsListe[event->status]);
			event->type = type;
			if(++(mbx->evt_a_ecrire) >= WMB_EVT_MAX) mbx->evt_a_ecrire = 0;
			accorde = 1;
		}
		sem_post(mbx->verrou);
		if(accorde) break; else usleep(1000);
	} while(limite--);

	return(accorde? event: 0);
}
/* .......................................................................... */
void WmboxEventSend(WmboxEvent event) {
	if(event) event->status = WMB_STS_COMPLET; // sinon, evt perdu
}
/* .......................................................................... */
WmboxEvent WmboxEventNext(Wmbox mbx) {
	WmboxEvent event;

	sem_wait(mbx->verrou);
	event = &(mbx->event[mbx->evt_a_lire]);
	if(event->status == WMB_STS_COMPLET) {
		event->status = WMB_STS_UTILISE;
		if(++(mbx->evt_a_lire) >= WMB_EVT_MAX) mbx->evt_a_lire = 0;
	} else event = 0;
	sem_post(mbx->verrou);
	return(event);
}
/* .......................................................................... */
void WmboxEventFree(WmboxEvent event) {
	if(event) event->status = WMB_STS_LIBRE;
}
/* ========================================================================== */
void WmboxFree(Wmbox mbx) {
	if(mbx->verrou != SEM_FAILED) sem_close(mbx->verrou);
	if(mbx->nom[0]) {
		//SharedMemKill(mbx->nom,(void *)mbx,mbx->id);
		int i = 256; while((shmdt((void *)mbx) != -1) && i) i-- ;
		shmctl(mbx->id,IPC_RMID,0);
	} else free(mbx);
}
