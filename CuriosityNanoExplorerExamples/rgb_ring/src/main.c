/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Curiosity Nano Explorer RGB LED ring example.
 *
 * Shows red, green and blue on all LEDs to check the color order, then
 * spins a rainbow around the ring. The LEDs are found through the
 * "led-strip" alias set in the board overlay.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(rgb_ring, LOG_LEVEL_INF);

#define STRIP_NODE DT_ALIAS(led_strip)
#define NUM_PIXELS DT_PROP(STRIP_NODE, chain_length)

/* WS2812B LEDs are very bright; keep the maximum channel value low */
#define BRIGHTNESS 32U

#define STEP_DELAY K_MSEC(40)

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);

static struct led_rgb pixels[NUM_PIXELS];

/* Map a position 0-255 on a color wheel to an RGB value */
static struct led_rgb color_wheel(uint8_t pos)
{
	struct led_rgb rgb;
	uint8_t ramp = (uint8_t)((pos % 85U) * 3U);

	if (pos < 85U) {
		rgb = (struct led_rgb){.r = 255U - ramp, .g = ramp, .b = 0U};
	} else if (pos < 170U) {
		rgb = (struct led_rgb){.r = 0U, .g = 255U - ramp, .b = ramp};
	} else {
		rgb = (struct led_rgb){.r = ramp, .g = 0U, .b = 255U - ramp};
	}

	rgb.r = (uint8_t)((rgb.r * BRIGHTNESS) / 255U);
	rgb.g = (uint8_t)((rgb.g * BRIGHTNESS) / 255U);
	rgb.b = (uint8_t)((rgb.b * BRIGHTNESS) / 255U);

	return rgb;
}

static int show_solid(struct led_rgb color)
{
	for (size_t i = 0; i < NUM_PIXELS; i++) {
		pixels[i] = color;
	}

	return led_strip_update_rgb(strip, pixels, NUM_PIXELS);
}

int main(void)
{
	static const struct {
		const char *name;
		struct led_rgb color;
	} tests[] = {
		{"red", {.r = BRIGHTNESS}},
		{"green", {.g = BRIGHTNESS}},
		{"blue", {.b = BRIGHTNESS}},
	};
	uint8_t offset = 0U;
	int rc;

	if (!device_is_ready(strip)) {
		LOG_ERR("LED strip %s is not ready", strip->name);
		return 0;
	}

	LOG_INF("Driving %d LEDs on %s", NUM_PIXELS, strip->name);

	for (size_t i = 0; i < ARRAY_SIZE(tests); i++) {
		LOG_INF("All LEDs %s", tests[i].name);
		rc = show_solid(tests[i].color);
		if (rc != 0) {
			LOG_ERR("Update failed: %d", rc);
		}
		k_sleep(K_SECONDS(1));
	}

	LOG_INF("Spinning rainbow");
	while (true) {
		for (size_t i = 0; i < NUM_PIXELS; i++) {
			pixels[i] = color_wheel((uint8_t)(offset + (i * 256U) / NUM_PIXELS));
		}

		rc = led_strip_update_rgb(strip, pixels, NUM_PIXELS);
		if (rc != 0) {
			LOG_ERR("Update failed: %d", rc);
		}

		offset += 4U;
		k_sleep(STEP_DELAY);
	}

	return 0;
}
