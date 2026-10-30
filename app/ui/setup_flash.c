/* Setup: installing both Picos' firmware through their BOOTSEL (UF2) drives. */
#include "setup_flash.h"
#include "ui_theme.h"
#include "fw/fw_image.h"
#include "util/util.h"
#include <stdio.h>
#include <string.h>

#define POLL_S 0.4
#define RESTART_TIMEOUT_S 20.0

static struct flash_state g;

void flash_enter(struct flash_state *s, enum flash_phase p)
{
    if (s->phase != p) {
        s->phase = p;
        s->since = GetTime();
    }
}

static void installing(struct flash_state *s, const struct plat_bootsel *b)
{
    /* Done once the Pico left bootloader mode (it restarts into the new firmware). */
    if (!s->write_pending && b->devices == 0) {
        s->done[s->role] = true;
        flash_enter(s, FLASH_DONE);
        ui_confetti_burst((Vector2){(float)GetScreenWidth() / 2, (float)GetScreenHeight() / 3});
    } else if (GetTime() - s->since > RESTART_TIMEOUT_S) {
        snprintf(s->error, sizeof(s->error), "The Pico did not restart. Unplug it and try again.");
        flash_enter(s, FLASH_FAILED);
    }
}

/* Automatic: the first Pico is the bridge, which stays plugged in; any Pico after that is the target. */
static uint8_t next_role(const struct wye_ui *ui, const struct flash_state *s)
{
    if (s->forced_role) {
        return s->forced_role;
    }
    if (s->next_role) {
        return (uint8_t)s->next_role;
    }
    return s->done[WYE_ROLE_BRIDGE] || ui->st.link.bridge.present ? WYE_ROLE_TARGET : WYE_ROLE_BRIDGE;
}

static void poll(struct wye_ui *ui, struct flash_state *s)
{
    struct plat_bootsel b;
    plat_bootsel_scan(&b);
    for (uint8_t r = WYE_ROLE_BRIDGE; r <= WYE_ROLE_TARGET && !s->reinstall; ++r) {
        s->done[r] = s->done[r] || flash_already_installed(ui, r);
    }
    if (s->phase == FLASH_INSTALLING) {
        installing(s, &b);
    } else if (b.devices > 1) {
        flash_enter(s, FLASH_ONE_AT_A_TIME);
    } else if (b.drives == 0 && flash_link_poll(ui, s)) {
        /* a running Pico is being updated or rescued over USB */
    } else if (s->phase != FLASH_FAILED) {
        if (b.drives > 0) {
            wye_strcpy(s->drive, sizeof(s->drive), b.path[0]);
            uint8_t role = next_role(ui, s);
            s->forced_role = 0;
            s->next_role = 0;
            flash_enter(s, flash_prepare(s, role) ? FLASH_INSTALLING : FLASH_FAILED);
        } else if (b.devices > 0) {
            flash_enter(s, b.stuck ? FLASH_STUCK : FLASH_OPENING);
        } else if (s->phase != FLASH_DONE) {
            flash_enter(s, flash_both_done(s) ? FLASH_ALL_SET : FLASH_WAITING);
        }
    }
}

static void act(struct wye_ui *ui, struct flash_state *s, enum flash_action a)
{
    if (a == FLASH_ACT_NEXT_AUTO || a == FLASH_ACT_NEXT_BRIDGE || a == FLASH_ACT_NEXT_TARGET) {
        s->next_role = a == FLASH_ACT_NEXT_BRIDGE ? WYE_ROLE_BRIDGE : a == FLASH_ACT_NEXT_TARGET ? WYE_ROLE_TARGET : 0;
    } else if (a == FLASH_ACT_REINSTALL) {
        flash_link_reinstall(ui, s);
    } else if (a == FLASH_ACT_RETRY) {
        s->error[0] = '\0';
        s->role = 0;
        flash_enter(s, FLASH_WAITING);
    }
}

bool setup_flash_step(struct wye_ui *ui, Rectangle c)
{
    if (!g.started) {
        g.started = true;
        g.since = GetTime();
    }
    if (GetTime() - g.polled > POLL_S) {
        g.polled = GetTime();
        poll(ui, &g);
    }
    act(ui, &g, flash_view(ui, &g, c));
    if (g.write_pending && GetTime() - g.since > 0.3) { /* the "Installing" scene is on screen */
        g.write_pending = false;
        if (!flash_write_uf2(wye_fw_embedded(g.role), g.drive)) {
            snprintf(g.error, sizeof(g.error), "Could not copy the firmware to %.100s.", g.drive);
            flash_enter(&g, FLASH_FAILED);
        }
    }
    return flash_both_done(&g);
}
