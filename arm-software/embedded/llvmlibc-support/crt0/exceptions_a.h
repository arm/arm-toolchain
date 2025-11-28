//
// Copyright (c) 2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// A-profile exception handling

#ifndef BOOTCODE_EXCEPTIONS_A_H
#define BOOTCODE_EXCEPTIONS_A_H

#include <string.h>
#ifdef __ARM_ARCH_ISA_A64
#include "exceptions_8a.h"
#else
#include "exceptions_7a.h"
#endif
#include "system_registers_a.h"

namespace bootcode {
namespace exceptions {

using namespace sysreg;

#ifndef __ARM_ARCH_ISA_A64
// Floating-point instructions aren't enabled yet
[[gnu::target("no-fpregs")]]
#endif
void setup() {
#if __ARM_ARCH_PROFILE == 'A'
  VBAR = reinterpret_cast<unsigned long>(&vector_table);
#elif __ARM_ARCH_PROFILE == 'R'
  // The vector table is always at address 0. The inline assembly is needed
  // here to hide the memcpy to a null pointer from the compiler.
  void *final_vector_table = NULL;
  // Don't use memcpy as llvmlibc memcpy will fault a copy to address 0.
  // Size is more important than speed here so don't unroll the loop.
  _Pragma("clang loop unroll(disable) vectorize(disable)") for (
      int i = 0; i < 64 / sizeof(unsigned int); i++) {
    (reinterpret_cast<unsigned int *>(final_vector_table))[i] =
        (reinterpret_cast<unsigned int *>(vector_table))[i];
  }
#endif
}

} // namespace exceptions
} // namespace bootcode

#endif // BOOTCODE_EXCEPTIONS_A_H
