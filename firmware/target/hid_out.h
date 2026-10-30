#ifndef WYE_TARGET_HID_OUT_H
#define WYE_TARGET_HID_OUT_H

#include "target.h"

/* TinyUSB-backed implementation of the target HID hooks. */
const struct wye_target_hid *hid_out(void);

#endif
