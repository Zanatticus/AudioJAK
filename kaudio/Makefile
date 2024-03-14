obj-m := kaudio.o
ccflags-y += -g -DDEBUG
ESL_KERNEL_VERSION ?= 5.10.0-esl
ESL_ZEDBOARD_IP ?= qemu
ifeq ($(OECORE_TARGET_SYSROOT),)
$(error "Can't determine SDK path. Verify that you have sourced the environment correctly.")
endif
KERNEL_SRC ?= $(OECORE_TARGET_SYSROOT)/lib/modules/$(ESL_KERNEL_VERSION)/build
ARCH ?= arm
CROSS_COMPILE ?= arm-linux-esl-gnueabi-
SRC := $(shell pwd)
UPLOAD_PATH ?= /home/root/

MODULE_OBJ:=$(obj-m:.o=.ko)

all: clean_all modules clean

modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(SRC)

modules_install:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(SRC) modules_install

modules_upload: modules
	scp $(MODULE_OBJ) root@$(ESL_ZEDBOARD_IP):$(UPLOAD_PATH)

clean:
	rm -f *.o *~ core .depend .*.cmd *.mod.c *.mod
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers

clean_all:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c *.mod
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers
