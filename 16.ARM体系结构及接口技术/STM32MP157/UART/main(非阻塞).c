#include <stdint.h>
#include "stm32mp157_rcc.h"
#include "stm32mp157_gpio.h"
#include "stm32mp157_iwdg.h"

/* ---- USART 本地定义（非阻塞版自包含，不依赖 uart.h）---- */
typedef struct {
    volatile unsigned int CR1;
    volatile unsigned int CR2;
    volatile unsigned int CR3;
    volatile unsigned int BRR;
    volatile unsigned int GTPR;
    volatile unsigned int RTOR;
    volatile unsigned int RQR;
    volatile unsigned int ISR;
    volatile unsigned int ICR;
    volatile unsigned int RDR;
    volatile unsigned int TDR;
    volatile unsigned int PRESC;
} uart_t;
#define USART4  ((uart_t *)0x40010000)

/* ---- UART 时钟参数 ---- */
#define USART_KER_CKPRES  64000000
#define TX_OR_RX_BAUD      115200

/* ============================================================
   栈空间：4KB，编译器自动放入 .bss
   ============================================================ */
__attribute__((aligned(8))) static uint8_t _stack[4096];

/* ============================================================
   入口：裸机 _start
   ============================================================ */
__attribute__((naked))
void _start(void)
{
    __asm__ volatile(
        "ldr sp, =_stack + 4096\n"
        "bl  main\n"
        "b   .\n"
    );
}

/* ============================================================
   UART 初始化（115200, 8N1, PB2=TX, PG6=RX）
   ============================================================ */
static void uart_init(void)
{
    /* 1. 使能时钟 */
    RCC->MP_APB1ENSETR |= (1 << 16);            // UART4
    RCC->MP_AHB4ENSETR |= (1 << 6) | (1 << 1);  // GPIOG, GPIOB

    /* 2. 引脚复用 */
    GPIOB->MODER &= ~(3 << 4);  GPIOB->MODER |= (2 << 4);   // PB2 → AF
    GPIOG->MODER &= ~(3 << 22); GPIOG->MODER |= (2 << 22);  // PG6 → AF
    GPIOB->AFRL  &= ~(0xF << 8);  GPIOB->AFRL  |= (8 << 8);  // PB2 → AF8
    GPIOG->AFRH  &= ~(0xF << 12); GPIOG->AFRH  |= (6 << 12); // PG6 → AF6

    /* 3. UART 控制器配置: 115200 8N1 */
    USART4->CR1 &= ~(1 << 0);                          // 先关闭
    USART4->CR1 &= ~((1 << 29) | (1 << 28) | (1 << 12) | (1 << 15) | (3 << 9));
    USART4->CR2 &= ~((1 << 19) | (3 << 12));           // LSB, 1停止位
    USART4->PRESC &= ~(0xF << 0);                      // 1分频
    USART4->BRR = USART_KER_CKPRES / TX_OR_RX_BAUD;    // 波特率
    USART4->CR1 |= (1 << 2) | (1 << 3) | (1 << 0);     // 使能 RX, TX, UART
}

/* ============================================================
   发送一个字符（\r 和 \n 统一输出 \r\n）
   ============================================================ */
static void uart_put_char(char c)
{
    if (c == '\n' || c == '\r') {
        while (!(USART4->ISR & (1 << 7)));
        USART4->TDR = '\r';
        while (!(USART4->ISR & (1 << 7)));
        USART4->TDR = '\n';
        return;
    }
    while (!(USART4->ISR & (1 << 7)));
    USART4->TDR = c;
}

/* ============================================================
   发送字符串
   ============================================================ */
static void uart_put_str(const char *s)
{
    while (*s) uart_put_char(*s++);
}

/* ============================================================
   非阻塞接收字符：有数据返回字符(>=0)，无数据返回 -1
   ============================================================ */
static int get_char_nb(void)
{
    if (USART4->ISR & (1 << 5))          // RXNE=1
        return (int)(char)USART4->RDR;
    return -1;
}

/* ============================================================
   主程序 —— 非阻塞事件循环
   ============================================================ */

    // TODO：(bug)算字符串长度有时错误(?）、算字符串长度比mian版少1

int main(void)
{
    uart_init();
    uart_put_str("=== Non-blocking UART Demo ===\r\n");
    uart_put_str("Please enter a string: \r\n");

    char buf[1024];
    char buf2[1024];
    int  buf_idx  = 0;

    while (1) {
        IWDG1->KR = IWDG_KEY_REFRESH; // 喂狗

        int c = get_char_nb();      // 非阻塞！
        if (c < 0)
            continue;

        if (c == '\r' || c == '\n') {
            uart_put_char((char)c);
            buf[buf_idx] = '\0';

            int len = buf_idx;
            uart_put_str("Received: ");
            uart_put_str(buf);
            uart_put_str("\r\n");
            uart_put_str("The length is: ");

            int temp = len, digits = 0;
            do { temp /= 10; digits++; } while (temp > 0);
            temp = len;
            for (int i = digits - 1; i >= 0; i--) {
                buf2[i] = (temp % 10) + '0';
                temp /= 10;
            }
            buf2[digits] = '\0';
            uart_put_str(buf2);
            uart_put_str("\r\n\r\n");

            /* ---- 重置，等待下一次输入 ---- */
            uart_put_str("Please enter a string: \r\n");
            buf_idx = 0;
            /* input_done 保持 0，不设 1 */

        } else if (buf_idx < 1023) {  
            uart_put_char((char)c);
            buf[buf_idx++] = (char)c;
        }
    }
    
    return 0;
}
