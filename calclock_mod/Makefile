obj-m		:= calclock.o

KERNEL_DIR	:= /lib/module/$(shell uname -r)/build
PWD			:= $(shell pwd)

default:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
