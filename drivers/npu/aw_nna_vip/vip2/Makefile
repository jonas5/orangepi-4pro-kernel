KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules CONFIG_AW_NNA_VIP=m

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
