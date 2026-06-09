#
# Copyright (c) 2026, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

LIBC=llvmlibc
include ../../Makefile.conf

ifneq ($(LIBC),llvmlibc)
$(error This sample targets LLVM libc only. Use LIBC=llvmlibc)
endif

KASAN_FLAGS=\
	-fsanitize=kernel-address \
	-mllvm -asan-instrumentation-with-call-threshold=0 \
	-mllvm -asan-mapping-offset=0x1c000000 \
	-mllvm -asan-stack=1 \
	-mllvm -asan-use-after-return=never \
	-mllvm -asan-instrument-dynamic-allocas=0

KASAN_ALLOC_WRAPS=\
	-Wl,--wrap=malloc \
	-Wl,--wrap=calloc \
	-Wl,--wrap=realloc \
	-Wl,--wrap=free

ifndef KASAN_TEST
KASAN_TEST=1
endif

build: hello.elf

microbit-kasan-shadow.ld:
	$(BIN_PATH)/clang -E -P -x c -DLIBC_LD_FILE=$(LIBC_LD_FILE) microbit-kasan-shadow.ld.in -o $@

hello.o: hello.c
	$(BIN_PATH)/clang $(CFG_FILE) $(MICROBIT_OPTIONS) $(KASAN_FLAGS) -DKASAN_TEST=$(KASAN_TEST) -g -c -o $@ $<

kasan_shadow_runtime.o: kasan_shadow_runtime.c
	$(BIN_PATH)/clang $(CFG_FILE) $(MICROBIT_OPTIONS) -g -c -o $@ $<

hello.elf: hello.o kasan_shadow_runtime.o | microbit-kasan-shadow.ld
	$(BIN_PATH)/clang $(CFG_FILE) $(MICROBIT_OPTIONS) $(CRT_SEMIHOST) -g -T microbit-kasan-shadow.ld $(KASAN_ALLOC_WRAPS) -o $@ $^

%.hex: %.elf
	$(BIN_PATH)/llvm-objcopy -O ihex $< $@

run: hello.hex
	qemu-system-arm -M microbit $(QEMU_SEMIHOSTING) -device loader,file=$<

debug: hello.hex
	qemu-system-arm -M microbit $(QEMU_SEMIHOSTING) -device loader,file=$< -s -S

clean:
	rm -f *.o *.elf *.hex *.ld

.PHONY: clean run debug
