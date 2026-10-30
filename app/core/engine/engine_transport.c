/* Link transport backed by real serial ports, plus device classification. */
#include "engine_internal.h"

#include "product.h"
#include "util/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t now(void *ctx) { (void)ctx; return plat_now_ms(); }

static int list(void *ctx, struct plat_port_info *out, int max)
{
    (void)ctx;
    return plat_serial_list(out, max);
}

/* bridge_ids: "vid:pid,vid:pid" in hex, for rebranded/reallocated identities. */
static bool in_id_list(const char *list, uint16_t vid, uint16_t pid)
{
    char buf[128];
    wye_strcpy(buf, sizeof(buf), list);
    for (char *tok = strtok(buf, ", "); tok; tok = strtok(NULL, ", ")) {
        unsigned v, p;
        if (sscanf(tok, "%x:%x", &v, &p) == 2 && v == vid && p == pid) {
            return true;
        }
    }
    return false;
}

static enum wye_port_kind classify(void *ctx, const struct plat_port_info *p)
{
    struct wye_engine *e = ctx;
    bool default_id = p->vid == WYE_BRIDGE_VID && p->pid == WYE_BRIDGE_PID;
    if (default_id || wye_contains_ci(p->product, WYE_PRODUCT_NAME) ||
        in_id_list(e->cfg.bridge_ids, p->vid, p->pid)) {
        return WYE_PORT_NATIVE;
    }
    bool generic_pico = p->vid == WYE_LEGACY_PICO_VID && p->pid == WYE_LEGACY_PICO_PID;
    return generic_pico && e->cfg.probe_generic_pico ? WYE_PORT_LEGACY : WYE_PORT_IGNORE;
}

static void *open_port(void *ctx, const char *path, char *err, size_t errlen)
{
    struct wye_engine *e = ctx;
    return wye_serial_worker_open(e->serial_worker, path, err, errlen);
}

static bool open_pending(void *ctx)
{
    return wye_serial_worker_pending(((struct wye_engine *)ctx)->serial_worker);
}

static int write_port(void *ctx, void *port, const uint8_t *data, size_t len)
{
    (void)ctx;
    return plat_serial_write(port, data, len);
}

static void close_port(void *ctx, void *port)
{
    wye_serial_worker_close(((struct wye_engine *)ctx)->serial_worker, port);
}

void wye_engine_transport_init(struct wye_engine *e)
{
    const struct wye_serial_ops ops = {plat_serial_open, plat_serial_close};
    e->serial_worker = wye_serial_worker_new(e->wake, &ops);
    e->transport = (struct wye_link_transport){e, now, list, classify, open_port, write_port, close_port, open_pending};
    struct wye_link_listener listener = {e, wye_engine_on_packet, wye_engine_on_state};
    wye_link_init(&e->link, &e->transport, &listener);
}
