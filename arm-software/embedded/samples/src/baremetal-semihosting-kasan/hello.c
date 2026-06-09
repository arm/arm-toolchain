/* Copyright (c) 2026, Arm Limited and affiliates.
 *
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef KASAN_TEST
#define KASAN_TEST 2
#endif

void kasan_poison(void *addr, size_t size);
void kasan_unpoison(void *addr, size_t size);

struct buffer_with_redzone {
  uint8_t data[16];
  uint8_t redzone[8];
};

static volatile unsigned out_of_bounds_index = 16;
static uint8_t global_buffer[16];

static void manual_redzone_test(void) {
  struct buffer_with_redzone buffer = {0};

  puts("Poisoning an 8-byte redzone after a 16-byte stack buffer");
  kasan_unpoison(&buffer, sizeof(buffer));
  kasan_poison(buffer.redzone, sizeof(buffer.redzone));

  puts("Writing one byte past the end of buffer.data");
  volatile uint8_t *access = buffer.data;
  access[out_of_bounds_index] = 0xff;
}

static void heap_overflow_test(void) {
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

static void global_overflow_test(void) {
  puts("Writing one byte past a 16-byte global buffer");
  volatile uint8_t *access = global_buffer;
  access[out_of_bounds_index] = 0xff;
}

static void heap_double_free_test(void) {
  uint8_t *buffer = malloc(16);
  if (!buffer) {
    puts("malloc failed");
    abort();
  }

  volatile uintptr_t saved_pointer = (uintptr_t)buffer;

  puts("Freeing a 16-byte allocation");
  free(buffer);

  puts("Freeing the same allocation again");
  free((void *)saved_pointer);
}

static void invalid_free_test(void) {
  uint8_t *buffer = malloc(16);
  if (!buffer) {
    puts("malloc failed");
    abort();
  }

  volatile uintptr_t interior_pointer = (uintptr_t)(buffer + sizeof(uint32_t));

  puts("Freeing an interior pointer that was not returned by malloc");
  free((void *)interior_pointer);
}

static void heap_use_after_free_test(void) {
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
  puts("KASan minimal runtime sample");

#if KASAN_TEST == 1
  puts("Test 1: manual stack redzone");
  manual_redzone_test();
#elif KASAN_TEST == 2
  puts("Test 2: LLVM libc heap use-after-free");
  heap_use_after_free_test();
#elif KASAN_TEST == 3
  puts("Test 3: LLVM libc heap buffer overflow");
  heap_overflow_test();
#elif KASAN_TEST == 4
  puts("Test 4: global buffer overflow");
  global_overflow_test();
#elif KASAN_TEST == 5
  puts("Test 5: LLVM libc double free");
  heap_double_free_test();
#elif KASAN_TEST == 6
  puts("Test 6: LLVM libc invalid free");
  invalid_free_test();
#else
#error "Unsupported KASAN_TEST value"
#endif

  puts("This line is not expected to be reached");
  return 0;
}
