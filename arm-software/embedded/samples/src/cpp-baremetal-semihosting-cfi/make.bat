@REM Copyright (c) 2023-2025, Arm Limited and affiliates.
@REM
@REM Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
@REM See https://llvm.org/LICENSE.txt for license information.
@REM SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

@if [%1]==[] goto :target_empty
@set target=%1
@goto :make
:target_empty
@set target=build

:make
@if [%target%]==[build] goto :build
@if [%target%]==[build-no-cfi] goto :build-no-cfi
@if [%target%]==[run] goto :run
@if [%target%]==[clean] goto :clean
@echo Error: unknown target "%target%"
@exit /B 1

:build
@if [%BIN_PATH%]==[] goto :bin_path_empty
@call :build_fn
@exit /B

:build-no-cfi
@if [%BIN_PATH%]==[] goto :bin_path_empty
@call :build_no_cfi_fn
@exit /B

:run
@if exist hello.hex goto :do_run
@if [%BIN_PATH%]==[] goto :bin_path_empty
@call :build_fn
:do_run
qemu-system-arm.exe -M microbit -semihosting -nographic -device loader,file=hello.hex
@exit /B

:clean
if exist hello.o del /q hello.o
if exist hello.elf del /q hello.elf
if exist hello.hex del /q hello.hex
@exit /B

:bin_path_empty
@echo Error: BIN_PATH environment variable is not set
@exit /B 1

:build_fn
%BIN_PATH%\clang++.exe --target=armv6m-none-eabi -mfloat-abi=soft -march=armv6m -mfpu=none -fno-exceptions -fno-rtti -flto -fsanitize=cfi -fvisibility=hidden -fno-sanitize-ignorelist -g -c hello.cpp
%BIN_PATH%\clang++.exe --target=armv6m-none-eabi -mfloat-abi=soft -march=armv6m -mfpu=none -nostartfiles -lcrt0-semihost -lsemihost -fno-exceptions -fno-rtti -flto -T ..\..\ldscripts\microbit.ld -g -o hello.elf hello.o
%BIN_PATH%\llvm-objcopy.exe -O ihex hello.elf hello.hex
@exit /B

:build_no_cfi_fn
%BIN_PATH%\clang++.exe --target=armv6m-none-eabi -mfloat-abi=soft -march=armv6m -mfpu=none -fno-exceptions -fno-rtti -flto -g -c hello.cpp
%BIN_PATH%\clang++.exe --target=armv6m-none-eabi -mfloat-abi=soft -march=armv6m -mfpu=none -nostartfiles -lcrt0-semihost -lsemihost -fno-exceptions -fno-rtti -flto -T ..\..\ldscripts\microbit.ld -g -o hello.elf hello.o
%BIN_PATH%\llvm-objcopy.exe -O ihex hello.elf hello.hex
@exit /B
