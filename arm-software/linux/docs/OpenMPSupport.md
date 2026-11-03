# OpenMP support

## Clang OpenMP support

See the [status page](https://clang.llvm.org/docs/OpenMPSupport.html)
for the details on Clang OpenMP support.

## Flang OpenMP support

See the [status page](https://flang.llvm.org/docs/OpenMPSupport.html)
for the details on Flang OpenMP support. Note that ATfL does not support
offloading.

### `do concurrent` mapping to OpenMP

Flang offers support for the `do concurrent` parallelization. To use this
feature, use the `-fdo-concurrent-to-openmp=[none|host]` flag along with
`-fopenmp`.
