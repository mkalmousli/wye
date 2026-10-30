/* Linux capture thread: device I/O, hotplug, LED mirroring. */
#define _DEFAULT_SOURCE
#include "evdev.h"
#include "log.h"
#include "posix_event.h"

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

static void read_device(struct ev_capture *c, int i)
{
    struct input_event evs[64];
    ssize_t n = read(c->dev[i].fd, evs, sizeof(evs));
    if (n < 0 && errno != EAGAIN && errno != EINTR) {
        LOG_I("input: %s removed", c->dev[i].name);
        ev_remove(c, i); /* ENODEV: unplugged */
        return;
    }
    for (ssize_t k = 0; k < n / (ssize_t)sizeof(evs[0]); ++k) {
        ev_handle(c, &c->dev[i], &evs[k]);
    }
}

void ev_thread_main(void *arg)
{
    struct ev_capture *c = arg;
    struct pollfd p[EV_MAX_DEVICES + 3];
    while (!c->stop) {
        c->heartbeat = plat_now_ms();
        if (c->rescan) {
            c->rescan = false;
            ev_scan(c);
        }
        ev_apply_grabs(c);
        if (c->leds_dirty) {
            c->leds_dirty = false;
            ev_write_leds(c, c->forwarding ? c->target_leds : c->local_leds);
        }
        int n = c->count;
        for (int i = 0; i < n; ++i) {
            p[i] = (struct pollfd){c->dev[i].fd, POLLIN, 0};
        }
        p[n] = (struct pollfd){c->inotify_fd, POLLIN, 0};
        p[n + 1] = (struct pollfd){plat_event_fd(c->wake), POLLIN, 0};
        p[n + 2] = (struct pollfd){c->ui_kbd, POLLIN, 0};
        if (poll(p, (nfds_t)n + 3, 500) <= 0) {
            continue;
        }
        for (int i = n - 1; i >= 0; --i) { /* reverse: ev_remove compacts the array */
            if (p[i].revents & (POLLIN | POLLERR | POLLHUP)) {
                read_device(c, i);
            }
        }
        if (p[n].revents & POLLIN) {
            char buf[4096];
            while (read(c->inotify_fd, buf, sizeof(buf)) > 0) {
            }
            usleep(50 * 1000); /* let udev apply permissions to the new node */
            c->rescan = true;
        }
        if (p[n + 1].revents & POLLIN) {
            plat_event_drain(c->wake);
        }
        if (p[n + 2].revents & POLLIN) {
            ui_read_leds(c);
        }
    }
}
