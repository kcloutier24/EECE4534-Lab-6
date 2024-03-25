obj-m := echo.o
ccflags-y += -g -DDEBUG

# conditional if not on COE systems 
#  - check if $HOST looks identical after replacing coe.neu.edu with nothing
ifeq ($(OECORE_TARGET_SYSROOT),)
$(error "Can't determine SDK path. Verify that you have sourced the environment correctly.")
endif
ESL_KERNEL_VERSION ?= 5.10.0-esl
KERNEL_SRC ?= $(OECORE_TARGET_SYSROOT)/lib/modules/$(ESL_KERNEL_VERSION)/build
CROSS_COMPILE ?= arm-linux-esl-gnueabi-
ESL_ZEDBOARD_IP?=qemu
ARCH ?= arm
SRC := $(shell pwd)
UPLOAD_PATH ?= /home/root/

MODULE_OBJ:=$(obj-m:.o=.ko)

all: modules

modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(SRC)

modules_install:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(SRC) modules_install

modules_upload: modules
	scp $(MODULE_OBJ) root@$(ESL_ZEDBOARD_IP):$(UPLOAD_PATH)

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers
