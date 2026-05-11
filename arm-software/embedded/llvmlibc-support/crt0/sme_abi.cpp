//
// Copyright (c) 2026, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM
// Exceptions. See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#if defined(__ARM_ARCH_ISA_A64)

#include "system_registers_a.h"

// Minimal SME ABI support for compiler-rt and LLVM libc users.
// https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst#811__arm_sme_state
// TODO: Consider caching the state to avoid rechecking id_aa64pfr1_el1
// when running user workloads.
using namespace bootcode::sysreg;

constexpr unsigned long SME_STATE_SUPPORT_MASK = 0xC000000000000000UL;
constexpr unsigned long SME_STATE_SM_MASK = 0x1;
constexpr unsigned long SME_STATE_ZA_MASK = 0x2;

struct SmeState {
  // __arm_sme_state returns its two-word result in registers x0 and x1.
  unsigned long x0;
  unsigned long x1;
};

extern "C" SmeState __arm_sme_state() {
  SmeState state = {0, 0};

  if (ID_AA64PFR1.SME == 0 && ID_AA64PFR1.SME2 == 0)
    return state;

  state.x0 = SME_STATE_SUPPORT_MASK;
  if (SVCR.SM != 0)
    state.x0 |= SME_STATE_SM_MASK;
  if (SVCR.ZA != 0)
    state.x0 |= SME_STATE_ZA_MASK;
  state.x1 = TPIDR2;
  return state;
}

asm(".variant_pcs __arm_sme_state");

#endif
