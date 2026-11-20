/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Bop It! for the Curiosity Nano Explorer (SAM D21 / SAM E51 Curiosity Nano).
 * An instruction is shown on the OLED; perform it before the time runs out.
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "game.h"
#include "game_config.h"
#include "inputs.h"
#include "ui.h"
#include "ring.h"
#include "audio.h"

int main(void)
{
	uint32_t avail;

	printk("Explorer Bop It! game\n");

	if (ui_init() != 0) {
		return 0;
	}
	if (ring_init() != 0) {
		printk("RGB ring not ready - playing without LEDs\n");
	}
	(void)audio_init();

	avail = inputs_init();
	if (avail == 0U) {
		printk("No inputs available\n");
		ui_show_game_over(0, 0, "No sensors!");
		return 0;
	}

	game_init(BIT_MASK(INPUT_COUNT) & ~avail);
	while (true) {
		game_step();
		k_msleep(POLL_PERIOD_MS);
	}
	return 0;
}
