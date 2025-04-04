## Using the Arm Performance Libraries

You can get greater performance from your code if you enable linking to the
optimized math libraries at compilation time.

### Linking to Arm Performance Libraries

To enable you to get the best performance on Arm-based systems, Arm recommends
linking to Arm Performance Libraries. Arm Performance Libraries provide
optimized standard core math libraries for high-performance computing
applications on Arm processors. Through a C interface, the following types of
routines are available:

* BLAS: Basic Linear Algebra Subprograms (including XBLAS, the extended
  precision BLAS).

* LAPACK: A comprehensive package of higher level linear algebra routines.

* FFT functions: A set of Fast Fourier Transform routines for real and complex
  data using the FFTW interface.

* Sparse linear algebra.

* libastring: A subset of libc, which is a set of optimized string functions.

The easiest way to make the Arm Performance Libraries visible to the compiler is
to load the `arm-performance-libraries` environment module. After the `atfl`
environment module has been loaded, the arm-performance-libraries module should
become available for loading:

```
$ module load arm-performance-libraries
```

This should set the `ARMPL_`-prefixed environment variables and two other
critical environment variables: `LD_LIBRARY_PATH` and `PKG_CONFIG_PATH`. The
recommended way of selecting command line flags for using a specific variant of
Arm Performance Libraries is through invoking the `pkg-config` command. Note
that the Arm Performance Libraries export several `pkg-config` modules, you
should be picking the one that you actually need, particularly when you plan to
use OpenMP (notice the `seq` vs. `omp` suffix):

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
```

#### C/C++ examples

To link to the OpenMP multi-threaded Arm Performance Libraries with a 64-bit
integer interface, and include compiler and library optimizations for an
Neoverse N1-based system, use:

```
$ armclang -fopenmp -o binary code_with_math_routines.c -mcpu=neoverse-n1 `pkg-config armpl-dynamic-ilp64-omp --cflags --libs`
```

To link to the OpenMP multi-threaded Arm Performance Libraries with a 32-bit
integer interface, and build portable output that is suitable for any
Armv8-A-based system, use:

```
$ armclang -fopenmp -o binary code_with_math_routines.c -mcpu=generic `pkg-config armpl-dynamic-lp64-omp --cflags --libs`
```

To link to the serial implementation of Arm Performance Libraries with a 32-bit
integer interface, and include compiler and library optimizations for an
Neoverse V2-based system, use:

```
$ armclang -o binary code_with_math_routines.c -mcpu=neoverse-v2 `pkg-config armpl-dynamic-lp64-seq --cflags --libs`
```

#### Fortran examples

To link to the OpenMP multi-threaded Arm Performance Libraries with a 64-bit
integer interface, and include compiler and library optimizations for an
Neoverse N2-based system, use:

```
$ armflang -fopenmp -o binary code_with_math_routines.f90 -mcpu=neoverse-n2 `pkg-config armpl-dynamic-ilp64-omp --cflags --libs`
```

To link to the OpenMP multi-threaded Arm Performance Libraries with a 32-bit
integer interface, and build portable output that is suitable for any
Armv8-A-based system, use:

```
$ armflang -fopenmp -o binary code_with_math_routines.f90 -mcpu=generic `pkg-config armpl-dynamic-lp64-omp --cflags --libs`
```

To link to the serial implementation of Arm Performance Libraries with a 32-bit
integer interface, and include compiler and library optimizations for an
Neoverse V1-based system, use:

```
$ armflang -o binary code_with_math_routines.f90 -mcpu=neoverse-v1 `pkg-config armpl-dynamic-lp64-seq --cflags --libs`
```

#### More information

For more information please visit this page:
[Get started with Arm Performance Libraries (stand-alone Linux version) Version 24.10](https://developer.arm.com/documentation/102620/latest).

To learn more about integrating Arm Performance Libraries with Arm Toolchain For
Linux please visit [Using Arm Performance Libraries (ArmPL) with ATfL](https://github.com/arm/arm-toolchain/blob/arm-software/arm-software/linux/README.md#using-arm-performance-libraries-armpl-with-atfl).

#### Note

The Arm Performance Libraries suite is also the provider of the vectorized math
routines library (libamath). This is a subset of the libm functions, which makes
it possible to vectorize the loops containing calls to those functions. The Arm
Toolchain for Linux default configuration instructs the C/C++ and Fortran
compilers to make use of this library during vectorization automatically, no
further command line options are needed. This can be disabled by specifying the
`-fveclib=none` option.
