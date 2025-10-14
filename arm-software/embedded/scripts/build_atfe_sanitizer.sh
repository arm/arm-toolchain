#!/bin/bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# A bash script to build the Arm Toolchain for Embedded, with address sanitizer enabled.

# Script implements 2-stage pipeline: first clang is built using arm-toolchain sources.
# Then this clang is used to compile ATfE sanitizer build.
#
# The script creates a build of the toolchain in the 'build' directory, inside
# the repository tree.

# If FVPs have been installed, the environment variable `FVP_INSTALL_DIR`
# should be set to their install location to enable their use in tests.

set -ex

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$( git -C "${SCRIPT_DIR}" rev-parse --show-toplevel )

clang --version

export CC=clang
export CXX=clang++

echo "==> Stage 1: Building clang (unsanitized)"
mkdir -p "${REPO_ROOT}"/build_llvm
cd "${REPO_ROOT}"/build_llvm

cmake ../llvm -G Ninja \
    -DLLVM_ENABLE_PROJECTS="clang;llvm;lld" \
    -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind;compiler-rt" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCLANG_DEFAULT_LINKER="lld" \
    -DCMAKE_INSTALL_PREFIX="${REPO_ROOT}/stage1.install"

ninja

echo "==> Stage 2: Building clang (sanitized)"

if [[ ! -z "${FVP_INSTALL_DIR}" ]]; then
    EXTRA_CMAKE_ARGS="${EXTRA_CMAKE_ARGS} -DENABLE_FVP_TESTING=ON -DFVP_INSTALL_DIR=${FVP_INSTALL_DIR}"
fi

mkdir -p "${REPO_ROOT}"/build_clang_with_sanitizer
cd "${REPO_ROOT}"/build_clang_with_sanitizer

cmake ../arm-software/embedded \
    -GNinja -DFETCHCONTENT_QUIET=OFF \
    -DCMAKE_C_COMPILER="${REPO_ROOT}/build_llvm/bin/clang" \
    -DCMAKE_CXX_COMPILER="${REPO_ROOT}/build_llvm/bin/clang++" \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_USE_SANITIZER="Address;Undefined" \
    -DLLVM_ENABLE_ASSERTIONS=ON ${EXTRA_CMAKE_ARGS} \
    -DCMAKE_INSTALL_PREFIX="${REPO_ROOT}/stage2.install"

ninja package-llvm-toolchain

echo "==> Stage 2: Completed building clang (sanitized)"

