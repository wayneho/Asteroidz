## Asteroidz

##### https://www.youtube.com/watch?v=VMtSydjA_Ow
========
  Created by: Wayne Ho 
 
 	The object of this game is to travel as far as possible while avoiding asteroids. 
 
 	The game is using:
 		- two 12 bit ADCs to control the player's x and y movement
 		- three 32 bit Timer Interrupts:
 			- one to control the asteroid spawn rate
 			- one to control the item spawn rate and keep count of time travelled
 			- one to create sound
 		- two GPIO Interrupts:
 			- PE3 is connected to the touch screen and starts or resets the game
 			- PA7 is connected to a button and activates the player shield item or resets the highscore (when in the end screen menu)
 		- EEPROM to store player highscore
 
 
========
#### Hardware:
 
    1x Tiva C TM4C123G LaunchPad
    1x 3.2in tft LCD 240x320 resolution (ssd1289 controller)
    1x 3.2in LCD adapter (from waveshare)
    1x Joystick
    1x 1.5k, 3k, 6k, and 12k resistors to create a 4bit weighted DAC
    1x Audio Jack (SPC24110)
 
========
#### Connections:
 
    LCD_DIN 7:0 = PB 7:0
    LCD_RS = PD2
    LCD_RD = PD3
    LCD_WR = PD6
    LCD_CS = PD7
    LCD_BL = PE5
    LCD_TIRQ = PE3
    LCD_RESET = PA6
    Switch = PA7
    adapter DEN = PE0
    adaper DLE = PE2
    4bit DAC = PC 7:4
    Joystick_X = PE1
    Joystick_Y = PE4
