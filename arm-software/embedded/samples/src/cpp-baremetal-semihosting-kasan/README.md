# C++ bare-metal Kernel Address Sanitizer (KASan) sample

This C++ sample demonstrates a small KASan runtime on the micro:bit semihosting
target to help detect a number of typical memory management problems.

KASan uses shadow memory to store compact metadata for application memory. In
this sample, each shadow byte describes an 8-byte application-memory granule;
loads and stores instrumented by Clang check the corresponding shadow byte and
report an error if the accessed address is poisoned. See Clang's
[AddressSanitizer documentation](https://clang.llvm.org/docs/AddressSanitizer.html)
for more detail on the sanitizer model.

The sample reserves the last 2 KiB of RAM for shadow memory in
`microbit-kasan-shadow.ld.in` and compiles with
`-mllvm -asan-mapping-offset=0x1c003800`, so the application RAM maps into that
reserved shadow region without moving the normal RAM origin. The remaining RAM
is exposed to the selected C library for data, heap, and stack.

## Build and run

`make run` builds and runs the sample that executes the following tests.

| Test | Demonstrates |
| ---- | ------------ |
| 1 | Stack buffer overflow |
| 2 | Heap buffer overflow through `malloc` wrapping |
| 3 | Heap use-after-free through `calloc` and `free` wrapping |
| 4 | Heap buffer overflow after `realloc` wrapping |

## Customizing the runtime

The shadow memory runtime is sensitive to the target memory map. If the RAM
origin, RAM size, or shadow placement changes, update
`microbit-kasan-shadow.ld.in`, `KASAN_SHADOW_OFFSET`, and the compiler
`-asan-mapping-offset` together so `shadow = (address >> 3) + offset` lands in
valid RAM for every covered application address. Keep the linker script as the
source of truth for which application regions are actually covered.

Tune heap checking by changing `KASAN_HEAP_REDZONE_SIZE` and the quarantine
policy. The sample keeps freed blocks poisoned forever so use-after-free is
reliable in a demo; a practical runtime should use a bounded quarantine or
return old blocks to the allocator once the diagnostic value is no longer worth the
RAM cost. Keep the runtime and allocator wrappers unsanitized, otherwise the
reporting path can recursively instrument itself.

The reporting path avoids `printf` and formats messages through weak output
hooks. The default `kasan_rt_putc` implementation uses libc `putc`, while
`kasan_rt_puts` and `kasan_rt_putaddr` build on top of it. A target can redirect
reports by defining its own non-weak hooks, usually just `kasan_rt_putc`:

```cpp
extern "C" void kasan_rt_putc(char c) {
  // Write c to UART, ITM/SWO, retained RAM, or another target-specific sink.
}
```

The runtime provides weak report handlers, `kasan_rt_report_access` and
`kasan_rt_report_alloc_error`. The default handlers print a report and abort.
This sample overrides them in `hello.cpp` with unsanitized handlers (to avoid
infinite recursion) that print `(recovered)` and return, so every demo can run
in one process:

```cpp
extern "C" __attribute__((no_sanitize("kernel-address"))) void
kasan_rt_report_access(const char *kind, uintptr_t addr, uintptr_t size) {
  // Report the access and return to recover, or abort to stop.
}
```

C code does not need the `extern "C"` wrapper. Keep these hooks small and avoid
allocating memory from them, because they run after KASan has found memory
corruption. Returning from a report handler allows the invalid access to
continue, so recovery is useful for diagnostics but is not a safe production
policy. Add symbolization only if the target has enough storage and the extra
report detail is worth the code-size cost.

## Limitations

Dynamic stack allocations remain disabled with
`-mllvm -asan-instrument-dynamic-allocas=0` to keep the runtime small.

The reserved shadow memory covers the application RAM range only. Stack, heap,
and RAM globals can map into this shadow region, but ROM/flash addresses map
outside it. The runtime treats accesses whose shadow address is outside the
reserved shadow region as unpoisoned, so redzones for ROM/flash globals are not
checked by this sample.
