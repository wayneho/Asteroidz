#include "game.h"
#include "images.h"
#include "math.h"
#include "stdlib.h"
#include "stdio.h"
#include "lcd.h"
#include "tm4c123gh6pm.h"
#include "interrupts.h"


unsigned char semaphore;
unsigned long TimerCount1;
unsigned char explosionCounter;                    // for explosion animation
unsigned int sliderPosition;

unsigned char buffer[10];

sPlayer Player;
State Explosion[3];
sAsteroid Asteroid[N];
sLaser Laser[LASERS];


// set the image and initial position of the rocket ship
void Init_Player(void){
    Player.state.x1 = 110;
    Player.state.y1 = 250;
    Player.state.x2 = Player.state.x1 + SPACESHIPWIDTH -1;
    Player.state.y2 = Player.state.y1 + SPACESHIPHEIGHT -1;
    Player.state.imageLength = SPACESHIPBMP;
    Player.state.image = spaceshipImage;
    Player.state.width = SPACESHIPWIDTH;
    Player.state.height = SPACESHIPHEIGHT;
}


void Init_Explosions(void){
    Explosion[0].x1 = 0;
    Explosion[0].y1 = 0;
    Explosion[0].x2 = Explosion[0].x1 + EXPLOSION1_WIDTH -1;
    Explosion[0].y2 = Explosion[0].y1 + EXPLOSION1_HEIGHT -1;
    Explosion[0].imageLength = EXPLOSION1_BMP;
    Explosion[0].image = explosion1;
    Explosion[0].width = EXPLOSION1_WIDTH;
    Explosion[0].height = EXPLOSION1_HEIGHT;

    Explosion[1].x1 = 0;
    Explosion[1].y1 = 0;
    Explosion[1].x2 = Explosion[1].x1 + EXPLOSION2_WIDTH -1;
    Explosion[1].y2 = Explosion[1].y1 + EXPLOSION2_HEIGHT -1;
    Explosion[1].imageLength = EXPLOSION2_BMP;
    Explosion[1].image = explosion2;
    Explosion[1].width = EXPLOSION2_WIDTH;
    Explosion[1].height = EXPLOSION2_HEIGHT;

    Explosion[2].x1 = 0;
    Explosion[2].y1 = 0;
    Explosion[2].x2 = Explosion[2].x1 + EXPLOSION3_WIDTH -1;
    Explosion[2].y2 = Explosion[2].y1 + EXPLOSION3_HEIGHT -1;
    Explosion[2].imageLength = EXPLOSION3_BMP;
    Explosion[2].image = explosion3;
    Explosion[2].width = EXPLOSION3_WIDTH;
    Explosion[2].height = EXPLOSION3_HEIGHT;
}

// calls the necessary functions to start game
void loopGame(void){
    int i;

    if (semaphore)
    {
        sliderPosition = ADC0_InSeq3();         // get conversion from slide pot
        getPlayerPosition(sliderPosition);      // get previous position of player
        playerControl(sliderPosition);
        moveLaser();
        deployAsteroid();
        moveAsteroid();

        for(i=0;i<N; i++)
        {
            while(collision(&Player.state, &Asteroid[i].state)){
                Timer1A_Stop();
                int num = TimerCount1;
                unsigned char words[] = {"TIME TRAVELLED(s): "};
                sprintf(buffer, "%d", num);
                writeString(words, 15, 30, red, white);
                writeString(buffer, 165,30, red, white);

            }
        }
    }

}

//
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

void getCenter(struct State *sprite)
{
    sprite->center_x = (sprite->x2 - sprite->x1)/2 + sprite->x1;       // get center x coordinate
    sprite->center_y = (sprite->y2 - sprite->y1)/2 + sprite->y1;       // get center y coordinate
}


// display the sprite image on LCD
void printBMP(struct State *sprite){
    int i;
    writeCmd(0x0022);
    for(i=0;i<sprite->imageLength;i++)
    {
        writeData(sprite->image[i]);
    }
}


// print sprites with transparent background
// transparent color = 0xFFFF
// slow
void printBMP2(struct State *sprite){
    int i;
    int row, column;
    row = 0;
    column = 0;
    //writeCmd(0x0022);
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



// control the rocket ship using a 10k slide potentiometer
void playerControl(unsigned int slider){
    int sliderValue= slider*0.0513;             // 12 bit ADC - 220pixels/4096values = ~0.0537


    if (Player.state.x1 != Player.px1)
    {
        if(abs(Player.state.x1 - Player.px1) > 2)             // debounce rapid changes in the slide potentiometer
        {

            Player.state.x1 = sliderValue;
            Player.state.x2 = sliderValue + (Player.state.width -1);
            setAddress(Player.state.x1,Player.state.y1,Player.state.x2,Player.state.y2);
            printBMP(&Player.state);
        }
    }
}

// get previous position of the rocket ship
void getPlayerPosition(unsigned int slider){
    Player.px1 = slider*0.0513;
}

// asteroid entering screen animation
void deployAsteroid(void){
    char i;
    int j;
    unsigned short x_start, x_end,y_start, y_end, row;


    for(i = 0; i < N; i++)
    {
        if(Asteroid[i].state.life == 1)
        {
            x_start = Asteroid[i].state.x1;
            x_end = Asteroid[i].state.x2;
            y_start = Asteroid[i].state.y1;
            y_end = Asteroid[i].state.y2;
            row = Asteroid[i].row;

            if(y_end < Asteroid[i].state.height)
            {
                setAddress(x_start, y_start, x_end, y_end);
                writeCmd(0x0022);
                row = row - M;
                Asteroid[i].row = row;
                for(j = Asteroid[i].state.width*(row); j < (Asteroid[i].state.width*Asteroid[i].state.height)-1; j++)      // draw row
                {
                    writeData(Asteroid[i].state.image[j]);
                }
                y_end = y_end + M;
                Asteroid[i].state.y2 = y_end;

             }
            else
            {
            Asteroid[i].state.y1 = (y_end - Asteroid[i].state.height);
            }
        }
    }


}


// move asteroid vertically down by M pixel(s)
void moveAsteroid(void){
    char i;
    for(i=0; i < N; i++)
    {
        if(Asteroid[i].state.life == 1)
        {
            if(Asteroid[i].row <= M)        // asteroid deploying animation finished
            {
                if(Asteroid[i].state.y1 >= 320)
                {
                    deleteAsteroid(i);
                }
                else
                {
                    Asteroid[i].state.y1 = Asteroid[i].state.y1 + M;
                    Asteroid[i].state.y2 = Asteroid[i].state.y2 + M;
                    clearArea(Asteroid[i].state.x1, Asteroid[i].state.y1-M, Asteroid[i].state.x2 , Asteroid[i].state.y1+1, white);
                    setAddress(Asteroid[i].state.x1,Asteroid[i].state.y1,Asteroid[i].state.x2,Asteroid[i].state.y2);
                    printBMP(&Asteroid[i].state);

                }
            }
        }
    }
}


// asteroid has reached the bottom of the LCD
// remove from array
void deleteAsteroid(unsigned short index){
    Asteroid[index].state.life = 0;
}

// create new astroid starting at a random x location
void addAsteroidMedium(unsigned short index){
    //Asteroid[index].state.x1 = randomValue();
    Asteroid[index].state.x1 = sliderPosition*0.0513 - (ASTEROIDWIDTH_M - SPACESHIPWIDTH);
    //Asteroid[index].state.x1 = 0 ;
    Asteroid[index].state.x2 = Asteroid[index].state.x1 + (ASTEROIDWIDTH_M-1);
    Asteroid[index].state.y1 = 0;
    Asteroid[index].state.y2 = M;
    Asteroid[index].state.life = 1;
    Asteroid[index].row = ASTEROIDHEIGHT_M;
    Asteroid[index].state.imageLength = ASTEROIDBMP_M;
    //Asteroid[index].state.image = asteroidmediumImage;
    Asteroid[index].state.image = asteroidm;
    Asteroid[index].state.height = ASTEROIDHEIGHT_M;
    Asteroid[index].state.width = ASTEROIDWIDTH_M;
}



/*
void addAsteroidLarge(unsigned short index){
    Asteroid[index].state.x1 = randomValue();
    Asteroid[index].state.x2 = Asteroid[index].state.x1 + (ASTEROIDWIDTH_L-1);
    Asteroid[index].state.y1 = 0;
    Asteroid[index].state.y2 = M;
    Asteroid[index].state.life = 1;
    Asteroid[index].row = ASTEROIDHEIGHT_L;
    Asteroid[index].state.imageLength = ASTEROIDBMP_L;
    Asteroid[index].state.image = asteroidLargeImage;
    Asteroid[index].state.height = ASTEROIDHEIGHT_L;
    Asteroid[index].state.width = ASTEROIDWIDTH_L;
}
*/


bool collision(struct State *A, struct State *B)
{
    int i;
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
        for(i = 0; i < 3; i++)
        {
            if(explosionCounter == 1)
            {
                Explosion[2].x1 = Ax - Explosion[2].width/2;
                Explosion[2].y1 = Ay - Explosion[2].height/2;
                Explosion[2].x2 = Explosion[2].x1 + Explosion[2].width -1;
                Explosion[2].y2 = Explosion[2].y1 + Explosion[2].height -1;

                setAddress(Explosion[2].x1,Explosion[2].y1,Explosion[2].x2,Explosion[2].y2);
                printBMP2(&Explosion[2]);
            }
            else
            {
                if (i == 2){
                    explosionCounter = 1;
                }
                Explosion[i].x1 = Ax - Explosion[i].width/2;
                Explosion[i].y1 = Ay - Explosion[i].height/2;
                Explosion[i].x2 = Explosion[i].x1 + Explosion[i].width -1;
                Explosion[i].y2 = Explosion[i].y1 + Explosion[i].height -1;

                setAddress(Explosion[i].x1,Explosion[i].y1,Explosion[i].x2,Explosion[i].y2);
                printBMP2(&Explosion[i]);
                delayMS(100);
            }
        }
        return true;
    }
    else
    {
        return false;
    }
    //return(distance < radius_A + radius_B);                                          // circle detection
    //return((dx*2 < (A->width + B->width)) && (dy*2 < (A->height+B->height)));        // box detection

}

void addLaser(unsigned short index)
{
    getCenter(&Player.state);
    Laser[index].state.x1 = Player.state.center_x - LASERBEAM__WIDTH/2;
    Laser[index].state.x2 = Laser[index].state.x1 + LASERBEAM__WIDTH -1;
    Laser[index].state.y1 = Player.state.y1 - LASERBEAM__HEIGHT -3;
    Laser[index].state.y2 = Player.state.y1 -2;
    Laser[index].state.life = 1;
    Laser[index].direction = 1;
    Laser[index].state.imageLength = LASERBEAM_BMP;
    Laser[index].state.image = laserbeam;
    Laser[index].state.height = LASERBEAM__HEIGHT;
    Laser[index].state.width = LASERBEAM__WIDTH;
    setAddress(Laser[index].state.x1, Laser[index].state.y1,Laser[index].state.x2,Laser[index].state.y2);
    printBMP(&Laser[index].state);
}


void moveLaser(void)
{
    int i;
    for(i = 0; i < LASERS; i++)
    {
        if(Laser[i].state.life == 1)        // hasnt hit an object yet
        {
            if(Laser[i].state.y1 > 0)      // hasnt reached the end of the screen yet
            {
                if(Laser[i].direction == 1)     // being fired from player
                {
                    Laser[i].state.y1 = Laser[i].state.y1 - LASERSPEED;
                    Laser[i].state.y2 = Laser[i].state.y2 - LASERSPEED;
                }
                else
                {
                    Laser[i].state.y1 = Laser[i].state.y1 + LASERSPEED;
                    Laser[i].state.y2 = Laser[i].state.y2 + LASERSPEED;
                }
                setAddress(Laser[i].state.x1, Laser[i].state.y1,Laser[i].state.x2,Laser[i].state.y2);
                printBMP(&Laser[i].state);

            }
            else   // delete laser and clear area
            {
                Laser[i].state.life = 0;
                clearArea(Laser[i].state.x1, Laser[i].state.y1,Laser[i].state.x2+1,Laser[i].state.y2 +1, white);
            }
        }
    }

}


// gives a random value between 0 and 189
// screen width: 240 pixels
// asteroid width: 50 pixels
unsigned short randomValue(void){
    char i;

    srand(ADC0_InSeq3());
    i = rand();
    while(i > 189){
        i = rand();
    }
    return i;
}


void writeString ( unsigned char word [], unsigned short x, unsigned short y, unsigned short textrgb , unsigned short background)
{
    int i =0;
    while ( word [i ]!=0)
    {
        writeCharacter ( word [i],x+8*i ,y, textrgb ,background );
        i++;
    }
}

void writeCharacter ( unsigned char c, unsigned short x, unsigned short y, unsigned short textrgb , unsigned short background)
{
    int i = 0;
    int j = 0;
    for (i = 0; i <8; i ++)
    {
        for (j=0;j <8;j++)
        {
            setCursor (x+j,y+i);
            if ((( font8x8_basic [c][i]>>j)&1) ==1)
            {
                writeReg (0x0022 , textrgb );
            }
                else
                {
                    writeReg (0x0022 ,background);
                }
         }
    }
}


// trigger when timer times out
void Timer0A_Handler(void){
  char i;
  TIMER0_ICR_R = 0x00000001;  // clear interrupt flag
  for(i = 0; i < N; i++)
  {
      if(Asteroid[i].state.life == 0)
      {
          addAsteroidMedium(i);
          return;                   // found open slot for new asteroid
      }

  }
}

// keep track of time traveled
void Timer1A_Handler(void){

  TIMER1_ICR_R = 0x00000001;  // clear interrupt flag
  TimerCount1++;

}

void GPIOPortA_Handler(void){
    int i;
    GPIO_PORTA_ICR_R = 0x80;    // clear interrupt flag
    delayMS(5);                // debounce switch
    if(SW0 == 0)
    {
        for(i = 0; i < 10; i++)
        {
            if(Laser[i].state.life == 0)
            {
                addLaser(i);
                return;
            }
        }
    }

}

// touchscreen interrupt for starting game
void GPIOPortE_Handler(void){
    int i;
    unsigned char *word[] = {"3", "2", "1","START"};
    clearLCD(white);

    for(i = 0; i < 4; i++)
    {
        writeString(word[i], 120, 160, red, white);
        delayMS(1000);
    }
    clearLCD(white);
    GPIO_PORTE_ICR_R = 0x08;    // clear interrupt flag
    semaphore = 1;
    Timer0A_Start();
    Timer1A_Start();
}
