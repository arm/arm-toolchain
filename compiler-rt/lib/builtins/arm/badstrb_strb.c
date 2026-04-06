//===----- lib/arm/badstrb_strb.c - Perform 16-bit store in-place of 8-bit stores ----*- C -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------------------------===//

#include <stdint.h>

void __attribute__((weak)) __attribute__((visibility("hidden"))) __attribute__((noinline))
__badstrb_strb(uint8_t *address, uint8_t value) {
  const uintptr_t address_i = ((uintptr_t)address);
  uint16_t *aligned_address = (uint16_t *)(address_i & ~(0x1));

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  const uint16_t shift = (address_i & 0x1) * 8;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  const uint16_t shift = (!(address_i & 0x1)) * 8;
#else
  #error "Invalid endianness"
#endif

  const uint16_t value_mask = (uint16_t)value << shift;
  const uint16_t clear_mask = 0xFF00 >> shift;

  // Disable interupts for make this operation atomic
  __asm__ __volatile__("CPSID I");

  const uint16_t old_value = *aligned_address;
  *aligned_address = (old_value & clear_mask) | value_mask;

  // Re-enable interrupts after updating the value
  __asm__ __volatile__("CPSIE I");
}
