[bits 32]
global switch_context
section .text
; void switch_context(uint32_t *old_esp, uint32_t new_esp)
; Saves callee-saved regs + eflags on the current stack, swaps stacks, restores the other thread's.
switch_context:
    mov eax, [esp + 4]
    mov edx, [esp + 8]
    push ebp
    push ebx
    push esi
    push edi
    pushfd
    mov [eax], esp
    mov esp, edx
    popfd
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
