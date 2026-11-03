//
// Copyright (c) 2026, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// Tiny header file to define the PCS modifier required for functions run
// before startup code enables the FPU.

#ifndef LLVMET_LLVMLIBC_SUPPORT_PLATFORM_SETUP_PCS_H
#define LLVMET_LLVMLIBC_SUPPORT_PLATFORM_SETUP_PCS_H

#ifndef __ARM_ARCH_ISA_A64
#define PLATFORM_SETUP_PCS __attribute__((pcs("aapcs")))
#else
#define PLATFORM_SETUP_PCS
#endif

#endif // LLVMET_LLVMLIBC_SUPPORT_PLATFORM_SETUP_PCS_H
