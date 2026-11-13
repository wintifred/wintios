; WintiOS stage-1 bootloader (512 bytes, BIOS/MBR)
; real mode: E820 memory map -> A20 -> load kernel (INT 13h ext) -> GDT -> protected mode -> jump
[bits 16]
[org 0x7C00]

KERNEL_SEG   equ 0x1000            ; kernel loaded at 0x1000:0 = 0x10000
E820_COUNT   equ 0x8000            ; dword: number of entries
E820_TABLE   equ 0x8004            ; 24-byte entries
%ifndef KERNEL_SECTORS
  %define KERNEL_SECTORS 64
%endif

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [boot_drive], dl

    mov si, msg_boot
    call print16
    call detect_memory

    in   al, 0x92                  ; fast A20
    or   al, 2
    and  al, 0xFE                  ; never set bit0 (system reset)
    out  0x92, al

    call load_kernel

    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or  eax, 1
    mov cr0, eax
    jmp 0x08:pm_entry

; ---------------------------------------------------------------
detect_memory:
    xor ebx, ebx
    xor bp, bp
    mov di, E820_TABLE
.next:
    mov eax, 0xE820
    mov edx, 0x534D4150            ; 'SMAP'
    mov ecx, 24
    mov dword [es:di+20], 1
    int 0x15
    jc  .done
    cmp eax, 0x534D4150
    jne .done
    jcxz .skip
    inc bp
    add di, 24
    cmp bp, 32
    jae .done
.skip:
    test ebx, ebx
    jnz .next
.done:
    mov [E820_COUNT], bp
    mov word [E820_COUNT+2], 0
    ret

load_kernel:
    mov cx, KERNEL_SECTORS
.loop:
    mov ax, cx
    cmp ax, 32
    jbe .go
    mov ax, 32
.go:
    mov [dap_count], ax
    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc  disk_error
    mov ax, [dap_count]
    sub cx, ax
    add word [dap_lba], ax
    adc word [dap_lba+2], 0
    shl ax, 5                      ; sectors * 512 / 16 = paragraphs
    add [dap_seg], ax
    test cx, cx
    jnz .loop
    ret

disk_error:
    mov si, msg_disk
    call print16
    jmp $

print16:
    mov ah, 0x0E
.l: lodsb
    test al, al
    jz .d
    int 0x10
    jmp .l
.d: ret

; ---------------------------------------------------------------
[bits 32]
pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    jmp 0x08:(KERNEL_SEG << 4)

; ---------------------------------------------------------------
align 4
gdt:
    dq 0
    dq 0x00CF9A000000FFFF          ; 0x08 code, base 0, 4 GiB
    dq 0x00CF92000000FFFF          ; 0x10 data
gdt_end:
gdt_desc:
    dw gdt_end - gdt - 1
    dd gdt

dap:
    db 0x10, 0
dap_count: dw 0
dap_off:   dw 0
dap_seg:   dw KERNEL_SEG
dap_lba:   dd 1
           dd 0
boot_drive: db 0
msg_boot: db "WintiOS: booting...", 13, 10, 0
msg_disk: db "disk read error", 0

times 510-($-$$) db 0
dw 0xAA55
