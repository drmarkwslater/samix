#include <stdio.h>
#include <ctype.h>
#include <stdlib.h> /* pour malloc, free et system */

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
#include "decode.h"

/* ................................. constantes ........................... */
#define MAXLEVELS 32
#define MAXPANEL 64
#define MAXLIGNE 74
#define LIGMENU 100
#define COLMENU 100
/* ............................. variables globales ....................... */
static char HelpModify = 0;
static char HelpInitGen = 1;
static char HelpInitItm,HelpRecurs;
static char *HelpDir = ".";
static char HelpLine[MAXLIGNE],HelpOutputName[40];
static char *HelpCommand[MAXLEVELS];
static Cadre HelpCdrCalled;
static Menu HelpMenuCalled,HelpMenuTemp;
static Panel HelpPanelCalled;
static MenuItem HelpItemTemp;
static int HelpLig,HelpCol,HelpLevel,HelpItemRecurs;
static FILE *HelpOutputAdrs;

/* ................................ terms ................................. */
Term HelpTerm;

/* ................................ panels ................................ */
Panel HelpPanelPpal,HelpPanelWrite;
char HelpNumLig[MAXPANEL][4];
char HelpTexte[MAXPANEL][MAXLIGNE];

/* ................................ menus ................................. */
static int HelpRead(Menu menu, MenuItem item);
static int HelpChange(Menu menu, MenuItem item);
static int HelpWrite(Menu menu, MenuItem item);

Menu HelpMenuPpal,HelpMenuType;
TypeMenuItem HelpItemPpal[] = {
	{ "Consultation",        MNU_FONCTION HelpRead },
	{ "Modification",        MNU_FONCTION HelpChange },
	{ "Impression",          MNU_FONCTION HelpWrite },
	{ "Quitter",             MNU_RETOUR },
	MNU_END
};
TypeMenuItem HelpItemType[] = {
	{ "Vue generale",        MNU_RETOUR },
	{ "Item particulier",    MNU_RETOUR },
	{ "Toute l'aide",        MNU_RETOUR },
	{ "Quitter",             MNU_RETOUR },
	MNU_END
};

static void HelpSetName(char *nom);

/* ======================================================================= */
void HelpSource(char *nom) {
	HelpDir = nom; 
}
/* ======================================================================= */
void HelpMode(char mode) {
	HelpModify = mode; 
}
/* ======================================================================= */
int HelpMenu(Cadre cdr, int cle) {
	int i;

	if(HelpInitGen) {
		HelpLig = 1;
		HelpCol = 1;
		HelpMenuPpal = MenuTitled("Aide en ligne",HelpItemPpal);
		HelpMenuType = MenuTitled("Type d'aide",HelpItemType);
		OpiumPosition(HelpMenuPpal->cdr,COLMENU,LIGMENU);
		OpiumPosition(HelpMenuType->cdr,COLMENU,LIGMENU);
		OpiumLog(HelpMenuPpal->cdr,0);
		OpiumLog(HelpMenuType->cdr,0);
		HelpMenuTemp = MenuCreate(0);
		HelpPanelPpal = PanelCreate(MAXPANEL);
		for(i=0; i<MAXPANEL; i++) {
			sprintf(HelpNumLig[i],"%02d",i+1);
			PanelText(HelpPanelPpal,HelpNumLig[i],HelpTexte[i],MAXLIGNE);
		}
		strcpy(HelpOutputName,"aide.doc");
		HelpRecurs = 1;
		HelpPanelWrite = PanelCreate(2);
		HelpItemRecurs = PanelBool(HelpPanelWrite,"Egalement menus suivants",&HelpRecurs);
		PanelText(HelpPanelWrite,"Fichier de sortie",HelpOutputName,40);
		HelpTerm = TermCreate(24,80,16384);
		HelpInitGen = 0;
	}
	if(DEBUG_OPIUM(1)) WndPrint("titre(%08lX)=[%s]\n",(Ulong)cdr,cdr->nom);
	HelpCdrCalled = cdr;
	switch(cdr->type) {
	  case OPIUM_MENU: HelpMenuCalled = (Menu)cdr->adrs; break;
	  case OPIUM_PANEL: HelpPanelCalled = (Panel)cdr->adrs; break;
	  default: return(0);
	}
	HelpInitItm = 1;
	HelpItemTemp = 0;
	if(HelpModify) OpiumExec(HelpMenuPpal->cdr); else HelpRead(0,0);
	return(0);
}
/* ======================================================================= */
static void HelpSetName(char * nom)  {
	size_t n,l,i;
	char *titre;

	titre = HelpCdrCalled->nom;
	if(DEBUG_OPIUM(1)) WndPrint("titre(%08lX)=[%s]\n",(Ulong)HelpCdrCalled,titre);
	strcpy(nom,HelpDir); RepertoireTermine(nom);
	n = strlen(nom);
	strcat(nom,titre);
	l = strlen(titre);
	for(i=0; i<l; i++) if(!isalpha(titre[i])) nom[i+n] = '_';
}
/* ....................................................................... */
static void HelpCreateMenuTempo(void) {
	int nbopt,i;
	MenuItem menucalled; PanelItem panelcalled;

	if(HelpInitItm) {
		switch(HelpCdrCalled->type) {
		  case OPIUM_MENU:
			nbopt = HelpMenuCalled->nbitems;
			if(HelpItemTemp) free(HelpItemTemp);
			HelpItemTemp = (MenuItem)malloc((nbopt + 1) * sizeof(TypeMenuItem));
			menucalled = HelpMenuCalled->items;
			for(i=0; i<nbopt; i++) {
				HelpItemTemp[i].texte = menucalled[i].texte;
				HelpItemTemp[i].sel = 1; /* menucalled[i].sel; */
				if(menucalled[i].type == MNU_CODE_SEPARE)
					HelpItemTemp[i].type = MNU_CODE_SEPARE;
				else HelpItemTemp[i].type = MNU_CODE_RETOUR;
				HelpItemTemp[i].adrs = 0;
			}
			HelpItemTemp[i].texte = 0;
			HelpMenuTemp->items = HelpItemTemp;
			HelpMenuTemp->nbitems = nbopt;
			HelpMenuTemp->defaut = HelpMenuCalled->defaut;
			strcpy((HelpMenuTemp->cdr)->titre,HelpCdrCalled->titre);
			break;
		  case OPIUM_PANEL:
			nbopt = HelpPanelCalled->cur;
			if(HelpItemTemp) free(HelpItemTemp);
			HelpItemTemp = (MenuItem)malloc((nbopt + 1) * sizeof(TypeMenuItem));
			panelcalled = HelpPanelCalled->items;
			for(i=0; i<nbopt; i++) {
				if(panelcalled[i].type < 0) continue;
				HelpItemTemp[i].texte = panelcalled[i].texte;
				HelpItemTemp[i].sel = 1; /* panelcalled[i].sel; */
				if(panelcalled[i].type == PNL_SEPARE)
					HelpItemTemp[i].type = MNU_CODE_SEPARE;
				else HelpItemTemp[i].type = MNU_CODE_RETOUR;
				HelpItemTemp[i].adrs = 0;
			}
			HelpItemTemp[i].texte = 0;
			HelpMenuTemp->items = HelpItemTemp;
			HelpMenuTemp->nbitems = nbopt;
			HelpMenuTemp->defaut = HelpMenuCalled->defaut;
			strcpy((HelpMenuTemp->cdr)->titre,HelpCdrCalled->titre);
			break;

		  default: return;
		}
	}
}
/* ....................................................................... */
static char HelpFindItem(FILE *f, char *texte){
	size_t l;

	if(texte == 0) return(0);
	l = strlen(texte);
	do {
		if(!fgets(HelpLine,MAXLIGNE,f)) break;
		if(!strncmp(HelpLine,"ITEM",4)) {
			if(!strncmp(HelpLine+5,texte,l)) return(1);
		}
	} while(1);
	return(0);
}
/* ....................................................................... */
static int HelpReadItem(FILE *f) {
	int n;

	n = 0;
	do {
		if(fgets(HelpLine,MAXLIGNE,f) == NULL) break;
		if(!strncmp(HelpLine,"ITEM",4)) break;
/* 		fputs(HelpLine,stdout); */
		TermPrint(HelpTerm,HelpLine); n++;
	} while(1);
	return(n);
}
/* ....................................................................... */
static int HelpMakeItem(FILE * f) {
	int n; size_t l;

	for(n=0; n<MAXPANEL; n++) HelpTexte[n][0] = '\0';
	if(!f) return(0);
	n = 0;
	do {
		if(fgets(HelpLine,MAXLIGNE,f) == NULL) break;
		if(!strncmp(HelpLine,"ITEM",4)) break;
		l = strlen(HelpLine);
		if(HelpLine[l-1] == '\n') HelpLine[l-1] = '\0';
		strcpy(HelpTexte[n++],HelpLine);
	} while(n < MAXPANEL);
	return(n);
}
/* ....................................................................... */
static int HelpWriteItem(FILE *f, int m)  {
	int n;

	n = 0; if(m >= MAXPANEL) m = MAXPANEL;
	while((n < m) /* && HelpTexte[n][0] arrete a la 1ere ligne blanche.. */) {
		fputs(HelpTexte[n++],f); putc('\n',f);
	}
	return(n);
}
#define HELP_COMPL_MAX 80
/* ....................................................................... */
static void HelpBasicItem(Cadre cdr, char numerote, int i,
	char *texte, int *blancs, int *soulignes) {
	MenuItem menucalled; PanelItem panelcalled;
	Menu nextmenu; Panel nextpanel;
	void *adrs;
	char genre[12],complement[HELP_COMPL_MAX];
	int64 v;
//	int v;

	switch(cdr->type) {
	  case OPIUM_MENU:
		menucalled = HelpMenuCalled->items;
		strcpy(genre,(menucalled[i].type == MNU_CODE_SEPARE)? "Sous-menu": "Item");
		*blancs = (int)strlen(genre) + 2;
		if(numerote) {
#ifdef ESC_SEQ
			sprintf(texte,"\n%d.- \x1b[4m%s \"%s\"\x1b[0m: ",i+1,genre,menucalled[i].texte);
#else
			sprintf(texte,"\n%d.- %s \"%s\": ",i+1,genre,menucalled[i].texte);
#endif
			*blancs += (i < 10)? 4: 5;
		} else sprintf(texte,"%s \"%s\": ",genre,menucalled[i].texte);
		*soulignes = (int)strlen(texte) - 4 - *blancs;
		adrs = menucalled[i].adrs;
		switch(menucalled[i].type) {
		  case MNU_CODE_SEPARE:
			break;
		  case MNU_CODE_RETOUR:
			snprintf(complement,HELP_COMPL_MAX,"rend la valeur %d au menu appelant",i+1);
			strcat(texte,complement);
			break;
		  case MNU_CODE_CONSTANTE:
			v = (int64)adrs;
//			v = (int)adrs;
			snprintf(complement,HELP_COMPL_MAX,"rend la valeur %lld au menu appelant",v);
			strcat(texte,complement);
			break;
		  case MNU_CODE_BOOLEEN:
			snprintf(complement,HELP_COMPL_MAX,"inverse un booleen");
			strcat(texte,complement);
			break;
		  case MNU_CODE_FONCTION:
			strcat(texte,"appelle une routine");
			break;
		  case MNU_CODE_COMMANDE:
			strcat(texte,"appelle une commande du shell");
			break;
		  case MNU_CODE_MENU:
			nextmenu = *(Menu *)adrs;
			if((nextmenu->cdr)->titre[0])
				snprintf(complement,HELP_COMPL_MAX,"appelle le menu \"%s\"",(nextmenu->cdr)->titre);
			else snprintf(complement,HELP_COMPL_MAX,"appelle un menu (qui devrait etre nomme \"%s\")",menucalled[i].texte);
			strcat(texte,complement);
			break;
		  case MNU_CODE_PANEL:
			nextpanel = *(Panel *)adrs;
			if((nextpanel->cdr)->titre[0])
				snprintf(complement,HELP_COMPL_MAX,"demande le panel \"%s\"",(nextpanel->cdr)->titre);
			else snprintf(complement,HELP_COMPL_MAX,"demande un panel (qui devrait etre nomme \"%s\")",menucalled[i].texte);
			strcat(texte,complement);
			break;
		}
		break;

	  case OPIUM_PANEL:
		panelcalled = HelpPanelCalled->items;
		strcpy(genre,(panelcalled[i].type == PNL_SEPARE)? "Sous-panel": "Variable");
		*blancs = (int)strlen(genre) + 2;
		if(numerote) {
#ifdef ESC_SEQ
			sprintf(texte,"\n%d.- \x1b[4m%s \"%s\"\x1b[0m: ",i+1,genre,panelcalled[i].texte);
#else
			sprintf(texte,"\n%d.- %s \"%s\": ",i+1,genre,panelcalled[i].texte);
#endif
			*blancs += (i < 10)? 4: 5;
		} else sprintf(texte,"%s \"%s\": ",genre,panelcalled[i].texte);
		*soulignes = (int)strlen(texte) - 4 - *blancs;
		switch(panelcalled[i].type) {
		  case PNL_SEPARE:
			break;
		  case PNL_TEXTE:
			snprintf(complement,HELP_COMPL_MAX,"introduit une chaine (d'au plus %d octets)",panelcalled[i].maxuser);
			strcat(texte,complement);
			break;
		  case PNL_PSWD:
			snprintf(complement,HELP_COMPL_MAX,"introduit un mot de passe (d'au plus %d octets)",panelcalled[i].maxuser);
			strcat(texte,complement);
			break;
		  case PNL_INT:
			strcat(texte,"introduit un entier (32 bits)");
			break;
		  case PNL_SHORT:
			strcat(texte,"introduit un entier court (16 bits)");
			break;
		  case PNL_CHAR:
			strcat(texte,"introduit un entier sur 8 bits");
			break;
		  case PNL_HEX:
			strcat(texte,"introduit un entier en hexadecimal (sur 32 bits)");
			break;
		  case PNL_SHEX:
			strcat(texte,"introduit un entier court en hexadecimal (sur 16 bits)");
			break;
		  case PNL_OCTET:
			strcat(texte,"introduit un entier decimal sur 8 bits");
			break;
		  case PNL_BYTE:
			strcat(texte,"introduit un entier en hexadecimal sur 8 bits");
			break;
		  case PNL_BOOL:
			strcat(texte,"introduit une reponse \"oui\" ou \"non\"");
			break;
		  case PNL_FLOAT:
			strcat(texte,"introduit un reel (32 bits)");
			break;
		  case PNL_DBLE:
			strcat(texte,"introduit un reel double-precision");
			break;
		  case PNL_LISTE: case PNL_LISTL: case PNL_LISTD: case PNL_LISTS: case PNL_LISTB:
			snprintf(complement,HELP_COMPL_MAX,"introduit un mot-cle (d'au plus %d octets)",panelcalled[i].maxuser);
			strcat(texte,complement);
			break;
		  case PNL_KEY: case PNL_KEYL: case PNL_KEYS: case PNL_KEYB:
			snprintf(complement,HELP_COMPL_MAX,"introduit l'un mot-cle de la liste %s",(char *)panelcalled[i].fctn);
			strcat(texte,complement);
			break;
		  case PNL_USER:
			strcat(texte,"introduit une valeur dont le format est specifique a cette application");
			break;
		}
		break;

		default: *blancs = *soulignes = 0; texte[0] = '\0';
	}
}
/* ....................................................................... */
static void HelpSouligneItem(int blancs, int soulignes, char *texte) {
	int i; char *c;

	c = texte;
	for(i=blancs; i; --i) *c++ = ' ';
	for(i=soulignes; i; --i) *c++ = '-';
	*c = '\0';
}
/* ....................................................................... */
static int HelpScan(int change) {
	int i,j,nligs,nbopt;
	int blancs,soulignes;
	char orig[80],copie[80],cde[200],*outnam;
	char texte[256];
	char trouve;
	FILE *f,*fc;
	MenuItem menucalled; PanelItem panelcalled;
	char *titre,*genre;

	genre = OPIUMCDRTYPE(HelpCdrCalled->type);
	titre = HelpCdrCalled->titre;
	HelpSetName(orig);
	strcpy(copie,orig);
	strcat(orig,".help");
	f = fopen(orig,"r");
	if(change) {
		strcat(copie,".temp");
		outnam = copie;
		fc = fopen(outnam,"w");
	} else {
		outnam = HelpOutputName;
		if(HelpLevel) fc = HelpOutputAdrs;
		else fc = fopen(outnam,"w");
	}
	if(!fc) {
		OpiumError("Impossible d'ouvrir le fichier d'aide %s",outnam);
/* 		RefreshMenu(HelpMenuPpal); */
		return(0);
	}
	if(DEBUG_OPIUM(1)) WndPrint("Ficher %s ouvert\n",outnam);
	nbopt =  0; menucalled = 0; /* pour faire taire gcc */
	nligs = HelpMakeItem(f);
	if(change) {
		PanelTitle(HelpPanelPpal,"Vue generale");
		OpiumExec(HelpPanelPpal->cdr);
	} else {
		if(HelpLevel) {
#ifdef ESC_SEQ
			fputs("\x0cCommande: \"\x1b[4m",fc); fflush(fc);
#else
			fputs("\fCommande: \"",fc); fflush(fc);
#endif
			for(j=0; j<HelpLevel; j++) {
				if(j) putc('/',fc);
				fputs(HelpCommand[j],fc);
			}
#ifdef ESC_SEQ
			fputs("\x1b[0m\"\n\n%c%s appele: ",toupper(genre[0]),genre+1,fc);
#else
			fputs("\"\n",fc);
			for(i=0; i<80; i++) putc('-',fc);
			fprintf(fc,"\n%c%s appele: ",toupper(genre[0]),genre+1);
			blancs = (int)strlen(genre) + 9;
#endif
		} else blancs = 0;
#ifdef ESC_SEQ
		fprintf(fc,"\x1b[4m%s\x1b[0m\n",titre);
		fprintf(fc,"\n0.- \x1b[4mVue generale\x1b[0m:\n");
#else
		fprintf(fc,"%s\n",titre);
		soulignes = (int)strlen(titre);
		HelpSouligneItem(blancs,soulignes,texte);
		fputs(texte,fc); putc('\n',fc);

		fprintf(fc,"\n0.- Vue generale:\n");
		HelpSouligneItem(4,12,texte);
		fputs(texte,fc); putc('\n',fc);
#endif
	}
	HelpWriteItem(fc,nligs);

	switch(HelpCdrCalled->type) {

	  case OPIUM_MENU:
		nbopt = HelpMenuCalled->nbitems;
		menucalled = HelpMenuCalled->items;
		for(i=0; i<nbopt; i++) {
			if(!(menucalled[i].texte[0])) continue;
			trouve = 0;
			if(f && !(trouve = HelpFindItem(f,menucalled[i].texte))) {
				rewind(f);
				trouve = HelpFindItem(f,menucalled[i].texte);
			}
			if(f && !trouve) nligs = -1; else nligs = HelpMakeItem(f);
			if(change) {
				PanelTitle(HelpPanelPpal,menucalled[i].texte);
				OpiumExec(HelpPanelPpal->cdr);
				fprintf(fc,"ITEM %s\n",menucalled[i].texte);
				/* valeur de nligs ??? */
			} else {
				HelpBasicItem(HelpCdrCalled,1,i,texte,&blancs,&soulignes);
				fputs(texte,fc); putc('\n',fc);
				HelpSouligneItem(blancs,soulignes,texte);
				fputs(texte,fc); putc('\n',fc);
			}
			if(nligs >= 0) {
				if(!HelpWriteItem(fc,nligs)) fprintf(fc,"(Pas d'aide pour le moment)\n");
			} else fprintf(fc,"(Item nouveau, non documente)\n");
		}
		break;

	  case OPIUM_PANEL:
		nbopt = HelpPanelCalled->cur;
		panelcalled = HelpPanelCalled->items;
		for(i=0; i<nbopt; i++) {
			if(panelcalled[i].type < 0) continue;
			trouve = 0;
			if(f && !(trouve = HelpFindItem(f,panelcalled[i].texte))) {
				rewind(f);
				trouve = HelpFindItem(f,panelcalled[i].texte);
			}
			if(f && !trouve) nligs = -1; else nligs = HelpMakeItem(f);
			if(change) {
				PanelTitle(HelpPanelPpal,panelcalled[i].texte);
				OpiumExec(HelpPanelPpal->cdr);
				fprintf(fc,"ITEM %s\n",panelcalled[i].texte);
				/* valeur de nligs ??? */
			} else {
				HelpBasicItem(HelpCdrCalled,1,i,texte,&blancs,&soulignes);
				fputs(texte,fc); putc('\n',fc);
				HelpSouligneItem(blancs,soulignes,texte);
				fputs(texte,fc); putc('\n',fc);
			}
			if(nligs >= 0) {
				if(!HelpWriteItem(fc,nligs)) fprintf(fc,"(Pas d'aide pour le moment)\n");
			} else fprintf(fc,"(Variable nouvelle, non documentee)\n");
		}
		break;
	}
	if(f) fclose(f);
	fflush(fc);

	if(change) {
		fclose(fc);
		if(OpiumCheck("Sauver ces modifications")) {
#ifdef OS9
			sprintf(cde,"copy %s %s -r",copie,orig);
#endif
#ifdef UNIX
			sprintf(cde,"cp %s %s",copie,orig);
#endif
			i = system(cde);
			if(i) OpiumError("Error %d: %s",i,strerror(i));
		} 
	} else {
		if(!HelpLevel) HelpOutputAdrs = fc;
		if(HelpRecurs && (HelpCdrCalled->type == OPIUM_MENU)) {
			HelpLevel++;
			for(i=0; i<nbopt; i++) if(menucalled[i].type == MNU_CODE_MENU) {
				HelpCommand[HelpLevel-1] = menucalled[i].texte;
				HelpMenuCalled = *(Menu *)menucalled[i].adrs;
				HelpCdrCalled = HelpMenuCalled->cdr;
				if(HelpCdrCalled->nom[0] == '\0') {
					OpiumTitle(HelpCdrCalled,menucalled[i].texte);
				}
				HelpScan(0);
			}
			--HelpLevel;
		}
		if(!HelpLevel) fclose(HelpOutputAdrs);
	}
/* 	if(!HelpLevel) RefreshMenu(HelpMenuPpal); */
	return(0);
}
/* ....................................................................... */
static void HelpPrintVueGenerale(FILE *f, char *genre, char *titre) {
	rewind(f);
/* 	TermPrint(HelpTerm,"			\x1b[4mMenu\x1b[0m: \"\x1b[1m%s\x1b[0m\"",HelpMenuCalled->titre); */
	TermPrint(HelpTerm,"%c%s: \"%s\"",toupper(genre[0]),genre+1,titre);
	TermPrint(HelpTerm,"\n\n");
	if(!HelpReadItem(f)) TermPrint(HelpTerm,"Pas de vue generale pour ce %s.\n",genre);
	TermPrint(HelpTerm,"                              --------------------\n\n");
}
/* ....................................................................... */
static void HelpPrintItem(FILE *f, int i) {
	char texte[256];
	char *titre;
	int blancs,soulignes;

/* 	TermPrint(HelpTerm,"			\x1b[4mOption\x1b[0m \"\x1b[1m%s\x1b[0m\": ",HelpItemTemp[i].texte); */
	titre = 0;
	HelpBasicItem(HelpCdrCalled,0,i,texte,&blancs,&soulignes);
	TermPrint(HelpTerm,"%s\n\n",texte);
	switch(HelpCdrCalled->type) {
	  case OPIUM_MENU: titre = HelpMenuCalled->items[i].texte; break;
	  case OPIUM_PANEL: titre = HelpPanelCalled->items[i].texte; break;
	  default:
		OpiumError("Aide sur les items %s%s: pas implementee",
			OpiumDuDela(HelpCdrCalled),OPIUMCDRTYPE(HelpCdrCalled->type));
	}
	if(HelpFindItem(f,titre)) HelpReadItem(f);
	else {
		rewind(f);
		if(HelpFindItem(f,titre)) HelpReadItem(f);
		else TermPrint(HelpTerm,"Pas d'aide particuliere.\n");
	}
	TermPrint(HelpTerm,"                              --------------------\n\n");
}
/* ======================================================================= */
static int HelpRead(Menu menu, MenuItem item) {
	int i,t;
	char nom[40];
	FILE *f;
	char *genre,*titre;

	genre = OPIUMCDRTYPE(HelpCdrCalled->type);
	titre = HelpCdrCalled->titre;
	HelpSetName(nom);
	strcat(nom,".help");
	f = fopen(nom,"r");
	if(!f) {
		if(titre[0]) OpiumError("Pas d'aide pour le %s \"%s\"",genre,titre);
		else OpiumError("Pas d'aide pour ce %s",genre);
		return(0);
	}

	HelpCreateMenuTempo();
	do {
		if((t = OpiumExec(HelpMenuType->cdr)) >= 4) break;
		if(!OpiumDisplayed(HelpTerm->cdr)) TermEmpty(HelpTerm);
		TermHold(HelpTerm);
		switch(t) {
		  case 1:
		  	HelpPrintVueGenerale(f,genre,titre);
			break;
		  case 2:
			HelpPrintItem(f,OpiumExec(HelpMenuTemp->cdr) - 1);
			break;
		  case 3:
		  	HelpPrintVueGenerale(f,genre,titre);
			for(i=0; i<HelpMenuTemp->nbitems; i++) HelpPrintItem(f,i);
			break;
		}
		TermRelease(HelpTerm);
		if(!OpiumDisplayed(HelpTerm->cdr)) OpiumDisplay(HelpTerm->cdr);
	} while(1);

	fclose(f);
	if(OpiumDisplayed(HelpTerm->cdr)) OpiumClear(HelpTerm->cdr);
/* 	if(HelpModify) RefreshMenu(HelpMenuPpal); */
	return(0);
}
/* ======================================================================= */
static int HelpChange(Menu menu, MenuItem item) { HelpLevel = 0; return(HelpScan(1)); }
/* ======================================================================= */
static int HelpWrite(Menu menu, MenuItem item) {
	PanelItemSelect(HelpPanelWrite,HelpItemRecurs,(HelpCdrCalled->type == OPIUM_MENU));
	OpiumExec(HelpPanelWrite->cdr);
	HelpLevel = 0;
	return(HelpScan(0));
}

