#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

// TODO 改写设备树版本

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
/* file_operations 回调函数 — 被用户态系统调用触发 */


static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_release,
    .write = led_write,
    .read = led_read,
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