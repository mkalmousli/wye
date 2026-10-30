/* Pico-to-Pico UART, interrupt driven both ways: sending only queues, so tud_task never stalls. */
#include "board.h"
#include "wye_proto.h"

#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"
#define LINK_UART uart0
#define LINK_IRQ UART0_IRQ
#define LINK_TX_PIN 0
#define LINK_RX_PIN 1
#define RING_SIZE 2048u

struct ring { volatile uint8_t data[RING_SIZE]; volatile uint32_t head, tail; };
static struct ring g_rx, g_tx;

static uint32_t used(const struct ring *r) { return (r->head + RING_SIZE - r->tail) % RING_SIZE; }
static void on_uart(void)
{
    while (uart_is_readable(LINK_UART)) {
        uint8_t byte = (uint8_t)uart_getc(LINK_UART);
        uint32_t next = (g_rx.head + 1u) % RING_SIZE;
        if (next != g_rx.tail) { /* on overflow drop; COBS framing resyncs */
            g_rx.data[g_rx.head] = byte;
            g_rx.head = next;
        }
    }
    while (g_tx.tail != g_tx.head && uart_is_writable(LINK_UART)) {
        uart_get_hw(LINK_UART)->dr = g_tx.data[g_tx.tail];
        g_tx.tail = (g_tx.tail + 1u) % RING_SIZE;
    }
    if (g_tx.tail == g_tx.head) { /* nothing left: stop asking for TX interrupts */
        hw_clear_bits(&uart_get_hw(LINK_UART)->imsc, UART_UARTIMSC_TXIM_BITS);
    }
}

void uart_link_init(void)
{
    uart_init(LINK_UART, WYE_UART_BAUD);
    gpio_set_function(LINK_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(LINK_RX_PIN, GPIO_FUNC_UART);
    gpio_pull_up(LINK_RX_PIN); /* idle-high when the cable is unplugged */
    uart_set_format(LINK_UART, 8, 1, UART_PARITY_NONE);
    uart_set_hw_flow(LINK_UART, false, false);
    uart_set_fifo_enabled(LINK_UART, true);
    irq_set_exclusive_handler(LINK_IRQ, on_uart);
    irq_set_enabled(LINK_IRQ, true);
    uart_set_irqs_enabled(LINK_UART, true, false); /* also sets the TX level used below */
    hw_write_masked(&uart_get_hw(LINK_UART)->ifls, 0 << UART_UARTIFLS_TXIFLSEL_LSB, UART_UARTIFLS_TXIFLSEL_BITS);
}

size_t uart_link_read(uint8_t *out, size_t cap)
{
    size_t n = 0;
    while (n < cap && g_rx.tail != g_rx.head) {
        out[n++] = g_rx.data[g_rx.tail];
        g_rx.tail = (g_rx.tail + 1u) % RING_SIZE;
    }
    return n;
}

/* Queue what fits; the interrupt (kicked here by priming the FIFO) sends it. */
static size_t queue(const uint8_t *data, size_t len)
{
    uint32_t irq = save_and_disable_interrupts();
    size_t n = 0;
    while (n < len && used(&g_tx) < RING_SIZE - 1u) {
        g_tx.data[g_tx.head] = data[n++];
        g_tx.head = (g_tx.head + 1u) % RING_SIZE;
    }
    on_uart();
    if (g_tx.tail != g_tx.head) {
        hw_set_bits(&uart_get_hw(LINK_UART)->imsc, UART_UARTIMSC_TXIM_BITS);
    }
    restore_interrupts(irq);
    return n;
}

void uart_link_write(const uint8_t *data, size_t len)
{
    for (size_t n = 0; n < len;) { /* control packets are never dropped: wait for room */
        n += queue(data + n, len - n);
    }
}

bool uart_link_try_write(const uint8_t *data, size_t len)
{
    /* Only this loop adds to the ring, so room never shrinks before queue().
       Audio may use half of it: control packets keep room, and it is better lost than late. */
    return used(&g_tx) + len <= RING_SIZE / 2u && queue(data, len) == len;
}

void uart_link_flush(void)
{
    while (g_tx.tail != g_tx.head) {
        tight_loop_contents();
    }
    uart_tx_wait_blocking(LINK_UART);
}
