/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Potentiometer (potentiometer_reading example, AIN7) and microphone
 * (microphone branch, AIN6) share ONE SAM ADC.
 *
 * Conflict: the examples use different references (pot: ADC_REF_INTERNAL,
 * mic: ADC_REF_VDD_1_2 on SAMD21 / ADC_REF_VDD_1 on SAME51). Zephyr's
 * adc_sam0 driver writes the global REFCTRL and INPUTCTRL registers in
 * adc_channel_setup() and only converts one channel per sequence, so the
 * channel is re-configured before every read of the "other" channel. Each
 * input therefore keeps exactly the configuration of its original example.
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include "hw.h"

#define USER_NODE DT_PATH(zephyr_user)

#if !DT_NODE_HAS_PROP(USER_NODE, io_channels)
#error "zephyr,user io-channels missing: use boards/<board>.overlay"
#endif

static const struct adc_dt_spec pot = ADC_DT_SPEC_GET_BY_NAME(USER_NODE, potentiometer);
static const struct adc_dt_spec mic = ADC_DT_SPEC_GET_BY_NAME(USER_NODE, microphone);
static const struct adc_dt_spec *active;

static int select_channel(const struct adc_dt_spec *spec)
{
	int ret = 0;

	if (active != spec) {
		ret = adc_channel_setup_dt(spec);
		active = (ret == 0) ? spec : NULL;
	}
	return ret;
}

static int read_one(const struct adc_dt_spec *spec, uint16_t *sample)
{
	struct adc_sequence seq = {.buffer = sample, .buffer_size = sizeof(*sample)};

	adc_sequence_init_dt(spec, &seq);
	return adc_read_dt(spec, &seq);
}

int analog_init(void)
{
	if (!adc_is_ready_dt(&pot) || !adc_is_ready_dt(&mic)) {
		return -ENODEV;
	}
	return select_channel(&pot);
}

int analog_read_pot(int32_t *value)
{
	uint16_t sample;
	int ret = select_channel(&pot);

	if (ret == 0) {
		ret = read_one(&pot, &sample);
	}
	if (ret == 0) {
		*value = sample;
	}
	return ret;
}

int analog_read_mic_level(uint32_t samples, int32_t *level)
{
	uint16_t sample, lo = UINT16_MAX, hi = 0U;
	int ret = select_channel(&mic);

	for (uint32_t i = 0; ret == 0 && i < samples; i++) {
		ret = read_one(&mic, &sample);
		lo = MIN(lo, sample);
		hi = MAX(hi, sample);
	}
	if (ret == 0) {
		*level = hi - lo;
	}
	return ret;
}
