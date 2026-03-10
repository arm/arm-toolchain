#
# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

include ../../Makefile.conf

CRT_SEMIHOST_NEWLIB=--config=newlib.cfg -lcrt0-rdimon -lrdimon

build: hello.elf

hello.elf: *.c
	$(BIN_PATH)/clang $(MICROBIT_TARGET) $(CRT_SEMIHOST_NEWLIB) -g -T ../../ldscripts/newlib-microbit.ld -nostartfiles -o hello.elf ../../startup/startup_ARMCM0.S $^

%.hex: %.elf
	$(BIN_PATH)/llvm-objcopy -O ihex $< $@

run: hello.hex
	qemu-system-arm -M microbit -semihosting -nographic -device loader,file=$< -m 1073741824k

debug: hello.hex
	qemu-system-arm -M microbit -semihosting -nographic -device loader,file=$< -s -S -m 1073741824k

clean:
	rm -f *.elf *.hex

.PHONY: clean run debug
