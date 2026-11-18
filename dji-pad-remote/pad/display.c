/*
 * display.c - see display.h
 *
 * Draws in horizontal bands so we never need a full 110 KB framebuffer
 * (Bluetooth needs that RAM).
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_log.h"

#include "display.h"
#include "pad_config.h"
#include "font8x16.h"

static const char *TAG = "DISPLAY";

#define BAND_MAX_H 48                     /* scale 3 x 16 px */
#define LCD_HOST   SPI2_HOST

static esp_lcd_panel_handle_t s_panel;
static uint16_t *s_band;                  /* LCD_WIDTH * BAND_MAX_H pixels, DMA capable */
static SemaphoreHandle_t s_done;          /* given when a band transfer finished */
static SemaphoreHandle_t s_lock;          /* one drawer at a time */

static bool on_trans_done(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *e, void *ctx) {
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(s_done, &woken);
    return woken == pdTRUE;
}

static inline uint16_t swap16(uint16_t c) { return (c >> 8) | (c << 8); }

esp_err_t display_init(void) {
    s_done = xSemaphoreCreateBinary();
    s_lock = xSemaphoreCreateMutex();
    s_band = heap_caps_malloc(LCD_WIDTH * BAND_MAX_H * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!s_done || !s_lock || !s_band) {
        ESP_LOGE(TAG, "out of memory");
        return ESP_ERR_NO_MEM;
    }

    gpio_config_t bl = { .pin_bit_mask = 1ULL << LCD_PIN_BL, .mode = GPIO_MODE_OUTPUT };
    gpio_config(&bl);
    gpio_set_level(LCD_PIN_BL, 0);

    spi_bus_config_t bus = {
        .sclk_io_num = LCD_PIN_SCLK,
        .mosi_io_num = LCD_PIN_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * BAND_MAX_H * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = LCD_PIN_CS,
        .dc_gpio_num = LCD_PIN_DC,
        .spi_mode = 0,
        .pclk_hz = LCD_PCLK_HZ,
        .trans_queue_depth = 4,
        .on_color_trans_done = on_trans_done,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_cfg, &io));

    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = LCD_PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io, &panel_cfg, &s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel, LCD_INVERT));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(s_panel, LCD_X_GAP, LCD_Y_GAP));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));

    display_fill(COL_BLACK);
    gpio_set_level(LCD_PIN_BL, 1);
    ESP_LOGI(TAG, "display ready");
    return ESP_OK;
}

/* Caller holds s_lock. Blocks until the band has been sent. */
static void push_band(int y, int h) {
    if (y < 0 || y >= LCD_HEIGHT) return;
    if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;
    esp_lcd_panel_draw_bitmap(s_panel, 0, y, LCD_WIDTH, y + h, s_band);
    xSemaphoreTake(s_done, pdMS_TO_TICKS(200));
}

void display_fill(uint16_t color) {
    if (!s_panel) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    uint16_t c = swap16(color);
    for (int i = 0; i < LCD_WIDTH * BAND_MAX_H; i++) s_band[i] = c;
    for (int y = 0; y < LCD_HEIGHT; y += BAND_MAX_H) push_band(y, BAND_MAX_H);
    xSemaphoreGive(s_lock);
}

void display_text_row(int y, int scale, const char *text, uint16_t fg, uint16_t bg) {
    if (!s_panel) return;
    if (scale < 1) scale = 1;
    if (scale > 3) scale = 3;
    const int h = 16 * scale, cw = 8 * scale;

    xSemaphoreTake(s_lock, portMAX_DELAY);
    uint16_t f = swap16(fg), b = swap16(bg);
    for (int i = 0; i < LCD_WIDTH * h; i++) s_band[i] = b;

    int x0 = 2;
    for (const char *p = text; *p && x0 + cw <= LCD_WIDTH; p++, x0 += cw) {
        unsigned char ch = (unsigned char)*p;
        if (ch < 0x20 || ch > 0x7E) ch = '?';
        const uint8_t *glyph = font8x16[ch - 0x20];
        for (int gy = 0; gy < 16; gy++) {
            uint8_t row = glyph[gy];
            if (!row) continue;
            for (int gx = 0; gx < 8; gx++) {
                if (!(row & (0x80 >> gx))) continue;
                for (int sy = 0; sy < scale; sy++) {
                    uint16_t *dst = &s_band[(gy * scale + sy) * LCD_WIDTH + x0 + gx * scale];
                    for (int sx = 0; sx < scale; sx++) dst[sx] = f;
                }
            }
        }
    }
    push_band(y, h);
    xSemaphoreGive(s_lock);
}
