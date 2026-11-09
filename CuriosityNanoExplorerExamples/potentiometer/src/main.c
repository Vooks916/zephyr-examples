#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
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

    while(1){
        (void)adc_sequence_init_dt(&adc_channel, &sequence); // initialize adc sequence
	    ret = adc_read_dt(&adc_channel, &sequence); // send a read request, output is put in sequence's buffer
        if (ret < 0) {
			printk("Could not read (%d)\n", ret);
            return 0;
	    }
		printk("Potentiometer reading: %d\n", buf);
		k_msleep(1000); // delay for a second
    }
}