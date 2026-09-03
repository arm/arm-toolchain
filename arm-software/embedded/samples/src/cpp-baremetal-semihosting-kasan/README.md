# C++ bare-metal Kernel Address Sanitizer (KASan) sample

This C++ sample demonstrates a small KASan runtime on the micro:bit semihosting
target to help detect a number of typical memory management problems.

KASan uses shadow memory to store compact metadata for application memory. In
this sample, each shadow byte describes an 8-byte application-memory granule;
loads and stores instrumented by Clang check the corresponding shadow byte and
report an error if the accessed address is poisoned. See Clang's
[AddressSanitizer documentation](https://clang.llvm.org/docs/AddressSanitizer.html)
for more detail on the sanitizer model.

The Makefile reserves the last 2 KB of RAM for shadow memory. It calculates the
shadow memory layout based on provided target memory map. The remaining RAM is
exposed to the C library for data, heap, and stack.

## Build and run

`make run` builds and runs the sample that executes the following tests.

| Test | Demonstrates |
| ---- | ------------ |
| 1 | Stack buffer overflow |
| 2 | Heap buffer overflow through `malloc` wrapping |
| 3 | Heap use-after-free through `calloc` and `free` wrapping |
| 4 | Heap buffer overflow after `realloc` wrapping |

For example:
```
C++ KASan shadow-memory sample
Test 1: automatic stack redzone
Writing one byte past a stack buffer
KASAN: invalid store at 0x200037a0, size 0x00000001, offset 0x00000000 (recovered)
Test 2: heap buffer overflow
Writing one byte past a malloc allocation
KASAN: invalid store at 0x20000868, size 0x00000001, offset 0x00000000 (recovered)
Test 3: heap use-after-free
Freeing an allocation
Attempting use-after-free
KASAN: invalid store at 0x200008a0, size 0x00000001, offset 0x00000000 (recovered)
Test 4: realloc buffer overflow
Writing one byte past a resized malloc allocation
KASAN: invalid store at 0x20000938, size 0x00000001, offset 0x00000000 (recovered)
All KASan tests completed
```

## Customizing the runtime

### Shadow memory

The shadow memory runtime is sensitive to the target memory map. If the RAM
origin or RAM size changes, override `RAM_ORIGIN` or `HW_RAM_SIZE` when
invoking `make`, for example:
`make run RAM_ORIGIN=<origin> HW_RAM_SIZE=<size>`.
The Makefile calculates `KASAN_SHADOW_OFFSET` from those values.

### Heap

The sample keeps recently freed blocks in a bounded quarantine queue so
use-after-free remains detectable. When the quarantine is full, the oldest
poisoned block is returned to the real allocator.

Use `make KASAN_HEAP_QUARANTINE_SIZE=<n>` to change the
number of retained freed blocks, or `0` to disable quarantine and free blocks
immediately.

### Redirecting messages

Error reporting avoids `printf` and formats messages through weak output
hooks. The default `kasan_rt_putc` implementation uses libc `putc`, while
`kasan_rt_puts`, `kasan_rt_putaddr`, and `kasan_rt_putsize` build on top of it.

A target can redirect reports by defining its own non-weak hooks, usually just
`kasan_rt_putc`:

```cpp
#include "kasan_shadow_runtime.h"

extern "C" void kasan_rt_putc(char c) {
  // Write c to UART, ITM/SWO, retained RAM, or another target-specific sink.
}
```

### Error reporting and recovery

The runtime provides weak report handlers: `kasan_rt_report_access`,
`kasan_rt_report_alloc_error`, and `kasan_rt_report_shadow_update_error`. The
default handlers print a report and abort. 

This sample overrides the access and allocator handlers in `hello.cpp` to print
`(recovered)` and return, so every demo can run in one process:

```cpp
#include "kasan_shadow_runtime.h"

// Handlers must be unsanitized to avoid infinite recursion
extern "C" __attribute__((no_sanitize("kernel-address"))) void
kasan_rt_report_access(const char *kind, uintptr_t addr, size_t size,
                       size_t offset) {
  // Report the access and return to recover, or abort to stop.
}
```

Avoid allocating memory in these hooks, because they run after KASan has found
memory corruption.

Returning from a report handler allows the invalid access to continue: recovery
is useful for diagnostics but is not a safe in production.

## Limitations

Dynamic stack allocations are disabled with
`-mllvm -asan-instrument-dynamic-allocas=0` to keep the runtime small.

The reserved shadow memory covers the application RAM range only. Stack, heap,
and RAM globals can map into this shadow region, but ROM/flash addresses map
outside it. The runtime treats accesses whose shadow address is outside the
reserved shadow region as unpoisoned, so redzones for ROM/flash globals are not
checked by this sample.
