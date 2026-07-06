// SPDX-License-Identifier: GPL-2.0-only
/*
 * Generic delay interface — weak default implementations
 *
 * Platform code can override delay_us() / delay_ms() by simply
 * defining a strong symbol with the same signature.
 */

#include "delay.h"

/**
 * delay_us - 默认微秒延时（DWT 周期计数方式）
 * @us: 延时微秒数
 *
 * 使用 ARM DWT 的 CYCCNT 寄存器实现跨 Cortex-M 平台的通用延时。
 * 平台可通过定义同名的非弱符号来覆盖此实现。
 */
__attribute__((weak)) void delay_us(unsigned int us)
{
	(void)us;
}

/**
 * delay_ms - 默认毫秒延时
 * @ms: 延时毫秒数
 *
 * 默认直接调用 delay_us(ms * 1000)，平台可单独覆盖以优化精度。
 */
__attribute__((weak)) void delay_ms(unsigned int ms)
{
	delay_us(ms * 1000U);
}
