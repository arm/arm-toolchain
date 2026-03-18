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

extern "C" [[gnu::weak]] void _platform_setup_arch_extensions() {
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

#ifdef __ARM_FEATURE_PAUTH
#ifdef __ARM_FEATURE_PAC_DEFAULT
  // Set to some random numbers to allow testing PACBTI library variants only.
  // Override _platform_setup_arch_extensions() to provide secure keys for
  // production use!
  // The numbers start with ACnn to make it easy to identify during debugging.

  PAC_KEY_P_0 = PAC_KEY_U_0 = 0xAC0017B4;
  PAC_KEY_P_1 = PAC_KEY_U_1 = 0xAC01C9E2;
  PAC_KEY_P_2 = PAC_KEY_U_2 = 0xAC025D8F;
  PAC_KEY_P_3 = PAC_KEY_U_3 = 0xAC03A641;

  // Enable PAC in both privileged and unprivileged mode.
  CONTROL.PAC_EN = 1;
  CONTROL.UPAC_EN = 1;
#endif

#ifdef __ARM_FEATURE_BTI_DEFAULT
  // Enable BTI in both privileged and unprivileged mode.
  CONTROL.BTI_EN = 1;
  CONTROL.UBTI_EN = 1;
#endif
#endif
}

} // namespace misc
} // namespace bootcode

#endif // BOOTCODE_MISC_M_H
