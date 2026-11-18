/*
 * gamepad.c - BLE gamepad -> DJI camera actions.
 *
 * How the radio is shared:
 *   Bluedroid allows only one GAP and one GATTC callback. The patched DJI
 *   ble.c registers ble_dispatch_* below instead of its own handlers, and we
 *   forward each event to DJI's handler, Espressif's esp_hidh handler, or both:
 *     - GATTC REG events go by app ID (DJI = 1, esp_hidh = 0)
 *     - connect/disconnect/open/close go by remote address (pad vs camera)
 *     - everything else goes by gattc_if
 *     - scan events go only to whoever started the scan
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_hidh.h"
#include "esp_hidh_gattc.h"

#include "gamepad.h"
#include "hid_layout.h"
#include "pad_config.h"

#include "connect_logic.h"
#include "command_logic.h"
#include "status_logic.h"
#include "enums_logic.h"
#include "key_logic.h"

static const char *TAG = "GAMEPAD";

/* ------------------------------------------------------------------ state */

static esp_gatt_if_t s_dji_if = ESP_GATT_IF_NONE;
static esp_gatt_if_t s_hid_if = ESP_GATT_IF_NONE;

static volatile bool s_pad_scanning = false;
static SemaphoreHandle_t s_scan_done;

static esp_bd_addr_t s_pad_bda;
static volatile bool s_pad_bda_set = false;
static esp_ble_addr_type_t s_pad_addr_type;
static volatile bool s_pad_found = false;

static esp_hidh_dev_t *s_dev = NULL;
static hid_pad_layout_t s_layout;
static uint32_t s_prev_buttons = 0;
static int s_prev_hat = -1;

static gamepad_status_t s_status = { .battery = -1 };
static char s_message[32] = "";

typedef enum {
    ACT_SHUTTER,
    ACT_QUICK_SWITCH,
    ACT_MODE_NEXT,
    ACT_MODE_PREV,
    ACT_MODE_PHOTO,
    ACT_MODE_VIDEO,
    ACT_CONNECT_CAM,
} pad_action_t;

static QueueHandle_t s_actions;

void gamepad_set_message(const char *msg) {
    strncpy(s_message, msg, sizeof(s_message) - 1);
    s_message[sizeof(s_message) - 1] = '\0';
    ESP_LOGI(TAG, "%s", msg);
}

const char *gamepad_get_message(void) { return s_message; }

void gamepad_get_status(gamepad_status_t *out) {
    *out = s_status;
    out->searching = s_pad_scanning;
}

static bool is_pad_bda(const uint8_t *bda) {
    return s_pad_bda_set && memcmp(bda, s_pad_bda, sizeof(esp_bd_addr_t)) == 0;
}

/* ------------------------------------------------------------ dispatching */

void ble_dispatch_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.app_id == DJI_GATTC_APP_ID) {
            s_dji_if = gattc_if;
            dji_gattc_event_handler(event, gattc_if, param);
        } else {
            s_hid_if = gattc_if;
            esp_hidh_gattc_event_handler(event, gattc_if, param);
        }
        return;
    }

    const uint8_t *bda = NULL;
    switch (event) {
    case ESP_GATTC_CONNECT_EVT:    bda = param->connect.remote_bda; break;
    case ESP_GATTC_DISCONNECT_EVT: bda = param->disconnect.remote_bda; break;
    case ESP_GATTC_OPEN_EVT:       bda = param->open.remote_bda; break;
    case ESP_GATTC_CLOSE_EVT:      bda = param->close.remote_bda; break;
    default: break;
    }

    if (bda) {
        /* Bluedroid reports link events to every registered app; only the
         * owner of that link should see them. */
        bool pad = is_pad_bda(bda);
        if (pad && gattc_if == s_hid_if) {
            esp_hidh_gattc_event_handler(event, gattc_if, param);
        } else if (!pad && gattc_if == s_dji_if) {
            dji_gattc_event_handler(event, gattc_if, param);
        }
        return;
    }

    if (gattc_if == s_dji_if) {
        dji_gattc_event_handler(event, gattc_if, param);
    } else if (gattc_if == s_hid_if) {
        esp_hidh_gattc_event_handler(event, gattc_if, param);
    }
}

static bool adv_has_uuid16(uint8_t *adv, uint16_t len, uint8_t type, uint16_t uuid) {
    uint8_t n = 0;
    uint8_t *p = esp_ble_resolve_adv_data_by_type(adv, len, type, &n);
    for (int i = 0; p && i + 1 < n; i += 2) {
        if ((p[i] | (p[i + 1] << 8)) == uuid) return true;
    }
    return false;
}

static bool is_bonded(const uint8_t *bda) {
    int n = esp_ble_get_bond_device_num();
    if (n <= 0) return false;
    esp_ble_bond_dev_t *list = malloc(sizeof(esp_ble_bond_dev_t) * n);
    if (!list) return false;
    bool hit = false;
    if (esp_ble_get_bond_device_list(&n, list) == ESP_OK) {
        for (int i = 0; i < n; i++) {
            if (memcmp(list[i].bd_addr, bda, sizeof(esp_bd_addr_t)) == 0) { hit = true; break; }
        }
    }
    free(list);
    return hit;
}

static bool scan_result_is_gamepad(esp_ble_gap_cb_param_t *r) {
    uint8_t *adv = r->scan_rst.ble_adv;
    uint16_t len = r->scan_rst.adv_data_len + r->scan_rst.scan_rsp_len;

    /* A bonded controller waking up often uses directed advertising (no data). */
    if (r->scan_rst.ble_evt_type == ESP_BLE_EVT_CONN_DIR_ADV) {
        return is_bonded(r->scan_rst.bda);
    }
    if (adv_has_uuid16(adv, len, ESP_BLE_AD_TYPE_16SRV_CMPL, 0x1812) ||
        adv_has_uuid16(adv, len, ESP_BLE_AD_TYPE_16SRV_PART, 0x1812)) {
        return true;
    }
    uint8_t n = 0;
    uint8_t *app = esp_ble_resolve_adv_data_by_type(adv, len, ESP_BLE_AD_TYPE_APPEARANCE, &n);
    if (app && n == 2) {
        uint16_t a = app[0] | (app[1] << 8);
        if (a == 0x03C3 /* joystick */ || a == 0x03C4 /* gamepad */) return true;
    }
    return false;
}

static void pad_gap_event(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
    /* --- pairing: "just works", accept everything --- */
    case ESP_GAP_BLE_SEC_REQ_EVT:
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_NC_REQ_EVT:
        esp_ble_confirm_reply(param->ble_security.key_notif.bd_addr, true);
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        if (param->ble_security.auth_cmpl.success) {
            ESP_LOGI(TAG, "pairing/encryption OK");
        } else {
            ESP_LOGW(TAG, "pairing failed, reason 0x%x", param->ble_security.auth_cmpl.fail_reason);
        }
        break;

    /* --- our own scan for a controller --- */
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
        if (s_pad_scanning) esp_ble_gap_start_scanning(5);
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
        if (!s_pad_scanning) break;
        if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
            if (!s_pad_found && scan_result_is_gamepad(param)) {
                memcpy(s_pad_bda, param->scan_rst.bda, sizeof(esp_bd_addr_t));
                s_pad_addr_type = param->scan_rst.ble_addr_type;
                s_pad_found = true;
                ESP_LOGI(TAG, "controller found: %02X:%02X:%02X:%02X:%02X:%02X rssi %d",
                         s_pad_bda[0], s_pad_bda[1], s_pad_bda[2], s_pad_bda[3], s_pad_bda[4], s_pad_bda[5],
                         param->scan_rst.rssi);
                esp_ble_gap_stop_scanning();
            }
        } else if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) {
            s_pad_scanning = false;
            xSemaphoreGive(s_scan_done);
        }
        break;
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (s_pad_scanning) {
            s_pad_scanning = false;
            xSemaphoreGive(s_scan_done);
        }
        break;
    default:
        break;
    }
}

void ble_dispatch_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    bool ours = s_pad_scanning;   /* sample before pad_gap_event may clear it */
    pad_gap_event(event, param);

    if (ours && (event == ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT ||
                 event == ESP_GAP_BLE_SCAN_START_COMPLETE_EVT ||
                 event == ESP_GAP_BLE_SCAN_RESULT_EVT ||
                 event == ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT)) {
        return;   /* camera code must not react to the controller scan */
    }
    dji_gap_event_handler(event, param);
}

/* ------------------------------------------------------------ HID events */

static void queue_action(pad_action_t a) {
    if (xQueueSend(s_actions, &a, 0) != pdTRUE) {
        ESP_LOGW(TAG, "busy, press ignored");
    }
}

static void on_button_down(int number) {
    s_status.last_button = number;
    ESP_LOGI(TAG, "button %d pressed", number);
    switch (number) {
    case PAD_BTN_SHUTTER:      queue_action(ACT_SHUTTER); break;
    case PAD_BTN_QUICK_SWITCH: queue_action(ACT_QUICK_SWITCH); break;
    case PAD_BTN_PHOTO_MODE:   queue_action(ACT_MODE_PHOTO); break;
    case PAD_BTN_VIDEO_MODE:   queue_action(ACT_MODE_VIDEO); break;
    case PAD_BTN_CONNECT_CAM:  queue_action(ACT_CONNECT_CAM); break;
    default: break;
    }
}

static void on_hat(int dir) {
    /* 0=N 1=NE 2=E 3=SE 4=S 5=SW 6=W 7=NW, -1 = centred */
    if (dir == 2) queue_action(ACT_MODE_NEXT);
    if (dir == 6) queue_action(ACT_MODE_PREV);
}

static void handle_input(const esp_hidh_event_data_t *p) {
    const uint8_t *d = p->input.data;
    size_t len = p->input.length;

    if (!s_status.layout_ok) {
        ESP_LOGI(TAG, "raw report id %u:", p->input.report_id);
        ESP_LOG_BUFFER_HEX(TAG, d, len);
        return;
    }

    const hid_field_t *b = &s_layout.buttons;
    if (b->report_id == p->input.report_id) {
        uint16_t n = b->count > 32 ? 32 : b->count;
        uint32_t now = 0;
        for (uint16_t i = 0; i < n; i++) {
            if (hid_read_bits(d, len, b->bit_offset + i, 1)) now |= (1u << i);
        }
        uint32_t pressed = now & ~s_prev_buttons;
        s_prev_buttons = now;
        for (uint16_t i = 0; i < n; i++) {
            if (pressed & (1u << i)) on_button_down(b->first_usage + i);
        }
    }

    const hid_field_t *h = &s_layout.hat;
    if (h->valid && h->report_id == p->input.report_id) {
        int v = (int)hid_read_bits(d, len, h->bit_offset, h->bit_size) - h->logical_min;
        int dir = (v >= 0 && v <= 7) ? v : -1;
        if (dir != s_prev_hat) {
            s_prev_hat = dir;
            if (dir >= 0) on_hat(dir);
        }
    }
}

static void load_layout(esp_hidh_dev_t *dev) {
    size_t n = 0;
    esp_hid_raw_report_map_t *maps = NULL;
    s_status.layout_ok = false;
    if (esp_hidh_dev_report_maps_get(dev, &n, &maps) != ESP_OK) return;
    for (size_t i = 0; i < n; i++) {
        if (hid_layout_parse(maps[i].data, maps[i].len, &s_layout)) {
            s_status.layout_ok = true;
            ESP_LOGI(TAG, "layout: %u buttons @bit %u (id %u), hat %s @bit %u",
                     s_layout.buttons.count, s_layout.buttons.bit_offset, s_layout.buttons.report_id,
                     s_layout.hat.valid ? "yes" : "no", s_layout.hat.bit_offset);
            return;
        }
    }
    ESP_LOGW(TAG, "could not understand the report descriptor; logging raw reports");
}

static void hidh_callback(void *arg, esp_event_base_t base, int32_t id, void *data) {
    esp_hidh_event_data_t *p = (esp_hidh_event_data_t *)data;
    switch ((esp_hidh_event_t)id) {
    case ESP_HIDH_OPEN_EVENT:
        if (p->open.status == ESP_OK) {
            s_dev = p->open.dev;
            const char *name = esp_hidh_dev_name_get(s_dev);
            strncpy(s_status.name, name ? name : "gamepad", sizeof(s_status.name) - 1);
            load_layout(s_dev);
            s_prev_buttons = 0;
            s_prev_hat = -1;
            s_status.connected = true;
            gamepad_set_message("Pad connected");
        }
        break;
    case ESP_HIDH_BATTERY_EVENT:
        s_status.battery = p->battery.level;
        break;
    case ESP_HIDH_INPUT_EVENT:
        handle_input(p);
        break;
    case ESP_HIDH_CLOSE_EVENT:
        s_dev = NULL;
        s_status.connected = false;
        s_status.battery = -1;
        gamepad_set_message("Pad disconnected");
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------ actions */

static const uint8_t s_mode_cycle[] = PAD_MODE_CYCLE;
#define MODE_CYCLE_LEN (sizeof(s_mode_cycle) / sizeof(s_mode_cycle[0]))

static void switch_mode(camera_mode_t mode) {
    camera_mode_switch_response_frame_t *r = command_logic_switch_camera_mode(mode);
    if (r) {
        free(r);
        gamepad_set_message("Mode changed");
    } else {
        gamepad_set_message("MODE FAIL");
    }
}

static void step_mode(int dir) {
    int idx = -1;
    for (int i = 0; i < (int)MODE_CYCLE_LEN; i++) {
        if (s_mode_cycle[i] == current_camera_mode) { idx = i; break; }
    }
    idx = (idx < 0) ? 0 : (idx + dir + MODE_CYCLE_LEN) % MODE_CYCLE_LEN;
    switch_mode((camera_mode_t)s_mode_cycle[idx]);
}

static void action_task(void *arg) {
    TickType_t last = 0;
    pad_action_t a;
    while (1) {
        if (xQueueReceive(s_actions, &a, portMAX_DELAY) != pdTRUE) continue;

        TickType_t now = xTaskGetTickCount();
        if (now - last < pdMS_TO_TICKS(PAD_ACTION_COOLDOWN_MS)) continue;
        last = now;

        if (a == ACT_CONNECT_CAM) {
            gamepad_set_message("Searching camera");
            handle_boot_long_press();
            gamepad_set_message(connect_logic_get_state() == PROTOCOL_CONNECTED ?
                                "Camera connected" : "Camera not found");
            continue;
        }
        if (connect_logic_get_state() != PROTOCOL_CONNECTED) {
            gamepad_set_message("No camera: START");
            continue;
        }
        switch (a) {
        case ACT_SHUTTER:
            handle_boot_single_press();
            break;
        case ACT_QUICK_SWITCH: {
            key_report_response_frame_t *r = command_logic_key_report_qs();
            if (r) free(r);
            break;
        }
        case ACT_MODE_NEXT:  step_mode(+1); break;
        case ACT_MODE_PREV:  step_mode(-1); break;
        case ACT_MODE_PHOTO: switch_mode(CAMERA_MODE_PHOTO); break;
        case ACT_MODE_VIDEO: switch_mode(CAMERA_MODE_NORMAL); break;
        default: break;
        }
    }
}

/* ------------------------------------------------------------ connection loop */

static bool scan_for_pad(void) {
    static esp_ble_scan_params_t params = {
        .scan_type          = BLE_SCAN_TYPE_ACTIVE,
        .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
        .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_interval      = 0x50,
        .scan_window        = 0x30,
        .scan_duplicate     = BLE_SCAN_DUPLICATE_DISABLE,
    };
    xSemaphoreTake(s_scan_done, 0);       /* clear stale signal */
    s_pad_found = false;
    s_pad_scanning = true;
    if (esp_ble_gap_set_scan_params(&params) != ESP_OK) {
        s_pad_scanning = false;
        return false;
    }
    if (xSemaphoreTake(s_scan_done, pdMS_TO_TICKS(8000)) != pdTRUE) {
        esp_ble_gap_stop_scanning();
        xSemaphoreTake(s_scan_done, pdMS_TO_TICKS(1000));
        s_pad_scanning = false;
    }
    return s_pad_found;
}

static void pad_connect_task(void *arg) {
    while (1) {
        if (s_status.connected) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        /* Don't fight the camera code for the scanner. */
        if (connect_logic_get_state() == BLE_SEARCHING) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        gamepad_set_message("Pair pad (BT mode)");
        if (scan_for_pad()) {
            s_pad_bda_set = true;   /* from now on link events for this address go to esp_hidh */
            gamepad_set_message("Connecting pad...");
            esp_hidh_dev_t *dev = esp_hidh_dev_open(s_pad_bda, ESP_HID_TRANSPORT_BLE, s_pad_addr_type);
            if (!dev) {
                gamepad_set_message("Pad connect failed");
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

esp_err_t gamepad_init(void) {
    s_scan_done = xSemaphoreCreateBinary();
    s_actions = xQueueCreate(4, sizeof(pad_action_t));
    if (!s_scan_done || !s_actions) return ESP_ERR_NO_MEM;

    /* Bonding without a PIN ("just works"), like a phone pairing a gamepad. */
    esp_ble_auth_req_t auth = ESP_LE_AUTH_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth, sizeof(auth));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(iocap));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(key_size));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(init_key));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(rsp_key));

    esp_hidh_config_t cfg = {
        .callback = hidh_callback,
        .event_stack_size = 4096,
        .callback_arg = NULL,
    };
    esp_err_t err = esp_hidh_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_hidh_init failed: %s", esp_err_to_name(err));
        return err;
    }

    xTaskCreate(action_task, "pad_actions", 6144, NULL, 3, NULL);
    xTaskCreate(pad_connect_task, "pad_connect", 4096, NULL, 2, NULL);
    return ESP_OK;
}
