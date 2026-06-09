/* Copyright (c) 2026, Arm Limited and affiliates.
 *
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define KASAN_MAX_POISONED_RANGES 256

struct poisoned_range {
  uintptr_t begin;
  uintptr_t end;
};

static struct poisoned_range poisoned_ranges[KASAN_MAX_POISONED_RANGES];
static unsigned poisoned_range_count;

/** @name Sample poisoning helpers
 * These helpers are sample APIs, not Clang ASan/KASan ABI entry points.
 * They track poisoned address ranges in a fixed table.
 * @{ */

/**
 * @brief Mark an address range as poisoned.
 * @param addr Start of the range to poison.
 * @param size Number of bytes to poison.
 */
void kasan_poison(void *addr, size_t size) {
  if (size == 0)
    return;

  if (poisoned_range_count ==
      sizeof(poisoned_ranges) / sizeof(poisoned_ranges[0]))
    abort();

  uintptr_t begin = (uintptr_t)addr;
  poisoned_ranges[poisoned_range_count].begin = begin;
  poisoned_ranges[poisoned_range_count].end = begin + size;
  ++poisoned_range_count;
}

/**
 * @brief Remove poisoning from an address range.
 * @param addr Start of the range to unpoison.
 * @param size Number of bytes to unpoison.
 */
void kasan_unpoison(void *addr, size_t size) {
  uintptr_t begin = (uintptr_t)addr;
  uintptr_t end = begin + size;

  for (unsigned i = 0; i < poisoned_range_count;) {
    uintptr_t range_begin = poisoned_ranges[i].begin;
    uintptr_t range_end = poisoned_ranges[i].end;

    if (end <= range_begin || begin >= range_end) {
      ++i;
      continue;
    }

    if (begin <= range_begin && end >= range_end) {
      poisoned_ranges[i] = poisoned_ranges[poisoned_range_count - 1];
      --poisoned_range_count;
      continue;
    }

    if (begin <= range_begin) {
      poisoned_ranges[i].begin = end;
      ++i;
      continue;
    }

    if (end >= range_end) {
      poisoned_ranges[i].end = begin;
      ++i;
      continue;
    }

    if (poisoned_range_count ==
        sizeof(poisoned_ranges) / sizeof(poisoned_ranges[0]))
      abort();

    poisoned_ranges[i].end = begin;
    poisoned_ranges[poisoned_range_count].begin = end;
    poisoned_ranges[poisoned_range_count].end = range_end;
    ++poisoned_range_count;
    ++i;
  }
}

/** @} */

/**
 * @brief Return non-zero if an access range overlaps a poisoned range.
 * @param addr First byte accessed by instrumented code.
 * @param size Number of bytes accessed.
 */
static int range_intersects_poison(uintptr_t addr, uintptr_t size) {
  uintptr_t end = addr + size;

  for (unsigned i = 0; i < poisoned_range_count; ++i) {
    if (addr < poisoned_ranges[i].end && end > poisoned_ranges[i].begin)
      return 1;
  }

  return 0;
}

/**
 * @brief Print a KASan access report and stop the program.
 * @param kind Access kind, such as "load" or "store".
 * @param addr Address reported by the instrumentation callback.
 * @param size Access size in bytes.
 */
static void report_access(const char *kind, uintptr_t addr, uintptr_t size) {
  printf("KASAN: invalid %s at 0x%08lx, size %lu\n", kind, (unsigned long)addr,
         (unsigned long)size);
  abort();
}

/**
 * @brief Print an allocator misuse report and stop the program.
 * @param kind Allocator error kind, such as "double free".
 * @param ptr Pointer passed to the allocator wrapper.
 */
static void report_alloc_error(const char *kind, const void *ptr) {
  printf("KASAN: invalid %s of 0x%08lx\n", kind, (unsigned long)(uintptr_t)ptr);
  abort();
}

/**
 * @brief Shared implementation for Clang's outlined access callbacks.
 * @param kind Access kind, such as "load" or "store".
 * @param addr Address reported by the instrumentation callback.
 * @param size Access size in bytes.
 */
static void check_access(const char *kind, uintptr_t addr, uintptr_t size) {
  if (range_intersects_poison(addr, size))
    report_access(kind, addr, size);
}

#ifdef KASAN_WRAP_ALLOC

/** @name LLVM libc allocator wrappers
 * These functions are reached through linker --wrap options. They are not
 * Clang ASan/KASan ABI entry points.
 * @{ */

#define KASAN_HEAP_MAGIC 0x4b41534eU
#define KASAN_HEAP_FREED_MAGIC 0x4b415346U
#define KASAN_HEAP_REDZONE_SIZE 16U

struct heap_header {
  uint32_t magic;
  size_t user_size;
  size_t total_size;
  size_t payload_offset;
};

void *__real_malloc(size_t size);
void *__real_calloc(size_t nmemb, size_t size);
void *__real_realloc(void *ptr, size_t size);
void __real_free(void *ptr);

void *__wrap_malloc(size_t size);
void *__wrap_calloc(size_t nmemb, size_t size);
void *__wrap_realloc(void *ptr, size_t size);
void __wrap_free(void *ptr);

/**
 * @brief Round a size up to an allocator metadata alignment.
 * @param value Value to align.
 * @param alignment Power-of-two alignment.
 */
static size_t align_up(size_t value, size_t alignment) {
  return (value + alignment - 1U) & ~(alignment - 1U);
}

/**
 * @brief Recover and validate the heap metadata for a user pointer.
 * @param ptr Pointer passed to a wrapped allocator function.
 * @return Heap header that owns the user pointer.
 */
static struct heap_header *header_from_payload(void *ptr) {
  uintptr_t payload = (uintptr_t)ptr;
  struct heap_header *header =
      (struct heap_header *)(payload - align_up(sizeof(struct heap_header) +
                                                    KASAN_HEAP_REDZONE_SIZE,
                                                sizeof(max_align_t)));

  if (header->magic != KASAN_HEAP_MAGIC &&
      header->magic != KASAN_HEAP_FREED_MAGIC)
    report_alloc_error("free", ptr);

  return header;
}

/**
 * @brief Allocate a block with poisoned heap redzones.
 * @param size User payload size in bytes.
 * @return Pointer to the unpoisoned user payload, or NULL on allocation failure.
 */
void *__wrap_malloc(size_t size) {
  size_t payload_offset =
      align_up(sizeof(struct heap_header) + KASAN_HEAP_REDZONE_SIZE,
               sizeof(max_align_t));
  size_t total_size = payload_offset + size + KASAN_HEAP_REDZONE_SIZE;
  uint8_t *base = __real_malloc(total_size);

  if (!base)
    return NULL;

  struct heap_header *header = (struct heap_header *)base;
  uint8_t *payload = base + payload_offset;

  header->magic = KASAN_HEAP_MAGIC;
  header->user_size = size;
  header->total_size = total_size;
  header->payload_offset = payload_offset;

  kasan_unpoison(base, total_size);
  kasan_poison(base + sizeof(struct heap_header),
               payload_offset - sizeof(struct heap_header));
  kasan_poison(payload + size, KASAN_HEAP_REDZONE_SIZE);

  return payload;
}

/**
 * @brief Allocate and zero a block with poisoned heap redzones.
 * @param nmemb Number of elements.
 * @param size Size of each element in bytes.
 * @return Pointer to the zeroed user payload, or NULL on allocation failure.
 */
void *__wrap_calloc(size_t nmemb, size_t size) {
  if (size != 0 && nmemb > SIZE_MAX / size)
    return NULL;

  size_t total = nmemb * size;
  uint8_t *payload = __wrap_malloc(total);

  if (!payload)
    return NULL;

  for (size_t i = 0; i < total; ++i)
    payload[i] = 0;

  return payload;
}

/**
 * @brief Allocate a resized block, copy old contents, and poison the old block.
 * @param ptr Previous user pointer, or NULL.
 * @param size New user payload size in bytes.
 * @return Pointer to the resized user payload, or NULL on allocation failure.
 */
void *__wrap_realloc(void *ptr, size_t size) {
  if (!ptr)
    return __wrap_malloc(size);

  if (size == 0) {
    __wrap_free(ptr);
    return NULL;
  }

  struct heap_header *old_header = header_from_payload(ptr);
  if (old_header->magic == KASAN_HEAP_FREED_MAGIC)
    report_alloc_error("realloc of freed allocation", ptr);

  uint8_t *new_payload = __wrap_malloc(size);
  if (!new_payload)
    return NULL;

  size_t copy_size =
      old_header->user_size < size ? old_header->user_size : size;
  uint8_t *old_payload = ptr;
  for (size_t i = 0; i < copy_size; ++i)
    new_payload[i] = old_payload[i];

  __wrap_free(ptr);
  return new_payload;
}

/**
 * @brief Poison a freed block and keep it quarantined for this demo.
 * @param ptr User pointer to free, or NULL.
 */
void __wrap_free(void *ptr) {
  if (!ptr)
    return;

  struct heap_header *header = header_from_payload(ptr);
  if (header->magic == KASAN_HEAP_FREED_MAGIC)
    report_alloc_error("double free", ptr);

  uint8_t *base = (uint8_t *)header;
  header->magic = KASAN_HEAP_FREED_MAGIC;

  kasan_unpoison(base, header->total_size);
  kasan_poison(base, header->total_size);

  /* Keep freed blocks quarantined so use-after-free remains detectable. */
  (void)__real_calloc;
  (void)__real_realloc;
  (void)__real_free;
}

/** @} */

#endif

/** @name Clang ASan/KASan ABI callbacks
 * Clang emits calls to these symbols when outlined instrumentation is enabled.
 * The _noabort variants are provided because Clang may reference them for
 * recoverable instrumentation modes.
 * @{ */

/**
 * @brief Generate load/store access-check callbacks for one access kind.
 * @param kind Callback kind token: load or store.
 */
#define DEFINE_ACCESS_CALLBACKS(kind)                                          \
  void __asan_##kind##1(uintptr_t addr) { check_access(#kind, addr, 1); }      \
  void __asan_##kind##2(uintptr_t addr) { check_access(#kind, addr, 2); }      \
  void __asan_##kind##4(uintptr_t addr) { check_access(#kind, addr, 4); }      \
  void __asan_##kind##8(uintptr_t addr) { check_access(#kind, addr, 8); }      \
  void __asan_##kind##16(uintptr_t addr) { check_access(#kind, addr, 16); }    \
  void __asan_##kind##N(uintptr_t addr, uintptr_t size) {                      \
    check_access(#kind, addr, size);                                           \
  }                                                                            \
  void __asan_##kind##1_noabort(uintptr_t addr) {                              \
    check_access(#kind, addr, 1);                                              \
  }                                                                            \
  void __asan_##kind##2_noabort(uintptr_t addr) {                              \
    check_access(#kind, addr, 2);                                              \
  }                                                                            \
  void __asan_##kind##4_noabort(uintptr_t addr) {                              \
    check_access(#kind, addr, 4);                                              \
  }                                                                            \
  void __asan_##kind##8_noabort(uintptr_t addr) {                              \
    check_access(#kind, addr, 8);                                              \
  }                                                                            \
  void __asan_##kind##16_noabort(uintptr_t addr) {                             \
    check_access(#kind, addr, 16);                                             \
  }                                                                            \
  void __asan_##kind##N_noabort(uintptr_t addr, uintptr_t size) {              \
    check_access(#kind, addr, size);                                           \
  }

DEFINE_ACCESS_CALLBACKS(load)
DEFINE_ACCESS_CALLBACKS(store)

/** @brief Called by Clang before code paths that do not return. */
void __asan_handle_no_return(void) {}

/** @brief ASan runtime initialization hook emitted by Clang. */
void __asan_init(void) {}

/** @brief Version check hook expected by some ASan-instrumented objects. */
void __asan_version_mismatch_check(void) {}

/**
 * @brief Clang ASan global descriptor layout used by __asan_register_globals.
 */
struct asan_global {
  uintptr_t beg;
  uintptr_t size;
  uintptr_t size_with_redzone;
  uintptr_t name;
  uintptr_t module_name;
  uintptr_t has_dynamic_init;
  uintptr_t source_location;
  uintptr_t odr_indicator;
};

/**
 * @brief Register globals and poison their trailing redzones.
 * @param globals Pointer to an array of Clang ASan global descriptors.
 * @param n Number of descriptors in the array.
 */
void __asan_register_globals(uintptr_t globals, uintptr_t n) {
  struct asan_global *global = (struct asan_global *)globals;

  for (uintptr_t i = 0; i < n; ++i) {
    uintptr_t beg = global[i].beg;
    uintptr_t size = global[i].size;
    uintptr_t size_with_redzone = global[i].size_with_redzone;

    (void)global[i].name;
    (void)global[i].module_name;
    (void)global[i].has_dynamic_init;
    (void)global[i].source_location;
    (void)global[i].odr_indicator;

    if (size_with_redzone <= size)
      continue;

    kasan_unpoison((void *)beg, size);
    kasan_poison((void *)(beg + size), size_with_redzone - size);
  }
}

/**
 * @brief Unpoison global ranges when Clang unregisters global descriptors.
 * @param globals Pointer to an array of Clang ASan global descriptors.
 * @param n Number of descriptors in the array.
 */
void __asan_unregister_globals(uintptr_t globals, uintptr_t n) {
  struct asan_global *global = (struct asan_global *)globals;

  for (uintptr_t i = 0; i < n; ++i)
    kasan_unpoison((void *)global[i].beg, global[i].size_with_redzone);
}

/** @} */
