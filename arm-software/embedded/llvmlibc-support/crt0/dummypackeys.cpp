//
// Copyright (c) 2026, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM
// Exceptions. See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#if __ARM_ARCH_PROFILE == 'M'

#define MISC_M_PLATFORM_SETUP_ARCH_EXTENSIONS_STRONG 1
#define MISC_M_PLATFORM_SETUP_ARCH_EXTENSIONS_SETUP_DUMMY_PAC_KEYS             \
  do {                                                                         \
    /* Dummy PAC keys are for PACBTI library variant testing only. */          \
    /* Do not use these keys in production. */                                 \
    /* The values start with ACnn to aid debugging. */                         \
    PAC_KEY_P_0 = 0xAC0017B4;                                                  \
    PAC_KEY_P_1 = 0xAC01C9E2;                                                  \
    PAC_KEY_P_2 = 0xAC025D8F;                                                  \
    PAC_KEY_P_3 = 0xAC03A641;                                                  \
    PAC_KEY_U_0 = 0xAC104A7C;                                                  \
    PAC_KEY_U_1 = 0xAC1191E2;                                                  \
    PAC_KEY_U_2 = 0xAC123DB5;                                                  \
    PAC_KEY_U_3 = 0xAC13F068;                                                  \
                                                                               \
    /* Enable PAC in both privileged and unprivileged mode. */                 \
    CONTROL.PAC_EN = 1;                                                        \
    CONTROL.UPAC_EN = 1;                                                       \
  } while (false)

#include "misc_m.h"

#endif
