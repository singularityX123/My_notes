#include <linux/init.h>
#include <linux/module.h>

int export_add(int a, int b)
{
    return a + b;
}
EXPORT_SYMBOL(export_add);  // 导出符号export_add
// or
// EXPORT_SYMBOL_GPL(export_add);  // 导出符号export_add，且只能被GPL许可的模块使用

MODULE_LICENSE("GPL");