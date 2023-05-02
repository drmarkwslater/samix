#ifdef macintosh
#pragma message("Compilation de "__FILE__)
#endif
#include <stdio.h>
#include <errno.h>

#include <environnement.h>
#define DIAGS_C

#ifdef CODE_WARRIOR_VSN
#include <defineVAR.h>
#include <claps.h>
#include <dateheure.h>
#include <opium_demande.h>
#include <opium.h>
#ifdef AVEC_PCI
#include <IcaLib.h>
#endif
#else
#include <utils/defineVAR.h>
#include <utils/claps.h>
#include <utils/dateheure.h>
#include <opium_demande.h>
#include <opium4/opium.h>
#ifdef AVEC_PCI
#include <PCI/IcaLib.h>
#endif
#endif
#include <samba.h>
#include <repartiteurs.h>
#include <diags.h>

#ifdef MODULES_RESEAU
#include <carla/data.h>
//#include <carla/msgs.h>
#endif
#include <objets_samba.h>

#define MAXDIAGTITRE 40
#define MAXDIAGEXPLIC 80
#define CLUZEL_vide(val) (((val) & 0xFFFF) == 0)  /* Plus sur pour le moment?? */

typedef struct {
	char *titre;
	TypeFctn fctn;
	char explic[MAXDIAGEXPLIC];
	int  resultat;
	char ok;
} TypeDiagElement;

typedef enum {
	DIAG_ICA_WR_REG = 0,
	DIAG_ICA_RD_FIFO,
	DIAG_CC_RESET,
	DIAG_CC_STATUS,
	DIAG_CC_READ,
	DIAG_MAXELTS
} DIAG_NUMTEST;

static int DiagIcaWriteReg(void);
static int DiagIcaReadFifo(void);
static int DiagCcReset(void);
static int DiagCcStatus(void);
static int DiagCcRead(void);

static TypeDiagElement DiagListe[] = {
	{ "Ecriture registres ICA", DiagIcaWriteReg, { '\0' } , 0 , 0 },
	{ "Lecture FIFO ICA",       DiagIcaReadFifo, { '\0' } , 0 , 0 },
	{ "Reset CC et IBB",        DiagCcReset,     { '\0' } , 0 , 0 },
	{ "Status CC",              DiagCcStatus,    { '\0' } , 0 , 0 },
	{ "Lecture CC",             DiagCcRead,      { '\0' } , 0 , 0 },
	{ 0, 0, { '\0' } , 0 }
} ;
static char *DiagNom[DIAG_MAXELTS+1];

static char DiagASauver;
static char DiagLog;
static int  DiagNb;
static Panel pDiag;

/* ========================================================================== */
static void DiagExplique(int i) {
/*	if(DiagLog) printf("          - %s: %s\n", DiagListe[i].titre, DiagListe[i].explic);
	else OpiumError(DiagListe[i].explic); */
	if(!DiagLog) OpiumError(DiagListe[i].explic);
}
/* .......................................................................... */
static int DiagIcaVerifReset(PciBoard board, int num) {
#ifdef AVEC_PCI
	int relu; int reg;

//	printf("          Ecrit 2 dans le registre RESET\n");
	IcaRegWrite(board,ICA_REG_RESET,0x02); /* reset carte */
	relu = IcaRegRead(board,ICA_REG_RESET);
	if(relu != 0x01) {
		sprintf(DiagListe[DIAG_ICA_WR_REG].explic,"Reset carte #%d incompris (relu %08X)",num,(hexa)relu);
		return(0);
	}
	IcaRegWrite(board,ICA_REG_RESET,0x00); /* il FAUT faire retomber le reset a la main */

	relu = IcaRegRead(board,ICA_REG_SELECT0);
	if(relu != 0xFFFFFFFF) {
		sprintf(DiagListe[DIAG_ICA_WR_REG].explic,"Select0 incorrect apres reset #%d (lu: %08X)",num,(hexa)relu);
		return(0);
	}

	for(reg=1; reg<4; reg++)  {
		relu = IcaRegRead(board,ICA_REG_SELECT0+reg);
		if(relu != 0x00000000) {
			sprintf(DiagListe[DIAG_ICA_WR_REG].explic,"Select%d incorrect apres reset #%d (lu: %08X)",reg,num,(hexa)relu);
			return(0);
		}
	}

/*	relu = IcaRegRead(board,ICA_REG_STATUS);
	if(relu != 0x000000) {
		sprintf(DiagListe[DIAG_ICA_WR_REG].explic,"Status incorrect apres reset (lu: %08X)",(hexa)relu);
		return(0);
	} */
	relu = IcaRegRead(board,ICA_REG_FPROM_ADRS);
	if((relu & 0x001FFFFF) != 0x000000) {
		sprintf(DiagListe[DIAG_ICA_WR_REG].explic,"Adrs FPROM incorrecte apres reset #%d (lu: %08X)",num,(hexa)relu);
		return(0);
	}
	relu = IcaRegRead(board,ICA_REG_FPROM_DATA);
	if(relu != 0xAA) {
	   sprintf(DiagListe[DIAG_ICA_WR_REG].explic,"Data FPROM incorrecte apres reset #%d (lu: %08X)",num,(hexa)relu);
	   return(0);
	}
	relu = IcaRegRead(board,ICA_REG_FPROM_CNTL);
	if(relu != 0x07) {
	   sprintf(DiagListe[DIAG_ICA_WR_REG].explic,"Cntl FPROM incorrect apres reset #%d (lu: %08X)",num,(hexa)relu);
	   return(0);
	}
	relu = IcaRegRead(board,ICA_REG_FIFO_CNTL);
	if(relu != 0x0D) {
	   sprintf(DiagListe[DIAG_ICA_WR_REG].explic,"Cntl FIFO incorrect apres reset #%d (lu: %08X)",num,(hexa)relu);
	   return(0);
	}
	relu = IcaRegRead(board,ICA_REG_SIGNATURE);
	if(relu != 0xCAFEFADE) {
	   sprintf(DiagListe[DIAG_ICA_WR_REG].explic,"Signature incorrecte apres reset #%d (lu: %08X)",num,(hexa)relu);
	   return(0);
	}
#endif

	return(1);
}
/* .......................................................................... */
static char DiagIcaTestSelect(PciBoard board, int reg, int val) {
#ifdef AVEC_PCI
	int relu;

	relu = IcaRegRead(board,ICA_REG_SELECT0+reg);
	if(relu != val) {
		sprintf(DiagListe[DIAG_ICA_WR_REG].explic,"Select%d incorrect apres reset #1 (lu: %08X)",reg,(hexa)relu);
		return(0);
	}
	val = -1 - val;
	IcaRegWrite(board,ICA_REG_SELECT0+reg,val);
	relu = IcaRegRead(board,ICA_REG_SELECT0+reg);
	if(relu != val) {
		sprintf(DiagListe[DIAG_ICA_WR_REG].explic,"Select%d incorrect apres ecriture (lu: %08X)",reg,(hexa)relu);
		return(0);
	}
	val = -1 - val;
	IcaRegWrite(board,ICA_REG_SELECT0+reg,val);
#endif
	return(1);
}
/* ========================================================================== */
static int DiagIcaWriteReg(void) {
	int reg;

	sprintf(DiagListe[DIAG_ICA_WR_REG].explic,"correct");
	DiagListe[DIAG_ICA_WR_REG].ok = 1;

	if(!DiagIcaVerifReset(PCIedw[PCInum].port,1)) { DiagListe[DIAG_ICA_WR_REG].ok = 0; goto termine; }

	if(!DiagIcaTestSelect(PCIedw[PCInum].port,0,0xFFFFFFFF)) {
		DiagListe[DIAG_ICA_WR_REG].ok = 0; goto termine;
	}
	for(reg=1; reg<4; reg++) if(!DiagIcaTestSelect(PCIedw[PCInum].port,reg,0x00000000)) {
		DiagListe[DIAG_ICA_WR_REG].ok = 0; goto termine;
	}
#ifdef AVEC_PCI
	IcaRegWrite(PCIedw[PCInum].port,ICA_REG_FPROM_ADRS,0x001FFFFF);
	IcaRegWrite(PCIedw[PCInum].port,ICA_REG_FPROM_DATA,0xFF);
	IcaRegWrite(PCIedw[PCInum].port,ICA_REG_FPROM_CNTL,0x0F);
#endif
	if(!DiagIcaVerifReset(PCIedw[PCInum].port,2)) DiagListe[DIAG_ICA_WR_REG].ok = 0;

termine:
	DiagExplique(DIAG_ICA_WR_REG);
	return(0);
}
/* .......................................................................... */
static int DiagIcaReadFifo(void) {
	int nb,nbtrames,max;
#ifdef AVEC_PCI
	int i,j,modele;
	hexa relu;
	char dejavu,en_dma; int cumul; int timeout,to2;
#endif

	nb = 0;
#ifdef AVEC_PCI
	IcaRegWrite(PCIedw[PCInum].port,ICA_REG_FIFO_CNTL,0x01);  /* passe en local du pt de vue du remplissage */
	IcaFifoReset(PCIedw[PCInum].port);
	IcaRegWrite(PCIedw[PCInum].port,ICA_REG_FIFO_CNTL,0x03);  /* demmarre le remplissage avec le pattern */
	SambaAttends(500);                         /* avec ca, la FIFO doit avoir deborde...  */
	IcaRegWrite(PCIedw[PCInum].port,ICA_REG_FIFO_CNTL,0x00);  /* termine le remplissage avec le pattern  */
	en_dma = IcaDmaDispo(PCIedw[PCInum].port);
	if(en_dma) {
		IcaXferStatusReset(PCIedw[PCInum].port);
		PCIedw[PCInum].bottom = PCIedw[PCInum].top = 0;
	}

	nbtrames = 100; /* soit 1 ms (remplissage en ~200 ms) */
	max = 32 * 3;   /* quelque soit l'etat des selections, on recupere 96 voies par trame */
	dejavu = 0; cumul = 0;
	for(j=0; j<nbtrames; ) {
		j++;
		modele = j << 8;
		for(i=0; i<max; i++) {
			if(en_dma) {
				to2 = 1000;
				do {
					if(PCIedw[PCInum].bottom >= PCIedw[PCInum].top) {
						IcaXferExec(PCIedw[PCInum].port);
						PCIedw[PCInum].bottom = 0;
						timeout = 120;
						do {
							PCIedw[PCInum].top = IcaXferDone(PCIedw[PCInum].port);
							if(PCIedw[PCInum].top < 0) printf("! Transfert avorte sur erreur #%d\n",IcaDerniereErreur());
							if(PCIedw[PCInum].top) break;
							SambaAttends(10);
						} while(--timeout);
						if(PCIedw[PCInum].bottom >= PCIedw[PCInum].top) {
							if(DiagLog) printf("          ");
							printf(". Le transfert par DMA ne s'est pas termine\n");
							sprintf(DiagListe[DIAG_ICA_RD_FIFO].explic,"DMA en erreur");
							DiagExplique(DIAG_ICA_RD_FIFO);
							nb = nbtrames * max;
							goto termine;
//						} else {
//							if(DiagLog) printf("          ");
//							printf(". Le transfert par DMA a rendu %d mots\n",PCIedw[PCInum].top);
						}
					}
					relu = PCIedw[PCInum].fifo[PCIedw[PCInum].bottom++];
				} while(CLUZEL_vide(relu) && (--to2));
				if(CLUZEL_vide(relu)) {
					if(dejavu) cumul++;
					else {
						if(DiagLog) printf("          ");
						printf(". La FIFO reste vide\n");
						dejavu = 1;
						cumul = 0;
					}
				}
			} else {
#ifdef CODE_WARRIOR_VSN
				/* ICA.Empty = 0; pour empecher la mise en cache, mais cette variable a disparu...! */
#endif
				// relu = IcaFifoRead(PCIedw[PCInum].port);
				relu = *(PCIadrs[PCInum]);
			}
			if((relu & 0xFFFF) != modele) {
				if(dejavu) cumul++;
				else {
					if(DiagLog) printf("          ");
					printf(". FIFO incorrecte au mot #%3d (%04X au lieu de %04X)\n",
						i+1,relu,modele);
					if(j > 1) dejavu = 1;
					cumul = 0;
				}
				nb++;
			} else {
				if(cumul > 0) {
					if(DiagLog) printf("          ");
					printf("  + %d autre(s) erreur(s) consecutive(s)\n",cumul);
				}
				dejavu = 0; cumul = 0;
			}
			modele++;
		}
	}

	if(dejavu && (cumul > 0)) {
		if(DiagLog) printf("          ");
		printf("  + %d autre(s) erreur(s) consecutive(s)\n",cumul);
	}

termine:
#else
    nbtrames = max = 0;
#endif
	if(nb) {
		sprintf(DiagListe[DIAG_ICA_RD_FIFO].explic,"%d/%d mots retrouves incorrects",nb,nbtrames * max);
		DiagListe[DIAG_ICA_RD_FIFO].ok = 0;
	} else {
		sprintf(DiagListe[DIAG_ICA_RD_FIFO].explic,"correct");
		DiagListe[DIAG_ICA_RD_FIFO].ok = 1;
	}
	DiagExplique(DIAG_ICA_RD_FIFO);
#ifdef AVEC_PCI
	IcaRegWrite(PCIedw[PCInum].port,ICA_REG_FIFO_CNTL,0x00);  /* repasse en externe du pt de vue du remplissage */
#endif
	return(0);
}
/* .......................................................................... */
static int DiagCcReset(void) {
#ifdef AVEC_PCI
	int relu;
#endif
	
	sprintf(DiagListe[DIAG_CC_RESET].explic,"correct");
	DiagListe[DIAG_CC_RESET].ok = 1;
	
#ifdef AVEC_PCI
//	if(VersionRepartiteur >= 2.0) {
		/* 1. reset de la carte par arret (suivi du redemarrage) de la transmission optique */
		CluzelEnvoieBasique(PCIedw[PCInum].port,0,0xA6,0x00,0x0000);  /* so-called "Reset Backplane" pour (peut-etre) faire tomber le bit 11 */
		CluzelIcaFlashWrite(PCIedw[PCInum].port,0x001FFFFF,0x01);
		SambaAttends(10);
		CluzelIcaFlashWrite(PCIedw[PCInum].port,0x001FFFFF,0x00);
		SambaAttends(10);
		/* 2. verification, dans le statuc CC, de l'etat de reset */
		CluzelEnvoieBasique(PCIedw[PCInum].port,0,0xAA,0x00,0x0001);  /* fait pointer ICA_REG_STATUS sur status CC */
		relu = IcaRegRead(PCIedw[PCInum].port,ICA_REG_STATUS);
 		if(!(relu & BIT(11))) {
			DiagListe[DIAG_CC_RESET].resultat = relu;
			DiagListe[DIAG_CC_RESET].ok = 0;
			sprintf(DiagListe[DIAG_CC_RESET].explic,"Reset CC inoperant (status = %08X)",relu);
			goto termine;
		}
		/* 3. sortie de l'etat de reset */
		CluzelEnvoieBasique(PCIedw[PCInum].port,0,0xA6,0x00,0x0000);  /* so-called "Reset Backplane" (efface le bit 11) */
		relu = IcaRegRead(PCIedw[PCInum].port,ICA_REG_STATUS);
 		if(relu & BIT(11)) {
			DiagListe[DIAG_CC_RESET].resultat = relu;
			DiagListe[DIAG_CC_RESET].ok = 0;
			sprintf(DiagListe[DIAG_CC_RESET].explic,"Reset CC pas efface (status = %08X)",relu);
			goto termine;
		}
		/* 4. reset des IBB tant qu'on y est */
		CluzelEnvoieBasique(PCIedw[PCInum].port,0,0xA8,0x00,0x0001);  /* debut reset IBB */
		relu = IcaRegRead(PCIedw[PCInum].port,ICA_REG_STATUS);
 		if(!(relu & BIT(10))) {
			DiagListe[DIAG_CC_RESET].resultat = relu;
			DiagListe[DIAG_CC_RESET].ok = 0;
			sprintf(DiagListe[DIAG_CC_RESET].explic,"Reset IBB inoperant (status = %08X)",relu);
			goto termine;
		}
		CluzelEnvoieBasique(PCIedw[PCInum].port,0,0xA8,0x00,0x0000);  /* fin de reset IBB */
		relu = IcaRegRead(PCIedw[PCInum].port,ICA_REG_STATUS);
 		if(relu & BIT(10)) {
			DiagListe[DIAG_CC_RESET].resultat = relu;
			DiagListe[DIAG_CC_RESET].ok = 0;
			sprintf(DiagListe[DIAG_CC_RESET].explic,"Reset IBB pas efface (status = %08X)",relu);
			goto termine;
		}
//	}
#endif
	
#ifdef AVEC_PCI
termine:
#endif
	DiagExplique(DIAG_CC_RESET);
	return(0);
}
/* .......................................................................... */
static int DiagCcStatus(void) {
#ifdef AVEC_PCI
	int relu,bit;

//	if(VersionRepartiteur >= 2.0) {
		CluzelEnvoieBasique(PCIedw[PCInum].port,0,0xAA,0x00,0x0001);  /* fait pointer ICA_REG_STATUS sur status CC */
		relu = IcaRegRead(PCIedw[PCInum].port,ICA_REG_STATUS);
		/* DiagCcPrint(relu) */
		printf("          Status CC: %08X\n",relu);
		printf("          ICA version %d.%d\n",(relu >> 29) & 0x7,(relu >> 27) & 0x3);
		printf("          CC  version %d.%d\n",(relu >> 24) & 0x7,(relu >> 22) & 0x3);
		for(bit=16; bit; ) {
			--bit;
			if(relu & BIT(bit)) 
				printf("  %7s %s\n",(bit < 8)? "erreur:": "",DiagTexteStatusCC[bit]);
		} // fin DiagCcPrint
		DiagListe[DIAG_CC_STATUS].resultat = relu;
		DiagListe[DIAG_CC_STATUS].ok = 0;
		if(relu & BIT(3)) {
			sprintf(DiagListe[DIAG_CC_STATUS].explic,"%s (%08X)",DiagTexteStatusCC[3],relu);
		} else if(relu & BIT(2)) {
			sprintf(DiagListe[DIAG_CC_STATUS].explic,"%s (%08X)",DiagTexteStatusCC[2],relu);
		} else if(relu & BIT(5)) {
			sprintf(DiagListe[DIAG_CC_STATUS].explic,"%s (%08X)",DiagTexteStatusCC[5],relu);
		} else if(relu & BIT(4)) {
			sprintf(DiagListe[DIAG_CC_STATUS].explic,"%s (%08X)",DiagTexteStatusCC[4],relu);
		} else if(relu & BIT(1)) {
			sprintf(DiagListe[DIAG_CC_STATUS].explic,"%s (%08X)",DiagTexteStatusCC[1],relu);
		} else if(relu & BIT(0)) {
			sprintf(DiagListe[DIAG_CC_STATUS].explic,"%s (%08X)",DiagTexteStatusCC[0],relu);
		} else {
			sprintf(DiagListe[DIAG_CC_STATUS].explic,"correct (%08X)",relu);
			DiagListe[DIAG_CC_STATUS].ok = 1;
		}
//	} else {
//		DiagListe[DIAG_CC_STATUS].resultat = 0;
//		sprintf(DiagListe[DIAG_CC_STATUS].explic,"non applicable");
//		DiagListe[DIAG_CC_STATUS].ok = 1;
//	}
#endif

	DiagExplique(DIAG_CC_STATUS);
	return(0);
}
/* .......................................................................... */
static int DiagCcRead(void) {
#ifdef AVEC_PCI
	int relu;
#endif
	
	sprintf(DiagListe[DIAG_CC_READ].explic,"correct");
	DiagListe[DIAG_CC_READ].ok = 1;
	
#ifdef AVEC_PCI
//	if(VersionRepartiteur >= 2.0) {
		CluzelEnvoieBasique(PCIedw[PCInum].port,0,0xAA,0x00,0x0002);  /* fait pointer ICA_REG_LECTCC sur FlashAdrs CC */
		CluzelEnvoieBasique(PCIedw[PCInum].port,0,0xA2,0x00,0xFFFF);  /* ecrit 0xFFFF dans FlashAdrs CC */
		relu = IcaRegRead(PCIedw[PCInum].port,ICA_REG_LECTCC);
		if(relu != 0xFFFF) {
			sprintf(DiagListe[DIAG_CC_READ].explic,"Ecriture/lecture de FlashAdrsCC en erreur (lu: %04X/%04X)",(shex)relu,0xFFFF);
			DiagListe[DIAG_CC_READ].ok = 0;
			return(0);
		}
		CluzelEnvoieBasique(PCIedw[PCInum].port,0,0xA2,0x00,0x0000);  /* ecrit 0x0000 dans FlashAdrs CC */
		relu = IcaRegRead(PCIedw[PCInum].port,ICA_REG_LECTCC);  /* ecrit 0x0000 dans FlashAdrs CC */
		if(relu != 0x0000) {
			sprintf(DiagListe[DIAG_CC_READ].explic,"Ecriture/lecture de FlashAdrsCC en erreur (lu: %04X/%04X)",(shex)relu,0x0000);
			DiagListe[DIAG_CC_READ].ok = 0;
			return(0);
		}
//	}
#endif

	DiagExplique(DIAG_CC_READ);
	return(0);
}
/* ========================================================================== */
int DiagComplet(void) {
	int i; TypeFctn fctn;

	DiagLog = 1;
	printf("%s/ Verification de l'electronique de lecture\n",DateHeure());
	for(i=0; i<DiagNb; i++) strcpy(DiagListe[i].explic,"a faire");
	OpiumDisplay(pDiag->cdr);
	for(i=0; i<DiagNb; i++) {
		fctn = DiagListe[i].fctn;
		(*fctn)(0);
		PanelRefreshVars(pDiag);
		if(!DiagListe[i].ok && (i < (DiagNb - 1))) { if(!OpiumChoix(2,SambaNonOui,"On continue?")) break; }
	}
	if(i < DiagNb) printf("%s/ Verification inachevee\n\n",DateHeure());
	else printf("%s/ Verification terminee\n\n",DateHeure());
	printf(" ______________________________________________________________________________________________\n");
	printf("|                                    Tests de l'electronique                                   |\n");
	printf("|______________________________________________________________________________________________|\n");
	printf("| Sous-ensemble teste              | Resultat                                                  |\n");
	printf("|__________________________________|___________________________________________________________|\n");
	for(i=0; i<DiagNb; i++)
		printf("| %-32s | %-57s |\n",DiagListe[i].titre,DiagListe[i].explic);
	printf("|__________________________________|___________________________________________________________|\n\n");
	DiagLog = 0;
	return(0);
}
/* ========================================================================== */
void DiagInit(void) {
	int i;

/* Initialisation generale */
	DiagASauver = 0;
	DiagLog = 0;
	DiagNb = 0;
	while(DiagListe[DiagNb].titre) {
		if(DiagNb >= DIAG_MAXELTS) {
			OpiumError("Tous les diagnostics ne sont pas implementes (seulement %d)",DIAG_MAXELTS);
			break;
		}
		DiagNom[DiagNb] = DiagListe[DiagNb].titre;
		DiagNb++;
	}
	DiagNom[DiagNb] = "\0";

	if(ModeBatch) return;

	mDiag = MenuLocalise(0);
	MenuItemArray(mDiag,DiagNb+1);
	pDiag = PanelCreate(DiagNb);
	for(i=0; i<DiagNb; i++) {
		MenuItemAdd(mDiag,DiagListe[i].titre,MNU_FONCTION DiagListe[i].fctn);
		PanelText(pDiag,DiagListe[i].titre,DiagListe[i].explic,MAXDIAGEXPLIC);
	}
	MenuItemAdd(mDiag,"Fermer", MNU_RETOUR);
	OpiumPosition(mDiag->cdr,300,50);
	OpiumPosition(pDiag->cdr,100,50);
}
/* ========================================================================== */
int DiagSetup(void) { return(1); }
/* ========================================================================== */
int DiagSauve(void) { PasImplementee(); return(0); }
/* ========================================================================== */
int DiagExit(void) {
	return(SambaSauve(&DiagASauver,"preferences de diagnostic",1,1,DiagSauve));
}
