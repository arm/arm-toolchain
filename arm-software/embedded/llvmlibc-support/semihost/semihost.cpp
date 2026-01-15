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
#include <stdlib.h>

namespace {

void stdio_open(struct __llvm_libc_stdio_cookie *cookie, size_t mode) {
  size_t args[3];
  args[0] = reinterpret_cast<size_t>(":tt");
  args[1] = mode;
  args[2] = static_cast<size_t>(3); /* name length */
  cookie->handle = semihosting_call(SYS_OPEN, args);
}
} // namespace

extern "C" {

static void semihosting_call_exit(int status) {

#if defined(__ARM_64BIT_STATE) && __ARM_64BIT_STATE
  size_t block[2];
  block[0] = ADP_Stopped_ApplicationExit;
  block[1] = status;
  semihosting_call(SYS_EXIT, block);
#else
  if (status == 0) {
    semihosting_call(
        SYS_EXIT, reinterpret_cast<const void *>(ADP_Stopped_ApplicationExit));
  } else {
    semihosting_call(SYS_EXIT, reinterpret_cast<const void *>(
                                   ADP_Stopped_RunTimeErrorUnknown));
  }
#endif

  __builtin_unreachable(); /* semihosting call doesn't return */
}

void __llvm_libc_exit(int status) {
  // TODO: Implement cleanup required by exit(): destructors, atexit, etc
  semihosting_call_exit(status);
}

void abort() {
  // Cleanly exit via semihosting
  // instead of trapping in the default abort() implementation
  semihosting_call_exit(1);
  __builtin_unreachable();
}

struct __llvm_libc_stdio_cookie __llvm_libc_stdin_cookie;
struct __llvm_libc_stdio_cookie __llvm_libc_stdout_cookie;
struct __llvm_libc_stdio_cookie __llvm_libc_stderr_cookie;

// Currently only supports reading from stdin.
// We use SYS_READC for reading from stdin as QEMUs SYS_READ does not block.
// For other files SYS_READ should be used as SYS_READC is intended for console
// input and may block indefinitely in QEMU.
// TODO: Extend to handle regular files when implemented in LLVM libc.

ssize_t __llvm_libc_stdio_read(struct __llvm_libc_stdio_cookie *cookie,
                               char *buf, size_t size) {
  if (cookie != &__llvm_libc_stdin_cookie)
    return -1;
  
  for (size_t i = 0; i < size; ++i) {
    long ch = semihosting_call(SYS_READC, nullptr);
    buf[i] = static_cast<char>(ch & 0xff);
    if (buf[i] == '\r')
      buf[i] = '\n';
  }
  return size;
}

ssize_t __llvm_libc_stdio_write(struct __llvm_libc_stdio_cookie *cookie,
                                const char *buf, size_t size) {
  size_t args[4];
  args[0] = static_cast<size_t>(cookie->handle);
  args[1] = reinterpret_cast<size_t>(buf);
  args[2] = size;
  ssize_t retval = semihosting_call(SYS_WRITE, args);
  if (retval >= 0)
    retval = size - retval;
  return retval;
}

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
