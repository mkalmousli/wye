/* Bridge USB descriptors: CDC-ACM control port and duplex USB audio. */
#include "usb_common.h"
#include "usb_audio_desc.h"

enum { ITF_CDC = 0, ITF_CDC_DATA, ITF_AUDIO, ITF_TOTAL = ITF_AUDIO + 3 };

#define EP_CDC_NOTIF 0x81
#define EP_CDC_OUT 0x02
#define EP_CDC_IN 0x82
#define CONFIG_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + WYE_AUDIO_DESC_LEN)

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

static const uint8_t g_config[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_TOTAL, 0, CONFIG_LEN, 0, 100),
    TUD_CDC_DESCRIPTOR(ITF_CDC, 0, EP_CDC_NOTIF, 8, EP_CDC_OUT, EP_CDC_IN, 64),
    WYE_AUDIO_DESCRIPTOR(ITF_AUDIO, 0x03, 0x83),
};

uint8_t const *tud_descriptor_device_cb(void)
{
    usb_fill_device_desc(&g_device);
    return (uint8_t const *)&g_device;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return g_config;
}
