# ACfL to ATfL porting guide

This section describes the key differences between using the Arm Toolchain for
Linux (ATfL) and the Arm Compiler for Linux (ACfL). It also lists command-line
options and macros that you can use with ATfL as alternatives to those used in
ACfL.

Both toolchains provide frontends for compiling C, C++, and Fortran code. The
names of these frontends are consistent across both: `armclang` for C,
`armclang++` for C++, and `armflang` for Fortran.

ATfL supports quadruple precision real and complex types. ACfL does not.

## Reference version

|Compiler                      |Version|
|------------------------------|-------|
|Arm Compiler for Linux (ACfL) |24.10  |
|Arm Toolchain for Linux (ATfL)|21.1   |

## ArmPL integration

In ACfL, Arm Performance Libraries (ArmPL) are bundled with the compiler. To
simplify usage, the `-armpl` flag is provided to automatically include the
necessary headers and libraries for BLAS, LAPACK, FFT, and other numerical
routines.

In contrast, ATfL does not include ArmPL directly, but it depends on the ArmPL
package, which is installed alongside the toolchain. To use ArmPL with ATfL, you
must manually specify the locations of the headers and libraries using
`pkg-config`. See the [Getting started](./GettingStarted.md) guide for detailed
instructions.

### Note

Vector math functions from libamath are accessible without manually specifying
the libraries.

## C/C++ frontend

The C/C++ frontend in ATfL is identical to the one used in ACfL. Both are built
on the Clang frontend from the upstream LLVM project.

## Fortran frontend

The Fortran frontend in ATfL is based on the new LLVM Flang project. This
frontend is a modern, from-scratch implementation.
In contrast, ACfL used the older Classic Flang frontend, available at
[https://github.com/flang-compiler/flang](https://github.com/flang-compiler/flang).
Because ATfL introduces a new Fortran frontend, this section describes the
differences in Fortran support between the two toolchains.

### Major difference

|                |Compatibility|
|----------------|-------------|
|Binary          |No           |
|Module file     |No           |
|Array descriptor|No           |

### Difference in Fortran features

|Feature                   |ACfL                                                                                         |ATfL                                                                                                 |
|--------------------------|---------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------|
|Base Fortran standard     |Fortran 2008                                                                                 |Fortran 2018                                                                                         |
|Base OpenMP Specification |OpenMP 4.0                                                                                   |OpenMP 2.5<br>OpenMP support is experimental                                                         |
|Parameterized Derived Type|Supported                                                                                    |PDT Kind - Supported<br>PDT Length - Not supported                                                   |
|Preprocessor              |Use `-cpp` to switch ON                                                                      |Always ON<br>Use `-cpp` to switch ON processing of predefined macros and macros from the command line|
|Directives                |`ivdep`, `prefetch`, `unroll`, `nounroll`, `vector always`, `vector vectorlength`, `novector`|`vector always`                                                                                      |
|Line length               |2100                                                                                         |Unlimited                                                                                            |
|Recursive functions       |Use `-frecursive`                                                                            |Default                                                                                              |
|Main function             |Is a library linked at link-time                                                             |Is generated in the object file containing the program statement                                     |

### Difference in command-line flags

The following table summarises some of the most commonly used compiler flags in
gfortran and gives their equivalent in the Arm Fortran compiler:

|ACfL              |ATfL                      |Description                                                                                        |
|------------------|--------------------------|---------------------------------------------------------------------------------------------------|
|`-module <path>`  |`-J <path>`<br>`-I <path>`|Specifies a directory to place module files<br>Specifies a directory to search for module files    |
|`-Mallocatable=03`|`-frealloc-lhs`           |Use Fortran 2003 standard semantics for assignments to allocatables                                |
|`-Mallocatable=95`|`-fno-realloc-lhs`        |Use pre-Fortran 2003 standard semantics for assignments to allocatables                            |
|`-r8`             |`-fdefault-real-8`        |Sets the default `KIND` for `REAL` and `COMPLEX` declarations, constants, functions, and intrinsics|
|`-i8`             |`-fdefault-integer-8`     |Set the default `KIND` for `INTEGER` and `LOGICAL` to 64bit (i.e., `KIND = 8`)                     |

* Pre-defined macros

`armflang` has the following compiler and machine specific predefined processor
macros:

|ACfL                  |ATfL                  |Value        |Description                                               |
|----------------------|----------------------|-------------|----------------------------------------------------------|
|`__FLANG`             |`__flang__`           |`1`          |Selection of compiler dependent source at compile time    |
|`__arch64`            |`__arch64`            |`1`          |Selection of architecture dependent source at compile time|
|`__aarch64__`         |`__arch64__`          |`1`          |Selection of architecture dependent source at compile time|
|`__ARM_ARCH`          |N/A                   |`8`          |Selection of architecture dependent source at compile time|
|`__ARM_ARCH__`        |N/A                   |`8`          |Selection of architecture dependent source at compile time|
|`__armflang_major__`  |`__flang_major__`     |`24`/`21`    |Underlying LLVM version details                           |
|`__armflang_minor__`  |`__flang_minor__`     |`10`/`1`     |Underlying LLVM version details                           |
|`__armflang_version__`|`__flang_patchlevel__`|`24.10.1`/`0`|Underlying LLVM version details                           |
|`__linux__`           |`__linux__`           |`1`          |Targeted Operating System                                 |
|`__linux`             |`__linux__`           |`1`          |Targeted Operating System                                 |
|`linux`               |`__linux__`           |`1`          |Targeted Operating System                                 |
