#include "../include/stm32mp157_gpio.h"
#include "../include/stm32mp157_rcc.h"
#include "../include/stm32mp157_tim.h"

// ===================== 风扇 ======================

void FAN1_pwm_init(void) {
    // 1. 使能 GPIO 和 TIM1 时钟
    RCC->MP_AHB4ENSETR |= (0x1 << 4);
    RCC->MP_APB2ENSETR |= (0x1 << 0);

    // 2. 配置 PB2 复用功能
    GPIOE->MODER &= (~(0x3 << 18));
    GPIOE->MODER |= (0x2 << 18);
    GPIOE->AFRH &= (~(0xF << 4));
    GPIOE->AFRH |= (0x1 << 4);

    // 3. 设置预分频寄存器，TIM1_PSC[15:0] = 208
    // 分频前时钟：CK_PSC = 209MHz, 提供给TIM1的时钟源的频率是209MHz
    // 分频后的时钟 CK_CNT = 209 000 000Hz / (208 + 1) = 1 000 000Hz
    TIM1->PSC = 209 - 1;

    // 4. 设置PWM方波的最终的周期  TIM1_ARR[16:0] = 1000
    //      得到一个1000-2000Hz的方波
    // PWM方波的频率 = CK_PSC / PSC / ARR
    TIM1->ARR = 1000;

    // 5. 设置PWM方波的占空比   TIM1_CCR1[16:0] = 700
    TIM1->CCR1 = 700;

    // 6. 设置TIM1_CH1通道为PWM1模式
    //      TIM1_CCMR1[16] = 0b0  TIM1_CCMR1[6:4] = 0b110
    //      pwm模式1  = 0b0110
    TIM1->CCMR1 &= (~(0x1 << 16 | 0x7 << 4));
    TIM1->CCMR1 |= (0x6 << 4);

    // 7. 使能TIM1_CH1的OC1为输出
    TIM1->CCMR1 &= (~(0x3 << 0));
    
    // 8. 设置TIM1_CH1通道输出PWM方波的极性，
    //    TIM1_CCER[1] = 0x1 or 0x0
#if 0
    TIM1->CCER |= (0x1 << 1);   // CCR1值越小，占空比越大
#else
    TIM1->CCER &= (~(0x1 << 1)); // CCR1值越大，占空比越大
#endif

    // 9. 设置TIM1_CH1通道的输出使能位，通过GPIO引脚输出PWM方波
    //      TIM1_CCER[0] = 0x1
    TIM1->CCER |= (0x1 << 0);

    // 10. 设置定时器的计数方式，边沿对齐,向上计数
    //      TIM1_CR1[6:5] = 0x0
    TIM1->CR1 &= (~(0x3 << 5));
    //      TIM1_CR1[4] = 0x0
    TIM1->CR1 &= (~(0x1 << 4));
}

void FAN1_pwm_on(void){
    FAN1_pwm_init();
    // 11. 使能TIM1_CH1计数器
    //      TIM1_CR1[0] = 0x1
    TIM1->CR1 |= (0x1 << 0);
    // 12. 设置主输出使能
    TIM1->BDTR |= (0x1 << 15);
}
void FAN1_pwm_off(void){
    FAN1_pwm_init();
    /* 停止计数*/
    TIM1->CR1 &= ~(0x1 << 0);
    TIM1->BDTR &= ~(1<<15);
}

// ===================== 马达 ======================

// void MOTOR_pwm_init(void) {
//     // 1. 使能 GPIO 和 TIM1 时钟
//     RCC->MP_AHB4ENSETR |= (0x1 << 4);
//     RCC->MP_APB2ENSETR |= (0x1 << 0);

//     // 2. 配置 PB2 复用功能
//     GPIOE->MODER &= (~(0x3 << 18));
//     GPIOE->MODER |= (0x2 << 18);
//     GPIOE->AFRH &= (~(0xF << 4));
//     GPIOE->AFRH |= (0x1 << 4);

//     // 3. 设置预分频寄存器，TIM1_PSC[15:0] = 208
//     // 分频前时钟：CK_PSC = 209MHz, 提供给TIM1的时钟源的频率是209MHz
//     // 分频后的时钟 CK_CNT = 209 000 000Hz / (208 + 1) = 1 000 000Hz
//     TIM1->PSC = 209 - 1;

//     // 4. 设置PWM方波的最终的周期  TIM1_ARR[16:0] = 1000
//     //      得到一个1000-2000Hz的方波
//     // PWM方波的频率 = CK_PSC / PSC / ARR
//     TIM1->ARR = 1000;

//     // 5. 设置PWM方波的占空比   TIM1_CCR1[16:0] = 700
//     TIM1->CCR1 = 700;

//     // 6. 设置TIM1_CH1通道为PWM1模式
//     //      TIM1_CCMR1[16] = 0b0  TIM1_CCMR1[6:4] = 0b110
//     //      pwm模式1  = 0b0110
//     TIM1->CCMR1 &= (~(0x1 << 16 | 0x7 << 4));
//     TIM1->CCMR1 |= (0x6 << 4);

//     // 7. 使能TIM1_CH1的OC1为输出
//     TIM1->CCMR1 &= (~(0x3 << 0));
    
//     // 8. 设置TIM1_CH1通道输出PWM方波的极性，
//     //    TIM1_CCER[1] = 0x1 or 0x0
// #if 0
//     TIM1->CCER |= (0x1 << 1);   // CCR1值越小，占空比越大
// #else
//     TIM1->CCER &= (~(0x1 << 1)); // CCR1值越大，占空比越大
// #endif

//     // 9. 设置TIM1_CH1通道的输出使能位，通过GPIO引脚输出PWM方波
//     //      TIM1_CCER[0] = 0x1
//     TIM1->CCER |= (0x1 << 0);

//     // 10. 设置定时器的计数方式，边沿对齐,向上计数
//     //      TIM1_CR1[6:5] = 0x0
//     TIM1->CR1 &= (~(0x3 << 5));
//     //      TIM1_CR1[4] = 0x0
//     TIM1->CR1 &= (~(0x1 << 4));
// }

// void MOTOR_pwm_on(void){
//     MOTOR_pwm_init();
//     // 11. 使能TIM1_CH1计数器
//     //      TIM1_CR1[0] = 0x1
//     TIM1->CR1 |= (0x1 << 0);
//     // 12. 设置主输出使能
//     TIM1->BDTR |= (0x1 << 15);
// }
// void MOTOR_pwm_off(void){
//     MOTOR_pwm_init();
//     /* 停止计数*/
//     TIM1->CR1 &= ~(0x1 << 0);
//     TIM1->BDTR &= ~(1<<15);
// }