# Standards support

## C standard support

See the [Clang C status page](https://clang.llvm.org/c_status.html) for
the details on C standard support.

## C++ standard support

See the [Clang C++ status page](https://clang.llvm.org/cxx_status.html) for
the details on C++ standard support.

## Fortran Standards Support

This section summarizes Fortran standards support in Flang. The information is
only provided as a guideline. The TODOs and the `Not Yet Implemented` messages
emitted by the compiler for unimplemented features should be treated as
authoritative.

### Note

No distinction is made between the support in the Parser/Semantics and MLIR or
Lowering support.

### Fortran 2023

See [A first take on Fortran 202X features for LLVM Flang](https://github.com/llvm/llvm-project/blob/main/flang/docs/F202X.md)
document describing the new features in Fortran 2023. The following table
summarizes the status of all important Fortran 2023 features.

The **Status** column uses the letters **P**, **Y**, **N** for the
implementation status:

* **P** : Partial. When the implementation is incomplete for a few cases

* **Y** : Yes. When the implementation is complete

* **N** : No. When the implementation is absent

| Feature                                                    | Status | Comments                                                |
|------------------------------------------------------------|--------|---------------------------------------------------------|
| Allow longer statement lines and overall statement length  | Y      | - |
| Automatic allocation of lengths of character variables     | N      | - |
| The specifiers typeof and classof                          | N      | - |
| Conditional expressions and arguments                      | N      | - |
| More use of boz constants                                  | P      | All usages other than enum are supported |
| Intrinsics for extracting tokens from a string             | N      | - |
| Intrinsics for Trig functions that work in degrees         | N      | - |
| Intrinsics for Trig functions that work in half revolutions| N      | - |
| Changes to system_clock                                    | N      | - |
| Changes for conformance with the new IEEE standard         | Y      | - |
| Additional named constants to specify kinds                | Y      | - |
| Extensions for c_f_pointer intrinsic                       | N      | - |
| Procedures for converting between fortran and c strings    | N      | - |
| The at edit descriptor                                     | N      | - |
| Control over leading zeros in output of real values        | N      | - |
| Extensions for Namelist                                    | N      | - |
| Allow an object of a type with a coarray ultimate component to be an array or allocatable | N | - |
| Put with Notify                                            | N      | - |
| Error conditions in collectives                            | N      | - |
| Simple procedures                                          | N      | - |
| Using integer arrays to specify subscripts                 | N      | - |
| Using integer arrays to specify rank and bound of an array | N      | - |
| Using an integer constant to specify rank                  | N      | - |
| Reduction specifier for do concurrent                      | P      | Syntax is accepted |
| Enumerations                                               | N      | - |

### Fortran 2018

The following table summarizes the status of all important Fortran 2018
features.

The **Status** column uses the letters **P**, **Y**, **N** for the
implementation status:

* **P** : Partial. When the implementation is incomplete for a few cases

* **Y** : Yes. When the implementation is complete

* **N** : No. When the implementation is absent

| Feature                                                    | Status | Comments                                                |
|------------------------------------------------------------|--------|---------------------------------------------------------|
| Asynchronous communication                                 | P      | Syntax is accepted |
| Teams                                                      | N      | - |
| Image failure                                              | P      | stat_failed_image is added |
| Form team statement                                        | N      | - |
| Change team construct                                      | N      | - |
| Coarrays allocated in teams                                | N      | - |
| Critical construct                                         | N      | - |
| Lock and unlock statements                                 | N      | - |
| Events                                                     | N      | - |
| Sync team construct                                        | N      | - |
| Image selectors                                            | N      | - |
| Intrinsic functions get_team 	and team_number              | N      | - |
| Intrinsic function image_index                             | N      | - |
| Intrinsic function num_images                              | N      | - |
| Intrinsic function this_image                              | N      | - |
| Intrinsic move_alloc extensions                            | P      | - |
| Detecting failed and stopped images                        | N      | - |
| Collective subroutines                                     | N      | - |
| New and enhanced atomic subroutines                        | N      | - |
| Failed images and stat= specifiers                         | N      | - |
| Intrinsic function coshape                                 | N      | - |

### Fortran 2008

The following table summarizes the status of all important Fortran 2008
features.

The **Status** column uses the letters **P**, **Y**, **N** for the
implementation status:

* **P** : Partial. When the implementation is incomplete for a few cases

* **Y** : Yes. When the implementation is complete

* **N** : No. When the implementation is absent

| Feature                                                    | Status | Comments                                                |
|------------------------------------------------------------|--------|---------------------------------------------------------|
| Coarrays                                                   | N      | Lowering and runtime support is not implemented         |
| do concurrent                                              | P      | Sequential execution works. Parallel support in progress|
| Internal procedure as an actual argument or pointer target | Y      | Current implementation requires stack to be executable. See [Proposal](https://github.com/llvm/llvm-project/blob/main/flang/docs/InternalProcedureTrampolines.md) |

### Fortran 2003

The following table summarizes the status of all important Fortran 2003
features.

The **Status** column uses the letters **P**, **Y**, **N** for the
implementation status:

* **P** : Partial. When the implementation is incomplete for a few cases

* **Y** : Yes. When the implementation is complete

* **N** : No. When the implementation is absent

| Feature                                                    | Status | Comments                                                |
|------------------------------------------------------------|--------|---------------------------------------------------------|
| Parameterized Derived Types                                | P      | PDT with length type parameters is not supported. See [Proposal](https://github.com/llvm/llvm-project/blob/main/flang/docs/ParameterizedDerivedTypes.md) |
| Assignment to allocatable                                  | P      | Assignment to whole allocatable in FORALL is not implemented |
| Pointer Assignment                                         | P      | Polymorphic assignment in FORALL is not implemented     |
| The VOLATILE attribute                                     | P      | VOLATILE in procedure interfaces is not implemented     |
| Asynchronous input/output                                  | P      | IO will happen synchronously                            |
| MIN/MAX extensions for CHARACTER                           | P      | Some variants are not supported                         |

### Fortran 95

All features are supported.

### Fortran 90

All features are supported.

### FORTRAN 77

All features are supported.
