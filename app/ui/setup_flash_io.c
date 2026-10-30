/* Setup: copying firmware to a BOOTSEL drive, and spotting Picos that need none. */
#include "setup_flash.h"
#include "fw/fw_image.h"
#include "log.h"
#include "product.h"
#include "util/util.h"
#include <stdio.h>

bool flash_write_uf2(const struct wye_fw_image *img, const char *drive)
{
    char path[PLAT_PATH_MAX];
    FILE *f = wye_path_join(path, sizeof(path), drive, "wye.uf2") ? fopen(path, "wb") : NULL;
    bool ok = f && fwrite(img->uf2, 1, img->uf2_len, f) == img->uf2_len;
    ok = f && fclose(f) == 0 && ok; /* the Pico restarts as soon as the last block lands */
    LOG_I("setup: wrote %s firmware to %s: %s", img->name, drive, ok ? "ok" : "failed");
    return ok;
}

/* Current firmware already on a Pico this computer can see. */
bool flash_already_installed(struct wye_ui *ui, uint8_t role)
{
    const struct wye_link_info *l = &ui->st.link;
    if (role == WYE_ROLE_BRIDGE) {
        return l->bridge.present && !ui->st.bridge_outdated;
    }
    return (l->target.present && !ui->st.target_outdated) || plat_usb_count(WYE_BRIDGE_VID, WYE_TARGET_PID) > 0;
}

/* We know which Pico this is: get its firmware ready to copy. */
bool flash_prepare(struct flash_state *s, uint8_t role)
{
    s->role = role;
    s->write_pending = wye_fw_embedded(role) != NULL;
    if (!s->write_pending) {
        snprintf(s->error, sizeof(s->error), "This build has no %s firmware (run ./build.c firmware, then ./build.c).",
                 role == WYE_ROLE_TARGET ? "target" : "bridge");
    }
    return s->write_pending;
}
