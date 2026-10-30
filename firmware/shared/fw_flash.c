/*
 * Firmware staging area and installer.
 *
 * The installer runs entirely from RAM with interrupts off: it copies the
 * verified staging area over the running image sector by sector, then
 * resets. The SDK flash routines are RAM-resident and use a RAM copy of
 * boot2, so erasing sector 0 is safe. A power cut mid-install needs BOOTSEL
 * recovery (the app's setup wizard handles that).
 */
#include "board.h"
#include "fwupdate.h"

#include "hardware/flash.h"
#include "hardware/watchdog.h"
#include "hardware/structs/psm.h"
#include "hardware/structs/watchdog.h"
#include "hardware/sync.h"

#include <string.h>

static const char *g_role_tag;
static uint8_t g_copy_buf[FLASH_SECTOR_SIZE];

static bool write_sector(void *ctx, uint32_t offset, const uint8_t *data)
{
    (void)ctx;
    uint32_t irq = save_and_disable_interrupts();
    flash_range_erase(WYE_FLASH_STAGING_OFFSET + offset, FLASH_SECTOR_SIZE);
    flash_range_program(WYE_FLASH_STAGING_OFFSET + offset, data, FLASH_SECTOR_SIZE);
    restore_interrupts(irq);
    return true;
}

static bool read_staged(void *ctx, uint32_t offset, uint8_t *out, size_t len)
{
    (void)ctx;
    memcpy(out, (const uint8_t *)(XIP_BASE + WYE_FLASH_STAGING_OFFSET + offset), len);
    return true;
}

static bool has_role_tag(const uint8_t *img, uint32_t size)
{
    size_t n = strlen(g_role_tag) + 1; /* include NUL: "target" must not match "targetX" */
    for (uint32_t i = 0; i + n <= size; ++i) {
        if (img[i] == 'W' && memcmp(img + i, g_role_tag, n) == 0) {
            return true;
        }
    }
    return false;
}

static bool validate(void *ctx, uint32_t size)
{
    (void)ctx;
    const uint8_t *img = (const uint8_t *)(XIP_BASE + WYE_FLASH_STAGING_OFFSET);
    uint32_t sp, reset;
    memcpy(&sp, img + 0x100, 4);       /* vector table follows the 256-byte boot2 */
    memcpy(&reset, img + 0x104, 4);
    bool sp_ok = sp > 0x20000000u && sp <= 0x20042000u;
    bool reset_ok = (reset & 1u) && reset > XIP_BASE && reset < XIP_BASE + size;
    return size <= WYE_FLASH_APP_MAX && sp_ok && reset_ok && has_role_tag(img, size);
}

static const struct wye_flash_ops g_ops = {
    NULL, WYE_FLASH_STAGING_SIZE, write_sector, read_staged, validate,
};

const struct wye_flash_ops *fw_flash_ops(const char *role_tag)
{
    g_role_tag = role_tag;
    return &g_ops;
}

static void __no_inline_not_in_flash_func(copy_and_reset)(uint32_t size)
{
    const volatile uint8_t *src = (const volatile uint8_t *)(XIP_BASE + WYE_FLASH_STAGING_OFFSET);
    for (uint32_t off = 0; off < size; off += FLASH_SECTOR_SIZE) {
        for (uint32_t i = 0; i < FLASH_SECTOR_SIZE; ++i) {
            g_copy_buf[i] = src[off + i];
        }
        flash_range_erase(off, FLASH_SECTOR_SIZE);
        flash_range_program(off, g_copy_buf, FLASH_SECTOR_SIZE);
    }
    /* A full chip reset, as at power-on. SYSRESETREQ would only reset the cores and leave USB and
       the other peripherals as they were, which sometimes kept the new firmware off the bus. */
    hw_set_bits(&psm_hw->wdsel, PSM_WDSEL_BITS & ~(PSM_WDSEL_ROSC_BITS | PSM_WDSEL_XOSC_BITS));
    watchdog_hw->scratch[4] = 0; /* plain boot from flash */
    watchdog_hw->ctrl = WATCHDOG_CTRL_TRIGGER_BITS;
    for (;;) {
    }
}

void fw_flash_install(uint32_t size)
{
    uart_link_flush();
    board_usb_leave();
    watchdog_disable(); /* copying the image takes longer than the watchdog allows */
    (void)save_and_disable_interrupts();
    copy_and_reset(size);
}
