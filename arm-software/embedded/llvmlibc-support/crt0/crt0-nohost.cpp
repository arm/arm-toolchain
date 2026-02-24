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

// Do nothing for debug output
[[gnu::weak]] void _platform_debug_putc(int c) { (void)c; }

// No command line options provided
[[gnu::weak]] int _platform_get_argv(char *cmdline, int max_cmdline,
                                     const char **argv, int max_argv) {
  if (cmdline && max_cmdline > 0)
    cmdline[0] = '\0';
  if (argv && max_argv > 0)
    argv[0] = nullptr;

  return 0;
}

// Go into busy infinite loop on exit
[[gnu::weak, noreturn]] void __llvm_libc_exit(int status) {
  (void)status;
  for (;;)
    __asm__ volatile("" ::: "memory");
}
} // extern "C"
