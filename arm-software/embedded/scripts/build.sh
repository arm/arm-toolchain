#!/bin/bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# A bash script to build the Arm Toolchain for Embedded

# The script creates a build of the toolchain in the 'build' directory, inside
# the repository tree.

# If FVPs have been installed, the environment variable `FVP_INSTALL_DIR`
# should be set to their install location to enable their use in tests.

set -ex

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$( git -C "${SCRIPT_DIR}" rev-parse --show-toplevel )

clang --version

export CC=clang
export CXX=clang++

EXTRA_CMAKE_ARGS=""
if [[ ! -z "${FVP_INSTALL_DIR}" ]]; then
    EXTRA_CMAKE_ARGS="${EXTRA_CMAKE_ARGS} -DENABLE_FVP_TESTING=ON -DFVP_INSTALL_DIR=${FVP_INSTALL_DIR}"
fi

mkdir -p "${REPO_ROOT}"/build
cd "${REPO_ROOT}"/build

cmake ../arm-software/embedded -GNinja -DFETCHCONTENT_QUIET=OFF ${EXTRA_CMAKE_ARGS}
ninja package-llvm-toolchain
