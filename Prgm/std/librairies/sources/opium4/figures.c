#include <stdlib.h> /* pour malloc et free */
#include <stdarg.h>

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

/* ========================================================================== */
Figure FigureCreate(char type, void *forme, void *adrs, TypeFigFctn fctn) {
	Figure fig; Cadre cdr;

	fig = (Figure)malloc(sizeof(TypeFigure));
	if(fig) {
		if(DEBUG_FIGURE(1)) WndPrint("Figure creee @%08lX\n",(Ulong)fig);
		cdr = OpiumCreate(OPIUM_FIGURE,fig);
		if(!cdr) { free(fig); return(0); }
		if(DEBUG_FIGURE(1)) WndPrint("Figure creee @%08lX: %s\n",(Ulong)fig,cdr->nom);
		fig->cdr = cdr;
		if(type == FIG_ZONE) fig->p.c = 0; else fig->p.gc = 0;
		fig->forme_tempo = 0;
		FigureChangeForme(fig,type,forme);
		fig->cote_texte = FIG_CENTRE;
		fig->texte[0] = '\0';
		fig->adrs = adrs;
		fig->fctn = fctn;
		fig->dessin = fig->icone = 0;
	}
	return(fig);
}
/* ========================================================================== */
void FigureChangeForme(Figure fig, char type, void *forme) {
	fig->type = type;
	switch(type) {
		case FIG_DROITE: fig->forme.droite = (TypeFigDroite *)forme; break;
		case FIG_LIEN:   fig->forme.lien   = (TypeFigLien   *)forme; break;
		case FIG_ZONE:
		case FIG_CADRE:  fig->forme.zone   = (TypeFigZone   *)forme; break;
		case FIG_ICONE:  fig->forme.icone  = (TypeFigIcone  *)forme; break;
	}
	fig->ecrit = (fig->type == FIG_DROITE)? WND_GC_CLEAR: WND_GC_NOIR; /* ??? */
	// printf("(%s:%d) %s @%08lX: couleur ecriture #%d\n",__func__,__LINE__,FigTypeNom[fig->type],(Ulong)fig,fig->ecrit);
	if(((fig->type == FIG_DROITE) || (fig->type == FIG_DROITE)) && fig->p.gc) {
		WndFrame f;
		OpiumCoordFenetre(fig->cdr,&f); WndContextFree(f,fig->p.gc); fig->p.gc = 0;
	} else if((fig->type == FIG_ZONE) && fig->p.c) { WndColorFree(fig->p.c); fig->p.c = 0; }
	if(fig->type == FIG_ZONE) (fig->cdr)->support = ((TypeFigZone *)forme)->support;
}
/* ========================================================================== */
void FigureFormeTempo(Figure fig, char tempo) { if(fig) fig->forme_tempo = tempo; }
/* ========================================================================== */
void FigureDelete(Figure fig) {
	if(fig) {
		OpiumDelete(fig->cdr);
		if(fig->forme_tempo) free(fig->forme.adrs);
		free(fig);
	}
}
/* ========================================================================== */
void FigureTitle(Figure fig, char *texte) { if(fig) OpiumTitle(fig->cdr,texte); }
/* ========================================================================== */
void FigureTexte(Figure fig, char *texte) {
	if(!fig) return;
	if(texte) {
		strncpy(fig->texte,texte,FIG_MAXTEXTE);
	} else fig->texte[0] = '\0';
	fig->cote_texte = FIG_CENTRE;
}
/* ========================================================================== */
void FigureLegende(Figure fig, FIG_COTE cote, char *texte) {
	if(!fig) return;
	fig->texte[0] = '\0'; fig->cote_texte = FIG_CENTRE;
	if(texte) {
		if(strlen(texte)) {
			strncpy(fig->texte,texte,FIG_MAXTEXTE);
			fig->cote_texte = cote;
		}
	}
}
/* ========================================================================== */
void FigureEcrit(Figure fig, WND_GC_COUL ecrit) { if(fig) fig->ecrit = (char)ecrit; }
/* ========================================================================== */
void FigureColor(Figure fig, WndColorLevel r, WndColorLevel g, WndColorLevel b) {
	TypeFigDroite *droite; TypeFigLien *lien; TypeFigZone *zone; // TypeFigIcone *icone;
	WndFrame f;

	if(!fig) return;
	switch(fig->type) {
	  case FIG_DROITE:
		if(fig->p.gc) { OpiumCoordFenetre(fig->cdr,&f); WndContextFree(f,fig->p.gc); fig->p.gc = 0; }
		droite = fig->forme.droite;
		droite->r = r; droite->g = g; droite->b = b;
		break;
	  case FIG_LIEN:
		if(fig->p.gc) { OpiumCoordFenetre(fig->cdr,&f); WndContextFree(f,fig->p.gc); fig->p.gc = 0; }
		lien = fig->forme.lien;
		lien->r = r; lien->g = g; lien->b = b;
		break;
	  case FIG_ZONE: case FIG_CADRE:
		if(fig->p.c) { WndColorFree(fig->p.c); fig->p.c = 0; }
		zone = fig->forme.zone;
		zone->r = r; zone->g = g; zone->b = b;
		break;
	}
}
/* ========================================================================== */
void OpiumFigureColorNb(Cadre cdr, Cadre fait, short *nc) {
	Figure fig,modele; short ic;

	if(!(fig = (Figure)cdr->adrs)) return;
	if(fait) modele = (Figure)fait->adrs; else modele = 0;
	if(modele) {
		fig->dessin = modele->dessin; fig->icone = modele->icone;
	} else {
		ic = *nc;
		fig->dessin = ic++; fig->icone = ic++;
		*nc = ic;
	}
}
/* ========================================================================== */
void OpiumFigureColorSet(Cadre cdr, WndFrame f) {
	Figure fig;

	if(!(fig = (Figure)cdr->adrs)) return;
	WndFenColorSet(f,fig->dessin,WND_GC_NOIR);
	WndFenColorSet(f,fig->icone,WND_GC_GREY);
}
/* ========================================================================== */
int  OpiumSizeFigure(Cadre cdr, char from_wm) {
	Figure fig; TypeFigDroite *droite; TypeFigLien *lien; TypeFigZone *zone; TypeFigIcone *icone;
	int l;
	
	if(from_wm) { return(1); }
	if(!cdr) return(0);
	fig = (Figure)cdr->adrs;
	if(!fig) return(0);
	switch(fig->type) {
	  case FIG_DROITE:
		droite = fig->forme.droite;
		if(droite->dx > 0) cdr->larg = droite->dx; else if(droite->dx < 0) cdr->larg = -droite->dx;
		else if(droite->relief) cdr->larg = 2;
		else cdr->larg = 2; //1
		if(droite->dy > 0) cdr->haut = droite->dy; else if(droite->dy < 0) cdr->haut = -droite->dy;
		else if(droite->relief) cdr->haut = 2;
		else cdr->haut = 2; //1
		break;
	  case FIG_LIEN:
		lien = fig->forme.lien;
		if(lien->dx > 0) cdr->larg = lien->dx; else if(lien->dx < 0) cdr->larg = -lien->dx;
		else if(lien->relief) cdr->larg = 2;
		else cdr->larg = 2; //1
		if(lien->dy > 0) cdr->haut = lien->dy; else if(lien->dy < 0) cdr->haut = -lien->dy;
		else if(lien->relief) cdr->haut = 2;
		else cdr->haut = 2; //1
		break;
	  case FIG_ZONE: case FIG_CADRE:
		zone = fig->forme.zone;
		cdr->larg = zone->larg; cdr->haut = zone->haut;
		break;
	  case FIG_ICONE:
		icone = fig->forme.icone;
		cdr->larg = icone->larg; cdr->haut = icone->haut;
		break;
	}
	l = (int)strlen(fig->texte);
	if(l) switch(fig->cote_texte) {
		case FIG_A_GAUCHE:   case FIG_A_DROITE:  cdr->larg += WndColToPix(l + 1); break;
		case FIG_EN_DESSOUS: case FIG_AU_DESSUS: cdr->haut += WndLigToPix(1); break;
		case FIG_CENTRE: if(WndColToPix(l) > cdr->larg) cdr->larg = WndColToPix(l); break;
        default: break;
	}
	cdr->dh = cdr->larg; cdr->dv = cdr->haut;
	return(1);
}
/* ========================================================================== */
static void FigureTraceDroite(Figure fig, WndFrame f, TypeFigDroite *droite, int x, int y) {
	char fait;

	if(!fig) return;
	if(!fig->p.gc) {
		fig->p.gc = WndContextCreate(f);
		WndContextFgndRGB(f,fig->p.gc,droite->r,droite->g,droite->b);
	}
	// printf("(%s) Droite entre (%d, %d) et (%d, %d)\n",__func__,x,y,x+droite->dx,y+droite->dy);
	fait = 1;
	switch(droite->relief) {
	  case WND_RAINURES: 
		if(droite->dx && !(droite->dy)) WndRelief(f,2,WND_RAINURE | WND_HORIZONTAL,x,y,droite->dx);
		else if(!(droite->dx) && droite->dy) WndRelief(f,2,WND_RAINURE | WND_VERTICAL,x,y,droite->dy);
		else fait = 0;
		break;
	  case WND_REGLETTES: 
		if(droite->dx && !(droite->dy)) WndRelief(f,2,WND_REGLETTE | WND_HORIZONTAL,x,y,droite->dx);
		else if(!(droite->dx) && droite->dy) WndRelief(f,2,WND_REGLETTE | WND_VERTICAL,x,y,droite->dy);
		else fait = 0;
		break;
	  default: fait = 0;
	}
	if(!fait) WndAddLine(f,x,y,droite->dx,droite->dy);
}
/* ========================================================================== */
char FigureRedessine(Figure fig, char plus_fond) {
	Cadre cdr; TypeFigDroite *droite; TypeFigLien *lien; TypeFigZone *zone; TypeFigIcone *icone;
	WndFrame f; // WND_GC_COUL couleur;
	TypeFigDroite segment; int forme; int x,y,xd,yd,k; int l; // size_t l;
	char doit_fermer;

	if(!fig) return(0);
	cdr = fig->cdr; 
	if(!cdr) return(0);
	if(!OpiumCoordFenetre(cdr,&f)) {
		printf("(%s) Figure %s: pas dans une fenetre\n",__func__,cdr->nom);
		return(0);
	}
	droite = 0; zone = 0; icone = 0;
	xd = 0; yd = 0;
	l = (int)strlen(fig->texte);
	if(l) switch(fig->cote_texte) {
		case FIG_A_GAUCHE:   xd = WndColToPix(l + 1); break;
		case FIG_AU_DESSUS:  yd = WndLigToPix(1); break;
        default: break;
	}
	switch(fig->type) {
	  case FIG_DROITE:
		droite = fig->forme.droite;
		doit_fermer = WndBeginLine(f,WND_TEXT,GL_LINES); WndAbsLine(f);
		FigureTraceDroite(fig,f,droite,xd,yd);
		if(doit_fermer) WndEndLine(f);
		break;
	  case FIG_LIEN:
		lien = fig->forme.lien;
		segment.relief = lien->relief; segment.r = lien->r; segment.g = lien->g; segment.b = lien->b;
		doit_fermer = WndBeginLine(f,WND_TEXT,GL_LINES); WndAbsLine(f);
		if(!(lien->coude)) {
			segment.dx = lien->dx; segment.dy = lien->dy; 
			FigureTraceDroite(fig,f,&segment,xd,yd);
		} else if(lien->horiz) {
			x = xd; y = yd;
			segment.dx = lien->coude; segment.dy = 0; 
			FigureTraceDroite(fig,f,&segment,x,y); x += segment.dx;
			segment.dx = 0; segment.dy = lien->dy; 
			FigureTraceDroite(fig,f,&segment,x,y); y += segment.dy;
			segment.dx = lien->dx - lien->coude; segment.dy = 0; 
			FigureTraceDroite(fig,f,&segment,x,y);
		} else {
			x = xd; y = yd;
			segment.dx = 0; segment.dy = lien->coude; 
			FigureTraceDroite(fig,f,&segment,x,y); y += segment.dy;
			segment.dx = lien->dx; segment.dy = 0; 
			FigureTraceDroite(fig,f,&segment,x,y); x += segment.dx;
			segment.dx = 0; segment.dy = lien->dy - lien->coude; 
			FigureTraceDroite(fig,f,&segment,x,y);
		}
		if(doit_fermer) WndEndLine(f);
		break;
	  case FIG_ZONE:
		zone = fig->forme.zone;
		if(!fig->p.c) fig->p.c = WndColorGetFromRGB(zone->r,zone->g,zone->b);
		WndPaint(f,xd,yd,zone->larg,zone->haut,fig->p.c);
		break;
	  case FIG_ICONE:
		icone = fig->forme.icone;
		//+ if(plus_fond) WndFillFgnd(f,WND_GC(f,fig->icone),xd,yd,icone->larg,icone->haut);
		WndImageOffset(f,xd,yd); // avant WndIconeInit
		WndIconeInit(f,icone);
		k = 0;
		for(y=0; y<icone->haut; y++) {
			for(x=0; x<icone->larg; x++) WndIconePixel(f,xd+x,yd+icone->haut-1-y,&(icone->pixel[k++]));
		}
		WndImageShow(f);
		break;
	  case FIG_CADRE:
		zone = fig->forme.zone;
		/* WND_SUPPORTS: de _RIEN a _REGLETTES alors qu'il faut ici utiliser WND_RAINURE, etc */
		if(zone->support == WND_RIEN) break;
		else switch(zone->support) {
		  case WND_LIGNES:    forme = WND_RAINURE; break;
		  case WND_PLAQUETTE: forme = 0; break;
		  case WND_CREUX:     forme = 0; break;
		  case WND_RAINURES:  forme = WND_RAINURE; break;
		  case WND_REGLETTES: forme = WND_REGLETTE; break;
		  default: forme = WND_RAINURE;
		}
		doit_fermer = WndBeginLine(f,WND_TEXT,GL_LINES); WndAbsLine(f);
		if(forme) {
			WndRelief(f,2,forme | WND_HORIZONTAL,xd,yd,zone->larg);
			WndRelief(f,2,forme | WND_VERTICAL,xd,yd,zone->haut);
			WndRelief(f,2,forme | WND_VERTICAL,xd+zone->larg,yd,zone->haut);
			WndRelief(f,2,forme | WND_HORIZONTAL,xd,yd+zone->haut,zone->larg);
		} else WndEntoure(f,zone->support,xd,yd,zone->larg,zone->haut);
		if(doit_fermer) WndEndLine(f);
		break;
	}
	if(fig->texte[0] && (fig->type != FIG_LIEN)) {
		WndServer *s; WndContextPtr gc;
		x = 0; y = 0;
		switch(fig->type) {
		  case FIG_DROITE:
			x = (droite->dx - WndColToPix(l)) / 2; if(x < 0) x = 0;
			y = ((droite->dy - WndLigToPix(1)) / 2) + WndLigToPix(1); if(y < 0) y = 0;
			break;
		  case FIG_ZONE: case FIG_CADRE:
			x = (zone->larg - WndColToPix(l)) / 2; if(x < 0) x = 0;
			y = ((zone->haut - WndLigToPix(1)) / 2) + WndLigToPix(1); if(y < 0) y = 0;
			break;
		  case FIG_ICONE:
			x = (icone->larg - WndColToPix(l)) / 2; if(x < 0) x = 0;
			y = ((icone->haut - WndLigToPix(1)) / 2) + WndLigToPix(1); if(y < 0) y = 0;
			break;
          default: break;
		}
		switch(fig->cote_texte) {
		  case FIG_A_DROITE: x = cdr->larg - WndColToPix(l); break;
		  case FIG_EN_DESSOUS: y = cdr->haut /* - WndLigToPix(1) */; break;
		  case FIG_HAUT: y = (3 * WndLigToPix(1)) / 4;
			WndErase(f,x - WndColToPix(1),y - WndLigToPix(1),WndColToPix(l + 2),WndLigToPix(1)); break;
		  case FIG_BAS: y = zone->haut;
			WndErase(f,x - WndColToPix(1),y - WndLigToPix(1),WndColToPix(l + 2),WndLigToPix(1)); break;
          default: break;
		}
		// printf("(%s:%d) %s @%08lX: couleur ecriture #%d\n",__func__,__LINE__,FigTypeNom[fig->type],(Ulong)fig,fig->ecrit);
//		printf("(%s:%d) gc@%08lX\n",__func__,__LINE__,(Ulong)(WND_GC(f,fig->ecrit)));
		s = f->s;
		gc = WndContextCreateFromVal(f,&(s->gc[f->qualite].coul[fig->ecrit]));
		WndString(f,gc,x,y,fig->texte);
		WndContextFree(f,gc);
	}
	/* Transfert dans planches.c (== valable pour tous les cadres inclus) deja tente avec Instrum ... */
	if(cdr->titre[0]) {
		x = (cdr->larg - WndColToPix((int)strlen(cdr->titre))) / 2;
		WndString(f,WND_GC(f,WND_GC_BRDNOIR),x,cdr->haut - 1,cdr->titre); // WND_GC_CLEAR?
	}
	return(1);
}
/* ========================================================================== */
int OpiumRefreshFigure(Cadre cdr) {
	if(cdr) FigureRedessine((Figure)cdr->adrs,0);
	return(1);
}
/* ========================================================================== */
int  OpiumRunFigure(Cadre cdr, WndAction *e) {
	Figure fig; TypeFigFctn fctn; // int rc;

	if(!cdr) return(0);
	fig = (Figure)cdr->adrs;
	// rc = 0; if((fctn = fig->fctn)) rc = fctn(fig,e); return(rc);
	return(fig? (((fctn = fig->fctn))? fctn(fig,e): 0): 0);
}
