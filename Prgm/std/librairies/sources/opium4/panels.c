#include "environnement.h"

#include <stdlib.h> /* pour malloc et free */

#ifndef WIN32
#ifndef __MWERKS__
#define STD_UNIX
#endif
#endif

#ifdef STD_UNIX
#include <strings.h>  /* pour strlen qui rend size_t si intrinseque */
#else
#include <string.h>
#endif

#ifndef WIN32
#include <unistd.h> /* pour getcwd */
#endif

/* les 2 suivants pour opendir et la suite */
#ifdef __MWERKS__
#include <Types.h>
#include <stat.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif
#ifndef WIN32
#ifndef CW5
#include <dirent.h>
#endif
#endif


#include "claps.h"
#include "decode.h"
#include <opium.h>

#define PNL_MAXCHAMP 32 /* valeur de PanelVarLngrMaxi par defaut */

/* static char *PanelFmtFloat = "%#g"; */
static char *PanelFmtFloat = "%g"; /* pour compatibilite avec OPIUM1 */
static char *PanelFmtDouble = "%.15g";
static short PanelVarLngrMaxi = PNL_MAXCHAMP; /* pas plus que 255 */
static char *PanelItemAnonyme = "(positionnel)";

extern Panel OpiumPanelQuick;

static int PanelAdd(Panel panel, char *texte, int type, char *format, void *adrs, int max);
// static char PanelCode(PanelItem item, char etape, char *texte, int *col);
static void PanelPrintVar(WndFrame f, Panel panel,PanelItem item, char selectr, int lig, int col, int max);
static void PanelEntryVerif(Cadre cdr, Panel panel, WndFrame f, PanelItem item, char direct,
	char *valeur, int lig, int col, int *car);
static void PanelCursorSet(Panel panel, WndFrame f, int lig, int col);

/* ========================================================================== */
Panel PanelCreate(int maxitems) {
	Cadre cdr; Panel panel; int i;

	panel = (Panel)malloc(sizeof(TypePanel));
	if(DEBUG_PANEL(1)) WndPrint("(%s) Panel cree P=%08llX\n",__func__,(IntAdrs)panel);
	if(panel) {
//		if(DEBUG_PANEL(1)) WndPrint("(%s) Creation du cadre\n",__func__);
		cdr = OpiumCreate(OPIUM_PANEL,panel);
		if(DEBUG_PANEL(1)) WndPrint("(%s) C=%08llX cree\n",__func__,(IntAdrs)cdr);
		if(!cdr) { free(panel); return(0);  }
//		if(DEBUG_PANEL(1)) WndPrint("(%s) Creation des items\n",__func__);
		panel->items = (PanelItem )malloc(maxitems * sizeof(TypePanelItem));
		if(DEBUG_PANEL(1)) WndPrint("(%s) I=%08llX crees\n",__func__,(IntAdrs)panel->items);
		if(!panel->items) { OpiumDelete(cdr); free(panel); return(0);  }
		panel->cdr = cdr;
		panel->max = (short)maxitems;
		panel->mode = PNL_SECURE;
		panel->support = WND_RIEN;
		panel->nbcols = 0;
		panel->maxligs = panel->max;
		panel->col = 0; panel->var = 0;
		panel->on_reset = panel->on_apply = panel->on_ok = 0;
//		if(DEBUG_PANEL(1)) WndPrint("(%s) Creation des boutons avec B=%08llX\n",__func__,(IntAdrs)OpiumReplyStandard);
		for(i=0; i<PNL_NBREPLIES; i++) {
//			if(DEBUG_PANEL(1)) WndPrint("(%s) Bouton[%d/%d]: %s\n",__func__,i+1,PNL_NBREPLIES,OpiumReplyStandard[i]);
			panel->bouton[i] = OpiumReplyStandard[i];
		}
		panel->trace = panel->mods = panel->hilite = 0;
//		if(DEBUG_PANEL(1)) WndPrint("(%s) Efface P=%08llX\n",__func__,(IntAdrs)panel);
		PanelErase(panel);
//		if(DEBUG_PANEL(1)) WndPrint("(%s) Options pour C=%08llX\n",__func__,(IntAdrs)cdr);
		OpiumSetOptions(cdr);
//		if(DEBUG_PANEL(1)) WndPrint("(%s) Utilise C=%08llX\n",__func__,(IntAdrs)panel->cdr);
		if(DEBUG_PANEL(1)) printf("(%s) Tableau %d x %d\n",__func__,panel->nbcols,panel->maxligs);
	}
	return(panel);
}
/* ========================================================================== */
void PanelDelete(Panel panel) {
	if(!panel) return;
	if(panel->cdr) OpiumDelete(panel->cdr); /* a besoin des items pour liberer des GC */
	if(panel->items) free(panel->items);
	if(panel->nbcols > 1) {
		if(panel->col) free(panel->col);
		if(panel->var) free(panel->var);
	}
	free(panel);
}
/* ========================================================================== */
void PanelErase(Panel panel) {
	int n; PanelItem item;

	if(!panel) return;
	if(panel->nbcols != 1) /* couvre le cas 0 */ {
		if(panel->col) free(panel->col);
		if(panel->var) free(panel->var);
		panel->nbcols = 1;
		// panel->col = (int *)malloc(panel->nbcols * sizeof(int));
		// panel->var = (int *)malloc(panel->nbcols * sizeof(int));
		panel->col = &(panel->col1);
		panel->var = &(panel->var1);
	}
	panel->cur = 0;
	panel->prochaine.col = 0;
	panel->prochaine.lig = 0;
	panel->xcurseur = panel->ycurseur = panel->icurseur = -1;
	panel->ccurseur = 0;
	panel->overstrike = 1;
	panel->entete_verrouillee = 0;
	panel->not_to_refresh = 0;
/* recalcule a la construction via PanelAdd */
	panel->nbligs = 1; // panel->max;
	panel->precedent = -1;
	for(n=0, item = panel->items; n<panel->max; n++,item++) {
		memset(item,0,sizeof(TypePanelItem));
		item->type = -1;
		item->etat = PNL_MODIFIABLE; // a verifier
		strcpy(item->valeur,"--");
		item->ptrval = item->valeur;
		item->ptrdim = 0;
	}
}
/* ========================================================================== */
Panel PanelDuplique(Panel p) {
	Cadre cdr; Panel panel;

	panel = (Panel)malloc(sizeof(TypePanel));
	if(panel) {
		cdr = OpiumCreate(OPIUM_PANEL,panel);
		if(!cdr) { free(panel); return(0);  }
		memcpy(panel,p,sizeof(TypePanel));
		panel->cdr = cdr;
	}
	return(panel);
}
/* ========================================================================== */
void PanelTitle(Panel panel, char *texte) {
	if(panel) OpiumTitle(panel->cdr,texte);
}
/* ========================================================================== */
void PanelMode(Panel panel, unsigned short mode) {
	if(panel) panel->mode = mode;
}
/* ========================================================================== */
void PanelDansPlanche(Panel panel) {
	if(panel) { panel->mode |= PNL_INBOARD; PanelSupport(panel,WND_CREUX); }
}
/* ========================================================================== */
void PanelVerrouEntete(Panel panel, unsigned char verrou) {
	if(panel) panel->entete_verrouillee = verrou;
}
/* ========================================================================== */
void PanelSupport(Panel panel, char support) {
	if(panel) panel->support = support;
}
/* ========================================================================== */
void PanelColumns(Panel panel, int cols) {
	if(panel) {
		if(panel->nbcols > 1) {
			if(panel->col) free(panel->col);
			if(panel->var) free(panel->var);
		}
		if(cols > 1) {
			panel->col = (short *)malloc(cols * sizeof(short));
			panel->var = (short *)malloc(cols * sizeof(short));
			if(panel->var == 0) {
				if(panel->col != 0) free(panel->col);
				cols = 1;
			}
		} else if(cols == 0) cols = 1;
		if(cols == 1) {
			panel->col = &(panel->col1);
			panel->var = &(panel->var1);
		}
		panel->nbcols = (short)cols;
		panel->nbligs = panel->maxligs = ((panel->max - 1) / panel->nbcols) + 1;
		// printf("(%s) Tableau %d x %d\n",__func__,panel->nbcols,panel->maxligs);
	}
}
/* ========================================================================== */
void PanelBoutonText(Panel panel, int n, char *texte) {

	if(panel && (n >= PNL_NOMODIF) && (n < PNL_NBREPLIES)) {
		if(texte) panel->bouton[n] = texte;
		else panel->bouton[n] = OpiumReplyStandard[n];
	}
}
/* ========================================================================== */
void PanelOnReset(Panel panel, TypePanelFctnArg fctn, void *arg) {
	if(panel) { panel->on_reset = fctn; panel->reset_arg = arg; }
}
/* ========================================================================== */
void PanelOnApply(Panel panel, TypePanelFctnArg fctn, void *arg) {
	if(panel) { panel->on_apply = fctn; panel->apply_arg = arg; }
}
/* ========================================================================== */
void PanelOnOK(Panel panel, TypePanelFctnArg fctn, void *arg) {
	if(panel) { panel->on_ok = fctn; panel->ok_arg = arg; }
}
/* ========================================================================== */
void *PanelApplyArg(Panel panel) { return(panel? panel->apply_arg: 0); }
/* ========================================================================== */
void *PanelOKArg(Panel panel) { return(panel? panel->ok_arg: 0); }
/* ========================================================================== */
void PanelMaxChamp(short max) {
	if(max > 0) {
		if(max < 256) PanelVarLngrMaxi = max;
		else PanelVarLngrMaxi = 255;
	} else PanelVarLngrMaxi = PNL_MAXCHAMP;
}
/* ========================================================================== */
int PanelBlank(Panel panel) { return(PanelRemark(panel,0)); }
/* ========================================================================== */
int PanelRemark(Panel panel, char *texte) {
	int nb;

	if(!panel) return(0);
	nb = PanelAdd(panel,texte,PNL_REMARK,"\0",0,0);
	panel->items[nb - 1].sel = 0;
	panel->items[nb - 1].etat = PNL_PROTEGE;
	return(nb);
}
/* ========================================================================== */
int PanelSepare(Panel panel, char *texte) {
	int nb;

	if(!panel) return(0);
	nb = PanelAdd(panel,texte,PNL_SEPARE,"\0",0,0);
	panel->items[nb - 1].sel = 0;
	panel->items[nb - 1].etat = PNL_PROTEGE;
	return(nb);
}
/* ========================================================================== */
int PanelPswd(Panel panel, char *texte, char *adrs, int lngr) {
	return(PanelAdd(panel,texte,PNL_PSWD,"%s",(void *)adrs,lngr-1));
}
/* ========================================================================== */
int PanelFile(Panel panel, char *texte, char *adrs, int lngr) {
	return(PanelAdd(panel,texte,PNL_FILE,"%s",(void *)adrs,lngr-1));
}
/* ========================================================================== */
int PanelText(Panel panel, char *texte, char *adrs, int lngr) {
	return(PanelAdd(panel,texte,PNL_TEXTE,"%s",(void *)adrs,lngr-1));
}
/* ========================================================================== */
int PanelLong(Panel panel, char *texte, int64 *adrs) {
	return(PanelAdd(panel,texte,PNL_LONG,"%lld",(void *)adrs,21));
}
/* ========================================================================== */
int PanelLHex(Panel panel, char *texte, int64 *adrs) {
	return(PanelAdd(panel,texte,PNL_LHEX,"%16llx",(void *)adrs,16));
}
/* ========================================================================== */
int PanelInt(Panel panel, char *texte, int *adrs) {
	return(PanelAdd(panel,texte,PNL_INT,"%d",(void *)adrs,11));
}
/* ========================================================================== */
int PanelHex(Panel panel, char *texte, int *adrs) {
	return(PanelAdd(panel,texte,PNL_HEX,"%08lX",(void *)adrs,8));
}
/* ========================================================================== */
int PanelShort(Panel panel, char *texte, short *adrs) {
	return(PanelAdd(panel,texte,PNL_SHORT,"%hd",(void *)adrs,6));
}
/* ========================================================================== */
int PanelSHex(Panel panel, char *texte, short *adrs) {
	return(PanelAdd(panel,texte,PNL_SHEX,"%04hX",(void *)adrs,4));
}
/* ========================================================================== */
int PanelChar(Panel panel, char *texte, char *adrs) {
	return(PanelAdd(panel,texte,PNL_CHAR,"%hhd",(void *)adrs,4));
}
/* ========================================================================== */
int PanelOctet(Panel panel, char *texte, unsigned char *adrs) {
	return(PanelAdd(panel,texte,PNL_OCTET,"%hhd",(void *)adrs,4));
}
/* ========================================================================== */
int PanelLettre(Panel panel, char *texte, char *adrs) {
	return(PanelAdd(panel,texte,PNL_LETTRE,"%c",(void *)adrs,2));
}
/* ========================================================================== */
int PanelByte(Panel panel, char *texte, unsigned char *adrs) {
	return(PanelAdd(panel,texte,PNL_BYTE,"%02hhX",(void *)adrs,2));
}
/* ========================================================================== */
int PanelAdu(Panel panel, char *texte, unsigned short *adrs) {
	return(PanelAdd(panel,texte,PNL_ADU,"%hd",(void *)adrs,6));
}
/* ========================================================================== */
int PanelBool(Panel panel, char *texte, char *adrs) {
	return(PanelAdd(panel,texte,PNL_BOOL,"%s",(void *)adrs,3));
}
/* ========================================================================== */
int PanelFloat(Panel panel, char *texte, float *adrs) {
	return(PanelAdd(panel,texte,PNL_FLOAT,PanelFmtFloat,(void *)adrs,12));
}
/* ========================================================================== */
int PanelDouble(Panel panel, char *texte, double *adrs) {
	return(PanelAdd(panel,texte,PNL_DBLE,PanelFmtDouble,(void *)adrs,18));
}
/* ========================================================================== */
int PanelList(Panel panel, char *texte, char **liste, int *adrs, int lngr) {
	int nb;

	if(!panel) return(0);
	if((nb = PanelAdd(panel,texte,PNL_LISTE,"%s",(void *)adrs,lngr-1)))
		(panel->items[nb - 1]).fctn = (void *)liste;
	return(nb);
}
/* ========================================================================== */
int PanelListL(Panel panel, char *texte, char **liste, long *adrs, int lngr) {
	int nb;

	if(!panel) return(0);
	if((nb = PanelAdd(panel,texte,PNL_LISTL,"%s",(void *)adrs,lngr-1)))
		(panel->items[nb - 1]).fctn = (void *)liste;
	return(nb);
}
/* ========================================================================== */
int PanelListD(Panel panel, char *texte, char **liste, int64 *adrs, int lngr) {
	int nb;

	if(!panel) return(0);
	if((nb = PanelAdd(panel,texte,PNL_LISTD,"%s",(void *)adrs,lngr-1)))
		(panel->items[nb - 1]).fctn = (void *)liste;
	return(nb);
}
/* ========================================================================== */
int PanelListS(Panel panel, char *texte, char **liste, short *adrs, int lngr) {
	int nb;

	if(!panel) return(0);
	if((nb = PanelAdd(panel,texte,PNL_LISTS,"%s",(void *)adrs,lngr-1)))
		(panel->items[nb - 1]).fctn = (void *)liste;
	return(nb);
}
/* ========================================================================== */
int PanelListB(Panel panel, char *texte, char **liste, char *adrs, int lngr) {
	int nb;

	if(!panel) return(0);
	if((nb = PanelAdd(panel,texte,PNL_LISTB,"%s",(void *)adrs,lngr-1)))
		(panel->items[nb - 1]).fctn = (void *)liste;
	return(nb);
}
/* ========================================================================== */
int PanelKey(Panel panel, char *texte, char *modeles, int *adrs, int lngr) {
	int nb;

	if(!panel) return(0);
	if((nb = PanelAdd(panel,texte,PNL_KEY,"%s",(void *)adrs,lngr-1)))
		(panel->items[nb - 1]).fctn = (void *)modeles;
	return(nb);
}
/* ========================================================================== */
int PanelKeyL(Panel panel, char *texte, char *modeles, int64 *adrs, int lngr) {
	int nb;

	if(!panel) return(0);
	if((nb = PanelAdd(panel,texte,PNL_KEYL,"%s",(void *)adrs,lngr-1)))
		(panel->items[nb - 1]).fctn = (void *)modeles;
	return(nb);
}
/* ========================================================================== */
int PanelKeyS(Panel panel, char *texte, char *modeles, short *adrs, int lngr) {
	int nb;

	if(!panel) return(0);
	if((nb = PanelAdd(panel,texte,PNL_KEYS,"%s",(void *)adrs,lngr-1)))
		(panel->items[nb - 1]).fctn = (void *)modeles;
	return(nb);
}
/* ========================================================================== */
int PanelKeyB(Panel panel, char *texte, char *modeles, char *adrs, int lngr) {
	int nb;

	if(!panel) return(0);
	if((nb = PanelAdd(panel,texte,PNL_KEYB,"%s",(void *)adrs,lngr-1)))
		(panel->items[nb - 1]).fctn = (void *)modeles;
	return(nb);
}
/* ========================================================================== */
int PanelUser(Panel panel, char *texte, TypeArgUserFctn fctn, void *adrs, int lngr) {
	int nb;

	if(!panel) return(0);
	if((nb = PanelAdd(panel,texte,PNL_USER,"%s",adrs,lngr-1)))
		(panel->items[nb - 1]).fctn = (void *)fctn;
	return(nb);
}
/* ========================================================================== */
int PanelBouton(Panel panel, char *texte, TypePanelFctnButn fctn, void *adrs, int lngr) {
	int nb;

	if(!panel) return(0);
	if((nb = PanelAdd(panel,texte,PNL_BOUTON,"%s",adrs,lngr-1)))
		(panel->items[nb - 1]).fctn = (void *)fctn;
	return(nb);
}
/* ========================================================================== */
Panel PanelSelecteur(OpiumSelecteur select, unsigned short exclusif) {
	Panel panel; int nb; OpiumSelecteur item;

	nb = 0; item = select;
	while(item->sel) { nb++; item++; }
	panel = PanelCreate(nb);
	item = select; while(item->sel) {
		PanelItemColors(panel,PanelKeyB(panel,item->texte,"-/+",item->sel,2),OpiumOrangeVert,2);
		item++;
	}
	if(exclusif) PanelMode(panel,panel->mode | PNL_EXCLUSF); // pas gere
	return(panel);
}
#ifdef OBSOLETE
/* ========================================================================== */
Panel PanelArgs(ArgDesc *desc) {
	/* !!! OBSOLETE: tableau de valeurs pas pris en compte */
	ArgDesc *d;
	int nb;
	Panel p;
	char *txt,texte[24];

	strcpy(texte,"Parametre positionnel");
	d = desc;
	nb = 0;
	while(d->type >= 0) { if(d->adrs) nb++; d++; }
	p = PanelCreate(nb);
	if(!p) return(0);

	d = desc;
	while(d->type >= 0) {
		if(d->adrs) {
			/* if(d->explic) txt = d->explic;
			else */ if(d->texte) txt = d->texte;
			else txt = texte;
			switch(d->type) {
				case ARG_TYPE_COMMENT:  PanelRemark(p,txt); break;
				case ARG_TYPE_TEXTE:  PanelText   (p,txt,(char *)d->adrs,d->lngr?(int)d->lngr:32);break;
				case ARG_TYPE_INT:    PanelInt    (p,txt,(int *)d->adrs); break;
				case ARG_TYPE_HEXA:   PanelHex    (p,txt,(int *)d->adrs); break;
				case ARG_TYPE_SHORT:  PanelShort  (p,txt,(short *)d->adrs); break;
				case ARG_TYPE_SHEX:   PanelSHex   (p,txt,(short *)d->adrs); break;
				case ARG_TYPE_FLOAT:  PanelFloat  (p,txt,(float *)d->adrs); break;
				case ARG_TYPE_OCTET:  PanelOctet  (p,txt,(char *)d->adrs); break;
				case ARG_TYPE_BYTE:   PanelByte   (p,txt,(unsigned char *)d->adrs); break;
				case ARG_TYPE_LETTRE: PanelLettre (p,txt,(char *)adrs); adrs++; break;
				case ARG_TYPE_BOOL:   PanelBool   (p,txt,(char *)d->adrs); break;
				case ARG_TYPE_KEY:    PanelKeyB   (p,txt,d->modele,(char *)d->adrs,32); break;
				case ARG_TYPE_LISTE:  PanelList   (p,txt,(char **)d->modele,(int *)d->adrs,32); break;
				case ARG_TYPE_USER:   PanelUser   (p,txt,(TypeArgUserFctn)d->modele,d->adrs,32); break;
				case ARG_TYPE_STRUCT: PanelRemark (p,txt); break;
				case ARG_TYPE_DESC:   PanelRemark (p,txt); break;
				case ARG_TYPE_RIEN:   PanelRemark (p,txt); break;
				case ARG_TYPE_VARIANTE: PanelRemark(p,txt); break;
				default:              PanelRemark(p,txt); break;
			}
		}
		d++;
	}
	return(p);
}
#endif
/* ========================================================================== */
void PanelAddDesc(Panel p, ArgDesc *desc, int larg) {
	ArgDesc *d;
	int k,dim,index;
	char *adrs,*txt,texte[64];

	strcpy(texte,"Parametre positionnel");
	d = desc; k = 0;
	while(d->type >= 0) {
		if(d->adrs) {
			/* if(d->explic) txt = d->explic; else */
			if(d->texte) txt = d->texte;
			else /* {
				if(larg > 23) snprintf(texte,63,"Parametre positionnel #%d",++k);
				else if(larg > 11) sprintf(texte,"Parametre #%d",++k);
				else sprintf(texte,"Parm #%d",++k);
				texte[63] = '\0';
				txt = texte;
			} */ txt = PanelItemAnonyme;
			if(d->taille) dim = *(d->taille); else dim = d->dim;
			adrs = (char *)d->adrs;
			for(index=0; index<dim; index++) {
				switch(d->type) {
					case ARG_TYPE_COMMENT:  PanelRemark(p,txt); break;
					case ARG_TYPE_TEXTE:  PanelText   (p,txt,adrs,d->lngr?(int)d->lngr:32); adrs = (adrs + d->lngr); break;
					case ARG_TYPE_INT:    PanelInt    (p,txt,(int *)adrs); adrs = adrs + sizeof(int); break;
					case ARG_TYPE_HEXA:   PanelHex    (p,txt,(int *)adrs); adrs = adrs + sizeof(int); break;
					case ARG_TYPE_SHORT:  PanelShort  (p,txt,(short *)adrs); adrs = adrs + sizeof(short); break;
					case ARG_TYPE_SHEX:   PanelSHex   (p,txt,(short *)adrs); adrs = adrs + sizeof(short); break;
					case ARG_TYPE_FLOAT:  PanelFloat  (p,txt,(float *)adrs); adrs = adrs + sizeof(float); break;
					case ARG_TYPE_OCTET:  PanelOctet  (p,txt,(unsigned char *)adrs); adrs++; break;
					case ARG_TYPE_BYTE:   PanelByte   (p,txt,(unsigned char *)adrs); adrs++; break;
					case ARG_TYPE_LETTRE: PanelLettre (p,txt,adrs); adrs++; break;
					case ARG_TYPE_BOOL:   PanelBool   (p,txt,adrs); adrs++; break;
					case ARG_TYPE_KEY:    PanelKeyB   (p,txt,d->modele,adrs,32); adrs++; break;
					case ARG_TYPE_LISTE:  PanelList   (p,txt,(char **)d->modele,(int *)adrs,32); adrs = adrs + sizeof(int); break;
					case ARG_TYPE_USER:   PanelUser   (p,txt,(TypeArgUserFctn)d->modele,adrs,32); adrs = adrs + sizeof(int); break;
					case ARG_TYPE_STRUCT: PanelRemark (p,txt); break;
					case ARG_TYPE_DESC:   PanelRemark (p,txt); break;
					case ARG_TYPE_RIEN:   PanelRemark (p,txt); break;
					case ARG_TYPE_VARIANTE: if(d->texte) PanelRemark(p,d->texte); else PanelRemark(p,"n/a"); break;
					default:              PanelRemark (p,txt); break;
				}
			}
		}
		d++;
	}

}
/* ========================================================================== */
Panel PanelDesc(ArgDesc *desc, char avec) {
	Panel p;
	ArgDesc *d;
	int dim,nb,cols,index; int l,larg;

	larg = 0;
	d = desc;
	nb = 0; cols = 1;
	while(d->type >= 0) {
		if(d->adrs) {
			if(d->taille) dim = *(d->taille); else dim = d->dim;
			nb += dim;
			if(d->texte) { l = (int)strlen(d->texte); if(l > larg) larg = l; }
			if(avec && d->explic) cols = 2;
		}
		d++;
	}
	p = PanelCreate(cols * nb);
	if(!p) return(0);
	if(cols > 1) PanelColumns(p,cols);

	PanelAddDesc(p,desc,larg);

	if(cols > 1) {
		d = desc;
		while(d->type >= 0) {
			if(d->adrs) {
				if(d->taille) dim = *(d->taille); else dim = d->dim;
				for(index=0; index<dim; index++) {
					switch(d->type) {
					  case ARG_TYPE_COMMENT:  PanelRemark(p,0); break;
					  default:
						if(d->explic) PanelItemSelect(p,PanelText(p,0,d->explic,(int)strlen(d->explic)+1),0);
						else PanelRemark(p,"..");
						break;
					}
				}
			}
			d++;
		}
	}

	return(p);
}
/* ========================================================================== */
/* ================== Entree simplifiee a une seule variable ================ */
/* ========================================================================== */
void OpiumReadTitle(char *titre) { OpiumTitle(OpiumPanelQuick->cdr,titre); }
/* ========================================================================== */
int OpiumReadPswd(char *texte, char *adrs, int lngr) {
	PanelErase(OpiumPanelQuick);
	PanelAdd(OpiumPanelQuick,texte,PNL_PSWD,"%s",(void *)adrs,lngr-1);
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadFile(char *texte, char *adrs, int lngr) {
	PanelErase(OpiumPanelQuick);
	PanelAdd(OpiumPanelQuick,texte,PNL_FILE,"%s",(void *)adrs,lngr-1);
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadText(char *texte, char *adrs, int lngr) {
	PanelErase(OpiumPanelQuick);
	PanelAdd(OpiumPanelQuick,texte,PNL_TEXTE,"%s",(void *)adrs,lngr-1);
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadLong(char *texte, int64 *adrs) {
	PanelErase(OpiumPanelQuick);
	PanelAdd(OpiumPanelQuick,texte,PNL_LONG,"%d",(void *)adrs,21);
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadLHex(char *texte, int64 *adrs) {
	PanelErase(OpiumPanelQuick);
	PanelAdd(OpiumPanelQuick,texte,PNL_LHEX,"%16llx",(void *)adrs,16);
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadInt(char *texte, int *adrs) {
	PanelErase(OpiumPanelQuick);
	PanelAdd(OpiumPanelQuick,texte,PNL_INT,"%d",(void *)adrs,11);
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadHex(char *texte, int *adrs) {
	PanelErase(OpiumPanelQuick);
	PanelAdd(OpiumPanelQuick,texte,PNL_HEX,"%08lX",(void *)adrs,8);
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadShort(char *texte, short *adrs) {
	PanelErase(OpiumPanelQuick);
	PanelAdd(OpiumPanelQuick,texte,PNL_SHORT,"%hd",(void *)adrs,6);
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadSHex(char *texte, short *adrs) {
	PanelErase(OpiumPanelQuick);
	PanelAdd(OpiumPanelQuick,texte,PNL_SHEX,"%04hX",(void *)adrs,4);
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadAdu(char *texte, unsigned short *adrs) {
	PanelErase(OpiumPanelQuick);
	PanelAdd(OpiumPanelQuick,texte,PNL_ADU,"%hd",(void *)adrs,6);
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadChar(char *texte, char *adrs) {
	PanelErase(OpiumPanelQuick);
	PanelAdd(OpiumPanelQuick,texte,PNL_CHAR,"%hhd",(void *)adrs,4);
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadOctet(char *texte, unsigned char *adrs) {
	PanelErase(OpiumPanelQuick);
	PanelAdd(OpiumPanelQuick,texte,PNL_OCTET,"%hhd",(void *)adrs,4);
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadLettre(char *texte, char *adrs) {
	PanelErase(OpiumPanelQuick);
	PanelAdd(OpiumPanelQuick,texte,PNL_LETTRE,"%c",(void *)adrs,2);
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadByte(char *texte, char *adrs) {
	PanelErase(OpiumPanelQuick);
	PanelAdd(OpiumPanelQuick,texte,PNL_BYTE,"%hh02X",(void *)adrs,2);
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadBool(char *texte, char *adrs) {
	PanelErase(OpiumPanelQuick);
	PanelAdd(OpiumPanelQuick,texte,PNL_BOOL,"%s",(void *)adrs,3);
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadFloat(char *texte, float *adrs) {
	PanelErase(OpiumPanelQuick);
	PanelAdd(OpiumPanelQuick,texte,PNL_FLOAT,"%g",(void *)adrs,12);
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadDouble(char *texte, double *adrs) {
	PanelErase(OpiumPanelQuick);
	PanelAdd(OpiumPanelQuick,texte,PNL_DBLE,"%g",(void *)adrs,18);
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadList(char *texte, char **liste, int *adrs, int lngr) {
	int nb;

	PanelErase(OpiumPanelQuick);
	if((nb = PanelAdd(OpiumPanelQuick,texte,PNL_LISTE,"%s",(void *)adrs,lngr-1)))
		(OpiumPanelQuick->items[nb - 1]).fctn = (void *)liste;
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadListL(char *texte, char **liste, long *adrs, int lngr) {
	int nb;

	PanelErase(OpiumPanelQuick);
	if((nb = PanelAdd(OpiumPanelQuick,texte,PNL_LISTL,"%s",(void *)adrs,lngr-1)))
		(OpiumPanelQuick->items[nb - 1]).fctn = (void *)liste;
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadListD(char *texte, char **liste, int64 *adrs, int lngr) {
	int nb;

	PanelErase(OpiumPanelQuick);
	if((nb = PanelAdd(OpiumPanelQuick,texte,PNL_LISTD,"%s",(void *)adrs,lngr-1)))
		(OpiumPanelQuick->items[nb - 1]).fctn = (void *)liste;
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadListS(char *texte, char **liste, short *adrs, int lngr) {
	int nb;

	PanelErase(OpiumPanelQuick);
	if((nb = PanelAdd(OpiumPanelQuick,texte,PNL_LISTS,"%s",(void *)adrs,lngr-1)))
		(OpiumPanelQuick->items[nb - 1]).fctn = (void *)liste;
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadListB(char *texte, char **liste, char *adrs, int lngr) {
	int nb;

	PanelErase(OpiumPanelQuick);
	if((nb = PanelAdd(OpiumPanelQuick,texte,PNL_LISTB,"%s",(void *)adrs,lngr-1)))
		(OpiumPanelQuick->items[nb - 1]).fctn = (void *)liste;
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadKey(char *texte, char *modeles, int *adrs, int lngr) {
	int nb;

	PanelErase(OpiumPanelQuick);
	if((nb = PanelAdd(OpiumPanelQuick,texte,PNL_KEY,"%s",(void *)adrs,lngr-1)))
		(OpiumPanelQuick->items[nb - 1]).fctn = (void *)modeles;
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadKeyL(char *texte, char *modeles, int64 *adrs, int lngr) {
	int nb;

	PanelErase(OpiumPanelQuick);
	if((nb = PanelAdd(OpiumPanelQuick,texte,PNL_KEYL,"%s",(void *)adrs,lngr-1)))
		(OpiumPanelQuick->items[nb - 1]).fctn = (void *)modeles;
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadKeyS(char *texte, char *modeles, short *adrs, int lngr) {
	int nb;

	PanelErase(OpiumPanelQuick);
	if((nb = PanelAdd(OpiumPanelQuick,texte,PNL_KEYS,"%s",(void *)adrs,lngr-1)))
		(OpiumPanelQuick->items[nb - 1]).fctn = (void *)modeles;
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadKeyB(char *texte, char *modeles, char *adrs, int lngr) {
	int nb;

	PanelErase(OpiumPanelQuick);
	if((nb = PanelAdd(OpiumPanelQuick,texte,PNL_KEYB,"%s",(void *)adrs,lngr-1)))
		(OpiumPanelQuick->items[nb - 1]).fctn = (void *)modeles;
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadUser(char *texte, TypeArgUserFctn fctn, void *adrs, int lngr) {
	int nb;

	PanelErase(OpiumPanelQuick);
	if((nb = PanelAdd(OpiumPanelQuick,texte,PNL_USER,"%s",adrs,lngr-1)))
		(OpiumPanelQuick->items[nb - 1]).fctn = (void *)fctn;
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
int OpiumReadBouton(char *texte, TypePanelFctnButn fctn, void *adrs, int lngr) {
	int nb;

	PanelErase(OpiumPanelQuick);
	if((nb = PanelAdd(OpiumPanelQuick,texte,PNL_BOUTON,"%s",adrs,lngr-1)))
		(OpiumPanelQuick->items[nb - 1]).fctn = (void *)fctn;
	return(OpiumExec(OpiumPanelQuick->cdr));
}
/* ========================================================================== */
void OpiumReadFormat(char *format) {
	strncpy((OpiumPanelQuick->items[0]).format,format,PNL_MAXFMT);
}
/* ========================================================================== */
void OpiumReadLngr(int lngr) {
	if((lngr > 0) && (lngr <= PanelVarLngrMaxi)) {
		(OpiumPanelQuick->items[0]).maxuser = (short)lngr + 1;
		(OpiumPanelQuick->items[0]).maxdisp = (short)lngr;
	}
}
/* ========================================================================== */
static inline int PanelItemNum(Panel panel, int col, int lig) {
	return((col * panel->maxligs) + lig);
	//	return(col + (lig * panel->nbcols));
}
#ifdef INUTILISE
/* ========================================================================== */
static inline void PanelItemPos(Panel panel, int num, int *col, int *lig) {
	*col = num / panel->maxligs; /* *lig = num % panel->maxligs; */ *lig = num - (*col * panel->maxligs);
	//	*lig = num / panel->nbcols; *col = num - (*lig * panel->nbcols);
}
#endif /* INUTILISE */
/* ========================================================================== */
void PanelPrepare(Panel panel, int col, int lig) {
	/* pour entrer les items dans le desordre (col et lig demarrent a 1) */
	if(!panel) return;
	if((col > 0) && (col <= panel->nbcols)
	&& (lig > 0) && (lig <= panel->maxligs)) {
		panel->prochaine.col = (short)col;
		panel->prochaine.lig = (short)lig;
		// printf("(%s)      Prochain item en colonne %d, ligne %d\n",__func__,col,lig);
	}
}
/* ========================================================================== */
static int PanelAdd(Panel panel, char *texte, int type, char *format, void *adrs, int max) {
	int numitem;
	PanelItem item;

	if(!panel) return(0);
/*	int lig,col;
	if(panel->prochaine.col > 0) {
		col = panel->prochaine.col - 1; lig = panel->prochaine.lig - 1;
	} else if(panel->mode & PNL_BYLINES) {
		col = panel->cur % panel->nbcols; lig = panel->cur / panel->nbcols;
	} else {
		col = panel->cur / panel->maxligs; lig = panel->cur % panel->maxligs;
	}
	numitem = (col * panel->maxligs) + lig; */
	if(panel->prochaine.col > 0)
		// numitem = ((panel->prochaine.col - 1) * panel->maxligs) + (panel->prochaine.lig - 1);
		numitem = PanelItemNum(panel,panel->prochaine.col - 1,panel->prochaine.lig - 1);
	else if(panel->mode & PNL_BYLINES)
		numitem = (panel->cur / panel->nbcols) + ((panel->cur % panel->nbcols) * panel->maxligs);
	else numitem = panel->cur;

	if(DEBUG_PANEL(1)) {
		if(panel->prochaine.col > 0) {
			printf("(%s) Item #%d (%s): place en %d x %d dans un tableau %d x %d\n",__func__,
				numitem,texte?texte:"anonyme",panel->prochaine.col-1,panel->prochaine.lig-1,panel->nbcols,panel->maxligs);
		} else {
			printf("(%s) Item #%d: %s @%08lX dans panel de %d lignes x %d cols (%d total): position %d\n",
				   __func__,numitem,texte?texte:"anonyme",(Ulong)adrs,panel->maxligs,panel->nbcols,panel->max,panel->cur);
		}
	}

	if(numitem >= panel->max) return(0);
	item = panel->items + numitem;
	item->num = numitem;
//	item->texte = texte;
	if(texte) item->texte = (char *)malloc((strlen(texte) + 1) * sizeof(char)); else item->texte = 0;
	if(item->texte) strcpy(item->texte,texte);
	item->traduit = item->texte;
	item->sel = item->mod = 1;
	item->souligne = item->barreDR = item->barreMI = item->barreGA = 0;
	item->type = (char)type;
	item->adrs = adrs;
	item->limites = 0;
	item->etat = PNL_MODIFIABLE;
	item->coultexte = 0;
	item->gctexte = 0;
	item->gc = 0;
	item->nbfonds = 0;
	item->numfond = -1;
	switch(item->type) {
	  case PNL_FLOAT: case PNL_DBLE:
		item->echelle.r.min = 0.0;
		item->echelle.r.total = 1.0;
		break;
	  default:
		item->echelle.i.min = 0;
		item->echelle.i.total = 1;
		break;
	}
	item->tempo = 0;
	if((max < 0) || (max >= 256)) max = 255;
	item->maxuser = (short)max + 1;
	if(((max == 0) || (max > PanelVarLngrMaxi)) && adrs) item->maxdisp = PanelVarLngrMaxi;
	else item->maxdisp = (short)max;
	item->first = 0;
	item->valeur[0] = '\0';
	item->ptrval = item->valeur;
	item->ptrdim = 0;
	item->exit = 0;
	strncpy(item->format,format,PNL_MAXFMT);
	if(panel->prochaine.col > 0) {
		if(numitem > panel->cur) panel->cur = (short)numitem;
		if(panel->prochaine.lig > panel->nbligs) panel->nbligs = panel->prochaine.lig;
		if(panel->mode & PNL_BYLINES) {
			if(panel->prochaine.col < panel->nbcols) panel->prochaine.col += 1;
			else { panel->prochaine.col = 1; panel->prochaine.lig += 1; }
		} else {
			if(panel->prochaine.lig < panel->nbligs) panel->prochaine.lig += 1;
			else { panel->prochaine.lig = 1; panel->prochaine.col += 1; }
		}
	} else {
		panel->cur += 1;
		panel->nbligs = ((panel->cur - 1) / panel->nbcols) + 1;
		if(DEBUG_PANEL(1)) printf("(%s) item #%d: cols=%d soit ligs=%d\n",__func__,panel->cur,panel->nbcols,panel->nbligs);
	}
	panel->prochaine.col = 0;
	return(numitem + 1);
}
/* ========================================================================== */
int PanelFormat(Panel panel, int num, char *format) {
	return(PanelItemFormat(panel,num,format));
}
/* ========================================================================== */
int PanelLngr(Panel panel, int num, int lngr) {
	return(PanelItemLngr(panel,num,lngr));
}
/* ========================================================================== */
int PanelItemMax(Panel panel) { if(panel) return(panel->max); else return(0); }
/* ========================================================================== */
int PanelItemNb(Panel panel) { if(panel) return(panel->cur); else return(0); }
/* ========================================================================== */
char PanelItemIsSelected(Panel panel, int num) {
	if(panel) {
		if((num > 0) && (num <= panel->max)) return(panel->items[num - 1].sel);
		else return(0);
	} else return(0);
}
/* ========================================================================== */
char PanelItemEstModifie(Panel panel, int num) {
	if(!panel) return(0);
	if((num < 1) || (num > panel->max)) return(0);
	return(panel->items[num - 1].etat == PNL_MODIFIE);
}
/* ========================================================================== */
void PanelItemChangeTexte(Panel panel, int num, char *texte) {
	int l;

	if(!panel) return;
	if((num < 1) || (num > panel->max)) return;
	l = (panel->items[num - 1].ptrdim)? panel->items[num - 1].ptrdim: PNL_MAXVAL;
	--l;
	strncpy(panel->items[num - 1].ptrval,texte,l);
	panel->items[num - 1].ptrval[l] = '\0';
	panel->items[num - 1].etat = PNL_MODIFIE;
}
/* ========================================================================== */
void PanelItemRelit(Panel panel, int num) {
	int poub;

	if(!panel) return;
	if((num < 1) || (num > panel->max)) return;
	poub = 0;
	PanelCode(&(panel->items[num-1]),PNL_READ,0,&poub);
	(panel->cdr)->var_modifiees = 1;
}
/* ========================================================================== */
int PanelItemFormat(Panel panel, int num, char *format) {
	if(!panel) return(0);
	if((num < 1) || (num > panel->max)) return(0);
	if(format) {
		if(format[0]) strncpy((panel->items[num - 1]).format,format,PNL_MAXFMT);
	}
	return(num);
}
/* ========================================================================== */
int PanelItemLngr(Panel panel, int num, int lngr) {
	if(!panel) return(0);
	if((num < 1) || (num > panel->max)) return(0);
	if((lngr > 0) && (lngr <= PanelVarLngrMaxi)) {
		(panel->items[num - 1]).maxuser = (short)lngr + 1;
		(panel->items[num - 1]).maxdisp = (short)lngr;
	}
	return(num);
}
/* ========================================================================== */
int PanelItemSelect(Panel panel, int num, char sel) {
	if(!panel) return(0);
	if((num > 0) && (num <= panel->max)) {
		panel->items[num - 1].sel = sel;
		if((panel->mode & PNL_RDONLY) == 0) panel->items[num - 1].mod = sel;
		panel->items[num - 1].etat = sel? PNL_MODIFIABLE: PNL_PROTEGE;
	}
	return(num);
}
/* ========================================================================== */
int PanelItemSouligne(Panel panel, int num, char souligne) {
	if(!panel) return(0);
	if((num > 0) && (num <= panel->max)) panel->items[num - 1].souligne = souligne;
	return(num);
}
/* ========================================================================== */
int PanelItemBarreDroite(Panel panel, int num, char trace) {
	if(!panel) return(0);
	if((num > 0) && (num <= panel->max)) panel->items[num - 1].barreDR = trace;
	return(num);
}
/* ========================================================================== */
int PanelItemBarreMilieu(Panel panel, int num, char trace) {
	if(!panel) return(0);
	if((num > 0) && (num <= panel->max)) panel->items[num - 1].barreMI = trace;
	return(num);
}
/* ========================================================================== */
int PanelItemBarreGauche(Panel panel, int num, char trace) {
	if(!panel) return(0);
	if((num > 0) && (num <= panel->max)) panel->items[num - 1].barreGA = trace;
	return(num);
}
/* ========================================================================== */
int PanelItemIScale(Panel panel, int num, int min, int total) {
	PanelItem item;

	if(!panel) return(0);
	if((num < 1) || (num > panel->max)) return(0);
	item = panel->items + num - 1;
	switch(item->type) {
		case PNL_PSWD: case PNL_FILE: case PNL_TEXTE:
		case PNL_LONG: case PNL_LHEX: case PNL_INT: case PNL_HEX:
		case PNL_SHORT: case PNL_SHEX: case PNL_ADU: case PNL_CHAR: case PNL_OCTET: case PNL_BYTE:
		case PNL_LISTE: case PNL_LISTL: case PNL_LISTD: case PNL_LISTS: case PNL_LISTB:
		case PNL_KEY: case PNL_KEYL: case PNL_KEYS: case PNL_KEYB:
			item->echelle.i.min = min;
			item->echelle.i.total = total;
			break;
		case PNL_FLOAT: case PNL_DBLE:
			item->echelle.r.min = (float)min;
			item->echelle.r.total = (float)total;
			break;
	}
	return(num);
}
/* ========================================================================== */
int PanelItemRScale(Panel panel, int num, float min, float total) {
	PanelItem item;

	if(!panel) return(0);
	if((num < 1) || (num > panel->max)) return(0);
	item = panel->items + num - 1;
	switch(item->type) {
		case PNL_PSWD: case PNL_FILE: case PNL_TEXTE:
		case PNL_LONG: case PNL_LHEX: case PNL_INT: case PNL_HEX:
		case PNL_SHORT: case PNL_SHEX: case PNL_ADU: case PNL_CHAR: case PNL_OCTET: case PNL_BYTE:
		case PNL_LISTE: case PNL_LISTL: case PNL_LISTD: case PNL_LISTS: case PNL_LISTB:
		case PNL_KEY: case PNL_KEYL: case PNL_KEYS: case PNL_KEYB:
			item->echelle.i.min = (int)min;
			item->echelle.i.total = (int)total;
			break;
		case PNL_FLOAT: case PNL_DBLE:
			item->echelle.r.min = min;
			item->echelle.r.total = total;
			break;
	}
	return(num);
}
/* ========================================================================== */
int PanelItemTextColor(Panel panel, int num, OpiumColor *coul) {
	if(!panel) return(0);
	if((num > 0) && (num <= panel->max)) {
		PanelItem item; Cadre cdr; WndFrame f;
		item = panel->items + (num  - 1);
		cdr = panel->cdr;
		if((f = cdr->f) && item->gctexte) {
			WndContextFree(f,item->gctexte); item->gctexte = 0;
		}
		item->coultexte = coul;
	}
	return(num);
}
/* ========================================================================== */
int PanelItemColors(Panel panel, int num, OpiumColor *fond, int nb) {
	if(!panel) return(0);
	if((num > 0) && (num <= panel->max)) {
		PanelItem item; Cadre cdr; WndFrame f; int i;
		item = panel->items + num - 1;
		if(item->gc) {
			cdr = panel->cdr;
			if((f = cdr->f)) for(i=0; i<item->nbfonds; i++) if(item->gc[i]) {
				WndContextFree(f,item->gc[i]); item->gc[i] = 0;
			}
			if((nb != item->nbfonds) || !nb) {
				free(item->gc); item->gc = 0;
				item->nbfonds = 0;
			}
		}
		if(!item->gc && nb) {
			item->gc = (WndContextPtr *)malloc(nb * sizeof(WndContextPtr));
			if(item->gc) {
				item->nbfonds = (short)nb;
				for(i=0; i<item->nbfonds; i++) item->gc[i] = 0;
				item->souligne = -1;
			}
		}
		item->fond = fond;
	}
	return(num);
}
/* ========================================================================== */
int PanelItemModif(Panel panel, int num, TypePanelFctnMod fctn, void *arg) {
	if(!panel) return(0);
	if((num > 0) && (num <= panel->max)) {
		panel->items[num - 1].modif = fctn;
		panel->items[num - 1].modif_arg = arg;
	}
	return(num);
}
/* ========================================================================== */
int PanelItemReturn(Panel panel, int num, TypePanelFctnItem fctn, void *arg) {
	if(!panel) return(0);
	if((num > 0) && (num <= panel->max)) {
		panel->items[num - 1].rtn = (TypePanelFctnRef)fctn;
		panel->items[num - 1].rtn_arg = arg;
	}
	return(num);
}
/* ========================================================================== */
int PanelItemExit(Panel panel, int num, TypePanelFctnItem fctn, void *arg) {
	if(!panel) return(0);
	if((num > 0) && (num <= panel->max)) {
		panel->items[num - 1].exit = (TypePanelFctnRef)fctn;
		panel->items[num - 1].exit_arg = arg;
	}
	return(num);
}
/* ========================================================================== */
void *PanelItemArg(Panel panel, int num) {
	if(!panel) return(0);
	if((num > 0) && (num <= panel->max))
		return(panel->items[num - 1].exit_arg);
	else return(0);
}
/* ========================================================================== */
int PanelItemILimits(Panel panel, int num, int min, int max) {
	PanelItem item;

	if(!panel) return(0);
	if((num < 1) || (num > panel->max)) return(0);
	item = panel->items + num - 1;
	switch(item->type) {
	  case PNL_LONG: case PNL_LHEX: case PNL_INT: case PNL_HEX:
	  case PNL_SHORT: case PNL_SHEX: case PNL_ADU: case PNL_CHAR: case PNL_OCTET: case PNL_BYTE:
	  case PNL_LISTE: case PNL_LISTL: case PNL_LISTD: case PNL_LISTS: case PNL_LISTB:
	  case PNL_KEY: case PNL_KEYL: case PNL_KEYS: case PNL_KEYB:
		item->lim.i.min = min;
		item->lim.i.max = max;
		item->limites = 1;
		break;
	  case PNL_FLOAT: case PNL_DBLE:
		  item->lim.r.min = (float)min;
		  item->lim.r.max = (float)max;
		  item->limites = 1;
		  break;
	}
	return(num);
}
/* ========================================================================== */
int PanelItemRLimits(Panel panel, int num, float min, float max) {
	PanelItem item;

	if(!panel) return(0);
	if((num < 1) || (num > panel->max)) return(0);
	item = panel->items + num - 1;
	switch(item->type) {
		case PNL_LONG: case PNL_LHEX: case PNL_INT: case PNL_HEX:
		case PNL_SHORT: case PNL_SHEX: case PNL_ADU: case PNL_CHAR: case PNL_OCTET: case PNL_BYTE:
		case PNL_LISTE: case PNL_LISTL: case PNL_LISTD: case PNL_LISTS: case PNL_LISTB:
		case PNL_KEY: case PNL_KEYL: case PNL_KEYS: case PNL_KEYB:
			item->lim.i.min = (int)min;
			item->lim.i.max = (int)max;
			item->limites = 1;
			break;
		case PNL_FLOAT: case PNL_DBLE:
		item->lim.r.min = min;
		item->lim.r.max = max;
		item->limites = 1;
		break;
	}
	return(num);
}
/* ========================================================================== */
int PanelItemNoLimits(Panel panel, int num) {
	if(!panel) return(0);
	if((num < 1) || (num > panel->max)) return(0);
	(panel->items[num - 1]).limites = 0;
	return(num);
}
/* ========================================================================== */
/* ========================================================================== */
char PanelCode(PanelItem item, char etape, char *texte, int *col) {
	void *adrs; int entier;
	char valeur[256],cle[256],*traduit,*ptrlec,*ptrecr;
	char **liste; char *modeles; TypeArgUserFctn fctn_user; TypePanelFctnButn fctn_butn;
	int i,l,k,m,n; float x; int curs;
	char lit,ecrit,modifie,recherche;
	int bon_debut,trouve;
	int64 i64; short court; char carac,booleen,sep; unsigned char octet;
	char *c,*debut,*svt;
	// char (*fctn_item)(void *);
	char suivi;

	if(item->type < 0) return(0);
	modifie = 0;
	ptrlec = ptrecr = 0; /* pour gcc */
	lit = ecrit = 0;	 /* pour gcc */
	traduit = 0;         /* pour gcc */
	adrs = item->adrs;
	suivi = (adrs == ArgAdrsSuivie);
	curs = col? *col: 0;
	switch(etape) {
	  case PNL_WRITE:
		lit = 0;
		ecrit = 1; ptrecr = valeur;
		modifie = 1;
		item->etat = PNL_MODIFIABLE;
		break;
	  case PNL_VERIFY:
		if(!texte) return(0);
		if(!strcmp(texte,item->ptrval)) return(0);
		lit = 1;   ptrlec = texte;
		ecrit = 1; ptrecr = valeur;
		modifie = 1;
		if(DEBUG_PANEL(5)) WndPrint("(%s) verifie %s: texte='%s'\n",__func__,item->texte,texte);
		break;
	  case PNL_READ:
		lit = item->mod && item->ecrit; ptrlec = item->ptrval;
		if(DEBUG_PANEL(5)) WndPrint("(%s) relit %s: ptrval='%s'\n",__func__,item->texte,item->ptrval);
		ecrit = 0;
		item->etat = PNL_MODIFIABLE;
		break;
	}

	if(!adrs) {
		if(ecrit) valeur[0] = '\0';
	} else  switch(item->type) {
	  case PNL_PSWD:
	  case PNL_FILE:
	  case PNL_TEXTE:
		switch(etape) {
		  case PNL_WRITE:   ptrecr = (char *)adrs; break;
		  case PNL_VERIFY:  ptrecr = texte; break;
		  case PNL_READ:    if(!item->mod || !item->ecrit) break;
			ptrecr = (char *)adrs;
			if(!(m = item->maxuser)) strcpy((char *)adrs,item->ptrval);
			else strncpy((char *)adrs,item->ptrval,m);
			break;
		}
		if(item->mod && item->ecrit && (item->maxuser > 0)) ptrecr[item->maxuser - 1] = '\0';
		// if(suivi) printf("(%s.2) suivi: %s = '%s'\n",__func__,ArgAdrsNom,ArgAdrsSuivie);
		if(item->nbfonds && item->echelle.i.total) {
			k = (int)ptrecr[0];
			item->numfond = (short)((item->nbfonds * (k - item->echelle.i.min)) / item->echelle.i.total);
			if(item->numfond < 0) item->numfond = 0;
			else if(item->numfond >= item->nbfonds) item->numfond = item->nbfonds - 1;
			// printf("(%s) '%s' -> k=%d (%d [%d]), fond #%d/%d\n",__func__,ptrecr,k,item->echelle.i.min,item->echelle.i.total,item->numfond,item->nbfonds);
		}
		break;
	  case PNL_LONG:
	  case PNL_LHEX:
		if(etape == PNL_VERIFY) adrs = (void *)&i64;
		if(lit) {
			if(ptrlec[0] == '\0') *(int64 *)adrs = 0;
			else if(!strcmp(ptrlec,"-")) *(int64 *)adrs = -1;
			else if(item->type == PNL_LONG) sscanf(ptrlec,"%lld",(int64 *)adrs);
			else sscanf(ptrlec,"%llx",(long unsigned long *)adrs);
		}
		if(item->limites) {
			if(*(int64 *)adrs < item->lim.i.min) *(int64 *)adrs = item->lim.i.min;
			else if(*(int64 *)adrs > item->lim.i.max) *(int64 *)adrs = item->lim.i.max;
		}
		if(item->nbfonds && item->echelle.i.total) {
			k = (int)*(int64 *)adrs;
			item->numfond = (short)((item->nbfonds * (k - item->echelle.i.min)) / item->echelle.i.total);
			if(item->numfond < 0) item->numfond = 0;
			else if(item->numfond >= item->nbfonds) item->numfond = item->nbfonds - 1;
		}
		if(ecrit) sprintf(ptrecr,item->format,*(int64 *)adrs);
		break;
	  case PNL_INT:
	  case PNL_HEX:
		if(etape == PNL_VERIFY) adrs = (void *)&entier;
		if(lit) {
			if(ptrlec[0] == '\0') *(int *)adrs = 0;
			else if(!strcmp(ptrlec,"-")) *(int *)adrs = -1;
			else if(item->type == PNL_INT) sscanf(ptrlec,"%d",(int *)adrs);
			else sscanf(ptrlec,"%x",(int *)adrs);
		}
		if(item->limites) {
			if(*(int *)adrs < item->lim.i.min) *(int *)adrs = item->lim.i.min;
			else if(*(int *)adrs > item->lim.i.max) *(int *)adrs = item->lim.i.max;
		}
		if(item->nbfonds && item->echelle.i.total) {
			k = *(int *)adrs;
			item->numfond = (short)((item->nbfonds * (k - item->echelle.i.min)) / item->echelle.i.total);
			if(item->numfond < 0) item->numfond = 0;
			else if(item->numfond >= item->nbfonds) item->numfond = item->nbfonds - 1;
		}
		if(ecrit) sprintf(ptrecr,item->format,*(int *)adrs);
		break;
	  case PNL_SHORT:
	  case PNL_SHEX:
	  case PNL_ADU:
		if(etape == PNL_VERIFY) adrs = (void *)&court;
		if(lit) {
			if(ptrlec[0] == '\0') *(short *)adrs = 0;
			else if(!strcmp(ptrlec,"-")) *(short *)adrs = -1;
			else if(item->type != PNL_SHEX) sscanf(ptrlec,"%hd",(short *)adrs);
			else sscanf(ptrlec,"%hx",(unsigned short *)adrs);
		}
		if((item->type == PNL_ADU) && !lit) {
			if(*(short *)adrs & 0x8000) *(short *)adrs &= 0x7FFF;
			else *(short *)adrs |= 0x8000;
		}
		if(item->limites) {
			if(*(short *)adrs < item->lim.i.min) *(short *)adrs = (short)item->lim.i.min;
			else if(*(short *)adrs > item->lim.i.max) *(short *)adrs = (short)item->lim.i.max;
		}
		if(item->nbfonds && item->echelle.i.total) {
			k = (int)*(short *)adrs;
			item->numfond = (short)((item->nbfonds * (k - item->echelle.i.min)) / item->echelle.i.total);
			if(item->numfond < 0) item->numfond = 0;
			else if(item->numfond >= item->nbfonds) item->numfond = item->nbfonds - 1;
		}
		if(ecrit) sprintf(ptrecr,item->format,*(short *)adrs);
		if((item->type == PNL_ADU) && lit) {
			if(*(short *)adrs & 0x8000) *(short *)adrs &= 0x7FFF;
			else *(short *)adrs |= 0x8000;
		}
		break;
	  case PNL_CHAR:
	  case PNL_LETTRE:
		if(etape == PNL_VERIFY) adrs = (void *)&carac;
		if(lit) {
			if(item->type == PNL_CHAR) {
				if(ptrlec[0] == '\0') *(char *)adrs = 0;
				else if(!strcmp(ptrlec,"-")) *(char *)adrs = -1;
				else {
					sscanf(ptrlec,"%d",&entier);
					*(char *)adrs = (entier & 0xFF);
				}
			} else *(char *)adrs = *ptrlec;
		}
		if(item->limites) {
			if(*(char *)adrs < item->lim.i.min) *(char *)adrs = (char)item->lim.i.min;
			else if(*(char *)adrs > item->lim.i.max) *(char *)adrs = (char)item->lim.i.max;
		}
		if(item->nbfonds && item->echelle.i.total) {
			k = (int)*(char *)adrs;
			item->numfond = (short)((item->nbfonds * (k - item->echelle.i.min)) / item->echelle.i.total);
			if(item->numfond < 0) item->numfond = 0;
			else if(item->numfond >= item->nbfonds) item->numfond = item->nbfonds - 1;
		}
		if(ecrit) sprintf(ptrecr,item->format,*(char *)adrs);
		break;
	  case PNL_OCTET:
	  case PNL_BYTE:
		if(etape == PNL_VERIFY) adrs = (void *)&octet;
		if(lit) {
			if(ptrlec[0] == '\0') *(unsigned char *)adrs = 0;
			else if(!strcmp(ptrlec,"-")) *(unsigned char *)adrs = -1;
			else {
				if(item->type == PNL_OCTET) sscanf(ptrlec,"%d",&entier);
				else sscanf(ptrlec,"%x",&entier);
				*(unsigned char *)adrs = (entier & 0xFF);
			}
		}
		if(item->limites) {
			if(*(unsigned char *)adrs < item->lim.i.min) *(unsigned char *)adrs = (unsigned char)(item->lim.i.min);
			else if(*(unsigned char *)adrs > item->lim.i.max) *(unsigned char *)adrs = (unsigned char)(item->lim.i.max);
		}
		if(item->nbfonds && item->echelle.i.total) {
			k = (int)*(unsigned char *)adrs;
			item->numfond = (short)(item->nbfonds * (k - item->echelle.i.min)) / item->echelle.i.total;
			if(item->numfond < 0) item->numfond = 0;
			else if(item->numfond >= item->nbfonds) item->numfond = item->nbfonds - 1;
		}
		if(ecrit) sprintf(ptrecr,item->format,*(unsigned char *)adrs);
		break;
	  case PNL_BOOL:
		if(etape == PNL_VERIFY) adrs = (void *)&booleen;
		if(lit) {
			k = curs; if(!k) k = (int)strlen(ptrlec);
			if(ptrlec[k - 1] == '?') {
				if(*(char *)adrs) *(char *)adrs = 0;
				else *(char *)adrs = 1;
			} else *(char *)adrs = (ptrlec[0] == 'o') || (ptrlec[0] == 'O')
			|| (ptrlec[0] == 's') || (ptrlec[0] == 'S')
			|| (ptrlec[0] == 'y') || (ptrlec[0] == 'Y')
			|| (ptrlec[0] == 'j') || (ptrlec[0] == 'J') || (ptrlec[0] == '1') ;
		}
		if(item->nbfonds >= 2) item->numfond = (short)*(char *)adrs;
		if(ecrit) sprintf(ptrecr,item->format,(*(char *)adrs)? OpiumOui: OpiumNon);
		break;
	  case PNL_FLOAT:
		if(etape == PNL_VERIFY) /* adrs = (void *)(&reel); */ { ptrecr = texte; break; }
		if(lit) {
			if(ptrlec[0] == '\0') *(float *)adrs = 0.0;
			else if(!strcmp(ptrlec,"-")) *(float *)adrs = -1.0;
			else sscanf(ptrlec,"%g",(float *)adrs);
		}
		if(item->limites) {
			if(*(float *)adrs < item->lim.r.min) *(float *)adrs = item->lim.r.min;
			else if(*(float *)adrs > item->lim.r.max) *(float *)adrs = item->lim.r.max;
		}
		if(item->nbfonds && (item->echelle.r.total != 0.0)) {
			x = *(float *)adrs;
			item->numfond = (short)((float)item->nbfonds * (x - item->echelle.r.min) / item->echelle.r.total);
			if(item->numfond < 0) item->numfond = 0;
			else if(item->numfond >= item->nbfonds) item->numfond = item->nbfonds - 1;
		}
		if(ecrit) {
			snprintf(ptrecr,256,item->format,*(float *)adrs);
		}
		break;
	  case PNL_DBLE:
		if(etape == PNL_VERIFY) /* adrs = (void *)(&dble); */ { ptrecr = texte; break; }
		if(lit) {
			if(ptrlec[0] == '\0') *(double *)adrs = 0.0;
			else if(!strcmp(ptrlec,"-")) *(double *)adrs = -1.0;
			else sscanf(ptrlec,"%lg",(double *)adrs);
		}
		if(item->limites) {
			if(*(double *)adrs < item->lim.r.min) *(double *)adrs = item->lim.r.min;
			else if(*(double *)adrs > item->lim.r.max) *(double *)adrs = item->lim.r.max;
		}
		if(item->nbfonds && (item->echelle.r.total != 0.0)) {
			x = (float)*(double *)adrs;
			item->numfond = (short)((float)item->nbfonds * (x - item->echelle.r.min) / item->echelle.r.total);
			if(item->numfond < 0) item->numfond = 0;
			else if(item->numfond >= item->nbfonds) item->numfond = item->nbfonds - 1;
		}
		if(ecrit) {
			snprintf(ptrecr,256,item->format,*(double *)adrs);
		}
		break;
	  case PNL_LISTE:
		liste = (char **)item->fctn;
		if(etape == PNL_VERIFY) adrs = (void *)&(item->tempo);
		if(lit) {
			k = curs;
			if(!k
			#ifdef WIN32
				|| (k == 999)
			#endif
				) k = (int)strlen(ptrlec);
			if(ptrlec[k - 1] == '?') {
				entier = m = *(int *)adrs;
				if(liste[entier][0] == '\0') entier = 0;
				if(liste[entier][0] != '\0') while(++entier != m) {
					if(liste[entier][0] == '\0') entier = 0;
					if(item->limites) {
						if((entier < item->lim.i.min) || (entier > item->lim.i.max)) continue;
					}
					if(k < 2) break;
					if(!strncmp(ptrlec,liste[entier],k - 1)) break;
				}
				*(int *)adrs = entier;
				if(DEBUG_PANEL(1)) WndPrint("variable: %d\n",*(int *)adrs);
				if(col) *col = k - 1;
			} else {
				if(item->limites) entier = item->lim.i.min - 1; else entier = -1;
				trouve = -1; bon_debut = -1;
				while(liste[++entier][0] != '\0') {
					if(DEBUG_PANEL(1))
						WndPrint("liste[%d]('%s') =? '%s'\n",entier,liste[entier],ptrlec);
					if(item->limites) { if(entier > item->lim.i.max) break; }
					traduit = liste[entier]; //- if(etape == PNL_READ) DicoTraduit(&OpiumDico,&traduit);
					if(!strcmp(ptrlec,traduit)) { trouve = entier; break; }
					if(!strncmp(ptrlec,traduit,k) && (bon_debut < 0)) bon_debut = entier;
				}
				if(DEBUG_PANEL(1)) WndPrint("liste %s terminee\n",liste[entier][0]?"non":"toute");
				if(trouve < 0) trouve = bon_debut;
				if(trouve >= 0) *(int *)adrs = trouve; else if(col && *col) *col -= 1;
				if(DEBUG_PANEL(1)) WndPrint("entier: %d, variable: %d\n",trouve,*(int *)adrs);
			}
		}
		if(item->nbfonds) {
			item->numfond = (short)*(int *)adrs;
			if(item->numfond < 0) item->numfond = 0;
			else if(item->numfond >= item->nbfonds) item->numfond = item->nbfonds - 1;
		}
		// if(ecrit) { l = *(int *)adrs; ptrecr = liste[l]; }
		if(ecrit) {
			l = *(int *)adrs; entier = 0;
			if(l >= 0) {
				while(liste[entier] && (liste[entier][0] != '\0')) if(l <= entier++) break;
				if(l < entier) ptrecr = liste[l]; else l = -1;
			}
			if(l < 0) { *(int *)adrs = 0; strcpy(ptrecr,OpiumListeInvl); }
			else if(entier < 1) { *(int *)adrs = 0; strcpy(ptrecr,OpiumListeVide); }
		}
		break;
	  case PNL_LISTL:
		liste = (char **)item->fctn;
		if(etape == PNL_VERIFY) adrs = (void *)&(item->tempo);
		if(lit) {
			k = curs; if(!k) k = (int)strlen(ptrlec);
			if(ptrlec[k - 1] == '?') {
				entier = m = (int)(*(long *)adrs);
				if(liste[entier][0] == '\0') entier = 0;
				if(liste[entier][0] != '\0') while(++entier != m) {
					if(liste[entier][0] == '\0') entier = 0;
					if(item->limites) {
						if((entier < item->lim.i.min) || (entier > item->lim.i.max)) continue;
					}
					if(k < 2) break;
					if(!strncmp(ptrlec,liste[entier],k - 1)) break;
				}
				*(long *)adrs = entier;
				if(col) *col = k - 1;
			} else {
				if(item->limites) entier = item->lim.i.min - 1; else entier = -1;
				trouve = -1; bon_debut = -1;
				while(liste[++entier][0] != '\0') {
					if(item->limites) { if(entier > item->lim.i.max) break; }
					traduit = liste[entier]; //- if(etape == PNL_READ) DicoTraduit(&OpiumDico,&traduit);
					if(!strcmp(ptrlec,traduit)) { trouve = entier; break; }
					if(!strncmp(ptrlec,traduit,k) && (bon_debut < 0)) bon_debut = entier;
				}
				if(trouve < 0) trouve = bon_debut;
				if(trouve >= 0) *(long *)adrs = trouve; else if(col && *col) *col -= 1;
			}
		}
		if(item->nbfonds) {
			item->numfond = (short)*(long *)adrs;
			if(item->numfond < 0) item->numfond = 0;
			else if(item->numfond >= item->nbfonds) item->numfond = item->nbfonds - 1;
		}
		// if(ecrit) { l = *(long *)adrs; ptrecr = liste[l]; }
		if(ecrit) {
			l = (int)(*(long *)adrs);
			entier = 0;
			if(l >= 0) {
				while(liste[entier] && (liste[entier][0] != '\0')) if(l <= entier++) break;
				if(l < entier) ptrecr = liste[l]; else l = -1;
			}
			if(l < 0) { *(long *)adrs = 0; strcpy(ptrecr,OpiumListeInvl); }
			else if(entier < 1) { *(int *)adrs = 0; strcpy(ptrecr,OpiumListeVide); }
		}
		break;
	  case PNL_LISTD:
		liste = (char **)item->fctn;
		if(etape == PNL_VERIFY) adrs = (void *)&(item->tempo);
		if(lit) {
			k = curs; if(!k) k = (int)strlen(ptrlec);
			if(ptrlec[k - 1] == '?') {
				entier = m = (int)(*(int64 *)adrs);
				if(liste[entier][0] == '\0') entier = 0;
				if(liste[entier][0] != '\0') while(++entier != m) {
					if(liste[entier][0] == '\0') entier = 0;
					if(item->limites) {
						if((entier < item->lim.i.min) || (entier > item->lim.i.max)) continue;
					}
					if(k < 2) break;
					if(!strncmp(ptrlec,liste[entier],k - 1)) break;
				}
				*(int64 *)adrs = entier;
				if(col) *col = k - 1;
			} else {
				if(item->limites) entier = item->lim.i.min - 1; else entier = -1;
				trouve = -1; bon_debut = -1;
				while(liste[++entier][0] != '\0') {
					if(item->limites) { if(entier > item->lim.i.max) break; }
					traduit = liste[entier]; //- if(etape == PNL_READ) DicoTraduit(&OpiumDico,&traduit);
					if(!strcmp(ptrlec,traduit)) { trouve = entier; break; }
					if(!strncmp(ptrlec,traduit,k) && (bon_debut < 0)) bon_debut = entier;
				}
				if(trouve < 0) trouve = bon_debut;
				if(trouve >= 0) *(int64 *)adrs = trouve; else if(col && *col) *col -= 1;
			}
		}
		if(item->nbfonds) {
			item->numfond = (short)*(int64 *)adrs;
			if(item->numfond < 0) item->numfond = 0;
			else if(item->numfond >= item->nbfonds) item->numfond = item->nbfonds - 1;
		}
		// if(ecrit) { l = *(int64 *)adrs; ptrecr = liste[l]; }
		if(ecrit) {
			l = (int)(*(int64 *)adrs);
			entier = 0;
			if(l >= 0) {
				while(liste[entier] && (liste[entier][0] != '\0')) if(l <= entier++) break;
				if(l < entier) ptrecr = liste[l]; else l = -1;
			}
			if(l < 0) { *(int64 *)adrs = 0; strcpy(ptrecr,OpiumListeInvl); }
			else if(entier < 1) { *(int *)adrs = 0; strcpy(ptrecr,OpiumListeVide); }
		}
		break;
	  case PNL_LISTS:
		liste = (char **)item->fctn;
		if(etape == PNL_VERIFY) adrs = (void *)&(item->tempo);
		if(lit) {
			k = curs; if(!k) k = (int)strlen(ptrlec);
			if(ptrlec[k - 1] == '?') {
				entier = m = *(short *)adrs;
				if(liste[entier][0] == '\0') entier = 0;
				if(liste[entier][0] != '\0') while(++entier != m) {
					if(liste[entier][0] == '\0') entier = 0;
					if(item->limites) {
						if((entier < item->lim.i.min) || (entier > item->lim.i.max)) continue;
					}
					if(k < 2) break;
					if(!strncmp(ptrlec,liste[entier],k - 1)) break;
				}
				*(short *)adrs = (short)entier;
				if(col) *col = k - 1;
			} else {
				if(item->limites) entier = item->lim.i.min - 1; else entier = -1;
				trouve = -1; bon_debut = -1;
				while(liste[++entier][0] != '\0') {
					if(item->limites) { if(entier > item->lim.i.max) break; }
					traduit = liste[entier]; //- if(etape == PNL_READ) DicoTraduit(&OpiumDico,&traduit);
					if(!strcmp(ptrlec,traduit)) { trouve = entier; break; }
					if(!strncmp(ptrlec,traduit,k) && (bon_debut < 0)) bon_debut = entier;
				}
				if(trouve < 0) trouve = bon_debut;
				if(trouve >= 0) *(short *)adrs = (short)trouve; else if(col && *col) *col -= 1;
			}
		}
		if(item->nbfonds) {
			item->numfond = (short)*(short *)adrs;
			if(item->numfond < 0) item->numfond = 0;
			else if(item->numfond >= item->nbfonds) item->numfond = item->nbfonds - 1;
		}
		// if(ecrit) { l = *(short *)adrs; ptrecr = liste[l]; }
		if(ecrit) {
			l = *(short *)adrs;
			entier = 0;
			if(l >= 0) {
				while(liste[entier] && (liste[entier][0] != '\0')) if(l <= entier++) break;
				if(l < entier) ptrecr = liste[l]; else l = -1;
			}
			if(l < 0) { *(short *)adrs = 0; strcpy(ptrecr,OpiumListeInvl); }
			else if(entier < 1) { *(int *)adrs = 0; strcpy(ptrecr,OpiumListeVide); }
		}
		break;
	  case PNL_LISTB:
		liste = (char **)item->fctn;
		if(etape == PNL_VERIFY) adrs = (void *)&(item->tempo);
		if(lit) {
			k = curs; if(!k) k = (int)strlen(ptrlec);
			if(ptrlec[k - 1] == '?') {
				entier = m = *(char *)adrs;
				if(liste[entier][0] == '\0') entier = 0;
				if(liste[entier][0] != '\0') while(++entier != m) {
					if(liste[entier][0] == '\0') entier = 0;
					if(item->limites) {
						if((entier < item->lim.i.min) || (entier > item->lim.i.max)) continue;
					}
					if(k < 2) break;
					if(!strncmp(ptrlec,liste[entier],k - 1)) break;
				}
				*(char *)adrs = (char)entier;
				if(col) *col = k - 1;
			} else {
				if(item->limites) entier = item->lim.i.min - 1; else entier = -1;
				trouve = -1; bon_debut = -1;
				while(liste[++entier][0] != '\0') {
					if(item->limites) { if(entier > item->lim.i.max) break; }
					traduit = liste[entier]; //- if(etape == PNL_READ) DicoTraduit(&OpiumDico,&traduit);
					if(DEBUG_PANEL(5)) printf("(%s) Compare %s avec cle[%d]='%s'\n",__func__,ptrlec,entier,traduit);
					if(!strcmp(ptrlec,traduit)) { trouve = entier; break; }
					if(!strncmp(ptrlec,traduit,k) && (bon_debut < 0)) bon_debut = entier;
				}
				if(DEBUG_PANEL(5)) printf("(%s) Retient cle[%d]->%d\n",__func__,trouve,(trouve >= 0)? trouve: *(char *)adrs);
				if(trouve < 0) trouve = bon_debut;
				if(trouve >= 0) *(char *)adrs = (char)trouve; else if(col && *col) *col -= 1;
			}
		}
		if(item->nbfonds) {
			item->numfond = (short)*(char *)adrs;
			if(item->numfond < 0) item->numfond = 0;
			else if(item->numfond >= item->nbfonds) item->numfond = item->nbfonds - 1;
		}
		// if(ecrit) { l = *(char *)adrs; ptrecr = liste[l]; }
		if(ecrit) {
			l = *(char *)adrs;
			entier = 0;
			if(l >= 0) {
				while(liste[entier] && (liste[entier][0] != '\0')) if(l <= entier++) break;
				if(l < entier) ptrecr = liste[l]; else l = -1;
			}
			if(l < 0) { *(char *)adrs = 0; strcpy(ptrecr,OpiumListeInvl); }
			else if(entier < 1) { *(int *)adrs = 0; strcpy(ptrecr,OpiumListeVide); }
		}
		break;
	  case PNL_KEY:
		modeles = (char *)item->fctn; sep = '/';
		if(etape == PNL_VERIFY) adrs = (void *)&(item->tempo);
		if(lit) {
			k = curs; if(!k) k = (int)strlen(ptrlec);
			if(ptrlec[k - 1] == '?') {
				entier = m = *(int *)adrs;
				c = debut = modeles; i = -1; recherche = 0;
				while(1) {
					c++;
					if((*c == sep) || (*c == '\0')) {
						if(*c == '\0') {
							if(!recherche) { entier = 0; break; }
							svt = modeles;
						} else svt = c + 1;
						i++;
						if(recherche) {
							entier = i;
							if(entier == m) break;
							if(item->limites) {
								if((entier < item->lim.i.min) || (entier > item->lim.i.max)) continue;
							}
							if(k < 2) break;
							if(!strncmp(debut,ptrlec,k - 1)) break;
						}
						if(i == entier) recherche = 1;
						c = debut = svt;
						if(svt == modeles) i = -1;
					}
				}
				*(int *)adrs = entier;
				if(col) *col = k - 1;
			} else {
				l = 0;
				c = modeles; debut = c; entier = 0;
				recherche = !item->limites;
				while(*c) {
					c++;
					if((*c == sep) || (*c == '\0')) {
						// l = (k < 256)? k: c - debut;
						l = (int)(c - debut);
						if(DEBUG_PANEL(1))
							WndPrint("liste[%d]('%s') =? '%s'/%d\n",entier,debut,ptrlec,k);
						if(item->limites) {
							if((entier < item->lim.i.min) || (entier > item->lim.i.max)) recherche = 0;
							else recherche = 1;
						}
						if(recherche) {
							if(etape == PNL_READ) {
								strncpy(cle,debut,l); cle[l] = '\0'; traduit = cle; // DicoTraduit(&OpiumDico,&traduit);
							} else traduit = debut;
						}
						if(DEBUG_PANEL(1)) WndPrint("cle[%d] %s = '%s'\n",entier,recherche?"proposee":"inutilisee",traduit);
						if(k <= l) {
							n = (k > 0)? k: l;
							if(recherche && (!strncmp(ptrlec,traduit,n))) { l = -1; break; }
						}
						debut = c + 1; entier++;
					}
				}
				if(DEBUG_PANEL(1)) WndPrint("liste %s terminee\n",(l < 0)?"non":"toute");
				if(l < 0) *(int *)adrs = entier; else if(col && *col) *col -= 1;
				if(DEBUG_PANEL(1)) WndPrint("entier: %d, variable: %d\n",entier,*(int *)adrs);
			}
		}
		if(item->nbfonds) {
			item->numfond = (short)*(int *)adrs;
			if(item->numfond < 0) item->numfond = 0;
			else if(item->numfond >= item->nbfonds) item->numfond = item->nbfonds - 1;
		}
		if(ecrit) {
			entier = *(int *)adrs + 1;
			c = modeles; debut = c;
			while(*c) {
				c++;
				if((*c == sep) || (*c == '\0')) {
					if(!(--entier)) break;
					debut = c + 1;
				}
			}
			l = (int)(c - debut);
			if(l > 0) { strncpy(ptrecr,debut,l); ptrecr[l] = '\0'; }
			else if(l == 0) ptrecr[0] = '\0';
			else { *(int *)adrs = 0; strcpy(ptrecr,OpiumListeInvl); }
			if(DEBUG_PANEL(1)) WndPrint("variable: %d, valeur: '%s'\n",*(int *)adrs,ptrecr);
		}
		break;
	  case PNL_KEYL:
		modeles = (char *)item->fctn; sep = '/';
		if(etape == PNL_VERIFY) adrs = (void *)&(item->tempo);
		if(lit) {
			k = curs; if(!k) k = (int)strlen(ptrlec);
			if(ptrlec[k - 1] == '?') {
				entier = m = (int)(*(int64 *)adrs);
				c = debut = modeles; i = -1; recherche = 0;
				while(1) {
					c++;
					if((*c == sep) || (*c == '\0')) {
						if(*c == '\0') {
							if(!recherche) { entier = 0; break; }
							svt = modeles;
						} else svt = c + 1;
						i++;
						if(recherche) {
							entier = i;
							if(entier == m) break;
							if(item->limites) {
								if((entier < item->lim.i.min) || (entier > item->lim.i.max)) continue;
							}
							if(k < 2) break;
							if(!strncmp(debut,ptrlec,k - 1)) break;
						}
						if(i == entier) recherche = 1;
						c = debut = svt;
						if(svt == modeles) i = -1;
					}
				}
				*(int64 *)adrs = entier;
				if(col) *col = k - 1;
			} else {
				l = 0;
				c = modeles; debut = c; entier = 0;
				recherche = !item->limites;
				while(*c) {
					c++;
					if((*c == sep) || (*c == '\0')) {
						// l = (k < 256)? k: c - debut;
						l = (int)(c - debut);
						if(item->limites) {
							if((entier < item->lim.i.min) || (entier > item->lim.i.max)) recherche = 0;
							else recherche = 1;
						}
						if(recherche) {
							if(etape == PNL_READ) {
								strncpy(cle,debut,l); cle[l] = '\0'; traduit = cle; //- DicoTraduit(&OpiumDico,&traduit);
							} else traduit = debut;
						}
						if(k <= l) {
							n = (k > 0)? k: l;
							if(recherche && (!strncmp(ptrlec,traduit,n))) { l = -1; break; }
						}
						debut = c + 1; entier++;
					}
				}
				if(l < 0) *(int64 *)adrs = entier; else if(col && *col) *col -= 1;
			}
		}
		if(item->nbfonds) {
			item->numfond = (short)*(int64 *)adrs;
			if(item->numfond < 0) item->numfond = 0;
			else if(item->numfond >= item->nbfonds) item->numfond = item->nbfonds - 1;
		}
		if(ecrit) {
			entier = (int)(*(int64 *)adrs + 1);
			c = modeles; debut = c;
			while(*c) {
				c++;
				if((*c == sep) || (*c == '\0')) {
					if(!(--entier)) break;
					debut = c + 1;
				}
			}
			l = (int)(c - debut);
			if(l > 0) { strncpy(ptrecr,debut,l); ptrecr[l] = '\0'; }
			else if(l == 0) ptrecr[0] = '\0';
			else { *(int64 *)adrs = 0; strcpy(ptrecr,OpiumListeInvl); }
		}
		break;
	  case PNL_KEYS:
		modeles = (char *)item->fctn; sep = '/';
		if(etape == PNL_VERIFY) adrs = (void *)&(item->tempo);
		if(lit) {
			k = curs; if(!k) k = (int)strlen(ptrlec);
			if(ptrlec[k - 1] == '?') {
				entier = m = *(short *)adrs;
				c = debut = modeles; i = -1; recherche = 0;
				while(1) {
					c++;
					if((*c == sep) || (*c == '\0')) {
						if(*c == '\0') {
							if(!recherche) { entier = 0; break; }
							svt = modeles;
						} else svt = c + 1;
						i++;
						if(recherche) {
							entier = i;
							if(entier == m) break;
							if(item->limites) {
								if((entier < item->lim.i.min) || (entier > item->lim.i.max)) continue;
							}
							if(k < 2) break;
							if(!strncmp(debut,ptrlec,k - 1)) break;
						}
						if(i == entier) recherche = 1;
						c = debut = svt;
						if(svt == modeles) i = -1;
					}
				}
				*(short *)adrs = (short)entier;
				if(col) *col = k - 1;
			} else {
				l = 0;
				c = modeles; debut = c; entier = 0;
				recherche = !item->limites;
				while(*c) {
					c++;
					if((*c == sep) || (*c == '\0')) {
						// l = (k < 256)? k: c - debut;
						l = (int)(c - debut);
						if(item->limites) {
							if((entier < item->lim.i.min) || (entier > item->lim.i.max)) recherche = 0;
							else recherche = 1;
						}
						if(recherche) {
							if(etape == PNL_READ) {
								strncpy(cle,debut,l); cle[l] = '\0'; traduit = cle; //- DicoTraduit(&OpiumDico,&traduit);
							} else traduit = debut;
							if(DEBUG_PANEL(5)) printf("(%s) Compare %s avec cle[%d]='%s'\n",__func__,ptrlec,entier,traduit);
						}
						if(k <= l) {
							n = (k > 0)? k: l;
							if(recherche && (!strncmp(ptrlec,traduit,n))) { l = -1; break; }
						}
						debut = c + 1; entier++;
					}
				}
				if(l < 0) *(short *)adrs = (short)entier; else if(col && *col) *col -= 1;
			}
		}
		if(item->nbfonds) {
			item->numfond = (short)*(short *)adrs;
			if(item->numfond < 0) item->numfond = 0;
			else if(item->numfond >= item->nbfonds) item->numfond = item->nbfonds - 1;
		}
		if(ecrit) {
			entier = *(short *)adrs + 1;
			c = modeles; debut = c;
			while(*c) {
				c++;
				if((*c == sep) || (*c == '\0')) {
					if(!(--entier)) break;
					debut = c + 1;
				}
			}
			l = (int)(c - debut);
			if(l > 0) { strncpy(ptrecr,debut,l); ptrecr[l] = '\0'; }
			else if(l == 0) ptrecr[0] = '\0';
			else { *(short *)adrs = 0; strcpy(ptrecr,OpiumListeInvl); }
		}
		break;
	  case PNL_KEYB:
		modeles = (char *)item->fctn; sep = '/';
		if(etape == PNL_VERIFY) adrs = (void *)&(item->tempo);
		if(lit) {
			k = curs; if(!k) k = (int)strlen(ptrlec);
			if(ptrlec[k - 1] == '?') {
				entier = m = *(char *)adrs;
				c = debut = modeles; i = -1; recherche = 0;
				while(1) {
					c++;
					if((*c == sep) || (*c == '\0')) {
						if(*c == '\0') {
							if(!recherche) { entier = 0; break; }
							svt = modeles;
						} else svt = c + 1;
						i++;
						if(recherche) {
							entier = i;
							if(entier == m) break;
							if(item->limites) {
								if((entier < item->lim.i.min) || (entier > item->lim.i.max)) continue;
							}
							if(k < 2) break;
							if(!strncmp(debut,ptrlec,k - 1)) break;
						}
						if(i == entier) recherche = 1;
						c = debut = svt;
						if(svt == modeles) i = -1;
					}
				}
				*(char *)adrs = (char)entier;
				if(col) *col = k - 1;
			} else {
				l = 0;
				c = modeles; debut = c; entier = 0;
				recherche = !item->limites;
				while(*c) {
					c++;
					if((*c == sep) || (*c == '\0')) {
						// l = (k < 256)? k: c - debut;
						l = (int)(c - debut);
						if(item->limites) {
							if((entier < item->lim.i.min) || (entier > item->lim.i.max)) recherche = 0;
							else recherche = 1;
						}
						if(recherche) {
							if(etape == PNL_READ) {
								strncpy(cle,debut,l); cle[l] = '\0'; traduit = cle; //- DicoTraduit(&OpiumDico,&traduit);
							} else traduit = debut;
							if(DEBUG_PANEL(5)) printf("(%s) Compare '%s' avec cle[%d]='%s'\n",__func__,ptrlec,entier,traduit);
						}
						if(k <= l) {
							n = (k > 0)? k: l;
							if(recherche && (!strncmp(ptrlec,traduit,n))) { l = -1; break; }
						}
						debut = c + 1; entier++;
					}
				}
				if(DEBUG_PANEL(5)) printf("(%s) Retient cle[%d]->%d\n",__func__,l,(l<0)? entier: *(char *)adrs);
				if(l < 0) *(char *)adrs = (char)entier; else if(col && *col) *col -= 1;
				if(DEBUG_PANEL(5)) printf("(%s) @%08lX=%d\n",__func__,(Ulong)adrs,*(char *)adrs);
			}
		}
		if(item->nbfonds) {
			item->numfond = (short)*(char *)adrs;
			if(item->numfond < 0) item->numfond = 0;
			else if(item->numfond >= item->nbfonds) item->numfond = item->nbfonds - 1;
		}
		if(ecrit) {
			entier = *(char *)adrs + 1;
			c = modeles; debut = c;
			while(*c) {
				c++;
				if((*c == sep) || (*c == '\0')) {
					if(!(--entier)) break;
					debut = c + 1;
				}
			}
			l = (int)(c - debut);
			if(l > 0) { strncpy(ptrecr,debut,l); ptrecr[l] = '\0'; }
			else if(l == 0) ptrecr[0] = '\0';
			else { *(char *)adrs = 0; strcpy(ptrecr,OpiumListeInvl); }
		}
		break;
	  case PNL_USER:
		fctn_user = (TypeArgUserFctn)(item->fctn);
		if(lit) fctn_user(ptrlec,adrs,col);
		if(ecrit) {
			if(!(m = item->maxuser)) strcpy(ptrecr,fctn_user(0,adrs,col));
			else {
				strncpy(ptrecr,fctn_user(0,adrs,col),m);
				ptrecr[m - 1] = '\0';
			}
		}
		break;
	  case PNL_BOUTON:
		fctn_butn = (TypePanelFctnButn)(item->fctn);
		if(lit) fctn_butn(adrs);
		else if(ecrit) strcpy(ptrecr,fctn_butn(0));
		break;
	}
	if(ecrit && ptrecr) {
		if((l = ((int)strlen(ptrecr) + 1)) > PNL_MAXVAL) {
			if(item->ptrdim < l) {
				if(item->ptrdim) free(item->ptrval);
				item->ptrval = (char *)malloc(l);
				if(!(item->ptrval)) {
					item->ptrdim = 0;
					item->mod = 0;
					strcpy(valeur,"non affichable");
/*					strcpy(valeur,"<NotPrintable>"); */
					ptrecr = valeur; item->ptrval = item->valeur;
				} else item->ptrdim = (short)l;
			}
		}
		if(item->ptrval) { strcpy(item->ptrval,ptrecr); item->ecrit = 1; }
	}
	if((etape == PNL_VERIFY) && item->modif) {
		if(DEBUG_PANEL(5)) WndPrint("(%s) appel de ItemModif pour '%s' = '%s',\n",__func__,item->texte,texte);
		modifie = (*(item->modif))((void *)adrs,ptrlec,item->modif_arg);
		if(DEBUG_PANEL(5)) WndPrint("(%s) qui retourne %d.\n",__func__,modifie);
	}
	return(modifie);
}
/* ========================================================================== */
static short PanelItemCouleur(Panel panel, PanelItem item) {
	switch(item->etat) {
		case PNL_PROTEGE: return(WND_GC_TEXT);     /* blanc */
		case PNL_MODIFIABLE: return(panel->mods);  /* vert  */
		case PNL_MODIFIE: return(panel->hilite);   /* jaune */
		default: return(WND_GC_TEXT);
	}
}
/* ========================================================================== */
static int PanelLigneSurEcran(Panel panel, int lig) {
	if(((panel->cdr)->ligmin < panel->entete_verrouillee) || (lig < panel->entete_verrouillee)) return(lig);
	else return(lig - (panel->cdr)->ligmin + panel->entete_verrouillee);
}
/* ========================================================================== */
static void PanelPrintVar(WndFrame f, Panel panel, PanelItem item, char selectr, int limp, int cimp, int max) {
	int m,n,type; char doit_terminer;
	char *affiche,champ[256]; /* correspond a un PanelVarLngrMaxi max de 255 */

	if(!f) return;
//	printf("(%s) panel P%08lX, item I%08lX\n",__func__,(Ulong)panel,(Ulong)item);
//	printf("(%s) item '%s = %s' en (%d, %d) @F=%08llX\n",__func__,item->texte,item->ptrval,limp,cimp,(IntAdrs)f);
	if(item->type < 0) return;
	if(selectr) {
		if(item->adrs) { /* WndContextBgndRGB(f,gc,r,g,b); */ }
		return;
	}
	if(item->ptrval == 0) {
		printf("(%s) %s/%s: pas d'espace d'affichage\n",__func__,(panel->cdr)->titre,item->texte);
		item->ptrval = item->valeur;
	}
//	printf("(%s) ptrval @%08lX\n",__func__,(Ulong)(item->ptrval));
	affiche = item->ptrval + item->first;
	n = (int)strlen(affiche);
//	printf("(%s) affichage de %d caractere%s\n",__func__,Accord1s(n));
	if(item->type == PNL_PSWD) {
		if(n > max) n = max;
		for(m=0; m<n; m++) champ[m] = '*'; champ[m] = '\0';
		affiche = champ;
	} else if(n > max) {
		// printf("(%s) affiche initial='%s', max=%d\n",__func__,affiche,max);
		strncpy(champ,affiche,max);
		champ[max] = '\0';
		affiche = champ;
		// printf("(%s) affiche final  ='%s', max=%d\n",__func__,affiche,max);
	}
//	printf("(%s) gc @%08lX\n",__func__,(Ulong)(item->gc));
	doit_terminer = WndRefreshBegin(f);
	if(item->gc) {
		if((item->nbfonds == 1) && (item->numfond = -1)) item->numfond = 0;
		if((item->numfond >= 0) && (item->numfond < item->nbfonds)) {
			if(item->gc[item->numfond]) {
				WndClearText(f,item->gc[item->numfond],limp,cimp,max); /* pas forcement item->maxdisp */
				WndWrite(f,item->gc[item->numfond],limp,cimp,affiche);
				if(item->souligne < 0) WndSouligne(f,item->gc[item->numfond],limp,cimp,max); /* forcement pas item->maxdisp */
				else if(item->souligne > 0) WndSouligne(f,PNL_TRACE,limp,cimp,max);
				if(doit_terminer) WndRefreshEnd(f);
				return;
			}
		}
	}
	type = PanelItemCouleur(panel,item);
//	printf("(%s) ClearText(%d)\n",__func__,type);
	if((type < 0) || (type >= f->nb_gc)) type = WND_GC_TEXT;
	WndClearText(f,WND_GC(f,type),limp,cimp,max);
//	printf("(%s) Write(%d)\n",__func__,type);
	if(item->sel) WndWrite(f,WND_GC(f,type),limp,cimp,affiche); /* pas !(panel->mode & PNL_RDONLY) car panel pas defini ici... */
	else WndWrite(f,WND_TEXT,limp,cimp,affiche);
//	printf("(%s) Souligne(%d)\n",__func__,type);
	if(item->souligne) WndSouligne(f,PNL_TRACE,limp,cimp,max); /* pas forcement item->maxdisp */
	// if(item->souligne) PRINT_FGC(f,panel->trace);
	if(doit_terminer) WndRefreshEnd(f);
}
/* ========================================================================== */
static void PanelEntryVerif(Cadre cdr, Panel panel, WndFrame f, PanelItem item, char direct,
	char *valeur, int limp, int cimp, int *car) {
	int poub; char modifie;

	modifie = PanelCode(item,PNL_VERIFY,valeur,car);
	if(modifie) {
		/* seulement si la valeur a change (entree valide notamment) */
		if(!direct || item->exit) item->etat = PNL_MODIFIE;
		PanelPrintVar(f,panel,item,0,limp,cimp,item->maxdisp);
	}
	if(direct) { poub = 0; PanelCode(item,PNL_READ,0,&poub); cdr->var_modifiees = 1; }
}
/* ========================================================================== */
static void PanelCursorSet(Panel panel, WndFrame f, int lig, int col) {
	short limp;

	panel->xcurseur = (short)col;
	panel->ycurseur = (short)lig;
	limp = (short)PanelLigneSurEcran(panel,lig);
	if(f) WndCursorSet(f,limp,(short)(col-(panel->cdr)->colmin));
}
/* ========================================================================== */
void OpiumPanelColorNb(Cadre cdr, Cadre fait, short *nc) {
	Panel panel,modele; short ic;

	if(!(panel = (Panel)cdr->adrs)) return;
	if(fait) modele = (Panel)fait->adrs; else modele = 0;
	if(modele) {
		panel->trace = modele->trace;
		panel->mods = modele->mods;
		panel->hilite = modele->hilite;
	} else {
		ic = *nc;
		panel->trace = ic++; panel->mods = ic++; panel->hilite = ic++;
		*nc = ic;
	}
}
/* ========================================================================== */
void OpiumPanelColorSet(Cadre cdr, WndFrame f) {
	Panel panel;

	if(!(panel = (Panel)cdr->adrs)) return;
	WndFenColorSet(f,panel->trace,WND_GC_DARK);
	WndFenColorSet(f,panel->mods,WND_GC_MODS);
	WndFenColorSet(f,panel->hilite,WND_GC_HILITE);
//	printf("(%s) %08lX/%2d, trace  gc@%08lX <- couleur #%d\n",__func__,(Ulong)f,panel->trace,(Ulong)f->gc[WND_Q_ECRAN].coul[panel->trace],WND_GC_DARK);
//	printf("(%s) %08lX/%2d, mods   gc@%08lX <- couleur #%d\n",__func__,(Ulong)f,panel->mods,(Ulong)f->gc[WND_Q_ECRAN].coul[panel->mods],WND_GC_MODS);
//	printf("(%s) %08lX/%2d, hilite gc@%08lX <- couleur #%d\n",__func__,(Ulong)f,panel->hilite,(Ulong)f->gc[WND_Q_ECRAN].coul[panel->hilite],WND_GC_HILITE);
}
/* ========================================================================== */
int OpiumSizePanel(Cadre cdr, char from_wm) {
	PanelItem item; Panel panel;
	int tete_colonne,numitem,lig,col,l,sep;
	int larg_txt,larg_var,larg_tot,larg_rem,larg_item;
	int larg_avant,larg_finale,suppl,reste;
	int coltexte;
#ifdef OPIUM_AJUSTE_WND
	int larg_col;
#endif
	char suivi = 0;

	panel = (Panel)cdr->adrs;
	if(DEBUG_OPIUM(1) && (cdr->type != OPIUM_PANEL)) {
		WndPrint("+++ OpiumSizePanel sur le %s @%08lX\n",cdr->nom,(Ulong)cdr->adrs);
		return(0);
	}
	if(DEBUG_OPIUM(3)) WndPrint("OpiumSizePanel (panel @%08lX)\n",(Ulong)panel);

	larg_txt = 0; item = 0 ; /* pour gcc */
	if(from_wm) {
#ifdef OPIUM_AJUSTE_WND
		larg_col = WndPixToCol(cdr->dh);
		cdr->dh = WndColToPix(larg_col);
		lig = WndPixToLig(cdr->dv);
		cdr->dv = WndLigToPix(lig);
#endif
		larg_finale = WndPixToCol(cdr->dh);
		larg_tot = 0;
		col = panel->nbcols - 1; larg_tot = panel->col[col] + panel->var[col];
		reste = larg_finale - larg_tot;
		if(DEBUG_PANEL(4)) WndPrint("=== OpiumSizePanel ===\nAgrandissement de %d a %d\n",larg_tot,larg_finale);
		if(reste) {
			coltexte = panel->col[0] + panel->var[0];
			for(tete_colonne=0,col=0; tete_colonne<panel->cur; tete_colonne += panel->nbligs,col++) {
				if(col < (panel->nbcols - 1)) {
					larg_txt = panel->col[col + 1] - coltexte;
					coltexte = panel->col[col + 1] + panel->var[col + 1];
				}
				suppl = reste / (panel->nbcols - col);
//				if(DEBUG_PANEL(4)) WndPrint("Item #%d, col #%d/%d: supplement de %d sur une largeur actuelle=%d\n",
//					tete_colonne,col,panel->nbcols,suppl,larg_txt);
				if(suppl) {
					/* la suite permet d'elargir plus que le supplement ci-dessus
					larg_prevue = panel->var[col] + suppl;
					larg_var = 1;
					for(lig=0; lig<panel->nbligs; lig++) {
						if((numitem = (tete_colonne + lig)) >= panel->cur) break;
						item = panel->items + numitem;
						if(!OpiumPanelResizable[item->type]) {
							if(larg_var < item->maxdisp) larg_var = item->maxdisp;
							if(larg_var >= larg_prevue) break;
						}
					}
					if(larg_var > larg_prevue) {
						suppl = larg_var - panel->var[col];
					} */
					larg_avant = panel->var[col];
					panel->var[col] += suppl;
					if(DEBUG_PANEL(4)) WndPrint("var[%d] passe de %d a %d\n",col,larg_avant,panel->var[col]);
					for(lig=0; lig<panel->nbligs; lig++) {
						if((numitem = (tete_colonne + lig)) >= panel->cur) break;
						item = panel->items + numitem;
						if(item->type < 0) continue;
						if(OpiumPanelResizable[(int)item->type]) {
							if((item->maxdisp == larg_avant) || (item->maxdisp > panel->var[col]))
								item->maxdisp = panel->var[col];
						}
					}
					reste -= suppl;
				}
				if(col < (panel->nbcols - 1)) {
					if(DEBUG_PANEL(4)) WndPrint("Col[%d] avant: %d",col+1,panel->col[col+1]);
					panel->col[col + 1] = panel->col[col] + panel->var[col] + (short)larg_txt;
					if(DEBUG_PANEL(4)) WndPrint(", apres: %d\n",panel->col[col+1]);
				}
			}
			col = panel->nbcols - 1; larg_tot = panel->col[col] + panel->var[col];
			if(DEBUG_PANEL(4)) WndPrint("Largeur finale: %d\n",larg_tot);
			cdr->dh = cdr->larg = WndColToPix(larg_tot);
		}
		if(DEBUG_PANEL(1)) WndPrint("(%s) Panel>%08lX/%s [%d x %d] (%d colonnes x %d lignes) -> [%d x %d]\n",__func__,
			(Ulong)cdr,cdr->nom,cdr->larg,cdr->haut,WndPixToCol(cdr->larg),WndPixToLig(cdr->haut),cdr->dh,cdr->dv);
		return(1);
	}

	larg_tot = 0;
	if(DEBUG_PANEL(2)) printf("(%s) tete_colonne: [%d,%d[ par pas de %d\n",__func__,0,panel->cur,panel->nbligs);
	for(tete_colonne=0,col=0; tete_colonne<panel->cur; tete_colonne += panel->nbligs,col++) {
		if(DEBUG_PANEL(2)) printf("(%s) . tete_colonne=%d,col=%d, ligne: [%d,%d[\n",__func__,tete_colonne,col,0,panel->nbligs);
		larg_txt = larg_var = larg_rem = 0;
		for(lig=0; lig<panel->nbligs; lig++) {
			if((numitem = (tete_colonne + lig)) >= panel->cur) break;
			item = panel->items + numitem;
			if(item->type < 0) continue;
			if(panel->mode & PNL_RDONLY) item->mod = 0; else item->mod = item->sel;
			// if(item->adrs == ArgAdrsSuivie) { printf("(%s.debut) suivi: %s = '%s'\n",__func__,ArgAdrsNom,ArgAdrsSuivie); suivi = 1; }
			if(item->traduit) l = (int)strlen(item->traduit); else l = 0;
			if((item->type == PNL_SEPARE) || (item->type == PNL_REMARK)) {
				if(larg_rem < l) larg_rem = l;
			} else {
				sep = panel->support? 1: ((item->barreMI)? 0: (l? 2: 1)); l += sep;
				if(larg_txt < l) larg_txt = l;
				if(panel->mode & PNL_SELECTR) { if(larg_var < 2) larg_var = 2; }
				else if(larg_var < item->maxdisp) larg_var = item->maxdisp;
			}
			if(DEBUG_PANEL(2)) printf("(%s) . item[%d] (en %d,%d): dim = %d + %d\n",__func__,numitem,lig,col,l,item->maxdisp);
		}
		larg_item = larg_txt + larg_var;
		if(larg_item < larg_rem) {
			larg_txt += (larg_rem - larg_item) / 2;
			larg_var = larg_rem - larg_txt;
		}
		// if(col && !(item->barreGA)) larg_txt += 1; /* pour separer les colonnes */
		if(panel->support || ((!col || (item->barreGA <= 0)) && (item->barreGA >= 0))) larg_txt += 1; /* pour separer les colonnes */
		panel->col[col] = (short)(larg_tot + larg_txt);
		panel->var[col] = (short)larg_var;
		larg_tot += larg_txt + larg_var;
		if(DEBUG_PANEL(2)) WndPrint("Colonne %d: col=%d, var=%d (total: %d)\n",col,panel->col[col],panel->var[col],larg_tot);
	}
	if(DEBUG_PANEL(2)) printf("(%s) panel %08lX/%-12s: %d lignes de donnees\n",__func__,(Ulong)cdr,cdr->nom,panel->nbligs);
	cdr->larg = WndColToPix(larg_tot);
	if((!cdr->displayed || (cdr->modexec != OPIUM_DISPLAY)) && OpiumUseMouse && !(panel->mode & PNL_DIRECT))
		cdr->haut = WndLigToPix(panel->nbligs + 1);
	else cdr->haut = WndLigToPix(panel->nbligs);
	if(DEBUG_PANEL(2)) printf("(%s) panel %08lX/%-12s [%d x %d] (%d colonnes x %d lignes)\n",__func__,
		(Ulong)cdr,cdr->nom,cdr->larg,cdr->haut,WndPixToCol(cdr->larg),WndPixToLig(cdr->haut));
	cdr->dh = cdr->larg; cdr->dv = cdr->haut;
	if(DEBUG_OPIUM(2)) WndPrint("(%s) Dims de %s %s definitives: [%d x %d]\n",__func__,OPIUMCDRTYPE(cdr->type),cdr->nom,cdr->dh,cdr->dv);
	if(suivi) printf("(%s.fin) suivi: %s = '%s'\n",__func__,ArgAdrsNom,(char *)ArgAdrsSuivie);
	return(1);
}
/* ========================================================================== */
void PanelItemUpdate(Panel panel, int num, int maj) {
	PanelItem item; Cadre cdr;
	WndFrame f;
	int numitem,lig,col,zone,limp;

	if(!panel) return;
	if((num < 1) || (num > panel->max)) return;
	cdr = panel->cdr;
	if(!OpiumCoordFenetre(cdr,&f)) return;
	if(!cdr->displayed) return;
	numitem = num  - 1;
	item = panel->items + numitem;
	if(item->type < 0) return;
	if(!item->adrs) return;
	if(maj) PanelCode(item,PNL_WRITE,0,0);

	col = numitem / panel->nbligs;
	lig = numitem - (col * panel->nbligs);
	zone = panel->col[col] - cdr->colmin;
	limp = PanelLigneSurEcran(panel,lig);
	PanelPrintVar(f,panel,item,0,limp,zone,panel->var[col]);
}
/* ========================================================================== */
void PanelUpdate(Panel panel, int maj) {
	PanelItem item; Cadre cdr; WndFrame f;
	int tete_colonne,fin,numitem,lig,col,limp;
	int lignb,nbaff,zone; char doit_terminer;

	if(!panel) return;
	cdr = panel->cdr;
	if(!OpiumCoordFenetre(cdr,&f)) return;
	if(!cdr->displayed) return;
	if(DEBUG_OPIUM(1)) {
		char nom[32]; NomAppelant(nom,32)
		WndPrint("(%s:%s) %s '%s' C=%08lX F=%08lX > F=%08lX\n",nom,__func__,
			OPIUMCDRTYPE(cdr->type),cdr->nom,(Ulong)cdr,(Ulong)cdr->f,(Ulong)f);
	}

	lignb  = WndPixToLig(cdr->dv);
	nbaff = (cdr->ligmin >= panel->entete_verrouillee)? lignb - panel->entete_verrouillee: lignb;
	if((cdr->modexec != OPIUM_DISPLAY) && OpiumUseMouse && !(panel->mode & PNL_DIRECT)) nbaff--;
	fin = cdr->ligmin + ((nbaff > panel->nbligs)? panel->nbligs: nbaff);

	// printf("(%s) tete_colonne: [%d,%d[ par pas de %d\n",__func__,0,panel->cur,panel->nbligs);
	doit_terminer = WndRefreshBegin(f);
	for(tete_colonne=0,col=0; tete_colonne<panel->cur; tete_colonne += panel->nbligs,col++) {
		// printf("(%s) . tete_colonne=%d,col=%d, ligne: [%d,%d[\n",__func__,tete_colonne,col,cdr->ligmin,fin);
		for(lig=cdr->ligmin; lig<fin; lig++) {
			if((numitem = tete_colonne + lig) >= panel->cur) break;
			item = panel->items + numitem;
			if(item->type < 0) continue;
			if(!item->adrs) continue;
			// printf("(%s)   . ligne=%d soit numitem=tete_colonne+lig=%d\n",__func__,lig,numitem);
			if(PanelItemEstModifie(panel,numitem)) continue;
			// printf("(%s)   . Encodage de la valeur #%d\n",__func__,numitem);
			if(!(panel->mode & PNL_SELECTR)) { if(maj) PanelCode(item,PNL_WRITE,0,0); }
			// printf("(%s)   . Valeur #%d: @%08lX\n",__func__,numitem,(Ulong)(item->ptrval));
			zone = panel->col[col] - cdr->colmin;
			limp = PanelLigneSurEcran(panel,lig);
			// printf("(%s)   . Ecriture de '%s'+%d (maxi %d) en colonne #%d/%d\n",__func__,item->ptrval,item->first,panel->var[col],col,panel->nbcols);
			PanelPrintVar(f,panel,item,0,limp,zone,panel->var[col]);
			// printf("(%s)   . Valeur #%d ecrite\n",__func__,numitem);
		}
	}
	if(doit_terminer) WndRefreshEnd(f);
}
/* ========================================================================== */
void PanelRefreshVars(Panel panel) { PanelUpdate(panel,1); }
/* ========================================================================== */
void PanelCursorOnVar(Panel panel, int item, int curs) {
	if(!panel) return;
	if((item < 1) || (item > panel->max)) return;
	panel->icurseur = (short)item;
	panel->ccurseur = (short)curs;
}
/* ========================================================================== */
void PanelCursorMove(Panel panel, int item, int curs) {
	WndFrame f;
	int x,y,col;

	if(!panel) return;
	if((item < 1) || (item > panel->max)) return;
	if(!OpiumCoordFenetre(panel->cdr,&f)) return;
	item -= 1;
	col = item / panel->nbligs;
	y = PanelLigneSurEcran(panel,item - (col * panel->nbligs));
	if(curs < 0) curs = 0; else if(curs >= panel->var[col]) curs = panel->var[col] - 1;
	x = panel->col[col] + curs;
	PanelCursorSet(panel,f,y,x);
}
/* ========================================================================== */
int OpiumRefreshPanel(Cadre cdr) {
	PanelItem item; Panel panel;
	WndFrame f; WndContextPtr gc;
	int tete_colonne,zone,coltext,fin,numitem,limp,cimp,sep,lig,col,deb_aff,fin_aff,l,m;
	int x,h,hp,nbboutons,premier; int debut,nbaff,dim;
	int xc,yc,lc,hc; char doit_terminer;
	char comment[256];

	if(DEBUG_OPIUM(1) && (cdr->type != OPIUM_PANEL)) {
		WndPrint("+++ OpiumDisplayPanel sur le %s @%08lX\n",cdr->nom,(Ulong)cdr->adrs);
		return(0);
	}
	if(DEBUG_PANEL(1)) WndPrint("(%s) Cadre '%s' C=%08lX\n",__func__,cdr?cdr->nom:"<nul>",(Ulong)cdr);
	if(!OpiumCoordFenetre(cdr,&f)) return(0);
	if(DEBUG_PANEL(1)) WndPrint("(%s) Fenetre F=%08llX, decalage = (%d, %d)\n",__func__,(IntAdrs)f,cdr->colmin,cdr->ligmin);
	panel = (Panel)cdr->adrs;
/*  if(panel->not_to_refresh) return(0); */

	if(DEBUG_PANEL(1)) printf("(%s) Panel de %d item%s sur %d colonne%s x %d ligne%s/%d\n",__func__,
		Accord1s(panel->cur),Accord1s(panel->nbcols),Accord1s(panel->nbligs),panel->maxligs);
	dim = (cdr->lignb > panel->nbligs)? panel->nbligs: cdr->lignb;
	nbaff = (cdr->ligmin >= panel->entete_verrouillee)? cdr->lignb - panel->entete_verrouillee: cdr->lignb;
	fin_aff = 0; /* pour gcc */
	// if(!(panel->mode & PNL_DIRECT)) nbaff--;
	if((cdr->modexec != OPIUM_DISPLAY) && OpiumUseMouse && !(panel->mode & PNL_DIRECT)) nbaff--;
	fin = cdr->ligmin + ((nbaff > panel->nbligs)? panel->nbligs: nbaff);
	if(DEBUG_PANEL(3)) printf("(%s) Affichage %d lignes / %d a partir de la %d (soit fin= %d)\n",__func__,nbaff,panel->nbligs,cdr->ligmin,fin);
	doit_terminer = WndRefreshBegin(f);
	x = 0;
	for(tete_colonne=0,col=0; tete_colonne<panel->cur; tete_colonne += panel->nbligs,col++) {
		if(DEBUG_PANEL(1)) WndPrint("(%s) == Debut colonne %d sur item #%d\n",__func__,col,tete_colonne);
		h = panel->col[col] + panel->var[col] - x;
		coltext = x - cdr->colmin;
		if(DEBUG_PANEL(4)) WndPrint("(%s)    x=%d, d'ou coltext[%d]=%d, col[%d]=%d, var[%d]=%d, soit h=%d\n",__func__,
			x,col,coltext,col,panel->col[col],col,panel->var[col],h);
		zone = panel->col[col] - cdr->colmin;
		// if(panel->support) {
		//	xc = WndColToPix(zone); yc = 0;
		//	lc = WndColToPix(panel->var[col]);
		//	hc = WndLigToPix(dim) + 1; /* +1 a cause du curseur */
		//	WndEntoure(f,panel->support,xc,yc,lc,hc);
		// }
		deb_aff = -1;
		if(panel->entete_verrouillee) debut = 0; else debut = cdr->ligmin;
		for(lig=debut; lig<fin; lig++) {
			if((lig >= panel->entete_verrouillee) && (lig < cdr->ligmin)) lig = cdr->ligmin;
			if((numitem = tete_colonne + lig) >= panel->cur) break;
			item = panel->items + numitem;
			if(DEBUG_PANEL(1)) WndPrint("(%s)    Ligne #%d: item #%d %s\n",__func__,lig,numitem,(item->type < 0)? "absent": "ajoute");
			if(item->type < 0) continue;
			if(DEBUG_PANEL(1)) WndPrint("(%s)    . texte: '%s'\n",__func__,item->traduit?item->traduit:"anonyme");
			if(item->traduit) l = (int)strlen(item->traduit); else l = 0;
			limp = PanelLigneSurEcran(panel,lig);
			cimp = coltext;
			if(DEBUG_PANEL(1)) { if(!tete_colonne) printf("(%s)    Affichage ligne %d en %d\n",__func__,lig,limp); }
			if(item->coultexte) {
				if(!item->gctexte) item->gctexte = WndContextCreate(f);
				gc = item->gctexte;
				WndContextFgndRGB(f,gc,GRF_RGB_WHITE);
				WndContextBgndRGB(f,gc,item->coultexte->r,item->coultexte->g,item->coultexte->b);
				/* if(cimp) --cimp; ne marche pas? */
			} else gc = WND_TEXT;
			hp = panel->col[col] + panel->var[col] - cimp - cdr->colmin;
			if(!cdr->planche) WndClearText(f,gc,limp,cimp,hp /*coltext,h*/);
			sep = panel->support? 1: ((item->barreMI)? 0: 2);
			if(item->type == PNL_REMARK) {
				if(l) {
					if(!item->format[0]) {
						m = (h - l) / 2;
						if(DEBUG_PANEL(4)) WndPrint("(%s) '%s' place en %d+(%d-%d)/2, soit au final %d\n",__func__,
							item->traduit,coltext,h,l,coltext+m);
						WndWrite(f,gc,limp,coltext+m,item->traduit);
					} else {
						sprintf(comment,item->format,item->traduit);
						l = (int)strlen(comment);
						WndWrite(f,gc,limp,zone-sep-l,comment);
					}
				}
			} else if(item->type == PNL_SEPARE) {
				if(l) {
					if(!item->format[0]) {
						m = (h - l) / 2;
						if(m > 1) WndHBar(f,gc,limp,coltext,m - 1);
						WndWrite(f,gc,limp,coltext+m,item->traduit);
						if(m > 1) WndHBar(f,gc,limp,coltext+m+l+1,m - 1);
					} else {
						sprintf(comment,item->format,item->traduit);
						l = (int)strlen(comment);
						WndWrite(f,gc,limp,zone-sep-l,comment);
					}
				} else WndHBar(f,gc,limp,coltext,h);
			} else if(l) {
				if(panel->support) {
					WndWrite(f,WND_CLEAR,limp,zone-sep-l,item->traduit);
					if(DEBUG_PANEL(1)) printf("(%s)    ecrit '%s' en F=%08llX + (%d, %d)\n",__func__,item->traduit,(IntAdrs)f,WndColToPix(limp),WndLigToPix(zone-sep-l));
				} else {
					if(DEBUG_PANEL(4)) printf("(%s)    '%s' place en %d-%d-%d, soit au final %d\n",__func__,
						item->traduit,zone,sep,l,zone-sep-l);
					WndWrite(f,gc,limp,zone-sep-l,item->traduit);
					if(sep == 2) WndWrite(f,gc,limp,zone-sep,": ");
				}
			} else {
				if(sep == 2) WndWrite(f,gc,limp,zone-sep,"  ");
			}
			if(!panel->support) {
				if(item->souligne > 0) WndSouligne(f,PNL_TRACE,limp,cimp,hp);
				if((col && (item->barreGA > 0)) || (item->barreGA < 0)) WndBarreGauche(f,PNL_TRACE,limp,cimp,hp);
			}
			if(DEBUG_PANEL(2)) WndPrint("(%s)    * Texte ecrit: '%s'\n",__func__,(item->traduit)?item->traduit:"anonyme");
			if(/* !cdr->displayed && */ item->adrs) {
				if(!(panel->not_to_refresh)) PanelCode(item,PNL_WRITE,0,0);
				if(item->type == PNL_FILE) {
					l = (int)strlen(item->ptrval);
					if(l > panel->var[col]) item->first = (short)l - panel->var[col] + 1;
				}
				if(item->nbfonds && item->gc) {
					int i;
					for(i=0; i<item->nbfonds; i++) {
						if((item->fond[i].r == 0) && (item->fond[i].g == 0) && (item->fond[i].b == 0)) {
							if(item->gc[i]) { free(item->gc[i]); item->gc[i] = 0; }
						} else {
							if(!item->gc[i]) item->gc[i] = WndContextCreate(f);
							WndContextFgndRGB(f,item->gc[i],GRF_RGB_BLACK);
							WndContextBgndRGB(f,item->gc[i],item->fond[i].r,item->fond[i].g,item->fond[i].b);
						}
					}
				}
				PanelPrintVar(f,panel,item,0,limp,zone,panel->var[col]);
				if(DEBUG_PANEL(1)) printf("(%s)    ecrit '%s' en F=%08llX + (%d, %d)\n",__func__,item->ptrval + item->first,(IntAdrs)f,WndColToPix(limp),WndLigToPix(zone));
				if(DEBUG_PANEL(2)) printf("(%s)    * Variable ecrite: '%s'\n",__func__,item->ptrval);
				if(deb_aff < 0) deb_aff = limp; fin_aff = limp;
			}
			if(!panel->support) {
				if(item->barreDR > 0) WndBarreDroite(f,PNL_TRACE,limp,zone,panel->var[col]);
			//	if(item->barreMI > 0) WndBarreGauche(f,PNL_TRACE,limp,zone,panel->var[col]); /* apres la separation */
				if(item->barreMI > 0) WndBarreDroite(f,PNL_TRACE,limp,cimp,hp-panel->var[col]); /* avant la separation */
			}
			if(DEBUG_PANEL(1)) WndPrint("(%s)    => Item #%d ecrit\n",__func__,numitem);
		}
		if(panel->support) {
			xc = WndColToPix(zone); yc = WndLigToPix(deb_aff);
			lc = WndColToPix(panel->var[col]);
			hc = WndLigToPix(fin_aff + 1 - deb_aff) + 1; /* +1 a cause du curseur */
			WndEntoure(f,panel->support,xc,yc,lc,hc);
		}
		x += h;
	}
	if(DEBUG_PANEL(3)) WndPrint("(%s) Panel termine\n",__func__);

	if((cdr->modexec == OPIUM_DISPLAY) || !OpiumUseMouse) { if(doit_terminer) WndRefreshEnd(f); return(1); }
	if(!(panel->mode & PNL_DIRECT)) {
		if(panel->mode & PNL_RDONLY) { nbboutons = 1; premier = PNL_OK; }
		else if(panel->mode & PNL_INBOARD) { nbboutons = 2; premier = PNL_RESET; }
		else { nbboutons = 4; premier = PNL_RESET; }
	#ifdef WXWIDGETS
//		if(!(cdr->planche)) WndClearString(f,gc,0,WndLigToPix(WndPixToLig(cdr->dv) - 1),cdr->dh);
	#endif
		OpiumBoutonsTjrs(cdr,f,premier,nbboutons,panel->bouton);
	}
	if(cdr->planche) {
		if(DEBUG_PANEL(3)) WndPrint("(%s) Cadre actif dans la planche: C=%08llX, cadre du panel: C=%08llX\n",__func__,
			(IntAdrs)((cdr->planche)->actif),(IntAdrs)cdr);
		if((cdr->planche)->actif != cdr) { if(doit_terminer) WndRefreshEnd(f); return(1); }
	}

	if(DEBUG_PANEL(3)) WndPrint("(%s) Curseur trouve en %d x %d\n",__func__,panel->ycurseur,panel->xcurseur);
	numitem = panel->icurseur - 1;
	if((numitem >= 0) && (numitem < panel->max)) {
		col = numitem / panel->nbligs;
		lig = numitem - (panel->nbligs * col);
		PanelCursorSet(panel,f,lig,panel->col[col]+panel->ccurseur);
	} else if(panel->xcurseur < 0) {
		lig = 0; col = -1;
		do {
			col++;
			numitem = (panel->nbligs * col) + lig;
			if((numitem >= panel->max) || (panel->items[numitem].type < 0)) {
				col = 0; lig++;
				numitem = (panel->nbligs * col) + lig;
				if((numitem >= panel->max) || (panel->items[numitem].type < 0)) break;
			}
			item = panel->items + numitem;
		} while(!(item->mod));
		if((numitem < panel->max) && (panel->items[numitem].type >= 0)) {
			PanelCursorSet(panel,f,lig,panel->col[col]);
		}
	} else PanelCursorSet(panel,f,panel->ycurseur,panel->xcurseur);
	if(DEBUG_PANEL(3)) WndPrint("(OpiumRefreshPanel) Curseur mis    en %d x %d\n",panel->ycurseur,panel->xcurseur);
	if(doit_terminer) WndRefreshEnd(f);
	return(1);
}
/* ========================================================================== */
void PanelApply(Panel panel, int maj) {
	PanelItem item;
	int i,curs;

	if(DEBUG_PANEL(3)) WndPrint("(PanelApply) Lecture du panel (%d valeurs)\n",panel->cur);
	curs = 0; /* avant, il y avait 999 (pourquoi?) */
	;
	for(i=0,item = panel->items; i<panel->max; i++,item++) if(item->type >= 0) {
		// printf("(%s) Recupere la valeur de l'item[%d]: '%s'\n",__func__,i,item->ptrval);
		if(item->type != PNL_BOUTON) PanelCode(item,PNL_READ,0,&curs);
	}
	(panel->cdr)->var_modifiees = 1;
	PanelUpdate(panel,maj); /* juste pour recolorer tout en vert si maj=0 */
}
/* ========================================================================== */
static void PanelDeselecte(WndFrame f) {
	//+	printf("(%s) suppression selection depuis 0x%08lX\n",__func__,(Ulong)f);
	if(OpiumEditSurligne[0] && (OpiumEditFen != 0)) {
		WndClearText(OpiumEditFen,WND_GC(OpiumEditFen,OpiumEditType),OpiumEditLig,OpiumEditCol,(int)strlen(OpiumEditSurligne));
		WndWrite(OpiumEditFen,WND_GC(OpiumEditFen,OpiumEditType),OpiumEditLig,OpiumEditCol,OpiumEditSurligne);
		/* printf("(%s) texte '%s' reaffiche dans 0x%08lX(%d,%d) en mode %d\n",__func__,OpiumEditSurligne,
		   (Ulong)OpiumEditFen,OpiumEditLig,OpiumEditCol,OpiumEditType); */
		OpiumUserAction();
	}
	OpiumEditSurligne[0] = '\0';
	OpiumEditCar0 = -1;
	OpiumEditFen = 0;
}
/* ========================================================================== */
int OpiumRunPanel(Cadre cdr, WndAction *e) {
	PanelItem item; Panel panel;
	Menu menu_cles; int nb_cles,num_cle;
	WndFrame f;
	char **liste; char *liste_cles,*c,*cle;
	char sep; char souris_glissante,ok,direct;
	int lig,col,zone,limp;
	int code_rendu;
	char valeur[256];
	int nbligs,num,nb,curs,car,i,j,k,l,m,n; int num_item;
	char traite,eol,sorti,pomme;
	TypePanelFctnItem fctn_item; TypePanelFctnArg fctn_arg;

	if(DEBUG_OPIUM(1) && (cdr->type != OPIUM_PANEL)) {
		WndPrint("+++ %s: Recu C=%08llX (%s %s), type incorrect\n",__func__,(IntAdrs)cdr->adrs,OPIUMCDRTYPE(cdr->type),cdr->nom);
		return(0);
	}
	OpiumEditDernierEvt = e;
	if(cdr->modexec == OPIUM_DISPLAY) return(0);
	panel = (Panel)cdr->adrs;
	if(DEBUG_PANEL(1)) WndPrint("(%s) Acces au panel '%s' %d x %d, C=%08llX\n",__func__,cdr->nom,cdr->dh,cdr->dv,(IntAdrs)cdr->adrs);
	if(!OpiumCoordFenetre(cdr,&f)) {
		WndPrint("(%s) Panel '%s': pas de fenetre associee\n",__func__,cdr->nom);
		return(0);
	}
	souris_glissante = 0;
// #ifdef GLISSE
	souris_glissante = ((e->type == WND_PRESS) /* && (WndEventPrecedent.type == WND_PRESS) */ && (e->w == WndEventPrecedent.w));
	pomme = (e->type == WND_KEY) && ((int)strlen(e->texte) == 1) && (e->code == XK_Alt_L);
	if(!OpiumGlissante && !pomme) PanelDeselecte(f);
	OpiumGlissante = souris_glissante;
// #endif
	if(e->type == WND_KEY) {
		lig = f->lig - f->lig0;
		col = f->col - f->col0;
		if(DEBUG_PANEL(1)) WndPrint("(%s) Touche en (%d, %d), soit lig=%d, col=%d\n",__func__,e->x,e->y,lig,col);
	} else {
		lig = WndPixToLig(e->y);
		col = WndPixToCol(e->x);
		if(DEBUG_PANEL(1)) WndPrint("(%s) Evt %s en (%d, %d), soit lig=%d, col=%d\n",__func__,WND_ACTIONNAME(e->type),e->x,e->y,lig,col);
	}
	if(cdr->ligmin > panel->entete_verrouillee) lig += (cdr->ligmin - panel->entete_verrouillee);
	if(DEBUG_PANEL(1)) printf("(%s) ligmin=%d, entete=%d soit lig=%d\n",__func__,cdr->ligmin,panel->entete_verrouillee,lig);
	limp = PanelLigneSurEcran(panel,lig);
	// printf("cdr->ligmin = %d, soit lig = %d\n",cdr->ligmin,lig);
	col += cdr->colmin;
	nbligs = cdr->ligmin + WndPixToLig(cdr->dv) - 1; // panel->nbligs;
	if(DEBUG_PANEL(1)) printf("(%s) nbligs = (ligmin=%d) + (WndPixToLig(cdr->dv)=%d) - 1 = %d\n",__func__,cdr->ligmin,WndPixToLig(cdr->dv),nbligs);
	if(cdr->ligmin >= panel->entete_verrouillee) nbligs -= panel->entete_verrouillee;
	direct = (panel->mode & PNL_DIRECT);
	if(direct) nbligs++;
	if(DEBUG_PANEL(2)) WndPrint("(%s) Panel %s: lig=%d/%d, col=%d+%d: evt %s\n",__func__,cdr->nom,lig,nbligs,col,cdr->colmin,WND_ACTIONNAME(e->type));
	num = 0;
	code_rendu = PNL_NOMODIF;
	if(lig == nbligs) {
		if(e->type == WND_RELEASE) {
		/* recuperer la bonne valeur associee au bouton! */
			if(panel->mode & PNL_RDONLY) code_rendu = PNL_OK;
			else if(panel->mode & PNL_INBOARD) {
				m = WndPixToCol(cdr->larg / 2);
				code_rendu = ((col + cdr->colmin) / m) + 1;
			} else {
				m = WndPixToCol(cdr->larg / 4);
				code_rendu = ((col + cdr->colmin) / m) + 1;
			}
			if(DEBUG_PANEL(1)) WndPrint("(%s) Bouton #%d\n",__func__,code_rendu); // OpiumPanelReply[code_rendu]);
		}
	} else {
		if(panel->mode & PNL_RDONLY) return(PNL_NOMODIF);
		zone = 0; curs = 0; /* pour gcc */
		for(k=0; k<panel->nbcols; k++) {
			zone = panel->col[k];
			curs = col - zone;
			if((curs >= 0) && (curs <= panel->var[k])) break;
		}
		if(DEBUG_PANEL(1)) WndPrint("avant: lig=%d, col=%d, k=%d, curs=%d\n",lig,col,k,curs);
		if(k >= panel->nbcols) return(PNL_NOMODIF);
		num = (panel->maxligs * k) + lig;
		if(DEBUG_PANEL(1)) WndPrint("item courant #%d\n",num);
		if((num < 0) || (num >= panel->max)) return(PNL_NOMODIF);
		num_item = num;
		item = panel->items + num;
		if((item->type < 0) || !(item->mod)) return(PNL_NOMODIF);
		if(curs > (int)strlen(item->ptrval)) curs = (int)strlen(item->ptrval);
		col = zone + curs;
		if(e->type == WND_KEY) {
			l = (int)strlen(e->texte);
			if(DEBUG_PANEL(1)) {
				WndPrint("cle %04lX + { ",e->code);
				for(m=0; m<l; m++) WndPrint("%02X ",e->texte[m]); WndPrint("}\n");
			}
			sorti = 0;
			if(l <= 0) {
				if(e->code == XK_Up) {
					if((panel->precedent == num) && (fctn_item = (TypePanelFctnItem)item->exit)) {
						(*fctn_item)(panel,num+1,item->exit_arg);
						OpiumCoordFenetre(cdr,&f); /* fctn_item pourrait changer f dans wnd.c */
						if(direct) {
							item->etat = PNL_MODIFIABLE;
							PanelPrintVar(f,panel,item,0,limp,zone-cdr->colmin,panel->var[k]);
						}
						panel->precedent = -1;
					}
					while(lig-- > 0) {
						num = (panel->maxligs * k) + lig;
						item = panel->items + num;
						if(item->mod) break;
					}
					if(lig < 0) sorti = 1;
					if((cdr->ligmin < panel->entete_verrouillee) || (lig < panel->entete_verrouillee)) limp = lig;
					else limp = lig - cdr->ligmin + panel->entete_verrouillee;
				} else if(e->code == XK_Down) {
					if((panel->precedent == num) && (fctn_item = (TypePanelFctnItem)item->exit)) {
						(*fctn_item)(panel,num+1,item->exit_arg);
						OpiumCoordFenetre(cdr,&f); /* fctn_item pourrait changer f dans wnd.c */
						if(direct) {
							item->etat = PNL_MODIFIABLE;
							PanelPrintVar(f,panel,item,0,limp,zone-cdr->colmin,panel->var[k]);
						}
						panel->precedent = -1;
					}
					while(++lig < nbligs) {
						num = (panel->maxligs * k) + lig;
						if(num >= panel->cur) { num = 0; sorti = 1; break; }
						item = panel->items + num;
						if(item->mod) break;
					}
					if((cdr->ligmin < panel->entete_verrouillee) || (lig < panel->entete_verrouillee)) limp = lig;
					else limp = lig - cdr->ligmin + panel->entete_verrouillee;
				} else if(e->code == XK_Left) {
					if(curs) --curs;
					else if(item->first) {
						item->first -= 1;
						PanelPrintVar(f,panel,item,0,limp,zone-cdr->colmin,panel->var[k]);
					}
				} else if(e->code == XK_Right) {
					if(DEBUG_PANEL(1)) WndPrint("curs=%d/%d,\n",curs,panel->var[k]);
					if(curs < panel->var[k]) curs++;
					else {
						n = (int)strlen(item->ptrval + item->first);
						if(n >= panel->var[k]) {
							item->first += 1;
							PanelPrintVar(f,panel,item,0,limp,zone-cdr->colmin,panel->var[k]);
						}
						if(DEBUG_PANEL(1)) WndPrint("first=%d/%d\n",item->first,n);
					}
					if(DEBUG_PANEL(1)) WndPrint("=> curs=%d, first=%d\n",curs,item->first);
				} else if(e->code == XK_Home) {
					k = lig = 0; curs = 0;
					if((cdr->ligmin < panel->entete_verrouillee) || (lig < panel->entete_verrouillee)) limp = lig;
					else limp = lig - cdr->ligmin + panel->entete_verrouillee;
				} else if(e->code == XK_KP_F4) sorti = 1;
			} else {
				traite = 0; eol = 0;
				if(l == 1) {
					if(e->code == XK_Alt_L) switch(e->texte[0]) {
					  case 0x63: /* Pomme-C */
						traite = 1;
						if(OpiumEditSurligne[0]) strcpy(OpiumEditReserve,OpiumEditSurligne);
						// printf("(%s) Clipboard: '%s'\n",__func__,OpiumEditReserve);
						break;
					  case 0x76: /* Pomme-V */
						traite = 1;
						if(!OpiumEditReserve[0]) break;
						l = (int)strlen(OpiumEditReserve);
						car = curs + item->first;
						strncpy(valeur,item->ptrval,car); valeur[car] = '\0';
						strcat(valeur,OpiumEditReserve);
						if(panel->overstrike) {
							n = (int)strlen(item->ptrval);
							if((car + l) < n) strcat(valeur,item->ptrval+car+l);
						} else strcat(valeur,item->ptrval+car);
						if(DEBUG_PANEL(1)) WndPrint("valeur obtenue: \"%s\"\n",valeur);
						car += l;
						PanelEntryVerif(cdr,panel,f,item,direct,valeur,limp,zone-cdr->colmin,&car);
						panel->precedent = (short)num;
						curs = car - item->first;
						if(curs >= panel->var[k]) {
							n = (int)strlen(valeur + item->first);
							if(n > panel->var[k]) item->first += (n - panel->var[k]);
							curs = panel->var[k] - 1;
						}
						break;
					  case 0x78: /* Pomme-X */
						traite = 1;
						if(!OpiumEditSurligne[0]) break;
						strcpy(OpiumEditReserve,OpiumEditSurligne);
						l = (int)strlen(OpiumEditSurligne);
						car = OpiumEditCurs + OpiumEditItem->first;
						if(car <= 0) break;
						n = (int)strlen(OpiumEditItem->ptrval);
					#ifdef OVERSTRIKE_ON_DELETE
						if(panel->overstrike) {
							if(car < n) {
								strcpy(valeur,OpiumEditItem->ptrval);
								if(curs) valeur[car - 1] = ' ';
							} else if(car) {
								if(car > 1) strncpy(valeur,OpiumEditItem->ptrval,car-1);
								valeur[car-1] = '\0';
							} else valeur[0] = '\0';
						} else {
					#endif
							if(car) strncpy(valeur,OpiumEditItem->ptrval,car);
							valeur[car] = '\0';
							if(n > car + l) {
								strcat(valeur,OpiumEditItem->ptrval + car + l);
							}
							valeur[n - l] = '\0';
					#ifdef OVERSTRIKE_ON_DELETE
						}
					#endif
						if(DEBUG_PANEL(1)) WndPrint("valeur obtenue: \"%s\"\n",valeur);
						if(OpiumEditCurs) --OpiumEditCurs; else if(OpiumEditItem->first) OpiumEditItem->first -= 1;
						if(car) --car;
						PanelEntryVerif(cdr,panel,f,OpiumEditItem,direct,valeur,limp,zone-cdr->colmin,&car);
						panel->precedent = (short)num;
						break;
					} else switch(e->texte[0]) {
					  case 0x01: /* Ctrl-A: toggle insert/overstrike */
						panel->overstrike = !(panel->overstrike); traite = 1; break;
					  case 0x05: /* Ctrl-E: go to end of line */
						n = (int)strlen(item->ptrval + item->first);
						if(n < panel->var[k]) curs = n;
						else {
							item->first += (n - panel->var[k]);
							curs = panel->var[k];
							PanelPrintVar(f,panel,item,0,limp,zone-cdr->colmin,panel->var[k]);
						}
						traite = 1;
						break;
					  case 0x09: /* Ctrl-I (TAB): go to next field */
						if((panel->precedent == num) && (fctn_item = (TypePanelFctnItem)item->exit)) {
							(*fctn_item)(panel,num+1,item->exit_arg);
							OpiumCoordFenetre(cdr,&f); /* fctn_item pourrait changer f dans wnd.c */
							if(direct) {
								item->etat = PNL_MODIFIABLE;
								PanelPrintVar(f,panel,item,0,limp,zone-cdr->colmin,panel->var[k]);
							}
							panel->precedent = -1;
						}
						do {
							k++;
							num = (panel->maxligs * k) + lig;
							if((k >= panel->nbcols) || (num >= panel->max)) {
								k = 0; lig++;
								num = (panel->maxligs * k) + lig;
								if((cdr->ligmin < panel->entete_verrouillee) || (lig < panel->entete_verrouillee)) limp = lig;
								else limp = lig - cdr->ligmin + panel->entete_verrouillee;
							}
							if(num < panel->max) {
								item = panel->items + num;
								if(item->mod) break;
							} else {
								k = -1; lig = 0;
								if((cdr->ligmin < panel->entete_verrouillee) || (lig < panel->entete_verrouillee)) limp = lig;
								else limp = lig - cdr->ligmin + panel->entete_verrouillee;
							}
						} while(lig < nbligs);
						curs = 0;
						traite = 1;
						break;
					  case 0x0D: eol = 1; curs = 0; traite = 1; break;
					  case 0x08: case 0x7F: /* Ctrl-H (BackSpace) ou DEL */
						car = curs + item->first;
						traite = 1;
						if(car <= 0) break;
						n = (int)strlen(item->ptrval);
					#ifdef OVERSTRIKE_ON_DELETE
						if(panel->overstrike) {
							if(car < n) {
								strcpy(valeur,item->ptrval);
								if(curs) valeur[car - 1] = ' ';
							} else if(car) {
								if(car > 1) strncpy(valeur,item->ptrval,car-1);
								valeur[car-1] = '\0';
							} else valeur[0] = '\0';
						} else {
					#endif
							if(car > 1) strncpy(valeur,item->ptrval,car - 1);
							if(n > car) {
								valeur[car - 1] = '\0';
								strcat(valeur,item->ptrval + car);
							}
							valeur[--n] = '\0';
					#ifdef OVERSTRIKE_ON_DELETE
						}
					#endif
						if(DEBUG_PANEL(1)) WndPrint("valeur obtenue: \"%s\"\n",valeur);
						if(curs) --curs; else if(item->first) item->first -= 1;
						if(car) --car;
						PanelEntryVerif(cdr,panel,f,item,direct,valeur,limp,zone-cdr->colmin,&car);
						panel->precedent = (short)num;
						break;
					  case 0x0B: /* Ctrl-K: clear to end of line */
						car = curs + item->first;
						if(car) strncpy(valeur,item->ptrval,car);
						valeur[car] = '\0';
						PanelEntryVerif(cdr,panel,f,item,direct,valeur,limp,zone-cdr->colmin,&car);
						panel->precedent = (short)num;
						traite = 1;
						break;
					}
				}
				if(!traite) {
					eol = (e->texte[l - 1] == 0x0D);
					if(eol) { --l; e->texte[l] = '\0'; }
					if(DEBUG_PANEL(1)) WndPrint("item: %s, entree: '%s' en %s\n",item->ptrval,e->texte,
						panel->overstrike?"surimpression": "ajout");
					car = curs + item->first;
					strncpy(valeur,item->ptrval,car); valeur[car] = '\0';
					strcat(valeur,e->texte);
					if(panel->overstrike) {
						n = (int)strlen(item->ptrval);
						if((car + l) < n) strcat(valeur,item->ptrval+car+l);
					} else strcat(valeur,item->ptrval+car);
					if(DEBUG_PANEL(1)) WndPrint("valeur obtenue: \"%s\"\n",valeur);
					car += l;
					PanelEntryVerif(cdr,panel,f,item,direct,valeur,limp,zone-cdr->colmin,&car);
					panel->precedent = (short)num;
					curs = car - item->first;
					if(curs >= panel->var[k]) {
						n = (int)strlen(valeur + item->first);
						if(n > panel->var[k]) item->first += (n - panel->var[k]);
						curs = panel->var[k] - 1;
					}
				}
				if(eol) {
/*					if(direct) { poub = 999; PanelCode(item,PNL_READ,0,&poub); }
					... oui mais si on change d'item par la souris, on ne passe pas par la... */
					int avant;
					if((fctn_item = (TypePanelFctnItem)item->rtn)) {
						if(DEBUG_PANEL(2)) {
							WndPrint("(%s) rtn sur C%08lX='%s' -> P%08lX[%d->%d] -> I%08lX\n",__func__,
								(Ulong)cdr,cdr->nom,(Ulong)panel,num_item+1,num+1,(Ulong)item);
							WndPrint("(%s) item '%s'\n",__func__,item->texte);
						}
						/* fctn_item pourrait changer f (ainsi que panel et item si WndClear(f)!!) dans wnd.c
						   Doit alors retourner 1 (au lieu de 0) si objet refait, d'ou tous pointeurs obsoletes */
						if((*fctn_item)(panel,num+1,item->rtn_arg)) return(PNL_NBREPLIES);
						eol = 0;
						OpiumCoordFenetre(cdr,&f);
						if(direct) {
							item->etat = PNL_MODIFIABLE;
							PanelPrintVar(f,panel,item,0,limp,zone-cdr->colmin,panel->var[k]);
						}
					}
					if((panel->precedent == num) && (fctn_item = (TypePanelFctnItem)item->exit)) {\
						/* Meme remarque que pour rtn */
						if((*fctn_item)(panel,num+1,item->exit_arg)) return(PNL_NBREPLIES);
						OpiumCoordFenetre(cdr,&f);
						if(direct) {
							item->etat = PNL_MODIFIABLE;
							PanelPrintVar(f,panel,item,0,limp,zone-cdr->colmin,panel->var[k]);
						}
						panel->precedent = -1;
					}
					avant = lig;
					/* k = 0; while(++lig < nbligs) {
						for(k=0,num=lig; k<panel->nbcols; k++,num+=panel->maxligs) {
							if(num < panel->cur) { item = panel->items + num; if(item->mod) break; } else k = panel->nbcols;
						}
						if(k < panel->nbcols) break;
					} */
					while(++lig < nbligs) {
						num = (panel->maxligs * k) + lig;
						while(num >= panel->cur) {
							k = 0; while(k < panel->nbcols) {
								if(num >= panel->cur) break;
								num = (panel->maxligs * k) + lig;
								item = panel->items + num; if(item->mod) break;
								k++;
							}
						}
						if(num < panel->cur) { item = panel->items + num; if(item->mod) break; }
					}
					if(!eol && (lig >= nbligs)) lig = avant; /* pas de OK implicite si fctn appelee sur <Return> */
					if((cdr->ligmin < panel->entete_verrouillee) || (lig < panel->entete_verrouillee)) limp = lig;
					else limp = lig - cdr->ligmin + panel->entete_verrouillee;
				}
			}
			if(lig >= nbligs) sorti = 1;
			if(sorti) {
				if(cdr->planche) {
					Cadre csvt;
					if(direct) csvt = 0;
					else {
						Panel psvt;
						csvt = cdr->suivant;
						while(csvt) {
							if(csvt->type == OPIUM_PANEL) {
								(cdr->planche)->actif = csvt;
								OpiumCoordFenetre(csvt,&f);
								psvt = (Panel)csvt->adrs;
								PanelCursorSet(psvt,f,0,psvt->col[0] + 2);
								break;
							}
							csvt = csvt->suivant;
						}
						if(!csvt) {
							csvt = (Cadre)((cdr->planche)->adrs);
							while(csvt) {
								if(csvt == cdr) { csvt = 0; break; }
								if(csvt->type == OPIUM_PANEL) {
									(cdr->planche)->actif = csvt;
									OpiumCoordFenetre(csvt,&f);
									psvt = (Panel)csvt->adrs;
									PanelCursorSet(psvt,f,0,psvt->col[0] + 2);
									break;
								}
								csvt = csvt->suivant;
							}
						}
					}
					if(!csvt) PanelCursorSet(panel,f,0,panel->col[0] + 2);
					panel->precedent = -1;
					code_rendu = PNL_APPLY;
				} else {
					PanelCursorSet(panel,f,-1,-1);
					code_rendu = PNL_OK;
				}
				num = -1;
			} else {
				zone = panel->col[k];
				num = (panel->maxligs * k) + lig;
				item = panel->items + num;
				if(item->adrs) {
					if(curs > (n = (int)strlen(item->ptrval + item->first))) curs = n;
				} else curs = 0;
				if(curs > panel->var[k]) curs = panel->var[k];
				PanelCursorSet(panel,f,lig,zone+curs);
			}
			if(DEBUG_PANEL(1)) WndPrint("apres: lig=%d, col=%d, k=%d, curs=%d\n",lig,f->col,k,curs);

		} else if(e->type == WND_PRESS) switch(e->code) {
		  case WND_MSELEFT:
			if(souris_glissante) {
				car = curs + item->first;
				if(WndEventPrecedent.type != WND_PRESS) {
					OpiumEditItem = item; OpiumEditCurs = curs;
					OpiumEditFen = f; OpiumEditCar0 = car; OpiumEditType = PanelItemCouleur(panel,item);
					OpiumEditLig = limp;
				} else if(limp == OpiumEditLig) {
					if((nb = car - OpiumEditCar0) < OPIUM_MAX_RESERVE) {
						if(nb > 0) {
							strncpy(OpiumEditSurligne,item->ptrval+OpiumEditCar0,nb);
							OpiumEditCol = zone - cdr->colmin + OpiumEditCar0;
							OpiumEditSurligne[nb] = '\0';
						} else if((nb < 0) && ((OpiumEditCar0 + nb) >= 0)) {
							strncpy(OpiumEditSurligne,item->ptrval+OpiumEditCar0 + nb,-nb);
							OpiumEditCol = zone - cdr->colmin + OpiumEditCar0 + nb;
							OpiumEditCurs = OpiumEditCar0 + nb - item->first;
							nb = -nb;
							OpiumEditSurligne[nb] = '\0';
						}
					}
					if(nb > 0) {
						PanelPrintVar(f,panel,item,0,limp,zone-cdr->colmin,panel->var[k]);
						if(OpiumEditFen != 0) {
							WndClearText(OpiumEditFen,WND_GC(OpiumEditFen,WND_GC_SELECTED),OpiumEditLig,OpiumEditCol,nb);
							WndWrite(OpiumEditFen,WND_GC(OpiumEditFen,WND_GC_SELECTED),OpiumEditLig,OpiumEditCol,OpiumEditSurligne);
							/* printf("(%s) texte '%s' surligne dans 0x%08lX(%d,%d) en mode %d\n",__func__,OpiumEditSurligne,
							   (Ulong)OpiumEditFen,OpiumEditLig,OpiumEditCol,OpiumEditType); */
						}
					}
				}
			}
			zone = panel->col[k];
			num = (panel->maxligs * k) + lig;
			if((panel->precedent >= 0) && (panel->precedent < panel->cur)) {
				item = panel->items + panel->precedent;
				if((num != panel->precedent) && (fctn_item = (TypePanelFctnItem)item->exit)) {
					(*fctn_item)(panel,panel->precedent+1,item->exit_arg);
					OpiumCoordFenetre(cdr,&f); /* fctn_item pourrait changer f dans wnd.c */
					if(direct) {
						item->etat = PNL_MODIFIABLE;
						PanelPrintVar(f,panel,item,0,limp,zone-cdr->colmin,panel->var[k]);
					}
					panel->precedent = -1;
				}
			}
			item = panel->items + num;
			if(item->adrs) {
				if(curs > (n = (int)strlen(item->ptrval + item->first))) curs = n;
			} else curs = 0;
			if(item->adrs) {
				ok = 0; liste_cles = 0; menu_cles = 0;
				liste = 0; nb_cles = 0;  /* pour gcc */
				if((item->type == PNL_BOOL)
				|| ((item->type >= PNL_LISTE) && (item->type <= PNL_KEYB))) {
//					if(DEBUG_PANEL(3)) WndPrint("WND_PRESS sur l'item #%d de type %d (%s)n",num,item->type,OpiumPanelType[(int)item->type]);
					menu_cles = MenuTitled(item->traduit,0);
					if(item->type == PNL_BOOL) {
						if(MenuItemArray(menu_cles,2)) {
							MenuItemAdd(menu_cles,OpiumNon,MNU_CONSTANTE 1);
							MenuItemAdd(menu_cles,OpiumOui,MNU_CONSTANTE 2);
							menu_cles->defaut = (!strcmp(item->ptrval,OpiumOui))? 2: 1;
							// MenuRelocalise(menu_cles);
							ok = 1;
						}
						liste_cles = 0;

					} else if((item->type >= PNL_LISTE) && (item->type <= PNL_LISTB)) {
						liste = (char **)item->fctn;
						nb_cles = 0; while(liste[nb_cles][0]) nb_cles++;
						i = 0;
						if(MenuItemArray(menu_cles,nb_cles)) while(liste[i][0]) {
							j = i + 1;
//							if(DEBUG_PANEL(3)) WndPrint("Cle #%d: '%s'\n",i,liste[i]);
							MenuItemAdd(menu_cles,liste[i],MNU_CONSTANTE (int64)j);
							if(!strcmp(liste[i],item->ptrval)) menu_cles->defaut = j;
							ok = 1;
							i++;
						}
						liste_cles = 0;

					} else if((item->type >= PNL_KEY) && (item->type <= PNL_KEYB)) {
						l = (int)strlen((char *)item->fctn);
						liste_cles = (char *)malloc(l + 1);
						if(liste_cles) {
							strcpy(liste_cles,(char *)item->fctn); sep = '/';
							c = liste_cles;
							nb_cles = 0;
							while(*c) {
								c++;
								if((*c == sep) || (*c == '\0')) nb_cles++;
							}
							c = cle = liste_cles; i = 1;
							if(MenuItemArray(menu_cles,nb_cles)) while(1) {
								c++;
								if((*c == sep) || (*c == '\0')) {
								#ifdef A_JUSTIFIER
									MenuItemAdd(menu_cles,cle,MNU_CONSTANTE i);
									if(*c == '\0') {
										if(!strcmp(cle,item->ptrval)) menu_cles->defaut = i;
										break;
									}
									*c = '\0';
									if(!strcmp(cle,item->ptrval)) menu_cles->defaut = i;
								#else  /* A_JUSTIFIER */
									char fin;
									fin = (*c == '\0');
									*c = '\0';
									MenuItemAdd(menu_cles,cle,MNU_CONSTANTE (int64)i);
									if(!strcmp(cle,item->ptrval)) menu_cles->defaut = i;
									if(fin) break;
								#endif /* A_JUSTIFIER */
									cle = c + 1;
									ok = 1;
									i++;
								}
							}
						}
					}
					if(ok) {
						(menu_cles->cdr)->defaultx = f->x + cdr->x0 + WndColToPix(zone);
						(menu_cles->cdr)->defaulty = f->y + cdr->y0 + WndLigToPix(limp + 1);
						/* avec (lig): plus beau mais fenetre pas scotchable */
						num_cle = OpiumSub(menu_cles->cdr) - 1;
						if(num_cle >= 0) {
							if(DEBUG_PANEL(5)) WndPrint("(%s) Cle rendue: #%d\n",__func__,num_cle);
							if(item->type == PNL_BOOL)
								strncpy(valeur,num_cle? "Oui": "Non",3);
							else if((item->type >= PNL_LISTE) && (item->type <= PNL_LISTB))
								strncpy(valeur,liste[num_cle],256);
							else if((item->type >= PNL_KEY) && (item->type <= PNL_KEYB)) {
								c = liste_cles;
								if(num_cle < nb_cles) {
									i = 0; while(i < num_cle) if(*c++ == '\0') i++;
								}
								strncpy(valeur,c,256);
								if(DEBUG_PANEL(5)) WndPrint("(%s) Valeur utilisee: '%s'\n",__func__,valeur);
							}
							curs = (int)strlen(valeur);
							OpiumCoordFenetre(cdr,&f); // refait WndRepere car degrade par le clic sur le menu
	//						if(DEBUG_PANEL(3))
	//							WndPrint("Texte correspondant: '%s' (curseur en %d)\n",valeur,curs);
							PanelEntryVerif(cdr,panel,f,item,direct,valeur,limp,zone-cdr->colmin,&curs);
						}
						curs = 0;
						panel->precedent = (short)num;
						MenuItemDelete(menu_cles);
					}
					MenuDelete(menu_cles);
					if(liste_cles) free(liste_cles);
					e->type = -1; /* sinon, boucle sur affichage menu */

				} else if(item->type == PNL_FILE) {
				#ifndef WIN32
				#ifndef CW5
					Cadre planche; Menu menu_dirs,menu_files; Panel panel_nom;
					char chemin[MAXFILENAME],nom_fichier[MAXNAMLEN]; char *s;
					DIR *repert; struct dirent *entree;
					int nb_dirs=0,nb_files=0; int i_dir,i_file; int k_dir,k_file; int l_dirs=0,l_files=0;
					int maxi,nbcols,cols_dir;
					char *noms_dir,*noms_file;
					char **items_dir,**items_file;
					int pointe,hmax; char encore=0;
					struct stat infos;
					char type_dir;
					char tempo[MAXFILENAME];
					int i_entree;
					#define MAX_ENTREES 1024
					char is_dir[MAX_ENTREES];

					planche = BoardCreate();
					panel_nom = PanelCreate(2);
					PanelFile(panel_nom,"chemin",chemin,MAXFILENAME);
					PanelText(panel_nom,"fichier",nom_fichier,MAXNAMLEN);
					PanelItemSelect(panel_nom,1,0);
					PanelMode(panel_nom,PNL_DIRECT);
					PanelSupport(panel_nom,WND_CREUX);
					menu_dirs = MenuCreate(0);
					menu_files = MenuTitled("Fichiers",0);
					strncpy(chemin,item->ptrval,MAXFILENAME); chemin[MAXFILENAME - 1] = '\0';
					s = rindex(chemin,(int)SLASH);
					if(s) { strncpy(nom_fichier,s+1,MAXNAMLEN); *s = '\0'; }
					else { strncpy(nom_fichier,chemin,MAXNAMLEN); getcwd(chemin,MAXFILENAME); }
					nom_fichier[MAXNAMLEN - 1] = '\0';
					do {
						do {
							repert = opendir(chemin);
							if(repert) break;
							l = (int)strlen(chemin);
							if(chemin[l - 1] == SLASH) chemin[l - 1] = '\0';
							s = rindex(chemin,(int)SLASH);
							if(s) *s = '\0'; else break;
							PanelRefreshVars(panel_nom);
						} while(1);
						if(repert) {
							l = (int)strlen(chemin);
							if(chemin[l - 1] != SLASH) strcat(chemin,RepertoireDelim);
							nb_dirs = 1; nb_files = 0;
							l_dirs = 3; l_files = 0;
							i_entree = 0;
							while((entree = readdir(repert)) != NULL) {
								if(entree->d_name[0] == '.') {
									if(((entree->d_name[1] == '.') && (entree->d_name[2] == '\0'))
									   || (entree->d_name[1] == '\0')) continue;
								}
								strcat(strcpy(tempo,chemin),entree->d_name);
								if(stat(tempo,&infos) == -1) type_dir = 0;
								else type_dir = S_ISDIR(infos.st_mode);
								is_dir[i_entree++] = type_dir;
								if(type_dir) {
									l_dirs += ((int)strlen(entree->d_name) + 1); nb_dirs++;
//									WndPrint("repertoire #%d: '%s'\n",nb_dirs,entree->d_name);
								} else {
									l_files += ((int)strlen(entree->d_name) + 1); nb_files++;
//									WndPrint("fichier #%d: '%s'\n",nb_files,entree->d_name);
								}
								if(i_entree >= MAX_ENTREES) break;
							}
						}
						ok = 0;
						if(MenuItemArray(menu_dirs,nb_dirs)) {
							ok = MenuItemArray(menu_files,nb_files);
						}
						if(ok) {
							maxi = WndPixToLig(OpiumServerHeigth(0)) - 5;
							if(nb_dirs > maxi) {
								nbcols = ((nb_dirs - 1) / maxi) + 1;
								MenuColumns(menu_dirs,nbcols);
								cols_dir = nbcols;
							} else {
								MenuColumns(menu_dirs,0);
								cols_dir = 1;
							}
							if(nb_files > maxi) {
								nbcols = ((nb_files - 1) / maxi) + 1;
								MenuColumns(menu_files,nbcols);
							} else MenuColumns(menu_files,0);
							noms_dir = (char *)malloc(l_dirs); if(noms_dir == 0) break;
							items_dir = (char **)malloc(nb_dirs * sizeof(char *)); if(items_dir == 0) break;
							if(l_files) {
								noms_file = (char *)malloc(l_files);
								if(noms_file == 0) break;
							} else noms_file = 0;
							if(nb_files) {
								items_file = (char **)malloc(nb_files * sizeof(char *));
								if(items_file == 0) break;
							} else items_file = 0;
							i_dir = 0; i_file = 0;
							l_dirs = 0; l_files = 0;
							k_dir = 10000; k_file = 20000;

							items_dir[i_dir] = noms_dir + l_dirs;
							strcpy(items_dir[i_dir],".."); l = (int)strlen(items_dir[i_dir]);
							++k_dir; MenuItemAdd(menu_dirs,items_dir[i_dir],MNU_CONSTANTE (int64)k_dir);
//								WndPrint("l'item '%s' renverra %d\n",items_dir[i_dir],k_dir);
							l_dirs += (l + 1); ++i_dir;
							hmax = l;

							if(repert) {
								rewinddir(repert); i_entree = 0;
								while((entree = readdir(repert)) != NULL) {
									if(entree->d_name[0] == '.') {
										if(((entree->d_name[1] == '.') && (entree->d_name[2] == '\0'))
										|| (entree->d_name[1] == '\0')) continue;
									}
									type_dir = is_dir[i_entree++];
									if(type_dir) {
										items_dir[i_dir] = noms_dir + l_dirs;
										strcpy(items_dir[i_dir],entree->d_name); l = (int)strlen(items_dir[i_dir]);
										++k_dir; MenuItemAdd(menu_dirs,items_dir[i_dir],MNU_CONSTANTE (int64)k_dir);
//										WndPrint("l'item '%s' renverra %d\n",items_dir[i_dir],k_dir);
										l_dirs += (l + 1); ++i_dir;
										if(l > hmax) hmax = l;
									} else {
										items_file[i_file] = noms_file + l_files;
										strcpy(items_file[i_file],entree->d_name); l = (int)strlen(items_file[i_file]);
										++k_file; MenuItemAdd(menu_files,items_file[i_file],MNU_CONSTANTE (int64)k_file);
//										WndPrint("l'item '%s' renverra %d\n",items_file[i_file],k_file);
										l_files += (l + 1); ++i_file;
									}
								}
							}
							if(hmax < 3) MenuTitle(menu_dirs,"Rep");
							else if(hmax < 6) MenuTitle(menu_dirs,"Repert");
							else MenuTitle(menu_dirs,"Repertoires");
							BoardAddPanel(planche,panel_nom,0,0,0);
							OpiumSupport(menu_dirs->cdr,WND_RAINURES);
							BoardAddMenu(planche,menu_dirs,0,WndLigToPix(4),0);
							if(nb_files) {
								OpiumSupport(menu_files->cdr,WND_RAINURES);
								BoardAddMenu(planche,menu_files,(WndColToPix(hmax)+10)*cols_dir,WndLigToPix(4),0);
							}
							planche->defaultx = f->x + cdr->x0 + WndColToPix(zone);
							planche->defaulty = f->y + cdr->y0 + WndLigToPix(lig + 1);
							pointe = OpiumSub(planche);
//								WndPrint("on recupere pointe=%d\n",pointe);
							encore = 0;
							if(pointe > 20000) {
								i_file = pointe - 20001;
//									WndPrint("soit fichier(%d)='%s'\n",i_file,items_file[i_file]);
								strcat(strcpy(valeur,chemin),items_file[i_file]);
//									WndPrint("et valeur='%s'\n",valeur);
							} else if(pointe > 10000) {
								i_dir = pointe - 10001;
//									WndPrint("soit dir(%d)='%s'\n",i_dir,items_dir[i_dir]);
								if(i_dir == 0) {
									l = (int)strlen(chemin);
									if(chemin[l - 1] == SLASH) chemin[l - 1] = '\0';
									s = rindex(chemin,(int)SLASH);
									if(s) *s = '\0';
								} else strcat(chemin,items_dir[i_dir]);
								PanelRefreshVars(panel_nom);
								encore = 1;
							} else if(pointe) {
								strcat(strcpy(valeur,chemin),nom_fichier);
							}
							free(noms_dir); if(noms_file) free(noms_file);
							free(items_dir); if(items_file) free(items_file);
							if(!encore && pointe) {
								l = (int)strlen(valeur);
								if(l > panel->var[k]) item->first = (short)l - panel->var[k] + 1;
								curs = l - 1;
								PanelEntryVerif(cdr,panel,f,item,direct,valeur,
									limp,zone-cdr->colmin,&curs);
								panel->precedent = (short)num;
							}
						}
						if(repert) closedir(repert);
						MenuItemDelete(menu_dirs);
						MenuItemDelete(menu_files);
						BoardDismount(planche);
					} while(encore);

					PanelDelete(panel_nom);
					MenuDelete(menu_dirs);
					MenuDelete(menu_files);
					OpiumDelete(planche);
					e->type = -1; /* sinon, boucle sur affichage menu */
				#endif
				#endif
				}
			}
			if(curs > panel->var[k]) curs = panel->var[k];
			PanelCursorSet(panel,f,lig,zone+curs);
			break;

		} else if(e->type == WND_RELEASE) switch(e->code) {
		  case WND_MSELEFT:
			zone = panel->col[k];
			num = (panel->maxligs * k) + lig;
			if((panel->precedent >= 0) && (panel->precedent < panel->cur)) {
				item = panel->items + panel->precedent;
				if((num != panel->precedent) && (fctn_item = (TypePanelFctnItem)item->exit)) {
					(*fctn_item)(panel,panel->precedent+1,item->exit_arg);
					OpiumCoordFenetre(cdr,&f); /* fctn_item pourrait changer f dans wnd.c */
					if(direct) {
						item->etat = PNL_MODIFIABLE;
						PanelPrintVar(f,panel,item,0,limp,zone-cdr->colmin,panel->var[k]);
					}
					panel->precedent = -1;
				}
			}
			item = panel->items + num;
			if(item->type == PNL_BOUTON) {
				TypePanelFctnButn fctn_butn;
				if((fctn_butn = (TypePanelFctnButn)(item->fctn))) fctn_butn(item->adrs);
				curs = 0;
			} else if(item->nbfonds) {
				strncpy(valeur,item->ptrval,256);
				if(item->type == PNL_BOOL) {
					strcpy(valeur,(!strcmp(item->ptrval,"Oui"))? "Non": "Oui");
				} else if((item->type >= PNL_LISTE) && (item->type <= PNL_LISTB)) {
					liste = (char **)item->fctn; i = 0;
					while(liste[i][0]) {
						if(!strcmp(liste[i++],item->ptrval)) {
							if(!(liste[i][0])) i = 0;
							strncpy(valeur,liste[i],256);
							break;
						}
						// i++;
					}
				} else if((item->type >= PNL_KEY) && (item->type <= PNL_KEYB)) {
					l = (int)strlen((char *)item->fctn);
					liste_cles = (char *)malloc(l + 1);
					if(liste_cles) {
						char fini;
						strcpy(liste_cles,(char *)item->fctn); sep = '/';
						c = cle = liste_cles; ok = 0;
						while(1) {
							fini = (*c == '\0');
							if(*c == sep) *c = '\0';
							if(*c == '\0') {
								if(ok) break;
								if(!strcmp(cle,item->ptrval)) ok = 1;
								if(fini) { if(ok) cle = liste_cles; break; }
								cle = c + 1;
							}
							c++;
						}
						if(ok) strncpy(valeur,cle,256);
						free(liste_cles);
					}
				}
				curs = (int)strlen(valeur);
				PanelEntryVerif(cdr,panel,f,item,direct,valeur,limp,zone-cdr->colmin,&curs);
				panel->precedent = (short)num;
				curs = 0;
			} else {
				if(item->adrs) {
					if(curs > (n = (int)strlen(item->ptrval + item->first))) curs = n;
				} else curs = 0;
				if(curs > panel->var[k]) curs = panel->var[k];
			}
			PanelCursorSet(panel,f,lig,zone+curs);
			if(DEBUG_PANEL(3)) WndPrint("apres: lig=%d, col=%d, k=%d, curs=%d\n",lig,col,k,curs);
			break;
		  case WND_MSERIGHT:
			PanelCursorSet(panel,f,0,panel->col[0]+2);
			// code_rendu = PNL_OK;
			break;
		}
	}

	if(DEBUG_PANEL(1)) WndPrint("(%s) Code rendu: %d\n",__func__,code_rendu);
	if(direct) panel->not_to_refresh = 1;
	else {
		if((code_rendu == PNL_OK) || (code_rendu == PNL_APPLY)) {
			PanelApply(panel,0);
			if((code_rendu == PNL_OK) && ((fctn_arg = panel->on_ok))) (*fctn_arg)(panel, panel->ok_arg);
			else if((code_rendu == PNL_APPLY) && ((fctn_arg = panel->on_apply))) (*fctn_arg)(panel, panel->apply_arg);

		} else if(code_rendu == PNL_RESET) {
			PanelRefreshVars(panel);
			if((fctn_arg = panel->on_reset)) (*fctn_arg)(panel, panel->reset_arg);
		}
/*  	else if(code_rendu == PNL_NOMODIF) PanelUpdate(panel,0); */
		panel->not_to_refresh = (code_rendu == PNL_NOMODIF);
		if(code_rendu > PNL_APPLY) panel->icurseur = -1;
	}
	if(DEBUG_PANEL(3))
		WndPrint("(%s) Curseur laisse en %d x %d\n",__func__,panel->ycurseur,panel->xcurseur);

	return(code_rendu);
}
