/* Shared by the drive tests (drive_harness.c). */
#ifndef WYE_DRIVE_HARNESS_H
#define WYE_DRIVE_HARNESS_H

#include "drive/drive.h"
#include "sim.h"

#define DRIVE_TEST_ROOT "out/test-drive"

extern struct sim_device g_sim;
extern struct wye_drive g_drive;
extern bool g_saw_removing;

/* Moves host-bound traffic into the drive and lets simulated time pass. */
void pump(uint32_t ms);
/* Pumps (with real sleeps, jobs run on threads) until the drive reaches s or gives up. */
void wait_state(enum wye_drive_state s);

#endif
