/* HID report descriptors for the target's three interfaces. */
#include "hid_desc.h"

#include "tusb.h"

const uint8_t hid_desc_kbd[] = {TUD_HID_REPORT_DESC_KEYBOARD()};

/* Like TinyUSB's mouse template but with 16-bit X/Y for fast, precise motion. */
const uint8_t hid_desc_mouse[] = {
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),
    HID_USAGE(HID_USAGE_DESKTOP_MOUSE),
    HID_COLLECTION(HID_COLLECTION_APPLICATION),
      HID_USAGE(HID_USAGE_DESKTOP_POINTER),
      HID_COLLECTION(HID_COLLECTION_PHYSICAL),
        HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),
        HID_USAGE_MIN(1), HID_USAGE_MAX(5),
        HID_LOGICAL_MIN(0), HID_LOGICAL_MAX(1),
        HID_REPORT_COUNT(5), HID_REPORT_SIZE(1),
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
        HID_REPORT_COUNT(1), HID_REPORT_SIZE(3), HID_INPUT(HID_CONSTANT),
        HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),
        HID_USAGE(HID_USAGE_DESKTOP_X), HID_USAGE(HID_USAGE_DESKTOP_Y),
        HID_LOGICAL_MIN_N(-32767, 2), HID_LOGICAL_MAX_N(32767, 2),
        HID_REPORT_SIZE(16), HID_REPORT_COUNT(2),
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_RELATIVE),
        HID_USAGE(HID_USAGE_DESKTOP_WHEEL),
        HID_LOGICAL_MIN(0x81), HID_LOGICAL_MAX(0x7f),
        HID_REPORT_SIZE(8), HID_REPORT_COUNT(1),
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_RELATIVE),
        HID_USAGE_PAGE(HID_USAGE_PAGE_CONSUMER),
        HID_USAGE_N(HID_USAGE_CONSUMER_AC_PAN, 2),
        HID_LOGICAL_MIN(0x81), HID_LOGICAL_MAX(0x7f),
        HID_REPORT_SIZE(8), HID_REPORT_COUNT(1),
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_RELATIVE),
      HID_COLLECTION_END,
    HID_COLLECTION_END,
};

const uint8_t hid_desc_extra[] = {
    TUD_HID_REPORT_DESC_ABSMOUSE(HID_REPORT_ID(RID_ABS)),
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(RID_CONSUMER)),
    TUD_HID_REPORT_DESC_SYSTEM_CONTROL(HID_REPORT_ID(RID_SYSTEM)),
};

const uint16_t hid_desc_kbd_len = sizeof(hid_desc_kbd);
const uint16_t hid_desc_mouse_len = sizeof(hid_desc_mouse);
const uint16_t hid_desc_extra_len = sizeof(hid_desc_extra);

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    switch (instance) {
    case HID_INST_KBD: return hid_desc_kbd;
    case HID_INST_MOUSE: return hid_desc_mouse;
    default: return hid_desc_extra;
    }
}
