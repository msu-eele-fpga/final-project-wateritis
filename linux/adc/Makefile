ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
obj-m  := de10nano_adc.o

else
# normal makefile

# path to kernel directory
KDIR ?= ../../../../linux-socfpga

default:
	$(MAKE) -C $(KDIR) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- M=$$PWD

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
endif
