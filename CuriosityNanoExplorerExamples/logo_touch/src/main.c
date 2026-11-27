/* SPDX-License-Identifier: Apache-2.0 */

/* 
 * Curiosity Nano Explorer logo touch demo.
 * 
 * Uses the SAM D21 Peripheral Touch Controller (PTC) to detect touch on
 * the standalone capacitive touch button underneath the Microchip logo 
 * on the Curiosity Nano Explorer and displays the button state, 
 * current sensor reading, and baseline value on the 128 x 64 OLED display.
 * 
 * The open-source Adafruit PTC library is used to configure
 * the PTC and read capacitive touch measurements.
 * https://github.com/adafruit/Adafruit_FreeTouch
 * 
 * This demo only supports the SAM D21 Curiosity Nano.
 * It will not work on the SAM E51 Curiosity Nano
 * because it lacks open-source driver support.
 */

#include <errno.h>
#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <soc.h>

#include "freetouch/adafruit_ptc.h"

#define TOUCH_PIN 2U // PA02
#define TOUCH_Y_LINE 0U // PTC Y0
#define TOUCH_SAMPLE_PERIOD_MS 50U
#define TOUCH_CALIBRATION_SAMPLES 32U
#define TOUCH_PRESS_DELTA 18
#define TOUCH_RELEASE_DELTA 3
#define TOUCH_CONVERSION_TIMEOUT_MS 20U

static const struct device *const display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

static struct adafruit_ptc_config touch_config;

static void touch_enable_clock(void)
{
	unsigned int key;
	uint8_t channel = PTC_GCLK_ID;
	uint32_t clock_config;
	uint32_t previous_generator;

	clock_config = ((uint32_t)channel << GCLK_CLKCTRL_ID_Pos) |
		       (3U << GCLK_CLKCTRL_GEN_Pos);

	key = irq_lock();
	*((volatile uint8_t *)&GCLK->CLKCTRL.reg) = channel;
	previous_generator = GCLK->CLKCTRL.bit.GEN;
	GCLK->CLKCTRL.bit.GEN = 0U;
	GCLK->CLKCTRL.reg &= ~GCLK_CLKCTRL_CLKEN;
	while ((GCLK->CLKCTRL.reg & GCLK_CLKCTRL_CLKEN) != 0U) {
	}
	GCLK->CLKCTRL.bit.GEN = previous_generator;
	GCLK->CLKCTRL.reg = clock_config;
	*((volatile uint8_t *)&GCLK->CLKCTRL.reg) = channel;
	GCLK->CLKCTRL.reg |= GCLK_CLKCTRL_CLKEN;
	irq_unlock(key);

	PM->APBCMASK.reg |= PM_APBCMASK_PTC;
}

static int touch_measure(uint16_t *value)
{
	int64_t deadline;

	adafruit_ptc_start_conversion(PTC, &touch_config);
	deadline = k_uptime_get() + TOUCH_CONVERSION_TIMEOUT_MS;
	while (!adafruit_ptc_is_conversion_finished(PTC)) {
		if (k_uptime_get() >= deadline) {
			return -ETIMEDOUT;
		}
	}

	*value = adafruit_ptc_get_conversion_result(PTC) / 4U;
	return 0;
}

static int show_message(const char *message)
{
	int ret;

	ret = cfb_framebuffer_clear(display_dev, false);
	if (ret < 0) {
		return ret;
	}

	ret = cfb_print(display_dev, "LOGO TOUCH", 0, 0);
	if (ret < 0) {
		return ret;
	}

	ret = cfb_print(display_dev, message, 0, 24);
	if (ret < 0) {
		return ret;
	}

	return cfb_framebuffer_finalize(display_dev);
}

static int show_touch(uint16_t raw, uint16_t baseline, bool touched)
{
	char raw_line[24];
	char baseline_line[24];
	int ret;

	ret = cfb_framebuffer_clear(display_dev, false);
	if (ret < 0) {
		return ret;
	}

	snprintk(raw_line, sizeof(raw_line), "RAW: %u", raw);
	snprintk(baseline_line, sizeof(baseline_line), "BASE: %u", baseline);

	ret = cfb_print(display_dev, "LOGO TOUCH", 0, 0);
	if (ret < 0) {
		return ret;
	}

	ret = cfb_print(display_dev, touched ? "TOUCHED" : "RELEASED", 0, 16);
	if (ret < 0) {
		return ret;
	}

	ret = cfb_print(display_dev, raw_line, 0, 32);
	if (ret < 0) {
		return ret;
	}

	ret = cfb_print(display_dev, baseline_line, 0, 48);
	if (ret < 0) {
		return ret;
	}

	return cfb_framebuffer_finalize(display_dev);
}

int main(void)
{
	uint32_t total = 0U;
	uint16_t baseline;
	uint16_t raw;
	bool touched = false;
	int ret;

	if (!device_is_ready(display_dev)) {
		printk("Display is not ready\n");
		return 0;
	}

	ret = display_set_pixel_format(display_dev, PIXEL_FORMAT_MONO10);
	if (ret != 0) {
		ret = display_set_pixel_format(display_dev, PIXEL_FORMAT_MONO01);
		if (ret != 0) {
			printk("Unsupported display pixel format\n");
			return 0;
		}
	}

	ret = cfb_framebuffer_init(display_dev);
	if (ret != 0) {
		printk("CFB initialization failed\n");
		return 0;
	}

	ret = cfb_framebuffer_set_font(display_dev, 0);
	if (ret != 0) {
		printk("Font selection failed\n");
		return 0;
	}

	ret = display_blanking_off(display_dev);
	if ((ret != 0) && (ret != -ENOSYS)) {
		printk("Failed to turn display on\n");
		return 0;
	}

	ret = show_message("CALIBRATING");
	if (ret < 0) {
		printk("Failed to update display\n");
		return 0;
	}

	adafruit_ptc_get_config_default(&touch_config);
	touch_config.pin = TOUCH_PIN;
	touch_config.yline = TOUCH_Y_LINE;
	touch_enable_clock();
	adafruit_ptc_init(PTC, &touch_config);

	for (uint8_t sample = 0U; sample < TOUCH_CALIBRATION_SAMPLES; sample++) {
		ret = touch_measure(&raw);
		if (ret < 0) {
			show_message("PTC TIMEOUT");
			printk("PTC conversion timed out\n");
			return 0;
		}
		total += raw;
		k_msleep(TOUCH_SAMPLE_PERIOD_MS);
	}
	baseline = total / TOUCH_CALIBRATION_SAMPLES;

	while (true) {
		int32_t delta;

		ret = touch_measure(&raw);
		if (ret < 0) {
			show_message("PTC TIMEOUT");
			printk("PTC conversion timed out\n");
			return 0;
		}
		delta = (int32_t)raw - baseline;
		if (!touched && (delta >= TOUCH_PRESS_DELTA)) {
			touched = true;
		} else if (touched && (delta <= TOUCH_RELEASE_DELTA)) {
			touched = false;
		}

		if (!touched) {
			baseline = (uint16_t)(((uint32_t)baseline * 31U + raw) / 32U);
		}

		ret = show_touch(raw, baseline, touched);
		if (ret < 0) {
			printk("Display update failed\n");
			return 0;
		}

		k_msleep(TOUCH_SAMPLE_PERIOD_MS);
	}
}
