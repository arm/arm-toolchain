#!/bin/bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# A bash script to build the Arm Toolchain for Linux, with Address (ASan) and Undefined Behaviour Sanitizer (UBSan) enabled.

# Script implements 3-stage pipeline: 
# 1. Unsanitized clang is built using arm-toolchain sources.
# 2. Needed libraries are built.
# 3. Unsanitized clang is used to compile the sanitized clang build.
#
# The script creates a build of the toolchain in the 'build_sanitizer' directory, inside
# the repository tree.

# Disable CCACHE
export CCACHE_DISABLE=1

set -vex

# Flag to link AddressSanitizer runtime before other libraries
export ASAN_OPTIONS=verify_asan_link_order=1

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$( git -C "${SCRIPT_DIR}" rev-parse --show-toplevel )

echo "==> Stage 1: Building clang (unsanitized)"
mkdir -p "${REPO_ROOT}"/build_llvm
cd "${REPO_ROOT}"/build_llvm

cmake -G Ninja ../llvm \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DLLVM_LIT_ARGS="-v" \
    -DCMAKE_INSTALL_PREFIX=../stage1.install \
    -DLLVM_TARGETS_TO_BUILD=AArch64 \
    -DLLVM_ENABLE_PROJECTS="clang;llvm;lld" \
    -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind;compiler-rt"

ninja

echo "==> Stage 2: Building libraries (sanitized)"

mkdir -p "${REPO_ROOT}"/build_libcxx
cd "${REPO_ROOT}"/build_libcxx

cmake -G Ninja ../runtimes \
    -DLLVM_TARGETS_TO_BUILD=AArch64 \
    -DLLVM_USE_SANITIZER="Address;Undefined" \
    -DLLVM_ENABLE_ASSERTIONS=ON \
    -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind;compiler-rt" \
    -DCMAKE_C_COMPILER="${REPO_ROOT}/build_llvm/bin/clang" \
    -DCMAKE_CXX_COMPILER="${REPO_ROOT}/build_llvm/bin/clang++" \
    -DLIBCXX_ENABLE_TIME_ZONE_DATABASE=OFF

ninja

echo "==> Stage 3: Building clang (sanitized)"

mkdir -p "${REPO_ROOT}"/build_sanitizer
cd "${REPO_ROOT}"/build_sanitizer

export LD_LIBRARY_PATH="${REPO_ROOT}"/build_libcxx/lib:$LD_LIBRARY_PATH

# Have chosen Address Sanitizer and Undefined Sanitizer to build and test.
# These sanitizers are most commonly used and relatively easy to set-up.
# These sanitizers will help to detect runtime issues related to use after free, 
# overflow (Heap buffer, stack buffer) etc.
# We have explicitly disabled memory leak detection, since the observed issues are
# unrelated to product under test.
cmake -G Ninja ../llvm \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_USE_SANITIZER="Address;Undefined" \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DCMAKE_C_COMPILER="${REPO_ROOT}/build_llvm/bin/clang" \
  -DCMAKE_CXX_COMPILER="${REPO_ROOT}/build_llvm/bin/clang++" \
  -DLLVM_LIT_ARGS="--ignore-fail --xunit-xml-output=lit_results.junit.xml \
  --param=env='ASAN_OPTIONS=verify_asan_link_order=1'" \
  -DCMAKE_INSTALL_PREFIX=../stage3.install \
  -DLLVM_TARGETS_TO_BUILD=AArch64 \
  -DLLVM_ENABLE_PROJECTS="clang;llvm;lld" \
  -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind;compiler-rt" \
  -DLIBCXX_ENABLE_TIME_ZONE_DATABASE=OFF

ninja
echo "==> Stage 3: Completed building clang (sanitized)"
