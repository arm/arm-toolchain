//
// Copyright (c) 2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// M-profile memory-related setup

#ifndef BOOTCODE_MEMORY_M_H
#define BOOTCODE_MEMORY_M_H

#include "memory_common.h"
#include "system_registers_m.h"

namespace bootcode {
namespace memory {

using namespace sysreg;

void enable_cache() {
  // Enable data and instruction caches
  CCR.DC = 1;
  CCR.IC = 1;
  // Ensure that the write to CCR completes before continuing.
  __dsb(0xf);
  __isb(0xf);
}

void setup() {
  // Disable the MPU
  MPU_CTRL.ENABLE = 0;

#ifndef __ARM_FEATURE_UNALIGNED
  // Enable alignment checks when unaligned accesses are disabled
  CCR.UNALIGN_TRP = 1;
#endif
}

} // namespace memory
} // namespace bootcode

#endif // BOOTCODE_MEMORY_M_H
