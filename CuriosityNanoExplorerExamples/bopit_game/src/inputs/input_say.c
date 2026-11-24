/* SPDX-License-Identifier: Apache-2.0 */
/*
 * "Say it!" - microphone (ADC AIN6, approach from the `microphone` branch):
 * the peak-to-peak level of a 64-sample window must rise above the ambient
 * level captured at round start.
 */
#include <zephyr/kernel.h>
#include "input.h"
#include "detector.h"
#include "hw.h"
#include "game_config.h"

static struct detector det = {
	.dir = DETECT_RISE, .abs_threshold = SAY_ABS_COUNTS,
	.pct_threshold = SAY_PCT, .hits_needed = 2,
};

static int init(void)
{
	return analog_init();
}

static void arm(void)
{
	int32_t level = 0, sum = 0;

	/* average a few windows for a stable ambient baseline */
	for (int i = 0; i < 4; i++) {
		(void)analog_read_mic_level(SAY_WINDOW_SAMPLES, &level);
		sum += level;
	}
	detector_arm(&det, sum / 4);
}

static enum input_state poll(void)
{
	int32_t level;

	if (analog_read_mic_level(SAY_WINDOW_SAMPLES, &level) != 0) {
		return INPUT_IDLE;
	}
	return detector_update(&det, level) ? INPUT_COMPLETE : INPUT_IDLE;
}

const struct bopit_input input_say = {
	.verb = "SAY",
	.extra_time_ms = 500U,
	/*
	 * Clicking the joystick or tapping the board is picked up by the MEMS
	 * microphone, so sound never counts as a wrong action (assumption).
	 */
	.can_fail = false,
	.init = init, .arm = arm, .poll = poll,
};
