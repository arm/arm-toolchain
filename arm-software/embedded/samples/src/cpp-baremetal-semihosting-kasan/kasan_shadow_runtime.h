/* Copyright (c) 2026, Arm Limited and affiliates.
 *
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

#ifndef KASAN_SHADOW_RUNTIME_H
#define KASAN_SHADOW_RUNTIME_H

#include <stddef.h>
#include <stdint.h>

/* These hooks can be overridden by the application to customize KASan runtime
 * output and report handling. */

#ifdef __cplusplus
extern "C" {
#endif

/* Output one report character.
 * It is enough to override only kasan_rt_putc() to redirect all report output:
 * other hooks are implemented in terms of it. */
void kasan_rt_putc(char c);

/* Output a NUL-terminated string. */
void kasan_rt_puts(const char *s);

/* Output an address-sized value in fixed-width hexadecimal. */
void kasan_rt_putaddr(uintptr_t addr);

/* Output a size value in fixed-width hexadecimal. */
void kasan_rt_putsize(size_t size);

/* Report a poisoned memory access.
 * @param kind Access kind, such as "load" or "store".
 * @param addr Address reported by the instrumentation callback.
 * @param size Access size in bytes.
 * @param offset Offset of the first poisoned byte within the access.
 */
void kasan_rt_report_access(const char *kind, uintptr_t addr, size_t size,
                            size_t offset);

/* Report allocator API misuse.
 * @param kind Allocator error kind, such as "double free".
 * @param ptr Pointer passed to the allocator wrapper.
 */
void kasan_rt_report_alloc_error(const char *kind, const void *ptr);

/* Report an invalid shadow-memory update.
 * @param kind Shadow update kind, such as "poison" or "unpoison".
 * @param addr Start of the unsupported address range.
 * @param size Size of the unsupported address range in bytes.
 */
void kasan_rt_report_shadow_update_error(const char *kind, const void *addr,
                                         size_t size);

#ifdef __cplusplus
}
#endif

#endif
