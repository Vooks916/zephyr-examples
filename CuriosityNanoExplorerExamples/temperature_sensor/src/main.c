#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/devicetree.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sleep.h>

#define TEMPERATURE_REGISTER_ADDRESS (0b0101)

bool read_temperature(const struct i2c_dt_spec *temperature_sensor, uint8_t *temperature)
{
	uint8_t i2c_write_buffer[1], i2c_read_buffer[2];

	i2c_write_buffer[0] = TEMPERATURE_REGISTER_ADDRESS;

	if (i2c_write_read_dt(temperature_sensor,
	                      i2c_write_buffer, sizeof(i2c_write_buffer),
	                      i2c_read_buffer, sizeof(i2c_read_buffer))) {
		return false;
	}

	uint8_t upper_byte = i2c_read_buffer[0];
	uint8_t lower_byte = i2c_read_buffer[1];

	bool is_signed = upper_byte & (1 << 5);

	*temperature = (upper_byte << 4) + (lower_byte >> 4);

	if (is_signed) {
		*temperature = 256 - *temperature;
	}

	return true;
}

int main(void)
{
	const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	const struct i2c_dt_spec temperature_sensor = I2C_DT_SPEC_GET(DT_ALIAS(temperature_sensor));

	if (!device_is_ready(display)) {
		return 0;
	}

	if (cfb_framebuffer_init(display)) {
		return 0;
	}

	uint8_t font_width, font_height;

	if (cfb_get_font_size(display, 0, &font_width, &font_height)) {
		return 0;
	}

	if (!i2c_is_ready_dt(&temperature_sensor)) {
		return 0;
	}

	while (1) {
		if (cfb_framebuffer_clear(display, false)) {
			return 0;
		}

		if (cfb_draw_text(display, "Temperature:", 0, font_height * 0)) {
			return 0;
		}

		uint8_t temperature;

		if (!read_temperature(&temperature_sensor, &temperature)) {
			return 0;
		}

		char line[13];

		snprintf(line, sizeof(line), "%hhu C", temperature);

		if (cfb_draw_text(display, line, 0, font_height * 1)) {
			return 0;
		}

		if (cfb_framebuffer_finalize(display)) {
			return 0;
		}

		k_sleep(K_MSEC(1000));
	}
}
