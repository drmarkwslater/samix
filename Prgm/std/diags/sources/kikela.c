#include <environnement.h>

/* Compilation avec:
 cd Prgm/std/applis/sources/utilitaires

 cc kikela.c ~/Prgm/std/librairies/sources/utils/claps.c ~/Prgm/std/librairies/sources/utils/dateheure.c ~/Prgm/std/librairies/sources/utils/decode.c -o ~/bin/kikela -I ~/Prgm/std/envir/ -I ~/Prgm/std/librairies/sources/ -I ~/Prgm/std/librairies/sources/utils -DXCODE
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <defineVAR.h>
#include <claps.h>
#include <decode.h>
#include <dateheure.h>

char SousReseau[20];
int  Depart,Arrivee;
char ParFtp;
char Bavard;
ArgDesc ListeArgs[] = {
	{ DESC_COMMENT "kikela: recherche de machines dans un sous-reeau" },
	{ 0,   DESC_TEXT  SousReseau, "Sous-reseau a examiner (xxx.yyy.zzz)" },
	{ "d", DESC_INT	 &Depart,     "Adresse de depart" },
	{ "a", DESC_INT &Arrivee,     "Adresse finale" },
	{ "f", DESC_BOOL &ParFtp,     "Recherche du nom interne par un acces FTP" },
	{ "v", DESC_BOOL &Bavard,     "Message pour chaque adresse (adresse connectees sinon)" },
	DESC_END
};

#define MAX_LIGNE 256
/* ========================================================================== */
int main(int argc, char *argv[]) {
	char creation,ok;
	int i,nb; char c;
	char adrsIP[20],buffer[80];
	char ligne[MAX_LIGNE],*nom,*r;
	FILE *f;

	strcpy(SousReseau,"192.168.0");
	Depart = 1;
	Arrivee = 255;
	creation = (ArgScan("KikelaArgs",ListeArgs) == 0);
    ArgListChange(argc,argv,ListeArgs);
	if(creation) ArgPrint("KikelaArgs",ListeArgs);
//	ArgPrint("*",ListeArgs);

	nb = 0;
	system("touch kikela.in");
	printf("Debut de recherche a %s\n",DateHeure());
	printf(". Interruptible via <Ctrl-C> ou 'x'\n");
	ClavierDeverouille();
	for(i=Depart; i<=Arrivee; i++) {
		if((c = ClavierTouche())) { if((c == 0x03) || (c == 'x')) break; }
		sprintf(adrsIP,"%s.%d",SousReseau,i);
#ifdef XCODE
		sprintf(buffer,"ping -t 1 -noQq %s >/dev/null",adrsIP);
#else
		sprintf(buffer,"ping -c 2 -t 2 -nq %s >/dev/null",adrsIP);
#endif
		if(system(buffer) == 0) {
			nom = adrsIP;
			nb++;
			ok = 0;
			if(ParFtp) {
				sprintf(buffer,"ftp -v %s <kikela.in >kikela.tmp 2>&1",adrsIP);
				system(buffer);
				f = fopen("kikela.tmp","r");
				if(f) {
					if(LigneSuivante(ligne,MAX_LIGNE,f)) {
						if(LigneSuivante(ligne,MAX_LIGNE,f)) {
							r = ligne + 4;
							nom = ItemSuivant(&r,' ');
						}
					}
					fclose(f);
				} else printf("! Connection FTP en erreur (%s)\n",strerror(errno));
			}
			if(nom == adrsIP) printf("En ligne: %s\n\r",adrsIP);
			else printf("En ligne: %s (%s)\n\r",nom,adrsIP);
		} else if(Bavard) printf("Examine : %s\n\r",adrsIP);
	}
	ClavierReverouille();
	if((c == 0x03) || (c == 'x')) printf(". Recherche arrêtée à l'adresse %s\n",adrsIP);
	printf("%d machine%s connectée%s\n",Accord2s(nb));
	printf("Fin   de recherche a %s\n",DateHeure());
	return(0);
}
