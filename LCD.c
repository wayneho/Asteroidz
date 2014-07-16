#include "LCD.H"
#include "tm4c123gh6pm.h"

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

// SPI interface
#define DIN											(*((volatile unsigned long *)0x40004040))	// PA4; SSI0Rx
#define DOUT										(*((volatile unsigned long *)0x40004080))  	// PA5; SSI0Tx
#define DCLK										(*((volatile unsigned long *)0x40004010))	// PA2; SSI0Clk
	

// resolution is 240x320
#define MAX_X										 240
#define MAX_Y										 320

// coordinates for touch screen
unsigned int TS_X;
unsigned int TS_Y;

// Initialize system clock
// Inputs: none
// Outputs: none
void Init_SysTick(void){
  NVIC_ST_CTRL_R = 0;                               // disable SysTick during setup
  NVIC_ST_CTRL_R = 0x00000005;                      // enable SysTick with core clock
}

// System clock time delay
// Inputs: delay
// Outputs: none
void Wait_SysTick(unsigned long delay){
  NVIC_ST_RELOAD_R = delay-1;                       // number of counts to wait
  NVIC_ST_CURRENT_R = 0;                            // any value written to CURRENT clears
  while((NVIC_ST_CTRL_R&0x00010000)==0){            // wait for count flag
  }
}

// Delay function based on the system clock
// Inputs: delay in millisecs
// Outputs: none
void delayMS(unsigned long ms){
  unsigned long i;
  Init_SysTick();
  for(i=0; i<ms; i++){
      Wait_SysTick(80000);                            // wait 1ms
  }
}
// Initalizes PLL for 80Mhz
// Inputs: none
// Outputs: none
void Init_PLL(void){
    SYSCTL_RCC2_R |=  0x80000000;                   // use RCC2
    SYSCTL_RCC2_R |=  0x00000800;                   // BYPASS2, PLL bypass
    SYSCTL_RCC_R = (SYSCTL_RCC_R &~0x000007C0)      // clear XTAL field, bits 10-6
                     + 0x00000540;                  // 10101, configure for 16 MHz crystal
    SYSCTL_RCC2_R &= ~0x00000070;                   // configure for main oscillator source
    SYSCTL_RCC2_R &= ~0x00002000;                   // activate PLL by clearing PWRDN
    SYSCTL_RCC2_R |= 0x40000000;                    // use 400 MHz PLL
    SYSCTL_RCC2_R = (SYSCTL_RCC2_R&~ 0x1FC00000)    // clear system clock divider
                      + (4<<22);                    // configure for 80 MHz clock
    while((SYSCTL_RIS_R&0x00000040)==0){};          // wait for PLLRIS bit
    SYSCTL_RCC2_R &= ~0x00000800;                   // enable use of PLL by clearing BYPASS
}

// Port Initialisation
// Inputs: none
// Outputs: none
void Init_Port(void) {
	volatile unsigned long delay;
	SYSCTL_RCGC2_R |= 0x0000001F;     					// activate clock for Port A, B, C, D, E
	SYSCTL_RCGC1_R |= 0x00000010;						// activate clock for SSI0
	SYSCTL_RCGC0_R |= 0x00010000;   					// activate clock for ADC0

	delay = SYSCTL_RCGC2_R;								// allow time for clock to start


	GPIO_PORTC_LOCK_R = 0x4C4F434B;	 					// unlock GPIO Port C
	GPIO_PORTD_LOCK_R = 0x4C4F434B;                     // unlock GPIO Port D
	GPIO_PORTD_CR_R |= 0xFF;                            // allow registers in Port D to be written to

	GPIO_PORTA_AMSEL_R &= ~0xFC;                        // disable analog mode for PA 7-2
	GPIO_PORTB_AMSEL_R &= ~0xFF;                        // disable analog mode for PB 7-0
	GPIO_PORTC_AMSEL_R &= ~0xF0;                        // disable analog mode for PC 7-4
	GPIO_PORTD_AMSEL_R &= ~0xFF;                        // disable analog mode for PD 7-0
	GPIO_PORTE_AMSEL_R &= ~0x3D;                        // disable analog mode for PE 5,4,3,2,0
	GPIO_PORTE_AMSEL_R |= 0x02;							// enable analog mode for PE1
	
	GPIO_PORTA_AFSEL_R &= 0x80;                         // disable alternative function for PA 7
	GPIO_PORTA_AFSEL_R |= 0x3C;       				  	// enable alternative function for PA 5,4,3,2
	GPIO_PORTB_AFSEL_R &= ~0xFF;                        // disable alternative function for PB 7-0
	GPIO_PORTC_AFSEL_R &= ~0xF0;                        // disable alternative function for PC 7-4
	GPIO_PORTD_AFSEL_R &= ~0xFF;                        // disable alternative function for PD 7-0
	GPIO_PORTE_AFSEL_R &= ~0x3D;                        // disable alternative function for PE 5,4,3,2,0
	GPIO_PORTE_AFSEL_R |= 0x02;							// enable alternative function for PE1
	
	GPIO_PORTA_PCTL_R = 0x00222200;   					// SSI0 functions for PA 5,4,3,2
	GPIO_PORTB_PCTL_R = 0x00000000;                     // GPIO functions for PB
	GPIO_PORTC_PCTL_R &= ~0xFFFF0000;                   // GPIO functions for PC 7-4
	GPIO_PORTD_PCTL_R = 0x00000000;                     // GPIO functions for PD
	GPIO_PORTE_PCTL_R = 0x00000000;                     // GPIO functions for PE
	
	GPIO_PORTA_DIR_R &= 0x80;                           // input for PA 7; output for PA 6-0
	GPIO_PORTB_DIR_R |= 0xFF;                           // output for PB 7-0
	GPIO_PORTC_DIR_R |= 0xF0;                           // output for PC 7-4
	GPIO_PORTD_DIR_R |= 0xFF;                           // output for PD 7-0
	GPIO_PORTE_DIR_R  = 0x35;							// input for PE 1,3; output for PE 5,4,2,0

	
	GPIO_PORTA_PUR_R |= 0xC0;							// enable pull up resistor for PA 7, 6
	GPIO_PORTE_PUR_R |= 0x08;                           // enable pull up resistor for PE 3
	
	GPIO_PORTA_DEN_R |= 0x80;          					// enable digital I/O for PA 7-4
	GPIO_PORTB_DEN_R |= 0xFF;                           // enable digital I/O for PB 7-0
	GPIO_PORTC_DEN_R |= 0xF0;                           // enable digital I/O for PC 7-4
	GPIO_PORTD_DEN_R |= 0xFF;                           // enable digital I/O for PD 7-0
	GPIO_PORTE_DEN_R |= 0x3D;                           // enable digital I/O for PE 5,4,3,2,0
	
}



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

// Initalizes SSI0 at 2Mhz
// Inputs: none
// Outputs: none
void Init_SSI0(void){
	SSI0_CR1_R = 0x0;					// clear SSE bit
	SSI0_CR1_R = 0x0;					// set SSI0 as master
	SSI0_CC_R = 0x0;					// use system Clock
	SSI0_CPSR_R = 0x08;					// set prescale: 2Mhz = 16Mhz /8 => 8 = PRE*(1+CR) => PRE = 8, CR = 1
	SSI0_CR0_R = 0x00000007;			// SPH=SPO=0; Freescale; 8bit data
	SSI0_CR1_R |= 0x2;					// enable SSI
}

// Initalizes ADC0 - Sequencer 3
// Inputs: none
// Outputs: none
void Init_Analog(void){

	SYSCTL_RCGC0_R |= 0x00000200;  	    //  analog sample speed: 500k
	                                    //    Value Description
	                                    //    0x3 1M samples/second
	                                    //    0x2 500K samples/second
	                                    //    0x1 250K samples/second
	                                    //    0x0 125K samples/second

	ADC0_SSPRI_R = 0x0123;				// set sequencer 3 to highest priority
	ADC0_ACTSS_R &= ~0x0008;			// disable sequencer 3 for configuration
	ADC0_EMUX_R &= ~0xF000;				// seq3 continuously sample
	ADC0_SSMUX3_R &= ~0x000F;       	// clear SS3 field
	ADC0_SSMUX3_R += 2;             	// set channel Ain2 (PE1)
	ADC0_SSCTL3_R = 0x0006;         	// no TS0 D0, yes IE0 END0
	ADC0_ACTSS_R |= 0x0008;         	// enable sample sequencer 3

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
void setAddress(unsigned short x1,unsigned short y1,unsigned short x2,unsigned short y2)
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
	setAddress(0,0,239,319);
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
    setAddress(x1,y1,x2,y2);
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

//------------ADC------------
// Busy-wait analog to digital conversion
// Inputs: none
// Outputs: 12-bit result of ADC conversion
unsigned long ADC0(void){
  unsigned long result;
  ADC0_PSSI_R = 0x0008;                // 1) initiate SS3
  while((ADC0_RIS_R&0x08)==0){};       // 2) wait for conversion done
  result = ADC0_SSFIFO3_R&0xFFF;       // 3) read result
  ADC0_ISC_R = 0x0008;                 // 4) acknowledge completion
  return result;
}



unsigned long Init_EEPROM(void){
    volatile unsigned long delay;
    unsigned long status;
    SYSCTL_RCGCEEPROM_R |= 0x1;        // enable EEPROM clock
    delay = SYSCTL_RCGC2_R;            // allow time for clock to start

    check_eeprom_done();               // wait for WORKING bit in EEDONE to clear before accessing any EEPROM registers

    status = EEPROM_EESUPP_R;

    if(status & (EEPROM_EESUPP_PRETRY | EEPROM_EESUPP_ERETRY)){             // check for errors
        EEPROM_EESUPP_R = EEPROM_EESUPP_START;                              // reset EEPROM
        check_eeprom_done();                                                // wait for EEPROM to finish resetting

        status = EEPROM_EESUPP_R;                                           // read status register again
        if(status & (EEPROM_EESUPP_PRETRY | EEPROM_EESUPP_ERETRY)){
            return(INIT_EEPROM_ERROR);
        }
        else{
            return(INIT_EEPROM_RETRY);
        }
    }

    EEPROM_EEBLOCK_R = 0x2;             // select block 1
    EEPROM_EEOFFSET_R = 0x0;            // select offset 0
    EEPROM_EEPROT_R &= ~0x0F;           // allow write and read
    return(INIT_EEPROM_GOOD);

}

void check_eeprom_done(void){
    while(EEPROM_EEDONE_R == 0x1){};            // poll EEDONE register
}

void write_sector(unsigned char sector, unsigned char offset, unsigned long *data){
    check_eeprom_done();                // check if EEPROM is idle first
    EEPROM_EEBLOCK_R = sector;          // select block 1
    EEPROM_EEOFFSET_R = offset;         // select offset 0
    EEPROM_EERDWR_R = *data;            // write highscore to EEPROM
    check_eeprom_done();                // check if EEPROM is done writing

}

unsigned long read_sector(unsigned char sector, unsigned char offset){
    check_eeprom_done();                // check if EEPROM is idle first
    EEPROM_EEBLOCK_R = sector;          // select block 1
    EEPROM_EEOFFSET_R = offset;         // select offset 0
    check_eeprom_done();                // check if EEPROM is done reading
    return EEPROM_EERDWR_R;             // read highscore from EEPROM
}

void erase_sector(unsigned char sector, unsigned char offset){
    check_eeprom_done();                // check if EEPROM is idle first
    EEPROM_EEBLOCK_R = sector;          // select block 1
    EEPROM_EEOFFSET_R = offset;         // select offset 0
    EEPROM_EERDWR_R = 0;                // write highscore to EEPROM
    check_eeprom_done();                // check if EEPROM is done writing
}



