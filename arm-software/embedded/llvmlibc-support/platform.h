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

#include "platform_setup_pcs.h"

#ifdef __cplusplus
extern "C" {
#endif

// libcrt0.a will call these functions after the stack pointer is
// initialized. If any setup specific to the libc porting layer is
// needed, this is where to do it.

// Set up the exceptions table and enable relevant interrupts.
PLATFORM_SETUP_PCS void _platform_setup_exceptions(void);

// Set up the Memory Management Unit and caches.
void _platform_setup_memory(void);

// Set up architecture extensions that require special initialization.
void _platform_setup_arch_extensions(void);

// Relocate read-write data into its runtime memory and clear the BSS region.
void _platform_init_data_segments(void);

// Initialize thread-local storage (TLS) for the initial execution context.
void _platform_init_tls(void);

// Any other initialization right before the main function is called,
// for example, in semihosting, the standard I/O handles must be opened
// via the SYS_OPEN operation, and this function is where libsemihost.a does it.
void _platform_init(void);

// Output one character to the debug console to report bootcode errors
void _platform_debug_putc(int c);

// Get command line options (argc/argv) for the main function
int _platform_get_argv(char *cmdline, int max_cmdline,
                                  const char **argv, int max_argv);

#ifdef __cplusplus
}
#endif

#endif // LLVMET_LLVMLIBC_SUPPORT_PLATFORM_H
