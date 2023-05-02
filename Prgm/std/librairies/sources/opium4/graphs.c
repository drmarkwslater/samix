#include <stdio.h>
#include <stdlib.h> /* pour malloc et free */
#include <stdarg.h> /* pour va_start */
#include <fcntl.h>

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

#include <math.h>
#ifdef linux
	#ifndef MAXFLOAT
		#define MAXFLOAT 1.0E73
	#endif
#endif

#include "environnement.h"


#include "decode.h"
#include "dateheure.h"
#include <opium.h>

// #define OpiumError WndPrint
#define GRAPH_ERROR
#ifdef GRAPH_ERROR
#define OpiumError GraphError
#endif

#define GRAPH_PSMARGE_X 10
#define GRAPH_PSMARGE_Y 5

static struct {
	int i0,i1;
	float r0,r1;
} GraphUser[2];
static int GraphZoneEnCours; static char GraphZoneLog;

static TypeInstrumGlissiere GraphGlissiere = {60, 15, 0, 0, 5, 3};

static void GraphInternalStart(GraphData data, int min);
static void GraphInternalUse(GraphData data, int nb);
static void GraphInternalScroll(GraphData data, int nb);
static void GraphInternalRescroll(GraphData data, int nb);
static int GraphRefresh(Instrum instrum);

static void GraphMarkOrigin(int type, int x, int y);
static int  GraphScaleDefine(Graph graph);
static int  GraphGradsDefine(Graph graph, unsigned char axe, unsigned int zoom);
static void GraphGradMin(GraphScale *scale, int decalage);
static void GraphAxisXDraw(Graph graph, int y, WndContextPtr gc, unsigned char typegrad);
static void GraphAxisYDraw(Graph graph, int x, WndContextPtr gc, unsigned char typegrad);
static inline void GraphMarkDraw(WndFrame f, WndContextPtr gc, int type, int nb);
static inline char GraphUserVal(GraphData data, int k, int type, int *i, float *r);
static inline int GraphPosFloat(int mode, float r, GraphScale *scale);
static inline int GraphPosShort(int mode, short i, GraphScale *scale);
static inline int GraphPosInt(int mode, int i, GraphScale *scale);
static inline int GraphPosData(int mode, GraphData data, int k, GraphScale *scale);

static void GraphDialogInit(Cadre cdr);
static int GraphDialogGeneral();
static int GraphDialogAxe(Menu menu, MenuItem item);
static int GraphDialogDonnee();
static int GraphDialogEtatRetabli();
static int GraphDialogZoomMoins();
static int GraphDialogZoomJuste();
static int GraphDialogTablesEcrit();

/* ========================================================================== */
void GraphLargeurTraits(int qual, unsigned char width) {
	if(width < 1) width = 1; else if(width > 7) width = 7;
	GraphWidzDefaut[qual] = width;
	// printf("(%s) GraphWidzDefaut %s = %d\n",__func__,WndSupportNom[qual],GraphWidzDefaut[qual]);
}
/* ========================================================================== */
void GraphQualiteDefaut(char qual) {
	GraphQualite = qual;
}
/* ========================================================================== */
Graph GraphCreate(int larg, int haut, short max) {
	Cadre cdr; Graph graph; int i;

	graph = (Graph)malloc(sizeof(TypeGraph) + (max * sizeof(TypeGraphData)));
	if(graph) {
		if(DEBUG_GRAPH(2)) WndPrint("Graph %d x %d cree @%08lX\n",larg,haut,(Ulong)graph);
		cdr = OpiumCreate(OPIUM_GRAPH,graph);
		if(!cdr) {
			free(graph); return(0);
		}
		graph->cdr = cdr;
		if(haut < 10) haut = 10;
		if(larg < 10) larg = 10;
		graph->dim = 0;
		graph->max = max;
		graph->axe[0].lngr = larg; graph->axe[1].lngr = haut;
		graph->axe[0].zoom = graph->axe[1].zoom = 1;
		graph->grad = 6; graph->of7 = 2;
		graph->data = (GraphData)((IntAdrs)graph + sizeof(TypeGraph));
		for(i=0; i<max; i++) {
			graph->data[i].titre[0] = '\0';
			graph->data_ptr[i] = graph->data + i;
		}
		for( ; i<GRF_MAX_DATA; i++) graph->data_ptr[i] = 0;
		graph->ajouts = 0;
		graph->onclic = 0;
		if(DEBUG_GRAPH(1)) {
			WndPrint("(%s) GraphData (%d preallouees @%08lX)",__func__,max,(Ulong)graph->data);
			for(i=0; i<GRF_MAX_DATA; i++) {
				if(!(i%8)) WndPrint("\n%3d:",i);
				WndPrint(" %08lX",(Ulong)graph->data_ptr[i]);
			}
			WndPrint("\n");
		}
		graph->pomme = graph->grille = 0;
		GraphErase(graph);
		OpiumSetOptions(cdr);
		if(DEBUG_GRAPH(3)) WndPrint("Utilise cdr @%08lX\n",(Ulong)graph->cdr);
	}

	return(graph);
}
/* ========================================================================== */
void GraphDelete(Graph graph) {
	int num,qual;
	GraphData data,*data_ptr;

	if(!graph) return;
	data_ptr = graph->data_ptr;
	for(num=0; num<graph->dim; num++, data_ptr++) {
		data = *data_ptr; if(!data) continue;
		if(graph->cdr) for(qual=0; qual<WND_NBQUAL; qual++) if(data->gc[qual]) {
			WndContextFree((graph->cdr)->f,data->gc[qual]); data->gc[qual] = 0;
		}
		if(num >= graph->max) free(data);
	}
	OpiumDelete(graph->cdr);
	free(graph);
}
/* ========================================================================== */
void GraphTitle(Graph graph, char *texte) {
	if(!graph) return;
	OpiumTitle(graph->cdr,texte);
}
/* ========================================================================== */
void GraphMode(Graph graph, unsigned short mode) {
/* Mode d'un graphe:
	- melange de (GRF_0AXES,GRF_2AXES,GRF_4AXES), GRF_GRID, GRF_NOGRAD,GRF_LEGEND
	- ou seulement GRF_DIRECT */
	if(!graph) return;
	graph->mode = mode;
	if(mode == GRF_DIRECT) {
		graph->axe[0].num          = graph->axe[1].num            = GRF_DIRECT;
		graph->axe[0].reel         = graph->axe[1].reel           = 0;
		graph->axe[0].minauto      = graph->axe[1].minauto        = 0;
		graph->axe[0].maxauto      = graph->axe[1].maxauto        = 0;
		graph->axe[0].gradauto     = graph->axe[1].gradauto       = 0;
		graph->axe[0].u.i.min      = 0; graph->axe[1].u.i.min     = 0;
		graph->axe[0].u.i.max      = graph->axe[0].lngr; graph->axe[1].u.i.max = graph->axe[1].lngr;
		graph->axe[0].ech = (float)(graph->axe[0].u.i.max - graph->axe[0].u.i.min);
		graph->axe[1].ech = (float)(graph->axe[1].u.i.max - graph->axe[1].u.i.min);
		graph->axe[0].pret         = graph->axe[1].pret           = 1;
		graph->axe[0].marge_min    = graph->axe[1].marge_min      = 0;
		graph->axe[0].marge_max    = graph->axe[1].marge_max      = 0;
		graph->axe[0].pixel        = graph->axe[1].pixel          = 1.0f;
		/* printf("(%s) graph %d x %d mode %x\n",__func__,graph->axe[0].lngr,graph->axe[1].lngr,graph->mode);
		printf("(%s) xscale: %d .. %d\n",__func__,graph->axe[0].u.i.min,graph->axe[0].u.i.max);
		printf("(%s) yscale: %d .. %d\n",__func__,graph->axe[1].u.i.min,graph->axe[1].u.i.max); */
	} else {
		graph->axe[0].num          = graph->axe[1].num  = GRF_UNDEFINED; /* donc, pas pret */
		graph->axe[0].pret         = graph->axe[1].pret = 0;
		graph->axe[0].marge_min    = 0; /* on ne sait jamais.. */
		graph->axe[0].marge_max    = graph->axe[0].marge_min;
		graph->axe[1].marge_min    = graph->grad + graph->of7;
		graph->axe[1].marge_max    = WndLigToPix(1);
	}
	graph->axe[0].marge_totale = graph->axe[0].marge_min + graph->axe[0].marge_max;
	graph->axe[1].marge_totale = graph->axe[1].marge_min + graph->axe[1].marge_max;
}
/* ========================================================================== */
void GraphErase(Graph graph) {
	int num; int i,qual;
	GraphData data,*data_ptr;

	if(!graph) return;
	data_ptr = graph->data_ptr + graph->max;
	if(DEBUG_GRAPH(1)) printf("(GraphErase) graph->max=%d, dim=%d\n",graph->max,graph->dim);
	for(num=graph->max; num<graph->dim; num++, data_ptr++) {
		if(DEBUG_GRAPH(1)) printf("(GraphErase) data_ptr[%d] = %08lX\n",num,(Ulong)data_ptr);
		data = *data_ptr; if(!data) continue;
		for(qual=0; qual<WND_NBQUAL; qual++) if(data->gc[qual]) { WndContextFree((graph->cdr)->f,data->gc[qual]); data->gc[qual] = 0; }
		free(data); *data_ptr = 0;
	}
	graph->dim = 0;
//	GraphMode(graph,GRF_DIRECT);
	GraphMode(graph,GRF_2AXES | (GraphQualite? GRF_QUALITE: 0));
	graph->mouse[0]          = graph->mouse[1]          = -1;
	graph->axe[0].num        = graph->axe[1].num        = GRF_UNDEFINED; /* donc, pas pret */
	graph->axe[0].minauto    = graph->axe[1].minauto    = 1;
	graph->axe[0].maxauto    = graph->axe[1].maxauto    = 1;
	graph->axe[0].gradauto   = graph->axe[1].gradauto   = 1;
	graph->axe[0].logd       = graph->axe[1].logd       = 0;
	graph->axe[0].titre[0]   = graph->axe[1].titre[0]   = '\0';
	graph->axe[0].chgt_unite = graph->axe[1].chgt_unite = 0;
	graph->axe[0].pixel      = graph->axe[1].pixel      = 1.0f;
	graph->axe[0].ech        = graph->axe[1].ech        = 0.0f;
	graph->axe[0].u.i.min = 0; graph->axe[1].u.i.min = 0;
	graph->axe[0].u.i.max = graph->axe[0].lngr; graph->axe[1].u.i.max = graph->axe[1].lngr;
	graph->axe[0].ech = (float)(graph->axe[0].u.i.max - graph->axe[0].u.i.min);
	graph->axe[1].ech = (float)(graph->axe[1].u.i.max - graph->axe[1].u.i.min);
	strcpy(graph->axe[0].format,"%g");
	strcpy(graph->axe[1].format,"%g");
	graph->axe[0].cols = 1;
	graph->mode_sauve = 0xFFFF;
	graph->pret = 0;
	for(i=0; i<GRF_MAX_ZONES; i++)
		graph->zone[i].xmin = graph->zone[i].ymin = graph->zone[i].xmax = graph->zone[i].ymax = 0;
}
#ifdef GRAPH_ERROR
/* ========================================================================== */
static int GraphError(char *fmt, ...) {
	va_list args; char texte[256]; int n;

	va_start(args,fmt);
	n = vsprintf(texte,fmt,args);
	va_end(args);
	// sprintf(texte,fmt,p0,p1,p2,p3,p4,p5,p6,p7);

	WndPrint("! %s\n",texte);
	return(1);
}
#endif
/* ========================================================================== */
int GraphConnect(Graph graph, int type, void *adrs, int dim) {
	return(GraphAdd(graph,type,adrs,dim));
}
/* ========================================================================== */
static void GraphDataDefault(Graph graph, GraphData data, int type, int dim)  {
	int type_axe,qual;

	/* Definitions utilisateur (attention: chgmt definition parm -> manoeuvres pour compatibilite */
	data->parm = (unsigned char)(type & (GRF_AXIS | GRF_ADRS | GRF_TYPE));
	data->style = 0;
	data->min = 0;
	data->max = dim;
	data->dim = dim;
	if((type & GRF_ICONE)) {
		data->parm |= GRF_IMAGE;
		data->couleur.type = GRF_ICN;
	} else {
		type_axe = (type & GRF_AXIS);
		if((type_axe == GRF_IMAGE) || (type_axe == GRF_ZAXIS)) {
			if((data->parm & GRF_TYPE) == GRF_INT) {
				data->gamma.i.xmin = 0;
				data->gamma.i.xmax = 255;
				data->gamma.i.vmin = 0;
				data->gamma.i.vmax = 255;
			} else if((data->parm & GRF_TYPE) == GRF_FLOAT) {
				data->gamma.r.xmin = 0.0f;
				data->gamma.r.xmax = 255.0f;
				data->gamma.r.vmin = 0.0f;
				data->gamma.r.vmax = 255.0f;
			}
			data->couleur.type = GRF_LUT;
			data->couleur.def.lut.dim = 0;
			data->parm = data->parm | GRF_GAMMA;
		} else {
			data->couleur.type = GRF_NOM;
			for(qual=0; qual<WND_NBQUAL; qual++) data->couleur.def.nom[qual][0] = '\0';
		}
	}
	if((data->parm & GRF_TYPE) == GRF_FLOAT) data->of7.r.v = 0.0f; else data->of7.i.v = 0;
	// if((type_axe != GRF_IMAGE) && (graph->mode == GRF_DIRECT)) GraphMode(graph,GRF_2AXES);
	graph->pret = 0;
	if(DEBUG_GRAPH(3)) WndPrint("(%s) graph->mode=%x, axeX=%x, axeY=%x (GRF_DIRECT=%x)\n",__func__,
		graph->mode,graph->axe[0].num,graph->axe[1].num,GRF_DIRECT);
}
/* ========================================================================== */
int GraphAdd(Graph graph, int type, void *adrs, int dim) {
	short index; int qual;
	GraphData data;

	if(!graph) return(-1);
	index = graph->dim; data = 0; /* pour le cas ou la boucle ne demarre meme pas */
	for(index=0; index<graph->dim; index++) {
		if((data = graph->data_ptr[index]) == 0) break;
		if(data->fctn == (void *)-1) break;
		data = 0; /* pour le cas ou la boucle va jusqu'a la fin */
	}
	if(index >= GRF_MAX_DATA) {
		OpiumError("(%s) Pas plus de %d tableaux",__func__,GRF_MAX_DATA);
		return(-1);
	} else if(index >= graph->max) {
		if(!data) data = (GraphData)malloc(sizeof(TypeGraphData));
		if(data) graph->data_ptr[index] = data;
		else { OpiumError("(%s) Manque de place memoire",__func__); return(-1); }
	} else data = graph->data_ptr[index];
	/* Parametres par defaut */
	data->titre[0] = '\0';
	for(qual=0; qual<WND_NBQUAL; qual++) {
		data->gc[qual] = 0;
		data->trace[qual] = (unsigned char)(((type & 3) << GRF_SHIFT_MODE) | (GraphWidzDefaut[qual] << GRF_SHIFT_WIDZ));
		// printf("(%s) data[%d] %s = 0x%02X\n",__func__,index,WndSupportNom[qual],data->trace[qual]);
	}
	data->fctn = adrs;  /* peut etre 0 pour 'reserver' le numero de tableau */
	GraphDataDefault(graph,data,type,dim);

	if(index == graph->dim) graph->dim = index + 1;
	return(index);
}
/* ========================================================================== */
int GraphDataReplace(Graph graph, int num, int type, void *adrs, int dim) {
	GraphData data;

	if(!graph) return(-1);
	if((num < 0) || (num >= graph->dim) ) {
		OpiumError("(%s '%s'#%d) Numero de tableau invalide (dim=%d)",__func__,
			(graph->cdr)->titre,num,graph->dim);
		return(-1);
	}
	data = graph->data_ptr[num];
	if(!data) { OpiumError("(%s) Tableau #%d pas alloue en memoire",__func__,num); return(-1); }
	data->fctn = adrs;
	GraphDataDefault(graph,data,type,dim);

	return(0);
}
/* ========================================================================== */
int GraphInclude(Graph graph, int type, void *adrs, int dim) {
	short index; int qual;
	GraphData data;

	if(!graph) return(-1);
	index = graph->dim;
	if(index >= GRF_MAX_DATA) {
		OpiumError("(%s) Pas plus de %d tableaux",__func__,GRF_MAX_DATA);
		return(-1);
	} else if(index >= graph->max) {
		data = (GraphData)malloc(sizeof(TypeGraphData));
		if(data) graph->data_ptr[index] = data;
		else { OpiumError("(%s) Manque de place memoire",__func__); return(-1); }
	} else data = graph->data_ptr[index];
	/* Parametres par defaut */
	for(qual=0; qual<WND_NBQUAL; qual++) {
		data->gc[qual] = 0;
		data->trace[qual] = (unsigned char)(((type & 3) << GRF_SHIFT_MODE) | (GraphWidzDefaut[qual] << GRF_SHIFT_WIDZ));
	}
	/* Probleme pour savoir qu'il faudra liberer data->fctn */
	if((type & GRF_TYPE) == GRF_FLOAT) {
		if((type & GRF_ADRS) == GRF_INDEX) {
			data->fctn = (void *)malloc(2*sizeof(float));
			memcpy(data->fctn,adrs,2*sizeof(float));
		} else {
			data->fctn = (void *)malloc(dim*sizeof(float));
			memcpy(data->fctn,adrs,dim*sizeof(float));
		}
	} else if((type & GRF_TYPE) == GRF_SHORT) {
		if((type & GRF_ADRS) == GRF_INDEX) {
			data->fctn = (void *)malloc(2*sizeof(short));
			memcpy(data->fctn,adrs,2*sizeof(short));
		} else {
			data->fctn = (void *)malloc(dim*sizeof(short));
			memcpy(data->fctn,adrs,dim*sizeof(short));
		}
	} else {
		if((type & GRF_ADRS) == GRF_INDEX) {
			data->fctn = (void *)malloc(2*sizeof(int));
			memcpy(data->fctn,adrs,2*sizeof(int));
		} else {
			data->fctn = (void *)malloc(dim*sizeof(int));
			memcpy(data->fctn,adrs,dim*sizeof(int));
		}
	}
	GraphDataDefault(graph,data,type,dim);

	graph->dim = index + 1;
	return(index);
}
/* ========================================================================== */
void GraphArrayWindowSize(int larg, int haut) {
	GraphArrayLarg = larg; GraphArrayHaut = haut;
}
/* ========================================================================== */
Graph GraphArrayDisplay(char *nom, char type_x, void *dx, char type_table, void *table, int lngr) {
	Graph g; short sx[2]; int ix[2]; float fx[2]; void *adrs;

	adrs = 0;
	switch(type_x) {
		case GRF_INT  : ix[0] = 0;   ix[1] = *(int *)dx;   adrs = (void *)ix; break;
		case GRF_SHORT: sx[0] = 0;   sx[1] = *(short *)dx; adrs = (void *)sx; break;
		case GRF_FLOAT: fx[0] = 0.0; fx[1] = *(float *)dx; adrs = (void *)fx; break;
	}
	if(!adrs) return(0);
	g = GraphCreate(GraphArrayLarg,GraphArrayHaut,0); GraphMode(g,GRF_2AXES|GRF_LEGEND);
	GraphInclude(g,type_x|GRF_INDEX|GRF_XAXIS,adrs,lngr);
	GraphInclude(g,type_table|GRF_YAXIS,table,lngr);
	GraphUse(g,lngr);
	GraphTitle(g,nom);
	OpiumDisplay(g->cdr);

	return(g);
}
/* ========================================================================== */
void GraphArrayHold(Graph graph, char hold) {
	GraphArrayEnCours = hold;
	if(!GraphArrayEnCours && graph) OpiumRefresh(graph->cdr);
}
/* ========================================================================== */
void GraphArrayAdd(Graph graph, char type_table, void *table, int lngr) {
	GraphInclude(graph,type_table|GRF_YAXIS,table,lngr);
	GraphUse(graph,lngr);
	if(!GraphArrayEnCours && graph) OpiumRefresh(graph->cdr);
}
/* ========================================================================== */
void GraphArrayName(Graph graph, int num, char *nom) {
	GraphDataName(graph,num+1,nom);
	if(!GraphArrayEnCours && graph) OpiumRefresh(graph->cdr);
}
/* ========================================================================== */
void GraphArrayRGB(Graph graph, int num, WndColorLevel r, WndColorLevel g, WndColorLevel b) {
	GraphDataRGB(graph,num+1,r,g,b);
	if(!GraphArrayEnCours && graph) OpiumRefresh(graph->cdr);
}
/* ========================================================================== */
void GraphArrayDelete(Graph graph) {
	if(graph) {
		if(OpiumDisplayed(graph->cdr)) OpiumClear(graph->cdr);
		GraphDelete(graph);
	}
}
/* ========================================================================== */
Graph GraphCreateIcone(WndIcone icone, unsigned int type) {
	int dim_image,i,k;
	WndColor *image; Graph g; unsigned char *masque;

	g = 0;
	dim_image = icone->larg * icone->haut;
	if(type == GRF_IMAGE) {
		image = GraphLUTCreate(dim_image,-1); // jamais purgee
		masque = (unsigned char *)malloc(dim_image * sizeof(unsigned char));
		if(masque == 0) return(0);
		for(k=0; k<dim_image; k++) {
			masque[k] = icone->pixel[k].m;
			WndColorSetByRGB(image+k,icone->pixel[k].r<<8,icone->pixel[k].g<<8,icone->pixel[k].b<<8);
		}
		g = GraphCreate(icone->larg,icone->haut,2); // pour que <masque> soit purge au delete
		GraphMode(g,GRF_DIRECT);
		GraphAdd(g,GRF_XAXIS|GRF_INDEX|GRF_INT,0,icone->larg);
		i = GraphAdd(g,GRF_YAXIS|GRF_INDEX|GRF_INT,0,icone->haut);
		GraphDataTranslate(g,i,GRF_NODSP);
		i = GraphAdd(g,GRF_IMAGE|GRF_INT,masque,dim_image);
		GraphDataBmp(g,i,image,dim_image);
	} else if(type == GRF_ICONE) {
		g = GraphCreate(icone->larg,icone->haut,3);
		GraphMode(g,GRF_DIRECT);
		GraphAdd(g,GRF_XAXIS|GRF_INDEX|GRF_INT,0,icone->larg);
		i = GraphAdd(g,GRF_YAXIS|GRF_INDEX|GRF_INT,0,icone->haut);
		GraphDataTranslate(g,i,GRF_NODSP);
		GraphAdd(g,GRF_ICONE|GRF_INT,icone,dim_image);
	}
	return(g);
}
/* ========================================================================== */
void GraphAjoute(Graph graph, TypeGraphFctn fctn) { if(graph) graph->ajouts = fctn; }
/* ========================================================================== */
void GraphOnClic(Graph graph, TypeGraphFctn fctn) { if(graph) graph->onclic = fctn; }
/* ========================================================================== */
int GraphDimGet(Graph graph) { if(graph) return(graph->dim); else return(0); }
/* ========================================================================== */
WndColor *GraphLUTCreate(int dim, int type) {
	WndColor *lut,*elt;
	int i,j,l,n,s;

	lut = (WndColor *)malloc(dim * sizeof(WndColor));
	if(!lut) return((WndColor *)-1);
	elt = lut;

	switch(type) {
		case GRF_LUT_ALL:
			n = dim / 5; l = n; s = WND_COLOR_MAX / n;
			i = 0; j = 0;
			for(; i<l; i++, j+=s) WndColorSetByRGB(elt++, 0,j,WND_COLOR_MAX); l += n;
			for(; i<l; i++, j-=s) WndColorSetByRGB(elt++, 0,WND_COLOR_MAX,j); l += n;
			for(; i<l; i++, j+=s) WndColorSetByRGB(elt++, j,WND_COLOR_MAX,0); l += n;
			for(; i<l; i++, j-=s) WndColorSetByRGB(elt++, WND_COLOR_MAX,j,0); l += n;
			for(; i<l; i++, j+=s) WndColorSetByRGB(elt++, WND_COLOR_MAX,0,j);
			for(; i<dim; i++)     WndColorSetByRGB(elt++, WND_COLOR_MAX,WND_COLOR_MAX,WND_COLOR_MAX);
			if(GraphQualite) {
				for(; i<dim; i++) WndColorSetByRGB(elt++, WND_COLOR_MAX,WND_COLOR_MAX,WND_COLOR_MAX);
				WndColorSetByRGB(lut,0,0,0);
			} else {
				for(; i<dim; i++) WndColorSetByRGB(elt++, 0,0,0);
				WndColorSetByRGB(lut,WND_COLOR_MAX,WND_COLOR_MAX,WND_COLOR_MAX);
			}
			break;
		case GRF_LUT_RED:
			n = dim / 3; l = n; s = WND_COLOR_MAX / n;
			i = 0; j = 0;
			for(; i<l; i++, j+=s) WndColorSetByRGB(elt++, j,0,0); l += n; j = 0;
			for(; i<l; i++, j+=s) WndColorSetByRGB(elt++, WND_COLOR_MAX,j,0); l += n; j = 0;
			for(; i<l; i++, j+=s) WndColorSetByRGB(elt++, WND_COLOR_MAX,WND_COLOR_MAX,j); l += n;
			for(; i<dim; i++)     WndColorSetByRGB(elt++, WND_COLOR_MAX,WND_COLOR_MAX,WND_COLOR_MAX);
			WndColorSetByRGB(lut,0,0,0);
			break;
		case GRF_LUT_GREEN:
			n = dim / 3; l = n; s = WND_COLOR_MAX / n;
			i = 0; j = 0;
			for(; i<l; i++, j+=s) WndColorSetByRGB(elt++, 0,j,0); l += n; j = 0;
			for(; i<l; i++, j+=s) WndColorSetByRGB(elt++, 0,WND_COLOR_MAX,j); l += n; j = 0;
			for(; i<l; i++, j+=s) WndColorSetByRGB(elt++, j,WND_COLOR_MAX,WND_COLOR_MAX); l += n;
			for(; i<dim; i++)     WndColorSetByRGB(elt++, WND_COLOR_MAX,WND_COLOR_MAX,WND_COLOR_MAX);
			WndColorSetByRGB(lut,0,0,0);
			break;
		case GRF_LUT_BLUE:
			n = dim / 3; l = n; s = WND_COLOR_MAX / n;
			i = 0; j = 0;
			for(; i<l; i++, j+=s) WndColorSetByRGB(elt++, 0,0,j); l += n; j = 0;
			for(; i<l; i++, j+=s) WndColorSetByRGB(elt++, j,0,WND_COLOR_MAX); l += n; j = 0;
			for(; i<l; i++, j+=s) WndColorSetByRGB(elt++, WND_COLOR_MAX,j,WND_COLOR_MAX); l += n;
			for(; i<dim; i++)     WndColorSetByRGB(elt++, WND_COLOR_MAX,WND_COLOR_MAX,WND_COLOR_MAX);
			WndColorSetByRGB(lut,0,0,0);
			break;
	}
	return(lut);
}
/* ========================================================================== */
void *GraphDataGet(Graph graph, int num, int *dim) {
	GraphData data;

	if(!graph) return(0);
	if((num < 0) || (num >= graph->dim) ) {
		OpiumError("(%s) Numero de tableau invalide (%d/%d)",__func__,num,graph->dim);
		return(0);
	}
	data = graph->data_ptr[num];
	if(!data) {
		OpiumError("(%s) Tableau #%d pas alloue en memoire",__func__,num); return(0);
	}
	*dim = data->dim;
	return(data->fctn);
}
/* ========================================================================== */
char GraphDataDisconnect(Graph graph, int num) {
	GraphData data;

	if(!graph) return(-1);
	if((num < 0) || (num >= graph->dim) ) {
		OpiumError("(%s '%s'#%d) Numero de tableau invalide (dim=%d)",__func__,
			(graph->cdr)->titre,num,graph->dim);
		return(-1);
	}
	data = graph->data_ptr[num];
	if(!data) {
		OpiumError("(%s) Tableau #%d pas alloue en memoire",__func__,num); return(-1);
	}
	data->fctn = (void *)-1;
	return(1);
}
/* ========================================================================== */
char GraphDataRemove(Graph graph, int num) {
	GraphData data; int i,qual;

	if(!graph) return(-1);
	if((num < 0) || (num >= graph->dim) ) {
		OpiumError("(%s '%s'#%d) Numero de tableau invalide (dim=%d)",__func__,
			(graph->cdr)->titre,num,graph->dim);
		return(-1);
	}
	data = graph->data_ptr[num];
	if(data) {
		for(qual=0; qual<WND_NBQUAL; qual++) if(data->gc[qual]) {
			WndContextFree((graph->cdr)->f,data->gc[qual]); data->gc[qual] = 0;
		}
	}
	if(num < graph->max) data->fctn = (void *)-1;
	else {
		if(data) free(data);
		graph->dim -= 1;
		for(i=num; i<graph->dim; i++) graph->data_ptr[i] = graph->data_ptr[i+1];
	}
	return(1);
}
/* ========================================================================== */
int GraphDataTranslate(Graph graph, int num, char transfo) {
	GraphData data;

	if(!graph) return(-1);
	if((num < 0) || (num >= graph->dim) ) {
		OpiumError("(%s '%s'#%d) Numero de tableau invalide (dim=%d)",__func__,
			(graph->cdr)->titre,num,graph->dim);
		return(0);
	}
	data = graph->data_ptr[num];
	if(!data) {
		OpiumError("(%s) Tableau #%d pas alloue en memoire",__func__,num); return(0);
	}
	data->parm &= (GRF_AXIS | GRF_ADRS | GRF_TYPE);
	data->parm |=  transfo;
	return(1);
}
/* ========================================================================== */
int GraphDataIntGamma(Graph graph, int num, int xmin, int xmax, int vmin, int vmax) {
	GraphData data;

	if(!graph) return(-1);
	if((num < 0) || (num >= graph->dim) ) {
		OpiumError("(%s '%s'#%d) Numero de tableau invalide (dim=%d)",__func__,
			(graph->cdr)->titre,num,graph->dim);
		return(0);
	}
	data = graph->data_ptr[num];
	if(!data) {
		OpiumError("(%s) Tableau #%d pas alloue en memoire",__func__,num); return(0);
	}
	if((data->parm & GRF_TYPE) == GRF_FLOAT) {
		OpiumError("(%s) Tableau #%d de type float",__func__,num); return(0);
	}
	data->gamma.i.xmin = xmin;
	data->gamma.i.xmax = xmax;
	data->gamma.i.vmin = vmin;
	data->gamma.i.vmax = vmax;
	data->parm &= (GRF_AXIS | GRF_ADRS | GRF_TYPE);
	data->parm |=  GRF_GAMMA;
	if(DEBUG_GRAPH(2))
		WndPrint("(%s) gamma: [%d .. %d] -> [%d .. %d]\n",__func__,
			data->gamma.i.xmin,data->gamma.i.xmax,data->gamma.i.vmin,data->gamma.i.vmax);
	return(1);
}
/* ========================================================================== */
int GraphDataFloatGamma(Graph graph, int num, float xmin, float xmax, float vmin, float vmax) {
	GraphData data;

	if(!graph) return(-1);
	if((num < 0) || (num >= graph->dim) ) {
		OpiumError("(%s %s,%d) Numero de tableau invalide (dim=%d)",__func__,
			(graph->cdr)->titre,num,graph->dim);
		return(0);
	}
	data = graph->data_ptr[num];
	if(!data) {
		OpiumError("(%s) Tableau #%d pas alloue en memoire",__func__,num); return(0);
	}
	if((data->parm & GRF_TYPE) != GRF_FLOAT) {
		OpiumError("(%s) Tableau #%d de type entier",__func__,num); return(0);
	}
	data->gamma.r.xmin = xmin;
	data->gamma.r.xmax = xmax;
	data->gamma.r.vmin = vmin;
	data->gamma.r.vmax = vmax;
	data->parm &= (GRF_AXIS | GRF_ADRS | GRF_TYPE);
	data->parm |=  GRF_GAMMA;
	if(DEBUG_GRAPH(1)) WndPrint("(%s) gamma: [%g .. %g] -> [%g .. %g]\n",__func__,
		data->gamma.r.xmin,data->gamma.r.xmax,data->gamma.r.vmin,data->gamma.r.vmax);
	return(1);
}
/* ========================================================================== */
int GraphDataIntOffset(Graph graph, int num, int ival) {
	GraphData data;

	if(!graph) return(-1);
	if((num < 0) || (num >= graph->dim) ) {
		OpiumError("(%s '%s'#%d) Numero de tableau invalide (dim=%d)",
				   __func__,(graph->cdr)->titre,num,graph->dim);
		return(0);
	}
	data = graph->data_ptr[num];
	if(!data) {
		OpiumError("(%s) Tableau #%d pas alloue en memoire",__func__,num); return(0);
	}
	data->of7.i.v = ival;
	return(1);
}
/* ========================================================================== */
int GraphDataFloatOffset(Graph graph, int num, float rval) {
	GraphData data;

	if(!graph) return(-1);
	if((num < 0) || (num >= graph->dim) ) {
		OpiumError("(%s '%s'#%d) Numero de tableau invalide (dim=%d)",__func__,
				   (graph->cdr)->titre,num,graph->dim);
		return(0);
	}
	data = graph->data_ptr[num];
	if(!data) {
		OpiumError("(%s) Tableau #%d pas alloue en memoire",__func__,num); return(0);
	}
	data->of7.r.v = rval;
	return(1);
}
/* ========================================================================== */
int GraphDataName(Graph graph, int num, char *nom) {
	GraphData data;

	if(!graph) return(-1);
	if(!nom) return(-1);
	if((num < 0) || (num >= graph->dim) ) {
		OpiumError("(%s '%s'#%d) Numero de tableau invalide (dim=%d)",__func__,
				   (graph->cdr)->titre,num,graph->dim);
		return(0);
	}
	data = graph->data_ptr[num];
	if(!data) {
		OpiumError("(%s) Tableau #%d pas alloue en memoire",__func__,num); return(0);
	}
	strncpy(data->titre,nom,GRF_MAX_TITRE);
	return(1);
}
/* ========================================================================== */
int GraphDataBmp(Graph graph, int num, WndColor *lut, int dim) {
	GraphData data;

	if(!graph) return(-1);
	if((num < 0) || (num >= graph->dim) ) {
		OpiumError("(%s '%s'#%d) Numero de tableau invalide (dim=%d)",__func__,
			(graph->cdr)->titre,num,graph->dim);
		return(0);
	}
	data = graph->data_ptr[num];
	if(!data) { OpiumError("(%s) Tableau #%d pas alloue en memoire",__func__,num); return(0); }
	data->couleur.type = GRF_BMP;
	data->couleur.def.lut.dim = dim;
	data->couleur.def.lut.val = lut;
	//- printf("(%s) LUT[%d] @%08lX\n",__func__,data->couleur[f->qualite].def.lut.dim,(Ulong)(data->couleur[f->qualite].def.lut.val));
	graph->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphDataColor(Graph graph, int num, char *nom) {
	GraphData data; int qual;

	if(!graph) return(-1);
	if((num < 0) || (num >= graph->dim) ) {
		OpiumError("(%s '%s'#%d) Numero de tableau invalide (dim=%d)",
				   __func__,(graph->cdr)->titre,num,graph->dim);
		return(0);
	}
	data = graph->data_ptr[num];
	if(!data) {
		OpiumError("(%s) Tableau #%d pas alloue en memoire",__func__,num); return(0);
	}
	data->couleur.type = GRF_NOM;
	for(qual=0; qual<WND_NBQUAL; qual++) strncpy(&(data->couleur.def.nom[qual][0]),nom,WND_MAXCOULEUR);
	graph->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphDataRGB(Graph graph, int num, WndColorLevel r, WndColorLevel g, WndColorLevel b) {
	GraphData data; int qual;

	if(!graph) return(-1);
	if((num < 0) || (num >= graph->dim) ) {
		OpiumError("(%s '%s'#%d) Numero de tableau invalide (dim=%d)",
			__func__,(graph->cdr)->titre,num,graph->dim);
		return(0);
	}
	data = graph->data_ptr[num];
	if(!data) {
		OpiumError("(%s) Tableau #%d pas alloue en memoire",__func__,num); return(0);
	}
	data->couleur.type = GRF_RGB;
	for(qual=0; qual<WND_NBQUAL; qual++) {
		data->couleur.def.rgb[qual].r = r;
		data->couleur.def.rgb[qual].g = g;
		data->couleur.def.rgb[qual].b = b;
	}
	graph->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphDataSupportColor(Graph graph, int num, int qual, char *nom) {
	GraphData data;

	if(!graph) return(-1);
	if((num < 0) || (num >= graph->dim) ) {
		OpiumError("(%s '%s'#%d) Numero de tableau invalide (dim=%d)",
				   __func__,(graph->cdr)->titre,num,graph->dim);
		return(0);
	}
	data = graph->data_ptr[num];
	if(!data) {
		OpiumError("(%s) Tableau #%d pas alloue en memoire",__func__,num); return(0);
	}
	data->couleur.type = GRF_NOM;
	strncpy(&(data->couleur.def.nom[qual][0]),nom,WND_MAXCOULEUR);
	graph->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphDataSupportRGB(Graph graph, int num, int qual, WndColorLevel r, WndColorLevel g, WndColorLevel b) {
	GraphData data;

	if(!graph) return(-1);
	if((num < 0) || (num >= graph->dim) ) {
		OpiumError("(%s '%s'#%d) Numero de tableau invalide (dim=%d)",
				   __func__,(graph->cdr)->titre,num,graph->dim);
		return(0);
	}
	data = graph->data_ptr[num];
	if(!data) {
		OpiumError("(%s) Tableau #%d pas alloue en memoire",__func__,num); return(0);
	}
	data->couleur.type = GRF_RGB;
	data->couleur.def.rgb[qual].r = r;
	data->couleur.def.rgb[qual].g = g;
	data->couleur.def.rgb[qual].b = b;
	// printf("(%s) data[%d] %s: %04X.%04X.%04X\n",__func__,num,WndSupportNom[qual],r,g,b);
	graph->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphDataLUT(Graph graph, int num, WndColor *lut, int dim) {
	GraphData data;

	if(!graph) return(-1);
	if((num < 0) || (num >= graph->dim) ) {
		OpiumError("(%s '%s'#%d) Numero de tableau invalide (dim=%d)",__func__,
			(graph->cdr)->titre,num,graph->dim);
		return(0);
	}
	data = graph->data_ptr[num];
	if(!data) {
		OpiumError("(%s) Tableau #%d pas alloue en memoire",__func__,num); return(0);
	}
	data->couleur.type = GRF_LUT;
	data->couleur.def.lut.dim = dim;
	data->couleur.def.lut.val = lut;
	return(1);
}
/* ========================================================================== */
int GraphDataTrace(Graph graph, int num, int mode, int type) {
	int qual; int rc;

	for(qual=0; qual<WND_NBQUAL; qual++) {
		rc = GraphDataSupportTrace(graph,num,qual,mode,type,GraphWidzDefaut[qual]);
		if(rc != 1) return(rc);
	}
	return(1);
}
/* ========================================================================== */
int GraphDataSupportTrace(Graph graph, int num, int qual, int mode, int type, int width) {
	GraphData data;

	if(!graph) return(-1);
	if((num < 0) || ((num >= graph->dim) && (graph->dim != 0))) return(0);
	data = graph->data_ptr[num];
	if(!data) return(0);
	//- printf("(%s) %s: epaisseur %d\n",__func__,data->titre,width);
	if(mode < 0) mode = 0; else if(mode > 3) mode = 3;
	if(type < 0) type = 0; else if(type > 7) type = 7;
	if(width < 1) width = 1; else if(width > 7) width = 7;
	data->trace[qual] = (unsigned char)(
		  ((unsigned char)(mode & 3) << GRF_SHIFT_MODE)
		| ((unsigned char)(type  & 7) << GRF_SHIFT_LINE)
		| ((unsigned char)(width & 7) << GRF_SHIFT_WIDZ));
	graph->pret = 0;
	return(1);
}
/* ========================================================================== */
char GraphDataStyle(Graph graph, int num, char style) {
	GraphData data;

	if(!graph) return(-1);
	if((num < 0) || ((num >= graph->dim) && (graph->dim != 0))) return(0);
	data = graph->data_ptr[num];
	if(!data) return(0);
	data->style = style;
	return(1);
}
/* ========================================================================== */
static void GraphInternalStart(GraphData data, int min) {
	if(!data) return;
	if(min < 0) min = 0;
	else if(min >= data->dim) min = data->dim - 1;
	data->min = min;
}
/* ========================================================================== */
static void GraphInternalEnd(GraphData data, int max) {
	if(!data) return;
	if(max < 0) max = 0;
	else if(max >= data->dim) max = data->dim - 1;
	data->max = max;
}
/* ========================================================================== */
static void GraphInternalUse(GraphData data, int nb) {
	if(!data) return;
//	WndPrint(" usage de %d valeurs (maxi %d);",nb,data->dim);
	if(nb < 0) nb = 0; else if(nb > data->dim) nb = data->dim;
	if(nb == 0) data->min = data->max = 0;
	else {
		data->max = data->min + nb;
		if(data->max > data->dim) data->max -= data->dim;
	}
//	WndPrint(" 1ere: %d, derniere: %d\n",data->min,data->max);
}
/* ========================================================================== */
static void GraphInternalScroll(GraphData data, int nb) {
	int of7,tours;
	char pas_vide;

	if(!data) return;
	if(data->dim == 0) return;
/*	if(nb < -data->dim) nb = -data->dim;
	else if(nb > data->dim) nb = data->dim; */
	tours = nb / data->dim;
	of7 = nb - (tours * data->min);
	if(!of7) return;
	data->min += of7;
	if(data->min < 0) data->min += data->dim;
	else if(data->min >= data->dim) data->min -= data->dim;
	data->max += of7;
	if(data->max == 0) {
		pas_vide = ((data->min != 0) || (data->max != 0));
		if(pas_vide) data->max = data->dim;
	} else if(data->max < 0) data->max += data->dim;
	else if(data->max > data->dim) data->max -= data->dim;
}
/* ========================================================================== */
static void GraphInternalRescroll(GraphData data, int nb) {
	int of7,tours,use;

	if(!data) return;
	if(data->dim == 0) return;
	tours = nb / data->dim;
	of7 = nb - (tours * data->dim);
/* --	if(nb == 0) continue; */
	use = data->max - data->min;
	if(((data->min != 0) || (data->max != 0)) && (use <= 0)) use += data->dim;
//	if(DEBUG_GRAPH(3)) {
//		WndPrint("    scroll de %d valeurs pour %d valeurs utilisees\n",
//			nb,use);
//		WndPrint("    soit %d tours de %d valeurs + decalage supplementaire de %d valeurs\n",
//			tours,data->dim,of7);
//	}
	data->min = of7;
//	if(DEBUG_GRAPH(3)) WndPrint("    premiere donnee a %d",data->min);
	if(data->min < 0) data->min += data->dim;
//	if(DEBUG_GRAPH(3)) WndPrint(" (en fait, %d)\n",data->min);
	data->max = data->min + use;
//	if(DEBUG_GRAPH(3)) WndPrint("    derniere donnee a %d",data->max);
	if(data->max == 0) {
		if(use) data->max = data->dim;
	} else if(data->max < 0) data->max += data->dim;
	else if(data->max > data->dim) data->max -= data->dim;
//	if(DEBUG_GRAPH(3)) WndPrint(" (en fait, %d)\n",data->max);
}
/* ========================================================================== */
void GraphDataStart(Graph graph, int num, int min) {
/* Demarre le trace a la valeur #<min> */
	GraphData data;

	if(!graph) return;
	if((num < 0) || (num >= graph->dim) ) {
		/* OpiumError("(GraphDataStart %s,%d) Numero de tableau invalide (dim=%d)",
			(graph->cdr)->titre,num,graph->dim); */ return;
	}
	data = graph->data_ptr[num];
	if(!data) {
		/* OpiumError("(GraphDataStart) Tableau pas alloue en memoire"); */ return;
	}
	GraphInternalStart(data,min);
	if(graph->axe[0].num == num) graph->axe[0].pret = 0;
	else if(graph->axe[1].num == num) graph->axe[1].pret = 0;
}
/* ========================================================================== */
void GraphDataEnd(Graph graph, int num, int max) {
	/* Demarre le trace a la valeur #<min> */
	GraphData data;

	if(!graph) return;
	if((num < 0) || (num >= graph->dim) ) {
		/* OpiumError("(GraphDataStart %s,%d) Numero de tableau invalide (dim=%d)",
			(graph->cdr)->titre,num,graph->dim); */ return;
	}
	data = graph->data_ptr[num];
	if(!data) {
		/* OpiumError("(GraphDataStart) Tableau pas alloue en memoire"); */ return;
	}
	GraphInternalEnd(data,max);
	if(graph->axe[0].num == num) graph->axe[0].pret = 0;
	else if(graph->axe[1].num == num) graph->axe[1].pret = 0;
}
/* ========================================================================== */
void GraphDataUse(Graph graph, int num, int nb) {
/* Limite le trace a <nb> valeurs */
	GraphData data;

	if(!graph) return;
	if((num < 0) || (num >= graph->dim) ) return;
	data = graph->data_ptr[num]; if(!data) return;
//	printf("(GraphDataUse) data[%d].nb=%d\n",num,nb);
	GraphInternalUse(data,nb);
	if(graph->axe[0].num == num) graph->axe[0].pret = 0;
	else if(graph->axe[1].num == num) graph->axe[1].pret = 0;
}
/* ========================================================================== */
void GraphDataScroll(Graph graph, int num, int nb) {
/* Demarre le trace a la valeur #+<nb> */
	GraphData data;

	if(!graph) return;
	if((num < 0) || (num >= graph->dim) ) return;
	data = graph->data_ptr[num]; if(!data) return;
	GraphInternalScroll(data,nb);
	if(graph->axe[0].num == num) graph->axe[0].pret = 0;
	else if(graph->axe[1].num == num) graph->axe[1].pret = 0;
}
/* ========================================================================== */
void GraphDataRescroll(Graph graph, int num, int nb) {
/* Demarre le trace a la valeur #+<nb> */
	GraphData data;

	if(!graph) return;
	if((num < 0) || (num >= graph->dim) ) return;
	data = graph->data_ptr[num]; if(!data) return;
	GraphInternalRescroll(data,nb);
	if(graph->axe[0].num == num) graph->axe[0].pret = 0;
	else if(graph->axe[1].num == num) graph->axe[1].pret = 0;
}
/* ========================================================================== */
void GraphStart(Graph graph, int min) {
/* Demarre le trace a la valeur #<min> */
	GraphData *data_ptr;
	int num;

	if(!graph) return;
	for(num = 0, data_ptr = graph->data_ptr; num < graph->dim ; num++,data_ptr++) {
		GraphInternalStart(*data_ptr,min);
	}
	graph->axe[0].pret = graph->axe[1].pret = 0;
}
/* ========================================================================== */
void GraphEnd(Graph graph, int max) {
	/* Termine le trace a la valeur #<max> */
	GraphData *data_ptr;
	int num;

	if(!graph) return;
	for(num = 0, data_ptr = graph->data_ptr; num < graph->dim ; num++,data_ptr++) {
		GraphInternalEnd(*data_ptr,max);
	}
	graph->axe[0].pret = graph->axe[1].pret = 0;
}
/* ========================================================================== */
void GraphReset(Graph graph) { if(graph) graph->pret = 0; }
/* ========================================================================== */
void GraphUse(Graph graph, int nb) {
	/* Limite le trace a <nb> valeurs */
	GraphData *data_ptr;
	int num;

	if(!graph) return;
//	printf("(GraphUse) graphe %s[%d], use(%d)\n",(graph->cdr)->nom,graph->dim,nb);
	for(num = 0, data_ptr = graph->data_ptr; num < graph->dim ; num++,data_ptr++) {
		GraphInternalUse(*data_ptr,nb);
	}
	graph->axe[0].pret = graph->axe[1].pret = 0;
}
/* ========================================================================== */
void GraphScroll(Graph graph, int nb) {
/* Demarre le trace a la valeur #+<nb> */
	GraphData *data_ptr;
	int num;

	if(!graph) return;
	for(num = 0, data_ptr = graph->data_ptr; num < graph->dim ; num++,data_ptr++) {
		GraphInternalScroll(*data_ptr,nb);
	}
	graph->axe[0].pret = graph->axe[1].pret = 0;
}
/* ========================================================================== */
void GraphRescroll(Graph graph, int nb) {
/* Demarre le trace a la valeur #+<nb> */
	GraphData *data_ptr;
	int num;

	if(!graph) return;
	for(num = 0, data_ptr = graph->data_ptr; num < graph->dim ; num++,data_ptr++) {
		GraphInternalRescroll(*data_ptr,nb);
	}
	graph->axe[0].pret = graph->axe[1].pret = 0;
}
/* ========================================================================== */
void GraphResize(Graph graph, int larg, int haut) {
	if(!graph) return;
	graph->axe[0].lngr = larg; graph->axe[1].lngr = haut;
}
/* ========================================================================== */
static char GraphScaleZoom(GraphScale *scale, unsigned int zoom) {
	if(zoom < 1) return(0);
	if(scale->zoom >= scale->lngr) scale->zoom = ((scale->lngr - 1) / 2) + 1; /* pas possible? */
	if(scale->zoom == 0) scale->zoom = 1;
	scale->lngr = scale->lngr / scale->zoom;
	scale->zoom = zoom;
	scale->lngr = scale->lngr * scale->zoom;
	return(1);
}
/* ========================================================================== */
void GraphZoom(Graph graph, unsigned int zoomh, unsigned int zoomv) {
#ifdef ANCIEN
	GraphScale *scale; unsigned int zoom[2]; int i; char touche;
	if(!graph) return;
	zoom[0] = zoomh; zoom[1] = zoomv;
	touche = 0;
	for(i=0; i<2; i++) if(zoom[i] >= 1) {
		scale = &(graph->axe[i]);
		if(scale->zoom >= scale->lngr) scale->zoom = scale->lngr; /* pas possible */
		scale->lngr = scale->lngr / scale->zoom;
		scale->zoom = zoom[i];
		scale->lngr = scale->lngr * scale->zoom;
		touche = 1;
	}
	if(touche) OpiumSizeGraph(graph->cdr,0);
#else  /* ANCIEN */
	char rc_x,rc_y;
	rc_x = GraphScaleZoom(&(graph->axe[0]),zoomh);
	rc_y = GraphScaleZoom(&(graph->axe[1]),zoomv);
	if(rc_x || rc_y) OpiumSizeGraph(graph->cdr,0);
#endif /* ANCIEN */
}
/* ========================================================================== */
void GraphDump(Graph graph) {
	GraphData data,*data_ptr; GraphScale *scale;
	int num; int i; int qual;

	if(!graph) return;
	WndPrint("=== Graph[%d] '%s' %d x %d *zoom %d x %d options %04X\n",
		graph->dim,(graph->cdr)->titre,graph->axe[0].lngr,graph->axe[1].lngr,
		graph->axe[0].zoom,graph->axe[1].zoom,graph->mode);
	for(i=0, scale=graph->axe; i<2; i++,scale++) {
		WndPrint("    Axe %c: ",i? 'Y': 'X');
		if(scale->num >= 0) {
			WndPrint("%s D[%d] ",(scale->reel)? "reel": "entier",scale->num);
			if(scale->minauto) WndPrint("(auto ");
			else if(scale->reel) WndPrint("(%g",scale->u.r.min);
			else WndPrint("(%d",scale->u.i.min);
			if(scale->maxauto) WndPrint(".. auto) ");
			else if(scale->reel) WndPrint(".. %g) ",scale->u.r.max);
			else WndPrint(".. %d) ",scale->u.i.max);
			if(scale->gradauto) WndPrint("/auto ");
			else WndPrint("/%g ",scale->grad);
			WndPrint("\n");
		} else WndPrint("indetermine\n");
	}
	for(num = 0, data_ptr = graph->data_ptr; num < graph->dim ; num++,data_ptr++) {
		data = *data_ptr; if(!data) continue;
		WndPrint("D[%d] (%d - %d): parm=%02X, fctn@%08lX",
			data->dim,data->min,data->max,data->parm,(Ulong)data->fctn);
		for(qual=0; qual<WND_NBQUAL; qual++) WndPrint(", trace %s=%02X",WndSupportNom[qual],data->trace[qual]);
		if(data->couleur.type == GRF_NOM)
			for(qual=0; qual<WND_NBQUAL; qual++) WndPrint(", couleur %s='%s'",WndSupportNom[qual],&(data->couleur.def.nom[qual][0]));
		else if(data->couleur.type == GRF_RGB)
			for(qual=0; qual<WND_NBQUAL; qual++) WndPrint(", couleur %s=rgb:%04X/%04X/%04X",WndSupportNom[qual],
				data->couleur.def.rgb[qual].r,data->couleur.def.rgb[qual].g,data->couleur.def.rgb[qual].b);
		else if(data->couleur.type == GRF_LUT)
			WndPrint(", couleur=lut");
		else WndPrint(", couleur de type %d (??..!)",data->couleur.type);
		WndPrint("\n");
	}
}
/* ========================================================================== */
int GraphAxisDefine(Graph graph, int num) {
/* Choisi le tableau definissant l'axe (x ou y selon data->parm) */
	GraphData data; GraphScale *scale;
	char reel;
	float rmin,rmax;

	if(!graph) return(0);
	if(graph->mode == GRF_DIRECT) return(0);
	if((num < 0) || (num >= graph->dim)) {
		OpiumError("(%s '%s'#%d) Numero de tableau invalide (dim=%d)",__func__,
			(graph->cdr)->titre,num,graph->dim);
		return(0);
	}

	data = graph->data_ptr[num];
	if(!data) {
		OpiumError("(%s) Tableau supprime",__func__);
		return(0);
	}
	scale = graph->axe + (((data->parm & GRF_AXIS) == GRF_YAXIS)? 1: 0);
	scale->num = (short)num;
	reel = (data->parm & GRF_TYPE) == GRF_FLOAT;
	/* Avant, on n'intervenait que sur ces cas de figure...
	if(scale->pret) {
		if(!scale->minauto) { };
		if(!scale->maxauto) { };
		if(!scale->minauto) { };
		if(!scale->maxauto) { };
	} else printf("(%s) Axe pas pret (%c)\n",__func__,reel? 'R': 'I');
	*/
	if(reel && !scale->reel) {
//		printf("(GraphAxisDefine) On avait   [%d .. %d] (I)\n",scale->u.i.min,scale->u.i.max);
		rmin = (float)scale->u.i.min; scale->u.r.min = rmin;
		rmax = (float)scale->u.i.max; scale->u.r.max = rmax;
//		printf("(GraphAxisDefine) On obtient [%g .. %g] (R)\n",scale->u.r.min,scale->u.r.max);
	} else if(!reel && scale->reel) {
//		printf("(GraphAxisDefine) On avait   [%g .. %g] (R)\n",scale->u.r.min,scale->u.r.max);
		rmin = scale->u.r.min; scale->u.i.min = (int)rmin;
		rmax = scale->u.r.max; scale->u.i.max = (int)rmax;
//		printf("(GraphAxisDefine) On obtient [%d .. %d] (I)\n",scale->u.i.min,scale->u.i.max);
	} /* else printf("(GraphAxisDefine) Extrema inchanges ([%g .. %g]) (%c -> %c)\n",
		scale->reel? scale->u.r.min: (float)scale->u.i.min,
		scale->reel? scale->u.r.max: (float)scale->u.i.max,
		scale->reel? 'R': 'I',reel? 'R': 'I'); */
	scale->pret = 0;
	scale->reel = reel;
	return(1);
}
/* ========================================================================== */
int GraphAxisTitle(Graph graph, unsigned char axe, char *titre) {
	GraphScale *scale; GraphData data; int num;

	if(!graph) return(0);
/*--	scale = graph->axe + (axe == GRF_YAXIS)? 1: 0; genere erreur: (int) -> (struct *) impossible */
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	strncpy(scale->titre,titre,80);
	num = scale->num;
	if((num >= 0) && (num < graph->dim)) {
		if((data = graph->data_ptr[num])) {
			if(data->titre[0] == '\0') strncpy(data->titre,titre,GRF_MAX_TITRE);
		}
	}
	graph->axe[1].pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphAxisSet(Graph graph, unsigned char axe) {
/* Impose de ne pas recalculer l'axe <axe> */
	GraphScale *scale;

	if(!graph) return(0);
/*--	scale = graph->axe + (axe == GRF_YAXIS)? 1: 0; genere erreur: (int) -> (struct *) impossible */
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	scale->pret = 1;
	return(1);
}
/* ========================================================================== */
int GraphAxisReset(Graph graph, unsigned char axe) {
/* Impose de recalculer l'axe <axe> */
	GraphScale *scale;

	if(!graph) return(0);
/*--	scale = graph->axe + (axe == GRF_YAXIS)? 1: 0; genere erreur: (int) -> (struct *) impossible */
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	scale->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphAxisFormat(Graph graph, unsigned char axe, char *format) {
/* Impose le format des graduations sur l'axe <axe> */
	GraphScale *scale;

	if(!graph) return(0);
/*--	scale = graph->axe + (axe == GRF_YAXIS)? 1: 0; genere erreur: (int) -> (struct *) impossible */
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	strncpy(scale->format,format,12);
	scale->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphAxisIntRange(Graph graph, unsigned char axe, int imin, int imax) {
	/* Impose l'intervalle des graduations sur l'axe <axe> si ce sont des entiers */
	GraphScale *scale;

	if(!graph) return(0);
	if(imin == imax) return(0);

	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	if((scale->num < 0) || (scale->num >= graph->dim)) scale->reel = 0;
	if(scale->reel) {
		scale->u.r.min = (float)imin; scale->u.r.max = (float)imax;
	} else {
		scale->u.i.min = imin; scale->u.i.max = imax;
	}
	scale->minauto = scale->maxauto = 0;
	scale->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphAxisFloatRange(Graph graph, unsigned char axe, float rmin, float rmax) {
/* Impose l'intervalle des graduations sur l'axe <axe> si ce sont des reels */
	GraphScale *scale;

	if(!graph) return(0);
	if(rmin == rmax) return(0);

	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	if((scale->num < 0) || (scale->num >= graph->dim)) scale->reel = 1;
	if(scale->reel) {
		scale->u.r.min = rmin; scale->u.r.max = rmax;
	} else {
		scale->u.i.min = (int)rmin; scale->u.i.max = (int)rmax;
	}
	scale->minauto = scale->maxauto = 0;
	scale->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphGetIntRange(Graph graph, unsigned char axe, int *imin, int *imax) {
	/* Recupere l'intervalle des graduations sur l'axe <axe> sous forme entiers */
	GraphScale *scale;

	if(!graph) return(0);
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	if((scale->num < 0) || (scale->num >= graph->dim)) scale->reel = 0;
	if(scale->reel) {
		*imin = (int)scale->u.r.min; *imax = (int)scale->u.r.max;
	} else {
		*imin = scale->u.i.min; *imax = scale->u.i.max;
	}
	return(1);
}
/* ========================================================================== */
int GraphGetFloatRange(Graph graph, unsigned char axe, float *rmin, float *rmax) {
	/* Recupere l'intervalle des graduations sur l'axe <axe> sous forme reels */
	GraphScale *scale;

	if(!graph) return(0);
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	if((scale->num < 0) || (scale->num >= graph->dim)) scale->reel = 0;
	if(scale->reel) {
		*rmin = scale->u.r.min; *rmax = scale->u.r.max;
	} else {
		*rmin = (float)scale->u.i.min; *rmax = (float)scale->u.i.max;
	}
	return(1);
}
/* ========================================================================== */
int GraphAxisAutoRange(Graph graph, unsigned char axe) {
/* Demande un calcul automatique de l'intervalle des graduations sur l'axe <axe> */
	GraphScale *scale;

	if(!graph) return(0);
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	scale->minauto = scale->maxauto = 1;
	scale->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphAxisIntMin(Graph graph, unsigned char axe, int imin) {
/* Impose le minimum des graduations sur l'axe <axe> si ce sont des entiers */
	GraphScale *scale;

	if(!graph) return(0);
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	if((scale->num < 0) || (scale->num >= graph->dim)) scale->reel = 0;
	if(scale->reel) scale->u.r.min = (float)imin; else scale->u.i.min = imin;
	scale->minauto = 0;
	scale->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphAxisFloatMin(Graph graph, unsigned char axe, float rmin) {
/* Impose le minimum des graduations sur l'axe <axe> si ce sont des reels */
	GraphScale *scale;

	if(!graph) return(0);
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	if((scale->num < 0) || (scale->num >= graph->dim)) scale->reel = 1;
	if(scale->reel) scale->u.r.min = rmin; else scale->u.i.min = (int)rmin;
	scale->minauto = 0;
	scale->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphAxisAutoMin(Graph graph, unsigned char axe) {
/* Demande un calcul automatique du minimum des graduations sur l'axe <axe> */
	GraphScale *scale;

	if(!graph) return(0);
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	scale->minauto = 1;
	scale->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphAxisIntMax(Graph graph, unsigned char axe, int imax) {
/* Impose le maximum des graduations sur l'axe <axe> si ce sont des entiers */
	GraphScale *scale;

	if(!graph) return(0);
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	if((scale->num < 0) || (scale->num >= graph->dim)) scale->reel = 0;
	if(scale->reel) scale->u.r.max = (float)imax; else scale->u.i.max = imax;
	scale->maxauto = 0;
	scale->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphAxisFloatMax(Graph graph, unsigned char axe, float rmax) {
/* Impose le maximum des graduations sur l'axe <axe> si ce sont des reels */
	GraphScale *scale;

	if(!graph) return(0);
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	if((scale->num < 0) || (scale->num >= graph->dim)) scale->reel = 1;
	if(scale->reel) scale->u.r.max = rmax; else scale->u.i.max = (int)rmax;
	scale->maxauto = 0;
	scale->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphAxisAutoMax(Graph graph, unsigned char axe) {
/* Demande un calcul automatique du maximum des graduations sur l'axe <axe> */
	GraphScale *scale;

	if(!graph) return(0);
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	scale->maxauto = 1;
	scale->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphAxisIntGrad(Graph graph, unsigned char axe, int igrad) {
/* Impose l'intervalle entre 2 graduations sur l'axe <axe> si ce sont des entiers */
	GraphScale *scale;

	if(!graph) return(0);
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	if((scale->num < 0) || (scale->num >= graph->dim)) scale->reel = 0;
	scale->grad = (float)igrad;
	scale->gradauto = 0;
	scale->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphAxisFloatGrad(Graph graph, unsigned char axe, float rgrad) {
/* Impose l'intervalle entre 2 graduations sur l'axe <axe> si ce sont des reels */
	GraphScale *scale;

	if(!graph) return(0);
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	if((scale->num < 0) || (scale->num >= graph->dim)) scale->reel = 1;
	scale->grad = rgrad;
	scale->gradauto = 0;
	scale->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphAxisAutoGrad(Graph graph, unsigned char axe) {
/* Demande un calcul automatique de l'intervalle entre 2 graduations sur l'axe <axe> */
	GraphScale *scale;

	if(!graph) return(0);
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	scale->gradauto = 1;
	scale->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphAxisChgtUnite(Graph graph, unsigned char axe, float (*fctn)(Graph, int, float)) {
/* Change l'unite des graduations sur l'axe <axe> */
	GraphScale *scale;

	if(!graph) return(0);
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	scale->chgt_unite = (float (*)(void *, int, float))fctn;
	scale->pret = 0;
	return(1);
}
/* ========================================================================== */
int GraphAxisLog(Graph graph, unsigned char axe, char logd) {
/* Demande (ou supprime) un affichage en log decimal sur l'axe <axe> */
	GraphScale *scale;

	if(!graph) return(0);
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	scale->logd = logd;
	scale->pret = 0;
	return(1);
}
/* ========================================================================== */
char GraphAxisIsLog(Graph graph, unsigned char axe) {
/* Recupere les limites et graduation de l'axe <axe> si ce sont des entiers */
	GraphScale *scale;

	if(!graph) return(0);
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	return(scale->logd);
}
/* ========================================================================== */
int GraphAxisIntGet(Graph graph, unsigned char axe, int *imin, int *imax, int *igrad) {
	/* Recupere les limites et graduation de l'axe <axe> si ce sont des entiers */
	GraphScale *scale;

	if(!graph) return(0);
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	if((scale->num < 0) || (scale->num >= graph->dim)) scale->reel = 0;
	if(scale->reel) { *imin = (int)(scale->u.r.min); *imax = (int)(scale->u.r.max); }
	else { *imin = scale->u.i.min; *imax = scale->u.i.max; }
	*igrad = (int)scale->grad;
	return(1);
}
/* ========================================================================== */
int GraphAxisFloatGet(Graph graph, unsigned char axe, float *rmin, float *rmax, float *rgrad) {
/* Recupere les limites et graduation de l'axe <axe> si ce sont des reels */
	GraphScale *scale;

	if(!graph) return(0);
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	if((scale->num < 0) || (scale->num >= graph->dim)) scale->reel = 0;
	if(scale->reel) { *rmin = scale->u.r.min; *rmax = scale->u.r.max; }
	else { *rmin = (float)(scale->u.i.min); *rmax = (float)(scale->u.i.max); }
	*rgrad = scale->grad;
	return(1);
}
/* ========================================================================== */
int GraphAxisMarge(Graph graph, unsigned char axe) {
    // printf("(%s) marge_min[%d]=%d\n",__func__,axe,graph->axe[axe].marge_min);
    if(graph) return(graph->axe[axe].marge_min); else return(0);
}
/* ========================================================================== */
int GraphUserScrollX(Graph graph, int ix, float rx) {
	Cadre cdr;
	GraphScale *xscale;

	if(!graph) return(0);
	xscale = graph->axe;
	if(!xscale->pret) GraphScaleDefine(graph);
	cdr = graph->cdr;
	if(xscale->reel) cdr->dx = (int)((rx - xscale->u.r.min) * xscale->pixel);
	else cdr->dx = (ix - xscale->u.i.min) * xscale->utile / (int)xscale->ech;
	if(cdr->dx < 0) cdr->dx = 0;
	else if((cdr->dx + cdr->dh) > cdr->larg) cdr->dx = cdr->larg - cdr->dh;
//	WndPrint("(GraphUserScrollX) rx=%g soit dx=%d/%d\n",rx,cdr->dx,cdr->larg);
	return(1);
}
/* ========================================================================== */
int GraphUserScrollY(Graph graph, int iy, float ry) {
	Cadre cdr;
	GraphScale *yscale;

	if(!graph) return(0);
	yscale = graph->axe + 1;
	if(!yscale->pret) GraphScaleDefine(graph);
	cdr = graph->cdr;
	if(yscale->reel) cdr->dy = (int)((ry - yscale->u.r.min) * yscale->pixel);
	else cdr->dy = (iy - yscale->u.i.min) * yscale->utile / (int)yscale->ech;
	if(cdr->dy < 0) cdr->dy = 0;
	else if((cdr->dy + cdr->dv) > cdr->haut) cdr->dy = cdr->haut - cdr->dv;
	//	WndPrint("(GraphUserScrollX) rx=%g soit dx=%d/%d\n",rx,cdr->dx,cdr->larg);
	return(1);
}
/* ========================================================================== */
int GraphScreenXFloat(Graph graph, float r) {
	return(GraphPosFloat(1,r,graph->axe) - (graph->cdr)->dx);
}
/* ========================================================================== */
int GraphScreenXInt(Graph graph, int r) {
	return(GraphPosInt(1,r,graph->axe) - (graph->cdr)->dx);
}
/* ========================================================================== */
int GraphScreenYFloat(Graph graph, float r) {
	return((graph->cdr)->haut - GraphPosFloat(1,r,graph->axe + 1) - (graph->cdr)->dy);
}
/* ========================================================================== */
int GraphScreenYInt(Graph graph, int r) {
	return((graph->cdr)->haut - GraphPosInt(1,r,graph->axe + 1) - (graph->cdr)->dy);
}
/* ========================================================================== */
void GraphAjusteParms(Graph graph, unsigned char axe, GraphAxeParm *parms) {
	GraphScale *scale;

	if(!graph) return;
	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	if((scale->num < 0) || (scale->num >= graph->dim)) scale->reel = 1;
	scale->logd = parms->log;
	if(parms->autom) {
		scale->minauto = scale->maxauto = 1;
		if(scale->reel) {
			if(parms->log) scale->u.r.min = 0.001f; else scale->u.r.min = 0.0f;
		} else {
			if(parms->log) scale->u.i.min = 1; else scale->u.i.min = 0;
		}
	} else {
		if(scale->reel) {
			scale->u.r.min = parms->lim.r.min;
			scale->u.r.max = parms->lim.r.max;
		} else {
			scale->u.i.min = parms->lim.i.min;
			scale->u.i.max = parms->lim.i.max;
		}
		scale->minauto = scale->maxauto = 0;
	}
	scale->pret = 0;
}
/* ========================================================================== */
int GraphBlank(Graph graph) {
	Cadre cdr; WndFrame f;

	if(!graph) return(0);
	cdr = graph->cdr;
	if(!OpiumCoordFenetre(cdr,&f)) return(0);
	WndPaint(f,0,0,cdr->dh,cdr->dv,WndColorBgnd[GraphQualite]);
	return(1);
}
/* ========================================================================== */
inline static void GraphPlacePoint(WndFrame f, WndContextPtr gc, unsigned char trace,
	int x, int y) {
	switch(trace) {
	  case GRF_CODE_POINT:  /* un point */
		if(WndPSopened()) {
			OpiumPointPapier[0].x = x - GRF_DIM_POINT;
			OpiumPointPapier[0].y = y - GRF_DIM_POINT;
			WndPoly(f,gc,OpiumPointPapier,5,WND_REL);
		} else {
			GraphMarkOrigin(GRF_POINT,x - (GRF_DIM_POINT / 2),y - (GRF_DIM_POINT / 2));
			WndPoly(f,gc,OpiumMark[GRF_POINT],5,WND_REL);
		}
		break;
	  case GRF_CODE_CROIX:  /* une croix */
		GraphMarkOrigin(GRF_CROIX,x,y - (GRF_DIM_CROSS / 2));
		GraphMarkDraw(f,gc,GRF_CROIX,4);
		break;
	  case GRF_CODE_STAR:   /* une etoile */
		GraphMarkOrigin(GRF_STAR,x,y - (GRF_DIM_STAR / 2));
		GraphMarkDraw(f,gc,GRF_STAR,8);
		break;
	  case GRF_CODE_TRGLE:  /* un triangle */
		GraphMarkOrigin(GRF_TRGLE,x,y - (GRF_DIM_TRGLE / 2));
		WndPoly(f,gc,OpiumMark[GRF_TRGLE],4,WND_REL);
		break;
	  case GRF_CODE_CARRE:  /* un carre */
		GraphMarkOrigin(GRF_CARRE,x - (GRF_DIM_CARRE / 2),y - (GRF_DIM_CARRE / 2));
		WndPoly(f,gc,OpiumMark[GRF_CARRE],5,WND_REL);
		break;
	  case GRF_CODE_ROND:  /* un cercle */
		GraphMarkOrigin(GRF_ROND,x - (GRF_DIM_ROND / 4),y - (GRF_DIM_ROND / 2));
		WndPoly(f,gc,OpiumMark[GRF_ROND],9,WND_REL);
		break;
	}
}
/* ========================================================================== */
inline static void GraphTraceBarresErreur(WndFrame f, WndContextPtr gc,
	int x, GraphData xerr, int jx, GraphScale *xscale,
	int y, GraphData yerr, int jy, GraphScale *yscale) {
	int h,v;

	if(xerr) {
		h = GraphPosData(0,xerr,jx,xscale);
		WndLine(f,gc,x-h/2,y,h,0);
	}
	if(yerr) {
		v = GraphPosData(0,yerr,jy,yscale);
		WndLine(f,gc,x,y-v/2,0,v);
	}
}
/* ========================================================================== */
void OpiumGraphColorNb(Cadre cdr, Cadre fait, short *nc) {
	Graph graph,modele; short ic;

	if(!(graph = (Graph)cdr->adrs)) return;
	// printf("(%s) Decompte des couleurs %s%s '%s' C=%08lX dans F=%08lX\n",__func__,OpiumDuDela(cdr),OPIUMCDRTYPE(cdr->type),cdr->nom,(Ulong)cdr,(Ulong)(cdr->f));
	if(fait) modele = (Graph)fait->adrs; else modele = 0;
	if(modele) {
		graph->pomme = modele->pomme;
		graph->grille = modele->grille;
		graph->ajout = modele->ajout;
	} else {
		ic = *nc;
		graph->pomme = ic++; graph->grille = ic++; graph->ajout = ic++;
		*nc = ic;
	}
}
/* ========================================================================== */
void OpiumGraphColorSet(Cadre cdr, WndFrame f) {
	Graph graph;

	if(!(graph = (Graph)cdr->adrs)) return;
	// printf("(%s) Affectation des couleurs %s%s '%s' C=%08lX dans F=%08lX\n",__func__,OpiumDuDela(cdr),OPIUMCDRTYPE(cdr->type),cdr->nom,(Ulong)cdr,(Ulong)(cdr->f));
	WndFenColorSet(f,graph->pomme,WND_GC_BUTN);
	WndFenColorSet(f,graph->grille,WND_GC_DARK);
	WndFenColorSet(f,graph->ajout,WND_GC_ROUGE);
	// WndFenColorDump(f,WND_Q_ECRAN);
}
/* ========================================================================== */
int OpiumSizeGraph(Cadre cdr, char from_wm) {
	Graph graph;

	if(DEBUG_OPIUM(1) && (cdr->type != OPIUM_GRAPH)) {
		WndPrint("(%s) %s: %s '%s' C=%08llX\n",__func__,
			from_wm?"From WM":"From Opium",OPIUMCDRTYPE(cdr->type),cdr->nom,(IntAdrs)cdr->adrs);
		return(0);
	}

	graph = (Graph)cdr->adrs;
	if(!graph) return(0);
	if(from_wm) {
/*		marche si figure toujours complete -> si tj meme rapport affiche/total (=zoom) */
		cdr->larg = graph->axe[0].zoom * cdr->dh;
		cdr->haut = graph->axe[1].zoom * cdr->dv;
		graph->axe[0].lngr = cdr->larg;
		graph->axe[1].lngr = cdr->haut;
		graph->axe[0].pret = graph->axe[1].pret = 0;
		if(DEBUG_OPIUM(2)) GraphDump(graph);
	} else {
		cdr->larg = graph->axe[0].lngr;
		cdr->haut = graph->axe[1].lngr;
		if(graph->axe[0].zoom < cdr->larg) cdr->dh = cdr->larg / graph->axe[0].zoom;
		if(graph->axe[1].zoom < cdr->haut) cdr->dv = cdr->haut / graph->axe[1].zoom;
		if(DEBUG_OPIUM(2)) WndPrint("(%s) larg=%d, zoomh=%d soit dh=%d\n",__func__,cdr->larg,graph->axe[0].zoom,cdr->dh);
	}
	return(1);
}
#ifdef A_COMPLETER
/* ========================================================================== */
static void GraphTraceAxes(Graph graph, GraphScale *xscale, GraphScale *yscale, Cadre cdr, WndFrame f) {
	WndContextPtr gc,gc_ab6; unsigned char typegrad;
	int xp,yp;

	gc = f->text[WND_GC_BUTN];
	xp = cdr->dh - WndColToPix(1);
	yp = WndLigToPix(1);
	if(!WndPSopened()) {
		if(!cdr->support) WndEntoure(f,WND_PLAQUETTE,xp,0,WndColToPix(1)*3/2,WndLigToPix(1));
	#ifdef macintosh
		WndString(f,gc,xp,yp,"ð");
	#else
		WndString(f,gc,xp,yp,"?");
	#endif
	}
	gc = gc_ab6;
	if(cdr->titre[0]) {
		xp = (cdr->dh - WndColToPix(strlen(cdr->titre))) / 2;
		yp = cdr->dv;
		WndString(f,gc,xp,yp,cdr->titre);
	}
	if(((graph->mode & GRF_NBAXES) != GRF_0AXES) && (graph->mode != GRF_DIRECT)) {
		GraphGradMin(xscale,cdr->dx);
		GraphGradMin(yscale,cdr->dy);
		ym = (WndLigToPix(3) / 2) + graph->of7 + graph->grad; /* WndLigToPix(3) */

		xaxe1 = yaxe1 = xaxe2 = yaxe2 = -1;
		if(((graph->mode & GRF_NBAXES) == GRF_2AXES) && (f->qualite == WND_Q_ECRAN)) {
			if(xscale->reel) xaxe1 = GraphPosFloat(1,xscale->u.r.axe,xscale);
			else xaxe1 = GraphPosInt(1,xscale->u.i.axe,xscale);
			if(yscale->reel) yaxe1 = GraphPosFloat(1,yscale->u.r.axe,yscale);
			else yaxe1 = GraphPosInt(1,yscale->u.i.axe,yscale);
			GraphAxisXDraw(graph,yaxe1,gc,yscale->typegrad);
			GraphAxisYDraw(graph,xaxe1,gc,xscale->typegrad);
			if(xscale->titre[0]) {
				xp = cdr->dh - WndColToPix(strlen(xscale->titre));
				yp = cdr->dv - yaxe1 + graph->grad + WndLigToPix(2);
				WndString(f,gc,xp,yp,xscale->titre);
			}
			if(yscale->titre[0]) {
				/* xp = xaxe1 - WndColToPix(strlen(yscale->titre)); */
				xp = xaxe1 - xscale->cols - graph->grad;
				yp = ym;
				WndString(f,gc,xp,yp,yscale->titre);
			}

		} else if(((graph->mode & GRF_NBAXES) == GRF_4AXES) || (f->qualite == WND_Q_PAPIER)) {
			if(xscale->reel) xaxe1 = GraphPosFloat(1,xscale->u.r.min,xscale);
			else xaxe1 = GraphPosInt(1,xscale->u.i.min,xscale);
			if(yscale->reel) yaxe1 = GraphPosFloat(1,yscale->u.r.min,yscale);
			else yaxe1 = GraphPosInt(1,yscale->u.i.min,yscale);
			GraphAxisXDraw(graph,yaxe1,gc,GRF_GRADINF);
			GraphAxisYDraw(graph,xaxe1,gc,GRF_GRADINF);
			if(xscale->titre[0]) {
				xp = cdr->dh - WndColToPix(strlen(xscale->titre));
				yp = cdr->dv - yaxe1 + graph->grad + WndLigToPix(2);
				WndString(f,gc,xp,yp,xscale->titre);
			}
			if(yscale->titre[0]) {
				/* xp = xaxe1 - WndColToPix(strlen(yscale->titre)); */
				xp = xaxe1 - xscale->cols - graph->grad;
				yp = ym;
				WndString(f,gc,xp,yp,yscale->titre);
			}
			if(xscale->reel) xaxe2 = GraphPosFloat(1,xscale->u.r.max,xscale);
			else xaxe2 = GraphPosInt(1,xscale->u.i.max,xscale);
			if(yscale->reel) yaxe2 = GraphPosFloat(1,yscale->u.r.max,yscale);
			else yaxe2 = GraphPosInt(1,yscale->u.i.max,yscale);
			typegrad = (f->qualite == WND_Q_ECRAN)? GRF_GRADSUP: GRF_GRADNONE;
			GraphAxisXDraw(graph,yaxe2,gc,typegrad);
			GraphAxisYDraw(graph,xaxe2,gc,typegrad);
			/*
			 if(xscale->titre[0]) {
			 xp = cdr->larg - WndColToPix(strlen(xscale->titre));
			 yp = cdr->haut - WndLigToPix(1);
			 WndString(f,gc,xp,yp,xscale->titre);
			 }
			 if(yscale->titre[0]) WndString(f,gc,0,ym,yscale->titre);
			 */
		}

		gc = OpiumPSenCours? WND_BUTN: WND_DARK; // PS: WND_LITE trop clair, ecran: WND_SDRK trop fonce
		if(graph->mode & GRF_GRID) {
			int xc,yc,xlim,ylim,timeout,nbgrad;
			float rx,ry;

			nbgrad = 1; /* pour gcc */
			if(xscale->logd) {
				rx = xscale->g1 * pow(10.0,(double)xscale->gmin);
				if(xscale->grad > 0.0f) xscale->grad = rx;
				nbgrad = 9;
			} else rx = xscale->g1 + ((float)xscale->gmin * xscale->grad);
			xc = GraphPosFloat(1,rx,xscale) - cdr->dx;
			xlim = xscale->marge_min + xscale->utile - cdr->dx;
			if(xlim > cdr->dh) xlim = cdr->dh;
			timeout = 0;
			xcp = xaxe1;
			do {
				if((xc >= xscale->marge_min) && (xc != xaxe1) && (xc != xaxe2) && (abs(xc - xcp) > 0)) {
					GraphAxisYDraw(graph,xc,gc,GRF_GRADNONE);
					xcp = xc;
				}
				if(xscale->logd) {
					if(xscale->grad < 0.0f) rx *= 10.0f;
					else {
						rx += xscale->grad;
						if(--nbgrad == 0) { xscale->grad *= 10.0f; nbgrad = 9; xcp = xaxe1; }
					}
				} else rx += xscale->grad;
				xc = GraphPosFloat(1,rx,xscale) - cdr->dx;
				if(++timeout > 100) break;
			} while(xc <= xlim);

			if(yscale->logd) {
				ry = yscale->g1 * pow(10.0,(double)xscale->gmin);
				if(yscale->grad > 0.0f) yscale->grad = ry;
				nbgrad = 9;
			} else ry = yscale->g1 + (yscale->gmin * yscale->grad);
			yc = GraphPosFloat(1,ry,yscale) + cdr->dy;
			ylim = cdr->haut - yscale->marge_min + cdr->dy;
			if(ylim > cdr->haut) ylim = cdr->haut;
			timeout = 0;
			ycp = yaxe1;
			do {
				if((yc >= yscale->marge_min) && (yc != yaxe1) && (yc != yaxe2) && (abs(yc - ycp) > 0)) {
					GraphAxisXDraw(graph,yc,gc,GRF_GRADNONE);
					ycp = yc;
				}
				if(yscale->logd) {
					if(yscale->grad < 0.0f) ry *= 10.0f;
					else {
						ry += yscale->grad;
						if(--nbgrad == 0) { yscale->grad *= 10.0f; nbgrad = 9; ycp = yaxe1; }
					}
				} else ry += yscale->grad;
				yc = GraphPosFloat(1,ry,yscale) + cdr->dy;
				if(++timeout > 100) break;
			} while(yc <= ylim);
		}
	}

	return;
}
#endif /* A_COMPLETER */
/* ========================================================================== */
int OpiumRefreshGraph(Cadre cdr) { return(GraphTrace(cdr,0)); }
/* ========================================================================== */
int GraphTrace(Cadre cdr, int ajoutes) {
	Graph graph;
	GraphData *data_ptr,data,data_ab6,data_ord;
	GraphData xerr_demande,yerr_demande,xerr,yerr;
	GraphScale *xscale,*yscale;
	TypeGraphFctn ajoute;
	WndFrame f; WndContextPtr gc,gc_ab6; WndPoint *p;
	int nbpts;
	TypeGraphIndiv *indiv_demande,*indiv,*indiv_point;
	int i,j,k,ii,jx,jy,jxmin,ab6,nb,nb_ab6,nb_ord,num,lx,nbx,nby;
	int x,y,xp,yp,xm,ym,xc,yc,xaxe1,yaxe1,xaxe2,yaxe2,xcp,ycp,axeX;
	int usr_i,ymin_i,ymax_i,ytot_i; float usr_r,ymin_r,ymax_r,ytot_r;
	int xcur,yvus,xnext,type; char niveau_detail,premier,pas_commence;
	char qualite_graph,qualite_fen;
	unsigned char mode,trace,trace_generale,trace_point,typegrad;
	char doit_fermer,doit_terminer,complet,reel,court,fctn,histo_libre;

	if(cdr->type != OPIUM_GRAPH) {
		if(DEBUG_OPIUM(1)) WndPrint("(%s) +++ Cadre '%s' @%08llX\n",__func__,cdr->nom,(IntAdrs)cdr->adrs);
		return(0);
	}
//	WndPrint("(OpiumRefreshGraph) Cadre '%s' @%08lX (graph @%08lX)\n",cdr->nom,(Ulong)cdr,(Ulong)cdr->adrs);
//	OpiumDumpConnexions(__func__,cdr);
	graph = (Graph)cdr->adrs; if(!graph) return(0);
/*
 * Preparation du fond de fenetre
 */
	if(!OpiumCoordFenetre(cdr,&f)) return(0);
	if(DEBUG_OPIUM(1)) printf("(%s) Affichage %s%s '%s' C=%08llX dans F=%08llX\n",__func__,OpiumDuDela(cdr),OPIUMCDRTYPE(cdr->type),cdr->nom,(IntAdrs)cdr,(IntAdrs)(cdr->f));
	complet = (ajoutes == 0);
	qualite_graph = f->qualite; // (char)((graph->mode & GRF_QUALITE) >> 5);
	qualite_fen = f->qualite;

//	WndErase(f,0,0,cdr->dh,cdr->dv); laisse en blanc le fond dans les planches
	doit_terminer = WndRefreshBegin(f);
	if(complet) {
		// PRINT_COLOR(WndColorBgnd[qualite_graph]);
		if(cdr->planche) WndPaint(f,0,0,cdr->dh,cdr->dv,WndColorBgnd[qualite_graph]);
		else WndPaint(f,0,0,cdr->dh+WND_ASCINT_WIDZ,cdr->dv+WND_ASCINT_WIDZ,WndColorBgnd[qualite_graph]);
	}
	if(GraphScaleDefine(graph)) strcpy(GraphErrorText,"jusque la, ca va...");
	else { WndWrite(f,0,1,1,GraphErrorText); if(doit_terminer) WndRefreshEnd(f); return(0); }
	xscale = graph->axe;
	yscale = xscale + 1;
	premier = 0; nb_ab6 = xcur = yvus = ytot_i = x = y = 0; ytot_r = 0.0f; data_ab6 = 0; // compilo

/*
 * Trace des axes
 */
	gc_ab6 = graph->data_ptr[xscale->num]->gc[f->qualite];
	// printf("(%s) gc_ab6 = data[%d](@%08lX).gc[%d] @%08lX\n",__func__,xscale->num,(Ulong)&(graph->data_ptr[xscale->num]),f->qualite,(Ulong)gc_ab6);
	// PRINT_GC(gc_ab6); PRINT_COLOR(gc_ab6->foreground); PRINT_COLOR(gc_ab6->background);
	if(complet) /* GraphTraceAxes(graph,xscale,yscale,cdr,f); */ {
		xp = cdr->dh - WndColToPix(1);
		yp = WndLigToPix(1);
		if(!WndPSopened()) {
			if(!cdr->support) WndEntoure(f,WND_PLAQUETTE,xp,0,WndColToPix(1)*3/2,WndLigToPix(1));
			gc = GRF_POMME;
		#ifdef QUICKDRAW
			// WndString(f,gc,xp,yp,"ð");
			unsigned char icone[2]; icone[0] = 0xF0; icone[1] = 0; WndString(f,gc,xp,yp,(char *)icone);
		#else
			WndString(f,gc,xp,yp,"+");
		#endif
		}
		gc = gc_ab6;
//		if(cdr->titre[0] && (xscale->titre[0] == '\0') && (yscale->titre[0] == '\0')) {
//			xp = (cdr->dh - WndColToPix((int)strlen(cdr->titre))) / 2;
//			yp = cdr->dv;
//			WndString(f,gc,xp,yp,cdr->titre);
//		}
		if(cdr->titre[0] && (xscale->titre[0] == '\0')) {
			yp = cdr->dv;
			if(yscale->titre[0] == '\0') {
				xp = (cdr->dh - WndColToPix((int)strlen(cdr->titre))) / 2;
				WndString(f,gc,xp,yp,cdr->titre);
			} else {
				xp = WndColToPix(1);
				WndString(f,gc,xp,yp,yscale->titre);
			}
		}
		if(DEBUG_GRAPH(1)) {
			if(xscale->reel) WndPrint("(%s) => %cscale->u.r = [%g .. %g]\n",__func__,'X',xscale->u.r.min,xscale->u.r.max);
			else WndPrint("(%s) => %cscale->u.i = [%d .. %d]\n",__func__,'X',xscale->u.i.min,xscale->u.i.max);
		}
		if(/* ((graph->mode & GRF_NBAXES) != GRF_0AXES) && */ (graph->mode != GRF_DIRECT)) {
			GraphGradMin(xscale,cdr->dx);
			GraphGradMin(yscale,cdr->dy);
			// ym = (WndLigToPix(3) / 2) + graph->of7 + graph->grad; /* WndLigToPix(3) */
			ym = yscale->marge_max - (WndLigToPix(1) / 2) - 2;

			xaxe1 = yaxe1 = xaxe2 = yaxe2 = -1;
			if(((graph->mode & GRF_NBAXES) == GRF_2AXES) && (f->qualite == WND_Q_ECRAN)) {
				if(xscale->reel) xaxe1 = GraphPosFloat(1,xscale->u.r.axe,xscale);
				else xaxe1 = GraphPosInt(1,xscale->u.i.axe,xscale);
				if(yscale->reel) yaxe1 = GraphPosFloat(1,yscale->u.r.axe,yscale);
				else yaxe1 = GraphPosInt(1,yscale->u.i.axe,yscale);
				GraphAxisXDraw(graph,yaxe1,gc,yscale->typegrad);
				GraphAxisYDraw(graph,xaxe1,gc,xscale->typegrad);
				if(xscale->titre[0]) {
					xp = cdr->dh - WndColToPix((int)strlen(xscale->titre));
					yp = cdr->dv - yaxe1 + graph->grad + WndLigToPix(2);
					WndString(f,gc,xp,yp,xscale->titre);
				}
				if(yscale->titre[0]) {
					/* xp = xaxe1 - WndColToPix(strlen(yscale->titre)); */
					xp = xaxe1 - xscale->cols - graph->grad;
					yp = ym;
					WndString(f,gc,xp,yp,yscale->titre);
				}

			} else if(((graph->mode & GRF_NBAXES) == GRF_4AXES) || (f->qualite == WND_Q_PAPIER)) {
				if(xscale->reel) xaxe1 = GraphPosFloat(1,xscale->u.r.min,xscale);
				else xaxe1 = GraphPosInt(1,xscale->u.i.min,xscale);
				if(yscale->reel) yaxe1 = GraphPosFloat(1,yscale->u.r.min,yscale);
				else yaxe1 = GraphPosInt(1,yscale->u.i.min,yscale);
				GraphAxisXDraw(graph,yaxe1,gc,GRF_GRADINF);
				GraphAxisYDraw(graph,xaxe1,gc,GRF_GRADINF);
				if(xscale->titre[0]) {
					xp = cdr->dh - WndColToPix((int)strlen(xscale->titre));
					yp = cdr->dv - yaxe1 + graph->grad + WndLigToPix(2);
					WndString(f,gc,xp,yp,xscale->titre);
				}
				if(yscale->titre[0]) {
					/* xp = xaxe1 - WndColToPix(strlen(yscale->titre)); */
					xp = xaxe1 - xscale->cols - graph->grad;
					yp = ym;
					WndString(f,gc,xp,yp,yscale->titre);
				}
				if(xscale->reel) xaxe2 = GraphPosFloat(1,xscale->u.r.max,xscale);
				else xaxe2 = GraphPosInt(1,xscale->u.i.max,xscale);
				if(yscale->reel) yaxe2 = GraphPosFloat(1,yscale->u.r.max,yscale);
				else yaxe2 = GraphPosInt(1,yscale->u.i.max,yscale);
				typegrad = (f->qualite == WND_Q_ECRAN)? GRF_GRADSUP: GRF_GRADNONE;
				GraphAxisXDraw(graph,yaxe2,gc,typegrad);
				GraphAxisYDraw(graph,xaxe2,gc,typegrad);
				/*
				 if(xscale->titre[0]) {
				 xp = cdr->larg - WndColToPix(strlen(xscale->titre));
				 yp = cdr->haut - WndLigToPix(1);
				 WndString(f,gc,xp,yp,xscale->titre);
				 }
				 if(yscale->titre[0]) WndString(f,gc,0,ym,yscale->titre);
				 */
			}

//			gc = OpiumPSenCours? WND_BUTN: WND_DARK; // PS: WND_LITE trop clair, ecran: WND_SDRK trop fonce
			if(graph->mode & GRF_GRID) {
				int xlim,ylim,timeout,nbgrad;
				float rx,ry;

				gc = GRF_GRILLE;
				doit_fermer = WndBeginLine(f,gc,GL_LINES);
//				nbgrad = 1; /* pour gcc */
				if(xscale->logd) {
					rx = (float)(xscale->g1 * pow(10.0,(double)xscale->gmin));
					if(xscale->grad > 0.0) xscale->grad = rx;
					nbgrad = 9;
				} else rx = xscale->g1 + ((float)xscale->gmin * xscale->grad);
				xc = GraphPosFloat(1,rx,xscale) - cdr->dx;
				xlim = xscale->marge_min + xscale->utile - cdr->dx;
				if(xlim > cdr->dh) xlim = cdr->dh;
				timeout = 0;
				xcp = xaxe1;
				do {
					if((xc >= xscale->marge_min) && (xc != xaxe1) && (xc != xaxe2) && (abs(xc - xcp) > 0)) {
						GraphAxisYDraw(graph,xc,gc,GRF_GRADNONE);
						xcp = xc;
					}
					if(xscale->logd) {
						if(xscale->grad < 0.0) rx *= 10.0f;
						else {
							rx += xscale->grad;
							if(--nbgrad == 0) { xscale->grad *= 10.0f; nbgrad = 9; xcp = xaxe1; }
						}
					} else rx += xscale->grad;
					xc = GraphPosFloat(1,rx,xscale) - cdr->dx;
					if(++timeout > 100) break;
				} while(xc <= xlim);

				if(yscale->logd) {
					ry = (float)(yscale->g1 * pow(10.0f,(double)xscale->gmin));
					if(yscale->grad > 0.0f) yscale->grad = ry;
					nbgrad = 9;
				} else ry = yscale->g1 + (yscale->gmin * yscale->grad);
				yc = GraphPosFloat(1,ry,yscale) + cdr->dy;
				ylim = cdr->haut - yscale->marge_max + cdr->dy;
				if(ylim > cdr->haut) ylim = cdr->haut;
				timeout = 0;
				ycp = yaxe1;
				do {
					if((yc >= yscale->marge_min) && (yc != yaxe1) && (yc != yaxe2) && (abs(yc - ycp) > 0)) {
						GraphAxisXDraw(graph,yc,gc,GRF_GRADNONE);
						ycp = yc;
					}
					if(yscale->logd) {
						if(yscale->grad < 0.0f) ry *= 10.0f;
						else {
							ry += yscale->grad;
							if(--nbgrad == 0) { yscale->grad *= 10.0f; nbgrad = 9; ycp = yaxe1; }
						}
					} else ry += yscale->grad;
					yc = GraphPosFloat(1,ry,yscale) + cdr->dy;
					if(++timeout > 100) break;
				} while(yc <= ylim);
				if(doit_fermer) WndEndLine(f);
			}
		}
	}

/*
 * Alignement du trace des legendes (esthetisme extreme)
 */
	lx = 0; /* gcc */
	if((graph->mode & GRF_LEGEND) && complet) {
		int l;
		data_ptr = graph->data_ptr; nbx = 0;
		for (num = 0; num < graph->dim ; num++,data_ptr++) {
			data = *data_ptr; if(!data) continue;
			if((data->parm & GRF_AXIS) == GRF_YAXIS) {
				if(data->fctn == (void *)-1) continue;
				if((data->trace[f->qualite] & GRF_MODE) == GRF_CODE_ERR) continue;
				if((data->parm & GRF_ADRS) == GRF_INDIV) continue;
				l = (int)strlen(data->titre);
				if(nbx < l) nbx = l;
			}
		}
		lx = cdr->dh - WndColToPix(nbx + 2);  /* ne pas ecrire sur la pomme */
	}

/*
 * Trace des figures
 */
	data_ptr = graph->data_ptr;
//	axeX = x = 0; /*pour gcc */
	if((xscale->num & 0xFFFF) != GRF_DIRECT) {
		ab6 = xscale->num;
		data_ab6 = data_ptr[ab6];
		if(!data_ab6) {
			WndWrite(f,0,0,0,"Abcisse definie sur un tableau absent");
			if(doit_terminer) WndRefreshEnd(f);
			f->qualite = qualite_fen;
			return(0);
		}
		gc_ab6 = data_ab6->gc[f->qualite];
		nb_ab6 = data_ab6->max - data_ab6->min;
		if(((data_ab6->min != 0) || (data_ab6->max != 0)) && (nb_ab6 <= 0)) nb_ab6 += data_ab6->dim;
	}
	/* Position de l'axe X: pour les histos */
	if(yscale->reel) axeX = cdr->haut - GraphPosFloat(1,yscale->u.r.axe,yscale) - cdr->dy;
	else axeX = cdr->haut - GraphPosInt(1,yscale->u.i.axe,yscale) - cdr->dy;

	data_ord = 0;
	if((yscale->num & 0xFFFF) == GRF_DIRECT) nb_ord = yscale->u.i.max - yscale->u.i.min;
	else nb_ord = 0;
	xerr_demande = yerr_demande = xerr = yerr = 0;
	indiv_demande = indiv = 0;

	p = 0; nby = 0;
	for (num = 0; num < graph->dim ; num++,data_ptr++) {
//-		OpiumDebugLevel[OPIUM_DEBUG_GRAPH] = (num >= 3)? 1: 0;
		if(DEBUG_GRAPH(1)) {
			WndPrint("(%s) Tableau #%d/%d @%08lX",__func__,num,graph->dim,(Ulong)*data_ptr);
			if(*data_ptr) WndPrint("->%08lX\n",(Ulong)((*data_ptr)->fctn)); else WndPrint(" inutilisable\n");
		}
		data = *data_ptr; if(!data) continue;
		if(data->fctn == (void *)-1) continue; // 0 permis a ce niveau
		//- printf("(%s.1[%d]) @%08lX.%08lX.line_width = %d\n",__func__,num,(Ulong)data,(Ulong)(data->gc),(data->gc)->line_width);
		if((data->trace[f->qualite] & GRF_MODE) == GRF_CODE_ERR) {
			if((data->parm & GRF_AXIS) == GRF_XAXIS) xerr_demande = data;
			else if((data->parm & GRF_AXIS) == GRF_YAXIS) yerr_demande = data;
			continue;
		}
		if((data->parm & GRF_ADRS) == GRF_INDIV) {
			indiv_demande = (TypeGraphIndiv *)data->fctn;
			if(DEBUG_GRAPH(1)) printf("(%s) Codes individuels @%08llX\n",__func__,(IntAdrs)indiv_demande);
			continue;
		}
		nb = data->max - data->min;
		if(((data->min != 0) || (data->max != 0)) && (nb <= 0)) nb += data->dim;
		if(DEBUG_GRAPH(1)) WndPrint("(%s) Donnees #%d: %d valeurs a partir de %d en %c\n",__func__,
			num,nb,data->min,
			(((data->parm & GRF_AXIS) == GRF_IMAGE)? '*':
			(((data->parm & GRF_AXIS) == GRF_XAXIS)? 'X':
			 ((data->parm & GRF_AXIS) == GRF_YAXIS)? 'Y': 'Z')));
		if((data->parm & GRF_AXIS) == GRF_XAXIS) {
			data_ab6 = data;
			gc_ab6 = data_ab6->gc[f->qualite];
			if((xscale->num & 0xFFFF) == GRF_DIRECT)
				nb = nb_ab6 = xscale->u.i.max - xscale->u.i.min;
			else nb_ab6 = nb;
			xerr = xerr_demande; xerr_demande = 0;
			continue;
		} else if((data->parm & GRF_AXIS) == GRF_YAXIS) {
			data_ord = data;
			if((yscale->num & 0xFFFF) == GRF_DIRECT) nb = nb_ord = yscale->u.i.max - yscale->u.i.min;
			else {
				nb_ord = nb; /* utilisable pour les ZAXIS ou les IMAGE */
				if(nb > nb_ab6) nb = nb_ab6; /* utilisable pour Y=f(X) */
			}
			nby++;
			yerr = yerr_demande; yerr_demande = 0;
			indiv = indiv_demande; indiv_demande = 0;
		}
		if((nb <= 0) || ((data->parm & GRF_TRAN) == GRF_NODSP)) continue;
		if(!complet) {
			if(nb < ajoutes) continue;
			nb = ajoutes;
		}
		type = data->parm & GRF_TYPE;
		court = (type == GRF_SHORT);
		reel = (type == GRF_FLOAT);

		/* ----- Partie speciale images (ou 3D) ----- */
		if(((data->parm & GRF_AXIS) == GRF_ZAXIS) || ((data->parm & GRF_AXIS) == GRF_IMAGE)) {
			float *rtab; short *stab; int *itab; unsigned char *ctab; WndIcone icone;
			float rmin,rmax,rof7,rcor,rtop,rval;
			int   imin,imax,iof7,     itop,ival;
			int xmin,ymin,xmax,ymax;
			TypeFctn fc;
			int xn,yn;

			if(DEBUG_GRAPH(1)) WndPrint("(%s) Surface avec LUT<%d>[%d]\n",__func__,data->couleur.type,data->couleur.def.lut.dim);
			if(!data_ord) continue;
			/* pour gcc */
			rtab = (float *)0; stab = (short *)0; itab = (int *)0; ctab = (unsigned char *)0; fc = 0; fctn = 0;
			imin = imax = iof7 = itop = 0; rmin = rmax = rof7 = rtop = rcor = 0.0f;

			if(data->couleur.type == GRF_ICN) {
				icone = (WndIcone)data->fctn;
				WndIconeInit(f,icone);
			} else {
				if(data->couleur.def.lut.dim == 0) continue;
				icone = 0; // gcc
				if(data->couleur.type == GRF_BMP) ctab = (unsigned char *)data->fctn;
				else {
					fctn = (data->parm & GRF_ADRS) == GRF_FCTN;
					if(fctn) fc = (TypeFctn)data->fctn;
					else if(reel) rtab = (float *)data->fctn;
					else if(court) stab = (short *)data->fctn;
					else itab = (int *)data->fctn;
					if(DEBUG_GRAPH(2)) WndPrint("Donnees de type '%s de %s'\n",fctn?"Fonction":"Tableau",
								 reel? "reels":(court? "entiers 16bits": "entiers 32bits"));
					if(reel) {
						rmin = data->gamma.r.xmin; rmax = data->gamma.r.xmax;
						if(rmin == rmax) continue;
						rof7 = data->gamma.r.vmin; rtop = data->gamma.r.vmax;
						rcor = (rtop - rof7) / (rmax - rmin);
					} else {
						imin = data->gamma.i.xmin; imax = data->gamma.i.xmax;
						if(imin == imax) continue;
						iof7 = data->gamma.i.vmin; itop = data->gamma.i.vmax;
						rcor = (float)(itop - iof7) / (float)(imax - imin);
					}
				}
				/* OpiumDebugLevel[OPIUM_DEBUG_GRAPH] = 2; */
				if(f->qualite == WND_Q_ECRAN) WndColorSetByRGB(data->couleur.def.lut.val,0,0,0);
				else WndColorSetByRGB(data->couleur.def.lut.val,WND_COLOR_MAX,WND_COLOR_MAX,WND_COLOR_MAX);
				xmin = (GraphPosData(1,data_ab6,1,xscale) + GraphPosData(1,data_ab6,0,xscale)) / 2;
				// ymin = yscale->marge_min;
				ymin = (GraphPosData(1,data_ord,1,yscale) + GraphPosData(1,data_ord,0,yscale)) / 2;
				WndImageOffset(f,xmin,ymin); // avant WndImageInit
				xmax = (GraphPosData(1,data_ab6,nb_ab6-1,xscale) + GraphPosData(1,data_ab6,nb_ab6-2,xscale)) / 2;
				ymax = (GraphPosData(1,data_ord,nb_ord-1,yscale) + GraphPosData(1,data_ord,nb_ord-2,yscale)) / 2;
				WndImageInit(f,xmax,ymax,data->couleur.def.lut.val,data->couleur.def.lut.dim);
				if(DEBUG_GRAPH(1)) WndPrint("(%s) Image brute: %d x %d [kmax %d, dim LUT=%d] dans un cadre %d x %d + %d x %d\n",__func__,
					nb_ab6,nb_ord,nb,data->couleur.def.lut.dim,xmin,xmax,xmax,ymax);
			}
			if(DEBUG_GRAPH(2)) {
				WndPrint("(%s) Cadre %3d x %3d\n",__func__,cdr->larg,cdr->haut);
				WndPrint("(%s) Decal %3d x %3d\n",__func__,cdr->dx,cdr->dy);
				WndPrint("(%s) Marge %3d x %3d\n",__func__,xscale->marge_min,yscale->marge_min);
				WndPrint("(%s) Debut %3d x %3d\n",__func__,GraphPosData(1,data_ab6,0,xscale),GraphPosData(1,data_ord,0,yscale));
				WndPrint("(%s) 2eme  %3d x %3d\n",__func__,GraphPosData(1,data_ab6,1,xscale),GraphPosData(1,data_ord,1,yscale));
				WndPrint("(%s) N-2   %3d x %3d\n",__func__,GraphPosData(1,data_ab6,nb_ab6 - 2,xscale),GraphPosData(1,data_ord,nb_ord - 2,yscale));
				WndPrint("(%s) N-1   %3d x %3d\n",__func__,GraphPosData(1,data_ab6,nb_ab6 - 1,xscale),GraphPosData(1,data_ord,nb_ord - 1,yscale));
				WndPrint("(%s) ----- %3s   %3s\n",__func__,"---","---");
				xmax = cdr->larg - cdr->dx - xscale->marge_min;
				ymax = cdr->haut - cdr->dy - yscale->marge_min;
				WndPrint("(%s) Calcl %3d x %3d\n",__func__,xmax,ymax);
				xmax = ((GraphPosData(1,data_ab6,nb_ab6-1,xscale) + GraphPosData(1,data_ab6,nb_ab6-2,xscale)) / 2)
					 - ((GraphPosData(1,data_ab6,1,xscale) + GraphPosData(1,data_ab6,0,xscale)) / 2);
//				ymax = ((GraphPosData(1,data_ord,nb_ord-1,yscale) + GraphPosData(1,data_ord,nb_ord-2,yscale)) / 2)
//					 - ((GraphPosData(1,data_ord,1,yscale) + GraphPosData(1,data_ord,0,yscale)) / 2);
				ymax = GraphPosData(1,data_ord,nb_ord-1,yscale) - yscale->marge_min;
				WndPrint("(%s) Posit %3d x %3d\n",__func__,xmax,ymax);
			}
			k = 0;
			yn = yp = cdr->haut - GraphPosData(1,data_ord,0,yscale) + cdr->dy;
			xmin = cdr->larg; ymin = cdr->haut; xmax = ymax = 0;
			for(j=0; j<nb_ord; j++) {
				if(j == (nb_ord - 1)) y = yscale->marge_min;
				else y = cdr->haut - GraphPosData(1,data_ord,j+1,yscale) + cdr->dy;
				ym = (int)(0.5 * (y + yp));
				xn = xp = GraphPosData(1,data_ab6,0,xscale) - cdr->dx;
				for(i=0; i<nb_ab6; i++) {
					char montre;
					if(i == (nb_ab6 - 1)) x = cdr->larg - xscale->marge_min;
					else x = GraphPosData(1,data_ab6,i+1,xscale) - cdr->dx;
					xm = (int)(0.5 * (x + xp));
					if(DEBUG_GRAPH(2)) WndPrint("Point (%3d, %3d) [#%3d] affiche en (%3d, %3d), mode #%d\n",i,j,k,x,y,data->couleur.type);
					if(data->couleur.type == GRF_ICN) {
						for(yc=ym; yc<yn; yc++) {
							for(xc=xn; xc<xm; ) { xc++; WndIconePixel(f,xc,yc,&(icone->pixel[k])); }
						}
						k++;
					} else {
						montre = 1;
						if(data->couleur.type == GRF_BMP) {
							if((montre = ctab[k])) ival = k;
						} else {
							if(fctn) {
								void (*fctnTmp)(int, float*, int*) =  (void (*)(int, float*, int*))fc;
								(*fctnTmp)(k,&rval,&ival);
							}
							else if(reel) rval = rtab[k];
							else if(court) ival = stab[k];
							else ival = itab[k];
							if(reel) {
								if(rval <= rmin) rval = rof7;
								else if(rval >= rmax) rval = rtop;
								else rval = ((rval - rmin) * rcor) + rof7;
								ival = (int)rval;
							} else {
								if(ival <= imin) ival = iof7;
								else if(ival >= imax) ival = itop;
								else ival = (int)((float)(ival - imin) * rcor) + iof7;
							}
						}
						/* ym = j + 1; yn = j + 2; xn = i; xm = i + 1; */
						if(DEBUG_GRAPH(2)) WndPrint("%s de %d dans (%d-%d,%d-%d)\n",montre?"Remplissage":"Masquage",ival,xn+1,xm,ym,yn-1);
						if(xmin > xm) xmin = xm; if(ymin > ym) ymin = ym;
						if(xmax < xn) xmax = xn; if(ymax < yn) ymax = yn;
						if(montre) for(yc=ym; yc<yn; yc++) {
							for(xc=xn; xc<xm; ) { xc++; WndImagePixel(f,xc,yc,ival); }
						}
						k++;
						if(data->couleur.type != GRF_BMP) { if(k > nb) { i = nb_ab6; j = nb_ord; } }
					}
					xp = x; xn = xm;
				}
				yp = y; yn = ym;
			}
			if(DEBUG_GRAPH(1)) WndPrint("(%s) Image definie: %d x %d  .. %d x %d\n",__func__,xmin,ymin,xmax,ymax);
			WndImageShow(f);
			continue;
		}

		/* ----- Courbes y=f(x) ----- */
		gc = data->gc[f->qualite];
		//- printf("(%s.2) @%08lX.%08lX.line_width = %d\n",__func__,(Ulong)data,(Ulong)gc,gc->line_width);
		if((graph->mode & GRF_LEGEND) && data->titre[0] && complet) { /* Trace des legendes */
			int dx;
//			si nbx pas calcule: xp = cdr->dh - WndColToPix(strlen(data->titre));
			yp = WndLigToPix(nby);
			if(lx > WndColToPix(2)) {
				xp = lx - WndColToPix(3);
				if(xp < 0) xp = 0;
				dx = (lx - xp) - WndColToPix(1);
				WndLine(f,gc,xp,yp-(WndLigToPix(1)/2),dx,0);
				WndString(f,gc_ab6,lx,yp,data->titre);
			} else WndString(f,gc,0,yp,data->titre);
		}
		mode = data->trace[f->qualite] & GRF_MODE;
		if((mode == GRF_CODE_ERR) || (mode == GRF_CODE_PNT)) niveau_detail = GRF_DETAIL_TOUT;
		else niveau_detail = (data->style & GRAPH_DETAIL) >> GRF_SHIFT_DETAIL;
		histo_libre = (data->style & GRAPH_HISTO_FLOTTANT);
		trace_generale = data->trace[f->qualite] & GRF_LINE;
		nbpts = 0;
		switch(mode) {
			case GRF_CODE_HST: nbpts = (2 * nb); if(!histo_libre) nbpts += 2; break;
			case GRF_CODE_LIN: nbpts = nb; break;
		}
		if(nbpts) { if((p = WndLigneAlloc(2*nbpts)) == 0) continue; }
//		if(DEBUG_GRAPH(2))
//			WndPrint("%d points stockables @%08lX\n",WndLigneDim,(Ulong)p);
		if(DEBUG_GRAPH(2)) printf("(%s) trace generale = %02X (mode %02X, ligne ou point #%02X)\n",__func__,
			data->trace[f->qualite],data->trace[f->qualite] & GRF_MODE,data->trace[f->qualite] & GRF_LINE);
		if(complet && ((data_ab6->parm & GRF_ADRS) == GRF_INDEX) && (cdr->dx > 0)) {
			int *itab; short *stab; float *rtab;
			int64 ival; float rval;

			rtab = (float *)0;          /* pour gcc */
			stab = (short *)0;          /* pour gcc */
			itab = (int *)0;            /* pour gcc */
			if((data_ab6->parm & GRF_TYPE) == GRF_FLOAT) {
				rval = (float)cdr->dx * xscale->ech; rval /= (float)xscale->utile; rval += xscale->u.r.min;
				rtab = (float *)data_ab6->fctn;
				if(rtab == 0) jxmin = (int)rval; else jxmin = (int)((rval - *rtab) / *(rtab + 1));
//				if(DEBUG_GRAPH(2)) WndPrint("(OpiumRefreshGraph) min_scale = %g, jxmin = (%g - %g) / %g = %d\n",
//					xscale->u.r.min,rval,*rtab,*(rtab+1),jxmin);
			} else if((data_ab6->parm & GRF_TYPE) == GRF_SHORT) {
				ival = (int64)cdr->dx * (int64)xscale->ech; ival /= xscale->utile; ival += xscale->u.i.min;
				stab = (short *)data_ab6->fctn;
				if(stab == 0) jxmin = (int)ival; else jxmin = (int)((ival - *stab) / *(stab + 1));
			} else {
				ival = (int64)cdr->dx * (int64)xscale->ech; ival /= xscale->utile; ival += xscale->u.i.min;
				itab = (int *)data_ab6->fctn;
				if(itab == 0) jxmin = (int)ival; else jxmin = (int)((ival - *itab) / *(itab + 1));
//				if(DEBUG_GRAPH(2)) WndPrint("decalage=%d/%d, echelle=%g/%d, min_scale=%d, jxmin=(%d-%d)/%d=%d\n",
//					cdr->dx,cdr->larg,xscale->ech,xscale->utile,xscale->u.i.min,
//					(int)ival,*itab,*(itab+1),jxmin);
			}
			j = jxmin - data_ab6->min; while(j < 0) j += data_ab6->dim;
			jx = jxmin; jy = data->min + j;
			/* les 2 corrections suivantes rendent aleatoire le positionnement, mais au moins jy restera dans les clous! */
			while(jy < 0) jy += data->dim;
			while(jy > data->dim) jy -= data->dim;
		} else {
			jx = data_ab6->min; jy = data->min;
			if(ajoutes) {
				jx -= ajoutes; jy -= ajoutes;
				if(jx < 0) jx -= data_ab6->dim;
				if(jy < 0) jy -= data->dim;
			}
		}
		doit_fermer = WndBeginLine(f,gc,GL_LINES);
		xp = GraphPosData(1,data_ab6,jx,xscale) - cdr->dx; yp = axeX; /* data->min a la place de jx ??? */
		if(DEBUG_GRAPH(2)) {
			printf("(%s) %d valeur%s, commencant a la valeur #%d\n",__func__,Accord1s(nb),jy);
			if(indiv) { int k,num,larg;
				int pt;
				larg = 20;
				printf("(%s) %s[%d] en partant de %d:",__func__,"indiv",nb,jy);
				printf("\n|     "); for(k=0; k<larg; k++) printf(" %2d",k);
				printf("\n|     "); for(k=0; k<larg; k++) printf("...");
				for(num=0, pt=jy; num<nb; num++,pt++) {
					if(!(num % larg)) printf("\n| %03d:",num);
					printf(" %02x",indiv[pt].trace);
				}
				printf("\n");
			}
		}
		premier = 1; pas_commence = 1; ii = 0;
		for(i=0; i<nb; i++, jx++,jy++) {
			if(jx >= data_ab6->dim) jx -= data_ab6->dim;
			if(jy >= data->dim) jy -= data->dim;
			if(i) { if(jy == data->min) break; }
			if(indiv) {
				indiv_point = indiv + jy;
				trace_point = indiv_point->trace;
				if(DEBUG_GRAPH(2)) printf("(%s) trace[%d] = %02x (%s)\n",__func__,jy,trace_point,GRF_HIDDEN(trace_point)?"cache":"montre");
				if(GRF_HIDDEN(trace_point)) continue;
				trace = trace_point & GRF_LINE;
			} else trace = trace_generale;
			x = GraphPosData(1,data_ab6,jx,xscale);
			x -= cdr->dx;
			if((x >= cdr->dh) && ((data_ab6->parm & GRF_ADRS) == GRF_INDEX)) break;
			if(niveau_detail == GRF_DETAIL_TOUT) y = cdr->haut - GraphPosData(1,data,jy,yscale);
			else {
				GraphUserVal(data,jy,type,&usr_i,&usr_r);
				if(premier) {
					xcur = x;
					if(niveau_detail == GRF_DETAIL_LISSE) {
						if(reel) ytot_r = usr_r; else ytot_i = usr_i; yvus = 1;
					} else if(niveau_detail == GRF_DETAIL_MINMAX) {
						if(reel) ymin_r = ymax_r = usr_r; else ymin_i = ymax_i = usr_i;
					}
					premier = 0;
					continue;
				} else {
					if(x == xcur) {
						if(niveau_detail == GRF_DETAIL_LISSE) {
							if(reel) ytot_r += usr_r; else ytot_i += usr_i;
							yvus++;
						} else if(niveau_detail == GRF_DETAIL_MINMAX) {
							if(reel) {
								if(usr_r < ymin_r) ymin_r = usr_r;
								else if(usr_r > ymax_r) ymax_r = usr_r;
							} else {
								if(usr_i < ymin_i) ymin_i = usr_i;
								else if(usr_i > ymax_i) ymax_i = usr_i;
							}
						}
						continue;
					}
					/* on change d'abcisse: cloture du precedent calcul et memorisation */
					xnext = x;
					x = xcur;
					if(niveau_detail == GRF_DETAIL_LISSE) {
						if(reel) {
							if(yvus > 1) ytot_r /= (float)yvus;
							y = cdr->haut - GraphPosFloat(1,ytot_r + data->of7.r.v,yscale);
						} else {
							if(yvus > 1) ytot_i /= yvus;
							y = cdr->haut - GraphPosInt(1,ytot_i + data->of7.i.v,yscale);
						}
					} else if(niveau_detail == GRF_DETAIL_MINMAX) {
						if(reel) y = cdr->haut - GraphPosFloat(1,ymin_r + data->of7.r.v,yscale);
						else y = cdr->haut - GraphPosInt(1,ymin_i + data->of7.i.v,yscale);
						y -= cdr->dy;
						if(y < 0) y = 0; else if(y >= cdr->dv) y = cdr->dv - 1;
						if((pas_commence || (x != xp) || (y != yp)) && (x >= 0) && (x < cdr->dh)) {
							switch (mode) {
							  case GRF_CODE_HST:
								xm = (xp + x) / 2;
								if((ii > 0) || !histo_libre) { p[ii].x = xm; p[ii].y = yp; ii++; }
								p[ii].x = xm; p[ii].y = y ; ii++;
								break;
							  case GRF_CODE_LIN:
								/* si polyedre, stocker les types de trait pour decoupage au tracage */
								p[ii].x = x; p[ii].y = y; ii++;
								GraphTraceBarresErreur(f,gc,x,xerr,jx,xscale,y,yerr,jy,yscale);
								break;
							}
							xp = x; yp = y; pas_commence = 0;
						}
						if(reel) y = cdr->haut - GraphPosFloat(1,ymax_r + data->of7.r.v,yscale);
						else y = cdr->haut - GraphPosInt(1,ymax_i + data->of7.i.v,yscale);
					}
				}
			}
			y -= cdr->dy;
			if(y < 0) y = 0; else if(y >= cdr->dv) y = cdr->dv - 1; /* d'ou test '&& (y >= 0) && (y < cdr->dv)' inutile */
			if((pas_commence || (x != xp) || (y != yp)) && (x >= 0) && (x < cdr->dh)) {
				WndContextPtr gc_point;
				switch (mode) {
				  case GRF_CODE_HST:
					xm = (xp + x) / 2;
					if((ii > 0) || !histo_libre) { p[ii].x = xm; p[ii].y = yp; ii++; }
					p[ii].x = xm; p[ii].y = y ; ii++;
					break;
				  case GRF_CODE_LIN:
					/* si polyedre, stocker les types de trait pour decoupage au tracage */
					p[ii].x = x; p[ii].y = y; ii++;
					GraphTraceBarresErreur(f,gc,x,xerr,jx,xscale,y,yerr,jy,yscale);
					break;
				  case GRF_CODE_PNT:
					if(i == (nb - 1)) gc_point = f->gc[f->qualite].coul[graph->ajout];
					else if(i == (nb - 2)) gc_point = f->gc[f->qualite].coul[WND_GC_HILITE];
					/* faire ajouter, via OpiumGraphColorNb et OpiumGraphColorSet, les couleurs des etiquettes
					else if((data->couleur.type == GRF_RGB) &&
						  ((indiv_point->r != WND_COLOR8(data->couleur.def.rgb[qualite_graph].r))
						|| (indiv_point->g != WND_COLOR8(data->couleur.def.rgb[qualite_graph].g))
						|| (indiv_point->b != WND_COLOR8(data->couleur.def.rgb[qualite_graph].b)))) {
						WndContextPtr gc_pnt;
						gc_pnt = WndContextCreate(f);
						WndContextFgndRGB(cdr->f,gc_pnt,indiv_point->r,indiv_point->g,indiv_point->b);
						gc_point = gc_pnt;
					} */ else gc_point = gc;
					GraphPlacePoint(f,gc_point,trace,x,y);
					GraphTraceBarresErreur(f,gc_point,x,xerr,jx,xscale,y,yerr,jy,yscale);
					break;
				}
				xp = x; yp = y; pas_commence = 0;
			}
			if(niveau_detail == GRF_DETAIL_LISSE) {
				xcur = xnext; if(reel) ytot_r = usr_r; else ytot_i = usr_i; yvus = 1; /* memorise le pt courant */
			} else if(niveau_detail == GRF_DETAIL_MINMAX) {
				xcur = xnext; if(reel) ymin_r = ymax_r = usr_r; else ymin_i = ymax_i = usr_i;
			}
		}

		if(niveau_detail != GRF_DETAIL_TOUT) {
			x = xcur;
			if(niveau_detail == GRF_DETAIL_LISSE) {
				if(reel) {
					if(yvus > 1) ytot_r /= (float)yvus;
					y = cdr->haut - GraphPosFloat(1,ytot_r + data->of7.r.v,yscale);
				} else {
					if(yvus > 1) ytot_i /= yvus;
					y = cdr->haut - GraphPosInt(1,ytot_i + data->of7.i.v,yscale);
				}
			} else if(niveau_detail == GRF_DETAIL_MINMAX) {
				if(reel) y = cdr->haut - GraphPosFloat(1,ymin_r + data->of7.r.v,yscale);
				else y = cdr->haut - GraphPosInt(1,ymin_i + data->of7.i.v,yscale);
				y -= cdr->dy;
				if(y < 0) y = 0; else if(y >= cdr->dv) y = cdr->dv - 1;
				if((pas_commence || (x != xp) || (y != yp)) && (x >= 0) && (x < cdr->dh)) {
					switch (mode) {
					  case GRF_CODE_HST:
						xm = (xp + x) / 2;
						if((ii > 0) || !histo_libre) { p[ii].x = xm; p[ii].y = yp; ii++; }
						p[ii].x = xm; p[ii].y = y ; ii++;
						break;
					  case GRF_CODE_LIN:
						/* si polyedre, stocker les types de trait pour decoupage au tracage */
						p[ii].x = x; p[ii].y = y; ii++;
						GraphTraceBarresErreur(f,gc,x,xerr,jx,xscale,y,yerr,jy,yscale);
						break;
					}
					xp = x; yp = y; pas_commence = 0;
				}
				if(reel) y = cdr->haut - GraphPosFloat(1,ymax_r + data->of7.r.v,yscale);
				else y = cdr->haut - GraphPosInt(1,ymax_i + data->of7.i.v,yscale);
			}
			y -= cdr->dy;
			if(y < 0) y = 0; else if(y >= cdr->dv) y = cdr->dv - 1; /* d'ou test '&& (y >= 0) && (y < cdr->dv)' inutile */
			if((pas_commence || (x != xp) || (y != yp)) && (x >= 0) && (x < cdr->dh)) {
				switch (mode) {
				  case GRF_CODE_HST:
					xm = (xp + x) / 2;
					if((ii > 0) || !histo_libre) { p[ii].x = xm; p[ii].y = yp; ii++; }
					p[ii].x = xm; p[ii].y = y ; ii++;
					break;
				  case GRF_CODE_LIN:
					/* si polyedre, stocker les types de trait pour decoupage au tracage */
					p[ii].x = x; p[ii].y = y; ii++;
					GraphTraceBarresErreur(f,gc,x,xerr,jx,xscale,y,yerr,jy,yscale);
					break;
				  case GRF_CODE_PNT:
					GraphPlacePoint(f,gc,trace,x,y);
					GraphTraceBarresErreur(f,gc,x,xerr,jx,xscale,y,yerr,jy,yscale);
					break;
				}
				xp = x; yp = y; pas_commence = 0;
			}
		}

		if(doit_fermer) WndEndLine(f);
		if(mode == GRF_CODE_HST) {
			p[ii].x = x; p[ii].y = yp ;   ii++;
			if(!histo_libre) { p[ii].x = x; p[ii].y = axeX ; ii++; }
		}
		if(ii > WndLigneDim) {
			WndPrint("(%s) ERREUR SYSTEME, PLACE MEMOIRE MAL CALCULEE\n",__func__);
			WndPrint(" reservee: %d points, utilisee: %d points\n",WndLigneDim,ii);
		}
		if(DEBUG_GRAPH(1)) WndPrint("(%s) %d point%s a tracer @%08lX en mode %d (%s)\n",__func__,
			Accord1s(ii),(Ulong)p,mode,(mode < GRF_NBMODES)? GraphModeName[mode]:"inutilisable");
		//- printf("(%s.2[%d]) @%08lX.%08lX.line_width = %d\n",__func__,num,(Ulong)data,(Ulong)(data->gc),(data->gc)->line_width);
		switch(mode) {
			/* si polyedre, autant d'appels que de changements de type de trait */
		  case GRF_CODE_LIN: WndPoly(f,gc,p,ii,WND_ABS); break;
		  case GRF_CODE_HST: WndPoly(f,gc,p,ii,WND_ABS); break;
		}
	}
	if((ajoute = graph->ajouts)) ajoute(graph,0);
	if(doit_terminer) WndRefreshEnd(f);

	f->qualite = qualite_fen;
	return(1);
}
/* ========================================================================== */
void GraphPrint(Graph graph, int x, int y, char *texte) {
	WndFrame f; GraphScale *xscale;

	OpiumCoordFenetre(graph->cdr,&f); /* cdr peut etre a 0, et f peut aussi etre a 0 */
	if(!f) return;
	if(x < 0) {
		xscale = graph->axe;
		if(xscale->reel) x = GraphPosFloat(1,xscale->u.r.axe,xscale);
		else x = GraphPosInt(1,xscale->u.i.axe,xscale);
	}
	WndString(f,0,x,y,texte);

}
/* ========================================================================== */
static inline void GraphClicCopie(int *dest, int *srce) {
	dest[0] = srce[0]; dest[1] = srce[1];
}
/* ========================================================================== */
static inline void GraphClicAjoute(int *dest, int *srce) {
	dest[0] += srce[0]; dest[1] += srce[1];
}
/* ========================================================================== */
static inline void GraphClicCoord(int *dest, int *srce, Cadre cdr) {
	dest[0] = srce[0]; dest[1] = cdr->haut - srce[1];
}
/* ========================================================================== */
static inline char GraphClicEgale(int *c1, int *c2) {
	return((c1[0] == c2[0]) && (c1[1] == c2[1]));
}
#define RALENTI_AFFICHAGE
/* ========================================================================== */
int OpiumRunGraph(Cadre cdr, WndAction *e) {
	Graph graph; WndFrame f; TypeGraphFctn supplement;
#ifdef RALENTI_AFFICHAGE
	GraphData ab6,ord; int j,k;
#endif /* RALENTI_AFFICHAGE */
	int clic[2],coord[2],of7[2],absol[2],z0[2],z1[2],affichage[2];
	char trois_d,doit_terminer,dbg; float z;
	int i,l; int lngr,px,qual,epais;
	int code_rendu;
	WndContextPtr gc; // GraphScale *xscale,*yscale;
	// int x,y;
	// int ix0,iy0; float rx0,ry0;

	if(DEBUG_OPIUM(1) && (cdr->type != OPIUM_GRAPH)) {
		WndPrint("+++ OpiumRunGraph sur le cadre '%s' @%08lX\n",cdr->nom,(Ulong)cdr->adrs);
		return(0);
	}
	if(!OpiumCoordFenetre(cdr,&f)) return(0);
	dbg = 0;
	graph = (Graph)cdr->adrs;
	if((supplement = graph->onclic)) {
		code_rendu = supplement(graph,e);
		if(code_rendu == -1) return(PNL_OK);
		else if(code_rendu == 1) return(0);
	} else code_rendu = 0;
	qual = f->qualite;
	epais = (graph->data_ptr[1]->trace[qual] & GRF_WIDZ) >> GRF_SHIFT_WIDZ;
	j = 0; z = 0.0f;
	// gc = WndContextCreate(f);
	gc = GRF_AJOUT;
	WndContextFgndColor(f,gc,OpiumRouge);
	clic[0] = e->x; clic[1] = e->y;
	of7[0] = cdr->dx; of7[1] = cdr->dy;
	if(e->type == WND_KEY) {
		if(dbg) WndPrint("(%s) KeyBoard en (%d, %d) + (%d, %d)\n",__func__,
			e->x,e->y,cdr->dx,cdr->dy);
		l = (int)strlen(e->texte);
		if(DEBUG_GRAPH(2)) {
			WndPrint("%04lX ( ",e->code);
			for(i=0; i<l; i++) WndPrint("%02X ",e->texte[i]); WndPrint(")\n");
		}
		if(l <= 0) {
			/* voir terms pour utilisation touches de curseur */
			if(e->code == XK_KP_F4) code_rendu = PNL_CANCEL;
		} else {
			/* et autres touches de texte */
			if(l == 1) switch(e->texte[0]) {
			  case '?':
				WndPaint(f,0,0,WndColToPix(60),WndLigToPix(19),WndColorSub[GraphQualite]);
				i = 0;
				WndWrite(f,0,i++,1,"m : appel du menu de configuration");
				WndWrite(f,0,i++,1,"g : modifier l'aspect general");
				if(f->qualite == WND_Q_PAPIER)
					WndWrite(f,0,i++,1,"p : graphique en qualite ecran");
				else WndWrite(f,0,i++,1,"p : graphique en qualite papier");
				WndWrite(f,0,i++,1,"j : sauver le graphique en JPEG");
				WndWrite(f,0,i++,1,"_ : courbe sous forme de traits");
				WndWrite(f,0,i++,1,"h : courbe sous forme d'histogramme");
				WndWrite(f,0,i++,1,". : courbe sous forme de points");
				WndWrite(f,0,i++,1,"x : modifier l'axe des X");
				WndWrite(f,0,i++,1,"y : modifier l'axe des Y");
				WndWrite(f,0,i++,1,"+ : autoriser le zoom avant");
				WndWrite(f,0,i++,1,"- : zoomer en arriere");
				WndWrite(f,0,i++,1,"* : ajuster les limites X et Y automatiquement");
				// WndWrite(f,0,i++,1,"r : retabli l'aspect initial (annule les modifs)");
				WndWrite(f,0,i++,1,"t : sauvegarder les tables");
				WndWrite(f,0,i++,1,"P : creer un postscript avec cette figure");
				WndWrite(f,0,i++,1,"1 : ouvrir un postscript avec cette figure en premier");
				WndWrite(f,0,i++,1,"a : ajouter cette figure au postscript en cours");
				WndWrite(f,0,i++,1,"s : ajouter un saut de page au postscript en cours");
				WndWrite(f,0,i++,1,"f : fermer le postscript en cours");
				WndWrite(f,0,i++,1,"q : quitter ce menu et reafficher le graphique");
				break;
			  case 'm': GraphDialogExec(cdr); break;
			  case 'g': GraphDialogInit(cdr); GraphDialogGeneral(); OpiumRefreshAgain(cdr); break;
			  case 'p': f->qualite = 1 - f->qualite; graph->pret = 0; OpiumRefreshAgain(cdr); break;
			  case 'j': OpiumPhotoSauve(); break;
			  case '_': if(graph->dim > 1) GraphDataSupportTrace(graph,1,qual,GRF_LIN,0,epais); OpiumRefreshAgain(cdr); break;
			  case 'h': if(graph->dim > 1) GraphDataSupportTrace(graph,1,qual,GRF_HST,0,epais); OpiumRefreshAgain(cdr); break;
			  case '.': if(graph->dim > 1) GraphDataSupportTrace(graph,1,qual,GRF_PNT,0,epais); OpiumRefreshAgain(cdr); break;
			  case 'x': GraphDialogInit(cdr); GraphDialogAxe(GraphMdial,GraphMdial->items+2);   OpiumRefreshAgain(cdr); break;
			  case 'y': GraphDialogInit(cdr); GraphDialogAxe(GraphMdial,GraphMdial->items+3);   OpiumRefreshAgain(cdr); break;
			  case '+': case 'z': OpiumZoomAutorise = 1; break;
			  case '-': GraphDialogInit(cdr); GraphDialogZoomMoins(); OpiumRefreshAgain(cdr); break;
			  case '*': GraphDialogInit(cdr); GraphDialogZoomJuste(); OpiumRefreshAgain(cdr); break;
			  // case 'r': GraphDialogEtatRetabli(); OpiumRefreshAgain(cdr); break;
			  case 't': GraphDialogInit(cdr); GraphDialogTablesEcrit(); break;
			  case 'P': GraphDialogInit(cdr); OpiumPSComplet(); OpiumRefreshAgain(cdr); break;
			  case '1': GraphDialogInit(cdr); OpiumPSDemarre(); OpiumRefreshAgain(cdr); break;
			  case 'a': GraphDialogInit(cdr); OpiumPSAjoute();  OpiumRefreshAgain(cdr); break;
			  case 's': GraphDialogInit(cdr); OpiumPSPage(); break;
			  case 'f': GraphDialogInit(cdr); OpiumPSTermine(); break;
			  case 'q': OpiumRefreshAgain(cdr); break;
			  case 0x08: case 0x7F: /* Ctrl-H (BackSpace) ou DEL */
				code_rendu = PNL_CANCEL;
				break;
			}
		}
	} else if(e->type == WND_PRESS) switch(e->code) {
	  case WND_MSERIGHT:
		  OpiumZoomAutorise = 1;
	  case WND_MSELEFT:
//-		if((e->x != graph->mouse[0]) || (e->y != graph->mouse[1]))
		// if(DEBUG_GRAPH(0))
		if(dbg) WndPrint("(%s) Mse%s en (%d, %d) + (%d, %d)\n",__func__,
			(e->code == WND_MSERIGHT)?"Right":"Left",e->x,e->y,cdr->dx,cdr->dy);
		if(!GraphClicEgale(clic,graph->mouse)) {
			OpiumRefreshAgain(cdr);
			if(DEBUG_GRAPH(3)) WndPrint("decalage: (%d, %d), evt: (%d, %d), cadre: (%d, %d)\n",
				f->x0,f->y0,e->x,e->y,cdr->larg,cdr->haut);
//-			graph->mouse[0] = e->x;
//-			graph->mouse[1] = e->y;
			GraphClicCopie(graph->mouse,clic);
			// WndContextFgndRGB(f,gc,GRF_RGB_RED);
#define CLASSIQUE
		#ifdef CLASSIQUE
			doit_terminer = WndRefreshBegin(f);
			if(OpiumZoomEnCours) {
				WndLine(f,gc,0,OpiumZoom0[1],cdr->dh,0);
				WndLine(f,gc,OpiumZoom0[0],0,0,cdr->dv);
			}
			WndLine(f,gc,0,e->y,cdr->dh,0);
			WndLine(f,gc,e->x,0,0,cdr->dv);
		#else
			WndExtraInit(f,5);
			if(OpiumZoomEnCours) {
				WndExtraAdd(f,WND_ACTN_LINE,gc,0,OpiumZoom0[1],cdr->dh,0,0);
				WndExtraAdd(f,WND_ACTN_LINE,gc,OpiumZoom0[0],0,0,cdr->dv,0);
			}
			WndExtraAdd(f,WND_ACTN_LINE,gc,0,e->y,cdr->dh,0,0);
			WndExtraAdd(f,WND_ACTN_LINE,gc,e->x,0,0,cdr->dv,0);
		#endif

			/* impression des coordonnees */
//			xscale = graph->axe;
//			x = e->x + cdr->dx;
//			GraphPosUser(xscale,x,&GraphUser[0].i1,&GraphUser[0].r1);
//			if(!xscale->reel) GraphUser[0].r1 = (float)GraphUser[0].i1; // pour avoir la meme valeur entiere
//			yscale = xscale + 1;
//			y = cdr->haut - e->y - cdr->dy;
//			GraphPosUser(yscale,y,&GraphUser[1].i1,&GraphUser[1].r1);
//			if(!yscale->reel) GraphUser[1].r1 = (float)GraphUser[1].i1; // pour avoir la meme valeur entiere
			GraphClicCopie(absol,clic); GraphClicAjoute(absol,of7); GraphClicCoord(coord,absol,cdr);
			for(i=0; i<2; i++) {
				GraphPosUser(&(graph->axe[i]),coord[i],&(GraphUser[i].i1),&(GraphUser[i].r1));
				if(!graph->axe[i].reel) GraphUser[i].r1 = (float)GraphUser[i].i1; // pour avoir la meme valeur entiere
			}
			trois_d = 0;
			/* impression de la valeur en Z si elle est disponible (histo 2d) */
			ab6 = graph->data_ptr[graph->axe[0].num];
			ord = graph->data_ptr[graph->axe[1].num];
			if((ab6 && ord && (ab6->parm & GRF_ADRS) == GRF_INDEX) && ((ord->parm & GRF_ADRS) == GRF_INDEX)) {
				GraphData *data_ptr,data; int num;
				char fctn,reel,court,xok,yok; int type;
				float *rtab; short *stab; int *itab;
				float rval; int ival;

				xok = yok = 0;
				type = ab6->parm & GRF_TYPE;
				court = (type == GRF_SHORT); reel = (type == GRF_FLOAT);
				if(reel) {        rtab = (float *)ab6->fctn; if(rtab[1] != 0.0f) i = (int)((GraphUser[0].r1 - rtab[0] + (rtab[1] / 2.0f)) / rtab[1]); xok = 1; }
				else if(court) {  stab = (short *)ab6->fctn; if(stab[1]) i = (GraphUser[0].i1 - stab[0] + (stab[1] / 2  )) / stab[1]; xok = 1; }
				else {            itab = (int *)ab6->fctn;   if(itab[1]) i = (GraphUser[0].i1 - itab[0] + (itab[1] / 2  )) / itab[1]; xok = 1; }
				type = ord->parm & GRF_TYPE;
				court = (type == GRF_SHORT); reel = (type == GRF_FLOAT);
				if(reel) {        rtab = (float *)ord->fctn; if(rtab[1] != 0.0f) j = (int)((GraphUser[1].r1 - rtab[0] + (rtab[1] / 2.0f)) / rtab[1]); yok = 1; }
				else if(court) {  stab = (short *)ord->fctn; if(stab[1]) j = (GraphUser[1].i1 - stab[0] + (stab[1] / 2  )) / stab[1]; yok = 1; }
				else {            itab = (int *)ord->fctn;   if(itab[1]) j = (GraphUser[1].i1 - itab[0] + (itab[1] / 2  )) / itab[1]; yok = 1; }
				if(xok && yok) {
					k = i + (j * (ab6->max - ab6->min));
					data_ptr = graph->data_ptr;
					for (num = 0; num < graph->dim ; num++,data_ptr++) {
						data = *data_ptr; if(!data) continue;
						if(data->fctn == (void *)-1) continue; // 0 permis a ce niveau?
						if(data->fctn == 0) continue; // mais pourquoi faire??
						if((data->parm & GRF_AXIS) == GRF_ZAXIS) {
							type = data->parm & GRF_TYPE;
							court = (type == GRF_SHORT); reel = (type == GRF_FLOAT);
							fctn = (data->parm & GRF_ADRS) == GRF_FCTN;
							if(fctn) {
								void (*fc)(int, float*, int*) = (void (*)(int, float*, int*))(data->fctn);
								(*fc)(k,&rval,&ival);
							} else if(reel) { rtab = (float *)data->fctn; rval = rtab[k]; }
							else if(court) {  stab = (short *)data->fctn; ival = stab[k]; }
							else {            itab = (int *)data->fctn;   ival = itab[k]; }
							if(reel) z = rval; else z = (float)ival;
							trois_d = 1;
						}
					}
				}
			}
			if(trois_d) sprintf(graph->usermouse,"(%g, %g): %g",GraphUser[0].r1,GraphUser[1].r1,z); // "(%g, %g, %g)"
			else sprintf(graph->usermouse,"(%g, %g)",GraphUser[0].r1,GraphUser[1].r1);
			lngr = WndColToPix((int)strlen(graph->usermouse));
			px = e->x + 1;
			if((e->x + lngr) > cdr->dh) px -= lngr - 4;
#ifdef CLASSIQUE
			WndString(f,gc,px,e->y,graph->usermouse);
			if(doit_terminer) WndRefreshEnd(f);
#else
			WndExtraAdd(f,WND_ACTN_STRING,gc,px,e->y,0,0,graph->usermouse);
			if(WndExtraNb(f)) OpiumRefreshAgain(cdr);
#endif
		}
		break;
	} else if(e->type == WND_RELEASE) switch(e->code) {
	  case WND_MSERIGHT:
	  case WND_MSELEFT:
		// if(DEBUG_GRAPH(0))
		if(dbg) WndPrint("(%s) MseRelease en (%d, %d) + (%d, %d)\n",__func__,
			e->x,e->y,cdr->dx,cdr->dy);
		if(OpiumZoomEnCours) {
			GraphClicCopie(OpiumZoom1,clic);
			GraphClicCopie(z0,OpiumZoom0); GraphClicAjoute(z0,of7); GraphClicCoord(z0,z0,cdr);
			GraphClicCopie(z1,OpiumZoom1); GraphClicAjoute(z1,of7); GraphClicCoord(z1,z1,cdr);
			affichage[0] = cdr->dh; affichage[1] = cdr->dv;
			for(i=0; i<2; i++) if(OpiumZoom1[i] != OpiumZoom0[i]) {
				int bout,tempo,decal; int zoom; GraphScale *scale;

				scale = &(graph->axe[i]);
				if(OpiumZoom0[i] > OpiumZoom1[i]) { tempo = OpiumZoom0[i]; OpiumZoom0[i] = OpiumZoom1[i]; OpiumZoom1[i] = tempo; }
				bout = i? cdr->dv - 5: cdr->dh - 5;
				if((OpiumZoom0[i] < 5) && (OpiumZoom1[i] > bout) && (OpiumZoomAutorise > 0)) {
					/* zoom pres des bords: dezoom d'un facteur 2 */
					float dr; int di;
					if(scale->zoom == 1) {
						if(scale->reel) {
							dr = scale->u.r.max - scale->u.r.min; scale->u.r.min -= dr; scale->u.r.max += dr;
						} else {
							di = scale->u.i.max - scale->u.i.min; scale->u.i.min -= di; scale->u.i.max += di;
						}
					} else {
						zoom = scale->zoom / 2; if(zoom < 1) zoom = 1;
						GraphScaleZoom(scale,zoom);
						if(i) { cdr->haut = scale->lngr; cdr->dv = cdr->haut / scale->zoom; }
						else { cdr->larg = scale->lngr; cdr->dh = cdr->larg / scale->zoom; }
					}
					scale->minauto = scale->maxauto = 0; scale->pret = 0;
				} else {
					/* sinon, zoom loin des bords: zoom effectif */
					if((scale->zoom == 1) || (OpiumZoomAutorise <= 0)) {
						if(i) {
							GraphPosUser(scale,z1[i],&(GraphUser[i].i0),&(GraphUser[i].r0));
							GraphPosUser(scale,z0[i],&(GraphUser[i].i1),&(GraphUser[i].r1));
						} else {
							GraphPosUser(scale,z0[i],&(GraphUser[i].i0),&(GraphUser[i].r0));
							GraphPosUser(scale,z1[i],&(GraphUser[i].i1),&(GraphUser[i].r1));
						}
						if(OpiumZoomAutorise > 0) {
							if(scale->reel) {
								scale->u.r.min = GraphUser[i].r0; scale->u.r.max = GraphUser[i].r1;
							} else {
								scale->u.i.min = GraphUser[i].i0; scale->u.i.max = GraphUser[i].i1;
							}
							scale->minauto = scale->maxauto = 0; scale->pret = 0;
						}
					} else if(OpiumZoomAutorise > 0) {
						zoom = (scale->zoom * affichage[i]) / (OpiumZoom1[i] - OpiumZoom0[i]);
						decal = (scale->zoom * of7[i]) / (OpiumZoom1[i] - OpiumZoom0[i]); // plus complique, utiliser OpiumZoom0[i]
						GraphScaleZoom(scale,zoom);
						if(i) {
							cdr->haut = scale->lngr; // du coup, yscale->zoom < cdr->haut
							cdr->dv = cdr->haut / scale->zoom;
							cdr->dy = decal;
						} else {
							cdr->larg = scale->lngr; // du coup, xscale->zoom < cdr->larg
							cdr->dh = cdr->larg / scale->zoom;
							cdr->dx = decal;
						}
						scale->minauto = scale->maxauto = 0; scale->pret = 0;
					}
				}
			}
			OpiumZoomEnCours = 0;
			OpiumZoomAutorise = 0;
			OpiumZoom0[0] = OpiumZoom0[1] = 0;
			WndExtraInit(f,-1);
			OpiumRefreshAgain(cdr);
			if(cdr->modexec == OPIUM_EXEC) code_rendu = PNL_OK;
		} else if(OpiumZoomAutorise) {
//			OpiumZoom0[0] = graph->mouse[0];
//			OpiumZoom0[1] = graph->mouse[1];
			GraphClicCopie(OpiumZoom0,graph->mouse);
			OpiumZoomEnCours = 1;
		} else {
			if(DEBUG_GRAPH(3)) WndPrint("(OpiumRunGraph) Pas de zoom en cours, souris en (%d, %d)\n",graph->mouse[0],graph->mouse[1]);
			WndExtraInit(f,-1);
			if((graph->mouse[0] >= 0) && (graph->mouse[1] >= 0)) {
				OpiumRefreshAgain(cdr);
				graph->mouse[0] = graph->mouse[1] = -1;
			}
			if((e->x > (cdr->dh - WndColToPix(1))) && (e->y < WndLigToPix(1)))
				GraphDialogExec(cdr /*,e */);
			else {
				if(DEBUG_GRAPH(1)) WndPrint("(OpiumRunGraph) Pas le menu Pomme, modexec=%d/%d\n",cdr->modexec,OPIUM_EXEC);
				for(i=0; i<GRF_MAX_ZONES; i++) {
					if((e->x >= graph->zone[i].xmin) && (e->x < graph->zone[i].xmax)
					   && (e->y >= graph->zone[i].ymin) && (e->y < graph->zone[i].ymax)) {
						GraphZoneAcces(graph,i);
						break;
					}
				}
				if(DEBUG_GRAPH(3)) WndPrint("(OpiumRunGraph) Zone #%d/%d\n",i,GRF_MAX_ZONES);
				if(i >= GRF_MAX_ZONES) {
//					x = e->x + cdr->dx;
//					xscale = graph->axe;
//					GraphPosUser(xscale,x,&GraphUser[0].i0,&GraphUser[0].r0);
//					if(DEBUG_GRAPH(1)) WndPrint("(OpiumRunGraph) x = %d = %g\n",GraphUser[0].i0,GraphUser[0].r0);
//					y = cdr->haut - e->y - cdr->dy;
//					yscale = xscale + 1;
//					GraphPosUser(yscale,y,&GraphUser[1].i0,&GraphUser[1].r0);
//					if(DEBUG_GRAPH(1)) WndPrint("(OpiumRunGraph) y = %d = %g\n",GraphUser[1].i0,GraphUser[1].r0);
					GraphClicCopie(absol,clic); GraphClicAjoute(absol,of7); GraphClicCoord(coord,absol,cdr);
					for(i=0; i<2; i++) {
						GraphPosUser(&(graph->axe[i]),coord[i],&GraphUser[i].i0,&GraphUser[i].r0);
						if(DEBUG_GRAPH(1)) WndPrint("(OpiumRunGraph) %c = %d = %g\n",i?'y':'x',GraphUser[i].i0,GraphUser[i].r0);
					}
					GraphUserSaisi = 1;
					if(cdr->modexec == OPIUM_EXEC) code_rendu = PNL_OK;
				}
			}
		}
		break;
	  case WND_MSEMIDDLE:
		code_rendu = PNL_CANCEL;
		break;
	}

	// WndContextFree(f,gc);
	if(DEBUG_GRAPH(1)) WndPrint("(OpiumRunGraph) code rendu=%d\n",code_rendu);
	return(code_rendu);
}
/* ========================================================================== */
char GraphGetLast(int *ix, float *rx, int *iy, float *ry) {
	if(!GraphUserSaisi) return(0);
	*ix = GraphUser[0].i0;
	*rx = GraphUser[0].r0;
	*iy = GraphUser[1].i0;
	*ry = GraphUser[1].r0;
	GraphUserSaisi = 0;
	return(1);
}
/* ========================================================================== */
int GraphGetPoint(Graph graph, int *ix, float *rx, int *iy, float *ry) {
	char a_refaire; int reponse;

	if(!graph) return(0);
	a_refaire = OpiumDisplayed(graph->cdr);
	if(a_refaire) OpiumClear(graph->cdr);
	OpiumPrioritaire = graph->cdr;
	reponse = OpiumExec(graph->cdr);
	if(DEBUG_GRAPH(1)) WndPrint("(GraphGetPoint) reponse: %d\n",reponse);
	OpiumPrioritaire = 0;
	if(a_refaire) { OpiumDisplay(graph->cdr); /* (graph->cdr)->displayed = 1; */ }
	*ix = GraphUser[0].i0;
	*rx = GraphUser[0].r0;
	*iy = GraphUser[1].i0;
	*ry = GraphUser[1].r0;
	GraphUserSaisi = 0;
	return(reponse);
}
/* ========================================================================== */
int GraphGetRect(Graph graph, int *ix0, float *rx0, int *iy0, float *ry0,
	int *ix1, float *rx1, int *iy1, float *ry1) {
	char a_refaire; int reponse;

	if(!graph) return(0);
	a_refaire = OpiumDisplayed(graph->cdr);
	if(a_refaire) OpiumClear(graph->cdr);
	OpiumPrioritaire = graph->cdr;
	OpiumZoomAutorise = -1;
	reponse = OpiumExec(graph->cdr);
	OpiumPrioritaire = 0;
	if(a_refaire) OpiumDisplay(graph->cdr);
	*ix0 = GraphUser[0].i0;
	*rx0 = GraphUser[0].r0;
	*iy0 = GraphUser[1].i0;
	*ry0 = GraphUser[1].r0;
	*ix1 = GraphUser[0].i1;
	*rx1 = GraphUser[0].r1;
	*iy1 = GraphUser[1].i1;
	*ry1 = GraphUser[1].r1;
	GraphUserSaisi = 0;
	return(reponse);
}
/* ========================================================================== */
void GraphPosUser(GraphScale *scale, int x, int *ival, float *rval) {
	float pval;

	if(DEBUG_GRAPH(1)) printf("(%s) p=(%d - %d)/%g + %d\n",__func__,x,scale->marge_min,scale->pixel,scale->u.i.min);
	pval = (float)(x - scale->marge_min) / scale->pixel;
	if(scale->logd) *rval = (float)pow(10.0f,(double)pval + scale->logmin);
	else if(scale->reel) *rval = pval + scale->u.r.min;
	else *rval = pval + (float)scale->u.i.min;
	*ival = (int)(*rval + 0.5);
}
/* ========================================================================== */
void GraphEventUserInt(Graph graph, WndAction *e, int *ix, int *iy) {
	Cadre cdr; int x,y; float rx,ry;

	cdr = graph->cdr;
	x = e->x + cdr->dx;             GraphPosUser(graph->axe  ,x,ix,&rx);
	y = cdr->haut - e->y - cdr->dy; GraphPosUser(graph->axe+1,y,iy,&ry);
}
/* ========================================================================== */
void GraphEventUserFloat(Graph graph, WndAction *e, float *rx, float *ry) {
	Cadre cdr; int x,y; int ix,iy;

	cdr = graph->cdr;
	x = e->x + cdr->dx;             GraphPosUser(graph->axe  ,x,&ix,rx);
	y = cdr->haut - e->y - cdr->dy; GraphPosUser(graph->axe+1,y,&iy,ry);
}
/* ========================================================================== */
#define arrondi(x) (x >= 0.0f)? (int)(x+ 0.5): (int)(x - 0.5)
/* ========================================================================== */
/* ..................... fonctions a usage interne .......................... */
/* .......................................................................... */
static size_t GraphXcols(Graph graph, GraphScale *yscale) {
	float (*fctn)(Graph, int, float);
	size_t lngr,cols;
	float ry,rlim;
	char s[80];
	int nb;

	if(!yscale->logd && (yscale->grad < 0.0f)) { sprintf(GraphErrorText,"(%s) GradY=%g",__func__,yscale->grad); return(0); }
	fctn = (float (*)(Graph, int, float))yscale->chgt_unite;
	cols = 0;
	if(yscale->reel) rlim = yscale->u.r.max; else rlim = (float)yscale->u.i.max;
	ry = yscale->g1;
	nb = 0;
	while(ry <= rlim) {
		if(++nb > 20) break;
		if(!yscale->logd) { if(fabs((double)ry) < (yscale->grad / 100.0f)) ry = 0.0f; }
		if(yscale->reel) {
			if(fctn) snprintf(s,80,yscale->format,fctn(graph,0,ry));
			else snprintf(s,80,yscale->format,ry);
		} else {
			if(fctn) snprintf(s,80,yscale->format,fctn(graph,0,ry));
			else snprintf(s,80,"%d",arrondi(ry));
		}
		s[79] = '\0';
		lngr = strlen(s);
		if(lngr > cols) cols = lngr;
		if(yscale->logd) ry *= 10.0f; else ry += yscale->grad;
	};
	return(cols);
}
/* .......................................................................... */
static int GraphScaleDefine(Graph graph) {
/* Prepare les echelles */
	GraphScale *xscale,*yscale; GraphData data,*data_ptr;
	Cadre cdr; WndFrame f; WndColor *c;
	unsigned int type,epaisseur; int num,cols,qual,dy,mmin; char *nom;

	cdr = graph->cdr;
	xscale = graph->axe;
	yscale = xscale + 1;
	qual = WND_Q_ECRAN;

/* Choix de l'axe X */
	if(xscale->num == GRF_UNDEFINED) {
		data_ptr = graph->data_ptr;
		for(num=0; num<graph->dim; num++, data_ptr++) {
			data = *data_ptr; if(!data) continue;
			if((data->parm & GRF_AXIS) == GRF_XAXIS) { /* >= 0 */
				if(((data->parm & GRF_ADRS) == GRF_INDIV)
				|| ((data->trace[qual] & GRF_MODE) == GRF_CODE_ERR))
					continue;
				GraphAxisDefine(graph,num); break;
			}
		}
	}
	if(xscale->num == GRF_UNDEFINED) {
		sprintf(GraphErrorText,"(%s) Abcisse non definie pour le graphe '%s'",__func__,cdr->nom);
		return(0);
	}
//	if(DEBUG_GRAPH(1)) WndPrint("(%s) Abcisse graphe %s: tableau #%d\n",__func__,cdr->nom,xscale->num);

/* Choix de l'axe Y */
	if(yscale->num == GRF_UNDEFINED) {
		data_ptr = graph->data_ptr;
		for(num=0; num<graph->dim; num++, data_ptr++) {
			data = *data_ptr; if(!data) continue;
			if((data->parm & GRF_AXIS) == GRF_YAXIS) { /* < 0 */
				if(((data->parm & GRF_ADRS) == GRF_INDIV)
				|| ((data->trace[qual] & GRF_MODE) == GRF_CODE_ERR))
					continue;
				GraphAxisDefine(graph,num); break;
			}
		}
	}
	if(yscale->num == GRF_UNDEFINED) {
		sprintf(GraphErrorText,"(%s) Aucune courbe definie pour le graphe '%s'",__func__,cdr->nom);
		return(0);
	}
//	if(DEBUG_GRAPH(1)) WndPrint("(%s) Ordonnee graphe '%s': tableau #%d\n",__func__,cdr->nom,yscale->num);

/* Types de trace */
	if(!graph->pret) {
		// printf("\n");
		data_ptr = graph->data_ptr;
		for(num=0; num<graph->dim; num++, data_ptr++) {
			data = *data_ptr; if(!data) continue;
			f = cdr->f;
			if((data->couleur.type == GRF_NOM) || (data->couleur.type == GRF_RGB)) for(qual=0; qual<WND_NBQUAL; qual++) {
				// printf("(%s:%d) ---------- @%08lX: DATA[%d].gc[%d], couleur %s\n",__func__,__LINE__,(Ulong)data,num,qual,(data->couleur.type == GRF_RGB)? "rgb": "nommee");
				if(data->gc[qual]) WndContextFree(cdr->f,data->gc[qual]);
				data->gc[qual] = WndContextSupportCreate(cdr->f,qual);
				// PRINT_GC(data->gc[qual]); PRINT_COLOR(data->gc[qual]->foreground); PRINT_COLOR(data->gc[qual]->background);
				if(data->couleur.type == GRF_NOM) {
					nom = &(data->couleur.def.nom[qual][0]);
					if(nom[0] == '\0') c = WndColorText[qual]; else c = WndColorGetFromName(nom);
					// PRINT_COLOR(WndColorText[WND_Q_PAPIER]); PRINT_COLOR(WndColorText[WND_Q_ECRAN]); PRINT_COLOR(c);
					if(!WndContextFgndColor(cdr->f,data->gc[qual],c)) {
						// OpiumError("(%s) couleur %s inexistante",__func__,&(data->couleur.def.nom[qual][0]));
					}
				} else if(data->couleur.type == GRF_RGB) {
					// printf("(%s:%d) rgb=%04X.%04X.%04X\n",__func__,__LINE__,data->couleur.def.rgb[qual].r,data->couleur.def.rgb[qual].g,data->couleur.def.rgb[qual].b);
					if(!WndContextFgndRGB(cdr->f,data->gc[qual],
						data->couleur.def.rgb[qual].r,data->couleur.def.rgb[qual].g,data->couleur.def.rgb[qual].b)) {
						// OpiumError("(%s) couleur %04X%04X%04X inexistante",__func__,
						// data->couleur.def.rgb[qual].r,data->couleur.def.rgb[qual].g,data->couleur.def.rgb[qual].b);
					}
				} else {
					// OpiumError("(%s) couleur de type %d (indefini) pour le graphe '%s'\n",__func__,data->couleur.type,cdr->nom);
				}
				if((data->trace[qual] & GRF_MODE) == GRF_CODE_PNT) type = 0;
				else type = (data->trace[qual] & GRF_LINE) >> GRF_SHIFT_LINE;
				epaisseur = (data->trace[qual] & GRF_WIDZ) >> GRF_SHIFT_WIDZ;
				// printf("(%s.2[%d]) epaisseur = %d\n",__func__,num,epaisseur);
				WndContextLine(f,data->gc[qual],type,epaisseur);
				// printf("(%s.3[%d]) @%08lX.%08lX.line_width = %d\n",__func__,num,(Ulong)data,(Ulong)(data->gc[qual]),(data->gc[qual])->line_width);
				// PRINT_COLOR(data->gc[qual]->foreground); PRINT_COLOR(data->gc[qual]->background);
			}
		}
		graph->pret = 1;
	}

/* Calcul des graduations utilisateur puis des marges */
	if(!yscale->pret) {
		dy = WndLigToPix(1);
		if(!GraphGradsDefine(graph,GRF_YAXIS,graph->axe[1].zoom)) {
			// sprintf(GraphErrorText,"(%s) Graduations Y non definies pour le graphe '%s'",__func__,cdr->nom);
			return(0);
		}
#ifdef MARGE_SELON_AXES
		if(!graph->avec_axes) {
			yscale->marge_min = 0;
			yscale->marge_max = 0;
		} else if(graph->avec_4_axes) {
			yscale->marge_max = yscale->marge_min = graph->grad + graph->of7 + dy;
		} else if(yscale->position == GRF_GRADINF) {
			yscale->marge_min = graph->grad + graph->of7 + dy;
			yscale->marge_max = 0;
		} else if(yscale->position == GRF_GRADSUP) {
			yscale->marge_min = 0;
			yscale->marge_max = graph->grad + graph->of7 + dy;
		} else {
			yscale->marge_min = 0;
			yscale->marge_max = 0;
		}
		yscale->utile = cdr->haut - yscale->marge_min - yscale->marge_max;
#endif
		if((graph->mode & GRF_NBAXES) == GRF_0AXES) {
			yscale->marge_min = yscale->marge_max = 0;
		} else {
			yscale->marge_min = graph->grad + graph->of7 + dy;
			if((graph->mode & GRF_NBAXES) == GRF_2AXES) yscale->marge_max = dy / 2;
			else yscale->marge_max = graph->grad + graph->of7;
			mmin = 3 * dy / 2;
			if(yscale->titre[0]) { if(yscale->marge_max < mmin) yscale->marge_max = mmin; }
		}
		if(cdr->titre[0] || xscale->titre[0]) yscale->marge_min += dy;
		yscale->marge_totale = yscale->marge_min + yscale->marge_max;
		yscale->pret = 1;
		if(xscale->pret) {
			if((cols = (int)GraphXcols(graph,yscale)) != xscale->cols) {
				xscale->cols = (short)WndColToPix(cols);
				if((graph->mode & GRF_NBAXES) == GRF_0AXES) xscale->marge_min = 0;
				else xscale->marge_min = graph->grad + graph->of7 + xscale->cols;
				xscale->marge_max = xscale->marge_min;
				xscale->marge_totale = xscale->marge_min + xscale->marge_max;
				xscale->utile = cdr->larg - xscale->marge_totale;
				if(xscale->ech != 0.0f) xscale->pixel = (float)xscale->utile / xscale->ech;
				else xscale->pixel = 1.0f;
			}
		}
		// printf("(%s) GradY=%g\n",__func__,yscale->grad);
	} // else printf("(%s) Axe Y deja pret\n",__func__);
	if(DEBUG_GRAPH(1)) printf("(%s) Echelle Y prete\n",__func__);

	if(!xscale->pret) {
		if(!GraphGradsDefine(graph,GRF_XAXIS,graph->axe[0].zoom)) {
			// sprintf(GraphErrorText,"(%s) Graduations X non definies pour le graphe '%s'",__func__,cdr->nom);
			return(0);
		}
		if((graph->mode & GRF_NBAXES) == GRF_0AXES) {
			xscale->cols = 0;
			xscale->marge_min = 0;
//			if(DEBUG_GRAPH(1)) printf("(%s) Axe X absent pour le graphe '%s'\n",__func__,cdr->nom);
		} else {
			cols = (int)GraphXcols(graph,yscale);
			xscale->cols = (short)WndColToPix(cols);
			if((graph->mode & GRF_NBAXES) == GRF_0AXES) xscale->marge_min = 0;
			else xscale->marge_min = graph->grad + graph->of7 + xscale->cols;
			xscale->pret = 1;
			//			printf("(%s) GradX=%g\n",__func__,xscale->grad);
		} // else printf("(%s) Axe X deja pret\n",__func__);
		xscale->marge_max = xscale->marge_min;
		xscale->marge_totale = xscale->marge_min + xscale->marge_max;
//		if(DEBUG_GRAPH(1)) printf("(%s) Echelle X prete\n",__func__);
	}

	yscale->utile = cdr->haut - yscale->marge_totale;
	xscale->utile = cdr->larg - xscale->marge_totale;
	if(DEBUG_GRAPH(1)) WndPrint("(%s) Axes prepares: Lx=%d/%g,Ly=%d/%g\n",__func__,xscale->utile,xscale->ech,yscale->utile,yscale->ech);
	if(xscale->ech != 0.0f) xscale->pixel = (float)xscale->utile / xscale->ech;
	else xscale->pixel = 1.0f;
	if(yscale->ech != 0.0f) yscale->pixel = (float)yscale->utile / yscale->ech;
	else yscale->pixel = 1.0f;
	return(1);
}
/* .......................................................................... */
static int GraphGradsExtrema(Graph graph, unsigned char axe) {
	GraphScale *scale; GraphData data; Cadre cdr; char prem,court,reel,fctn; int i,j,nb,pmin,pmax;
	int *itab; int ival,imin,imax;
	short *stab; short sval;
	float *rtab; float rval; double rmin,rmax;
	TypeFctn fc;
#ifndef MAX_SUR_AXE
	int num; GraphData *data_ptr;
#endif

	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	if((scale->num & 0xFFFF) == GRF_DIRECT) return(0);
/* 	Valeurs minimum et/ou maximum dans le tableau */
	cdr = graph->cdr;
	if((scale->num < 0) || (scale->num >= graph->dim)) {
		sprintf(GraphErrorText,"(%s) Numero d'axe (%d) incorrect: pas dans [0..%d[ pour le graphe '%s'",__func__,scale->num,graph->dim,cdr->nom);
		return(0);
	}
	prem = 1;
#ifdef MAX_SUR_AXE
	data = graph->data_ptr[scale->num];
	if(!data) {
		cdr = graph->cdr;
		sprintf(GraphErrorText,"(%s) Axe %c defini sur un tableau pas alloue en memoire (#%d), pour le graphe '%s'",__func__,
			(axe == GRF_YAXIS)? 'Y': 'X',scale->num,cdr->nom);
		return(0);
	}
	if(DEBUG_GRAPH(2)) WndPrint("(%s) Parms de l'axe @%08lX (data #%d)=%02x\n",__func__,(Ulong)scale,scale->num,data->parm);
#else
	data_ptr = graph->data_ptr;
	for(num=0; num<graph->dim; num++, data_ptr++) {
		data = *data_ptr; if(!data) continue;
		if((data->parm & GRF_AXIS) != axe) continue;
#endif
		itab = (int *)0;               /* pour gcc */
		stab = (short *)0;             /* pour gcc */
		rtab = (float *)0;             /* pour gcc */
		imin = imax = 0; rmin = rmax = 0.0f; /* pour gcc */
		fc = 0;                             /* pour gcc */
		nb = data->max - data->min;
		if(((data->min != 0) || (data->max != 0)) && (nb <= 0)) nb += data->dim;
		if(!nb) {
			continue;
//			cdr = graph->cdr;
//			sprintf(GraphErrorText,"(%s) Pas de valeur sur l'axe @%d (valeurs dans [%d, %d[) pour le graphe '%s'",__func__,
//				scale->num,data->min,data->max,cdr->nom);
//			return(0);
		}
#ifdef MAX_SUR_AXE
		if(!data->fctn || (data->fctn == (void *)-1)) {
			sprintf(GraphErrorText,"(%s) Axe %c defini sur un tableau inactive (#%d), pour le graphe '%s'",__func__,
				(axe == GRF_YAXIS)? 'Y': 'X',scale->num,cdr->nom);
			return(0);
		}
#else
		if(!data->fctn || (data->fctn == (void *)-1)) continue;
#endif
		pmin = data->min; pmax = pmin + nb - 1;
		court = (data->parm & GRF_TYPE) == GRF_SHORT;
		reel = (data->parm & GRF_TYPE) == GRF_FLOAT;
		if(num == scale->num) scale->reel = reel;
		fctn = (data->parm & GRF_ADRS) == GRF_FCTN;
		if(fctn) fc = (TypeFctn)data->fctn;
		else if(reel) rtab = (float *)data->fctn + pmin;
		else if(court) stab = (short *)data->fctn + pmin;
		else itab = (int *)data->fctn + pmin;
		if(DEBUG_GRAPH(1)) WndPrint("(%s) Data #%d: %d valeurs %s @%08lX (pmin=%d)\n",__func__,
			num,nb,reel?"reelles":"entieres",reel?(Ulong)rtab:(Ulong)itab,pmin);

		if(scale->minauto && scale->maxauto) {
			if((data->parm & GRF_ADRS) == GRF_INDEX) {
				if(reel) {
					rtab = (float *)data->fctn;
					rmin = *rtab + (pmin * *(rtab+1));
					rmax = *rtab + (pmax * *(rtab+1));
					if(DEBUG_GRAPH(1)) WndPrint("(%s) | index debut: %g, pas: %g, entre %d et %d => min=%g, max=%g\n",__func__,
						*rtab,*(rtab+1),pmin,pmax,rmin,rmax);
				} else if(court) {
					stab = (short *)data->fctn;
					imin = *stab + (pmin * *(stab+1));
					imax = *stab + (pmax * *(stab+1));
				} else {
					itab = (int *)data->fctn;
					imin = *itab + (pmin * *(itab+1));
					imax = *itab + (pmax * *(itab+1));
				}
			} else {
				rmin = FLOAT_MAX; rmax = -rmin;
				imin = 0x7FFFFFFF; imax = -imin;
				for(i=0, j=pmin; i<nb; i++,j++) {
					if(j >= data->dim) {
						j -= data->dim;
						if(reel) rtab = (float *)data->fctn + j;
						else if(court) stab = (short *)data->fctn + j;
						else itab = (int *)data->fctn + j;
					}
					if(fctn) {
						if(reel) {
							void (*fctnTmp)(int, float*) =  (void (*)(int, float*))fc;
							(*fctnTmp)(j,&rval);
							if(rval < rmin) rmin = rval;
							if(rval > rmax) rmax = rval;
						} else if(court) {
							void (*fctnTmp)(int, short*) =  (void (*)(int, short*))fc;
							(*fctnTmp)(j,&sval);
							if(sval < imin) imin = sval;
							if(sval > imax) imax = sval;
						} else {
							void (*fctnTmp)(int, int*) =  (void (*)(int, int*))fc;
							(*fctnTmp)(j,&ival);
							if(ival < imin) imin = ival;
							if(ival > imax) imax = ival;
						}
					} else {
						if(reel) {
							if(*rtab < rmin) rmin = *rtab;
							if(*rtab > rmax) rmax = *rtab;
							rtab++;
						} else if(court) {
							if(*stab < imin) imin = *stab;
							if(*stab > imax) imax = *stab;
							stab++;
						} else {
							if(*itab < imin) imin = *itab;
							if(*itab > imax) imax = *itab;
							itab++;
						}
					}
				}
			}
			if(reel) {
				rmin += data->of7.r.v; rmax += data->of7.r.v;
				if(prem) {
					if(scale->reel) {
						scale->u.r.min = (float)rmin; scale->u.r.max = (float)rmax;
					} else {
						scale->u.i.min = (int)rmin; scale->u.i.max = (int)rmax;
					}
				} else {
					if(scale->reel) {
						if((float)rmin < scale->u.r.min) scale->u.r.min = (float)rmin;
						if((float)rmax > scale->u.r.max) scale->u.r.max = (float)rmax;
					} else {
						if((int)rmin < scale->u.i.min) scale->u.i.min = (int)rmin;
						if((int)rmax > scale->u.i.max) scale->u.i.max = (int)rmax;
					}
				}
				if(DEBUG_GRAPH(1)) WndPrint("(%s) | Rmin=%g, Rmax=%g\n",__func__,rmin,rmax);
			} else {
				imin += data->of7.i.v; imax += data->of7.i.v;
				if(prem) {
					if(scale->reel) {
						scale->u.r.min = (float)imin; scale->u.r.max = (float)imax;
					} else {
						scale->u.i.min = imin; scale->u.i.max = imax;
					}
				} else {
					if(scale->reel) {
						if((float)imin < scale->u.r.min) scale->u.r.min = (float)imin;
						if((float)imax > scale->u.r.max) scale->u.r.max = (float)imax;
					} else {
						if(imin < scale->u.i.min) scale->u.i.min = imin;
						if(imax > scale->u.i.max) scale->u.i.max = imax;
					}
				}
				if(DEBUG_GRAPH(1)) WndPrint("(%s) | Imin=%d, Imax=%d\n",__func__,imin,imax);
			}
		} else if(scale->minauto) {
			if((data->parm & GRF_ADRS) == GRF_INDEX) {
				if(reel) rmin = *rtab + (pmin * *(rtab+1));
				else if(court) imin = *stab + (pmin * *(stab+1));
				else imin = *itab + (pmin * *(itab+1));
			} else {
				rmin = FLOAT_MAX;
				imin = 0x7FFFFFFF;
				for(i=0, j=pmin; i<nb; i++,j++) {
					if(j >= data->dim) {
						j -= data->dim;
						if(reel) rtab = (float *)data->fctn + j;
						else if(court) stab = (short *)data->fctn + j;
						else itab = (int *)data->fctn + j;
					}
					if(fctn) {
						if(reel) {
							void (*fctnTmp)(int, float*) =  (void (*)(int, float*))fc;
							(*fctnTmp)(j,&rval);
							if(rval < rmin) rmin = rval;
						} else if(court) {
							void (*fctnTmp)(int, short*) =  (void (*)(int, short*))fc;
							(*fctnTmp)(j,&sval);
							if(sval < imin) imin = sval;
						} else {
							void (*fctnTmp)(int, int*) =  (void (*)(int, int*))fc;
							(*fctnTmp)(j,&ival);
							if(ival < imin) imin = ival;
						}
					} else {
						if(reel) {
							if(*rtab < rmin) rmin = *rtab;
							rtab++;
						} else if(court) {
							if(*stab < imin) imin = *stab;
							stab++;
						} else {
							if(*itab < imin) imin = *itab;
							itab++;
						}
					}
				}
			}
			if(reel) {
				rmin += data->of7.r.v;
				if(scale->reel) {
					if(prem) scale->u.r.min = (float)rmin;
					else if(rmin < scale->u.r.min) scale->u.r.min = (float)rmin;
				} else {
					if(prem) scale->u.i.min = (int)rmin;
					else if((int)rmin < scale->u.i.min) scale->u.i.min = (int)rmin;
				}
				if(DEBUG_GRAPH(1)) WndPrint("(%s) | Rmin=%e (Rmax=%e)\n",__func__,rmin,scale->u.r.max);
			} else {
				imin += data->of7.i.v;
				if(scale->reel) {
					if(prem) scale->u.r.min = (float)imin;
					else if((float)imin < scale->u.r.min) scale->u.r.min = (float)imin;
				} else {
					if(prem) scale->u.i.min = imin;
					else if(imin < scale->u.i.min) scale->u.i.min = imin;
				}
				if(DEBUG_GRAPH(1)) WndPrint("(%s) | Imin=%d (Imax=%d)\n",__func__,imin,scale->u.i.max);
			}
		} else /* if(scale->maxauto) */ {
			if((data->parm & GRF_ADRS) == GRF_INDEX) {
				if(reel) rmax = *rtab + (pmax * *(rtab+1));
				else if(court) imax = *stab + (pmax * *(stab+1));
				else imax = *itab + (pmax * *(itab+1));
			} else {
				rmax = FLOAT_MAX; rmax = -rmax;
				imax = 0x7FFFFFFF; imax = -imax;
				for(i=0, j=pmin; i<nb; i++,j++) {
					if(j >= data->dim) {
						j -= data->dim;
						if(reel) rtab = (float *)data->fctn + j;
						else if(court) stab = (short *)data->fctn + j;
						else itab = (int *)data->fctn + j;
					}
					if(fctn) {
						if(reel) {
							void (*fctnTmp)(int, float*) =  (void (*)(int, float*))fc;
							(*fctnTmp)(j,&rval);
							if(rval > rmax) rmax = rval;
						} else if(court) {
							void (*fctnTmp)(int, short*) =  (void (*)(int, short*))fc;
							(*fctnTmp)(j,&sval);
							if(sval > imax) imax = sval;
						} else {
							void (*fctnTmp)(int, int*) =  (void (*)(int, int*))fc;
							(*fctnTmp)(j,&ival);
							if(ival > imax) imax = ival;
						}
					} else {
						if(reel) {
							if(*rtab > rmax) rmax = *rtab;
							rtab++;
						} else if(court) {
							if(*stab > imax) imax = *stab;
							stab++;
						} else {
							if(*itab > imax) imax = *itab;
							itab++;
						}
					}
				}
			}
			if(reel) {
				rmax += data->of7.r.v;
				if(scale->reel) {
					if(prem) scale->u.r.max = (float)rmax;
					else if(rmax > scale->u.r.max) scale->u.r.max = (float)rmax;
				} else {
					if(prem) scale->u.i.max = (int)rmax;
					else if((int)rmax > scale->u.i.max) scale->u.i.max = (int)rmax;
				}
				if(DEBUG_GRAPH(1)) WndPrint("(%s) | (Rmin=%e) Rmax=%e\n",__func__,scale->u.r.min,rmax);
			} else {
				imax += data->of7.i.v;
				if(scale->reel) {
					if(prem) scale->u.r.max = (float)imax;
					else if((float)imax > scale->u.r.max) scale->u.r.max = (float)imax;
				} else {
					if(prem) scale->u.i.max = imax;
					else if(imax > scale->u.i.max) scale->u.i.max = imax;
				}
				if(DEBUG_GRAPH(1)) WndPrint("(%s) | (Imin=%d) Imax=%d\n",__func__,scale->u.i.min,imax);
			}
		}
		if(DEBUG_GRAPH(1)) {
			if(scale->reel) WndPrint("(%s) => %cscale->u.r = [%g .. %g]\n",__func__,(axe == GRF_YAXIS)? 'Y': 'X',scale->u.r.min,scale->u.r.max);
			else WndPrint("(%s) => %cscale->u.i = [%d .. %d]\n",__func__,(axe == GRF_YAXIS)? 'Y': 'X',scale->u.i.min,scale->u.i.max);
		}
#ifndef MAX_SUR_AXE
		prem = 0;
	}
#endif

//	WndPrint("(%s) scale->u.r.min=%e, scale->u.r.max=%e\n",__func__,scale->u.r.min,scale->u.r.max);
	if(scale->reel) {
		if(scale->u.r.min == scale->u.r.max) {
//			sprintf(GraphErrorText,"(%s) Axe %c defini sur un tableau constant (#%d), pour le graphe '%s'",__func__,
//					(axe == GRF_YAXIS)? 'Y': 'X',scale->num,cdr->nom);
//			return(0);
			scale->u.r.min -= 0.5f;
			scale->u.r.max += 0.5f;
			return(1);
		}
	} else {
		if(scale->u.i.min == scale->u.i.max) {
//			sprintf(GraphErrorText,"(%s) Axe %c defini sur un tableau constant (#%d), pour le graphe '%s'",__func__,
//					(axe == GRF_YAXIS)? 'Y': 'X',scale->num,cdr->nom);
//			return(0);
			if((Ulong)(scale->u.i.min) != 0x80000000) scale->u.i.min -= 1;
			if((Ulong)(scale->u.i.max) != 0x7FFFFFFF) scale->u.i.max += 1;
			return(1);
		}
	}
	return(1);
}
/* .......................................................................... */
static void GraphGradsValide(Graph graph, GraphScale *scale) {
	float rlim,nbgrad_max; char trop_de_grads;

	if(scale->reel) rlim = scale->u.r.max; else rlim = (float)scale->u.i.max;
	nbgrad_max = 20.0f;
	if(scale->logd) trop_de_grads = ((scale->g1 + powf(10.0f,nbgrad_max)) < rlim);
	else trop_de_grads = ((scale->g1 + (nbgrad_max * scale->grad)) < rlim);
	if(trop_de_grads) {
		if((rlim > -1.0e6) && (rlim < 1.0e6)) scale->grad = rlim / (float)10.0f;
		else scale->grad = 1.0f;
		scale->g1 = scale->grad;
	}
}
/* .......................................................................... */
static int GraphGradsFloatLineaires(Graph graph, GraphScale *scale, unsigned int zoom) {
	double echelle,nbgrad; float (*fctn)(Graph, int, float); double grad; int igrad;

/* Variables auxiliaires pour manipuler les graduations */
	echelle = (double)scale->u.r.max - (double)scale->u.r.min;
	scale->ech = (float)echelle;

/* Valeur utilisateur automatique d'une graduation */
	if(scale->gradauto) {
		echelle /= (double)zoom;
		if((fctn = (float (*)(Graph, int, float))scale->chgt_unite)) {
			echelle = (double)fctn(graph,0,(float)echelle);
			grad = OpiumGradsLineaires(echelle);
			scale->grad = fctn(graph,1,(float)grad);
		} else scale->grad = (float)OpiumGradsLineaires(echelle);
	}
	// printf("(%s) graduation #%d %s: %g\n",__func__,scale->num,(scale->gradauto)?"auto":"manu",scale->grad);

/* position de l'autre axe (si 2 axes), type de graduation, valeur de la premiere graduation */
//	modf(scale->u.r.min/scale->grad,&nbgrad);
	if(scale->grad < 1.0e-6) scale->grad = (float)1.0e-6;
	igrad = (int)(scale->u.r.min / scale->grad);
	nbgrad = (double)igrad;
	if(scale->u.r.min >= 0.0f) {
		scale->u.r.axe = scale->u.r.min; nbgrad += 1.0f;
		scale->typegrad = GRF_GRADINF;
	} else if(scale->u.r.max >= 0.0f) {
		scale->u.r.axe = 0.0f;
		scale->typegrad = GRF_GRADBOTH;
	} else {
		scale->u.r.axe = scale->u.r.max;
		scale->typegrad = GRF_GRADSUP;
	}
//	scale->g1 = nbgrad * scale->grad;
	scale->g1 = (float)igrad * scale->grad;
	GraphGradsValide(graph,scale);

	if(DEBUG_GRAPH(1)) WndPrint("Position utilisateur de l'autre axe: %g\n",scale->u.r.axe);

	return(1);
}
/* .......................................................................... */
static int GraphGradsIntLineaires(Graph graph, GraphScale *scale, unsigned int zoom) {
	int igrad; double echelle; float (*fctn)(Graph, int, float); double grad;

/* Variables auxiliaires pour manipuler les graduations */
	echelle = (double)(scale->u.i.max - scale->u.i.min);
	scale->ech = (float)echelle;

/* Valeur utilisateur automatique d'une graduation */
	if(scale->gradauto) {
		echelle /= (double)zoom;
		if((fctn = (float (*)(Graph, int, float))scale->chgt_unite)) {
			echelle = (double)fctn(graph,0,(float)echelle);
			grad = OpiumGradsLineaires(echelle);
			scale->grad = fctn(graph,1,(float)grad);
		} else scale->grad = (float)OpiumGradsLineaires(echelle);
	}

/* position de l'autre axe (si 2 axes), type de graduation, valeur de la premiere graduation */
	if(scale->grad < 1.0f) scale->grad = 1.0f;
	igrad = (int)((float)scale->u.i.min / scale->grad);
	if(scale->u.i.min >= 0) {
		scale->u.i.axe = scale->u.i.min; igrad++;
		scale->typegrad = GRF_GRADINF;
	} else if(scale->u.i.max >= 0.0f) {
		scale->u.i.axe = 0;
		scale->typegrad = GRF_GRADBOTH;
	} else {
		scale->u.i.axe = scale->u.i.max;
		scale->typegrad = GRF_GRADSUP;
	}
	scale->g1 = (float)igrad * scale->grad;
	GraphGradsValide(graph,scale);

	if(DEBUG_GRAPH(1)) WndPrint("Position utilisateur de l'autre axe: %d\n",scale->u.i.axe);

	return(1);
}
/* .......................................................................... */
static int GraphGradsFloatLog(GraphScale *scale) {
	double logmin,logmax; double echelle;
/* Variables auxiliaires pour manipuler les graduations */
	if((scale->u.r.min <= 0.0f) || (scale->u.r.max <= 0.0f)) {
//		sprintf(GraphErrorText,"(GraphGradsFloatLog) Limite negative pour un axe logarithmique");
		if(scale->u.r.min < 0.0f) { scale->u.r.min = 0.001f; scale->minauto = 0; }
		if(scale->u.r.max < 0.0f) { scale->u.r.max = scale->u.r.min * 10.0f; scale->maxauto = 0; }
//		return(0);
	}
	scale->logmin = log10((double)scale->u.r.min);
	logmin = scale->logmin; if(logmin > scale->logmin) logmin--;
	echelle = log10((double)scale->u.r.max);
	logmax = echelle; if(logmax < echelle) logmax++;
	echelle = echelle - scale->logmin;
	scale->ech = (float)echelle;

/* Valeur utilisateur automatique d'une graduation */
	if(scale->gradauto) scale->grad = (float)(((logmax - logmin) <= 5.0)? 1.0: -1.0);

/* position de l'autre axe (si 2 axes), type de graduation, valeur de la premiere graduation */
	scale->u.r.axe = scale->u.r.min;
	scale->typegrad = GRF_GRADINF;
	scale->g1 = (float)pow(10.0,logmin);
	return(1);
}
/* .......................................................................... */
static int GraphGradsIntLog(GraphScale *scale) {
	double logmin,logmax; double echelle;

/* Variables auxiliaires pour manipuler les graduations */
	if((scale->u.i.min <= 0) || (scale->u.i.max <= 0)) {
//		sprintf(GraphErrorText,"(GraphGradsIntLog) Limite negative pour un axe logarithmique");
		if(scale->u.i.min <= 0) { scale->u.i.min = 1; scale->minauto = 0; }
		if(scale->u.i.max <= 0) { scale->u.i.max = scale->u.i.min * 10; scale->maxauto = 0; }
//		return(0);
	}
	scale->logmin = log10((double)scale->u.i.min);
	logmin = scale->logmin; if(logmin > scale->logmin) logmin--;
	echelle = log10((double)scale->u.i.max);
	logmax = echelle; if(logmax < echelle) logmax++;
	echelle = echelle - scale->logmin;
	scale->ech = (int)echelle;

/* Valeur utilisateur automatique d'une graduation */
	if(scale->gradauto) scale->grad = (float)(((logmax - logmin) <= 5.0)? 1.0: -1.0);

/* position de l'autre axe (si 2 axes), type de graduation, valeur de la premiere graduation */
	scale->u.i.axe = scale->u.i.min;
	scale->typegrad = GRF_GRADINF;
	scale->g1 = (int)pow(10.0,logmin);
	return(1);
}
/* .......................................................................... */
static int GraphGradsDefine(Graph graph, unsigned char axe, unsigned int zoom)  {
/* Rempli la structure <scale> */
	GraphScale *scale; char autoscale;

	if(axe == GRF_YAXIS) scale = graph->axe + 1; else scale = graph->axe;
	if(scale->reel) autoscale = (scale->u.r.min == scale->u.r.max);
	else autoscale = (scale->u.i.min == scale->u.i.max);
	if(autoscale) scale->minauto = scale->maxauto = 1;
	if(scale->minauto || scale->maxauto) {
		if(!GraphGradsExtrema(graph,axe)) return(0);
	}
	if(scale->reel) {
		if(scale->logd) return(GraphGradsFloatLog(scale));
		else return(GraphGradsFloatLineaires(graph,scale,zoom));
	} else {
		if(scale->logd) return(GraphGradsIntLog(scale));
		else return(GraphGradsIntLineaires(graph,scale,zoom));
	}
}
/* .......................................................................... */
static void GraphGradMin(GraphScale *scale, int decalage) {
	float rprod; int64 iprod;

	if(DEBUG_GRAPH(3)) {
		WndPrint("Axe X %s\n",scale->reel?"reel":"entier");
		WndPrint("min: %d, total: %d, echelle: %g\n",scale->u.i.min,scale->utile,scale->ech);
		WndPrint("Graduations User: %g + n x %g\n",scale->g1,scale->grad);
	}
	if(scale->reel) {
		if(scale->pixel == 0.0f) return;
		rprod = (float)decalage / scale->pixel;
		if(!scale->logd) {
			if(scale->grad == 0.0f) return;
			rprod += (scale->u.r.min - scale->g1);
			rprod /= scale->grad;
		}
		scale->gmin = (int)rprod;
	} else {
		if(scale->utile == 0) return;
		iprod = decalage;
		iprod *= (int64)scale->ech;
		iprod /= scale->utile;
		if(!scale->logd) {
			if(scale->grad == 0.0f) return;
			iprod += (scale->u.i.min - (int)scale->g1);
			iprod /= (int64)scale->grad;
		}
		scale->gmin = (int)iprod;
	}
}
/* .......................................................................... */
static void GraphAxisXDraw(Graph graph, int y, WndContextPtr gc, unsigned char typegrad ) {
	Cadre cdr; WndFrame f; GraphScale *xscale;
	float (*fctn)(Graph, int, float);
	int ygrad,lgrad,sgrad,xtext,ytext,nbgrad; int i,k; char doit_fermer;
	int xc,xp,xd,xl,xs,ys,xlim,lngr,utile; float rx;
	char s[80];
/*--	int i2; float r2; int delta; */
	int timeout; char min_fait,grad_faite;
	// float (*fctnTmp)(Graph, int, float) =  (float (*)(Graph, int, float))fctn;

	cdr = graph->cdr;
	xscale = graph->axe;
	f = cdr->f;
	y = cdr->dv - y; /* anciennement cdr->haut - y */

	utile = cdr->dh - xscale->marge_totale;
	WndLine(f,gc,xscale->marge_min,y,utile,0);
	if((graph->mode & GRF_NOGRAD) || (typegrad == GRF_GRADNONE)) return;

	nbgrad = 1;
	if(typegrad == GRF_GRADINF) {
		ygrad = y;
		lgrad = graph->grad;
		sgrad = (2 * lgrad) / 3; if(sgrad < 2) sgrad = 2;
		ys = ygrad;
		ytext = ygrad + lgrad + graph->of7 + WndLigToPix(1);
	} else if(typegrad == GRF_GRADSUP) {
		ygrad = y;
		lgrad = -graph->grad;
		sgrad = (2 * lgrad) / 3; if(sgrad > -2) sgrad = -2;
		ys = ygrad;
		ytext = ygrad + lgrad - graph->of7;
	} else /* if(typegrad == GRF_GRADBOTH) */ {
		ygrad = y - graph->grad;
		lgrad = 2 * graph->grad;
		sgrad = (2 * lgrad) / 3; if(sgrad < 2) sgrad = 2;
		ys = ygrad + ((lgrad - sgrad) / 2);
		ytext = ygrad + lgrad + graph->of7 + WndLigToPix(1);
	}

/*
**  Trace des graduations et des valeurs associees.
*/
	for(k=0; k<2; k++) {
		if(xscale->logd) {
			rx = xscale->g1 * (float)pow(10.0,(double)xscale->gmin);
			if(xscale->grad > 0.0f) xscale->grad = rx;
			nbgrad = 9;
		} else rx = xscale->g1 + ((float)xscale->gmin * xscale->grad);
		if(xscale->reel && (fabs((double)rx) < (xscale->grad / 100.0f))) rx = 0.0f;
		xc = GraphPosFloat(1,rx,xscale) - cdr->dx; xp = -1;
		fctn = (float (*)(Graph, int, float))xscale->chgt_unite;
		xlim = xscale->utile + xscale->marge_min - cdr->dx;
		if(xlim > cdr->dh) xlim = cdr->dh;
		timeout = 0;
		min_fait = grad_faite = 0;
		xtext = 0;
		if(k) { doit_fermer = WndBeginLine(f,WND_STD,GL_LINES); WndAbsLine(f); } else doit_fermer = 0;
		do {
			if(DEBUG_GRAPH(1)) WndPrint("Axe X: graduation a %d pixels\n",xc);
			if(xc >= xscale->marge_min) {
				if(k && !xscale->logd && (f->qualite == WND_Q_PAPIER)) {
					if(xp >= 0) {
						xd = xc - xp;
						for(i = 1; i < 5; i++) {
							xs = xp + ((i * xd) / 5);
							if(xs > xlim) break;
							WndLine(f,gc,xs,ys,0,sgrad);
						}
						if(xs > xlim) break;
					}
					xp = xc;
				}
				if(xc > xlim) break;
				if(k) WndLine(f,gc,xc,ygrad,0,lgrad);
				else if(!xscale->logd || (nbgrad == 9)) {
					if(xscale->reel) {
						if(fabs((double)rx) < (xscale->grad / 100.0f)) rx = 0.0f; // peut-etre inutile
						if(fctn) sprintf(s,xscale->format,fctn(graph,0,rx));
						else sprintf(s,xscale->format,rx);
					} else {
						if(fctn) sprintf(s,xscale->format,fctn(graph,0,rx));
						else sprintf(s,"%d",arrondi(rx));
					}
					// if(DEBUG_GRAPH(1)) WndPrint("Axe X: graduation=%s\n",s);
					lngr = WndColToPix((int)strlen(s)); xl = xc - (lngr / 2);
					if(xl > (xtext + WndColToPix(1))) WndString(f,gc,xl,ytext,s);
					else lngr = WndColToPix(1);
					{
						xtext = xc + (lngr / 2) + 1;
						graph->zone[GRF_ZONE_XMAX].xmin = xc - (lngr/2);
						graph->zone[GRF_ZONE_XMAX].ymin = ytext - WndLigToPix(1);
						graph->zone[GRF_ZONE_XMAX].xmax = xtext;
						graph->zone[GRF_ZONE_XMAX].ymax = ytext;
						if(!min_fait) {
							graph->zone[GRF_ZONE_XMIN].xmin = graph->zone[GRF_ZONE_XMAX].xmin;
							graph->zone[GRF_ZONE_XMIN].ymin = graph->zone[GRF_ZONE_XMAX].ymin;
							graph->zone[GRF_ZONE_XMIN].xmax = graph->zone[GRF_ZONE_XMAX].xmax;
							graph->zone[GRF_ZONE_XMIN].ymax = graph->zone[GRF_ZONE_XMAX].ymax;
							min_fait = 1;
						} else if(!grad_faite) {
							graph->zone[GRF_ZONE_XGRAD].xmin = graph->zone[GRF_ZONE_XMAX].xmin;
							graph->zone[GRF_ZONE_XGRAD].ymin = graph->zone[GRF_ZONE_XMAX].ymin;
							graph->zone[GRF_ZONE_XGRAD].xmax = graph->zone[GRF_ZONE_XMAX].xmax;
							graph->zone[GRF_ZONE_XGRAD].ymax = graph->zone[GRF_ZONE_XMAX].ymax;
							grad_faite = 1;
						}
					}
				}
			}
			if(xscale->logd) {
				if(xscale->grad < 0.0f) rx *= 10.0f;
				else {
					rx += xscale->grad;
					if(--nbgrad == 0) { xscale->grad *= 10.0f; nbgrad = 9; }
				}
			} else rx += xscale->grad;
			xc = GraphPosFloat(1,rx,xscale) - cdr->dx;
			if(++timeout > 100) break;
		} while(1); // (xc <= xlim) /* || !xscale->logd */);
		if(doit_fermer) WndEndLine(f);
	}

#ifdef A_REVOIR
/*
** Trace de la valeur de l'origine de l'axe des X.
*/
	if (x == xscale->marge_min) {
		if(xscale->reel) sprintf(s,xscale->format,xscale->u.r.min);
		else sprintf(s,"%d",xscale->u.i.min);
		lngr = WndColToPix(strlen(s));
		WndString(f,gc,x - (lngr/2) - cdr->dx,y3,s);
	} else if (x == xscale->marge_min+xscale->utile) {
		if(xscale->reel) sprintf(s,xscale->format,xscale->u.r.max);
		else sprintf(s,"%d",xscale->u.i.max);
		lngr = WndColToPix(strlen(s));
		WndString(f,gc,x - (lngr/2) - cdr->dx,y3,s);
	}
#endif
}
/* .......................................................................... */
static void GraphAxisYDraw(Graph graph, int x, WndContextPtr gc, unsigned char typegrad) {
	Cadre cdr; WndFrame f; GraphScale *xscale,*yscale;
	float (*fctn)(Graph, int, float);
	int xgrad,lgrad,sgrad,xtext,ytext,lngr,utile; int i,k; char doit_fermer;
	int xs,yc,yp,ys,yd,dy,ylim,decal,nbgrad;
	float ry;
	char s[80];
/*--	int i2; float r2; int delta; */
	int timeout; char min_fait,grad_faite;

	cdr = graph->cdr;
	xscale = graph->axe;
	yscale = graph->axe + 1;
	f = cdr->f;

	utile = cdr->dv - yscale->marge_totale;
	WndLine(f,gc,x,yscale->marge_max,0,utile); // y croissants vers le bas!!
	if((graph->mode & GRF_NOGRAD) || (typegrad == GRF_GRADNONE)) return;

	/* nbgrad = 1;
	if(typegrad == GRF_GRADINF) {
		xgrad = x;
		lgrad = -graph->grad;
		xtext = xgrad + lgrad - graph->of7;
	} else if(typegrad == GRF_GRADSUP) {
		xgrad = x;
		lgrad = graph->grad;
		xtext = xgrad + lgrad + xscale->cols + graph->of7;
	} else {
		xgrad = x - graph->grad;
		lgrad = 2 * graph->grad;
		xtext = xgrad - graph->of7;
	} */
	nbgrad = 1;
	if(typegrad == GRF_GRADINF) {
		xgrad = x;
		lgrad = -graph->grad;
		sgrad = (2 * lgrad) / 3; if(sgrad > -2) sgrad = -2;
		xs = xgrad;
		xtext = xgrad + lgrad - graph->of7;
	} else if(typegrad == GRF_GRADSUP) {
		xgrad = x;
		lgrad = graph->grad;
		sgrad = (2 * lgrad) / 3; if(sgrad < 2) sgrad = 2;
		xs = xgrad;
		xtext = xgrad + lgrad + xscale->cols + graph->of7;
	} else /* if(typegrad == GRF_GRADBOTH) */ {
		xgrad = x - graph->grad;
		lgrad = 2 * graph->grad;
		sgrad = (2 * lgrad) / 3; if(sgrad < 2) sgrad = 2;
		xs = xgrad + ((lgrad - sgrad) / 2);
		xtext = xgrad - graph->of7;
	}

/*--	xgrad -= cdr->dx; */
/*--	xtext -= cdr->dx; */
/*
**  Trace des graduations et des valeurs associees.
*/
	for(k=0; k<2; k++) {
		if(yscale->logd) {
			ry = yscale->g1 * (float)pow(10.0,(double)xscale->gmin);
			if(yscale->grad > 0.0f) yscale->grad = ry;
			nbgrad = 9;
		} else ry = yscale->g1 + ((float)yscale->gmin * yscale->grad);
		//??? if(yscale->reel && (fabs((double)ry) < (yscale->grad / 100.0f))) ry = 0.01f;
		yc = GraphPosFloat(1,ry,yscale);
		yc = cdr->haut - yc - cdr->dy;
		yp = -999;
		fctn = (float (*)(Graph, int, float))yscale->chgt_unite;
		ylim = yscale->marge_max - cdr->dy;
		if(ylim < 0) ylim = 0;
		decal = WndLigToPix(1) / 2;
		timeout = 0;
		min_fait = grad_faite = 0; ytext = 0;
		if(k) { doit_fermer = WndBeginLine(f,WND_STD,GL_LINES); WndAbsLine(f); } else doit_fermer = 0;
		do {
			if(DEBUG_GRAPH(1)) WndPrint("Axe Y: graduation a %d pixels\n",yc);
			if(yc <= (cdr->haut - yscale->marge_max)) {
				if(k && !yscale->logd && (f->qualite == WND_Q_PAPIER)) {
					if(yp >= 0) {
						yd = yc - yp;
						for(i = 1; i < 5; i++) {
							ys = yp + ((i * yd) / 5);
							if(ys < ylim) break;
							WndLine(f,gc,xs,ys,sgrad,0);
						}
						if(ys < ylim) break;
					}
					yp = yc;
				}
				if(yc < ylim) break;
				dy = (yc > ytext)? yc - ytext: ytext - yc;
				if(k) WndLine(f,gc,xgrad,yc,lgrad,0);
				else if((!yscale->logd || (nbgrad == 9)) && (dy > WndLigToPix(1))) {
					// float (*fctnTmp)(Graph, int, float) =  (float (*)(Graph, int, float))fctn;
					if(yscale->reel) {
						if(fabs((double)ry) < (yscale->grad / 100.0f)) ry = 0.0f; // peut-etre inutile
						if(fctn) sprintf(s,yscale->format,fctn(graph,0,ry));
						else sprintf(s,yscale->format,ry);
					} else {
						if(fctn)sprintf(s,yscale->format,fctn(graph,0,ry));
						else sprintf(s,"%d",arrondi(ry));
					}
					if(DEBUG_GRAPH(1)) WndPrint("Axe Y: graduation=%s\n",s);
					lngr = WndColToPix((int)strlen(s));
					WndString(f,gc,xtext - lngr,yc + decal,s);
					ytext = yc;
					graph->zone[GRF_ZONE_YMAX].xmin = xtext - lngr;
					graph->zone[GRF_ZONE_YMAX].ymin = yc + decal - WndLigToPix(1);
					graph->zone[GRF_ZONE_YMAX].xmax = xtext;
					graph->zone[GRF_ZONE_YMAX].ymax = yc + decal;
					if(!min_fait) {
						graph->zone[GRF_ZONE_YMIN].xmin = graph->zone[GRF_ZONE_YMAX].xmin;
						graph->zone[GRF_ZONE_YMIN].ymin = graph->zone[GRF_ZONE_YMAX].ymin;
						graph->zone[GRF_ZONE_YMIN].xmax = graph->zone[GRF_ZONE_YMAX].xmax;
						graph->zone[GRF_ZONE_YMIN].ymax = graph->zone[GRF_ZONE_YMAX].ymax;
						min_fait = 1;
					} else if(!grad_faite) {
						graph->zone[GRF_ZONE_YGRAD].xmin = graph->zone[GRF_ZONE_YMAX].xmin;
						graph->zone[GRF_ZONE_YGRAD].ymin = graph->zone[GRF_ZONE_YMAX].ymin;
						graph->zone[GRF_ZONE_YGRAD].xmax = graph->zone[GRF_ZONE_YMAX].xmax;
						graph->zone[GRF_ZONE_YGRAD].ymax = graph->zone[GRF_ZONE_YMAX].ymax;
						grad_faite = 1;
					}
				}
			}
			/*-- ry += yscale->grad; yc -= delta; */
			if(yscale->logd) {
				if(yscale->grad < 0.0f) ry *= 10.0f;
				else {
					ry += yscale->grad;
					if(--nbgrad == 0) { yscale->grad *= 10.0f; nbgrad = 9; }
				}
			} else ry += yscale->grad;
			yc = GraphPosFloat(1,ry,yscale);
			yc = cdr->haut - yc - cdr->dy;
			if(++timeout > 100) break;
		} while(1); // yc >= ylim);
		if(doit_fermer) WndEndLine(f);
	}

#ifdef A_REVOIR
/*
** Trace de la valeur de l'origine de l'axe des Y.
*/
	if(y == yscale->marge_min) {
		if(yscale->reel) sprintf(s,yscale->format,yscale->u.r.max);
		else sprintf(s,"%d",yscale->u.i.max);
		lngr = WndColToPix(strlen(s));
		if(xstr < 0) x3 = x1 + dx - lngr - 2;
		else x3 = x1 + dx + 2;
		WndString(f,gc,x3,y + (WndLigToPix(1)/2) - cdr->dy,s);
	} else if (y == yscale->marge_min+yscale->utile) {
		if(yscale->reel) sprintf(s,yscale->format,yscale->u.r.min);
		else sprintf(s,"%d",yscale->u.i.min);
		lngr = WndColToPix(strlen(s));
		if(xstr < 0) x3 = x1 + dx - lngr - 2;
		else x3 = x1 + dx + 2;
		WndString(f,gc,x3,y + (WndLigToPix(1)/2) - cdr->dy,s);
	}
#endif
}
/* .......................................................................... */
static void GraphMarkOrigin(int type, int x, int y) {
	OpiumMark[type][0].x = x; OpiumMark[type][0].y =  y;
}
/* .......................................................................... */
static inline void GraphMarkDraw(WndFrame f, WndContextPtr gc, int type, int nb) {
	int i; int x,y,dx,dy;

	x = OpiumMark[type][0].x;
	y = OpiumMark[type][0].y;
	for(i=1; i<nb; i++) {
		dx = OpiumMark[type][i].x;
		dy = OpiumMark[type][i].y;
		WndLine(f,gc,x,y,dx,dy);
		if(++i >= nb) return;
		x += dx + OpiumMark[type][i].x;
		y += dy + OpiumMark[type][i].y;
	}
}
/* .......................................................................... */
static inline int GraphPosFloat(int mode, float r, GraphScale *scale) {
/* Converti la valeur d'un point float quelconque en un nombre de pixels
   mode = 0: taille ecran / 1: position sur l'ecran */
	if(mode == 0) {
		if(scale->logd) {
			if(r > 0.0f) return((int)((float)log10((double)r) * scale->pixel)); else return(-1);
		} else return((int)(r * scale->pixel));
	} else {
		if(scale->logd) {
			if(r > 0.0f) return((int)((float)(log10((double)r) - scale->logmin) * scale->pixel) + scale->marge_min);
			else return(-1 /*scale->marge_min*/);
		} else {
			if(scale->reel)
				return((int)((r - scale->u.r.min) * scale->pixel) + scale->marge_min);
			else return((int)((r - (float)scale->u.i.min) * scale->pixel) + scale->marge_min);
		}
	}
}
/* .......................................................................... */
static inline int GraphPosShort(int mode, short i, GraphScale *scale) {
/* Converti la valeur d'un point short quelconque en un nombre de pixels
   mode = 0: taille ecran / 1: position sur l'ecran */
	int64 prod; int pos;

	if(mode == 0) {
		if(scale->logd) {
			if(i > 0) {
				prod = (int64)(log10((double)i) * (double)scale->utile);
			} else prod = -(0x7FFFFFFF);
		} else {
			prod = i * scale->utile;
		}
		pos = (int)((float)prod / scale->ech);
		return(pos);
	} else {
		if(scale->logd) {
			if(i > 0) {
				prod = (int64)((log10((double)i) - scale->logmin) * (double)scale->utile);
			} else prod = -(0x7FFFFFFF);
		} else {
			if(scale->reel) prod = (i - (int)scale->u.r.min);
			else prod = (i - scale->u.i.min);
			prod *= scale->utile;
		}
		pos = (int)((float)prod / scale->ech);
		return(pos + scale->marge_min);
	}
}
/* .......................................................................... */
static inline int GraphPosInt(int mode, int i, GraphScale *scale) {
/* Converti la valeur d'un point int quelconque en un nombre de pixels
   mode = 0: taille ecran / 1: position sur l'ecran */
	int64 prod; int pos;

	if(mode == 0) {
		if(scale->logd) {
			if(i > 0)
				prod = (int64)((log10((double)i)) * (double)scale->utile);
			else prod = -(0x7FFFFFFF);
		} else {
			prod = i * scale->utile;
		}
		pos = (int)((float)prod / scale->ech);
		return(pos);
	} else {
		if(scale->logd) {
			if(i > 0)
				prod = (int64)((log10((double)i) - scale->logmin) * (double)scale->utile);
			else prod = -(0x7FFFFFFF);
		} else {
			if(scale->reel) prod = (i - (int)scale->u.r.min);
			else prod = (i - scale->u.i.min);
			prod *= scale->utile;
		}
		pos = (int)((float)prod / scale->ech);
		return(pos + scale->marge_min);
	}
}
/* .......................................................................... */
static inline char GraphUserVal(GraphData data, int k, int type, int *i, float *r) {
	int *itab; short *stab; float *rtab; void (*fctn)(int, float*, int*);
	short s;

	if(data->fctn == (void *)-1) return(0);
	if((data->parm & GRF_ADRS) == GRF_FCTN) {
		if(data->fctn == 0) return(0);
		fctn = (void (*)(int, float*, int*))data->fctn;
		(*fctn)(k,r,i);
	} else if(type == GRF_FLOAT) {
		rtab = (float *)(data->fctn);
		if((data->parm & GRF_ADRS) == GRF_INDEX) {
			if(rtab == 0) {
				if(k >= data->min) *r = (float)k;
				else *r = (float)(k + data->dim);
			} else {
				if(k >= data->min) *r = *rtab + (k * *(rtab+1));
				else *r = *rtab + ((k + data->dim) * *(rtab+1));
			}
		} else if(data->fctn == 0) return(0);
		else *r = rtab[k];
	} else if(type == GRF_SHORT) {
		stab = (short *)data->fctn;
		if((data->parm & GRF_ADRS) == GRF_INDEX) {
			if(stab == 0) {
				if(k >= data->min) s = (short)k;
				else s = (short)(k + data->dim);
			} else {
				if(k >= data->min) s = *stab + ((short)k * *(stab+1));
				else s = *stab + ((short)(k + data->dim) * *(stab+1));
			}
		} else if(data->fctn == 0) return(0);
		else s = stab[k];
		*i = (int)s;
	} else {
		itab = (int *)data->fctn;
		if((data->parm & GRF_ADRS) == GRF_INDEX) {
			if(itab == 0) {
				if(k >= data->min) *i = k;
				else *i = k + data->dim;
			} else {
				if(k >= data->min) *i = *itab + (k * *(itab+1));
				else *i = *itab + ((k + data->dim) * *(itab+1));
			}
		} else if(data->fctn == 0) return(0);
		else *i = itab[k];
	}

	return(1);
}
/* .......................................................................... */
static inline int GraphPosData(int mode, GraphData data, int k, GraphScale *scale) {
	int type; int i; float r;

	if((scale->num & 0xFFFF) == GRF_DIRECT) {
		int d,z;
		if((d = (scale->u.i.max - scale->u.i.min)) > 0) z = scale->lngr / d; else z = 1;
		return((z * k) + scale->u.i.min);
	}
	type = data->parm & GRF_TYPE;
	if(GraphUserVal(data,k,type,&i,&r)) switch(type) {
		case GRF_SHORT: return(GraphPosShort(mode,(short)(i + data->of7.i.v),scale));
		case GRF_INT: return(GraphPosInt(mode,i + data->of7.i.v,scale));
		case GRF_FLOAT: return(GraphPosFloat(mode,r + data->of7.r.v,scale));
	}
	return(0);
}
#ifdef ANCIEN
/* .......................................................................... */
static inline int GraphPosData(int mode, GraphData data, int k, GraphScale *scale, int *usr_i, float *usr_r) {
/* Converti la valeur du point #k d'un tableau en un nombre de pixels
   mode = 0: taille ecran / 1: position sur l'ecran */
	int *itab; int i;
	short *stab; short s;
	float *rtab; float r;
	void (*fctn)(int, float*, int*);
	int type;

	if((scale->num & 0xFFFF) == GRF_DIRECT) {
		int d,z;
		if((d = (scale->u.i.max - scale->u.i.min)) > 0) z = scale->lngr / d; else z = 1;
		return((z * k) + scale->u.i.min);
	}
	if(data->fctn == (void *)-1) return(0);
	type = (data->parm & GRF_TYPE);
	if((data->parm & GRF_ADRS) == GRF_FCTN) {
		if(data->fctn == 0) return(0);
		fctn = (void (*)(int, float*, int*))data->fctn;
		(*fctn)(k,usr_r,usr_i);
		if(type == GRF_FLOAT) return(GraphPosFloat(mode,*usr_r + data->of7.r.v,scale));
		else return(GraphPosInt(mode,*usr_i + data->of7.i.v,scale));
	} else if(type == GRF_FLOAT) {
		rtab = (float *)(data->fctn);
		if((data->parm & GRF_ADRS) == GRF_INDEX) {
			if(rtab == 0) {
				if(k >= data->min) r = (float)k; // return(GraphPosFloat(mode,(float)k,scale));
				else r = (float)(k + data->dim); // return(GraphPosFloat(mode,(float)(k + data->dim),scale));
			} else {
				if(k >= data->min) r = *rtab + (k * *(rtab+1)); // return(GraphPosFloat(mode,*rtab + (k * *(rtab+1)),scale));
				else r = *rtab + ((k + data->dim) * *(rtab+1)); // return(GraphPosFloat(mode,*rtab + ((k + data->dim) * *(rtab+1)),scale));
			}
		} else if(data->fctn == 0) return(0);
		else r = rtab[k]; // return(GraphPosFloat(mode,rtab[k] + data->of7.r.v,scale));
		*usr_r = r;
		return(GraphPosFloat(mode,r + data->of7.r.v,scale));
	} else if(type == GRF_SHORT) {
		stab = (short *)data->fctn;
		if((data->parm & GRF_ADRS) == GRF_INDEX) {
			if(stab == 0) {
				if(k >= data->min) s = (short)k; // return(GraphPosShort(mode,(short)k,scale));
				else s = (short)(k + data->dim); // return(GraphPosShort(mode,(short)(k + data->dim),scale));
			} else {
				if(k >= data->min) s = *stab + (k * *(stab+1)); // return(GraphPosShort(mode,*stab + (k * *(stab+1)),scale));
				else s = *stab + ((k + data->dim) * *(stab+1)); // return(GraphPosShort(mode,*stab + ((k + data->dim) * *(stab+1)),scale));
			}
		} else if(data->fctn == 0) return(0);
		else s = stab[k]; // return(GraphPosShort(mode,stab[k] + data->of7.i.v,scale));
		*usr_i = (int)s;
		return(GraphPosShort(mode,s + (short)data->of7.i.v,scale));
	} else {
		itab = (int *)data->fctn;
		if((data->parm & GRF_ADRS) == GRF_INDEX) {
			if(itab == 0) {
				if(k >= data->min) i = k; // return(GraphPosInt(mode,k,scale));
				else i = k + data->dim; // return(GraphPosInt(mode,k + data->dim,scale));
			} else {
				if(k >= data->min) i = *itab + (k * *(itab+1)); // return(GraphPosInt(mode,*itab + (k * *(itab+1)),scale));
				else i = *itab + ((k + data->dim) * *(itab+1)); // return(GraphPosInt(mode,*itab + ((k + data->dim) * *(itab+1)),scale));
			}
		} else if(data->fctn == 0) return(0);
		else i = itab[k]; // return(GraphPosInt(mode,itab[k],scale));
		*usr_i = i;
		return(GraphPosInt(mode,i + data->of7.i.v,scale));
	}
}
#endif
/* ========================================================================== */
static void GraphZoneRefresh(Graph g, int num) {
	char autoscale;

	switch(num) {
	  case GRF_ZONE_XMIN:
		if(g->axe[0].reel) autoscale = (g->axe[0].u.r.min == g->axe[0].u.r.max);
		else autoscale = (g->axe[0].u.i.min == g->axe[0].u.i.max);
		if(autoscale) g->axe[0].minauto = g->axe[0].maxauto = 1;
		else g->axe[0].minauto = 0;
		g->axe[0].pret = 0;
		break;
	  case GRF_ZONE_XMAX:
		if(g->axe[0].reel) autoscale = (g->axe[0].u.r.min == g->axe[0].u.r.max);
		else autoscale = (g->axe[0].u.i.min == g->axe[0].u.i.max);
		if(autoscale) g->axe[0].minauto = g->axe[0].maxauto = 1;
		else g->axe[0].maxauto = 0;
		g->axe[0].pret = 0;
		break;
	  case GRF_ZONE_YMIN:
		if(g->axe[1].reel) autoscale = (g->axe[1].u.r.min == g->axe[1].u.r.max);
		else autoscale = (g->axe[1].u.i.min == g->axe[1].u.i.max);
		if(autoscale) g->axe[1].minauto = g->axe[1].maxauto = 1;
		else g->axe[1].minauto = 0;
		g->axe[1].pret = 0;
		break;
	  case GRF_ZONE_YMAX:
		if(g->axe[1].reel) autoscale = (g->axe[1].u.r.min == g->axe[1].u.r.max);
		else autoscale = (g->axe[1].u.i.min == g->axe[1].u.i.max);
		if(autoscale) g->axe[1].minauto = g->axe[1].maxauto = 1;
		else g->axe[1].maxauto = 0;
		g->axe[1].pret = 0;
		break;
	  case GRF_ZONE_XGRAD:
		if(g->axe[0].logd) switch(GraphZoneLog) {
		  case 0: g->axe[0].grad = -1.0f; break;
		  case 1: g->axe[0].grad =  0.0f; break;
		  case 2: g->axe[0].grad =  1.0f; break;
		}
		g->axe[0].gradauto = 0;
		g->axe[0].pret = 0;
		break;
	  case GRF_ZONE_YGRAD:
		if(g->axe[1].logd) switch(GraphZoneLog) {
		  case 0: g->axe[1].grad = -1.0f; break;
		  case 1: g->axe[1].grad =  0.0f; break;
		  case 2: g->axe[1].grad =  1.0f; break;
		}
		g->axe[1].gradauto = 0;
		g->axe[1].pret = 0;
		break;
	}
	OpiumRefresh(g->cdr);
}
/* .......................................................................... */
static int GraphZoneChange(Panel p, void *arg) {
	Graph g; int num;

//	g = (Graph)PanelApplyArg(p);
	g = (Graph)arg;
	num = GraphZoneEnCours;
	GraphZoneRefresh(g,num);
	return(1);
}
/* .......................................................................... */
void GraphZoneAcces(Graph g, int num) {
	int x,y; Panel p; WndFrame f;

	GraphZoneEnCours = num;
	f = (g->cdr)->f;
	x = f->x + g->zone[num].xmin; y = f->y + g->zone[num].ymin;
	p = PanelCreate(1);
	OpiumPosition(p->cdr,x-WndColToPix(2),y);
	PanelOnApply(p,GraphZoneChange,(void *)g);
	switch(num) {
	  case GRF_ZONE_XMIN:
		if(g->axe[0].reel) PanelFloat(p,0,&(g->axe[0].u.r.min));
		else PanelInt(p,0,&(g->axe[0].u.i.min));
		break;
	  case GRF_ZONE_XMAX:
		if(g->axe[0].reel) PanelFloat(p,0,&(g->axe[0].u.r.max));
		else PanelInt(p,0,&(g->axe[0].u.i.max));
		break;
	  case GRF_ZONE_YMIN:
		if(g->axe[1].reel) PanelFloat(p,0,&(g->axe[1].u.r.min));
		else PanelInt(p,0,&(g->axe[1].u.i.min));
		break;
	  case GRF_ZONE_YMAX:
		if(g->axe[1].reel) PanelFloat(p,0,&(g->axe[1].u.r.max));
		else PanelInt(p,0,&(g->axe[1].u.i.max));
		break;
	  case GRF_ZONE_XGRAD:
	  	if(g->axe[0].logd) {
	  		GraphZoneLog = (g->axe[0].grad > 0.0f)? 2: ((g->axe[0].grad < 0.0f)? 0: 1);
	  		PanelKeyB(p,0,"simple/neant/dixiemes",&GraphZoneLog,10);
		} else PanelFloat(p,0,&(g->axe[0].grad));
		break;
	  case GRF_ZONE_YGRAD:
		if(g->axe[1].logd) {
	  		GraphZoneLog = (g->axe[1].grad > 0.0f)? 2: ((g->axe[1].grad < 0.0f)? 0: 1);
	  		PanelKeyB(p,0,"simple/neant/dixiemes",&GraphZoneLog,10);
		} else PanelFloat(p,0,&(g->axe[1].grad));
		break;
	}
	if(OpiumSub(p->cdr) == PNL_OK) GraphZoneRefresh(g,num);
	PanelDelete(p);
}
/* ========================================================================== */
static Cadre GraphDialogCadre;
static Graph GraphDialogGraph;

static Panel GraphPgeneral;
static char GraphPaneltitre[OPIUM_MAXTITRE];
static char GraphPanelnb_axes,GraphPanelgrille,GraphPanelgrads,GraphPanellegende,GraphPanelqualite;

static Panel GraphPaxe;
static int  GraphPaneltableau;
static int  GraphPanelzoom;
static char GraphPanelmin_auto,GraphPanelmax_auto,GraphPanelgrad_auto,GraphPanellog;
static char GraphPanelfmt[12];

static Panel GraphPmanu;
static float GraphPanelRmin,GraphPanelRmax,GraphPanelRgrad;
static int   GraphPanelImin,GraphPanelImax,GraphPanelIgrad;

static Panel GraphPnumdata;
static Panel GraphPtableau;
static char GraphPanelforme,GraphPanelDetails;
static unsigned char GraphPaneltype,GraphPanelepais;
static char  GraphPanelNom[GRF_MAX_TITRE];
static char  GraphPanelcouleur[WND_MAXCOULEUR];

/* ========================================================================== */
static int GraphDialogGeneral(void) {
	Graph graph; WndFrame f;
	unsigned short mode; char a_refaire;

	graph = GraphDialogGraph; a_refaire = 0;

	strncpy(GraphPaneltitre,GraphDialogCadre->titre,OPIUM_MAXTITRE);
	mode = graph->mode;
	GraphPanelnb_axes  = graph->mode & GRF_NBAXES;
	GraphPanelgrille  = (graph->mode & GRF_GRID)    >> 2;
	GraphPanelgrads = ~((graph->mode & GRF_NOGRAD)  >> 3);
	GraphPanellegende = (graph->mode & GRF_LEGEND)  >> 4;
	GraphPanelqualite = (graph->mode & GRF_QUALITE) >> 5;

	f = (GraphMdial->cdr)->f;
	if(!f) f = (graph->cdr)->f;
	if(f) OpiumPosition(GraphPgeneral->cdr,f->x+f->h,f->y);
	if(OpiumExec(GraphPgeneral->cdr) == PNL_CANCEL) return(0);

	strncpy(GraphDialogCadre->titre,GraphPaneltitre,OPIUM_MAXTITRE);
	graph->mode = (unsigned short)(
	               GraphPanelnb_axes
 	           |  (GraphPanelgrille  << 2)
	           | ((~GraphPanelgrads  << 3) & 0x08)
	           |  (GraphPanellegende << 4)
	           |  (GraphPanelqualite << 5));
	if(graph->mode != mode) a_refaire = 1;

	f = (graph->cdr)->f;
	if(f && (f->qualite != GraphPanelqualite)) {
		f->qualite = GraphPanelqualite;
		a_refaire = 1;
	}

	if(a_refaire) { graph->pret = 0; OpiumRefreshGraph(graph->cdr); }

	return(0);
}
/* ========================================================================== */
static int GraphDialogAxe(Menu menu, MenuItem item) {
	Graph graph; GraphScale *scale; WndFrame f;
	int num; char ok;

	graph = GraphDialogGraph;

 //	num = MenuItemGetIndex(menu,item->texte) - 1;
	num = MenuItemNum(menu,item) - 3;
	if((num < 0) || (num > 1)) {
		OpiumError("ERREUR SYSTEME dans GraphDialogAxe");
		return(0);
	}
	scale = graph->axe + num;

	GraphPaneltableau = scale->num;
	GraphPanelzoom = scale->zoom;
	GraphPanelmin_auto = scale->minauto;
	GraphPanelmax_auto = scale->maxauto;
	GraphPanelgrad_auto = scale->gradauto;
	GraphPanellog = scale->logd;
	strncpy(GraphPanelfmt,scale->format,12);
	strncpy(GraphPaneltitre,scale->titre,80);

	f = (GraphMdial->cdr)->f;
	if(!f) f = (graph->cdr)->f;
	if(f) OpiumPosition(GraphPaxe->cdr,f->x+f->h,f->y);
	OpiumTitle(GraphPaxe->cdr,item->texte);
	ok = 0;
	while(!ok) {
		if(OpiumExec(GraphPaxe->cdr) == PNL_CANCEL) return(0);

		if(!GraphPanelmin_auto || !GraphPanelmax_auto || !GraphPanelgrad_auto) {
			PanelErase(GraphPmanu);
			if(!GraphPanelmin_auto) {
				if(scale->reel) {
					GraphPanelRmin = scale->u.r.min;
					PanelFloat(GraphPmanu,"Minimum",&GraphPanelRmin);
				} else {
					GraphPanelImin = scale->u.i.min;
					PanelInt(GraphPmanu,"Minimum",&GraphPanelImin);
				}
			}
			if(!GraphPanelmax_auto) {
				if(scale->reel) {
					GraphPanelRmax = scale->u.r.max;
					PanelFloat(GraphPmanu,"Maximum",&GraphPanelRmax);
				} else {
					GraphPanelImax = scale->u.i.max;
					PanelInt(GraphPmanu,"Maximum",&GraphPanelImax);
				}
			}
			if(!GraphPanelgrad_auto) {
				if(scale->reel) {
					GraphPanelRgrad = scale->grad;
					PanelFloat(GraphPmanu,"Graduation",&GraphPanelRgrad);
				} else {
					GraphPanelIgrad = (int)scale->grad;
					PanelInt(GraphPmanu,"Graduation",&GraphPanelIgrad);
				}
			}

			if(f) OpiumPosition(GraphPmanu->cdr,f->x+f->h,f->y);
			if(OpiumExec(GraphPmanu->cdr) == PNL_CANCEL) continue;

			if(!GraphPanelmin_auto) {
				if(scale->reel) scale->u.r.min = GraphPanelRmin;
				else scale->u.i.min = GraphPanelImin;
			}
			if(!GraphPanelmax_auto) {
				if(scale->reel) scale->u.r.max = GraphPanelRmax;
				else scale->u.i.max = GraphPanelImax;
			}
			if(!GraphPanelgrad_auto) {
				if(scale->reel) scale->grad = GraphPanelRgrad;
				else scale->grad = (float)GraphPanelIgrad;
			}
		}
		ok = 1;
	}

	scale->num = (short)GraphPaneltableau;
	if((scale->zoom != GraphPanelzoom) && (GraphPanelzoom >= 1)) {
//-		if(GraphPanelzoom >= scale->lngr) GraphPanelzoom = scale->lngr / 2;
//-		scale->lngr = scale->lngr / scale->zoom;
//-		scale->zoom = GraphPanelzoom;
//-		scale->lngr = scale->lngr * scale->zoom;
//-		OpiumSizeGraph(graph->cdr,0);
//-		OpiumRefreshGraph(graph->cdr);
		if(GraphScaleZoom(scale,GraphPanelzoom)) OpiumSizeGraph(graph->cdr,0);
		else GraphPanelzoom = scale->zoom;
	}
	scale->minauto = GraphPanelmin_auto;
	scale->maxauto = GraphPanelmax_auto;
	scale->gradauto = GraphPanelgrad_auto;
	scale->logd = GraphPanellog;
	strncpy(scale->format,GraphPanelfmt,12);
	if(strcmp(scale->titre,GraphPaneltitre)) {
		strncpy(scale->titre,GraphPaneltitre,80);
		graph->axe[1].pret = 0;
	}

	scale->pret = 0;
	OpiumRefreshGraph(graph->cdr);
	return(0);
}
/* ========================================================================== */
static int GraphRefresh(Instrum instrum) {
	void *arg;

	arg = InstrumChangeArg(instrum);
	OpiumRefresh((Cadre)arg);
	return(0);
}
/* ========================================================================== */
static void GraphDialogPlancheZ(GraphData data) {
	int k,nb;
	char fctn,reel,court;
	float *rtab; short *stab; int *itab; TypeFctn fc;
	float rval,rmin,rmax; int ival,imin,imax;
	Cadre planche; Instrum inst1,inst2;

	rtab = (float *)0;          /* pour gcc */
	stab = (short *)0;          /* pour gcc */
	itab = (int *)0;            /* pour gcc */
	fc = 0;                     /* pour gcc */
	rmin = rmax = 0.0f; imin = imax = 0;   /* pour gcc */
	nb = data->max - data->min;
	court = (data->parm & GRF_TYPE) == GRF_SHORT;
	reel = (data->parm & GRF_TYPE) == GRF_FLOAT;
	fctn = (data->parm & GRF_ADRS) == GRF_FCTN;
	if(fctn) fc = (TypeFctn)data->fctn;
	else if(reel) rtab = (float *)data->fctn;
	else if(court) stab = (short *)data->fctn;
	else itab = (int *)data->fctn;
	for(k=0; k<nb; k++) {
		void (*fctnTmp)(int, float*, int*) = (void (*)(int, float*, int*))fc;
		if(fctn) (*fctnTmp)(k,&rval,&ival);
		else if(reel) rval = rtab[k];
		else if(court) ival = stab[k];
		else ival = itab[k];
		if(reel) {
			if(k == 0) rmin = rmax = rval;
			else if(rval < rmin) rmin = rval;
			else if(rval > rmax) rmax = rval;
		} else {
			if(k == 0) imin = imax = ival;
			else if(ival < imin) imin = ival;
			else if(ival > imax) imax = ival;
		}
	}
	planche = BoardCreate();
	InstrumRectDim(&GraphGlissiere,8*WndLigToPix(1),2*WndColToPix(1));  /* 5 lignes = 4 interlignes */
	if(reel) {
		rmin /= 2.0f; rmax *= 2.0f;
		inst1 = InstrumFloat(INS_GLISSIERE,(void *)&GraphGlissiere,&(data->gamma.r.xmin),rmin,rmax);
		InstrumGradLog(inst1,1); InstrumTitle(inst1,"min");
		InstrumOnChange(inst1,(TypeFctn)GraphRefresh,GraphDialogCadre);
		BoardAddInstrum(planche,inst1,0,0,0);
		inst2 = InstrumFloat(INS_GLISSIERE,(void *)&GraphGlissiere,&(data->gamma.r.xmax),rmin,rmax);
		InstrumGradLog(inst2,1); InstrumTitle(inst2,"max");
		InstrumOnChange(inst2,(TypeFctn)GraphRefresh,GraphDialogCadre);
		BoardAddInstrum(planche,inst2,OPIUM_A_DROITE_DE inst1->cdr);
	} else {
		imin /= 2; imax *= 2;
		inst1 = InstrumInt(INS_GLISSIERE,(void *)&GraphGlissiere,&(data->gamma.i.xmin),imin,imax);
		InstrumGradLog(inst1,1); InstrumTitle(inst1,"min");
		InstrumOnChange(inst1,(TypeFctn)GraphRefresh,GraphDialogCadre);
		BoardAddInstrum(planche,inst1,0,0,0);
		inst2 = InstrumInt(INS_GLISSIERE,(void *)&GraphGlissiere,&(data->gamma.i.xmax),imin,imax);
		InstrumGradLog(inst2,1); InstrumTitle(inst2,"max");
		InstrumOnChange(inst2,(TypeFctn)GraphRefresh,GraphDialogCadre);
		BoardAddInstrum(planche,inst2,OPIUM_A_DROITE_DE inst1->cdr);
	}
	OpiumExec(planche);
	BoardDelete(planche);
	InstrumDelete(inst1);
	InstrumDelete(inst2);

	return;
}
/* ========================================================================== */
static int GraphDialogDonnee(void) {
	int index,qual,rc;
	char *nom[OPIUM_MAXCDR];
	Graph graph; GraphData data; WndFrame f;

	graph = GraphDialogGraph;
	f = (GraphMdial->cdr)->f;
	GraphPaneltableau = graph->axe[1].num;
//	OpiumPosition(GraphPnumdata->cdr,f->x+f->h,f->y);
//	if(OpiumExec(GraphPnumdata->cdr) == PNL_CANCEL) return(0);
//	if((GraphPaneltableau < 0) || (GraphPaneltableau >= graph->dim)) {
//		OpiumError("Valeur incorrecte! choisir dans [0..%d[",
//			graph->dim);
//		return(0);
//	}
	for(index=0; index<graph->dim; index++) {
		data = graph->data_ptr[index];
		if(data) { if(data->titre[0]) { nom[index] = data->titre; continue; } }
		nom[index] = malloc(80);
		if(data) {
			/* if(data->titre[0]) snprintf(nom[index],31,"%s (donnee #%d)",data->titre,index);
			else */ snprintf(nom[index],79,"donnee #%d",index);
		} else strcpy(nom[index],"(neant)");
	}
	nom[index] = "\0";
	if(!f) f = (graph->cdr)->f;
	if(f) {
		OpiumPosition(OpiumPanelQuick->cdr,f->x+f->h,f->y);
		qual = f->qualite;
	} else qual = WND_Q_ECRAN;
	rc = OpiumReadList("Donnee a retracer",nom,&GraphPaneltableau,20); // OPIUM_MAXNOM
	for(index=0; index<graph->dim; index++) {
		data = graph->data_ptr[index];
		if(data) { if(data->titre[0]) continue; }
		free(nom[index]);
	}
	if(rc == PNL_CANCEL) return(0);

	data = graph->data_ptr[GraphPaneltableau]; if(!data) return(0);
	if(((data->parm & GRF_AXIS) == GRF_ZAXIS) || ((data->parm & GRF_AXIS) == GRF_IMAGE)) GraphDialogPlancheZ(data);
	else {
		strcpy(GraphPanelNom,data->titre);
		GraphPanelforme = (data->trace[qual] & GRF_MODE) >> GRF_SHIFT_MODE;
		GraphPaneltype  = (data->trace[qual] & GRF_LINE) >> GRF_SHIFT_LINE;
		GraphPanelepais = (data->trace[qual] & GRF_WIDZ) >> GRF_SHIFT_WIDZ;
		GraphPanelDetails = (data->style & GRAPH_DETAIL) >> GRF_SHIFT_DETAIL;
		PanelItemSelect(GraphPtableau,6,1);
		switch(data->couleur.type) {
		  case GRF_NOM: strncpy(GraphPanelcouleur,&(data->couleur.def.nom[qual][0]),WND_MAXCOULEUR); break;
		  case GRF_RGB:
			snprintf(GraphPanelcouleur,WND_MAXCOULEUR,"0x%04X%04X%04X",
				data->couleur.def.rgb[qual].r,data->couleur.def.rgb[qual].g,data->couleur.def.rgb[qual].b);
			break;
		  case GRF_LUT:
			PanelItemSelect(GraphPtableau,6,0);
			GraphPanelcouleur[0] = '\0';
			break;
		}

		if(f) OpiumPosition(GraphPtableau->cdr,f->x+f->h,f->y);
		if(OpiumExec(GraphPtableau->cdr) == PNL_CANCEL) return(0);

		if(GraphPanelforme < 0) GraphPanelforme = 0; else if(GraphPanelforme > 3) GraphPanelforme = 3;
		/* if(GraphPaneltype < 0) GraphPaneltype = 0; else */ if(GraphPaneltype > 7) GraphPaneltype = 7;
		if(GraphPanelepais < 1) GraphPanelepais = 1; else if(GraphPanelepais > 7) GraphPanelepais = 7;
		data->style = (data->style & (unsigned char)~(GRAPH_DETAIL)) | (unsigned char)(GraphPanelDetails << GRF_SHIFT_DETAIL);
		strcpy(data->titre,GraphPanelNom);
		data->trace[qual] = (unsigned char)
		   (((GraphPanelforme & 3) << GRF_SHIFT_MODE)
		  | ((GraphPaneltype  & 7) << GRF_SHIFT_LINE)
		  | ((GraphPanelepais & 7) << GRF_SHIFT_WIDZ));
		if(!strncmp(GraphPanelcouleur,"0x",2)) {
			data->couleur.type = GRF_RGB;
			sscanf(GraphPanelcouleur,"0x%04hx%04hx%04hx",&(data->couleur.def.rgb[qual].r),
				&(data->couleur.def.rgb[qual].g),&(data->couleur.def.rgb[qual].b));
		} else if(GraphPanelcouleur[0] != '\0') {
			data->couleur.type = GRF_NOM;
			strcpy(&(data->couleur.def.nom[qual][0]),GraphPanelcouleur);
		}
		// GC defini dans Xlib.h comme "struct _XGC" avec juste un element: GContext gid
		// if(data->gc[qual]) (data->gc[qual])->line_width = GraphPanelepais;
		WndContextLine(f,data->gc[qual],GraphPaneltype,GraphPanelepais);
		//- printf("(%s) @%08lX.%08lX.line_width = %d\n",__func__,(Ulong)data,(Ulong)(data->gc),(data->gc)->line_width);
	}

	graph->pret = 0;
	if(GraphPanelDetails != GRF_DETAIL_TOUT) OpiumRefreshGraph(graph->cdr);

	return(0);
}
/* ========================================================================== */
static int GraphDialogContraste(void) {
	int index; Graph graph; GraphData data;

	graph = GraphDialogGraph;
	for(index=0; index<graph->dim; index++) {
		data = graph->data_ptr[index]; if(!data) continue;
		if(((data->parm & GRF_AXIS) == GRF_ZAXIS) || ((data->parm & GRF_AXIS) == GRF_IMAGE)) {
			GraphDialogPlancheZ(data);
			return(index);
		}
	}
	return(-1);
}
/* ========================================================================== */
static int GraphDialogAjoute(void) {
	int i,n; int nb,num,qual;
	short index; int rc;
	char *nom[OPIUM_MAXCDR];
	Cadre cdr; Graph graph; GraphData data,data_ajoutee; WndFrame f;

	nb = 0; cdr = 0;
	for(i=0; i<OpiumNbCdr; i++) {
		cdr = OpiumCdr[i];
		if(cdr->type == OPIUM_GRAPH) nom[nb++] = cdr->nom;
	}
	if(cdr == 0) return(0);
	nom[nb] = "\0";
	f = (GraphMdial->cdr)->f;
	if(f) OpiumPosition(OpiumPanelQuick->cdr,f->x+f->h,f->y);
	num = 0;
	if(OpiumReadList("De quel graphique",nom,&num,OPIUM_MAXNOM) == PNL_CANCEL) return(0);
	n = 0;
	for(i=0; i<OpiumNbCdr; i++) {
		cdr = OpiumCdr[i];
		if(cdr->type == OPIUM_GRAPH) {
			if(n == num) break; else n++;
		}
	}
	graph = (Graph)cdr->adrs;
	if(graph->dim <= 0) { OpiumError("Ce graphique est vide");  return(0); }
	GraphPaneltableau = graph->axe[1].num;
	for(index=0; index<graph->dim; index++) {
		data = graph->data_ptr[index];
		if(data) {
			if(data->titre[0]) {
				nom[index] = data->titre;
				continue;
			}
		}
		nom[index] = malloc(16);
		if(data) sprintf(nom[index],"(donnee #%d)",index);
		else strcpy(nom[index],"(neant)");
	}
	nom[index] = "\0";
	if(!f) f = (graph->cdr)->f;
	if(f) OpiumPosition(OpiumPanelQuick->cdr,f->x+f->h,f->y);
	rc = OpiumReadList("Table a copier",nom,&GraphPaneltableau,OPIUM_MAXNOM);
	for(index=0; index<graph->dim; index++) {
		data = graph->data_ptr[index];
		if(data) { if(data->titre[0]) continue; }
		free(nom[index]);
	}
	if(rc == PNL_CANCEL) return(0);
	data_ajoutee = graph->data_ptr[GraphPaneltableau]; if(!data_ajoutee) return(0);

	graph = GraphDialogGraph;
	index = graph->dim; data = 0; /* pour le cas ou la boucle ne demarre meme pas */
	for(index=0; index<graph->dim; index++) {
		if((data = graph->data_ptr[index]) == 0) break;
		if(data->fctn == (void *)-1) break;
		data = 0; /* pour le cas ou la boucle va jusqu'a la fin */
	}
	if(index >= GRF_MAX_DATA) {
		OpiumError("(GraphDialogAjoute) Pas plus de %d tableaux",GRF_MAX_DATA);
		return(-1);
	} else if(index >= graph->max) {
		if(!data) data = (GraphData)malloc(sizeof(TypeGraphData));
		if(data) graph->data_ptr[index] = data;
		else {
			OpiumError("(GraphDialogAjoute) Manque de place memoire");
			return(-1);
		}
	} else data = graph->data_ptr[index];
	memcpy(data,data_ajoutee,sizeof(TypeGraphData));
	for(qual=0; qual<WND_NBQUAL; qual++) if(data->gc[qual]) {
		/* chacun son gc, et les data seront bien gerees */
		data->gc[qual] = WndContextSupportCreate(cdr->f,qual);
		WndContextCopy(cdr->f,data_ajoutee->gc[qual],data->gc[qual]);
	}
	if(index == graph->dim) graph->dim = index + 1;
	graph->pret = 0;
	OpiumRefreshGraph(graph->cdr);

	return(0);
}
/* ========================================================================== */
static int GraphDialogRetire(void) {
	int index,rc;
	char *nom[OPIUM_MAXCDR];
	Graph graph; GraphData data; WndFrame f;

	graph = GraphDialogGraph;
	f = (GraphMdial->cdr)->f;
	GraphPaneltableau = graph->axe[1].num;
	for(index=0; index<graph->dim; index++) {
		data = graph->data_ptr[index];
		if(data) {
			if(data->titre[0]) {
				nom[index] = data->titre;
				continue;
			}
		}
		nom[index] = malloc(16);
		if(data) sprintf(nom[index],"(donnee #%d)",index);
		else strcpy(nom[index],"(neant)");
	}
	nom[index] = "\0";
	if(!f) f = (graph->cdr)->f;
	if(f) OpiumPosition(OpiumPanelQuick->cdr,f->x+f->h,f->y);
	rc = OpiumReadList("Table a retirer",nom,&GraphPaneltableau,OPIUM_MAXNOM);
	for(index=0; index<graph->dim; index++) {
		data = graph->data_ptr[index];
		if(data) {
			if(data->titre[0]) continue;
		}
		free(nom[index]);
	}
	if(rc == PNL_CANCEL) return(0);
	GraphDataRemove(graph,GraphPaneltableau);
	graph->pret = 0;
	OpiumRefreshGraph(graph->cdr);

	return(0);
}
/* ========================================================================== */
static int GraphDialogZoomPlus(void) { OpiumZoomAutorise = 1; return(1); }
/* ========================================================================== */
static int GraphDialogZoomMoins(void) {
	int i; GraphScale *scale;
	float rmilieu,ramplitude;
	int imilieu,iamplitude;

	for(i=0,scale = GraphDialogGraph->axe; i<2; i++,scale++) {
		if(scale->reel) {
			rmilieu = (scale->u.r.max + scale->u.r.min) / 2.0f;
			ramplitude = scale->u.r.max - scale->u.r.min;
			scale->u.r.min = rmilieu - ramplitude;
			scale->u.r.max = rmilieu + ramplitude;
			if(scale->logd && (scale->u.r.min <= 0.0f)) scale->u.r.min = 0.001f;
		} else {
			imilieu = (scale->u.i.max + scale->u.i.min) / 2;
			iamplitude = scale->u.i.max - scale->u.i.min;
			scale->u.i.min = imilieu - iamplitude;
			scale->u.i.max = imilieu + iamplitude;
			if(scale->logd && (scale->u.i.min <= 0)) scale->u.i.min = 1;
		}
		scale->minauto = scale->maxauto = 0;
		scale->pret = 0;
	}
	// graph->pret = 0;
	OpiumRefreshGraph(GraphDialogGraph->cdr);

	return(1);
}
/* ========================================================================== */
static int GraphDialogZoomJuste(void) {
	int i; GraphScale *scale;

	for(i=0,scale = GraphDialogGraph->axe; i<2; i++,scale++) {
		scale->minauto = scale->maxauto = 1; scale->pret = 0;
	}
	GraphDialogGraph->pret = 0;
	OpiumRefreshGraph(GraphDialogGraph->cdr);

	return(1);
}
/* ========================================================================== */
static void GraphDialogTablesVal(GraphData data, int k, char demi, char *val) {
	int i,nb;
	char court,reel,fctn;
	int *itab; int ival;
	short *stab; short sval;
	float *rtab; float rval;

	nb = data->max - data->min;
	if(((data->min != 0) || (data->max != 0)) && (nb <= 0)) nb += data->dim;
	if(k < nb) {
		i = data->min + k; if(i >= data->dim) i -= data->dim;
		court = (data->parm & GRF_TYPE) == GRF_SHORT;
		reel = (data->parm & GRF_TYPE) == GRF_FLOAT;
		fctn = (data->parm & GRF_ADRS) == GRF_FCTN;
		if((data->parm & GRF_ADRS) == GRF_INDEX) {
			if(reel) {
				rtab = (float *)data->fctn;
				rval = *rtab + ((data->min + k) * *(rtab+1)); if(demi) rval /= 2.0f;
				// fprintf(OpiumTxtfile," %g",rval);
				sprintf(val,"%g",rval);
			} else if(court) {
				stab = (short *)data->fctn;
				sval = *stab + ((short)(data->min + k) * *(stab+1)); if(demi) sval /= 2;
				sprintf(val,"%d",sval);
			} else {
				itab = (int *)data->fctn;
				ival = *itab + ((data->min + k) * *(itab+1)); if(demi) ival /= 2;
				sprintf(val,"%d",ival);
			}
		} else if(fctn) {
			if(reel) {
				void (*fctnTmp)(int, float*) = (void (*)(int, float*))data->fctn;
				(*fctnTmp)(i,&rval); if(demi) rval /= 2.0f;
				sprintf(val,"%g",rval);
			} else if(court) {
				void (*fctnTmp)(int, short*) = (void (*)(int, short*))data->fctn;
				(*fctnTmp)(i,&sval); if(demi) sval /= 2;
				sprintf(val,"%d",sval);
			} else {
				void (*fctnTmp)(int, int*) = (void (*)(int, int*))data->fctn;
				(*fctnTmp)(i,&ival); if(demi) ival /= 2;
				sprintf(val,"%d",ival);
			}
		} else {
			if(reel) {
				rtab = (float *)data->fctn + i;
				rval = *rtab; if(demi) rval /= 2.0f;
				sprintf(val,"%g",rval);
			} else if(court) {
				stab = (short *)data->fctn + i;
				sval = *stab; if(demi) sval /= 2;
				sprintf(val,"%d",sval);
			} else {
				itab = (int *)data->fctn + i;
				ival = *itab; if(demi) ival /= 2;
				sprintf(val,"%d",ival);
			}
		}
	} else strcpy(val,"x");
}
/* ========================================================================== */
static int GraphDialogTablesEcrit(void) {
	Graph graph; GraphData *data_ptr,data;
	TypeGraphIndiv *indiv,*indiv_point;
	GraphData /* xerr_demande,yerr_demande, */ xerr,yerr;
	WndFrame f; int qual;
	unsigned char axe;
	int num,k,x,y,lngr;
	int nb,xmax,ymax;
	char trois_d;
	char titre[GRF_MAX_TITRE];
#define MAXLIGNE 256
	char ligne[MAXLIGNE],val[32]; char *c;

	if(!OpiumTxtCreer()) {
		OpiumError("Tables non sauvegardees");
		return(0);
	}
	graph = GraphDialogGraph;
	if((f = (graph->cdr)->f)) qual = f->qualite; else qual = WND_Q_ECRAN;
	trois_d = 0;

/*	Premiere ligne: nom des variables representees */
	fprintf(OpiumTxtfile,"# %s\n",(graph->cdr)->titre);
	fprintf(OpiumTxtfile,"#");
	xmax = ymax = 0;
	data_ptr = graph->data_ptr;
	for(num=0; num<graph->dim; num++, data_ptr++) {
		data = *data_ptr; if(!data) continue;
		if(data->fctn == (void *)-1) continue;
		if((data->trace[qual] & GRF_MODE) == GRF_CODE_ERR) continue;
		if((data->parm & GRF_ADRS) == GRF_INDIV) continue;
		strcpy(titre,data->titre);
		if(titre[0]) {
			size_t l;
			l = strlen(titre);
			while(l--) if(titre[l] ==' ') titre[l]= '_';
		} else sprintf(titre,"Var%d",num);
		fprintf(OpiumTxtfile," %s",titre);
		nb = data->max - data->min;
		if(((data->min != 0) || (data->max != 0)) && (nb <= 0)) nb += data->dim;
		axe = data->parm & GRF_AXIS;
		if(axe == GRF_XAXIS) { if(xmax < nb) xmax = nb; }
		else if(axe == GRF_YAXIS) { if(ymax < nb) ymax = nb; }
		// else if(axe == GRF_ZAXIS) trois_d = 1; ne marche pas encore
	}
	fprintf(OpiumTxtfile,"\n");

	if(!trois_d) { if(xmax < ymax) xmax = ymax; }
/*	Lignes suivantes: valeurs */
	x = 0;
	while(x < xmax) {
		lngr = 0; ligne[0] = '\0'; c = ligne;
		if(trois_d) y = 0; else y = x;
		while(y < ymax) {
			data_ptr = graph->data_ptr;
			xerr = yerr = 0;
			for(num=0; num<graph->dim; num++, data_ptr++) {
				data = *data_ptr; if(!data) continue;
				if(data->fctn == (void *)-1) continue;
				axe = data->parm & GRF_AXIS;
				if((data->trace[qual] & GRF_MODE) == GRF_CODE_ERR) {
					if(axe == GRF_XAXIS) xerr = data;
					else if(axe == GRF_YAXIS) yerr = data;
					continue;
				}
				if((data->parm & GRF_ADRS) == GRF_INDIV) {
					indiv = (TypeGraphIndiv *)data->fctn;
					indiv_point = indiv + x;
					if(GRF_HIDDEN(indiv_point->trace)) { ligne[0] = '\0'; break; }
					else continue;
				}
				if(trois_d) {
					if(axe == GRF_XAXIS) GraphDialogTablesVal(data,x,0,val);
					else if(axe == GRF_YAXIS) GraphDialogTablesVal(data,y,0,val);
					else if(axe == GRF_ZAXIS) { k = (y * xmax) + x; GraphDialogTablesVal(data,k,0,val); }
				} else GraphDialogTablesVal(data,x,0,val);
				if(!lngr) strncpy(c,val,MAXLIGNE);
				else { strncat(strncpy(c+lngr," ",MAXLIGNE-lngr),val,MAXLIGNE-lngr-1); lngr++; }
				lngr += strlen(val); if(lngr >= MAXLIGNE) break;
				if(xerr) {
					GraphDialogTablesVal(xerr,x,1,val);
					strncat(strncpy(c+lngr,"+/-",MAXLIGNE-lngr),val,MAXLIGNE-lngr-1); lngr += 3;
					xerr = 0;
				}
				if(yerr) {
					GraphDialogTablesVal(yerr,x,1,val);
					strncat(strncpy(c+lngr,"+/-",MAXLIGNE-lngr),val,MAXLIGNE-lngr-1); lngr += 3;
					yerr = 0;
				}
			}
			if(ligne[0]) fprintf(OpiumTxtfile,"%s\n",ligne);
			if(trois_d) y++; else break;
		}
		x++;
	}

	fclose(OpiumTxtfile);
	return(1);
}
/* ========================================================================== */
static int GraphDialogEtatRetabli(void) {
	if(GraphDialogGraph->mode_sauve == 0xFFFF) {
		OpiumError("L'etat precedent n'a pas ete sauve, et ne sera donc pas restaure");
		return(0);
	}
	GraphDialogGraph->mode = GraphDialogGraph->mode_sauve;
	memcpy(&(GraphDialogGraph->axe[0]),&(GraphDialogGraph->axe_sauve[0]),sizeof(GraphScale));
	memcpy(&(GraphDialogGraph->axe[1]),&(GraphDialogGraph->axe_sauve[1]),sizeof(GraphScale));
	GraphDialogGraph->pret = 0;
	OpiumRefreshGraph(GraphDialogGraph->cdr);
	return(1);
}
/* ========================================================================== */
static int GraphDialogEtatSauve(void) {
	GraphDialogGraph->mode_sauve = GraphDialogGraph->mode;
	memcpy(&(GraphDialogGraph->axe_sauve[0]),&(GraphDialogGraph->axe[0]),sizeof(GraphScale));
	memcpy(&(GraphDialogGraph->axe_sauve[1]),&(GraphDialogGraph->axe[1]),sizeof(GraphScale));
	return(0);
}
/* ========================================================================== */
int GraphParmsSave(Graph graph) {
	if(!graph) return(0);
	graph->mode_sauve = graph->mode;
	memcpy(&(graph->axe_sauve[0]),&(graph->axe[0]),sizeof(GraphScale));
	memcpy(&(graph->axe_sauve[1]),&(graph->axe[1]),sizeof(GraphScale));
	return(1);
}
/* ========================================================================== */
int OpiumPSComplet(),OpiumPSDemarre(),OpiumPSAjoute(),OpiumPSPage(),OpiumPSTermine();
TypeMenuItem GraphIdial[] = {
	{ "Aspect",          MNU_SEPARATION },
	{ "General",         MNU_FONCTION GraphDialogGeneral },
	{ "X (axe des X)",   MNU_FONCTION GraphDialogAxe },
	{ "Y (axe des Y)",   MNU_FONCTION GraphDialogAxe },
	{ "Donnee",          MNU_FONCTION GraphDialogDonnee },
	{ "Contraste [3D]",  MNU_FONCTION GraphDialogContraste },
	{ "Autoriser zoom",  MNU_FONCTION GraphDialogZoomPlus },
	{ "Zoom AR (x2)",    MNU_FONCTION GraphDialogZoomMoins },
	{ "Dimensions auto", MNU_FONCTION GraphDialogZoomJuste },
	{ "Sauver etat",     MNU_FONCTION GraphDialogEtatSauve },
	{ "Annuler modifs",  MNU_FONCTION GraphDialogEtatRetabli },
	{ "Contenu",         MNU_SEPARATION },
	{ "Ajouter donnee",  MNU_FONCTION GraphDialogAjoute },
	{ "Retirer donnee",  MNU_FONCTION GraphDialogRetire },
	{ "Sauver tables",   MNU_FONCTION GraphDialogTablesEcrit  },
	{ "Postscript",      MNU_SEPARATION },
	{ "Creer et fermer", MNU_FONCTION OpiumPSComplet },
	{ "Premier dessin",  MNU_FONCTION OpiumPSDemarre },
	{ "Dessin suivant",  MNU_FONCTION OpiumPSAjoute },
	{ "Saut de page",    MNU_FONCTION OpiumPSPage },
	{ "Fin du fichier",  MNU_FONCTION OpiumPSTermine },
	{ "\0",              MNU_SEPARATION },
	{ "Quitter",         MNU_RETOUR },
	MNU_END
};
/* ========================================================================== */
void GraphDialogCreate(void) {

	GraphMdial = MenuLocalise(GraphIdial);

	GraphPgeneral = PanelCreate(6);
	PanelText (GraphPgeneral,"Titre",GraphPaneltitre,OPIUM_MAXTITRE);
	PanelKeyB (GraphPgeneral,"Reperes","pas d'axe/2 axes/4 axes",&GraphPanelnb_axes,12);
	PanelBool (GraphPgeneral,"Grille",&GraphPanelgrille);
	PanelBool (GraphPgeneral,"Graduations",&GraphPanelgrads);
	PanelBool (GraphPgeneral,"Legendes",&GraphPanellegende);
	PanelListB(GraphPgeneral,"Qualite",WndSupportNom,&GraphPanelqualite,7);
	/* mode peut etre GRAPH_DIRECT */

	GraphPaxe = PanelCreate(8);
	PanelInt (GraphPaxe,"Numero de tableau",&GraphPaneltableau);
	PanelInt (GraphPaxe,"Fenetrage relatif",&GraphPanelzoom);
	PanelKeyB(GraphPaxe,"Minimum","impose/auto",&GraphPanelmin_auto,8);
	PanelKeyB(GraphPaxe,"Maximum","impose/auto",&GraphPanelmax_auto,8);
	PanelKeyB(GraphPaxe,"Valeur graduations","impose/auto",&GraphPanelgrad_auto,8);
	PanelKeyB(GraphPaxe,"Echelle","lineaire/log",&GraphPanellog,10);
	PanelText(GraphPaxe,"Format graduations",GraphPanelfmt,12);
	PanelText(GraphPaxe,"Titre",GraphPaneltitre,40);

	GraphPmanu = PanelCreate(3);

	GraphPnumdata = PanelCreate(1);
	PanelInt(GraphPnumdata,"Numero de tableau",&GraphPaneltableau);

	GraphPtableau = PanelCreate(6);
	PanelText (GraphPtableau,"Nom",GraphPanelNom,GRF_MAX_TITRE);
	PanelListB(GraphPtableau,"Forme trace",GraphModeName,&GraphPanelforme,8);
	PanelInt  (GraphPtableau,"Type trace",(int *)&GraphPaneltype);
	PanelKeyB (GraphPtableau,"Detail","lissage/min-max/tout",&GraphPanelDetails,8);
	PanelInt  (GraphPtableau,"Epaisseur",(int *)&GraphPanelepais);
	PanelText (GraphPtableau,"Couleur",GraphPanelcouleur,WND_MAXCOULEUR);
/*	axe [axe]: (tableau) (min/auto) (max/auto) (grad/auto) (log) (format)
	trace [tableau]: (courbe/histo/erreur/points) (type trace ou points)
	                 (epaisseur) (couleur rgb)
*/
}
/* ========================================================================== */
static void GraphDialogInit(Cadre cdr) {
	OpiumPScadre = cdr;
	GraphDialogCadre = cdr;
	GraphDialogGraph = (Graph)cdr->adrs;
	if(GraphDialogGraph->mode == GRF_DIRECT) MenuItemDisable(GraphMdial,1);
	else MenuItemEnable(GraphMdial,1);
}
/* ========================================================================== */
int GraphDialogExec(Cadre cdr /*, WndAction *u */) {
	int x,dx;

	GraphDialogInit(cdr);
/*1	OpiumPosition(GraphMdial->cdr,(cdr->f)->x,(cdr->f)->y); */
/*	OpiumPosition(GraphMdial->cdr,u->x,u->y); */
/*	OpiumPosition(GraphMdial->cdr,(cdr->f)->x+((cdr->f)->h/2),(cdr->f)->y+((cdr->f)->v/2)); */
/*	OpiumPosition(GraphMdial->cdr,(cdr->f)->x+u->x,(cdr->f)->y+u->y); */
	dx = WndColToPix(16); /* largeur supposee du menu GraphMdial */
	if(cdr->f) {
		x = (cdr->f)->x + (cdr->f)->h + WndColToPix(1) + dx;
		if(x > OpiumServerWidth(0)) x = (cdr->f)->x + (cdr->f)->h - WndColToPix(2);
		OpiumPosition(GraphMdial->cdr,x - dx,(cdr->f)->y);
	}
	MenuTitle(GraphMdial,cdr->titre);
	OpiumExec(GraphMdial->cdr);
	// OpiumRefresh(cdr);
	return(0);
}

