# Shared Support Libraries

Arm Toolchain for Embedded provides small support libraries that are shared by
multiple C library variants. These libraries are installed in the selected
multilib sysroot and can be used with `picolibc`, `newlib`, `newlib-nano`, and
LLVM libc variants.

## Atomic operation helpers

LLVM can emit calls to runtime helper functions such as `__atomic_load_4`,
`__atomic_exchange_4`, and `__atomic_fetch_add_4` when the target does not have
instructions that handle an atomic operation inline. This is common on older or
small Arm targets where the maximum lock-free atomic size is smaller than the
operation being compiled, for example Armv4T, Armv5TE, and Armv6-M.

For Armv4, Armv5, and Armv6 variants that do not support pointer-size atomic
operations, ATfE provides `libatomic-fallback.a` with weak definitions for the LLVM
`__atomic_*` libcall ABI for 1, 2, 4, and 8 byte operations, plus the generic
any-size helpers. The implementation uses plain load, store, and
read-modify-write operations, and fails to compile on targets that support
pointer-size atomic operations. For those variants, such as Armv6K and later,
the `__atomic_*` helpers are provided by `compiler-rt` libraries instead.

> [!CAUTION]
> The non-lock-free fallback is intended to make non-preemptive bare-metal
> programs and tests link successfully. It does not provide atomicity if the
> same object can be accessed from interrupts, multiple cores, an RTOS
> scheduler, DMA, or any other asynchronous agent.

When real atomicity is needed, provide strong definitions for the required
`__atomic_*` functions in your application or platform library.
Those strong definitions override the weak definitions from
`libatomic-fallback.a`.

For example, a platform can override the 32-bit fetch-add helper by masking
interrupts around the critical section:

```cpp
#include <stdint.h>

static inline uint32_t save_and_disable_irq(void) {
  uint32_t primask;
  __asm__ volatile(
      "mrs %0, primask\n"
      "cpsid i\n"
      : "=r"(primask)
      :
      : "memory");
  return primask;
}

static inline void restore_irq(uint32_t primask) {
  __asm__ volatile("msr primask, %0" : : "r"(primask) : "memory");
}

extern "C" uint32_t __atomic_fetch_add_4(
    volatile void *ptr, uint32_t value, int ordering) {
  (void)ordering;
  uint32_t primask = save_and_disable_irq();
  volatile uint32_t *p = static_cast<volatile uint32_t *>(ptr);
  uint32_t old = *p;
  *p = old + value;
  restore_irq(primask);
  return old;
}
```

This example is only appropriate for targets where the `primask` register exists
and where interrupt masking is the desired synchronization mechanism.
Other platforms can instead use RTOS locks, architecture-specific atomic
instructions, or another hardware synchronization mechanism.

If you override one helper for a shared object size, also consider overriding
the related load, store, exchange, compare exchange, and fetch operation helpers
for that size so all users of that object follow the same synchronization
policy.
