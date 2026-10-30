/* USB helpers shared by both firmware images. */
#ifndef WYE_USB_COMMON_H
#define WYE_USB_COMMON_H

#include "tusb.h"

/* Fills VID/PID/bcdDevice from the runtime identity. */
void usb_fill_device_desc(tusb_desc_device_t *d);

/*
 * Marks an image with its role so the updater refuses cross-flashing.
 * Pass wye_fw_role_tag to fw_flash_ops(); that reference keeps it linked.
 */
#define WYE_FW_ROLE_TAG(role) static const char wye_fw_role_tag[] = "WYE-FW:" role

#endif
