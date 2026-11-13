# WintiOS

32-bit x86 hobby kernel: custom BIOS bootloader, VGA text console (+ COM1 mirror), IDT/PIC, PIT timer,
PS/2 keyboard ISR, bitmap physical memory manager (E820-driven), `kmalloc`/`kfree` heap, preemptive
round-robin scheduler (kernel threads, asm context switch, 50 ms slice), tiny shell.

    sudo apt install nasm gcc-multilib qemu-system-x86 make
    make run                 # QEMU window, serial mirrored to terminal
    make run-nographic       # terminal only
    python3 tests/smoke.py   # headless boot + shell commands
    make debug               # QEMU paused, gdb: target remote :1234

Shell: help clear mem memtest uptime sleep alloc echo threads spin ps kill crash reboot

Scheduler demo: `threads` starts 3 sleeping workers; `spin` starts a thread that never yields -
the shell stays responsive because the PIT tick preempts it; `ps` shows per-thread CPU ticks.

Layout: boot/boot.asm (512 B MBR) -> loads kernel to 0x10000 -> kernel/entry.asm -> kmain.c
Memory: 0x8000 E820 table, 0x10000 kernel, 0x90000 stack top; PMM manages RAM >= 1 MiB.
