#include "environnement.h"

#include <errno.h>
#ifndef ERROR
#define ERROR -1
#endif

// pour printf
#include <stdio.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>

#include "defineVAR.h"
#include <shared_memory.h>
extern char *__progname;

#ifdef OS9
	#include <module.h>
	struct datamodule {
		struct modhcom header;
		int data_offset;
	} *modstart;
#endif

/* ========================================================================= */
void *SharedMemMake(char *nom, size_t taille, int *id) {
#ifdef OS9
	if(((int)modstart=_mkdata_module(nom,taille,0x8001,0x333)) == (int)ERROR)
		return(ERROR);
	else return( (void *)modstart + modstart->data_offset );
#else
	int p,v; key_t cle,lim; void *adrs; char dbg=0;

	p = open(nom, O_CREAT|O_RDWR,TOUTPERMIS);
	//-	if((adrs = malloc(taille))) { bzero(adrs,taille); write(p,adrs,taille); free(adrs); }
	close(p);
	errno = 0;
	cle = ftok(nom,*id); lim = cle + 512;
	if((errno != 0) || dbg) {
		printf("  | ftok: cle 0x%08X (%d, %s)\n",(unsigned int)cle,errno,strerror(errno));
		if(errno != 0) return(0);
	}
	do {
		errno = 0;
		v = shmget(cle,taille,IPC_CREAT|TOUTPERMIS); // (IPC_PRIVATE,taille,0644|IPC_CREAT|IPC_EXCL)
	} while((++cle < lim) && (errno == EACCES));
	if((errno != 0) || dbg) {
		printf("  | shmget: Id[%ld] 0x%08X (%d, %s)\n",taille,(unsigned int)v,errno,strerror(errno));
		if(errno != 0) return(0);
	}
#ifdef AJUSTE_DROITS
	struct shmid_ds conf;
	p = shmctl(v,IPC_STAT,&conf);
	if((errno != 0) || dbg) {
		if(p != -1) printf("  > shmctl(IPC_STAT): %ld/%ld octets\n",conf.shm_segsz,taille);
		else printf("  | shmctl(IPC_STAT): %d, %s\n",errno,strerror(errno));
		if(errno != 0) return(0);
	}
	conf.shm_perm.mode = TOUTPERMIS;
	shmctl(v,IPC_SET,&conf);
	if((errno != 0) || dbg) {
		printf("  | shmctl(IPC_SET): %d, %s\n",errno,strerror(errno));
		if(errno != 0) return(0);
	}
#endif /* AJUSTE_DROITS */
	adrs = shmat(v,0,0); // SHM_RND
	if((errno != 0) || dbg) {
		printf("  | shmat: adresse 0x%08llX (%d, %s)\n",(unsigned long long)adrs,errno,strerror(errno));
		if(errno != 0) return(0);
	}
	*id = v;
	if(adrs == (void *)-1) adrs = 0;
	printf("  Partage Memoire pour %s (via %s[%ld]:%d) @0x%08llX (retour #%d: %s)\n",
		__progname,nom,taille,*id,(IntAdrs)adrs,errno,errno?strerror(errno):"successful");
	if(!adrs) printf("  => ! Memoire partagee %s inutilisable pour %s !\n",nom,__progname);
	return(adrs);

#endif
}
/* ========================================================================= */
void *SharedMemMaster(size_t taille, char *dir, char *nom, int *ident) {
	int attente; char memfile[MAXFILENAME];
	void *mbx;

	snprintf(memfile,MAXFILENAME,"%s%s",dir,nom);
	errno = 0; attente = 15; do { if(unlink(memfile) < 0) break; } while(--attente);
	if(errno == ENOENT) printf("  Ancienne memoire partagee [%s] effacee\n",nom);
	else printf("  ! Effacement de l'ancienne memoire partagee [%s] en erreur %d (%s)\n",
		nom,errno,strerror(errno));

	if((mbx = SharedMemMake(nom,taille,ident))) bzero(mbx,taille);

	return(mbx);
}
/* ========================================================================= */
void *SharedMemLink(char *nom, int id) {
#ifdef OS9
	if(((int)modstart=modlink(nom,0x400)) == (int)ERROR) return(ERROR);
	else return( (void *)modstart + modstart->data_offset );
#else
	return(shmat(id,0,0)); // SHM_RND
#endif
}
/* ========================================================================= */
void *SharedMemAdrs(char *nom, size_t taille, int *id) {
#ifdef OS9
	if(((int)modstart=modlink(nom,0x400)) == (int)ERROR) {
		if(((int)modstart=_mkdata_module(nom,taille,0x8001,0x333)) == (int)ERROR)
			return(ERROR);
	}
	return( (void *)modstart + modstart->data_offset );
#else
	void *adrs;
	if((adrs = shmat(*id,0,0)) == (void *)-1) adrs = SharedMemMake(nom,taille,id); // SHM_RND
	return(adrs);
#endif
}
/* ========================================================================= */
int SharedMemUnlk(char *nom, void *adrs) {
#ifdef OS9
	if(munload(nom,0) == ERROR) return(ERROR);
	else return(0);
#else
	return(shmdt(adrs));
#endif
}
/* ========================================================================= */
int SharedMemKill(char *nom, void *adrs, int id) {
	int i = 256;
#ifdef OS9
	while((munload(nom,0) != ERROR) && i) i-- ; return(i);
#else
	while((shmdt(adrs) != -1) && i) i-- ;
	shmctl(id,IPC_RMID,0);
	return(i);
#endif
}
/* ========================================================================= */
void SharedMemRaz(char *nom, int id) {
#ifdef OS9
	register int i;
	i = 256; while((munload(nom,0) != ERROR) && i) i-- ;
#else
	struct shmid_ds buffer;
	shmctl(id,IPC_RMID,&buffer);
#endif
}
/* ========================================================================= */
void SharedMemRazAll(int depart, int arrivee) {
	int i;
	struct shmid_ds buffer;

	for(i=depart; i<arrivee; i++) shmctl(i,IPC_RMID,&buffer);
}
/* ========================================================================= */
void *SharedMemLoad(char *nom, int *id, char *path) {
	void *adrs;
#ifdef OS9
	char commande[80];
#endif

	if((adrs = SharedMemLink(nom,*id)) == (void *)ERROR) {
		if(path) {
#ifdef OS9
			strcpy(commande,"load -d ");
			strcat(commande,path);
			strcat(commande,"/");
			strcat(commande,nom);
			strcat(commande," <>>>/nil");
			system(commande);                /*   load module from disk  */
#else
			/* pour UNIX: creer la zone (d'ou pointeur <adrs> et ident <id>),
			   lire le fichier avec read(n,<adrs>,..)
			*/
#endif
			if((adrs = SharedMemLink(nom,*id)) == (void *)ERROR) {
#ifdef DEBUG
				printf("(SharedMemload) commande %s en erreur\n",commande);
#endif
				/*
				return(SharedMemMake(nom,taille));
				*/
			} else {
			        SharedMemUnlk(nom,adrs);
#ifdef DEBUG
				printf("(SharedMemload) %s lu du fichier\n",nom);
#endif
			}
		}
	}
#ifdef DEBUG
	  else printf("(SharedMemLoad) %s deja charge\n",nom);
#endif
	return(adrs);
}
