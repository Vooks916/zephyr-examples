#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>

static const struct pwm_dt_spec red_pwm_led =
	PWM_DT_SPEC_GET(DT_ALIAS(red_pwm_led));
static const struct pwm_dt_spec green_pwm_led =
	PWM_DT_SPEC_GET(DT_ALIAS(green_pwm_led));
static const struct pwm_dt_spec blue_pwm_led =
	PWM_DT_SPEC_GET(DT_ALIAS(blue_pwm_led));

#define HUE_STEPS   768   /* 6 segments x 128 steps */
#define SEG_STEPS   128
#define DELAY_MS    20    /* total cycle = 768 * 20 ms ≈ 15 s */

static int set_color(const struct pwm_dt_spec *led, uint32_t level)
{
	/* level: 0..SEG_STEPS */
	uint32_t pulse = (uint32_t)(((uint64_t)led->period * level) / SEG_STEPS);

	return pwm_set_pulse_dt(led, pulse);
}

int main(void)
{
	int ret;

	printk("Smooth RGB transition\n");

	if (!pwm_is_ready_dt(&red_pwm_led) ||
	    !pwm_is_ready_dt(&green_pwm_led) ||
	    !pwm_is_ready_dt(&blue_pwm_led)) {
		printk("Error: one or more PWM devices not ready\n");
		return 0;
	}

	while (1) {
		for (int h = 0; h < HUE_STEPS; h++) {
			int seg = h / SEG_STEPS;
			uint32_t up = h % SEG_STEPS;
			uint32_t down = SEG_STEPS - up;
			uint32_t r, g, b;

			switch (seg) {
			case 0: r = SEG_STEPS; g = up;        b = 0;         break; /* red -> yellow */
			case 1: r = down;      g = SEG_STEPS; b = 0;         break; /* yellow -> green */
			case 2: r = 0;         g = SEG_STEPS; b = up;        break; /* green -> cyan */
			case 3: r = 0;         g = down;      b = SEG_STEPS; break; /* cyan -> blue */
			case 4: r = up;        g = 0;         b = SEG_STEPS; break; /* blue -> magenta */
			default: r = SEG_STEPS; g = 0;        b = down;      break; /* magenta -> red */
			}

			ret = set_color(&red_pwm_led, r);
			if (ret == 0) { ret = set_color(&green_pwm_led, g); }
			if (ret == 0) { ret = set_color(&blue_pwm_led, b); }
			if (ret != 0) {
				printk("Error %d: PWM write failed\n", ret);
				return 0;
			}

			k_sleep(K_MSEC(DELAY_MS));
		}
	}

	return 0;
}