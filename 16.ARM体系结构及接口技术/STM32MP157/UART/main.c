/* 阻塞IO,忙等待喂狗（无法处理多任务请求）*/
#include "stm32mp157_uart.h"
#include "stm32mp157_iwdg.h"
#include <stdint.h>


/* 栈空间：4KB，编译器自动放入 .bss */
__attribute__((aligned(8))) static uint8_t _stack[4096];

/* 入口：naked = 不生成函数序言，100% 自己控制 */
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
    uart_init();

    // 测试发送字符串到TX引脚，minicom监听此uart串口设备
    uart_put_str("Please enter a string: ");
    uart_put_str("\n");

    // 测试从RX引脚接收字符串，minicom发送字符串到此uart串口设备
    char buf[1024];
    char buf2[1024];
    int len = uart_get_str(buf, sizeof(buf));

    uart_put_str("Received: ");
    uart_put_str(buf);
    uart_put_str("\n");

    uart_put_str("The length of the received string is: ");

    // 将整数 len 转换为字符串并发送
    int temp = len;
    int digits = 0;
    do {
        temp /= 10;
        digits++;
    } while (temp > 0); // 计算数字的位数
    temp = len;
    for (int i = digits - 1; i >= 0; i--) {
        buf2[i] = (temp % 10) + '0'; // 将每位数字转换为字符
        temp /= 10;
    }
    buf2[digits] = '\0'; // 字符串结尾
    uart_put_str(buf2);
    uart_put_str("\n");


    while(1)
    {   
        /*喂狗*/
        IWDG1->KR = IWDG_KEY_REFRESH; // 喂狗，重置看门狗计数器
    }

    return 0;
}
