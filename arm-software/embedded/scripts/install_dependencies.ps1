# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# This script installs the essential build dependencies for ATfE.

# Upgrade pip and install dependencies
python -m pip install --upgrade pip
python -m pip install cmake==4.3.0 meson==1.2.3

# Ensure this step and later workflow steps use the cmake and meson installed
# by pip, rather than any preinstalled versions on the runner.
$pythonScriptsDir = python -c "import sysconfig; print(sysconfig.get_path('scripts'))"

$env:PATH = "$pythonScriptsDir;$env:PATH"
if ($env:GITHUB_PATH) {
    Add-Content -Path $env:GITHUB_PATH -Value $pythonScriptsDir
}

cmake --version
