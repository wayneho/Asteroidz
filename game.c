#include "game.h"
#include "images.h"
#include "math.h"
#include "stdlib.h"
#include "stdio.h"
#include "lcd.h"
#include "tm4c123gh6pm.h"
#include "interrupts.h"

#define DAC         (*((volatile unsigned long *)0x400063C0))           // 4 bit weighted resistor DAC; PC 7-4


const unsigned char SineWave[30] = {8,9,10,11,12,13,14,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,1,2,3,4,5,6,7};        // annoying noise
unsigned char Index;
unsigned char reset = 1;
unsigned char semaphore;                           //flag to start game after screen has been touched
unsigned long TimerCount;                          //counter for time travelled
unsigned int sliderPosition;                       //position of slider pot (0-4096)
static long highscore;



Player player;
State explosion[3];
Asteroid asteroid[N];
Laser laser[LASERS];


// Initializes the player image and gives it a starting location on the LCD
// Inputs: none
// Outputs: none
void Init_Player(void){
    player.state.x1 = 110;
    player.state.y1 = 250;
    player.state.x2 = player.state.x1 + SPACESHIPWIDTH -1;
    player.state.y2 = player.state.y1 + SPACESHIPHEIGHT -1;
    player.state.imageLength = SPACESHIPBMP;
    player.state.image = spaceshipImage;
    player.state.width = SPACESHIPWIDTH;
    player.state.height = SPACESHIPHEIGHT;
    player.state.life = 1;
}

// Sets the explosion animations to the correct images
// Inputs: none
// Outputs: none
void Init_Explosions(void){
    explosion[0].x1 = 0;
    explosion[0].y1 = 0;
    explosion[0].x2 = explosion[0].x1 + EXPLOSION1_WIDTH -1;
    explosion[0].y2 = explosion[0].y1 + EXPLOSION1_HEIGHT -1;
    explosion[0].imageLength = EXPLOSION1_BMP;
    explosion[0].image = explosionImage1;
    explosion[0].width = EXPLOSION1_WIDTH;
    explosion[0].height = EXPLOSION1_HEIGHT;

    explosion[1].x1 = 0;
    explosion[1].y1 = 0;
    explosion[1].x2 = explosion[1].x1 + EXPLOSION2_WIDTH -1;
    explosion[1].y2 = explosion[1].y1 + EXPLOSION2_HEIGHT -1;
    explosion[1].imageLength = EXPLOSION2_BMP;
    explosion[1].image = explosionImage2;
    explosion[1].width = EXPLOSION2_WIDTH;
    explosion[1].height = EXPLOSION2_HEIGHT;

    explosion[2].x1 = 0;
    explosion[2].y1 = 0;
    explosion[2].x2 = explosion[2].x1 + EXPLOSION3_WIDTH -1;
    explosion[2].y2 = explosion[2].y1 + EXPLOSION3_HEIGHT -1;
    explosion[2].imageLength = EXPLOSION3_BMP;
    explosion[2].image = explosionImage3;
    explosion[2].width = EXPLOSION3_WIDTH;
    explosion[2].height = EXPLOSION3_HEIGHT;
}


// Displays the starting image
// Inputs: none
// Outputs: none
void Init_StartScreen(void)
{
    int i ;
    setAddress(0,0,239,319);
    writeCmd(0x0022);
    for(i = 0; i < 76800; i++)
    {
        //writeData(startImage[i]);
    }
}

// Displays the time travelled after player has crashed
// Inputs: none
// Outputs: none
void displayEndScreen(void){
    Distance_Stop();                                     // stop distance counter
    Asteroid_Stop();                                     // stop asteroids being generated
    long num = TimerCount;
    char buffer[10];
    char words[] = {"TIME TRAVELLED(s): "};
    char words2[] = {"HIGHSCORE: "};

    sprintf(buffer, "%i", num);
    writeString(words, 15, 30, red, white);
    writeString(buffer, 165,30, red, white);
    writeString(words2, 15, 50, red, white);

    if(num > highscore){
        highscore = num;
    }
    sprintf(buffer, "%i", highscore);

    writeString(buffer, 165,50, red, white);


}


// Updates center coordinate of a sprite
// Inputs: Sprite
// Outputs: none
void getCenter(State *sprite)
{
    sprite->center_x = (sprite->x2 - sprite->x1)/2 + sprite->x1;       // get center x coordinate
    sprite->center_y = (sprite->y2 - sprite->y1)/2 + sprite->y1;       // get center y coordinate
}


// Prints the image of a sprite
// Inputs: Sprite
// Outputs: none
void printBMP(State *sprite){
    int i;
    writeCmd(0x0022);
    for(i=0;i<sprite->imageLength;i++)
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
    int row, column;
    row = 0;
    column = 0;

    for(i=0;i<sprite->imageLength;i++)
    {
        column++;
        if(sprite->image[i] != 0xFFFF)                          // skip drawing if color is 0xFFFF;
        {
            writeCmd(0x0022);
            writeData(sprite->image[i]);
        }
        else
        {
            setCursor(sprite->x1+column, sprite->y1+row);       // manually move the address counter to next location
        }

        if(column == sprite->width )                            // if x address reaches the last pixel increment to next row
        {
            row++;
            column = 0;
        }
    }
}



// Controls the rocket ship using a 10k slide potentiometer
// Inputs: 12 bit ADC sample
// Outputs: none
void playerControl(unsigned int slider){
    int sliderValue= slider*0.05;             // 12 bit ADC - 220pixels/4096values = ~0.0537

    if (player.state.x1 != player.px1)
    {
        if(abs(player.state.x1 - player.px1) > 2)             // debounce rapid changes in the slide potentiometer
        {

            player.state.x1 = sliderValue;
            player.state.x2 = sliderValue + (player.state.width -1);
            setAddress(player.state.x1,player.state.y1,player.state.x2,player.state.y2);
            printBMP(&player.state);
        }
    }
}

// Get previous position of the space ship
// LCD:ADC ratio is 0.0513 (220/4096)
// Inputs: 12 bit ADC sample
// Outputs: none
void getPlayerPosition(unsigned int slider){
    player.px1 = slider*0.05;
}

// Function to animate an asteroid entering the LCD
// Inputs: none
// Outputs: none
void deployAsteroid(void){
    char i;
    int j;
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

            if(y_end < asteroid[i].state.height)        // if astroid has not fully entered screen
            {
                setAddress(x_start, y_start, x_end, y_end);
                writeCmd(0x0022);
                row = row - M;
                asteroid[i].row = row;
                for(j = asteroid[i].state.width*(row); j < (asteroid[i].state.width*asteroid[i].state.height)-1; j++)      // draw row
                {
                    writeData(asteroid[i].state.image[j]);
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
                    setAddress(asteroid[i].state.x1,asteroid[i].state.y1,asteroid[i].state.x2,asteroid[i].state.y2);
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
    asteroid[index].state.imageLength = ASTEROIDBMP_M;
    asteroid[index].state.image = asteroidm;
    asteroid[index].state.height = ASTEROIDHEIGHT_M;
    asteroid[index].state.width = ASTEROIDWIDTH_M;
}

// Displays the 3 animations when the player crashes
// Inputs: Ax, Ay are the center of the image to overlap
// Outputs: none
void displayExplosionAnimation(unsigned short Ax, unsigned short Ay){
    int i;
    for(i = 0; i < 3; i++)
    {
        explosion[i].x1 = Ax - explosion[i].width/2;
        explosion[i].y1 = Ay - explosion[i].height/2;
        explosion[i].x2 = explosion[i].x1 + explosion[i].width -1;
        explosion[i].y2 = explosion[i].y1 + explosion[i].height -1;

        setAddress(explosion[i].x1,explosion[i].y1,explosion[i].x2,explosion[i].y2);
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

    if(distance < radius_A + radius_B)              // circle detection
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

// If collision between player and asteroid then loop infinately
// Inputs: none
// Outputs: none
/*void detectPlayerCollision(void){
    int i;
    for(i=0;i<N; i++)
    {
        if(collision(&player.state, &asteroid[i].state)){
            while(player.state.life == 0){
                if(reset == 1){
                    return;
                }
                displayEndScreen();
                displayExplosionAnimation(player.state.center_x,player.state.center_y);
                playSound();

            }
        }
    }
}*/


void detectPlayerCollision(void){
    int i;
    for(i=0;i<N; i++)
    {
        if(collision(&player.state, &asteroid[i].state)){
            loopEndGame();
        }
    }
}

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
    laser[index].state.y1 = player.state.y1 - LASERBEAM__HEIGHT -3;
    laser[index].state.y2 = player.state.y1 -2;
    laser[index].state.life = 1;
    laser[index].direction = 1;
    laser[index].state.imageLength = LASERBEAM_BMP;
    laser[index].state.image = laserbeam;
    laser[index].state.height = LASERBEAM__HEIGHT;
    laser[index].state.width = LASERBEAM__WIDTH;
    setAddress(laser[index].state.x1, laser[index].state.y1,laser[index].state.x2,laser[index].state.y2);
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
                setAddress(laser[i].state.x1, laser[i].state.y1,laser[i].state.x2,laser[i].state.y2);
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
    semaphore = 1;
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
}
