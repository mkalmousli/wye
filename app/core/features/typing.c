#include "typing.h"

#include "wye_msg.h"
#include "wye_proto.h"

static bool push(struct wye_typing *t, uint8_t modifiers, uint8_t key)
{
    if (t->count >= WYE_TYPING_QUEUE) {
        return false;
    }
    t->steps[(t->head + t->count) % WYE_TYPING_QUEUE] = (struct wye_typing_step){modifiers, key};
    t->count++;
    return true;
}

bool wye_typing_chord(struct wye_typing *t, uint8_t modifiers, uint8_t key)
{
    if (t->count + 2 > WYE_TYPING_QUEUE) {
        return false;
    }
    push(t, modifiers, key);
    push(t, 0, 0);
    return true;
}

size_t wye_typing_text(struct wye_typing *t, const char *text)
{
    size_t queued = 0;
    for (; *text; ++text) {
        uint8_t mods, key;
        if (*text == '\r' && text[1] == '\n') {
            continue; /* CRLF -> one Enter */
        }
        if (!wye_ascii_to_hid(*text, &mods, &key)) {
            continue;
        }
        if (!wye_typing_chord(t, mods, key)) {
            break;
        }
        queued++;
    }
    return queued;
}

void wye_typing_tick(struct wye_typing *t, uint64_t now, const struct wye_sender *out)
{
    if (t->count == 0 || now < t->next_at) {
        return;
    }
    const struct wye_typing_step *s = &t->steps[t->head];
    struct wye_keyboard k = {s->modifiers, {s->key, 0, 0, 0, 0, 0}};
    uint8_t buf[WYE_KEYBOARD_LEN];
    if (!wye_send(out, WYE_MSG_KEYBOARD, buf, wye_keyboard_pack(&k, buf))) {
        wye_typing_cancel(t); /* link gone: never resume half-typed text later */
        return;
    }
    t->head = (t->head + 1) % WYE_TYPING_QUEUE;
    t->count--;
    t->next_at = now + WYE_TYPING_STEP_MS;
}

void wye_typing_cancel(struct wye_typing *t)
{
    t->head = 0;
    t->count = 0;
}

bool wye_typing_busy(const struct wye_typing *t)
{
    return t->count > 0;
}
