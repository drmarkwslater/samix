#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>

#include <environnement.h>

#define MAINFILE
// deja defini: #define macintosh
#include <defineVAR.h>
#ifndef VT100
	#include <opium.h>
#endif
#include <decode.h>

#define MAXBUFFER 16*64

char NomFichier[MAXFILENAME],PathDelim[2]; int Fichier;
off_t Adrs,Dim;
size_t Max,Nb;
char TexteCherche[MAXFILENAME];
unsigned char Buffer[MAXBUFFER];

#ifndef OPIUM_H
#include <strings.h>
#include <stdarg.h> /* pour va_start */
#define PNL_CANCEL 0
typedef struct {
	char *texte;
	int (*fctn)(void);
} MenuItem;
/* ======================================================================= */
int OpiumError(const char *fmt, ...) {
	char texte[256];
	va_list argptr; int cnt;
	
	va_start(argptr, fmt);
	cnt = vsprintf(texte, fmt, argptr);
	va_end(argptr);
	
	printf("!!! %s\n",texte);
	return(1);
}
/* ======================================================================= */
int OpiumReadFile(char *texte, char *nom, int max) {
//2	size_t lngr;
//3	char *line = NULL; size_t linecap = 0; ssize_t linelen;

	printf("* %s => ",texte); fflush(stdout);
	//1 s'arrete au premier blanc
	scanf("%s",nom);
	//2 n'attend pas la reponse
//2	fgets(nom,max,stdin);
//2	lngr = strlen(nom) - 1;
//2	nom[lngr] = '\0';
	//3 ne recupere que le <return> precedent et n'attend pas la suite
//3	linelen = getline(&line, &linecap, stdin);
//3	if(linelen > 0) strncpy(nom,line,linelen);
//3	else return(0);
	//4 n'elimine pas les caracteres precedents de stdin, donc on boucle sur la demande
//4	fread(nom,(size_t)max,1,stdin);

	/* {
		char *lu,*ecrit; int i;
		lu = ecrit = nom; i = 0;
		while(*lu) {
			if(*lu != 0x0a) *ecrit++ = *lu++; else lu++;
			if(++i > max) break;
		}
		*ecrit++ = '\0';
		ecrit = nom; i = 0;
		printf("> Cherche: [");
		while(*ecrit) { printf("%02x.",*ecrit++); i++; if(i > max) break; }
		printf("]\n");
	} */

	return(1);
}
/* ======================================================================= */
int OpiumReadLHex(char *texte, int64 *val) {
	char rep[80];

	printf("* %s [\\ = %llx] => ",texte,*val); fflush(stdout);
	scanf("%s",rep);
	if(strcmp(rep,"\\")) sscanf(rep,"%llx",val);
	return(1);
}
/* ======================================================================= */
int OpiumReadHex(char *texte, unsigned long *val) {
	char rep[80];
	
	printf("* %s [\\ = %lx] => ",texte,*val); fflush(stdout);
	scanf("%s",rep);
	if(strcmp(rep,"\\")) sscanf(rep,"%lx",val);
	return(1);
}
/* ======================================================================= */
int MenuExec(MenuItem *menu) {
	MenuItem *item;
	int i; char rep;
	
	do {
		i = 0; item = menu;
		printf("* ");
		while(item->texte[0]) {
			if(i) printf("/");
			// printf(item->texte);
			fputs(item->texte,stdout);
			i++; item++;
		};
		printf(" => "); fflush(stdout);
		scanf("%s",&rep);
		// rep = getchar(); n'elimine pas le <return> pourtant necessaire
		if((rep >= 'a') && (rep <= 'z')) rep -= ('a' - 'A');
		item = menu;
		while(item->texte[0]) {
			if(rep == item->texte[0]) {
				if(item->fctn == 0) return(1);
				(*item->fctn)();
				break;
			}
			item++;
		};
	} while(1);
	return(1);
}
#endif
/* ======================================================================= */
char repositionne(char separe) {
	if(lseek(Fichier,Adrs,SEEK_SET) < 0) {
		OpiumError("Positionnement a %08llX impossible (%s)",Adrs,strerror(errno));
		return(0);
	} else {
		if(separe) printf("--------\n");
		return(1);
	}
}
/* ======================================================================= */
void imprime() {
	size_t taille,inf,sup,loc,deb,max;
	unsigned char *ptr,*ligne;

	taille = Max;
	if(Dim) {
		if(Adrs >= Dim) {
			OpiumError("Fin du fichier a %08llX",Dim);
			Adrs = Dim - Max;
			repositionne(1);
			return;
		}
		if((Adrs + Max) > Dim) taille = Dim - Adrs;
	}
//	printf("Fichier accede a %08X\n",(lhex)lseek(Fichier,0,SEEK_CUR));
	Nb = read(Fichier,Buffer,taille);
//	printf("Fichier laisse a %08X\n",(lhex)lseek(Fichier,0,SEEK_CUR));
	if(Nb <= 0) {
		OpiumError("Lecture de %x octet(s) a %08llX en erreur (%s)",
			taille,Adrs,strerror(errno));
		Adrs = 0;
		repositionne(1);
	} else {
		if((Dim == 0) && (Nb < taille)) Dim = Adrs + Nb;
		inf = 0;
		ptr = Buffer;
		while(inf < Nb) {
			sup = max = inf + 16;
			if(sup > Nb) sup = Nb;
			printf("%08llx: ",Adrs + inf);
			deb = inf;
			ligne = ptr;
			loc = 0;
			while(inf < max) {
				if(inf < sup) printf("%02X",*ptr++);
				else fputs("  ",stdout);
				inf++; loc++;
				if(!(loc % 2)) putchar(' ');
				if(!(loc % 4)) putchar(' ');
			}
			inf = deb;
			ptr = ligne;
			while(inf < sup) {
				if(isprint(*ptr)) putchar(*ptr);
				else putchar('\\');
				ptr++;
				inf++;
			}
			putchar('\n');
		}
		Adrs += Nb;
	}
}
/* ======================================================================= */
int Ouvrir() {
	struct stat infos;

	if(Fichier) close(Fichier);
	Dim = 0;
	Adrs = 0;
	Fichier = open(NomFichier,O_RDONLY);
	if(Fichier > 0) {
		printf("========== Fichier %s ==========\n",NomFichier);
		if(stat(NomFichier,&infos) == -1)
			printf(". Taille du fichier indisponible: %s\n",strerror(errno));
		else {
			Dim = infos.st_size;
			if(Dim < 0x100000) 
				printf(". Fichier de %.3f ko (%lld octets, 0x%04llX)\n",
					(float)Dim/1024.0,Dim,Dim);
			else printf(". Fichier de %.3f Mo (%lld octets, 0x%08llX)\n",
			   (float)Dim/(1024.0 * 1024.0),Dim,Dim);
		}
	} else {
		OpiumError("Fichier '%s' inaccessible (%s)",NomFichier,strerror(errno));
		Fichier = 0;
	}
	return(0);
}
/* ======================================================================= */
int Lequel() {
	if(OpiumReadFile("Fichier a examiner",NomFichier,MAXFILENAME) == PNL_CANCEL) return(0);
	Ouvrir();
	return(0);
}
/* ======================================================================= */
int AllerA() {
	if(!Fichier) { OpiumError("Pas de fichier ouvert"); return(0); }
	if(OpiumReadLHex("Adresse (octets)",&Adrs) == PNL_CANCEL) return(0);
	if(repositionne(1)) imprime();
	return(0);
}
/* ======================================================================= */
int Precedent() {
	if(!Fichier) { OpiumError("Pas de fichier ouvert"); return(0); }
	Adrs -= 2 * Max;
	if(repositionne(1)) imprime();
	return(0);
}
/* ======================================================================= */
int Suivant() {
	if(!Fichier) { OpiumError("Pas de fichier ouvert"); return(0); }
	imprime();
	return(0);
}
/* ======================================================================= */
int Chercher() {
	size_t taille,deb,max,lngr; char place;
	off_t initiale;

	if(OpiumReadFile("Texte a trouver",TexteCherche,MAXFILENAME) == PNL_CANCEL) return(0);
	lngr = strlen(TexteCherche);
	initiale = Adrs;
	do {
		taille = Max;
		if((Adrs + Max) > Dim) taille = Dim - Adrs;
		if(taille < lngr) {
			OpiumError("Texte '%s' introuvable",TexteCherche);
			Adrs = initiale; repositionne(0);
			break;
		}
		Nb = read(Fichier,Buffer,taille);
		if(Nb <= 0) {
			OpiumError("Lecture de %x octet(s) a %08llX en erreur (%s)",
				taille,Adrs,strerror(errno));
			Adrs = 0;
			repositionne(1);
		} else {
			max = Nb - lngr;
			for(deb=0; deb<max; deb++) if(!strncmp((char *)(Buffer+deb),TexteCherche,lngr)) break;
			Adrs += deb;
			place = repositionne(0);
			if(deb < max) { if(place) imprime(); break; }
			else { if(max <= 0) Adrs += 1; continue; }
		}
	} while(1);
	return(0);
}
/* ======================================================================= */
int Blocs() {
	OpiumReadHex("Dimension des blocs (octets)",&Max);
	return(0);
}
#ifdef OPIUM_H
/* ======================================================================= */
Menu mPpal;
MenuItem iPpal[] = {
	{ "Ouvrir",    MNU_FONCTION  Lequel },
	{ "Aller a",   MNU_FONCTION  AllerA },
	{ "Precedent", MNU_FONCTION  Precedent },
	{ "Suivant",   MNU_FONCTION  Suivant },
	{ "Chercher",  MNU_FONCTION  Chercher },
	{ "Blocs",     MNU_FONCTION  Blocs },
	{ "Quitter",   MNU_RETOUR },
	MNU_END
};
#else
/* ======================================================================= */
MenuItem mPpal[] = {
	{ "Ouvrir",      Lequel },
	{ "Aller a",     AllerA },
	{ "Precedent",   Precedent },
	{ "Suivant",     Suivant },
	{ "Chercher",    Chercher },
	{ "Blocs",       Blocs },
	{ "Quitter",     0 },
	{ "\0",          0 }
};
#endif
/* ======================================================================= */
int main(argc,argv) int argc; char *argv[]; {
	size_t l;

	Fichier = 0;
	Adrs = 0;
	Max = 256;
#ifdef OPIUM_H
	OpiumInit(0);
#endif
	
	if(argc > 1) {
		strcpy(NomFichier,argv[1]);
		Ouvrir(); imprime();
	} else {
		getcwd(NomFichier,MAXFILENAME);
		PathDelim[0] = (NomFichier[0] == '/')? '/': ':'; PathDelim[1] = '\0';
		if((l = strlen(NomFichier))) {
			if(strcmp(NomFichier+l-1,PathDelim)) strcat(NomFichier,PathDelim);
		}
		strcat(NomFichier,"fichier");
	}

#ifdef OPIUM_H
	mPpal = MenuCreate(iPpal);
	OpiumExec(mPpal->cdr);
#else
	MenuExec(mPpal);
#endif

	if(Fichier) close(Fichier);
#ifdef OPIUM_H
	OpiumExit();
#endif
	exit(0);
}
