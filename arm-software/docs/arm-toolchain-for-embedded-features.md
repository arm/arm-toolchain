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

## PAC-RET hardening against the PACMAN attack

Arm Toolchain for Embedded provides the command-line option
`-mharden-pac-ret=load-return-address`, which can be used to harden return
address signing with a load of the return address, effectively reducing the
attack surface for the PACMAN attack. A function attribute that follows the same
naming scheme is also introduced.

# Features

## elf2bin utility
In addition to the LLVM tools, Arm Toolchain for Embedded provides a
utility `elf2bin`. This extracts the contents of the loadable segments
from an ELF executable file, and outputs it in various forms suitable
for loading into embedded targets, such as Intel Hex, Motorola
S-records, or raw binary files. The documentation for `elf2bin` can be
found in `elf2bin.md`.
