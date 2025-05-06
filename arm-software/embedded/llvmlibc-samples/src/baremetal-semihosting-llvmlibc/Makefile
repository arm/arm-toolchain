#
# Copyright (c) 2020-2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

include ../../Makefile.conf

build: hello.elf

hello.elf: *.c
	$(BIN_PATH)/clang --config=llvmlibc.cfg $(MICROBIT_TARGET) -nostartfiles -lsemihost -lm -g -fno-exceptions -fno-rtti -T microbit-llvmlibc.ld -o hello.elf $^

%.hex: %.elf
	$(BIN_PATH)/llvm-objcopy -O ihex $< $@

run: hello.hex
	qemu-system-arm -M microbit -semihosting -nographic -device loader,file=$<

debug: hello.hex
	qemu-system-arm -M microbit -semihosting -nographic -device loader,file=$< -s -S

clean:
	rm -f *.elf *.hex

.PHONY: clean run debug
