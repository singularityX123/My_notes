#include "stm32mp157_i2c.h"

void i2c_init(void) {
    /* 使能GPIOF时钟 */
    RCC->MP_AHB4ENSETR |= (1 << 5);  /* GPIOF时钟使能 */

    /* 初始化SCL和SDA引脚 */
    I2C_SCL_INIT();
    I2C_SDA_INIT();
}

/*
 * I2C 标准速度等级及对应延时
 *
 * 参数           标准模式(100kHz)  快速模式(400kHz)  快速+(1MHz)
 * ─────────────────────────────────────────────────────────────
 * SCL 频率          100 kHz         400 kHz         1 MHz
 * 半周期             5 µs           1.25 µs         0.5 µs
 * 起始保持时间        4 µs           0.6 µs          0.26 µs
 * 数据建立时间       250 ns          100 ns           50 ns
 *
 * #define I2C_DELAY()  delay_us(5)    ← 100 kHz，最通用
 * #define I2C_DELAY()  delay_us(1)    ← 400 kHz
 * 快速+(1MHz)在mp157上靠指令本身的执行时间就够，让代码裸跑就行，不需要额外延时
 */

/**
 * i2c_wait_scl_high - 等待SCL实际变高（处理从机时钟拉伸，带超时）
 * 返回：0=成功，-1=超时
 */
static int i2c_wait_scl_high(void)
{
    uint32_t timeout = I2C_TIMEOUT_CNT;

    I2C_SCL_HIGH();                 /* 释放SCL（开漏输出，从机可拉低做时钟拉伸） */
    while (!(GPIOF->IDR & (1 << 14)) && --timeout){

    }

    return timeout ? 0 : -1;
}

int i2c_start(void)
{
    /* 总线忙检测：释放 SDA 后若仍为低，说明从机占用总线 */
    I2C_SDA_RELEASE();
    I2C_DELAY();
    if (!(GPIOF->IDR & (1 << 15))) {
        I2C_SDA_INIT();             /* 恢复 SDA 为输出模式 */
        return -1;
    }

    I2C_SDA_HIGH();                 /* 确保 SDA 为高 */
    if (i2c_wait_scl_high() < 0)    /* SCL 上升沿 + 时钟拉伸检测 */
        return -1;
    I2C_DELAY();
    I2C_SDA_LOW();                  /* SDA 先变低 → 起始条件 */
    I2C_DELAY();
    I2C_SCL_LOW();                  /* 拉低 SCL 准备传数据 */
    I2C_DELAY();
    return 0;
}

int i2c_stop(void)
{
    I2C_SDA_LOW();
    if (i2c_wait_scl_high() < 0)    /* SCL 上升沿 + 时钟拉伸检测 */
        return -1;
    I2C_DELAY();
    I2C_SDA_HIGH();                 /* SDA 变高 → 停止条件 */
    I2C_DELAY();
    return 0;
}

/**
 * i2c_write - 发送一个字节（8位数据 + 第9时钟等 ACK）
 * @byte:  要发送的数据
 * @return: 1=ACK, 0=NACK, -1=超时
 */
int i2c_write(uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        if (byte & 0x80)
            I2C_SDA_HIGH();
        else
            I2C_SDA_LOW();

        I2C_DELAY();
        if (i2c_wait_scl_high() < 0)
            return -1;              /* SCL 时钟拉伸超时 */
        I2C_DELAY();
        I2C_SCL_LOW();
        byte <<= 1;
    }

    /* 第 9 个时钟：释放 SDA，从机拉低=ACK */
    I2C_SDA_RELEASE();
    I2C_DELAY();
    if (i2c_wait_scl_high() < 0)
        return -1;                  /* 超时 */
    I2C_DELAY();
    int ack = !(GPIOF->IDR & (1 << 15));
    I2C_SCL_LOW();
    return ack;                     /* 1=ACK, 0=NACK */
}

/**
 * i2c_read - 接收一个字节（8位数据 + 第9时钟发 ACK/NACK）
 * @ack:  0=发ACK(继续读), 1=发NACK(停止读)
 * @return: 读取到的字节
 */
uint8_t i2c_read(int ack)
{
    uint8_t byte = 0;

    I2C_SDA_RELEASE();              /* 释放 SDA 由从机驱动 */
    for (int i = 0; i < 8; i++) {
        byte <<= 1;
        if (i2c_wait_scl_high() < 0)
            return 0;               /* SCL 超时，数据无效 */
        I2C_DELAY();
        if (GPIOF->IDR & (1 << 15))
            byte |= 1;
        I2C_SCL_LOW();
        I2C_DELAY();
    }

    /* 第 9 个时钟：发 ACK/NACK */
    if (ack) {
        I2C_SDA_HIGH();             /* NACK */
    } else {
        I2C_SDA_LOW();              /* ACK */
    }
    I2C_DELAY();
    if (i2c_wait_scl_high() < 0)    /* SCL 上升沿 + 时钟拉伸检测 */
        return 0;
    I2C_DELAY();
    I2C_SCL_LOW();
    I2C_DELAY();

    return byte;
}

/* ===== 总线恢复 ===== */

/**
 * i2c_bus_recover - I2C总线恢复
 * 发送最多9个SCL时钟脉冲，直到SDA被释放，最后发STOP
 * 返回：0=恢复成功，-1=恢复失败（9个脉冲后SDA仍被拉低）
 */
int i2c_bus_recover(void)
{
    int ret = -1;

    /* 重新初始化引脚，确保干净状态 */
    I2C_SCL_INIT();
    I2C_SDA_INIT();                     /* 完整重新初始化 SDA */
    I2C_SDA_RELEASE();                  /* 释放SDA，让上拉电阻拉高 */
    I2C_DELAY();

    for (int i = 0; i < 9; i++) {
        I2C_SCL_LOW();
        I2C_DELAY();
        if (i2c_wait_scl_high() < 0)    /* SCL 上升沿 + 时钟拉伸检测 */
            break;
        I2C_DELAY();

        if (GPIOF->IDR & (1 << 15)) {   /* SDA 已释放 */
            ret = 0;
            break;
        }
    }

    /* 发 STOP 条件，使总线回到空闲态 */
    I2C_SDA_LOW();
    I2C_DELAY();
    I2C_SCL_HIGH();
    I2C_DELAY();
    I2C_SDA_HIGH();
    I2C_DELAY();

    return ret;
}

/* ===== 寄存器级上层接口（带临界区保护） ===== */

/* 向从机指定寄存器写入一个字节 */
/* 返回：0=成功，-1=NACK/超时错误 */
int i2c_write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data)
{
    int ret = 0;

    I2C_ENTER_CRITICAL();
    if (i2c_start()                   < 0) { ret = -1; goto fail; }
    if (i2c_write(dev_addr << 1 | 0) <= 0) { ret = -1; goto fail; }
    if (i2c_write(reg_addr)           <= 0) { ret = -1; goto fail; }
    if (i2c_write(data)               <= 0) { ret = -1; goto fail; }
    i2c_stop();
    I2C_EXIT_CRITICAL();
    return 0;

fail:
    i2c_stop();
    I2C_EXIT_CRITICAL();
    return ret;
}

/* 向从机连续寄存器写入多个字节 */
/* 返回：0=成功，-1=NACK/超时错误 */
int i2c_write_regs(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *data, uint32_t len)
{
    int ret = 0;

    I2C_ENTER_CRITICAL();
    if (i2c_start()                   < 0) { ret = -1; goto fail; }
    if (i2c_write(dev_addr << 1 | 0) <= 0) { ret = -1; goto fail; }
    if (i2c_write(reg_addr)           <= 0) { ret = -1; goto fail; }

    for (uint32_t i = 0; i < len; i++) {
        if (i2c_write(data[i]) <= 0) { ret = -1; goto fail; }
    }

    i2c_stop();
    I2C_EXIT_CRITICAL();
    return 0;

fail:
    i2c_stop();
    I2C_EXIT_CRITICAL();
    return ret;
}

/* 从从机指定寄存器读取一个字节 */
/* 返回：0=成功，-1=NACK/超时错误 */
int i2c_read_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data)
{
    int ret = 0;

    I2C_ENTER_CRITICAL();
    /* 第一步：写寄存器地址 */
    if (i2c_start()                   < 0) { ret = -1; goto fail; }
    if (i2c_write(dev_addr << 1 | 0) <= 0) { ret = -1; goto fail; }
    if (i2c_write(reg_addr)           <= 0) { ret = -1; goto fail; }

    /* 第二步：RESTART + 切换方向读数据 */
    if (i2c_start()                   < 0) { ret = -1; goto fail; }
    if (i2c_write(dev_addr << 1 | 1) <= 0) { ret = -1; goto fail; }
    *data = i2c_read(1);                    /* NACK — 只读一个字节 */
    i2c_stop();
    I2C_EXIT_CRITICAL();
    return 0;

fail:
    i2c_stop();
    I2C_EXIT_CRITICAL();
    return ret;
}

/* 从从机连续寄存器读取多个字节 */
/* 返回：0=成功，-1=NACK/超时错误 */
int i2c_read_regs(uint8_t dev_addr, uint8_t reg_addr, uint8_t *buf, uint32_t len)
{
    int ret = 0;

    I2C_ENTER_CRITICAL();
    /* 第一步：写寄存器地址 */
    if (i2c_start()                   < 0) { ret = -1; goto fail; }
    if (i2c_write(dev_addr << 1 | 0) <= 0) { ret = -1; goto fail; }
    if (i2c_write(reg_addr)           <= 0) { ret = -1; goto fail; }

    /* 第二步：RESTART + 连续读取 */
    if (i2c_start()                   < 0) { ret = -1; goto fail; }
    if (i2c_write(dev_addr << 1 | 1) <= 0) { ret = -1; goto fail; }

    for (uint32_t i = 0; i < len; i++) {
        if (i == len - 1)
            buf[i] = i2c_read(1);       /* 最后一个字节 → NACK */
        else
            buf[i] = i2c_read(0);       /* 前面所有字节 → ACK，继续读 */
    }

    i2c_stop();
    I2C_EXIT_CRITICAL();
    return 0;

fail:
    i2c_stop();
    I2C_EXIT_CRITICAL();
    return ret;
}

