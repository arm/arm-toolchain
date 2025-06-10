#!/bin/bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# A bash script to run post-merge tests for the Arm Toolchain for Embedded.
#
# It assumes that a successful build of the toolchain already exists
# in the 'build' directory within the repository tree.

set -ex

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$( git -C "${SCRIPT_DIR}" rev-parse --show-toplevel )

cd "${REPO_ROOT}"/build

# If a test fails, lit will ordinarily return a non-zero result,
# which prevents further testing. Setting the --ignore-fail option
# will cause testing to continue, so that CI systems can get a
# full set of results.
# The check-all target runs the upstream clang and LLVM tests,
# which do not generate a junit xml results file by default.
# Additionally setting the --xunit-xml-output option store the
# results.
export LIT_OPTS="--ignore-fail --xunit-xml-output=results.xml"
ninja check-all

# The llvm-toolchain targets already set --xunit-xml-output so
# only the --ignore-fail option is needed.
# The picolibc tests do not use lit so do not support this option.
export LIT_OPTS="--ignore-fail"
ninja check-picolibc-aarch64a_unaligned \
      check-compiler-rt-aarch64a_unaligned \
      check-cxxabi-aarch64a_unaligned \
      check-unwind-aarch64a_unaligned
