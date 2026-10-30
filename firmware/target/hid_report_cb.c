/* HID requests from the target PC: keyboard LEDs in, nothing to read back. */
#include "hid_desc.h"
#include "target_main.h"
#include "tusb.h"

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t type,
                           uint8_t const *buffer, uint16_t size)
{
    (void)report_id;
    if (instance == HID_INST_KBD && type == HID_REPORT_TYPE_OUTPUT && size >= 1) {
        wye_target_set_leds(target_get(), buffer[size == 1 ? 0 : 1] & 0x07u);
    }
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t type,
                               uint8_t *buffer, uint16_t reqlen)
{
    (void)instance; (void)report_id; (void)type; (void)buffer; (void)reqlen;
    return 0;
}
