
REM A bat script to run the tests from the Arm Toolchain for Embedded

vcvars64.bat

cd arm-toolchain\build
ninja check-llvm-toolchain
