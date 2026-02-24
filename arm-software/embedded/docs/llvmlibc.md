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

* `-lcrt0-semihost` to include a library defining the `_start` symbol (or else
  provide that symbol yourself)

* `-lsemihost` to include a library that implements porting functions
  in LLVM's libc in terms of the Arm semihosting API (or else provide
  an alternative implementation of those functions yourself)

* `-T llvmlibc.ld` to include the default linker script. Alternatively,
  you can include the linker script in your custom linker script,
  similar to [how `picolibc.ld` is used](https://github.com/picolibc/picolibc/blob/main/doc/linking.md#using-picolibcld),
  or write your own linker script defining `__stack`, and
  `__llvm_libc_heap_limit` if you are using the heap

> [!IMPORTANT]
> The default `llvmlibc.ld` is provided for testing and is derived from the
> `picolibc.ld` licensed under the BSD 3 Clause license. This may cause
> licensing obligations if used in real projects.

For example:

```
clang --config=llvmlibc.cfg --target=arm-none-eabi -march=armv7m  -nostartfiles -lcrt0-semihost -lsemihost -T llvmlibc.ld -o hello hello.c
```

> [!TIP]
> For easier migration from picolibc to LLVM libc, use the following startup
> libraries:
> * `-lcrt0` the default startup library that provides initialization and exit
>   for not hosted environments. You can override one or more of the following
>   functions in your application:
>   * `void _platform_setup_exceptions()`
>   * `void _platform_setup_memory()`
>   * `void _platform_setup_arch_extensions()`
>   * `void _platform_init()`
>   * `void _platform_debug_putc(int c)`
>   * `int _platform_get_argv(char *cmdline, int max_cmdline, const char **argv, int max_argv)`
>   * `void __llvm_libc_exit(int status)`
> * `-lcrt0-semihost` startup library to be used with the semihosting library
> `-lsemihost`.
> * `-lcrt0-none` an empty library, you have to provide the `_start` symbol.

## LLVM libc initialization

When used with ATfE provided `crt0` startup code, LLVM libc calls the following
functions in this order:
1. `void _platform_setup_exceptions()`
1. `void _platform_setup_memory()`
1. `void _platform_setup_arch_extensions()`
1. `void _platform_init_data_segments()`
1. `void _platform_init()`

You can override any of these functions in your application to customize.
The expected behavior is as follows:

* `void _platform_setup_exceptions()` - Set up the exceptions table and enable
relevant interrupts.
* `void _platform_setup_memory()` - Set up the Memory Management Unit and caches.
* `void _platform_setup_arch_extensions()` - Set up architecture extensions
that require special initialization, for example, security features that require
a cryptographic key.
* `void _platform_init_data_segments()` - Relocate read-write data into its
  runtime memory and clear the BSS (uninitialized static storage) region.
  By default, the following linker script symbols are used:
  * `__data_source` - the load address of the start of the read-write data
    image in ROM/flash.
  * `__data_start` - the destination address of the start of the read-write
    segment.
  * `__data_size` - the size of the read-write data segment to be copied.
  * `__bss_start` - the address of the start of the BSS region.
  * `__bss_size` - the size of the BSS region to be cleared.
* `void _platform_init()` - Any other initialization right before the main
function is called, for example, setup standard I/O streams.

Without ATfE provided `crt0` startup code you have to handle the following in
your own startup code:
* Initialize all relevant aspects of the hardware.
* Copy read-write and zero-initialized data.
* Call `__libc_init_array()` to run constructors.
* Call the main function.

## LLVM libc finalization

LLVM libc performs required clean up like calling destructors, then calls
`void __llvm_libc_exit(int status)` to finish execution in a way appropriate
for the platform.

The default ATfE provided `crt0` handler is an infinite loop.

## I/O retargeting

See the baremetal version of
[io.h](../../../libc/src/__support/OSUtil/baremetal/io.h) for the LLVM libc
I/O retargeting interface that should be implemented in your application to
redirect standard I/O streams.

Example implementations are provided for:
* Semihosting: [semihost.cpp](../llvmlibc-support/semihost/semihost.cpp)
* UART output: [samples](../samples/src/baremetal-uart/hello.c).

### Debug output

LLVM libc startup code uses `void _platform_debug_putc(int c)` to
emit debug diagnostic messages, for example, inside exception handlers.

With `crt0-semihost` the output is directed to `stderr` using semihosting.

With `crt0` the output is discarded. You can implement
`void _platform_debug_putc(int c)` in your application to redirect it,
for example, to a UART. The implementation must be safe to be called with
only the assumption that the stack pointer is setup: it must not use heap or
other LLVM libc functions. It must not access any code or data that is
initialized by `_platform_init_data_segments()`.

## Providing command line options

`crt0-semihost` supports getting command line options via semihosting and
passing them as `argc` and `argv` to the main function.

Semihosting passes the options as one line parsed by `crt0-semihost` using the
following rules:
* Arguments are split by a whitespace.
* No special handling for the program name: `argv[0]` will contain the first
  argument provided by semihosting.
* To pass an argument with a space in it, use quotation marks, for example,
  `"a b c "` or `'a b c '` will keep all spaces. Alternatively, use `\` to
  escape the space, for example, `a\ b\ c\ `.
  Note that if the closing quotation mark is missing then all text till the end
  of the provided command line will be treated as one argument.
* To pass a quotation mark or backslash, use escape sequences: `\"`, `\'`
  and `\\` to put `"`, `'` and `\` respectively.
  In general, `\` is treated as escape and copies the next character unless
  inside a single quotation `'` or at the end of the provided string then it
  does not have special meaning.

When using `crt0` in a no-host environment, you can provide your own
implementation of
`int _platform_get_argv(char *cmdline, int max_cmdline, const char **argv, int max_argv)`
to provide `argc` and `argv` to the main function:
* `_platform_get_argv` accepts the following parameters:
  * `char *cmdline` - the buffer to put the command line into.
  * `int max_cmdline` - the size of the `cmdline` buffer.
  * `const char **argv` - the buffer to put the arguments into or `nullptr` to
    request the estimation of the size required for this buffer.
  * `int max_argv` - the size of `argv` array.
* `_platform_get_argv` returns the following:
  * Number of arguments in the command line string if the provided `argv` is
    `nullptr` (`max_argv` is ignored in this case).
    The `argv` array must provide one extra space for the terminator.
  * The number of parsed arguments - `argc` - if the provided `argv` is
    not `nullptr`.
  * `-1` in case of an error.

The maximum accepted command line length is 255 characters.

## Samples

To use the sample programs provided by Arm Toolchain for Embedded with LLVM libc,
set the `LIBC` environment variable to `llvmlibc`, for example:
```
$ LIBC=llvmlibc make build
```
> [!WARNING]
> C++ samples have limitations described below when used with LLVM libc.


## Limitations of LLVM libc in Arm Toolchain for Embedded

At present, this toolchain builds C++ libraries limited to what is supported with
LLVM libc, for example, iostream is not available.

LLVM libc is a work in
progress. It is incomplete: not all standard C library functionality
is provided yet.
