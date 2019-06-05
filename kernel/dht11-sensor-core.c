/*
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
#include <linux/string.h>
#include <linux/slab.h>

#include "dht11-sensor-core.h"

#define DHT11_RECV_HIGH 1
#define DHT11_RECV_LOW 0

static inline enum DHT11_DATA_STATE dht11_check_result(struct dht11_sensor_core *s, int whitch)
{
	return 
		s->sensor_data[whitch][DHT11_HUMI_HI] +
		s->sensor_data[whitch][DHT11_HUMI_LO] +
		s->sensor_data[whitch][DHT11_TEMP_HI] +
		s->sensor_data[whitch][DHT11_TEMP_LO] ==
		s->sensor_data[whitch][DHT11_CHECK_BIT] ? DHT11_DATA_OK : DHT11_DATA_ERR;
}

static enum hrtimer_restart dht11_hrtimer_callback(struct hrtimer *hrt)
{
    struct dht11_sensor_core *s = container_of(hrt,
                struct dht11_sensor_core, sensor_hrtimer);
    int i,j; // for loop
	int which;	// circular queue pointer
    unsigned long flags; // irq flags

	which = (s->sensor_data_front + 1) % DHT11_SENSOR_DATA_SIZE;

    gpio_direction_input(s->gpio);
    udelay(10);
    while(gpio_get_value(s->gpio) == 0);
    udelay(90);

    spin_lock_irqsave(&s->sensor_core_lock, flags);
    /* fill data */
    for(i=0; i<5; i++) {
        for(j=0; j<8; j++) {
            while (gpio_get_value(s->gpio) == 0);
            udelay(40);
            if (gpio_get_value(s->gpio) == 1) {
                s->sensor_data[which][i] =
                    (s->sensor_data[which][i] << 1) + DHT11_RECV_HIGH;
            } else {
                s->sensor_data[which][i] =
                    (s->sensor_data[which][i] << 1) + DHT11_RECV_LOW;
            }
            while(gpio_get_value(s->gpio) == 1);
        }
    }
    spin_unlock_irqrestore(&s->sensor_core_lock, flags);

    /* reset gpio direction */
    gpio_direction_output(s->gpio, 1);
	/* check the result */
	s->data_state = dht11_check_result(s, which);
	if (s->data_state == DHT11_DATA_OK) {
		// seqlock
		write_seqlock(&s->read_lock);
		s->sensor_data_front = which;
		if ((s->sensor_data_front + 1) % DHT11_SENSOR_DATA_SIZE == s->sensor_data_rear)
			s->sensor_data_rear = (s->sensor_data_rear + 1) % DHT11_SENSOR_DATA_SIZE;
		write_sequnlock(&s->read_lock);
	}

    return HRTIMER_NORESTART;
}

static enum hrtimer_restart dht11_schedule_timer_callback(struct hrtimer* hrt)
{
	struct dht11_sensor_core* s = container_of(hrt,
		struct dht11_sensor_core, schedule_timer);
	/* ready to receive data */
	gpio_direction_output(s->gpio, 0);
	hrtimer_start(&s->sensor_hrtimer, ms_to_ktime(20), HRTIMER_MODE_REL);
	hrtimer_forward_now(hrt, ms_to_ktime(s->poll_time*1000));

	return HRTIMER_RESTART;
}

int dht11_init_sensor_core(struct dht11_sensor_core *s, struct platform_device *pdev)
{
    /* init gpio */
    struct gpio_desc *g= 
            devm_gpiod_get(&pdev->dev, "dht11-sensor", GPIOD_OUT_HIGH);
    if(!g)
        return -ENODEV;
    s->gpio = desc_to_gpio(g);

	s->sensor_data_front = s->sensor_data_rear = 0;
	s->data_state = DHT11_DATA_ERR;
	s->poll_time = 15;

    /* init spinlock */
    spin_lock_init(&s->sensor_core_lock);

	/* init seqlock */
	seqlock_init(&s->read_lock);

    /* init hrtimer */
	hrtimer_init(&s->sensor_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	s->sensor_hrtimer.function = dht11_hrtimer_callback;
	hrtimer_init(&s->schedule_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	s->schedule_timer.function = dht11_schedule_timer_callback;

	hrtimer_start(&s->schedule_timer, ms_to_ktime(100), HRTIMER_MODE_REL);

    return 0;
}

void dht11_release_sensor_core(struct dht11_sensor_core *s, struct platform_device *pdev)
{
    struct gpio_desc *g = gpio_to_desc(s->gpio);
	while (hrtimer_try_to_cancel(&s->schedule_timer) == -1);
    devm_gpiod_put(&pdev->dev, g);
}

/*
dht11_transfer_data:
	@s: pointer to strcut dht11_sensor_core
	Returns: the sensor data structure  --remember to free the retval!
*/
 struct dht11_return_val* dht11_transfer_data(struct dht11_sensor_core *s)
{
	 struct dht11_return_val* ret;
	 unsigned int seqcount;
	 ret = kzalloc(sizeof(*ret), GFP_KERNEL);
	 do {
		 seqcount = read_seqbegin(&s->read_lock);
		 if ((s->sensor_data_front + 2) % DHT11_SENSOR_DATA_SIZE != s->sensor_data_rear)
			 goto out; // initializing...
		 //  copy
		 memmove(ret->data, s->sensor_data[s->sensor_data_rear], sizeof(u8) * 5);
	 } while (read_seqretry(&s->read_lock,  seqcount));

	if (s->data_state == DHT11_DATA_OK)
		ret->is_expried = 0;
	else
		ret->is_expried = (!0);
	return ret;

out:
	kfree(ret);
	return NULL;
}