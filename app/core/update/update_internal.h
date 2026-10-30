/* State shared by the update service and its API. */
#ifndef WYE_UPDATE_INTERNAL_H
#define WYE_UPDATE_INTERNAL_H

#include "plat_thread.h"
#include "update.h"

struct wye_updater {
    plat_mutex *lock;
    plat_event *wake;
    plat_thread *thread;
    volatile bool stop, check_req, install_req, busy;
    struct wye_config cfg;
    struct wye_update_status st;
};

extern struct wye_updater g_up;

void wye_updater_service(void *arg);

#endif
