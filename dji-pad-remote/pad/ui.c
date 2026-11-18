/*
 * ui.c - status screen. Redraws only the rows whose content changed.
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ui.h"
#include "display.h"
#include "gamepad.h"

#include "connect_logic.h"
#include "status_logic.h"
#include "enums_logic.h"

typedef struct {
    int y, scale;
    char text[24];
    uint16_t fg, bg;
    bool drawn;
} row_cache_t;

enum { R_TITLE, R_PAD, R_CAM, R_REC, R_MODE, R_BATT, R_BTN, R_MSG, R_HELP1, R_HELP2, R_COUNT };

static row_cache_t s_rows[R_COUNT] = {
    [R_TITLE] = {   4, 1 },
    [R_PAD]   = {  26, 2 },
    [R_CAM]   = {  62, 2 },
    [R_REC]   = { 106, 3 },
    [R_MODE]  = { 162, 2 },
    [R_BATT]  = { 204, 1 },
    [R_BTN]   = { 224, 1 },
    [R_MSG]   = { 244, 1 },
    [R_HELP1] = { 278, 1 },
    [R_HELP2] = { 298, 1 },
};

static void row(int idx, const char *text, uint16_t fg, uint16_t bg) {
    row_cache_t *r = &s_rows[idx];
    if (r->drawn && r->fg == fg && r->bg == bg && strncmp(r->text, text, sizeof(r->text) - 1) == 0) {
        return;
    }
    r->drawn = true;
    strncpy(r->text, text, sizeof(r->text) - 1);
    r->text[sizeof(r->text) - 1] = '\0';
    r->fg = fg;
    r->bg = bg;
    display_text_row(r->y, r->scale, r->text, fg, bg);
}

static const char *mode_name(uint8_t m) {
    switch (m) {
    case CAMERA_MODE_SLOW_MOTION:     return "SLOW-MO";
    case CAMERA_MODE_NORMAL:          return "VIDEO";
    case CAMERA_MODE_TIMELAPSE:       return "TIMELAPSE";
    case CAMERA_MODE_PHOTO:           return "PHOTO";
    case CAMERA_MODE_HYPERLAPSE:      return "HYPERLAPS";
    case CAMERA_MODE_LIVE_STREAMING:  return "LIVE";
    case CAMERA_MODE_SUPERNIGHT:      return "NIGHT";
    case CAMERA_MODE_SUBJECT_TRACKING:return "TRACKING";
    case CAMERA_MODE_PANORAMIC_VIDEO_360: return "360 VIDEO";
    case CAMERA_MODE_PANORAMIC_PHOTO_360: return "360 PHOTO";
    default:                          return "MODE ?";
    }
}

static void ui_task(void *arg) {
    char buf[24];
    gamepad_status_t pad;

    row(R_TITLE, "DJI PAD REMOTE", COL_BLUE, COL_BLACK);
    row(R_HELP1, "A=REC  START=CAMERA", COL_GREY, COL_BLACK);
    row(R_HELP2, "<  >=MODE  X=QS", COL_GREY, COL_BLACK);

    while (1) {
        gamepad_get_status(&pad);

        if (pad.connected) {
            row(R_PAD, "PAD  OK", COL_GREEN, COL_BLACK);
        } else {
            row(R_PAD, pad.searching ? "PAD  SCAN" : "PAD  ---", COL_YELLOW, COL_BLACK);
        }

        connect_state_t cs = connect_logic_get_state();
        switch (cs) {
        case PROTOCOL_CONNECTED: row(R_CAM, "CAM  OK",   COL_GREEN,  COL_BLACK); break;
        case BLE_CONNECTED:      row(R_CAM, "CAM  LINK", COL_YELLOW, COL_BLACK); break;
        case BLE_SEARCHING:      row(R_CAM, "CAM  SCAN", COL_YELLOW, COL_BLACK); break;
        case BLE_DISCONNECTING:  row(R_CAM, "CAM  BYE",  COL_GREY,   COL_BLACK); break;
        default:                 row(R_CAM, "CAM  ---",  COL_GREY,   COL_BLACK); break;
        }

        bool cam_ok = (cs == PROTOCOL_CONNECTED) && camera_status_initialized;
        if (cam_ok && is_camera_recording()) {
            row(R_REC, " REC ", COL_WHITE, COL_RED);
        } else if (cam_ok) {
            row(R_REC, "READY", COL_GREEN, COL_BLACK);
        } else {
            row(R_REC, "-----", COL_GREY, COL_BLACK);
        }

        row(R_MODE, cam_ok ? mode_name(current_camera_mode) : "", COL_WHITE, COL_BLACK);

        if (pad.battery >= 0) {
            snprintf(buf, sizeof(buf), "Pad battery %d%%", pad.battery);
        } else {
            snprintf(buf, sizeof(buf), "Pad battery ?");
        }
        row(R_BATT, buf, COL_GREY, COL_BLACK);

        if (pad.last_button) {
            snprintf(buf, sizeof(buf), "Last: BTN %d%s", pad.last_button, pad.layout_ok ? "" : " RAW");
        } else {
            snprintf(buf, sizeof(buf), pad.connected && !pad.layout_ok ? "Layout: RAW (log)" : "Last: -");
        }
        row(R_BTN, buf, COL_GREY, COL_BLACK);

        row(R_MSG, gamepad_get_message(), COL_YELLOW, COL_BLACK);

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void ui_start(void) {
    xTaskCreate(ui_task, "ui", 4096, NULL, 1, NULL);
}
