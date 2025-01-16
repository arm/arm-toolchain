#!/bin/bash

# A bash script to run the tests from the Arm Toolchain for Embedded

cd arm-toolchain/build
ninja check-llvm-toolchain
