# The Arm Toolchain for Linux (ATfL)

Welcome to the Arm Toolchain for Linux!

This toolchain is based on the LLVM compiler suite. It has been prepared
specifically for the AArch64 GNU/Linux systems.

## Installation

After unpacking the ATfL tarball, the `atfl` directory becomes available, which
contains a complete toolchain directory tree.

## Usage

The ATfL toolchain consists of the full LLVM toolkit: the compiler and
the auxiliary tools. There are three compilers available:

* armclang - The C compiler
* armclang++ - The C++ compiler
* armflang - The Fortran compiler

Before using the compiler, it is worth sourcing the `env.bash` script, which
sets the `PATH` variable, so the compiler commands are easily available:

```
$ . atfl/env.bash

$ which armclang
~/atfl/bin/armclang
```

The command line syntax is the same as LLVM's. For a more detailed description
of LLVM and the Clang compiler, you can visit this page: https://clang.llvm.org/docs/UsersManual.html

The Fortran compiler has been described here: https://flang.llvm.org/docs and
its command line reference has been provided here: https://flang.llvm.org/docs/FlangCommandLineReference.html

## Fortran support

The `armflang` compiler is the mainline LLVM Flang compiler (previously known as
`flang-new`). It supports the Fortran language standards up to and including
2003, except for length PDTs. It also largely (but not completely) supports the
2008 standard and some features of the 2018 standard (like assumed-rank). The
Coarrays are not supported.

### OpenMP support in Fortran

Experimental OpenMP 2.5 support has been made available, except atomic write and
capture with a `COMPLEX` type.

### Vectorizing loops with optimized math function calls

To extend auto-vectorization capabilities with the ability to make it vectorize
the loops containing any calls to the math library functions, The
`-fveclib=ArmPL` flag has been set by default. The consequence of this is that
the final executable binary is linked against the vectorized functions library.

## Using Arm Performance Libraries (ArmPL) with ATfL

The Arm Performance Libraries suite (ArmPL) provides optimized standard core
math libraries for numerical applications on 64-bit Arm (AArch64) processors.

In order to use ArmPL with ATfL, a special version of ArmPL needs to be
downloaded from this page: https://developer.arm.com/Tools%20and%20Software/Arm%20Performance%20Libraries#Downloads

The Fortran part of this library has been compiled with Flang (known as
`flang-new` at the time the ArmPL tarballs have been created), which should be
library-compatible with the ATfL's Fortran compiler.

A tarball matching with the installed Linux distribution should be downloaded
(for example, `arm-performance-libraries_24.10_deb_flang-new.tar`). In this
guide, Ubuntu is being used as an example.

By default, ArmPL is installed to the globally accessible `/opt/arm` directory.
However, this requires root privileges during installation. For simplicity, this
guide follows the single-user installation flow, which installs ArmPL into your
home directory.

### Installation in the user's home directory

To proceed with the installation, unpack the tarball and run the installation
script:

```
$ tar -xf arm-performance-libraries_24.10_deb_flang-new.tar

$ arm-performance-libraries_24.10_deb/arm-performance-libraries_24.10_deb.sh -a -i $HOME/armpl
```

Note that the installation script will extract the ArmPL files into your
`$HOME/armpl` directory from unpacked the `.deb` files, it will execute the
`dpkg` command for this purpose.

### Using provided environment module files

Although this is optional, it is worth loading the provided `armpl` environment
module. This should set the `ARMPL_`-prefixed environment variables and two
other critical environment variables: `LD_LIBRARY_PATH`  and `PKG_CONFIG_PATH`:

```
$ MODULEPATH=$HOME/armpl/modulefiles module avail
---- $HOME/armpl/modulefiles ----
armpl/24.10.0_flang-new

Key:
modulepath

$ MODULEPATH=$HOME/armpl/modulefiles module load armpl

$ set | grep ARMPL
ARMPL_BUILD=16
ARMPL_DIR=/home/pawosm01/armpl/armpl_24.10_flang-new
ARMPL_INCLUDES=/home/pawosm01/armpl/armpl_24.10_flang-new/include
ARMPL_LIBRARIES=/home/pawosm01/armpl/armpl_24.10_flang-new/lib

$ echo $LD_LIBRARY_PATH
$HOME/armpl/armpl_24.10_flang-new/lib

$ echo $PKG_CONFIG_PATH
$HOME/armpl/armpl_24.10_flang-new/lib/pkgconfig

$ pkg-config armpl --modversion
24.10.0

$ pkg-config armpl --variable=libdir
$HOME/armpl/armpl_24.10_flang-new/lib/pkgconfig/../../lib

$ ls -1 `pkg-config armpl --variable=libdir`
libamath.a
libamath.so
libarmpl.a
libarmpl.so
libarmpl_ilp64.a
libarmpl_ilp64.so
libarmpl_ilp64.so.3
libarmpl_ilp64.so.3.12.0
libarmpl_ilp64_mp.a
libarmpl_ilp64_mp.so
libarmpl_ilp64_mp.so.3
libarmpl_ilp64_mp.so.3.12.0
libarmpl_int64.a
libarmpl_int64.so
libarmpl_int64_mp.a
libarmpl_int64_mp.so
libarmpl_lp64.a
libarmpl_lp64.so
libarmpl_lp64.so.3
libarmpl_lp64.so.3.12.0
libarmpl_lp64_mp.a
libarmpl_lp64_mp.so
libarmpl_lp64_mp.so.3
libarmpl_lp64_mp.so.3.12.0
libarmpl_mp.a
libarmpl_mp.so
libastring.a
libastring.so
pkgconfig
```

Note that the ArmPL library exports several `pkg-config` modules, you should be
picking the one that you really need, particularly when you plan to use OpenMP:

```
$ pkg-config --list-all | grep armpl
armpl                           ArmPL - Arm Performance Libraries
armpl-Fortran-dynamic-ilp64-omp ArmPL - Arm Performance Libraries
armpl-Fortran-dynamic-ilp64-seq ArmPL - Arm Performance Libraries
armpl-Fortran-dynamic-lp64-omp  ArmPL - Arm Performance Libraries
armpl-Fortran-dynamic-lp64-seq  ArmPL - Arm Performance Libraries
armpl-Fortran-static-ilp64-omp  ArmPL - Arm Performance Libraries
armpl-Fortran-static-ilp64-seq  ArmPL - Arm Performance Libraries
armpl-Fortran-static-lp64-omp   ArmPL - Arm Performance Libraries
armpl-Fortran-static-lp64-seq   ArmPL - Arm Performance Libraries
armpl-dynamic-ilp64-omp         ArmPL - Arm Performance Libraries
armpl-dynamic-ilp64-seq         ArmPL - Arm Performance Libraries
armpl-dynamic-lp64-omp          ArmPL - Arm Performance Libraries
armpl-dynamic-lp64-seq          ArmPL - Arm Performance Libraries
armpl-static-ilp64-omp          ArmPL - Arm Performance Libraries
armpl-static-ilp64-seq          ArmPL - Arm Performance Libraries
armpl-static-lp64-omp           ArmPL - Arm Performance Libraries
armpl-static-lp64-seq           ArmPL - Arm Performance Libraries

$ pkg-config armpl-dynamic-ilp64-omp --cflags
-DINTEGER64 -fopenmp -I$HOME/armpl/armpl_24.10_flang-new/lib/pkgconfig/../../include
```

The default `armpl` module is an equivalent of `armpl-static-lp64-seq.pc`:

```
$ pkg-config armpl --cflags
-static -I$HOME/armpl/armpl_24.10_flang-new/lib/pkgconfig/../../include
```

### Examples of use

#### Compiling and linking an example C/C++ program

Note that the `--libs` flag is not yet supported by the ArmPL's `pkg-config`
modules, hence the examples below make use only of the `--libs-only-L` flag
instead and the libraries to link are being listed explicitly.

##### Plain C file

Compiling:

```
$ armclang -c example.c `pkg-config armpl --cflags`
```

Linking:

```
$ armclang -o example example.o `pkg-config armpl --libs-only-L` -larmpl -lm -static
```

##### Plain C++ file

Compiling:

```
$ armclang++ -c example.cc `pkg-config armpl --cflags`
```

Linking:

```
$ armclang++ -o example example.o `pkg-config armpl --libs-only-L` -larmpl -lm -static
```

##### An OpenMP C example

Compiling:

```
$ armclang -fopenmp -c example.c `pkg-config armpl-dynamic-lp64-omp --cflags`
```

Linking:

```
$ armclang -fopenmp -o example example.o `pkg-config armpl-dynamic-lp64-omp --libs-only-L` -larmpl -lm

$ ldd ./example
    linux-vdso.so.1 (0x0000f76d2927d000)
    libarmpl.so => $HOME/armpl/armpl_24.10_flang-new/lib/libarmpl.so (0x0000f76d25200000)
    libm.so.6 => /lib/aarch64-linux-gnu/libm.so.6 (0x0000f76d25160000)
    libomp.so => $HOME/atfl/bin/../lib/aarch64-unknown-linux-gnu/libomp.so (0x0000f76d25050000)
    libc.so.6 => /lib/aarch64-linux-gnu/libc.so.6 (0x0000f76d24ea0000)
    librt.so.1 => /lib/aarch64-linux-gnu/librt.so.1 (0x0000f76d29210000)
    libgcc_s.so.1 => /lib/aarch64-linux-gnu/libgcc_s.so.1 (0x0000f76d24e70000)
    /lib/ld-linux-aarch64.so.1 (0x0000f76d29244000)
```

#### Compiling and linking example Fortran program

Compiling (yes, it is still `--cflags` being passed to `pkg-config`):

```
$ armflang -c example.f90 `pkg-config armpl --cflags`
```

Linking:

```
$ armflang -o example example.o `pkg-config armpl --libs-only-L` -larmpl -lm -static
```

An OpenMP Fortran example:

```
$ armflang -fopenmp -c example.f90 `pkg-config armpl-Fortran-dynamic-lp64-omp --cflags`
flang-20: warning: OpenMP support in flang is still experimental [-Wexperimental-option]

$ armflang -fopenmp -o example example.o `pkg-config armpl-Fortran-dynamic-lp64-omp --libs-only-L` -larmpl -lm

$ ldd ./example
    linux-vdso.so.1 (0x0000f0bd2175a000)
    libarmpl.so => $HOME/armpl/armpl_24.10_flang-new/lib/libarmpl.so (0x0000f0bd1d600000)
    libm.so.6 => /lib/aarch64-linux-gnu/libm.so.6 (0x0000f0bd21660000)
    libFortranRuntime.so.20.0 => $HOME/atfl/bin/../lib/aarch64-unknown-linux-gnu/libFortranRuntime.so.20.0 (0x0000f0bd1cc00000)
    libFortranDecimal.so.20.0 => $HOME/atfl/bin/../lib/aarch64-unknown-linux-gnu/libFortranDecimal.so.20.0 (0x0000f0bd21610000)
    libatomic.so.1 => /lib/aarch64-linux-gnu/libatomic.so.1 (0x0000f0bd215f0000)
    libomp.so => $HOME/atfl/bin/../lib/aarch64-unknown-linux-gnu/libomp.so (0x0000f0bd1caf0000)
    libc.so.6 => /lib/aarch64-linux-gnu/libc.so.6 (0x0000f0bd1c940000)
    librt.so.1 => /lib/aarch64-linux-gnu/librt.so.1 (0x0000f0bd1d5e0000)
    libgcc_s.so.1 => /lib/aarch64-linux-gnu/libgcc_s.so.1 (0x0000f0bd1c910000)
    /lib/ld-linux-aarch64.so.1 (0x0000f0bd21721000)
```

#### Using ArmPL without loading the environment module

In the examples above it was assumed that the environment module for ArmPL has
been loaded, therefore the `LD_LIBRARY_PATH` variable has been set. This helps
the dynamic linker to find the ArmPL shared object file when an executable
program is being loaded. Yet if loading a module is not desirable, the `RPATH`
needs to be set for the non-static builds at the link time.

An OpenMP C example:

```
$ armclang -fopenmp -o example example.o `pkg-config armpl-dynamic-lp64-omp --libs-only-L` -larmpl -lm -Wl,-rpath=`pkg-config armpl-dynamic-lp64-omp --variable=libdir`

$ chrpath -l ./example
./example: RUNPATH=$HOME/armpl/armpl_24.10_flang-new/lib/pkgconfig/../../lib:$HOME/atfl/lib/clang/20/lib/aarch64-unknown-linux-gnu:$HOME/atfl/bin/../lib/aarch64-unknown-linux-gnu
```
