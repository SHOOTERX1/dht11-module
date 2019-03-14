# build script
KVERS = $(shell uname -r)

# Kernel modules
obj-m += dht-sensor.o

dht-sensor-objs := dht11.o dht11-sensor-core.o

# Specify flags for module compilation
#EXTRA_CFLAGS=-g -O0

build: kernel_modules

kernel_modules:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules

modules_install:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules_install

clean:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean
