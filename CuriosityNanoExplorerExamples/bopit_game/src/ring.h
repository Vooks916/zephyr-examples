/* SPDX-License-Identifier: Apache-2.0 */
#ifndef BOPIT_RING_H_
#define BOPIT_RING_H_

#include <stdint.h>

int ring_init(void);
void ring_off(void);
void ring_countdown(uint32_t permille_left);  /* dim white segments = time left */
void ring_rainbow_flash(void);                 /* correct action (blocking ~600 ms) */
void ring_fail(void);                          /* game over (blocking ~750 ms) */
void ring_idle_step(void);                     /* slow dim rainbow on start/game-over screens */

#endif
