#ifndef SPRITES_H
#define SPRITES_H

#include "stdbool.h"

#define N  2                // number of asteroids on the screen at a time (too many will cause lag)
#define M 3                 // speed at which asteroids travel
#define LASERS 3
#define LASERSPEED 1

struct State{
  unsigned short x1;                      // current x1
  unsigned short y1;                      // current y1
  unsigned short x2;                      // current x2
  unsigned short y2;                      // current y2
  const unsigned short *image;            // ptr->image
  unsigned short imageLength;             // image length
  unsigned char life;                     // 0 = dead
  unsigned short height;                  // height of image BMP
  unsigned short width;                   // width of image BMP
  unsigned short center_x;                // center x coordinate of image
  unsigned short center_y;                // center y coordinate of image

};
typedef struct State State;


struct Player {
    State state;
    unsigned short px1;                   // previous x1 coordinate
    //unsigned short px2;                   // previous x2 coordinate
};

typedef struct Player sPlayer;

struct Asteroid{
    State state;
    unsigned short row;                  // row deploy animation

};


typedef struct Asteroid sAsteroid;


struct Laser{
    State state;
    unsigned short direction;           // 1 = up; 0 = down

};

typedef struct Laser sLaser;



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


// helper functions
void printBMP(struct State *sprite);                    // prints a NxM bitmap
void printBMP2(struct State *sprite);                   // prints a NxM bitmap with transparent background (must be white)
void loopGame(void);
bool collision(struct State *A, struct State *B);       // collision detection using bounding circles algorithm
unsigned short randomValue(void);                       // random between 0-189 to place asteroid inside LCD resolution of 240
void getCenter(struct State *sprite);                   // updates the center coordinates of a sprite
void Init_StartScreen(void);

// functions to display simple text
void writeString ( unsigned char word[], unsigned short x, unsigned short y, unsigned short background, unsigned short rgb);
void writeCharacter ( unsigned char character, unsigned short x, unsigned short y, unsigned short background,unsigned short rgb);

// Timer handlers
void Timer0A_Handler(void);
void Timer1A_Handler(void);
void Sound_Handler(void);




#endif
