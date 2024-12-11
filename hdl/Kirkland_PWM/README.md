# Kirkland_PWM

## Files

### PWM_Controller.vhdl

This VHDL code makes a pulse width modulator, where duty cycle is a fixed point 22.21 number, and period is a fixed point 13.7 number.

### PWM_Controller_avalon.vhdl

This exports the pulse width modulator for the avalon memory mapping tools

## Device Tree Node

```dts
	rgb_controller: rgb_controller@ff33E710 {
		compatible = "Kirkland,kirkland_rgb";
		reg = <0xff33E710 16>;
	};
```

## Register Map

| Name | Address | Offset | Purpose |
| ------------ | --------- | ----- | - |
| Base Address |  0x13E710 || Base Address |
| period_reg |  | 0x0 | Pulse Period |
| red_dc_reg || 0x04 | Red Duty Cycle |
| grn_dc_reg || 0x08 | Green Duty Cycle |
| blu_dc_reg || 0x0C | Blue Duty Cycle |