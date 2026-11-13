#include "kernel.h"
/* Bitmap physical frame allocator. 1 bit per 4 KiB frame: 1 = used, 0 = free.
 * Everything below 1 MiB (BIOS, VGA, kernel image, stack, bitmap) is never handed out. */
#define FRAME 4096u
#define LOW_LIMIT 0x100000u
static uint32_t *bitmap;
static uint32_t max_frame, total, free_cnt, hint;

static inline int  test(uint32_t f)  { return bitmap[f >> 5] & (1u << (f & 31)); }
static inline void mark(uint32_t f)  { bitmap[f >> 5] |= (1u << (f & 31)); }
static inline void clear(uint32_t f) { bitmap[f >> 5] &= ~(1u << (f & 31)); }

void pmm_init(struct e820_entry *map, uint32_t count, uint32_t kernel_end) {
    uint64_t top = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (map[i].type != 1 || map[i].base >= 0x100000000ULL) continue;
        uint64_t end = map[i].base + map[i].len;
        if (end > 0x100000000ULL) end = 0x100000000ULL;
        if (end > top) top = end;
    }
    max_frame = (uint32_t)(top >> 12);
    bitmap = (uint32_t *)((kernel_end + 15) & ~15u);
    uint32_t words = (max_frame + 31) / 32;
    memset(bitmap, 0xFF, words * 4);                      /* all used */
    for (uint32_t i = 0; i < count; i++) {
        if (map[i].type != 1 || map[i].base >= 0x100000000ULL) continue;
        uint64_t s = map[i].base, e = map[i].base + map[i].len;
        if (e > 0x100000000ULL) e = 0x100000000ULL;
        if (s < LOW_LIMIT) s = LOW_LIMIT;
        s = (s + FRAME - 1) & ~(uint64_t)(FRAME - 1);
        e &= ~(uint64_t)(FRAME - 1);
        for (uint64_t a = s; a < e; a += FRAME) { clear((uint32_t)(a >> 12)); total++; free_cnt++; }
    }
    hint = LOW_LIMIT >> 12;
}

uint32_t pmm_alloc_contiguous(uint32_t n) {
    if (!n || n > free_cnt) return 0;
    uint32_t run = 0, start = 0;
    for (uint32_t k = 0; k < max_frame; k++) {
        uint32_t f = (hint + k) % max_frame;
        if (f == 0) run = 0;                              /* wrapped: runs can't span the seam */
        if (!test(f)) { if (!run) start = f; if (++run == n) {
            for (uint32_t i = 0; i < n; i++) mark(start + i);
            free_cnt -= n; hint = start + n; return start << 12; } }
        else run = 0;
        if (f == max_frame - 1) run = 0;
    }
    return 0;
}
uint32_t pmm_alloc_frame(void) { return pmm_alloc_contiguous(1); }
void pmm_free_frame(uint32_t a) { pmm_free_contiguous(a, 1); }
void pmm_free_contiguous(uint32_t a, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        uint32_t f = (a >> 12) + i;
        if (f < max_frame && test(f)) { clear(f); free_cnt++; }
    }
}
uint32_t pmm_total_frames(void) { return total; }
uint32_t pmm_free_frames(void) { return free_cnt; }

void pmm_dump_map(struct e820_entry *map, uint32_t count) {
    static const char *t[] = { "?", "usable", "reserved", "ACPI reclaim", "ACPI NVS", "bad" };
    for (uint32_t i = 0; i < count; i++)
        kprintf("  %08x - %08x  %s\n", (uint32_t)map[i].base, (uint32_t)(map[i].base + map[i].len),
                map[i].type < 6 ? t[map[i].type] : "?");
}
