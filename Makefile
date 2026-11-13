CC      := gcc
CFLAGS  := -m32 -std=gnu11 -O2 -Wall -Wextra -ffreestanding -fno-pic -fno-pie -fno-stack-protector \
           -fno-builtin -fno-tree-loop-distribute-patterns -fcf-protection=none \
           -fno-asynchronous-unwind-tables -nostdlib -Iinclude
LDFLAGS := -m elf_i386 -T linker.ld --oformat binary -nostdlib
BUILD   := build
CSRC    := $(wildcard kernel/*.c)
ASRC    := kernel/entry.asm kernel/isr.asm kernel/switch.asm
OBJ     := $(patsubst kernel/%.c,$(BUILD)/%.o,$(CSRC)) $(patsubst kernel/%.asm,$(BUILD)/%.ao,$(ASRC))

all: $(BUILD)/wintios.img

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: kernel/%.c include/kernel.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.ao: kernel/%.asm | $(BUILD)
	nasm -f elf32 $< -o $@

$(BUILD)/kernel.bin: $(OBJ) linker.ld
	ld $(LDFLAGS) $(OBJ) -o $@
	@sz=$$(stat -c%s $@); pad=$$(( (512 - sz % 512) % 512 )); \
	 if [ $$pad -ne 0 ]; then dd if=/dev/zero bs=1 count=$$pad >> $@ 2>/dev/null; fi

$(BUILD)/boot.bin: boot/boot.asm $(BUILD)/kernel.bin
	nasm -f bin -DKERNEL_SECTORS=$$(( $$(stat -c%s $(BUILD)/kernel.bin) / 512 )) $< -o $@

$(BUILD)/wintios.img: $(BUILD)/boot.bin $(BUILD)/kernel.bin
	cat $^ > $@
	truncate -s 1M $@
	@echo "built $@ ($$(( $$(stat -c%s $(BUILD)/kernel.bin) / 512 )) kernel sectors)"

run: all
	qemu-system-i386 -m 16M -drive format=raw,file=$(BUILD)/wintios.img -serial stdio

run-nographic: all
	qemu-system-i386 -m 16M -drive format=raw,file=$(BUILD)/wintios.img -display none -serial stdio

debug: all
	qemu-system-i386 -m 16M -drive format=raw,file=$(BUILD)/wintios.img -serial stdio -s -S

clean:
	rm -rf $(BUILD)

.PHONY: all run run-nographic debug clean
