#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <defineVAR.h>
#include <claps.h>
#include <rundata.h>
#include <salsa.h>
#include <detecteurs.h>

#define SIGNATURE "* Archive SAMBA"
#define LIGNE_VERSION "* Version"
#define LIMITE_ENTETE "* ----------"
#define FIN_DE_BLOC "%s\n",LIMITE_ENTETE

/* .............. Enregistrement a envoyer a la basse de donnees ............ */
/*
 * ..... Detecteurs
 */
ARCHIVE_VAR TypeDetecteur BoloEcrit;
ARCHIVE_VAR TypeVoieAlire VoieEcrite;
ARCHIVE_VAR char ArchExplics[MAXERREURLNGR];
ARCHIVE_VAR char ArchDuree[16];
ARCHIVE_VAR int ArchEvtTot;
ARCHIVE_VAR int ArchNbPart[ARCH_TYPEDATA];
ARCHIVE_VAR float ArchiveEcrits[ARCH_TYPEDATA];

typedef enum {
	ARCH_STATUS_NONE = 0,
	ARCH_STATUS_ACTIF,
	ARCH_STATUS_ERREUR,
	ARCH_STATUS_POUM,
	ARCH_STATUS_TERMINE,
	ARCH_STATUS_NB
} ARCH_STATUS;
#define ARCH_STATUS_CLES "none/active/errored/crashed/closed"

typedef enum {
	ARCH_BLOC = 0,
	ARCH_BRUTES,
	ARCH_DIRECT,
	ARCH_NBSTREAMS
} ARCH_MODE_STREAM;
ARCHIVE_VAR char *ArchNomStream[ARCH_NBSTREAMS + 1]
#ifdef ARCHIVE_C
 = { "blocs", "brutes", "direct", "\0" }
#endif
;

ARCHIVE_VAR ArgDesc *ArchEnvirDesc;
ARCHIVE_VAR ArgDesc *ArchAutomInfo,*ArchTriggerInfo,*ArchRevInclus;
#ifndef DB_LNGRTEXTE
	#define DB_LNGRTEXTE 256
#endif
ARCHIVE_VAR char ArchiveId[DB_LNGRTEXTE];
ARCHIVE_VAR char ArchiveRev[DB_LNGRTEXTE];
ARCHIVE_VAR ArgDesc ArchRunInfoIdRev[]
#ifdef ARCHIVE_C
 = {
	{ "_id",  DESC_STR(DB_LNGRTEXTE) ArchiveId,  0 },
	{ "_rev", DESC_STR(DB_LNGRTEXTE) ArchiveRev, 0 },
	DESC_END
}
#endif
;

#ifdef ARCHIVE_C
static TypeCapteur Bolo1Capteur;
static ArgDesc ArchRunInfoCapteur[] = {
	{ "nom",        DESC_STR(MODL_NOM)    Bolo1Capteur.nom,       0 },
	{ "numeriseur", DESC_OCTET           &Bolo1Capteur.bb.serial, 0 },
	{ "ADC",        DESC_OCTET           &Bolo1Capteur.bb.adc,    0 },
	{ "rang",       DESC_SHORT           &Bolo1Capteur.voie,      0 },
	DESC_END
};
ArgStruct ArchRunInfoCaptAS = { (void *)BoloEcrit.captr, (void *)&Bolo1Capteur, sizeof(TypeCapteur), ArchRunInfoCapteur };

static TypeReglage Bolo1Reglage;
static ArgDesc ArchRunInfoReglage[] = {
	{ "nom",    DESC_STR(DETEC_NOM_LNGR)    Bolo1Reglage.nom, 0 },
	{ "valeur", DESC_STR(NUMER_RESS_VALTXT) Bolo1Reglage.std, 0 },
	DESC_END
};
ArgStruct ArchRunInfoReglAS = { (void *)BoloEcrit.reglage, (void *)&Bolo1Reglage, sizeof(TypeReglage), ArchRunInfoReglage };

static ArgDesc ArchRunInfoDetectr[] = {
	{ "nom",       DESC_TEXT                BoloEcrit.nom,     "Nom du detecteur" },
	{ "etat",      DESC_KEY DETEC_STATUTS, &BoloEcrit.lu,      "Etat ("DETEC_STATUTS")" },
	{ "position",  DESC_TEXT               &BoloEcrit.adresse, "Position dans le cryostat" },
	{ "masse(g)",  DESC_FLOAT              &BoloEcrit.masse,   "Masse (g)" },
	{ "D2",        DESC_SHORT              &BoloEcrit.d2,      "Diviseur D2" },
	{ "D3",        DESC_SHORT              &BoloEcrit.d3,      "Diviseur D3" },
	{ "Capteurs",  DESC_STRUCT_SIZE(BoloEcrit.nbcapt,DETEC_CAPT_MAX)     &ArchRunInfoCaptAS, 0 },
	{ "Reglages",  DESC_STRUCT_SIZE(BoloEcrit.nbreglages,DETEC_REGL_MAX) &ArchRunInfoReglAS, 0 },
	DESC_END
};
ArgStruct ArchRunInfoDetAS  = { (void *)Bolo, (void *)&BoloEcrit, sizeof(TypeDetecteur), ArchRunInfoDetectr };

/*
 * ..... Voies
 */

static ArgDesc ArchRunInfoVoieElec[] = {
	{ "ADCsigne",        DESC_BOOL  &VoieEcrite.signe,		     "Vrai si l'ADC est signe" },
	{ "ADCmask",         DESC_INT   &VoieEcrite.ADCmask,		 "Bits valides pour le signal" },
	{ "ADUenmV",         DESC_FLOAT &VoieEcrite.ADUenmV,         "Valeur d'1 ADU en mV a l'entree de l'ADC" },
	{ "gain_fixe",	 	 DESC_FLOAT &VoieEcrite.gain_fixe,       "Gain dans la chaine" },
	{ "gain_variable",   DESC_FLOAT &VoieEcrite.gain_variable,   "Gain post-ampli" },
	{ "modulee",         DESC_BOOL  &VoieEcrite.modulee,         "vrai si l'electronique genere une modulation" },
	{ "rapide",          DESC_NONE },
	{ "Rmodul",          DESC_FLOAT &VoieEcrite.Rmodul,          "R0modul*C0modul / Cfroide (Mohms)" },
	DESC_END
};
static ArgDesc ArchRunInfoVoieEvt[] = {
	{ "duree(ms)",       DESC_FLOAT &VoieEcrite.def.evt.duree,       "Duree evenement (millisec.)" },
	{ "pretrigger",      DESC_FLOAT &VoieEcrite.def.evt.pretrigger,  "Proportion de l'evenement avant le trigger (%)" },
	{ "temps-mort(ms)",  DESC_FLOAT &VoieEcrite.def.evt.tempsmort,   "Temps mort apres maxi evt (ms)" },
	{ "delai(ms)",       DESC_FLOAT &VoieEcrite.def.evt.delai,       "Delai evenement (millisec.)" },
	{ "phase1.t0(ms)",   DESC_FLOAT &VoieEcrite.def.evt.phase[0].t0, "Debut integrale rapide (millisec.)" },
	{ "phase1.dt(ms)",   DESC_FLOAT &VoieEcrite.def.evt.phase[0].dt, "Duree integrale rapide (millisec.)" },
	{ "phase2.t0(ms)",   DESC_FLOAT &VoieEcrite.def.evt.phase[1].t0, "Debut integrale longue (millisec.)" },
	{ "phase2.dt(ms)",   DESC_FLOAT &VoieEcrite.def.evt.phase[1].dt, "Duree integrale longue (millisec.)" },
	DESC_END
};
static ArgDesc ArchRunInfoVoieTrmt[] = {
	{ "au-vol",          DESC_KEY TrmtTypeCles, 
	                                &VoieEcrite.def.trmt[TRMT_AU_VOL].type,     "Traitement au vol (neant/demodulation/lissage/filtrage)" },
//	{ "au-vol.int",      DESC_INT   &VoieEcrite.def.trmt[TRMT_AU_VOL].parm,     "Parametre (int) du traitement au vol" },
	{ "au-vol.parm",     DESC_TEXT  VoieEcrite.def.trmt[TRMT_AU_VOL].texte,     "Parametre (char[]) du traitement au vol" },
	{ "au-vol.gain",     DESC_INT  &VoieEcrite.def.trmt[TRMT_AU_VOL].gain,      "Gain logiciel lors du traitement au vol" },
	{ "sur-tampon",      DESC_KEY TrmtTypeCles, 
								   &VoieEcrite.def.trmt[TRMT_PRETRG].type,  "Traitement pre-trigger (neant/demodulation/lissage/filtrage)" },
//	{ "sur-tampon.int",  DESC_INT  &VoieEcrite.def.trmt[TRMT_PRETRG].parm,  "Parametre (int) du traitement pre-trigger" },
	{ "sur-tampon.parm", DESC_TEXT  VoieEcrite.def.trmt[TRMT_PRETRG].texte, "Parametre (char[]) du traitement pre-trigger" },
	{ "sur-tampon.gain", DESC_INT  &VoieEcrite.def.trmt[TRMT_PRETRG].gain,  "Gain logiciel lors du traitement pre-trigger" },
	DESC_END
};
static ArgDesc ArchRunInfoVoieTrgr[] = {
	{ "type",          DESC_KEY TrgrTypeCles,  &VoieEcrite.def.trgr.type,    "Type de trigger (si trigger cable utilise)" },
	{ "sens",          DESC_KEY TRGR_SENS_CLES,   &VoieEcrite.def.trgr.sens,    "Sens du pulse recherche ("TRGR_SENS_CLES")" },
	{ "origine",       DESC_KEY TrgrOrigineCles,  &VoieEcrite.def.trgr.origine, "Origine du trigger (interne/deporte/fpga)" },
	{ "montee(ms)",    DESC_FLOAT &VoieEcrite.def.trgr.montee,     "Temps de montee minimum (ms)" },
	{ "deadline(ms)",  DESC_FLOAT &VoieEcrite.def.trgr.montmax,    "Temps de montee maximum (ms)" },
	{ "duree(ms)",     DESC_FLOAT &VoieEcrite.def.trgr.porte,      "Largeur a mi-hauteur minimum (ms)" },
	{ "ampl.pos(ADU)", DESC_FLOAT &VoieEcrite.def.trgr.minampl,    "Amplitude minimum pour evt positif (ADU)" },
	{ "ampl.neg(ADU)", DESC_FLOAT &VoieEcrite.def.trgr.maxampl,    "Amplitude maximum pour evt negatif (ADU)" },
	DESC_END
};
static ArgDesc ArchRunInfoVoieRegl[] = {
	{ "type",             DESC_KEY "neant/taux/intervalles", 
								     &VoieEcrite.def.rgul.type,    "Type de regulation (neant/taux/intervalles)" },
	{ "pos.plafond(ADU)", DESC_FLOAT &VoieEcrite.def.rgul.minsup,  "Plafond  des seuils positifs (ADU)" },
	{ "pos.plancher(ADU)",DESC_FLOAT &VoieEcrite.def.rgul.mininf,  "Plancher des seuils positifs (ADU)" },
	{ "neg.plafond(ADU)", DESC_FLOAT &VoieEcrite.def.rgul.maxsup,  "Plafond  des seuils negatifs (ADU)" },
	{ "neg.plancher(ADU)",DESC_FLOAT &VoieEcrite.def.rgul.maxinf,  "Plancher des seuils negatifs (ADU)" },
	DESC_END
};


static ArgStruct ArchRunInfoVoieElecAS = { 0, 0, 0, ArchRunInfoVoieElec };
static ArgStruct ArchRunInfoVoieEvtAS =  { 0, 0, 0, ArchRunInfoVoieEvt  };
static ArgStruct ArchRunInfoVoieTrmtAS = { 0, 0, 0, ArchRunInfoVoieTrmt };
static ArgStruct ArchRunInfoVoieTrgrAS = { 0, 0, 0, ArchRunInfoVoieTrgr };
static ArgStruct ArchRunInfoVoieReglAS = { 0, 0, 0, ArchRunInfoVoieRegl };

static ArgDesc ArchRunInfoVoie[] = {
	{ "nom",       DESC_STR(VOIE_NOM)           VoieEcrite.nom,        "Nom de la voie" },
	{ "detecteur", DESC_LISTE BoloNom,         &VoieEcrite.det,        "Detecteur d'origine" },
	{ "type",      DESC_LISTE ModelePhysListe, &VoieEcrite.physique,   "Signal physique" },
	{ "elec",	   DESC_STRUCT_DIM(1)         &ArchRunInfoVoieElecAS, 0 },
	{ "evt",	   DESC_STRUCT_DIM(1)         &ArchRunInfoVoieEvtAS,  0 },
	{ "trmt",	   DESC_STRUCT_DIM(1)         &ArchRunInfoVoieTrmtAS, 0 },
	{ "trigger",   DESC_STRUCT_DIM(1)         &ArchRunInfoVoieTrgrAS, 0 },
	{ "regul",	   DESC_STRUCT_DIM(1)         &ArchRunInfoVoieReglAS, 0 },
	DESC_END
};
/* static ArgDesc ArchRunInfoVoie[] = {
 { "nom",       DESC_STR(VOIE_NOM)           VoieEcrite.nom,        "Nom de la voie" },
 { "detecteur", DESC_LISTE BoloNom,         &VoieEcrite.det,        "Detecteur d'origine" },
 { "type",      DESC_LISTE ModelePhysListe, &VoieEcrite.physique,   "Signal physique" },
 { "elec",	    DESC_STRUCT                 &ArchRunInfoVoieElec, 0 },
 { "evt",	    DESC_STRUCT                 &ArchRunInfoVoieEvt,  0 },
 { "trmt",	    DESC_STRUCT                 &ArchRunInfoVoieTrmt, 0 },
 { "trigger",   DESC_STRUCT                 &ArchRunInfoVoieTrgr, 0 },
 { "regul",	    DESC_STRUCT                 &ArchRunInfoVoieRegl, 0 },
 DESC_END
}; */
ArgStruct ArchRunInfoVoieAS  = { (void *)VoieManip, (void *)&VoieEcrite, sizeof(TypeVoieAlire), ArchRunInfoVoie };
#endif
/*
 * ..... Structure globale de l'information
 */
ARCHIVE_VAR ArgDesc ArchRunInfo[]
#ifdef ARCHIVE_C
= {
	DESC_INCLUDE(ArchRevInclus),
	{ "Intitule",             DESC_TEXT                     SambaIntitule,         "Designation de la configuration" },
//-	{ "Type",                 DESC_KEY RunCategCles,           &RunCategNum,       "Declare au moment du lancement" },
	{ "Date",                 DESC_TEXT                     ArchDate,              "Date de lancement du run" },
	{ "Heure",                DESC_TEXT                     ArchHeure,             "Heure de debut du run" },
	{ "status",               DESC_KEY ARCH_STATUS_CLES,   &ArchInfoStatus,        "Etat du run ci-dessous designe" },
	{ "Cause",                DESC_TEXT                     ArchExplics,           "Raison de l'arret" },
	{ "Detecteurs.nb",        DESC_INT                     &BoloNb,                "Nombre effectif de detecteurs (sur toute la manip)" },
	{ "Voies.nb",             DESC_INT                     &VoiesNb,               "Nombre effectif de voies (sur toute la manip)" },
	{ "Evts.nb",              DESC_INT                     &ArchEvtTot,            "Nombre d'evenements lus" },
	{ "Duree",                DESC_TEXT                     ArchDuree,             "Duree lue" },
	{ "Fichier",              DESC_NONE }, // DESC_TEXT                     ArchiveChemin[EVTS],    0 },
	{ "Size(MB)",             DESC_NONE }, // DESC_FLOAT                   &ArchiveEcrits[EVTS],   "Nombre d'octets ecrits dans ce fichier" },
	{ "Partitions",           DESC_NONE }, // DESC_INT                     &ArchNbPart[EVTS],      "Derniere partition creee" },
	{ "Evts.fichier",         DESC_TEXT                     ArchiveChemin[EVTS],   "Chemin du fichier des evenements" },
	{ "Evts.size(MB)",        DESC_FLOAT                   &ArchiveEcrits[EVTS],   "Nombre d'octets ecrits dans ce fichier" },
	{ "Evts.partitions",      DESC_INT                     &ArchNbPart[EVTS],      "Derniere partition d'evenements creee" },
	{ "Strm.fichier",         DESC_TEXT                     ArchiveChemin[STRM],   "Chemin du fichier des streams" },
	{ "Strm.size(MB)",        DESC_FLOAT                   &ArchiveEcrits[STRM],   "Nombre d'octets ecrits dans ce fichier" },
	{ "Strm.partitions",      DESC_INT                     &ArchNbPart[STRM],      "Derniere partition de donnees brutee creee" },
	{ "Date.secondes",        DESC_INT                     &ArchT0sec,             "Date absolue de debut de run (secondes)" },
	{ "Date.microsecs",       DESC_INT                     &ArchT0msec,            "Date absolue de debut de run (microsecondes)" },
	{ "GigaStamp0",           DESC_INT                     &GigaStamp0,            "Moment du depart du run (milliards d'echantillons 100kHz)" },
	{ "TimeStamp0",           DESC_INT                     &TimeStamp0,            "Moment du depart du run (nb echantillons 100kHz modulo 1 milliard)" },
	{ "GigaStampN",           DESC_INT                     &GigaStampN,            "Moment du depart de la partition (milliards d'echantillons 100kHz)" },
	{ "TimeStampN",           DESC_INT                     &TimeStampN,            "Moment du depart de la partition (nb echantillons 100kHz modulo 1 milliard)" },
	{ DESC_COMMENT "..... Definition de l'environnement ....." },
	  DESC_INCLUDE(ArchEnvirDesc),
//	{ "Temperature",          DESC_FLOAT                    T_Bolo,		         "Temperature de la platine bolo (K)" },
	{ "Temperature(K)",	      DESC_NONE }, // DESC_FLOAT                   &ReglTbolo,          "Temperature de la platine bolo (K)" },
	{ "Tubes-pulses",	      DESC_NONE }, // DESC_KEY "arretes/actifs",   &EtatTubes,          "Etat des tubes pulses" },
#ifdef EDW_ORIGINE
	{ "Calibration",	      DESC_NONE }, // DESC_KEY "non/en-cours",     &EtatCalib,          "Etat de la calibration au demarrage" },
	{ "Regeneration",	      DESC_NONE }, // DESC_KEY "non/en-cours",     &EtatRegen,          "Etat de la regeneration au demarrage" },
	{ "Source.1.calib",	      DESC_NONE }, // DESC_KEY "absente/en-place", &(EtatSrceCalib[0]), "Etat de la 1ere source de calibration" },
	{ "Source.2.calib",	      DESC_NONE }, // DESC_KEY "absente/en-place", &(EtatSrceCalib[1]), "Etat de la 2eme source de calibration" },
	{ "Source.1.regen",	      DESC_NONE }, // DESC_KEY "absente/en-place", &(EtatSrceRegen[0]), "Etat de la 1ere source de regeneration" },
	{ "Source.2.regen",	      DESC_NONE }, // DESC_KEY "absente/en-place", &(EtatSrceRegen[1]), "Etat de la 2eme source de regeneration" },
#endif /* EDW_ORIGINE */
	  DESC_INCLUDE(ArchAutomInfo),
	{ DESC_COMMENT "..... Autres informations generales ....." },
	{ "Trigger.actif",        DESC_BOOL                     &Trigger.enservice,     "Detection d'evenement en service (sinon, c'est un stream)" },
//	{ "Trigger.Type",         DESC_KEY TrgrDefCles,        &Trigger.type,         "Type de detection des evenements (neant/cable/scrpt/perso)" },
//	{ "Trigger.scrpt",        DESC_TEXT                      Trigger.nom_scrpt,    "Nom du fichier si trigger de type scrpt" },
	{ "Trmt.sur_pretrig",     DESC_BOOL                     &SettingsCalcPretrig,  "Evalue l'evenement sur le tampon de pretrigger (les donnees brutes sinon)" },
	{ "Trmt.calcul",          DESC_KEY TrmtCalculCles,      &SettingsCalculEvt,    "Mode de calcul de l'evenement sur les voies" },
	{ "Trmt.evt.calage",      DESC_KEY CALAGES_TEXTES,      &SettingsCalageType,   "Calage des temps d'evenement ("CALAGES_TEXTES")" },
	{ "Trmt.fen.centrage",    DESC_KEY CALAGES_TEXTES,      &SettingsCentrageType, "Calage des fenetres ("CALAGES_TEXTES")" },
	{ "Trmt.datation",        DESC_KEY TrgrDateCles,        &SettingsTempsEvt,     "Datation de l'evenement" },
	{ "Trmt.pattern",         DESC_KEY TRMT_PTN_TEXTES,     &SettingsSoustra,      "Soustraction du pattern moyen ("TRMT_PTN_TEXTES")" },
	{ "Trmt.saute_evt",       DESC_BOOL                     &SettingsSauteEvt,     "Sauter evt pour calculer le pattern moyen" },
	{ "Trgr.garde_signe",     DESC_BOOL                     &TrgrGardeSigne,       "Amplitudes du signe du trigger demande" },
	{ "Sauvegarde.stream",    DESC_KEY ArchClesStream,      &ArchModeStream,       "Mode de sauvegarde si stream (blocs/brutes/direct)" },
	{ "Sauvegarde.evt",       DESC_LISTE ArchModeNoms,      &ArchDetecMode,        "Mode de sauvegarde si events (seul/coinc/voisins/tous/manip)" },
	{ "Sauvegarde.assoc",     DESC_BOOL                     &ArchAssocMode,        "Sauvegarde egalement des detecteurs associes" },
	{ "Evts.taux.max(Hz)",    DESC_FLOAT                    &LectTauxSeuil,        "Seuil d'alarme de taux d'evenement (Hz)" },
	{ "Evts.delai.min(ms)",   DESC_FLOAT                    &LectDelaiMini,        "Seuil de delai pour enregistrement (ms)" },
	{ "Sauvegarde.reduction", DESC_INT                      &ArchReduc,            "Taux de reduction du nombre d'evenements sauves" },
	{ "Sauvegarde.regen",     DESC_INT                      &ArchRegenTaux,        "Inverse du taux d'evenements sauves pendant la regeneration" },
	{ "Hote",                 DESC_STR(CODE_MAX)            SambaCodeHote,         "Code de l'hote ayant realise la lecture" },
	{ "Daq_dns",              DESC_TEXT                     SambaAdrsHote,         "Adresse IP de l'hote ayant realise la lecture" },
	{ "Release",              DESC_TEXT                     SambaRelease,          "Numero de version complet" },
	{ "author",               DESC_TEXT                     SambaMoiMeme,          "Qui qu'c'est qu'a ben pu faire tout ca?" },
	{ "doctype",              DESC_TEXT                     ArchMoiMeme,           "Quoi que je fous la, moi?" },
	{ "Detecteurs",           DESC_STRUCT_SIZE(BoloNb,DETEC_MAX)  &ArchRunInfoDetAS,  0 },
	{ "Voies",                DESC_STRUCT_SIZE(VoiesNb,VOIES_MAX) &ArchRunInfoVoieAS, 0 },
	DESC_END
}
#endif
;

ARCHIVE_VAR ArgDesc *ArchHdrNbInc;
/*
 * ..... Nombre d'entetes
 */
ARCHIVE_VAR ArgDesc ArchHdrStrmNbDesc[]
#ifdef ARCHIVE_C
= {
	{ "Bolo.presents",   DESC_SHORT &BolosPresents,     "Nombre d'entetes de detecteurs dans ce fichier" },
	{ "Voies.presentes", DESC_SHORT &ArchDeclare[STRM], "Nombre d'entetes de voies dans ce fichier" },
	{ "Voies.sauvees",   DESC_SHORT &ArchSauve[STRM],   "Nombre effectif de voies ecrites dans ce fichier" },
	DESC_END
}
#endif
;
ARCHIVE_VAR ArgDesc ArchHdrEvtsNbDesc[]
#ifdef ARCHIVE_C
= {
	{ "Bolo.presents",   DESC_SHORT &BolosPresents,     "Nombre d'entetes de detecteurs dans ce fichier" },
	{ "Voies.presentes", DESC_SHORT &ArchDeclare[EVTS], "Nombre d'entetes de voies dans ce fichier" },
	DESC_END
}
#endif
;
ARCHIVE_VAR ArgDesc *ArchHdrNbDesc[ARCH_TYPEDATA]
#ifdef ARCHIVE_C
 = { ArchHdrStrmNbDesc, ArchHdrEvtsNbDesc }
#endif
;

/* ................... Entetes des fichiers de sauvegarde ................... */
ARCHIVE_VAR ArgDesc ArchHdrGeneral[]
#ifdef ARCHIVE_C
 = {
	{ DESC_COMMENT "===== Entete generale =====" },
	{ DESC_COMMENT 0 },
	{ "Intitule",           DESC_TEXT                     SambaIntitule,        "Designation de la configuration" },
//	{ "Type",               DESC_KEY RunCategCles,           &RunCategNum,      "Type de run" },
	{ "Date",               DESC_TEXT                     ArchDate,             "Date de lancement du run" },
	{ "Heure",              DESC_TEXT                     ArchHeure,            "Heure de debut du run" },
	{ "Echantillonnage",    DESC_FLOAT                   &Echantillonnage,      "Frequence de l'ADC (kHz)" },
	{ "Frequence",          DESC_FLOAT                   &Frequence,            "Frequence des donnees ecrites (kHz)" },
	{ DESC_COMMENT 0 },
	{ "Byte-order",         DESC_KEY "little/big",       &BigEndianOrder,       "Big-endian or litte-endian unformatted data (little/big)" },
	{ "bit-trigger",        DESC_KEY SAMBA_BITNIV_CLES,  &BitTriggerNiveau,     "Assignation of the bits in the bit-trigger word ("SAMBA_BITNIV_CLES")" },
	{ "bit-numbers",        DESC_KEY SAMBA_BITNUM_CLES,  &BitTriggerNums,       "Detector or channel numbering ("SAMBA_BITNUM_CLES")" },
	{ "bit-block",          DESC_INT                     &BitTriggerBloc,       "Bits per detector, if channel-assigned (0: detector dependent)" },
	{ "Bolo.nb",            DESC_INT                     &BoloNb,               "Nombre total de detecteurs sur toute l'experience" },
	{ "Voies.nb",           DESC_INT                     &VoiesNb,              "Nombre total de voies sur toute l'experience" },
	{ "Type.donnees",       DESC_KEY ARCH_TYPE_CLES,     &ArchType,             "Type des donnees ("ARCH_TYPE_CLES")" },
	{ 0,                    DESC_VARIANTE(ArchType)       ArchHdrNbDesc, 0 },
	  DESC_INCLUDE(ArchHdrNbInc),
	{ "Fichier",            DESC_TEXT                     ArchiveCheminCourant, "Chemin du fichier sauve" },
	{ "Version",            DESC_FLOAT                   &Version,              "Version de SAMBA" },
	{ "Release",            DESC_TEXT                     SambaRelease,         "Numero de version complet" },
	{ DESC_COMMENT "..... Definition de l'environnement ....." },
	  DESC_INCLUDE(ArchEnvirDesc),
	  DESC_INCLUDE(ArchAutomInfo),

	{ "Voies",              DESC_NONE }, // DESC_TEXT   VoiesConnues,           "Noms de voies (termine par + si modulee)" },
	{ "FIFO.dim",           DESC_NONE }, // DESC_INT   &FIFOdim,            "Profondeur des FIFO (mots de 9 bits)" },
	{ "PCI.connecte",       DESC_NONE }, // DESC_BOOL  &PCIconnecte,        "Vrai si carte PCI connectee" },
	{ "Voie.status",        DESC_NONE }, // DESC_TEXT   VoieStatus,         "Nom de la voie avec status" },
	{ "VersionRepartiteur", DESC_NONE }, // DESC_FLOAT &VersionRepartiteur, "Version du repartiteur" },
	{ "Prgm.repartiteur",   DESC_NONE }, // DESC_HEXA  &PrgmRepartiteur,    "Page de programmation des IBB" },
	{ "VersionNumeriseur",  DESC_NONE }, // DESC_FLOAT &VersionIdentification,  "Version de l'identification des numeriseurs" },
	{ "Oscillateur",        DESC_NONE }, // DESC_FLOAT &HorlogeSysteme,     "Horloge fondamentale du systeme (MHz)" },
	{ "Diviseur.0",         DESC_NONE }, // DESC_INT   &Diviseur0,          "Valeur du diviseur 0" },
	{ "Diviseur.1",         DESC_NONE }, // DESC_INT   &Diviseur1,          "Valeur du diviseur 1" },
	{ "Bolo.voies",         DESC_NONE }, // DESC_INT   &ModeleVoieNb,       "Nombre de voies par detecteur" },
	{ "Trame.bolos",        DESC_NONE }, // DESC_INT   &BoloRetransmis,     "Nombre de detecteurs dans chaque trame" },
	{ "Filtres.nb",         DESC_NONE }, // DESC_INT   &FiltreNb,           "Nombre effectif de filtres generaux" },
	{ "Bolo.max",           DESC_NONE }, // DESC_INT   &BoloNb,             "Nombre effectif de detecteurs (sur toute la manip)" },
	{ "Voies.max",          DESC_NONE }, // DESC_INT   &VoiesNb,            "Nombre effectif de voies (sur toute la manip)" },
	{ "Bolo.lus",           DESC_NONE }, // DESC_INT   &BoloNb,             "Nombre effectif de detecteurs (sur toute la manip)" },
	{ "ADC.signe",          DESC_NONE },
	{ "ADC.bits",           DESC_NONE },
	{ DESC_COMMENT 0 },
	DESC_END
}
#endif
;
/*
 * ..... Detecteurs
 */
typedef struct {
	char nom[DETEC_REGL_NOM];
	char valeur[NUMER_RESS_VALTXT];
} TypeBoloReglArch;
ARCHIVE_VAR TypeBoloReglArch BoloReglageEcrit[DETEC_REGL_MAX],Bolo1ReglArch;
ARCHIVE_VAR ArgDesc ArchHdrBoloRegl[]
#ifdef ARCHIVE_C
 = {
	{ 0, DESC_STR(DETEC_REGL_NOM)    Bolo1ReglArch.nom,    0 },
	{ 0, DESC_STR(NUMER_RESS_VALTXT) Bolo1ReglArch.valeur, 0 },
	DESC_END
}
#endif
;
ARCHIVE_VAR ArgStruct ArchHdrBoloReglAS
#ifdef ARCHIVE_C
 = { (void *)BoloReglageEcrit, (void *)&Bolo1ReglArch, sizeof(TypeBoloReglArch), ArchHdrBoloRegl }
#endif
;
ARCHIVE_VAR ArgDesc ArchHdrBoloDef[]
#ifdef ARCHIVE_C
= {
	{ "Bolo.nom",             DESC_NONE }, // DESC_TEXT       BoloEcrit.nom,         "Nom du detecteur" },
	{ "Bolo.etat",            DESC_KEY DETEC_STATUTS, &BoloEcrit.lu,       "Etat ("DETEC_STATUTS")" },
	{ "Bolo.position",        DESC_SHEX               &BoloEcrit.pos,      "Position dans le cryostat" },
	{ "Bolo.index.local",     DESC_SHORT              &BoloEcrit.num_hdr,  "Index dans l'entete" },
	{ "Bolo.index.global",    DESC_SHORT              &BoloEcrit.numero,   "Index dans la liste globale" },
	{ "Bolo.masse",           DESC_FLOAT              &BoloEcrit.masse,    "Masse (g)" },
	{ "Bolo.hote",            DESC_TEXT                BoloEcrit.hote,     "Hotes charges de la lecture" },
	{ "Bolo.reglages",        DESC_STRUCT_SIZE(BoloEcrit.nbreglages,DETEC_REGL_MAX) &ArchHdrBoloReglAS, 0 },

	{ "Bolo.d2",              DESC_NONE }, // DESC_SHORT  &BoloEcrit.d2,                  "Diviseur D2" },
	{ "Bolo.d3",              DESC_NONE }, // DESC_SHORT  &BoloEcrit.d3,                  "Diviseur D3" },
	{ "Bolo.bb.serial",       DESC_NONE }, // DESC_OCTET  &BoloEcrit.captr[0].bb.serial,  "Numero de serie du numeriseur" },
	{ "Bolo.bb.adrs",         DESC_NONE }, // DESC_OCTET  &BoloEcrit.captr[0].bb.adrs,          "Adresse (donnee par le piano)" },
	{ "Bolo.ip",              DESC_NONE },
	{ "Bolo.modulation",      DESC_NONE }, // DESC_SHORT  &BoloEcrit.d2,                "Diviseur D2" },
	{ "Bolo.synchronisation", DESC_NONE }, // DESC_SHORT  &BoloEcrit.d3,           "Diviseur D3" },
	{ "Bolo.ampl.modul",      DESC_NONE }, // DESC_FLOAT  &BoloEcrit.tension[CMDE_AMPL_MODUL],  "Amplitude de la modulation" },
	{ "Bolo.comp.modul",      DESC_NONE }, // DESC_FLOAT  &BoloEcrit.tension[CMDE_COMP_MODUL],  "Compensation (carre inverse)" },
	{ "Bolo.corr.trngl",      DESC_NONE }, // DESC_FLOAT  &BoloEcrit.tension[CMDE_CORR_TRNGL],  "Correction des transitoires (triangle)" },
	{ "Bolo.corr.pied",       DESC_NONE }, // DESC_FLOAT  &BoloEcrit.tension[CMDE_CORR_PIED],   "Correction de piedestal" },
	{ "Bolo.polar.ionis",     DESC_NONE }, // DESC_FLOAT  &BoloEcrit.tension[CMDE_POLAR_IONIS], "Polarisation des voies ionisation (V)" },
	//	{ "Bolo.polar.voie3",  DESC_FLOAT     &BoloEcrit.tension[CMDE_POLAR_VOIE3], "Polarisation de la voie garde (V)" },
	{ "Bolo.polar.voie2",     DESC_NONE },
	{ "Bolo.polar.voie3",     DESC_NONE },
	{ "Bolo.gain.FET.voie1",  DESC_NONE },
	{ "Bolo.gain.FET.voie2",  DESC_NONE },
	{ "Bolo.gain.FET.voie3",  DESC_NONE },
	{ "Bolo.gain.BB.voie1",   DESC_NONE },
	{ "Bolo.gain.BB.voie2",   DESC_NONE },
	{ "Bolo.gain.BB.voie3",   DESC_NONE },
	{ "Bolo.chauffe.FET",     DESC_NONE },
	{ "Bolo.regul.bolo",      DESC_NONE }, // DESC_FLOAT     &BoloEcrit.tension[CMDE_REGUL_BOLO],  "Regulation du detecteur" },
	{ "Bolo.FET.chauffe",     DESC_NONE }, // DESC_FLOAT     &BoloEcrit.tension[CMDE_CHAUFFE_FET], "Chauffage des FET" },
	{ "Bolo.FET.masse",       DESC_NONE }, // DESC_BOOL      &BoloEcrit.fet_masse,                 "Mise des FET a la masse" },
	DESC_END
 }
#endif
;
/*
 * ..... Physique
 */
ARCHIVE_VAR ArgDesc ArchHdrPhysDef[]
#ifdef ARCHIVE_C
= {
	{ "liste", DESC_STR_SIZE(MODL_NOM,ModelePhysNb,PHYS_TYPES) ModelePhysNoms, 0 },
	DESC_END
}
#endif
;
/*
 * ..... Voies
 */
ARCHIVE_VAR TypeVoieArchivee ArchVoie;
ARCHIVE_VAR ArgDesc ArchHdrVoieDef[]
#ifdef ARCHIVE_C
= {
	{ "Voie.rang",              DESC_NONE }, // DESC_SHORT &VoieEcrite.numero,          "Index dans le tableau des voies" },
	{ "Voie.index.local",       DESC_SHORT &VoieEcrite.num_hdr[EVTS],    "Index dans l'entete" },
	{ "Voie.index.global",      DESC_SHORT &VoieEcrite.numero,          "Index dans la liste globale" },
	{ "Voie.type",              DESC_SHORT &VoieEcrite.physique,        "Signal physique" },
	{ "Voie.duree",             DESC_FLOAT &VoieEcrite.def.evt.duree,       "Duree evenement (millisec.)" },
	{ "Voie.pretrigger",        DESC_FLOAT &VoieEcrite.def.evt.pretrigger,  "Proportion de l'evenement avant le trigger (%)" },
	{ "Voie.temps-mort",        DESC_FLOAT &VoieEcrite.def.evt.tempsmort,   "Temps mort apres maxi evt (ms)" },
	{ "Voie.delai",             DESC_FLOAT &VoieEcrite.def.evt.delai,       "Delai evenement (millisec.)" },
	{ "Voie.rapide",            DESC_NONE },
	{ "Voie.phase1.t0",	        DESC_FLOAT &VoieEcrite.def.evt.phase[0].t0, "Debut integrale rapide (millisec.)" },
	{ "Voie.phase1.dt",	        DESC_FLOAT &VoieEcrite.def.evt.phase[0].dt, "Duree integrale rapide (millisec.)" },
	{ "Voie.phase2.t0",	        DESC_FLOAT &VoieEcrite.def.evt.phase[1].t0, "Debut integrale longue (millisec.)" },
	{ "Voie.phase2.dt",	        DESC_FLOAT &VoieEcrite.def.evt.phase[1].dt, "Duree integrale longue (millisec.)" },
	{ "Voie.ADCsigne",          DESC_BOOL  &VoieEcrite.signe,		   "Vrai si l'ADC est signe" },
	{ "Voie.ADCmask",           DESC_INT   &VoieEcrite.ADCmask,		   "Bits valides pour le signal" },
	{ "Voie.ADUenmV",           DESC_FLOAT &VoieEcrite.ADUenmV,         "Valeur d'1 ADU en mV a l'entree de l'ADC" },
	{ "Voie.gain_fixe",		    DESC_FLOAT &VoieEcrite.gain_fixe,       "Gain dans la chaine" },
	{ "Voie.gain_variable",     DESC_FLOAT &VoieEcrite.gain_variable,   "Gain post-ampli" },
	{ "Voie.RC",                DESC_FLOAT &VoieEcrite.RC,              "Temps de descente de l'ampli (ms)" },
	{ "Voie.modulee",           DESC_BOOL  &VoieEcrite.modulee,         "vrai si l'electronique genere une modulation" },
	{ "Voie.Rmodul",            DESC_FLOAT &VoieEcrite.Rmodul,          "R0modul*C0modul / Cfroide (Mohms)" },
	{ "Voie.au-vol",            DESC_KEY TrmtTypeCles, 
	                                &VoieEcrite.def.trmt[TRMT_AU_VOL].type, "Traitement au vol (neant/demodulation/lissage/filtrage)" },
	{ "Voie.au-vol.int",        DESC_INT  &VoieEcrite.def.trmt[TRMT_AU_VOL].parm,  "Parametre (int) du traitement au vol" },
	{ "Voie.au-vol.parm",       DESC_TEXT  VoieEcrite.def.trmt[TRMT_AU_VOL].texte, "Parametre (char[]) du traitement au vol" },
	{ "Voie.au-vol.gain",       DESC_INT  &VoieEcrite.def.trmt[TRMT_AU_VOL].gain,  "Gain logiciel lors du traitement au vol" },
	{ "Voie.sur-tampon",        DESC_KEY TrmtTypeCles, 
								    &VoieEcrite.def.trmt[TRMT_PRETRG].type, "Traitement pre-trigger (neant/demodulation/lissage/filtrage)" },
	{ "Voie.sur-tampon.int",    DESC_INT  &VoieEcrite.def.trmt[TRMT_PRETRG].parm,  "Parametre (int) du traitement pre-trigger" },
	{ "Voie.sur-tampon.parm",   DESC_TEXT  VoieEcrite.def.trmt[TRMT_PRETRG].texte, "Parametre (char[]) du traitement pre-trigger" },
	{ "Voie.sur-tampon.gain",   DESC_INT  &VoieEcrite.def.trmt[TRMT_PRETRG].gain,  "Gain logiciel lors du traitement pre-trigger" },
	{ "Voie.trigger.type",      DESC_KEY TrgrTypeCles, &VoieEcrite.def.trgr.type, "Type de trigger (si trigger cable utilise)" },
	{ "Voie.trigger.sens",      DESC_KEY TRGR_SENS_CLES, 
	                                             &VoieEcrite.def.trgr.sens, "Sens du pulse recherche ("TRGR_SENS_CLES")" },
	{ "Voie.trigger.ampl.pos",  DESC_FLOAT &VoieEcrite.def.trgr.minampl,    "Amplitude minimum pour evt positif (ADU)" },
	{ "Voie.trigger.ampl.neg",  DESC_FLOAT &VoieEcrite.def.trgr.maxampl,    "Amplitude maximum pour evt negatif (ADU)" },
	{ "Voie.trigger.montee",    DESC_FLOAT &VoieEcrite.def.trgr.montee,     "Temps de montee minimum (ms)" },
	{ "Voie.trigger.duree",     DESC_FLOAT &VoieEcrite.def.trgr.porte,      "Largeur a mi-hauteur minimum (ms)" },
	{ "Voie.trigger.plancher",  DESC_FLOAT &VoieEcrite.def.trgr.underflow,  "Amplitude (absolue) minimum avant rejet (ADU)" },
	{ "Voie.trigger.plafond",   DESC_FLOAT &VoieEcrite.def.trgr.overflow,   "Amplitude (absolue) maximum avant rejet (ADU)" },
	{ "Voie.trigger.mintime",   DESC_FLOAT &VoieEcrite.def.trgr.montmin,    "Temps de montee minimum avant rejet (ms)" },
	{ "Voie.trigger.deadline",  DESC_FLOAT &VoieEcrite.def.trgr.montmax,    "Temps de montee maximum avant rejet (ms)" },
	{ "Voie.regul.type",        DESC_KEY "neant/taux/intervalles", 
								           &VoieEcrite.def.rgul.type,      "Type de regulation (neant/taux/intervalles)" },
	{ "Voie.regul.pos.plafond", DESC_SHORT &VoieEcrite.def.rgul.minsup,    "Plafond  des seuils positifs (ADU)" },
	{ "Voie.regul.pos.plancher",DESC_SHORT &VoieEcrite.def.rgul.mininf,    "Plancher des seuils positifs (ADU)" },
	{ "Voie.regul.neg.plafond", DESC_SHORT &VoieEcrite.def.rgul.maxsup,    "Plafond  des seuils negatifs (ADU)" },
	{ "Voie.regul.neg.plancher",DESC_SHORT &VoieEcrite.def.rgul.maxinf,    "Plancher des seuils negatifs (ADU)" },

#ifdef OBSOLETE
	{ DESC_COMMENT "===== A ne plus utiliser =====" },
/* quelle voie? */	
	{ "Voie.horloge",       DESC_FLOAT &ArchVoie.horloge,   "Duree d'un point (millisecs)" },
	{ "Voie.lngr",          DESC_INT   &ArchVoie.dim,       "Nombre de points sauvegardes" },
	{ "Voie.trigger.pre",   DESC_INT   &ArchVoie.avant_evt, "Nombre de points dans le pre-trigger" },
#endif

	DESC_END
 }
#endif
;
/*
 * ..... Filtre
 */
ARCHIVE_VAR TypeFiltre ArchFiltre;
ARCHIVE_VAR ArgDesc ArchHdrFiltre[]
#ifdef ARCHIVE_C
= {
	{ DESC_COMMENT "===== Entete de filtre =====" },
	{ "Etages", DESC_INT &(ArchFiltre.nbetg), 0 },
	{ DESC_COMMENT 0 },

	DESC_END
}
#endif
;
/*
 * ..... Run
 */
ARCHIVE_VAR ArgDesc ArchHdrRun[]
#ifdef ARCHIVE_C
= {
	{ DESC_COMMENT "===== Entete de run =====" },
	{ DESC_COMMENT 0 },
	{ "Run",                  DESC_INT                     &NumeroLect,            "Numero de lecture absolu" },
	{ DESC_COMMENT 0 },
	{ DESC_COMMENT "..... Valeurs de l'environnement ....." },
	  DESC_INCLUDE(LectEnvirDesc),
	  DESC_INCLUDE(ArchAutomInfo),
	{ "Temperature",	      DESC_FLOAT                   &ReglTbolo,             "Temperature de la platine bolo (K)" },
	{ "Tubes-pulses",	      DESC_KEY "arretes/actifs",   &EtatTubes,             "Etat des tubes pulses" },
#ifdef EDW_ORIGINE
	{ "Source.regen",	      DESC_NONE },
	{ "Source.calib",	      DESC_NONE },
	{ "Calibration",	      DESC_NONE },// DESC_KEY "non/en-cours",     &EtatCalib,             "Etat de la calibration au demarrage" },
	{ "Regeneration",	      DESC_NONE },// DESC_KEY "non/en-cours",     &EtatRegen,             "Etat de la regeneration au demarrage" },
	{ "Source.1.calib",	      DESC_NONE },// DESC_KEY "absente/en-place", &(EtatSrceCalib[0]),    "Etat de la 1ere source de calibration" },
	{ "Source.2.calib",	      DESC_NONE },// DESC_KEY "absente/en-place", &(EtatSrceCalib[1]),    "Etat de la 2eme source de calibration" },
	{ "Source.1.regen",	      DESC_NONE },// DESC_KEY "absente/en-place", &(EtatSrceRegen[0]),    "Etat de la 1ere source de regeneration" },
	{ "Source.2.regen",	      DESC_NONE },// DESC_KEY "absente/en-place", &(EtatSrceRegen[1]),    "Etat de la 2eme source de regeneration" },
#endif /* EDW_ORIGINE */
	{ DESC_COMMENT "..... Demarrage ....." },
	{ "Heure",			      DESC_TEXT                     ArchHeure,             "Heure de debut de lecture" },
	{ "Date.secondes",        DESC_INT                     &ArchT0sec,             "Date absolue de debut de run (secondes)" },
	{ "Date.microsecs",       DESC_INT                     &ArchT0msec,            "Date absolue de debut de run (microsecondes)" },
	{ "GigaStamp0",           DESC_INT                     &GigaStamp0,            "Moment du depart du run (milliards d'echantillons 100kHz)" },
	{ "TimeStamp0",           DESC_INT                     &TimeStamp0,            "Moment du depart du run (nb echantillons 100kHz modulo 1 milliard)" },
	{ "GigaStampN",           DESC_INT                     &GigaStampN,            "Moment du depart de la partition (milliards d'echantillons 100kHz)" },
	{ "TimeStampN",           DESC_INT                     &TimeStampN,            "Moment du depart de la partition (nb echantillons 100kHz modulo 1 milliard)" },
	{ DESC_COMMENT "..... Autres informations generales ....." },
	{ "Hote",                 DESC_STR(CODE_MAX)            SambaCodeHote,         "Code de l'hote ayant realise la lecture" },
	{ "Daq_dns",              DESC_TEXT                     SambaAdrsHote,         "Adresse IP de l'hote ayant realise la lecture" },
	{ "Starter",              DESC_TEXT                     Starter,               "Nom de l'hote declenchant la lecture si multitache inactif" },
	{ "Duree.tampon",         DESC_INT                     &DureeTampons,          "Profondeur tampon par bolo et par voie (ms)" },
	{ "Duree.modulation",     DESC_NONE },
	{ "Duree.synchronisation", DESC_INT                    &Diviseur2,             "Longueur d'une synchronisation en nombre de points" },
	{ DESC_COMMENT 0 },
	{ "Donnees.source",       DESC_KEY SrceCles,           &SrceType,              "Source des donnees" },
	{ "Donnees.fichier",      DESC_TEXT                     SrceName,              "Fichier de donnees si relecture (source=reel)" },
	{ "Lect.taux.seuil",      DESC_FLOAT                    &LectTauxSeuil,        "Seuil d'alarme de taux d'evenement (Hz)" },
	{ "Lect.delai.mini",      DESC_FLOAT                    &LectDelaiMini,        "Seuil de delai pour enregistrement (ms)" },
	{ DESC_COMMENT 0 },
	{ "Trigger.actif",        DESC_BOOL                     &Trigger.enservice,     "Detection d'evenement en service (sinon, c'est un stream)" },
	{ "Trigger.Type",         DESC_NONE }, // DESC_KEY TrgrDefCles, &Trigger.type,         "Type de detection des evenements (neant/cable/scrpt/perso)" },
	{ "Trigger.scrpt",        DESC_NONE }, // DESC_TEXT               Trigger.nom_scrpt,    "Nom du fichier si trigger de type scrpt" },
	{ "Trmt.inhibe",          DESC_NONE }, // DESC_FLOAT             &SettingsInhibe,       "Inhibition sur trigger (ms) (<0 si = duree evt)" },
	{ "Trmt.sur_pretrig",     DESC_BOOL                     &SettingsCalcPretrig,  "Evalue l'evenement sur le tampon de pretrigger (les donnees brutes sinon)" },
	{ "Trmt.calcul",          DESC_KEY TrmtCalculCles,      &SettingsCalculEvt,    "Mode de calcul de l'evenement sur les voies ajoutees" },
	{ "Trmt.datation",        DESC_KEY TrgrDateCles,        &SettingsTempsEvt,     "Datation de l'evenement" },
	{ "Trmt.altivec",         DESC_BOOL                     &SettingsAltivec,      "Calcul avec altivec" },
	{ "Trmt.pattern",         DESC_KEY TRMT_PTN_TEXTES,     &SettingsSoustra,      "Soustraction du pattern moyen ("TRMT_PTN_TEXTES")" },
	{ "Trmt.sans_fltr",       DESC_BOOL                     &SettingsSansTrmt,     "Suppression du traitement pre-trigger" },
	{ "Trmt.demarrage",       DESC_BOOL                     &SettingsStartactive,  "Procedures de demarrage des detecteurs, activees" },
	{ "Trmt.maintenance",     DESC_BOOL                     &SettingsDLUactive,    "Procedures d'entretien des detecteurs, activees" },
	{ "Trmt.saute_evt",       DESC_BOOL                     &SettingsSauteEvt,     "Sauter evt pour calculer le pattern moyen" },
	{ "Trmt.calage",          DESC_NONE }, // DESC_BOOL  &SettingsCalageType,        "Calage des temps" },
	{ "Trmt.evt.calage",      DESC_KEY CALAGES_TEXTES,      &SettingsCalageType,   "Calage des temps d'evenement ("CALAGES_TEXTES")" },
	{ "Trmt.fen.centrage",    DESC_KEY CALAGES_TEXTES,      &SettingsCentrageType, "Calage des fenetres ("CALAGES_TEXTES")" },
    { "Trgr.garde_signe",     DESC_BOOL                     &TrgrGardeSigne,       "Amplitudes du signe du trigger demande" },
	{ DESC_COMMENT 0 },
	{ "Sauvegarde.stream",    DESC_KEY ArchClesStream,      &ArchModeStream,       "Mode de sauvegarde si stream (blocs/brutes/direct)" },
	{ "Sauvegarde.evt",       DESC_LISTE ArchModeNoms,      &ArchDetecMode,        "Mode de sauvegarde si events (seul/coinc/voisins/tous/manip)" },
	{ "Sauvegarde.assoc",     DESC_BOOL                     &ArchAssocMode,        "Sauvegarde egalement des detecteurs associes" },
	{ "Sauvegarde.reduction", DESC_INT                      &ArchReduc,            "Taux de reduction du nombre d'evenements sauves" },
	{ "Sauvegarde.regen",     DESC_INT                      &ArchRegenTaux,        "Inverse du taux d'evenements sauves pendant la regeneration" },
	{ DESC_COMMENT 0 },
	{ "Type",                 DESC_NONE }, // DESC_KEY RunCategCles,           &RunCategNum,       "Declare au moment du lancement" },
	{ "Condition",            DESC_NONE }, // DESC_KEY RunEnvir,           &SettingsRunEnvr,       "Declaree au moment du lancement" },
	{ "CPU.secondes",         DESC_NONE }, // DESC_INT   &ArchT0sec,               "Date absolue de debut de run (secondes)" },
	{ "CPU.microsecs",        DESC_NONE }, // DESC_INT   &ArchT0msec,              "Date absolue de debut de run (microsecondes)" },
	{ "Lect.buffer",          DESC_NONE }, // DESC_KEY "fixe/circulaire",          &SettingsModeDonnees,   "Mode d'utilisation du buffer de lecture" },
	{ "Lect.FIFO.tache",      DESC_NONE }, // DESC_KEY "serie/parallele/boostee",  &SettingsMultitasks,    "Tache de lecture (serie/parallele/boostee)" },
	{ "Lect.FIFO.periode",    DESC_NONE }, // DESC_INT                             &SettingsReadoutPeriod, "Periode d'activation de la lecture (ms)" },
	{ "Lect.auto-synchro",    DESC_NONE }, // DESC_BOOL                            &SettingsAutoSync,      "Resynchronisation auto sur donnees perdues" },
	{ "Lect.status-check",    DESC_NONE }, // DESC_BOOL                            &SettingsStatusCheck,   "Verification du status (10eme mot du bloc)" },
	{ "Lect.moduls.trmt",     DESC_NONE }, // DESC_INT                             &SettingsIntervTrmt,   "Intervalle entre traitements pre-trigger (ms)" },
	{ "Lect.buffer",          DESC_NONE },
	{ "Lect.FIFO.tache",      DESC_NONE },
	{ "Lect.FIFO.periode",    DESC_NONE },
	{ "Lect.auto-synchro",    DESC_NONE },
	{ "Lect.status-check",    DESC_NONE },
	{ "Lect.moduls.trmt",     DESC_NONE },
	{ "Lect.start.synchro",   DESC_NONE }, // DESC_KEY "neant/repart/seconde", &LectSynchroType, "Calage du demarrage de la lecture (neant/seconde/repart)" },
	DESC_END
}
#endif
;
/*
 * ..... Evenements
 */
ARCHIVE_VAR TypeEvt ArchEvt;
ARCHIVE_VAR int     ArchVoiesNb;
ARCHIVE_VAR ArgDesc ArchHdrEvt[]
#ifdef ARCHIVE_C
 = {
	{ DESC_COMMENT 0 },
	{ DESC_COMMENT "===== Entete d'evenement =====" },
	{ "Numero",              DESC_INT   &ArchEvt.num,        "Numero de l'evenement dans le run" },
	{ "Regeneration",        DESC_BOOL  &ArchEvt.regen,      "Vrai si produit pendant une regeneration" },
	{ "Date.secondes",       DESC_INT   &ArchEvt.sec,        "Compteur voie triggee (secondes)" },
	{ "Date.microsecs",      DESC_INT   &ArchEvt.msec,       "Compteur voie triggee (microsecs)" },
	{ "GigaStamp",           DESC_INT   &ArchEvt.gigastamp,  "Moment du maximum (milliards d'echantillons 100kHz)" },
	{ "TimeStamp",           DESC_INT   &ArchEvt.stamp,      "Moment du maximum (nb echantillons 100kHz modulo 1 milliard)" },
	{ "TempsMort.secondes",  DESC_INT   &ArchEvt.TMsec,      "Total temps mort (secondes)" },
	{ "TempsMort.microsecs", DESC_INT   &ArchEvt.TMmsec,     "Total temps mort (microsecs)" },
	{ "Delai",               DESC_FLOAT &ArchEvt.delai,      "Delai depuis le precedent evenement (s)" },
	{ "Trigger",             DESC_FLOAT &ArchEvt.trig,       "Delai depuis le precedent trigger (s)" },
//	{ "Temperature",         DESC_FLOAT  T_Bolo,		     "Temperature de la platine (K)" },
	{ "Temperature",	     DESC_NONE }, // DESC_FLOAT                   &ReglTbolo, "Temperature de la platine bolo (K)" },
	{ "Tubes-pulses",	     DESC_NONE }, // DESC_KEY "arretes/actifs",   &EtatTubes, "Etat des tubes pulses" },
	{ "Source.regen",	     DESC_NONE }, // DESC_KEY "absente/en-place", &EtatRegen, "Etat de la source de regeneration" },
	{ "Source.calib",	     DESC_NONE }, // DESC_KEY "absente/en-place", &EtatCalib, "Etat de la source de calibration" },
	  DESC_INCLUDE(ArchAutomInfo),
//	{ "Liste:31-0",          DESC_INT   &ArchEvt.liste[0],   "1 bit par detecteur touche (0..31)" },
//	{ "Liste:63-32",         DESC_INT   &ArchEvt.liste[1],   "1 bit par detecteur touche (32..63)" },
	  DESC_INCLUDE(ArchTriggerInfo),
	{ "Bolo.touche",         DESC_SHORT &ArchEvt.bolo_hdr,   "Index local du bolo ayant declenche le plus fort" },
	{ "Voie.touchee",        DESC_SHORT &ArchEvt.voie_hdr,   "Index local de la voie ayant declenche le plus fort" },
	{ "Voie.declenchee",     DESC_SHORT &ArchEvt.voie_evt,   "Index dans l'evt de la voie ayant declenche le plus fort" },
	{ "Voies.nb",            DESC_INT   &ArchVoiesNb,        "Nombre de voies enregistrees" },

#ifdef OBSOLETE
	{ DESC_COMMENT "===== A ne plus utiliser =====" },
	{ "Horloge.secondes",    DESC_INT   &ArchEvt.sec,        "Compteur voie triggee (secondes)" },
	{ "Horloge.microsecs",   DESC_INT   &ArchEvt.msec,       "Compteur voie triggee (microsecs)" },
	{ "CPU.secondes",        DESC_INT   &ArchEvt.sec,        "Compteur voie triggee (secondes)" },
	{ "CPU.microsecs",       DESC_INT   &ArchEvt.msec,       "Compteur voie triggee (microsecs)" },
	{ "detecteur",           DESC_INT   &ArchEvt.bolo_hdr,   "Numero du bolo ayant declenche le plus fort" },
	{ "Voie",                DESC_INT   &ArchEvt.voie_hdr,   "Numero de la voie ayant declenche le plus fort" },
/* quelle voie? */
	{ "Base",                DESC_NONE },
	{ "Bruit",               DESC_NONE },
	{ "Maxi",                DESC_NONE },
	{ "Montee",              DESC_NONE },
	{ "Amplitude",           DESC_NONE },
	{ "Dimension",           DESC_NONE },
	{ DESC_COMMENT 0 },
#endif

	DESC_END
}
#endif
;

ARCHIVE_VAR ArgDesc ArchHdrVoieEvt[]
#ifdef ARCHIVE_C
= {
	{ DESC_COMMENT 0 },
	{ DESC_COMMENT "===== Entete de voie =====" },
	{ "Numero",             DESC_INT   &ArchVoie.enregistree, "Numero de la voie dans le tableau declare" },
	{ "Compteur.secondes",  DESC_INT   &ArchVoie.sec,         "Debut de l'evenement / horloge externe (secondes)" },
	{ "Compteur.microsecs", DESC_INT   &ArchVoie.msec,        "Debut de l'evenement / horloge externe (microsecs)" },
	{ "GigaStamp",          DESC_INT   &ArchVoie.gigastamp,   "Moment relatif du debut de la trace (milliards d'echantillons 100kHz)" },
	{ "TimeStamp",          DESC_INT   &ArchVoie.stamp,       "Moment relatif du debut de la trace (nb echantillons 100kHz modulo 1 milliard)" },
	{ "Horloge",            DESC_FLOAT &ArchVoie.horloge,     "Duree d'un point (millisecs)" },
	{ "Dimension",          DESC_INT   &ArchVoie.dim,         "Nombre de points sauvegardes" },
	{ "Trigger.ampl.pos",   DESC_FLOAT &ArchVoie.trig_pos,    "Amplitude minimum pour evt positif (ADU)" },
	{ "Trigger.ampl.neg",   DESC_FLOAT &ArchVoie.trig_neg,    "Amplitude maximum pour evt negatif (ADU)" },
	{ "Pretrigger",         DESC_INT   &ArchVoie.avant_evt,   "Nombre de points dans le pre-trigger" },
	{ "Base",               DESC_FLOAT &ArchVoie.base,        "Ligne de base (ADU)" },
	{ "Reference",          DESC_KEY TRMT_CLASSES_CLES, &ArchVoie.ref, "Tampon ayant servi aux evaluations ci-dessous ("TRMT_CLASSES_CLES")" },
	{ "Bruit",              DESC_FLOAT &ArchVoie.bruit,       "Bruit (ADU)" },
//	{ "Maxi",               DESC_INT   &ArchVoie.maxi,        "Maximum (ADU)" },
	{ "Montee",             DESC_FLOAT &ArchVoie.montee,      "Temps de montee (millisecondes)" },
	{ "Descente",           DESC_FLOAT &ArchVoie.descente,    "Temps de descente (millisecondes)" },
	{ "Amplitude",          DESC_FLOAT &ArchVoie.amplitude,   "Amplitude (ADU)" },
	{ "Filtre.nb",          DESC_INT   &ArchVoie.nbfiltres,   "Nombre de points de donnees filtrees" },
	{ "Filtre.recalcul",    DESC_INT   &ArchVoie.recalcul,    "Index dans l'evt ou le recalcul doit commencer" },
	{ "CPU.secondes",       DESC_NONE }, // DESC_INT   &ArchVoie.cpusec,    "Temps CPU au pic (secondes)" },
	{ "CPU.microsecs",      DESC_NONE }, // DESC_INT   &ArchVoie.cpusec,    "Temps CPU au pic (secondes)" },
	{ DESC_COMMENT 0 },
	DESC_END
}
#endif
;

ARCHIVE_VAR int ArchVoieNum,ArchVoieDebut,ArchVoieDim;
ARCHIVE_VAR ArgDesc ArchHdrVoieXfer[]
#ifdef ARCHIVE_C
 = {
	{ DESC_COMMENT 0 },
	{ DESC_COMMENT "===== Entete de voie =====" },
	{ "Numero",            DESC_INT  &ArchVoieNum,       "Numero de la voie sauvegardee" },
	{ "Dimension",         DESC_INT  &ArchVoieDim,       "Nombre de points a lire" },
	{ DESC_COMMENT 0 },
	DESC_END
}
#endif
;
ARCHIVE_VAR ArgDesc ArchHdrFin[]
#ifdef ARCHIVE_C
= {
	{ DESC_COMMENT "===== Fin de run =====" },
	{ DESC_COMMENT 0 },
	{ "Date.secondes",      DESC_INT   &ArchT0sec,             "Date absolue de fin de run (secondes)" },
	{ "Date.microsecs",     DESC_INT   &ArchT0msec,            "Date absolue de fin de run (microsecondes)" },
	DESC_END
}
#endif
;

/* ...................... Fonctions utilisees ailleurs ...................... */
void ArchiveDefini(char log);
void ArchiveNouvellePartition(int fmt);
void ArchiveNettoie(int reserve, char imprime, const char *origine);
char ArchiveEvt(int reserve, char vidage, const char *origine);
INLINE void ArchiveVide(char sauve, const char *origine);
char ArchiveBrutes(char termine);
int  ArchiveBloc(int voie);

#endif
