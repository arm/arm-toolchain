#!/bin/bash

# A bash script to build the Arm Toolchain for Embedded

set -ex

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$( git -C ${SCRIPT_DIR} rev-parse --show-toplevel )

clang --version

export CC=clang
export CXX=clang++

mkdir build
cd build

cmake ${REPO_ROOT}/arm-software/embedded -GNinja -DFETCHCONTENT_QUIET=OFF
ninja package-llvm-toolchain
