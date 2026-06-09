//
// Copyright (c) 2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// Exception-related definitions common to both M-profile and A-profile

#ifndef BOOTCODE_EXCEPTIONS_COMMON_H
#define BOOTCODE_EXCEPTIONS_COMMON_H

// We don't want memory tagging in exception handling functions, or the
// functions that they call, as if there's been an exception due to memory
// tagging we may well end up causing a recursive exception.
#define EXFN_ATTR [[clang::no_sanitize("memtag")]]

namespace bootcode {
namespace exceptions {

// The functions below are used when printing out exception information in the
// exception handlers. These are used instead of printf or similar as stdio may
// not yet be initialized at the time an exception occurs, or the exception
// could be a result of an error in stdio itself.

// The output is delegated to the platform specific handler:

extern "C" void _platform_debug_putc(int c);

EXFN_ATTR inline void print_char(int c) { _platform_debug_putc(c); }

EXFN_ATTR inline void print_str(const char *str) {
  for (const char *p = str; *p; ++p)
    print_char(*p);
}

template <typename T>
EXFN_ATTR inline void print_hex(T val, bool print_leading_zeros = true) {
  bool started = print_leading_zeros;
  for (int digit = sizeof(T) * 2 - 1; digit >= 0; --digit) {
    T digit_val = (val >> (digit * 4)) & 0xf;
    if (digit_val == 0 && !started && digit != 0) {
      // Don't print leading zeroes
    } else if (digit_val < 10) {
      print_char('0' + digit_val);
      started = true;
    } else {
      print_char('a' + digit_val - 10);
      started = true;
    }
  }
}

} // namespace exceptions
} // namespace bootcode

// LLVM libc defined platform specific exit handler
extern "C" [[noreturn]] void __llvm_libc_exit(int status);

#endif // BOOTCODE_EXCEPTIONS_COMMON_H
