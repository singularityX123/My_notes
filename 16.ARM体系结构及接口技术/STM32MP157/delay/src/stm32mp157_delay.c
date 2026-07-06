// delay/src/stm32mp157_delay.c
#include "../../include/delay.h"
#include "../../include/stm32mp157_tim.h"
#include "../../include/stm32mp157_rcc.h"

/*
 * TIM2 位于 APB1 总线
 * APB1 = MCU_CLK = 208.878 MHz（APB1DIV=÷1, TIMG1PRER=禁用）
 * PSC = 208 → CNT_CLK = 209MHz / 209 = 1.00 MHz (1 tick = 1 us)
 */
#define TIM2_PSC	208U

/**
 * stm32mp157_delay_init() - 初始化 TIM2 作为微秒计时器
 *
 * 使能 TIM2 时钟，配置预分频器，启动计数。
 * 在 main 入口处调用一次。
 */
void stm32mp157_delay_init(void)
{
	/* 使能 TIM2 时钟（MPU 域 APB1，bit 0 — A7 核用 MP_，不是 MC_） */
	RCC->MP_APB1ENSETR |= (1U << 0);

	/* 配置预分频器: 计数器时钟 = 1 MHz */
	TIM2->PSC = TIM2_PSC;

	/* 生成更新事件，锁存 PSC 到活跃寄存器 */
	TIM2->EGR |= (1U << 0);

	/* 使能 TIM2 */
	TIM2->CR1 |= (1U << 0);
}

/**
 * delay_us - STM32MP157 微秒延时（强符号，覆盖 weak 版本）
 * @us: 延时微秒数
 */
void delay_us(unsigned int us)
{	//  外设版
	TIM2->CNT = 0;
	while (TIM2->CNT < us)
		;

	/* 不用初始化外设直接使用 */
	// while(us--){
    //     unsigned int i = 2;
    //     while (i--);
    // }
}
/*
 * delay_ms 无需重写，链接器自动选用 delay.c 中的 __weak 版本，
 * 其内部调用 delay_us(ms * 1000)
 */