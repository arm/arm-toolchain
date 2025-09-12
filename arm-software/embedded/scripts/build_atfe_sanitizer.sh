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

# Stage 1: Compile clang
mkdir -p "${REPO_ROOT}"/build_llvm
cd "${REPO_ROOT}"/build_llvm

cmake ../llvm -G Ninja \
    -DLLVM_ENABLE_PROJECTS="clang;llvm;lld" \
    -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind;compiler-rt" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCLANG_DEFAULT_LINKER="lld"

ninja

echo "==> Stage 1: Completed clang build"

# Stage 2: Compile ATfE with sanitizer
export CC="${REPO_ROOT}/build_llvm/bin/clang"
export CXX="${REPO_ROOT}/build_llvm/bin/clang++"

if [[ ! -z "${FVP_INSTALL_DIR}" ]]; then
    EXTRA_CMAKE_ARGS="${EXTRA_CMAKE_ARGS} -DENABLE_FVP_TESTING=ON -DFVP_INSTALL_DIR=${FVP_INSTALL_DIR}"
fi

mkdir -p "${REPO_ROOT}"/build
cd "${REPO_ROOT}"/build

# Enable below flag to disable memory leaks detection of LeakSanitizer.
# export ASAN_OPTIONS=detect_leaks=0

cmake ../arm-software/embedded -GNinja -DFETCHCONTENT_QUIET=OFF -DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_BUILD_TYPE=Release -DLLVM_USE_SANITIZER="Address;Undefined" -DLLVM_ENABLE_ASSERTIONS=ON ${EXTRA_CMAKE_ARGS}

ninja package-llvm-toolchain

echo "==> Stage 2: Completed sanitizer build"
