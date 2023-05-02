#ifdef macintosh
#pragma message(__FILE__)
#endif
#define EXPORT_C

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <environnement.h>

#include <claps.h>
#include <decode.h>
#include <dateheure.h>
#include <opium_demande.h>
#include <opium.h>

#include <samba.h>
#include <numeriseurs.h>
#include <detecteurs.h>
#include <objets_samba.h>
#include <export.h>

/* ........................ variables particulieres ......................... */
#define EXPORT_MESURE_CLE "trgr_type/trgr_seuil_min/impedance/taux_voie/taux_global"
typedef enum {
	EXPORT_TRGR_TYPE = 0,
	EXPORT_TRGR_SEUIL,
	EXPORT_IMPEDANCE,
	EXPORT_TAUX_VOIE,
	EXPORT_TAUX_GLOBAL,
	EXPORT_MESURES_NB
} EXPORT_MESURES;

TypeExportCateg ExportCategLue;
TypeExportPack ExportPackLu;

/* .......................... variables a exporter .......................... */
//a static short Exportables[EXPORT_VAR_MAX];
// voir export.h !!

/* ................. description du fichier de preferences .................. */
static ArgDesc ExportEditDesc[] = {
	{ "Nom utilisateur", DESC_STR(EXPORT_CATEG_NOM)   ExportCategLue.nom,    0 },
	{ "Definition",      DESC_KEY EXPORT_MESURE_CLE, &ExportCategLue.mesure, 0 },
	DESC_END
};
static ArgStruct ExportEditAS = { (void *)ExportPackLu.categ, (void *)&ExportCategLue, sizeof(TypeExportCateg), ExportEditDesc };

static ArgDesc ExportMesureDesc[] = {
	{ 0, DESC_STR(EXPORT_CATEG_NOM)   ExportCategLue.nom,    0 },
	{ 0, DESC_KEY EXPORT_MESURE_CLE, &ExportCategLue.mesure, 0 },
	DESC_END
};

static ArgStruct ExportCategAS = { (void *)ExportPackLu.categ, (void *)&ExportCategLue, sizeof(TypeExportCateg), ExportMesureDesc };

static ArgDesc ExportPackDesc[] = {
	{ "nom",        DESC_STR(EXPORT_NOM)           ExportPackLu.nom,          "Identification du paquet" },
	{ "timing",     DESC_KEY EXPORT_QUAND_CLES,   &ExportPackLu.quand,        "Moment d'exportation ("EXPORT_QUAND_CLES")" },
	{ "support",    DESC_KEY EXPORT_SUPPORT_CLES, &ExportPackLu.support_type, "Support d'envoi ("EXPORT_SUPPORT_CLES")" },
	{ "chemin",     DESC_STR(EXPORT_SUPPORT_NOM)   ExportPackLu.support_nom,  "Nom du support" },
	{ "periode_ms", DESC_INT                      &ExportPackLu.periode,      "Intervalle entre exportations (ms)" },
	{ "exportable", DESC_STRUCT_SIZE(ExportPackLu.nbcategs,EXPORT_CATEG_MAX) &ExportCategAS, 0 },
	{ "exporte",    DESC_STR_SIZE(EXPORT_VAR_NOM,ExportPackLu.dim,EXPORT_VAR_MAX)   ExportPackLu.var_demandee, 0 },
	DESC_END
};

static ArgStruct ExportPackAS = { (void *)ExportPack, (void *)&ExportPackLu, sizeof(TypeExportPack), ExportPackDesc };
static ArgDesc ExportDesc[] = {
	{ DESC_COMMENT 0 },
	{ DESC_COMMENT "Mesures implementees: "EXPORT_MESURE_CLE },
	{ DESC_COMMENT 0 },
	{ "Exports", DESC_STRUCT_SIZE(ExportPackNb,EXPORT_MAXPACKS) &ExportPackAS, 0 },
	DESC_END
};

/* .......................................................................... */
static void ExportCreeVars(int k) {
	TypeExportPack *pack;
	int i,j,l; int voie; char *nom;

	pack = &(ExportPack[k]);
	pack->nbvars = 0;
	for(j=0; j<pack->nbcategs; j++) {
		switch(pack->categ[j].mesure) {
			case EXPORT_TRGR_TYPE:
				for(voie=0; voie<VoiesNb; voie++) {
					if(BoloNb == 1) nom = VoieManip[voie].prenom; else nom = VoieManip[voie].nom;
					snprintf(pack->var[pack->nbvars].nom,EXPORT_VAR_NOM,"%s[%s]",pack->categ[j].nom,nom);
					pack->var[pack->nbvars].type = ARG_TYPE_TEXTE;
					pack->var[pack->nbvars].adrs = (void *)(TrgrTypeListe[VoieManip[voie].def.trgr.type]);
					if(++pack->nbvars >= EXPORT_VAR_MAX) break;
				}
				break;
			case EXPORT_TRGR_SEUIL:
				for(voie=0; voie<VoiesNb; voie++) {
					if(BoloNb == 1) nom = VoieManip[voie].prenom; else nom = VoieManip[voie].nom;
					snprintf(pack->var[pack->nbvars].nom,EXPORT_VAR_NOM,"%s[%s]",pack->categ[j].nom,nom);
					pack->var[pack->nbvars].type = ARG_TYPE_FLOAT;
					pack->var[pack->nbvars].adrs = (void *)(&(VoieManip[voie].def.trgr.minampl));
					if(++pack->nbvars >= EXPORT_VAR_MAX) break;
				}
				break;
			case EXPORT_IMPEDANCE:
				for(voie=0; voie<VoiesNb; voie++) if(VoieManip[voie].modulee) {
					if(BoloNb == 1) nom = VoieManip[voie].prenom; else nom = VoieManip[voie].nom;
					snprintf(pack->var[pack->nbvars].nom,EXPORT_VAR_NOM,"%s[%s]",pack->categ[j].nom,nom);
					pack->var[pack->nbvars].type = ARG_TYPE_FLOAT;
					pack->var[pack->nbvars].adrs = (void *)(&(VoieInfo[voie].R));
					if(++pack->nbvars >= EXPORT_VAR_MAX) break;
				}
				break;
			case EXPORT_TAUX_VOIE:
				for(voie=0; voie<VoiesNb; voie++) {
					if(BoloNb == 1) nom = VoieManip[voie].prenom; else nom = VoieManip[voie].nom;
					snprintf(pack->var[pack->nbvars].nom,EXPORT_VAR_NOM,"%s[%s]",pack->categ[j].nom,nom);
					pack->var[pack->nbvars].type = ARG_TYPE_FLOAT;
					pack->var[pack->nbvars].adrs = (void *)(&(VoieTampon[voie].taux));
					if(++pack->nbvars >= EXPORT_VAR_MAX) break;
				}
				break;
			case EXPORT_TAUX_GLOBAL:
				snprintf(pack->var[pack->nbvars].nom,EXPORT_VAR_NOM,"%s",pack->categ[j].nom);
				pack->var[pack->nbvars].type = ARG_TYPE_FLOAT;
				pack->var[pack->nbvars].adrs = (void *)(&(TousLesEvts.freq));
				++pack->nbvars;
				break;
		}
		if(pack->nbvars >= EXPORT_VAR_MAX) break;
	}
	for(j=0; j<pack->nbvars; j++) {
		l = 0; while(pack->var[j].nom[++l]) if(pack->var[j].nom[l] == ' ') pack->var[j].nom[l] = '-';
		for(i=0; i<pack->dim; i++) if(!strcmp(&(pack->var_demandee[i][0]),pack->var[j].nom)) break;
		pack->var[j].a_exporter = (i < pack->dim);
	}
}
/* .......................................................................... */
static void ExportCreeDesc(int k, char modifie) {
	TypeExportPack *pack;
	ArgDesc *elt; int i,n;

	pack = &(ExportPack[k]);
	//a n = 0; for(i=0; i<pack->nbvars; i++) if(pack->var[i].a_exporter) Exportables[n++] = i; pack->dim = n;
	if(pack->info_desc) { free(pack->info_desc); pack->info_desc = 0; }
	n = 0; for(i=0; i<pack->nbvars; i++) if(pack->var[i].a_exporter) n++;
	if(n) {
		pack->info_desc = ArgCreate(n);
		elt = pack->info_desc;
		for(i=0; i<pack->nbvars; i++) if(pack->var[i].a_exporter)
			ArgAdd(elt++,pack->var[i].nom,0,pack->var[i].type,0,0,1,0,0,(void *)(pack->var[i].adrs),0);
	}
	if(modifie) { if(OpiumCheck("Sauver ces choix")) ExportEcrit(); }
}
/* ========================================================================== */
int ExportLit() {
	int k;

	if(!ArgScan(FichierPrefExports,ExportDesc)) { ExportPackNb = 0; return(0); }
	for(k=0; k<ExportPackNb; k++) {
		if((ExportPack[k].support_type == EXPORT_FICHIER) || (ExportPack[k].support_type == EXPORT_LOG))
			SambaAjoutePrefPath(ExportPack[k].info_nom,ExportPack[k].support_nom);
		else if(ExportPack[k].support_type == EXPORT_CATALOGUE) strcpy(ExportPack[k].info_nom,FichierCatalogue);
		else strcpy(ExportPack[k].info_nom,ExportPack[k].support_nom);
		ExportCreeVars(k);
		ExportCreeDesc(k,0);
	}

	return(1);
}
/* ========================================================================== */
void ExportEcrit() {
	int i,j,k;

	for(k=0; k<ExportPackNb; k++) {
		i = 0; for(j=0; j<ExportPack[k].nbvars; j++) if(ExportPack[k].var[j].a_exporter) strcpy(&(ExportPack[k].var_demandee[i++][0]),ExportPack[k].var[j].nom);
		ExportPack[k].dim = i;
	}
	ArgPrint(FichierPrefExports,ExportDesc);
	printf("* Definition des exportations sauvees dans %s\n",FichierPrefExports);
}
/* ========================================================================== */
static void ExportDefaut() {
	TypeExportPack *pack; int j;

	pack = &(ExportPack[ExportPackNum]);
	strcpy(pack->nom,"Exportations");
	pack->quand = EXPORT_PERIODIQUE;
	pack->support_type = EXPORT_FICHIER;
	strcpy(pack->support_nom,"FichierExport");
	pack->periode = 1000;
	j = 0;
	strcpy(pack->categ[j].nom,"taux"); pack->categ[j].mesure = EXPORT_TAUX_VOIE; j++;
	pack->nbcategs = j;
}
/* ========================================================================== */
static void ExportComplete(char sauve) {
	TypeExportPack *pack; Panel p; int i,rc,ligs,cols;

	pack = &(ExportPack[ExportPackNum]);
	if(!pack->nbvars) { OpiumWarn("Pas d'information a exporter. Modifier le pack."); return; }
	ligs = OpiumServerHeigth(0) / WndLigToPix(1);
	cols = ((pack->nbvars - 1) / ligs) + 1;
	p = PanelCreate(pack->nbvars); PanelColumns(p,cols);
	for(i=0; i<pack->nbvars; i++) PanelItemColors(p,PanelBool(p,pack->var[i].nom,&(pack->var[i].a_exporter)),SambaRougeVert,2);
	rc = OpiumExec(p->cdr);
	if(sauve || (rc == PNL_OK)) ExportCreeDesc(ExportPackNum,1);
	PanelDelete(p);
}
/* ========================================================================== */
static void ExportModifie() {
	TypeExportPack *pack; Panel p; // static OpiumTableDeclaration tExportCategs,iExportCategs;

	pack = &(ExportPack[ExportPackNum]);
	p = PanelCreate(5);
	PanelText(p,"Identification",pack->nom,EXPORT_NOM);
	PanelKeyB(p,"Timing",EXPORT_QUAND_CLES,&pack->quand,12);
	PanelKeyB(p,"Type de support",EXPORT_SUPPORT_CLES,&pack->support_type,10);
	PanelText(p,"Nom du support",pack->support_nom,EXPORT_SUPPORT_NOM); PanelItemLngr(p,2,16);
	PanelInt (p,"Periode (ms)",&pack->periode);
	//	OpiumTableCreate(&tExportCategs); //  met juste tExportCategs a 0
	//	iExportCategs = OpiumTableAdd(&tExportCategs,"Variables",ARG_TYPE_STRUCT,(void *)&ExportCategAS,&pack->nbcategs,0);

	if(OpiumExec(p->cdr) == PNL_OK) {
		memcpy(&ExportPackLu,pack,sizeof(TypeExportPack));
		if(OpiumTableExec("Variables",ARG_TYPE_STRUCT,(void *)(&ExportEditAS),&ExportPackLu.nbcategs,1,0) == PNL_OK) {
			memcpy(pack,&ExportPackLu,sizeof(TypeExportPack));
			if(ExportPackNum >= ExportPackNb) ExportPackNb = ExportPackNum + 1;
			ExportCreeVars(ExportPackNum);
			ExportCreeDesc(ExportPackNum,0);
			ExportComplete(1);

		}
	}
	PanelDelete(p);
}
/* ========================================================================== */
int ExportAjoute(Menu menu, MenuItem item) {
	ExportPackNum = ExportPackNb; ExportDefaut();
	ExportModifie();
	return(0);
}
/* ========================================================================== */
int ExportRetire(Menu menu, MenuItem item) {
	int k;

	if(!ExportPackNb) { OpiumWarn("Pas de pack a retirer"); return(0); }
	else if(ExportPackNb > 1) {
		if(OpiumReadList("Pack a retirer",ExportPackListe,&ExportPackNum,EXPORT_NOM) == PNL_CANCEL) return(0);
	} else ExportPackNum = 0;
	--ExportPackNb;
	for(k=ExportPackNum; k<ExportPackNb; k++) memcpy(&(ExportPack[k]),&(ExportPack[k+1]),sizeof(TypeExportPack));
	if(OpiumCheck("Sauver ce choix")) ExportEcrit();

	return(0);
}
/* ========================================================================== */
int ExportChange(Menu menu, MenuItem item) {
	if(!ExportPackNb) { ExportPackNum = 0; ExportDefaut(); }
	else if(ExportPackNb > 1) {
		if(OpiumReadList("Pack a modifier",ExportPackListe,&ExportPackNum,EXPORT_NOM) == PNL_CANCEL) return(0);
	} else ExportPackNum = 0;
	ExportModifie();
	return(0);
}
/* ========================================================================== */
int ExportVariables(Menu menu, MenuItem item) {
	if(!ExportPackNb) { OpiumWarn("Pas de pack a exporter. Creer un pack."); return(0); }
	else if(ExportPackNb > 1) {
		if(OpiumReadList("Pack a exporter",ExportPackListe,&ExportPackNum,EXPORT_NOM) == PNL_CANCEL) return(0);
	} else ExportPackNum = 0;
	ExportComplete(0);
	return(0);
}
#define EXPORT_INFO 256
/* ========================================================================== */
void ExporteInfos(FILE *g, int k) {
	FILE *f; struct timeval temps; struct tm date;
	int i; char suite;

	switch(ExportPack[k].support_type) {
		case EXPORT_FICHIER:
			if((f = fopen(ExportPack[k].info_nom,"w"))) {
				gettimeofday(&temps,0); memcpy(&date,localtime(&(temps.tv_sec)),sizeof(struct tm));
				ArgBegin(f,0,ExportPack[k].info_desc);
				fprintf(f,"Date = %02d.%02d.%02d/%02d:%02d:%02d.%06d\n",
					date.tm_year % 100,date.tm_mon+1,date.tm_mday,
					date.tm_hour,date.tm_min,date.tm_sec,temps.tv_usec);
				fclose(f);
			}
			break;
		case EXPORT_LOG:
			if((f = fopen(ExportPack[k].info_nom,"a"))) {
				gettimeofday(&temps,0); memcpy(&date,localtime(&(temps.tv_sec)),sizeof(struct tm));
				fprintf(f,"%02d.%02d.%02d %02d:%02d:%02d.%06d/ ",
					date.tm_year % 100,date.tm_mon+1,date.tm_mday,
					date.tm_hour,date.tm_min,date.tm_sec,temps.tv_usec);
			}
		case EXPORT_CATALOGUE:
			if(ExportPack[k].support_type == EXPORT_CATALOGUE) f = g;
			if(!f) break;
			suite = 0;
			for(i=0; i<ExportPack[k].nbvars; i++) if(ExportPack[k].var[i].a_exporter) {
				if(suite) fprintf(f,", "); else suite = 1;
				switch(ExportPack[k].var[i].type) {
					case ARG_TYPE_TEXTE: fprintf(f,"%s=%s",ExportPack[k].var[i].nom,(char *)ExportPack[k].var[i].adrs); break;
					case ARG_TYPE_INT:   fprintf(f,"%s=%d",ExportPack[k].var[i].nom,*(int *)ExportPack[k].var[i].adrs); break;
					case ARG_TYPE_FLOAT: fprintf(f,"%s=%g",ExportPack[k].var[i].nom,*(float *)ExportPack[k].var[i].adrs); break;
				}
			}
			if(ExportPack[k].support_type != EXPORT_CATALOGUE) { fprintf(f,"\n"); fclose(f); }
			break;
	}
}
/* ========================================================================== */
void ExportInit() {
	int j,k;

	for(k=0; k<EXPORT_MAXPACKS; k++) ExportPackListe[k] = ExportPack[k].nom; ExportPackListe[k] = "\0";

	k = 0; j = 0;
#ifdef EXPORT_EXEMPLES
	strcpy(ExportPack[k].nom,"regul_tempe");
	ExportPack[k].quand = EXPORT_PERIODIQUE;
	ExportPack[k].support_type = EXPORT_FICHIER;
	strcpy(ExportPack[k].support_nom,"DernierExport");
	ExportPack[k].periode = 1000;
	j = 0;
	strcpy(ExportPack[k].categ[j].nom,"R"); ExportPack[k].categ[j].mesure = EXPORT_IMPEDANCE; j++;
	strcpy(ExportPack[k].categ[j].nom,"taux"); ExportPack[k].categ[j].mesure = EXPORT_TAUX_VOIE; j++;
	k++;
	strcpy(ExportPack[k].nom,"catalogue");
	ExportPack[k].quand = EXPORT_RUN_FIN;
	ExportPack[k].support_type = EXPORT_CATALOGUE;
	strcpy(ExportPack[k].support_nom,"sans-objet");
	ExportPack[k].periode = 0;
	j = 0;
	strcpy(ExportPack[k].categ[j].nom,"trgr"); ExportPack[k].categ[j].mesure = EXPORT_TRGR_TYPE; j++;
	strcpy(ExportPack[k].categ[j].nom,"seuil"); ExportPack[k].categ[j].mesure = EXPORT_TRGR_SEUIL; j++;
	k++;
#endif /* EXPORT_EXEMPLES */

	ExportPackNb = k;
	ExportPack[ExportPackNb].nom[0] = '\0';
	for(k=0; k<EXPORT_MAXPACKS; k++) {
		ExportPack[k].info_desc = 0;
		ExportPack[k].nbcategs = j;
		ExportPack[k].nbvars = 0;
		ExportPack[k].dim = 0;
	}
	ExportPackNum = 0;

}
