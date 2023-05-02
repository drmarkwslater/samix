#include "environnement.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <termios.h>
#include <ctype.h>     /* pour isprint et al., ainsi que toupper */
#include <sys/fcntl.h> /* pour Clavier* */
/* ca pour sysctlbyname */
#include <sys/types.h>
#include <sys/sysctl.h>
#ifdef _SYS_SYSCTL_H
//	#define _SYS_SYSCTL_H_
#endif
/* et ca pour stat */
#ifdef CODE_WARRIOR_VSN
	#include <Types.h>
	#include <stat.h>
#else
	#include <sys/stat.h>
	#include <sys/types.h>
#endif
#include <unistd.h>

#ifdef XCODE
	/* tout ca pour juste 'read' ... */
	#include <sys/uio.h>
#endif

#ifdef WIN32
	#include <io.h>
#endif

#ifdef CODE_WARRIOR_VSN
	#include <utsname.h> /* pour uname */
#else
	#include <sys/utsname.h> /* pour uname */
#endif

#define DECODE_C
#include "decode.h"

#define EOL_IS_0A

extern char *__progname;

static char Home[256];
static char DecodeTexte[256];

static struct termios ClavierNormal,ClavierBrut;
static int ClavierParms = -1;
static char ClavierEcriture[8];

/* Voir /etc/apache2/mime.types */
static char *MimeExtension[] = {
	"txt",
	"doc", "xls", "ppt", "docx", "xlsx", "pptx",
	"csv", "jpg", "jpeg", "pdf", "html", "\0"
};
static char *MimeTypeListe[] = {
	"text/text",
	"application/vnd.ms-word", "application/vnd.ms-excel", "application/vnd.ms-powerpoint",
	"application/vnd.ms-word", "application/vnd.ms-excel", "application/vnd.ms-powerpoint",
	"text/csv", "image/jpeg", "image/jpeg", "application/pdf", "application/html", "\0"
};

/* ========================================================================== */
char *MimeTypeOf(char *fichier) {
	char *p; int i;

	if((p = rindex(fichier,'.'))) {
		p++; i = 0;
		while(MimeExtension[i][0] != '\0') {
			if(!strcmp(p,MimeExtension[i])) return(MimeTypeListe[i]);
			i++;
		}
	}
	return(MimeTypeListe[0]);
}
#ifdef linux
/* ========================================================================== */
char *strerror(int num) {
  sprintf(DecodeTexte,"Erreur #%d",num);
  return(DecodeTexte);
}
#endif
#ifdef _SYS_SYSCTL_H_
/* ========================================================================== */
int System_int32(char *texte) {
	int val; size_t lngr;

	lngr = sizeof(val);
#ifdef _SYS_SYSCTL_H
	int mib[CTL_MAXNAME]; size_t dim;
	sysctlnametomib(texte,mib,&dim);
	sysctl(mib,dim,&val,&lngr,NULL,0);
#else
	sysctlbyname(texte,&val,&lngr,NULL,0);
#endif
	return(val);
}
/* ========================================================================== */
int64 System_int64(char *texte) {
	int64 val; size_t lngr;

	lngr = sizeof(val);
#ifdef _SYS_SYSCTL_H
	int mib[CTL_MAXNAME]; size_t dim;
	sysctlnametomib(texte,mib,&dim);
	sysctl(mib,dim,&val,&lngr,NULL,0);
#else
	sysctlbyname(texte,&val,&lngr,NULL,0);
#endif
	return(val);
}
/* ========================================================================== */
char *System_texte(char *texte) {
	size_t lngr;

	lngr = sizeof(DecodeTexte);
#ifdef _SYS_SYSCTL_H
	int mib[CTL_MAXNAME]; size_t dim;
	sysctlnametomib(texte,mib,&dim);
	sysctl(mib,dim,DecodeTexte,&lngr,NULL,0);
#else
	sysctlbyname(texte,DecodeTexte,&lngr,NULL,0);
	return(DecodeTexte);
#endif
}
#endif /* _SYS_SYSCTL_H_ */
/* ========================================================================== */
char EndianIsBig(void) {
	union {
		char octet[2];
		unsigned short mot;
	} preuve;
	preuve.octet[0] = 1; preuve.octet[1] = 0;
	return(preuve.mot > 255);
}
/* ========================================================================== */
char *NomApplication(void) {
#ifdef CODE_WARRIOR_VSN
	return("<application>");
#else /* !CODE_WARRIOR_VSN */
	#ifdef OS9
		return(_prgname());
	#else /* !OS9 */
		#ifdef __USE_GNU
			return(program_invocation_name); /* dans errno.h */
		#else /* !__USE_GNU */
			return(__progname);
		#endif /* !__USE_GNU */
	#endif /* !OS9 */
#endif /* !CODE_WARRIOR_VSN */
}
/* ========================================================================== */
char **CallStackGet(int *nb) {
	void *callstack[256];

	*nb = backtrace(callstack,256);
	return(backtrace_symbols(callstack,*nb));
}
/* ========================================================================== */
void CallStackInfo(char *niveau, char **rang, char **lib, char **adresse, char **nom) {
	char *r,*s;

/* si char **niveaux = CallStackGet(&nb), on peut utiliser CallStackInfo(niveaux[i],&rang,&lib,&adresse,&nom);
	avec char *rang,*lib,*adresse,*nom; (et int i,nb;) */
	//	printf("(%s)   Niveau: \"%s\"\n",__func__,niveau);
#ifdef Darwin
	s = niveau;
	r = s; while((*s != '\0') && (*s != ' ')) s++; if(*s != '\0') { *s = '\0'; s++; }
	*rang = r;
	//	printf("(%s) rang @%02d: \"%s\"\n",__func__,(int)(r - niveau),r);
	while((*s != '\0') && (*s == ' ')) s++; r = s; while((*s != '\0') && (*s != ' ')) s++; if(*s != '\0') { *s = '\0'; s++; };
	*lib = r;
	//	printf("(%s)  lib @%02d: \"%s\"\n",__func__,(int)(r - niveau),r);
	while((*s != '\0') && (*s == ' ')) s++; r = s; while((*s != '\0') && (*s != ' ')) s++; if(*s != '\0') { *s = '\0'; s++; };
	*adresse = r;
	//	printf("(%s) adrs @%02d: \"%s\"\n",__func__,(int)(r - niveau),r);
	//	printf("(%s)  nom @%02d: \"%s\"\n",__func__,(int)(s - niveau),s);
	*nom = s;
#endif
#ifdef Linux
	char *slash;
	s = niveau; slash = 0;
	r = s; while((*s != '\0') && (*s != ' ')) { if(*s == '/') slash = s; s++; } if(*s != '\0') { *s = '\0'; s++; }
	if(slash) { *slash++ = '\0'; *lib = r; *nom = slash; }
	else *nom = r;
	while((*s != '\0') && (*s == ' ')) s++; r = s; while((*s != '\0') && (*s != ']')) s++; if(*s != '\0') { r++; *s = '\0'; s++; };
	*adresse = r;
	if(!slash) *lib = s;
#endif
}
/* ========================================================================== */
void RepertoireInit(char *home) {
	char *d;

	if((d = getenv("HOME"))) strcpy(Home,d);
	else if(!getcwd(Home,256)) strcpy(Home,"/Users/acquis/");
	RepertoireDelim[0] = ((Home[0] == '/') || (Home[0] == '\\'))? Home[0]: ':';
	RepertoireDelim[1] = '\0';
	if(home) {
		strcpy(home,Home);
		RepertoireTermine(home);
	}
}
/* ========================================================================== */
void RepertoirePrefs(char *home, char *appli, char *prefs) {
	strcat(strcat(strcpy(prefs,home),"Library/Application Support/"),appli);
	RepertoireAbsent(prefs);
	strcat(strcat(prefs,"/"),"prefs");
}
/* ========================================================================== */
void RepertoireTermine(char *chemin) {
	size_t l;
	if((l = strlen(chemin))) {
		if(strcmp(chemin+l-1,RepertoireDelim)) strcat(chemin,RepertoireDelim);
	}
}
/* ========================================================================== */
void RepertoireSimplifie(char *chemin) {
	char nom_final[MAXFILENAME];
	size_t i,l; char *c,*s;

	nom_final[0] = '\0';
	l = strlen(chemin); s = c = chemin;
	for(i=0; i<l; i++,c++) {
		if(!strncmp(c,"/../",4)) {
//			printf("Arrive a '%s', ",c);
			c += 3; i += 3; s = c;
//			printf("repart a '%s'\n",c);
		} else if(!strcmp(c,"/..")) {
//			printf("Stoppe a '%s'\n",c);
			s = c;
			break;
		} else if(*c == '/') {
			if(c > s) {
				strncat(nom_final,s,c - s);
				if(!strncmp(c,"/.",2)) c += 2;
//				printf("Arrive de '%s' a '%s', retient '%s'\n",s,c,nom_final);
				s = c;
			}
		}
	}
	if(c > s) {
		strncat(nom_final,s,c - s);
//		printf("Arrive de '%s' a '%s', retient '%s'\n",s,c,nom_final);
		s = c;
	}
	strcpy(chemin,nom_final);
}
/* ========================================================================== */
static int RepertoireTrouveRacine(char *nom_complet, char *racine) {
	/* retourne la racine la plus precise possible contenue dans le nom complet */
	char *nom; size_t l;

	//= printf("> Verification de la racine de '%s'\n",nom_complet);
	//- nom = rindex(nom_complet,SLASH); si nom_complet se termine par SLASH, on ne doit pas juste le retirer
	//- if(!nom) return(0); /* pas de repertoire reference, donc pas de racine */
	l = strlen(nom_complet);
	if(l >= 4) /* pas de racine si pas au minimum '/a/b' */ {
		nom = &(nom_complet[l - 2]);
		while(nom != nom_complet) { if(*nom == SLASH) break; nom--; }
		l = nom - nom_complet;
		//= if(l == 0) printf("> C'est un /device\n");
		if(l == 0) return(0); /* forme '/device' ou nom sans racine */
		strncpy(racine,nom_complet,l); racine[l] = '\0';
		return((int)l);
	} else return(0);
}
/* ========================================================================== */
void RepertoireExtrait(char *nom_complet, char *base, char *nom_local) {
	/* retourne la racine la plus precise possible et le nom local
	 contenu dans le nom complet */
	char racine[MAXFILENAME]; int l;

	l = RepertoireTrouveRacine(nom_complet,racine);
	if(base) strcpy(base,racine);
	if(nom_local) {
		if((l > 0) || (nom_complet[0] == '/')) strcpy(nom_local,nom_complet+l+1);
		else  strcpy(nom_local,nom_complet);
		l = (int)strlen(nom_local);
		if(l) { --l; if(nom_local[l] == SLASH) nom_local[l] = '\0'; }
	}
}
/* ========================================================================== */
void RepertoireRelatif(char *base, char *nom_complet, char *nom_local) {
	/*  Rend le nom <nom_local> qu'il faudra ajouter a <base>
	pour retrouver le fichier <nom_complet> (<base> sensee terminee par '/') */
	char *b,*c,*d,*s,*p;

	b = base; c = nom_complet; s = d = b + 1;
	while(*b == *c) {
		if(*b == '\0') break;
		if(*b == SLASH) s = b + 1;
		b++; c++;
	}
	*nom_local = '\0';
	if(*b || (s != b)) {
		do {
			p = d;
			d = s;
			s = index(d,SLASH);
			if(s == 0) { s = d; d = p; }
			s++;
			if(s != d) strcat(strcat(nom_local,".."),RepertoireDelim);
		} while(*s);
	}
	strcat(nom_local,c);
}
/* ========================================================================== */
void RepertoireNomComplet(char *base, char *nom_demande, char rep, char *nom_complet) {

	if(nom_demande[0] != SLASH) {
		if(!strcmp(nom_demande,".") || !strcmp(nom_demande,"./")) strcpy(nom_complet,base);
		else {
			strcat(strcpy(nom_complet,base),nom_demande);
			RepertoireSimplifie(nom_complet);
		}
	} else strcpy(nom_complet,nom_demande);
	if(rep == REPERT_REP) RepertoireTermine(nom_complet);
}
/* ========================================================================== */
void RepertoireIdentique(char *base, char *nom_local, char *nom_complet) {
	/*  Cree le chemin complet pour un fichier <nom_local>
	 situe dans le meme repertoire que <base> */
	char *nom; size_t l;

	if(nom_local[0] == SLASH) strcpy(nom_complet,nom_local);
	else {
		nom = rindex(base,SLASH);
		if(nom) {
			l = nom - base + 1;
			strncpy(nom_complet,base,l); nom_complet[l] = '\0';
			strcat(nom_complet,nom_local);
		} else strcpy(nom_complet,nom_local);
	}
}
/* ========================================================================== */
char RepertoireExiste(char *chemin) {
/* Indique si <chemin> est bien un repertoire */
#ifndef CODE_WARRIOR_VSN
	DIR *repert;

	repert = opendir(chemin);
	if(repert) { closedir(repert); return(1); }
#endif
	return(0);
}
/* ========================================================================== */
int RepertoireAbsent(char *chemin) {
/* Au cas ou <chemin> ne serait pas un repertoire existant, tente de le creer */
	int rc;
#ifndef CODE_WARRIOR_VSN
	char commande[MAXFILENAME]; DIR *repert;
#endif

	//= printf("> Verification du chemin '%s'\n",chemin);
	rc = 0;
#ifndef CODE_WARRIOR_VSN
	repert = opendir(chemin);
	if(repert) closedir(repert);
	else {
		printf("> Le chemin '%s' n'existe pas\n",chemin);
		if((rc = RepertoireCreeRacine(chemin))) return(rc);
		errno = 0;
		sprintf(commande,"mkdir \"%s\"",chemin);
		rc = system(commande);  /* rc = exit status du shell */
		printf("> Le chemin '%s' %s cree\n",chemin,rc? "N'A PAS PU etre ": "a ete");
	}
#endif
	return(rc);
}
/* ========================================================================== */
int RepertoireCreeRacine(char *nom_complet) {
/* Cree tous les repertoires necessaires a l'acces au fichier <nom_complet> */
	char racine[MAXFILENAME];

#ifdef ANCIENNE_PROGRAMMATION
	char *nom; size_t l; int rc;

	//= printf("> Verification de la racine de '%s'\n",nom_complet);
	//- nom = rindex(nom_complet,SLASH); si nom_complet se termine par SLASH, on ne doit pas juste le retirer
	//- if(!nom) return(0); /* pas de repertoire reference, donc pas de racine */
	l = strlen(nom_complet);
	if(l >= 4) /* pas de racine si au minimum '/a/b' */ {
		nom = &(nom_complet[l - 2]);
		while(nom != nom_complet) { if(*nom == SLASH) break; nom--; }
		l = nom - nom_complet;
		//= if(l == 0) printf("> C'est un /device\n");
		if(l == 0) return(0); /* forme '/device' */
		strncpy(racine,nom_complet,l); racine[l] = '\0';
		rc = RepertoireAbsent(racine);
		return(rc);
	} else return(0);
#else
	if(RepertoireTrouveRacine(nom_complet,racine))
		return(RepertoireAbsent(racine));
	else return(0);
#endif
}
/* ========================================================================== */
void RepertoireListeCree(char rep, char *chemin, char *liste[], int max, int *nb) {
/*  Cree la liste des fichiers contenus dans le repertoire <chemin>
	rep = REPERT_FIC: ne stocke que les fichiers;
		  REPERT_REP: ne stocke que les repertoires;
	      REPERT_ALL: stocke tout.
	type_dir = 0: est un fichier;
	           1: est un repertoire. */
#ifndef CODE_WARRIOR_VSN
	DIR *repert; struct dirent *entree;
	struct stat infos;
#endif
	char type_dir;
	char tempo[MAXFILENAME];
	size_t l; int n;

//	printf("* Liste des %ss du repertoire %s\n",rep? "repertoire": "fichier",chemin);
	n = 0;
#ifndef CODE_WARRIOR_VSN
	l = strlen(chemin) - 1;
	repert = opendir(chemin);
//	printf("* Repertoire %s\n",repert? "accessible": "inutilisable");
	if(repert) {
		while((entree = readdir(repert)) != NULL) {
			// if(!strcmp(entree->d_name,".")) continue;
			// if(!strcmp(entree->d_name,"..")) continue;
//			printf("* Trouve: '%s'\n",entree->d_name);
			if(entree->d_name[0] == '.') continue;
			if(chemin[l] == SLASH) strcat(strcpy(tempo,chemin),entree->d_name);
			else snprintf(tempo,MAXFILENAME,"%s%c%s",chemin,SLASH,entree->d_name);
			if(stat(tempo,&infos) == -1) type_dir = 0;
			else type_dir = S_ISDIR(infos.st_mode);
//			printf("  de type %s\n",type_dir? "repertoire": "fichier");
			if((type_dir == rep) || (rep == REPERT_ALL)) {
				if(n >= (max - 1)) break;
//-			#ifdef XCODE
//-				liste[n] = (char *)malloc(entree->d_namlen+1);
//-			#else
				liste[n] = (char *)malloc(strlen(entree->d_name)+1);
//-			#endif
				// printf("* Creation du nom[%d][%ld] @%08llX: '%s'\n",n,strlen(entree->d_name),(hex64)liste[n],entree->d_name);
				if(liste[n]) { strcpy(liste[n],entree->d_name); n++; }
			}
		}
		closedir(repert);
//		printf("* Repertoire ferme avec %d element%s\n",Accord1s(n));
	}
//	else printf("  (%s)\n",strerror(errno));
	if(n < max) { liste[n] = (char *)malloc(1); *(liste[n]) = '\0'; }
#endif
	*nb = n;

/* Note:
	stat(nom_fichier,&infos) rend un "struct stat"
	info.st_birthtimespec est de type "struct timespec"
	si struct timespec date; -> date contient tv_sec et tv_nsec; */
}
/* ========================================================================== */
void RepertoireListeLibere(char *liste[], int nb) {
	int i;
	if(nb) for(i=0; i<=nb; i++) free(liste[i]);
}
/* ========================================================================== */
int RepertoireFichierInfo(char *nom, char *texte) {
	struct stat infos; int k,l;

	l = sprintf(texte,"| Etat du fichier %s:\n",nom); k = l;
	stat(nom,&infos);
	l = sprintf(texte+k,"| .  uid: %4d\n",infos.st_uid); k += l;
	l = sprintf(texte+k,"| .  gid: %4d\n",infos.st_gid); k += l;
	l = sprintf(texte+k,"| . mode: %04X\n",infos.st_mode); k += l;
	l = sprintf(texte+k,"| . size: %lld octets\n",(int64)(infos.st_size)); k += l; // st_size: long sous linux
	return(k);
}
/* ========================================================================== */
char EditeurOuvre(char *nom_complet) {
	FILE *fic; char commande[MAXFILENAME];

	if((fic = fopen(nom_complet,"r"))) {
		fclose(fic); sprintf(commande,"open -e %s",nom_complet); system(commande);
		return(1);
	}
	return(0);
}
/* ========================================================================== */
void UsbListeCree(char *liste[], int max, int *nb) {
#define DEVICES_MAX 4096
	char *devices[DEVICES_MAX]; int devicesNb;
	char dbg; int n;

	dbg = 0;
	RepertoireListeCree(0,"/dev",devices,DEVICES_MAX,&devicesNb);
	*nb = 0;
	if(dbg) printf("Devices connus:");
	for(n=0; n<devicesNb; n++) {
		if(dbg) { if(!(n % 10)) printf("\n"); printf("%12s ",devices[n]); }
		if(!strncmp(devices[n],"tty.usbserial",13)) {
			strcpy(liste[*nb],devices[n]+13);
			*nb += 1; if(*nb >= (max - 1)) break;
		}
	}
	if(dbg) printf("\n");
//	if(*nb < max) *liste[*nb] = '\0';
}
/* ========================================================================== */
int UsbOpen(char *nom) {
	char chemin[MAXFILENAME];
	struct termios parms_terminal;
	int port;

	strcat(strcpy(chemin,"/dev/tty.usbserial"),nom);
    port = open(chemin,O_RDWR | O_NOCTTY | O_NDELAY);
    if(port < 0) return(-1); /* Acces au port USB impossible */
	tcgetattr(port, &parms_terminal);
	tcflush(port, TCIFLUSH);
	parms_terminal.c_cflag = B9600 | CS7 |CREAD | CLOCAL | HUPCL;
	cfsetospeed(&parms_terminal, B9600);
	tcsetattr(port, TCSANOW, &parms_terminal);
	return(port);
}
/* ========================================================================= */
char UsbWrite(int port, char *ligne, char *decode) {
	char *c,d; char escape;

	// int l; l = strlen(ligne); ligne[l++] = '\r'; ligne[l] = '\0';
	if(decode) {
		c = ligne; strcpy(decode,ClavierDump(*c++));
		while(*c) strcat(decode,ClavierDump(*c++));
	}
	c = ligne; escape = 0;
	while(*c) {
		if(*c == '\\') escape = 1;
		else {
			d = *c;
			if(escape) { if(*c == 'n') d = 0x0a; else if(*c == 'r') d = 0x0d; escape = 0; }
			if(decode) {
				if(c == ligne) strcpy(decode,ClavierDump(d));
				else strcat(decode,ClavierDump(d));
			}
			if(port >= 0) {
				if(write(port,&d,1) < 0) return(0);
				usleep(50000);
			}
		}
		c++;
	}
	return(1);
}
/* ========================================================================== */
char ClavierDeverouille(void) {
	tcgetattr(0,&ClavierNormal);
	ClavierBrut = ClavierNormal; // memcpy(&ClavierBrut,&ClavierNormal,sizeof(struct termios));
	// ClavierBrut.c_cflag &= ~(ISIG | ICANON | ECHO);
	cfmakeraw(&ClavierBrut);
	tcsetattr(0,TCSANOW,&ClavierBrut);
	ClavierParms = fcntl(0,F_SETFL,O_NONBLOCK);
	return((ClavierParms >= 0));
}
/* ========================================================================== */
char ClavierTouche(void) {
	int rc; if((rc = getchar()) > 0) return((char)rc); else return(0);
}
/* ========================================================================== */
char *ClavierDump(char c) {
	if(isprint(c)) sprintf(ClavierEcriture,"%c",c); else {
		if(c == 0x0a) sprintf(ClavierEcriture,"<LF>"); // \n ou <Ctrl-J
		else if(c == 0x0d) sprintf(ClavierEcriture,"<CR>"); // \r ou Ctrl-M ou <Return>
		else if((c > 0) && (c < 27)) sprintf(ClavierEcriture,"<^%c>",'A'+c-1);
		else if(c == 27) sprintf(ClavierEcriture,"<Esc>");
		else sprintf(ClavierEcriture,"<%02X>",(unsigned char)(c));
	}
	return(ClavierEcriture);
}
/* ========================================================================== */
void ClavierReverouille(void) {
	tcsetattr(0, TCSANOW, &ClavierNormal);
	if(ClavierParms >= 0) {
		ClavierParms &= ~O_NONBLOCK;
		fcntl(0,F_SETFL,ClavierParms);
	}
}
/* ========================================================================== */
char *LigneSuivante(char *ligne, int maxi, FILE *fichier) {
#ifdef EOL_IS_0A
	char *c; int m;
#endif

#ifdef EOL_IS_0A
	c = ligne; m = maxi - 2;
	do {
		if((*c = (char)getc(fichier)) == EOF)  {  /* EOF: EndOfFile OU Error... */
			if(c == ligne) return(NULL); else *c = '\n';
		} else if(*c == '\r') *c = '\n';
		if(*c == '\n') break; else { if(--m > 0) c++; else { *(++c) = '\n'; break; } }
	} while(1);
	*(++c) = '\0'; return(ligne);
#else
	return(fgets(ligne,maxi,fichier));
#endif
}
/* ========================================================================== */
char *EnregSuivant(char *ligne, int maxi, int p) {
#ifdef EOL_IS_0A
	char *c; int m;
#endif

#ifdef EOL_IS_0A
	c = ligne; m = maxi - 2;
	do {
		if(read(p,c,1) <= 0)  {  /* -1: Error / 0: EOF */
			if(c == ligne) return(NULL); else *c = '\n';
		} else if((*c == 0x0a) || (*c == '\r')) *c = '\n'; //! 0x0a == '\n'
		if(*c == '\n') break; else { if(--m > 0) c++; else { *(++c) = '\n'; break; } }
	} while(1);
	*(++c) = '\0'; return(ligne);
#else
	return(read(p,ligne,maxi));
#endif
}
/* ========================================================================== */
void NettoieLigne(char *ligne) {
	size_t l;

	l = strlen(ligne); while(l) {
		--l;
		if((ligne[l] == '\n') || (ligne[l] == 0x0a)) ligne[l] = '\0';
		else if(ligne[l] == ' ') ligne[l] = '\0';
		else break;
	}
}
#ifdef CODE_WARRIOR_VSN
/* ========================================================================== */
char *index(char *s, int c) {
	char k,*x;

	k = (char)c; x = s;
	while(*x) { if(*x == k) return(x); x++; }
	return(0);
}
/* ========================================================================== */
char *rindex(char *s, int c) {
	char k,*x;

	k = (char)c; x = s + strlen(s);
	while(--x >= s) if(*x == k) return(x);
	return(0);

}
#endif
/* ========================================================================== */
/* Routine a remplacer par ItemSuivant                                        */
/* Soit un texte, on recherche le prochain item termine par un certain caractere.
   La routine rend les pointeurs sur le debut de l'item et le debut du suivant.
   La chaine est garantie terminer par un caractere nul et sans blanc.
   Des guillemets (") permettent de masquer la recherche du delimiteur. Toutefois,
   ne sont retires de la chaine que le guillemet ouvrant, s'il est en premier
   caractere, et le guillemet fermant, s'il est en dernier caractere. */
void ItemScan(char *texte, char delimiteur, char **debut, char **suivant) {
	char *c;
	char guillemets;

	c = texte;
	while((*c != '\0') && (*c != '\n') && ((*c == ' ') || (*c == 0x09))) c++;
	if((guillemets = (*c == '\"'))) c++;
	*debut = c;
	while((*c != '\0') && (*c != '\n')) {
		if(guillemets) {
			if(*c++ == '\"') guillemets = 0;
		} else if(*c == delimiteur) break;
		else if((delimiteur == ' ') && (*c == 0x09)) break;
		else c++;
	}
	if(*c == '\n') *c = '\0';
	if(*c == '\0') *suivant = c; else *suivant = c + 1;
	--c;
	while((c > *debut) && ((*c == ' ') || (*c == 0x09))) --c;
	if(*c == '\"') *c = '\0';
	else if(c < *suivant) *(++c) = '\0';
}
#ifdef ABANDONNE
/* ========================================================================== */
/* Soit un texte, on recherche le prochain item termine par un certain caractere.
   La routine rend les pointeurs sur le debut de l'item et le debut du suivant.
   La chaine est garantie terminer par un caractere nul et sans blanc.
   Des guillemets (") permettent de masquer la recherche du delimiteur. Toutefois,
   ne sont retires de la chaine que le guillemet ouvrant, s'il est en premier
   caractere, et le guillemet fermant, s'il est en dernier caractere. */
char *ItemSuivant(char **texte, char delimiteur) {
	char *c,*debut,*suivant;
	char guillemets;

	c = *texte;
	while((*c != '\0') && (*c != '\n') && ((*c == ' ') || (*c == 0x09))) c++;
	if((guillemets = (*c == '\"'))) c++;
	debut = c;
	while((*c != '\0') && (*c != '\n')) {
		if(guillemets) {
			if(*c++ == '\"') guillemets = 0;
		} else if(*c == delimiteur) break;
		else if((delimiteur == ' ') && (*c == 0x09)) break;
		else c++;
	}
	if(*c == '\n') *c = '\0';
	if(*c == '\0') suivant = c; else suivant = c + 1;
	if(c > debut) {
		--c;
		while((c > debut) && ((*c == ' ') || (*c == 0x09))) --c;
		if((*c != '\"') && (c < suivant)) c++;
	}
	*c = '\0';
	*texte = suivant;
	return(debut);
}
/* ========================================================================== */
/* Soit un texte, on recherche le prochain item termine par un caractere parmi
   un ensemble (donne comme une chaine terminee par un \0).
   La routine rend les pointeurs sur le debut de l'item et le debut du suivant.
   La chaine est garantie terminer par un caractere nul et sans blanc.
   Des guillemets (") permettent de masquer la recherche du delimiteur. Toutefois,
   ne sont retires de la chaine que le guillemet ouvrant, s'il est en premier
   caractere, et le guillemet fermant, s'il est en dernier caractere. */
char *ItemTrouve(char **texte, char *delimiteurs, char *trouve) {
	char *c,*d,*debut,*suivant;
	char guillemets,blanc_demande;

	d = delimiteurs;
	while(*d) { if(*d == ' ') break; d++; }
	blanc_demande = (*d == ' ');
	c = *texte;
//	while((*c != '\0') && (*c != '\n') && (((*c == ' ') || (*c == 0x09)) ^ blanc_demande)) c++;
	while((*c != '\0') && (*c != '\n') && ((*c == ' ') || (*c == 0x09))) c++;
	if((guillemets = (*c == '\"'))) c++;
	debut = c;
	while((*c != '\0') && (*c != '\n')) {
		if(guillemets) {
			if(*c++ == '\"') guillemets = 0;
			continue;
		}
		d = delimiteurs;
		while(*d) { if(*c == *d) break; d++; }
		if(*d) break;
		else if(blanc_demande && (*c == 0x09)) break;
		else c++;
	}
	if(*c == 0x09) *trouve = ' ';
	else {
		if(*c == '\n') *c = '\0';
		*trouve = *c;
	}
	if(*c == '\0') suivant = c; else suivant = c + 1;
	if(c > debut) {
		--c;
		while((c > debut) && ((*c == ' ') || (*c == 0x09))) --c;
		if((*c != '\"') && (c < suivant)) c++;
	}
	*c = '\0';
	*texte = suivant;
	return(debut);
}
#endif /* ABANDONNE */
/* ========================================================================== */
/* Soit un texte, on recherche le prochain item termine par un certain caractere.
   La routine rend les pointeurs sur le debut de l'item et le debut du suivant.
   La chaine est garantie terminer par un caractere nul et sans blanc.
   Des guillemets (") permettent de masquer la recherche du delimiteur. Toutefois,
   ne sont retires de la chaine que le guillemet ouvrant, s'il est en premier
   caractere, et le guillemet fermant, s'il est en dernier caractere. */
char *ItemJusquA(char delimiteur, char **suite) {
	char *c,*debut,*suivant;
	char guillemets;

	c = *suite;
	while((*c != '\0') && (*c != '\n') && ((*c == ' ') || ((*c == 0x09) && (delimiteur != 0x09)))) c++;
	if((guillemets = (*c == '\"'))) c++;
	debut = c;
	while((*c != '\0') && (*c != '\n')) {
		if(*c == '\\') { c++; if((*c != '\0') && (*c != '\n')) c++; }
		else if(guillemets) { if(*c++ == '\"') guillemets = 0; }
		else if(*c == delimiteur) break;
		else if((delimiteur == ' ') && (*c == 0x09)) break;
		else c++;
	}
	if(*c == '\n') *c = '\0';
	if(*c == '\0') suivant = c; else suivant = c + 1;
	if(c > debut) {
		--c;
		while((c > debut) && ((*c == ' ') || (*c == 0x09))) --c;
		if((*c != '\"') && (c < suivant)) c++;
	}
	*c = '\0';
	*suite = suivant;
	return(debut);
}
/* ========================================================================== */
/* Soit un texte, on recherche le prochain item termine par un caractere parmi
   un ensemble (donne comme une chaine terminee par un \0).
   La routine rend les pointeurs sur le debut de l'item et le debut du suivant.
   La chaine est garantie terminer par un caractere nul et sans blanc.
   Des guillemets (") permettent de masquer la recherche du delimiteur. Toutefois,
   ne sont retires de la chaine que le guillemet ouvrant, s'il est en premier
   caractere, et le guillemet fermant, s'il est en dernier caractere. */
char *ItemAvant(char *delimiteur, char *trouve, char **suite) {
	char *c,*d,*debut,*suivant;
	char guillemets,blanc_demande,tab_demande;

	blanc_demande = tab_demande = 0;
	d = delimiteur;
	while(*d) { if(*d == ' ') blanc_demande = 1; else if(*d == 0x09) tab_demande = 1; d++; }
	//? blanc_demande = (*d == ' ');
	c = *suite;
//	while((*c != '\0') && (*c != '\n') && (((*c == ' ') || (*c == 0x09)) ^ blanc_demande)) c++;
	while((*c != '\0') && (*c != '\n') && ((*c == ' ') || ((*c == 0x09) && !tab_demande))) c++;
	if((guillemets = (*c == '\"'))) c++;
	debut = c;
	while((*c != '\0') && (*c != '\n')) {
		if(*c == '\\') { c++; if((*c != '\0') && (*c != '\n')) c++; continue; }
		if(guillemets) { if(*c++ == '\"') guillemets = 0; continue; }
		d = delimiteur;
		while(*d) { if(*c == *d) break; d++; }
		if(*d) break;
		else if(blanc_demande && (*c == 0x09)) break;
		else c++;
	}
	*trouve = ((*c == 0x09) && !tab_demande)? ' ': ((*c == '\n')? '\0': *c);
	suivant = (*c == '\0')? c: c + 1;
	if(c > debut) {
		--c;
		while((c > debut) && ((*c == ' ') || (*c == 0x09))) --c;
		if((*c != '\"') && (c < suivant)) c++;
	}
	*c = '\0';
	*suite = suivant;
	return(debut);
}
/* ========================================================================== */
char *ItemLigne(char **suite) {
	char *c,*debut;

	c = *suite; debut = c;
	while(*c != '\0') if(*c == '\n') break; else c++;
	if(*c == '\0') *suite = c; else { *c = '\0'; *suite = c + 1; }
	return(debut);
}
/* ========================================================================== */
void ItemImprimeHexa(char *lue) {
	int k; char eol;

	eol = 0;
	k = 0; while(lue[k] != '\0') { if(lue[k] == '\n') eol = 1; printf(" %c",lue[k]); k++; }
	if(!eol) printf("\n");
	k = 0; while(lue[k] != '\0') printf("%02x",(unsigned char)(lue[k++])); printf("\n");
}
// Codage MacRoman:
// '¤' -> 0xA4 -> 164
// 'Ž' -> 0x8E -> 142
// '' -> 0x8F -> 143
// 'ˆ' -> 0x88 -> 136
// '' -> 0x8D -> 141
// '' -> 0x9D -> 157
// 'µ' -> 0xB5 -> 181
// '¼' -> 0xBC -> 188
// '¬' -> 0xAC -> 172
#define CORRIGE_WINDOWS
/* ========================================================================== */
INLINE unsigned char *ItemRetireAccents(unsigned char *texte) {
	unsigned char *c,*d; char decale;

	c = d = texte; decale = 0;
	while(*c) {
		switch(*c) {
			case 0x80: *c = 'E'; break; /* 'Ž' */
			case 0x82: *c = 'e'; break; /* 'Ž' */
			case 0x85: *c = 'a'; break; /* 'ˆ' */
			case 0x88: *c = 'a'; break; /* 'ˆ' */
			case 0x8a: *c = 'e'; break; /* '' */
			case 0x8d: *c = 'c'; break; /* '' */
			case 0x8e: *c = 'e'; break; /* 'Ž' */
			case 0x8f: *c = 'e'; break; /* '' */
			case 0x90: *c = 'e'; break; /* '™' */
			case 0x94: *c = 'i'; break; /* '”' */
			case 0x99: *c = 'o'; break; /* '™' */
			case 0xa1: *c = 'o'; break; /* '¼' */
			case 0xa7: *c = 'c'; break; /* '' */
			case 0xb0: *c = 'o'; break; /* '¼' */
#ifdef CORRIGE_WINDOWS
			case 0xc2: c++; decale = 1;
				switch(*c) {
					case 0xa0: *d = ' ';  break; /* blanc */
					default: *d = *c;
				}
				break;
			case 0xc3: c++; decale = 1;
				switch(*c) {
					case 0x80: *d = 'A';  break; /* 'Ë' */
					case 0x88: *d = 'E';  break; /* 'é' */
					case 0x89: *d = 'E';  break; /* 'ƒ' */
					case 0x8a: *d = 'E';  break; /* 'æ' */
					case 0x94: *d = 'O';  break; /* 'ï' */
					case 0xa8: *d = 'e';  break; /* '' */
					case 0xa9: *d = 'e';  break; /* 'Ž' */
					default: *d = *c;
				}
				break;
#endif
			case 0xe0: *c = 'a'; break; /* 'ˆ' */
			case 0xe2: *c = 'a'; break; /* '‰' */
			case 0xe7: *c = 'c'; break; /* '' */
			case 0xe8: *c = 'e'; break; /* '' */
			case 0xe9: *c = 'e'; break; /* 'Ž' */
			case 0xea: *c = 'e'; break; /* '' */
			case 0xeb: *c = 'e'; break; /* '‘' */
			case 0xee: *c = 'i'; break; /* '”' */
			case 0xef: *c = 'i'; break; /* '•' */
			case 0xf4: *c = 'o'; break; /* '™' */
			case 0xf9: *c = 'u'; break; /* '' */
			case 0xfb: *c = 'u'; break; /* 'ž' */
#ifdef CORRIGE_WINDOWS
			default: if(decale) *d = *c;
#endif
		}
		c++; d++;
	};
	*d = '\0';

	return(texte);
}
/* ========================================================================== */
INLINE unsigned char *ItemCorrigeAccents(unsigned char *texte) {
    unsigned char *c,*d; char decale;

	c = d = texte; decale = 0;
	while(*c) {
		switch(*c) {
			case 0x80: *d = 'Û';  break; /* 'Û' */
			case 0x82: *d = 0x8e; break; /* 'Ž' */
			case 0x85: *d = 'ˆ';  break; /* 'ˆ' */
			case 0x88: *d = 'ˆ';  break; /* 'ˆ' */
			case 0x8a: *d = 0x8f; break; /* '' */
			case 0x90: *d = '';  break; /* '' */
			case 0x94: *d = '”';  break; /* '”' */
			case 0x99: *d = '™';  break; /* '™' */
			case 0xa1: *d = 0xba /* '¼' */ ;  break; /* '¼' */
			case 0xa7: *d = '';  break; /* '' */
			case 0xb0: *d = 0xba /* '¼' */ ;  break; /* '¼' */
#ifdef CORRIGE_WINDOWS
			case 0xc2: c++; decale = 1;
				switch(*c) {
					case 0xa0: *d = ' ';  break; /* blanc */
					default: *d = *c;
				}
				break;
			case 0xc3: c++; decale = 1;
				switch(*c) {
					case 0x80: *d = 'A';   break; /* 'Ë' */
					case 0x88: *d = 0x8e;  break; /* 'Ž' */
					case 0x89: *d = 'E';   break; /* 'ƒ' */
					case 0x8b: *d = 0x8f;  break; /* '' */
					case 0x8a: *d = 'E';   break; /* 'æ' */
					case 0x94: *d = 'O';   break; /* 'ï' */
					case 0xa0: *d = 'ˆ';   break; /* 'ˆ' */
					case 0xa8: *d = 0x8f;  break; /* '' */
					case 0xa9: *d = 0x8e;  break; /* 'Ž' */
					case 0xaa: *d = '';   break; /* '' */
					case 0xb4: *d = '™';   break; /* '™' */
					default:
//					etend c a l'infini a cause de "if(decale) *d = *c;" par la suite:
//						printf("!%02x%02x>",*(c-1),*c);
//						snprintf(code,4,"%02x",*c); code[2] = 0; printf("%s>",code);
//						*d++ = '@'; *d++ = code[0]; *d = code[1]; printf("%c%c%c!\n",*(d-2),*(d-1),*d);
						*d = *c;
				}
				break;
#endif
			case 0xe0: *d = 'ˆ';  break; /* 'ˆ' */
			case 0xe2: *d = '‰';  break; /* '‰' */
			case 0xe7: *d = 0x8d; break; /* '' */
			case 0xe8: *d = 0x8f; break; /* '' */
			case 0xe9: *d = 0x8e; break; /* 'Ž' */
			case 0xea: *d = '';  break; /* '' */
			case 0xeb: *d = '‘';  break; /* '‘' */
			case 0xee: *d = '”';  break; /* '”' */
			case 0xef: *d = '•';  break; /* '•' */
			case 0xf4: *d = '™';  break; /* '™' */
			case 0xf9: *d = '';  break; /* '' */
			case 0xfb: *d = 'ž';  break; /* 'ž' */
#ifdef CORRIGE_WINDOWS
			default: if(decale) *d = *c;
#endif
		}
		c++; d++;
	};
	*d = '\0';

	return(texte);
}
/* ========================================================================== */
INLINE unsigned char *ItemHtmlAccents(unsigned char *texte) {
	unsigned char *c,*d; char decale;

	c = d = texte; decale = 0;
	while(*c) {
		switch(*c) {
			case 0x82: *c = 0xe9; break; /* 'Ž' */
			case 0x85: *c = 0xe0; break; /* 'ˆ' */
			case 0x88: *c = 0xe0; break; /* 'ˆ' */
			case 0x89: *c = 0xe2; break; /* '‰' */
			case 0x8a: *c = 0xe8; break; /* '' */
			case 0x8d: *c = 0xe7; break; /* '' */
			case 0x8e: *c = 0xe9; break; /* 'Ž' */
			case 0x8f: *c = 0xe8; break; /* '' */
			case 0x90: *c = 0xea; break; /* '' */
			case 0x91: *c = 0xeb; break; /* '‘' */
			case 0x94: *c = 0xee; break; /* '”' */
			case 0x95: *c = 0xef; break; /* '•' */
			case 0x98: *c = 'o'; break; /* '˜' */
			case 0x99: *c = 0xf4; break; /* '™' */
			case 0x9a: *c = 'o'; break; /* 'š' */
			case 0x9d: *c = 0xf9; break; /* '™' */
			case 0x9e: *c = 0xfb; break; /* 'ž' */
			case 0x9f: *c = 'u'; break; /* 'Ÿ' */
			case 0xa1: *c = 0xba; break; /* '¼' */
			case 0xa7: *c = 0xe7; break; /* '' */
			case 0xb0: *c = 0xba; break; /* '¼' */
			case 0xdb: *c = 0x80; break; /* 'Û' */
#ifdef CORRIGE_WINDOWS
			case 0xc2: c++; decale = 1;
				switch(*c) {
					case 0xa0: *d = ' ';  break; /* blanc */
					default: *d = *c;
				}
				break;
			case 0xc3: c++; decale = 1;
				switch(*c) {
					case 0x80: *d = 'A';  break; /* 'Ë' */
					case 0x88: *d = 0x8e; break; /* 'é' */
					case 0x89: *d = 'E';  break; /* 'ƒ' */
					case 0x8a: *d = 'E';  break; /* 'æ' */
					case 0x8b: *d = 0x8f; break; /* '' */
					case 0x94: *d = 'O';  break; /* 'ï' */
					case 0xa0: *d = 0xe0; break; /* 'ˆ' */
					case 0xa8: *d = 0xe8; break; /* '' */
					case 0xa9: *d = 0xe9; break; /* 'Ž' */
					case 0xaa: *d = 0xea; break; /* '' */
					case 0xb4: *d = 0xf4; break; /* '™' */
					default: *d = *c;
				}
				break;
			default: if(decale) *d = *c;
#endif
		}
		c++; d++;
	};
	*d = '\0';

	return(texte);
}
/* ========================================================================== */
INLINE unsigned char *ItemMajuscules(unsigned char *texte) {
	unsigned char *c,d;

	c = texte; while(*c) { d = (unsigned char)toupper(*c); *c++ = d; };

	return(texte);
}
/* ========================================================================== */
INLINE unsigned char *ItemMinuscules(unsigned char *texte) {
	unsigned char *c,d;

	c = texte; while(*c) { d = (unsigned char)tolower(*c); *c++ = d; };

	return(texte);
}
/* ========================================================================== */
INLINE unsigned char *ItemCapitales(unsigned char *texte) {
	unsigned char *c,d;
	char premiere; unsigned char e[2];

	e[1] = '\0';
	premiere = 1;
	c = texte; while(*c) {
		e[0] = *c;
		ItemRetireAccents(e);
		if(isalpha(e[0]) && (*c != 0x80)) {
			if(premiere) { d = (unsigned char)toupper(*c); premiere = 0; }
			else d = (unsigned char)tolower(*c);
			*c++ = d;
		} else { premiere = 1; c++; }
	};

	return(texte);
}
/* ========================================================================== */
INLINE unsigned char *ItemRetireBlancs(unsigned char *texte) {
	unsigned char *c,*d; char decale;

	c = d = texte; decale = 0;
	while(*c != '\0') {
		if(*c == ' ') decale = 1;
		else {
			if(decale) *d = *c;
			d++;
		}
		c++;
	}
	if(decale) *d = '\0';

	return(texte);
}
/* ========================================================================== */
char *ItemVarAlloc(char *texte) {
	return(strcpy((char *)malloc(strlen(texte)+1),texte));
}
/* ========================================================================== */
void ItemVarSet(ItemVar var, char *nom, char *valeur) {
	var->nom = nom;
	var->valeur = valeur;
}
/* ========================================================================== */
static ItemVar ItemVarNew(void) {
	ItemVar var;
	var = (ItemVar)malloc(sizeof(TypeItemVar));
	ItemVarSet(var,ITEMVAR_END);
	var->svte = 0;
	return(var);
}
/* ========================================================================== */
ItemVar ItemVarAdd(ItemVar table, char *nom, char *valeur) {
	ItemVar var;

	if(table == 0) table = ItemVarNew();
	var = table; while(var->nom) var = var->svte;
	ItemVarSet(var,nom,valeur);
	var->svte = ItemVarNew();
	return(table);
}
/* ========================================================================== */
void ItemVarUsePrefix(ItemVar var, char *original, char *traduit, int lngr) {
	ItemVar trad; size_t l; char fait;

	trad = var; fait = 0;
	while(trad->nom) {
		l = strlen(trad->nom);
		if(!strncmp(original,trad->nom,l)) {
			strncat(strncpy(traduit,trad->valeur,lngr),original+l,lngr);
			fait = 1; break;
		}
		trad++;
	}
	if(!fait) strncpy(traduit,original,lngr);
}
/* ========================================================================== */
void ItemVarUse(ItemVar var, char *original, char *traduit, int lngr) {
	/* une seule substitution, prevoir de tout scanner */
	ItemVar trad; size_t k,l,m,n; char fait;

	fait = 0;
	if((trad = var)) {
		n = strlen(original);
		while(trad->nom) {
			l = strlen(trad->nom);
			m = n - l;
			for(k=0; k<m; k++) {
				if(!strncmp(original+k,trad->nom,l)) {
					if(k) strncat(strcat(strncpy(traduit,original,k),trad->valeur),original+k+l,lngr);
					else strncat(strncpy(traduit,trad->valeur,lngr),original+l,lngr);
					fait = 1; break;
				}
			}
			if(fait) break;
			trad++;
		}
	}
	if(!fait) strncpy(traduit,original,lngr);
}
#ifndef INLINE_DANS_DEFINEVAR
/* ========================================================================== */
INLINE void ByteSwappeInt(unsigned int *tab) {
	IntSwappable tempo;

	tempo.octet.b0 = ((IntSwappable *)tab)->octet.b3;
	tempo.octet.b1 = ((IntSwappable *)tab)->octet.b2;
	tempo.octet.b2 = ((IntSwappable *)tab)->octet.b1;
	tempo.octet.b3 = ((IntSwappable *)tab)->octet.b0;
	*tab = tempo.mot;
}
/* ========================================================================== */
INLINE void ByteSwappeLong(int64 *tab) {
	LongSwappable tempo;

	tempo.octet.b0 = ((LongSwappable *)tab)->octet.b7;
	tempo.octet.b1 = ((LongSwappable *)tab)->octet.b6;
	tempo.octet.b2 = ((LongSwappable *)tab)->octet.b5;
	tempo.octet.b3 = ((LongSwappable *)tab)->octet.b4;
	tempo.octet.b4 = ((LongSwappable *)tab)->octet.b3;
	tempo.octet.b5 = ((LongSwappable *)tab)->octet.b2;
	tempo.octet.b6 = ((LongSwappable *)tab)->octet.b1;
	tempo.octet.b7 = ((LongSwappable *)tab)->octet.b0;
	*tab = tempo.mot64;
}
/* ========================================================================== */
INLINE void ByteSwappeShort(unsigned short *tab) {
	ShortSwappable tempo;

	tempo.octet.lsb = ((ShortSwappable *)tab)->octet.msb;
	tempo.octet.msb = ((ShortSwappable *)tab)->octet.lsb;
	*tab = tempo.mot;
}
/* ========================================================================== */
INLINE void ByteSwappeIntArray(unsigned int *tab, int nb) {
	int i; unsigned int *adrs;
	IntSwappable tempo;

	for(i=0, adrs = tab; i<nb; i++, adrs++) {
		tempo.octet.b0 = ((IntSwappable *)adrs)->octet.b3;
		tempo.octet.b1 = ((IntSwappable *)adrs)->octet.b2;
		tempo.octet.b2 = ((IntSwappable *)adrs)->octet.b1;
		tempo.octet.b3 = ((IntSwappable *)adrs)->octet.b0;
		*adrs = tempo.mot;
	}
}
/* ========================================================================== */
INLINE void ByteSwappeFloatArray(float *tab, int nb) {
	int i; float *adrs;
	FloatSwappable tempo;

	for(i=0, adrs = tab; i<nb; i++, adrs++) {
		tempo.octet.b0 = ((IntSwappable *)adrs)->octet.b3;
		tempo.octet.b1 = ((IntSwappable *)adrs)->octet.b2;
		tempo.octet.b2 = ((IntSwappable *)adrs)->octet.b1;
		tempo.octet.b3 = ((IntSwappable *)adrs)->octet.b0;
		*adrs = tempo.mot;
	}
}
/* ========================================================================== */
INLINE void ByteSwappeLongArray(int64 *tab, int nb) {
	int i; int64 *adrs;
	LongSwappable tempo;

	for(i=0, adrs = tab; i<nb; i++, adrs++) {
		tempo.octet.b0 = ((LongSwappable *)adrs)->octet.b7;
		tempo.octet.b1 = ((LongSwappable *)adrs)->octet.b6;
		tempo.octet.b2 = ((LongSwappable *)adrs)->octet.b5;
		tempo.octet.b3 = ((LongSwappable *)adrs)->octet.b4;
		tempo.octet.b4 = ((LongSwappable *)adrs)->octet.b3;
		tempo.octet.b5 = ((LongSwappable *)adrs)->octet.b2;
		tempo.octet.b6 = ((LongSwappable *)adrs)->octet.b1;
		tempo.octet.b7 = ((LongSwappable *)adrs)->octet.b0;
		*adrs = tempo.mot64;
	}
}
/* ========================================================================== */
INLINE void ByteSwappeDoubleArray(double *tab, int nb) {
	int i; double *adrs;
	LongSwappable tempo;

	for(i=0, adrs = tab; i<nb; i++, adrs++) {
		tempo.octet.b0 = ((LongSwappable *)adrs)->octet.b7;
		tempo.octet.b1 = ((LongSwappable *)adrs)->octet.b6;
		tempo.octet.b2 = ((LongSwappable *)adrs)->octet.b5;
		tempo.octet.b3 = ((LongSwappable *)adrs)->octet.b4;
		tempo.octet.b4 = ((LongSwappable *)adrs)->octet.b3;
		tempo.octet.b5 = ((LongSwappable *)adrs)->octet.b2;
		tempo.octet.b6 = ((LongSwappable *)adrs)->octet.b1;
		tempo.octet.b7 = ((LongSwappable *)adrs)->octet.b0;
		*adrs = tempo.mot64;
	}
}
/* ========================================================================== */
INLINE void ByteSwappeShortArray(unsigned short *tab, int nb) {
	int i; unsigned short *adrs;
	ShortSwappable tempo;

	for(i=0, adrs = tab; i<nb; i++, adrs++) {
		tempo.octet.lsb = ((ShortSwappable *)adrs)->octet.msb;
		tempo.octet.msb = ((ShortSwappable *)adrs)->octet.lsb;
		*adrs = tempo.mot;
	}
}
/* ========================================================================== */
INLINE void TwoShortsSwappeInt(unsigned int *entier) {
	TwoShortsSwappable tempo;

	tempo.court.s1 = ((TwoShortsSwappable *)entier)->court.s0;
	tempo.court.s0 = ((TwoShortsSwappable *)entier)->court.s1;
	*entier = tempo.mot;
}
/* ========================================================================== */
INLINE void FourShortsSwappeLong(int64 *mot64) {
	FourShortsSwappable tempo;

	tempo.court.s3 = ((FourShortsSwappable *)mot64)->court.s0;
	tempo.court.s2 = ((FourShortsSwappable *)mot64)->court.s1;
	tempo.court.s1 = ((FourShortsSwappable *)mot64)->court.s2;
	tempo.court.s0 = ((FourShortsSwappable *)mot64)->court.s3;
	*mot64 = tempo.mot64;
}
/* ========================================================================== */
INLINE void TwoShortsSwappeIntArray(unsigned int *tab, int nb) {
	int i; unsigned int *adrs;
	TwoShortsSwappable tempo;

	for(i=0, adrs = tab; i<nb; i++, adrs++) {
		tempo.court.s1 = ((TwoShortsSwappable *)adrs)->court.s0;
		tempo.court.s0 = ((TwoShortsSwappable *)adrs)->court.s1;
		*adrs = tempo.mot;
	}
}
/* ========================================================================== */
INLINE void FourShortsSwappeLongArray(int64 *tab, int nb) {
	int i; int64 *adrs;
	FourShortsSwappable tempo;

	for(i=0, adrs = tab; i<nb; i++, adrs++) {
		tempo.court.s3 = ((FourShortsSwappable *)adrs)->court.s0;
		tempo.court.s2 = ((FourShortsSwappable *)adrs)->court.s1;
		tempo.court.s1 = ((FourShortsSwappable *)adrs)->court.s2;
		tempo.court.s0 = ((FourShortsSwappable *)adrs)->court.s3;
		*adrs = tempo.mot64;
	}
}
/* ========================================================================== */
/* !!! OBSOLETE, a remplacer par ByteSwappeShortArray !!!                */
INLINE void ByteSwappeWordArray(unsigned short *tab, int nb) {
	int i;
	unsigned short *adrs;
	ShortSwappable tempo;

	for(i=0, adrs = tab; i<nb; i++, adrs++) {
		tempo.octet.lsb = ((ShortSwappable *)adrs)->octet.msb;
		tempo.octet.msb = ((ShortSwappable *)adrs)->octet.lsb;
		*adrs = tempo.mot;
	}
}
/* ========================================================================== */
#ifdef CODE_WARRIOR_VSN
int Modulo(int64 v1, int v2)
#else
INLINE int Modulo(int64 v1, int v2)
#endif
	{
	int64 max,tours,point1;
	int k;

	max = (int64)v2;
	tours = v1 / max;
	point1 = v1 - (tours * max);
	k = (int)point1;
	return(k);
}
/* ========================================================================== */
INLINE int inintf(float x) { return((x >= 0.0)? (int)(x+ 0.5): (int)(x - 0.5)); }
#endif /* !INLINE_DANS_DEFINEVAR */

/* ========================================================================== */
void AudioWaveHdrInit(AudioWaveHdr hdr, int nb_voies, int nb_echantillons) {
	sprintf(hdr->encodage,"WAVE");
	hdr->freq_Hz = WAV_FREQ;  // 11025, 22050, 44100 et Žventuellement 48000 et 96000
	hdr->nb_bits = 16;        // 8, 16, 24
	hdr->format_stockage = 1; // 1: PCM entier (3: PCM flottant)

	hdr->nb_canaux = (unsigned short)nb_voies;
	hdr->dim_echantillon = hdr->nb_canaux * hdr->nb_bits / 8;
	hdr->dim_donnees = nb_echantillons * hdr->dim_echantillon;
	hdr->dim_totale = 36 + hdr->dim_donnees; // entete: 44-8 octets
	hdr->dim_bloc_fmt = 16;
	hdr->debit = hdr->freq_Hz * hdr->dim_echantillon;
	hdr->complet = 1;
}
/* ========================================================================== */
AudioWaveHdr AudioWaveHdrCreate(int nb_voies, int nb_echantillons) {
	AudioWaveHdr hdr;

	if(!(hdr = Malloc(1,TypeAudioWaveHdr))) return(hdr);
	AudioWaveHdrInit(hdr,nb_voies,nb_echantillons);

	return(hdr);
}
/* ========================================================================== */
unsigned short AudioWaveVoiesNb(AudioWaveHdr hdr) { return(hdr->nb_canaux); }
/* ========================================================================== */
float AudioWaveDuree(AudioWaveHdr hdr) {
	long nb_echantillons;

	if(hdr->complet && (hdr->freq_Hz > 0)) {
		nb_echantillons = hdr->dim_donnees / hdr->dim_echantillon;
		return((float)nb_echantillons / (float)(hdr->freq_Hz));
	} else return(0.0);
}
/* ========================================================================== */
FILE *AudioWaveCreate(char *nom_complet, AudioWaveHdr hdr) {
	// voir https://fr.wikipedia.org/wiki/Waveform_Audio_File_Format
	FILE *f;

	if(!(f = fopen(nom_complet,"wb"))) return(f);

	fwrite("RIFF",4,1,f);
	fwrite(&hdr->dim_totale,4,1,f);
	fwrite(hdr->encodage,4,1,f);

	fwrite("fmt ",4,1,f);
	fwrite(&hdr->dim_bloc_fmt,4,1,f);
	fwrite(&hdr->format_stockage,2,1,f);
	fwrite(&hdr->nb_canaux,2,1,f);
	fwrite(&hdr->freq_Hz,4,1,f);
	fwrite(&hdr->debit,4,1,f);
	fwrite(&hdr->dim_echantillon,2,1,f);
	fwrite(&hdr->nb_bits,2,1,f);

	fwrite("data",4,1,f);
	fwrite(&hdr->dim_donnees,4,1,f);

	return(f);
}
/* ========================================================================== */
FILE *AudioWaveInit(char *nom_complet, int nb_voies, int nb_echantillons) {
	TypeAudioWaveHdr hdr;

	AudioWaveHdrInit(&hdr,nb_voies,nb_echantillons);
	return(AudioWaveCreate(nom_complet,&hdr));
}
/* ========================================================================== */
FILE *AudioWaveRead(char *nom_complet, AudioWaveHdr hdr) {
	FILE *f; char nom_bloc[5]; char junk[32]; char trouves;

	if(!(f = fopen(nom_complet,"rb"))) { hdr->complet = 0; return(f); }
	nom_bloc[4] = '\0'; trouves = 0;
	do {
		fread(nom_bloc,4,1,f);
		if(!strcmp(nom_bloc,"RIFF")) {
			fread(&(hdr->dim_totale),4,1,f);
			fread(hdr->encodage,4,1,f); hdr->encodage[4] = '\0';
			trouves++;
		} else if(!strcmp(nom_bloc,"JUNK")) {
			fread(junk,32,1,f);
		} else if(!strcmp(nom_bloc,"fmt ")) {
			fread(&(hdr->dim_bloc_fmt),4,1,f);
			fread(&(hdr->format_stockage),2,1,f);
			fread(&(hdr->nb_canaux),2,1,f);
			fread(&(hdr->freq_Hz),4,1,f);
			fread(&(hdr->debit),4,1,f);
			fread(&(hdr->dim_echantillon),2,1,f);
			fread(&(hdr->nb_bits),2,1,f);
			trouves++;
		} else if(!strcmp(nom_bloc,"data")) {
			fread(&(hdr->dim_donnees),4,1,f);
			trouves++;
		} else break;
	} forever;
	hdr->complet = (trouves >= 3);

	return(f);
}
/* ========================================================================== */
int AudioSpeakP(unsigned char *chaine_pascal) {
	char texte[1020],commande[1024]; int l;

	l = (int)chaine_pascal[0];
	if(l > 1019) l = 1019;
	memcpy(texte,chaine_pascal+1,l); texte[l] = '\0';
	snprintf(commande,1023,"say %s",texte);
	return(system(commande));
}
/* ========================================================================== */
int AudioSpeak(char *texte) {
	char commande[1024];

	snprintf(commande,1023,"say %s",texte);
	return(system(commande));
}
/* ========================================================================== */
int AudioFilePlay(char *wavpath, char *appli) {
	char commande[256]; int p;

	if((p = open(wavpath,O_RDONLY)) >= 0) close(p); else return(-1);
	if(appli && appli[0]) {
		if(!strcmp(appli,"afplay")) snprintf(commande,255,"%s %s",appli,wavpath);
		else snprintf(commande,255,"open -a %s %s",appli,wavpath);
	} else snprintf(commande,255,"open %s",wavpath);
	return(system(commande));
}
/* ========================================================================== */
FILE *SonWaveEntete(char *nom_complet, int nb_voies, int nb_echantillons) {
// voir https://fr.wikipedia.org/wiki/Waveform_Audio_File_Format
	FILE *f;
	char	encodage[5];
	short	format_stockage;
	long	dim_totale,dim_bloc_fmt,dim_donnees;
	unsigned short	nb_bits,nb_canaux,dim_echantillon;
	unsigned long	freq_Hz,debit;

	if(!(f = fopen(nom_complet,"wb"))) return(0);
	sprintf(encodage,"WAVE");
	freq_Hz = WAV_FREQ;  // 11025, 22050, 44100 et Žventuellement 48000 et 96000
	nb_bits = 16;        // 8, 16, 24
	format_stockage = 1; // 1: PCM

	nb_canaux = (unsigned short)nb_voies;
	dim_echantillon = nb_canaux * nb_bits / 8;
	dim_donnees = nb_echantillons * dim_echantillon;
	dim_totale = 36 + dim_donnees; // entete: 44-8 octets
	dim_bloc_fmt = 16;
	debit = freq_Hz * dim_echantillon;

	fwrite("RIFF",4,1,f);
	fwrite(&dim_totale,4,1,f);
	fwrite(encodage,4,1,f);

	fwrite("fmt ",4,1,f);
	fwrite(&dim_bloc_fmt,4,1,f);
	fwrite(&format_stockage,2,1,f);
	fwrite(&nb_canaux,2,1,f);
	fwrite(&freq_Hz,4,1,f);
	fwrite(&debit,4,1,f);
	fwrite(&dim_echantillon,2,1,f);
	fwrite(&nb_bits,2,1,f);

	fwrite("data",4,1,f);
	fwrite(&dim_donnees,4,1,f);

	return(f);
}
/* ========================================================================== */
void ImprimeNomSysteme(void) {
	struct utsname systeme;
#ifdef ESSAI
	ProcessInfoRec info;
#endif

	uname(&systeme);
#ifdef CODE_WARRIOR_VSN
	if(atoi(systeme.release) <= 9)
		printf("%s \"%s\" de %s\n  Systeme %s version %s.%s\n  CodeWarrior version %d\n  ---------------\n\n",
			   systeme.machine,systeme.nodename,getlogin(),
			   systeme.sysname,systeme.release,systeme.version,CODE_WARRIOR_VSN);
	else
#endif
		printf("%s \"%s\" de %s\nSysteme %s version %s.%d\n---------------\n\n",
			   systeme.machine,systeme.nodename,getlogin(),
			   systeme.sysname,systeme.release,atoi(systeme.version));
#ifdef ESSAI
	info.processName = (StringPtr)malloc(32);
	GetProcessInformation((ProcessSerialNumber *)kCurrentProcess,&info);
	printf("  Application %s\n",info.processName);
#endif
}

