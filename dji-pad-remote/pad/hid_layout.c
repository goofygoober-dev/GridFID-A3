/*
 * hid_layout.c - see hid_layout.h
 */
#include <string.h>
#include "hid_layout.h"

#define MAX_USAGES 32

typedef struct {
    uint16_t usage_page;
    int32_t  logical_min;
    int32_t  logical_max;
    uint8_t  report_size;
    uint8_t  report_id;
    uint16_t report_count;
} globals_t;

static int32_t sign_extend(uint32_t v, uint8_t bytes) {
    if (bytes == 1) return (int8_t)v;
    if (bytes == 2) return (int16_t)v;
    return (int32_t)v;
}

uint32_t hid_read_bits(const uint8_t *data, size_t len, uint16_t bit_offset, uint8_t bit_size) {
    if (bit_size == 0 || bit_size > 32 || (size_t)(bit_offset + bit_size + 7) / 8 > len) {
        return 0;
    }
    uint32_t v = 0;
    for (uint8_t i = 0; i < bit_size; i++) {
        uint16_t b = bit_offset + i;
        if (data[b >> 3] & (1u << (b & 7))) {
            v |= (1u << i);
        }
    }
    return v;
}

static void set_field(hid_field_t *f, const globals_t *g, uint16_t offset) {
    f->valid = true;
    f->report_id = g->report_id;
    f->bit_offset = offset;
    f->bit_size = g->report_size;
    f->logical_min = g->logical_min;
    f->logical_max = g->logical_max;
}

bool hid_layout_parse(const uint8_t *desc, size_t len, hid_pad_layout_t *out) {
    memset(out, 0, sizeof(*out));

    globals_t g = {0}, g_stack[4];
    int g_sp = 0;
    uint16_t offsets[256] = {0};    /* running input bit offset per report ID */

    uint32_t usages[MAX_USAGES];
    int n_usages = 0;
    uint32_t usage_min = 0, usage_max = 0;
    bool have_range = false;

    size_t i = 0;
    while (i < len) {
        uint8_t prefix = desc[i++];
        if (prefix == 0xFE) {                    /* long item: skip */
            if (i + 1 >= len) break;
            i += 2 + desc[i];
            continue;
        }
        uint8_t size = prefix & 0x03;
        if (size == 3) size = 4;
        uint8_t type = (prefix >> 2) & 0x03;
        uint8_t tag  = (prefix >> 4) & 0x0F;
        if (i + size > len) break;
        uint32_t val = 0;
        for (uint8_t b = 0; b < size; b++) val |= (uint32_t)desc[i + b] << (8 * b);
        i += size;

        if (type == 1) {                                   /* Global */
            switch (tag) {
            case 0x0: g.usage_page = val; break;
            case 0x1: g.logical_min = sign_extend(val, size); break;
            case 0x2: g.logical_max = (g.logical_min < 0) ? sign_extend(val, size) : (int32_t)val; break;
            case 0x7: g.report_size = val; break;
            case 0x8: g.report_id = val; break;
            case 0x9: g.report_count = val; break;
            case 0xA: if (g_sp < 4) g_stack[g_sp++] = g; break;
            case 0xB: if (g_sp > 0) g = g_stack[--g_sp]; break;
            default: break;
            }
        } else if (type == 2) {                            /* Local */
            uint32_t u = (size == 4) ? val : ((uint32_t)g.usage_page << 16) | val;
            switch (tag) {
            case 0x0: if (n_usages < MAX_USAGES) usages[n_usages++] = u; break;
            case 0x1: usage_min = u; have_range = true; break;
            case 0x2: usage_max = u; have_range = true; break;
            default: break;
            }
        } else if (type == 0) {                            /* Main */
            if (tag == 0x8) {                              /* Input */
                uint16_t *off = &offsets[g.report_id];
                bool constant = val & 0x01;
                bool variable = val & 0x02;
                uint16_t page = g.usage_page;

                if (!constant && variable) {
                    if (page == 0x09 && g.report_size == 1 && !out->buttons.valid) {
                        set_field(&out->buttons, &g, *off);
                        out->buttons.count = g.report_count;
                        out->buttons.first_usage = have_range ? (usage_min & 0xFFFF) :
                                                   (n_usages ? (usages[0] & 0xFFFF) : 1);
                        if (out->buttons.first_usage == 0) out->buttons.first_usage = 1;
                    } else {
                        /* Walk each element and match it to its usage. */
                        for (uint16_t e = 0; e < g.report_count; e++) {
                            uint32_t u;
                            if (e < n_usages) {
                                u = usages[e];
                            } else if (have_range && usage_min + e - n_usages <= usage_max) {
                                u = usage_min + e - n_usages;
                            } else if (n_usages) {
                                u = usages[n_usages - 1];
                            } else {
                                break;
                            }
                            uint16_t eo = *off + e * g.report_size;
                            switch (u) {
                            case 0x00010039: if (!out->hat.valid) set_field(&out->hat, &g, eo); break;
                            case 0x00010030: if (!out->x.valid)   set_field(&out->x, &g, eo);   break;
                            case 0x00010031: if (!out->y.valid)   set_field(&out->y, &g, eo);   break;
                            default: break;
                            }
                        }
                    }
                }
                *off += g.report_size * g.report_count;
            }
            /* Any main item clears local state. */
            n_usages = 0;
            usage_min = usage_max = 0;
            have_range = false;
        }
    }
    return out->buttons.valid;
}
