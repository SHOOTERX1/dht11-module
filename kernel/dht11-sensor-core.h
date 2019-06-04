#if !defined(DHT11_SENSOR_CORE_H)
#define DHT11_SENSOR_CORE_H

#include <linux/gpio/consumer.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/seqlock.h>

enum DHT11_DATA_MEMBER {
    DHT11_HUMI_HI = 0,
    DHT11_HUMI_LO,
    DHT11_TEMP_HI,
    DHT11_TEMP_LO,
    DHT11_CHECK_BIT
};

enum DHT11_DATA_STATE {
	DHT11_DATA_OK,
	DHT11_DATA_ERR
};

struct dht11_sensor_core {
	/*
	* 基于整数的GPIO，在测试环境：
	* 开发板：roc-rk3329-cc 	系统：rockchip-linux:stable-4.4-rk3399-linux
	* 中，基于描述符的GPIO设置为输出，存在不能修改高低电平的问题。
	*/
    unsigned gpio;

#define DHT11_SENSOR_DATA_SIZE 3
	/*
	* 划分为两个数据区，一读一写。
	* 实现为循环队列，由于循环队列的特性，实际需要3个数据域
	* 
	* 初始化：在队满之前可以视为初始化未完成。
	*/
	u8 sensor_data[DHT11_SENSOR_DATA_SIZE][5];
	int sensor_data_front;	// 头指针（写指针）
	int sensor_data_rear;		// 尾指针（读指针）

	spinlock_t sensor_core_lock;
	struct hrtimer sensor_hrtimer;			// 数据读取时序控制定时器（高精度需求*）
	struct hrtimer schedule_timer;			// 数据区更新定时器
	int poll_time;							// 两次测量之间的时间间隔（秒）

	/*
	* 标识上一次数据读取的结果
	* @DHT11_DATA_OK : 成功
	* @DHT11_DATA_ERR : 出错
	*/
	enum DHT11_DATA_STATE data_state;

	seqlock_t read_lock;	// 顺序锁
};

/* dht11传感器数据返回结构 */
struct dht11_return_val {
	u8 data[5];
	/*
	* 是否是过期数据
	数据传输失败后，由data_state指示
	*/
	int is_expried;
};

struct dht11_return_val* dht11_transfer_data(struct dht11_sensor_core *s);
int dht11_init_sensor_core(struct dht11_sensor_core *s, struct platform_device *pdev);
void dht11_release_sensor_core(struct dht11_sensor_core *s, struct platform_device *pdev);

#endif // DHT11_SENSOR_CORE_H