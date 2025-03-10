//
// Copyright (c) 2022-2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#include <stddef.h>

#include "platform.h"
#include "semihost.h"

struct __llvm_libc_stdio_cookie __llvm_libc_stdin_cookie;
struct __llvm_libc_stdio_cookie __llvm_libc_stdout_cookie;
struct __llvm_libc_stdio_cookie __llvm_libc_stderr_cookie;

static void stdio_open(struct __llvm_libc_stdio_cookie *cookie, int mode) {
  size_t args[3];
  args[0] = (size_t) ":tt";
  args[1] = (size_t)mode;
  args[2] = (size_t)3; /* name length */
  cookie->handle = semihosting_call(SYS_OPEN, args);
}

void _platform_init(void) {
  stdio_open(&__llvm_libc_stdin_cookie, OPENMODE_R);
  stdio_open(&__llvm_libc_stdout_cookie, OPENMODE_W);
  stdio_open(&__llvm_libc_stderr_cookie, OPENMODE_W);
}
