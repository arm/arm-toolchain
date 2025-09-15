# Additional features available in Arm Toolchain for Embedded

Arm Toolchain for Embedded may have additional features not available
in a toolchain built from the upstream repositories. This file
contains documentation for the additional features that are available
in Arm Toolchain for Embedded.

Additional features common to Arm Toolchain for Embedded and Arm
Toolchain for Linux can be found in `arm-toolchain-features.md`.

# Experimental Features

The following features are experimental. Experimental features may
change or be removed at any point in the future.

There are no experimental features implemented.

# Features

## elf2bin utility
In addition to the LLVM tools, Arm Toolchain for Embedded provides a
utility `elf2bin`. This extracts the contents of the loadable segments
from an ELF executable file, and outputs it in various forms suitable
for loading into embedded targets, such as Intel Hex, Motorola
S-records, or raw binary files. The documentation for `elf2bin` can be
found in `elf2bin.md`.

## Inline memcpy with LD/ST instructions
In some cases inlining of memcpy instructions performs best when using LD/ST instructions.

Usage:
```
    -mllvm -enable-inline-memcpy-ld-st
```
