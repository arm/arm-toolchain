#
# Copyright (c) 2020-2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

include ../../Makefile.conf

build: hello.elf

hello.elf: *.c
	$(BIN_PATH)/clang $(AARCH64_TARGET) $(CRT_SEMIHOST) -Wl,--pic-veneer -mno-unaligned-access -g -T ../../ldscripts/raspi3b.ld -o hello.elf $^

%.img: %.elf
	$(BIN_PATH)/llvm-objcopy -O binary $< $@

run: hello.img
	qemu-system-aarch64 -M raspi3b -semihosting -nographic -kernel $<

debug: hello.img
	qemu-system-aarch64 -M raspi3b -semihosting -nographic -kernel $< -s -S

clean:
	rm -f *.elf *.img

.PHONY: clean run debug
