#if !defined(DHT11_CONSUMER_H)
#define DHT11_CONSUMER_H

/**
 *  declare the data struct of the dht11-sensor
 */

#define DHT11_SENSOR_PATH "/dev/dht11_sensor"
#define DHT11_BIT (8)
#define DHT11_ARRAY_LEN (5)
#define DHT11_ARRAY_SIZE (DHT11_BIT*DHT11_ARRAY_LEN)

enum DHT11_DATA_MEMBER {
    HUMI_HI,
    HUMI_LO,
    TEMP_HI,
    TEMP_LO,
    CHECK_BIT
};

typedef struct {
    unsigned char data[DHT11_ARRAY_LEN];
    int size;
} dht11_data_t;

#endif // DHT11_CONSUMER_H
