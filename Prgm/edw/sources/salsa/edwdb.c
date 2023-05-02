#ifdef macintosh
#pragma message(__FILE__)
#endif
#include <environnement.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifndef CODE_WARRIOR_VSN
#include <sys/types.h>  // tout ca ..
#include <sys/uio.h>	// .. pour ..
#include <unistd.h>     // .. read !!
#include <sys/types.h>  // et ca ..
#include <dirent.h>     // pour DIR
#endif

#define EDWDB_C
#include <defineVAR.h>
#include <edwdb.h>
#include <opium_demande.h>
#include <rundata.h>
#include <decode.h>

/* ========================================================================== */
static char EdwDbCheminNouveau(char *chemin, char *dbaz, int bolo, int vm) {
	int rc;

	sprintf(chemin,"%s%s",dbaz,Bolo[bolo].nom);
	if((rc = RepertoireAbsent(chemin))) {
		printf("! La creation du repertoire %s retourne %d (%s/ errno=%d)\n",
			   chemin,rc,strerror(rc),errno);
		return(0);
	}
	if(vm >= 0) {
		strcat(strcat(chemin,RepertoireDelim),ModeleVoie[vm].nom);
		if((rc = RepertoireAbsent(chemin))) {
			printf("! La creation du repertoire %s retourne %d (%s/ errno=%d)\n",
				   chemin,rc,strerror(rc),errno);
			return(0);
		}
	}
	return(1);
}
/* ========================================================================== */
int EdwDbPrint(char *dbaz, int bolo, int vm, EdwDbElt elt) {
	char chemin[MAXFILENAME];

	if(!EdwDbCheminNouveau(chemin,dbaz,bolo,vm)) return(0);
#ifdef ForSamba
	{ char fichier[MAXFILENAME];
		DbPrint(chemin,elt->type,elt->desc,fichier);
		printf("* Element '%s' sauve dans %s\n",elt->type,fichier);
	}
#endif
	return(1);
}
/* ========================================================================== */
static char EdwDbCheminAncien(char *chemin, char *dbaz, int bolo, int vm, char log) {
#ifndef CODE_WARRIOR_VSN
	DIR *repert;
#endif

	sprintf(chemin,"%s%s",dbaz,Bolo[bolo].nom);
#ifndef CODE_WARRIOR_VSN
	repert = opendir(chemin);
	if(repert) closedir(repert);
	else {
		if(log) printf("! Pour le detecteur %s, aucune donnee n'a ete sauvee dans:\n  %s\n",Bolo[bolo].nom,chemin);
		return(0);
	}
#endif
	if(vm >= 0) strcat(strcat(chemin,RepertoireDelim),ModeleVoie[vm].nom);
#ifndef CODE_WARRIOR_VSN
	repert = opendir(chemin);
	if(!repert) {
		if(log) printf("! Pour la voie %s de %s, aucune donnee n'a ete sauvee dans:\n  %s\n",ModeleVoie[vm].nom,Bolo[bolo].nom,chemin);
		return(0);
	}
	closedir(repert);
#endif
	return(1);
}
/* ========================================================================== */
int EdwDbScan(char *dbaz, int date, int bolo, int vm, EdwDbElt elt, char log) {
	char chemin[MAXFILENAME];
	int trouvee;

	if(!EdwDbCheminAncien(chemin,dbaz,bolo,vm,log)) return(0);
#ifdef ForSamba
	{ char fichier[MAXFILENAME];
		trouvee = DbScan(chemin,elt->type,elt->desc,date,fichier);
		if(!trouvee && log) printf("! Pas de '%s' pour la voie %s %s\n",elt->type,ModeleVoie[vm].nom,Bolo[bolo].nom);
	}
#else
	trouvee = 0;
#endif
	return(trouvee);
}
