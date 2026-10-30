/* Target HID layout shared by descriptors and report senders. */
#ifndef WYE_TARGET_HID_DESC_H
#define WYE_TARGET_HID_DESC_H

#include <stdint.h>

enum { HID_INST_KBD = 0, HID_INST_MOUSE = 1, HID_INST_EXTRA = 2 };
enum { RID_ABS = 1, RID_CONSUMER = 2, RID_SYSTEM = 3 };

/* Report-protocol mouse report: 16-bit relative motion. */
struct __attribute__((packed)) mouse16_report {
    uint8_t buttons;
    int16_t x, y;
    int8_t wheel, pan;
};

extern const uint8_t hid_desc_kbd[];
extern const uint8_t hid_desc_mouse[];
extern const uint8_t hid_desc_extra[];
extern const uint16_t hid_desc_kbd_len, hid_desc_mouse_len, hid_desc_extra_len;

#endif
