# Bare-metal KASan shadow-memory sample

This sample demonstrates a small KASan runtime that uses real ASan shadow
memory on the microbit semihosting target. Unlike the range-table KASan sample,
this one enables compiler-generated stack redzone poisoning.

The sample reserves the first 2 KiB of RAM for shadow memory in
`microbit-kasan-shadow.ld.in` and compiles with
`-mllvm -asan-mapping-offset=0x1c000000`, so application RAM maps into that
reserved shadow region. The remaining RAM is exposed to LLVM libc for data,
heap, and stack.

## Build and run

This sample targets LLVM libc only:

```sh
make build
make run
```

The program intentionally reports a KASan error and aborts, so `make run` is
expected to exit with a non-zero status after printing a `KASAN:` line.

## Test modes

Select a test with `KASAN_TEST=<n>`.

| Test | Command | Demonstrates |
| ---- | ------- | ------------ |
| 1 | `make run KASAN_TEST=1` | Automatic stack redzone poisoning from Clang |
| 2 | `make run KASAN_TEST=2` | Heap buffer overflow through LLVM libc allocator wrapping |
| 3 | `make run KASAN_TEST=3` | Heap use-after-free through LLVM libc allocator wrapping |

## Customizing the runtime

The shadow-memory runtime is more sensitive to the target memory map than the
range-table sample. If the RAM origin or size changes, update
`microbit-kasan-shadow.ld.in`, `KASAN_SHADOW_OFFSET`, and the reserved shadow
region together so `shadow = (address >> 3) + offset` lands in valid RAM for
every covered application address. Keep the linker script as the source of
truth for which application regions are actually covered.

Tune heap checking by changing `KASAN_HEAP_REDZONE_SIZE` and the quarantine
policy. The sample keeps freed blocks poisoned forever so use-after-free is
reliable in a demo; a practical runtime should use a bounded quarantine or
return old blocks to LLVM libc once the diagnostic value is no longer worth the
RAM cost. Keep the runtime and allocator wrappers unsanitized, otherwise the
reporting path can recursively instrument itself.

The reporting path currently prints a compact message and aborts. On hardware,
replace that with the project's normal fault path, such as UART output,
semihosting, ITM/SWO, retained crash records, or a debugger breakpoint. Add
symbolization only if the target has enough storage and the extra report detail
is worth the code-size cost.

## Limitations

Dynamic stack allocations remain disabled with
`-mllvm -asan-instrument-dynamic-allocas=0` to keep the runtime small.

The reserved shadow memory covers the application RAM range only. Stack, heap,
and RAM globals can map into this shadow region, but ROM/flash addresses map
outside it. The runtime treats accesses whose shadow address is outside the
reserved shadow region as unpoisoned, so redzones for ROM/flash globals are not
checked by this sample.
