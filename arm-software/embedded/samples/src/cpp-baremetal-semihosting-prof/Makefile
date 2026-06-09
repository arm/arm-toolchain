#
# Copyright (c) 2023-2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

include ../../Makefile.conf

build: hello.elf

hello.elf: *.cpp
	$(BIN_PATH)/clang -E -P -x c -DLIBC_LD_FILE=$(LIBC_LD_FILE) ../../ldscripts/microbit.ld.in -o microbit.ld
	$(BIN_PATH)/clang $(CFG_FILE) $(MICROBIT_OPTIONS) -g -c proflib.c
	$(BIN_PATH)/clang++ $(CFG_FILE) $(MICROBIT_OPTIONS) $(CRT_SEMIHOST) $(CPP_FLAGS) -g -T microbit.ld -fprofile-instr-generate -fcoverage-mapping -o hello.elf hello.cpp proflib.o

%.hex: %.elf
	$(BIN_PATH)/llvm-objcopy -O ihex $< $@

run: hello.hex
	qemu-system-arm -M microbit $(QEMU_SEMIHOSTING) -device loader,file=$<
	$(BIN_PATH)/llvm-profdata merge -sparse default.profraw -o hello.profdata
	$(BIN_PATH)/llvm-cov show hello.elf -instr-profile=hello.profdata

debug: hello.hex
	qemu-system-arm -M microbit $(QEMU_SEMIHOSTING) -device loader,file=$< -s -S

clean:
	rm -f *.elf *.hex *.profraw *.profdata *.o *.ld

.PHONY: clean run debug
