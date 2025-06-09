#!/bin/bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# A bash script to build the Arm Toolchain for Embedded, with address sanitizer enabled.

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

# Get processor count, to execute job in parallel threads
PROCESSOR_COUNT=$(getconf _NPROCESSORS_ONLN)

# Disable memory leaks detection of LeakSanitizer
export ASAN_OPTIONS=detect_leaks=0

if [[ ! -z "${FVP_INSTALL_DIR}" ]]; then
    EXTRA_CMAKE_ARGS="${EXTRA_CMAKE_ARGS} -DENABLE_FVP_TESTING=ON -DFVP_INSTALL_DIR=${FVP_INSTALL_DIR}"
fi

mkdir -p "${REPO_ROOT}"/build
cd "${REPO_ROOT}"/build

cmake ../arm-software/embedded -GNinja -DFETCHCONTENT_QUIET=OFF -DCMAKE_BUILD_TYPE=Release -DLLVM_USE_SANITIZER="Address" -DLLVM_ENABLE_ASSERTIONS=ON ${EXTRA_CMAKE_ARGS}

ninja -j$PROCESSOR_COUNT package-llvm-toolchain
