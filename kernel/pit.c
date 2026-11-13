#include "kernel.h"
static volatile uint32_t ticks;
static uint32_t freq;
static void tick(struct regs *r) { (void)r; ticks++; sched_tick(); }
void pit_init(uint32_t hz) {
    freq = hz;
    uint32_t div = 1193182 / hz;
    outb(0x43, 0x36);                     /* ch0, lobyte/hibyte, mode 3 (square wave) */
    outb(0x40, (uint8_t)(div & 0xFF));
    outb(0x40, (uint8_t)(div >> 8));
    irq_install_handler(0, tick);
}
uint32_t pit_ticks(void) { return ticks; }
uint32_t pit_hz(void) { return freq; }
void pit_sleep_ms(uint32_t ms) {
    uint32_t end = ticks + (ms * freq + 999) / 1000;
    while ((int32_t)(end - ticks) > 0) __asm__ volatile("hlt");
}
