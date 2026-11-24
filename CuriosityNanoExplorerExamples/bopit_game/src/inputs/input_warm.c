/* SPDX-License-Identifier: Apache-2.0 */
/*
 * "Warm it!" - MCP9808 temperature rises (thumb on the sensor).
 * Reads the full 1/16 degC resolution. Note: the temperature_sensor example
 * treats bit 5 of the upper byte as the sign; per the MCP9808 datasheet the
 * sign is bit 4 (bits 7..5 are alert flags), which is used here.
 * Rise-only detection, so a sensor cooling down after a previous "Warm it!"
 * round never triggers.
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include "input.h"
#include "detector.h"
#include "game_config.h"

#define MCP9808_REG_TA 0x05
#define SAMPLE_MS      250   /* sensor conversion time at 0.0625 degC resolution */

static const struct i2c_dt_spec tsens = I2C_DT_SPEC_GET(DT_ALIAS(temperature_sensor));
static struct detector det = {
	.dir = DETECT_RISE, .abs_threshold = WARM_ABS_SIXTEENTHS, .hits_needed = 1,
};
static int64_t next_sample;
static bool triggered;

static int read_sixteenths(int32_t *t)
{
	uint8_t reg = MCP9808_REG_TA, buf[2];
	int ret = i2c_write_read_dt(&tsens, &reg, 1, buf, sizeof(buf));

	if (ret == 0) {
		int32_t raw = ((buf[0] & 0x0F) << 8) | buf[1];

		*t = (buf[0] & 0x10) ? raw - 4096 : raw;
	}
	return ret;
}

static int init(void)
{
	int32_t t;

	return i2c_is_ready_dt(&tsens) ? read_sixteenths(&t) : -ENODEV;
}

static void arm(void)
{
	int32_t t = 0;

	(void)read_sixteenths(&t);
	detector_arm(&det, t);
	triggered = false;
	next_sample = k_uptime_get() + SAMPLE_MS;
}

static enum input_state poll(void)
{
	int32_t t;

	if (!triggered && k_uptime_get() >= next_sample) {
		next_sample = k_uptime_get() + SAMPLE_MS;
		if (read_sixteenths(&t) == 0) {
			triggered = detector_update(&det, t);
		}
	}
	return triggered ? INPUT_COMPLETE : INPUT_IDLE;
}

const struct bopit_input input_warm = {
	.verb = "WARM",
	.extra_time_ms = WARM_EXTRA_TIME_MS,
	/* reaching for the sensor may pass the optical sensor */
	.ignore_mask = BIT(INPUT_COVER) | BIT(INPUT_MOVE),
	.can_fail = true,
	.init = init, .arm = arm, .poll = poll,
};
