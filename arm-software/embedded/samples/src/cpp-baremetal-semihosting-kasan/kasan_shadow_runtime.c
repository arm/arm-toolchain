/* Copyright (c) 2026, Arm Limited and affiliates.
 *
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define KASAN_SHADOW_SCALE 3U
#define KASAN_SHADOW_GRANULE_SIZE (1U << KASAN_SHADOW_SCALE)
#define KASAN_SHADOW_OFFSET 0x1c003800U

#define KASAN_USER_POISON 0xf7U
#define KASAN_HEAP_LEFT_REDZONE 0xfaU
#define KASAN_HEAP_FREED 0xfdU
#define KASAN_GLOBAL_REDZONE 0xf9U

extern uint8_t __kasan_shadow_start[];
extern uint8_t __kasan_shadow_end[];

/** @name Shadow-memory helpers
 * These are sample-internal helpers for the linker-defined shadow region.
 * They are not Clang ASan/KASan ABI entry points.
 * @{ */

/**
 * @brief Convert an application address to its ASan shadow address.
 * @param addr Application address.
 * @return Shadow-memory address for the application address.
 */
static uintptr_t shadow_addr(uintptr_t addr) {
  return (addr >> KASAN_SHADOW_SCALE) + KASAN_SHADOW_OFFSET;
}

/**
 * @brief Convert an application address to a shadow byte pointer.
 * @param addr Application address.
 * @return Pointer to the corresponding shadow byte.
 */
static uint8_t *shadow_for(uintptr_t addr) {
  return (uint8_t *)shadow_addr(addr);
}

/**
 * @brief Return non-zero if a shadow pointer is inside reserved shadow RAM.
 * @param shadow Shadow byte pointer to test.
 */
static int shadow_in_range(uint8_t *shadow) {
  return shadow >= __kasan_shadow_start && shadow < __kasan_shadow_end;
}

/** @brief Clear the reserved shadow RAM so all covered memory starts valid. */
static void clear_shadow(void) {
  for (uint8_t *shadow = __kasan_shadow_start; shadow < __kasan_shadow_end;
       ++shadow)
    *shadow = 0;
}

/** @brief Pre-initialization hook that clears shadow before constructors run. */
static void kasan_preinit(void) { clear_shadow(); }

/** @brief Register the sample shadow clear hook in the C preinit array. */
void (*const __kasan_preinit)(void) __attribute__((section(".preinit_array"),
                                                  used)) = kasan_preinit;

/** @} */

/** @name Retargettable report output hooks
 * Users can provide non-weak definitions of these functions to route reports to
 * UART, ITM/SWO, retained RAM, or another target-specific sink.
 * @{ */

/**
 * @brief Output one report character.
 * @param c Character to output.
 */
__attribute__((weak)) void kasan_rt_putc(char c) { putc(c, stdout); }

/**
 * @brief Output a NUL-terminated report string.
 * @param s String to output.
 */
__attribute__((weak)) void kasan_rt_puts(const char *s) {
  while (*s)
    kasan_rt_putc(*s++);
}

/**
 * @brief Output an address-sized value in fixed-width hexadecimal.
 * @param addr Value to output.
 */
__attribute__((weak)) void kasan_rt_putaddr(uintptr_t addr) {
  static const char hex[] = "0123456789abcdef";

  kasan_rt_puts("0x");
  for (int shift = (int)(sizeof(uintptr_t) * 8U) - 4; shift >= 0;
       shift -= 4)
    kasan_rt_putc(hex[(addr >> (unsigned)shift) & 0xfU]);
}

/** @} */

/** @name Retargettable report handlers
 * Users can provide non-weak definitions of these functions to change whether
 * reports abort or recover. Returning from a handler lets the invalid access
 * continue, which is useful for diagnostics but unsafe for production.
 * @{ */

/**
 * @brief Report a poisoned memory access and stop the program by default.
 * @param kind Access kind, such as "load" or "store".
 * @param addr Address reported by the instrumentation callback.
 * @param size Access size in bytes.
 */
__attribute__((weak)) void kasan_rt_report_access(const char *kind,
                                                  uintptr_t addr,
                                                  uintptr_t size) {
  kasan_rt_puts("KASAN: invalid ");
  kasan_rt_puts(kind);
  kasan_rt_puts(" at ");
  kasan_rt_putaddr(addr);
  kasan_rt_puts(", size ");
  kasan_rt_putaddr(size);
  kasan_rt_puts(" (aborted)\n");
  abort();
}

/**
 * @brief Report allocator API misuse and stop the program by default.
 * @param kind Allocator error kind, such as "double free".
 * @param ptr Pointer passed to the allocator wrapper.
 */
__attribute__((weak)) void kasan_rt_report_alloc_error(const char *kind,
                                                       const void *ptr) {
  kasan_rt_puts("KASAN: invalid ");
  kasan_rt_puts(kind);
  kasan_rt_puts(" of ");
  kasan_rt_putaddr((uintptr_t)ptr);
  kasan_rt_puts(" (aborted)\n");
  abort();
}

/** @} */

/**
 * @brief Dispatch a KASan access report to the retargettable handler.
 * @param kind Access kind, such as "load" or "store".
 * @param addr Address reported by the instrumentation callback.
 * @param size Access size in bytes.
 */
static void report_access(const char *kind, uintptr_t addr, uintptr_t size) {
  kasan_rt_report_access(kind, addr, size);
}

/**
 * @brief Dispatch allocator misuse reports to the retargettable handler.
 * @param kind Allocator error kind, such as "double free".
 * @param ptr Pointer passed to the allocator wrapper.
 */
static void report_alloc_error(const char *kind, const void *ptr) {
  kasan_rt_report_alloc_error(kind, ptr);
}

/**
 * @brief Return non-zero if an application address is poisoned in shadow memory.
 * @param addr Application address to test.
 */
static int address_is_poisoned(uintptr_t addr) {
  uint8_t *shadow = shadow_for(addr);
  if (!shadow_in_range(shadow))
    return 0;

  uint8_t value = *shadow;
  if (value == 0)
    return 0;

  if (value < KASAN_SHADOW_GRANULE_SIZE)
    return (addr & (KASAN_SHADOW_GRANULE_SIZE - 1U)) >= value;

  return 1;
}

/**
 * @brief Shared implementation for Clang's outlined access callbacks.
 * @param kind Access kind, such as "load" or "store".
 * @param addr Address reported by the instrumentation callback.
 * @param size Access size in bytes.
 */
static void check_access(const char *kind, uintptr_t addr, uintptr_t size) {
  for (uintptr_t i = 0; i < size; ++i) {
    if (address_is_poisoned(addr + i))
      report_access(kind, addr, size);
  }
}

/** @name Sample poisoning helpers
 * These helpers update shadow memory for sample code and allocator wrappers.
 * Clang stack instrumentation writes shadow bytes directly instead.
 * @{ */

/**
 * @brief Mark an address range as addressable in shadow memory.
 * @param addr Start of the range to unpoison.
 * @param size Number of bytes to unpoison.
 */
void kasan_unpoison(void *addr, size_t size) {
  if (size == 0)
    return;

  uintptr_t begin = (uintptr_t)addr;
  uintptr_t end = begin + size;
  uintptr_t shadow_begin = shadow_addr(begin);
  uintptr_t shadow_end = shadow_addr(end - 1U);

  for (uintptr_t shadow = shadow_begin; shadow <= shadow_end; ++shadow) {
    uint8_t *shadow_byte = (uint8_t *)shadow;
    if (!shadow_in_range(shadow_byte))
      continue;

    uintptr_t granule_begin = (shadow - KASAN_SHADOW_OFFSET)
                              << KASAN_SHADOW_SCALE;
    uintptr_t granule_end = granule_begin + KASAN_SHADOW_GRANULE_SIZE;

    if (begin <= granule_begin && end >= granule_end) {
      *shadow_byte = 0;
      continue;
    }

    if (begin <= granule_begin && end < granule_end)
      *shadow_byte = (uint8_t)(end - granule_begin);
  }
}

/**
 * @brief Mark an address range as poisoned in shadow memory.
 * @param addr Start of the range to poison.
 * @param size Number of bytes to poison.
 */
void kasan_poison(void *addr, size_t size) {
  if (size == 0)
    return;

  uintptr_t begin = (uintptr_t)addr;
  uintptr_t end = begin + size;
  uintptr_t shadow_begin = shadow_addr(begin);
  uintptr_t shadow_end = shadow_addr(end - 1U);

  for (uintptr_t shadow = shadow_begin; shadow <= shadow_end; ++shadow) {
    uint8_t *shadow_byte = (uint8_t *)shadow;
    if (!shadow_in_range(shadow_byte))
      continue;

    uintptr_t granule_begin = (shadow - KASAN_SHADOW_OFFSET)
                              << KASAN_SHADOW_SCALE;
    uintptr_t granule_end = granule_begin + KASAN_SHADOW_GRANULE_SIZE;

    if (begin <= granule_begin && end >= granule_end) {
      *shadow_byte = KASAN_USER_POISON;
      continue;
    }

    if (begin > granule_begin && end >= granule_end)
      *shadow_byte = (uint8_t)(begin - granule_begin);
  }
}

/**
 * @brief Fill shadow bytes for an address range with a specific poison value.
 * @param addr Start of the range to poison.
 * @param size Number of bytes to poison.
 * @param value ASan poison byte to write to shadow memory.
 */
static void kasan_poison_with_value(void *addr, size_t size, uint8_t value) {
  uintptr_t begin = (uintptr_t)addr;
  uintptr_t end = begin + size;

  if (size == 0)
    return;

  for (uintptr_t shadow = shadow_addr(begin); shadow <= shadow_addr(end - 1U);
       ++shadow) {
    uint8_t *shadow_byte = (uint8_t *)shadow;
    if (shadow_in_range(shadow_byte))
      *shadow_byte = value;
  }
}

/** @} */

/** @name Allocator wrappers
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

  kasan_poison_with_value(base, payload_offset, KASAN_HEAP_LEFT_REDZONE);
  kasan_unpoison(payload, size);
  kasan_poison_with_value(payload + size, KASAN_HEAP_REDZONE_SIZE,
                          KASAN_HEAP_LEFT_REDZONE);

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

  kasan_poison_with_value(base, header->total_size, KASAN_HEAP_FREED);

  /* Keep freed blocks quarantined so use-after-free remains detectable. */
  (void)__real_calloc;
  (void)__real_realloc;
  (void)__real_free;
}

/** @} */

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
void __asan_init(void) { clear_shadow(); }

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
    kasan_poison_with_value((void *)(beg + size), size_with_redzone - size,
                            KASAN_GLOBAL_REDZONE);
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
