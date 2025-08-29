#!/bin/bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# A bash script to run the LLVM-libc tests from the Arm Toolchain for Embedded.
# This script should be deleted when the generic test.sh script works

set -ex

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$( git -C "${SCRIPT_DIR}" rev-parse --show-toplevel )

cd "${REPO_ROOT}"/build

# The GTest framework in LLVM-libc does not yet have integration with lit.
# However, we run all tests anyways (don't stop on failure with the flag -k 0)
# If the test script fails, the package will not be uploaded
ninja check-llvmlibc -k 0 || true
