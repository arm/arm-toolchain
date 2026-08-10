/* Copyright (c) 2026, Arm Limited and affiliates.
 *
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "kasan_shadow_runtime.h"

// Override the default aborting report hooks so this sample can recover and
// continue through all demo tests in one run.

extern "C" __attribute__((no_sanitize("kernel-address"))) void
kasan_rt_report_access(const char *kind, uintptr_t addr, size_t size,
                       size_t offset) {
  kasan_rt_puts("KASAN: invalid ");
  kasan_rt_puts(kind);
  kasan_rt_puts(" at ");
  kasan_rt_putaddr(addr);
  kasan_rt_puts(", size ");
  kasan_rt_putsize(size);
  kasan_rt_puts(", offset ");
  kasan_rt_putsize(offset);
  kasan_rt_puts(" (recovered)\n");
}

extern "C" __attribute__((no_sanitize("kernel-address"))) void
kasan_rt_report_alloc_error(const char *kind, const void *ptr) {
  kasan_rt_puts("KASAN: invalid ");
  kasan_rt_puts(kind);
  kasan_rt_puts(" of ");
  kasan_rt_putaddr(reinterpret_cast<uintptr_t>(ptr));
  kasan_rt_puts(" (recovered)\n");
}

// Define a test for each type of error

static constexpr size_t buffer_size = 16;
static volatile unsigned out_of_bounds_index = buffer_size;

__attribute__((noinline)) static void test_stack_overflow(void) {
  uint8_t buffer[buffer_size] = {0};

  puts("Writing one byte past a stack buffer");
  volatile uint8_t *access = buffer;
  access[out_of_bounds_index] = 0xff;
}

__attribute__((noinline)) static void test_heap_overflow(void) {
  uint8_t *buffer = static_cast<uint8_t *>(malloc(buffer_size));
  if (!buffer)
    abort();

  puts("Writing one byte past a malloc allocation");
  volatile uint8_t *access = buffer;
  access[out_of_bounds_index] = 0xff;
}

__attribute__((noinline)) static void test_heap_use_after_free(void) {
  uint8_t *buffer = static_cast<uint8_t *>(calloc(buffer_size, 1));
  if (!buffer)
    abort();

  puts("Freeing an allocation");
  free(buffer);

  puts("Attempting use-after-free");
  volatile uint8_t *access = buffer;
  access[0] = 0xff;
}

__attribute__((noinline)) static void test_heap_realloc_overflow(void) {
  uint8_t *buffer = static_cast<uint8_t *>(malloc(buffer_size));
  if (!buffer)
    abort();

  buffer = static_cast<uint8_t *>(realloc(buffer, buffer_size / 2));
  if (!buffer)
    abort();

  puts("Writing one byte past a resized malloc allocation");
  volatile uint8_t *access = buffer;
  access[buffer_size / 2] = 0xff;
}

int main(void) {
  puts("C++ KASan shadow-memory sample");

  puts("Test 1: automatic stack redzone");
  test_stack_overflow();

  puts("Test 2: heap buffer overflow");
  test_heap_overflow();

  puts("Test 3: heap use-after-free");
  test_heap_use_after_free();

  puts("Test 4: realloc buffer overflow");
  test_heap_realloc_overflow();

  puts("All KASan tests completed");
  return 0;
}
