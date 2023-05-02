#define VERSION 9.47
#include <release.h>

#include <environnement.h>
#define chaine(s) chaine_preprocesseur(s)
#define chaine_preprocesseur(s) #s

#ifdef CW5
	char *SambaVersion = "9.47";
#else
	char *SambaVersion = chaine(VERSION)"."chaine(RELEASE);
#endif
// #ifdef macintosh
// #pragma message("Compilation de la version "chaine(VERSION)" (release "chaine(RELEASE)")")
// #endif

// #if __LP64__
// 	#pragma message("long int sur 64 bits")
// #else
// 	#pragma message("long int sur 32 bits")
// #endif

char *SambaCompile = __DATE__" a "__TIME__;

/* Include files avec Code Warrior:
MSL/MSL_C/MSL_Common/Include
MSL/MSL_C/MSL_MacOS/Include
MacOS Support/Universal/Interfaces/CIncludes
*/

/* Marques Xcode:
	// TODO: <texte>
	// FIXME: <texte>
	// MARK: <texte> avec - si trait a ajouter
*/

/* Partage de memoire:
 #include <sys/mman.h>
 path = shm_open(const char *name,O_CREAT|O_RDWR);
 adrs = void *mmap(0, size_t len, PROT_READ|PROT_WRITE, MAP_SHARED, path, 0);
 ...
 close(path);
 
 Euh non pas tout a fait, finalement voir SharedMemMake...
*/

/* Logiciel AB au 19.09.08:
	se trouve dans 134.158.176.20:/Users/archeops/CVS_EDW/Programmes/OPERA/OPERA_EDW/CEW_lib/c et h

	Codes de la commande OPERA 'Q':
	0 = startup -0          = recharche cew + lance cew
	1 = startup -1          = recharge cew_c1.jbc + etraxjbi cew_c1.jbc + lance cew
	2 = startup -2          = recharge bbv2.rbf
	  + mise a jour BBv2
	3 = mise a jour BBv2
	4 = startup -a          = recharge startup, cew_c1.jbc, cew, bbv2.rbf + etraxjbi cew_c1.jbc + lance cew
	5 = sauve_config
*/

/* Resume des episodes precedents
date    numero  difference avec la version precedente
--------  ----  -------------------------------------
24.11.99  0.1   structuration du programme principal
13.12.99  0.2   debut de la branche CMDES
11.02.00  0.3   fichiers de config; fichier detecteurs
17.02.00  0.4   debut de la branche LECT
09.03.00  0.5   ajout d'une branche SIMUL
-------------------------------------
14.04.00  1.0   graphs sur buffers 'Donnees' pour P.Cluzel
19.04.00  1.1   pointeurs sur Donnees par voie
26.04.00  1.2   intervalles user et data dans settings
27.04.00  1.3   repartiteur lu dans le fichier detecteurs
10.05.00  1.4   commandes globales dans le fichier detecteurs + rectification ecriture commandes
11.05.00  1.5   toujours <BoloGeres> lus dans la FIFO
08.06.00  1.6   lecture/archivage donnees brutes y.c. EDW1
28.06.00  1.7   pause lecture et limite en #modulations
-------------------------------------
26.07.00  2.0   retrait branche SIMUL et ajout sous-branche TRAITMT
05.09.00  2.1   module de traitements (au vol et pre-trigger) + trigger
18.09.00  2.2   histos, zooms sur donnees brutes
20.09.00  2.3   mise au point acq. reelle (sur Mac de Lyon)
26.09.00  2.4   tolerance possible de defauts FIFO (LectVoieBloc)
05.10.00  2.5   trigger "bruit"
13.10.00  2.6   pratiquement toutes les variables Settings en preferences
17.10.00  2.7   liste des evenements
30.10.00  2.8   corrige erreur 1er point graphique donnees brutes + Setup EDW
02.11.00  2.9   debut d'archivage
-------------------------------------
06.11.00  3.0   archivage incluant la gestion du buffer (.first)
22.01.01  3.1   histo 2D (amplitude x temps de montee)
30.01.01  3.2   utilisation de rundata.h
05.02.01  3.3   debut de l'aide en ligne
16.02.01  3.4   reorganisation des settings+monit
05.04.01  3.5   sauvegarde des donnees brutes
10.04.01  3.6   prise en compte de la version de l'electronique
25.04.01  3.7   ajout d'une branche Calculs
-------------------------------------
07.06.01  4.0   lecture PCI sous timer
27.08.01  4.1   ADC 14 bits avec electronique 2. Item Reglages. Protection de menus.
04.09.01  4.2   Vidage FIFO + rattrapage synchro OK
27.09.01  4.3   Test status optionnel + comptage points sur 64 bits + MAP demodul
05.10.01  4.4   Parms: ajout FIFO.dim et deplacement Voiei.min/max vers Monit
18.02.02  4.5   Modif menu principal pour utilisation de planches de bord
12.03.02  4.6   Planche operationnelle pour reglages bolo
11.07.02  4.7   Le mode EDW1 devient EDW0 + creation mode EDW1 (stream par Chardin)
06.08.02  4.8   Trigger multi-voies et modes cable, scrpt et perso
21.08.02  4.9   Regroupement affichage des voies + dimension tampons selon traitement
10.10.02  4.10  Attente garantie apres arret repartiteur
23.10.02  4.11  Redefinition des repertoires par defaut
28.11.02  4.12  recherche evts: boucle sur temps devient exterieure
09.12.02  4.13  Lit SambaArgs dans repertoire du binaire pour avoir le HomeDir
23.01.03  4.14  Adequation avec la note
23.04.03  4.15  Identification au demarrage + trigger/voie dans settings
24.04.03  4.16  Noms de run type EDW (attention: archives sans Hdr)
07.05.03  4.17  CmdeSend: CmdeAttendsMot (SambAttends) + RAZFifo OK + ArchiveDonnees hors It
06.06.03  4.18  Divers demandes de JG + ouverture archives permettant les Hdr
14.10.03  4.19  Acces direct PCI pour adresse ICA selon version
27.10.03  4.20  Echelles de temps evts + decalage debut d'evt selon voie
17.12.03  4.21  OscilloReglage + MonitFen type EVENT + def evts
08.01.04  4.22  Passage "Prises de donnees"<->"Reglages" + Compensation + Gestion carte MiG
15.01.04  4.23  TypeDonnee devient signe !!!
26.01.04  4.24  Graphiques 'donnees brutes' deviennent 'utilisateur' (histos user) + DMA
-------------------------------------
04.03.04  5.0   Passage sur MacOS-X (avec driver ICA), d'ou couche Ica sous MacOS classique
03.05.04  5.1   Le timer de lecture n'est jamais arrete (pb au redemarrage)
02.06.04  5.2   Mise a niveau apres serie de tests de l'electronique
-------------------------------------
07.09.04  6.0   Version Multitaches, donc sous XCode
14.10.04  6.1   Analyses multi-intervalles, pre-trigger par voie
09.11.04  6.2   Somme(FFT) et non FFT(Somme) => gestion spectres et menu FFT + filtres calculables + renommage settings
26.11.04  6.3   Trigger seuil au point + diagnostics H/W + relecture stream EDW2 + format archives + H/W HD
16.02.05  6.4   Debut d'utilisation de la messagerie
06.04.05  6.5   Prise en compte des numeriseurs V1.1
22.07.05  6.6   Modes de sauvegarde (relecture en rapport)
-------------------------------------
22.09.05  7.0   Toutes les voies sont independantes
09.11.05  7.1   Inclue les requetes "reglages bolo" de la reunion du 4 courant
18.11.05  7.2   Sait (enfin!) gerer le repartiteur version 2
05.01.06  7.3   Voies standard, planche LdB, types de buffer et nouveaux types de graphes ds graph. user, etc...
	            PREMIERE VERSION EN OPERATION EFFECTIVE, utilisee a la mise en route du 10.01.06
13.01.06  7.4   repertoires de resultats et figures, ntuples, legendes et sauve graphs, mode batch, automates
13.02.06  7.5   RAZ FETs, gains amplis BB, vsn rep=2 avec carte MiG
17.03.06  7.6   Mode blanc/noir par defaut+filtrage continue si evt+lecture tranchee+ntp ascii+spectre au vol
12.05.06  7.7   Chargement bolo au start si archive, histos evt sur ttes voies, catalogue, gestion evtmoyen et pattern
15.05.06  7.8   depart+lngr LdB, chargement bolo au startup samba, chgmt adrs bolo
14.06.06  7.9   chgmt BB au vol, tranches archive meme pour evts, divers
26.06.06  7.10  1ere procedure de regen, archive fmt edw1, garantie vidage FIFO, nb jetees
10.07.06  7.11  gestion du temps mort, acceleration du filtrage pre-trigger, gains variables, modeles de det
21.08.06  7.12  restructuration dialogue pour shifteurs lambda; d3
13.09.06  7.13  spectres bruit utilisateur (mode FFT_PUISSANCE), repertoire fichiers crees y.c. Db, ecriture automates,ntp
13.10.06  7.14  archivage tensions bolos, enchainement auto lecture tranches, suite tps mort, priorite stream
08.11.06  7.15  independance demodul reglage bolo, nom run avant commentaires, delai min pour tt sauver, deblocage polars, 
                nvle regen, gestion razfet
20.11.06  7.16  filtrage avec doubles, amelioration depattern (?), bolos associes, voies non lues si OscilloReglage
28.11.06  7.17  utilisation du seuil pour accepter recalage, attente reglable razfet NbSi seulmt apres polar, relance trmt
07.12.06  7.18  gestion temps+dimension tampon traitees (inclus ntp chaleur et regen auto), gestion D3, compatible OPERA v1
18.12.06  7.19  gestion empilements evts, supprime calage chaleur, sauve D2+D3 REP, args compat.TANGO, spectres auto, sequences
27.03.07  7.20  incrementations #runs, spectres en Hz, map spectres auto, datation voies dans evts, sauve evts meme si tps mort
02.04.07  7.21  trmt moyenne et lissage, gain logiciel, voie pas archivee si trmt_tampon=invalid, restart depatterning, faux bolos, restart/erreur SC
27.04.07  7.22  acq synchrones avec SC en l'etat, regul.evt
15.05.07  7.23  procedure de regen limitee en temps, idem RAZ FET, compat NOSTOS, ident sans RAZ HW, options pattern, HoteCode=z si d=edw2
08.06.07  7.24  panel taux evts ttes voies, gestion OPERA OK (mais version 1, fin 2006)
26.06.07  7.25  fichier detecteur decoupable, map duplication de voies (sauf affichage)
-------------------------------------
21.08.07  8.0   n x boitiers OPERA v2 et m x cartes PCI
11.10.07  8.1   nom ctlg par mac, commentaire+type run conserves, timestamp dans ntp, regl.bolo individuel ds menu run, assoc. multiple
15.11.07  8.2   sauve seuils ds hdr evt, pos.evts ds ntp, repart. sans BB, gestion deblocage BB, panel consignes det. par paquets, Cahier.manip,
                journal de samba, bit-trigger OK, taux evts OK
28.11.07  8.3   integrale evt sur 2 phases, bug fixe pour panels sur planche traitements, ampl evts float
11.12.07  8.4   date et timestamp chaleur ok, controle temps passe dans TraiteStocke et Display, 2x2 sources, ID et IAS dans regen,
                supprime synchro + ajout sequence et regul ds "Prise de donnees", ferme skt apres acces automate
  .02.08  8.5   bolos et BB avec nb voies != 3, regul par intervalles, recuperation timestamp
14.03.08  8.6   reorganisation des variables et introduction des macros xxx_VAR, demarrage sur D3 si BO
xx.03.08  8.7   decouplage 'capteurs bolos'/'voies BB'
14.04.08  8.8   demodulation au vol faisable pre-trigger. bolos sauvables individuellement.
07.05.08  8.9   timestamp dans hdr run+voie, byteswap OPERA OK, VoieModele.utilisee, stream OK si compactage
28.05.08  8.10  synchronisation du demarrage sur D3 ou D2 selon repartiteur(s), decalage trace ionis OK
17.06.08  8.11  personnalisation filtres, ArgGet refait d'ou lecture prefs calculs OK et lecture modeles presque OK, bolo voisins modifiables
18.07.08  8.12  Ident avec plusieurs BO OK, Sauvegarde.max, bugfix sur moyenne pdt sauvegarde evt, flip polars, regul DIV, regul par voie
30.09.08  8.13  Redemande trames BO perdues, lecture status BBv2 OK, planchers seuils regul
03.10.08  8.14  Cdes MAJ des BO (Q), retards FO individuels, planche BB1 affichage direct OK, ReglageCreeVoie, VerifDisplay, temps CPU
14.10.08  8.15  Reprise auto sur erreur synchro, envoi mail si arret sur erreur, + d'infos sur perte evts
07.11.08  8.16  Voies.liste dans ConfigGenerale permet autant de voies par bolo que necessaire (mode VOIES_STYLE_NTD)
10.11.08  8.17  Mise au point du calcul par convolution; amelioration lecture streams; Maj ressources BBv2; histo delai trmt; compat BBv2=BBv1 (avec dico)
26.11.08  8.18  Lecture des Modeles
11.12.08  8.19  La procedure d'initialisation des BB utilise SelectionneVoies, compensation meme si demodulee et meme globale pendant run
10.02.09  8.20  Temps mort evt apres son max et par voie, moyenne evt sur signal brut, norme sur filtrage par template
06.03.09  8.21  2 echantillons/stamp BB si 1 voie
18.08.09  8.22  calage des voies en temps par physique (optionnel), gestion decalage maxi evt si template, attente de la D3 finale, MAJ pour CW
xx.xx.09  8.23  me souviens plus...
15.05.09  8.24  editeur de structures, test presence OPERA sur flag debug, histos delais evts, syncD3 retentee si rep HS, pile evts (pb delais < 0)
22.06.09  8.25  sauve bolo sur flip polars
30.06.09  8.26  meilleur CR des erreurs BO
06.07.09  8.27  suppression 'define MARINIERE', trigger 'derivee'
          8.xx  tests calage avec ADUenKeV, DAC global
-------------------------------------
06.08.09  9.0   definition de cablages des reglages bolos, vrais bolos a N voies
21.08.09  9.1   support de la NI6251, reglages de numeriseurs, modulation v2 (attrib selon BB, Ibolo, Rbolo)
25.05.10  9.2   gestion des modeles de detecteurs, meilleure gestion des FIFO IP
02.06.10  9.3   inversion des boucles repart/vidage FIFO dans DepileBoost->DepileOptimise, modif TimerStart
23.07.10  9.4   Fonctions SambaTraiteDonnee et SambaDeclenche
18.08.10  9.5   Trigger: [cf 9.1.7g] overflow ampl. + montee maxi (deadline) pour garder un evt si FRONT ou DERIVEE + rejet si sens oppose apres recalage + critere bruit LdB
02.09.10  9.6   Localisation des repartiteurs (donc possible liste unique des rep. accessibles)
13.09.10  9.7   Verification si 2eme Samba actif, trigger: duree min
04.11.10  9.8   planches en coord. relatives a (Dx,Dy), reco BB OK, relaxation regle BO arretee, decompte nb data entre D3 + test err_opera
23.11.10  9.9   decompte donnees total si restart auto, verrou signal si rep arrete, deactivation rep si muet, fichier matos.csv
01.12.10  9.10  version internationalisee (menus et textes des panels), VoieTampon.lue=-1 si BO arretee (NTP!), consignes{ETAT,CMDE}, reglages mieux controles
05.01.11  9.11  repartiteurs: fichier de maintenance possiblement commun; ajout ModeleRep.delai_msg apres envoi commande aux repartiteurs
15.02.11  9.12  arch: mode coinc+mode par detec (cf veto), reset SCv2 optionnel, ecriture CSV, arrondi seuils negatifs OK, DetecteursChargeTous ok si BBv2
21.03.11  9.13  commande de relais, diffusion numeriseurs et repartiteurs
02.05.11  9.14  reservoir pour repartiteurs (SC). Modes regen et relais dans scripts.
12.05.11  9.15  info run vers couchdb, nom filtre dans prefs bolos
20.05.11  9.16  recupere status BBv2 directement dans trame -1, debut doc PDF
24.06.11  9.17  variables automates (arch,aff) definies dans les prefs
06.07.11  9.18  pseudo voies, limites 10-90 temps_montee ajustables, of7 dans affichage evt (bugge)
01.09.11  9.19  options -installe et -info; installation et execution n'importe ou
18.10.11  9.20  trmt lissage et intediff, trigger derivee, aligne bases evt brut et traite, decodage hexa script
18.11.11  9.21  CALI, compensation sans le gain fixe, limite nb.instrum planche trmt, mise au point scripts
17.01.12  9.22  repartiteurs globaux et locaux, regen selon scripts, reconvergence
31.01.12  9.23  calcul du temps de montee dans le sens inverse
29.02.12  9.24  deplacement BB OK, organigrame OK, separation sources scripts et sequences
06.03.12  9.25  correction/optimisation sauvegarde status BB
20.03.12  9.26  abandon type de numerisation, gestion repart. par liste de numeriseurs, nb.voies par numeriseur variable par repart, debug pseudos
21.03.12  9.27  underflow ampl.
10.05.12  9.28  supprime lect/ecri au fmt v8. Autorise <categ> pour TypeDetecteur, et parallelise xxx 'tous'.
10.07.12  9.29  gestion Echantillonnage (->parms)
12.09.12  9.30  gains chaleur et compensation, support IPE
02.10.12  9.31  import de csv (repart+bb+slot), variable $maitre
12.10.12  9.32  sauve seulement bolos et voies locaux, nettoie les status BB tempo avant run, mode repartiteur avec verif optionelle
23.10.12  9.33  fichiers 'organigramme', fenetre connexion IPE, modes repart, decompte reprises, pas d'entretien bolo sur reprise
07.11.12  9.34  nouveau type de reglage: razfet, cablage avec procedure, decodage micro_bbv2, raz FLT inactives [1406 releases]
08.09.13  9.35  format liste bolos, organisation modeles, reseau, atelier de cablage, pseudos, demarrages, scripts, spectres multiples, etats, internationalisation...
xx.xx.xx  9.36  xxxx
16.01.15  9.37  les scripts admettent boucles, cdes 'acquis' et 'stream', et increment reglages [54 releases]
22.05.15  9.38  test si $maitre monte; min/max trigger float; traitees float; attente reset pour spectres auto; peut sauver ordre bb/bolos; graph taux evts OK [216 releases]
06.10.15  9.39  boucles dans scripts detec et bb, + gestion contexte exec. anomalie planches voies [268 releases].
20.04.16  9.40  bruit ampli en electrons [230 releases].
09.12.16  9.41  sauvegarde mixte, calendrier [536 releases].
23.02.18  9.42  2 qualites graphiques, demarrage repart en parallele [1039 releases].
11.10.19  9.43  definition des Graphiques via claps [549 releases]
14.04.20  9.44  trigger multivoies: modes OU et ET [veto]. Configs de detecteurs. Multiples generateurs [231 releases].
10.06.20  9.45  Coupure inversee (=rejet). Gene avec modulation. Gestion decalage template. Tango depuis Samba [435 releases].
12.03.21  9.46  Refonte calage. Incr+Decr ampl/div oscillo. Temps mort special alphas [367 releases].
15.09.21  9.47  Definition chassis plus souple. Donnees sur 23 bits. Categories d'evts.
 
- plantages evts random
- modif selections via samba
* categories dans les NTP
* nom categ dans evts?
+ double analyse sur deconvolution (avance=date montee ppale-1ere date montee, prelim=charge dans cet intervalle)
+ graphs users sous condition de categorie
O suivi: ajouter histos (ou 2d?)
+ ou plutot graphs user dans fenetre de suivi (f->affiche = "suivi")
- coupures dans Analyses > Graphiques evenements

 Deconvolution: si a(i) valeur lissee, y(i) = a(i) - a(i-1) + F * a(i) avec F ~ 1/RC
 
 pas de chgt avant le 18
 * verification du montage de S2:acquis (== acces $maitre)
 * courante testee == flottante
 * spectres de bruit: attendre le reset et demarrer le stockage 1 seconde apres
 - idem avec spectres live
 - spectre live sur plusieurs voies (plusieurs fenetres au choix)
 - veto multi-destinataire (IPE)
 - random trigger a tester
 - random trigger commun, et en plus du trigger std (ne participe pas au calcul du temps)
 - mode cooperatif
 */

/* corrections et versions en service

 07.03.14: 9.35.759
 o reglages chaleur: marge = 0 -> parametre trmt_au_vol par defaut = 0 au cas ou trmt=filtre
 - reglages chaleur: marge pas affichee
 * reglages chaleur: envoi des valeurs si pas de status: sur option, par defaut=non
 + reglages chaleur: deblocage BB pas note + deblocage port fibre pour toutes les fibres
 - connexion numeriseur: valeur user pas rafraichie si pas de modif
 + etat numeriseur: pas de chgmt de timestamp si pas dans acquisition
 - etat numeriseur: pouvoir remettre une selection de numeriseur dans l'acquisition
 + synchro partition: communication des events ssi mode bolos == manip (bolo==tous <=> locaux seulement)
 - enregistrements DB refuses -> valeur 0xn incomprise -> type DESC_OCTET pour ecriture decimale
 10.03.14: 9.35.762
 + syncho start: en mode sauvegarde, le panel 'type de run' bloque
 10.03.14: 9.35.765
 
 */

/* Programme de travail (dixit JM, le 13.11.98)
* initialisation du run (load des cartes PCI)
+ demarrage du run (lecture temperatures, temps et autres parms)
- decouplage de la partie Saclay du reste qui continue (ecriture buffer evt)
- recouplage partie Saclay (gestion du temps)
- recuperation-reconstitution d'evt et bufferisation memoire (pour que d'autres taches puissent acceder aux data)
	* visualisation individuelle d'un bolo selectionne (par interrogation du buffer evt)
	. soit evt apres evt (taches Saclay decouplee et visu a la demande)
	. soit en mode espion
	* regroupement et cumul des infos de run, automatique en debut et en fin de run
- suppression de zero pour le stockage
	* ecriture sur support
+ analyse (FFT, filtre optimal ou pas, ...)

	Temps caracteristiques
- dans le cristal:
		. charge :               montee 10 µs, descente 100 µs
		. chaleur: retard qq ms, montee 30 ms, descente 1 a qq s
- dans le NTD
     . charge : pas de signal
     . chaleur: retard 2 a 3 ms, montee 2 ms, descente 100 ms
- pre-trigger: 3/8, post-trigger: 5/8
*/

/*
 1.- Fichier Bolos:
 nom/type position[-interne] statut {tension=valeur(V)}
 + commandes de chargement 'classiques' (a executer avant? apres?)
 2.- Fichier Cablage
 position-interne connecteur-externe
 3.- Fichier Boitiers Bolo
 numero connecteur[-externe] adresse R=valeur-R C=valeur-C
 
 position-interne: {a..l}{1..12} code hexa {01..bc}
 connecteur-externe: {1..255}
 tables inverses:
 (a) TableBolo[position:=byte] -> index du bolo
 (b) TableCablage[connecteur:=byte] -> position
 si reperage connecteur != {1..255}, coder quand meme un byte avec...
 donc lire fichier 2, faire table (b),
 puis lire 1, faire table (a),
 puis lire 3, a chaque item avec tables (b+a) inserer directement dans structure BoitierBolo
 */


