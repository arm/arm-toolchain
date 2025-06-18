//
// Copyright (c) 2022-2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// This header file defines the interface between libcrt0.a, which defines
// the program entry point, and libsemihost.a, which implements the
// LLVM-libc porting functions in terms of semihosting. If you replace
// libsemihost.a with something else, this header file shows how to make
// that work with libcrt0.a.

#ifndef LLVMET_LLVMLIBC_SUPPORT_PLATFORM_H
#define LLVMET_LLVMLIBC_SUPPORT_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

// libcrt0.a will call this function after the stack pointer is
// initialized. If any setup specific to the libc porting layer is
// needed, this is where to do it. For example, in semihosting, the
// standard I/O handles must be opened via the SYS_OPEN operation, and
// this function is where libsemihost.a does it.
void _platform_init(void);

#ifdef __cplusplus
}
#endif

#endif // LLVMET_LLVMLIBC_SUPPORT_PLATFORM_H
