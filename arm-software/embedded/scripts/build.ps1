
# A Powershell script to build the Arm Toolchain for Embedded

# The script creates a build of the toolchain in the 'build' directory, inside
# the repository tree.

$installPath = &"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -version 16.0 -property installationpath
Import-Module (Join-Path $installPath "Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
Enter-VsDevShell -VsInstallPath $installPath -SkipAutomaticLocation

$repoRoot = git -C $PSScriptRoot rev-parse --show-toplevel
$buildDir = (Join-Path $repoRoot build)

mkdir $buildDir
cd $buildDir

cmake ..\arm-software\embedded -GNinja -DFETCHCONTENT_QUIET=OFF
ninja package-llvm-toolchain
