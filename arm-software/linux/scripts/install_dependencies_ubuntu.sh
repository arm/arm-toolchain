#!/bin/bash

# Copyright (c) 2026, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# This script installs the essential build dependencies for ATfL build on Ubuntu OS.

set -euo pipefail

apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    bash \
    binutils-dev \
    build-essential \
    ca-certificates \
    ccache \
    clang \
    cmake \
    curl \
    debhelper \
    dpkg-dev \
    gettext-base \
    git \
    graphviz \
    libzstd-dev \
    make \
    ninja-build \
    python3 \
    python3-dev \
    python3-myst-parser \
    python3-pip \
    python3-pygments \
    python3-setuptools \
    python3-sphinx \
    python3-yaml \
    rpm \
    ruby-full \
    rubygems \
    valgrind \
    zlib1g-dev

# Install fpm
gem install --no-document fpm

rm -rf /var/lib/apt/lists/*
