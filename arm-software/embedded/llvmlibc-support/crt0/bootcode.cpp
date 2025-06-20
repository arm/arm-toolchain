//
// Copyright (c) 2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#include <stdint.h>

#if __ARM_ARCH_PROFILE == 'A' || __ARM_ARCH_PROFILE == 'R'
#include "exceptions_a.h"
#include "memory_a.h"
#include "misc_a.h"
#include "system_registers_a.h"
#elif __ARM_ARCH_PROFILE == 'M'
#include "exceptions_m.h"
#include "memory_m.h"
#include "misc_m.h"
#include "system_registers_m.h"
#endif

using namespace bootcode;

int main(int argc, const char **argv);
extern "C" void __libc_init_array();
extern "C" void _platform_init();

// Required for exit() on baremetal
// TODO: This is temporary, remove once implemented upstream
extern "C" [[gnu::weak]] void __cxa_finalize(void *) {}

extern char __data_source[];
extern char __data_start[];
extern char __data_size[];
extern char __bss_start[];
extern char __bss_size[];

#ifdef __ARM_FEATURE_PAUTH
// Disable pointer authentication, as it isn't enabled until misc::setup meaning
// the PAC at the beginning would do nothing so the AUT at the end would fail.
__attribute__((target("branch-protection=none")))
#endif
extern "C" void __startup() {
  exceptions::setup();
  memory::setup();
  misc::setup();

  // Perform the equivalent of scatterloading
  memcpy(__data_start, __data_source, (uintptr_t)__data_size);
  memset(__bss_start, '\0', (uintptr_t)__bss_size);

  memory::enable_cache();
  __libc_init_array();
  _platform_init();
  exit(main(0, 0));
}

// The entry point sets sp and branches to the main startup function.
#ifdef __ARM_ARCH_ISA_ARM
// If the target supports the A32 instruction set then the entry point has to
// use it.
__attribute__((target("arm")))
#endif
__attribute__((naked)) extern "C" void _start() {
#if __ARM_ARCH_PROFILE != 'M' && __ARM_ARCH >= 8 && !defined(__ARM_ARCH_ISA_A64)
  // Check if we're in hypervisor mode
  asm("mrs r0, CPSR\n\t"
      "and r0, r0, #0x1f\n\t"
      "cmp r0, #0x1a\n\t"
      "bne after_mode_switch"
      :
      :
      : "r0");
  // If so switch to svc mode, which is what we would have started execution in
  // if we didn't have hypervisor mode.
  asm("adr r0, after_mode_switch\n\t"
      "msr ELR_hyp, r0"
      :
      :
      : "r0");
  asm("msr SPSR_hyp, %0" : : "r"(0x13));
  asm("eret");
  asm("after_mode_switch:");
#endif

  // Configured through linker script defined symbols
  asm("mov sp, %0" : : "r"(&__stack));
  asm("bl %0" : : "X"(__startup));
}
