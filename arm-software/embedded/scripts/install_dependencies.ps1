# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# This script installs the essential build dependencies for ATfE.

# Upgrade pip and install dependencies
python -m pip install --upgrade pip
python -m pip install --user cmake==4.3.0 meson==1.2.3

# Ensure this step and later workflow steps use the `cmake.exe` installed by
# `pip install --user`, rather than any preinstalled CMake on the runner.
$pythonScriptsDir = python -c "import site; print(site.USER_BASE)"
$pythonScriptsDir = Join-Path $pythonScriptsDir "Scripts"

$env:PATH = "$pythonScriptsDir;$env:PATH"
if ($env:GITHUB_PATH) {
    Add-Content -Path $env:GITHUB_PATH -Value $pythonScriptsDir
}

cmake --version
