/* Public control of the update service. */
#include "update_internal.h"

void wye_updater_start(const struct wye_config *cfg)
{
    g_up.lock = plat_mutex_new();
    g_up.wake = plat_event_new();
    g_up.cfg = *cfg;
    g_up.thread = plat_thread_start(wye_updater_service, NULL);
}

void wye_updater_stop(void)
{
    g_up.stop = true;
    plat_event_signal(g_up.wake);
    plat_thread_join(g_up.thread);
    g_up.thread = NULL;
}

void wye_updater_configure(const struct wye_config *cfg)
{
    plat_mutex_lock(g_up.lock);
    g_up.cfg = *cfg;
    plat_mutex_unlock(g_up.lock);
}

void wye_updater_check_now(void) { g_up.check_req = true; plat_event_signal(g_up.wake); }
void wye_updater_install_now(void) { g_up.install_req = true; plat_event_signal(g_up.wake); }
void wye_updater_set_busy(bool busy) { g_up.busy = busy; }

void wye_updater_status(struct wye_update_status *out)
{
    plat_mutex_lock(g_up.lock);
    *out = g_up.st;
    plat_mutex_unlock(g_up.lock);
}
