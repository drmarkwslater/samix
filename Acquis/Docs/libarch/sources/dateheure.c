#include <stdio.h>

#include <dateheure.h>
#ifdef __MWERKS__
#include <Timer.h>
#include <string.h>
#else
#include <sys/time.h>    /* pour gettimeofday */
#include <strings.h>
#endif

#include <claps.h>

static char DateHeureTexte[12];
static char DateJourTexte[12];
static char DateTexteCompact[12];

/* ========================================================================== */
char *DateHeure(void) {
	time_t temps;
	struct tm date;

	time(&temps);
	memcpy(&date,localtime(&temps),sizeof(struct tm));
	sprintf(DateHeureTexte,"%02d:%02d:%02d",date.tm_hour,date.tm_min,date.tm_sec);
	return(DateHeureTexte);
}
/* ========================================================================== */
char *DateCivile(void) {
	time_t temps;
	struct tm date;

	time(&temps);
	memcpy(&date,localtime(&temps),sizeof(struct tm));
	sprintf(DateJourTexte,"%02d.%02d.%02d",date.tm_mday,date.tm_mon+1,date.tm_year % 100);
	return(DateJourTexte);
}
/* ========================================================================== */
char *DateJour(void) {
	time_t temps;
	struct tm date;
	
	time(&temps);
	memcpy(&date,localtime(&temps),sizeof(struct tm));
	sprintf(DateJourTexte,"%02d.%02d.%02d",date.tm_year % 100,date.tm_mon+1,date.tm_mday);
	return(DateJourTexte);
}
/* ========================================================================== */
int DateHeureInt(void) {
	time_t temps;
	struct tm date;
	
	time(&temps);
	memcpy(&date,localtime(&temps),sizeof(struct tm));
	return((((date.tm_hour * 100) + date.tm_min) * 100) + date.tm_sec);
}
/* ========================================================================== */
int DateJourInt(void) {
	time_t temps;
	struct tm date;

	time(&temps);
	memcpy(&date,localtime(&temps),sizeof(struct tm));
	return(((((date.tm_year % 100) * 100) + date.tm_mon + 1) * 100) + date.tm_mday);
}
/* ========================================================================== */
char *DateCompacte(void) {
    time_t date; struct tm m;
    
    time(&date); memcpy(&m,localtime(&date),sizeof(struct tm));
    sprintf(DateTexteCompact,"%c%c%02d%c%03x",
        'a'+((m.tm_year % 100) % 26),'a'+m.tm_mon,m.tm_mday,'a'+m.tm_hour,(m.tm_min * 60)+m.tm_sec);
    return(DateTexteCompact);
}
/* ========================================================================== */
int64 DateLong(void) {
	time_t temps;
	struct tm date;
//	int64 d;

	time(&temps);
	memcpy(&date,localtime(&temps),sizeof(struct tm));
//	d = ((((((((((((int64)date.tm_year % 100) * 100) + (int64)date.tm_mon + 1) * 100) + (int64)date.tm_mday) * 100) + (int64)date.tm_hour) * 100) + (int64)date.tm_min) * 100) + (int64)date.tm_sec);
//	printf("(%s) Date: %04d.%02d.%02d %02dh%02d' %02d\" = %lld\n",__func__,date.tm_year%100,date.tm_mon + 1,date.tm_mday,date.tm_hour,date.tm_min,date.tm_sec,d);
	return((((((((((((int64)date.tm_year % 100) * 100) + (int64)date.tm_mon + 1) * 100) + (int64)date.tm_mday) * 100)
		+ (int64)date.tm_hour) * 100) + (int64)date.tm_min) * 100) + (int64)date.tm_sec);
}
/* ========================================================================== 
int64 DateJourHeureToLong(int jour, int heure) {
	return((((int64)jour) * 1000000) + (int64)heure);
}
   ========================================================================== */
void DateLongPrint(char *texte, int64 date) {
	int64 cc,aa,mm,jj,hh,mn,ss;
	
	mn = date / 100; ss = date - (mn * 100);
	hh = mn / 100;   mn = mn - (hh * 100);
	jj = hh / 100;   hh = hh - (jj * 100);
	mm = jj / 100;   jj = jj - (mm * 100);
	aa = mm / 100;   mm = mm - (aa * 100);
	cc = aa / 100;   aa = aa - (cc * 100);
	sprintf(texte,"%02lld.%02lld.%02lld a %02lld:%02lld:%02lld",jj,mm,aa,hh,mn,ss);
}
/* ========================================================================== */
void DateIntToHeure(char *texte, int entier) {
	int hh,mn,ss;

	mn = entier / 100; ss = entier - (mn * 100);
	hh = mn / 100;     mn = mn - (hh * 100);
	sprintf(texte,"%02d:%02d:%02d",hh,mn,ss);
}
/* ========================================================================== */
void DateIntToJour(char *texte, int entier) {
	int cc,aa,mm,jj;
	
	mm = entier / 100; jj = entier - (mm * 100);
	aa = mm / 100;     mm = mm - (aa * 100);
	cc = aa / 100;     aa = aa - (cc * 100);
	sprintf(texte,"%02d.%02d.%02d",jj,mm,aa);
}
/* ========================================================================== */
time_t DateLongToTime(int64 d) {
	struct tm date;
	int64 d2,d3; int c;

	d2 = d;
	d3 = d2 / 100; c = (int)(d2 - (d3 * 100)); d2 = d3; date.tm_sec = c;
	d3 = d2 / 100; c = (int)(d2 - (d3 * 100)); d2 = d3; date.tm_min = c;
	d3 = d2 / 100; c = (int)(d2 - (d3 * 100)); d2 = d3; date.tm_hour = c;
	d3 = d2 / 100; c = (int)(d2 - (d3 * 100)); d2 = d3; date.tm_mday = c;
	d3 = d2 / 100; c = (int)(d2 - (d3 * 100)); d2 = d3; date.tm_mon = c - 1;
	d3 = d2 / 100; c = (int)(d2 - (d3 * 100)); d2 = d3; date.tm_year = c + 100;
//	printf("Date utilisee   : %lld\n",d);
//	printf("Date a convertir: %02d.%02d.%02d %02d:%02d\n",date.tm_mday,date.tm_mon+1,date.tm_year+1900,date.tm_hour,date.tm_min);
//	{ time_t temps;
//		temps = mktime(&date);
//		printf("Temps utilise   : %ld\n",temps);
//		memcpy(&date,localtime(&temps),sizeof(struct tm));
// resultat: 1 heure de moins!!
//		printf("Date recuperee  : %02d.%02d.%02d %02d:%02d\n",date.tm_mday,date.tm_mon+1,date.tm_year+1900,date.tm_hour,date.tm_min);
//	}
	return(mktime(&date));
}
/* ========================================================================== */
int DateHeureToInt(char *texte) {
	char chaine[DATE_MAX];
	char *c,*d;
	int i,k,l;
	short ss,mn,hh;

	if(*texte == '\0') {
		time_t temps; struct tm date;
		time(&temps);
		memcpy(&date,localtime(&temps),sizeof(struct tm));
		return((((date.tm_hour * 100) + date.tm_min) * 100) + date.tm_sec);
	}
	// printf("(%s) decodage de la date '%s'\n",__func__,texte);
	l = strlen(texte); if(l > (DATE_MAX-1)) l = (DATE_MAX-1);
	if(l == 0) return(0);
	strncpy(chaine,texte,l);
	chaine[l] = '\0';
	i = 0; ss = mn = hh = 0;
	d = c = chaine;
	while(*c) {
		if((*c == ' ') && (d != chaine)) break;
		if((*c == '.') || (*c == '/') || (*c == '-'))  *c = ':';
		if(*c == ':') {
			*c = '\0';
			// printf("(%s) element de date #%d: %s\n",__func__,i+1,d);
			if(c > d) {
				k = 0;
				sscanf(d,"%d",&k);
				// printf("(%s) valeur de date  #%d: %d\n",__func__,i+1,k);
				switch(i++) {
					case 0: hh = k; break;
					case 1: mn = k; break;
					case 2: ss = k; break;
				}
			} else i++;
			d = c + 1;
		}
		if(i >= 3) break;
		c++;
	}
	if((i < 3) && (c > d)) {
		k = 0;
		// printf("(%s) element de date #%d: %s\n",__func__,i+1,d);
		sscanf(d,"%d",&k);
		// printf("(%s) valeur de date   #%d: %d\n",__func__,i+1,k);
		switch(i) {
			case 0: hh = k; break;
			case 1: mn = k; break;
			case 2: ss = k; break;
		}
	}
	return((((hh * 100) + mn) * 100) + ss);
}
/* ========================================================================== */
int DateJourToInt(char *texte) {
	char chaine[DATE_MAX];
	char *c,*d;
	int i,k,l;
	short jj,mm,aa,tmp;
	char *mois="janv/fev/mars/avr/mai/juin/juil/aout/sept/oct/nov/dec";
	//1 char dbg = 0;

	if(*texte == '\0') {
		time_t temps; struct tm date;
		time(&temps);
		memcpy(&date,localtime(&temps),sizeof(struct tm));
		return(((((date.tm_year % 100) * 100) + date.tm_mon+1) * 100) + date.tm_mday);
	}
	// printf("(%s) decodage de la date '%s'\n",__func__,texte);
	l = strlen(texte); if(l > (DATE_MAX-1)) l = (DATE_MAX-1);
	if(l == 0) return(0);
	strncpy(chaine,texte,l);
	chaine[l] = '\0';
	i = 0; jj = mm = aa = 0;
	d = c = chaine;
	while(*c) {
		//1 if((*c == '-') && !dbg) { printf("Date a decoder: [%s]\n",texte); dbg = 1; }
		if((*c == ' ') || (*c == '/') || (*c == '-'))  *c = '.';
		if(*c == '.') {
			*c = '\0';
			// printf("(%s) element de date #%d: %s\n",__func__,i+1,d);
			if(c > d) {
				k = 0;
				sscanf(d,"%d",&k);
				// printf("(%s) valeur de date primaire  #%d: %d\n",__func__,i+1,k);
				if(!k && (i == 1)) k = ArgKeyGetIndex(mois,d) + 1;
				// printf("(%s) valeur de date definitif #%d: %d\n",__func__,i+1,k);
				switch(i++) {
				  case 0: jj = k; break;
				  case 1: mm = k; break;
				  case 2: aa = k; break;
				}
			} else i++;
			d = c + 1;
		}
		if(i >= 3) break;
		c++;
	}
	//1 if(dbg) printf("v[%d] = %d-%d-%d\n",i,jj,mm,aa);
	if(!i) {
		if(l == 8) { sscanf(chaine,"%4hd%2hd%2hd",&aa,&mm,&jj); i = 3; }
		else { sscanf(chaine,"%2hd%2hd%2hd",&aa,&mm,&jj); i = ((l - 1) / 2) + 1; }
	}
	if((i < 3) && (c > d)) {
		k = 0;
		// printf("(%s) element de date #%d: %s\n",__func__,i+1,d);
		sscanf(d,"%d",&k);
		// printf("(%s) valeur de date primaire  #%d: %d\n",__func__,i+1,k);
		if(!k && (i == 1)) k = ArgKeyGetIndex(mois,d) + 1;
		// printf("(%s) valeur de date definitif #%d: %d\n",__func__,i+1,k);
		switch(i) {
		  case 0: jj = k; break;
		  case 1: mm = k; break;
		  case 2: aa = k; break;
		}
	}
	if(jj >= 40) { tmp = aa; aa = jj; jj = tmp; } /* permet de taper un 3 sur 29 */
	//1 if(dbg) printf("m[%d] = %d-%d-%d\n",i,jj,mm,aa);
	if(aa > 1900) aa = aa % 100;
	if((jj > 0) && (mm == 0) && (aa == 0)) mm = aa = 1; /* pour permettre la saisie a partir de date == 0 */
	else if(aa == 0) {
		time_t temps; struct tm date;
		time(&temps); memcpy(&date,localtime(&temps),sizeof(struct tm));
		aa = (date.tm_year % 100);
	}
	/* aider plutot que gueuler */
	if(mm < 1) mm = 1;
	if(mm > 12) mm = 10;
	if(jj < 1) jj = 1;
	if(jj > 31) jj = NbJoursDuMois(mm,aa);
	if((aa < 0) || (aa > 99) /* || (mm < 1) || (mm > 12) || (jj < 1) || (jj > 31) */) 
		return(0);
	else return((((aa * 100) + mm) * 100) + jj);
}
/* ========================================================================== */
char *DateCode(char *texte, int *date, int *col) {
	int k;
	
	if(texte) {
		if((k = DateJourToInt(texte))) {
			if(((*col + 1) % 3) == 0) *col += 1;
			*date = k;
			return((char *)1);
		} else return(0);
	} else {
		DateIntToJour(DateJourTexte,*date); return(DateJourTexte);
	}
}
/* ========================================================================== */
int DateIntToJours(int jour) {
	int cc,aa,mm,jj; int an,nb;
	
	mm = jour / 100; jj = jour - (mm * 100);
	aa = mm / 100;   mm = mm - (aa * 100);
	cc = aa / 100;   aa = aa - (cc * 100);
	// return((((aa * 12) + mm) * 30.4375) + jj); // mois moyen: 365.25 / 12.0 = 30.4375 jours
	nb = 0;
	for(an=16; an<aa; an++) nb += (an % 4)? 365: 366;
	nb += jj;
	if(mm >  1) nb += 31; else return(nb);
	if(mm >  2) nb += (aa % 4)? 28: 29; else return(nb);
	if(mm >  3) nb += 31; else return(nb);
	if(mm >  4) nb += 30; else return(nb);
	if(mm >  5) nb += 31; else return(nb);
	if(mm >  6) nb += 30; else return(nb);
	if(mm >  7) nb += 31; else return(nb);
	if(mm >  8) nb += 31; else return(nb);
	if(mm >  9) nb += 30; else return(nb);
	if(mm > 10) nb += 31; else return(nb);
	if(mm > 11) nb += 30;
	return(nb);
}
/* ========================================================================== */
int DateIntToSecondes(int heure) {
	int hh,mn,ss;
	
	mn = heure / 100; ss = heure - (mn * 100);
	hh = mn / 100;    mn = mn - (hh * 100);
	return((((hh * 60) + mn) * 60) + ss);
}
/* ========================================================================== */
int64 DateLongToSecondes(int64 date) {
	int jour,heure; int jours,secondes;

	jour = (int)(date / 1000000); heure = (int)(date - (jour * 1000000));
	jours = DateIntToJours(jour); secondes = DateIntToSecondes(heure);
	return((jours * 86400) + secondes);
}
/* ========================================================================== */
int DateSecondesToInt(int secs) {
	int hh,mn,ss;
	
	mn = secs / 60; ss = secs - (mn * 60);
	hh = mn / 60;   mn = mn - (hh * 60);
	return((((hh * 100) + mn) * 100) + ss);
}
/* ========================================================================== */
INLINE int64 DateMicroSecs() {
	int64 total;
#ifdef CODE_WARRIOR_VSN
	Microseconds((UnsignedWide *)(&total));
#else
	struct timeval temps;
	struct timezone zone;
	gettimeofday(&temps,&zone);
	total = ((int64)temps.tv_sec * 1000000) + (int64)temps.tv_usec;
#endif
	return(total);
}
/* ========================================================================== */
void S_usPrint(char *texte, int s, int us) {
	if(s < 10) {
		if(us) sprintf(texte,"%d,%06d s",s,us);
		else sprintf(texte,"%d s",s);
	} else if(s < 100) {
		if(us) sprintf(texte,"%d,%03d s",s,us/1000);
		else sprintf(texte,"%d s",s);
	} else {
		int jj,hh,mn,ss;
		mn = s / 60; ss = s - (mn * 60); hh = mn / 60;
		if(hh) {
			mn = mn - (hh * 60); jj = hh / 24;
			if(jj) {
				hh = hh - (jj * 24);
				sprintf(texte,"%dj %dh%02d'%02d\"",jj,hh,mn,ss);
			} else sprintf(texte,"%dh%02d' %02d\"",hh,mn,ss);
		} else {
			if(us) sprintf(texte,"%d' %02d,%03d s",mn,ss,us/1000);
			else sprintf(texte,"%d' %02d s",mn,ss);
		}
	}
}
/* ========================================================================== */

hex64 S_usFromInt(int s, int us) {
	TypeS_us resul;
	resul.t.s = s; resul.t.us = us;
	return(resul.s_us);
}
/* ========================================================================== */

hex64 S_usFromFloat(float f) {
	TypeS_us resul;

	resul.t.s = (int)f;
	resul.t.us = (int)((f - (float)resul.t.s) * 1000000.0);
	return(resul.s_us);
}
/* ========================================================================== */

hex64 S_usOper(S_us t0, char oper, S_us t1) {
	TypeS_us resul;
	
	if(oper == S_US_PLUS) { resul.t.s = t0->t.s + t1->t.s; resul.t.us = t0->t.us + t1->t.us; }
	else { resul.t.s = t0->t.s - t1->t.s; resul.t.us = t0->t.us - t1->t.us; }
	while(resul.t.us >= 1000000) { resul.t.s += 1; resul.t.us -= 1000000; }
	while(resul.t.us < 0) { resul.t.s -= 1; resul.t.us += 1000000; }

	return(resul.s_us);
}
/* ========================================================================== */

hex64 S_nsFromInt(int s, int ns) {
	TypeS_ns resul;
	resul.t.s = s; resul.t.us = ns;
	return(resul.s_us);
}
/* ========================================================================== */

hex64 S_nsFromFloat(float f) {
	TypeS_ns resul;

	resul.t.s = (int)f;
	resul.t.us = (int)((f - (float)resul.t.s) * 1000000000.0);
	return(resul.s_us);
}
/* ========================================================================== */

hex64 S_nsOper(S_ns t0, char oper, S_ns t1) {
	TypeS_ns resul;

	if(oper == S_US_PLUS) { resul.t.s = t0->t.s + t1->t.s; resul.t.us = t0->t.us + t1->t.us; }
	else { resul.t.s = t0->t.s - t1->t.s; resul.t.us = t0->t.us - t1->t.us; }
	while(resul.t.us >= 1000000000) { resul.t.s += 1; resul.t.us -= 1000000000; }
	while(resul.t.us < 0) { resul.t.s -= 1; resul.t.us += 1000000000; }

	return(resul.s_us);
}
/* ========================================================================== */
// INLINE float S_usFloat(S_us t) { return((float)t->t.s + ((float)t->t.us / 1000000.0)); }
/* ========================================================================== */
//	TypeS_us t; t.s_us = s_us; return((float)t->t.s + ((float)t->t.us / 1000000.0));
// INLINE float S_usFloat_us(hex64 s_us) {
// 	return((float)(((TypeS_us)s_us).t.s) + ((float)(((TypeS_us)s_us).t.us) / 1000000.0));
// }
#ifdef macintosh
/* ========================================================================== */

int64 DateSinceBoot(int *hh, int *mn, int *ss, long *usec) {
#ifdef CODE_WARRIOR_VSN_JAMAIS_UTILISE
	int64 total; int secondes;
	
	Microseconds((UnsignedWide *)(&total));
	secondes = total / 1000000;
	*mn = secondes / 60;
	*ss = secondes - (*mn * 60);
	*hh = *mn / 60;
	*mn -= (*hh * 60);
	*usec = total - (secondes * 1000000);
	return(total);
#else
	return(0);
#endif
}
#endif
