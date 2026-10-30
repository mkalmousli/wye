/* Small line icons for compact controls (ui_icons.c). */
#ifndef WYE_UI_ICONS_H
#define WYE_UI_ICONS_H

#include "raylib.h"
#include <stdbool.h>

enum ui_icon {
    UI_ICON_CONTROL, UI_ICON_JIGGLE, UI_ICON_PASTE, UI_ICON_FOLDER, UI_ICON_EXPAND, UI_ICON_CLOSE,
    UI_ICON_HOME, UI_ICON_ROUTINE, UI_ICON_KEYBOARD, UI_ICON_CHIP, UI_ICON_GEAR, UI_ICON_INFO,
    UI_ICON_BOLT, UI_ICON_MINI, UI_ICON_SPEAKER, UI_ICON_MIC,
};

void ui_icon(enum ui_icon icon, Vector2 center, Color color, bool active);
void ui_icon_nav(enum ui_icon icon, Vector2 c, Color col);   /* ui_icons_nav.c */
bool ui_icon_button(Rectangle r, enum ui_icon icon, Color color, bool active, bool enabled, bool *hovered);

#endif
