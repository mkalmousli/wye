/* USB audio control: stopping streams safely, and one fixed, always valid sample rate. */
#include "usb_audio.h"
#include "usb_audio_desc.h"
#include "board.h"

#include "hardware/structs/usb.h"
#include "hardware/sync.h"

/* TinyUSB 0.17 on the RP2040 leaves an ISO buffer armed when its stream stops, and
   panics ("ep was already available") when the host starts the stream again - which
   Linux does as soon as the device appears. Disarm it here, as the stream stops. */
static void disarm(uint8_t ep)
{
    uint32_t irq = save_and_disable_interrupts();
    if (ep & 0x80) {
        usb_dpram->ep_buf_ctrl[ep & 0x0f].in = 0;
    } else {
        usb_dpram->ep_buf_ctrl[ep & 0x0f].out = 0;
    }
    restore_interrupts(irq);
}

bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const *request)
{
    (void)rhport;
    uint8_t itf = (uint8_t)tu_le16toh(request->wIndex);
    if (itf == wye_usb_audio_control_itf + 1) {
        disarm(wye_usb_audio_ep_out);
    } else if (itf == wye_usb_audio_control_itf + 2) {
        disarm(wye_usb_audio_ep_in);
        wye_usb_audio_reset();
    }
    return true;
}


bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *request)
{
    if ((tu_le16toh(request->wIndex) >> 8) != WYE_AUDIO_CLOCK_ID ||
        (uint8_t)tu_le16toh(request->wIndex) != wye_usb_audio_control_itf) {
        return false;
    }
    uint8_t control = (uint8_t)(tu_le16toh(request->wValue) >> 8);
    uint8_t buf[14] = {0};
    if (control == AUDIO_CS_CTRL_SAM_FREQ && request->bRequest == AUDIO_CS_REQ_CUR) {
        wye_put_u32(buf, WYE_AUDIO_RATE);
        return tud_audio_buffer_and_schedule_control_xfer(rhport, request, buf, 4);
    }
    if (control == AUDIO_CS_CTRL_SAM_FREQ && request->bRequest == AUDIO_CS_REQ_RANGE) {
        wye_put_u16(buf, 1);
        wye_put_u32(buf + 2, WYE_AUDIO_RATE);
        wye_put_u32(buf + 6, WYE_AUDIO_RATE);
        return tud_audio_buffer_and_schedule_control_xfer(rhport, request, buf, sizeof(buf));
    }
    if (control == AUDIO_CS_CTRL_CLK_VALID && request->bRequest == AUDIO_CS_REQ_CUR) {
        buf[0] = 1;
        return tud_audio_buffer_and_schedule_control_xfer(rhport, request, buf, 1);
    }
    return false;
}
