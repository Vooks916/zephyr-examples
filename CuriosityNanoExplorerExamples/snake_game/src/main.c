#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <zephyr/devicetree.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sleep.h>

#define IO_EXPANDER_I2C_ADDRESS (0x24)

#define IO_EXPANDER_GPIO_REGISTER (0x9)
#define IO_EXPANDER_GPPU_REGISTER (0x6)

#define IO_EXPANDER_GPIO_JOYSTICK_UP (0)
#define IO_EXPANDER_GPIO_JOYSTICK_LEFT (1)
#define IO_EXPANDER_GPIO_JOYSTICK_DOWN (2)
#define IO_EXPANDER_GPIO_JOYSTICK_RIGHT (3)
#define IO_EXPANDER_GPIO_JOYSTICK_PUSH (4)

struct node {
	uint8_t next;
	uint8_t x;
	uint8_t y;
};

uint8_t on_snake(struct node *nodes, uint8_t head, int x, int y) {
	uint8_t n = head;
	uint8_t on_snake;
	do {
		on_snake = nodes[n].x == x && nodes[n].y == y;
		n = nodes[n].next;
	} while (n != head && !on_snake);
	return on_snake;
}

void new_dot(struct node *nodes, uint8_t head, int width, int height, int *x, int *y) {
	*x = sys_rand32_get() % width;
	*y = sys_rand32_get() % height;

	for (int i = 0; i < width * height - 1 && on_snake(nodes, head, *x, *y); i++) {
		*x = (*x + 1) % width;
		*y = (*y + (*x == 0)) % height;
	}
}

int main(void)
{
	const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	const struct device *i2c_controller = DEVICE_DT_GET(DT_ALIAS(i2c_controller));

	if (!device_is_ready(display)) {
		return 0;
	}

	cfb_framebuffer_init(display);

	if (cfb_framebuffer_set_font(display, 0)) {
		return 0;
	}

	uint8_t font_width, font_height;

	if (cfb_get_font_size(display, 0, &font_width, &font_height)) {
		return 0;
	}

	if (!device_is_ready(i2c_controller)) {
		return 0;
	}

	if (i2c_reg_write_byte(i2c_controller, IO_EXPANDER_I2C_ADDRESS, IO_EXPANDER_GPPU_REGISTER, ~0)) {
		return 0;
	}


	int width_pixels = cfb_get_display_parameter(display, CFB_DISPLAY_WIDTH);
	int height_pixels = cfb_get_display_parameter(display, CFB_DISPLAY_HEIGHT);

init:
	uint8_t pixels = 8;
	int width = width_pixels / pixels;
	int height = height_pixels / pixels;

	uint8_t length = 3;
#define MAX_NODES 128
	struct node nodes[MAX_NODES];

	uint8_t head = 0;
	nodes[head].x = 0;
	nodes[head].y = height / 2;
	for (size_t i = 1; i < length; i++) {
		nodes[head].next = i;

		head = nodes[head].next;
		nodes[head].x = i;
		nodes[head].y = height / 2;
	}

	nodes[head].next = 0;

	enum snake_direction {
		SNAKE_UP,
		SNAKE_LEFT,
		SNAKE_DOWN,
		SNAKE_RIGHT,
	};
	enum snake_direction snake_direction = SNAKE_RIGHT;

	int dot_x;
	int dot_y;

	new_dot(nodes, head, width, height, &dot_x, &dot_y);

	uint8_t game_over = 0;
	uint16_t score = 0;

	while (1) {
		uint8_t up = 0;
		uint8_t left = 0;
		uint8_t right = 0;
		uint8_t down = 0;
		uint8_t push = 0;

		for (int i = 0; i < 8; i++) {
			uint8_t io_gpio_register;

			if (i2c_reg_read_byte(i2c_controller, IO_EXPANDER_I2C_ADDRESS, IO_EXPANDER_GPIO_REGISTER, &io_gpio_register)) {
				return 0;
			}

			up |= !(io_gpio_register & (1 << IO_EXPANDER_GPIO_JOYSTICK_UP));
			left |= !(io_gpio_register & (1 << IO_EXPANDER_GPIO_JOYSTICK_LEFT));
			right |= !(io_gpio_register & (1 << IO_EXPANDER_GPIO_JOYSTICK_RIGHT));
			down |= !(io_gpio_register & (1 << IO_EXPANDER_GPIO_JOYSTICK_DOWN));
			push |= !(io_gpio_register & (1 << IO_EXPANDER_GPIO_JOYSTICK_PUSH));

			k_sleep(K_MSEC(25));
		}

		if (game_over) {
			if (push) {
				goto init;
			}
		} else {
			if (up && snake_direction != SNAKE_DOWN) {
				snake_direction = SNAKE_UP;
			} else if (left && snake_direction != SNAKE_RIGHT) {
				snake_direction = SNAKE_LEFT;
			} else if (right && snake_direction != SNAKE_LEFT) {
				snake_direction = SNAKE_RIGHT;
			} else if (down && snake_direction != SNAKE_UP) {
				snake_direction = SNAKE_DOWN;
			}

			int8_t offset_x = 0;
			int8_t offset_y = 0;
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

			int head_x = nodes[head].x + offset_x;
			int head_y = nodes[head].y + offset_y;

			if (on_snake(nodes, head, head_x, head_y) || head_x == -1 || head_x == width || head_y == -1 || head_y == height) {
				game_over = 1;
			} else {
				if (head_x == dot_x && head_y == dot_y) {
					if (length < MAX_NODES) {
						nodes[length].next = nodes[head].next;
						nodes[head].next = length;
						length++;
					}

					new_dot(nodes, head, width, height,  &dot_x, &dot_y);

					score++;
				}

				head = nodes[head].next;
				nodes[head].x = head_x;
				nodes[head].y = head_y;
			}
		}

		cfb_framebuffer_clear(display, false);

		if (game_over) {
			cfb_draw_text(display, "GAME OVER", 0, font_height * 0);

			char line[13];
			snprintf(line, 13, "Score: %hu", score);
			cfb_draw_text(display, line, 0, font_height * 1);
		} else {
			char line[13];
			snprintf(line, 13, "%hu", score);
			cfb_draw_text(display, line, 0, 0);

			uint8_t n = head;
			do {
				for (uint8_t y = 0; y < pixels; y++) {
					for (uint8_t x = 0; x < pixels; x++) {
						struct cfb_position position = {
							.x = nodes[n].x * pixels + x,
							.y = nodes[n].y * pixels + y,
						};
						cfb_draw_point(display, &position);
					}
				}
				n = nodes[n].next;
			} while (n != head);

			for (uint8_t y = 0; y < pixels; y++) {
				for (uint8_t x = 0; x < pixels; x++) {
					uint8_t x_distance = abs((int)(x + 1) - pixels / 2);
					uint8_t y_distance = abs((int)(y + 1) - pixels / 2);
					if (x_distance * x_distance + y_distance * y_distance < pixels * pixels / 8) {
						struct cfb_position position = {
							.x = dot_x * pixels + x,
							.y = dot_y * pixels + y,
						};
						cfb_draw_point(display, &position);
					}
				}
			}
		}

		cfb_framebuffer_finalize(display);
	}

}
