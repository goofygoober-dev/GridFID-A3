/*
 * hid_layout.h - tiny HID report-descriptor parser.
 *
 * Finds where the buttons, the hat switch (D-pad) and the left stick live in a
 * gamepad's input report, so we don't have to hard-code one controller.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    bool     valid;
    uint8_t  report_id;     /* 0 = descriptor uses no report IDs */
    uint16_t bit_offset;    /* offset inside the report payload (report ID byte excluded) */
    uint8_t  bit_size;
    uint16_t count;         /* buttons only */
    uint16_t first_usage;   /* buttons only: HID number of the first bit (normally 1) */
    int32_t  logical_min;
    int32_t  logical_max;
} hid_field_t;

typedef struct {
    hid_field_t buttons;
    hid_field_t hat;
    hid_field_t x;
    hid_field_t y;
} hid_pad_layout_t;

/* Returns true if at least a button field was found. */
bool hid_layout_parse(const uint8_t *desc, size_t len, hid_pad_layout_t *out);

/* Little-endian bit-field read. Returns 0 if the field is outside the buffer. */
uint32_t hid_read_bits(const uint8_t *data, size_t len, uint16_t bit_offset, uint8_t bit_size);
