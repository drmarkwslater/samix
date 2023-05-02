#ifndef DICO_H
#define DICO_H

#include <stdlib.h>
#include <string.h>

typedef enum {
	DICO_NET = 0,
	DICO_FIXE,
	DICO_MAJ
} DICO_MODE;

typedef struct {
	char *accepte;
	char *officiel;
} TypeDicoLexique,*DicoLexique;

#define MAXDICONOM 15
typedef struct {
	char nom[MAXDICONOM];
	char enrichi;
	int nbtermes;
	int maxtermes;
	DicoLexique terme;
} TypeDico,*Dico;

DicoLexique DicoInit(Dico dico, char *nom, int max);
DicoLexique DicoReset(Dico dico, char *nom, int max);
void DicoRaz(Dico dico);
char DicoLit(Dico dico, char *fichier);
void DicoEnrichi(Dico dico, DICO_MODE demande);
void DicoAjouteTerme(DicoLexique terme, char *accepte, char *officiel);
char DicoAjouteLigne(Dico dico, int i, char *accepte, char *officiel);
char DicoInsere(Dico dico, char *accepte, char *officiel); 
void DicoImprime(Dico dico, char *prefixe);
char DicoTraduit(Dico dico, char **texte);
void DicoFormatte(Dico dico, char *texte, char *fmt, ...);
DicoLexique DicoTerme(Dico dico, char *recherche, int *i);
char *DicoLocale(Dico dico, char *accepte);
char **DicoLocaleListeNouvelle(Dico dico, char **liste);
char **DicoLocaleListeChange(Dico dico, char **liste, char **ancienne);
void DicoLocaleListeLibere(Dico dico, char **originale, char **ancienne);

// char **DicoListeLocalise(Dico dico, char **liste);
// char **DicoListeCree(Dico dico, char **liste, char avec_maj);

void DicoCopie(Dico dico2, Dico dico1);
void DicoImprime(Dico dico, char *prefixe);
char DicoSauve(Dico dico, char *fichier);
void DicoVide(Dico dico);

#endif
