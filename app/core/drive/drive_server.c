/* Serving the drive image's blocks to the target Pico, and its status reports. */
#include "drive.h"
#include "util/util.h"
#include <string.h>

void wye_drive_send_set(struct wye_drive *d, bool present, uint64_t now, const struct wye_sender *out)
{
    uint8_t p[5] = {present ? 1 : 0};
    wye_put_u32(p + 1, present ? d->blocks : 0);
    wye_send(out, WYE_MSG_DISK_SET, p, sizeof(p));
    d->set_sent = now ? now : 1;
}

static bool serving(const struct wye_drive *d)
{
    return d->img && d->state != WYE_DRIVE_OFF && d->state != WYE_DRIVE_PACKING;
}

static void read_blocks(struct wye_drive *d, uint32_t lba, uint8_t count, const struct wye_sender *out)
{
    uint8_t block[WYE_DISK_BLOCK], p[6 + WYE_DISK_CHUNK];
    for (uint8_t i = 0; i < count && i < WYE_DISK_READAHEAD; ++i, ++lba) {
        memset(block, 0, sizeof(block));
        if (lba >= d->blocks || !wye_fseek64(d->img, (uint64_t)lba * WYE_DISK_BLOCK) ||
            fread(block, 1, sizeof(block), d->img) != sizeof(block)) {
            continue; /* the target retries, then reports a read error */
        }
        for (uint16_t off = 0; off < WYE_DISK_BLOCK; off += WYE_DISK_CHUNK) {
            uint16_t n = WYE_DISK_BLOCK - off < WYE_DISK_CHUNK ? (uint16_t)(WYE_DISK_BLOCK - off) : WYE_DISK_CHUNK;
            wye_put_u32(p, lba);
            wye_put_u16(p + 4, off);
            memcpy(p + 6, block + off, n);
            wye_send(out, WYE_MSG_DISK_DATA, p, 6u + n);
        }
        d->reads++;
    }
}

/* Chunks arrive in order; a resent block starts again at offset 0. */
static void write_chunk(struct wye_drive *d, const struct wye_packet *pkt, const struct wye_sender *out)
{
    uint32_t lba = wye_get_u32(pkt->payload);
    uint16_t off = wye_get_u16(pkt->payload + 4);
    size_t n = (size_t)pkt->len - 6u;
    if (off == 0) {
        d->wlba = lba;
        d->wgot = 0;
    }
    if (lba != d->wlba || off != d->wgot || off + n > WYE_DISK_BLOCK) {
        return;
    }
    memcpy(d->wbuf + off, pkt->payload + 6, n);
    d->wgot = (uint16_t)(off + n);
    if (d->wgot < WYE_DISK_BLOCK) {
        return;
    }
    bool ok = serving(d) && lba < d->blocks && wye_fseek64(d->img, (uint64_t)lba * WYE_DISK_BLOCK) &&
              fwrite(d->wbuf, 1, WYE_DISK_BLOCK, d->img) == WYE_DISK_BLOCK && fflush(d->img) == 0;
    uint8_t ack[5];
    wye_put_u32(ack, lba);
    ack[4] = ok ? 0 : 1;
    wye_send(out, WYE_MSG_DISK_WACK, ack, sizeof(ack));
    d->wgot = 0;
    d->writes += ok;
}

void wye_drive_packet(struct wye_drive *d, const struct wye_packet *pkt, const struct wye_sender *out)
{
    if (pkt->type == WYE_MSG_DISK_STATUS && pkt->len >= 1) {
        d->flags = pkt->payload[0];
    } else if (pkt->type == WYE_MSG_DISK_READ && pkt->len >= 5 && serving(d)) {
        read_blocks(d, wye_get_u32(pkt->payload), pkt->payload[4], out);
    } else if (pkt->type == WYE_MSG_DISK_WRITE && pkt->len > 6) {
        write_chunk(d, pkt, out);
    }
}
