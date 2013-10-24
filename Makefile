KERNEL_DIR = /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

obj-m += morse_io.o

default:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules

clean:
	@rm -rf *.ko
	@rm -rf *.mod.*
	@rm -rf *.cmd
	@rm -rf *.o
	@rm -rf *.order
	@rm -rf *.symvers
	@rm -rf *.unsigned
