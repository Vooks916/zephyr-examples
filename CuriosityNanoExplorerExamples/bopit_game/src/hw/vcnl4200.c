/* SPDX-License-Identifier: Apache-2.0 */
/*
 * VCNL4200 helper merged from ambient_light (ALS) and proximity_sensor (PS).
 * Zephyr has no VCNL4200 driver; registers are 16 bit, low byte first.
 * Both "Cover it!" and "Move it!" use this one chip.
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include "hw.h"

#define REG_ALS_CONF  0x00
#define REG_PS_CONF1  0x03   /* PS_CONF1 (low byte) / PS_CONF2 (high byte) */
#define REG_PS_DATA   0x08
#define REG_ALS_DATA  0x09
#define REG_ID        0x0E
#define ID_VALUE      0x1058U

static const struct i2c_dt_spec vcnl = I2C_DT_SPEC_GET(DT_NODELABEL(vcnl4200));

static int reg_read(uint8_t reg, uint16_t *value)
{
	uint8_t data[2];
	int ret = i2c_write_read_dt(&vcnl, &reg, 1, data, sizeof(data));

	if (ret == 0) {
		*value = sys_get_le16(data);
	}
	return ret;
}

static int reg_write(uint8_t reg, uint16_t value)
{
	uint8_t data[3] = {reg, value & 0xFFU, value >> 8};

	return i2c_write_dt(&vcnl, data, sizeof(data));
}

int vcnl4200_init(void)
{
	uint16_t id;
	int ret;

	if (!i2c_is_ready_dt(&vcnl)) {
		return -ENODEV;
	}
	ret = reg_read(REG_ID, &id);
	if (ret != 0) {
		return ret;
	}
	if (id != ID_VALUE) {
		printk("Unexpected VCNL4200 ID 0x%04x\n", id);
		return -ENODEV;
	}
	/* ALS on, 50 ms integration (ambient_light example) */
	ret = reg_write(REG_ALS_CONF, 0x0000U);
	if (ret == 0) {
		/* PS on, default duty/integration (proximity_sensor example) */
		ret = reg_write(REG_PS_CONF1, 0x0000U);
	}
	return ret;
}

int vcnl4200_read_als(uint16_t *value)
{
	return reg_read(REG_ALS_DATA, value);
}

int vcnl4200_read_ps(uint16_t *value)
{
	return reg_read(REG_PS_DATA, value);
}
