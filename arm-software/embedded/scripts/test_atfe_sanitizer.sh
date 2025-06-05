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

cd "${REPO_ROOT}"/build

# The llvm-toolchain targets already set --xunit-xml-output so
# only the --ignore-fail option is needed.
# The picolibc tests do not use lit so do not support this option.
export LIT_OPTS="--ignore-fail"
ninja check-llvm-toolchain check-cxxabi check-unwind check-package-llvm-toolchain
