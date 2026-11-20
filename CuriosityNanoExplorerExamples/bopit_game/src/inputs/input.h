/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Common interface for every Bop It! input. To add an input: write a module
 * that fills a struct bopit_input, add an id to enum bopit_input_id and add
 * the descriptor to the registry in inputs.c.
 */
#ifndef BOPIT_INPUT_H_
#define BOPIT_INPUT_H_

#include <stdbool.h>
#include <stdint.h>

enum bopit_input_id {
	INPUT_COVER,
	INPUT_TOUCH,
	INPUT_TWIRL,
	INPUT_TWIST,
	INPUT_MOVE,
	INPUT_WARM,
	INPUT_SAY,
	INPUT_COUNT
};

enum input_state {
	INPUT_IDLE = 0,     /* nothing happened since arm() */
	INPUT_ACTIVITY,     /* player touched this input but the action is not complete */
	INPUT_COMPLETE,     /* the requested action was performed */
};

struct bopit_input {
	const char *verb;          /* big OLED word, e.g. "COVER" -> "COVER IT!" */
	uint32_t extra_time_ms;    /* added to the round time limit (slow sensors) */
	uint32_t ignore_mask;      /* BIT(id) of inputs NOT counted as wrong while this is the target */
	bool can_fail;             /* may this input end the game as a "wrong action"? */
	int (*init)(void);         /* 0 = available */
	void (*arm)(void);         /* capture baseline at round start */
	enum input_state (*poll)(void);
};

extern const struct bopit_input input_cover, input_touch, input_twirl,
	input_twist, input_move, input_warm, input_say;

#endif
