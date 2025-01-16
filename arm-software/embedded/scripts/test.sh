#!/bin/bash

# A bash script to run the tests from the Arm Toolchain for Embedded.

# The script assumes a successful build of the toolchain exists in the 'build'
# directory inside the repository tree.

set -ex

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$( git -C ${SCRIPT_DIR} rev-parse --show-toplevel )

cd ${REPO_ROOT}/build
ninja check-llvm-toolchain
