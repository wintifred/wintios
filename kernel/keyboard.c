#include "kernel.h"
static const char map[58] = { 0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' ' };
static const char smap[58] = { 0,27,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,' ' };
#define BUFSZ 256
static volatile char buf[BUFSZ];
static volatile uint32_t head, tail;
static int shift, caps, ext;

static void on_key(struct regs *r) {
    (void)r;
    uint8_t sc = inb(0x60);
    if (sc == 0xE0) { ext = 1; return; }
    if (ext) { ext = 0; return; }                 /* ignore extended keys for now */
    int brk = sc & 0x80; sc &= 0x7F;
    if (sc == 0x2A || sc == 0x36) { shift = !brk; return; }
    if (brk) return;
    if (sc == 0x3A) { caps = !caps; return; }
    if (sc >= sizeof(map)) return;
    char c = shift ? smap[sc] : map[sc];
    if (caps && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) c ^= 0x20;
    if (!c) return;
    uint32_t nh = (head + 1) % BUFSZ;
    if (nh != tail) { buf[head] = c; head = nh; }
}
void keyboard_init(void) { irq_install_handler(1, on_key); }
char keyboard_getchar(void) {
    for (;;) {
        cli();
        if (head != tail) { char c = buf[tail]; tail = (tail + 1) % BUFSZ; sti(); return c; }
        __asm__ volatile("sti; hlt");
    }
}
