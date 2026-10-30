/* Link transport backed by the device simulator, with fault injection. */
#ifndef WYE_SIM_TRANSPORT_H
#define WYE_SIM_TRANSPORT_H

#include "link/link.h"
#include "sim.h"

struct sim_transport {
    struct sim_device dev;
    struct wye_link_transport ops;
    bool plugged;        /* bridge USB present */
    bool silent;         /* port exists but nothing answers (old firmware) */
    bool legacy_ids;     /* enumerate as a generic Pico instead of Wye */
    int opens;
};

void sim_transport_init(struct sim_transport *t);
/* Advance simulated time, pumping bytes between link and devices each ms. */
void sim_transport_run(struct sim_transport *t, struct wye_link *link, uint32_t ms);

#endif
