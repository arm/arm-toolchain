#
# Copyright (c) 2023-2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

include ../../Makefile.conf

build: hello.elf
build-trap: hello-trap.elf

microbit.ld:
	$(BIN_PATH)/clang -E -P -x c -DLIBC_LD_FILE=$(LIBC_LD_FILE) ../../ldscripts/microbit.ld.in -o microbit.ld

hello.elf: *.cpp | microbit.ld		# build with minimal runtime mode
	$(BIN_PATH)/clang++ $(CFG_FILE) $(MICROBIT_OPTIONS) $(CRT_SEMIHOST) $(CPP_FLAGS) --std=c++17 -fsanitize=undefined -fsanitize-minimal-runtime -g -T microbit.ld -o hello.elf $^

hello-trap.elf: *.cpp | microbit.ld	# build with trap mode
	$(BIN_PATH)/clang++ $(CFG_FILE) $(MICROBIT_OPTIONS) $(CRT_SEMIHOST) $(CPP_FLAGS) --std=c++17 -fsanitize=undefined -fsanitize-trap=all -g -T microbit.ld -o hello.elf $^

%.hex: %.elf
	$(BIN_PATH)/llvm-objcopy -O ihex $< $@

run: hello.hex
	qemu-system-arm -M microbit $(QEMU_SEMIHOSTING) -device loader,file=$<

debug: hello.hex
	qemu-system-arm -M microbit $(QEMU_SEMIHOSTING) -device loader,file=$< -s -S

clean:
	rm -f *.elf *.hex *.ld

.PHONY: clean run debug
