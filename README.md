# F Prime ARM Vorago Toolchain

This repository contains the fork of the Arm Toolchain project for the
[F Prime Vorago support package][1], intended to help build software
for the VA41630 microcontroller. The Arm Toolchain project is a fork of
the LLVM project.

 [1]: https://github.com/fprime-community/fprime-vorago

The binaries built under this project are compatible with RHEL 8.

# Supported Variants

For compactness, this toolchain supports only the following ARMv7-M variants:

- `armv7m_hard_fpv4_sp_d16_unaligned_size` - Unaligned access enabled, no badstrb workaround
- `armv7m_hard_fpv4_sp_d16_unaligned_badstrb` - Unaligned access enabled, with badstrb workaround
- `armv7m_hard_fpv4_sp_d16_size` - Aligned access only (no unaligned), no badstrb workaround
- `armv7m_hard_fpv4_sp_d16_badstrb` - Aligned access only (no unaligned), with badstrb workaround

All variants use hard float ABI with FPv4-SP-D16, and disable C++ exceptions and RTTI.

Other ARM variants will need to be added to `LLVM_TOOLCHAIN_LIBRARY_VARIANTS` in
`arm-software/embedded/CMakeLists.txt`.

# Release builds

In order to provide efficient memset and memcpy implementations, this toolchain
compiles picolibc in Release mode rather than MinSize.

# Vorago `badstrb` feature

This compiler includes a non-standard feature for the ARM backend called
`badstrb`. This feature works around an issue in the Vorago VA416x0
microprocessor with unaligned stores (<16-bit). Any unaligned write to memory
will corrupt the neighboring byte in a 16-bit word in _external memory_. Many of
these issues are already resolved by using structure field alignment which
is standard for compilers. The problem is that 8-bit stores are _always_ going to
corrupt external memory.

This feature will avoid 8-bit store operations by lowering them to a new compiler_rt
function `__badstrb_strb`. This function will implement `strb` using an atomic
load-modify-store in 16-bits.

> [!WARNING]
> 8-bit atomic operations are discouraged in this toolchain. There is an experimental
> implementation of these operations in compiler which are _not_ enabled. It is recommeded
> to use the `verify_nostrb.py` feature in the `fprime-vorago/va416x0-baremetal-nostrb` to
> validate that these instructions do not exist in the final binary.

To read more about this feature, see [here](https://github.com/fprime-community/llvm-vorago-arm-toolchain/pull/3/).

# Reporting Issues

Please report issues under [fprime-community/fprime-vorago][2].

 [2]: https://github.com/fprime-community/fprime-vorago/issues
