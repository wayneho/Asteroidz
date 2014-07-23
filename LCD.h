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

#define INIT_EEPROM_GOOD    0
#define INIT_EEPROM_RETRY   1
#define INIT_EEPROM_ERROR   2



// initialization functions
void Init_PLL(void);
void Init_Port(void);       //setup GPIO pins
void Init_LCD(void);        //base LCD config
void Init_SSI0(void);       //setup SSI0
void Init_Analog(void);
unsigned long Init_EEPROM(void);
void Init_DMA(void);

// functions to send cmd/data to LCD
void writeCmd(unsigned short cmd);
void writeData(unsigned short data);
void writeReg(unsigned short cmd,unsigned short data);

// helper functions
void clearLCD(unsigned short rgb);                                                                                  //set entire LCD to the color rgb
void clearArea(unsigned short x1, unsigned short y1,unsigned short x2,unsigned short y2, unsigned short color);     // erase an area on the LCD
void setCursor(unsigned short x,unsigned short y);                                                                  //set current pixel to x,y
void setAddress(unsigned short x1,unsigned short y1,unsigned short x2,unsigned short y2);                                   // set an address range
void delayMS(unsigned long ms);                                                                                     // delay in ms using system clock

// functions for time delay using SysTick
void Init_SysTick(void);
void Wait_SysTick(unsigned long delay);

// touchscreen functions
unsigned long getX(void);
unsigned long getY(void);

// function to get ADC sample
unsigned long ADC0(void);

//EEPROM functions
void check_eeprom_done(void);
void write_sector(unsigned char sector, unsigned char offset, unsigned long *data);
unsigned long read_sector(unsigned char sector, unsigned char offset);
void erase_sector(unsigned char sector, unsigned char offset);

//Flash memory functions
void check_flash_done(void);
void write_flash(unsigned long *data, unsigned long address);
void write_flash_buffer(unsigned long *data,unsigned long address, unsigned char offset);
unsigned long read_flash(unsigned long address);
void mass_erase_flash(void);

// DMA functions
void Set_DMA_Base_Address(unsigned long address);
void config_DMA_channel(unsigned char channel, unsigned long source, unsigned long destination, unsigned long control_word);

#endif
