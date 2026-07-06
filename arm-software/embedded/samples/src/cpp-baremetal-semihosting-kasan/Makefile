#
# Copyright (c) 2026, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

include ../../Makefile.conf

# Define the memory layout
BOOT_FLASH_ORIGIN=0x00000000
BOOT_FLASH_SIZE=0x3000
FLASH_ORIGIN=0x00003000
FLASH_SIZE=0x3d000
RAM_ORIGIN=0x20000000
HW_RAM_SIZE=0x4000
STACK_SIZE=0x400

# Calculate the shadow layout from the normal memory-map inputs.
#
#   RAM_ORIGIN                            RAM_ORIGIN + HW_RAM_SIZE
#   |------------------------------------------------------------|
#   | Application RAM                     | Shadow RAM           |
#   |<----------- KASAN_RAM_SIZE -------->|<- KASAN_SHADOW_SIZE->|
#                                         |
#                                         KASAN_SHADOW_START
#
# KASan instrumentation maps application addresses with:
#
#   shadow = (address >> KASAN_SHADOW_SCALE) + KASAN_SHADOW_OFFSET
#
# The offset is chosen so RAM_ORIGIN maps to KASAN_SHADOW_START:
# KASAN_SHADOW_OFFSET = KASAN_SHADOW_START - (RAM_ORIGIN >> KASAN_SHADOW_SCALE)

KASAN_SHADOW_SCALE=3
KASAN_LAYOUT=$(shell python3 -c 'kasan_scale = int("$(KASAN_SHADOW_SCALE)", 0); \
ram_origin = int("$(RAM_ORIGIN)", 0); \
hw_ram_size = int("$(HW_RAM_SIZE)", 0); \
shadow_size = hw_ram_size >> kasan_scale; \
shadow_start = ram_origin + hw_ram_size - shadow_size; \
shadow_end = shadow_start + shadow_size; \
shadow_offset = shadow_start - (ram_origin >> kasan_scale); \
ram_size = hw_ram_size - shadow_size; \
print(f"KASAN_SHADOW_SIZE=0x{shadow_size:x} " \
      f"KASAN_SHADOW_START=0x{shadow_start:x} " \
      f"KASAN_SHADOW_END=0x{shadow_end:x} " \
      f"KASAN_SHADOW_OFFSET=0x{shadow_offset:x} " \
      f"KASAN_RAM_SIZE=0x{ram_size:x}")')
$(foreach var,$(KASAN_LAYOUT),$(eval $(var)))

# Number of freed heap blocks to keep poisoned before returning the oldest one
# to the allocator. Larger values improve recent use-after-free detection but
# retain more heap memory.
KASAN_HEAP_QUARANTINE_SIZE=4

KASAN_DEFINES=\
	-DKASAN_SHADOW_SCALE=$(KASAN_SHADOW_SCALE) \
	-DKASAN_SHADOW_OFFSET=$(KASAN_SHADOW_OFFSET) \
	-DKASAN_HEAP_QUARANTINE_SIZE=$(KASAN_HEAP_QUARANTINE_SIZE)

KASAN_MEMORY_MAP=\
	-Wl,--defsym=__boot_flash=$(BOOT_FLASH_ORIGIN) \
	-Wl,--defsym=__boot_flash_size=$(BOOT_FLASH_SIZE) \
	-Wl,--defsym=__flash=$(FLASH_ORIGIN) \
	-Wl,--defsym=__flash_size=$(FLASH_SIZE) \
	-Wl,--defsym=__ram=$(RAM_ORIGIN) \
	-Wl,--defsym=__ram_size=$(KASAN_RAM_SIZE) \
	-Wl,--defsym=__stack_size=$(STACK_SIZE) \
	-Wl,--defsym=__kasan_shadow_scale=$(KASAN_SHADOW_SCALE) \
	-Wl,--defsym=__kasan_shadow_size=$(KASAN_SHADOW_SIZE) \
	-Wl,--defsym=__kasan_shadow_start=$(KASAN_SHADOW_START) \
	-Wl,--defsym=__kasan_shadow_end=$(KASAN_SHADOW_END) \
	-Wl,--defsym=__kasan_shadow_offset=$(KASAN_SHADOW_OFFSET)

KASAN_FLAGS=\
	-fsanitize=kernel-address \
	-mllvm -asan-instrumentation-with-call-threshold=0 \
	-mllvm -asan-mapping-offset=$(KASAN_SHADOW_OFFSET) \
	-mllvm -asan-stack=1 \
	-mllvm -asan-use-after-return=never \
	-mllvm -asan-instrument-dynamic-allocas=0

KASAN_ALLOC_WRAPS=\
	-Wl,--wrap=malloc \
	-Wl,--wrap=calloc \
	-Wl,--wrap=realloc \
	-Wl,--wrap=free

build: hello.elf

hello.o: hello.cpp
	$(BIN_PATH)/clang++ $(CFG_FILE) $(MICROBIT_OPTIONS) \
		$(CPP_FLAGS) --std=c++17 $(KASAN_FLAGS) $(KASAN_DEFINES) \
		-g -c -o $@ $<

kasan_shadow_runtime.o: kasan_shadow_runtime.c
	$(BIN_PATH)/clang $(CFG_FILE) $(MICROBIT_OPTIONS) $(KASAN_DEFINES) -g -c -o $@ $<

hello.elf: hello.o kasan_shadow_runtime.o
	$(BIN_PATH)/clang++ $(CFG_FILE) $(MICROBIT_OPTIONS) \
		$(CRT_SEMIHOST) $(CPP_FLAGS) --std=c++17 -g \
		$(KASAN_MEMORY_MAP) -T $(LIBC_LD_FILE) $(KASAN_ALLOC_WRAPS) \
		-o $@ $^

%.hex: %.elf
	$(BIN_PATH)/llvm-objcopy -O ihex $< $@

run: hello.hex
	qemu-system-arm -M microbit $(QEMU_SEMIHOSTING) -device loader,file=$<

debug: hello.hex
	qemu-system-arm -M microbit $(QEMU_SEMIHOSTING) -device loader,file=$< -s -S

clean:
	rm -f *.o *.elf *.hex *.ld

.PHONY: clean run debug
