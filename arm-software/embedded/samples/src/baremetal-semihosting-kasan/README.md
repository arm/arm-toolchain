# Bare-metal KASan sample

This sample shows a small Kernel Address Sanitizer (KASan) runtime for
bare-metal semihosting. The runtime uses outlined KASan access callbacks and a
compact poisoned-range table instead of a full ASan shadow-memory mapping, which
implies a much higher overhead.

## Build and run

Build with the default C library:

```sh
make build
make run
```

Build with LLVM libc:

```sh
LIBC=llvmlibc make build
LIBC=llvmlibc make run
```

The program intentionally reports an error and aborts, so `make run` is expected
to exit with a non-zero status after printing a `KASAN:` line.

## Test modes

Select a test with `KASAN_TEST=<n>`.

| Test | Command | Demonstrates |
| ---- | ------- | ------------ |
| 1 | `make run KASAN_TEST=1` | Manual poisoned redzone after a stack object |
| 2 | `LIBC=llvmlibc make run KASAN_TEST=2` | Heap use-after-free through LLVM libc allocator wrapping |
| 3 | `LIBC=llvmlibc make run KASAN_TEST=3` | Heap buffer overflow through LLVM libc allocator wrapping |
| 4 | `make run KASAN_TEST=4` | Global buffer overflow using Clang global metadata |
| 5 | `LIBC=llvmlibc make run KASAN_TEST=5` | Double free through LLVM libc allocator wrapping |
| 6 | `LIBC=llvmlibc make run KASAN_TEST=6` | Invalid free through LLVM libc allocator wrapping |

When `LIBC=llvmlibc`, test 2 is the default. Otherwise, test 1 is the default.

## Runtime scope

The runtime supports:

- Outlined `__asan_load*` and `__asan_store*` access callbacks,
- Manual poisoning with `kasan_poison` / `kasan_unpoison`,
- Global redzone poisoning from `__asan_register_globals`,
- LLVM libc `malloc`, `calloc`, `realloc`, and `free` interception via linker `--wrap`,
- Heap redzones, use-after-free detection, double-free detection, and invalid-free detection.

The allocator wrapper keeps freed blocks quarantined by not returning them to
LLVM libc, so use-after-free remains detectable in this small demo. This is
useful for a sample but is not a production allocator policy.

## Customizing the runtime

Start by deciding which memory regions need coverage. The range-table runtime is
easy to adapt because `kasan_poison` and `kasan_unpoison` work with ordinary
address ranges, but every tracked redzone consumes an entry in
`KASAN_MAX_POISONED_RANGES`. Increase that limit, replace the fixed table with a
target-specific data structure, or make allocation failures non-fatal if the
runtime must keep running after diagnostic metadata is exhausted.

For heap checking, adjust `KASAN_HEAP_REDZONE_SIZE` and the quarantine policy to
match the target's RAM budget. This sample intentionally leaks freed blocks so
use-after-free is easy to demonstrate; a practical runtime should use a bounded
quarantine or eventually call the real allocator once the diagnostic window is
closed. If the project does not use LLVM libc, replace the linker `--wrap`
intercepts with the allocator hook mechanism used by that C library.

The reporting path is deliberately small and uses `printf` followed by `abort`.
Embedded projects often replace this with semihosting, UART, ITM/SWO, a crash
record in retained RAM, or a breakpoint instruction. Keep the reporting code and
allocator wrappers unsanitized to avoid recursive KASan reports while handling
an error.

## Intentional limitations

This is a minimal sample runtime, not compiler-rt ASan. It does not implement a
full shadow-memory address mapping, automatic stack redzone poisoning,
symbolized reports, thread support, or a bounded quarantine.

Enabling compiler-generated stack poisoning would require implementing ASan
shadow memory, because Clang poisons stack redzones by writing shadow bytes
directly rather than by calling the sample's `kasan_poison` helper. That adds
RAM overhead for the shadow region, linker-script complexity, and runtime
shadow checks, which is a high cost for a small bare-metal system.
