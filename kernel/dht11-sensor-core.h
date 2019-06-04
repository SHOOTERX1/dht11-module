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
	* ����������GPIO���ڲ��Ի�����
	* �����壺roc-rk3329-cc 	ϵͳ��rockchip-linux:stable-4.4-rk3399-linux
	* �У�������������GPIO����Ϊ��������ڲ����޸ĸߵ͵�ƽ�����⡣
	*/
    unsigned gpio;

#define DHT11_SENSOR_DATA_SIZE 3
	/*
	* ����Ϊ������������һ��һд��
	* ʵ��Ϊѭ�����У�����ѭ�����е����ԣ�ʵ����Ҫ3��������
	* 
	* ��ʼ�����ڶ���֮ǰ������Ϊ��ʼ��δ��ɡ�
	*/
	u8 sensor_data[DHT11_SENSOR_DATA_SIZE][5];
	int sensor_data_front;	// ͷָ�루дָ�룩
	int sensor_data_rear;		// βָ�루��ָ�룩

	spinlock_t sensor_core_lock;
	struct hrtimer sensor_hrtimer;			// ���ݶ�ȡʱ����ƶ�ʱ�����߾�������*��
	struct hrtimer schedule_timer;			// ���������¶�ʱ��
	int poll_time;							// ���β���֮���ʱ�������룩

	/*
	* ��ʶ��һ�����ݶ�ȡ�Ľ��
	* @DHT11_DATA_OK : �ɹ�
	* @DHT11_DATA_ERR : ����
	*/
	enum DHT11_DATA_STATE data_state;

	seqlock_t read_lock;	// ˳����
};

/* dht11���������ݷ��ؽṹ */
struct dht11_return_val {
	u8 data[5];
	/*
	* �Ƿ��ǹ�������
	���ݴ���ʧ�ܺ���data_stateָʾ
	*/
	int is_expried;
};

struct dht11_return_val* dht11_transfer_data(struct dht11_sensor_core *s);
int dht11_init_sensor_core(struct dht11_sensor_core *s, struct platform_device *pdev);
void dht11_release_sensor_core(struct dht11_sensor_core *s, struct platform_device *pdev);

#endif // DHT11_SENSOR_CORE_H