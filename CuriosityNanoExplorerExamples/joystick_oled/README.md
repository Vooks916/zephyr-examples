# Explorer Joystick + OLED demo (PIC32CM PL10 Curiosity Nano)

Moves a dot around the Curiosity Nano Explorer's 128x64 OLED with the 5-way joystick.
Pressing the joystick center switches between a white dot on black and a black dot on white.

## Hardware (from the Explorer user guide DS50003716 and the PL10 CNANO user guide DS50004003)
| Item | Connection |
|---|---|
| Explorer I2C bus | PL10 PA00 = SDA (SERCOM0 PAD0), PA01 = SCL (SERCOM0 PAD1) |
| OLED (SSD1306, 128x64) | I2C 0x3D |
| I/O Expander 2 (MCP23008) | I2C 0x24, GP0 Up, GP1 Left, GP2 Down, GP3 Right, GP4 Push (active-low) |

No jumper wires are needed: the I2C bus goes straight to the CNANO socket.

## Build and flash
```
west build -p always -b pic32cm_pl10_cnano path/to/joystick_oled
west flash
```
Serial console: 115200 8N1 on the Nano Debugger virtual COM port.

## Notes
- The stock `pic32cm_pl10_cnano` board files don't enable I2C, so `boards/pic32cm_pl10_cnano.overlay` turns SERCOM0 into an I2C controller.
- If the build fails in `i2c_mchp_sercom_g1.c` (for example a missing `SERCOM_I2CM_ADDR_ADDR_Msk`), your Zephyr/hal_microchip checkout is too old: run `west update`, or move to a newer Zephyr.
- If the screen stays blank but the console says everything is ready, try `reg = <0x3c>` in the overlay. If the image is shifted by 2 columns, the module is really an SH1106: change the compatible to `sinowealth,sh1106` and set `segment-offset = <2>`.
