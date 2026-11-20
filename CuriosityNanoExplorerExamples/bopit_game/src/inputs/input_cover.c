/* SPDX-License-Identifier: Apache-2.0 */
/* "Cover it!" - VCNL4200 ambient light drops when the sensor is covered. */
#include <zephyr/kernel.h>
#include "input.h"
#include "detector.h"
#include "hw.h"
#include "game_config.h"

static struct detector det = {
	.dir = DETECT_FALL, .abs_threshold = COVER_ABS_COUNTS,
	.pct_threshold = COVER_PCT, .hits_needed = 2,
};

static int init(void)
{
	return vcnl4200_init(); /* idempotent, shared with "Move it!" */
}

static void arm(void)
{
	uint16_t v = 0;

	(void)vcnl4200_read_als(&v);
	detector_arm(&det, v);
}

static enum input_state poll(void)
{
	uint16_t v;

	if (vcnl4200_read_als(&v) != 0) {
		return INPUT_IDLE;
	}
	return detector_update(&det, v) ? INPUT_COMPLETE : INPUT_IDLE;
}

const struct bopit_input input_cover = {
	.verb = "COVER",
	/* Covering the sensor also raises the proximity count (same chip) */
	.ignore_mask = BIT(INPUT_MOVE),
	.can_fail = true,
	.init = init, .arm = arm, .poll = poll,
};
