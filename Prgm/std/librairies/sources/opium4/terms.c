#include <stdio.h>
#include <ctype.h>
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

#ifdef OS9
#define ENDOFLINE 0x0d
#define LINEFEED 0x0a
#else
#define ENDOFLINE 0x0a
#define LINEFEED 0x0d
#endif

/* ========================================================================== */
Term TermCreate(int ligs, int cols, int taille) {
	Cadre cdr; Term term;

	term = (Term)malloc(sizeof(TypeTerm));
	if(term) {
		if(DEBUG_TERM(1)) WndPrint("Term cree @%08lX\n",(Ulong)term);
		cdr = OpiumCreate(OPIUM_TERM,term);
		if(!cdr) {
			free(term); return(0); 
		}
		term->buffer = (char *)malloc(taille);
		if(!term->buffer) {
			OpiumDelete(cdr); free(term); return(0); 
		}
		term->cdr = cdr;
		term->haut = ligs;
		term->larg = cols;
		term->normal = 0;
		term->taille = taille;
		term->maxlignes = taille / 20;
		term->ligne = (int *)malloc(term->maxlignes * sizeof(int));
		if(!term->ligne) {
			free(term->buffer);
			OpiumDelete(cdr); free(term); return(0);
		}
		TermEmpty(term);
		OpiumSetOptions(cdr);
		if(DEBUG_TERM(3)) WndPrint("Utilise cdr @%08lX\n",(Ulong)term->cdr);
	}
	return(term);
}
/* ========================================================================== */
void TermEmpty(Term term) {
	term->nblignes = 0;
	term->premiere = 0;
	term->derniere = term->courante = -1;
	term->col1 = 0;
	term->debut = 0;
	term->nbcars = 0;
	term->hold = 0;
	term->printed = 0;
}
/* ========================================================================== */
void TermDelete(Term term) {
	free(term->ligne);
	free(term->buffer);
	OpiumDelete(term->cdr);
	free(term);
}
/* ========================================================================== */
void TermTitle(Term t,char *texte) { OpiumTitle(t->cdr,texte); }
/* ========================================================================== */
void TermColumns(Term term, int cols) { term->larg = cols; }
/* ========================================================================== */
void TermLines(Term term, int ligs) { term->haut = ligs; }
/* ========================================================================== */
void TermHold(Term term) { term->hold = 1; }
/* ========================================================================== */
void TermRelease(Term term) {
	Cadre cdr;

	term->hold = 0; 
	if(term->printed) { cdr = term->cdr; if(cdr->f) OpiumRefresh(cdr); }
}
/* ========================================================================== */
void TermBlank(Term term) { WndFrame f; if((f = (term->cdr)->f)) WndBlank(f); }
/* ========================================================================== */
void TermAdd(Term term, char *ligne) {
	int  car,suivant,suivante;
	char *ecrit,*buffer,precedent;
	char on_top;
	Cadre cdr; int haut,sorties;

	if(DEBUG_TERM(2)) WndStep("debut du Print");

	/* car= num. du caractere courant (=a entrer);
	   precedent: caractere precedent (si \0d, une nouvelle ligne est a creer) */
	car = term->debut + term->nbcars;
	if(DEBUG_TERM(2)) WndPrint("(TermAdd) nbcars=%d\n",term->nbcars);
	if(term->nbcars) {
		while(car >= term->taille) car -= term->taille;
		if(car) precedent = term->buffer[car - 1];
		else precedent = term->buffer[term->taille - 1];
	} else precedent = ENDOFLINE; /* buffer vide */
	if(DEBUG_TERM(2)) WndPrint("(TermAdd) car1 #%d, precedent=%02x\n",car,precedent);

	buffer = term->buffer + car;
	ecrit = ligne;
	cdr = term->cdr;
	while(*ecrit) {
		if(precedent == ENDOFLINE) {
			/* Creation d'une ligne supplementaire */
			term->derniere += 1;
			while(term->derniere >= term->maxlignes) term->derniere -= term->maxlignes;
			/* Tampon des lignes plein? */
			if(term->nblignes >= term->maxlignes) {
				on_top = (term->courante == term->premiere);
				term->premiere = term->derniere + 1;
				while(term->premiere >= term->maxlignes) term->premiere -= term->maxlignes;
				if(on_top) term->courante = term->premiere;
				term->col1 = 0;
			} else term->nblignes += 1;
			term->ligne[term->derniere] = car;
			if(DEBUG_TERM(2)) WndPrint("(TermAdd) premiere=%d, derniere=%d\n",
				term->premiere,term->derniere);
		}
		if((car == term->debut) && (term->nbcars > 0)) {
			/* Tampon des caracteres plein: modifier la premiere ligne */
			suivant = term->debut + 1; while(suivant >= term->taille) suivant -= term->taille;
			suivante = term->premiere + 1; while(suivante >= term->maxlignes) suivante -= term->maxlignes;
			if(suivant == term->ligne[suivante]) {
				/* le caractere suivant est le debut de la ligne suivante */
				if(term->courante == term->premiere) term->courante = suivante;
				term->premiere = suivante;
				term->nblignes -= 1;
				term->col1 = 0;
				if(DEBUG_TERM(2)) WndPrint("(TermAdd) premiere=%d @%d\n",term->premiere,
					term->ligne[term->premiere]);
			} else {
				/* le caractere suivant est le debut de la premiere ligne: lui decaler le debut */
				if(term->ligne[term->premiere] == car) {
					term->ligne[term->premiere] += 1;
					if(term->ligne[term->premiere] >= term->taille) term->ligne[term->premiere] = 0;
					term->col1 += 1;
					if(DEBUG_TERM(2)) WndPrint("(TermAdd) colonne1=%d @%d\n",term->col1,
						term->ligne[term->premiere]);
				}
			} /* sinon, les 2 tampons ne coincident pas */
			term->debut = suivant;
		} else term->nbcars += 1; 
		precedent = *ecrit;
		*buffer++ = *ecrit++; car++; 
		while(car >= term->taille) {
			car -= term->taille; buffer = term->buffer + car; 
		}
	}

	term->printed = 1;
	/* Nombre de lignes total du cadre */
	haut = WndLigToPix(term->nblignes);
	if(cdr->haut < haut) cdr->haut = haut;
	if(DEBUG_TERM(2)) WndPrint("(TermAdd) %d lignes\n",term->nblignes);
	/* Deplacement automatique de l'ascenseur a fond en bas */
	haut = WndPixToLig(cdr->dv);     /* nb lignes affichables dans la fenetre */
	if(haut > term->nblignes) haut = term->nblignes; /* nb lignes a afficher  */
	sorties = term->nblignes - haut; /* nb lignes "au dessus"  de la fenetre  */
	cdr->dy = WndLigToPix(sorties);  /* = decalage de l'ascenseur             */
	if(cdr->f && !term->hold) {
		if(DEBUG_TERM(2)) WndStep("Print termine");
		OpiumRefresh(cdr);
	}
	
	if(DEBUG_TERM(3)) WndPrint("(TermAdd) termine..\n");
}
/* ========================================================================== */
int TermPrint(Term term, char *fmt, ...) {
	char ligne[256];
	va_list argptr; int cnt;

	va_start(argptr, fmt);
	cnt = vsprintf(ligne, fmt, argptr);
	va_end(argptr);

	if(DEBUG_TERM(1)) WndPrint("(TermPrint) Ligne ecrite: {%s}\n",ligne);
	TermAdd(term,ligne);
	return(cnt);
}
/* ==========================================================================
void TermFloat(Term term, char *format,
	float p0, float p1, float p2, float p3, float p4,
	float p5, float p6, float p7, float p8, float p9) {
	char ligne[256];

	sprintf(ligne,format,p0,p1,p2,p3,p4,p5,p6,p7,p8,p9);
	if(DEBUG_TERM(1)) WndPrint("(TermFloat) Ligne ecrite: {%s}\n",ligne);
	TermAdd(term,ligne);
}
   ==========================================================================
void TermDouble(Term term, char *format,
	double p0, double p1, double p2, double p3, double p4,
	double p5, double p6, double p7, double p8, double p9) {
	char ligne[256];

	sprintf(ligne,format,p0,p1,p2,p3,p4,p5,p6,p7,p8,p9);
	if(DEBUG_TERM(1)) WndPrint("(TermDouble) Ligne ecrite: {%s}\n",ligne);
	TermAdd(term,ligne);
}
   ========================================================================== */
int TermScroll(Term term, int depl) {
#ifdef A_LA_MAIN
	if(!term) return(0);
	term->courante += depl;

	if(term->courante < 0) term->courante += term->maxlignes;
	if(term->courante >= term->maxlignes) term->courante -= term->maxlignes;

	if(term->premiere > term->derniere) {
		if((term->derniere < term->courante) && (term->courante < term->premiere)) {
			if(depl > 0) term->courante = term->derniere;
			else term->courante = term->premiere;
		}
	} else if(term->courante < term->premiere)
		term->courante = term->premiere;
	else if(term->derniere < term->courante)
		term->courante = term->derniere;
#else
	Cadre cdr;
	int ligmin;

	if(!term) return(0);
	cdr = term->cdr;
	ligmin = (cdr->dy)? WndPixToLig(cdr->dy - 1) + 1: 0;
	ligmin += depl;
	if(ligmin < 0) ligmin = 0;
	else if(ligmin >= term->nblignes) ligmin = term->nblignes - 1;
	cdr->dy = WndLigToPix(ligmin);
#endif
	return(1);
}
/* ========================================================================== */
void TermDump(Term term, FILE *f) {
	unsigned char *buffer;
	int i,j,k;

	fprintf(f,"%d lignes, %d colonnes\n",term->haut,term->larg);
	fprintf(f,"Buffer[%d] rempli avec %04X caracteres, debut @%d\n",
		term->taille,term->nbcars,term->debut);
	fprintf(f,"%d/%d lignes enregistrees\n",term->nblignes,term->maxlignes);
	fprintf(f,"Premiere ligne=%d, demarre en colonne %d, pointe @%d\n",
		term->premiere,term->col1,term->ligne[term->premiere]);
	fprintf(f,"Derniere ligne=%d, pointe @%d\n",
		term->derniere,term->ligne[term->derniere]);
	buffer = (unsigned char *)term->buffer;
	i = 0;
	while(i < term->nbcars) {
		fprintf(f,"%08X ",i);
		for(k=0; k<16; k+=4) {
			fprintf(f," ");
			for(j=k; j<k+4; j++) {
				if((i+j) < term->nbcars) fprintf(f,"%02x",buffer[i+j]);
				else fprintf(f,"  ");
			}
		}
		fprintf(f,"  ");  
		for(k=0; k<16; k+=4) {
			for(j=k; j<k+4; j++) {
				if((i+j) >= term->nbcars) break;
				if(isprint(buffer[i+j])) fprintf(f,"%c",buffer[i+j]);
				else fprintf(f,".");
			}
		}
		fprintf(f,"\n");
		i += 16;
	}
}
/* ========================================================================== */
void OpiumTermColorNb(Cadre cdr, Cadre fait, short *nc) {
	Term term,modele; short ic;

	if(!(term = (Term)cdr->adrs)) return;
	if(fait) modele = (Term)fait->adrs; else modele = 0;
	if(modele) {
		term->normal = modele->normal;
	} else {
		ic = *nc;
		term->normal = ic++;
		*nc = ic;
	}
}
/* ========================================================================== */
void OpiumTermColorSet(Cadre cdr, WndFrame f) {
	Term term;

	if(!(term = (Term)cdr->adrs)) return;
	WndFenColorSet(f,term->normal,WND_GC_STD);
}
/* ========================================================================== */
int OpiumSizeTerm(Cadre cdr, char from_wm) {
	Term term;
#ifdef OPIUM_AJUSTE_WND
	int haut,larg;
#endif

	if(DEBUG_OPIUM(1) && (cdr->type != OPIUM_TERM)) {
		WndPrint("+++ OpiumSizeTerm sur le %s @%08lX\n",cdr->nom,(Ulong)cdr->adrs);
		return(0);
	}
	term = (Term)cdr->adrs;
	if(from_wm) {
/* 		term->larg = WndPixToCol(cdr->larg); term->haut = WndPixToLig(cdr->haut); */
#ifdef OPIUM_AJUSTE_WND
		larg = WndPixToCol(cdr->dh);
		cdr->dh = WndColToPix(larg);
		haut = WndPixToLig(cdr->dv);
		cdr->dv = WndLigToPix(haut);
#endif
		return(1);
	} else {
		cdr->larg = WndColToPix(term->larg);
		cdr->haut = WndLigToPix(term->haut);
		cdr->dh = cdr->larg; cdr->dv = cdr->haut;
	}
	return(1);
}
/* ========================================================================== */
int OpiumRefreshTerm(Cadre cdr) {
	Term term;
	WndFrame f;
	int car,nbcars;
	int courante,derniere,nblignes,lig,col1,haut;
	char *ecrit,*buffer;
#define MAX_LIGNE_ECRITE 256
	char ligne[MAX_LIGNE_ECRITE+1]; int l;

/*	OpiumDebug(OPIUM_DEBUG_TERM,1); */
	if(DEBUG_OPIUM(1) && (cdr->type != OPIUM_TERM)) {
		WndPrint("+++ OpiumDisplayTerm sur le %s @%08lX\n",cdr->nom,(Ulong)cdr->adrs);
		return(0);
	}
	term = (Term)cdr->adrs;
	if(term->hold) return(0);
	if(!OpiumCoordFenetre(cdr,&f)) return(0);

	/* Decalage maxi de facon a voir au moins une ligne */
	if(cdr->ligmin >= term->nblignes) {
		cdr->ligmin = term->nblignes - 1;
		cdr->dy = WndLigToPix(cdr->ligmin);
	}

	/* Nombre de lignes a voir (pas plus que la taille de la fenetre) */
	haut = term->nblignes - cdr->ligmin;
	nblignes = (cdr->lignb < haut)? cdr->lignb: haut;
	if(nblignes == 0) return(1);

	/* Premiere et derniere lignes a imprimer */  
	courante = term->premiere + cdr->ligmin;
	if(courante >= term->maxlignes) courante -= term->maxlignes;
	derniere = courante + nblignes - 1;
	if(derniere >= term->maxlignes) derniere -= term->maxlignes;

	if(DEBUG_TERM(2)) {
		WndPrint("Buffer[%d] rempli avec %d caracteres, debut @%d\n",
			term->taille,term->nbcars,term->debut);
		WndPrint("%d/%d lignes enregistrees\n",term->nblignes,term->maxlignes);
		WndPrint("Premiere ligne=%d, demarre en colonne %d, pointe @%d\n",
			term->premiere,term->col1,term->ligne[term->premiere]);
		WndPrint("Derniere ligne=%d, pointe @%dX\n",
			term->derniere,term->ligne[term->derniere]);
		WndPrint("ligne courante=%d, caractere #%04X\n",
			courante,term->ligne[courante]);
	}

//+	WndBlank(f); /* de toutes facons, on commence a lig = 0 */
	WndFillBgnd(f,WND_GC(f,term->normal),0,0,cdr->dh,cdr->dv);
	if(courante == term->premiere) col1 = term->col1; else col1 = 0;
	car = term->ligne[courante];
	buffer = term->buffer + car;
	lig = 0;
	nbcars = 0;
	while((nbcars < term->nbcars) && (lig < cdr->lignb)) {
		ecrit = ligne; l = 0;
		if(DEBUG_TERM(2)) WndPrint("(Refresh) Ligne[%d] ecrite en %d",courante,lig);
		while((*buffer != ENDOFLINE) && (*buffer != LINEFEED) && (nbcars < term->nbcars) && (l < MAX_LIGNE_ECRITE)) {
			*ecrit++ = *buffer++; car++; nbcars++; l++;
			if(car >= term->taille) {
				car = 0; buffer = term->buffer; 
			}
		}
		if(DEBUG_TERM(2)) WndPrint(" (%04X caracteres)\n",l);
		*ecrit = '\0';
		if(DEBUG_TERM(3)) WndPrint("(Refresh) a (%d, %d): {%s}\n",lig,col1,ligne);
		if(col1 >= cdr->colmin)
			WndWrite(f,WND_GC(f,term->normal),lig,col1-cdr->colmin,ligne);
		else WndWrite(f,WND_GC(f,term->normal),lig,0,ligne+cdr->colmin-col1);
		if(courante == derniere) break;
		switch(*buffer) {
		  case ENDOFLINE:
			if(++courante >= term->maxlignes) courante = 0;
/*			if(++lig >= haut) lig = 0; */
			++lig;
			col1 = 0;
			break;
		  case LINEFEED:
			col1 = 0;
			break;
		  default:
			col1 += l;
			if(col1 >= term->larg) {
				col1 = 0;
/*				if(++lig >= haut) lig = 0; */
				++lig;
			}
		}
		if(DEBUG_TERM(3)) WndPrint("(Refresh) courante -> %d\n",courante);
		buffer++; car++; nbcars++;
		if(car >= term->taille) {
			car = 0; buffer = term->buffer; 
		}
	}
	term->printed = 0;
	if(DEBUG_TERM(2)) WndPrint("(Refresh) termine..\n");

	return(1);
}
/* ========================================================================== */
int OpiumRunTerm(Cadre cdr, WndAction *e) {
	Term term;
	WndFrame f;
	int lig,col1;
	size_t i,l;
	char fin_de_ligne;
	int code_rendu;

	if(DEBUG_OPIUM(1) && (cdr->type != OPIUM_TERM)) {
		WndPrint("+++ OpiumRunTerm sur le %s @%08lX\n",cdr->nom,(Ulong)cdr->adrs);
		return(0);
	}
	if(!OpiumCoordFenetre(cdr,&f)) return(0);
	term = (Term)cdr->adrs;
	lig = WndPixToLig(e->y); col1 = WndPixToCol(e->x);
	code_rendu = 0;
	if(e->type == WND_KEY) {
		l = strlen(e->texte);
		if(DEBUG_TERM(2)) {
			WndPrint("%04lX ( ",e->code);
			for(i=0; i<l; i++) WndPrint("%02X ",e->texte[i]); WndPrint(")\n");
		}
		if(l <= 0) {
			if(e->code == XK_Up) {
				if(term->premiere > 0) term->premiere -= 1; 
			} else if(e->code == XK_Down) {
				if(term->premiere < (term->haut - 1)) term->premiere += 1; 
			} else if(e->code == XK_Left) {
				if(term->col1 > 0) term->col1 -= 1; 
			} else if(e->code == XK_Right) {
				if(term->col1 < (term->larg - 1)) term->col1 += 1; 
			} else if(e->code == XK_Home) {
				term->premiere = term->col1 = 0; 
			} else if(e->code == XK_KP_F4) code_rendu = 1;
			return(code_rendu);
		} else {
			fin_de_ligne = (e->texte[l - 1] == 0x0D);
			for(i=0; i<l; i++) if(!isprint(e->texte[i])) e->texte[i] = ' ';
			WndWrite(f,WND_GC(f,term->normal),term->premiere,term->col1,e->texte);
		}
		if(fin_de_ligne) {
			term->col1 = 0; term->premiere += 1; 
		} else {
			term->col1 += 1;
			if(term->col1 >= term->larg) {
				term->col1 = 0; term->premiere += 1; 
			}
		}
		if(term->premiere >= term->haut) term->premiere = 0;
	} else if(e->type == WND_RELEASE) switch(e->code) {
		case WND_MSELEFT:
		term->premiere = lig; term->col1 = col1;
		break;
		case WND_MSERIGHT:
		code_rendu = 1;
		break;
	}

	return(code_rendu);
}

