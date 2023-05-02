#define CABLAGE_C
#include <environnement.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>

#include <utils/decode.h>

#include <samba.h>
#include <cablage.h>

#define CABLAGE_INDIQUE_V2 "Version = 2"

static char CablageCodeConnecteur[8];
static char CablageCodePosition[8];
static char CablageFichierModeles[MAXFILENAME];

static char CablageModlV2;

static ArgDesc CablageModlDesc[] = {
	{ "Cablages", DESC_STRUCT_SIZE(ModeleCableNb,CABLAGE_TYPE_MAX) &ModeleCablageAS,  0 },
	DESC_END
};

/* ========================================================================== */
INLINE void CablageAnalysePosition(TypeCablePosition pos_hexa, short *galette, short *tour, short *branche) {
#ifdef CABLAGE_CHAMPS
	*galette = CABLAGE_POSX(pos_hexa); *tour = CABLAGE_POSY(pos_hexa);
	#ifdef AVEC_POSZ
		*branche = CABLAGE_POSZ(pos_hexa);
	#else
		*branche = 0;
	#endif
#else /* !CABLAGE_CHAMPS */
	short position[CHASSIS_DIMMAX];
	SambaAnalysePosition((short)pos_hexa,ChassisDetec,ChassisDetecDim,position);
	*galette = position[0];
	*tour = position[1];
	*branche = position[2];
#endif /* !CABLAGE_CHAMPS */
	//- printf("(%s) Position codee %04X soit branche %d, galette %d, tour %d\n",__func__,pos_hexa,*branche,*galette,*tour);
}
/* ========================================================================== */
static TypeCablePosition CablageSynthetisePosition(TypeCablePosition *position, TypeChassisAxe *direction, int dim) {
#ifdef CABLAGE_CHAMPS
	#ifdef AVEC_POSZ
		return((TypeCablePosition)((position[2] & CABLAGE_POSZ_MAX) << CABLAGE_POSZ_SHIFT) | CABLAGE_POSITION(position[0],position[1]));
	#else
		return((TypeCablePosition)CABLAGE_POSITION(position[0],position[1]));
	#endif
#else /* !CABLAGE_CHAMPS */
	TypeCablePosition place; int i;
	place = 0;
	for(i=0; i<dim; i++) {
		if(i) place *= direction[i].max;
		place += position[i];
	}
	return(place);
#endif /* !CABLAGE_CHAMPS */
}
/* ========================================================================== */
TypeCablePosition CablageDecodePosition(char *code) {
	TypeCablePosition pos_hexa;

// #define CODE_CABLAGE_A_LA_MAIN
#ifdef CODE_CABLAGE_A_LA_MAIN
	short galette,tour,branche; size_t l; int pos;

	if(!code || !code[0] || !strcmp(code,"---") || !strcmp(code,"ext")) return(CABLAGE_POS_NOTDEF);
	if((code[0] >= 'a') && (code[0] <= 'z')) galette = code[0] - 'a' + 1;
	else if((code[0] >= 'A') && (code[0] <= 'Z')) galette = code[0] - 'A' + 1;
	else { sscanf(code,"%d",&pos); return((TypeCablePosition)(pos - 1)); /* pos-1 parce que tour-- */ }
	if((galette < 0) || (galette > CABLAGE_POSX_MAX)) galette = CABLAGE_POSX_MAX;
	sscanf(code+1,"%hd",&tour); tour--;
	if((tour < 0) || (tour > CABLAGE_POSY_MAX)) tour = CABLAGE_POSY_MAX;
	l = strlen(code) - 1;
	if((code[l] >= 'a') && (code[l] <= 'c')) branche = code[l] - 'a' + 1;
	else branche = 0;
	/* Dans l'ordre inverse, les positions seraient classees par proximite et en tours
		mais alors le "cablage indefini" devrait etre code differement */
	pos_hexa = ((branche & 0x3) << 8) | CABLAGE_POSITION(galette,tour);
	// printf("(%s) Position %s codee %03X car branche %d, galette %d, tour %d\n",__func__,code,pos_hexa,branche,galette,tour);
	return(pos_hexa);
#else
	TypeCablePosition position[CHASSIS_DIMMAX];

	if(!code || !code[0] || !strcmp(code,"---") || !strcmp(code,"ext")) return(CABLAGE_POS_NOTDEF);
	else if(!strcmp(code,".")) return(CABLAGE_POS_DIRECT);
	int i; for(i=0; i<CHASSIS_DIMMAX; i++) position[i] = 0;
	SambaPositionDecode(ChassisDetec,ChassisDetecDim,code,position);
	pos_hexa = CablageSynthetisePosition(position,ChassisDetec,ChassisDetecDim);

/*	printf("(%s) Position lue: '%s', position=[",__func__,code);
	for(i=0; i<ChassisDetecDim; i++) printf("%s%d",i?" ":"",position[i]);
	printf("], code 0x%02X (=%d)\n",pos_hexa,pos_hexa); */ 

	return(pos_hexa);
#endif
}
/* ========================================================================== */
char *CablageEncodePosition(TypeCablePosition pos_hexa) {

	if(pos_hexa == CABLAGE_POS_NOTDEF) strcpy(CablageCodePosition,"---");
	else if(pos_hexa == CABLAGE_POS_DIRECT) strcpy(CablageCodePosition,".");
	else {
		short position[CHASSIS_DIMMAX];
		CablageAnalysePosition(pos_hexa,&(position[0]),&(position[1]),&(position[2]));
		// printf("(%s) Position 0x%02X analysee en %d, %d, %d\n",__func__,pos_hexa,position[0],position[1],position[2]);
		SambaPositionEncode(ChassisDetec,ChassisDetecDim,position,CablageCodePosition);
		// printf("(%s) Encodee en %s\n",__func__,CablageCodePosition);
	}
	return(CablageCodePosition);
}
/* ========================================================================== */
char *CablageEncodeCoord(TypeCablePosition pos_hexa) {

	if(pos_hexa == CABLAGE_POS_NOTDEF) strcpy(CablageCodePosition,"---");
	else if(pos_hexa == CABLAGE_POS_DIRECT) strcpy(CablageCodePosition,".");
	else {
		short position[CHASSIS_DIMMAX];
		CablageAnalysePosition(pos_hexa,&(position[0]),&(position[1]),&(position[2]));
		SambaCoordEncode(ChassisDetec,ChassisDetecDim,position,CablageCodePosition,0);
	}
	return(CablageCodePosition);
}
/* ========================================================================== */
INLINE void CablageAnalyseConnecteur(TypeCableConnecteur connecteur, short *planche, short *distance) {
#ifdef CODE_CABLAGE_A_LA_MAIN
	if(connecteur > 0) {
		*planche = (connecteur - 1) / QUADRANT_OF7;
		*distance = (connecteur - 1) - (*quadrant * QUADRANT_OF7);
	} else *planche = *distance = 0;
#else  /* CODE_CABLAGE_A_LA_MAIN */
	short position[CHASSIS_DIMMAX];
	SambaAnalysePosition((short)connecteur,ChassisNumer,ChassisNumerDim,position);
	if(ChassisNumerDim == 2) { *planche = position[0]; *distance = position[1]; }
	else { *planche = 0; *distance = position[0]; }
#endif /*CODE_CABLAGE_A_LA_MAIN */
}
/* ========================================================================== */
static TypeCableConnecteur CablageSynthetiseConnecteur(TypeCablePosition *position, TypeChassisAxe *direction, int dim) {
// 	return(CABLAGE_CONNECTEUR(position[1],position[0]));
	TypeCableConnecteur connecteur; int i;

	connecteur = 0;
	for(i=0; i<dim; i++) {
		if(i) connecteur *= direction[i].max;
		connecteur += position[i];
	}
	return(connecteur);
}
/* ========================================================================== */
TypeCableConnecteur CablageDecodeConnecteur(char *code) {
	TypeCableConnecteur connecteur;
#ifdef CODE_CABLAGE_A_LA_MAIN
	size_t i,l,m;
	short numero;
	TypeCableConnecteur serie;
	char c; char hb,gd;

	if(!strcmp(code,CABLAGE_TXTNUL) || !strcmp(code,"ext") || !code[0]) return(0);
	else if(!strcmp(code,CABLAGE_TXTDIR)) return(CABLAGE_CONN_DIR);
	hb = gd = 0; serie = 0;  /* par defaut pour H et G */
	l = strlen(code);
	m = 2; if(m > l) m = l;
	for(i=0; i<m; i++) {
		c = code[i];
		if((c == 'h') || (c == 'H')) hb = 1;
		else if((c == 'g') || (c == 'G')) gd = 1;
		else if((c == 'b') || (c == 'B')) { if(!hb) serie += QUADRANT_OF7; hb = 1; }
		else if((c == 'd') || (c == 'D')) { if(!gd) serie += (2 * QUADRANT_OF7); gd = 1; }
		else break;
	}
	sscanf(code + i,"%hd",&numero);
	connecteur = (TypeCableConnecteur)numero + serie;
	return(connecteur);
#else  /* CODE_CABLAGE_A_LA_MAIN */
	TypeCablePosition position[CHASSIS_DIMMAX];

	if(!strcmp(code,CABLAGE_TXTNUL) || !strcmp(code,"ext") || !code[0]) return(0);
	else if(!strcmp(code,CABLAGE_TXTDIR)) return(CABLAGE_CONN_DIR);
	SambaPositionDecode(ChassisNumer,ChassisNumerDim,code,position);
	connecteur = CablageSynthetiseConnecteur(position,ChassisNumer,ChassisNumerDim);
	/*
	int j;
	printf("(%s) Connecteur lu: '%s', position=[",__func__,code);
	for(j=0; j<ChassisNumerDim; j++) printf("%s%d",j?" ":"",position[j]);
	printf("], code 0x%02X = %d\n",connecteur,connecteur);
	*/
	return(connecteur);
#endif /*CODE_CABLAGE_A_LA_MAIN */
}
/* ========================================================================== */
char *CablageEncodeConnecteur(TypeCableConnecteur connecteur) {

	if(connecteur == CABLAGE_CONN_DIR) strcpy(CablageCodeConnecteur,CABLAGE_TXTDIR);
	else if(connecteur) {
#ifdef CODE_CABLAGE_A_LA_MAIN
	#ifdef CONNECTE_LETTRE
		int index,quadrant,distance;
		index = connecteur - 1;
		quadrant = index / QUADRANT_OF7; distance = index - (quadrant * QUADRANT_OF7);
		bas = quadrant % 2;
		droite = (quadrant > 1);
		if(bas || droite)
			sprintf(CablageCodeConnecteur,"%c%c%02d",bas?'b':'h',droite?'d':'g',distance+1);
		else 
	#else  /* CONNECTE_LETTRE */
		sprintf(CablageCodeConnecteur,"%02d",connecteur);
	#endif /* CONNECTE_LETTRE */
#else  /* CODE_CABLAGE_A_LA_MAIN */
		short position[CHASSIS_DIMMAX];
//		printf("(%s) Connecteur 0x%02d",__func__,connecteur);
		CablageAnalyseConnecteur(connecteur,&(position[1]),&(position[0]));
		SambaPositionEncode(ChassisNumer,ChassisNumerDim,position,CablageCodeConnecteur);
#endif /*CODE_CABLAGE_A_LA_MAIN */
	} else strcpy(CablageCodeConnecteur,CABLAGE_TXTNUL);
	return(CablageCodeConnecteur);
}
#ifdef CABLAGE_DICO
/* ========================================================================== */
static char CablageCreeDico() {
	int d,i,n;
	Dico cablage; DicoLexique terme;

	if((d = CablageDicoEntreeNb) >= CABLAGE_DICO_MAX) {
		printf("  ! Trop de types de cablage (CABLAGE_DICO_MAX=%d). Type %s ignore\n",
			CablageDicoEntreeNb,SambaDicoTempo.nom);
		return(0);
	}
	cablage = CablageDicoEntree + d;
	terme = DicoInit(cablage,SambaDicoTempo.nom,SambaDicoTempo.nbtermes);
	for(i=0; i<SambaDicoTempo.nbtermes; i++) {
		DicoAjouteTerme(terme++,SambaDicoTempo.terme[i].accepte,SambaDicoTempo.terme[i].officiel);
	}
	CablageDicoEntreeNb++;
	n = 0;
	printf("  . Cablage de type '%s':\n",cablage->nom);
	for(i=0; i<cablage->nbtermes; i++) {
		if(cablage->terme[i].officiel[0] == '\0')
			printf("    | %s n'est pas branche,\n",cablage->terme[i].accepte);
		else if(!strcmp(cablage->terme[i].officiel,"neant"))
			printf("    | %s est debranche,\n",cablage->terme[i].accepte);
		else {
			printf("    | %s est relie a %s,\n",cablage->terme[i].accepte,cablage->terme[i].officiel);
			n++;
		}
	}
	printf("    | soit %d broches pour %d reglages.\n",n,cablage->nbtermes);
	SambaDicoTempo.nbtermes = 0;
	return(1);
}
/* ========================================================================== */
static void CablageCreeDicoNTD() {
// #ifdef DETEC_NTD ne marche pas: DETEC_NTD pas considere ici comme "def"
#ifndef VOIES_STYLE_UNIQUE
	int i,n;
	Dico cablage; DicoLexique terme;

	cablage = &(CablageDicoEntree[0]);
	n = ModeleDet[DETEC_NTD].nbregl;
	terme = DicoInit(cablage,"NTD-BB1",n);
	DicoAjouteTerme(terme++,"d2","D2");
	DicoAjouteTerme(terme++,"d3","D3");
	DicoAjouteTerme(terme++,"ampl-modul","DAC1");
	DicoAjouteTerme(terme++,"comp-modul","DAC2");
	DicoAjouteTerme(terme++,"corr-trngl","DAC3");
	DicoAjouteTerme(terme++,"corr-pied","DAC4");
	DicoAjouteTerme(terme++,"polar-centre","DAC5");
	DicoAjouteTerme(terme++,"polar-garde","DAC6");
	DicoAjouteTerme(terme++,"gain-chaleur","gain-v1");
	DicoAjouteTerme(terme++,"gain-centre","gain-v2");
	DicoAjouteTerme(terme++,"gain-garde","gain-v3");
	DicoAjouteTerme(terme++,"polar-fet","cntl-v1");
	DicoAjouteTerme(terme++,"chauffe-fet","DAC7");
	DicoAjouteTerme(terme++,"regul-bolo","DAC8");
	CablageDicoEntreeNb = 1;
	printf("  . Cablage par defaut, de type '%s':\n",cablage->nom);
	for(i=0; i<cablage->nbtermes; i++) {
		printf("    | %s est relie a %s,\n",cablage->terme[i].accepte,cablage->terme[i].officiel);
	}
	printf("    | soit %d broches.\n",cablage->nbtermes);
#endif
}
#endif /* CABLAGE_DICO */
/* ========================================================================== */
static void CablageModlInit(int num, char *nom) {
	int n;
	
	strncpy(ModeleCable[num].nom,nom,CABLAGE_MODL_NOM);
	ModeleCable[num].num = num;
	ModeleCable[num].modl_bolo = 0;
	ModeleCable[num].nb_numer = 1;
	for(n=0; n<DETEC_CAPT_MAX; n++) ModeleCable[num].modl_numer[n] = 0;
	ModeleCable[num].nb_capt = 2;
	for(n=0; n<DETEC_CAPT_MAX; n++) {
		sprintf(ModeleCable[num].capteur[n].nom,"Capteur%c",'A'+n);
		ModeleCable[num].capteur[n].gain = CABLAGE_DEFAUT_GAIN;
		ModeleCable[num].capteur[n].capa = CABLAGE_DEFAUT_CAPA;
		ModeleCable[num].capteur[n].rc   = CABLAGE_DEFAUT_RC;
		ModeleCable[num].capteur[n].numer_declare = 1;
		ModeleCable[num].capteur[n].numer_adc = 1;
	}
	ModeleCable[num].nbconnections = 0;
}
#ifdef CABLAGE_STD_NTD
/* ========================================================================== */
static void CablageStandardNTD() {
	TypeCablageConnectRegl *connection; int n;

	CablageModlInit(0,"NTD-BB1");
	connection = &(ModeleCable[0].connection[0]);
	strcpy(connection->reglage,"d2");           strcpy(connection->ressource,"D2");      connection++;
	strcpy(connection->reglage,"d3");           strcpy(connection->ressource,"D3");      connection++;
	strcpy(connection->reglage,"ampl-modul");   strcpy(connection->ressource,"DAC1");    connection++;
	strcpy(connection->reglage,"comp-modul");   strcpy(connection->ressource,"DAC2");    connection++;
	strcpy(connection->reglage,"corr-trngl");   strcpy(connection->ressource,"DAC3");    connection++;
	strcpy(connection->reglage,"corr-pied");    strcpy(connection->ressource,"DAC4");    connection++;
	strcpy(connection->reglage,"polar-centre"); strcpy(connection->ressource,"DAC5");    connection++;
	strcpy(connection->reglage,"polar-garde");  strcpy(connection->ressource,"DAC6");    connection++;
	strcpy(connection->reglage,"gain-chaleur"); strcpy(connection->ressource,"gain-v1"); connection++;
	strcpy(connection->reglage,"gain-centre");  strcpy(connection->ressource,"gain-v2"); connection++;
	strcpy(connection->reglage,"gain-garde");   strcpy(connection->ressource,"gain-v3"); connection++;
	strcpy(connection->reglage,"polar-fet");    strcpy(connection->ressource,"cntl-v1"); connection++;
	// strcpy(connection->reglage,"chauffe-fet");  strcpy(connection->ressource,"DAC7");    connection++;
	// strcpy(connection->reglage,"regul-bolo");   strcpy(connection->ressource,"DAC8");    connection++;
	ModeleCable[0].nbconnections = connection - ModeleCable[0].connection;
	for(n=0; n<ModeleCable[0].nbconnections; n++) ModeleCable[0].connection[n].valeurs = 0;
	ModeleCableNb = 1;
}
#else
/* ========================================================================== */
static void CablageStandard() {
	TypeCablageConnectRegl *connection; int n;
	
	CablageModlInit(0,"FID-BB2");
	connection = &(ModeleCable[0].connection[0]);
	strcpy(connection->reglage,"d2");            strcpy(connection->ressource,"D2");           connection++;
	strcpy(connection->reglage,"d3");            strcpy(connection->ressource,"D3");           connection++;
	strcpy(connection->reglage,"ampl-modul");    strcpy(connection->ressource,"ampl-tri-d11"); connection++;
	strcpy(connection->reglage,"-ampl-modul");   strcpy(connection->ressource,"ampl-tri-d12"); connection++;
	strcpy(connection->reglage,"comp-modul");    strcpy(connection->ressource,"comp-car-v5"); connection++;
	strcpy(connection->reglage,"-comp-modul");   strcpy(connection->ressource,"comp-car-v6"); connection++;
	strcpy(connection->reglage,"corr-trngl");    strcpy(connection->ressource,"ampl-car-d11"); connection++;
	strcpy(connection->reglage,"-corr-trngl");   strcpy(connection->ressource,"ampl-car-d12"); connection++;
	strcpy(connection->reglage,"gain-chaleur");  strcpy(connection->ressource,"gain-v5");      connection++;
	strcpy(connection->reglage,"+gain-chaleur"); strcpy(connection->ressource,"gain-v6");      connection++;
	strcpy(connection->reglage,"polar-A");       strcpy(connection->ressource,"DAC2");         connection++;
	strcpy(connection->reglage,"polar-B");       strcpy(connection->ressource,"DAC4");         connection++;
	strcpy(connection->reglage,"polar-C");       strcpy(connection->ressource,"DAC6");         connection++;
	strcpy(connection->reglage,"polar-D");       strcpy(connection->ressource,"DAC8");         connection++;
	strcpy(connection->reglage,"gain-A");        strcpy(connection->ressource,"gain-v1");      connection++;
	strcpy(connection->reglage,"gain-B");        strcpy(connection->ressource,"gain-v2");      connection++;
	strcpy(connection->reglage,"gain-C");        strcpy(connection->ressource,"gain-v3");      connection++;
	strcpy(connection->reglage,"gain-D");        strcpy(connection->ressource,"gain-v4");      connection++;
	strcpy(connection->reglage,"polar-fet-A");   strcpy(connection->ressource,"DAC1");         connection++;
	strcpy(connection->reglage,"polar-fet-B");   strcpy(connection->ressource,"DAC3");         connection++;
	strcpy(connection->reglage,"polar-fet-C");   strcpy(connection->ressource,"DAC5");         connection++;
	strcpy(connection->reglage,"polar-fet-D");   strcpy(connection->ressource,"DAC7");         connection++;
	ModeleCable[0].nbconnections = (int)(connection - ModeleCable[0].connection);
	for(n=0; n<ModeleCable[0].nbconnections; n++) ModeleCable[0].connection[n].valeurs = 0;

	strcpy(ModeleCable[1].nom,"direct"); ModeleCable[1].nbconnections = 0;

	ModeleCableNb = 2;
}
#endif
/* ========================================================================== */
static void CablageCree(int type, TypeCablePosition pos_hexa, TypeCableConnecteur connecteur) {
	int capteur;
	
	CablageSortant[pos_hexa].defini = 1;
	CablageSortant[pos_hexa].type = type;
	CablageSortant[pos_hexa].nbcapt = 1;
	for(capteur=0; capteur<CablageSortant[pos_hexa].nbcapt; capteur++) {
		CablageSortant[pos_hexa].captr[capteur].connecteur = connecteur;
		CablageSortant[pos_hexa].captr[capteur].vbb = capteur;
		CablageSortant[pos_hexa].captr[capteur].gain = CABLAGE_DEFAUT_GAIN;
		CablageSortant[pos_hexa].captr[capteur].capa = CABLAGE_DEFAUT_CAPA;
		CablageSortant[pos_hexa].captr[capteur].rc   = CABLAGE_DEFAUT_RC;
	}
}
/* ========================================================================== */
static char CablageModlDecode(FILE *h, char inclus, char log) {
	TypeCablageConnectRegl *connection;
	char ligne[256],*r,*item,s;
	char nouveau_modele,definition_en_cours;
	int i,j,n,p,adc,num; int nb_erreurs;
	TypeDetModele *modele_det;

	nb_erreurs = 0;
	nouveau_modele = 0; definition_en_cours = 0; // pour la 1ere version
	while(LigneSuivante(ligne,256,h)) {
		if((ligne[0] == '#') || (ligne[0] == '\n')) continue;
		if(!strncmp(ligne,CABLAGE_INDIQUE_V2,11)) {
			CablageModlV2 = 1;
			// ArgDebug(3);
			ArgFromFile(h,CablageModlDesc,inclus? "}": 0);
			// ArgDebug(0);
			// printf("  > Modeles de cablage:\n"); ArgPrint("*",CablageModlDesc);
			for(i=0; i<ModeleCableNb; i++) {
				modele_det = &(ModeleDet[ModeleCable[i].modl_bolo]);
				ModeleCable[i].max_adc = modele_det->nbcapt;
				for(j=0; j<modele_det->nbcapt; j++) {
					ModeleCable[i].capteur[j].numer_index = 0;
					ModeleCable[i].capteur[j].numer_adc = 1;
				}
				for(n=0; n<ModeleCable[i].nb_capt; n++) {
					for(j=0; j<modele_det->nbcapt; j++) 
						if(!strcmp(ModeleVoie[modele_det->type[j]].nom,ModeleCable[i].capteur[n].nom)) break;
					if(j < modele_det->nbcapt) {
						r = ModeleCable[i].capteur[n].adc;
						if(*r) sscanf(ItemJusquA('.',&r),"%d",&num); else num = 1;
						if(num <= 0) {
							printf("  ! Cablage %s: la connection du capteur %s est illegale: %d\n",
								ModeleCable[i].nom,ModeleCable[i].capteur[n].nom,num);
							nb_erreurs++;
							continue;
						} else if(num > ModeleCable[i].nb_numer) {
							printf("  ! Cablage %s: le capteur %s est connecte sur le numeriseur #%d, et il n'y en a que %d prevu%s\n",
								ModeleCable[i].nom,ModeleCable[i].capteur[n].nom,num,Accord1s(ModeleCable[i].nb_numer));
							nb_erreurs++;
							continue;
						}
						ModeleCable[i].capteur[n].numer_declare = num;
						ModeleCable[i].capteur[j].numer_index = ModeleCable[i].capteur[n].numer_declare - 1;
						if(*r) sscanf(r,"%d",&adc); else {
							adc = 1;
							for(p=0; p<n; p++) if(ModeleCable[i].capteur[p].numer_declare == num) adc++;
						}
						ModeleCable[i].capteur[j].numer_adc = adc;
						//- printf("  > ModeleCable[%d].capteur[%d].numer_adc = %d\n",i,j,adc);
					} else {
						printf("  ! Cablage %s: le capteur %s est inconnu pour le detecteur %s\n",
							ModeleCable[i].nom,ModeleCable[i].capteur[n].nom,modele_det->nom);
						nb_erreurs++;
					}
				}
			}
			return(0);
		}
		r = ligne;
		if(*r == '=') {
			r++;
			item = ItemSuivant(&r,' ');
			nouveau_modele = 1;
		} else if(definition_en_cours) {
			if(*r == '}') {
				ModeleCableNb++;
				definition_en_cours = 0;
			} else if(ModeleCableNb < CABLAGE_TYPE_MAX) /* sinon: on a deja proteste, maintenant on saute juste la definition */ {
				do {
					n = ModeleCable[ModeleCableNb].nbconnections;
					if(n < DETEC_REGL_MAX) {
						connection = &(ModeleCable[ModeleCableNb].connection[n]);
						item = ItemJusquA(':',&r); if(!item[0]) break;
						strncpy(connection->reglage,item,DETEC_REGL_NOM);
						if(*r == '=') r++;
						item = ItemAvant("=,",&s,&r);
						strncpy(connection->ressource,item,NUMER_RESS_NOM);
						if(s == '=') {
							item = ItemJusquA(',',&r);
							if(item[0] != 0) {
								connection->valeurs = (char *)malloc(strlen(item)+1);
								strcpy(connection->valeurs,item);
							} else connection->valeurs = 0;
						} else connection->valeurs = 0;
						ModeleCable[ModeleCableNb].nbconnections += 1;
					} else {
						printf("  ! Deja %d connection lues! Connections suivantes abandonnees\n",ModeleCable[ModeleCableNb].nbconnections);
						++nb_erreurs;
					}
				} while(*r != '\0');
			}
		} else {
			item = ItemSuivant(&r,'=');
			if(item[0]) nouveau_modele = 1; else break;
		}
		if(nouveau_modele) {
			if(ModeleCableNb < CABLAGE_TYPE_MAX) {
				CablageModlInit(ModeleCableNb,item);
				if(log) printf("  . Modele de cablage #%d: %s\n",ModeleCableNb+1,ModeleCable[ModeleCableNb].nom);
			} else {
				printf("  ! Trop de types de cablage (CABLAGE_TYPE_MAX=%d). Type %s ignore.\n",CABLAGE_TYPE_MAX,item);
				++nb_erreurs;
			}
			nouveau_modele = 0; definition_en_cours = 1;
		}
	}
	return(nb_erreurs);
}
/* ========================================================================== */
void CablageModlLit(char *fichier) {
	char auxiliaire[MAXFILENAME]; char log;
	FILE *f;

	log = 0;
	if(fichier) RepertoireIdentique(CablageFichierLu,fichier,auxiliaire);
	else strcpy(auxiliaire,FichierModlCablage);
	f = fopen(auxiliaire,"r");
	if(f) {
		if(SambaLogDemarrage) SambaLogDef(". Lecture des modeles de cablage","dans",auxiliaire);
		CablageModlDecode(f,0,log);
		fclose(f);
		if(log) { printf("  . Modeles utilisables:\n"); CablageModlEcrit(stdout); }
	} else {
		printf("  ! Fichier %s inutilisable (%s). Modeles standard utilises\n",auxiliaire,strerror(errno));
	}
}
/* ========================================================================== */
void CablageModlEcrit(FILE *f) {
	if(CablageModlV2) {
		int i,j;
		for(i=0; i<ModeleCableNb; i++) {
			TypeDetModele *modele_det;
			modele_det = &(ModeleDet[ModeleCable[i].modl_bolo]);
			ModeleCable[i].nb_capt = modele_det->nbcapt;
			for(j=0; j<modele_det->nbcapt; j++) {
				ModeleCable[i].capteur[j].numer_declare = ModeleCable[i].capteur[j].numer_index + 1;
				strcpy(ModeleCable[i].capteur[j].nom,ModeleVoie[modele_det->type[j]].nom);
				sprintf(ModeleCable[i].capteur[j].adc,"%d.%d",ModeleCable[i].capteur[j].numer_declare,ModeleCable[i].capteur[j].numer_adc);
			}
		}
		fprintf(f,"%s\n",CABLAGE_INDIQUE_V2);
		ArgAppend(f,0,CablageModlDesc);
	} else {
		int k;
		for(k=0; k<ModeleCableNb; k++) {
			int l,n;
			TypeCablageConnectRegl *connection;
			fprintf(f,"#\n%s = {",ModeleCable[k].nom);
			connection = &(ModeleCable[k].connection[0]);
			l = 80;
			for(n=0; n<ModeleCable[k].nbconnections; n++,connection++) {
				if(l < 72) l += fprintf(f,", "); else { if(n) fprintf(f,","); fprintf(f,"\n\t"); l = 8; }
				l += fprintf(f,"%s: %s",connection->reglage,connection->ressource);
				if(connection->valeurs) {
					char *c;
					c = connection->valeurs;
					while(*c) { if(*c == ',') break; else c++; }
					if(*c) l += fprintf(f,"=\"%s\"",connection->valeurs);
					else l += fprintf(f,"=%s",connection->valeurs);
				}
			}
			fprintf(f,"\n}\n");
		}
		fprintf(f,"#\n");
	}
}
/* ========================================================================== */
void CablageModlSauve(char *fichier) {
	FILE *f;

	if((f = fopen(fichier,"w"))) {
		CablageModlEcrit(f);
		fclose(f);
		printf("* Modeles de cablages sauves sur %s\n",fichier);
		SambaNote("Modeles de cablages sauves sur %s\n",fichier);
	} else {
		OpiumError("Sauvegarde des modeles de cablages sur %s impossible (%s)\n",fichier,strerror(errno));
	}
}
/* ========================================================================== */
void CablageEcrit(char *fichier) {
	FILE *f; int k,n;
	int capteur,rang_bb; char ecrit_tous,ecrit_calib,ecrit_capa;
	TypeCablePosition pos_hexa;
	TypeCablageConnectRegl *connection;

	if((f = fopen(fichier,"w"))) {
		printf("\n= Creation du fichier de cablage '%s' (detecteur <> numeriseur)\n",fichier);
		fprintf(f,"# Description du cablage froid (detecteur <> numeriseur)\n");
		fprintf(f,"#\n");
		fprintf(f,"# Syntaxe: { '=' <definition> | <cablage_reel_v1> | <cablage_reel_v2> }\n");
		fprintf(f,"# <definition>:  <type> '{' // { <reglage-detecteur> ':' <ressource_numeriseur> // ...} '}'\n");
		fprintf(f,"#\n");
		fprintf(f,"# <cablage_reel_v1>: [ [<nom>]['/'<type>] ':' ] <position> '>' { <connecteur> [ '.' { <voie> | '*' } ] [ 'x' <gain_V/pC> [ '/' <capa_pF> ] ] [','] }\n");
		fprintf(f,"#\n");
		fprintf(f,"# <cablage_reel_v2>: <position> '<' [<nom>'/'][<type>] '>' { <connecteur> [ { '.' <entree> | '*' [ 'x' <gain_V/pC> [ '/' <capa_pF> ] ] } ] [','] }\n");
		fprintf(f,"# <gain_V/pC> par defaut: %g, <capa_pF> par defaut: %g, <rc> par defaut: %g\n",CABLAGE_DEFAUT_GAIN,CABLAGE_DEFAUT_CAPA,CABLAGE_DEFAUT_RC);
		fprintf(f,"#\n");
		if(CablageFichierModeles[0]) {
			if(CablageFichierModeles[0] == '*') {
				for(k=0; k<ModeleCableNb; k++) {
					fprintf(f,"#\n=%s {\n",ModeleCable[k].nom);
					connection = &(ModeleCable[k].connection[0]);
					for(n=0; n<ModeleCable[k].nbconnections; n++,connection++) fprintf(f,"\t%s: %s\n",connection->reglage,connection->ressource);
					fprintf(f,"}\n");
				}
				fprintf(f,"#\n");
			} else fprintf(f,"modeles @%s\n#\n",CablageFichierModeles);
		}
        ecrit_calib = 1;
 		// for(pos_hexa=0; pos_hexa<CABLAGE_POS_MAX; pos_hexa++) CablageSortant[pos_hexa].v2 = 1; seulement les cablages neufs, pour l'instant
		
		for(pos_hexa=0; pos_hexa<CABLAGE_POS_MAX; pos_hexa++) if(CablageSortant[pos_hexa].defini) {
			/* for(capteur=0; capteur<CablageSortant[pos_hexa].nbcapt; capteur++) if(CablageSortant[pos_hexa].captr[capteur].connecteur != CABLAGE_CONN_DIR) break;
			 if(capteur >= CablageSortant[pos_hexa].nbcapt) continue; */
			if(CablageSortant[pos_hexa].v2) {
				fprintf(f,"%s < ",CablageEncodePosition(pos_hexa));
				if(strncmp(CablageSortant[pos_hexa].nom,"Cable#",6)) fprintf(f,"%s/",CablageSortant[pos_hexa].nom);
				fprintf(f,"%s > ",ModeleCable[CablageSortant[pos_hexa].type].nom);
				for(rang_bb=0; rang_bb < ModeleCable[CablageSortant[pos_hexa].type].nb_numer; rang_bb++) {
					if(rang_bb) fprintf(f,", ");
					fprintf(f,"%d",CablageSortant[pos_hexa].fischer[rang_bb]);
				}
				fprintf(f,"\n");
			} else {
				if(strncmp(CablageSortant[pos_hexa].nom,"Cable#",6)) fprintf(f,"%s",CablageSortant[pos_hexa].nom);
				fprintf(f,"/%s: %s > ",ModeleCable[CablageSortant[pos_hexa].type].nom,CablageEncodePosition(pos_hexa));
				ecrit_tous = 1;
				if(!ecrit_calib && CablageSortant[pos_hexa].nbcapt) {
					TypeCableConnecteur connecteur;
					connecteur = CablageSortant[pos_hexa].captr[0].connecteur;
					for(capteur=0; capteur<CablageSortant[pos_hexa].nbcapt; capteur++) {
						if((CablageSortant[pos_hexa].captr[capteur].connecteur != connecteur)
						   || (CablageSortant[pos_hexa].captr[capteur].vbb != capteur)
						   || (CablageSortant[pos_hexa].captr[capteur].gain != CABLAGE_DEFAUT_GAIN)
						   || (CablageSortant[pos_hexa].captr[capteur].capa != CABLAGE_DEFAUT_CAPA)
						   || (CablageSortant[pos_hexa].captr[capteur].rc != CABLAGE_DEFAUT_RC)) break;
					}
					if(capteur >= CablageSortant[pos_hexa].nbcapt) {
						fprintf(f,"%s",CablageEncodeConnecteur(connecteur));
						ecrit_tous = 0;
					}
				}
				if(ecrit_tous) for(capteur=0; capteur<CablageSortant[pos_hexa].nbcapt; capteur++) {
					if(capteur) fprintf(f,", ");
					fprintf(f,"%s.%d",CablageEncodeConnecteur(CablageSortant[pos_hexa].captr[capteur].connecteur),
							CablageSortant[pos_hexa].captr[capteur].vbb+1);
					ecrit_capa = (CablageSortant[pos_hexa].captr[capteur].capa != CABLAGE_DEFAUT_CAPA) || ecrit_calib;
					if((CablageSortant[pos_hexa].captr[capteur].gain != CABLAGE_DEFAUT_GAIN) || ecrit_capa) {
						fprintf(f," x%g",CablageSortant[pos_hexa].captr[capteur].gain);
						if(ecrit_capa) fprintf(f," /%g",CablageSortant[pos_hexa].captr[capteur].capa);
					}
				}
			}
			fprintf(f,"\n");
		}
		fclose(f);
	} else printf("! Fichier de cablage inaccessible: '%s' (%s)\n",fichier,strerror(errno));
}
/* ========================================================================== */
void CablageNeuf(TypeCablePosition pos_hexa) {
	short capteur;

	CablageSortant[pos_hexa].nom[0] = '\0';
	CablageSortant[pos_hexa].defini = 0;
	CablageSortant[pos_hexa].v2 = 1;
	CablageSortant[pos_hexa].type = -1; /* c'est a dire, pas de cablage, et non par defaut le type NTD */
	CablageSortant[pos_hexa].bolo = -1;
	CablageSortant[pos_hexa].nb_fischer = 0;
	CablageSortant[pos_hexa].nbcapt = 0;
	for(capteur=0; capteur<DETEC_CAPT_MAX; capteur++) {
		CablageSortant[pos_hexa].captr[capteur].gain = CABLAGE_DEFAUT_GAIN;
		CablageSortant[pos_hexa].captr[capteur].capa = CABLAGE_DEFAUT_CAPA; // 8.2;
		CablageSortant[pos_hexa].captr[capteur].rc   = CABLAGE_DEFAUT_RC;
		CablageSortant[pos_hexa].captr[capteur].connecteur = 0;
		CablageSortant[pos_hexa].captr[capteur].vbb = 0;
	}
	CablageDeplie[pos_hexa] = 0;
}
/* ========================================================================== */
void CablageStructInit() {
	TypeCablePosition pos_hexa;

	CablageModlV2 = 0;
	ModeleCableNb = 0;
	CablageFichierModeles[0] = '\0';
	//	for(pos=0; pos<CABLAGE_POS_MAX; pos++) etc..: ca bououcle!! (pos ne peut PAS etre = 256...)
	for(pos_hexa=0; pos_hexa<CABLAGE_POS_MAX; pos_hexa++) CablageNeuf(pos_hexa);
}
/* ========================================================================== */
int CablageLit(char *fichier) {
	char log,definition_en_cours,v2;
	int nb_erreurs;
	int j,k,n,m;
	int modl,adc,vbmax,rang_bb;
	FILE *f,*h;
	char ligne[256],*r,*item,*v,*g,*s,sep,fin;
	char auxiliaire[MAXFILENAME];
	char nom[16],position[8];
	short capteur;
	TypeCablageConnectRegl *connection;
	TypeCablePosition pos_hexa;
	TypeCableConnecteur connecteur;
	float gain,capa,rc;
	/*  cablage version 1: "position > connecteur.adc [x<gain> [/<capa>] ], ..."
		cablage version 2: "connecteur < position.capteur.type ['(' parm ')'], ..." 
		     avec <type> = <reglage> | "lecture"
		     et <parm> = <capacite> pour "ampl", <gain> pour "lecture", etc...
		exemple:  hb1 < a1.chaleur.ampl (10pF), a1.chaleur.comp, ..., a1.ionis.polar, a1.detec.chauffage,
		                a1.chaleur.lecture, a1.centre.lecture, a1.garde.lecture
	*/

	log = 0;
	v2 = 0;
	rang_bb = 0;
	nb_erreurs = 0;
	strcpy(CablageFichierLu,fichier);
	f = fopen(CablageFichierLu,"r");
	if(!f && StartFromScratch) {
		strcpy(CablageFichierModeles,"ModelesCablage");
		RepertoireExiste(CablageFichierLu);
		RepertoireIdentique(CablageFichierLu,CablageFichierModeles,auxiliaire);
		SambaLogDef("= Installe un cablage","dans",auxiliaire);
		CablageStandard(); CablageModlSauve(auxiliaire);
		CablageCree(0,1,1); CablageCree(1,2,2); CablageEcrit(CablageFichierLu);
		f = fopen(CablageFichierLu,"r");
	}

	// Initialisation du CablageSortant deja faite dans CablageStructInit()
	if(f) {
		SambaLogDef("= Lecture du cablage","dans",CablageFichierLu);
		definition_en_cours = 0;
		k = 1;
		while(LigneSuivante(ligne,256,f)) {
			if((ligne[0] == '#') || (ligne[0] == '\n')) continue;
			r = ligne;

			/* Entree d'un type de cablage */
			/* '=' <type> '{' // { <reglage-detecteur> ':' <ressource_numeriseur> // ...} '}' */
			if(!strncmp(ligne,"modele",6)) {
				item = ItemAvant("=@",&fin,&r);
				if(fin == '@') /* modeles dans un fichier separe */ {
					strcpy(CablageFichierModeles,ItemSuivant(&r,' '));
					RepertoireIdentique(CablageFichierLu,CablageFichierModeles,auxiliaire);
					h = fopen(auxiliaire,"r");
					if(!h) {
						printf("  ! Fichier %s inutilisable (%s). Modeles standard utilises\n",auxiliaire,strerror(errno));
						continue;
					}
				} else { h = f; CablageFichierModeles[0] = '*'; /* modeles inclus dans ce fichier */ }
				SambaLogDef(". Lecture des modeles","dans",(h == f)? CablageFichierLu: auxiliaire);
				CablageModlDecode(h,(h == f),log);
				if(h != f) { fclose(h); /* if(!CablageModlV2) { CablageModlV2 = 1; CablageModlSauve(auxiliaire); } */ }
				if(log) { printf("  . \n"); ArgAppend(stdout,0,CablageModlDesc); }
			} else if(*r == '=') {
				r++;
				item = ItemSuivant(&r,' ');
				if(ModeleCableNb < CABLAGE_TYPE_MAX) {
					CablageModlInit(ModeleCableNb,item);
					if(log) printf("  . Modele de cablage #%d: %s\n",ModeleCableNb,ModeleCable[ModeleCableNb].nom);
				} else {
					printf("  ! Trop de types de cablage (CABLAGE_TYPE_MAX=%d). Type %s ignore.\n",CABLAGE_TYPE_MAX,item);
					++nb_erreurs;
				}
				definition_en_cours = 1;
				continue;
			} else if(definition_en_cours) {
				if(*r == '}') {
					ModeleCableNb++;
					definition_en_cours = 0;
				} else if(ModeleCableNb < CABLAGE_TYPE_MAX) /* sinon: on a deja proteste plus haut, on saute juste la definition */ {
					n = ModeleCable[ModeleCableNb].nbconnections;
					if(n < DETEC_REGL_MAX) {
						connection = &(ModeleCable[ModeleCableNb].connection[n]);
						item = ItemSuivant(&r,':');
						strncpy(connection->reglage,item,DETEC_REGL_NOM);
						if(*r == '=') r++;
						item = ItemSuivant(&r,' ');
						strncpy(connection->ressource,item,NUMER_RESS_NOM);
						ModeleCable[ModeleCableNb].nbconnections += 1;
					} else {
						printf("  ! Deja %d connection lues! Connections suivantes abandonnees\n",ModeleCable[ModeleCableNb].nbconnections);
						++nb_erreurs;
					}
				}
				continue;
			}
			
			/* Entree d'un cablage reel */
			/* [ [<nom>]['/'<type>] ':' ] <position> '>' { <connecteur> [ '.' <voie> ] [ 'x' <gain> [ '/' <capa> ] ] [',']} */
			if(!ModeleCableNb) {
				strcpy(CablageFichierModeles,"ModelesCablage");
				RepertoireIdentique(CablageFichierLu,CablageFichierModeles,auxiliaire);
				CablageStandard(); CablageModlSauve(auxiliaire);
			}
			modl = 0; nom[0] = '\0';
			if(log) printf("  . Lu: [%s]\n",r);
			v = ItemAvant("<>",&sep,&r); // v en debut de toute chaine, et r en debut du cablage (sep:<) ou de la liste des connecteurs (sep:>)
			if(sep == '>') {
				v2 = 0;
				g = ItemJusquA(':',&v); // g en debut de toute chaine, et v avant la position
				if(*v != '\0') {
					s = g; ItemJusquA('/',&s); // g inchange, s sur le type de cablage
					if(*g) strncpy(nom,g,16);
					g = ItemJusquA('\0',&v);
				}
				strncpy(position,g,8);
			} else /* (sep == '<') */ {
				v2 = 1;
				s = ItemAvant("/>",&sep,&r); // s en debut de cablage, et r en debut du type (sep:/) ou de la liste des connecteurs (sep:>)
				if(sep == '/') {
					if(*s != '/') strncpy(nom,s,16);
					s = ItemJusquA('>',&r); // s en debut du type, et r en debut de la liste des connecteurs
				} 
				if(log) printf("  . Position: [%s]\n",v);
				strncpy(position,v,8);
				rang_bb = 0;
			}
			if(nom[0] == '\0') sprintf(nom,"Cable#%d",k);
			if(*s != '\0') { for(modl=0; modl<ModeleCableNb; modl++) if(!strcmp(s,ModeleCable[modl].nom)) break; }
			if(modl >= ModeleCableNb) {
				modl = 0;
				printf("  ! Type de cablage inconnu pour %s: '%s'. Remplace par '%s'\n",nom,s,ModeleCable[modl].nom);
				nb_erreurs++;
			}
			//- printf("(%s) Decode position '%s'\n",__func__,position);
			pos_hexa = CablageDecodePosition(position);
			// printf("(%s) Position = 0x%X = %d\n",__func__,pos_hexa,pos_hexa);
			strcpy(CablageSortant[pos_hexa].nom,nom);
			CablageSortant[pos_hexa].type = modl;
			CablageSortant[pos_hexa].v2 = v2;
			CablageSortant[pos_hexa].nb_fischer = ModeleCable[modl].nb_numer;
			for(m=0; m<CablageSortant[pos_hexa].nb_fischer; m++) CablageSortant[pos_hexa].fischer[m] = 0;
			CablageSortant[pos_hexa].defini = 1;
			do {
				item = ItemJusquA(',',&r);
				if(v2) {
					if(rang_bb < ModeleCable[modl].nb_numer) {
						// int c; sscanf(item,"%d",&c); connecteur = c;
						//- printf("(%s) Decode connecteur '%s'\n",__func__,item);
						connecteur = CablageDecodeConnecteur(item);
						//- printf("(%s) Connecteur = 0x%X\n",__func__,connecteur);
						CablageSortant[pos_hexa].fischer[rang_bb] = connecteur;
						for(j=0; j<ModeleCable[modl].max_adc; j++) {
							if(ModeleCable[modl].capteur[j].numer_index == rang_bb) {
								CablageSortant[pos_hexa].captr[j].connecteur = connecteur;
								CablageSortant[pos_hexa].captr[j].vbb  = ModeleCable[modl].capteur[j].numer_adc - 1;
								// printf("  > CablageSortant[%d].captr[%d].vbb = %d\n",pos_hexa,j+1,CablageSortant[pos_hexa].captr[j].vbb);
								CablageSortant[pos_hexa].captr[j].gain = ModeleCable[modl].capteur[j].gain;
								CablageSortant[pos_hexa].captr[j].capa = ModeleCable[modl].capteur[j].capa;
								CablageSortant[pos_hexa].captr[j].rc   = ModeleCable[modl].capteur[j].rc;
								if(j >= CablageSortant[pos_hexa].nbcapt) CablageSortant[pos_hexa].nbcapt = j + 1;
							}
						}
						rang_bb++;
					}
				} else {
					g = item; ItemJusquA('x',&g); /* g pointe sur le gain */
					v = item; ItemJusquA('.',&v); /* v pointe sur la voie */
					connecteur = CablageDecodeConnecteur(item);
					if((*v != '\0') && (*v != '*')) {
						sscanf(v,"%d",&adc);
						if((adc <= 0) || (adc > DETEC_CAPT_MAX)) {
							printf("  ! Numero de voie de numeriseur illegal: %d (<= 0 ou > %d), remis a 1\n",adc,DETEC_CAPT_MAX);
							adc = 1;
							nb_erreurs++;
							continue;
						}
						capteur = CablageSortant[pos_hexa].nbcapt;
						// CablageInverse[connecteur][vbb] = (((shex)pos_hexa & 0xFF) << 4) | (capteur & 0xF);
						CablageSortant[pos_hexa].captr[capteur].connecteur = connecteur;
						CablageSortant[pos_hexa].captr[capteur].vbb = adc - 1;
						if(capteur < ModeleCable[modl].nb_capt) {
							if((rang_bb = ModeleCable[modl].capteur[capteur].numer_index) < ModeleCable[modl].nb_numer) 
								CablageSortant[pos_hexa].fischer[rang_bb] = connecteur;
						}
						// printf("Cablage sortant '%8s': [%02X.%d] -> %2d.%d\n",nom,pos_hexa,capteur,connecteur,adc);
						if(*g != '\0') {
							v = g;
							ItemJusquA('/',&v);
							sscanf(g,"%g",&(CablageSortant[pos_hexa].captr[capteur].gain));
							if(*v) sscanf(v,"%g",&(CablageSortant[pos_hexa].captr[capteur].capa));
							else CablageSortant[pos_hexa].captr[capteur].capa = CABLAGE_DEFAUT_CAPA;
						} else {
							CablageSortant[pos_hexa].captr[capteur].gain = CABLAGE_DEFAUT_GAIN;
							CablageSortant[pos_hexa].captr[capteur].capa = CABLAGE_DEFAUT_CAPA;
							CablageSortant[pos_hexa].captr[capteur].rc   = CABLAGE_DEFAUT_RC;
						}
						CablageSortant[pos_hexa].nbcapt = ++capteur;
					} else {
						if(*g != '\0') {
							v = g;
							ItemJusquA('/',&v);
							sscanf(g,"%g",&gain);
							if(*v) sscanf(v,"%g",&capa); else capa = CABLAGE_DEFAUT_CAPA;
							rc = CABLAGE_DEFAUT_RC;
						} else { gain = CABLAGE_DEFAUT_GAIN; capa = CABLAGE_DEFAUT_CAPA; rc = CABLAGE_DEFAUT_RC; }
						capteur = CablageSortant[pos_hexa].nbcapt;
						/* int bb; for(bb=0; bb<NumeriseurNb; bb++) if(Numeriseur[bb].fischer == connecteur) break;
						 if(bb < NumeriseurNb) vbmax = ModeleBN[Numeriseur[bb].def.type].nbadc;
						 else */
						vbmax = ModeleCable[modl].max_adc;
						while(capteur < vbmax) {
							// CablageInverse[connecteur][vbb] = (((shex)pos_hexa & 0xFF) << 4) | (capteur & 0xF);
							CablageSortant[pos_hexa].captr[capteur].gain = gain;
							CablageSortant[pos_hexa].captr[capteur].capa = capa;
							CablageSortant[pos_hexa].captr[capteur].rc   = rc;
							CablageSortant[pos_hexa].captr[capteur].connecteur = connecteur;
							CablageSortant[pos_hexa].captr[capteur].vbb = ModeleCable[modl].capteur[capteur].numer_adc - 1; // capteur;
							// printf("Cablage sortant '%8s': %02X.%d -> %2d.%d\n",nom,pos_hexa,capteur,connecteur,vbb);
							if(capteur < ModeleCable[modl].nb_capt) {
								if((rang_bb = ModeleCable[modl].capteur[capteur].numer_index) < ModeleCable[modl].nb_numer) 
									CablageSortant[pos_hexa].fischer[rang_bb] = connecteur;
							}
							++capteur;
						}
						CablageSortant[pos_hexa].nbcapt = capteur;
					}
				}
			} while(*r != '\0');
			if(log) printf("  . Position %03X > cablage %s de type %s (#%d) pour %d capteur%s\n",
				   pos_hexa,CablageSortant[pos_hexa].nom,
				   (CablageSortant[pos_hexa].type < ModeleCableNb)? ModeleCable[CablageSortant[pos_hexa].type].nom: "inconnu",
				   CablageSortant[pos_hexa].type,Accord1s(CablageSortant[pos_hexa].nbcapt));
			k++;
		}
		if(!feof(f)) {
			printf("  ! Lecture du cablage en erreur (%s)\n",strerror(errno));
			nb_erreurs++;
		}
		fclose(f);
	} else {
		printf("! Fichier de cablage illisible: '%s' (%s)\n",CablageFichierLu,strerror(errno));
		nb_erreurs++;
	}
	if(log) {
		for(pos_hexa=0; pos_hexa<CABLAGE_POS_MAX; pos_hexa++) {
			if(CablageSortant[pos_hexa].bolo >= 0) printf("(%s) @0x%02x <- Bolo #%d\n",__func__,pos_hexa,CablageSortant[pos_hexa].bolo);
		}
	}
// RepertoireIdentique(CablageFichierLu,"../modeles_EDW/cablage_lu",auxiliaire); CablageModlSauve(auxiliaire);
	return(nb_erreurs);
}
/* ========================================================================== */
int CablageRessource(TypeModeleCable *cablage, char **nom, char signe) {
	int i;
	
	// printf("(%s) recherche la ressource connectee a %s.\n",__func__,*nom);
	if(signe) {
		for(i=0; i<cablage->nbconnections; i++) if((cablage->connection[i].reglage[0] == signe) && !strcmp(*nom,cablage->connection[i].reglage+1)) {
			*nom = cablage->connection[i].ressource; return(i+1);
		}
	} else {
		for(i=0; i<cablage->nbconnections; i++) if(!strcmp(*nom,cablage->connection[i].reglage)) {
			// printf("(%s) trouve la ressource %s.\n",__func__,cablage->connection[i].ressource);
			*nom = cablage->connection[i].ressource; return(i+1);
		}
	}
	return(0);
}
