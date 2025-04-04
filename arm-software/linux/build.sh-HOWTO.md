# The Arm Toolchain for Linux building script (build.sh)

The aim of the `build.sh` script is to build the ATfL product and package it
into a tarball. Usually it is being started in a CI environment which is
responsible for providing all of the necessary building tools and setting
the environment variables it utilizes. It should be also possible to run
this script manually and this document should explain how to do that on
various Linux distributions.

## Preparation of the building environment

### `.deb`-based systems (Debian, Ubuntu)

The following packages need to be installed (using `apt`):

- `binutils-dev`
- `build-essential`
- `cmake`
- `git`
- `libzstd-dev`
- `ninja-build`
- `python3`
- `python3-dev`
- `python3-myst-parser`
- `python3-pip`
- `python3-pygments`
- `python3-yaml`
- `zlib1g-dev`

### `.rpm`-based systems (RHEL, CentOS, Alma, Amazon)

- `libgcc`
- `gcc`
- `gcc-c++`
- `diffutils`
- `patch`
- `binutils`
- `binutils-devel`
- `make`
- `cmake`
- `git`
- `ninja-build`
- `zlib-devel`
- `zlib-static`
- `zstd`
- `libzstd-static` (since RHEL9)
- `wget`
- `mpfr`
- `mpfr-devel`
- `libmpc`
- `libmpc-devel`
- `isl`
- `isl-devel`
- `perl-FindBin`
- `perl-Hash-Util`
- `perl-Hash-Util-FieldHash`
- `perl-Sys-Hostname`
- `perl-File-Copy`
- `cpio`
- `tar`
- `gzip`

#### Additionally, on older `.rpm`-based systems (RHEL8, CentOS8):

- `python38`
- `python38-devel`
- `python38-pip`
- `python38-pyyaml`

#### Additionally, on more recent `.rpm`-based systems:

- `python3`
- `python3-devel`
- `python3-pip`
- `python3-pygments`
- `python3-pyyaml`
- `python3-myst-parser` (since RHEL10)

## Optional: obtaining libamath.a and libamath.so

The Amath library is used by the loop vectorizer to vectorize the loops from
which math functions are being called. This library is distributed along with
Arm Performance Libraries.

On the `.deb`-based systems, the easiest way to obtain and extract `libamath.a`
and `libamath.so` (current latest version, 24.10) is to do the following:

```
$ mkdir libamath

$ cd libamath

$ wget https://developer.arm.com/-/cdn-downloads/permalink/Arm-Performance-Libraries/Version_24.10/arm-performance-libraries_24.10_deb_flang-new.tar

$ tar -xf arm-performance-libraries_24.10_deb_flang-new.tar

$ arm-performance-libraries_24.10_deb/arm-performance-libraries_24.10_deb.sh -a -f -s .

$ dpkg -x armpl_24.10_flang-new.deb .

$ cp opt/arm/armpl_24.10_flang-new/lib/libamath.a .

$ cp opt/arm/armpl_24.10_flang-new/lib/libamath.so .

$ cd ..
```

On the `.rpm`-based systems, the easiest way to obtain and extract `libamath.a`
and `libamath.so` (current latest version, 24.10) is to do the following:

```
$ mkdir libamath

$ cd libamath

$ wget https://developer.arm.com/-/cdn-downloads/permalink/Arm-Performance-Libraries/Version_24.10/arm-performance-libraries_24.10_rpm_flang-new.tar

$ tar -xf arm-performance-libraries_24.10_rpm_flang-new.tar

$ arm-performance-libraries_24.10_rpm/arm-performance-libraries_24.10_rpm.sh -a -f -s .

$ rpm2cpio armpl_24.10_flang-new.rpm | cpio -idmv

$ cp opt/arm/armpl_24.10_flang-new/lib/libamath.a .

$ cp opt/arm/armpl_24.10_flang-new/lib/libamath.so .

$ cd ..
```

The directory to which `libamath.a` and `libamath.so` files have been copied
must be pointed at by the `LIBRARIES_DIR` environmental variable.

## Setting the environment variables in shell before running `build.sh`

The `build.sh` script reads the following environment variables:

- `README_MD_PATH` - Specifies the location of the README.md file to bundle
  (default: `arm-software/linux/README.md`)
- `SBOM_FILE_PATH` - Specifies the location of the SBOM JSON file to bundle
  (default: `arm-software/linux/SBOM_Files/ATfL-SBOM.spdx.json`)
- `MKMODULEDIRS_PATH` - Specifies the location of mkmoduledirs.sh.var to tweak
  (default: `arm-software/linux/mkmoduledirs.sh.var`)
- `SOURCES_DIR` - The directory where all source code will be stored
  (default: the top level of the cloned git repository)
- `LIBRARIES_DIR` - The **optional** directory where the ArmPL veclibs will be stored
  (default: `arm-software/linux/lib`)
- `PATCHES_DIR` - The **optional** directory where all patches will be stored
  (default: `arm-software/linux/patches`)
- `DOCS_DIR` - The directory where ATfL documents will be stored
  (default: `arm-software/linux/docs`)
- `BUILD_DIR` - The directory where all build will be happening
  (default: `arm-software/linux/build`)
- `LOGS_DIR` - The directory where all build logs will be stored
  (default: `arm-software/linux/logs`)
- `OUTPUT_DIR` - The directory where all build output will be stored
  (default: `arm-software/linux/output`)
- `RELEASE_FLAGS` - Enable release flags in the build `true`/`false`
  (default: false), set this to `true` when doing a proper release (not nightly) build
- `PARALLEL_JOBS` - The number of parallel jobs to run during the build
  (default: number of the available CPU cores)
- `ATFL_ASSERTIONS` - Enable assertions in the build ON/OFF
  (default: ON), set this to `OFF` when doing a proper release (not nightly) build
- `ATFL_TARGET_TRIPLE` - Specify the default target triple
  (default: aarch64-unknown-linux-gnu), Set it to aarch64-amazon-linux when building for Amazon Linux
- `OS_NAME` - Specify the OS name
  (default: `linux`), this name will be used as a part of the output tarball name (if not overwritten by `TAR_NAME`)
- `TAR_NAME` - The name of the tarball to be created
  (default: atfl-0.0-linux-aarch64.tar.gz)
- `ZLIB_STATIC_PATH` - Specifies the location of the static zlib library (libz.a)
  (default: `/usr/lib/aarch64-linux-gnu/libz.a`), on the RHEL and alike systems set this to `/usr/lib64/libz.a`

Particular attention must be paid when doing a release build, the
`RELEASE_FLAGS` and `ATFL_ASSERTIONS` variables must be set correctly.

The optional directories (`PATCHES_DIR` and `LIBRARIES_DIR`) will not be used
if not present.
