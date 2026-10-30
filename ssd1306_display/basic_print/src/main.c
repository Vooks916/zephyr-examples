#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/display/cfb.h>

#if !DT_HAS_CHOSEN(zephyr_display)
#error "No suitable devicetree overlay specified"
#endif

int main() {

    //SSD1306 SETUP
    const struct device* dev;
    dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    
	if (!device_is_ready(dev)) {
		return 0;
	}

    if (cfb_framebuffer_init(dev)) { // initialize cfb frame buffer to communicate with ssd1306
		return 0;
	}

    while(1){
        uint8_t font_width; //unused here, returns 10 pixels
        uint8_t font_height; //initially 16 pixels, used to select row # for text
        cfb_get_font_size(dev, 0, &font_width, &font_height);

        cfb_framebuffer_clear(dev, false);

		cfb_print(dev, "Line 1", 0, 0);
        cfb_print(dev, "Line 2", 0, font_height);
		cfb_print(dev, "------------", 0, font_height*2);
		cfb_print(dev, "Hello World!", 0, font_height*3); // last line possible with font_height

		cfb_framebuffer_finalize(dev);
    }
}