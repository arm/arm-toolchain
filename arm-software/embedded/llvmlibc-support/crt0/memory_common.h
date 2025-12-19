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

extern "C" [[gnu::weak]] extern char __stack;
[[gnu::weak]] extern char __heap_start;

namespace bootcode {
namespace memory {

unsigned long get_stackheap_start() {
  unsigned long val;

  if ((val = reinterpret_cast<unsigned long>(&__heap_start))) {
    return val;
  }

  // Place stackheap in the page after this one (assuming 1Gb pages)
  unsigned long page =
      (reinterpret_cast<unsigned long>(&get_stackheap_start)) >> 30;
  return (page + 1) << 30;
}

unsigned long get_stackheap_end() {
  unsigned long val;

  if ((val = reinterpret_cast<unsigned long>(&__stack))) {
    return val;
  }

  // Place stackheap in the page after this one (assuming 1Gb pages)
  unsigned long page =
      (reinterpret_cast<unsigned long>(&get_stackheap_end)) >> 30;
  return (page + 2) << 30;
}

} // namespace memory
} // namespace bootcode

#endif // BOOTCODE_MEMORY_COMMON_H
