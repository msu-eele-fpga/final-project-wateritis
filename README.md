
```diff
@@	▓▓▓  ▓▓▓  ▓▓▓   ▓▓▓▓▓▓   ▓▓▓▓▓▓▓  ▓▓▓▓▓▓▓▓  ▓▓▓▓▓▓▓   ▓▓▓  ▓▓▓▓▓▓▓  ▓▓▓   ▓▓▓▓▓▓   	@@
@@	▓▓▓  ▓▓▓  ▓▓▓  ▓▓▓▓▓▓▓▓  ▓▓▓▓▓▓▓  ▓▓▓▓▓▓▓▓  ▓▓▓▓▓▓▓▓  ▓▓▓  ▓▓▓▓▓▓▓  ▓▓▓  ▓▓▓▓▓▓▓   	@@
@@	▓▓▒  ▓▓▒  ▓▓▒  ▓▓▒  ▓▓▓    ▓▓▒    ▓▓▒       ▓▓▒  ▓▓▓  ▓▓▒    ▓▓▒    ▓▓▒  ▒▓▓       	@@
@@	▒▓▒  ▒▓▒  ▒▓▒  ▒▓▒  ▓▒▓    ▒▓▒    ▒▓▒       ▒▓▒  ▓▒▓  ▒▓▒    ▒▓▒    ▒▓▒  ▒▓▒       	@@
@@	▓▒▒  ▒▒▓  ▓▒▓  ▓▒▓▒▓▒▓▒    ▓▒▒    ▓▒▒▒░▒    ▓▒▓▒▒▓▒   ▒▒▓    ▓▒▒    ▒▒▓  ▒▒▓▓▒▒    	@@
@@	▒▓▒  ▒▒▒  ▒▓▒  ▒▒▒▓▒▒▒▒    ▒▒▒    ▒▒▒▒▒░    ▒▒▓▒▓▒    ▒▒▒    ▒▒▒    ▒▒▒   ▒▒▓▒▒▒   	@@
@@	▒▒░  ▒▒░  ▒▒░  ▒▒░  ▒▒▒    ▒▒░    ▒▒░       ▒▒░ ░▒▒   ▒▒░    ▒▒░    ▒▒░       ▒░▒  	@@
@@	░▒░  ░▒░  ░▒░  ░▒░  ▒░▒    ░▒░    ░▒░       ░▒░  ▒░▒  ░▒░    ░▒░    ░▒░      ▒░▒   	@@
@@	 ░░░░ ░░ ░░░   ░░   ░░░     ░░     ░░ ░░░░  ░░   ░░░   ░░     ░░     ░░  ░░░░ ░░   	@@
@@	  ░░ ░  ░ ░     ░   ░ ░     ░     ░ ░░ ░░    ░   ░ ░  ░       ░     ░    ░░ ░ ░    	@@
  ```

# Project Overview

This project aims to create an enviornmental monitoring system, where water quality is monitored and the system alerts through both visual and auditory means if the quality drops below a set amount.

# Table of Contents

- [Installation](#installation)
	- [FPGA](#fpga)
	- [Device Tree](#device-tree)
	- [Drivers](#drivers)
- [Usage](#usage)
	- [Loading Drivers](#loading-drivers)
	- [Removing Drivers](#removing-drivers)
	- [Shell Script](#shell-script)
	- [System Level](#system-level)
- [Credits](#credits)

# Installation

## FPGA

The Quartis project to program the FPGA is found in `quartus/pwm/`, and will need to be built on each individual system. The generated programming file can then be converted to a raw binary file to set up the FPGA to program on boot using the quartus file conversion process.

## Device Tree
The final project device tree file can be found at `linux/dts/socfpga_cyclone5_de10nano_final_project.dts`

This will have to be compiled with the kernel it is going to be associated with. Please see kernel documentation for device tree compilation instructions.

## Drivers
The four project drivers can be found in: 
* `linux/adc/`
* `linux/drivers/kirkland-buzzer/`
* `linux/drivers/kirkland-rgb/`
* `linux/pwm/`

Each of these drivers has an associated Makefile that can be used to compile the `.ko` file.

# Usage

## Loading Drivers

The driver files can be loaded using the following shell commands:

```
sudo insmod de10nano_adc.ko
sudo insmod kirkland-buzzer.ko
sudo insmod kirkland-rgb.ko
sudo insmod pwm.ko
```

## Removing Drivers

The drivers can be unloaded using the following shell commands;
```
sudo rmmod de10nano_adc
sudo rmmod kirkland_buzzer
sudo rmmod kirkland_rgb
sudo rmmod pwm
```


## Shell Script

The shell script to run the associated software is located at `sw/pwm_miscdev_test.c`. This would be run on the device that is running the drivers and device tree, and requires the drivers be loaded prior to running. 

## System Level

Loading the drivers opens up the following:


* ``de10nano_adc >  /sys/devices/platform/ff200000.de10nano_adc``
	* ``
* ``kirkland_buzzer >  /sys/devices/platform/ff334200.kirkland_buzzer``
	* `period_reg`
* ``kirkland_rgb >  /sys/devices/platform/ff33E710.kirkland_rgb``
	* `period_reg`
	* `red_duty_cycle`
	* `grn_duty_cycle`
	* `blu_duty_cycle`
* ``pwm > /sys/devices/platform/ff25E240.pwm``
	* ``


# Credits

This project is presented with thanks to code and course material developed by:
* Trevor Vannoy
* Ross Snider
* Connor Dack