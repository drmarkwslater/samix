#include "claps.h"
#include <opium.h>

#ifdef CW5
#define __func__ "xxx"
#endif

#define OPIUM_TABLE_CLES "./e/+/-"
typedef enum {
	OPIUM_TABLE_EDIT = 1,
	OPIUM_TABLE_AJOUTE,
	OPIUM_TABLE_RETIRE,
	OPIUM_TABLE_NBCMDES
} OPIUM_TABLE_CMDE;

static OpiumColor OpiumTableCouleurs[] = {
	{ GRF_RGB_GREEN }, { GRF_RGB_ORANGE }, { GRF_RGB_YELLOW }, { GRF_RGB_RED }
};

/* ========================================================================== */
static char *OpiumTableBouton(void *adrs) {
	if(adrs) {
		ArgDesc *r;
		
		r = (ArgDesc *)adrs;
		if(r->type == ARG_TYPE_STRUCT) OpiumTableExec(r->texte,r->type,r->adrs,r->taille,1,0);
		else OpiumTableExec(r->texte,r->type,r->adrs,r->taille,1,r);
		return(0);
	} else return("ouvrir");
}
/* ========================================================================== */
char OpiumTableExec(char *nom, ARG_TYPES type, void *var, int *nb, int nbrepet, ArgDesc *restreinte) {
	ArgStruct *argstruct;
	void *table;
	ArgDesc *d,*r,*s,**v; int nbcols,nbligs;
	Panel p;
	long elt,of7; void *adrs;
	int i,j,k,m,saut,lngr,rc;
	char *fmt;
	char *txt;
#define MAX_LIGNES 4096
#define MAX_COLS 64
	char numero[MAX_LIGNES][4],cmde[MAX_LIGNES],rmk[MAX_LIGNES][MAX_COLS];
	char nom_elt[256],nom_complet[256];
	char souligne,modifs,a_refaire,autre;

	if(!var) return(0);
	argstruct = 0; k = 0;
	d = restreinte;
	switch(type) {
	  case ARG_TYPE_CHAR: case ARG_TYPE_OCTET: case ARG_TYPE_BYTE:
	  case ARG_TYPE_LETTRE: case ARG_TYPE_BOOL: case ARG_TYPE_KEY:
		if((type == ARG_TYPE_KEY) && !d) return(0);
		table = var; saut = 1; nbcols = 1;
		break;
	  case ARG_TYPE_SHORT: case ARG_TYPE_SHEX:
		table = var; saut = 2; nbcols = 1;
		break;
	  case ARG_TYPE_INT: case ARG_TYPE_HEXA: case ARG_TYPE_LISTE: case ARG_TYPE_FLOAT:
		table = var; saut = 2; nbcols = 1;
		break;
	  case ARG_TYPE_TEXTE:
		table = var; nbcols = 1;
		if(d) saut = d->lngr; else saut = 1;
		break;
	  case ARG_TYPE_DESC:
		if(!d) return(0);
		table = var; saut = 4;
		r = d; nbcols = 0;
		while(r->type >= 0) { nbcols++; r++; }
		if(nbcols > MAX_COLS) nbcols = MAX_COLS;			
		break;
	  case ARG_TYPE_STRUCT:
		argstruct = (ArgStruct *)var;
		if(!d) d = argstruct->desc; if(!d) return(0);
		table = argstruct->table;
		saut = argstruct->lngr;
		r = d; nbcols = 0;
		while(r->type >= 0) { if(r->type != ARG_TYPE_RIEN) nbcols++; r++; }
		if(nbcols > MAX_COLS) nbcols = MAX_COLS;
		break;
	  default:
		table = var; saut = 4; nbcols = 1;
		break;
	}
	if(!nb) nbligs = 1; /* pas une table (utilise pour une structure: type == ARG_TYPE_DESC) */
	else if(*nb) nbligs = *nb;
	else {
		nbligs = 1;
		memset((void *)table,0,saut);
	}
	// printf("Edition d'%s %s\n",nb?"une vraie table:":"un element de",nom);
/*	printf("(%s) Edition de %s[%d] %s(%d)\n",__func__,ArgNomType[type],nbligs,nom,nbcols);
	if((type == ARG_TYPE_DESC) || (type == ARG_TYPE_STRUCT)) {
		r = d;
		while(r->type >= 0) {
			if(r->texte) printf("(%s) . element #%d: %s %s[%d] @%08lX\n",__func__,(int)(r - d),ArgNomType[(int)r->type],r->texte,r->dim,(Ulong)r->adrs);
			else printf("(%s) . element #%d: %s anonyme[%d] @%08lX\n",__func__,(int)(r - d),ArgNomType[(int)r->type],r->dim,(Ulong)r->adrs);
			r++; 
		}
	} */
	if(nbrepet <= 0) nbrepet = 1;
	if(nbligs < 0) return(0); else if(nbligs > MAX_LIGNES) nbligs = MAX_LIGNES;
	modifs = 0;

	do {
		if(nb) /* vraie table */ {
			p = PanelCreate((nbcols + 1) * (nbligs + 1));
			PanelColumns(p,(nbcols + 1) * nbrepet);
			PanelMode(p,PNL_BYLINES);
			PanelVerrouEntete(p,1);
			if((type == ARG_TYPE_DESC) || (type == ARG_TYPE_STRUCT)) {
				PanelItemSouligne(p,PanelRemark(p,0),1);
				if((r = d)) while(r->type >= 0) {
					k = PanelRemark(p,r->texte);
					PanelItemSouligne(p,k,1);
					PanelItemBarreGauche(p,k,1);
					r++;
				}
			} else {
				PanelItemSouligne(p,PanelRemark(p,0),1);
				PanelItemSouligne(p,PanelRemark(p,nom),1);
			}
		} else /* un element unique de DESC, en fait */ {
			p = PanelCreate(nbcols * 2);
			PanelColumns(p,2);
			PanelMode(p,PNL_BYLINES);
		}
		PanelTitle(p,nom);
		elt = (long)table;
		lngr = d? ((d->lngr)? d->lngr: 0): 0;
		fmt = d? ((d->modele)? d->modele: 0): 0;
		for(i=0; i<nbligs; i++, elt += saut) {
			if(nb) {
				souligne = !((i + 1) % 3); 
				sprintf(&(numero[i][0]),"%3d",i+1);
				cmde[i] = 0;
				k = PanelKeyB(p,&(numero[i][0]),OPIUM_TABLE_CLES,cmde+i,2);
				PanelItemColors(p,k,OpiumTableCouleurs,4);
				if(i == (nbligs - 1)) PanelCursorOnVar(p,k,0);
			} else {
				souligne = 0;
				// k = PanelRemark(p,&(numero[i][0]));
			}
			PanelItemSouligne(p,k,souligne);
			adrs = (void *)elt;
			switch(type) {
				case ARG_TYPE_COMMENT: k = PanelRemark(p,0); break;
				case ARG_TYPE_TEXTE:   k = PanelText (p,0,adrs,lngr? lngr: 16); break;
				case ARG_TYPE_INT:     k = PanelInt  (p,0,adrs); break;
				case ARG_TYPE_HEXA:    k = PanelHex  (p,0,adrs); break;
				case ARG_TYPE_SHORT:   k = PanelShort(p,0,adrs); break;
				case ARG_TYPE_SHEX:    k = PanelSHex (p,0,adrs); break;
				case ARG_TYPE_FLOAT:   k = PanelFloat(p,0,adrs); break;
				case ARG_TYPE_OCTET:   k = PanelOctet(p,0,adrs); break;
				case ARG_TYPE_BYTE:    k = PanelByte (p,0,adrs); break;
				case ARG_TYPE_BOOL:    k = PanelBool (p,0,adrs); break;
				case ARG_TYPE_KEY:     k = PanelKeyB (p,0,d? d->modele: 0,adrs,12); break;
				case ARG_TYPE_LISTE:   k = PanelList (p,0,d? (char **)d->modele: 0,adrs,12); break;
				case ARG_TYPE_USER:    k = PanelUser (p,0,d? (TypeArgUserFctn)d->modele: 0,adrs,12); break;
				case ARG_TYPE_DESC: 
				case ARG_TYPE_STRUCT:
					j = 0;
					if((r = d)) while(r->type >= 0) {
						if(r->type == ARG_TYPE_RIEN) { r++; continue; }
						if(nb) txt = 0; else txt = r->texte;
						rmk[i][j] = 0;
						if(r->adrs) {
							if(r->dim == 1) /* pas une table (meme dimensionnee a 1) */ {
								if(type == ARG_TYPE_DESC) adrs = r->adrs;
								else {
									of7 = (long)r->adrs - (long)argstruct->tempo;
									adrs = (void *)(elt + of7);
								}
								lngr = r? ((r->lngr)? r->lngr: 0): 0;
								fmt = r? ((r->modele)? r->modele: 0): 0;
								switch(r->type) {
									case ARG_TYPE_COMMENT: k = PanelRemark(p,txt); break;
									case ARG_TYPE_TEXTE:   k = PanelText (p,txt,adrs,lngr? lngr: 16); break;
									case ARG_TYPE_INT:     k = PanelInt  (p,txt,adrs); break;
									case ARG_TYPE_HEXA:    k = PanelHex  (p,txt,adrs); break;
									case ARG_TYPE_SHORT:   k = PanelShort(p,txt,adrs); break;
									case ARG_TYPE_SHEX:    k = PanelSHex (p,txt,adrs); break;
									case ARG_TYPE_FLOAT:   k = PanelFloat(p,txt,adrs); break;
									case ARG_TYPE_OCTET:   k = PanelOctet(p,txt,adrs); break;
									case ARG_TYPE_BYTE:    k = PanelByte (p,txt,adrs); break;
									case ARG_TYPE_BOOL:    k = PanelBool (p,txt,adrs); break;
									case ARG_TYPE_KEY:     k = PanelKeyB (p,txt,r->modele,adrs,lngr? lngr: 12); break;
									case ARG_TYPE_LISTE:   k = PanelList (p,txt,(char **)r->modele,adrs,lngr? lngr: 12); break;
									case ARG_TYPE_USER:    k = PanelUser (p,txt,(TypeArgUserFctn)r->modele,adrs,lngr? lngr: 12); break;
									// case ARG_TYPE_STRUCT:  k = PanelKeyB (p,txt,"(structure)/editer",&(rmk[i][j]),12); lngr = 0; break;
									case ARG_TYPE_STRUCT:  k = PanelBouton(p,txt,OpiumTableBouton,(void *)r,6); lngr = 0; break;
									case ARG_TYPE_DESC:    k = PanelKeyB (p,txt,"(descript.)/editer",&(rmk[i][j]),12); lngr = 0; break;
									case ARG_TYPE_RIEN:    k = PanelRemark(p,"(obsolete)"); break;
									case ARG_TYPE_VARIANTE: PanelKeyB(p,txt,"(ca depend)/editer",&(rmk[i][j]),12); lngr = 0; break;
									default: k = PanelRemark(p,"(a supporter)"); break;
								}
								if(lngr) PanelLngr(p,k,lngr);
								if(fmt) PanelFormat(p,k,fmt);
							} else {
								k = PanelKeyB(p,txt,"(table)/editer",&(rmk[i][j]),12);
								// k = PanelBouton(p,txt,OpiumTableBouton,(void *)r,6);
							}
						} else k = PanelRemark(p,txt);
						if(!nb) PanelRemark(p,r->explic);
						PanelItemBarreGauche(p,k,1);
						PanelItemSouligne(p,k,souligne);
						r++; j++;
						if(j > MAX_COLS) break;
					}
					k = -1;
					break;
				case ARG_TYPE_RIEN:     k = PanelRemark(p,"(obsolete)"); break;
				case ARG_TYPE_VARIANTE: k = PanelKeyB(p,0,"(ca depend)/editer",&(rmk[i][0]),12); lngr = 0; break;
				default: k = PanelRemark(p,"(a supporter)"); break;
			}
			if(k >= 0) {
				if(lngr) PanelLngr(p,k,lngr);
				if(fmt) PanelFormat(p,k,fmt);
				PanelItemSouligne(p,k,souligne);
			}
		}
		a_refaire = 0;
		// (p->cdr)->dy = 999999; // OpiumScrollYmax(p->cdr);
		if((rc = OpiumExec(p->cdr)) == PNL_OK) {
			if(nb) {
				if(*nb == 0) *nb = 1;
				i = nbligs;
				while(i--) {
					sprintf(nom_elt,"%d",i+1);
					if(cmde[i] == OPIUM_TABLE_EDIT) /* edition en version complete */ {
						elt = (long)table + (i * saut);
						// sprintf(nom_complet,"%s[%s]",nom,nom_elt);
						sprintf(nom_complet,"%s: %s",nom,nom_elt);
						if(type == ARG_TYPE_STRUCT) {
							s = restreinte;
							if(!s) s = argstruct->desc; if(!s) continue;
							memcpy((void *)argstruct->tempo,(void *)elt,saut);
							if(OpiumTableExec(nom_complet,ARG_TYPE_DESC,(void *)elt,0,1,s) == PNL_OK) modifs = 1;
							memcpy((void *)elt,(void *)argstruct->tempo,saut);
						} else if(OpiumTableExec(nom_complet,type,(void *)elt,0,1,restreinte) == PNL_OK) modifs = 1;
						a_refaire = 1;
					} else if(cmde[i] == OPIUM_TABLE_AJOUTE) /* insertion */ {
						if(nbligs < MAX_LIGNES) {
							nbligs += 1;
							j = nbligs;
							while(--j > i) {
								elt = (long)table + (j * saut);
								memcpy((void *)elt,(void *)(elt - saut),saut);
							}
							if(nb) *nb = nbligs;
							modifs = 1; a_refaire = 1;
						}
					} else if(cmde[i] == OPIUM_TABLE_RETIRE) /* suppression */ {
						if(nbligs) {
							nbligs -= 1;
							for(j=i; j<nbligs; j++) {
								elt = (long)table + (j * saut);
								memcpy((void *)elt,(void *)(elt + saut),saut);
							}
							if(nb) *nb = nbligs;
							modifs = 1; a_refaire = 1;
						}
					} else if((type == ARG_TYPE_DESC) || (type == ARG_TYPE_STRUCT)) /* rien sauf si edition d'un champ particulier */ {
						elt = (long)table + (i * saut);
						if(type == ARG_TYPE_STRUCT) memcpy((void *)argstruct->tempo,(void *)elt,saut);
						j = 0; autre = 0;
						if((r = d)) while(r->type >= 0) {
							if(r->type == ARG_TYPE_RIEN) { r++; continue; }
							if((r->type == ARG_TYPE_TEXTE) && !autre) strcpy(nom_elt,(char *)r->adrs);
							if((r->type != ARG_TYPE_COMMENT) && (r->type != ARG_TYPE_RIEN)) autre = 1;
							if(rmk[i][j]) {
								a_refaire = 1;
								if(r->dim == 1) /* pas une table (meme dimensionnee a 1) */ {
									switch(r->type) {
									  case ARG_TYPE_STRUCT:  break;
									  case ARG_TYPE_DESC:
										// if(r->texte) sprintf(nom_complet,"%s[%s].%s",nom,nom_elt,r->texte);
										// else sprintf(nom_complet,"%s[%s].<%d>",nom,nom_elt,j+1);
										if(r->texte) sprintf(nom_complet,"%s.%s",nom_elt,r->texte);
										 else sprintf(nom_complet,"%s.<%d>",nom_elt,j+1);
										if(OpiumTableExec(nom_complet,ARG_TYPE_DESC,var,0,1,(ArgDesc *)r->adrs) == PNL_OK) modifs = 1;
										break;
									  case ARG_TYPE_RIEN:    break;
									  case ARG_TYPE_VARIANTE:
										// printf("(%s) . edition de la variante #%d\n",__func__,*(r->modele));
										// if(r->texte) sprintf(nom_complet,"%s[%s].%s",nom,nom_elt,r->texte);
										// else sprintf(nom_complet,"%s[%s].<%d>",nom,nom_elt,j+1);
										if(r->texte) sprintf(nom_complet,"%s.%s",nom_elt,r->texte);
										else sprintf(nom_complet,"%s.<%d>",nom_elt,j+1);
										v = (ArgDesc **)r->adrs; m = (int)*(r->modele);
										if(OpiumTableExec(nom_complet,ARG_TYPE_DESC,var,0,1,v[m]) == PNL_OK) modifs = 1;
										break;
									  default: OpiumError("Champ non editable)"); break;
									}
								} else {
									// printf("(%s) . edition recursive d'une table[%d] avec *(%08lX)=%d\n",__func__,r->dim,r->taille,*(r->taille));
									// if(r->texte) sprintf(nom_complet,"%s[%s].%s",nom,nom_elt,r->texte);
									// else sprintf(nom_complet,"%s[%s].<%d>",nom,nom_elt,j+1);
									if(r->texte) sprintf(nom_complet,"%s.%s",nom_elt,r->texte);
									else sprintf(nom_complet,"%s.<%d>",nom_elt,j+1);
									if(r->type == ARG_TYPE_STRUCT) rc = OpiumTableExec(nom_complet,r->type,r->adrs,r->taille,1,0);
									else rc = OpiumTableExec(nom_complet,r->type,r->adrs,r->taille,1,r);
									if(rc == PNL_OK) modifs = 1;
								}
							}
							r++; j++;
						}
						if(type == ARG_TYPE_STRUCT) memcpy((void *)elt,(void *)argstruct->tempo,saut); // modif = 1; ?
					}
				}
			} else modifs = 1; // ??
		}
		PanelDelete(p);
	} while(a_refaire);

	return((modifs || (rc == PNL_OK))? PNL_OK: PNL_CANCEL);
}

/* Declaration des structures editables pour generation auto des dialogues */
/* ========================================================================== */
static OpiumTableDeclaration OpiumTableUtilisee(Menu menu) {
	OpiumTableDeclaration courante;
	
	courante = OpiumTableChaineGlobale;
	while(courante) {
		if(courante->menu == menu) return(courante);
		courante = courante->suivante;
	}
	return(0);
}
/* ========================================================================== */
static void OpiumTableDesc(OpiumTableDeclaration courante, ArgDesc *d) {
	d->texte = courante->nom;
	d->type = courante->type; d->handle = 0;
	d->lngr = 0; // sauf type texte
	d->dim = *courante->nb; d->taille = 0;
	d->modele = 0;
	d->adrs = courante->var; d->explic = (char *)0;
	d++;
	d->texte = (char *)0; d->type = -1; d->handle = 0;
	d->lngr = 0; d->dim = 0; d->taille = (int *)0;
	d->modele = (char *)0;
	d->adrs = (void *)0; d->explic = (char *)0;
}
/* ========================================================================== */
static char OpiumTableEdite(Menu menu, MenuItem item) {
	OpiumTableDeclaration courante;
	char nom_panel[80];
	
	if(!(courante = OpiumTableUtilisee(menu))) return(0);
	snprintf(nom_panel,80,"%s: table",courante->nom);
	OpiumTableExec(nom_panel,courante->type,(void *)(courante->var),courante->nb,1,courante->restreinte);
	return(0);
}
/* ========================================================================== */
static char OpiumTableListe(Menu menu, MenuItem item) {
	OpiumTableDeclaration courante;
	ArgDesc d[2];
	
	if(!(courante = OpiumTableUtilisee(menu))) return(0);
	OpiumTableDesc(courante,d);
	ArgPrint("*",d);

	return(0);
}
/* ========================================================================== */
static char OpiumTableSauve(Menu menu, MenuItem item) {
	OpiumTableDeclaration courante;
	ArgDesc d[2],*desc; char propose[MAXFILENAME],*fichier;

	if(!(courante = OpiumTableUtilisee(menu))) return(0);
	if(!(desc = courante->desc)) { OpiumTableDesc(courante,d); desc = d; }
	if(courante->fichier[0] == '\0') {
		sprintf(propose,"./%s.defs",courante->nom);
		if(OpiumReadFile("sur quel fichier",propose,MAXFILENAME) == PNL_CANCEL) return(0);
		fichier = propose;
	} else fichier = courante->fichier;
	ArgPrint(fichier,desc);
	if(courante->nom[0]) printf("* Table des %s%s sauvee dans %s\n",
		courante->nom,(courante->nom[strlen(courante->nom)-1] == 's')? "":"s",fichier);
	else printf("* Table (sans nom) sauvee dans %s\n",fichier);

	return(0);
}
/* ========================================================================== */
static char OpiumTableRestore(Menu menu, MenuItem item) {
	OpiumTableDeclaration courante;
	ArgDesc d[2],*desc; char propose[MAXFILENAME],*fichier;

	if(!(courante = OpiumTableUtilisee(menu))) return(0);
	if(!(desc = courante->desc)) { OpiumTableDesc(courante,d); desc = d; }
	if(courante->fichier[0] == '\0') {
		sprintf(propose,"./%s.defs",courante->nom);
		if(OpiumReadFile("depuis quel fichier",propose,MAXFILENAME) == PNL_CANCEL) return(0);
		fichier = propose;
	} else fichier = courante->fichier;
	ArgScan(fichier,desc);
	if(courante->nom[0]) printf("* Table des %s%s relue depuis %s\n",
		courante->nom,(courante->nom[strlen(courante->nom)-1] == 's')? "":"s",fichier);
	else printf("* Table (sans nom) relue depuis %s\n",fichier);

	return(0);
}
/* ========================================================================== */
static TypeMenuItem OpiumTableItems[] = {
	{ "Editer",       MNU_FONCTION OpiumTableEdite },
	{ "Lister",       MNU_FONCTION OpiumTableListe },
	{ "Sauver",       MNU_FONCTION OpiumTableSauve },
	{ "Relire",       MNU_FONCTION OpiumTableRestore },
	{ "Quitter",      MNU_RETOUR },
	MNU_END
};
/* ========================================================================== */
void OpiumTableCreate(OpiumTableDeclaration *liste) { *liste = 0; }
/* ========================================================================== */
OpiumTableDeclaration OpiumTableAdd(OpiumTableDeclaration *liste,
	char *nom, char type, void *var, int *nb, ArgDesc *restreinte) {
	OpiumTableDeclaration decl,courante,derniere;
	char nom_menu[80];
	
	decl = (OpiumTableDeclaration)malloc(sizeof(TypeOpiumTableDeclaration));
	if(!decl) return(0);
	derniere = 0;
	if(*liste) courante = *liste; else courante = OpiumTableChaineGlobale;
	while(courante) { derniere = courante; courante = courante->suivante; }
	if(derniere) derniere->suivante = decl; else OpiumTableChaineGlobale = decl;
	if(*liste == 0) *liste = decl;
	snprintf(nom_menu,80,"%s: acces",nom);
	decl->nom = nom;
	decl->type = type;
	decl->menu = MenuCreate(OpiumTableItems); MenuTitle(decl->menu,nom_menu);
	decl->restreinte = restreinte;
	decl->nb = nb;
	decl->var = var;
	decl->suivante = 0;
	decl->fichier[0] = '\0';
	decl->desc = 0;
	return(decl);
}
/* ========================================================================== */
OpiumTableDeclaration OpiumTablePtr(OpiumTableDeclaration liste, char *nom) {
	OpiumTableDeclaration courante;
	
	courante = liste;
	while(courante) {
		if(!strcmp(courante->nom,nom)) return(courante);
		courante = courante->suivante;
	}
	return(0);
}
/* ========================================================================== */
void OpiumTableFichier(OpiumTableDeclaration decl, char *fichier, ArgDesc *desc) {
	if(!decl) return;
	if(fichier) strncpy(decl->fichier,fichier,MAXFILENAME);
	decl->desc = desc;
}
/* ========================================================================== */
Menu OpiumTableMenuCreate(OpiumTableDeclaration liste) {
	OpiumTableDeclaration courante;
	Menu m; int nb;
	
	nb = 0;
	courante = liste;
	while(courante) { nb++; courante = courante->suivante; }
	m = MenuCreate(0);
	MenuItemArray(m,nb+1);
	courante = liste;
	while(courante) {
		MenuItemAdd(m,courante->nom,MNU_MENU &(courante->menu));
		courante = courante->suivante;
	}
	MenuItemAdd(m,"Quitter",MNU_RETOUR);
	return(m);
}
