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
#include <stdlib.h>
#include <time.h>

namespace {

bool stdio_open(struct __llvm_libc_stdio_cookie *cookie, size_t mode) {
  const char std_stream_name[] = ":tt";
  size_t args[] = {
      reinterpret_cast<size_t>(std_stream_name),
      mode,
      sizeof(std_stream_name) - 1UL,
  };
  cookie->handle = semihosting_call(SYS_OPEN, args);
  return cookie->handle >= 0;
}
} // namespace

extern "C" {

static void semihosting_call_exit(int status) {

#if defined(__ARM_64BIT_STATE) && __ARM_64BIT_STATE
  size_t block[] = {
      ADP_Stopped_ApplicationExit,
      static_cast<size_t>(status),
  };
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
  // Application state cleanup is done in libc exit() that calls internal::exit()
  // that calls this handler in turn.
  semihosting_call_exit(status);
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
  size_t args[] = {
      static_cast<size_t>(cookie->handle),
      reinterpret_cast<size_t>(buf),
      size,
  };
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
  // The convention of opening ":tt" in append mode to specify stderr is not
  // supported by all semihosting implementations. If this open fails, retry in
  // write mode, because having stderr squashed into stdout is better than not
  // having it at all.
  if (!stdio_open(&__llvm_libc_stderr_cookie, OPENMODE_A))
    stdio_open(&__llvm_libc_stderr_cookie, OPENMODE_W);
}

// Debug output
void _platform_debug_putc(int c) {
  unsigned char ch = (unsigned char)c;

  semihosting_call(SYS_WRITEC, &ch);
}

// Provide command line options (argc/argv) for the main function

// Supported features:
// - Arguments are split by whitespace.
// - Quoted text is copied as-is: "a b c " or 'a b c ' will keep all spaces.
//   Not closed quote will run till the end of the provided line.
// - Escape sequences: \ copies next char as-is unless inside ' quotes
//   or at the end of the string.

// Helper functions implemented here to avoid dependency on libc which is not
// available in LLVM libc hermetic testing.
static int _isspace(char ch) {
  return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '\v' ||
         ch == '\f';
}

__attribute__((no_builtin("strlen"))) static size_t _strlen(const char *str) {
  const char *pend = str;
  while (*pend) {
    pend++;
  }
  return (size_t)(pend - str);
}

static inline void skip_spaces(const char *&p) {
  while (_isspace(*p))
    ++p;
}

static int parse_cmdline_buf(char *buf) {
  if (!buf)
    return -1;

  int argc = 0;
  const char *p = buf;
  char *w = buf;
  skip_spaces(p);

  while (*p != '\0') {
    // Start of token
    argc++;

    char quote = '\0';
    while (*p != '\0') {
      char c = *p++;

      if (c == '\\') {
        // Handle escape: copy next symbol unless inside ' quote or at the end
        if (quote != '\'' && *p != '\0')
          c = *p++;
      } else if (!quote && (c == '"' || c == '\'')) {
        quote = c; // Begin quoted section
        continue;
      } else if (quote && c == quote) {
        quote = '\0'; // End quoted section
        continue;
      } else if (!quote && _isspace(c)) {
        break; // End of token
      }

      *w++ = c;
    }

    *w++ = '\0'; // Null-terminate token

    skip_spaces(p);
  }

  return argc;
}

static int fill_argv_from_parsed_buf(const char *buf, const char **argv,
                                     int argc) {
  for (int i = 0; i < argc; i++) {
    argv[i] = buf;
    buf += _strlen(buf) + 1;
  }
  argv[argc] = nullptr;
  return argc;
}

// Parse the command line into argc/argv for the main function.
//
// Must be called with argv == nullptr first to get the number of arguments,
// then with allocated argv and the number of arguments from the first call
// to fill in the argv.
int _platform_get_argv(char *cmdline, int max_cmdline, const char **argv,
                       int max_argv) {
  if (!cmdline || max_cmdline <= 0)
    return -1;

  if (argv && max_argv <= 0)
    return -1;

  if (argv)
    return fill_argv_from_parsed_buf(cmdline, argv, max_argv - 1);

  struct {
    char *buf;
    int len;
  } get_cmdline_args = {cmdline, max_cmdline};

  if (semihosting_call(SYS_GET_CMDLINE, &get_cmdline_args) != 0)
    return -1;

  return parse_cmdline_buf(cmdline);
}
} // extern "C"
