#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <termios.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>

#define MAXFILENAME 256
#define MAXCHARS 80

#define Accord2s(var) var,(var>1)?"s":"",(var>1)?"s":""

#define CRLF "\r\n"

static int Port;
static unsigned long Bauds;

static struct termios ClavierNormal,ClavierBrut;
static int ClavierParms;
static char ClavierEcriture[8];
static char GenereCR,GenereLF,Debug,Datation;

#define DEVICES_MAX 4096
static char *Devices[DEVICES_MAX];
static int DevicesNb;

static char DriverDir[64];
static char DriverType[64];

/* ===================================================================== */
void RepertoireListeCree(char rep, char *chemin, char *liste[], int max, int *nb) {
	/* rep = 0: ne stocke que les fichiers; 1: ne stocke que les repertoires. */
	DIR *repert; struct dirent *entree;
	struct stat infos;
	char type_dir;
	char tempo[MAXFILENAME];
	int l,n;
	
	n = 0;
	l = strlen(chemin);
	repert = opendir(chemin);
	if(repert) {
		while((entree = readdir(repert)) != NULL) {
			if(entree->d_name[0] == '.') continue;
			if(chemin[l - 1] == '/') strcat(strcpy(tempo,chemin),entree->d_name);
			else sprintf(tempo,"%s/%s",chemin,entree->d_name);
			if(stat(tempo,&infos) == -1) type_dir = 0;
			else type_dir = S_ISDIR(infos.st_mode);
			if(type_dir == rep) {
				if(n >= (max - 1)) break;
#ifdef XCODE
				liste[n] = (char *)malloc(entree->d_namlen+1);
#else
				liste[n] = (char *)malloc(strlen(entree->d_name)+1);
#endif
				if(liste[n]) { strcpy(liste[n],entree->d_name); n++; }
			}
		}
		closedir(repert);
	}
	if(n < max) { liste[n] = (char *)malloc(1); *(liste[n]) = '\0'; }
	*nb = n;
}
/* ========================================================================= */
static char OuvreLePort() {
	struct termios parms_terminal;
	int n,nb,p,lngr;
	int port_serie[DEVICES_MAX];
	char chemin[MAXFILENAME];
	/* ports connus:
	 FTFO3ZKR
	 A6008XdS, soit au complet: "/dev/tty.usbserial-A6008XdS" (134.158.176.58)
	 */
	
	if(Port >= 0) { printf("! Un port est deja ouvert\n"); return(1); }
	
//	Debug = 0;
	RepertoireListeCree(0,DriverDir,Devices,DEVICES_MAX,&DevicesNb);
	nb = 0; lngr = strlen(DriverType);
	// if(Debug) printf("------------------------------\n* %d port%s connu%s:",Accord2s(DevicesNb));
	for(n=0; n<DevicesNb; n++) {
		// if(Debug) { if(!(n % 8)) printf("\n"); printf("%15s ",Devices[n]); }
		if(!strncmp(Devices[n],DriverType,lngr)) port_serie[nb++] = n;
	}
	if(Debug) {
		printf("\n------------------------------\n");
		printf("* %d port%s utilisable%s\n",Accord2s(nb));
		for(n=0; n<nb; n++) printf("%3d: %s ['%s']\n",n+1,Devices[port_serie[n]],Devices[port_serie[n]]+lngr);
	}
	
	if(!nb) {
        printf("!!! Pas de port de type '%s' actif (repertoire vu: %s)\n",DriverType,DriverDir);
		return(0);
	} else if(nb > 1) {
		do {
			printf("Choisis l'un des ports serie listes ci-dessous:\n");
			for(n=0; n<nb; n++) printf("%2d: %s\n",n+1,Devices[port_serie[n]]+lngr);
			printf("Numero du port choisi (entre 1 et %d) => ",nb); fflush(stdout);
			scanf("%d",&p);
		} while((p <= 0) || (p > nb));
		p--;
	} else p = 0;
	strcat(strcat(strcpy(chemin,DriverDir),"/"),Devices[port_serie[p]]);
    Port = open(chemin,O_RDWR | O_NOCTTY | O_NDELAY); //  sur SeDiNe
    if(Port < 0) {
        printf("!!! Acces au port '%s' impossible: %s\n",chemin,strerror(errno));
		return(0);
    } else printf("* Port '%s' ouvert a %ld bauds\n",chemin,Bauds);
	tcgetattr(Port, &parms_terminal);
	tcflush(Port, TCIFLUSH);
//	parms_terminal.c_cflag = CS7 | CREAD | CLOCAL | HUPCL;
	parms_terminal.c_cflag = CS8 | CREAD | CLOCAL | HUPCL;
	cfsetispeed(&parms_terminal,Bauds);
	cfsetospeed(&parms_terminal,Bauds);
	tcsetattr(Port, TCSANOW, &parms_terminal);
	// printf("Port USB ouvert\n");
	return(1);
}
/* ========================================================================= */
char ClavierDeverouille() {
	tcgetattr(0,&ClavierNormal);
	ClavierBrut = ClavierNormal; // memcpy(&ClavierBrut,&ClavierNormal,sizeof(struct termios));
	// ClavierBrut.c_cflag &= ~(ISIG | ICANON | ECHO);
	cfmakeraw(&ClavierBrut);
	tcsetattr(0,TCSANOW,&ClavierBrut);
	ClavierParms = fcntl(0,F_SETFL,O_NONBLOCK);
	return((ClavierParms >= 0));
}
/* ========================================================================= */
char *ClavierDump(char c) {
	if(isprint(c)) sprintf(ClavierEcriture,"%c",c); else {
		if(c == 0x0a) sprintf(ClavierEcriture,"<LF>"); // \n
		else if(c == 0x0d) sprintf(ClavierEcriture,"<CR>"); // \r
		else if((c > 0) && (c < 27)) sprintf(ClavierEcriture,"<^%c>",'A'+c-1);
		else if(c == 27) sprintf(ClavierEcriture,"<Esc>");
		else sprintf(ClavierEcriture,"<%02X>",(unsigned char)(c));
	}
	return(ClavierEcriture);
}
/* ========================================================================= */
char *ClavierTrad(char c) {
	if(GenereCR && (c == 0x0a)) sprintf(ClavierEcriture,CRLF);
	else if(isprint(c) || (c == 0x0a) || (c == 0x0d)) sprintf(ClavierEcriture,"%c",c);
	else {
		if((c > 0) && (c < 27)) sprintf(ClavierEcriture,"<^%c>",'A'+c-1);
		else if(c == 27) sprintf(ClavierEcriture,"<Esc>");
		else sprintf(ClavierEcriture,"<%02X>",(unsigned char)(c));
	}
	return(ClavierEcriture);
}
/* ========================================================================= */
char ClavierTouche() {
	int rc;
	if((rc = getchar()) > 0) return((char)rc); else return(0);
}
/* ========================================================================= */
void ClavierReverouille() {
	tcsetattr(0, TCSANOW, &ClavierNormal);
	if(ClavierParms >= 0) {
		ClavierParms &= ~O_NONBLOCK;
		fcntl(0,F_SETFL,ClavierParms);
	}
}
/* ========================================================================= */
char Envoi(char *ligne, char log) {
	int i,l;

	l = strlen(ligne); ligne[l++] = '\r'; ligne[l] = '\0';
	if(log) {
		printf("Envoye: '");
		l = strlen(ligne); for(i=0; i<l; i++) fputs(ClavierDump(ligne[i]),stdout);
		printf("'\n");
	}
	for(i=0; i<l; i++) {
		if(i) usleep(50000);
		if(write(Port,ligne+i,1) < 0) return(0);
	}
	return(1);
}
/* ========================================================================= */
int main(int argc, char *argv[]) {
	char ligne[MAXCHARS],c;
	int rc,l;
	char debut_ligne,escape;
	#define DE_VT100

//: screen /dev/tty.usbserial-DM02JZ94 115200 8N1
	Bauds = B9600; // B115200;
	GenereCR = 1; GenereLF = 0; Debug = 0; Datation = 0;
	strcpy(DriverDir,"/dev");
	strcpy(DriverType,"tty.usbserial-");
	l = 1;
	while(l < argc) {
		if(!strncmp(argv[l],"-b",2)) {
			// if(!strcmp(argv[l]+2,"9600")) Bauds = 9600;
			sscanf(argv[l]+2,"%ld",&Bauds);
		} else if(!strncmp(argv[l],"-r",2)) strcpy(DriverDir,argv[l]+2);
		else if(!strcmp(argv[l],"-date")) Datation = 1;
		else if(!strcmp(argv[l],"-debug")) Debug = 1;
		else if(!strcmp(argv[l],"-sansCR")) GenereCR = 0;
		else if(!strcmp(argv[l],"-avecLF")) GenereLF = 1;
		else if(!strcmp(argv[l],"-h")) {
			printf("options: -date, -debug, -sansCR, -avecLF, -b<baud> [%ld], -r<rep> [%s] <type-driver> [%s]\n",Bauds,DriverDir,DriverType);
			return(0);
		}
		else strcpy(DriverType,argv[l]);
		l++;
	}
	Port = -1; if(!OuvreLePort()) return(0);
	printf("Faire <Ctrl-Z> pour quitter la communication\n");
	if(!ClavierDeverouille()) printf("Attente clavier bloquante (%s)\n",strerror(errno));
	debut_ligne = 0; escape = 0;
	if(Datation) printf("        > ");
	do {
		if((ligne[0] = ClavierTouche())) {
			if(Debug) fputs(ClavierDump(ligne[0]),stdout);
			if(ligne[0] == 0x1A) { printf(CRLF); break; } else
			if(ligne[0] == 0x04) { Debug = 1 - Debug; printf("%s debug\r\n",Debug?"Avec":"Sans"); continue; } else {
				write(Port,ligne,1);
				if((ligne[0] == 0x0D) && GenereLF) { c = 0x0A; write(Port,&c,1); }
			}
		}
		do {
			rc = read(Port,ligne,MAXCHARS-1);
			if(rc > 0) {
				if(Debug) {
//					for(l=0; l<rc; l++) printf("%02x ",(unsigned char)ligne[l]); printf(CRLF);
//					for(l=0; l<rc; l++) printf(" %c ",isprint(ligne[l])? ligne[l]: '.'); printf(CRLF);
					printf(">Lu: {"); for(l=0; l<rc; l++) fputs(ClavierDump(ligne[l]),stdout); printf("}\r\n");
				} else for(l=0; l<rc; l++) {
					if(Datation && debut_ligne) {
						time_t temps; struct tm moment;
						time(&temps); memcpy(&moment,localtime(&temps),sizeof(struct tm));
						printf("%02d:%02d:%02d/ ",moment.tm_hour,moment.tm_min,moment.tm_sec);
						debut_ligne = 0;
					}
				#ifdef DE_ET_POUR_VT100
					if(ligne[l] == 0x1b) { escape = 1; continue; }
					if(isprint(ligne[l]) || (ligne[l] == 0x0a) || (ligne[l] == 0x0d)) {
						if(escape) {
							if(ligne[l] == 'H') printf("\n\r-----------------------------------\n\r");
							else if(ligne[l] == 'J') printf("<Clear>\n\r");
							else if(ligne[l] == '[') { printf("%c%c",(char)0x1b,ligne[l]); }
							else { fputs(ClavierDump(0x1b),stdout); fputs(ClavierTrad(ligne[l]),stdout); }
						} else printf("%c",ligne[l]);
					} else fputs(ClavierTrad(ligne[l]),stdout);
					escape = 0;
				#endif /* DE_ET_POUR_VT100 */
				#ifdef DE_VT100
					if(ligne[l] == 0x1b) { escape = 1; continue; }
					if((ligne[l] == 0x0a) || (ligne[l] == 0x0d)) escape = 0;
					if(escape) {
						if((ligne[l] == 'H') || (ligne[l] == 'J') || (ligne[l] == 'm') || (ligne[l] == 'X')) escape = 0;
						continue;
					}
					fputs(ClavierTrad(ligne[l]),stdout);
				#endif /* DE_VT100 */
				#ifdef BASIQUE
					fputs(ClavierTrad(ligne[l]),stdout);
				#endif /* BASIQUE */
					if(Datation && (ligne[l] == 0x0d)) debut_ligne = 1;
				}
			}
		} while(rc >= 0);
	} while(1);
	ClavierReverouille();
	return(0);
}
