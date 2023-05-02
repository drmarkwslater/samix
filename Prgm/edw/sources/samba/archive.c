#define ARCHIVE
#define ARCHIVE_BINAIRE

#ifdef macintosh
#pragma message(__FILE__)
#endif
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <environnement.h>

#include <samba.h>
#include <numeriseurs.h>
#include <objets_samba.h>

#ifdef CODE_WARRIOR_VSN
	#include <claps.h>
	#include <decode.h>
	#include <dateheure.h>
	#include <opium_demande.h>
	#include <opium.h>
#else
	#include <utils/claps.h>
	#include <utils/decode.h>
	#include <utils/dateheure.h>
	#include <opium_demande.h>
	#include <opium4/opium.h>
#endif

/* ................... definitions propres a l'archivage ............... */

/* ................... structures propres a l'archivage ................ */
// int ArchAn,ArchMois,ArchJour;
#undef LIBRAIRIE
#include <archive.h>
static char ArchiveDeclaree[ARCH_TYPEDATA];

#ifdef POUR_DAS
#undef printf
#define OpiumRefresh FonctionVide
#define PanelRefreshVars FonctionVide
#define WndJournalTitle FonctionVide
#undef OpiumDisplayed
#define OpiumDisplayed FonctionFausse

char OrgaEcritCSV(char *fichier);

/* ========================================================================== */
void SambaCompleteLigne(int tot, int cur) {
	int i;
	
	for(i=cur; i<(tot-1); i++) printf(" ");
	printf("|\n");
}
/* ========================================================================== */
void FonctionVide() { }
/* ========================================================================== */
char FonctionFausse() { return(0); }
#endif /* POUR_DAS */

/* ========================================================================== */
void ArchiveDefini(char log) {
//	int64 t0;
	int voie; int i,l,ph,fmt;
	char premier_appel;
	char nom_voie[VOIE_NOM],matos[MAXFILENAME];
	FILE *f;
#ifdef PAW_H
	char nom_hbook[MAXFILENAME];
#endif
	
	premier_appel = (LectCntl.LectRunNouveau && (LectSession < 2));

	if(log) {
		printf(" ____________________________________________________________________________________\n");
		printf("|                                   Gestion du run                                   |\n");
		printf("|____________________________________________________________________________________|\n");
		l = printf("| Acquisition #%d du %02d.%02d.%02d",NumeroLect,ExecJour,ExecMois+1,ExecAn);
		if(Archive.enservice) l += printf(" (%s '%s')",premier_appel? "nouveau run,": "reprise du run",Acquis[AcquisLocale].etat.nom_run);
		SambaCompleteLigne(86,l);
	}
    for(fmt=0; fmt<ARCH_TYPEDATA; fmt++) {
        ArchiveDeclaree[fmt] = Archive.enservice;
        ArchiveOuverte[fmt] = 0;
    }
	if(Archive.enservice) {
		if(log) {
			for(fmt=0; fmt<ARCH_TYPEDATA; fmt++) if(ArchSauve[fmt]) {
				SambaCompleteLigne(86,printf("| Sauvegarde des %s %s:",ArchFmtNom[fmt],ArchiveSurRepert? "dans le repertoire": "dans le fichier"));
				SambaCompleteLigne(86,printf("|   %s",ArchiveChemin[fmt]));
			}
		}
		if(Trigger.enservice) {
			if(log) {
				if(RegenEnCours && (ArchRegenTaux > 1))
					SambaCompleteLigne(86,printf("| Regeneration en cours, seul 1 evenement sur %d sera sauvegarde;",ArchRegenTaux));
				else if(ArchReduc > 1)
					SambaCompleteLigne(86,printf("| Seul 1 evenement sur %d sera sauvegarde;",ArchReduc));
				if(BolosUtilises > 1) {
					l = printf("| Sauvegarde des detecteurs %s",ArchModeTexte[(int)ArchDetecMode]);
					if(BolosAssocies) l += printf(", %s leurs associes;",ArchAssocMode?"avec":"sans");
					SambaCompleteLigne(86,l);
				}
			}
#ifdef PAW_H
			if(ArchSauveNtp == NTP_PAW) {
				strcat(strcpy(nom_hbook,Acquis[AcquisLocale].etat.nom_run),".ntuple");
				if(log) SambaCompleteLigne(86,printf("| Sauvegarde des ntuples PAW dans le fichier:"));
				if(ArchiveSurRepert) {
					PawFileOpen(HbId,ArchiveChemin[EVTS],nom_hbook,(float *)&HbVar,HbVarNoms);
					if(log) SambaCompleteLigne(86,printf("|   %s%s",ArchiveChemin[EVTS],nom_hbook));
				} else {
					PawFileOpen(HbId,ArchZone[EVTS],nom_hbook,(float *)&HbVar,HbVarNoms);
					if(log) SambaCompleteLigne(86,printf("|   %s%s",ArchZone[EVTS],nom_hbook));
				}
				FichierNtuples[0] = '\0';
			} else
#endif
			if(ArchSauveNtp) {
				if(ArchiveSurRepert) sprintf(FichierNtuples,"%s%s%s_ntp",ArchiveChemin[EVTS],FILESEP,Acquis[AcquisLocale].etat.nom_run);
				else strcat(strcpy(FichierNtuples,ArchiveChemin[EVTS]),"_ntp");
				// f = fopen(FichierNtuples,"w"); mais LectAcqStd peut appeller ArchiveDefini plusieurs fois..
				if((f = fopen(FichierNtuples,"r"))) {
					fclose(f);
					f = fopen(FichierNtuples,"a");
					if(f && log) {
						SambaCompleteLigne(86,printf("| Sauvegarde des ntuples ASCII, a nouveau dans le fichier:"));
						SambaCompleteLigne(86,printf("|   %s",FichierNtuples));
					}
				} else {
					f = fopen(FichierNtuples,"w");
					if(f) {
						// fprintf(f," Evt Date(s) Date(us) GigaStamp Stamp Fichier Position Liste(63:32) Liste(31:0) Detecteur Voie Delai(s)");
						fprintf(f," Evt Date(s) Date(us) GigaStamp Stamp Fichier Position");
						for(i=BitTriggerDim; i; ) { fprintf(f," Liste(%02d:%02d)",(i*32)-1,(i-1)*32); --i; }
						fprintf(f," Detecteur Voie Classe Delai(s)");
						for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
							if(VoiesNb > 1) {
								if(BolosUtilises > 1) strcpy(nom_voie,VoieManip[voie].nom);
								else strcpy(nom_voie,VoieManip[voie].prenom);
								l = 0; while(nom_voie[++l]) if(nom_voie[l] == ' ') nom_voie[l] = '-';
							#ifdef NTP_IMPOSE
								fprintf(f," Mont-%s Ampl-%s Base-%s Bruit-%s Descente-%s",nom_voie,nom_voie,nom_voie,nom_voie,nom_voie);
							#else
								int k; for(k=0; k<VarSelection.enreg.nbvars; k++) fprintf(f," %s-%s",NtpName[VarSelection.enreg.ntuple[k]],nom_voie);
							#endif
								for(ph=0; ph<DETEC_PHASES_MAX; ph++) if(VoieTampon[voie].phase[ph].dt) fprintf(f," Phase%d-%s",ph+1,nom_voie);
								if(VoieTampon[voie].trig.calcule == TRMT_INTEGRAL) fprintf(f," Total-%s",nom_voie);
							} else {
							#ifdef NTP_IMPOSE
								fprintf(f," Mont Ampl Base Bruit Descente");
							#else
								int k; for(k=0; k<VarSelection.enreg.nbvars; k++) fprintf(f," %s",NtpName[VarSelection.enreg.ntuple[k]]);
							#endif
								for(ph=0; ph<DETEC_PHASES_MAX; ph++) if(VoieTampon[voie].phase[ph].dt) fprintf(f," Phase%d",ph+1);
								if(VoieTampon[voie].trig.calcule == TRMT_INTEGRAL) fprintf(f," Total");
							}
						}
						fprintf(f,"\n");
						if(log) {
							SambaCompleteLigne(86,printf("| Sauvegarde des ntuples ASCII dans le fichier:"));
							SambaCompleteLigne(86,printf("|   %s",FichierNtuples));
						}
					}
				}
				if(f) fclose(f);
				else {
					SambaCompleteLigne(86,printf("* Sauvegarde des ntuples impossible, fichier inaccessible:"));
					SambaCompleteLigne(86,printf("*   %s",FichierNtuples));
					SambaCompleteLigne(86,printf("* (%s)",strerror(errno)));
				}
			} else if(log) SambaCompleteLigne(86,printf("| Pas de sauvegarde de MonitNtp;"));
			if(TrmtRegulActive) {
				if(ArchiveSurRepert) sprintf(FichierSeuils,"%s%s%s_seuils",ArchiveChemin[EVTS],FILESEP,Acquis[AcquisLocale].etat.nom_run);
				else strcat(strcpy(FichierSeuils,ArchiveChemin[EVTS]),"_seuils");
				// f = fopen(FichierSeuils,"w"); voir plus haut
				if((f = fopen(FichierSeuils,"r"))) {
					fclose(f);
					f = fopen(FichierSeuils,"a");
					if(f && log) {
						l = printf("| Sauvegarde des seuils, a nouveau dans le fichier:"); SambaCompleteLigne(86,l);
						l = printf("|   %s",FichierSeuils); SambaCompleteLigne(86,l);
					}
				} else {
					f = fopen(FichierSeuils,"w");
					if(f) {
						fprintf(f,"Date(s) ");
						for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].sauvee && (VoieTampon[voie].trig.trgr->type != NEANT)) {
							strcpy(nom_voie,VoieManip[voie].nom);
							l = 0; while(nom_voie[++l]) if(nom_voie[l] == ' ') nom_voie[l] = '-';
							fprintf(f," Seuil-neg-%s Seuil-pos-%s",nom_voie,nom_voie);
						}
						fprintf(f,"\n");
						if(log) {
							l = printf("| Sauvegarde des seuils dans le fichier:"); SambaCompleteLigne(86,l);
							l = printf("|   %s",FichierSeuils); SambaCompleteLigne(86,l);
						}
					}
				}
				if(f) fclose(f);
				else {
					l = printf("* Sauvegarde des seuils impossible, fichier inaccessible:"); SambaCompleteLigne(86,l);
					l = printf("*   %s",FichierSeuils); SambaCompleteLigne(86,l);
					l = printf("* (%s)",strerror(errno)); SambaCompleteLigne(86,l);
				}
			} else if(log) SambaCompleteLigne(86,printf("| Regulation des seuils inactive: pas de fichier seuils cree;"));
		}
		if(LectSauveStatus) {
			int bb,rep; TypeNumeriseur *numeriseur; char prem;
			/* version 2: on sauve le status de toutes les entrees de tous les repartiteurs lus */
			for(rep=0; rep<RepartNb; rep++) if(Repart[rep].actif) {
				for(l=0; l<Repart[rep].nbentrees; l++) if((numeriseur = (TypeNumeriseur *)(Repart[rep].entree[l].adrs))) numeriseur->a_lire = 1;
			}
			for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].a_lire && ModeleBN[Numeriseur[bb].def.type].enreg_dim) break;
			if(bb < NumeriseurNb) {
				fmt = (ArchSauve[EVTS])? EVTS: STRM;
				if(ArchiveSurRepert) sprintf(FichierStatusBB,"%s%s%s_BB",ArchiveChemin[fmt],FILESEP,Acquis[AcquisLocale].etat.nom_run);
				else strcat(strcpy(FichierStatusBB,ArchiveChemin[fmt]),"_BB");
				// f = fopen(FichierStatusBB,"w"); voir plus haut
				if((f = fopen(FichierStatusBB,"r"))) {
					fclose(f);
					f = fopen(FichierStatusBB,"a");
					if(f && log) {
						l = printf("| Sauvegarde des status definitif des numeriseurs, a nouveau dans le fichier:"); SambaCompleteLigne(86,l);
						l = printf("|   %s",FichierStatusBB); SambaCompleteLigne(86,l);
					}
				} else {
					f = fopen(FichierStatusBB,"w");
					if(f) {
						int j,n;
						for(j=0; j<ModeleBNNb; j++) if(ModeleBN[j].enreg_dim) {
							fprintf(f,"%s [%d]:",ModeleBN[j].nom,ModeleBN[j].enreg_dim);
							/* for(k=0; k<ModeleBN[j].nbress; k++) if(ModeleBN[j].ress[k].enreg >= 0) fprintf(f," %s",ModeleBN[j].ress[k].nom); */
							for(n=0; n<ModeleBN[j].enreg_dim; n++) fprintf(f," %s",ModeleBN[j].ress[ModeleBN[j].numress[ModeleBN[j].enreg_sts[n]]].nom);
							fprintf(f,"\n");
						}
						fprintf(f,"Donnees :\n");
						if(log) {
							l = printf("| Sauvegarde des status definitifs des numeriseurs dans le fichier:"); SambaCompleteLigne(86,l);
							l = printf("|   %s",FichierStatusBB); SambaCompleteLigne(86,l);
						}
					}
				}
				if(f) fclose(f);
				else {
					l = printf("* Sauvegarde des status definitifs des numeriseurs impossible, fichier inaccessible:"); SambaCompleteLigne(86,l);
					l = printf("*   %s",FichierStatusBB); SambaCompleteLigne(86,l);
					l = printf("* (%s)",strerror(errno)); SambaCompleteLigne(86,l);
					FichierStatusBB[0] = '\0';
				}
				if(premier_appel && RepertoireExiste(BBstsPath)) /* sinon, ecrase en cas de reprise */ {
					char commande[MAXFILENAME];
					sprintf(commande,"rm -f %s*",BBstsPath); // printf("> Commande: '%s'\n",commande);
					system(commande);
				}
				prem = 1;
				for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].a_lire) {
					TypeBNModele *modele_bn; int n; FILE *g;
					numeriseur = &(Numeriseur[bb]);
					modele_bn = &(ModeleBN[numeriseur->def.type]);
					if(modele_bn->enreg_dim) {
						sprintf(numeriseur->enreg_txt,"%s%s",BBstsPath,numeriseur->nom);
						RepertoireCreeRacine(numeriseur->enreg_txt);
						if((g = fopen(numeriseur->enreg_txt,"r"))) {
							fclose(g);
							g = fopen(numeriseur->enreg_txt,"a");
							if(g && log) {
								if(prem) {
									l = printf("| Sauvegarde des status temporaires des numeriseurs, a nouveau dans les fichiers:"); SambaCompleteLigne(86,l);
									prem = 0;
								}
								l = printf("|   %s",numeriseur->enreg_txt); SambaCompleteLigne(86,l);
							}
						} else {
							g = fopen(numeriseur->enreg_txt,"w");
							if(g) {
								fprintf(g,"timestamp");
								for(n=0; n<modele_bn->enreg_dim; n++) fprintf(g," %s",modele_bn->ress[modele_bn->numress[modele_bn->enreg_sts[n]]].nom);
								fprintf(g,"\n");
								if(log) {
									if(prem) {
										l = printf("| Sauvegarde des status temporaires des numeriseurs dans:"); SambaCompleteLigne(86,l);
										prem = 0;
									}
									l = printf("|   %s",numeriseur->enreg_txt); SambaCompleteLigne(86,l);
								}
							}
						}
						if(g) fclose(g);
						else {
							l = printf("* Sauvegarde du status temporaire de %s impossible, fichier inaccessible:",numeriseur->nom); SambaCompleteLigne(86,l);
							l = printf("*   %s",numeriseur->enreg_txt); SambaCompleteLigne(86,l);
							l = printf("* (%s)",strerror(errno)); SambaCompleteLigne(86,l);
							numeriseur->enreg_txt[0] = '\0';
						}
					}
				} else Numeriseur[bb].enreg_txt[0] = '\0';
			} else if(log) { l = printf("| Aucun numeriseur n'a de status a sauvegarder;"); SambaCompleteLigne(86,l); }
		} else if(log) { l = printf("| Pas de sauvegarde de status de numeriseur demandee;"); SambaCompleteLigne(86,l); }
		if(premier_appel) {
			for(fmt=0; fmt<ARCH_TYPEDATA; fmt++) if(ArchSauve[fmt] && ArchDeclare[fmt]) {
				if(ArchiveSurRepert) sprintf(matos,"%s%s%s_setup.csv",ArchiveChemin[fmt],FILESEP,Acquis[AcquisLocale].etat.nom_run);
				else strcat(strcpy(matos,ArchiveChemin[fmt]),"_setup.csv");
				if(OrgaEcritCSV(matos)) {
					l = printf("| Materiel decrit dans le fichier:"); SambaCompleteLigne(86,l);
					l = printf("|   %s",matos); SambaCompleteLigne(86,l);
				} else {
					l = printf("* Sauvegarde du materiel impossible, fichier inaccessible:"); SambaCompleteLigne(86,l);
					l = printf("*   %s",matos); SambaCompleteLigne(86,l);
					l = printf("* (%s)",strerror(errno)); SambaCompleteLigne(86,l);
					OpiumError("Le fichier %s n'a pas pu etre cree (%s)",matos,strerror(errno));
				}
			}

		}
		if(log) {
			if(LoggerType == NEANT) l = printf("| Pas de cahier de manip.");
			else if(LoggerAdrs[0] == '\0') l = printf("| Cahier de manip demande mais pas defini.");
			else l = printf("| Compte-rendu sur le cahier de manip %s %s.",(LoggerType == CAHIER_ELOG)? "servi par":"ecrit dans",LoggerAdrs);
			SambaCompleteLigne(86,l);
		}
	} else if(log) printf("| Donnees non sauvegardees                                                           |\n");
	if(log) printf("|____________________________________________________________________________________|\n\n");

	/* Cette datation sera prise dans LectSynchro au demarrage de l'electronique */
//	t0 = VerifTempsPasse? DateMicroSecs(): 0;
//	ArchT0sec = t0 / 1000000;
//	ArchT0msec = t0 - (ArchT0sec * 1000000);
	/* Date et heure prises au moment de l'appel a SambaRunDate */
	sprintf(ArchHeure,"%02d:%02d:%02d",ExecHeure,ExecMin,ExecSec);
	sprintf(ArchDate,"%02d/%02d/%02d",ExecAn,ExecMois+1,ExecJour);
	TempsTotalArch = 0;
	ArchNbLus = 0;
	ArchInfoStatus = ARCH_STATUS_POUM; strcpy(ArchExplics,"Erreur logicielle interne dans SAMBA");
}
/* ========================================================================== */
void ArchiveNouvellePartition(int fmt) {
	struct stat infos;

	if(stat(ArchiveFichier[fmt],&infos) != -1) ArchiveEcrits[fmt] += (float)infos.st_size / (float)(MB);
	if(RegenEnCours) { if(ArchEcrites[fmt]) ArchNbPart[fmt] = ++LectRegenNum; } 
	else if(MiniStream) ArchNbPart[fmt] = ++LectStreamNum; 
	else if(ArchEcrites[fmt]) ArchNbPart[fmt] = ++(LectTrancheNum[fmt]);
	if(EdbActive) {
		if(DbGetLastId(ArchiveId,ArchiveRev)) ArchRevInclus = ArchRunInfoIdRev; else ArchRevInclus = 0;
		DbSendDesc(EdbServeur,ArchiveId,ArchRunInfo);
		if(DbStatus) printf("%s/ Base de Donnees renseignee (Id: %s, rev: %s)\n",DateHeure(),DbId,DbRev);
		else {
			printf("%s/ ! Renseignement de la Base de Donnees par %s en erreur\n",DateHeure(),__func__);
			printf("%9s   . Nature: %s, raison: %s\n"," ",DbErreur,DbRaison);
			printf("%9s   . Id: %s, rev: %s\n"," ",ArchiveId,ArchRevInclus?ArchiveRev:"nouvelle entree");
		}
	}
	ArchiveOuverte[fmt] = 0; ArchEcrites[fmt] = 0; ArchTrancheReste[fmt] = ArchTrancheTaille;
	LectTimeStampN = LectTimeStamp0 + ArchStampsSauves;
}
/* ========================================================================== */
FILE *ArchiveAccede(int fmt) {
	FILE *f; int bolo,voie,etage,nb,i,pos;
	int num; char comments,modetrig;
	TypeFiltre *filtre;
	char commande[256]; char entete[MAXFILENAME]; char fichier[MAXFILENAME];
	
    /* printf("(%s) Archivage '%s' demande (%s, archive %s)\n",__func__,ArchFmtNom[fmt],
           ArchSauve[fmt]? "autorise":"stoppe",ArchiveOuverte[fmt]? "ouverte":"fermee"); */
	if(!ArchSauve[fmt]) return(0);
	if(!ArchiveOuverte[fmt]) {
		if(RegenEnCours) sprintf(ArchivePartit,"%s_R%02d",Acquis[AcquisLocale].etat.nom_run,LectRegenNum);
		else if(MiniStream) sprintf(ArchivePartit,"%s_S%02d",Acquis[AcquisLocale].etat.nom_run,LectStreamNum);
		else if(ArchiveSurRepert) {
			if(fmt == EVTS) sprintf(ArchivePartit,"%s_%03d",Acquis[AcquisLocale].etat.nom_run,LectTrancheNum[fmt]);
			else sprintf(ArchivePartit,"%s_S%02d",Acquis[AcquisLocale].etat.nom_run,LectTrancheNum[fmt]);
		} else strcpy(ArchivePartit,Acquis[AcquisLocale].etat.nom_run);
		if(ArchiveSurRepert) sprintf(ArchiveFichier[fmt],"%s%s%s",ArchiveChemin[fmt],FILESEP,ArchivePartit);
		else {
			if(RegenEnCours) sprintf(ArchiveFichier[fmt],"%s_R%02d",ArchiveChemin[fmt],LectRegenNum);
			else if(MiniStream) sprintf(ArchiveFichier[fmt],"%s_S%02d",ArchiveChemin[fmt],LectStreamNum);
			else strcpy(ArchiveFichier[fmt],ArchiveChemin[fmt]);
		}
		/*
		 Ouverture
		 */
		if(!ArchiveDeclaree[fmt]) {
			printf("%s/ %s maintenant sauvegarde%ss %s:\n           '%s'\n",DateHeure(),
				   ArchFmtNom[fmt],fmt?"":"e",ArchiveSurRepert? "dans le repertoire": "sur le fichier",ArchiveChemin[fmt]);
			if(ArchiveSurRepert) {
				sprintf(commande,"mkdir %s",ArchiveChemin[fmt]);
				if(!system(commande)) {
					printf("          Creation de ce repertoire impossible (%s)\n",strerror(errno));
					if((f = fopen(ArchiveChemin[fmt],"r"))) {
						fclose(f);
						printf("          Ce nom existe deja\n");
					} else {
						printf("          Un seul fichier sera cree\n");
						ArchiveSurRepert = 0;
						LectDureeActive = 0;
						// if(OpiumDisplayed(pLectHachoir->cdr)) PanelRefreshVars(pLectHachoir);
						if(OpiumDisplayed(bSauvegardes)) OpiumRefresh(bSauvegardes);
						strcpy(ArchiveFichier[fmt],ArchiveChemin[fmt]);
					}
				}
			}
			if(ArchiveSurRepert) sprintf(fichier,"%s%s%s_setup.csv",ArchiveChemin[fmt],FILESEP,Acquis[AcquisLocale].etat.nom_run);
			else strcat(strcpy(fichier,Acquis[AcquisLocale].etat.nom_run),"_setup.csv");
			OrgaEcritCSV(fichier);
			ArchiveDeclaree[fmt] = 1;
		}
		/* MacOS9 renvoie toujours rc=-1 avec errno=0
		 rc = open(ArchiveChemin[fmt],O_CREAT|O_EXCL,0x644);
		 printf("(ArchiveAccede) open(\"%s\"):\n   retourne %d (erreur #%d, '%s')\n",
		 ArchiveChemin[fmt],rc,errno,strerror(errno));
		 if(rc == -1) return(fopen(ArchiveChemin[fmt],"a"));
		 close(rc);
		 */
		if((f = fopen(ArchiveFichier[fmt],"r"))) {
			fclose(f);
			printf("%s/ Re-ouverture du fichier %s\n",DateHeure(),ArchiveFichier[fmt]);
			f = fopen(ArchiveFichier[fmt],"a");
			if(f) ArchiveOuverte[fmt] = 1;
			return(f);
		}
		if(ArchFormat == ARCH_FMT_EDW1) {
			if(ArchiveSurRepert) sprintf(entete,"%s%s%s_hdr",ArchiveChemin[fmt],FILESEP,Acquis[AcquisLocale].etat.nom_run);
			else strcat(strcpy(entete,ArchiveChemin[fmt]),"_hdr");
			if((f = fopen(entete,"r"))) {
				fclose(f); f = 0;
			} else if(!(f = fopen(entete,"w"))) printf("%s/ Le fichier %s ne sera pas cree (%s)\n",DateHeure(),entete,strerror(errno));
			if(f) printf("%s/ Creation du fichier %s\n",DateHeure(),entete);
            else printf("%s/ Le fichier %s n'a pas pu etre cree (%s)\n",DateHeure(),entete,strerror(errno));
		} else /* if(ArchFormat == ARCH_FMT_EDW2) */ {
			if(ArchiveSurRepert) printf("%s/ Creation du fichier %s\n",DateHeure(),ArchiveFichier[fmt]);
			f = fopen(ArchiveFichier[fmt],"w");
            // fait plus bas: if(!f) printf("%s/ !!! Ce fichier n'a pas pu etre cree (%s)\n",DateHeure(),strerror(errno));
		}
		if(f) {
			/*
			 Entete generale
			 */
			// printf("ArchEnvirDesc: "); if(ArchEnvirDesc) ArgPrint("*",ArchEnvirDesc); else printf("... absente\n");
			// printf("ArchAutomInfo: "); if(ArchAutomInfo) ArgPrint("*",ArchAutomInfo); else printf("... absente\n");
			fprintf(f,"%s\n",SIGNATURE);
			fprintf(f,"%s %g\n",LIGNE_VERSION,Version);
			comments = 0;
			for(i=0; i<MAXCOMMENTNB; i++) if(LectComment[i][0] != '\0') {
				if(!comments) { fprintf(f,"#\n"); comments = 1; }
				LectComment[i][MAXCOMMENTLNGR-1] = '\0';
				fprintf(f,"# %s\n",LectComment[i]);
				//+ LectComment[i][0] = '\0';
			}
			if(comments) fprintf(f,"#\n");
			strcpy(ArchiveCheminCourant,ArchiveChemin[fmt]);
			ArchHdrNbInc = ArchHdrNbDesc[fmt];
			ArchType = (char)fmt;
			ArgAppend(f,0,ArchHdrGeneral);
			fprintf(f,FIN_DE_BLOC);
			/*
			 Description des detecteurs
			 */
			for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].num_hdr >= 0) {
				memcpy(&BoloEcrit,Bolo+bolo,sizeof(TypeDetecteur));
				for(num=0; num<BoloEcrit.nbreglages; num++) {
					int cap,bb,k;
					strcpy(BoloReglageEcrit[num].nom,ModeleDet[Bolo[bolo].type].regl[num].nom);
					cap = Bolo[bolo].reglage[num].capt;
					bb = Bolo[bolo].captr[cap].bb.num;
					k = Bolo[bolo].reglage[num].ress;
					if((bb >= 0) && (k >= 0)) {
						if(Numeriseur[bb].ress[k].cval[0])
							strcpy(BoloReglageEcrit[num].valeur,Numeriseur[bb].ress[k].cval);
						else strcpy(BoloReglageEcrit[num].valeur,"indetermine");
					} else strcpy(BoloReglageEcrit[num].valeur,"inconnu");
				}
				fprintf(f,"* Detecteur %s\n",Bolo[bolo].nom);
				ArgAppend(f,0,ArchHdrBoloDef);
				fprintf(f,FIN_DE_BLOC);
			}
			/*
			 Descriptions des voies
			 Quand on doit utiliser ArchiveBrutes(), il faut ne decrire que les voies sauvees
			 */
			fprintf(f,"* Physique\n");
			ArgAppend(f,0,ArchHdrPhysDef);
			fprintf(f,FIN_DE_BLOC);

			if(fmt == STRM) {
				if(ArchModeStream == ARCH_BRUTES) {
					for(voie=0; voie<VoiesNb; voie++) if(VoieManip[voie].num_hdr[STRM] >= 0) {
						memcpy(&VoieEcrite,VoieManip+voie,sizeof(TypeVoieAlire));
					#ifdef OBSOLETE
						ArchVoie.horloge = VoieEvent[voie].horloge;
						ArchVoie.dim = VoieEvent[voie].lngr;
						ArchVoie.avant_evt = VoieEvent[voie].avant_evt;
					#endif
						fprintf(f,"* Voie \"%s\":\n",VoieManip[voie].nom);
						ArgAppend(f,0,ArchHdrVoieDef);
						fprintf(f,FIN_DE_BLOC);
					}
				}
			} else if(fmt == EVTS) {
				for(voie=0; voie<VoiesNb; voie++) if(VoieManip[voie].num_hdr[EVTS] >= 0) {
					memcpy(&VoieEcrite,VoieManip+voie,sizeof(TypeVoieAlire));
				#ifdef OBSOLETE
					if(VoieManip[voie].duree.post_moyenne == 1) ArchVoie.horloge = VoieEvent[voie].horloge;
					else ArchVoie.horloge = (VoieEvent[voie].horloge * (float)VoieManip[voie].duree.post_moyenne);
					ArchVoie.dim = VoieEvent[voie].lngr;
					ArchVoie.avant_evt = VoieEvent[voie].avant_evt;
				#endif
					fprintf(f,"* Voie \"%s\":\n",VoieManip[voie].nom);
					ArgAppend(f,0,ArchHdrVoieDef);
					fprintf(f,FIN_DE_BLOC);
					/*
					 Description des filtres (obligatoirement dans celle des voies)
					 */
					// memcyp(&ArchFiltre,VoieTampon[voie].trmt[TRMT_PRETRG].filtre,sizeof(TypeFiltre));
					// ArgAppend(f,0,ArchHdrFiltre);
					filtre = VoieTampon[voie].trmt[TRMT_PRETRG].filtre;
					if(filtre) {
						fprintf(f,"* Filtre:\n");
						nb = filtre->nbetg; fwrite(&nb,sizeof(int),1,f);
						for(etage=0; etage<filtre->nbetg; etage++) {
							nb = filtre->etage[etage].nbdir; fwrite(&nb,sizeof(int),1,f);
							for(i=0; i<nb; i++) 
								fwrite(&(filtre->etage[etage].direct[i]),sizeof(double),1,f);
							nb = filtre->etage[etage].nbinv; fwrite(&nb,sizeof(int),1,f);
							for(i=0; i<nb; i++)
								fwrite(&(filtre->etage[etage].inverse[i]),sizeof(double),1,f);
						}
					}
				}
			}
			/*
			 Entete de run
			 */
			fprintf(f,"* Run\n");
			GigaStamp0 = (int)(LectTimeStamp0/1000000000);
			TimeStamp0 = Modulo(LectTimeStamp0,1000000000);
			GigaStampN = (int)(LectTimeStampN/1000000000);
			TimeStampN = Modulo(LectTimeStampN,1000000000);
			modetrig = Trigger.enservice;
			Trigger.enservice = (fmt == EVTS);
			ArgAppend(f,0,ArchHdrRun);
			fprintf(f,FIN_DE_BLOC);
			fprintf(f,"* Donnees\n");
			Trigger.enservice = modetrig;
		}
		if(ArchFormat == ARCH_FMT_EDW1) {
			if(f) fclose(f);
			if(ArchiveSurRepert) printf("%s/ Creation du fichier %s\n",DateHeure(),ArchiveFichier[fmt]);
			f = fopen(ArchiveFichier[fmt],"w");
		}
		if(f) {
			pos = (int)ftell(f);
			printf("%s/ Les donnees commencent dans le fichier a l'octet %d (0x%04x)\n",DateHeure(),pos,pos);
			ArchiveOuverte[fmt] = 1; ArchEcrites[fmt] = 0;
			if(EdbActive) {
				ARG_STYLES ancien;
				SambaTrmtEncodeTous();
				if(DbGetLastId(ArchiveId,ArchiveRev)) ArchRevInclus = ArchRunInfoIdRev;
				else {
					ArchRevInclus = 0;
					sprintf(ArchiveId,"run_%s",Acquis[AcquisLocale].etat.nom_run);
				}
				/* Fichier a effacer en fin de run, sinon il y eu un crash */
				ancien = ArgStyle(ARG_STYLE_JSON); ArgPrint("RunInfo",ArchRunInfo); ArgStyle(ancien);
				/* declaration immediate, a modifier en fin de run */
				ArchInfoStatus = ARCH_STATUS_ACTIF;
				DbSendDesc(EdbServeur,ArchiveId,ArchRunInfo);
				if(DbStatus) printf("%s/ Base de Donnees renseignee (Id: %s, rev: %s)\n",DateHeure(),DbId,DbRev);
				else {
					printf("%s/ ! Renseignement de la Base de Donnees par %s en erreur\n",DateHeure(),__func__);
					printf("%9s   . Nature: %s, raison: %s\n"," ",DbErreur,DbRaison);
					printf("%9s   . Id: %s, rev: %s\n"," ",ArchiveId,ArchRevInclus?ArchiveRev:"nouvelle entree");
				}
			}
		} else {
			//- printf("%s/ Sauvegarde impossible (%s),le fichier n'a pas pu etre ouvert\n",DateHeure(),strerror(errno));
			printf("%s/ Ouverture du fichier impossible (%s), acquisition stoppee\n",DateHeure(),strerror(errno));
			Archive.enservice = 0;
			LectDeclencheErreur(_ORIGINE_,90,LECT_ARCH);
		}
	} else f = fopen(ArchiveFichier[fmt],"a");

	return(f);
}
/* ========================================================================== */
void ArchiveSeuils(int secs) {
	FILE *f; int voie;

//	printf("%s/ Report dans le fichier %s\n",DateHeure(),FichierSeuils);
	f = fopen(FichierSeuils,"a");
	if(f) {
		fprintf(f," %11d ",secs);
		for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].sauvee && (VoieTampon[voie].trig.trgr->type != NEANT)) 
			fprintf(f," %g %g",VoieTampon[voie].trig.trgr->maxampl,VoieTampon[voie].trig.trgr->minampl);
		fprintf(f,"\n");
		fclose(f);
	}
}
/* ========================================================================== */
void ArchiveNettoie(int reserve, char imprime, const char *origine) {
	int i,m,n;
	int va,voie,dernier; int64 debut;
	int64 t0,n0,temps_lecture,temps_utilise; clock_t c0,cpu_utilise;

	c0 = VerifConsoCpu? clock() : 0;
	t0 = (VerifTempsPasse? DateMicroSecs(): 0); n0 = TempsTotalLect;
	m = EvtASauver - reserve;
	if(reserve) {
		for(voie=0; voie<VoiesNb; voie++) {
			n = VoieTampon[voie].dernier_evt;
//			if((EvtASauver - n) < reserve) VoieTampon[voie].dernier_evt -= m;
			if(m < n) VoieTampon[voie].dernier_evt -= m;
		}
	} else {
		for(voie=0; voie<VoiesNb; voie++) VoieTampon[voie].dernier_evt = -1;
	}
	dernier = (m > 0)? Evt[m-1].num: 0;
	for(i=0, n=m; n<EvtASauver; i++, n++) {
		for(va=0; va<Evt[i].nbvoies; va++) {
			if(Evt[i].voie[va].filtrees) {
				free(Evt[i].voie[va].filtrees);
				Evt[i].voie[va].filtrees = 0;
			}
			if(Evt[i].voie[va].demarrage) {
				free(Evt[i].voie[va].demarrage);
				Evt[i].voie[va].demarrage = 0;
			}
		}
		memcpy(Evt+i,Evt+n,sizeof(TypeEvt));
	}
	for(i=reserve; i<EvtASauver; i++) {
		for(va=0; va<Evt[i].nbvoies; va++) {
			if(Evt[i].voie[va].filtrees) {
				free(Evt[i].voie[va].filtrees);
				Evt[i].voie[va].filtrees = 0;
			}
			if(Evt[i].voie[va].demarrage) {
				free(Evt[i].voie[va].demarrage);
				Evt[i].voie[va].demarrage = 0;
			}
		}
	}
	if(imprime) {
		printf("%s/ ",DateHeure());
		if(origine) printf("%s: on jete ",origine); else printf("On jete ");
		printf("%d evenement%s deja detecte%s",Accord2s(EvtASauver));
		if(reserve) printf(", sauf %d",reserve);
		printf(" (total: %d)\n",dernier);
	}
	EvtASauver = EvtAffichable = reserve;
	if(EvtASauver > 0) {
		for(va=0; va<Evt[0].nbvoies; va++) {
			voie = Evt[0].voie[va].num;
			debut = Evt[0].voie[va].debut * VoieTampon[voie].trmt[TRMT_AU_VOL].compactage;
			if(va == 0) LectDebutEvts = debut;
			else if(LectDebutEvts > debut) LectDebutEvts = debut;
			
		}
	} else LectDebutEvts = -1;
//	printf("(%s) Reste %d evenements debutant a p=%d\n",__func__,EvtASauver,(int)LectDebutEvts);

	temps_lecture = TempsTotalLect - n0;
	ArchNbLus += temps_lecture;
	temps_utilise = (VerifTempsPasse? DateMicroSecs(): 0) - t0 - temps_lecture;
	TempsTotalArch += temps_utilise;
	TempsTempoArch += temps_utilise;
	cpu_utilise = VerifConsoCpu? clock() - c0: 0;
	CpuTempoArch += cpu_utilise;
	CpuTotalArch += cpu_utilise;

}
#ifndef CODE_WARRIOR_VSN
//#define AJUSTE_TAILLE_EVT
#endif
/* ========================================================================== */
char ArchiveEvt(int reserve, char vidage, const char *origine) {
	int voie,va,vp,ph;
	int i,j,k,l,n,dim,dim_avant; int debut,fin,nb1,nb2;
	int evt_total,hdr_sauves,evt_sauves,evt_jetes,dernier;
	int voies_total,voies_jetees,voies_perdues;
	char evt_a_jeter,voie_a_jeter; int reduc;
	FILE *f,*g;
	#ifdef AJUSTE_TAILLE_EVT
	off_t avant,apres; int lngr;
	#endif
	int64 t0,n0,temps_lecture,temps_utilise; clock_t c0,cpu_utilise;

	evt_total = EvtASauver - reserve;
	if(!evt_total) return(0);
	if(!(f = ArchiveAccede(EVTS))) return(0);
	c0 = VerifConsoCpu? clock() : 0;
	t0 = (VerifTempsPasse? DateMicroSecs(): 0); n0 = TempsTotalLect;

	hdr_sauves = evt_sauves = evt_jetes = dernier = 0;
	voies_total = voies_jetees = voies_perdues = 0;
	dim_avant = 0;
	#ifdef AJUSTE_TAILLE_EVT
		avant = ftello(f);
	#endif
	// bof:	if(ArchFormat == ARCH_FMT_EDW2) fprintf(f,"* Sauvegarde prevue de %d evenements\n",evt_total);
	dernier = 0;
	reduc = 0;
	for(n=0; n<evt_total; n++) {
		if(Evt[n].regen && (ArchRegenTaux > 1)) {
			if(reduc++ % ArchRegenTaux) continue;
		} else if(ArchReduc > 1) {
			if(reduc++ % ArchReduc) continue;
		} else reduc = 0;
	#ifdef CODE_WARRIOR_VSN
		Evt[n].pos = 0;
	#else
		Evt[n].pos = ftello(f);
	#endif
		memcpy(&ArchEvt,Evt+n,sizeof(TypeEvt));
		for(va=0; va<ArchEvt.nbvoies; va++) if(VoieManip[ArchEvt.voie[va].num].num_hdr[EVTS] >= 0) break;
		if(va >= ArchEvt.nbvoies) continue;
		dernier = ArchEvt.num;
		hdr_sauves++;
		evt_a_jeter = ((ArchEvt.delai * 1000.0) < LectDelaiMini);
//		if(Evt[n].num < 100) printf("Evt[%d] %s (delai=%f %c mini=%f)\n",Evt[n].num,evt_a_jeter?"jete ":"garde",
//			   Evt[n].delai,evt_a_jeter?'<':'>',LectDelaiMini/1000.0);
		if(ArchFormat == ARCH_FMT_EDW2) {
			fprintf(f,"* Evenement %d\n",ArchEvt.num);
			ArchVoiesNb = 0;
			for(va=0; va<ArchEvt.nbvoies; va++) {
				voie = ArchEvt.voie[va].num;
				if(VoieTampon[voie].lue && VoieManip[voie].sauvee && (VoieManip[voie].num_hdr[EVTS] >= 0)) ArchVoiesNb++;
			}
			/* la ligne suivante, pour "compatibilite entete evt", est obsolete:
			va = ArchEvt.voie_evt; memcpy(&ArchVoie,&(ArchEvt.voie[va]),sizeof(TypeVoieArchivee)); */
			ArgAppend(f,0,ArchHdrEvt);
			#ifdef AJUSTE_TAILLE_EVT
				apres = ftello(f);
				if(!ArchLngrEvt) ArchLngrEvt = (((apres - avant) / 80) + 2) * 80;
				lngr = ArchLngrEvt - (apres - avant);
				fprintf(f,"#");
				for(k=1; k<lngr-1; k++) fprintf(f,".");
				fprintf(f,"\n");
			#endif
			fprintf(f,FIN_DE_BLOC);
		}
		if(!evt_a_jeter) evt_sauves++;
		for(va=0; va<ArchEvt.nbvoies; va++) {
			voies_total++;
			voie = ArchEvt.voie[va].num;
			if(!(VoieTampon[voie].lue && VoieManip[voie].sauvee) || (VoieManip[voie].num_hdr[EVTS] < 0)) continue;
			memcpy(&ArchVoie,&(ArchEvt.voie[va]),sizeof(TypeVoieArchivee));
			if(evt_a_jeter) {
				if(EvtPrecedent.num >= 0) {
					for(vp=0; vp<EvtPrecedent.nbvoies; vp++) if(ArchEvt.voie[va].num == EvtPrecedent.voie[vp].num) break;
					voie_a_jeter = (vp < EvtPrecedent.nbvoies);
				} else voie_a_jeter = 1;
				if(voie_a_jeter) {
					voies_jetees++;
					ArchVoie.nbfiltres = 0; ArchVoie.dim = 0;
				}
//			} else if(!ArchSauveTrace) {
//				ArchVoie.nbfiltres = 0; ArchVoie.dim = 0;
			} else if(ArchVoie.debut < VoieTampon[voie].trmt[TRMT_AU_VOL].plus_ancien) {
				voies_perdues++;
				// va = ArchEvt.nbvoies; continue;
				ArchVoie.nbfiltres = 0; ArchVoie.dim = 0;
			}
			ArchVoie.horloge = VoieEvent[voie].horloge;
			if(ArchFormat == ARCH_FMT_EDW2) {
				#ifdef AJUSTE_TAILLE_EVT
					avant = ftello(f);
				#endif
				fprintf(f,"* Voie \"%s\"\n",VoieManip[voie].nom);
				if(VoieManip[voie].duree.post_moyenne > 1) {
					dim = ArchVoie.dim / VoieManip[voie].duree.post_moyenne;
					if(dim > ArchMoyenneMax) {
						if(ArchMoyenneMax) free(ArchMoyenne);
						ArchMoyenne = (TypeDonnee *)malloc(dim * sizeof(TypeDonnee));
						if(ArchMoyenne) ArchMoyenneMax = dim;
					}
					if(ArchMoyenne) {
						dim_avant = ArchVoie.dim;
						ArchVoie.dim = dim;
						ArchVoie.horloge = (VoieEvent[voie].horloge * (float)VoieManip[voie].duree.post_moyenne);
					}
				}
				ArgAppend(f,0,ArchHdrVoieEvt);
				#ifdef AJUSTE_TAILLE_EVT
					apres = ftello(f);
					if(!ArchLngrVoie) ArchLngrVoie = (((apres - avant) / 80) + 2) * 80;
					lngr = ArchLngrVoie - (apres - avant);
					fprintf(f,"#");
					for(k=1; k<lngr-1; k++) fprintf(f,".");
					fprintf(f,"\n");
				#endif
				fprintf(f,FIN_DE_BLOC);
			}
			if((ArchVoie.dim <= 0) || !ArchSauveTrace) continue;
			for(i=0; i<ArchVoie.nbfiltres; i++)
				fwrite(&(ArchVoie.demarrage[i]),sizeof(double),1,f);
			debut = ArchVoie.adresse;
			if((VoieManip[voie].duree.post_moyenne > 1) && ArchMoyenne) {
				l = debut;
				j = 0; ArchMoyenne[j] = 0; k = 0;
				for(i=0; i<dim_avant; i++) {
					ArchMoyenne[j] += VoieTampon[voie].brutes->t.sval[l];
					if(++k >= VoieManip[voie].duree.post_moyenne) {
						ArchMoyenne[j] /= k;
						if(++j >= ArchVoie.dim) break;
						ArchMoyenne[j] = 0; k = 0;
					}
					if(++l >= VoieTampon[voie].brutes->max) l = 0;
				}
				if(k < VoieManip[voie].duree.post_moyenne) ArchMoyenne[j] /= k;
				fwrite(ArchMoyenne,sizeof(TypeDonnee),ArchVoie.dim,f);
				ArchVoie.dim = dim_avant;
			} else {
				fin = debut + ArchVoie.dim;
				if(fin <= VoieTampon[voie].brutes->max) {
				#ifdef ARCHIVE_BINAIRE
					fwrite(VoieTampon[voie].brutes->t.sval + debut,sizeof(TypeDonnee),ArchVoie.dim,f);
				#else
					fprintf(f,"# (en binaire: %4d valeurs, voie %d, a partir du point %7d)\n",
						dim,voie,debut);
				#endif
				} else {
					nb1 = VoieTampon[voie].brutes->max - debut;
					nb2 = fin - VoieTampon[voie].brutes->max;
				#ifdef ARCHIVE_BINAIRE
					fwrite(VoieTampon[voie].brutes->t.sval + debut,sizeof(TypeDonnee),nb1,f);
					fwrite(VoieTampon[voie].brutes->t.sval,sizeof(TypeDonnee),nb2,f);
				#else
					fprintf(f,"# (en binaire: %4d valeurs, voie %d, a partir du point %7d)\n",
						nb1,voie,debut);
					fprintf(f,"# (en binaire: %4d valeurs, voie %d, a partir du point %7d)\n",
						nb2,voie,0);
				#endif
				}
			}
		}
		#ifdef ARCH_DEBUG
		if(ArchEvt2emeLot < 999999) ArchEvt2emeLot++;
		#endif
		memcpy(&EvtPrecedent,&ArchEvt,sizeof(TypeEvt));
	}
	//	fprintf(f,"* Fin sauvegarde\n");
	fclose(f);
// evt_sauves = hdr_sauves = evt_total;

#ifdef PAW_H
	/* MonitNtp de PAW (voie qui a trigge) */
	if(ArchSauveNtp == NTP_PAW) {
		HbVar.Voie  = (float)triggee->num;
		HbVar.Type  = (float)VoieManip[voietrig].type;
		HbVar.Base  = triggee->val[MONIT_BASE];
		HbVar.Ampl  = triggee->val[MONIT_AMPL];
		HbVar.Mont  = triggee->val[MONIT_MONTEE];
		HbVar.Desc  = triggee->val[MONIT_DESCENTE];
		HbVar.Bruit = triggee->val[MONIT_BRUIT];
		HbVar.Delai = Evt[n].delai*1000000;
		HFNT(HbId);
	} else 
#endif
		if(ArchSauveNtp && FichierNtuples[0] && !RegenEnCours) {
			// int tranche;
			TypeVoieArchivee *ajoutee;
			/* MonitNtp ASCII (toutes les voies) */
			ajoutee = 0;
			g = fopen(FichierNtuples,"a");
			if(g) {
				for(n=0; n<evt_total; n++) {
					// if(RegenEnCours) tranche = LectRegenNum; else if(MiniStream) tranche = LectStreamNum; else tranche = LectTrancheNum[EVTS];
					// Evt Date(s) Date(us) GigaStamp Stamp Fichier Position Liste(63:32) Liste(31:0) Detecteur Voie Delai(s)
					fprintf(g," %d %d %d %d %d %s %lld",Evt[n].num,Evt[n].sec,Evt[n].msec,Evt[n].gigastamp,Evt[n].stamp,ArchivePartit,Evt[n].pos);
					for(i=BitTriggerDim; i; ) fprintf(g," %d",Evt[n].liste[--i]);
					fprintf(g," %d %d %s %g",Evt[n].bolo_hdr,Evt[n].voie_hdr,Evt[n].nom_catg,Evt[n].delai);
					for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue) {
						// Ampl Base Bruit et plus encore, peut-etre
						if(VoieTampon[voie].lue < 0) va = Evt[n].nbvoies;
						else for(va=0; va<Evt[n].nbvoies; va++) {
							ajoutee = &(Evt[n].voie[va]);
							if(voie == ajoutee->num) break;
						}
					#ifdef NTP_IMPOSE
						if(va >= Evt[n].nbvoies) fprintf(g," 0.0 0.0 0.0 0.0 0.0");
						else fprintf(g," %g %g %g %g %g",ajoutee->val[MONIT_MONTEE],ajoutee->val[MONIT_AMPL],ajoutee->val[MONIT_BASE],ajoutee->val[MONIT_BRUIT],ajoutee->val[MONIT_DESCENTE]);
					#else
						int k; for(k=0; k<VarSelection.enreg.nbvars; k++) {
							if(va >= Evt[n].nbvoies) fprintf(g," 0.0");
							else fprintf(f," %g",ajoutee->val[VarSelection.enreg.ntuple[k]]);
						}
					#endif
					for(ph=0; ph<DETEC_PHASES_MAX; ph++) if(VoieTampon[voie].phase[ph].dt) {
							if(va >= Evt[n].nbvoies) fprintf(g," 0.0");
							else fprintf(g," %g",ajoutee->integrale[ph]);
						}
						if(VoieTampon[voie].trig.calcule == TRMT_INTEGRAL) {
							if(va >= Evt[n].nbvoies) fprintf(g," 0.0");
							else fprintf(g," %g",ajoutee->total);
						}
					}
					fprintf(g,"\n");
				}
				fclose(g);
			}
		}
			
	printf("%s/ ",DateHeure());
	if(origine) printf("%s: ",origine);
	printf("%d evenement%s",Accord1s(evt_sauves));
	n = hdr_sauves - evt_sauves;
	if(n) printf(" + %d entete%s",Accord1s(n));
	if(voies_perdues < (voies_total - voies_jetees)) {
		printf(" sauvegarde%s %s",(hdr_sauves>1)?"s":"",n?"(evenement trop rapproche)":"");
		if(voies_perdues) printf("          sauf %d voie%s perdue%s / %d",Accord2s(voies_perdues),voies_total-voies_jetees);
		Acquis[AcquisLocale].etat.evt_ecrits += hdr_sauves;   /* pour le run */
		ArchEvtsVides += n;
		// origine = 0;
	} else {
//		printf(" %s%s%s",ArchSauveTrace? "perdu": "jete",(evt_sauves?"":"e"),(hdr_sauves>1)?"s":"");
		printf(" perdu%s%s",ArchSauveTrace? (evt_sauves?"":"e"):"jete",(hdr_sauves>1)?"s":"");
		ArchEvtsPerdus += evt_sauves;
		ArchHorsDelai = 1; /* voies_perdues+voies_jetees = voies_total, donc aucun evt sauve meme partiellement */
	}
	printf(", dernier sauve: #%d\n",dernier);
	evt_jetes = evt_total - hdr_sauves;
	if(evt_jetes) printf("         +%d evenement%s jete%s (%sreduction par %d demandee)\n",
		Accord2s(evt_jetes),RegenEnCours? "regeneration en cours, ": "",RegenEnCours? ArchRegenTaux: ArchReduc);
	ArchEcrites[EVTS] += hdr_sauves;      /* par tranche */

	if(vidage) ArchiveNettoie(reserve,0,"Vidage");

	#ifdef ARCH_DEBUG
	if(ArchEvt2emeLot > 999999) ArchEvt2emeLot = 1;
	if(ArchEvt2emeLot < MAXDEBUG) printf("%s/ On repart avec EvtASauver=%d\n",DateHeure(),EvtASauver);
	#endif

	temps_lecture = TempsTotalLect - n0;
	ArchNbLus += temps_lecture;
	temps_utilise = (VerifTempsPasse? DateMicroSecs(): 0) - t0 - temps_lecture;
	TempsTotalArch += temps_utilise;
	TempsTempoArch += temps_utilise;
	cpu_utilise = VerifConsoCpu? clock() - c0: 0;
	CpuTempoArch += cpu_utilise;
	CpuTotalArch += cpu_utilise;
	
	return(1);
}
/* ========================================================================== */
INLINE void ArchiveVide(char sauve, const char *origine) {
	if(sauve) ArchiveEvt(RESERVE_EVT,1,origine);
	else ArchiveNettoie(RESERVE_EVT,1,origine);
}
/* ========================================================================== */
char ArchiveBrutes(char termine) {
	FILE *f;
	int64 t0,n0,temps_lecture,temps_utilise; clock_t c0,cpu_utilise;
	int voie,nb,i,ref; int point[VOIES_MAX],dispo[VOIES_MAX],minimal[VOIES_MAX],commun;
	int64 avant,but_final,ajoutees;
	int compactage,a_sauver,fait,diff;
	TypeDonnee tampon[VOIES_MAX];
	fpos_t fin;

	but_final = LectStampsLus;
	a_sauver = (int)(but_final - ArchStampsSauves);
	if(a_sauver <= 0) return(0);
//	if(a_sauver > LngrTampons) {
//		int64 sync;
//		ajoutees = a_sauver - UsageTampons;
//		sync = ajoutees / BlocsParD3; ajoutees = BlocsParD3 * (sync + 1);
//		LectValZappees += ajoutees;
//		// printf("(%s) Sautes       : %lld (total: %lld)\n",__func__,ajoutees,LectValZappees);
//		ArchStampsSauves += ajoutees;
//		printf("%s/ %8d donnees brutes jetees par voie\n",DateHeure(),(int)ajoutees);
//	}
	if(!termine && (a_sauver <= UsageTampons)) return(0);
	if(VerifOctetsStream) {
		printf("(%16s) Lus au total : %12lld\n",__func__,but_final);
		printf("(%16s) Deja sauves  : %12lld\n","",ArchStampsSauves);
		printf("(%16s) A sauver     : %12ld\n","",a_sauver);
		printf("(%16s) Seuil min    : %12ld\n","",UsageTampons);
		printf("(%16s) Memoire max  : %12lld\n","",LngrTampons);
	}
	if(!(f = ArchiveAccede(STRM))) return(0);
	c0 = VerifConsoCpu? clock() : 0;
	t0 = (VerifTempsPasse? DateMicroSecs(): 0); n0 = TempsTotalLect;

	commun = -1; compactage = -1;
	nb = 0;
	for(voie=0; voie<VoiesNb; voie++) if(VoieTampon[voie].lue && (VoieManip[voie].num_hdr[STRM] >= 0) && (VoieTampon[voie].brutes->max > 0)) {
		dispo[voie] = (int)(but_final  - LngrTampons);
		minimal[voie] = Modulo(dispo[voie]/compactage,VoieTampon[voie].brutes->max);
		point[voie] = Modulo(ArchStreamSauve,VoieTampon[voie].brutes->max);
		if(commun < 0) { ref = voie; commun = point[voie]; compactage = VoieTampon[voie].trmt[TRMT_AU_VOL].compactage; }
		else if(commun != point[voie]) {
			printf("%s/ !! Desynchronisation entre les voies %s et %s [%lld]\n",DateHeure(),VoieManip[ref].nom,VoieManip[voie].nom,++ArchSyncWrite);
		} else if(compactage != VoieTampon[voie].trmt[TRMT_AU_VOL].compactage) /* en stream, tout le monde doit avoir le meme */ {
			printf("%s/ !! Conflit d'horloge entre les voies %s et %s [%lld]\n",DateHeure(),VoieManip[ref].nom,VoieManip[voie].nom,++ArchSyncWrite);
		}
		nb++;
	}
	/* printf("(%s) Deja sauves  : %lld\n",__func__,ArchStampsSauves);
	printf("(%s) Lus au total : %lld\n",__func__,but_final); */
	if(ArchStreamSauve != (ArchStampsSauves / compactage)) {
		printf("%s/ !! Desynchronisation entre debut des donnees ecrites: %lld et sauvees: %lld / %d [%lld]\n",DateHeure(),ArchStreamSauve,ArchStampsSauves,compactage,++ArchSyncWrite);
	}
	fait = 0; ajoutees = 0;
	avant = ArchStreamSauve; 
	if(compactage > 1) but_final--; /* il faut attendre <compactage> blocs pour avoir un point valide */
	while(ArchStampsSauves < but_final) {
		i = 0;
		for(voie=0; voie<VoiesNb; voie++) if(VoieManip[voie].num_hdr[STRM] >= 0) {
			if(VoieTampon[voie].lue) {
				if(ArchStampsSauves >= dispo[voie]) {
					tampon[i++] = *(VoieTampon[voie].brutes->t.sval + point[voie]);
				} else {
					tampon[i++] = *(VoieTampon[voie].brutes->t.sval + minimal[voie]);
					LectValZappees++;
					ajoutees++;
				}
				point[voie] += 1; if(point[voie] >= VoieTampon[voie].brutes->max) point[voie] = 0;
			} else { tampon[i++] = 0; ajoutees++; }
		}
		fwrite(tampon,sizeof(TypeDonnee),i,f); fait++;
		ArchStreamSauve++;
		ArchStampsSauves += compactage;
	}

#ifdef OBSOLETE
	while(ArchStampsSauves < but_final) {
		if(ArchStampsSauves >= (LectStampsLus - LngrTampons)) {
			i = 0;
			for(voie=0; voie<VoiesNb; voie++) if(VoieManip[voie].num_hdr[STRM] >= 0) {
				tampon[i++] = *(VoieTampon[voie].brutes->t.sval + point[voie]);
				point[voie] += 1;
				if(point[voie] >= VoieTampon[voie].brutes->max) point[voie] = 0;
			}
			fwrite(tampon,sizeof(TypeDonnee),i,f);
			ArchStreamSauve++;
			fait++;
			ArchStampsSauves += compactage;
		} else {
			// printf("(%s) Memoire      : %lld\n",__func__,LngrTampons);
			decale = (LectStampsLus - UsageTampons - ArchStampsSauves) / compactage;
			decale *= compactage;
			sync = decale / BlocsParD3; decale = BlocsParD3 * (sync + 1);
			LectValZappees += decale;
			// printf("(%s) Sautes       : %lld (total: %lld)\n",__func__,decale,LectValZappees);
			ArchStampsSauves += decale;
			ArchStreamSauve += decale / compactage;
		}
	}
#endif

	// pos = ftell(f); pos: long, donc 32 bits, donc trop court...
	fgetpos(f,&fin);
	fclose(f);
	/* printf("(%s) Sauves maint.: %lld\n",__func__,ArchStampsSauves); */
	printf("%s/ %8d donnees brutes sauvegardees x %d voies\n",DateHeure(),fait,nb);
	if(ajoutees) printf("          dont %lld donnee%s generee%s\n",Accord2s(ajoutees));
	if(fait) ArchEcrites[STRM]++;
	diff = (int)(ArchStreamSauve - avant) - fait;
	if(diff) printf("         ! +%8d donnees brutes jetees x %d voies\n",diff,nb);
#ifndef __linux__
	printf("          soit %lld octets (%.1f MB) ecrits au total\n",fin,(float)fin/(1024.0*1024.0));
#endif

	temps_lecture = TempsTotalLect - n0;
	ArchNbLus += temps_lecture;
	temps_utilise = (VerifTempsPasse? DateMicroSecs(): 0) - t0 - temps_lecture;
	TempsTotalArch += temps_utilise;
	TempsTempoArch += temps_utilise;
	cpu_utilise = VerifConsoCpu? clock() - c0: 0;
	CpuTempoArch += cpu_utilise;
	CpuTotalArch += cpu_utilise;
	
	return(1);
}
/* ========================================================================== */
int ArchiveBloc(int voie) {
	int debut,fin,dim,nb1,nb2;
	FILE *f;
	int64 t0,temps_utilise; clock_t c0,cpu_utilise;
	int64 lus;  /* valeur bloquee ceans, DepileBoost pouvant la modifier entre-temps */

	if(!(VoieManip[voie].sauvee)) return(0);
	if(!(f = ArchiveAccede(STRM))) return(0);
	fprintf(f,"* Sauvegarde de donnees brutes, voie %d\n",voie);
	c0 = VerifConsoCpu? clock() : 0;
	t0 = (VerifTempsPasse? DateMicroSecs(): 0);
	lus = VoieTampon[voie].lus;
	dim = (int)(lus -  VoieTampon[voie].sauve);
	fwrite(&voie,sizeof(int),1,f);
	fwrite(&dim,sizeof(int),1,f);
	if(dim <= 0) return(0);
	debut = Modulo(VoieTampon[voie].sauve,VoieTampon[voie].brutes->max);
	fin = debut + dim;
	if(fin <= VoieTampon[voie].brutes->max) {
#ifdef ARCHIVE_BINAIRE
		fwrite(VoieTampon[voie].brutes->t.sval + debut,sizeof(TypeDonnee),dim,f);
#else
		fprintf(f,"# (en binaire: %4d valeurs, voie %d, a partir du point %7d)\n",
			dim,voie,debut);
#endif
	} else {
		nb1 = VoieTampon[voie].brutes->max - debut;
		nb2 = fin - VoieTampon[voie].brutes->max;
#ifdef ARCHIVE_BINAIRE
		fwrite(VoieTampon[voie].brutes->t.sval + debut,sizeof(TypeDonnee),nb1,f);
		fwrite(VoieTampon[voie].brutes->t.sval,sizeof(TypeDonnee),nb2,f);
#else
		fprintf(f,"# (en binaire: %4d valeurs, voie %d, a partir du point %7d)\n",
			nb1,voie,debut);
		fprintf(f,"# (en binaire: %4d valeurs, voie %d, a partir du point %7d)\n",
			nb2,voie,0);
#endif
	}
	fclose(f);
	printf("%s/ %8d donnees brutes sauvegardees pour la voie %s\n",DateHeure(),dim,VoieManip[voie].nom);
	VoieTampon[voie].sauve = lus;
	temps_utilise = (VerifTempsPasse? DateMicroSecs(): 0) - t0;
	TempsTotalArch += temps_utilise;
	TempsTempoArch += temps_utilise;
	cpu_utilise = VerifConsoCpu? clock() - c0: 0;
	CpuTempoArch += cpu_utilise;
	CpuTotalArch += cpu_utilise;
	return(1);
}
