/* Shared drive, target side: host messages, flags and the medium state. */
#include "target.h"
#include <string.h>

void wye_disk_init(struct wye_disk *d)
{
    memset(d, 0, sizeof(*d));
}

bool wye_disk_ready(const struct wye_disk *d)
{
    return d->present && !d->ejected && d->blocks > 0;
}

uint8_t wye_disk_flags(const struct wye_target *t)
{
    const struct wye_disk *d = &t->disk;
    bool busy = d->writing || (d->wrote && wye_target_now(t) - d->last_write < WYE_DISK_IDLE_MS);
    return (uint8_t)((d->present ? WYE_DISK_PRESENT : 0) | (d->ejected ? WYE_DISK_EJECTED : 0) |
                     (busy ? WYE_DISK_BUSY : 0));
}

static void report(struct wye_target *t)
{
    uint8_t flags = wye_disk_flags(t);
    t->disk.reported = flags;
    wye_node_send(&t->node, WYE_MSG_DISK_STATUS, 0, &flags, 1);
}

static void set_medium(struct wye_target *t, const struct wye_packet *pkt)
{
    struct wye_disk *d = &t->disk;
    bool present = pkt->payload[0] != 0;
    uint32_t blocks = wye_get_u32(pkt->payload + 1);
    if (present != d->present || blocks != d->blocks) {
        wye_disk_init(d); /* drops cache and anything in flight */
        d->present = present;
        d->blocks = present ? blocks : 0;
        d->changed = present;
    }
    report(t);
}

/* Read-ahead data, accepted strictly in order per block (retries resend everything). */
static void data(struct wye_target *t, const struct wye_packet *pkt)
{
    struct wye_disk *d = &t->disk;
    uint32_t lba = wye_get_u32(pkt->payload);
    uint16_t offset = wye_get_u16(pkt->payload + 4);
    size_t len = (size_t)pkt->len - 6u;
    if (!d->reading || lba < d->cache_lba || lba >= d->cache_lba + d->cache_count) {
        return;
    }
    uint32_t slot = lba - d->cache_lba;
    if (offset != d->got[slot] || offset + len > WYE_DISK_BLOCK) {
        return;
    }
    memcpy(d->cache[slot] + offset, pkt->payload + 6, len);
    d->got[slot] = (uint16_t)(offset + len);
    bool complete = true;
    for (uint32_t i = 0; i < d->cache_count; ++i) {
        complete = complete && d->got[i] == WYE_DISK_BLOCK;
    }
    d->reading = !complete;
}

static void write_ack(struct wye_target *t, const struct wye_packet *pkt)
{
    struct wye_disk *d = &t->disk;
    if (d->writing && wye_get_u32(pkt->payload) == d->write_lba) {
        d->write_acked = pkt->payload[4] == 0;
        d->write_failed = pkt->payload[4] != 0;
    }
}

bool wye_disk_handle(struct wye_target *t, const struct wye_packet *pkt)
{
    if (pkt->type == WYE_MSG_DISK_SET && pkt->len >= 5) {
        set_medium(t, pkt);
    } else if (pkt->type == WYE_MSG_DISK_DATA && pkt->len > 6) {
        data(t, pkt);
    } else if (pkt->type == WYE_MSG_DISK_WACK && pkt->len >= 5) {
        write_ack(t, pkt);
    } else {
        return false;
    }
    return true;
}

void wye_disk_task(struct wye_target *t)
{
    if (wye_disk_flags(t) != t->disk.reported) {
        report(t);
    }
}
