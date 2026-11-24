/* SPDX-License-Identifier: Apache-2.0 */
/* All game tunables in one place. Thresholds are untested guesses - tune on hardware. */
#ifndef BOPIT_GAME_CONFIG_H_
#define BOPIT_GAME_CONFIG_H_

/* Round timing */
#define ROUND_TIME_START_MS     4000U
#define ROUND_TIME_STEP_MS       150U   /* subtracted per point scored */
#define ROUND_TIME_MIN_MS       1200U
#define ROUND_SETTLE_MS          250U   /* inputs ignored right after arming */
#define GAME_OVER_LOCKOUT_MS    1500U   /* before "any input" restarts */
#define POLL_PERIOD_MS            20U
#define OLED_BAR_REFRESH_MS      200U

/* Cover it! - VCNL4200 ambient light must DROP by >= 50 % (and >= 20 counts) */
#define COVER_ABS_COUNTS          20
#define COVER_PCT                 50
/* Move it! - VCNL4200 proximity count must RISE by >= 30 counts or 50 % */
#define MOVE_ABS_COUNTS           30
#define MOVE_PCT                  50
/* Twist it! - potentiometer change of ~10 % of the 12-bit range */
#define TWIST_ABS_COUNTS         400
/* Warm it! - MCP9808 rise in 1/16 degC (6 = 0.375 degC) */
#define WARM_ABS_SIXTEENTHS        6
#define WARM_EXTRA_TIME_MS      3000U
/* Say it! - microphone peak-to-peak level rise (64-sample window) */
#define SAY_ABS_COUNTS           200
#define SAY_PCT                  100
#define SAY_WINDOW_SAMPLES        64U
/* Twirl it! - all four joystick directions */
#define TWIRL_EXTRA_TIME_MS     1500U

/* RGB ring */
#define RING_BRIGHTNESS           32U

#endif
