/* Maintenance enums: reboot modes and firmware update status codes. */
#ifndef WYE_MAINT_H
#define WYE_MAINT_H

enum wye_reboot { WYE_REBOOT_NORMAL = 0, WYE_REBOOT_BOOTSEL = 1 };

enum wye_fw_status {
    WYE_FW_OK = 0,
    WYE_FW_ERR_STATE,   /* DATA/COMMIT without BEGIN */
    WYE_FW_ERR_SIZE,    /* image too large */
    WYE_FW_ERR_OFFSET,  /* out of order; next_offset says what is wanted */
    WYE_FW_ERR_CRC,
    WYE_FW_ERR_IMAGE,   /* failed sanity checks */
    WYE_FW_ERR_FLASH,
};

#endif
