//
// Copyright (c) 2022-2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions. 
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#include "semihost.h"
#include "platform.h"

#include <stddef.h>

namespace {

void stdio_open(struct __llvm_libc_stdio_cookie *cookie, int mode) {
  size_t args[3];
  args[0] = (size_t)":tt";
  args[1] = (size_t)mode;
  args[2] = (size_t)3; /* name length */
  cookie->handle = semihosting_call(SYS_OPEN, args);
}
} // namespace

extern "C" {

// LLVM-libc doesn't implement `errno`, assume single-threading
int *__llvm_libc_errno() {
  static int internal_err;
  return &internal_err;
}

void __llvm_libc_exit(int status) {

#if defined(__ARM_64BIT_STATE) && __ARM_64BIT_STATE
  size_t block[2];
  block[0] = ADP_Stopped_ApplicationExit;
  block[1] = status;
  semihosting_call(SYS_EXIT, block);
#else
  semihosting_call(SYS_EXIT, (const void *)ADP_Stopped_ApplicationExit);
#endif

  __builtin_unreachable(); /* semihosting call doesn't return */
}

ssize_t __llvm_libc_stdio_read(struct __llvm_libc_stdio_cookie *cookie,
                               const char *buf, size_t size) {
  size_t args[4];
  args[0] = (size_t)cookie->handle;
  args[1] = (size_t)buf;
  args[2] = (size_t)size;
  args[3] = 0;
  ssize_t retval = semihosting_call(SYS_READ, args);
  if (retval >= 0)
    retval = size - retval;
  return retval;
}

ssize_t __llvm_libc_stdio_write(struct __llvm_libc_stdio_cookie *cookie,
                                const char *buf, size_t size) {
  size_t args[4];
  args[0] = (size_t)cookie->handle;
  args[1] = (size_t)buf;
  args[2] = (size_t)size;
  ssize_t retval = semihosting_call(SYS_WRITE, args);
  if (retval >= 0)
    retval = size - retval;
  return retval;
}

struct __llvm_libc_stdio_cookie __llvm_libc_stdin_cookie;
struct __llvm_libc_stdio_cookie __llvm_libc_stdout_cookie;
struct __llvm_libc_stdio_cookie __llvm_libc_stderr_cookie;

// Entry point
void _platform_init(void) {
  stdio_open(&__llvm_libc_stdin_cookie, OPENMODE_R);
  stdio_open(&__llvm_libc_stdout_cookie, OPENMODE_W);
  stdio_open(&__llvm_libc_stderr_cookie, OPENMODE_W);
}
} // extern "C"
