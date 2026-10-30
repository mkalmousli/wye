/* Status snapshot for the UI. */
#include "engine_internal.h"

#include "util/util.h"

#include <string.h>

static bool outdated(const struct wye_node_info *n, uint8_t role)
{
    const struct wye_fw_image *img = wye_fw_embedded(role);
    return n->present && img && wye_fw_is_older(img, n->hello.fw_major, n->hello.fw_minor, n->hello.fw_patch);
}

void wye_engine_status(struct wye_status *s)
{
    struct wye_engine *e = &g_engine;
    memset(s, 0, sizeof(*s));
    plat_mutex_lock(e->lock);
    uint64_t now = plat_now_ms();
    s->link = e->link.info;
    s->mode = e->input.mode;
    s->jiggler_on = e->jiggler.active;
    s->jiggler_progress = wye_jiggler_progress(&e->jiggler, now);
    s->recording = e->recorder.file != NULL;
    s->recorded_events = e->recorder.events;
    s->playing = e->player.active;
    s->play_paused = e->player.paused;
    s->play_loop = e->player.loop;
    s->play_progress = wye_player_progress(&e->player, now);
    s->play_speed = e->player.speed_percent ? e->player.speed_percent : 100;
    wye_strcpy(s->play_name, sizeof(s->play_name), e->player.name);
    s->typing = wye_typing_busy(&e->typing);
    s->fw_phase = e->fw.phase;
    s->fw_node = e->fw.node;
    s->fw_progress = wye_fw_session_progress(&e->fw);
    wye_strcpy(s->fw_error, sizeof(s->fw_error), e->fw.error);
    s->bridge_outdated = outdated(&e->link.info.bridge, WYE_ROLE_BRIDGE);
    s->target_outdated = outdated(&e->link.info.target, WYE_ROLE_TARGET);
    s->learning_key = e->learning_key;
    wye_audio_status(&s->audio);
    s->learned_key = e->learned_key;
    memcpy(s->identity_valid, e->identity_valid, sizeof(s->identity_valid));
    memcpy(s->identity, e->identity, sizeof(s->identity));
    wye_drive_status(&e->drive, &s->drive);
    wye_activity_take(&e->activity, &s->activity);
    memcpy(s->profile_valid, e->profile_valid, sizeof(s->profile_valid));
    memcpy(s->profile, e->profile, sizeof(s->profile));
    s->functions_valid = e->functions_valid;
    s->functions = e->functions;
    plat_mutex_unlock(e->lock);
    s->capture = plat_input_status();
}
