/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/display.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>

#define BUTTONS_NODE DT_NODELABEL(explorer_buttons)

static const struct device *const display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static const struct device *const buttons_dev = DEVICE_DT_GET(BUTTONS_NODE);

static atomic_t pressed_buttons;

static void button_input_cb(struct input_event *evt, void *user_data)
{
	atomic_val_t button_bit;

	ARG_UNUSED(user_data);

	if (evt->type != INPUT_EV_KEY) {
		return;
	}

	switch (evt->code) {
	case INPUT_KEY_1:
		button_bit = BIT(0);
		break;
	case INPUT_KEY_2:
		button_bit = BIT(1);
		break;
	case INPUT_KEY_3:
		button_bit = BIT(2);
		break;
	default:
		return;
	}

	if (evt->value != 0) {
		atomic_or(&pressed_buttons, button_bit);
	} else {
		atomic_and(&pressed_buttons, ~button_bit);
	}
}

INPUT_CALLBACK_DEFINE(buttons_dev, button_input_cb, NULL);

static int show_button(atomic_val_t buttons)
{
	const char *message = "     -     ";
	int ret;

	if ((buttons & BIT(0)) != 0) {
		if ((buttons & BIT(1)) != 0) {
			if ((buttons & BIT(2)) != 0) {
				message = "SW1 SW2 SW3";
			} else {
				message = "SW1 SW2";
			}
		} else if ((buttons & BIT(2)) != 0) {
			message = "SW1     SW3";
		} else {
			message = "SW1";
		}
	} else if ((buttons & BIT(1)) != 0) {
		if ((buttons & BIT(2)) != 0) {
			message = "    SW2 SW3";
		} else {
			message = "    SW2";
		}
	} else if ((buttons & BIT(2)) != 0) {
		message = "        SW3";
	}

	ret = cfb_framebuffer_clear(display_dev, false);
	if (ret < 0) {
		return ret;
	}

	ret = cfb_print(display_dev, "CAPACITIVE  BUTTONS", 0, 0);
	if (ret < 0) {
		return ret;
	}

	ret = cfb_print(display_dev, message, 0, 48);
	if (ret < 0) {
		return ret;
	}

	return cfb_framebuffer_finalize(display_dev);
}

int main(void)
{
	atomic_val_t previous_buttons = -1;
	int ret;

	if (!device_is_ready(display_dev) || !device_is_ready(buttons_dev)) {
		printk("Hardware not ready\n");
		return 0;
	}

	ret = display_set_pixel_format(display_dev, PIXEL_FORMAT_MONO10);
	if (ret != 0) {
		ret = display_set_pixel_format(display_dev, PIXEL_FORMAT_MONO01);
		if (ret != 0) {
			printk("Unsupported pixel format\n");
			return 0;
		}
	}

	ret = cfb_framebuffer_init(display_dev);
	if (ret != 0) {
		printk("CFB initialization failed\n");
		return 0;
	}

	ret = cfb_framebuffer_set_font(display_dev, 0);
	if (ret != 0) {
		printk("Font selection failed\n");
		return 0;
	}

	ret = display_blanking_off(display_dev);
	if (ret != 0 && ret != -ENOSYS) {
		printk("Failed to turn display blanking off\n");
		return 0;
	}

	while (1) {
		atomic_val_t current_buttons = atomic_get(&pressed_buttons);

		if (current_buttons != previous_buttons) {
			ret = show_button(current_buttons);
			if (ret < 0) {
				printk("Failed to update display\n");
				return 0;
			}

			previous_buttons = current_buttons;
		}

		k_msleep(50);
	}

	return 0;
}
