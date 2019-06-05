#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
//#include <linux/gpio.h> // using for debug

#include "dht11-sensor-core.h"

//#define DEBUG

// --TODO-- : 考虑open()的需求

struct dht11_sensor {
	struct miscdevice misc;
	struct mutex dht11_mutex;
	struct dht11_sensor_core core;
};

/* file operations */
static long dht11_ioctl(struct file* filp, unsigned int cmd,
	unsigned long arg)
{
	// --TODO--: 传感器的io控制（重置、定时器控制）
	return 0;
}

static ssize_t dht11_read(struct file* filp, char __user* buf,
	size_t size, loff_t* ppos)
{
	unsigned long p = *ppos;
	unsigned int count = size;
	ssize_t retval = 0;
	struct dht11_sensor* dhtp = container_of(filp->private_data,
		struct dht11_sensor, misc);

	struct dht11_return_val* dht11_ret = NULL;
	unsigned long missing;

#ifdef DEBUG
	printk(KERN_NOTICE "dht11_read called\n");
#endif // DEBUG


	dht11_ret = dht11_transfer_data(&dhtp->core);
	if (!dht11_ret) {
		printk(KERN_NOTICE "dht11_ret:NULL, data not ready, dht11 is initializing.\n");
		return 0;
	}

	if (p >= sizeof(struct dht11_return_val))
		return 0;
	if (count > sizeof(struct dht11_return_val) - p)
		count = sizeof(struct dht11_return_val) - p;

	mutex_lock(&dhtp->dht11_mutex);
	missing = copy_to_user(buf, dht11_ret, count);
	*ppos += count;
	retval = count;

	mutex_unlock(&dhtp->dht11_mutex);
	kfree(dht11_ret);
	return retval;
}

static ssize_t dht11_write(struct file* filp, const char __user* buf,
	size_t size, loff_t* ppos)
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
static int dht11_sensor_probe(struct platform_device* pdev)
{
	int ret;
	struct dht11_sensor* dhtp;
	/* alloc mem for struct dht11_sensor */
	dhtp = devm_kzalloc(&pdev->dev, sizeof(*dhtp), GFP_KERNEL);
	if (!dhtp) {
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
	if (ret < 0) {
		printk(KERN_ERR "misc register failure\n");
		goto misc_err;
	}
	/* init dht11 sensor core */
	if (dht11_init_sensor_core(&dhtp->core, pdev)) {
		printk(KERN_ERR "core init failure\n");
		goto core_init_err;
	}
	/* ok! */

#ifdef DEBUG
	printk(KERN_INFO "dht11_bc in!\n");
#endif // DEBUG

	return 0;
core_init_err:
	misc_deregister(&dhtp->misc);
misc_err:
	return ret;
}

static int dht11_sensor_remove(struct platform_device* pdev)
{
	struct dht11_sensor* dhtp = platform_get_drvdata(pdev);
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