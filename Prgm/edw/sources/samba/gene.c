//  Created by Michel Gros on 15.04.21.

#define GENE_C

#include <samba.h>
#include <gene.h>
#include <repartiteurs.h>
#include <detecteurs.h>


static TypeGene *GeneOuvert;

int GegeneGere(Menu menu, MenuItem item);
int GegeneAuto(Menu menu, MenuItem item);
static TypeMenuItem iGeneGere[] = {
	{ "Gerer",     MNU_FONCTION GegeneGere  },
	{ "Attribuer", MNU_FONCTION GegeneOpen  },
	{ "Autostart", MNU_FONCTION GegeneAuto  },
	{ "Sauver",    MNU_FONCTION GegeneEcrit },
	{ "Fermer",    MNU_RETOUR },
	MNU_END
};

static void GeneRaz(int gene);

/* ========================================================================== */
static void GeneInit(int gene) {
	// Gene[0].nom[0] = '\0';
	sprintf(Gene[gene].nom,"Gene#%d",gene+1);
	Gene[gene].voie[0] = 0;
	Gene[gene].nbvoies = 1;
	Gene[gene].ldb = 0.0;
	Gene[gene].bruit = 5.0;
	Gene[gene].freq = 1.0;
	Gene[gene].pic = 1000.0;
	Gene[gene].reso = 10.0;
	Gene[gene].interv = GENE_INTERV_POIS;
	Gene[gene].evts = 0;
	Gene[gene].start = 0;
	GeneRaz(gene);
}
/* ========================================================================== */
static void GeneRaz(int gene) {
	Gene[gene].num = gene;
	Gene[gene].periode = 1.4 / EnSecondes / Gene[gene].freq; // temporaire, Echantillonnage peut encore changer
	Gene[gene].mode_actif = Gene[gene].mode_manu = Gene[gene].mode_auto = 0;
	Gene[gene].panneau.avant = 0;
	Gene[gene].panneau.bouton.manu = Gene[gene].panneau.bouton.svg = 0;
	Gene[gene].panneau.glissiere.ldb = Gene[gene].panneau.glissiere.brt = 0;
	Gene[gene].panneau.glissiere.frq = Gene[gene].panneau.glissiere.amp = Gene[gene].panneau.glissiere.res = 0;
	Gene[gene].panneau.texte.msg = Gene[gene].panneau.texte.ldb = Gene[gene].panneau.texte.brt = 0;
	Gene[gene].panneau.texte.frq = Gene[gene].panneau.texte.amp = Gene[gene].panneau.texte.res = 0;
	Gene[gene].panneau.evts = 0;
	Gene[gene].panneau.message[0] = '\0';
}
/* ========================================================================== */
int GegeneLit() {
	int nb_erreurs; FILE *f;
	int j,gene,voie;

	for(gene=0; gene<GENE_MAX; gene++) GeneListe[gene] = Gene[gene].nom; GeneListe[gene] = "\0";
	if((f = fopen(FichierPrefGene,"r"))) {
		SambaLogDef("= Lecture des generateurs","dans",FichierPrefGene);
		nb_erreurs = ArgFromFile(f,GeneDesc,0);
		fclose(f);
		// ArgPrint("*",GeneDesc);
	} else { /* Fichier des generateurs inaccessible */
		nb_erreurs = 0;
		printf("* Pas de generateur defini via '%s' (%s)\n",FichierPrefGene,strerror(errno));
		// simulation pas obligatoire: nb_erreurs++;
		GeneInit(0); GeneNb = 1; GegeneEcrit(0,0);
	}
	mGeneGere = MenuLocalise(iGeneGere);
	GeneNum = 0;
	GeneModifies = 0;
	GeneOuvert = 0;
	for(gene=0; gene<GeneNb; gene++) {
		if(Gene[gene].nom[0] == '\0') {
			if(Gene[gene].nbvoies && (voie = Gene[gene].voie[0]) >= 0) strncpy(Gene[gene].nom,VoieManip[voie].nom,MODL_NOM);
			else sprintf(Gene[gene].nom,"Gene#%d",gene+1);
		}
		GeneRaz(gene);
	}
	Gene[gene].nom[0] = '\0';
	for(gene=0; gene<GeneNb; gene++) {
		printf("  | Generateur '%s': %d voie%s (",Gene[gene].nom,Accord1s(Gene[gene].nbvoies));
		if(Gene[gene].nbvoies > 5) printf("\n  | . ");
		for(j=0; j<Gene[gene].nbvoies; ) {
			printf("%s",VoieManip[Gene[gene].voie[j++]].nom);
			if(j < Gene[gene].nbvoies) printf(",%s",(j%5)?" ":"\n  | . ");
		}
		printf("), demarrage %s.\n",(Gene[gene].start)? "automatique":"a la demande");
	}
	return(nb_erreurs);
}
/* ========================================================================== */
int GegeneEcrit(Menu menu, MenuItem item) {
	FILE *f;

	RepertoireCreeRacine(FichierPrefGene);
	if((f = fopen(FichierPrefGene,"w"))) {
		printf("\n");
		SambaLogDef("= Sauvegarde des generateurs","dans",FichierPrefGene);
		// fprintf(f,"# Generateurs\n");
		fprintf(f,"#\n");
		ArgAppend(f,0,GeneDesc);
		fclose(f);
		GeneModifies = 0;
	} else printf("! Fichier des generateurs inaccessible: '%s' (%s)\n",FichierPrefGene,strerror(errno));
	return((f != 0));
}
/* ========================================================================== */
void GegeneConnecte(int gene) {
	int voie,j; char **boutons; char *choix[] = {
		" Garde le precedent, ignore celui-ci ",
		" Garde celui-ci, ignore le precedent ",
		"\0"
	};

	for(voie=0; voie<VoiesNb; voie++) VoieManip[voie].gene = -1;
	for(gene=0; gene<GeneNb; gene++) if(Gene[gene].mode_actif) {
		for(j=0; j<Gene[gene].nbvoies; j++) if((voie = Gene[gene].voie[j]) >= 0) {
			if(VoieManip[voie].gene < 0) VoieManip[voie].gene = gene;
			else {
				boutons = LL_(choix); if(OpiumChoix(2,boutons,
					L_("La voie %s est deja branchee sur le generateur %s. Que fais-je?"),VoieManip[voie].nom,Gene[VoieManip[voie].gene].nom))
				VoieManip[voie].gene = gene;
			}
		}
	}
}
/* ========================================================================== */
static void GegeneChangeFait(int gene) {
	GeneModifies = 1;
	if(OpiumDisplayed(Gene[gene].panneau.bouton.svg)) {
		MenuItemAllume((Menu)(Gene[gene].panneau.bouton.svg->adrs),1,0,GRF_RGB_YELLOW);
	}
}
/* ========================================================================== */
static int GegeneChangeInstrumLdb(Instrum instrum, int gene) {
	PanelRefreshVars(Gene[gene].panneau.texte.ldb);
	GegeneChangeFait(gene);
	return(0);
}
/* ========================================================================== */
static int GegeneChangePanelLdb(Panel panel, int item, void *arg) {
	int gene;

	gene = (int)(IntAdrs)arg;
	if(Gene[gene].ldb > 32000.0) Gene[gene].ldb = 32000.0;
	else if(Gene[gene].ldb < -32000.0) Gene[gene].ldb = -32000.0;
	PanelRefreshVars(Gene[gene].panneau.texte.ldb);
	InstrumRefreshVar(Gene[gene].panneau.glissiere.ldb);
	GegeneChangeFait(gene);
	return(0);
}
/* ========================================================================== */
static int GegeneChangeInstrumBrt(Instrum instrum, int gene) {
	PanelRefreshVars(Gene[gene].panneau.texte.brt);
	GegeneChangeFait(gene);
	return(0);
}
/* ========================================================================== */
static int GegeneChangePanelBrt(Panel panel, int item, void *arg) {
	int gene;

	gene = (int)(IntAdrs)arg;
	if(Gene[gene].bruit > 500.0) Gene[gene].bruit = 500.0;
	else if(Gene[gene].bruit < 0.0) Gene[gene].bruit = 0.0;
	PanelRefreshVars(Gene[gene].panneau.texte.brt);
	InstrumRefreshVar(Gene[gene].panneau.glissiere.brt);
	GegeneChangeFait(gene);
	return(0);
}
/* ========================================================================== */
static int GegeneChangeInstrumFrq(Instrum instrum, int gene) {

	if(Gene[gene].freq > 1000.0) Gene[gene].freq = 1000.0;
	else if(Gene[gene].freq < 0.01) Gene[gene].freq = 0.01;
	Gene[gene].periode = 1.4 * Echantillonnage / (Gene[gene].freq / 1000.0);
	PanelRefreshVars(Gene[gene].panneau.texte.frq);
	GegeneChangeFait(gene);
	return(0);
}
/* ========================================================================== */
static int GegeneChangePanelFrq(Panel panel, int item, void *arg) {
	int gene;

	gene = (int)(IntAdrs)arg;
	if(Gene[gene].freq > 1000.0) Gene[gene].freq = 1000.0;
	else if(Gene[gene].freq < 0.01) Gene[gene].freq = 0.01;
	Gene[gene].periode = 1.4 * Echantillonnage / (Gene[gene].freq / 1000.0);
	PanelRefreshVars(Gene[gene].panneau.texte.frq);
	InstrumRefreshVar(Gene[gene].panneau.glissiere.frq);
	GegeneChangeFait(gene);
	return(0);
}
/* ========================================================================== */
static int GegeneChangeInstrumAmp(Instrum instrum, int gene) {
	PanelRefreshVars(Gene[gene].panneau.texte.amp);
	GegeneChangeFait(gene);
	return(0);
}
/* ========================================================================== */
static int GegeneChangePanelAmp(Panel panel, int item, void *arg) {
	int gene;

	gene = (int)(IntAdrs)arg;
	if(Gene[gene].pic > 32000.0) Gene[gene].pic = 32000.0;
	else if(Gene[gene].pic < 1.0) Gene[gene].pic = 1.0;
	PanelRefreshVars(Gene[gene].panneau.texte.amp);
	InstrumRefreshVar(Gene[gene].panneau.glissiere.amp);
	GegeneChangeFait(gene);
	return(0);
}
/* ========================================================================== */
static int GegeneChangeInstrumRes(Instrum instrum, int gene) {
	PanelRefreshVars(Gene[gene].panneau.texte.res);
	GegeneChangeFait(gene);
	return(0);
}
/* ========================================================================== */
static int GegeneChangePanelRes(Panel panel, int item, void *arg) {
	int gene;

	gene = (int)(IntAdrs)arg;
	if(Gene[gene].reso > 100.0) Gene[gene].reso = 50.0;
	else if(Gene[gene].reso < 0.0) Gene[gene].reso = 0.0;
	PanelRefreshVars(Gene[gene].panneau.texte.res);
	InstrumRefreshVar(Gene[gene].panneau.glissiere.res);
	GegeneChangeFait(gene);
	return(0);
}
/* ========================================================================== */
int GegeneAutorise(Instrum instrum, void *arg) {
	int gene,voie;

	for(gene=0; gene<GeneNb; gene++) {
		if(SettingsGenePossible) strcpy(Gene[gene].panneau.message,L_("Generateur reconnecte"));
		else strcpy(Gene[gene].panneau.message,L_("Generateur deconnecte (voir Controles)"));
		if(OpiumAlEcran(Gene[gene].panneau.texte.msg)) PanelRefreshVars(Gene[gene].panneau.texte.msg);
		/* for(j=0; j<Gene[gene].nbvoies; j++) if((voie = Gene[gene].voie[j]) >= 0) {
			int rang,rep,j;
			if((rang = VoieTampon[voie].rang_sample) >= 0) {
				if((rep = SambaEchantillon[rang].rep) >= 0) {
					Repart[rep].avec_gene = (SettingsGenePossible && Gene[gene].mode_actif);
				}
			}
		} */
	}
	for(voie=0; voie<VoiesNb; voie++) if(VoieInfo[voie].planche) ReglageReplaceEcran(0,voie);

	return(0);
}
/* ========================================================================== */
static int GegeneSauve(Menu menu, MenuItem item) {
	int gene;

	GegeneEcrit(menu,item);
	for(gene=0; gene<GeneNb; gene++) if(OpiumDisplayed(Gene[gene].panneau.bouton.svg)) {
		MenuItemEteint((Menu)(Gene[gene].panneau.bouton.svg->adrs),1,0);
	}
	GeneModifies = 0;
	return(0);
}
/* ========================================================================== */
static int GenereOnOff(Menu menu, MenuItem item) {
	Cadre board; int gene;

	board = (menu->cdr)->planche;
	for(gene=0; gene<GeneNb; gene++) if(Gene[gene].panneau.avant == board) break;
	if(gene >= GeneNb) {
		OpiumFail(L_("ERREUR SYSTEME! On a perdu le generateur en operation"));
		return(0);
	}
	Gene[gene].mode_actif = 1 - Gene[gene].mode_actif;
	if(Gene[gene].mode_actif) MenuItemAllume(menu,1," On  ",GRF_RGB_YELLOW);
	else MenuItemAllume(menu,1," Off ",GRF_RGB_RED);
	GegeneConnecte(gene);
	return(0);
}
/* ========================================================================== */
int GegeneGere(Menu menu, MenuItem item) {
	Panel p; int cols,nbitems,nbavant; int gene,j; char en_cours;
	char action[GENE_MAX];

	cols = 2; p = 0; nbavant = -1;
	do {
		nbitems = cols * (GeneNb + 1);
		if(nbitems != nbavant) {
			if(p) PanelDelete(p);
			p = PanelCreate(nbitems); PanelColumns(p,cols); PanelTitle(p,"Gestion du pool de generateurs");
		} else PanelErase(p);
		PanelRemark(p,"action");
		for(gene=0; gene<GeneNb; gene++) {
			action[gene] = EDITLIST_GARDE;
			PanelItemColors(p,PanelKeyB(p,0,L_(EDITLIST_ACTION_CLES),&(action[gene]),8),SambaRougeVertJauneOrange,3);
		}
		PanelRemark(p,L_("nom du generateur"));
		for(gene=0; gene<GeneNb; gene++) PanelText(p,0,Gene[gene].nom,MODL_NOM);
		en_cours = 0;
		if(OpiumExec(p->cdr) != PNL_CANCEL) {
			for(gene=0; gene<GeneNb; gene++) {
				switch(action[gene]) {
				  case EDITLIST_INSERE:  /* inserer gene */
					if(GeneNb < GENE_MAX) {
						for(j = GeneNb; j>gene; --j) {
							memcpy(&(Gene[j]),&(Gene[j-1]),sizeof(TypeGene));
							if(Gene[j].panneau.avant) {
								OpiumClear(Gene[j].panneau.avant);
								BoardTrash(Gene[j].panneau.avant);
							}
							Gene[j].num = j;
							if(j > (gene + 1)) action[j] = action[j-1]; else action[j] = 0;
						}
						// GeneInit(GeneNum); initialise via memcpy
						sprintf(Gene[gene].nom,"Gene#%d",gene+1);
						GeneRaz(gene); GeneNb++;
						gene++; GeneNum = gene;
						Gene[GeneNb].nom[0] = '\0';
						GeneModifies = 1;
					} else {
						OpiumError(L_("Pas plus de %d generateurs"),GENE_MAX);
						gene = GeneNb;
					}
					en_cours = 1;
					break;
				  case EDITLIST_EFFACE:  /* retirer gene */
					if(Gene[gene].panneau.avant) {
						//? if(OpiumDisplayed(Gene[gene].panneau.avant))
						OpiumClear(Gene[gene].panneau.avant);
						BoardTrash(Gene[gene].panneau.avant);
						BoardDelete(Gene[gene].panneau.avant);
					}
					--GeneNb;
					for(j=gene; j<GeneNb; j++) {
						memcpy(&(Gene[j]),&(Gene[j+1]),sizeof(TypeGene));
						if(Gene[j].panneau.avant) {
							OpiumClear(Gene[j].panneau.avant);
							BoardTrash(Gene[j].panneau.avant);
						}
						Gene[j].num = j;
						action[j] = action[j+1];
					}
					if(GeneNum >= gene) { if(--GeneNum < 0) GeneNum = 0; }
					--gene;
					Gene[GeneNb].nom[0] = '\0';
					GeneModifies = 1;
					en_cours = 1;
					break;
				}
			}
		}
	} while(en_cours);
	PanelDelete(p);

	return(0);
}
/* ========================================================================== */
int GegeneAuto(Menu menu, MenuItem item) {
	Panel p; int gene;

	p = PanelCreate(GeneNb);
	for(gene=0; gene < GeneNb; gene++) {
		PanelKeyB(p,Gene[gene].nom,"a la demande/autostart",&(Gene[gene].start),13);
	}
	OpiumExec(p->cdr);
	PanelDelete(p);

	return(0);
}
/* ========================================================================== */
int GegeneOpen(Menu menu, MenuItem item) {
	if(!GeneNb) OpiumWarn(L_("Pas de generateur defini"));
	else {
		if(GeneNb > 1) {
			OpiumReadTitle(L_("Mise en service"));
			if(OpiumReadList(L_("Nom du generateur"),GeneListe,&GeneNum,23) == PNL_CANCEL) return(0);
		} else GeneNum = 0;
		GegeneMontre(GeneNum);
	}
	return(0);
}
/* ========================================================================== */
int GegeneSortieAjoute(Menu menu, MenuItem item) {
	Cadre cdr,panneau; int gene,voie,sortie;

	cdr = menu->cdr; panneau = cdr->planche;
	for(gene=0; gene<GeneNb; gene++) if(Gene[gene].panneau.avant == panneau) {
		GeneOuvert = &(Gene[gene]);
		if(GeneOuvert->nbvoies >= VOIES_MAX) {
			OpiumWarn(L_("Tous les branchements (%d) sont occupes"),VOIES_MAX);
			return(0);
		}
		for(voie=0; voie<VoiesNb; voie++) {
			sortie = 0;
			for( ; sortie<GeneOuvert->nbvoies; sortie++) if(GeneOuvert->voie[sortie] == voie) break;
			if(sortie >= GeneOuvert->nbvoies) break;
		}
		if(voie >= VoiesNb) {
			OpiumWarn(L_("Toutes les voies sont deja utilisees"));
			return(0);
		}
		OpiumReadTitle(L_("Ajout d'une voie"));
		if(OpiumReadList(L_("Voie a ajouter"),VoieNom,&voie,32) == PNL_OK) {
			sortie = GeneOuvert->nbvoies;
			GeneOuvert->voie[sortie] = voie;
			GeneOuvert->nbvoies += 1;
			GegeneChangeFait(gene);
			GegeneMontre(GeneOuvert->num);
		}
		break;
	}
	return(0);
}
/* .......................................................................... */
int GegeneSortieRetire(Menu menu, MenuItem item) {
	Cadre cdr,panneau; int gene,voie,sortie;

	cdr = menu->cdr; panneau = cdr->planche;
	for(gene=0; gene<GeneNb; gene++) if(Gene[gene].panneau.avant == panneau) {
		GeneOuvert = &(Gene[gene]);
		if(GeneOuvert->nbvoies == 0) {
			OpiumWarn(L_("Tous les branchements sont vides"),VOIES_MAX);
			return(0);
		}
		for(voie=0; voie<VoiesNb; voie++) {
			for(sortie=0; sortie<GeneOuvert->nbvoies; sortie++) if(GeneOuvert->voie[sortie] == voie) break;
			if(sortie < GeneOuvert->nbvoies) break;
		}
		if(voie >= VoiesNb) {
			OpiumWarn(L_("Voie branchee inconnue!!!"));
			return(0);
		}
		OpiumReadTitle(L_("Suppression d'une voie"));
		if(OpiumReadList(L_("Voie a retirer"),VoieNom,&voie,48) == PNL_OK) {
			for(sortie=0; sortie<GeneOuvert->nbvoies; sortie++) if(GeneOuvert->voie[sortie] == voie) break;
			if(sortie >= GeneOuvert->nbvoies) {
				OpiumWarn(L_("Voie non branchee!!!"));
				return(0);
			}
			GeneOuvert->nbvoies -= 1;
			for( ; sortie<GeneOuvert->nbvoies; sortie++) GeneOuvert->voie[sortie] = GeneOuvert->voie[sortie+1];
			GegeneChangeFait(gene);
			GegeneMontre(GeneOuvert->num);
		}
		break;
	}
	return(0);
}
/* .......................................................................... */
TypeMenuItem iGegeneEdition[] = {
	{ " +", MNU_FONCTION GegeneSortieAjoute },
	{ " - ", MNU_FONCTION GegeneSortieRetire },
	MNU_END
};
/* ========================================================================== */
void GegeneMontre(int gene) {
	int i,j,nb; Menu menu; Panel panel,p; char titre[80];
	int voie; char afficher_entete;

	if(Gene[gene].panneau.avant == 0) {
		Gene[gene].panneau.avant = BoardCreate();
		snprintf(titre,80,L_("Generateur %s"),Gene[gene].nom);
		OpiumTitle(Gene[gene].panneau.avant,titre);
	} else {
		OpiumClear(Gene[gene].panneau.avant);
		BoardTrash(Gene[gene].panneau.avant);
	}
	printf("* Affichage du generateur '%s'\n",Gene[gene].nom);

	menu = MenuLocalise(iGegeneEdition); MenuColumns(menu,2); OpiumSupport(menu->cdr,WND_PLAQUETTE);
	nb = 0; for(j=0; j<Gene[gene].nbvoies; j++) if(Gene[gene].voie[j] >= 0) nb++;
//	if(!nb) { OpiumWarn(L_("Le generateur %s ne genere de donnees dans aucune voie"),Gene[gene].nom); return; }
	if(nb > 0) {
		afficher_entete = 1;
		if(nb > 1) nb++;
		p = panel = PanelCreate(2*nb); PanelColumns(panel,2); PanelMode(panel,PNL_RDONLY|PNL_DIRECT|PNL_BYLINES); PanelSupport(panel,WND_CREUX);
		for(j=0; j<Gene[gene].nbvoies; j++) if((voie = Gene[gene].voie[j]) >= 0) {
			if(nb == 1) {
				PanelItemSelect(panel,PanelText(panel,L_("Detecteur"),Bolo[VoieManip[voie].det].nom,DETEC_NOM),0);
				PanelItemSelect(panel,PanelText(panel,L_("voie"),VoieManip[voie].prenom,MODL_NOM),0);
			} else {
				if(afficher_entete) {
					i = PanelItemSelect(panel,PanelText(panel,0,L_("Detecteur"),DETEC_NOM),0); PanelItemSouligne(panel,i,1);
					i = PanelItemSelect(panel,PanelText(panel,0,L_("voie"),MODL_NOM),0); PanelItemSouligne(panel,i,1);
					afficher_entete = 0;
				}
				// PanelItemSelect(panel,PanelText(panel,0,Bolo[VoieManip[voie].det].nom,DETEC_NOM),0);
				// PanelItemSelect(panel,PanelText(panel,0,VoieManip[voie].prenom,MODL_NOM),0);
				PanelText(panel,0,Bolo[VoieManip[voie].det].nom,DETEC_NOM);
				PanelText(panel,0,VoieManip[voie].prenom,MODL_NOM);
			}
		}
		BoardAddPanel(Gene[gene].panneau.avant,panel,0,Dy,0);
		if(VoiesNb > 1) BoardAddMenu(Gene[gene].panneau.avant,menu,OPIUM_A_DROITE_DE OPIUM_PRECEDENT);
	} else {
		BoardAddMenu(Gene[gene].panneau.avant,menu,Dx,Dy,0);
		GegeneSortieAjoute(menu,0);
		return;
	}

	// avertir si VoieTampon[voie].unite.taille == 0
	panel = Gene[gene].panneau.texte.msg = PanelCreate(1); PanelMode(panel,PNL_RDONLY|PNL_DIRECT); PanelSupport(panel,WND_CREUX);
	PanelItemSelect(panel,PanelText(panel,0,Gene[gene].panneau.message,GENE_MSG_LNGR),0);
	PanelItemColors(panel,1,SambaVertJauneOrangeRouge,2);
	BoardAddPanel(Gene[gene].panneau.avant,panel,OPIUM_EN_DESSOUS_DE p->cdr);

	Gene[gene].panneau.glissiere.ldb = InstrumFloat(INS_GLISSIERE,(void *)&Gliss12lignes,&(Gene[gene].ldb),-32000.0,32000.0);
	InstrumTitle(Gene[gene].panneau.glissiere.ldb,L_("LdB"));
	InstrumOnChange(Gene[gene].panneau.glissiere.ldb,GegeneChangeInstrumLdb,(void *)(long)gene);
	BoardAddInstrum(Gene[gene].panneau.avant,Gene[gene].panneau.glissiere.ldb,OPIUM_EN_DESSOUS_DE 0); // 0,5 * Dy,0);
	panel = Gene[gene].panneau.texte.ldb = PanelCreate(1); PanelMode(panel,PNL_DIRECT); PanelSupport(panel,WND_CREUX);
	PanelFloat(panel,0,&(Gene[gene].ldb)); PanelItemFormat(panel,1,"%5.1f"); PanelItemLngr(panel,1,7);
	PanelItemExit(Gene[gene].panneau.texte.ldb,1,GegeneChangePanelLdb,(void *)(long)gene);
	BoardAddPanel(Gene[gene].panneau.avant,panel,OPIUM_EN_DESSOUS_DE 0);

	Gene[gene].panneau.glissiere.brt = InstrumFloat(INS_GLISSIERE,(void *)&Gliss12lignes,&(Gene[gene].bruit),0.0,500.0); // bruit = Gene[gene].bruit * 12.0;
	InstrumTitle(Gene[gene].panneau.glissiere.brt,L_("Bruit"));
	InstrumOnChange(Gene[gene].panneau.glissiere.brt,GegeneChangeInstrumBrt,(void *)(long)gene);
	BoardAddInstrum(Gene[gene].panneau.avant,Gene[gene].panneau.glissiere.brt,OPIUM_A_DROITE_DE Gene[gene].panneau.glissiere.ldb->cdr);
	panel = Gene[gene].panneau.texte.brt = PanelCreate(1); PanelMode(panel,PNL_DIRECT); PanelSupport(panel,WND_CREUX);
	PanelFloat(panel,0,&(Gene[gene].bruit)); PanelItemFormat(panel,1,"%5.1f"); PanelItemLngr(panel,1,7);
	PanelItemExit(Gene[gene].panneau.texte.brt,1,GegeneChangePanelBrt,(void *)(long)gene);
	BoardAddPanel(Gene[gene].panneau.avant,panel,OPIUM_EN_DESSOUS_DE 0);

	Gene[gene].panneau.glissiere.frq = InstrumFloat(INS_GLISSIERE,(void *)&Gliss12lignes,&(Gene[gene].freq),0.01,1000.0);
	InstrumTitle(Gene[gene].panneau.glissiere.frq,L_("Frequence")); InstrumGradLog(Gene[gene].panneau.glissiere.frq,1);
	InstrumOnChange(Gene[gene].panneau.glissiere.frq,GegeneChangeInstrumFrq,(void *)(long)gene);
	BoardAddInstrum(Gene[gene].panneau.avant,Gene[gene].panneau.glissiere.frq,OPIUM_A_DROITE_DE Gene[gene].panneau.glissiere.brt->cdr);
	panel = Gene[gene].panneau.texte.frq = PanelCreate(1); PanelMode(panel,PNL_DIRECT); PanelSupport(panel,WND_CREUX);
	PanelFloat(panel,0,&(Gene[gene].freq)); PanelItemFormat(panel,1,"%6.2f"); PanelItemLngr(panel,1,7);
	PanelItemExit(Gene[gene].panneau.texte.frq,1,GegeneChangePanelFrq,(void *)(long)gene);
	BoardAddPanel(Gene[gene].panneau.avant,panel,OPIUM_EN_DESSOUS_DE 0);

	Gene[gene].panneau.glissiere.amp = InstrumFloat(INS_GLISSIERE,(void *)&Gliss12lignes,&(Gene[gene].pic),1.0,32000.0);
	InstrumTitle(Gene[gene].panneau.glissiere.amp,L_("Amplitude")); InstrumGradLog(Gene[gene].panneau.glissiere.amp,1);
	InstrumOnChange(Gene[gene].panneau.glissiere.amp,GegeneChangeInstrumAmp,(void *)(long)gene);
	BoardAddInstrum(Gene[gene].panneau.avant,Gene[gene].panneau.glissiere.amp,OPIUM_A_DROITE_DE Gene[gene].panneau.glissiere.frq->cdr);
	panel = Gene[gene].panneau.texte.amp = PanelCreate(1); PanelMode(panel,PNL_DIRECT); PanelSupport(panel,WND_CREUX);
	PanelFloat(panel,0,&(Gene[gene].pic)); PanelItemFormat(panel,1,"%5.1f"); PanelItemLngr(panel,1,7);
	PanelItemExit(Gene[gene].panneau.texte.amp,1,GegeneChangePanelAmp,(void *)(long)gene);
	BoardAddPanel(Gene[gene].panneau.avant,panel,OPIUM_EN_DESSOUS_DE 0);

	Gene[gene].panneau.glissiere.res = InstrumFloat(INS_GLISSIERE,(void *)&Gliss12lignes,&(Gene[gene].reso),0.0,50.0);
	InstrumTitle(Gene[gene].panneau.glissiere.res,L_("Resolution"));
	InstrumOnChange(Gene[gene].panneau.glissiere.res,GegeneChangeInstrumRes,(void *)(long)gene);
	BoardAddInstrum(Gene[gene].panneau.avant,Gene[gene].panneau.glissiere.res,OPIUM_A_DROITE_DE Gene[gene].panneau.glissiere.amp->cdr);
	panel = Gene[gene].panneau.texte.res = PanelCreate(1); PanelMode(panel,PNL_DIRECT); PanelSupport(panel,WND_CREUX);
	PanelFloat(panel,0,&(Gene[gene].reso)); PanelItemFormat(panel,1,"%5.1f"); PanelItemLngr(panel,1,7);
	PanelItemExit(Gene[gene].panneau.texte.res,1,GegeneChangePanelRes,(void *)(long)gene);
	BoardAddPanel(Gene[gene].panneau.avant,panel,OPIUM_EN_DESSOUS_DE 0);

	// BoardAddPoussoir(Gene[gene].panneau.avant,"Power",&(Gene[gene].mode_actif),OPIUM_EN_DESSOUS_DE (Gene[gene].panneau.texte.ldb)->cdr); // .brt
	menu = MenuBouton("Power", MNU_FONCTION GenereOnOff);
	if(Gene[gene].mode_actif) MenuItemAllume(menu,1," On  ",GRF_RGB_YELLOW); else MenuItemAllume(menu,1," Off ",GRF_RGB_RED);
	BoardAddMenu(Gene[gene].panneau.avant,menu,OPIUM_EN_DESSOUS_DE (Gene[gene].panneau.texte.ldb)->cdr); // .brt
	Gene[gene].panneau.bouton.manu = BoardAddPoussoir(Gene[gene].panneau.avant,L_("Manuel"),&(Gene[gene].mode_manu),OPIUM_A_DROITE_DE OPIUM_PRECEDENT);
	BoardAddPoussoir(Gene[gene].panneau.avant,"Auto",&(Gene[gene].mode_auto),OPIUM_A_DROITE_DE OPIUM_PRECEDENT);
	panel = PanelCreate(1); PanelMode(panel,PNL_DIRECT); PanelSupport(panel,WND_CREUX);
	PanelKeyB(panel,0,GENE_INTERV_CLES,&(Gene[gene].interv),8);
	BoardAddPanel(Gene[gene].panneau.avant,panel,OPIUM_A_DROITE_DE OPIUM_PRECEDENT);
	panel = Gene[gene].panneau.evts = PanelCreate(1); PanelMode(panel,PNL_DIRECT|PNL_RDONLY); PanelSupport(panel,WND_CREUX);
	PanelInt(panel,0,&(Gene[gene].evts)); PanelItemLngr(panel,1,7);
	BoardAddPanel(Gene[gene].panneau.avant,panel,OPIUM_A_DROITE_DE OPIUM_PRECEDENT);

	Gene[gene].panneau.bouton.svg = BoardAddBouton(Gene[gene].panneau.avant,L_("Sauver"),GegeneSauve,OPIUM_EN_DESSOUS_DE (Gene[gene].panneau.texte.res)->cdr);

	if(!SettingsGenePossible) strcpy(Gene[gene].panneau.message,L_("Generateur deconnecte (voir Options)"));
	// pour WXO: printf("(%s) Generateur '%s' construit\n",__func__,Gene[gene].nom);
	OpiumUse(Gene[gene].panneau.avant,OPIUM_FORK);
	// pour WXO: printf("(%s) Generateur '%s' affiche\n",__func__,Gene[gene].nom);
	OpiumUserAction();
}
