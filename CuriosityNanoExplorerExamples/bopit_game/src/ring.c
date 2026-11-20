/* SPDX-License-Identifier: Apache-2.0 */
/* WS2812B ring animations (based on the rgb_ring example, led-strip alias). */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include "ring.h"
#include "game_config.h"

#define STRIP_NODE DT_ALIAS(led_strip)
#define NUM_PIXELS DT_PROP(STRIP_NODE, chain_length)

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);
static struct led_rgb pixels[NUM_PIXELS];
static bool ready;

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
	rgb.r = (uint8_t)((rgb.r * RING_BRIGHTNESS) / 255U);
	rgb.g = (uint8_t)((rgb.g * RING_BRIGHTNESS) / 255U);
	rgb.b = (uint8_t)((rgb.b * RING_BRIGHTNESS) / 255U);
	return rgb;
}

static void show(void)
{
	if (ready) {
		(void)led_strip_update_rgb(strip, pixels, NUM_PIXELS);
	}
}

static void fill(struct led_rgb c)
{
	for (size_t i = 0; i < NUM_PIXELS; i++) {
		pixels[i] = c;
	}
	show();
}

int ring_init(void)
{
	ready = device_is_ready(strip);
	ring_off();
	return ready ? 0 : -ENODEV;
}

void ring_off(void)
{
	fill((struct led_rgb){0});
}

void ring_countdown(uint32_t permille_left)
{
	uint32_t lit = (NUM_PIXELS * MIN(permille_left, 1000U) + 999U) / 1000U;
	struct led_rgb c = {.r = RING_BRIGHTNESS / 4U, .g = RING_BRIGHTNESS / 4U,
			    .b = RING_BRIGHTNESS / 4U};

	for (size_t i = 0; i < NUM_PIXELS; i++) {
		pixels[i] = (i < lit) ? c : (struct led_rgb){0};
	}
	show();
}

void ring_rainbow_flash(void)
{
	/* Flashing, spinning rainbow: 6 on/off flashes, ~600 ms total */
	for (uint8_t f = 0; f < 6U; f++) {
		for (size_t i = 0; i < NUM_PIXELS; i++) {
			pixels[i] = color_wheel((uint8_t)(f * 40U + (i * 256U) / NUM_PIXELS));
		}
		show();
		k_msleep(70);
		ring_off();
		k_msleep(30);
	}
}

void ring_fail(void)
{
	for (int f = 0; f < 3; f++) {
		fill((struct led_rgb){.r = RING_BRIGHTNESS});
		k_msleep(150);
		ring_off();
		k_msleep(100);
	}
}

void ring_idle_step(void)
{
	static uint8_t offset;

	for (size_t i = 0; i < NUM_PIXELS; i++) {
		struct led_rgb c = color_wheel((uint8_t)(offset + (i * 256U) / NUM_PIXELS));

		pixels[i] = (struct led_rgb){.r = c.r / 4U, .g = c.g / 4U, .b = c.b / 4U};
	}
	offset += 4U;
	show();
}
