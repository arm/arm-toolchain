//
// Copyright (c) 2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// ARMv8-A memory-related setup

#ifndef BOOTCODE_MEMORY_8A_H
#define BOOTCODE_MEMORY_8A_H

#include "system_registers_a.h"

#define PAGE_TABLE_ENTRY_SIZE 8
#define PAGE_TABLE_ENTRY_COUNT 512
#define PAGE_TABLE_ALIGNMENT 4096

namespace bootcode {
namespace memory {

using namespace sysreg;

void setup_mmu(volatile unsigned long *pagetable, unsigned long stackheap_start,
               unsigned long stackheap_end) {
  // Find the memory pages that the image and stackheap occupy
  unsigned long start_page = ((unsigned long)setup_mmu) >> 30;
  unsigned long stackheap_page = stackheap_start >> 30;

  // Invalidate the TLBs
#if defined(__ARM_ARCH_ISA_A64)
  asm volatile("TLBI VMALLE1");
#else
  asm volatile("MCR p15, 0, r0, c8, c7, 0"); // TLBIALL
#endif

  // Set the base address
  if (!pagetable) {
    // Place at end of image page
    pagetable = (unsigned long *)(((start_page + 1) << 30) -
                                  (512 * sizeof(unsigned long)));
  }
  TTBR0 = (unsigned long)pagetable;

  // No need to program TTBR1_EL2, as we will set TCR_EL3.EPD1=1 to prevent
  // table walks using this table.

  // Set up memory attributes
  // -------------------------
  // This equates to:
  // 0 = b01000100 = Normal, Inner/Outer Non-Cacheable
  // 1 = b11111111 = Normal, Inner/Outer WB/WA/RA
  // 2 = b00000000 = Device-nGnRnE
  // 3 = b11110000 = Normal, Inner/Outer WB/WA/RA, Tagged
  MAIR = 0x00000000F000FF44;

  // Set up TCR_EL2
  // ---------------
  // EPD0=0b0      Table walks using TTBR0 enabled
  // EPD1=0b1      Table walks using TTBR1 disabled
  // TBI*=0b0      Top byte not ignored
  // TG0=0b00      4KB granule
  // T0SZ=0b011001 Limits VA space to 38 bits, translation starts @ l1
  // SH0=0b11      Inner Shareable
  // IGRN0=0b01    Walks to TTBR0 are Inner WB/WA
  // OGRN0=0b01    Walks to TTBR0 are Outer WB/WA
  // AS=0          8-bit ASID
  unsigned long tcr_val = 0x0000000080813519;
#ifdef __ARM_FEATURE_MEMORY_TAGGING
  // If ID_AA64PFR1_EL1 bits [55:52] are non-zero then MTE4 is present.
  if (ID_AA64PFR1 & (0xfUL << 52)) {
    // When MTE4 is present enable canonical tag checking by setting TCR.MTX
    // to 1.
    tcr_val |= 1UL << 33;
  } else {
    // MTE before MTE4 requires the "top byte ignore" feature to be enabled,
    // which is done by setting TCR_ELx.TBI (bit 20) to 1.
    tcr_val |= 1UL << 20;
  }
#endif
  TCR = tcr_val;

  // Ensure changes to system register are visible before the MMU is enabled
  __isb(0xf);

  // Generate TTBR0 L1
  // ------------------
  // First fill the table with Faults
  for (int i = 0; i < 512; ++i) {
    pagetable[i] = 0;
  }

  // Page occupied by the image
  unsigned long image_attrs = 0x405; // Index = 1, AF=1
#ifdef __ARM_FEATURE_MEMORY_TAGGING
  // If we have memory tagging and the stack/heap is in the same page
  // as the image then it needs to be tagged.
  if (start_page == stackheap_page)
    image_attrs = 0x40d; // Index = 3, AF=1
#endif
#ifdef __ARM_FEATURE_BTI_DEFAULT
  // Set the Guarded Page bit
  image_attrs |= 1UL << 50;
#endif
  pagetable[start_page] = image_attrs | (start_page << 30);

  // Page occupied by stack/heap, if it's outside of the above
  if (start_page != stackheap_page) {
    unsigned long stackheap_attrs = 0x405; // Index = 1, AF=1
#ifdef __ARM_FEATURE_MEMORY_TAGGING
    stackheap_attrs = 0x40d; // Index = 3, AF=1
#endif
    stackheap_attrs |= (1UL << 54) | (1UL << 53); // UXN and PXN
    pagetable[stackheap_page] = stackheap_attrs | (stackheap_page << 30);
  }

  __dsb(0xf);
  SCTLR.WXN = 0; // Disable WXN, as it would make the entire image XN
  SCTLR.M = 1;   // Enable MMU
  __isb(0xf);
}

} // namespace memory
} // namespace bootcode

#endif // BOOTCODE_MEMORY_8A_H
