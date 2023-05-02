#ifndef BANC_H
#define BANC_H

#ifdef BANC_C
#define BANC_VAR
#else
#define BANC_VAR extern
#endif

BANC_VAR char BancPrefs[MAXFILENAME];
BANC_VAR char BancAlimConnectee,BancGeneConnecte;
BANC_VAR float BancAlimVmesure,BancAlimImesure;
BANC_VAR Cadre bBanc;
BANC_VAR char BancEnTest;
BANC_VAR char BancUrgence;
BANC_VAR Instrum BancProgression;
BANC_VAR int *BancPrgs;

#define BANC_MSG_LNGR 100
BANC_VAR char BancMessage[BANC_MSG_LNGR];
BANC_VAR char BancEnErreur;

int BancAffichePlanche(Menu menu, MenuItem item);
void BancSptrSelecteVoies(char log);
void BancSptrRafraichi(int avance, int etape);
int BancInit();

#endif /* BANC_H */
