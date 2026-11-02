#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/display/cfb.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

static const struct i2c_dt_spec temperature_sensor = I2C_DT_SPEC_GET(DT_ALIAS(temperature_sensor));

int main(void)
{
	const struct device* dev;
	uint8_t font_width, font_height;

#define LINE_COUNT 4
#define LINE_WIDTH 12
	char lines[LINE_COUNT][LINE_WIDTH + 1];

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(dev)) {
		return 0;
	}

	if (cfb_framebuffer_init(dev)) {
		return 0;
	}

	if (cfb_framebuffer_clear(dev, true) < 0) {
		return 0;
	}

	if (cfb_get_font_size(dev, 0, &font_width, &font_height)) {
		return 0;
	}

	snprintf(lines[0], LINE_WIDTH + 1, "Temperature");
	snprintf(lines[1], LINE_WIDTH + 1, "Sensor");
	snprintf(lines[2], LINE_WIDTH + 1, "Address:");
	snprintf(lines[3], LINE_WIDTH + 1, "%#hx", temperature_sensor.addr);

	while (1) {
		k_sleep(K_MSEC(1000));

		cfb_framebuffer_clear(dev, false);

		for (int i = 0; i < LINE_COUNT; i++) {
			cfb_print(dev, lines[i], 0, font_height * i);
		}

		cfb_framebuffer_finalize(dev);
	}

	return 0;
}
