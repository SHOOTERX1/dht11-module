#include "dht11-consumer.h"
#include <unistd.h>
// #include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>

int main(int argc, char const *argv[])
{
    int fd;
	dht11_data_t d;

    fd = open(DHT11_SENSOR_PATH, O_RDONLY);
	for (int i = 0; i < 5; i++)
		d.data[i] = 0;
	d.is_expried = -1;

    if(read(fd, d.data, DHT11_ARRAY_SIZE) < 0) {
        printf("data get failure\n");
    }
    close(fd);

	printf("is_expried %d\n", d.is_expried);

    printf("current humidity %d.%.2d\n", d.data[HUMI_HI], d.data[HUMI_LO]);
    printf("current temp %d.%.2d\n", d.data[TEMP_HI], d.data[TEMP_LO]);
    printf("check bit %d\n", d.data[CHECK_BIT]);

    int i,j;
    for(i=0;i<5;i++) {
        printf("print bits:\t");
        for(j=7; j>=0;j--) {
            printf("%d",(d.data[i] >> j) & 1);
        }
        printf("\n");
    }

    return 0;
}