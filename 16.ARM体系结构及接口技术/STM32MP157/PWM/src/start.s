@ src/start.s - PWM 项目裸机启动代码
@ 负责：设置栈、禁用看门狗、跳转 main

.text
.global _start

.equ IWDG1_KR, 0x5A002000
.equ IWDG2_KR, 0x5A004000

_start:
    @ 1. 设置栈指针（DDR 顶部 256MB 处，U-Boot 只用了低端）
    ldr sp, =0xC4000000

    @ 2. 配置并启动 IWDG1（独立看门狗）
    @    流程: 解锁 → 设预分频 → 设重载值 → 启动 → 刷新
    @    LSI=32kHz, PR=7(256分频), RLR=0xFFF → 超时≈32.7s
    ldr r0, =IWDG1_KR
    ldr r1, =0x5555        @ 解锁密钥（使能 PR/RLR 写访问）
    str r1, [r0]

    ldr r0, =0x5A002004     @ IWDG1_PR (预分频)
    ldr r1, =0x7            @ 256 分频（最大值）
    str r1, [r0]

    ldr r0, =0x5A002008     @ IWDG1_RLR (重装载)
    ldr r1, =0xFFF          @ 最大重装载值 (4095)
    str r1, [r0]

    ldr r0, =IWDG1_KR
    ldr r1, =0xCCCC         @ ★ 启动看门狗（关键！之前漏掉了）
    str r1, [r0]

    ldr r0, =IWDG1_KR
    ldr r1, =0xAAAA         @ 刷新看门狗，使新配置生效
    str r1, [r0]

    @ 注意：IWDG2 在 STM32MP157 上不存在，已移除

    @ 3. 跳转 C 入口
    bl main

    @ 4. main 返回后死循环（不返回 U-Boot）
halt:
    b halt

.size _start, .-_start
