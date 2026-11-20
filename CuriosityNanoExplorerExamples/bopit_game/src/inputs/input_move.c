/* SPDX-License-Identifier: Apache-2.0 */
/* "Move it!" - VCNL4200 proximity count rises when a hand moves in front. */
#include <zephyr/kernel.h>
#include "input.h"
#include "detector.h"
#include "hw.h"
#include "game_config.h"

static struct detector det = {
	.dir = DETECT_RISE, .abs_threshold = MOVE_ABS_COUNTS,
	.pct_threshold = MOVE_PCT, .hits_needed = 2,
};

static int init(void)
{
	return vcnl4200_init();
}

static void arm(void)
{
	uint16_t v = 0;

	(void)vcnl4200_read_ps(&v);
	detector_arm(&det, v);
}

static enum input_state poll(void)
{
	uint16_t v;

	if (vcnl4200_read_ps(&v) != 0) {
		return INPUT_IDLE;
	}
	return detector_update(&det, v) ? INPUT_COMPLETE : INPUT_IDLE;
}

const struct bopit_input input_move = {
	.verb = "MOVE",
	.ignore_mask = BIT(INPUT_COVER),
	/*
	 * Hands reaching for the joystick/pot/touch pads pass in front of the
	 * sensor, so proximity is NOT a wrong action (assumption, see README).
	 */
	.can_fail = false,
	.init = init, .arm = arm, .poll = poll,
};
