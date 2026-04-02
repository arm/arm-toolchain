# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# This script installs the essential build dependencies for ATfE.

# Upgrade pip and install meson
python -m pip install --upgrade pip
python -m pip install meson==1.2.3
