/*
 *  scripts.c
 *  pour le projet Samba
 *
 *  Created by Michel GROS on 28.02.12.
 *  Copyright 2012 CEA/DSM/DAPNIA. All rights reserved.
 *
 */

#define SCRIPTS_C

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <decode.h>
#include <dateheure.h>

#include <samba.h>
#include <objets_samba.h>
#include <detecteurs.h>
#include <numeriseurs.h>
#include <media.h>
#include <autom.h>
#include <archive.h>
// #include <banc.h>
#include <scripts.h>

#define MAXLIGNE 256

static HwScript ScriptEnCours;
static TypeHwExecContext ScriptPC;

static int ScriptAjoute(HwScript script, char *nomcomplet, char *prefixe, char *bloc_ouvert, int *bcur);
static int ScriptStoppe(Panel p, void *arg);

/* ========================================================================== */
void ScriptInit() {
	/* doit etre appele apres OpiumInit a cause des PanelCreate */
	int i;

	mScriptIntr = MenuLocalise(iScriptIntr);
	OpiumSupport(mScriptIntr->cdr,WND_PLAQUETTE);

	pScriptEtat = PanelCreate(1);
	i = PanelKeyB(pScriptEtat,0," /en cours/termine",&ScriptEtat,10);
	PanelItemColors(pScriptEtat,i,SambaBleuOrangeVertRougeJaune,3); PanelItemSouligne(pScriptEtat,i,0);
	PanelSupport(pScriptEtat,WND_CREUX); PanelMode(pScriptEtat,PNL_RDONLY|PNL_DIRECT);
//	PRINT_OPIUM_ADRS(pScriptEtat);

	pScriptCntl = PanelCreate(1);
	PanelSupport(pScriptCntl,WND_CREUX);
	i = PanelInt(pScriptCntl,"wait",&ScriptDuree); // &ScriptWait);
	PanelItemLngr(pScriptCntl,i,6); PanelItemSelect(pScriptCntl,i,0);
	PanelBoutonText(pScriptCntl,PNL_RESET,"kill"); PanelOnReset(pScriptCntl,ScriptStoppe,(void *)1);
	PanelBoutonText(pScriptCntl,PNL_APPLY,"stop"); PanelOnApply(pScriptCntl,ScriptStoppe,(void *)0);
//	PRINT_OPIUM_ADRS(pScriptCntl);
	
	pScriptWait = PanelCreate(1); PanelMode(pScriptWait,PNL_DIRECT|PNL_RDONLY); PanelSupport(pScriptWait,WND_CREUX);
	i = PanelInt(pScriptWait,"Attente fin regen:",&ScriptDuree); // &ScriptWait);
	PanelItemLngr(pScriptWait,i,4); PanelItemSelect(pScriptWait,i,0);
//	PRINT_OPIUM_ADRS(pScriptWait);
	
	ScriptModeExec = 1;
	ScriptDateLimite = 0;
	ScriptEtat = 0;
	ScriptKill = 0;
	
	ScriptEnCours = 0;
	ScriptPC.inst = 0; ScriptPC.boucle = 0; ScriptPC.date = -1;
	ScriptSuite[0] = '\0';
}
/* ========================================================================== */
void ScriptRaz(HwScript script) {
	script->nb_blocs = 0; script->nb_actns = 0;
	script->precedent = script->suivant = 0;
	script->nb_vars = 0;
}
/* ========================================================================== */
int ScriptEcritSyntaxe() { 
	char i,*ligne;
	i = 0; ligne = ScriptSyntaxe[i];
	while(*ligne) { printf("%s\n",ligne); ligne = ScriptSyntaxe[++i]; }
	return(0);
}
/* ========================================================================== */
static HwScript ScriptCree() {
	HwScript script;

	script = (HwScript)malloc(sizeof(TypeHwScript));
	if(script) {
		ScriptRaz(script);
		if(ScriptEnCours) ScriptEnCours->suivant = script;
		script->precedent = ScriptEnCours;
		ScriptEnCours = script;
	}
	return(script);
}
/* ========================================================================== */
static HwScript ScriptRetireDernier() {
	HwScript script;

	script = 0;
	if(ScriptEnCours) {
		script = ScriptEnCours->precedent;
		free(ScriptEnCours);
	}
	if(script) script->suivant = 0;
	ScriptEnCours = script;
	return(script);
}
/* ========================================================================== */
static HwBoucle ScriptBoucleNouvelle(HwBoucle *en_cours, short debut, HW_OBJ obj, void *hw, int decompte) {
	HwBoucle boucle;
	
	boucle = (HwBoucle)malloc(sizeof(TypeHwBoucle));
	if(boucle) {
		boucle->debut = debut;
		boucle->obj = obj;
		boucle->hw = hw;
		boucle->decompte = decompte;
		boucle->limitee = (decompte != -1);
		boucle->en_cours = boucle->limitee;
		if((*en_cours)) (*en_cours)->suivante = boucle;
		boucle->precedente = *en_cours;
		*en_cours = boucle;
	}
	return(boucle);
}
/* ========================================================================== */
static HwBoucle ScriptBoucleTermine(HwBoucle *en_cours) {
	HwBoucle boucle;
	
	if((*en_cours)) {
		boucle = (*en_cours)->precedente;
		free(*en_cours);
	} else boucle = 0;
	
	if(boucle) boucle->suivante = 0;
	*en_cours = boucle;
	return(boucle);
}
/* ========================================================================== */
void ScriptBoucleVide(HwBoucle *en_cours) {
	HwBoucle boucle;
	
	while(*en_cours) {
		boucle = (*en_cours)->precedente;
		free(*en_cours);
		*en_cours = boucle;
	}
}
/* ========================================================================== */
static INLINE char *ScriptPrefixe(char *prefixe, char *debut, int max) {
	if(prefixe) {
		if(prefixe[0]) return(prefixe); else sprintf(debut,"%s  [",DateHeure()); // "%s  | "
	} else debut[0] = '\0';
	return(debut);
}
/* ========================================================================== */
static int ScriptStoppe(Panel p, void *arg) {
	int k;
	
	k = (int)(IntAdrs)arg; if(k) { ScriptKill = 1; ScriptDuree = 0; } else ScriptDuree -= ScriptWait;
	ScriptWait = 0; 
	if(ScriptKill) printf("%s/ Script arrete definitivement par l'utilisateur\n",DateHeure());
	else printf("%s/ Attente supprimee par l'utilisateur\n",DateHeure());
	if(OpiumDisplayed(pScriptCntl->cdr)) PanelRefreshVars(pScriptCntl);
	if(OpiumDisplayed(pScriptWait->cdr)) PanelRefreshVars(pScriptWait);
	// if(OpiumDisplayed(mScriptIntr->cdr)) MenuItemEteint(mScriptIntr,1,0);
	return(0);
}
/* ========================================================================== */
int ScriptArrete(Menu menu, MenuItem item) { ScriptWait = 0; return(0); }
/* ========================================================================== */
int ScriptNouveauBloc(HwScript script, HW_OBJ obj, void *hw) {
	short num;
	
	if((num = script->nb_blocs) >= HW_MAXBLOC) return(-1);
	if((obj == HW_GLOBAL) && (script->bloc[num-1].obj == HW_GLOBAL)) return(num - 1);
	script->bloc[num].obj = obj;
	script->bloc[num].hw = hw;
	script->bloc[num].premiere = script->nb_actns; script->bloc[num].derniere = script->nb_actns;
	script->bloc[num].courante = script->bloc[num].premiere;
	script->bloc[num].num = num;
//	script->bloc[num].nb = 0;
	script->bloc[num].invalide = 0;
	script->nb_blocs += 1;
	return(num);
}
/* ========================================================================== */
char ScriptDecode(HwScript script, char *ligne, char *bloc_ouvert, int *bcur, char *prefixe) {
	char *r; char sep; size_t l;
	char *symbole; HW_OBJ obj;
	int bolo,bb,modl,b; char ok; char debut[80],valeur[HW_MAXNOM];
	Media media;
	
	// printf("(%s) Decodage de la ligne: %s",__func__,ligne);
	r = ligne;
	if((*r == '#') || (*r == '\n')) return(1);
	
	ok = 1;
	symbole = ItemAvant(" :=",&sep,&r); // " :=+-*/" si symboles sans '-'
	// printf("(%s) > symbole #1: '%s', separateur '%c' (bloc %s)\n",__func__,symbole,sep,(*bloc_ouvert)?"ouvert":"ferme");
	if(symbole[0] == '\0') return(1);
	if(!strcmp(symbole,"}")) { *bloc_ouvert = 0; return(1); }
	if(!strcmp(symbole,"exec")) {
		ScriptAjoute(script,ItemJusquA(' ',&r),prefixe,bloc_ouvert,bcur);
		return(1);
	}
	script->action[script->nb_actns].sautee = 0;
	script->action[script->nb_actns].limitee = 0;
	
	/* Definition du bloc courant */
	if(*bloc_ouvert == 0) {
		for(obj=HW_DETEC; obj<HW_NB; obj++) if(!strcmp(symbole,HwCat[obj])) break;
		if(obj >= HW_NB) obj = HW_GLOBAL;
		if((b = ScriptNouveauBloc(script,obj,0)) < 0) {
			if(prefixe) printf("%s! Bloc de script inutilisable (deja %d bloc%s cree%s)\n",ScriptPrefixe(prefixe,debut,80),Accord2s(script->nb_blocs));
			return(0);
		}
		*bcur = b;
		//- if(obj == HW_GLOBAL) sep = '\0';
		if(obj == HW_DEFINE) {
			if(sep != ' ') { if(prefixe) printf("%s! Nom de variable oublie: '%s'\n",ScriptPrefixe(prefixe,debut,80),symbole); ok = 0; }
			else script->nb_vars += 1;
		} else if(obj != HW_GLOBAL) {
			symbole = ItemAvant(":{",&sep,&r);
			if(*symbole == '<') {
				if(obj == HW_DETEC) script->bloc[b].obj = HW_MODL_DET;
				else if(obj == HW_NUMER) script->bloc[b].obj = HW_MODL_NUM;
				symbole++;
				l = strlen(symbole) - 1;
				if(symbole[l] == '>') symbole[l] = '\0';
			}
			if(!strcmp(symbole,"tous")) {
				if(obj == HW_MODL_DET) script->bloc[b].obj = HW_DETEC;
				else if(obj == HW_MODL_NUM) script->bloc[b].obj = HW_NUMER;
			} else /* trouver l'adresse de l'objet ainsi designe */ switch(obj) {
			  case HW_DETEC:
				for(bolo=0; bolo<BoloNb; bolo++) if(!strcmp(symbole,Bolo[bolo].nom)) break;
				if(bolo < BoloNb) script->bloc[b].hw = (void *)(&Bolo[bolo]);
				else { if(prefixe) printf("%s! Detecteur inconnu: '%s'\n",ScriptPrefixe(prefixe,debut,80),symbole); ok = 0; }
				break;
			  case HW_NUMER:
				for(bb=0; bb<NumeriseurNb; bb++) if(!strcmp(symbole,Numeriseur[bb].nom)) break;
				if(bb < NumeriseurNb) script->bloc[b].hw = (void *)(&(Numeriseur[bb]));
				else { if(prefixe) printf("%s! Numeriseur inconnu: '%s'\n",ScriptPrefixe(prefixe,debut,80),symbole); ok = 0; }
				break;
			  case HW_MODL_DET:
				for(modl=0; modl<ModeleDetNb; modl++) if(!strcmp(symbole,ModeleDet[modl].nom)) break;
				if(modl < ModeleDetNb) script->bloc[b].hw = (void *)(&ModeleDet[modl]);
				else { if(prefixe) printf("%s! Type de detecteur inconnu: '%s'\n",ScriptPrefixe(prefixe,debut,80),symbole); ok = 0; }
				break;
			  case HW_MODL_NUM:
				for(modl=0; modl<ModeleBNNb; modl++) if(!strcmp(symbole,ModeleBN[modl].nom)) break;
				if(modl < ModeleBNNb) script->bloc[b].hw = (void *)(&ModeleBN[modl]);
				else { if(prefixe) printf("%s! Type de numeriseur inconnu: '%s'\n",ScriptPrefixe(prefixe,debut,80),symbole); ok = 0; }
				break;
			  case HW_MEDIA:
				if((media = MediaListeAdrs(SambaMedia,symbole,MEDIA_WR))) script->bloc[b].hw = (void *)media;
				else { if(prefixe) printf("%s! Type de media inconnu: '%s'\n",ScriptPrefixe(prefixe,debut,80),symbole); ok = 0; }
				break;
			  default: break;
			}
		}
		if(sep == '{') {
			*bloc_ouvert = 1;
			script->bloc[script->nb_blocs - 1].invalide = !ok;
			return(1); /* et non ok, pour preserver les instrections suivantes */
		}
		if(obj != HW_GLOBAL) symbole = ItemAvant(":=",&sep,&r); // ":=+-*/" si symboles sans '-'
	} else b = *bcur;
	
	/* Decodage d'une instruction */
	script->action[script->nb_actns].blocadrs = &(script->bloc[b]);
	script->action[script->nb_actns].sautee = script->bloc[script->nb_blocs - 1].invalide;
	/* ... registre */
	if((b >= 0) && (script->bloc[b].obj == HW_MEDIA)) {
		script->action[script->nb_actns].reg.type_sym = NEANT;
		script->action[script->nb_actns].oper = ':';
	} else {
		if((script->bloc[b].obj != HW_DEFINE) && !strncmp(symbole,"0x",2)) {
			char *c;
			script->action[script->nb_actns].reg.type_sym = HW_VAL;
			c = symbole + 2;
			sscanf(ItemJusquA('-',&c),"%x",&(script->action[script->nb_actns].reg.v.i.val1));
			if(*c) sscanf(ItemJusquA(0,&c),"%x",&(script->action[script->nb_actns].reg.v.i.valN));
			else script->action[script->nb_actns].reg.v.i.valN = script->action[script->nb_actns].reg.v.i.val1;
		} else {
			script->action[script->nb_actns].reg.type_sym = HW_NOM;
			strncpy(script->action[script->nb_actns].reg.v.nom,symbole,HW_MAXNOM);
		}
		/* ... affectation */
		if(sep == ' ') ItemAvant(":=",&sep,&r); // ":=+-*/" si symboles sans '-'
		if((sep == '=') && ((*r == '+') || (*r == '-') || (*r == '*') || (*r == '/'))) sep = *r++;
		else if(((sep == '+') || (sep == '-') || (sep == '*') || (sep == '/')) && (*r == '=')) r++;
		script->action[script->nb_actns].oper = sep;
		/* ... valeur */
		symbole = ItemJusquA('#',&r);
	}
	if(symbole[0]) {
		if(script->nb_vars) {
			int num,n; n = 0;
			for(num=0; num<script->nb_actns; num++) if(script->bloc[num].obj == HW_DEFINE) {
				if(!strcmp(script->action[num].reg.v.nom,symbole)) {
					if(script->action[num].val.type_sym == HW_VAL) {
						snprintf(valeur,HW_MAXNOM,"0x%x",script->action[num].val.v.val); symbole = valeur;
					} else symbole = script->action[num].val.v.nom;
					break;
				}
				if(++n >= script->nb_vars) break;
			}
		}
		if((b >= 0) && (script->bloc[b].obj != HW_MEDIA) && !strncmp(symbole,"0x",2)) {
			script->action[script->nb_actns].val.type_sym = HW_VAL;
			sscanf(symbole+2,"%x",&(script->action[script->nb_actns].val.v.val));
		} else {
			if(script->action[script->nb_actns].reg.type_sym == HW_VAL) {
				if(prefixe) printf("%s! Impossible d'evaluer '%s' pour une ressource anonyme\n",ScriptPrefixe(prefixe,debut,80),symbole);
				return(script->action[script->nb_actns].sautee? 1: 0);
			}
			script->action[script->nb_actns].val.type_sym = HW_NOM;
			strncpy(script->action[script->nb_actns].val.v.nom,symbole,HW_MAXNOM);
		}
	} else script->action[script->nb_actns].val.type_sym = NEANT;
	script->nb_actns += 1;
	if(script->nb_blocs) script->bloc[script->nb_blocs - 1].derniere = script->nb_actns;
	// printf("(%s) %d actions retenues (on %s)\n",__func__,script->nb_actns,ok?"continue":"arrete");
	return(ok);
}
/* ========================================================================== */
static char *ScriptNomHw(HwBloc bloc) {
	char *nom;
	
	if(bloc->obj == HW_GLOBAL) return("");
	else if(bloc->hw) switch(bloc->obj) {
		case HW_DETEC: nom = ((TypeDetecteur *)(bloc->hw))->nom; break;
		case HW_NUMER: nom = ((TypeNumeriseur *)(bloc->hw))->nom; break;
		case HW_MODL_DET: nom = ((TypeDetModele *)(bloc->hw))->nom; break;
		case HW_MODL_NUM: nom = ((TypeBNModele *)(bloc->hw))->nom; break;
		case HW_MEDIA: nom = ((Media)(bloc->hw))->nom; break;
		default: return("");
	} else return("tous");
	return(nom);
}
/* ========================================================================== 
static void ScriptDump(HwScript script) {
	int i; char oper;

	printf("____________________________________\n");
	printf("  | ---- Blocs:\n");
	for(i=0; i<script->nb_blocs; i++) {
		printf("%2d| %s %s: ",i,HwCat[script->bloc[i].obj],ScriptNomHw(&(script->bloc[i])));
		printf("actions %d - %d %svalides\n",script->bloc[i].premiere,script->bloc[i].derniere-1,script->bloc[i].invalide? "in":"");
	}
	printf("  | ---- Actions:\n");
	for(i=0; i<script->nb_actns; i++) {
		printf("%2d| ",i);
		printf("%c",script->action[i].sautee? '(': ' ');
		if(script->action[i].reg.type_sym == HW_VAL) printf("0x%08X-0x%08X ",script->action[i].reg.v.i.val1,script->action[i].reg.v.i.valN);
		else printf("%s ",script->action[i].reg.v.nom);
		oper = script->action[i].oper;
		if((oper == '+') || (oper == '-') || (oper == '*') || (oper == '/')) printf("=");
		printf("%c ",oper);
		if(script->action[i].val.type_sym == HW_VAL) printf("0x%08X ",script->action[i].val.v.val);
		else printf("%s ",script->action[i].val.v.nom);
		printf("%c",script->action[i].sautee? ')': ' ');
		printf("\n");
	}
	printf("__|_________________________________\n");
}
   ========================================================================== */
static void ScriptImprime(HwScript script) {
	int i; short p; char oper;

	/* { char mode; mode = ScriptModeExec; ScriptModeExec = 0; ScriptRun(ScriptEnCours,"\t"); ScriptModeExec = mode; } */
	printf(" _____________________________________\n");
	for(i=0; i<script->nb_blocs; i++) {
		printf("| %2d.-- | %s %s {\n",i+1,HwCat[script->bloc[i].obj],ScriptNomHw(&(script->bloc[i])));
		for(p=script->bloc[i].premiere; p<script->bloc[i].derniere; p++) {
			printf("|   .%-2d |     ",p+1);
			printf("%c",script->action[p].sautee? '(': ' ');
			if(script->action[p].reg.type_sym == HW_VAL) printf("0x%08X-0x%08X ",script->action[p].reg.v.i.val1,script->action[p].reg.v.i.valN);
			else printf("%s ",script->action[p].reg.v.nom);
			oper = script->action[p].oper;
			if((oper == '+') || (oper == '-') || (oper == '*') || (oper == '/')) printf("=");
			printf("%c ",oper);
			if(script->action[p].val.type_sym == HW_VAL) printf("0x%08X ",script->action[p].val.v.val);
			else printf("%s ",script->action[p].val.v.nom);
			printf("%c",script->action[p].sautee? ')': ' ');
			printf("\n");
		}
		printf("|       | }\n");
	}
	printf("|_______|_____  %02d instruction%s ______\n",Accord1s(script->nb_actns));
}
/* ========================================================================== */
static void ScriptDAC(char *nom, TypeNumeriseur *numeriseur, TypeBNModele *modele_bn, int k, char *entete, char *marge) {
	if(!ScriptModeExec || ScriptKill) return;
	if(!strcmp(nom,"RAZ_DAC")) {
		if(modele_bn->ress[k].type == RESS_FLOAT) numeriseur->ress[k].val.r = 0.0;
		else numeriseur->ress[k].val.i = 0;
		NumeriseurRessValChangee(numeriseur,k);
	} else if(!strcmp(nom,"load_DAC")) {
		if(!numeriseur->ress[k].init) return;
	} else return;
	NumeriseurRessourceChargeAuto(numeriseur,k,entete,marge);
}
/* ========================================================================== */
static INLINE char ScriptLimiteAtteinte(TypeNumeriseur *numeriseur, int k, char type, char limite, int ilim, float rlim) {
	char atteinte;

	if(limite > 0) {
		//- printf("(%g <= %g)?",numeriseur->ress[k].val.r,rlim);
		if(type == RESS_FLOAT) atteinte = (numeriseur->ress[k].val.r <= rlim);
		else atteinte = (numeriseur->ress[k].val.i <= ilim);
	} else if(limite < 0) {
		//- printf("(%g >= %g)?",numeriseur->ress[k].val.r,rlim);
		if(type == RESS_FLOAT) atteinte = (numeriseur->ress[k].val.r >= rlim);
		else atteinte = (numeriseur->ress[k].val.i >= ilim);
    } else atteinte = 0;
	//- printf("%s,",atteinte?"oui":"non");
	return(atteinte);
}
/* ========================================================================== */
static INLINE char ScriptChargeRessource(TypeHwInstr *instr, 
	TypeNumeriseur *numeriseur, int k, char *suite, char *marge) {
	/* charge une ressource de numeriseur */
	TypeBNModele *modele_bn; TypeBNModlRessource *ress; char valeur[HW_MAXNOM];
	int ival,ilim; float rval,rlim; shex hval;
	char *r,*s,sep; char limite,atteinte;
	
	if(ScriptKill) return(0);
	if(!ScriptModeExec) { if(suite && (strlen(suite) < 2)) printf("\n"); return(0); }
	hval = 0;
	modele_bn = &(ModeleBN[numeriseur->def.type]); ress = modele_bn->ress + k;
	
	limite = 0; atteinte = 1;
	if(instr->val.type_sym == HW_NOM) {
		strcpy(valeur,instr->val.v.nom);
		r = valeur;
		ItemAvant("<>",&sep,&r);
		if(sep) {
			s = ItemJusquA('\0',&r);
			if(!strcmp(s,"standard")) {
				if((numeriseur->ress[k].regl))
					hval = NumeriseurRessUserDecode(numeriseur,k,(numeriseur->ress[k].regl)->std,&ilim,&rlim);
				else {
					printf("%s !!! Reglage associe non defini, valeur inconnue\n",suite);
					return(0);
				}
			} else if(!strcmp(s,"consigne")) {
				if((numeriseur->ress[k].regl))
					hval = NumeriseurRessUserDecode(numeriseur,k,(numeriseur->ress[k].regl)->user,&ilim,&rlim);
				else {
					printf("%s !!! Reglage associe non defini, valeur inconnue\n",suite);
					return(0);
				}
			} else hval = NumeriseurRessUserDecode(numeriseur,k,s,&ilim,&rlim);
			if(!instr->limitee) {
				hval = NumeriseurRessCurrentVal(numeriseur,k,&ival,&rval);
				if(ress->type == RESS_FLOAT) instr->limitee = (rval > rlim)? -1: 1;
				else instr->limitee = (ival > ilim)? -1: 1;
			}
			limite = instr->limitee;
		}
	}
	// printf("(%s) %s %c= %s %c %g:",__func__,ress->nom,instr->oper,valeur,(limite>0)?'>':((limite<0)?'<':'='),numeriseur->ress[k].val.r);
	
	if(instr->oper == '=') {
		if(instr->val.type_sym == HW_NOM) {
			if(strcmp(valeur,"standard") && strcmp(valeur,"consigne")) {
				strncpy(numeriseur->ress[k].cval,valeur,NUMER_RESS_VALTXT);
				NumeriseurRessUserChange(numeriseur,k);
			}
		} else /* (instr->val.type_sym == HW_VAL) */ {
			numeriseur->ress[k].hval = (shex)(instr->val.v.val);
			NumeriseurRessHexaChange(numeriseur,k);
		}
		NumeriseurRessourceChargeAuto(numeriseur,k,suite,marge);
	} else {
		shex mval; int ival; float rval; char *cval; char texte[80],prec[80]; // NUMER_RESS_VALTXT
		char non_std,avec_oper;
		mval = 0;
		non_std = 1; avec_oper = 0;
		if(instr->val.type_sym == HW_NOM) {
			if(strcmp(valeur,"standard") && strcmp(valeur,"consigne")) {
				if((instr->oper == '+') || (instr->oper == '-')) {
					if(limite && ScriptLimiteAtteinte(numeriseur,k,ress->type,limite,ilim,rlim)) {
						NumeriseurRessHexaEncode(numeriseur,k,hval,valeur,0,0);
						printf("%s *limite deja atteinte*      %8s: %13s = %s %s\n",suite,numeriseur->nom,ress->nom,valeur,ress->unite);
						return(2);
					}
					mval = numeriseur->ress[k].hval; strcpy(prec,numeriseur->ress[k].cval);
					hval = NumeriseurRessUserDecode(numeriseur,k,valeur,&ival,&rval);
					if(limite > 0) { if(ival < 0) { ival = -ival; rval = -rval; } }
					else if(limite < 0) { if(ival > 0) { ival = -ival; rval = -rval; } }
					else if(instr->oper == '-') { rval = -rval; ival = -ival; }
					if(ress->type == RESS_FLOAT) numeriseur->ress[k].val.r += rval;
					else numeriseur->ress[k].val.i += ival;
					avec_oper = 1;
				} else if((instr->oper == '*') || (instr->oper == '/')) {
					if(limite && ScriptLimiteAtteinte(numeriseur,k,ress->type,limite,ilim,rlim)) {
						NumeriseurRessHexaEncode(numeriseur,k,hval,valeur,0,0);
						printf("%s *limite deja atteinte*      %8s: %13s = %s %s\n",suite,numeriseur->nom,ress->nom,valeur,ress->unite);
						return(2);
					}
					mval = numeriseur->ress[k].hval; strcpy(prec,numeriseur->ress[k].cval);
					sscanf(valeur,"%g",&rval);
					if(limite > 0) { if(rval < 0.0) rval = -rval; }
					else if(limite < 0) { if(rval > 0.0) rval = -rval; }
					if(instr->oper == '*') {
						if(ress->type == RESS_FLOAT) numeriseur->ress[k].val.r *= rval;
						else numeriseur->ress[k].val.i = (int)((float)(numeriseur->ress[k].val.i) * rval);
					} else if(rval != 0.0) {
						if(ress->type == RESS_FLOAT) numeriseur->ress[k].val.r /= rval;
						else numeriseur->ress[k].val.i = (int)((float)(numeriseur->ress[k].val.i) / rval);
					}
					avec_oper = 1;
				} else {
					hval = NumeriseurRessUserDecode(numeriseur,k,valeur,&ival,&rval);
					cval = valeur;
				}
			} else non_std = 0;
		} else /* (instr->val.type_sym == HW_VAL) */ {
			hval = (shex)(instr->val.v.val);
			NumeriseurRessHexaEncode(numeriseur,k,hval,texte,0,0);
			cval = texte;
		}
		if(avec_oper) {
			if(limite && (atteinte = ScriptLimiteAtteinte(numeriseur,k,ress->type,limite,ilim,rlim))) {
				if(ress->type == RESS_FLOAT) numeriseur->ress[k].val.r = rlim;
				else numeriseur->ress[k].val.i = ilim;
			}
			NumeriseurRessValChangee(numeriseur,k);
		} else if(non_std && !avec_oper) {
			mval = numeriseur->ress[k].hval; strcpy(prec,numeriseur->ress[k].cval);
			numeriseur->ress[k].hval = hval; strcpy(numeriseur->ress[k].cval,cval);
		}
		NumeriseurRessourceChargeAuto(numeriseur,k,suite,marge);
		if(non_std || !avec_oper) { numeriseur->ress[k].hval = mval; strcpy(numeriseur->ress[k].cval,prec); }
	}
	ReglageEffetDeBord(numeriseur->ress[k].regl,0);

	//- printf("rc=%d/",atteinte+1);
	return(atteinte+1);
}
/* ========================================================================== */
static char ScriptAffecteReglage(TypeHwInstr *instr, TypeDetecteur *detectr, int num, 
	char imprime, char *suite, char *marge) {
	/* affecte une valeur a un reglage de detecteur */
	TypeDetModele *modele_det; TypeReglage *regl;
	TypeNumeriseur *numeriseur; TypeBNModele *modele_bn; int k,cap,bb; char rc;
	shex mval; char prec[80]; // NUMER_RESS_VALTXT

	mval = 0;
	rc = 0;
	if(ScriptKill) return(0);
	modele_det = &(ModeleDet[detectr->type]);
	regl = &(detectr->reglage[num]); 
	cap = regl->capt; if(cap < 0) cap = 0;
	if((bb = detectr->captr[cap].bb.num) < 0) {
		if(imprime) printf("%s ! Le capteur '%s' de %s n'est pas connecte a un numeriseur\n",suite,ModeleVoie[detectr->captr[cap].modele].nom,detectr->nom);
		return(0);
	}
	numeriseur = &(Numeriseur[bb]); k = regl->ress;
	modele_bn = &(ModeleBN[numeriseur->def.type]);
	if((k < 0) || (k >= modele_bn->nbress)) {
		if(imprime) printf("%s ! Le reglage '%s' de %s n'a pas de ressource numeriseur associee\n",suite,modele_det->regl[num].nom,detectr->nom);
	} else if(modele_bn->ress[k].ssadrs <= 0) {
		if(imprime) printf("%s ! Le reglage '%s' de %s ne peut pas etre modifie (ressource '%s' en lecture seule)\n",
			suite,modele_det->regl[num].nom,detectr->nom,modele_bn->ress[k].nom);
	} else {
		if((instr->oper != '=') && (instr->oper != ':')) 
			rc = ScriptChargeRessource(instr,numeriseur,k,suite,marge);
		else {
			if(instr->oper == ':') { mval = numeriseur->ress[k].hval; strcpy(prec,numeriseur->ress[k].cval); }
			if((instr->val.type_sym == HW_NOM) && !strcmp(instr->val.v.nom,"standard")) {
				strncpy(numeriseur->ress[k].cval,regl->std,NUMER_RESS_VALTXT);
				NumeriseurRessUserChange(numeriseur,k);
				NumeriseurRessourceChargeAuto(numeriseur,k,suite,marge);
			} else if((instr->val.type_sym == HW_NOM) && !strcmp(instr->val.v.nom,"consigne")) {
				strncpy(numeriseur->ress[k].cval,regl->user,NUMER_RESS_VALTXT);
				NumeriseurRessUserChange(numeriseur,k);
				NumeriseurRessourceChargeAuto(numeriseur,k,suite,marge);
			} else if(regl->cmde == REGL_RAZFET) {
				char valeurs[256]; char *r,*off,*on,*cval; char raz;
				if(regl->valeurs) { strncpy(valeurs,regl->valeurs,256); valeurs[255] = '\0'; } 
				else valeurs[0] = '\0';
				if(instr->val.type_sym == HW_NOM)
					raz = (!strcmp(instr->val.v.nom,"on")
						   || !strcmp(instr->val.v.nom,"oui")
						   || !strcmp(instr->val.v.nom,"start")
						   || !strcmp(instr->val.v.nom,"masse")
						   || !strcmp(instr->val.v.nom,"actif"));
				else raz = ((shex)(instr->val.v.val) > 0);
				if(ScriptModeExec) {
					if(valeurs[0]) {
						r = valeurs; off = ItemJusquA('/',&r);
						if(raz) { on = ItemJusquA('/',&r); r = on; } else r = off;
						while(*r) {
							cval = ItemJusquA(',',&r);
							if(*cval) {
								strncpy(numeriseur->ress[k].cval,cval,NUMER_RESS_VALTXT);
								NumeriseurRessUserChange(numeriseur,k);
								NumeriseurRessourceChargeAuto(numeriseur,k,suite,marge);
							}
						}
					} else {
						// numeriseur->ress[k].val.i = raz;
						// NumeriseurRessValChangee(numeriseur,k);
						strncpy(numeriseur->ress[k].cval,instr->val.v.nom,NUMER_RESS_VALTXT);
						NumeriseurRessUserChange(numeriseur,k);
						NumeriseurRessourceChargeAuto(numeriseur,k,suite,marge);
					}
					detectr->razfet_en_cours = raz;
				} else if(suite && (strlen(suite) < 2)) printf("\n");
			} else {
				ScriptChargeRessource(instr,numeriseur,k,suite,marge);
				if(instr->oper == '=') /* mettre a jour la consigne */ {
					if(modele_bn->ress[k].type == RESS_FLOAT) regl->val.r = numeriseur->ress[k].val.r;
					else regl->val.i = numeriseur->ress[k].val.i;
					strcpy(regl->user,numeriseur->ress[k].cval);
				}
			}
			if(instr->oper == ':') { numeriseur->ress[k].hval = mval; strcpy(numeriseur->ress[k].cval,prec); }
			rc = 2;
		}
	}
	return(rc);
}
/* ========================================================================== */
char ScriptExecBatch(HwInstr prgm, HW_OBJ obj, void *hw, HwExecContext pc, short derniere, short *secs, char *prefixe) {
	HwInstr instr; void *hwcur; HW_OBJ objcur;
	TypeDetecteur *detectr; TypeNumeriseur *numeriseur;
	TypeDetModele *modele_det; TypeBNModele *modele_bn; Media media;
	int num,k,fmt; short ligne;
	char *suite,saut[4],nom[MAXFILENAME],debut[80];
	int bb,bolo,cap,categ,num_regl,det; char *symbole,*rsce,*cpt,*dlm;
	int i,j,l; int rc; int ival; // shex hval;
	char imprime,trouve,pas_decode;

	if(secs) *secs = 0;
	imprime = (prefixe != 0);
	if((obj != HW_GLOBAL) && !hw) {
		if(imprime) printf("%s! Materiel indefini (erreur systeme)\n",ScriptPrefixe(prefixe,debut,80));
		return(0);
	}
	k = 0; media = 0; pas_decode = 1; // GCC
	hwcur = hw; objcur = obj;
	detectr = 0; numeriseur = 0; modele_det = 0; modele_bn = 0;
	while(pc->inst < derniere) {
		det = -1;
		instr = &(prgm[pc->inst]); ligne = pc->inst + 1;
		pc->inst += 1;
		switch(objcur) {
			case HW_DETEC:    detectr    = (TypeDetecteur *)hwcur;  break;
			case HW_NUMER:    numeriseur = (TypeNumeriseur *)hwcur; break;
			case HW_MODL_DET: modele_det = (TypeDetModele *)hwcur;  break;
			case HW_MODL_NUM: modele_bn  = (TypeBNModele *)hwcur;   break;
			case HW_MEDIA:    media      = (Media)hwcur;            break;
			default: break;
		}
		num_regl = -1;
		if(imprime) {
			l = printf("%s%2d] ",ScriptPrefixe(prefixe,debut,80),ligne);
			if(hwcur) switch(objcur) {
			  case HW_DETEC:    l += printf(detectr->nom);    break;
			  case HW_NUMER:    l += printf(numeriseur->nom); break;
			  case HW_MODL_DET: l += printf("<%s>",modele_det->nom); break;
			  case HW_MODL_NUM: l += printf("<%s>",modele_bn->nom);  break;
			  case HW_MEDIA:    l += printf("%s",media->nom);  break;
			  default: break;
			} //- else l += printf("[global]");
			if(instr->reg.type_sym == HW_NOM) {
				if(hwcur) l += printf(".%-16s",instr->reg.v.nom);
				else l += printf("%-16s",instr->reg.v.nom);
			} else if(instr->reg.type_sym == HW_VAL) {
				if(instr->reg.v.i.valN > instr->reg.v.i.val1) {
					if(hwcur) l += printf(".@%04X-%04X      ",instr->reg.v.i.val1,instr->reg.v.i.valN);
					else l += printf(" @%04X-%04X      ",instr->reg.v.i.val1,instr->reg.v.i.valN);
				} else {
					if(hwcur) l += printf(".@%04X           ",instr->reg.v.i.val1);
					else l += printf(" @%04X           ",instr->reg.v.i.val1);
				}
			}
			if(instr->val.type_sym != NEANT) {
				if((instr->oper == '+') || (instr->oper == '-') || (instr->oper == '*') || (instr->oper == '/')) {
					l += printf(" =%c",instr->oper);
					if(instr->val.type_sym == HW_NOM) l += printf(" %-8s",instr->val.v.nom);
					else l += printf(" 0x%04X  ",instr->val.v.val);
				} else {
					l += printf(" %c",instr->oper);
					if(instr->val.type_sym == HW_NOM) l += printf(" %-9s",instr->val.v.nom);
					else l += printf(" 0x%04X   ",instr->val.v.val);
				}
			} else l+= printf("            ");
			for(j=0; j<l; j++) ScriptSuite[j] = ' '; ScriptSuite[l++] = '|'; ScriptSuite[l] = '\0';
			strcpy(saut,"|");
		} else {
			l = 0; ScriptSuite[l] = '\0'; strcpy(saut," ");
		}
		if(imprime) suite = saut; else suite = 0;
		if(instr->sautee) {
			if(imprime) printf("%s ! Instruction desactivee\n",suite);
			return(1);
		}
	/*
	 objet           syntaxe                       variables produites    fonction
	 -----           -------                       -------------------    --------
	 detecteur       numeriseur(capteur).0xadrs    numeriseur             (special)
					 numeriseur(capteur).<categ>   numeriseur,categ       ChargeRessource
					 numeriseur(capteur).ressource numeriseur,k           ChargeRessource
					 <categ>                       numeriseur=0           AffecteReglage
					 reglage                       num_regl               AffecteReglage
	 modele_det      <categ>                       numeriseur=0           AffecteReglage
					 reglage                       num_regl               AffecteReglage
	 
	 numeriseur      <categ>                       numeriseur,categ       ChargeRessource
					 ressource                     numeriseur,k           ChargeRessource
	 modele_num      <categ>                       numeriseur,categ       ChargeRessource
					 ressource                     numeriseur,k           ChargeRessource
	 
	 ChargeRessource [oriente numeriseur]: instr->val  => numeriseur.ress[k]
	 AffecteReglage  [oriente detecteur]:  si standard, detectr->regl[num] => numeriseur.ress[k] + NumeriseurRessourceChargeAuto
										   sinon: ChargeRessource + (si !direct, numeriseur.ress[k] => detectr->regl[num])
		  > si num_regl n'est pas evalue [exemple: numeriseur(capteur)], ChargeRessource est appele (donc 'standard' pas applicable au detecteur)
	 */
		categ = (objcur == HW_MEDIA)? 0: -1;
		if(instr->reg.type_sym == HW_NOM) {
			if(instr->val.type_sym == HW_NOM) {
				if(!strcmp(instr->val.v.nom,"indefinie") || !strcmp(instr->val.v.nom,"infinite")) ival = -1;
				else { ival = 1; sscanf(instr->val.v.nom,"%d",&ival); }
			} else ival = instr->val.v.val;
			if(!strcmp(instr->reg.v.nom,"boucle") || !strcmp(instr->reg.v.nom,"loop")) {
				if(ScriptModeExec) ScriptBoucleNouvelle(&(pc->boucle),pc->inst,objcur,hwcur,ival);
				if(imprime) {
					if(!strcmp(instr->val.v.nom,"indefinie")) 
						printf("%s Boucle@%d indefinie\n",suite,ligne);
					else printf("%s Boucle@%d, %d fois\n",suite,ligne,ival);
				}
				continue;
			} else if(!strcmp(instr->reg.v.nom,"recommence") || !strcmp(instr->reg.v.nom,"endloop")) {
				if(ScriptModeExec) {
					// if(imprime) printf("%s Fin de boucle [%08X.boucle=%08X]\n",suite,(hexa)hwcur,(hexa)pc->boucle);
					if(pc->boucle == 0) { printf("%s Pas de boucle en cours\n",suite); return(0); }
					if(((pc->boucle)->limitee && ((pc->boucle)->decompte > 1)) || (pc->boucle)->en_cours) {
						pc->inst = (pc->boucle)->debut;
						hwcur = (pc->boucle)->hw;
						objcur = (pc->boucle)->obj;
						if((pc->boucle)->limitee) {
							(pc->boucle)->decompte -= 1;
							if(imprime) printf("%s Retour a la ligne %d (reste %d execution%s)\n",suite,(pc->inst)+1,Accord1s((pc->boucle)->decompte));
						} else {
							(pc->boucle)->en_cours = 0;
							if(imprime) printf("%s Retour a la ligne %d\n",suite,(pc->inst)+1);
						}
					} else { if(imprime) printf("%s Boucle@%d terminee\n",suite,(pc->boucle)->debut); ScriptBoucleTermine(&(pc->boucle)); }
				} else if(imprime) printf("%s Boucle@%d terminee\n",suite,ligne);
				continue;
			} else if(!strcmp(instr->reg.v.nom,"acquisition")) {
				if(!Trigger.demande) {
					if(imprime) {
						printf("%s Passage du mode 'stream' a 'events'",suite);
						if(Acquis[AcquisLocale].etat.active) printf(", arret de l'acquisition\n",suite); else printf("\n");
					}
					LectStreamNum++;
					ArchiveOuverte[EVTS] = 0; ArchEcrites[EVTS] = 0;
					Acquis[AcquisLocale].etat.active = 0;
					Trigger.demande = 1;
				}
				if(ScriptModeExec) {
					if(imprime) printf("%s Regeneration actuellement %s, on %s\n",suite,
						RegenEnCours?"en cours":"arretee",RegenEnCours?"l'arrete":"continue comme ca");
					if(RegenEnCours) LectRegenStoppe(0);
				}
				gettimeofday(&LectDateRun,0);
				ScriptDateLimite = LectDateRun.tv_sec + (long)ival;
				if(ScriptModeExec) LectAcqStd();
				ScriptDateLimite = 0;
				continue;
			} else if(!strcmp(instr->reg.v.nom,"stream")) {
				if(Trigger.demande) {
					if(imprime) {
						printf("%s Passage du mode 'events' a 'stream'",suite);
						if(Acquis[AcquisLocale].etat.active) printf(", arret de l'acquisition\n"); else printf("\n");
					}
					if(ArchEcrites[EVTS]) { if(RegenEnCours) LectRegenNum++; else (LectTrancheNum[EVTS])++; }
					ArchiveOuverte[STRM] = 0; ArchEcrites[STRM] = 0;
					Acquis[AcquisLocale].etat.active = 0;
					Trigger.demande = 0;
				}
				gettimeofday(&LectDateRun,0);
				ScriptDateLimite = LectDateRun.tv_sec + (long)ival;
				if(ScriptModeExec) LectAcqStd();
				ScriptDateLimite = 0;
				continue;
			} else if(!strcmp(instr->reg.v.nom,"wait")) {
				if(imprime) printf("%s Attente: %d s\n",suite,ival);
				if(secs) *secs = ival; else if(ScriptModeExec) {
					if(OpiumDisplayed(mScriptIntr->cdr)) MenuItemAllume(mScriptIntr,1,0,GRF_RGB_YELLOW);
					if(ScriptDuree <= 0) ScriptDuree = ival;
					ScriptWait = ival; while(ScriptWait > 0) {
						if(OpiumDisplayed(pScriptCntl->cdr)) PanelRefreshVars(pScriptCntl);
						if(OpiumDisplayed(pScriptWait->cdr)) PanelRefreshVars(pScriptWait);
	//					if(OpiumDisplayed(BancProgression->cdr)) {
	//						*BancPrgs = 100 - ((ScriptWait * 100) / ival);
	//						InstrumRefreshVar(BancProgression);
	//					}
						if(Acquis[AcquisLocale].etat.active) {
							int64 nummodul; int intervalle,nb; 
							intervalle = 50000;
							nb = 1000000 / intervalle;
							nummodul = 0;
							while(nb--) {
								// LectDepileDonnees(); 
								usleep(intervalle); 
						#ifdef REFRESH_V1
								if(nummodul != SynchroD2lues) {
									nummodul = SynchroD2lues;
									if(!LectTrmtSousIt) LectTraiteStocke();
									//- printf("(%s) %d synchroD2 lues, prochain affichage a %d (tous les %d)\n",__func__,(int)SynchroD2lues,(int)LectNextDisp,(int)LectIntervData);
									LectActionUtilisateur();
								} else {
									// LectDisplay();
									if(ReglagesEtatDemande) {
										PanelRefreshVars(pReglagesConsignes[LECT_CONS_ETAT]);
										PanelRefreshVars(pEtatDetecteurs);
									}
									if(NumeriseurEtatDemande) BoardRefreshVars(bNumeriseurEtat);
									OpiumUserAction();
								}
						#endif
							}
						#ifndef REFRESH_V1
							if(NumeriseurEtatDemande || ReglagesEtatDemande) {
								for(bb=0; bb<NumeriseurNb; bb++) {
									if(Numeriseur[bb].status.a_copier) {
										for(i=0; i<Numeriseur[bb].status.nb; i++) NumeriseurRecopieStatus(&(Numeriseur[bb]),i,Numeriseur[bb].status_val[i]);
										if(Numeriseur[bb].status.demande) {
											if(NumeriseurEtatDemande) NumeriseurRapportEtabli(&(Numeriseur[bb]));
											NumeriseurRafraichi(&(Numeriseur[bb]));
										}
									}
								}
								if(NumeriseurEtatDemande) BoardRefreshVars(bNumeriseurEtat);
								if(ReglagesEtatDemande) {
									PanelRefreshVars(pReglagesConsignes[LECT_CONS_ETAT]);
									PanelRefreshVars(pEtatDetecteurs);
								}
							}
							OpiumUserAction();
						#endif
						} else { OpiumUserAction(); sleep(1); }
						ScriptWait--; ScriptDuree--;
					}
					ScriptWait = 0;
					if(OpiumDisplayed(pScriptCntl->cdr)) PanelRefreshVars(pScriptCntl);
					if(OpiumDisplayed(pScriptWait->cdr)) PanelRefreshVars(pScriptWait);
	//				if(OpiumDisplayed(BancProgression->cdr)) {
	//					*BancPrgs = 100;
	//					InstrumRefreshVar(BancProgression);
	//				}
					// if(OpiumDisplayed(mScriptIntr->cdr)) MenuItemEteint(mScriptIntr,1,0);
				}
				if(imprime) suite = ScriptSuite;
				return(1);
			} else if(!strcmp(instr->reg.v.nom,"trigger")) /* 'suspendu' | 'actif' */ {
				if(instr->val.type_sym == HW_NOM) 
					Trigger.actif = ((instr->val.v.nom[0] == 'a') || (instr->val.v.nom[0] == 'o') || (instr->val.v.nom[0] == 'y'));
				else Trigger.actif = (char)(instr->val.v.val);
				if(imprime) printf("\n");
				continue;
			} else if(!strcmp(instr->reg.v.nom,"sources")) /* 'hs' | 'actives' */ {
				char descendre;
				if(instr->val.type_sym == HW_NOM) 
					descendre = ((instr->val.v.nom[0] == 'a') || (instr->val.v.nom[0] == 'o') || (instr->val.v.nom[0] == 'y'));
				else descendre = (char)(instr->val.v.val);
				LectSourceActive = AutomSourceDeplace(AUTOM_CALIB,descendre,1,suite);
				if(imprime) printf("\n");
				continue;
			} else if(!strcmp(instr->reg.v.nom,"mode-regen")) /* 'oui' | 'non' */ {
				char regen;
				if(imprime) printf("\n");
				strcpy(RegenDebutJour,DateCivile()); strcpy(RegenDebutHeure,DateHeure());
				if(instr->val.type_sym == HW_NOM) regen = (instr->val.v.nom[0] == 'o');
				else regen = (char)(instr->val.v.val);
				if(ScriptModeExec) LectRegenTermine(regen);
				continue;
			} else if(!strcmp(instr->reg.v.nom,"regul_clock")) {
				if(imprime) printf("%s Horloge regulation DACs: %d synchro%s D3\n",suite,Accord1s(ival));
				if(ScriptModeExec) TrgrRegulClock = ival;
				continue;
			} else if(!strcmp(instr->reg.v.nom,"compensation")) {
				if(imprime) printf("\n");
				if(ScriptModeExec) LectCompenseTout();
				continue;
			} else if(!strcmp(instr->reg.v.nom,"partition")) {
				if(imprime) printf("\n");
				if(ScriptModeExec) for(fmt=0; fmt<ARCH_TYPEDATA; fmt++) ArchiveNouvellePartition(fmt);
				continue;
			} else if(!strcmp(instr->reg.v.nom,"acces")) /* 'ouvert' | 'ferme' */ {
				char autorise;
				autorise = (instr->val.v.nom[0] == 'o');
				if(objcur == HW_NUMER) {
					if(ScriptModeExec) {
						NumeriseurAutoriseAcces(numeriseur,autorise,0);
						if(imprime) printf("%s %s | %8s: acces %s\n",suite,
							RepartiteurValeurEnvoyee,numeriseur->nom,autorise? "ouvert":"ferme");
					}
				} else if(objcur == HW_MODL_NUM) {
					if(ScriptModeExec) for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].def.type == modele_bn->num) {
						NumeriseurAutoriseAcces(&(Numeriseur[bb]),autorise,0);
						if(imprime) {
							printf("%s %s | %8s: acces %s\n",suite,
								RepartiteurValeurEnvoyee,Numeriseur[bb].nom,autorise? "ouvert":"ferme");
							suite = ScriptSuite;
						}
					}
				} else {
					if(imprime) printf("%s ! fonction non programmee pour un %s\n",suite,HwCat[objcur]);
					return(0);
				}
				if(imprime && !ScriptModeExec) printf("\n");
				continue;
			} else if(!strcmp(instr->reg.v.nom,"RAZ_DAC") || !strcmp(instr->reg.v.nom,"load_DAC")) {
				if(objcur == HW_NUMER) {
					if(imprime) printf("%s Appele: %s(%s)\n",suite,instr->reg.v.nom,numeriseur->nom);
					modele_bn = &(ModeleBN[numeriseur->def.type]);
				} else if(objcur == HW_MODL_NUM) {
					if(imprime) printf("%s Appele: %s({%s})\n",suite,instr->reg.v.nom,modele_bn->nom);
				} else {
					if(imprime) printf("%s ! fonction non programmee pour un %s\n",suite,HwCat[objcur]);
					return(0);
				}
				if(!ScriptModeExec) continue;
				for(k=0; k<modele_bn->nbress; k++) 
					if((modele_bn->ress[k].categ == RESS_MODUL) 
					|| (modele_bn->ress[k].categ == RESS_COMP)
					|| (modele_bn->ress[k].categ == RESS_CORR)
					|| (modele_bn->ress[k].categ == RESS_AUXI)) {
						if(objcur == HW_NUMER) ScriptDAC(instr->reg.v.nom,numeriseur,modele_bn,k,ScriptPrefixe(prefixe,debut,80),ScriptSuite);
						else {
							for(bb=0; bb<NumeriseurNb; bb++) if(&(ModeleBN[Numeriseur[bb].def.type]) == modele_bn) {
								ScriptDAC(instr->reg.v.nom,&(Numeriseur[bb]),modele_bn,k,ScriptPrefixe(prefixe,debut,80),ScriptSuite);
							}
						}
					}
				if(imprime) printf("%s ......  %s termine\n",suite,instr->reg.v.nom);
				continue;
			} else {
				symbole = 0;
				switch(objcur) {
				  case HW_DETEC: case HW_MODL_DET:
					strcpy(nom,instr->reg.v.nom);
					if((rsce = index(nom,'.'))) {
						*rsce++ = '\0';
						if(objcur == HW_MODL_DET) {
							// if(imprime) printf("%s ! Syntaxe inutilisable pour les modeles de detecteurs (pour l'instant)\n",suite); return(0);
							while(++det < BoloNb) if(Bolo[bolo].a_lire && (Bolo[det].type == modele_det->num)) break;
							if(det < BoloNb) detectr = &(Bolo[det]);
							else {
								if(imprime) printf("%s ! Type de detecteur inutilise: '%s'\n",suite,modele_det->nom);
								return(0);
							}
						}
						if(!strncmp(nom,"numeriseur",10)) {
							cpt = nom; ItemJusquA('(',&cpt);
							dlm = cpt; ItemJusquA(')',&dlm);
							for(cap=0; cap<detectr->nbcapt; cap++) if(!strcmp(cpt,ModeleVoie[detectr->captr[cap].modele].nom)) break;
							if(cap < detectr->nbcapt) {
								bb = detectr->captr[cap].bb.num;
								if((bb < 0) || (bb >= NumeriseurNb)) {
									if(imprime) printf("%s ! Le capteur '%s' du detecteur %s n'est pas connecte\n",suite,cpt,detectr->nom);
									return(0);
								}
							} else {
								if(imprime) printf("%s ! Capteur '%s' inconnu pour le detecteur %s\n",suite,cpt,detectr->nom);
								return(0);
							}
						} else {
							if(objcur == HW_MODL_DET) {
								if(imprime) printf("%s ! Syntaxe inappropriee avec un modele de detecteurs\n",suite);
								return(0);
							}
							for(bb=0; bb<NumeriseurNb; bb++) if(!strcmp(nom,Numeriseur[bb].nom)) break;
							if(bb >= NumeriseurNb) {
								if(imprime) printf("%s ! Numeriseur inconnu: '%s'\n",suite,nom);
								return(0);
							}
						}
						numeriseur = &(Numeriseur[bb]);
						if(!strncmp(rsce,"0x",2)) {
							if(objcur == HW_MODL_DET) {
								if(imprime) printf("%s ! Syntaxe actuellement inutilisable avec un modele de detecteurs\n",suite);
								return(0);
							}
							dlm = rsce + 2;
							sscanf(ItemJusquA('-',&dlm),"%x",&(instr->reg.v.i.val1));
							if(*dlm) sscanf(ItemJusquA(0,&dlm),"%x",&(instr->reg.v.i.valN));
							else instr->reg.v.i.valN = instr->reg.v.i.val1;
							instr->reg.type_sym = HW_VAL;
						} else {
							modele_bn = &(ModeleBN[numeriseur->def.type]);
							if(*rsce == '<') {
								symbole = rsce + 1;
								if((dlm = index(symbole,'>'))) *dlm++ = '\0';
								for(categ=0; categ<RESS_NBCATEG; categ++) if(!strcmp(symbole,RessCategListe[categ])) break;
								if(categ >= RESS_NBCATEG) {
									if(imprime) printf("%s ! Categorie de ressource: '%s' inconnue\n",suite,symbole);
									categ = -1;
									return(0);
								}
							} else {
								for(k=0; k<modele_bn->nbress; k++) if(!strcmp(rsce,modele_bn->ress[k].nom)) break;
								if(k >= modele_bn->nbress) {
									if(imprime) printf("%s ! Ressource %s.%s inconnue\n",suite,numeriseur->nom,rsce);
									return(0);
								}
								//							adrs = modele_bn->ress[k].ssadrs;
							}
						}
					} else {
						if(objcur == HW_DETEC) modele_det = &(ModeleDet[detectr->type]);
						numeriseur = 0;
						if(*nom == '<') {
							symbole = nom + 1;
							if((dlm = index(symbole,'>'))) *dlm++ = '\0';
							for(categ=0; categ<REGL_NBTYPES; categ++) if(!strcmp(symbole,ReglCmdeListe[categ])) break;
							if(categ >= REGL_NBTYPES) {
								if(imprime) printf("%s ! Categorie de reglage: '%s' inconnue\n",suite,symbole);
								categ = -1;
								return(0);
							}
						} else {
							for(num=0; num<modele_det->nbregl; num++) if(!strcmp(nom,modele_det->regl[num].nom)) break;
							if(num < modele_det->nbregl) num_regl = num;
							else {
								if(imprime) printf("%s ! Un detecteur de type %s n'a pas de reglage '%s'\n",suite,modele_det->nom,nom);
								return(0);
							}
						}
					}
					break;
				  case HW_NUMER: case HW_MODL_NUM:
					if(instr->reg.v.nom[0] == '<') {
						strcpy(nom,instr->reg.v.nom);
						symbole = nom + 1;
						if((dlm = index(symbole,'>'))) *dlm++ = '\0';
						for(categ=0; categ<RESS_NBCATEG; categ++) if(!strcmp(symbole,RessCategListe[categ])) break;
						if(categ >= RESS_NBCATEG) {
							if(imprime) printf("%s ! Categorie de ressource: '%s' inconnue\n",suite,symbole);
							categ = -1;
							return(0);
						}
					} else {
						if(objcur == HW_NUMER) modele_bn = &(ModeleBN[numeriseur->def.type]);
						for(k=0; k<modele_bn->nbress; k++) if(!strcmp(instr->reg.v.nom,modele_bn->ress[k].nom)) break;
						if(k >= modele_bn->nbress) {
							if(imprime) printf("%s ! Ressource %s.%s inconnue\n",suite,modele_bn->nom,instr->reg.v.nom);
							return(0);
						}
	//					adrs = modele_bn->ress[k].ssadrs;
					}
					break;
				  default: break;
				}
			}
		} 
		if(instr->reg.type_sym == HW_VAL) /* Adresses hardcodees, pas de notion de capteur */ {
			char connu;
			if(detectr && !numeriseur) numeriseur = &(Numeriseur[detectr->captr[0].bb.num]);
			if(numeriseur) {
				if(instr->reg.v.i.valN < instr->reg.v.i.val1) {
					if(imprime) printf("%s ! Intervalle negatif, pas d'execution\n",suite);
					continue;
				} else if(instr->reg.v.i.valN == instr->reg.v.i.val1) {
					modele_bn = &(ModeleBN[numeriseur->def.type]);
					for(k=0; k<modele_bn->nbress; k++) if(modele_bn->ress[k].ssadrs == instr->reg.v.i.val1) break;
					connu = (k < modele_bn->nbress);
	//				adrs = instr->reg.v.i.val1; instr->oper = ':';
				} else connu = 0;
				if(!connu) {
					if(instr->val.type_sym == HW_NOM) {
						if(imprime) printf("%s ! Ressource %s.@%02X inconnue\n",suite,numeriseur->nom,instr->reg.v.i.val1);
						return(0);
					} else /* (instr->val.type_sym == HW_VAL) */ {
						hexa val;
						for(val=instr->reg.v.i.val1; val<=instr->reg.v.i.valN; val++) {
							NumeriseurAdrsChargeAuto(numeriseur,val,(shex)(instr->val.v.val),suite,ScriptSuite);
							if(imprime) suite = ScriptSuite;
						}
					}
					continue;
				}
			}
		}
		
		// instr->oper = '=';
		trouve = 0;
		if(categ >= 0) {
			switch(objcur) {
			  case HW_MODL_DET:
				trouve = 0;
				if(det < 0) {
					for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire && (Bolo[bolo].type == modele_det->num)) {
						detectr = &(Bolo[bolo]);
						for(num_regl=0; num_regl<detectr->nbreglages; num_regl++) if(categ == modele_det->regl[num_regl].cmde) {
							rc = ScriptAffecteReglage(instr,detectr,num_regl,imprime,suite,ScriptSuite);
							if((rc == 1) && (pc->boucle)) (pc->boucle)->en_cours = 1;
							//- printf("[a] recupere %d %s boucle @%08X\n",rc,(pc->boucle)?(((pc->boucle)->en_cours)?"dans":"hors"):"sans",(hexa)pc->boucle);
							if(imprime) suite = ScriptSuite;
							trouve = 1;
						}
					}
					if(imprime && !trouve) printf("%s*Pas de detecteur de type %s a lire avec un reglage de type %s\n",suite,modele_det->nom,ReglCmdeListe[categ]);
				} else {
					do {
						for(num_regl=0; num_regl<detectr->nbreglages; num_regl++) if(categ == modele_det->regl[num_regl].cmde) {
							rc = ScriptAffecteReglage(instr,detectr,num_regl,imprime,suite,ScriptSuite);
							if((rc == 1) && (pc->boucle)) (pc->boucle)->en_cours = 1;
							//- printf("[a] recupere %d %s boucle @%08X\n",rc,(pc->boucle)?(((pc->boucle)->en_cours)?"dans":"hors"):"sans",(hexa)pc->boucle);
							if(imprime) suite = ScriptSuite;
							trouve = 1;
						}
						if(imprime && !trouve) printf("%s*Le detecteur '%s' n'a pas de reglage de type %s\n",suite,detectr->nom,ReglCmdeListe[categ]);
						while(++det < BoloNb) if(Bolo[bolo].a_lire && (Bolo[det].type == modele_det->num)) break;
						if(det < BoloNb) detectr = &(Bolo[det]); else break;
					} while(1);
				}
				break;
			  case HW_DETEC:
				if(numeriseur) {
					modele_bn = &(ModeleBN[numeriseur->def.type]);
					for(k=0; k<modele_bn->nbress; k++) if(categ == modele_bn->ress[k].categ) {
	//					adrs = modele_bn->ress[k].ssadrs;
						rc = ScriptChargeRessource(instr,numeriseur,k,suite,ScriptSuite);
						if((rc == 1) && (pc->boucle)) (pc->boucle)->en_cours = 1;
						//- printf("[b] recupere %d %s boucle @%08X\n",rc,(pc->boucle)?(((pc->boucle)->en_cours)?"dans":"hors"):"sans",(hexa)pc->boucle);
						if(imprime) suite = ScriptSuite;
						trouve = 1;
					}
					if(imprime && !trouve) printf("%s*Pas de ressource de type %s dans un numeriseur de type %s\n",suite,RessCategListe[categ],modele_bn->nom);
				} else {
					for(num=0; num<detectr->nbreglages; num++) if(categ == modele_det->regl[num].cmde) {
						rc = ScriptAffecteReglage(instr,detectr,num,imprime,suite,ScriptSuite);
						if((rc == 1) && (pc->boucle)) (pc->boucle)->en_cours = 1;
						//- printf("[c] recupere %d %s boucle @%08X\n",rc,(pc->boucle)?(((pc->boucle)->en_cours)?"dans":"hors"):"sans",(hexa)pc->boucle);
						if(imprime) suite = ScriptSuite;
						trouve = 1;
					}
					if(imprime && !trouve) printf("%s*Pas de reglage de type %s dans un detecteur de type %s\n",suite,ReglCmdeListe[categ],modele_det->nom);
				}
				break;
			  case HW_NUMER:
				modele_bn = &(ModeleBN[numeriseur->def.type]);
				for(k=0; k<modele_bn->nbress; k++) if(categ == modele_bn->ress[k].categ) {
					rc = ScriptChargeRessource(instr,numeriseur,k,suite,ScriptSuite);
					if((rc == 1) && (pc->boucle)) (pc->boucle)->en_cours = 1;
					//- printf("[d] recupere %d %s boucle @%08X\n",rc,(pc->boucle)?(((pc->boucle)->en_cours)?"dans":"hors"):"sans",(hexa)pc->boucle);
					if(imprime) suite = ScriptSuite;
					trouve = 1;
				}
				if(imprime && !trouve) printf("%s*Pas de ressource de type %s dans un numeriseur de type %s\n",suite,RessCategListe[categ],modele_bn->nom);
				break;
			  case HW_MODL_NUM:
				for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].sel && (Numeriseur[bb].def.type == modele_bn->num)) {
					// if(!((Numeriseur[bb].repart)->actif)) continue; pas encore realiste?
					for(k=0; k<modele_bn->nbress; k++) if(categ == modele_bn->ress[k].categ) {
						rc = ScriptChargeRessource(instr,&(Numeriseur[bb]),k,suite,ScriptSuite);
						if((rc == 1) && (pc->boucle)) (pc->boucle)->en_cours = 1;
						//- printf("[e] recupere %d %s boucle @%08X\n",rc,(pc->boucle)?(((pc->boucle)->en_cours)?"dans":"hors"):"sans",(hexa)pc->boucle);
						if(imprime) suite = ScriptSuite;
						trouve = 1;
					}
				}
				if(imprime && !trouve) printf("%s*Pas de numeriseur de type %s utilise avec une ressource de type %s\n",suite,modele_bn->nom,RessCategListe[categ]);
				break;
			  case HW_MEDIA:
				l = (int)strlen(instr->val.v.nom);
				k = (int)write(media->ip.path,instr->val.v.nom,l);
				if(imprime) {
					if(k == l) printf("%s Ecrit via %s : %s <- \"%s\"\n",suite,media->nom,media->adrs,instr->val.v.nom);
					else printf("%s*Ecriture en erreur (%s), %d/%d octets transferes.\n",suite,strerror(errno),k,l,AccordAvec(k),AccordAvec(k));
				}
				break;
			  default: break;
			}
		} else if((objcur == HW_MODL_DET) && (num_regl >= 0)) {
			for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire && (Bolo[bolo].type == modele_det->num)) {
				rc = ScriptAffecteReglage(instr,&(Bolo[bolo]),num_regl,imprime,suite,ScriptSuite);
				if((rc == 1) && (pc->boucle)) (pc->boucle)->en_cours = 1;
				// printf("(%s) recupere %d %s boucle\n",__func__,rc,(pc->boucle)?(((pc->boucle)->en_cours)?"dans":"hors"):"sans");
				if(imprime) suite = ScriptSuite;
				trouve = 1;
			}
			if(imprime && !trouve) printf("%s*Pas de detecteur de type %s a lire\n",suite,modele_det->nom);
		} else if((objcur == HW_MODL_DET) && (det >= 0)) {
			do {
				rc = ScriptChargeRessource(instr,numeriseur,k,suite,ScriptSuite);
				if((rc == 1) && (pc->boucle)) (pc->boucle)->en_cours = 1;
				if(imprime) suite = ScriptSuite;
				while(++det < BoloNb) if(Bolo[bolo].a_lire && (Bolo[det].type == modele_det->num)) break;
				if(det < BoloNb) detectr = &(Bolo[det]); else break;
				for(cap=0; cap<detectr->nbcapt; cap++) if(!strcmp(cpt,ModeleVoie[detectr->captr[cap].modele].nom)) break;
				if(cap < detectr->nbcapt) {
					bb = detectr->captr[cap].bb.num;
					if((bb < 0) || (bb >= NumeriseurNb)) {
						if(imprime) printf("%s ! Le capteur '%s' du detecteur %s n'est pas connecte\n",suite,cpt,detectr->nom);
						return(0);
					}
				} else {
					if(imprime) printf("%s ! Capteur '%s' inconnu pour le detecteur %s\n",suite,cpt,detectr->nom);
					return(0);
				}
				numeriseur = &(Numeriseur[bb]); modele_bn = &(ModeleBN[numeriseur->def.type]);
				for(k=0; k<modele_bn->nbress; k++) if(!strcmp(rsce,modele_bn->ress[k].nom)) break;
				if(k >= modele_bn->nbress) {
					if(imprime) printf("%s ! Ressource %s.%s inconnue\n",suite,numeriseur->nom,rsce);
					return(0);
				}
			} while(1);
		} else if(objcur == HW_MODL_NUM) {
			for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].sel && (Numeriseur[bb].def.type == modele_bn->num)) {
				if(!(Numeriseur[bb].repart)) continue;
				// if(!((Numeriseur[bb].repart)->actif)) continue; pas encore realiste?
				rc = ScriptChargeRessource(instr,&(Numeriseur[bb]),k,suite,ScriptSuite);
				if((rc == 1) && (pc->boucle)) (pc->boucle)->en_cours = 1;
				// printf("(%s) recupere %d %s boucle\n",__func__,rc,(pc->boucle)?(((pc->boucle)->en_cours)?"dans":"hors"):"sans");
				if(imprime) suite = ScriptSuite;
				trouve = 1;
			}
			if(imprime && !trouve) printf("%s*Pas de numeriseur de type %s utilise\n",suite,modele_bn->nom);
		} else if(num_regl >= 0) {
			rc = ScriptAffecteReglage(instr,detectr,num_regl,imprime,suite,ScriptSuite);
			if((rc == 1) && (pc->boucle)) (pc->boucle)->en_cours = 1;
			//- printf("[f] recupere %d %s boucle @%08X\n",rc,(pc->boucle)?(((pc->boucle)->en_cours)?"dans":"hors"):"sans",(hexa)pc->boucle);
			if(imprime) suite = ScriptSuite;
		} else if(numeriseur) {
			modele_bn = &(ModeleBN[numeriseur->def.type]);
			if((k >= 0) && (k < modele_bn->nbress)) {
				rc = ScriptChargeRessource(instr,numeriseur,k,suite,ScriptSuite);
				if((rc == 1) && (pc->boucle)) (pc->boucle)->en_cours = 1;
				// printf("(%s) recupere %d %s boucle\n",__func__,rc,(pc->boucle)?(((pc->boucle)->en_cours)?"dans":"hors"):"sans");
			} else if(imprime) printf("%s ! pas de ressource valide utilisable dans %s\n",suite,numeriseur->nom);
		} else {
			if(imprime) printf("%s ! pas de numeriseur associe\n",suite);
		}
	}

	return(1);
}
/* ========================================================================== */
char *ScriptMarge() { return(ScriptSuite); }
/* ========================================================================== */
static short ScriptRunBloc(HW_OBJ obj, void *hw, HwInstr prgm, short premiere, short derniere, char *prefixe) {
	HwBloc b; short p;
	
	// un peu raide: for(p=premiere; p<derniere; p++) if(!ScriptRunInstr(&(prgm[p]),obj,hw,&b,&p,&BoucleEnCours,0,prefixe)) break;
	ScriptPC.inst = premiere;
	b = prgm[premiere].blocadrs;
	for(p=premiere; p<derniere; p++) {
		if(ScriptKill) { p = -1; break; }
		// printf("*** L'action %d.%d ...\n",a,p);
		ScriptExecBatch(prgm,obj,hw,&ScriptPC,p+1,0,prefixe);
		// printf("*** ... renvoie a %d.%d\n",b,p);
		if(prgm[p+1].blocadrs != b) break;
	}
	if(p >= derniere) p = -1;
	return(p);
}
/* ========================================================================== */
static int ScriptRun(HwScript script, char *prefixe) {
	int i,bolo,bb,objets_a_traiter; short premiere,derniere,b,p; short secs;
	int ival,tempo,nb; char demande;
	HwBloc bloc; HW_OBJ obj; TypeHwInstr *instr;

	demande = 0; p = 0;
	ScriptWait = 0;
	ScriptDuree = 0;
	nb = 0;
	tempo = -1;
	for(i=0; i<script->nb_blocs; i++) {
		bloc = &(script->bloc[i]);
		bloc->courante = bloc->premiere;
		obj = bloc->obj;
		for(p=bloc->premiere; p<bloc->derniere; p++) {
			instr = &(script->action[p]);
			if((instr->reg.type_sym == HW_NOM)  && !strcmp(instr->reg.v.nom,"boucle")) {
			   if(instr->val.type_sym == HW_NOM) { ival = 1; sscanf(instr->val.v.nom,"%d",&ival); }
			   else ival = instr->val.v.val;
				nb = ival; tempo = 0;
				// printf("isn %2d> boucle %d fois, debut compte temporaire\n",p,nb);
			} else if((instr->reg.type_sym == HW_NOM) && !strcmp(instr->reg.v.nom,"recommence")) {
				// printf("isn %2d> %d x %ds, soit ajout de %ds\n",p,nb,tempo,(nb * tempo));
				ScriptDuree += (nb * tempo); tempo = -1;
			} else if((instr->reg.type_sym == HW_NOM) && (!strcmp(instr->reg.v.nom,"wait") || !strcmp(instr->reg.v.nom,"acquisition") || !strcmp(instr->reg.v.nom,"stream"))) {
				if(instr->val.type_sym == HW_NOM) { ival = 1; sscanf(instr->val.v.nom,"%d",&ival); }
				else ival = instr->val.v.val;
				// printf("isn %2d> ajout de %ds",p,ival);
				if(tempo < 0) ScriptDuree += ival; else tempo += ival;
				// if(tempo < 0) printf("\n"); else printf(" soit boucle %ds\n",tempo);
				if(!strcmp(instr->reg.v.nom,"acquisition") || !strcmp(instr->reg.v.nom,"stream")) demande = 1;
			} // else printf("isn %2d> ..\n",p);
			// printf("     -> total %ds\n",ScriptDuree);
		}
	}
	if(demande) {
		if(OpiumExec(pLectConditions->cdr) == PNL_CANCEL) return(0);
		if(Archive.demandee) {
			if(OpiumExec(pRunParms->cdr) == PNL_CANCEL) return(0);
		}
	}
	if(ScriptDuree && OpiumDisplayed(pScriptCntl->cdr)) PanelRefreshVars(pScriptCntl);
	if(ScriptDuree && OpiumDisplayed(pScriptWait->cdr)) PanelRefreshVars(pScriptWait);
	ScriptKill = 0;
	for(b=0; b<script->nb_blocs; b++) {
		if(ScriptKill) break;
		bloc = &(script->bloc[b]);
		obj = bloc->obj;
		premiere = bloc->courante;
		derniere = bloc->derniere;
		// printf("(%s) Execution du bloc #%d de %d a %d\n",__func__,b,premiere,derniere);
		if(bloc->hw) p = ScriptRunBloc(obj,bloc->hw,script->action,premiere,derniere,prefixe);
		else switch(obj) /* 'tous' demande */ {
		  case HW_DETEC: 
			objets_a_traiter = 0;
			for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire) {
				Bolo[bolo].exec.inst = premiere; Bolo[bolo].exec.date = 0; Bolo[bolo].exec.boucle = 0;
				objets_a_traiter++;
			}
			while(objets_a_traiter) {
				gettimeofday(&LectDateRun,0);
				for(bolo=0; bolo<BoloNb; bolo++) if(Bolo[bolo].a_lire && (Bolo[bolo].exec.date <= LectDateRun.tv_sec)) {
					short derniere;
					if(ScriptKill) break;
                    b = Bolo[bolo].start.bloc;
                    Bolo[bolo].exec.inst = DetecScriptsLibrairie.bloc[b].premiere;
                    derniere = DetecScriptsLibrairie.bloc[b].derniere;
					while(Bolo[bolo].exec.inst < derniere) {
						if(ScriptKill) break;
						ScriptExecBatch(script->action,HW_DETEC,(void *)&(Bolo[bolo]),&(Bolo[bolo].exec),derniere,&secs,prefixe);
						if(secs) { Bolo[bolo].exec.date = LectDateRun.tv_sec + secs; break; }
					}
					if(Bolo[bolo].exec.inst >= derniere) --objets_a_traiter;
				}
				if(objets_a_traiter && !ScriptKill) sleep(1);
			}
			ScriptBoucleVide(&(Bolo[bolo].exec.boucle));
			p = Bolo[bolo].exec.inst;
			break;
		  case HW_NUMER: 
			objets_a_traiter = 0;
			for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].sel) {
				Numeriseur[bb].exec.inst = premiere; Numeriseur[bb].exec.date = 0; Numeriseur[bb].exec.boucle = 0;
				objets_a_traiter++;
			}
			while(objets_a_traiter) {
				gettimeofday(&LectDateRun,0);
				for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].sel && (Numeriseur[bb].exec.date <= LectDateRun.tv_sec)) {
					if(ScriptKill) break;
					while(Numeriseur[bb].exec.inst < derniere) {
						if(ScriptKill) break;
						ScriptExecBatch(script->action,HW_NUMER,(void *)&(Numeriseur[bb]),&(Numeriseur[bb].exec),derniere,&secs,prefixe);
						if(secs) { Numeriseur[bb].exec.date = LectDateRun.tv_sec + secs; break; }
					}
					if(Numeriseur[bb].exec.inst >= derniere) --objets_a_traiter;
				}
				if(objets_a_traiter && !ScriptKill) sleep(1);
			}
			ScriptBoucleVide(&(Numeriseur[bb].exec.boucle));
			p = Numeriseur[bb].exec.inst;
			break;
		  case HW_GLOBAL: 
			if(ScriptKill) break;
			p = ScriptRunBloc(obj,0,script->action,premiere,derniere,prefixe);
			break;
		  default: break;
		}
		if(p >= 0) {
			b = (script->action[p].blocadrs)->num; p++;
			if(p < script->bloc[b].derniere) script->bloc[b].courante = p; 
			else { b++; script->bloc[b].courante = script->bloc[b].premiere; }
			--b; /* incrementation boucle immediatement a suivre */
		}
	}
	ScriptKill = 0;
	NumeriseurChargeFin(prefixe);
	return(0);
}
/* ========================================================================== */
static int ScriptAjoute(HwScript script, char *nomcomplet, char *prefixe, char *bloc_ouvert, int *bcur) {
	FILE *f; char ligne[MAXLIGNE]; char debut[80];
	char ok;
	
	if((f = fopen(nomcomplet,"r"))) {
		ok = 1;
		// if(prefixe) printf("%s Inclusion du fichier %s\n",(*prefixe)?prefixe:DateHeure(),nomcomplet);
		while(LigneSuivante(ligne,MAXLIGNE,f) && ok) ok = (ok && ScriptDecode(script,ligne,bloc_ouvert,bcur,prefixe));
		fclose(f);
	} else {
		if(prefixe) printf("%s! Fichier %s illisible (%s)\n",ScriptPrefixe(prefixe,debut,80),nomcomplet,strerror(errno));
		return(0);
	}
	return(1);
}
/* ========================================================================== */
int ScriptExecType(HW_OBJ obj, void *hw, char *nomcomplet, char *prefixe) {
	FILE *f; char ligne[MAXLIGNE]; char debut[80];
	char bloc_ouvert,ok; int bcur;

    ok = 0;
	if((f = fopen(nomcomplet,"r"))) {
		// if(prefixe) printf("%s Execution du fichier %s\n",(*prefixe)?prefixe:DateHeure(),nomcomplet);
		if(ScriptCree()) {
			if(obj == HW_NB) bloc_ouvert = 0; else { bcur = ScriptNouveauBloc(ScriptEnCours,obj,hw); bloc_ouvert = 1; }
			ok = 1; while(LigneSuivante(ligne,MAXLIGNE,f) && ok) ok = (ok && ScriptDecode(ScriptEnCours,ligne,&bloc_ouvert,&bcur,prefixe));
			fclose(f);
			ScriptImprime(ScriptEnCours);
			if(ok) ScriptRun(ScriptEnCours,prefixe); else printf("%s! Erreur au decodage\n",ScriptPrefixe(prefixe,debut,80));
			ScriptRetireDernier();
		} else printf("%s! Erreur memoire, interpretation du script %s impossible\n",ScriptPrefixe(prefixe,debut,80),nomcomplet);
	} else {
		if(prefixe) printf("%s! Fichier %s illisible (%s)\n",ScriptPrefixe(prefixe,debut,80),nomcomplet,strerror(errno));
	}
	return(ok);
}
/* ========================================================================== */
int ScriptExec(char *nomcomplet, char *nomcourt, char *prefixe) {
	int rc; char log;

	if(ScriptEtat == 1) { OpiumError("Un script est deja en cours d'execution"); return(0); }
	log = (prefixe)? (prefixe[0] == '\0'): 0;
	ScriptEtat = 1;
	if(OpiumDisplayed(pScriptEtat->cdr)) PanelRefreshVars(pScriptEtat);
	OpiumUserAction();
	if(log) printf("%s/ Execution du script '%s' ----------------------------\n",DateHeure(),nomcourt);
	// if(log) NumeriseursSelPrint();
	rc = ScriptExecType(HW_NB,0,nomcomplet,prefixe);
	if(log) printf("%s/ Script '%s' termine ---------------------------------\n",DateHeure(),nomcourt);
	ScriptBoucleVide(&(ScriptPC.boucle));
	ScriptEtat = 2;
	if(OpiumDisplayed(pScriptEtat->cdr)) PanelRefreshVars(pScriptEtat);
	OpiumUserAction();
	return(rc);
}
