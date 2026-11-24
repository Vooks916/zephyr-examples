/* SPDX-License-Identifier: Apache-2.0 */
/* "Twist it!" - potentiometer (ADC AIN7) moved significantly in either direction. */
#include <zephyr/kernel.h>
#include "input.h"
#include "detector.h"
#include "hw.h"
#include "game_config.h"

static struct detector det = {
	.dir = DETECT_EITHER, .abs_threshold = TWIST_ABS_COUNTS, .hits_needed = 2,
};

static int init(void)
{
	return analog_init();
}

static void arm(void)
{
	int32_t v = 0;

	(void)analog_read_pot(&v);
	detector_arm(&det, v);
}

static enum input_state poll(void)
{
	int32_t v;

	if (analog_read_pot(&v) != 0) {
		return INPUT_IDLE;
	}
	return detector_update(&det, v) ? INPUT_COMPLETE : INPUT_IDLE;
}

const struct bopit_input input_twist = {
	.verb = "TWIST",
	.can_fail = true,
	.init = init, .arm = arm, .poll = poll,
};
