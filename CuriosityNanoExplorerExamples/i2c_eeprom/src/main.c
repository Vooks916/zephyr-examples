#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/devicetree.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/i2c/target/eeprom.h>
#include <zephyr/kernel.h>
#include <zephyr/sleep.h>

int main(void)
{
	const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	const struct device *eeprom = DEVICE_DT_GET(DT_ALIAS(eeprom));

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

	if (!device_is_ready(eeprom)) {
		return 0;
	}

	size_t eeprom_size = eeprom_target_get_size(eeprom);

	char line[13];

	snprintf(line, sizeof(line), "%zu", eeprom_size);

	while (1) {
		if (cfb_framebuffer_clear(display, false)) {
			return 0;
		}

		if (cfb_draw_text(display, "EEPROM size:", 0, font_height * 0)) {
			return 0;
		}

		if (cfb_draw_text(display, line, 0, font_height * 1)) {
			return 0;
		}

		if (cfb_framebuffer_finalize(display)) {
			return 0;
		}

		k_sleep(K_MSEC(1000));
	}
}
