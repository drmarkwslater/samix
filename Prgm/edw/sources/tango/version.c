#define VERSION 3.30
#include <release.h>

#include <environnement.h>
#include <defineVAR.h>

#ifdef CW5
	char *TangoVersion = "3.30";
#else
	char *TangoVersion = chaine(VERSION)"."chaine(RELEASE);
#endif

// marche, mais genere un warning a la compilation
//#ifdef macintosh
//#pragma message("Compilation de la version "chaine(VERSION)" (release "chaine(RELEASE)")")
//#endif

char *TangoCompile = __DATE__" a "__TIME__;

/* Ci-dessous, la vie mouvementee de ce logiciel:
 
 Auteur   |    jour    heure   | numero de version et changements
 ---------|--------------------|---------------------------------
 gros     | 07 Mar 03          | 1.0 version de faisabilite
 gros     | 24 Mar 03 16:33:27 | 1.1 debut des versions destinees a un usage reel
 gros     | 13 Mai 03 12:34:00 | 1.2 integration de la lecture EDW2 (chgt args: t->d)
 gros     | 17 Jun 03          | 1.3 Sauver/Relire le ntuple, afficher entetes
 gros     | 04 Aug 03          | 1.4 Separation du fichier nanopaw
 
 gros     | 10 Sep 03 10:55:38 | 2.0 Compatibilite avec CAST
 gros     |  7 Oct 03 16:40:03 | 2.1 Fit parabole, compte evts selectes
 gros     |                    | 2.2 ??? version pas annotee...!
 gros     | 20 Oct 03          | 2.3 MenuBar, tSelect, CalibApplique, etc...
 gros     | 05 Nov 03          | 2.4 Calcul de l'evenement unite
 gros     | 12 Nov 03          | 2.5 Ajout menus 'Calibration'
 gros     | 09 Dec 03          | 2.6 Lecture de plusieurs runs + remplace fread par read
 gros     | 04 Fev 04          | 2.7 Reforme dialogue (items Ntuple et Specifique)
 gros     | 24 Fev 04          | 2.8 Traces/supplements, ZonesQ, ChaleurAjuste et Corrige
 
 gros     | 01 Avr 04          | 3.0 Notion d'espace
 gros     | 11 Jun 04          | 3.1 sauver/relire ntuples (coupure, ajout)
 gros     | 17 Jun 04          | 3.2 EvtDefile (EvtAuto suspendu)
 gros     | 22 Jun 04          | 3.3 SelecteVar + corrige NtupleRelit
 gros     | 01 Oct 04          | 3.4 Menu Run (fctn Add et Del)
 gros     | 05 Oct 04          | 3.5 T0 des runs pour dates et delais evts + fichier .ctlg
 gros     | 11 Oct 04          | 3.6 retire obligation EvtRunAdd car menu mRun plus complet
 gros     |    Mar 05          | 3.7 ajustement chaleur par fit
 gros     | 23 Nov 05          | 3.8 mise a niveau, rapport a la version 7 de SAMBA
 lemrani  |    Mai 06          | 3.9 mise au point du refiltrage + crosstalk electronique
 gros     | 13 Jun 06          | 3.10 evt unite compatible SAMBA, sauvegarde selective d'evts
 gros     | 06 Sep 06          | 3.11 ArchEvtInitDisplay vire, gere par relecture.c
 gros+EA  | 29 Jan 07          | 3.12 Reorganisation dialogue+TangoDesc, acces AuPif
 gros     | 05 Jul 07          | 3.13 Options specifiques manip
 gros     | 05 Fev 08          | 3.14 Choix nom evt unite, marquage par zone sur biplot
 gros     | 20 Fev 08          | 3.15 restructuration de la couche de relecture
 gros     | 23 Jun 08          | 3.16 option exec avec commande 'ntuple'
 gros     | 03 Jul 08          | 3.17 option de discontinuite + -e devient -evts pour garder -exec
 gros     | 18 Jul 08          | 3.18 jusqu'a 2048 fichiers utilisables, et non 128
 gros     | 15 Jun 09          | 3.19 choix d'un intervalle de partitions
 gros     | 07 Jul 09          | 3.20 VoieEvent mieux gere (pb des montee==0), mode batch ok pour ntuples
 gros     | 24 Nov 09          | 3.21 QuotaMontee, Duree -> Descente, ProdTemps*, ajout ArchExit, XTalk seulmt si VoiesNb>1
 gros     | 31.08.12           | 3.22 limites fit via graphique, gestion resultats fit, graphiques stockables
 gros     | 31 Jul 13          | 3.23 coupures stockables, appelable depuis SAMBA, rattrapage SAMBA
 gros     | 20 Sep 13          | 3.24 evt vide -> VoieEvent[].lngr faux, remplace par ArchEvt[].dim
 gros     | 27 Jan 14          | 3.25 integrale en standard, recalcul delais sur coupure
 gros     | 19 Mar 14          | 3.26 pente de montee calculee apres passage du seuil
 gros     | 26 Mar 14          | 3.27 extractions de 2D-histos: bin, comptage zone, profils. Simplification du menu Ntuples et Evenements
 gros     | 12 Avr 17          | 3.28 liste des physiques; peut lire des streams
 gros     | 03 Jul 18          | 3.29 affichage deconvolution
 gros     | 11 Fev 20          | 3.30 sauve les evenements selectionnes. Mode batch avec scripts. Choix du separateur du _ntuple genere.

 gros     |                    | 4.00 peut lire des streams

 Deconvolution: si a(i) valeur lissee, y(i) = a(i) - a(i-1) + F * a(i) avec F ~ 1/RC
 
 Transfert d'un run: rsync -rv `sedine`:/Volumes/DonneesSPCi4/events/sf02b001 .

*/


