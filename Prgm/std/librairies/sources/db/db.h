#ifndef DB_H
#define DB_H

#ifdef DB_C
#define DB_VAR
#else
#define DB_VAR extern
#endif

#include "claps.h"

#define DB_LNGRTEXTE 256
DB_VAR char DbErreur[DB_LNGRTEXTE];
DB_VAR char DbRaison[DB_LNGRTEXTE];
DB_VAR char DbId[DB_LNGRTEXTE];
DB_VAR char DbRev[DB_LNGRTEXTE];
DB_VAR char DbStatus;

DB_VAR ArgDesc DbReponseTextes[]
#ifdef DB_C
= {
	{ "error",	DESC_STR(DB_LNGRTEXTE)   DbErreur, 0 },
	{ "reason",	DESC_STR(DB_LNGRTEXTE)   DbRaison, 0 },
	{ "id",	    DESC_STR(DB_LNGRTEXTE)   DbId,     0 },
	{ "rev",	DESC_STR(DB_LNGRTEXTE)   DbRev,    0 },
	{ "ok",	    DESC_KEY "false/true",  &DbStatus, 0 },
	DESC_END
}
#endif
;
DB_VAR ArgStruct DbAsRep
#ifdef DB_C
	= { 0, 0, 0, DbReponseTextes }
#endif
;
DB_VAR ArgDesc DbReponse[]
#ifdef DB_C
 = {
//	{ 0,	DESC_STRUCT_DIM(1) &DbAsRep, 0 },
	{ "error",	DESC_STR(DB_LNGRTEXTE)   DbErreur, 0 },
	{ "reason",	DESC_STR(DB_LNGRTEXTE)   DbRaison, 0 },
	{ "id",	    DESC_STR(DB_LNGRTEXTE)   DbId,     0 },
	{ "rev",	DESC_STR(DB_LNGRTEXTE)   DbRev,    0 },
	{ "ok",	    DESC_KEY "false/true",  &DbStatus, 0 },
	DESC_END
}
#endif
;

int DbSendDesc(char *serveur, char *id, ArgDesc *desc);
int DbSendFile(char *serveur, char *id, FILE *f);
int DbSendFichier(char *serveur, char *id, char *donnees);
void DbRazLastId(void);
char DbGetLastId(char *id, char *rev);
int DbRemove(char *serveur, char *id, char *rev);
char DbPrint(char *chemin, char *nom, ArgDesc *desc, char *fichier);
int DbScan(char *chemin, char *nom, ArgDesc *desc, int date, char *fichier);

#endif

