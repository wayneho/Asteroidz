#ifndef SPRITES_H
#define SPRITES_H

#include "stdbool.h"

#define N  4            // number of asteroids on the screen at a time (too many will cause lag)
#define M 2             // speed at which asteroids travel
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
  unsigned short height;                  // 0 = medium, 1 = large
  unsigned short width;
  unsigned short center_x;
  unsigned short center_y;

};
typedef struct State State;


struct Player {
    State state;
    unsigned short px1;                   // previous x1 coordinate
    unsigned short px2;                   // previous x2 coordinate
};

typedef struct Player sPlayer;

struct Asteroid{
    State state;
    unsigned short row;                  // row deploy animation

};


typedef struct Asteroid sAsteroid;


struct Laser{
    State state;
    unsigned short direction;           // 1 = up, 0 = down

};

typedef struct Laser sLaser;



// player functions
void Init_Player(void);
void playerControl(unsigned int);
void getPlayerPosition(unsigned int);

// asteroid functions

void deployAsteroid(void);
void addAsteroidMedium(unsigned short index);
void addAsteroidLarge(unsigned short index);
void deleteAsteroid(unsigned short index);
unsigned short getAsteroidPosition(unsigned short index);     // return y2 coordinate
void moveAsteroid(void);

// explosions
void Init_Explosions(void);

// bullets
void addLaser(unsigned short index);
void moveLaser(void);


// helper functions
void printBMP(struct State *sprite);
void printBMP2(struct State *sprite);       // for printing transparent backgrounds
void loopGame(void);
bool collision(struct State *A, struct State *B);
unsigned short randomValue(void);       // random between 0-189 to place asteroid inside LCD resolution of 240
void getCenter(struct State *sprite);
void Init_StartScreen(void);

// functions to display simple text
void writeString ( unsigned char word[], unsigned short x, unsigned short y, unsigned short background, unsigned short rgb);
void writeCharacter ( unsigned char character, unsigned short x, unsigned short y, unsigned short background,unsigned short rgb);

#endif
