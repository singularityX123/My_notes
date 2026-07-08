#include <linux/init.h>
#include <linux/module.h>

static int __init hellodriver_init(void) // GNU C
{
    printk("Hello, driver!\n");
    return 0;
}

static void __exit hellodriver_exit(void)
{
    printk("Goodbye, driver!\n");
}

module_init(hellodriver_init);
module_exit(hellodriver_exit);
MODULE_LICENSE("GPL");