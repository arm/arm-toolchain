#!/bin/bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# A bash script to build the Arm Toolchain for Embedded

# The script creates a build of the toolchain in the 'build' directory, inside
# the repository tree.

set -ex

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$( git -C ${SCRIPT_DIR} rev-parse --show-toplevel )

clang --version

export CC=clang
export CXX=clang++

mkdir -p ${REPO_ROOT}/build
cd ${REPO_ROOT}/build

cmake ../arm-software/embedded -GNinja -DFETCHCONTENT_QUIET=OFF
ninja package-llvm-toolchain
