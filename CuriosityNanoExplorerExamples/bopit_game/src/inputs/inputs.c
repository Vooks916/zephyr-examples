/* SPDX-License-Identifier: Apache-2.0 */
/* Input registry: the only place that needs a new entry when adding an input. */
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "inputs.h"

static const struct bopit_input *const registry[INPUT_COUNT] = {
	[INPUT_COVER] = &input_cover,
	[INPUT_TOUCH] = &input_touch,
	[INPUT_TWIRL] = &input_twirl,
	[INPUT_TWIST] = &input_twist,
	[INPUT_MOVE] = &input_move,
	[INPUT_WARM] = &input_warm,
	[INPUT_SAY] = &input_say,
};

static uint32_t available;

uint32_t inputs_init(void)
{
	available = 0;
	for (int i = 0; i < INPUT_COUNT; i++) {
		int ret = registry[i]->init();

		if (ret == 0) {
			available |= BIT(i);
		} else {
			printk("Input %s unavailable (%d)\n", registry[i]->verb, ret);
		}
	}
	return available;
}

uint32_t inputs_available(void)
{
	return available;
}

const struct bopit_input *inputs_get(enum bopit_input_id id)
{
	return registry[id];
}

void inputs_arm_all(void)
{
	for (int i = 0; i < INPUT_COUNT; i++) {
		if (available & BIT(i)) {
			registry[i]->arm();
		}
	}
}

void inputs_poll_all(enum input_state states[INPUT_COUNT])
{
	for (int i = 0; i < INPUT_COUNT; i++) {
		states[i] = (available & BIT(i)) ? registry[i]->poll() : INPUT_IDLE;
	}
}
