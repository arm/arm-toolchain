//
// Copyright (c) 2026, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM
// Exceptions. See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

extern "C" {

// Do nothing for platform initialization
[[gnu::weak]] void _platform_init() {}

// Go into busy infinite loop on exit
[[gnu::weak, noreturn]] void __llvm_libc_exit(int status) {
  (void)status;
  for (;;)
    __asm__ volatile("" ::: "memory");
}

} // extern "C"
