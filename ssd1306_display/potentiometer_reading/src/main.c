#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels) || \
	!DT_HAS_CHOSEN(zephyr_shell_uart)
#error "No suitable devicetree overlay specified"
#endif

/* Data of ADC io-channel specified in devicetree. */
static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

int main() {
	printk("Beginning potentiometer reading...");
    // ADC SETUP
    int ret = 0; //used for error handling
	uint16_t buf = 0; //adc output reading
	struct adc_sequence sequence = { //adc sequence
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};
    if (!adc_is_ready_dt(&adc_channel)) {
		printk("ADC controller device %s not ready\n", adc_channel.dev->name);
		return 0;
	}

		ret = adc_channel_setup_dt(&adc_channel);
	if (ret < 0) {
		printk("Could not setup channel (%d)\n", ret);
		return 0;
	}

    //SSD1306 SETUP
    const struct device* dev;
    dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display)); //ssd1306 display dt node
    
	if (!device_is_ready(dev)) {
		printk("Device %s not ready\n", dev->name);
		return 0;
	}

    if (cfb_framebuffer_init(dev)) { // initialize cfb frame buffer to communicate with ssd1306
		printk("Framebuffer initialization failed!\n");
		return 0;
	}

    while(1){
        uint8_t font_width;
        uint8_t font_height;
        cfb_get_font_size(dev, 0, &font_width, &font_height);

        int ret = cfb_framebuffer_clear(dev, false);
		if (ret < 0) {
			printk("Failed to clear the framebuffer: %d\n", ret);
		}

		ret = cfb_print(dev, "POT-METER", 0, 0); //line 0 on display
		if (ret < 0) {
			printk("Failed to print a string: %d\n", ret);
		}

        ret = cfb_print(dev, "------------", 0, font_height); //line 1 on display
        if (ret < 0) {
			printk("Failed to print a string: %d\n", ret);
		}

        (void)adc_sequence_init_dt(&adc_channel, &sequence); // initialize adc sequence
	    ret = adc_read_dt(&adc_channel, &sequence); // send a read request, output is put in sequence's buffer
        if (ret < 0) {
			printk("Could not read (%d)\n", ret);
            return 0;
	    }

        char str[6];
        buf /= 41; //highest reading is 4095, so this scales the reading from 0-99
        snprintf(str, sizeof(str), "%d", buf); //store adc output reading into str
        char message[16];
        snprintf(message, sizeof(message), "Value: %s", str); // store "Val: {output reading}" into message

        ret = cfb_print(dev, message, 0, font_height * 3); //line 3 on display (prints message)
        if (ret < 0) {
			printk("Failed to print a message: %d\n", ret);
		}

		ret = cfb_framebuffer_finalize(dev);
		if (ret < 0) {
			printk("Finalize failed: %d\n", ret);
		}
    }
}