/* Copyright (c) 2026, Arm Limited and affiliates. 
 * 
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

__flash = DEFINED(__flash) ? __flash : 0x00003000;
__ram = DEFINED(__ram) ? __ram : 0x20000000;
__ram_size = DEFINED(__ram_size) ? __ram_size : 0x4000;
