/*
 * Your own name and colour for each Pico, kept on this computer and tied to the
 * board's flash ID, so they stay with the board whichever port or role it has.
 * What the other computer sees is the USB identity, set separately on the device.
 */
#ifndef WYE_PICO_NAMES_H
#define WYE_PICO_NAMES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WYE_PICO_NAME_MAX 24   /* bytes of UTF-8 */
#define WYE_PICO_NAMES 32      /* boards remembered; the least recently changed goes first */
#define WYE_PICO_COLORS 7      /* 0: the role's own colour */

struct wye_pico_name {
    uint8_t uid[8];
    uint8_t color;
    char name[WYE_PICO_NAME_MAX + 1];  /* empty: the role's own name */
};

struct wye_pico_names {
    struct wye_pico_name items[WYE_PICO_NAMES];
    int count;
};

void wye_pico_names_load(struct wye_pico_names *n);
bool wye_pico_names_save(const struct wye_pico_names *n);
const struct wye_pico_name *wye_pico_names_find(const struct wye_pico_names *n, const uint8_t uid[8]);
/* An empty name with colour 0 forgets the board. False if the board has no ID. */
bool wye_pico_names_set(struct wye_pico_names *n, const uint8_t uid[8], const char *name, int color);
/* One line, no surrounding spaces, cut to WYE_PICO_NAME_MAX on a character boundary. */
void wye_pico_name_clean(char *name);

#endif
