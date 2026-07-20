// src/uart.c
#include "../../include/stm32mp157_rcc.h"
#include "../../include/stm32mp157_gpio.h"
#include "../../include/stm32mp157_uart.h"
#include "../../include/stm32mp157_iwdg.h"

#define USART_KER_CKPRES 64000000 // APB1时钟频率 64MHz
#define TX_OR_RX_BAUD 115200 // 波特率

/*115200 8N1 NON-FIFO*/
void uart_init(void)
{
    /*1. 使能GPIOB GPIOG UART4控制器时钟*/
    RCC->MP_APB1ENSETR |= (1<<16); // UART4
    RCC->MP_AHB4ENSETR |= (1<<6 | 1<<1); // GPIOG GPIOB
    /*2. 管脚复用功能设置*/
    GPIOB->MODER &= ~(3<<4); // PB2清零
    GPIOB->MODER |= (2<<4); // PB2 AF

    GPIOG->MODER &= ~(3<<22); // PG6清零
    GPIOG->MODER |= (2<<22); // PG6 AF

    GPIOB->AFRL &= ~(0xF<<8); 
    GPIOB->AFRL |= (8<<8); // PB2 AF8

    GPIOG->AFRH &= ~(0xF<<12);
    GPIOG->AFRH |= (0x6<<12);

    /*UART 控制器设置： 115200 8n1 （8bit位、non奇偶校验、1位停止位）*/
    // CR1 uart控制
    USART4->CR1 &= ~(1<<0); // disable uart4
    USART4->CR1 &= ~(1<<29); // disable fifo
    USART4->CR1 &= ~(1<<28 | 1<<12); // 8bit位
    USART4->CR1 &= ~(1<<15); // 16倍过采样
    USART4->CR1 &= ~(3<<9); // 只要10位为0就是无校验

    // CR2 uart协议规定先传最低位
    USART4->CR2 &= ~(1<<19); // LSB
    USART4->CR2 &= ~(3<<12); // 1位停止位
    // PRESC uart分频预分频
    USART4->PRESC &= ~(0xF<<0); // 1分频
    // BRR 第二次分频
    USART4->BRR = USART_KER_CKPRES / TX_OR_RX_BAUD; // 115200波特率所需时钟频率
    
    USART4->CR1 |= (1<<2); // enable receiver
    USART4->CR1 |= (1<<3); // enable transmitter
    USART4->CR1 |= (1<<0); // enable uart4

}

void uart_put_char(char c)
{
    // 统一：\r 和 \n 都输出 \r\n（终端兼容）
    if (c == '\n' || c == '\r') {
        while (!(USART4->ISR & (1<<7))); // 等待 TXE=1
        USART4->TDR = '\r';
        while (!(USART4->ISR & (1<<7)));
        USART4->TDR = '\n';
        return;
    }
    while (!(USART4->ISR & (1<<7)));     // 等待 TXE=1
    USART4->TDR = c;
}

void uart_put_str(const char *str)
{
    while(*str){
        uart_put_char(*str++);
    }
}

char uart_get_char(void)
{
    while (1){ // 阻塞忙等待 RXNE=1，接收数据寄存器有完整一字节了，可读RDR
        if (!(USART4->ISR & (1<<5))) {
            // 喂狗
            IWDG1->KR = IWDG_KEY_REFRESH;
        }else{
            break;
        }
    }
   
    return (char)USART4->RDR; // 读取数据寄存器，返回接收到的字符
}

int uart_get_str(char *buf, int max_len)
{   
    int i = 0;
    char c;

    while (i < max_len - 1) { // 留一个位置给字符串结束符
        c = uart_get_char();

        if (c == '\r' || c == '\n') {
            uart_put_char(c);           // 回显（uart_put_char 统一转 \r\n）
            buf[i++] = '\n';            // 统一存 \n
            break;
        }

        uart_put_char(c);               // 回显普通字符
        buf[i++] = c;
    }

    buf[i] = '\0'; // 字符串结尾

    return i; // 返回接收到的字符数 = 字符串长度
}
