//
// Copyright (c) 2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// A-profile memory-related setup

#ifndef BOOTCODE_MEMORY_A_H
#define BOOTCODE_MEMORY_A_H

#include "memory_common.h"
#if __ARM_ARCH_PROFILE == 'A'
#ifdef __ARM_ARCH_ISA_A64
#include "memory_8a.h"
#else
#include "memory_7a.h"
#endif
#endif
#include "system_registers_a.h"

namespace bootcode {
namespace memory {

using namespace sysreg;

void invalidate_cache() {
  // Disable caches and MMU in case they were left enabled from an earlier run
  // This does not need to be done from a cold reset
  SCTLR.I = 0; // Disable instruction cache
  SCTLR.C = 0; // Disable data cache
  SCTLR.M = 0; // Disable MMU
  __isb(0xf);
  __dmb(0xf);

  // Invalidate caches
  unsigned coherence = CLIDR.LoC;
  for (unsigned level = 0; level < coherence; ++level) {
    // We only need to invalidate if we have a data cache at this level
    if (CLIDR.Ctype(level) >= 2) {
      // Select current cache level in CSSELR
      CSSELR = level << 1;
      // ISB required to reflect new CSIDR
      __isb(0xf);

      // Clean and invalidate by set/way
      unsigned int log2_line_size = CCSIDR.LineSize + 4;
      unsigned int sets = CCSIDR.NumSets + 1;
      unsigned int ways = CCSIDR.Associativity + 1;
      unsigned way_offset = __builtin_clz(ways); // 32-log2(number of ways)
      for (int set = 0; set < sets; ++set) {
        for (int way = 0; way < ways; ++way) {
          unsigned long val =
              (level << 1) | (set << log2_line_size) | (way << way_offset);
#if defined(__ARM_ARCH_ISA_A64)
          asm volatile("DC CISW, %0" : : "r"(val));
#else
          asm volatile("MCR p15, 0, %0, c7, c6, 2" : : "r"(val)); // DCCISW
#endif
        }
      }
    }
  }
}

void enable_cache() {
#if defined(__ARM_ARCH_ISA_A64)
  // Invalidate I cache and BTAC
  asm volatile("IC IALLU");
  __dsb(0xf);
  __isb(0xf);
#endif

  // Enable caches
  SCTLR.I = 1;
  SCTLR.C = 1;
  __isb(0xf);
}

void setup() {
  invalidate_cache();
#if __ARM_ARCH_PROFILE == 'A'
  // Put the page table in the .init section so it doesn't later get
  // zero-initialized.
  static_assert(sizeof(unsigned long) == PAGE_TABLE_ENTRY_SIZE);
  static volatile unsigned long pagetable[PAGE_TABLE_ENTRY_COUNT]
      __attribute__((section(".init"), aligned(PAGE_TABLE_ALIGNMENT)));
  setup_mmu(pagetable, get_stackheap_start(), get_stackheap_end());
#endif

#if __ARM_ARCH_PROFILE == 'R'
  // Enable the branch predictor
  SCTLR.Z = 1;
#endif

#ifdef __ARM_FEATURE_MEMORY_TAGGING
  SCTLR.ATA = 1; // Allocation tag access
  SCTLR.TCF = 1; // Synchronous exceptions on tag check failure
  GCR.RRND = 1;  // Enable random tag generation
#endif

#ifndef __ARM_FEATURE_UNALIGNED
  // Enable alignment checks when unaligned accesses are disabled
  SCTLR.A = 1;
#endif
}

} // namespace memory
} // namespace bootcode

#endif // BOOTCODE_MEMORY_A_H
