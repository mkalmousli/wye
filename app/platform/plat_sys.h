/* Filesystem locations, processes, network and desktop integration. */
#ifndef WYE_PLAT_SYS_H
#define WYE_PLAT_SYS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define PLAT_PATH_MAX 1024

/* Per-user directories, created on demand. */
bool plat_config_dir(char *out, size_t len);
bool plat_data_dir(char *out, size_t len);
bool plat_temp_dir(char *out, size_t len);
bool plat_exe_path(char *out, size_t len);
bool plat_mkdirs(const char *path);
bool plat_rename_replace(const char *from, const char *to);
bool plat_path_exists(const char *path);
const char *plat_path_sep(void);

/* Cryptographically secure random bytes. */
bool plat_random(void *buf, size_t len);

/* "windows-x64", "macos-universal", "linux-x64", ... (update asset key). */
const char *plat_os_id(void);
void plat_os_description(char *out, size_t len);

/* HTTPS GET into a file; follows redirects; aborts above max_bytes. */
bool plat_http_download(const char *url, const char *dest, size_t max_bytes, char *err, size_t errlen);

/* The bridge's sound devices are a pipe, not a speaker: full volume, unmuted.
   id: the sound server's device name (PulseAudio/PipeWire on Linux). */
void plat_audio_full_volume(const char *id, bool capture);

/* Show a folder in the file manager's sidebar, like a drive (once; harmless to repeat). */
void plat_places_pin(const char *path, const char *label);

bool plat_open_url(const char *url);
bool plat_reveal_path(const char *path);

bool plat_autostart_get(void);
bool plat_autostart_set(bool enable);

/*
 * Single instance. Returns false if another instance runs; with notify, the
 * running one is asked to show its window (see plat_instance_show_requested).
 */
bool plat_instance_acquire(bool notify);
/* Started from a terminal: carry on in the background and give the terminal back. */
void plat_detach_from_terminal(void);
bool plat_instance_show_requested(void);
/* The system asked this process to end (Ctrl+C, kill, the terminal closing). */
bool plat_instance_quit_requested(void);

/* Mounted UF2 bootloader drives (RPI-RP2 / RP2350); returns count. */
int plat_uf2_drives(char paths[][PLAT_PATH_MAX], int max);

/* USB devices attached to this computer (to copy an identity from). */
#define PLAT_USB_STR 64
struct plat_usb_device {
    uint16_t vid, pid;
    char manufacturer[PLAT_USB_STR], product[PLAT_USB_STR], serial[PLAT_USB_STR];
};
int plat_usb_list(struct plat_usb_device *out, int max);

/* Lets a big file stay sparse (Windows needs a flag; elsewhere it already is). */
bool plat_file_sparse(FILE *f);

/* Attached USB devices with this VID:PID (0 if none or not detectable). */
int plat_usb_count(uint16_t vid, uint16_t pid);

/* Picos in UF2 bootloader (BOOTSEL) mode and their drives. */
#define PLAT_BOOTSEL_MAX 4
struct plat_bootsel {
    int devices;                            /* seen on USB, drive open or not */
    int drives;                             /* usable drives in path[] */
    char path[PLAT_BOOTSEL_MAX][PLAT_PATH_MAX];
    bool stuck;                             /* a Pico is there but its drive never opened */
};
/* Scans for bootloader Picos. On Linux it also mounts drives the desktop
   left unmounted (no automount, e.g. XFCE), through udisks without root. */
void plat_bootsel_scan(struct plat_bootsel *out);

/* Replace the running executable/bundle with `downloaded` and relaunch. */
bool plat_install_update(const char *downloaded, char *err, size_t errlen);
/* Removes leftovers of the version replaced by the last update. */
void plat_cleanup_previous_version(void);

/* OS-specific permission state for input capture. */
enum plat_permission { PLAT_PERM_OK = 0, PLAT_PERM_NEEDED, PLAT_PERM_UNSUPPORTED };
enum plat_permission plat_input_permission(void);
/* Starts the OS flow that grants input permission (prompt, pkexec, ...). */
bool plat_input_permission_request(char *err, size_t errlen);

#endif
