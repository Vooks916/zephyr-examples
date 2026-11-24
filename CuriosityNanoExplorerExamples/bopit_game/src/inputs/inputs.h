/* SPDX-License-Identifier: Apache-2.0 */
#ifndef BOPIT_INPUTS_H_
#define BOPIT_INPUTS_H_

#include "input.h"

/* Initialise every input; returns BIT(id) mask of working inputs. */
uint32_t inputs_init(void);
uint32_t inputs_available(void);
const struct bopit_input *inputs_get(enum bopit_input_id id);
/* Capture a fresh baseline for every available input. */
void inputs_arm_all(void);
void inputs_poll_all(enum input_state states[INPUT_COUNT]);

#endif
