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
@if [%target%]==[run] goto :run
@if [%target%]==[clean] goto :clean
@echo Error: unknown target "%target%"
@exit /B 1

:build
@if [%BIN_PATH%]==[] goto :bin_path_empty
@call :build_fn
@exit /B

:run
@if exist hello.img goto :do_run
@if [%BIN_PATH%]==[] goto :bin_path_empty
@call :build_fn
:do_run
qemu-system-aarch64.exe -M raspi3b -semihosting -nographic -kernel hello.img
@exit /B

:clean
if exist hello.elf del /q hello.elf
if exist hello.img del /q hello.img
@exit /B

:bin_path_empty
@echo Error: BIN_PATH environment variable is not set
@exit /B 1

:build_fn
%BIN_PATH%\clang.exe --target=aarch64-none-elf -nostartfiles -lcrt0-semihost -lsemihost -Wl,--pic-veneer -mno-unaligned-access -g -T ..\..\ldscripts\raspi3b.ld -o hello.elf hello.c
%BIN_PATH%\llvm-objcopy.exe -O binary hello.elf hello.img
@exit /B
