obj-m := hcsr04.o

SRC := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC)

modules_install:
	$(MAKE) INSTALL_MOD_DIR=kernel/drivers/range-sensors -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.C
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers