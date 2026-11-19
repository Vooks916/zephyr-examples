#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/devicetree.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/i2c/target/eeprom.h>
#include <zephyr/kernel.h>
#include <zephyr/sleep.h>

int main(void)
{
	const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	const struct device *eeprom = DEVICE_DT_GET(DT_ALIAS(eeprom));
	const struct adc_dt_spec potentiometer = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

	if (!device_is_ready(display)) {
		return 0;
	}

	if (!device_is_ready(eeprom)) {
		return 0;
	}

	if (!adc_is_ready_dt(&potentiometer)) {
		return 0;
	}

	if (adc_channel_setup_dt(&potentiometer)) {
		return 0;
	}

	uint16_t initial_potentiometer_value, potentiometer_value;

	struct adc_sequence sequence = {
		.buffer = &potentiometer_value,
		.buffer_size = sizeof(potentiometer_value),
	};

	if (adc_sequence_init_dt(&potentiometer, &sequence)) {
		return 0;
	}

	if (cfb_framebuffer_init(display)) {
		return 0;
	}

	uint8_t font_width, font_height;

	if (cfb_get_font_size(display, 0, &font_width, &font_height)) {
		return 0;
	}

	if (!device_is_ready(eeprom)) {
		return 0;
	}

	off_t eeprom_offset = 23;

	if (eeprom_read(eeprom, eeprom_offset, &initial_potentiometer_value, sizeof(potentiometer_value))) {
		return 0;
	}

	char rotation_line[13];
	char initial_rotation_line[13];

	snprintf(initial_rotation_line, sizeof(initial_rotation_line), "%hu", initial_potentiometer_value);

	while (1) {
		if (cfb_framebuffer_clear(display, false)) {
			return 0;
		}

		if (adc_read_dt(&potentiometer, &sequence)) {
			return 0;
		}

		if (cfb_draw_text(display, "Rotation:", 0, font_height * 0)) {
			return 0;
		}

		snprintf(rotation_line, sizeof(rotation_line), "%hu", potentiometer_value);

		if (cfb_draw_text(display, rotation_line, 0, font_height * 1)) {
			return 0;
		}

		if (eeprom_write(eeprom, eeprom_offset, &potentiometer_value, sizeof(potentiometer_value))) {
			return 0;
		}

		if (cfb_draw_text(display, "Initial:", 0, font_height * 2)) {
			return 0;
		}

		if (cfb_draw_text(display, initial_rotation_line, 0, font_height * 3)) {
			return 0;
		}

		if (cfb_framebuffer_finalize(display)) {
			return 0;
		}

		k_sleep(K_MSEC(1000));
	}
}
