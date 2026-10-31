/* macOS: Pico bootloader drives (mounted under /Volumes) and the bootloader scan. */
#import <Foundation/Foundation.h>
#include "plat_sys.h"
#include <string.h>

#define PICO_VID 0x2e8a
#define RP2040_BOOT_PID 0x0003
#define RP2350_BOOT_PID 0x000f

/* A mounted volume with INFO_UF2.TXT on it is a bootloader drive. */
static bool is_uf2_volume(NSString *dir)
{
    NSString *info = [dir stringByAppendingPathComponent:@"INFO_UF2.TXT"];
    return [[NSFileManager defaultManager] fileExistsAtPath:info];
}

/* Fills paths with the UF2 drives; with pico_only, only the Pico labels (RPI-RP2, RP2350). */
static int find_drives(char paths[][PLAT_PATH_MAX], int max, bool pico_only)
{
    NSFileManager *fm = [NSFileManager defaultManager];
    NSArray *names = [fm contentsOfDirectoryAtPath:@"/Volumes" error:nil];
    int n = 0;
    for (NSString *name in names) {
        if (n >= max) {
            break;
        }
        bool pico = [name hasPrefix:@"RPI-RP2"] || [name isEqualToString:@"RP2350"];
        if (pico_only && !pico) {
            continue;
        }
        NSString *dir = [@"/Volumes" stringByAppendingPathComponent:name];
        if (is_uf2_volume(dir) && snprintf(paths[n], PLAT_PATH_MAX, "%s", dir.fileSystemRepresentation) < PLAT_PATH_MAX) {
            n++;
        }
    }
    return n;
}

int plat_uf2_drives(char paths[][PLAT_PATH_MAX], int max)
{
    return find_drives(paths, max, false);
}

void plat_bootsel_scan(struct plat_bootsel *out)
{
    memset(out, 0, sizeof(*out));
    out->devices = plat_usb_count(PICO_VID, RP2040_BOOT_PID) + plat_usb_count(PICO_VID, RP2350_BOOT_PID);
    out->drives = find_drives(out->path, PLAT_BOOTSEL_MAX, true);
    /* macOS mounts the drive itself, so "stuck" means a Pico is there but no drive showed up. */
    out->stuck = out->devices > 0 && out->drives == 0;
}
