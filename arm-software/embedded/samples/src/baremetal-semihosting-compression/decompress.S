/* Copyright (c) 2023-2026, Arm Limited and affiliates. 
 * 
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

/* decompress.S - entry point for data decompression demo */

.text
.globl _start, _bp
.type _start, %function

_start:
        /* Set up a stack pointer to run the decompressor with */
        ldr r0, =__stack
        mov sp, r0

        /* Call the decompression routine */
        ldr r0, =decompressed_rw_start /* outptr */
        ldr r1, =compressed_rw_start /* inptr */
        bl  deflate_fixed

        /* Jump to the inner image's entry point, which we expect to be
         * defined on the command line while assembling this file */
        ldr r0, =DECOMPRESSED_ENTRY
        bx  r0

.size _start, . - _start
