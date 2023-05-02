#include <stdio.h>
#include <string.h> /* strlen */
#include <utils/Paw.h>

//------------------------------------------------------
// defintion of PAWC FORTRAN COMMON
//------------------------------------------------------
#define LPAWC 900000
#define Comm_PAWC pawc_

struct {
	long histo[LPAWC];
} Comm_PAWC;

static int HbLpawc = LPAWC;    
static int HbLun = 1, HbLrec = 1024, HbIstat;

/* ========================================================================== */
void PawInit(void) { HLIMIT (HbLpawc); }
/* ========================================================================== */
void PawFileOpen(int id, char *chemin, char *fichier, float *bloc, char *noms) {
	HROPEN (HbLun, chemin, fichier, "N", HbLrec, HbIstat);
	HBNT   (id,"SAMBA"," ");  
	HBNAME (id,"Event",bloc,noms);
}
/* ========================================================================== */
void PawFileClose(char *chemin) {
	int zero = 0, icycle = 0;
//	int error;
//	char fnewhist[100];
//	void chmod( char *, int );

//	HCDIR ("//PAWC", " ");
	HCDIR(chemin, " ");
	HROUT(zero,icycle,"T");
	HREND(chemin);
//	HCDIR ("//PAWC", " ");

}
