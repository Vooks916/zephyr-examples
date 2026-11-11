/*
 * Curiosity Nano Explorer speaker test: MCP4821 DAC -> Class-D amp -> 8 ohm speaker.
 * Alternates 440 Hz / 880 Hz square waves (~500 ms each) forever.
 * Pattern follows samples/drivers/dac/src/main.c.
 *
 * Timing note: half-periods use k_busy_wait() plus SPI-write overhead, so the
 * pitch will be slightly flat. k_busy_wait() accuracy depends on
 * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC being correct for the board.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/gpio.h>

#define DAC_NODE        DT_ALIAS(speaker_dac)
#define SPK_EN_NODE     DT_ALIAS(speaker_enable)
#define DAC_CHANNEL_ID  0
#define DAC_RESOLUTION  12

#if !DT_NODE_HAS_STATUS(DAC_NODE, okay)
#error "speaker-dac alias missing: use boards/<board>.overlay"
#endif

#define DAC_HIGH  3072U /* 75 % of 4096 */
#define DAC_LOW   1024U /* 25 % of 4096 */
#define DAC_MID   2048U
#define TONE_MS   500U

static const struct device *const dac_dev = DEVICE_DT_GET(DAC_NODE);

#if DT_NODE_EXISTS(SPK_EN_NODE)
static const struct gpio_dt_spec spk_en = GPIO_DT_SPEC_GET(SPK_EN_NODE, gpios);
#endif

static const struct dac_channel_cfg dac_ch_cfg = {
	.channel_id = DAC_CHANNEL_ID,
	.resolution = DAC_RESOLUTION,
	.buffered = true,
};

static int play_tone(uint32_t freq_hz, uint32_t duration_ms)
{
	const uint32_t half_us = 1000000U / (2U * freq_hz); /* 440 -> 1136, 880 -> 568 */
	const uint32_t cycles = (duration_ms * 1000U) / (2U * half_us); /* ~220 / ~440 */
	int ret;

	for (uint32_t i = 0; i < cycles; i++) {
		ret = dac_write_value(dac_dev, DAC_CHANNEL_ID, DAC_HIGH);
		if (ret) {
			return ret;
		}
		k_busy_wait(half_us);
		ret = dac_write_value(dac_dev, DAC_CHANNEL_ID, DAC_LOW);
		if (ret) {
			return ret;
		}
		k_busy_wait(half_us);
	}
	return 0;
}

int main(void)
{
	int ret;

	printk("CNANO Explorer MCP4821 speaker test\n");

#if DT_NODE_EXISTS(SPK_EN_NODE)
	/* UNVERIFIED whether IO 12 must be driven or the amp is enabled by
	 * hardware alone; drive it HIGH to be safe.
	 */
	if (!gpio_is_ready_dt(&spk_en)) {
		printk("speaker-enable GPIO not ready\n");
		return 0;
	}
	ret = gpio_pin_configure_dt(&spk_en, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		printk("speaker-enable configure failed: %d\n", ret);
		return 0;
	}
	printk("SPEAKER_ENABLE (IO 12) driven HIGH\n");
#endif

	if (!device_is_ready(dac_dev)) {
		printk("DAC device %s is not ready\n", dac_dev->name);
		return 0;
	}

	ret = dac_channel_setup(dac_dev, &dac_ch_cfg);
	if (ret) {
		printk("dac_channel_setup failed: %d\n", ret);
		return 0;
	}
	(void)dac_write_value(dac_dev, DAC_CHANNEL_ID, DAC_MID);

	while (1) {
		printk("Tone: 440 Hz\n");
		ret = play_tone(440, TONE_MS);
		if (ret) {
			break;
		}
		printk("Tone: 880 Hz\n");
		ret = play_tone(880, TONE_MS);
		if (ret) {
			break;
		}
	}

	printk("dac_write_value failed: %d\n", ret);
	return 0;
}
