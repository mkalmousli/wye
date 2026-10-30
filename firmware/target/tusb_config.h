#ifndef WYE_TARGET_TUSB_CONFIG_H
#define WYE_TARGET_TUSB_CONFIG_H

#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#define CFG_TUSB_OS OPT_OS_PICO
#define CFG_TUD_ENDPOINT0_SIZE 64

/* Keyboard (boot), mouse (boot-capable), extras (abs pointer, media, system). */
#define CFG_TUD_HID 3
#define CFG_TUD_HID_EP_BUFSIZE 16
/* Shared drive: one block per callback (blocks come from the host). */
#define CFG_TUD_MSC 1
#define CFG_TUD_MSC_EP_BUFSIZE 512

#include "usb_audio_config.h"

#endif
