
REM A bat script to run the tests from the Arm Toolchain for Embedded

REM The script assumes a successful build of the toolchain exists in the 'build'
REM directory inside the repository tree.

set SCRIPT_DIR=%~dp0
for /f %%i in ('git -C %SCRIPT_DIR% rev-parse --show-toplevel') do set REPO_ROOT=%%i

vcvars64.bat

cd %REPO_ROOT%\build
ninja check-llvm-toolchain
