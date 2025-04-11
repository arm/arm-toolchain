#!/bin/bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# Build Arm Toolchain for Embedded with llvmlibc as the default C library

# The script creates a build of the toolchain in the 'build_llvmlibc_toolchain'
# directory, inside the repository tree.

set -ex

export CC=clang
export CXX=clang++

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$( git -C ${SCRIPT_DIR} rev-parse --show-toplevel )
BUILD_DIR=${REPO_ROOT}/build_llvmlibc_toolchain

mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

cmake ../arm-software/embedded -GNinja -DFETCHCONTENT_QUIET=OFF -DLLVM_TOOLCHAIN_C_LIBRARY=llvmlibc
ninja package-llvm-toolchain
