#!/bin/bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# A bash script to run the tests from the Arm Toolchain for Linux, when sanitizers
# are enabled.

# The script assumes a successful build of the toolchain exists in the 'build_clang_with_sanitizer'
# directory inside the repository tree.

# Script does not exit, when the command in the script exits with a non-zero status.
# Also, print the line of the script being executed.
set -vx

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$( git -C "${SCRIPT_DIR}" rev-parse --show-toplevel )

cd "${REPO_ROOT}"/build_clang_with_sanitizer || exit

# Flag to disable memory leaks detection of LeakSanitizer.
export ASAN_OPTIONS=detect_leaks=0

# If a test fails, lit will ordinarily return a non-zero result,
# which prevents further testing. Setting the --ignore-fail option
# will cause testing to continue, so that CI systems can get a
# full set of results.
# Upstream clang and LLVM tests do not generate the junit xml results file by default.
# Additionally setting the --xunit-xml-output option store the
# results.
export LIT_ARGS="--ignore-fail --xunit-xml-output=lit_results.junit.xml --param=env='ASAN_OPTIONS=detect_leaks=0'"

ninja -v check-llvm
echo "check-llvm exit code: $?"

ninja -v check-clang
echo "check-clang exit code: $?"

ninja -v check-cxx
echo "check-cxx exit code: $?"

ninja -v check-cxxabi
echo "check-cxxabi exit code: $?"

ninja -v check-compiler-rt
echo "check-compiler-rt exit code: $?"

ninja -v check-unwind
echo "check-unwind exit code: $?"
