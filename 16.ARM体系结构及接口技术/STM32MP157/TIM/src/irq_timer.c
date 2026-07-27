#include "../../include/stm32mp157_rcc.h"
#include "../../include/stm32mp157_tim.h"
#include "../../include/stm32mp157_gic.h"
#include "../../include/stm32mp157_uart.h"

/*初始化timer3中断*/
void timer3_irq_init(void)
{
    // timer3时钟使能
    RCC->MP_APB1ENSETR |= (1<<1); // bit1: TIM3EN

    // 分频获取驱动计数器的时钟信号 10000HZ
    TIM3->PSC = 20900 - 1;

    // 计数上限，一秒一溢出，一秒一中断
    TIM3->ARR = 10000 - 1;

    // 设置向上计数
    TIM3->CR1 &= (~(0x3 << 5));
    TIM3->CR1 &= (~(0x1 << 4));

    // 中断使能
    TIM3->DIER |= (1<<0); // bit0: UIE=1

    // GICD
    GICD->ISENABLER[1] |= (1<<29);
    GICD->IPRIORITYR[15] &= ~(0x0f<<12);
    GICD->IPRIORITYR[15] |= (0x06<<12);
    GICD->ITARGETSR[15] &= ~(3<<8);
    GICD->ITARGETSR[15] |= (1<<8);

    // 计数使能
    TIM3->CR1 |= (0x1 << 0);
}

void timer3_irq_handler(void)
{
    // 处理定时器中断
    uart_put_str("timer3_irq_handler, 1s delay done.\n");

    // 清除中断源中的pending
    TIM3->SR &= ~(1<<0);

    // 通知GIC中断处理完毕（EOIR: End of Interrupt Register）
    // TIM3 IRQ=29, SPI中断号 = 32 + 29 = 61
    GICC->EOIR = 61;
}