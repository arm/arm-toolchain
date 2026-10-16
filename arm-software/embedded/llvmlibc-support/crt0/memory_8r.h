//
// Copyright (c) 2026, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM
// Exceptions. See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// Armv8-R AArch64 memory-related setup

#ifndef BOOTCODE_MEMORY_8R_H
#define BOOTCODE_MEMORY_8R_H

#include "system_registers_a.h"

namespace bootcode {
namespace memory {

using namespace sysreg;

void setup_mpu() {
  // Enable the 8-R.64 MPU, and configure a single 'normal memory' region
  // covering the whole address map.

  // PRBAR: base address of memory region. Base address
  // itself (most of the register) is 0. The bottom 6 bits are
  // separate fields:
  //
  // bits 5,4 are SH = 0b00: non-shareable
  //
  // bits 3,2 are AP = 0b01: read and write permissions at all ELs
  //
  // bits 1,0 are XN = 0b00: don't prohibit execution at any EL
  __arm_wsr64("S3_4_C6_C8_0", 0x4);

  // PRLAR: limit address of memory region. Mostly all 1s.
  // The bottom 6 bits are separate fields:
  //
  // bit 5 is reserved
  //
  // bit 4 is NS = 1: this is non-secure address space
  //
  // bits 3,2,1 are AttrIndx = 0b000: this memory region is
  // controlled by the low byte of MAIR (see below)
  //
  // bit 0 is EN = 1: this memory region is enabled at all
  __arm_wsr64("S3_4_C6_C8_1", 0x000FFFFFFFFFFFD1UL);

  // MAIR: attributes of the memory region. Each of its 8
  // bytes potentially controls a different class of memory
  // regions; we only set byte 0, because that's the one we
  // referred to in PRLAR above. 0x77 means normal memory
  // with write-back caching.
  MAIR = 0x77;

  // Ensure the region configuration is visible before enabling the MPU.
  __dsb(0xf);

  // SCTLR: the M bit (bit 0) turns on the MPU.
  SCTLR.M = 1;

  // And immediately issue an ISB to synchronise everything with
  // the new state of the world.
  __isb(0xf);
}

} // namespace memory
} // namespace bootcode

#endif // BOOTCODE_MEMORY_8R_H
