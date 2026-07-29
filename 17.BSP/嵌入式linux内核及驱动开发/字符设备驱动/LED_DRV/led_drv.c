#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

#define LD1 74 //PE10
#define LD2 90 //PF10
#define LD3 72 //PE8

#define LED_NUM 3
static const int led_pins[LED_NUM] = { LD1, LD2, LD3 };

/* 定时器：实现 open 时流水灯 */
static struct timer_list led_timer;
static int led_idx;
/* file_operations 回调函数 — 被用户态系统调用触发 */
static int led_open(struct inode *inode, struct file *filp)
{   
    printk("enter %s\n", __func__);
    printk("led_open - start chaser\n");

    led_idx = 0;
    gpio_set_value(led_pins[0], 1);
    mod_timer(&led_timer, jiffies + HZ / 3);  // 0.33 秒后开始流水
    return 0;
}
static int led_release(struct inode *inode, struct file *filp)
{
    int i;

    printk("enter %s\n", __func__);
    printk("led_release - stop chaser\n");

    del_timer(&led_timer);          // 停止定时器

    for (i = 0; i < LED_NUM; i++)
        gpio_set_value(led_pins[i], 1);  // 全部恢复常亮
    return 0;
}
static ssize_t led_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    char cmd[8] = {0};
    int i;

    printk("enter %s\n", __func__);

    if (count > sizeof(cmd) - 1)
        count = sizeof(cmd) - 1;

    if (copy_from_user(cmd, buf, count))
        return -EFAULT;

    printk("led_write: cmd=%s\n", cmd);

    del_timer(&led_timer);

    if (strncmp(cmd, "on", 2) == 0) {
        /* 只亮 LD2 */
        gpio_set_value(LD1, 0);
        gpio_set_value(LD2, 1);
        gpio_set_value(LD3, 0);
    } else if (strncmp(cmd, "off", 3) == 0) {
        /* 全灭 */
        for (i = 0; i < LED_NUM; i++)
            gpio_set_value(led_pins[i], 0);
    }

    return count;
}
static ssize_t led_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{   
    char status[16];
    int len;

    printk("enter %s\n", __func__);
    printk("led_read\n");

    del_timer(&led_timer);

    /* 返回当前 GPIO 74/90/72 的电平状态 */
    len = snprintf(status, sizeof(status), "%d %d %d\n",
                   gpio_get_value(LD1),
                   gpio_get_value(LD2),
                   gpio_get_value(LD3));

    if (copy_to_user(buf, status, min((size_t)len, count)))
        return -EFAULT;

    return min((size_t)len, count);
}

/* ========== ioctl 命令定义 ========== */
#define LED_IOC_MAGIC  'L'

#define LED_IOC_ALL_ON   _IO(LED_IOC_MAGIC, 0)   /* 全部亮 */
#define LED_IOC_ALL_OFF  _IO(LED_IOC_MAGIC, 1)   /* 全部灭 */
#define LED_IOC_SET      _IOW(LED_IOC_MAGIC, 2, int) /* 只亮第 arg 个 (0~2) */
#define LED_IOC_GET      _IOR(LED_IOC_MAGIC, 3, int) /* 读取状态 bitmap */
#define LED_IOC_TOGGLE   _IO(LED_IOC_MAGIC, 4)   /* 全部翻转 */

static long led_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int i, val;
    int nr;

    printk("enter %s\n", __func__);

    /* 检查幻数 */
    if (_IOC_TYPE(cmd) != LED_IOC_MAGIC)
        return -ENOTTY;

    del_timer(&led_timer);   /* 执行 ioctl 时暂停流水灯 */

    switch (cmd) {
    case LED_IOC_ALL_ON:
        printk("led_ioctl: ALL ON\n");
        for (i = 0; i < LED_NUM; i++)
            gpio_set_value(led_pins[i], 1);
        break;

    case LED_IOC_ALL_OFF:
        printk("led_ioctl: ALL OFF\n");
        for (i = 0; i < LED_NUM; i++)
            gpio_set_value(led_pins[i], 0);
        break;

    case LED_IOC_SET:
        /* 用户传一个 int，表示要点亮的 LED 序号 (0/1/2) */
        if (copy_from_user(&nr, (int __user *)arg, sizeof(nr)))
            return -EFAULT;
        if (nr < 0 || nr >= LED_NUM)
            return -EINVAL;
        printk("led_ioctl: SET LED %d\n", nr);
        for (i = 0; i < LED_NUM; i++)
            gpio_set_value(led_pins[i], (i == nr) ? 1 : 0);
        break;

    case LED_IOC_GET:
        /* 返回 bitmap: bit0=LD1, bit1=LD2, bit2=LD3 */
        val = 0;
        for (i = 0; i < LED_NUM; i++) {
            if (gpio_get_value(led_pins[i]))
                val |= (1 << i);
        }
        if (copy_to_user((int __user *)arg, &val, sizeof(val)))
            return -EFAULT;
        printk("led_ioctl: GET status=0x%x\n", val);
        break;

    case LED_IOC_TOGGLE:
        printk("led_ioctl: TOGGLE ALL\n");
        for (i = 0; i < LED_NUM; i++)
            gpio_set_value(led_pins[i], !gpio_get_value(led_pins[i]));
        break;

    default:
        return -ENOTTY;   /* 不支持的命令 */
    }

    return 0;
}

/* file_operations 回调函数 — 被用户态系统调用触发 */


static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_release,
    .write = led_write,
    .read = led_read,
    .unlocked_ioctl = led_ioctl, // 可以实现 ioctl 控制
    //...其他文件操作函数指针
};
static dev_t dev; //设备号
static struct class *led_class;
static struct device *led_device;

/*1.1 定义一个cdev变量*/
static struct cdev *led_cdev;

static void led_chaser_timer(struct timer_list *t)
{
    int i;

    /* 全部熄灭 */
    for (i = 0; i < LED_NUM; i++)
        gpio_set_value(led_pins[i], 0);

    /* 点亮当前那一个 */
    gpio_set_value(led_pins[led_idx], 1);

    led_idx = (led_idx + 1) % LED_NUM;
    mod_timer(&led_timer, jiffies + HZ / 3);  // ~0.33 秒切换一次
}

int __init led_drv_init(void)
{
    int ret;

    /*1.2 动态分配设备号*/
    ret = alloc_chrdev_region(&dev, 0, 1, "leds");
    if (ret < 0) {
        printk("alloc_chrdev_region failed: %d\n", ret);
        return ret;
    }
    printk("alloc_chrdev_region: major=%d minor=%d\n", MAJOR(dev), MINOR(dev));

    /*2. 分配cdev变量*/
    led_cdev = cdev_alloc();
    if (!led_cdev) {
        printk("cdev_alloc failed\n");
        unregister_chrdev_region(dev, 1);
        return -ENOMEM;
    }
    cdev_init(led_cdev, &led_fops);

    /*3. 注册cdev变量*/
    ret = cdev_add(led_cdev, dev, 1);
    if (ret < 0) {
        printk("cdev_add failed: %d\n", ret);
        cdev_del(led_cdev);
        unregister_chrdev_region(dev, 1);
        return ret;
    }
/////////////////////////////////////////////////
    led_class = class_create(THIS_MODULE, "led_class");/*创建 class（/sys/class/led_class/*/
    if (IS_ERR(led_class)) {
        ret = PTR_ERR(led_class);
        printk("class_create failed: %d\n", ret);
        cdev_del(led_cdev);
        unregister_chrdev_region(dev, 1);
        return ret;
    }

    led_device = device_create(led_class, NULL, dev, NULL, "leds");/* 创建设备节点（udev/mdev 自动在 /dev/ 下生成 leds）*/
    if (IS_ERR(led_device)) {
        ret = PTR_ERR(led_device);
        printk("device_create failed: %d\n", ret);
        class_destroy(led_class);
        cdev_del(led_cdev);
        unregister_chrdev_region(dev, 1);
        return ret;
    }
/////////////////////////////////////////////////
    printk("led_drv loaded successfully\n");

    gpio_request(LD1, "led_gpio_PE10"); //申请GPIO
    gpio_direction_output(LD1, 1); //设置为输出

    gpio_request(LD2, "led_gpio_PF10");
    gpio_direction_output(LD2, 1);

    gpio_request(LD3, "led_gpio_PE8");
    gpio_direction_output(LD3, 1);

    /* 初始化定时器，但先不启动（等 open） */
    timer_setup(&led_timer, led_chaser_timer, 0);

    return 0;
}

void __exit led_drv_exit(void)
{
    int i;

    del_timer(&led_timer);          /* 确保定时器已停止 */

    for (i = 0; i < LED_NUM; i++)
        gpio_set_value(led_pins[i], 0);  /* 全部关灯 */

    gpio_free(LD1);
    gpio_free(LD2);
    gpio_free(LD3);

    device_destroy(led_class, dev);
    class_destroy(led_class); /*销毁设备节点和 class*/

    /*4. 注销cdev变量*/
    cdev_del(led_cdev);

    unregister_chrdev_region(dev, 1); //注销设备号
}

module_init(led_drv_init);
module_exit(led_drv_exit);


#if 0
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
#endif