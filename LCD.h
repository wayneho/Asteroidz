#ifndef LCD_H
#define LCD_H


// 16-bit RGB values for some common colors
#define white          0xFFFF
#define black          0x0000
#define grey           0xF7DE
#define blue           0x001F
#define red            0xF800
#define magenta        0xF81F
#define green          0x07E0
#define cyan           0x7FFF
#define yellow         0xFFE0



// initialization functions
void Init_PLL(void);
void Init_Port(void); //setup GPIO pins
void Init_LCD(void); //base LCD config
void Init_SSI0(void); //setup SSI0
void Init_Analog(void);

// functions to send cmd/data to LCD
void writeCmd(unsigned short cmd);
void writeData(unsigned short data);
void writeReg(unsigned short cmd,unsigned short data);

// helper functions
void clearLCD(unsigned short rgb); //set entire LCD to the color rgb 
void clearArea(unsigned short x1, unsigned short y1,unsigned short x2,unsigned short y2, unsigned short color);     // erase an area on the LCD
void setCursor(unsigned short x,unsigned short y); //set current pixel to x,y
void setAddress(unsigned int x1,unsigned int y1,unsigned int x2,unsigned int y2);      // set an address range
void delayMS(unsigned long ms); // delay in ms using system clock



// functions for time delay using SysTick
void Init_SysTick(void);
void Wait_SysTick(unsigned long delay);

// touchscreen functions
unsigned int getX(void);
unsigned int getY(void);

// function to get ADC sample
unsigned long ADC0(void);


#endif
