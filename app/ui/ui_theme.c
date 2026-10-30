/* Fonts (embedded Inter, see ui_fonts.c) and raygui styling. */
#include "ui.h"
#include "ui_theme.h"

#include "raygui.h"

static int rgba(Color c)
{
    return ColorToInt(c);
}

void ui_theme_load(struct wye_ui *ui)
{
    ui->font = ui_font(UI_FONT, false); /* raygui draws its widgets at exactly this size */
    ui->bold = ui_font(UI_FONT, true);
    ui->display = ui_font(UI_FONT_HERO, true);
    GuiLoadStyleDefault();
    GuiSetFont(ui->font);
    GuiSetStyle(DEFAULT, TEXT_SIZE, (int)UI_FONT);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 0);
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, rgba(UI_BG));
    GuiSetStyle(DEFAULT, LINE_COLOR, rgba(UI_CARD_EDGE));
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, rgba(UI_CARD_EDGE));
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, rgba(UI_CARD_HI));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, rgba(UI_TEXT));
    GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, rgba(UI_ACCENT));
    GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, rgba(UI_CARD_EDGE));
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, rgba(UI_TEXT));
    GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, rgba(UI_ACCENT));
    GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, rgba(UI_ACCENT));
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, rgba(WHITE));
    GuiSetStyle(DEFAULT, BORDER_COLOR_DISABLED, rgba(UI_CARD_EDGE));
    GuiSetStyle(DEFAULT, BASE_COLOR_DISABLED, rgba(UI_CARD));
    GuiSetStyle(DEFAULT, TEXT_COLOR_DISABLED, rgba(UI_MUTED));
    GuiSetStyle(DEFAULT, BORDER_WIDTH, 1);
    GuiSetStyle(TEXTBOX, TEXT_PADDING, 8);
    GuiSetStyle(SLIDER, BASE_COLOR_PRESSED, rgba(UI_ACCENT));
    GuiSetStyle(TOGGLE, BASE_COLOR_PRESSED, rgba(UI_ACCENT));    /* selected segment */
    GuiSetStyle(TOGGLE, BORDER_COLOR_PRESSED, rgba(UI_ACCENT));
    GuiSetStyle(TEXTBOX, BASE_COLOR_PRESSED, rgba(UI_BG));       /* editing */
    GuiSetStyle(TEXTBOX, BORDER_COLOR_PRESSED, rgba(UI_ACCENT));
}

void ui_theme_unload(struct wye_ui *ui)
{
    ui_fonts_unload();
    ui->font = ui->bold = ui->display = (Font){0};
}
