#include "kernel.h"
struct idt_entry { uint16_t off_lo, sel; uint8_t zero, flags; uint16_t off_hi; } __attribute__((packed));
struct idt_ptr { uint16_t limit; uint32_t base; } __attribute__((packed));
static struct idt_entry idt[256];
static struct idt_ptr ip;
extern uint32_t isr_stub_table[48];
static irq_handler_t handlers[16];

static const char *exc_names[32] = {
    "Divide error", "Debug", "NMI", "Breakpoint", "Overflow", "Bound range", "Invalid opcode",
    "Device not available", "Double fault", "Coprocessor overrun", "Invalid TSS", "Segment not present",
    "Stack fault", "General protection fault", "Page fault", "Reserved", "x87 FP exception",
    "Alignment check", "Machine check", "SIMD FP exception", "Virtualization", "Control protection",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved" };

void idt_init(void) {
    for (int i = 0; i < 48; i++) {
        uint32_t a = isr_stub_table[i];
        idt[i] = (struct idt_entry){ (uint16_t)(a & 0xFFFF), 0x08, 0, 0x8E, (uint16_t)(a >> 16) };
    }
    ip.limit = sizeof(idt) - 1; ip.base = (uint32_t)idt;
    __asm__ volatile("lidt %0" :: "m"(ip));
}

/* ---- 8259 PIC ---- */
#define PIC1 0x20
#define PIC2 0xA0
static uint16_t irq_mask = 0xFFFF;
static void io_wait(void) { outb(0x80, 0); }
static void apply_mask(void) { outb(PIC1 + 1, irq_mask & 0xFF); outb(PIC2 + 1, irq_mask >> 8); }
void pic_init(void) {
    outb(PIC1, 0x11); io_wait(); outb(PIC2, 0x11); io_wait();
    outb(PIC1 + 1, 0x20); io_wait();      /* IRQ0-7  -> vectors 32-39 */
    outb(PIC2 + 1, 0x28); io_wait();      /* IRQ8-15 -> vectors 40-47 */
    outb(PIC1 + 1, 4); io_wait(); outb(PIC2 + 1, 2); io_wait();
    outb(PIC1 + 1, 1); io_wait(); outb(PIC2 + 1, 1); io_wait();
    irq_mask = 0xFFFB;                    /* all masked except cascade (IRQ2) */
    apply_mask();
}
void pic_unmask(uint8_t irq) { irq_mask &= (uint16_t)~(1u << irq); apply_mask(); }
static void pic_eoi(uint8_t irq) { if (irq >= 8) outb(PIC2, 0x20); outb(PIC1, 0x20); }
void irq_install_handler(uint8_t irq, irq_handler_t h) { handlers[irq] = h; pic_unmask(irq); }

void panic(const char *msg, struct regs *r) {
    cli();
    vga_set_color(0x4F);
    kprintf("\n*** KERNEL PANIC: %s ***\n", msg);
    if (r) {
        kprintf("int=%u err=%08x eip=%08x cs=%04x eflags=%08x\n", r->int_no, r->err_code, r->eip, r->cs, r->eflags);
        kprintf("eax=%08x ebx=%08x ecx=%08x edx=%08x\n", r->eax, r->ebx, r->ecx, r->edx);
        kprintf("esi=%08x edi=%08x ebp=%08x\n", r->esi, r->edi, r->ebp);
    }
    kprintf("System halted.\n");
    for (;;) __asm__ volatile("hlt");
}

void isr_handler(struct regs *r) {
    if (r->int_no < 32) panic(exc_names[r->int_no], r);
    uint8_t irq = (uint8_t)(r->int_no - 32);
    if (handlers[irq]) handlers[irq](r);
    pic_eoi(irq);
}
