#include "kernel.h"
#define VGA ((volatile uint16_t *)0xB8000)
#define COLS 80
#define ROWS 25
static int cx, cy;
static uint8_t color = 0x07;

static void cursor(void) {
    uint16_t p = (uint16_t)(cy * COLS + cx);
    outb(0x3D4, 14); outb(0x3D5, p >> 8);
    outb(0x3D4, 15); outb(0x3D5, p & 0xFF);
}
void vga_set_color(uint8_t c) { color = c; }
void vga_clear(void) {
    for (int i = 0; i < COLS * ROWS; i++) VGA[i] = (uint16_t)(color << 8) | ' ';
    cx = cy = 0; cursor();
}
void vga_init(void) { vga_clear(); }
static void scroll(void) {
    for (int i = 0; i < COLS * (ROWS - 1); i++) VGA[i] = VGA[i + COLS];
    for (int i = COLS * (ROWS - 1); i < COLS * ROWS; i++) VGA[i] = (uint16_t)(color << 8) | ' ';
    cy = ROWS - 1;
}
void vga_putc(char c) {
    if (c == '\n') { cx = 0; cy++; }
    else if (c == '\r') cx = 0;
    else if (c == '\t') cx = (cx + 8) & ~7;
    else if (c == '\b') { if (cx > 0) { cx--; VGA[cy * COLS + cx] = (uint16_t)(color << 8) | ' '; } }
    else { VGA[cy * COLS + cx] = (uint16_t)(color << 8) | (uint8_t)c; cx++; }
    if (cx >= COLS) { cx = 0; cy++; }
    if (cy >= ROWS) scroll();
    cursor();
}
