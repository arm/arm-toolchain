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

if [[ ! -z "${FVP_INSTALL_DIR}" ]]; then
    EXTRA_CMAKE_ARGS="${EXTRA_CMAKE_ARGS} -DENABLE_FVP_TESTING=ON -DFVP_INSTALL_DIR=${FVP_INSTALL_DIR}"
fi

POST_MERGE_VARIANTS="armv6m_soft_nofp_size;\
armv6m_soft_nofp_exn_rtti_size;\
armebv6m_soft_nofp_size;\
armv7m_hard_fpv4_sp_d16_exn_rtti_size;\
armv8.1m.main_hard_nofp_mve_pacret_bti_exn_rtti_unaligned_size;\
armv7r_hard_vfpv3_d16;\
armv7a_hard_vfpv3_d16_exn_rtti;\
aarch64a_exn_rtti;\
aarch64a_be"

mkdir -p "${REPO_ROOT}"/build
cd "${REPO_ROOT}"/build

cmake ../arm-software/embedded \
    -GNinja \
    -DFETCHCONTENT_QUIET=OFF \
    -DCPACK_PACKAGE_DIRECTORY=atfe_packages \
    -DLLVM_CCACHE_BUILD=On \
    -DLLVM_TOOLCHAIN_ENABLE_PICOLIBC=OFF \
    -DLLVM_TOOLCHAIN_ENABLE_LLVMLIBC=ON \
    -DLLVM_TOOLCHAIN_LIBRARY_VARIANTS="${POST_MERGE_VARIANTS}" \
    ${EXTRA_CMAKE_ARGS}
ninja package-llvm-toolchain

# Remove CPack working directory.
rm -rf atfe_packages/_CPack_Packages
