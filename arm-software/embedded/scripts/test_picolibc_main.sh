#!/bin/bash

# Copyright (c) 2026, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# Run picolibc tests for every configured variant with tests enabled.
set -ex

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$( git -C "${SCRIPT_DIR}" rev-parse --show-toplevel )

cd "${REPO_ROOT}"/build
RESULTS_DIR="${REPO_ROOT}/build/multilib-picolibc-builds"
# Remove old reports so a missing new report cannot be mistaken for a pass.
find "${RESULTS_DIR}" -type f -name lit_results.junit.xml -delete
export LIT_OPTS="--ignore-fail --xunit-xml-output=lit_results.junit.xml"

status=0
ninja -k 0 check-picolibc || status=1
python3 "${SCRIPT_DIR}"/fail_on_test_failures.py \
    "${RESULTS_DIR}" lit_results.junit.xml || status=1

exit "${status}"
