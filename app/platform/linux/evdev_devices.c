/* Discovering, classifying and grabbing evdev devices. */
#define _DEFAULT_SOURCE
#include "evdev.h"
#include "log.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define BITS(n) (((n) + 8 * sizeof(unsigned long) - 1) / (8 * sizeof(unsigned long)))
#define HAS(b, n) (((b)[(n) / (8 * sizeof(unsigned long))] >> ((n) % (8 * sizeof(unsigned long)))) & 1UL)

static bool already_open(const struct ev_capture *c, const char *path)
{
    for (int i = 0; i < c->count; ++i) {
        if (strcmp(c->dev[i].path, path) == 0) {
            return true;
        }
    }
    return false;
}

static bool classify(struct ev_device *d)
{
    unsigned long ev[BITS(EV_MAX + 1)] = {0}, key[BITS(KEY_MAX + 1)] = {0};
    unsigned long rel[BITS(REL_MAX + 1)] = {0}, abs_[BITS(ABS_MAX + 1)] = {0}, prop[BITS(INPUT_PROP_MAX + 1)] = {0};
    ioctl(d->fd, EVIOCGBIT(0, sizeof(ev)), ev);
    ioctl(d->fd, EVIOCGBIT(EV_KEY, sizeof(key)), key);
    ioctl(d->fd, EVIOCGBIT(EV_REL, sizeof(rel)), rel);
    ioctl(d->fd, EVIOCGBIT(EV_ABS, sizeof(abs_)), abs_);
    ioctl(d->fd, EVIOCGPROP(sizeof(prop)), prop);
    d->keyboard = HAS(ev, EV_KEY) && HAS(key, KEY_A) && HAS(key, KEY_Z) && HAS(key, KEY_ENTER);
    d->pointer = HAS(ev, EV_REL) && HAS(rel, REL_X) && HAS(key, BTN_LEFT);
    d->is_touchpad = HAS(ev, EV_ABS) && HAS(abs_, ABS_X) && HAS(key, BTN_TOOL_FINGER) &&
                     !HAS(prop, INPUT_PROP_DIRECT);
    d->pointer = d->pointer || d->is_touchpad;
    d->has_leds = HAS(ev, EV_LED);
    bool media = HAS(ev, EV_KEY) && (HAS(key, KEY_VOLUMEUP) || HAS(key, KEY_PLAYPAUSE));
    d->keyboard = d->keyboard || (media && !d->pointer); /* media key pads */
    return d->keyboard || d->pointer;
}

static void try_add(struct ev_capture *c, const char *path)
{
    struct ev_device *d = &c->dev[c->count];
    memset(d, 0, sizeof(*d));
    d->fd = open(path, O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (d->fd < 0) {
        d->fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    }
    if (d->fd < 0) {
        return;
    }
    ioctl(d->fd, EVIOCGNAME(sizeof(d->name)), d->name);
    /* Skip our own virtual devices and those of the pre-3.0 app. */
    bool ours = strncmp(d->name, "Wye virtual", 11) == 0;
    if (ours || !classify(d)) {
        close(d->fd);
        return;
    }
    snprintf(d->path, sizeof(d->path), "%.60s", path);
    if (d->is_touchpad) {
        tp_init(&d->tp, d->fd);
    }
    LOG_I("input: using %s (%s%s%s)", d->name, d->keyboard ? "keyboard " : "",
          d->pointer ? "pointer " : "", d->is_touchpad ? "touchpad" : "");
    c->count++;
}

void ev_scan(struct ev_capture *c)
{
    DIR *dir = opendir("/dev/input");
    for (struct dirent *e = dir ? readdir(dir) : NULL; e && c->count < EV_MAX_DEVICES; e = readdir(dir)) {
        char path[300];
        snprintf(path, sizeof(path), "/dev/input/%s", e->d_name);
        if (strncmp(e->d_name, "event", 5) == 0 && !already_open(c, path)) {
            try_add(c, path);
        }
    }
    if (dir) {
        closedir(dir);
    }
}

void ev_remove(struct ev_capture *c, int i)
{
    close(c->dev[i].fd);
    c->dev[i] = c->dev[--c->count];
}
