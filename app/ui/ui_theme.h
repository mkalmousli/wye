/* Colour palette and metrics shared by all pages. */
#ifndef WYE_UI_THEME_H
#define WYE_UI_THEME_H

#include "raylib.h"

#define UI_BG (Color){10, 13, 20, 255}
#define UI_BG2 (Color){15, 19, 30, 255}         /* bottom of the background gradient */
#define UI_SIDEBAR (Color){13, 17, 26, 255}
#define UI_CARD (Color){19, 24, 36, 255}
#define UI_CARD_HI (Color){27, 33, 49, 255}     /* raised: buttons, hovered rows */
#define UI_CARD_EDGE (Color){37, 45, 65, 255}
#define UI_TEXT (Color){236, 240, 248, 255}
#define UI_MUTED (Color){150, 161, 184, 255}
#define UI_ACCENT (Color){79, 140, 255, 255}
#define UI_ACCENT2 (Color){34, 211, 238, 255}    /* cyan end of accent gradients */
#define UI_OK (Color){52, 211, 153, 255}
#define UI_WARN (Color){251, 191, 36, 255}
#define UI_BAD (Color){248, 113, 113, 255}
#define UI_TARGET (Color){168, 85, 247, 255}     /* "controlling the other computer" */
#define UI_TARGET2 (Color){236, 72, 153, 255}

#define UI_FONT 16.0f
#define UI_FONT_SMALL 14.0f
#define UI_FONT_TITLE 28.0f
#define UI_FONT_HERO 40.0f
#define UI_PAD 18.0f
#define UI_ROW 36.0f
#define UI_SIDEBAR_W 88.0f

#endif
