#include "kernel.h"
static void readline(char *buf, int max) {
    int n = 0;
    for (;;) {
        char c = keyboard_getchar();
        if (c == '\n') { kputc('\n'); buf[n] = 0; return; }
        if (c == '\b') { if (n) { n--; kputc('\b'); } continue; }
        if (c >= 32 && c < 127 && n < max - 1) { buf[n++] = c; kputc(c); }
    }
}
static int split(char *s, char **argv, int max) {
    int n = 0;
    while (*s && n < max) {
        while (*s == ' ') *s++ = 0;
        if (!*s) break;
        argv[n++] = s;
        while (*s && *s != ' ') s++;
    }
    return n;
}
static void cmd_mem(void) {
    struct heap_stats h; heap_get_stats(&h);
    uint32_t t = pmm_total_frames(), f = pmm_free_frames();
    kprintf("PMM : %u KiB usable, %u KiB free, %u KiB allocated\n", t * 4, f * 4, (t - f) * 4);
    kprintf("Heap: arena %u KiB | used %u B in %u blocks | free %u B in %u blocks\n",
            h.arena_bytes / 1024, h.used_bytes, h.used_blocks, h.free_bytes, h.free_blocks);
}
static void cmd_memtest(void) {
    /* allocate 1 KiB blocks until OOM, chain them through their first word, then free all */
    uint32_t t = pmm_total_frames();
    void *first = NULL; uint32_t n = 0;
    for (;;) {
        void **p = kmalloc(1024);
        if (!p) break;
        *p = first; first = p; n++;
    }
    struct heap_stats h; heap_get_stats(&h);
    uint32_t arena_pct10 = t ? ((h.arena_bytes / 1024) * 1000) / (t * 4) : 0;
    uint32_t pay_pct10 = t ? (n * 1000u) / (t * 4) : 0;   /* n KiB of payload vs t*4 KiB usable */
    kprintf("allocated %u x 1 KiB blocks before OOM\n", n);
    kprintf("heap owns %u.%u%% of usable RAM; payload in use = %u.%u%% of usable RAM\n",
            arena_pct10 / 10, arena_pct10 % 10, pay_pct10 / 10, pay_pct10 % 10);
    while (first) { void *nx = *(void **)first; kfree(first); first = nx; }
    heap_get_stats(&h);
    kprintf("freed all: %u used blocks, %u free block(s) after coalescing\n", h.used_blocks, h.free_blocks);
}
static void worker(void *arg) {
    char id = (char)(uint32_t)arg;
    uint32_t ms = 150 + (uint32_t)(id - 'A') * 150;
    for (int i = 1; i <= 5; i++) { kprintf("[%c] step %d\n", id, i); thread_sleep_ms(ms); }
}
static void spinner(void *arg) { (void)arg; volatile uint32_t *c = thread_counter(); for (;;) (*c)++; }
static void cmd_threads(void) {
    static const char *names[] = { "worker-A", "worker-B", "worker-C" };
    for (int i = 0; i < 3; i++)
        if (thread_create(names[i], worker, (void *)(uint32_t)('A' + i)) < 0) kprintf("thread_create failed\n");
}
static void cmd_crash(void) { __asm__ volatile("xor %%edx,%%edx\n mov $1,%%eax\n xor %%ecx,%%ecx\n div %%ecx" ::: "eax", "ecx", "edx"); }  /* forces #DE */

void shell_run(void) {
    char line[128]; char *argv[8];
    for (;;) {
        vga_set_color(0x0E); kprintf("winti"); vga_set_color(0x07); kprintf("> ");
        readline(line, sizeof(line));
        int argc = split(line, argv, 8);
        if (!argc) continue;
        if (!strcmp(argv[0], "help"))
            kprintf("help clear mem memtest uptime sleep <ms> alloc <bytes> echo <txt> threads spin ps kill <id> crash reboot\n");
        else if (!strcmp(argv[0], "clear")) vga_clear();
        else if (!strcmp(argv[0], "mem")) cmd_mem();
        else if (!strcmp(argv[0], "memtest")) cmd_memtest();
        else if (!strcmp(argv[0], "uptime")) kprintf("%u ticks @ %u Hz = %u s\n", pit_ticks(), pit_hz(), pit_ticks() / pit_hz());
        else if (!strcmp(argv[0], "sleep") && argc > 1) { pit_sleep_ms(atou(argv[1])); kprintf("done\n"); }
        else if (!strcmp(argv[0], "alloc") && argc > 1) { void *p = kmalloc(atou(argv[1])); kprintf("kmalloc -> %p\n", p); }
        else if (!strcmp(argv[0], "echo")) { for (int i = 1; i < argc; i++) kprintf("%s%c", argv[i], i + 1 < argc ? ' ' : '\n'); if (argc == 1) kputc('\n'); }
        else if (!strcmp(argv[0], "threads")) cmd_threads();
        else if (!strcmp(argv[0], "spin")) { int id = thread_create("spinner", spinner, NULL); kprintf("spinner -> thread %d (never yields; preemption keeps the shell alive)\n", id); }
        else if (!strcmp(argv[0], "ps")) thread_list();
        else if (!strcmp(argv[0], "kill") && argc > 1) kprintf("%s\n", thread_kill((int)atou(argv[1])) ? "no such killable thread" : "killed");
        else if (!strcmp(argv[0], "crash")) cmd_crash();
        else if (!strcmp(argv[0], "reboot")) { outb(0x64, 0xFE); for (;;) __asm__ volatile("hlt"); }
        else kprintf("unknown command: %s\n", argv[0]);
    }
}
