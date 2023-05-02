#ifndef SNCF_H
#define SNCF_H

#ifdef SNCF_C
	#define SNCF_VAR
#else
	#define SNCF_VAR extern
#endif

#include <shared_memory.h>
#include <dimensions.h>

#define SAMBA_SHR_NOM "SambaMem"
#define SAMBA_SHR_CLE 2018
#define SAMBA_SHR_PORT 2016
#define SAMBA_SHR_FCTN 12
#define SAMBA_SHR_MAXPTS 1024
typedef enum {
	SAMBA_SHR_SUIVI = 0,
	SAMBA_SHR_FOCUS,
	SAMBA_SHR_PLOTNB
} SAMBA_SHR_PLOTS;

SNCF_VAR char *SambaMemPlotListe[SAMBA_SHR_PLOTNB+1]
#ifdef SNCF_C
= { "Suivi", "Focus", "\0" }
#endif
;

//- #define SAMBA_SHR_MAX 256
//- SNCF_VAR char SambaMemFile[SAMBA_SHR_MAX];
SNCF_VAR int SambaMemCle;
SNCF_VAR int SambaMemId;
SNCF_VAR int SambaMemPort;
typedef struct {
	int catg;
	char uniteX[12],uniteY[12];
	int nb,prem;
	float val[SAMBA_SHR_MAXPTS];
	float min,max;
} TypeSambaCourbe;

typedef struct {
	char origine[SAMBA_SHR_FCTN];
	char validite[32];
	char run[12],duree[12];
	char lance_run,en_cours,trigger_demande,archive_demandee,change_focus; short sessions;
	int evts_vus,evts_sauves;
	float taux_actuel,taux_global;
	int nb_categs,catg_sonore;
	struct {
		char nom[CATG_NOM];
		float freq;
	} categ[CATG_MAX+1];
	TypeSambaCourbe courbe[SAMBA_SHR_PLOTNB];
} TypeSambaShared;
SNCF_VAR TypeSambaShared *SambaInfos;
SNCF_VAR int  SambaShrCatg[SAMBA_SHR_PLOTNB];
SNCF_VAR char SambaMemAttachee;

#endif /* SNCF_H */
