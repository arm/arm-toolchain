#
# Copyright (c) 2024-2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

include ../../Makefile.conf

build: hello.hex hello-exn.hex

hello.hex: hello.cpp
	$(BIN_PATH)/clang++ $(MICROBIT_TARGET) $(CRT_SEMIHOST) $(CPP_FLAGS) -print-multi-directory -g -T ../../ldscripts/microbit.ld -o hello.elf $^ | grep -v "_exn_"
	$(BIN_PATH)/clang++ $(MICROBIT_TARGET) $(CRT_SEMIHOST) $(CPP_FLAGS) -g -T ../../ldscripts/microbit.ld -o hello.elf $^
	$(BIN_PATH)/llvm-objcopy -O ihex hello.elf hello.hex

hello-exn.hex: hello-exn.cpp
	$(BIN_PATH)/clang++ $(MICROBIT_TARGET) $(CRT_SEMIHOST) -print-multi-directory -g -T ../../ldscripts/microbit.ld -o hello-exn.elf $^ | grep "_exn_"
	$(BIN_PATH)/clang++ $(MICROBIT_TARGET) $(CRT_SEMIHOST) -g -T ../../ldscripts/microbit.ld -o hello-exn.elf $^
	$(BIN_PATH)/llvm-objcopy -O ihex hello-exn.elf hello-exn.hex

run: hello.hex hello-exn.hex
	qemu-system-arm -M microbit -semihosting -nographic -device loader,file=hello.hex
	qemu-system-arm -M microbit -semihosting -nographic -device loader,file=hello-exn.hex 2>&1 | grep "caught"

clean:
	rm -f *.elf *.hex

.PHONY: clean run
