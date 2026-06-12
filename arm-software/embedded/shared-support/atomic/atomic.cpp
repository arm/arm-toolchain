//
// Copyright (c) 2026, Arm Limited and affiliates.
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#include <atomic>
#include <cstddef>
#include <cstdint>

// Internal implementation helpers. These pick a compiler atomic builtin when
// the target reports the operation as always lock-free, otherwise they use a
// plain single-thread fallback suitable for weak bare-metal defaults.
namespace {

template <typename T> constexpr bool always_lock_free = std::atomic<T>::is_always_lock_free;

template <typename T> T plain_load(T *ptr) { return *static_cast<volatile T *>(ptr); }

template <typename T> void plain_store(T *ptr, T value) { *static_cast<volatile T *>(ptr) = value; }

template <typename T> bool is_aligned(const void *ptr) {
  auto addr = reinterpret_cast<std::uintptr_t>(ptr);
  return ptr == nullptr || addr % alignof(T) == 0;
}

template <typename T> bool is_lock_free(const void *ptr) { return always_lock_free<T> && is_aligned<T>(ptr); }

template <typename T> T atomic_load(T *ptr, int ordering) {
  if constexpr (always_lock_free<T>)
    return __atomic_load_n(ptr, ordering);
  else
    return plain_load(ptr);
}

template <typename T> void atomic_store(T *ptr, T value, int ordering) {
  if constexpr (always_lock_free<T>)
    __atomic_store_n(ptr, value, ordering);
  else
    plain_store(ptr, value);
}

template <typename T> T atomic_exchange(T *ptr, T value, int ordering) {
  if constexpr (always_lock_free<T>)
    return __atomic_exchange_n(ptr, value, ordering);
  else {
    T old = plain_load(ptr);
    plain_store(ptr, value);
    return old;
  }
}

template <typename T> bool atomic_compare_exchange(T *ptr, T *expected, T desired, int success, int failure) {
  if constexpr (always_lock_free<T>)
    return __atomic_compare_exchange_n(ptr, expected, desired, false, success, failure);
  else {
    T old = plain_load(ptr);
    if (old == *expected) {
      plain_store(ptr, desired);
      return true;
    }
    *expected = old;
    return false;
  }
}

template <typename T, typename Op> T atomic_fetch_modify(T *ptr, T value, int ordering, Op op) {
  if constexpr (always_lock_free<T>) {
    if constexpr (Op::kind == '+')
      return __atomic_fetch_add(ptr, value, ordering);
    else if constexpr (Op::kind == '-')
      return __atomic_fetch_sub(ptr, value, ordering);
    else if constexpr (Op::kind == '&')
      return __atomic_fetch_and(ptr, value, ordering);
    else if constexpr (Op::kind == '|')
      return __atomic_fetch_or(ptr, value, ordering);
    else if constexpr (Op::kind == '^')
      return __atomic_fetch_xor(ptr, value, ordering);
    else if constexpr (Op::kind == 'n')
      return __atomic_fetch_nand(ptr, value, ordering);
  }

  T old = plain_load(ptr);
  plain_store(ptr, op(old, value));
  return old;
}

struct Add {
  static constexpr char kind = '+';
  template <typename T> T operator()(T lhs, T rhs) const { return lhs + rhs; }
};

struct Sub {
  static constexpr char kind = '-';
  template <typename T> T operator()(T lhs, T rhs) const { return lhs - rhs; }
};

struct And {
  static constexpr char kind = '&';
  template <typename T> T operator()(T lhs, T rhs) const { return lhs & rhs; }
};

struct Or {
  static constexpr char kind = '|';
  template <typename T> T operator()(T lhs, T rhs) const { return lhs | rhs; }
};

struct Xor {
  static constexpr char kind = '^';
  template <typename T> T operator()(T lhs, T rhs) const { return lhs ^ rhs; }
};

struct Nand {
  static constexpr char kind = 'n';
  template <typename T> T operator()(T lhs, T rhs) const { return ~(lhs & rhs); }
};

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
#define ASM_LABEL(name) asm(name)

// Size-specialized libatomic API functions. Clang emits calls to these for
// fixed-width atomic operations, for example __atomic_fetch_add_4.
#define DEFINE_ATOMIC_SIZE(N, TYPE)                                                                                     \
  WEAK TYPE __atomic_load_##N(TYPE *ptr, int ordering) { return atomic_load(ptr, ordering); }                           \
  WEAK void __atomic_store_##N(TYPE *ptr, TYPE value, int ordering) { atomic_store(ptr, value, ordering); }              \
  WEAK TYPE __atomic_exchange_##N(TYPE *ptr, TYPE value, int ordering) { return atomic_exchange(ptr, value, ordering); } \
  WEAK bool __atomic_compare_exchange_##N(TYPE *ptr, TYPE *expected, TYPE desired, int success, int failure) {           \
    return atomic_compare_exchange(ptr, expected, desired, success, failure);                                            \
  }                                                                                                                     \
  WEAK TYPE __atomic_fetch_add_##N(TYPE *ptr, TYPE value, int ordering) {                                                \
    return atomic_fetch_modify(ptr, value, ordering, Add{});                                                            \
  }                                                                                                                     \
  WEAK TYPE __atomic_fetch_sub_##N(TYPE *ptr, TYPE value, int ordering) {                                                \
    return atomic_fetch_modify(ptr, value, ordering, Sub{});                                                            \
  }                                                                                                                     \
  WEAK TYPE __atomic_fetch_and_##N(TYPE *ptr, TYPE value, int ordering) {                                                \
    return atomic_fetch_modify(ptr, value, ordering, And{});                                                            \
  }                                                                                                                     \
  WEAK TYPE __atomic_fetch_or_##N(TYPE *ptr, TYPE value, int ordering) {                                                 \
    return atomic_fetch_modify(ptr, value, ordering, Or{});                                                             \
  }                                                                                                                     \
  WEAK TYPE __atomic_fetch_xor_##N(TYPE *ptr, TYPE value, int ordering) {                                                \
    return atomic_fetch_modify(ptr, value, ordering, Xor{});                                                            \
  }                                                                                                                     \
  WEAK TYPE __atomic_fetch_nand_##N(TYPE *ptr, TYPE value, int ordering) {                                               \
    return atomic_fetch_modify(ptr, value, ordering, Nand{});                                                           \
  }

DEFINE_ATOMIC_SIZE(1, std::uint8_t)
DEFINE_ATOMIC_SIZE(2, std::uint16_t)
DEFINE_ATOMIC_SIZE(4, std::uint32_t)
DEFINE_ATOMIC_SIZE(8, std::uint64_t)

// Generic libatomic API functions. The C++ source uses internal shared-support
// names with asm labels to avoid redeclaring Clang's builtin function names.
WEAK bool __shared_support_atomic_is_lock_free(std::size_t size, const volatile void *ptr) noexcept
    ASM_LABEL("__atomic_is_lock_free");

WEAK void __shared_support_atomic_load(std::size_t size, void *ptr, void *ret, int) ASM_LABEL("__atomic_load");

WEAK void __shared_support_atomic_store(std::size_t size, void *ptr, void *value, int) ASM_LABEL("__atomic_store");

WEAK void __shared_support_atomic_exchange(std::size_t size, void *ptr, void *value, void *ret, int)
    ASM_LABEL("__atomic_exchange");

WEAK bool __shared_support_atomic_compare_exchange(
    std::size_t size, void *ptr, void *expected, void *desired, int, int) ASM_LABEL("__atomic_compare_exchange");

WEAK bool __shared_support_atomic_is_lock_free(std::size_t size, const volatile void *ptr) noexcept {
  switch (size) {
  case 1:
    return is_lock_free<std::uint8_t>(const_cast<const void *>(ptr));
  case 2:
    return is_lock_free<std::uint16_t>(const_cast<const void *>(ptr));
  case 4:
    return is_lock_free<std::uint32_t>(const_cast<const void *>(ptr));
  case 8:
    return is_lock_free<std::uint64_t>(const_cast<const void *>(ptr));
  default:
    return false;
  }
}

WEAK void __shared_support_atomic_load(std::size_t size, void *ptr, void *ret, int) {
  copy_bytes(ret, ptr, size);
}

WEAK void __shared_support_atomic_store(std::size_t size, void *ptr, void *value, int) {
  copy_bytes(ptr, value, size);
}

WEAK void __shared_support_atomic_exchange(std::size_t size, void *ptr, void *value, void *ret, int) {
  copy_bytes(ret, ptr, size);
  copy_bytes(ptr, value, size);
}

WEAK bool __shared_support_atomic_compare_exchange(
    std::size_t size, void *ptr, void *expected, void *desired, int, int) {
  if (equal_bytes(ptr, expected, size)) {
    copy_bytes(ptr, desired, size);
    return true;
  }
  copy_bytes(expected, ptr, size);
  return false;
}

#undef DEFINE_ATOMIC_SIZE
#undef ASM_LABEL
#undef WEAK
