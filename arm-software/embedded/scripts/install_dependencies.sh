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
    cmake \
    ninja-build=1.10.1-1 \
    python3-pip \
    python3-setuptools \
    qemu-system-arm \
    ipxe-qemu

# Upgrade pip and install dependencies
python3 -m pip install --upgrade pip
python3 -m pip install meson==1.2.3 ruff==0.8.6
