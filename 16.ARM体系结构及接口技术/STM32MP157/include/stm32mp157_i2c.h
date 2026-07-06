// include/stm32mp157_i2c.h
/* 软件模拟I2C */
#ifndef __STM32MP157_I2C__
#define __STM32MP157_I2C__

#include "stm32mp157_rcc.h"
#include "stm32mp157_gpio.h"
#include "delay.h" 
#include <stdint.h>

/* I2C always has 2 lines: SCL and SDA (half-duplex)
   Optional SMBus mode adds a 3rd line: SMBA (Alert) */

   /* PF14引脚为SCL，PF15引脚为SDA */

/*
 * SCL 时钟控制
 * SCL 始终由主机驱动，初始化时已配为输出，BSRR/BRR 原子操作
 */
#define I2C_SCL_INIT()     do {                                        \
    GPIOF->PUPDR &= ~(3 << (14 * 2));     /* 无上下拉 */                \
    GPIOF->OTYPER |= (1 << 14);           /* 开漏输出 */                \
    GPIOF->OSPEEDR &= ~(3 << (14 * 2));   /* 清速度位 */                \
    GPIOF->OSPEEDR |= (2 << (14 * 2));    /* 高速 */                   \
    GPIOF->BSRR = (1 << 14);              /* ODR 预置高电平 */           \
    GPIOF->MODER &= ~(3 << (14 * 2));     /* 清模式位 */                \
    GPIOF->MODER |= (1 << (14 * 2));      /* 输出模式 */                \
} while(0)

/* SCL 始终由主机驱动，无需释放，直接 BSRR 原子操作 */
#define I2C_SCL_HIGH()    (GPIOF->BSRR = (1 << 14))
#define I2C_SCL_LOW()     (GPIOF->BRR  = (1 << 14))

/*
 * SDA 数据控制
 * 拉高/拉低时先写 BSRR/BRR 再切 MODER，避免输出瞬间电平错误
 */
#define I2C_SDA_INIT()     do {                                        \
    GPIOF->PUPDR &= ~(3 << (15 * 2));     /* 无上下拉 */                \
    GPIOF->OTYPER |= (1 << 15);           /* 开漏输出 */                \
    GPIOF->OSPEEDR &= ~(3 << (15 * 2));   /* 清速度位 */                \
    GPIOF->OSPEEDR |= (2 << (15 * 2));    /* 高速 */                   \
    GPIOF->BSRR = (1 << 15);              /* ODR 预置高电平 */          \
    GPIOF->MODER &= ~(3 << (15 * 2));     /* 清模式位 */                \
    GPIOF->MODER |= (1 << (15 * 2));      /* 输出模式 */                \
} while(0)

#define I2C_SDA_HIGH()    do {                                         \
    GPIOF->BSRR = (1 << 15);             /* BSRR 原子置位 */            \
    GPIOF->MODER &= ~(3 << (15 * 2));                                  \
    GPIOF->MODER |= (1 << (15 * 2));                                   \
} while(0)

#define I2C_SDA_LOW()     do {                                         \
    GPIOF->BRR  = (1 << 15);             /* BRR 原子复位 */             \
    GPIOF->MODER &= ~(3 << (15 * 2));                                  \
    GPIOF->MODER |= (1 << (15 * 2));                                   \
} while(0)

/* 释放 SDA 控制权（输入 = 高阻，让从机或上拉电阻驱动） */
#define I2C_SDA_RELEASE() do {                                         \
    GPIOF->MODER &= ~(3 << (15 * 2));                                  \
} while(0)

#define I2C_DELAY()  delay_us(1)   /* 400 kHz 标准模式，1 µs 半周期延时 */

/* ===== 鲁棒性增强 ===== */

/* 超时：等待SCL高电平的最多循环次数（约3-4ms @ 800MHz） */
#define I2C_TIMEOUT_CNT  500000

/* 临界区保护：关/开全局中断（Cortex-A7 CPSID/CPSIE） */
#define I2C_ENTER_CRITICAL()  __asm__ volatile("cpsid i" : : : "memory")
#define I2C_EXIT_CRITICAL()   __asm__ volatile("cpsie i" : : : "memory")

// 硬件I2C 寄存器结构体定义（仅包含常用寄存器）
typedef struct {
    volatile unsigned int CR1;      // 0x00
    volatile unsigned int CR2;      // 0x04
    volatile unsigned int OAR1;     // 0x08
    volatile unsigned int OAR2;     // 0x0C
    volatile unsigned int TIMINGR;  // 0x10
    volatile unsigned int TIMEOUTR; // 0x14
    volatile unsigned int ISR;      // 0x18
    volatile unsigned int ICR;      // 0x1C
    volatile unsigned int PECR;     // 0x20
    volatile unsigned int RXDR;     // 0x24
    volatile unsigned int TXDR;     // 0x28
} i2c_t;

/* 硬件 I2C 基地址（APB1 / APB5） */
#define I2C1  ((i2c_t *)0x40012000)
#define I2C2  ((i2c_t *)0x40013000)
#define I2C3  ((i2c_t *)0x40014000)
#define I2C5  ((i2c_t *)0x40015000)
#define I2C4  ((i2c_t *)0x5C002000)
#define I2C6  ((i2c_t *)0x5C009000)

void i2c_init(void);

/* ===== 低级原语（灵活拼装任意I2C协议） ===== */
/**
 * i2c_start  - 发送 START 条件
 * i2c_stop   - 发送 STOP 条件
 * i2c_write  - 发送一个字节，返回 ACK 状态（1=ACK，0=NACK，-1=超时）
 * i2c_read   - 接收一个字节，参数 ack=1 发 NACK，ack=0 发 ACK
 */
int      i2c_start(void);
int      i2c_stop(void);
int      i2c_write(uint8_t byte);
uint8_t  i2c_read(int ack);

/* ===== 寄存器级上层接口 ===== */
/**
 * i2c_write_reg - 向从机指定寄存器写入一个字节
 * @dev_addr:  从机地址（7位，左对齐，如 0x50）
 * @reg_addr:  寄存器地址
 * @data:      要写入的数据
 * @return:    0=成功，-1=NACK错误
 */
int i2c_write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);

/**
 * i2c_write_regs - 向从机连续寄存器写入多个字节
 * @dev_addr:  从机地址（7位）
 * @reg_addr:  起始寄存器地址
 * @data:      数据缓冲区指针
 * @len:       字节数
 * @return:    0=成功，-1=NACK错误
 */
int i2c_write_regs(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *data, uint32_t len);

/**
 * i2c_read_reg - 从从机指定寄存器读取一个字节
 * @dev_addr:  从机地址（7位）
 * @reg_addr:  寄存器地址
 * @data:      输出缓冲区指针
 * @return:    0=成功，-1=NACK错误
 */
int i2c_read_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data);

/**
 * i2c_read_regs - 从从机连续寄存器读取多个字节
 * @dev_addr:  从机地址（7位）
 * @reg_addr:  起始寄存器地址
 * @buf:       接收缓冲区指针
 * @len:       要读取的字节数
 * @return:    0=成功，-1=NACK错误
 */
int i2c_read_regs(uint8_t dev_addr, uint8_t reg_addr, uint8_t *buf, uint32_t len);

/**
 * i2c_bus_recover - I2C总线恢复
 * 发送最多9个SCL时钟脉冲，直到SDA被释放
 * 返回：0=恢复成功，-1=恢复失败
 */
int i2c_bus_recover(void);

#endif