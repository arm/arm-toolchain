/* Copyright (c) 2023-2026, Arm Limited and affiliates. 
 * 
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

/* This file ensures the compressed data ends up in rodata and we have a known address to provide the decompressor */

.section .rodata
.globl compressed_rw_start
compressed_rw_start: .incbin "rw-compressed.bin"
