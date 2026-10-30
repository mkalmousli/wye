/* Overlapped COM port I/O so a blocking read can be woken by the engine. */
#include "plat_serial.h"
#include "win32_event.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct plat_serial {
    HANDLE h;
    OVERLAPPED rd;
    bool pending;
    uint8_t buf[4096];
};

static HANDLE open_port(const char *path, DWORD baud, char *err, size_t errlen)
{
    HANDLE h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    DCB dcb = {.DCBlength = sizeof(dcb)};
    if (h == INVALID_HANDLE_VALUE || !GetCommState(h, &dcb)) {
        snprintf(err, errlen, "cannot open %s (error %lu)", path, GetLastError());
        if (h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
        }
        return INVALID_HANDLE_VALUE;
    }
    dcb.BaudRate = baud;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE; /* tells the bridge a host is present */
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fOutxCtsFlow = dcb.fOutxDsrFlow = dcb.fOutX = dcb.fInX = FALSE;
    /* MAXDWORD/MAXDWORD/N: a read completes as soon as one byte arrives,
       or empty after N ms; with overlapped I/O nothing spins while idle. */
    COMMTIMEOUTS t = {MAXDWORD, MAXDWORD, 1000, 0, 200};
    SetCommState(h, &dcb);
    SetCommTimeouts(h, &t);
    return h;
}

plat_serial *plat_serial_open(const char *path, char *err, size_t errlen)
{
    HANDLE h = open_port(path, CBR_115200, err, errlen);
    if (h == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);
    struct plat_serial *s = calloc(1, sizeof(*s));
    s->h = h;
    s->rd.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    return s;
}

int plat_serial_read(plat_serial *s, uint8_t *out, size_t cap, int timeout_ms, plat_event *wake)
{
    DWORD got = 0;
    if (!s->pending) {
        ResetEvent(s->rd.hEvent);
        if (ReadFile(s->h, s->buf, sizeof(s->buf), &got, &s->rd)) {
            goto done;
        }
        if (GetLastError() != ERROR_IO_PENDING) {
            return -1;
        }
        s->pending = true;
    }
    HANDLE waits[2] = {s->rd.hEvent, wake ? plat_event_handle(wake) : s->rd.hEvent};
    DWORD r = WaitForMultipleObjects(wake ? 2 : 1, waits, FALSE, (DWORD)timeout_ms);
    if (r != WAIT_OBJECT_0) {
        return 0; /* timeout or woken: the read stays pending for next time */
    }
    s->pending = false;
    if (!GetOverlappedResult(s->h, &s->rd, &got, FALSE)) {
        return -1;
    }
done:
    got = got < cap ? got : (DWORD)cap;
    memcpy(out, s->buf, got);
    return (int)got;
}

int plat_serial_write(plat_serial *s, const uint8_t *data, size_t len)
{
    OVERLAPPED ov = {0};
    DWORD done = 0;
    ov.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    BOOL ok = WriteFile(s->h, data, (DWORD)len, &done, &ov);
    if (!ok && GetLastError() == ERROR_IO_PENDING) {
        ok = WaitForSingleObject(ov.hEvent, 200) == WAIT_OBJECT_0 && GetOverlappedResult(s->h, &ov, &done, FALSE);
        if (!ok) {
            CancelIoEx(s->h, &ov);
        }
    }
    CloseHandle(ov.hEvent);
    return ok ? (int)done : -1;
}

void plat_serial_close(plat_serial *s)
{
    if (!s) {
        return;
    }
    CancelIoEx(s->h, NULL);
    if (s->pending) {
        DWORD got;
        GetOverlappedResult(s->h, &s->rd, &got, TRUE);
    }
    CloseHandle(s->rd.hEvent);
    CloseHandle(s->h);
    free(s);
}

bool plat_serial_touch_1200(const char *path)
{
    char err[64];
    HANDLE h = open_port(path, 1200, err, sizeof(err));
    if (h == INVALID_HANDLE_VALUE) {
        return false;
    }
    EscapeCommFunction(h, CLRDTR);
    CloseHandle(h);
    return true;
}
