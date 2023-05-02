#include <stdio.h>
#include <ctype.h>  /* pour toupper */
#include <string.h> /* pour memcpy */
#include <stdlib.h> /* pour system */
#include <stdarg.h>

#ifndef OS9
	#include <time.h>
	#ifndef WIN32
		#include <unistd.h> /* pour sleep */
	#endif
	#ifndef __USE_GNU
		extern char *__progname;
	#endif
#endif

#include <opium.h>
#define MAXLIGNE 8192

#define LOG_ON_TERM
#ifdef LOG_ON_TERM
	static Term LogTerm;
#else
	static int LogTerm = 0;
#endif
static FILE *LogFile = 0;
static int LogMbx = -1;
static char LogStd = 1;

/* ======================================================================= */
void LogInit(void) { LogStd = 1; LogTerm = 0; LogFile = 0; LogMbx = -1;
//--	printf("(%s) Ecriture standard %s\n",__func__,LogStd?"en service":"HORS service");
}
/* ======================================================================= */
void LogOnStd(char b) { LogStd = b; }
/* ======================================================================= */
FILE *LogOnFile(char *nom) {
	LogOffFile();
	if(!nom) return(0);
	if(!strcmp(nom,"*")) LogFile = stdout;
	else if(!strcmp(nom,"**")) LogFile = stderr;
	else LogFile = fopen(nom,"w");
	return(LogFile);
}
/* ======================================================================= */
FILE *LogAddFile(char *nom) {
	LogOffFile();
	if(!nom) return(0);
	if(!strcmp(nom,"*")) LogFile = stdout;
	else if(!strcmp(nom,"**")) LogFile = stderr;
	else LogFile = fopen(nom,"a");
	return(LogFile);
}
/* ======================================================================= */
void LogOffFile(void) {
	if(LogFile) {
		if((LogFile != stdout) && (LogFile != stderr)) fclose(LogFile);
		LogFile = 0;
	}
}
/* ======================================================================== */
#ifdef LOG_ON_TERM
/* ======================================================================= */
lhex LogOnTerm(char *nom, int ligs, int cols, int taille) {
	LogTerm = TermCreate(ligs,cols,taille);
//	TermPort(LogTerm,nom);
	OpiumTitle(LogTerm->cdr,nom);
	OpiumDisplay(LogTerm->cdr);
	return((lhex)LogTerm);
}
/* ======================================================================= */
void LogOffTerm(void) {
	if(LogTerm) { TermDelete(LogTerm); LogTerm = 0;  }
	return;
}
/* ======================================================================== */
void LogBlocBegin(void) { if(LogTerm) TermHold(LogTerm); }
/* ======================================================================== */
void LogBlocEnd(void) { if(LogTerm) TermRelease(LogTerm); }
/* ======================================================================== */
#endif
#ifdef LOG_ON_SERVER
/* ======================================================================= */
int LogServerStart(char * nom)  {
	int rc;

	char cmde[80];
	sprintf(cmde,"logsvr %s &",nom);
	rc = system(cmde);
#ifdef WIN32
	Sleep(1);
#else
	sleep(1);
#endif
	return(rc);
}
/* ======================================================================== */
/* ======================================================================= */
char LogServerOutput(type,nom) char type,*nom; {
	char ligne[80];
	int l;

	if(LogMbx == -1) return(0);
	ligne[0] = type; if(nom) strcpy(ligne+1,nom); else ligne[1] = '\0';
	l = strlen(ligne);
	_mbx_send(LogMbx,ligne,l);
	return(1);
}
/* ======================================================================= */
void LogServerStop(void) {
	if(LogMbx != -1) { _mbx_send(LogMbx,"x",1); LogMbx = -1; }
}
/* ======================================================================= */
int LogOnServer(nom)  char *nom; {
	char mbxname[32];

	sprintf(mbxname,"LogMail_%s",nom+1);
	LogMbx = _mbx_link(mbxname);
	return(LogMbx);
}
/* ======================================================================== */
#endif
#ifdef LOG_ON_TERM_OUBLIE
/* ======================================================================= */
char LogMove(mode,count)  char mode; int count; {
#ifdef LOG_ON_SERVER
	char ligne[80]; int l;
#endif

	if(LogTerm) switch(mode) {
		case 'd': TermTextMove(LogTerm,-999999); break;
		case 'p': TermTextMove(LogTerm,-(LogTerm->haut-2)*count); break;
		case 'h': TermTextMove(LogTerm,-count); break;
		case 'b': TermTextMove(LogTerm,count); break;
		case 's': TermTextMove(LogTerm,(LogTerm->haut-2)*count); break;
		case 'f': TermTextMove(LogTerm,999999); break;
	}
#ifdef LOG_ON_SERVER
	if(LogMbx != 1) {
		if(count == 1) {
			ligne[0] = mode; ligne[1] = '\0'; l = 1; 
		}
		else {
			sprintf(ligne,"%c%d",mode,count); l = strlen(ligne); 
		}
		_mbx_send(LogMbx,ligne,l);
	}
#endif
	return(1);
}
/* ======================================================================== */
#endif
/* ======================================================================= */
void LogDump(FILE *f) {
#ifdef LOG_ON_SERVER
	char ligne[MAXLIGNE];
#endif

#ifdef LOG_ON_TERM
	if(LogTerm) TermDump(LogTerm,f);
#endif
#ifdef LOG_ON_SERVER
	if(LogMbx != -1) {
		ligne[0] = '$';
		strcpy(ligne+1,fichier);
		_mbx_send(LogMbx,ligne,strlen(fichier)+1);
	}
#endif
}
/* ======================================================================= */
void LogDate(void) {
#ifdef OS9
	int heure, date, tick_code, ticks, ticks_second ;
	short jour;
#else
	time_t temps;
	struct tm date;
#endif
	int hh, mm, ss, cc;
#ifdef LOG_ON_SERVER
	char ligne[256];
	int l;
#endif

#ifdef OS9
	_sysdate(2,&heure,&date,&jour,&tick_code);
	ticks = tick_code & 0xffff ;
	ticks_second = ( tick_code >> 16 ) & 0xffff  ;
	hh = (heure >> 16) & 0xFF; mm = (heure >> 8) & 0xFF; ss = heure & 0xFF;
	cc = ticks * 100 / ticks_second;
#else
	time(&temps);
	memcpy(&date,localtime(&temps),sizeof(struct tm));
	hh = date.tm_hour; mm = date.tm_min; ss = date.tm_sec; cc = 0;
#endif
	if(!LogStd && !LogFile && !LogTerm && (LogMbx == -1)) LogStd = 1;
	if(LogStd) {
		printf("%02dh%02d %02d,%02d/ ",hh,mm,ss,cc); fflush(stdout);
	}
	if(LogFile) {
		fprintf(LogFile,"%02dh%02d %02d,%02d/ ",hh,mm,ss,cc); fflush(LogFile);
	}
#ifdef LOG_ON_TERM
	if(LogTerm) TermPrint(LogTerm,"%02dh%02d %02d,%02d/ ",hh,mm,ss,cc);
#endif
#ifdef LOG_ON_SERVER
	if(LogMbx != -1) {
		ligne[0] = 'i';
		sprintf(ligne+1,"%02dh%02d %02d,%02d/ ",hh,mm,ss,cc);
		l = strlen(ligne);
		_mbx_send(LogMbx,ligne,l);
	}
#endif
}
/* ======================================================================= */
void LogName(void) {
	char ligne[256];
#ifdef OS9
	sprintf(ligne,"(%s) ",_prgname());
#else
	// strcpy(ligne,"(x) ");
#ifdef __USE_GNU
	sprintf(ligne,"(%s) ",program_invocation_name); /* dans errno.h */
#else
	sprintf(ligne,"(%s) ",__progname);
#endif
#endif
	ligne[1] = (char)toupper(ligne[1]);
	LogString(ligne);
}
/* ======================================================================= */
void LogEltFloat(char *chaine, int i, float f) {
	char ligne[MAXLIGNE];
	sprintf(ligne,chaine,i,f);
	LogString(ligne);
}
/* ======================================================================= */
void LogFloat(char *chaine, float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9)
 {
	char ligne[MAXLIGNE];
	sprintf(ligne,chaine,f0,f1,f2,f3,f4,f5,f6,f7,f8,f9);
	LogString(ligne);
}
/* ======================================================================= */
void LogString(char *chaine) {
#ifdef LOG_ON_SERVER
	char ligne[MAXLIGNE+1]; int l;
#endif

	if(!LogStd && !LogFile && !LogTerm && (LogMbx == -1)) LogStd = 1;
	if(LogStd) { fputs(chaine,stdout); fflush(stdout); }
	if(LogFile) { fputs(chaine,LogFile); fflush(LogFile); }
#ifdef LOG_ON_TERM
	if(LogTerm) TermAdd(LogTerm,chaine);
#endif
#ifdef LOG_ON_SERVER
	if(LogMbx != -1) {
		ligne[0] = 'i';
		strncpy(ligne+1,chaine,MAXLIGNE);
		l = strlen(ligne);
		_mbx_send(LogMbx,ligne,l);
	}
#endif
}
/* ======================================================================= */
int LogBook(char *fmt, ...) {
	char texte[MAXLIGNE];
	va_list argptr; int cnt;
#ifdef LOG_ON_SERVER
	char ligne[MAXLIGNE+1]; int l;
#endif

	va_start(argptr, fmt);
	cnt = vsnprintf(texte, MAXLIGNE, fmt, argptr);
	va_end(argptr);

	if(!LogStd && !LogFile && !LogTerm && (LogMbx == -1)) LogStd = 1;
	//-- printf("(%s) Ecriture standard %s\n",__func__,LogStd?"en service":"HORS service");
	if(LogStd) { fputs(texte,stdout); fflush(stdout); }
	if(LogFile) { fputs(texte,LogFile); fflush(LogFile); }
#ifdef LOG_ON_TERM
	if(LogTerm) TermAdd(LogTerm,texte);
#endif
#ifdef LOG_ON_SERVER
	if(LogMbx != -1) {
		ligne[0] = 'i';
		strncpy(ligne+1,texte,MAXLIGNE);
		l = strlen(ligne);
		_mbx_send(LogMbx,ligne,l);
	}
#endif
	return(cnt);
//	return(strlen(texte));
}
/* ======================================================================= */
void LogClose(void) {
	if(LogFile) {
		if((LogFile != stdout) && (LogFile != stderr)) fclose(LogFile); LogFile = 0; 
	}
#ifdef LOG_ON_TERM
	if(LogTerm) {
		TermDelete(LogTerm); LogTerm = 0; 
	}
#endif
#ifdef LOG_ON_SERVER
	if(LogMbx) {
		_mbx_send(LogMbx,"z",1); LogMbx = -1; 
	}
#endif
}

