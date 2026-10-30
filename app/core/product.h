/*
 * Product identity. Rebranding = editing this file (and the icons in
 * app/resources). Version numbers come from the top-level VERSION file.
 */
#ifndef WYE_PRODUCT_H
#define WYE_PRODUCT_H

#define WYE_PRODUCT_NAME "Wye"
#define WYE_PRODUCT_ID "wye"           /* file and directory names */
#define WYE_VENDOR_NAME "Wye"
#define WYE_HOMEPAGE_URL "https://mkalmousli.github.io/wye/"
#define WYE_SOURCE_URL "https://github.com/mkalmousli/wye"
#define WYE_SUPPORT_URL "https://github.com/mkalmousli/wye/issues"

/* Signed release manifest; the app verifies it with WYE_RELEASE_PUBKEY. */
#define WYE_UPDATE_URL "https://github.com/mkalmousli/wye/releases/latest/download/latest.json"

/* Default bridge USB identities (pid.codes test PIDs until allocated). */
#define WYE_BRIDGE_VID 0x1209
#define WYE_BRIDGE_PID 0x0001
#define WYE_TARGET_PID 0x0002
#define WYE_LEGACY_PICO_VID 0x2e8a       /* Raspberry Pi: pico-sdk stdio firmware */
#define WYE_LEGACY_PICO_PID 0x000a

#ifndef WYE_VERSION_STRING
#define WYE_VERSION_STRING "0.0.0-dev"
#endif

#endif
