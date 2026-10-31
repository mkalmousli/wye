/* macOS: menu bar item (NSStatusItem) with a native menu. Clicks land in plat_tray_poll. */
#include "plat_tray.h"

#import <AppKit/AppKit.h>
#include <string.h>

#define MENU_ICON_PT 18 /* the icon is drawn at 2x for Retina screens */

static volatile int g_picked = PLAT_TRAY_NONE;

@interface WyeTrayTarget : NSObject
- (void)pick:(NSMenuItem *)sender;
@end

@implementation WyeTrayTarget
- (void)pick:(NSMenuItem *)sender
{
    g_picked = (int)sender.tag;
}
@end

static NSStatusItem *g_item;
static WyeTrayTarget *g_target;

bool plat_tray_start(plat_tray_painter painter)
{
    (void)painter;
    g_target = g_target ?: [WyeTrayTarget new];
    g_item = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
    return g_item != nil;
}

void plat_tray_stop(void)
{
    if (g_item) {
        [[NSStatusBar systemStatusBar] removeStatusItem:g_item];
        g_item = nil;
    }
}

bool plat_tray_available(void)
{
    return g_item != nil;
}

int plat_tray_icon_size(void)
{
    return MENU_ICON_PT * 2;
}

void plat_tray_set_icon(const uint8_t *rgba, int size)
{
    if (!g_item) return;
    NSBitmapImageRep *rep = [[NSBitmapImageRep alloc]
        initWithBitmapDataPlanes:NULL pixelsWide:size pixelsHigh:size bitsPerSample:8
                 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace
                     bytesPerRow:size * 4 bitsPerPixel:32];
    memcpy(rep.bitmapData, rgba, (size_t)size * size * 4);
    NSImage *img = [[NSImage alloc] initWithSize:NSMakeSize(MENU_ICON_PT, MENU_ICON_PT)];
    [img addRepresentation:rep];
    g_item.button.image = img;
}

void plat_tray_set_tooltip(const char *text)
{
    if (g_item) {
        g_item.button.toolTip = [NSString stringWithUTF8String:text];
    }
}

void plat_tray_set_menu(const struct plat_tray_item *items, int count)
{
    if (!g_item) return;
    NSMenu *menu = [[NSMenu alloc] init];
    for (int i = 0; i < count; i++) {
        if (!items[i].label[0]) {
            [menu addItem:[NSMenuItem separatorItem]];
            continue;
        }
        NSMenuItem *mi = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:items[i].label]
                                                    action:@selector(pick:) keyEquivalent:@""];
        mi.tag = items[i].id;
        mi.target = g_target;
        mi.enabled = items[i].enabled;
        mi.state = items[i].checked ? NSControlStateValueOn : NSControlStateValueOff;
        [menu addItem:mi];
    }
    g_item.menu = menu;
}

int plat_tray_poll(void)
{
    int v = g_picked;
    g_picked = PLAT_TRAY_NONE;
    return v;
}
