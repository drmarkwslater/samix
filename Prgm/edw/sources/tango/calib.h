#ifndef CALIB_H
#define CALIB_H

#include <defineVAR.h>

#define MAXEVTNOM 64
#define MAXEVTMOYEN 16
// #define MAXEVTDATA 32768
#define MAXEVTDATA 100000

typedef struct {
	char nom[MAXEVTNOM];
	short debut,taille;
	float amplitude;
	int dep,arr;
} TypeEvtUnite;

TANGO_VAR TypeEvtUnite EvtUnite[MAXEVTMOYEN];
TANGO_VAR float EvtData[MAXEVTDATA];
TANGO_VAR int   EvtUniteNb,EvtDataNb;

int   EvtUniteCherche(char *nom, float **adrs, int *lngr, int *dep, int *arr);
float EvtUniteAmplitude(char *nom);
int   EvtUniteCalcule(char *nom, float energie_moyenne, char manu, char affichages);
int   EvtUniteSauve(char *nom);
int   EvtUniteRelit(char *nom);
int   EvtUniteFitteEnergies(char *nom, char affichages);
void  EvtUniteInit() ;

void ProdChannelBasics(float *adrs, int dim, float *base);

#endif

