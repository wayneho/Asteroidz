#include "game.h"
#include "images.h"
#include "math.h"
#include "stdlib.h"
#include "stdio.h"
#include "lcd.h"
#include "tm4c123gh6pm.h"
#include "interrupts.h"
#include "driver.h"


#define DAC         (*((volatile unsigned long *)0x400063C0))           // 4 bit weighted resistor DAC; PC 7-4

static char N = 5;            // number of asteroids on the screen at a time (too many will cause lag)
static char M = 1;            // speed at which asteroids travel
static unsigned int shield_timer;
static unsigned int star_timer;
static const unsigned char SineWave[30] = {8,9,10,11,12,13,14,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,1,2,3,4,5,6,7};        // annoying noise
static unsigned char Index;
volatile unsigned char reset = 1;
volatile unsigned char start;                            //flag to start game after screen has been touched
static unsigned int TimerCount;                          //counter for time travelled
static unsigned int game_score;


Player player;
State explosion[4];
State powerup[2];
Asteroid asteroid[5];



// Initialize the player image and give it a starting location on the LCD
void Init_Player(void){
    player.state.x1 = 100;
    player.state.y1 = 250;
    player.state.x2 = player.state.x1 + SPACESHIPWIDTH -1;
    player.state.y2 = player.state.y1 + SPACESHIPHEIGHT -1;
    player.state.imageSize = SPACESHIPBMP;
    player.state.image = spaceshipImage;
    player.state.width = SPACESHIPWIDTH;
    player.state.height = SPACESHIPHEIGHT;
    player.state.life = 1;
    player.item = 0;
    player.mode = 0;
    setWindow(player.state.x1,player.state.y1,player.state.x2,player.state.y2);
    printBMP(&player.state);
}

// Sets the explosion animations to the correct images
void Init_Explosions(void){
    int i;

    for(i=0;i<4;i++){
        explosion[i].x1 = 0;
        explosion[i].y1 = 0;
        explosion[i].x2 = explosion[i].x1 + EXPLOSION_WIDTH -1;
        explosion[i].y2 = explosion[i].y1 + EXPLOSION_HEIGHT -1;
        explosion[i].imageSize = EXPLOSION_BMP;
        explosion[i].width = EXPLOSION_WIDTH;
        explosion[i].height = EXPLOSION_HEIGHT;
    }
    explosion[0].image = explosionImage0;
    explosion[1].image = explosionImage1;
    explosion[2].image = explosionImage2;
    explosion[3].image = explosionImage3;

}


// Displays the time travelled after player has crashed
void displayEndScreen(void){
    unsigned int highscore;
    unsigned int num;

    Distance_Timer_Stop();                                     // stop distance counter
    Asteroid_Timer_Stop();
    num = game_score;
    highscore = read_highscore();

    char buffer[10];
    char words[] = {"SCORE: "};
    char words2[] = {"HIGHSCORE: "};

    sprintf(buffer, "%i", num);
    writeString(words, 15, 30, red, white);
    writeString(buffer, 150,30, red, white);
    writeString(words2, 15, 50, red, white);

    if(num > highscore){
        write_highscore(num);
        highscore = num;
    }
    sprintf(buffer, "%i", highscore);
    writeString(buffer, 150,50, red, white);
    //clearArea(0,0,24,10,white);
}


// Displays the game timer.
// Score display commented out to speed up system
void displayTime(void){
	unsigned long score, time;
    //char words[] = {"TIME: "};
	//char words2[] = {"SCORE: "};
	char buffer[10];
	char buffer2[10];

	score = game_score;
	time = TimerCount;

	sprintf(buffer, "%i", time);
	sprintf(buffer2, "%i", score);

    //writeString(words, 0, 1, red, white);
    //writeString(words2, 0,10, red, white);
	writeString(buffer, 0,1, red, white);
	writeString(buffer2, 0,10, red, white);

}

// Writes the new high score into EEPROM
// Inputs: 32bit word
void write_highscore(unsigned int score){
    write_eeprom(0x2,0x0,&score);                   // block 1, offset 0
}

// Reads the high score from EEPROM
// Outputs: 32 bit word
unsigned long read_highscore(void){
    return read_eeprom(0x2,0x0);                   // block 1, offset 0
}


// Updates center coordinate of a sprite
// Inputs: Sprite
void getCenter(State *sprite)
{
    sprite->center_x = (sprite->x2 - sprite->x1)/2 + sprite->x1;       // get center x coordinate
    sprite->center_y = (sprite->y2 - sprite->y1)/2 + sprite->y1;       // get center y coordinate
}

// Prints the image of a sprite
// Inputs: Sprite
__inline void printBMP(State *sprite){
    int i;
    writeCmd(0x0022);
    for(i=0;i<sprite->imageSize;i++)
    {
        writeData(sprite->image[i]);
    }
}

// Prints sprites with transparent background
// transparent color = 0xFFFF
// slower than printBMP
// Inputs: pointer to Sprite struct
void printBMP2(State *sprite){
    int i;
    int row, column;
    row = 0;
    column = 0;

    for(i=0;i<sprite->imageSize;i++)
    {
        column++;                                               // column holds next x address
        if(sprite->image[i] != TRANSPARENT_COLOR)                               // skip drawing if color is 0;
        {
            writeCmd(0x0022);
            writeData(sprite->image[i]);
        }
        else
        {
            setCursor(sprite->x1+column, sprite->y1+row);       // manually move the address counter to next location
        }

        if(column == sprite->width )                            // if x address reaches the last pixel increment to next row and reset column
        {
            row++;
            column = 0;
            setCursor(sprite->x1+column, sprite->y1+row);
        }

    }
}


// Controls the rocket ship using a joystick
// player movement is based on the values from two 12bit ADCs
// Inputs: 12 bit ADC sample for x and y direction
void playerControl(unsigned int x, unsigned int y){

	// player is traveling in the x direction
	if(player.state.x1 < 207){
		if(x >= 2150 && x < 3596){
			player.state.x1 = player.state.x1 + 1;
			player.state.x2 = player.state.x2 + 1;
		}
		if(x>=3596 && x < 4096){
			player.state.x1 = player.state.x1 + 2;
			player.state.x2 = player.state.x2 + 2;
		}
	}
	if(player.state.x1 > 1){
		if(x < 2050 && x >= 1500){
			player.state.x1 = player.state.x1 - 1;
			player.state.x2 = player.state.x2 - 1;
		}
		if(x < 1500 && x >= 0){
			player.state.x1 = player.state.x1 - 2;
			player.state.x2 = player.state.x2 - 2;
		}
	}
	// player is traveling in the y direction
	if(player.state.y1 < 320){
		if(y >= 2100 && y < 3596){
			player.state.y1 = player.state.y1 + 1;
			player.state.y2 = player.state.y2 + 1;
		}
		if(y>=3596 && y < 4096){
			player.state.y1 = player.state.y1 + 2;
			player.state.y2 = player.state.y2 + 2;
		}
	}
	if(player.state.y1 > 1){
		if(y < 2000 && y >= 1500){
			player.state.y1 = player.state.y1 - 1;
			player.state.y2 = player.state.y2 - 1;
		}
		if(y < 1500 && y >= 0){
			player.state.y1 = player.state.y1 - 2;
			player.state.y2 = player.state.y2 - 2;
		}
	}

    setWindow(player.state.x1,player.state.y1,player.state.x2,player.state.y2);
    printBMP(&player.state);

}

// Function to check if player has used shield item or acquired the star PowerUp
// Shield item expires after 3 seconds and star PowerUp expires after 5 seconds
// At 4 seconds the coins start to fade to indicate star PowerUp expiring
void PowerUp_Status(void){
	int i;

	if(player.state.image == invulnerable_spaceship){
		if(TimerCount >= shield_timer+3){
			player.state.life = 1;
			player.state.image = spaceshipImage;
		}
	}

	if(player.mode == 1){
		if(TimerCount >= star_timer+5){
			for(i = 0;i<N;i++){
				asteroid[i].state.image = asteroidm;
			}
			player.mode = 0;
		}
		if(TimerCount > star_timer+3 && TimerCount < star_timer+5){
			for(i = 0;i<N;i++){
				asteroid[i].state.image = coin_fade_image;
			}
		}
	}
	else{
		asteroid[0].state.image = asteroidm;
		asteroid[1].state.image = asteroidm;
		asteroid[2].state.image = asteroidm;
		asteroid[3].state.image = asteroidm;
		asteroid[4].state.image = asteroidm;

	}

}

// Function to animate an asteroid entering the LCD
// Draws an asteroid bitmap 'M' rows at a time depending on asteroid speed until it has fully entered the screen
void deployAsteroid(void){
    int i;
    int j;
    unsigned int x_start, x_end,y_start, y_end, row;


    for(i = 0; i < N; i++)
    {
        if(asteroid[i].state.life == 1)
        {
            x_start = asteroid[i].state.x1;
            x_end = asteroid[i].state.x2;
            y_start = asteroid[i].state.y1;
            y_end = asteroid[i].state.y2;
            row = asteroid[i].row;

            if(y_end < asteroid[i].state.height)                    // if astroid has not fully entered screen
            {
                setWindow(x_start, y_start, x_end, y_end);
                writeCmd(0x0022);
                row = row - M;
                asteroid[i].row = row;
                for(j = asteroid[i].state.width*(row); j < (asteroid[i].state.width*asteroid[i].state.height)-1; j++)      // draw bottom row(s) as asteroid enters screen from above
                {
                    writeData(asteroid[i].state.image[j]);
                }
                y_end = y_end + M;									// increment number of bottom rows to draw depending on speed of asteroid
                asteroid[i].state.y2 = y_end;

             }
            else
            {
            	if(asteroid[i].row == M && M >= 2){
            		clearArea(asteroid[i].state.x1,0,asteroid[i].state.x2,M, white);
            	}
            	asteroid[i].state.y1 = (y_end - asteroid[i].state.height);
            }
        }
    }


}


// Moves the asteroid vertically down by M pixel(s)
// Set life of the asteroid to 0 if it has gone outside the screen resolution
void moveAsteroid(void){
    int i;
    for(i=0; i < N; i++)
    {
        if(asteroid[i].state.life == 1)
        {
            if(asteroid[i].row <= M)        // asteroid deploying animation finished
            {
                if(asteroid[i].state.y1 >= 320)
                {
                    asteroid[i].state.life = 0;
                }
                else
                {
                    asteroid[i].state.y1 = asteroid[i].state.y1 + M;
                    asteroid[i].state.y2 = asteroid[i].state.y2 + M;
                    setWindow(asteroid[i].state.x1,asteroid[i].state.y1,asteroid[i].state.x2,asteroid[i].state.y2);
                    printBMP(&asteroid[i].state);

                }
            }
        }
    }
}

// Sets the life of the sprite to 0 and moves it offscreen
// Inputs: pointer to sprite to delete
void deleteSprite(State *sprite){
	sprite->life = 0;
	sprite->x1 = 0;
	sprite->x2 = 0;
	sprite->y1 = 340;
	sprite->y2 = 340;
}

void Init_Asteroids(void){
	int i;
	for(i=0;i<N;i++){
	    asteroid[i].state.life = 0;
	    asteroid[i].state.imageSize = ASTEROIDBMP_M;
	    asteroid[i].state.height = ASTEROIDHEIGHT_M;
	    asteroid[i].state.width = ASTEROIDWIDTH_M;
	    asteroid[i].state.image = asteroidm;
	}
}

// Creates new astroid starting at a random x location
// Inputs: index of Asteriod struct array with life = 0
void addAsteroidMedium(unsigned short i){
    asteroid[i].state.x1 = randomValue(219);
    asteroid[i].state.x2 = asteroid[i].state.x1 + (ASTEROIDWIDTH_M-1);
    asteroid[i].state.y1 = 0;
    asteroid[i].state.y2 = M;
    asteroid[i].state.life = 1;
    asteroid[i].row = ASTEROIDHEIGHT_M;
}




// Displays the 4 explosion animations
// Inputs: Ax, Ay are the center of the image to overlap
void displayExplosionAnimation(unsigned short Ax, unsigned short Ay){
    int i;
    for(i = 0; i < 4; i++)
    {
    	if(player.state.x1 <= 1){
    		//explosion[i].x1 = Ax - explosion[i].width/2 + (EXPLOSION_WIDTH-SPACESHIPWIDTH);
    		explosion[i].x1 = 0;
			explosion[i].y1 = Ay - explosion[i].height/2;
			explosion[i].x2 = explosion[i].x1 + explosion[i].width -1;
			explosion[i].y2 = explosion[i].y1 + explosion[i].height -1;
    	}
    	else if(player.state.x2 >= 238){
    		//explosion[i].x1 = Ax - explosion[i].width/2 - (EXPLOSION_WIDTH-SPACESHIPWIDTH);
    		explosion[i].x1 = 190;
			explosion[i].y1 = Ay - explosion[i].height/2;
			explosion[i].x2 = explosion[i].x1 + explosion[i].width -1;
			explosion[i].y2 = explosion[i].y1 + explosion[i].height -1;
    	}
    	else{
    		explosion[i].x1 = Ax - explosion[i].width/2;
			explosion[i].y1 = Ay - explosion[i].height/2;
			explosion[i].x2 = explosion[i].x1 + explosion[i].width -1;
			explosion[i].y2 = explosion[i].y1 + explosion[i].height -1;
    	}

        setWindow(explosion[i].x1,explosion[i].y1,explosion[i].x2,explosion[i].y2);
        printBMP2(&explosion[i]);
        delayMS(100);
    }
}

// Function to detect the collision between two sprites using bounding circles algorithm
// Inputs: 2 sprite structs
// Outputs: true if collision detected
bool collision(State *A, State *B)
{
    unsigned int Ax,Ay,Bx,By,dx,dy;
    unsigned int radius_A, radius_B, distance;
    getCenter(A);
    getCenter(B);
    Ax = A->center_x;
    Ay = A->center_y;
    Bx = B->center_x;
    By = B->center_y;
    radius_A = A->width/2;
    radius_B =  B->width/2;
    dx = abs(Ax - Bx);
    dy = abs(Ay - By);
    distance = sqrt(dx*dx + dy*dy);

    if(distance < 0.9*radius_A + 0.9*radius_B)              // circle detection
    {
        return true;
    }
    else
    {
        return false;
    }

    //return(distance < radius_A + radius_B);                                          // circle detection algorithm
    //return((dx*2 < (A->width + B->width)) && (dy*2 < (A->height+B->height)));        // box detection algorithm

}

// Check if player has collided with another asteroid or a PowerUp
// If player has collided with an asteroid display explosion animations
// If player has collided with a coin add 1 to player score
// If player has collided with a shield PowerUp then give player shield item
// If player has collided with a bonus points PowerUp then give add 3 to player score
// If player has collided with a star PowerUp then turn all asteriods into stars
void detectPlayerCollision(void){
    int i;
    for(i=0;i<N; i++)
    {
    	if(asteroid[i].state.life == 1){
			if(collision(&player.state, &asteroid[i].state)){
				clearArea(asteroid[i].state.x1, asteroid[i].state.y1, asteroid[i].state.x2 , asteroid[i].state.y2, white);
				deleteSprite(&asteroid[i].state);

				if(asteroid[i].state.image == asteroidm){
					player.state.image = spaceshipImage;
					player.state.life = player.state.life-1;
				}
				else{
					game_score += 1;
				}

				if(player.state.life == 0){
					loopEndGame();
					return;
				}
			}
    	}
    }

    for(i=0;i<2;i++)
    {
    	if(collision(&powerup[i], &player.state)){
    		if(powerup[i].life == 1){
    			powerup[i].life = powerup[i].life-1;
				clearArea(powerup[i].x1,powerup[i].y1,powerup[i].x2,powerup[i].y2, white);

				if(powerup[i].image == powerup_shield_image){
					player.state.image = spaceship_item_acquired;
					player.item = 1;
				}
				else if(powerup[i].image == powerup_star_image){
					star_timer = TimerCount;
					player.mode = 1;
					for(i = 0;i<N;i++){
						asteroid[i].state.image = coin_image;
					}
				}
				else{
					game_score+=3;
				}
    		}
    	}
    }
}

// Display explosion animation continuously
void loopEndGame(void){
    displayEndScreen();
    while(reset == 0){
        displayExplosionAnimation(player.state.center_x,player.state.center_y);
        playSound();
    }
}


// Plays sound that alternates between 440Hz and 330Hz
// ex. calculation:  80Mhz / 440Hz /30 = ~6061
void playSound(void){
    static char note = 0;
    if(note == 0){
        Init_Sound(6061);                                   // 6061 ~440Hz (A note)
    }
    else{
        Init_Sound(8081);                                   // 8081 ~330Hz (E note)
    }
    note ^= 1;
}

// Displays strings: 3, 2, 1, START
void displayCountDown(void){
    int i;
    char *word[] = {"3", "2", "1","START!"};
    clearLCD(white);
    for(i = 0; i < 4; i++)
    {
        if(i == 3){
            writeString(word[i], 100, 160, red, white);
            delayMS(1000);
            clearArea(90,150,190,250,white);
        }
        else{
        writeString(word[i], 120, 160, red, white);
        delayMS(1000);
        }
    }
    clearLCD(white);
}

// Generates a random value using the value from ADC0() as seed
// Inputs: desired random value range
// Outputs: random value between 0:range (inclusive)
unsigned short randomValue(unsigned char range){
    int i;

    i = rand()%range;
    return i;
}

// Displays a string onto the LCD and set the background rgb and string rgb
// Inputs: string to be displayed, x and y position, color of string and color of background
void writeString (char word [], unsigned short x, unsigned short y, unsigned short textrgb , unsigned short background)
{
    int i =0;
    while (word[i]!=0)
    {
        writeCharacter (word[i],x+8*i ,y, textrgb ,background);
        i++;
    }
}

// Displays a single character onto the LCD and set the background rgb and character rgb
// Inputs: character to be displayed, x and y position, color of text and color of background
void writeCharacter (unsigned char c, unsigned short x, unsigned short y, unsigned short textrgb , unsigned short background)
{
    int i = 0;
    int j = 0;
    for (i = 0; i <8; i ++)
    {
        for (j=0;j <8;j++)
        {
            setCursor (x+j,y+i);
            if (((font8x8_basic [c][i]>>j)&1) ==1)
            {
                writeReg(0x0022 , textrgb );
            }
                else
                {
                    writeReg(0x0022 ,background);
                }
         }
    }
}

// Interrupt to a switch.  This button uses the shield PowerUp if the player has acquired that item.
// If the game is displaying the end screen then this button resets the highscore
// Priority level 4
void GPIOPortA_Handler(void){
    //int i;

    delayMS(5);                // debounce switch
    if(SW0 == 0)
    {
    	if(player.item == 1){
    		if(player.state.image == spaceship_item_acquired)
    		{
				player.state.life += 1;
				player.state.image = invulnerable_spaceship;
				shield_timer = TimerCount;
    		}
			player.item = 0;
    	}

    	if(player.state.life == 0){
    		erase_eeprom(0x2,0x0);              // reset highscore - set block 1 offset 0 to 0
    	}
    }
    GPIO_PORTA_ICR_R = 0x80;    // clear interrupt flag

}

// Touchscreen interrupt used for resetting and starting the game
// Priority level 5
void Touchscreen_Handler(void){
    GPIO_PORTE_ICR_R = 0x08;    // clear interrupt flag
    start = 1;
    reset = 1;
}

// Timer0 interrupt. Spawns an asteroid when timer times out
void Asteroid_Handler(void){
	int i;
	TIMER0_ICR_R = 0x00000001;  // clear interrupt flag
	for(i = 0; i < N; i++)
	{
	  if(asteroid[i].state.life == 0)
	  {
		  addAsteroidMedium(i);
		  return;                   // found open slot for new asteroid
	  }

	}

}

// Timer1 interrupt.  Keeps track of time traveled and spawns powerups randomly
// Player score is proportional to time travelled
// Asteroid speed increases when game time reaches 20 seconds
void Distance_Handler(void){
	int i, random_num;
	TIMER1_ICR_R = 0x00000001;  // clear interrupt flag
	TimerCount++;
	game_score++;
	random_num = randomValue(4);
	if(TimerCount == 20){
		level_two();
		return;
	}
	if(random_num != 0 || random_num != 1){
	  for(i=0;i<2;i++){
		  if(powerup[i].life == 0){
			  spawnPowerUp(i);
			  return;
		  }
	  }

	}
}

// Creates sound when player crashes into an asteroid
// The sine wave is connected to a 4bit weighted DAC
void Sound_Handler(void){
	TIMER2_ICR_R = 0x00000001;     // clear interrupt flag
	if(Index >= 30){
	  Index = 0;
	}
	Index = (Index+1)&0xFF;       // 8,9,10,11,12,13,14,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,1,2,3,4,5,6,7
	DAC = SineWave[Index] << 4;   // output one value each interrupt
}

// Resets the game by setting score and time to 0
// Moves all sprites off screen and sets their life to 0
void resetGame(void){
    int i;
    TimerCount = 0;
    game_score = 0;
    Index = 0;
    reset = 0;
    M = 1;

    for(i = 0;i < N;i++){
        deleteSprite(&asteroid[i].state);
    }

    for(i = 0; i < 2;i++){
    	deleteSprite(&powerup[i]);
    }
    level_one();
    Init_Player();
}

// Function to setup the PowerUp image sizes
void Init_PowerUp(void){
	int i;
	for(i=0;i<2;i++){
		powerup[i].x1 = 0;
		powerup[i].y1 = 330;
		powerup[i].x2 = powerup[i].x1 + POWERUP_WIDTH -1;
		powerup[i].y2 = powerup[i].y1 + POWERUP_HEIGHT -1;
		powerup[i].height = POWERUP_HEIGHT;
		powerup[i].width = POWERUP_WIDTH;
		powerup[i].imageSize = POWERUP_BMP;
	}
}
// Function to spawn power ups.  The points powerup spawns more frequently than the shield powerup.  The star powerup is the most rare.
// If player is already holding the shield powerup then only the other powerups will spawn
// Input: index into powerup struct array
void spawnPowerUp(unsigned char i){
	int random_num;
	powerup[i].x1 = randomValue(209);
	powerup[i].x2 = powerup[i].x1 + (POWERUP_WIDTH -1);
	powerup[i].y1 = 0;
	powerup[i].y2 = powerup[i].y1 + (POWERUP_HEIGHT-1);
	powerup[i].life = 1;

	random_num = randomValue(5);
	//random_num = 0;

	if((random_num == 0 || random_num == 1) && (player.state.image == spaceshipImage) ){
		  powerup[i].image = powerup_shield_image;
	}
	else if(random_num == 2){
	  powerup[i].image = powerup_star_image;
	}
	else{
	  powerup[i].image = powerup_points_image;
	}
}
// Moves PowerUps down the screen by one pixel.  The star PowerUp moves quicker than the others
// If PowerUp is outside the screen display set its life to 0
void movePowerUp(void){
	int i;
	for(i=0;i<2;i++){
		if(powerup[i].life == 1){
			if(powerup[i].y1 < 320){
				if(powerup[i].image == powerup_star_image){
					powerup[i].y1 = powerup[i].y1 + 1;
					powerup[i].y2 = powerup[i].y2 + 1;
				}
				else{
					powerup[i].y1 = powerup[i].y1 + 1;
					powerup[i].y2 = powerup[i].y2 + 1;
				}
				setWindow(powerup[i].x1,powerup[i].y1,powerup[i].x2,powerup[i].y2);
				printBMP(&powerup[i]);
			}
			else{
				powerup[i].life = 0;
			}
		}
	}
}

// Displays the title of the game
void Init_StartScreen(void)
{
    int i;

    setWindow(5, 140, 234, 182);
    writeCmd(0x0022);
    for(i = 0; i < 9890; i++)
    {
        writeData(start_image[i]);
    }
}
// Displays the string "LEVEL 2" and a count down to begin the level
// Resets all powerups and asteroids
void level_two(void){
	int i, num;
	char buffer[10];
	char word[] = {"LEVEL 2"};
	char words2[] = {"SCORE: "};

	num = game_score;
	Asteroid_Timer_Stop();
	Distance_Timer_Stop();

	clearLCD(white);

	writeString(word, 100, 160, red, white);
	writeString(words2, 92, 145, red, white);
    sprintf(buffer, "%i", num);
    writeString(buffer, 148,145, red, white);


    delayMS(2000);
    displayCountDown();
	M = 2;									// increase asteroid speed

    for(i = 0;i < N;i++){
        deleteSprite(&asteroid[i].state);
        asteroid[i].state.image = asteroidm;
    }

    for(i = 0; i < 2;i++){
    	deleteSprite(&powerup[i]);
    }
    star_timer = TimerCount;
    Init_Player();

    Asteroid_Timer_Start();
    Distance_Timer_Start();
}

// Displays the string "LEVEL 1" and a count down to begin the level
void level_one(void){
	char word[] = {"LEVEL 1"};
	clearLCD(white);
	writeString(word, 100, 160, red, white);
	delayMS(1000);
	displayCountDown();
}

