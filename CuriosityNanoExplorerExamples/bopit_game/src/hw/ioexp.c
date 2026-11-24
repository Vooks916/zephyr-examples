/* SPDX-License-Identifier: Apache-2.0 */
/*
 * MCP23008 (I/O Expander 2) read directly over I2C, as in joystick_oled and
 * snake_game. The capacitive_touch example used Zephyr's MCP23008 GPIO driver
 * + gpio-keys for the same chip; using both would give the chip two owners,
 * so the game reads all 8 inputs here and both the joystick and touch modules
 * share this helper (see README).
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include "hw.h"

#define MCP_IODIR 0x00
#define MCP_GPPU  0x06
#define MCP_GPIO  0x09

static const struct i2c_dt_spec ioexp = I2C_DT_SPEC_GET(DT_NODELABEL(ioexp2));

int ioexp_init(void)
{
	int ret;

	if (!i2c_is_ready_dt(&ioexp)) {
		return -ENODEV;
	}
	ret = i2c_reg_write_byte_dt(&ioexp, MCP_IODIR, 0xFF); /* all inputs */
	if (ret == 0) {
		ret = i2c_reg_write_byte_dt(&ioexp, MCP_GPPU, 0xFF); /* pull-ups on */
	}
	return ret;
}

int ioexp_read_pressed(void)
{
	uint8_t val;
	int ret = i2c_reg_read_byte_dt(&ioexp, MCP_GPIO, &val);

	return ret ? ret : (uint8_t)~val; /* switches are active-low */
}
