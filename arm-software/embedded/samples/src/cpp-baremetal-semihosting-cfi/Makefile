#
# Copyright (c) 2023-2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

include ../../Makefile.conf

build: hello.elf
build-no-cfi: hello-no-cfi.elf

hello.elf: *.cpp
	$(BIN_PATH)/clang++ $(MICROBIT_TARGET) $(CPP_FLAGS) -flto -fsanitize=cfi -fvisibility=hidden -fno-sanitize-ignorelist -g -c $^
	$(BIN_PATH)/clang++ $(MICROBIT_TARGET) $(CRT_SEMIHOST) $(CPP_FLAGS) -flto -T ../../ldscripts/microbit.ld -g -o hello.elf hello.o

hello-no-cfi.elf: *.cpp
	$(BIN_PATH)/clang++ $(MICROBIT_TARGET) $(CPP_FLAGS) -flto -g -c $^
	$(BIN_PATH)/clang++ $(MICROBIT_TARGET) $(CRT_SEMIHOST) $(CPP_FLAGS) -flto -T ../../ldscripts/microbit.ld -g -o hello.elf hello.o

%.hex: %.elf
	$(BIN_PATH)/llvm-objcopy -O ihex $< $@

run: hello.hex
	-qemu-system-arm -M microbit -semihosting -nographic -device loader,file=$<

debug: hello.hex
	qemu-system-arm -M microbit -semihosting -nographic -device loader,file=$< -s -S

clean:
	rm -f *.o *.elf *.hex

.PHONY: clean run debug
