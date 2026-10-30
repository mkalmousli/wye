/* USB string descriptors served from the runtime identity. */
#include "board.h"

#include "tusb.h"

#include <string.h>

static uint16_t g_desc[WYE_ID_STR_MAX + 1];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    const struct wye_identity *id = identity_current();
    const char *str = NULL;
    size_t count = 0;
    switch (index) {
    case 0:
        g_desc[1] = 0x0409; /* English (US) */
        count = 1;
        break;
    case 1: str = id->manufacturer; break;
    case 2: str = id->product; break;
    case 3: str = id->serial; break;
    case 4: str = "Wye Speaker"; break;    /* plays to the other computer */
    case 5: str = "Wye Microphone"; break; /* records what the other computer sends */
    case 6: str = "Wye Audio"; break;
    default: return NULL;
    }
    if (str) {
        count = strlen(str);
        if (count > WYE_ID_STR_MAX - 1) {
            count = WYE_ID_STR_MAX - 1;
        }
        for (size_t i = 0; i < count; ++i) {
            g_desc[1 + i] = (uint8_t)str[i];
        }
    }
    g_desc[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * count + 2));
    return g_desc;
}

void usb_fill_device_desc(tusb_desc_device_t *d)
{
    const struct wye_identity *id = identity_current();
    d->idVendor = id->vid;
    d->idProduct = id->pid;
    d->bcdDevice = (uint16_t)((WYE_VERSION_MAJOR << 8) | (WYE_VERSION_MINOR << 4) | WYE_VERSION_PATCH);
}
