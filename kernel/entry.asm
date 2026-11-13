[bits 32]
extern kmain
extern __bss_start
extern __bss_end
global _start

section .entry progbits alloc exec nowrite align=4
_start:
    cli
    mov esp, 0x90000
    cld
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb
    call kmain
.hang:
    cli
    hlt
    jmp .hang
