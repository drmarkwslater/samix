#ifndef SALSA_H
#define SALSA_H

/* Commun a SAMBA et TANGO_EDW mais pas dans evtread */

#include <environnement.h>
#include <defineVAR.h>

#ifdef SAMBA_VAR
/* le "nom Apple" (si bien parametre dans chaque sat+maitre) est connu de l'exterieur */
// SAMBA_VAR char  NomApple[HOTE_NOM];
SAMBA_VAR char  AdresseIP[HOTE_NOM];
SAMBA_VAR unsigned int CodeIpLocal;
SAMBA_VAR char  RelecPath[MAXFILENAME];
SAMBA_VAR char  ArchZone[ARCH_TYPEDATA][MAXFILENAME];
SAMBA_VAR char  FichierNtuples[MAXFILENAME],FichierSeuils[MAXFILENAME],FichierStatusBB[MAXFILENAME];
#ifdef MODULES_RESEAU
SAMBA_VAR char  AcqCible[HOTE_NOM];
#endif
#endif /* SAMBA_VAR */

#include <db.h>
int EdwDbPrint(char *dbaz, int bolo, int vm, EdwDbElt elt);
int EdwDbScan(char *dbaz, int date, int bolo, int vm, EdwDbElt elt, char log);

#endif
