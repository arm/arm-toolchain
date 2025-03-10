//
// Copyright (c) 2022-2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#include <stddef.h>

#include "semihost.h"

void __llvm_libc_exit(int status) {

#if defined(__ARM_64BIT_STATE) && __ARM_64BIT_STATE
  size_t block[2];
  block[0] = ADP_Stopped_ApplicationExit;
  block[1] = status;
  semihosting_call(SYS_EXIT, block);
#else
  semihosting_call(SYS_EXIT, (const void *)ADP_Stopped_ApplicationExit);
#endif

  __builtin_unreachable(); /* semihosting call doesn't return */
}
