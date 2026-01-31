#!/bin/bash
set -e -u

git config --global --add safe.directory "${PWD}"
git config --global --add safe.directory "${PWD}/picolibc"

mkdir build/
cd build/
cmake -G Ninja -DFETCHCONTENT_SOURCE_DIR_PICOLIBC=../picolibc ../arm-software/embedded/
ninja llvm-toolchain
ninja package-llvm-toolchain
