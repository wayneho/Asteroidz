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

static char N = 6;            // number of asteroids on the screen at a time (too many will cause lag)
static char M = 1;            // speed at which asteroids travel
const unsigned char SineWave[30] = {8,9,10,11,12,13,14,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,1,2,3,4,5,6,7};        // annoying noise
unsigned char Index;
unsigned char reset = 1;
unsigned char start;                               //flag to start game after screen has been touched
unsigned long TimerCount;                          //counter for time travelled
unsigned int sliderPosition;                       //position of slider pot (0-4096)
unsigned char sliderValue;

Player player;
Player alien[2];
State explosion[4];
Asteroid asteroid[6];
Laser laser[LASERS];


// Initialize the player image and give it a starting location on the LCD
// Inputs: none
// Outputs: none
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
    //setWindow(player.state.x1,player.state.y1,player.state.x2,player.state.y2);
    //printBMP(&player.state);
}

// Initialize the enemy image and give it a starting location on the LCD
// Inputs: none
// Outputs: none
void Init_Alien(void){
    int i;
    alien[0].state.x1 = 60;
    alien[0].state.y1 = 40;
    for(i=0;i<2;i++){
        alien[i].state.x2 = alien[i].state.x1 + ALIENSHIPWIDTH -1;
        alien[i].state.y2 = alien[i].state.y1 + ALIENSHIPHEIGHT -1;
        alien[i].state.imageSize = ALIENSHIPBMP;
        alien[i].state.image = alienshipImg;
        alien[i].state.width = ALIENSHIPWIDTH;
        alien[i].state.height = ALIENSHIPHEIGHT;
        alien[i].state.life = 1;
    }
    //setWindow(alien[0].state.x1,alien[0].state.y1,alien[0].state.x2,alien[0].state.y2);

    setWindow(alien[0].state.x1,alien[0].state.y1,alien[0].state.x2,alien[0].state.y2);
    printBMP(&alien[0].state);
}

// Sets the explosion animations to the correct images
// Inputs: none
// Outputs: none
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


// Displays the starting image
// Inputs: none
// Outputs: none
void Init_StartScreen(void)
{
    int i ;
    //int palette_index;
    setWindow(0,0,239,319);
    writeCmd(0x0022);
    for(i = 0; i < 76800; i++)
    {
        //palette_index = startImage[i];
        //writeData(colorPalette[palette_index]);
    }
}

// Displays the time travelled after player has crashed
// Inputs: none
// Outputs: none
void displayEndScreen(void){
    unsigned long highscore;
    unsigned long num;

    Distance_Stop();                                     // stop distance counter
    Asteroid_Stop();                                     // stop asteroids being generated
    num = TimerCount;
    highscore = read_highscore();

    char buffer[10];
    char words[] = {"TIME TRAVELLED(s): "};
    char words2[] = {"HIGHSCORE: "};

    sprintf(buffer, "%i", num);
    writeString(words, 15, 30, red, white);
    writeString(buffer, 165,30, red, white);
    writeString(words2, 15, 50, red, white);

    if(num > highscore){
        write_highscore(num);
        highscore = num;
    }
    sprintf(buffer, "%i", highscore);
    writeString(buffer, 165,50, red, white);
}

// Writes the new high score into EEPROM
// Inputs: 32bit word
// Outputs: none
void write_highscore(unsigned long score){
    write_eeprom(0x2,0x0,&score);                   // block 1, offset 0
}

// Writes the high score from EEPROM
// Inputs: none
// Outputs: 32 bit word
unsigned long read_highscore(void){
    return read_eeprom(0x2,0x0);                   // block 1, offset 0
}



// Updates center coordinate of a sprite
// Inputs: Sprite
// Outputs: none
void getCenter(State *sprite)
{
    sprite->center_x = (sprite->x2 - sprite->x1)/2 + sprite->x1;       // get center x coordinate
    sprite->center_y = (sprite->y2 - sprite->y1)/2 + sprite->y1;       // get center y coordinate
}

/*void printBMP3(State *sprite){
    int i;
    int palette_index;
    writeCmd(0x0022);
    for(i=0;i<sprite->imageSize;i++)
    {
        palette_index = sprite->image[i];
        writeData(colorPalette[palette_index]);
    }
}*/


// Prints the image of a sprite
// Inputs: Sprite
// Outputs: none
void printBMP(State *sprite){
    int i;
    writeCmd(0x0022);
    for(i=0;i<sprite->imageSize;i++)
    {
        writeData(sprite->image[i]);
    }
}


// Prints sprites with transparent background
// transparent color = 0xFFFF
// Inputs: Sprite
// Outputs: none
void printBMP2(State *sprite){
    int i;
    //int palette_index;
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
            //palette_index = sprite->image[i];
            //writeData(colorPalette[palette_index]);
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



// Controls the rocket ship using a 10k slide potentiometer
// adc value 112 = stop
// Inputs: 12 bit ADC sample
// Outputs: none
void playerControl(unsigned int slider){
    int sliderValue= slider*0.0488;             // 12 bit ADC - 220pixels/4096values = ~0.0537

    if (player.state.x1 != player.px1)
    {
        if(abs(player.state.x1 - player.px1) > 2)             // debounce rapid changes in the slide potentiometer
        {
            player.state.x1 = sliderValue;
            player.state.x2 = sliderValue + (player.state.width -1);
            setWindow(player.state.x1,player.state.y1,player.state.x2,player.state.y2);
            printBMP(&player.state);
        }
    }
}

// joystick configuration
/*void playerControl(unsigned int slider){


    if(player.state.x1 > 200){
        player.state.x1 = 199;
        player.state.x2 = player.state.x1 + SPACESHIPWIDTH -1;
    }
    else if(player.state.x1 < 1){
        player.state.x1 = 2;
        player.state.x1 = player.state.x1 + SPACESHIPWIDTH -1;
    }
    else
    {
        if(slider >= 2151 && slider < 4096){
            player.state.x1 = player.state.x1 + 1;
            player.state.x2 = player.state.x2 + 1;
        }
        if(slider < 2049 && slider >= 1){
            player.state.x1 = player.state.x1 - 1;
            player.state.x2 = player.state.x2 - 1;
        }

    }

    setWindow(player.state.x1,player.state.y1,player.state.x2,player.state.y2);
    printBMP(&player.state);

}*/

// Get previous position of the space ship
// Inputs: 12 bit ADC sample
// Outputs: none
void getPlayerPosition(unsigned int slider){
    player.px1 = slider*0.0488;
    //player.px1 = player.state.x1;

}

// Function to animate an asteroid entering the LCD
// Draws an asteroid bitmap 'M' rows at a time depending on asteroid speed
// Inputs: none
// Outputs: none
void deployAsteroid(void){
    char i;
    int j;
    //int palette_index;
    unsigned short x_start, x_end,y_start, y_end, row;


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
                row = row - M;                                      // increment number of bottom rows to draw depending on speed of asteroid
                asteroid[i].row = row;
                for(j = asteroid[i].state.width*(row); j < (asteroid[i].state.width*asteroid[i].state.height)-1; j++)      // draw bottom row(s) as asteroid enters screen from above
                {
                    writeData(asteroid[i].state.image[j]);
                    //palette_index = asteroid[i].state.image[j];
                    //writeData(colorPalette[palette_index]);
                }
                y_end = y_end + M;
                asteroid[i].state.y2 = y_end;

             }
            else
            {
            asteroid[i].state.y1 = (y_end - asteroid[i].state.height);
            }
        }
    }


}


// Moves the asteroid vertically down by M pixel(s)
// deletes the asteroid if it has gone outside the screen resolution
// Inputs: none
// Outputs: none
void moveAsteroid(void){
    char i;
    for(i=0; i < N; i++)
    {
        if(asteroid[i].state.life == 1)
        {
            if(asteroid[i].row <= M)        // asteroid deploying animation finished
            {
                if(asteroid[i].state.y1 >= 320)
                {
                    deleteAsteroid(i);
                }
                else
                {
                    asteroid[i].state.y1 = asteroid[i].state.y1 + M;
                    asteroid[i].state.y2 = asteroid[i].state.y2 + M;
                    clearArea(asteroid[i].state.x1, asteroid[i].state.y1-M, asteroid[i].state.x2 , asteroid[i].state.y1+1, white);
                    setWindow(asteroid[i].state.x1,asteroid[i].state.y1,asteroid[i].state.x2,asteroid[i].state.y2);
                    printBMP(&asteroid[i].state);

                }
            }
        }
    }
}

// Sets the life to 0 to be overwritten by a new asteroid
// Inputs: index of asteroid to delete
// Outputs: none
void deleteAsteroid(unsigned short index){
    asteroid[index].state.life = 0;
/*    asteroid[index].state.x1 = 300;
    asteroid[index].state.x2 = asteroid[index].state.x1 + (ASTEROIDWIDTH_M-1);
    asteroid[index].state.y1 = 350;
    asteroid[index].state.y2 = 350+M;*/
}

// Creates new astroid starting at a random x location
// Inputs: index of Asteriod struct array with life = 0
// Outputs: none
void addAsteroidMedium(unsigned short index){
    asteroid[index].state.x1 = randomValue();
    //Asteroid[index].state.x1 = sliderPosition*0.0513 - (ASTEROIDWIDTH_M - SPACESHIPWIDTH);
    asteroid[index].state.x2 = asteroid[index].state.x1 + (ASTEROIDWIDTH_M-1);
    asteroid[index].state.y1 = 0;
    asteroid[index].state.y2 = M;
    asteroid[index].state.life = 1;
    asteroid[index].row = ASTEROIDHEIGHT_M;
    asteroid[index].state.imageSize = ASTEROIDBMP_M;
    asteroid[index].state.image = asteroidm;
    asteroid[index].state.height = ASTEROIDHEIGHT_M;
    asteroid[index].state.width = ASTEROIDWIDTH_M;
}



// Displays the 4 animations when the player crashes
// Inputs: Ax, Ay are the center of the image to overlap
// Outputs: none
void displayExplosionAnimation(unsigned short Ax, unsigned short Ay){
    int i;
    for(i = 0; i < 4; i++)
    {
        explosion[i].x1 = Ax - explosion[i].width/2;
        explosion[i].y1 = Ay - explosion[i].height/2;
        explosion[i].x2 = explosion[i].x1 + explosion[i].width -1;
        explosion[i].y2 = explosion[i].y1 + explosion[i].height -1;

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
    unsigned short Ax,Ay,Bx,By,dx,dy;
    unsigned short radius_A, radius_B, distance;
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

    if(distance < 0.8*radius_A + radius_B)              // circle detection
    {
        displayExplosionAnimation(Ax,Ay);
        A->life = 0;
        return true;
    }
    else
    {
        return false;
    }

    //return(distance < radius_A + radius_B);                                          // circle detection algorithm
    //return((dx*2 < (A->width + B->width)) && (dy*2 < (A->height+B->height)));        // box detection algorithm

}

// Check if player has collided with an asteroid
// Inputs: none
// Outputs: none
void detectPlayerCollision(void){
    int i;
    for(i=0;i<N; i++)
    {
        if(collision(&player.state, &asteroid[i].state)){
            loopEndGame();
        }
    }
}

// Display explosion animation continuously
// Inputs: none
// Outputs: none
void loopEndGame(void){
    displayEndScreen();
    while(player.state.life == 0){
        displayExplosionAnimation(player.state.center_x,player.state.center_y);
        playSound();
    }
}

// Adds a laser beam to the Laser Struct array
// Inputs: index in the array
// Outputs: none
void addLaser(unsigned short index)
{
    getCenter(&player.state);
    laser[index].state.x1 = player.state.center_x - LASERBEAM__WIDTH/2;
    laser[index].state.x2 = laser[index].state.x1 + LASERBEAM__WIDTH -1;
    laser[index].state.y1 = player.state.y1 - LASERBEAM__HEIGHT - 3;
    laser[index].state.y2 = player.state.y1 -2;
    laser[index].state.life = 1;
    laser[index].direction = 1;
    laser[index].state.imageSize = LASERBEAM_BMP;
    laser[index].state.image = laserbeam;
    laser[index].state.height = LASERBEAM__HEIGHT;
    laser[index].state.width = LASERBEAM__WIDTH;
    setWindow(laser[index].state.x1, laser[index].state.y1,laser[index].state.x2,laser[index].state.y2);
    printBMP(&laser[index].state);

}

// Move all lasers depending on direction and speed
// Inputs: none
// Outputs: none
void moveLaser(void)
{
    int i;

    for(i = 0; i < LASERS; i++)
    {
        if(laser[i].state.life == 1)        // hasnt hit an object yet
        {
            if(laser[i].state.y1 > 0)      // hasnt reached the end of the screen yet
            {
                if(laser[i].direction == 1)     // being fired from player
                {
                    laser[i].state.y1 = laser[i].state.y1 - LASERSPEED;
                    laser[i].state.y2 = laser[i].state.y2 - LASERSPEED;
                }
                else
                {
                    laser[i].state.y1 = laser[i].state.y1 + LASERSPEED;
                    laser[i].state.y2 = laser[i].state.y2 + LASERSPEED;
                }
                setWindow(laser[i].state.x1, laser[i].state.y1,laser[i].state.x2,laser[i].state.y2);

                printBMP(&laser[i].state);

            }
            else   // delete laser and clear area
            {
                laser[i].state.life = 0;
                clearArea(laser[i].state.x1, laser[i].state.y1,laser[i].state.x2+1,laser[i].state.y2 +1, white);
            }
        }
    }

}

// Plays sound that alternates between 440Hz and 330Hz
// Inputs: none
// Outputs: none
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
// Inputs: none
// Outputs: none
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
}

// Generates a random value between 0 and 189
// screen width: 240 pixels
// asteroid width: 50 pixels
// Inputs: none
// Outputs: random value (0-189)
unsigned short randomValue(void){
    char i;
    srand(ADC0());
    i = rand();
    while(i > 189){
        i = rand();
    }
    return i;
}

// Displays a string onto the LCD and set the background rgb and string rgb
// Inputs: string to be displayed, x and y position, color of string and color of background
// Outputs: none
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
// Outputs: none
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

// Interrupt to a switch used to fire lasers
// Priority level 4
// Inputs: none
// Outputs: none
void GPIOPortA_Handler(void){
    int i;
    GPIO_PORTA_ICR_R = 0x80;    // clear interrupt flag
    delayMS(5);                // debounce switch
    if(SW0 == 0)
    {
        for(i = 0; i < 10; i++)
        {
            if(laser[i].state.life == 0)
            {
                addLaser(i);
                return;
            }
        }

        //erase_eeprom(0x2,0x0);              // reset highscore - set block 1 offset 0 to 0
    }

}

// Touchscreen interrupt for starting game
// Priority level 5
// Inputs: none
// Outputs: none
void Touchscreen_Handler(void){
    //displayCountDown();
    GPIO_PORTE_ICR_R = 0x08;    // clear interrupt flag
    reset = 1;
    start = 1;
    player.state.life = 1;

}

// Initiates an asteroid onto the screen
// trigger when timer times out
// Inputs: none
// Outputs: none
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

// Keeps track of time traveled with a counter
// Inputs: none
// Outputs: none
void Distance_Handler(void){
  TIMER1_ICR_R = 0x00000001;  // clear interrupt flag
  TimerCount++;
}

// Creates sound when player crashes into an asteroid
// Inputs: none
// Outputs: none
void Sound_Handler(void){
  TIMER2_ICR_R = 0x00000001;     // clear interrupt flag
  if(Index >= 30){
      Index = 0;
  }
  Index = (Index+1)&0xFF;       // 8,9,10,11,12,13,14,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,1,2,3,4,5,6,7
  DAC = SineWave[Index] << 4;   // output one value each interrupt
}

// Resets the game by setting score to 0 and removing all asteriods and lasers from LCD
// Inputs: none
// Outputs: none
void resetGame(void){
    int i;
    TimerCount = 0;
    Index = 0;

    for(i = 0;i < N;i++){
        deleteAsteroid(i);
        asteroid[i].state.x1 = 0;
        asteroid[i].state.x2 = asteroid[i].state.x1 + (ASTEROIDWIDTH_M-1);
        asteroid[i].state.y1 = 330;
        asteroid[i].state.y2 = M;
    }

    for(i = 0;i < M;i++){
        laser[i].state.life = 0;
    }
}
