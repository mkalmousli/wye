/* Input payloads: keyboard, relative mouse, absolute mouse. */
#include "wye_msg.h"

#include <string.h>

size_t wye_keyboard_pack(const struct wye_keyboard *k, uint8_t *out)
{
    out[0] = k->modifiers;
    memcpy(&out[1], k->keys, 6);
    return WYE_KEYBOARD_LEN;
}

bool wye_keyboard_unpack(const uint8_t *p, size_t len, struct wye_keyboard *k)
{
    if (len < WYE_KEYBOARD_LEN) {
        return false;
    }
    k->modifiers = p[0];
    memcpy(k->keys, &p[1], 6);
    return true;
}

size_t wye_mouse_pack(const struct wye_mouse *m, uint8_t *out)
{
    out[0] = m->buttons;
    wye_put_u16(&out[1], (uint16_t)m->dx);
    wye_put_u16(&out[3], (uint16_t)m->dy);
    out[5] = (uint8_t)m->wheel;
    out[6] = (uint8_t)m->pan;
    return WYE_MOUSE_LEN;
}

bool wye_mouse_unpack(const uint8_t *p, size_t len, struct wye_mouse *m)
{
    if (len < WYE_MOUSE_LEN) {
        return false;
    }
    m->buttons = p[0];
    m->dx = (int16_t)wye_get_u16(&p[1]);
    m->dy = (int16_t)wye_get_u16(&p[3]);
    m->wheel = (int8_t)p[5];
    m->pan = (int8_t)p[6];
    return true;
}

size_t wye_abs_mouse_pack(const struct wye_abs_mouse *m, uint8_t *out)
{
    out[0] = m->buttons;
    wye_put_u16(&out[1], m->x);
    wye_put_u16(&out[3], m->y);
    out[5] = (uint8_t)m->wheel;
    out[6] = (uint8_t)m->pan;
    return WYE_ABS_MOUSE_LEN;
}

bool wye_abs_mouse_unpack(const uint8_t *p, size_t len, struct wye_abs_mouse *m)
{
    if (len < WYE_ABS_MOUSE_LEN) {
        return false;
    }
    m->buttons = p[0];
    m->x = wye_get_u16(&p[1]);
    m->y = wye_get_u16(&p[3]);
    m->wheel = (int8_t)p[5];
    m->pan = (int8_t)p[6];
    return true;
}
