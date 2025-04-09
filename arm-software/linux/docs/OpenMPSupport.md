<!--===- docs/OpenMPSupport.md

   Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
   See https://llvm.org/LICENSE.txt for license information.
   SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

-->

# Clang OpenMP support

Refer to the (status page](https://releases.llvm.org/20.1.0/tools/clang/docs/OpenMPSupport.html) for details on Clang OpenMP support.

# Flang OpenMP Support

```{contents}
---
local:
---
```

This section outlines the OpenMP API features supported by Flang. It is intended as a general reference.
For the most accurate information on unimplemented features, rely on the compiler’s TODO or “Not Yet Implemented”
messages, which are considered authoritative.  With the exception of a few corner cases, Flang
offers full support for (OpenMP 2.5)[#openmp-25-openmp-11], and partial support for [OpenMP 3.1](#openmp-31-openmp-30) 
and [OpenMP 4.0](#openmp-40). The tables below outline the current status of OpenMP 4.0, 3.1, 3.0 feature support.
Work is ongoing to add support for OpenMP 4.5 and newer versions; a support statement for these will be shared in the future.

The feature support information is provided as a table with three columns that are self explanatory. The Status column uses
the letters **P**, **Y**, **N** for the implementation status:
- **P** : Partial. When the implementation is incomplete for a few cases
- **Y** : Yes. When the implementation is complete
- **N** : No. When the implementation is absent

Note : No distinction is made between the support in Parser/Semantics, MLIR, Lowering or the OpenMPIRBuilder.

## OpenMP 4.0

| Feature                                                    | Status | Comments                                                |
|------------------------------------------------------------|--------|---------------------------------------------------------|
| proc_bind clause                                           | Y      | |
| simd construct                                             | P      | Some clauses are not supported |
| declare simd construct                                     | N      | |
| do simd construct                                          | Y      | |
| target data construct                                      | N      | |
| target construct                                           | N      | |
| target update construct                                    | N      | |
| declare target directive                                   | N      | |
| teams construct                                            | N      | |
| distribute construct                                       | N      | |
| distribute simd construct                                  | N      | |
| distribute parallel loop construct                         | N      | |
| distribute parallel loop simd construct                    | N      | |
| depend clause                                              | P      | Depend clause with array sections are not supported |
| declare reduction construct                                | N      | |
| atomic construct extensions                                | Y      | |
| cancel construct                                           | N      | |
| cancellation point construct                               | N      | |
| parallel do simd construct                                 | Y      | |
| target teams construct                                     | N      | |
| teams distribute construct                                 | N      | |
| teams distribute simd construct                            | N      | |
| target teams distribute construct                          | N      | |
| teams distribute parallel loop construct                   | N      | |
| target teams distribute parallel loop construct            | N      | |
| teams distribute parallel loop simd construct              | N      | |
| target teams distribute parallel loop simd construct       | N      | |

## OpenMP 3.1, OpenMP 3.0

| Feature                                                    | Status | Comments                                                |
|------------------------------------------------------------|--------|---------------------------------------------------------|
| intent(in) in firstprivate                                 | Y      | |
| const-qualified types in firstprivate                      | N      | |
| pointers in firstprivate and lastprivate                   | Y      | |
| final and mergeable clauses in task                        | Y      | |
| taskyield construct                                        | Y      | |
| atomic construct extensions                                | Y      | |
| assumed-size arrays are shared                             | Y      | |
| allocatable arrays in private, firstprivate, lastprivate, reduction, copyin, copyprivate  | Y      | |
| firstprivate in default                                    | Y      | |
| collapse clause                                            | Y      | |
| schedule kind auto                                         | Y      | |
| task construct                                             | P      | delayed execution of tasks is not supported |
| taskwait construct                                         | Y      | |

## OpenMP 2.5, OpenMP 1.1
All features except a few corner cases in atomic (complex type, different but compatible types in lhs and rhs), threadprivate (character type) constructs/clauses are supported.
