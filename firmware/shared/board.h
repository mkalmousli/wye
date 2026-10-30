/* Board services shared by bridge and target firmware. */
#ifndef WYE_BOARD_H
#define WYE_BOARD_H

#include "wye_msg.h"

#include <stdbool.h>
#include <stdint.h>

/* Flash layout (2 MiB Pico). Running image, staging area, identity sector. */
#define WYE_FLASH_TOTAL (2u * 1024u * 1024u)
#define WYE_FLASH_APP_MAX (1024u * 1024u)
#define WYE_FLASH_STAGING_OFFSET WYE_FLASH_APP_MAX
#define WYE_FLASH_IDENTITY_OFFSET (WYE_FLASH_TOTAL - 4096u)
#define WYE_FLASH_STAGING_SIZE (WYE_FLASH_IDENTITY_OFFSET - WYE_FLASH_STAGING_OFFSET)

uint32_t board_now_ms(void);
void board_hello(struct wye_hello_ack *a, uint8_t role, uint16_t caps);
void board_reboot(uint8_t mode);
void board_led(bool on);
void board_usb_leave(void);  /* disconnect from USB cleanly before a restart */

/* Persistent, runtime-configurable USB identity (identity_store.c). */
void identity_init(const struct wye_identity *defaults);
const struct wye_identity *identity_current(void);
bool identity_save(const struct wye_identity *id);
/* Target only: drive/audio switches and drive names (same flash sector). */
void functions_init(const struct wye_functions *defaults);
const struct wye_functions *functions_current(void);
bool functions_save(const struct wye_functions *f);

/* Firmware staging + install (fw_flash.c). tag: "WYE-FW:<role>". */
const struct wye_flash_ops *fw_flash_ops(const char *role_tag);
void fw_flash_install(uint32_t size);

/* UART link between the two Picos (uart_link.c). */
void uart_link_init(void);
size_t uart_link_read(uint8_t *out, size_t cap);
void uart_link_write(const uint8_t *data, size_t len);      /* waits for room, never drops */
bool uart_link_try_write(const uint8_t *data, size_t len);  /* all or nothing, never waits */
void uart_link_flush(void);                                 /* before a reboot */

/* Crashes and freezes (crash.c): restart instead of hanging, report why to the host. */
struct wye_node;
void wye_crash_start(void);   /* first thing in main: reads the last crash, arms the watchdog */
void wye_crash_feed(void);    /* every pass of the main loop */
void wye_crash_tell(struct wye_node *n, bool host_seen, uint32_t now);

#endif
