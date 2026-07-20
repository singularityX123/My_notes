//#include <stdio.h>  裸机，无法使用标准库函数
#include <stdint.h>
#include "../../include/stm32mp157_rcc.h"
#include "../../include/stm32mp157_gpio.h"
#include "../../include/stm32mp157_iwdg.h"

/*c中嵌汇编启动*/
/* 栈空间：4KB，编译器自动放入 .bss */
__attribute__((aligned(8))) static uint8_t _stack[4096];

/* 真正入口：naked = 不生成函数序言，100% 自己控制 */
__attribute__((naked))
void _start(void)
{
    __asm__ volatile(
        "ldr sp, =_stack + 4096\n"   /* SP 指向栈顶（高地址） */
        "bl  main\n"                  /* 调用 C 代码 */
        "b   .\n"                     /* main 返回则死循环 */
    );
}


int main(void)
{
    /*GPIO E10 管脚初始化*/
    /*1.使能 GPIOE 控制器时钟*/
    RCC->MP_AHB4ENSETR |= (1 << 4);

    /*2.设置 GPIOE10 为输出模式*/
    GPIOE->MODER &= ~(0x3 << (10 * 2)); // 清除 GPIOE10 模式位
    GPIOE->MODER |= (0x1 << (10 * 2));  // 设置 GPIOE10 为输出模式

    /*3.设置 GPIOE10 输出类型为推挽输出*/
    GPIOE->OTYPER &= ~(0x1 << 10);

    /*4.设置 GPIOE10 输出速度为高速*/
    GPIOE->OSPEEDR &= ~(0x3 << (10 * 2));
    GPIOE->OSPEEDR |= (0x2 << (10 * 2));

    /*5.设置禁止上下拉*/
    GPIOE->PUPDR &= ~(0x3 << (10 * 2));

    /*6.配置看门狗*/
    IWDG1->KR  = IWDG_KEY_UNLOCK;  // 解锁 PR/RLR 写访问
    IWDG1->PR  = 0x01;             // 设置预分频器
    IWDG1->RLR = 0xFFF0;           // 设置重装载寄存器
    IWDG1->KR  = IWDG_KEY_ENABLE;  // 启动看门狗

    /*循环LED*/
    while(1)
    {   
        GPIOE->ODR ^= (1 << 10); // 切换 GPIOE10 的状态
        for(volatile int i = 0; i < 2000000; i++); // 延时

        /*喂狗*/
        IWDG1->KR = IWDG_KEY_REFRESH; // 喂狗，重置看门狗计数器
    }


    return 0; // main 返回，虽然在裸机中通常不会执行到这里
}