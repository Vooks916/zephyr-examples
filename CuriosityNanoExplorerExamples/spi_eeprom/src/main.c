#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/eeprom.h>
#include <stdio.h>

#if !DT_HAS_CHOSEN(zephyr_display)
#error "No suitable devicetree overlay specified"
#endif

int main() {
    const struct device* ssd1306 = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    const struct device* eeprom  = DEVICE_DT_GET(DT_NODELABEL(eeprom));
    
	if (!device_is_ready(ssd1306)) {
		return 0;
	}

    if (!device_is_ready(eeprom)) {
        return 0;
    }

    if (cfb_framebuffer_init(ssd1306)) { // initialize cfb frame buffer to communicate with ssd1306
		return 0;
	}

    uint8_t read_val = 0;
    if (eeprom_read(eeprom, 0x0000, &read_val, sizeof(read_val)) != 0)
	{   
		return 0;
	}

    while(1){
        uint8_t font_width; //unused here, returns 10 pixels
        uint8_t font_height; //initially 16 pixels, used to select row # for text
        int ret = 0; //used for error handling
        cfb_get_font_size(ssd1306, 0, &font_width, &font_height);

        cfb_framebuffer_clear(ssd1306, false);

		cfb_print(ssd1306, "EEPROM", 0, 0);

        char message[6];
		snprintf(message, sizeof(message), "%d", read_val);
		ret = cfb_print(ssd1306, message, 0, font_height * 1); //line 2 on display
		if (ret < 0)
		{   
			return 0;
		}

		cfb_framebuffer_finalize(ssd1306);
    }
}