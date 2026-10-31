/* macOS: native window extras for raylib's GLFW window (spaces, overlay, cursor position). */
#include "plat_window.h"

#import <AppKit/AppKit.h>
#include <ApplicationServices/ApplicationServices.h>

/* From GLFW (linked into raylib): the NSWindow behind a GLFWwindow. */
extern void *glfwGetCocoaWindow(void *glfw_window);

static NSWindow *window_of(void *handle)
{
    return (__bridge NSWindow *)glfwGetCocoaWindow(handle);
}

void plat_window_all_desktops(void *handle, bool on)
{
    NSWindow *w = window_of(handle);
    if (!w) {
        return;
    }
    NSWindowCollectionBehavior b = NSWindowCollectionBehaviorCanJoinAllSpaces |
                                   NSWindowCollectionBehaviorStationary;
    w.collectionBehavior = on ? (w.collectionBehavior | b) : (w.collectionBehavior & ~b);
}

/* Overlay: no Dock or Cmd-Tab entry (app-wide accessory policy) and a floating level.
   Takes effect when the window is shown, so call it while the window is hidden. */
void plat_window_set_overlay(void *handle, bool on)
{
    NSWindow *w = window_of(handle);
    if (!w) {
        return;
    }
    [NSApp setActivationPolicy:on ? NSApplicationActivationPolicyAccessory
                                  : NSApplicationActivationPolicyRegular];
    w.level = on ? NSFloatingWindowLevel : NSNormalWindowLevel;
    w.hidesOnDeactivate = NO;
}

void plat_window_to_current_desktop(void *handle)
{
    NSWindow *w = window_of(handle);
    if (!w) {
        return;
    }
    NSWindowCollectionBehavior saved = w.collectionBehavior;
    w.collectionBehavior = saved | NSWindowCollectionBehaviorMoveToActiveSpace;
    [w orderFrontRegardless];
    w.collectionBehavior = saved;
}

/* CGEvent locations are in the global top-left-origin space, which suits the callers. */
bool plat_cursor_screen_pos(void *handle, int *x, int *y)
{
    (void)handle;
    CGEventRef e = CGEventCreate(NULL);
    if (!e) {
        return false;
    }
    CGPoint p = CGEventGetLocation(e);
    CFRelease(e);
    *x = (int)p.x;
    *y = (int)p.y;
    return true;
}
