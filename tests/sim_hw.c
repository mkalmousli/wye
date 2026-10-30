/* Fake HID endpoints and flash for the device simulator. */
#include "sim.h"

#include <string.h>

static struct sim_report *add(struct sim_device *s, char kind)
{
    if (s->report_count >= SIM_MAX_REPORTS) {
        return NULL;
    }
    struct sim_report *r = &s->reports[s->report_count++];
    memset(r, 0, sizeof(*r));
    r->kind = kind;
    return r;
}

static bool ready(void *ctx) { return ((struct sim_device *)ctx)->hid_ready; }
static int16_t rel_max(void *ctx) { return ((struct sim_device *)ctx)->rel_max; }
static void wake(void *ctx) { ((struct sim_device *)ctx)->wakeups++; }

static bool kbd_send(void *ctx, const struct wye_keyboard *k)
{
    struct sim_report *r = add(ctx, 'k');
    if (r) {
        r->kbd = *k;
    }
    return r != NULL;
}

static bool rel_send(void *ctx, uint8_t buttons, int16_t dx, int16_t dy, int8_t wheel, int8_t pan)
{
    (void)wheel;
    (void)pan;
    struct sim_report *r = add(ctx, 'r');
    if (r) {
        r->buttons = buttons;
        r->dx = dx;
        r->dy = dy;
    }
    return r != NULL;
}

static bool abs_send(void *ctx, const struct wye_abs_mouse *m)
{
    struct sim_report *r = add(ctx, 'a');
    if (r) {
        r->buttons = m->buttons;
        r->x = m->x;
        r->y = m->y;
    }
    return r != NULL;
}

static bool usage_send(struct sim_device *s, char kind, uint16_t usage)
{
    struct sim_report *r = add(s, kind);
    if (r) {
        r->usage = usage;
    }
    return r != NULL;
}

static bool consumer_send(void *ctx, uint16_t u) { return usage_send(ctx, 'c', u); }
static bool system_send(void *ctx, uint8_t u) { return usage_send(ctx, 's', u); }

void sim_setup_hid(struct sim_device *s)
{
    s->hid = (struct wye_target_hid){
        .ctx = s, .kbd_ready = ready, .kbd_send = kbd_send, .ptr_ready = ready,
        .rel_send = rel_send, .abs_send = abs_send, .rel_max = rel_max,
        .extra_ready = ready, .consumer_send = consumer_send, .system_send = system_send,
        .remote_wakeup = wake,
    };
}

static bool flash_write(void *ctx, uint32_t off, const uint8_t *data)
{
    struct sim_flash *f = ctx;
    if (off + WYE_FW_SECTOR > SIM_FLASH_SIZE) {
        return false;
    }
    memcpy(f->mem + off, data, WYE_FW_SECTOR);
    return true;
}

static bool flash_read(void *ctx, uint32_t off, uint8_t *out, size_t len)
{
    memcpy(out, ((struct sim_flash *)ctx)->mem + off, len);
    return true;
}

void sim_setup_flash(struct sim_flash *f)
{
    memset(f->mem, 0xff, sizeof(f->mem));
    f->ops = (struct wye_flash_ops){f, SIM_FLASH_SIZE, flash_write, flash_read, NULL};
}
