# pwm
This folder was designed by Kenneth Vincent for the final project for SOC FPGA I

## Files

### pwm_controller_avalon.vhdl
The device manager for the HPS avalon bus system for the PWM.
This will export the hardware for the pulse width modulator
with the mapping tools for the device tree.

### pwm_controller.vhdl
This the main hardware file for PWM as its the pulse width modulator with a fixed point duty cycle of 15.14 and a fixed point period of 26.20.

### pwm_controller_avalon_tb.vhdl
This is the test bench to test the hardware file to see if the
right ouputs responded with the inputs.

## Device Tree Node

```dts
    pwm: pwm@ff25E240 {
        compatible = "Vincent,pwm";
        reg = <0xff25E240 16>;
    };
```

## Register Map

| Name | Address | Offset | Purpose |
| ------------ | --------- | ----- | - |
| Base Address |  0x05E240 || Base Address |
| peri        |  | 0x0 | Pulse Period |
| red_out || 0x04 | Red Duty Cycle |
| green_out || 0x08 | Green Duty Cycle |
| blue_out || 0x12 | Blue Duty Cycle |