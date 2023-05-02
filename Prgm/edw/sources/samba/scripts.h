/*
 *  scripts.h
 *  pour le projet Samba
 *
 *  Created by Michel GROS on 28.02.12.
 *  Copyright 2012 CEA/DSM/DAPNIA. All rights reserved.
 *
 */
#ifndef SCRIPTS_H
#define SCRIPTS_H

#ifdef SCRIPTS_C
#define SCRIPTS_VAR
#else
#define SCRIPTS_VAR extern
#endif

#include <environnement.h>
#include <opium.h>

#define HW_MAXBLOC 64
#define HW_MAXACTN 512
#define HW_MAXNOM 128

typedef enum { HW_DETEC, HW_NUMER, HW_MODL_DET, HW_MODL_NUM, HW_MEDIA, HW_DEFINE, HW_GLOBAL, HW_NB } HW_OBJ;
SCRIPTS_VAR char *HwCat[]
#ifdef SCRIPTS_C
 = { "Detecteur", "Numeriseur", "TypeDetecteur", "TypeNumeriseur", "Media", "Var", "Global", "\0" }
#endif
;
typedef enum { HW_VAL = 1, HW_NOM, HW_CAT } HW_FORME;

typedef struct {
	HW_FORME type_sym;
	union {
		struct { hexa val1,valN; } i;
		char nom[HW_MAXNOM];
	} v;
} TypeHwRegistre;

typedef struct {
	HW_FORME type_sym;
	union {
		int  val;
		char nom[HW_MAXNOM];
	} v;
} TypeHwValeur;

typedef struct {
	HW_OBJ obj; void *hw;    /* si hw == 0, tous les objets de type <obj> */
	short premiere,derniere; /* index dans TypeHwInstr HwAction[]         */
	short courante;
	short num;
//	short nb;                /* nombre de contextes alloues               */
	char invalide;           /* les instr. internes sont inapplicables    */
} TypeHwBloc,*HwBloc;

typedef struct { TypeHwRegistre reg; TypeHwValeur val; char oper,sautee,limitee; HwBloc blocadrs; } TypeHwInstr,*HwInstr;

typedef struct StructHwScript {
	TypeHwInstr action[HW_MAXACTN]; short nb_actns;
	TypeHwBloc bloc[HW_MAXBLOC]; short nb_blocs;
	int nb_vars;
	struct StructHwScript *precedent,*suivant;
} TypeHwScript,*HwScript;

typedef struct StructHwBoucle {
	short  debut;
	char   limitee,en_cours;
	HW_OBJ obj; void *hw;
	int    decompte;
	struct StructHwBoucle *precedente,*suivante;
} TypeHwBoucle,*HwBoucle;

typedef struct {
	long  date; /* date de la prochaine execution */
	HwBoucle boucle;
	short inst; /* instruction en cours */
} TypeHwExecContext,*HwExecContext;

int ScriptArrete(Menu menu, MenuItem item);
SCRIPTS_VAR Menu mScriptIntr;
SCRIPTS_VAR TypeMenuItem iScriptIntr[]
#ifdef SCRIPTS_C
 = { { "stopwait", MNU_FONCTION ScriptArrete }, MNU_END }
#endif
;
SCRIPTS_VAR Panel pScriptChoix,pScriptCntl,pScriptEtat,pScriptWait;
SCRIPTS_VAR Cadre bScriptCntl;
SCRIPTS_VAR int  ScriptWait,ScriptDuree;
SCRIPTS_VAR int64 ScriptDateLimite;
SCRIPTS_VAR char ScriptEtat;
SCRIPTS_VAR char ScriptKill;
SCRIPTS_VAR char ScriptModeExec;
SCRIPTS_VAR char ScriptSuite[MAXFILENAME];
SCRIPTS_VAR char *ScriptSyntaxe[]
#ifdef SCRIPTS_C
 = {
	"Syntaxe des scripts SAMBA:",
	"  <instr_globale>",
	"  'Detecteur'      <detec>      { ':' <consigne_det> | '{' // (<consigne_det> //) '}' }",
	"  'Numeriseur'     <numer>      { ':' <consigne_num> | '{' // (<consigne_num> //) '}' }",
	"  'TypeDetecteur'  <type_detec> { ':' <consigne_det> | '{' // (<consigne_det> //) '}' }",
	"  'TypeNumeriseur' <type_numer> { ':' <consigne_num> | '{' // (<consigne_num> //) '}' }",
	"  'Media'          <media>      { ':' <message> | '{' // (<message> //) '}' }",
    "  'Var'            <nom>        ':' <texte>",
	" ",
	"  <detec>    := { <nom> | '<' <type_detec> '>' | 'tous' }  -- pour <type_detec>, 'TypeDetecteur' est utilise",
	"  <numer>    := { <nom> | '<' <type_numer> '>' | 'tous' }  -- pour <type_numer>, 'TypeNumeriseur' est utilise",
	"  -- pour 'TypeDetecteur  tous', 'Detecteur tous' est utilise",
	"  -- pour 'TypeNumeriseur tous', 'Numeriseur tous' est utilise",
	" ",
	"  <consigne_det> := {",
	"      <reglage>           <affect>  <valeur_user>",
	// 	"    | <hexa>              <affect>  <entier>        -- <hexa>: numero de reglage", // decimal abandonne cause indistinguable de texte user
	"    | <bb>'.'<ressource>  <affect>  { <valeur_user> | <hexa> }",
	"    | <bb>'.'<hexa>       <affect>  <entier>                               -- <hexa>: sous-adresse ressource",
	"    | <instr_globale>",
	"  }",
	"  <consigne_num> := {",
	"      acces        <affect>  { 'ouvert' | 'ferme' | 'oui' | 'non' }",
	"    | <ressource>  <affect>  { <valeur_user> | <hexa> }",
	"    | <hexa>       <affect>  <entier>                                      -- <hexa>: sous-adresse ressource",
	"    | 'RAZ_DAC'",
	"    | 'load_DAC'",
	"    | <instr_globale>",
	"  }",
	"  <instr_globale> := {",
	"      'exec'        [<affect>]  <fichier_script>",
	"    | 'boucle'      [<affect>]  { <nb>  | 'indefinie' }                    -- ou 'loop' et 'infinite'",
	"    | 'recommence'                                                         -- ou 'endloop'",
	"    | 'wait'        [<affect>]  <secondes>",
	"    | 'acquisition' [<affect>]  <secondes>",
	"    | 'stream'      [<affect>]  <secondes>",
	"    | 'compensation'",
	"    | 'partition'",
	"    | 'sources'     [<affect>]  { 'hs' | { 'actives' | 'oui' | 'on' | 'yes' } }",
	"    | 'trigger'     [<affect>]  { 'suspendu' | { 'actif' | 'oui' | 'on' | 'yes' } }",
	"    | 'mode-regen'  [<affect>]  { 'oui' | 'non' }",
	"    | 'regul_clock' [<affect>]  <D3>",
	"  }",
	" ",
	"  <bb>           := { <nom_numeriseur> | 'numeriseur('<nom_capteur>')' }",
	"  <ressource>    := { <nom> | '<' <categorie> '>' }",
	"  <reglage>      := { <nom> | '<' <cmde> '>' }",
	"  <affect>       := {   '='  -- memorise",
	"                      | ':'  -- direct (non memorise par SAMBA si consigne det ou num) ",
	"                      | '=+' [ { '<' | '>' } { <limite> | 'standard' | 'consigne' } ] -- increment (memorise par SAMBA) ",
	"                      | '=-' [ { '<' | '>' } { <limite> | 'standard' | 'consigne' } ] -- decrement (memorise par SAMBA) ",
	"                      | '=*' [ { '<' | '>' } { <limite> | 'standard' | 'consigne' } ] -- facteur multiplicatif (memorise par SAMBA) ",
	"                      | '=/' [ { '<' | '>' } { <limite> | 'standard' | 'consigne' } ] -- division (memorise par SAMBA) }",
	"  <valeur_user>  := { <texte> | 'standard' | 'consigne' }",
	"  <entier>       := { <decimal> | <hexa> }",
	"  <hexa>         := '0x'<hexadecimal>",
	"\0"
}
#endif
;

int  ScriptEcritSyntaxe();
void ScriptInit();
void ScriptRaz(HwScript script);
int  ScriptNouveauBloc(HwScript script, HW_OBJ obj, void *hw);
char ScriptDecode(HwScript script, char *ligne, char *bloc_ouvert, int *pcur, char *prefixe);
// char ScriptRunInstr(TypeHwInstr *instr, HW_OBJ obj, void *hw, short *bloc, short *pc, HwBoucle *boucle, short *secs, char *prefixe);
char ScriptExecBatch(HwInstr prgm, HW_OBJ obj, void *hw, HwExecContext pc, short derniere, short *secs, char *prefixe);
char *ScriptMarge();
int  ScriptExecType(HW_OBJ obj, void *hw, char *nomcomplet, char *prefixe);
int  ScriptExec(char *nomcomplet, char *nomcourt, char *prefixe);
void ScriptBoucleVide(HwBoucle *en_cours);

#endif
