#include <environnement.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <errno.h>
#ifndef WIN32
#include <strings.h>  /* pour strlen qui rend size_t si intrinseque, et rindex */
#define MODE_OPEN O_RDONLY
#else /* WIN32 */
#include <string.h>
#include <sys/stat.h>
#define MODE_OPEN O_RDONLY | O_BINARY
#include <io.h>
#include <ConsoleWin32.h>
#endif /* WIN32 */

#include <decode.h>  // pour RepertoireExiste()
#ifdef CMDE_UNIX
	// #pragma message("Compilation pour une commande unix")
	#define L_(texte) texte
	#define LL_(liste) liste
#else
	// #pragma message("Compilation pour une application")
	#include <opium.h>
#endif

#define EVTREAD_C
#include <rundata.h>
#include <archive.h>
#include <evtread.h>

#ifndef WIN32
static char *DelimNomFichier = "/";
#else
static char *DelimNomFichier = "\\";
#endif

#define LONG_IS_32BITS

char ArchDataInit();
int  ArchRunHeader(int f, int *t0sec, int *t0msec, char log, char *explic);
int  ArchEvtLastNum();
ssize_t  ArchEvtSkip(int f, char *explic);
int  ArchEvtBack(int f, int *num_lu, off_t *of7);
int  ArchEvtRead(int f, int num, off_t *retour, char *explic);

/* ========================================================================== */
char ArchReadInit(int max) {
	int i;

	EvtRunNb = 0;
	EvtNb = EvtLus = EvtDejaPresents = 0;
	EvtsParFichier = 0;
	EvtAuPif = 1;
	OctetsDejaLus = 0;

	if(max) EvtsMax = max; else EvtsMax = 1;
	EvtSel = (char *)malloc(EvtsMax * sizeof(char));
	EvtPos = (off_t *)malloc((EvtsMax + 1) * sizeof(off_t));
	EvtStock = (TypeEvtStocke **)malloc(EvtsMax * sizeof(TypeEvtStocke *));
	for(i=0; i<EvtsMax+1; i++) { EvtPos[i] = -1; EvtStock[i] = 0; }
#ifdef DATEHEURE_H
	EvtDateExacte = (TypeS_us *)malloc((EvtsMax + 1) * sizeof(TypeS_us));
	EvtRunT0.s_us = 0;
#endif
	if(max == 0) EvtsMax = 0;

	EvtOpenNb = 0;
	EvtOpenMax = getdtablesize() / 2;
	EvtAffiche = -1;
	return(ArchDataInit() && (EvtPos != 0) && (EvtSel != 0));
}
/* ========================================================================== */
char ArchEvtKeepFree() {
	int j;
	
	if(EvtOpenNb >= EvtOpenMax) {
		for(j=0; j<EvtRunNb; j++) if(EvtRunInfo[j].path >= 0) break;
		if(j < EvtRunNb) {
			close(EvtRunInfo[j].path); EvtRunInfo[j].path = -1; EvtOpenNb--;
		} else {
			printf("!!! ERREUR SYSTEME, pas de fichier ouvert parmi les %d/%d declares, avec %d ouvertures autorisees\n",
				   EvtOpenNb,EvtRunNb,EvtOpenMax);
			return(0);
		}
	}
	return(1);
}
/* ========================================================================== */
char ArchEvtFilesFull() {
	if(errno == EMFILE) {
		printf("- %s: fermeture temporaire du plus ancien fichier ouvert (%d maxi)\n",strerror(errno),EvtOpenNb);
		EvtOpenMax = EvtOpenNb;
		return(1);
	} else return(0);
}
#define LECTURE_PREALABLE
/* ========================================================================== */
char ArchRunOpen(char *nom, char log, char *raison) {
	sprintf(raison,"Incident indetermine depuis %s",__func__);
	return(ArchRunOpenRange(nom,0,999999,log,raison));
}
/* ========================================================================== */
char ArchRunOpenRange(char *nom, int premiere, int derniere, char log, char *raison) {
	long l,lngr;
	char nomcomplet[MAXFILENAME],nomrun[80],nomfichier[80],numero[12];
	int f; off_t pos; int premier,num0,numN,t0sec,t0msec,nb,evt;
	struct stat infos;
	char tres_grand,type_run; int tranche;
	enum { STD=0, STREAM, REGEN };
#ifdef LECTURE_PREALABLE
	int repere; char explic[256];
#ifdef CMDE_UNIX
	int precedent;
#endif
#endif

/*
 * run: servi en tranches ou fichier unique?
 */
 	sprintf(raison,"Incident indetermine depuis %s",__func__);
	strcpy(nomcomplet,nom);
	l = lngr = (long)strlen(nomcomplet) - 1;
	if(lngr < 0) { strcpy(raison,L_("Nom de fichier vide")); return(0); }
	if(!strcmp(nomcomplet+lngr,DelimNomFichier)) nomcomplet[lngr] = '\0'; else lngr++;
	while(--l >= 0) { if(nomcomplet[l] == DelimNomFichier[0]) break; }
	strcpy(nomrun,nomcomplet+l+1);
	numero[0] = '\0'; tranche = premiere; type_run = STD;
	printf("= Verification de la nature du chemin '%s'\n",nomcomplet);
	if(RepertoireExiste(nomcomplet)) {
		printf("  > C'est un repertoire\n");
		strncpy(EvtPhotoDir,nomcomplet,MAXFILENAME-1);
		sprintf(numero,"_%03d",tranche);
		strcat(strcat(strcat(strcpy(FichierNtuple,nomcomplet),DelimNomFichier),nomrun),"_ntuple");
		strcat(strcat(strcat(strcpy(FichierSupp,nomcomplet),DelimNomFichier),nomrun),"_tgo");
		strcat(strcat(strcat(nomcomplet,DelimNomFichier),nomrun),numero);
		if((f = open(nomcomplet,MODE_OPEN,0)) < 0) {
			nomcomplet[lngr] = '\0';
			sprintf(numero,"_S%02d",tranche);
			strcat(strcat(strcat(nomcomplet,DelimNomFichier),nomrun),numero);
			if((f = open(nomcomplet,MODE_OPEN,0)) < 0) {
				nomcomplet[lngr] = '\0';
				sprintf(numero,"_R%02d",tranche);
				strcat(strcat(strcat(nomcomplet,DelimNomFichier),nomrun),numero);
				if((f = open(nomcomplet,MODE_OPEN,0)) < 0) {
					sprintf(raison,L_("Repertoire vide, run inutilisable (%s)"),strerror(errno));
					return(0);
				} else { close(f); type_run = REGEN; }
			} else { close(f); type_run = STREAM; }
		} else close(f);
		strcat(strcpy(nomfichier,nomrun),numero);
	} else {
		printf("  > C'est le fichier a lire\n");
		strcat(strcpy(FichierNtuple,nomcomplet),"_ntuple");
		strcat(strcpy(FichierSupp,nomcomplet),"_tgo");
		strcpy(nomfichier,nomrun);
	}
	ArgCompatible(1);

/*
 * boucle sur toutes les tranches du paquet
 */
	nb = num0 = 0;
	do {
		if(EvtRunNb < MAXFICHIERS) {
			if(log) printf("* %s du fichier #%d, '%s'\n",EvtRunNb? "Ajout": "Lecture",EvtRunNb,nomcomplet);
		} else {
			printf("! %s du fichier #%d, '%s', impossible (MAXFICHIERS=%d)\n",
				EvtRunNb? "Ajout": "Lecture",EvtRunNb,nomcomplet,MAXFICHIERS);
			break;
		}

		/* ..... Doit ouvrir et avancer jusqu'au 1er evenement ..... */
		if(!ArchEvtKeepFree()) { strcpy(raison,L_("Anomalie irrecuperable (voir log)")); return(0); }
		if((f = open(nomcomplet,MODE_OPEN,0)) < 0) {
			if(tranche == premiere) {
				sprintf(raison,L_("Fichier inutilisable (%s)"),strerror(errno));
				return(0);
			} else {
				if(ArchEvtFilesFull()) continue;
				if(log) printf("  . %s: plus d'autre fichier essaye\n",strerror(errno));
				break;
			}
		}
		EvtOpenNb++;
		if(fstat(f,&infos) != -1) {
		#ifdef LONG_IS_32BITS
			if(log) printf("  . Fichier de %.1f Mo (%lld octets, 0x%08llX)\n",
				(float)infos.st_size/(1024.0 * 1024.0),infos.st_size,infos.st_size);
		#else
			if(log) printf("  . Fichier de %.1f Mo (%ld octets, 0x%08lX)\n",
				(float)infos.st_size/(1024.0 * 1024.0),infos.st_size,infos.st_size);
		#endif
		} else {
			if(log) printf("  . Taille du fichier #%d indisponible:\n  %s\n  (%s)\n",
				f,nomcomplet,strerror(errno));
			infos.st_size = 100;
			close(f); EvtOpenNb--;
			sprintf(raison,"Defaut d'information de taille (voir log)"); return(0);
		}
		tres_grand = (infos.st_size > 1000000);
		if(!ArchRunHeader(f,&t0sec,&t0msec,log && (tranche == premiere),raison)) {
			close(f); EvtOpenNb--;
			return(0);
		}

		/* ..... Enregistrement de la position du premier evenement dans ce fichier ..... */
		premier = EvtNb;
		pos = lseek(f,0,SEEK_CUR);
		if(pos == -1) {
			if(log) printf("  . Position dans le fichier apres l'entete, inconnue (%s)\n",strerror(errno));
			close(f); EvtOpenNb--;
			strcpy(raison,L_("Defaut d'information de position (voir log)")); return(0);
		} 
//1		 else if(log) printf(". Position dans le fichier apres l'entete: %3ld\n",(long)pos);

		if(Trigger.enservice) {
		#ifdef LECTURE_PREALABLE
			/* ..... Evenements de longueur variable et lecture prealable ..... */
			/* l'inconvenient, c'est que pour faire patienter l'utilisateur,
				on a besion d'OPIUM */
			if(EvtsMax && !EvtAuPif) {
				Discontinu = 1;
				repere = tres_grand? (int)infos.st_size / 1000000: (int)infos.st_size;
			#ifdef CMDE_UNIX
				printf("<Return> pour arreter\n");
				ClavierDeverouille();
				printf("  .   0%% du fichier a ete lu"); precedent = 0;
			#else
				OpiumProgresTitle("Lecture fichier");
				OpiumProgresInit(repere);
			#endif
				while(EvtNb < EvtsMax) {
					repere = tres_grand? (int)pos / 1000000: (int)pos;
				#ifdef CMDE_UNIX
					if(ClavierTouche() == 0x0d) break;
					if(repere > precedent) { printf("\r  . %3d%%",repere); precedent = repere; }
				#else
					if(!OpiumProgresRefresh(repere)) break;
				#endif
					if(!ArchEvtSkip(f,explic)) break;
					if(EvtNb == premier) num0 = ArchEvtLastNum();
					EvtPos[EvtNb] = pos + OctetsDejaLus;
					pos = lseek(f,0,SEEK_CUR);
					EvtNb++;
					if(EvtsParFichier) {
						if((EvtNb - premier) >= EvtsParFichier) break;
					}
				};
				EvtPos[EvtNb] = pos + OctetsDejaLus; /* pour marquer la fin du precedent */
			#ifdef CMDE_UNIX
				printf("\r\n");
				ClavierReverouille();
			#else
				OpiumProgresClose();
			#endif
				
			} else 
		#endif

			/* ..... Evenements de longueur fixe OU ouverture seule pour le 1er fichier ..... */
			if(EvtRunNb == 0) {
	//1			if(log) printf(". Position dans le fichier avant le 1er evt     : %lld\n",lseek(f,0,SEEK_CUR));
				if(!ArchEvtSkip(f,raison)) {
					printf("!!! %s\n",raison);
					sprintf(raison,"Pas d'evenement identifiable"); close(f);
					EvtOpenNb--; return(0);
				}
				num0 = ArchEvtLastNum();
	//1			if(log) printf(". Position dans le fichier apres le 1er evt (#%d): %lld\n",num0,lseek(f,0,SEEK_CUR));
				EvtPos[0] = pos;
	//			if(log) printf(". pos[%d] = %lld\n",0,EvtPos[0]);
				EvtPos[1] = lseek(f,0,SEEK_CUR);
	//			if(log) printf(". pos[%d] = %lld\n",1,EvtPos[1]);
				if(EvtsMax) {
					EvtDim = EvtPos[1] - EvtPos[0]; /* 1ere approx pour commencer a pouvoir reculer de 1 evt */
					lseek(f,0,SEEK_END);
					ArchEvtBack(f,&numN,&pos);
					nb = numN - num0;
					if(nb) {
						if(nb > EvtsMax) {
							sprintf(raison,L_("On a un evenement #%d alors qu'on n'en gere que %d"),nb,EvtsMax);
							close(f);
							EvtOpenNb--;
							return(0);
						}
						if(!Discontinu) {
							EvtPos[nb] = pos;
							EvtDim = (pos - EvtPos[0]) / nb;
						}
					}
					nb += 1;
				} else {
					EvtDim = EvtPos[1] - EvtPos[0];
					if(EvtDim) {
						nb = (int)((off_t)infos.st_size - EvtPos[0]) / EvtDim;
						if((infos.st_size - (nb * EvtDim) - EvtPos[0]) > (EvtDim / 2)) nb++;
						if(EvtsParFichier && (EvtNb > EvtsParFichier)) {
							if(log) printf("  . Le fichier contient %d evenements,\n",EvtNb);
							if(log) printf("    mais tu as demande de n'en lire que %d, donc...\n",EvtsParFichier);
							EvtNb = EvtsParFichier;
						}
					#ifdef LONG_IS_32BITS
						if(log) printf("  . Taille d'un evenement: %lld\n",EvtDim);
						if(log) printf("  . Entete de %lld octets + supplement de %lld octets\n",
							   EvtPos[0],(infos.st_size % EvtDim) - EvtPos[0]);
					#else
						if(log) printf("  . Taille d'un evenement: %ld\n",EvtDim);
						if(log) printf("  . Entete de %ld octets + supplement de %ld octets\n",
							   EvtPos[0],(infos.st_size % EvtDim) - EvtPos[0]);
					#endif
					} else {
						strcpy(raison,L_("Taille du premier evenement apparemment nulle"));
						close(f);
						EvtOpenNb--;
						return(0);
					}
				}
				EvtNb = nb;
				
			/* ..... Evenements de longueur fixe OU ouverture seule pour les fichiers suivants ..... */
			} else /* if(EvtRunNb != 0) */ {
				if(!ArchEvtSkip(f,raison)) {
					printf("!!! %s\n",raison);
					sprintf(raison,"Pas d'evenement identifiable");
					close(f);
					EvtOpenNb--;
					return(0);
				}
				num0 = ArchEvtLastNum();
	//1			if(log) printf(". Position dans le fichier apres l'evt #%d: %lld\n",num0,lseek(f,0,SEEK_CUR));
				if(EvtsMax) {
					/* les tranches ne sont pas forcement consecutives (EDW: REGEN)
					if((tranche != premiere) && (num0 < EvtRunInfo[EvtRunNb-1].numN)) {
						EvtRunInfo[EvtRunNb-1].numN = num0;
						nb = EvtRunInfo[EvtRunNb-1].numN - EvtRunInfo[EvtRunNb-1].num0;
						premier = EvtRunInfo[EvtRunNb-1].premier + nb;
						EvtNb = premier;
					} */
					if((premier + 1) > EvtsMax) {
						sprintf(raison,L_("On a un evenement #%d alors qu'on n'en gere que %d"),premier+1,EvtsMax);
						close(f);
						EvtOpenNb--;
						return(0);
					}
					if(!Discontinu) {
						EvtPos[premier] = pos + OctetsDejaLus;
	//2					if(log) printf(". pos[%d] = %lld\n",premier,EvtPos[premier]);
						EvtPos[premier+1] = lseek(f,0,SEEK_CUR) + OctetsDejaLus;
	//2					if(log) printf(". pos[%d] = %lld\n",premier+1,EvtPos[premier+1]);
					}
					lseek(f,0,SEEK_END);
					ArchEvtBack(f,&numN,&pos);
					nb = numN - num0;
					if(nb) {
						if((premier+nb) > EvtsMax) {
							sprintf(raison,L_("On a un evenement #%d alors qu'on n'en gere que %d"),premier+nb,EvtsMax);
							close(f);
							EvtOpenNb--;
							return(0);
						}
						if(!Discontinu) EvtPos[premier+nb] = pos + OctetsDejaLus;
					}
					nb += 1;
				} else {
					nb = (int)((off_t)infos.st_size - EvtPos[0]) / EvtDim;
					if((infos.st_size - (nb * EvtDim) - EvtPos[0]) > (EvtDim / 2)) nb++;
				}
				EvtNb += nb;
				if(EvtsParFichier && (EvtNb > EvtsParFichier)) {
					if(log) printf("  . Les fichiers ouverts contiennent au moins %d evenements au total,\n",EvtNb);
					if(log) printf("    mais tu as demande de n'en utiliser que %d, donc...\n",EvtsParFichier);
					EvtNb = EvtsParFichier;
				}
			}
			if(log) {
				if(premier != 0) printf("  . Ajout de  %d evenements\n",EvtNb-premier);
				printf("  . Au total, %d evenements\n",EvtNb);
			}
		} else /* stream */ { }
		
		/* ..... Reperage pour le fichier ainsi ouvert ..... */
		EvtRunInfo[EvtRunNb].path = f;
		EvtRunInfo[EvtRunNb].debut = OctetsDejaLus;
		EvtRunInfo[EvtRunNb].taille = infos.st_size;
		EvtRunInfo[EvtRunNb].premier = premier;
		strcpy(EvtRunInfo[EvtRunNb].nom,nomcomplet);
		strcpy(EvtRunInfo[EvtRunNb].fichier,nomfichier);
		EvtRunInfo[EvtRunNb].num0 = num0;
		EvtRunInfo[EvtRunNb].numN = num0 + nb;
		EvtRunInfo[EvtRunNb].t0sec = t0sec;
		EvtRunInfo[EvtRunNb].t0msec = t0msec;
		if(log && (EvtRunNb == 1)) {
			// printf("  . Fichier #%d (%s) ouvert sur %d:\n",EvtRunNb,EvtRunInfo[EvtRunNb].fichier,EvtRunInfo[EvtRunNb].path);
			printf("  . 1er evt tango: #%d (samba #%d), position logique %lld\n",
				EvtRunInfo[EvtRunNb].premier,EvtRunInfo[EvtRunNb].num0,EvtRunInfo[EvtRunNb].debut);
		}
		if(EvtSel) for(evt=premier; evt<EvtNb; evt++) EvtSel[evt] = 1;
		EvtRunNb++;
		EvtRunInfo[EvtRunNb].nom[0] = '\0';
		EvtRunInfo[EvtRunNb].premier = EvtNb;
	#ifdef DATEHEURE_H
		if(EvtRunNb == 1) EvtRunT0.s_us = S_usFromInt(t0sec,t0msec);
		if(log && (EvtRunNb == 1)) printf("  . T0=%d,%06d\n",EvtRunT0.t.s,EvtRunT0.t.us);
	#endif
		OctetsDejaLus += infos.st_size;
		EvtLus = EvtNb;
		
		/* ..... Recherche d'autres tranches ..... */
		if(type_run == STREAM) numero[0] = '\0';
		if(numero[0]) {
			TrancheRelue = tranche;
			if(tranche >= derniere) break;
			nomcomplet[lngr] = '\0';
			switch(type_run) {
				case STREAM: sprintf(numero,"_S%02d",++tranche); break;
				case REGEN: sprintf(numero,"_R%02d",++tranche); break;
				default: sprintf(numero,"_%03d",++tranche);
			}
			strcat(strcat(strcat(nomcomplet,DelimNomFichier),nomrun),numero);
			strcat(strcpy(nomfichier,nomrun),numero);
		}
	} while(numero[0]);
	ArgCompatible(0);

	return(1);
}
/* ========================================================================== */
void ArchRunClose() {
	/* voir EvtRunPurge */
	int j;
	
	for(j=0; j<EvtRunNb; j++) if(EvtRunInfo[j].path >= 0) close(EvtRunInfo[j].path);
	EvtRunNb = 0;
	EvtNb = EvtLus = 0;
	OctetsDejaLus = 0;
	EvtsParFichier = 0;
	EvtAuPif = 1;
	if(EvtPos) for(j=0; j<EvtsMax+1; j++) EvtPos[j] = -1;
}
/* ========================================================================== */
int ArchEvtSaved(int evt) {
	int i;
	
	for(i=0; i<EvtStockNb; i++) if(EvtStock[i]->index == evt) break;
	return((i < EvtStockNb)? i+1: 0);
}
/* ========================================================================== */
char ArchEvtRestore(int i) {
	int vt,voie;
	
	char debug = 0;

	if(!i) return(0);
	if(debug) printf(" . Recuperation du stock #%d\n",i);
	memcpy(&(ArchEvt),&(EvtStock[i]->evt),sizeof(TypeEvt));
	if(debug) printf("   + Evenement recupe\n");
	for(vt=0; vt<ArchEvt.nbvoies; vt++) {
		if(debug) printf("   + Recuperation de la trace #%d/%d\n",vt+1,ArchEvt.nbvoies);
		voie = ArchEvt.voie[vt].num;
		if(debug) printf("     - Recuperation de la voie #%d\n",voie+1);
		memcpy(&(VoieEvent[voie]),&(EvtStock[i]->signaux[vt]),sizeof(TypeVoieDonnees));
		if(debug) printf("     - Voie recuperee\n");
		if((VoieEvent[voie].lngr > 0) && EvtStock[i]->signaux[vt].brutes.t.sval) {
			if(debug) printf("     - Recuperation de %d donnees\n",VoieEvent[voie].lngr);
			memcpy(VoieEvent[voie].brutes.t.sval,EvtStock[i]->signaux[vt].brutes.t.sval,VoieEvent[voie].lngr * sizeof(TypeDonnee));
			if(debug) printf("     - Donnees recuperees\n");
		} else EvtStock[i]->signaux[vt].brutes.t.sval = 0;
	}
	if(debug) printf(" . Stock #%d recupere\n",i);

	return(1);
}
/* ========================================================================== */
char ArchEvtGet(int evt, char *explic) {
	int f; off_t pos,*retour; int i,j,num,lu;
	int mvt; char rc;
	off_t iseek;
	char cause[256];

	char debug;

	debug = 0; // (evt >= 2354);
	if(debug) printf(". recherche de l'evenement #%d\n",evt+1);
	sprintf(explic,"Pas d'erreur identifiee");
	if(evt >= EvtNb) {
		if(debug) printf("! Erreur #0\n");
		sprintf(explic,"Evenement #%d/%d inaccessible!",evt+1,EvtNb);
		return(0);
	}
	rc = 0; retour = 0;
	if(EvtsMax) {
		if(debug) printf(". EvtsMax @%08X\n",(hexa)EvtsMax);
		if((j = ArchEvtSaved(evt))) { ArchEvtRestore(j-1); return(1); }
		if(debug) printf(". evenement #%d pas sauvegarde\n",evt+1);
		pos = EvtPos[evt];
		if(pos == -1) {
			for(j=evt; j; ) { --j; if(EvtPos[j] >= 0) break; }
			pos = EvtPos[j] + ((evt - j) * EvtDim);
			//1			printf("- position prevue pour l'evt tango #%d: %lld\n",evt+1,pos);
			if(debug) printf(". position prevue pour l'evt #%d: %lld\n",evt+1,pos);
			retour = &(EvtPos[evt]);
		}
		//1		 else printf("- position connue pour l'evt tango #%d: %lld\n",evt+1,pos);
		else if(debug) printf(". position connue pour l'evt #%d: %lld\n",evt+1,pos);
	} else {
		pos = EvtPos[0] + (evt * EvtDim);
		if(debug) printf(". position supposee pour l'evt #%d: %lld\n",evt+1,pos);
	}
	if(pos > OctetsDejaLus) {
		if(debug) printf("! Erreur #1\n");
		sprintf(explic,"Position evenement #%d inaccessible (%lld/%lld)",evt+1,pos,OctetsDejaLus);
		return(0);
	}
	if(EvtRunNb == 1) i = 0; else {
		for(i=0; i<EvtRunNb; i++) if(EvtRunInfo[i].debut > pos) break;
		if(i == 0) {
			if(debug) printf("! Erreur #2\n");
			sprintf(explic,"!!! ERREUR SYSTEME, debut_partition[0]=%lld > pos_evt[%d]=%lld",
				   EvtRunInfo[i].debut,evt+1,pos);
			return(0);
		}
		--i;
		if(debug) printf(". Examen de %s (1er tango=%d, samba=%d)\n",EvtRunInfo[i].nom,EvtRunInfo[i].premier,EvtRunInfo[i].num0);
	}
	ArgCompatible(1);
	mvt = 0; errno = 0;
	do {
		if((f = EvtRunInfo[i].path) < 0) {
			if(!ArchEvtKeepFree()) break;
			f = EvtRunInfo[i].path = open(EvtRunInfo[i].nom,MODE_OPEN,0);
			if(f < 0) {
				if(ArchEvtFilesFull()) continue;
				if(debug) printf("! Erreur #3\n");
				sprintf(explic,"Fichier %s illisible (%s)",EvtRunInfo[i].nom,strerror(errno));
				break;
			}
			EvtOpenNb++;
			if(debug) printf("* Ouverture du fichier #%d, '%s'\n",i,EvtRunInfo[i].nom);
		}
		iseek = lseek(f,pos - EvtRunInfo[i].debut,SEEK_SET);
		if(debug) printf(". fichier retenu: #%d (ouvert en %d), positionne sur %lld\n",i,f,iseek);
		num = (evt - EvtRunInfo[i].premier) + EvtRunInfo[i].num0;
		if(debug) printf(". evt samba demande: #%d\n",num);
		rc = ArchEvtRead(f,num,retour,cause);
		if((rc < -1) && retour) {
			if(debug) printf(". erreur de recherche: %d\n",rc);
			if((*retour < 0) && (mvt <= 0)) { 
				if(--i >= 0) { pos = EvtRunInfo[i].taille - (2 * EvtDim) + EvtRunInfo[i].debut; mvt = -1; continue; }
			} else if((*retour >= EvtRunInfo[i].taille) && (mvt >= 0)) {
				if(++i <  EvtRunNb) { pos = EvtRunInfo[i].debut; mvt = 1; continue; }
			}
		}
		if(rc <= 0) {
			if(debug) printf("! Erreur #4\n");
			sprintf(explic,"Pas relu l'evenement #%d (position %lld) dans le fichier #%d demarrant a %lld:\n    %s\n    Erreur %s%d (%s, %s)",
				   evt,pos,i,EvtRunInfo[i].debut,EvtRunInfo[i].nom,
				   (rc == -1)? "unix, code ": "specifique #",(rc == -1)? errno: -rc,(rc == -1)? strerror(errno): "ArchEvtRead",cause);
			rc = 0;
		} else {
			if(EvtsMax) {
				if(retour) *retour += EvtRunInfo[i].debut;
				//2 printf("  pos[%d] = %lld\n",evt+1,EvtPos[evt]);
				if(debug) printf("  pos[%d] = %lld\n",evt+1,EvtPos[evt]);
				if(EvtPos[evt+1] == -1) EvtPos[evt+1] = lseek(f,0,SEEK_CUR) + EvtRunInfo[i].debut;
				//2 printf("  pos[%d] = %lld\n",evt+1,EvtPos[evt+1]);
				if(debug) printf("  pos[%d] = %lld\n",evt+1,EvtPos[evt+1]);
				if(!Discontinu && (num != (lu = ArchEvtLastNum()))) {
					if(debug) printf("! Erreur #5\n");
					sprintf(explic,"ATTENTION: l'evenement %d est introuvable, remplace par le %d",num,lu);
					printf("! %s\n",explic);					
				}
			}
		}
		break;
	} while(1);
	ArgCompatible(0);
	return(rc);
}
