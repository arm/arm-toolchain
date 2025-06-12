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
    check-picolibc-aarch64a_exn_rtti_unaligned \
    check-compiler-rt-aarch64a_exn_rtti_unaligned \
    check-cxxabi-aarch64a_exn_rtti_unaligned \
    check-unwind-aarch64a_exn_rtti_unaligned \
    check-picolibc-armv8.1m.main_hard_nofp_mve_pacret_bti_exn_rtti \
    check-compiler-rt-armv8.1m.main_hard_nofp_mve_pacret_bti_exn_rtti \
    check-cxxabi-armv8.1m.main_hard_nofp_mve_pacret_bti_exn_rtti \
    check-unwind-armv8.1m.main_hard_nofp_mve_pacret_bti_exn_rtti
