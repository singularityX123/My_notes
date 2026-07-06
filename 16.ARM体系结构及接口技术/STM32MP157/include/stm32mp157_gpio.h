// include/stm32mp157_gpio.h
#ifndef __STM32MP157_GPIO__
#define __STM32MP157_GPIO__

typedef struct
{
    volatile unsigned int MODER;   // 0x00
    volatile unsigned int OTYPER;  // 0x04
    volatile unsigned int OSPEEDR; // 0x08
    volatile unsigned int PUPDR;   // 0x0C
    volatile unsigned int IDR;     // 0x10
    volatile unsigned int ODR;     // 0x14
    volatile unsigned int BSRR;    // 0x18
    volatile unsigned int LCKR;    // 0x1C
    volatile unsigned int AFRL;    // 0x20
    volatile unsigned int AFRH;    // 0x24
    volatile unsigned int BRR;     // 0x28
    volatile unsigned int res;     // 0x2C
    volatile unsigned int SECCFGR; // 0x30
} gpio_t;

#define GPIOA ((gpio_t *)0x50002000)
#define GPIOB ((gpio_t *)0x50003000)
#define GPIOC ((gpio_t *)0x50004000)
#define GPIOD ((gpio_t *)0x50005000)
#define GPIOE ((gpio_t *)0x50006000)
#define GPIOF ((gpio_t *)0x50007000)
#define GPIOG ((gpio_t *)0x50008000)
#define GPIOH ((gpio_t *)0x50009000)
#define GPIOI ((gpio_t *)0x5000A000)
#define GPIOJ ((gpio_t *)0x5000B000)
#define GPIOK ((gpio_t *)0x5000C000)
#define GPIOZ ((gpio_t *)0x54004000)

#endif
