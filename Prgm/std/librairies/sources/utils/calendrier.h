/*
 *  calendrier.h
 *  Librairies
 *
 *  Created by Michel GROS on 28.11.16.
 *  Copyright 2016 __MyCompanyName__. All rights reserved.
 *
 */
#ifndef CALENDRIERS_H
#define CALENDRIERS_H

#ifdef CALENDRIERS_C
#define CALENDRIERS_VAR
#else
#define CALENDRIERS_VAR extern
#endif

#include "defineVAR.h"

#define CALDR_EVTNOM 32
#define CALDR_EVTTXT 32

typedef struct {
	char titre[CALDR_EVTNOM];
	struct { int64 debut,fin; } date;
	char texte[CALDR_EVTTXT];
} TypeCaldrEvt,*CaldrEvt;

char CaldrInit(char *fichier);
int CaldrOuvre(char *nom);
int CaldrEvenements(int cal, CaldrEvt evt, int max);

#endif

