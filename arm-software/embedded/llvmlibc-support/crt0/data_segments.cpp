//
// Copyright (c) 2026, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM
// Exceptions. See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#include "platform.h"

#include <string.h> // for memcpy(), memset()

extern char __data_source[];
extern char __data_start[];
extern char __data_size[];
extern char __bss_start[];
extern char __bss_size[];

extern "C" [[gnu::weak]] void _platform_init_data_segments() {
  // Copy read-write data and clear the BSS region
  memcpy(__data_start, __data_source, reinterpret_cast<size_t>(__data_size));
  memset(__bss_start, '\0', reinterpret_cast<size_t>(__bss_size));
}
