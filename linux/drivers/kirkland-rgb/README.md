# kirkland-rgb

These files are for the driver for the rgb led controller, allowing the registers to be accessed from linux.

## Building

Building this driver can be done using the included Makefile, which specifies the needed ARCH=arm and CROSS_COMPILE=arm-linux-gnueabihf- values:

```
sudo make
```

Additionally, both the c and makefile need to be moved outside of the shared folder to be successfully built due to file system issues. Once built, mode *.ko file to device.

## Files

### kirkland-rgb.c

Main driver file

### kirkland-rgb.ko
Compiled driver module for ARM. Can be loaded using the command

```command
sudo insmod kirkland-rgb.ko
```

Removing the module can be done using
```
sudo rmmod kirkland_rgb
```


### Makefile
Makefile to compile the driver
