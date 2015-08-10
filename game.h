#ifndef SPRITES_H
#define SPRITES_H

#include "stdbool.h"

#define FB_BASE         0x00020000        // frame buffer located at address 0x00020000 - 0x00032BFF
#define FB_SIZE         76800

extern volatile unsigned char start;                               //flag to start game after screen has been touched
extern volatile unsigned char reset;

//  This struct holds variables for object information such as:
//  coordinates, life, sprite image, etc..
typedef struct{
  unsigned char x1;                       // current x1
  unsigned short y1;                      // current y1
  unsigned char x2;                       // current x2
  unsigned short y2;                      // current y2
  const unsigned char *image;            // ptr->image
  unsigned short imageSize;               // image size
  unsigned short life;                    // 0 = dead
  unsigned short height;                  // height of image BMP
  unsigned short width;                   // width of image BMP
  unsigned short center_x;                // center x coordinate of image
  unsigned short center_y;                // center y coordinate of image
} State;


//  This struct inherits from the struct State
//  item: 0 = no items; 1 = shield
//	mode: 0 = default game mode; 1 = bonus mode
typedef struct {
    State state;
    unsigned char item;
    unsigned char mode;

} Player;

//  This struct inherits from the struct State and contains the row for asteroids
//  entering the screen animation

typedef struct{
    State state;
    unsigned short row;                  // row deploy animation
} Asteroid;

extern State explosion[];
extern Asteroid asteroid[];
extern Player player;
extern State laser[];

// player functions
void Init_Player(void);
void playerControl(unsigned int x, unsigned int y);
void getPlayerPosition(unsigned int);

// asteroid functions
void Init_Asteroids(void);
void deployAsteroid(void);                              // animation for astroids entering the LCD
void addAsteroidMedium(unsigned short index);
void deleteAsteroid(unsigned short index);
void moveAsteroid(void);
void deleteSprite(State *sprite);

// explosion function
void Init_Explosions(void);

//powerup functions
void Init_PowerUp(void);
void spawnPowerUp(unsigned char index);
void movePowerUp(void);
void PowerUp_Status(void);



// functions to display images and animations
void printBMP(State *sprite);                    // prints a NxM bitmap
void printBMP2(State *sprite);                   // prints a NxM bitmap with transparent background (must be white)
void Init_StartScreen(void);
void displayCountDown(void);
void displayEndScreen(void);
void displayExplosionAnimation(unsigned short x, unsigned short y);
void displayTime(void);

// functions for detecting a collision
bool collision(State *A, State *B);              // collision detection using bounding circles algorithm
void detectPlayerCollision(void);

// function to create sound
void playSound(void);

// functions to display an 8x8 string
void writeString (char word[], unsigned short x, unsigned short y, unsigned short background, unsigned short rgb);
void writeCharacter (unsigned char character, unsigned short x, unsigned short y, unsigned short background,unsigned short rgb);

// helper functions
unsigned short randomValue(unsigned char range);
void getCenter(State *sprite);
void resetGame(void);
void loopEndGame(void);
void write_highscore(unsigned int score);
unsigned long read_highscore(void);
void level_two(void);
void level_one(void);

// Interrupt handlers
void Asteroid_Handler(void);
void Distance_Handler(void);
void Sound_Handler(void);
void GPIOPortA_Handler(void);
void Touchscreen_Handler(void);



#endif
