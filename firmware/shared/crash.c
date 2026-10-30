/*
 * Crashes and freezes: a Pico must never hang until someone unplugs it. A
 * panic or hard fault restarts it at once, the watchdog restarts it if the
 * main loop stops, and afterwards it tells the host why (WYE_MSG_LOG).
 * The reason survives the restart in the watchdog's scratch registers.
 */
#include "board.h"
#include "node.h"

#include "hardware/structs/watchdog.h"
#include "hardware/watchdog.h"

#include <stdio.h>
#include <string.h>

#define CRASH_MAGIC 0x48435253u /* "HCRS" */
#define WATCHDOG_MS 3000u
enum { KIND_PANIC = 1, KIND_FAULT = 2 };

static char g_report[112];

static void __attribute__((noreturn)) restart(uint32_t kind, uint32_t what, uint32_t arg)
{
    watchdog_hw->scratch[0] = CRASH_MAGIC;
    watchdog_hw->scratch[1] = kind;
    watchdog_hw->scratch[2] = what;
    watchdog_hw->scratch[3] = arg;
    watchdog_reboot(0, 0, 0);
    for (;;) {
        tight_loop_contents();
    }
}

/* PICO_PANIC_FUNCTION: panic(fmt, ...) arrives here with fmt in r0 and its first argument in r1. */
void __attribute__((noreturn)) wye_panic(const char *fmt, uint32_t arg) { restart(KIND_PANIC, (uint32_t)fmt, arg); }

void __attribute__((noreturn, used)) wye_fault(const uint32_t *frame)
{
    restart(KIND_FAULT, frame[6], frame[5]); /* the stacked PC and LR: where it happened */
}

__attribute__((naked)) void isr_hardfault(void)
{
    __asm volatile("movs r0, #4\n mov r1, lr\n tst r0, r1\n beq 1f\n mrs r0, psp\n b 2f\n"
                   "1: mrs r0, msp\n2: bl wye_fault\n");
}

/* The panic text lives in flash; only trust it if it looks like text (a firmware update moves it). */
static void panic_text(uint32_t fmt, uint32_t arg)
{
    const char *s = (const char *)fmt;
    size_t n = (size_t)snprintf(g_report, sizeof(g_report), "restarted after a crash: ");
    bool flash = fmt >= XIP_BASE && fmt < XIP_BASE + WYE_FLASH_APP_MAX;
    for (size_t i = 0; flash && s[i] && n < sizeof(g_report) - 14; ++i) {
        g_report[n++] = s[i] == '%' ? '?' : (s[i] >= 32 && s[i] < 127 ? s[i] : '.');
    }
    snprintf(g_report + n, sizeof(g_report) - n, " (%08lx)", (unsigned long)arg);
}

void wye_crash_start(void)
{
    if (watchdog_hw->scratch[0] == CRASH_MAGIC) {
        uint32_t kind = watchdog_hw->scratch[1], what = watchdog_hw->scratch[2], arg = watchdog_hw->scratch[3];
        watchdog_hw->scratch[0] = 0;
        if (kind == KIND_PANIC) {
            panic_text(what, arg);
        } else {
            snprintf(g_report, sizeof(g_report), "restarted after a fault at %08lx (from %08lx)",
                     (unsigned long)what, (unsigned long)arg);
        }
    } else if (watchdog_enable_caused_reboot()) {
        snprintf(g_report, sizeof(g_report), "restarted after it stopped responding");
    }
    watchdog_enable(WATCHDOG_MS, true); /* paused while a debugger halts the chip */
}

void wye_crash_feed(void)
{
    watchdog_update();
}

/* Say it a few times once the host talks to us: the app only listens after its handshake. */
void wye_crash_tell(struct wye_node *n, bool host_seen, uint32_t now)
{
    static uint32_t first, told;
    if (!g_report[0] || !host_seen || told >= 3) {
        return;
    }
    first = first ? first : now;
    if (now - first >= told * 4000u + 2000u) {
        wye_node_send(n, WYE_MSG_LOG, 0, g_report, strlen(g_report));
        ++told;
    }
}
