/* SPDX-License-Identifier: Apache-2.0 */
/*
 * No-op audio stubs. To implement, follow the `speaker` example
 * (MCP4821 SPI DAC -> class-D amp, SPEAKER ENABLE GPIO): copy
 * dts/bindings/microchip,mcp4821.yaml and src/dac_mcp4821.c, uncomment the
 * speaker nodes in boards/<board>.overlay, add CONFIG_SPI/CONFIG_DAC and
 * replace these bodies with a mixer thread fed by a k_timer (see README).
 */
#include <zephyr/kernel.h>
#include "audio.h"

int audio_init(void)
{
	return 0;
}

void audio_start_loop(void)
{
}

void audio_stop_loop(void)
{
}

void audio_play_instruction(enum bopit_input_id id)
{
	ARG_UNUSED(id);
}

void audio_play_success(void)
{
}

void audio_play_start(void)
{
}

void audio_play_end(void)
{
}
