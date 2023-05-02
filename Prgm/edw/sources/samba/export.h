#ifndef EXPORT_H
#define EXPORT_H

#ifdef EXPORT_C
#define EXPORT_VAR
#else
#define EXPORT_VAR extern
#endif

#define EXPORT_NOM 32
#define EXPORT_SUPPORT_NOM 80
#define EXPORT_CATEG_MAX 4
#define EXPORT_CATEG_NOM 15
#define EXPORT_VAR_MAX 256
#define EXPORT_VAR_NOM 32

#define EXPORT_QUAND_CLES "periodique/debut_run/fin_run"
typedef enum {
	EXPORT_PERIODIQUE = 0,
	EXPORT_RUN_DEBUT,
	EXPORT_RUN_FIN,
	EXPORT_QUAND_NB
} EXPORT_QUAND;

#define EXPORT_SUPPORT_CLES "fichier/catalogue/log/memoire/udp"
typedef enum {
	EXPORT_FICHIER = 0,
	EXPORT_CATALOGUE,
	EXPORT_LOG,
	EXPORT_MEMOIRE,
	EXPORT_UDP,
	EXPORT_SUPPORTS_NB
} EXPORT_SUPPORTS;

typedef struct {
	char nom[EXPORT_VAR_NOM];
	char type; // ARG_TYPES
	char a_exporter;
	void *adrs;
} TypeExportable;

typedef struct {
	char nom[EXPORT_CATEG_NOM];
	char mesure;
} TypeExportCateg;

typedef struct {
	char nom[EXPORT_NOM];
	char quand;
	char support_type;
	char support_nom[EXPORT_SUPPORT_NOM];
	int  periode;
	char info_nom[MAXFILENAME];
	char var_demandee[EXPORT_VAR_MAX][EXPORT_VAR_NOM];
	ArgDesc *info_desc;
	int  nbcategs;
	TypeExportCateg categ[EXPORT_CATEG_MAX];
	int  nbvars;
	TypeExportable var[EXPORT_VAR_MAX];
	int  dim;
} TypeExportPack;

EXPORT_VAR TypeExportPack ExportPack[EXPORT_MAXPACKS];
EXPORT_VAR int ExportPackNb,ExportPackNum;
EXPORT_VAR char *ExportPackListe[EXPORT_MAXPACKS+1];

/* ..... fonctions ..... */
int  ExportLit();
void ExportEcrit();
int  ExportAjoute(Menu menu, MenuItem item);
int  ExportRetire(Menu menu, MenuItem item);
int  ExportChange(Menu menu, MenuItem item);
int  ExportVariables(Menu menu, MenuItem item);
void ExporteInfos(FILE *g, int k);
void ExportInit();

#endif /* EXPORT_H */
