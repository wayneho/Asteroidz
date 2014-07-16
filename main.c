

#include "LCD.H"
#include "game.h"
#include "math.h"
#include "interrupts.h"

int main(void)

{
    int EEPROM_Status;

    // system setup
	Init_PLL();
	Init_Port();
	Init_LCD();
	Init_Interrupt();
	Init_Analog();


	retry:
	EEPROM_Status = Init_EEPROM();
	switch(EEPROM_Status){
	case 0:                                 // no errors
	    break;
	case 1:
	    goto retry;                         //retry
	default:
	    while(1){};                         // error enter infinite loop
	}

	// game setup
	clearLCD(white);
	Init_Timer0A(40000000);     // 80000000*12.5ns = 1s
	Init_Timer1A(80000000);
	Init_Player();
    Init_Explosions();
    Init_StartScreen();

	while(1)
	{
	    if (semaphore)                                      // start game after screen has been touched
	    {
	        if(reset == 1){
	            delayMS(200);                               // delay needed for touchscreen interrupt to settle
	            reset = 0;
	            resetGame();
	            displayCountDown();
	            Asteroid_Start();
	            Distance_Start();
	        }
	        else
	        {
                sliderPosition = ADC0();                        // get conversion from slide pot
                getPlayerPosition(sliderPosition);              // get previous position of player
                playerControl(sliderPosition);                  // move player according to slide pot value
                //moveLaser();
                deployAsteroid();
                moveAsteroid();
                detectPlayerCollision();
	        }
	    }

	}
}


