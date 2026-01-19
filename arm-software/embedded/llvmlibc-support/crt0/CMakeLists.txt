#
# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

# crt0 with nohost implementation of platform init and exit
add_library(crt0 STATIC
  bootcode.cpp
  crt0-nohost.cpp
)

target_include_directories(crt0 PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# crt0 depending on semihosting implementation of platform init and exit
add_library(crt0-semihost STATIC
  bootcode.cpp
)

target_include_directories(crt0-semihost PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_library(crt0-none STATIC
  crt0-none.c
)
