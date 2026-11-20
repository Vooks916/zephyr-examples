/* SPDX-License-Identifier: Apache-2.0 */
/* "Touch it!" - any capacitive touch pad (SW1..SW3 on MCP23008 GP5..GP7). */
#include <zephyr/kernel.h>
#include "input.h"
#include "hw.h"

static uint8_t prev;

static int init(void)
{
	return ioexp_init();
}

static void arm(void)
{
	int p = ioexp_read_pressed();

	/* pads already held at round start must be released first */
	prev = (p < 0) ? IOEXP_TOUCH_ALL : (p & IOEXP_TOUCH_ALL);
}

static enum input_state poll(void)
{
	int p = ioexp_read_pressed();
	uint8_t now, edges;

	if (p < 0) {
		return INPUT_IDLE;
	}
	now = p & IOEXP_TOUCH_ALL;
	edges = now & ~prev;
	prev = now;
	return edges ? INPUT_COMPLETE : INPUT_IDLE;
}

const struct bopit_input input_touch = {
	.verb = "TOUCH",
	.can_fail = true,
	.init = init, .arm = arm, .poll = poll,
};
