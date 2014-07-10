#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#define SW0                 (*((volatile unsigned long *)0x40004200))       // PA7; active low
#define TIRQ                (*((volatile unsigned long *)0x40024020))       // PE3; active low

// initalize ports for interrupt
void Init_Interrupt(void);

// controls asteroid spawn
void Init_Timer0A(unsigned long period);
void Timer0A_Stop(void);
void Timer0A_Start(void);

// keeps track of time travelled
void Init_Timer1A(unsigned long period);
void Timer1A_Stop(void);
void Timer1A_Start(void);

// intialize sound
void Init_Sound(unsigned long period);
void Sound_Stop(void);
void Sound_Start(void);




#endif
