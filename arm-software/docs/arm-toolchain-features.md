# Additional features available in all Arm Toolchains

Arm Toolchain for Linux and Arm Toolchain for Embedded may have
additional features not available in a toolchain built from the
upstream repositories. This file contains documentation for the
additional features that are available in both Arm Toolchain for Linux
and Arm Toolchain for Embedded.

Additional features specific to a toolchain can be found in:
* `arm-toolchain-for-embedded-features.md`
* `arm-toolchain-for-linux-features.md`

# Experimental Features

The following features are experimental. Experimental features may
change or be removed at any point in the future.

There are no experimental features implemented.

# Features

## Additional loop unroll in the LTO pipeline
In some cases it is benefitial to perform an additional loop unroll pass so that extra information becomes available to later passes, e.g. SROA.
Use cases where this could be beneficial - multiple (N>=4) nested loops.

Usage:
```
    -Wl,-plugin-opt=-extra-LTO-loop-unroll=true/false
```
