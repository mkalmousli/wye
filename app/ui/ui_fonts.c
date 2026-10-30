/*
 * Crisp text: every face is rasterised at exactly the pixel size it is drawn
 * at (times the screen's scale), on first use, instead of shrinking one big
 * raster - which made small text thin and blurry. Inter: Regular for body text,
 * SemiBold for emphasis, Bold for headlines.
 */
#include "ui.h"
#include "ui_theme.h"

#include <math.h>

extern const unsigned char wye_font_regular[], wye_font_bold[], wye_font_heavy[];
extern const size_t wye_font_regular_len, wye_font_bold_len, wye_font_heavy_len;

#define MAX_PX 128
#define MIN_SIZE 13.0f /* nothing smaller is comfortable to read */

static Font g_cache[3][MAX_PX + 1];

static float scale(void)
{
    Vector2 s = GetWindowScaleDPI();
    return s.y > 0.5f ? s.y : 1.0f;
}

float ui_font_size(float size)
{
    return size < MIN_SIZE && size >= 9 ? MIN_SIZE : size; /* below 9: artwork labels, drawn as asked */
}

Font ui_font(float size, bool bold)
{
    size = ui_font_size(size);
    int face = !bold ? 0 : size >= 22 ? 2 : 1;
    int px = (int)lroundf(size * scale());
    px = px < 8 ? 8 : px > MAX_PX ? MAX_PX : px;
    Font *f = &g_cache[face][px];
    if (f->texture.id == 0) {
        static const unsigned char *const data[3] = {wye_font_regular, wye_font_bold, wye_font_heavy};
        const size_t len[3] = {wye_font_regular_len, wye_font_bold_len, wye_font_heavy_len};
        int codepoints[224]; /* ASCII + Latin-1: the UI strings and most names */
        for (int i = 0; i < 224; ++i) {
            codepoints[i] = 32 + i;
        }
        SetTraceLogLevel(LOG_ERROR); /* tall glyphs such as '|' only warn: they still render fine */
        *f = len[face] ? LoadFontFromMemory(".ttf", data[face], (int)len[face], px, codepoints, 224)
                       : GetFontDefault();
        SetTraceLogLevel(LOG_WARNING);
        SetTextureFilter(f->texture, TEXTURE_FILTER_BILINEAR);
    }
    return *f;
}

Vector2 ui_measure(const char *s, float size, bool bold)
{
    return MeasureTextEx(ui_font(size, bold), s, ui_font_size(size), 0);
}

/* Whole (screen) pixels: text between pixels is smeared by the filter. */
void ui_draw_text(const char *s, float x, float y, float size, bool bold, Color c)
{
    float k = scale();
    Vector2 at = {roundf(x * k) / k, roundf(y * k) / k};
    DrawTextEx(ui_font(size, bold), s, at, ui_font_size(size), 0, c);
}

void ui_fonts_unload(void)
{
    for (int face = 0; face < 3; ++face) {
        for (int px = 0; px <= MAX_PX; ++px) {
            if (g_cache[face][px].texture.id && g_cache[face][px].texture.id != GetFontDefault().texture.id) {
                UnloadFont(g_cache[face][px]);
            }
            g_cache[face][px] = (Font){0};
        }
    }
}
