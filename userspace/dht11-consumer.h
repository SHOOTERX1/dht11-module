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

/* dht11传感器数据返回结构 */
typedef struct {
	unsigned char data[5];
	/*
	* 是否是过期数据
	* 数据传输失败后，由data_state指示
	*/
	int is_expried;
} dht11_data_t;

#endif // DHT11_CONSUMER_H
