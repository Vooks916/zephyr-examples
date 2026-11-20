/* SPDX-License-Identifier: Apache-2.0 */
#ifndef BOPIT_UI_H_
#define BOPIT_UI_H_

#include <stdint.h>

int ui_init(void);
void ui_show_start(uint32_t missing_inputs);
/* Big "<VERB>" / "IT!" with score and a time bar (permille 0..1000 left). */
void ui_show_instruction(const char *verb, uint32_t score, uint32_t permille_left);
void ui_show_game_over(uint32_t score, uint32_t best, const char *reason);

#endif
