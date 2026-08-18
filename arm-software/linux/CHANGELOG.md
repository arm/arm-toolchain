# Changelog

All notable changes to this project will be documented in this file.

## [23.1.0]

- The `armclang` and `armflang` executable binaries are now optimized with BOLT.

## [22.1.0]

This is the third release of the Arm Toolchain for Linux (ATfL), a successor of
the Arm Compiler for Linux (ACfL).

Although ATfL is based entirely on LLVM version 22.1.0, several changes have
been introduced specifically for this toolchain. The most notable include:

- The compiler uses a config file by default, which improves
  performance-specific optimizations; most notably, it encourages the use of the
  vectorized mathematical routines in the Loop Vectorizer which enables the
  possibility of vectorizing loops containing the calls to the mathematical
  library functions.

- The Bash autocompletion has been extended to cover `armclang`, `armclang++`
  and `armflang`.

- The Amazon Linux target triple is properly recognized, which enables
  vectorizations of the pieces of code calling the `sincos`* functions.
  See [this pull request](https://github.com/llvm/llvm-project/pull/136114) for
  more details.

- New Fortran directives supported by `armflang`: `vector vectorlength`,
  `prefetch`, `inline`, `forceinline`, `noinline`.

- Non-standard `rtc` Fortran intrinsic implemented as an alias for `time`.

- `threadprivate` common block variables appearing in equivalence in
  a Fortran/OpenMP code are allowed.

- The Arm Toolchains package repositories are transitioning to a new and
  improved structure. Users who have previously installed using the native
  package manager can update to ATfL 22.1.0 automatically. New users must
  follow the updated instructions in the Installation.md file.

## [21.1.1]

This is the second release of the Arm Toolchain for Linux (ATfL), a successor of
the Arm Compiler for Linux (ACfL).

Although ATfL is based entirely on LLVM version 21.1.1, several changes have
been introduced specifically for this toolchain. The most notable include:

- The compiler uses a config file by default, which improves
  performance-specific optimizations; most notably, it encourages the use of the
  vectorized mathematical routines in the Loop Vectorizer which enables the
  possibility of vectorizing loops containing the calls to the mathematical
  library functions.

- The Bash autocompletion has been extended to cover `armclang`, `armclang++`
  and `armflang`.

- BOLT is now included as a part of the toolchain.

- The Amazon Linux target triple is properly recognized, which enables
  vectorizations of the pieces of code calling the `sincos`* functions.
  See [this pull request](https://github.com/llvm/llvm-project/pull/136114) for
  more details.

- Passing non-contiguous arrays to an MPI procedure causes issues in MPICH.
  [This bug report](https://github.com/llvm/llvm-project/issues/138471) provides
  more details. The issue is scheduled to be fixed in the 22.x release.

## [20.1.0]

This is the first release of the Arm Toolchain for Linux (ATfL), a successor of
the Arm Compiler for Linux (ACfL).

Although ATfL is based entirely on LLVM version 20.1, several changes have been
introduced specifically for this toolchain. The most notable include:

- The compiler uses a config file by default, which improves
  performance-specific optimizations; most notably, it encourages the use of the
  vectorized mathematical routines in the Loop Vectorizer which enables the
  possibility of vectorizing loops containing the calls to the mathematical
  library functions.

- For function whose `vscale_range` is limited to a single value, ATfL can size
  scalable vectors. The compiler can now perform bitcast-like operations between
  fixed and scalable vectors, improving optimization opportunities for code
  utilizing scalable vector types.
  See [this pull request](https://github.com/llvm/llvm-project/pull/130973) for
  more details.

- A part of transformation in the Loop Vectorizer causing 'Verification Error'
  on the WRF benchmark has been deactivated.
  See [this bug report](https://github.com/llvm/llvm-project/issues/126836) for
  more details.

- The Bash autocompletion has been extended to cover `armclang`, `armclang++`
  and `armflang`.

Please examine the `docs` directory for more details specific to the Arm
Toolchain for Linux.
