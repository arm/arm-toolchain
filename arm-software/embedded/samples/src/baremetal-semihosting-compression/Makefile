#
# Copyright (c) 2023-2026, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

include ../../Makefile.conf

build: compression-demo.elf

# First, link the image with a simple ld script that does no memory setup and
# places every segment at its execution-time address.
exec.elf: demo.c inner.ld
	$(BIN_PATH)/clang $(MICROBIT_TARGET) $(CRT_SEMIHOST) -T inner.ld -o $@ $<

# Now extract the binary data of each PT_LOAD segment. Because we manually
# specified a fixed set of those segments, we know what address ranges to extract.
#
# Note also that we use --zi, so that the RW data blob will physically contain
# zero bytes on the end covering the ZI data. Including these as the input to
# compress.py will ensure that the decompression step at load time will
# initialise the ZI as well as RW data. (And, of course, any LZ77 compression
# scheme will be able to represent a long string of zeroes compactly.)

exec-ro.bin: exec.elf
	$(BIN_PATH)/elf2bin --bincombined --segments=0x3000-0x1fffffff -O $@ $<

exec-rw.bin: exec.elf
	$(BIN_PATH)/elf2bin --bincombined --segments=0x20000000-0x22000000 --zi -O $@ $<

# Compress the data
rw-compressed.bin: exec-rw.bin
	python3 compress.py $^ -o $@

# (Optional) Check that it decompresses correctly, by building the same
# decompression code into a standalone tool using the system compiler.
deflate-fixed.out: deflate-fixed.c
	cc -DSTANDALONE -o $@ $<

rw-decompressed.bin: deflate-fixed.out rw-compressed.bin
	./$^ $@

check-compression: rw-decompressed.bin exec-rw.bin
	cmp $^

# Disable implicit rule.
%.o : %.s

# Compile rw-compressed.o which ensures the compressed data ends up in the right section
rw-compressed.o: rw-compressed.s rw-compressed.bin
	$(BIN_PATH)/clang $(MICROBIT_TARGET) -g -c -o $@ $<

ro.o: ro.s exec-ro.bin 
	$(BIN_PATH)/clang $(MICROBIT_TARGET) -c -o $@ $<

deflate-fixed.o: deflate-fixed.c
	$(BIN_PATH)/clang $(CFG_FILE) $(MICROBIT_OPTIONS) -g -c -o $@ $<

# Find out the demo file's entry point.
decompress.o: decompress.S exec.elf
	entry=$$($(BIN_PATH)/llvm-readobj --elf-output-style=JSON --file-header exec.elf | python3 -c 'import sys, json; print(json.load(sys.stdin)[0]["ElfHeader"]["Entry"])'); \
	echo "$$entry"; \
	$(BIN_PATH)/clang $(MICROBIT_TARGET) -DDECOMPRESSED_ENTRY="$$entry" -c $< -o $@

compression-demo.elf: decompress.o deflate-fixed.o ro.o rw-compressed.o
	$(BIN_PATH)/ld.lld $^ -Bstatic -T outer.ld -o $@

%.hex: %.elf
	$(BIN_PATH)/llvm-objcopy --output-target ihex $< $@

run: compression-demo.hex check-compression
	qemu-system-arm -M microbit $(QEMU_SEMIHOSTING) -device loader,file=$<,cpu-num=0

debug: compression-demo.elf
	qemu-system-arm -M microbit $(QEMU_SEMIHOSTING) -device loader,file=$<,cpu-num=0 -s -S

clean:
	rm -f *.elf *.hex microbit.ld *.bin *.o *.out

.PHONY: clean run debug
