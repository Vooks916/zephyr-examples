#include <stdbool.h>
#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sleep.h>

#define IO_EXPANDER_I2C_ADDRESS (0x24)

#define IO_EXPANDER_GPIO_REGISTER (0x9)
#define IO_EXPANDER_GPPU_REGISTER (0x6)

#define IO_EXPANDER_GPIO_JOYSTICK_UP (0)
#define IO_EXPANDER_GPIO_JOYSTICK_LEFT (1)
#define IO_EXPANDER_GPIO_JOYSTICK_DOWN (2)
#define IO_EXPANDER_GPIO_JOYSTICK_RIGHT (3)

int main(void)
{
	const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	const struct device *i2c_controller = DEVICE_DT_GET(DT_ALIAS(i2c_controller));

	if (!device_is_ready(display)) {
		return 0;
	}

	cfb_framebuffer_init(display);


	if (!device_is_ready(i2c_controller)) {
		return 0;
	}

	if (i2c_reg_write_byte(i2c_controller, IO_EXPANDER_I2C_ADDRESS, IO_EXPANDER_GPPU_REGISTER, ~0)) {
		return 0;
	}


	int width_pixels = cfb_get_display_parameter(display, CFB_DISPLAY_WIDTH);
	int height_pixels = cfb_get_display_parameter(display, CFB_DISPLAY_HEIGHT);

	size_t snake_pixels = 4;
	int width = width_pixels / snake_pixels;
	int height = height_pixels / snake_pixels;

	size_t snake_length = 3;
	size_t snake_head = snake_length - 1;
	struct cfb_position snake_positions[snake_length];

	enum snake_direction {
		SNAKE_UP,
		SNAKE_LEFT,
		SNAKE_DOWN,
		SNAKE_RIGHT,
	};
	enum snake_direction snake_direction = SNAKE_RIGHT;

	for (size_t i = 0; i < snake_length; i++) {
		snake_positions[i].x = i;
		snake_positions[i].y = height / 2;
	}

	while (1) {
		uint8_t io_gpio_register;

		if (i2c_reg_read_byte(i2c_controller, IO_EXPANDER_I2C_ADDRESS, IO_EXPANDER_GPIO_REGISTER, &io_gpio_register)) {
			return 0;
		}

		int up = !(io_gpio_register & (1 << IO_EXPANDER_GPIO_JOYSTICK_UP));
		int left = !(io_gpio_register & (1 << IO_EXPANDER_GPIO_JOYSTICK_LEFT));
		int right = !(io_gpio_register & (1 << IO_EXPANDER_GPIO_JOYSTICK_RIGHT));
		int down = !(io_gpio_register & (1 << IO_EXPANDER_GPIO_JOYSTICK_DOWN));

		if (up && snake_direction != SNAKE_DOWN) {
			snake_direction = SNAKE_UP;
		} else if (left && snake_direction != SNAKE_RIGHT) {
			snake_direction = SNAKE_LEFT;
		} else if (right && snake_direction != SNAKE_LEFT) {
			snake_direction = SNAKE_RIGHT;
		} else if (down && snake_direction != SNAKE_UP) {
			snake_direction = SNAKE_DOWN;
		}

		cfb_framebuffer_clear(display, false);

		for (size_t i = 0; i < snake_length; i++) {
			for (size_t y = 0; y < snake_pixels; y++) {
				for (size_t x = 0; x < snake_pixels; x++) {
					struct cfb_position position = {
						.x = snake_positions[i].x * snake_pixels + x,
						.y = snake_positions[i].y * snake_pixels + y,
					};
					cfb_draw_point(display, &position);
				}
			}
		}

		size_t next_snake_head = (snake_head + 1) % snake_length;
		int offset_x = 0;
		int offset_y = 0;
		switch (snake_direction) {
			case SNAKE_UP:
				offset_y = -1;
				break;
			case SNAKE_DOWN:
				offset_y = 1;
				break;
			case SNAKE_LEFT:
				offset_x = -1;
				break;
			case SNAKE_RIGHT:
				offset_x = 1;
				break;
		}
		snake_positions[next_snake_head].x = (snake_positions[snake_head].x + offset_x) % width + ((snake_positions[snake_head].x + offset_x) < 0) * width;
		snake_positions[next_snake_head].y = (snake_positions[snake_head].y + offset_y) % height + ((snake_positions[snake_head].y + offset_y) < 0) * height;
		snake_head = next_snake_head;

		cfb_framebuffer_finalize(display);

		k_sleep(K_MSEC(100));
	}

}
