/* Shared drive constants (messages WYE_MSG_DISK_* in wye_proto.h). */
#ifndef WYE_DISK_H
#define WYE_DISK_H

#define WYE_DISK_BLOCK 512u
#define WYE_DISK_CHUNK 224u
#define WYE_DISK_READAHEAD 4u
enum wye_disk_flag {
    WYE_DISK_PRESENT = 1u << 0,  /* medium inserted (the target PC sees the drive) */
    WYE_DISK_EJECTED = 1u << 1,  /* the target PC ejected it */
    WYE_DISK_BUSY = 1u << 2,     /* writing, or wrote very recently: not safe to remove */
};

#endif
