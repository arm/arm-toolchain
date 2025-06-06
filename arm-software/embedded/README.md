# Arm Toolchain for Embedded

[![ATfE Nightly Build and Test](https://github.com/arm/arm-toolchain/actions/workflows/atfe_nightly_build_and_test.yml/badge.svg)](https://github.com/arm/arm-toolchain/actions/workflows/atfe_nightly_build_and_test.yml)

This repository contains build scripts and auxiliary material for building a
bare-metal LLVM based toolchain targeting Arm® architecture based on:
* clang + llvm
* lld
* libc++abi
* libc++
* compiler-rt
* picolibc, or optionally newlib(-nano) or LLVM's libc

## Goal

The goal is to provide an LLVM based bare-metal toolchain that can target the
Arm architecture family from Armv6-M and newer. The toolchain follows the ABI
for the Arm Architecture and attempts to provide typical features needed for
embedded and realtime operating systems.

## Supported architectures

- Armv6-M
- Armv7-M
- Armv7E-M
- Armv8-M Mainline and Baseline
- Armv8.1-M Mainline and Baseline
- Armv4T (experimental)
- Armv5TE (experimental)
- Armv6 (experimental, using the Armv5TE library variant)
- Armv7-A
- Armv7-R
- AArch32 Armv8-A
- AArch32 Armv8-R
- AArch64 Armv8-A
- AArch64 Armv8-R

## C++ support

C++ is partially supported with the use of libc++ and libc++abi from LLVM. Features
that are not supported include:
 - Multithreading

Arm Toolchain for Embedded uses the unstable libc++ ABI version. This ABI
uses all the latest libc++ improvements and bugfixes, but may result in link
errors when linking against objects compiled against older versions of the ABI.
For more information see https://libcxx.llvm.org/DesignDocs/ABIVersioning.html.

Libc++ testing has been fully completed for only a few of the variants. The specific
variants for which testing is enabled can be found in the corresponding JSON files
located at the [path](arm-multilib/json/variants).

## Components

The Arm Toolchain for Embedded relies on the following upstream components

Component  | Link
---------- | ------------------------------------
LLVM       | https://github.com/llvm/llvm-project
picolibc   | https://github.com/picolibc/picolibc

## License

Content of this repository is licensed under Apache-2.0 with LLVM Exceptions, see
[LICENSE.txt](LICENSE.txt). Individual patch files under the
[patches](patches) folder may contain code under the upstream project license. If they are
cherry-picks of upstream commits into the Arm Toolchain for Embedded release branches,
see corresponding pull requests for references.

The resulting binaries are covered under their respective open source licenses,
see component links above.

Testing for some targets uses the freely-available but not open-source Arm FVP
models, which have their own licenses. These are not used by default, see
[Building from source](docs/building-from-source.md) for details.

## Host platforms

Arm Toolchain for Embedded is built and tested on:
* Linux Ubuntu 22.04 LTS on x86_64 and AArch64.
* macOS on x86_64 and Apple Silicon.
* Windows Server 2019 with Visual Studio on x86_64.

[Binary packages](https://github.com/arm/arm-toolchain/releases)
are provided for major LLVM releases for Linux and Windows.

## Getting started

Download a release of the toolchain for your platform from [GitHub
releases](https://github.com/arm/arm-toolchain/releases), or use a
[Nightly](docs/atfe-nightly-builds.md)
build for early access to the latest features.

After downloading, extract the archive into an arbitrary directory.

### Pre-requisite for using toolchain on Windows

Install appropriate latest supported Microsoft Visual C++ Redistributable package, such as from [Microsoft Visual C++ Redistributable latest supported downloads](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170).

### Using the toolchain

> *Note:* If you are using the toolchain in a shared environment with untrusted input,
> make sure it is sufficiently sandboxed.

To use the toolchain, on the command line you need to provide the following options:
* The target triple.
* The FPU to use.
* Disabling/enabling C++ exceptions and RTTI.
* The C runtime library: either `crt0` or `crt0-semihost`.
  `crt0` will be linked automatically, but this can be suppressed
  with the `-nostartfiles` option so that `crt0-semihost` can be used.
* The semihosting library, if using `crt0-semihost`.
* A [linker script](
  https://sourceware.org/binutils/docs/ld/Scripts.html) specified with `-T`.
  Default `picolibcpp.ld` and `picolibc.ld` scripts are provided and can be used
  directly or included from a [custom linker script](
  https://github.com/picolibc/picolibc/blob/main/doc/linking.md#using-picolibcld).

For example:
```
$ PATH=<install-dir>/ATfE-<revision>/bin:$PATH
$ clang \
--target=armv6m-none-eabi \
-mfpu=none \
-fno-exceptions \
-fno-rtti \
-nostartfiles \
-lcrt0-semihost \
-lsemihost \
-T picolibc.ld \
-o example example.c
```

`clang`'s multilib system will automatically select an appropriate set of
libraries based on your compile flags. `clang` will emit a warning if no
appropriate set of libraries can be found.

To display the directory selected by the multilib system, add the flag
`-print-multi-directory` to your `clang` command line options.

To display all available multilibs run `clang` with the flag `-print-multi-lib`
and a target triple like `--target=aarch64-none-elf` or `--target=arm-none-eabi`.

It's possible that `clang` will choose a set of libraries that are not the ones
you want to use. In this case you can bypass the multilib system by providing a
`--sysroot` option specifying the directory containing the `include` and `lib`
directories of the libraries you want to use. For example:

```
$ clang \
--sysroot=<install-dir>/ATfE-<revision>/lib/clang-runtimes/arm-none-eabi/armv6m_soft_nofp \
--target=armv6m-none-eabi \
-mfpu=none \
-fno-exceptions \
-fno-rtti \
-nostartfiles \
-lcrt0-semihost \
-lsemihost \
-T picolibc.ld \
-o example example.c
```

The FPU selection can be skipped, but it is not recommended to as the defaults
are different to GCC ones.


The builds of the toolchain come packaged with two config files, Omax.cfg and OmaxLTO.cfg.
When used, these config files enable several build optimisation flags to achieve highest performance on typical embedded benchmarks. OmaxLTO.cfg enables link-time optimisation (LTO) specific flags.
These configs can be optionally passed using the `--config` flag. For example:

```
$ clang \
example.c \
...
--config=Omax.cfg \
--config=OmaxLTO.cfg \
-o example
```

Users should be warned that Omax.cfg enables `-ffast-math` which breaks IEEE compliance and
enables maths optimisations which can affect code correctness.  LTOs are
kept separately in OmaxLTO.cfg as users may not want LTOs due to potential increase in link time
and/or increased memory usage during linking. Some of the options in the config files are undocumented internal LLVM options. For these undocumented options please see the source code of the
corresponding optimisation passes in the [LLVM project](https://github.com/llvm/llvm-project)
to find out more. Users are also encouraged to create their own configs and tune their own
flag parameters.
Information on Arm Toolchain for Embedded specific optimization flags is available in [Optimization Flags](docs/optimization-flags.md)

To optimize for code size, use `Osize.cfg` or a relevant subset of flags
provided there.

Binary releases of the Arm Toolchain for Embedded are based on release
branches of the upstream LLVM Project, thus can safely be used with all tools
provided by LLVM [releases](https://github.com/llvm/llvm-project/releases)
of matching version.

See [Migrating from Arm GNU Toolchain](docs/migrating.md)
and [Experimental newlib support](docs/newlib.md)
for advice on using Arm Toolchain for Embedded with existing projects
relying on the Arm GNU Toolchain.

> *Note:* `picolibc` provides excellent
> [support for Arm GNU Toolchain](https://github.com/picolibc/picolibc/blob/main/doc/using.md),
> so projects that require using both Arm GNU Toolchain and Arm Toolchain for Embedded
> can choose either `picolibc` or `newlib`/`newlib-nano`.

## Building from source

Arm Toolchain for Embedded is an open source project and thus can be built
from source. Please see the [Building from source](docs/building-from-source.md)
guide for detailed instructions.

## Providing feedback and reporting issues

Please see the [Contribution Guide](../../CONTRIBUTING.md#report-an-issue) for guidance on how to report an issue or raise a feature request.

## Contributions and Pull Requests

Please see the [Contribution Guide](../../CONTRIBUTING.md) for details.
