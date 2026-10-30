/* SCSI commands TinyUSB leaves to the application. Android and Windows send these to drives. */
#include "tusb.h"
#include <string.h>

#define SCSI_SYNC_CACHE_10 0x35
#define SCSI_SYNC_CACHE_16 0x91
#define SCSI_MODE_SENSE_10 0x5a

int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize)
{
    switch (scsi_cmd[0]) {
    case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
    case SCSI_SYNC_CACHE_10:
    case SCSI_SYNC_CACHE_16:
        return 0; /* every write already reached the host: nothing is cached here */
    case SCSI_MODE_SENSE_10: {
        uint8_t header[8] = {0, 6}; /* 6 more bytes, no block descriptors, not write-protected */
        uint16_t n = bufsize < sizeof(header) ? bufsize : (uint16_t)sizeof(header);
        memcpy(buffer, header, n);
        return n;
    }
    default:
        tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
        return -1;
    }
}
