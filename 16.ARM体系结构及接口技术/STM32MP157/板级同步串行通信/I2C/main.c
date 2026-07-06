#include "stm32mp157_i2c.h"
#include "stm32mp157_uart.h"
#include "stm32mp157_iwdg.h"
#include "delay.h"

/* ===== SI7006 温湿度传感器 ===== */
#define SI7006_SLAVE          0x40
#define WRITE_USER_REG_CMD    0xE6
#define WRITE_USER_REG_VALUE  0x3A    /* 12位湿度，14位温度，禁止加热器 */
#define MEASURE_HUM_CMD       0xE5
#define MEASURE_TEMP_CMD      0xE3


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


/*
 * itoa - 将无符号整数转为定长10位字符串（前补零）
 * @buffer: 输出缓冲区（至少11字节）
 * @num:    要转换的整数
 */
static void itoa(char *buffer, unsigned int num)
{
    int i = 9;
    unsigned int tmp;

    while (num) {
        tmp = num % 10;
        buffer[i] = tmp + '0';
        i--;
        num /= 10;
    }
    while (i >= 0)
        buffer[i--] = '0';
    buffer[10] = '\0';
}

/* 打印 16 位十六进制（固定 4 位，前补零） */
static void print_hex16(unsigned short val)
{
    static const char hex[] = "0123456789ABCDEF";
    char buf[5];

    buf[0] = hex[(val >> 12) & 0xF];
    buf[1] = hex[(val >>  8) & 0xF];
    buf[2] = hex[(val >>  4) & 0xF];
    buf[3] = hex[ val        & 0xF];
    buf[4] = '\0';
    uart_put_str(buf);
}

/*
 * si7006_init - SI7006 初始化
 * 写用户寄存器：12位湿度、14位温度、禁止加热器
 * 返回：0=成功，-1=传感器无应答
 */
static int si7006_init(void)
{
    int ack;

    i2c_init();
    uart_put_str("I2C GPIO init done\n");

    /* 传感器上电稳定（datasheet: max 25ms） */
    delay_ms(50);

    if (i2c_start() < 0) {
        uart_put_str("SI7006 init FAIL: bus busy/timeout\n");
        return -1;
    }
    ack = i2c_write(SI7006_SLAVE << 1);
    if (!ack) {
        uart_put_str("SI7006 init FAIL: no ACK on address 0x40\n");
        i2c_stop();
        return -1;
    }
    i2c_write(WRITE_USER_REG_CMD);
    i2c_write(WRITE_USER_REG_VALUE);
    i2c_stop();

    uart_put_str("SI7006 init OK\n");
    return 0;
}

/*
 * si7006_read_hum_data - 读取湿度
 */
static void si7006_read_hum_data(void)
{
    unsigned short hum;
    unsigned char hum_h, hum_l;
    char itoa_buf[11];

    if (i2c_start() < 0) {
        uart_put_str("SI7006 hum FAIL: bus busy/timeout\n");
        return;
    }
    i2c_write(SI7006_SLAVE << 1);
    i2c_write(MEASURE_HUM_CMD);
    if (i2c_start() < 0) {               /* RESTART */
        uart_put_str("SI7006 hum RESTART FAIL: bus busy/timeout\n");
        return;
    }
    i2c_write((SI7006_SLAVE << 1) | 1);
    delay_ms(100);

    uart_put_str("I2C start read hum data\n");

    hum_h = i2c_read(0);
    hum_l = i2c_read(1);
    i2c_stop();

    hum = (hum_h << 8) | hum_l;

    uart_put_str("hum raw: 0x");
    print_hex16(hum);
    uart_put_str(" (");
    itoa(itoa_buf, hum);
    uart_put_str(itoa_buf);
    uart_put_str(")\n");

    hum = 12500UL * hum / 65536 - 600;   /* 整数运算，同 100*(125*hum/65536-6) */

    itoa(itoa_buf, hum / 100);
    uart_put_str("current hum: ");
    uart_put_str(itoa_buf);
    uart_put_str(".");
    itoa(itoa_buf, hum % 100);
    uart_put_str(itoa_buf + 8);
    uart_put_str(" %RH\n");
}

/*
 * si7006_read_temp_data - 读取温度（整数运算，无浮点）
 */
static void si7006_read_temp_data(void)
{
    int temp;          /* 用 int，避免 short 溢出（>41°C 时 raw > 32767） */
    int raw;
    unsigned char temp_h, temp_l;
    char itoa_buf[11];

    if (i2c_start() < 0) {
        uart_put_str("SI7006 temp FAIL: bus busy/timeout\n");
        return;
    }
    i2c_write(SI7006_SLAVE << 1);
    i2c_write(MEASURE_TEMP_CMD);
    if (i2c_start() < 0) {               /* RESTART */
        uart_put_str("SI7006 temp RESTART FAIL: bus busy/timeout\n");
        return;
    }
    i2c_write((SI7006_SLAVE << 1) | 1);
    delay_ms(100);

    uart_put_str("I2C start read temp data\n");

    temp_h = i2c_read(0);
    temp_l = i2c_read(1);
    i2c_stop();

    raw = (temp_h << 8) | temp_l;

    uart_put_str("temp raw: 0x");
    print_hex16(raw);
    uart_put_str(" (");
    itoa(itoa_buf, raw);
    uart_put_str(itoa_buf);
    uart_put_str(")\n");

    /* 整数运算：17572 * raw / 65536 - 4685 = 100*(175.72*raw/65536-46.85) */
    temp = 17572 * raw / 65536 - 4685;

    if (temp < 0) {
        uart_put_char('-');
        temp = -temp;
    }
    itoa(itoa_buf, temp / 100);
    uart_put_str("current temp: ");
    uart_put_str(itoa_buf);
    uart_put_str(".");
    itoa(itoa_buf, temp % 100);
    uart_put_str(itoa_buf + 8);
    uart_put_str(" °C\n");
}

int main(void)
{   
    IWDG1->KR = IWDG_KEY_REFRESH;

    uart_init();
    uart_put_str("\n=== Boot OK ===\n");

    stm32mp157_delay_init();
    uart_put_str("delay init done\n");

    if (si7006_init() < 0) {
        uart_put_str("=== SI7006 init FAILED, halt ===\n");
        while (1)
            IWDG1->KR = IWDG_KEY_REFRESH;
    }
    uart_put_str("=== SI7006 init done ===\n");

    while (1) {
        IWDG1->KR = IWDG_KEY_REFRESH;


        si7006_read_temp_data();

        uart_put_str("\n");

        si7006_read_hum_data();

        delay_ms(5000);
    }

    return 0;
}