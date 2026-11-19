/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define MICROPHONE_NODE DT_PATH(zephyr_user)

#define SAMPLE_COUNT 64U
#define UPDATE_PERIOD K_MSEC(250)
#define BAR_FULL_SCALE 1024U

#if !DT_NODE_EXISTS(MICROPHONE_NODE) || !DT_NODE_HAS_PROP(MICROPHONE_NODE, io_channels)
#error "No microphone ADC channel is defined"
#endif

static const struct adc_dt_spec microphone = ADC_DT_SPEC_GET_BY_IDX(MICROPHONE_NODE, 0);

static void show_reading(const struct device *display, uint16_t level)
{
	uint16_t width = cfb_get_display_parameter(display, CFB_DISPLAY_WIDTH);
	uint16_t height = cfb_get_display_parameter(display, CFB_DISPLAY_HEIGHT);
	uint8_t font_width;
	uint8_t font_height;
	struct cfb_position start;
	struct cfb_position end;
	uint16_t fill;
	char line[16];

	cfb_get_font_size(display, 0, &font_width, &font_height);
	cfb_framebuffer_clear(display, false);

	cfb_print(display, "MICROPHONE", 0, 0);

	snprintf(line, sizeof(line), "PK-PK: %u", level);
	cfb_print(display, line, 0, 2 * font_height);

	start.x = 0;
	start.y = 3 * font_height + 2;
	end.x = width - 1U;
	end.y = height - 1U;
	cfb_draw_rect(display, &start, &end);

	fill = (uint16_t)(((width - 4U) * MIN(level, BAR_FULL_SCALE)) / BAR_FULL_SCALE);
	if (fill > 0U) {
		cfb_invert_area(display, start.x + 2, start.y + 2, fill,
				end.y - start.y - 3U);
	}

	cfb_framebuffer_finalize(display);
}

int main(void)
{
	const struct device *const display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	uint16_t sample;
	struct adc_sequence sequence = {
		.buffer = &sample,
		.buffer_size = sizeof(sample),
	};
	int ret;

	if (!adc_is_ready_dt(&microphone)) {
		printk("ADC controller %s not ready\n", microphone.dev->name);
		return 0;
	}

	if (!device_is_ready(display)) {
		printk("OLED not ready\n");
		return 0;
	}

	ret = cfb_framebuffer_init(display);
	if (ret != 0) {
		printk("Framebuffer init failed (%d)\n", ret);
		return 0;
	}

	ret = display_blanking_off(display);
	if (ret != 0 && ret != -ENOSYS) {
		printk("OLED enable failed (%d)\n", ret);
		return 0;
	}

	ret = adc_channel_setup_dt(&microphone);
	if (ret != 0) {
		printk("Microphone ADC setup failed (%d)\n", ret);
		return 0;
	}

	adc_sequence_init_dt(&microphone, &sequence);

	while (true) {
		uint16_t minimum = UINT16_MAX;
		uint16_t maximum = 0U;

		for (uint32_t index = 0U; index < SAMPLE_COUNT; index++) {
			ret = adc_read_dt(&microphone, &sequence);
			if (ret != 0) {
				printk("Microphone ADC read failed (%d)\n", ret);
				break;
			}

			if (sample < minimum) {
				minimum = sample;
			}
			if (sample > maximum) {
				maximum = sample;
			}
		}

		if (ret == 0) {
			uint16_t level = (uint16_t)(maximum - minimum);

			show_reading(display, level);
		}

		k_sleep(UPDATE_PERIOD);
	}

	return 0;
}
