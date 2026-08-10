#!/bin/bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# This script installs the essential build dependencies for ATfE.

set -e

sudo apt-get update && sudo apt-get install -y --no-install-recommends \
    clang \
    ccache \
    ninja-build \
    python3-pip \
    python3-setuptools \
    qemu-system-arm \
    ipxe-qemu

# Upgrade pip and install dependencies
python3 -m pip install --upgrade pip
python3 -m pip install --user cmake==4.3.0 meson==1.2.3 psutil==7.2.2 ruff==0.8.6

export PATH="${HOME}/.local/bin:${PATH}"
cmake --version
