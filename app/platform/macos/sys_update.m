/* macOS: self-update and the input permissions Wye needs. */
#include "plat_sys.h"
#include "proc.h"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Replace the running binary in place (macOS allows it while running), then relaunch. */
bool plat_install_update(const char *downloaded, char *err, size_t errlen)
{
    char exe[PLAT_PATH_MAX], staged[PLAT_PATH_MAX + 8];
    if (!plat_exe_path(exe, sizeof(exe))) {
        snprintf(err, errlen, "cannot locate the running program");
        return false;
    }
    snprintf(staged, sizeof(staged), "%s.new", exe);
    NSFileManager *fm = [NSFileManager defaultManager];
    [fm removeItemAtPath:@(staged) error:nil];
    if (![fm copyItemAtPath:@(downloaded) toPath:@(staged) error:nil]) {
        snprintf(err, errlen, "%s is not writable", exe);
        return false;
    }
    chmod(staged, 0755);
    if (rename(staged, exe) != 0) {
        snprintf(err, errlen, "replacing %s failed: %s", exe, strerror(errno));
        remove(staged);
        return false;
    }
    const char *argv[] = {exe, "--updated", NULL};
    return posix_spawn_detached(argv);
}

/* Capture needs Input Monitoring (listen) and Accessibility (post events). */
enum plat_permission plat_input_permission(void)
{
    bool listen = CGPreflightListenEventAccess();
    bool access = AXIsProcessTrusted();
    return listen && access ? PLAT_PERM_OK : PLAT_PERM_NEEDED;
}

bool plat_input_permission_request(char *err, size_t errlen)
{
    NSDictionary *opts = @{(__bridge NSString *)kAXTrustedCheckOptionPrompt: @YES};
    AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)opts);
    CGRequestListenEventAccess();
    NSURL *settings = [NSURL URLWithString:
        @"x-apple.systempreferences:com.apple.preference.security?Privacy_ListenEvent"];
    [[NSWorkspace sharedWorkspace] openURL:settings];
    if (err && errlen) {
        snprintf(err, errlen, "allow Wye under Privacy & Security, then restart Wye");
    }
    return true;
}
