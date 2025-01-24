// Copyright (c) 2024-2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

extern int main(int argc, char** argv);

extern void _platform_init();

extern char __data_source[];
extern char __data_start[];
extern char __data_end[];
extern char __data_size[];
extern char __bss_start[];
extern char __bss_end[];
extern char __bss_size[];
extern char __tls_base[];
extern char __tdata_end[];
extern char __tls_end[];

void _start(void) {
  memcpy(__data_start, __data_source, (size_t) __data_size);
  memset(__bss_start, 0, (size_t) __bss_size);
  _platform_init();
  _Exit(main(0, NULL));
}
