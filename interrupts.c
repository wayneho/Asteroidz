#include "tm4c123gh6pm.h"

void Init_Interrupt(void){
    GPIO_PORTE_IS_R  &= ~0x08;                                                      //  PE3 is edge-sensitive
    GPIO_PORTE_IBE_R &= ~0x08;                                                      //  PE3 is not both edges
    GPIO_PORTE_IEV_R &= ~0x08;                                                      //  PE3 falling edge event
    GPIO_PORTE_ICR_R  = 0x08;                                                       //  clear flag PE3
    GPIO_PORTE_IM_R  |= 0x08;                                                       //  enable interrupt on PE3
    NVIC_PRI1_R = (NVIC_PRI1_R&0xFFFFFF0F)|0x000000A0;                              //  priority 5


    GPIO_PORTA_IS_R  &= ~0x80;                                                      //  PA7 is edge-sensitive
    GPIO_PORTA_IBE_R &= ~0x80;                                                      //  PA7 is not both edges
    GPIO_PORTA_IEV_R &= ~0x80;                                                      //  PA7 falling edge event
    GPIO_PORTA_ICR_R  = 0x80;                                                       //  clear flag PA7
    GPIO_PORTA_IM_R  |= 0x80;                                                       //  enable interrupt on PE3
    NVIC_PRI0_R = (NVIC_PRI0_R&0xFFFFFF0F)|0x00000080;                              //  priority 4

    NVIC_EN0_R = 0x00000011;                                                        // enable interrupt vector for Port A, E

}

void Init_Timer0A(unsigned long period){
    unsigned long volatile delay;
    SYSCTL_RCGCTIMER_R |= 0x01;         // activate clock for timer0A
    delay = SYSCTL_RCGCTIMER_R;         // give time for clock to start

    TIMER0_CTL_R = 0x00000000;          // disable timer0A
    TIMER0_CFG_R = 0x00000000;          // 32-bit mode
    TIMER0_TAMR_R = 0x00000002;         // periodic mode
    TIMER0_TAILR_R = period-1;          // reload value (80Mhz clock)
    TIMER0_TAPR_R = 0;                  // prescale 0
    TIMER0_ICR_R = 0x00000001;          // clear timeout flag
    TIMER0_IMR_R = 0x00000001;          // enable interrupt for timerA
    NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x80000000;      // priority 4
    NVIC_EN0_R = 1<<19;                 // enable IRQ 19
    //TIMER0_CTL_R = 0x00000001;          // enable timer0A
}

void Timer0A_Stop(void){
  TIMER0_CTL_R &= ~0x00000001; // disable
}

void Timer0A_Start(void){
  TIMER0_CTL_R |= 0x00000001;   // enable
}



void Init_Timer1A(unsigned long period){
    unsigned long volatile delay;
    SYSCTL_RCGCTIMER_R |= 0x02;         // activate clock for timer1A
    delay = SYSCTL_RCGCTIMER_R;         // give time for clock to start

    TIMER1_CTL_R = 0x00000000;          // disable timer1A
    TIMER1_CFG_R = 0x00000000;          // 32-bit mode
    TIMER1_TAMR_R = 0x00000002;         // periodic mode
    TIMER1_TAILR_R = period-1;          // reload value (80Mhz clock)
    TIMER1_TAPR_R = 0;                  // prescale 0
    TIMER1_ICR_R = 0x00000001;          // clear timeout flag
    TIMER1_IMR_R = 0x00000001;          // enable interrupt for timerA
    NVIC_PRI5_R = (NVIC_PRI5_R&0xFFFF00FF)|0x00006000;      // priority 3
    NVIC_EN0_R = 1<<21;                 // enable IRQ 21
    //TIMER1_CTL_R = 0x00000001;          // enable timer1A

}

void Timer1A_Stop(void){
  TIMER1_CTL_R &= ~0x00000001; // disable
}

void Timer1A_Start(void){
  TIMER1_CTL_R |= 0x00000001;   // enable
}


