/*
 *  organigramme.h
 *  pour le projet Samba
 *
 *  Created by Michel GROS on 28.02.12.
 *  Copyright 2012 CEA/DSM/DAPNIA. All rights reserved.
 *
 */
#ifndef ORGANIGRAMME_H
#define ORGANIGRAMME_H

#ifdef ORGANIGRAMME_C
#define ORGANIGRAMME_VAR
#else
#define ORGANIGRAMME_VAR extern
#endif

#include <environnement.h>

#include <stdio.h>

#include <opium.h>

typedef enum {
	ORGA_MAG_REPART = 1,
	ORGA_MAG_NUMER,
	ORGA_MAG_CABLG,
	ORGA_MAG_DETEC,
	ORGA_SPC_NUMER,
	ORGA_SPC_DETEC,
	ORGA_MAG_ORDI,
	ORGA_MAG_NBMODES
} ORGA_MODES;
#define ORGA_MAG_MAX ORGA_MAG_DETEC

typedef enum {
	ORGA_ZONES = 0,
	ORGA_ICONES,
	ORGA_STYLES_NB
} ORGA_STYLE;

typedef struct {
	TypeFigLien trace;
	int x,y;
} TypeOrgaLien;
typedef struct {
	Cadre planche;
	TypeOrgaLien *lien;
	int nb,max,derniere;
	int mont_nb,desc_nb;
	char croise;
} TypeOrgaLiaisons;

ORGANIGRAMME_VAR char OrgaStyle,OrgaMagMode;
ORGANIGRAMME_VAR char OrgaMagModifie[ORGA_MAG_MAX+1];
ORGANIGRAMME_VAR Cadre bSchemaManip,bEspaceNumer,bEspaceDetec,bOrgaCryostat,bSchemaCablage;
ORGANIGRAMME_VAR Cadre bNumeriseurSelecte,bDetecteurSelecte;
ORGANIGRAMME_VAR char  OrgaNumeriseursChoisis,OrgaDetecteursChoisis,OrgaRepartAfficheCryo;
// ORGANIGRAMME_VAR char  OrgaReponse;

void OrgaInit();
int  OrgaSelectionNumeriseurConstruit(char exclusive);
int  OrgaSelectionDetecteurConstruit(char exclusive);
int  OrgaEspaceNumeriseur(Menu menu, MenuItem item);
int  OrgaEspaceDetecteur(Menu menu, MenuItem item);
int  OrgaMagasinRefait();
int  OrgaMagasinVisite(Menu menu, MenuItem item);
int  OrgaMagasinEnregistre(Menu menu, MenuItem item);
int  OrgaTraceTout(Menu menu, MenuItem item);
int  OrgaDetecteurMontre(Menu menu, MenuItem item);
int  OrgaImporte(Menu menu, MenuItem item);
int  OrgaExporte();
char OrgaEcritCSV(char *fichier);

#endif
