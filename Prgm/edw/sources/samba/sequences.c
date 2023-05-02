/*
 *  sequences.c
 *  Samba
 *
 *  Created by Michel GROS on 29.02.12.
 *  Copyright 2012 CEA/DSM/DAPNIA. All rights reserved.
 *
 */

#include <environnement.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <decode.h>
#include <dateheure.h>
#include <samba.h>
#include <objets_samba.h>

#define SEQUENCES_C
#include "sequences.h"

static Panel pLectSequences;

static TypeLectSequence SequenceLue;

static ArgDesc SequenceDureeDesc[]   = { { "duree", DESC_INT &(SequenceLue.p.duree), 0 }, DESC_END };
static ArgDesc SequenceSansDesc[]    = { DESC_END };
static ArgDesc SequenceFichierDesc[] = { { "fichier", DESC_STR(MAXFILENAME) SequenceLue.p.nom, 0 }, DESC_END };
static ArgDesc *SequenceParmDesc[] = { SequenceDureeDesc, SequenceDureeDesc, SequenceDureeDesc, SequenceSansDesc, SequenceFichierDesc, 0 };

static ArgDesc SequenceDesc[] = {
	{ 0, DESC_KEY SequenceCles,          &(SequenceLue.code), 0 },
	{ 0, DESC_VARIANTE(SequenceLue.code)   SequenceParmDesc,  0 },
	DESC_END
};
static ArgStruct SequenceAS  = { (void *)Sequence, (void *)&SequenceLue, sizeof(TypeLectSequence), SequenceDesc };
static ArgDesc SequencesDesc[] = {
	{ "Programme", DESC_STRUCT_SIZE(SequenceNb,SEQUENCE_MAX) &SequenceAS, 0 },
	DESC_END
};

/* ========================================================================== 
static void SequencesInit() {
	
	SequenceNb = 0;
	
	Sequence[SequenceNb].label[0] = '\0';
	Sequence[SequenceNb].code = SEQUENCE_ACQ;
	Sequence[SequenceNb].p.duree = LectDureeAcq;
	SequenceNb++;
	
	Sequence[SequenceNb].label[0] = '\0';
	Sequence[SequenceNb].code = SEQUENCE_STREAM;
	Sequence[SequenceNb].p.duree = LectDureeStream;
	SequenceNb++;
	
	Sequence[SequenceNb].label[0] = '\0';
	Sequence[SequenceNb].code = SEQUENCE_REGEN;
	Sequence[SequenceNb].p.duree = LectDureeRegen;
	SequenceNb++;
}
   ========================================================================== */
void SequencesLit() {

	ArgKeyFromList(SequenceCles,SequenceFctn);
	// printf("* Instructions des sequences: '%s'\n",SequenceCles);
	
	SequenceNb = 0;
	if(NomSequences[0] && strcmp(NomSequences,"neant")) {
		if(SambaLogDemarrage) printf("= Lecture des sequences d'acquisition   dans '%s'\n",FichierPrefSequences);
		RepertoireCreeRacine(FichierPrefSequences);
		ArgScan(FichierPrefSequences,SequencesDesc);
		if(!SequenceNb) {
			Sequence[SequenceNb].label[0] = '\0';
			Sequence[SequenceNb].code = SEQUENCE_ACQ;
			Sequence[SequenceNb].p.duree = LectDureeAcq;
			SequenceNb++;
		}
	} else if(SambaLogDemarrage) printf("  Pas de fichier de sequences d'acquisition\n");
	SequenceCourante = 0;
	SequenceRegenAsuivre = 0;
	pLectSequences = 0;
	
	return;
}
/* ========================================================================== */
static void SequencesEcrit() {	
	printf("* Sequencement des runs");
	RepertoireCreeRacine(FichierPrefSequences);
	if(ArgPrint(FichierPrefSequences,SequencesDesc) > 0)
		printf(" sauve dans '%s'\n",FichierPrefSequences);
	else printf(" non sauvegarde (%s: %s)\n",FichierPrefSequences,strerror(errno));
}
/* ========================================================================== */
int SequencesImprime() {
	int i,l;
	
	LogBlocBegin();
	if(LectAutoriseSequences) {
		printf(" ____________________________________________________________________________________\n");
		printf("|                                Sequencement du run                                 |\n");
		printf("|____________________________________________________________________________________|\n");
		for(i=0; i<SequenceNb; i++) {
			l = printf("| %2d/ %-9s%c %-12s",i+1,Sequence[i].label,
					   Sequence[i].label[0]? ':': ' ',
					   SequenceFctn[Sequence[i].code]);
					   if(Sequence[i].code < SEQUENCE_SPECTRES) printf(" (%7d mn )",Sequence[i].p.duree);
					   else if(Sequence[i].code == SEQUENCE_SCRIPT) printf(" %s",Sequence[i].p.nom);
			if(i == SequenceCourante) l += printf("  // sequence a executer maintenant");
			SambaCompleteLigne(86,l);
		}
		printf("|____________________________________________________________________________________|\n\n");
	} else {
		printf(" ____________________________________________________________________________________\n");
		printf("|                            Pas de sequencement du run                              |\n");
		printf("|____________________________________________________________________________________|\n");
	}
	LogBlocEnd();
	return(0);
}
/* ========================================================================== */
int SequencesEdite() {
	int cols; int i,j,x,y;
	char replace,manque_infos;
	char action[SEQUENCE_MAX];
	
	do {
		cols = 3;
		if(pLectSequences && (PanelItemMax(pLectSequences) != (cols * (SequenceNb + 1)))) {
			OpiumGetPosition(pLectSequences->cdr,&x,&y); replace = 1;
			PanelDelete(pLectSequences);
			pLectSequences = 0;
		} else replace = 0;
		if(pLectSequences == 0) {
			pLectSequences = PanelCreate(cols * (SequenceNb + 1));
			OpiumPosition(pLectSequences->cdr,500,50);
		}
		PanelErase(pLectSequences);
		PanelColumns(pLectSequences,cols);
		PanelRemark(pLectSequences,"action");
		for(i=0; i<SequenceNb; i++) {
			action[i] = 0;
			PanelKeyB(pLectSequences,0," /inserer/retirer",&(action[i]),8);
		}
		PanelRemark(pLectSequences,"sequence");
		for(i=0; i<SequenceNb; i++) PanelListB(pLectSequences,0,SequenceFctn,&(Sequence[i].code),16);
		PanelRemark(pLectSequences,"duree(mn)");
		for(i=0; i<SequenceNb; i++) PanelInt(pLectSequences,0,&(Sequence[i].p.duree));
		if(replace) OpiumPosition(pLectSequences->cdr,x,y);
		
		if(OpiumExec(pLectSequences->cdr) == PNL_CANCEL) return(-1);
		manque_infos = 0;
		for(i=0; i<SequenceNb; i++) switch(action[i]) {
			case 1:  /* inserer sequence */
				if(SequenceNb >= SEQUENCE_MAX) {
					OpiumError("Pas plus de %d sequences",SEQUENCE_MAX);
					i = SequenceNb;
					break;
				}
				for(j=SequenceNb; j>i; --j) {
					memcpy(&(Sequence[j]),&(Sequence[j-1]),sizeof(TypeLectSequence));
					if(j > (i + 1)) action[j] = action[j-1]; else action[j] = 0;
				}
				i++;
				SequenceNb += 1;
				manque_infos = 1;
				break;
			case 2:  /* retirer sequence */
				SequenceNb -= 1;
				for(j=i; j<SequenceNb; j++) {
					memcpy(&(Sequence[j]),&(Sequence[j+1]),sizeof(TypeLectSequence));
					action[j] = action[j+1];
				}
				i -= 1;
				break;
		}
	} while(manque_infos);
	
	//	SequencesImprime();
	SequencesEcrit();
	return(0);
}
/* ========================================================================== */
void SequenceDemarre(char *derniere) {
	int i;

	for(i=0; i<SequenceNb; i++) if(Sequence[i].code == SEQUENCE_SPECTRES) Sequence[i].p.duree = 1;
	/* CalcSpectreAutoParms(); EA ne veut pas qu'on demande a chaque fois */
	SequenceCourante = 0; /* On doit demarrer avec les bons parms de la 1ere sequence utilisable */
	if(SequenceNb) while(Sequence[SequenceCourante].p.duree == 0) {
		if(++SequenceCourante >= SequenceNb) break;
	}
	if(SequenceCourante >= SequenceNb) {
		Sequence[SequenceNb].label[0] = '\0';
		Sequence[SequenceNb].code = SEQUENCE_ACQ;
		Sequence[SequenceNb].p.duree = 1;
		SequenceNb++;
	}
	*derniere = (SequenceCourante >= (SequenceNb - 1));
}
/* ========================================================================== */
int SequenceSuivante() {
	do {
		SequenceCourante++;
#ifdef SEQUENCE_PROGRAMMABLE
		if(SequenceCourante >= SequenceNb) {
			Acquis[AcquisLocale].etat.active = 0;
			LectAutreSequence = 0;
			return(0);
		}
#else
		if(SequenceCourante >= SequenceNb) SequenceCourante = 0;
#endif
	} while(Sequence[SequenceCourante].p.duree == 0);
	return(1);
}
/* ========================================================================== */
char SequenceCodeCourant() { return(Sequence[SequenceCourante].code); }
/* ========================================================================== */
int SequenceParms(char log) {
#ifdef SEQUENCE_PROGRAMMABLE
	int n1,n2,n3;
	n1 = Sequence[SequenceCourante].parm[0];
	n2 = Sequence[SequenceCourante].parm[1];
	n3 = Sequence[SequenceCourante].parm[2];
#endif
	
	if(RegenManuelle) return(1);
	if(Sequence[SequenceCourante].code != SEQUENCE_SPECTRES)
#ifndef CODE_WARRIOR_VSN
		SequenceDateChange = LectDateRun.tv_sec + (long)Sequence[SequenceCourante].p.duree * 60;
#else
		SequenceDateChange = SequenceDateChange + (long)Sequence[SequenceCourante].p.duree * 60;
#endif
	if(log) {
		printf("%s/ Sequence #%d: %s",DateHeure(),
			   SequenceCourante+1,SequenceFctn[Sequence[SequenceCourante].code]);
#ifdef SEQUENCE_PROGRAMMABLE
		if(Sequence[SequenceCourante].code == SEQUENCE_GOTO) printf(" sequence #%d\n",n1);
		else 
#endif
		{
			if(Sequence[SequenceCourante].code != SEQUENCE_SPECTRES)
				printf(" pour %d mn",Sequence[SequenceCourante].p.duree);
			printf(" (mode '%s' en cours)\n",MiniStream? "stream": "events");
		}
	}
	if(Sequence[SequenceCourante].p.duree) switch(Sequence[SequenceCourante].code) {
		case SEQUENCE_ACQ:
			if(MiniStream) {
				LectStreamNum++;
				MiniStream = 0; LectFixePostTrmt();
				ArchiveOuverte[EVTS] = 0; ArchEcrites[EVTS] = 0;
				Acquis[AcquisLocale].etat.active = 0;
				printf("%s/ Passage du mode 'stream' a 'events', arret de l'acquisition\n",DateHeure());
			}
			if(log) printf("%s/ Regeneration actuellement %s, on %s\n",DateHeure(),
						   RegenEnCours?"en cours":"arretee",RegenEnCours?"l'arrete":"continue comme ca");
			if(RegenEnCours) LectRegenStoppe(0);
			break;
		case SEQUENCE_REGEN:
			if(MiniStream) {
				LectStreamNum++;
				MiniStream = 0; LectFixePostTrmt();
				ArchiveOuverte[STRM] = 0; ArchEcrites[STRM] = 0;
				Acquis[AcquisLocale].etat.active = 0;
				printf("%s/ Passage du mode 'stream' a 'regen', arret de l'acquisition\n",DateHeure());
				SequenceRegenAsuivre = 1; /* pour ne mettre en regen qu'en debut de la sequence dediee a cela */
			}
			if(log) printf("%s/ Regeneration actuellement %s, on %s\n",DateHeure(),
						   RegenEnCours?"en cours":"arretee",RegenEnCours?"continue comme ca":(SequenceRegenAsuivre? "la lancera au redemarrage":"la lance"));
			if(!RegenEnCours && !SequenceRegenAsuivre) LectRegenLance(1);
			break;
		case SEQUENCE_STREAM:
			if(!MiniStream) {
				if(ArchEcrites[STRM]) { if(RegenEnCours) LectRegenNum++; else (LectTrancheNum[STRM])++; }
				ArchiveOuverte[STRM] = 0; ArchEcrites[STRM] = 0;
				Acquis[AcquisLocale].etat.active = 0;
				ArchStampsSauves = LectStampsLus; /* pour empecher l'archive finale lancee car Active=0 */
				printf("%s/ Passage au mode 'stream', arret de l'acquisition\n",DateHeure());
			}
			MiniStream = 1; LectFixePostTrmt();
			break;
		case SEQUENCE_SPECTRES:
			if(MiniStream) {
				LectStreamNum++;
				MiniStream = 0; LectFixePostTrmt();
				ArchiveOuverte[EVTS] = 0; ArchEcrites[EVTS] = 0;
				Acquis[AcquisLocale].etat.active = 0;
				printf("%s/ Passage du mode 'stream' a 'events', arret de l'acquisition\n",DateHeure());
			}
			if(!LectSpectreTampons()) break;
			LectModeSpectresAuto = 1;
#ifndef CODE_WARRIOR_VSN
			gettimeofday(&CalcDateSpectre,0);
#endif
			break;
		case SEQUENCE_SCRIPT:
			break;
#ifdef SEQUENCE_PROGRAMMABLE
		case SEQUENCE_GOTO:
			SequenceCourante = n1 - 1;
			SequenceParms(log); /* pour re-switcher de suite */
			break;
#endif
	}
	return(0);
}
