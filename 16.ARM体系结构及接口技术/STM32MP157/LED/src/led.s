.text
.global _start
.equ RCC_MP_AHB4ENSETR, 0x50000A28
.equ GPIOE_MODER, 0x50006000
.equ GPIOE_OTYPER, 0x50006004   
.equ GPIOE_OSPEEDR, 0x50006008
.equ GPIOE_PUPDR, 0x5000600C
.equ GPIOE_IDR, 0x50006010 @ 输入数据寄存器
.equ GPIOE_ODR, 0x50006014 @ 输出输出数据寄存器地址
.equ IWDG1_KR, 0x5A002000  @ IWDG1 关键字寄存器（看门狗）
.equ IWDG1_PR,  0x5A002004  @ IWDG1 预分频寄存器
.equ IWDG1_RLR, 0x5A002008  @ IWDG1 重装载寄存器

_start:
    /*GPIO E10 管脚初始化*/
    /*1.使能 GPIOE 控制器时钟*/
    ldr r0, =RCC_MP_AHB4ENSETR @ 伪指令，将RCC_MP_AHB4ENSETR的地址加载到r0寄存器中
    ldr r1, [r0] @ 加载 读取寄存器[r0]值到r1寄存器中
    orr r1, r1,  #(1 << 4) @ 修改 按位或 (1 << 4)表示将数字1左移4位，即00010000，表示使能GPIOE控制器时钟
    str r1, [r0] @ 写回

    /*读出 加载 修改 写回 */

    /*2.设置 GPIOE10 为输出模式*/
    ldr r0, =GPIOE_MODER 
    ldr r1, [r0] 
    bic r1, r1, #(3 << 20) @ 20,21都清零
    orr r1, r1, #(1 << 20) @ 20位置1，设置为输出模式
    str r1, [r0] 

    /*3.设置 GPIOE10 输出类型为推挽输出*/
    ldr r0, =GPIOE_OTYPER
    ldr r1, [r0]
    bic r1, r1, #(1 << 10) @ 将10位置0，设置为推挽输出
    str r1, [r0]

    /*4.设置低速模式*/
    ldr r0, =GPIOE_OSPEEDR
    ldr r1, [r0]
    bic r1, r1, #(3 << 20) @ 20,21都清零
    str r1, [r0]

    /*5.设置禁止上拉/下拉*/
    ldr r0, =GPIOE_PUPDR
    ldr r1, [r0]
    bic r1, r1, #(3 << 20) @ 20,21都清零
    str r1, [r0]


    /* 1. 解锁 */
    ldr r0, =IWDG1_KR
    ldr r1, =0x5555
    str r1, [r0]

    /* 2. 设置分频器（示例：4分频）*/
    ldr r0, =IWDG1_PR
    mov r1, #0
    str r1, [r0]

    /* 3. 设置重装载值（示例：0xFFF）*/
    ldr r0, =IWDG1_RLR
    ldr r1, =0xFFF
    str r1, [r0]

    /* 4. 启动看门狗 */
    ldr r0, =IWDG1_KR
    ldr r1, =0xCCCC
    str r1, [r0]

    /*循环led闪烁*/
loop:
    /*亮灯*/
    ldr r0, =GPIOE_ODR
    ldr r1, [r0]
    orr r1, r1, #(1 << 10) @ 将10位置1，点亮LED
    str r1, [r0]

    bl delay

    /*灭灯*/
    ldr r0, =GPIOE_ODR
    ldr r1, [r0]
    bic r1, r1, #(1 << 10) @ 将10位置0，熄灭LED
    str r1, [r0]

    bl delay
    b loop

delay: @ 延时函数
    /* 喂狗：向 IWDG_KR 写入 0xAAAA 重装载看门狗计数器 */
    ldr r3, =IWDG1_KR
    ldr r4, =0xAAAA
    str r4, [r3]
    ldr r2, =50000000 @ 延时计数器初始值
delay_loop:
    subs r2, r2, #1 @ 计数器递减
    bne delay_loop @ 如果Z=0（结果非零），继续循环（）
    bx lr @ 返回调用函数

.end
