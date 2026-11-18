/*
 * display.h - minimal text-only driver for the ST7789 on the ESP32-C6-LCD-1.47.
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

#define RGB565(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

#define COL_BLACK  RGB565(0, 0, 0)
#define COL_WHITE  RGB565(255, 255, 255)
#define COL_GREY   RGB565(120, 120, 120)
#define COL_RED    RGB565(230, 30, 30)
#define COL_GREEN  RGB565(40, 200, 80)
#define COL_YELLOW RGB565(240, 200, 0)
#define COL_BLUE   RGB565(40, 120, 240)

esp_err_t display_init(void);

/* Draw one full-width text row at pixel row y, scale 1..3 (8x16 font).
 * The whole row (16*scale px tall) is cleared to bg first. */
void display_text_row(int y, int scale, const char *text, uint16_t fg, uint16_t bg);

void display_fill(uint16_t color);
