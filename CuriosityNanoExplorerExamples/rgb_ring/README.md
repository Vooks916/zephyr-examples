# RGB LED ring

Drives the eight WS2812B-compatible RGB LEDs on the Curiosity Nano Explorer
through the Zephyr `led_strip` API.

On start the example lights all LEDs red, green and blue for one second each
(if the colors come out in the wrong order, the `color-mapping` in the overlay
is wrong), then spins a rainbow around the ring. Progress is logged on the
Nano's virtual COM port at 115200 baud.

## Supported boards

| Board | Data pin (Explorer IO 27) | Overlay |
|---|---|---|
| SAM D21 Curiosity Nano (`samd21_cnano`) | PA28 | `boards/samd21_cnano.overlay` |
| SAM E51 Curiosity Nano (`same51_cnano`) | PB04 | `boards/same51_cnano.overlay` |

Zephyr picks up `boards/<board>.overlay` automatically, so no extra build
arguments are needed.

## How it works

The overlay adds a `worldsemi,ws2812-gpio` node on the LED data pin and points
the `led-strip` alias at it. `src/main.c` gets the device with
`DEVICE_DT_GET(DT_ALIAS(led_strip))`, fills an array of `struct led_rgb` and
sends it with `led_strip_update_rgb()`.

Neither data pin can be routed to a SERCOM, so the SPI and UART based WS2812
drivers cannot be used. The GPIO driver generates the bit timing in software
with interrupts locked, about 30 us per LED. Its delays are derived from the
CPU `clock-frequency` in the board devicetree.

The SAM support in the GPIO driver (`drivers/led_strip/ws2812_gpio.c`) is a
change in our Zephyr tree that is not upstream yet. Build against a Zephyr
tree that contains it.

## Building and flashing

From the Zephyr workspace:

```shell
west build -p -b samd21_cnano /path/to/zephyr-examples/rgb_ring
west flash
```

Use `-b same51_cnano` for the SAM E51 Curiosity Nano.
