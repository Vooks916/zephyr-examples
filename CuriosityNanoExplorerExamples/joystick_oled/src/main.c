/*
 * Joystick + OLED demo for PIC32CM PL10 Curiosity Nano on the Curiosity Nano Explorer.
 *
 *  - A dot is drawn on the 128x64 SSD1306 OLED.
 *  - Joystick UP/DOWN/LEFT/RIGHT moves the dot 4 px (wraps at the edges).
 *    Holding a direction auto-repeats.
 *  - Joystick PUSH (center click) "changes the color": the OLED is monochrome,
 *    so it toggles between a white dot on black and a black dot on white.
 *
 * Joystick wiring (Explorer user guide DS50003716, Buttons schematic):
 *   I/O Expander 2 = MCP23008 @ I2C 0x24
 *   GP0 = J-UP, GP1 = J-LEFT, GP2 = J-DOWN, GP3 = J-RIGHT, GP4 = J-PUSH,
 *   GP5 = SW3, GP6 = SW2, GP7 = SW1
 *   Switches pull to GND when pressed (active-low); pull-ups must be enabled in the MCP23008.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/printk.h>
#include <string.h>

/* ---------- MCP23008 (I/O Expander 2) ---------- */
#define IOEXP2_ADDR     0x24
#define MCP_IODIR       0x00
#define MCP_GPPU        0x06
#define MCP_GPIO        0x09

#define JOY_UP          BIT(0)
#define JOY_LEFT        BIT(1)
#define JOY_DOWN        BIT(2)
#define JOY_RIGHT       BIT(3)
#define JOY_PUSH        BIT(4)
#define JOY_DIR_MASK    (JOY_UP | JOY_LEFT | JOY_DOWN | JOY_RIGHT)

/* ---------- Display / dot ---------- */
#define SCREEN_W        128
#define SCREEN_H        64
#define DOT_SIZE        6
#define STEP_PX         4

#define POLL_MS         20      /* joystick polling period */
#define REPEAT_DELAY_MS 300     /* hold time before auto-repeat starts */
#define REPEAT_RATE_MS  60      /* auto-repeat interval */

static const struct device *const i2c_bus = DEVICE_DT_GET(DT_NODELABEL(sercom0));
static const struct device *const display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

/* 1 bit per pixel, vertically tiled (SSD1306 native): byte = 8 vertical pixels, LSB on top */
static uint8_t framebuf[SCREEN_W * SCREEN_H / 8];

/* Rounded 6x6 dot, one row per byte, bit 5 = leftmost pixel */
static const uint8_t dot_bitmap[DOT_SIZE] = {
	0x1E, /* .####. */
	0x3F, /* ###### */
	0x3F, /* ###### */
	0x3F, /* ###### */
	0x3F, /* ###### */
	0x1E, /* .####. */
};

static int dot_x = (SCREEN_W - DOT_SIZE) / 2;
static int dot_y = (SCREEN_H - DOT_SIZE) / 2;
static bool inverted;

static void set_pixel(int x, int y, bool on)
{
	/* wrap so the dot slides smoothly off one edge and onto the other */
	x = (x + SCREEN_W) % SCREEN_W;
	y = (y + SCREEN_H) % SCREEN_H;

	uint8_t *byte = &framebuf[(y / 8) * SCREEN_W + x];
	uint8_t mask = BIT(y % 8);

	if (on) {
		*byte |= mask;
	} else {
		*byte &= ~mask;
	}
}

static int redraw(void)
{
	/* background: black normally, white when inverted */
	memset(framebuf, inverted ? 0xFF : 0x00, sizeof(framebuf));

	for (int row = 0; row < DOT_SIZE; row++) {
		for (int col = 0; col < DOT_SIZE; col++) {
			if (dot_bitmap[row] & BIT(DOT_SIZE - 1 - col)) {
				/* dot is the opposite color of the background */
				set_pixel(dot_x + col, dot_y + row, !inverted);
			}
		}
	}

	struct display_buffer_descriptor desc = {
		.buf_size = sizeof(framebuf),
		.width = SCREEN_W,
		.height = SCREEN_H,
		.pitch = SCREEN_W,
	};

	return display_write(display, 0, 0, &desc, framebuf);
}

static int ioexp_init(void)
{
	int ret;

	ret = i2c_reg_write_byte(i2c_bus, IOEXP2_ADDR, MCP_IODIR, 0xFF); /* all inputs */
	if (ret) {
		return ret;
	}
	return i2c_reg_write_byte(i2c_bus, IOEXP2_ADDR, MCP_GPPU, 0xFF); /* all pull-ups on */
}

/* Returns a bitmask of PRESSED inputs (active-high after inversion), or <0 on error */
static int ioexp_read_pressed(void)
{
	uint8_t val;
	int ret = i2c_reg_read_byte(i2c_bus, IOEXP2_ADDR, MCP_GPIO, &val);

	return ret ? ret : (uint8_t)~val;
}

static void move_dot(uint8_t dirs)
{
	if (dirs & JOY_UP) {
		dot_y -= STEP_PX;
	}
	if (dirs & JOY_DOWN) {
		dot_y += STEP_PX;
	}
	if (dirs & JOY_LEFT) {
		dot_x -= STEP_PX;
	}
	if (dirs & JOY_RIGHT) {
		dot_x += STEP_PX;
	}
	dot_x = (dot_x + SCREEN_W) % SCREEN_W;
	dot_y = (dot_y + SCREEN_H) % SCREEN_H;
}

int main(void)
{
	printk("Explorer joystick + OLED demo\n");

	if (!device_is_ready(i2c_bus)) {
		printk("ERROR: I2C bus (sercom0) not ready\n");
		return 0;
	}
	if (!device_is_ready(display)) {
		printk("ERROR: OLED display not ready (check I2C address 0x3D)\n");
		return 0;
	}

	/* 1 = lit pixel */
	display_set_pixel_format(display, PIXEL_FORMAT_MONO01);
	display_blanking_off(display);

	if (ioexp_init()) {
		printk("ERROR: I/O Expander 2 (MCP23008 @ 0x24) not responding\n");
		return 0;
	}

	redraw();

	uint8_t prev = 0;
	int64_t next_repeat = 0;

	while (1) {
		int pressed = ioexp_read_pressed();

		if (pressed < 0) {
			printk("I2C read error %d\n", pressed);
			k_msleep(200);
			continue;
		}

		uint8_t now = (uint8_t)pressed;
		uint8_t new_presses = now & ~prev;
		bool dirty = false;
		int64_t t = k_uptime_get();

		/* center click -> toggle color (on press edge only) */
		if (new_presses & JOY_PUSH) {
			inverted = !inverted;
			printk("Click: %s\n", inverted ? "black dot / white bg" : "white dot / black bg");
			dirty = true;
		}

		/* directions: move on press, then auto-repeat while held */
		uint8_t dirs = now & JOY_DIR_MASK;

		if (new_presses & JOY_DIR_MASK) {
			move_dot(dirs);
			next_repeat = t + REPEAT_DELAY_MS;
			dirty = true;
		} else if (dirs && t >= next_repeat) {
			move_dot(dirs);
			next_repeat = t + REPEAT_RATE_MS;
			dirty = true;
		}

		if (dirty) {
			redraw();
		}

		prev = now;
		k_msleep(POLL_MS);
	}

	return 0;
}
