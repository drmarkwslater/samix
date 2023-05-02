#ifndef TIMERUSER_H
#define TIMERUSER_H

typedef struct StructTimer {
	long delai;              /* delai entre interrupts      */
	long compte;             /* decompte avant interrupt    */
	void (*fctn)(void *);    /* fonction a executer         */
	void *donnees;           /* argument de ladite fonction */
	struct StructTimer *suivant;
} TypeTimer,*Timer;

void TimerAttente(int ms);
void TimerMilliSleep(int ms);
/* void TimerInit(); */
void TimerExit(void);
Timer TimerCreate(void (*fctn)(void *), void *donnees);
void TimerDelete(Timer timer);
void TimerStart(Timer timer, long delai);
void TimerContinue(Timer timer);
void TimerTrig(Timer timer);
long TimerStop(Timer timer);
Timer TimerAlarm();

#endif

