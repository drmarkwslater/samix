/*
 *  sequences.h
 *  Samba
 *
 *  Created by Michel GROS on 29.02.12.
 *  Copyright 2012 CEA/DSM/DAPNIA. All rights reserved.
 *
 */

#ifndef SEQUENCES_H
#define SEQUENCES_H

#ifdef SEQUENCES_C
#define SEQUENCES_VAR
#else
#define SEQUENCES_VAR extern
#endif

#include <environnement.h>

// #define SEQUENCE_PROGRAMMABLE
// Utiliser goto implique une reflexion sur le parametrage des commandes
// (saisie, syntaxe fichier, utilisation de SequenceVar),
// sachant que le parm visible de goto devrait etre un label, ou #<numseq> au choix.
// On pourait utiliser duree comme #seq, ou plutot utiliser partout parm[0]
// comme duree (en supprimant duree), a condition de ne pas verrouiller 
// la syntaxe possible des parms...
// Voir proposition dans le fichier Prefs_MiG/Sequencement.

#define SEQUENCE_MAX 16
#define SEQUENCE_VARS 4
typedef enum {
	SEQUENCE_ACQ = 0,
	SEQUENCE_REGEN,
	SEQUENCE_STREAM,
	SEQUENCE_SPECTRES,
	SEQUENCE_SCRIPT,
#ifdef TESTE_MEMOIRE
	//	SEQUENCE_SI,
	SEQUENCE_GOTO,
#endif
	SEQUENCE_CODEMAX
} SEQUENCE_CODES;

SEQUENCES_VAR char SequenceCles[MAXLISTENOMS];
SEQUENCES_VAR char *SequenceFctn[SEQUENCE_CODEMAX+1]
#ifdef SEQUENCES_C
 = {
	"acquisition", "regeneration", "stream", "spectres", "script",
	#ifdef SEQUENCE_PROGRAMMABLE
	//	"si",
	"goto", 
	#endif
	"\0"
}
#endif
;

typedef struct {
	char label[9];
	char code;
	union {
		int duree;   /* minutes */
		char nom[MAXFILENAME];
	} p;
} TypeLectSequence;

#ifdef SEQUENCE_PROGRAMMABLE
static struct {
	char nom[16];
	float val;
} SequenceVar[SEQUENCE_VARS]; /* ensemble des parametres autres que la duree */
#endif

SEQUENCES_VAR TypeLectSequence Sequence[SEQUENCE_MAX];
SEQUENCES_VAR int  SequenceNb;
SEQUENCES_VAR int  SequenceCourante;
SEQUENCES_VAR long SequenceDateChange;
SEQUENCES_VAR char SequenceRegenAsuivre;

void SequencesLit();
int  SequencesImprime();
int  SequencesEdite();
void SequenceDemarre(char *derniere);
int  SequenceSuivante();
char SequenceCodeCourant();
int  SequenceParms(char log);

#endif
