## Release notes

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
