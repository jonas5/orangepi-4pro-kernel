KDIR ?= /srv/kernel/orangepi-build/kernel/linux-6.18.y
PWD := $(shell pwd)

default:
	$(MAKE) ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- -C $(KDIR) M=$(PWD) modules CONFIG_AW_NNA_VIP=m

clean:
	$(MAKE) ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- -C $(KDIR) M=$(PWD) clean
