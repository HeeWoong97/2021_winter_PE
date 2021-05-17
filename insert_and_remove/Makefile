obj-m	:= h.o
h-objs	:= vector_arr.o calclock.o tree.o utils.o lockfree_utils.o insert_and_remove.o 

KERNEL_DIR	:= /lib/modules/$(shell uname -r)/build
PWD			:= $(shell pwd)

default:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
