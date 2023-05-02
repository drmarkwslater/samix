#ifndef HISTOS_H
#define HISTOS_H

#include "defineVAR.h"
#include <opium.h>

#ifdef HISTOS_C
#define HISTO_VAR
#else
#define HISTO_VAR extern
#endif

typedef enum {
	HST_INT = 0,
	HST_SHORT,
	HST_FLOAT,
	HST_NBTYPES
} HST_TYPE;

typedef enum {
	HST_AXEX = 0,
	HST_AXEY
} HST_AXES;

typedef struct {
	char type;
	union {
		int i[2];
		short s[2];
		float r[2];
	} v;
} TypeVecteur,*Vecteur;

typedef struct StructHistoData {
	char type;                       /* HST_INT / HST_SHORT / HST_FLOAT */
	void *adrs;                      /* pointeur sur les valeurs        */
	char titre[GRF_MAX_TITRE];
	union {
		int i;
		short s;
		float r;
	} under;                         /* nombre de valeurs trop petites  */
	union {
		int i;
		short s;
		float r;
	} over;                          /* nombre de valeurs trop grandes  */
	struct {
		WndColorLevel r,g,b;         /* couleur                         */
	} coul[WND_NBQUAL];
	void *histo;                     /* histogramme d'appartenance      */
	struct StructHistoData *next;    /* autres valeurs ajoutees         */
} TypeHistoData,*HistoData;

typedef struct {
	int nb;                      /* nombre de bins                  */
	char type;                   /* HST_INT / HST_SHORT / HST_FLOAT */
	union {
		int i[2];
		short s[2];
		float r[2];
	} v;                         /* minimum et pas des bins         */
} HistoAxe;

typedef struct {
	HistoAxe x;
	struct {
		WndColorLevel r,g,b;     /* couleur  des axes               */
	} axe[WND_NBQUAL];
	HistoData first;             /* premieres valeurs ajoutees      */
	HistoData last;              /* dernieres valeurs ajoutees      */
} TypeHisto,*Histo;

typedef struct {
	HistoAxe x,y;
	struct {
		WndColorLevel r,g,b;     /* couleur  des axes               */
	} axe[WND_NBQUAL];
	TypeHistoData data;
	struct {
		unsigned short dim;
		WndColor *val;
	} lut;                       /* LUT                             */
} TypeH2D,*H2D;

//typedef struct {
//	TypeVecteur x,y;
//} TypePlotCoupure,*Coupure;

typedef enum {
	HST_MEMORY = 1,
	HST_NUMBER,
	HST_RANGE,
	HST_WIDTH,
	HST_NBERRORS
} HST_ERROR;

HISTO_VAR int HistoError;
HISTO_VAR char *HistoMsg[HST_NBERRORS]
#ifdef HISTOS_C
 = {
 	"No error",
 	"Not enough memory",
 	"Bin number not positive",
 	"Range max-min not positive",
 	"Bin width not positive",
}
#endif
;

Histo HistoCreateInt(int min, int max, int nb);
Histo HistoCreateFloat(float min, float max, int nb);
void HistoErase(Histo histo);
void HistoDelete(Histo histo);
void HistoDataDelete(HistoData data);
void HistoRGB(Histo histo, 
	WndColorLevel r, WndColorLevel g, WndColorLevel b);
HistoData HistoAdd(Histo histo, char type);
void HistoAxeRGB(Histo histo, int qual, WndColorLevel r, WndColorLevel g, WndColorLevel b);
void HistoDataName(HistoData data, char *nom);
void HistoDataSupportRGB(HistoData data, int qual,
	WndColorLevel r, WndColorLevel g, WndColorLevel b);
void HistoFillIntToInt(HistoData data, int val, int poids);
void HistoFillIntToFloat(HistoData data, int val, float poids);
void HistoFillFloatToInt(HistoData data, float val, int poids);
void HistoFillFloatToFloat(HistoData data, float val, float poids);
int HistoLimitsInt(Histo histo, int min, int max);
int HistoLimitsFloat(Histo histo, float min, float max);
int HistoRebin(Histo histo, int nb);
void HistoGetBinInt(Histo histo, int *min, int *bin, int *nb);
int HistoGetBinFloat(Histo histo, float *min, float *bin, int *nb);
int *HistoGetDataInt(HistoData hd);
float *HistoGetDataFloat(HistoData hd);
void HistoRaz(Histo histo);
void HistoDataRaz(HistoData data);
void HistoDump(Histo histo);
void HistoGraphAdd(Histo histo, Graph graph);
H2D H2DCreateIntIntToInt(int xmin, int xmax, int xnb, 
	int ymin, int ymax, int ynb);
H2D H2DCreateIntFloatToInt(int xmin, int xmax, int xnb, 
	float ymin, float ymax, int ynb);
H2D H2DCreateFloatIntToInt(float xmin, float xmax, int xnb, 
	int ymin, int ymax, int ynb);
H2D H2DCreateFloatFloatToInt(float xmin, float xmax, int xnb, 
	float ymin, float ymax, int ynb);
void H2DDelete(H2D histo);
// void H2DRGB(H2D histo,WndColorLevel r, WndColorLevel g, WndColorLevel b);
void H2DSupportRGB(H2D histo, int qual, WndColorLevel r, WndColorLevel g, WndColorLevel b);
void H2DLUT(H2D histo, WndColor *lut, unsigned short dim);
void H2DFillIntIntToInt(H2D histo, int xval, int yval, int poids);
void H2DFillIntFloatToInt(H2D histo, int xval, float yval, int poids);
void H2DFillFloatIntToInt(H2D histo, float xval, int yval, int poids);
void H2DFillFloatFloatToInt(H2D histo, float xval, float yval, int poids);
int H2DLimitsInt(H2D histo, int type_axe, int min, int max);
int H2DLimitsFloat(H2D histo, int type_axe, float min, float max);
int H2DGetExtremaInt(H2D histo, int *min, int *max);
int H2DGetExtremaFloat(H2D histo, float *min, float *max);
int H2DRebin(H2D histo, int type_axe, int nb);
void H2DGetBinInt(H2D histo, int type_axe, int *min, int *bin, int *nb);
int H2DGetBinFloat(H2D histo, int type_axe, float *min, float *bin, int *nb);
void H2DRaz(H2D histo);
void H2DDump(H2D histo);
void H2DGraphAdd(H2D histo, Graph graph);

#define HistoOf(data) ((Histo)data->histo)
#define HistoBinsNb(histo) (histo->nb)
#define HistoDataFirst(histo) (histo->first)
#define HistoDataNext(data) (data->next)

#endif

