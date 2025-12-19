//
// Copyright (c) 2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// M-profile miscellaneous setup

#ifndef BOOTCODE_MISC_M_H
#define BOOTCODE_MISC_M_H

#include "system_registers_m.h"

namespace bootcode {
namespace misc {

using namespace sysreg;

void setup() {
#ifdef __ARM_FP
  // CPACR enable access to vfp and simd
  CPACR.CP10 = 0x3;
  CPACR.CP11 = 0x3;
  // NSACR enable access to vfp in nonsecure
  NSACR.CP10 = 1;
  NSACR.CP11 = 1;
  // Ensure LSPACT bit is clear in FPCCR
  FPCCR.LSPACT = 0;
#endif

  // Enable branch prediction (does nothing if there's no branch predictor)
  CCR.BP = 1;

  // Enable low-overhead-branch cache (does nothing if there's no LOB)
  CCR.LOB = 1;

}

} // namespace misc
} // namespace bootcode

#endif // BOOTCODE_MISC_M_H
