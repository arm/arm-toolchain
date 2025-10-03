/* Copyright (c) 2025, Arm Limited and affiliates. 
 *
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

/*
Raspberry Pi does not have flash, but to accommodate for the llvmlibc.ld, we
put the boot_flash, flash and ram momory in Raspberry's RAM. The boot_flash
memory has to be put at fixed address. The flash and ram memory are placed
arbitrarily.

This is enough for simple testing, for real projects appropriate custom
linker script file should be provided.
*/

__boot_flash = 0x80000; /* Default firmware address for Rapberry Pi 3 */
__boot_flash_size = 0x3000; /* 12K of space for added page table */
__flash = 0x10000000;
__flash_size = 0x10000000;
__ram = 0x20000000;
__ram_size = 0x20000000;
__stack_size = 4096;

INCLUDE llvmlibc.ld
