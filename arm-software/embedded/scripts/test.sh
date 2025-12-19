#!/bin/bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# A bash script to run the tests from the Arm Toolchain for Embedded.

# The script assumes a successful build of the toolchain exists in the 'build'
# directory inside the repository tree.

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
ninja check-all
ninja check-llvm-toolchain
ninja check-cxxabi
ninja check-unwind
ninja check-package-llvm-toolchain
