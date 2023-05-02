#include <stdlib.h> /* pour malloc et free */
#include <stdio.h>
#include <stdarg.h>
#include <strings.h>  /* pour strlen qui rend size_t si intrinseque */
#include <string.h>

#ifdef Linux
// extern int vasprintf (char **__restrict __ptr, const char *__restrict __f,_G_va_list __arg)__THROWNL __attribute__ ((__format__ (__printf__, 2, 0))) __wur;
	extern int vasprintf (char **texte, const char *fmt, va_list arg);
#endif

#include <opium.h>

/* ========================================================================== */
Info InfoCreate(int lignes, int colonnes) {
	Cadre cdr; Info info;
	int i,k;

	info = (Info)malloc(sizeof(TypeInfo));
	if(info) {
		if(DEBUG_INFO(1)) WndPrint("Info cree @%08lX\n",(Ulong)info);
		cdr = OpiumCreate(OPIUM_INFO,info);
		if(!cdr) {
			free(info); return(0); 
		}
		info->texte = (char *)malloc(lignes * colonnes);
		if(!info->texte) {
			OpiumDelete(cdr); free(info); return(0); 
		}
		info->cdr = cdr;
		info->haut = lignes;
		info->larg = colonnes;
		for(i=0,k=0; i<lignes; i++,k+=info->larg) info->texte[k] = '\0';
		if(DEBUG_INFO(3)) WndPrint("Utilise cdr @%08lX\n",(Ulong)info->cdr);
	}
	return(info);
}
/* ========================================================================== */
void InfoDelete(Info info) {
	free(info->texte);
	OpiumDelete(info->cdr);
	free(info);
}
/* ========================================================================== */
void InfoErase(Info info) {
	int i,k;
	for(i=0,k=0; i<info->haut; i++,k+=info->larg) info->texte[k] = '\0';
}
/* ========================================================================== */
void InfoTitle(Info info,char *texte)  {
	OpiumTitle(info->cdr,texte); 
}
/* ========================================================================== */
int InfoWrite(Info info, int ligne, char *fmt, ...) {
	char *texte; char *utilise; int k,l;
	va_list argptr; int cnt;

	if(!info) return(0);
	if((ligne <= 0) || (ligne > info->haut)) return(0);
	l = ligne - 1;
	k = info->larg * l;
	texte = info->texte + k;

	utilise = fmt; // L_(fmt);
	va_start(argptr, fmt);
	cnt = vsprintf(texte, utilise, argptr);
	va_end(argptr);

	if(DEBUG_INFO(3)) WndPrint("Ligne %d @%d: {%s}\n",ligne,k,info->texte+k);
	if(OpiumLastRefreshed == info->cdr) OpiumLastRefreshed = 0;
	return(1);
}
/* ========================================================================== */
int InfoInsert(Info info, int ligne, int col, char *fmt, ...) {
	char *texte; char *utilise; int k,l;
	va_list argptr; int cnt;

	if(!info) return(0);
	if((ligne <= 0) || (ligne > info->haut)) return(0);
	l = ligne - 1;
	k = info->larg * l;
	texte = info->texte + k + col;

	utilise = fmt; // L_(fmt);
	va_start(argptr, fmt);
	cnt = vsprintf(texte, utilise, argptr);
	va_end(argptr);

	if(DEBUG_INFO(3)) WndPrint("Ligne %d @%d: {%s}\n",ligne,k,info->texte+k);
	if(OpiumLastRefreshed == info->cdr) OpiumLastRefreshed = 0;
	return(1);
}
/* ==========================================================================
int InfoDeleteOnClear(Cadre cdr, void *arg) {
	InfoDelete((Info)cdr->adrs);
	return(0);
}
   ========================================================================== */
Info InfoNotifie(Info info, char *fmt, ...) {
	char *texte; char *utilise;
	va_list argptr; int cnt;

	utilise = fmt; // L_(fmt);
	va_start(argptr, fmt);
	cnt = vasprintf(&texte, utilise, argptr);
	va_end(argptr);

	if(info) {
		if(OpiumAlEcran(info)) OpiumClear(info->cdr);
	} else {
		info = InfoCreate(1,127); // strlen(texte)
		OpiumTitle(info->cdr,"Notification");
	}
	strncpy(info->texte,texte,127); free(texte);
	// OpiumClearFctn(info->cdr,InfoDeleteOnClear,0);
	OpiumDisplay(info->cdr);
	return(info);
}
/* ==========================================================================
int InfoFloat(Info info, int ligne, char *fmt,
float p0,float p1,float p2,float p3,float p4,float p5,float p6,float p7) {
	int k,l;

	if(!info) return(0);
	if((ligne <= 0) || (ligne > info->haut)) return(0);
	l = ligne - 1;
	k = info->larg * l;
	sprintf(info->texte+k,fmt,p0,p1,p2,p3,p4,p5,p6,p7);
	if(DEBUG_INFO(3)) WndPrint("Ligne %d @%d: {%s}\n",ligne,k,info->texte+k);
	return(1);
}
   ========================================================================== 
int InfoDouble(Info info, int ligne, char *fmt,
double p0,double p1,double p2,double p3,double p4,double p5,double p6,double p7) {
	int k,l;

	if(!info) return(0);
	if((ligne <= 0) || (ligne > info->haut)) return(0);
	l = ligne - 1;
	k = info->larg * l;
	sprintf(info->texte+k,fmt,p0,p1,p2,p3,p4,p5,p6,p7);
	if(DEBUG_INFO(3)) WndPrint("Ligne %d @%d: {%s}\n",ligne,k,info->texte+k);
	return(1);
}
   ========================================================================== */
int OpiumSizeInfo(Cadre cdr, char from_wm) {
	Info info;
	int i,k,larg,haut; size_t l;

	if(DEBUG_OPIUM(1) && (cdr->type != OPIUM_INFO)) {
		WndPrint("+++ OpiumSizeInfo sur le %s @%08lX\n",cdr->nom,(Ulong)cdr->adrs);
		return(0);
	}
	if(from_wm) {
#ifdef OPIUM_AJUSTE_WND
		larg = WndPixToCol(cdr->dh);
		cdr->dh = WndColToPix(larg);
		haut = WndPixToLig(cdr->dv);
		cdr->dv = WndLigToPix(haut);
#endif
		return(1);
	}
	info = (Info)cdr->adrs;
	haut = larg = 0;
	for(i=0,k=0; i<info->haut; i++,k+=info->larg) {
		if(info->texte[k]) {
			haut = i;
			l = strlen(info->texte + k);
			if(DEBUG_INFO(1)) WndPrint("(OpiumSizeInfo) Texte[%ld] en ligne %d\n",l,i);
			if(larg < l) larg = (int)l;
		}
	}
	haut++;
	if(DEBUG_INFO(1)) WndPrint("Info %d x %d\n",larg,haut);
	if(larg < 1) larg = 1; if(haut < 1) haut = 1;
	cdr->larg = WndColToPix(larg); cdr->haut = WndLigToPix(haut);
	cdr->dh = cdr->larg; cdr->dv = cdr->haut;
	return(1);
}
/* ========================================================================== */
int OpiumRefreshInfo(Cadre cdr) {
	Info info;
	WndFrame f;
	int i,k;

	if(DEBUG_OPIUM(1) && (cdr->type != OPIUM_INFO)) {
		WndPrint("+++ OpiumRefreshInfo sur le %s @%08lX\n",cdr->nom,(Ulong)cdr->adrs);
		return(0);
	}
	if(!OpiumCoordFenetre(cdr,&f)) return(0);
	info = (Info)cdr->adrs;
	for(i=0,k=0; i<info->haut; i++,k+=info->larg)
		WndWrite(f,WND_TEXT,i,0,info->texte+k);
	return(1);
}
/* ========================================================================== */
int OpiumRunInfo(Cadre cdr, WndAction *e) {
	Info info;
	int lig;
	int code_rendu;
	char return_done;
	size_t l,m;

	if(DEBUG_OPIUM(1) && (cdr->type != OPIUM_INFO)) {
		WndPrint("+++ OpiumRunInfo sur le %s @%08lX\n",cdr->nom,(Ulong)cdr->adrs);
		return(0);
	}
	if(cdr->planche) return(-1);
	info = (Info)cdr->adrs;
	lig = WndPixToLig(e->y);
	if(DEBUG_INFO(1)) WndPrint("Info @%08lX, ligne %d\n",(Ulong)info,lig);
	code_rendu = -1;
	if(e->type == WND_KEY) {
		l = strlen(e->texte);
		if(DEBUG_INFO(2)) {
			WndPrint("%04lX ( ",e->code);
			for(m=0; m<l; m++) WndPrint("%02X ",e->texte[m]); WndPrint(")\n");
		}
		if(l <= 0) {
			if(e->code == XK_KP_F4) code_rendu = 0;
		} else {
			return_done = (e->texte[l - 1] == 0x0D);
			if(return_done) code_rendu = 1;
		}
	} else if(e->type == WND_RELEASE) switch(e->code) {
		case WND_MSELEFT:
		code_rendu = 1;
		break;
		default:
/* autres entrees souris dans une info: pas d'action */
		break;
	}

	return(code_rendu);
}

