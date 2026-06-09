//
// Copyright (c) 2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#include <stdint.h>
#include <stdlib.h> // for exit()

#include "exceptions_common.h"
#include "platform.h"

#if __ARM_ARCH_PROFILE == 'A' || __ARM_ARCH_PROFILE == 'R'
#include "system_registers_a.h"
#elif __ARM_ARCH_PROFILE == 'M'
#include "system_registers_m.h"
#else
// ARMv4T
// TODO: fill in stub functions once we can start testing LLVM-libc
#endif

int main(int argc, const char **argv);
extern "C" void __libc_init_array();

[[gnu::weak]] extern char __stack;

using namespace bootcode::exceptions;

namespace {
#ifdef __ARM_FEATURE_PAUTH
// Disable pointer authentication, as it isn't enabled until misc::setup meaning
// the PAC at the beginning would do nothing so the AUT at the end would fail.
[[gnu::target("branch-protection=none")]]
#endif
void do_start() {
  _platform_setup_exceptions();
  _platform_setup_memory();
  _platform_setup_arch_extensions();

  _platform_init_data_segments();
  _platform_init_tls();

  __libc_init_array();
  _platform_init();

  // Provide command line options (argc/argv) for the main function
  char *cmdline = nullptr;
  int max_cmdline = 256; // Arm semihosting requires at least 80 bytes
                         // There is no API to get the actual size of the line
  const char **argv = nullptr;
  int max_argv = 0;
  int argc = 0;
  int status = 1; // Report error on early returns

  // Allocate the buffer for the command line
  cmdline = static_cast<char *>(malloc(static_cast<size_t>(max_cmdline)));
  if (!cmdline) {
    print_str("ERROR: libc cannot allocate memory for command line options.\n");
    goto free_and_exit;
  }

  // Probe for number of arguments and allocate the buffer for argv[]
  max_argv = _platform_get_argv(cmdline, max_cmdline, nullptr, 0) + 1;
  if (max_argv <= 0) {
    print_str(
        "ERROR: _platform_get_argv failed, command line may be too long.\n");
    goto free_and_exit;
  }
  argv = static_cast<const char **>(
      malloc(sizeof(*argv) * static_cast<size_t>(max_argv)));
  if (!argv) {
    print_str("ERROR: libc cannot allocate memory for argv[].\n");
    goto free_and_exit;
  }

  // Finally, parse the comand line into argc/argv[]
  argc = _platform_get_argv(cmdline, max_cmdline, argv, max_argv);
  if (argc < 0) {
    print_str("ERROR: libc cannot parse the command line.\n");
    goto free_and_exit;
  }

  status = main(argc, argv);

free_and_exit:
  free(cmdline);
  free(argv);
  exit(status);
}
} // namespace

extern "C" {
// The entry point sets sp and branches to the main startup function.
#ifdef __ARM_ARCH_ISA_ARM
// If the target supports the A32 instruction set then the entry point has
// to use it.
[[gnu::target("arm")]]
#elif defined(__ARM_ARCH_ISA_A64)
// QEMU (AArch64) will jump to the first section when running from a .bin/.hex
// file after boot. Therefore, place the entrypoint there.
[[gnu::section(".text.init.enter")]]
#endif
[[gnu::naked]] void _start() {
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
  asm("bl %0" : : "X"(::do_start));
}
} // extern "C"
