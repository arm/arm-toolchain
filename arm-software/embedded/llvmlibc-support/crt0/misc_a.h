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
  constexpr auto FEAT_SME_1 = 1UL << 24;
  constexpr auto FEAT_SME_2 = 1UL << 25;

  return (static_cast<unsigned long>(ID_AA64PFR1) & (FEAT_SME_1 | FEAT_SME_2)) != 0;
}

[[clang::always_inline]] inline void init_sme_state() {
  if (!has_sme_or_sme2())
    return;

  // TPIDR2_EL0 resets to an architecturally unknown value, so clear it.
  asm volatile("msr s3_3_c13_c0_5, xzr\n\tisb" : : : "memory");

  // Try to set the streaming vector length to the architectural maximum.
  unsigned long smcr;
  if (__arm_rsr("CurrentEL") == 3 << 2) {
    asm volatile("mrs %0, s3_6_c1_c2_6" : "=r"(smcr));
    smcr = (smcr & ~0xfUL) | 0xfUL;
    asm volatile("msr s3_6_c1_c2_6, %0\n\tisb" : : "r"(smcr) : "memory");
  } else if (__arm_rsr("CurrentEL") == 2 << 2) {
    asm volatile("mrs %0, s3_4_c1_c2_6" : "=r"(smcr));
    smcr = (smcr & ~0xfUL) | 0xfUL;
    asm volatile("msr s3_4_c1_c2_6, %0\n\tisb" : : "r"(smcr) : "memory");
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
  __arm_wsr("CPSR_c", 0x11); // FIQ
  asm volatile("mov sp, %0" : : "r"(__builtin_frame_address(0)));
  __arm_wsr("CPSR_c", 0x12); // IRQ
  asm volatile("mov sp, %0" : : "r"(__builtin_frame_address(0)));
  __arm_wsr("CPSR_c", 0x17); // ABT
  asm volatile("mov sp, %0" : : "r"(__builtin_frame_address(0)));
  __arm_wsr("CPSR_c", 0x1B); // UND
  asm volatile("mov sp, %0" : : "r"(__builtin_frame_address(0)));
  __arm_wsr("CPSR_c", 0x1F); // SYS
  asm volatile("mov sp, %0" : : "r"(__builtin_frame_address(0)));
  __arm_wsr("CPSR_c", 0x13); // SVC
#endif
}

} // namespace misc
} // namespace bootcode

#endif // BOOTCODE_MISC_A_H
