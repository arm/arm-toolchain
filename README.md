# F Prime ARM Vorago Toolchain

This repository contains the fork of the Arm Toolchain project for the
[F Prime Vorago support package][1], intended to help build software
for the VA41630 microcontroller. The Arm Toolchain project is a fork of
the LLVM project.

 [1]: https://github.com/fprime-community/fprime-vorago

The binaries built under this project are compatible with RHEL 8.

# Limitations

For compactness, this toolchain supports only armv7m_hard_fpv4_sp_d16_unaligned
and not any other ARM variants. Other variants will need to be added to
LLVM_TOOLCHAIN_LIBRARY_VARIANTS in arm-software/embedded/CMakeLists.txt.

# Reporting Issues

Please report issues under [fprime-community/fprime-vorago][2].

 [2]: https://github.com/fprime-community/fprime-vorago/issues
