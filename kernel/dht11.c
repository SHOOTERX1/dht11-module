#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/gpio.h> // using for debug

#include "dht11-sensor-core.h"

struct dht11_sensor {
    struct miscdevice misc;
    struct mutex dht11_mutex;
    struct dht11_sensor_core core;
};

/* file operations */
static long dht11_ioctl(struct file *filp, unsigned int cmd,
                    unsigned long arg)
{
    /* gpio debug */
    struct dht11_sensor *d = container_of(filp->private_data, struct dht11_sensor, misc);
    switch (cmd)
    {
        case 0x01:
            gpio_set_value(d->core.dht11_gpio, 1);
            break;
        case 0x02:
            gpio_set_value(d->core.dht11_gpio, 0);
            break;
        case 0x03:
            gpio_direction_input(d->core.dht11_gpio);
            break;
        case 0x04:
            gpio_direction_output(d->core.dht11_gpio, 1);
        default:
            return -EINVAL;
    }
    return 0;
}

// static ssize_t dht11_read(struct file *filp, char __user *buf,
//                     size_t size, loff_t *ppos)
// {
//     return 0;
// }
static ssize_t dht11_read(struct file *filp, char __user *buf,
                    size_t size, loff_t *ppos)
{
    unsigned long p = *ppos;
    unsigned int count = size;
    ssize_t retval = 0;
    struct dht11_sensor *dhtp = container_of(filp->private_data,
                    struct dht11_sensor, misc);
    retval = dht11_transfer_data(&dhtp->core);
    if(retval < 0) {
            printk(KERN_WARNING "dht11_transfer_data failure. errcode: %d", (int)retval);
            // return retval;
    }

    if(p >= ARRAY_SIZE(dhtp->core.sensor_data))
        return 0;
    if(count > ARRAY_SIZE(dhtp->core.sensor_data) - p)
        count = ARRAY_SIZE(dhtp->core.sensor_data) - p;

    mutex_lock(&dhtp->dht11_mutex);
    if(copy_to_user(buf, dhtp->core.sensor_data, count)) {
        retval = -EFAULT;
        goto out;
    }
    *ppos += count;
    retval = count;
out:
    mutex_unlock(&dhtp->dht11_mutex);
    return retval;
}

static ssize_t dht11_write(struct file *filp, const char __user *buf,
                    size_t size, loff_t *ppos)
{
    return 0;
}

static const struct file_operations dht11_sensor_fops = {
    .owner = THIS_MODULE,
    .read = dht11_read,
    .write = dht11_write,
    .unlocked_ioctl = dht11_ioctl,
    .llseek = no_llseek,
};

/* platform probe & remove */
static int dht11_sensor_probe(struct platform_device *pdev)
{
    int ret;
    struct dht11_sensor *dhtp;
    /* alloc mem for struct dht11_sensor */
    dhtp = devm_kzalloc(&pdev->dev, sizeof(*dhtp), GFP_KERNEL);
    if(!dhtp) {
        printk(KERN_ERR "struct dht11_sensor kzalloc failure\n");
        return -ENOMEM;
    }
    /* init metex */
    mutex_init(&dhtp->dht11_mutex);
    /* save the dht11_sensor struct pointer */
    platform_set_drvdata(pdev, dhtp);
    /* fill the misc data */
    dhtp->misc.minor = MISC_DYNAMIC_MINOR;
    dhtp->misc.name = "dht11_sensor";
    dhtp->misc.fops = &dht11_sensor_fops;
    /* register the miscdevice */
    ret = misc_register(&dhtp->misc);
    if(ret < 0) {
        printk(KERN_ERR "misc register failure\n");
        goto misc_err;
    }
    /* init dht11 sensor core */
    if(dht11_init_sensor_core(&dhtp->core, pdev)) {
        printk(KERN_ERR "core init failure\n");
        goto core_init_err;
    }
    /* ok! */
    return 0;
core_init_err:
    misc_deregister(&dhtp->misc);
misc_err:
    return ret;
}

static int dht11_sensor_remove(struct platform_device *pdev)
{
    struct dht11_sensor *dhtp = platform_get_drvdata(pdev);
    misc_deregister(&dhtp->misc);
    dht11_release_sensor_core(&dhtp->core, pdev);
    return 0;
}

static struct platform_driver dht11_sensor_driver = {
    .driver = {
        .name = "dht11_sensor",
        .owner = THIS_MODULE,
    },
    .probe = dht11_sensor_probe,
    .remove = dht11_sensor_remove,
};
module_platform_driver(dht11_sensor_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("YoRHa.A2 <yorha.a2@foxmail.com>");