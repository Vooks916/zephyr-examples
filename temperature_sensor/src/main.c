#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>

static const struct i2c_dt_spec temperature_sensor = I2C_DT_SPEC_GET(DT_ALIAS(temperature_sensor));

int main(void)
{
	printk("Temperature Sensor Application on board: %s\n", CONFIG_BOARD_TARGET);

	printk("Temperature Sensor I2C address: %hu\n", temperature_sensor.addr);

	return 0;
}
