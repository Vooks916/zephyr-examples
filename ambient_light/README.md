# Explorer ambient light + OLED demo

Reads the ambient light sensor on the Curiosity Nano Explorer twice a second and shows the
light level in lux, the raw sensor count and a bar graph on the 128x64 OLED. The same values
are printed on the serial console.

## Supported boards
| Board | `-b` identifier | Zephyr source | Overlay |
|---|---|---|---|
| SAM D21 Curiosity Nano | `samd21_cnano` | [Vooks916/zephyr `samd21-cnano`](https://github.com/Vooks916/zephyr/tree/samd21-cnano) | `boards/samd21_cnano.overlay` |
| SAM E51 Curiosity Nano | `same51_cnano` | [Vooks916/zephyr `same51-cnano`](https://github.com/Vooks916/zephyr/tree/same51-cnano) | `boards/same51_cnano.overlay` |

`src/main.c`, `prj.conf` and `CMakeLists.txt` are the same for every board. Zephyr picks up the
right overlay automatically from `boards/<board>.overlay`.

## Explorer hardware (user guide DS50003716)
| Item | Connection |
|---|---|
| VCNL4200 proximity and ambient light sensor | Explorer I2C bus, address 0x51 |
| OLED (SSD1306, 128x64) | Explorer I2C bus, address 0x3D |

The I2C SDA/SCL jumper caps in the remapping area must be fitted (they are by default).

## How it works
Zephyr has no driver for the VCNL4200, so `main.c` talks to it directly with `i2c_write_read()`
and `i2c_write()`. The I2C controller is taken from the OLED's parent devicetree node, since
both parts share the Explorer I2C bus.

| VCNL4200 register | Use |
|---|---|
| `0x0E` ID | Read at startup; must be `0x1058` |
| `0x00` ALS_CONF | Written to `0x0000`: turns the light sensor on (it is off after power-up), 50 ms integration time |
| `0x09` ALS_DATA | Raw light count. At 50 ms integration time one count is 0.024 lux, so the range is 0 to about 1570 lux |

Registers are 16 bits wide and sent low byte first. The OLED is drawn with the character
framebuffer (`cfb_print()`, `cfb_draw_rect()`, `cfb_invert_area()`). The bar is full at 1000 lux.

## Per-board I2C setup
| Board | SDA / SCL | I2C controller | Speed | What the overlay does |
|---|---|---|---|---|
| SAM D21 | PA12 / PA13 | SERCOM4 | 400 kHz | Adds the OLED |
| SAM E51 | PA12 / PA13 | SERCOM2 | 400 kHz | Adds the OLED |

## Build and flash
Run from the Zephyr checkout on the branch that contains the board:

```
west build -p always -b samd21_cnano path/to/ambient_light
west build -p always -b same51_cnano path/to/ambient_light
west flash
```

## Serial console
115200 8N1 on the Nano Debugger virtual COM port. At startup the app prints
`Explorer ambient light demo`, then one line per reading, for example `raw 5142, 123.4 lux`.

## Troubleshooting
- **OLED shows `NO SENSOR`:** the console says whether the VCNL4200 did not answer or
  returned the wrong ID. Check the I2C jumper caps and how the Nano is seated. If it still
  fails, try the bus at 100 kHz by adding `clock-frequency = <I2C_BITRATE_STANDARD>;` (with
  `#include <zephyr/dt-bindings/i2c/i2c.h>`) to the I2C controller node in the overlay.
- **Reading stays at 0:** make sure nothing covers the VCNL4200, and that the console does not
  show read errors.
