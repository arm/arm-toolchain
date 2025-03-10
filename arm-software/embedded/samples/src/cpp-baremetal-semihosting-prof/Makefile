#
# Copyright (c) 2023-2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

include ../../Makefile.conf

build: hello.elf

hello.elf: *.cpp
	$(BIN_PATH)/clang $(MICROBIT_TARGET) -g -c proflib.c
	$(BIN_PATH)/clang++ $(MICROBIT_TARGET) $(CRT_SEMIHOST) $(CPP_FLAGS) -g -T ../../ldscripts/microbit.ld -fprofile-instr-generate -fcoverage-mapping -o hello.elf hello.cpp proflib.o

%.hex: %.elf
	$(BIN_PATH)/llvm-objcopy -O ihex $< $@

run: hello.hex
	qemu-system-arm -M microbit -semihosting -nographic -device loader,file=$<
	$(BIN_PATH)/llvm-profdata merge -sparse default.profraw -o hello.profdata
	$(BIN_PATH)/llvm-cov show hello.elf -instr-profile=hello.profdata

debug: hello.hex
	qemu-system-arm -M microbit -semihosting -nographic -device loader,file=$< -s -S

clean:
	rm -f *.elf *.hex *.profraw *.profdata *.o

.PHONY: clean run debug
