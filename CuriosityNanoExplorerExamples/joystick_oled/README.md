# Explorer Joystick + OLED demo (Curiosity Nano + Curiosity Nano Explorer)

Moves a dot around the Curiosity Nano Explorer's 128x64 OLED with the 5-way joystick.
Pressing the joystick center switches between a white dot on black and a black dot on white.
Holding a direction keeps the dot moving; it wraps at the screen edges.

## Supported boards
| Board | `-b` identifier | Zephyr source | Overlay |
|---|---|---|---|
| PIC32CM PL10 Curiosity Nano | `pic32cm_pl10_cnano` | Upstream Zephyr | `boards/pic32cm_pl10_cnano.overlay` |
| SAM D21 Curiosity Nano | `samd21_cnano` | [Vooks916/zephyr `samd21-cnano`](https://github.com/Vooks916/zephyr/tree/samd21-cnano) | `boards/samd21_cnano.overlay` |
| SAM E51 Curiosity Nano | `same51_cnano` | [Vooks916/zephyr `same51-cnano`](https://github.com/Vooks916/zephyr/tree/same51-cnano) | `boards/same51_cnano.overlay` |

`src/main.c`, `prj.conf` and `CMakeLists.txt` are the same for every board. Zephyr picks up the right overlay
automatically from `boards/<board>.overlay`. The code finds the I2C bus from the OLED's parent node, so the
I2C controller only has to be set up in the overlay.

## Explorer hardware (user guide DS50003716)
| Item | Connection |
|---|---|
| OLED (SSD1306, 128x64) | Explorer I2C bus, address 0x3D |
| I/O Expander 2 (MCP23008) | Explorer I2C bus, address 0x24. GP0 Up, GP1 Left, GP2 Down, GP3 Right, GP4 Push (active-low, the firmware enables the expander's pull-ups) |
| Explorer I2C bus | CNANO socket I2C SDA / SCL pins (COM section) |

No jumper wires are needed. The I2C SDA/SCL jumper caps in the remapping area must be fitted (they are by default).
Seat the Nano with its USB connector toward the top of the Explorer, pushed all the way to that end of the socket.

## Per-board I2C setup
| Board | SDA | SCL | I2C controller | Driver | What the overlay does |
|---|---|---|---|---|---|
| PIC32CM PL10 | PA00 (`PA0C_SERCOM0_PAD0`) | PA01 (`PA1C_SERCOM0_PAD1`) | SERCOM0, 400 kHz, GCLK0 24 MHz | `microchip,sercom-g1-i2c` | Board files don't enable I2C, so the overlay turns SERCOM0 into an I2C controller (pins, clock, compatible) and adds the OLED |
| SAM D21 | PA12 (`PA12D_SERCOM4_PAD0`) | PA13 (`PA13D_SERCOM4_PAD1`) | SERCOM4, 400 kHz, GCLK0 48 MHz | `atmel,sam0-i2c` | Board already enables SERCOM4 as I2C; overlay restates it with `input-enable` and adds the OLED |
| SAM E51 | PA12 (`PA12C_SERCOM2_PAD0`) | PA13 (`PA13C_SERCOM2_PAD1`) | SERCOM2, 400 kHz, GCLK1 60 MHz | `microchip,sercom-g1-i2c` | Board already enables SERCOM2 as I2C; overlay adds `input-enable` and the OLED |

## Build and flash
Run from the Zephyr checkout that contains the board (upstream for the PL10, the matching Vooks916 branch for the SAMD21 / SAME51):

```
west build -p always -b pic32cm_pl10_cnano path/to/joystick_oled
west build -p always -b samd21_cnano       path/to/joystick_oled
west build -p always -b same51_cnano       path/to/joystick_oled
west flash
```

All three boards flash with pyOCD through the onboard nEDBG debugger. pyOCD needs the device pack for each chip once:

```
pyocd pack install pic32cm6408pl10048
pyocd pack install atsamd21g17d
pyocd pack install atsame51j20a
```

If `west flash` fails with `Unexpected ACK '0'`, connect slower with the chip held in reset:

```
west flash --flash-opt="-f 100000" --flash-opt="-M under-reset"
```

## Serial console
115200 8N1 on the Nano Debugger virtual COM port.

| Board | Console UART |
|---|---|
| PIC32CM PL10 | SERCOM1 (PB00 / PB01) |
| SAM D21 | SERCOM5 (PB22 / PA22) |
| SAM E51 | SERCOM5 (PB16 / PB17) |

At startup the app prints `I2C bus: <device>` and reports if the OLED or the MCP23008 doesn't respond.

## Troubleshooting
- **Build fails in `i2c_mchp_sercom_g1.c`** (for example a missing `SERCOM_I2CM_ADDR_ADDR_Msk`): your Zephyr / hal_microchip checkout is too old for that I2C driver. Update the checkout.
- **Dot shows but doesn't move:** the console will say whether the MCP23008 at 0x24 answered. If it didn't, check the I2C jumper caps and how the Nano is seated.
- **Screen stays blank but the console says everything is ready:** try `reg = <0x3c>` for the OLED in the overlay.
- **Image shifted by 2 columns:** the module is an SH1106. Change the OLED's compatible to `sinowealth,sh1106` and set `segment-offset = <2>`.
