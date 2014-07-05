#include "LCD.H"
#include "game.h"
#include "math.h"
#include "interrupts.h"


int main(void)

{
    // system setup
	Init_PLL();
	Init_Port();
	Init_LCD();
	Init_Interrupt();
	Init_Analog();

	// game setup
	clearLCD(white);
	//Init_Timer0A(40000000);     // 80000000*12.5ns = 1s
	//Init_Timer1A(80000000);

	Init_Player();
    Init_Explosions();
    Init_StartScreen();

    Init_Sound(5099);           // 5099 ~523Hz (C note)
    Sound_Start();

	while(1)
	{

	    //loopGame();
	}
}


