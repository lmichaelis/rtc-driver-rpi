ifneq ($(KERNELVERSION),)
obj-m	:= ds3231.o
ds3231-objs :=  ds3231_drv.o ds3231_hw.o ds3231_io.o


else
KDIR	:= /lib/modules/$(shell uname -r)/build
PWD	:= $(shell pwd)

default: ds3231

ds3231:
	@echo "Building module ..."
	@(cd $(KDIR) && make -C $(KDIR) SUBDIRS=$(PWD) modules)

clean:
	-rm -f *.o *.ko .*.cmd .*.flags *.mod.c Module.symvers modules.order
	-rm -rf .tmp_versions
	-rm -rf *~

reload: ds3231
	-sudo rmmod ds3231
	sudo insmod ds3231.ko

test: reload
	cat /dev/ds3231

endif
