# LTC2308 driver for the DE10 Nano

The DE10 Nano has an [Analog Devices LTC2308 ADC](https://www.analog.com/en/products/ltc2308.html) on the PCB. Terasic / the Intel University Program provide an HDL component that interfaces with and drives the ADC. 

## Building

The Makefile in this directory cross-compiles the driver. Update the `KDIR` variable to point to your linux-socfpga repository directory.

Run `make` in this directory to build to kernel module.

## Device tree node

Use the following device tree node:
```devicetree
de10nano_adc: adc@ff200000 {
    compatible = "adsd,de10nano_adc";
    reg = <0xff200000 32>;
};
```

## Notes / bugs :bug:

The Intel FPGA University Program documentation claims the ADC has an input range of 0--5 V. According to the AD datasheet, the unipolar input range is 0--VREFCOMP, which 4.096 V. If you hook a pot up to a 5 V supply, you'll notice there is a deadzone at the upper end of the pot's range, indicating that the input range stops before 5 V :facepalm:

The `update` and `auto_update` registers don't appear to do anything... The channels always update when you read them, regardless of the `auto_update` setting... :bug:

## Register map

This register map is dumb. Write-only registers are dumb. Having different read/write values at the same address is dumb. And they don't even appear to work (see the previous section).

| Offset | Name         | R/W | Purpose                    |
|--------|--------------|-----|----------------------------|
| 0x0    | CH_0         | R   | Channel 0 value            |
| 0x0    | udpate       | W   | Manually update values     |
| 0x4    | CH_1         | R   | Channel 1 value            |
| 0x4    | auto_update  | W   | enable/disable auto update |
| 0x8    | CH_2         | R   | Channel 2 value            |
| 0xC    | CH_3         | R   | Channel 3 value            |
| 0x10   | CH_4         | R   | Channel 4 value            |
| 0x14   | CH_5         | R   | Channel 5 value            |
| 0x18   | CH_6         | R   | Channel 6 value            |
| 0x1C   | CH_7         | R   | Channel 7 value            |

## Documentation

- [DE-Series ADC Controller HDL component documentation](https://ftp.intel.com/Public/Pub/fpgaup/pub/Teaching_Materials/current/Tutorials/Using_DE_Series_ADC.pdf)
- [AD LTC2308 ADC](https://www.analog.com/en/products/ltc2308.html)
