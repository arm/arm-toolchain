
REM A bat script to build the Arm Toolchain for Embedded

REM The script creates a build of the toolchain in the 'build' directory, inside
REM the repository tree.

set SCRIPT_DIR=%~dp0
for /f %%i in ('git -C %SCRIPT_DIR% rev-parse --show-toplevel') do set REPO_ROOT=%%i

vcvars64.bat

mkdir %REPO_ROOT%\build
cd %REPO_ROOT%\build

cmake ..\arm-software\embedded -GNinja -DFETCHCONTENT_QUIET=OFF
ninja package-llvm-toolchain
