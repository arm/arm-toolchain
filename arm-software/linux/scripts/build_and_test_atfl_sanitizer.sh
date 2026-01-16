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

set -vex

# Disable CCACHE
export CCACHE_DISABLE=1

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

# Flag to disable LeakSanitizer (memory leaks detection) in AddressSanitizer.
export ASAN_OPTIONS=verify_asan_link_order=1

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

# Start testing
cd "${REPO_ROOT}"/build_sanitizer || exit

export LD_LIBRARY_PATH="${REPO_ROOT}/build_llvm/lib:${LD_LIBRARY_PATH}"

# Use unsanitized symbolizer
if [[ ! -x "${REPO_ROOT}/build_llvm/bin/llvm-symbolizer" ]]; then
  ninja -C "${REPO_ROOT}/build_llvm" llvm-symbolizer || {
    echo "llvm-symbolizer missing; build stage1 (build_llvm) first." >&2
    exit 1
  }
fi
SYMBOLIZER_BIN="${REPO_ROOT}/build_llvm/bin/llvm-symbolizer"

# Prefer an unsanitized symbolizer
if [[ ! -x "${SYMBOLIZER_BIN}" ]] && command -v llvm-symbolizer >/dev/null 2>&1; then
  SYMBOLIZER_BIN="$(command -v llvm-symbolizer)"
fi

if [[ ! -x "${SYMBOLIZER_BIN}" ]]; then
  echo "Unsanitized llvm-symbolizer not found; build stage1 (build_llvm) first." >&2
  exit 1
fi

export PATH="$(dirname "${SYMBOLIZER_BIN}"):${PATH}"
export LLVM_SYMBOLIZER_PATH="${SYMBOLIZER_BIN}"
export ASAN_SYMBOLIZER_PATH="${SYMBOLIZER_BIN}"
export UBSAN_SYMBOLIZER_PATH="${SYMBOLIZER_BIN}"
export LSAN_SYMBOLIZER_PATH="${SYMBOLIZER_BIN}"
SYMBOLIZER_OPTS="external_symbolizer_path=${SYMBOLIZER_BIN}:symbolize=1"

# Use common options so LSAN tests that override LSAN_OPTIONS still get symbolization.
export SANITIZER_COMMON_OPTIONS="${SYMBOLIZER_OPTS}"
export LSAN_OPTIONS="${SYMBOLIZER_OPTS}:${LSAN_OPTIONS:-}"

# If a test fails, lit will ordinarily return a non-zero result,
# which prevents further testing. Setting the --ignore-fail option
# will cause testing to continue, so that CI systems can get a
# full set of results.
# Upstream clang and LLVM tests do not generate the junit xml results file by default.
# Additionally setting the --xunit-xml-output option store the
# results.
export LIT_ARGS="--ignore-fail --xunit-xml-output=lit_results.junit.xml --param=env='ASAN_OPTIONS=verify_asan_link_order=1'"

# Provide the suppression file in the lit working dir so the
# compiler-rt test `suppressions-exec-relative-location.cpp` can find it.
SUPPRESSION_FILE="${REPO_ROOT}/build_sanitizer/bin/supp.txt"
echo "interceptor_via_fun:crash_function" > "${SUPPRESSION_FILE}"

ninja -v check-compiler-rt
ninja -v check-llvm
ninja -v check-clang
ninja -v check-cxx
ninja -v check-cxxabi
ninja -v check-unwind
