#
# Copyright (c) 2026, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

# This directory builds support libraries that are shared by multiple C library
# variants.
#
# libatomic-fallback.a contains weak fallback definitions for compiler-emitted
# __atomic_* libcalls. On targets without lock-free atomics these definitions
# are only suitable for non-preemptive bare-metal use; platforms that need real
# atomicity should provide strong replacements.

cmake_minimum_required(VERSION 3.20.0)
project(shared-support LANGUAGES C CXX ASM)

option(LLVM_USE_RELATIVE_PATHS_IN_MACROS "Use relative paths in macro expansions" OFF)
option(ENABLE_SHARED_SUPPORT_ATOMIC_FALLBACK "Build libatomic-fallback support" OFF)

if(LLVM_USE_RELATIVE_PATHS_IN_MACROS)
    file(RELATIVE_PATH relative_root "${CMAKE_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
    add_compile_options(
        "$<$<COMPILE_LANGUAGE:C,CXX,ASM>:-fmacro-prefix-map=${CMAKE_BINARY_DIR}=${relative_root}>"
        "$<$<COMPILE_LANGUAGE:C,CXX,ASM>:-fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}/=>"
    )
endif()

install(CODE "")

if(ENABLE_SHARED_SUPPORT_ATOMIC_FALLBACK)
    add_subdirectory(atomic-fallback)
    install(TARGETS atomic-fallback DESTINATION lib)
endif()
