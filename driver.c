#include "tm4c123gh6pm.h"
#include "driver.h"

// SSI interface
#define DIN											(*((volatile unsigned long *)0x40004040))	// PA4; SSI0Rx
#define DOUT										(*((volatile unsigned long *)0x40004080))  	// PA5; SSI0Tx
#define DCLK										(*((volatile unsigned long *)0x40004010))	// PA2; SSI0Clk


// Initialize system clock
void Init_SysTick(void){
  NVIC_ST_CTRL_R = 0;                               // disable SysTick during setup
  NVIC_ST_CTRL_R = 0x00000005;                      // enable SysTick with core clock
}

// System clock time delay
void Wait_SysTick(unsigned long delay){
  NVIC_ST_RELOAD_R = delay-1;                       // number of counts to wait
  NVIC_ST_CURRENT_R = 0;                            // any value written to CURRENT clears
  while((NVIC_ST_CTRL_R&0x00010000)==0){            // wait for count flag
  }
}

// Delay function based on the system clock
// Inputs: delay in millisecs
void delayMS(unsigned long ms){
  unsigned long i;
  Init_SysTick();
  for(i=0; i<ms; i++){
      Wait_SysTick(80000);                            // wait 1ms
  }
}


// Initalizes PLL for 80Mhz
void Init_PLL(void){
    SYSCTL_RCC2_R |=  0x80000000;                   			// use RCC2
    SYSCTL_RCC2_R |=  0x00000800;                   			// BYPASS2, PLL bypass
    SYSCTL_RCC_R = (SYSCTL_RCC_R &~0x000007C0) + 0x00000540;    // clear XTAL field, bits 10-6, 10101, configure for 16 MHz crystal
    SYSCTL_RCC2_R &= ~0x00000070;                   			// configure for main oscillator source
    SYSCTL_RCC2_R &= ~0x00002000;                   			// activate PLL by clearing PWRDN
    SYSCTL_RCC2_R |= 0x40000000;                    			// use 400 MHz PLL
    SYSCTL_RCC2_R = (SYSCTL_RCC2_R&~ 0x1FC00000) + (4<<22);     // clear system clock divider, configure for 80 MHz clock
    while((SYSCTL_RIS_R&0x00000040)==0){};          			// wait for PLLRIS bit
    SYSCTL_RCC2_R &= ~0x00000800;                   			// enable use of PLL by clearing BYPASS
}

// Port Initialisation
void Init_Port(void) {
	volatile unsigned long delay;
	SYSCTL_RCGC2_R |= 0x0000001F;     					// activate clock for Port A, B, C, D, E
	SYSCTL_RCGC1_R |= 0x00000010;						// activate clock for SSI0
	SYSCTL_RCGC0_R |= 0x00030000;   					// activate clock for ADC0, ADC1

	delay = SYSCTL_RCGC2_R;								// allow time for clock to start

	GPIO_PORTC_LOCK_R = 0x4C4F434B;	 					// unlock GPIO Port C
	GPIO_PORTD_LOCK_R = 0x4C4F434B;                     // unlock GPIO Port D
	GPIO_PORTD_CR_R |= 0xFF;                            // allow registers in Port D to be written to

	GPIO_PORTA_AMSEL_R &= ~0xFC;                        // disable analog mode for PA 7-2
	GPIO_PORTB_AMSEL_R &= ~0xFF;                        // disable analog mode for PB 7-0
	GPIO_PORTC_AMSEL_R &= ~0xF0;                        // disable analog mode for PC 7-4
	GPIO_PORTD_AMSEL_R &= ~0xFF;                        // disable analog mode for PD 7-0
	GPIO_PORTE_AMSEL_R &= ~0x2D;                        // disable analog mode for PE 5,3,2,0
	GPIO_PORTE_AMSEL_R |= 0x12;							// enable analog mode for PE1, PE4

	GPIO_PORTA_AFSEL_R &= 0xC0;                         // disable alternative function for PA 7, 6
	//GPIO_PORTA_AFSEL_R |= 0x3C;       				  	// enable alternative function for PA 5,4,3,2
	GPIO_PORTB_AFSEL_R &= ~0xFF;                        // disable alternative function for PB 7-0
	GPIO_PORTC_AFSEL_R &= ~0xF0;                        // disable alternative function for PC 7-4
	GPIO_PORTD_AFSEL_R &= ~0xFF;                        // disable alternative function for PD 7-0
	GPIO_PORTE_AFSEL_R &= ~0x2D;                        // disable alternative function for PE 5,3,2,0
	GPIO_PORTE_AFSEL_R |= 0x12;							// enable alternative function for PE1, PE4

	GPIO_PORTA_PCTL_R = 0x00000000;						// GPIO functions for PA
	//GPIO_PORTA_PCTL_R = 0x00222200;   				// SSI0 functions for PA 5,4,3,2
	GPIO_PORTB_PCTL_R = 0x00000000;                     // GPIO functions for PB
	GPIO_PORTC_PCTL_R &= ~0xFFFF0000;                   // GPIO functions for PC 7-4
	GPIO_PORTD_PCTL_R = 0x00000000;                     // GPIO functions for PD
	GPIO_PORTE_PCTL_R = 0x00000000;                     // GPIO functions for PE

	GPIO_PORTA_DIR_R &= 0x80;                           // input for PA 7
	GPIO_PORTA_DIR_R |= 0x40;							// output for PA 6
	GPIO_PORTB_DIR_R |= 0xFF;                           // output for PB 7-0
	GPIO_PORTC_DIR_R |= 0xF0;                           // output for PC 7-4
	GPIO_PORTD_DIR_R |= 0xFF;                           // output for PD 7-0
	GPIO_PORTE_DIR_R  = 0x25;							// input for PE 1,3,4; output for PE 5,2,0

	GPIO_PORTA_PUR_R |= 0x80;							// enable pull up resistor for PA 7
	GPIO_PORTE_PUR_R |= 0x08;                           // enable pull up resistor for PE 3

	GPIO_PORTA_DEN_R |= 0xC0;          					// enable digital I/O for PA 7,2
	GPIO_PORTB_DEN_R |= 0xFF;                           // enable digital I/O for PB 7-0
	GPIO_PORTC_DEN_R |= 0xF0;                           // enable digital I/O for PC 7-4
	GPIO_PORTD_DEN_R |= 0xFF;                           // enable digital I/O for PD 7-0
	GPIO_PORTE_DEN_R |= 0x3D;                           // enable digital I/O for PE 5,4,3,2,0
}

// Initalizes SSI0 at 2Mhz
void Init_SSI0(void){
	SSI0_CR1_R = 0x0;					// clear SSE bit
	SSI0_CR1_R = 0x0;					// set SSI0 as master
	SSI0_CC_R = 0x0;					// use system Clock
	SSI0_CPSR_R = 0x08;					// set prescale: 2Mhz = 16Mhz /8 => 8 = PRE*(1+CR) => PRE = 8, CR = 1
	SSI0_CR0_R = 0x00000007;			// SPH=SPO=0; Freescale; 8bit data
	SSI0_CR1_R |= 0x2;					// enable SSI
}

// Initalizes ADC0 and ADC1 using sequencer 3
void Init_Analog(void){
	SYSCTL_RCGC0_R |= 0x00000A00;  	    //  analog sample speed: 500k
	                  	  	  	  	  	//  Value Description:
	                                    //    0x7 1M samples/second
	                                    //    0x5 500K samples/second
	                                    //    0x3 250K samples/second
	                                    //    0x1 125K samples/second

	ADC0_SSPRI_R = 0x0123;				// set sequencer 3 to highest priority
	ADC0_ACTSS_R &= ~0x0008;			// disable sequencer 3 for configuration
	ADC0_EMUX_R &= ~0xF000;				// seq3 continuously sample
	ADC0_SSMUX3_R &= ~0x000F;       	// clear SS3 field
	ADC0_SSMUX3_R += 2;             	// set channel Ain2 (PE1)
	ADC0_SSCTL3_R = 0x0006;         	// no TS0 D0, yes IE0 END0
	ADC0_ACTSS_R |= 0x0008;         	// enable sample sequencer 3


	ADC1_SSPRI_R = 0x0123;				// set sequencer 3 to highest priority
	ADC1_ACTSS_R &= ~0x0008;			// disable sequencer 3 for configuration
	ADC1_EMUX_R &= ~0xF000;				// seq3 continuously sample
	ADC1_SSMUX3_R &= ~0x000F;       	// clear SS3 field
	ADC1_SSMUX3_R += 9;             	// set channel Ain9 (PE4)
	ADC1_SSCTL3_R = 0x0006;         	// no TS0 D0, yes IE0 END0
	ADC1_ACTSS_R |= 0x0008;         	// enable sample sequencer 3


}

// Busy-wait analog to digital conversion
// Outputs: 12-bit result of ADC conversion
unsigned long ADC0(void){
  unsigned long result;
  ADC0_PSSI_R = 0x0008;                // 1) initiate SS3
  while((ADC0_RIS_R&0x08)==0){};       // 2) wait for conversion done
  result = ADC0_SSFIFO3_R&0xFFF;       // 3) read result
  ADC0_ISC_R = 0x0008;                 // 4) acknowledge completion
  return result;
}

// Busy-wait analog to digital conversion
// Outputs: 12-bit result of ADC conversion
unsigned long ADC1(void){
  unsigned long result;
  ADC1_PSSI_R = 0x0008;                // 1) initiate SS3
  while((ADC1_RIS_R&0x08)==0){};       // 2) wait for conversion done
  result = ADC1_SSFIFO3_R&0xFFF;       // 3) read result
  ADC1_ISC_R = 0x0008;                 // 4) acknowledge completion
  return result;
}


// Initialize reading and writing of EEPROM
// Outputs: 0 if no error
//			1 to retry initialisation
//			2 if error has occured
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
        if(status & (EEPROM_EESUPP_PRETRY | EEPROM_EESUPP_ERETRY)){			// check for errors again
            return(INIT_EEPROM_ERROR);
        }
        else{
            return(INIT_EEPROM_RETRY);
        }
    }
    EEPROM_EEBLOCK_R = 0x2;             // select block 1
    EEPROM_EEOFFSET_R = 0x0;            // select offset 0
    EEPROM_EEPROT_R &= ~0x0F;           // allow write and read
    return(INIT_EEPROM_SUCCESS);

}

// Polls the status of EEDONE register to check if EEPROM is performing any operations
void check_eeprom_done(void){
    while(EEPROM_EEDONE_R == 0x1){};
}

// Writes to a word in EEPROM
// Inputs:	data to be written
// 			block and offset selects the location

void write_eeprom(unsigned char block, unsigned char offset, unsigned long *data){
    check_eeprom_done();                // check if EEPROM is idle first
    EEPROM_EEBLOCK_R = block;           // select block
    EEPROM_EEOFFSET_R = offset;         // select offset
    EEPROM_EERDWR_R = *data;            // write highscore to EEPROM
    check_eeprom_done();                // check if EEPROM is done writing

}

// Reads a word from EEPROM
// Inputs: block and offset selects the location of the word to read
// Outputs: a 32bit word
unsigned long read_eeprom(unsigned char block, unsigned char offset){
    check_eeprom_done();                // check if EEPROM is idle first
    EEPROM_EEBLOCK_R = block;           // select block 1
    EEPROM_EEOFFSET_R = offset;         // select offset 0
    check_eeprom_done();                // check if EEPROM is done reading
    return EEPROM_EERDWR_R;             // read highscore from EEPROM
}

// Writes 0 to the selected location
// Inputs: block and offset selects the location
void erase_eeprom(unsigned char block, unsigned char offset){
    check_eeprom_done();                // check if EEPROM is idle first
    EEPROM_EEBLOCK_R = block;          // select block 1
    EEPROM_EEOFFSET_R = offset;         // select offset 0
    EEPROM_EERDWR_R = 0;                // write highscore to EEPROM
    check_eeprom_done();                // check if EEPROM is done writing
}

// Loops while flash memory is performing any operations
void check_flash_done(){
    while(((FLASH_FMC_R & 0x1) | (FLASH_FMC_R & 0x2) | (FLASH_FMC_R & 0x4) | (FLASH_FMC_R & 0x8))){};
}

// Write a single 32-bit word to flash memory
// Inputs: data and address of location
void write_flash(unsigned long *data, unsigned long address){
    FLASH_FMD_R = *data;											// write the data to FMD register
    FLASH_FMA_R = address&0x3FFFF;									// write the target address to FMA register
    FLASH_FMC_R = FLASH_FMC_WRKEY + FLASH_FMC_WRITE;				// write the write key and write bit to the FMC register
    while(FLASH_FMC_R&FLASH_FMC_WRITE == 0x1){};					// wait for completion
}

// Write 32 words with a single buffered flash memory write operation
// Inputs: data and address of location
// offset selects one of the 32 registers in the write buffer
void write_flash_buffer(unsigned long *data,unsigned long address, unsigned char offset){
    volatile unsigned long *reg = ((volatile unsigned long *)0x400FD100);
    reg[offset] = *data;												// offset selects one of the 32 registers in flash write buffer
    FLASH_FMA_R = address&0x3FFFF;                                      // 18bit address
    FLASH_FMC2_R = FLASH_FMC_WRKEY + FLASH_FMC2_WRBUF;                  // write flash memory write key and WRBUF bit
    while(FLASH_FMC2_R&FLASH_FMC2_WRBUF == 0x1){};                      // wait for WRBUF bit to clear
}


// Setup the base address of the DMA controller
// Inputs: base address for DMA control table must be 1024 bytes alligned
void Set_DMA_Base_Address(unsigned long address){
	UDMA_CTLBASE_R = address;
}

// Initialize DMA - select channel 30 for software trigger
void Init_DMA(void){
	SYSCTL_RCGCDMA_R = 0x1;							// enable DMA clock
	UDMA_CFG_R = 0x1;								// enable DMA controller
	Set_DMA_Base_Address(0x20003000);				// set table base address aligned on a 1024 byte boundary
	UDMA_ENASET_R |= 0x40000000;					// set channel 30 to high priority
	UDMA_ALTCLR_R |= 0x40000000;					// set primary control structure
	UDMA_USEBURSTCLR_R |= 0x40000000;				// channel 30 responds to single and burst requests
	UDMA_REQMASKCLR_R |= 0x40000000;				// allow controller to recognize requests from channel 30
}

// Configure the DMA channel
// Inputs:  channel
//			source end address
//			destination end address
//			32 bit control word (see page 609 in mcu datasheet)
void config_DMA_channel(unsigned char channel,unsigned long source, unsigned long destination, unsigned long control_word){
	volatile unsigned long *base = ((volatile unsigned long *)UDMA_CTLBASE_R);							// get address of control table base
	unsigned short s_pointer = channel*16;				// calculate offset for source end pointer
	unsigned short d_pointer = channel*16 + 4;			// calculate offset for destination end pointer
	unsigned short c_word = channel*16 + 8;				// calculate offset for control word
	base[s_pointer/4] = source;							// set source end pointer
	base[d_pointer/4] = destination;					// set destination end pointer
	base[c_word/4] = control_word;						// set control word
}

// Begin DMA transfer on channel 30
void start_DMA_transfer(void){
	UDMA_ENASET_R |= 0x40000000;					// enable channel 30
	UDMA_SWREQ_R |= 0x40000000;						// issue transfer request for channel 30
}
