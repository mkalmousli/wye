#include "link.h"

#include "log.h"
#include "util/util.h"

#include <string.h>

static const char *state_name(enum wye_link_state s)
{
    static const char *names[] = {"searching", "handshake", "legacy", "bridge-only", "ready"};
    return names[s];
}

void wye_link_init(struct wye_link *l, const struct wye_link_transport *tr,
                   const struct wye_link_listener *listener)
{
    memset(l, 0, sizeof(*l));
    l->tr = tr;
    if (listener) {
        l->listener = *listener;
    }
    wye_decoder_init(&l->decoder);
}

void wye_link_set_state(struct wye_link *l, enum wye_link_state s)
{
    enum wye_link_state old = l->info.state;
    if (old == s) {
        return;
    }
    l->info.state = s;
    l->state_since = l->tr->now(l->tr->ctx);
    LOG_I("link: %s -> %s", state_name(old), state_name(s));
    if (l->listener.state) {
        l->listener.state(l->listener.ctx, old, s);
    }
}

void wye_link_close(struct wye_link *l, const char *reason)
{
    if (l->port) {
        LOG_W("link: closing %s: %s", l->info.port, reason);
        void *port = l->port;
        l->port = NULL;
        l->tr->close(l->tr->ctx, port);
    }
    l->write_failed = false;
    memset(&l->info.bridge, 0, sizeof(l->info.bridge));
    memset(&l->info.target, 0, sizeof(l->info.target));
    l->info.port[0] = '\0';
    wye_decoder_init(&l->decoder);
    if (l->info.state != WYE_LINK_LEGACY) {
        wye_link_set_state(l, WYE_LINK_SEARCHING);
    }
}

bool wye_link_target_ready(const struct wye_link *l)
{
    return l->info.state == WYE_LINK_READY;
}

void wye_link_process(struct wye_link *l, const uint8_t *data, size_t len)
{
    uint64_t now = l->tr->now(l->tr->ctx);
    if (l->write_failed) {
        wye_link_close(l, "write failed");
        l->next_scan = now + WYE_LINK_TIMING_SCAN_MS;
        return;
    }
    struct wye_packet pkt;
    for (size_t i = 0; i < len && l->port; ++i) {
        enum wye_decode r = wye_decoder_feed(&l->decoder, data[i], &pkt);
        if (r == WYE_DECODE_PACKET) {
            wye_link_handle(l, &pkt, now);
        } else if (r == WYE_DECODE_ERROR) {
            l->info.rx_errors++;
        }
    }
    if (!l->port) {
        wye_link_scan(l, now);
    } else {
        wye_link_timers(l, now);
    }
}
