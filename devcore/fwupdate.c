#include "fwupdate.h"
#include "wye_codec.h"

#include <string.h>

void wye_fwupdate_reset(struct wye_fwupdate *fw)
{
    fw->active = false;
    fw->ready = false;
    fw->size = 0;
    fw->crc = 0;
    fw->next = 0;
    fw->crc_running = 0;
    memset(fw->sector, 0xff, sizeof(fw->sector));
}

bool wye_fwupdate_flush(struct wye_fwupdate *fw, const struct wye_flash_ops *ops, uint32_t upto)
{
    uint32_t base = (upto - 1u) & ~(WYE_FW_SECTOR - 1u);
    uint32_t fill = upto - base;
    memset(fw->sector + fill, 0xff, WYE_FW_SECTOR - fill);
    bool ok = ops->write_sector(ops->ctx, base, fw->sector);
    memset(fw->sector, 0xff, sizeof(fw->sector));
    return ok;
}

uint8_t wye_fwupdate_begin(struct wye_fwupdate *fw, const struct wye_flash_ops *ops, uint32_t size,
                           uint32_t crc)
{
    wye_fwupdate_reset(fw);
    if (!ops) {
        return WYE_FW_ERR_STATE;
    }
    if (size == 0 || size > ops->staging_size) {
        return WYE_FW_ERR_SIZE;
    }
    fw->active = true;
    fw->size = size;
    fw->crc = crc;
    return WYE_FW_OK;
}

uint8_t wye_fwupdate_data(struct wye_fwupdate *fw, const struct wye_flash_ops *ops, uint32_t offset,
                          const uint8_t *data, size_t len, uint32_t *next_offset)
{
    *next_offset = fw->next;
    if (!fw->active || fw->ready) {
        return WYE_FW_ERR_STATE;
    }
    if (offset < fw->next && offset + len <= fw->next) {
        return WYE_FW_OK; /* duplicate of data we already have */
    }
    if (offset != fw->next) {
        return WYE_FW_ERR_OFFSET;
    }
    if (len == 0 || (uint64_t)offset + len > fw->size) {
        return WYE_FW_ERR_SIZE;
    }
    for (size_t i = 0; i < len; ++i) {
        uint32_t pos = offset + (uint32_t)i;
        fw->sector[pos % WYE_FW_SECTOR] = data[i];
        if ((pos + 1u) % WYE_FW_SECTOR == 0 && !wye_fwupdate_flush(fw, ops, pos + 1u)) {
            wye_fwupdate_reset(fw);
            return WYE_FW_ERR_FLASH;
        }
    }
    fw->crc_running = wye_crc32_update(fw->crc_running, data, len);
    fw->next = offset + (uint32_t)len;
    *next_offset = fw->next;
    return WYE_FW_OK;
}
