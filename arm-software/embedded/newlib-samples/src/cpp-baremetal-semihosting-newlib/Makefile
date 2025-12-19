#
# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

include ../../Makefile.conf

CRT_SEMIHOST_NEWLIB=--config=newlib.cfg -lcrt0-rdimon -lrdimon
TARGET=--target=arm-none-eabi -march=armv6m -mfpu=none -mfloat-abi=soft
LD_SCRIPT=-T ../../ldscripts/newlib-cm4.ld
STARTUP=../../startup/startup_ARMCM4.S

build: hello.elf

hello.elf: *.cpp
	$(BIN_PATH)/clang++ $(TARGET) $(CRT_SEMIHOST_NEWLIB) $(CPP_FLAGS) -g $(LD_SCRIPT) -nostartfiles -o hello.elf $(STARTUP) $^
	$(BIN_PATH)/clang++ $(TARGET) $(CRT_SEMIHOST_NEWLIB) -fexceptions -DTEST_EXN -g $(LD_SCRIPT) -nostartfiles -o hello-exn.elf $(STARTUP) $^

%.hex: %.elf
	$(BIN_PATH)/llvm-objcopy -O ihex $< $@

run: hello.hex
	qemu-system-arm -M mps2-an386 -semihosting -nographic -device loader,file=hello.hex
	qemu-system-arm -M mps2-an386 -semihosting -nographic -device loader,file=hello-exn.elf

debug: hello.hex
	qemu-system-arm -M mps2-an386 -semihosting -nographic -device loader,file=$< -s -S

clean:
	rm -f *.elf *.hex

.PHONY: clean run debug
