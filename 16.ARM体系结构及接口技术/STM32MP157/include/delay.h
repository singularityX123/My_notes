/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __DELAY_H__
#define __DELAY_H__

/* 不依赖外部头文件 — unsigned int 为内建类型 */

/**
 * delay_us() - 微秒级阻塞延时
 * @us: 延时微秒数
 *
 * 此函数由具体平台提供强符号实现覆盖。
 * 若平台未提供，则使用 delay.c 中的 __weak 默认实现（DWT）。
 */
void delay_us(unsigned int us);

/**
 * delay_ms() - 毫秒级阻塞延时
 * @ms: 延时毫秒数
 *
 * 默认基于 delay_us() 实现，平台也可单独覆盖。
 */
void delay_ms(unsigned int ms);

/**
 * stm32mp157_delay_init() - 初始化延时定时器（TIM2）
 *
 * 使能 TIM2 时钟，配置预分频器使计数器工作在 1 MHz（1 tick = 1 μs）。
 * 在使用 delay_us / delay_ms 之前调用一次。
 */
void stm32mp157_delay_init(void);

#endif /* __DELAY_H__ */
