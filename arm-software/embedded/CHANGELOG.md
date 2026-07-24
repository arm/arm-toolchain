# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [23.1.0]

### Added

- Shared support libraries now provide `libatomic-fallback.a` whose weak
definitions can be overridden by users on platforms without native atomic
operation support.
- Bare-metal semihosting KASan sample with a minimal sanitizer runtime added.
- SME support added to the LLVM libc startup code by providing
[__arm_sme_state](https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst#811__arm_sme_state).
- Static TLS (Thread Local Storage) support added in LLVM libc startup code to
enable language conformance testing. This did not add multithreading support.
- `clangd` was added to the binary package.
- [Guidance](docs/llvmlibc.md#migrating-from-picolibc-to-llvm-libc) added for
migrating projects from `picolibc` to LLVM libc.

### Changed

- LLVM libc support is no longer marked experimental and is considered
production quality for typical embedded use cases.
- Modular `printf` is implemented in LLVM libc so that floating point
conversion code is not linked in if it is not used by the application,
significantly reducing the code size.
- `clang_rt.profile` profiling and code coverage library is now automatically
added to the command line when profiling instrumentation is enabled.

### Deprecated

- `picolibc` will be superseded with LLVM libc as the default C library in
  ATfE 24. Further in ATfE 25 `picolibc` will be moved out of the main ATfE
  package into an overlay package. To keep using `picolibc` in your project
  add `--config=picolibc.cfg` to the command line.
- Intel-based Mac support is deprecated: in a future release the Darwin binary
package will be converted from `universal` to `arm64`-only. ATfE will keep the
CMake option to build the `universal` package from source.

### Removed
### Fixed
### Security

## [22.1.0]

### Added
- Core clang tools (`clang-check`, `clang-format`) and `clang-scan-deps` added
to the package.
- A new sample to show how to target and run code with an Arm FVP
(Fixed Virtual Platform).

### Changed
- LLVM libc added to the main ATfE package.
- LLVM libc startup code and default linker script provided.
- Many LLVM libc++ support limitations removed when used with LLVM libc.
C++ console I/O can be used now, but not file I/O yet.
- ATfE samples updated to work with LLVM libc, the separate set of LLVM libc
samples removed.
- LLVM compiler-rt profile library provided for each library variant.
It replaced the ATfE sample minimal profile runtime library implementation.
- newlib updated to version 4.6.0

### Removed
- `make.bat` files were removed from samples, because of additional logic added
in make files that was difficult to replicate in batch files. Use MSYS2 or
similar environment on Windows to run samples via make files.
- LLVM libc overlay package was removed, because LLVM libc was included into the
main ATfE package.

### Security
- AArch64 A-profile's Sign Return Address Hardening against the PACMAN attack
was implemented, see `-mharden-pac-ret` command line option.

## [21.1.0]

### Added

- [`elf2bin`](../docs/elf2bin.md) utility added.
- Use of zlib on Linux and macOS to allow compression of debug data with `-gz=zlib`.
- C++ standard library `<fstream>` header support with `picolibc` and `newlib`.
- C++ standard library support with LLVM libc overlay package.
- Startup code, C++ sample for LLVM libc overlay package.

### Changed

- Multiple changes in added and updated library variants to improve coverage
of supported targets.
- Common header files for C and C++ libraries variants are now centralized under
each target triple to avoid duplication and significantly reduce package size.
- Arm AEABI memory builtins that are provided by the C library were excluded from the `compiler-rt` build, `-meabi gnu` should be used with `-nostdlib` to prevent the compiler from using the missing builtins.
- `picolibc` now uses `__bothinit_array_start` and `__bothinit_array_end`
linker defined symbols to run the static constructors instead of the traditional
`__init_array_start` and `__init_array_end`. Linker scripts will need to be
updated to define the `__bothinit_array_start` and `__bothinit_array_end` symbols.
See the `picolibc.ld` linker script for an example.

### Removed

- Support for using `--sysroot` to point to a specific library variant directly,
multilib driver selection logic should be used instead.

### Fixed

- Using samples on Windows.
- Producing 64-bit compiler and tools binaries on Windows.

## [20.1.0]

This release is migrated from [LLVM-ET](https://github.com/ARM-software/LLVM-embedded-toolchain-for-Arm).
The package structure remains the same, making this a direct successor release.

### Added
- Support for targeting AArch64 v8-R in both big-endian and little-endian modes (#64) (#68) (#102).
- Additional library variants for AArch32 with strict alignment (LLVM-ET #605).
- Support for targeting AArch32 M-profile in big-endian mode (#44) (#50) (#51) (LLVM-ET #626).
- Support for targeting AArch32 A-profile in big-endian mode (#92) (#98).
- Support for downloading AArch64 versions of FVPs (#73).
- Newlib samples (#38).
- Newlib-nano as multilib overlay package (#60).

### Changed
- AArch64 A-profile big endian library variants made strictly aligned (LLVM-ET #607).
- Ensure sysroot is set when running libcxx tests (#57).
- Enable exceptions/RTTI builds of libcxx with newlib (#36).
- Disable debug symbols in picolibc builds (#135).
- Handle meson test return code in picolibc tests (#162).
- Reduce nesting of subproject build folders (#54).
- Improve build efficiency by building library subprojects in parallel (#31).

### Fixed
- Store check-all results and continue libcxx tests on failure (#151).

## [Old Releases]

Previous release changelogs can be found [here](https://github.com/ARM-software/LLVM-embedded-toolchain-for-Arm/blob/llvm-19/CHANGELOG.md).
