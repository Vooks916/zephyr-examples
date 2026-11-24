/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Audio extension point. All functions are currently no-ops (see audio.c and
 * README "Audio extension"). Calls are non-blocking by contract so a real
 * implementation must run playback in its own thread / timer + DMA.
 */
#ifndef BOPIT_AUDIO_H_
#define BOPIT_AUDIO_H_

#include "input.h"

int audio_init(void);
void audio_start_loop(void);                       /* repetitive percussion backing track */
void audio_stop_loop(void);
void audio_play_instruction(enum bopit_input_id id); /* "Twist it!" voice line / sound */
void audio_play_success(void);
void audio_play_start(void);                       /* start jingle */
void audio_play_end(void);                         /* game over jingle */

#endif
