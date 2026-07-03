/* Copyright (c) 2026, Arm Limited and affiliates.
 *
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kasan_shadow_runtime.h"

#ifndef KASAN_SHADOW_SCALE
#error "KASAN_SHADOW_SCALE must be defined"
#endif
#ifndef KASAN_SHADOW_OFFSET
#error "KASAN_SHADOW_OFFSET must be defined"
#endif

#define KASAN_SHADOW_GRANULE_SIZE (1U << KASAN_SHADOW_SCALE)

#define KASAN_HEAP_LEFT_REDZONE 0xfaU
#define KASAN_HEAP_FREED 0xfdU
#define KASAN_GLOBAL_REDZONE 0xf9U

#ifndef KASAN_HEAP_QUARANTINE_SIZE
#define KASAN_HEAP_QUARANTINE_SIZE 4U
#endif

extern uint8_t __kasan_shadow_start[];
extern uint8_t __kasan_shadow_end[];
extern uint8_t __ram[];

/* Shadow-memory internal helpers. */

/* Convert an application address to its ASan shadow address. */
static uintptr_t shadow_addr(uintptr_t addr) {
  return (addr >> KASAN_SHADOW_SCALE) + KASAN_SHADOW_OFFSET;
}

/* Check if a shadow pointer is inside reserved shadow RAM. */
static bool shadow_in_range(uint8_t *shadow) {
  return shadow >= __kasan_shadow_start && shadow < __kasan_shadow_end;
}

/* Check if an application range is in RAM covered by this runtime's shadow. */
static bool app_range_has_shadow(uintptr_t begin, size_t size) {
  if (size == 0)
    return true;

  uintptr_t end = begin + size;
  uintptr_t app_begin = (uintptr_t)__ram;
  uintptr_t app_end = (uintptr_t)__kasan_shadow_start;

  return end >= begin && begin >= app_begin && end <= app_end;
}

/* Clear the reserved shadow RAM so all covered memory starts as valid. */
static void clear_shadow(void) {
  memset(__kasan_shadow_start, 0,
         (size_t)(__kasan_shadow_end - __kasan_shadow_start));
}

/* Register the sample shadow clear hook in the C preinit array. */
void (*const __kasan_preinit)(void) __attribute__((section(".preinit_array"),
                                                  used)) = clear_shadow;

/* Retargettable report output hooks.
 * Application can provide non-weak definitions of these functions to route
 * output to UART, ITM/SWO, retained RAM, or another target-specific sink. */

/* Output one report character. 
 * It is enough to override only kasan_rt_putc() to redirect all report output:
 * other hooks are implemented in terms of it. */
__attribute__((weak)) void kasan_rt_putc(char c) { putc(c, stdout); }

/* Output a NUL-terminated string. */
__attribute__((weak)) void kasan_rt_puts(const char *s) {
  while (*s)
    kasan_rt_putc(*s++);
}

/* Output an address-sized value in fixed-width hexadecimal. */
__attribute__((weak)) void kasan_rt_putaddr(uintptr_t addr) {
  static const char hex[] = "0123456789abcdef";

  kasan_rt_puts("0x");
  for (int shift = (int)(sizeof(uintptr_t) * 8U) - 4; shift >= 0;
       shift -= 4)
    kasan_rt_putc(hex[(addr >> (unsigned)shift) & 0xfU]);
}

/* Output a size value in fixed-width hexadecimal. */
__attribute__((weak)) void kasan_rt_putsize(size_t size) {
  kasan_rt_putaddr((uintptr_t)size);
}

/* Retargettable report handlers.
 * Application can provide non-weak definitions of these functions to change
 * whether reports abort or recover. 
 * Returning from a handler lets the invalid access continue (recover),
 * which is useful for diagnostics but unsafe for production. */

/* Report a poisoned memory access and abort the program by default.
 * @param kind Access kind, such as "load" or "store".
 * @param addr Address reported by the instrumentation callback.
 * @param size Access size in bytes.
 * @param offset Offset of the first poisoned byte within the access.
 */
__attribute__((weak)) void kasan_rt_report_access(const char *kind,
                                                  uintptr_t addr,
                                                  size_t size,
                                                  size_t offset) {
  kasan_rt_puts("KASAN: invalid ");
  kasan_rt_puts(kind);
  kasan_rt_puts(" at ");
  kasan_rt_putaddr(addr);
  kasan_rt_puts(", size ");
  kasan_rt_putsize(size);
  kasan_rt_puts(", offset ");
  kasan_rt_putsize(offset);
  kasan_rt_puts(" (aborted)\n");
  abort();
}

/* Report allocator API misuse and abort the program by default.
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

/* Report an invalid shadow-memory update and abort the program by default.
 * @param kind Shadow update kind, such as "poison" or "unpoison".
 * @param addr Start of the unsupported address range.
 * @param size Size of the unsupported address range in bytes.
 */
__attribute__((weak)) void
kasan_rt_report_shadow_update_error(const char *kind, const void *addr,
                                    size_t size) {
  kasan_rt_puts("KASAN: invalid ");
  kasan_rt_puts(kind);
  kasan_rt_puts(" range at ");
  kasan_rt_putaddr((uintptr_t)addr);
  kasan_rt_puts(", size ");
  kasan_rt_putsize(size);
  kasan_rt_puts(" (aborted)\n");
  abort();
}

/* Check if an application address is poisoned in shadow memory. */
static bool address_is_poisoned(uintptr_t addr) {
  uint8_t *shadow = (uint8_t *)shadow_addr(addr);
  if (!shadow_in_range(shadow))
    return false;

  uint8_t value = *shadow;
  if (value == 0)
    return false;

  /* Shadow byte encoding:
   *   0      all bytes in this granule are valid
   *   1..N   only the first N bytes in this granule are valid
   *   other  granule is poisoned
   */
  if (value < KASAN_SHADOW_GRANULE_SIZE)
    return (addr & (KASAN_SHADOW_GRANULE_SIZE - 1U)) >= value;

  return true;
}

/* Shared implementation for Clang's outlined access callbacks.
 * @param kind Access kind, such as "load" or "store".
 * @param addr Address reported by the instrumentation callback.
 * @param size Access size in bytes.
 */
static void check_access(const char *kind, uintptr_t addr, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    if (address_is_poisoned(addr + i))
      kasan_rt_report_access(kind, addr, size, i);
  }
}

/* Poisoning helpers to update shadow memory for allocator wrappers.
 * Clang stack instrumentation writes shadow bytes directly instead. */

/* Mark an address range as valid (unpoisoned) in shadow memory.
 * @param addr Start of the range to unpoison.
 * @param size Number of bytes to unpoison.
 *
 * The range must be wholly inside the non-shadow application RAM covered by
 * this runtime. Unsupported ranges report an error and abort.
 */
void kasan_unpoison(void *addr, size_t size) {
  if (size == 0)
    return;

  uintptr_t begin = (uintptr_t)addr;
  if (!app_range_has_shadow(begin, size))
    kasan_rt_report_shadow_update_error("unpoison", addr, size);

  uintptr_t end = begin + size;
  uintptr_t shadow_begin = shadow_addr(begin);
  uintptr_t shadow_end = shadow_addr(end - 1U);

  for (uintptr_t shadow = shadow_begin; shadow <= shadow_end; ++shadow) {
    uint8_t *shadow_byte = (uint8_t *)shadow;

    /* Each shadow byte describes one application granule:
     *   0      all bytes in this granule are valid
     *   1..N   only the first N bytes in this granule are valid
     *   other  granule is poisoned
     *
     * Reverse the shadow mapping to find the application byte range controlled
     * by this shadow byte, so partial final granules can be encoded as "first
     * N bytes valid".
     */
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

/* Fill shadow bytes for an address range with a specific poison value.
 * @param addr Start of the range to poison.
 * @param size Number of bytes to poison.
 * @param value ASan poison byte to write to shadow memory.
 */
static void kasan_poison(void *addr, size_t size, uint8_t value) {
  if (size == 0)
    return;

  uintptr_t begin = (uintptr_t)addr;
  if (!app_range_has_shadow(begin, size))
    kasan_rt_report_shadow_update_error("poison", addr, size);

  uintptr_t end = begin + size;
  for (uintptr_t shadow = shadow_addr(begin); shadow <= shadow_addr(end - 1U);
       ++shadow) {
    uint8_t *shadow_byte = (uint8_t *)shadow;
    if (shadow_in_range(shadow_byte))
      *shadow_byte = value;
  }
}

/* Allocator wrappers for the C library malloc family of functions.
 * These functions are reached through linker --wrap options. They are not
 * Clang ASan/KASan ABI entry points. */

/* "KASA" - KASan Allocated. */
#define KASAN_HEAP_ALLOCATED_MAGIC 0x4b415341U
/* "KASF" - KASan Freed. */
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

#if KASAN_HEAP_QUARANTINE_SIZE > 0
static struct heap_header
    *heap_quarantine[KASAN_HEAP_QUARANTINE_SIZE] = {0};
static size_t heap_quarantine_oldest = 0;
static size_t heap_quarantine_count = 0;

/* Return the oldest quarantined block to the real allocator. */
static void release_oldest_quarantined_block(void) {
  struct heap_header *header = heap_quarantine[heap_quarantine_oldest];

  heap_quarantine[heap_quarantine_oldest] = NULL;
  heap_quarantine_oldest =
      (heap_quarantine_oldest + 1U) % KASAN_HEAP_QUARANTINE_SIZE;
  --heap_quarantine_count;

  __real_free(header);
}

/* Keep a freed block poisoned for a bounded number of future frees. */
static void quarantine_block(struct heap_header *header) {
  if (heap_quarantine_count == KASAN_HEAP_QUARANTINE_SIZE)
    release_oldest_quarantined_block();

  size_t index = (heap_quarantine_oldest + heap_quarantine_count) %
                 KASAN_HEAP_QUARANTINE_SIZE;
  heap_quarantine[index] = header;
  ++heap_quarantine_count;
}
#else
/* Quarantine disabled: return freed blocks to the allocator immediately. */
static void quarantine_block(struct heap_header *header) { __real_free(header); }
#endif

/* Round a size up to an allocator metadata alignment.
 * @param value Value to align.
 * @param alignment Power-of-two alignment.
 */
static size_t align_up(size_t value, size_t alignment) {
  return (value + alignment - 1U) & ~(alignment - 1U);
}

/* Find and validate the heap metadata for a user pointer.
 * @param ptr Pointer passed to a wrapped allocator function.
 * @return Heap header that owns the user pointer.
 */
static struct heap_header *header_from_payload(void *ptr) {
  uintptr_t payload = (uintptr_t)ptr;
  struct heap_header *header =
      (struct heap_header *)(payload - align_up(sizeof(struct heap_header) +
                                                    KASAN_HEAP_REDZONE_SIZE,
                                                sizeof(max_align_t)));

  if (header->magic != KASAN_HEAP_ALLOCATED_MAGIC &&
      header->magic != KASAN_HEAP_FREED_MAGIC)
    kasan_rt_report_alloc_error("free", ptr);

  return header;
}

/* Allocate a block with poisoned heap redzones.
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

  header->magic = KASAN_HEAP_ALLOCATED_MAGIC;
  header->user_size = size;
  header->total_size = total_size;
  header->payload_offset = payload_offset;

  kasan_poison(base, payload_offset, KASAN_HEAP_LEFT_REDZONE);
  kasan_unpoison(payload, size);
  kasan_poison(payload + size, KASAN_HEAP_REDZONE_SIZE,
               KASAN_HEAP_LEFT_REDZONE);

  return payload;
}

/* Allocate and zero a block with poisoned heap redzones.
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

  memset(payload, 0, total);

  return payload;
}

/* Allocate a resized block, copy old contents, and poison the old block.
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
    kasan_rt_report_alloc_error("realloc of freed allocation", ptr);

  uint8_t *new_payload = __wrap_malloc(size);
  if (!new_payload)
    return NULL;

  size_t copy_size =
      old_header->user_size < size ? old_header->user_size : size;
  memcpy(new_payload, ptr, copy_size);

  __wrap_free(ptr);
  return new_payload;
}

/* Poison a freed block and keep it quarantined.
 * @param ptr User pointer to free, or NULL.
 */
void __wrap_free(void *ptr) {
  if (!ptr)
    return;

  struct heap_header *header = header_from_payload(ptr);
  if (header->magic == KASAN_HEAP_FREED_MAGIC)
    kasan_rt_report_alloc_error("double free", ptr);

  uint8_t *base = (uint8_t *)header;
  header->magic = KASAN_HEAP_FREED_MAGIC;

  kasan_poison(base, header->total_size, KASAN_HEAP_FREED);
  quarantine_block(header);
}

/* Clang ASan/KASan ABI callbacks: Clang emits calls to these symbols when
 * outlined instrumentation is enabled. The _noabort variants are provided
 * because Clang may reference them for recoverable instrumentation modes. */

/* Generate load/store access-check callbacks for one access kind.
 * @param kind Callback kind token: load or store.
 */
#define DEFINE_ACCESS_CALLBACKS(kind)                                          \
  void __asan_##kind##1(uintptr_t addr) { check_access(#kind, addr, 1); }      \
  void __asan_##kind##2(uintptr_t addr) { check_access(#kind, addr, 2); }      \
  void __asan_##kind##4(uintptr_t addr) { check_access(#kind, addr, 4); }      \
  void __asan_##kind##8(uintptr_t addr) { check_access(#kind, addr, 8); }      \
  void __asan_##kind##16(uintptr_t addr) { check_access(#kind, addr, 16); }    \
  void __asan_##kind##N(uintptr_t addr, uintptr_t size) {                      \
    check_access(#kind, addr, (size_t)size);                                   \
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
    check_access(#kind, addr, (size_t)size);                                   \
  }

DEFINE_ACCESS_CALLBACKS(load)
DEFINE_ACCESS_CALLBACKS(store)

/* Called by Clang before code paths that do not return. */
void __asan_handle_no_return(void) {}

/* Clang ASan global descriptor layout used by __asan_register_globals. */
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

/* Register globals and poison their trailing redzones.
 * @param globals Pointer to an array of Clang ASan global descriptors.
 * @param n Number of descriptors in the array.
 */
void __asan_register_globals(uintptr_t globals, uintptr_t n) {
  struct asan_global *asan_globals = (struct asan_global *)globals;

  for (uintptr_t i = 0; i < n; ++i) {
    /* This sample only uses the descriptor fields that define the global's
     * memory layout; metadata fields are ignored. */
    uintptr_t beg = asan_globals[i].beg;
    uintptr_t size = asan_globals[i].size;
    uintptr_t size_with_redzone = asan_globals[i].size_with_redzone;

    if (size_with_redzone <= size) {
      kasan_rt_report_shadow_update_error("global descriptor", (void *)beg,
                                          (size_t)size_with_redzone);
      continue;
    }

    if (!app_range_has_shadow(beg, (size_t)size_with_redzone))
      continue;

    kasan_unpoison((void *)beg, size);
    kasan_poison((void *)(beg + size), size_with_redzone - size,
                 KASAN_GLOBAL_REDZONE);
  }
}

/* Unpoison global ranges when Clang unregisters global descriptors.
 * @param globals Pointer to an array of Clang ASan global descriptors.
 * @param n Number of descriptors in the array.
 */
void __asan_unregister_globals(uintptr_t globals, uintptr_t n) {
  struct asan_global *asan_globals = (struct asan_global *)globals;

  for (uintptr_t i = 0; i < n; ++i) {
    if (!app_range_has_shadow(asan_globals[i].beg,
                              (size_t)asan_globals[i].size_with_redzone))
      continue;

    kasan_unpoison((void *)asan_globals[i].beg,
                   asan_globals[i].size_with_redzone);
  }
}
