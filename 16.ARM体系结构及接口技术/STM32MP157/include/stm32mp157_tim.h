// include/stm32mp157_tim.h
#ifndef __STM32MP157_TIM_H__
#define __STM32MP157_TIM_H__

typedef struct{
    volatile unsigned int CR1;
    volatile unsigned int CR2;
    volatile unsigned int SMCR;
    volatile unsigned int DIER;
    volatile unsigned int SR;
    volatile unsigned int EGR;
    volatile unsigned int CCMR1;
    volatile unsigned int CCMR2;
    volatile unsigned int CCER;
    volatile unsigned int CNT;
    volatile unsigned int PSC;
    volatile unsigned int ARR;
    volatile unsigned int RCR;
    volatile unsigned int CCR1;
    volatile unsigned int CCR2;
    volatile unsigned int CCR3;
    volatile unsigned int CCR4;
    volatile unsigned int BDTR;
    volatile unsigned int DCR;
    volatile unsigned int DMAR;
    volatile unsigned int RES1;
    volatile unsigned int CCMR3;
    volatile unsigned int CCR5;
    volatile unsigned int CCR6;
    volatile unsigned int AF1;
    volatile unsigned int AF2;
    volatile unsigned int TISEL;
} tim1_t;

#define TIM1  ((tim1_t *)0x44000000)

typedef struct {
    volatile unsigned int CR1;
    volatile unsigned int CR2;
    volatile unsigned int SMCR;
    volatile unsigned int DIER;
    volatile unsigned int SR;
    volatile unsigned int EGR;
    volatile unsigned int CCMR1;
    volatile unsigned int CCMR2;
    volatile unsigned int CCER;
    volatile unsigned int CNT;
    volatile unsigned int PSC;
    volatile unsigned int ARR;
    volatile unsigned int RES1[1];
    volatile unsigned int CCR1;
    volatile unsigned int CCR2;
    volatile unsigned int CCR3;
    volatile unsigned int CCR4;
    volatile unsigned int RES2[1];
    volatile unsigned int DCR;
    volatile unsigned int DMAR;
    volatile unsigned int RES3[4];
    volatile unsigned int AF1;
    volatile unsigned int RES4[1];
    volatile unsigned int TISEL;
}tim2_3_4_5_t;

#define  TIM2   ((tim2_3_4_5_t *)0x40000000)
#define  TIM3   ((tim2_3_4_5_t *)0x40001000)
#define  TIM4   ((tim2_3_4_5_t *)0x40002000)
#define  TIM5   ((tim2_3_4_5_t *)0x40003000)

typedef struct{
    volatile unsigned int CR1;
    volatile unsigned int CR2;
    volatile unsigned int RES1;
    volatile unsigned int DIER;
    volatile unsigned int SR;
    volatile unsigned int EGR;
    volatile unsigned int CCMR1;
    volatile unsigned int RES2;
    volatile unsigned int CCER;
    volatile unsigned int CNT;
    volatile unsigned int PSC;
    volatile unsigned int ARR;
    volatile unsigned int RCR;
    volatile unsigned int CCR1;
    volatile unsigned int RES3[3];
    volatile unsigned int BDTR;
    volatile unsigned int DCR;
    volatile unsigned int DMAR;
    volatile unsigned int RES4[4];
    volatile unsigned int AF1;
    volatile unsigned int RES5;
    volatile unsigned int TISEL;
}tim16_17_t;

#define TIM16  ((volatile tim16_17_t *)0x44007000)
#define TIM17  ((volatile tim16_17_t *)0x44008000)

typedef struct {
    volatile unsigned int CR1;
    volatile unsigned int CR2;
    volatile unsigned int SMCR;
    volatile unsigned int DIER;
    volatile unsigned int SR;
    volatile unsigned int EGR;
    volatile unsigned int CCMR1;
    volatile unsigned int RES1;
    volatile unsigned int CCER;
    volatile unsigned int CNT;
    volatile unsigned int PSC;
    volatile unsigned int ARR;
    volatile unsigned int RCR;
    volatile unsigned int CCR1;
    volatile unsigned int CCR2;
    volatile unsigned int RES3[2];
    volatile unsigned int BDTR;
    volatile unsigned int DCR;
    volatile unsigned int DMAR;
    volatile unsigned int RES4[4];
    volatile unsigned int AF1;
    volatile unsigned int RES5;
    volatile unsigned int TISEL;
}tim15_t;

#define TIM15  ((volatile tim15_t *)0x44006000)

#endif