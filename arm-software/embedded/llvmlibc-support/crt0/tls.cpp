//
// Copyright (c) 2026, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM
// Exceptions. See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#include <stdint.h>

// TLS symbols defined in the linker script.
extern "C" {
[[gnu::weak]] extern char __tls_first[];
[[gnu::weak]] extern char __tls_size[];

#if defined(__ARM_ARCH_ISA_A64)
[[gnu::weak]] extern char __arm64_tls_tcb_offset[];
#elif defined(__arm__)
[[gnu::weak]] extern char __arm32_tls_tcb_offset[];
#endif
} // extern "C"

// Helper functions
namespace {
inline uintptr_t linker_value(const void *symbol) {
  return reinterpret_cast<uintptr_t>(symbol);
}
} // namespace

// _set_tls() for compatibility with picolibc and
// LLVM libc style _platform_init_tls() handler.
extern "C" {
#if defined(__ARM_ARCH_ISA_A64)
[[gnu::weak]] void _set_tls(void *tls) {
  const uintptr_t tp =
      reinterpret_cast<uintptr_t>(tls) - linker_value(__arm64_tls_tcb_offset);
  __asm__ volatile("msr tpidr_el0, %0" : : "r"(tp));
}
#elif defined(__arm__)
// Thread pointer for software TLS implementation.
static uintptr_t __llvm_libc_thread_pointer = 0;

[[gnu::weak]] void _set_tls(void *tls) {
  const uintptr_t tp =
      reinterpret_cast<uintptr_t>(tls) - linker_value(__arm32_tls_tcb_offset);
  __llvm_libc_thread_pointer = tp;
#if __ARM_ARCH_PROFILE != 'M' && __ARM_ARCH >= 6
  __asm__ volatile("mcr p15, 0, %0, c13, c0, 3" : : "r"(tp)); // TPIDRURO
#endif
}

[[gnu::weak]] void *__aeabi_read_tp(void) {
  return reinterpret_cast<void *>(__llvm_libc_thread_pointer);
}
#else
[[gnu::weak]] void _set_tls(void *tls) { (void)tls; }
#endif

[[gnu::weak]] void _platform_init_tls() {
  // Linker defined symbols for TLS must be aligned in the linker script to
  // meet the ABI requirements, see for an example:
  // https://github.com/arm/arm-toolchain/blob/arm-software/arm-software/embedded/llvmlibc-support/llvmlibc.ld.in
  if (linker_value(__tls_size) == 0 || linker_value(__tls_first) == 0)
    return;

  _set_tls(__tls_first);
}

} // extern "C"
