// include/stm32mp157_uart.h
#ifndef __STM32MP157_UART_H__
#define __STM32MP157_UART_H__

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
}uart_t;

#define USART1  ((uart_t *)0x5C000000)
#define USART2  ((uart_t *)0x4000E000)
#define USART3  ((uart_t *)0x4000F000)
#define USART4  ((uart_t *)0x40010000)
#define USART5  ((uart_t *)0x40011000)
#define USART6  ((uart_t *)0x44003000)
#define USART7  ((uart_t *)0x40018000)
#define USART8  ((uart_t *)0x40019000)

void uart_init(void);
void uart_put_char(char c);
void uart_put_str(const char *str);
char uart_get_char(void);
int  uart_get_str(char *buf, int max_len);

#endif