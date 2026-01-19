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

// Define architecture specific idle mode
#if defined(__aarch64__)
static inline void mask_irqs(void) {
  __asm__ volatile("msr daifset, #0xf \n isb \n" ::: "memory");
}
#elif defined(__arm__) && (__ARM_ARCH_PROFILE == 'M')
static inline void mask_irqs(void) {
  __asm__ volatile("cpsid i \n isb \n" ::: "memory");
}
#elif defined(__arm__)
static inline void mask_irqs(void) {
  __asm__ volatile("cpsid if \n isb \n" ::: "memory");
}
#else
#error "Unsupported architecture"
#endif

static inline void idle_once(void) { __asm__ volatile("wfi" ::: "memory"); }

// Go into low power infinite loop on exit
[[gnu::weak, noreturn]] void __llvm_libc_exit(int status) {
  (void)status;
  mask_irqs();
  for (;;)
    idle_once();
}

} // extern "C"
