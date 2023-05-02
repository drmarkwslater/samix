#ifndef DECODE_H
#define DECODE_H

#ifdef DECODE_C
#define DECODE_VAR
#else
#define DECODE_VAR extern
#endif

// #pragma message("Utilisation de "__FILE__)

#include "environnement.h"

#include <stdio.h>
//#include <string.h>
/* ca pour sysctlbyname */
#include <sys/types.h>
#include <sys/sysctl.h>
#ifdef _SYS_SYSCTL_H
//	#define _SYS_SYSCTL_H_
#endif

#ifndef CODE_WARRIOR_VSN
	#include <sys/types.h>
	#include <dirent.h>
#endif

#include <defineVAR.h>

#ifdef OS9
#define FLOAT_MAX HUGE
#endif
#ifdef UNIX
#define FLOAT_MAX MAXFLOAT
#endif
#ifdef macintosh
#ifndef FLOAT_MAX
#define FLOAT_MAX HUGE_VALF
#endif
#endif
#ifndef FLOAT_MAX
#define FLOAT_MAX 1.0e35 /* < e40, HUGE seulement pour (double) */
#endif

#define WAV_FREQ 22050
typedef struct {
	char	encodage[5]; char complet;
	short	format_stockage;
	long	dim_totale,dim_bloc_fmt,dim_donnees;
	unsigned short	nb_canaux,dim_echantillon,nb_bits;
	unsigned long	freq_Hz,debit;
} TypeAudioWaveHdr,*AudioWaveHdr;

typedef enum {
	REPERT_FIC = 0,
	REPERT_REP,
	REPERT_ALL = -1
} REPERT_CONTENU;

typedef struct StructItemVar {
	char *nom;
	char *valeur;
	struct StructItemVar *svte;
} TypeItemVar,*ItemVar;
#define ITEMVAR_END 0,0

/*
typedef union {
	struct {
		unsigned char msb,lsb;
	} octet;
	unsigned short mot;
} ShortSwappable;

typedef union {
	struct {
		unsigned char b0,b1,b2,b3;
	} octet;
	unsigned int mot;
} IntSwappable;

typedef union {
	struct {
		unsigned char b0,b1,b2,b3;
	} octet;
	float mot;
} FloatSwappable;

typedef union {
	struct {
		unsigned char b0,b1,b2,b3,b4,b5,b6,b7;
	} octet;
	int64 mot64;
} LongSwappable;

typedef union {
	struct {
		unsigned char b0,b1,b2,b3,b4,b5,b6,b7;
	} octet;
	double mot64;
} DoubleSwappable;

typedef union {
	struct {
		unsigned short s0,s1;
	} court;
	unsigned int mot;
} TwoShortsSwappable;

typedef union {
	struct {
		unsigned short s0,s1,s2,s3;
	} court;
	int64 mot64;
} FourShortsSwappable;
*/

DECODE_VAR char RepertoireDelim[2];
#define SLASH RepertoireDelim[0]

#ifdef WIN32
	#define getcwd(start_dir,lngr) GetCurrentDirectory(lngr,start_dir)
#endif
#ifdef linux
	char *strerror(int num);
#endif
#ifdef _SYS_SYSCTL_H_
	int   System_int32(char *texte);
	int64 System_int64(char *texte);
	char *System_texte(char *texte);
#endif

char EndianIsBig(void);
char *NomApplication(void);
char **CallStackGet(int *nb);
void CallStackInfo(char *niveau, char **rang, char **lib, char **adresse, char **nom);

void RepertoireInit(char *home);
void RepertoirePrefs(char *home, char *appli, char *prefs);
void RepertoireTermine(char *chemin);
void RepertoireSimplifie(char *chemin);
void RepertoireExtrait(char *nom_complet, char *base, char *nom_local);
void RepertoireRelatif(char *base, char *nom_complet, char *nom_local);
void RepertoireNomComplet(char *base, char *nom_demande, char rep, char *nom_complet);
void RepertoireIdentique(char *base, char *nom_local, char *nom_complet);
char RepertoireExiste(char *chemin);
int  RepertoireAbsent(char *chemin);
int  RepertoireCreeRacine(char *nom_complet);
void RepertoireListeCree(char rep, char *chemin, char *liste[], int max, int *nb);
void RepertoireListeLibere(char *liste[], int nb);
int RepertoireFichierInfo(char *nom, char *texte);
char EditeurOuvre(char *nom_complet);
char *MimeTypeOf(char *fichier);

void UsbListeCree(char *liste[], int max, int *nb);
int  UsbOpen(char *nom);
char UsbWrite(int port, char *ligne, char *decode);
char ClavierDeverouille(void);
char ClavierTouche(void);
char *ClavierDump(char c);
void ClavierReverouille(void);

char *LigneSuivante(char *ligne, int maxi, FILE *fichier);
char *EnregSuivant(char *ligne, int maxi, int p);
void NettoieLigne(char *ligne);
#ifdef CODE_WARRIOR_VSN
	char *index(char *s, int c);
	char *rindex(char *s, int c);
#endif
void ItemScan(char *texte, char delimiteur, char **debut, char **suivant);
// char *ItemSuivant(char **chaine, char delimiteur);
// char *ItemTrouve(char **texte, char *delimiteur, char *trouve);
#define ItemSuivant(suite,delimiteur) ItemJusquA(delimiteur,suite)
#define ItemTrouve(suite,delimiteurs,trouve) ItemAvant(delimiteurs,trouve,suite)
char *ItemJusquA(char delimiteur, char **suite);
char *ItemAvant(char *delimiteurs, char *trouve, char **suite);
char *ItemLigne(char **suite);

void ItemImprimeHexa(char *lue);
INLINE unsigned char *ItemRetireAccents(unsigned char *texte);
INLINE unsigned char *ItemCorrigeAccents(unsigned char *texte);
INLINE unsigned char *ItemHtmlAccents(unsigned char *texte);
INLINE unsigned char *ItemMajuscules(unsigned char *texte);
INLINE unsigned char *ItemMinuscules(unsigned char *texte);
INLINE unsigned char *ItemCapitales(unsigned char *texte);
INLINE unsigned char *ItemRetireBlancs(unsigned char *texte);

char *ItemVarAlloc(char *texte);
void ItemVarSet(ItemVar var, char *nom, char *valeur);
ItemVar ItemVarAdd(ItemVar table, char *nom, char *valeur);
void ItemVarUsePrefix(ItemVar var, char *original, char *traduit, int lngr);
void ItemVarUse(ItemVar var, char *original, char *traduit, int lngr);

#ifndef INLINE_DANS_DEFINEVAR
INLINE void ByteSwappeInt(unsigned int *tab);
INLINE void ByteSwappeLong(int64 *tab);
INLINE void ByteSwappeShort(unsigned short *tab);
INLINE void ByteSwappeIntArray(unsigned int *tab, int nb);
INLINE void ByteSwappeFloatArray(float *tab, int nb);
INLINE void ByteSwappeLongArray(int64 *tab, int nb);
INLINE void ByteSwappeDoubleArray(double *tab, int nb);
INLINE void ByteSwappeShortArray(unsigned short *tab, int nb);
INLINE void ByteSwappeWordArray(unsigned short *tab, int nb);
INLINE void TwoShortsSwappeInt(unsigned int *entier);
INLINE void FourShortsSwappeLong(int64 *mot64);
INLINE void TwoShortsSwappeIntArray(unsigned int *tab, int nb);
INLINE void FourShortsSwappeLongArray(int64 *tab, int nb);
#ifdef CODE_WARRIOR_VSN
	int Modulo(int64 v1, int v2);
#else
	INLINE int Modulo(int64 v1, int v2);
#endif
INLINE int inintf(float x);
#endif /* !INLINE_DANS_DEFINEVAR */

void AudioWaveHdrInit(AudioWaveHdr hdr, int nb_voies, int nb_echantillons);
AudioWaveHdr AudioWaveHdrCreate(int nb_voies, int nb_echantillons);
unsigned short AudioWaveVoiesNb(AudioWaveHdr hdr);
float AudioWaveDuree(AudioWaveHdr hdr);
FILE *AudioWaveCreate(char *nom_complet, AudioWaveHdr hdr);
FILE *AudioWaveInit(char *nom_complet, int nb_voies, int nb_echantillons);
FILE *AudioWaveRead(char *nom_complet, AudioWaveHdr hdr);
int AudioSpeakP(unsigned char *chaine_pascal);
int AudioSpeak(char *texte);
int AudioFilePlay(char *wavpath, char *appli);

FILE *SonWaveEntete(char *nom_complet, int nb_voies, int nb_echantillons);
void ImprimeNomSysteme(void);

#endif

