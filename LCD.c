#include "LCD.H"
#include "tm4c123gh6pm.h"
#include "driver.h"

// 16 to 8 bit 8080-series parallel interface
#define LCD_DataBus						        GPIO_PORTB_DATA_R 				                // PB 7-0 = DIN 7-0
#define LCD_RS			 						(*((volatile unsigned long *)0x40007010))	  	// PD2; data = 1, command = 0
#define LCD_RD 									(*((volatile unsigned long *)0x40007020))		// PD3; data read latch signal when low
#define LCD_WR  								(*((volatile unsigned long *)0x40007100))		// PD6; write = 0, read = 1
#define LCD_CS  		 						(*((volatile unsigned long *)0x40007200))		// PD7; active low
#define LCD_RESET								(*((volatile unsigned long *)0x40024040))		// PE4; active low
#define LCD_BL									(*((volatile unsigned long *)0x40024080)) 		// PE5; active low
#define DEN										(*((volatile unsigned long *)0x40024004))		// PE0; active low
#define DDIR									(*((volatile unsigned long *)0x40004100))		// PA6;
#define DLE										(*((volatile unsigned long *)0x40024010))		// PE2;	active low
#define TIRQ									(*((volatile unsigned long *)0x40024020))  		// PE3; active low
#define SW0                                     (*((volatile unsigned long *)0x40004200))       // PA7; active low

// touchscreen interface  
#define TS_CS										(*((volatile unsigned long *)0x40004020))   // PA3; active low

// resolution is 240x320
#define MAX_X										 240
#define MAX_Y										 320

// coordinates for touch screen
unsigned int TS_X;
unsigned int TS_Y;


// Initialisation sequence to start LCD
// Inputs: none
// Outputs: none
void Init_LCD(void)
{
	
	//reset lcd and turn on back light
	LCD_RESET = 0xFF;
	delayMS(5);
	LCD_RESET = 0x00;
	delayMS(15);
	LCD_RESET = 0xFF;
	LCD_CS=0xFF;
	LCD_RD=0xFF;
	LCD_WR=0xFF;
	delayMS(20);
	
	
	//backlight
	LCD_BL = 0xFF;
	
	writeReg(0x0000,0x0001);    delayMS(5);   /* Enable LCD Oscillator */
	writeReg(0x0003,0xA8A4);    delayMS(5);   // Power control(1)
	writeReg(0x000C,0x0000);    delayMS(5);   // Power control(2)
	writeReg(0x000D,0x080C);    delayMS(5);   // Power control(3)
	writeReg(0x000E,0x2B00);    delayMS(5);   // Power control(4)
	writeReg(0x001E,0x00B0);    delayMS(5);   // Power control(5)
	writeReg(0x0001,0x2B3F);    delayMS(5);   // Driver Output Control /* 320*240 0x2B3F */
	writeReg(0x0002,0x0600);    delayMS(5);   // LCD Drive AC Control
	writeReg(0x0010,0x0000);    delayMS(5);   // Sleep Mode off
	writeReg(0x0011,0x6070);    delayMS(5);   // Entry Mode
	writeReg(0x0005,0x0000);    delayMS(5);   // Compare register(1)
	writeReg(0x0006,0x0000);    delayMS(5);   // Compare register(2)
	writeReg(0x0016,0xEF1C);    delayMS(5);   // Horizontal Porch
	writeReg(0x0017,0x0003);    delayMS(5);   // Vertical Porch
	writeReg(0x0007,0x0233);    delayMS(5);   // Display Control     // 0x0133 - 0x0233
	writeReg(0x000B,0x0000);    delayMS(5);   // Frame Cycle control
	writeReg(0x000F,0x0000);    delayMS(5);   // Gate scan start position
	writeReg(0x0041,0x0000);    delayMS(5);   // Vertical scroll control(1)
	writeReg(0x0042,0x0000);    delayMS(5);   // Vertical scroll control(2)
	writeReg(0x0048,0x0000);    delayMS(5);   // First window start
	writeReg(0x0049,0x013F);    delayMS(5);   // First window end
	writeReg(0x004A,0x0000);    delayMS(5);   // Second window start
	writeReg(0x004B,0x0000);    delayMS(5);   // Second window end
	writeReg(0x0044,0xEF00);    delayMS(5);   // Horizontal RAM address position
	writeReg(0x0045,0x0000);    delayMS(5);   // Vertical RAM address start position
	writeReg(0x0046,0x013F);    delayMS(5);   // Vertical RAM address end position
	writeReg(0x0030,0x0707);    delayMS(5);   // gamma control(1)
	writeReg(0x0031,0x0204);    delayMS(5);   // gamma control(2)
	writeReg(0x0032,0x0204);    delayMS(5);   // gamma control(3)
	writeReg(0x0033,0x0502);    delayMS(5);   // gamma control(4)
	writeReg(0x0034,0x0507);    delayMS(5);   // gamma control(5)
	writeReg(0x0035,0x0204);    delayMS(5);   // gamma control(6)
	writeReg(0x0036,0x0204);    delayMS(5);   // gamma control(7)
	writeReg(0x0037,0x0502);    delayMS(5);   // gamma control(8)
	writeReg(0x003A,0x0302);    delayMS(5);   // gamma control(9)
	writeReg(0x003B,0x0302);    delayMS(5);   // gamma control(10)
	writeReg(0x0023,0x0000);    delayMS(5);   // RAM write data mask(1)
	writeReg(0x0024,0x0000);    delayMS(5);   // RAM write data mask(2)
	writeReg(0x0025,0x8000);    delayMS(5);   // Frame Frequency
	writeReg(0x004f,0);                       // Set GDDRAM Y address counter
	writeReg(0x004e,0);                       // Set GDDRAM X address counter
	
	delayMS(5);
}

// writes a command to the databus
// Inputs: 16bit command
// Outputs: none
void writeCmd(unsigned short cmd){
	LCD_RS = 0x00;
	LCD_CS = 0x00;
	DEN = 0x00;
	DDIR = 0xFF;
	DLE = 0xFF;
	LCD_DataBus	= cmd & 0x00FF;		        // transfer lower byte
	DLE = 0x00;
	LCD_DataBus = (cmd & 0xFF00) >> 8;      // transfer upper byte
	LCD_WR = 0x00;
	LCD_WR = 0xFF;
	LCD_CS = 0xFF;
}
// writes data to the databus
// Inputs: 16bit data
// Outputs: none
void writeData(unsigned short data){
	LCD_RS = 0xFF;
	LCD_CS = 0x00;
	DEN = 0x00;
	DDIR = 0xFF;
	DLE = 0xFF;
	LCD_DataBus	= data & 0x00FF;
	DLE = 0x00;
	LCD_DataBus = (data & 0xFF00) >> 8;
	LCD_WR = 0x00;
	LCD_WR = 0xFF;
	LCD_CS = 0xFF;
}

// comebines writeCmd and writeDat
// Inputs: command and data
// Outputs: none
void writeReg(unsigned short cmd,unsigned short data)
{
	writeCmd(cmd);
	writeData(data);
}

// Sets the beginning address in the graphics display data ram (GDDRAM)
// Inputs: x1,y1
// Outputs: none
void setCursor(unsigned short x,unsigned short y)
{
	writeReg(0x004E, x);
	writeReg(0x004F, y);
}

// Sets the address window in the GDDRAM
// Inputs: x1,y1,x2,y2 coordinates
// Outputs: none
void setWindow(unsigned short x1,unsigned short y1,unsigned short x2,unsigned short y2)
{
    writeReg(0x0044,(x2<<8)+x1);
    writeReg(0x0045,y1);
    writeReg(0x0046,y2);
    writeReg(0x004e,x1);
    writeReg(0x004f,y1);
}

// Clears the entire LCD screen
// Inputs: color of background
// Outputs: none
void clearLCD(unsigned short rgb){
	unsigned int i;
	setWindow(0,0,239,319);
	writeCmd(0x0022);
	for(i = 0; i < (MAX_X*MAX_Y); i++)
	{
		writeData(rgb);
	}
}

// Clears a certain area on the LCD screen
// Inputs: x1,y1,x2,y2 coordinates and color of area to be cleared
// Outputs: none
void clearArea(unsigned short x1, unsigned short y1,unsigned short x2,unsigned short y2, unsigned short color)
{
    unsigned int i;
    unsigned short dx, dy;
    setWindow(x1,y1,x2,y2);
    dx = x2-x1;
    dy = y2-y1;
    writeCmd(0x0022);
    for(i = 0; i < (dx*dy); i++)
    {
       writeData(color);
    }
}

// Requests the x coordinate from the touchscreen controller
// Inputs: none
// Outputs: 12-bit conversion
unsigned long getX(void){
	unsigned int coordinate = 0;
	//TS_CS	 = 0x00;										// set touchscreen chip select
	SSI0_DR_R =  0xD0;										// send control byte (differential mode, 12bit conversion)
	SSI0_DR_R =  0x00;
	SSI0_DR_R =  0x00;
	while(SSI0_SR_R&0x10);									// loop while busy
	coordinate = SSI0_DR_R;
	coordinate = (coordinate <<8) + SSI0_DR_R;
	coordinate = (coordinate <<8) + SSI0_DR_R;
	//TS_CS	 = 0xFF;
	return coordinate;
}

// Requests the y coordinate from the touchscreen controller
// Inputs: none
// Outputs: 12-bit ADC conversion
unsigned long getY(void){
	unsigned int coordinate = 0;
	//TS_CS	 = 0x00;											// set touchscreen chip select
	SSI0_DR_R =  0x90;											// send control byte (differential mode, 12bit conversion)
	SSI0_DR_R =  0x00;
	SSI0_DR_R =  0x00;
	while(SSI0_SR_R&0x10);										// loop while busy
	coordinate = SSI0_DR_R;
	coordinate = (coordinate <<8) + SSI0_DR_R;
	coordinate = (coordinate <<8) + SSI0_DR_R;
	//TS_CS	 = 0xFF;
	return coordinate;
}
