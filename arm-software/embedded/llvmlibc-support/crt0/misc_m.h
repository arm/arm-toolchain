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

// Default is weak _platform_setup_arch_extensions() that does not set PAC keys
#ifndef MISC_M_PLATFORM_SETUP_ARCH_EXTENSIONS_STRONG
#define MISC_M_PLATFORM_SETUP_ARCH_EXTENSIONS_STRONG 0
#endif

#ifndef MISC_M_PLATFORM_SETUP_ARCH_EXTENSIONS_SETUP_DUMMY_PAC_KEYS
#define MISC_M_PLATFORM_SETUP_ARCH_EXTENSIONS_SETUP_DUMMY_PAC_KEYS 0
#endif

// Strong _platform_setup_arch_extensions() is required for dummypackeys.cpp
#if MISC_M_PLATFORM_SETUP_ARCH_EXTENSIONS_STRONG
#define MISC_M_PLATFORM_SETUP_ARCH_EXTENSIONS_BINDING
#else
#define MISC_M_PLATFORM_SETUP_ARCH_EXTENSIONS_BINDING [[gnu::weak]]
#endif

extern "C" MISC_M_PLATFORM_SETUP_ARCH_EXTENSIONS_BINDING
    __attribute__((target("pacbti"))) void
    _platform_setup_arch_extensions() {
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

// Set PAC keys only for the dummypackeys library in dummypackeys.cpp
#if MISC_M_PLATFORM_SETUP_ARCH_EXTENSIONS_SETUP_DUMMY_PAC_KEYS
#ifdef __ARM_FEATURE_PAC_DEFAULT
  // Set to some random numbers to allow testing PACBTI library variants only.
  // Do not use these keys in production.
  // The numbers start with ACnn to make it easy to identify during debugging.
  PAC_KEY_P_0 = 0xAC0017B4;
  PAC_KEY_P_1 = 0xAC01C9E2;
  PAC_KEY_P_2 = 0xAC025D8F;
  PAC_KEY_P_3 = 0xAC03A641;

  PAC_KEY_U_0 = 0xAC104A7C;
  PAC_KEY_U_1 = 0xAC1191E2;
  PAC_KEY_U_2 = 0xAC123DB5;
  PAC_KEY_U_3 = 0xAC13F068;

  // Enable PAC in both privileged and unprivileged mode.
  CONTROL.PAC_EN = 1;
  CONTROL.UPAC_EN = 1;
#endif // __ARM_FEATURE_PAC_DEFAULT
#endif // MISC_M_PLATFORM_SETUP_ARCH_EXTENSIONS_SETUP_DUMMY_PAC_KEYS

#ifdef __ARM_FEATURE_BTI_DEFAULT
  // Enable BTI in both privileged and unprivileged mode.
  CONTROL.BTI_EN = 1;
  CONTROL.UBTI_EN = 1;
#endif
}

} // namespace misc
} // namespace bootcode

#endif // BOOTCODE_MISC_M_H
