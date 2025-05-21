# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# This script installs the essential build dependencies for ATfE.

# Fail on error
$ErrorActionPreference = "Stop"

# Ensure Chocolatey is available
if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
    Write-Error "Chocolatey is not installed!"
    exit 1
}

# Install packages via Chocolatey
choco install -y qemu --version=2024.12.20

# Upgrade pip and install meson
python -m pip install --upgrade pip
python -m pip install meson==1.2.3