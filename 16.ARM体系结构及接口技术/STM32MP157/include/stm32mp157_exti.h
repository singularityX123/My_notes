/* include/stm32mp157_exti.h */
#ifndef __STM32MP1XX_EXTI_H__
#define __STM32MP1XX_EXTI_H__

typedef struct{
    volatile unsigned int RTSR1;    // EXTI rising trigger selection register   
    volatile unsigned int FTSR1;    // EXTI falling trigger selection register  
    volatile unsigned int SWIER1;   // EXTI software interrupt event register 
    volatile unsigned int RPR1;     // EXTI rising edge pending register
    volatile unsigned int FPR1;     // EXTI falling edge pending register
    volatile unsigned int TZENR1;   // EXTI TrustZone enable register
    volatile unsigned int RES1[2];  
    volatile unsigned int RTSR2;    // EXTI rising trigger selection register
    volatile unsigned int FTSR2;    // EXTI falling trigger selection register
    volatile unsigned int SWIER2;   // EXTI software interrupt event register
    volatile unsigned int RPR2;     // EXTI rising edge pending register
    volatile unsigned int FPR2;     // EXTI falling edge pending register
    volatile unsigned int TZENR2;   // EXTI TrustZone enable register
    volatile unsigned int RES2[2];
    volatile unsigned int RTSR3;    // EXTI rising trigger selection register
    volatile unsigned int FTSR3;    // EXTI falling trigger selection register
    volatile unsigned int SWIER3;   // EXTI software interrupt event register
    volatile unsigned int RPR3;     // EXTI rising edge pending register
    volatile unsigned int FPR3;     // EXTI falling edge pending register
    volatile unsigned int TZENR3;   // EXTI TrustZone enable register
    volatile unsigned int RES3[2];
    volatile unsigned int EXTICR1;  // EXTI external interrupt selection register 1
    volatile unsigned int EXTICR2;  // EXTI external interrupt selection register 2 
    volatile unsigned int EXTICR3;  // EXTI external interrupt selection register 3 
    volatile unsigned int EXTICR4;  // EXTI external interrupt selection register 4 
    volatile unsigned int RES4[4];
    volatile unsigned int IMR1;   // EXTI CPU1 wakeup with interrupt mask register
    volatile unsigned int EMR1;   // EXTI CPU1 wakeup with event mask register
    volatile unsigned int RES5[2];
    volatile unsigned int C1IMR2;   // EXTI CPU1 wakeup with interrupt mask register
    volatile unsigned int C1EMR2;   // EXTI CPU1 wakeup with event mask register
    volatile unsigned int RES6[2];
    volatile unsigned int C1IMR3;   // EXTI CPU1 wakeup with interrupt mask register    
    volatile unsigned int C1EMR3;   // EXTI CPU1 wakeup with event mask register 
    volatile unsigned int RES7[6];
    volatile unsigned int C2IMR1;   // EXTI CPU2 wakeup with interrupt mask register
    volatile unsigned int C2EMR1;   // EXTI CPU2 wakeup with event mask register
    volatile unsigned int RES8[2];
    volatile unsigned int C2IMR2;   // EXTI CPU2 wakeup with interrupt mask register
    volatile unsigned int C2EMR2;   // EXTI CPU2 wakeup with event mask register
    volatile unsigned int RES9[2];
    volatile unsigned int C2IMR3;   // EXTI CPU2 wakeup with interrupt mask register 
    volatile unsigned int C2EMR3;   // EXTI CPU2 wakeup with event mask register 
    volatile unsigned int RES10[2];
}exti_t;

#define  EXTI   ((exti_t*)0x5000D000)

#endif