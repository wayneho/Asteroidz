   ___    ____ ______ ___   ____    ____  ___   ____
  / _ |  / __//_  __// _ \ / __ \  /  _/ / _ \ /_  /
 / __ | _\ \   / /  / , _// /_/ / _/ /  / // /  / /_
/_/ |_|/___/  /_/  /_/|_| \____/ /___/ /____/  /___/
========
Created by: Wayne Ho - July 27, 2014
 
 	The object of this game is to travel as far as possible while dodging asteroids along the way.  There are 3 power ups that spawn randomly.
 	The points power up spawns the most often and when picked up adds 3 points to the player score.  The shield power up allows the player to become
 	invulnerable once activated until it is popped.  If it is not popped after 3 seconds then it disappears .  The star power up spawns the least
 	frequent and it turns all the asteroids in the next 5 seconds into coins which can be picked up to add 1 point to the player's score.  After 4
    seconds of picking up the star power up, the coin sprite changes to a faded red color to indicate that it is about to expire.  As the game
    progresses the asteroids move faster indicated by the level you are on (currently there are only 2 levels).
 
 	The game is using:
 		- two 12 bit ADCs to control the player's x and y movement
 		- three 32 bit Timer Interrupts:
 			- one to control the asteroid spawn rate
 			- one to control the power up spawn rate and keep count of time travelled
 			- one to create sound
 		- two GPIO Interrupts:
 			- PE3 is connected to the touch screen and starts or resets the game
 			- PA7 is connected to a switch and activates the player shield item or resets the highscore (when in the end screen menu)
 		- EEPROM to store player highscore
 
 
========
    Hardware:
 
    1x Tiva C TM4C123G LaunchPad
    1x 3.2in tft LCD 240x320 resolution (ssd1289 controller)
    1x 3.2in LCD adapter (from waveshare)
    1x Joystick
    1x 1.5k, 3k, 6k, and 12k resistors to create a 4bit weighted DAC
    1x Audio Jack (SPC24110)
 
========
   Connection:
 
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
    adapter DDIR = PA6
    adaper DLE = PE2
    4bit DAC = PC 7:4
    Joystick_X = PE1
    Joystick_Y = PE4
