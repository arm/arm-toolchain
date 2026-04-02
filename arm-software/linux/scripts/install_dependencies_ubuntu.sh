#!/bin/bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# This script installs the essential build dependencies for ATfL build on Ubuntu OS.

set -e

sudo apt-get update && sudo apt-get install -y --no-install-recommends \
    binutils-dev \
    build-essential \
    ccache \
    clang \
    graphviz \
    cmake \
    git \
    libzstd-dev \
    ninja-build \
    python3 \
    python3-dev \
    python3-myst-parser \
    python3-pip \
    python3-pygments \
    python3-setuptools \
    python3-yaml \
    zlib1g-dev \
    valgrind

