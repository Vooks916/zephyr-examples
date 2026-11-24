/*
 * Curiosity Nano Explorer 25CSM04 SPI EEPROM Test
 * Reads the last 4 joystick inputs from the user and determines if it matches
 * the current password. If the last 4 joystick inputs match the password, then the
 * screen will say "UNLOCKED", otherwise it will say "LOCKED".
 * 
 * Password note: The initial password is UP, RIGHT, DOWN, LEFT (U R D L) but
 * the moment that the user presses the SW3 button the password is changed to the
 * last 4 joystick inputs. The password is written into EEPROM so even upon turning
 * the power off the most recent password will remain.
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <stdio.h>

/* ---------- MCP23008 (I/O Expander 2) ---------- */
#define IOEXP2_ADDR     0x24
#define MCP_IODIR       0x00
#define MCP_GPPU        0x06
#define MCP_GPIO        0x09

#define JOY_UP          BIT(0)
#define JOY_LEFT        BIT(1)
#define JOY_DOWN        BIT(2)
#define JOY_RIGHT       BIT(3)
#define SW3             BIT(5)
#define JOY_DIR_MASK    (JOY_UP | JOY_LEFT | JOY_DOWN | JOY_RIGHT | SW3)

/* ---------- Timing ---------- */
#define POLL_MS         20      /* joystick polling period */

static const struct device* ssd1306 = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static const struct device* eeprom  = DEVICE_DT_GET(DT_NODELABEL(eeprom));
static const struct device* i2c_bus = DEVICE_DT_GET(DT_BUS(DT_CHOSEN(zephyr_display)));
static uint16_t past_moves = 0xFFFF; // used to represent the past 4 moves
static uint16_t password = 0;

static int ioexp_init(void)
{
	int ret;

	ret = i2c_reg_write_byte(i2c_bus, IOEXP2_ADDR, MCP_IODIR, 0xFF); /* all inputs */
	if (ret) {
		return ret;
	}
	return i2c_reg_write_byte(i2c_bus, IOEXP2_ADDR, MCP_GPPU, 0xFF); /* all pull-ups on */
}

/* Returns a bitmask of PRESSED inputs (active-high after inversion), or <0 on error */
static int ioexp_read_pressed(void)
{
	uint8_t val;
	int ret = i2c_reg_read_byte(i2c_bus, IOEXP2_ADDR, MCP_GPIO, &val);

	return ret ? ret : (uint8_t)~val;
}

static void handle_move(uint8_t dirs)
{
	if (dirs & JOY_UP) {
		past_moves <<= 4;
        return;
	}
	if (dirs & JOY_RIGHT) {
		past_moves <<= 4;
		past_moves |= 0x1;
        return;
	}
	if (dirs & JOY_DOWN) {
		past_moves <<= 4;
		past_moves |= 0x2;
        return;
	}
	if (dirs & JOY_LEFT) {
		past_moves <<= 4;
		past_moves |= 0x3;
        return;
	}
	if(dirs & SW3){
		password = past_moves;
		int ret = eeprom_write(eeprom, 0x0000, &password, sizeof(password));
		if(ret != 0) {
			return;
		}
	}
}

int main() {

	if (!device_is_ready(ssd1306)) {
		return 0;
	}

    if (!device_is_ready(eeprom)) {
        return 0;
    }

    if (!device_is_ready(i2c_bus)) {
        return 0;
    }

    if (cfb_framebuffer_init(ssd1306)) { // initialize CFB for use with SSD1306
		return 0;
	}

    if (ioexp_init() != 0) { // initialize I/O Expander 2
        return 0;
    }

    if (eeprom_read(eeprom, 0x0000, &password, sizeof(password)) != 0) // ensure EEPROM is reading correctly
	{   
		return 0;
	}

    if(password == 0xFFFF) { // if there is no password currently in EEPROM
        password = 0x0123;   // set the password to up right left down
    }

    uint8_t prev = 0;

    while(1) {
        uint8_t font_height; //initially 16 pixels, used to select row # for text
        int ret = 0; //used for error handling

        cfb_get_font_size(ssd1306, 0, 0, &font_height);

        cfb_framebuffer_clear(ssd1306, false);

		if(past_moves == password){
			ret = cfb_print(ssd1306, "UNLOCKED", 0, 0); // line 0 on display
			if (ret < 0)
			{   
				return 0;
			}
		} else {
			ret = cfb_print(ssd1306, "LOCKED", 0, 0); // line 0 on display
			if (ret < 0)
			{   
				return 0;
			}
	}

		ret = cfb_print(ssd1306, "------------", 0, font_height*1); // line 1 on display
		if (ret < 0)
		{   
			return 0;
		}

		ret = cfb_print(ssd1306, "INPUT:", 0, font_height*2); // line 2 on display
		if (ret < 0)
		{   
			return 0;
		}
		
		/* --------- JOYSTICK INPUT ----------------*/
        int pressed = ioexp_read_pressed();
		if (pressed < 0) {
			printk("I2C read error %d\n", pressed);
			k_msleep(200);
			continue;
		}

		uint8_t now = (uint8_t)pressed;
		uint8_t new_presses = now & ~prev;

		uint8_t dirs = now & JOY_DIR_MASK;
		if (new_presses & JOY_DIR_MASK) {
			handle_move(dirs);
		}
		/* ----------- END JOYSTICK INPUT --------------- */

		/* --------- MESSAGE CONSTRUCTION ----------------*/
		char message[8];
		const char directions[] = {'U', 'R', 'D', 'L'};

		for (int i = 0; i < 4; i++)
		{
			uint16_t input = (past_moves >> (12 - (i * 4))) & 0xF;

			message[i * 2] = (input < 4) ? directions[input] : 'E';
			message[i * 2 + 1] = ' ';
		}

		message[7] = '\0';
		/* ------------ END MESSAGE CONSTRUCTION---------------- */

		ret = cfb_print(ssd1306, message, 0, font_height * 3); //line 3 on display
		if (ret < 0)
		{   
			return 0;
		}

		prev = now;
		cfb_framebuffer_finalize(ssd1306);
        k_msleep(POLL_MS);
    }
}