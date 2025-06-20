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
#define EXFN_ATTR __attribute__((no_sanitize("memtag")))

namespace bootcode {
namespace exceptions {

// The functions below are used when printing out exception information in the
// exception handlers. These are used instead of printf or similar as stdio may
// not yet be initialized at the time an exception occurs, or the exception
// could be a result of an error in stdio itself.

EXFN_ATTR inline void print_char(unsigned int c) {
#ifdef __ARM_ARCH_ISA_A64
#define REG "x"
#define INSTR "hlt 0xf000"
#else
#define REG "r"
#if __ARM_ARCH_PROFILE == 'M'
#define INSTR "bkpt 0xab"
#elif defined(__thumb__)
#define INSTR "svc 0xab"
#else
#define INSTR "svc 0x123456"
#endif
#endif
  register long v __asm__(REG "0") = 3; // SYS_WRITEC
  register const void *p __asm__(REG "1") = &c;
  __asm__ __volatile__(INSTR : "+r"(v), "+r"(p) : : "memory", "cc");
#undef REG
#undef INSTR
}

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

#endif // BOOTCODE_EXCEPTIONS_COMMON_H
