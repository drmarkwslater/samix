#ifndef WIN32
#ifndef __MWERKS__
#define STD_UNIX
#endif
#endif

#include <unistd.h>
#include <math.h>     /* pour sqrtf */
#ifdef STD_UNIX
#include <strings.h>  /* pour strlen qui rend size_t si intrinseque */
#else
#include <string.h>
#endif

#include <opium.h>
#include "decode.h"

#define MAX_ZONES 32
typedef struct {
	char *titre;
	int xmin,ymin,xmax,ymax;
	int dx_g,dx_d,dy_h,dy_b;
	WND_SUPPORTS support;
} TypeBoardArea,*BoardArea;
static BoardArea BoardZone[MAX_ZONES];

typedef enum {
	BOARD_REP_OUVRE = 0,  // ouvre le fichier ou le repertoire clique
	BOARD_REP_NOMME       // recupere juste le nom clique
} BOARD_REP_ACTION;

static TypeFigZone BoardIconeFic = { 100, 30, WND_RAINURES, GRF_RGB_GREEN   };
static TypeFigZone BoardIconeRep = { 100, 30, WND_RAINURES, GRF_RGB_ORANGE  };
static char BoardFichierChoisi[MAXFILENAME];

static Cadre BoardRepertoire(char *repert, char qui, char mode);

/* ========================================================================== */
Cadre BoardCreate(void) { return(OpiumCreate(OPIUM_BOARD,0)); }
/* ========================================================================== */
void BoardOnClic(Cadre board, int (*reponse)(Cadre, WndAction *, void *), void *arg) {
	if(!board) return;
	board->a_vide = reponse;
	board->a_vide_arg = arg;
}
/* ========================================================================== */
static void BoardPlaceComposant(Cadre boitier, Cadre precedent) {
	Cadre ref;

	if(!boitier) return;
	OpiumSizeChanged(boitier);
	ref = boitier->ref_pos; if(!ref) ref = precedent;
	if(DEBUG_BOARD(1)) WndPrint("(%s) Ajout prevu %-12s@%08lX (%d, %d) + [%d x %d]\n",
		__func__,boitier->nom,(Ulong)boitier,boitier->defaultx,boitier->defaulty,boitier->larg,boitier->haut);
	if(boitier->defaultx == OPIUM_ADJACENT) {
		if(ref) {
			if(boitier->defaulty < 0) {
				boitier->defaultx = ref->x - boitier->larg - 2 - WndColToPix(1);
				if(ref->support) boitier->defaultx -= 2;
			} else {
				boitier->defaultx = ref->x + ref->larg + WndColToPix(1) + 2;
				if(ref->support) boitier->defaultx += 2;
			}
			boitier->defaulty = ref->y;
		} else {
			boitier->defaultx = 2;
			boitier->defaulty = 2;
		}
	} else if(boitier->defaulty == OPIUM_ADJACENT) {
		if(ref) {
			if(boitier->defaultx < 0) {
				boitier->defaulty = ref->y - boitier->haut - 2 - WndLigToPix(1);
				if(DEBUG_BOARD(1)) WndPrint("(%s) %s %d x %d au dessus de %s (%d, %d), place en (%d, %d)\n",__func__,
					boitier->titre,boitier->larg,boitier->haut,ref->titre,ref->x,ref->y,boitier->defaultx,boitier->defaulty);
				if(ref->support) boitier->defaulty -= 2;
			} else {
				boitier->defaulty = ref->y + ref->haut + 2 + WndLigToPix(1);
				if(ref->support) boitier->defaulty += 2;
			}
			boitier->defaultx = ref->x;
		} else {
			boitier->defaultx = 2;
			boitier->defaulty = 2;
		}
	} else {
		if(boitier->defaultx == OPIUM_AVANT) {
			if(ref) {
				boitier->defaultx = ref->x - boitier->larg - 2 - WndColToPix(1);
				if(ref->support) boitier->defaultx -= 2;
			} else boitier->defaultx = 2;
		} else if(boitier->defaultx == OPIUM_APRES) {
			if(ref) {
				boitier->defaultx = ref->x + ref->larg + WndColToPix(1) + 2;
				if(ref->support) boitier->defaultx += 2;
			} else boitier->defaultx = 2;
		} else if(boitier->defaultx == OPIUM_ALIGNE) {
			if(ref) {
				boitier->defaultx = ref->x;
			} else boitier->defaultx = 2;
		}
		if(boitier->defaulty == OPIUM_AVANT) {
			if(ref) {
				boitier->defaulty = ref->y - boitier->haut - 2 - WndLigToPix(1);
				if(ref->support) boitier->defaulty -= 2;
			} else boitier->defaulty = 2;
		} else if(boitier->defaulty == OPIUM_APRES) {
			if(ref) {
				boitier->defaulty = ref->y + ref->haut + 2 + WndLigToPix(1);
				if(ref->support) boitier->defaulty += 2;
			} else boitier->defaulty = 2;
		} else if(boitier->defaulty == OPIUM_ALIGNE) {
			if(ref) {
				boitier->defaulty = ref->y;
			} else boitier->defaulty = 2;
		}
	}
	if(boitier->defaultx < 2) boitier->defaultx = 2;
	if(boitier->defaulty < 2) boitier->defaulty = 2;
	OpiumPlaceLocale(boitier);
	boitier->defaultx = boitier->x; boitier->defaulty = boitier->y;
}
/* ========================================================================== */
static Cadre BoardLink(Cadre board, Cadre boitier, char mode, int x, int y, Cadre ref) {
	Cadre suivant,dernier;
	int n; int xm,ym;

	if(DEBUG_BOARD(1)) printf("(%s) Ajoute cadre @%08lX a la planche @%08lX\n",__func__,(Ulong)boitier,(Ulong)board);
	if(!board || !boitier) return(0);
	if(board->type != OPIUM_BOARD) return(0);
	/* boitier->suivant = 0 au moment de sa creation, sinon c'est qu'il est deja inclus [quelque part! mais ou? ] */
	if(boitier->suivant) return(0);
	suivant = dernier = (Cadre)board->adrs;
	while(suivant) { dernier = suivant; suivant = dernier->suivant; }
	if(dernier) dernier->suivant = boitier; else board->adrs = (void *)boitier;
	/* boitier->suivant = 0 au moment de sa creation */

	boitier->planche = board;
#ifdef OPENGL
	if(mode) boitier->planche = 0;
	boitier->volant = mode;
#endif
	if(boitier->type != OPIUM_GRAPH) boitier->modexec = board->modexec;  /* pour la recomposition dynamique */
	/* if(x != OPIUM_AUTOPOS) */ boitier->defaultx = x;
	/* if(y != OPIUM_AUTOPOS) */ boitier->defaulty = y;
	boitier->ref_pos = ref;
	if(boitier->type == OPIUM_PANEL) {
		Panel panel; if((panel = (Panel)(boitier->adrs))) panel->mode |= PNL_INBOARD;
		else WndPrint("(%s) !! Adresse du panel du cadre @%08lX (inclus dans la planche '%s' @%08lX) perdue\n",__func__,(Ulong)boitier,board->nom,(Ulong)board);
	}
	if(BoardZoneNb) {
		BoardPlaceComposant(boitier,dernier);
		xm = boitier->x + boitier->larg;
		ym = boitier->y + boitier->haut;
		if(DEBUG_BOARD(1)) printf("(%s) | Cadre @%08lX: (%d, %d) -> (%d, %d)\n",__func__,(Ulong)boitier,boitier->x,boitier->y,xm,ym);
		for(n=0; n<BoardZoneNb; n++) {
			if((BoardZone[n]->xmin < 0) || (boitier->x < BoardZone[n]->xmin)) BoardZone[n]->xmin = boitier->x;
			if((BoardZone[n]->ymin < 0) || (boitier->y < BoardZone[n]->ymin)) BoardZone[n]->ymin = boitier->y;
			if(xm > BoardZone[n]->xmax) BoardZone[n]->xmax = xm;
			if(ym > BoardZone[n]->ymax) BoardZone[n]->ymax = ym;
			if(DEBUG_BOARD(1)) printf("(%s) | Zone[%d]: (%d, %d) -> (%d, %d)\n",__func__,n,BoardZone[n]->xmin,BoardZone[n]->ymin,BoardZone[n]->xmax,BoardZone[n]->ymax);
		}
	}
	
	return(boitier);
}
/* ========================================================================== */
Cadre BoardAdd(Cadre board, Cadre boitier, int x, int y, Cadre ref) {
	return(BoardLink(board,boitier,0,x,y,ref));
}
/* ========================================================================== */
Cadre BoardJoin(Cadre board, Cadre boitier, int x, int y, Cadre ref) {
	return(BoardLink(board,boitier,1,x,y,ref));
}
/* ========================================================================== */
Cadre BoardAddMenu(Cadre board, Menu menu, int x, int y, Cadre ref) {
	return(BoardLink(board,menu->cdr,0,x,y,ref));
}
/* ========================================================================== */
Cadre BoardAddBouton(Cadre board, char *texte, void *fctn, int x, int y, Cadre ref) {
	Menu menu;
	menu = MenuBouton(texte,MNU_FONCTION fctn);
	return(BoardLink(board,menu->cdr,0,x,y,ref));
}
/* ========================================================================== */
Cadre BoardAddPoussoir(Cadre board, char *texte, char *etat, int x, int y, Cadre ref) {
/* Ecrit deux fois le texte, pourquoi?? */
	Menu menu;
	menu = MenuBouton(texte,MNU_BOOLEEN etat);
	return(BoardLink(board,menu->cdr,0,x,y,ref));
}
/* ========================================================================== */
Cadre BoardAddPanel(Cadre board, Panel panel, int x, int y, Cadre ref) {
	return(BoardLink(board,panel->cdr,0,x,y,ref));
}
/* ========================================================================== */
Cadre BoardAddGraph(Cadre board, Graph graph, int x, int y, Cadre ref) {
	return(BoardLink(board,graph->cdr,0,x,y,ref));
}
/* ========================================================================== */
Cadre BoardAddInstrum(Cadre board, Instrum instrum, int x, int y, Cadre ref) {
	return(BoardLink(board,instrum->cdr,0,x,y,ref));
}
/* ========================================================================== */
Cadre BoardAddTerm(Cadre board, Term term, int x, int y, Cadre ref) {
	return(BoardLink(board,term->cdr,0,x,y,ref));
}
/* ========================================================================== */
Cadre BoardAddInfo(Cadre board, Info info, int x, int y, Cadre ref) {
	return(BoardLink(board,info->cdr,0,x,y,ref));
}
/* ========================================================================== */
Cadre BoardAddFigure(Cadre board, Figure fig, int x, int y, Cadre ref) {
//	printf("(%s) ajoute figure '%s' en (%d, %d)\n",__func__,fig->texte,x,y);
	return(BoardLink(board,fig->cdr,0,x,y,ref));
}
/* ========================================================================== */
Cadre BoardAddOnglet(Cadre board, Onglet onglet, int y) {
	return(BoardLink(board,onglet->cdr,0,0,y,0));
}
/* ========================================================================== */
char BoardAreaOpen(char *titre, WND_SUPPORTS support) {
	if(BoardZoneNb < MAX_ZONES) {
		if((BoardZone[BoardZoneNb] = Malloc(1,TypeBoardArea))) {
			BoardZone[BoardZoneNb]->titre = titre;
			BoardZone[BoardZoneNb]->xmin = BoardZone[BoardZoneNb]->ymin = -1;
			BoardZone[BoardZoneNb]->xmax = BoardZone[BoardZoneNb]->ymax = -1;
			BoardZone[BoardZoneNb]->dx_g = BoardZone[BoardZoneNb]->dx_d = 0;
			BoardZone[BoardZoneNb]->dy_h = BoardZone[BoardZoneNb]->dy_b = 0;
			BoardZone[BoardZoneNb]->support = support;
			BoardZoneNb++;
			return(1);
		}
	}
	return(0);
}
/* ========================================================================== */
void BoardAreaMargesHori(int dx_avant, int dx_apres) {
	BoardZone[BoardZoneNb-1]->dx_g = dx_avant; BoardZone[BoardZoneNb-1]->dx_d = dx_apres;
}
/* ========================================================================== */
void BoardAreaMargesVert(int dy_dessus, int dy_dessous) {
	BoardZone[BoardZoneNb-1]->dy_h = dy_dessus; BoardZone[BoardZoneNb-1]->dy_b = dy_dessous;
}
/* ========================================================================== */
void BoardAreaDeplaceBas(TypeFigZone *zone, int dx, int dy) {
	zone->larg += dx; zone->haut += dy;
}
/* ========================================================================== */
void BoardAreaCancel(Cadre board, int *x, int *y) {
	int n; 

	n = BoardZoneNb - 1;
	// if(!strcmp(BoardZone[n]->titre,"Systeme")) printf("(%s) zone %d = [%d .. %d] x [%d .. %d]\n",__func__,n+1,BoardZone[n]->xmin,BoardZone[n]->xmax,BoardZone[n]->ymin,BoardZone[n]->ymax);
	if(BoardZone[n]->dx_g) BoardZone[n]->xmin -= BoardZone[n]->dx_g;
	if(BoardZone[n]->dx_d) BoardZone[n]->xmax += BoardZone[n]->dx_d;
	if(BoardZone[n]->dy_h) BoardZone[n]->ymin -= BoardZone[n]->dy_h;
	if(BoardZone[n]->dy_b) BoardZone[n]->ymax += BoardZone[n]->dy_b;

	*x = BoardZone[n]->xmax; *y = BoardZone[n]->ymax;
	free(BoardZone[n]);
	BoardZoneNb--;
}
/* ========================================================================== */
void BoardAreaCurrentPos(Cadre board, int *xmin, int *ymin, int *xmax, int *ymax) {
	int n;
	n = BoardZoneNb - 1;
	*xmin = (BoardZone[n]->dx_g)? BoardZone[n]->xmin - BoardZone[n]->dx_g: BoardZone[n]->xmin;
	*xmax = (BoardZone[n]->dx_d)? BoardZone[n]->xmax + BoardZone[n]->dx_d: BoardZone[n]->xmax;
	*ymin = (BoardZone[n]->dy_h)? BoardZone[n]->ymin - BoardZone[n]->dy_h: BoardZone[n]->ymin;
	*ymax = (BoardZone[n]->dy_b)? BoardZone[n]->ymax + BoardZone[n]->dy_b: BoardZone[n]->ymax;
}
/* ========================================================================== */
void BoardAreaClose(Cadre board, TypeFigZone *zone, int *x, int *y) {
	int n; Figure fig;
	
	n = BoardZoneNb - 1;
	// if(!strcmp(BoardZone[n]->titre,"Systeme")) printf("(%s) zone %d = [%d .. %d] x [%d .. %d]\n",__func__,n+1,BoardZone[n]->xmin,BoardZone[n]->xmax,BoardZone[n]->ymin,BoardZone[n]->ymax);
	if(BoardZone[n]->dx_g) BoardZone[n]->xmin -= BoardZone[n]->dx_g;
	if(BoardZone[n]->dx_d) BoardZone[n]->xmax += BoardZone[n]->dx_d;
	if(BoardZone[n]->dy_h) BoardZone[n]->ymin -= BoardZone[n]->dy_h;
	if(BoardZone[n]->dy_b) BoardZone[n]->ymax += BoardZone[n]->dy_b;
	zone->larg = (short)(BoardZone[n]->xmax - BoardZone[n]->xmin + (2 * WndColToPix(1)));
	zone->haut = (short)(BoardZone[n]->ymax - BoardZone[n]->ymin + (2 * WndLigToPix(1)));
	zone->support = (char)BoardZone[n]->support;
	zone->r = zone->g = zone->b = 0x8000;
	fig = FigureCreate(FIG_CADRE,(void *)zone,0,0);
	FigureLegende(fig,FIG_HAUT,BoardZone[n]->titre);
	FigureEcrit(fig,(zone->support == WND_PLAQUETTE)? WND_GC_AREANAME: WND_GC_BRDNOIR);
	BoardAddFigure(board,fig,BoardZone[n]->xmin - WndColToPix(1),BoardZone[n]->ymin - WndLigToPix(1) - 1,0);
	
	*x = BoardZone[n]->xmax; *y = BoardZone[n]->ymax;
	free(BoardZone[n]);
	BoardZoneNb--;
}
/* ========================================================================== */
void BoardAreaEnd(Cadre board, int *x, int *y) {
	int n; Figure fig; TypeFigZone *zone;

	n = BoardZoneNb - 1;
	// if(!strcmp(BoardZone[n]->titre,"Systeme")) printf("(%s) zone %d = [%d .. %d] x [%d .. %d]\n",__func__,n+1,BoardZone[n]->xmin,BoardZone[n]->xmax,BoardZone[n]->ymin,BoardZone[n]->ymax);
	if(BoardZone[n]->dx_g) BoardZone[n]->xmin -= BoardZone[n]->dx_g;
	if(BoardZone[n]->dx_d) BoardZone[n]->xmax += BoardZone[n]->dx_d;
	if(BoardZone[n]->dy_h) BoardZone[n]->ymin -= BoardZone[n]->dy_h;
	if(BoardZone[n]->dy_b) BoardZone[n]->ymax += BoardZone[n]->dy_b;
	zone = Malloc(1,TypeFigZone);
	zone->larg = (short)(BoardZone[n]->xmax - BoardZone[n]->xmin + (2 * WndColToPix(1)));
	zone->haut = (short)(BoardZone[n]->ymax - BoardZone[n]->ymin + (2 * WndLigToPix(1)));
	zone->support = (char)BoardZone[n]->support;
	zone->r = zone->g = zone->b = 0x8000;
	fig = FigureCreate(FIG_CADRE,(void *)zone,0,0);
	FigureFormeTempo(fig,1);
	FigureLegende(fig,FIG_HAUT,BoardZone[n]->titre);
	FigureEcrit(fig,(zone->support == WND_PLAQUETTE)? WND_GC_AREANAME: WND_GC_BRDNOIR);
	BoardAddFigure(board,fig,BoardZone[n]->xmin - WndColToPix(1),BoardZone[n]->ymin - WndLigToPix(1) - 1,0);

	*x = BoardZone[n]->xmax; *y = BoardZone[n]->ymax;
	free(BoardZone[n]);
	BoardZoneNb--;
}
/* ========================================================================== */
void BoardBoutonText(Cadre board, char **liste) {
	char *texte; short n;

	if(board) {
		board->bouton = liste;
		n = 0;
		if(board->bouton) do {
			texte = board->bouton[n];
			if(!texte || (texte[0] == '\0')) break;
			n++;
		} forever;
		board->nb_boutons = n;
	}
}
/* ========================================================================== */
void BoardReplace(Cadre board, Cadre avant, Cadre apres) {
	Cadre suivant,dernier;
	
	if(!board || !avant || !apres) return;
	if(board->type != OPIUM_BOARD) return;
	suivant = dernier = (Cadre)board->adrs;
	while(suivant) {
		if(suivant == avant) {
			if(suivant == (Cadre)board->adrs) board->adrs = (void *)apres;
			else dernier->suivant = apres;
			if(apres->type != OPIUM_GRAPH) apres->modexec = board->modexec;  /* pour la recomposition dynamique */
			apres->defaultx = avant->x;
			apres->defaulty = avant->y;
			apres->ref_pos = avant->ref_pos;
			if(apres->type == OPIUM_PANEL) ((Panel)(apres->adrs))->mode |= PNL_INBOARD;
			apres->suivant = avant->suivant; apres->planche = board;
			avant->suivant = 0; avant->planche = 0; 
			if(OpiumDisplayed(avant)) {
				avant->displayed = 0;
				OpiumUse(apres,avant->modexec);
			}
			break;
		}
		dernier = suivant; suivant = dernier->suivant;
	}
}
/* ========================================================================== */
void BoardRemove(Cadre board, Cadre boitier) {
	Cadre suivant,dernier;
	
	if(!board || !boitier ) return;
	if(board->type != OPIUM_BOARD) return;
	suivant = dernier = (Cadre)board->adrs;
	while(suivant) {
		if(suivant == boitier) {
			if(suivant == (Cadre)board->adrs) board->adrs = (void *)suivant->suivant;
			else dernier->suivant = suivant->suivant;
			boitier->planche = 0; boitier->suivant = 0;
			break;
		}
		dernier = suivant; suivant = dernier->suivant;
	}
}
/* ========================================================================== */
void BoardDismount(Cadre board) {
	Cadre suivant,dernier;
	Panel panel;
	
	if(!board) return;
	if(board->type != OPIUM_BOARD) return;
	suivant = dernier = (Cadre)board->adrs;
	while(suivant) {
		dernier = suivant;
		suivant = dernier->suivant;
		dernier->suivant = 0;
		dernier->planche = 0;
		dernier->f = 0;  /* pour pouvoir re-evaluer la position si deplace entre-temps */
		if(dernier->type == OPIUM_PANEL) {
			panel = (Panel)dernier->adrs;
			panel->mode &= ~PNL_INBOARD;
		}
	}
	board->adrs = 0;
}
/* ========================================================================== */
void BoardTrash(Cadre board) {
	Cadre suivant,dernier;
	
	if(!board) return;
	if(board->type != OPIUM_BOARD) return;
	// printf("(%s) --- poubellisation de la planche '%s' @%08lX\n",__func__,board->nom,(Ulong)board);
	suivant = dernier = (Cadre)board->adrs;
	while(suivant) {
		dernier = suivant;
		suivant = dernier->suivant;
		if(DEBUG_BOARD(1) && dernier->f) {
			printf("(%s) Suppression du %s %s @%08lX ->cdr@%08lX ->f@%08lX dans la planche %s\n",__func__,OPIUMCDRTYPE(dernier->type),
			   dernier->nom,(Ulong)(dernier->adrs),(Ulong)dernier,(Ulong)(dernier->f),board->nom);
			printf("(%s) Element actif: %s %s ->cdr@%08lX ->f@%08lX\n",__func__,OPIUMCDRTYPE((board->actif)->type),(board->actif)->nom,(Ulong)(board->actif),(Ulong)((board->actif)->f));
		}
//		if(dernier->f == board->f) dernier->f = 0; /* evite que OpiumDelete(cdr) appelle OpiumClear(cdr) */
		switch(dernier->type) {
			case OPIUM_MENU:    MenuDelete((Menu)(dernier->adrs)); break;
			case OPIUM_PANEL:   PanelDelete((Panel)(dernier->adrs)); break;
			case OPIUM_TERM:    TermDelete((Term)(dernier->adrs)); break;
			case OPIUM_GRAPH:   GraphDelete((Graph)(dernier->adrs)); break;
			case OPIUM_INFO:    InfoDelete((Info)(dernier->adrs)); break;
			case OPIUM_PROMPT:  break;
			case OPIUM_BOARD:   BoardTrash(dernier); break;  /* supprime les composants */
			case OPIUM_INSTRUM: InstrumDelete((Instrum)(dernier->adrs)); break;
			case OPIUM_FIGURE:  FigureDelete((Figure)(dernier->adrs)); break;
			case OPIUM_ONGLET:  OngletDelete((Onglet)(dernier->adrs)); break;
		}
	}
	board->adrs = 0;
	// printf("(%s) --- poubellisation terminee -------------------------\n",__func__);
}
/* ========================================================================== */
Onglet OngletDefini(char **liste, void (*construit)(int)) {
	Cadre cdr; Onglet onglet;
	
	onglet = (Onglet)malloc(sizeof(TypeOnglet));
	if(onglet) {
		cdr = OpiumCreate(OPIUM_ONGLET,onglet);
		if(!cdr) { free(onglet); return(0);  }
		onglet->cdr = cdr;
		onglet->liste = liste;
		onglet->construit = construit;
		onglet->numero = 0;
		onglet->nblig = 1; onglet->nbcol = (short)OngletNb(onglet);
		onglet->selec = 0;
		OpiumSetOptions(cdr);
	}
	return(onglet);
}
/* ========================================================================== */
int OngletNb(Onglet onglet) { 
	int nb; char *texte;
	
	if(!onglet) return(0);
	nb = 0;
	do {
		texte = onglet->liste[nb];
		if(!texte) break; if(*texte == '\0') break;
		nb++;
	} forever;
	return(nb);
}
/* ========================================================================== */
void OngletMaxCol(Onglet onglet, short nbcol) {
	short total;

	if(!onglet || (nbcol <= 0)) return;
	total = (short)OngletNb(onglet);
	onglet->nblig = ((total - 1) / nbcol) + 1;
	onglet->nbcol = ((total - 1) / onglet->nblig) + 1;
}
/* ========================================================================== */
void OngletNbLigs(Onglet onglet, short nblig) {
	if(!onglet || (nblig <= 0)) return;
	onglet->nblig = nblig;
	onglet->nbcol = (((short)OngletNb(onglet) - 1) / onglet->nblig) + 1;
}
/* ========================================================================== */
int OngletRangees(Onglet onglet) { return(onglet? (int)(onglet->nblig): 0); }
/* ========================================================================== */
void OngletDemande(Onglet onglet, int numero) {
	if((numero >= 0) && (numero < OngletNb(onglet))) onglet->numero = numero;
}
/* ========================================================================== */
int OngletCourant(Onglet onglet) { return(onglet? onglet->numero: 0); }
/* ========================================================================== */
void OpiumOngletColorNb(Cadre cdr, Cadre fait, short *nc) {
	Onglet onglet,modele; short ic;

	if(!(onglet = (Onglet)cdr->adrs)) return;
	if(fait) modele = (Onglet)fait->adrs; else modele = 0;
	if(modele) {
		onglet->selec = modele->selec;
	} else {
		ic = *nc;
		onglet->selec = ic++;
		*nc = ic;
	}
}
/* ========================================================================== */
void OpiumOngletColorSet(Cadre cdr, WndFrame f) {
	Onglet onglet;

	if(!(onglet = (Onglet)cdr->adrs)) return;
	WndFenColorSet(f,onglet->selec,WND_GC_MODS); // WND_GC_SELECTED
}
/* ========================================================================== */
int OpiumSizeOnglet(Cadre cdr, char from_wm) {
	Onglet onglet; char *texte; int i,j,k,l,m,nb,max;

	if(!(onglet = (Onglet)cdr->adrs)) return(0);
	
	// if(from_wm) return(1);
	cdr->haut = onglet->nblig * (WndLigToPix(1) + 2);
	max = 0; nb = 1; m = 0;
	nb = OngletNb(onglet); max = 0; j = 0;
	for(i=0; i<nb; i++) {
		if(j == onglet->nbcol) {
			k = j * (max + 2); if(m < k) m = k;
			max = 0; j = 0;
		} else j++;
		texte = onglet->liste[i];
		if(!texte) break; if(*texte == '\0') break;
		if(max < (l = (int)strlen(texte))) max = l;
	}
	cdr->larg = WndColToPix(m);
	cdr->dh = cdr->larg; cdr->dv = cdr->haut;
	
	return(1);
}
/* ========================================================================== */
int OpiumRefreshOnglet(Cadre cdr) {
	WndFrame f; Onglet onglet;
	int i,j,nb,cur; char *texte;
	int larg,x0,x,y;
	
	if(!OpiumCoordFenetre(cdr,&f)) return(0);
	if(!(onglet = (Onglet)cdr->adrs)) return(0);
	
	if(!(nb = OngletNb(onglet))) return(1);
	cur = OngletCourant(onglet);
	larg = ((cdr->larg - 1) / onglet->nbcol) + 1;
	j = 0; x0 = 0; y = 0;
	for(i=0; i<nb; i++) {
		if(j == onglet->nbcol) {
			y += (WndLigToPix(1) + 2);
			x0 = 0; j = 1;
		} else j++;
		texte = onglet->liste[i];
		x = x0 + ((larg - WndColToPix((int)strlen(texte))) / 2);
		if(x < x0) x = x0;
		if(i == cur) WndString(f,WND_GC(f,onglet->selec),x,y + WndLigToPix(1) + 1,texte); // WND_GC_SELECTED
		else {
			WndString(f,WND_CLEAR,x,y + WndLigToPix(1) + 1,texte);
			if(j < onglet->nbcol) WndRelief(f,2,WND_FOND_BAS,x0,y+WndLigToPix(1)+1,larg-2);
			else WndRelief(f,2,WND_FOND_BAS,x0,y+WndLigToPix(1)+1,larg-4);
			if((i - onglet->nbcol) == cur) WndRelief(f,2,WND_FOND_HAUT,x0,y+2,larg-2);
		}
		x0 += larg;
		if((j < onglet->nbcol) && (((i + 1) < nb) || (i != cur))) {
			if((i + 1) == cur) WndRelief(f,2,WND_FOND_DRTE,x0-1,y,WndLigToPix(1));
			else WndRelief(f,2,WND_FOND_GCHE,x0,y,WndLigToPix(1));
		}
	}
	
	return(1);
}
/* ========================================================================== */
int OpiumRunOnglet(Cadre cdr, WndAction *e) {
	Onglet onglet; int larg,j,k;

	if(!(onglet = (Onglet)cdr->adrs)) return(0);
	if(!cdr->larg) return(0);
	larg = ((cdr->larg - 1) / onglet->nbcol) + 1;
	j = e->x / larg; k = e->y / (WndLigToPix(1) + 2);
	onglet->numero = j + (k * onglet->nbcol);
	(*(onglet->construit))(onglet->numero);
	return(0);
}
/* ========================================================================== */
void OngletDelete(Onglet onglet) {
	if(!onglet) return;
	if(onglet->cdr) OpiumDelete(onglet->cdr);
	free(onglet);
}
/* ========================================================================== */
static int BoardRepertoireFichierOuvre(Figure fig, WndAction *e) {
	char *nom; char commande[MAXFILENAME];

	nom = (char *)(fig->adrs);
	if(e->type == WND_RELEASE) {
		if(e->code == WND_MSELEFT) {
			// printf("  | Ouverture du fichier '%s'\n",nom);
			sprintf(commande,"open \"%s\"",nom);
			system(commande);
		} else if(e->code == WND_MSERIGHT) {
			// printf("  | Choix du fichier '%s'\n",nom);
			strncpy(BoardFichierChoisi,nom,MAXFILENAME);
		}
	}
	return(0);
}
/* ========================================================================== */
static int BoardRepertoireFichierNomme(Figure fig, WndAction *e) {
	// printf("  | Choix du fichier '%s'\n",(char *)(fig->adrs));
	strncpy(BoardFichierChoisi,(char *)(fig->adrs),MAXFILENAME);
	return(0);
}
/* ========================================================================== */
static int BoardRepertoireEfface(Cadre board, void *arg) {
	/* dealloc(fig->adrs) */
	Cadre suivant,dernier; Figure fig;
	
	suivant = (Cadre)board->adrs;
	while(suivant) {
		dernier = suivant;
		suivant = dernier->suivant;
		if(dernier->type == OPIUM_FIGURE) {
			fig = (Figure)dernier->adrs;
			free(fig->adrs); /* libere a_ouvrir[n] */
			FigureDelete(fig);
		}
	}
	board->adrs = 0;
	return(1);
}
/* ========================================================================== */
static int BoardRepertoireChange(Figure fig, WndAction *e) {
	char *repert;

	repert = (char *)(fig->adrs);
	if(e->type == WND_RELEASE) {
		if(e->code == WND_MSELEFT) {
			// printf("  | Choix du repertoire '%s'\n",repert);
			strncpy(BoardFichierChoisi,repert,MAXFILENAME);
		} else if(e->code == WND_MSERIGHT) {
			// printf("  | Affichage du repertoire '%s'\n",repert);
			BoardRepertoire(repert,REPERT_ALL,BOARD_REP_OUVRE);
		}
	}
	return(0);
}
/* ========================================================================== */
static Cadre BoardRepertoire(char *repert, char qui, char mode) {
	Cadre board; Figure fig;
	int nb_tot,cote,i,j,n,dy; size_t l;
	char *dessus; char complet[MAXFILENAME];
	int nb_top;
#define MAX_REP 256
	char *liste_rep[MAX_REP+1]; int nb_rep;
#define MAX_FIC 256
	char *liste_fic[MAX_FIC+1]; int nb_fic;
	char *a_ouvrir[1+MAX_REP+MAX_FIC];

	// printf(". Affichage du repertoire %s\n",repert);
	dessus = "..";
	l = strlen(repert) - 1;
	if(repert[l] == '/') strcat(strcpy(complet,repert),dessus);
	else strcat(strcat(strcpy(complet,repert),"/"),dessus);
	// printf("  | Repertoire parent: '%s'\n",complet);
	if((qui == REPERT_REP) || (qui == REPERT_ALL)) RepertoireListeCree(REPERT_REP,repert,liste_rep,MAX_REP,&nb_rep); else nb_rep = 0;
	if((qui == REPERT_FIC) || (qui == REPERT_ALL)) RepertoireListeCree(REPERT_FIC,repert,liste_fic,MAX_FIC,&nb_fic); else nb_fic = 0;
	nb_top = (qui != REPERT_FIC)? 1: 0;
	// printf("  | %d repertoire%s et %d fichier%s\n",Accord1s(nb_rep),Accord1s(nb_fic));
	nb_tot = nb_top + nb_rep + nb_fic;
	cote = (int)sqrtf(/* 1.6 * */ (float)(nb_tot)); // 3/5 c * c = N
	board = BoardCreate();
	OpiumTitle(board,repert);
	dy = 5; j = 0;
	for(n=0; n<nb_tot; n++) {
		i = n - nb_top;
		if(i < 0) {
			/* alloc(nom_complet) */
			a_ouvrir[n] = (char *)malloc(l+1+2+1);
			if(repert[l] == '/') strcat(strcpy(a_ouvrir[n],repert),"..");
			else sprintf(a_ouvrir[n],"%s/%s",repert,"..");
			fig = FigureCreate(FIG_ZONE,(void *)(&BoardIconeRep),(void *)a_ouvrir[n],BoardRepertoireChange);
			FigureLegende(fig,FIG_CENTRE,dessus);
			if(j == cote) { j = 0; dy += (30 + 10); }
			BoardAddFigure(board,fig,15+(j++*110),dy,0);
		} else if(i < nb_rep) {
			// printf("  | Item #%d (repertoire #%d): %s\n",n,i,liste_rep[i]);
			/* alloc(nom_complet) */
			a_ouvrir[n] = (char *)malloc(l+1+strlen(liste_rep[i])+1);
			if(repert[l] == '/') strcat(strcpy(a_ouvrir[n],repert),liste_rep[i]);
			else sprintf(a_ouvrir[n],"%s/%s",repert,liste_rep[i]);
			fig = FigureCreate(FIG_ZONE,(void *)(&BoardIconeRep),(void *)a_ouvrir[n],BoardRepertoireChange);
			FigureLegende(fig,FIG_CENTRE,liste_rep[i]);
			if(j == cote) { j = 0; dy += (30 + 10); }
			BoardAddFigure(board,fig,15+(j++*110),dy,0);
		} else {
			i = i - nb_rep;
			/* alloc(nom_complet) */
			// printf("  | Item #%d (fichier #%d): %s\n",n,i,liste_fic[i]);
			a_ouvrir[n] = (char *)malloc(l+1+strlen(liste_fic[i])+1);
			if(repert[l] == '/') strcat(strcpy(a_ouvrir[n],repert),liste_fic[i]);
			else sprintf(a_ouvrir[n],"%s/%s",repert,liste_fic[i]);
			if(mode == BOARD_REP_NOMME) fig = FigureCreate(FIG_ZONE,(void *)(&BoardIconeFic),(void *)a_ouvrir[n],BoardRepertoireFichierNomme);
			else /* if(mode == BOARD_REP_OUVRE) */ fig = FigureCreate(FIG_ZONE,(void *)(&BoardIconeFic),(void *)a_ouvrir[n],BoardRepertoireFichierOuvre);
			FigureLegende(fig,FIG_CENTRE,liste_fic[i]);
			if(j == cote) { j = 0; dy += (30 + 10); }
			BoardAddFigure(board,fig,15+(j++*110),dy,0);
		}
	}
	OpiumExitFctn(board,BoardRepertoireEfface,0);
	OpiumFork(board);
	return(board);
}
/* ========================================================================== */
Cadre BoardRepertoireOuvre(char *repert, char qui) { return(BoardRepertoire(repert,qui,BOARD_REP_OUVRE)); }
/* ========================================================================== */
char *BoardRepertoireChoisi(char *repert, char qui) {
	Cadre board;

	BoardFichierChoisi[0] = '\0';
	board = BoardRepertoire(repert,qui,BOARD_REP_NOMME);
	do { OpiumUserAction(); usleep(20000); } while(board->adrs && (BoardFichierChoisi[0] == '\0'));
//	if(BoardFichierChoisi[0]) BoardRepertoireEfface(board,0);
	OpiumClear(board);
	
	return(BoardFichierChoisi);
}
/* ========================================================================== */
int BoardRefreshVars(Cadre cdr) {
	Cadre board,boitier; WndFrame f; char doit_terminer;

	f = cdr->f;
	board = cdr;
	if(board->type != OPIUM_BOARD) return(0);
	boitier = (Cadre)(board->adrs);
	if(!(board->actif)) board->actif = boitier;
	doit_terminer = WndRefreshBegin(f);
	while(boitier) {
		if(boitier->type == OPIUM_PANEL)
			PanelRefreshVars((Panel)(boitier->adrs));
		else if(boitier->type == OPIUM_GRAPH)
			OpiumRefreshGraph(boitier);
		else if(boitier->type == OPIUM_INSTRUM)
			InstrumRefreshVar((Instrum)(boitier->adrs));
		boitier = boitier->suivant;
	}
	if(doit_terminer) WndRefreshEnd(f);
	return(1);
}
/* ========================================================================== */
void OpiumBoardColorNb(Cadre cdr, Cadre fait, short *nc) {
	Cadre board,boitier,modele,vu[OPIUM_NBTYPES]; short ic; int i;

	for(i=0; i<OPIUM_NBTYPES; i++) vu[i] = 0;
	board = cdr;
	modele = fait;
	if(modele) {
		board->fond = modele->fond;
	} else {
		ic = *nc;
		boitier = (Cadre)(board->adrs);
		while(boitier) {
			if(!(boitier->volant)) {
				switch(boitier->type) {
				  case OPIUM_MENU:    OpiumMenuColorNb(boitier,vu[boitier->type],&ic); break;
				  case OPIUM_PANEL:   OpiumPanelColorNb(boitier,vu[boitier->type],&ic); break;
				  case OPIUM_TERM:    OpiumTermColorNb(boitier,vu[boitier->type],&ic); break;
				  case OPIUM_GRAPH:   OpiumGraphColorNb(boitier,vu[boitier->type],&ic); break;
				  case OPIUM_INSTRUM: OpiumInstrumColorNb(boitier,vu[boitier->type],&ic); break;
				  case OPIUM_FIGURE:  OpiumFigureColorNb(boitier,vu[boitier->type],&ic); break;
				  default: break;
				}
				if(!(vu[boitier->type])) vu[boitier->type] = boitier;
			}
			boitier = boitier->suivant;
		}
		board->fond = ic++;
		*nc = ic;
	}
}
/* ========================================================================== */
void OpiumBoardColorSet(Cadre cdr, WndFrame f) {
	Cadre board,boitier,vu[OPIUM_NBTYPES]; int i;

	for(i=0; i<OPIUM_NBTYPES; i++) vu[i] = 0;
	board = cdr;
	boitier = (Cadre)(board->adrs);
	while(boitier) {
		if(!(boitier->volant) && !(vu[boitier->type])) {
			switch(boitier->type) {
				case OPIUM_MENU:    OpiumMenuColorSet(boitier,f); WndFenColorSet(f,((Menu)boitier->adrs)->normal,WND_GC_BRDBLANC); break;
				case OPIUM_PANEL:   OpiumPanelColorSet(boitier,f); break;
				case OPIUM_TERM:    OpiumTermColorSet(boitier,f); break;
				case OPIUM_GRAPH:   OpiumGraphColorSet(boitier,f); break;
				case OPIUM_INSTRUM: OpiumInstrumColorSet(boitier,f); break;
				case OPIUM_FIGURE:  OpiumFigureColorSet(boitier,f); break;
				case OPIUM_BOARD:   OpiumBoardColorSet(boitier,f); break;
			}
			vu[boitier->type] = boitier;
		}
		boitier = boitier->suivant;
	}
	WndFenColorSet(f,board->fond,WND_GC_BRDNOIR);
}
/* ========================================================================== */
int OpiumSizeBoard(Cadre cdr, char from_wm) {
/* Remarque: la taille d'une planche depend de la POSITION de ses composants,
             qui, elle, est definie dans OpiumUse... */
	Cadre board,boitier,precedent,cdr_onglets;
	// Menu premier;

	int larg,haut;

	if(from_wm) {
		if(DEBUG_OPIUM(2)) WndPrint("(%s impose) Planche %s %d x %d, affichage %d x %d\n",__func__,cdr->nom,cdr->larg,cdr->haut,cdr->dh,cdr->dv);
		return(1);
	}
	if(DEBUG_OPIUM(1) && (cdr->type != OPIUM_BOARD)) {
		WndPrint("+++ OpiumSizeBoard sur le %s @%08lX\n",cdr->nom,(Ulong)cdr->adrs);
		return(0);
	}
	if(DEBUG_BOARD(1)) WndPrint("(%s) Calcul de la taille de la planche %s\n",__func__,cdr->nom);
	cdr_onglets = 0;
	board = cdr;
	larg = 0; haut = 0; precedent = 0;
	boitier = (Cadre)(board->adrs);
	// premier = 0;
	while(boitier) {
		// if(boitier->type == OPIUM_MENU) { if(!premier) (Menu)boitier->adrs; }
		if(boitier->type == OPIUM_ONGLET) { cdr_onglets = boitier; OpiumSizeChanged(cdr_onglets); }
		else {
			BoardPlaceComposant(boitier,precedent);
			if((boitier->x + boitier->larg) > larg) larg = boitier->x + boitier->larg;
			if((boitier->y + boitier->haut) > haut) haut = boitier->y + boitier->haut;
			if(DEBUG_BOARD(1)) WndPrint("(%s) Ajout final %-12s@%08lX (%d, %d) + [%d x %d] -> dim = %d x %d\n",
				__func__,boitier->nom,(Ulong)boitier,boitier->x,boitier->y,boitier->larg,boitier->haut,larg,haut);
		}
		precedent = boitier;
		boitier = boitier->suivant;
	}

	if(cdr_onglets) {
		haut += cdr_onglets->haut;
		if(cdr_onglets->larg > larg) larg = cdr_onglets->larg;
	}
	cdr->larg = larg + 1 + WndColToPix(1);
	cdr->haut = haut + 1 + WndLigToPix(2);
	cdr->dx = cdr->dy = 0;
	cdr->dh = cdr->larg; cdr->dv = cdr->haut;
	if(cdr_onglets) {
		cdr_onglets->larg = cdr->larg;
		cdr_onglets->x = cdr_onglets->y = cdr_onglets->dx = cdr_onglets->dy = 0;
		cdr_onglets->dh = cdr_onglets->larg; cdr_onglets->dv = cdr_onglets->haut;
	}

	if(DEBUG_BOARD(1)) WndPrint("(%s theorique) Planche %s %d x %d, affichage %d x %d\n",__func__,cdr->nom,cdr->larg,cdr->haut,cdr->dh,cdr->dv);
	if(DEBUG_BOARD(1)) WndPrint("(%s) Taille de la planche calculee\n",__func__);
	// if(premier) printf("(%s) @%08lX: %s.type=%d -> %08lX\n",__func__,(Ulong)premier->items,premier->items[0].texte,premier->items[0].type,(Ulong)(premier->items[0].adrs));
	return(1);
}
/* ========================================================================== */
int OpiumRefreshBoard(Cadre cdr) {
	Cadre board,boitier; Menu menu; WndFrame f,g; char doit_terminer;
	int x,y; char support; // char *utilise;

//	OpiumDebugLevel[OPIUM_DEBUG_OPIUM] = 1;
//	OpiumDebugLevel[OPIUM_DEBUG_PANEL] = 4;
//	OpiumDebugLevel[OPIUM_DEBUG_BOARD] = 1;
	if(!cdr) { if(DEBUG_BOARD(1)) printf("(%s) ----- Affichage d'une planche indefinie\n",__func__); return(0); }
	if(DEBUG_BOARD(1)) printf("(%s) ========== Affichage de %s%s '%s' C=%08llX [%d x %d] @(%d, %d) ==========\n",__func__,
		OpiumLeLa(cdr),OPIUMCDRTYPE(cdr->type),cdr->nom,(IntAdrs)cdr,cdr->dh,cdr->dv,cdr->x,cdr->y);
	//- if(!OpiumCoordFenetre(cdr,&f)) return(0);
	f = cdr->f;
	board = cdr;
	if(board->type != OPIUM_BOARD) return(0);
	x = WndPixToCol(cdr->dx);
	cdr->dx = WndColToPix(x);
	y = WndPixToLig(cdr->dy);
	cdr->dy = WndLigToPix(y);
	if(DEBUG_OPIUM(1)) printf("(%s)       sous-fenetre (%d + %d ; %d + %d)\n",__func__,cdr->dx,cdr->dh,cdr->dy,cdr->dv);
	//+? WndErase(f,0,0,cdr->dh,cdr->dv);

	// OpiumDumpConnexions(__func__,cdr);

	doit_terminer = WndRefreshBegin(f);
//+	WndFillBgnd(f,WND_GC(f,board->fond),0,0,cdr->dh,cdr->dv);
	WndFillBgnd(f,WND_GC(f,board->fond),0,0,cdr->dh+WND_ASCINT_WIDZ,cdr->dv+WND_ASCINT_WIDZ); // BRD_FOND == WND_GC(f,board->fond)
	boitier = (Cadre)(board->adrs);
	if(DEBUG_BOARD(1)) {
		WndPrint("(%s/1) Demarre par %s '%s' C=%08lX\n",__func__,
				 boitier?OPIUMCDRTYPE(boitier->type):"<nul>",boitier?boitier->nom:"",(Ulong)boitier);
	}
	if(!(board->actif)) board->actif = boitier;
	while(boitier) {
		if(DEBUG_BOARD(1)) {
			WndPrint("(%s/2) %s '%s' C=%08lX (suivant: C=%08lX)\n",__func__,
				OPIUMCDRTYPE(boitier->type),boitier->nom,(Ulong)boitier,(Ulong)boitier->suivant);
		}
		if(!boitier->volant) {
			if(DEBUG_BOARD(1)) printf("(%s/3) %s '%s': insertion en (%d + %d ; %d + %d) ",__func__,
				OPIUMCDRTYPE(boitier->type),boitier->nom[0]? boitier->nom: OPIUMCDRTYPE(boitier->type),boitier->x,boitier->dh,boitier->y,boitier->dv);
#define EXCLUSION_STRICTE
// #define EXCLUSION_LARGE
		#ifdef EXCLUSION_STRICTE
			if((boitier->x < cdr->dx) || (boitier->y < cdr->dy)
			|| ((boitier->x + boitier->dh) > (cdr->dx + cdr->dh))
			|| ((boitier->y + boitier->dv) > (cdr->dy + cdr->dv))) {
				if(DEBUG_BOARD(1)) printf("abandonnee (hors cadre)!\n");
				boitier = boitier->suivant; continue;
			}
		#endif
		#ifdef EXCLUSION_LARGE
			if(((boitier->x < cdr->dx) && ((boitier->x + boitier->dh) < cdr->dx)
				&& (boitier->y < cdr->dy) && ((boitier->y + boitier->dv) < cdr->dy))
			|| ((boitier->x > (cdr->dx + cdr->dh)) && ((boitier->x + boitier->dh) > (cdr->dx + cdr->dh))
				&& (boitier->y > (cdr->dy + cdr->dv)) && ((boitier->y + boitier->dv) > (cdr->dy + cdr->dv)))) {
				if(DEBUG_BOARD(1)) printf("abandonnee..\n");
				boitier = boitier->suivant; continue;
			}
		#endif
			if(DEBUG_BOARD(1)) printf("entreprise\n");
			OpiumRefresh(boitier);
			if(DEBUG_BOARD(1)) printf("(%s) %s '%s' C=%08llX inclus dans C=%08llX et %saffiche\n",__func__,
				OPIUMCDRTYPE(boitier->type),boitier->nom,(IntAdrs)boitier,(IntAdrs)board,boitier->displayed?"":"PAS ");
			/* if(boitier->titre[0]) {
				x = (boitier->dh - WndColToPix(strlen(boitier->titre))) / 2;
				WndString(f,f->text[WND_GC_CLEAR],x,boitier->dv - WndLigToPix(1) - 1,boitier->titre);
			} */
			/* if(boitier->titre[0] && OpiumCoordFenetre(cdr,&g)) {
				x = (boitier->larg - WndColToPix(strlen(boitier->titre))) / 2;
				WndString(g,g->text[WND_GC_CLEAR],x,boitier->haut + WndLigToPix(1) + 1,boitier->titre);
			} */
			if(boitier->type == OPIUM_MENU) {
				menu = (Menu)(boitier->adrs);
				if((menu->items[0].type == MNU_CODE_BOOLEEN) && (menu->items[0].adrs)) {
					boitier->support = (*(char *)(menu->items[0].adrs))? WND_RAINURES: WND_PLAQUETTE;
					if(*(char *)(menu->items[0].adrs)) MenuItemAllume(menu,1,0,GRF_RGB_YELLOW); else MenuItemEteint(menu,1,0);
				}
			}
			if((support = boitier->support) != WND_RIEN) {
				WndRepere(f,0,0,cdr->dh,cdr->dv);
				WndEntoure(f,support,boitier->x - cdr->dx,boitier->y - cdr->dy,boitier->dh,boitier->dv);
			}
			if((boitier->type == OPIUM_MENU) && boitier->titre[0]) {
				x = (boitier->dh - WndColToPix((int)strlen(boitier->titre))) / 2;
				WndRepere(f,0,0,cdr->dh,cdr->dv);
				WndString(f,WND_CLEAR,boitier->x + x - cdr->dx,boitier->y + boitier->dv - cdr->dy + WndLigToPix(1) + 1,boitier->titre);
			}
		}
		boitier = boitier->suivant;
	}

	if(cdr->planche) {
		if(cdr->titre[0]) {
			if(!OpiumCoordFenetre(cdr,&g)) return(1);
			x = (cdr->larg - WndColToPix((int)strlen(cdr->titre))) / 2;
			WndString(g,WND_GC(g,WND_GC_CLEAR),x,cdr->haut - WndLigToPix(1) - 1,cdr->titre);
		}
	} else {
		// WndBorders(f); WndRepere(f,-cdr->dx,-cdr->dy,cdr->dh,cdr->dv);
		WndRepere(f,0,0,cdr->dh,cdr->dv);
		WndRelief(f,2,WND_BORD_BAS,0,cdr->dv,cdr->dh);
		WndRelief(f,2,WND_BORD_DRTE,cdr->dh,0,cdr->dv);
		// OpiumAfficheBoutons(cdr,f,1,PNL_CANCEL);
		if(cdr->bouton) OpiumBoutonsTjrs(cdr,f,0,cdr->nb_boutons,cdr->bouton);
		else { cdr->nb_boutons = 1; OpiumBouton(cdr,f,L_("Fermer")); }
	}
	if(DEBUG_OPIUM(1)) WndPrint("(%s) Planche %s %d x %d, affichage %d x %d termine\n",__func__,cdr->nom,cdr->larg,cdr->haut,cdr->dh,cdr->dv);
	if(doit_terminer) WndRefreshEnd(f);
	OpiumDebugLevel[OPIUM_DEBUG_BOARD] = 0;
	OpiumDebugLevel[OPIUM_DEBUG_PANEL] = 0;
	OpiumDebugLevel[OPIUM_DEBUG_OPIUM] = 0;

	return(1);
}
/* ========================================================================== */
int OpiumRunBoard(Cadre cdr, WndAction *e) {
	int lig,larg;
	int code_rendu;
	char return_done;
	int m;
	WndFrame f;
	
	if(DEBUG_OPIUM(1) && (cdr->type != OPIUM_BOARD)) {
		WndPrint("+++ OpiumRunPrompt sur le %s @%08lX\n",cdr->nom,(Ulong)cdr->adrs); return(0);
	}
	lig = WndPixToLig(e->y);
	if(DEBUG_BOARD(1)) WndPrint("Board @%08lX, ligne %d, bouton %d\n",(Ulong)cdr,lig,e->x/(cdr->larg/2));
	code_rendu = 0;
	if(e->type == WND_KEY) {
		size_t l;
		l = strlen(e->texte);
		if(DEBUG_PROMPT(2)) { WndPrint("%04lX ( ",e->code); for(m=0; m<l; m++) WndPrint("%02X ",e->texte[m]); WndPrint(")\n"); }
		if(l == 0) { if(e->code == XK_KP_F4) code_rendu = 1; }
		else {
			return_done = (e->texte[l - 1] == 0x0D);
			/* if(return_done) WndPrint("(OpiumRunBoard) <Return> recu\n"); */
			if(return_done) code_rendu = 1;
		}
	} else {
		if((e->type == WND_RELEASE) && (e->code == WND_MSELEFT) && (lig == (WndPixToLig(cdr->dv) - 1))) {
			if(cdr->nb_boutons) larg = cdr->dh / cdr->nb_boutons; else larg = cdr->dh;
			code_rendu = (e->x / larg) + 1;
		} else if(cdr->a_vide) code_rendu = (*cdr->a_vide)(cdr,e,cdr->a_vide_arg);
		else {
			if(e->type == WND_RELEASE) switch(e->code) {
			  case WND_MSERIGHT:
				cdr->dx = cdr->dy = 0;
				cdr->dh = cdr->larg;
				cdr->dv = cdr->haut;
				OpiumCoordFenetre(cdr,&f);
				f->h = cdr->dh; f->v = cdr->dv; WndResize(f);
				OpiumRefreshBoard(cdr);
				break;
			  default:
				/* autres entrees souris dans un board: pas d'action */
				break;
			}
		}
	}
	
	if(DEBUG_BOARD(1)) WndPrint("(OpiumRunBoard) Code rendu: %d\n",code_rendu);
	return(code_rendu);
}
/* ========================================================================== */
Cadre BoardComposant(Cadre board, int x, int y) {
	Cadre boitier;

	if(board->type != OPIUM_BOARD) return(0);
	boitier = (Cadre)(board->adrs);
	while(boitier) {
		if(DEBUG_BOARD(1)) WndPrint("(BoardComposant) Evt (%d,%d): Boitier examine, %08lX/%12s (%d, %d) + [%d x %d]\n",
			x,y,(Ulong)boitier,boitier->nom,boitier->x,boitier->y,boitier->dh,boitier->dv);
		if((x >= boitier->x) && (y >= boitier->y)
		&& (x < (boitier->x + boitier->dh))
		&& (y < (boitier->y + boitier->dv))) return(boitier);
		if(DEBUG_BOARD(1)) WndPrint("(BoardComposant) Boitier elimine\n");
		boitier = boitier->suivant;
	}
	return(0);
}
/* ========================================================================== */
void BoardDelete(Cadre board) {

	if(board->type != OPIUM_BOARD) return;
//	boitier = board->adrs;
//	while(boitier) {
//		OpiumDelete(boitier); plus subtil: en general, autre free a faire selon type.
//		boitier = boitier->suivant;
//	}
	OpiumDelete(board);
}
