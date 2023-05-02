#ifndef DATEHEURE_H
#define DATEHEURE_H

#include <time.h>

#include "environnement.h"

#include <utils/defineVAR.h>

#define DATE_MAX 32

/* attention: mois va de 1 a 12 */
#define NbJoursDuMois(mois,an) (((mois) < 8)? (((mois) % 2)? 31: (((mois) == 2)? (((an) % 4)? 28:29): 30)): (((mois) % 2)? 30: 31))
#define DateLongToJour(date) ((int)(date / 1000000))
#define DateLongToHeure(date) ((int)(date % 1000000))
#define DateJourHeureToLong(jour,heure) ((((int64)jour) * 1000000) + (int64)heure)
#define S_usFloat(d) ((float)d->t.s + ((float)d->t.us / 1000000.0))
#define S_usFloat_us(s_us) ((float)(((TypeS_us)s_us).t.s) + ((float)(((TypeS_us)s_us).t.us) / 1000000.0))

#define S_nsHex64(d) ((((hex64)d.t.s) * 1000000000) + (hex64)(d.t.us))

typedef enum {
	S_US_PLUS = 0,
	S_US_MOINS,
	S_US_OPERNB
} S_US_OPER;

typedef union {
	struct { int s,us; } t;
	hex64 s_us;
} TypeS_us,*S_us,TypeS_ns,*S_ns;

char *DateHeure(void);
char *DateCivile(void);
char *DateJour(void);
char *DateCompacte(void);
int32 DateHeureInt(void);
int32 DateJourInt(void);
int64 DateLong(void);
void DateLongPrint(char *texte, int64 date);
time_t DateLongToTime(int64 d);
void DateIntToHeure(char *texte, int32 entier);
void DateIntToJour(char *texte, int32 entier);
int32 DateHeureToInt(char *texte);
int32 DateJourToInt(char *texte);
int DateIntToJours(int32 jour);
int32 DateIntToSecondes(int32 heure);
int64 DateLongToSecondes(int64 date);
int32 DateSecondesToInt(int32 secs);
int64 DateMicroSecs(void);

void  S_usPrint(char *texte, int s, int us);
hex64 S_usFromInt(int s, int us);
hex64 S_usFromFloat(float f);
hex64 S_usOper(S_us t0, char oper, S_us t1);

hex64 S_nsFromInt(int s, int ns);
hex64 S_nsFromFloat(float f);
hex64 S_nsOper(S_us t0, char oper, S_us t1);

// INLINE float S_usFloat(S_us t);
// INLINE float S_usFloat_us(hex64 s_us);

#ifdef macintosh
	int64 DateSinceBoot(int *hh, int *mn, int *ss, long *usec);
#endif
char *DateCode(char *texte, void *date, int *col);

#endif
