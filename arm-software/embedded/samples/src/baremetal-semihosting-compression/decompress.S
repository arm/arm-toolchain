/* decompress.S - entry point for data decompression demo
 *
 * Copyright 2023 Arm Limited. All rights reserved.
 */

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
