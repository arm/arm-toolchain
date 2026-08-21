#
# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

set(CRT0_COMMON_SOURCES
  bootcode.cpp
  data_segments.cpp
  exceptions.cpp
  memory.cpp
  misc.cpp
  sme_abi.cpp
  system_registers_a.cpp
  system_registers_m.cpp
  tls.cpp
)

# crt0 with nohost implementation of platform initialization and finalization
add_library(crt0 STATIC
  ${CRT0_COMMON_SOURCES}
  crt0-nohost.cpp
)

target_include_directories(crt0 PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# crt0 depending on semihosting implementation of platform initialization and
# finalization
add_library(crt0-semihost STATIC
  ${CRT0_COMMON_SOURCES}
)

target_include_directories(crt0-semihost PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_library(crt0-none STATIC
  crt0-none.c
)

if(LLVMLIBC_SUPPORT_ENABLE_DUMMYPACKEYS)
  add_library(dummypackeys-object OBJECT
    dummypackeys.cpp
  )

  target_include_directories(dummypackeys-object PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
  )

  set(dummypackeys_object_file
    ${CMAKE_CURRENT_BINARY_DIR}/dummypackeys.o
  )
  add_custom_command(
    OUTPUT ${dummypackeys_object_file}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_OBJECTS:dummypackeys-object>
      ${dummypackeys_object_file}
    DEPENDS dummypackeys-object
    COMMAND_EXPAND_LISTS
  )

  # Plain static archives do not reliably override a weak definition that has
  # already been extracted from libcrt0.a, so install this opt-in library as a
  # linker script that forces in the object providing the strong replacement.
  file(GENERATE
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libdummypackeys.a
    CONTENT "INPUT(dummypackeys.o)\n"
  )

  add_custom_target(dummypackeys ALL
    DEPENDS
      ${dummypackeys_object_file}
      ${CMAKE_CURRENT_BINARY_DIR}/libdummypackeys.a
  )

  install(
    FILES
      ${dummypackeys_object_file}
      ${CMAKE_CURRENT_BINARY_DIR}/libdummypackeys.a
    DESTINATION lib
  )
endif()
