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
#include <time.h>

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

bool __llvm_libc_timespec_get_active(struct timespec *ts) {
  long retval = semihosting_call(SYS_CLOCK, 0);
  if (retval == -1)
    return false;

  // Semihosting uses centiseconds
  ts->tv_sec = (retval / 100);
  ts->tv_nsec = (retval % 100) * (1'000'000'000 / 100);
  return true;
}

bool __llvm_libc_timespec_get_utc(struct timespec *ts) {
  long retval = semihosting_call(SYS_TIME, 0);

  // Semihosting uses seconds
  ts->tv_sec = retval;
  ts->tv_nsec = 0;
  return true;
}

// Entry point
void _platform_init(void) {
  stdio_open(&__llvm_libc_stdin_cookie, OPENMODE_R);
  stdio_open(&__llvm_libc_stdout_cookie, OPENMODE_W);
  stdio_open(&__llvm_libc_stderr_cookie, OPENMODE_W);
}
} // extern "C"
