ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
obj-m := kirkland-rgb.o

else
# normal makefile

KDIR ?= /home/grant/Desktop/linux-socfpga

default:
	$(MAKE) -C $(KDIR) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- M=$$PWD

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
endif
