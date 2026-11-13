#include "kernel.h"
/* Preemptive round-robin scheduler for kernel threads.
 * The PIT tick calls sched_tick(); every SLICE_TICKS it switches to the next READY thread.
 * Thread 0 is the boot context (kmain + shell). Interrupts are off while schedule() runs. */
#define MAX_THREADS 16
#define STACK_SIZE  8192
#define SLICE_TICKS 5
enum { T_UNUSED, T_READY, T_RUNNING, T_SLEEPING, T_DEAD };
struct thread {
    uint32_t esp; uint8_t *stack; int state; char name[16];
    uint32_t wake_tick, run_ticks; volatile uint32_t counter;
    void (*entry)(void *); void *arg;
};
static struct thread threads[MAX_THREADS];
static struct thread *current;
static uint32_t slice_left;
static int enabled;
extern void switch_context(uint32_t *old_esp, uint32_t new_esp);

static void schedule(void) {                 /* call with interrupts disabled */
    struct thread *prev = current;
    uint32_t now = pit_ticks();
    if (prev->state == T_RUNNING) prev->state = T_READY;
    int start = (int)(prev - threads);
    struct thread *next = NULL;
    for (int i = 1; i <= MAX_THREADS; i++) {
        struct thread *t = &threads[(start + i) % MAX_THREADS];
        if (t->state == T_SLEEPING && (int32_t)(now - t->wake_tick) >= 0) t->state = T_READY;
        if (t->state == T_READY) { next = t; break; }
    }
    if (!next) next = &threads[0];
    slice_left = SLICE_TICKS;
    next->state = T_RUNNING;
    if (next == prev) return;
    current = next;
    switch_context(&prev->esp, next->esp);
}

void sched_init(void) {
    memset(threads, 0, sizeof(threads));
    threads[0].state = T_RUNNING;
    const char *n = "kmain"; for (int i = 0; n[i]; i++) threads[0].name[i] = n[i];
    current = &threads[0];
    slice_left = SLICE_TICKS;
    enabled = 1;
}
void sched_tick(void) {
    if (!enabled) return;
    current->run_ticks++;
    if (slice_left > 1) { slice_left--; return; }
    schedule();
}
void thread_yield(void) { uint32_t f = irq_save(); schedule(); irq_restore(f); }
void thread_sleep_ms(uint32_t ms) {
    uint32_t f = irq_save();
    current->wake_tick = pit_ticks() + (ms * pit_hz() + 999) / 1000;
    current->state = T_SLEEPING;
    schedule();
    irq_restore(f);
}
void thread_exit(void) {
    irq_save();
    current->state = T_DEAD;
    schedule();
    for (;;) __asm__ volatile("hlt");
}
static void thread_start(void) { current->entry(current->arg); thread_exit(); }

static void reap(void) {                     /* free stacks of finished threads (never our own) */
    for (int i = 1; i < MAX_THREADS; i++)
        if (threads[i].state == T_DEAD && &threads[i] != current) {
            kfree(threads[i].stack);
            threads[i].stack = NULL; threads[i].state = T_UNUSED;
        }
}
int thread_create(const char *name, void (*fn)(void *), void *arg) {
    uint32_t f = irq_save();
    reap();
    int id = -1;
    for (int i = 1; i < MAX_THREADS; i++) if (threads[i].state == T_UNUSED) { id = i; break; }
    uint8_t *stk = id < 0 ? NULL : kmalloc(STACK_SIZE);
    if (!stk) { irq_restore(f); return -1; }
    struct thread *t = &threads[id];
    memset(t, 0, sizeof(*t));
    for (int i = 0; i < 15 && name[i]; i++) t->name[i] = name[i];
    t->stack = stk; t->entry = fn; t->arg = arg;
    uint32_t *sp = (uint32_t *)(stk + STACK_SIZE);
    *--sp = 0;                               /* fake return address for thread_start */
    *--sp = (uint32_t)thread_start;          /* switch_context's `ret` lands here */
    *--sp = 0; *--sp = 0; *--sp = 0; *--sp = 0;   /* ebp ebx esi edi */
    *--sp = 0x202;                           /* eflags: IF=1 */
    t->esp = (uint32_t)sp;
    t->state = T_READY;
    irq_restore(f);
    return id;
}
int thread_kill(int id) {
    uint32_t f = irq_save();
    int ok = id > 0 && id < MAX_THREADS && &threads[id] != current &&
             (threads[id].state == T_READY || threads[id].state == T_SLEEPING);
    if (ok) threads[id].state = T_DEAD;
    irq_restore(f);
    return ok ? 0 : -1;
}
volatile uint32_t *thread_counter(void) { return &current->counter; }
void thread_list(void) {
    static const char *st[] = { "unused", "ready", "running", "sleeping", "dead" };
    uint32_t f = irq_save();
    kprintf("ID  NAME\t\tSTATE\t\tTICKS\tCOUNTER\n");
    for (int i = 0; i < MAX_THREADS; i++)
        if (threads[i].state != T_UNUSED)
            kprintf("%2d  %s\t%s\t%u\t%u\n", i, threads[i].name, st[threads[i].state],
                    threads[i].run_ticks, threads[i].counter);
    irq_restore(f);
}
