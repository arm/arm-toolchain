#!/bin/bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# A bash script to build the Arm Toolchain for Linux, with address and undefined sanitizer enabled.

# Script implements 2-stage pipeline: first clang is built using arm-toolchain sources.
# Then this clang is used to compile ATfL sanitizer build.
#
# The script creates a build of the toolchain in the 'build_clang_with_sanitizer' directory, inside
# the repository tree.

set -vex

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$( git -C "${SCRIPT_DIR}" rev-parse --show-toplevel )

clang --version

export CC=clang
export CXX=clang++

# Stage 1: Compile clang
echo "==> Stage 1: Starting clang build"

mkdir -p "${REPO_ROOT}"/build_llvm
cd "${REPO_ROOT}"/build_llvm

cmake -G Ninja ../llvm \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
    -DLLVM_LIT_ARGS="-v" \
    -DCMAKE_INSTALL_PREFIX=../stage1.install \
    -DLLVM_TARGETS_TO_BUILD=AArch64 \
    -DLLVM_ENABLE_PROJECTS="clang-tools-extra;clang;llvm" \
    -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind;compiler-rt"

ninja

echo "==> Stage 1: Completed clang build"

# Stage 2: Compile library
echo "==> Stage 2: Starting library build"

mkdir -p "${REPO_ROOT}"/build_libcxx
cd "${REPO_ROOT}"/build_libcxx

export CC="${REPO_ROOT}/build_llvm/bin/clang"
export CXX="${REPO_ROOT}/build_llvm/bin/clang++"

cmake -G Ninja ../runtimes \
    -DLLVM_TARGETS_TO_BUILD=AArch64 \
    -DLLVM_USE_SANITIZER="Address;Undefined" \
    -DLLVM_ENABLE_ASSERTIONS=True \
    -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind;compiler-rt" \
    -DCMAKE_C_COMPILER="$CC" -DCMAKE_CXX_COMPILER="$CXX"

ninja

echo "==> Stage 2: Completed library build"

# Stage 3: Compile sanitizer build
echo "==> Stage 3: Starting sanitizer build"

mkdir -p "${REPO_ROOT}"/build_clang_with_sanitizer
cd "${REPO_ROOT}"/build_clang_with_sanitizer

export CC="${REPO_ROOT}/build_llvm/bin/clang"
export CXX="${REPO_ROOT}/build_llvm/bin/clang++"
export LD_LIBRARY_PATH="${REPO_ROOT}"/build_libcxx/lib:$LD_LIBRARY_PATH

# Flag to disable memory leaks detection of LeakSanitizer.
export ASAN_OPTIONS=detect_leaks=0

cmake -G Ninja ../llvm \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_USE_SANITIZER="Address;Undefined" \
    -DLLVM_ENABLE_ASSERTIONS=True \
    -DCMAKE_C_COMPILER="$CC" -DCMAKE_CXX_COMPILER="$CXX" \
    -DLLVM_LIT_ARGS="-v --param=env=ASAN_OPTIONS=detect_leaks=0" \
    -DCMAKE_INSTALL_PREFIX=../stage1.install \
    -DLLVM_TARGETS_TO_BUILD=AArch64 \
    -DLLVM_ENABLE_PROJECTS="clang-tools-extra;clang;llvm" \
    -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind;compiler-rt"

ninja
echo "==> Stage 3: Completed sanitizer build"

