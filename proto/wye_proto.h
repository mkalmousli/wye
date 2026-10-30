/*
 * Wye wire protocol v2 — constants shared by app and firmware.
 *
 * Every packet is COBS-encoded and terminated by 0x00. Decoded layout:
 *   [0] node  (host->device: destination, device->host: source)
 *   [1] type  (WYE_MSG_*)
 *   [2] seq   (echoed in replies)
 *   [3..]     payload, 0..WYE_MAX_PAYLOAD bytes, little endian fields
 *   [last 2]  CRC-16/CCITT-FALSE of everything before it
 */
#ifndef WYE_PROTO_H
#define WYE_PROTO_H

#define WYE_PROTO_VERSION 2u
#define WYE_UART_BAUD 921600u
#define WYE_MAX_PAYLOAD 232u
#define WYE_HEADER_LEN 3u
#define WYE_CRC_LEN 2u
#define WYE_MAX_PACKET (WYE_HEADER_LEN + WYE_MAX_PAYLOAD + WYE_CRC_LEN)
#define WYE_MAX_ENCODED (WYE_MAX_PACKET + WYE_MAX_PACKET / 254u + 2u)

#define WYE_FW_CHUNK 224u        /* FW_DATA bytes per packet */
#define WYE_ABS_MAX 32767u       /* absolute pointer range */
#include "wye_disk.h"
#include "wye_audio.h"

enum wye_node_id { WYE_NODE_HOST = 0, WYE_NODE_BRIDGE = 1, WYE_NODE_TARGET = 2 };
enum wye_role { WYE_ROLE_BRIDGE = 1, WYE_ROLE_TARGET = 2 };

enum wye_msg {
    /* Session */
    WYE_MSG_HELLO = 0x01,        /* h->n  u8 proto_version */
    WYE_MSG_HELLO_ACK = 0x02,    /* n->h  struct wye_hello_ack */
    WYE_MSG_PING = 0x03,         /* h->n  u32 token */
    WYE_MSG_PONG = 0x04,         /* n->h  struct wye_pong */
    WYE_MSG_ERROR = 0x05,        /* n->h  u8 code, u8 offending type */
    WYE_MSG_LOG = 0x06,          /* n->h  text */
    /* Input state replication, host -> target. All idempotent. */
    WYE_MSG_KEYBOARD = 0x10,     /* struct wye_keyboard */
    WYE_MSG_MOUSE = 0x11,        /* struct wye_mouse (relative) */
    WYE_MSG_CONSUMER = 0x12,     /* u16 usage, 0 = released */
    WYE_MSG_SYSTEM = 0x13,       /* u8 usage, 0 = released */
    WYE_MSG_RELEASE_ALL = 0x14,
    WYE_MSG_ABS_MOUSE = 0x15,    /* struct wye_abs_mouse */
    /* Target -> host notifications */
    WYE_MSG_LEDS = 0x20,         /* u8 WYE_LED_* */
    WYE_MSG_TARGET_STATE = 0x21, /* u8 WYE_TSTATE_* */
    /* Maintenance */
    WYE_MSG_REBOOT = 0x30,       /* u8 WYE_REBOOT_* */
    WYE_MSG_FW_BEGIN = 0x31,     /* u32 size, u32 crc32 */
    WYE_MSG_FW_DATA = 0x32,      /* u32 offset, data */
    WYE_MSG_FW_COMMIT = 0x33,
    WYE_MSG_FW_ACK = 0x34,       /* u8 WYE_FW_*, u32 next_offset */
    WYE_MSG_IDENTITY_GET = 0x35,
    WYE_MSG_IDENTITY = 0x36,     /* n->h  struct wye_identity */
    WYE_MSG_IDENTITY_SET = 0x37, /* h->n  struct wye_identity; applied on reboot */
    WYE_MSG_FUNCS_GET = 0x38,    /* h->t  what the target shows the other PC */
    WYE_MSG_FUNCS = 0x39,        /* t->h  struct wye_functions */
    WYE_MSG_FUNCS_SET = 0x3a,    /* h->t  struct wye_functions; applied on reboot */
    /* Shared drive: blocks of the target's USB drive live on the host */
    WYE_MSG_DISK_SET = 0x40,     /* h->t  u8 present, u32 block count */
    WYE_MSG_DISK_STATUS = 0x41,  /* t->h  u8 WYE_DISK_* flags (on change and after DISK_SET) */
    WYE_MSG_DISK_READ = 0x42,    /* t->h  u32 lba, u8 count (<= WYE_DISK_READAHEAD) */
    WYE_MSG_DISK_DATA = 0x43,    /* h->t  u32 lba, u16 offset, data (<= WYE_DISK_CHUNK) */
    WYE_MSG_DISK_WRITE = 0x44,   /* t->h  u32 lba, u16 offset, data (<= WYE_DISK_CHUNK) */
    WYE_MSG_DISK_WACK = 0x45,    /* h->t  u32 lba, u8 status (0 = stored) */
    /* Pico to Pico only (node TARGET both ways, never to the app): each side's USB playback
       becomes the other side's microphone. 2..WYE_AUDIO_CHUNK bytes of whole PCM samples. */
    WYE_MSG_AUDIO = 0x50,
};

enum wye_error { WYE_ERR_UNKNOWN_TYPE = 1, WYE_ERR_BAD_LENGTH, WYE_ERR_BAD_VERSION, WYE_ERR_BUSY };

enum wye_caps {
    WYE_CAP_FW_UPDATE = 1u << 0,
    WYE_CAP_BOOT_KEYBOARD = 1u << 1,
    WYE_CAP_MOUSE16 = 1u << 2,
    WYE_CAP_REMOTE_WAKEUP = 1u << 3,
    WYE_CAP_ABS_MOUSE = 1u << 4,
    WYE_CAP_IDENTITY = 1u << 5,
    WYE_CAP_DISK = 1u << 6,
    WYE_CAP_AUDIO = 1u << 7,
};

enum wye_tstate {
    WYE_TSTATE_USB_MOUNTED = 1u << 0,
    WYE_TSTATE_USB_SUSPENDED = 1u << 1,
    WYE_TSTATE_KBD_BOOT = 1u << 2,
};
enum wye_bstate { WYE_BSTATE_HOST_DTR = 1u << 0 };
enum wye_led { WYE_LED_NUM = 1u << 0, WYE_LED_CAPS = 1u << 1, WYE_LED_SCROLL = 1u << 2 };

enum wye_button {
    WYE_BTN_LEFT = 1u << 0, WYE_BTN_RIGHT = 1u << 1, WYE_BTN_MIDDLE = 1u << 2, WYE_BTN_BACK = 1u << 3,
    WYE_BTN_FORWARD = 1u << 4,
};

#include "wye_maint.h"

#endif
