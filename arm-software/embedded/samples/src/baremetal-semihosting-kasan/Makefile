#
# Copyright (c) 2026, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

include ../../Makefile.conf

KASAN_FLAGS=\
	-fsanitize=kernel-address \
	-mllvm -asan-instrumentation-with-call-threshold=0 \
	-mllvm -asan-stack=0 \
	-mllvm -asan-instrument-dynamic-allocas=0

ifeq ($(LIBC),llvmlibc)
KASAN_ALLOC_WRAPS=\
	-Wl,--wrap=malloc \
	-Wl,--wrap=calloc \
	-Wl,--wrap=realloc \
	-Wl,--wrap=free
KASAN_RUNTIME_FLAGS=-DKASAN_WRAP_ALLOC
DEFAULT_KASAN_TEST=2
else
DEFAULT_KASAN_TEST=1
endif

ifndef KASAN_TEST
KASAN_TEST=$(DEFAULT_KASAN_TEST)
endif

build: hello.elf

microbit.ld:
	$(BIN_PATH)/clang -E -P -x c -DLIBC_LD_FILE=$(LIBC_LD_FILE) ../../ldscripts/microbit.ld.in -o microbit.ld

hello.o: hello.c
	$(BIN_PATH)/clang $(CFG_FILE) $(MICROBIT_OPTIONS) $(KASAN_FLAGS) -DKASAN_TEST=$(KASAN_TEST) -g -c -o $@ $<

kasan_runtime.o: kasan_runtime.c
	$(BIN_PATH)/clang $(CFG_FILE) $(MICROBIT_OPTIONS) $(KASAN_RUNTIME_FLAGS) -g -c -o $@ $<

hello.elf: hello.o kasan_runtime.o | microbit.ld
	$(BIN_PATH)/clang $(CFG_FILE) $(MICROBIT_OPTIONS) $(CRT_SEMIHOST) -g -T microbit.ld $(KASAN_ALLOC_WRAPS) -o $@ $^

%.hex: %.elf
	$(BIN_PATH)/llvm-objcopy -O ihex $< $@

run: hello.hex
	qemu-system-arm -M microbit $(QEMU_SEMIHOSTING) -device loader,file=$<

debug: hello.hex
	qemu-system-arm -M microbit $(QEMU_SEMIHOSTING) -device loader,file=$< -s -S

clean:
	rm -f *.o *.elf *.hex *.ld

.PHONY: clean run debug
