/* Target USB device and configuration descriptors. */
#include "hid_desc.h"
#include "board.h"
#include "usb_common.h"
#include "usb_audio_desc.h"

#define HID_LEN (TUD_CONFIG_DESC_LEN + 3 * TUD_HID_DESC_LEN)
#define CONFIG_LEN (HID_LEN + TUD_MSC_DESC_LEN + WYE_AUDIO_DESC_LEN)
#define ITF_MSC 3

static tusb_desc_device_t g_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

static uint8_t g_config[CONFIG_LEN];

uint8_t const *tud_descriptor_device_cb(void)
{
    usb_fill_device_desc(&g_device);
    return (uint8_t const *)&g_device;
}

/* Build the optional drive and audio after HID, preserving HID interface numbers. */
uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    bool drive = functions_current()->flags & WYE_FN_DRIVE;
    bool audio = functions_current()->flags & WYE_FN_AUDIO;
    uint16_t total = HID_LEN + (drive ? TUD_MSC_DESC_LEN : 0) + (audio ? WYE_AUDIO_DESC_LEN : 0);
    uint8_t audio_itf = drive ? 4 : 3;
    const uint8_t desc[] = {
        TUD_CONFIG_DESCRIPTOR(1, audio_itf + (audio ? 3 : 0), 0, total, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,
                              100),
        TUD_HID_DESCRIPTOR(HID_INST_KBD, 0, HID_ITF_PROTOCOL_KEYBOARD, hid_desc_kbd_len, 0x81,
                           CFG_TUD_HID_EP_BUFSIZE, 1),
        TUD_HID_DESCRIPTOR(HID_INST_MOUSE, 0, HID_ITF_PROTOCOL_MOUSE, hid_desc_mouse_len, 0x82,
                           CFG_TUD_HID_EP_BUFSIZE, 1),
        TUD_HID_DESCRIPTOR(HID_INST_EXTRA, 0, HID_ITF_PROTOCOL_NONE, hid_desc_extra_len, 0x83,
                           CFG_TUD_HID_EP_BUFSIZE, 1),
    };
    const uint8_t msc[] = {TUD_MSC_DESCRIPTOR(ITF_MSC, 0, 0x04, 0x84, 64)};
    const uint8_t sound[] = {WYE_AUDIO_DESCRIPTOR(audio_itf, 0x05, 0x85)};
    size_t pos = sizeof(desc);
    memcpy(g_config, desc, pos);
    if (drive) {
        memcpy(g_config + pos, msc, sizeof(msc));
        pos += sizeof(msc);
    }
    if (audio) {
        memcpy(g_config + pos, sound, sizeof(sound));
    }
    return g_config;
}
