//
// Copyright (c) 2022-2026, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM
// Exceptions. See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#include "semihost.h"
#include "platform.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

namespace {
// File cookies
constexpr size_t DEFAULT_FILE_COOKIE_COUNT = 4;
__llvm_libc_semihost_file_cookie
    default_file_cookies[DEFAULT_FILE_COOKIE_COUNT];
} // namespace

extern "C" {
// Override this weak definition with a pool backed by an application-owned
// array to change the maximum number of simultaneously open files.
__attribute__((weak)) __llvm_libc_semihost_file_cookie_pool
    __llvm_libc_semihost_file_cookie_storage = {default_file_cookies,
                                                DEFAULT_FILE_COOKIE_COUNT};
struct __llvm_libc_stdio_cookie __llvm_libc_stdin_cookie;
struct __llvm_libc_stdio_cookie __llvm_libc_stdout_cookie;
struct __llvm_libc_stdio_cookie __llvm_libc_stderr_cookie;
}

namespace {
// File cookie helpers
bool is_std_stream(void *cookie) {
  return cookie == &__llvm_libc_stdin_cookie ||
         cookie == &__llvm_libc_stdout_cookie ||
         cookie == &__llvm_libc_stderr_cookie;
}

size_t get_handle(void *cookie) {
  return static_cast<__llvm_libc_stdio_cookie *>(cookie)->handle;
}

__llvm_libc_semihost_file_cookie *allocate_file() {
  for (size_t i = 0; i < __llvm_libc_semihost_file_cookie_storage.size; ++i) {
    auto *cookie = &__llvm_libc_semihost_file_cookie_storage.cookies[i];
    if (!cookie->in_use) {
      cookie->in_use = true;
      cookie->position = 0;
      return cookie;
    }
  }
  return nullptr;
}

void release_file(void *cookie) {
  static_cast<__llvm_libc_semihost_file_cookie *>(cookie)->in_use = false;
}

off_t get_file_position(void *cookie) {
  return static_cast<__llvm_libc_semihost_file_cookie *>(cookie)->position;
}

void set_file_position(void *cookie, off_t position) {
  static_cast<__llvm_libc_semihost_file_cookie *>(cookie)->position = position;
}

void advance_file_position(void *cookie, off_t amount) {
  set_file_position(cookie, get_file_position(cookie) + amount);
}
} // namespace

namespace {
// Helper functions implemented here to avoid dependency on libc which is not
// available in LLVM libc hermetic testing.
int _isspace(char ch) {
  return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '\v' ||
         ch == '\f';
}

__attribute__((no_builtin("strlen"))) size_t _strlen(const char *str) {
  const char *end = str;
  while (*end)
    ++end;
  return static_cast<size_t>(end - str);
}

// Semihosting helpers
int semihost_errno_negative() {
  int error = semihosting_call(SYS_ERRNO, nullptr);
  return error > 0 ? -error : -EIO;
}

off_t semihost_file_length(size_t handle) {
  size_t args[] = {handle};
  off_t length = semihosting_call(SYS_FLEN, args);
  return length < 0 ? semihost_errno_negative() : length;
}

ssize_t semihost_read(size_t handle, char *buf, size_t size) {
  size_t args[] = {
      handle,
      reinterpret_cast<size_t>(buf),
      size,
  };
  ssize_t not_read = semihosting_call(SYS_READ, args);
  return not_read < 0 ? semihost_errno_negative()
                      : static_cast<ssize_t>(size) - not_read;
}

ssize_t semihost_read_console(char *buf, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    long ch = semihosting_call(SYS_READC, nullptr);
    buf[i] = static_cast<char>(ch);
    if (buf[i] == '\r')
      buf[i] = '\n';
  }
  return static_cast<ssize_t>(size);
}

ssize_t semihost_write(size_t handle, const char *buf, size_t size) {
  size_t args[] = {
      handle,
      reinterpret_cast<size_t>(buf),
      size,
  };
  ssize_t not_written = semihosting_call(SYS_WRITE, args);
  return not_written < 0 ? semihost_errno_negative()
                         : static_cast<ssize_t>(size) - not_written;
}

int semihost_seek(size_t handle, off_t position) {
  size_t args[] = {
      handle,
      static_cast<size_t>(position),
  };
  return semihosting_call(SYS_SEEK, args) == 0 ? 0 : semihost_errno_negative();
}

long semihost_open(const char *path, size_t length, size_t mode) {
  size_t args[] = {
      reinterpret_cast<size_t>(path),
      mode,
      length,
  };
  long handle = semihosting_call(SYS_OPEN, args);
  return handle < 0 ? semihost_errno_negative() : handle;
}

int semihost_close(size_t handle) {
  size_t args[] = {handle};
  return semihosting_call(SYS_CLOSE, args);
}

void stdio_cookie_open(struct __llvm_libc_stdio_cookie *cookie, size_t mode) {
  const char std_stream_name[] = ":tt";
  cookie->handle = static_cast<size_t>(
      semihost_open(std_stream_name, sizeof(std_stream_name) - 1, mode));
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

ssize_t __llvm_libc_stdio_read(void *cookie, char *buf, size_t size) {
  if (cookie == &__llvm_libc_stdin_cookie)
    return semihost_read_console(buf, size);
  if (is_std_stream(cookie))
    return -EBADF;

  ssize_t result = semihost_read(get_handle(cookie), buf, size);
  if (result < 0)
    return result;
  advance_file_position(cookie, result);
  return result;
}

ssize_t __llvm_libc_stdio_write(void *cookie, const char *buf, size_t size) {
  ssize_t result = semihost_write(get_handle(cookie), buf, size);
  if (result < 0)
    return result;
  if (!is_std_stream(cookie))
    advance_file_position(cookie, result);
  return result;
}

int __llvm_libc_stdio_open(const char *path, const char *mode, void **cookie) {
  auto *file_cookie = allocate_file();
  if (!file_cookie)
    return -EMFILE;

  size_t open_mode = OPENMODE_R;
  if (mode[0] == 'w')
    open_mode = OPENMODE_W;
  else if (mode[0] == 'a')
    open_mode = OPENMODE_A;
  for (const char *option = mode + 1; *option != '\0'; ++option) {
    if (*option == '+')
      open_mode |= OPENMODE_PLUS;
    else if (*option == 'b')
      open_mode |= OPENMODE_B;
  }

  long handle = semihost_open(path, _strlen(path), open_mode);
  if (handle < 0) {
    release_file(file_cookie);
    return static_cast<int>(handle);
  }
  file_cookie->stdio_cookie.handle = static_cast<size_t>(handle);
  if (open_mode & OPENMODE_A) {
    off_t length = semihost_file_length(get_handle(file_cookie));
    if (length < 0) {
      semihost_close(get_handle(file_cookie));
      release_file(file_cookie);
      return static_cast<int>(length);
    }
    set_file_position(file_cookie, length);
  }
  *cookie = file_cookie;
  return 0;
}

off_t __llvm_libc_stdio_seek(void *cookie, off_t offset, int whence) {
  if (is_std_stream(cookie))
    return -ESPIPE;

  off_t base;
  if (whence == SEEK_SET)
    base = 0;
  else if (whence == SEEK_CUR)
    base = get_file_position(cookie);
  else if (whence == SEEK_END) {
    base = semihost_file_length(get_handle(cookie));
    if (base < 0)
      return base;
  } else
    return -EINVAL;

  off_t position;
  if (__builtin_add_overflow(base, offset, &position) || position < 0)
    return -EINVAL;
  int error = semihost_seek(get_handle(cookie), position);
  if (error)
    return error;
  set_file_position(cookie, position);
  return position;
}

int __llvm_libc_stdio_set_buffer(void *, char *, size_t, int) {
  // Semihost streams are unbuffered.
  return 0;
}

int __llvm_libc_stdio_flush(void *) {
  // Semihost streams are unbuffered.
  return 0;
}

int __llvm_libc_stdio_close(void *cookie) {
  int result = semihost_close(get_handle(cookie));
  if (!is_std_stream(cookie))
    release_file(cookie);
  return result;
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
  stdio_cookie_open(&__llvm_libc_stdin_cookie, OPENMODE_R);
  stdio_cookie_open(&__llvm_libc_stdout_cookie, OPENMODE_W);
  stdio_cookie_open(&__llvm_libc_stderr_cookie, OPENMODE_W);
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
