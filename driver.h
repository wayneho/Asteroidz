#ifndef DRIVER_H
#define DRIVER_H

#define INIT_EEPROM_SUCCESS     0
#define INIT_EEPROM_RETRY   	1
#define INIT_EEPROM_ERROR   	2

// initialization functions
void Init_PLL(void);				//setup PLL (80Mhz)
void Init_Port(void);       		//setup GPIO pins
void Init_SSI0(void);       		//setup SSI0
void Init_Analog(void);				//setup ADC0
unsigned long Init_EEPROM(void);	//setup EEPROm
void Init_DMA(void);				//setup DMA

// functions for time delay using SysTick
void Init_SysTick(void);
void Wait_SysTick(unsigned long delay);
void delayMS(unsigned long ms);            // delay in milliseconds

// function to get ADC sample
unsigned long ADC0(void);
unsigned long ADC1(void);

//EEPROM functions
void check_eeprom_done(void);
void write_eeprom(unsigned char block, unsigned char offset, unsigned long *data);
unsigned long read_eeprom(unsigned char block, unsigned char offset);
void erase_eeprom(unsigned char block, unsigned char offset);

//Flash memory functions
void check_flash_done(void);
void write_flash(unsigned long *data, unsigned long address);
void write_flash_buffer(unsigned long *data,unsigned long address, unsigned char offset);

// DMA functions
void Set_DMA_Base_Address(unsigned long address);
void config_DMA_channel(unsigned char channel, unsigned long source, unsigned long destination, unsigned long control_word);
void start_DMA_transfer(void);


#endif
