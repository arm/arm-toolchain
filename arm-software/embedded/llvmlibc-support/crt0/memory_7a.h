//
// Copyright (c) 2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// ARMv7-A memory-related setup

#ifndef BOOTCODE_MEMORY_7A_H
#define BOOTCODE_MEMORY_7A_H

#include "system_registers_a.h"

#define PAGE_TABLE_ENTRY_SIZE 4
#define PAGE_TABLE_ENTRY_COUNT 4096
#define PAGE_TABLE_ALIGNMENT 16384

namespace bootcode {
namespace memory {

using namespace sysreg;

void setup_mmu(volatile unsigned long *pagetable, unsigned long stackheap_start,
               unsigned long stackheap_end) {
  // Find the memory pages that the image and stackheap occupy
  unsigned long start_page = ((unsigned long)setup_mmu) >> 30;
  unsigned long stackheap_page = stackheap_start >> 30;

  // Enable manager access to domain 0
  DACR.D0 = 3;

  // TCR.Control==0 means always use TTBR0
  TCR = 0;

  // Set the base address
  if (!pagetable) {
    // Place after the end of the image
    pagetable = (unsigned long *)stackheap_end;
  }
  // Set the low bit to mark translation table as inner cacheable
  TTBR0 = 1 | (unsigned long)pagetable;

  // Ensure changes to system register are visible before the MMU is enabled
  __isb(0xf);

  // Fill the page table with a flat mapping of 4096 1MB sections with all
  // sections marked as normal.
  //  base address = bits 20:31
  //  bits 18:19 set to 0
  //  nG = bit 17 set to 0 (global)
  //  S = bit 16 set to 0 (non-shared)
  //  APX = bit 15 set to 0 (full read/write)
  //  TEX = bits 12:14 = b111 (normal)
  //  AP = bits 10:11 set to b11 (full read/write)
  //  P = bit 9 set to 0 (no ECC)
  //  domain = bits 5:8 = b000
  //  XN = bit 4 set to 0
  //  C, B bits = bits 2:3 set to b11 (normal)
  //  size = 1MB = bits 0:1 set to b10
  unsigned long entry = 0x7c0e;
  for (unsigned long page = 0; page < 4096; ++page) {
    pagetable[page] = entry | (page << 20);
  }

  // Enable MMU
  __dsb(0xf);
  SCTLR.M = 1;
  __isb(0xf);
}

} // namespace memory
} // namespace bootcode

#endif // BOOTCODE_MEMORY_7A_H
