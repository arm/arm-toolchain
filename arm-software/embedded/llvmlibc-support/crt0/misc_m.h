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

// Strong _platform_setup_arch_extensions() is required for dummypackeys.cpp
#if MISC_M_PLATFORM_SETUP_ARCH_EXTENSIONS_STRONG
#define MISC_M_PLATFORM_SETUP_ARCH_EXTENSIONS_BINDING
#else
#define MISC_M_PLATFORM_SETUP_ARCH_EXTENSIONS_BINDING [[gnu::weak]]
#endif

extern "C" MISC_M_PLATFORM_SETUP_ARCH_EXTENSIONS_BINDING
    __attribute__((target("pacbti"))) void
    _platform_setup_arch_extensions() {
#if defined(__ARM_FP) || defined(__ARM_FEATURE_MVE)
  constexpr unsigned long CPACR_CP10_FULL_ACCESS = 3UL << (10 * 2);
  constexpr unsigned long CPACR_CP11_FULL_ACCESS = 3UL << (11 * 2);
  constexpr unsigned long NSACR_CP10_ACCESS = 1UL << 10;
  constexpr unsigned long NSACR_CP11_ACCESS = 1UL << 11;

  // CP10 and CP11 must have matching values; set each pair in one write.
  // Enable access to floating-point and MVE instructions when present.
  CPACR.write(CPACR.read() | CPACR_CP10_FULL_ACCESS | CPACR_CP11_FULL_ACCESS);
  // Permit non-secure access too.
  NSACR.write(NSACR.read() | NSACR_CP10_ACCESS | NSACR_CP11_ACCESS);
  __dsb(0xf);
  __isb(0xf);
  // FPCCR also exists with integer MVE and controls its lazy context saving.
  FPCCR.LSPACT = 0;
#endif

  // Enable branch prediction (does nothing if there's no branch predictor)
  CCR.BP = 1;

  // Enable low-overhead-branch cache (does nothing if there's no LOB)
  CCR.LOB = 1;

// Set PAC keys only for the dummypackeys library in dummypackeys.cpp
#ifdef __ARM_FEATURE_PAC_DEFAULT
#ifdef MISC_M_PLATFORM_SETUP_ARCH_EXTENSIONS_SETUP_DUMMY_PAC_KEYS
  MISC_M_PLATFORM_SETUP_ARCH_EXTENSIONS_SETUP_DUMMY_PAC_KEYS;
#endif // MISC_M_PLATFORM_SETUP_ARCH_EXTENSIONS_SETUP_DUMMY_PAC_KEYS
#endif // __ARM_FEATURE_PAC_DEFAULT

#ifdef __ARM_FEATURE_BTI_DEFAULT
  // Enable BTI in both privileged and unprivileged mode.
  CONTROL.BTI_EN = 1;
  CONTROL.UBTI_EN = 1;
#endif // __ARM_FEATURE_BTI_DEFAULT
}

} // namespace misc
} // namespace bootcode

#endif // BOOTCODE_MISC_M_H
