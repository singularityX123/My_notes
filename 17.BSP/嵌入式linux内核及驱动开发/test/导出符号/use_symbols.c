#include <linux/init.h>
#include <linux/module.h>

extern int export_add(int a, int b);  // 声明导入符号export_add

static int __init export_symbols_init(void)
{
    printk(KERN_INFO "Export symbols module loaded.\n");

    printk(KERN_INFO "Calling export_add(3, 4): %d\n", export_add(3, 4));

    return 0;
}

static void __exit export_symbols_exit(void)
{
    printk(KERN_INFO "Export symbols module unloaded.\n");
}

module_init(export_symbols_init);
module_exit(export_symbols_exit);

MODULE_LICENSE("GPL");