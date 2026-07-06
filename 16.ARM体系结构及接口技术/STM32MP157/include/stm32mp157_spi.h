// include/stm32mp157_spi.h
/* 软件模拟SPI */
#ifndef __STM32MP157_SPI__
#define __STM32MP157_SPI__

#include "stm32mp157_rcc.h"
#include "stm32mp157_gpio.h"
#include "delay.h" 

/*
 * SPI Configurations (Pin Count):
 *   4 lines (Standard):   SCK (SCL/SCLK) , MOSI, MISO, CS (NCS/NSS/SS)   — 1 master + 1 slave
 *   3 lines (Half-duplex): SCK, MOSI(bidirectional), CS — 收发共用一根数据线
 *   2 lines:               SCK + Data (no CS)        — 单从机，CS 常拉低
 *   N+3 lines:             SCK, MOSI, MISO + 每从机独立 CS — 多从机拓扑
 */

/* PE11-CS/RCLK, PE12-SCK, PE14-MOSI, PE13-MISO */

/* ===== 电平控制宏（软件 SPI 位操作） ===== */
#define SPI_CS_LOW()      (GPIOE->BRR  = (1 << 11))   /* 片选使能 / RCLK 低 */
#define SPI_CS_HIGH()     (GPIOE->BSRR = (1 << 11))   /* 片选释放 / RCLK 高 */
#define SPI_RCLK_LOW()    SPI_CS_LOW()
#define SPI_RCLK_HIGH()   SPI_CS_HIGH()

#define SPI_SCK_LOW()     (GPIOE->BRR  = (1 << 12))
#define SPI_SCK_HIGH()    (GPIOE->BSRR = (1 << 12))

#define SPI_MOSI_LOW()    (GPIOE->BRR  = (1 << 14))
#define SPI_MOSI_HIGH()   (GPIOE->BSRR = (1 << 14))

#define SPI_MISO_READ()   ((GPIOE->IDR >> 13) & 0x1)  /* 读 MISO 引脚电平 */

/* ── 通用 SPI: CS 空闲高（从机未选中），传输期间拉低 ── */
#define SPI_CS_INIT()     do {                                         \
    GPIOE->PUPDR &= ~(3 << (11 * 2));     /* 无上下拉 */                \
    GPIOE->OTYPER &= ~(1 << 11);          /* 推挽输出 */                \
    GPIOE->OSPEEDR &= ~(3 << (11 * 2));   /* 清速度位 */                \
    GPIOE->OSPEEDR |= (2 << (11 * 2));    /* 高速 */                   \
    GPIOE->BSRR = (1 << 11);              /* ODR 预置高电平（未选中） */  \
    GPIOE->MODER &= ~(3 << (11 * 2));     /* 清模式位 */                \
    GPIOE->MODER |= (1 << (11 * 2));      /* 输出模式 */                \
} while(0)

/* ── 595 专用: PE11 复用为 RCLK（锁存脚），空闲低电平 ── */
#define SPI_RCLK_INIT()   do {                                         \
    GPIOE->PUPDR &= ~(3 << (11 * 2));     /* 无上下拉 */                \
    GPIOE->OTYPER &= ~(1 << 11);          /* 推挽输出 */                \
    GPIOE->OSPEEDR &= ~(3 << (11 * 2));   /* 清速度位 */                \
    GPIOE->OSPEEDR |= (2 << (11 * 2));    /* 高速 */                   \
    GPIOE->BRR  = (1 << 11);              /* ODR 预置低电平（RCLK 空闲） */ \
    GPIOE->MODER &= ~(3 << (11 * 2));     /* 清模式位 */                \
    GPIOE->MODER |= (1 << (11 * 2));      /* 输出模式 */                \
} while(0)

/* ===== 595 专用 ===== */
/* 595 锁存脉冲：RCLK 上升沿将移位寄存器数据输出到 Q0~Q7 */
#define SPI_595_LATCH()  do {           \
    SPI_CS_HIGH();                      \
    delay_us(1);                        \
    SPI_CS_LOW();                       \
} while(0)

//////////////////////////////////////////////////////////////////////////

/* SCK: 主机驱动，推挽输出，空闲低电平 */
#define SPI_SCK_INIT()     do {                                        \
    GPIOE->PUPDR &= ~(3 << (12 * 2));     /* 无上下拉 */                \
    GPIOE->OTYPER &= ~(1 << 12);          /* 推挽输出 */                \
    GPIOE->OSPEEDR &= ~(3 << (12 * 2));   /* 清速度位 */                \
    GPIOE->OSPEEDR |= (2 << (12 * 2));    /* 高速 */                   \
    GPIOE->BRR  = (1 << 12);              /* ODR 预置低电平（空闲） */    \
    GPIOE->MODER &= ~(3 << (12 * 2));     /* 清模式位 */                \
    GPIOE->MODER |= (1 << (12 * 2));      /* 输出模式 */                \
} while(0)

/* MOSI: 主机驱动，推挽输出 */
#define SPI_MOSI_INIT()     do {                                       \
    GPIOE->PUPDR &= ~(3 << (14 * 2));     /* 无上下拉 */                \
    GPIOE->OTYPER &= ~(1 << 14);          /* 推挽输出 */                \
    GPIOE->OSPEEDR &= ~(3 << (14 * 2));   /* 清速度位 */                \
    GPIOE->OSPEEDR |= (2 << (14 * 2));    /* 高速 */                   \
    GPIOE->BSRR = (1 << 14);              /* ODR 预置高电平 */          \
    GPIOE->MODER &= ~(3 << (14 * 2));     /* 清模式位 */                \
    GPIOE->MODER |= (1 << (14 * 2));      /* 输出模式 */                \
} while(0)

/* MISO: 从机驱动，主机端配置为输入（浮空） */
#define SPI_MISO_INIT()     do {                                       \
    GPIOE->PUPDR &= ~(3 << (13 * 2));     /* 无上下拉 */                \
    GPIOE->OSPEEDR &= ~(3 << (13 * 2));   /* 清速度位 */                \
    GPIOE->OSPEEDR |= (2 << (13 * 2));    /* 高速 */                   \
    GPIOE->MODER &= ~(3 << (13 * 2));     /* 输入模式 (00) */           \
} while(0)

// TODO：硬件SPI 寄存器结构体定义（仅包含常用寄存器）

/* ===== 通用初始化 ===== */
/**
 * spi_init - SPI 初始化
 * @mode:  NULL 或 "" → 标准 SPI (CS 空闲高)
 *         "595"      → 595 模式 (PE11=RCLK, 空闲低)
 */
void spi_init_mode(const char *mode);
#define spi_init()       spi_init_mode(NULL)    // 无参 → 标准
#define spi_init_595()   spi_init_mode("595")   // 595 → RCLK


/**
 * spi_write - SPI 写数据
 * @data: 要写入的数据
 */
void spi_write(unsigned char data);


#endif