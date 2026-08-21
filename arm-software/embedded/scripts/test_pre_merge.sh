#!/bin/bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# A bash script to run pre-merge tests for the Arm Toolchain for Embedded.
#
# It assumes that a successful build of the toolchain already exists
# in the 'build' directory within the repository tree.

set -ex

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

# Run all relevant test targets using Ninja.
cd "${REPO_ROOT}"/build
ninja -k 0 check-all \
    check-compiler-rt-armv7a_hard_vfpv3_d16_exn_rtti_unaligned \
    check-compiler-rt-armv7m_hard_fpv5_d16_exn_rtti_unaligned_size \
    check-cxx-armv7a_hard_vfpv3_d16_exn_rtti_unaligned \
    check-cxx-armv7m_hard_fpv5_d16_exn_rtti_unaligned_size \
    check-cxxabi-armv7a_hard_vfpv3_d16_exn_rtti_unaligned \
    check-cxxabi-armv7m_hard_fpv5_d16_exn_rtti_unaligned_size \
    check-picolibc-armv7a_hard_vfpv3_d16_exn_rtti_unaligned \
    check-picolibc-armv7m_hard_fpv5_d16_exn_rtti_unaligned_size \
    check-unwind-armv7a_hard_vfpv3_d16_exn_rtti_unaligned \
    check-unwind-armv7m_hard_fpv5_d16_exn_rtti_unaligned_size
