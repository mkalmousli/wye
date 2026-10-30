#ifndef WYE_USB_AUDIO_H
#define WYE_USB_AUDIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern uint8_t wye_usb_audio_control_itf;                 /* the audio control interface number */
extern uint8_t wye_usb_audio_ep_out, wye_usb_audio_ep_in;  /* its ISO endpoints */
/* What this Pico's USB speaker plays goes to the other Pico over the UART. */
void wye_usb_audio_init(uint8_t control_itf, uint8_t ep_out, uint8_t ep_in, bool enabled);
void wye_usb_audio_rx(void *ctx, const uint8_t *data, size_t len);
void wye_usb_audio_reset(void);
void wye_usb_audio_suspend(void);
void wye_usb_audio_task(void);

#endif
