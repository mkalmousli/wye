/* The home link map's nodes (home_map_node.c) used by home_map.c. */
#ifndef WYE_HOME_MAP_H
#define WYE_HOME_MAP_H

#include "ui.h"

enum map_kind { MAP_COMPUTER, MAP_PICO };
enum map_state { MAP_OFF, MAP_OK, MAP_WARN, MAP_BAD };

struct map_node {
    enum map_kind kind;
    const char *title, *detail;
    enum map_state state;
    Color glyph;     /* a Pico's own colour; unset: plain */
};

void map_node_draw(struct wye_ui *ui, const struct map_node *n, Vector2 at, bool controlled);
void map_leds(struct wye_ui *ui, Vector2 at, uint8_t leds);
const char *map_version(const struct wye_node_info *n);

#endif
