# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# A Powershell script to build the Arm Toolchain for Embedded

# The script creates a build of the toolchain in the 'build' directory, inside
# the repository tree.

$installPath = &"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -version 16.0 -property installationpath
Import-Module (Join-Path $installPath "Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
Enter-VsDevShell -VsInstallPath $installPath -SkipAutomaticLocation -DevCmdArguments "/arch=amd64"

$repoRoot = git -C $PSScriptRoot rev-parse --show-toplevel
$buildDir = (Join-Path $repoRoot build)

mkdir $buildDir
cd $buildDir

cmake ..\arm-software\embedded -GNinja -DFETCHCONTENT_QUIET=OFF -DCPACK_PACKAGE_DIRECTORY=atfe_packages -DLLVM_TOOLCHAIN_ENABLE_PICOLIBC=ON -DLLVM_TOOLCHAIN_ENABLE_LLVMLIBC=ON -DENABLE_QEMU_TESTING=OFF -DENABLE_FVP_TESTING=OFF -DLLVM_PARALLEL_LINK_JOBS=2 -DSHORT_BUILD_PATHS=ON
ninja -j 48 package-llvm-toolchain

# Remove CPack working directory.
Remove-Item -Recurse -Force atfe_packages/_CPack_Packages
