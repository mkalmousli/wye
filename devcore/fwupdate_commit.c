/* Final verification of a staged firmware image. */
#include "fwupdate.h"
#include "wye_codec.h"

static uint8_t fail(struct wye_fwupdate *fw, uint8_t status)
{
    wye_fwupdate_reset(fw);
    return status;
}

static bool flash_crc(const struct wye_flash_ops *ops, uint32_t size, uint32_t *crc)
{
    uint8_t buf[256];
    *crc = 0;
    for (uint32_t off = 0; off < size; off += sizeof(buf)) {
        size_t n = size - off < sizeof(buf) ? size - off : sizeof(buf);
        if (!ops->read(ops->ctx, off, buf, n)) {
            return false;
        }
        *crc = wye_crc32_update(*crc, buf, n);
    }
    return true;
}

uint8_t wye_fwupdate_commit(struct wye_fwupdate *fw, const struct wye_flash_ops *ops)
{
    if (fw->ready) {
        return WYE_FW_OK; /* retransmitted COMMIT */
    }
    if (!fw->active || fw->next != fw->size) {
        return WYE_FW_ERR_STATE;
    }
    if (fw->size % WYE_FW_SECTOR != 0 && !wye_fwupdate_flush(fw, ops, fw->size)) {
        return fail(fw, WYE_FW_ERR_FLASH);
    }
    if (fw->crc_running != fw->crc) {
        return fail(fw, WYE_FW_ERR_CRC);
    }
    uint32_t crc;
    if (!flash_crc(ops, fw->size, &crc)) {
        return fail(fw, WYE_FW_ERR_FLASH);
    }
    if (crc != fw->crc) {
        return fail(fw, WYE_FW_ERR_CRC);
    }
    if (ops->validate && !ops->validate(ops->ctx, fw->size)) {
        return fail(fw, WYE_FW_ERR_IMAGE);
    }
    fw->ready = true;
    return WYE_FW_OK;
}
