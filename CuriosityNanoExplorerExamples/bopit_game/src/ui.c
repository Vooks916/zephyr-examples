/* SPDX-License-Identifier: Apache-2.0 */
/*
 * OLED screens through the character framebuffer (as in the examples).
 * Large text: for each line the biggest built-in CFB font that fits the
 * 128 px width is chosen (default fonts are 10x16, 15x24 and 20x32).
 */
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/printk.h>
#include "ui.h"

static const struct device *const display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static uint16_t width, height;

/* Select the largest font whose height <= max_h and that fits `text`; returns its height. */
static uint8_t pick_font_w(const char *text, uint8_t max_h, uint8_t *font_w)
{
        int best = 0, n = cfb_get_numof_fonts(display);
        uint8_t best_w = 0, best_h = 0, w, h;
        size_t len = strlen(text);

        for (int i = 0; i < n; i++) {
                if (cfb_get_font_size(display, i, &w, &h) != 0) {
                        continue;
                }
                if (w * len <= width && h <= max_h && h > best_h) {
                        best = i;
                        best_w = w;
                        best_h = h;
                }
        }
        if (best_h == 0U) { /* nothing fits: use font 0 */
                cfb_get_font_size(display, 0, &best_w, &best_h);
        }
        cfb_framebuffer_set_font(display, best);
        *font_w = best_w;
        return best_h;
}

static uint8_t pick_font(const char *text, uint8_t max_h)
{
        uint8_t w;

        return pick_font_w(text, max_h, &w);
}

static void print_line(const char *text, uint16_t x, uint16_t y, uint8_t max_h)
{
        (void)pick_font(text, max_h);
        cfb_print(display, text, x, y);
}

static void print_centered(const char *text, uint16_t y, uint8_t max_h)
{
        uint8_t w = 0;
        uint16_t text_w;

        (void)pick_font_w(text, max_h, &w);
        text_w = (uint16_t)(w * strlen(text));
        cfb_print(display, text, (text_w < width) ? (width - text_w) / 2U : 0U, y);
}

int ui_init(void)
{
        int ret;

        if (!device_is_ready(display)) {
                printk("OLED not ready (check I2C address 0x3D)\n");
                return -ENODEV;
        }
        /* capacitive_touch example: prefer MONO10, fall back to MONO01 */
        if (display_set_pixel_format(display, PIXEL_FORMAT_MONO10) != 0) {
                (void)display_set_pixel_format(display, PIXEL_FORMAT_MONO01);
        }
        ret = cfb_framebuffer_init(display);
        if (ret != 0) {
                printk("Framebuffer init failed (%d)\n", ret);
                return ret;
        }
        (void)display_blanking_off(display);
        width = cfb_get_display_parameter(display, CFB_DISPLAY_WIDTH);
        height = cfb_get_display_parameter(display, CFB_DISPLAY_HEIGHT);
        return 0;
}

void ui_show_start(uint32_t missing_inputs)
{
        cfb_framebuffer_clear(display, false);
        print_centered("BOP IT!", 0, 32);
        print_line(missing_inputs ? "Some sensors off" : "Touch or press", 0, 32, 16);
        print_line("to start", 0, 48, 16);
        cfb_framebuffer_finalize(display);
}

void ui_show_instruction(const char *verb, uint32_t score, uint32_t permille_left)
{
        char num[8];
        uint16_t bar;

        cfb_framebuffer_clear(display, false);
        print_line(verb, 0, 0, 32);          /* e.g. "TWIRL" in 20x32 */
        print_line("IT!", 0, 32, 24);        /* 15x24 */
        snprintf(num, sizeof(num), "%u", score);
        print_line(num, width - 10U * strlen(num), 36, 16);

        /* time bar along the bottom 4 rows */
        bar = (uint16_t)(((uint32_t)width * MIN(permille_left, 1000U)) / 1000U);
        if (bar > 0U) {
                cfb_invert_area(display, 0, height - 4U, bar, 4);
        }
        cfb_framebuffer_finalize(display);
}

void ui_show_game_over(uint32_t score, uint32_t best, const char *reason)
{
        char line[20];

        cfb_framebuffer_clear(display, false);
        print_line("GAME", 0, 0, 24);
        print_line("OVER", 64, 0, 24);
        snprintf(line, sizeof(line), "Score %u  Best %u", score, best);
        print_line(line, 0, 26, 16);
        print_line(reason, 0, 42, 16);
        cfb_framebuffer_finalize(display);
}
