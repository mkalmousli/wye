/* termios serial I/O shared by Linux and macOS. */
#define _DEFAULT_SOURCE
#include "plat_serial.h"
#include "posix_event.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

struct plat_serial { int fd; };

static int open_raw(const char *path, speed_t speed, char *err, size_t errlen)
{
    int fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    struct termios tty;
    if (fd < 0 || tcgetattr(fd, &tty) != 0) {
        snprintf(err, errlen, "%s", strerror(errno));
        if (fd >= 0) {
            close(fd);
        }
        return -1;
    }
    cfmakeraw(&tty);
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~(tcflag_t)(CSTOPB | PARENB | CRTSCTS);
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        snprintf(err, errlen, "%s", strerror(errno));
        close(fd);
        return -1;
    }
    int lines = TIOCM_DTR | TIOCM_RTS;
    ioctl(fd, TIOCMBIS, &lines); /* DTR tells the bridge a host is present */
    return fd;
}

plat_serial *plat_serial_open(const char *path, char *err, size_t errlen)
{
    int fd = open_raw(path, B115200, err, errlen); /* CDC ignores the baud rate */
    if (fd < 0) {
        return NULL;
    }
    tcflush(fd, TCIOFLUSH);
    struct plat_serial *s = calloc(1, sizeof(*s));
    s->fd = fd;
    return s;
}

int plat_serial_read(plat_serial *s, uint8_t *buf, size_t cap, int timeout_ms, plat_event *wake)
{
    struct pollfd p[2] = {{s->fd, POLLIN, 0}, {wake ? plat_event_fd(wake) : -1, POLLIN, 0}};
    int rc = poll(p, wake ? 2 : 1, timeout_ms);
    if (rc < 0) {
        return errno == EINTR ? 0 : -1;
    }
    if (wake && (p[1].revents & POLLIN)) {
        plat_event_drain(wake);
    }
    if (p[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
        return -1;
    }
    if (!(p[0].revents & POLLIN)) {
        return 0;
    }
    ssize_t n = read(s->fd, buf, cap);
    if (n == 0) {
        return -1; /* readable but EOF: device vanished */
    }
    return n < 0 ? (errno == EAGAIN || errno == EINTR ? 0 : -1) : (int)n;
}

int plat_serial_write(plat_serial *s, const uint8_t *data, size_t len)
{
    size_t done = 0;
    for (int spins = 0; done < len && spins < 50; ++spins) {
        ssize_t n = write(s->fd, data + done, len - done);
        if (n > 0) {
            done += (size_t)n;
        } else if (n < 0 && errno != EAGAIN && errno != EINTR) {
            return -1;
        } else {
            struct pollfd p = {s->fd, POLLOUT, 0};
            poll(&p, 1, 1); /* buffer full: wait briefly, never block the caller long */
        }
    }
    return (int)done;
}

void plat_serial_close(plat_serial *s)
{
    if (s) {
        close(s->fd);
        free(s);
    }
}

bool plat_serial_touch_1200(const char *path)
{
    char err[64];
    int fd = open_raw(path, B1200, err, sizeof(err));
    if (fd < 0) {
        return false;
    }
    int lines = TIOCM_DTR;
    ioctl(fd, TIOCMBIC, &lines); /* dropping DTR at 1200 baud triggers the reset */
    close(fd);
    return true;
}
