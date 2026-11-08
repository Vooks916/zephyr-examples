#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>

#if !DT_NODE_EXISTS(DT_NODELABEL(vcnl4200)) || \
	!DT_NODE_EXISTS(DT_NODELABEL(ssd1306))
#error "No suitable devicetree overlay specified"
#endif

#define VCNL4200_ADDR      0x51 
#define VCNL4200_PS_CONF1   0x03 //PS_CONF1 is a 2 byte register that handles basic PS function settings

static const struct i2c_dt_spec vcnl4200 = I2C_DT_SPEC_GET(DT_NODELABEL(vcnl4200));
static const struct device* ssd1306 = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

int vcnl4200_init(const struct i2c_dt_spec *i2c_dev);

int main() {
	printk("Beginning proximity sensor reading...");

	if (!device_is_ready(ssd1306)) {
		printk("SSD1306 is not ready\n");
		return 0;
	}

    if (cfb_framebuffer_init(ssd1306)) { // initialize cfb frame buffer to communicate with ssd1306
		printk("Framebuffer initialization failed!\n");
		return 0;
	}

	if(!i2c_is_ready_dt(&vcnl4200)){
		printk("Proximity sensor not ready!\n");
		return 0;
	}

	if (vcnl4200_init(&vcnl4200)) {
		printk("Proximity sensor initialization failed!\n");
		return 0;
	}

    while(1) {
        uint8_t font_height;
		cfb_get_font_size(ssd1306, 0, 0, &font_height);

        int ret = cfb_framebuffer_clear(ssd1306, false);
		if (ret < 0) {
			printk("Failed to clear the framebuffer: %d\n", ret);
		}

		ret = cfb_print(ssd1306, "Prox Sensor", 0, 0); //line 0 on display
		if (ret < 0) {
			printk("Failed to print a string: %d\n", ret);
		}

		ret = cfb_print(ssd1306, "------------", 0, font_height); // line 1 on display
		if (ret < 0)
		{
			printk("Failed to print a string: %d\n", ret);
		}

		uint8_t reg = 0x08;
		uint16_t read_value;
		ret = i2c_write_read_dt(&vcnl4200, &reg, 1, &read_value, 2); // write the register to read from
		if (ret < 0)
		{
			printk("Proximity read failed\n");
			continue;
		}

		char message[6];
		snprintf(message, sizeof(message), "%d", read_value);
		ret = cfb_print(ssd1306, message, 0, font_height * 2); //line 2 on display
		if (ret < 0)
		{
			printk("Failed to print a string: %d\n", ret);
		}

		ret = cfb_framebuffer_finalize(ssd1306);
		if (ret < 0) {
			printk("Finalize failed: %d\n", ret);
		}
    }
	
	return 0;
}

/**********************************
* This function initializes the VCNL4200 proximity sensor.
* PS_CONF1 is a 2 byte register that handles basic PS
* functions, including  duty ratio, integration time, 
* interrupt, PS enable/disable and persistence.
*
* arguments: 
* spec - I2C device tree specification
*
* returns:
* 0 on success
*/
int vcnl4200_init(const struct i2c_dt_spec *spec)
{
    uint8_t config[] = {
        VCNL4200_PS_CONF1,
		0x00, // lower byte
		0x00  // higher byte
    };

    return i2c_write_dt(spec, config, sizeof(config));
}