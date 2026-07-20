// src/pwm_beep.c - PWM 蜂鸣器驱动（纯寄存器操作）
#include "../../include/stm32mp157_gpio.h"
#include "../../include/stm32mp157_rcc.h"
#include "../../include/stm32mp157_tim.h"
#include "music_engine.h"

void beep_pwm_init(void) {
    /* 1. 使能 GPIOB、TIM4 控制器时钟 */
    RCC->MP_AHB4ENSETR |= (1 << 1);   // GPIOB 时钟
    RCC->MP_APB1ENSETR |= (1 << 2);   // TIM4 时钟

    /* 2. 设置 PB6 为 TIM4_CH1 复用功能 */
    GPIOB->MODER &= ~(0x3 << 12);
    GPIOB->MODER |=  (0x2 << 12);     // AF 模式
    GPIOB->AFRL  &= ~(0xF << 24);
    GPIOB->AFRL  |=  (0x2 << 24);     // AF2 → TIM4_CH1

    /* 3. 设置分频系数: 209MHz / 209 = 1MHz (1us/tick) */
    TIM4->PSC = 209 - 1;

    /* 4. 设置 PWM 周期（默认值，音符播放时会覆盖） */
    TIM4->ARR = 1000;

    /* 5. 设置占空比（默认值，音符播放时会覆盖） */
    TIM4->CCR1 = 800;

    /* 6. 设置 TIM4_CH1 为 PWM 模式 1 */
    TIM4->CCMR1 &= ~(0x1 << 16 | 0x7 << 4);
    TIM4->CCMR1 |=  (0x6 << 4);       // OC1M = 110 (PWM mode 1)

    /* 7. CC1 配置为输出模式 */
    TIM4->CCMR1 &= ~(0x3 << 0);       // CC1S = 00 (output)

    /* 8. 配置 ACTIVE 状态为高电平 */
    TIM4->CCER &= ~(0x1 << 1);        // CC1P = 0 (active high)

    /* 9. 输出使能 */
    TIM4->CCER |= (0x1 << 0);         // CC1E = 1

    /* 10. 向上计数模式 */
    TIM4->CR1 &= ~(0x3 << 5);         // CMS = 00 (edge-aligned)
    TIM4->CR1 &= ~(0x1 << 4);         // DIR = 0 (up counter)
}

void beep_pwm_on(void) {
    /* 使能计数器 */
    TIM4->CR1 |= (0x1 << 0);          // CEN = 1
}

void beep_pwm_off(void) {
    TIM4->CR1  &= ~(0x1 << 0);        // CEN = 0，停计数器
    TIM4->CCER &= ~(0x1 << 0);        // CC1E = 0，关输出（防引脚卡高）
}
