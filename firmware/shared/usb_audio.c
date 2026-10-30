/* USB playback -> framed UART -> peer USB microphone, on both Picos. */
#include "usb_audio.h"
#include "usb_audio_desc.h"
#include "audio_queue.h"
#include "board.h"
#include "wye_codec.h"
#include "wye_proto.h"

static struct wye_audio_queue g_rx;
uint8_t wye_usb_audio_ep_out, wye_usb_audio_ep_in;
uint8_t wye_usb_audio_control_itf;
static bool g_enabled, g_recording;

void wye_usb_audio_reset(void)
{
    g_recording = false;
    wye_audio_queue_reset(&g_rx);
}

void wye_usb_audio_suspend(void)
{
    /* USB alternate settings survive suspend; keep the recording state. */
    wye_audio_queue_reset(&g_rx);
    if (g_enabled && tud_audio_mounted()) {
        tud_audio_clear_ep_in_ff();
        tud_audio_clear_ep_out_ff();
    }
}

void wye_usb_audio_init(uint8_t control_itf, uint8_t ep_out, uint8_t ep_in, bool enabled)
{
    wye_usb_audio_ep_out = ep_out;
    wye_usb_audio_ep_in = ep_in;
    wye_usb_audio_control_itf = control_itf;
    g_enabled = enabled;
    wye_usb_audio_reset();
}

void wye_usb_audio_rx(void *ctx, const uint8_t *data, size_t len)
{
    (void)ctx;
    if (g_enabled && g_recording && tud_mounted() && !tud_suspended()) {
        wye_audio_queue_push(&g_rx, data, len, board_now_ms());
    }
}

/* Audio is better lost than late: never wait for room on the UART. */
static void send(const uint8_t *data, size_t len)
{
    uint8_t buf[WYE_MAX_ENCODED];
    size_t n = wye_packet_encode(WYE_NODE_TARGET, WYE_MSG_AUDIO, 0, data, len, buf, sizeof(buf));
    uart_link_try_write(buf, n);
}

void wye_usb_audio_task(void)
{
    if (!g_enabled || !tud_audio_mounted() || tud_suspended()) {
        return;
    }
    uint8_t data[WYE_AUDIO_CHUNK];
    uint16_t available = tud_audio_available();
    if (available >= WYE_AUDIO_CHUNK / 2u) { /* 2 ms per packet: half the UART overhead of 1 ms */
        uint16_t n = available > sizeof(data) ? sizeof(data) : available;
        n -= n % WYE_AUDIO_SAMPLE_BYTES;
        n = tud_audio_read(data, n);
        if (n) {
            send(data, n);
        }
    }
}

bool tud_audio_tx_done_pre_load_cb(uint8_t rhport, uint8_t func_id, uint8_t ep_in, uint8_t alt)
{
    (void)rhport; (void)func_id; (void)ep_in; (void)alt;
    uint8_t data[WYE_AUDIO_USB_MAX_BYTES];
    size_t len = wye_audio_queue_frame(&g_rx, data, board_now_ms());
    tud_audio_write(data, (uint16_t)len);
    return true;
}

bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const *request)
{
    (void)rhport;
    uint8_t itf = (uint8_t)tu_le16toh(request->wIndex);
    uint16_t alt = tu_le16toh(request->wValue);
    if (alt > 1) {
        return false;
    }
    if (itf == wye_usb_audio_control_itf + 2) {
        wye_audio_queue_reset(&g_rx);
        g_recording = alt == 1;
    }
    return true;
}
