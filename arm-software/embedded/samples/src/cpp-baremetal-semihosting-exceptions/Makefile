#
# Copyright (c) 2024-2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

include ../../Makefile.conf

build: hello.hex hello-exn.hex

microbit.ld:
	$(BIN_PATH)/clang -E -P -x c -DLIBC_LD_FILE=$(LIBC_LD_FILE) ../../ldscripts/microbit.ld.in -o microbit.ld


hello.hex: hello.cpp | microbit.ld
	$(BIN_PATH)/clang++ $(CFG_FILE) $(MICROBIT_OPTIONS) $(CRT_SEMIHOST) $(CPP_FLAGS) -print-multi-directory $^ | grep -v "_exn_"
	$(BIN_PATH)/clang++ $(CFG_FILE) $(MICROBIT_OPTIONS) $(CRT_SEMIHOST) $(CPP_FLAGS) -g -T microbit.ld -o hello.elf $^
	$(BIN_PATH)/llvm-objcopy -O ihex hello.elf hello.hex

hello-exn.hex: hello-exn.cpp | microbit.ld
	$(BIN_PATH)/clang++ $(CFG_FILE) $(MICROBIT_OPTIONS) $(CRT_SEMIHOST) -print-multi-directory $^ | grep "_exn_"
	$(BIN_PATH)/clang++ $(CFG_FILE) $(MICROBIT_OPTIONS) $(CRT_SEMIHOST) -g -T microbit.ld -o hello-exn.elf $^
	$(BIN_PATH)/llvm-objcopy -O ihex hello-exn.elf hello-exn.hex

run: hello.hex hello-exn.hex
	qemu-system-arm -M microbit $(QEMU_SEMIHOSTING) -device loader,file=hello.hex
	qemu-system-arm -M microbit $(QEMU_SEMIHOSTING) -device loader,file=hello-exn.hex 2>&1 | grep "caught"

clean:
	rm -f *.elf *.hex *.ld

.PHONY: clean run
