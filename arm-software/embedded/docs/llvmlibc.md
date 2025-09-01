# Experimental LLVM libc support

Arm Toolchain for Embedded uses
[`picolibc`](https://github.com/picolibc/picolibc) as the standard C
library. For experimental and evaluation purposes, you can instead
choose to use the LLVM project's own C library.

> [!WARNING]
> `llvmlibc` support in Arm Toolchain for Embedded is
> an experimental technology preview, with significant limitations.

## Building the toolchain with LLVM libc

> [!IMPORTANT]
> Building the LLVM libc package is only supported on Linux and macOS.

Configure the toolchain with the CMake setting
`-DLLVM_TOOLCHAIN_C_LIBRARY=llvmlibc` to build a version of the
toolchain based on LLVM libc.

If you also add `-DLLVM_TOOLCHAIN_LIBRARY_OVERLAY_INSTALL=on` then the
`package-llvm-toolchain` CMake target will generate an overlay package
similar to the [newlib overlay
package](/docs/newlib.md).
If you unpack this over an existing installation of the toolchain,
then you can switch to LLVM libc by adding `--config=llvmlibc.cfg` on
the command line.

## Using LLVM libc

To compile a program with this LLVM libc, you must provide the
following command line options, in addition to `--target`, `-march` or
`-mcpu`, and the input and output files:

* `--config=llvmlibc.cfg` if you are using LLVM libc as an overlay
  package (but you do not need this if you have built the whole
  toolchain with only LLVM libc)

* `-nostartfiles` to not include the currently non-existent `crt0.o`

* `-lcrt0` to include a library defining the `_start` symbol (or else
  provide that symbol yourself)

* `-lsemihost` to include a library that implements porting functions
  in LLVM's libc in terms of the Arm semihosting API (or else provide
  an alternative implementation of those functions yourself)

* `-T llvmlibc.ld` to include the default linker script. Alternatively,
  you can include the linker script in your custom linker script,
  similar to [how `picolibc.ld` is used](https://github.com/picolibc/picolibc/blob/main/doc/linking.md#using-picolibcld),
  or write your own linker script defining `__stack`, and
  `__llvm_libc_heap_limit` if you are using the heap

For example:

```
clang --config=llvmlibc.cfg --target=arm-none-eabi -march=armv7m  -nostartfiles -lcrt0 -lsemihost -T llvmlibc.ld -o hello hello.c
```

## Samples

The overlay package installs a llvmlibc directory in the samples/src
directory containing sample programs that use LLVM libc.

## Limitations of LLVM libc in Arm Toolchain for Embedded

At present, this toolchain builds C++ libraries limited to what is supported with
LLVM libc, for example, iostream is not available.

LLVM libc is a work in
progress. It is incomplete: not all standard C library functionality
is provided yet.
