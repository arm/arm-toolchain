#!/bin/bash
set -e -u

git config --global --add safe.directory "${PWD}"
git config --global --add safe.directory "${PWD}/picolibc"

mkdir build/
cd build/
cmake -G Ninja -DFETCHCONTENT_SOURCE_DIR_PICOLIBC=../picolibc ../arm-software/embedded/

if [ "${1:-}" = "--dry-run" ]
then
    for arch in x86_64 AArch64
    do
        mkdir LVAT-Linux-${arch}
        mkdir LVAT-Linux-${arch}/bin LVAT-Linux-${arch}/include LVAT-Linux-${arch}/lib
        tar -cJf LVAT-Linux-${arch}.tar.xz LVAT-Linux-${arch}
    done
    exit
fi

ninja llvm-toolchain
ninja package-llvm-toolchain
