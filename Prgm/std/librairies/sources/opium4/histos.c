#ifdef macintosh
#pragma message(__FILE__)
#endif

#include <stdlib.h>
#include <string.h>

#define HISTOS_C
#include <histos.h>

/* ===================================================================== */
static int HistoVerifAxeInt(int min, int max, int nb) {
	if(nb <= 0) { HistoError = HST_NUMBER; return(0); }
	if(max <= min) { HistoError = HST_RANGE; return(0); }
	return(1);
}
/* ..................................................................... */
static int HistoVerifAxeFloat(float min, float max, int nb) {
	if(nb <= 0) { HistoError = HST_NUMBER; return(0); }
	if(max <= min) { HistoError = HST_RANGE; return(0); }
	return(1);
}
/* ..................................................................... */
static void HistoInitAxeInt(HistoAxe *axe, int min, int max, int nb) {
	axe->type = HST_INT;
	axe->v.i[0] = min;
	axe->v.i[1] = (max - min) / nb;
	axe->nb = nb;
}
/* ..................................................................... */
static void HistoInitAxeFloat(HistoAxe *axe, float min, float max, int nb) {
	axe->type = HST_FLOAT;
	axe->v.r[0] = min;
	axe->v.r[1] = (max - min) / (float)nb;
	axe->nb = nb;
}
/* ..................................................................... */
static void HistoDataClear(HistoData data, int nb) {
	int k;
	int *i; short *s; float *r;

	if(!data) return;
	switch(data->type) {
	  case HST_INT:
		data->under.i = data->over.i = 0;
		i = (int *)(data->adrs);
		k = nb; while(k--) *i++ = 0;
		break;
	  case HST_SHORT:
		data->under.s = data->over.s = 0;
		s = (short *)(data->adrs);
		k = nb; while(k--) *s++ = 0;
		break;
	  case HST_FLOAT:
	  	data->under.r = data->over.r = 0.0;
		r = (float *)(data->adrs);
		k = nb; while(k--) *r++ = 0.0;
	  	break;
	}
}
/* ===================================================================== */
Histo HistoCreateInt(int min, int max, int nb) {
	Histo h; int qual;

	if(!HistoVerifAxeInt(min,max,nb)) return(0);
	h = (Histo)malloc(sizeof(TypeHisto));
	if(!h) { HistoError = HST_MEMORY; return(0); }
	HistoInitAxeInt(&(h->x),min,max,nb);
	for(qual=0; qual<WND_NBQUAL; qual++)
		h->axe[qual].r = h->axe[qual].g = h->axe[qual].b = WndLevelText[qual];
	h->last = h->first = 0;

	return(h);
}
/* ===================================================================== */
Histo HistoCreateFloat(float min, float max, int nb) {
	Histo h; int qual;

	if(!HistoVerifAxeFloat(min,max,nb)) return(0);
	h = (Histo)malloc(sizeof(TypeHisto));
	if(!h) { HistoError = HST_MEMORY; return(0); }
	HistoInitAxeFloat(&(h->x),min,max,nb);
	for(qual=0; qual<WND_NBQUAL; qual++)
		h->axe[qual].r = h->axe[qual].g = h->axe[qual].b = WndLevelText[qual];
	h->last = h->first = 0;
//	printf("(HistoCreateFloat) Create Histo @%08lX\n",(Ulong)h);

	return(h);
}
/* ===================================================================== */
void HistoErase(Histo histo) {
	HistoData data,next;

	if(!histo) return;
//	printf("(HistoErase) Erase Histo @%08lX (first data @%08lX)\n",(Ulong)histo,(Ulong)(histo->first));
	data = histo->first;
	while(data) {
//		printf("(HistoErase) data to free @%08lX\n",(Ulong)data);
		next = data->next;
//		printf("(HistoErase) next data @%08lX\n",(Ulong)next);
		free(data);
		data = next;
	}
	histo->last = histo->first = 0;
}
/* ===================================================================== */
void HistoDelete(Histo histo) {
	if(histo) {
		HistoErase(histo);
		free(histo); /* printf("(HistoDelete) freed Histo @%08lX\n",(Ulong)histo); */
	}
}
/* ===================================================================== */
void HistoDataDelete(HistoData data) {
	Histo histo; HistoData prev,next;

	if(data) {
		histo = data->histo;
		next = histo->first; prev = 0;
		while(next) {
			if(next == data) break;
			prev = next;
			next = next->next;
		}
		if(next) {
			if(prev) prev->next = data->next; else histo->first = data->next;
			if(next == histo->last) { prev->next = 0; histo->last = prev; }
			free(data);
		}
	}
}
/* ===================================================================== */
void HistoAxeRGB(Histo histo, int qual,
	WndColorLevel r, WndColorLevel g, WndColorLevel b) {
	if(!histo) return;
	histo->axe[qual].r = r; histo->axe[qual].g = g ; histo->axe[qual].b = b;
}
/* ===================================================================== */
HistoData HistoAdd(Histo histo, char type) {
	int taille,qual;
	HistoData data,adrs;

	if(!histo) return(0);
//	printf("(%s) Add to Histo @%08lX (last data @%08lX)\n",__func__,(Ulong)histo,(Ulong)(histo->last));
	switch(type) {
	  case HST_INT: taille = sizeof(int); break;
	  case HST_SHORT: taille = sizeof(short); break;
	  case HST_FLOAT: taille = sizeof(float); break;
	  default: taille = 4;
	}
	adrs = (HistoData)malloc(sizeof(TypeHistoData) + (histo->x.nb * taille));
	if(!adrs) { HistoError = HST_MEMORY; return(0); };
//	printf("(%s) data added @%08lX\n",__func__,(Ulong)adrs);
	data = histo->last;
	if(data) {
		data->next = adrs;
//		printf("(%s) %08lX->next = @%08lX\n",__func__,(Ulong)data,(Ulong)adrs);
		data = data->next;
	} else histo->first = data = adrs;
//	printf("(%s) %08lX->first = @%08lX\n",__func__,(Ulong)histo,(Ulong)(histo->first));
	data->type = type;
	data->titre[0]= '\0';
	data->adrs = (void *)((IntAdrs)adrs + sizeof(TypeHistoData));
	HistoDataClear(data,histo->x.nb);
	for(qual=0; qual<WND_NBQUAL; qual++)
		data->coul[qual].r = data->coul[qual].g = data->coul[qual].b = WndLevelText[qual];
	data->histo = (void *)histo;
	data->next = 0;
//	printf("(%s) %08lX->next = @%08lX\n",__func__,(Ulong)data,(Ulong)(data->next));
	histo->last = data;
//	printf("(%s) => %08lX->last = @%08lX\n",__func__,(Ulong)histo,(Ulong)(histo->last));
	return(data);
}
/* ===================================================================== */
void HistoDataName(HistoData data, char *nom) {
	if(!data) return;
	strncpy(data->titre,nom,GRF_MAX_TITRE);
}
/* ===================================================================== */
void HistoDataSupportRGB(HistoData data, int qual,
				  WndColorLevel r, WndColorLevel g, WndColorLevel b) {
	if(!data) return;
	data->coul[qual].r = r; data->coul[qual].g = g ; data->coul[qual].b = b;
	// printf("(%s) HistoData %s: %04X.%04X.%04X\n",__func__,WndSupportNom[qual],r,g,b);
}
/* ===================================================================== */
void HistoFillIntToInt(HistoData data, int val, int poids) {
/* <val> est du type <histo->x.type>, <data> et <poids> du type <data->type> */
	Histo histo; int bin; int *adrs;

	if(!data) return;
	if(data->type != HST_INT) return;
	histo = (Histo)data->histo;
	if(histo->x.type != HST_INT) return;
	bin = (val - histo->x.v.i[0]) / histo->x.v.i[1];
	if(bin < 0) data->under.i += poids;
	else if(bin >= histo->x.nb) data->over.i += poids;
	else { adrs = (int *)data->adrs; adrs[bin] += poids; }
}
/* ===================================================================== */
void HistoFillIntToFloat(HistoData data, int val, float poids) {
/* <val> est du type <histo->x.type>, <data> et <poids> du type <data->type> */
	Histo histo; int bin; float *adrs;

	if(!data) return;
	if(data->type != HST_FLOAT) return;
	histo = (Histo)data->histo;
	if(histo->x.type != HST_INT) return;
	bin = (val - histo->x.v.i[0]) / histo->x.v.i[1];
	if(bin < 0) data->under.r += poids;
	else if(bin >= histo->x.nb) data->over.r += poids;
	else { adrs = (float *)data->adrs; adrs[bin] += poids; }
}
/* ===================================================================== */
void HistoFillFloatToInt(HistoData data, float val, int poids) {
/* <val> est du type <histo->x.type>, <data> et <poids> du type <data->type> */
	Histo histo; int bin; int *adrs;

	if(!data) return;
	if(data->type != HST_INT) return;
	histo = (Histo)data->histo;
	if(histo->x.type != HST_FLOAT) return;
	bin = (int)((val - histo->x.v.r[0]) / histo->x.v.r[1]);
	if(bin < 0) data->under.i += poids;
	else if(bin >= histo->x.nb) data->over.i += poids;
	else { adrs = (int *)data->adrs; adrs[bin] += poids; }
}
/* ===================================================================== */
void HistoFillFloatToFloat(HistoData data, float val, float poids) {
/* <val> est du type <histo->x.type>, <data> et <poids> du type <data->type> */
	Histo histo; int bin; float *adrs;

	if(!data) return;
	if(data->type != HST_FLOAT) return;
	histo = (Histo)data->histo;
	if(histo->x.type != HST_FLOAT) return;
	bin = (int)((val - histo->x.v.r[0]) / histo->x.v.r[1]);
	if(bin < 0) data->under.r += poids;
	else if(bin >= histo->x.nb) data->over.r += poids;
	else { adrs = (float *)data->adrs; adrs[bin] += poids; }
}
/* ===================================================================== */
int HistoLimitsInt(Histo histo, int min, int max) {
	if(!histo) return(0);
	if(max <= min) { HistoError = HST_RANGE; return(0); }
	histo->x.type = HST_INT;
	histo->x.v.i[0] = min;
	histo->x.v.i[1] = (max - min) / histo->x.nb;
	if(histo->x.v.i[1] <= 0) { HistoError = HST_WIDTH; return(0); }
	return(1);
}
/* ===================================================================== */
int HistoLimitsFloat(Histo histo, float min, float max) {
	if(!histo) return(0);
	if(max <= min) { HistoError = HST_RANGE; return(0); }
	histo->x.type = HST_FLOAT;
	histo->x.v.r[0] = min;
	histo->x.v.r[1] = (max - min) / (float)(histo->x.nb);
	if(histo->x.v.r[1] <= 0.0) { HistoError = HST_WIDTH; return(0); }
	return(1);
}
/* ===================================================================== */
int HistoRebin(Histo histo, int nb) {
	HistoData data,svte; int qual;
	int imax; short smax; float rmax;

	if(!histo) return(0);
	if(nb <= 0) { HistoError = HST_NUMBER; return(0); }
	if(nb == histo->x.nb) return(1);
	switch(histo->x.type) {
	  case HST_INT:
		imax = histo->x.v.i[0] + (histo->x.nb * histo->x.v.i[1]);
		histo->x.v.i[1] = (imax - histo->x.v.i[0]) / nb;
		break;
	  case HST_SHORT:
		smax = histo->x.v.s[0] + ((short)histo->x.nb * histo->x.v.s[1]);
		histo->x.v.s[1] = (smax - histo->x.v.s[0]) / nb;
		break;
	  case HST_FLOAT:
		rmax = histo->x.v.r[0] + ((float)histo->x.nb * histo->x.v.r[1]);
		histo->x.v.r[1] = (rmax - histo->x.v.r[0]) / (float)nb;
		break;
	}
	histo->x.nb = nb;
	data = histo->first;
	histo->last = 0;
	while(data) {
		for(qual=0; qual<WND_NBQUAL; qual++)
			HistoDataSupportRGB(HistoAdd(histo,data->type),qual,data->coul[qual].r,data->coul[qual].g,data->coul[qual].b);
		svte = data->next; free(data); data = svte;
	}
	return(1);
}
/* ===================================================================== */
void HistoGetBinInt(Histo histo, int *min, int *bin, int *nb) {
	if(!histo) { *min = *bin = *nb = 0; return; }
	switch(histo->x.type) {
	  case HST_INT:
		*min = histo->x.v.i[0]; *bin = histo->x.v.i[1];
		break;
	  case HST_SHORT:
		*min = (int)histo->x.v.s[0]; *bin = (int)histo->x.v.s[1];
		break;
	  case HST_FLOAT:
		*min = (int)histo->x.v.r[0]; *bin = (int)histo->x.v.r[1];
		break;
	}
	*nb = histo->x.nb;
	return;
}
/* ===================================================================== */
int HistoGetBinFloat(Histo histo, float *min, float *bin, int *nb) {
	if(!histo) { *min = *bin = *nb = 0; return(0); }
	switch(histo->x.type) {
	  case HST_INT:
		*min = (float)histo->x.v.i[0]; *bin = (float)histo->x.v.i[1];
		break;
	  case HST_SHORT:
		*min = (float)histo->x.v.s[0]; *bin = (float)histo->x.v.s[1];
		break;
	  case HST_FLOAT:
		*min = histo->x.v.r[0]; *bin = histo->x.v.r[1];
		break;
	}
	*nb = histo->x.nb;
	return(1);
}
/* ===================================================================== */
int *HistoGetDataInt(HistoData hd) { return((int *)hd->adrs); }
/* ===================================================================== */
float *HistoGetDataFloat(HistoData hd) { return((float *)hd->adrs); }
/* ===================================================================== */
void HistoRaz(Histo histo) {
	HistoData data;

	if(!histo) return;
	data = histo->first;
	while(data) { HistoDataClear(data,histo->x.nb); data = data->next; }
}
/* ===================================================================== */
void HistoDataRaz(HistoData data) {
	HistoDataClear(data,((Histo)(data->histo))->x.nb);
}
/* ===================================================================== */
void HistoDump(Histo histo) {
	HistoData data; int k,qual;
	int *i; short *s; float *r;

	if(!histo) { WndPrint("(HistoDump) Histo non defini\n"); return; }
	WndPrint("(HistoDump) Histo @%08lX:\n",(Ulong)histo);
	switch(histo->x.type) {
	  case HST_INT:
		WndPrint("          abcisse INT  : [%d + %d x %d]",
			histo->x.v.i[0],histo->x.nb,histo->x.v.i[1]);
		break;
	  case HST_SHORT:
		WndPrint("          abcisse SHORT: [%d + %d x %d]",
			histo->x.v.s[0],histo->x.nb,histo->x.v.s[1]);
		break;
	  case HST_FLOAT:
		WndPrint("          abcisse FLOAT: [%g + %d x %g]",
			histo->x.v.r[0],histo->x.nb,histo->x.v.r[1]);
		break;
	  default:
		WndPrint("          abscisse non definie\n");
	}
	for(qual=0; qual<WND_NBQUAL; qual++)
		WndPrint(", couleur %s=rgb:%04X.%04X.%04X",WndSupportNom[qual],histo->axe[qual].r,histo->axe[qual].g,histo->axe[qual].b);
	WndPrint("\n");
	data = histo->first;
	while(data) {
		switch(data->type) {
		  case HST_INT:
			WndPrint("          donnees INT  @%08lX",(Ulong)data->adrs);
			for(qual=0; qual<WND_NBQUAL; qual++)
				WndPrint(", couleur %s=rgb:%04X.%04X.%04X",WndSupportNom[qual],data->coul[qual].r,data->coul[qual].g,data->coul[qual].b);
			WndPrint("\n");
			i = (int *)data->adrs;
			WndPrint("               underflow: %d",data->under.i);
			for(k=0; k<histo->x.nb; k++) {
				if(!(k % 5)) WndPrint("\n      %6d:  ",k);
				WndPrint("%12d",i[k]);
			}
			WndPrint(" \n");
			WndPrint("                overflow: %d\n",data->over.i);
			break;
		  case HST_SHORT:
			WndPrint("          donnees SHORT @%08lX",(Ulong)data->adrs);
			for(qual=0; qual<WND_NBQUAL; qual++)
				WndPrint(", couleur %s=rgb:%04X.%04X.%04X",WndSupportNom[qual],data->coul[qual].r,data->coul[qual].g,data->coul[qual].b);
			WndPrint("\n");
			s = (short *)data->adrs;
			WndPrint("               underflow: %d",data->under.s);
			for(k=0; k<histo->x.nb; k++) {
				if(!(k % 5)) WndPrint("\n      %6d:  ",k);
				WndPrint("%12d",s[k]);
			}
			WndPrint(" \n");
			WndPrint("                overflow: %d\n",data->over.s);
			break;
		  case HST_FLOAT:
			WndPrint("          donnees FLOAT @%08lX",(Ulong)data->adrs);
			for(qual=0; qual<WND_NBQUAL; qual++)
				WndPrint(", couleur %s=rgb:%04X.%04X.%04X",WndSupportNom[qual],data->coul[qual].r,data->coul[qual].g,data->coul[qual].b);
			WndPrint("\n");
			r = (float *)data->adrs;
			WndPrint("               underflow: %g",data->under.r);
			for(k=0; k<histo->x.nb; k++) {
				if(!(k % 5)) WndPrint("\n      %6d:  ",k);
				WndPrint("%12g",r[k]);
			}
			WndPrint(" \n");
			WndPrint("                overflow: %g\n",data->over.r);
			break;
		  default:
			WndPrint("          donnees non definies @%08lX\n",(Ulong)data->adrs);
		}
		data = data->next;
	}
	WndPrint("(HistoDump) Fin histo @%08lX:\n",(Ulong)histo);
}
/* ===================================================================== */
void HistoGraphAdd(Histo histo, Graph graph) {
	HistoData data; int i; int qual;

	if(!histo) return;
	if(!graph) return;
	switch(histo->x.type) {
	  case HST_INT:
		i = GraphAdd(graph,GRF_XAXIS|GRF_INDEX|GRF_INT,histo->x.v.i,histo->x.nb);
		break;
	  case HST_SHORT:
		i = GraphAdd(graph,GRF_XAXIS|GRF_INDEX|GRF_SHORT,histo->x.v.s,histo->x.nb);
		break;
	  case HST_FLOAT:
		i = GraphAdd(graph,GRF_XAXIS|GRF_INDEX|GRF_FLOAT,histo->x.v.r,histo->x.nb);
		break;
	  default: i = -1; break;
	}
	for(qual=0; qual<WND_NBQUAL; qual++)
		GraphDataSupportRGB(graph,i,qual,histo->axe[qual].r,histo->axe[qual].g,histo->axe[qual].b);
	data = histo->first;
	while(data) {
		switch(data->type) {
		  case HST_INT:
			i = GraphAdd(graph,GRF_YAXIS|GRF_INT,data->adrs,histo->x.nb);
			break;
		  case HST_SHORT:
			i = GraphAdd(graph,GRF_YAXIS|GRF_SHORT,data->adrs,histo->x.nb);
			break;
		  case HST_FLOAT:
			i = GraphAdd(graph,GRF_YAXIS|GRF_FLOAT,data->adrs,histo->x.nb);
			break;
		  default: i = -1;
		}
		if(i >= 0) {
			if(data->titre[0]) GraphDataName(graph,i,data->titre);
			for(qual=0; qual<WND_NBQUAL; qual++) {
				// printf("(%s) HistoData %s: %04X.%04X.%04X\n",__func__,WndSupportNom[qual],data->coul[qual].r,data->coul[qual].g,data->coul[qual].b);
				GraphDataSupportRGB(graph,i,qual,data->coul[qual].r,data->coul[qual].g,data->coul[qual].b);
				GraphDataSupportTrace(graph,i,qual,GRF_HST,0,GraphWidzDefaut[qual]);
			}
		}
		data = data->next;
	}
}
/* ===================================================================== */
static void H2DInitSurfInt(H2D h, void *adrs) {
	int i; int *v;

	h->data.type = HST_INT;
	h->data.adrs = adrs;
	h->data.under.i = h->data.over.i = 0;
	v = (int *)(h->data.adrs);
	i = (h->x).nb * (h->y).nb; while(i--) *v++ = 0;
	h->lut.dim = 0;
	h->data.histo = 0;
}
/* ..................................................................... */
static void H2DInitSurfFloat(H2D h, void *adrs) {
	int i; float *v;

	h->data.type = HST_FLOAT;
	h->data.adrs = adrs;
	h->data.under.r = h->data.over.r = 0;
	v = (float *)(h->data.adrs);
	i = (h->x).nb * (h->y).nb; while(i--) *v++ = 0.0;
	h->lut.dim = 0;
	h->data.histo = 0;
}
/* ===================================================================== */
H2D H2DCreateIntIntToInt(int xmin, int xmax, int xnb, 
	int ymin, int ymax, int ynb) {
	H2D h; void *adrs; int qual;

	HistoError = 0;
	
	if(!HistoVerifAxeInt(xmin,xmax,xnb)) return(0);
	if(!HistoVerifAxeInt(ymin,ymax,ynb)) return(0);

	h = (H2D)malloc(sizeof(TypeH2D));
	if(!h) { HistoError = HST_MEMORY; return(0); }
	adrs = (void *)malloc(xnb * ynb * sizeof(int));
	if(!adrs) { free(h); HistoError = HST_MEMORY; return(0); }

	HistoInitAxeInt(&(h->x),xmin,xmax,xnb);
	HistoInitAxeInt(&(h->y),ymin,ymax,ynb);
	for(qual=0; qual<WND_NBQUAL; qual++)
		h->axe[qual].r = h->axe[qual].g = h->axe[qual].b = WndLevelText[qual];
	H2DInitSurfInt(h,adrs);

	return(h);
}
/* ===================================================================== */
H2D H2DCreateIntFloatToInt(int xmin, int xmax, int xnb, 
	float ymin, float ymax, int ynb) {
	H2D h; void *adrs; int qual;

	HistoError = 0;
	
	if(!HistoVerifAxeInt(xmin,xmax,xnb)) return(0);
	if(!HistoVerifAxeFloat(ymin,ymax,ynb)) return(0);

	h = (H2D)malloc(sizeof(TypeH2D));
	if(!h) { HistoError = HST_MEMORY; return(0); }
	adrs = (void *)malloc(xnb * ynb * sizeof(int));
	if(!adrs) { free(h); HistoError = HST_MEMORY; return(0); }

	HistoInitAxeInt(&(h->x),xmin,xmax,xnb);
	HistoInitAxeFloat(&(h->y),ymin,ymax,ynb);
	for(qual=0; qual<WND_NBQUAL; qual++)
		h->axe[qual].r = h->axe[qual].g = h->axe[qual].b = WndLevelText[qual];
	H2DInitSurfInt(h,adrs);

	return(h);
}
/* ===================================================================== */
H2D H2DCreateFloatIntToInt(float xmin, float xmax, int xnb, 
	int ymin, int ymax, int ynb) {
	H2D h; void *adrs; int qual;

	HistoError = 0;
	
	if(!HistoVerifAxeFloat(xmin,xmax,xnb)) return(0);
	if(!HistoVerifAxeInt(ymin,ymax,ynb)) return(0);

	h = (H2D)malloc(sizeof(TypeH2D));
	if(!h) { HistoError = HST_MEMORY; return(0); }
	adrs = (void *)malloc(xnb * ynb * sizeof(int));
	if(!adrs) { free(h); HistoError = HST_MEMORY; return(0); }

	HistoInitAxeFloat(&(h->x),xmin,xmax,xnb);
	HistoInitAxeInt(&(h->y),ymin,ymax,ynb);
	for(qual=0; qual<WND_NBQUAL; qual++)
		h->axe[qual].r = h->axe[qual].g = h->axe[qual].b = WndLevelText[qual];
	H2DInitSurfInt(h,adrs);

	return(h);
}
/* ===================================================================== */
H2D H2DCreateFloatFloatToInt(float xmin, float xmax, int xnb, 
	float ymin, float ymax, int ynb) {
	H2D h; void *adrs; int qual;

	HistoError = 0;

	if(!HistoVerifAxeFloat(xmin,xmax,xnb)) return(0);
	if(!HistoVerifAxeFloat(ymin,ymax,ynb)) return(0);

	h = (H2D)malloc(sizeof(TypeH2D));
	if(!h) { HistoError = HST_MEMORY; return(0); }
	adrs = (void *)malloc(xnb * ynb * sizeof(int));
	if(!adrs) { free(h); HistoError = HST_MEMORY; return(0); }

	HistoInitAxeFloat(&(h->x),xmin,xmax,xnb);
	HistoInitAxeFloat(&(h->y),ymin,ymax,ynb);
	// printf("(%s) [%g - %g]/%d x [%g - %g]/%d\n",__func__,xmin,xmax,xnb,ymin,ymax,ynb);
	for(qual=0; qual<WND_NBQUAL; qual++)
		h->axe[qual].r = h->axe[qual].g = h->axe[qual].b = WndLevelText[qual];
	H2DInitSurfInt(h,adrs);

	return(h);
}
/* ===================================================================== */
void H2DDelete(H2D histo) {
	if(!histo) return;
//	printf("(%s) Libere les donnees @%08llX\n",__func__,(IntAdrs)(histo->data.adrs));
	if(histo->data.adrs) free(histo->data.adrs);
//	printf("(%s) Donnees liberees @%08llX, histo @%08llX a effacer\n",__func__,
//		(IntAdrs)(histo->data.adrs),(IntAdrs)histo);
	free(histo);
//	printf("(%s) histo @%08llX efface\n",__func__,(IntAdrs)histo);
}
/* ===================================================================== */
void H2DSupportRGB(H2D histo, int qual,
	WndColorLevel r, WndColorLevel g, WndColorLevel b) {

	if(!histo) return;
	histo->axe[qual].r = r; histo->axe[qual].g = g ; histo->axe[qual].b = b;
}
/* ===================================================================== */
void H2DLUT(H2D histo, WndColor *lut, unsigned short dim) {
	if(histo) { histo->lut.dim = dim; histo->lut.val = lut; }
}
/* ===================================================================== */
void H2DFillIntIntToInt(H2D histo, int xval, int yval, int poids) {
/* <*val> sont du type <histo->*.type>, <poids> du type <data->type> */
	HistoData data; int xbin,ybin,zbin; int *adrs;

	if(!histo) return;
	if(histo->x.type != HST_INT) return;
	if(histo->y.type != HST_INT) return;
	data = &(histo->data);
	if(data->type != HST_INT) return;
	xbin = (xval - histo->x.v.i[0]) / histo->x.v.i[1];
	ybin = (yval - histo->y.v.i[0]) / histo->y.v.i[1];
	zbin = (ybin * histo->x.nb) + xbin;
	if((xbin < 0) || (ybin < 0) ||  (zbin < 0)) data->under.i += poids;
	else if((xbin >= histo->x.nb) || (ybin >= histo->y.nb) || (zbin >= (histo->x.nb * histo->y.nb)))
		data->over.i += poids;
	else { adrs = (int *)data->adrs; adrs[zbin] += poids; }
}
/* ===================================================================== */
void H2DFillIntFloatToInt(H2D histo, int xval, float yval, int poids) {
/* <*val> sont du type <histo->*.type>, <poids> du type <data->type> */
	HistoData data; int xbin,ybin,zbin; int *adrs;

	if(!histo) return;
	if(histo->x.type != HST_INT) return;
	if(histo->y.type != HST_FLOAT) return;
	data = &(histo->data);
	if(data->type != HST_INT) return;
	xbin = (xval - histo->x.v.i[0]) / histo->x.v.i[1];
	ybin = (int)((yval - histo->y.v.r[0]) / histo->y.v.r[1]);
	zbin = (ybin * histo->x.nb) + xbin;
	if((xbin < 0) || (ybin < 0) ||  (zbin < 0)) data->under.i += poids;
	else if((xbin >= histo->x.nb) || (ybin >= histo->y.nb) || (zbin >= (histo->x.nb * histo->y.nb)))
		data->over.i += poids;
	else { adrs = (int *)data->adrs; adrs[zbin] += poids; }
}
/* ===================================================================== */
void H2DFillFloatIntToInt(H2D histo, float xval, int yval, int poids) {
/* <*val> sont du type <histo->*.type>, <poids> du type <data->type> */
	HistoData data; int xbin,ybin,zbin; int *adrs;

	if(!histo) return;
	if(histo->x.type != HST_FLOAT) return;
	if(histo->y.type != HST_INT) return;
	data = &(histo->data);
	if(data->type != HST_INT) return;
	xbin = (int)((xval - histo->x.v.r[0]) / histo->x.v.r[1]);
	ybin = (yval - histo->y.v.i[0]) / histo->y.v.i[1];
	zbin = (ybin * histo->x.nb) + xbin;
	if((xbin < 0) || (ybin < 0) ||  (zbin < 0)) data->under.i += poids;
	else if((xbin >= histo->x.nb) || (ybin >= histo->y.nb) || (zbin >= (histo->x.nb * histo->y.nb)))
		data->over.i += poids;
	else { adrs = (int *)data->adrs; adrs[zbin] += poids; }
}
/* ===================================================================== */
void H2DFillFloatFloatToInt(H2D histo, float xval, float yval, int poids) {
/* <*val> sont du type <histo->*.type>, <poids> du type <data->type> */
	HistoData data; int xbin,ybin,zbin; int *adrs;

	if(!histo) return;
	if(histo->x.type != HST_FLOAT) return;
	if(histo->y.type != HST_FLOAT) return;
	data = &(histo->data);
	if(data->type != HST_INT) return;
	xbin = (int)((xval - histo->x.v.r[0]) / histo->x.v.r[1]);
	ybin = (int)((yval - histo->y.v.r[0]) / histo->y.v.r[1]);
	zbin = (ybin * histo->x.nb) + xbin;
	if((xbin < 0) || (ybin < 0) ||  (zbin < 0)) data->under.i += poids;
	else if((xbin >= histo->x.nb) || (ybin >= histo->y.nb) || (zbin >= (histo->x.nb * histo->y.nb)))
		data->over.i += poids;
	else { adrs = (int *)data->adrs; adrs[zbin] += poids; }
}
/* ===================================================================== */
int H2DLimitsInt(H2D histo, int type_axe, int min, int max) {
	HistoAxe *axe;

	if(!histo) return(0);
	switch(type_axe) {
	  case HST_AXEX: axe = &(histo->x); break;
	  case HST_AXEY: axe = &(histo->y); break;
	  default: return(0);
	}
	if(!HistoVerifAxeInt(min,max,axe->nb)) return(0);
	HistoInitAxeInt(axe,min,max,axe->nb);
	return(1);
}
/* ===================================================================== */
int H2DLimitsFloat(H2D histo, int type_axe, float min, float max) {
	HistoAxe *axe;

	if(!histo) return(0);
	switch(type_axe) {
	  case HST_AXEX: axe = &(histo->x); break;
	  case HST_AXEY: axe = &(histo->y); break;
	  default: return(0);
	}
	if(!HistoVerifAxeFloat(min,max,axe->nb)) return(0);
	HistoInitAxeFloat(axe,min,max,axe->nb);
	return(1);
}
/* ===================================================================== */
int H2DGetExtremaInt(H2D histo, int *min, int *max) {
	int i; int *v;

	if(!histo) return(0);
	if(histo->data.type != HST_INT) return(0);
	v = (int *)(histo->data.adrs);
	*min = *max = *v;
	i = (histo->x).nb * (histo->y).nb;
	while(i--) {
		if(*v < *min) *min = *v;
		if(*v > *max) *max = *v;
		v++;
	}
	return(1);
}
/* ===================================================================== */
int H2DGetExtremaFloat(H2D histo, float *min, float *max) {
	int i; float *v;

	if(!histo) return(0);
	if(histo->data.type != HST_FLOAT) return(0);
	v = (float *)(histo->data.adrs);
	*min = *max = *v;
	i = (histo->x).nb * (histo->y).nb;
	while(i--) {
		if(*v < *min) *min = *v;
		if(*v > *max) *max = *v;
		v++;
	}
	return(1);
}
/* ===================================================================== */
int H2DRebin(H2D histo, int type_axe, int nb) {
	HistoAxe *axe; void *adrs;

	if(!histo) return(0);
	if(nb <= 0) { HistoError = HST_NUMBER; return(0); }
	switch(type_axe) {
	  case HST_AXEX: axe = &(histo->x); break;
	  case HST_AXEY: axe = &(histo->y); break;
	  default: return(0);
	}
	if(nb == axe->nb) return(1);
	free(histo->data.adrs);
	axe->nb = nb;
	adrs = (void *)malloc(histo->x.nb * histo->y.nb * sizeof(int));
	if(!adrs) { HistoError = HST_MEMORY; return(0); }
	switch(histo->data.type) {
	  case HST_INT:   H2DInitSurfInt(histo,adrs);   break;
	  case HST_FLOAT: H2DInitSurfFloat(histo,adrs); break;
	}
	return(1);
}
/* ===================================================================== */
void H2DGetBinInt(H2D histo, int type_axe, int *min, int *bin, int *nb) {
	HistoAxe *axe;

	if(!histo) { *min = *bin = *nb = 0; return; }
	switch(type_axe) {
	  case HST_AXEX: axe = &(histo->x); break;
	  case HST_AXEY: axe = &(histo->y); break;
	  default: return;
	}
	switch(histo->x.type) {
	  case HST_INT:
		*min = axe->v.i[0]; *bin = axe->v.i[1];
		break;
	  case HST_SHORT:
		*min = (int)axe->v.s[0]; *bin = (int)axe->v.s[1];
		break;
	  case HST_FLOAT:
		*min = (int)axe->v.r[0]; *bin = (int)axe->v.r[1];
		break;
	}
	*nb = axe->nb;
	return;
}
/* ===================================================================== */
int H2DGetBinFloat(H2D histo, int type_axe, float *min, float *bin, int *nb) {
	HistoAxe *axe;

	if(!histo) { *min = *bin = *nb = 0; return(0); }
	switch(type_axe) {
	  case HST_AXEX: axe = &(histo->x); break;
	  case HST_AXEY: axe = &(histo->y); break;
	  default: return(0);
	}
	switch(axe->type) {
	  case HST_INT:
		*min = (float)axe->v.i[0]; *bin = (float)axe->v.i[1];
		break;
	  case HST_SHORT:
		*min = (float)axe->v.s[0]; *bin = (float)axe->v.s[1];
		break;
	  case HST_FLOAT:
		*min = axe->v.r[0]; *bin = axe->v.r[1];
		break;
	}
	*nb = axe->nb;
	return(1);
}
/* ===================================================================== */
void H2DRaz(H2D histo) {
	if(histo) switch(histo->data.type) {
	  case HST_INT:   H2DInitSurfInt(histo,histo->data.adrs);   break;
	  case HST_FLOAT: H2DInitSurfFloat(histo,histo->data.adrs); break;
	}
}
/* ===================================================================== */
void H2DDump(H2D histo) {
	HistoData data; int k; int nb; int qual;
	int *i; short *s; float *r;

	if(!histo) { WndPrint("(H2DDump) H2D non defini\n"); return; }
	WndPrint("(H2DDump) H2D @%08lX:\n",(Ulong)histo);
	for(qual=0; qual<WND_NBQUAL; qual++)
		WndPrint("          couleur[%s]=rgb:%04X.%04X.%04X\n",WndSupportNom[qual],histo->axe[qual].r,histo->axe[qual].g,histo->axe[qual].b);
	switch(histo->x.type) {
	  case HST_INT:
		WndPrint("          abcisse  INT  : [%d + %d x %d]\n",
			histo->x.v.i[0],histo->x.nb,histo->x.v.i[1]);
		break;
	  case HST_SHORT:
		WndPrint("          abcisse  SHORT: [%d + %d x %d]\n",
			histo->x.v.s[0],histo->x.nb,histo->x.v.s[1]);
		break;
	  case HST_FLOAT:
		WndPrint("          abcisse  FLOAT: [%g + %d x %g]\n",
			histo->x.v.r[0],histo->x.nb,histo->x.v.r[1]);
		break;
	  default:
		WndPrint("          abscisse non definie\n");
	}
	switch(histo->y.type) {
	  case HST_INT:
		WndPrint("          ordonnee INT  : [%d + %d x %d]\n",
			histo->y.v.i[0],histo->y.nb,histo->y.v.i[1]);
		break;
	  case HST_SHORT:
		WndPrint("          ordonnee SHORT: [%d + %d x %d]\n",
			histo->y.v.s[0],histo->y.nb,histo->y.v.s[1]);
		break;
	  case HST_FLOAT:
		WndPrint("          ordonnee FLOAT: [%f + %d x %f]\n",
			histo->y.v.r[0],histo->y.nb,histo->y.v.r[1]);
		break;
	  default:
		WndPrint("          ordonnee non definie\n");
	}
	WndPrint("axe Y @%08lX\n",(Ulong)&(histo->y));
	data = &(histo->data);
	nb = histo->x.nb * histo->y.nb;
	switch(data->type) {
	  case HST_INT:
		WndPrint("          donnees  INT  @%08lX\n",(Ulong)data->adrs);
		i = (int *)data->adrs;
		WndPrint("               underflow: %12d",data->under.i);
		for(k=0; k<nb; k++) {
			if(!(k % 5)) WndPrint("\n      %6d:  ",k);
			WndPrint("%12d",i[k]);
		}
		WndPrint(" \n");
		WndPrint("                overflow: %12d\n",data->over.i);
		break;
	  case HST_SHORT:
		WndPrint("          donnees SHORT @%08lX\n",(Ulong)data->adrs);
		s = (short *)data->adrs;
		WndPrint("               underflow: %12d",data->under.s);
		for(k=0; k<nb; k++) {
			if(!(k % 5)) WndPrint("\n      %6d:  ",k);
			WndPrint("%12d",s[k]);
		}
		WndPrint(" \n"); 
		WndPrint("                overflow: %12d\n",data->over.s);
		break;
	  case HST_FLOAT:
		WndPrint("          donnees FLOAT @%08lX\n",(Ulong)data->adrs);
		r = (float *)data->adrs;
		WndPrint("               underflow: %12g",data->under.r);
		for(k=0; k<nb; k++) {
			if(!(k % 5)) WndPrint("\n      %6d:  ",k);
			WndPrint("%12g",r[k]);
		}
		WndPrint(" \n");
		WndPrint("                overflow: %12g\n",data->over.r);
		break;
	  default:
		WndPrint("          donnees non definies @%08lX\n",(Ulong)data->adrs);
	}
	WndPrint("(H2DDump) Fin histo @%08lX:\n",(Ulong)histo);
}
/* ===================================================================== */
void H2DGraphAdd(H2D histo, Graph graph) {
	HistoData data; int i,nb; int qual;
	int imin,imax; float rmin,rmax; char reel;

	if(!histo) return;
	if(!graph) return;
	reel = 0;
	switch(histo->x.type) {
	  case HST_INT:
		i = GraphAdd(graph,GRF_XAXIS|GRF_INDEX|GRF_INT,histo->x.v.i,histo->x.nb);
		imin = histo->x.v.i[0]; imax = imin + (histo->x.v.i[1] * histo->x.nb);
		break;
	  case HST_SHORT:
		i = GraphAdd(graph,GRF_XAXIS|GRF_INDEX|GRF_SHORT,histo->x.v.s,histo->x.nb);
		imin = (int)histo->x.v.s[0]; imax = imin + ((int)(histo->x.v.s[1]) * histo->x.nb);
		break;
	  case HST_FLOAT:
		i = GraphAdd(graph,GRF_XAXIS|GRF_INDEX|GRF_FLOAT,histo->x.v.r,histo->x.nb);
		rmin = histo->x.v.r[0]; rmax = rmin + (histo->x.v.r[1] * (float)(histo->x.nb)); reel = 1;
		break;
	  default: i = -1;
	}
	if(i >= 0) GraphAxisDefine(graph,i); else return;
	if(reel) GraphAxisFloatRange(graph,GRF_XAXIS,rmin,rmax);
	else GraphAxisIntRange(graph,GRF_XAXIS,imin,imax);

	for(qual=0; qual<WND_NBQUAL; qual++)
		GraphDataSupportRGB(graph,i,qual,histo->axe[qual].r,histo->axe[qual].g,histo->axe[qual].b);

	reel = 0;
	switch(histo->y.type) {
	  case HST_INT:
		i = GraphAdd(graph,GRF_YAXIS|GRF_INDEX|GRF_INT,histo->y.v.i,histo->y.nb);
		imin = histo->y.v.i[0]; imax = imin + (histo->y.v.i[1] * histo->y.nb);
		break;
	  case HST_SHORT:
		i = GraphAdd(graph,GRF_YAXIS|GRF_INDEX|GRF_SHORT,histo->y.v.s,histo->y.nb);
		imin = (int)histo->y.v.s[0]; imax = imin + ((int)(histo->y.v.s[1]) * histo->y.nb);
		break;
	  case HST_FLOAT:
		i = GraphAdd(graph,GRF_YAXIS|GRF_INDEX|GRF_FLOAT,histo->y.v.r,histo->y.nb);
		rmin = histo->y.v.r[0]; rmax = rmin + (histo->y.v.r[1] * (float)(histo->y.nb)); reel = 1;
		break;
	  default: i = -1;
	}
	if(i >= 0) { GraphAxisDefine(graph,i); GraphDataTranslate(graph,i,GRF_NODSP); } else return;
	if(reel) GraphAxisFloatRange(graph,GRF_YAXIS,rmin,rmax);
	else GraphAxisIntRange(graph,GRF_YAXIS,imin,imax);

	nb = (histo->x).nb * (histo->y).nb;
	data = &(histo->data);
	switch(data->type) {
	  case HST_INT:
		i = GraphAdd(graph,GRF_ZAXIS|GRF_INT,data->adrs,nb);
		break;
	  case HST_SHORT:
		i = GraphAdd(graph,GRF_ZAXIS|GRF_SHORT,data->adrs,nb);
		break;
	  case HST_FLOAT:
		i = GraphAdd(graph,GRF_ZAXIS|GRF_FLOAT,data->adrs,nb);
		break;
	  default: i = -1;
	}
	GraphDataLUT(graph,i,histo->lut.val,histo->lut.dim);
}

