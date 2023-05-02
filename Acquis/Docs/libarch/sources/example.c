#include <environnement.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <decode.h>
#include <rundata.h>
#include <evtread.h>
#include <archive.h>

/* ========================================================================== */
int main(int argc, char *argv[]) {
	char home[256];
	char rundir[256];        /* directory where event runs can be found     */
	char runname[256];        /* name of a run to be scanned                 */
	char runpath[256];       /* full path of the run                        */
	char explaination[256];  /* reason of a failure in the evtread library  */
	int max_evts;            /* maximum event to get from the run           */
	int num;                 /* event number to print                       */
	int i,voie,vt,vm;        /* indexes used to print the event             */

	/* initialisation to avoid lengthy user entries */
	RepertoireInit(home);
	// sprintf(rundir,"%s/Samba/Runs/events/",getenv("HOME"));
	strcat(strcpy(rundir,home),"Samba/donnees/events/");
	printf("Default run directory: %s\n",rundir);

	/* initialisation of the evtread library */
	max_evts = 1000000;
	if(!ArchReadInit(max_evts)) {
		printf("! Initialisation impossible (%s)\n",strerror(errno));
		return(errno);
	}

	/* query only the filename of the run */
	printf("Run to scan => "); fflush(stdout);
	scanf("%s",runname);

	/* build the full path and open the run (the folder, i.e. all the slices) */
	if(runname[0] == '/') {
		strcpy(runpath,runname);
		RepertoireExtrait(runpath,rundir,runname);
		printf("Run %s to be found in %s\n",runname,rundir);
	} else strcat(strcpy(runpath,rundir),runname);
	if(!ArchRunOpen(runpath,1,explaination)) {
		printf("! Run %s unreadable:\n! %s\n! Scan exited\n",runpath,explaination);
		return(errno);
	}

	/* loop on event requests */
	do {
		printf("Event number to read (-1 to exit) => "); fflush(stdout);
		scanf("%d",&num);
		if(num < 0) break;
		/* this retrieves the event and all the related information,
		   which is put in predefined variables */
		if(!ArchEvtGet(num,explaination)) {
			printf("! Event %d unreadable: %s\n",num,explaination);
			continue;
		}
		/* prints some of the information available about this event */
		printf("Event #%d occured at %d.%06d s (trigged on detector %5s, channel '%s')\n",
			ArchEvt.num,ArchEvt.sec,ArchEvt.msec,Bolo[ArchEvt.bolo_hdr].nom,
			VoieManip[ArchEvt.voie_hdr].nom);
		printf("  %d channels recorded\n",ArchEvt.nbvoies);
		for(vt=0; vt<ArchEvt.nbvoies; vt++) {
			voie = ArchEvt.voie[vt].num;
			vm = VoieManip[voie].type;
			printf("  channel '%s' (type '%s', %d values were saved):",VoieManip[voie].nom,ModeleVoie[vm].nom,ArchEvt.voie[vt].dim);
			for(i=0; i<ArchEvt.voie[vt].dim; i++) {
				if(!(i % 10)) printf("\n    %5d:",i);
				printf(" %10d",VoieEvent[voie].brutes.t.sval[i]);
			}
			printf("\n");
		}

	} while(1);

	/* mandatory only if a further run is to be open */
	ArchRunClose();

	return(0);
}
