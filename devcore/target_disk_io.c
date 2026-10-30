/* Shared drive, target side: block reads (with read-ahead) and writes. */
#include "target.h"
#include <string.h>

static void request_read(struct wye_target *t)
{
    struct wye_disk *d = &t->disk;
    uint8_t p[5];
    memset(d->got, 0, sizeof(d->got));
    wye_put_u32(p, d->cache_lba);
    p[4] = d->cache_count;
    d->read_sent = wye_target_now(t);
    wye_node_send(&t->node, WYE_MSG_DISK_READ, 0, p, sizeof(p));
}

static void send_write(struct wye_target *t)
{
    struct wye_disk *d = &t->disk;
    uint8_t p[6 + WYE_DISK_CHUNK];
    for (uint16_t off = 0; off < WYE_DISK_BLOCK; off += WYE_DISK_CHUNK) {
        uint16_t n = WYE_DISK_BLOCK - off < WYE_DISK_CHUNK ? (uint16_t)(WYE_DISK_BLOCK - off) : WYE_DISK_CHUNK;
        wye_put_u32(p, d->write_lba);
        wye_put_u16(p + 4, off);
        memcpy(p + 6, d->wbuf + off, n);
        wye_node_send(&t->node, WYE_MSG_DISK_WRITE, 0, p, 6u + n);
    }
    d->write_sent = wye_target_now(t);
}

int32_t wye_disk_read(struct wye_target *t, uint32_t lba, uint8_t *out)
{
    struct wye_disk *d = &t->disk;
    if (!wye_disk_ready(d) || lba >= d->blocks) {
        return -1;
    }
    bool cached = lba >= d->cache_lba && lba < d->cache_lba + d->cache_count;
    if (cached && d->got[lba - d->cache_lba] == WYE_DISK_BLOCK) {
        memcpy(out, d->cache[lba - d->cache_lba], WYE_DISK_BLOCK);
        return WYE_DISK_BLOCK;
    }
    uint32_t t_now = wye_target_now(t);
    if (!d->reading || !cached) {
        uint32_t left = d->blocks - lba;
        d->cache_lba = lba;
        d->cache_count = (uint8_t)(left < WYE_DISK_READAHEAD ? left : WYE_DISK_READAHEAD);
        d->reading = true;
        d->read_started = t_now;
        request_read(t);
    } else if (t_now - d->read_started > WYE_DISK_TIMEOUT_MS) {
        d->reading = false;
        d->cache_count = 0;
        return -1;
    } else if (t_now - d->read_sent > WYE_DISK_RETRY_MS) {
        request_read(t);
    }
    return 0;
}

int32_t wye_disk_write(struct wye_target *t, uint32_t lba, const uint8_t *data)
{
    struct wye_disk *d = &t->disk;
    uint32_t t_now = wye_target_now(t);
    if (!wye_disk_ready(d) || lba >= d->blocks) {
        return -1;
    }
    if (!d->writing) {
        memcpy(d->wbuf, data, WYE_DISK_BLOCK);
        d->write_lba = lba;
        d->writing = true;
        d->write_acked = d->write_failed = false;
        d->write_started = t_now;
        send_write(t);
        return 0;
    }
    if (d->write_acked || d->write_failed || t_now - d->write_started > WYE_DISK_TIMEOUT_MS) {
        bool ok = d->write_acked;
        d->writing = false;
        d->last_write = t_now;
        d->wrote = true;
        if (ok && lba >= d->cache_lba && lba < d->cache_lba + d->cache_count) {
            memcpy(d->cache[lba - d->cache_lba], d->wbuf, WYE_DISK_BLOCK); /* keep the cache true */
        }
        return ok ? (int32_t)WYE_DISK_BLOCK : -1;
    }
    if (t_now - d->write_sent > WYE_DISK_RETRY_MS) {
        send_write(t);
    }
    return 0;
}

void wye_disk_eject(struct wye_target *t)
{
    t->disk.ejected = true;
    t->disk.reading = false;
}
