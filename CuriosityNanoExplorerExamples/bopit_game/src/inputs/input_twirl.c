/* SPDX-License-Identifier: Apache-2.0 */
/*
 * "Twirl it!" - joystick pushed up, down, left and right (each at least once
 * per round). A single direction or the center push is reported as ACTIVITY,
 * which counts as a wrong action when another instruction is shown and as
 * "any input" on the start / game over screens.
 */
#include <zephyr/kernel.h>
#include "input.h"
#include "hw.h"
#include "game_config.h"

#define JOY_ALL (IOEXP_JOY_DIRS | IOEXP_JOY_PUSH)

static uint8_t prev, seen;
static bool active;

static int init(void)
{
	return ioexp_init();
}

static void arm(void)
{
	int p = ioexp_read_pressed();

	prev = (p < 0) ? JOY_ALL : (p & JOY_ALL);
	seen = 0;
	active = false;
}

static enum input_state poll(void)
{
	int p = ioexp_read_pressed();
	uint8_t now, edges;

	if (p < 0) {
		return active ? INPUT_ACTIVITY : INPUT_IDLE;
	}
	now = p & JOY_ALL;
	edges = now & ~prev;
	prev = now;

	if (edges) {
		active = true;
		seen |= edges & IOEXP_JOY_DIRS;
	}
	if (seen == IOEXP_JOY_DIRS) {
		return INPUT_COMPLETE;
	}
	return active ? INPUT_ACTIVITY : INPUT_IDLE;
}

const struct bopit_input input_twirl = {
	.verb = "TWIRL",
	.extra_time_ms = TWIRL_EXTRA_TIME_MS,
	.can_fail = true,
	.init = init, .arm = arm, .poll = poll,
};
