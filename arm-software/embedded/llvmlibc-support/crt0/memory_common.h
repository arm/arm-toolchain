//
// Copyright (c) 2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// Memory-related definitions common to both M-profile and A-profile

#ifndef BOOTCODE_MEMORY_COMMON_H
#define BOOTCODE_MEMORY_COMMON_H

extern char __stack __attribute__((weak));
extern char __heap_start __attribute__((weak));

namespace bootcode {
namespace memory {

unsigned long get_stackheap_start() {
  unsigned long val;

  if ((val = (unsigned long)&__heap_start)) {
    return val;
  }

  // Place stackheap in the page after this one (assuming 1Gb pages)
  unsigned long page = ((unsigned long)get_stackheap_start) >> 30;
  return (page + 1) << 30;
}

unsigned long get_stackheap_end() {
  unsigned long val;

  if ((val = (unsigned long)&__stack)) {
    return val;
  }

  // Place stackheap in the page after this one (assuming 1Gb pages)
  unsigned long page = ((unsigned long)get_stackheap_end) >> 30;
  return (page + 2) << 30;
}

} // namespace memory
} // namespace bootcode

#endif // BOOTCODE_MEMORY_COMMON_H
