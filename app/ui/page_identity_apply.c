/* USB identity: sending a new identity to a Pico. */
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Sends the identity; true once the Pico accepted it (it restarts with it). */
bool identity_apply(struct wye_ui *ui, int node, const char *vid, const char *pid, char fields[3][WYE_ID_STR_MAX])
{
    struct wye_identity id = {0};
    id.vid = (uint16_t)strtoul(vid, NULL, 16);
    id.pid = (uint16_t)strtoul(pid, NULL, 16);
    snprintf(id.manufacturer, sizeof(id.manufacturer), "%s", fields[0]);
    snprintf(id.product, sizeof(id.product), "%s", fields[1]);
    snprintf(id.serial, sizeof(id.serial), "%s", fields[2]);
    if (id.vid == 0 || id.pid == 0) {
        ui_toast(ui, "VID and PID must be non-zero hex numbers, like 046d");
        return false;
    } else if (!wye_engine_identity_set((uint8_t)node, &id)) {
        ui_toast(ui, "That Pico is not connected right now");
        return false;
    } else {
        if (node == WYE_NODE_BRIDGE) { /* keep finding the bridge under its new identity */
            char entry[16];
            snprintf(entry, sizeof(entry), "%s%04x:%04x", ui->cfg.bridge_ids[0] ? "," : "", id.vid, id.pid);
            strncat(ui->cfg.bridge_ids, entry, sizeof(ui->cfg.bridge_ids) - strlen(ui->cfg.bridge_ids) - 1);
            ui_save_config(ui);
        }
        ui_toast(ui, "Saved - the Pico restarts with its new identity now");
    }
    return true;
}
