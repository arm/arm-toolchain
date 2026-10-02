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
math libraries for numerical applications on 64-bit Arm(R) processors (AArch64).

In order to use ArmPL with ATfL, a special version of ArmPL needs to be
downloaded from this page: https://developer.arm.com/Tools%20and%20Software/Arm%20Performance%20Libraries#Downloads

The Fortran part of this library has been compiled with Flang (known as
`flang-new` at the time the ArmPL tarballs have been created), which should be
library-compatible with the ATfL's Fortran compiler.

A tarball matching with the installed Linux distribution should be downloaded
(for example, `arm-performance-libraries_26.07_deb_gcc.tar`). In this
guide, Ubuntu is being used as an example.

By default, ArmPL is installed to the globally accessible `/opt/arm` directory.
However, this requires root privileges during installation. For simplicity, this
guide follows the single-user installation flow, which installs ArmPL into your
home directory.

### Installation in the user's home directory

To proceed with the installation, unpack the tarball and run the installation
script:

```
$ tar -xf arm-performance-libraries_26.07_deb_gcc.tar

$ arm-performance-libraries_26.07_deb/arm-performance-libraries_26.07_deb.sh -a -i $HOME/armpl
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
---- ~/armpl/modulefiles ----
armpl/26.07.0_gcc

Key:
modulepath

$ MODULEPATH=$HOME/armpl/modulefiles module load armpl

$ set | grep ARMPL
ARMPL_BUILD=5643
ARMPL_DIR=~/armpl/armpl_26.07_gcc
ARMPL_INCLUDES=~/armpl/armpl_26.07_gcc/include
ARMPL_LIBRARIES=~/armpl/armpl_26.07_gcc/lib

$ echo $LD_LIBRARY_PATH
~/armpl/armpl_26.07_gcc/lib

$ echo $PKG_CONFIG_PATH
~/armpl/armpl_26.07_gcc/lib/pkgconfig

$ pkg-config armpl --modversion
26.07.0

$ pkg-config armpl --variable=libdir
~/armpl/armpl_26.07_gcc/lib/pkgconfig/../../lib

$ ls -1 `pkg-config armpl --variable=libdir`
libamath.a
libamath_repro.a
libamath_repro.so
libamath.so
libarmpl.a
libarmpl_ilp64.a
libarmpl_ilp64_mp.a
libarmpl_ilp64_mp.so
libarmpl_ilp64_mp.so.3
libarmpl_ilp64_mp.so.3.12.1
libarmpl_ilp64.so
libarmpl_ilp64.so.3
libarmpl_ilp64.so.3.12.1
libarmpl_int64.a
libarmpl_int64_mp.a
libarmpl_int64_mp.so
libarmpl_int64.so
libarmpl_lp64.a
libarmpl_lp64_mp.a
libarmpl_lp64_mp.so
libarmpl_lp64_mp.so.3
libarmpl_lp64_mp.so.3.12.1
libarmpl_lp64.so
libarmpl_lp64.so.3
libarmpl_lp64.so.3.12.1
libarmpl_mp.a
libarmpl_mp.so
libarmpl.so
libastring.a
libastring.so
pkgconfig
```

Note that the ArmPL library exports several `pkg-config` modules, you should be
picking the one that you really need, particularly when you plan to use OpenMP:

```
$ pkg-config --list-all | grep armpl
armpl                          ArmPL - Arm Performance Libraries
armpl-Fortran-ilp64-omp        ArmPL - Arm Performance Libraries
armpl-Fortran-ilp64-seq        ArmPL - Arm Performance Libraries
armpl-Fortran-lp64-omp         ArmPL - Arm Performance Libraries
armpl-Fortran-lp64-seq         ArmPL - Arm Performance Libraries
armpl-ilp64-omp                ArmPL - Arm Performance Libraries
armpl-ilp64-seq                ArmPL - Arm Performance Libraries
armpl-lp64-omp                 ArmPL - Arm Performance Libraries
armpl-lp64-seq                 ArmPL - Arm Performance Libraries

$ pkg-config armpl-ilp64-omp --cflags
-DINTEGER64 -I~/armpl/armpl_26.07_gcc/lib/pkgconfig/../../include
```

The default `armpl` module is an equivalent of `armpl-lp64-seq.pc`:

```
$ pkg-config armpl --cflags
-I~/armpl/armpl_26.07_gcc/lib/pkgconfig/../../include
```

### Examples of use

#### Compiling and linking an example C/C++ program

##### Plain C file

Compiling:

```
$ armclang -c example.c `pkg-config armpl --cflags`
```

Linking:

```
$ armclang -o example example.o `pkg-config armpl --libs` -lm
```

##### Plain C++ file

Compiling:

```
$ armclang++ -c example.cc `pkg-config armpl --cflags`
```

Linking:

```
$ armclang++ -o example example.o `pkg-config armpl --libs` -lm
```

##### An OpenMP C example

Compiling:

```
$ armclang -fopenmp -c example.c `pkg-config armpl-lp64-omp --cflags`
```

Linking:

```
$ armclang -fopenmp -o example example.o `pkg-config armpl-lp64-omp --libs` -lm

$ ldd ./example
    linux-vdso.so.1 (0x0000e1da0185d000)
    libarmpl_mp.so => ~/armpl/armpl_26.07_gcc/lib/libarmpl_mp.so (0x0000e1d9f4640000)
    libm.so.6 => /lib/aarch64-linux-gnu/libm.so.6 (0x0000e1d9f4590000)
    libomp.so => ~/atfl/bin/../lib/aarch64-unknown-linux-gnu/libomp.so (0x0000e1d9f4460000)
    libc.so.6 => /lib/aarch64-linux-gnu/libc.so.6 (0x0000e1d9f42a0000)
    /lib/ld-linux-aarch64.so.1 (0x0000e1da01820000)
```

#### Compiling and linking example Fortran program

Compiling (yes, it is still `--cflags` being passed to `pkg-config`):

```
$ armflang -c example.f90 `pkg-config armpl --cflags`
```

Linking:

```
$ armflang -o example example.o `pkg-config armpl --libs` -lm
```

An OpenMP Fortran example:

```
$ armflang -fopenmp -c example.f90 `pkg-config armpl-Fortran-lp64-omp --cflags`

$ armflang -fopenmp -o example example.o `pkg-config armpl-Fortran-lp64-omp --libs` -lm

$ ldd ./example
    linux-vdso.so.1 (0x0000e57539a3f000)
    libarmpl_mp.so => ~/armpl/armpl_26.07_gcc/lib/pkgconfig/../../lib/libarmpl_mp.so (0x0000e5752c820000)
    libm.so.6 => /lib/aarch64-linux-gnu/libm.so.6 (0x0000e5752c770000)
    libflang_rt.runtime.so => ~/atfl/lib/clang/*major_version*/lib/aarch64-unknown-linux-gnu/libflang_rt.runtime.so (0x0000e5752c0a0000)
    libatomic.so.1 => /lib/aarch64-linux-gnu/libatomic.so.1 (0x0000e5752c070000)
    libomp.so => ~/atfl/bin/../lib/aarch64-unknown-linux-gnu/libomp.so (0x0000e5752bf40000)
    libc.so.6 => /lib/aarch64-linux-gnu/libc.so.6 (0x0000e5752bd80000)
    /lib/ld-linux-aarch64.so.1 (0x0000e575399f0000)
```

#### Using ArmPL without loading the environment module

In the examples above it was assumed that the environment module for ArmPL has
been loaded, therefore the `LD_LIBRARY_PATH` variable has been set. This helps
the dynamic linker to find the ArmPL shared object file when an executable
program is being loaded. Yet if loading a module is not desirable, the `RUNPATH`
needs to be set for the non-static builds at the link time.

An OpenMP C example:

```
$ armclang -fopenmp -o example example.o `pkg-config armpl-lp64-omp --libs` -lm -Wl,-rpath=`pkg-config armpl-lp64-omp --variable=libdir`

$ chrpath -l ./example
./example: RUNPATH=$HOME/armpl/armpl_26.07_gcc/lib/pkgconfig/../../lib:$HOME/atfl/lib/clang/*major_version*/lib/aarch64-unknown-linux-gnu:$HOME/atfl/bin/../lib/aarch64-unknown-linux-gnu
```

## Nightly build binary distribution

The [ATfL Nightly Build and Test](https://github.com/arm/arm-toolchain/actions/workflows/atfl_nightly_build_and_test.yml) workflow provides nightly binary tarballs as downloadable zipped artifacts. Use the latest successful nightly build for the most up-to-date pre-release ATfL version.

### Using the non-root installer script with nightly builds

The [non-root installer script](https://github.com/arm/arm-toolchain/blob/arm-software/arm-software/linux/scripts/user_install.sh) supports a `--nightly` option to download and install the latest successful build from the `arm-software` branch for your Linux distribution.
Currently supported for AArch64 systems running Ubuntu 24.04 or RHEL 10. This option also downloads and installs the latest matching ArmPL package into the same user-writable installation root.

The non-root installer `--nightly` option requires the GitHub CLI `gh` command is available and authenticated before it runs. Follow the [GitHub CLI installation instructions for Linux](https://github.com/cli/cli/blob/trunk/docs/install_linux.md), then sign in to `github.com` and check your authentication:

```
$ gh auth login --hostname github.com

$ gh auth status --hostname github.com
```
The authenticated account or token must have access to the workflow runs and permission to download GitHub Actions artifacts from `arm/arm-toolchain`.
Authentication is required even though the repository is public.

Download the script from GitHub and run it with `--nightly` and your chosen installation root, for example:

```
$ curl -fsSL -o user_install.sh https://raw.githubusercontent.com/arm/arm-toolchain/arm-software/arm-software/linux/scripts/user_install.sh

$ bash user_install.sh --nightly "$HOME/tools/atfl"
```

The installer detects your distribution, verifies the downloaded artifact's SHA-256 digest, and extracts the toolchain without requiring root privileges.

It prints the selected workflow run, source commit, and artifact expiry, followed by commands to activate the installed toolchain using Environment Modules or its `env.bash` script. Add `--yes` to run non-interactively and perform a post-install compiler self-test.

Use `bash user_install.sh --help` for the full list of options.
Without `--nightly`, the script installs the latest release from the public package repositories at https://developer.arm.com/packages/arm-toolchains.

You can also download a zipped nightly tarball manually from the workflow's artifacts. Verify the zipped file SHA-256 checksum before use.

ATfL binaries must be used on trusted inputs (such as your own source code). When compiling untrusted code, sandbox the compiler.

## Providing feedback and reporting issues

Please see the [Contribution Guide](../../CONTRIBUTING.md#report-an-issue) for guidance on how to report an issue or raise a feature request.

## Contributions and Pull Requests

Please see the [Contribution Guide](../../CONTRIBUTING.md) for details.
