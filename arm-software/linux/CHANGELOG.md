# Changelog

All notable changes to this project will be documented in this file.

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
