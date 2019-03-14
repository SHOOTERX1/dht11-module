#if !defined(DHT11_SENSOR_CORE_H)
#define DHT11_SENSOR_CORE_H

#include <linux/gpio/consumer.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>

enum DHT11_DATA_MEMBER {
    HUMI_HI,
    HUMI_LO,
    TEMP_HI,
    TEMP_LO,
    CHECK_BIT
};

struct dht11_sensor_core {
    unsigned dht11_gpio;
    u8 sensor_data[5];
    bool data_ready;
    spinlock_t dht11_sensor_core_lock;
    struct hrtimer dht11_hrtimer;
};

int dht11_transfer_data(struct dht11_sensor_core *s);
int dht11_init_sensor_core(struct dht11_sensor_core *s, struct platform_device *pdev);
void dht11_release_sensor_core(struct dht11_sensor_core *s, struct platform_device *pdev);

#endif // DHT11_SENSOR_CORE_H