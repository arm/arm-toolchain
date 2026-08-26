# Changelog

All notable changes to this project will be documented in this file.

Although ATfL is based entirely on the LLVM project, several changes have
been introduced specifically for this toolchain. The most notable include:

- The compiler uses a config file by default, which improves
  performance-specific optimizations; most notably, it encourages the use of the
  vectorized mathematical routines in the Loop Vectorizer which enables the
  possibility of vectorizing loops containing the calls to the mathematical
  library functions.

- The Bash autocompletion has been extended to cover `armclang`, `armclang++`
  and `armflang`.

- BOLT is included as a part of the toolchain, and the compiler binaries are
  optimized with BOLT for faster compilation.

- The Amazon Linux target triple is properly recognized, which enables
  vectorizations of the pieces of code calling the `sincos`* functions.
  See [this pull request](https://github.com/llvm/llvm-project/pull/136114) for
  more details.

## [23.1.0]

This is the fourth release of the Arm Toolchain for Linux (ATfL), a successor of
the Arm Compiler for Linux (ACfL).

- Based on LLVM version 23.1.0.

- The `armclang` and `armflang` executable binaries are now optimized with BOLT.

- The ATfL package repositories have transitioned to a new and improved
  structure. Users who have previously installed ATfL using the native package
  manager need to re-configure the repository for their distribution following
  the steps described in the
  [Installation section](https://support.arm.com/documentation/110477/latest/Installation)
  of the User Guide.

- The configuration file for `armclang` and `armflang` sets the optimizer flag
  `-use-dereferenceable-at-point-semantics=false` by default to prevent a
  performance regression in comparison to the previous ATfL release.

- The configuration file for `armflang` sets the `-freal-sum-reassociation` flag
  by default due to its performance benefits. See
  [this pull request](https://github.com/llvm/llvm-project/pull/207371) for more
  details.

- Performance improvement cherry-picks:

  - https://github.com/arm/arm-toolchain/pull/984 - Resolve private array source
    from block-arg owner in alias analysis

  - https://github.com/arm/arm-toolchain/pull/985 - Avoid boxing constant-size
    trivial private arrays

  - https://github.com/arm/arm-toolchain/pull/986 - `foldVectorBinop` - don't
    fold length changing shuffles across binops

  - https://github.com/arm/arm-toolchain/pull/1008 - Use vector parts for
    internal non-power-of-two vectors

- Regression preventing reverts:

  - https://github.com/arm/arm-toolchain/pull/994 - Revert [InstCombine] Combine
    `select(C0, select(C1, b, a), b)` -> `select(C0&&!C1, a, b)`

## [22.1.0]

This is the third release of the Arm Toolchain for Linux (ATfL), a successor of
the Arm Compiler for Linux (ACfL).

- Based on LLVM version 22.1.0.

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

- Based on LLVM version 21.1.1.

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

- Based on LLVM version 20.1.

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
