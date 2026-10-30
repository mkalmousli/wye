/* The simulated target's stored function settings. */
#include "sim.h"

bool sim_fn_get(void *ctx, struct wye_functions *f) { *f = ((struct sim_device *)ctx)->functions; return true; }
bool sim_fn_set(void *ctx, const struct wye_functions *f) { ((struct sim_device *)ctx)->functions = *f; return true; }
bool sim_profile_get(void *ctx, struct wye_profile *f) { *f = ((struct sim_device *)ctx)->profile; return true; }
bool sim_profile_set(void *ctx, const struct wye_profile *f) { ((struct sim_device *)ctx)->profile = *f; return true; }
