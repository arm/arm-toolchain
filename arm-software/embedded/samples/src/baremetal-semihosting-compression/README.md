# Bare-metal compression sample

This sample demonstrates compressing the RW data and using a decompressor routine at startup to build a smaller boot image.

memory_map.ld specifies the relevant memory layout variables for the target machine, and both runtime.ld and compressed.ld include it. The runtime.ld script is used to build exec.elf and represents the execution-time configuration. The Make process then extracts and compresses the RW data, and uses compressed.ld to build compression-demo.elf, which includes the compressed RW data as RO data, and bundles the decompressor code.

The use of these two linker scripts in this manner allows the program to use RW variables transparently without extra modification, as the compressed RO data is decompressed into the expected address space before other code runs.

The picolibc.ld and llvmlibc.ld scripts supplied with the toolchain and used by other samples cannot be used as-is, due to them including the .data section in RO FLASH and copying it to RAM at runtime. These scripts can be modified to work with this compression setup and be used instead of runtime.ld, but we leave this as an exercise to the user.

Notes:
- The decompressor code must be standalone and should not depend on any other code/data that may still be compressed.
- A system-provided cc is required to build the standalone decompressor binary used to verify the RW data was compressed correctly.
- QEMU is run with cpu-num=0 to start at the ELF entry point specified by the compressed.ld phase (which corresponds to decompress.S) rather than the reset address.

This sample uses semihosting with
[QEMU Arm System emulator](https://www.qemu.org/docs/master/system/target-arm.html)
targeting the
[micro:bit board model](https://www.qemu.org/2019/05/22/microbit/).

Running the sample using `make run` should print the message found in demo.c