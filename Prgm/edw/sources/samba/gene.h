//  Created by Michel Gros on 15.04.21.

#ifndef GENE_H
#define GENE_H

#ifdef GENE_C
#define GENE_VAR
#else
#define GENE_VAR extern
#endif

#include <environnement.h>

#include <stdio.h>

#include <opium.h>

typedef enum {
	GENE_INTERV_POIS = 0,
	GENE_INTERV_FIXE,
	GENE_INTERV_NB
} GENE_INTERVALLE;
#define GENE_INTERV_CLES "poisson/fixe"

#define GENE_MSG_LNGR 51
typedef struct {
	char nom[MODL_NOM];
	int  voie[VOIES_MAX];
	int  nbvoies;
	int  num;
	float ldb,bruit;     /* ADU */
	float freq,pic,reso; /* Hz, ADU, % */
	float periode;       /* secondes */
	char mode_actif,mode_manu,mode_auto,interv,start;
	int evts;
	struct {
		Cadre avant;     /* planche complete */
		struct { Cadre manu,svg; } bouton;
		struct { Instrum ldb,brt,frq,amp,res; } glissiere;
		struct { Panel msg,ldb,brt,frq,amp,res; } texte;
		Panel evts;
		char message[GENE_MSG_LNGR];
	} panneau;
} TypeGene;

#define GENE_MAX VOIES_TYPES
GENE_VAR TypeGene Gene[GENE_MAX+1],GeneLu;
GENE_VAR char *GeneListe[GENE_MAX+1];
GENE_VAR int GeneNum,GeneNb;
GENE_VAR char GeneModifies;

GENE_VAR ArgDesc GeneModeDesc[]
#ifdef GENE_C
 = {
	{ 0,               DESC_STR(MODL_NOM) GeneLu.nom, 0 },
	{ "voies",         DESC_LISTE_SIZE(GeneLu.nbvoies,VOIES_MAX) VoieNom, GeneLu.voie, 0 },
	{ "niveau_DC",     DESC_FLOAT &GeneLu.ldb,    0 },
	{ "bruit",         DESC_FLOAT &GeneLu.bruit,  0 },
	{ "frequence_evt", DESC_FLOAT &GeneLu.freq,   0 },
	{ "energie_ADU",   DESC_FLOAT &GeneLu.pic,    0 },
	{ "resolution",    DESC_FLOAT &GeneLu.reso,   0 },
	{ "intervalle",    DESC_KEY "poisson/fixe", &GeneLu.interv, 0 },
	{ "autostart",     DESC_BOOL  &GeneLu.start,  0 },
	DESC_END
}
#endif
;
GENE_VAR ArgStruct GeneAS
#ifdef GENE_C
 = { (void *)Gene, (void *)&GeneLu, sizeof(TypeGene), GeneModeDesc }
#endif
;
GENE_VAR ArgDesc GeneDesc[]
#ifdef GENE_C
 = {
	{ "Simulations",   DESC_DEVIENT("Generateurs") },
	{ "Generateurs",   DESC_STRUCT_SIZE(GeneNb,GENE_MAX) &GeneAS, 0 },
	DESC_END
 }
#endif
;

int  GegeneLit();
int  GegeneEcrit(Menu menu, MenuItem item);
int  GegeneOpen(Menu menu, MenuItem item);
void GegeneConnecte(int gene);
void GegeneMontre(int gene);

#endif /* GENE_H */
