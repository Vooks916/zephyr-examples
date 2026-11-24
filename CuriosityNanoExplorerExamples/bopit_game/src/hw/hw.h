/* SPDX-License-Identifier: Apache-2.0 */
/* Shared hardware helpers. Each chip has exactly one owner module here. */
#ifndef BOPIT_HW_H_
#define BOPIT_HW_H_

#include <stdint.h>
#include <zephyr/sys/util.h>

/* MCP23008 I/O expander 2 @ 0x24 (joystick + capacitive touch outputs SW1..SW3) */
#define IOEXP_JOY_UP    BIT(0)
#define IOEXP_JOY_LEFT  BIT(1)
#define IOEXP_JOY_DOWN  BIT(2)
#define IOEXP_JOY_RIGHT BIT(3)
#define IOEXP_JOY_PUSH  BIT(4)
#define IOEXP_JOY_DIRS  (IOEXP_JOY_UP | IOEXP_JOY_LEFT | IOEXP_JOY_DOWN | IOEXP_JOY_RIGHT)
#define IOEXP_TOUCH_ALL (BIT(5) | BIT(6) | BIT(7))  /* SW3, SW2, SW1 */

int ioexp_init(void);
/* Bitmask of pressed inputs (active-high), or negative errno. */
int ioexp_read_pressed(void);

/* VCNL4200 proximity + ambient light @ 0x51 */
int vcnl4200_init(void);
int vcnl4200_read_als(uint16_t *value);
int vcnl4200_read_ps(uint16_t *value);

/* SAM ADC: potentiometer (AIN7) and microphone (AIN6) */
int analog_init(void);
int analog_read_pot(int32_t *value);
/* Peak-to-peak level over `samples` reads. */
int analog_read_mic_level(uint32_t samples, int32_t *level);

#endif
