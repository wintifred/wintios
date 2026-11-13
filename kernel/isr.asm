[bits 32]
extern isr_handler
global isr_stub_table

%macro ISR_NOERR 1
isr%1:
    push dword 0
    push dword %1
    jmp isr_common
%endmacro
%macro ISR_ERR 1
isr%1:
    push dword %1
    jmp isr_common
%endmacro

section .text
isr_common:
    pusha
    cld
    push esp                       ; struct regs *
    call isr_handler
    add esp, 4
    popa
    add esp, 8                     ; int_no + err_code
    iret

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR   21
%assign n 22
%rep 26
ISR_NOERR n
%assign n n+1
%endrep

section .data
isr_stub_table:
%assign n 0
%rep 48
    dd isr %+ n
%assign n n+1
%endrep
