#ifndef OBJETS_SAMBA_H
#define OBJETS_SAMBA_H

typedef struct {
	char LectMode;
	char Active;
	char TriggerDemande;
	char TriggerType;
	char TriggerActif;
	char ArchiveDemandee;
	char LectRunNouveau;
	char RunCategNum;
//	char SettingsRunEnvr;
	int  ArchDetecMode;
	int  ArchAssocMode;
	float polar;
	char RegenDebutJour[10],RegenDebutHeure[10];
	char RelaisDebutJour[10],RelaisDebutHeure[10];
	int  LectTauxPile;
	int  EvtTot;
//	float EvtTaux;
	float TauxGlobal;
	int MonitEvtNum;
	char  LectEcoule[20];
} TypeControleLecture;
SAMBA_VAR TypeControleLecture *cntlLecture;
SAMBA_VAR TypeControleLecture LectCntl;

#ifdef MODULES_RESEAU
#include <carla/data.h>

// #ifdef ETAT_RESEAU
// SAMBA_VAR int IdAcquis[MAXSAT];
// #endif /* ETAT_RESEAU */

#ifdef PARMS_RESEAU
typedef struct {
	char  SambaIntitule[256];  // Designation de la configuration

//	hexa  PCIadrs[PCI_MAX];     // Adresse de la carte PCI
	char  PCIdemande;          // Acces PCI demande
	char  DmaDemande;          // Lecture en DMA si c'est possible
	int   FIFOdim;             // Profondeur des FIFO (mots de 9 bits)
	char  RunCategCles[80];        // Noms des types de run
//	char  RunEnvir[256];       // Noms des conditions de run
	int   ModeleVoieNb;        // Nombre de modeles de voie
	int   BoloGeres;		   // Nombre maximum de detecteurs
	int   BoloNb;			   // Nombre effectif de detecteurs (sur toute la manip)
	int   VoiesGerees;         // Nombre maximum de voies
	int   VoiesNb;             // Nombre effectif de voies (sur toute la manip)
#ifdef FILTRES_PARTAGES
	int   FiltreNb;            // Nombre effectif de filtres generaux
#endif
	char  VoieStatus[MODL_NOM]; // Nom de la voie avec status
	char  ImprimeConfigs;      // Imprime les configs
	char  ExpertConnecte;
} TypeSambaSetup;
SAMBA_VAR TypeSambaSetup *SambaSetup;

#define MAXEVTPARTAGE 4096
typedef struct {
	int num;
	int voie,bolo;
	int lngr;
	float amplitude,montee;
	TypeDonnee val[MAXEVTPARTAGE];
} TypeEvtPartage;
SAMBA_VAR TypeEvtPartage *EvtPartage;

SAMBA_VAR int IdSetup,IdCntlLecture,IdEvtPartage;
#ifdef FILTRES_PARTAGES
SAMBA_VAR int IdFiltreGen;
#endif /* FILTRES_PARTAGES */
SAMBA_VAR TypeCarlaDataDef SambaGeneral[]
#ifdef SAMBA_C
 = {
	{ "SambaSetup",     &IdSetup,       (void **)&SambaSetup,  sizeof(TypeSambaSetup),		CARLA_NOINIT, CARLA_TEMPO },
	{ "ControleLecture",&IdCntlLecture, (void **)&cntlLecture, sizeof(TypeControleLecture), CARLA_NOINIT, CARLA_TEMPO },
	#ifdef FILTRES_PARTAGES
	{ "FiltreGeneral",  &IdFiltreGen,   (void **)&FiltreGeneral, (FLTR_MAX+1)*sizeof(TypeFiltreDef),   CARLA_NOINIT, CARLA_TEMPO },
	#endif
	{ "EvtPartage",     &IdEvtPartage,  (void **)&EvtPartage,  sizeof(TypeEvtPartage),      CARLA_NOINIT, CARLA_TEMPO },
	{ CARLA_ENDLIST }
}
#endif
;
#endif /* PARMS_RESEAU */

#ifdef SETUP_RESEAU
SAMBA_VAR int IdVoies,IdBolos;
SAMBA_VAR TypeCarlaDataDef SambaVoies[]
#ifdef SAMBA_C
= {
	{ "Voies",      &IdVoies,       (void **)&VoieManip,  sizeof(TypeVoieAlire)*VOIES_MAX, CARLA_NOINIT, CARLA_TEMPO },
	{ "Bolometres", &IdBolos,       (void **)&Bolo,       sizeof(TypeDetecteur)*DETEC_MAX,      CARLA_NOINIT, CARLA_TEMPO },
	{ CARLA_ENDLIST }
}
#endif
;
#endif /* SETUP_RESEAU */

#endif  /* MODULES_RESEAU */

#endif /* OBJETS_SAMBA_H */
