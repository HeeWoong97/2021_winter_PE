obj-m	:= h.o
h-objs	:= vector_arr.o calclock.o tree.o utils.o lockfree_utils.o test_parallel.o 

KERNEL_DIR	:= /lib/modules/$(shell uname -r)/build
PWD			:= $(shell pwd)

default:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	rm h.* *.o
