#include "board.h"
#include "tusb.h"
#include "wye_proto.h"

#include "hardware/flash.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "hardware/sync.h"

#include <string.h>

#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

uint32_t board_now_ms(void)
{
    return to_ms_since_boot(get_absolute_time());
}

void board_hello(struct wye_hello_ack *a, uint8_t role, uint16_t caps)
{
    memset(a, 0, sizeof(*a));
    a->proto_version = WYE_PROTO_VERSION;
    a->role = role;
    a->fw_major = WYE_VERSION_MAJOR;
    a->fw_minor = WYE_VERSION_MINOR;
    a->fw_patch = WYE_VERSION_PATCH;
    a->board = 1;
    a->caps = caps;
    a->build_id = WYE_BUILD_ID;
    uint32_t irq = save_and_disable_interrupts();
    flash_get_unique_id(a->uid);
    restore_interrupts(irq);
}

/* Leave the bus properly before restarting: some hosts (Intel xHCI) lock up the port when a
   device drops and reconnects within a few hundred milliseconds. */
void board_usb_leave(void)
{
    tud_disconnect();
    sleep_ms(300);
}

void board_reboot(uint8_t mode)
{
    uart_link_flush();
    board_usb_leave();
    if (mode == WYE_REBOOT_BOOTSEL) {
        reset_usb_boot(0, 0);
    }
    watchdog_reboot(0, 0, 0);
    for (;;) {
        tight_loop_contents();
    }
}

void board_led(bool on)
{
    static bool init;
    if (!init) {
        gpio_init(PICO_DEFAULT_LED_PIN);
        gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
        init = true;
    }
    gpio_put(PICO_DEFAULT_LED_PIN, on);
}
