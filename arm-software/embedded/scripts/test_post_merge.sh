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

# Run each configured lit test target with a separate JUnit result file.
cd "${REPO_ROOT}"/build
python3 "${SCRIPT_DIR}"/run_lit_tests_and_check_results.py \
    "${REPO_ROOT}"/build/test-results \
    --lit-opts="--ignore-fail" \
    --check-targets \
    check-all \
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
