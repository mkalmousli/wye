/* Devices page panels (page_devices.c, devices_*.c). */
#ifndef WYE_UI_DEVICES_H
#define WYE_UI_DEVICES_H

#include "ui.h"

/* Sound tab: two lanes, one per direction, each with its own switch (devices_audio*.c). */
struct devices_lane {
    const char *title, *state;
    int from_icon;
    const char *from_who, *from_device;
    int to_icon;
    const char *to_who, *to_device;
    Color from_tint, to_tint;
    bool on, running;
    float level;
};
void devices_audio(struct wye_ui *ui, Rectangle c);
bool devices_audio_lane(struct wye_ui *ui, Rectangle r, const struct devices_lane *l); /* true: switch flipped */

/* Overview tab: a tile per function with its own switch (devices_overview.c, devices_tile.c). */
struct devices_tile {
    int icon;                 /* enum ui_icon */
    const char *name, *about;
    bool on, available;
    const char *status;
    float level;              /* sound level 0..1, or < 0 for no meter */
    Color tint;
};
bool devices_tile(struct wye_ui *ui, Rectangle r, const struct devices_tile *t); /* true: switch flipped */
void devices_overview(struct wye_ui *ui, Rectangle a);
void devices_set_drive(struct wye_ui *ui, bool on);   /* serve the folder here + the drive on the Pico */

/* An editable copy of the target's function settings (drive and audio switches, drive name). */
void devices_functions_sync(struct wye_ui *ui);
struct wye_functions *devices_functions(void);          /* NULL until the target answered */
bool devices_functions_dirty(const struct wye_ui *ui);
void devices_functions_save(struct wye_ui *ui);         /* the target restarts with them */
int *devices_editing(void);                             /* which text box is being edited */

void devices_input(struct wye_ui *ui, Rectangle r);              /* keyboard & mouse tab */
void devices_folder(struct wye_ui *ui, Rectangle c);
void devices_size_row(struct wye_ui *ui, Rectangle c, float y);
void devices_shortcuts(struct wye_ui *ui, Rectangle c);

#endif
