#ifndef EVENEMENT_H
#define EVENEMENT_H

/* ................................ evenement ............................... */

#define REPART_EVENT (shex)0x7FFFFF
#define REPART_EVENT_DEF    0x0001
#define REPART_EVENT_DATA   0x0002
#define REPART_EVENT_CLOSED 0x0003
static char *Etiquette[4] = { "INDEFINIE", "DEFINITION", "DONNEES", "EVT TERMINE" };

typedef struct {
	int64 stamp;      /* date du debut de trace                            */
	short dim;        /* nombre total de donnees transmises                */
	short entree;     /* numero de numeriseur (index dans repart->entree)  */
	short adc;        /* numero d'adc: voie = numeriseur->voie[adc]        */
	shex nul;
} TypeEventChannel;

typedef struct {
	int64 stamp;      /* date absolue en nb de coups d'horloge                    */
	int num;          /* numero d'evenement                                       */
	byte  bb;         /* numeriseur de la voie ayant declenche                    */
	byte  trig;       /* voie (en fait, ADC) ayant declenche                      */
	shex niveau;      /* maximum de  l'evenement                                  */
	shex dim;         /* nombre total de voies transmises                         */
	byte version;     /* version de la structure                                  */
	byte vide;        /* complement inutilise en version 0                        */
	hexa temps_mort;  /* temps depuis le dernier evenement                        */
} TypeEventDefinition;

typedef struct {
	TypeEventChannel canal[REPART_VOIES_MAX];
} TypeEventCanaux;

typedef struct {
	TypeEventDefinition def;
	TypeEventCanaux trace;
} TypeEventHeader;

typedef struct {
	shex etiquette; /* determine le contenu de la trame (REPART_EVENT_xxx) */
	shex nul[3];
} TypeEventTypeTrame;

typedef struct {
	int evt;                         /* numero d'evenement (comparer avec EventHeader)    */
	short tranche;                   /* numero de tranche                                 */
	short nb;                        /* nombre de valeurs dans cette tranche              */
} TypeEventDataRef;

#define REPART_EVENT_SIZE (CALI_TAILLE_DONNEES-sizeof(TypeEventTypeTrame)-sizeof(TypeEventDataRef))
#define REPART_EVENT_NBVAL (REPART_EVENT_SIZE/sizeof(TypeADU))
typedef struct {
	TypeEventDataRef ref;
	union {
		TypeDonnee s[REPART_EVENT_NBVAL];
		TypeLarge  i[REPART_EVENT_NBVAL];
		TypeSignal r[REPART_EVENT_NBVAL];
	} val; /* tableau des valeurs transmises                    */
} TypeEventData;

typedef struct {
	TypeEventTypeTrame type;
	union {
		TypeEventHeader hdr;
		TypeEventData donnees;
	} is;
} TypeEventFrame;

#endif /* EVENEMENT_H */
