/*
 * USB identity (VID/PID/strings), the target's function settings and your name
 * for the board, kept together in the last flash sector so all can change at
 * runtime from the desktop app. Firmware updates leave this sector alone. Each
 * record falls back to defaults when blank or corrupt; saving one rewrites the
 * sector with all of them.
 */
#include "board.h"
#include "wye_codec.h"

#include "hardware/flash.h"
#include "hardware/sync.h"

#include <stddef.h>
#include <string.h>

#define IDENTITY_MAGIC 0x57594944u  /* "WYID" */
#define FUNCTIONS_MAGIC 0x57594632u /* "WYF2": flags + drive names */
#define PROFILE_MAGIC 0x57595031u   /* "WYP1" */
#define FUNCTIONS_AT 1024u
#define PROFILE_AT 2048u

struct stored_identity { uint32_t magic; struct wye_identity id; uint32_t crc; };
struct stored_functions { uint32_t magic; struct wye_functions fn; uint32_t crc; };
struct stored_profile { uint32_t magic; struct wye_profile p; uint32_t crc; };

static struct wye_identity g_identity;
static struct wye_functions g_functions;
static struct wye_profile g_profile;

static const uint8_t *flash_at(uint32_t offset)
{
    return (const uint8_t *)(XIP_BASE + WYE_FLASH_IDENTITY_OFFSET + offset);
}

void identity_init(const struct wye_identity *defaults)
{
    const struct stored_identity *s = (const struct stored_identity *)flash_at(0);
    bool valid = s->magic == IDENTITY_MAGIC && s->id.vid != 0 &&
                 s->crc == wye_crc32((const uint8_t *)s, offsetof(struct stored_identity, crc));
    g_identity = valid ? s->id : *defaults;
}

void functions_init(const struct wye_functions *defaults)
{
    const struct stored_functions *s = (const struct stored_functions *)flash_at(FUNCTIONS_AT);
    bool valid = s->magic == FUNCTIONS_MAGIC &&
                 s->crc == wye_crc32((const uint8_t *)s, offsetof(struct stored_functions, crc));
    g_functions = valid ? s->fn : *defaults;
}

void profile_init(void)
{
    const struct stored_profile *s = (const struct stored_profile *)flash_at(PROFILE_AT);
    bool valid = s->magic == PROFILE_MAGIC && s->crc == wye_crc32((const uint8_t *)s, offsetof(struct stored_profile, crc));
    if (valid) {
        g_profile = s->p;
    }
}

const struct wye_profile *profile_current(void) { return &g_profile; }
const struct wye_identity *identity_current(void) { return &g_identity; }
const struct wye_functions *functions_current(void) { return &g_functions; }

static bool write_sector(void)
{
    static uint8_t page[FLASH_SECTOR_SIZE];
    struct stored_identity si = {IDENTITY_MAGIC, g_identity, 0};
    struct stored_functions sf = {FUNCTIONS_MAGIC, g_functions, 0};
    static struct stored_profile sp;
    memset(&sp, 0, sizeof(sp)); /* no stray padding in the checksum */
    sp.magic = PROFILE_MAGIC;
    sp.p = g_profile;
    sp.crc = wye_crc32((const uint8_t *)&sp, offsetof(struct stored_profile, crc));
    si.crc = wye_crc32((const uint8_t *)&si, offsetof(struct stored_identity, crc));
    sf.crc = wye_crc32((const uint8_t *)&sf, offsetof(struct stored_functions, crc));
    memset(page, 0xff, sizeof(page));
    memcpy(page, &si, sizeof(si));
    memcpy(page + FUNCTIONS_AT, &sf, sizeof(sf));
    memcpy(page + PROFILE_AT, &sp, sizeof(sp));
    uint32_t irq = save_and_disable_interrupts();
    flash_range_erase(WYE_FLASH_IDENTITY_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(WYE_FLASH_IDENTITY_OFFSET, page, FLASH_SECTOR_SIZE);
    restore_interrupts(irq);
    return true;
}

bool identity_save(const struct wye_identity *id)
{
    g_identity = *id; /* takes effect on the reboot that follows */
    return write_sector();
}

bool functions_save(const struct wye_functions *f)
{
    g_functions = *f;
    return write_sector();
}

bool profile_save(const struct wye_profile *f)
{
    if (memcmp(&g_profile, f, sizeof(*f)) == 0) {
        return true; /* spare the flash */
    }
    g_profile = *f;
    return write_sector();
}
