#include "sim_transport.h"

#include "util/util.h"

#include <string.h>

static uint64_t now(void *ctx) { return ((struct sim_transport *)ctx)->dev.now; }

static int list(void *ctx, struct plat_port_info *out, int max)
{
    struct sim_transport *t = ctx;
    if (!t->plugged || max < 1) {
        return 0;
    }
    memset(out, 0, sizeof(*out));
    wye_strcpy(out->path, sizeof(out->path), "SIM0");
    out->vid = t->legacy_ids ? 0x2e8a : 0x1209;
    out->pid = t->legacy_ids ? 0x000a : 0x0001;
    return 1;
}

static enum wye_port_kind classify(void *ctx, const struct plat_port_info *p)
{
    (void)ctx;
    return p->vid == 0x1209 ? WYE_PORT_NATIVE : WYE_PORT_LEGACY;
}

static void *open_port(void *ctx, const char *path, char *err, size_t errlen)
{
    struct sim_transport *t = ctx;
    (void)path;
    if (!t->plugged) {
        wye_strcpy(err, errlen, "no such device");
        return NULL;
    }
    t->opens++;
    t->dev.host_present = true;
    return t;
}

static int write_port(void *ctx, void *port, const uint8_t *data, size_t len)
{
    struct sim_transport *t = ctx;
    (void)port;
    if (!t->plugged) {
        return -1;
    }
    if (!t->silent) {
        sim_host_write(&t->dev, data, len);
    }
    return (int)len;
}

static void close_port(void *ctx, void *port)
{
    (void)port;
    ((struct sim_transport *)ctx)->dev.host_present = false;
}

void sim_transport_init(struct sim_transport *t)
{
    memset(t, 0, sizeof(*t));
    sim_init(&t->dev);
    t->dev.host_present = false;
    t->plugged = true;
    t->ops = (struct wye_link_transport){t, now, list, classify, open_port, write_port, close_port, NULL};
}

void sim_transport_run(struct sim_transport *t, struct wye_link *link, uint32_t ms)
{
    uint8_t buf[4096];
    for (uint32_t i = 0; i < ms; ++i) {
        sim_advance(&t->dev, 1);
        size_t n = sim_host_read(&t->dev, buf, sizeof(buf));
        if (link->port && !t->plugged) {
            wye_link_close(link, "device unplugged"); /* what a failing read reports */
        }
        wye_link_process(link, buf, t->silent || !link->port ? 0 : n);
    }
}
