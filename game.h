#ifndef SPRITES_H
#define SPRITES_H

#include "stdbool.h"


#define LASERS          6
#define LASERSPEED      2
#define FB_BASE         0x00020000        // frame buffer located at address 0x00020000 - 0x00032BFF
#define FB_SIZE         76800

extern unsigned char start;                               //flag to start game after screen has been touched
extern unsigned int sliderPosition;                       //position of slider pot (0-4096)
extern unsigned char reset;

//  this struct holds variables for object information such as:
//  coordinates, life, sprite image, etc..
typedef struct{
  unsigned char x1;                      // current x1
  unsigned short y1;                      // current y1
  unsigned char x2;                      // current x2
  unsigned short y2;                      // current y2
  const unsigned short *image;             // ptr->image
  unsigned short imageSize;               // image size
  unsigned short life;                    // 0 = dead
  unsigned short height;                  // height of image BMP
  unsigned short width;                   // width of image BMP
  unsigned short center_x;                // center x coordinate of image
  unsigned short center_y;                // center y coordinate of image
} State;


//  this struct inherits from the struct State and contains the previous coordinate
//  to debounce rapid changes in the pot slider
typedef struct {
    State state;
    unsigned short px1;                   // previous x1 coordinate
    //unsigned short px2;                 // previous x2 coordinate
} Player;


//  this struct inherits from the struct State and contains the row for asteroids
//  entering the screen animation

typedef struct{
    State state;
    unsigned short row;                  // row deploy animation
} Asteroid;


//  this struct inherits from the struct State and contains the direction of the
//  laser beam
typedef struct{
    State state;
    unsigned short direction;           // 1 = up; 0 = down
} Laser;


extern Asteroid asteroid[];
extern State explosion[];
extern Player player;

// player functions
void Init_Player(void);
void playerControl(unsigned int);
void getPlayerPosition(unsigned int);

// asteroid functions
void deployAsteroid(void);                              // animation for astroids entering the LCD
void addAsteroidMedium(unsigned short index);           // updates array with new astroid state
void deleteAsteroid(unsigned short index);
void moveAsteroid(void);

// explosions
void Init_Explosions(void);

// bullets
void addLaser(unsigned short index);
void moveLaser(void);

// aliens
void Init_Alien(void);

// helper functions
void printBMP(State *sprite);                    // prints a NxM bitmap
void printBMP2(State *sprite);                   // prints a NxM bitmap with transparent background (must be white)
void printBMP3(State *sprite);
bool collision(State *A, State *B);              // collision detection using bounding circles algorithm
unsigned short randomValue(void);                // random between 0-189 to place asteroid inside LCD resolution of 240
void getCenter(State *sprite);                   // updates the center coordinates of a sprite
void Init_StartScreen(void);
void displayEndScreen(void);                     // function to display score at the end of the game
void detectPlayerCollision(void);
void displayCountDown(void);
void resetGame(void);
void displayExplosionAnimation(unsigned short x, unsigned short y);
void loopEndGame(void);
void write_highscore(unsigned long score);
unsigned long read_highscore(void);

// function to create sound
void playSound(void);

// functions to display simple text
void writeString (char word[], unsigned short x, unsigned short y, unsigned short background, unsigned short rgb);
void writeCharacter (unsigned char character, unsigned short x, unsigned short y, unsigned short background,unsigned short rgb);


// Interrupt handlers
void Asteroid_Handler(void);
void Distance_Handler(void);
void Sound_Handler(void);
void GPIOPortA_Handler(void);
void Touchscreen_Handler(void);



#endif
