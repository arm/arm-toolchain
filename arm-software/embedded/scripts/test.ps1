# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# A Powershell script to run the tests from the Arm Toolchain for Embedded

# The script assumes a successful build of the toolchain exists in the 'build'
# directory inside the repository tree.

$installPath = &"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -version 16.0 -property installationpath
Import-Module (Join-Path $installPath "Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
Enter-VsDevShell -VsInstallPath $installPath -SkipAutomaticLocation

$repoRoot = git -C $PSScriptRoot rev-parse --show-toplevel
$buildDir = (Join-Path $repoRoot build)

cd $buildDir

ninja check-llvm-toolchain
