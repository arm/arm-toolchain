#
# Copyright (c) 2022-2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

# This directory builds additional library files to go with llvm-libc:
#
# libcrt0.a contains an implementation of the _start() default entry point,
# which sets up the stack and does any necessary initialization before
# calling main().
#
# libsemihost.a implements llvm-libc's porting function API such as
# __llvm_libc_stdio_write, in terms of the Arm semihosting system.
#
# libdummypackeys.a is an opt-in test-only helper for M-profile PACBTI
# variants. It overrides the default architecture-extension hook with fixed
# dummy PAC keys.
#
# To use LLVM libc in a semihosting context, include both of these
# libraries. To use it in a different context, you will need to
# reimplement the same functions that libsemihost.a provides, but
# libcrt0.a might still be useful.

cmake_minimum_required(VERSION 3.20.0)
project(llvmlibc-support LANGUAGES C CXX ASM)

option(LLVM_USE_RELATIVE_PATHS_IN_MACROS "Use relative paths in macro expansions" OFF)

if(LLVM_USE_RELATIVE_PATHS_IN_MACROS)
    file(RELATIVE_PATH relative_root "${CMAKE_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
    add_compile_options(
        "$<$<COMPILE_LANGUAGE:C,CXX,ASM>:-fmacro-prefix-map=${CMAKE_BINARY_DIR}=${relative_root}>"
        "$<$<COMPILE_LANGUAGE:C,CXX,ASM>:-fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}/=>"
    )
endif()

option(
    LLVMLIBC_SUPPORT_ENABLE_DUMMYPACKEYS
    "Build the opt-in dummy PAC key helper library for M-profile PACBTI testing."
    OFF
)

add_subdirectory(crt0) # crt0, crt0-none and crt0-semihost
add_subdirectory(semihost)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/llvmlibc.ld.in
    ${CMAKE_CURRENT_BINARY_DIR}/llvmlibc.ld
    @ONLY
)

install(TARGETS crt0 crt0-none crt0-semihost semihost DESTINATION lib)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/llvmlibc.ld DESTINATION lib)
