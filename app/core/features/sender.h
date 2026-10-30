/* How feature modules emit protocol messages to the target. */
#ifndef WYE_SENDER_H
#define WYE_SENDER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct wye_sender {
    void *ctx;
    bool (*send)(void *ctx, uint8_t type, const void *payload, size_t len);
};

static inline bool wye_send(const struct wye_sender *s, uint8_t type, const void *p, size_t len)
{
    return s->send(s->ctx, type, p, len);
}

#endif
