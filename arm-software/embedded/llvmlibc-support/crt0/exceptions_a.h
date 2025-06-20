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
__attribute__((target("no-fpregs")))
#endif
void setup() {
#if __ARM_ARCH_PROFILE == 'A'
  VBAR = (unsigned long)&vector_table;
#elif __ARM_ARCH_PROFILE == 'R'
  // The vector table is always at address 0. The inline assembly is needed
  // here to hide the memcpy to a null pointer from the compiler.
  void *final_vector_table = NULL;
  asm("" : "+r"(final_vector_table));
  memcpy(final_vector_table, (void *)&vector_table, 64);
#endif
}

} // namespace exceptions
} // namespace bootcode

#endif // BOOTCODE_EXCEPTIONS_A_H
