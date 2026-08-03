# cmake build script for elf2bin
#
# Copyright (c) 2022-2025, Arm Limited and affiliates.
#
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

# Specify which LLVM libraries we want to link with
set(LLVM_LINK_COMPONENTS
  Object
  Option
  Support
)

# Add the LLVM include directories (in both source and build dirs),
# and the elf2bin directory in the build dir where tablegen will write
# Opts.inc to.
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../llvm/include
  ${CMAKE_BINARY_DIR}/llvm/include
  ${CMAKE_BINARY_DIR}/elf2bin
)

# Turn off RTTI: the LLVM libraries are compiled without it, so we
# must compile without it too, or we'll get a link error trying to
# find the RTTI for LLVM types such as llvm::opt::OptTable.
if(LLVM_COMPILER_IS_GCC_COMPATIBLE)
  list(APPEND LLVM_COMPILE_FLAGS "-fno-rtti")
elseif(MSVC)
  list(APPEND LLVM_COMPILE_FLAGS "/GR-")
endif()

# Process Opts.td into a list of command-line options.
set(LLVM_TARGET_DEFINITIONS Opts.td)
tablegen(LLVM Opts.inc -gen-opt-parser-defs)
add_public_tablegen_target(Elf2BinOptsTableGen)

# Append elf2bin to LLVM_TOOLCHAIN_TOOLS, which will cause
# add_llvm_tool to set it as an installable target.
list(APPEND LLVM_TOOLCHAIN_TOOLS elf2bin)

# Build the elf2bin binary itself.
add_llvm_tool(elf2bin
  elf2bin.cpp
  elf.cpp
  bin.cpp
  hex.cpp
  DEPENDS
  Elf2BinOptsTableGen
)

# And run its Python-based test suite.
add_custom_target(check-elf2bin
  DEPENDS elf2bin
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/test.py
    $<TARGET_FILE:elf2bin>)
