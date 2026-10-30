/*
 * Setup wizard: installing the firmware on both Picos, one at a time and in
 * any order. Each Pico that shows up in BOOTSEL mode is asked about ("which
 * one is this?") before anything is written, so one USB port can serve both.
 * BOOTSEL is only needed for a Pico's very first install: Picos that already
 * run Wye are updated over USB (the target through the bridge), and old
 * firmware is restarted into install mode by software.
 * setup_flash.c runs the steps, setup_flash_view.c / _words.c explain them
 * and setup_art*.c draw the animated scene.
 */
#ifndef WYE_SETUP_FLASH_H
#define WYE_SETUP_FLASH_H

#include "ui.h"
#include "plat_sys.h"

enum flash_phase {
    FLASH_WAITING = 0,   /* no Pico in BOOTSEL mode yet */
    FLASH_OPENING,       /* Pico found, its drive is not ready yet */
    FLASH_STUCK,         /* the drive never appeared: the user has to help */
    FLASH_ASK,           /* a Pico is ready: the user says which one it is */
    FLASH_INSTALLING,    /* firmware copied, waiting for the Pico to restart */
    FLASH_DONE,          /* that Pico is finished */
    FLASH_ALL_SET,       /* both Picos already run current firmware */
    FLASH_FAILED,
    FLASH_UPDATING,      /* updating a running Pico over the link: no button needed */
    FLASH_RESCUE,        /* old firmware found: restarting it into install mode */
};

struct flash_state {
    bool started;
    enum flash_phase phase;
    double since;                 /* GetTime() when the phase began */
    double polled;
    uint8_t role;                 /* WYE_ROLE_* chosen for the Pico at hand, 0 = not asked yet */
    bool done[3];                 /* per WYE_ROLE_*: installed now or found up to date */
    bool reinstall;               /* user chose to install again: ignore "up to date" */
    bool write_pending;           /* copy on the next frame, after it was drawn */
    bool queued[3];               /* per WYE_ROLE_*: update over the link when idle */
    char rescued[PLAT_PATH_MAX];  /* old-firmware port already sent to install mode */
    char drive[PLAT_PATH_MAX];
    char error[160];
};

/* Actions the view hands back to the step logic. */
enum flash_action {
    FLASH_ACT_NONE = 0,
    FLASH_ACT_RETRY,
    FLASH_ACT_REINSTALL,
    FLASH_ACT_PICK_BRIDGE,
    FLASH_ACT_PICK_TARGET,
};

static inline bool flash_both_done(const struct flash_state *s)
{
    return s->done[WYE_ROLE_BRIDGE] && s->done[WYE_ROLE_TARGET];
}

struct wye_fw_image;

/* setup_flash.c */
void flash_enter(struct flash_state *s, enum flash_phase p);

/* setup_flash_link.c: updates and rescues without BOOTSEL */
bool flash_link_poll(struct wye_ui *ui, struct flash_state *s);   /* true: it owns the phase */
void flash_link_reinstall(struct wye_ui *ui, struct flash_state *s);

/* setup_flash_io.c */
bool flash_write_uf2(const struct wye_fw_image *img, const char *drive);
bool flash_already_installed(struct wye_ui *ui, uint8_t role);
bool flash_prepare(struct flash_state *s, uint8_t role);

/* setup_flash_view.c */
enum flash_action flash_view(struct wye_ui *ui, const struct flash_state *s, Rectangle c);

/* setup_art.c / setup_art_parts.c: the scene (computer, cable, Pico) */
struct art_scene {
    uint8_t role;                 /* tag on the board; 0 = unknown ("?") */
    float press;                  /* 0..1: finger on BOOTSEL */
    float plug;                   /* 0..1: cable plugged into the Pico */
    Color led;                    /* the Pico's LED (BLANK = off) */
    float check;                  /* big tick over the Pico: 0 none, grows to 1 */
};
void art_scene(struct wye_ui *ui, Rectangle r, const struct art_scene *sc);
void art_pico(struct wye_ui *ui, Rectangle board, uint8_t role, Color led);
Vector2 art_computer(Rectangle r);              /* returns its USB port */
Vector2 art_bootsel_pos(Rectangle board);
Vector2 art_usb_pos(Rectangle board);
void art_finger(struct wye_ui *ui, Vector2 button, float press);
void art_check(Vector2 c, float radius, Color col);
void art_spinner(Vector2 c, float radius, Color col);

/* ui_confetti.c */
void ui_confetti_burst(Vector2 at);
void ui_confetti_draw(void);

#endif
