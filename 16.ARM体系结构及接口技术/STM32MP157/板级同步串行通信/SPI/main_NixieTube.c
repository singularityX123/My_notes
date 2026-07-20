#include "../../include/stm32mp157_spi.h"
#include "../../include/stm32mp157_iwdg.h"
#include "../../include/stm32mp157_uart.h"
#include "../../include/delay.h"
#include <stdint.h>

/* 栈空间：4KB，编译器自动放入 .bss */
__attribute__((aligned(8))) uint8_t _stack[4096];

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

unsigned char code[] = {
    0x3f, //0
    0x06, //1
    0x5b, //2
    0x4f, //3
    0x66, //4
    0x6d, //5
    0x7d, //6
    0x07, //7
    0x7f, //8
    0x6f, //9
};
unsigned char which[] = {
    0x1, //sg0
    0x2, //sg1
    0x4, //sg2
    0x8, //sg3
};

void show_num(void){
    int i = 0;
    for(i=0; i<10; i++){
        spi_write(0x0f);
        spi_write(code[i]);
        SPI_595_LATCH();
        delay_ms(1000);
    }
}
void show_diff_num(void){
    int i = 0;
    int j = 3000;
    while(j--){
        for(; i<4; i++){
            spi_write(which[i]);
            spi_write(code[i]);
            SPI_595_LATCH();
            delay_ms(5);  // 每位显示 5ms
        }
        i = 0;      
    }
}

int main(void)
{
    /* 延时定时器初始化（必须在任何 delay_xx 调用之前） */
    stm32mp157_delay_init();

    /* SPI 初始化 */
    spi_init_595();  // 595 模式初始化

    uart_init(); 
    uart_put_str("\n=== SPI init done ===\r\n");

    while(1)
    {
        IWDG1->KR = IWDG_KEY_REFRESH;

        show_num();
        show_diff_num();

        uart_put_str("=== Main loop done ===\r\n");

        delay_ms(10000);  // 延时 10 秒
        
    }

    return 0;
}