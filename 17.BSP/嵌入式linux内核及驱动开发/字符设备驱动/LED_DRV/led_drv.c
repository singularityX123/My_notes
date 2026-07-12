#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");

/* 静态分配设备号前先查看当前系统中是否有设备号被占用，可以使用命令：
 * cat /proc/devices
 * ls -l /dev
 * 如果设备号被占用，可以选择其他未被占用的设备号。
 */
static unsigned int led_major = 200; //主设备号（静态分配） 自己实验玩的，非社区标准分配
static unsigned int led_minor = 0;
static dev_t dev;
static struct cdev led_cdev;

/* 文件操作 */
static int led_open(struct inode *inode, struct file *filp)
{
    printk("led_open\n");
    return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
    printk("led_release\n");
    return 0;
}

static struct file_operations led_fops = {
    .owner   = THIS_MODULE,
    .open    = led_open,
    .release = led_release,
};

static int __init led_drv_init(void)
{
    int ret;

    /* 1. 静态注册设备号 */
    dev = MKDEV(led_major, led_minor);
    ret = register_chrdev_region(dev, 1, "leds");
    if (ret < 0) {
        printk("Failed to register device number\n");
        return ret;
    }

    /* 2. 初始化并添加 cdev */
    cdev_init(&led_cdev, &led_fops);
    led_cdev.owner = THIS_MODULE;
    ret = cdev_add(&led_cdev, dev, 1);
    if (ret < 0) {
        printk("Failed to add cdev\n");
        unregister_chrdev_region(dev, 1);
        return ret;
    }

    printk("led_drv_init: major=%d minor=%d\n", led_major, led_minor);
    return 0;
}

static void __exit led_drv_exit(void)
{
    cdev_del(&led_cdev);
    unregister_chrdev_region(dev, 1);
    printk("led_drv_exit\n");
}

module_init(led_drv_init);
module_exit(led_drv_exit);