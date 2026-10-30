/* Sending to the Picos. May run on the UI or capture thread, beside the I/O thread's read. */
#include "link.h"

#include <string.h>

bool wye_link_send(struct wye_link *l, uint8_t node, uint8_t type, const void *payload, size_t len)
{
    uint8_t buf[WYE_MAX_ENCODED];
    if (!l->port || l->write_failed) {
        return false;
    }
    size_t n = wye_packet_encode(node, type, l->seq++, payload, len, buf, sizeof(buf));
    if (n == 0) {
        return false;
    }
    int written = l->tr->write(l->tr->ctx, l->port, buf, n);
    if (written < 0) {
        /* UI/capture callers can write while the I/O thread is reading. Only
           that thread may retire the port, after its read has returned. */
        l->write_failed = true;
        return false;
    }
    /* Short write: the Pico is busy (e.g. erasing flash during an update) - the
       packet is lost, not the link; callers retry on their own timeouts. */
    if (written < (int)n) {
        static const uint8_t end = 0; /* terminate the cut frame so the next one decodes */
        l->tr->write(l->tr->ctx, l->port, &end, 1);
        return false;
    }
    return true;
}
