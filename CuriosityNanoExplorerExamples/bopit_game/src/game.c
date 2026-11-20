/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Bop It! state machine (single thread, polled every POLL_PERIOD_MS):
 *
 *   START --any input--> ROUND --target COMPLETE--> SUCCESS --> ROUND ...
 *                          |--wrong input / timeout--> GAME_OVER --any input--> ROUND
 */
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/printk.h>
#include "game.h"
#include "game_config.h"
#include "inputs.h"
#include "ui.h"
#include "ring.h"
#include "audio.h"

enum game_state { STATE_START, STATE_ROUND, STATE_GAME_OVER };

static enum game_state state;
static enum bopit_input_id target;
static int prev_target = -1;
static uint32_t score, best;
static int64_t round_start, round_deadline, armed_at, next_redraw;
static uint32_t rng_mix;

static uint32_t round_time_ms(void)
{
        uint32_t t = ROUND_TIME_START_MS;
        uint32_t dec = score * ROUND_TIME_STEP_MS;

        t = (dec < t - ROUND_TIME_MIN_MS) ? t - dec : ROUND_TIME_MIN_MS;
        return t + inputs_get(target)->extra_time_ms;
}

static enum bopit_input_id pick_target(void)
{
        uint32_t avail = inputs_available();
        uint32_t n = 0;
        enum bopit_input_id ids[INPUT_COUNT];

        for (int i = 0; i < INPUT_COUNT; i++) {
                /* avoid the same instruction twice in a row when possible */
                if ((avail & BIT(i)) && (i != prev_target || __builtin_popcount(avail) == 1)) {
                        ids[n++] = (enum bopit_input_id)i;
                }
        }
        return ids[(sys_rand32_get() ^ rng_mix) % n];
}

/* Any activity on any input (used to start / restart). */
static bool any_activity(const enum input_state st[INPUT_COUNT])
{
        for (int i = 0; i < INPUT_COUNT; i++) {
                if (st[i] != INPUT_IDLE) {
                        return true;
                }
        }
        return false;
}

static void arm(void)
{
        inputs_arm_all();
        armed_at = k_uptime_get();
}

static uint32_t permille_left(int64_t now)
{
        int64_t total = round_deadline - round_start;
        int64_t left = round_deadline - now;

        return (left <= 0 || total <= 0) ? 0U : (uint32_t)((left * 1000) / total);
}

static void start_round(void)
{
        target = pick_target();
        prev_target = target;
        arm();
        round_start = k_uptime_get();
        round_deadline = round_start + round_time_ms();
        next_redraw = 0;
        printk("Round %u: %s IT! (%u ms)\n", score + 1, inputs_get(target)->verb,
               (uint32_t)(round_deadline - round_start));
        audio_play_instruction(target);
}

static void new_game(void)
{
        rng_mix ^= k_cycle_get_32(); /* player timing adds entropy */
        score = 0;
        prev_target = -1;
        audio_play_start();
        audio_start_loop();
        state = STATE_ROUND;
        start_round();
}

static void game_over(const char *reason)
{
        printk("Game over (%s), score %u\n", reason, score);
        best = MAX(best, score);
        audio_stop_loop();
        audio_play_end();
        ui_show_game_over(score, best, reason);
        ring_fail();
        k_msleep(GAME_OVER_LOCKOUT_MS);
        arm();
        state = STATE_GAME_OVER;
}

void game_init(uint32_t missing_inputs)
{
        state = STATE_START;
        ui_show_start(missing_inputs);
        arm();
}

void game_step(void)
{
        enum input_state st[INPUT_COUNT];
        int64_t now = k_uptime_get();

        inputs_poll_all(st);
        if (now - armed_at < ROUND_SETTLE_MS) {
                return; /* let sensors settle after the baseline was captured */
        }

        switch (state) {
        case STATE_START:
        case STATE_GAME_OVER:
                ring_idle_step();
                if (any_activity(st)) {
                        new_game();
                }
                break;

        case STATE_ROUND: {
                const struct bopit_input *t = inputs_get(target);

                if (st[target] == INPUT_COMPLETE) {
                        score++;
                        audio_play_success();
                        ring_rainbow_flash();
                        start_round();
                        break;
                }
                for (int i = 0; i < INPUT_COUNT; i++) {
                        if (i != (int)target && st[i] != INPUT_IDLE && inputs_get(i)->can_fail &&
                            !(t->ignore_mask & BIT(i))) {
                                game_over("Wrong move!");
                                return;
                        }
                }
                if (now >= round_deadline) {
                        game_over("Too slow!");
                        break;
                }
                if (now >= next_redraw) {
                        uint32_t left = permille_left(now);

                        ui_show_instruction(t->verb, score, left);
                        ring_countdown(left);
                        next_redraw = now + OLED_BAR_REFRESH_MS;
                }
                break;
        }
        }
}
