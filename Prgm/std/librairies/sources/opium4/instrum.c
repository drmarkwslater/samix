#include <stdio.h>
#include <stdlib.h> /* pour malloc et free */
#include <math.h>

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

#include <opium.h>

static Instrum InstrumAdd(INS_FORME forme, void *parms);
static int InstrumUpdate(Instrum instrum, double dev);
static char InstrumCode(Instrum instrum, char etape, char *texte/*, int *col*/);
static void InstrumLegende(Instrum instrum, double g1, int nb, int *i, int *j,
	char *legende);

/* ========================================================================== */
static Instrum InstrumAdd(INS_FORME forme, void *parms) {
	Instrum instrum; Cadre cdr;

	instrum = (Instrum)malloc(sizeof(TypeInstrum));
	if(instrum) {
		cdr = OpiumCreate(OPIUM_INSTRUM,instrum);
		if(!cdr) {
			free(instrum); return(0);
		}
		instrum->cdr = cdr;
		instrum->forme = forme;
		instrum->parms = parms;
		instrum->deja_trace = 0;
		instrum->gradauto = 1;
		instrum->log = 0;
		instrum->grad = 1.0;
		strcpy(instrum->format,"%g");
		instrum->support = WND_RIEN;
		instrum->on_change = 0;
		instrum->change_arg = 0;
		instrum->dev = 0.0;
		instrum->relief = 0; instrum->cadran = 0;
		instrum->bouton = 0; instrum->interr = 0;
		instrum->gc.bgnd = 0; // WND_GC_DARK;
		instrum->gc.fgnd = 0; // WND_GC_TEXT;
		instrum->gc.interr = 0; // WND_GC_HILITE;
	}
	return(instrum);
}
/* ========================================================================== */
Instrum InstrumInt(INS_FORME forme, void *parms, int *adrs, int min, int max) {
	Instrum instrum;

	if((instrum = InstrumAdd(forme,parms))) {
		instrum->type = PNL_INT;
		instrum->adrs = (void *)adrs;
		instrum->lim.i.min = min;
		instrum->lim.i.max = max;
	}
	return(instrum);
}
/* ========================================================================== */
Instrum InstrumShort(INS_FORME forme, void *parms, short *adrs, short min, short max) {
	Instrum instrum;

	if((instrum = InstrumAdd(forme,parms))) {
		instrum->type = PNL_SHORT;
		instrum->adrs = (void *)adrs;
		instrum->lim.s.min = min;
		instrum->lim.s.max = max;
	}
	return(instrum);
}
/* ========================================================================== */
Instrum InstrumFloat(INS_FORME forme, void *parms, float *adrs, float min, float max) {
	Instrum instrum;

	if((instrum = InstrumAdd(forme,parms))) {
		instrum->type = PNL_FLOAT;
		instrum->adrs = (void *)adrs;
		instrum->lim.f.min = min;
		instrum->lim.f.max = max;
	}
	return(instrum);
}
/* ========================================================================== */
Instrum InstrumDouble(INS_FORME forme, void *parms, double *adrs, double min, double max) {
	Instrum instrum;

	if((instrum = InstrumAdd(forme,parms))) {
		instrum->type = PNL_DBLE;
		instrum->adrs = (void *)adrs;
		instrum->lim.d.min = min;
		instrum->lim.d.max = max;
	}
	return(instrum);
}
/* ========================================================================== */
Instrum InstrumList(INS_FORME forme, void *parms, int *adrs, char **liste) {
	Instrum instrum;

	if((instrum = InstrumAdd(forme,parms))) {
		instrum->type = PNL_LISTE;
		instrum->adrs = (void *)adrs;
		instrum->lim.l.liste = (void *)liste;
	}
	return(instrum);
}
/* ========================================================================== */
Instrum InstrumListL(INS_FORME forme, void *parms, int64 *adrs, char **liste) {
	Instrum instrum;

	if((instrum = InstrumAdd(forme,parms))) {
		instrum->type = PNL_LISTD;
		instrum->adrs = (void *)adrs;
		instrum->lim.l.liste = (void *)liste;
	}
	return(instrum);
}
/* ========================================================================== */
Instrum InstrumListS(INS_FORME forme, void *parms, short *adrs, char **liste) {
	Instrum instrum;

	if((instrum = InstrumAdd(forme,parms))) {
		instrum->type = PNL_LISTS;
		instrum->adrs = (void *)adrs;
		instrum->lim.l.liste = (void *)liste;
	}
	return(instrum);
}
/* ========================================================================== */
Instrum InstrumListB(INS_FORME forme, void *parms, char *adrs, char **liste) {
	Instrum instrum;

	if((instrum = InstrumAdd(forme,parms))) {
		instrum->type = PNL_LISTB;
		instrum->adrs = (void *)adrs;
		instrum->lim.l.liste = (void *)liste;
	}
	return(instrum);
}
/* ========================================================================== */
Instrum InstrumKey(INS_FORME forme, void *parms, int *adrs, char *modeles) {
	Instrum instrum;

	if((instrum = InstrumAdd(forme,parms))) {
		instrum->type = PNL_KEY;
		instrum->adrs = (void *)adrs;
		instrum->lim.l.liste = (void *)modeles; // L_(modeles);
	}
	return(instrum);
}
/* ========================================================================== */
Instrum InstrumKeyL(INS_FORME forme, void *parms, int64 *adrs, char *modeles) {
	Instrum instrum;

	if((instrum = InstrumAdd(forme,parms))) {
		instrum->type = PNL_KEYL;
		instrum->adrs = (void *)adrs;
		instrum->lim.l.liste = (void *)modeles; // L_(modeles);
	}
	return(instrum);
}
/* ========================================================================== */
Instrum InstrumKeyS(INS_FORME forme, void *parms, short *adrs, char *modeles) {
	Instrum instrum;

	if((instrum = InstrumAdd(forme,parms))) {
		instrum->type = PNL_KEYS;
		instrum->adrs = (void *)adrs;
		instrum->lim.l.liste = (void *)modeles; // L_(modeles);
	}
	return(instrum);
}
/* ========================================================================== */
Instrum InstrumKeyB(INS_FORME forme, void *parms, char *adrs, char *modeles) {
	Instrum instrum;

	if((instrum = InstrumAdd(forme,parms))) {
		instrum->type = PNL_KEYB;
		instrum->adrs = (void *)adrs;
		instrum->lim.l.liste = (void *)modeles; // L_(modeles);
	}
	return(instrum);
}
/* ========================================================================== */
Instrum InstrumBool(INS_FORME forme, void *parms, char *adrs) {
	Instrum instrum;
	
	if((instrum = InstrumAdd(forme,parms))) {
		instrum->type = PNL_BOOL;
		instrum->adrs = (void *)adrs;
		instrum->lim.l.liste = (void *)OpiumChoixBool;
	}
	return(instrum);
}
/* ========================================================================== */
void InstrumCircDim(TypeInstrumCirc *cercle, int rayon) {
	if(cercle) cercle->rayon = rayon;
}
/* ========================================================================== */
void InstrumRectDim(TypeInstrumRect *rect, int longueur, int largeur) {
	if(rect) { rect->longueur = longueur; rect->largeur = largeur; }
}
/* ========================================================================== */
void InstrumLevierDim(TypeInstrumLevier *levier, int hauteur, int largeur) {
	if(levier) { levier->hauteur = hauteur; levier->largeur = largeur; }
}
/* ========================================================================== */
void InstrumBascDim(TypeInstrumBascule *bascule, int longueur, int largeur) {
	if(bascule) { bascule->longueur = longueur; bascule->largeur = largeur; }
}
/* ========================================================================== */
void InstrumCircGrad(TypeInstrumCirc *cercle, int graduation) {
	if(cercle) cercle->graduation = (double)graduation;
}
/* ========================================================================== */
void InstrumRectGrad(TypeInstrumRect *rect, int graduation) {
	if(rect) rect->graduation = graduation;
}
/* ========================================================================== */
void InstrumRectHoriz(TypeInstrumRect *rect, char horiz) {
	if(rect) rect->horiz = horiz;
}
/* ========================================================================== */
void InstrumBascHoriz(TypeInstrumBascule *bascule, char horiz) {
	if(bascule) bascule->horiz = horiz;
}
/* ========================================================================== */
int InstrumCircRayon(TypeInstrumCirc *cercle) { return((int)(cercle->rayon)); }
/* ========================================================================== */
int InstrumRectLngr(TypeInstrumRect *rect) { return(rect->longueur); }
/* ========================================================================== */
int InstrumRectLarg(TypeInstrumRect *rect) { return(rect->largeur); }
/* ========================================================================== */
int InstrumLevierHaut(TypeInstrumLevier *levier) { return(levier->hauteur); }
/* ========================================================================== */
int InstrumBascLngr(TypeInstrumBascule *bascule) { return(bascule->longueur); }
/* ========================================================================== */
void InstrumTitle(Instrum i, char *texte) { if(i) OpiumTitle(i->cdr,texte); }
/* ========================================================================== */
void InstrumILimits(Instrum i, int min, int max) {
	if(i) switch(i->type) {
	  case PNL_INT:   i->lim.i.min =         min; i->lim.i.max =         max; break;
	  case PNL_SHORT: i->lim.s.min =  (short)min; i->lim.s.max = (short) max; break;
	  case PNL_FLOAT: i->lim.f.min =  (float)min; i->lim.f.max = (float) max; break;
	  case PNL_DBLE:  i->lim.d.min = (double)min; i->lim.d.max = (double)max; break;
	}
}
/* ========================================================================== */
void InstrumRLimits(Instrum i, float min, float max) {
	if(i) switch(i->type) {
	  case PNL_INT:   i->lim.i.min =    (int)min; i->lim.i.max =    (int)max; break;
	  case PNL_SHORT: i->lim.s.min =  (short)min; i->lim.s.max =  (short)max; break;
	  case PNL_FLOAT: i->lim.f.min =         min; i->lim.f.max =         max; break;
	  case PNL_DBLE:  i->lim.d.min = (double)min; i->lim.d.max = (double)max; break;
	}
}
/* ========================================================================== */
void InstrumGradSet(Instrum i, float grad) {
	if(i) { i->grad = (double)grad; i->gradauto = 0; }
}
/* ========================================================================== */
void InstrumGradAuto(Instrum i) { if(i) i->gradauto = 1;}
/* ========================================================================== */
/* log = -1: graduations type oscillo (1,2,5) */
void InstrumGradLog(Instrum i, char log) { if(i) i->log = log; }
/* ========================================================================== */
void InstrumSupport(Instrum i, char support) { if(i) i->support = support; }
/* ========================================================================== */
void InstrumFormat(Instrum i, char *format) {
	if(i) strncpy(i->format,format,INS_MAXFMT);
}
/* ========================================================================== */
void InstrumCouleur(Instrum i, int interr) { if(i) i->gc.interr = interr; }
/* ========================================================================== */
void InstrumOnChange(Instrum i, TypeFctn fctn, void *arg) {
	if(i) { i->on_change = fctn; i->change_arg = arg; }
}
/* ========================================================================== */
void *InstrumChangeArg(Instrum i) { return(i? i->change_arg: 0); }
/* ========================================================================== */
void static InstrumLegende(Instrum instrum, double g1, int nb, int *i, int *j,
	char *legende) {
	int col0,coln;
	char **liste; char *modeles; char *a_ecrire;

	if((instrum->type >= PNL_LISTE) && (instrum->type <= PNL_LISTB)) {
		liste = (char **)(instrum->lim.l.liste);
		a_ecrire = liste[nb]; // L_(liste[nb]);
		strcpy(legende,a_ecrire);
	} else if(((instrum->type >= PNL_KEY) && (instrum->type <= PNL_KEYB)) || (instrum->type == PNL_BOOL)) {
		modeles = (char *)(instrum->lim.l.liste);
		col0 = *i; coln = *j;
		while((modeles[coln] != '/') && (modeles[coln] != '\0')) coln++;
		strncpy(legende,modeles+col0,coln-col0); legende[coln - col0] = '\0';
		a_ecrire = legende; // L_(legende);
		if(a_ecrire != legende) strcpy(legende,a_ecrire);
		if(modeles[coln] != '\0') col0 = ++coln;
		*i = col0; *j = coln;
	} else sprintf(legende,instrum->format,g1);
}
/* ========================================================================== */
void OpiumInstrumColorNb(Cadre cdr, Cadre fait, short *nc) {
	Instrum instrum,modele; short ic;

	if(!(instrum = (Instrum)cdr->adrs)) return;
	if(fait) modele = (Instrum)fait->adrs; else modele = 0;
	if(modele) {
		instrum->relief = modele->relief; instrum->cadran = modele->cadran;
		instrum->bouton = modele->bouton; instrum->interr = modele->interr;
	} else {
		ic = *nc;
		instrum->relief = ic++; instrum->cadran = ic++;
		instrum->bouton = ic++; instrum->interr = ic++;
		*nc = ic;
	}
}
/* ========================================================================== */
void OpiumInstrumColorSet(Cadre cdr, WndFrame f) {
	Instrum instrum;

	if(!(instrum = (Instrum)cdr->adrs)) return;
	WndFenColorSet(f,instrum->relief,WND_GC_GREY);
	WndFenColorSet(f,instrum->cadran,WND_GC_MODS);
	WndFenColorSet(f,instrum->bouton,WND_GC_BUTN);
	WndFenColorSet(f,instrum->interr,WND_GC_HILITE);
//	printf("(%s) %08lX/%2d, relief gc@%08lX <- couleur #%d\n",__func__,(Ulong)f,instrum->relief,(Ulong)f->gc[WND_Q_ECRAN].coul[instrum->relief],WND_GC_GREY);
//	printf("(%s) %08lX/%2d, cadran gc@%08lX <- couleur #%d\n",__func__,(Ulong)f,instrum->cadran,(Ulong)f->gc[WND_Q_ECRAN].coul[instrum->cadran],WND_GC_MODS);
//	printf("(%s) %08lX/%2d, bouton gc@%08lX <- couleur #%d\n",__func__,(Ulong)f,instrum->bouton,(Ulong)f->gc[WND_Q_ECRAN].coul[instrum->bouton],WND_GC_BUTN);
//	printf("(%s) %08lX/%2d, interr gc@%08lX <- couleur #%d\n",__func__,(Ulong)f,instrum->interr,(Ulong)f->gc[WND_Q_ECRAN].coul[instrum->interr],WND_GC_HILITE);
}
/* ========================================================================== */
int OpiumSizeInstrum(Cadre cdr, char from_wm) {
	int larg,haut;
	char axe;
	double inf,sup; double x,y,c,x0,y0;
	double bx,by,gx,gy;
	int grad;
	int col0,coln; int i,l;
	char **liste; char *modeles;
	Instrum instrum;
	TypeInstrumCirc *cercle; TypeInstrumRect *rect; TypeInstrumLevier *levier;
	TypeInstrumBascule *bascule;

	instrum = (Instrum)cdr->adrs;
	if(DEBUG_OPIUM(1) && (cdr->type != OPIUM_INSTRUM)) {
		WndPrint("+++ OpiumSizeInstrum sur le %s @%08lX\n",cdr->nom,(Ulong)cdr->adrs);
		return(0);
	}

	if(from_wm) {
		larg = cdr->larg;
		c = (double)cdr->haut / (double)cdr->larg;
		cdr->larg = cdr->dh;
		cdr->dv = cdr->haut = (int)((double)cdr->larg * c);
		c = (double)cdr->larg / (double)larg;
		switch(instrum->forme) {
		  case INS_CADRAN: case INS_POTAR:
		  /*
			cdr->larg = cdr->dh;
			larg = cdr->larg - 1;
			rayon = (double)larg / instrum->geom.cercle.x; (rayon total)
			haut = (int)(rayon * instrum->geom.cercle.y) + WndLigToPix(1);
			cdr->dv = cdr->haut = haut + 1 + WndLigToPix(1);
			cercle = (TypeInstrumCirc *)instrum->parms;
			cercle->rayon = rayon - (2 * cercle->graduation);
		  */
			cercle = (TypeInstrumCirc *)instrum->parms;
			cercle->rayon = ((cercle->rayon + (2.0 * cercle->graduation)) * c) - (2.0 * cercle->graduation);
			break;
		  default:
			break;
		}
		return(1);
	}
	larg = 0;
	haut = 0;
	switch(instrum->forme) {
	  case INS_CADRAN: case INS_POTAR:
		cercle = (TypeInstrumCirc *)instrum->parms;
		if(cercle->angle_min < cercle->angle_max) {
			inf = cercle->angle_min;
			sup = cercle->angle_max;
		} else if(cercle->angle_min > cercle->angle_max) {
			inf = cercle->angle_max;
			sup = cercle->angle_min;
		} else return(0);

		if(instrum->forme == INS_CADRAN) {
			if((inf + (2.0 * PI)) == sup) axe = INS_AXE_CENTRE;
			else if((sup > PI) || (inf < -PI)) axe = INS_AXE_CENTRE;
			else if(sup > (PI / 2.0)) {
				if(inf >= (PI / 2.0)) axe = INS_AXE_HAUT_GAUCHE;
				else if(inf > 0.0) axe = INS_AXE_MILIEU_GAUCHE;
				else if((inf <= -(PI / 2.0)) && (inf > -(0.75 * PI))) axe = INS_AXE_HAUT_MILIEU;
				else axe = INS_AXE_CENTRE;
			} else if(sup > 0.0) {
				if(inf >= 0.0) axe = INS_AXE_BAS_GAUCHE;
				else if(inf >= -(PI / 2.0)) axe = INS_AXE_BAS_MILIEU;
				else axe = INS_AXE_CENTRE;
			} else if(sup > -(PI / 2.0)) {
				if(inf >= -(PI / 2.0)) axe = INS_AXE_BAS_DROIT;
				else axe = INS_AXE_MILIEU_DROIT;
			} else axe = INS_AXE_HAUT_DROIT;
		} else axe = INS_AXE_CENTRE;

		gx = gy = 0.0; /* supplement de position centre pour les graduations */
		bx = by = 0.0; /* supplement de hauteur/largeur pour le bouton */
		switch(axe) {
		  case INS_AXE_CENTRE:
			x = y = 2.0; x0 = x / 2; y0 = y / 2; gx = gy = 1.0; break;
		  case INS_AXE_BAS_GAUCHE:
			x = sin(sup); y = cos(inf); x0 = 0.0; y0 = y; gy = 1.0; bx = by = 1.0; break;
		  case INS_AXE_BAS_MILIEU:
			c = -sin(inf); x = c + sin(sup); y = 1.0;  x0 = c; y0 = y; gx = gy = 1.0; by = 1.0; break;
		  case INS_AXE_BAS_DROIT:
			x = -sin(inf); y = cos(sup); x0 = x; y0 = y; gx = gy = 1.0; bx = by = 1.0; break;
		  case INS_AXE_MILIEU_DROIT:
			x = 1.0; c = cos(sup); y = c - cos(inf); x0 = x; y0 = c; gx = gy = 1.0; bx = 1.0; break;
		  case INS_AXE_HAUT_DROIT:
			x = -sin(sup); y = -cos(inf); x0 = x; y0 = 0; gx = 1.0; bx = by = 1.0; break;
		  case INS_AXE_HAUT_MILIEU:
			c = -sin(inf); x = sin(sup) + c; y = 1.0; x0 = c; y0 = 0.0; gx = 1.0; bx = by = 1.0; break;
		  case INS_AXE_HAUT_GAUCHE:
			x = sin(inf); y = -cos(sup); x0 = y0 = 0.0; bx = by = 1.0; break;
		  case INS_AXE_MILIEU_GAUCHE:
			x = 1.0; c = cos(inf); y = c - cos(sup); x0 = 0.0; y0 = c; gy = 1.0; bx = 1.0; break;
		  default: x = y = x0 = y0 = 0; break;
		}
		if(instrum->forme == INS_CADRAN) bx = by = 0.0;
		instrum->geom.cercle.x = x;
		instrum->geom.cercle.y = y;
		instrum->geom.cercle.x0 = x0 + ((gx * cercle->graduation) / cercle->rayon);
		instrum->geom.cercle.y0 = y0 + ((gy * (cercle->graduation + WndLigToPix(1))) / cercle->rayon);
		larg = (int)((x * (cercle->rayon + (2.0 * cercle->graduation))) + (bx * cercle->rayon));
		haut = (int)((y * (cercle->rayon + (1.0 * cercle->graduation))) + (by * cercle->rayon));
			if(instrum->forme == INS_POTAR) {
				if((inf < -(3.0 * PI / 4.0)) || (sup > (0.75 * PI))) haut += (int)cercle->graduation;
			} else haut += (int)cercle->graduation;
		if(instrum->forme == INS_CADRAN) haut += WndLigToPix(1);
		break;
		
	  case INS_COLONNE:
	  case INS_GLISSIERE:
		rect = (TypeInstrumRect *)instrum->parms;
		larg = rect->largeur;
		grad = rect->graduation + (rect->horiz? WndLigToPix(1): WndColToPix(rect->nbchars));
		if(instrum->support) grad += 3;
		instrum->geom.rect.decal = 0;
		if(rect->position <= 0) {
			larg += rect->graduation + grad;
			instrum->geom.rect.decal = grad;
		}
		if(rect->position >= 0) {
			larg += rect->graduation;
			if(rect->position > 0) larg += grad;
		}
		if(rect->horiz) {
			haut = larg;
			larg = abs(rect->longueur) + rect->largeur /* + WndColToPix(rect->nbchars) */ ;
		} else haut = abs(rect->longueur) + rect->largeur /* 2 x 1/2 place du bouton */ ;
		break;

	  case INS_LEVIER:
		levier = (TypeInstrumLevier *)instrum->parms;
		instrum->geom.levier.htext = WndLigToPix(1) + 2;
		haut = levier->hauteur + (2 * instrum->geom.levier.htext);
		larg = levier->largeur;
		break;

	  case INS_BASCULE:
		bascule = (TypeInstrumBascule *)instrum->parms;
		if(bascule->horiz) {
			if((instrum->type >= PNL_LISTE) && (instrum->type <= PNL_LISTB)) {
				larg = 2;
				liste = (char **)(instrum->lim.l.liste);
				i = 0;
				while(*(liste[i])) {
					larg += (int)strlen(liste[i]);
					if(i++ > 0) break;
				}
			} else if(((instrum->type >= PNL_KEY) && (instrum->type <= PNL_KEYB)) || (instrum->type == PNL_BOOL)) {
				larg = 2; i = 0;
				modeles = (char *)(instrum->lim.l.liste);
				col0 = 0; coln = 0;
				do {
					while((modeles[coln] != '/') && (modeles[coln] != '\0')) coln++;
					larg += (coln - col0);
					if(modeles[coln] != '\0') col0 = ++coln; else break;
					if(i++ > 0) break;
				} while(1);
			} else larg = 2; /* meilleure evaluation du texte a ecrire? */
			larg = bascule->longueur + WndColToPix(larg);
			haut = bascule->largeur;
		} else {
			if((instrum->type >= PNL_LISTE) && (instrum->type <= PNL_LISTB)) {
				haut = 0;
				liste = (char **)(instrum->lim.l.liste);
				i = 0;
				while(*(liste[i])) {
					if(strlen(liste[i])) haut++;
					if(i++ > 0) break;
				}
			} else if(((instrum->type >= PNL_KEY) && (instrum->type <= PNL_KEYB)) || (instrum->type == PNL_BOOL)) {
				haut = 0; i = 0;
				modeles = (char *)(instrum->lim.l.liste);
				col0 = 0; coln = 0;
				do {
					while((modeles[coln] != '/') && (modeles[coln] != '\0')) coln++;
					if(coln - col0) haut++;
					if(modeles[coln] != '\0') col0 = ++coln; else break;
					if(i++ > 0) break;
				} while(1);
			} else haut = 0;
			haut = bascule->longueur + WndLigToPix(2 * haut);
			larg = bascule->largeur;
		}
		break;

	  case INS_POUSSOIR:
		  if((instrum->type >= PNL_LISTE) && (instrum->type <= PNL_LISTB)) {
			  larg = 0;
			  liste = (char **)(instrum->lim.l.liste);
			  i = 0;
			  while(*(liste[i])) {
				  l = (int)strlen(liste[i]);
				  if(l > larg) larg = l;
				  i++;
			  }
		  } else if(((instrum->type >= PNL_KEY) && (instrum->type <= PNL_KEYB)) || (instrum->type == PNL_BOOL)) {
			  larg = 0;
			  modeles = (char *)(instrum->lim.l.liste);
			  col0 = 0; coln = 0;
			  do {
				  while((modeles[coln] != '/') && (modeles[coln] != '\0')) coln++;
				  l = coln - col0;
				  if(l > larg) larg = l;
				  if(modeles[coln] != '\0') col0 = ++coln; else break;
			  } while(1);
		  } else larg = 2; /* meilleure evaluation du texte a ecrire? */
		  larg = WndColToPix(larg);
		  haut = WndLigToPix(2);
		  break;

	  case INS_RADIO:
	  case INS_VOYANT:
		break;
	}
	if(cdr->titre[0]) haut += WndLigToPix(1);
	cdr->dh = cdr->larg = larg + 1;
	cdr->dv = cdr->haut = haut + 1;
	return(1);
}
/* ========================================================================== */
int OpiumRefreshInstrum(Cadre cdr) {
	Instrum instrum; WndFrame f;
	TypeInstrumCirc *cercle; TypeInstrumRect *rect; TypeInstrumLevier *levier;
	TypeInstrumBascule *bascule;
	int i,j,k,nb,v,d; char doit_fermer,doit_terminer;
	double logmin,logmax;
	double dev,angle,sina,cosa,dev1;
	double grad,gradinit,g1,g1init,nbgrad; char legende[1024];
	char **liste; char *modeles;
	int x,y,l,h,a0,b0,da;
	int c,x1,y1,xmin,xmax,ymin,ymax;
	Cadre cdr_demande;

	cdr_demande = cdr;
	if(cdr->type != OPIUM_INSTRUM) return(0);
	if(!OpiumCoordFenetre(cdr,&f)) return(0);
	instrum = (Instrum)cdr->adrs;
	if(instrum->cdr != cdr_demande) {
		OpiumError("(OpiumRefreshInstrum) cadre demande: %08lX, gere: %08lX\n",cdr_demande,instrum->cdr);
		return(0);
	}

	instrum->deja_trace = 0;
	logmin = logmax = 0.0;

	switch(instrum->type) {
	  case PNL_SHORT:
		if(instrum->log) {
			if(instrum->lim.s.min > 0) {
				if(instrum->log > 0) logmin = log10((double)instrum->lim.s.min); 
				else {
					v = OpiumGradOscillo((float)instrum->lim.s.min,0,&d);
					logmin = log10((double)(v * d));
				}
			} else logmin = 0.0;
			if(instrum->lim.s.max > 0) {
				if(instrum->log > 0) logmax = log10((double)instrum->lim.s.max); 
				else {
					v = OpiumGradOscillo((float)instrum->lim.s.max,1,&d);
					logmax = log10((double)(v * d));
				}
			} else logmax = logmin + 1.0;
		} else {
			instrum->echelle = (double)(instrum->lim.s.max - instrum->lim.s.min);
//			if(instrum->echelle >= 0.0) instrum->min = (double)instrum->lim.s.min;
//			else instrum->min = (double)instrum->lim.s.max;
			instrum->min = (double)instrum->lim.s.min;
		}
		break;
	  case PNL_INT:
		if(instrum->log) {
			if(instrum->lim.i.min > 0) {
				if(instrum->log > 0) logmin = log10((double)instrum->lim.i.min); 
				else {
					v = OpiumGradOscillo((float)instrum->lim.i.min,0,&d);
					logmin = log10((double)(v * d));
				}
			} else logmin = 0.0;
			if(instrum->lim.i.max > 0) {
				if(instrum->log > 0) logmax = log10((double)instrum->lim.i.max); 
				else {
					v = OpiumGradOscillo((float)instrum->lim.i.max,1,&d);
					logmax = log10((double)(v * d));
				}
			} else logmax = logmin + 1.0;
		} else {
			instrum->echelle = (double)(instrum->lim.i.max - instrum->lim.i.min);
//			if(instrum->echelle >= 0.0) instrum->min = (double)instrum->lim.i.min;
//			else instrum->min = (double)instrum->lim.i.max;
			instrum->min = (double)instrum->lim.i.min;
		}
		break;
	  case PNL_FLOAT:
		if(instrum->log) {
			if(instrum->lim.f.min > 0.0) {
				if(instrum->log > 0) logmin = log10((double)instrum->lim.f.min); 
				else {
					v = OpiumGradOscillo(instrum->lim.f.min,0,&d);
					logmin = log10((double)(v * d));
				}
			} else logmin = 0.0;
			if(instrum->lim.f.max > 0.0) {
				if(instrum->log > 0) logmax = log10((double)instrum->lim.f.max); 
				else {
					v = OpiumGradOscillo(instrum->lim.f.max,1,&d);
					logmax = log10((double)(v * d));
				}
			} else logmax = logmin + 1.0;
		} else {
			instrum->echelle = (double)(instrum->lim.f.max - instrum->lim.f.min);
//			if(instrum->echelle >= 0.0) instrum->min = (double)instrum->lim.f.min;
//			else instrum->min = (double)instrum->lim.f.max;
			instrum->min = (double)instrum->lim.f.min;
		}
		break;
	  case PNL_DBLE:
		if(instrum->log) {
			if(instrum->lim.d.min > 0) {
				if(instrum->log > 0.0) logmin = log10((double)instrum->lim.d.min); 
				else {
					v = OpiumGradOscillo((float)instrum->lim.d.min,0,&d);
					logmin = log10((double)(v * d));
				}
			} else logmin = 0.0;
			if(instrum->lim.d.max > 0) {
				if(instrum->log > 0.0) logmax = log10((double)instrum->lim.d.max); 
				else {
					v = OpiumGradOscillo((float)instrum->lim.d.max,1,&d);
					logmax = log10((double)(v * d));
				}
			} else logmax = logmin + 1.0;
		} else {
			instrum->echelle = instrum->lim.d.max - instrum->lim.d.min;
//			if(instrum->echelle >= 0.0) instrum->min = instrum->lim.d.min;
//			else instrum->min = instrum->lim.d.max;
			instrum->min = instrum->lim.d.min;
		}
		break;
	  case PNL_LISTE: case PNL_LISTL: case PNL_LISTD: case PNL_LISTS: case PNL_LISTB:
		instrum->min = 0.0;
		liste = (char **)(instrum->lim.l.liste);
		nb = 0; while(liste[nb][0] != '\0') nb++;
		if(nb <= 0) return(0);
		nb--;
		instrum->echelle = (double)nb;
		break;
	  case PNL_KEY: case PNL_KEYL: case PNL_KEYS: case PNL_KEYB:
		instrum->min = 0.0;
		modeles = (char *)(instrum->lim.l.liste);
		nb = 0; i = 0; while(modeles[i] != '\0') { if(modeles[i] == '/') nb++; i++; }
		instrum->echelle = (double)nb;
		break;
	  case PNL_BOOL:
		instrum->min = 0.0;
		instrum->echelle = 1.0;
		break;
	}
	if(((instrum->type >= PNL_LISTE) && (instrum->type <= PNL_KEYB)) || (instrum->type == PNL_BOOL)) {
		g1 = 0.0; grad = 1.0; dev = 0.0;
	} else {
		if(instrum->log) {
			if(logmax != logmin) instrum->echelle = logmax - logmin; else instrum->echelle = 1.0;
			instrum->min = logmin;
			// grad = pow(10.0,(double)((int)logmin));
			g1 = pow(10.0,logmin); grad = g1;
			dev = (logmin - instrum->min) / instrum->echelle;
		} else {
			if(instrum->gradauto) grad = OpiumGradsLineaires(instrum->echelle);
			else grad = instrum->grad;
			// modf(instrum->min/fabs(grad),&nbgrad); g1 = nbgrad * grad;  /* premiere graduation */
			modf(instrum->min/grad,&nbgrad); g1 = nbgrad * grad;  /* premiere graduation */
			if(instrum->echelle > 0.0) { if(g1 < instrum->min) g1 += grad; }
			else { if(g1 > instrum->min) g1 += grad; }
			dev = (g1 - instrum->min) / instrum->echelle;
		}
	}
	if(DEBUG_INSTRUM(2)) {
		printf("   min: %g, echelle: %g\n",instrum->min,instrum->echelle);
		printf("   graduations: %g (premiere a %g)\n",grad,g1);
	}
	gradinit = grad; g1init = g1; dev1 = dev;
	
	instrum->gc.bgnd = instrum->bouton;
	instrum->gc.fgnd = instrum->cadran;
	instrum->gc.interr = instrum->interr;
	switch(instrum->forme) {
		case INS_CADRAN:
			instrum->gc.bgnd = WND_GC_CLEAR;
			break;
		case INS_COLONNE:
			instrum->gc.bgnd = instrum->interr;
			instrum->gc.fgnd = WND_GC_TEXT;
			break;
		case INS_POTAR:
		case INS_GLISSIERE:
		case INS_LEVIER:
		case INS_BASCULE:
		case INS_RADIO:
			instrum->gc.fgnd = WND_GC_TEXT;
			instrum->gc.interr = WND_GC_CLEAR;
			break;
		case INS_POUSSOIR:
			instrum->gc.fgnd = WND_GC_CLEAR;
			break;
		default:
			break;
	}
	doit_terminer = WndRefreshBegin(f);
	doit_fermer = 0;
	xmin = xmax = ymin = ymax = -1;
	switch(instrum->forme) {
	  case INS_CADRAN:
	  case INS_POTAR:
		cercle = (TypeInstrumCirc *)instrum->parms;
		if(instrum->forme == INS_CADRAN) {
			x = y = 0;
			l = cdr->larg - 1;
			h = cdr->haut - 1 - WndLigToPix(2);
			WndFillFgnd(f,WND_GC(f,instrum->gc.bgnd),x,y,l,h);
			if(instrum->support) WndEntoure(f,instrum->support,x,y,l,h);
			x = (int)((instrum->geom.cercle.x0 - 1.0) * cercle->rayon);
			y = (int)((instrum->geom.cercle.y0 - 1.0) * cercle->rayon);
			l = h = (int)(2 * cercle->rayon);
			a0 = (int)(cercle->angle_min * RADTODEG);
			da = (int)(cercle->angle_max * RADTODEG) - a0;
			WndArc(f,WND_GC(f,instrum->gc.fgnd),x,y,l,h,a0,da);
		} else {
			x = (int)((instrum->geom.cercle.x0 - 1.0) * cercle->rayon);
			y = (int)((instrum->geom.cercle.y0 - 1.0) * cercle->rayon);
			l = h = (int)(2 * cercle->rayon);
			WndSecteur(f,WND_GC(f,instrum->gc.bgnd),x,y,l,h,0,360);
			doit_fermer = WndBeginLine(f,WND_GC(f,instrum->gc.fgnd),GL_LINE_STRIP);
			WndArc(f,WND_GC(f,WND_GC_LITE),x,y,l,h,-135,180);
			WndArc(f,WND_GC(f,WND_GC_DARK),x,y,l,h,45,180);
			//- WndArc(f,WND_GC(f,WND_GC_LITE),x,y,l,h,-135,-180);
			//- WndArc(f,WND_GC(f,WND_GC_DARK),x,y,l,h,45,-180);
			//wx WndArc(f,WND_GC(f,WND_GC_LITE),x,y,l,h,45,180);
			//wx WndArc(f,WND_GC(f,WND_GC_DARK),x,y,l,h,-135,180);
			if(doit_fermer) WndEndLine(f);
		}
		if(cercle->graduation > 0.0) for(k=0; k<2; k++) {
			nb = 0; i = 0; j = 0; grad = gradinit; g1 = g1init; dev = dev1;
			if(k) { doit_fermer = WndBeginLine(f,WND_GC(f,instrum->gc.fgnd),GL_LINES); WndAbsLine(f); } else doit_fermer = 0;
			do {
				angle = (dev * (cercle->angle_max - cercle->angle_min)) + cercle->angle_min;
				sina = sin(angle); cosa = cos(angle);
				x = (int)((instrum->geom.cercle.x0 + sina) * cercle->rayon);
				y = (int)((instrum->geom.cercle.y0 - cosa) * cercle->rayon);
				l = (int)(sina * cercle->graduation);
				h = -(int)(cosa * cercle->graduation);
				if(k) WndLine(f,WND_GC(f,instrum->gc.fgnd),x,y,l,h);
				else {
					InstrumLegende(instrum,g1,nb,&i,&j,legende); c = (int)strlen(legende);
					x = x + l + (int)(0.5 * (sina - 1.0) * (double)WndColToPix(1));
					y = y + h - (int)(0.5 * (cosa - 1.0) * (double)WndLigToPix(1));
					if(((angle > -(0.3 * PI)) && (angle < (0.3 * PI)))
					|| (angle < -(0.7 * PI)) || (angle > (0.7 * PI)))
						x -= (WndColToPix(c - 1) / 2);
					else if((angle < -(0.3 * PI)) && (angle > -(0.7 * PI)))
						x -= WndColToPix(c - 1);
					x1 = x + WndColToPix(c);
					y1 = y + WndLigToPix(1);
					if((!instrum->log || (g1 < (2.0 * grad))) && ((x >= xmax) || (x1 <= xmin) || (y >= ymax) || (y1 <= ymin))) {
						WndString(f,WND_GC(f,instrum->gc.fgnd),x,y,legende);
						xmin = x; ymin = y;
						xmax = x1; ymax = y1;
					}
				}
				if(DEBUG_INSTRUM(2)) WndPrint("Graduation pour %s: dev=%g%%\n",legende,dev*100.0);
				g1 += grad;
				if(instrum->log) {
					if(instrum->log < 0) {
						if(g1 > (5.1 * grad)) g1 = 10.0 * grad;
						else if(g1 > (2.1 * grad)) g1 = 5.0 * grad;
					}
					dev = (log10(g1) - instrum->min) / instrum->echelle;
					if(g1 > (9.5 * grad)) grad *= 10.0;
				} else dev = (g1 - instrum->min) / instrum->echelle;
				if(++nb > 100) break;
			} while(dev <= 1.0);
			if(doit_fermer) WndEndLine(f);
		}
		break;

	  case INS_COLONNE:
	  case INS_GLISSIERE:
		rect = (TypeInstrumRect *)instrum->parms;
		if(instrum->forme == INS_COLONNE) {
			a0 = instrum->geom.rect.decal;
			da = rect->largeur;
			b0 = 0;
		} else {
			a0 = instrum->geom.rect.decal + (rect->largeur / 2) - 1;
			da = 3;
			b0 = rect->largeur / 2; /* 1/2 epaisseur bouton */
		}
		if(rect->horiz) {
			x = b0; y = a0;
			l = abs(rect->longueur); h = da;
		} else {
			x = a0; y = b0;
			l = da; h = abs(rect->longueur);
		}
		if(instrum->forme == INS_COLONNE) {
			WndFillBgnd(f,WND_GC(f,instrum->gc.bgnd),x,y,l,h);
			// printf("(%s) fond #%d\n",__func__,instrum->gc.bgnd);
			// printf("1er dessin: "); PRINT_FGC(f,instrum->gc.bgnd);
			if(instrum->support) WndEntoure(f,instrum->support,x,y,l,h);
		} else WndFillFgnd(f,WND_GC(f,WND_GC_DARK),x,y,l,h);
		if(rect->graduation > 0.0) for(k=0; k<2; k++) {
			nb = 0; i = 0; j = 0; grad = gradinit; g1 = g1init; dev = dev1;
			if(k) { doit_fermer = WndBeginLine(f,WND_GC(f,instrum->gc.fgnd),GL_LINES); WndAbsLine(f); } else doit_fermer = 0;
			do {
				da = (int)(dev * (double)rect->longueur);
				InstrumLegende(instrum,g1,nb,&i,&j,legende);
				c = (int)strlen(legende);
				if(rect->position <= 0) {
					if(rect->horiz) {
						x = da + b0;
						if(rect->longueur < 0) x -= rect->longueur;
						y = instrum->geom.rect.decal - rect->graduation;
						if(instrum->support) y -= 3;
						if(k) WndLine(f,WND_GC(f,instrum->gc.fgnd),x,y,0,rect->graduation);
						else x -= (WndColToPix(c) / 2);
					} else {
						x = instrum->geom.rect.decal - rect->graduation;
						if(instrum->support) x -= 3;
						y = b0 - da;
						if(rect->longueur > 0) y += rect->longueur;
						if(k) WndLine(f,WND_GC(f,instrum->gc.fgnd),x,y,rect->graduation,0);
						else { x -= WndColToPix(rect->nbchars); y += (WndLigToPix(1) / 2); }
					}
					if(!k) {
						x1 = x + WndColToPix(c);
						y1 = y + WndLigToPix(1);
						if((!instrum->log || (g1 < (2.0 * grad))) && ((x >= xmax) || (x1 <= xmin) || (y >= ymax) || (y1 <= ymin))) {
							WndString(f,WND_GC(f,instrum->gc.fgnd),x,y,legende);
							xmin = x; ymin = y;
							xmax = x1; ymax = y1;
						}
					}
				}
				if(rect->position >= 0) {
					if(rect->horiz) {
						x = da + b0;
						if(rect->longueur < 0) x -= rect->longueur;
						y = instrum->geom.rect.decal + rect->largeur;
						if(instrum->forme == INS_GLISSIERE) y += 1;
						if(instrum->support) y += 2;
						if(k) WndLine(f,WND_GC(f,instrum->gc.fgnd),x,y,0,rect->graduation);
						else if(rect->position > 0) {
							x -= (WndColToPix(c) / 2);
							y += (rect->graduation + WndLigToPix(1));
							x1 = x + WndColToPix(c);
							y1 = y + WndLigToPix(1);
							if((!instrum->log || (g1 < (2.0 * grad))) && ((x >= xmax) || (x1 <= xmin) || (y >= ymax) || (y1 <= ymin))) {
								WndString(f,WND_GC(f,instrum->gc.fgnd),x,y,legende);
								xmin = x; ymin = y;
								xmax = x1; ymax = y1;
							}
						}
					} else {
						x = instrum->geom.rect.decal + rect->largeur;
						if(instrum->support) x += 2;
						y = b0 - da;
						if(rect->longueur > 0) y += rect->longueur;
						if(k) WndLine(f,WND_GC(f,instrum->gc.fgnd),x,y,rect->graduation,0);
						else if(rect->position > 0) {
							x += rect->graduation + (WndColToPix(1) / 2);
							y += (WndLigToPix(1) / 2);
							x1 = x + WndColToPix(c);
							y1 = y + WndLigToPix(1);
							if((!instrum->log || (g1 < (2.0 * grad))) && ((x >= xmax) || (x1 <= xmin) || (y >= ymax) || (y1 <= ymin))) {
								WndString(f,WND_GC(f,instrum->gc.fgnd),x,y,legende);
								xmin = x; ymin = y;
								xmax = x1; ymax = y1;
							}
						}
					}
				}
				if(DEBUG_INSTRUM(2)) WndPrint("Graduation pour %s: dev=%g%%\n",legende,dev*100.0);
				g1 += grad;
				if(instrum->log) {
					if(instrum->log < 0) {
						if(g1 > (5.1 * grad)) g1 = 10.0 * grad;
						else if(g1 > (2.1 * grad)) g1 = 5.0 * grad;
					}
					dev = (log10(g1) - instrum->min) / instrum->echelle;
					if(g1 > (9.5 * grad)) grad *= 10.0;
				} else dev = (g1 - instrum->min) / instrum->echelle;
				if(++nb > 100) break;
			} while(dev <= 1.0);
			if(doit_fermer) WndEndLine(f);
		}
		break;

	  case INS_LEVIER:
		levier = (TypeInstrumLevier *)instrum->parms;
		y = levier->hauteur + (2 * instrum->geom.levier.htext);
		nb = 0; i = 0; j = 0;
		do {
			InstrumLegende(instrum,g1,nb,&i,&j,legende);
			c = (int)strlen(legende);
			x = (levier->largeur - WndColToPix(c)) / 2; if(x < 0) x = 0;
			x1 = x + WndColToPix(c);
			y1 = y + WndLigToPix(1);
			if((x >= xmax) || (x1 <= xmin) || (y >= ymax) || (y1 <= ymin)) {
				if(!instrum->log || (g1 > (9.0 * grad))) 
					WndString(f,WND_GC(f,instrum->gc.fgnd),x,y,legende);
				xmin = x; ymin = y;
				xmax = x1; ymax = y1;
			}
			if(instrum->log && ((g1 + grad) > (10.0 * grad))) grad *= 10.0;
			g1 += grad;
			if(instrum->log) {
				if(instrum->log < 0) {
					if(g1 > (5.1 * grad)) g1 = 10.0 * grad;
					else if(g1 > (2.1 * grad)) g1 = 5.0 * grad;
				}
				dev = (log10(g1) - instrum->min) / instrum->echelle;
				if(g1 > (9.5 * grad)) grad *= 10.0;
			} else dev = (g1 - instrum->min) / instrum->echelle;
			if(++nb > 2) break;
			y = instrum->geom.levier.htext;
		} while(dev <= 1.01);
		break;

	  case INS_BASCULE:
		bascule = (TypeInstrumBascule *)instrum->parms;
		nb = 0; i = 0; j = 0; l = 0;
		InstrumLegende(instrum,g1,nb,&i,&j,legende);
		if(legende[0]) {
			if(bascule->horiz) {
				WndString(f,WND_GC(f,instrum->gc.fgnd),0,WndLigToPix(1),legende);
				l = (int)strlen(legende);
			} else {
				y = cdr->haut - 1 - WndLigToPix(2);
				WndString(f,WND_GC(f,instrum->gc.fgnd),0,y,legende);
			}
		}
		InstrumLegende(instrum,g1,nb,&i,&j,legende);
		if(legende[0]) {
			if(bascule->horiz) {
				x = WndColToPix(l+2) + bascule->longueur;
				WndString(f,WND_GC(f,instrum->gc.fgnd),x,WndLigToPix(1),legende);
			} else {
				WndString(f,WND_GC(f,instrum->gc.fgnd),0,0,legende);
			}
		}
		break;

	  case INS_POUSSOIR:
		nb = 0; i = 0; j = 0;
		x = y = 0;
#ifdef QUICKDRAW
		InstrumLegende(instrum,g1,nb,&i,&j,legende);
		WndString(f,WND_GC(f,instrum->gc.fgnd),x,y+WndLigToPix(1),legende);
#endif
		y = cdr->haut - WndLigToPix(1);
		if(cdr->titre[0]) y -= WndLigToPix(1);
		WndEntoure(f,WND_PLAQUETTE,0,0,cdr->larg,y);
		break;
	  case INS_VOYANT:
		break;
	  case INS_RADIO:
		break;
	}

/* Transfert dans planches.c (== valable pour tous les cadres inclus) deja tente... */
	if(cdr->titre[0]) {
		x = (cdr->larg - WndColToPix((int)strlen(cdr->titre))) / 2;
		WndString(f,WND_GC(f,WND_GC_CLEAR),x,cdr->haut - 1,cdr->titre);
	}

	if(instrum->cdr != cdr_demande) {
		OpiumError("(OpiumRefreshInstrum) cadre demande: %08lX, rafraichi: %08lX\n",cdr_demande,instrum->cdr);
		return(0);
	}
	InstrumRefreshVar(instrum);
	if(doit_terminer) WndRefreshEnd(f);

	return(1);
}
/* ========================================================================== */
int InstrumRefreshVar(Instrum instrum) {
	char c; short s; int i; int64 i64; float r; double val; int n; short nb; char b;
	double dev;

	if(!instrum) return(0);
	if(instrum->echelle == 0.0) dev = 0.0;
	else {
		switch(instrum->type) {
		  case PNL_SHORT:
			s = *(short *)(instrum->adrs);
			if(s < instrum->lim.s.min) s = instrum->lim.s.min;
			else if(s > instrum->lim.s.max) s = instrum->lim.s.max;
			val = (double)s;
			break;
		  case PNL_INT:
			i = *(int *)(instrum->adrs);
			if(i < instrum->lim.i.min) i = instrum->lim.i.min;
			else if(i > instrum->lim.i.max) i = instrum->lim.i.max;
			val = (double)i;
			break;
		  case PNL_FLOAT:
			r = *(float *)(instrum->adrs);
			if(r < instrum->lim.f.min) r = instrum->lim.f.min;
			else if(r > instrum->lim.f.max) r = instrum->lim.f.max;
			val = (double)r;
			break;
		  case PNL_DBLE:
			val = *(double *)(instrum->adrs);
			if(val < instrum->lim.d.min) val = instrum->lim.d.min;
			else if(val > instrum->lim.d.max) val = instrum->lim.d.max;
			break;
		  case PNL_LISTE: case PNL_KEY:
			i = *(int *)(instrum->adrs);
			n = (int)instrum->echelle;
			if(i < 0) i = 0; else if(i > n) i = n;
			val = (double)i;
			break;
		  case PNL_LISTD: case PNL_KEYL:
			i64 = *(int64 *)(instrum->adrs);
			n = (int)instrum->echelle;
			if(i64 < 0) i64 = 0; else if(i64 > n) i64 = n;
			val = (double)i64;
			break;
		  case PNL_LISTS: case PNL_KEYS:
			s = *(short *)(instrum->adrs);
			nb = (short)instrum->echelle;
			if(s < 0) s = 0; else if(s > nb) s = nb;
			val = (double)s;
			break;
		  case PNL_LISTB: case PNL_KEYB:
			c = *(char *)(instrum->adrs);
			b = (char)instrum->echelle;
			if(c < 0) c = 0; else if(c > b) c = b;
			val = (double)c;
			break;
		  case PNL_BOOL:
			val = (*(char *)(instrum->adrs))? 1.0: 0.0;
		  default: val = 0.0; break;
		}
		if(instrum->log) {
			if(val > 0.0) dev = (log10(val) - instrum->min) / instrum->echelle;
			else dev = 0;
		} else dev = (val - instrum->min) / instrum->echelle;
	}
	if(instrum->deja_trace && (dev == instrum->dev)) return(0);
	return(InstrumUpdate(instrum,dev));
}
/* ========================================================================== */
static int InstrumUpdate(Instrum instrum, double dev) {
	WndFrame f;
	TypeInstrumCirc *cercle; TypeInstrumRect *rect; TypeInstrumLevier *levier;
	TypeInstrumBascule *bascule;
	int x,y,l,h,v,da; char doit_fermer,doit_terminer;
	int nb,i,j; char legende[1024]; double g1; Cadre cdr;
	int epaisr,de_face,a_plat;
	double angle;

	cdr = instrum->cdr;
	if(!OpiumCoordFenetre(cdr,&f)) return(-1);

	doit_terminer = WndRefreshBegin(f);
	doit_fermer = 0;
	switch(instrum->forme) {
	  case INS_CADRAN: case INS_POTAR:
		cercle = (TypeInstrumCirc *)instrum->parms;
		doit_fermer = WndBeginLine(f,WND_STD,GL_LINES); WndAbsLine(f);
		if(instrum->deja_trace) {
			x = instrum->trace.x;
			y = instrum->trace.y;
			l = instrum->trace.l;
			h = instrum->trace.h;
			WndLine(f,WND_GC(f,instrum->gc.bgnd),x,y,l,h);
		}
		angle = (dev * (cercle->angle_max - cercle->angle_min)) + cercle->angle_min;
		x = (int)(instrum->geom.cercle.x0 * cercle->rayon);
		y = (int)(instrum->geom.cercle.y0 * cercle->rayon);
		l = (int)(sin(angle) * cercle->rayon);
		h = -(int)(cos(angle) * cercle->rayon);
		if(instrum->forme == INS_CADRAN) WndLine(f,WND_GC(f,WND_GC_STD),x,y,l,h);
		else WndLine(f,WND_GC(f,WND_GC_CLEAR),x,y,l,h);
		instrum->trace.x = x;
		instrum->trace.y = y;
		instrum->trace.l = l;
		instrum->trace.h = h;
		break;

	  case INS_COLONNE:
	  case INS_GLISSIERE:
		rect = (TypeInstrumRect *)instrum->parms;
		if(instrum->deja_trace) {
			x = instrum->trace.x;
			y = instrum->trace.y;
			l = instrum->trace.l;
			h = instrum->trace.h;
			if(instrum->forme == INS_COLONNE) {
				WndFillBgnd(f,WND_GC(f,instrum->gc.bgnd),x,y,l,h);
			} else {
				if(rect->horiz) {
					WndFillFgnd(f,WND_GC(f,instrum->relief),x,y,l,h);
					y = instrum->geom.rect.decal + (rect->largeur / 2) - 1; h = 3;
					WndFillFgnd(f,WND_GC(f,WND_GC_DARK),x,y,l,h);
				} else {
					y -= (h / 2);
					WndFillFgnd(f,WND_GC(f,instrum->relief),x+1,y-1,l,h+2);
					x = instrum->geom.rect.decal + (rect->largeur / 2) - 1; l = 3;
					WndFillFgnd(f,WND_GC(f,WND_GC_DARK),x,y-1,l,h+2);
				}
			}
		}
		da = (int)(dev * (double)rect->longueur);
		if(rect->horiz) {
			y = instrum->geom.rect.decal; h = rect->largeur;
			if(instrum->forme == INS_COLONNE) {
				l = abs(da);
				if(rect->longueur > 0) x = 0; else x = da - rect->longueur;
				WndFillFgnd(f,WND_GC(f,instrum->gc.bgnd),x,y,l,h);
			} else {
				x = da + (rect->largeur / 2); /* position ou a ete tracee la graduation */
				if(rect->longueur < 0) x -= rect->longueur;
				l = rect->largeur / 3;
				WndFillFgnd(f,WND_GC(f,WND_GC_LITE),x-(rect->largeur/2),y,l,h);
				WndFillFgnd(f,WND_GC(f,instrum->gc.bgnd),x-(rect->largeur/6),y,l,h);
				WndFillFgnd(f,WND_GC(f,WND_GC_DARK),x+(rect->largeur/6),y,l,h);
				doit_fermer = WndBeginLine(f,WND_STD,GL_LINES); WndAbsLine(f);
				// WndRelief(f,2,WND_BORD_HAUT,x-(rect->largeur/2)+1,y+2,rect->largeur-3);
				// WndRelief(f,2,WND_BORD_BAS,x-(rect->largeur/2)+1,y+rect->largeur,rect->largeur-3);
				// WndLine(f,WND_GC(f,WND_GC_CLEAR),x,y,0,h);
				// x = x-(rect->largeur/2); l = rect->largeur; y -= 1; h += 2;
				WndRelief(f,1,WND_BORD_HAUT,x-(rect->largeur/2)+1,y+1,rect->largeur-3);
				WndRelief(f,1,WND_BORD_BAS,x-(rect->largeur/2)+1,y+rect->largeur,rect->largeur-3);
				WndLine(f,WND_GC(f,WND_GC_CLEAR),x,y,0,h);
				x = x-(rect->largeur/2); l = rect->largeur; y-= 1; h += 1;
			}
		} else {
			x = instrum->geom.rect.decal; l = rect->largeur;
			if(rect->longueur > 0) y = rect->longueur - da; else y = 0;
			if(instrum->forme == INS_COLONNE) {
				h = abs(da);
				WndFillFgnd(f,WND_GC(f,instrum->gc.bgnd),x,y,l,h);
				// printf("niveau: "); PRINT_FGC(f,instrum->gc.bgnd);
			} else {
				l -= 1;
				y += (rect->largeur / 2);
				h = rect->largeur / 3;
				WndFillFgnd(f,WND_GC(f,WND_GC_LITE),x,y-(rect->largeur/2),l,h);
				WndFillFgnd(f,WND_GC(f,instrum->gc.bgnd),x,y-(rect->largeur/6),l,h);
				WndFillFgnd(f,WND_GC(f,WND_GC_DARK),x,y+(rect->largeur/6),l,h);
				doit_fermer = WndBeginLine(f,WND_STD,GL_LINES); WndAbsLine(f);
				WndRelief(f,2,WND_BORD_GCHE,x+2,y-(rect->largeur/2)+1,rect->largeur-3);
				WndRelief(f,2,WND_BORD_DRTE,x+rect->largeur-1,y-(rect->largeur/2)+1,rect->largeur-3);
				WndLine(f,WND_GC(f,WND_GC_CLEAR),x+2,y,l-4,0);
				x -= 2; l += 2; h = rect->largeur;
			}
		}
		instrum->trace.x = x;
		instrum->trace.y = y;
		instrum->trace.l = l;
		instrum->trace.h = h;
		break;

	  case INS_LEVIER:
		levier = (TypeInstrumLevier *)instrum->parms;
		if(instrum->deja_trace) {
			x = instrum->trace.x;
			y = instrum->trace.y;
			l = instrum->trace.l;
			h = instrum->trace.h;
			WndFillFgnd(f,WND_GC(f,instrum->relief),x,y,l,h);
		}
		x = 0; l = levier->largeur;
		epaisr = 6; /* epaisseur apparente */
		a_plat = 4;
		de_face = levier->hauteur - epaisr - a_plat;
		y = instrum->geom.levier.htext;
		if(dev < 0.5) {
			WndFillFgnd(f,WND_GC(f,WND_GC_LITE),x,y,l,de_face);
			WndFillFgnd(f,WND_GC(f,instrum->gc.bgnd),x,y+de_face,l,epaisr);
			WndFillFgnd(f,WND_GC(f,WND_GC_DARK),x,y+de_face+epaisr,l,a_plat);
		} else {
			WndFillFgnd(f,WND_GC(f,WND_GC_LITE),x,y,l,a_plat);
			WndFillFgnd(f,WND_GC(f,instrum->gc.bgnd),x,y+a_plat,l,epaisr);
			WndFillFgnd(f,WND_GC(f,WND_GC_DARK),x,y+a_plat+epaisr,l,de_face);
		}
		doit_fermer = WndBeginLine(f,WND_STD,GL_LINES); WndAbsLine(f);
		WndRelief(f,2,WND_BORD_GCHE,x+2,y+2,levier->hauteur-4);
		WndRelief(f,2,WND_BORD_DRTE,x+l-2,y+2,levier->hauteur-4);
		instrum->trace.x = x;
		instrum->trace.y = y;
		instrum->trace.l = l;
		instrum->trace.h = levier->hauteur;
		break;
	  case INS_BASCULE:
		bascule = (TypeInstrumBascule *)instrum->parms;
		l = bascule->longueur; h = bascule->largeur;
		epaisr = 2;
		nb = 0; i = 0; j = 0; g1 = 0.0;
		InstrumLegende(instrum,g1,nb,&i,&j,legende);
		if(bascule->horiz) {
			if(legende[0]) x = WndColToPix((int)strlen(legende)+1);
			else x = 0;
//			x += epaisr;
			if(dev < 0.5) {
				WndFillFgnd(f,WND_GC(f,instrum->relief),x,0,l/2,h);
				WndFillFgnd(f,WND_GC(f,WND_GC_LITE),x+l/2,0,l/2,h);
//				WndFillFgnd(f,WND_GC(f,instrum->gc.bgnd),x+l,0,epaisr,h);
//				WndFillFgnd(f,WND_GC(f,WND_GC_LITE),x+l/2,0,l/2,epaisr);
//				WndFillFgnd(f,WND_GC(f,WND_GC_DARK),x+l/2,h,l/2,epaisr);
			} else {
//				WndFillFgnd(f,WND_GC(f,instrum->gc.bgnd),x-epaisr,0,epaisr,h);
				WndFillFgnd(f,WND_GC(f,WND_GC_DARK),x,0,l/2,h);
				WndFillFgnd(f,WND_GC(f,instrum->relief),x+l/2,0,l/2,h);
//				WndFillFgnd(f,WND_GC(f,WND_GC_LITE),x,0,l/2,epaisr);
//				WndFillFgnd(f,WND_GC(f,WND_GC_DARK),x,h,l/2,epaisr);
			}
			doit_fermer = WndBeginLine(f,WND_STD,GL_LINES); WndAbsLine(f);
			WndEntoure(f,WND_PLAQUETTE,x+2,2,l-2,h-2);
		} else {
			g1 = 0.0;
			InstrumLegende(instrum,g1,nb,&i,&j,legende);
			if(legende[0]) y = WndLigToPix(2); else y = 0;
//			y += epaisr;
			if(dev < 0.5) {
//				WndFillFgnd(f,WND_GC(f,instrum->gc.bgnd),0,y-epaisr,h,epaisr);
				WndFillFgnd(f,WND_GC(f,WND_GC_DARK),0,y,h,l/2);
				WndFillFgnd(f,WND_GC(f,instrum->relief),0,y+l/2,h,l/2);
//				WndFillFgnd(f,WND_GC(f,WND_GC_LITE),0,y,epaisr,l/2);
//				WndFillFgnd(f,WND_GC(f,WND_GC_DARK),h,y,epaisr,l/2);
			} else {
				WndFillFgnd(f,WND_GC(f,instrum->relief),0,y,h,l/2);
				WndFillFgnd(f,WND_GC(f,WND_GC_LITE),0,y+l/2,h,l/2);
//				WndFillFgnd(f,WND_GC(f,instrum->gc.bgnd),0,y+l,h,epaisr);
//				WndFillFgnd(f,WND_GC(f,WND_GC_LITE),0,y+l/2,epaisr,l/2);
//				WndFillFgnd(f,WND_GC(f,WND_GC_DARK),h,y+l/2,epaisr,l/2);
			}
			doit_fermer = WndBeginLine(f,WND_STD,GL_LINES); WndAbsLine(f);
			WndEntoure(f,WND_PLAQUETTE,2,y+2,h-2,l-2);
		}
		break;
	  case INS_POUSSOIR:
		nb = 0; i = 0; j = 0; g1 = 0.0;
		InstrumLegende(instrum,g1,nb,&i,&j,legende);
		if(dev > 0.5) InstrumLegende(instrum,g1,nb,&i,&j,legende);
		x = y = 0;
		WndErase(f,x,y,cdr->larg,WndLigToPix(1));
		WndString(f,WND_GC(f,instrum->gc.fgnd),x,y+WndLigToPix(1),legende);
		h = cdr->larg; v = cdr->haut - WndLigToPix(3);
		epaisr = 3;
		doit_fermer = WndBeginLine(f,WND_STD,GL_LINES); WndAbsLine(f);
		if(dev < 0.5) {
			// WndEntoure(f,WND_PLAQUETTE,0,0,cdr->larg,cdr->haut-WndLigToPix(2));
			WndRelief(f,epaisr,WND_BORD_HAUT,x,y,h);
			WndRelief(f,epaisr,WND_BORD_DRTE,x+h,y,v);
			WndRelief(f,epaisr,WND_BORD_BAS, x,y+v,h);
			WndRelief(f,epaisr,WND_BORD_GCHE,x,y,v);
		} else {
			// WndEntoure(f,WND_CREUX,0,0,cdr->larg,cdr->haut-WndLigToPix(2));
			WndRelief(f,epaisr,WND_FOND_HAUT,x,y,h);
			WndRelief(f,epaisr,WND_FOND_DRTE,x+h,y,v);
			WndRelief(f,epaisr,WND_FOND_BAS, x,y+v,h);
			WndRelief(f,epaisr,WND_FOND_GCHE,x,y,v);
		}
		break;
	  case INS_VOYANT:
		break;
	  case INS_RADIO:
		break;
	}
	if(doit_fermer) WndEndLine(f);
	if(doit_terminer) WndRefreshEnd(f);
	instrum->dev = dev;
	instrum->deja_trace = 1;

	return(1);
}
/* ========================================================================== */
static char InstrumCode(Instrum instrum, char etape, char *texte /*, int *col*/) {
    int i,j,k,d; int64 l;
	char **liste; char *modeles;

	switch(etape) {
	  case PNL_WRITE:
		switch(instrum->type) {
		  case PNL_INT: sprintf(texte,"%d",*(int *)instrum->adrs); break;
		  case PNL_SHORT: sprintf(texte,"%d",*(short *)instrum->adrs); break;
		  case PNL_FLOAT: sprintf(texte,"%g",*(float *)instrum->adrs); break;
		  case PNL_DBLE: sprintf(texte,"%g",*(double *)instrum->adrs); break;
		  default:
			switch(instrum->type) {
			  case PNL_LISTE: case PNL_KEY:  i = *(int *)instrum->adrs; break;
                case PNL_LISTD: case PNL_KEYL: l = *(int64 *)instrum->adrs; i = (int)l; break;
			  case PNL_LISTS: case PNL_KEYS: i = *(short *)instrum->adrs; break;
			  case PNL_BOOL:
			  case PNL_LISTB: case PNL_KEYB: i = *(char *)instrum->adrs; break;
			  default: i = 0;
			}
			if((instrum->type >= PNL_LISTE) && (instrum->type <= PNL_LISTB)) {
				liste = (char **)(instrum->lim.l.liste);
				strcpy(texte,liste[i]);
			} else if(((instrum->type >= PNL_KEY) && (instrum->type <= PNL_KEYB)) || (instrum->type == PNL_BOOL)) {
				modeles = (char *)(instrum->lim.l.liste);
				k = 0; j = -1; d = 0;
				do {
					if((modeles[k] == '/') || (modeles[k] == '\0')) {
						if(++j == i) break;
						d = k + 1;
					}
					k++;
				} while(modeles[k - 1] != '\0');
				strncpy(texte,modeles+d,k - d); texte[k - d] = '\0';
			}
			break;
		}
		break;
	  case PNL_VERIFY: break;
	  case PNL_READ:
		switch(instrum->type) {
		  case PNL_INT: sscanf(texte,"%d",(int *)instrum->adrs); break;
		  case PNL_SHORT: sscanf(texte,"%hd",(short *)instrum->adrs); break;
		  case PNL_FLOAT: sscanf(texte,"%g",(float *)instrum->adrs); break;
		  case PNL_DBLE: sscanf(texte,"%lg",(double *)instrum->adrs); break;
		}
		break;
	}
	return(1);
}
/* ========================================================================== */
int OpiumRunInstrum(Cadre cdr, WndAction *e) {
/* Seulement pour les instruments de commande! */
	Instrum instrum; WndFrame f;
	TypeInstrumCirc *cercle; TypeInstrumRect *rect;
	short lig,col;
	char avant[256],valeur[256];
	int x,y,l;
	double cde,dev,val;
	int code_rendu,change;
	char return_done;
	TypeFctn fctn;

	if(DEBUG_OPIUM(1) && (cdr->type != OPIUM_INSTRUM)) {
		WndPrint("+++ OpiumRunPrompt sur le %s @%08lX\n",cdr->nom,(Ulong)cdr->adrs);
		return(0);
	}
	instrum = (Instrum)cdr->adrs;
	if(!OpiumCoordFenetre(cdr,&f)) return(0);
	code_rendu = -1;
	if(e->type == WND_KEY) {
		/* entree directe de la valeur */
		l = (int)strlen(e->texte);
		if(l <= 0) {
			if(e->code == XK_KP_F4) code_rendu = 0;
		} else {
			return_done = (e->texte[l - 1] == 0x0D);
/*			if(return_done) WndPrint("(OpiumRunInstrum) <Return> recu\n"); */
			if(return_done) { e->texte[--l] = '\0'; code_rendu = 1; }
			lig = f->lig - f->lig0;
//			if((lig < 0) || (lig >= ???)) lig = 0;
			col = f->col - f->col0;
			if((col < 0) || (col >= 256)) col = 0;
			if(l) {
				InstrumCode(instrum,PNL_WRITE,avant);
				if(col) {
					strncpy(valeur,avant,col); valeur[col] = '\0';
					strcat(valeur,e->texte);
					strcat(valeur,avant+col);
				} else strcpy(valeur,e->texte);
				if(InstrumCode(instrum,PNL_VERIFY,valeur)) {
					InstrumCode(instrum,PNL_READ,valeur);
					cdr->var_modifiees = 1;
					col += l;
				}
			}
			if(f) {
				if(return_done) col = 0;
				WndCursorSet(f,lig,col);
			}
		}
//	} else if((e->type == WND_RELEASE) || ((e->type == WND_PRESS) && (e->code == WND_MSERIGHT))) {
	} else if((e->type == WND_RELEASE) || (e->type == WND_PRESS)) {
		switch(instrum->forme) {
		  case INS_CADRAN: case INS_POTAR:
			cercle = (TypeInstrumCirc *)instrum->parms;
			x = e->x - (int)(instrum->geom.cercle.x0 * cercle->rayon);
			y = e->y - (int)(instrum->geom.cercle.y0 * cercle->rayon);
			if(y == 0) cde = (x > 0)? PI / 2.0: -PI / 2.0;
			else {
				cde = atan(-(double)x / (double)y);
				if(y > 0) cde = (x > 0)? cde + PI: cde - PI;
			}
			if(cde > cercle->angle_max) cde = cercle->angle_max;
			else if(cde < cercle->angle_min) cde = cercle->angle_min;
			dev = (cde - cercle->angle_min) / (cercle->angle_max - cercle->angle_min);
			if(DEBUG_INSTRUM(3)) WndPrint("Angle=%g, soit dev=%g%%\n",cde,dev*100.0);
			break;
		  case INS_COLONNE: case INS_GLISSIERE:
			rect = (TypeInstrumRect *)instrum->parms;
			  if(rect->horiz) {
				  cde = (double)e->x;
				  dev = (cde - (double)(rect->largeur/ 2)) / (double)rect->longueur;
			  } else {
				  cde = (double)(rect->longueur - e->y);
				  dev = (cde + (double)(rect->largeur/ 2)) / (double)rect->longueur;
			  }
			if(DEBUG_INSTRUM(3)) WndPrint("Position=%g, soit dev=%g%%\n",cde,dev*100.0);
			break;
		  case INS_LEVIER: case INS_BASCULE: case INS_POUSSOIR:
			if(instrum->dev < 0.5) dev = 1.0; else dev = 0.0;
			break;
		  default: dev = instrum->dev; break;
		}
		if(dev != instrum->dev) {
			val = (dev * instrum->echelle) + instrum->min;
			if(DEBUG_INSTRUM(3)) WndPrint("echelle=%g d'ou valeur=%g\n",instrum->echelle,val);
			if(instrum->log > 0) val = pow(10.0,val);
			else if(instrum->log < 0) {
				int k,d,g;
				k = (int)val;
				d = (int)(pow(10.0,(double)k));
				g = (int)(pow(10.0,val) / (double)d);
				if(g > 5) g = 5; else if(g > 2) g = 2; else g = 1;
				val = (float)(g * d);
			}
			if(DEBUG_INSTRUM(3)) WndPrint("en fait, valeur=%g\n",val);
			switch(instrum->type) {
			  case PNL_SHORT: *(short *)(instrum->adrs) = (short)(val + 0.4); break;
			  case PNL_INT:   *(int *)(instrum->adrs) = (int)(val + 0.4); break;
			  case PNL_FLOAT: *(float *)(instrum->adrs) = (float)val; break;
			  case PNL_DBLE:  *(double *)(instrum->adrs) = val; break;
			  case PNL_LISTE: case PNL_KEY: *(int *)(instrum->adrs) = (int)(val + 0.4); break;
			  case PNL_LISTD: case PNL_KEYL: *(int64 *)(instrum->adrs) = (int64)(val + 0.4); break;
			  case PNL_LISTS: case PNL_KEYS: *(short *)(instrum->adrs) = (short)(val + 0.4); break;
			  case PNL_BOOL:
			  case PNL_LISTB: case PNL_KEYB: *(char *)(instrum->adrs) = (char)(val + 0.4); break;
			}
			change = InstrumRefreshVar(instrum);
			if(change) {
				cdr->var_modifiees = 1;
				if((fctn = instrum->on_change)) {
					void (*fctnTmp)(Instrum, void *arg) =  (void (*)(Instrum, void *arg))fctn;
					(*fctnTmp)(instrum,instrum->change_arg);
				}
			}
		}
	}

	if(DEBUG_INSTRUM(1)) WndPrint("(OpiumRunInstrum) Code rendu: %d\n",code_rendu);
	return(code_rendu);
}
/* ========================================================================== */
void InstrumDelete(Instrum instrum) {
	OpiumDelete(instrum->cdr);
	free(instrum);
}

