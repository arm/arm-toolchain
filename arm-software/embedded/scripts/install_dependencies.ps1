# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# This script installs the essential build dependencies for ATfE.

# Use a virtual environment so CMake and pip use the same Python installation.
$venvDir = Join-Path $env:USERPROFILE ".atfe-venv"
$venvScripts = Join-Path $venvDir "Scripts"
$venvPython = Join-Path $venvScripts "python.exe"
python -m venv $venvDir
if ($LASTEXITCODE) { exit $LASTEXITCODE }

# Upgrade pip and install the Python build dependencies in the environment.
& $venvPython -m pip install --upgrade pip
if ($LASTEXITCODE) { exit $LASTEXITCODE }
& $venvPython -m pip install -r "$PSScriptRoot\requirements.txt"
if ($LASTEXITCODE) { exit $LASTEXITCODE }

$env:VIRTUAL_ENV = $venvDir
$env:PATH = "$venvScripts;$env:PATH"
if ($env:GITHUB_PATH) {
    Add-Content -Path $env:GITHUB_PATH -Value $venvScripts -Encoding utf8
}
if ($env:GITHUB_ENV) {
    Add-Content -Path $env:GITHUB_ENV -Value "VIRTUAL_ENV=$venvDir" -Encoding utf8
}
