# Microchip Logo Standalone Capacitive Touch Button demo

Uses the SAMD 21 Peripheral Touch Controller (PTC) to detect touch on 
the standalone capacitive touch button underneath the Microchip logo 
on the Curiosity Nano Explorer and displays the button state, 
current sensor reading, and baseline value on the 128 x 64 OLED display.

## Setup and Wiring

To use the standalone capacitive touch button, the touch sensor must be 
connected to ADC0 (PA02), which corresponds to the PTC Y0 channel on the SAMD21.

On the Curiosity Nano Explorer board:
1. Connect a jumper wire from ADC0 to the SENSOR pin at J404 (labeled TS4)
2. Remove the pin jumper connecting ADC0 and SW1.

## How to Build and Flash

From the Zephyr workspace:

```shell
west build --pristine -b samd21_cnano path/to/logo_touch
west flash --target atsamd21g17d
```

## Notes

This demo uses files from the open-source 
[Adafruit FreeTouch library](https://github.com/adafruit/Adafruit_FreeTouch)

- `adafruit_ptc.c`
- `adafruit_ptc.h`
- `samd21_ptc_component.h` (modified)
