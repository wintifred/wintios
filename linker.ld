ENTRY(_start)
SECTIONS {
    . = 0x10000;
    .text   : { *(.entry) *(.text*) }
    .rodata : { *(.rodata*) }
    .data   : { *(.data*) }
    .bss    : { __bss_start = .; *(COMMON) *(.bss*) . = ALIGN(16); __bss_end = .; }
    _kernel_end = .;
    /DISCARD/ : { *(.comment) *(.eh_frame*) *(.note*) }
}
