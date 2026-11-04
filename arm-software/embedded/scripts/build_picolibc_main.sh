#!/bin/bash

# Copyright (c) 2026, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# Build ATfE against the latest commit on picolibc's main branch.
set -ex

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$( git -C "${SCRIPT_DIR}" rev-parse --show-toplevel )

clang --version

export CC=clang
export CXX=clang++

if [[ ! -z "${FVP_INSTALL_DIR}" ]]; then
    EXTRA_CMAKE_ARGS="${EXTRA_CMAKE_ARGS} -DENABLE_FVP_TESTING=ON -DFVP_INSTALL_DIR=${FVP_INSTALL_DIR}"
fi

mkdir -p "${REPO_ROOT}"/build
cd "${REPO_ROOT}"/build

cmake ../arm-software/embedded \
    -GNinja \
    -DFETCHCONTENT_QUIET=OFF \
    -DCPACK_PACKAGE_DIRECTORY=atfe_packages \
    -DPICOLIBC_GIT_TAG=origin/main \
    -DLLVM_TOOLCHAIN_ENABLE_PICOLIBC=ON \
    -DLLVM_TOOLCHAIN_ENABLE_LLVMLIBC=OFF \
    ${EXTRA_CMAKE_ARGS}
ninja package-llvm-toolchain

# Remove CPack working directory.
rm -rf atfe_packages/_CPack_Packages
