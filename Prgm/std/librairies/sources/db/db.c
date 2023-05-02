#include "environnement.h"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>  /* pour strlen qui rend size_t si intrinseque */
#include <string.h>   /* pour strcpy, etc...  (sous linux) */
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#define DB_C

#include "defineVAR.h"
#include <db.h>
#include "dateheure.h"
#include "claps.h"
#include "decode.h"

#ifndef __linux__
	#define ACCES_DB
#endif

#ifdef ACCES_DB
//+ #pragma message("Utilisation de CURL")
#include <curl/curl.h>
#endif

/*    USAGE EN MODE LIGNE:

 ident   : nom de l'enregistrement dans la database (pour SAMBA: run_<runname>)
 donnees : "variable":"valeur" (sauf flottants, entiers et true/false: sans quote)
 fichier : fichier contenant les donnees encadrees par des { }

 Ecrire:
		curl -X PUT http://134.158.176.27:5984/testdb/ident -d '{donnees}'
		curl -X PUT http://134.158.176.27:5984/testdb/ident -d @fichier
 Lire:
		curl http://134.158.176.27:5984/testdb/ident
		curl http://134.158.176.27:5984/testdb/ident -o toto (resultat dans toto)
 Effacer:
		curl -X DELETE 'http://134.158.176.27:5984/testdb/ident?rev=xxxxx'
		(remarquer les ' encadrant l'URL complet)

transferts collectifs: tout champ remplacable par {a,b,c} ou [i1-iN]
reference a la valeur effective d'un champ n: #n
 exemple: curl scp://134.158.176.{22,24}/SambaArgs -o "SambaArgs_#1"

 */

/* ========================================================================== */
int DbSendDesc(char *serveur, char *id, ArgDesc *desc) {
#ifdef ACCES_DB
	int rc; ARG_STYLES ancien;
	char nom_complet[MAXFILENAME];
	char template_envoi[256],*donnees,template_retour[256],*retour;
	FILE *f,*g;
	CURL *curl; CURLcode res;

	rc = 1;
	curl = curl_easy_init();
	if(!curl) return(0);
	strcpy(template_envoi,"dbenvoi_XXXX"); donnees = mktemp(template_envoi);
	if(donnees) {
		RepertoireCreeRacine(donnees);
		ancien = ArgStyle(ARG_STYLE_JSON); ArgPrint(donnees,desc); ArgStyle(ancien);
		f = fopen(donnees,"r");
		if(f) {
			strcpy(template_retour,"dbresultat_XXXX"); retour = mktemp(template_retour);
			if(retour) g = fopen(retour,"w"); else g = 0;
			snprintf(nom_complet,MAXFILENAME,"%s/%s",serveur,id);
			curl_easy_setopt(curl,CURLOPT_URL,nom_complet);
			curl_easy_setopt(curl,CURLOPT_READFUNCTION,0);
			curl_easy_setopt(curl,CURLOPT_READDATA,f);
			curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,0);
			if(g) curl_easy_setopt(curl,CURLOPT_WRITEDATA,g);
			else curl_easy_setopt(curl,CURLOPT_WRITEDATA,stdout);
			// curl_easy_setopt(curl,CURLOPT_VERBOSE,1);
			curl_easy_setopt(curl,CURLOPT_PUT,1);
			res = curl_easy_perform(curl);
			if(res) fprintf(stderr,"! Ecriture de %s vers %s en erreur, code %d\n",id,serveur,res);
			fclose(f);
			if(g) {
				fclose(g);
				strcpy(DbErreur,"(neant)"); strcpy(DbRaison,"(neant)");
				strcpy(DbId,"(absent)"); strcpy(DbRev,"(absente)");
				DbStatus = 0;
				ArgScan(retour,DbReponse);
				if(!DbStatus) {
					// printf("----- Envoye:\n");
					sprintf(nom_complet,"cp %s dbdata_not_received",donnees);
					system(nom_complet);
					sprintf(nom_complet,"cp %s dbexplics",retour);
					system(nom_complet);
					// printf("----- Fin envoi\n");
					// ArgPrint("*",DbReponse);
				}
			}
			if(retour) remove(retour);
		} else rc = 0;
	} else rc = 0;
	curl_easy_cleanup(curl);
	if(donnees) remove(donnees);
	return(rc);
#else
	return(1);
#endif
}
/* ========================================================================== */
int DbSendFile(char *serveur, char *id, FILE *f) {
#ifdef ACCES_DB
	int rc;
	char nom_complet[MAXFILENAME];
	char template_retour[256],*retour;
	FILE *g;
	CURL *curl; CURLcode res;

	rc = 1;
	curl = curl_easy_init();
	if(!curl) return(0);
	if(f) {
		strcpy(template_retour,"dbresultat_XXXX"); retour = mktemp(template_retour);
		if(retour) g = fopen(retour,"w"); else g = 0;
		snprintf(nom_complet,MAXFILENAME,"%s/%s",serveur,id);
		curl_easy_setopt(curl,CURLOPT_URL,nom_complet);
		curl_easy_setopt(curl,CURLOPT_READFUNCTION,0);
		curl_easy_setopt(curl,CURLOPT_READDATA,f);
		curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,0);
		if(g) curl_easy_setopt(curl,CURLOPT_WRITEDATA,g);
		else curl_easy_setopt(curl,CURLOPT_WRITEDATA,stdout);
		// curl_easy_setopt(curl,CURLOPT_VERBOSE,1);
		curl_easy_setopt(curl,CURLOPT_PUT,1);
		res = curl_easy_perform(curl);
		if(res) fprintf(stderr,"! Ecriture de %s vers %s en erreur, code %d\n",id,serveur,res);
		fclose(f);
		if(g) {
			fclose(g);
			strcpy(DbErreur,"(neant)"); strcpy(DbRaison,"(neant)");
			strcpy(DbId,"(absent)"); strcpy(DbRev,"(absente)");
			DbStatus = 0;
			ArgScan(retour,DbReponse);
			// ArgPrint("*",DbReponse);
		}
		if(retour) remove(retour);
	} else rc = 0;
	curl_easy_cleanup(curl);
	return(rc);
#else
	return(1);
#endif
}
/* ========================================================================== */
int DbSendFichier(char *serveur, char *id, char *donnees) {
#ifdef ACCES_DB
	int rc;
	char nom_complet[MAXFILENAME];
	char template_retour[256],*retour;
	FILE *f,*g;
	CURL *curl; CURLcode res;

	rc = 1;
	curl = curl_easy_init();
	if(!curl) return(0);
	f = fopen(donnees,"r");
	if(f) {
		strcpy(template_retour,"dbresultat_XXXX"); retour = mktemp(template_retour);
		if(retour) g = fopen(retour,"w"); else g = 0;
		snprintf(nom_complet,MAXFILENAME,"%s/%s",serveur,id);
		curl_easy_setopt(curl,CURLOPT_URL,nom_complet);
		curl_easy_setopt(curl,CURLOPT_READFUNCTION,0);
		curl_easy_setopt(curl,CURLOPT_READDATA,f);
		curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,0);
		if(g) curl_easy_setopt(curl,CURLOPT_WRITEDATA,g);
		else curl_easy_setopt(curl,CURLOPT_WRITEDATA,stdout);
		// curl_easy_setopt(curl,CURLOPT_VERBOSE,1);
		curl_easy_setopt(curl,CURLOPT_PUT,1);
		res = curl_easy_perform(curl);
		if(res) fprintf(stderr,"! Ecriture de %s vers %s en erreur, code %d\n",id,serveur,res);
		fclose(f);
		if(g) {
			fclose(g);
			strcpy(DbErreur,"(neant)"); strcpy(DbRaison,"(neant)");
			strcpy(DbId,"(absent)"); strcpy(DbRev,"(absente)");
			DbStatus = 0;
			ArgScan(retour,DbReponse);
			// ArgPrint("*",DbReponse);
		}
		if(retour) remove(retour);
	} else rc = 0;
	curl_easy_cleanup(curl);
	return(rc);
#else
	return(1);
#endif
}
/* ========================================================================== */
void DbRazLastId() { DbId[0] = '\0'; DbRev[0] = '\0'; }
/* ========================================================================== */
char DbGetLastId(char *id, char *rev) {
	if((DbId[0] == '\0') || (DbId[0] == '(')) return(0);
	strcpy(id,DbId); strcpy(rev,DbRev);
	return(1);
}
/* ========================================================================== */
int DbRemove(char *serveur, char *id, char *rev) {
	/* rev recupere a l'issue de l'ecriture (DbRev) */
#ifdef ACCES_DB
	int rc;
	char nom_complet[MAXFILENAME];
	char template_retour[256],*retour;
	FILE *g;
	CURL *curl; CURLcode res;

	rc = 1;
	curl = curl_easy_init();
	if(!curl) return(0);
	strcpy(template_retour,"dbresultat_XXXX"); retour = mktemp(template_retour);
	if(retour) g = fopen(retour,"w"); else g = 0;
	snprintf(nom_complet,MAXFILENAME,"%s/%s?rev=%s",serveur,id,rev);
	curl_easy_setopt(curl,CURLOPT_URL,nom_complet);
	curl_easy_setopt(curl,CURLOPT_READFUNCTION,0);
	curl_easy_setopt(curl,CURLOPT_READDATA,0);
	curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,0);
	if(g) curl_easy_setopt(curl,CURLOPT_WRITEDATA,g);
	else curl_easy_setopt(curl,CURLOPT_WRITEDATA,stdout);
	//? curl_easy_setopt(curl,CURLOPT_VERBOSE,1);
	curl_easy_setopt(curl,CURLOPT_CUSTOMREQUEST,"DELETE");
	res = curl_easy_perform(curl);
	if(res) fprintf(stderr,"! Ecriture de %s vers %s en erreur, code %d\n",id,serveur,res);
	if(g) {
		fclose(g);
		strcpy(DbErreur,"(neant)"); strcpy(DbRaison,"(neant)");
		strcpy(DbId,"(absent)"); strcpy(DbRev,"(absente)");
		DbStatus = 0;
		ArgScan(retour,DbReponse);
		// ArgPrint("*",DbReponse);
	}
	if(retour) remove(retour);
	curl_easy_cleanup(curl);
	return(rc);
#else
	return(1);
#endif
}
/* ========================================================================== */
char DbPrint(char *chemin, char *nom, ArgDesc *desc, char *fichier) {
	FILE *f; int num;

	sprintf(fichier,"%s%s%s_%lld",chemin,FILESEP,nom,DateLong());
	num = 0;
	do {
		f = fopen(fichier,"r");
		if(f) {
			fclose(f);
			sprintf(fichier,"%s%s%s_%lld_%02d",chemin,FILESEP,nom,DateLong(),++num);
		}
	} while(f);
	ArgPrint(fichier,desc);
	return(1);
}
/* ========================================================================== */
int DbScan(char *chemin, char *nom, ArgDesc *desc, int date, char *fichier) {
	char vu;
#ifndef __MWERKS__
	char datevue[8];
	DIR *repert; struct dirent *entree,trouvee;
	int l,j,k,actuelle,dernier,derniere;
	int aa,mm,jj,nbjours; int diff;
#endif

	vu = 0;
#ifndef __MWERKS__
	if(date) {
		mm = date / 100;
		jj = date - (mm * 100);
		aa = mm / 100;
		mm = mm - (aa * 100);
		nbjours = (((aa * 12) + mm) * 31) + jj;
		derniere = 99999999; /* la, c'est la derniere difference retenue */
	} else derniere = 0; /* alors que la, c'est la derniere date retenue */

	nbjours = 0; /* gcc */
	actuelle = 0; fichier[0] = '\0';
	l = (int)strlen(nom); dernier = 0;
	repert = opendir(chemin);
	while((entree = readdir(repert)) != NULL) {
		if(!strncmp(entree->d_name,nom,l)) {
			strncpy(datevue,entree->d_name+l,6); datevue[6] = '\0';
			// printf("(DbRead) Recherche %s: fichier %s en date du %s\n",nom,entree->d_name,datevue);
			sscanf(datevue,"%d",&actuelle);
			if(date == 0) { /* si date = 0, on prend la plus recente sauvegarde */
				if(actuelle > derniere) {
					memcpy(&trouvee,entree,sizeof(struct dirent)); derniere = actuelle; vu = 1;
					if(strlen(entree->d_name) < (l + 7)) dernier = 0;
					else sscanf(entree->d_name+l+7,"%d",&dernier);
				} else if(actuelle == derniere) {
					if(strlen(entree->d_name) < (l + 7)) k = 0;
					else sscanf(entree->d_name+l+7,"%d",&k);
					if(k > dernier) {
						memcpy(&trouvee,entree,sizeof(struct dirent)); vu = 1;
						derniere = actuelle;
						dernier = k;
					}
				}
			} else { /* si la date est fixee, on prend la plus proche (algo vrai a 1 ou 2 jours pres..) */
				mm = actuelle / 100;
				jj = actuelle - (mm * 100);
				aa = mm / 100;
				mm = mm - (aa * 100);
				j = (((aa * 12) + mm) * 31) + jj;
				diff = nbjours - j; if(diff < 0) diff = -diff;
				if(diff < derniere) {
					memcpy(&trouvee,entree,sizeof(struct dirent)); derniere = diff; vu = 1;
					if(strlen(entree->d_name) < (l + 7)) dernier = 0;
					else sscanf(entree->d_name+l+7,"%d",&dernier);
				} else if(diff == derniere) { /* a difference egale, on prend la derniere version */
					if(strlen(entree->d_name) < (l + 7)) k = 0;
					else sscanf(entree->d_name+l+7,"%d",&k);
					if(k > dernier) {
						memcpy(&trouvee,entree,sizeof(struct dirent)); vu = 1;
						derniere = actuelle;
						dernier = k;
					}
				}
			}
		}
	}
	closedir(repert);
	if(vu) {
		sprintf(fichier,"%s%s%s",chemin,FILESEP,trouvee.d_name);
		if(ArgScan(fichier,desc) <= 0) return(-1);
		return(actuelle);
	} else
#endif
	return(0);
}

