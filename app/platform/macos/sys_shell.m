/* macOS: opening things, the login item, and two integrations that have no public API. */
#include "plat_sys.h"

#import <AppKit/AppKit.h>

/* The bridge's sound is a pipe on Linux; on macOS it stays as the system set it. */
void plat_audio_full_volume(const char *id, bool capture)
{
    (void)id;
    (void)capture;
}

/* Finder's sidebar has no public API for this; the shared folder is still reachable from Finder. */
void plat_places_pin(const char *path, const char *label)
{
    (void)path;
    (void)label;
}

bool plat_open_url(const char *url)
{
    NSURL *u = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
    return u && [[NSWorkspace sharedWorkspace] openURL:u];
}

bool plat_reveal_path(const char *path)
{
    NSURL *u = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path]];
    [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:@[u]];
    return true;
}

/* ~/Library/LaunchAgents/com.mkalmousli.wye.plist starts Wye at login. */
static NSString *login_item_path(void)
{
    NSString *dir = [NSHomeDirectory() stringByAppendingPathComponent:@"Library/LaunchAgents"];
    return [dir stringByAppendingPathComponent:@"com.mkalmousli.wye.plist"];
}

bool plat_autostart_get(void)
{
    return [[NSFileManager defaultManager] fileExistsAtPath:login_item_path()];
}

bool plat_autostart_set(bool enable)
{
    NSFileManager *fm = [NSFileManager defaultManager];
    NSString *path = login_item_path();
    if (!enable) {
        return [fm removeItemAtPath:path error:nil] || ![fm fileExistsAtPath:path];
    }
    char exe[PLAT_PATH_MAX];
    if (!plat_exe_path(exe, sizeof(exe))) {
        return false;
    }
    NSDictionary *plist = @{
        @"Label": @"com.mkalmousli.wye",
        @"ProgramArguments": @[[NSString stringWithUTF8String:exe]],
        @"RunAtLoad": @YES,
        @"ProcessType": @"Interactive",
    };
    [fm createDirectoryAtPath:[path stringByDeletingLastPathComponent] withIntermediateDirectories:YES
                   attributes:nil error:nil];
    return [plist writeToFile:path atomically:YES];
}
