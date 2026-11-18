/*
 * gamepad.h - BLE gamepad (HID over GATT) support that shares the radio with
 * DJI's camera code.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"

/* DJI's GATTC app uses this ID; esp_hidh hard-codes app ID 0. */
#define DJI_GATTC_APP_ID 1

/* Implemented in the (patched) DJI ble/ble.c */
void dji_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
void dji_gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

/* Registered by the patched ble_init() instead of DJI's own handlers. They
 * route each Bluetooth event to the camera code or the gamepad code. */
void ble_dispatch_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
void ble_dispatch_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

typedef struct {
    bool connected;
    bool searching;
    int  battery;        /* -1 = unknown */
    int  last_button;    /* HID button number of the last press, 0 = none */
    bool layout_ok;      /* report descriptor understood */
    char name[24];
} gamepad_status_t;

/* Call once after DJI's ble_init(). Starts searching for a controller. */
esp_err_t gamepad_init(void);

void gamepad_get_status(gamepad_status_t *out);

/* Short message for the bottom of the screen. */
void gamepad_set_message(const char *msg);
const char *gamepad_get_message(void);
