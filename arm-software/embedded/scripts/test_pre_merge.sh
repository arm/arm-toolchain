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

cd "${REPO_ROOT}"/build
RESULTS_DIR="${REPO_ROOT}/build/test-results"
mkdir -p "${RESULTS_DIR}"
# Remove old reports so a missing new report cannot be mistaken for a pass.
rm -f "${RESULTS_DIR}"/*_lit_results.junit.xml

declare -a check_targets=(
    "check-all"
    "check-compiler-rt-armv7a_hard_vfpv3_d16_exn_rtti_unaligned"
    "check-compiler-rt-armv7m_hard_fpv5_d16_exn_rtti_unaligned_size"
    "check-cxx-armv7a_hard_vfpv3_d16_exn_rtti_unaligned"
    "check-cxx-armv7m_hard_fpv5_d16_exn_rtti_unaligned_size"
    "check-cxxabi-armv7a_hard_vfpv3_d16_exn_rtti_unaligned"
    "check-cxxabi-armv7m_hard_fpv5_d16_exn_rtti_unaligned_size"
    "check-picolibc-armv7a_hard_vfpv3_d16_exn_rtti_unaligned"
    "check-picolibc-armv7m_hard_fpv5_d16_exn_rtti_unaligned_size"
    "check-unwind-armv7a_hard_vfpv3_d16_exn_rtti_unaligned"
    "check-unwind-armv7m_hard_fpv5_d16_exn_rtti_unaligned_size"
)

# Finish all targets and check all reports before returning a failure.
status=0
for target in "${check_targets[@]}"
do
    export LIT_OPTS="--ignore-fail --xunit-xml-output=${RESULTS_DIR}/${target}_lit_results.junit.xml"
    ninja -k 0 "${target}" || status=1
done

for target in "${check_targets[@]}"
do
    python3 "${SCRIPT_DIR}"/fail_on_test_failures.py \
        "${RESULTS_DIR}" "${target}_lit_results.junit.xml" || status=1
done

exit "${status}"
