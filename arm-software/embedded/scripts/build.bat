
REM A bat script to build the Arm Toolchain for Embedded

set SCRIPT_DIR=%~dp0
for /f %%i in ('git -C %SCRIPT_DIR% rev-parse --show-toplevel') do set REPO_ROOT=%%i

vcvars64.bat

mkdir build
cd build

cmake %REPO_ROOT%\arm-software\embedded -GNinja -DFETCHCONTENT_QUIET=OFF
ninja package-llvm-toolchain
