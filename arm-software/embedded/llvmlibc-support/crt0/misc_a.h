//
// Copyright (c) 2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// A-profile miscellaneous setup

#ifndef BOOTCODE_MISC_A_H
#define BOOTCODE_MISC_A_H

#include "system_registers_a.h"

namespace bootcode {
namespace misc {

using namespace sysreg;

#if defined(__ARM_ARCH_ISA_A64)
[[clang::always_inline]] inline bool has_sme_or_sme2() {
  return ID_AA64PFR1.SME != 0 || ID_AA64PFR1.SME2 != 0;
}

[[clang::always_inline]] inline void init_sme_state() {
  if (!has_sme_or_sme2())
    return;

  // TPIDR2_EL0 resets to an architecturally unknown value, so clear it.
  TPIDR2 = 0;
  __isb(0xf);

  // Try to set the streaming vector length to the architectural maximum.
  if (CurrentEL.is(ExceptionLevel::EL3)) {
    SMCR_EL3.LEN = 0xf;
    __isb(0xf);
  } else if (CurrentEL.is(ExceptionLevel::EL2)) {
    SMCR_EL2.LEN = 0xf;
    __isb(0xf);
  }
}
#endif

extern "C" [[gnu::weak]] void _platform_setup_arch_extensions() {
#ifdef __ARM_FEATURE_PAUTH
  // Set all of the pointer authentication keys to different values. In
  // reality these would be set to random values to prevent attackers from
  // being able to predict signed pointer values, but we just set them to
  // arbitary, fixed values for testing.
  APIAKeyLo = 0x1111111111111111;
  APIAKeyHi = 0x1111111111111111;
  APIBKeyLo = 0x2222222222222222;
  APIBKeyHi = 0x2222222222222222;
  APDAKeyLo = 0x3333333333333333;
  APDAKeyHi = 0x3333333333333333;
  APDBKeyLo = 0x4444444444444444;
  APDBKeyHi = 0x4444444444444444;
  APGAKeyLo = 0x5555555555555555;
  APGAKeyHi = 0x5555555555555555;
  // Enable all pointer authentication instructions at the current EL. There
  // is one bit of SCTLR for each of the IA, IB, DA and DB keys. There is no
  // bit for the GA key, which is always enabled.
  SCTLR.EnIA = 1;
  SCTLR.EnIB = 1;
  SCTLR.EnDB = 1;
  SCTLR.EnDA = 1;
#endif

#ifdef __ARM_FP
  // CPACR enable access to vfp and simd
  CPACR.CP10 = 0x3;
  CPACR.CP11 = 0x3;
#if defined(__ARM_ARCH_ISA_A64)
  // Don't trap FP instructions
  CPTR.TFP = 0;
  // Don't trap SVE or SME instructions, for targets which do not have the
  // relevant feature these bits are ignored, so safe to set unconditionally.
  CPTR.EZ = 1;
  CPTR.ESM = 1;
  init_sme_state();
#else
  // Enable VFP and SIMD
  __arm_wsr("fpexc", 1 << 30);
#endif
#endif

  // Enable cycle counter in EL2 if PMUv3 is implementerd
  if (ID_DFR0.PerfMon != 0xF && ID_DFR0.PerfMon >= 3)
    PMCCFILTR.NSH = 1;

#if !defined(__ARM_ARCH_ISA_A64) &&                                            \
    !(__ARM_ARCH_PROFILE == 'R' && __ARM_ARCH >= 8)
  // Copy the current sp value to each of the banked copies of sp.
  asm volatile(
      "mov r0, sp\n"
      "mov r1, #0x11\n" // FIQ
      "msr CPSR_c, r1\n"
      "mov sp, r0\n"
      "mov r1, #0x12\n" // IRQ
      "msr CPSR_c, r1\n"
      "mov sp, r0\n"
      "mov r1, #0x17\n" // ABT
      "msr CPSR_c, r1\n"
      "mov sp, r0\n"
      "mov r1, #0x1B\n" // UND
      "msr CPSR_c, r1\n"
      "mov sp, r0\n"
      "mov r1, #0x1F\n" // SYS
      "msr CPSR_c, r1\n"
      "mov sp, r0\n"
      "mov r1, #0x13\n" // return to SVC
      "msr CPSR_c, r1" : : : "r0", "r1");
#endif
}

} // namespace misc
} // namespace bootcode

#endif // BOOTCODE_MISC_A_H
