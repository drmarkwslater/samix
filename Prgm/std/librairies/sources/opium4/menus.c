#include <stdlib.h>

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
#include "timeruser.h"
#include "html.h"

#ifdef WIN32
extern WndIdent WndRoot;
#endif

#ifdef CW5
/* ========================================================================== */
static int strncasecmp(char *t1, char *t2, int n) {
	char *p1,*p2,c1,c2;
	int k;

	k = n; p1 = t1; p2 = t2;
	while(k--) {
		c1 = *p1++; c2 = *p2++;
		if(!c1 || !c2) break;
		c1 = toupper(c1); c2 = toupper(c2);
		if(c1 != c2) break;
	}
	if(c1 > c2) return(1);
	else if(c1 < c2) return(-1);
	else return(0);
}
#endif

#ifdef WIN32
/* ========================================================================== */
static int strncasecmp(char *t1, char *t2, int n) {
	char *p1,*p2,c1,c2;
	int k;

	k = n; p1 = t1; p2 = t2;
	while(k--) {
		c1 = *p1++; c2 = *p2++;
		if(!c1 || !c2) break;
		c1 = toupper(c1); c2 = toupper(c2);
		if(c1 != c2) break;
	}
	if(c1 > c2) return(1);
	else if(c1 < c2) return(-1);
	else return(0);
}
#endif
/* ========================================================================== */
static Menu MenuCreation(MenuItem items, char traduit) {
	Cadre cdr; MenuItem item; Menu menu;
	
	menu = (Menu)malloc(sizeof(TypeMenu));
	if(menu) {
		if(DEBUG_MENU(1)) WndPrint("Menu cree @%08lX\n",(Ulong)menu);
		cdr = OpiumCreate(OPIUM_MENU,menu);
		if(!cdr) {
			free(menu); return(0); 
		}
		menu->cdr = cdr;
		menu->items = items;
		menu->loc = menu->larg = 0; menu->maxcols = 0;
		menu->nbligs = 0;
		MenuColumns(menu,1);
		menu->taille_calculee = 0;
		menu->defaut = 0;
		menu->item_pointe = 0;
		menu->curs = 0;
		menu->nbitems = 0;
		if((item = items)) while(item->texte) {
			if(traduit) item->traduit = L_(item->texte);
			else item->traduit = item->texte;
			item->posx = item->posy = 0; item->lngr = (short)WndColToPix((int)strlen(item->traduit));
			menu->nbitems += 1;
			item++;
		}
		menu->maxitems = menu->nbitems;
		menu->normal = menu->hilite = menu->reverse = 0;
		OpiumSetOptions(cdr);
		if(DEBUG_MENU(3)) WndPrint("Utilise cdr @%08lX\n",(Ulong)menu->cdr);
	}
	
	return(menu);
}
/* ========================================================================== */
Menu MenuCreate(MenuItem items) {
	return(MenuCreation(items,0));
}
/* ========================================================================== */
Menu MenuLocalise(MenuItem items) {
	return(MenuCreation(items,1));
}
/* ========================================================================== */
void MenuRelocalise(Menu m) {
	int i; MenuItem item;

	if(!m) return;
	item = m->items;
	for(i=0; i<m->maxitems; i++, item++) item->traduit = L_(item->texte);
	if(m->cdr) {
		char titre[OPIUM_MAXTITRE];
		strncpy(titre,(m->cdr)->titre,OPIUM_MAXTITRE);
		strncpy((m->cdr)->titre,L_(titre),OPIUM_MAXTITRE);
		(m->cdr)->titre[OPIUM_MAXTITRE - 1] = '\0';
	}
}
/* ========================================================================== */
void MenuTitle(Menu m, char *texte) { OpiumTitle(m->cdr,texte); }
/* ========================================================================== */
Menu MenuIntitule(MenuItem items, char *texte) {
	Menu m;

#ifdef MENU_BARRE_WIN32
	SetWindowText(WndRoot, texte);
#endif
	if((m = MenuCreation(items,1))) OpiumTitle(m->cdr,L_(texte));

	return(m);
}
/* ========================================================================== */
Menu MenuTitled(char *texte, MenuItem items) {
	Menu m;
	
#ifdef MENU_BARRE_WIN32
	SetWindowText(WndRoot, texte);
#endif
	if((m = MenuCreate(items))) OpiumTitle(m->cdr,texte);
	
	return(m);
}
/* ========================================================================== */
void MenuColumns(Menu m, int nbcols) {
	if(m) {
		if(nbcols > m->maxcols) {
			if(m->loc) free(m->loc); if(m->larg) free(m->larg);
			m->maxcols = 0;
			m->loc = (int *)malloc(nbcols * sizeof(int));
			if(m->loc) {
				m->larg = (int *)malloc(nbcols * sizeof(int));
				if(m->larg) m->maxcols = nbcols;
				else free(m->loc);
			}
		}
		m->nbcols = (nbcols <= m->maxcols)? nbcols: m->maxcols;
	}
}
/* ========================================================================== */
int MenuItemNum(Menu m, MenuItem item_cherche) {
	int i; MenuItem item;

	item = m->items;
	for(i=0; i<m->maxitems; i++, item++) if(item_cherche == item) break;
	if(i == m->maxitems) return(0); else return(i + 1);
}
/* ========================================================================== */
int MenuItemNb(Menu m) { return(m? m->nbitems: 0); }
/* ========================================================================== */
int MenuLargeurPix(Menu m) {
	OpiumSizeMenu(m->cdr,0);
	return((m->cdr)->larg);
}
/* ========================================================================== */
int MenuItemGetIndex(Menu m, IntAdrs num) {
	int i; MenuItem item;

	if(!m) return(-1);
	if(!num) return(-1);
	if((num < 1) || (num > m->maxitems)) {
		item = m->items;
		for(i=0; i<m->maxitems; i++, item++) if(!strcmp(item->texte,(char *)num)) break;
		if(i == m->maxitems) return(-1);
	} else i = (int)(num - 1);
	return(i);
}
/* ========================================================================== */
char *MenuItemGetText(Menu m, IntAdrs num) {
	int i;

	if((i = MenuItemGetIndex(m,num)) >= 0) return(m->items[i].texte);
	else return("");
}
/* ========================================================================== */
void MenuItemSetText(Menu m, IntAdrs num, char *nouveau) {
	int i;

	if((i = MenuItemGetIndex(m,num)) >= 0) {
		m->items[i].texte = nouveau;
		m->items[i].traduit = m->items[i].texte;
	}
}
/* ========================================================================== */
char *MenuItemGetLocalise(Menu m, IntAdrs num) {
	int i;
	
	if((i = MenuItemGetIndex(m,num)) >= 0) return(m->items[i].traduit);
	else return("");
}
/* ========================================================================== */
void MenuItemSetLocalise(Menu m, IntAdrs num, char *nouveau) {
	int i;
	
	if((i = MenuItemGetIndex(m,num)) >= 0) {
		m->items[i].texte = nouveau;
		m->items[i].traduit = L_(m->items[i].texte);
	}
}
/* ========================================================================== */
WndContextPtr MenuItemGetContext(Menu m, IntAdrs num) {
	int i;

	if((i = MenuItemGetIndex(m,num)) >= 0) return(m->items[i].gc);
	else return(0);
}
/* ========================================================================== */
void MenuItemSetContext(Menu m, IntAdrs num, WndContextPtr gc) {
	int i;

	if((i = MenuItemGetIndex(m,num)) >= 0) m->items[i].gc = gc;
}
/* ========================================================================== */
void MenuItemDefault(Menu m, IntAdrs num) {
	int i;

	if((i = MenuItemGetIndex(m,num)) >= 0) m->defaut = i + 1;
}
/* ========================================================================== */
void MenuItemEnable(Menu m, IntAdrs num) {
	int i;

	if((i = MenuItemGetIndex(m,num)) >= 0) m->items[i].sel = MNU_ITEM_OK;
}
/* ========================================================================== */
void MenuItemDisable(Menu m, IntAdrs num) {
	int i;

	if((i = MenuItemGetIndex(m,num)) >= 0) m->items[i].sel = MNU_ITEM_HS;
}
/* ========================================================================== */
void MenuItemHide(Menu m, IntAdrs num) {
	int i;
	
	if((i = MenuItemGetIndex(m,num)) >= 0) m->items[i].sel = MNU_ITEM_CACHE;
}
/* ========================================================================== */
void MenuItemSelect(Menu m, IntAdrs num, char *nouveau, char sel) {
	int i; char doit_terminer;
	
	if((i = MenuItemGetIndex(m,num)) >= 0) {
		if(nouveau) MenuItemSetText(m,num,nouveau);
		m->items[i].sel = sel;
		if(OpiumAlEcran(m)) {
			doit_terminer = WndRefreshBegin((m->cdr)->f);
			OpiumRefresh(m->cdr);
			if(doit_terminer) WndRefreshEnd((m->cdr)->f);
		}
	}
}
/* ========================================================================== */
void MenuItemAllume(Menu menu, IntAdrs num, char *nouveau, 
	WndColorLevel r, WndColorLevel g, WndColorLevel b) {
	Cadre cdr; WndContextPtr gc; char doit_terminer;

	if(!menu) return;
	cdr = menu->cdr;
	gc = MenuItemGetContext(menu,num);
	// PRINT_GC(gc);
	if(!gc) {
		OpiumDrawingRGB(cdr,&gc,0,0,0);
		MenuItemSetContext(menu,num,gc);
	}
	if(gc && cdr) WndContextBgndRGB(cdr->f,gc,r,g,b);
	// PRINT_GC(gc); PRINT_COLOR(gc->background); PRINT_COLOR(gc->foreground);
	if(nouveau) MenuItemSetText(menu,num,nouveau);
	if(OpiumDisplayed(cdr)) {
		doit_terminer = WndRefreshBegin(cdr->f);
		OpiumRefresh(cdr);
		if(doit_terminer) WndRefreshEnd(cdr->f);
	}
}
/* ========================================================================== */
void MenuItemEteint(Menu menu, IntAdrs num, char *nouveau) {
	Cadre cdr; WndContextPtr gc; char doit_terminer;
	
	if(!menu) return;
	cdr = menu->cdr;
	if((gc = MenuItemGetContext(menu,num))) {
		if(cdr) WndContextFree(cdr->f,gc);
		MenuItemSetContext(menu,num,0);
	}
	if(nouveau) MenuItemSetText(menu,num,nouveau);
	if(OpiumDisplayed(cdr)) {
		doit_terminer = WndRefreshBegin(cdr->f);
		OpiumRefresh(cdr);
		if(doit_terminer) WndRefreshEnd(cdr->f);
	}
}
/* ========================================================================== */
char MenuItemArray(Menu menu, int max) {
	MenuItem items;

	if(!menu || (max == 0)) return(0);
	items = (MenuItem)malloc((max + 1) * sizeof(TypeMenuItem));
	if(!items) return(0);
	menu->items = items;
	menu->maxitems = max;
	items->texte = 0;
	return(1);
}
/* ========================================================================== */
int MenuItemAdd(Menu menu, char *texte, char *traduit, short posx, short posy, short lngr,
	WndContextPtr gc, char sel, char aff, char key, char type, void *adrs) {
	MenuItem item;

	if(!menu) return(0);
	if(!(menu->items)) return(0);
	if(menu->nbitems >= menu->maxitems) return(0);
	item = menu->items + menu->nbitems;
	item->texte = texte;
	item->traduit = item->texte;
	item->posx = posx;
	item->posy = posy;
	item->lngr = lngr;
	item->gc = gc;
	item->sel = sel;
	item->aff = aff;
	item->key = key;
	item->type = type;
	item->adrs = adrs;
	item++; item->texte = 0;
	menu->nbitems += 1;
	return(menu->nbitems);
}
/* ========================================================================== */
int MenuItemReplace(Menu menu, char *texte_origine, char *texte_nouveau,
	short posx, short posy, short lngr,
	WndContextPtr gc, char sel, char key, char type, void *adrs) {
	MenuItem item; int i;
	
	if(!menu) return(0);
	if((i = MenuItemGetIndex(menu,(IntAdrs)texte_origine)) < 0) return(0);// menu->maxitems??
	item = menu->items + i;
	item->texte = texte_nouveau;
	item->traduit = item->texte;
	item->posx = posx;
	item->posy = posy;
	item->lngr = lngr;
	item->gc = gc;
	item->sel = sel;
	item->key = key;
	item->type = type;
	item->adrs = adrs;
	return(i+1); // menu->nbitems??
}
/* ========================================================================== */
int MenuItemCopy(Menu menu, MenuItem included) {
	MenuItem item;

	if(!menu) return(0);
	if(!(menu->items)) return(0);
	if(menu->nbitems >= menu->maxitems) return(0);
	item = menu->items + menu->nbitems;
	item->texte = included->texte;
	if(included->traduit) item->traduit = included->traduit;
	else item->traduit = item->texte;
	item->posx = included->posx;
	item->posy = included->posy;
	item->lngr = included->lngr;
	item->gc = included->gc;
	item->sel = included->sel;
	item->key = included->key;
	item->type = included->type;
	item->adrs = included->adrs;
	item++; item->texte = 0;
	menu->nbitems += 1;
	return(menu->nbitems);
}
/* ========================================================================== */
void MenuSupport(Menu menu, char support) {
	if(menu) { if(menu->cdr) (menu->cdr)->support = support; }
}
/* ========================================================================== */
Menu MenuBouton(char *texte, char *traduit, short posx, short posy, short lngr,
	WndContextPtr gc, char sel, char aff, char key, char type, void *adrs) {
	Menu menu;
	
	menu = MenuCreate(0);
	if(MenuItemArray(menu,1)) {
		MenuItemAdd(menu,texte,traduit,posx,posy,lngr,gc,sel,aff,key,type,adrs);
		MenuSupport(menu,WND_PLAQUETTE);
	}
	return(menu);
}
/* ========================================================================== */
void MenuItemDelete(Menu menu) {
	if(menu) {
		if(menu->items) {
			int i;
			for(i=0; i<menu->maxitems; i++) WndContextFree((menu->cdr)->f,menu->items[i].gc);
			free(menu->items); menu->items = 0;
		}
		menu->nbitems = menu->maxitems = 0;
	}
}
/* ========================================================================== */
void MenuDelete(Menu m) {
	if(!m) return;
	if(m->cdr) {
		int i; for(i=0; i<m->maxitems; i++) {
			// PRINT_GC(m->items[i].gc);
			WndContextFree((m->cdr)->f,m->items[i].gc); m->items[i].gc = 0;
			// PRINT_GC(m->items[i].gc);
		}
		OpiumDelete(m->cdr);
	}
	free(m);
}
/* ========================================================================== */
void OpiumMenuColorNb(Cadre cdr, Cadre fait, short *nc) {
	Menu menu,modele; short ic;

	if(!(menu = (Menu)cdr->adrs)) return;
	if(fait) modele = (Menu)fait->adrs; else modele = 0;
	if(modele) {
		menu->normal = modele->normal; menu->hilite = modele->hilite; menu->reverse = modele->reverse;
	} else {
		ic = *nc;
		menu->normal = ic++; menu->hilite = ic++; menu->reverse = ic++;
		*nc = ic;
	}
}
/* ========================================================================== */
void OpiumMenuColorSet(Cadre cdr, WndFrame f) {
	Menu menu;

	if(!(menu = (Menu)cdr->adrs)) return;
	WndFenColorSet(f,menu->normal,WND_GC_TEXT);
	WndFenColorSet(f,menu->hilite,WND_GC_HILITE);
	WndFenColorSet(f,menu->reverse,WND_GC_REVERSE);
}
#define INTERCOL 1
/* ========================================================================== */
int OpiumSizeMenu(Cadre cdr, char from_wm) {
	MenuItem item; Menu menu;
	int k,l,n,nbcols,maxi,larg,total;

	if(from_wm) {
#ifdef OPIUM_AJUSTE_WND
		larg = WndPixToCol(cdr->dh);
		cdr->dh = WndColToPix(larg);
		haut = WndPixToLig(cdr->dv);
		cdr->dv = WndLigToPix(haut);
#endif
		return(1);
	}
	if(DEBUG_OPIUM(1) && (cdr->type != OPIUM_MENU)) {
		WndPrint("!! Erreur systeme: %s(%s @%08lX)\n",__func__,cdr->nom,(Ulong)cdr->adrs);
		return(0);
	}
	menu = (Menu)cdr->adrs;
	if(!menu) return(0);
	menu->nbitems = 0;
	item = menu->items; if(!item) return(0);
	while(item->texte) {
		if(item->sel != MNU_ITEM_CACHE) menu->nbitems++;
		item++;
	}
//	printf("(%s) ============= %d colonne%s maxi, loc@%08X, larg@%08X\n",__func__,
//		Accord1s(menu->maxcols),(hexa)(menu->loc),(hexa)(menu->larg));
//	printf("(%s) %d item%s\n",__func__,Accord1s(menu->nbitems));
	if(menu->nbitems == 0) {
		menu->nbligs = 0;
		cdr->haut = cdr->larg =0;
		menu->taille_calculee = 0;
		printf("(%s) Menu vide!\n",__func__);
		cdr->dh = cdr->larg; cdr->dv = cdr->haut;
		return(1);
	}
//	printf("(%s) %d colonne%s\n",__func__,Accord1s(menu->nbcols));
	if(menu->nbcols) nbcols = menu->nbcols;
	else {
		maxi = WndPixToLig(OpiumServerHeigth(0)) - 4;
		nbcols = ((menu->nbitems - 1) / maxi) + 1;
	}
	if(nbcols < 1) nbcols = 1;
	menu->nbligs = ((menu->nbitems - 1) / nbcols) + 1;
//	printf("(%s) %d ligne%s\n",__func__,Accord1s(menu->nbligs));

	if(menu->loc) { for(k=0; k<menu->nbcols; k++) menu->loc[k] = 0; }
	if(menu->larg) { for(k=0; k<menu->nbcols; k++) menu->larg[k] = 0; }
	larg = 0;
	n = 0;
//	printf("(%s) calcul des largeurs\n",__func__);
	item = menu->items; while(item->texte) {
		if(item->sel != MNU_ITEM_CACHE) {
			k = n / menu->nbligs;
			if(item->traduit) l = (int)strlen(item->traduit); else l = (int)strlen(item->texte);
//			printf("(%s) item#%d: '%s'[%d], colonne %d\n",__func__,n+1,item->traduit,l,k);
			if(menu->larg) { if(menu->larg[k] < l) menu->larg[k] = l; }
			if(larg < l) larg = l;
			n++;
		}
		item++;
	}
	if(menu->larg) {
		total = 0; for(k=0; k<menu->nbcols; k++) {
			if(menu->loc) menu->loc[k] = total;
			total += (menu->larg[k] + INTERCOL);
		}
	} else total = menu->nbcols * (larg + INTERCOL);
	cdr->haut = WndLigToPix(menu->nbligs);
	cdr->larg = WndColToPix(total - INTERCOL);
	menu->taille_calculee = larg;
//	printf("(%s) ............. total=%d, taille_calculee=%d: menu %d x %d\n",__func__,total,menu->taille_calculee,cdr->larg,cdr->haut);
	cdr->dh = cdr->larg; cdr->dv = cdr->haut;

	return(1);
}
// #define ITEM_TEXTE_CONSTANT
/* ========================================================================== */
int OpiumRefreshMenu(Cadre cdr) {
	Menu menu; MenuItem item;
	WndFrame f; WndContextPtr gc; WndServer *s;
	int ligitem,colitem,colprec,lig,col,haut,numitem,h,l,d,marge,depart,demicol,n; char aff;
	char doit_terminer;
//	int x,y,v;

	if(DEBUG_OPIUM(1) && (cdr->type != OPIUM_MENU)) {
		WndPrint("+++ %s sur le %s @%08lX\n",__func__,cdr->nom,(Ulong)cdr->adrs);
		return(0);
	}
	if(!OpiumCoordFenetre(cdr,&f)) return(0);
	
	menu = (Menu)cdr->adrs; if(!menu) return(0);
	item = menu->items; if(!item) return(0);
	if(menu->nbligs == 0) {
		if(!menu->taille_calculee) WndPrint("(%s) Le menu %s n'a pas ete evalue\n",__func__,cdr->nom);
		return(0);
	}
	s = f->s;
	while(item->texte) { item->aff = -1; item++; }
	haut =s->lig; demicol = s->col / 2; d = s->lig - s->decal;
	aff = 0; colprec = 0;
	if(!menu->larg) h = menu->taille_calculee;
	doit_terminer = WndRefreshBegin(f);
	numitem = cdr->ligmin; item = menu->items + numitem;
	while(item->texte) {
		int x,y;
		if(item->sel == MNU_ITEM_CACHE) { item->aff = -1; goto suivant; }
		colitem = aff / menu->nbligs;
		ligitem = aff - (colitem * menu->nbligs);
		if(ligitem >= cdr->lignb) goto suivant; // continue;
		lig = ligitem;
		if(menu->larg) h = menu->larg[colitem];
		if(menu->loc) col = menu->loc[colitem]; else { col = colprec; colprec += (h + 2); }
		col -= cdr->colmin;
//		printf("(%s) item#%d: '%s'[larg[%d]=%d] en (%d, %d)\n",__func__,aff+1,item->traduit,colitem,h,col,lig);
		if(col >= cdr->colnb) break;
		else if((col + h) < 0 /* cdr->colmin */ ) goto suivant; // continue;
		WndTextToPix(f,lig,col,&x,&y);
		item->aff = aff++;
		item->posx = (short)x; item->posy = (short)y; item->lngr = (short)WndColToPix(h);
		// if(colitem < (menu->nbcols - 1)) item->lngr += demicol;

		if(item->traduit) l = (int)strlen(item->traduit); else l = 0;
		if(item->type == MNU_CODE_BOOLEEN) {
			char *b;
			b = (char *)item->adrs;
			if(b) {
				if(*b) {
					if(!(gc = item->gc)) {
						OpiumDrawingRGB(cdr,&gc,0,0,0);
						MenuItemSetContext(menu,numitem+1,gc);
					}
					if(gc) WndContextBgndRGB(f,gc,GRF_RGB_YELLOW);
				} else {
					if((gc = item->gc)) {
						WndContextFree(f,gc);
						MenuItemSetContext(menu,numitem+1,0);
					}
				}
			}
		}
		if(!(gc = item->gc)) gc = MNU_NORMAL;
		if(cdr->planche && (cdr->support != WND_RIEN)) {
		#ifdef ITEM_TEXTE_CONSTANT
			if(ligitem) WndRelief(f,2,WND_TOT | WND_RAINURE | WND_HORIZONTAL,item->posx,item->posy-1,item->lngr);
			if(colitem) WndRelief(f,2,WND_TOT | WND_RAINURE | WND_VERTICAL,item->posx-demicol,item->posy-1,haut);
		#else
			if(ligitem) WndRelief(f,2,WND_RAINURE | WND_HORIZONTAL,WndColToPix(col),WndLigToPix(lig)-1,item->lngr);
			if(colitem) WndRelief(f,2,WND_RAINURE | WND_VERTICAL,WndColToPix(col)-demicol,WndLigToPix(lig)-1,haut);
		#endif
		} else WndClearString(f,gc,item->posx,item->posy,item->lngr);
		if(item->type == MNU_CODE_SEPARE) {
			if(cdr->planche) {
				if(l) {
					marge = (h - l) / 2;
					depart = marge + col;
					if(depart >= 0) WndWrite(f,gc,lig,depart,item->traduit);
					else if(-depart < l) WndWrite(f,gc,lig,0,item->traduit-depart);
				}
			} else if(l) {
				marge = (h - l) / 2;
				depart = marge + col;
				if(marge > 0) {
					if(col >= 0) WndHBar(f,gc,lig,col,marge);
					else if(-col < marge) WndHBar(f,gc,lig,0,marge + col);
				}
				if(depart >= 0) WndWrite(f,gc,lig,depart,item->traduit);
				else if(-depart < l) WndWrite(f,gc,lig,0,item->traduit-depart);
				n = h - (marge + l);
				if(n > 0) {
					depart = marge + l + col;
					if((depart + n) < cdr->colnb) WndHBar(f,gc,lig,depart,n);
					else WndHBar(f,gc,lig,depart,cdr->colnb-depart);
				}
			} else {
				if(col >= 0) WndHBar(f,gc,lig,col,h);
				else if(-col < h) WndHBar(f,gc,lig,0,h+col);
			}
		} else if(menu->defaut == (numitem + 1)) {
		#ifdef ITEM_TEXTE_CONSTANT
			if(col >= 0) WndWrite(f,MNU_HILITE,lig,col,item->traduit);
			else if(-col < l) WndWrite(f,MNU_HILITE,lig,0,item->traduit-col);
		#else
			if(col >= 0) WndDrawString(f,MNU_HILITE,x,y+d,item->traduit,strlen(item->traduit));
			else if(-col < l) WndWrite(f,MNU_HILITE,lig,0,item->traduit-col);
		#endif
		} else if(item->sel) {
		#ifdef ITEM_TEXTE_CONSTANT
			if(col >= 0) WndWrite(f,gc,lig,col,item->traduit);
			else if(-col < l) WndWrite(f,gc,lig,0,item->traduit-col);
		#else
			if(col >= 0) WndDrawString(f,gc,x,y+d,item->traduit,strlen(item->traduit));
			else if(-col < l) WndWrite(f,gc,lig,0,item->traduit-col);
		#endif
		}
	suivant:
		numitem++; item++;
	}
#ifdef SUPERFLU
	if(cdr->planche && (cdr->support != WND_RIEN)) {
		if((aff == 1) && (menu->items[0].type == MNU_CODE_BOOLEEN) && (menu->items[0].adrs)) {
			if(*(char *)(menu->items[0].adrs)) WndEntoure(f,WND_RAINURES,0,0,cdr->larg,cdr->haut);
			else WndEntoure(f,WND_PLAQUETTE,0,0,cdr->larg,cdr->haut);
		} else WndEntoure(f,cdr->support,0,0,cdr->larg,cdr->haut);
	}
#endif
	if(doit_terminer) WndRefreshEnd(f);
	return(1);
}
/* ========================================================================== */
static void MenuPointe(Menu menu, int i, WndFrame f) {
	MenuItem item; int num; int x,y,l,d;

	if((i < 1) || (i > menu->nbitems)) return;
	num = i - 1;
	item = menu->items + num;
	d = (f->s)->lig - (f->s)->decal;
#ifdef ITEM_TEXTE_CONSTANT
	int lig,col,aff;
	// col = num / menu->nbligs; lig = num - (col * menu->nbligs);
	aff = item->aff;
	col = aff / menu->nbligs;
	lig = aff - (col * menu->nbligs);
	col *= menu->taille_calculee;
	//++ WndTextToPix(f,lig,col,&x,&y,&l);
	l = WndPixToCol(item->lngr) - 1;
	if(i == menu->item_pointe) {
		if(i == 0) return;
		if(menu->defaut == menu->item_pointe) {
			WndClearText(f,MNU_HILITE,lig,col,l); // menu->taille_calculee-1
			WndWrite(f,MNU_HILITE,lig,col,item->traduit);
		} else {
			WndClearText(f,MNU_NORMAL,lig,col,l); // menu->taille_calculee-1
			WndWrite(f,MNU_NORMAL,lig,col,item->traduit);
		}
		menu->item_pointe = 0;
	} else {
		WndClearText(f,MNU_REVERSE,lig,col,l); // menu->taille_calculee-1
		WndWrite(f,MNU_REVERSE,lig,col,item->traduit);
		menu->item_pointe = i;
	}
#else
	x = item->posx; y = item->posy; // + ((f->s)->lig / 2);
	l = item->lngr;
	if(i == menu->item_pointe) {
		if(i == 0) return;
		if(menu->defaut == menu->item_pointe) {
			WndClearString(f,MNU_HILITE,x,y,l);
			WndDrawString(f,MNU_HILITE,x,y+d,item->traduit,strlen(item->traduit));
		} else {
			WndClearString(f,MNU_NORMAL,x,y,l);
			WndDrawString(f,MNU_NORMAL,x,y+d,item->traduit,strlen(item->traduit));
		}
		menu->item_pointe = 0;
	} else {
		WndClearString(f,MNU_REVERSE,x,y,l);
		WndDrawString(f,MNU_REVERSE,x,y+d,item->traduit,strlen(item->traduit));
		menu->item_pointe = i;
	}
#endif

}
/* ========================================================================== */
int OpiumRunMenu(Cadre cdr, WndAction *e, int *cdr_ouverts) {
	MenuItem item; Menu menu; Panel panel; Cadre cadre; WndFrame f;
	int lig,col; int defaut,aff_pointe,item_pointe,item_choisi,code_rendu;
	char eol,doit_terminer;
	int k,l,m,n; IntAdrs v; char *b;

	if(DEBUG_OPIUM(1) && (cdr->type != OPIUM_MENU)) {
		WndPrint("(%s) %s @%12llX\n",__func__,cdr->nom,(IntAdrs)cdr->adrs);
		return(0);
	}
	if(cdr->modexec == OPIUM_DISPLAY) return(0);
	if(!OpiumCoordFenetre(cdr,&f)) return(0);
	menu = (Menu)cdr->adrs;
	if(!menu->items) return(0);
//1	cdr->ligmin = (cdr->dy)? WndPixToLig(cdr->dy - 1) + 1: 0;
//1	cdr->colmin = (cdr->dx)? WndPixToCol(cdr->dx - 1) + 1: 0;
//2	WndPixToText(f,e->x,e->y,&lig,&col);
	lig = WndPixToLig(e->y) + cdr->ligmin;
	col = WndPixToCol(e->x) + cdr->colmin;
//	printf("(%s) evt en %d x %d, soit caractere pointe en col=%d, lig=%d\n",__func__,e->x,e->y,col,lig);

#ifdef WIN32_DEBUG	
	{	char tmp[80];
		sprintf(tmp, "lig:%d col:%d", lig, col);
		OpiumWin32Log(tmp);
	}
#endif

#ifdef ANCIEN
	// item_pointe = ((col / menu->taille_calculee) * menu->nbligs) + lig + 1;
	if(menu->nbligs > 1) {
		aff_pointe = ((col / menu->taille_calculee) * menu->nbligs) + lig;
		item_pointe = 0; while(menu->items[item_pointe].texte && (menu->items[item_pointe].aff != aff_pointe)) item_pointe++;
	} else {
		int xevt,xitem,xsvt;
		xevt = e->x + WndColToPix(cdr->colmin);
		item = menu->items; if(!item) return(0);
		item_pointe = 0; xitem = 0; xsvt = xitem + item->lngr;
		while(item->texte) {
			if(item->sel == MNU_ITEM_CACHE) { item->aff = -1; goto suivant; }
			xsvt = xitem + item->lngr;
			if(xevt < xsvt) break;
			xitem = xsvt;
		suivant:
			item_pointe++; item++;
		}
		if(item->texte == 0) return(0);
	}
#else  /* ANCIEN */
	if(menu->loc && menu->larg) for(k=0; k<menu->nbcols; k++) {
		if((col >= menu->loc[k]) && (col < (menu->loc[k] + menu->larg[k]))) break;
	} else k = col / menu->taille_calculee;
	aff_pointe = (k * menu->nbligs) + lig;
	item_pointe = 0; while(menu->items[item_pointe].texte && (menu->items[item_pointe].aff != aff_pointe)) item_pointe++;
#endif /* ANCIEN */

	item = menu->items + item_pointe;
#ifdef DEBUG_RUNMENU
	printf("(%s.1) item pointe @%08llX: %s.type=%d (%s) -> adrs=%08llX\n",__func__,(IntAdrs)item,item->texte,item->type,
		   ((item->type >= 0) && (item->type <= MNU_NBCODES))?OpiumMenuType[item->type]:"(inconnu)",(IntAdrs)(item->adrs));
#endif
	item_pointe += 1;
	defaut = menu->defaut;
	code_rendu = 0;
	item_choisi = 0;
	doit_terminer = 0;
	if(e->type == WND_KEY) {
		l = (int)strlen(e->texte);
		if(DEBUG_MENU(1)) {
			WndPrint("%04lX ( ",e->code);
			for(m=0; m<l; m++) WndPrint("%02X ",e->texte[m]); WndPrint(")\n");
		}
		if(l <= 0) {
			if(e->code == XK_Up) {
				if(menu->defaut) menu->defaut -= 1;
				while(menu->defaut) {
					item = menu->items + menu->defaut - 1;
					if((item->sel == MNU_ITEM_OK) && (item->aff >= 0)) break;
					menu->defaut -= 1;
				}
			} else if(e->code == XK_Down) do {
				item = menu->items + menu->defaut;
				menu->defaut += 1;
				if((item->sel == MNU_ITEM_OK) && (item->aff >= 0)) break;
				if(menu->defaut >= menu->maxitems) menu->defaut = 0;
			} while(menu->defaut);
			else if(e->code == XK_Home) menu->defaut = 0;
			else if(e->code == XK_KP_F4) code_rendu = menu->maxitems;
		} else if(e->code == XK_Alt_L) {
			if(e->texte[0] == 'w') return(1);
			if(e->texte[0] == 'q') { WndQueryExit = 1; return(1); }
		} else {
			eol = (e->texte[l - 1] == 0x0D);
			if(eol) e->texte[--l] = '\0';
			if(l > 0) {
				if((e->texte[0] == 0x7F) || (e->texte[0] == 0x08)) {
					if(menu->curs > 0) menu->curs -= 1;
					else menu->defaut = 0;
				} else {
					item = menu->items;
					m = 1; n = 0;
					while(item->texte) {
						if(!menu->curs) item->key = item->sel;
						if((item->key == MNU_ITEM_OK) && !strncasecmp(item->traduit + menu->curs,e->texte,l)) {
							if(!n) item_choisi = m;
							n++; 
						} else item->key = MNU_ITEM_HS;
						item++; m++;
					}
					if(n > 1) {
						menu->defaut = item_choisi;
						item_choisi = 0;
						menu->curs += 1;
					}
				}
			}
			if(eol && !item_choisi) item_choisi = menu->defaut;
		}

	} else if(e->type == WND_RELEASE) {
		if((item_pointe <= 0) || (item_pointe > menu->maxitems)) return(code_rendu);
		item = menu->items + item_pointe - 1;
		if(item->sel != MNU_ITEM_OK) {
			if(item->sel == MNU_ITEM_HS) OpiumFail("Desactive");
			return(code_rendu);
		}
		doit_terminer = WndRefreshBegin(f);
		MenuPointe(menu,menu->item_pointe,f);
		switch(e->code) {
		  case WND_MSELEFT:   item_choisi  = item_pointe; break;
		  case WND_MSEMIDDLE: menu->defaut = item_pointe; break;
		  default: /* autres entrees souris dans un menu: pas d'action */
			break;
		}

	} else if(e->type == WND_PRESS) switch(e->code) {
		case WND_MSELEFT:
		  if((item_pointe <= 0) || (item_pointe > menu->maxitems)) break;
		  item = menu->items + item_pointe - 1;
		  if(item->sel != MNU_ITEM_OK) {
			  if(item->sel == MNU_ITEM_HS) OpiumFail("Desactive");
			  break;
		  }
		  if(menu->item_pointe != item_pointe) {
			  doit_terminer = WndRefreshBegin(f);
			  MenuPointe(menu,menu->item_pointe,f);
			  MenuPointe(menu,item_pointe,f);
		  }
		  if((item->type == MNU_CODE_MENU) || (item->type == MNU_CODE_PANEL)) item_choisi = item_pointe;
		  break;
		default: /* autres selections souris dans un menu: pas d'action */
		  break;
	}

	if(defaut != menu->defaut) {
	#ifdef ITEM_TEXTE_CONSTANT
		if(defaut > 0) {
			item = menu->items + defaut - 1;
			doit_terminer = WndRefreshBegin(f);
			WndClearText(f,MNU_NORMAL,item->aff,0,cdr->larg);
			WndWrite(f,MNU_NORMAL,item->aff,0,item->traduit);
		}
		if(menu->defaut > 0) {
			item = menu->items + menu->defaut - 1;
			doit_terminer = WndRefreshBegin(f);
			WndClearText(f,MNU_HILITE,item->aff,0,cdr->larg);
			WndWrite(f,MNU_HILITE,item->aff,0,item->traduit);
		}
	#else
		int x,y,d;
		d = (f->s)->lig - (f->s)->decal;
		if(defaut > 0) {
			item = menu->items + defaut - 1;
			doit_terminer = WndRefreshBegin(f);
			x = item->posx; y = item->posy; // + ((f->s)->lig / 2);
			l = item->lngr;
			WndClearString(f,MNU_NORMAL,x,y,l);
			WndDrawString(f,MNU_NORMAL,x,y+d,item->traduit,strlen(item->traduit));
		}
		if(menu->defaut > 0) {
			item = menu->items + menu->defaut - 1;
			doit_terminer = WndRefreshBegin(f);
			x = item->posx; y = item->posy; // + ((f->s)->lig / 2);
			l = item->lngr;
			WndClearString(f,MNU_HILITE,x,y,l);
			WndDrawString(f,MNU_HILITE,x,y+d,item->traduit,strlen(item->traduit));
		}
	#endif
	}
	if(doit_terminer) WndRefreshEnd(f);
	if(item_choisi) {
		TypeMenuFctn fctnMenu;
		menu->curs = 0;
		item = menu->items + item_choisi - 1;
	#ifdef DEBUG_RUNMENU
		printf("(%s.2) item choisi @%08llX: %s.type=%d (%s '%s') -> adrs=%08llX\n",__func__,(IntAdrs)item,item->texte,item->type,
			   ((item->type >= 0) && (item->type <= MNU_NBCODES))?OpiumMenuType[item->type]:"(inconnu)",item->traduit,(IntAdrs)(item->adrs));
	#endif
		OpiumNextTitle = item->traduit;
		if(cdr->log) LogBook(" (%s)\n",OpiumNextTitle);
		if(item->sel == MNU_ITEM_OK) switch(item->type) {
		  case MNU_CODE_MENU:
			if(!item->adrs) { code_rendu = 0; break; }
		#ifdef DEBUG_RUNMENU
			printf("(%s) Menu!\n",__func__);
		#endif
			menu = *(Menu *)item->adrs;
			code_rendu = OpiumUse(menu->cdr,OPIUM_EXEC);
			if(!code_rendu) *cdr_ouverts += 1;
			break;
		  case MNU_CODE_PANEL:
			if(!item->adrs) { code_rendu = 0; break; }
		#ifdef DEBUG_RUNMENU
			printf("(%s) panel!\n",__func__);
		#endif
			panel = *(Panel *)item->adrs;
			if((panel->cdr)->f) OpiumUse(panel->cdr,OPIUM_EXEC);
			else {
				OpiumPosition(panel->cdr,cdr->x+e->x,cdr->y+e->y);
				code_rendu = OpiumUse(panel->cdr,OPIUM_EXEC);
				if(!code_rendu) *cdr_ouverts += 1;
			}
			break;
		  case MNU_CODE_EXEC:
			if(!item->adrs) { code_rendu = 0; break; }
			cadre = *(Cadre *)item->adrs;
			if(cadre->f) OpiumUse(cadre,OPIUM_EXEC);
			else {
				code_rendu = OpiumUse(cadre,OPIUM_EXEC);
				if(!code_rendu) *cdr_ouverts += 1;
			}
			break;
		  case MNU_CODE_FORK:
			if(!item->adrs) { code_rendu = 0; break; }
			cadre = *(Cadre *)item->adrs;
			if(cadre->f) OpiumUse(cadre,OPIUM_FORK);
			else {
				code_rendu = OpiumUse(cadre,OPIUM_FORK);
				if(!code_rendu) *cdr_ouverts += 1;
			}
			break;
		  case MNU_CODE_DISPLAY:
			if(!item->adrs) { code_rendu = 0; break; }
			cadre = *(Cadre *)item->adrs;
			OpiumUse(cadre,OPIUM_DISPLAY);
			code_rendu = 0;
			break;
		  case MNU_CODE_FONCTION:
			if(!item->adrs) { code_rendu = 0; break; }
			fctnMenu = (TypeMenuFctn)item->adrs;
			code_rendu = (*fctnMenu)(menu,item);
			break;
		  case MNU_CODE_COMMANDE:
			if(!item->adrs) { code_rendu = 0; break; }
			code_rendu = WndLaunch((char * )item->adrs);
			break;
		  case MNU_CODE_SEPARE:
			code_rendu = item_choisi;
			break;
		  case MNU_CODE_CONSTANTE:
			v = (IntAdrs)item->adrs;
			code_rendu = (int)v;
			break;
		  case MNU_CODE_BOOLEEN:
			b = (char *)item->adrs;
			if(b) {
				/* Cadre board; */
				*b = (*b)? 0: 1;
				if(*b) MenuItemAllume(menu,item_choisi,0,GRF_RGB_YELLOW); else MenuItemEteint(menu,item_choisi,0);
			}
			code_rendu = 0;
			break;
		  case MNU_CODE_TOPWND:
			WndSendToTop((WndFrame)item->adrs);
			break;
		  case MNU_CODE_RETOUR:
			code_rendu = item_choisi;
		#ifdef DEBUG_RUNMENU
			printf("(%s) Retour!\n",__func__);
		#endif
			break;
		  case MNU_CODE_EXIT:
			WndEventLoopKill();
		#ifdef DEBUG_RUNMENU
			printf("(%s) WndQueryExit!\n",__func__);
		#endif
			// *cdr_ouverts = 1;
			code_rendu = item_choisi;
			break;
		}
		OpiumNextTitle = 0;
	}

	return(code_rendu);
}
#ifdef MENU_BARRE
/* ========================================================================== */
int MenuBarreManage(int menu_num, unsigned long item_num) {
	Menu menu; MenuItem item; Panel panel; Cadre cadre;
	char item_vu;
	TypeFctn fctn;
	int code_rendu; char *b;

	code_rendu = 0;
	item_vu = (char)(item_num & 0xFF);
	menu = MenuEnBarre; // GCC
	item = MenuEnBarre->items; while(item->texte && (item->aff != menu_num)) item++;
	if((item->type == MNU_CODE_MENU) && (item_num != 999)) {
		menu = *(Menu *)(item->adrs);
		item = menu->items; while(item->texte && (item->aff != item_vu)) item++;
	}
	// printf("(%s) Menu #%d, item #%d soit texte=[%s] type %d ->  %08lX\n",__func__,menu_num,item_num,item->texte,item->type,(Ulong)item->adrs);
	OpiumNextTitle = item->traduit;
	if(item->sel == MNU_ITEM_OK) switch(item->type) {
	  case MNU_CODE_MENU:
		if(!item->adrs) break;
		menu = *(Menu *)item->adrs;
		code_rendu = OpiumUse(menu->cdr,OPIUM_EXEC);
		break;
	  case MNU_CODE_PANEL:
		if(!item->adrs) break;
		panel = *(Panel *)item->adrs;
		if((panel->cdr)->f) OpiumUse(panel->cdr,OPIUM_EXEC);
		else code_rendu = OpiumUse(panel->cdr,OPIUM_EXEC);
		break;
	  case MNU_CODE_EXEC:
		if(!item->adrs) break;
		cadre = *(Cadre *)item->adrs;
		if(cadre->f) OpiumUse(cadre,OPIUM_EXEC);
		else code_rendu = OpiumUse(cadre,OPIUM_EXEC);
		break;
	  case MNU_CODE_FORK:
		if(!item->adrs) break;
		cadre = *(Cadre *)item->adrs;
		if(cadre->f) OpiumUse(cadre,OPIUM_FORK);
		else code_rendu = OpiumUse(cadre,OPIUM_FORK);
		break;
	  case MNU_CODE_DISPLAY:
		if(!item->adrs) break;
		cadre = *(Cadre *)item->adrs;
		OpiumUse(cadre,OPIUM_DISPLAY);
		code_rendu = 0;
		break;
	  case MNU_CODE_FONCTION:
		if(!item->adrs) break;
		fctn = (TypeFctn)item->adrs;
		code_rendu = (*fctn)(menu,item);
		break;
	  case MNU_CODE_COMMANDE:
		if(!item->adrs) break;
		code_rendu = WndLaunch((char*)item->adrs);
		break;
	  case MNU_CODE_SEPARE:
		code_rendu = item_num + 1;
		break;
	  case MNU_CODE_CONSTANTE:
		code_rendu = (int)item->adrs;
		break;
	  case MNU_CODE_BOOLEEN:
		b = (char *)item->adrs;
		if(b) *b = (*b)? 0: 1;
		code_rendu = 0;
		break;
	  case MNU_CODE_TOPWND:
		WndSendToTop((WndFrame)item->adrs);
		break;
	  case MNU_CODE_RETOUR:
		code_rendu = item_num + 1;
		break;
	} else if(item->sel == MNU_ITEM_HS) OpiumFail("Desactive");
	OpiumNextTitle = 0;
	return(code_rendu);
}
/* ========================================================================== */
void MenuBarreCreate(Menu menu) {
	MenuItem item;
	Menu sousmenu; MenuItem sousitem;
	char col,lig;
	char *texte;

	if(!(MenuEnBarre = menu)) return;
	item = menu->items; if(!item) return;
	col = 0;
	while(item->texte) {
		if(item->sel == MNU_ITEM_CACHE) goto item_svt;
		if(item->type == MNU_CODE_MENU) {
			if(!WndMenuCreate(item->traduit)) break;
			item->aff = col++;
			if(WndCodeHtml) goto item_svt;
			sousmenu = *(Menu *)item->adrs; if(!sousmenu) goto item_svt;  /* menu pas encore cree */
			sousitem = sousmenu->items; if(!sousitem) goto item_svt;      /* item vide */
			lig = 0;
			while(sousitem->texte) {
				/* (1) item pas montre => erreur sur le numero d'item rendu a MenuBarreManage;
				   (2) separation avec texte: trait de separation pas trace en plus du texte. */
				if(sousitem->sel == MNU_ITEM_CACHE) sousitem->aff = -1; else {
					switch(sousitem->type) {
					  // case MNU_CODE_SEPARE: if(sousitem->traduit[0]) WndMenuItem(sousitem->traduit,'s',0); else WndMenuSepare(); break;
					  case MNU_CODE_SEPARE: if(sousitem->traduit[0]) WndMenuAnnonce(sousitem->traduit); else WndMenuSepare(); break;
					  case MNU_CODE_RETOUR: break;
					  default: WndMenuItem(sousitem->traduit,
						(sousitem->type == MNU_CODE_MENU)? 'm': ((sousitem->type == MNU_CODE_PANEL)? 'p': '\0'),sousitem->sel); break;
					}
					sousitem->aff = lig++;
				}
				sousitem++;
			}
			WndMenuInsert();
		} else if((item->type != MNU_CODE_SEPARE) && (item->type != MNU_CODE_RETOUR)) {
			if(!WndMenuCreate(item->traduit)) break;
			item->aff = col++;
			if((item->type == MNU_CODE_FONCTION) || (item->type == MNU_CODE_COMMANDE)) 
				texte = L_("Lancer");
			else texte = L_("Afficher");
			WndMenuItem(texte,'p',item->sel); WndMenuInsert();
		}
item_svt:
		item++; // i++;
	}
	WndMenuCreate(L_("Fenetres")); WndMenuItem(L_("Sauver en JPEG"),0,1); WndMenuSepare(); WndMenuInsert();
	WndMenuDisplay();
}
//#define GESTION_BARRE_V0
/* ========================================================================== */
void MenuBarreFenetreOuverte(Cadre cdr) { WndMenuItem(cdr->titre,0,1); }
/* ========================================================================== */
void MenuBarreExec(void) {
	Cadre cdr; WndAction u; int cdr_ouverts; int i;

	WndQueryExit = 0;
	cdr = 0;
	cdr_ouverts = 0;
	u.type = -1; /* rapport au cas WND_PRESS */
	printf("(%s) Mode %s\n",__func__,WndCodeHtml? "serveur": "ecran");
#ifdef GESTION_BARRE_V0
	while(!WndQueryExit) {
		WndEventNew(&u,WND_WAIT);
		if(u.type == WND_BARRE) { if(u.x >= 0) MenuBarreManage(u.x,u.code); }
		else OpiumManage(0,&u,&cdr_ouverts);
		if(!WndMenuAffiche && !OpiumDisplayed(MenuEnBarre->cdr)) {
			if(!OpiumUse(MenuEnBarre->cdr,OPIUM_EXEC)) OpiumRefresh(MenuEnBarre->cdr);
		}
	}
#else
	if(WndCodeHtml) {
		OpiumHttpDesc = 0;
		// printf("(%s) Sur le port %d\n",__func__,OpiumHttpPort);
		HttpServeur(OpiumHttpPort);
	} else while(!WndQueryExit) {
		if(WndEventNew(&u,WND_CHECK)) {
			// WndPrint("(%s) Recupere evt '%s %s'\n",__func__,(u.type==-1)?"neant":WND_ACTIONNAME(u.type),
			//	(u.type==-1)?"de chez  neant":((u.type == WND_KEY)?u.texte:OpiumMseButton[u.code]));
			OpiumDernierMessage[0] = '\0';
			OpiumManage(0,&u,&cdr_ouverts);
			if(!WndMenuAffiche && !OpiumDisplayed(MenuEnBarre->cdr)) {
				if(!OpiumUse(MenuEnBarre->cdr,OPIUM_EXEC)) OpiumRefresh(MenuEnBarre->cdr);
			}
			if(OpiumDernierMessage[0] == '!') OpiumWarn(OpiumDernierMessage);
			else if(OpiumDernierMessage[0] == '*') OpiumNote(OpiumDernierMessage+2);
		} else {
			if(OpiumUserPeriodic) (*OpiumUserPeriodic)();
			for(i=0; i<OpiumNbRefresh; i++) {
				cdr = OpiumToRefresh[i];
				if(cdr->restant > 0) cdr->restant -= 17;
				else {
					if(OpiumDisplayed(cdr)) {
						if(cdr->type == OPIUM_PANEL) PanelRefreshVars((Panel)(cdr->adrs));
						else if(cdr->type == OPIUM_INSTRUM) InstrumRefreshVar((Instrum)(cdr->adrs));
						else OpiumRefresh(cdr);
						cdr->restant = cdr->delai;
					}
				}
			}
			TimerMilliSleep(17);
		}
	}
#endif

}
#endif /* MENU_BARRE */
#define MENU_WND_MAX 64
/* ==========================================================================
static void MenuTopWndDump(const char *fctn) {
	MenuItem item; int i;

	printf("(%s) ___.___ Fenetres referencees ___\n",fctn);
	if((item = mMenuFenetres->items)) {
		i = 1; while(item->texte) {
			printf("(%s) %2d | F=%12llX: '%s'\n",fctn,i,(IntAdrs)(item->adrs),item->texte);
			item++; i++;
		}
		printf("(%s) ---|----------------------------\n",fctn);
	} else printf("(%s) ---|--- aucune ----------------\n",fctn);
}
   ========================================================================== */
static int MenuTopPhoto(Menu menu, MenuItem item) { OpiumPhotoSauve(); return(1); }
/* ========================================================================== */
void MenuTopWndItem(Menu menu) {
	if(!menu) return;
	if(MenuItemArray(mMenuFenetres,MENU_WND_MAX)) {
		MenuItemAdd(mMenuFenetres,L_("Sauver en JPEG"),MNU_FONCTION MenuTopPhoto);
		MenuItemAdd(mMenuFenetres,L_("Affichees"),MNU_SEPARATION);
		if(MenuItemAdd(menu,L_("Fenetres"),MNU_MENU &mMenuFenetres)) {
			mMenuTop = menu;
			// MenuTopWndDump(__func__);
		} else MenuItemDelete(mMenuFenetres);
	}
}
/* ========================================================================== */
void MenuTopWndAdd(Cadre cdr) {
	if((cdr == 0) || (cdr == mMenuTop->cdr) || (cdr == mMenuFenetres->cdr)) return;
	if(cdr->f) {
		int nb;
		// printf("(%s) +++++ Ajout du cadre '%s' (F=%12llX)\n",__func__,cdr->titre,(IntAdrs)cdr->f);
		nb = MenuItemAdd(mMenuFenetres,cdr->titre,MNU_TOPWND cdr->f);
		// if(nb) printf("(%s) Item #%d '%s': ajoute\n",__func__,nb,mMenuFenetres->items[nb-1].texte);
		// else printf("(%s) Item abandonne\n",__func__);
		// MenuTopWndDump(__func__);
		if(OpiumAlEcran(mMenuFenetres)) {
			OpiumSizeMenu(mMenuFenetres->cdr,0);
			OpiumRefresh(mMenuFenetres->cdr);
		}
	}
}
/* ========================================================================== */
void MenuTopWndDel(Cadre cdr) {
	MenuItem item,svt;

	if((cdr == 0) || (cdr == mMenuTop->cdr) || (cdr == mMenuFenetres->cdr)) return;
	// printf("(%s) ----- Retrait du cadre '%s' (F=%12llX)\n",__func__,cdr->titre,(IntAdrs)cdr->f);
	if(!(cdr->f)) return;
	if((item = mMenuFenetres->items)) {
		while(item->texte) {
			if(item->adrs == (void *)(cdr->f)) break;
			// printf("(%s) Item '%s': garde\n",__func__,item->texte);
			item++;
		}
		if(item->texte) {
			do {
				svt = item + 1;
				// printf("(%s) Item '%s': ",__func__,item->texte);
				memcpy(item,svt,sizeof(TypeMenuItem));
				// if(item->texte) printf("remplace par '%s\n",item->texte);
				// else printf("suprime\n");
				item = svt;
			} while(item->texte);
			mMenuFenetres->nbitems -= 1;
			if(OpiumAlEcran(mMenuFenetres)) {
				OpiumSizeMenu(mMenuFenetres->cdr,0);
				OpiumRefresh(mMenuFenetres->cdr);
			}
		}
	}
	// MenuTopWndDump(__func__);
}

