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

# Get processor count, to execute job in parallel threads
PROCESSOR_COUNT=$(getconf _NPROCESSORS_ONLN)

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

# Run all relevant test targets using Ninja in parallel
ninja -j$PROCESSOR_COUNT \
    check-all \
    check-compiler-rt-armv7a_hard_vfpv3_d16_exn_rtti_unaligned \
    check-compiler-rt-armv7m_hard_fpv5_d16_exn_rtti_unaligned \
    check-cxx-armv7a_hard_vfpv3_d16_exn_rtti_unaligned \
    check-cxx-armv7m_hard_fpv5_d16_exn_rtti_unaligned \
    check-cxxabi-armv7a_hard_vfpv3_d16_exn_rtti_unaligned \
    check-cxxabi-armv7m_hard_fpv5_d16_exn_rtti_unaligned \
    check-picolibc-armv7a_hard_vfpv3_d16_exn_rtti_unaligned \
    check-picolibc-armv7m_hard_fpv5_d16_exn_rtti_unaligned \
    check-unwind-armv7a_hard_vfpv3_d16_exn_rtti_unaligned \
    check-unwind-armv7m_hard_fpv5_d16_exn_rtti_unaligned

