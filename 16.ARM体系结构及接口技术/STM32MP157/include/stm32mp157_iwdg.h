// include/stm32mp157_iwdg.h
#ifndef __STM32MP157_IWDG__
#define __STM32MP157_IWDG__

/* IWDG 寄存器映射（独立看门狗）*/
typedef struct {
    volatile unsigned int KR;    // 0x00 — 密钥寄存器
    volatile unsigned int PR;    // 0x04 — 预分频寄存器
    volatile unsigned int RLR;   // 0x08 — 重装载寄存器
    volatile unsigned int SR;    // 0x0C — 状态寄存器
    volatile unsigned int WINR;  // 0x10 — 窗口寄存器
} iwdg_t;

#define IWDG1 ((iwdg_t *)0x5A002000)
#define IWDG2 ((iwdg_t *)0x5A004000)

/* IWDG_KR 密钥值 */
#define IWDG_KEY_ENABLE   0xCCCC  // 启动看门狗
#define IWDG_KEY_REFRESH  0xAAAA  // 喂狗，重装载计数器
#define IWDG_KEY_UNLOCK   0x5555  // 解锁 PR/RLR 写访问

#define IWDG1_INIT() do{            \
    IWDG1->KR  = IWDG_KEY_UNLOCK;   \
    IWDG1->PR  = 0x01;              \
    while (IWDG1->SR & (1 << 0));   \
    IWDG1->RLR = 0xFFF0;            \
    while (IWDG1->SR & (1 << 1));   \
    IWDG1->KR  = IWDG_KEY_ENABLE;   \
} while(0)

#endif
