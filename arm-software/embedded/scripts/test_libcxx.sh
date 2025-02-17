#!/bin/bash

# A bash script to run the libcxx tests from the Arm Toolchain for Embedded.

# These tests are not included in the generic test.sh script due to the very
# long time required for them to complete.

# The script assumes a successful build of the toolchain exists in the 'build'
# directory inside the repository tree.

set -ex

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$( git -C ${SCRIPT_DIR} rev-parse --show-toplevel )

cd ${REPO_ROOT}/build
ninja check-cxx
