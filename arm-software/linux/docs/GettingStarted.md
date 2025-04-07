## Getting Started

This document describes how to get started with the already installed Arm
Toolchain for Linux. Here you will find out how to use it to compile a source
code into an executable binary.

### Looking at the toolchain

The default installation site of Arm Toolchain For Linux is the
`/opt/arm/arm-toolchain-for-linux` directory. It contains a complete set of LLVM
tooling, header files, compiler libraries and runtime libraries (including the
OpenMP runtime). The main tools are as follows:

* `armclang` - the C compiler
* `armclang++` - the C++ compiler
* `armflang` - the Fortran compiler

Note that the LLVM equivalents of these commands (`clang`, `clang++`, `flang`)
are also available and functionally identical.

### Configure and load environment modules

After installation, you should be able to invoke any of those commands with
their absolute paths, e.g.:

```
$ /opt/arm/arm-toolchain-for-linux/bin/armclang -print-resource-dir
/opt/arm/arm-toolchain-for-linux/lib/clang/<version>
```

Although fully operable, this is not the most convenient way of using the
toolchain. Therefore, we recommend using the environment modules. These can be
installed on most of the existing Linux distributions, as presented below.

#### Ubuntu systems

```
$ sudo apt install environment-modules
```

#### Red Hat Enterprise Linux and Amazon Linux systems

```
$ sudo dnf install environment-modules
```

#### SUSE Linux Enterprise Server systems

```
$ sudo zypper install environment-modules
```

After installing environment modules, you need to execute a profile script which
matches with your default shell:

#### BASH

```
$ source /etc/profile.d/modules.sh
```

#### csh or tcsh

```
$ source /etc/profile.d/modules.csh
```

Use the `module use` command to update your `MODULEPATH` environment variable to
include the path to the Arm Toolchain For Linux module files directory:

```
$ module use /opt/arm/modulefiles
```

Alternatively, you can set or amend your `MODULEPATH` environment variable
manually. Use the `module avail` command to examine the list of available modules.

To load the module for Arm Toolchain for Linux, type:

```
$ module load atfl
```

This should load the default version of Arm Toolchain for Linux. Alternatively,
if multiple versions are available, you can load the desired version by
specifying `atfl/<version>`.

From now on, the toolchain commands should be accessible from the command line:

```
$ which armclang
/opt/arm/arm-toolchain-for-linux/bin/armclang

$ which armclang++
/opt/arm/arm-toolchain-for-linux/bin/armclang++

$ which armflang
/opt/arm/arm-toolchain-for-linux/bin/armflang
```

### Using the compiler

#### Example 1: Compile and run an example C program

Consider a simple program stored in a `.c` file, for example: `hello.c`:

```
#include <stdio.h>

int main()
{
  printf("Hello, World!");
  return 0;
}
```

To generate an executable binary, compile your example C program with the
`armclang` compiler. Specify the input file, `hello.c`, and the binary name
(using `-o`), `hello`:

```
$ armclang -o hello hello.c
```

Run the generated binary `hello`:

```
$ ./hello
Hello, World!
```

#### Example 2: Compile and run an example Fortran program

Consider a simple program stored in a `.f90` file, for example: `hello.f90`:

```
program hello
  print *, 'hello world'
end program
```

To generate an executable binary, compile your example Fortran program with the
`armflang` compiler. Specify the input file, `hello.f90`, and the binary name
(using `-o`), `hello`:

```
$ armflang -o hello hello.f90
```

Run the generated binary `hello`:

```
$ ./hello
 hello world
```

### Compile and link C/C++ programs

To generate an executable binary, compile your source file (for example,
`source.c`) with the `armclang` command:

```
$ armclang source.c
```

A binary with the filename `a.out` is the output.

Optionally, use the `-o` option to set the binary filename (for example, `binary`):

```
$ armclang -o binary source.c
```

You can specify multiple source files on a single line. Each source file is
compiled individually and then linked into a single executable binary. For
example, to compile the source files `source1.c` and `source2.c`, use:

```
$ armclang -o binary source1.c source2.c
```

To compile each of your source files individually into an object (`.o`) file,
specify the compile-only option, `-c`, and then pass the resulting object files
into another invocation of `armclang` to link them into an executable binary:

```
$ armclang -c source1.c

$ armclang -c source2.c

$ armclang -o binary source1.o source2.o
```

#### Note

For the C/C++ compiler's command line arguments reference visit this page:
[Clang command line argument reference](https://releases.llvm.org/20.1.0/tools/clang/docs/ClangCommandLineReference.html).

### Compile and link Fortran programs

To generate an executable binary, compile your source file (for example,
`source.f90`) with the `armflang` command:

```
$ armflang source.f90
```

A binary with the filename `a.out` is the output.

You can specify multiple source files on a single line. Each source file is
compiled individually and then linked into a single executable binary. For
example, to compile the source files `source1.f90` and `source2.f90`, use:

```
$ armflang -o binary source1.f90 source2.f90
```

To compile each of your source files individually into an object (`.o`) file,
specify the compile-only option, `-c`, and then pass the resulting object files
into another invocation of `armflang` to link them into an executable binary:

```
$ armflang -c source1.f90

$ armflang -c source2.f90

$ armflang -o binary source1.o source2.o
```

When mixing both C/C++ and Fortran codes in a single application, it is
important to to make sure that the Fortran runtime library is always linked in.
This can be ensured by using the `armflang` command for linking:

```
$ armflang -c source1.f90

$ armclang -c source2.c

$ armflang -o binary source1.o source2.o
```

#### Note

For the Fortran compiler's command line arguments reference visit this page:
[Flang command line argument reference](https://flang.llvm.org/docs/FlangCommandLineReference.html).

### Increase the optimization level

To control the optimization level, specify the `-O<level>` option on your
compile line, and replace `<level>` with one of `0`, `1`, `2` or `3`. The `-O0`
option is the lowest, and the default, optimization level. `-O3` is the highest
optimization level. Arm compilers performs auto-vectorization at level `-O2` and
above.

For example, to compile the `source.c` file into a binary called `binary`, and
use the `-O3` optimization level, use:

```
$ armclang -O3 -o binary source.c
```

To compile the `source.f90` file into a binary called binary, and use the `-O3`
optimization level, use:

```
$ armflang -O3 -o binary source.f90
```

#### Note

Similarly to other compilers, the Arm Toolchain for Linux C and C++ compilers
can also be supplied with the `-Ofast` option. This will however result in
displaying the following deprecation warning:

```
warning: argument '-Ofast' is deprecated; use '-O3 -ffast-math' for the same behavior, or '-O3' to enable only conforming optimizations [-Wdeprecated-ofast]
```

As the warning message states, the effect of applying the `-Ofast` option when
compiling the C/C++ programs can be achieved by using the `-O3 -ffast-math`
options instead.

In case of Fortran, the use of the `-Ofast` option triggers the following
deprecation warning:

```
warning: argument '-Ofast' is deprecated; use '-O3 -ffast-math -fstack-arrays' for the same behavior, or '-O3 -fstack-arrays' to enable only conforming optimizations [-Wdeprecated-ofast]
```

As the warning message states, the effect of applying the `-Ofast` option when
compiling Fortram programs can be achieved by using the
`-O3 -ffast-math -fstack-arrays` options instead.

### Compile and optimize using CPU auto-detection

If you tell the compiler what target CPU your application will run on, it can
make target-specific optimization decisions. Target-specific optimization
decisions help ensure your application runs as efficiently as possible. To tell
the compiler to make target-specific compilation decisions, use the
`-mcpu=<target>` option and replace `<target>` with your target processor (from
a supported list of targets).

In addition, the `-mcpu` option also supports the `native` argument.
`-mcpu=native` enables the compiler to auto-detect the architecture and
processor type of the CPU that you are running the compiler on.

For example, to auto-detect the target CPU and optimize your C application for
this target, use:

```
$ armclang -O3 -mcpu=native -o binary source.c
```

To auto-detect the target CPU and optimize your Fortran application for this
target, use:

```
$ armflang -O3 -mcpu=native -o binary source.f90
```

The `-mcpu` option supports a range of Armv8-A-based Systems-on-Chips (SoCs).
When `-mcpu` is not specified, by default, `-mcpu=generic` is set, which
generates  portable output suitable for any Armv8-A-based target.

#### Note

* The optimizations that are performed from setting the `-mcpu` option (also
  known as hardware, or CPU, tuning) are independent of the optimizations that
  are performed from setting the `-O<level>` option.

* If you run the compiler on one target, but will run the application you are
  compiling on a different target, do not use `-mcpu=native`. Instead, use
  `-mcpu=<target>` where `<target>` is the target processor that you will run
  the application on.

### Fortran Recommendations

#### Who should use Arm Toolchain For Linux

* Code with modern Fortran features (except coarrays/teams/collectives) will
  work with ATfL.

* Applications that are standard compliant will work with ATfL.

* Applications like CP2K can be compiled with ATfL.

* Applications requiring quadmath support can be compiled with ATfL.

#### Who should not use Arm Toolchain For Linux

* Performance is not guaranteed. For users seeking highest performance ATfL is
  not recommended.

* OpenMP support is experimental.

* Code containing non-standard features/intrinsics might not work as expected.

* CMake versions older than 3.20 are not supported.
