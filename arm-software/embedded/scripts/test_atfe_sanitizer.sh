#!/bin/bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# A bash script to run the tests from the Arm Toolchain for Embedded, when sanitizers
# are enabled.

# The script assumes a successful build of the toolchain exists in the 'build'
# directory inside the repository tree.

set -ex

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$( git -C "${SCRIPT_DIR}" rev-parse --show-toplevel )

# Get processor count, to execute job in parallel threads
PROCESSOR_COUNT=$(getconf _NPROCESSORS_ONLN)

cd "${REPO_ROOT}"/build

# The llvm-toolchain targets already set --xunit-xml-output so
# only the --ignore-fail option is needed.
# The picolibc tests do not use lit so do not support this option.
export LIT_OPTS="--ignore-fail"
ninja -j$PROCESSOR_COUNT check-all \
    check-compiler-rt-armv7a_hard_vfpv3_d16_exn_rtti_unaligned \
    check-picolibc-armv7a_hard_vfpv3_d16_exn_rtti_unaligned \
    check-cxx-armv7a_hard_vfpv3_d16_exn_rtti_unaligned \
    check-cxxabi-armv7a_hard_vfpv3_d16_exn_rtti_unaligned \
    check-unwind-armv7a_hard_vfpv3_d16_exn_rtti_unaligned \
    check-compiler-rt-armv7m_hard_fpv5_d16_exn_rtti_unaligned \
    check-picolibc-armv7m_hard_fpv5_d16_exn_rtti_unaligned \
    check-cxx-armv7m_hard_fpv5_d16_exn_rtti_unaligned \
    check-cxxabi-armv7m_hard_fpv5_d16_exn_rtti_unaligned \
    check-unwind-armv7m_hard_fpv5_d16_exn_rtti_unaligned
