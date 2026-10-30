/* Motion and light for the UI: eased per-widget values, glows, shadows, keycaps. */
#ifndef WYE_UI_FX_H
#define WYE_UI_FX_H

#include "raylib.h"
#include <stdbool.h>
#include <stdint.h>

struct wye_ui;

/* A value that eases toward target (per id, frame-rate independent). speed ~ 1/seconds. */
float ui_tween(uint32_t id, float target, float speed);
/* An id from a label and where it is drawn (same label elsewhere = another widget). */
uint32_t ui_id(const char *label, float x, float y);

Color ui_mix(Color a, Color b, float t);
void ui_background(void);                                 /* window gradient with soft light */
void ui_shadow(Rectangle r, float roundness);             /* under raised surfaces */
void ui_glow(Vector2 c, float radius, Color col, float strength);
void ui_gradient_bar(Rectangle r, Color from, Color to);  /* rounded, left to right */
/* A keyboard key: label in the middle, a darker lip underneath; lit = glowing. */
void ui_keycap(struct wye_ui *ui, Rectangle r, const char *label, bool lit, Color tint);
float ui_pulse(float hz);                                 /* 0..1, smooth */

#endif
