//
// Copyright (c) 2026, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM
// Exceptions. See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#include "platform.h"

#if __ARM_ARCH_PROFILE == 'A' || __ARM_ARCH_PROFILE == 'R'
#include "memory_a.h"
#elif __ARM_ARCH_PROFILE == 'M'
#include "memory_m.h"
#else
// ARMv4T
// TODO: fill in stub functions once we can start testing LLVM-libc
extern "C" [[gnu::weak]] void _platform_setup_memory() {}
#endif
