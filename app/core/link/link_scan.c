/* Finding and opening a bridge. */
#include "link.h"

#include "log.h"
#include "util/util.h"

#include <string.h>

#define MAX_PORTS 32

void wye_link_blacklist(struct wye_link *l, const char *path, uint64_t until)
{
    size_t oldest = 0;
    for (size_t i = 0; i < WYE_LINK_BLACKLIST; ++i) {
        if (strcmp(l->blacklist[i].path, path) == 0) {
            oldest = i;
            break;
        }
        if (l->blacklist[i].until < l->blacklist[oldest].until) {
            oldest = i;
        }
    }
    wye_strcpy(l->blacklist[oldest].path, sizeof(l->blacklist[oldest].path), path);
    l->blacklist[oldest].until = until;
}

static bool blacklisted(const struct wye_link *l, const char *path, uint64_t now)
{
    for (size_t i = 0; i < WYE_LINK_BLACKLIST; ++i) {
        if (l->blacklist[i].until > now && strcmp(l->blacklist[i].path, path) == 0) {
            return true;
        }
    }
    return false;
}

/* 0 = failed, 1 = opened, 2 = background open in progress. */
static int try_open(struct wye_link *l, const struct plat_port_info *p, bool legacy, uint64_t now)
{
    char err[128] = "";
    void *port = l->tr->open(l->tr->ctx, p->path, err, sizeof(err));
    if (!port) {
        if (l->tr->open_pending && l->tr->open_pending(l->tr->ctx)) {
            l->next_scan = now + 20;
            return 2;
        }
        LOG_W("link: cannot open %s: %s", p->path, err);
        wye_link_blacklist(l, p->path, now + WYE_LINK_TIMING_BLACKLIST_MS);
        return false;
    }
    LOG_I("link: probing %s (%04x:%04x %s)", p->path, p->vid, p->pid, p->product);
    l->port = port;
    l->legacy_candidate = legacy;
    wye_strcpy(l->info.port, sizeof(l->info.port), p->path);
    wye_strcpy(l->info.product, sizeof(l->info.product), p->product);
    wye_decoder_init(&l->decoder);
    l->next_bridge_hello = now;
    wye_link_set_state(l, WYE_LINK_HANDSHAKE);
    return true;
}

void wye_link_scan(struct wye_link *l, uint64_t now)
{
    if (now < l->next_scan) {
        return;
    }
    l->next_scan = now + WYE_LINK_TIMING_SCAN_MS;
    struct plat_port_info ports[MAX_PORTS];
    int n = l->tr->list(l->tr->ctx, ports, MAX_PORTS);
    bool legacy_still_present = false;
    /* Prefer real Wye devices over generic Pico serial ports. */
    for (int pass = 0; pass < 2; ++pass) {
        for (int i = 0; i < n; ++i) {
            enum wye_port_kind kind = l->tr->classify(l->tr->ctx, &ports[i]);
            if (kind == WYE_PORT_LEGACY && strcmp(ports[i].path, l->info.legacy_port) == 0) {
                legacy_still_present = true;
            }
            bool wanted = pass == 0 ? kind == WYE_PORT_NATIVE : kind == WYE_PORT_LEGACY;
            if (wanted && !blacklisted(l, ports[i].path, now) &&
                try_open(l, &ports[i], kind == WYE_PORT_LEGACY, now)) {
                return;
            }
        }
    }
    if (l->info.state == WYE_LINK_LEGACY && !legacy_still_present) {
        l->info.legacy_port[0] = '\0';
        wye_link_set_state(l, WYE_LINK_SEARCHING);
    }
}
