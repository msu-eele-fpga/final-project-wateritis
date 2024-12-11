# Buzzer

## Files

### Buzzer.vhdl

This VHDL code makes a square wave with a 50% duty cycle. This uses a period which is a fixed point 13.12 number.

### Buzzer_avalon.vhdl

This exports the buzzer for the avalon memory mapping tools

### Buzzer_avalon_hw.tcl

Device manager buzzer avalon component

## Device Tree Node

```dts
	buzzer: buzzer@ff334200 {
		compatible = "Kirkland,kirkland_buzzer";
		reg = <0xff334200 4>;
	};
```

## Register Map

| Name | Address | Offset | Purpose |
| ------------ | --------- | ----- | - |
| Base Address |  0x134200 || Base Address |
| period_reg |  | 0x0 | Pulse Period |
