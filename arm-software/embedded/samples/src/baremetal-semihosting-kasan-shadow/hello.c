/* Copyright (c) 2026, Arm Limited and affiliates.
 *
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef KASAN_TEST
#define KASAN_TEST 1
#endif

static volatile unsigned out_of_bounds_index = 16;

__attribute__((noinline)) static void stack_overflow_test(void) {
  uint8_t buffer[16] = {0};

  puts("Writing one byte past a 16-byte stack buffer");
  volatile uint8_t *access = buffer;
  access[out_of_bounds_index] = 0xff;
}

__attribute__((noinline)) static void heap_overflow_test(void) {
  uint8_t *buffer = malloc(16);
  if (!buffer) {
    puts("malloc failed");
    abort();
  }

  puts("Writing one byte past a 16-byte malloc allocation");
  volatile uint8_t *access = buffer;
  access[out_of_bounds_index] = 0xff;
  free(buffer);
}

__attribute__((noinline)) static void heap_use_after_free_test(void) {
  uint8_t *buffer = calloc(16, 1);
  if (!buffer) {
    puts("calloc failed");
    abort();
  }

  puts("Freeing a 16-byte allocation");
  free(buffer);

  puts("Writing to the freed allocation");
  volatile uint8_t *access = buffer;
  access[0] = 0xff;
}

int main(void) {
  puts("KASan shadow-memory sample");

#if KASAN_TEST == 1
  puts("Test 1: automatic stack redzone");
  stack_overflow_test();
#elif KASAN_TEST == 2
  puts("Test 2: LLVM libc heap buffer overflow");
  heap_overflow_test();
#elif KASAN_TEST == 3
  puts("Test 3: LLVM libc heap use-after-free");
  heap_use_after_free_test();
#else
#error "Unsupported KASAN_TEST value"
#endif

  puts("This line is not expected to be reached");
  return 0;
}
