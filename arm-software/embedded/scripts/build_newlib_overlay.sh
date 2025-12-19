#!/bin/bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# A bash script to build the newlib overlay for the Arm Toolchain for Embedded

# The script creates a build of the toolchain in the 'build_newlib_overlay'
# directory, inside the repository tree.

set -ex

export CC=clang
export CXX=clang++

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$( git -C "${SCRIPT_DIR}" rev-parse --show-toplevel )
BUILD_DIR=${REPO_ROOT}/build_newlib_overlay

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

cmake ../arm-software/embedded -GNinja -DFETCHCONTENT_QUIET=OFF -DLLVM_TOOLCHAIN_C_LIBRARY=newlib -DLLVM_TOOLCHAIN_LIBRARY_OVERLAY_INSTALL=on
ninja package-llvm-toolchain

# The package-llvm-toolchain target will produce a .tar.xz package, but we also
# a zip version for Windows users
cpack -G ZIP
