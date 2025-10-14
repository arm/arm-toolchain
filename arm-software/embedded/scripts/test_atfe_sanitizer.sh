#!/bin/bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# A bash script to run the tests from the Arm Toolchain for Embedded, when sanitizers
# are enabled.

# The script assumes a successful build of the toolchain exists in the 'build'
# directory inside the repository tree.

# Script does not exit, when the command in the script exits with a non-zero status.
# Also, print the line of the script being executed.
set -vx

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$( git -C "${SCRIPT_DIR}" rev-parse --show-toplevel )

# If a test fails, lit will ordinarily return a non-zero result,
# which prevents further testing. Setting the --ignore-fail option
# will cause testing to continue, so that CI systems can get a
# full set of results.
# The lit test suites do not generate xml results by default.
# This can be enabled with the --xunit-xml-output option. The file
# written will be relative to the individual suite's build directly,
# so the same name can be used for all files for consistency.
export LIT_OPTS="--ignore-fail --xunit-xml-output=lit_results.junit.xml"

if [[ ! -d "${REPO_ROOT}/build_clang_with_sanitizer" ]]; then
  echo "Error: build directory not found. Run the sanitizer build first."
  exit 1
fi

# Command for each test is splitted across individual lines, to aid in debugging.
cd "${REPO_ROOT}"/build_clang_with_sanitizer
ninja check-all
ninja check-compiler-rt-armv7m_hard_fpv5_d16_exn_rtti_unaligned_size
ninja check-picolibc-armv7m_hard_fpv5_d16_exn_rtti_unaligned_size
ninja check-cxx-armv7m_hard_fpv5_d16_exn_rtti_unaligned_size
ninja check-cxxabi-armv7m_hard_fpv5_d16_exn_rtti_unaligned_size
ninja check-unwind-armv7m_hard_fpv5_d16_exn_rtti_unaligned_size
