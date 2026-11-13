#include "kernel.h"
extern uint8_t _kernel_end;
#define E820_COUNT (*(volatile uint32_t *)0x8000)
#define E820_TABLE ((struct e820_entry *)0x8004)

static void ok(const char *s) { vga_set_color(0x0A); kprintf("[ ok ] "); vga_set_color(0x07); kprintf("%s\n", s); }

void kmain(void) {
    serial_init();
    vga_init();
    vga_set_color(0x0E); kprintf("WintiOS v0.2 - 32-bit x86\n\n"); vga_set_color(0x07);
    gdt_init();  ok("GDT loaded");
    idt_init();  pic_init(); ok("IDT + PIC remapped");
    kprintf("BIOS E820 memory map (%u entries):\n", E820_COUNT);
    pmm_dump_map(E820_TABLE, E820_COUNT);
    pmm_init(E820_TABLE, E820_COUNT, (uint32_t)&_kernel_end);
    ok("physical memory manager");
    heap_init(); ok("kmalloc heap");
    sched_init(); ok("scheduler (round-robin, 50 ms slice)");
    pit_init(100); ok("PIT timer @ 100 Hz");
    keyboard_init(); ok("keyboard driver");
    sti();

    void *a = kmalloc(100), *b = kmalloc(5000);        /* boot self-test */
    kfree(a); kfree(b);
    struct heap_stats h; heap_get_stats(&h);
    if (h.used_blocks) panic("heap self-test failed", NULL);
    ok("heap self-test");
    kprintf("\nType 'help'.\n\n");
    shell_run();
}
