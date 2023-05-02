#ifndef DIAGS_H
#define DIAGS_H

#ifdef DIAGS_C
#define DIAGS_VAR
#else
#define DIAGS_VAR extern
#endif

#define BIT(n) (1 << n)

DIAGS_VAR char *DiagTexteStatusCC[16]
#ifdef MAINSAMBA
 = {
	"Enumeration impossible",
	"DONE bas apres configuration IBB",
	"Pas de Rx synchronise dans l'ICA",
	"Pas de puissance sur fibre Rx",
	"Plusieurs CC maitresses",
	"Adresses BB identiques",
	"Bit 6 positionne",
	"Bit 7 positionne",
	"Enumeration des IBB commencee",
	"Enumeration des IBB terminee",
	"Les IBB sont en reset",
	"La CC est en reset",
	"Flash CC  occupee",
	"Flash ICA occupee",
	"Bit 14 positionne",
	"Bit 15 positionne",
}
#endif
;

void DiagCmde(int adrs, int cmde, int val);
void DiagFlashWrite(int adrs, int val);

#endif
