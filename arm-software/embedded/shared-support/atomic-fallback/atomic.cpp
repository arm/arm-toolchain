//
// Copyright (c) 2026, Arm Limited and affiliates.
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#include <cstddef>
#include <cstdint>

static_assert(
    !__atomic_always_lock_free(sizeof(void *), nullptr),
    "libatomic-fallback is only for targets without lock-free pointer-size atomic operations; "
    "use compiler-rt atomic builtins instead");

namespace {

// Internal implementation helpers. These are plain single-threaded operations
// suitable only for weak bare-metal fallback definitions.
template <typename T> T fallback_load(T *ptr) { return *static_cast<volatile T *>(ptr); }

template <typename T> void fallback_store(T *ptr, T value) { *static_cast<volatile T *>(ptr) = value; }

template <typename T> T fallback_exchange(T *ptr, T value) {
  T old = fallback_load(ptr);
  fallback_store(ptr, value);
  return old;
}

template <typename T> bool fallback_compare_exchange(T *ptr, T *expected, T desired) {
  T old = fallback_load(ptr);
  if (old == *expected) {
    fallback_store(ptr, desired);
    return true;
  }
  *expected = old;
  return false;
}

template <typename T, typename Op> T fallback_fetch_modify(T *ptr, T value, Op op) {
  T old = fallback_load(ptr);
  fallback_store(ptr, op(old, value));
  return old;
}

// Do not replace these byte helpers with libc mem*() calls. This library is
// linked into LLVM libc hermetic tests, where introducing a libc dependency
// would break the test link.
// Do not add __restrict on the source or destination pointers as this may
// allow the LoopIdiomRecognize optimization pass to turn the loops into
// memcpy/memcmp calls, recreating the libc dependency.

void copy_bytes(void *dst, const void *src, std::size_t size) {
  auto *dst_bytes = static_cast<unsigned char *>(dst);
  auto *src_bytes = static_cast<const unsigned char *>(src);
  for (std::size_t i = 0; i != size; ++i)
    dst_bytes[i] = src_bytes[i];
}

bool equal_bytes(const void *lhs, const void *rhs, std::size_t size) {
  auto *lhs_bytes = static_cast<const unsigned char *>(lhs);
  auto *rhs_bytes = static_cast<const unsigned char *>(rhs);
  for (std::size_t i = 0; i != size; ++i) {
    if (lhs_bytes[i] != rhs_bytes[i])
      return false;
  }
  return true;
}

} // namespace

// Weak libatomic ABI exports. Applications can provide strong definitions for
// any of these symbols to use target-specific synchronization, such as masking
// interrupts around a non-lock-free operation.
#define WEAK extern "C" __attribute__((weak))

// Size-specialized libatomic API functions. Clang emits calls to these for
// fixed-width atomic operations, for example __atomic_fetch_add_4.
#define DEFINE_ATOMIC_N(N, TYPE)                                                                                     \
  WEAK TYPE __atomic_load_##N(TYPE *ptr, int) { return fallback_load(ptr); }                                         \
  WEAK void __atomic_store_##N(TYPE *ptr, TYPE value, int) { fallback_store(ptr, value); }                           \
  WEAK TYPE __atomic_exchange_##N(TYPE *ptr, TYPE value, int) { return fallback_exchange(ptr, value); }              \
  WEAK bool __atomic_compare_exchange_##N(TYPE *ptr, TYPE *expected, TYPE desired, int, int) {                       \
    return fallback_compare_exchange(ptr, expected, desired);                                                        \
  }                                                                                                                  \
  WEAK TYPE __atomic_fetch_add_##N(TYPE *ptr, TYPE value, int) {                                                     \
    return fallback_fetch_modify(ptr, value, [](TYPE lhs, TYPE rhs) -> TYPE { return lhs + rhs; });                  \
  }                                                                                                                  \
  WEAK TYPE __atomic_fetch_sub_##N(TYPE *ptr, TYPE value, int) {                                                     \
    return fallback_fetch_modify(ptr, value, [](TYPE lhs, TYPE rhs) -> TYPE { return lhs - rhs; });                  \
  }                                                                                                                  \
  WEAK TYPE __atomic_fetch_and_##N(TYPE *ptr, TYPE value, int) {                                                     \
    return fallback_fetch_modify(ptr, value, [](TYPE lhs, TYPE rhs) -> TYPE { return lhs & rhs; });                  \
  }                                                                                                                  \
  WEAK TYPE __atomic_fetch_or_##N(TYPE *ptr, TYPE value, int) {                                                      \
    return fallback_fetch_modify(ptr, value, [](TYPE lhs, TYPE rhs) -> TYPE { return lhs | rhs; });                  \
  }                                                                                                                  \
  WEAK TYPE __atomic_fetch_xor_##N(TYPE *ptr, TYPE value, int) {                                                     \
    return fallback_fetch_modify(ptr, value, [](TYPE lhs, TYPE rhs) -> TYPE { return lhs ^ rhs; });                  \
  }                                                                                                                  \
  WEAK TYPE __atomic_fetch_nand_##N(TYPE *ptr, TYPE value, int) {                                                    \
    return fallback_fetch_modify(ptr, value, [](TYPE lhs, TYPE rhs) -> TYPE { return ~(lhs & rhs); });               \
  }

DEFINE_ATOMIC_N(1, std::uint8_t)
DEFINE_ATOMIC_N(2, std::uint16_t)
DEFINE_ATOMIC_N(4, std::uint32_t)
DEFINE_ATOMIC_N(8, std::uint64_t)

// Generic libatomic API functions. The C++ source uses internal shared-support
// names with asm labels to avoid redeclaring Clang's builtin function names.
WEAK bool __shared_support_atomic_is_lock_free(std::size_t, const volatile void *) noexcept
    asm("__atomic_is_lock_free");
WEAK bool __shared_support_atomic_is_lock_free(std::size_t, const volatile void *) noexcept {
  return false;
}

WEAK void __shared_support_atomic_load(std::size_t size, void *ptr, void *ret, int) asm("__atomic_load");
WEAK void __shared_support_atomic_load(std::size_t size, void *ptr, void *ret, int) {
  copy_bytes(ret, ptr, size);
}

WEAK void __shared_support_atomic_store(std::size_t size, void *ptr, void *value, int) asm("__atomic_store");
WEAK void __shared_support_atomic_store(std::size_t size, void *ptr, void *value, int) {
  copy_bytes(ptr, value, size);
}

WEAK void __shared_support_atomic_exchange(std::size_t size, void *ptr, void *value, void *ret, int)
    asm("__atomic_exchange");
WEAK void __shared_support_atomic_exchange(std::size_t size, void *ptr, void *value, void *ret, int) {
  copy_bytes(ret, ptr, size);
  copy_bytes(ptr, value, size);
}

WEAK bool __shared_support_atomic_compare_exchange(
    std::size_t size, void *ptr, void *expected, void *desired, int, int) asm("__atomic_compare_exchange");
WEAK bool __shared_support_atomic_compare_exchange(
    std::size_t size, void *ptr, void *expected, void *desired, int, int) {
  if (equal_bytes(ptr, expected, size)) {
    copy_bytes(ptr, desired, size);
    return true;
  }
  copy_bytes(expected, ptr, size);
  return false;
}

#undef DEFINE_ATOMIC_N
#undef WEAK
