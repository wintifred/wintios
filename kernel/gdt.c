#include "kernel.h"
struct gdt_entry { uint16_t lim_lo, base_lo; uint8_t base_mid, access, gran, base_hi; } __attribute__((packed));
struct gdt_ptr { uint16_t limit; uint32_t base; } __attribute__((packed));
static struct gdt_entry gdt[3];
static struct gdt_ptr gp;

static void set(int i, uint8_t access) {
    gdt[i] = (struct gdt_entry){ 0xFFFF, 0, 0, access, 0xCF, 0 };   /* base 0, limit 4 GiB, 4K gran, 32-bit */
}
void gdt_init(void) {
    gdt[0] = (struct gdt_entry){0};
    set(1, 0x9A);   /* ring0 code */
    set(2, 0x92);   /* ring0 data */
    gp.limit = sizeof(gdt) - 1; gp.base = (uint32_t)gdt;
    __asm__ volatile("lgdt %0\n"
                     "ljmp $0x08, $1f\n"
                     "1: mov $0x10, %%ax\n"
                     "mov %%ax, %%ds\n mov %%ax, %%es\n mov %%ax, %%fs\n mov %%ax, %%gs\n mov %%ax, %%ss\n"
                     :: "m"(gp) : "ax", "memory");
}
