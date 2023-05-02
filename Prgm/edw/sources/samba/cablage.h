#ifndef CABLAGE_H
#define CABLAGE_H

#ifdef CABLAGE_C
#define CABLAGE_VAR
#else
#define CABLAGE_VAR extern
#endif

#include <environnement.h>
#include <edw_types.h>
#include <numeriseurs.h>

#define CABLAGE_CHAMPS

#ifdef CABLAGE_CHAMPS
	// On doit avoir CABLAGE_POS_BITS > CABLAGE_POSX_BITS + CABLAGE_POSY_BITS
	#define CABLAGE_POS_BITS 10
	#define CABLAGE_POS_MAX (1 << CABLAGE_POS_BITS)
	#define CABLAGE_POS_NOTDEF (CABLAGE_POS_MAX - 1)
	#define CABLAGE_POS_DIRECT (CABLAGE_POS_MAX - 2)

	#define CABLAGE_CONN_DIR (CABLAGE_CONNECT_MAX - 1)
	#define CABLAGE_TXTNUL "vide"
	#define CABLAGE_TXTDIR "intr"
	#define CABLAGE_EXPLIC " (0="CABLAGE_TXTNUL", "chaine(CABLAGE_CONN_DIR)"="CABLAGE_TXTDIR")"

	#define CABLAGE_DEFAUT_GAIN 1.0
	#define CABLAGE_DEFAUT_CAPA 10.0
	#define CABLAGE_DEFAUT_RC 0.5

	/* Galette: ChassisDetec[0] (X)
		  Tour: ChassisDetec[1] (Y)
	   Branche: ChassisDetec[2] (Z) */

	#define CABLAGE_POSX_BITS 5
	#define CABLAGE_POSY_BITS 4

	#define CABLAGE_POSX_MAX ((1 << CABLAGE_POSX_BITS) - 1)
	#define CABLAGE_POSY_MAX ((1 << CABLAGE_POSY_BITS) - 1)

	#define CABLAGE_POSX_SHIFT CABLAGE_POSY_BITS
	#define CABLAGE_POSY_SHIFT 0

	#define CABLAGE_POSX(pos_hexa) (short)((pos_hexa >> CABLAGE_POSX_SHIFT) & CABLAGE_POSX_MAX)
	#define CABLAGE_POSY(pos_hexa) (short)(pos_hexa & CABLAGE_POSY_MAX)

	#define CABLAGE_POSITION(galette,tour) (((galette & CABLAGE_POSX_MAX) << CABLAGE_POSX_SHIFT) | (tour & CABLAGE_POSY_MAX))

	#define CABLAGE_POSZ_BITS (CABLAGE_POS_BITS - CABLAGE_POSX_BITS - CABLAGE_POSY_BITS)
	#define CABLAGE_POSZ_MAX ((1 << CABLAGE_POSZ_BITS) - 1)
	#ifdef AVEC_POSZ
		#define CABLAGE_POSZ_SHIFT (CABLAGE_POSX_SHIFT + CABLAGE_POSX_BITS)
		#define CABLAGE_POSZ(pos_hexa) (short)((pos_hexa >> CABLAGE_POSZ_SHIFT) & CABLAGE_POSZ_MAX)
	#endif


	/* Planche: ChassisNumer[0] (X)
	  Distance: ChassisNumer[1] (Y) */

	#define CABLAGE_DISTANCE_BITS 7
	// #define CABLAGE_PLANCHE_BITS ((sizeof(TypeCableConnecteur)*8 -1) - CABLAGE_DISTANCE_BITS)
	#define CABLAGE_PLANCHE_BITS ((sizeof(TypeCableConnecteur)*8) - CABLAGE_DISTANCE_BITS)

	#define CABLAGE_DISTANCE_MAX ((1 << CABLAGE_DISTANCE_BITS) - 1)
	#define CABLAGE_PLANCHE_MAX ((1 << CABLAGE_PLANCHE_BITS) - 1)

	#define CABLAGE_DISTANCE_SHIFT 0
	#define CABLAGE_PLANCHE_SHIFT CABLAGE_DISTANCE_BITS

	#define CABLAGE_DISTANCE(connecteur) (short)(connecteur & CABLAGE_DISTANCE_MAX)
	#define CABLAGE_PLANCHE(connecteur) (short)((connecteur >> CABLAGE_PLANCHE_SHIFT) & CABLAGE_PLANCHE_MAX)

	#define CABLAGE_CONNECTEUR(planche,distance) \
		(TypeCableConnecteur)(((planche & CABLAGE_PLANCHE_MAX) << CABLAGE_PLANCHE_SHIFT) | (distance & CABLAGE_DISTANCE_MAX))

#else /* !CABLAGE_CHAMPS */
#endif /* !CABLAGE_CHAMPS */

#define CABLAGE_MODL_NOM 16
#define CABLAGE_NOM 16
#define CABLAGE_TYPE_MAX 16

/*	printf("= Codage des positions:\n");
	printf("  .  Detecteurs: (etage & %X) << %d | (tour & %X) << %d\n",
 		CABLAGE_POSX_MAX,CABLAGE_POSX_SHIFT,CABLAGE_POSY_MAX,CABLAGE_POSY_SHIFT);
	printf("  . Numeriseurs: (planche & %X) << %d | (distance & %X) << %d\n",
 		CABLAGE_PLANCHE_MAX,CABLAGE_PLANCHE_SHIFT,CABLAGE_DISTANCE_MAX,CABLAGE_DISTANCE_SHIFT);
	printf("\n"); */

typedef struct {
	char  nom[MODL_NOM];
	char  adc[8];
	float gain,capa,rc;
	short numer_declare;  // Attention: [1..n], correspond a nom et est indexe par le rang dans la declaration du modele
	short numer_index;    // Attention: [0..n[, correspond a <j> et est indexe par le rang dans la declaration du detecteur
	short numer_adc;      // adc dans [1..n] et est indexe par le rang dans la declaration du detecteur, comme numer_index
} TypeCablageConnectCapt;
typedef struct {
	char reglage[DETEC_REGL_NOM];
	char ressource[NUMER_RESS_NOM];
	char *valeurs;        // utilise dans les RAZ (FET, etc...)
} TypeCablageConnectRegl;
typedef struct {
	char  nom[CABLAGE_MODL_NOM];
	int   num;
	int   modl_bolo;
	int   nb_numer;
	int   modl_numer[DETEC_CAPT_MAX];
	int   nb_capt,max_adc;
	TypeCablageConnectCapt capteur[DETEC_CAPT_MAX];
	int  nbconnections;
	TypeCablageConnectRegl connection[DETEC_REGL_MAX];
} TypeModeleCable;

CABLAGE_VAR TypeModeleCable ModeleCable[CABLAGE_TYPE_MAX],ModeleCableLu;
CABLAGE_VAR char *ModeleCableListe[CABLAGE_TYPE_MAX+1];
CABLAGE_VAR int ModeleCableNb;
CABLAGE_VAR TypeCablageConnectCapt ConnectCaptLue;
CABLAGE_VAR TypeCablageConnectRegl ConnectReglLue;

typedef struct {
	float gain,capa,rc;           /* caracteristiques de l'electronique froide */
	TypeCableConnecteur connecteur;
	char vbb;                     /* numero d'adc dans le numeriseur (0..n-1) */
} TypeCablageCaptr;
typedef struct {
	char nom[CABLAGE_NOM];
	char defini;                  /* si oui, lu (donc, a ecrire) dans le fichier Cablage */
	char v2;                      /* vrai si lu au format v2                             */
	int type;                     /* un type de cablage est un numero de dico/modele     */
	int bolo;                     /* detecteur place a cette position                    */
	int nb_fischer;
	TypeCableConnecteur fischer[DETEC_CAPT_MAX];
	int nbcapt;
	TypeCablageCaptr captr[DETEC_CAPT_MAX];            /* inverse de la table (b) */
} TypeCablage;

CABLAGE_VAR TypeCablage CablageSortant[CABLAGE_POS_MAX];
CABLAGE_VAR char CablageDeplie[CABLAGE_POS_MAX];
CABLAGE_VAR int GaletteMin,GaletteMax;
// CABLAGE_VAR shex CablageInverse[CABLAGE_CONNECT_MAX][DETEC_CAPT_MAX]; /* table (b) */
// CABLAGE_VAR shex CablageType[256];           /* un type de cablage est un dico */

#ifdef CABLAGE_DICO
	#define CABLAGE_DICO_MAX 32
	CABLAGE_VAR TypeDico CablageDicoEntree[CABLAGE_DICO_MAX];
	CABLAGE_VAR int CablageDicoEntreeNb;
#endif /* CABLAGE_DICO */

CABLAGE_VAR ArgDesc ConnectCaptDesc[]
#ifdef CABLAGE_C
 = {
	{ 0, DESC_STR(MODL_NOM)    ConnectCaptLue.nom,  0 },
	{ 0, DESC_STR(8)           ConnectCaptLue.adc,  0 },
	{ "gain_V/pC", DESC_FLOAT &ConnectCaptLue.gain, 0 },
	{ "capa_pF",   DESC_FLOAT &ConnectCaptLue.capa, 0 },
	{ "RC_ms",     DESC_FLOAT &ConnectCaptLue.rc,   0 },
	DESC_END
}
#endif
;
CABLAGE_VAR ArgStruct ConnectCaptAS
#ifdef CABLAGE_C
 = { (void *)ModeleCableLu.capteur, (void *)&ConnectCaptLue, sizeof(TypeCablageConnectCapt), ConnectCaptDesc }
#endif
;
CABLAGE_VAR ArgDesc ConnectReglDesc[]
#ifdef CABLAGE_C
 = {
	{ 0, DESC_STR(DETEC_REGL_NOM)  ConnectReglLue.reglage, 0 },
	{ 0, DESC_STR(NUMER_NOM)       ConnectReglLue.ressource, 0 },
//+	{ "procedures", DESC_STR_PTR  &(ConnectReglLue.valeurs), 0 },
	DESC_END
}
#endif
;
CABLAGE_VAR ArgStruct ConnectReglAS
#ifdef CABLAGE_C
 = { (void *)ModeleCableLu.connection, (void *)&ConnectReglLue, sizeof(TypeCablageConnectRegl), ConnectReglDesc }
#endif
;
CABLAGE_VAR ArgDesc ModeleCableDesc[]
#ifdef CABLAGE_C
 = {
	{ 0,                DESC_STR(CABLAGE_MODL_NOM)                                               ModeleCableLu.nom, 0 },
	{ "detecteur",      DESC_LISTE ModeleDetListe,                                              &ModeleCableLu.modl_bolo, 0 },
	{ "numeriseurs",    DESC_LISTE_SIZE(ModeleCableLu.nb_numer,DETEC_CAPT_MAX) ModeleNumerListe, ModeleCableLu.modl_numer, 0 },
	{ "capteurs",       DESC_STRUCT_SIZE(ModeleCableLu.nb_capt,DETEC_CAPT_MAX)                  &ConnectCaptAS, 0 },
	{ "reglages",       DESC_STRUCT_SIZE(ModeleCableLu.nbconnections,DETEC_REGL_MAX)            &ConnectReglAS, 0 },
	DESC_END
}
#endif
;
CABLAGE_VAR ArgStruct ModeleCablageAS
#ifdef CABLAGE_C
 = { (void *)ModeleCable, (void *)&ModeleCableLu, sizeof(TypeModeleCable), ModeleCableDesc }
#endif
;

CABLAGE_VAR char CablageFichierLu[MAXFILENAME];

void                CablageModlLit(char *fichier);
void                CablageModlEcrit(FILE *f);
void                CablageModlSauve(char *fichier);
void                CablageNeuf(TypeCablePosition pos_hexa);
void                CablageEcrit(char *fichier);
int                 CablageLit(char *fichier);
TypeCablePosition   CablageDecodePosition(char *position);
char               *CablageEncodePosition(TypeCablePosition pos_hexa);
char               *CablageEncodeCoord(TypeCablePosition pos_hexa);
INLINE void         CablageAnalysePosition(TypeCablePosition pos_hexa, short *galette, short *trou, short *branche);
TypeCableConnecteur CablageDecodeConnecteur(char *texte);
INLINE void         CablageAnalyseConnecteur(TypeCableConnecteur connecteur, short *quadrant, short *distance);
char               *CablageEncodeConnecteur(TypeCableConnecteur connecteur);
int                 CablageRessource(TypeModeleCable *cablage, char **nom, char signe);

#endif
