#include "bridge.h"

#include <string.h>

void wye_bridge_init(struct wye_bridge *b, const struct wye_hello_ack *identity,
                     const struct wye_node_hooks *hooks, const struct wye_bridge_io *io)
{
    memset(b, 0, sizeof(*b));
    wye_node_init(&b->node, WYE_NODE_BRIDGE, identity, hooks);
    b->io = io;
    wye_decoder_init(&b->host_decoder);
    wye_decoder_init(&b->uart_decoder);
}

static void forward_raw(struct wye_bridge *b)
{
    static const uint8_t delim = 0;
    b->io->uart_write(b->io->ctx, b->host_decoder.raw, b->host_decoder.raw_len);
    b->io->uart_write(b->io->ctx, &delim, 1);
}

void wye_bridge_host_rx(struct wye_bridge *b, const uint8_t *data, size_t len)
{
    struct wye_packet pkt;
    for (size_t i = 0; i < len; ++i) {
        enum wye_decode r = wye_decoder_feed(&b->host_decoder, data[i], &pkt);
        if (r == WYE_DECODE_ERROR) {
            wye_node_count_error(&b->node);
        } else if (r != WYE_DECODE_PACKET) {
            continue;
        } else if (pkt.node == WYE_NODE_TARGET) {
            forward_raw(b);
        } else if (pkt.node == WYE_NODE_BRIDGE && !wye_node_handle(&b->node, &pkt)) {
            wye_node_error(&b->node, pkt.seq, WYE_ERR_UNKNOWN_TYPE, pkt.type);
        }
    }
}

void wye_bridge_uart_rx(struct wye_bridge *b, const uint8_t *data, size_t len)
{
    struct wye_packet pkt;
    for (size_t i = 0; i < len; ++i) {
        enum wye_decode r = wye_decoder_feed(&b->uart_decoder, data[i], &pkt);
        if (r == WYE_DECODE_ERROR) {
            wye_node_count_error(&b->node);
        } else if (r == WYE_DECODE_PACKET) {
            if (pkt.type == WYE_MSG_AUDIO) {
                if (pkt.node == WYE_NODE_TARGET && pkt.len && pkt.len <= WYE_AUDIO_CHUNK &&
                    !(pkt.len % WYE_AUDIO_SAMPLE_BYTES) && b->node.hooks->audio_rx) {
                    b->node.hooks->audio_rx(b->node.hooks->ctx, pkt.payload, pkt.len);
                }
            } else {
                /* Preserve the complete original encoded packet, including delimiter. */
                uint8_t raw[WYE_MAX_ENCODED];
                size_t n = b->uart_decoder.raw_len;
                memcpy(raw, b->uart_decoder.raw, n);
                raw[n++] = 0;
                b->node.hooks->send_host(b->node.hooks->ctx, raw, n);
            }
        }
    }
}

static void release_target(struct wye_bridge *b)
{
    uint8_t buf[16];
    size_t n = wye_packet_encode(WYE_NODE_TARGET, WYE_MSG_RELEASE_ALL, 0, NULL, 0, buf, sizeof(buf));
    /* Leading delimiter terminates any half-forwarded frame first. */
    static const uint8_t delim = 0;
    b->io->uart_write(b->io->ctx, &delim, 1);
    b->io->uart_write(b->io->ctx, buf, n);
}

void wye_bridge_task(struct wye_bridge *b)
{
    bool present = b->io->host_present(b->io->ctx);
    if (b->host_was_present && !present) {
        release_target(b);
        wye_decoder_init(&b->host_decoder);
    }
    b->host_was_present = present;
    wye_node_task(&b->node);
}
