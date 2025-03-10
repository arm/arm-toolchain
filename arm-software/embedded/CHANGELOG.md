# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [20.1.0]

### Added
- Support for targeting AArch64 v8-R in both big-endian and little-endian modes (#64) (#68) (#102).
- Additional library variants for AArch32 with strict alignment (LLVM-ET #605).
- Newlib samples (#38).
- Support for downloading AArch64 versions of FVPs (#73).
- Newlib-nano as multilib overlay package (#60).
- Support for targeting AArch32 M-profile in big-endian mode (#44) (#50) (#51) (LLVM-ET #626).
- Support for targeting AArch32 A-profile in big-endian mode (#92) (#98).

### Changed
- AArch64 A-profile big endian library variants made strictly aligned (LLVM-ET #607).
- Disable debug symbols in picolibc builds (#135).
- Ensure sysroot is set when running libcxx tests (#57).
- Reduce nesting of subproject build folders (#54).
- Improve build efficiency by building library subprojects in parallel (#31).
- Enable exceptions/Rtti builds of libcxx with newlib (#36).
- Handle meson test return code in picolibc tests (#162).

### Fixed
- Store check-all results and continue libcxx tests on failure (#151).

## [Old Releases]

Previous release changelogs can be found [here](https://github.com/ARM-software/LLVM-embedded-toolchain-for-Arm/blob/llvm-19/CHANGELOG.md).
