#ifndef KERNEL_H
#define KERNEL_H
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

/* ---- port I/O ---- */
static inline void outb(uint16_t p, uint8_t v) { __asm__ volatile("outb %0,%1" :: "a"(v), "Nd"(p)); }
static inline uint8_t inb(uint16_t p) { uint8_t v; __asm__ volatile("inb %1,%0" : "=a"(v) : "Nd"(p)); return v; }
static inline void sti(void) { __asm__ volatile("sti"); }
static inline void cli(void) { __asm__ volatile("cli"); }

/* ---- interrupt frame (matches isr.asm) ---- */
struct regs {
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags;
};

/* ---- vga / serial / printf ---- */
void vga_init(void);
void vga_clear(void);
void vga_putc(char c);
void vga_set_color(uint8_t c);
void serial_init(void);
void serial_putc(char c);
void kputc(char c);
void kprintf(const char *fmt, ...);

/* ---- cpu setup ---- */
void gdt_init(void);
void idt_init(void);
void pic_init(void);
void pic_unmask(uint8_t irq);
typedef void (*irq_handler_t)(struct regs *);
void irq_install_handler(uint8_t irq, irq_handler_t h);
void panic(const char *msg, struct regs *r);

/* ---- devices ---- */
void pit_init(uint32_t hz);
uint32_t pit_ticks(void);
uint32_t pit_hz(void);
void pit_sleep_ms(uint32_t ms);
void keyboard_init(void);
char keyboard_getchar(void);

/* ---- memory ---- */
struct e820_entry { uint64_t base, len; uint32_t type, acpi; } __attribute__((packed));
void pmm_init(struct e820_entry *map, uint32_t count, uint32_t kernel_end);
uint32_t pmm_alloc_frame(void);                 /* phys addr, 0 on failure */
uint32_t pmm_alloc_contiguous(uint32_t frames); /* phys addr, 0 on failure */
void pmm_free_frame(uint32_t addr);
void pmm_free_contiguous(uint32_t addr, uint32_t frames);
uint32_t pmm_total_frames(void);                /* usable frames managed (>= 1 MiB) */
uint32_t pmm_free_frames(void);
void pmm_dump_map(struct e820_entry *map, uint32_t count);

void heap_init(void);
void *kmalloc(size_t n);
void *kcalloc(size_t n, size_t sz);
void kfree(void *p);
struct heap_stats { uint32_t arena_bytes, used_bytes, free_bytes, used_blocks, free_blocks; };
void heap_get_stats(struct heap_stats *s);

/* ---- libc-ish ---- */
void *memset(void *d, int c, size_t n);
void *memcpy(void *d, const void *s, size_t n);
void *memmove(void *d, const void *s, size_t n);
int memcmp(const void *a, const void *b, size_t n);
size_t strlen(const char *s);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);
uint32_t atou(const char *s);

void shell_run(void);
#endif
