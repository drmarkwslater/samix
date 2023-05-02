#include <stdlib.h>
#include <stdio.h>

#include "environnement.h"
#ifdef CODE_WARRIOR_VSN
	#pragma message(__FILE__)
#endif

//#ifndef CODE_WARRIOR_VSN
//	#define UNIX
//#endif

#ifdef Forutils64
	#define UNIX
#endif

/* priorite a UNIX */
#ifdef UNIX
	#undef macintosh
	// #pragma message("Implementation UNIX");
#endif

/* sinon, priorite a mac classique */
#ifdef macintosh
	#undef UNIX
	#undef XCODE
	#define MACOSclassic
	#define CARBON
	// #pragma message("Implementation MACOSclassic")
#endif

#include "timeruser.h"

/* #ifdef PUR_UNIX
	#undef macintosh
	#define UNIX
#endif PUR_UNIX */

#ifdef UNIX
	#include <signal.h>
	#include <unistd.h> /* pour usleep et ualarm */
	#include <sys/time.h>

	sigset_t TimerPrevMask;
	struct sigaction TimerAction,TimerPrevActn;
	#define TIMER_EN_MICROSECONDES
	#define TIMER_UNIT 100          /* soit 0,0001 seconde */
	#ifdef hpux
		#define TIMER_FLAGS SA_RESETHAND;
	#else
		#define TIMER_FLAGS SA_RESTART;
	#endif
#endif /* UNIX */

#ifdef macintosh
	#ifndef XCODE /* deja servi via UNIX */
		#define MACOSclassic

		#ifdef CODE_WARRIOR_VSN /* CodeWarrior y.c. CW7 */
			#ifdef CW7
				#ifdef TARGET_API_MAC_CARBON
					#undef TARGET_API_MAC_CARBON
				#endif
				#define TARGET_API_MAC_CARBON 1
			#endif /* CW7 */
			#include <Timer.h>
			#include <Events.h>
		#else /* !CODE_WARRIOR_VSN */
			/* "a la mac" alors qu'on est en realite sous Xcode */
			#include <CoreServices/CoreServices.h>
			#include <Carbon/Carbon.h>
			// #include <Carbon/HIToolbox/Events.h> not found
			// pas 64 bits: unsigned char WaitNextEvent(EventMask eventMask, EventRecord *theEvent, UInt32 sleep, RgnHandle mouseRgn);
		#endif /* CODE_WARRIOR_VSN */

		TMTask TimerAction;              /* Apples' time struct    */
		#define TIMER_EN_MILLISECONDES
		#define TIMER_UNIT 1000          /* soit 0,001 seconde */
		//#define TIMER_EN_MICROSECONDES
		//#define TIMER_UNIT 1          /* soit 0,000001 seconde */
		#ifdef TIMER_EN_MILLISECONDES
			#define TIMER_BASE TIMER_UNIT/1000
		#else /* TIMER_EN_MICROSECONDES */
			#define TIMER_BASE -TIMER_UNIT
		#endif /* TIMER_EN_MICROSECONDES */
	#endif /* !XCODE */
#endif /* macintosh */

#ifdef OS9
	#include <signal.h>
	#define TIMER_ALARM 9999
	int TimerAlmId;
	#define TIMER_EN_MILLISECONDES
	#define TIMER_UNIT 1000          /* soit 0,001 seconde */
#endif /* OS9 */

#ifdef WIN32
	#include <windows.h>
	#define TIMER_EN_MILLISECONDES
	#define TIMER_UNIT 1000          /* soit 0,001 seconde */
#endif /* WIN32 */

#define TIMER_SECONDE  (1000000 / TIMER_UNIT) /* valeur a mettre pour avoir une seconde */
#define TIMER_DECISEC  (100000 / TIMER_UNIT)  /* valeur a mettre pour avoir une deciseconde */
#define TIMER_MILLISEC (1000 / TIMER_UNIT)    /* valeur a mettre pour avoir une milliseconde */

static unsigned char TimerCreated = 0,TimerEnabled;
static void TimerDisable();
static Timer TimerFirst;
#define TIMER_NONE 0

/* ===================================================================== */
void TimerAttente(int ms) {
#ifdef UNIX
	usleep(ms*1000);
#endif
#ifdef WIN32
	Sleep(ms);
#endif
#ifdef MACOSclassic
	int temps,delai,decompte;
	temps = TickCount(); /* 1 tick = 1/60 seconde */
	delai = ms * 60; if(delai < 1000) delai = 1; else delai /= 1000;
	delai += temps; decompte = 1000000000;
	while((TickCount() < delai) && decompte) decompte--;
#endif
}
/* ===================================================================== */
void TimerMilliSleep(int ms) {
#ifdef UNIX
	usleep(ms * 1000);
#endif
#ifdef OS9
	int attente;

	attente = ms * 256;
	if(attente < 1000) attente = 1; else attente /= 1000;
	tsleep(0x80000000 | attente);
#endif
#ifdef MACOSclassic
	/* int attente; EventRecord evt;
	attente = ms * 60;
	if(attente < 1000) attente = 1; else attente /= 1000;
	WaitNextEvent(0,&evt,attente,nil); */
	usleep(ms * 1000);
#endif
#ifdef WIN32
	Sleep(ms);
#endif
}
/* ===================================================================== */
static void TimerGestion(
#ifdef UNIX
						 int demande_par_unix
#endif
						 ) {
	Timer timer;

	timer = TimerFirst;
	while(timer != TIMER_NONE) {
		if(timer->compte > 0) {
			timer->compte -= 1;
			if(!timer->compte) {
				if(timer->fctn) (timer->fctn)(timer->donnees);
			}
		}
		timer = timer->suivant;
	}
#ifdef UNIX
	struct itimerval timer_val;
	 //	ualarm(TIMER_UNIT,0); // plutot des millisecondes, en fait?
	timer_val.it_value.tv_sec = 0; timer_val.it_value.tv_usec = TIMER_UNIT;
	timer_val.it_interval.tv_sec = timer_val.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL,&timer_val,0);
#endif
#ifdef OS9
	TimerAlmId = alm_set(TIMER_ALARM,
		0x80000000|(TIMER_UNIT*256/1000000));
#endif
#ifdef MACOSclassic
	PrimeTime((QElemPtr)&TimerAction,TIMER_BASE);
#endif
}
/* ===================================================================== */
static void TimerInit(void) {
	if(TimerCreated) return;
	TimerEnabled = 0;
	TimerFirst = TIMER_NONE;

#ifdef UNIX
	TimerAction.sa_handler = TimerGestion;
	sigemptyset(&TimerAction.sa_mask);
	TimerAction.sa_flags = TIMER_FLAGS;
	sigaction(SIGALRM,&TimerAction,&TimerPrevActn);
#endif /* UNIX */
#ifdef OS9
	intercept(TimerGestion);
#endif /* OS9 */
#ifdef MACOSclassic
	TimerAction.qLink = NULL;
	TimerAction.qType = 0;
/* 1:	TimerAction.tmAddr = NewRoutineDescriptor((long (*)())TimerGestion,
		(long)(kCStackBased<<kCallingConventionPhase),
		kPowerPCISA);
   2:	TimerAction.tmAddr = NewProcPtr((struct RoutineDescriptor *)TimerGestion);
   3:	TimerAction.tmAddr = (TimerUPP)NewRoutineDescriptor((ProcPtr)(TimerGestion),
		uppTimerProcInfo,GetCurrentArchitecture());
*/
	#ifdef CODE_WARRIOR_VSN
		#ifdef CARBON
			#pragma message("Utilise CARBON")
		#endif
	#endif
#ifdef CARBON
	TimerAction.tmAddr = NewTimerUPP(TimerGestion);
#else
	TimerAction.tmAddr = NewTimerProc(TimerGestion);
#endif
	TimerAction.tmCount = 0;
	TimerAction.tmWakeUp = 0;
	TimerAction.tmReserved = 0;

	InsXTime((QElemPtr)&TimerAction); /* ou bien InstallXTimeTask */
#endif /* MACOSclassic */
	TimerCreated = 1;
#ifdef TIMER_LOG
	printf("##### Timers initialises #####\n");
#endif

}
/* ===================================================================== */
void TimerExit(void) {
	if(TimerEnabled) TimerDisable();
#ifdef UNIX
	sigprocmask(SIG_SETMASK,&TimerPrevMask,NULL);
#endif
#ifdef OS9
	intercept(0);
#endif
#ifdef MACOSclassic
#ifdef CARBON
	DisposeTimerUPP(TimerAction.tmAddr);
#else
	DisposeRoutineDescriptor(TimerAction.tmAddr);
#endif
#endif
	TimerCreated = 0;
#ifdef TIMER_LOG
	printf("##### Timers termines #####\n");
#endif
}
/* ===================================================================== */
Timer TimerCreate(void (*fctn)(void *), void *donnees) {
	Timer timer,element;
	int i; /* pour mise au point */

	timer = (Timer)malloc(sizeof(TypeTimer));
	if(!timer) return(0);
	timer->delai = timer->compte = 0;
	timer->fctn = fctn;
	timer->donnees = donnees;
	timer->suivant = TIMER_NONE;

	if(!TimerCreated) TimerInit();
	i = 0;
	if(TimerFirst == TIMER_NONE) TimerFirst = timer;
	else {
		element = TimerFirst;
		while(element->suivant != TIMER_NONE) { element = element->suivant; i++; }
		element->suivant = timer; i++;
	}

#ifdef TIMER_LOG
	printf("##### Timer #%d cree #####\n",i);
#endif
	return(timer);
}
/* ===================================================================== */
void TimerDelete(Timer timer) {
	Timer element,suivant;
	int i; /* pour mise au point */

	if(!timer) return;
	TimerStop(timer);
	i = 0;
	if(timer == TimerFirst) TimerFirst = timer->suivant;
	else {
		element = TimerFirst;
		while(element != TIMER_NONE) {
			if((suivant = element->suivant) == timer) break;
			element = suivant; i++;
		}
		if(element == TIMER_NONE) return;
		element->suivant = timer->suivant; i++;
	}
	free(timer);
#ifdef TIMER_LOG
	printf("##### Timer #%d supprime #####\n",i);
#endif
	if(TimerFirst == TIMER_NONE) TimerExit();
}
/* ===================================================================== */
static void TimerEnable(void) {
#ifdef UNIX
	struct itimerval timer_val;
//	ualarm(TIMER_UNIT,0); // plutot des millisecondes, en fait?
	timer_val.it_value.tv_sec = 0; timer_val.it_value.tv_usec = TIMER_UNIT;
	timer_val.it_interval.tv_sec = timer_val.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL,&timer_val,0);
	printf("(%s) 1 coup pour %d usecs\n",__func__,TIMER_UNIT);
#endif
#ifdef OS9
	TimerAlmId = alm_set(TIMER_ALARM,0x80000000|(TIMER_UNIT*256/1000000));
#endif
#ifdef MACOSclassic
//	WndStep("Lancement des timers");
	PrimeTime((QElemPtr)&TimerAction,TIMER_BASE);
//	WndStep("Timers lances");
#endif
	TimerEnabled = 1;
#ifdef TIMER_LOG
	printf("##### Timers autorises #####\n");
#endif
}
/* ===================================================================== */
static void TimerDisable(void) {
#ifdef UNIX
	struct itimerval timer_val;
	//	ualarm(0,0);
	timer_val.it_value.tv_sec = 0; timer_val.it_value.tv_usec = 0;
	timer_val.it_interval.tv_sec = timer_val.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL,&timer_val,0);
#endif
#ifdef OS9
	alm_delete(TimerAlmId);
#endif
#ifdef MACOSclassic
	RmvTime((QElemPtr)&TimerAction);
#endif
	TimerEnabled = 0;
#ifdef TIMER_LOG
	printf("##### Timers inhibes #####\n");
#endif
}
/* ===================================================================== */
void TimerStart(Timer timer, long us) {
	if(!timer) return;
	timer->delai = timer->compte = (us > TIMER_UNIT)? (us / TIMER_UNIT): 1;
	printf("(%s) Periode: %ld coups (soit %ld us)\n",__func__,timer->delai,TIMER_UNIT * timer->delai);
	if(timer->delai && !TimerEnabled) TimerEnable();
#ifdef TIMER_LOG
	printf("##### Timer(%ld) demarre #####\n",timer->delai);
#endif
}
/* ===================================================================== */
void TimerContinue(Timer timer) {
	if(timer) timer->compte = timer->delai;
	/* Attention, fonction normalement appelee sous interuption */
#ifdef TIMER_LOG
	printf("##### Timer(%ld) redemarre #####\n",timer->delai);
#endif
}
/* ===================================================================== */
void TimerTrig(Timer timer) {
	if(timer) timer->compte = 1;
#ifdef TIMER_LOG
	printf("##### Timer(%ld) trigge #####\n",timer->delai);
#endif
}
/* ===================================================================== */
long TimerStop(Timer timer) {
	long compte;

	if(timer) {
	#ifdef TIMER_LOG
		printf("##### Timer(%ld) arrete #####\n",timer->delai);
	#endif
		compte = timer->compte;
		timer->delai = timer->compte = 0;
	#ifndef CARBON
		Timer element;
		element = TimerFirst;
		while(element != TIMER_NONE) {
			if(element->delai > 0) break;
			element = element->suivant;
		}
		if(element == TIMER_NONE) TimerDisable();
	#endif
		return(compte);
	} else return(0);
}
#ifdef A_FAIRE
/* ===================================================================== */
Timer TimerAlarm(void) {
	Timer timer;

	timer = (Timer)malloc(sizeof(TypeTimer));
	if(!timer) return(0);

#ifdef UNIX
	TimerAction.sa_handler = SIG_IGN;
	sigemptyset(&TimerAction.sa_mask);
	TimerAction.sa_flags = 0;
	sigaction(SIGALRM,&TimerAction,&TimerPrevActn);
#endif
#ifdef OS9
	intercept(TimerGestion); /* tache = 'handler' interne a timeruser */
	timer->delai /= 1000000;
#endif
#ifdef MACOSclassic
	TimerAction.qLink = NULL;
	TimerAction.qType = 0;
	TimerAction.tmAddr = NewRoutineDescriptor((long (*)())fctn,
		(long)(kCStackBased<<kCallingConventionPhase),
		kPowerPCISA);
	TimerAction.tmCount = 0;
	TimerAction.tmWakeUp = 0;
	TimerAction.tmReserved = 0;

	InsXTime((QElemPtr)&TimerAction);
#endif

	return(timer);
}
#endif

