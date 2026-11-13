#include "kernel.h"

void kputc(char c) {
    vga_putc(c);
    if (c == '\n') serial_putc('\r');
    serial_putc(c);
}
static void put_num(uint32_t v, uint32_t base, int upper, int width, char pad) {
    char buf[32]; int n = 0;
    const char *d = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    if (v == 0) buf[n++] = '0';
    while (v) { buf[n++] = d[v % base]; v /= base; }
    while (width > n) { kputc(pad); width--; }
    while (n) kputc(buf[--n]);
}
static void kvprintf(const char *f, va_list ap) {
    for (; *f; f++) {
        if (*f != '%') { kputc(*f); continue; }
        f++;
        char pad = ' '; int width = 0;
        if (*f == '0') { pad = '0'; f++; }
        while (*f >= '0' && *f <= '9') width = width * 10 + (*f++ - '0');
        switch (*f) {
        case 'c': kputc((char)va_arg(ap, int)); break;
        case 's': { const char *s = va_arg(ap, const char *); if (!s) s = "(null)";
                    int l = (int)strlen(s); while (width > l) { kputc(' '); width--; }
                    while (*s) kputc(*s++);
                    break; }
        case 'd': case 'i': { int32_t v = va_arg(ap, int32_t);
                    if (v < 0) { kputc('-'); put_num((uint32_t)(-v), 10, 0, width ? width - 1 : 0, pad); }
                    else put_num((uint32_t)v, 10, 0, width, pad);
                    break; }
        case 'u': put_num(va_arg(ap, uint32_t), 10, 0, width, pad); break;
        case 'x': put_num(va_arg(ap, uint32_t), 16, 0, width, pad); break;
        case 'X': put_num(va_arg(ap, uint32_t), 16, 1, width, pad); break;
        case 'p': kputc('0'); kputc('x'); put_num(va_arg(ap, uint32_t), 16, 0, 8, '0'); break;
        case '%': kputc('%'); break;
        case 0: return;
        default: kputc('%'); kputc(*f); break;
        }
    }
}
void kprintf(const char *fmt, ...) { va_list ap; va_start(ap, fmt); kvprintf(fmt, ap); va_end(ap); }
