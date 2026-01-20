#!/bin/bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# A bash script to run the tests from the Arm Toolchain for Linux, when sanitizers
# are enabled.

# The script assumes a successful build of the toolchain exists in the 'build_sanitizer'
# directory inside the repository tree.

# Script does not exit, when the command in the script exits with a non-zero status.
# Also, print the line of the script being executed.

# Disable CCACHE
export CCACHE_DISABLE=1

set -vx

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$( git -C "${SCRIPT_DIR}" rev-parse --show-toplevel )

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

# If a test fails, lit will ordinarily return a non-zero result,
# which prevents further testing. Setting the --ignore-fail option
# will cause testing to continue, so that CI systems can get a
# full set of results.
# Upstream clang and LLVM tests do not generate the junit xml results file by default.
# Additionally setting the --xunit-xml-output option store the
# results.
export LIT_ARGS="--ignore-fail --xunit-xml-output=lit_results.junit.xml"

# Provide the suppression file in the lit working dir so the
# compiler-rt test `suppressions-exec-relative-location.cpp` can find it.
SUPPRESSION_FILE="${REPO_ROOT}/build_sanitizer/bin/supp.txt"
echo "interceptor_via_fun:crash_function" > "${SUPPRESSION_FILE}"

LIT_FILTER_OUT='(MemorySanitizer-AARCH64 :: TestCases/(Linux/reexec_unlimited_stack)|SanitizerCommon-(asan|hwasan|msan|tsan)-aarch64-Linux :: TestCases/(Posix/mmap_write_exec.cpp))' ASAN_OPTIONS="verify_asan_link_order=1" ninja -v check-compiler-rt

ninja -v check-llvm
ninja -v check-clang
ninja -v check-cxx
ninja -v check-cxxabi
ninja -v check-unwind

