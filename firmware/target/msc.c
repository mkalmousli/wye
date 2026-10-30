/*
 * USB mass storage for the shared drive. Blocks come from the host over the
 * link, so a read or write may wait for the UART. TinyUSB's "not ready yet"
 * return re-queues the event inside tud_task() and would starve the main
 * loop, so these callbacks wait here instead, pumping the link meanwhile.
 */
#include "board.h"
#include "target_main.h"
#include "tusb.h"
#include <string.h>

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
    (void)lun;
    const struct wye_functions *f = functions_current();
    memset(vendor_id, ' ', 8); /* space padded, as SCSI wants */
    memset(product_id, ' ', 16);
    for (int i = 0; i < 8 && f->drive_vendor[i]; ++i) {
        vendor_id[i] = (uint8_t)f->drive_vendor[i];
    }
    for (int i = 0; i < 16 && f->drive_product[i]; ++i) {
        product_id[i] = (uint8_t)f->drive_product[i];
    }
    memcpy(product_rev, "1.0 ", 4);
}

bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
    struct wye_disk *d = &target_get()->disk;
    if (!wye_disk_ready(d)) {
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00); /* medium not present */
        return false;
    }
    if (d->changed) {
        d->changed = false;
        tud_msc_set_sense(lun, SCSI_SENSE_UNIT_ATTENTION, 0x28, 0x00); /* medium may have changed */
        return false;
    }
    return true;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size)
{
    (void)lun;
    const struct wye_disk *d = &target_get()->disk;
    *block_count = wye_disk_ready(d) ? d->blocks : 0;
    *block_size = WYE_DISK_BLOCK;
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
    (void)lun;
    (void)power_condition;
    if (load_eject && !start) {
        wye_disk_eject(target_get()); /* "Eject" on the target PC: the host may take it back */
    }
    return true;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)
{
    (void)lun;
    if (offset != 0 || bufsize < WYE_DISK_BLOCK) {
        return -1; /* CFG_TUD_MSC_EP_BUFSIZE is one block */
    }
    for (;;) {
        int32_t r = wye_disk_read(target_get(), lba, buffer);
        if (r != 0) {
            return r;
        }
        target_pump();
    }
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize)
{
    (void)lun;
    if (offset != 0 || bufsize < WYE_DISK_BLOCK) {
        return -1;
    }
    for (;;) {
        int32_t r = wye_disk_write(target_get(), lba, buffer);
        if (r != 0) {
            return r;
        }
        target_pump();
    }
}
