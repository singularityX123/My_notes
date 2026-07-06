// src/stm32mp157_spi.c
#include "stm32mp157_spi.h"

void spi_init_mode(const char *mode)
{
    /* 使能 GPIOE 时钟 */
    RCC->MP_AHB4ENSETR |= (1 << 4);  /* GPIOE 时钟使能 */

    /* CS/RCLK 二选一 */
    if (mode && mode[0] == '5' && mode[1] == '9' && mode[2] == '5' && mode[3] == '\0')
        SPI_RCLK_INIT();             /* 595: RCLK 空闲低 */
    else
        SPI_CS_INIT();               /* 标准 SPI: CS 空闲高 */

    SPI_SCK_INIT();
    SPI_MISO_INIT();
    SPI_MOSI_INIT();
}

// 配合595版
void spi_write(unsigned char data)
{
    unsigned char i;
    for(i = 0; i < 8; i++)
    {
        if(data & 0x80)
        {
            SPI_MOSI_HIGH();  // MOSI线写高
        } else {
            SPI_MOSI_LOW();  // MOSI线写低
        }
        data <<= 1;
        // 时钟线从低电平到高电平的变化时，MOSI数据线上的数据
        // 被写到595芯片的移位寄存器中
        SPI_SCK_LOW();   // SCK拉低
        delay_us(10);
        SPI_SCK_HIGH();   // SCK拉高
        delay_us(10);
    }
    /* 锁存由调用者用 SPI_595_LATCH() 完成（HIGH→LOW, 上升沿锁存并恢复空闲） */
}


// void spi_write(unsigned char data)
// {
//     unsigned char i;
//     for(i = 0; i < 8; i++)
//     {
//         SPI_SCK_LOW();              // ① 下降沿（为改变数据做准备）
//         delay_us(1);

//         if(data & 0x80)             // ② SCK 低电平时设置 MOSI 
//             SPI_MOSI_HIGH();
//         else
//             SPI_MOSI_LOW();
//         data <<= 1;

//         delay_us(1);                // ③ 数据建立时间

//         SPI_SCK_HIGH();             // ④ 上升沿 → 595 采样 
//         delay_us(10);
//     }
//     SPI_SCK_LOW();                  // ⑤ 恢复 SCK 空闲低电平
// }