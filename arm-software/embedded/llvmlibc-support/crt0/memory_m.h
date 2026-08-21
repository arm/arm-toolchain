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

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) ||                   \
    defined(__ARM_ARCH_8M_BASE__) || defined(__ARM_ARCH_8M_MAIN__) ||          \
    defined(__ARM_ARCH_8_1M_MAIN__)
#define BOOTCODE_M_ARCH_MPU_SUPPORTED 1
#else
#define BOOTCODE_M_ARCH_MPU_SUPPORTED 0
#endif

#if defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__) ||              \
    defined(__ARM_ARCH_8_1M_MAIN__)
#define BOOTCODE_M_ARCH_CACHE_SUPPORTED 1
#else
#define BOOTCODE_M_ARCH_CACHE_SUPPORTED 0
#endif

#if BOOTCODE_M_ARCH_CACHE_SUPPORTED
static inline void sync_barriers() {
  constexpr unsigned int FullSystemScope = 0xf;

  __dsb(FullSystemScope);
  __isb(FullSystemScope);
}

static inline bool is_cache_implemented() { return CLIDR.HasAnyCache(); }

void invalidate_cache() {
  if (CLIDR.HasICache()) {
    sync_barriers();
    ICIALLU = 0; // Instruction Cache Invalidate All to PoU
    sync_barriers();
  }

  if (CLIDR.HasDCache()) {
    CSSELR = 0; // Select Level 1 D-cache in the Cache Size Selection Register
    sync_barriers();

    unsigned long line_size = CCSIDR.LineSize + 4; // Number of offset bits
    unsigned long num_ways = CCSIDR.Associativity + 1;
    unsigned long num_sets = CCSIDR.NumSets + 1;

    // Target DCISW layout:
    // 31                                                             0
    // +----------------------+-----------+---------------------------+
    // | WAY field (top bits) | SET field | 0s for the size of OFFSET |
    // +----------------------+-----------+---------------------------+
    //                                    ^ Shift to fill in OFFSET bits
    // ^ Shift to the very top

    // Shift to consume all leading 0s to get into the top bits
    unsigned long way_shift = __builtin_clz(num_ways - 1);
    unsigned long set_shift = line_size; // Shift by offset bits

    for (unsigned long set = 0; set < num_sets; ++set) {
      for (unsigned long way = 0; way < num_ways; ++way) {
        DCISW = (way << way_shift) | (set << set_shift);
      }
    }
    sync_barriers();
  }
}

void enable_cache() {
  if (CLIDR.HasDCache()) {
    CCR.DC = 1;
  }
  if (CLIDR.HasICache()) {
    CCR.IC = 1;
  }
  // Ensure that the write to CCR completes before continuing.
  sync_barriers();
}
#endif // BOOTCODE_M_ARCH_CACHE_SUPPORTED

extern "C" [[gnu::weak]] void _platform_setup_memory() {
#ifndef __ARM_FEATURE_UNALIGNED
  // Enable alignment checks when unaligned accesses are disabled
  CCR.UNALIGN_TRP = 1;
#endif

#if BOOTCODE_M_ARCH_MPU_SUPPORTED
  // Disable the MPU
  if (MPU_TYPE.HasMPU()) {
    MPU_CTRL.ENABLE = 0;
  }
#endif

#if BOOTCODE_M_ARCH_CACHE_SUPPORTED
  // Enable cache if present
  if (is_cache_implemented()) {
    invalidate_cache();
    enable_cache();
  }
#endif
}

} // namespace memory
} // namespace bootcode

#endif // BOOTCODE_MEMORY_M_H
