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
