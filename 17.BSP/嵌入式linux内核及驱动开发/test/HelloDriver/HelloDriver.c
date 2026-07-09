/*写内核驱动模块时必须加的两个头文件*/
#include <linux/init.h>
#include <linux/module.h>
/*
    static, 标识该函数本模块可见
    __init, 它是一个宏
            被其修饰的函数链接时会放入 .init.text段中
            该段中的代码执行一次对应的内存空间就释放
*/
static int __init hellodriver_init(void){
    /*类似于printf*/
    printk("Hello driver!\n");
    return 0;
}
/*
    __exit, 它是一个宏
             被其修饰的函数链接时放入 .exit.text段中
             该段中的代码执行一次对应的内存空间就释放
*/
static void __exit hellodriver_exit(void){
    printk("byebye!\n");
}
/*
  它是一个宏， 被其修饰的函数在安装内核模块时会被调用
  安装内核模块时被调用函数类型；int xxx(void)
 */
module_init(hellodriver_init);
/*
   它是一个宏， 被其修饰的函数在卸载内核模块时会被调用
   卸载内核模块时被调用的函数类型： void xxx(void)
 */
module_exit(hellodriver_exit);
/*将该模块声明为开源程序*/
MODULE_LICENSE("GPL");