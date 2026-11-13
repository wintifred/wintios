#include "kernel.h"
/* First-fit free-list allocator with split + coalesce. Grows by taking contiguous
 * frames from the PMM (identity mapped, so phys == virt). 16-byte header, 16-byte alignment. */
#define MAGIC_USED 0xA110C8EDu
#define MAGIC_FREE 0xF4EEB10Cu
#define GROW_BYTES (64u * 1024)
typedef struct blk { uint32_t magic, size; struct blk *next, *prev; } blk_t;   /* 16 bytes */
#define HDR ((uint32_t)sizeof(blk_t))
static blk_t *head, *tail;
static uint32_t arena;

static int adjacent(blk_t *a, blk_t *b) { return (uint8_t *)a + HDR + a->size == (uint8_t *)b; }
static void merge_next(blk_t *b) {
    blk_t *n = b->next;
    b->size += HDR + n->size;
    b->next = n->next;
    if (n->next) n->next->prev = b; else tail = b;
    n->magic = 0;
}
static int grow(uint32_t need) {
    uint32_t bytes = (need + HDR + 4095) & ~4095u;
    uint32_t chunk = bytes > GROW_BYTES ? bytes : GROW_BYTES;
    uint32_t addr = pmm_alloc_contiguous(chunk >> 12);
    if (!addr) { chunk = bytes; addr = pmm_alloc_contiguous(chunk >> 12); }
    if (!addr) {                                           /* last resort: take whatever fits in one frame run */
        for (chunk = bytes; chunk > 4096 && !addr; ) { chunk -= 4096; addr = pmm_alloc_contiguous(chunk >> 12); }
        if (!addr) return 0;
    }
    blk_t *b = (blk_t *)addr;
    b->magic = MAGIC_FREE; b->size = chunk - HDR; b->next = NULL; b->prev = tail;
    if (tail) tail->next = b; else head = b;
    tail = b; arena += chunk;
    if (b->prev && b->prev->magic == MAGIC_FREE && adjacent(b->prev, b)) merge_next(b->prev);
    return 1;
}
void heap_init(void) { head = tail = NULL; arena = 0; grow(0); }

void *kmalloc(size_t n) {
    if (!n) return NULL;
    uint32_t need = ((uint32_t)n + 15) & ~15u;
    for (;;) {
        for (blk_t *b = head; b; b = b->next) {
            if (b->magic != MAGIC_FREE || b->size < need) continue;
            if (b->size >= need + HDR + 16) {              /* split */
                blk_t *r = (blk_t *)((uint8_t *)(b + 1) + need);
                r->magic = MAGIC_FREE; r->size = b->size - need - HDR;
                r->next = b->next; r->prev = b;
                if (b->next) b->next->prev = r; else tail = r;
                b->next = r; b->size = need;
            }
            b->magic = MAGIC_USED;
            return b + 1;
        }
        if (!grow(need)) return NULL;
    }
}
void *kcalloc(size_t n, size_t sz) { void *p = kmalloc(n * sz); if (p) memset(p, 0, n * sz); return p; }
void kfree(void *p) {
    if (!p) return;
    blk_t *b = (blk_t *)p - 1;
    if (b->magic == MAGIC_FREE) panic("kfree: double free", NULL);
    if (b->magic != MAGIC_USED) panic("kfree: bad pointer / heap corruption", NULL);
    b->magic = MAGIC_FREE;
    if (b->next && b->next->magic == MAGIC_FREE && adjacent(b, b->next)) merge_next(b);
    if (b->prev && b->prev->magic == MAGIC_FREE && adjacent(b->prev, b)) merge_next(b->prev);
}
void heap_get_stats(struct heap_stats *s) {
    memset(s, 0, sizeof(*s)); s->arena_bytes = arena;
    for (blk_t *b = head; b; b = b->next) {
        if (b->magic == MAGIC_USED) { s->used_bytes += b->size; s->used_blocks++; }
        else { s->free_bytes += b->size; s->free_blocks++; }
    }
}
