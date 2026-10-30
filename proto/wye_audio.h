/* Duplex PCM transport between the USB audio functions on the two Picos. */
#ifndef WYE_AUDIO_H
#define WYE_AUDIO_H

#define WYE_AUDIO_RATE 16000u
#define WYE_AUDIO_SAMPLE_BYTES 2u   /* signed 16-bit little endian, mono */
#define WYE_AUDIO_FRAME_BYTES 32u   /* one USB full-speed millisecond */
#define WYE_AUDIO_USB_MAX_BYTES 34u /* asynchronous IN: allow one extra sample */
#define WYE_AUDIO_CHUNK 128u        /* at most four milliseconds per UART packet */

#endif
