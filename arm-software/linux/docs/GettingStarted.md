# Getting started

This section describes how to get started with the Arm Toolchain for Linux.
It describes how to use it to compile a source code into an executable binary.

## What is in the toolchain

The default installation site of Arm Toolchain for Linux is the
`/opt/arm/arm-toolchain-for-linux` directory. It contains a complete set of LLVM
tooling, header files, compiler libraries, and runtime libraries, including the
OpenMP runtime. The main tools are as follows:

* `armclang` - the C compiler

* `armclang++` - the C++ compiler

* `armflang` - the Fortran compiler

### Note

The LLVM equivalents of these commands are also available and functionally
identical:

* `clang` - the C compiler

* `clang++` - the C++ compiler

* `flang` - the Fortran compiler

## Configure and load environment modules

After installation, you can invoke any of those commands with their absolute
paths, for example:

```
$ /opt/arm/arm-toolchain-for-linux/bin/armclang -print-resource-dir
/opt/arm/arm-toolchain-for-linux/lib/clang/<version>
```

Although fully operable, this is not the best way to use the toolchain.
Therefore, we recommend using the environment modules. You can install these on
most of the existing Linux distributions:

* Ubuntu systems

```
$ sudo apt install environment-modules
```

* Red Hat Enterprise Linux and Amazon Linux systems

```
$ sudo dnf install environment-modules
```

* SUSE Linux Enterprise Server systems

```
$ sudo zypper install environment-modules
```

After installing the environment modules, you must execute a profile script
which matches with your default shell:

* BASH

```
$ source /etc/profile.d/modules.sh
```

* csh or tcsh

```
$ source /etc/profile.d/modules.csh
```

Use the `module use` command to update your `MODULEPATH` environment variable to
include the path to the Arm Toolchain for Linux module files directory:

```
$ module use /opt/arm/modulefiles
```

You can use the `module avail` command to examine the list of available modules.

To load the module for Arm Toolchain for Linux, type:

```
$ module load atfl
```

This loads the default version of Arm Toolchain for Linux.

If multiple versions are available, you can load the necessary version by
specifying `atfl/<version>`.

Now, the toolchain commands are accessible from the command line:

```
$ which armclang
/opt/arm/arm-toolchain-for-linux/bin/armclang

$ which armclang++
/opt/arm/arm-toolchain-for-linux/bin/armclang++

$ which armflang
/opt/arm/arm-toolchain-for-linux/bin/armflang
```

## Using the compiler

### Example 1: Compile and run an example C program

Consider a simple program stored in a `.c` file, for example: `hello.c`:

```
#include <stdio.h>

int main()
{
  printf("Hello, World!");
  return 0;
}
```

To generate an executable binary file, compile your example C program with the
`armclang` compiler. Specify the input file name, `hello.c`, and the binary file
name (using `-o`), `hello`:

```
$ armclang -o hello hello.c
```

Run the executable binary file `hello`:

```
$ ./hello
Hello, World!
```

### Example 2: Compile and run an example Fortran program

Consider a simple program stored in a `.f90` file, for example: `hello.f90`:

```
program hello
  print *, 'hello world'
end program
```

To generate an executable binary file, compile your example Fortran program with
the `armflang` compiler. Specify the input file name, `hello.f90`, and the
binary file name (using `-o`), `hello`:

```
$ armflang -o hello hello.f90
```

Run the executable binary file `hello`:

```
$ ./hello
 hello world
```

## Compile and link C/C++ programs

To generate an executable binary, compile your source file, for example,
`source.c`, with the `armclang` command:

```
$ armclang source.c
```

A binary with the filename `a.out` is the output.

Optionally, use the `-o` option to set the executable filename, for example,
`binary`:

```
$ armclang -o binary source.c
```

You can specify multiple source files on a single line. This command
compiles each source file individually and links them into a single executable
binary file. For example, to compile the source files `source1.c`, and
`source2.c`, use:

```
$ armclang -o binary source1.c source2.c
```

To compile each of your source files individually into the object files,
specify the compile-only option, `-c`. Pass the resulting object files into
another invocation of `armclang` to link them into an executable binary file:

```
$ armclang -c source1.c

$ armclang -c source2.c

$ armclang -o binary source1.o source2.o
```

### Note

For the C/C++ compiler's command-line arguments reference see:
[Clang command-line argument reference](https://clang.llvm.org/docs/ClangCommandLineReference.html).

## Compile and link Fortran programs

To generate an executable binary file, compile your source file, for example,
`source.f90`, with the `armflang` command:

```
$ armflang source.f90
```

A binary with the filename `a.out` is the output.

You can specify multiple source files on a single line. This command
compiles each source file individually and links them into a single executable
binary file. For example, to compile the source files `source1.f90`, and
`source2.f90`, use:

```
$ armflang -o binary source1.f90 source2.f90
```

To compile each of your source files individually into the object files,
specify the compile-only option, `-c`. Pass the resulting object files into
another invocation of `armflang` to link them into an executable binary file:

```
$ armflang -c source1.f90

$ armflang -c source2.f90

$ armflang -o binary source1.o source2.o
```

When mixing both C/C++ and Fortran codes in a single application, make sure that
the Fortran runtime library is always linked in. You can ensure this by using
the `armflang` command for linking:

```
$ armflang -c source1.f90

$ armclang -c source2.c

$ armflang -o binary source1.o source2.o
```

### Note

For the Fortran compiler's command-line arguments reference visit this page:
[Flang command-line argument reference](https://flang.llvm.org/docs/FlangCommandLineReference.html).

## Increase the optimization level

To control the optimization level, specify the `-O<level>` option on your
compile line, and replace `<level>` with one of `0`, `1`, `2` or `3`. The `-O0`
option is the lowest, and the default, optimization level. `-O3` is the highest
optimization level. Arm compilers perform auto-vectorization at level `-O2` and
above.

For example, to compile the `source.c` file into an executable file named
`binary`, and enable the `-O3` optimization level, use:

```
$ armclang -O3 -o binary source.c
```

To compile the `source.f90` file into an executable file named `binary`, and
enable the `-O3` optimization level, use:

```
$ armflang -O3 -o binary source.f90
```

### Note

Similar to other compilers, the Arm Toolchain for Linux C and C++ compilers
can also be supplied with the `-Ofast` command-line option. However this results
in displaying the following deprecation warning:

```
warning: argument '-Ofast' is deprecated; use '-O3 -ffast-math' for the same behavior, or '-O3' to enable only conforming optimizations [-Wdeprecated-ofast]
```

You can achieve the effect of applying the `-Ofast` option when compiling the
C/C++ programs by using the `-O3 -ffast-math` options instead.

For Fortran programs, using the `-Ofast` option triggers the following
deprecation warning:

```
warning: argument '-Ofast' is deprecated; use '-O3 -ffast-math -fstack-arrays' for the same behavior, or '-O3 -fstack-arrays' to enable only conforming optimizations [-Wdeprecated-ofast]
```

You can achieve the effect of applying the `-Ofast` option when compiling the
Fortran programs by using the `-O3 -ffast-math -fstack-arrays` options instead.

## Compile and optimize using CPU auto-detection

If you tell the compiler what target CPU your application will run on, it can
make target-specific optimization decisions. Target-specific optimization
decisions help to ensure your application runs as efficiently as possible. To
tell the compiler to make target-specific compilation decisions, use the
`-mcpu=<target>` option and replace `<target>` with your target processor, from
a supported list of the targets.

The `-mcpu` option also supports the `native` argument. Using `-mcpu=native`
enables the compiler to auto-detect the architecture and processor type of the
CPU that you are running the compiler on.

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
When `-mcpu` is not specified, by default, `-mcpu=generic` is set. This setting
generates a portable output suitable for any Armv8-A-based target.

### Note

* The optimizations that are performed due to setting the `-mcpu` option, are independent of the optimizations that are performed due to setting the `-O<level>` option.

* If you run the compiler on one target, but plan to run the application you are compiling on a different target, do not use `-mcpu=native`. Instead, use `-mcpu=<target>` where `<target>` is the target processor that you run the application on.

## Standards support

For information on C, C++, and Fortran language support in Arm Toolchain for
Linux, see the [Standards support](StandardsSupport.md) section.

For details on OpenMP support in Arm Toolchain for Linux, see the
[OpenMP support](OpenMPSupport.md) section.

## Fortran recommendations

### When to use Arm Toolchain for Linux?

* To compile a code with the modern Fortran features (except coarrays/teams/collectives).

* To compile Applications that are standards compliant.

* To compile large scale applications like CP2K.

* To compile applications requiring quadruple precision real/complex type support.

### When not to use Arm Toolchain for Linux?

* Performance is not guaranteed. For the users seeking highest performance, Arm Toolchain for Linux is not recommended.

* OpenMP support is experimental.

* Code containing non-standard features/intrinsics might not work as expected.

* CMake versions older than 3.28 are not supported.
