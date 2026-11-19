/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Curiosity Nano Explorer ambient light demo.
 *
 * Reads the ambient light channel of the Explorer's VCNL4200 proximity and
 * ambient light sensor twice a second and shows the result on the OLED:
 * the light level in lux, the raw sensor count, and a bar graph.
 *
 * Zephyr has no VCNL4200 driver, so the sensor is read directly over I2C.
 * It shares the Explorer I2C bus with the OLED, so the I2C controller is
 * taken from the OLED's parent devicetree node.
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

/* VCNL4200 (Vishay), 7-bit I2C address 0x51 on the Explorer */
#define VCNL4200_ADDR      0x51
/* 16-bit registers, sent and received low byte first */
#define VCNL4200_ALS_CONF  0x00
#define VCNL4200_ALS_DATA  0x09
#define VCNL4200_ID        0x0E
#define VCNL4200_ID_VALUE  0x1058U
/* ALS_CONF = 0: sensor on, 50 ms integration time, interrupts off */
#define VCNL4200_ALS_ON    0x0000U
/* At 50 ms integration time one count is 0.024 lux, so 24 millilux */
#define MILLILUX_PER_COUNT 24U

/* Light level that fills the bar graph on the OLED */
#define BAR_FULL_SCALE_LUX 1000U

#define UPDATE_PERIOD K_MSEC(500)

static int vcnl4200_read(const struct device *i2c, uint8_t reg, uint16_t *value)
{
	uint8_t data[2];
	int ret;

	ret = i2c_write_read(i2c, VCNL4200_ADDR, &reg, sizeof(reg), data, sizeof(data));
	if (ret == 0) {
		*value = sys_get_le16(data);
	}

	return ret;
}

static int vcnl4200_write(const struct device *i2c, uint8_t reg, uint16_t value)
{
	uint8_t data[3] = {reg, value & 0xFFU, value >> 8};

	return i2c_write(i2c, data, sizeof(data), VCNL4200_ADDR);
}

static int vcnl4200_init(const struct device *i2c)
{
	uint16_t id;
	int ret;

	ret = vcnl4200_read(i2c, VCNL4200_ID, &id);
	if (ret != 0) {
		printk("VCNL4200 not responding at 0x%02x (%d)\n", VCNL4200_ADDR, ret);
		return ret;
	}

	if (id != VCNL4200_ID_VALUE) {
		printk("Unexpected VCNL4200 ID 0x%04x\n", id);
		return -ENODEV;
	}

	/* The ambient light channel is shut down after power-up */
	return vcnl4200_write(i2c, VCNL4200_ALS_CONF, VCNL4200_ALS_ON);
}

static void show_reading(const struct device *display, uint16_t raw, uint32_t millilux)
{
	uint16_t width = cfb_get_display_parameter(display, CFB_DISPLAY_WIDTH);
	uint16_t height = cfb_get_display_parameter(display, CFB_DISPLAY_HEIGHT);
	uint8_t font_width;
	uint8_t font_height;
	char line[16];

	cfb_get_font_size(display, 0, &font_width, &font_height);
	cfb_framebuffer_clear(display, false);

	cfb_print(display, "LIGHT SENSOR", 0, 0);

	snprintf(line, sizeof(line), "%u.%u lux", millilux / 1000U, (millilux % 1000U) / 100U);
	cfb_print(display, line, 0, font_height);

	snprintf(line, sizeof(line), "raw %u", raw);
	cfb_print(display, line, 0, 2 * font_height);

	/* Bar graph in the space below the text */
	struct cfb_position start = {.x = 0, .y = 3 * font_height + 2};
	struct cfb_position end = {.x = width - 1, .y = height - 1};
	uint32_t lux = MIN(millilux / 1000U, BAR_FULL_SCALE_LUX);
	uint16_t fill = (uint16_t)(((width - 4U) * lux) / BAR_FULL_SCALE_LUX);

	cfb_draw_rect(display, &start, &end);
	if (fill > 0U) {
		cfb_invert_area(display, start.x + 2, start.y + 2, fill, end.y - start.y - 3);
	}

	cfb_framebuffer_finalize(display);
}

int main(void)
{
	const struct device *const display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	const struct device *const i2c = DEVICE_DT_GET(DT_BUS(DT_CHOSEN(zephyr_display)));
	int ret;

	printk("Explorer ambient light demo\n");

	if (!device_is_ready(i2c)) {
		printk("I2C bus %s not ready\n", i2c->name);
		return 0;
	}

	if (!device_is_ready(display)) {
		printk("OLED not ready (check I2C address 0x3D)\n");
		return 0;
	}

	ret = cfb_framebuffer_init(display);
	if (ret != 0) {
		printk("Framebuffer init failed (%d)\n", ret);
		return 0;
	}
	display_blanking_off(display);

	ret = vcnl4200_init(i2c);
	if (ret != 0) {
		cfb_framebuffer_clear(display, false);
		cfb_print(display, "NO SENSOR", 0, 0);
		cfb_framebuffer_finalize(display);
		return 0;
	}

	while (true) {
		uint16_t raw;

		/* The first reading is ready one integration time after power-on */
		k_sleep(UPDATE_PERIOD);

		ret = vcnl4200_read(i2c, VCNL4200_ALS_DATA, &raw);
		if (ret != 0) {
			printk("VCNL4200 read failed (%d)\n", ret);
			continue;
		}

		uint32_t millilux = (uint32_t)raw * MILLILUX_PER_COUNT;

		printk("raw %u, %u.%u lux\n", raw, millilux / 1000U, (millilux % 1000U) / 100U);
		show_reading(display, raw, millilux);
	}

	return 0;
}
