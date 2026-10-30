/* Devices: an editable copy of the target's settings (drive switch and name). */
#include "devices.h"

#include <string.h>

static struct {
    bool loaded;
    struct wye_functions f;
    int editing;
    double requested;
} g;

void devices_functions_sync(struct wye_ui *ui)
{
    if (!g.loaded && ui->st.functions_valid) {
        g.f = ui->st.functions;
        g.loaded = true;
    } else if (!ui->st.functions_valid && GetTime() - g.requested > 2.0) {
        g.requested = GetTime();
        g.loaded = false;
        wye_engine_functions_request();
    }
}

struct wye_functions *devices_functions(void) { return g.loaded ? &g.f : NULL; }
int *devices_editing(void) { return &g.editing; }

bool devices_functions_dirty(const struct wye_ui *ui)
{
    return g.loaded && ui->st.functions_valid && memcmp(&g.f, &ui->st.functions, sizeof(g.f)) != 0;
}

void devices_functions_save(struct wye_ui *ui)
{
    ui_toast(ui, wye_engine_functions_set(&g.f) ? "Saved - the target Pico restarts with the new settings"
                                                : "The target Pico is not connected");
    g.loaded = false;
}
