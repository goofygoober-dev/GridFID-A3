/*
 * pad_config.h - everything you are likely to want to change lives here.
 */
#pragma once

#include "driver/gpio.h"

/* ---------------------------------------------------------------------------
 * Button mapping
 *
 * Numbers are HID button numbers (1-based). Every controller numbers its
 * buttons differently, so: flash, connect the controller, press each button
 * and read "BTN n" at the bottom of the screen (also printed in the serial
 * log), then put the right numbers here.
 *
 * The defaults below are a guess at the usual 8BitDo BLE layout.
 * ------------------------------------------------------------------------- */
#define PAD_BTN_SHUTTER        1   /* A      : start/stop recording (photo in photo mode) */
#define PAD_BTN_QUICK_SWITCH   4   /* X      : camera "QS" quick-switch button          */
#define PAD_BTN_PHOTO_MODE     5   /* Y      : jump straight to Photo mode              */
#define PAD_BTN_VIDEO_MODE     2   /* B      : jump straight to Video mode              */
#define PAD_BTN_CONNECT_CAM   12   /* Start  : scan + connect nearest DJI camera        */

/* D-pad (hat switch) left/right cycles through these modes. Modes your
 * camera does not support simply fail and show "MODE FAIL" on screen. */
#define PAD_MODE_CYCLE { CAMERA_MODE_NORMAL, CAMERA_MODE_PHOTO, CAMERA_MODE_SLOW_MOTION, \
                         CAMERA_MODE_TIMELAPSE, CAMERA_MODE_HYPERLAPSE, CAMERA_MODE_SUPERNIGHT }

/* Ignore repeated presses faster than this (ms). */
#define PAD_ACTION_COOLDOWN_MS 400

/* ---------------------------------------------------------------------------
 * Waveshare ESP32-C6-LCD-1.47 (ST7789, 172x320). Check against your board's
 * wiki/schematic if the screen stays black.
 * ------------------------------------------------------------------------- */
#define LCD_PIN_MOSI   GPIO_NUM_6
#define LCD_PIN_SCLK   GPIO_NUM_7
#define LCD_PIN_CS     GPIO_NUM_14
#define LCD_PIN_DC     GPIO_NUM_15
#define LCD_PIN_RST    GPIO_NUM_21
#define LCD_PIN_BL     GPIO_NUM_22

#define LCD_WIDTH      172
#define LCD_HEIGHT     320
#define LCD_X_GAP      34      /* 172-px panel sits in the middle of a 240-px controller */
#define LCD_Y_GAP      0
#define LCD_INVERT     1       /* set 0 if colours look like a photo negative */
#define LCD_PCLK_HZ    (40 * 1000 * 1000)
