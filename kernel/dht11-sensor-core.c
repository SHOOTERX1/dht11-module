/**
 * The core of the dht11 sensor data transfer porgress
 */
#include <linux/gpio/consumer.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

#include "dht11-sensor-core.h"

#define DHT11_RECV_HIGH 1
#define DHT11_RECV_LOW 0

static bool dht11_check_result(struct dht11_sensor_core *s)
{
    return (s->sensor_data[HUMI_HI] + s->sensor_data[HUMI_LO] +
        s->sensor_data[TEMP_HI] + s->sensor_data[TEMP_LO] ==
        s->sensor_data[CHECK_BIT]) ? true : false;
}

static enum hrtimer_restart dht11_hrtimer_callback(struct hrtimer *hrt)
{
    struct dht11_sensor_core *s = container_of(hrt,
                struct dht11_sensor_core, dht11_hrtimer);
    int i,j; // for loop
    unsigned long flags; // irq flags

    /* transfer start */
    /*### Use hrtimer instead ###*/
    /* disable irq */
    // spin_lock_irqsave(&s->dht11_sensor_core_lock, flags);
    // mdelay(19);
    // /* enable irq */
    // spin_unlock_irqrestore(&s->dht11_sensor_core_lock, flags);

    gpio_direction_input(s->dht11_gpio);
    udelay(10);
    while(gpio_get_value(s->dht11_gpio) == 0);
    udelay(90);
    /* data come in */
    spin_lock_irqsave(&s->dht11_sensor_core_lock, flags);
    /* fill data */
    for(i=0; i<5; i++) {
        for(j=0; j<8; j++) {
            while (gpio_get_value(s->dht11_gpio) == 0);
            udelay(40);
            if (gpio_get_value(s->dht11_gpio) == 1) {
                s->sensor_data[i] =
                    (s->sensor_data[i] << 1) + DHT11_RECV_HIGH;
            } else {
                s->sensor_data[i] =
                    (s->sensor_data[i] << 1) + DHT11_RECV_LOW;
            }
            while(gpio_get_value(s->dht11_gpio) == 1);
        }
    }
    spin_unlock_irqrestore(&s->dht11_sensor_core_lock, flags);
    /* data receive end */
    /* reset gpio direction */
    gpio_direction_output(s->dht11_gpio, 1);
    /* transfer end */
    s->data_ready = dht11_check_result(s);
    return HRTIMER_NORESTART;
}

int dht11_init_sensor_core(struct dht11_sensor_core *s, struct platform_device *pdev)
{
    /* init gpio */
    struct gpio_desc *g= 
            devm_gpiod_get(&pdev->dev, "dht11-sensor", GPIOD_OUT_HIGH);
    if(!g)
        return -ENODEV;
    s->dht11_gpio = desc_to_gpio(g);

    /* init spinlock */
    spin_lock_init(&s->dht11_sensor_core_lock);

    /* init hrtimer */
    hrtimer_init(&s->dht11_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    s->dht11_hrtimer.function = dht11_hrtimer_callback;
    return 0;
}
/**
 * The core progress
 * Return: 0 -- OK
 *      -EAGAIN -- result check failure
 */
void dht11_release_sensor_core(struct dht11_sensor_core *s, struct platform_device *pdev)
{
    struct gpio_desc *g = gpio_to_desc(s->dht11_gpio);
    devm_gpiod_put(&pdev->dev, g);
}

int dht11_transfer_data(struct dht11_sensor_core *s)
{
    /* ready to receive data */
    gpio_direction_output(s->dht11_gpio, 0);
    hrtimer_start(&s->dht11_hrtimer, ms_to_ktime(20), HRTIMER_MODE_REL);
    return  s->data_ready? : -EAGAIN;
}